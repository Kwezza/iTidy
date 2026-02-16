/*
 * palette_quantization.h - Median Cut Palette Quantization
 *
 * Implements Median Cut color quantization to reduce the number of
 * unique colors in a palette to a target count. Used when the user's
 * "Max icon colors" setting requires palette reduction.
 *
 * All arithmetic is integer-only (no floats) for 68020 compatibility.
 *
 * See: IFF_Thumbnail_Implementation_Plan.md section 12b
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef PALETTE_QUANTIZATION_H
#define PALETTE_QUANTIZATION_H

#include <exec/types.h>
#include <datatypes/pictureclass.h>  /* struct ColorRegister */

/*========================================================================*/
/* Constants                                                              */
/*========================================================================*/

/** Maximum number of color boxes during Median Cut splitting */
#define ITIDY_MAX_COLOR_BOXES 256

/*========================================================================*/
/* Types                                                                  */
/*========================================================================*/

/**
 * @brief Histogram entry tracking a unique RGB color and its frequency.
 */
typedef struct {
    UBYTE r, g, b;    /* RGB color value */
    UBYTE _pad;       /* Alignment padding */
    ULONG count;      /* Number of pixels using this color */
} iTidy_ColorEntry;

/**
 * @brief A "box" in RGB color space used during Median Cut splitting.
 *
 * Represents a rectangular region in 3D color space containing
 * a subset of the histogram entries. The box is split along its
 * longest axis at the median to produce two child boxes.
 */
typedef struct {
    UWORD start;      /* Start index in histogram array */
    UWORD count;      /* Number of entries in this box */
    UBYTE r_min, r_max;  /* Red channel bounds */
    UBYTE g_min, g_max;  /* Green channel bounds */
    UBYTE b_min, b_max;  /* Blue channel bounds */
    UBYTE longest_axis;  /* 0=R, 1=G, 2=B — axis with greatest range */
    UBYTE _pad;          /* Alignment padding */
    ULONG pixel_count;   /* Total pixel count in this box (sum of entry counts) */
} iTidy_ColorBox;

/*========================================================================*/
/* Public API                                                             */
/*========================================================================*/

/**
 * @brief Build a color histogram from a pixel buffer and its palette.
 *
 * Scans the pixel buffer, looks up each palette index's RGB value,
 * and builds a frequency table of unique RGB colors actually used.
 *
 * @param pixels         Pixel buffer (palette indices)
 * @param pixel_count    Total number of pixels
 * @param palette        Source palette (index -> RGB lookup)
 * @param palette_size   Number of entries in source palette
 * @param out_histogram  Output: array to fill with histogram entries
 *                       (caller allocates, max 256 entries)
 * @param out_count      Output: number of unique colors found
 * @return TRUE on success, FALSE on error
 */
BOOL itidy_build_color_histogram(const UBYTE *pixels, ULONG pixel_count,
                                 const struct ColorRegister *palette,
                                 ULONG palette_size,
                                 iTidy_ColorEntry *out_histogram,
                                 UWORD *out_count);

/**
 * @brief Perform Median Cut quantization to reduce palette to N colors.
 *
 * Takes a histogram of used colors and reduces it to target_colors
 * entries using the Median Cut algorithm. The resulting palette is
 * written to out_palette.
 *
 * Algorithm:
 * 1. Start with one box containing all histogram entries
 * 2. Find box with longest RGB axis range
 * 3. Sort entries along that axis and split at median
 * 4. Repeat until target_colors boxes exist
 * 5. Average colors within each box for final palette
 *
 * @param histogram      Color histogram (from itidy_build_color_histogram)
 * @param hist_count     Number of entries in histogram
 * @param target_colors  Desired number of output colors (4-256)
 * @param out_palette    Output: reduced palette entries
 * @param out_pal_size   Output: actual palette size produced
 * @return TRUE on success, FALSE on allocation failure
 */
BOOL itidy_median_cut(iTidy_ColorEntry *histogram, UWORD hist_count,
                      UWORD target_colors,
                      struct ColorRegister *out_palette,
                      UWORD *out_pal_size);

/**
 * @brief High-level palette quantization: reduce pixel buffer palette.
 *
 * Builds histogram, runs Median Cut, and produces a reduced palette.
 * This is the main entry point for palette quantization.
 *
 * @param pixels         Pixel buffer (palette indices, not modified)
 * @param pixel_count    Total number of pixels
 * @param src_palette    Source palette (full palette)
 * @param src_pal_size   Number of entries in source palette
 * @param target_colors  Desired max colors in output palette
 * @param out_palette    Output: reduced palette (caller allocates 256 entries)
 * @param out_pal_size   Output: actual number of colors produced
 * @return TRUE on success, FALSE on error
 */
BOOL itidy_quantize_palette(const UBYTE *pixels, ULONG pixel_count,
                            const struct ColorRegister *src_palette,
                            ULONG src_pal_size,
                            UWORD target_colors,
                            struct ColorRegister *out_palette,
                            UWORD *out_pal_size);

#endif /* PALETTE_QUANTIZATION_H */
