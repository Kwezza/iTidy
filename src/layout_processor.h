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
 * Applies the global layout preferences to a directory's icons,
 * including sorting, positioning, and window resizing.
 * 
 * Uses the global preferences set via UpdateGlobalPreferences().
 * The path and recursive mode are also read from global preferences.
 * 
 * @return TRUE if successful, FALSE on error
 * 
 * @note This function may take time for large directories or when
 *       processing recursively. Consider adding progress feedback.
 * 
 * @example
 * // Set preferences first
 * LayoutPreferences prefs;
 * InitLayoutPreferences(&prefs);
 * ApplyPreset(&prefs, PRESET_MODERN);
 * strcpy(prefs.folder_path, "SYS:Utilities");
 * prefs.recursive_subdirs = FALSE;
 * UpdateGlobalPreferences(&prefs);
 * 
 * // Then process
 * if (ProcessDirectoryWithPreferences())
 * {
 *     printf("Processing complete!\n");
 * }
 */
BOOL ProcessDirectoryWithPreferences(void);

/**
 * @brief Process directory with progress window integration
 * 
 * Same as ProcessDirectoryWithPreferences() but routes all status
 * updates to the provided progress window. Supports cancellation
 * via the progress window's Cancel button.
 * 
 * @param progress_window Pointer to initialized progress window
 * @return TRUE if successful, FALSE on error or cancellation
 * 
 * @note The progress window must be opened by the caller before
 *       calling this function, and closed after it returns.
 * 
 * @example
 * struct iTidyMainProgressWindow progress_window;
 * if (itidy_main_progress_window_open(&progress_window))
 * {
 *     BOOL success = ProcessDirectoryWithPreferencesAndProgress(&progress_window);
 *     itidy_main_progress_window_close(&progress_window);
 * }
 */
struct iTidyMainProgressWindow;
BOOL ProcessDirectoryWithPreferencesAndProgress(struct iTidyMainProgressWindow *progress_window);

/**
 * @brief Scan directory for default tools only (no tidying)
 * 
 * Walks through directories and loads icons purely to validate their
 * default tools and populate the tool cache. Does NOT sort, layout,
 * resize, or save any changes to icons.
 * 
 * Uses the global preferences for path, recursive mode, and skipHiddenFolders.
 * 
 * This is useful for rebuilding the tool cache in the Tool Cache Window
 * without modifying any icon positions.
 * 
 * @return TRUE if scan completed successfully, FALSE on error
 * 
 * @note The tool cache remains active after scanning. Call FreeToolCache()
 *       when you're done viewing the results.
 * 
 * @example
 * // Set path and recursive mode first
 * SetGlobalScanPath("Work:");
 * SetGlobalRecursiveMode(TRUE);
 * 
 * // Rebuild tool cache for Tool Cache Window
 * if (ScanDirectoryForToolsOnly())
 * {
 *     printf("Tool cache rebuilt - %d tools found\n", g_ToolCacheCount);
 *     DumpToolCache();
 * }
 */
BOOL ScanDirectoryForToolsOnly(void);

/**
 * @brief Scan directory for default tools with progress window integration
 * 
 * Same as ScanDirectoryForToolsOnly() but routes all status updates to the
 * provided progress window. Supports cancellation via the progress window's
 * Cancel button.
 * 
 * @param progress_window Pointer to initialized progress window
 * @return TRUE if scan completed successfully, FALSE on error or cancellation
 * 
 * @note The progress window must be opened by the caller before
 *       calling this function, and closed after it returns.
 * 
 * @example
 * struct iTidyMainProgressWindow progress_window;
 * if (itidy_main_progress_window_open(&progress_window))
 * {
 *     BOOL success = ScanDirectoryForToolsOnlyWithProgress(&progress_window);
 *     itidy_main_progress_window_close(&progress_window);
 * }
 */
BOOL ScanDirectoryForToolsOnlyWithProgress(struct iTidyMainProgressWindow *progress_window);

#endif /* LAYOUT_PROCESSOR_H */
