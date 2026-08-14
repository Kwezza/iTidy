/*
 * icon_coloricon.h - Direct OS3.5/3.9 ColorIcon decoder (FORM ICON)
 *
 * Decodes IFF FORM ICON (FACE + IMAG) from a parsed .info envelope.
 * GlowIcon imagery uses the same encoding; SRC_GLOWICON is a heuristic label.
 * Does not use icon.library. Does not decode NewIcons or ARGB/PNG.
 *
 * Target: 68000+, Workbench 2.x+, C89/C99 subset
 */

#ifndef ITIDY_ICON_COLORICON_H
#define ITIDY_ICON_COLORICON_H

#include "icon_file.h"

/**
 * Decode ColorIcon / GlowIcon imagery from a parsed .info file.
 *
 * Uses file->extension_offset / extension_size as the FORM ICON start.
 * Allocates pixel and palette buffers with whd_malloc(); the caller must
 * release them with icon_decoded_free().
 *
 * Does not resize, dither, remap pens, or write files.
 * On error, *out is zeroed and no allocations are retained.
 */
iTidy_IconError icon_coloricon_decode(const iTidy_IconFile *file,
                                      iTidy_DecodedIcon *out);

#endif /* ITIDY_ICON_COLORICON_H */
