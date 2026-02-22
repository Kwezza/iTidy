/**
 * @file tool_cache.c
 * @brief Tool validation and caching system implementation
 * 
 * This module implements a comprehensive caching system for default tool
 * validation. It tracks tool existence, locations, versions, and file
 * references to dramatically reduce disk I/O during icon processing.
 * 
 * Key features:
 * - PATH-based tool search with fallback to explicit paths
 * - DOS volume requester suppression (no "Insert volume..." popups)
 * - Version string extraction from executables ($VER: tag)
 * - File reference tracking (up to 200 files per tool)
 * - Automatic cache expansion and cleanup
 * - Hit count statistics for cache effectiveness analysis
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>

#include "../itidy_types.h"
#include "../utilities.h"
#include "../writeLog.h"
#include "tool_cache.h"          /* Must come before icon_types_internal.h */
#include "icon_types_internal.h"
#include "path_manager.h"
#include "tool_cache.h"

/*========================================================================*/
/* Global Tool Cache Variables                                            */
/*========================================================================*/

ToolCacheEntry *g_ToolCache = NULL;
int g_ToolCacheCount = 0;
int g_ToolCacheCapacity = 0;

/*========================================================================*/
/* Forward Declarations for Private Helpers                               */
/*========================================================================*/

static ToolCacheEntry *SearchToolCache(const char *toolName);
static char *GetToolVersion(const char *filePath);
static ToolCacheEntry *AddToolToCache(const char *toolName, BOOL exists, const char *fullPath, const char *versionString);
static BOOL RemoveToolFromCache(const char *toolName);

/*========================================================================*/
/* Public: Initialize Tool Cache                                          */
/*========================================================================*/

/**
 * @brief Initialize the tool cache with initial capacity
 * 
 * @return TRUE if initialized successfully, FALSE on error
 */
BOOL InitToolCache(void)
{
#if PLATFORM_AMIGA
    g_ToolCacheCapacity = 20;  /* Start with capacity for 20 tools */
    g_ToolCache = (ToolCacheEntry *)whd_malloc(g_ToolCacheCapacity * sizeof(ToolCacheEntry));
    
    if (!g_ToolCache)
    {
        log_error(LOG_GENERAL, "InitToolCache: Failed to allocate cache array\n");
        return FALSE;
    }
    
    memset(g_ToolCache, 0, g_ToolCacheCapacity * sizeof(ToolCacheEntry));
    
    g_ToolCacheCount = 0;
    log_debug(LOG_GENERAL, "InitToolCache: Cache initialized (capacity: %d)\n", g_ToolCacheCapacity);
    return TRUE;
#else
    return FALSE;
#endif
}

/*========================================================================*/
/* Private: Remove Tool From Cache                                        */
/*========================================================================*/

/**
 * @brief Remove a tool entry from the cache
 * 
 * Completely removes a tool from the cache, freeing all associated memory.
 * This is typically called when a tool has no more file references.
 * 
 * @param toolName The tool name to remove from cache
 * @return TRUE if removed successfully, FALSE if not found
 */
static BOOL RemoveToolFromCache(const char *toolName)
{
#if PLATFORM_AMIGA
    int i, j, tool_index;
    
    if (!toolName || !g_ToolCache || g_ToolCacheCount == 0)
        return FALSE;
    
    /* Find the tool in the cache */
    tool_index = -1;
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        if (g_ToolCache[i].toolName && strcmp(g_ToolCache[i].toolName, toolName) == 0)
        {
            tool_index = i;
            break;
        }
    }
    
    if (tool_index < 0)
    {
        log_debug(LOG_ICONS, "RemoveToolFromCache: Tool '%s' not found\n", toolName);
        return FALSE;
    }
    
    /* Free all strings in this entry */
    if (g_ToolCache[tool_index].toolName)
        whd_free(g_ToolCache[tool_index].toolName);
    if (g_ToolCache[tool_index].fullPath)
        whd_free(g_ToolCache[tool_index].fullPath);
    if (g_ToolCache[tool_index].versionString)
        whd_free(g_ToolCache[tool_index].versionString);
    
    /* Free file references array */
    if (g_ToolCache[tool_index].referencingFiles)
    {
        for (j = 0; j < g_ToolCache[tool_index].fileCount; j++)
        {
            if (g_ToolCache[tool_index].referencingFiles[j])
                whd_free(g_ToolCache[tool_index].referencingFiles[j]);
        }
        whd_free(g_ToolCache[tool_index].referencingFiles);
    }
    
    /* Shift remaining entries down (this copies pointers, not memory) */
    for (i = tool_index; i < g_ToolCacheCount - 1; i++)
    {
        g_ToolCache[i] = g_ToolCache[i + 1];
    }
    
    /* Clear last entry (don't free - we just moved the pointers) */
    memset(&g_ToolCache[g_ToolCacheCount - 1], 0, sizeof(ToolCacheEntry));
    g_ToolCacheCount--;
    
    log_debug(LOG_ICONS, "RemoveToolFromCache: Removed tool '%s' from cache (%d tools remain)\n",
             toolName, g_ToolCacheCount);
    
    return TRUE;
#else
    return FALSE;
#endif
}

/*========================================================================*/
/* Private: Search Tool Cache                                             */
/*========================================================================*/

/**
 * @brief Search for a tool in the cache
 * 
 * Uses case-insensitive comparison since AmigaDOS paths are case-insensitive.
 * Increments the hit count when a match is found.
 * 
 * @param toolName The tool name to search for
 * @return Pointer to cache entry if found, NULL otherwise
 */
static ToolCacheEntry *SearchToolCache(const char *toolName)
{
#if PLATFORM_AMIGA
    int i;
    
    if (!toolName || !g_ToolCache)
        return NULL;
    
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        if (g_ToolCache[i].toolName && platform_stricmp(g_ToolCache[i].toolName, toolName) == 0)
        {
            /* Increment hit counter */
            g_ToolCache[i].hitCount++;
            return &g_ToolCache[i];
        }
    }
#endif
    return NULL;
}

/*========================================================================*/
/* Private: Extract Tool Version                                          */
/*========================================================================*/

/**
 * @brief Extract version string from an executable file
 * 
 * Reads the embedded version string from an Amiga executable by searching
 * for the $VER: tag in the file contents.
 * 
 * @param filePath Full path to the executable
 * @return Allocated string with version info, or NULL if not found (caller must free)
 */
static char *GetToolVersion(const char *filePath)
{
#if PLATFORM_AMIGA
    BPTR file;
    char buffer[512];
    char *versionStr = NULL;
    LONG bytesRead;
    int i;
    const char *versionTag = "$VER:";
    int tagLen = 5;
    
    file = Open((CONST_STRPTR)filePath, MODE_OLDFILE);
    if (!file)
    {
        return NULL;
    }
    
    /* Read file in chunks looking for version string */
    while ((bytesRead = Read(file, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytesRead] = '\0';
        
        /* Search for $VER: tag */
        for (i = 0; i < bytesRead - tagLen; i++)
        {
            if (memcmp(&buffer[i], versionTag, tagLen) == 0)
            {
                /* Found version tag, extract the string */
                char *start = &buffer[i + tagLen];
                char *end = start;
                int len;
                
                /* Skip leading whitespace */
                while (*end == ' ' || *end == '\t')
                    end++;
                start = end;
                
                /* Find end of version string (newline, null, or control char) */
                while (*end && *end != '\n' && *end != '\r' && *end >= 32 && *end < 127)
                    end++;
                
                len = end - start;
                if (len > 0 && len < 200)
                {
                    versionStr = (char *)whd_malloc(len + 1);
                    if (versionStr)
                    {
                        memset(versionStr, 0, len + 1);
                        memcpy(versionStr, start, len);
                        versionStr[len] = '\0';
                    }
                }
                
                Close(file);
                return versionStr;
            }
        }
    }
    
    Close(file);
#endif
    return NULL;
}

/*========================================================================*/
/* Private: Add Tool To Cache                                             */
/*========================================================================*/

/**
 * @brief Add a tool to the cache
 * 
 * Creates a new cache entry with tool information. Expands cache capacity
 * if needed (doubles size dynamically).
 * 
 * @param toolName Simple tool name
 * @param exists TRUE if tool was found
 * @param fullPath Full path to tool (or NULL)
 * @param versionString Version info (or NULL)
 * @return Pointer to new cache entry, or NULL on error
 */
static ToolCacheEntry *AddToolToCache(const char *toolName, BOOL exists, const char *fullPath, const char *versionString)
{
#if PLATFORM_AMIGA
    ToolCacheEntry *newEntry;
    int nameLen, pathLen, verLen;
    
    if (!toolName || !g_ToolCache)
        return NULL;
    
    /* Check if we need to expand the cache */
    if (g_ToolCacheCount >= g_ToolCacheCapacity)
    {
        ToolCacheEntry *newCache;
        int newCapacity = g_ToolCacheCapacity * 2;
        
        newCache = (ToolCacheEntry *)whd_malloc(newCapacity * sizeof(ToolCacheEntry));
        if (!newCache)
        {
            log_error(LOG_GENERAL, "AddToolToCache: Failed to expand cache\n");
            return NULL;
        }
        
        memset(newCache, 0, newCapacity * sizeof(ToolCacheEntry));
        
        /* Copy old entries */
        memcpy(newCache, g_ToolCache, g_ToolCacheCount * sizeof(ToolCacheEntry));
        
        /* Free old cache */
        whd_free(g_ToolCache);
        
        g_ToolCache = newCache;
        g_ToolCacheCapacity = newCapacity;
        
        log_info(LOG_GENERAL, "AddToolToCache: Cache expanded to %d entries\n", newCapacity);
    }
    
    /* Add new entry */
    newEntry = &g_ToolCache[g_ToolCacheCount];
    
    /* Allocate and copy tool name */
    nameLen = strlen(toolName);
    newEntry->toolName = (char *)whd_malloc(nameLen + 1);
    if (!newEntry->toolName)
    {
        return NULL;
    }
    memset(newEntry->toolName, 0, nameLen + 1);
    strcpy(newEntry->toolName, toolName);
    
    newEntry->exists = exists;
    
    /* Allocate and copy full path if provided */
    if (fullPath)
    {
        pathLen = strlen(fullPath);
        newEntry->fullPath = (char *)whd_malloc(pathLen + 1);
        if (newEntry->fullPath)
        {
            memset(newEntry->fullPath, 0, pathLen + 1);
            strcpy(newEntry->fullPath, fullPath);
        }
    }
    else
    {
        newEntry->fullPath = NULL;
    }
    
    /* Allocate and copy version string if provided */
    if (versionString)
    {
        verLen = strlen(versionString);
        newEntry->versionString = (char *)whd_malloc(verLen + 1);
        if (newEntry->versionString)
        {
            memset(newEntry->versionString, 0, verLen + 1);
            strcpy(newEntry->versionString, versionString);
        }
    }
    else
    {
        newEntry->versionString = NULL;
    }
    
    /* Initialize hit count to 0 (first access will increment it) */
    newEntry->hitCount = 0;
    
    /* Initialize file reference tracking */
    newEntry->referencingFiles = NULL;
    newEntry->fileCount = 0;
    newEntry->fileCapacity = 0;
    
    g_ToolCacheCount++;
    
    log_debug(LOG_ICONS, "AddToolToCache: Cached '%s' -> %s [%s] v:%s\n",
             toolName,
             exists ? "EXISTS" : "MISSING",
             fullPath ? fullPath : "(none)",
             versionString ? versionString : "(no version)");
    
    return newEntry;
#else
    return NULL;
#endif
}

/*========================================================================*/
/* Public: Validate Default Tool                                          */
/*========================================================================*/

/**
 * ValidateDefaultTool - Check if a default tool command/executable exists
 * 
 * This function validates whether a default tool specified in an icon can be found.
 * Uses a cache to avoid repeated disk access for the same tools.
 * 
 * Enhanced with:
 * - Tool cache for performance (avoids redundant lookups)
 * - Version string extraction from executables
 * - Full path tracking
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
BOOL ValidateDefaultTool(const char *defaultTool)
{
#if PLATFORM_AMIGA
    BPTR lock = 0;
    BOOL isExplicitPath = FALSE;
    BOOL toolExists = FALSE;
    char testPath[256];
    char *foundPath = NULL;
    char *versionStr = NULL;
    ToolCacheEntry *cacheEntry;
    int i;
    struct Process *proc;
    APTR oldWindowPtr;
    
    if (defaultTool == NULL || defaultTool[0] == '\0')
    {
        return FALSE;  /* No tool specified */
    }
    
    /* ================================================================
     * DISABLE VOLUME REQUESTERS
     * ================================================================
     * When validating default tools, we may encounter paths to devices
     * that aren't currently mounted (e.g., "DeluxePaintIII:DPaint" when
     * the floppy/CD isn't inserted).
     * 
     * Normally, AmigaDOS Lock() would display a volume requester asking
     * the user to insert the disk. This would:
     * - Block the entire validation process
     * - Require user interaction for every unmounted device
     * - Make processing large directories with old files impractical
     * 
     * By setting pr_WindowPtr to -1, we disable all DOS requesters for
     * this process. Lock() will simply fail and return NULL instead of
     * showing the "Please insert volume..." requester.
     * 
     * This is restored at the end of the function to maintain normal
     * DOS behavior for other operations.
     * ================================================================
     */
    proc = (struct Process *)FindTask(NULL);
    oldWindowPtr = proc->pr_WindowPtr;
    proc->pr_WindowPtr = (APTR)-1;  /* Disable DOS requesters */
    
    /* Check cache first */
    cacheEntry = SearchToolCache(defaultTool);
    if (cacheEntry)
    {
#ifdef DEBUG
        log_debug(LOG_ICONS, "ValidateDefaultTool: Cache hit for '%s' -> %s\n", 
                 defaultTool, cacheEntry->exists ? "EXISTS" : "MISSING");
#endif
        /* Restore DOS requesters before returning */
        proc->pr_WindowPtr = oldWindowPtr;
        return cacheEntry->exists;
    }
    
    /* Not in cache - perform validation */
    
    /* Check if this is an explicit path (contains : or /) */
    if (strchr(defaultTool, ':') != NULL || strchr(defaultTool, '/') != NULL)
    {
        isExplicitPath = TRUE;
    }
    
    if (isExplicitPath)
    {
        /* Explicit path - check directly */
#ifdef DEBUG
        log_debug(LOG_ICONS, "ValidateDefaultTool: Checking explicit path '%s'\n", defaultTool);
#endif
        lock = Lock((CONST_STRPTR)defaultTool, ACCESS_READ);
        if (lock)
        {
            UnLock(lock);
            toolExists = TRUE;
            foundPath = (char *)defaultTool;  /* Use the provided path as-is */
#ifdef DEBUG
            log_debug(LOG_ICONS, "  -> Found at explicit path\n");
#endif
        }
        else
        {
#ifdef DEBUG
            log_debug(LOG_ICONS, "  -> NOT found at explicit path\n");
#endif
        }
    }
    else
    {
        /* Simple command name - search system PATH */
#ifdef DEBUG
        log_debug(LOG_ICONS, "ValidateDefaultTool: Searching PATH for command '%s'\n", defaultTool);
#endif
        
        /* Use global PATH list if available */
        if (g_PathSearchList && g_PathSearchCount > 0)
        {
            for (i = 0; i < g_PathSearchCount && !toolExists; i++)
            {
                /* Build test path */
                snprintf(testPath, sizeof(testPath), "%s%s", g_PathSearchList[i], defaultTool);
                
#ifdef DEBUG
                log_debug(LOG_ICONS, "  -> Trying: %s\n", testPath);
#endif
                
                lock = Lock((CONST_STRPTR)testPath, ACCESS_READ);
                if (lock)
                {
                    UnLock(lock);
                    toolExists = TRUE;
                    foundPath = testPath;
#ifdef DEBUG
                    log_debug(LOG_ICONS, "  -> FOUND at: %s\n", testPath);
#endif
                }
            }
        }
        else
        {
            /* Fallback: PATH list not built, try C: directory only */
            log_warning(LOG_ICONS, "ValidateDefaultTool: PATH list not available, checking C: only\n");
            snprintf(testPath, sizeof(testPath), "C:%s", defaultTool);
            
            lock = Lock((CONST_STRPTR)testPath, ACCESS_READ);
            if (lock)
            {
                UnLock(lock);
                toolExists = TRUE;
                foundPath = testPath;
            }
        }
        
        if (!toolExists)
        {
#ifdef DEBUG
            log_debug(LOG_ICONS, "  -> NOT found in any PATH directory\n");
#endif
        }
    }
    
    /* Extract version if tool was found */
    if (toolExists && foundPath)
    {
        versionStr = GetToolVersion(foundPath);
#ifdef DEBUG
        if (versionStr)
        {
            log_debug(LOG_ICONS, "  -> Version: %s\n", versionStr);
        }
#endif
    }
    
    /* Add to cache */
    AddToolToCache(defaultTool, toolExists, foundPath, versionStr);
    
    /* Free version string (it's been copied into cache) */
    if (versionStr)
    {
        whd_free(versionStr);
    }
    
    /* Restore DOS requesters */
    proc->pr_WindowPtr = oldWindowPtr;
    
    return toolExists;
#else
    /* Host platform stub - assume tool exists */
    (void)defaultTool;
    return TRUE;
#endif
}

/*========================================================================*/
/* Public: Add File Reference To Tool Cache                               */
/*========================================================================*/

/**
 * @brief Add a file reference to a tool cache entry
 * 
 * Tracks which files use a particular default tool. This builds up a list
 * of file paths that reference each tool, up to TOOL_CACHE_MAX_FILES_PER_TOOL.
 * 
 * @param toolName The tool name to add a file reference for
 * @param filePath The file path that uses this tool
 * @return TRUE if added successfully, FALSE on error or if limit reached
 */
BOOL AddFileReferenceToToolCache(const char *toolName, const char *filePath)
{
#if PLATFORM_AMIGA
    ToolCacheEntry *entry;
    char *newFilePath;
    int i, filePathLen;
    
    if (!toolName || !filePath || !g_ToolCache)
        return FALSE;
    
    /* Find the tool in the cache */
    entry = NULL;
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        if (g_ToolCache[i].toolName && strcmp(g_ToolCache[i].toolName, toolName) == 0)
        {
            entry = &g_ToolCache[i];
            break;
        }
    }
    
    if (!entry)
    {
        log_debug(LOG_ICONS, "AddFileReferenceToToolCache: Tool '%s' not found in cache\n", toolName);
        return FALSE;
    }
    
    /* Check if we've reached the limit */
    if (entry->fileCount >= TOOL_CACHE_MAX_FILES_PER_TOOL)
    {
        log_debug(LOG_ICONS, "AddFileReferenceToToolCache: Max file limit reached for '%s'\n", toolName);
        return FALSE;  /* Silently stop adding files */
    }
    
    /* Allocate array if needed */
    if (entry->referencingFiles == NULL)
    {
        entry->fileCapacity = 20;  /* Start with 20, grow as needed */
        entry->referencingFiles = (char **)whd_malloc(entry->fileCapacity * sizeof(char *));
        if (!entry->referencingFiles)
        {
            log_error(LOG_GENERAL, "AddFileReferenceToToolCache: Failed to allocate file array\n");
            return FALSE;
        }
        memset(entry->referencingFiles, 0, entry->fileCapacity * sizeof(char *));
    }
    
    /* Expand array if needed */
    if (entry->fileCount >= entry->fileCapacity)
    {
        char **newArray;
        int newCapacity = entry->fileCapacity * 2;
        
        /* Cap at max files */
        if (newCapacity > TOOL_CACHE_MAX_FILES_PER_TOOL)
            newCapacity = TOOL_CACHE_MAX_FILES_PER_TOOL;
        
        newArray = (char **)whd_malloc(newCapacity * sizeof(char *));
        if (!newArray)
        {
            log_error(LOG_GENERAL, "AddFileReferenceToToolCache: Failed to expand file array\n");
            return FALSE;
        }
        
        memset(newArray, 0, newCapacity * sizeof(char *));
        
        /* Copy existing pointers */
        memcpy(newArray, entry->referencingFiles, entry->fileCount * sizeof(char *));
        
        /* Free old array */
        whd_free(entry->referencingFiles);
        
        entry->referencingFiles = newArray;
        entry->fileCapacity = newCapacity;
    }
    
    /* Allocate and copy file path */
    filePathLen = strlen(filePath);
    newFilePath = (char *)whd_malloc(filePathLen + 1);
    if (!newFilePath)
    {
        log_error(LOG_GENERAL, "AddFileReferenceToToolCache: Failed to allocate file path\n");
        return FALSE;
    }
    memset(newFilePath, 0, filePathLen + 1);
    strcpy(newFilePath, filePath);
    
    /* Add to array */
    entry->referencingFiles[entry->fileCount] = newFilePath;
    entry->fileCount++;
    
    log_debug(LOG_ICONS, "AddFileReferenceToToolCache: Added file '%s' to tool '%s' (%d/%d)\n",
             filePath, toolName, entry->fileCount, TOOL_CACHE_MAX_FILES_PER_TOOL);
    
    return TRUE;
#else
    return FALSE;
#endif
}

/*========================================================================*/
/* Public: Remove File Reference From Tool Cache                          */
/*========================================================================*/

/**
 * @brief Remove a file reference from a tool cache entry
 * 
 * Removes a specific file path from a tool's referencing files list.
 * This is used when a file's default tool changes.
 * 
 * @param toolName The tool name to remove the file reference from
 * @param filePath The file path to remove
 * @return TRUE if removed successfully, FALSE if not found
 */
BOOL RemoveFileReferenceFromToolCache(const char *toolName, const char *filePath)
{
#if PLATFORM_AMIGA
    ToolCacheEntry *entry;
    int i, j;
    
    if (!toolName || !filePath || !g_ToolCache)
        return FALSE;
    
    /* Find the tool in the cache */
    entry = NULL;
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        if (g_ToolCache[i].toolName && strcmp(g_ToolCache[i].toolName, toolName) == 0)
        {
            entry = &g_ToolCache[i];
            break;
        }
    }
    
    if (!entry || !entry->referencingFiles)
    {
        log_debug(LOG_ICONS, "RemoveFileReferenceFromToolCache: Tool '%s' not found or has no files\n", toolName);
        return FALSE;
    }
    
    /* Find and remove the file */
    for (i = 0; i < entry->fileCount; i++)
    {
        if (entry->referencingFiles[i] && strcmp(entry->referencingFiles[i], filePath) == 0)
        {
            /* Free this file path */
            whd_free(entry->referencingFiles[i]);
            
            /* Shift remaining entries down */
            for (j = i; j < entry->fileCount - 1; j++)
            {
                entry->referencingFiles[j] = entry->referencingFiles[j + 1];
            }
            
            /* Clear last entry */
            entry->referencingFiles[entry->fileCount - 1] = NULL;
            entry->fileCount--;
            
            log_debug(LOG_ICONS, "RemoveFileReferenceFromToolCache: Removed file '%s' from tool '%s' (%d files remain)\n",
                     filePath, toolName, entry->fileCount);
            
            /* If this was the last file, remove the entire tool entry from cache */
            if (entry->fileCount == 0)
            {
                log_debug(LOG_ICONS, "RemoveFileReferenceFromToolCache: Tool '%s' has no more files, removing from cache\n",
                         toolName);
                RemoveToolFromCache(toolName);
            }
            
            return TRUE;
        }
    }
    
    log_debug(LOG_ICONS, "RemoveFileReferenceFromToolCache: File '%s' not found in tool '%s'\n",
             filePath, toolName);
    return FALSE;
#else
    return FALSE;
#endif
}

/*========================================================================*/
/* Public: Update Tool Cache For File Change                              */
/*========================================================================*/

/**
 * @brief Update tool cache when a file's default tool changes
 * 
 * Atomically moves a file reference from one tool to another in the cache.
 * This removes the file from oldTool's list and adds it to newTool's list,
 * creating the newTool cache entry if needed.
 * 
 * @param filePath The .info file path being updated
 * @param oldTool The previous default tool name (NULL or empty if none)
 * @param newTool The new default tool name (NULL or empty to clear)
 * @return TRUE if cache updated successfully
 */
BOOL UpdateToolCacheForFileChange(const char *filePath, const char *oldTool, const char *newTool)
{
#if PLATFORM_AMIGA
    BOOL removed = FALSE;
    BOOL added = FALSE;
    ToolCacheEntry *newEntry;
    
    if (!filePath)
        return FALSE;
    
    log_debug(LOG_ICONS, "UpdateToolCacheForFileChange: Updating cache for '%s'\n", filePath);
    log_debug(LOG_ICONS, "  Old tool: '%s'\n", oldTool ? oldTool : "(none)");
    log_debug(LOG_ICONS, "  New tool: '%s'\n", newTool ? newTool : "(none)");
    
    /* Log current cache state for old tool */
    if (oldTool && oldTool[0] != '\0')
    {
        ToolCacheEntry *oldEntry = SearchToolCache(oldTool);
        if (oldEntry)
        {
            log_debug(LOG_ICONS, "  BEFORE: Old tool '%s' has %d files in cache\n", 
                     oldTool, oldEntry->fileCount);
        }
    }
    
    /* Log current cache state for new tool */
    if (newTool && newTool[0] != '\0')
    {
        ToolCacheEntry *preCheckEntry = SearchToolCache(newTool);
        if (preCheckEntry)
        {
            log_debug(LOG_ICONS, "  BEFORE: New tool '%s' has %d files in cache\n",
                     newTool, preCheckEntry->fileCount);
        }
        else
        {
            log_debug(LOG_ICONS, "  BEFORE: New tool '%s' not yet in cache\n", newTool);
        }
    }
    
    /* Remove from old tool (if any) */
    if (oldTool && oldTool[0] != '\0')
    {
        removed = RemoveFileReferenceFromToolCache(oldTool, filePath);
        if (removed)
        {
            log_debug(LOG_ICONS, "  Successfully removed from old tool cache\n");
        }
        else
        {
            log_debug(LOG_ICONS, "  Note: File not found in old tool cache (may not have been scanned)\n");
        }
    }
    
    /* Add to new tool (if any) */
    if (newTool && newTool[0] != '\0')
    {
        /* First check if this tool exists in cache - if not, validate it */
        newEntry = SearchToolCache(newTool);
        
        if (!newEntry)
        {
            /* Tool not in cache yet - validate it to create proper entry
             * This will search for the tool, get its path, version, etc. */
            log_debug(LOG_ICONS, "  New tool '%s' not in cache - validating and caching\n", newTool);
            ValidateDefaultTool(newTool);  /* This adds to cache with full info */
            
            /* Search again to get the newly created entry */
            newEntry = SearchToolCache(newTool);
        }
        
        /* Now add the file reference */
        if (newEntry)
        {
            added = AddFileReferenceToToolCache(newTool, filePath);
            if (added)
            {
                log_debug(LOG_ICONS, "  Successfully added to new tool cache\n");
            }
            else
            {
                log_debug(LOG_ICONS, "  Warning: Failed to add to new tool cache (may be at capacity)\n");
            }
        }
        else
        {
            log_debug(LOG_ICONS, "  Warning: Failed to create cache entry for new tool\n");
        }
    }
    
    /* Log AFTER cache state for verification */
    if (oldTool && oldTool[0] != '\0')
    {
        ToolCacheEntry *oldEntry = SearchToolCache(oldTool);
        if (oldEntry)
        {
            log_debug(LOG_ICONS, "  AFTER: Old tool '%s' now has %d files in cache\n",
                     oldTool, oldEntry->fileCount);
        }
        else
        {
            log_debug(LOG_ICONS, "  AFTER: Old tool '%s' removed from cache (0 files)\n", oldTool);
        }
    }
    
    if (newTool && newTool[0] != '\0')
    {
        ToolCacheEntry *newEntry = SearchToolCache(newTool);
        if (newEntry)
        {
            log_debug(LOG_ICONS, "  AFTER: New tool '%s' now has %d files in cache\n",
                     newTool, newEntry->fileCount);
        }
    }
    
    log_debug(LOG_ICONS, "UpdateToolCacheForFileChange: Complete (removed=%d, added=%d)\n",
             removed, added);
    
    return TRUE;  /* Return TRUE even if individual ops failed - cache is still consistent */
#else
    return FALSE;
#endif
}

/*========================================================================*/
/* Public: Dump Tool Cache                                                */
/*========================================================================*/

/**
 * @brief Dump tool cache contents to log
 */
void DumpToolCache(void)
{
#if PLATFORM_AMIGA
    int i, j;
    
    if (!g_ToolCache || g_ToolCacheCount == 0)
    {
        log_debug(LOG_GENERAL, "\n*** Tool Cache: Empty ***\n");
        return;
    }
    
    log_debug(LOG_GENERAL, "\n========================================\n");
    log_debug(LOG_GENERAL, "Tool Validation Cache Summary\n");
    log_debug(LOG_GENERAL, "========================================\n");
    log_debug(LOG_GENERAL, "Total tools cached: %d\n", g_ToolCacheCount);
    log_debug(LOG_GENERAL, "Cache capacity: %d\n\n", g_ToolCacheCapacity);
    
    log_debug(LOG_GENERAL, "Tool Name             | Hits | Status  | Files | Full Path                              | Version\n");
    log_debug(LOG_GENERAL, "----------------------|------|---------|-------|----------------------------------------|---------------------------\n");
    
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        log_debug(LOG_GENERAL, "%-21s | %4d | %-7s | %5d | %-38s | %s\n",
                g_ToolCache[i].toolName ? g_ToolCache[i].toolName : "(null)",
                g_ToolCache[i].hitCount,
                g_ToolCache[i].exists ? "EXISTS" : "MISSING",
                g_ToolCache[i].fileCount,
                g_ToolCache[i].fullPath ? g_ToolCache[i].fullPath : "(not found)",
                g_ToolCache[i].versionString ? g_ToolCache[i].versionString : "(no version)");
        
        /* If tool has file references, dump them */
        if (g_ToolCache[i].fileCount > 0 && g_ToolCache[i].referencingFiles)
        {
            log_debug(LOG_GENERAL, "    Files using this tool:\n");
            for (j = 0; j < g_ToolCache[i].fileCount; j++)
            {
                if (g_ToolCache[i].referencingFiles[j])
                {
                    log_debug(LOG_GENERAL, "      [%3d] %s\n", j + 1, g_ToolCache[i].referencingFiles[j]);
                }
            }
            if (g_ToolCache[i].fileCount >= TOOL_CACHE_MAX_FILES_PER_TOOL)
            {
                log_debug(LOG_GENERAL, "      (max capacity reached - %d files)\n", TOOL_CACHE_MAX_FILES_PER_TOOL);
            }
            log_debug(LOG_GENERAL, "\n");
        }
    }
    
    log_debug(LOG_GENERAL, "========================================\n\n");
#endif
}

/*========================================================================*/
/* Public: Free Tool Cache                                                */
/*========================================================================*/

/**
 * @brief Free the tool cache
 */
void FreeToolCache(void)
{
#if PLATFORM_AMIGA
    int i, j;
    
    if (g_ToolCache)
    {
        /* Free all strings in each entry */
        for (i = 0; i < g_ToolCacheCount; i++)
        {
            if (g_ToolCache[i].toolName)
                whd_free(g_ToolCache[i].toolName);
            if (g_ToolCache[i].fullPath)
                whd_free(g_ToolCache[i].fullPath);
            if (g_ToolCache[i].versionString)
                whd_free(g_ToolCache[i].versionString);
            
            /* Free file references */
            if (g_ToolCache[i].referencingFiles)
            {
                for (j = 0; j < g_ToolCache[i].fileCount; j++)
                {
                    if (g_ToolCache[i].referencingFiles[j])
                        whd_free(g_ToolCache[i].referencingFiles[j]);
                }
                whd_free(g_ToolCache[i].referencingFiles);
            }
        }
        
        /* Free the array itself */
        whd_free(g_ToolCache);
        
        g_ToolCache = NULL;
        g_ToolCacheCount = 0;
        g_ToolCacheCapacity = 0;
        
        log_debug(LOG_GENERAL, "FreeToolCache: Tool cache freed\n");
    }
#endif
}
