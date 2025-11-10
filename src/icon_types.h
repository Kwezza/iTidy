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
    IconSize size;              /* Icon width and height */
    int iconType;               /* Icon format: standard, NewIcon, or OS3.5 */
    BOOL hasFrame;              /* Whether icon has a border/frame */
    char *defaultTool;          /* Default tool path (caller must free) */
    BOOL isNewIcon;             /* TRUE if NewIcon format detected */
    BOOL isOS35Icon;            /* TRUE if OS3.5 format detected */
} IconDetailsFromDisk;

/* Optimized function to read ALL icon details in a single disk operation */
BOOL GetIconDetailsFromDisk(const char *filePath, IconDetailsFromDisk *details);

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

/**
 * @brief Cache entry for a validated default tool
 * 
 * Stores information about a tool that has been validated, including
 * its existence, location, and version. This prevents repeated disk
 * access for the same tool.
 */
typedef struct {
    char *toolName;        /* Simple tool name (e.g., "MultiView") */
    BOOL exists;           /* TRUE if tool was found */
    char *fullPath;        /* Full path where found (e.g., "Workbench:Utilities/MultiView") or NULL if not found */
    char *versionString;   /* Version info (e.g., "MultiView 47.17") or NULL if unavailable */
    int hitCount;          /* Number of times this cache entry was accessed */
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
 * @brief Dump tool cache contents to log
 * 
 * Outputs all cached tool information to the log for debugging and
 * reporting purposes.
 */
void DumpToolCache(void);

#endif /* ICON_TYPES_H */
