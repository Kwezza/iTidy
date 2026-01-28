/*
 * beta_options_window.h - iTidy Beta Options Window (ReAction)
 * ReAction-based GUI for Workbench 3.2+
 * Stage 1 Migration
 */

#ifndef ITIDY_BETA_OPTIONS_WINDOW_H
#define ITIDY_BETA_OPTIONS_WINDOW_H

#include <exec/types.h>
#include "layout_preferences.h"

/*------------------------------------------------------------------------*/
/* Public Functions                                                       */
/*------------------------------------------------------------------------*/

/*
 * Opens the Beta Options window.
 * Returns TRUE if changes were applied (OK clicked), FALSE otherwise.
 */
BOOL open_itidy_beta_options_window(LayoutPreferences *prefs);

#endif
