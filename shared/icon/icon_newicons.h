/*
 * icon_newicons.h - NewIcons IM1=/IM2= ToolType decoder
 *
 * Decodes chunky indexed imagery stored in ToolTypes. Each ToolType is a
 * new ASCII segment (do not concatenate "IM1="/"IM2=" prefixes). Unpacked
 * sample bits are kept across lines so a palette or pixel field may
 * finish after a 7-bit character wrap. Incomplete GetBits requests do
 * not consume bits.
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
 * On error, *out is zeroed and no allocations are retained.
 */
iTidy_IconError icon_newicons_decode(const iTidy_IconFile *file,
                                     iTidy_DecodedIcon *out);

#endif /* ITIDY_ICON_NEWICONS_H */
