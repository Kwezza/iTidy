/**
 * tool_scanner.h - Default Tool Cache Scanner
 *
 * Provides directory scanning to build the default tool validation cache
 * without modifying or saving any icon positions.  Complements
 * the main icon-tidying pipeline in layout_processor.c.
 *
 * Part of the layout subsystem (src/layout/).
 *
 * @note The tool cache remains loaded after scanning.
 *       Call FreeToolCache() when you are done with the results.
 */

#ifndef TOOL_SCANNER_H
#define TOOL_SCANNER_H

#include <exec/types.h>

/* Forward declaration - avoid pulling in the full progress window header */
struct iTidyMainProgressWindow;

/**
 * @brief Scan directories and build default tool cache (no tidying)
 *
 * Walks through the directory specified in global preferences, loading
 * icons purely to validate their default tools and populate the tool
 * cache.  Does NOT sort, lay out, resize, or save any changes to icons.
 *
 * @return TRUE if scan completed successfully, FALSE on error
 */
BOOL ScanDirectoryForToolsOnly(void);

/**
 * @brief Scan for default tools with progress window integration
 *
 * Same as ScanDirectoryForToolsOnly() but routes all status updates
 * to the provided progress window and supports cancellation.
 *
 * The global progress window pointer (in layout_processor.c) must be
 * set before calling the inner scanning functions; this wrapper takes
 * care of that via GetCurrentProgressWindow().
 *
 * @param progress_window Pointer to an opened progress window
 * @return TRUE if scan completed, FALSE on error or user cancellation
 */
BOOL ScanDirectoryForToolsOnlyWithProgress(struct iTidyMainProgressWindow *progress_window);

#endif /* TOOL_SCANNER_H */
