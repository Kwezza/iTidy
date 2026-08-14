/*
 * icon_classic.h - Classic planar Image decoder
 *
 * Converts serialized Amiga planar struct Image data into chunky indexes.
 * Classic icons do not embed an RGB palette; indexes are Workbench pens.
 *
 * Target: 68000+, Workbench 2.x+, C89/C99 subset
 */

#ifndef ITIDY_ICON_CLASSIC_H
#define ITIDY_ICON_CLASSIC_H

#include "icon_file.h"

/**
 * Decode classic planar normal (and selected, if present) imagery.
 *
 * Allocates pixel buffers with whd_malloc(); the caller must release them
 * with icon_decoded_free(). palette is left NULL and palette_count 0:
 * indexes follow Intuition PlanePick / PlaneOnOff (Workbench pens).
 *
 * Does not resize, dither, remap, or write files.
 * On error, *out is zeroed and no allocations are retained.
 */
iTidy_IconError icon_classic_decode(const iTidy_IconFile *file,
                                    iTidy_DecodedIcon *out);

#endif /* ITIDY_ICON_CLASSIC_H */
