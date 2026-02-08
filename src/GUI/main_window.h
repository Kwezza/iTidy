/*
 * main_window.h - iTidy Main Window Header
 * ReAction GUI for Workbench 3.2+
 */

#ifndef ITIDY_MAIN_WINDOW_H
#define ITIDY_MAIN_WINDOW_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>
#include <libraries/gadtools.h>

/* ReAction gadget ID definitions */
#include "main_window_reaction.h"

/*------------------------------------------------------------------------*/
/* ReAction Library Bases (extern declarations)                           */
/* Actual variables are defined in main_window.c                          */
/*------------------------------------------------------------------------*/

extern struct Library *WindowBase;
extern struct Library *LayoutBase;
extern struct Library *ButtonBase;
extern struct Library *CheckBoxBase;
extern struct Library *ChooserBase;
extern struct Library *GetFileBase;
extern struct Library *LabelBase;

/*------------------------------------------------------------------------*/
/* Main Window Data Structure                                            */
/*------------------------------------------------------------------------*/

struct iTidyMainWindow
{
    /* Screen and window handles */
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Intuition window (from RA_OpenWindow) */
    Object *window_obj;                 /* ReAction window object */
    APTR visual_info;                   /* GadTools visual info (for menus) */
    struct MsgPort *app_port;           /* Application message port */
    BOOL window_open;                   /* Window state flag */
    
    /* Gadget array (ReAction style) */
    struct Gadget *gadgets[ITIDY_GAD_IDX_COUNT];
    
    /* Chooser label lists (must be freed on cleanup) */
    struct List *order_labels;          /* Order chooser labels */
    struct List *sortby_labels;         /* Sort by chooser labels */
    struct List *position_labels;       /* Window position chooser labels */
    
    /* Menu system */
    struct Menu *menu_strip;            /* GadTools menu strip */
    
    /* GUI state (selections from gadgets) */
    WORD order_selected;                /* Order chooser selection */
    WORD sortby_selected;               /* Sort by chooser selection */
    BOOL recursive_subdirs;             /* Cleanup subfolders checkbox */
    BOOL enable_backup;                 /* Backup icons checkbox */
    BOOL enable_deficons_icon_creation; /* Create new icons checkbox (DefIcons feature) */
    WORD window_position_selected;      /* Window position chooser selection */
    
    /* Folder path buffer */
    char folder_path_buffer[256];
    
    /* Last save path (for Project menu Save/Save As) */
    char last_save_path[256];
};

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Initialize ReAction libraries
 * 
 * Opens all ReAction class libraries needed for the GUI.
 * Must be called before any window operations.
 * 
 * @return BOOL TRUE if all libraries opened successfully
 */
BOOL init_reaction_libs(void);

/**
 * @brief Cleanup ReAction libraries
 * 
 * Closes all ReAction class libraries.
 * Must be called after all windows are closed.
 */
void cleanup_reaction_libs(void);

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
