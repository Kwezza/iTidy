/*
 * beta_options_window.h - iTidy Beta Options Window Header
 * GadTools-based GUI for Workbench 3.0+
 * Experimental Feature Configuration
 */

#ifndef ITIDY_BETA_OPTIONS_WINDOW_H
#define ITIDY_BETA_OPTIONS_WINDOW_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include "layout_preferences.h"

/*------------------------------------------------------------------------*/
/* Gadget IDs                                                             */
/*------------------------------------------------------------------------*/
#define GID_BETA_OPEN_FOLDERS       2001
#define GID_BETA_UPDATE_WINDOWS     2002
#define GID_BETA_LOG_LEVEL          2003
#define GID_BETA_MEMORY_LOGGING     2004
#define GID_BETA_PERFORMANCE_LOG    2005
#define GID_BETA_OK                 2006
#define GID_BETA_CANCEL             2007

/*------------------------------------------------------------------------*/
/* Beta Options Window Data Structure                                    */
/*------------------------------------------------------------------------*/
struct iTidyBetaOptionsWindow
{
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Beta options window */
    APTR visual_info;                   /* GadTools visual info */
    struct Gadget *glist;               /* Gadget list */
    BOOL window_open;                   /* Window state flag */
    
    /* Gadget pointers for easy access */
    struct Gadget *open_folders_check;
    struct Gadget *update_windows_check;
    struct Gadget *log_level_cycle;
    struct Gadget *memory_logging_check;
    struct Gadget *performance_logging_check;
    struct Gadget *ok_btn;
    struct Gadget *cancel_btn;
    
    /* Current settings */
    BOOL open_folders_enabled;          /* beta_openFoldersAfterProcessing */
    BOOL update_windows_enabled;        /* beta_FindWindowOnWorkbenchAndUpdate */
    UWORD log_level_selected;           /* Log level: 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR */
    BOOL memory_logging_enabled;        /* Enable memory allocation logging */
    BOOL performance_logging_enabled;   /* Enable performance timing logging */
    
    /* Pointer to preferences to update */
    LayoutPreferences *prefs;
    
    /* Result flag */
    BOOL changes_accepted;              /* TRUE if OK clicked, FALSE if Cancel */
};

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Open the iTidy Beta Options window
 *
 * Opens a modal window for configuring experimental beta features.
 * The window is positioned on the Workbench screen and blocks
 * interaction with the main window until closed.
 *
 * @param beta_data Pointer to beta options window data structure
 * @param prefs Pointer to LayoutPreferences to configure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
BOOL open_itidy_beta_options_window(struct iTidyBetaOptionsWindow *beta_data, 
                                     LayoutPreferences *prefs);

/**
 * @brief Close the iTidy Beta Options window and cleanup resources
 *
 * Frees all gadgets, visual info, and closes the window. Does not modify
 * preferences unless changes_accepted flag is TRUE.
 *
 * @param beta_data Pointer to beta options window data structure
 */
void close_itidy_beta_options_window(struct iTidyBetaOptionsWindow *beta_data);

/**
 * @brief Handle beta options window events (main event loop)
 *
 * Processes window events including gadget interactions, close requests,
 * and refresh events. Returns FALSE when window should be closed.
 *
 * @param beta_data Pointer to beta options window data structure
 * @return BOOL TRUE to continue, FALSE to close window
 */
BOOL handle_beta_options_window_events(struct iTidyBetaOptionsWindow *beta_data);

/**
 * @brief Load current preferences into beta options window gadgets
 *
 * Reads values from LayoutPreferences structure and updates all gadgets
 * to reflect current settings. Called after window opens.
 *
 * @param beta_data Pointer to beta options window data structure
 */
void load_preferences_to_beta_options_window(struct iTidyBetaOptionsWindow *beta_data);

/**
 * @brief Save beta options window gadget values to preferences
 *
 * Reads all gadget values and updates the LayoutPreferences structure.
 * Only called when OK button is clicked.
 *
 * @param beta_data Pointer to beta options window data structure
 */
void save_beta_options_window_to_preferences(struct iTidyBetaOptionsWindow *beta_data);

#endif /* ITIDY_BETA_OPTIONS_WINDOW_H */
