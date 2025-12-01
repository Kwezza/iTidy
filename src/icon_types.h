#ifndef ICON_TYPES_H
#define ICON_TYPES_H

#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stddef.h>
#include <exec/memory.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "itidy_types.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"

/* Structure to hold comprehensive icon details from a single disk read */
typedef struct {
    IconPosition position;      /* Icon X,Y coordinates */
    IconSize size;              /* Icon width and height (base bitmap only) */
    int iconType;               /* Icon format: standard, NewIcon, or OS3.5 */
    BOOL hasFrame;              /* Whether icon has a border/frame */
    char *defaultTool;          /* Default tool path (caller must free) */
    BOOL isNewIcon;             /* TRUE if NewIcon format detected */
    BOOL isOS35Icon;            /* TRUE if OS3.5 format detected */
    
    /* Calculated size fields (require emboss settings and optional text) */
    int borderWidth;            /* Actual border width (0 if frameless, embossSize otherwise) */
    IconSize iconWithEmboss;    /* Bitmap + one-side emboss (e.g., 38+3=41, 11+3=14) */
    IconSize iconVisualSize;    /* Full visual footprint with borders both sides (e.g., 44x17) */
    IconSize textSize;          /* Text label dimensions (0x0 if text not provided) */
    IconSize totalDisplaySize;  /* Icon + text + gap (complete display rectangle) */
} IconDetailsFromDisk;

/* Optimized function to read ALL icon details in a single disk operation */
BOOL GetIconDetailsFromDisk(const char *filePath, IconDetailsFromDisk *details, const char *iconTextForFont);

/* Legacy functions - still available but less efficient */
void GetNewIconSizePath(const char *filePath, IconSize *newIconSize);
BOOL GetStandardIconSize(const char *filePath, IconSize *iconSize);
IconPosition GetIconPositionFromPath(const char *iconPath);
BOOL IsNewIconPath(const STRPTR filePath);
int isOS35IconFormat(const char *filename);
BOOL IsNewIcon(struct DiskObject *diskObject);
int getOS35IconSize(const char *filename, IconSize *size);
int isIconTypeDisk(const char *filename,long fib_DirEntryType);
BOOL GetIconSizeFromFile(const char *filePath, IconSize *iconSize);

/* Function to set/update default tool */
BOOL SetIconDefaultTool(const char *iconPath, const char *newDefaultTool);

/* Function to validate if a default tool exists */
BOOL ValidateDefaultTool(const char *defaultTool);

/*========================================================================*/
/* PATH Search List - System PATH-based tool validation                  */
/*========================================================================*/

/* Global PATH search list (built from Process->pr_Path at startup) */
extern char **g_PathSearchList;
extern int g_PathSearchCount;

/**
 * @brief Build PATH search list from AmigaDOS Process->pr_Path
 * 
 * Reads the system PATH from the current process's pr_Path linked list
 * and builds a global array of search directories. This should be called
 * once at program startup.
 * 
 * @return TRUE if PATH list was built successfully, FALSE on error
 */
BOOL BuildPathSearchList(void);

/**
 * @brief Free the PATH search list
 * 
 * Releases all memory allocated by BuildPathSearchList(). Should be
 * called at program shutdown.
 */
void FreePathSearchList(void);

/*========================================================================*/
/* Tool Cache - Validated tool information cache                         */
/*========================================================================*/

/* Maximum number of file references to store per tool */
#define TOOL_CACHE_MAX_FILES_PER_TOOL  200

/**
 * @brief Cache entry for a validated default tool
 * 
 * Stores information about a tool that has been validated, including
 * its existence, location, version, and list of files that use it.
 * This prevents repeated disk access for the same tool.
 */
typedef struct {
    char *toolName;        /* Simple tool name (e.g., "MultiView") */
    BOOL exists;           /* TRUE if tool was found */
    char *fullPath;        /* Full path where found (e.g., "Workbench:Utilities/MultiView") or NULL if not found */
    char *versionString;   /* Version info (e.g., "MultiView 47.17") or NULL if unavailable */
    int hitCount;          /* Number of times this cache entry was accessed */
    
    /* NEW: File reference tracking */
    char **referencingFiles;  /* Array of file paths that use this tool */
    int fileCount;            /* Number of files currently stored */
    int fileCapacity;         /* Allocated capacity for files array */
} ToolCacheEntry;

/* Global tool cache */
extern ToolCacheEntry *g_ToolCache;
extern int g_ToolCacheCount;
extern int g_ToolCacheCapacity;

/**
 * @brief Initialize the tool cache
 * 
 * Allocates initial capacity for the tool cache. Should be called
 * at program startup.
 * 
 * @return TRUE if cache initialized successfully, FALSE on error
 */
BOOL InitToolCache(void);

/**
 * @brief Free the tool cache
 * 
 * Releases all memory allocated for the tool cache. Should be called
 * at program shutdown.
 */
void FreeToolCache(void);

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
BOOL AddFileReferenceToToolCache(const char *toolName, const char *filePath);

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
BOOL RemoveFileReferenceFromToolCache(const char *toolName, const char *filePath);

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
BOOL UpdateToolCacheForFileChange(const char *filePath, const char *oldTool, const char *newTool);

/**
 * @brief Dump tool cache contents to log
 * 
 * Outputs all cached tool information to the log for debugging and
 * reporting purposes.
 */
void DumpToolCache(void);

#endif /* ICON_TYPES_H */
