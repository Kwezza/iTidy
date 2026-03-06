/*
 * palette_harmonised.h - GlowIcons Harmonised Palette
 *
 * Provides the 29-colour AmigaOS icon harmonised palette from the
 * UI Style Guide (GlowIcons standard).  When selected, every pixel
 * is remapped to the nearest colour in this fixed palette instead of
 * using a per-image Median Cut palette.
 *
 * This produces icons with a consistent, cross-project colour scheme —
 * the same visual style used by Workbench 3.2's own GlowIcons set.
 *
 * Dithering is supported (and recommended) because only 29 colours are
 * available, so error diffusion reduces banding on photographic sources.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef PALETTE_HARMONISED_H
#define PALETTE_HARMONISED_H

#include <exec/types.h>
#include <datatypes/pictureclass.h>  /* struct ColorRegister */
#include "../icon_image_access.h"    /* iTidy_IconImageData  */

/*========================================================================*/
/* Constants                                                              */
/*========================================================================*/

/** Number of colours in the GlowIcons harmonised palette */
#define ITIDY_HARMONISED_PALETTE_SIZE  29

/*========================================================================*/
/* Palette Generation                                                     */
/*========================================================================*/

/**
 * @brief Fill an output array with the 29-colour GlowIcons harmonised palette.
 *
 * Colours are ordered: neutrals, blues, warm browns/oranges, yellows,
 * reds, greens, purples — matching the AmigaOS UI Style Guide image.
 *
 * @param out_palette  Caller-allocated array of at least
 *                     ITIDY_HARMONISED_PALETTE_SIZE entries.
 */
void itidy_harmonised_palette(struct ColorRegister *out_palette);

/*========================================================================*/
/* Icon Image Application                                                 */
/*========================================================================*/

/**
 * @brief Force the GlowIcons harmonised palette onto an icon image.
 *
 * Replaces the icon's current palette with the fixed 29-colour
 * GlowIcons set, then remaps every pixel to the nearest harmonised
 * colour.  Dithering is applied according to dither_method.
 *
 * This is a drop-in alternative to itidy_reduce_palette() when the
 * ITIDY_MAX_COLORS_HARMONISED_INDEX chooser entry is selected.
 *
 * @param img           Icon image data (palette and pixels modified in-place)
 * @param dither_method Dithering method (ITIDY_DITHER_* constant, or AUTO)
 * @return TRUE on success, FALSE on allocation failure or bad parameters
 */
BOOL itidy_apply_harmonised_palette(iTidy_IconImageData *img,
                                    UWORD dither_method);

#endif /* PALETTE_HARMONISED_H */
