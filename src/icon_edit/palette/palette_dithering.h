/*
 * palette_dithering.h - Dithering Algorithms for Palette Reduction
 *
 * Provides ordered (Bayer 4x4) and Floyd-Steinberg error diffusion
 * dithering for use during palette remapping. Dithering breaks up
 * color banding when reducing from many colors to few.
 *
 * All arithmetic is integer-only (no floats) for 68020 compatibility.
 * Floyd-Steinberg uses 16.16 fixed-point for error propagation.
 *
 * See: IFF_Thumbnail_Implementation_Plan.md section 12c
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef PALETTE_DITHERING_H
#define PALETTE_DITHERING_H

#include <exec/types.h>
#include <datatypes/pictureclass.h>  /* struct ColorRegister */

/*========================================================================*/
/* Dithering Method Constants                                             */
/*========================================================================*/

/** No dithering — pure nearest-color match */
#define ITIDY_DITHER_NONE     0

/** Ordered dithering using 4x4 Bayer threshold matrix (fast) */
#define ITIDY_DITHER_ORDERED  1

/** Floyd-Steinberg error diffusion dithering (best quality) */
#define ITIDY_DITHER_FLOYD    2

/** Auto — select best method based on color count */
#define ITIDY_DITHER_AUTO     3

/*========================================================================*/
/* Bayer Ordered Dithering                                                */
/*========================================================================*/

/**
 * @brief Get the Bayer 4x4 dither offset for a pixel position.
 *
 * Returns a signed offset in the range -7 to +8, centered around 0.
 * This offset is added to RGB values before palette matching to break
 * up color bands with a repeating 4x4 pattern.
 *
 * @param pixel_x  Pixel X position (only lower 2 bits used)
 * @param pixel_y  Pixel Y position (only lower 2 bits used)
 * @return Signed dither offset (-7 to +8)
 */
WORD itidy_dither_bayer_offset(UWORD pixel_x, UWORD pixel_y);

/*========================================================================*/
/* Floyd-Steinberg Error Diffusion                                        */
/*========================================================================*/

/**
 * @brief Apply Floyd-Steinberg error diffusion dithering to a pixel buffer.
 *
 * Processes the entire pixel buffer, remapping each pixel to the closest
 * color in the target palette while propagating quantization error to
 * neighboring pixels. Produces organic, natural-looking dither patterns.
 *
 * Error distribution pattern:
 *           X   7/16
 *     3/16  5/16  1/16
 *
 * Uses integer arithmetic with 16.16 fixed-point error accumulation.
 * Allocates temporary error buffers via whd_malloc() (freed before return).
 *
 * @param pixels       Pixel buffer to dither (modified in-place)
 * @param width        Image width in pixels
 * @param height       Image height in pixels
 * @param old_palette  Source palette (maps current indices to RGB)
 * @param old_pal_size Number of entries in source palette
 * @param new_palette  Target palette (reduced palette to remap into)
 * @param new_pal_size Number of entries in target palette
 * @return TRUE on success, FALSE on allocation failure
 */
BOOL itidy_dither_floyd_steinberg(UBYTE *pixels, UWORD width, UWORD height,
                                  const struct ColorRegister *old_palette,
                                  ULONG old_pal_size,
                                  const struct ColorRegister *new_palette,
                                  ULONG new_pal_size);

/*========================================================================*/
/* Auto Selection                                                         */
/*========================================================================*/

/**
 * @brief Select the best dithering method based on target color count.
 *
 * Auto selection logic:
 *   64+ colors  -> ITIDY_DITHER_NONE (enough colors for smooth gradients)
 *   32 colors   -> ITIDY_DITHER_ORDERED (good quality, fast)
 *   16 colors   -> ITIDY_DITHER_ORDERED (acceptable quality)
 *   8 colors    -> ITIDY_DITHER_FLOYD (quality critical)
 *   4 colors    -> ITIDY_DITHER_FLOYD (essential for usability)
 *
 * @param target_colors  Maximum color count setting
 * @return Recommended dithering method constant
 */
UWORD itidy_dither_auto_select(UWORD target_colors);

#endif /* PALETTE_DITHERING_H */
