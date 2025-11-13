/*
 * default_tool_update_window.h - Default Tool Replacement Window Header
 * Provides UI for batch or single icon default tool replacement
 * Works with tool cache to replace default tools across multiple icons
 */

#ifndef ITIDY_DEFAULT_TOOL_UPDATE_WINDOW_H
#define ITIDY_DEFAULT_TOOL_UPDATE_WINDOW_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

/*------------------------------------------------------------------------*/
/* Gadget IDs                                                             */
/*------------------------------------------------------------------------*/
#define GID_TOOL_UPDATE_BASE            5000

#define GID_TOOL_NEW_PATH_STRING        (GID_TOOL_UPDATE_BASE + 1)
#define GID_TOOL_BROWSE_BTN             (GID_TOOL_UPDATE_BASE + 2)
#define GID_TOOL_BACKUP_CHECKBOX        (GID_TOOL_UPDATE_BASE + 3)
#define GID_TOOL_UPDATE_BTN             (GID_TOOL_UPDATE_BASE + 4)
#define GID_TOOL_STATUS_LISTVIEW        (GID_TOOL_UPDATE_BASE + 5)
#define GID_TOOL_CLOSE_BTN              (GID_TOOL_UPDATE_BASE + 6)

/*------------------------------------------------------------------------*/
/* Window Layout Constants                                                */
/*------------------------------------------------------------------------*/
#define TOOL_UPDATE_WINDOW_WIDTH_CHARS  65   /* Medium window size */
#define TOOL_UPDATE_WINDOW_MARGIN_LEFT  10
#define TOOL_UPDATE_WINDOW_MARGIN_TOP   10
#define TOOL_UPDATE_WINDOW_MARGIN_RIGHT 10
#define TOOL_UPDATE_WINDOW_MARGIN_BOTTOM 10
#define TOOL_UPDATE_WINDOW_SPACE_X      8
#define TOOL_UPDATE_WINDOW_SPACE_Y      8
#define TOOL_UPDATE_BUTTON_PADDING      8

#define TOOL_UPDATE_STATUS_LINES        6    /* Visible lines in status listview */
#define TOOL_UPDATE_PATH_INPUT_CHARS    35   /* Width of path input field */

/* Status column widths for fixed-width font listview */
#define STATUS_ICON_PATH_WIDTH          45   /* Characters for icon path column */
#define STATUS_TEXT_WIDTH               10   /* Characters for status text column */

/*------------------------------------------------------------------------*/
/* Update Mode                                                            */
/*------------------------------------------------------------------------*/
typedef enum {
    UPDATE_MODE_BATCH,   /* Batch update all icons for a tool (from upper listview) */
    UPDATE_MODE_SINGLE   /* Update single .info file (from lower listview) */
} iTidy_UpdateMode;

/*------------------------------------------------------------------------*/
/* Update Context - Passed when opening window                           */
/*------------------------------------------------------------------------*/
struct iTidy_DefaultToolUpdateContext {
    iTidy_UpdateMode mode;           /* Batch or single mode */
    char *current_tool;              /* Current default tool name (for display) */
    
    /* For batch mode: */
    int icon_count;                  /* Number of icons that will be updated */
    char **icon_paths;               /* Array of .info file paths (from g_ToolCache) */
    
    /* For single mode: */
    char *single_info_path;          /* Single .info file path */
};

/*------------------------------------------------------------------------*/
/* Status List Entry - Displayed in status listview                      */
/*------------------------------------------------------------------------*/
struct iTidy_StatusEntry {
    struct Node node;                /* For Exec list */
    char *icon_path;                 /* Icon path (truncated for display) */
    char *status_text;               /* Status: SUCCESS, FAILED, READ-ONLY, etc. */
    char *display_text;              /* Full formatted text for listview */
};

/*------------------------------------------------------------------------*/
/* Window Data Structure                                                  */
/*------------------------------------------------------------------------*/
struct iTidy_DefaultToolUpdateWindow {
    /* System resources */
    struct Screen *screen;           /* Workbench screen */
    struct Window *window;           /* Window pointer */
    struct Gadget *glist;            /* Gadget list */
    APTR visual_info;                /* Visual info for GadTools */
    struct TextFont *system_font;    /* System default font (for columnar data) */
    struct TextAttr system_font_attr;/* Font attributes */
    BOOL window_open;                /* Window state flag */
    
    /* Gadgets */
    struct Gadget *current_tool_text;    /* Shows current default tool (TEXT_KIND) */
    struct Gadget *mode_text;            /* Shows mode and icon count (TEXT_KIND) */
    struct Gadget *change_to_label;      /* "Change to:" label (TEXT_KIND) */
    struct Gadget *new_path_string;      /* New tool path (STRING_KIND, read-only) */
    struct Gadget *browse_btn;           /* Browse button (BUTTON_KIND) */
    struct Gadget *backup_checkbox;      /* Backup checkbox (CHECKBOX_KIND, disabled for now) */
    struct Gadget *update_btn;           /* Update button (BUTTON_KIND) */
    struct Gadget *status_listview;      /* Status listview (LISTVIEW_KIND) */
    struct Gadget *close_btn;            /* Close button (BUTTON_KIND) */
    
    /* Data */
    struct iTidy_DefaultToolUpdateContext context;  /* Update context */
    struct List status_list;         /* List of iTidy_StatusEntry */
    char new_tool_path[256];         /* New tool path buffer */
    char window_title[128];          /* Window title */
    char current_tool_label[256];    /* Formatted "Current tool: xxx" label */
    char mode_label[256];            /* Formatted mode text label */
    BOOL update_in_progress;         /* Flag to prevent concurrent updates */
};

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Open the Default Tool Update window
 * 
 * Opens a window for replacing default tools in icons. Supports two modes:
 * - Batch mode: Updates all icons using a specific tool
 * - Single mode: Updates a single .info file
 * 
 * @param data Window data structure (must be allocated by caller)
 * @param context Update context with mode, tool info, and icon paths
 * @return TRUE if window opened successfully, FALSE on error
 */
BOOL iTidy_OpenDefaultToolUpdateWindow(struct iTidy_DefaultToolUpdateWindow *data,
                                        struct iTidy_DefaultToolUpdateContext *context);

/**
 * @brief Close the Default Tool Update window
 * 
 * Cleans up all resources associated with the window.
 * 
 * @param data Window data structure
 */
void iTidy_CloseDefaultToolUpdateWindow(struct iTidy_DefaultToolUpdateWindow *data);

/**
 * @brief Handle window events
 * 
 * Processes IDCMP events for the Default Tool Update window.
 * 
 * @param data Window data structure
 * @return TRUE to continue, FALSE to close window
 */
BOOL iTidy_HandleDefaultToolUpdateEvents(struct iTidy_DefaultToolUpdateWindow *data);

/**
 * @brief Free status list entries
 * 
 * Frees all entries in the status listview.
 * 
 * @param data Window data structure
 */
void iTidy_FreeStatusEntries(struct iTidy_DefaultToolUpdateWindow *data);

#endif /* ITIDY_DEFAULT_TOOL_UPDATE_WINDOW_H */
