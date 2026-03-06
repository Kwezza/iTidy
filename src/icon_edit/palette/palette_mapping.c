/*
 * palette_mapping.c - Color Distance & Nearest-Color Matching
 *
 * See palette_mapping.h for API documentation.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include "palette_mapping.h"
#include "palette_dithering.h"
#include "../../writeLog.h"

#include <string.h>  /* memset */

/*========================================================================*/
/* itidy_palette_manhattan_distance                                       */
/*========================================================================*/

UWORD itidy_palette_manhattan_distance(UBYTE r1, UBYTE g1, UBYTE b1,
                                       UBYTE r2, UBYTE g2, UBYTE b2)
{
    UWORD dist = 0;

    /* Absolute differences — no multiplies, 68020-friendly */
    if (r1 > r2) dist += (UWORD)(r1 - r2); else dist += (UWORD)(r2 - r1);
    if (g1 > g2) dist += (UWORD)(g1 - g2); else dist += (UWORD)(g2 - g1);
    if (b1 > b2) dist += (UWORD)(b1 - b2); else dist += (UWORD)(b2 - b1);

    return dist;
}

/*========================================================================*/
/* itidy_palette_find_nearest                                             */
/*========================================================================*/

UBYTE itidy_palette_find_nearest(const struct ColorRegister *palette,
                                 ULONG palette_size,
                                 UBYTE target_r, UBYTE target_g,
                                 UBYTE target_b)
{
    UBYTE best_idx = 0;
    UWORD best_dist = 0xFFFF;  /* Start with maximum possible */
    ULONG i;

    if (palette == NULL || palette_size == 0)
        return 0;

    for (i = 0; i < palette_size; i++)
    {
        UWORD dist = itidy_palette_manhattan_distance(
            target_r, target_g, target_b,
            palette[i].red, palette[i].green, palette[i].blue);

        if (dist < best_dist)
        {
            best_dist = dist;
            best_idx = (UBYTE)i;
            if (dist == 0)
                break;  /* Exact match — can't do better */
        }
    }

    return best_idx;
}

/*========================================================================*/
/* itidy_palette_find_nearest_dithered                                    */
/*========================================================================*/

UBYTE itidy_palette_find_nearest_dithered(
    const struct ColorRegister *palette,
    ULONG palette_size,
    UBYTE target_r, UBYTE target_g, UBYTE target_b,
    UWORD pixel_x, UWORD pixel_y,
    UWORD dither_method)
{
    UBYTE adj_r = target_r;
    UBYTE adj_g = target_g;
    UBYTE adj_b = target_b;

    /* Apply Bayer dither offset if ordered dithering is selected */
    if (dither_method == ITIDY_DITHER_ORDERED)
    {
        WORD offset = itidy_dither_bayer_offset(pixel_x, pixel_y);

        if (offset > 0)
        {
            WORD temp;
            temp = (WORD)adj_r + offset;
            adj_r = (temp > 255) ? 255 : (UBYTE)temp;
            temp = (WORD)adj_g + offset;
            adj_g = (temp > 255) ? 255 : (UBYTE)temp;
            temp = (WORD)adj_b + offset;
            adj_b = (temp > 255) ? 255 : (UBYTE)temp;
        }
        else if (offset < 0)
        {
            WORD abs_off = -offset;
            WORD temp;
            temp = (WORD)adj_r - abs_off;
            adj_r = (temp < 0) ? 0 : (UBYTE)temp;
            temp = (WORD)adj_g - abs_off;
            adj_g = (temp < 0) ? 0 : (UBYTE)temp;
            temp = (WORD)adj_b - abs_off;
            adj_b = (temp < 0) ? 0 : (UBYTE)temp;
        }
    }
    /* Floyd-Steinberg is handled as a full-pass in palette_dithering.c,
     * not per-pixel here. If ITIDY_DITHER_FLOYD is passed, we just do
     * nearest-color (the F-S pass handles error propagation separately). */

    return itidy_palette_find_nearest(palette, palette_size,
                                      adj_r, adj_g, adj_b);
}

/*========================================================================*/
/* itidy_palette_count_unique                                             */
/*========================================================================*/

UWORD itidy_palette_count_unique(const UBYTE *pixels, ULONG pixel_count)
{
    UBYTE seen[256];
    UWORD count = 0;
    ULONG i;

    if (pixels == NULL || pixel_count == 0)
        return 0;

    memset(seen, 0, sizeof(seen));

    for (i = 0; i < pixel_count; i++)
    {
        if (!seen[pixels[i]])
        {
            seen[pixels[i]] = 1;
            count++;
        }
    }

    return count;
}

/*========================================================================*/
/* itidy_palette_remap                                                    */
/*========================================================================*/

void itidy_palette_remap(UBYTE *pixels, UWORD width, UWORD height,
                         const struct ColorRegister *old_palette,
                         ULONG old_pal_size,
                         const struct ColorRegister *new_palette,
                         ULONG new_pal_size,
                         UWORD dither_method)
{
    UWORD x, y;

    if (pixels == NULL || old_palette == NULL || new_palette == NULL)
        return;

    if (old_pal_size == 0 || new_pal_size == 0)
        return;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            ULONG offset = (ULONG)y * (ULONG)width + (ULONG)x;
            UBYTE old_idx = pixels[offset];
            UBYTE r, g, b;

            /* Look up RGB from original palette */
            if ((ULONG)old_idx < old_pal_size)
            {
                r = old_palette[old_idx].red;
                g = old_palette[old_idx].green;
                b = old_palette[old_idx].blue;
            }
            else
            {
                r = g = b = 0;  /* Safety: out-of-range index -> black */
            }

            /* Find closest match in new palette with optional dithering */
            pixels[offset] = itidy_palette_find_nearest_dithered(
                new_palette, new_pal_size,
                r, g, b,
                x, y,
                dither_method);
        }
    }
}
