#ifndef ICON_TYPES_INTERNAL_H
#define ICON_TYPES_INTERNAL_H

/**
 * @file icon_types_internal.h
 * @brief Internal shared types and globals for icon_types subsystem
 * 
 * This header contains shared data structures, constants, and extern declarations
 * used internally by the icon_types subsystem modules. It should ONLY be included
 * by files within src/icon_types/ directory.
 * 
 * External code should use the public API in icon_types.h instead.
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>
#include <exec/types.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>

#include "../itidy_types.h"
#include "tool_cache.h"  /* For ToolCacheEntry type definition */

/*========================================================================*/
/* Shared Constants                                                       */
/*========================================================================*/

/* Maximum number of file references to store per tool */
#define TOOL_CACHE_MAX_FILES_PER_TOOL  200

/* Chunk sizes for OS3.5 icon parsing */
#define CHUNK_SIZE  1024
#define HEADER_SIZE 8

/*========================================================================*/
/* PATH Search List - Global Variables                                   */
/*========================================================================*/

/**
 * Global PATH search list (built from startup scripts at program start)
 * Managed by path_manager.c
 */
extern char **g_PathSearchList;
extern int g_PathSearchCount;

/*========================================================================*/
/* Tool Cache - Shared Global Variables                                  */
/*========================================================================*/

/**
 * Note: ToolCacheEntry type is now defined in tool_cache.h (public API).
 * Include tool_cache.h before this header to get the type definition.
 */

/**
 * Global tool cache (managed by tool_cache.c)
 */
extern ToolCacheEntry *g_ToolCache;
extern int g_ToolCacheCount;
extern int g_ToolCacheCapacity;

#endif /* ICON_TYPES_INTERNAL_H */
