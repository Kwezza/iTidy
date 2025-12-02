/*
 * tool_cache_window.h - iTidy Tool Cache Window Header
 * Displays default tool validation cache with filtering options
 * GadTools-based window for Workbench 2.0+
 */

#ifndef ITIDY_TOOL_CACHE_WINDOW_H
#define ITIDY_TOOL_CACHE_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>

/*------------------------------------------------------------------------*/
/* Gadget IDs                                                             */
/*------------------------------------------------------------------------*/
#define GID_TOOL_FOLDER_PATH    4000
#define GID_TOOL_BROWSE         4011
#define GID_TOOL_LIST           4001
#define GID_TOOL_FILTER_CYCLE   4002
#define GID_TOOL_REBUILD_CACHE  4005
#define GID_TOOL_CACHE_CLOSE    4006
#define GID_TOOL_REPLACE_BATCH  4007
#define GID_TOOL_REPLACE_SINGLE 4008
#define GID_TOOL_DETAILS_LIST   4009
#define GID_TOOL_RESTORE_DEFAULT_TOOLS 4010
#define GID_TOOL_SCAN_BOTTOM    4012

/*------------------------------------------------------------------------*/
/* Window Spacing Constants                                              */
/*------------------------------------------------------------------------*/
#define TOOL_WINDOW_SPACE_X         8       /* Horizontal spacing */
#define TOOL_WINDOW_SPACE_Y         8       /* Vertical spacing */
#define TOOL_WINDOW_MARGIN_LEFT     10      /* Left margin */
#define TOOL_WINDOW_MARGIN_TOP      10      /* Top margin */
#define TOOL_WINDOW_MARGIN_RIGHT    10      /* Right margin */
#define TOOL_WINDOW_MARGIN_BOTTOM   10      /* Bottom margin */
#define TOOL_WINDOW_BUTTON_PADDING  8       /* Button text padding */

/*------------------------------------------------------------------------*/
/* Standard Window Width                                                 */
/*------------------------------------------------------------------------*/
#define TOOL_WINDOW_WIDTH_CHARS     70      /* Width in characters */

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
/*------------------------------------------------------------------------*/
struct ToolCacheDisplayEntry
{
    struct Node node;               /* For linking in tool_entries list */
    struct Node filter_node;        /* For linking in filtered_entries list */
    char *tool_name;                /* Tool name */
    char *display_text;             /* Formatted display text for listview */
    BOOL exists;                    /* TRUE if tool exists */
    int hit_count;                  /* Number of cache hits (deprecated - use file_count) */
    int file_count;                 /* Number of files using this tool */
    char *full_path;                /* Full path or "(not found)" */
    char *version;                  /* Version string or "(no version)" */
};

/*------------------------------------------------------------------------*/
/* Tool Cache Window Data Structure                                      */
/*------------------------------------------------------------------------*/
struct iTidyToolCacheWindow
{
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Tool cache window */
    APTR visual_info;                   /* GadTools visual info */
    struct Gadget *glist;               /* Gadget list */
    struct TextFont *system_font;       /* System default font (if opened) */
    struct TextAttr system_font_attr;   /* Font attributes (must persist) */
    BOOL window_open;                   /* Window state flag */
    
    /* Gadget pointers */
    struct Gadget *folder_label;        /* Folder label */
    struct Gadget *folder_path;         /* Folder path string gadget */
    struct Gadget *browse_btn;          /* Browse button */
    struct Gadget *tool_list;           /* ListView showing tools */
    struct Gadget *details_listview;    /* ListView showing file details */
    struct Gadget *filter_cycle;        /* Filter cycle gadget */
    struct Gadget *scan_bottom_btn;     /* Scan button (bottom row) */
    struct Gadget *close_btn;           /* Close button */
    struct Gadget *replace_batch_btn;   /* Replace Tool (Batch) button */
    struct Gadget *replace_single_btn;  /* Replace Tool (Single) button */
    struct Gadget *restore_default_tools_btn; /* Restore Default Tools button */
    
    /* Data */
    struct List tool_entries;           /* List of ToolCacheDisplayEntry nodes */
    struct List filtered_entries;       /* Filtered list for display */
    ToolFilterType current_filter;      /* Current filter mode */
    ULONG total_count;                  /* Total tools */
    ULONG valid_count;                  /* Tools found */
    ULONG missing_count;                /* Tools not found */
    char summary_text[80];              /* Summary bar text */
    char window_title[80];              /* Window title (must persist) */
    
    /* Details panel data (for selected tool) */
    LONG selected_index;                /* Currently selected index (-1 = none) */
    LONG selected_details_index;        /* Selected index in details panel (-1 = none) */
    struct List details_list;           /* List for details panel */
    
    /* Folder path for Rebuild Cache operation */
    char folder_path_buffer[256];       /* Current folder path */
    char last_save_path[512];           /* Last saved file path for Save menu */
    
    /* Folder path display box coordinates (for custom drawing) */
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
 * Opens a window showing the default tool validation cache with statistics
 * and filtering options. Reads data from the global g_ToolCache.
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
 * display entries for the listview.
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
 * Updates the details listview with information about the currently
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
