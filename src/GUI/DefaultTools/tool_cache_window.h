/*
 * tool_cache_window.h - iTidy Tool Cache Window Header
 * Displays default tool validation cache with filtering options
 * ReAction-based window for Workbench 3.2+
 * 
 * Migrated from GadTools to ReAction following REACTION_MIGRATION_GUIDE.md
 * Uses ListBrowser gadgets for tool list and details display.
 */

#ifndef ITIDY_TOOL_CACHE_WINDOW_H
#define ITIDY_TOOL_CACHE_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>

/*------------------------------------------------------------------------*/
/* Gadget IDs (ReAction uses these via GA_ID)                            */
/*------------------------------------------------------------------------*/
enum {
    GID_TOOL_ROOT_LAYOUT = 4000,
    GID_TOOL_FOLDER_LAYOUT,
    GID_TOOL_FOLDER_GETFILE,
    GID_TOOL_SCAN_BUTTON,
    GID_TOOL_FILTER_CHOOSER,
    GID_TOOL_LIST_LISTBROWSER,
    GID_TOOL_DETAILS_LISTBROWSER,
    GID_TOOL_REPLACE_BATCH_BUTTON,
    GID_TOOL_REPLACE_SINGLE_BUTTON,
    GID_TOOL_RESTORE_TOOLS_BUTTON,
    GID_TOOL_CLOSE_BUTTON
};

/* Legacy gadget IDs for backward compatibility */
#define GID_TOOL_FOLDER_PATH    GID_TOOL_FOLDER_GETFILE
#define GID_TOOL_BROWSE         GID_TOOL_FOLDER_GETFILE
#define GID_TOOL_LIST           GID_TOOL_LIST_LISTBROWSER
#define GID_TOOL_FILTER_CYCLE   GID_TOOL_FILTER_CHOOSER
#define GID_TOOL_REBUILD_CACHE  GID_TOOL_SCAN_BUTTON
#define GID_TOOL_CACHE_CLOSE    GID_TOOL_CLOSE_BUTTON
#define GID_TOOL_REPLACE_BATCH  GID_TOOL_REPLACE_BATCH_BUTTON
#define GID_TOOL_REPLACE_SINGLE GID_TOOL_REPLACE_SINGLE_BUTTON
#define GID_TOOL_DETAILS_LIST   GID_TOOL_DETAILS_LISTBROWSER
#define GID_TOOL_RESTORE_DEFAULT_TOOLS GID_TOOL_RESTORE_TOOLS_BUTTON
#define GID_TOOL_SCAN_BOTTOM    GID_TOOL_SCAN_BUTTON

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define TOOL_WINDOW_TITLE "iTidy - Default Tool Analysis"

/*------------------------------------------------------------------------*/
/* Layout Configuration                                                   */
/*------------------------------------------------------------------------*/
#define TOOL_NAME_COLUMN_WIDTH  40  /* Maximum characters for tool name column */

/*------------------------------------------------------------------------*/
/* Filter Types                                                           */
/*------------------------------------------------------------------------*/
typedef enum {
    TOOL_FILTER_ALL = 0,
    TOOL_FILTER_VALID,
    TOOL_FILTER_MISSING
} ToolFilterType;

/*------------------------------------------------------------------------*/
/* Tool Cache Display Entry                                              */
/* This structure is used for internal tracking and cache lookup         */
/*------------------------------------------------------------------------*/
struct ToolCacheDisplayEntry
{
    struct Node node;               /* For linking in tool_entries list */
    struct Node filter_node;        /* For linking in filtered_entries list */
    char *tool_name;                /* Tool name */
    char *display_text;             /* Formatted display text (legacy) */
    BOOL exists;                    /* TRUE if tool exists */
    int hit_count;                  /* Number of cache hits (deprecated - use file_count) */
    int file_count;                 /* Number of files using this tool */
    char *full_path;                /* Full path or "(not found)" */
    char *version;                  /* Version string or "(no version)" */
    int cache_index;                /* Index into g_ToolCache array */
};

/*------------------------------------------------------------------------*/
/* Tool Cache Window Data Structure (ReAction)                           */
/*------------------------------------------------------------------------*/
struct iTidyToolCacheWindow
{
    /* Screen and window */
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Window pointer (from RA_OpenWindow) */
    Object *window_obj;                 /* ReAction window object */
    BOOL window_open;                   /* Window state flag */
    
    /* ReAction gadget objects */
    Object *folder_getfile_obj;         /* GetFile gadget for folder selection */
    Object *scan_btn_obj;               /* Scan button */
    Object *filter_chooser_obj;         /* Filter chooser (dropdown) */
    Object *tool_listbrowser_obj;       /* ListBrowser for tool list */
    Object *details_listbrowser_obj;    /* ListBrowser for file details */
    Object *replace_batch_btn_obj;      /* Replace Tool (Batch) button */
    Object *replace_single_btn_obj;     /* Replace Tool (Single) button */
    Object *restore_tools_btn_obj;      /* Restore Default Tools button */
    Object *close_btn_obj;              /* Close button */
    
    /* ListBrowser node lists */
    struct List *tool_list_nodes;       /* ListBrowser nodes for tool list */
    struct List *details_list_nodes;    /* ListBrowser nodes for details */
    
    /* Chooser labels list */
    struct List *filter_labels;         /* Chooser labels for filter dropdown */
    
    /* Data structures */
    struct List tool_entries;           /* List of ToolCacheDisplayEntry nodes */
    struct List filtered_entries;       /* Filtered list for internal tracking */
    ToolFilterType current_filter;      /* Current filter mode */
    ULONG total_count;                  /* Total tools */
    ULONG valid_count;                  /* Tools found */
    ULONG missing_count;                /* Tools not found */
    char summary_text[80];              /* Summary bar text */
    char window_title[80];              /* Window title (must persist) */
    
    /* Selection tracking */
    LONG selected_index;                /* Currently selected index (-1 = none) */
    LONG selected_details_index;        /* Selected index in details panel (-1 = none) */
    struct List details_list;           /* Legacy list for details (internal) */
    
    /* Sorting tracking */
    ULONG last_sort_column;             /* Last sorted column */
    ULONG sort_direction;               /* Current sort direction (LBMSORT_FORWARD or LBMSORT_REVERSE) */
    
    /* Folder path */
    char folder_path_buffer[256];       /* Current folder path */
    char last_save_path[512];           /* Last saved file path for Save menu */
    
    /* Legacy compatibility - unused but kept for struct size compatibility */
    APTR visual_info;                   /* Unused in ReAction version */
    struct Gadget *glist;               /* Unused in ReAction version */
    struct TextFont *system_font;       /* Unused in ReAction version */
    struct TextAttr system_font_attr;   /* Unused in ReAction version */
    
    /* Legacy gadget pointers - mapped to obj pointers */
    struct Gadget *folder_label;        /* Unused - ReAction uses labels */
    struct Gadget *folder_path;         /* Maps to folder_getfile_obj */
    struct Gadget *browse_btn;          /* Maps to folder_getfile_obj */
    struct Gadget *tool_list;           /* Maps to tool_listbrowser_obj */
    struct Gadget *details_listview;    /* Maps to details_listbrowser_obj */
    struct Gadget *filter_cycle;        /* Maps to filter_chooser_obj */
    struct Gadget *scan_btn;            /* Maps to scan_btn_obj */
    struct Gadget *close_btn;           /* Maps to close_btn_obj */
    struct Gadget *replace_batch_btn;   /* Maps to replace_batch_btn_obj */
    struct Gadget *replace_single_btn;  /* Maps to replace_single_btn_obj */
    struct Gadget *restore_default_tools_btn; /* Maps to restore_tools_btn_obj */
    
    /* Legacy folder box coords - unused in ReAction (auto-layout) */
    WORD folder_box_left;
    WORD folder_box_top;
    WORD folder_box_width;
    WORD folder_box_height;
};

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Open the Tool Cache Window
 * 
 * Opens a ReAction window showing the default tool validation cache with 
 * statistics and filtering options. Reads data from the global g_ToolCache.
 * 
 * @param tool_data Pointer to tool cache window data structure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
BOOL open_tool_cache_window(struct iTidyToolCacheWindow *tool_data);

/**
 * @brief Close the Tool Cache Window and cleanup resources
 * 
 * @param tool_data Pointer to tool cache window data structure
 */
void close_tool_cache_window(struct iTidyToolCacheWindow *tool_data);

/**
 * @brief Handle tool cache window events (main event loop)
 * 
 * @param tool_data Pointer to tool cache window data structure
 * @return BOOL TRUE to continue, FALSE to close window
 */
BOOL handle_tool_cache_window_events(struct iTidyToolCacheWindow *tool_data);

/**
 * @brief Build display list from global tool cache
 * 
 * Reads from extern g_ToolCache, g_ToolCacheCount and builds formatted
 * display entries for the listbrowser.
 * 
 * @param tool_data Pointer to tool cache window data
 * @return BOOL TRUE if successful, FALSE on error
 */
BOOL build_tool_cache_display_list(struct iTidyToolCacheWindow *tool_data);

/**
 * @brief Apply filter to tool list
 * 
 * Filters the tool list based on current filter type (all/valid/missing).
 * 
 * @param tool_data Pointer to tool cache window data
 */
void apply_tool_filter(struct iTidyToolCacheWindow *tool_data);

/**
 * @brief Update details panel for selected tool
 * 
 * Updates the details listbrowser with information about the currently
 * selected tool entry.
 * 
 * @param tool_data Pointer to tool cache window data
 * @param selected_index Index of selected tool (-1 for none)
 */
void update_tool_details(struct iTidyToolCacheWindow *tool_data, LONG selected_index);

/**
 * @brief Free all tool cache display entries
 * 
 * @param tool_data Pointer to tool cache window data
 */
void free_tool_cache_entries(struct iTidyToolCacheWindow *tool_data);

/**
 * @brief Refresh the tool cache window display
 * 
 * Rebuilds the display list from the current g_ToolCache and refreshes
 * both the tool list and details panel. Call this after the cache has
 * been modified externally (e.g., by default tool updates).
 * 
 * @param tool_data Pointer to tool cache window data
 */
void refresh_tool_cache_window(struct iTidyToolCacheWindow *tool_data);

#endif /* ITIDY_TOOL_CACHE_WINDOW_H */
