#ifndef ICON_TYPES_TOOL_CACHE_H
#define ICON_TYPES_TOOL_CACHE_H

/**
 * @file tool_cache.h
 * @brief Tool validation and caching system
 * 
 * This module provides a caching system for default tool validation.
 * It tracks which tools exist, where they're located, their versions,
 * and which files reference them. This dramatically reduces disk I/O
 * during icon processing.
 * 
 * Features:
 * - Tool existence validation with PATH search
 * - Version string extraction from executables
 * - File reference tracking (up to 200 files per tool)
 * - Hit count statistics
 * - DOS volume requester suppression
 */

#include <exec/types.h>

/*========================================================================*/
/* Type Definitions                                                       */
/*========================================================================*/

/**
 * @brief Tool cache entry structure
 * 
 * Stores validation results and metadata for a single default tool.
 * Includes version information, full path, hit count statistics,
 * and a list of files that reference this tool.
 */
typedef struct {
    char *toolName;        /* Simple tool name (e.g., "MultiView") */
    BOOL exists;           /* TRUE if tool was found */
    char *fullPath;        /* Full path where found (e.g., "Workbench:Utilities/MultiView") or NULL if not found */
    char *versionString;   /* Version info (e.g., "MultiView 47.17") or NULL if unavailable */
    int hitCount;          /* Number of times this cache entry was accessed */
    
    /* File reference tracking */
    char **referencingFiles;  /* Array of file paths that use this tool */
    int fileCount;            /* Number of files currently stored */
    int fileCapacity;         /* Allocated capacity for files array */
} ToolCacheEntry;

/*========================================================================*/
/* Tool Cache Management                                                  */
/*========================================================================*/

/**
 * @brief Initialize the tool cache with initial capacity
 * 
 * Allocates initial capacity for the tool cache (20 tools, expands as needed).
 * Should be called at program startup.
 * 
 * @return TRUE if initialized successfully, FALSE on error
 */
BOOL InitToolCache(void);

/**
 * @brief Free the tool cache
 * 
 * Releases all memory allocated for the tool cache, including all cached
 * tool entries and file references. Should be called at program shutdown.
 */
void FreeToolCache(void);

/*======================================================================*/
/* Tool Validation                                                        */
/*========================================================================*/

/**
 * @brief Check if a default tool command/executable exists
 * 
 * This function validates whether a default tool specified in an icon can be found.
 * Uses a cache to avoid repeated disk access for the same tools.
 * 
 * Enhanced with:
 * - Tool cache for performance (avoids redundant lookups)
 * - Version string extraction from executables
 * - Full path tracking
 * - DOS volume requester suppression (no "Insert volume..." popups)
 * 
 * Search Strategy:
 * 1. Check cache first (fast path)
 * 2. If path contains a colon (:) or slash (/), treat as explicit path - check directly
 * 3. Otherwise, treat as command name and search the system PATH
 * 4. Extract version information if found
 * 5. Add result to cache
 * 
 * @param defaultTool The default tool string to validate (e.g., "MultiView" or "SYS:Utilities/More")
 * @return TRUE if the tool exists and is accessible, FALSE otherwise
 * 
 * Examples:
 *   ValidateDefaultTool("MultiView")              -> Searches PATH, caches result with version
 *   ValidateDefaultTool("SYS:Utilities/MultiView") -> Checks exact path
 *   ValidateDefaultTool("InvalidTool")             -> Returns FALSE, caches as missing
 */
BOOL ValidateDefaultTool(const char *defaultTool);

/*========================================================================*/
/* File Reference Tracking                                                */
/*========================================================================*/

/**
 * @brief Add a file reference to a tool cache entry
 * 
 * Tracks which files use a particular default tool. This builds up a list
 * of file paths that reference each tool, up to TOOL_CACHE_MAX_FILES_PER_TOOL (200).
 * 
 * If the tool is not yet in the cache, this function will return FALSE.
 * Call ValidateDefaultTool() first to ensure the tool is cached.
 * 
 * @param toolName The tool name to add a file reference for
 * @param filePath The file path that uses this tool
 * @return TRUE if added successfully, FALSE on error or if limit reached
 */
BOOL AddFileReferenceToToolCache(const char *toolName, const char *filePath);

/**
 * @brief Remove a file reference from a tool cache entry
 * 
 * Removes a specific file path from a tool's referencing files list.
 * This is used when a file's default tool changes.
 * 
 * If this is the last file referencing the tool, the entire tool entry
 * is removed from the cache.
 * 
 * @param toolName The tool name to remove the file reference from
 * @param filePath The file path to remove
 * @return TRUE if removed successfully, FALSE if not found
 */
BOOL RemoveFileReferenceFromToolCache(const char *toolName, const char *filePath);

/**
 * @brief Update tool cache when a file's default tool changes
 * 
 * Atomically moves a file reference from one tool to another in the cache.
 * This removes the file from oldTool's list and adds it to newTool's list,
 * creating the newTool cache entry if needed.
 * 
 * This is the recommended way to update the cache when a file's default
 * tool changes, as it handles all edge cases automatically.
 * 
 * @param filePath The .info file path being updated
 * @param oldTool The previous default tool name (NULL or empty if none)
 * @param newTool The new default tool name (NULL or empty to clear)
 * @return TRUE if cache updated successfully
 */
BOOL UpdateToolCacheForFileChange(const char *filePath, const char *oldTool, const char *newTool);

/*========================================================================*/
/* Cache Inspection                                                       */
/*========================================================================*/

/**
 * @brief Dump tool cache contents to log
 * 
 * Outputs all cached tool information to the log for debugging and
 * reporting purposes. Includes tool names, hit counts, existence status,
 * file count, full paths, versions, and list of referencing files.
 */
void DumpToolCache(void);

/*========================================================================*/
/* Public Accessors (defined in icon_types_internal.h)                   */
/*========================================================================*/

/**
 * Global tool cache variables (read-only access recommended)
 * Managed internally by tool_cache.c
 * 
 * extern ToolCacheEntry *g_ToolCache;
 * extern int g_ToolCacheCount;
 * extern int g_ToolCacheCapacity;
 */

#endif /* ICON_TYPES_TOOL_CACHE_H */
