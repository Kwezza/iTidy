/**
 * folder_scanner.h - Fast Folder Pre-Scanning for Progress Tracking
 * 
 * Provides lightweight directory tree scanning to count folders containing
 * icons without loading or processing any icon data. Designed for speed
 * to enable accurate progress bars during recursive processing.
 * 
 * Performance: 10-100x faster than full icon processing
 *   - Uses MatchFirst/MatchNext pattern matching
 *   - Early exit on first .info file found
 *   - No DiskObject allocation or icon loading
 *   - Minimal memory footprint
 */

#ifndef FOLDER_SCANNER_H
#define FOLDER_SCANNER_H

#include <exec/types.h>
#include "layout_preferences.h"

/**
 * @brief Fast pre-scan to count folders containing icons
 * 
 * Recursively walks directory tree and counts folders that contain
 * at least one .info file. Uses MatchFirst with early exit for speed.
 * Does NOT load icons, just checks for existence.
 * 
 * This is designed as a lightweight pre-scan phase before actual icon
 * processing to enable accurate progress tracking. The function respects
 * skipHiddenFolders preference (folders without .info files are skipped).
 * 
 * @param path Starting directory path
 * @param prefs Layout preferences (uses skipHiddenFolders setting)
 * @param folderCount Pointer to counter (incremented for each folder with icons)
 * @return TRUE if scan completed successfully, FALSE on error
 * 
 * @note Typical performance: ~1-5ms per folder vs ~50-500ms for full icon loading
 * 
 * @example
 *   ULONG totalFolders = 0;
 *   const LayoutPreferences *prefs = GetGlobalPreferences();
 *   
 *   if (CountFoldersWithIcons("Work:Projects", prefs, &totalFolders))
 *   {
 *       printf("Found %lu folders with icons\n", totalFolders);
 *       // Initialize progress bar with totalFolders as maximum
 *   }
 */
BOOL CountFoldersWithIcons(const char *path, 
                           const LayoutPreferences *prefs,
                           ULONG *folderCount);

#endif /* FOLDER_SCANNER_H */
