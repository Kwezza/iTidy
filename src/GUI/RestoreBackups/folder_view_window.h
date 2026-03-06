/*
 * folder_view_window.h - iTidy Folder View Window Header
 * Displays folder hierarchy for a selected backup run
 * ReAction-based window for Workbench 3.2+ with Hierarchical ListBrowser
 */

#ifndef ITIDY_FOLDER_VIEW_WINDOW_H
#define ITIDY_FOLDER_VIEW_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>

/*------------------------------------------------------------------------*/
/* Gadget IDs                                                             */
/*------------------------------------------------------------------------*/
#define GID_FOLDER_VIEW_LISTBROWSER   3001
#define GID_FOLDER_VIEW_CLOSE_BTN     3002

/*------------------------------------------------------------------------*/
/* Window Spacing Constants                                              */
/*------------------------------------------------------------------------*/
#define FOLDER_VIEW_SPACE_X         8       /* Horizontal spacing */
#define FOLDER_VIEW_SPACE_Y         8       /* Vertical spacing */
#define FOLDER_VIEW_MARGIN_LEFT     10      /* Left margin */
#define FOLDER_VIEW_MARGIN_TOP      10      /* Top margin */
#define FOLDER_VIEW_MARGIN_RIGHT    10      /* Right margin */
#define FOLDER_VIEW_MARGIN_BOTTOM   10      /* Bottom margin */
#define FOLDER_VIEW_BUTTON_PADDING  8       /* Button text padding */

/*------------------------------------------------------------------------*/
/* Standard Window Dimensions                                            */
/*------------------------------------------------------------------------*/
#define FOLDER_VIEW_WIDTH           420     /* Window width in pixels */
#define FOLDER_VIEW_HEIGHT          300     /* Window height in pixels */
#define FOLDER_VIEW_MIN_WIDTH       300     /* Minimum window width */
#define FOLDER_VIEW_MIN_HEIGHT      200     /* Minimum window height */

/*------------------------------------------------------------------------*/
/* Hierarchical ListBrowser Node User Data                               */
/* This is stored as user data on each ListBrowser node                  */
/*------------------------------------------------------------------------*/
typedef struct iTidyFolderNodeData
{
    char *path;                     /* Full folder path (allocated) */
    char *display_name;             /* Display name for column (allocated) */
    char *size_info;                /* Size/icons info for column (allocated) */
    UWORD depth;                    /* Nesting depth (0 = root) */
    ULONG archive_number;           /* Archive number */
    BOOL has_children;              /* TRUE if this node has children */
} iTidyFolderNodeData;

/*------------------------------------------------------------------------*/
/* Folder View Window Data Structure                                     */
/*------------------------------------------------------------------------*/
struct iTidyFolderViewWindow
{
    /* Screen and Window */
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Window pointer (from RA_OpenWindow) */
    Object *window_obj;                 /* ReAction window object */
    BOOL window_open;                   /* Window state flag */
    
    /* ReAction Gadget Objects */
    Object *main_layout;                /* Main layout gadget */
    Object *listbrowser_obj;            /* Hierarchical ListBrowser */
    Object *close_button_obj;           /* Close button */
    
    /* ListBrowser Data */
    struct List *folder_list;           /* List of ListBrowser nodes */
    struct ColumnInfo *column_info;     /* Column information (2 columns) */
    
    /* Node User Data Tracking (for cleanup) */
    struct List node_data_list;         /* List to track allocated iTidyFolderNodeData */
    
    /* Window Info */
    char run_name[16];                  /* Run name (e.g., "Run_0008") */
    char window_title[80];              /* Window title (must persist) */
    UWORD run_number;                   /* Run number */
    char date_str[24];                  /* Date created */
    ULONG archive_count;                /* Total archives */
};

/*------------------------------------------------------------------------*/
/* Node Data Tracking Node (for cleanup)                                 */
/*------------------------------------------------------------------------*/
struct NodeDataTracker
{
    struct Node node;                   /* For linking in list */
    iTidyFolderNodeData *data;          /* Pointer to allocated data */
};

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Open the Folder View Window for a backup run
 * 
 * Opens a modal window showing the folder hierarchy for a selected backup run.
 * Uses ReAction with hierarchical ListBrowser for native tree display.
 * 
 * @param folder_data Pointer to folder view window data structure
 * @param catalog_path Full path to the catalog.txt file
 * @param run_number Run number for display
 * @param date_str Date created string
 * @param archive_count Total number of archives
 * @return BOOL TRUE if successful, FALSE otherwise
 */
BOOL open_folder_view_window(struct iTidyFolderViewWindow *folder_data,
                             const char *catalog_path,
                             UWORD run_number,
                             const char *date_str,
                             ULONG archive_count);

/**
 * @brief Close the Folder View Window and cleanup resources
 * 
 * @param folder_data Pointer to folder view window data structure
 */
void close_folder_view_window(struct iTidyFolderViewWindow *folder_data);

/**
 * @brief Handle folder view window events (main event loop)
 * 
 * @param folder_data Pointer to folder view window data structure
 * @return BOOL TRUE to continue, FALSE to close window
 */
BOOL handle_folder_view_window_events(struct iTidyFolderViewWindow *folder_data);

/**
 * @brief Parse catalog and build hierarchical folder tree
 * 
 * Parses the catalog file and builds a hierarchical ListBrowser node tree.
 * 
 * @param catalog_path Path to catalog.txt file
 * @param folder_data Pointer to folder view window data
 * @return BOOL TRUE if successful, FALSE on error
 */
BOOL parse_catalog_and_build_tree(const char *catalog_path,
                                  struct iTidyFolderViewWindow *folder_data);

/**
 * @brief Free all folder entries in the list
 * 
 * Frees all ListBrowser nodes and associated user data.
 * 
 * @param folder_data Pointer to folder view window data
 */
void free_folder_entries(struct iTidyFolderViewWindow *folder_data);

#endif /* ITIDY_FOLDER_VIEW_WINDOW_H */
