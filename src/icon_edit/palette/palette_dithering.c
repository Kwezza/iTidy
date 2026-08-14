/*
 * palette_dithering.c - iTidy2 adapter for shared dithering
 */

#include "palette_dithering.h"

#include <image/image_dither.h>

WORD itidy_dither_bayer_offset(UWORD pixel_x, UWORD pixel_y)
{
    return image_dither_bayer_offset(pixel_x, pixel_y);
}

BOOL itidy_dither_floyd_steinberg(UBYTE *pixels, UWORD width, UWORD height,
                                  const struct ColorRegister *old_palette,
                                  ULONG old_pal_size,
                                  const struct ColorRegister *new_palette,
                                  ULONG new_pal_size)
{
    return image_dither_floyd_steinberg(pixels, width, height,
                                        (const iTidy_RGB8 *)old_palette,
                                        old_pal_size,
                                        (const iTidy_RGB8 *)new_palette,
                                        new_pal_size);
}

UWORD itidy_dither_auto_select(UWORD target_colors)
{
    return image_dither_auto_select(target_colors);
}
