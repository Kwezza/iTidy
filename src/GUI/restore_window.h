/*
 * restore_window.h - iTidy Restore Window Header
 * GadTools-based Backup Restore GUI for Workbench 2.0+
 * Pure Intuition + GadTools (No MUI, No ReAction)
 */

#ifndef ITIDY_RESTORE_WINDOW_H
#define ITIDY_RESTORE_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>

/* Include listview formatter for iTidy_ListViewState */
#include "helpers/listview_columns_api.h"

/*------------------------------------------------------------------------*/
/* Gadget IDs                                                             */
/*------------------------------------------------------------------------*/
#define GID_RESTORE_RUN_LIST        2003
#define GID_RESTORE_DETAILS         2004
#define GID_RESTORE_WINDOW_GEOM_CHK 2005
#define GID_RESTORE_RUN_BTN         2006
#define GID_RESTORE_VIEW_FOLDERS    2007
#define GID_RESTORE_DELETE_RUN      2008
#define GID_RESTORE_CANCEL          2009
#define GID_RESTORE_MODE_CYCLE      2010
#define GID_RESTORE_APPLY_MODE      2011
#define GID_RESTORE_PAGE_SIZE_CYCLE 2012

/*------------------------------------------------------------------------*/
/* Window Spacing Constants                                              */
/*------------------------------------------------------------------------*/
#define RESTORE_SPACE_X         10      /* Horizontal spacing */
#define RESTORE_SPACE_Y         8       /* Vertical spacing */
#define RESTORE_MARGIN_LEFT     4      /* Left margin */
#define RESTORE_MARGIN_TOP      4      /* Top margin (added to currentWindowBarHeight) */
#define RESTORE_MARGIN_RIGHT    10      /* Right margin */
#define RESTORE_MARGIN_BOTTOM   10      /* Bottom margin */
#define RESTORE_BEVEL_BORDER    4       /* Checkerboard border around bevel */
#define RESTORE_CONTENT_PADDING 8       /* Padding inside bevel for content */
#define RESTORE_BUTTON_SPACING  0       /* Space between bevel and buttons */

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
};

/*------------------------------------------------------------------------*/
/* Main Restore Window Data Structure                                    */
/*------------------------------------------------------------------------*/
struct iTidyRestoreWindow
{
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Restore window */
    APTR visual_info;                   /* GadTools visual info */
    struct Gadget *glist;               /* Gadget list */
    struct TextFont *system_font;       /* System default font (if opened) */
    BOOL window_open;                   /* Window state flag */
    
    /* Gadget pointers */
    struct Gadget *run_list;
    struct Gadget *details_listview;   /* Read-only details display */
    struct Gadget *window_geom_chk;    /* Checkbox: restore window geometry */
    struct Gadget *restore_run_btn;
    struct Gadget *view_folders_btn;
    struct Gadget *delete_run_btn;
    struct Gadget *cancel_btn;
    struct Gadget *mode_cycle;         /* ListView mode selector */
    struct Gadget *page_size_cycle;    /* Page size selector */
    struct Gadget *apply_mode_btn;     /* Apply selected mode to run list */
    
    /* Current state */
    char backup_root_path[256];         /* Current backup location */
    struct RestoreRunEntry *run_entries; /* Array of runs */
    struct List *run_list_strings;      /* Formatted display list for run ListView */
    struct List run_entry_list;         /* Entry list for sortable run ListView */
    iTidy_ListViewState *run_list_state; /* State tracking for sortable columns */
    struct List *details_list_strings;  /* List for details ListView */
    ULONG run_count;                    /* Number of runs found */
    LONG selected_run_index;            /* Currently selected (-1 if none) */
    BOOL restore_window_geometry;       /* TRUE to restore window positions (default TRUE) */
    
    /* Pagination state (API-managed via run_list_state) */
    int current_page;                   /* Initial page for first load (1-based) - API manages after that */
    int page_size;                      /* Entries per page (0 = no pagination) */
    int current_mode;                   /* Current ListView mode (0-3: FULL/FULL_NO_SORT/SIMPLE/SIMPLE_PAGINATED) */
    
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
