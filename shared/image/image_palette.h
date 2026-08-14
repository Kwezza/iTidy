/*
 * image_palette.h - Nearest-colour matching, remapping, Median Cut
 *
 * Target: 68000+, Workbench 2.x+, C89/C99 subset
 */

#ifndef ITIDY_IMAGE_PALETTE_H
#define ITIDY_IMAGE_PALETTE_H

#include "image_types.h"

/*========================================================================*/
/* Histogram / Median Cut types                                           */
/*========================================================================*/

#ifndef ITIDY_MAX_COLOR_BOXES
#define ITIDY_MAX_COLOR_BOXES 256
#endif

typedef struct {
    UBYTE r, g, b;
    UBYTE _pad;
    ULONG count;
} iTidy_ColorEntry;

typedef struct {
    UWORD start;
    UWORD count;
    UBYTE r_min, r_max;
    UBYTE g_min, g_max;
    UBYTE b_min, b_max;
    UBYTE longest_axis;
    UBYTE _pad;
    ULONG pixel_count;
} iTidy_ColorBox;

/*========================================================================*/
/* Distance and nearest-colour                                            */
/*========================================================================*/

UWORD image_palette_manhattan_distance(UBYTE r1, UBYTE g1, UBYTE b1,
                                       UBYTE r2, UBYTE g2, UBYTE b2);

UBYTE image_palette_find_nearest(const iTidy_RGB8 *palette,
                                 ULONG palette_size,
                                 UBYTE target_r, UBYTE target_g,
                                 UBYTE target_b);

UBYTE image_palette_find_nearest_dithered(const iTidy_RGB8 *palette,
                                          ULONG palette_size,
                                          UBYTE target_r, UBYTE target_g,
                                          UBYTE target_b,
                                          UWORD pixel_x, UWORD pixel_y,
                                          UWORD dither_method);

UWORD image_palette_count_unique(const UBYTE *pixels, ULONG pixel_count);

void image_palette_remap(UBYTE *pixels, UWORD width, UWORD height,
                         const iTidy_RGB8 *old_palette,
                         ULONG old_pal_size,
                         const iTidy_RGB8 *new_palette,
                         ULONG new_pal_size,
                         UWORD dither_method);

/*========================================================================*/
/* Quantisation                                                           */
/*========================================================================*/

BOOL image_build_color_histogram(const UBYTE *pixels, ULONG pixel_count,
                                 const iTidy_RGB8 *palette,
                                 ULONG palette_size,
                                 iTidy_ColorEntry *out_histogram,
                                 UWORD *out_count);

BOOL image_median_cut(iTidy_ColorEntry *histogram, UWORD hist_count,
                      UWORD target_colors,
                      iTidy_RGB8 *out_palette,
                      UWORD *out_pal_size);

BOOL image_quantize_palette(const UBYTE *pixels, ULONG pixel_count,
                            const iTidy_RGB8 *src_palette,
                            ULONG src_pal_size,
                            UWORD target_colors,
                            iTidy_RGB8 *out_palette,
                            UWORD *out_pal_size);

void image_quantize_rgb24_to_cube(const UBYTE *src_rgb24,
                                  UBYTE *dest_chunky,
                                  ULONG pixel_count,
                                  ULONG palette_size);

#endif /* ITIDY_IMAGE_PALETTE_H */
