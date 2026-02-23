/*
 * palette_reduction.h - High-Level Palette Reduction Pipeline
 *
 * Coordinates the full palette reduction pipeline: count unique colors,
 * decide whether reduction is needed, run Median Cut quantization,
 * apply dithering, and rebuild the icon's palette/pixel data.
 *
 * This is the main entry point called by icon_content_preview.c.
 * Other palette/ modules are implementation details.
 *
 * See: IFF_Thumbnail_Implementation_Plan.md sections 12b-12e
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef PALETTE_REDUCTION_H
#define PALETTE_REDUCTION_H

#include <exec/types.h>
#include "../icon_image_access.h"  /* iTidy_IconImageData */

/*========================================================================*/
/* Max Color Count Option Values                                          */
/*========================================================================*/

/** Chooser index to actual color count mapping */
#define ITIDY_MAX_COLORS_4     4
#define ITIDY_MAX_COLORS_8     8
#define ITIDY_MAX_COLORS_16   16
#define ITIDY_MAX_COLORS_32   32
#define ITIDY_MAX_COLORS_64   64
#define ITIDY_MAX_COLORS_128 128
#define ITIDY_MAX_COLORS_256 256   /* No limit (full palette) */

/** Number of entries in the max colors chooser (excluding Ultra and Harmonised) */
#define ITIDY_MAX_COLORS_CHOOSER_COUNT  8

/** GlowIcons harmonised palette chooser index (between 16 and 32, index 3) */
#define ITIDY_MAX_COLORS_HARMONISED_INDEX  3

/** Ultra mode chooser index (last entry, index 8) */
#define ITIDY_MAX_COLORS_ULTRA_INDEX    8

/*========================================================================*/
/* Chooser Index Mapping                                                  */
/*========================================================================*/

/**
 * @brief Convert chooser index to actual color count value.
 *
 * Maps:  0->4, 1->8, 2->16, 3->29(Harmonised), 4->32, 5->64,
 *         6->128, 7->256, 8->256(Ultra)
 *
 * @param chooser_index  Index from the Max Colors chooser gadget (0-8)
 * @return Actual color count (4, 8, 16, 29, 32, 64, 128, or 256)
 */
UWORD itidy_max_colors_from_index(UWORD chooser_index);

/**
 * @brief Convert actual color count value to chooser index.
 *
 * Reverse mapping for loading preferences into chooser gadget.
 *
 * @param max_colors  Actual color count value
 * @return Chooser index (0-6, or 6 for any value >= 256)
 */
UWORD itidy_max_colors_to_index(UWORD max_colors);

/*========================================================================*/
/* Main Palette Reduction API                                             */
/*========================================================================*/

/**
 * @brief Apply palette reduction to an icon's image data.
 *
 * This is the main entry point for palette reduction. It:
 * 1. Counts unique colors in the pixel buffer
 * 2. If count exceeds max_colors, runs Median Cut quantization
 * 3. Remaps all pixels to the reduced palette (with dithering if enabled)
 * 4. Updates img->palette_normal and img->palette_size_normal
 *
 * For 4-8 color modes, respects the lowcolor_mapping preference
 * (grayscale, Workbench palette, or hybrid).
 *
 * If max_colors >= current unique color count, this is a no-op.
 *
 * @param img             Icon image data (pixel buffer + palette modified in-place)
 * @param max_colors      Maximum number of colors allowed (4-256)
 * @param dither_method   Dithering method (ITIDY_DITHER_* constant, or AUTO)
 * @param lowcolor_mapping Low-color mapping mode (0=Gray, 1=WB, 2=Hybrid)
 *                         Only used when max_colors <= 8
 * @return TRUE on success (or no-op), FALSE on allocation failure
 */
BOOL itidy_reduce_palette(iTidy_IconImageData *img,
                          UWORD max_colors,
                          UWORD dither_method,
                          UWORD lowcolor_mapping);

#endif /* PALETTE_REDUCTION_H */
