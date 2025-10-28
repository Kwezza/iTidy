/**
 * backup_session.h - iTidy Backup Session Manager Interface
 * 
 * Provides high-level backup session management, integrating all
 * backup subsystems (paths, runs, catalog, LHA, markers).
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 * Task: 7 - Session Manager
 */

#ifndef BACKUP_SESSION_H
#define BACKUP_SESSION_H

#include "backup_types.h"
#include "layout_preferences.h"

#ifdef __cplusplus
extern "C" {
#endif

/*========================================================================*/
/* Session Management                                                     */
/*========================================================================*/

/**
 * Initialize a backup session.
 * 
 * Creates a new Run_NNNN directory, initializes the catalog,
 * and prepares the context for folder backups.
 * 
 * @param ctx Backup context to initialize
 * @param prefs Backup preferences
 * @param sourceDirectory Optional source directory being tidied (can be NULL)
 * @return TRUE on success, FALSE on failure
 * 
 * @note Call CloseBackupSession() when done to finalize the catalog.
 */
BOOL InitBackupSession(BackupContext *ctx, const BackupPreferences *prefs, const char *sourceDirectory);

/**
 * Backup a single folder's icon layout.
 * 
 * This is the main backup operation that:
 * 1. Checks if folder has .info files
 * 2. Creates an LHA archive of all icons
 * 3. Adds a path marker to the archive
 * 4. Logs the operation to the catalog
 * 
 * @param ctx Active backup context (from InitBackupSession)
 * @param folderPath Full path to folder (e.g., "DH0:Projects/MyApp/")
 * @return BACKUP_OK on success, error code on failure
 * 
 * @note Root folders (e.g., "DH0:") are handled specially.
 * @note Empty folders (no .info files) are skipped automatically.
 */
BackupStatus BackupFolder(BackupContext *ctx, const char *folderPath);

/**
 * Close a backup session.
 * 
 * Finalizes the catalog file with statistics and closes all resources.
 * 
 * @param ctx Active backup context
 * 
 * @note Always call this, even if backups failed, to ensure proper cleanup.
 */
void CloseBackupSession(BackupContext *ctx);

/*========================================================================*/
/* Utility Functions                                                      */
/*========================================================================*/

/**
 * Check if a folder contains any .info files.
 * 
 * @param folderPath Path to folder
 * @return TRUE if .info files exist, FALSE otherwise
 */
BOOL FolderHasInfoFiles(const char *folderPath);

#ifdef __cplusplus
}
#endif

#endif /* BACKUP_SESSION_H */
