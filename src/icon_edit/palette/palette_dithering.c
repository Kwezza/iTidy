/*
 * palette_dithering.c - Dithering Algorithms for Palette Reduction
 *
 * See palette_dithering.h for API documentation.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include "palette_dithering.h"
#include "palette_mapping.h"
#include "../../writeLog.h"
#include "../../platform/platform.h"  /* whd_malloc, whd_free */

#include <string.h>  /* memset */

/*========================================================================*/
/* 4x4 Bayer Ordered Dithering Matrix                                     */
/*========================================================================*/

/**
 * 4x4 Bayer threshold matrix (range 0-15).
 * Accessed as: bayer_4x4[(y & 3) * 4 + (x & 3)]
 */
static const UBYTE bayer_4x4[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
};

/*========================================================================*/
/* itidy_dither_bayer_offset                                              */
/*========================================================================*/

WORD itidy_dither_bayer_offset(UWORD pixel_x, UWORD pixel_y)
{
    UBYTE threshold = bayer_4x4[(pixel_y & 3) * 4 + (pixel_x & 3)];

    /* Center range from 0-15 to -7..+8 */
    return (WORD)threshold - 7;
}

/*========================================================================*/
/* itidy_dither_floyd_steinberg                                           */
/*========================================================================*/

BOOL itidy_dither_floyd_steinberg(UBYTE *pixels, UWORD width, UWORD height,
                                  const struct ColorRegister *old_palette,
                                  ULONG old_pal_size,
                                  const struct ColorRegister *new_palette,
                                  ULONG new_pal_size)
{
    /* Error buffers: 2 rows per channel (current + next row)
     * Using WORD (16-bit signed) for error values */
    WORD *err_r = NULL;
    WORD *err_g = NULL;
    WORD *err_b = NULL;
    WORD *curr_r, *next_r;
    WORD *curr_g, *next_g;
    WORD *curr_b, *next_b;
    ULONG row_size;
    UWORD x, y;

    if (pixels == NULL || old_palette == NULL || new_palette == NULL)
        return FALSE;

    if (width == 0 || height == 0)
        return TRUE;  /* Nothing to do */

    /* Allocate error buffers: 2 rows x width per channel */
    row_size = (ULONG)width * sizeof(WORD);

    err_r = (WORD *)whd_malloc(row_size * 2);
    err_g = (WORD *)whd_malloc(row_size * 2);
    err_b = (WORD *)whd_malloc(row_size * 2);

    if (err_r == NULL || err_g == NULL || err_b == NULL)
    {
        log_error(LOG_ICONS, "palette_dithering: F-S error buffer alloc "
                  "failed (width=%u)\n", (unsigned)width);
        if (err_r) whd_free(err_r);
        if (err_g) whd_free(err_g);
        if (err_b) whd_free(err_b);
        return FALSE;
    }

    /* Initialize error buffers to zero */
    memset(err_r, 0, row_size * 2);
    memset(err_g, 0, row_size * 2);
    memset(err_b, 0, row_size * 2);

    /* Set up current/next row pointers */
    curr_r = err_r;              next_r = err_r + width;
    curr_g = err_g;              next_g = err_g + width;
    curr_b = err_b;              next_b = err_b + width;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            ULONG pix_offset = (ULONG)y * (ULONG)width + (ULONG)x;
            UBYTE old_idx = pixels[pix_offset];
            WORD adj_r, adj_g, adj_b;
            UBYTE chosen_idx;
            WORD err_r_val, err_g_val, err_b_val;

            /* Get original RGB from old palette */
            if ((ULONG)old_idx < old_pal_size)
            {
                adj_r = (WORD)old_palette[old_idx].red   + curr_r[x];
                adj_g = (WORD)old_palette[old_idx].green  + curr_g[x];
                adj_b = (WORD)old_palette[old_idx].blue   + curr_b[x];
            }
            else
            {
                adj_r = curr_r[x];
                adj_g = curr_g[x];
                adj_b = curr_b[x];
            }

            /* Clamp to valid range */
            if (adj_r < 0) adj_r = 0; else if (adj_r > 255) adj_r = 255;
            if (adj_g < 0) adj_g = 0; else if (adj_g > 255) adj_g = 255;
            if (adj_b < 0) adj_b = 0; else if (adj_b > 255) adj_b = 255;

            /* Find closest color in new palette */
            chosen_idx = itidy_palette_find_nearest(
                new_palette, new_pal_size,
                (UBYTE)adj_r, (UBYTE)adj_g, (UBYTE)adj_b);

            pixels[pix_offset] = chosen_idx;

            /* Calculate quantization error */
            err_r_val = adj_r - (WORD)new_palette[chosen_idx].red;
            err_g_val = adj_g - (WORD)new_palette[chosen_idx].green;
            err_b_val = adj_b - (WORD)new_palette[chosen_idx].blue;

            /* Distribute error to neighbors using bit shifts:
             * 7/16 to right, 3/16 to below-left, 5/16 to below,
             * 1/16 to below-right.
             * Using (err * N + 8) >> 4 for rounding. */

            /* Right neighbor: 7/16 */
            if (x + 1 < width)
            {
                curr_r[x + 1] += (err_r_val * 7 + 8) >> 4;
                curr_g[x + 1] += (err_g_val * 7 + 8) >> 4;
                curr_b[x + 1] += (err_b_val * 7 + 8) >> 4;
            }

            /* Below-left: 3/16 */
            if (x > 0)
            {
                next_r[x - 1] += (err_r_val * 3 + 8) >> 4;
                next_g[x - 1] += (err_g_val * 3 + 8) >> 4;
                next_b[x - 1] += (err_b_val * 3 + 8) >> 4;
            }

            /* Below: 5/16 */
            next_r[x] += (err_r_val * 5 + 8) >> 4;
            next_g[x] += (err_g_val * 5 + 8) >> 4;
            next_b[x] += (err_b_val * 5 + 8) >> 4;

            /* Below-right: 1/16 */
            if (x + 1 < width)
            {
                next_r[x + 1] += (err_r_val * 1 + 8) >> 4;
                next_g[x + 1] += (err_g_val * 1 + 8) >> 4;
                next_b[x + 1] += (err_b_val * 1 + 8) >> 4;
            }
        }

        /* Swap current and next row buffers */
        {
            WORD *tmp;
            tmp = curr_r; curr_r = next_r; next_r = tmp;
            tmp = curr_g; curr_g = next_g; next_g = tmp;
            tmp = curr_b; curr_b = next_b; next_b = tmp;
        }

        /* Clear the new "next" row (was previous "current") */
        memset(next_r, 0, row_size);
        memset(next_g, 0, row_size);
        memset(next_b, 0, row_size);
    }

    /* Cleanup */
    whd_free(err_r);
    whd_free(err_g);
    whd_free(err_b);

    log_debug(LOG_ICONS, "palette_dithering: Floyd-Steinberg applied "
              "(%ux%u pixels)\n", (unsigned)width, (unsigned)height);

    return TRUE;
}

/*========================================================================*/
/* itidy_dither_auto_select                                               */
/*========================================================================*/

UWORD itidy_dither_auto_select(UWORD target_colors)
{
    if (target_colors >= 64)
        return ITIDY_DITHER_NONE;
    else if (target_colors >= 16)
        return ITIDY_DITHER_ORDERED;
    else
        return ITIDY_DITHER_FLOYD;  /* 4-8 colors: quality critical */
}
