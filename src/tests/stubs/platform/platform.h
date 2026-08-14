#ifndef PLATFORM_H
#define PLATFORM_H

/*
 * Test stub for shared image/icon kernels (host GCC and Amiga VBCC).
 *
 * Classic production src/platform/platform.h enables DEBUG_MEMORY_TRACKING
 * unconditionally. Shared tests must not depend on that production tracker.
 *
 * Keep this directory off the host_stubs path that also contains exec/types.h
 * so Amiga builds still pick up the real SDK exec/types.h.
 */

#include <stdlib.h>
#include <string.h>

#define whd_malloc(sz)  malloc(sz)
#define whd_free(p)     free(p)

#endif /* PLATFORM_H */
