/*
 * icon_image_bevel.h - Thumbnail bevel highlight post-processor
 *
 * Applies a 2-pixel graduated bevel to the safe-area boundary of a rendered
 * thumbnail, simulating illumination from the top-left:
 *
 *   Outer ring (1 px):  50% blend toward white  (top/left edges)
 *                        50% blend toward black  (bottom/right edges)
 *   Inner ring (2 px):  25% blend toward white  (top/left edges)
 *                        25% blend toward black  (bottom/right edges)
 *   Ambiguous corners:  top-right and bottom-left receive NO bevel
 *
 * Because Amiga icons use indexed colour (no real alpha channel), the
 * semi-transparent look is achieved by expanding palette_normal with
 * pre-blended colour variants and remapping border pixel indices through
 * lookup tables, following the same approach as expand_palette_for_adaptive()
 * in icon_image_access.c.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef ICON_IMAGE_BEVEL_H
#define ICON_IMAGE_BEVEL_H

#include "../icon_image_access.h"

/**
 * @brief Number of palette slots the bevel reserves for blend variants.
 *
 * Callers that generate a palette before invoking itidy_apply_thumbnail_bevel()
 * should cap their palette target at (256 - BEVEL_PALETTE_RESERVED) so the
 * bevel has room to append its blend-variant entries without exceeding the
 * 256-entry Amiga palette limit.
 */
#define BEVEL_PALETTE_RESERVED  8U

/**
 * @brief Apply a 2-pixel graduated bevel highlight to a rendered thumbnail.
 *
 * Must be called AFTER pixel data has been written into img->pixel_data_normal
 * and img->palette_normal has been set.
 *
 * The function expands img->palette_normal with up to 4 * palette_size new
 * blend-variant entries (deduplicated, capped at 256 total).  If fewer than
 * BEVEL_MIN_FREE_SLOTS (8) palette slots are available the bevel is silently
 * skipped to avoid palette overflow.
 *
 * Only img->pixel_data_normal and img->palette_normal / palette_size_normal
 * are modified.  pixel_data_selected and palette_selected are untouched.
 *
 * @param img  Icon image data (pixel buffer + palette, modified in place)
 * @param rp   Render parameters describing the safe-area boundary
 */
void itidy_apply_thumbnail_bevel(iTidy_IconImageData      *img,
                                 const iTidy_RenderParams *rp);

#endif /* ICON_IMAGE_BEVEL_H */
