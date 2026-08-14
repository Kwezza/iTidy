/*
 * icon_newicons.h - NewIcons IM1=/IM2= ToolType decoder
 *
 * Decodes chunky indexed imagery stored in ToolTypes. Each physical
 * IM1=/IM2= ToolType is a framing boundary: residual bits are discarded
 * and samples never straddle strings. Palette decoding finishes on a
 * ToolType boundary; pixel decoding starts on the next same-image line.
 * Ordinary 7-bit groups and RLE zero groups share one ordered bitstream.
 *
 * The ToolType table is never modified.
 *
 * Target: 68000+, Workbench 2.x+, C89/C99 subset
 */

#ifndef ITIDY_ICON_NEWICONS_H
#define ITIDY_ICON_NEWICONS_H

#include "icon_file.h"

/**
 * Decode NewIcons IM1= (normal) and IM2= (selected) imagery.
 *
 * Allocates pixel and palette buffers with whd_malloc(); the caller must
 * release them with icon_decoded_free().
 *
 * Does not resize, dither, remap pens, or write files.
 * Does not rewrite ToolTypes.
 * On error, *out is zeroed and no allocations are retained.
 */
iTidy_IconError icon_newicons_decode(const iTidy_IconFile *file,
                                     iTidy_DecodedIcon *out);

#endif /* ITIDY_ICON_NEWICONS_H */
