/*
 * palette_mapping.c - iTidy2 adapter for shared palette matching
 */

#include "palette_mapping.h"

#include <image/image_palette.h>

UWORD itidy_palette_manhattan_distance(UBYTE r1, UBYTE g1, UBYTE b1,
                                       UBYTE r2, UBYTE g2, UBYTE b2)
{
    return image_palette_manhattan_distance(r1, g1, b1, r2, g2, b2);
}

UBYTE itidy_palette_find_nearest(const struct ColorRegister *palette,
                                 ULONG palette_size,
                                 UBYTE target_r, UBYTE target_g,
                                 UBYTE target_b)
{
    return image_palette_find_nearest((const iTidy_RGB8 *)palette,
                                      palette_size,
                                      target_r, target_g, target_b);
}

UBYTE itidy_palette_find_nearest_dithered(
    const struct ColorRegister *palette,
    ULONG palette_size,
    UBYTE target_r, UBYTE target_g, UBYTE target_b,
    UWORD pixel_x, UWORD pixel_y,
    UWORD dither_method)
{
    return image_palette_find_nearest_dithered((const iTidy_RGB8 *)palette,
                                               palette_size,
                                               target_r, target_g, target_b,
                                               pixel_x, pixel_y,
                                               dither_method);
}

UWORD itidy_palette_count_unique(const UBYTE *pixels, ULONG pixel_count)
{
    return image_palette_count_unique(pixels, pixel_count);
}

void itidy_palette_remap(UBYTE *pixels, UWORD width, UWORD height,
                         const struct ColorRegister *old_palette,
                         ULONG old_pal_size,
                         const struct ColorRegister *new_palette,
                         ULONG new_pal_size,
                         UWORD dither_method)
{
    image_palette_remap(pixels, width, height,
                        (const iTidy_RGB8 *)old_palette, old_pal_size,
                        (const iTidy_RGB8 *)new_palette, new_pal_size,
                        dither_method);
}
