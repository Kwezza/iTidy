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
 * @brief Open the DefIcons Text Templates window
 *
 * Opens a non-modal ReAction window listing every ASCII DefIcons sub-type.
 * For each type the user can see whether a custom def_<type>.info template
 * exists in PROGDIR:Icons/, create or overwrite it from the master
 * (def_ascii.info), open the Workbench Information dialog to edit ToolTypes,
 * validate the ToolTypes, and revert a custom template back to the master.
 *
 * A "Show" filter chooser allows the list to be narrowed to
 * "All", "Custom only", or "Missing only" entries.
 *
 * All file actions are immediate. The window has a single Close button.
 *
 * @param prefs  Pointer to LayoutPreferences (kept for context; future use)
 *
 * @note Requires g_cached_deficons_tree to be initialised.
 */
void open_text_templates_window(LayoutPreferences *prefs);

#endif /* TEXT_TEMPLATES_WINDOW_H */
