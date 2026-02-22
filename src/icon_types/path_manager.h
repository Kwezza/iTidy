#ifndef ICON_TYPES_PATH_MANAGER_H
#define ICON_TYPES_PATH_MANAGER_H

/**
 * @file path_manager.h
 * @brief System PATH management for tool validation
 * 
 * This module manages the system PATH search list used for validating
 * default tool executables. It parses startup scripts (S:Startup-Sequence
 * and S:User-Startup) to extract PATH commands and build a search list.
 */

#include <exec/types.h>

/*========================================================================*/
/* PATH Search List Management                                            */
/*========================================================================*/

/**
 * @brief Build PATH search list from startup scripts
 * 
 * Parses S:Startup-Sequence and S:User-Startup for "Path" and "Path ADD"
 * commands to build a global array of search directories. This should be
 * called once at program startup.
 * 
 * Implementation:
 * 1. Opens and parses S:Startup-Sequence
 * 2. Opens and parses S:User-Startup
 * 3. Handles both "Path <dirs>" (replace) and "Path <dirs> ADD" (append)
 * 4. Normalizes paths (ensures trailing / or :)
 * 5. Falls back to hardcoded paths if scripts unavailable
 * 
 * Fallback paths: C:, SYS:Utilities/, SYS:Tools/, SYS:System/, S:
 * 
 * @return TRUE if PATH list was built successfully, FALSE on error
 */
BOOL BuildPathSearchList(void);

/**
 * @brief Free the PATH search list
 * 
 * Releases all memory allocated by BuildPathSearchList(). Should be
 * called at program shutdown.
 */
void FreePathSearchList(void);

/*========================================================================*/
/* Public Accessors (defined in icon_types_internal.h)                   */
/*========================================================================*/

/**
 * Global PATH search list variables (read-only access)
 * Managed internally by path_manager.c
 * 
 * extern char **g_PathSearchList;
 * extern int g_PathSearchCount;
 */

#endif /* ICON_TYPES_PATH_MANAGER_H */
