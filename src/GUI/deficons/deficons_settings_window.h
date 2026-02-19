/*
 * deficons_settings_window.h - DefIcons Type Selection Window (ReAction)
 * 
 * GUI window for selecting which DefIcons file types should have automatic
 * icon creation enabled. Uses hierarchical ListBrowser to display type tree.
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#ifndef DEFICONS_SETTINGS_WINDOW_H
#define DEFICONS_SETTINGS_WINDOW_H

#include <exec/types.h>
#include "layout_preferences.h"

/**
 * @brief Open DefIcons type selection settings window
 * 
 * Opens a modal-like ReAction window showing the hierarchical DefIcons
 * type tree. User can enable/disable icon creation for specific categories.
 * 
 * @param prefs Pointer to LayoutPreferences to edit (modified in-place)
 * 
 * @return TRUE if user clicked OK (changes applied), FALSE if cancelled
 * 
 * @note Window displays tree loaded from global DefIcons cache
 * @note Only generation 0 (root) types can be toggled
 * @note Child types are displayed for information only
 */
BOOL open_itidy_deficons_settings_window(LayoutPreferences *prefs);

#endif /* DEFICONS_SETTINGS_WINDOW_H */
