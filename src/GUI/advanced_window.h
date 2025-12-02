/*
 * advanced_window.h - iTidy Advanced Settings Window Header
 * GadTools-based GUI for Workbench 3.0+
 * Aspect Ratio and Window Overflow Configuration
 */

#ifndef ITIDY_ADVANCED_WINDOW_H
#define ITIDY_ADVANCED_WINDOW_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include "layout_preferences.h"

/*------------------------------------------------------------------------*/
/* Gadget IDs                                                             */
/*------------------------------------------------------------------------*/
#define GID_ADV_ASPECT_RATIO      1001
#define GID_ADV_CUSTOM_WIDTH      1002
#define GID_ADV_CUSTOM_HEIGHT     1003
#define GID_ADV_OVERFLOW_MODE     1004
#define GID_ADV_SPACING_X         1005
#define GID_ADV_SPACING_Y         1006
#define GID_ADV_MIN_ICONS_ROW     1007
#define GID_ADV_MAX_ICONS_ROW     1008
#define GID_ADV_MAX_AUTO_CHECKBOX 1011
#define GID_ADV_MAX_WIDTH_PCT     1012
#define GID_ADV_VERTICAL_ALIGN    1013
#define GID_ADV_REVERSE_SORT      1014
#define GID_ADV_OPTIMIZE_COLS     1016
#define GID_ADV_SKIP_HIDDEN       1017
#define GID_ADV_COLUMN_LAYOUT     1018
#define GID_ADV_ICONS_LABEL       1019
#define GID_ADV_STRIP_NEWICON_BORDERS 1020
#define GID_ADV_BETA_OPTIONS      1015
#define GID_ADV_OK                1009
#define GID_ADV_CANCEL            1010

/*------------------------------------------------------------------------*/
/* Aspect Ratio Preset Indices                                           */
/*------------------------------------------------------------------------*/
#define ASPECT_PRESET_TALL        0   /* 0.75 (3:4) */
#define ASPECT_PRESET_SQUARE      1   /* 1.0  (1:1) */
#define ASPECT_PRESET_COMPACT     2   /* 1.3  (4:3) */
#define ASPECT_PRESET_CLASSIC     3   /* 1.6  (16:10) */
#define ASPECT_PRESET_WIDE        4   /* 2.0  (2:1) */
#define ASPECT_PRESET_ULTRAWIDE   5   /* 2.4  (21:9 approx) */

/*------------------------------------------------------------------------*/
/* Max Window Width Percentage Preset Indices                            */
/*------------------------------------------------------------------------*/
#define MAX_WIDTH_AUTO            0   /* Auto (use preset default) */
#define MAX_WIDTH_30              1   /* 30% of screen */
#define MAX_WIDTH_50              2   /* 50% of screen */
#define MAX_WIDTH_70              3   /* 70% of screen */
#define MAX_WIDTH_90              4   /* 90% of screen */
#define MAX_WIDTH_100             5   /* 100% of screen (full width) */

/*------------------------------------------------------------------------*/
/* Advanced Window Data Structure                                        */
/*------------------------------------------------------------------------*/
struct iTidyAdvancedWindow
{
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Advanced settings window */
    APTR visual_info;                   /* GadTools visual info */
    struct Gadget *glist;               /* Gadget list */
    BOOL window_open;                   /* Window state flag */
    
    /* Gadget pointers for easy access */
    struct Gadget *aspect_ratio_cycle;
    struct Gadget *custom_width_int;
    struct Gadget *custom_height_int;
    struct Gadget *overflow_mode_cycle;
    struct Gadget *spacing_x_slider;
    struct Gadget *spacing_y_slider;
    struct Gadget *min_icons_row_int;
    struct Gadget *max_icons_row_int;
    struct Gadget *max_auto_checkbox;
    struct Gadget *max_width_pct_cycle;
    struct Gadget *vertical_align_cycle;
    struct Gadget *reverse_sort_check;
    struct Gadget *optimize_cols_check;
    struct Gadget *skip_hidden_check;
    struct Gadget *column_layout_check;
    struct Gadget *strip_newicon_borders_check;
    struct Gadget *beta_options_btn;
    struct Gadget *ok_btn;
    struct Gadget *cancel_btn;
    
    /* Current settings */
    WORD aspect_preset_selected;        /* Index into aspect ratio presets */
    UWORD custom_aspect_width;          /* Custom ratio width (e.g., 16) */
    UWORD custom_aspect_height;         /* Custom ratio height (e.g., 9) */
    WORD overflow_mode_selected;        /* Index: 0=Horizontal, 1=Vertical, 2=Both */
    UWORD spacing_x_value;              /* Horizontal spacing (4-20 pixels) */
    UWORD spacing_y_value;              /* Vertical spacing (4-20 pixels) */
    UWORD min_icons_per_row;            /* Minimum columns (1-10) */
    UWORD max_icons_per_row;            /* Maximum columns (0=Auto, 1-20) */
    BOOL max_auto_enabled;              /* TRUE if Auto mode, FALSE if manual */
    WORD max_width_pct_selected;        /* Index into max width percentage presets */
    WORD vertical_align_selected;       /* Index: 0=Top, 1=Middle, 2=Bottom */
    BOOL reverse_sort_enabled;          /* TRUE if reverse sort enabled */
    BOOL optimize_cols_enabled;         /* TRUE if column width optimization enabled */
    BOOL skip_hidden_enabled;           /* TRUE if skip hidden folders enabled */
    BOOL column_layout_enabled;         /* TRUE if column layout (center icons) enabled */
    BOOL strip_newicon_borders_enabled; /* TRUE if strip NewIcon borders enabled */
    
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
 * Opens a modal window for configuring aspect ratio and window overflow
 * settings. The window is positioned on the Workbench screen and blocks
 * interaction with the main window until closed.
 *
 * @param adv_data Pointer to advanced window data structure
 * @param prefs Pointer to LayoutPreferences to configure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
BOOL open_itidy_advanced_window(struct iTidyAdvancedWindow *adv_data, 
                                 LayoutPreferences *prefs);

/**
 * @brief Close the iTidy Advanced Settings window and cleanup resources
 *
 * Frees all gadgets, visual info, and closes the window. Does not modify
 * preferences unless changes_accepted flag is TRUE.
 *
 * @param adv_data Pointer to advanced window data structure
 */
void close_itidy_advanced_window(struct iTidyAdvancedWindow *adv_data);

/**
 * @brief Handle advanced window events (main event loop)
 *
 * Processes window events including gadget interactions, close requests,
 * and refresh events. Returns FALSE when window should be closed.
 *
 * @param adv_data Pointer to advanced window data structure
 * @return BOOL TRUE to continue, FALSE to close window
 */
BOOL handle_advanced_window_events(struct iTidyAdvancedWindow *adv_data);

/**
 * @brief Load current preferences into advanced window gadgets
 *
 * Reads values from LayoutPreferences structure and updates all gadgets
 * to reflect current settings. Called after window opens.
 *
 * @param adv_data Pointer to advanced window data structure
 */
void load_preferences_to_advanced_window(struct iTidyAdvancedWindow *adv_data);

/**
 * @brief Save advanced window gadget values to preferences
 *
 * Reads all gadget values and updates the LayoutPreferences structure.
 * Only called when OK button is clicked.
 *
 * @param adv_data Pointer to advanced window data structure
 */
void save_advanced_window_to_preferences(struct iTidyAdvancedWindow *adv_data);

/**
 * @brief Enable/disable custom aspect ratio input gadgets
 *
 * Enables the custom width/height integer gadgets when "Custom" is selected
 * in the aspect ratio cycle gadget, disables them otherwise.
 *
 * @param adv_data Pointer to advanced window data structure
 * @param enable TRUE to enable, FALSE to disable
 */
void set_custom_ratio_gadgets_state(struct iTidyAdvancedWindow *adv_data, 
                                    BOOL enable);

/**
 * @brief Enable/disable Max Icons/Row integer gadget based on Auto checkbox
 *
 * Disables the max icons/row integer gadget when Auto checkbox is checked,
 * enables it when unchecked.
 *
 * @param adv_data Pointer to advanced window data structure
 * @param auto_enabled TRUE if Auto mode, FALSE if manual
 */
void set_max_icons_gadget_state(struct iTidyAdvancedWindow *adv_data,
                                BOOL auto_enabled);

/**
 * @brief Enable/disable Max Window Width Pct gadget based on Max Icons/Row mode
 *
 * Disables the max window width percentage cycle gadget when Max Icons/Row 
 * is in manual mode (not Auto), enables it when in Auto mode.
 *
 * @param adv_data Pointer to advanced window data structure
 */
void update_max_width_pct_gadget_state(struct iTidyAdvancedWindow *adv_data);

#endif /* ITIDY_ADVANCED_WINDOW_H */
