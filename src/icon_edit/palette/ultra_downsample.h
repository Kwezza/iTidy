/*
 * ultra_downsample.h - Detail-Preserving Downsampling (Ultra Mode)
 *
 * Implements "Ultra Quality" downsampling that preserves fine details
 * like hair strands, stars, and texture patterns that standard
 * area-averaging would blur away. Works in RGB24 space with full
 * 256-color palette generation.
 *
 * Key difference from standard area-average:
 *   - Detects bright/dark isolated pixels in each source region
 *   - Boosts their contribution so they survive averaging
 *   - Result: miniature looks like original, not a blurry smear
 *
 * All arithmetic is integer-only (no floats) for 68020 compatibility.
 *
 * See: IFF_Thumbnail_Implementation_Plan.md section 12e
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef ULTRA_DOWNSAMPLE_H
#define ULTRA_DOWNSAMPLE_H

#include <exec/types.h>
#include <datatypes/pictureclass.h>  /* struct ColorRegister */

/*========================================================================*/
/* Tuning Constants                                                       */
/*========================================================================*/

/**
 * Brightness detection threshold: a pixel must be this many times
 * brighter than the region average to trigger detail preservation.
 * Value 3 = moderate (recommended). Higher = fewer pixels boosted.
 */
#define ITIDY_ULTRA_DETAIL_THRESHOLD  3

/**
 * Boost blend strength: fraction of detail boost applied.
 * 3/10 = 30% boost (recommended). Higher = stronger boost.
 */
#define ITIDY_ULTRA_BOOST_NUMERATOR    3
#define ITIDY_ULTRA_BOOST_DENOMINATOR 10

/*========================================================================*/
/* Public API                                                             */
/*========================================================================*/

/**
 * @brief Detail-preserving downsampling for Ultra Quality mode.
 *
 * Takes a source RGB24 buffer and downscales to dest dimensions.
 * For each output pixel, scans the corresponding source region:
 *   1. Computes area-average color (standard)
 *   2. Tracks brightest pixel in the region
 *   3. If brightest is THRESHOLD times brighter than average,
 *      blends 30% of the bright detail into the output
 *
 * Output is RGB24 (3 bytes per pixel: R, G, B). Caller must
 * allocate dest_rgb with at least dest_w * dest_h * 3 bytes.
 *
 * @param src_rgb     Source RGB24 buffer (3 bytes/pixel)
 * @param src_w       Source width in pixels
 * @param src_h       Source height in pixels
 * @param dest_rgb    Output RGB24 buffer (caller allocates)
 * @param dest_w      Desired output width
 * @param dest_h      Desired output height
 */
void itidy_ultra_downsample(const UBYTE *src_rgb,
                            UWORD src_w, UWORD src_h,
                            UBYTE *dest_rgb,
                            UWORD dest_w, UWORD dest_h);

/**
 * @brief Generate an optimal N-color palette from an RGB24 buffer.
 *
 * Builds a color histogram from the raw RGB24 data and applies
 * Median Cut quantization to produce an optimal palette.
 *
 * @param rgb24        RGB24 pixel data (3 bytes per pixel)
 * @param pixel_count  Number of pixels (width * height)
 * @param target_colors Maximum palette entries (typically 256)
 * @param out_palette  Output palette (caller allocates, max 256 entries)
 * @param out_pal_size Output: actual number of colors produced
 * @return TRUE on success, FALSE on allocation failure
 */
BOOL itidy_ultra_generate_palette(const UBYTE *rgb24, ULONG pixel_count,
                                  UWORD target_colors,
                                  struct ColorRegister *out_palette,
                                  UWORD *out_pal_size);

/**
 * @brief Remap an RGB24 buffer to palette-indexed pixels.
 *
 * Creates a new palette-indexed pixel buffer by finding the nearest
 * palette entry for each RGB24 pixel.
 *
 * @param rgb24        Source RGB24 data (3 bytes per pixel)
 * @param pixel_count  Number of pixels
 * @param palette      Target palette
 * @param pal_size     Number of palette entries
 * @param out_pixels   Output indexed pixel buffer (caller allocates)
 */
void itidy_ultra_remap_to_indexed(const UBYTE *rgb24, ULONG pixel_count,
                                  const struct ColorRegister *palette,
                                  ULONG pal_size,
                                  UBYTE *out_pixels);

#endif /* ULTRA_DOWNSAMPLE_H */
