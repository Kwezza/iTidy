/*
 * restore_window.c - iTidy Restore Window Implementation (ReAction)
 * ReAction GUI for Workbench 3.2+
 * 
 * Migrated from GadTools to ReAction following REACTION_MIGRATION_GUIDE.md
 * Uses ListBrowser gadgets for run list and details display.
 */

/* =========================================================================
 * LIBRARY BASE ISOLATION
 * Redefine library bases to local unique names BEFORE including proto headers.
 * This prevents linker collisions with bases defined in main_window.c
 * ========================================================================= */
#define WindowBase      iTidy_Restore_WindowBase
#define LayoutBase      iTidy_Restore_LayoutBase
#define ButtonBase      iTidy_Restore_ButtonBase
#define CheckBoxBase    iTidy_Restore_CheckBoxBase
#define ListBrowserBase iTidy_Restore_ListBrowserBase
#define LabelBase       iTidy_Restore_LabelBase

#include "platform/platform.h"
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/dos.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>
#include <stdio.h>

/* ReAction headers */
#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/checkbox.h>
#include <proto/listbrowser.h>
#include <proto/label.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/checkbox.h>
#include <gadgets/listbrowser.h>
#include <images/label.h>

#include "restore_window.h"
#include "GUI/StatusWindows/progress_window.h"
#include "backup_restore.h"
#include "writeLog.h"
#include "folder_view_window.h"
#include "GUI/gui_utilities.h"
#include "backup_runs.h"
#include "backup_catalog.h"
#include "string_functions.h"
#include "../easy_request_helper.h"

#ifndef NewList
VOID NewList(struct List *list);
#endif

/*------------------------------------------------------------------------*/
/* Library Bases (Prefixed to avoid collision with main_window.c)        */
/*------------------------------------------------------------------------*/
struct Library *iTidy_Restore_WindowBase = NULL;
struct Library *iTidy_Restore_LayoutBase = NULL;
struct Library *iTidy_Restore_ButtonBase = NULL;
struct Library *iTidy_Restore_CheckBoxBase = NULL;
struct Library *iTidy_Restore_ListBrowserBase = NULL;
struct Library *iTidy_Restore_LabelBase = NULL;

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define RESTORE_WINDOW_TITLE "iTidy - Restore Backups"

/*------------------------------------------------------------------------*/
/* Gadget IDs (ReAction uses these via GA_ID)                            */
/*------------------------------------------------------------------------*/
enum {
    GID_RESTORE_ROOT_LAYOUT = 1,
    GID_RESTORE_RUN_LIST_LAYOUT,
    GID_RESTORE_RUN_LISTBROWSER,
    GID_RESTORE_DETAILS_LAYOUT,
    GID_RESTORE_DETAILS_LISTBROWSER,
    GID_RESTORE_BUTTONS_LAYOUT,
    GID_RESTORE_DELETE_BUTTON,
    GID_RESTORE_RESTORE_BUTTON,
    GID_RESTORE_VIEW_BUTTON,
    GID_RESTORE_CANCEL_BUTTON,
    GID_RESTORE_GEOM_CHECKBOX
};

/*------------------------------------------------------------------------*/
/* Column Configuration for Run ListBrowser                              */
/*------------------------------------------------------------------------*/
static struct ColumnInfo run_list_column_info[] = {
    { 8,  "Run",       0 },
    { 20, "Date/Time", 0 },
    { 10, "Folders",   0 },
    { 10, "Size",      0 },
    { 12, "Status",    0 },
    { -1, (STRPTR)~0, -1 }
};

static struct ColumnInfo details_column_info[] = {
    { 100, "", 0 },
    { -1, (STRPTR)~0, -1 }
};

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static BOOL open_reaction_classes(void);
static void close_reaction_classes(void);
static void free_listbrowser_list(struct List *list);
static BOOL delete_directory_recursive(const char *path);

/*------------------------------------------------------------------------*/
/**
 * @brief Format a size in bytes to human-readable string
 */
/*------------------------------------------------------------------------*/
void format_size_string(ULONG bytes, char *buffer)
{
    if (bytes >= 1048576)
    {
        ULONG mb = bytes / 1048576;
        ULONG decimal = ((bytes % 1048576) * 10) / 1048576;
        sprintf(buffer, "%lu.%lu MB", mb, decimal);
    }
    else if (bytes >= 1024)
    {
        ULONG kb = bytes / 1024;
        ULONG decimal = ((bytes % 1024) * 10) / 1024;
        sprintf(buffer, "%lu.%lu KB", kb, decimal);
    }
    else
    {
        sprintf(buffer, "%lu B", bytes);
    }
}

/*------------------------------------------------------------------------*/
/**
 * @brief Format DateStamp to human-readable string
 */
/*------------------------------------------------------------------------*/
static void format_datestamp_string(struct DateStamp *ds, char *buffer, int bufferSize)
{
    ULONG days, minutes, seconds;
    ULONG year, month, day, hour, minute;
    ULONG daysSinceEpoch;
    
    if (ds == NULL || buffer == NULL || bufferSize < 20)
    {
        if (buffer != NULL && bufferSize > 0)
            strcpy(buffer, "Unknown");
        return;
    }
    
    days = ds->ds_Days;
    minutes = ds->ds_Minute;
    seconds = ds->ds_Tick / 50;
    
    hour = minutes / 60;
    minute = minutes % 60;
    
    daysSinceEpoch = days;
    year = 1978;
    
    while (1)
    {
        ULONG daysInYear = 365;
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
            daysInYear = 366;
        
        if (daysSinceEpoch >= daysInYear)
        {
            daysSinceEpoch -= daysInYear;
            year++;
        }
        else
            break;
    }
    
    {
        ULONG daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        BOOL isLeap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
        
        if (isLeap)
            daysInMonth[1] = 29;
        
        month = 1;
        while (month <= 12 && daysSinceEpoch >= daysInMonth[month - 1])
        {
            daysSinceEpoch -= daysInMonth[month - 1];
            month++;
        }
        
        day = daysSinceEpoch + 1;
    }
    
    snprintf(buffer, bufferSize, "%04lu-%02lu-%02lu %02lu:%02lu:%02lu",
             year, month, day, hour, minute, seconds);
}

/*------------------------------------------------------------------------*/
/* Helper structure for catalog statistics                               */
/*------------------------------------------------------------------------*/
struct CatalogStatsContext
{
    ULONG count;
    ULONG totalBytes;
};

/*------------------------------------------------------------------------*/
/* Callback to accumulate catalog statistics                             */
/*------------------------------------------------------------------------*/
static BOOL catalog_stats_callback(const BackupArchiveEntry *entry, void *userData)
{
    struct CatalogStatsContext *stats = (struct CatalogStatsContext *)userData;
    
    if (stats != NULL && entry != NULL)
    {
        stats->count++;
        stats->totalBytes += entry->sizeBytes;
    }
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Extract source directory from catalog header
 */
/*------------------------------------------------------------------------*/
static BOOL extract_source_directory(const char *catalog_path, char *buffer, ULONG buffer_size)
{
    BPTR file;
    char line[512];
    BOOL found = FALSE;
    
    if (!catalog_path || !buffer || buffer_size == 0)
        return FALSE;
    
    buffer[0] = '\0';
    
    file = Open((STRPTR)catalog_path, MODE_OLDFILE);
    if (!file)
        return FALSE;
    
    while (FGets(file, line, sizeof(line)))
    {
        ULONG len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';
        
        if (strncmp(line, "Source Directory: ", 18) == 0)
        {
            const char *path = line + 18;
            strncpy(buffer, path, buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
            found = TRUE;
            break;
        }
        
        if (strstr(line, "----") != NULL)
            break;
    }
    
    Close(file);
    return found;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Scan backup directory for available runs
 */
/*------------------------------------------------------------------------*/
ULONG scan_backup_runs(const char *backup_root, 
                       struct RestoreRunEntry **entries_out)
{
    UWORD highest_run;
    ULONG count = 0;
    ULONG i;
    struct RestoreRunEntry *entries = NULL;
    char run_path[256];
    char catalog_path[300];
    BPTR lock;
    
    if (backup_root == NULL || entries_out == NULL)
        return 0;
    
    highest_run = FindHighestRunNumber(backup_root);
    if (highest_run == 0)
    {
        log_debug(LOG_BACKUP, "scan_backup_runs: No backup runs found in %s\n", backup_root);
        *entries_out = NULL;
        return 0;
    }
    
    log_debug(LOG_BACKUP, "scan_backup_runs: Highest run number = %u\n", highest_run);
    
    entries = (struct RestoreRunEntry *)whd_malloc(
        sizeof(struct RestoreRunEntry) * highest_run);
    
    if (entries == NULL)
    {
        log_error(LOG_BACKUP, "scan_backup_runs: Failed to allocate memory for entries\n");
        return 0;
    }
    memset(entries, 0, sizeof(struct RestoreRunEntry) * highest_run);
    
    for (i = 1; i <= highest_run; i++)
    {
        struct FileInfoBlock *fib;
        struct DateStamp runDate;
        BOOL hasDate = FALSE;
        
        if (!GetRunDirectoryPath(run_path, backup_root, (UWORD)i))
            continue;
        
        lock = Lock(run_path, ACCESS_READ);
        if (lock == 0)
            continue;
        
        fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
        if (fib != NULL)
        {
            if (Examine(lock, fib))
            {
                runDate = fib->fib_Date;
                hasDate = TRUE;
            }
            FreeDosObject(DOS_FIB, fib);
        }
        
        UnLock(lock);
        
        struct RestoreRunEntry *entry = &entries[count];
        entry->runNumber = (UWORD)i;
        FormatRunDirectoryName(entry->runName, (UWORD)i);
        strcpy(entry->fullPath, run_path);
        
        if (hasDate)
        {
            format_datestamp_string(&runDate, entry->dateStr, sizeof(entry->dateStr));
        }
        else
        {
            strcpy(entry->dateStr, "Unknown");
        }
        
        sprintf(catalog_path, "%s/catalog.txt", run_path);
        lock = Lock(catalog_path, ACCESS_READ);
        entry->hasCatalog = (lock != 0);
        if (lock != 0)
            UnLock(lock);
        
        if (entry->hasCatalog)
        {
            struct CatalogStatsContext stats;
            
            stats.count = 0;
            stats.totalBytes = 0;
            ParseCatalog(catalog_path, catalog_stats_callback, &stats);
            
            entry->folderCount = stats.count;
            entry->totalBytes = stats.totalBytes;
            format_size_string(stats.totalBytes, entry->sizeStr);
            entry->statusCode = RESTORE_STATUS_COMPLETE;
            strcpy(entry->statusStr, "Complete");
            
            if (!extract_source_directory(catalog_path, entry->sourceDirectory, 
                                         sizeof(entry->sourceDirectory)))
            {
                strcpy(entry->sourceDirectory, "(Unknown)");
            }
        }
        else
        {
            entry->folderCount = 0;
            entry->totalBytes = 0;
            strcpy(entry->sizeStr, "N/A");
            entry->statusCode = RESTORE_STATUS_ORPHANED;
            strcpy(entry->statusStr, "NoCAT");
            strcpy(entry->sourceDirectory, "(No catalog)");
        }
        
        count++;
    }
    
    log_debug(LOG_BACKUP, "scan_backup_runs: Found %lu valid run directories\n", count);
    
    *entries_out = entries;
    return count;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Populate run list ListBrowser with backup entries
 */
/*------------------------------------------------------------------------*/
void populate_run_list(struct iTidyRestoreWindow *restore_data,
                       struct RestoreRunEntry *entries,
                       ULONG count,
                       int nav_direction)
{
    ULONG i;
    struct Node *node;
    char run_num_str[16];
    char folder_str[16];
    char short_date[32];
    
    if (restore_data == NULL || restore_data->run_listbrowser_obj == NULL)
        return;
    
    /* Detach old list from gadget */
    SetGadgetAttrs((struct Gadget *)restore_data->run_listbrowser_obj, 
                   restore_data->window, NULL,
                   LISTBROWSER_Labels, ~0,
                   TAG_DONE);
    
    /* Free existing list */
    if (restore_data->run_list_nodes != NULL)
    {
        free_listbrowser_list(restore_data->run_list_nodes);
        restore_data->run_list_nodes = NULL;
    }
    
    /* Create new list */
    restore_data->run_list_nodes = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
    if (restore_data->run_list_nodes == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to allocate run_list_nodes\n");
        return;
    }
    NewList(restore_data->run_list_nodes);
    
    /* Add data rows */
    for (i = 0; i < count; i++)
    {
        sprintf(run_num_str, "%u", entries[i].runNumber);
        sprintf(folder_str, "%lu", entries[i].folderCount);
        
        /* Format date using iTidy_FormatTimestamp() */
        char sort_key_date[20];
        if (strlen(entries[i].dateStr) >= 19)
        {
            sprintf(sort_key_date, "%.4s%.2s%.2s_%.2s%.2s%.2s",
                    entries[i].dateStr,
                    entries[i].dateStr + 5,
                    entries[i].dateStr + 8,
                    entries[i].dateStr + 11,
                    entries[i].dateStr + 14,
                    entries[i].dateStr + 17);
        }
        else
        {
            strcpy(sort_key_date, "00000000_000000");
        }
        
        if (!iTidy_FormatTimestamp(sort_key_date, short_date, sizeof(short_date)))
        {
            if (strlen(entries[i].dateStr) >= 16)
            {
                strncpy(short_date, entries[i].dateStr, 16);
                short_date[16] = '\0';
            }
            else
            {
                strcpy(short_date, entries[i].dateStr);
            }
        }
        
        /* Create ListBrowser node with 5 columns */
        node = AllocListBrowserNode(5,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, run_num_str,
                LBNCA_Justification, LCJ_RIGHT,
            LBNA_Column, 1,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, short_date,
            LBNA_Column, 2,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, folder_str,
                LBNCA_Justification, LCJ_RIGHT,
            LBNA_Column, 3,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, entries[i].sizeStr,
                LBNCA_Justification, LCJ_RIGHT,
            LBNA_Column, 4,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, entries[i].statusStr,
            TAG_DONE);
        
        if (node)
        {
            AddTail(restore_data->run_list_nodes, node);
        }
    }
    
    /* Reattach list to gadget */
    SetGadgetAttrs((struct Gadget *)restore_data->run_listbrowser_obj, 
                   restore_data->window, NULL,
                   LISTBROWSER_Labels, restore_data->run_list_nodes,
                   LISTBROWSER_AutoFit, TRUE,
                   TAG_DONE);
    
    /* Enable buttons if we have data */
    if (count > 0)
    {
        /* Select first row */
        SetGadgetAttrs((struct Gadget *)restore_data->run_listbrowser_obj,
                       restore_data->window, NULL,
                       LISTBROWSER_Selected, 0,
                       TAG_DONE);
        
        restore_data->selected_run_index = 0;
        update_details_panel(restore_data, &entries[0]);
        
        /* Enable restore and delete buttons */
        SetGadgetAttrs((struct Gadget *)restore_data->restore_run_btn,
                       restore_data->window, NULL,
                       GA_Disabled, FALSE,
                       TAG_DONE);
        
        SetGadgetAttrs((struct Gadget *)restore_data->delete_run_btn,
                       restore_data->window, NULL,
                       GA_Disabled, FALSE,
                       TAG_DONE);
        
        /* Enable view folders if catalog exists */
        SetGadgetAttrs((struct Gadget *)restore_data->view_folders_btn,
                       restore_data->window, NULL,
                       GA_Disabled, !entries[0].hasCatalog,
                       TAG_DONE);
    }
}

/*------------------------------------------------------------------------*/
/**
 * @brief Update details panel with selected run information
 */
/*------------------------------------------------------------------------*/
void update_details_panel(struct iTidyRestoreWindow *restore_data,
                          struct RestoreRunEntry *selected_entry)
{
    char line_buffer[7][256];
    struct Node *node;
    
    if (restore_data == NULL || restore_data->details_listbrowser_obj == NULL)
        return;
    
    /* Detach list from gadget */
    SetGadgetAttrs((struct Gadget *)restore_data->details_listbrowser_obj, 
                   restore_data->window, NULL,
                   LISTBROWSER_Labels, ~0,
                   TAG_DONE);
    
    /* Free existing list */
    if (restore_data->details_list_nodes != NULL)
    {
        free_listbrowser_list(restore_data->details_list_nodes);
        restore_data->details_list_nodes = NULL;
    }
    
    /* Create new list */
    restore_data->details_list_nodes = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
    if (restore_data->details_list_nodes == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to allocate details_list_nodes\n");
        return;
    }
    NewList(restore_data->details_list_nodes);
    
    if (selected_entry == NULL)
    {
        strcpy(line_buffer[0], "(No run selected)");
        node = AllocListBrowserNode(1,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, line_buffer[0],
            TAG_DONE);
        if (node) AddTail(restore_data->details_list_nodes, node);
    }
    else
    {
        char status_desc[64];
        
        switch (selected_entry->statusCode)
        {
            case RESTORE_STATUS_COMPLETE:
                strcpy(status_desc, "Complete (catalog present)");
                break;
            case RESTORE_STATUS_INCOMPLETE:
                strcpy(status_desc, "Incomplete (missing archives)");
                break;
            case RESTORE_STATUS_ORPHANED:
                strcpy(status_desc, "Orphaned (no catalog)");
                break;
            case RESTORE_STATUS_CORRUPTED:
                strcpy(status_desc, "Corrupted (catalog error)");
                break;
            default:
                strcpy(status_desc, "Unknown");
                break;
        }
        
        /* Format each detail line */
        sprintf(line_buffer[0], "Run Number:       %04u", selected_entry->runNumber);
        sprintf(line_buffer[1], "Date Created:     %s", selected_entry->dateStr);
        sprintf(line_buffer[2], "Source Directory: %s", selected_entry->sourceDirectory);
        sprintf(line_buffer[3], "Total Archives:   %lu", selected_entry->folderCount);
        sprintf(line_buffer[4], "Total Size:       %s", selected_entry->sizeStr);
        sprintf(line_buffer[5], "Status:           %s", status_desc);
        sprintf(line_buffer[6], "Location:         %s", selected_entry->fullPath);
        
        /* Add each line as a node */
        for (int i = 0; i < 7; i++)
        {
            node = AllocListBrowserNode(1,
                LBNA_Column, 0,
                    LBNCA_CopyText, TRUE,
                    LBNCA_Text, line_buffer[i],
                TAG_DONE);
            if (node) AddTail(restore_data->details_list_nodes, node);
        }
    }
    
    /* Reattach list to gadget */
    SetGadgetAttrs((struct Gadget *)restore_data->details_listbrowser_obj, 
                   restore_data->window, NULL,
                   LISTBROWSER_Labels, restore_data->details_list_nodes,
                   TAG_DONE);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Perform full run restore with progress feedback
 */
/*------------------------------------------------------------------------*/
BOOL perform_restore_run(struct iTidyRestoreWindow *restore_data,
                        struct RestoreRunEntry *run_entry)
{
    RestoreStatus status;
    char message[256];
    
    if (restore_data == NULL || run_entry == NULL)
        return FALSE;
    
    if (restore_data->window == NULL)
    {
        log_error(LOG_GUI, "ERROR: Window is NULL in perform_restore_run\n");
        return FALSE;
    }
    
    sprintf(message, "Restore all folders from %s?", run_entry->runName);
    
    if (!ShowEasyRequest(restore_data->window, 
                         "Confirm Restore",
                         message,
                         "Restore|Cancel"))
    {
        log_info(LOG_BACKUP, "Restore cancelled by user\n");
        return FALSE;
    }
    
    log_info(LOG_BACKUP, "Starting restore of run %u from %s\n",
             run_entry->runNumber,
             restore_data->backup_root_path);
    
    RestoreContext restoreCtx;
    if (!InitRestoreContext(&restoreCtx))
    {
        log_error(LOG_BACKUP, "ERROR: Failed to initialize restore context - LHA not available\n");
        
        sprintf(message, "LHA executable not found!\nRestore requires LHA to be installed.");
        
        ShowEasyRequest(restore_data->window,
                       "Restore Failed",
                       message,
                       "OK");
        
        return FALSE;
    }
    
    restoreCtx.restoreWindowGeometry = restore_data->restore_window_geometry;
    log_info(LOG_BACKUP, "Window geometry restore: %s\n",
             restoreCtx.restoreWindowGeometry ? "ENABLED" : "DISABLED");
    
    char runPath[512];
    sprintf(runPath, "%s/%s", restore_data->backup_root_path, run_entry->runName);
    
    log_info(LOG_BACKUP, "Restoring from: %s\n", runPath);
    log_info(LOG_BACKUP, "Restoring %lu folder(s)...\n", run_entry->folderCount);
    
    struct iTidy_ProgressWindow *progress_window = NULL;
    sprintf(message, "Restoring %s", run_entry->runName);
    progress_window = iTidy_OpenProgressWindow(restore_data->screen, 
                                                message,
                                                (UWORD)run_entry->folderCount);
    if (!progress_window)
    {
        log_warning(LOG_GUI, "WARNING: Failed to open progress window, continuing without progress display\n");
    }
    
    restoreCtx.userData = progress_window;
    
    status = RestoreFullRun(&restoreCtx, runPath);
    
    if (progress_window)
    {
        iTidy_CloseProgressWindow(progress_window);
        progress_window = NULL;
    }
    
    log_info(LOG_BACKUP, "Restore completed with status: %s\n", GetRestoreStatusMessage(status));
    log_info(LOG_BACKUP, "Archives restored: %u, Archives failed: %u\n",
             restoreCtx.stats.archivesRestored,
             restoreCtx.stats.archivesFailed);
    
    if (restoreCtx.stats.hasErrors)
    {
        log_error(LOG_BACKUP, "First error: %s\n", restoreCtx.stats.firstError);
    }
    
    if (status == RESTORE_OK)
    {
        sprintf(message, "Successfully restored %s\n\n%u folder(s) restored\n%u failed",
                run_entry->runName,
                restoreCtx.stats.archivesRestored,
                restoreCtx.stats.archivesFailed);
        
        ShowEasyRequest(restore_data->window,
                       "Restore Complete",
                       message,
                       "OK");
        
        restore_data->restore_performed = TRUE;
    }
    else
    {
        const char *statusMsg = GetRestoreStatusMessage(status);
        
        if (restoreCtx.stats.hasErrors && restoreCtx.stats.firstError[0] != '\0')
        {
            sprintf(message, "Failed to restore %s\n\nStatus: %s\nError: %s\n\nCheck iTidy.log for details",
                    run_entry->runName,
                    statusMsg,
                    restoreCtx.stats.firstError);
        }
        else
        {
            sprintf(message, "Failed to restore %s\n\nStatus: %s\n\nCheck iTidy.log for details",
                    run_entry->runName,
                    statusMsg);
        }
        
        ShowEasyRequest(restore_data->window,
                       "Restore Failed",
                       message,
                       "OK");
    }
    
    return (status == RESTORE_OK);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Recursively delete a directory and all its contents
 */
/*------------------------------------------------------------------------*/
static BOOL delete_directory_recursive(const char *path)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    BOOL success = TRUE;
    char full_path[512];
    
    if (path == NULL)
        return FALSE;
    
    log_debug(LOG_BACKUP, "delete_directory_recursive: %s\n", path);
    
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        log_error(LOG_BACKUP, "ERROR: Cannot lock directory: %s\n", path);
        return FALSE;
    }
    
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        UnLock(lock);
        log_error(LOG_BACKUP, "ERROR: Cannot allocate FIB\n");
        return FALSE;
    }
    
    if (!Examine(lock, fib))
    {
        FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        log_error(LOG_BACKUP, "ERROR: Cannot examine directory: %s\n", path);
        return FALSE;
    }
    
    while (ExNext(lock, fib))
    {
        snprintf(full_path, sizeof(full_path), "%s/%s", path, fib->fib_FileName);
        
        if (fib->fib_DirEntryType > 0)
        {
            log_debug(LOG_BACKUP, "  Entering subdirectory: %s\n", fib->fib_FileName);
            if (!delete_directory_recursive(full_path))
            {
                success = FALSE;
                log_error(LOG_BACKUP, "ERROR: Failed to delete subdirectory: %s\n", full_path);
            }
        }
        else
        {
            log_debug(LOG_BACKUP, "  Deleting file: %s\n", fib->fib_FileName);
            if (!DeleteFile((STRPTR)full_path))
            {
                success = FALSE;
                log_error(LOG_BACKUP, "ERROR: Failed to delete file: %s\n", full_path);
            }
        }
    }
    
    LONG error = IoErr();
    if (error != ERROR_NO_MORE_ENTRIES)
    {
        log_error(LOG_BACKUP, "ERROR: ExNext failed with error code: %ld\n", error);
        success = FALSE;
    }
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    if (success)
    {
        log_debug(LOG_BACKUP, "  Deleting directory: %s\n", path);
        if (!DeleteFile((STRPTR)path))
        {
            log_error(LOG_BACKUP, "ERROR: Failed to delete directory: %s\n", path);
            success = FALSE;
        }
    }
    
    return success;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Open ReAction class libraries
 */
/*------------------------------------------------------------------------*/
static BOOL open_reaction_classes(void)
{
    if (!WindowBase)      WindowBase = OpenLibrary("window.class", 0);
    if (!LayoutBase)      LayoutBase = OpenLibrary("gadgets/layout.gadget", 0);
    if (!ButtonBase)      ButtonBase = OpenLibrary("gadgets/button.gadget", 0);
    if (!CheckBoxBase)    CheckBoxBase = OpenLibrary("gadgets/checkbox.gadget", 0);
    if (!ListBrowserBase) ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 0);
    if (!LabelBase)       LabelBase = OpenLibrary("images/label.image", 0);
    
    if (!WindowBase || !LayoutBase || !ButtonBase || 
        !CheckBoxBase || !ListBrowserBase || !LabelBase)
    {
        log_error(LOG_GUI, "Failed to open ReAction classes for restore window\n");
        return FALSE;
    }
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Close ReAction class libraries
 */
/*------------------------------------------------------------------------*/
static void close_reaction_classes(void)
{
    if (LabelBase)       { CloseLibrary(LabelBase);       LabelBase = NULL; }
    if (ListBrowserBase) { CloseLibrary(ListBrowserBase); ListBrowserBase = NULL; }
    if (CheckBoxBase)    { CloseLibrary(CheckBoxBase);    CheckBoxBase = NULL; }
    if (ButtonBase)      { CloseLibrary(ButtonBase);      ButtonBase = NULL; }
    if (LayoutBase)      { CloseLibrary(LayoutBase);      LayoutBase = NULL; }
    if (WindowBase)      { CloseLibrary(WindowBase);      WindowBase = NULL; }
}

/*------------------------------------------------------------------------*/
/**
 * @brief Free a ListBrowser list and all its nodes
 */
/*------------------------------------------------------------------------*/
static void free_listbrowser_list(struct List *list)
{
    struct Node *node;
    struct Node *next;
    
    if (!list)
        return;
    
    node = list->lh_Head;
    while ((next = node->ln_Succ))
    {
        FreeListBrowserNode(node);
        node = next;
    }
    
    FreeMem(list, sizeof(struct List));
}

/*------------------------------------------------------------------------*/
/**
 * @brief Open the iTidy Restore Window
 */
/*------------------------------------------------------------------------*/
BOOL open_restore_window(struct iTidyRestoreWindow *restore_data)
{
    struct Screen *screen;
    
    log_info(LOG_GUI, "=== open_restore_window: Starting (ReAction) ===\n");
    
    if (restore_data == NULL)
    {
        log_error(LOG_GUI, "ERROR: restore_data is NULL\n");
        return FALSE;
    }
    
    /* Initialize structure */
    memset(restore_data, 0, sizeof(struct iTidyRestoreWindow));
    restore_data->selected_run_index = -1;
    restore_data->restore_window_geometry = TRUE;
    strcpy(restore_data->backup_root_path, "PROGDIR:Backups");
    
    /* Open ReAction classes */
    if (!open_reaction_classes())
    {
        log_error(LOG_GUI, "Failed to open ReAction classes\n");
        return FALSE;
    }
    
    /* Get Workbench screen */
    screen = LockPubScreen(NULL);
    if (screen == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to lock Workbench screen\n");
        close_reaction_classes();
        return FALSE;
    }
    
    restore_data->screen = screen;
    
    /* Create the ReAction window object */
    restore_data->window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title, RESTORE_WINDOW_TITLE,
        WA_ScreenTitle, RESTORE_WINDOW_TITLE,
        WA_PubScreen, screen,
        WA_Left, 50,
        WA_Top, 30,
        WA_Width, 500,
        WA_Height, 350,
        WA_MinWidth, 400,
        WA_MinHeight, 250,
        WA_MaxWidth, 8192,
        WA_MaxHeight, 8192,
        WA_CloseGadget, TRUE,
        WA_DepthGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_DragBar, TRUE,
        WA_Activate, TRUE,
        WA_NoCareRefresh, TRUE,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE,
        
        WINDOW_ParentGroup, NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_DeferLayout, TRUE,
            
            /* Root vertical layout */
            LAYOUT_AddChild, restore_data->gadgets[0] = NewObject(LAYOUT_GetClass(), NULL,
                GA_ID, GID_RESTORE_ROOT_LAYOUT,
                LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                
                /* Run List Layout */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, GID_RESTORE_RUN_LIST_LAYOUT,
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                    LAYOUT_LeftSpacing, 2,
                    LAYOUT_RightSpacing, 2,
                    LAYOUT_TopSpacing, 2,
                    LAYOUT_BottomSpacing, 2,
                    
                    LAYOUT_AddChild, restore_data->run_listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
                        GA_ID, GID_RESTORE_RUN_LISTBROWSER,
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        LISTBROWSER_ColumnInfo, run_list_column_info,
                        LISTBROWSER_ColumnTitles, TRUE,
                        LISTBROWSER_ShowSelected, TRUE,
                        LISTBROWSER_AutoFit, TRUE,
                    TAG_END),
                TAG_END),
                CHILD_WeightedHeight, 45,
                
                /* Details Layout */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, GID_RESTORE_DETAILS_LAYOUT,
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                    LAYOUT_LeftSpacing, 2,
                    LAYOUT_RightSpacing, 2,
                    LAYOUT_TopSpacing, 2,
                    LAYOUT_BottomSpacing, 2,
                    
                    LAYOUT_AddChild, restore_data->details_listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
                        GA_ID, GID_RESTORE_DETAILS_LISTBROWSER,
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        LISTBROWSER_ColumnInfo, details_column_info,
                        LISTBROWSER_ColumnTitles, FALSE,
                        LISTBROWSER_ShowSelected, FALSE,
                        LISTBROWSER_AutoFit, TRUE,
                    TAG_END),
                TAG_END),
                CHILD_WeightedHeight, 35,
                
                /* Checkbox row */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_TopSpacing, 4,
                    
                    LAYOUT_AddChild, restore_data->window_geom_chk = NewObject(CHECKBOX_GetClass(), NULL,
                        GA_ID, GID_RESTORE_GEOM_CHECKBOX,
                        GA_Text, "Restore window positions",
                        GA_Selected, TRUE,
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                    TAG_END),
                TAG_END),
                CHILD_WeightedHeight, 0,
                
                /* Button row */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, GID_RESTORE_BUTTONS_LAYOUT,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_TopSpacing, 4,
                    LAYOUT_EvenSize, TRUE,
                    
                    LAYOUT_AddChild, restore_data->delete_run_btn = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_RESTORE_DELETE_BUTTON,
                        GA_Text, "Delete Run",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        GA_Disabled, TRUE,
                    TAG_END),
                    
                    LAYOUT_AddChild, restore_data->restore_run_btn = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_RESTORE_RESTORE_BUTTON,
                        GA_Text, "Restore Run",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        GA_Disabled, TRUE,
                    TAG_END),
                    
                    LAYOUT_AddChild, restore_data->view_folders_btn = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_RESTORE_VIEW_BUTTON,
                        GA_Text, "View Folders...",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        GA_Disabled, TRUE,
                    TAG_END),
                    
                    LAYOUT_AddChild, restore_data->cancel_btn = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_RESTORE_CANCEL_BUTTON,
                        GA_Text, "Cancel",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                    TAG_END),
                TAG_END),
                CHILD_WeightedHeight, 0,
                
            TAG_END), /* End root layout */
        TAG_END), /* End parent group */
    TAG_END);
    
    if (restore_data->window_obj == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to create restore window object\n");
        UnlockPubScreen(NULL, screen);
        close_reaction_classes();
        return FALSE;
    }
    
    /* Open the window */
    restore_data->window = (struct Window *)RA_OpenWindow(restore_data->window_obj);
    if (restore_data->window == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to open restore window\n");
        DisposeObject(restore_data->window_obj);
        restore_data->window_obj = NULL;
        UnlockPubScreen(NULL, screen);
        close_reaction_classes();
        return FALSE;
    }
    
    restore_data->window_open = TRUE;
    log_info(LOG_GUI, "Restore window opened successfully\n");
    
    /* Set busy pointer while scanning */
    safe_set_window_pointer(restore_data->window, TRUE);
    
    /* Scan for backup runs */
    log_info(LOG_BACKUP, "Scanning for backup runs in: %s\n", restore_data->backup_root_path);
    restore_data->run_count = scan_backup_runs(restore_data->backup_root_path,
                                               &restore_data->run_entries);
    
    log_info(LOG_BACKUP, "Found %lu backup runs\n", restore_data->run_count);
    
    if (restore_data->run_count > 0)
    {
        populate_run_list(restore_data,
                         restore_data->run_entries,
                         restore_data->run_count,
                         0);
    }
    else
    {
        /* Show empty state in details */
        update_details_panel(restore_data, NULL);
    }
    
    /* Clear busy pointer */
    safe_set_window_pointer(restore_data->window, FALSE);
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Close the Restore Window and cleanup resources
 */
/*------------------------------------------------------------------------*/
void close_restore_window(struct iTidyRestoreWindow *restore_data)
{
    if (restore_data == NULL)
        return;
    
    log_info(LOG_GUI, "close_restore_window: Starting cleanup\n");
    
    /* Dispose window object (automatically frees gadgets) */
    if (restore_data->window_obj != NULL)
    {
        /* Detach lists before disposing */
        if (restore_data->window != NULL)
        {
            if (restore_data->run_listbrowser_obj != NULL)
            {
                SetGadgetAttrs((struct Gadget *)restore_data->run_listbrowser_obj, 
                               restore_data->window, NULL,
                               LISTBROWSER_Labels, ~0,
                               TAG_DONE);
            }
            
            if (restore_data->details_listbrowser_obj != NULL)
            {
                SetGadgetAttrs((struct Gadget *)restore_data->details_listbrowser_obj, 
                               restore_data->window, NULL,
                               LISTBROWSER_Labels, ~0,
                               TAG_DONE);
            }
        }
        
        DisposeObject(restore_data->window_obj);
        restore_data->window_obj = NULL;
        restore_data->window = NULL;
        restore_data->run_listbrowser_obj = NULL;
        restore_data->details_listbrowser_obj = NULL;
    }
    
    /* Free ListBrowser lists */
    if (restore_data->run_list_nodes != NULL)
    {
        free_listbrowser_list(restore_data->run_list_nodes);
        restore_data->run_list_nodes = NULL;
    }
    
    if (restore_data->details_list_nodes != NULL)
    {
        free_listbrowser_list(restore_data->details_list_nodes);
        restore_data->details_list_nodes = NULL;
    }
    
    /* Free run entries */
    if (restore_data->run_entries != NULL)
    {
        whd_free(restore_data->run_entries);
        restore_data->run_entries = NULL;
    }
    
    /* Unlock screen */
    if (restore_data->screen != NULL)
    {
        UnlockPubScreen(NULL, restore_data->screen);
        restore_data->screen = NULL;
    }
    
    /* Close ReAction classes */
    close_reaction_classes();
    
    restore_data->window_open = FALSE;
    
    log_info(LOG_GUI, "Restore window closed successfully\n");
}

/*------------------------------------------------------------------------*/
/**
 * @brief Handle restore window events (main event loop)
 */
/*------------------------------------------------------------------------*/
BOOL handle_restore_window_events(struct iTidyRestoreWindow *restore_data)
{
    ULONG result, code;
    BOOL continue_running = TRUE;
    
    if (restore_data == NULL || restore_data->window_obj == NULL)
        return FALSE;
    
    /* Wait for input */
    Wait(1L << restore_data->window->UserPort->mp_SigBit);
    
    while ((result = RA_HandleInput(restore_data->window_obj, &code)) != WMHI_LASTMSG)
    {
        switch (result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                log_debug(LOG_GUI, "Close gadget clicked\n");
                continue_running = FALSE;
                break;
            
            case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                    case GID_RESTORE_RUN_LISTBROWSER:
                        {
                            ULONG selected = ~0;
                            
                            GetAttr(LISTBROWSER_Selected, 
                                   restore_data->run_listbrowser_obj, 
                                   &selected);
                            
                            if (selected != ~0 && selected < restore_data->run_count)
                            {
                                log_debug(LOG_GUI, "Selected run index: %lu\n", selected);
                                
                                restore_data->selected_run_index = (LONG)selected;
                                
                                /* Update details panel */
                                update_details_panel(restore_data,
                                                    &restore_data->run_entries[selected]);
                                
                                /* Enable buttons */
                                SetGadgetAttrs((struct Gadget *)restore_data->restore_run_btn,
                                               restore_data->window, NULL,
                                               GA_Disabled, FALSE,
                                               TAG_DONE);
                                
                                SetGadgetAttrs((struct Gadget *)restore_data->delete_run_btn,
                                               restore_data->window, NULL,
                                               GA_Disabled, FALSE,
                                               TAG_DONE);
                                
                                /* Enable view folders if catalog exists */
                                SetGadgetAttrs((struct Gadget *)restore_data->view_folders_btn,
                                               restore_data->window, NULL,
                                               GA_Disabled, !restore_data->run_entries[selected].hasCatalog,
                                               TAG_DONE);
                            }
                        }
                        break;
                    
                    case GID_RESTORE_GEOM_CHECKBOX:
                        {
                            ULONG checked = 0;
                            GetAttr(GA_Selected, restore_data->window_geom_chk, &checked);
                            restore_data->restore_window_geometry = (BOOL)checked;
                            log_debug(LOG_GUI, "Window geometry restore: %s\n", 
                                     restore_data->restore_window_geometry ? "ENABLED" : "DISABLED");
                        }
                        break;
                    
                    case GID_RESTORE_RESTORE_BUTTON:
                        if (restore_data->selected_run_index >= 0 &&
                            restore_data->selected_run_index < (LONG)restore_data->run_count)
                        {
                            perform_restore_run(restore_data,
                                &restore_data->run_entries[restore_data->selected_run_index]);
                        }
                        break;
                    
                    case GID_RESTORE_DELETE_BUTTON:
                        if (restore_data->selected_run_index >= 0 &&
                            restore_data->selected_run_index < (LONG)restore_data->run_count)
                        {
                            struct RestoreRunEntry *selected_entry = 
                                &restore_data->run_entries[restore_data->selected_run_index];
                            char message[512];
                            
                            sprintf(message, "Delete backup run %s?\n\nThis will permanently delete:\n- %lu folder archive(s)\n- Catalog file\n- Run directory\n\nThis action cannot be undone!",
                                    selected_entry->runName,
                                    selected_entry->folderCount);
                            
                            if (ShowEasyRequest(restore_data->window,
                                               "Confirm Delete",
                                               message,
                                               "Delete|Cancel"))
                            {
                                char run_path[512];
                                
                                sprintf(run_path, "%s/%s", restore_data->backup_root_path, selected_entry->runName);
                                
                                log_info(LOG_BACKUP, "Deleting backup run: %s\n", run_path);
                                
                                if (delete_directory_recursive(run_path))
                                {
                                    log_info(LOG_BACKUP, "Successfully deleted run: %s\n", selected_entry->runName);
                                    
                                    /* Rescan backup directory */
                                    if (restore_data->run_entries != NULL)
                                    {
                                        whd_free(restore_data->run_entries);
                                        restore_data->run_entries = NULL;
                                    }
                                    
                                    restore_data->run_count = scan_backup_runs(
                                        restore_data->backup_root_path,
                                        &restore_data->run_entries);
                                    
                                    if (restore_data->run_count > 0)
                                    {
                                        populate_run_list(restore_data,
                                                         restore_data->run_entries,
                                                         restore_data->run_count,
                                                         0);
                                    }
                                    else
                                    {
                                        /* No runs left */
                                        update_details_panel(restore_data, NULL);
                                        
                                        SetGadgetAttrs((struct Gadget *)restore_data->restore_run_btn,
                                                       restore_data->window, NULL,
                                                       GA_Disabled, TRUE,
                                                       TAG_DONE);
                                        SetGadgetAttrs((struct Gadget *)restore_data->delete_run_btn,
                                                       restore_data->window, NULL,
                                                       GA_Disabled, TRUE,
                                                       TAG_DONE);
                                        SetGadgetAttrs((struct Gadget *)restore_data->view_folders_btn,
                                                       restore_data->window, NULL,
                                                       GA_Disabled, TRUE,
                                                       TAG_DONE);
                                    }
                                    
                                    sprintf(message, "Backup run %s deleted successfully.", selected_entry->runName);
                                    
                                    ShowEasyRequest(restore_data->window,
                                                   "Delete Complete",
                                                   message,
                                                   "OK");
                                }
                                else
                                {
                                    log_error(LOG_BACKUP, "ERROR: Failed to delete run: %s\n", run_path);
                                    
                                    sprintf(message, "Failed to delete backup run %s.\n\nThe directory may be in use or protected.\nCheck the log for details.", selected_entry->runName);
                                    
                                    ShowEasyRequest(restore_data->window,
                                                   "Delete Failed",
                                                   message,
                                                   "OK");
                                }
                            }
                            else
                            {
                                log_debug(LOG_BACKUP, "Delete cancelled by user\n");
                            }
                        }
                        break;
                    
                    case GID_RESTORE_VIEW_BUTTON:
                        if (restore_data->selected_run_index >= 0 &&
                            restore_data->selected_run_index < (LONG)restore_data->run_count)
                        {
                            struct RestoreRunEntry *selected_entry = 
                                &restore_data->run_entries[restore_data->selected_run_index];
                            char catalog_path[512];
                            struct iTidyFolderViewWindow folder_view_data;
                            
                            sprintf(catalog_path, "%s/%s/catalog.txt",
                                   restore_data->backup_root_path,
                                   selected_entry->runName);
                            
                            log_debug(LOG_GUI, "Opening folder view for: %s\n", catalog_path);
                            
                            memset(&folder_view_data, 0, sizeof(folder_view_data));
                            folder_view_data.screen = restore_data->screen;
                            
                            if (open_folder_view_window(&folder_view_data,
                                                       catalog_path,
                                                       selected_entry->runNumber,
                                                       selected_entry->dateStr,
                                                       selected_entry->folderCount))
                            {
                                while (handle_folder_view_window_events(&folder_view_data))
                                {
                                    WaitPort(folder_view_data.window->UserPort);
                                }
                                
                                close_folder_view_window(&folder_view_data);
                            }
                            else
                            {
                                log_error(LOG_GUI, "ERROR: Failed to open folder view window\n");
                            }
                        }
                        break;
                    
                    case GID_RESTORE_CANCEL_BUTTON:
                        log_debug(LOG_GUI, "Cancel button clicked\n");
                        continue_running = FALSE;
                        break;
                }
                break;
        }
    }
    
    return continue_running;
}
