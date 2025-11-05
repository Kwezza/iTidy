/*
 * progress_window.h - Simple single-bar progress window
 * For operations with known item counts (backup, restore, single folder processing)
 */

#ifndef ITIDY_PROGRESS_WINDOW_H
#define ITIDY_PROGRESS_WINDOW_H

#include <exec/types.h>

/* Forward declarations */
struct Screen;
struct Window;

/*
 * Progress Window Structure
 * Displays single-level progress with:
 * - Task label and percentage
 * - 3D beveled progress bar
 * - Helper text showing current item
 */
typedef struct iTidy_ProgressWindow {
    struct Window *window;
    struct Screen *screen;
    
    /* Display state */
    char task_label[128];      /* "Restoring Backup Run 0007" */
    UWORD total_items;         /* Total count (e.g., 63 archives) */
    UWORD current_item;        /* Current item (1-based) */
    
    /* Layout positions (pre-calculated for fast updates) */
    WORD label_x, label_y;
    WORD percent_x, percent_y;
    WORD bar_x, bar_y;
    UWORD bar_w, bar_h;
    WORD helper_x, helper_y;
    UWORD helper_max_width;
    
    /* Font metrics */
    UWORD font_width, font_height;
    
    /* Cached state for smart redrawing */
    UWORD last_fill_width;     /* Last bar fill width in pixels */
    UWORD last_percent;        /* Last percentage value (0-100) */
    char last_helper_text[256]; /* Last helper text */
    
    /* Completion state */
    BOOL completed;            /* TRUE when showing completion UI */
    struct Gadget *close_button; /* Close button gadget (NULL during operation) */
    
    /* Future-proof: Reserved for Cancel button feature */
    volatile BOOL userCancelled; /* Reserved - currently unused */
    
} iTidy_ProgressWindow;

/*
 * Open progress window
 * Opens instantly with empty progress bar, ready for updates.
 * 
 * Parameters:
 *   screen      - Workbench screen to open on
 *   task_label  - Task description (e.g., "Restoring Backup Run 0007")
 *   total_items - Total number of items to process
 * 
 * Returns: Progress window handle, or NULL on failure
 * 
 * Note: Window opens immediately (<0.1s) with busy pointer.
 *       No slow operations should happen before this call.
 */
struct iTidy_ProgressWindow* iTidy_OpenProgressWindow(
    struct Screen *screen,
    const char *task_label,
    UWORD total_items
);

/*
 * Update progress
 * Updates progress bar and helper text. Uses smart redrawing to minimize flicker.
 * 
 * Parameters:
 *   pw          - Progress window handle
 *   current_item - Current item number (1-based, range: 1..total_items)
 *   helper_text - Description of current operation (e.g., "Extracting: 00015.lha")
 *                 Can be NULL to leave unchanged.
 * 
 * Note: Automatically handles IDCMP_REFRESHWINDOW events to prevent artifacts.
 *       Only redraws changed elements (progress bar, percentage, text).
 */
void iTidy_UpdateProgress(
    struct iTidy_ProgressWindow *pw,
    UWORD current_item,
    const char *helper_text
);

/*
 * Show completion state
 * Transitions window to completion state with Close button.
 * Replaces progress bar with full-width Close button.
 * Clears busy pointer - operation is complete.
 * 
 * Parameters:
 *   pw      - Progress window handle
 *   success - TRUE for success message, FALSE for error message
 * 
 * After calling this, you must call iTidy_HandleProgressWindowEvents()
 * in a loop until it returns FALSE (user clicked Close).
 */
void iTidy_ShowCompletionState(
    struct iTidy_ProgressWindow *pw,
    BOOL success
);

/*
 * Handle progress window events
 * Processes window events during completion state.
 * Call in a loop after iTidy_ShowCompletionState().
 * 
 * Parameters:
 *   pw - Progress window handle
 * 
 * Returns: TRUE to keep window open, FALSE when user clicked Close button
 * 
 * Example:
 *   iTidy_ShowCompletionState(pw, TRUE);
 *   while (iTidy_HandleProgressWindowEvents(pw)) {
 *       WaitPort(pw->window->UserPort);
 *   }
 *   iTidy_CloseProgressWindow(pw);
 */
BOOL iTidy_HandleProgressWindowEvents(
    struct iTidy_ProgressWindow *pw
);

/*
 * Close progress window
 * Cleans up and closes the window. Always call this when done.
 * Safe to call even if window opening failed (checks for NULL).
 * 
 * Parameters:
 *   pw - Progress window handle (can be NULL)
 */
void iTidy_CloseProgressWindow(
    struct iTidy_ProgressWindow *pw
);

#endif /* ITIDY_PROGRESS_WINDOW_H */
