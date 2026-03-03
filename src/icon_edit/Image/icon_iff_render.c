/*
 * icon_iff_render.c - IFF ILBM Thumbnail Renderer (public API + orchestration)
 *
 * Progress reporting, high-level IFF render pipeline, and parameter cleanup.
 *
 * Implementation is split across:
 *   icon_iff_decode.c      - IFF/ILBM parsing, ByteRun1, bitplane->chunky
 *   icon_image_scale.c     - Area-average scalers, pre-filter, quantiser
 *   icon_datatype_render.c - datatypes.library fallback renderer
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <string.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <console_output.h>

#include "icon_iff_render.h"
#include "icon_image_scale.h"
#include "../icon_image_access.h"
#include "../../writeLog.h"

/*========================================================================*/
/* Progress Reporting Helper                                              */
/*========================================================================*/

/**
 * @brief Report progress with time-based throttling.
 *
 * Invokes the progress callback only if a callback is set AND either:
 *  - This is the final update (current == total), OR
 *  - At least min_ticks have elapsed since the last update
 *
 * Uses DOS ticks (TICKS_PER_SECOND = 50 on PAL, 60 on NTSC).
 */
BOOL itidy_report_progress_throttled(iTidy_IFFRenderParams *params,
                                     const char *phase,
                                     ULONG current,
                                     ULONG total,
                                     ULONG min_ticks)
{
    struct DateStamp ds;
    ULONG current_ticks;
    
    /* Debug: Log every call */
    log_debug(LOG_ICONS, "itidy_report_progress_throttled called: phase=%s, current=%lu, total=%lu, params=%p, callback=%p\n",
              phase ? phase : "NULL", current, total, (void*)params, 
              params ? (void*)params->progress_callback : NULL);
    
    /* Check for cancellation first */
    if (params != NULL && params->cancel_flag != NULL && *params->cancel_flag)
    {
        return FALSE;  /* Operation cancelled */
    }
    
    if (params == NULL || params->progress_callback == NULL)
    {
        log_debug(LOG_ICONS, "No callback available (params=%p, callback=%p)\n",
                  (void*)params, params ? (void*)params->progress_callback : NULL);
        return TRUE;  /* No callback, continue */
    }
    
    /* Always report final progress */
    if (current >= total)
    {
        params->progress_callback(params->progress_user_data, phase, current, total);
        params->last_progress_ticks = 0;  /* Reset for next phase */
        
        /* Log final progress update */
        log_debug(LOG_ICONS, "Progress: %s: %lu / %lu (complete)\n",
                  phase ? phase : "Processing", current, total);
        return TRUE;
    }
    
    /* Check if enough time has elapsed since last update */
    /* DateStamp has ticks (1/50th or 1/60th second), convert to simple tick count */
    /* ds.ds_Days = days, ds.ds_Minute = minutes, ds.ds_Tick = ticks (1/50 or 1/60 sec) */
    DateStamp(&ds);
    current_ticks = ds.ds_Tick +
                    (ds.ds_Minute * 60UL * TICKS_PER_SECOND) +
                    (ds.ds_Days * 86400UL * TICKS_PER_SECOND);  /* 86400 = 24*60*60 */
    
    if (current_ticks - params->last_progress_ticks >= min_ticks)
    {
        params->progress_callback(params->progress_user_data, phase, current, total);
        params->last_progress_ticks = current_ticks;
        
        /* Log progress update for post-analysis */
        log_debug(LOG_ICONS, "Progress: %s: %lu / %lu\n", 
                  phase ? phase : "Processing", current, total);
    }
    
    return TRUE;  /* Continue processing */
}

/*========================================================================*/
/* itidy_render_iff_thumbnail — Complete IFF rendering pipeline            */
/*========================================================================*/

int itidy_render_iff_thumbnail(const char *source_path,
                               iTidy_IFFRenderParams *iff_params,
                               iTidy_IconImageData *img)
{
    int parse_result;
    UWORD output_width, output_height;
    UWORD offset_x, offset_y;
    ULONG x_scale_fp, y_scale_fp, scale_fp;

    if (source_path == NULL || iff_params == NULL || img == NULL)
    {
        log_error(LOG_ICONS, "itidy_render_iff_thumbnail: NULL parameter\n");
        return -1;
    }

    /*--------------------------------------------------------------------*/
    /* Step 1: Parse the IFF file                                         */
    /*--------------------------------------------------------------------*/

    parse_result = itidy_iff_parse(source_path, iff_params);
    if (parse_result != 0)
    {
        return parse_result;  // Error or NOT_SUPPORTED
    }

    /*--------------------------------------------------------------------*/
    /* Step 2: Calculate scaling to fit safe area with aspect ratio        */
    /*--------------------------------------------------------------------*/

    if (iff_params->base.safe_width == 0 || iff_params->base.safe_height == 0)
    {
        log_error(LOG_ICONS, "itidy_render_iff_thumbnail: zero safe area\n");
        itidy_iff_params_free(iff_params);
        return -1;
    }

    // Fixed-point 16.16: scale = source / dest
    x_scale_fp = ((ULONG)iff_params->display_width << 16) / iff_params->base.safe_width;
    y_scale_fp = ((ULONG)iff_params->display_height << 16) / iff_params->base.safe_height;

    // Use the larger scale factor to fit entirely within safe area
    scale_fp = (x_scale_fp > y_scale_fp) ? x_scale_fp : y_scale_fp;

    if (scale_fp == 0)
    {
        scale_fp = 1 << 16;  // 1:1 minimum
    }

    // Output dimensions at this scale
    output_width  = (UWORD)(((ULONG)iff_params->display_width << 16) / scale_fp);
    output_height = (UWORD)(((ULONG)iff_params->display_height << 16) / scale_fp);

    // Clamp to safe area
    if (output_width > iff_params->base.safe_width)
    {
        output_width = iff_params->base.safe_width;
    }
    if (output_height > iff_params->base.safe_height)
    {
        output_height = iff_params->base.safe_height;
    }

    // Ensure minimum 1x1
    if (output_width == 0) output_width = 1;
    if (output_height == 0) output_height = 1;

    // Center within safe area (letterbox/pillarbox)
    offset_x = iff_params->base.safe_left + (iff_params->base.safe_width - output_width) / 2;
    offset_y = iff_params->base.safe_top + (iff_params->base.safe_height - output_height) / 2;

    // Store output dimensions for caller (enables post-render cropping)
    iff_params->output_width = output_width;
    iff_params->output_height = output_height;
    iff_params->output_offset_x = offset_x;
    iff_params->output_offset_y = offset_y;

    log_debug(LOG_ICONS, "itidy_render_iff_thumbnail: scaling %ux%u -> %ux%u "
             "(safe %ux%u, offset %u,%u)\n",
             (unsigned)iff_params->display_width,
             (unsigned)iff_params->display_height,
             (unsigned)output_width, (unsigned)output_height,
             (unsigned)iff_params->base.safe_width,
             (unsigned)iff_params->base.safe_height,
             (unsigned)offset_x, (unsigned)offset_y);

    /*--------------------------------------------------------------------*/
    /* Step 3: Fill safe area with background colour                      */
    /*--------------------------------------------------------------------*/

    if (iff_params->base.bg_color_index != ITIDY_NO_BG_COLOUR)
    {
        UWORD row;
        for (row = 0; row < iff_params->base.safe_height; row++)
        {
            UWORD y = iff_params->base.safe_top + row;
            if (y < img->height)
            {
                ULONG row_offset = (ULONG)y * (ULONG)img->width + iff_params->base.safe_left;
                UWORD fill_w = iff_params->base.safe_width;
                if (iff_params->base.safe_left + fill_w > img->width)
                {
                    fill_w = img->width - iff_params->base.safe_left;
                }
                memset(img->pixel_data_normal + row_offset,
                       iff_params->base.bg_color_index, fill_w);
            }
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 4: Replace icon palette with source CMAP                       */
    /*--------------------------------------------------------------------*/

    // Replace the icon's palette with the source image's palette
    if (img->palette_normal != NULL)
    {
        whd_free(img->palette_normal);
    }

    img->palette_normal = (struct ColorRegister *)whd_malloc(
        iff_params->src_palette_size * sizeof(struct ColorRegister));

    if (img->palette_normal == NULL)
    {
        log_error(LOG_ICONS, "itidy_render_iff_thumbnail: palette replacement alloc failed\n");
        itidy_iff_params_free(iff_params);
        return -1;
    }

    memcpy(img->palette_normal, iff_params->src_palette,
           iff_params->src_palette_size * sizeof(struct ColorRegister));
    img->palette_size_normal = iff_params->src_palette_size;

    log_debug(LOG_ICONS, "itidy_render_iff_thumbnail: replaced icon palette "
              "with source CMAP (%lu entries)\n", iff_params->src_palette_size);

    /*--------------------------------------------------------------------*/
    /* Step 5: Scale source pixels into the safe area                     */
    /*         Uses 2x2 pre-filter for extreme reduction ratios to cap    */
    /*         per-pixel work on 68020 while maintaining quality.         */
    /*--------------------------------------------------------------------*/

    {
        ULONG ratio_x = (output_width > 0) ?
            (ULONG)iff_params->display_width / (ULONG)output_width : 1;
        ULONG ratio_y = (output_height > 0) ?
            (ULONG)iff_params->display_height / (ULONG)output_height : 1;
        ULONG ratio = (ratio_x > ratio_y) ? ratio_x : ratio_y;
        ULONG src_area = (ULONG)iff_params->src_width * (ULONG)iff_params->src_height;

        if (ratio > ITIDY_SCALE_PREFILTER_RATIO && src_area > ITIDY_SCALE_PREFILTER_MIN_AREA)
        {
            // Pre-filter: halve the source image first, then area-average
            UBYTE *half_buf = NULL;
            UWORD half_w = 0, half_h = 0;

            log_debug(LOG_ICONS, "itidy_render_iff_thumbnail: ratio=%lu area=%lu — using 2x2 pre-filter\n",
                      ratio, src_area);

            if (prefilter_2x2(iff_params->src_chunky,
                              iff_params->src_width, iff_params->src_height,
                              &half_buf, &half_w, &half_h,
                              iff_params->src_palette,
                              iff_params->src_palette_size))
            {
                area_average_scale(half_buf, half_w, half_h,
                                   img->pixel_data_normal,
                                   output_width, output_height,
                                   img->width,
                                   offset_x, offset_y,
                                   iff_params->src_palette,
                                   iff_params->src_palette_size,
                                   img->palette_normal,
                                   img->palette_size_normal,
                                   iff_params);
                whd_free(half_buf);
            }
            else
            {
                // Pre-filter failed — fall back to direct scale
                log_warning(LOG_ICONS, "itidy_render_iff_thumbnail: prefilter failed, using direct scale\n");
                area_average_scale(iff_params->src_chunky,
                                   iff_params->src_width, iff_params->src_height,
                                   img->pixel_data_normal,
                                   output_width, output_height,
                                   img->width,
                                   offset_x, offset_y,
                                   iff_params->src_palette,
                                   iff_params->src_palette_size,
                                   img->palette_normal,
                                   img->palette_size_normal,
                                   iff_params);
            }
        }
        else
        {
            // Direct area-average — small enough to handle directly
            area_average_scale(iff_params->src_chunky,
                               iff_params->src_width, iff_params->src_height,
                               img->pixel_data_normal,
                               output_width, output_height,
                               img->width,
                               offset_x, offset_y,
                               iff_params->src_palette,
                               iff_params->src_palette_size,
                               img->palette_normal,
                               img->palette_size_normal,
                               iff_params);
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 6: Cleanup parsed source data                                 */
    /*--------------------------------------------------------------------*/

    itidy_iff_params_free(iff_params);

    log_debug(LOG_ICONS, "itidy_render_iff_thumbnail: complete for '%s'\n", source_path);
    return 0;
}

/*========================================================================*/
/* itidy_iff_params_free — Free IFF render params buffers                 */
/*========================================================================*/

void itidy_iff_params_free(iTidy_IFFRenderParams *params)
{
    if (params == NULL)
    {
        return;
    }

    if (params->src_chunky != NULL)
    {
        whd_free(params->src_chunky);
        params->src_chunky = NULL;
    }

    if (params->src_palette != NULL)
    {
        whd_free(params->src_palette);
        params->src_palette = NULL;
        params->src_palette_size = 0;
    }
}
