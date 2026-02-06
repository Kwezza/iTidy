/*
 * restore_window.h - iTidy Restore Window Header
 * ReAction GUI for Workbench 3.2+
 * 
 * Migrated from GadTools to ReAction.
 */

#ifndef ITIDY_RESTORE_WINDOW_H
#define ITIDY_RESTORE_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>

/*------------------------------------------------------------------------*/
/* Status Codes                                                           */
/*------------------------------------------------------------------------*/
typedef enum {
    RESTORE_STATUS_COMPLETE = 0,        /* Has catalog, all archives present */
    RESTORE_STATUS_INCOMPLETE = 1,      /* Has catalog, missing archives */
    RESTORE_STATUS_ORPHANED = 2,        /* No catalog.txt */
    RESTORE_STATUS_CORRUPTED = 3        /* Catalog parse error */
} RestoreRunStatus;

/*------------------------------------------------------------------------*/
/* Run Entry Structure                                                    */
/*------------------------------------------------------------------------*/
struct RestoreRunEntry
{
    UWORD runNumber;                    /* Run number (1-9999) */
    char displayString[80];             /* Formatted for ListView */
    char runName[16];                   /* "Run_0007" */
    char dateStr[24];                   /* "2025-10-25 14:32:17" full date */
    char sourceDirectory[256];          /* Source folder that was tidied */
    ULONG folderCount;                  /* Number of archives */
    ULONG totalBytes;                   /* Total size in bytes */
    char sizeStr[16];                   /* "46 KB" formatted */
    UWORD statusCode;                   /* RestoreRunStatus enum */
    char statusStr[16];                 /* "Complete" text */
    char fullPath[256];                 /* Full path to run directory */
    BOOL hasCatalog;                    /* TRUE if catalog.txt exists */
    ULONG iconsCreated;                 /* Number of .info files DefIcons created during run */
};

/*------------------------------------------------------------------------*/
/* Main Restore Window Data Structure (ReAction)                         */
/*------------------------------------------------------------------------*/
struct iTidyRestoreWindow
{
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Intuition window */
    Object *window_obj;                 /* ReAction window object */
    BOOL window_open;                   /* Window state flag */
    
    /* ReAction gadget objects */
    Object *gadgets[16];                /* Gadget object array */
    Object *run_listbrowser_obj;        /* Run list ListBrowser */
    Object *details_listbrowser_obj;    /* Details ListBrowser */
    Object *restore_run_btn;            /* Restore Run button */
    Object *view_folders_btn;           /* View Folders button */
    Object *delete_run_btn;             /* Delete Run button */
    Object *cancel_btn;                 /* Cancel button */
    
    /* ListBrowser data lists */
    struct List *run_list_nodes;        /* ListBrowser nodes for run list */
    struct List *details_list_nodes;    /* ListBrowser nodes for details */
    
    /* Current state */
    char backup_root_path[256];         /* Current backup location */
    struct RestoreRunEntry *run_entries; /* Array of runs */
    ULONG run_count;                    /* Number of runs found */
    LONG selected_run_index;            /* Currently selected (-1 if none) */
    BOOL restore_window_geometry;       /* TRUE to restore window positions (default TRUE) */
    
    /* Double-click tracking */
    ULONG last_click_secs;              /* Last click timestamp seconds */
    ULONG last_click_micros;            /* Last click timestamp microseconds */
    
    /* Result */
    BOOL restore_performed;             /* TRUE if any restore done */
};

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Open the iTidy Restore Window
 * 
 * Opens a modal window for browsing and restoring backups.
 * 
 * @param restore_data Pointer to restore window data structure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
BOOL open_restore_window(struct iTidyRestoreWindow *restore_data);

/**
 * @brief Close the Restore Window and cleanup resources
 * 
 * @param restore_data Pointer to restore window data structure
 */
void close_restore_window(struct iTidyRestoreWindow *restore_data);

/**
 * @brief Handle restore window events (main event loop)
 * 
 * @param restore_data Pointer to restore window data structure
 * @return BOOL TRUE to continue, FALSE to close window
 */
BOOL handle_restore_window_events(struct iTidyRestoreWindow *restore_data);

/**
 * @brief Scan backup directory for available runs
 * 
 * @param backup_root Path to backup root directory
 * @param entries_out Pointer to receive allocated array
 * @return ULONG Number of runs found
 */
ULONG scan_backup_runs(const char *backup_root, 
                       struct RestoreRunEntry **entries_out);

/**
 * @brief Populate list view with backup run entries
 * 
 * @param restore_data Pointer to restore window data
 * @param entries Array of run entries
 * @param count Number of entries
 * @param nav_direction Navigation direction (-1=Previous, 0=None, +1=Next)
 */
void populate_run_list(struct iTidyRestoreWindow *restore_data,
                       struct RestoreRunEntry *entries,
                       ULONG count,
                       int nav_direction);

/**
 * @brief Update details panel with selected run information
 * 
 * @param restore_data Pointer to restore window data
 * @param selected_entry Selected run entry (or NULL)
 */
void update_details_panel(struct iTidyRestoreWindow *restore_data,
                          struct RestoreRunEntry *selected_entry);

/**
 * @brief Perform full run restore with progress feedback
 * 
 * @param restore_data Pointer to restore window data
 * @param run_entry Backup run to restore
 * @return BOOL TRUE if successful, FALSE on error
 */
BOOL perform_restore_run(struct iTidyRestoreWindow *restore_data,
                        struct RestoreRunEntry *run_entry);

/**
 * @brief Format a size in bytes to human-readable string
 * 
 * @param bytes Size in bytes
 * @param buffer Output buffer (must be at least 16 bytes)
 */
void format_size_string(ULONG bytes, char *buffer);

#endif /* ITIDY_RESTORE_WINDOW_H */
