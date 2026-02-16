/*
 * ultra_downsample.c - Detail-Preserving Downsampling (Ultra Mode)
 *
 * See ultra_downsample.h for API documentation.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include "ultra_downsample.h"
#include "palette_mapping.h"
#include "palette_quantization.h"
#include "../../writeLog.h"
#include "../../platform/platform.h"  /* whd_malloc, whd_free */

#include <string.h>  /* memset */

/*========================================================================*/
/* Internal: Brightness calculation                                       */
/*========================================================================*/

/**
 * @brief Compute perceptual brightness (ITU-R BT.601 luminance).
 *
 * Integer-only: (306*R + 601*G + 117*B) >> 10
 */
static UBYTE compute_brightness(UBYTE r, UBYTE g, UBYTE b)
{
    return (UBYTE)((306L * (ULONG)r + 601L * (ULONG)g +
                    117L * (ULONG)b) >> 10);
}

/*========================================================================*/
/* itidy_ultra_downsample                                                 */
/*========================================================================*/

void itidy_ultra_downsample(const UBYTE *src_rgb,
                            UWORD src_w, UWORD src_h,
                            UBYTE *dest_rgb,
                            UWORD dest_w, UWORD dest_h)
{
    /* Use 16.16 fixed-point for precise scaling ratios */
    ULONG scale_x_fp, scale_y_fp;  /* Source pixels per dest pixel (16.16) */
    UWORD dest_x, dest_y;

    if (src_rgb == NULL || dest_rgb == NULL)
        return;

    if (src_w == 0 || src_h == 0 || dest_w == 0 || dest_h == 0)
        return;

    /* Fixed-point scale factors: how many source pixels per dest pixel */
    scale_x_fp = ((ULONG)src_w << 16) / (ULONG)dest_w;
    scale_y_fp = ((ULONG)src_h << 16) / (ULONG)dest_h;

    for (dest_y = 0; dest_y < dest_h; dest_y++)
    {
        /* Source Y range for this output pixel */
        UWORD src_y_start = (UWORD)((((ULONG)dest_y * scale_y_fp) >> 16));
        UWORD src_y_end   = (UWORD)(((((ULONG)dest_y + 1) * scale_y_fp) >> 16));

        if (src_y_end > src_h) src_y_end = src_h;
        if (src_y_start >= src_y_end) src_y_start = src_y_end > 0 ? src_y_end - 1 : 0;

        for (dest_x = 0; dest_x < dest_w; dest_x++)
        {
            /* Source X range for this output pixel */
            UWORD src_x_start = (UWORD)((((ULONG)dest_x * scale_x_fp) >> 16));
            UWORD src_x_end   = (UWORD)(((((ULONG)dest_x + 1) * scale_x_fp) >> 16));

            ULONG r_sum = 0, g_sum = 0, b_sum = 0;
            UBYTE max_brightness = 0;
            UBYTE max_r = 0, max_g = 0, max_b = 0;
            ULONG sample_count = 0;
            UWORD sy, sx;
            UBYTE avg_r, avg_g, avg_b;
            UBYTE avg_brightness;
            ULONG dest_offset;

            if (src_x_end > src_w) src_x_end = src_w;
            if (src_x_start >= src_x_end) src_x_start = src_x_end > 0 ? src_x_end - 1 : 0;

            /* Scan the source region */
            for (sy = src_y_start; sy < src_y_end; sy++)
            {
                for (sx = src_x_start; sx < src_x_end; sx++)
                {
                    ULONG src_offset = ((ULONG)sy * (ULONG)src_w + (ULONG)sx) * 3;
                    UBYTE r = src_rgb[src_offset + 0];
                    UBYTE g = src_rgb[src_offset + 1];
                    UBYTE b = src_rgb[src_offset + 2];
                    UBYTE brightness = compute_brightness(r, g, b);

                    /* Track brightest pixel for detail preservation */
                    if (brightness > max_brightness)
                    {
                        max_brightness = brightness;
                        max_r = r;
                        max_g = g;
                        max_b = b;
                    }

                    /* Accumulate for area average */
                    r_sum += r;
                    g_sum += g;
                    b_sum += b;
                    sample_count++;
                }
            }

            if (sample_count == 0)
                sample_count = 1;  /* Safety: avoid division by zero */

            /* Compute area-average color */
            avg_r = (UBYTE)(r_sum / sample_count);
            avg_g = (UBYTE)(g_sum / sample_count);
            avg_b = (UBYTE)(b_sum / sample_count);

            /* Detail preservation: boost if brightest pixel is
             * significantly brighter than the average */
            avg_brightness = compute_brightness(avg_r, avg_g, avg_b);

            if (avg_brightness > 0 &&
                max_brightness > (UBYTE)((UWORD)avg_brightness * ITIDY_ULTRA_DETAIL_THRESHOLD))
            {
                /* Blend: (1-boost) average + boost * brightest */
                WORD boost_r = (WORD)max_r - (WORD)avg_r;
                WORD boost_g = (WORD)max_g - (WORD)avg_g;
                WORD boost_b = (WORD)max_b - (WORD)avg_b;

                WORD new_r = (WORD)avg_r + (boost_r * ITIDY_ULTRA_BOOST_NUMERATOR) / ITIDY_ULTRA_BOOST_DENOMINATOR;
                WORD new_g = (WORD)avg_g + (boost_g * ITIDY_ULTRA_BOOST_NUMERATOR) / ITIDY_ULTRA_BOOST_DENOMINATOR;
                WORD new_b = (WORD)avg_b + (boost_b * ITIDY_ULTRA_BOOST_NUMERATOR) / ITIDY_ULTRA_BOOST_DENOMINATOR;

                avg_r = (new_r > 255) ? 255 : (new_r < 0) ? 0 : (UBYTE)new_r;
                avg_g = (new_g > 255) ? 255 : (new_g < 0) ? 0 : (UBYTE)new_g;
                avg_b = (new_b > 255) ? 255 : (new_b < 0) ? 0 : (UBYTE)new_b;
            }

            /* Write output pixel (RGB24) */
            dest_offset = ((ULONG)dest_y * (ULONG)dest_w + (ULONG)dest_x) * 3;
            dest_rgb[dest_offset + 0] = avg_r;
            dest_rgb[dest_offset + 1] = avg_g;
            dest_rgb[dest_offset + 2] = avg_b;
        }
    }

    log_debug(LOG_ICONS, "ultra_downsample: %ux%u -> %ux%u "
              "(detail-preserving)\n",
              (unsigned)src_w, (unsigned)src_h,
              (unsigned)dest_w, (unsigned)dest_h);
}

/*========================================================================*/
/* itidy_ultra_generate_palette                                           */
/*========================================================================*/

BOOL itidy_ultra_generate_palette(const UBYTE *rgb24, ULONG pixel_count,
                                  UWORD target_colors,
                                  struct ColorRegister *out_palette,
                                  UWORD *out_pal_size)
{
    /* Build a histogram directly from RGB24 data.
     * Since we can have up to 256*256*256 unique colors in RGB24,
     * but AmigaOS icons are small (max ~100x100 = 10,000 pixels),
     * we build the histogram by scanning unique colors directly. */
    iTidy_ColorEntry histogram[256];
    UWORD hist_count = 0;
    ULONG i;

    if (rgb24 == NULL || out_palette == NULL || out_pal_size == NULL)
        return FALSE;

    /* Build histogram from RGB24 data.
     * For small icon sizes, we can afford a simple O(n*m) scan
     * where n = pixels and m = unique colors found so far. */
    for (i = 0; i < pixel_count; i++)
    {
        UBYTE r = rgb24[i * 3 + 0];
        UBYTE g = rgb24[i * 3 + 1];
        UBYTE b = rgb24[i * 3 + 2];
        UWORD j;
        BOOL found = FALSE;

        /* Check if this color already exists in histogram */
        for (j = 0; j < hist_count; j++)
        {
            if (histogram[j].r == r && histogram[j].g == g &&
                histogram[j].b == b)
            {
                histogram[j].count++;
                found = TRUE;
                break;
            }
        }

        if (!found && hist_count < 256)
        {
            histogram[hist_count].r = r;
            histogram[hist_count].g = g;
            histogram[hist_count].b = b;
            histogram[hist_count]._pad = 0;
            histogram[hist_count].count = 1;
            hist_count++;
        }
        else if (!found)
        {
            /* Histogram full (256 unique colors found).
             * Map to closest existing entry. */
            UWORD best_j = 0;
            UWORD best_dist = 0xFFFF;

            for (j = 0; j < hist_count; j++)
            {
                UWORD dist = itidy_palette_manhattan_distance(
                    r, g, b,
                    histogram[j].r, histogram[j].g, histogram[j].b);
                if (dist < best_dist)
                {
                    best_dist = dist;
                    best_j = j;
                }
            }
            histogram[best_j].count++;
        }
    }

    log_debug(LOG_ICONS, "ultra_downsample: %u unique colors from %lu "
              "RGB24 pixels\n",
              (unsigned)hist_count, (unsigned long)pixel_count);

    /* If we already have <= target colors, use them directly */
    if (hist_count <= target_colors)
    {
        for (i = 0; i < hist_count; i++)
        {
            out_palette[i].red   = histogram[i].r;
            out_palette[i].green = histogram[i].g;
            out_palette[i].blue  = histogram[i].b;
        }
        *out_pal_size = hist_count;
        return TRUE;
    }

    /* Run Median Cut to reduce to target_colors */
    return itidy_median_cut(histogram, hist_count, target_colors,
                            out_palette, out_pal_size);
}

/*========================================================================*/
/* itidy_ultra_remap_to_indexed                                           */
/*========================================================================*/

void itidy_ultra_remap_to_indexed(const UBYTE *rgb24, ULONG pixel_count,
                                  const struct ColorRegister *palette,
                                  ULONG pal_size,
                                  UBYTE *out_pixels)
{
    ULONG i;

    if (rgb24 == NULL || palette == NULL || out_pixels == NULL)
        return;

    for (i = 0; i < pixel_count; i++)
    {
        UBYTE r = rgb24[i * 3 + 0];
        UBYTE g = rgb24[i * 3 + 1];
        UBYTE b = rgb24[i * 3 + 2];

        out_pixels[i] = itidy_palette_find_nearest(palette, pal_size,
                                                    r, g, b);
    }
}
