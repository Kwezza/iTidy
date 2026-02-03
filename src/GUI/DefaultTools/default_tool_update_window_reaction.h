/*
 * default_tool_update_window_reaction.h - Default Tool Replacement Window Header (ReAction)
 * Provides UI for batch or single icon default tool replacement
 * Works with tool cache to replace default tools across multiple icons
 * ReAction-based window for Workbench 3.2+
 */

#ifndef ITIDY_DEFAULT_TOOL_UPDATE_WINDOW_REACTION_H
#define ITIDY_DEFAULT_TOOL_UPDATE_WINDOW_REACTION_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include "default_tool_backup.h"

/*------------------------------------------------------------------------*/
/* Gadget IDs (matching testcode.c enum default_tool_update_idx)         */
/* Base 6000 to avoid collision with tool_cache_window.h (base 4000)     */
/*------------------------------------------------------------------------*/
enum {
    GID_TOOL_UPDATE_MAIN_LAYOUT = 6000,
    GID_TOOL_UPDATE_CURRENT_TOOL_LAYOUT,
    GID_TOOL_UPDATE_LABEL_CURRENT_TOOL,
    GID_TOOL_UPDATE_MODE_LAYOUT,
    GID_TOOL_UPDATE_MODE_TEXT,
    GID_TOOL_UPDATE_CHANGE_TO_LAYOUT,
    GID_TOOL_UPDATE_NEW_TOOL_GETFILE,
    GID_TOOL_UPDATE_PROGRESS_LAYOUT,
    GID_TOOL_UPDATE_PROGRESS_LABEL,
    GID_TOOL_UPDATE_LISTVIEW_PROGRESS,
    GID_TOOL_UPDATE_BUTTONS_LAYOUT,
    GID_TOOL_UPDATE_UPDATE_BUTTON,
    GID_TOOL_UPDATE_CLOSE_BUTTON
};

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
    
    /* Parent window refresh callback */
    void *parent_window;             /* Pointer to parent iTidyToolCacheWindow (if any) */
};

/*------------------------------------------------------------------------*/
/* ListBrowser column IDs                                                 */
/*------------------------------------------------------------------------*/
#define LBCOL_STATUS    0    /* Status column (SUCCESS, FAILED, etc.) */
#define LBCOL_FILEPATH  1    /* Icon filepath column */

/*------------------------------------------------------------------------*/
/* Window Data Structure                                                  */
/*------------------------------------------------------------------------*/
struct iTidy_DefaultToolUpdateWindow_ReAction {
    /* System resources */
    struct Screen *screen;           /* Workbench screen */
    struct Window *window;           /* Intuition window pointer */
    Object *window_obj;              /* ReAction window object */
    
    /* ReAction gadget objects */
    Object *main_layout_obj;
    Object *current_tool_layout_obj;
    Object *label_current_tool_obj;  /* Label image (can be updated with RefreshSetGadgetAttrs) */
    Object *mode_layout_obj;
    Object *mode_text_obj;           /* Label image (can be updated with RefreshSetGadgetAttrs) */
    Object *change_to_layout_obj;
    Object *new_tool_getfile_obj;    /* GetFile gadget for tool selection */
    Object *update_progress_layout_obj;
    Object *update_progress_label_obj;
    Object *listbrowser_progress_obj;   /* ListBrowser gadget */
    Object *buttons_layout_obj;
    Object *update_button_obj;
    Object *close_button_obj;
    
    /* Data */
    struct iTidy_DefaultToolUpdateContext context;  /* Update context */
    struct List status_list;         /* List of ListBrowser nodes */
    char new_tool_path[256];         /* New tool path buffer */
    char current_tool_label[256];    /* Formatted "Current tool: xxx" label */
    char mode_label[256];            /* Formatted mode text label */
    BOOL update_in_progress;         /* Flag to prevent concurrent updates */
    BOOL window_open;                /* Window state flag */
    
    /* Backup System */
    struct iTidy_ToolBackupManager backup_manager;  /* Backup session manager */
};

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Open the Default Tool Update window (ReAction)
 * 
 * Opens a ReAction window for replacing default tools in icons. Supports two modes:
 * - Batch mode: Updates all icons using a specific tool
 * - Single mode: Updates a single .info file
 * 
 * @param data Window data structure (must be allocated by caller)
 * @param context Update context with mode, tool info, and icon paths
 * @return TRUE if window opened successfully, FALSE on error
 */
BOOL iTidy_OpenDefaultToolUpdateWindow_ReAction(struct iTidy_DefaultToolUpdateWindow_ReAction *data,
                                                 struct iTidy_DefaultToolUpdateContext *context);

/**
 * @brief Close the Default Tool Update window (ReAction)
 * 
 * Cleans up all resources associated with the ReAction window.
 * 
 * @param data Window data structure
 */
void iTidy_CloseDefaultToolUpdateWindow_ReAction(struct iTidy_DefaultToolUpdateWindow_ReAction *data);

/**
 * @brief Handle window events (ReAction)
 * 
 * Processes IDCMP events for the Default Tool Update window.
 * 
 * @param data Window data structure
 * @return TRUE to continue, FALSE to close window
 */
BOOL iTidy_HandleDefaultToolUpdateEvents_ReAction(struct iTidy_DefaultToolUpdateWindow_ReAction *data);

#endif /* ITIDY_DEFAULT_TOOL_UPDATE_WINDOW_REACTION_H */
