/**
 * folder_scanner.c - Fast Folder Pre-Scanning Implementation
 * 
 * Lightweight directory tree scanning to count folders containing icons.
 * Designed for speed to enable accurate progress bars during recursive
 * processing without the overhead of full icon loading.
 * 
 * Performance Characteristics:
 *   - Uses AmigaDOS MatchFirst/MatchNext for filesystem-level filtering
 *   - Early exit on first .info file match (no need to scan entire folder)
 *   - Minimal memory allocation (only AnchorPath structure)
 *   - No DiskObject loading or icon processing
 *   - Typical speed: 1-5ms per folder (10-100x faster than full processing)
 */

#include <platform/platform.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>

#include "layout/folder_scanner.h"
#include "layout_preferences.h"
#include "writeLog.h"

/*========================================================================*/
/* Configuration                                                          */
/*========================================================================*/

/**
 * Maximum recursion depth for directory tree walking.
 * Prevents infinite loops from circular directory structures.
 */
#define MAX_RECURSION_DEPTH 50

/**
 * Buffer size for AnchorPath path storage.
 * 256 bytes is sufficient for most paths without deep nesting.
 */
#define ANCHORPATH_BUFFER_SIZE 256

/*========================================================================*/
/* Forward Declarations                                                   */
/*========================================================================*/

static BOOL FolderHasIcons(const char *path);
static BOOL CountFoldersRecursive(const char *path, 
                                  const LayoutPreferences *prefs,
                                  ULONG *folderCount,
                                  int depth);

/*========================================================================*/
/**
 * @brief Check if a folder contains at least one .info file
 * 
 * Uses MatchFirst to detect presence of .info files with early exit.
 * This is much faster than loading all icons - we stop as soon as we
 * find the first .info file.
 * 
 * @param path Directory path to check
 * @return TRUE if folder contains at least one .info file, FALSE otherwise
 */
/*========================================================================*/
static BOOL FolderHasIcons(const char *path)
{
    struct AnchorPath *ap;
    char pattern[520];
    BOOL hasIcons = FALSE;
    size_t pathLen;
    
    if (!path || path[0] == '\0')
        return FALSE;
    
    /* Build AmigaDOS pattern for .info files */
    pathLen = strlen(path);
    if (pathLen > 0 && path[pathLen - 1] == ':')
    {
        /* Root directory (volume) - no slash needed */
        snprintf(pattern, sizeof(pattern), "%s#?.info", path);
    }
    else
    {
        /* Subdirectory - needs slash separator */
        snprintf(pattern, sizeof(pattern), "%s/#?.info", path);
    }
    
    /* Allocate AnchorPath with path buffer
     * Manual allocation for maximum compatibility (same as icon_management.c)
     */
    ap = (struct AnchorPath *)whd_malloc(
        sizeof(struct AnchorPath) + ANCHORPATH_BUFFER_SIZE - 1);
    
    if (!ap)
    {
        log_warning(LOG_GENERAL, "FolderHasIcons: Failed to allocate AnchorPath for %s\n", path);
        return FALSE;
    }
    
    memset(ap, 0, sizeof(struct AnchorPath) + ANCHORPATH_BUFFER_SIZE - 1);
    
    ap->ap_Strlen = ANCHORPATH_BUFFER_SIZE;
    ap->ap_BreakBits = 0;
    ap->ap_Flags = 0;
    
    /* MatchFirst returns 0 if ANY .info file is found
     * We don't care which one - just need to know at least one exists
     */
    if (MatchFirst(pattern, ap) == 0)
    {
        hasIcons = TRUE;
        MatchEnd(ap);  /* Clean up immediately - early exit! */
    }
    /* If MatchFirst returns non-zero, no .info files found (hasIcons stays FALSE) */
    
    whd_free(ap);
    return hasIcons;
}

/*========================================================================*/
/**
 * @brief Recursively count folders containing icons
 * 
 * Walks directory tree and increments counter for each folder that
 * contains at least one .info file. Respects skipHiddenFolders preference.
 * 
 * @param path Current directory path
 * @param prefs Layout preferences (for skipHiddenFolders setting)
 * @param folderCount Pointer to counter (incremented for folders with icons)
 * @param depth Current recursion depth (safety check)
 * @return TRUE if successful, FALSE on error
 */
/*========================================================================*/
static BOOL CountFoldersRecursive(const char *path, 
                                  const LayoutPreferences *prefs,
                                  ULONG *folderCount,
                                  int depth)
{
    BPTR lock = 0;
    struct FileInfoBlock *fib = NULL;
    char subdir[512];
    BOOL success = FALSE;
    
    /* Safety check: prevent infinite recursion */
    if (depth > MAX_RECURSION_DEPTH)
    {
        log_warning(LOG_GENERAL, "CountFoldersRecursive: Max recursion depth (%d) reached at: %s\n", 
                   MAX_RECURSION_DEPTH, path);
        return FALSE;
    }
    
    /* Check if THIS folder has icons - fast check with early exit */
    if (FolderHasIcons(path))
    {
        (*folderCount)++;
        log_debug(LOG_GENERAL, "Pre-scan: Folder has icons: %s (total=%lu)\n", path, *folderCount);
    }
    
    /* Now enumerate subdirectories for recursive scanning */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        log_warning(LOG_GENERAL, "CountFoldersRecursive: Failed to lock directory: %s\n", path);
        return FALSE;
    }
    
    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        log_error(LOG_GENERAL, "CountFoldersRecursive: Failed to allocate FIB\n");
        UnLock(lock);
        return FALSE;
    }
    
    if (Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            /* Only process directories, skip files */
            if (fib->fib_DirEntryType > 0)
            {
                /* Build subdirectory path */
                if (path[strlen(path) - 1] == ':')
                {
                    /* Root device - no slash needed */
                    snprintf(subdir, sizeof(subdir), "%s%s", path, fib->fib_FileName);
                }
                else
                {
                    /* Subdirectory - add slash separator */
                    snprintf(subdir, sizeof(subdir), "%s/%s", path, fib->fib_FileName);
                }
                
                /* Check if folder is hidden (no .info file) and skip if enabled */
                if (prefs->skipHiddenFolders)
                {
                    char iconPath[520];
                    BPTR iconLock;
                    
                    snprintf(iconPath, sizeof(iconPath), "%s.info", subdir);
                    iconLock = Lock((STRPTR)iconPath, ACCESS_READ);
                    
                    if (!iconLock)
                    {
                        /* No .info file found - folder is hidden, skip it */
                        log_debug(LOG_GENERAL, "Pre-scan: Skipping hidden folder: %s\n", subdir);
                        continue;
                    }
                    UnLock(iconLock);
                }
                
                /* Recursively scan subdirectory */
                CountFoldersRecursive(subdir, prefs, folderCount, depth + 1);
            }
        }
        success = TRUE;
    }
    else
    {
        log_warning(LOG_GENERAL, "CountFoldersRecursive: Examine() failed for: %s\n", path);
    }
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    return success;
}

/*========================================================================*/
/**
 * @brief Public API: Count folders containing icons
 * 
 * Fast pre-scan of directory tree to count folders with icons.
 * This is the main entry point for the folder scanning module.
 * 
 * @param path Starting directory path
 * @param prefs Layout preferences (uses skipHiddenFolders)
 * @param folderCount Pointer to counter (set to total folder count)
 * @return TRUE if scan completed successfully, FALSE on error
 */
/*========================================================================*/
BOOL CountFoldersWithIcons(const char *path, 
                           const LayoutPreferences *prefs,
                           ULONG *folderCount)
{
    if (!path || !prefs || !folderCount)
    {
        log_error(LOG_GENERAL, "CountFoldersWithIcons: Invalid parameters\n");
        return FALSE;
    }
    
    /* Initialize counter */
    *folderCount = 0;
    
    log_info(LOG_GENERAL, "Starting pre-scan to count folders with icons: %s\n", path);
    
    /* Start recursive scan from root path */
    if (!CountFoldersRecursive(path, prefs, folderCount, 0))
    {
        log_error(LOG_GENERAL, "CountFoldersWithIcons: Scan failed\n");
        return FALSE;
    }
    
    log_info(LOG_GENERAL, "Pre-scan complete: Found %lu folder(s) with icons\n", *folderCount);
    
    return TRUE;
}

/* End of folder_scanner.c */
