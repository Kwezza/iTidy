/*
 * advanced_window.h - iTidy Advanced Settings Window
 * ReAction-based GUI for Workbench 3.2+
 * Refactored from GadTools version (Stage 3 Migration)
 */

#ifndef ITIDY_ADVANCED_WINDOW_H
#define ITIDY_ADVANCED_WINDOW_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <libraries/gadtools.h>
#include "layout_preferences.h"

/*------------------------------------------------------------------------*/
/* Gadget Indices (matching implementation with Tab structure)            */
/*------------------------------------------------------------------------*/
enum itidy_advanced_gadgets {
    ITIDY_ADV_ROOT_LAYOUT = 0,
    ITIDY_ADV_CLICKTAB,
    /* Tab Pages */
    ITIDY_ADV_TAB_LAYOUT,
    ITIDY_ADV_TAB_DENSITY,
    ITIDY_ADV_TAB_LIMITS,
    ITIDY_ADV_TAB_COLUMNS_GROUPS,
    ITIDY_ADV_TAB_FILTERS_MISC,
    /* Tab 1: Layout - Gadgets */
    ITIDY_ADV_ASPECT_CHOOSER,
    ITIDY_ADV_OVERFLOW_CHOOSER,
    ITIDY_ADV_VERT_ALIGN_CHOOSER,
    /* Tab 2: Density - Gadgets */
    ITIDY_ADV_SPACING_X,
    ITIDY_ADV_SPACING_Y,
    ITIDY_ADV_SPACE_DENSITY,
    /* Tab 3: Limits - Gadgets */
    ITIDY_ADV_MIN_ICONS_ROW,
    ITIDY_ADV_AUTO_MAX_CHECK,
    ITIDY_ADV_MAX_ICONS_ROW,
    ITIDY_ADV_MAX_WIDTH_CHOOSER,
    /* Tab 4: Columns & Groups - Gadgets */
    ITIDY_ADV_COLUMN_LAYOUT_CHECK,
    ITIDY_ADV_BLOCK_GAP_CHOOSER,
    ITIDY_ADV_SPACE_COLUMNS,
    /* Tab 5: Filters & Misc - Gadgets */
    ITIDY_ADV_SKIP_HIDDEN_CHECK,
    ITIDY_ADV_STRIP_BORDERS_CHECK,
    ITIDY_ADV_REVERSE_SORT_CHECK,
    ITIDY_ADV_OPTIMIZE_COLS_CHECK,
    /* Buttons */
    ITIDY_ADV_BUTTON_LAYOUT,
    ITIDY_ADV_BUTTON_COL1,
    ITIDY_ADV_BUTTON_COL2,
    ITIDY_ADV_DEFAULTS_BUTTON,
    ITIDY_ADV_CANCEL_BUTTON,
    ITIDY_ADV_OK_BUTTON,
    
    ITIDY_ADV_NUM_GADGETS
};

/*------------------------------------------------------------------------*/
/* GA_ID Values for WMHI_GADGETUP Events and HintInfo                    */
/* These must match the testcode.c enum values                           */
/*------------------------------------------------------------------------*/
#define ITIDY_ADV_GAID_ROOT_LAYOUT                  0
#define ITIDY_ADV_GAID_CLICKTAB                     1
/* Tab Pages */
#define ITIDY_ADV_GAID_TAB_LAYOUT                   2
#define ITIDY_ADV_GAID_ASPECT_CHOOSER               3
#define ITIDY_ADV_GAID_OVERFLOW_CHOOSER             4
#define ITIDY_ADV_GAID_VERT_ALIGN_CHOOSER           5
#define ITIDY_ADV_GAID_TAB_DENSITY                  6
#define ITIDY_ADV_GAID_SPACING_X                    7
#define ITIDY_ADV_GAID_SPACING_Y                    8
#define ITIDY_ADV_GAID_SPACE_DENSITY                9
#define ITIDY_ADV_GAID_TAB_LIMITS                   10
#define ITIDY_ADV_GAID_MIN_ICONS_ROW                11
#define ITIDY_ADV_GAID_AUTO_MAX_CHECK               12
#define ITIDY_ADV_GAID_MAX_ICONS_ROW                13
#define ITIDY_ADV_GAID_MAX_WIDTH_CHOOSER            14
#define ITIDY_ADV_GAID_TAB_COLUMNS_GROUPS           15
#define ITIDY_ADV_GAID_COLUMN_LAYOUT_CHECK          16
#define ITIDY_ADV_GAID_BLOCK_GAP_CHOOSER            17
#define ITIDY_ADV_GAID_SPACE_COLUMNS                18
#define ITIDY_ADV_GAID_TAB_FILTERS_MISC             19
#define ITIDY_ADV_GAID_SKIP_HIDDEN_CHECK            20
#define ITIDY_ADV_GAID_STRIP_BORDERS_CHECK          21
#define ITIDY_ADV_GAID_REVERSE_SORT_CHECK           22
#define ITIDY_ADV_GAID_OPTIMIZE_COLS_CHECK          23
#define ITIDY_ADV_GAID_BUTTON_LAYOUT                24
#define ITIDY_ADV_GAID_BUTTON_COL1                  25
#define ITIDY_ADV_GAID_BUTTON_COL2                  26
#define ITIDY_ADV_GAID_DEFAULTS_BUTTON              27
#define ITIDY_ADV_GAID_CANCEL_BUTTON                28
#define ITIDY_ADV_GAID_OK_BUTTON                    29

/*------------------------------------------------------------------------*/
/* Advanced Window Data Structure                                         */
/*------------------------------------------------------------------------*/
struct iTidyAdvancedWindow
{
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Window pointer (from window_obj) */
    Object *window_obj;                 /* Main ReAction window object */
    Object *gadgets[ITIDY_ADV_NUM_GADGETS]; /* Gadget objects */
    BOOL window_open;                   /* Window state flag */
    
    /* Layout Labels Lists (Need to be freed) */
    struct List *aspect_labels;
    struct List *overflow_labels;
    struct List *width_labels;
    struct List *align_labels;
    struct List *gap_labels;
    struct List *tab_labels;

    /* Current settings state (mirrors widgets) */
    /* These track current widget state to handle interaction logic */
    BOOL auto_max_enabled;
    
    /* Pointer to preferences to update */
    LayoutPreferences *prefs;
    
    /* Result flag */
    BOOL changes_accepted;              /* TRUE if OK clicked, FALSE if Cancel */
};

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Open the iTidy Advanced Settings window
 *
 * Opens a modal window for configuring advanced settings.
 *
 * @param adv_data Pointer to window data structure
 * @param prefs Pointer to global preferences
 * @return TRUE if window opened successfully
 */
BOOL open_itidy_advanced_window(struct iTidyAdvancedWindow *adv_data, 
                                LayoutPreferences *prefs);

/**
 * @brief Handle events for the Advanced Settings window
 *
 * Runs the event loop until the window is closed or OK/Cancel is clicked.
 *
 * @param adv_data Pointer to window data structure
 */
void handle_itidy_advanced_window_events(struct iTidyAdvancedWindow *adv_data);

/**
 * @brief Close the Advanced Settings window and free resources
 *
 * @param adv_data Pointer to window data structure
 */
void close_itidy_advanced_window(struct iTidyAdvancedWindow *adv_data);

#endif /* ITIDY_ADVANCED_WINDOW_H */
