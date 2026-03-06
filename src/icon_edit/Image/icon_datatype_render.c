/*
 * icon_datatype_render.c - Datatypes-based fallback image renderer
 *
 * Implements itidy_render_via_datatype(): opens any image format via
 * datatypes.library / picture.datatype, reads full RGB/RGBA pixel data,
 * downscales to thumbnail size in RGB24 space, builds a 6x6x6 colour-cube
 * palette, and quantises the small thumbnail for final icon insertion.
 *
 * Also handles:
 *   - Optional RGBA alpha channel strip and area-average alpha threshold
 *   - Magenta key-colour transparency sweep for palette-based formats
 *   - Raw RGB24 output passthrough for Ultra mode callers
 *
 * See: icon_iff_render.h for the public function signature.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <string.h>

#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <proto/datatypes.h>
#include <proto/exec.h>

#include <console_output.h>

#include "icon_iff_render.h"
#include "icon_image_scale.h"
#include "../icon_image_access.h"
#include "../../writeLog.h"

/*========================================================================*/
/* itidy_render_via_datatype — Datatypes-based fallback renderer          */
/*========================================================================*/

/**
 * @brief Render IFF thumbnail via datatypes.library fallback.
 *
 * Optimized pipeline (v2 -- Feb 2026):
 *   1. Open image via datatypes, read full RGB24 pixel data
 *   2. Downscale RGB24 directly to thumbnail size (area-average in RGB space)
 *   3. Quantize only the tiny thumbnail pixels to 6x6x6 cube palette
 *      (direct index calculation, no brute-force search)
 *
 * This replaces the old pipeline which quantized ALL source pixels
 * (e.g. 148,512 for a 336x442 image) via brute-force palette search
 * before scaling.  The old approach took ~24 minutes on 68000; this
 * approach takes seconds.
 */
int itidy_render_via_datatype(const char *source_path,
                              iTidy_IFFRenderParams *params,
                              iTidy_IconImageData *dest_img,
                              BOOL ilbm_heuristics)
{
    Object *dt_obj = NULL;
    struct Library *DataTypesBase = NULL;
    struct BitMapHeader *bmhd = NULL;
    UBYTE *rgb24_buffer = NULL;
    UBYTE *thumb_rgb24  = NULL;
    UBYTE *thumb_chunky = NULL;
    ULONG src_width = 0, src_height = 0;
    ULONG display_width, display_height;
    ULONG rgb24_size;
    int result = ITIDY_PREVIEW_NOT_SUPPORTED;
    ULONG modeid;
    BOOL is_hires     = FALSE;
    BOOL is_lace      = FALSE;
    BOOL dt_has_alpha = FALSE;   /* PDTA_AlphaChannel result */
    UWORD bytes_per_pixel = 3;  /* 3 = RGB, 4 = RGBA */

    if (params != NULL)
        params->src_has_alpha = FALSE;  /* default: no alpha */

    log_debug(LOG_ICONS,
              "itidy_render_via_datatype: attempting datatype fallback for '%s'\n",
              source_path);

    /* Open datatypes.library */
    DataTypesBase = OpenLibrary("datatypes.library", 39);
    if (DataTypesBase == NULL)
    {
        log_error(LOG_ICONS,
                  "itidy_render_via_datatype: failed to open datatypes.library\n");
        return ITIDY_PREVIEW_NOT_SUPPORTED;
    }

    /* Progress: phase 1/3 -- starting datatype open */
    itidy_report_progress_throttled(params, "Loading image", 0, 3, 0);

    /* Open the image via datatypes */
    dt_obj = NewDTObject((APTR)source_path,
                         DTA_SourceType, DTST_FILE,
                         DTA_GroupID,    GID_PICTURE,
                         PDTA_Remap,     FALSE,  /* Don't remap - we want original colors */
                         TAG_DONE);

    if (dt_obj == NULL)
    {
        log_error(LOG_ICONS,
                  "itidy_render_via_datatype: NewDTObject failed for '%s'\n",
                  source_path);
        CloseLibrary(DataTypesBase);
        return ITIDY_PREVIEW_NOT_SUPPORTED;
    }

    /* Progress: phase 1/3 complete -- datatype opened */
    itidy_report_progress_throttled(params, "Loading image", 1, 3, 0);

    /* Get bitmap header for dimensions */
    if (GetDTAttrs(dt_obj, PDTA_BitMapHeader, &bmhd, TAG_DONE) != 1 || bmhd == NULL)
    {
        log_error(LOG_ICONS,
                  "itidy_render_via_datatype: failed to get BitMapHeader\n");
        goto cleanup;
    }

    src_width  = bmhd->bmh_Width;
    src_height = bmhd->bmh_Height;

    log_debug(LOG_ICONS,
              "itidy_render_via_datatype: image %lux%lu depth=%u\n",
              src_width, src_height, (unsigned)bmhd->bmh_Depth);

    if (src_width == 0 || src_height == 0 || src_width > 4096 || src_height > 4096)
    {
        log_error(LOG_ICONS,
                  "itidy_render_via_datatype: invalid dimensions\n");
        goto cleanup;
    }

    /* Check for alpha channel (PDTA_AlphaChannel requires picture.datatype V47+) */
    {
        ULONG alpha_val = 0;
        if (GetDTAttrs(dt_obj, PDTA_AlphaChannel, &alpha_val, TAG_DONE) == 1
            && alpha_val != 0)
        {
            dt_has_alpha    = TRUE;
            bytes_per_pixel = 4;
            log_debug(LOG_ICONS,
                     "itidy_render_via_datatype: PDTA_AlphaChannel=TRUE "
                     "-- using RGBA mode\n");
        }
        else
        {
            log_debug(LOG_ICONS,
                     "itidy_render_via_datatype: no alpha channel -- using RGB mode\n");
        }
    }

    /* Get mode ID for aspect ratio correction */
    if (GetDTAttrs(dt_obj, PDTA_ModeID, &modeid, TAG_DONE) == 1)
    {
        is_hires = (modeid & 0x8000) ? TRUE : FALSE;
        is_lace  = (modeid & 0x0004) ? TRUE : FALSE;
        log_debug(LOG_ICONS,
                 "itidy_render_via_datatype: ModeID=0x%08lX hires=%s lace=%s\n",
                 modeid, is_hires ? "yes" : "no", is_lace ? "yes" : "no");
    }

    /* Infer hires/interlace from pixel dimensions only when processing ILBM  */
    /* files where Amiga-specific screen modes apply.  PNG/JPEG/GIF/BMP use   */
    /* square pixels and these shortcuts would corrupt their dimensions.      */
    if (ilbm_heuristics)
    {
        if (!is_hires && src_width >= ITIDY_HIRES_WIDTH_THRESHOLD)
        {
            is_hires = TRUE;
            log_debug(LOG_ICONS,
                     "itidy_render_via_datatype: inferred hires from width %lu "
                     "(ModeID lacked HIRES flag)\n", src_width);
        }

        if (!is_lace && src_height >= 400)
        {
            is_lace = TRUE;
            log_debug(LOG_ICONS,
                     "itidy_render_via_datatype: inferred interlace from height %lu "
                     "(ModeID lacked LACE flag)\n", src_height);
        }
    }

    /* Apply aspect ratio corrections */
    display_width  = src_width;
    display_height = src_height;

    if (is_hires && src_width >= ITIDY_HIRES_WIDTH_THRESHOLD)
    {
        display_width = src_width / 2;
        log_debug(LOG_ICONS,
                  "itidy_render_via_datatype: hires correction: %lu -> %lu width\n",
                  src_width, display_width);
    }

    if (is_lace)
    {
        display_height = src_height / 2;
        log_debug(LOG_ICONS,
                  "itidy_render_via_datatype: interlace correction: %lu -> %lu height\n",
                  src_height, display_height);
    }

    /* Allocate RGB/RGBA buffer */
    rgb24_size  = src_width * src_height * (ULONG)bytes_per_pixel;
    rgb24_buffer = (UBYTE *)whd_malloc(rgb24_size);
    if (rgb24_buffer == NULL)
    {
        log_error(LOG_ICONS,
                  "itidy_render_via_datatype: failed to allocate RGB24 buffer (%lu bytes)\n",
                  rgb24_size);
        goto cleanup;
    }

    /* Read pixel data: RGB24 normally, RGBA32 when DT reported alpha */
    if (dt_has_alpha)
    {
        if (DoMethod(dt_obj, PDTM_READPIXELARRAY,
                     rgb24_buffer,
                     PBPAFMT_RGBA,
                     src_width * 4,
                     0, 0,
                     src_width, src_height)
            == 0)
        {
            /* Some DTs claim PDTA_AlphaChannel but don't support RGBA readback. */
            /* Fall back to RGB in that case.                                    */
            log_warning(LOG_ICONS,
                        "itidy_render_via_datatype: "
                        "RGBA readback failed -- retrying as RGB24\n");
            dt_has_alpha    = FALSE;
            bytes_per_pixel = 3;
            if (DoMethod(dt_obj, PDTM_READPIXELARRAY,
                         rgb24_buffer,
                         PBPAFMT_RGB,
                         src_width * 3,
                         0, 0,
                         src_width, src_height)
                == 0)
            {
                log_error(LOG_ICONS,
                          "itidy_render_via_datatype: PDTM_READPIXELARRAY failed\n");
                goto cleanup;
            }
        }
    }
    else
    {
        if (DoMethod(dt_obj, PDTM_READPIXELARRAY,
                     rgb24_buffer,
                     PBPAFMT_RGB,
                     src_width * 3,
                     0, 0,
                     src_width, src_height)
            == 0)
        {
            log_error(LOG_ICONS,
                      "itidy_render_via_datatype: PDTM_READPIXELARRAY failed\n");
            goto cleanup;
        }
    }

    /* Progress: phase 2/3 -- pixel data read */
    itidy_report_progress_throttled(params, "Decoding image", 2, 3, 0);

    log_debug(LOG_ICONS,
              "itidy_render_via_datatype: read %lux%lu %s pixels (%lu bytes)\n",
             src_width, src_height,
             dt_has_alpha ? "RGBA32" : "RGB24", rgb24_size);

    /*--------------------------------------------------------------------*/
    /* Calculate output thumbnail dimensions                               */
    /*--------------------------------------------------------------------*/
    {
        ULONG x_scale_fp, y_scale_fp, scale_fp;
        UWORD output_width, output_height;
        UWORD offset_x, offset_y;
        ULONG thumb_pixel_count;
        ULONG thumb_rgb24_size;

        /* Fixed-point 16.16: scale = source / dest */
        x_scale_fp = ((ULONG)display_width  << 16) / params->base.safe_width;
        y_scale_fp = ((ULONG)display_height << 16) / params->base.safe_height;

        /* Use larger scale to fit entirely within safe area */
        scale_fp = (x_scale_fp > y_scale_fp) ? x_scale_fp : y_scale_fp;
        if (scale_fp == 0)
        {
            scale_fp = 1 << 16;  /* 1:1 minimum */
        }

        /* No-upscale guard: if source fits entirely within the safe area
         * (scale_fp < 1.0) and upscaling is not requested, clamp scale to
         * 1:1 so the output dimensions never exceed the source dimensions.
         * The thumbnail will appear at its natural pixel size, centred in
         * the safe area, rather than being stretched to fill it.           */
        if (params != NULL && !params->allow_upscale && scale_fp < (1UL << 16))
        {
            log_debug(LOG_ICONS,
                      "itidy_render_via_datatype: no-upscale: "
                      "scale_fp=0x%08lX -> clamped to 1:1\n", scale_fp);
            scale_fp = 1UL << 16;
        }

        /* Output dimensions at this scale */
        output_width  = (UWORD)(((ULONG)display_width  << 16) / scale_fp);
        output_height = (UWORD)(((ULONG)display_height << 16) / scale_fp);

        /* Clamp to safe area */
        if (output_width  > params->base.safe_width)  output_width  = params->base.safe_width;
        if (output_height > params->base.safe_height) output_height = params->base.safe_height;

        /* Ensure minimum 1x1 */
        if (output_width  == 0) output_width  = 1;
        if (output_height == 0) output_height = 1;

        /* Center within safe area */
        offset_x = params->base.safe_left + (params->base.safe_width  - output_width)  / 2;
        offset_y = params->base.safe_top  + (params->base.safe_height - output_height) / 2;

        thumb_pixel_count = (ULONG)output_width * (ULONG)output_height;
        thumb_rgb24_size  = thumb_pixel_count * 3;

        log_debug(LOG_ICONS,
                 "itidy_render_via_datatype: scaling %lux%lu -> %ux%u "
                 "(display %lux%lu, safe %ux%u, offset %u,%u)\n",
                 src_width, src_height,
                 (unsigned)output_width, (unsigned)output_height,
                 display_width, display_height,
                 (unsigned)params->base.safe_width, (unsigned)params->base.safe_height,
                 (unsigned)offset_x, (unsigned)offset_y);

        /*----------------------------------------------------------------*/
        /* Step 1: Downscale to thumbnail size (area-average in RGB space) */
        /* When RGBA, extract only the RGB component for the scaler;      */
        /* alpha is harvested separately after PDTM_READPIXELARRAY.       */
        /*----------------------------------------------------------------*/
        thumb_rgb24 = (UBYTE *)whd_malloc(thumb_rgb24_size);
        if (thumb_rgb24 == NULL)
        {
            log_error(LOG_ICONS,
                      "itidy_render_via_datatype: failed to allocate thumbnail RGB24 (%lu bytes)\n",
                      thumb_rgb24_size);
            goto cleanup;
        }

        if (dt_has_alpha)
        {
            /* Strip alpha plane into a separate packed-RGB24 copy for the scaler. */
            /* Also compute average alpha per output pixel so we can threshold it.  */
            ULONG total_src_pixels = src_width * src_height;
            UBYTE *rgb_only   = (UBYTE *)whd_malloc(total_src_pixels * 3);
            UBYTE *alpha_only = (UBYTE *)whd_malloc(total_src_pixels);

            if (rgb_only == NULL || alpha_only == NULL)
            {
                log_error(LOG_ICONS,
                          "itidy_render_via_datatype: alpha split alloc failed\n");
                if (rgb_only)   whd_free(rgb_only);
                if (alpha_only) whd_free(alpha_only);
                goto cleanup;
            }

            /* Deinterleave RGBA -> packed RGB + packed alpha */
            {
                ULONG px;
                for (px = 0; px < total_src_pixels; px++)
                {
                    rgb_only[px * 3 + 0] = rgb24_buffer[px * 4 + 0];
                    rgb_only[px * 3 + 1] = rgb24_buffer[px * 4 + 1];
                    rgb_only[px * 3 + 2] = rgb24_buffer[px * 4 + 2];
                    alpha_only[px]       = rgb24_buffer[px * 4 + 3];
                }
            }

            /* Scale the RGB planes */
            rgb24_area_average_scale(rgb_only,
                                     (UWORD)src_width, (UWORD)src_height,
                                     thumb_rgb24,
                                     output_width, output_height,
                                     params);

            /* Scale the alpha plane and apply threshold */
            {
                UBYTE *thumb_alpha = (UBYTE *)whd_malloc(thumb_pixel_count);
                if (thumb_alpha != NULL)
                {
                    UWORD ax, ay;
                    ULONG x_fp = ((ULONG)src_width  << 16) / output_width;
                    ULONG y_fp = ((ULONG)src_height << 16) / output_height;

                    for (ay = 0; ay < output_height; ay++)
                    {
                        for (ax = 0; ax < output_width; ax++)
                        {
                            ULONG src_x0 = ((ULONG)ax * x_fp) >> 16;
                            ULONG src_y0 = ((ULONG)ay * y_fp) >> 16;
                            ULONG src_x1 = (((ULONG)(ax + 1) * x_fp) >> 16);
                            ULONG src_y1 = (((ULONG)(ay + 1) * y_fp) >> 16);
                            ULONG sum = 0, count = 0;
                            ULONG sy, sx;

                            if (src_x1 <= src_x0) src_x1 = src_x0 + 1;
                            if (src_y1 <= src_y0) src_y1 = src_y0 + 1;
                            if (src_x1 > src_width)  src_x1 = src_width;
                            if (src_y1 > src_height) src_y1 = src_height;

                            for (sy = src_y0; sy < src_y1; sy++)
                            {
                                for (sx = src_x0; sx < src_x1; sx++)
                                {
                                    sum += alpha_only[sy * src_width + sx];
                                    count++;
                                }
                            }

                            thumb_alpha[ay * output_width + ax] =
                                (count > 0) ? (UBYTE)(sum / count) : 0;
                        }
                    }

                    /* Quantize now, then apply alpha sweep, then free alpha buffer */
                    thumb_chunky = (UBYTE *)whd_malloc(thumb_pixel_count);
                    if (thumb_chunky == NULL)
                    {
                        log_error(LOG_ICONS,
                                  "itidy_render_via_datatype: failed to allocate thumbnail chunky\n");
                        whd_free(thumb_alpha);
                        whd_free(rgb_only);
                        whd_free(alpha_only);
                        goto cleanup;
                    }

                    quantize_rgb24_to_cube(thumb_rgb24, thumb_chunky,
                                           thumb_pixel_count,
                                           dest_img->palette_size_normal);

                    log_debug(LOG_ICONS,
                             "itidy_render_via_datatype: quantized %lu pixels; "
                             "alpha sweep (threshold=128)\n", thumb_pixel_count);

                    /* Alpha sweep: pixels with average alpha <= 128 get    */
                    /* mapped to palette index 0 (transparent placeholder). */
                    {
                        ULONG px;
                        BOOL found_transparent = FALSE;
                        for (px = 0; px < thumb_pixel_count; px++)
                        {
                            if (thumb_alpha[px] <= 128)
                            {
                                thumb_chunky[px] = 0;
                                found_transparent = TRUE;
                            }
                        }
                        if (found_transparent && params != NULL)
                        {
                            params->src_has_alpha = TRUE;
                            log_debug(LOG_ICONS,
                                     "itidy_render_via_datatype: "
                                     "transparent pixels found -- src_has_alpha=TRUE\n");
                        }
                    }

                    whd_free(thumb_alpha);
                }
                else
                {
                    /* Alpha buffer alloc failed -- treat as opaque */
                    log_warning(LOG_ICONS,
                                "itidy_render_via_datatype: "
                                "alpha thumbnail alloc failed -- alpha ignored\n");
                    thumb_chunky = (UBYTE *)whd_malloc(thumb_pixel_count);
                    if (thumb_chunky == NULL)
                    {
                        log_error(LOG_ICONS,
                                  "itidy_render_via_datatype: "
                                  "failed to allocate thumbnail chunky\n");
                        whd_free(rgb_only);
                        whd_free(alpha_only);
                        goto cleanup;
                    }
                    quantize_rgb24_to_cube(thumb_rgb24, thumb_chunky,
                                           thumb_pixel_count,
                                           dest_img->palette_size_normal);
                }
            }

            whd_free(alpha_only);
            whd_free(rgb_only);
        }
        else
        {
            /* No alpha: straightforward RGB scale (quantize done below) */
            rgb24_area_average_scale(rgb24_buffer,
                                     (UWORD)src_width, (UWORD)src_height,
                                     thumb_rgb24,
                                     output_width, output_height,
                                     params);
        }

        log_debug(LOG_ICONS,
                 "itidy_render_via_datatype: RGB24 downscaled to %ux%u (%lu pixels)\n",
                 (unsigned)output_width, (unsigned)output_height, thumb_pixel_count);

        /*----------------------------------------------------------------*/
        /* Step 2: Build 6x6x6 color cube palette + grayscale ramp       */
        /* Index 0 is reserved for transparent pixels when alpha present. */
        /*----------------------------------------------------------------*/
        {
            ULONG pal_idx = 0;
            ULONG r_step, g_step, b_step;

            /* Index 0: transparent placeholder (mid-grey) when alpha present */
            if (dt_has_alpha)
            {
                dest_img->palette_normal[0].red   = 0xAA;
                dest_img->palette_normal[0].green = 0xAA;
                dest_img->palette_normal[0].blue  = 0xAA;
                pal_idx = 1;  /* Start cube from index 1 */
            }

            /* Generate 6x6x6 RGB color cube */
            for (r_step = 0; r_step < 6 && pal_idx < dest_img->palette_size_normal; r_step++)
            {
                for (g_step = 0; g_step < 6 && pal_idx < dest_img->palette_size_normal; g_step++)
                {
                    for (b_step = 0; b_step < 6 && pal_idx < dest_img->palette_size_normal; b_step++)
                    {
                        dest_img->palette_normal[pal_idx].red   = (r_step * 255) / 5;
                        dest_img->palette_normal[pal_idx].green = (g_step * 255) / 5;
                        dest_img->palette_normal[pal_idx].blue  = (b_step * 255) / 5;
                        pal_idx++;
                    }
                }
            }

            /* Fill remaining with grayscale ramp */
            while (pal_idx < dest_img->palette_size_normal)
            {
                UBYTE gray = (UBYTE)((pal_idx * 255) / (dest_img->palette_size_normal - 1));
                dest_img->palette_normal[pal_idx].red   = gray;
                dest_img->palette_normal[pal_idx].green = gray;
                dest_img->palette_normal[pal_idx].blue  = gray;
                pal_idx++;
            }

            log_debug(LOG_ICONS,
                     "itidy_render_via_datatype: built %u-color palette "
                     "(6x6x6 cube + grays%s)\n",
                     (unsigned)dest_img->palette_size_normal,
                     dt_has_alpha ? ", index 0 reserved for alpha" : "");
        }

        /*----------------------------------------------------------------*/
        /* Step 3: Quantize tiny thumbnail (direct cube index calc)       */
        /* Non-alpha path only — alpha path already quantized above.      */
        /*----------------------------------------------------------------*/
        if (!dt_has_alpha)
        {
            thumb_chunky = (UBYTE *)whd_malloc(thumb_pixel_count);
            if (thumb_chunky == NULL)
            {
                log_error(LOG_ICONS,
                          "itidy_render_via_datatype: "
                          "failed to allocate thumbnail chunky\n");
                goto cleanup;
            }

            quantize_rgb24_to_cube(thumb_rgb24, thumb_chunky,
                                   thumb_pixel_count,
                                   dest_img->palette_size_normal);

            log_debug(LOG_ICONS,
                     "itidy_render_via_datatype: quantized %lu thumbnail pixels "
                     "to %u-color palette\n",
                     thumb_pixel_count, (unsigned)dest_img->palette_size_normal);

            /* -----------------------------------------------------------
             * Magenta key-colour transparency detection.
             *
             * Many Amiga PNG/GIF icons use solid magenta (#FF00FF) as a
             * transparency key colour instead of a proper alpha channel.
             * picture.datatype does NOT report PDTA_AlphaChannel=TRUE for
             * these files, so the RGBA path above is never entered.
             *
             * After quantisation, scan the palette for an entry matching
             * magenta (R>=240, G<=15, B>=240).  If found:
             *   1. Swap that palette entry with index 0.
             *   2. Remap all pixel indices accordingly.
             *   3. Set src_has_alpha=TRUE so the caller enables transparency.
             *
             * The 6x6x6 cube ALWAYS contains #FF00FF at index 185 (r=5,g=0,b=5).
             *
             * The caller sets params->try_magenta_key=FALSE for JPEG/BMP.
             * ----------------------------------------------------------- */
            if (params == NULL || params->try_magenta_key)
            {
                UWORD mi;
                BOOL  found_magenta = FALSE;
                UBYTE magenta_idx   = 0;

                for (mi = 0; mi < (UWORD)dest_img->palette_size_normal; mi++)
                {
                    if (dest_img->palette_normal[mi].red   >= 240 &&
                        dest_img->palette_normal[mi].green <= 15  &&
                        dest_img->palette_normal[mi].blue  >= 240)
                    {
                        magenta_idx  = (UBYTE)mi;
                        found_magenta = TRUE;
                        log_debug(LOG_ICONS,
                                 "itidy_render_via_datatype: "
                                 "magenta key colour found at palette index %u "
                                 "(R=%u G=%u B=%u)\n",
                                 (unsigned)magenta_idx,
                                 (unsigned)dest_img->palette_normal[mi].red,
                                 (unsigned)dest_img->palette_normal[mi].green,
                                 (unsigned)dest_img->palette_normal[mi].blue);
                        break;
                    }
                }

                if (found_magenta && magenta_idx != 0)
                {
                    /* Swap palette entries 0 and magenta_idx */
                    struct ColorRegister tmp_col;
                    ULONG px;

                    tmp_col = dest_img->palette_normal[0];
                    dest_img->palette_normal[0]           = dest_img->palette_normal[magenta_idx];
                    dest_img->palette_normal[magenta_idx] = tmp_col;

                    /* Remap pixels: magenta_idx -> 0 and 0 -> magenta_idx */
                    for (px = 0; px < thumb_pixel_count; px++)
                    {
                        if (thumb_chunky[px] == magenta_idx)
                            thumb_chunky[px] = 0;
                        else if (thumb_chunky[px] == 0)
                            thumb_chunky[px] = magenta_idx;
                    }

                    if (params != NULL)
                        params->src_has_alpha = TRUE;

                    log_debug(LOG_ICONS,
                             "itidy_render_via_datatype: "
                             "magenta key transparency applied -- "
                             "index 0 is now transparent, src_has_alpha=TRUE\n");
                }
                else if (found_magenta && magenta_idx == 0)
                {
                    /* Magenta is already at index 0 -- just flag alpha */
                    if (params != NULL)
                        params->src_has_alpha = TRUE;

                    log_debug(LOG_ICONS,
                             "itidy_render_via_datatype: "
                             "magenta key at index 0 -- src_has_alpha=TRUE\n");
                }
                else
                {
                    log_debug(LOG_ICONS,
                              "itidy_render_via_datatype: "
                              "no magenta key colour found in palette\n");
                }
            } /* end if try_magenta_key */
            else
            {
                log_debug(LOG_ICONS,
                         "itidy_render_via_datatype: "
                         "magenta key sweep skipped (format not transparency-capable)\n");
            }
        }
        else
        {
            /* Alpha path: thumb_chunky already allocated and quantized above */
            log_debug(LOG_ICONS,
                     "itidy_render_via_datatype: quantized %lu thumbnail pixels "
                     "to %u-color palette (alpha path)\n",
                     thumb_pixel_count, (unsigned)dest_img->palette_size_normal);
        }

        /* Progress: phase 3/3 -- complete */
        itidy_report_progress_throttled(params, "Scaling image", 3, 3, 0);

        /* Free thumbnail RGB24 - no longer needed, unless caller requested it.
         * When params->raw_rgb24_out is non-NULL the caller (e.g. Ultra mode)
         * needs the pre-quantisation RGB24 data.  Transfer ownership to the
         * caller instead of freeing; caller must whd_free() the buffer.       */
        if (params != NULL && params->raw_rgb24_out != NULL)
        {
            *params->raw_rgb24_out         = thumb_rgb24;
            params->raw_rgb24_pixel_count  = thumb_pixel_count;
            thumb_rgb24 = NULL;  /* Prevent cleanup path from freeing it */
            log_debug(LOG_ICONS,
                      "itidy_render_via_datatype: "
                      "transferred raw RGB24 (%lu pixels) to caller\n",
                      (unsigned long)thumb_pixel_count);
        }
        else
        {
            whd_free(thumb_rgb24);
            thumb_rgb24 = NULL;
        }

        /*----------------------------------------------------------------*/
        /* Step 4: Copy quantised thumbnail into destination icon buffer  */
        /*----------------------------------------------------------------*/
        {
            UWORD ty;
            for (ty = 0; ty < output_height; ty++)
            {
                const UBYTE *src_row = thumb_chunky + (ULONG)ty * (ULONG)output_width;
                UBYTE *dest_row = dest_img->pixel_data_normal
                                + (ULONG)(offset_y + ty) * (ULONG)dest_img->width
                                + (ULONG)offset_x;
                memcpy(dest_row, src_row, output_width);
            }
        }

        /* Store output dimensions for cropping */
        params->output_width    = output_width;
        params->output_height   = output_height;
        params->output_offset_x = offset_x;
        params->output_offset_y = offset_y;
    }

    log_debug(LOG_ICONS,
              "itidy_render_via_datatype: complete for '%s'\n", source_path);
    result = 0;

cleanup:
    if (thumb_chunky)  whd_free(thumb_chunky);
    if (thumb_rgb24)   whd_free(thumb_rgb24);
    if (rgb24_buffer)  whd_free(rgb24_buffer);
    if (dt_obj)        DisposeDTObject(dt_obj);
    if (DataTypesBase) CloseLibrary(DataTypesBase);
    return result;
}
