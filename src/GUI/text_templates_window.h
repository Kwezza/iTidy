/*
 * text_templates_window.h - ASCII Text Template Manager Window (ReAction)
 *
 * GUI window for managing per-type text preview template icons (def_<type>.info).
 * Displays all ASCII DefIcons sub-types, shows which have a custom template,
 * and provides Copy, ToolType validation, and WB Info actions.
 *
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#ifndef TEXT_TEMPLATES_WINDOW_H
#define TEXT_TEMPLATES_WINDOW_H

#include <exec/types.h>
#include "../layout_preferences.h"

/* Forward declaration - callers need not include intuition.h */
struct Window;

/*========================================================================*/
/* Public API                                                            */
/*========================================================================*/

/**
 * @brief Open the ASCII Text Template Manager window
 *
 * Opens a modal ReAction window listing every ASCII DefIcons sub-type.
 * For each type the user can see whether a custom def_<type>.info icon
 * exists in PROGDIR:Icons/, copy def_ascii.info to create/replace it,
 * open the Workbench Information dialog for the icon via the WB ARexx
 * port, and validate the icon's ITIDY_ ToolType keys and values.
 *
 * Also provides a master "Enable text preview" checkbox that mirrors
 * prefs->deficons_enable_text_previews.
 *
 * @param prefs      Pointer to LayoutPreferences to edit in-place
 *
 * @return TRUE  if user closed with OK (prefs->deficons_enable_text_previews
 *               may have changed).
 *         FALSE if user cancelled or window could not be opened.
 *
 * @note Window is modal - blocks until OK or Cancel pressed.
 * @note Requires g_cached_deficons_tree to be initialised.
 */
BOOL open_text_templates_window(LayoutPreferences *prefs);

#endif /* TEXT_TEMPLATES_WINDOW_H */
