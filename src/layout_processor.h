/**
 * layout_processor.h - Icon Processing with Layout Preferences
 * 
 * Provides the main processing function that applies LayoutPreferences
 * to directories and their icons. This is the bridge between the GUI
 * preferences and the actual icon manipulation code.
 */

#ifndef LAYOUT_PROCESSOR_H
#define LAYOUT_PROCESSOR_H

#include <exec/types.h>
#include "layout_preferences.h"

/**
 * @brief Process directory with layout preferences
 * 
 * Applies the specified layout preferences to a directory's icons,
 * including sorting, positioning, and window resizing.
 * 
 * @param path Directory path to process (e.g., "SYS:", "DH0:Games")
 * @param recursive Process subdirectories recursively
 * @param prefs Pointer to LayoutPreferences structure with settings
 * @return TRUE if successful, FALSE on error
 * 
 * @note This function may take time for large directories or when
 *       processing recursively. Consider adding progress feedback.
 * 
 * @example
 * LayoutPreferences prefs;
 * InitLayoutPreferences(&prefs);
 * ApplyPreset(&prefs, PRESET_MODERN);
 * 
 * if (ProcessDirectoryWithPreferences("SYS:Utilities", FALSE, &prefs))
 * {
 *     printf("Processing complete!\n");
 * }
 */
BOOL ProcessDirectoryWithPreferences(const char *path, 
                                    BOOL recursive, 
                                    const LayoutPreferences *prefs);

#endif /* LAYOUT_PROCESSOR_H */
