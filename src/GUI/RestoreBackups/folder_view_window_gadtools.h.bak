/*
 * folder_view_window.h - iTidy Folder View Window Header
 * Displays folder hierarchy for a selected backup run
 * GadTools-based window for Workbench 2.0+
 */

#ifndef ITIDY_FOLDER_VIEW_WINDOW_H
#define ITIDY_FOLDER_VIEW_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>

/*------------------------------------------------------------------------*/
/* Gadget IDs                                                             */
/*------------------------------------------------------------------------*/
#define GID_FOLDER_LIST         3001
#define GID_FOLDER_CLOSE_BTN    3002

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
/* Standard Window Width                                                 */
/*------------------------------------------------------------------------*/
#define FOLDER_VIEW_WIDTH_CHARS     65      /* Width in characters */

/*------------------------------------------------------------------------*/
/* Folder Entry Structure                                                */
/*------------------------------------------------------------------------*/
struct FolderEntry
{
    struct Node node;               /* For linking in list */
    char *path;                     /* Full folder path */
    char *display_text;             /* Formatted display text with tree lines */
    UWORD depth;                    /* Nesting depth (0 = root) */
    ULONG archive_number;           /* Archive number (00001, 00002, etc.) */
    ULONG size_bytes;               /* Size in bytes */
};

/*------------------------------------------------------------------------*/
/* Folder View Window Data Structure                                     */
/*------------------------------------------------------------------------*/
struct iTidyFolderViewWindow
{
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Folder view window */
    APTR visual_info;                   /* GadTools visual info */
    struct Gadget *glist;               /* Gadget list */
    struct TextFont *system_font;       /* System default font (if opened) */
    struct TextAttr system_font_attr;   /* Font attributes (must persist) */
    BOOL window_open;                   /* Window state flag */
    
    /* Gadget pointers */
    struct Gadget *folder_list;         /* ListView showing folder hierarchy */
    struct Gadget *close_btn;           /* Close button */
    
    /* Data */
    struct List folder_entries;         /* List of FolderEntry nodes */
    char run_name[16];                  /* Run name (e.g., "Run_0008") */
    char window_title[80];              /* Window title (must persist) */
    UWORD run_number;                   /* Run number */
    char date_str[24];                  /* Date created */
    ULONG archive_count;                /* Total archives */
};

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Open the Folder View Window for a backup run
 * 
 * Opens a modal window showing the folder hierarchy for a selected backup run.
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
 * @brief Parse catalog and build folder hierarchy with tree structure
 * 
 * @param catalog_path Path to catalog.txt file
 * @param folder_data Pointer to folder view window data
 * @return BOOL TRUE if successful, FALSE on error
 */
BOOL parse_catalog_and_build_tree(const char *catalog_path,
                                  struct iTidyFolderViewWindow *folder_data);

/**
 * @brief Format folder entry with ASCII tree lines
 * 
 * Formats a folder path with proper indentation and ASCII tree characters
 * based on its depth in the hierarchy.
 * 
 * @param path Folder path
 * @param depth Nesting depth (0 = root)
 * @param is_last TRUE if this is the last child at this depth
 * @param parent_lines Array indicating which parent levels need vertical lines
 * @param buffer Output buffer for formatted text
 * @param buffer_size Size of output buffer
 */
void format_folder_with_tree_lines(const char *path,
                                   UWORD depth,
                                   BOOL is_last,
                                   BOOL *parent_lines,
                                   char *buffer,
                                   UWORD buffer_size);

/**
 * @brief Free all folder entries in the list
 * 
 * @param folder_data Pointer to folder view window data
 */
void free_folder_entries(struct iTidyFolderViewWindow *folder_data);

#endif /* ITIDY_FOLDER_VIEW_WINDOW_H */
