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
#define GID_PRESET           3
#define GID_LAYOUT           4
#define GID_SORT             5
#define GID_ORDER            6
#define GID_SORTBY           7
#define GID_CENTER_ICONS     8
#define GID_OPTIMIZE_COLS    9
#define GID_RECURSIVE        10
#define GID_BACKUP           11
#define GID_ICON_UPGRADE     12
#define GID_SKIP_HIDDEN      13
#define GID_ADVANCED         14
#define GID_APPLY            15
#define GID_CANCEL           16
#define GID_RESTORE          17
#define GID_ENUMERATE        18

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
    struct Gadget *folder_path;
    struct Gadget *browse_btn;
    struct Gadget *preset_cycle;
    struct Gadget *layout_cycle;
    struct Gadget *sort_cycle;
    struct Gadget *order_cycle;
    struct Gadget *sortby_cycle;
    struct Gadget *center_check;
    struct Gadget *optimize_check;
    struct Gadget *recursive_check;
    struct Gadget *backup_check;
    struct Gadget *iconupgrade_check;
    struct Gadget *skip_hidden_check;
    struct Gadget *advanced_btn;
    struct Gadget *restore_btn;
    struct Gadget *apply_btn;
    struct Gadget *cancel_btn;
    struct Gadget *enumerate_btn;
    
    /* Current settings */
    char folder_path_buffer[256];
    WORD preset_selected;
    WORD layout_selected;
    WORD sort_selected;
    WORD order_selected;
    WORD sortby_selected;
    BOOL center_icons;
    BOOL optimize_columns;
    BOOL recursive_subdirs;
    BOOL enable_backup;
    BOOL enable_icon_upgrade;
    BOOL skip_hidden_folders;
    
    /* Advanced settings (overrides preset defaults when set) */
    BOOL has_advanced_settings;         /* TRUE if user configured advanced settings */
    float advanced_aspect_ratio;
    BOOL advanced_use_custom_ratio;
    UWORD advanced_custom_width;
    UWORD advanced_custom_height;
    UWORD advanced_overflow_mode;
    UWORD advanced_spacing_x;
    UWORD advanced_spacing_y;
    UWORD advanced_min_icons_row;
    UWORD advanced_max_icons_row;
    UWORD advanced_max_width_pct;       /* Max window width percentage */
    UWORD advanced_vertical_align;      /* TextAlignment value */
    BOOL advanced_reverse_sort;         /* Reverse sort order (Z->A) */
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
