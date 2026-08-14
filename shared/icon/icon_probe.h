/*
 * icon_probe.h - Inexpensive read-only icon format identification
 *
 * Does not decode NewIcons or ColorIcon pixels.
 * Target: 68000+, Workbench 2.x+, C89/C99 subset
 */

#ifndef ITIDY_ICON_PROBE_H
#define ITIDY_ICON_PROBE_H

#include "icon_file.h"

typedef struct iTidy_IconProbe
{
    BOOL has_classic;
    BOOL has_newicons;
    BOOL has_coloricon;
    BOOL has_glowicon;
    BOOL has_os4_argb;
    BOOL has_png;

    BOOL has_normal;
    BOOL has_selected;

    BOOL has_unsupported;
} iTidy_IconProbe;

/**
 * Identify representations in an already-parsed .info envelope.
 * Does not modify the buffer.
 */
iTidy_IconError icon_probe_file(const iTidy_IconFile *file,
                                iTidy_IconProbe *out);

/**
 * Parse then probe a buffer. PNG-only data (no DiskObject header) is
 * reported as has_png / has_unsupported rather than BAD_MAGIC.
 */
iTidy_IconError icon_probe_buffer(const UBYTE *data, ULONG size,
                                  iTidy_IconProbe *out);

#endif /* ITIDY_ICON_PROBE_H */
