/*
 * palette_grayscale.h - Grayscale & Workbench Palette Mapping
 *
 * Provides palette strategies for very low color counts (4-8 colors):
 *   - Grayscale: Convert to gray levels for consistent appearance
 *   - Workbench palette: Map to stable system colors (indices 0-7)
 *   - Hybrid: Grayscale base with WB accent colors (blue, orange)
 *
 * These modes eliminate runtime color remapping by Workbench, speeding
 * up icon display on classic 4-8 color screens.
 *
 * All luminance calculations use integer arithmetic (no floats).
 *
 * See: IFF_Thumbnail_Implementation_Plan.md section 12d
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef PALETTE_GRAYSCALE_H
#define PALETTE_GRAYSCALE_H

#include <exec/types.h>
#include <datatypes/pictureclass.h>  /* struct ColorRegister */

/*========================================================================*/
/* Low-Color Mapping Mode Constants                                       */
/*========================================================================*/

/** Convert image to grayscale with evenly spaced gray levels */
#define ITIDY_LOWCOLOR_GRAYSCALE   0

/** Map to standard Workbench 0-7 palette colors */
#define ITIDY_LOWCOLOR_WORKBENCH   1

/** Hybrid: grayscale base + WB accent colors (blue, orange) */
#define ITIDY_LOWCOLOR_HYBRID      2

/*========================================================================*/
/* Luminance Calculation                                                  */
/*========================================================================*/

/**
 * @brief Convert RGB to perceptual grayscale (ITU-R BT.601).
 *
 * Uses integer-only formula: (306*R + 601*G + 117*B) >> 10
 * This gives luminance in 0-255 range.
 *
 * @param r  Red component (0-255)
 * @param g  Green component (0-255)
 * @param b  Blue component (0-255)
 * @return Grayscale luminance value (0-255)
 */
UBYTE itidy_rgb_to_gray(UBYTE r, UBYTE g, UBYTE b);

/*========================================================================*/
/* Palette Generation                                                     */
/*========================================================================*/

/**
 * @brief Generate an evenly-spaced grayscale palette.
 *
 * Creates a palette with num_colors evenly spaced gray levels
 * from black (0,0,0) to white (255,255,255).
 *
 * @param out_palette   Output palette array (caller allocates)
 * @param num_colors    Number of gray levels (4 or 8)
 */
void itidy_grayscale_palette(struct ColorRegister *out_palette,
                             UWORD num_colors);

/**
 * @brief Get the standard Workbench 0-7 palette.
 *
 * Provides the typical Workbench 3.x system palette (8 colors).
 * For 4-color mode, only indices 0-3 are used.
 *
 * @param out_palette   Output palette array (caller allocates, min 8)
 * @param num_colors    Number of colors to fill (4 or 8)
 */
void itidy_workbench_palette(struct ColorRegister *out_palette,
                             UWORD num_colors);

/**
 * @brief Get the hybrid grayscale + accent palette.
 *
 * Provides a palette with grayscale levels plus Workbench accent
 * colors (blue, orange) for 8-color mode.
 * For 4-color mode, falls back to pure grayscale.
 *
 * @param out_palette   Output palette array (caller allocates, min 8)
 * @param num_colors    Number of colors to fill (4 or 8)
 */
void itidy_hybrid_palette(struct ColorRegister *out_palette,
                          UWORD num_colors);

/*========================================================================*/
/* Pixel Conversion                                                       */
/*========================================================================*/

/**
 * @brief Remap pixels using grayscale conversion.
 *
 * For each pixel: look up RGB from old palette, convert to grayscale,
 * find nearest match in target grayscale palette.
 *
 * @param pixels         Pixel buffer (modified in-place)
 * @param width          Image width
 * @param height         Image height
 * @param old_palette    Source palette (maps indices to RGB)
 * @param old_pal_size   Number of entries in source palette
 * @param gray_palette   Target grayscale palette
 * @param gray_pal_size  Number of entries in grayscale palette
 * @param dither_method  Dithering method (ITIDY_DITHER_* constant)
 */
void itidy_grayscale_remap(UBYTE *pixels, UWORD width, UWORD height,
                           const struct ColorRegister *old_palette,
                           ULONG old_pal_size,
                           const struct ColorRegister *gray_palette,
                           ULONG gray_pal_size,
                           UWORD dither_method);

#endif /* PALETTE_GRAYSCALE_H */
