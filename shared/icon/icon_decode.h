/*
 * icon_decode.h - Unified icon decode entry points
 *
 * Probe, decode a requested representation, or decode the best available
 * enhanced image. Does not write files or alter GUI state.
 *
 * Target: 68000+, Workbench 2.x+, C89/C99 subset
 */

#ifndef ITIDY_ICON_DECODE_H
#define ITIDY_ICON_DECODE_H

#include "icon_file.h"
#include "icon_probe.h"
#include "icon_classic.h"
#include "icon_newicons.h"
#include "icon_coloricon.h"

#define ITIDY_ICON_REQ_BEST        0UL
#define ITIDY_ICON_REQ_COLORICON   1UL
#define ITIDY_ICON_REQ_NEWICONS    2UL
#define ITIDY_ICON_REQ_CLASSIC     3UL

/**
 * Decode a requested representation from a parsed .info envelope.
 * ITIDY_ICON_REQ_BEST prefers ColorIcon/GlowIcon, then NewIcons, then
 * classic planar. Unsupported later formats (PNG, OS4 ARGB) return
 * ITIDY_ICON_ERR_UNSUPPORTED rather than a corruption error when they
 * are the only remaining payload.
 *
 * Caller frees with icon_decoded_free().
 */
iTidy_IconError icon_decode(const iTidy_IconFile *file, ULONG request,
                            iTidy_DecodedIcon *out);

/**
 * Parse then decode a buffer. PNG-only data (no DiskObject header) is
 * reported as UNSUPPORTED rather than BAD_MAGIC.
 */
iTidy_IconError icon_decode_buffer(const UBYTE *data, ULONG size,
                                   ULONG request, iTidy_DecodedIcon *out);

#endif /* ITIDY_ICON_DECODE_H */
