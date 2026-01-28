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
/* Gadget Indices (matching implementation)                               */
/*------------------------------------------------------------------------*/
enum itidy_advanced_gadgets {
    ITIDY_ADV_ROOT_LAYOUT = 0,
    ITIDY_ADV_ROW_DETAILS,
    ITIDY_ADV_ASPECT_GROUP,
    ITIDY_ADV_ASPECT_CHOOSER,
    ITIDY_ADV_OVERFLOW_CHOOSER,
    ITIDY_ADV_SPACING_GROUP,
    ITIDY_ADV_SPACING_X,
    ITIDY_ADV_SPACING_Y,
    ITIDY_ADV_ICONS_ROW_GROUP,
    ITIDY_ADV_MIN_ICONS_ROW,
    ITIDY_ADV_AUTO_MAX_CHECK,
    ITIDY_ADV_MAX_ICONS_ROW,
    ITIDY_ADV_WIDTH_VERT_GROUP,
    ITIDY_ADV_MAX_WIDTH_CHOOSER,
    ITIDY_ADV_VERT_ALIGN_CHOOSER,
    ITIDY_ADV_CHECKBOX_GROUP,
    ITIDY_ADV_CHECKBOX_COL1,
    ITIDY_ADV_REVERSE_SORT_CHECK,
    ITIDY_ADV_OPTIMIZE_COLS_CHECK,
    ITIDY_ADV_COLUMN_LAYOUT_CHECK,
    ITIDY_ADV_CHECKBOX_COL2,
    ITIDY_ADV_SKIP_HIDDEN_CHECK,
    ITIDY_ADV_STRIP_BORDERS_CHECK,
    ITIDY_ADV_GROUPING_LAYOUT,
    ITIDY_ADV_BLOCK_GAP_CHOOSER,
    ITIDY_ADV_BUTTON_LAYOUT,
    ITIDY_ADV_OK_BUTTON,
    ITIDY_ADV_CANCEL_BUTTON,
    
    ITIDY_ADV_NUM_GADGETS
};

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
