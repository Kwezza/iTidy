/**
 * default_tool_restore_window.h - Default Tool Restore Window Interface
 * 
 * Provides UI for browsing and restoring previous default tool changes
 * from CSV backup sessions. Uses a two-ListView design:
 *   - Top ListView: Shows available backup sessions
 *   - Bottom ListView: Shows tool changes within selected session
 * 
 * Based on amiga_window_template.c and AI_AGENT_GETTING_STARTED.md guidelines.
 */

#ifndef DEFAULT_TOOL_RESTORE_WINDOW_H
#define DEFAULT_TOOL_RESTORE_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include "default_tool_backup.h"

/*========================================================================*/
/* Constants                                                              */
/*========================================================================*/

/* Window Layout */
#define RESTORE_WINDOW_WIDTH_CHARS 70          /* Window width in characters */
#define RESTORE_WINDOW_MARGIN_LEFT 5           /* Left margin */
#define RESTORE_WINDOW_MARGIN_TOP 5            /* Top margin */
#define RESTORE_WINDOW_MARGIN_RIGHT 5          /* Right margin */
#define RESTORE_WINDOW_MARGIN_BOTTOM 5         /* Bottom margin */
#define RESTORE_WINDOW_SPACE_X 8               /* Horizontal spacing between gadgets */
#define RESTORE_WINDOW_SPACE_Y 8               /* Vertical spacing between gadgets */

/* ListView Dimensions */
#define RESTORE_SESSION_LIST_LINES 6           /* Lines in session ListView */
#define RESTORE_TOOL_CHANGE_LIST_LINES 4       /* Lines in tool change ListView */

/* Button Dimensions */
#define RESTORE_BUTTON_PADDING 16              /* Extra padding for button width */

/*========================================================================*/
/* Gadget IDs                                                             */
/*========================================================================*/
typedef enum {
    GID_RESTORE_SESSION_LISTVIEW = 1000,
    GID_RESTORE_TOOL_CHANGE_LISTVIEW,
    GID_RESTORE_ALL_BTN,
    GID_RESTORE_SELECTED_BTN,
    GID_RESTORE_DELETE_BTN,
    GID_RESTORE_CLOSE_BTN
} RestoreWindowGadgetID;

/*========================================================================*/
/* Restore Window Data Structure                                         */
/*========================================================================*/
/**
 * @brief Main data structure for the restore window
 * 
 * Contains all UI elements, lists, and state for the restore window.
 * Follows the pattern from amiga_window_template.c.
 */
typedef struct iTidy_DefaultToolRestoreWindow {
    /* Window and Screen */
    struct Window *window;                     /* Main window */
    struct Screen *screen;                     /* Locked Workbench screen */
    APTR visual_info;                          /* VisualInfo for gadgets */
    BOOL window_open;                          /* TRUE if window is open */
    
    /* Gadgets */
    struct Gadget *glist;                      /* Gadget list */
    struct Gadget *session_listview;           /* Session ListView */
    struct Gadget *tool_change_listview;       /* Tool change ListView */
    struct Gadget *restore_all_btn;            /* "Restore All" button */
    struct Gadget *restore_selected_btn;       /* "Restore Selected" button */
    struct Gadget *delete_btn;                 /* "Delete" button */
    struct Gadget *close_btn;                  /* "Close" button */
    
    /* Data Lists */
    struct List session_list;                  /* List of iTidy_ToolBackupSession */
    struct List tool_change_list;              /* List of iTidy_ToolChange */
    
    /* Selection State */
    char selected_session_id[32];             /* Currently selected session ID */
    LONG selected_session_index;              /* Index in session_list (-1 = none) */
    LONG selected_tool_change_index;          /* Index in tool_change_list (-1 = none) */
    
    /* Font Handling */
    struct TextAttr system_font_attr;         /* System font attributes (if needed) */
    struct TextFont *system_font;             /* Opened system font (if needed) */
    
    /* Window Title */
    char window_title[128];                   /* Window title buffer */
} iTidy_DefaultToolRestoreWindow;

/*========================================================================*/
/* Function Prototypes                                                    */
/*========================================================================*/

/**
 * @brief Open the default tool restore window
 * 
 * Creates and displays the restore window with session list populated.
 * Follows the pattern from create_template_window() in amiga_window_template.c.
 * 
 * @param data Pointer to window data structure (must be zeroed)
 * @return TRUE if successful, FALSE on error
 */
BOOL iTidy_OpenDefaultToolRestoreWindow(struct iTidy_DefaultToolRestoreWindow *data);

/**
 * @brief Close the default tool restore window
 * 
 * Properly cleans up all resources including gadgets, lists, and window.
 * Follows the cleanup order from close_template_window() in amiga_window_template.c.
 * 
 * @param data Pointer to window data structure
 */
void iTidy_CloseDefaultToolRestoreWindow(struct iTidy_DefaultToolRestoreWindow *data);

/**
 * @brief Handle window events
 * 
 * Processes IDCMP events for the restore window. Returns FALSE when
 * the window should close.
 * 
 * @param data Pointer to window data structure
 * @return TRUE to continue, FALSE to close window
 */
BOOL iTidy_HandleDefaultToolRestoreEvents(struct iTidy_DefaultToolRestoreWindow *data);

/**
 * @brief Refresh the session ListView
 * 
 * Reloads the list of available backup sessions and updates the display.
 * 
 * @param data Pointer to window data structure
 */
void iTidy_RefreshSessionList(struct iTidy_DefaultToolRestoreWindow *data);

/**
 * @brief Populate the tool change ListView for selected session
 * 
 * Loads tool changes from the selected session and displays them in
 * the bottom ListView.
 * 
 * @param data Pointer to window data structure
 */
void iTidy_PopulateToolChangeList(struct iTidy_DefaultToolRestoreWindow *data);

#endif /* DEFAULT_TOOL_RESTORE_WINDOW_H */
