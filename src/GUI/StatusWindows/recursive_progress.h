/*
 * recursive_progress.h - Dual-bar recursive progress window
 * For recursive operations (folder tree walking with icon processing)
 */

#ifndef ITIDY_RECURSIVE_PROGRESS_H
#define ITIDY_RECURSIVE_PROGRESS_H

#include <exec/types.h>

/* Forward declarations */
struct Screen;
struct Window;

/*
 * Prescan Result Structure
 * Contains results from directory tree prescan
 * Required to show accurate progress bars with known totals
 */
typedef struct iTidy_RecursiveScanResult {
    ULONG totalFolders;        /* Total folders to process */
    ULONG totalIcons;          /* Total icons across all folders */
    char **folderPaths;        /* Array of folder path strings */
    UWORD *iconCounts;         /* Icons per folder (parallel array) */
    ULONG allocated;           /* Internal: allocated array size */
} iTidy_RecursiveScanResult;

/*
 * Recursive Progress Window Structure
 * Displays dual-level progress with:
 * - Outer bar: Folder progress (e.g., 227/500 folders)
 * - Inner bar: Icon progress within current folder (e.g., 15/43 icons)
 * - Current folder path display
 */
typedef struct iTidy_RecursiveProgressWindow {
    struct Window *window;
    struct Screen *screen;
    
    /* Display state */
    char task_label[128];      /* "Processing Icons Recursively" */
    ULONG total_folders;       /* Total folders (from prescan) */
    ULONG total_icons;         /* Total icons (from prescan) */
    ULONG current_folder;      /* Current folder number (1-based) */
    UWORD current_icon;        /* Current icon in folder (1-based) */
    UWORD icons_in_folder;     /* Icons in current folder */
    
    /* Layout positions (pre-calculated) */
    WORD label_x, label_y;
    WORD percent_x, percent_y;
    WORD folder_label_x, folder_label_y;
    WORD folder_bar_x, folder_bar_y;
    UWORD folder_bar_w, folder_bar_h;
    WORD folder_count_x, folder_count_y;
    WORD path_x, path_y;
    UWORD path_max_width;
    WORD icon_label_x, icon_label_y;
    WORD icon_bar_x, icon_bar_y;
    UWORD icon_bar_w, icon_bar_h;
    WORD icon_count_x, icon_count_y;
    
    /* Font metrics */
    UWORD font_width, font_height;
    
    /* Cached state for smart redrawing */
    ULONG last_folder_fill_width;
    ULONG last_icon_fill_width;
    UWORD last_folder_percent;
    UWORD last_icon_percent;
    char last_folder_path[256];
    
} iTidy_RecursiveProgressWindow;

/*
 * Prescan directory tree recursively
 * Walks entire directory tree to count folders and icons.
 * CRITICAL: Yields to multitasking via Delay() to keep system responsive.
 * 
 * Parameters:
 *   rootPath - Root directory to scan (e.g., "Work:WHDLoad")
 * 
 * Returns: Scan result structure with totals and paths, or NULL on failure
 * 
 * Note: This can take 1-2 seconds for large trees (500 folders).
 *       Progress is not shown, but system remains responsive via yielding.
 *       Call iTidy_FreeScanResult() when done.
 */
struct iTidy_RecursiveScanResult* iTidy_PrescanRecursive(
    const char *rootPath
);

/*
 * Open recursive progress window
 * Opens dual-bar progress window for recursive operations.
 * Window opens instantly with both bars at 0%.
 * 
 * Parameters:
 *   screen      - Workbench screen to open on
 *   task_label  - Task description (e.g., "Processing Icons Recursively")
 *   scan        - Prescan results (from iTidy_PrescanRecursive)
 * 
 * Returns: Recursive progress window handle, or NULL on failure
 * 
 * Note: scan parameter is borrowed (not owned). Don't free it until after
 *       closing the window.
 */
struct iTidy_RecursiveProgressWindow* iTidy_OpenRecursiveProgress(
    struct Screen *screen,
    const char *task_label,
    const struct iTidy_RecursiveScanResult *scan
);

/*
 * Update folder progress (outer bar)
 * Call when starting to process a new folder.
 * Updates outer bar and resets inner bar to 0%.
 * 
 * Parameters:
 *   rpw             - Recursive progress window handle
 *   folder_index    - Current folder number (1-based, range: 1..total_folders)
 *   folder_path     - Path being processed (e.g., "Work:WHDLoad/GamesOCS/")
 *   icons_in_folder - Number of icons in this folder (for inner bar setup)
 * 
 * Note: Automatically handles refresh events. Only redraws changed elements.
 */
void iTidy_UpdateFolderProgress(
    struct iTidy_RecursiveProgressWindow *rpw,
    ULONG folder_index,
    const char *folder_path,
    UWORD icons_in_folder
);

/*
 * Update icon progress (inner bar)
 * Call when processing each icon within the current folder.
 * Updates inner bar only (outer bar unchanged).
 * 
 * Parameters:
 *   rpw        - Recursive progress window handle
 *   icon_index - Current icon number within folder (1-based)
 * 
 * Note: Call iTidy_UpdateFolderProgress() first to set up the folder context.
 */
void iTidy_UpdateIconProgress(
    struct iTidy_RecursiveProgressWindow *rpw,
    UWORD icon_index
);

/*
 * Close recursive progress window
 * Cleans up and closes the window. Always call this when done.
 * Safe to call even if window opening failed (checks for NULL).
 * 
 * Parameters:
 *   rpw - Recursive progress window handle (can be NULL)
 * 
 * Note: Does NOT free the scan result. Call iTidy_FreeScanResult() separately.
 */
void iTidy_CloseRecursiveProgress(
    struct iTidy_RecursiveProgressWindow *rpw
);

/*
 * Free prescan result
 * Frees all memory allocated by iTidy_PrescanRecursive().
 * Safe to call with NULL pointer.
 * 
 * Parameters:
 *   scan - Scan result to free (can be NULL)
 */
void iTidy_FreeScanResult(
    struct iTidy_RecursiveScanResult *scan
);

#endif /* ITIDY_RECURSIVE_PROGRESS_H */
