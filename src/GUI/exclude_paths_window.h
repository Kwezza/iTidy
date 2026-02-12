/*
 * exclude_paths_window.h - DefIcons Exclude Paths Management Window
 * 
 * GUI window for managing user-configurable exclude paths for DefIcons
 * icon creation. Allows users to add, remove, and modify directory paths
 * that should be skipped during icon creation scanning.
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#ifndef EXCLUDE_PATHS_WINDOW_H
#define EXCLUDE_PATHS_WINDOW_H

#include <exec/types.h>
#include "../layout_preferences.h"

/*========================================================================*/
/* Public Functions                                                      */
/*========================================================================*/

/**
 * @brief Open the exclude paths management window
 * 
 * Opens a modal ReAction window for managing the DefIcons exclude paths
 * list. User can add, remove, modify paths, and reset to defaults.
 * 
 * Window features:
 * - ListBrowser displaying current exclude paths
 * - Add button (with GetFile gadget for directory selection)
 * - Remove button (removes selected path)
 * - Modify button (edit existing path)
 * - Reset to Defaults button (restores default exclude list)
 * - OK/Cancel buttons (standard modal dialog)
 * 
 * @param prefs Working copy of layout preferences to modify
 * 
 * @return TRUE if user clicked OK (save changes), FALSE if Cancel
 * 
 * @note Changes are made to working copy - caller must update global prefs
 * @note Window is modal - blocks until OK or Cancel clicked
 */
BOOL open_exclude_paths_window(LayoutPreferences *prefs);

#endif /* EXCLUDE_PATHS_WINDOW_H */
