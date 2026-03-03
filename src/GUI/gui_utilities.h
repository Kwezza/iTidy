/*
 * gui_utilities.h - Shared GUI Utilities for iTidy
 * 
 * Provides cross-version compatibility wrappers for Workbench 2.x and 3.x systems.
 * Contains helper functions that gracefully degrade features on older Workbench versions.
 */

#ifndef GUI_UTILITIES_H
#define GUI_UTILITIES_H

#include <exec/types.h>
#include <intuition/intuition.h>

/*------------------------------------------------------------------------*/
/* Workbench Version-Safe Window Pointer Functions                       */
/*------------------------------------------------------------------------*/

/**
 * safe_set_window_pointer - Safely set window pointer (WB 3.0+ only)
 * 
 * SetWindowPointer() with WA_BusyPointer is only available on Workbench 3.0+
 * (Intuition v39+). On WB 2.x, calling this causes a crash with error 80000004.
 * This wrapper checks the cached Workbench version from prefsWorkbench and only
 * calls SetWindowPointer on systems that support it.
 * 
 * When clearing the busy pointer (busy=FALSE), this function also flushes any
 * IDCMP messages that accumulated on the window's UserPort while it was busy.
 * This prevents stale clicks, close events, etc. from firing immediately when
 * the parent event loop resumes after a child window closes.
 * 
 * @param window Pointer to the window to modify (may be NULL, function will return safely)
 * @param busy TRUE to set busy pointer, FALSE to restore normal pointer
 * 
 * @note On WB 2.x systems, this function silently does nothing (no busy pointer available)
 * @note Uses prefsWorkbench.workbenchVersion which is populated at startup
 */
void safe_set_window_pointer(struct Window *window, BOOL busy);

#endif /* GUI_UTILITIES_H */
