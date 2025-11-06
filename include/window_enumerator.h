/*
 * window_enumerator.h - Workbench Window Enumeration for Debugging
 * Part of iTidy Icon Cleanup Tool
 * 
 * Purpose: Enumerate all open Workbench windows and display their
 *          titles and filesystem paths (where available)
 * 
 * Usage: Call Debug_ListWorkbenchWindows() to print window info to console
 */

#ifndef WINDOW_ENUMERATOR_H
#define WINDOW_ENUMERATOR_H

#include <exec/types.h>
#include <intuition/intuition.h>

/*------------------------------------------------------------------------*/
/* Folder Window Tracking                                                 */
/*------------------------------------------------------------------------*/

/**
 * @brief Structure to track a potential folder window
 * 
 * Used to store window information for folder windows that iTidy may
 * need to resize/reposition after icon cleanup operations.
 */
typedef struct FolderWindowInfo
{
    struct Window *window;      /* Pointer to the actual Window structure */
    char title[256];            /* Window title (folder name) */
    WORD left;                  /* Current X position */
    WORD top;                   /* Current Y position */
    WORD width;                 /* Current width */
    WORD height;                /* Current height */
} FolderWindowInfo;

/**
 * @brief Array of folder windows tracked by iTidy
 * 
 * This is populated before an iTidy run to track open folder windows
 * that may need to be resized/repositioned after icon operations.
 */
typedef struct FolderWindowTracker
{
    FolderWindowInfo *windows;  /* Dynamic array of folder windows */
    ULONG count;                /* Number of windows in array */
    ULONG capacity;             /* Allocated capacity */
} FolderWindowTracker;

/*------------------------------------------------------------------------*/
/* Public API                                                             */
/*------------------------------------------------------------------------*/

/**
 * @brief Enumerate and print all open Workbench windows
 * 
 * This debugging function locks the Workbench screen and iterates through
 * all open windows, printing their titles and drawer paths (where available).
 * 
 * For each window found:
 * - Prints the window title
 * - If the window is a Workbench drawer, attempts to resolve the full
 *   filesystem path using NameFromLock() on the DrawerData structure
 * 
 * All output is sent to console via printf(). This is a debugging tool
 * intended for research and verification purposes only.
 * 
 * Safety:
 * - Properly locks/unlocks IntuitionBase during window list traversal
 * - Safely handles NULL pointers in title, UserData, and locks
 * - Does NOT modify any window properties
 * 
 * @return void (prints results to console)
 */
/*------------------------------------------------------------------------*/
void Debug_ListWorkbenchWindows(void);

/**
 * @brief Build array of potential folder windows for iTidy tracking
 * 
 * Scans all open Workbench windows and creates a tracking array of those
 * that are likely folder/drawer windows. This should be called before an
 * iTidy run to capture the current state of folder windows.
 * 
 * The function allocates memory for the tracker and populates it with
 * windows classified as "Workbench Drawer (likely)". Each entry stores
 * the window pointer, title, and current geometry.
 * 
 * @param tracker Pointer to FolderWindowTracker to populate (must be freed by caller)
 * @return BOOL TRUE if successful, FALSE on error
 */
/*------------------------------------------------------------------------*/
BOOL BuildFolderWindowList(FolderWindowTracker *tracker);

/**
 * @brief Free the folder window tracker and its resources
 * 
 * Releases all memory allocated by BuildFolderWindowList().
 * 
 * @param tracker Pointer to FolderWindowTracker to free
 */
/*------------------------------------------------------------------------*/
void FreeFolderWindowList(FolderWindowTracker *tracker);

/**
 * @brief Debug print the folder window list to log
 * 
 * Outputs the contents of a FolderWindowTracker to the debug log.
 * 
 * @param tracker Pointer to FolderWindowTracker to print
 */
/*------------------------------------------------------------------------*/
void Debug_PrintFolderWindowList(const FolderWindowTracker *tracker);

/**
 * @brief Move and resize a window to specified geometry
 * 
 * Applies new position and size to a window. This is used to restore
 * folder window geometry after iTidy has finished icon repositioning.
 * 
 * The function uses Intuition's MoveWindow() and SizeWindow() to change
 * the window dimensions. Changes are immediate and visible to the user.
 * 
 * @param win Pointer to the Window to modify
 * @param left New X position (screen coordinates)
 * @param top New Y position (screen coordinates)
 * @param width New width in pixels
 * @param height New height in pixels
 * @return BOOL TRUE if successful, FALSE if window pointer is NULL
 */
/*------------------------------------------------------------------------*/
BOOL ApplyWindowGeometry(struct Window *win, WORD left, WORD top, WORD width, WORD height);

#endif /* WINDOW_ENUMERATOR_H */
