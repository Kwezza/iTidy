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
 * @param iconCount Number of icons being backed up (0 to auto-count)
 * @return BACKUP_OK on success, error code on failure
 * 
 * @note Root folders (e.g., "DH0:") are handled specially.
 * @note Empty folders (no .info files) are skipped automatically.
 */
BackupStatus BackupFolder(BackupContext *ctx, const char *folderPath, UWORD iconCount);

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
/* Created Icons Manifest (DefIcons Integration)                          */
/*========================================================================*/

/**
 * Open the created_icons.txt manifest file in the run directory.
 * 
 * This file records every .info file that DefIcons creates during the run,
 * so that a restore can delete these files to return the folder to its
 * exact pre-iTidy state.
 * 
 * @param ctx Active backup context (from InitBackupSession)
 * @return TRUE on success, FALSE on failure
 * 
 * @note Call CloseCreatedIconsManifest() when done.
 */
BOOL OpenCreatedIconsManifest(BackupContext *ctx);

/**
 * Log a single created .info file path to the manifest.
 * 
 * Appends the path immediately and flushes for crash safety.
 * Also increments ctx->iconsCreated.
 * 
 * @param ctx Active backup context with open manifest
 * @param info_path Full path to the created .info file
 */
void LogCreatedIconToManifest(BackupContext *ctx, const char *info_path);

/**
 * Close the created_icons.txt manifest file.
 * 
 * @param ctx Active backup context
 */
void CloseCreatedIconsManifest(BackupContext *ctx);

/**
 * Read the created_icons.txt manifest and return count of entries.
 * 
 * @param run_directory Path to run directory (e.g., "PROGDIR:Backups/Run_0001")
 * @return Number of entries in manifest, or 0 if not found
 */
ULONG CountCreatedIconsInManifest(const char *run_directory);

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

/**
 * Count the number of .info files in a folder.
 * 
 * @param folderPath Path to folder
 * @return Number of .info files found (0 if none or error)
 */
UWORD CountInfoFiles(const char *folderPath);

#ifdef __cplusplus
}
#endif

#endif /* BACKUP_SESSION_H */
