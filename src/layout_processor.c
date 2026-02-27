/**
 * layout_processor.c - Icon Processing with Layout Preferences
 * 
 * Implements preference-aware directory processing that integrates
 * with the existing iTidy icon management system.
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

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
#include "layout/aspect_ratio_layout.h"
#include "layout/folder_scanner.h"
#include "layout/icon_sorter.h"
#include "layout/icon_positioner.h"
#include "layout/block_layout.h"
#include "layout/tool_scanner.h"

/* Backup system integration */
#include "backups/backup_session.h"
#include "backups/backup_catalog.h"
#include "path_utilities.h"

/* DefIcons icon creation system */
#include "deficons/deficons_identify.h"
#include "deficons/deficons_templates.h"
#include "deficons/deficons_filters.h"
#include "deficons/deficons_creation.h"

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

/* Getter for progress window - allows icon_management.c and file_directory_handling.c
 * to update the heartbeat status without needing access to the static variable */
struct iTidyMainProgressWindow *GetCurrentProgressWindow(void)
{
    return g_progressWindow;
}

/* Setter for progress window - allows sub-modules (tool_scanner.c) to update the
 * active progress window stored here. Pass NULL to clear. */
void SetCurrentProgressWindow(struct iTidyMainProgressWindow *pw)
{
    g_progressWindow = pw;
}

/* Global statistics tracking */
static ULONG g_foldersProcessed = 0;
static ULONG g_iconsProcessed = 0;
static struct DateStamp g_startTime;
static struct DateStamp g_endTime;

/* DefIcons icon creation statistics tracking now in deficons_creation.c */

/* Helper macro for triple output (console + progress window + log file) */
#define PROGRESS_STATUS(...) \
    do { \
        char _buf[256]; \
        snprintf(_buf, sizeof(_buf), __VA_ARGS__); \
        CONSOLE_STATUS("%s\n", _buf); \
        log_info(LOG_GENERAL, "%s\n", _buf); \
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

/* Helper macro to pump events and check for cancel during long operations */
#define PUMP_AND_CHECK_CANCEL() \
    do { \
        if (g_progressWindow) { \
            itidy_main_progress_window_handle_events(g_progressWindow); \
            if (g_progressWindow->cancel_requested) { \
                CONSOLE_STATUS("\n*** User cancelled operation ***\n"); \
                return FALSE; \
            } \
        } \
    } while (0)

/* Icon creation stats functions now in deficons_creation.c */

/* Icon creation log functions now in deficons_creation.c */

/* Forward declarations for helper functions */
static BOOL ProcessSingleDirectory(const char *path, 
                                   const LayoutPreferences *prefs);
static BOOL ProcessDirectoryRecursive(const char *path, 
                                     const LayoutPreferences *prefs, 
                                     int recursion_level);

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
    deficons_creation_init_stats();
    
    /* Get start time */
    DateStamp(&g_startTime);
    
    /* Apply logging preferences before processing
     * Note: prefs->logLevel is set from tooltype at startup in main_gui.c
     * or from the Log mode menu selection, ensuring user choice is preserved */
    set_global_log_level((LogLevel)prefs->logLevel);
    /* Note: performance logging is controlled by PERFLOG tooltype - set at startup, not overridden here */
    
    log_info(LOG_GENERAL, "Logging preferences applied:\n");
    log_info(LOG_GENERAL, "  Log Level: %s\n",
             prefs->logLevel == 0 ? "DEBUG" :
             prefs->logLevel == 1 ? "INFO" :
             prefs->logLevel == 2 ? "WARNING" : "ERROR");
    log_info(LOG_GENERAL, "  Performance Logging: %s (PERFLOG tooltype)\n", 
             is_performance_logging_enabled() ? "ENABLED" : "DISABLED");
    
    /* Copy and sanitize the path from preferences */
    strncpy(sanitizedPath, prefs->folder_path, sizeof(sanitizedPath) - 1);
    sanitizedPath[sizeof(sanitizedPath) - 1] = '\0';
    sanitizeAmigaPath(sanitizedPath);
    

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
                log_debug(LOG_GENERAL, "Tool cache initialized\n");
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
    
    /* ===================================================================== */
    /* DUMP CURRENT PREFERENCES FOR DIAGNOSTICS                             */
    /* ===================================================================== */
    CONSOLE_STATUS("=== Current Preferences Settings ===");
    log_info(LOG_GENERAL, "\n*** Current Preferences Settings ***\n");
    log_info(LOG_GENERAL, "Target Path: %s\n", sanitizedPath);
    CONSOLE_STATUS("Target Path: %s", sanitizedPath);
    log_info(LOG_GENERAL, "Recursive Mode: %s\n", prefs->recursive_subdirs ? "YES" : "NO");
    CONSOLE_STATUS("Recursive Mode: %s", prefs->recursive_subdirs ? "YES" : "NO");
    log_info(LOG_GENERAL, "Skip Hidden Folders: %s\n", prefs->skipHiddenFolders ? "YES" : "NO");
    log_info(LOG_GENERAL, "Sort By: %d (%s)\n", prefs->sortBy, 
             prefs->sortBy == 0 ? "Name" : 
             prefs->sortBy == 1 ? "Type" : 
             prefs->sortBy == 2 ? "Date" : 
             prefs->sortBy == 3 ? "Size" : "Unknown");
    CONSOLE_STATUS("Sort By: %d", prefs->sortBy);
    log_info(LOG_GENERAL, "Reverse Sort: %s\n", prefs->reverseSort ? "YES" : "NO");
    log_info(LOG_GENERAL, "Backup Enabled: %s\n", prefs->backupPrefs.enableUndoBackup ? "YES" : "NO");
    CONSOLE_STATUS("Backup Enabled: %s", prefs->backupPrefs.enableUndoBackup ? "YES" : "NO");
    log_info(LOG_GENERAL, "Default Tool Validation: %s\n", prefs->validate_default_tools ? "YES" : "NO");
    log_info(LOG_GENERAL, "\n--- DefIcons Icon Creation Settings ---\n");
    CONSOLE_STATUS("--- DefIcons Settings ---");
    log_info(LOG_GENERAL, "DefIcons icon creation: %s\n", prefs->enable_deficons_icon_creation ? "YES" : "NO");
    CONSOLE_STATUS("DefIcons icon creation: %s", prefs->enable_deficons_icon_creation ? "YES" : "NO");
    log_info(LOG_GENERAL, "Disabled Types: '%s'\n", prefs->deficons_disabled_types);
    log_info(LOG_GENERAL, "Folder Icon Mode: %u (%s)\n", prefs->deficons_folder_icon_mode,
             prefs->deficons_folder_icon_mode == 0 ? "Smart" :
             prefs->deficons_folder_icon_mode == 1 ? "Always" :
             prefs->deficons_folder_icon_mode == 2 ? "Never" : "Unknown");
    log_info(LOG_GENERAL, "Skip System Assigns: %s\n", prefs->deficons_skip_system_assigns ? "YES" : "NO");
    log_info(LOG_GENERAL, "Exclude Paths Count: %u\n", GetGlobalExcludePaths()->count);
    if (GetGlobalExcludePaths()->count > 0)
    {
        log_info(LOG_GENERAL, "Exclude Paths List:\n");
        for (UWORD i = 0; i < GetGlobalExcludePaths()->count; i++)
        {
            log_debug(LOG_GENERAL, "  [%u] %s\n", i, GetGlobalExcludePaths()->paths[i]);
        }
    }
    log_info(LOG_GENERAL, "========================================\n\n");
    CONSOLE_STATUS("===================================");
    
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
            log_error(LOG_GENERAL, "ERROR: Failed to start backup session\n");
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
    
    /* Phase: Icon Creation (DefIcons integration) */
    /* This runs BEFORE icon tidying to ensure all files have icons */
    if (prefs->enable_deficons_icon_creation)
    {
        iTidy_DefIconsCreationResult creation_result;
        creation_result.icons_created = 0;
        creation_result.has_visible_contents = FALSE;
        
        log_info(LOG_ICONS, "\n*** Starting DefIcons Icon Creation Phase ***\n");
        PROGRESS_STATUS("Creating missing icons using DefIcons...");
        
        /* Initialize DefIcons modules */
        if (!deficons_initialize_arexx())
        {
            log_error(LOG_ICONS, "Failed to initialize DefIcons ARexx - icon creation disabled\n");
            PROGRESS_STATUS("Warning: DefIcons not available, skipping icon creation");
        }
        else if (!deficons_initialize_templates())
        {
            log_error(LOG_ICONS, "Failed to initialize DefIcons templates - icon creation disabled\n");
            PROGRESS_STATUS("Warning: DefIcons templates not loaded, skipping icon creation");
            deficons_cleanup_arexx();
        }
        else
        {
            /* Open icon creation log if enabled */
            if (prefs->deficons_log_created_icons)
            {
                deficons_creation_open_log();
            }
            
            /* Open created icons manifest in backup run (for restore to delete these) */
            if (g_backupContext != NULL && g_backupContext->sessionActive)
            {
                if (!OpenCreatedIconsManifest(g_backupContext))
                {
                    log_warning(LOG_BACKUP, "Failed to open created icons manifest - restore may leave extra icons\n");
                }
            }
            
            /* Create icons recursively or single folder */
            if (prefs->recursive_subdirs)
            {
                if (deficons_create_missing_icons_recursive(sanitizedPath, prefs, 0,
                        &creation_result, g_progressWindow, g_backupContext))
                {
                    log_info(LOG_ICONS, "Icon creation complete: %lu icon(s) created\n", creation_result.icons_created);
                    PROGRESS_STATUS("Created %lu icon(s) recursively", creation_result.icons_created);
                }
                else
                {
                    log_warning(LOG_ICONS, "Icon creation incomplete (errors occurred)\n");
                    PROGRESS_STATUS("Warning: Icon creation had errors (%lu created)", creation_result.icons_created);
                }
            }
            else
            {
                if (deficons_create_missing_icons_in_directory(sanitizedPath, prefs,
                        &creation_result, g_progressWindow, g_backupContext, FALSE))
                {
                    log_info(LOG_ICONS, "Icon creation complete: %lu icon(s) created\n", creation_result.icons_created);
                    PROGRESS_STATUS("Created %lu icon(s) in current folder", creation_result.icons_created);
                }
                else
                {
                    log_warning(LOG_ICONS, "Icon creation incomplete (errors occurred)\n");
                    PROGRESS_STATUS("Warning: Icon creation had errors (%lu created)", creation_result.icons_created);
                }
            }
            
            /* Clear the heartbeat display after icon creation finishes */
            if (g_progressWindow)
            {
                itidy_main_progress_clear_heartbeat(g_progressWindow);
            }
            
            /* Cleanup DefIcons modules (templates first, then ARexx) */
            deficons_cleanup_templates();
            deficons_cleanup_arexx();
        }
        
        /* Update progress heartbeat after icon creation */
        if (g_progressWindow && creation_result.icons_created > 0)
        {
            itidy_main_progress_update_heartbeat(g_progressWindow, "Icons created", creation_result.icons_created, creation_result.icons_created);
        }
    }
    
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
        success = ProcessDirectoryRecursive(sanitizedPath, prefs, 0);
    }
    else
    {
        success = ProcessSingleDirectory(sanitizedPath, prefs);
    }
    
    /* Clear heartbeat status now that processing is complete */
    if (g_progressWindow != NULL)
    {
        itidy_main_progress_clear_heartbeat(g_progressWindow);
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
        
        /* Display icon creation statistics if any were created */
        {
            ULONG total_created = deficons_creation_get_total();
            if (total_created > 0)
            {
                PROGRESS_STATUS("  Icons created: %lu", total_created);
                deficons_creation_display_stats();
            }
        }
        
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
        log_debug(LOG_GENERAL, "\n*** Tool cache summary ***\n");
        DumpToolCache();  /* Show all cached tools after processing */
        
        /* Note: Tool cache is NOT freed here - it remains available for post-processing */
        log_debug(LOG_GENERAL, "Tool cache retained for post-processing\n");
        
        log_debug(LOG_GENERAL, "Freeing PATH search list\n");
        FreePathSearchList();
        g_ValidateDefaultTools = FALSE;
    }
    
    /* Close icon creation log if open */
    if (prefs->deficons_log_created_icons)
    {
        deficons_creation_close_log();
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
        if (g_backupContext->iconsCreated > 0)
        {
            PROGRESS_STATUS("  Icons created (DefIcons): %lu", g_backupContext->iconsCreated);
        }
        PROGRESS_STATUS("  Location: %s", g_backupContext->runDirectory);

        log_info(LOG_GENERAL, "\n*** Backup session completed ***\n");
        append_to_log("  Folders backed up: %u\n", g_backupContext->foldersBackedUp);
        append_to_log("  Failed backups: %u\n", g_backupContext->failedBackups);
        append_to_log("  Total bytes: %lu\n", (unsigned long)g_backupContext->totalBytesArchived);
        if (g_backupContext->iconsCreated > 0)
        {
            append_to_log("  Icons created (DefIcons): %lu\n", g_backupContext->iconsCreated);
        }
        append_to_log("  Location: %s\n", g_backupContext->runDirectory);
        
        g_backupContext = NULL;
    }
    
    return success;
}

/*========================================================================*/
/* Single Directory Processing                                           */
/*========================================================================*/

static BOOL ProcessSingleDirectory(const char *path, 
                                   const LayoutPreferences *prefs)
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
            resizeFolderToContents((char *)path, iconArray, prefs);
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
    
    /* Pump events before sort to register any pending Cancel clicks */
    PUMP_AND_CHECK_CANCEL();
    
    SortIconArrayWithPreferences(iconArray, prefs);
    
    /* Pump events after sort to allow cancellation */
    PUMP_AND_CHECK_CANCEL();
    
    /* Calculate and apply new positions based on sorted order */
#ifdef DEBUG
    append_to_log("==== SECTION: CALCULATING LAYOUT ====\n");
#endif
    
    /* Check if block grouping mode is enabled */
    if (prefs->blockGroupMode == BLOCK_GROUP_BY_TYPE)
    {
        /* Use block-based layout - group by Workbench icon type */
        int contentWidth = 0;
        int contentHeight = 0;
        
        log_info(LOG_ICONS, "Using block layout (group by icon type)\n");
        
        if (!CalculateBlockLayout(iconArray, prefs, &contentWidth, &contentHeight))
        {
            log_error(LOG_ICONS, "Block layout calculation failed - falling back to standard layout\n");
            PROGRESS_STATUS("  Warning: Block layout failed, using standard layout");
            goto standard_layout;
        }
        
        log_debug(LOG_ICONS, "Block layout calculated: %dx%d\n", contentWidth, contentHeight);
    }
    else
    {
standard_layout:
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
            PROGRESS_STATUS("  Layout: %d cols x %d rows", finalColumns, finalRows);
            
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
    }
    
    /* Pump events after layout calculation to allow cancellation */
    PUMP_AND_CHECK_CANCEL();
    
    /* Resize window if requested */
    if (prefs->resizeWindows)
    {
        PROGRESS_STATUS("  Resizing window...");
        resizeFolderToContents((char *)path, iconArray, prefs);
    }
    
    PUMP_AND_CHECK_CANCEL();
    
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
                                     int recursion_level)
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
    if (!ProcessSingleDirectory(path, prefs))
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
            /* Check for cancel between subdirectories */
            PUMP_AND_CHECK_CANCEL();

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
                ProcessDirectoryRecursive(subdir, prefs, recursion_level + 1);
            }
        }
        success = TRUE;
    }
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    return success;
}

