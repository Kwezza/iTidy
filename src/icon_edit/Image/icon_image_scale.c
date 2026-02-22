/*
 * icon_image_scale.c - Image scaling and quantisation routines
 *
 * Area-average downscalers (chunky-palette and RGB24), 2x2 pre-filter,
 * and direct 6x6x6 colour-cube quantiser.
 *
 * Shared between icon_iff_render.c (IFF/ILBM pipeline) and
 * icon_datatype_render.c (datatypes fallback pipeline).
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <string.h>

#include <console_output.h>

#include "icon_iff_render.h"
#include "icon_image_scale.h"
#include "../../writeLog.h"

/*========================================================================*/
/* Internal only: multiply-free palette search (Manhattan distance)       */
/*========================================================================*/

/**
 * @brief Find the closest palette entry to an RGB value using Manhattan
 *        distance (L1 norm) instead of Euclidean distance (L2 norm).
 *
 * On a 68000 without hardware multiply, Euclidean distance requires 3x
 * MULS instructions per palette entry (~70 cycles each = 210 cycles).
 * Manhattan distance uses only subtractions and absolute values (~30
 * cycles total), giving approximately 7x faster inner loop.
 *
 * For thumbnail palette matching with small palettes (16-64 entries),
 * the perceptual quality difference versus Euclidean is negligible.
 */
static UBYTE find_closest_color_fast(const struct ColorRegister *palette,
                                     ULONG palette_size,
                                     UBYTE target_r, UBYTE target_g,
                                     UBYTE target_b)
{
    ULONG best_dist = 999999UL;
    UBYTE best_idx  = 0;
    ULONG i;

    for (i = 0; i < palette_size; i++)
    {
        LONG dr = (LONG)palette[i].red   - (LONG)target_r;
        LONG dg = (LONG)palette[i].green - (LONG)target_g;
        LONG db = (LONG)palette[i].blue  - (LONG)target_b;
        ULONG dist;

        // Manhattan distance — no multiplies needed
        if (dr < 0) dr = -dr;
        if (dg < 0) dg = -dg;
        if (db < 0) db = -db;
        dist = (ULONG)(dr + dg + db);

        if (dist < best_dist)
        {
            best_dist = dist;
            best_idx  = (UBYTE)i;
            if (dist == 0) break;  // Exact match — can't do better
        }
    }

    return best_idx;
}

/*========================================================================*/
/* area_average_scale — Area-Average Downscaler (Fixed-Point 16.16)      */
/*========================================================================*/

/**
 * @brief Area-average downscale source image into destination buffer.
 *
 * Uses fixed-point (16.16) arithmetic only — no floating point.
 * Each output pixel maps to a rectangle in the source image. All source
 * pixels within that rectangle have their RGB values looked up via
 * src_palette, summed, and averaged. The resulting average RGB is then
 * matched to the closest entry in dest_palette.
 *
 * In PICTURE palette mode (src_palette == dest_palette), the source pixel
 * indices are used directly for palette lookup and the dest_palette match
 * finds the same or nearest index — this is the common fast path.
 */
void area_average_scale(const UBYTE *src_chunky,
                        UWORD src_w, UWORD src_h,
                        UBYTE *dest_buf, UWORD dest_w, UWORD dest_h,
                        UWORD dest_stride,
                        UWORD dest_offset_x, UWORD dest_offset_y,
                        const struct ColorRegister *src_palette,
                        ULONG src_palette_size,
                        const struct ColorRegister *dest_palette,
                        ULONG dest_palette_size,
                        iTidy_IFFRenderParams *params)
{
    UWORD out_x, out_y;
    ULONG scale_x_fp;  // Fixed 16.16: src pixels per output pixel (horizontal)
    ULONG scale_y_fp;  // Fixed 16.16: src pixels per output pixel (vertical)

    if (dest_w == 0 || dest_h == 0 || src_w == 0 || src_h == 0)
    {
        return;
    }

    if (src_palette == NULL || dest_palette == NULL)
    {
        log_warning(LOG_ICONS, "area_average_scale: NULL palette\n");
        return;
    }

    // Fixed-point 16.16 scale factors: how many source pixels per output pixel
    scale_x_fp = ((ULONG)src_w << 16) / (ULONG)dest_w;
    scale_y_fp = ((ULONG)src_h << 16) / (ULONG)dest_h;

    for (out_y = 0; out_y < dest_h; out_y++)
    {
        // Source Y range for this output row (fixed-point -> integer bounds)
        ULONG src_y_start_fp = (ULONG)out_y * scale_y_fp;
        ULONG src_y_end_fp   = src_y_start_fp + scale_y_fp;
        UWORD sy_start = (UWORD)(src_y_start_fp >> 16);
        UWORD sy_end   = (UWORD)(src_y_end_fp   >> 16);

        // Clamp to source bounds
        if (sy_end   > src_h) sy_end   = src_h;
        if (sy_start >= src_h) sy_start = src_h - 1;
        if (sy_end   <= sy_start) sy_end = sy_start + 1;

        for (out_x = 0; out_x < dest_w; out_x++)
        {
            // Source X range for this output column
            ULONG src_x_start_fp = (ULONG)out_x * scale_x_fp;
            ULONG src_x_end_fp   = src_x_start_fp + scale_x_fp;
            UWORD sx_start = (UWORD)(src_x_start_fp >> 16);
            UWORD sx_end   = (UWORD)(src_x_end_fp   >> 16);

            ULONG sum_r = 0, sum_g = 0, sum_b = 0;
            ULONG pixel_count = 0;
            UWORD sy, sx;
            UBYTE avg_r, avg_g, avg_b;
            UBYTE best_idx;
            ULONG dest_offset;
            UBYTE first_idx;
            BOOL  all_same = TRUE;

            // Clamp to source bounds
            if (sx_end   > src_w) sx_end   = src_w;
            if (sx_start >= src_w) sx_start = src_w - 1;
            if (sx_end   <= sx_start) sx_end = sx_start + 1;

            // Get first pixel index for same-index fast path
            first_idx = src_chunky[(ULONG)sy_start * (ULONG)src_w + sx_start];
            if ((ULONG)first_idx >= src_palette_size) first_idx = 0;

            // Sum RGB values and track whether all pixels share the same index
            for (sy = sy_start; sy < sy_end; sy++)
            {
                const UBYTE *src_row = src_chunky + (ULONG)sy * (ULONG)src_w;

                for (sx = sx_start; sx < sx_end; sx++)
                {
                    UBYTE idx = src_row[sx];

                    // Clamp index to palette size
                    if ((ULONG)idx >= src_palette_size)
                    {
                        idx = 0;
                    }

                    if (idx != first_idx) all_same = FALSE;

                    sum_r += src_palette[idx].red;
                    sum_g += src_palette[idx].green;
                    sum_b += src_palette[idx].blue;
                    pixel_count++;
                }
            }

            // Destination offset for this output pixel
            dest_offset = (ULONG)(dest_offset_y + out_y) * (ULONG)dest_stride
                        + (ULONG)(dest_offset_x + out_x);

            // Fast path: all source pixels have the same palette index —
            // no RGB averaging or palette search needed
            if (all_same)
            {
                dest_buf[dest_offset] = first_idx;
                continue;
            }

            // Average the accumulated RGB
            if (pixel_count > 0)
            {
                avg_r = (UBYTE)(sum_r / pixel_count);
                avg_g = (UBYTE)(sum_g / pixel_count);
                avg_b = (UBYTE)(sum_b / pixel_count);
            }
            else
            {
                avg_r = 0;
                avg_g = 0;
                avg_b = 0;
            }

            // Find closest match in dest palette (multiply-free Manhattan)
            best_idx = find_closest_color_fast(
                dest_palette, dest_palette_size, avg_r, avg_g, avg_b);

            dest_buf[dest_offset] = best_idx;
        }

        /* Report progress every 5 rows (throttled to ~1 second) */
        if (params != NULL && (out_y % 5 == 0 || out_y == dest_h - 1))
        {
            if (!itidy_report_progress_throttled(params, "Scaling image",
                                                 out_y + 1, dest_h, TICKS_PER_SECOND))
            {
                log_info(LOG_ICONS,
                         "area_average_scale: cancelled by user at row %u\n",
                         (unsigned)out_y);
                return;  /* Abort scaling */
            }
        }
    }

    log_debug(LOG_ICONS,
              "area_average_scale: %ux%u -> %ux%u (scale_x=0x%lX scale_y=0x%lX)\n",
              (unsigned)src_w, (unsigned)src_h,
              (unsigned)dest_w, (unsigned)dest_h,
              scale_x_fp, scale_y_fp);
}

/*========================================================================*/
/* prefilter_2x2 — Half-size pre-filter for extreme reduction ratios      */
/*========================================================================*/

/**
 * @brief Pre-filter: average 2x2 blocks of source pixels to half-size buffer.
 *
 * For extreme reduction ratios (>8:1 on large images), this pre-filter
 * halves the source dimensions first.  The area-average scaler then works
 * on the smaller buffer, capping per-pixel work and keeping quality high.
 */
BOOL prefilter_2x2(const UBYTE *src, UWORD src_w, UWORD src_h,
                   UBYTE **out_buf, UWORD *out_w, UWORD *out_h,
                   const struct ColorRegister *palette,
                   ULONG palette_size)
{
    UWORD half_w, half_h;
    UBYTE *half_buf;
    UWORD hx, hy;

    *out_buf = NULL;
    *out_w   = 0;
    *out_h   = 0;

    if (src == NULL || palette == NULL || src_w < 2 || src_h < 2)
    {
        return FALSE;
    }

    half_w = src_w / 2;
    half_h = src_h / 2;

    half_buf = (UBYTE *)whd_malloc((ULONG)half_w * (ULONG)half_h);
    if (half_buf == NULL)
    {
        log_error(LOG_ICONS, "prefilter_2x2: alloc failed (%ux%u)\n",
                  (unsigned)half_w, (unsigned)half_h);
        return FALSE;
    }

    for (hy = 0; hy < half_h; hy++)
    {
        UWORD src_y = hy * 2;
        const UBYTE *row0 = src + (ULONG)src_y       * (ULONG)src_w;
        const UBYTE *row1 = src + (ULONG)(src_y + 1) * (ULONG)src_w;

        for (hx = 0; hx < half_w; hx++)
        {
            UWORD src_x = hx * 2;
            UBYTE i0 = row0[src_x];
            UBYTE i1 = row0[src_x + 1];
            UBYTE i2 = row1[src_x];
            UBYTE i3 = row1[src_x + 1];
            ULONG avg_r, avg_g, avg_b;

            // Clamp indices to palette size
            if ((ULONG)i0 >= palette_size) i0 = 0;
            if ((ULONG)i1 >= palette_size) i1 = 0;
            if ((ULONG)i2 >= palette_size) i2 = 0;
            if ((ULONG)i3 >= palette_size) i3 = 0;

            // Average the 4 pixels' RGB values
            avg_r = ((ULONG)palette[i0].red   + palette[i1].red
                   + palette[i2].red   + palette[i3].red)   / 4;
            avg_g = ((ULONG)palette[i0].green + palette[i1].green
                   + palette[i2].green + palette[i3].green) / 4;
            avg_b = ((ULONG)palette[i0].blue  + palette[i1].blue
                   + palette[i2].blue  + palette[i3].blue)  / 4;

            // Find closest palette match for the averaged color
            half_buf[(ULONG)hy * (ULONG)half_w + hx] =
                find_closest_color_fast(palette, palette_size,
                                       (UBYTE)avg_r, (UBYTE)avg_g,
                                       (UBYTE)avg_b);
        }
    }

    *out_buf = half_buf;
    *out_w   = half_w;
    *out_h   = half_h;

    log_debug(LOG_ICONS, "prefilter_2x2: %ux%u -> %ux%u\n",
              (unsigned)src_w, (unsigned)src_h,
              (unsigned)half_w, (unsigned)half_h);

    return TRUE;
}

/*========================================================================*/
/* rgb24_area_average_scale — Downscale RGB24 data directly              */
/*========================================================================*/

/**
 * @brief Area-average downscale operating directly on RGB24 pixel data.
 *
 * Unlike the chunky-palette area_average_scale(), this operates on raw
 * RGB triplets.  It averages the R, G, B channels of all source pixels
 * that map to each output pixel, producing a small RGB24 thumbnail.
 *
 * Key optimisation for the datatypes path: downscale the large RGB24
 * buffer (e.g. 336x442 = 148K pixels) to thumbnail size (e.g. 64x42 =
 * 2,688 pixels) BEFORE quantising.  This reduces quantisation work ~55x.
 */
void rgb24_area_average_scale(const UBYTE *src_rgb24,
                              UWORD src_w, UWORD src_h,
                              UBYTE *dest_rgb24,
                              UWORD dest_w, UWORD dest_h,
                              iTidy_IFFRenderParams *params)
{
    UWORD out_x, out_y;
    ULONG scale_x_fp, scale_y_fp;

    if (dest_w == 0 || dest_h == 0 || src_w == 0 || src_h == 0)
    {
        return;
    }

    // Fixed-point 16.16 scale factors: source pixels per output pixel
    scale_x_fp = ((ULONG)src_w << 16) / (ULONG)dest_w;
    scale_y_fp = ((ULONG)src_h << 16) / (ULONG)dest_h;

    for (out_y = 0; out_y < dest_h; out_y++)
    {
        // Source Y range for this output row
        ULONG src_y_start_fp = (ULONG)out_y * scale_y_fp;
        ULONG src_y_end_fp   = src_y_start_fp + scale_y_fp;
        UWORD sy_start = (UWORD)(src_y_start_fp >> 16);
        UWORD sy_end   = (UWORD)(src_y_end_fp   >> 16);
        UBYTE *dest_row = dest_rgb24 + (ULONG)out_y * (ULONG)dest_w * 3;

        // Clamp to source bounds
        if (sy_end   > src_h) sy_end   = src_h;
        if (sy_start >= src_h) sy_start = src_h - 1;
        if (sy_end   <= sy_start) sy_end = sy_start + 1;

        for (out_x = 0; out_x < dest_w; out_x++)
        {
            // Source X range for this output column
            ULONG src_x_start_fp = (ULONG)out_x * scale_x_fp;
            ULONG src_x_end_fp   = src_x_start_fp + scale_x_fp;
            UWORD sx_start = (UWORD)(src_x_start_fp >> 16);
            UWORD sx_end   = (UWORD)(src_x_end_fp   >> 16);

            ULONG sum_r = 0, sum_g = 0, sum_b = 0;
            ULONG pixel_count = 0;
            UWORD sy, sx;

            // Clamp to source bounds
            if (sx_end   > src_w) sx_end   = src_w;
            if (sx_start >= src_w) sx_start = src_w - 1;
            if (sx_end   <= sx_start) sx_end = sx_start + 1;

            // Sum RGB values of all source pixels in this rectangle
            for (sy = sy_start; sy < sy_end; sy++)
            {
                const UBYTE *src_row = src_rgb24 + (ULONG)sy * (ULONG)src_w * 3;

                for (sx = sx_start; sx < sx_end; sx++)
                {
                    const UBYTE *px = src_row + (ULONG)sx * 3;
                    sum_r += px[0];
                    sum_g += px[1];
                    sum_b += px[2];
                    pixel_count++;
                }
            }

            // Average the accumulated RGB and write output
            if (pixel_count > 0)
            {
                dest_row[out_x * 3]     = (UBYTE)(sum_r / pixel_count);
                dest_row[out_x * 3 + 1] = (UBYTE)(sum_g / pixel_count);
                dest_row[out_x * 3 + 2] = (UBYTE)(sum_b / pixel_count);
            }
            else
            {
                dest_row[out_x * 3]     = 0;
                dest_row[out_x * 3 + 1] = 0;
                dest_row[out_x * 3 + 2] = 0;
            }
        }

        // Report progress every 5 rows (throttled to ~1 second)
        if (params != NULL && (out_y % 5 == 0 || out_y == dest_h - 1))
        {
            if (!itidy_report_progress_throttled(params, "Scaling image",
                                                 out_y + 1, dest_h, TICKS_PER_SECOND))
            {
                log_info(LOG_ICONS,
                         "rgb24_area_average_scale: cancelled at row %u\n",
                         (unsigned)out_y);
                return;
            }
        }
    }

    log_debug(LOG_ICONS,
              "rgb24_area_average_scale: %ux%u -> %ux%u "
              "(scale_x=0x%lX scale_y=0x%lX)\n",
              (unsigned)src_w, (unsigned)src_h,
              (unsigned)dest_w, (unsigned)dest_h,
              scale_x_fp, scale_y_fp);
}

/*========================================================================*/
/* quantize_rgb24_to_cube — Fast 6x6x6 cube quantisation (no search)    */
/*========================================================================*/

/**
 * @brief Quantize RGB24 pixels to a 6x6x6 color cube palette by direct
 *        index calculation — NO brute-force palette search needed.
 *
 * For the known 6x6x6 cube palette, the nearest palette index for any
 * RGB value is computed by rounding each channel to the nearest of the
 * 6 steps (0, 51, 102, 153, 204, 255) then computing:
 *    index = r_idx * 36 + g_idx * 6 + b_idx
 *
 * This replaces brute-force palette search, giving approximately 100x
 * speedup per pixel on 68000.
 */
void quantize_rgb24_to_cube(const UBYTE *src_rgb24,
                            UBYTE *dest_chunky,
                            ULONG pixel_count,
                            ULONG palette_size)
{
    ULONG i;
    const UBYTE *rgb_ptr = src_rgb24;
    UBYTE *out_ptr = dest_chunky;

    for (i = 0; i < pixel_count; i++)
    {
        UBYTE r = *rgb_ptr++;
        UBYTE g = *rgb_ptr++;
        UBYTE b = *rgb_ptr++;

        // Map each channel to the nearest of 6 steps: 0,51,102,153,204,255
        // The step boundaries are at 25, 76, 127, 178, 229
        // Formula: idx = (val + 25) / 51, clamped to 0-5
        UWORD r_idx = (UWORD)(r + 25) / 51;
        UWORD g_idx = (UWORD)(g + 25) / 51;
        UWORD b_idx = (UWORD)(b + 25) / 51;

        // Clamp to valid range (handles overflow from 255+25=280)
        if (r_idx > 5) r_idx = 5;
        if (g_idx > 5) g_idx = 5;
        if (b_idx > 5) b_idx = 5;

        // Direct palette index: r*36 + g*6 + b
        *out_ptr++ = (UBYTE)(r_idx * 36 + g_idx * 6 + b_idx);
    }

    // Suppress unused parameter warning if palette_size not used
    (void)palette_size;
}
