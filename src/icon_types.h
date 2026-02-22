#ifndef ICON_TYPES_H
#define ICON_TYPES_H

/**
 * @file icon_types.h
 * @brief Public API for icon type detection, validation, and metadata operations
 * 
 * This is the umbrella header for the icon_types subsystem. All functionality
 * is now modularized into separate focused modules:
 * 
 * - format.h/c:       Icon format detection and size extraction
 * - reader.h/c:       Optimized icon metadata reader (GetIconDetailsFromDisk)
 * - writer.h/c:       Icon modification (SetIconDefaultTool)
 * - path_manager.h/c: System PATH management for tool validation
 * - tool_cache.h/c:   Tool validation and caching system
 * 
 * This header maintains the same public API as before the refactoring,
 * ensuring backward compatibility with all dependent files. Simply include
 * this header and all icon_types functionality will be available.
 * 
 * IMPLEMENTATION NOTE:
 * The original 2,284-line icon_types.c has been split into 5 focused modules
 * within the src/icon_types/ directory. This improves maintainability,
 * build times, and code organization while preserving performance.
 */

#include <exec/types.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/icon.h>  /* For GetDiskObject, PutDiskObject, FreeDiskObject */

/* 
 * Include all icon_types subsystem modules
 * These provide the complete public API
 */
#include "icon_types/format.h"         /* Icon format detection and size extraction */
#include "icon_types/reader.h"         /* Optimized icon reader (GetIconDetailsFromDisk) */
#include "icon_types/writer.h"         /* Icon modification (SetIconDefaultTool) */
#include "icon_types/path_manager.h"   /* PATH management */
#include "icon_types/tool_cache.h"     /* Tool validation and caching */

/*========================================================================*/
/* Public API Summary (exported from subsystem modules)                   */
/*========================================================================*/

/*
 * FORMAT DETECTION (format.h):
 * - BOOL IsNewIcon(struct DiskObject *diskObject)
 * - BOOL IsNewIconPath(const STRPTR filePath)
 * - int isOS35IconFormat(const char *filename)
 * - int isIconTypeDisk(const char *filename, long fib_DirEntryType)
 * 
 * SIZE EXTRACTION (format.h):
 * - void GetNewIconSizePath(const char *filePath, IconSize *newIconSize)
 * - BOOL GetStandardIconSize(const char *filePath, IconSize *iconSize)
 * - int getOS35IconSize(const char *filename, IconSize *size)
 * - BOOL GetIconSizeFromFile(const char *filePath, IconSize *iconSize)
 * - IconPosition GetIconPositionFromPath(const char *iconPath)
 * 
 * OPTIMIZED ICON READER (reader.h):
 * - BOOL GetIconDetailsFromDisk(const char *filePath, IconDetailsFromDisk *details, const char *iconTextForFont)
 *   ** CRITICAL PERFORMANCE FUNCTION - Single disk read for all icon metadata **
 * 
 * ICON MODIFICATION (writer.h):
 * - BOOL SetIconDefaultTool(const char *iconPath, const char *newDefaultTool)
 * 
 * PATH MANAGEMENT (path_manager.h):
 * - BOOL BuildPathSearchList(void)              [Call at startup]
 * - void FreePathSearchList(void)               [Call at shutdown]
 * 
 * TOOL VALIDATION & CACHING (tool_cache.h):
 * - BOOL InitToolCache(void)                    [Call at startup]
 * - void FreeToolCache(void)                    [Call at shutdown]
 * - BOOL ValidateDefaultTool(const char *defaultTool)
 * - BOOL AddFileReferenceToToolCache(const char *toolName, const char *filePath)
 * - BOOL RemoveFileReferenceFromToolCache(const char *toolName, const char *filePath)
 * - BOOL UpdateToolCacheForFileChange(const char *filePath, const char *oldTool, const char *newTool)
 * - void DumpToolCache(void)
 */

/*========================================================================*/
/* Global Variables (extern declarations for dependent code)              */
/*========================================================================*/

/**
 * PATH search list - populated by BuildPathSearchList()
 * Managed by path_manager.c
 */
extern char **g_PathSearchList;
extern int g_PathSearchCount;

/**
 * Tool validation cache - populated by ValidateDefaultTool()
 * Managed by tool_cache.c
 */
extern ToolCacheEntry *g_ToolCache;
extern int g_ToolCacheCount;
extern int g_ToolCacheCapacity;

/**
 * Tool cache capacity constant
 */
#define TOOL_CACHE_MAX_FILES_PER_TOOL  200

#endif /* ICON_TYPES_H */
