/*
 * palette_mapping.h - Color Distance & Nearest-Color Matching
 *
 * Provides integer-only color distance calculations and palette-based
 * nearest-color search functions. Used by quantization, dithering, and
 * remapping modules to find the closest palette entry for a given RGB.
 *
 * All distance calculations use Manhattan (L1) norm — multiply-free
 * for best performance on 68020.
 *
 * See: IFF_Thumbnail_Implementation_Plan.md sections 12b-12c
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef PALETTE_MAPPING_H
#define PALETTE_MAPPING_H

#include <exec/types.h>
#include <datatypes/pictureclass.h>  /* struct ColorRegister */

/*========================================================================*/
/* Color Distance Functions                                               */
/*========================================================================*/

/**
 * @brief Compute Manhattan (L1) color distance between two RGB colors.
 *
 * Uses absolute differences with no multiplies — fast on 68020.
 * Range: 0 (identical) to 765 (black vs white).
 *
 * @param r1,g1,b1  First color RGB components (0-255)
 * @param r2,g2,b2  Second color RGB components (0-255)
 * @return Manhattan distance (0-765)
 */
UWORD itidy_palette_manhattan_distance(UBYTE r1, UBYTE g1, UBYTE b1,
                                       UBYTE r2, UBYTE g2, UBYTE b2);

/*========================================================================*/
/* Nearest-Color Search                                                   */
/*========================================================================*/

/**
 * @brief Find the closest palette entry to a target RGB color.
 *
 * Scans the entire palette using Manhattan distance and returns the
 * index of the best match. Ties are broken by first occurrence.
 *
 * @param palette      Palette array (ColorRegister: R, G, B bytes)
 * @param palette_size Number of entries in the palette
 * @param target_r     Target red component (0-255)
 * @param target_g     Target green component (0-255)
 * @param target_b     Target blue component (0-255)
 * @return Index of closest palette entry (0 to palette_size-1)
 */
UBYTE itidy_palette_find_nearest(const struct ColorRegister *palette,
                                 ULONG palette_size,
                                 UBYTE target_r, UBYTE target_g,
                                 UBYTE target_b);

/**
 * @brief Find closest palette entry with optional dithering adjustment.
 *
 * When dithering is enabled (dither_method != ITIDY_DITHER_NONE),
 * applies a position-dependent offset to the target RGB before
 * searching. For ordered dithering, this is a Bayer matrix threshold.
 *
 * @param palette        Palette array
 * @param palette_size   Number of entries
 * @param target_r,g,b   Target RGB components
 * @param pixel_x,pixel_y Pixel position (for Bayer matrix lookup)
 * @param dither_method  Dithering method (ITIDY_DITHER_* constant)
 * @return Index of closest palette entry
 */
UBYTE itidy_palette_find_nearest_dithered(
    const struct ColorRegister *palette,
    ULONG palette_size,
    UBYTE target_r, UBYTE target_g, UBYTE target_b,
    UWORD pixel_x, UWORD pixel_y,
    UWORD dither_method);

/*========================================================================*/
/* Pixel Buffer Utilities                                                 */
/*========================================================================*/

/**
 * @brief Count the number of unique palette indices used in a pixel buffer.
 *
 * Scans the pixel buffer and counts distinct index values present.
 * Maximum return value is 256 (full 8-bit palette usage).
 *
 * @param pixels      Pixel buffer (one byte per pixel = palette index)
 * @param pixel_count Total number of pixels in the buffer
 * @return Number of unique palette indices used (1-256)
 */
UWORD itidy_palette_count_unique(const UBYTE *pixels, ULONG pixel_count);

/**
 * @brief Remap all pixels from old palette indices to a new palette.
 *
 * For each pixel, looks up the RGB color in the old palette, then finds
 * the closest match in the new palette. Optionally applies dithering
 * during the remapping process.
 *
 * Modifies the pixel buffer in-place.
 *
 * @param pixels         Pixel buffer to remap (modified in-place)
 * @param width          Image width in pixels
 * @param height         Image height in pixels
 * @param old_palette    Source palette (maps old indices to RGB)
 * @param old_pal_size   Number of entries in old palette
 * @param new_palette    Target palette (reduced palette to remap into)
 * @param new_pal_size   Number of entries in new palette
 * @param dither_method  Dithering method (ITIDY_DITHER_* constant)
 */
void itidy_palette_remap(UBYTE *pixels, UWORD width, UWORD height,
                         const struct ColorRegister *old_palette,
                         ULONG old_pal_size,
                         const struct ColorRegister *new_palette,
                         ULONG new_pal_size,
                         UWORD dither_method);

#endif /* PALETTE_MAPPING_H */
