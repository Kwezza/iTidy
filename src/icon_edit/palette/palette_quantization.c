/*
 * palette_quantization.c - iTidy2 adapter for shared Median Cut
 */

#include "palette_quantization.h"

#include <image/image_palette.h>

BOOL itidy_build_color_histogram(const UBYTE *pixels, ULONG pixel_count,
                                 const struct ColorRegister *palette,
                                 ULONG palette_size,
                                 iTidy_ColorEntry *out_histogram,
                                 UWORD *out_count)
{
    return image_build_color_histogram(pixels, pixel_count,
                                       (const iTidy_RGB8 *)palette,
                                       palette_size,
                                       out_histogram, out_count);
}

BOOL itidy_median_cut(iTidy_ColorEntry *histogram, UWORD hist_count,
                      UWORD target_colors,
                      struct ColorRegister *out_palette,
                      UWORD *out_pal_size)
{
    return image_median_cut(histogram, hist_count, target_colors,
                            (iTidy_RGB8 *)out_palette, out_pal_size);
}

BOOL itidy_quantize_palette(const UBYTE *pixels, ULONG pixel_count,
                            const struct ColorRegister *src_palette,
                            ULONG src_pal_size,
                            UWORD target_colors,
                            struct ColorRegister *out_palette,
                            UWORD *out_pal_size)
{
    return image_quantize_palette(pixels, pixel_count,
                                  (const iTidy_RGB8 *)src_palette,
                                  src_pal_size,
                                  target_colors,
                                  (iTidy_RGB8 *)out_palette,
                                  out_pal_size);
}
