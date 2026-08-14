/*
 * image_dither.h - Bayer ordered dither and Floyd-Steinberg error diffusion
 *
 * Target: 68000+, Workbench 2.x+, C89/C99 subset
 */

#ifndef ITIDY_IMAGE_DITHER_H
#define ITIDY_IMAGE_DITHER_H

#include "image_types.h"

/*========================================================================*/
/* Dithering method constants                                             */
/*========================================================================*/

#ifndef ITIDY_DITHER_NONE
#define ITIDY_DITHER_NONE     0
#define ITIDY_DITHER_ORDERED  1
#define ITIDY_DITHER_FLOYD    2
#define ITIDY_DITHER_AUTO     3
#endif

WORD image_dither_bayer_offset(UWORD pixel_x, UWORD pixel_y);

BOOL image_dither_floyd_steinberg(UBYTE *pixels, UWORD width, UWORD height,
                                  const iTidy_RGB8 *old_palette,
                                  ULONG old_pal_size,
                                  const iTidy_RGB8 *new_palette,
                                  ULONG new_pal_size);

UWORD image_dither_auto_select(UWORD target_colors);

#endif /* ITIDY_IMAGE_DITHER_H */
