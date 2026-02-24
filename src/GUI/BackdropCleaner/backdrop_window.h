/*
 * backdrop_window.h - iTidy Workbench Screen Manager Window Header
 * ReAction GUI for Workbench 3.2+
 *
 * Provides a ListBrowser-based view of all Workbench backdrop entries
 * (device icons and left-out icons) with audit status and layout tools.
 */

#ifndef ITIDY_BACKDROP_WINDOW_H
#define ITIDY_BACKDROP_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>

#include "backups/backdrop_parser.h"
#include "DOS/device_scanner.h"
#include "layout/workbench_layout.h"

/*------------------------------------------------------------------------*/
/* Backdrop Window Data Structure                                         */
/*------------------------------------------------------------------------*/
struct iTidyBackdropWindow
{
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Intuition window */
    Object *window_obj;                 /* ReAction window object */
    BOOL window_open;                   /* Window state flag */

    /* ReAction gadget objects */
    Object *listbrowser_obj;            /* Main ListBrowser */
    Object *scan_btn;                   /* Scan button */
    Object *remove_btn;                 /* Remove Selected button */
    Object *tidy_btn;                   /* Tidy Layout button */
    Object *test_btn;                   /* TEST: Update+Redraw All button */
    Object *close_btn;                  /* Close button */
    Object *status_label_obj;           /* Status label (dynamic text) */

    /* ListBrowser data */
    struct List *list_nodes;            /* ListBrowser node list */

    /* Scan results */
    iTidy_BackdropList *backdrop_list;          /* Parsed backdrop entries */
    iTidy_DeviceList *device_list;              /* Scanned devices */

    /* State */
    LONG selected_index;                /* Currently selected (-1 = none) */
    BOOL scan_performed;                /* TRUE after first scan */
    BOOL changes_made;                  /* TRUE if any removals or tidy done */

    /* Column sorting state */
    ULONG sort_column;                  /* Last column sorted (0-based) */
    ULONG sort_direction;               /* LBMSORT_FORWARD or LBMSORT_REVERSE */

    /* Menu */
    struct Menu *menu_strip;            /* GadTools menu strip */
    APTR         visual_info;           /* VisualInfo for LayoutMenus */
};

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Open the Backdrop Cleaner / Workbench Screen Manager window
 *
 * Opens a modal window that scans all mounted volumes for .backdrop entries,
 * validates them, and presents the results in a ListBrowser.
 *
 * @param bd_data  Pointer to backdrop window data (caller-allocated)
 * @return BOOL TRUE if window opened successfully
 */
BOOL open_backdrop_window(struct iTidyBackdropWindow *bd_data);

/**
 * @brief Close the Backdrop Window and clean up all resources
 *
 * @param bd_data  Pointer to backdrop window data
 */
void close_backdrop_window(struct iTidyBackdropWindow *bd_data);

/**
 * @brief Handle one batch of events for the backdrop window
 *
 * Call this in a loop. Internally calls Wait() for events.
 *
 * @param bd_data  Pointer to backdrop window data
 * @return BOOL TRUE to keep running, FALSE to close window
 */
BOOL handle_backdrop_window_events(struct iTidyBackdropWindow *bd_data);

#endif /* ITIDY_BACKDROP_WINDOW_H */
