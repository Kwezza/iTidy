/*
 * main_window_reaction.h - iTidy Main Window ReAction Gadget IDs
 * ReAction GUI gadget index definitions for Workbench 3.2+
 */

#ifndef ITIDY_MAIN_WINDOW_REACTION_H
#define ITIDY_MAIN_WINDOW_REACTION_H

/*------------------------------------------------------------------------*/
/* ReAction Gadget Array Indices                                          */
/* These indices are used to access gadgets in the win_data->gadgets[]   */
/* array. Each index corresponds to a specific gadget or layout object.  */
/*------------------------------------------------------------------------*/

enum iTidyReActionGadgetIndex
{
    /* Layout containers */
    ITIDY_GAD_IDX_MASTER_LAYOUT = 0,
    ITIDY_GAD_IDX_FOLDER_LAYOUT,
    ITIDY_GAD_IDX_OPTIONS_LAYOUT,
    ITIDY_GAD_IDX_LEFT_COLUMN,
    ITIDY_GAD_IDX_RIGHT_COLUMN,
    ITIDY_GAD_IDX_TOOLS_LAYOUT,
    ITIDY_GAD_IDX_BUTTONS_LAYOUT,
    
    /* Folder selection gadgets */
    ITIDY_GAD_IDX_FOLDER_GETFILE,
    
    /* Tidy options gadgets - Left column */
    ITIDY_GAD_IDX_ORDER_CHOOSER,
    ITIDY_GAD_IDX_RECURSIVE_CHECKBOX,
    ITIDY_GAD_IDX_POSITION_CHOOSER,
    
    /* Tidy options gadgets - Right column */
    ITIDY_GAD_IDX_SORTBY_CHOOSER,
    ITIDY_GAD_IDX_BACKUP_CHECKBOX,
    ITIDY_GAD_IDX_CREATE_NEW_ICONS,
    
    /* Tool buttons */
    ITIDY_GAD_IDX_ADVANCED_BUTTON,
    ITIDY_GAD_IDX_DEFAULT_TOOLS_BUTTON,
    ITIDY_GAD_IDX_RESTORE_BUTTON,
    ITIDY_GAD_IDX_ICON_CREATION_BUTTON,
    
    /* Action buttons */
    ITIDY_GAD_IDX_START_BUTTON,
    ITIDY_GAD_IDX_EXIT_BUTTON,
    
    /* Count of gadget indices (must be last) */
    ITIDY_GAD_IDX_COUNT
};

/*------------------------------------------------------------------------*/
/* ReAction Gadget IDs (for event handling)                              */
/* These IDs are assigned to gadgets via GA_ID tag and are used in       */
/* the WMHI_GADGETUP event handling to identify which gadget was clicked */
/*------------------------------------------------------------------------*/

#define GID_MAIN_FOLDER_GETFILE     1000
#define GID_MAIN_ORDER_CHOOSER      1001
#define GID_MAIN_SORTBY_CHOOSER     1002
#define GID_MAIN_RECURSIVE_CHECKBOX 1003
#define GID_MAIN_BACKUP_CHECKBOX    1004
#define GID_MAIN_CREATE_NEW_ICONS   1005
#define GID_MAIN_POSITION_CHOOSER   1006
#define GID_MAIN_ADVANCED_BUTTON    1007
#define GID_MAIN_DEFAULT_TOOLS_BTN  1008
#define GID_MAIN_RESTORE_BUTTON     1009
#define GID_MAIN_ICON_CREATION_BTN  1012
#define GID_MAIN_START_BUTTON       1010
#define GID_MAIN_EXIT_BUTTON        1011

/*------------------------------------------------------------------------*/
/* ReAction Gadget IDs (ITIDY_GAID_* aliases for hint system)           */
/* These are used with GA_ID tags during gadget creation                 */
/*------------------------------------------------------------------------*/

#define ITIDY_GAID_MASTER_LAYOUT        1100
#define ITIDY_GAID_FOLDER_LAYOUT        1101
#define ITIDY_GAID_OPTIONS_LAYOUT       1102
#define ITIDY_GAID_LEFT_COLUMN          1103
#define ITIDY_GAID_RIGHT_COLUMN         1104
#define ITIDY_GAID_TOOLS_LAYOUT         1105
#define ITIDY_GAID_BUTTONS_LAYOUT       1106

#define ITIDY_GAID_FOLDER_GETFILE       GID_MAIN_FOLDER_GETFILE
#define ITIDY_GAID_ORDER_CHOOSER        GID_MAIN_ORDER_CHOOSER
#define ITIDY_GAID_SORTBY_CHOOSER       GID_MAIN_SORTBY_CHOOSER
#define ITIDY_GAID_RECURSIVE_CHECKBOX   GID_MAIN_RECURSIVE_CHECKBOX
#define ITIDY_GAID_BACKUP_CHECKBOX      GID_MAIN_BACKUP_CHECKBOX
#define ITIDY_GAID_CREATE_NEW_ICONS     GID_MAIN_CREATE_NEW_ICONS
#define ITIDY_GAID_POSITION_CHOOSER     GID_MAIN_POSITION_CHOOSER
#define ITIDY_GAID_ADVANCED_BUTTON      GID_MAIN_ADVANCED_BUTTON
#define ITIDY_GAID_DEFAULT_TOOLS_BUTTON GID_MAIN_DEFAULT_TOOLS_BTN
#define ITIDY_GAID_RESTORE_BUTTON           GID_MAIN_RESTORE_BUTTON
#define ITIDY_GAID_ICON_CREATION_BUTTON     GID_MAIN_ICON_CREATION_BTN
#define ITIDY_GAID_START_BUTTON             GID_MAIN_START_BUTTON
#define ITIDY_GAID_EXIT_BUTTON              GID_MAIN_EXIT_BUTTON

/*------------------------------------------------------------------------*/
/* Menu Item IDs                                                         */
/*------------------------------------------------------------------------*/

/* Presets menu */
#define MENU_PRESETS_RESET              2001
#define MENU_PRESETS_OPEN               2002
#define MENU_PRESETS_SAVE               2003
#define MENU_PRESETS_SAVE_AS            2004
#define MENU_PRESETS_QUIT               2005

/* Settings menu */
#define MENU_SETTINGS_ADVANCED          3001
#define MENU_SETTINGS_DEFI_CATS         3002
#define MENU_SETTINGS_DEFI_PREVIEW      3003
#define MENU_SETTINGS_DEFI_EXCLUDE      3004
#define MENU_SETTINGS_BACKUP_TOGGLE     3005
#define MENU_SETTINGS_LOG_DISABLED      3006
#define MENU_SETTINGS_LOG_DEBUG         3007
#define MENU_SETTINGS_LOG_INFO          3008
#define MENU_SETTINGS_LOG_WARNING       3009
#define MENU_SETTINGS_LOG_ERROR         3010
#define MENU_SETTINGS_LOG_FOLDER        3011
#define MENU_SETTINGS_LOG_ARCHIVE       3012

/* Tools menu */
#define MENU_TOOLS_RESTORE_LAYOUTS      4001
#define MENU_TOOLS_RESTORE_DEFTOOLS     4002

/* Help menu */
#define MENU_HELP_GUIDE                 5001
#define MENU_HELP_ABOUT                 5002

#endif /* ITIDY_MAIN_WINDOW_REACTION_H */
