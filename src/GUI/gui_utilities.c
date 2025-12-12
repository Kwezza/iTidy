/*
 * gui_utilities.c - Shared GUI Utilities for iTidy
 * 
 * Provides cross-version compatibility wrappers for Workbench 2.x and 3.x systems.
 */

#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include <utility/tagitem.h>

#include "GUI/gui_utilities.h"
#include "Settings/WorkbenchPrefs.h"  /* For WorkbenchSettings struct */

/* External reference to global Workbench settings (populated at startup) */
extern struct WorkbenchSettings prefsWorkbench;

/*------------------------------------------------------------------------*/
/**
 * safe_set_window_pointer - Safely set window pointer (WB 3.0+ only)
 * 
 * SetWindowPointer() with WA_BusyPointer is only available on Workbench 3.0+
 * (Intuition v39+). On WB 2.x, calling this causes a crash with error 80000004.
 * This wrapper checks the cached Workbench version from prefsWorkbench and only
 * calls SetWindowPointer on systems that support it.
 * 
 * The Workbench version is read once at startup by fetchWorkbenchSettings() and
 * stored in prefsWorkbench.workbenchVersion, avoiding repeated SysBase reads.
 * 
 * Version values:
 *   36    = Workbench 2.0
 *   37-38 = Workbench 2.1
 *   39+   = Workbench 3.0+
 * 
 * @param window Pointer to the window to modify (may be NULL, function will return safely)
 * @param busy TRUE to set busy pointer, FALSE to restore normal pointer
 * 
 * @note On WB 2.x systems, this function silently does nothing (no busy pointer available)
 */
/*------------------------------------------------------------------------*/
void safe_set_window_pointer(struct Window *window, BOOL busy)
{
    /* Safety check - NULL window is valid, just return */
    if (!window) return;
    
    /* Only use SetWindowPointer on Workbench 3.0+ (version 39+) */
    /* Use cached version from prefsWorkbench (loaded at startup) */
    if (prefsWorkbench.workbenchVersion >= 39)
    {
        if (busy)
        {
            SetWindowPointer(window, WA_BusyPointer, TRUE, TAG_DONE);
        }
        else
        {
            SetWindowPointer(window, WA_Pointer, NULL, TAG_DONE);
        }
    }
    /* On WB 2.x, no busy pointer support - silently skip */
}
