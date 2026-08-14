/*
 * image_scale.c - Indexed and RGB24 area-average scaling
 *
 * Extracted from iTidy2 icon_image_scale.c without algorithm changes.
 * Progress is an optional callback; logging is optional via image_set_log_fn().
 */

#include "image_scale.h"
#include "image_palette.h"

#include <platform/platform.h>

void image_scale_indexed(const UBYTE *src_chunky,
                         UWORD src_w, UWORD src_h,
                         UBYTE *dest_buf, UWORD dest_w, UWORD dest_h,
                         UWORD dest_stride,
                         UWORD dest_offset_x, UWORD dest_offset_y,
                         const iTidy_RGB8 *src_palette,
                         ULONG src_palette_size,
                         const iTidy_RGB8 *dest_palette,
                         ULONG dest_palette_size,
                         const iTidy_ImageProgress *progress)
{
    UWORD out_x, out_y;
    ULONG scale_x_fp;
    ULONG scale_y_fp;

    if (dest_w == 0 || dest_h == 0 || src_w == 0 || src_h == 0)
    {
        return;
    }

    if (src_palette == NULL || dest_palette == NULL)
    {
        image_log_warning("area_average_scale: NULL palette\n");
        return;
    }

    scale_x_fp = ((ULONG)src_w << 16) / (ULONG)dest_w;
    scale_y_fp = ((ULONG)src_h << 16) / (ULONG)dest_h;

    for (out_y = 0; out_y < dest_h; out_y++)
    {
        ULONG src_y_start_fp = (ULONG)out_y * scale_y_fp;
        ULONG src_y_end_fp   = src_y_start_fp + scale_y_fp;
        UWORD sy_start = (UWORD)(src_y_start_fp >> 16);
        UWORD sy_end   = (UWORD)(src_y_end_fp   >> 16);

        if (sy_end   > src_h) sy_end   = src_h;
        if (sy_start >= src_h) sy_start = src_h - 1;
        if (sy_end   <= sy_start) sy_end = sy_start + 1;

        for (out_x = 0; out_x < dest_w; out_x++)
        {
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

            if (sx_end   > src_w) sx_end   = src_w;
            if (sx_start >= src_w) sx_start = src_w - 1;
            if (sx_end   <= sx_start) sx_end = sx_start + 1;

            first_idx = src_chunky[(ULONG)sy_start * (ULONG)src_w + sx_start];
            if ((ULONG)first_idx >= src_palette_size) first_idx = 0;

            for (sy = sy_start; sy < sy_end; sy++)
            {
                const UBYTE *src_row = src_chunky + (ULONG)sy * (ULONG)src_w;

                for (sx = sx_start; sx < sx_end; sx++)
                {
                    UBYTE idx = src_row[sx];

                    if ((ULONG)idx >= src_palette_size)
                    {
                        idx = 0;
                    }

                    if (idx != first_idx) all_same = FALSE;

                    sum_r += src_palette[idx].r;
                    sum_g += src_palette[idx].g;
                    sum_b += src_palette[idx].b;
                    pixel_count++;
                }
            }

            dest_offset = (ULONG)(dest_offset_y + out_y) * (ULONG)dest_stride
                        + (ULONG)(dest_offset_x + out_x);

            if (all_same)
            {
                dest_buf[dest_offset] = first_idx;
                continue;
            }

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

            best_idx = image_palette_find_nearest(
                dest_palette, dest_palette_size, avg_r, avg_g, avg_b);

            dest_buf[dest_offset] = best_idx;
        }

        if (progress != NULL && progress->fn != NULL &&
            (out_y % 5 == 0 || out_y == dest_h - 1))
        {
            if (!progress->fn(progress->user_data, "Scaling image",
                              (ULONG)out_y + 1, (ULONG)dest_h))
            {
                image_log_info("area_average_scale: cancelled by user at row %u\n",
                               (unsigned)out_y);
                return;
            }
        }
    }

    image_log_debug("area_average_scale: %ux%u -> %ux%u (scale_x=0x%lX scale_y=0x%lX)\n",
                    (unsigned)src_w, (unsigned)src_h,
                    (unsigned)dest_w, (unsigned)dest_h,
                    scale_x_fp, scale_y_fp);
}

BOOL image_prefilter_2x2(const UBYTE *src, UWORD src_w, UWORD src_h,
                         UBYTE **out_buf, UWORD *out_w, UWORD *out_h,
                         const iTidy_RGB8 *palette,
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
        image_log_error("prefilter_2x2: alloc failed (%ux%u)\n",
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

            if ((ULONG)i0 >= palette_size) i0 = 0;
            if ((ULONG)i1 >= palette_size) i1 = 0;
            if ((ULONG)i2 >= palette_size) i2 = 0;
            if ((ULONG)i3 >= palette_size) i3 = 0;

            avg_r = ((ULONG)palette[i0].r + palette[i1].r
                   + palette[i2].r + palette[i3].r) / 4;
            avg_g = ((ULONG)palette[i0].g + palette[i1].g
                   + palette[i2].g + palette[i3].g) / 4;
            avg_b = ((ULONG)palette[i0].b + palette[i1].b
                   + palette[i2].b + palette[i3].b) / 4;

            half_buf[(ULONG)hy * (ULONG)half_w + hx] =
                image_palette_find_nearest(palette, palette_size,
                                           (UBYTE)avg_r, (UBYTE)avg_g,
                                           (UBYTE)avg_b);
        }
    }

    *out_buf = half_buf;
    *out_w   = half_w;
    *out_h   = half_h;

    image_log_debug("prefilter_2x2: %ux%u -> %ux%u\n",
                    (unsigned)src_w, (unsigned)src_h,
                    (unsigned)half_w, (unsigned)half_h);

    return TRUE;
}

void image_scale_rgb24(const UBYTE *src_rgb24,
                       UWORD src_w, UWORD src_h,
                       UBYTE *dest_rgb24,
                       UWORD dest_w, UWORD dest_h,
                       const iTidy_ImageProgress *progress)
{
    UWORD out_x, out_y;
    ULONG scale_x_fp, scale_y_fp;

    if (dest_w == 0 || dest_h == 0 || src_w == 0 || src_h == 0)
    {
        return;
    }

    scale_x_fp = ((ULONG)src_w << 16) / (ULONG)dest_w;
    scale_y_fp = ((ULONG)src_h << 16) / (ULONG)dest_h;

    for (out_y = 0; out_y < dest_h; out_y++)
    {
        ULONG src_y_start_fp = (ULONG)out_y * scale_y_fp;
        ULONG src_y_end_fp   = src_y_start_fp + scale_y_fp;
        UWORD sy_start = (UWORD)(src_y_start_fp >> 16);
        UWORD sy_end   = (UWORD)(src_y_end_fp   >> 16);
        UBYTE *dest_row = dest_rgb24 + (ULONG)out_y * (ULONG)dest_w * 3;

        if (sy_end   > src_h) sy_end   = src_h;
        if (sy_start >= src_h) sy_start = src_h - 1;
        if (sy_end   <= sy_start) sy_end = sy_start + 1;

        for (out_x = 0; out_x < dest_w; out_x++)
        {
            ULONG src_x_start_fp = (ULONG)out_x * scale_x_fp;
            ULONG src_x_end_fp   = src_x_start_fp + scale_x_fp;
            UWORD sx_start = (UWORD)(src_x_start_fp >> 16);
            UWORD sx_end   = (UWORD)(src_x_end_fp   >> 16);

            ULONG sum_r = 0, sum_g = 0, sum_b = 0;
            ULONG pixel_count = 0;
            UWORD sy, sx;

            if (sx_end   > src_w) sx_end   = src_w;
            if (sx_start >= src_w) sx_start = src_w - 1;
            if (sx_end   <= sx_start) sx_end = sx_start + 1;

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

        if (progress != NULL && progress->fn != NULL &&
            (out_y % 5 == 0 || out_y == dest_h - 1))
        {
            if (!progress->fn(progress->user_data, "Scaling image",
                              (ULONG)out_y + 1, (ULONG)dest_h))
            {
                image_log_info("rgb24_area_average_scale: cancelled at row %u\n",
                               (unsigned)out_y);
                return;
            }
        }
    }

    image_log_debug("rgb24_area_average_scale: %ux%u -> %ux%u "
                    "(scale_x=0x%lX scale_y=0x%lX)\n",
                    (unsigned)src_w, (unsigned)src_h,
                    (unsigned)dest_w, (unsigned)dest_h,
                    scale_x_fp, scale_y_fp);
}
