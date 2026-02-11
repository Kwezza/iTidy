/*
 * main_window_reaction.h - ReAction GUI ID Definitions for iTidy
 * 
 * This header maps ReAction gadget IDs exported from the GUI design tool
 * (ReBuild) to iTidy's internal naming conventions.
 * 
 * IMPORTANT: The numeric ID values come from the GUI tool export and
 * should NOT be changed manually. When updating the GUI in ReBuild,
 * re-export and update the values here if they change.
 * 
 * Workbench 3.2+ ReAction GUI
 */

#ifndef ITIDY_MAIN_WINDOW_REACTION_H
#define ITIDY_MAIN_WINDOW_REACTION_H

/*------------------------------------------------------------------------*/
/* ReAction Gadget Array Indices                                          */
/* These are used to access gadgets via main_gadgets[index]               */
/*------------------------------------------------------------------------*/

typedef enum {
    ITIDY_GAD_IDX_MASTER_LAYOUT = 0,     /* Root vertical layout */
    ITIDY_GAD_IDX_FOLDER_LAYOUT,          /* Folder selection group */
    ITIDY_GAD_IDX_FOLDER_GETFILE,         /* GetFile gadget for folder path */
    ITIDY_GAD_IDX_OPTIONS_LAYOUT,         /* Main options group (horizontal) */
    ITIDY_GAD_IDX_LEFT_COLUMN,            /* Left options column */
    ITIDY_GAD_IDX_ORDER_CHOOSER,          /* Order chooser (Folders First, etc.) */
    ITIDY_GAD_IDX_RECURSIVE_CHECKBOX,     /* Cleanup subfolders checkbox */
    ITIDY_GAD_IDX_POSITION_CHOOSER,       /* Window position chooser */
    ITIDY_GAD_IDX_RIGHT_COLUMN,           /* Right options column */
    ITIDY_GAD_IDX_SORTBY_CHOOSER,         /* Sort by chooser (Name, Type, etc.) */
    ITIDY_GAD_IDX_BACKUP_CHECKBOX,        /* Backup icons checkbox */
    ITIDY_GAD_IDX_CREATE_NEW_ICONS,       /* Create new icons checkbox */
    ITIDY_GAD_IDX_TOOLS_LAYOUT,           /* Tools button group */
    ITIDY_GAD_IDX_ADVANCED_BUTTON,        /* Advanced settings button */
    ITIDY_GAD_IDX_DEFAULT_TOOLS_BUTTON,   /* Fix default tools button */
    ITIDY_GAD_IDX_RESTORE_BUTTON,         /* Restore backups button */
    ITIDY_GAD_IDX_BUTTONS_LAYOUT,         /* Bottom buttons group */
    ITIDY_GAD_IDX_START_BUTTON,           /* Start/Apply button */
    ITIDY_GAD_IDX_EXIT_BUTTON,            /* Exit/Cancel button */
    ITIDY_GAD_IDX_COUNT                   /* Total gadget count */
} iTidyGadgetIndex;

/*------------------------------------------------------------------------*/
/* ReAction GA_ID Values for WMHI_GADGETUP Events                         */
/* IMPORTANT: These values come from the GUI tool export!                 */
/*------------------------------------------------------------------------*/

#define ITIDY_GAID_MASTER_LAYOUT          6
#define ITIDY_GAID_FOLDER_LAYOUT          28
#define ITIDY_GAID_FOLDER_GETFILE         52
#define ITIDY_GAID_OPTIONS_LAYOUT         26
#define ITIDY_GAID_LEFT_COLUMN            33
#define ITIDY_GAID_ORDER_CHOOSER          35
#define ITIDY_GAID_RECURSIVE_CHECKBOX     36
#define ITIDY_GAID_POSITION_CHOOSER       37
#define ITIDY_GAID_RIGHT_COLUMN           34
#define ITIDY_GAID_SORTBY_CHOOSER         38
#define ITIDY_GAID_BACKUP_CHECKBOX        39
#define ITIDY_GAID_CREATE_NEW_ICONS       40
#define ITIDY_GAID_TOOLS_LAYOUT           45
#define ITIDY_GAID_ADVANCED_BUTTON        46
#define ITIDY_GAID_DEFAULT_TOOLS_BUTTON   47
#define ITIDY_GAID_RESTORE_BUTTON         48
#define ITIDY_GAID_BUTTONS_LAYOUT         49
#define ITIDY_GAID_START_BUTTON           50
#define ITIDY_GAID_EXIT_BUTTON            51

/*------------------------------------------------------------------------*/
/* Backward Compatibility Mapping                                         */
/* Maps old GadTools GID_* names to new ReAction IDs                      */
/* This allows existing event handler code to work with minimal changes   */
/*------------------------------------------------------------------------*/

#define GID_FOLDER_PATH           ITIDY_GAID_FOLDER_GETFILE
#define GID_BROWSE                ITIDY_GAID_FOLDER_GETFILE  /* Integrated into GetFile */
#define GID_ORDER                 ITIDY_GAID_ORDER_CHOOSER
#define GID_SORTBY                ITIDY_GAID_SORTBY_CHOOSER
#define GID_RECURSIVE             ITIDY_GAID_RECURSIVE_CHECKBOX
#define GID_BACKUP                ITIDY_GAID_BACKUP_CHECKBOX
#define GID_ADVANCED              ITIDY_GAID_ADVANCED_BUTTON
#define GID_APPLY                 ITIDY_GAID_START_BUTTON
#define GID_CANCEL                ITIDY_GAID_EXIT_BUTTON
#define GID_RESTORE               ITIDY_GAID_RESTORE_BUTTON
#define GID_VIEW_TOOL_CACHE       ITIDY_GAID_DEFAULT_TOOLS_BUTTON
#define GID_WINDOW_POSITION       ITIDY_GAID_POSITION_CHOOSER

/*------------------------------------------------------------------------*/
/* Menu Item IDs (unchanged from GadTools version)                        */
/*------------------------------------------------------------------------*/

#define MENU_PROJECT_NEW          1001
#define MENU_PROJECT_OPEN         1002
#define MENU_PROJECT_SAVE         1003
#define MENU_PROJECT_SAVE_AS      1004
#define MENU_PROJECT_CLOSE        1005
#define MENU_PROJECT_ABOUT        1006
#define MENU_PROJECT_DEFICONS     1007

/* Log mode menu IDs */
#define MENU_LOG_DISABLED         2001
#define MENU_LOG_DEBUG            2002
#define MENU_LOG_INFO             2003
#define MENU_LOG_WARNING          2004
#define MENU_LOG_ERROR            2005

#endif /* ITIDY_MAIN_WINDOW_REACTION_H */
