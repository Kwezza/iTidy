/*
 * deficons_creation_window.h - DefIcons Icon Creation Settings Window (ReAction)
 * 
 * GUI window for configuring DefIcons icon creation settings such as:
 * - Folder icon mode (Smart/Always/Never)
 * - Icon size (Small/Medium/Large)
 * - Thumbnail options (borders, text preview, picture preview)
 * - Color depth and dithering settings
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#ifndef DEFICONS_CREATION_WINDOW_H
#define DEFICONS_CREATION_WINDOW_H

#include <exec/types.h>
#include "layout_preferences.h"

/**
 * @brief Open DefIcons icon creation settings window
 * 
 * Opens a ReAction window showing icon creation configuration options.
 * User can configure folder icon mode, icon size, thumbnail settings,
 * color depth, dithering method, and low-color mapping.
 * 
 * @param prefs Pointer to LayoutPreferences to edit (modified in-place on OK)
 * 
 * @return TRUE if user clicked OK (changes applied), FALSE if cancelled
 * 
 * @note Window can be open simultaneously with DefIcons type selection window
 * @note Changes are only saved to prefs if user clicks OK
 */
BOOL open_itidy_deficons_creation_window(LayoutPreferences *prefs);

#endif /* DEFICONS_CREATION_WINDOW_H */
