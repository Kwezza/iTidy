/*
 * main_window_reaction.h - iTidy Main Window ReAction Gadget IDs
 * ReAction GUI gadget index definitions for Workbench 3.2+
 */

#ifndef ITIDY_MAIN_WINDOW_REACTION_H
#define ITIDY_MAIN_WINDOW_REACTION_H

/*------------------------------------------------------------------------*/
/* Gadget Array Indices - match ReBuild designer enum names exactly      */
/* from Tests/ReActon/testcode.c (void main_window, enum main_window_idx)*/
/* IMPORTANT: Values are explicit to preserve current storage slots.     */
/* Do NOT reorder without updating all gadgets[] accesses in main_window.c*/
/*------------------------------------------------------------------------*/

enum main_window_idx
{
    /* Layout containers */
    main_root_layout             = 0,   /* master vertical container    */
    main_target_path_layout      = 1,   /* folder selection group       */
    main_options_layout          = 2,   /* options horizontal group     */
    main_options_left_col_layout = 3,   /* left column (choosers)       */
    main_options_right_col_layout= 4,   /* right column (checkboxes)    */
    main_tools_layout            = 5,   /* tool buttons group           */
    main_action_buttons_layout   = 6,   /* Start / Exit buttons group   */

    /* Folder selection gadget */
    main_gf_target_path          = 7,   /* GetFile gadget               */

    /* Left column gadgets */
    main_ch_sort_primary         = 8,   /* "Grouping" chooser           */
    main_cb_cleanup_subfolders   = 9,   /* "Include Subfolders" checkbox */
    main_ch_positioning          = 10,  /* "Window Position" chooser    */

    /* Right column gadgets */
    main_ch_sort_secondary       = 11,  /* "Sort By" chooser            */
    main_cb_backup_icons         = 12,  /* "Back Up Layout" checkbox    */
    main_cb_create_new_icons     = 13,  /* "Create Icons" checkbox      */

    /* Tool buttons */
    main_btn_advanced            = 14,  /* "Advanced..." button         */
    main_btn_default_tools       = 15,  /* "Fix Default Tools..." button*/
    main_btn_restore_backups     = 16,  /* "Restore Backups..." button  */
    main_btn_icon_settings       = 17,  /* "Icon Creation..." button    */

    /* Action buttons */
    main_btn_start               = 18,  /* "Start" button               */
    main_btn_exit                = 19,  /* "Exit" button                */

    /* Array size sentinel (must be last) */
    MAIN_WINDOW_GAD_COUNT        = 20
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

/* Tools menu */
#define MENU_TOOLS_RESTORE_LAYOUTS      4001
#define MENU_TOOLS_RESTORE_DEFTOOLS     4002

/* Help menu */
#define MENU_HELP_GUIDE                 5001
#define MENU_HELP_ABOUT                 5002

#endif /* ITIDY_MAIN_WINDOW_REACTION_H */
