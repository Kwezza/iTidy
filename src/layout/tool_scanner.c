/**
 * tool_scanner.c - Default Tool Cache Scanner Implementation
 *
 * Extracted from layout_processor.c to keep each module focused.
 * Scans directories to populate the default tool validation cache
 * without touching icon positions.
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <console_output.h>

#include "icon_types.h"
#include "icon_management.h"
#include "icon_misc.h"
#include "layout_preferences.h"
#include "writeLog.h"
#include "utilities.h"
#include "GUI/StatusWindows/main_progress_window.h"
#include "layout_processor.h"    /* GetCurrentProgressWindow, SetCurrentProgressWindow */

#include "layout/tool_scanner.h"

/*========================================================================*/
/* Filesystem Lock Release Timing Configuration                          */
/*========================================================================*/
/*
 * Same delay used in layout_processor.c for ProcessDirectoryRecursive.
 * See the detailed comment in layout_processor.c for full background.
 */
#define ENABLE_FILESYSTEM_LOCK_DELAY  1
#define FILESYSTEM_LOCK_DELAY_TICKS   1

/*========================================================================*/
/* Progress Macros (use getter so no direct access to g_progressWindow)  */
/*========================================================================*/

#define PROGRESS_STATUS(...) \
    do { \
        struct iTidyMainProgressWindow *_pw = GetCurrentProgressWindow(); \
        char _buf[256]; \
        snprintf(_buf, sizeof(_buf), __VA_ARGS__); \
        CONSOLE_STATUS("%s\n", _buf); \
        log_info(LOG_GENERAL, "%s\n", _buf); \
        if (_pw) { \
            itidy_main_progress_window_append_status(_pw, _buf); \
            itidy_main_progress_window_handle_events(_pw); \
        } \
    } while (0)

#define CHECK_CANCEL() \
    do { \
        struct iTidyMainProgressWindow *_pw = GetCurrentProgressWindow(); \
        if (_pw && _pw->cancel_requested) { \
            CONSOLE_STATUS("\n*** User cancelled operation ***\n"); \
            return FALSE; \
        } \
    } while (0)

/*========================================================================*/
/* External Globals                                                       */
/*========================================================================*/

/* Defined in icon_management.c */
extern BOOL g_ValidateDefaultTools;

/*========================================================================*/
/* Static Helper: Scan Single Directory for Tools                        */
/*========================================================================*/

static BOOL ScanSingleDirectoryForTools(const char *path)
{
    BPTR lock = 0;
    IconArray *iconArray = NULL;
    BOOL success = FALSE;

    PROGRESS_STATUS("  Scanning: %s", path);
    CHECK_CANCEL();

    /* Lock the directory */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        LONG error = IoErr();
        log_error(LOG_GENERAL, "Failed to lock directory: %s (error: %ld)\n", path, error);
        return FALSE;
    }

    /* Create icon array - this automatically validates default tools */
    iconArray = CreateIconArrayFromPath(lock, path);

    if (!iconArray)
    {
        PROGRESS_STATUS("  No icons found or error reading directory");
        UnLock(lock);
        return FALSE;
    }

    PROGRESS_STATUS("  Found %lu icons (tools validated)", (unsigned long)iconArray->size);
    success = TRUE;

    /* Clean up - we only needed the icons to validate tools */
    FreeIconArray(iconArray);
    UnLock(lock);

    return success;
}

/*========================================================================*/
/* Static Helper: Scan Directory Recursively for Tools                   */
/*========================================================================*/

static BOOL ScanDirectoryRecursiveForTools(const char *path, int recursion_level)
{
    BPTR lock = 0;
    struct FileInfoBlock *fib = NULL;
    char subdir[512];
    BOOL success = FALSE;

    /* Safety check for recursion depth */
    if (recursion_level > 50)
    {
        log_warning(LOG_GENERAL, "Maximum recursion depth reached at: %s\n", path);
        return FALSE;
    }

    /* Scan this directory first */
    if (!ScanSingleDirectoryForTools(path))
    {
        return FALSE; /* Stop if current directory fails */
    }

    /* Small delay to prevent filesystem lock issues on fast systems */
#if ENABLE_FILESYSTEM_LOCK_DELAY
    Delay(FILESYSTEM_LOCK_DELAY_TICKS);
#endif

    /* Now scan subdirectories */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        return FALSE;
    }

    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
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
                    snprintf(subdir, sizeof(subdir), "%s%s", path, fib->fib_FileName);
                }
                else
                {
                    snprintf(subdir, sizeof(subdir), "%s/%s", path, fib->fib_FileName);
                }

                /* Check for hidden folders (no .info file) */
                {
                    char iconPath[520];
                    BPTR iconLock;

                    snprintf(iconPath, sizeof(iconPath), "%s.info", subdir);
                    iconLock = Lock((STRPTR)iconPath, ACCESS_READ);

                    if (!iconLock)
                    {
                        /* Hidden folder - skip it */
                        log_debug(LOG_GENERAL, "Skipping hidden folder: %s\n", subdir);
                        continue;
                    }
                    UnLock(iconLock);
                }

                /* Recursively scan subdirectory */
                PROGRESS_STATUS("");
                PROGRESS_STATUS("Entering: %s", subdir);
                ScanDirectoryRecursiveForTools(subdir, recursion_level + 1);
            }
        }
    }

    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);

    return TRUE;
}

/*========================================================================*/
/* Public: Scan Directories for Default Tools Only (No Tidying)         */
/*========================================================================*/

BOOL ScanDirectoryForToolsOnly(void)
{
    const LayoutPreferences *prefs;
    char sanitizedPath[512];
    BOOL success = FALSE;

    /* External globals for PATH search list */
    extern char **g_PathSearchList;
    extern int g_PathSearchCount;

    /* Get global preferences */
    prefs = GetGlobalPreferences();
    if (!prefs)
    {
        log_error(LOG_GENERAL, "ScanDirectoryForToolsOnly: Failed to get global preferences\n");
        return FALSE;
    }

    /* Validate path from preferences */
    if (!prefs->folder_path || prefs->folder_path[0] == '\0')
    {
        log_error(LOG_GENERAL, "ScanDirectoryForToolsOnly: No folder path set in preferences\n");
        return FALSE;
    }

    /* Copy and sanitize the path from preferences */
    strncpy(sanitizedPath, prefs->folder_path, sizeof(sanitizedPath) - 1);
    sanitizedPath[sizeof(sanitizedPath) - 1] = '\0';
    sanitizeAmigaPath(sanitizedPath);

    /* Check if PATH search list is already built (should be at program startup) */
    if (!g_PathSearchList || g_PathSearchCount == 0)
    {
        PROGRESS_STATUS("*** Building PATH search list for default tool scanning ***");
        if (!BuildPathSearchList())
        {
            log_warning(LOG_GENERAL, "Failed to build PATH search list - validation may be limited\n");
            PROGRESS_STATUS("WARNING: Failed to build PATH search list - validation may be limited");
        }
    }
    else
    {
        PROGRESS_STATUS("*** Using PATH search list (already loaded at startup) ***");
        log_info(LOG_GENERAL, "PATH search list already built (%d directories)\n", g_PathSearchCount);
    }

    g_ValidateDefaultTools = TRUE;
    PROGRESS_STATUS("Default tool validation enabled");

    /* Free existing cache and initialize fresh */
    FreeToolCache();
    if (!InitToolCache())
    {
        log_warning(LOG_GENERAL, "Failed to initialize tool cache - scan will be slower\n");
    }

    /* Load left-out icons from the device's .backdrop file */
    loadLeftOutIcons(sanitizedPath);

    PROGRESS_STATUS("");
    PROGRESS_STATUS("Scanning for default tools: %s", sanitizedPath);
    PROGRESS_STATUS("Recursive: %s", prefs->recursive_subdirs ? "Yes" : "No");
    PROGRESS_STATUS("Mode: SCAN TOOLS ONLY (no tidying)");
    PROGRESS_STATUS("");

    CHECK_CANCEL();

    /* Start scanning */
    if (prefs->recursive_subdirs)
    {
        success = ScanDirectoryRecursiveForTools(sanitizedPath, 0);
    }
    else
    {
        success = ScanSingleDirectoryForTools(sanitizedPath);
    }

    /* Show results */
    PROGRESS_STATUS("");
    PROGRESS_STATUS("*** Tool scanning complete ***");
    DumpToolCache();

    /* Note: Tool cache remains active for viewing in Tool Cache Window */
    PROGRESS_STATUS("Tool cache retained for viewing");

    /* Note: PATH search list is kept loaded for the entire session */
    /* It will be freed at program exit */
    g_ValidateDefaultTools = FALSE;

    return success;
}

/*========================================================================*/
/* Public: Scan with Progress Window Integration                         */
/*========================================================================*/

BOOL ScanDirectoryForToolsOnlyWithProgress(struct iTidyMainProgressWindow *progress_window)
{
    BOOL result;

    if (!progress_window)
    {
        log_error(LOG_GENERAL, "ScanDirectoryForToolsOnlyWithProgress: NULL progress window\n");
        return FALSE;
    }

    /* Set global progress window so PROGRESS_STATUS/CHECK_CANCEL work */
    SetCurrentProgressWindow(progress_window);

    /* Run the main scan */
    result = ScanDirectoryForToolsOnly();

    /* Clear global progress window pointer */
    SetCurrentProgressWindow(NULL);

    return result;
}
