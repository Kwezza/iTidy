/*
 * main_window.h - iTidy Main Window Header
 * GadTools-based GUI for Workbench 3.0+
 * No MUI - Pure Intuition + GadTools
 */

#ifndef ITIDY_MAIN_WINDOW_H
#define ITIDY_MAIN_WINDOW_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>

/*------------------------------------------------------------------------*/
/* Gadget IDs                                                             */
/*------------------------------------------------------------------------*/
#define GID_FOLDER_PATH      1
#define GID_BROWSE           2
#define GID_ORDER            6
#define GID_SORTBY           7
#define GID_RECURSIVE        10
#define GID_BACKUP           11
#define GID_ADVANCED         14
#define GID_APPLY            15
#define GID_CANCEL           16
#define GID_RESTORE          17
#define GID_VIEW_TOOL_CACHE  18
#define GID_WINDOW_POSITION  21
#define GID_WINDOW_POSITION_HELP 22

/*------------------------------------------------------------------------*/
/* Main Window Data Structure                                            */
/*------------------------------------------------------------------------*/
struct iTidyMainWindow
{
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Main window */
    APTR visual_info;                   /* GadTools visual info */
    struct Gadget *glist;               /* Gadget list */
    BOOL window_open;                   /* Window state flag */
    
    /* Gadget pointers for easy access */
    struct Gadget *folder_label;
    struct Gadget *folder_path;
    struct Gadget *browse_btn;
    struct Gadget *order_cycle;
    struct Gadget *sortby_cycle;
    struct Gadget *recursive_check;
    struct Gadget *backup_check;
    struct Gadget *advanced_btn;
    struct Gadget *restore_btn;
    struct Gadget *apply_btn;
    struct Gadget *cancel_btn;
    struct Gadget *view_tool_cache_btn;
    struct Gadget *window_position_label;
    struct Gadget *window_position_cycle;
    struct Gadget *window_position_help_btn;
    
    /* Temporary GUI state (for gadget selections only) */
    WORD order_selected;
    WORD sortby_selected;
    BOOL recursive_subdirs;
    BOOL enable_backup;
    WORD window_position_selected;
    
    /* Temporary folder path buffer (for string gadget only) */
    char folder_path_buffer[256];
    
    /* Folder path display box coordinates (for custom drawing) */
    WORD folder_box_left;
    WORD folder_box_top;
    WORD folder_box_width;
    WORD folder_box_height;
    
    /* Advanced settings override flag */
    BOOL has_advanced_settings;         /* TRUE if user configured advanced settings */
    
    /* Group box rectangles (for layout calculations) */
    struct Rectangle folder_group_box;
    struct Rectangle tidy_options_group_box;
    struct Rectangle tools_group_box;
};

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Open the iTidy main window on Workbench screen
 *
 * @param win_data Pointer to window data structure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
BOOL open_itidy_main_window(struct iTidyMainWindow *win_data);

/**
 * @brief Close the iTidy main window and cleanup resources
 *
 * @param win_data Pointer to window data structure
 */
void close_itidy_main_window(struct iTidyMainWindow *win_data);

/**
 * @brief Handle window events (main event loop)
 *
 * @param win_data Pointer to window data structure
 * @return BOOL TRUE to continue, FALSE to quit
 */
BOOL handle_itidy_window_events(struct iTidyMainWindow *win_data);

#endif /* ITIDY_MAIN_WINDOW_H */
