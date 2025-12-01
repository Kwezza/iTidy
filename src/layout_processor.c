/**
 * layout_processor.c - Icon Processing with Layout Preferences
 * 
 * Implements preference-aware directory processing that integrates
 * with the existing iTidy icon management system.
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <proto/wb.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

/*========================================================================*/
/* Filesystem Lock Release Timing Fix Configuration                      */
/*========================================================================*/
/*
 * BACKGROUND:
 * -----------
 * This module can crash on emulated Amiga systems running at maximum speed
 * (or very fast real hardware) during deep recursive directory processing.
 * The crash manifests as Amiga error #80000003 (software exception).
 * 
 * ROOT CAUSE ANALYSIS:
 * -------------------
 * The ProcessDirectoryRecursive() function performs nested Lock() calls on
 * the same directory path:
 *   1. ProcessSingleDirectory() locks the directory for icon processing
 *   2. Parent function immediately locks the SAME directory for subdirectory enumeration
 * 
 * When running at emulated speeds (thousands of times faster than original
 * hardware), these nested Lock()/UnLock() operations occur so rapidly that
 * the AmigaOS filesystem doesn't have time to release internal lock table
 * entries and other filesystem resources between calls.
 * 
 * On original hardware (7MHz 68000), natural I/O delays provided sufficient
 * time for cleanup. On modern emulators running at maximum speed, there is
 * essentially zero delay between operations, exhausting DOS lock tables.
 * 
 * EVIDENCE:
 * ---------
 * 1. Memory profiling showed peak usage of only 4.3KB (trivial)
 * 2. Increasing stack from 20KB to 80KB made crashes WORSE (not better)
 * 3. Enabling memory logging (which adds I/O delays) completely prevented crashes
 * 4. Crash occurred at varying depths (level 2 vs level 5+) depending on speed
 * 
 * This is a classic "Heisenbug" - observation changes behavior. The 20-50ms
 * I/O overhead from debug logging provides enough delay for the filesystem
 * to release locks properly.
 * 
 * SOLUTION:
 * ---------
 * Add a configurable delay after each directory is processed to give the
 * AmigaOS filesystem time to release internal lock resources. The delay
 * defaults to 1 tick (20ms on PAL, ~17ms on NTSC) which is imperceptible
 * to users but sufficient for filesystem cleanup.
 * 
 * This can be disabled for debugging or if running on original hardware
 * where natural I/O delays are sufficient.
 * 
 * Date: November 10, 2025
 * Issue: Recursive directory processing crash on fast emulated systems
 */

/* Enable filesystem lock release delay (disable for debugging/original hardware) */
#define ENABLE_FILESYSTEM_LOCK_DELAY 1

/* Delay in DOS ticks (1 tick = 1/50 second PAL, 1/60 second NTSC) */
/* 1 tick = ~20ms PAL, ~17ms NTSC - imperceptible to users but sufficient for DOS */
#define FILESYSTEM_LOCK_DELAY_TICKS 1

/*========================================================================*/
/* End of Configuration                                                   */
/*========================================================================*/

#include "layout_processor.h"
#include "layout_preferences.h"
#include "file_directory_handling.h"
#include "icon_management.h"
#include "icon_types.h"
#include "window_management.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "spinner.h"
#include "aspect_ratio_layout.h"
#include "folder_scanner.h"

/* Backup system integration */
#include "backup_session.h"
#include "backup_catalog.h"
#include "path_utilities.h"
#include "window_enumerator.h"

/* Progress window integration */
#include "GUI/StatusWindows/main_progress_window.h"

/* Global backup context (initialized by ProcessDirectoryWithPreferences) */
static BackupContext *g_backupContext = NULL;

/* External global for default tool validation */
extern BOOL g_ValidateDefaultTools;

/* External tool cache globals for statistics */
extern ToolCacheEntry *g_ToolCache;
extern int g_ToolCacheCount;

/* Global progress window context (set by ProcessDirectoryWithPreferencesAndProgress) */
static struct iTidyMainProgressWindow *g_progressWindow = NULL;

/* Global statistics tracking */
static ULONG g_foldersProcessed = 0;
static ULONG g_iconsProcessed = 0;
static struct DateStamp g_startTime;
static struct DateStamp g_endTime;

/* Helper macro for dual output (console + progress window) */
#define PROGRESS_STATUS(...) \
    do { \
        char _buf[256]; \
        snprintf(_buf, sizeof(_buf), __VA_ARGS__); \
        CONSOLE_STATUS("%s\n", _buf); \
        if (g_progressWindow) { \
            itidy_main_progress_window_append_status(g_progressWindow, _buf); \
            itidy_main_progress_window_handle_events(g_progressWindow); \
        } \
    } while (0)

/* Helper macro to check for user cancellation */
#define CHECK_CANCEL() \
    do { \
        if (g_progressWindow && g_progressWindow->cancel_requested) { \
            CONSOLE_STATUS("\n*** User cancelled operation ***\n"); \
            return FALSE; \
        } \
    } while (0)

/* Forward declarations for helper functions */
static int CompareIconsWithPreferences(const void *a, const void *b, 
                                      const LayoutPreferences *prefs);
static void SortIconArrayWithPreferences(IconArray *iconArray, 
                                        const LayoutPreferences *prefs);
static BOOL ProcessSingleDirectory(const char *path, 
                                   const LayoutPreferences *prefs,
                                   FolderWindowTracker *windowTracker);
static BOOL ProcessDirectoryRecursive(const char *path, 
                                     const LayoutPreferences *prefs, 
                                     int recursion_level,
                                     FolderWindowTracker *windowTracker);

/* Forward declarations for tool scanning (Phase 1: Rebuild Cache) */
static BOOL ScanSingleDirectoryForTools(const char *path);
static BOOL ScanDirectoryRecursiveForTools(const char *path, int recursion_level);

/* Forward declarations for column centering */
static void CalculateLayoutPositions(IconArray *iconArray, 
                                    const LayoutPreferences *prefs,
                                    int targetColumns);
static void CalculateLayoutPositionsWithColumnCentering(IconArray *iconArray, 
                                                       const LayoutPreferences *prefs,
                                                       int targetColumns);

/* Helper functions for sorting */
static const char* GetFileExtension(const char *filename);
static int CompareDateStamps(const struct DateStamp *date1, const struct DateStamp *date2);

/*========================================================================*/
/* Main Processing Function                                              */
/*========================================================================*/

BOOL ProcessDirectoryWithPreferencesAndProgress(struct iTidyMainProgressWindow *progress_window)
{
    BOOL result;
    
    if (!progress_window)
    {
        CONSOLE_ERROR("Error: NULL progress window pointer\n");
        return FALSE;
    }
    
    /* Set global progress window pointer */
    g_progressWindow = progress_window;
    
    /* Call the main processing function */
    result = ProcessDirectoryWithPreferences();
    
    /* Clear global progress window pointer */
    g_progressWindow = NULL;
    
    return result;
}

BOOL ProcessDirectoryWithPreferences(void)
{
    const LayoutPreferences *prefs;
    char sanitizedPath[512];
    BOOL success = FALSE;
    BackupContext localContext;
    BackupPreferences backupPrefs;
    FolderWindowTracker windowTracker;
    BOOL trackerBuilt = FALSE;
    
    /* Get global preferences */
    prefs = GetGlobalPreferences();
    if (!prefs)
    {
        CONSOLE_ERROR("Error: Failed to get global preferences\n");
        return FALSE;
    }
    
    /* Validate path from preferences */
    if (!prefs->folder_path || prefs->folder_path[0] == '\0')
    {
        CONSOLE_ERROR("Error: No folder path set in preferences\n");
        return FALSE;
    }
    
    /* Initialize statistics tracking and start timer FIRST */
    g_foldersProcessed = 0;
    g_iconsProcessed = 0;
    
    /* Get start time */
    DateStamp(&g_startTime);
    
    /* Apply logging preferences before processing */
    set_global_log_level((LogLevel)prefs->logLevel);
    set_memory_logging_enabled(prefs->memoryLoggingEnabled);
    set_performance_logging_enabled(prefs->enable_performance_logging);
    
    log_info(LOG_GENERAL, "Logging preferences applied:\n");
    log_info(LOG_GENERAL, "  Log Level: %s\n",
             prefs->logLevel == 0 ? "DEBUG" :
             prefs->logLevel == 1 ? "INFO" :
             prefs->logLevel == 2 ? "WARNING" : "ERROR");
    log_info(LOG_GENERAL, "  Memory Logging: %s\n", 
             prefs->memoryLoggingEnabled ? "ENABLED" : "DISABLED");
    log_info(LOG_GENERAL, "  Performance Logging: %s\n", 
             prefs->enable_performance_logging ? "ENABLED" : "DISABLED");
    
    /* Copy and sanitize the path from preferences */
    strncpy(sanitizedPath, prefs->folder_path, sizeof(sanitizedPath) - 1);
    sanitizedPath[sizeof(sanitizedPath) - 1] = '\0';
    sanitizeAmigaPath(sanitizedPath);
    
    /* Build window tracker if window moving is enabled (beta feature) */
    if (prefs->beta_FindWindowOnWorkbenchAndUpdate)
    {
        log_info(LOG_GENERAL, "\n*** Building window tracker for open folder windows ***\n");
        if (BuildFolderWindowList(&windowTracker))
        {
            trackerBuilt = TRUE;
            log_info(LOG_GENERAL, "Window tracker built successfully: %lu window(s) tracked\n", 
                    windowTracker.count);
        }
        else
        {
            log_warning(LOG_GENERAL, "Failed to build window tracker - window moving disabled for this run\n");
            /* Continue without window moving - not a fatal error */
        }
    }
    else
    {
        log_info(LOG_GENERAL, "Window moving disabled in preferences\n");
    }
    
    /* Build PATH search list for default tool validation */
    if (prefs->validate_default_tools)
    {
        log_info(LOG_GENERAL, "\n*** Building PATH search list for default tool validation ***\n");
        if (BuildPathSearchList())
        {
            g_ValidateDefaultTools = TRUE;
            log_info(LOG_GENERAL, "Default tool validation enabled\n");
            
            /* Free existing cache if present (from previous run) */
            FreeToolCache();
            
            /* Initialize tool cache for performance */
            if (InitToolCache())
            {
                log_info(LOG_GENERAL, "Tool cache initialized\n");
            }
            else
            {
                log_warning(LOG_GENERAL, "Failed to initialize tool cache - validation will be slower\n");
            }
        }
        else
        {
            log_warning(LOG_GENERAL, "Failed to build PATH search list - validation disabled for this run\n");
            g_ValidateDefaultTools = FALSE;
        }
    }
    else
    {
        log_info(LOG_GENERAL, "Default tool validation disabled in preferences\n");
        g_ValidateDefaultTools = FALSE;
    }
    
    /* Initialize backup session if backup is enabled */
    if (prefs->backupPrefs.enableUndoBackup)
    {
        CONSOLE_STATUS("\n*** Backup enabled - creating backup session ***\n");
        CONSOLE_STATUS("Backup location: %s\n", prefs->backupPrefs.backupRootPath);
        
        /* Copy backup preferences from layout preferences */
        backupPrefs.enableUndoBackup = prefs->backupPrefs.enableUndoBackup;
        backupPrefs.useLha = prefs->backupPrefs.useLha;
        strncpy(backupPrefs.backupRootPath, prefs->backupPrefs.backupRootPath, 
                sizeof(backupPrefs.backupRootPath) - 1);
        backupPrefs.maxBackupsPerFolder = prefs->backupPrefs.maxBackupsPerFolder;
        
        if (InitBackupSession(&localContext, &backupPrefs, sanitizedPath))
        {
            g_backupContext = &localContext;
            CONSOLE_STATUS("Backup session started successfully (Run #%u)\n", localContext.runNumber);
            CONSOLE_STATUS("Backup directory: %s\n", localContext.runDirectory);
            CONSOLE_STATUS("Source directory: %s\n", sanitizedPath);
            append_to_log("Backup session started: %s\n", localContext.runDirectory);
            append_to_log("Source directory: %s\n", sanitizedPath);
        }
        else
        {
            CONSOLE_WARNING("Warning: Failed to start backup session - continuing without backup\n");
            append_to_log("ERROR: Failed to start backup session\n");
            g_backupContext = NULL;
        }
    }
    else
    {
        CONSOLE_STATUS("\n*** Backup disabled - no backup will be created ***\n");
        g_backupContext = NULL;
    }
    
    /* Load left-out icons from the device's .backdrop file */
    loadLeftOutIcons(sanitizedPath);
    
    PROGRESS_STATUS("Processing: %s", sanitizedPath);
    PROGRESS_STATUS("Recursive: %s", prefs->recursive_subdirs ? "Yes" : "No");
    
    /* Pre-scan disabled - not currently used */
    /* NOTE: Pre-scan functionality exists but is disabled to improve performance.
     * The folder count from prescan was not being used for progress tracking.
     * If needed in the future, uncomment the block below.
     */
#if 0
    /* Pre-scan folders if recursive mode is enabled (for progress tracking) */
    if (prefs->recursive_subdirs)
    {
        ULONG totalFolders = 0;
        
        log_info(LOG_GENERAL, "\n*** Starting pre-scan to count folders with icons ***\n");
        PROGRESS_STATUS("Pre-scanning folders for progress tracking...");
        
        if (CountFoldersWithIcons(sanitizedPath, prefs, &totalFolders))
        {
            log_info(LOG_GENERAL, "Pre-scan complete: Found %lu folder(s) with icons\n", totalFolders);
            PROGRESS_STATUS("Found %lu folder(s) with icons to process", totalFolders);
            
            /* TODO: Initialize progress bar with totalFolders as maximum */
            /* This will be implemented when progress window integration is added */
        }
        else
        {
            log_warning(LOG_GENERAL, "Pre-scan failed - continuing without progress tracking\n");
            PROGRESS_STATUS("Warning: Pre-scan failed, continuing without folder count");
        }
    }
#endif
    
    /* Start processing */
    CHECK_CANCEL();
    
    if (prefs->recursive_subdirs)
    {
        success = ProcessDirectoryRecursive(sanitizedPath, prefs, 0, 
                                           trackerBuilt ? &windowTracker : NULL);
    }
    else
    {
        success = ProcessSingleDirectory(sanitizedPath, prefs,
                                        trackerBuilt ? &windowTracker : NULL);
    }
    
    /* Free window tracker if it was built */
    if (trackerBuilt)
    {
        log_info(LOG_GENERAL, "Freeing window tracker\n");
        FreeFolderWindowList(&windowTracker);
    }
    
    /* Calculate elapsed time and display statistics */
    {
        LONG elapsedDays, elapsedMinutes, elapsedTicks;
        LONG totalSeconds;
        LONG minutes, seconds;
        
        DateStamp(&g_endTime);
        
        /* Calculate difference in each field */
        elapsedDays = g_endTime.ds_Days - g_startTime.ds_Days;
        elapsedMinutes = g_endTime.ds_Minute - g_startTime.ds_Minute;
        elapsedTicks = g_endTime.ds_Tick - g_startTime.ds_Tick;
        
        /* Convert to total seconds */
        /* DateStamp: ds_Days = days since Jan 1, 1978 */
        /*            ds_Minute = minutes past midnight (0-1439) */
        /*            ds_Tick = ticks past current minute (0-2999, 50 ticks/sec) */
        totalSeconds = (elapsedDays * 24 * 60 * 60) +  /* Days to seconds */
                       (elapsedMinutes * 60) +           /* Minutes to seconds */
                       (elapsedTicks / 50);              /* Ticks to seconds (PAL: 50/sec) */
        
        minutes = totalSeconds / 60;
        seconds = totalSeconds % 60;
        
        /* Display final statistics */
        PROGRESS_STATUS("");
        PROGRESS_STATUS("=== Processing Statistics ===");
        PROGRESS_STATUS("  Folders processed: %lu", g_foldersProcessed);
        PROGRESS_STATUS("  Icons processed: %lu", g_iconsProcessed);
        
        /* Display default tool validation statistics if enabled */
        if (g_ValidateDefaultTools && g_ToolCache && g_ToolCacheCount > 0)
        {
            int validTools = 0;
            int missingTools = 0;
            int i;
            
            /* Count valid vs missing tools */
            for (i = 0; i < g_ToolCacheCount; i++)
            {
                if (g_ToolCache[i].exists)
                    validTools++;
                else
                    missingTools++;
            }
            
            PROGRESS_STATUS("  Default tools: %d valid, %d missing", validTools, missingTools);
        }
        
        if (minutes > 0)
            PROGRESS_STATUS("  Total time: %ld min %ld sec", minutes, seconds);
        else
            PROGRESS_STATUS("  Total time: %ld seconds", seconds);
    }
    
    /* Free PATH search list if validation was enabled */
    if (g_ValidateDefaultTools)
    {
        log_info(LOG_GENERAL, "\n*** Tool cache summary ***\n");
        DumpToolCache();  /* Show all cached tools after processing */
        
        /* Note: Tool cache is NOT freed here - it remains available for post-processing */
        log_info(LOG_GENERAL, "Tool cache retained for post-processing\n");
        
        log_info(LOG_GENERAL, "Freeing PATH search list\n");
        FreePathSearchList();
        g_ValidateDefaultTools = FALSE;
    }
    
    /* End backup session if one was started */
    if (g_backupContext != NULL)
    {
        PROGRESS_STATUS("");
        PROGRESS_STATUS("*** Finalizing backup session ***");
        CloseBackupSession(g_backupContext);
        PROGRESS_STATUS("Backup session completed successfully");
        PROGRESS_STATUS("  Folders backed up: %u", g_backupContext->foldersBackedUp);
        PROGRESS_STATUS("  Total bytes archived: %lu", (unsigned long)g_backupContext->totalBytesArchived);
        PROGRESS_STATUS("  Location: %s", g_backupContext->runDirectory);

        log_info(LOG_GENERAL, "\n*** Backup session completed ***\n");
        append_to_log("  Folders backed up: %u\n", g_backupContext->foldersBackedUp);
        append_to_log("  Failed backups: %u\n", g_backupContext->failedBackups);
        append_to_log("  Total bytes: %lu\n", (unsigned long)g_backupContext->totalBytesArchived);
        append_to_log("  Location: %s\n", g_backupContext->runDirectory);
        
        g_backupContext = NULL;
    }
    
    return success;
}

/*========================================================================*/
/* Scan Directory for Default Tools Only (No Tidying)                    */
/*========================================================================*/

/**
 * @brief Scan directories for tools with progress window integration
 * 
 * Wrapper that sets the global progress window pointer before calling
 * ScanDirectoryForToolsOnly(), enabling progress updates and cancellation.
 * 
 * @param progress_window Pointer to opened progress window
 * @return TRUE if successful, FALSE on error or cancellation
 */
BOOL ScanDirectoryForToolsOnlyWithProgress(struct iTidyMainProgressWindow *progress_window)
{
    BOOL result;
    
    if (!progress_window)
    {
        log_error(LOG_GENERAL, "Error: NULL progress window pointer\n");
        return FALSE;
    }
    
    /* Set global progress window pointer */
    g_progressWindow = progress_window;
    
    /* Call the main scanning function */
    result = ScanDirectoryForToolsOnly();
    
    /* Clear global progress window pointer */
    g_progressWindow = NULL;
    
    return result;
}

/**
 * @brief Scan directories and build default tool cache without tidying icons
 * 
 * This function walks through directories and loads icons purely to validate
 * their default tools and populate the tool cache. Unlike ProcessDirectoryWithPreferences,
 * it does NOT sort, layout, resize, or save any changes to icons.
 * 
 * Uses the global preferences for path, recursive mode, and skipHiddenFolders setting.
 * 
 * @return TRUE if scan completed successfully
 */
BOOL ScanDirectoryForToolsOnly(void)
{
    const LayoutPreferences *prefs;
    char sanitizedPath[512];
    BOOL success = FALSE;
    
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
    
    /* Build PATH search list for default tool validation */
    PROGRESS_STATUS("*** Building PATH search list for default tool scanning ***");
    if (!BuildPathSearchList())
    {
        log_error(LOG_GENERAL, "Failed to build PATH search list - scan aborted\n");
        PROGRESS_STATUS("ERROR: Failed to build PATH search list - scan aborted");
        return FALSE;
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
    
    /* Free PATH search list */
    PROGRESS_STATUS("Freeing PATH search list");
    FreePathSearchList();
    g_ValidateDefaultTools = FALSE;
    
    return success;
}

/*========================================================================*/
/* Scan Single Directory for Tools (Helper Function)                     */
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
/* Scan Directory Recursively for Tools (Helper Function)                */
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
/* Calculate Icon Positions Based on Layout Preferences                  */
/*========================================================================*/

static void CalculateLayoutPositions(IconArray *iconArray, 
                                    const LayoutPreferences *prefs,
                                    int targetColumns)
{
    int i;
    int iconSpacingX;   /* Horizontal spacing between icons */
    int iconSpacingY;   /* Vertical spacing between icons */
    int currentX;       /* Current X position */
    int currentY;       /* Current Y position */
    int maxWidth = 640; /* Default screen width */
    int effectiveMaxWidth;
    int rightMargin;    /* Safety margin on right edge */
    int nextX;
    int maxHeightInRow = 0; /* Track tallest icon in current row */
    int rowStartY;      /* Y position where current row started */
    int rowStartIndex = 0; /* Index of first icon in current row */
    int adjustmentOffset;
    int iconsInCurrentRow = 0; /* Track icons placed in current row */
    
    /* Use spacing from preferences */
    iconSpacingX = prefs->iconSpacingX;
    iconSpacingY = prefs->iconSpacingY;
    
    /* Starting positions use spacing as margin */
    currentX = iconSpacingX;
    currentY = iconSpacingY;
    rowStartY = iconSpacingY;
    rightMargin = iconSpacingX;
    
    if (!iconArray || iconArray->size == 0)
        return;
    
    /* Use actual screen width if available */
    if (screenWidth > 0)
        maxWidth = screenWidth;
    
    /* Apply maxWindowWidthPct to limit the usable width */
    if (prefs->maxWindowWidthPct > 0 && prefs->maxWindowWidthPct < 100)
    {
        effectiveMaxWidth = (maxWidth * prefs->maxWindowWidthPct) / 100;
    }
    else
    {
        effectiveMaxWidth = maxWidth;
    }
    
#ifdef DEBUG
    append_to_log("CalculateLayoutPositions: Positioning %d icons (targetColumns=%d)\n", 
                  iconArray->size, targetColumns);
    append_to_log("  screenWidth=%d, maxWindowWidthPct=%d, effectiveMaxWidth=%d\n",
                  maxWidth, prefs->maxWindowWidthPct, effectiveMaxWidth);
    append_to_log("  textAlignment=%s\n", 
                  prefs->textAlignment == TEXT_ALIGN_TOP ? "TOP" :
                  prefs->textAlignment == TEXT_ALIGN_MIDDLE ? "MIDDLE" : "BOTTOM");
    append_to_log("%-3s | %-30s | %-4s | %-4s | %-5s | %-5s | %-6s\n", 
                  "ID", "Icon", "NewX", "NewY", "Width", "Height", "Offset");
    append_to_log("------------------------------------------------------------------------------------\n");
#endif
    
    /* FIRST PASS: Position icons in rows, tracking row boundaries */
    for (i = 0; i < iconArray->size; i++)
    {
        FullIconDetails *icon = &iconArray->array[i];
        int iconOffset = 0;
        BOOL shouldWrap = FALSE;
        
        /* Calculate where the NEXT icon would start */
        nextX = currentX + icon->icon_max_width + iconSpacingX;
        
        /* Check if we should wrap based on target columns or width constraint */
        if (targetColumns > 0)
        {
            /* Aspect ratio mode: wrap based on column count */
            shouldWrap = (iconsInCurrentRow >= targetColumns);
        }
        else
        {
            /* Traditional mode: wrap based on width */
            shouldWrap = (nextX > (effectiveMaxWidth - rightMargin));
        }
        
        /* If should wrap to next row (but always place at least one icon per row) */
        if (currentX > iconSpacingX && shouldWrap)
        {
            /* Before starting new row, apply vertical alignment to previous row */
            if (maxHeightInRow > 0)
            {
                /* Adjust all icons in the previous row based on alignment setting */
                for (int j = rowStartIndex; j < i; j++)
                {
                    FullIconDetails *rowIcon = &iconArray->array[j];
                    
                    /* Calculate adjustment based on alignment mode */
                    switch (prefs->textAlignment)
                    {
                        case TEXT_ALIGN_TOP:
                            /* No adjustment needed - icons already at top */
                            adjustmentOffset = 0;
                            break;
                            
                        case TEXT_ALIGN_MIDDLE:
                            /* Center icon vertically within row height */
                            adjustmentOffset = (maxHeightInRow - rowIcon->icon_max_height) / 2;
                            break;
                            
                        case TEXT_ALIGN_BOTTOM:
                            /* Align to bottom of row */
                            adjustmentOffset = maxHeightInRow - rowIcon->icon_max_height;
                            break;
                            
                        default:
                            adjustmentOffset = 0;
                            break;
                    }
                    
                    if (adjustmentOffset > 0)
                    {
                        rowIcon->icon_y += adjustmentOffset;
#ifdef DEBUG
                        append_to_log("  Adjusted icon %d ('%s') down by %d pixels for %s alignment\n",
                                      j, rowIcon->icon_text, adjustmentOffset,
                                      prefs->textAlignment == TEXT_ALIGN_TOP ? "top" :
                                      prefs->textAlignment == TEXT_ALIGN_MIDDLE ? "middle" : "bottom");
#endif
                    }
                }
            }
            
            /* Start new row - use the tallest icon from previous row */
            currentX = iconSpacingX;
            currentY = rowStartY + maxHeightInRow + iconSpacingY;
            rowStartY = currentY;
            rowStartIndex = i; /* Mark start of new row */
            maxHeightInRow = 0; /* Reset for new row */
            iconsInCurrentRow = 0; /* Reset column counter for new row */
        }
        
        /* If text is wider than icon, center the icon within the text width */
        if (icon->text_width > icon->icon_width)
        {
            iconOffset = (icon->text_width - icon->icon_width) / 2;
        }
        
        /* Set position for this icon (with centering offset if needed) */
        icon->icon_x = currentX + iconOffset;
        icon->icon_y = currentY;
        
        /* Track the tallest icon in this row */
        if (icon->icon_max_height > maxHeightInRow)
            maxHeightInRow = icon->icon_max_height;

#ifdef DEBUG
        append_to_log("%-3d | %-30s | %-4d | %-4d | %-5d | %-5d | %-3d\n", 
                      i, icon->icon_text, icon->icon_x, icon->icon_y, 
                      icon->icon_max_width, icon->icon_max_height, iconOffset);
#endif
        
        /* Advance X position for next icon (using max width, not offset position) */
        currentX += icon->icon_max_width + iconSpacingX;
        iconsInCurrentRow++; /* Count icons in current row */
    }
    
    /* SECOND PASS: Adjust the last row with vertical alignment */
    /* (This handles the final row that didn't trigger the wrap condition) */
    if (maxHeightInRow > 0)
    {
#ifdef DEBUG
        append_to_log("\nAdjusting final row (indices %d to %d) for vertical alignment\n",
                      rowStartIndex, iconArray->size - 1);
        append_to_log("  maxHeightInRow = %d, alignment = %s\n", maxHeightInRow,
                      prefs->textAlignment == TEXT_ALIGN_TOP ? "top" :
                      prefs->textAlignment == TEXT_ALIGN_MIDDLE ? "middle" : "bottom");
#endif
        
        for (int j = rowStartIndex; j < iconArray->size; j++)
        {
            FullIconDetails *rowIcon = &iconArray->array[j];
            
            /* Calculate adjustment based on alignment mode */
            switch (prefs->textAlignment)
            {
                case TEXT_ALIGN_TOP:
                    /* No adjustment needed - icons already at top */
                    adjustmentOffset = 0;
                    break;
                    
                case TEXT_ALIGN_MIDDLE:
                    /* Center icon vertically within row height */
                    adjustmentOffset = (maxHeightInRow - rowIcon->icon_max_height) / 2;
                    break;
                    
                case TEXT_ALIGN_BOTTOM:
                    /* Align to bottom of row */
                    adjustmentOffset = maxHeightInRow - rowIcon->icon_max_height;
                    break;
                    
                default:
                    adjustmentOffset = 0;
                    break;
            }
            
            if (adjustmentOffset > 0)
            {
                rowIcon->icon_y += adjustmentOffset;
#ifdef DEBUG
                append_to_log("  Adjusted icon %d ('%s') down by %d pixels for %s alignment\n",
                              j, rowIcon->icon_text, adjustmentOffset,
                              prefs->textAlignment == TEXT_ALIGN_TOP ? "top" :
                              prefs->textAlignment == TEXT_ALIGN_MIDDLE ? "middle" : "bottom");
#endif
            }
        }
    }
    
#ifdef DEBUG
    append_to_log("\nFinal icon positions:\n");
    append_to_log("%-3s | %-30s | %-4s | %-4s\n", "ID", "Icon", "X", "Y");
    append_to_log("--------------------------------------------\n");
    for (i = 0; i < iconArray->size; i++)
    {
        append_to_log("%-3d | %-30s | %-4d | %-4d\n", 
                      i, iconArray->array[i].icon_text,
                      iconArray->array[i].icon_x, iconArray->array[i].icon_y);
    }
#endif
}

/*========================================================================*/
/* Calculate Icon Positions With Column Centering                        */
/*========================================================================*/
/**
 * @brief Position icons with column-based centering
 * 
 * This function implements a two-pass algorithm for centering icons within
 * columns where each column's width is determined by its widest icon.
 * 
 * Algorithm Overview:
 * Phase 1: Statistics & Column Width Calculation
 *   - Calculate average icon width for initial column estimate
 *   - Determine how many columns can fit in window
 *   - Assign icons to columns and calculate actual max width per column
 *   - Validate that total width fits in window (adjust if needed)
 * 
 * Phase 2: Icon Positioning
 *   - Position each icon within its column
 *   - Center narrower icons within their column width
 *   - Apply text alignment adjustments
 * 
 * Performance: Maximum 2 calculation passes, efficient for 1000+ icons
 * 
 * @param iconArray Array of icons to position
 * @param prefs Layout preferences including centerIconsInColumn flag
 */
static void CalculateLayoutPositionsWithColumnCentering(IconArray *iconArray, 
                                                       const LayoutPreferences *prefs,
                                                       int targetColumns)
{
    int i, col, row;
    int iconSpacingX;   /* Horizontal spacing between icons */
    int iconSpacingY;   /* Vertical spacing between icons */
    int maxWidth = 640;
    int effectiveMaxWidth;
    int rightMargin;
    int startX;
    int startY;
    
    /* Use spacing from preferences */
    iconSpacingX = prefs->iconSpacingX;
    iconSpacingY = prefs->iconSpacingY;
    
    /* Starting positions use defined margins (ICON_START_X/Y) for proper window padding */
    startX = ICON_START_X;
    startY = ICON_START_Y;
    rightMargin = iconSpacingX;
    
    /* Phase 1 variables: Statistics and column calculation */
    int totalWidth = 0;
    int averageWidth = 0;
    int estimatedColumns = 0;
    int finalColumns = targetColumns; /* Use aspect ratio calculated columns */
    int *columnWidths = NULL;
    int *columnXPositions = NULL;
    int calculationAttempts = 0;
    
    /* Phase 2 variables: Positioning */
    int maxHeightInRow = 0;
    int rowStartIndex = 0;
    int rowStartY = startY;
    int adjustmentOffset;
    int iconX, iconY;
    int centerOffset;
    
    if (!iconArray || iconArray->size == 0)
        return;
    
    /* Use actual screen width if available */
    if (screenWidth > 0)
        maxWidth = screenWidth;
    
    /* Apply maxWindowWidthPct to limit the usable width */
    if (prefs->maxWindowWidthPct > 0 && prefs->maxWindowWidthPct < 100)
    {
        effectiveMaxWidth = (maxWidth * prefs->maxWindowWidthPct) / 100;
    }
    else
    {
        effectiveMaxWidth = maxWidth;
    }
    
#ifdef DEBUG
    append_to_log("CalculateLayoutPositionsWithColumnCentering: Processing %d icons\n", 
                  iconArray->size);
    append_to_log("  screenWidth=%d, maxWindowWidthPct=%d, effectiveMaxWidth=%d\n",
                  maxWidth, prefs->maxWindowWidthPct, effectiveMaxWidth);
#endif
    
    /*====================================================================*/
    /* PHASE 1: STATISTICS & COLUMN WIDTH CALCULATION                    */
    /*====================================================================*/
    
    /* Step 1: Calculate average icon width for initial estimate */
    totalWidth = 0;
    for (i = 0; i < iconArray->size; i++)
    {
        totalWidth += iconArray->array[i].icon_max_width;
    }
    averageWidth = totalWidth / iconArray->size;
    
#ifdef DEBUG
    append_to_log("Phase 1: Statistics\n");
    append_to_log("  Average icon width: %d pixels\n", averageWidth);
    append_to_log("  Total icons: %d\n", iconArray->size);
#endif
    
    /* Step 2: Estimate initial column count (or use target from aspect ratio) */
    if (targetColumns > 0)
    {
        /* Use aspect ratio calculated columns */
        estimatedColumns = targetColumns;
        finalColumns = targetColumns;
#ifdef DEBUG
        append_to_log("  Using aspect ratio target columns: %d\n", targetColumns);
#endif
    }
    else
    {
        /* Calculate columns based on width */
        estimatedColumns = (effectiveMaxWidth - startX - rightMargin) / (averageWidth + iconSpacingX);
        if (estimatedColumns < 1)
            estimatedColumns = 1;
        finalColumns = estimatedColumns;
#ifdef DEBUG
        append_to_log("  Initial estimated columns: %d\n", estimatedColumns);
#endif
    }
    
    /* Allocate arrays for column widths and positions */
    columnWidths = (int *)malloc(estimatedColumns * sizeof(int));
    columnXPositions = (int *)malloc(estimatedColumns * sizeof(int));
    
    if (!columnWidths || !columnXPositions)
    {
        /* Fallback: use standard layout if allocation fails */
#ifdef DEBUG
        append_to_log("ERROR: Failed to allocate column arrays, falling back to standard layout\n");
#endif
        if (columnWidths) free(columnWidths);
        if (columnXPositions) free(columnXPositions);
        CalculateLayoutPositions(iconArray, prefs, targetColumns);
        return;
    }
    
    /* Step 3 & 4: Calculate actual column widths (with potential adjustment) */
    /* Skip adjustment loop if using targetColumns */
    
    do {
        calculationAttempts++;
        
        /* Clear column width array */
        for (col = 0; col < finalColumns; col++)
        {
            columnWidths[col] = 0;
        }
        
        /* Calculate column widths based on optimization setting */
        if (prefs->useColumnWidthOptimization)
        {
            /* OPTIMIZED: Calculate maximum width for each column individually */
            for (i = 0; i < iconArray->size; i++)
            {
                col = i % finalColumns;
                if (iconArray->array[i].icon_max_width > columnWidths[col])
                {
                    columnWidths[col] = iconArray->array[i].icon_max_width;
                }
            }
            
#ifdef DEBUG
            append_to_log("Using optimized column widths (per-column max)\n");
#endif
        }
        else
        {
            /* UNIFORM: Use the same width for all columns (widest icon overall) */
            int uniformWidth = 0;
            
            /* Find widest icon across all icons */
            for (i = 0; i < iconArray->size; i++)
            {
                if (iconArray->array[i].icon_max_width > uniformWidth)
                {
                    uniformWidth = iconArray->array[i].icon_max_width;
                }
            }
            
            /* Apply uniform width to all columns */
            for (col = 0; col < finalColumns; col++)
            {
                columnWidths[col] = uniformWidth;
            }
            
#ifdef DEBUG
            append_to_log("Using uniform column width: %d pixels (widest icon)\n", uniformWidth);
#endif
        }
        
        /* Step 5: Validate total width */
        totalWidth = startX;
        for (col = 0; col < finalColumns; col++)
        {
            totalWidth += columnWidths[col];
            if (col < finalColumns - 1)
                totalWidth += iconSpacingX;
        }
        totalWidth += rightMargin;
        
#ifdef DEBUG
        append_to_log("Attempt %d: %d columns, total width = %d (max = %d)\n",
                      calculationAttempts, finalColumns, totalWidth, effectiveMaxWidth);
#endif
        
        /* If using targetColumns, accept width even if it exceeds screen */
        if (targetColumns > 0)
        {
#ifdef DEBUG
            append_to_log("  Using aspect ratio target, accepting width\n");
#endif
            break;
        }
        
        /* If doesn't fit and we have more than 1 column, reduce and recalculate */
        if (totalWidth > effectiveMaxWidth && finalColumns > 1)
        {
            finalColumns--;
#ifdef DEBUG
            append_to_log("  Width exceeded, reducing to %d columns\n", finalColumns);
#endif
        }
        else
        {
            /* Fits! Break out of loop */
            break;
        }
        
    } while (calculationAttempts < 2); /* Maximum 2 attempts */
    
#ifdef DEBUG
    append_to_log("Final configuration: %d columns, total width = %d\n", 
                  finalColumns, totalWidth);
    append_to_log("Column widths: ");
    for (col = 0; col < finalColumns; col++)
    {
        append_to_log("%d ", columnWidths[col]);
    }
    append_to_log("\n");
#endif
    
    /* Calculate X position for start of each column */
    columnXPositions[0] = startX;
    for (col = 1; col < finalColumns; col++)
    {
        columnXPositions[col] = columnXPositions[col - 1] + columnWidths[col - 1] + iconSpacingX;
    }
    
    /*====================================================================*/
    /* PHASE 2: ICON POSITIONING WITH CENTERING                          */
    /*====================================================================*/
    
#ifdef DEBUG
    append_to_log("\nPhase 2: Icon Positioning\n");
    append_to_log("%-3s | %-30s | Col | Row | %-4s | %-4s | Center\n", 
                  "ID", "Icon", "X", "Y");
    append_to_log("-------------------------------------------------------------------------\n");
#endif
    
    /* Position each icon within its column */
    for (i = 0; i < iconArray->size; i++)
    {
        FullIconDetails *icon = &iconArray->array[i];
        
        /* Determine column and row */
        col = i % finalColumns;
        row = i / finalColumns;
        
        /* Start a new row? Track row boundaries for text alignment */
        if (col == 0 && i > 0)
        {
            /* Apply vertical alignment to previous row */
            if (maxHeightInRow > 0)
            {
                for (int j = rowStartIndex; j < i; j++)
                {
                    FullIconDetails *rowIcon = &iconArray->array[j];
                    
                    /* Calculate adjustment based on alignment mode */
                    switch (prefs->textAlignment)
                    {
                        case TEXT_ALIGN_TOP:
                            /* No adjustment needed - icons already at top */
                            adjustmentOffset = 0;
                            break;
                            
                        case TEXT_ALIGN_MIDDLE:
                            /* Center icon vertically within row height */
                            adjustmentOffset = (maxHeightInRow - rowIcon->icon_max_height) / 2;
                            break;
                            
                        case TEXT_ALIGN_BOTTOM:
                            /* Align to bottom of row */
                            adjustmentOffset = maxHeightInRow - rowIcon->icon_max_height;
                            break;
                            
                        default:
                            adjustmentOffset = 0;
                            break;
                    }
                    
                    if (adjustmentOffset > 0)
                    {
                        rowIcon->icon_y += adjustmentOffset;
                    }
                }
            }
            
            /* Move to next row */
            rowStartY += maxHeightInRow + iconSpacingY;
            rowStartIndex = i;
            maxHeightInRow = 0;
        }
        
        /* Calculate Y position */
        iconY = rowStartY;
        
        /* Calculate X position with centering within column */
        centerOffset = (columnWidths[col] - icon->icon_max_width) / 2;
        if (centerOffset < 0)
            centerOffset = 0;
        
        iconX = columnXPositions[col] + centerOffset;
        
        /* Additional centering if text is wider than icon */
        if (icon->text_width > icon->icon_width)
        {
            int textCenterOffset = (icon->text_width - icon->icon_width) / 2;
            iconX += textCenterOffset;
        }
        
        /* Set icon position */
        icon->icon_x = iconX;
        icon->icon_y = iconY;
        
        /* Track tallest icon in current row */
        if (icon->icon_max_height > maxHeightInRow)
            maxHeightInRow = icon->icon_max_height;
        
#ifdef DEBUG
        append_to_log("%-3d | %-30s | %-3d | %-3d | %-4d | %-4d | %-6d\n",
                      i, icon->icon_text, col, row, 
                      icon->icon_x, icon->icon_y, centerOffset);
#endif
    }
    
    /* Apply vertical alignment to final row */
    if (maxHeightInRow > 0)
    {
#ifdef DEBUG
        append_to_log("\nAdjusting final row (indices %d to %d) for vertical alignment\n",
                      rowStartIndex, iconArray->size - 1);
        append_to_log("  maxHeightInRow = %d, alignment = %s\n", maxHeightInRow,
                      prefs->textAlignment == TEXT_ALIGN_TOP ? "top" :
                      prefs->textAlignment == TEXT_ALIGN_MIDDLE ? "middle" : "bottom");
#endif
        for (int j = rowStartIndex; j < iconArray->size; j++)
        {
            FullIconDetails *rowIcon = &iconArray->array[j];
            
            /* Calculate adjustment based on alignment mode */
            switch (prefs->textAlignment)
            {
                case TEXT_ALIGN_TOP:
                    /* No adjustment needed - icons already at top */
                    adjustmentOffset = 0;
                    break;
                    
                case TEXT_ALIGN_MIDDLE:
                    /* Center icon vertically within row height */
                    adjustmentOffset = (maxHeightInRow - rowIcon->icon_max_height) / 2;
                    break;
                    
                case TEXT_ALIGN_BOTTOM:
                    /* Align to bottom of row */
                    adjustmentOffset = maxHeightInRow - rowIcon->icon_max_height;
                    break;
                    
                default:
                    adjustmentOffset = 0;
                    break;
            }
            
            if (adjustmentOffset > 0)
            {
                rowIcon->icon_y += adjustmentOffset;
#ifdef DEBUG
                append_to_log("  Adjusted icon %d down by %d pixels\n", j, adjustmentOffset);
#endif
            }
        }
    }
    
#ifdef DEBUG
    append_to_log("\nColumn-centered layout complete!\n");
#endif
    
    /* Cleanup */
    free(columnWidths);
    free(columnXPositions);
}

/*========================================================================*/
/* Single Directory Processing                                           */
/*========================================================================*/

static BOOL ProcessSingleDirectory(const char *path, 
                                   const LayoutPreferences *prefs,
                                   FolderWindowTracker *windowTracker)
{
    BPTR lock = 0;
    IconArray *iconArray = NULL;
    BOOL success = FALSE;
    
    /* Reset logging performance stats for this folder */
    reset_log_performance_stats();
    
#ifdef DEBUG
    append_to_log("ProcessSingleDirectory: path='%s'\n", path);
#endif
    
    /* Lock the directory */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        LONG error = IoErr();
        CONSOLE_ERROR("Failed to lock directory: %s (error: %ld)\n", path, error);
#ifdef DEBUG
        append_to_log("Failed to lock %s, error: %ld\n", path, error);
#endif
        return FALSE;
    }
    
    /* Create icon array from directory */
    PROGRESS_STATUS("  Scanning for icons...");
#ifdef DEBUG
    append_to_log("==== SECTION: LOADING ICONS ====\n");
#endif
    iconArray = CreateIconArrayFromPath(lock, path);
    
    CHECK_CANCEL();
    
    if (!iconArray)
    {
        PROGRESS_STATUS("  Error reading directory");
#ifdef DEBUG
        append_to_log("Error creating icon array: %s\n", path);
#endif
        UnLock(lock);
        return FALSE;
    }
    
    if (iconArray->size == 0)
    {
        PROGRESS_STATUS("  No icons found in directory");
#ifdef DEBUG
        append_to_log("No icons in directory: %s\n", path);
#endif
        
        /* Resize window to default empty folder size if requested */
        if (prefs->resizeWindows)
        {
            PROGRESS_STATUS("  Resizing to default empty folder size...");
            resizeFolderToContents((char *)path, iconArray, windowTracker, prefs);
        }
        
        FreeIconArray(iconArray);
        UnLock(lock);
        return TRUE;  /* Success - folder processed (even though empty) */
    }
    
    PROGRESS_STATUS("  Found %lu icons", (unsigned long)iconArray->size);
    
    /* Update statistics */
    g_foldersProcessed++;
    g_iconsProcessed += iconArray->size;
    
    CHECK_CANCEL();
    
    /* Backup this directory if backup is enabled */
    if (g_backupContext != NULL)
    {
        PROGRESS_STATUS("  Creating backup...");
        BackupStatus status = BackupFolder(g_backupContext, path, (UWORD)iconArray->size);
        if (status == BACKUP_OK)
        {
            PROGRESS_STATUS("  Backup created successfully");
        }
        else
        {
            PROGRESS_STATUS("  Warning: Backup failed (status %d) - continuing anyway", status);
        }
        CHECK_CANCEL();
    }
    
    /* Sort icons according to preferences */
    PROGRESS_STATUS("  Sorting icons...");
#ifdef DEBUG
    append_to_log("==== SECTION: SORTING ICONS ====\n");
#endif
    SortIconArrayWithPreferences(iconArray, prefs);
    
    CHECK_CANCEL();
    
    /* Calculate and apply new positions based on sorted order */
#ifdef DEBUG
    append_to_log("==== SECTION: CALCULATING LAYOUT ====\n");
#endif
    
    /* Calculate optimal layout based on aspect ratio and overflow preferences */
    {
        int finalColumns = 0;
        int finalRows = 0;
        
        /* Use aspect ratio calculation to determine optimal columns/rows */
        CalculateLayoutWithAspectRatio(iconArray, prefs, &finalColumns, &finalRows);
        
#ifdef DEBUG
        append_to_log("Aspect ratio calculation complete: %d cols × %d rows\n", 
                      finalColumns, finalRows);
#endif
        
        /* Choose layout algorithm based on centerIconsInColumn preference */
        if (prefs->centerIconsInColumn)
        {
#ifdef DEBUG
            append_to_log("Using column-centered layout algorithm\n");
#endif
            CalculateLayoutPositionsWithColumnCentering(iconArray, prefs, finalColumns);
        }
        else
        {
#ifdef DEBUG
            append_to_log("Using standard row-based layout algorithm\n");
#endif
            CalculateLayoutPositions(iconArray, prefs, finalColumns);
        }
    }
    
    /* Resize window if requested */
    if (prefs->resizeWindows)
    {
        PROGRESS_STATUS("  Resizing window...");
        resizeFolderToContents((char *)path, iconArray, windowTracker, prefs);
    }
    
    CHECK_CANCEL();
    
    /* Save icon positions to disk */
    PROGRESS_STATUS("  Saving icon positions...");
#ifdef DEBUG
    append_to_log("==== SECTION: SAVING ICONS ====\n");
#endif
    if (saveIconsPositionsToDisk(iconArray) == 0)
    {
        success = TRUE;
        PROGRESS_STATUS("  Done!");
    }
    else
    {
        PROGRESS_STATUS("  Failed to save icon positions");
    }
    
    /* Experimental Feature: Auto-open folders during processing
     * When enabled, this calls Workbench's OpenWorkbenchObjectA() to open
     * the folder window after iTidy has tidied the icons. This allows users
     * to see the results in real-time during recursive operations.
     */
    if (prefs->beta_openFoldersAfterProcessing && success)
    {
        BOOL openResult;
        
#ifdef DEBUG
        append_to_log("Opening folder window via Workbench: %s\n", path);
#endif
        
        /* OpenWorkbenchObjectA() opens a folder window via Workbench.
         * - First parameter: Full path to the folder
         * - Second parameter: TagList (NULL for default behavior)
         * 
         * Note: workbench.library is auto-opened by proto/wb.h,
         * so we don't need to manually OpenLibrary().
         * 
         * Returns: TRUE if successful, FALSE otherwise
         */
        openResult = OpenWorkbenchObjectA((CONST_STRPTR)path, NULL);
        
        if (!openResult)
        {
#ifdef DEBUG
            append_to_log("Warning: Failed to open folder window for: %s\n", path);
#endif
            /* Non-fatal error - continue processing even if window open fails */
        }
    }
    
    /* Print logging performance statistics */
    print_log_performance_stats();
    
    /* Cleanup */
    FreeIconArray(iconArray);
    UnLock(lock);
    
    return success;
}

/*========================================================================*/
/* Recursive Directory Processing                                        */
/*========================================================================*/

static BOOL ProcessDirectoryRecursive(const char *path, 
                                     const LayoutPreferences *prefs, 
                                     int recursion_level,
                                     FolderWindowTracker *windowTracker)
{
    BPTR lock = 0;
    struct FileInfoBlock *fib = NULL;
    char subdir[512];
    BOOL success = FALSE;
    
#ifdef DEBUG
    append_to_log("ProcessDirectoryRecursive: path='%s', level=%d\n", 
                  path, recursion_level);
#endif
    
    /* Check for user cancellation */
    CHECK_CANCEL();
    
    /* Safety check for recursion depth */
    if (recursion_level > 50)
    {
        PROGRESS_STATUS("Warning: Maximum recursion depth reached at: %s", path);
        return FALSE;
    }
    
    /* Process this directory first */
    if (!ProcessSingleDirectory(path, prefs, windowTracker))
    {
        return FALSE; /* Stop if current directory fails */
    }
    
    /*
     * FILESYSTEM LOCK RELEASE DELAY
     * ------------------------------
     * After processing the current directory, we add a small delay before
     * locking it again for subdirectory enumeration. This gives the AmigaOS
     * filesystem time to release internal lock table entries.
     * 
     * ProcessSingleDirectory() above has just called Lock() on this same path
     * and then UnLock(). We're about to Lock() it again below for subdirectory
     * scanning. On emulated systems running at maximum speed, these operations
     * occur so fast that DOS doesn't have time to clean up internal structures.
     * 
     * The delay is imperceptible to users (20ms) but critical for stability
     * on fast systems. See configuration section at top of file for details.
     */
#if ENABLE_FILESYSTEM_LOCK_DELAY
    Delay(FILESYSTEM_LOCK_DELAY_TICKS);
#endif
    
    /* Now process subdirectories */
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
                /* Check if path ends with ':' (root device) - if so, don't add '/' */
                if (path[strlen(path) - 1] == ':') {
                    snprintf(subdir, sizeof(subdir), "%s%s", path, fib->fib_FileName);
                } else {
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
#ifdef DEBUG
                        append_to_log("Skipping hidden folder (no .info): %s\n", subdir);
#endif
                        /* Only log to console, don't show in progress window */
                        CONSOLE_STATUS("Skipping hidden folder: %s\n", fib->fib_FileName);
                        continue;
                    }
                    UnLock(iconLock);
                }
                
                /* Recursively process subdirectory */
                PROGRESS_STATUS("");
                {
                    char shortened_path[256];
                    iTidy_ShortenPathWithParentDir(subdir, shortened_path, 60);
                    PROGRESS_STATUS("Entering: %s", shortened_path);
                }
                ProcessDirectoryRecursive(subdir, prefs, recursion_level + 1, windowTracker);
            }
        }
        success = TRUE;
    }
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    return success;
}

/*========================================================================*/
/* Helper Functions for Sorting                                          */
/*========================================================================*/

/**
 * @brief Get file extension from filename
 * @param filename The filename to extract extension from
 * @return Pointer to extension (including dot) or empty string if none
 */
static const char* GetFileExtension(const char *filename)
{
    const char *dot;
    
    if (!filename)
        return "";
    
    dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    
    return dot;
}

/**
 * @brief Compare two DateStamp structures
 * @param date1 First date
 * @param date2 Second date
 * @return <0 if date1 < date2, 0 if equal, >0 if date1 > date2
 */
static int CompareDateStamps(const struct DateStamp *date1, const struct DateStamp *date2)
{
    /* Compare days first (most significant) */
    if (date1->ds_Days != date2->ds_Days)
        return (date1->ds_Days > date2->ds_Days) ? 1 : -1;
    
    /* Days are equal, compare minutes */
    if (date1->ds_Minute != date2->ds_Minute)
        return (date1->ds_Minute > date2->ds_Minute) ? 1 : -1;
    
    /* Minutes are equal, compare ticks (1/50th second) */
    if (date1->ds_Tick != date2->ds_Tick)
        return (date1->ds_Tick > date2->ds_Tick) ? 1 : -1;
    
    /* Dates are identical */
    return 0;
}

/*========================================================================*/
/* Icon Sorting with Preferences                                         */
/*========================================================================*/

/* Global preference pointer for qsort comparison */
static const LayoutPreferences *g_sort_prefs = NULL;

/**
 * @brief Wrapper comparison function for qsort
 */
static int CompareIconsWrapper(const void *a, const void *b)
{
    return CompareIconsWithPreferences(a, b, g_sort_prefs);
}

static void SortIconArrayWithPreferences(IconArray *iconArray, 
                                        const LayoutPreferences *prefs)
{
    int i;
    
    if (!iconArray || iconArray->size <= 1 || !prefs)
    {
        return;
    }
    
#ifdef DEBUG
    append_to_log("Sorting %d icons with preferences\n", iconArray->size);
    append_to_log("  Sort by: %d, Priority: %d, Reverse: %d\n",
                  prefs->sortBy, prefs->sortPriority, prefs->reverseSort);
#endif
    
    /* Set global preference pointer for qsort */
    g_sort_prefs = prefs;
    
    /* Sort using preference-aware comparison */
    qsort(iconArray->array, iconArray->size, sizeof(FullIconDetails), 
          CompareIconsWrapper);
    
    /* Clear global pointer */
    g_sort_prefs = NULL;

#ifdef DEBUG
    append_to_log("After sorting, icon order:\n");
    append_to_log("%-3s | %-30s | %-6s\n", "ID", "Icon", "IsDir");
    append_to_log("--------------------------------------------\n");
    for (i = 0; i < iconArray->size; i++)
    {
        append_to_log("%-3d | %-30s | %-6s\n", 
                      i, 
                      iconArray->array[i].icon_text,
                      iconArray->array[i].is_folder ? "Folder" : "File");
    }
#endif
}

/*========================================================================*/
/* Preference-Aware Icon Comparison                                      */
/*========================================================================*/

static int CompareIconsWithPreferences(const void *a, const void *b, 
                                      const LayoutPreferences *prefs)
{
    const FullIconDetails *iconA = (const FullIconDetails *)a;
    const FullIconDetails *iconB = (const FullIconDetails *)b;
    int result = 0;
    const char *extA, *extB;
    size_t lenA, lenB, maxLen;
    
    if (!prefs)
        return 0;
    
    /* Apply folder/file priority first (unless MIXED) */
    if (prefs->sortPriority == SORT_PRIORITY_FOLDERS_FIRST)
    {
        if (iconA->is_folder && !iconB->is_folder) return -1;
        if (!iconA->is_folder && iconB->is_folder) return 1;
    }
    else if (prefs->sortPriority == SORT_PRIORITY_FILES_FIRST)
    {
        if (iconA->is_folder && !iconB->is_folder) return 1;
        if (!iconA->is_folder && iconB->is_folder) return -1;
    }
    /* SORT_PRIORITY_MIXED: No folder/file separation */
    
    /* Apply primary sort criteria */
    switch (prefs->sortBy)
    {
        case SORT_BY_NAME:
            /* Sort alphabetically by name */
            lenA = strlen(iconA->icon_text);
            lenB = strlen(iconB->icon_text);
            maxLen = (lenA > lenB) ? lenA : lenB;
            result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
            break;
            
        case SORT_BY_TYPE:
            /* Sort by file extension (type) */
            extA = GetFileExtension(iconA->icon_text);
            extB = GetFileExtension(iconB->icon_text);
            
            /* Compare extensions */
            lenA = strlen(extA);
            lenB = strlen(extB);
            maxLen = (lenA > lenB) ? lenA : lenB;
            
            if (lenA == 0 && lenB == 0)
            {
                /* Both have no extension, sort by name */
                lenA = strlen(iconA->icon_text);
                lenB = strlen(iconB->icon_text);
                maxLen = (lenA > lenB) ? lenA : lenB;
                result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
            }
            else if (lenA == 0)
            {
                /* A has no extension, put it first */
                result = -1;
            }
            else if (lenB == 0)
            {
                /* B has no extension, put it first */
                result = 1;
            }
            else
            {
                /* Compare extensions */
                result = strncasecmp_custom(extA, extB, maxLen);
                
                /* If extensions are the same, sort by name */
                if (result == 0)
                {
                    lenA = strlen(iconA->icon_text);
                    lenB = strlen(iconB->icon_text);
                    maxLen = (lenA > lenB) ? lenA : lenB;
                    result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
                }
            }
            break;
            
        case SORT_BY_DATE:
            /* Sort by modification date (newest first) */
            result = CompareDateStamps(&iconB->file_date, &iconA->file_date);
            
            /* If dates are the same, sort by name */
            if (result == 0)
            {
                lenA = strlen(iconA->icon_text);
                lenB = strlen(iconB->icon_text);
                maxLen = (lenA > lenB) ? lenA : lenB;
                result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
            }
            break;
            
        case SORT_BY_SIZE:
            /* Sort by file size (largest first for files, folders always 0) */
            if (iconA->file_size > iconB->file_size)
            {
                result = -1;  /* A is larger, comes first */
            }
            else if (iconA->file_size < iconB->file_size)
            {
                result = 1;   /* B is larger, comes first */
            }
            else
            {
                /* Same size, sort by name */
                lenA = strlen(iconA->icon_text);
                lenB = strlen(iconB->icon_text);
                maxLen = (lenA > lenB) ? lenA : lenB;
                result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
            }
            break;
            
        default:
            /* Unknown sort mode, default to name */
            lenA = strlen(iconA->icon_text);
            lenB = strlen(iconB->icon_text);
            maxLen = (lenA > lenB) ? lenA : lenB;
            result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
            break;
    }
    
    /* Apply reverse sort if requested */
    if (prefs->reverseSort)
    {
        result = -result;
    }
    
    return result;
}
