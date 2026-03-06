/**
 * backup_paths.h - iTidy Backup System Path Utilities
 * 
 * Provides path manipulation and validation functions for the backup system.
 * Handles root folder detection, hierarchical archive path calculation,
 * and AmigaDOS-specific path operations.
 * 
 * Design: BACKUP_SYSTEM_PROPOSAL.md
 * Implementation: BACKUP_IMPLEMENTATION_GUIDE.md - Phase 1, Task 2
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

#ifndef BACKUP_PATHS_H
#define BACKUP_PATHS_H

#include "backup_types.h"

/*========================================================================*/
/* Root Folder Detection                                                 */
/*========================================================================*/

/**
 * @brief Check if path is a root/volume path
 * 
 * Determines if a path points to a root directory (e.g., "DH0:", "Work:")
 * versus a subdirectory (e.g., "DH0:Projects", "Work:Documents").
 * 
 * This is critical for backup because root folders store their .info file
 * differently than subdirectories:
 * - Root: "DH0:" → window settings in "DH0:.info"
 * - Normal: "DH0:Projects/MyFolder" → settings in "DH0:Projects/MyFolder.info"
 * 
 * Examples:
 *   IsRootFolder("DH0:") → TRUE
 *   IsRootFolder("Work:") → TRUE
 *   IsRootFolder("RAM:") → TRUE
 *   IsRootFolder("DH0:Projects") → FALSE
 *   IsRootFolder("Work:Documents/Letters") → FALSE
 *   IsRootFolder("InvalidPath") → FALSE (no colon)
 * 
 * @param path AmigaDOS path to check
 * @return TRUE if path is a root/volume path, FALSE otherwise
 */
BOOL IsRootFolder(const char *path);

/*========================================================================*/
/* Archive Path Calculation                                              */
/*========================================================================*/

/**
 * @brief Calculate hierarchical archive path from index
 * 
 * Generates the full archive path including hierarchical subfolder.
 * Uses 100-file-per-folder structure for FFS performance.
 * 
 * Examples:
 *   CalculateArchivePath(buf, "PROGDIR:Backups/Run_0001", 1)
 *     → "PROGDIR:Backups/Run_0001/000/00001.lha"
 *   
 *   CalculateArchivePath(buf, "Work:iTidyBackups/Run_0042", 12345)
 *     → "Work:iTidyBackups/Run_0042/123/12345.lha"
 * 
 * Path structure:
 *   [runDirectory]/[folderNum]/[archiveIndex].lha
 *   
 * Where:
 *   folderNum = archiveIndex / 100 (formatted as 3 digits: 000, 001, 123)
 *   archiveIndex = formatted as 5 digits: 00001, 00042, 12345
 * 
 * @param outPath Buffer for output path (min MAX_BACKUP_PATH bytes)
 * @param runDirectory Full path to Run_NNNN directory
 * @param archiveIndex Archive index (1-99999)
 * @return TRUE on success, FALSE if path would be too long
 */
BOOL CalculateArchivePath(char *outPath, const char *runDirectory, ULONG archiveIndex);

/**
 * @brief Calculate hierarchical subfolder path only
 * 
 * Generates just the subfolder path component within a run directory.
 * 
 * Examples:
 *   CalculateSubfolderPath(buf, "PROGDIR:Backups/Run_0001", 1)
 *     → "PROGDIR:Backups/Run_0001/000"
 *   
 *   CalculateSubfolderPath(buf, "Work:Backups/Run_0001", 12345)
 *     → "Work:Backups/Run_0001/123"
 * 
 * @param outPath Buffer for output path (min MAX_BACKUP_PATH bytes)
 * @param runDirectory Full path to Run_NNNN directory
 * @param archiveIndex Archive index (1-99999)
 * @return TRUE on success, FALSE if path would be too long
 */
BOOL CalculateSubfolderPath(char *outPath, const char *runDirectory, ULONG archiveIndex);

/**
 * @brief Format archive filename from index
 * 
 * Generates just the archive filename component (5-digit index + .lha).
 * 
 * Examples:
 *   FormatArchiveName(buf, 1) → "00001.lha"
 *   FormatArchiveName(buf, 42) → "00042.lha"
 *   FormatArchiveName(buf, 12345) → "12345.lha"
 * 
 * @param outName Buffer for filename (min MAX_ARCHIVE_NAME bytes)
 * @param archiveIndex Archive index (1-99999)
 */
void FormatArchiveName(char *outName, ULONG archiveIndex);

/*========================================================================*/
/* Path Manipulation                                                     */
/*========================================================================*/

/**
 * @brief Get the parent directory path from a full path
 * 
 * Extracts the parent directory portion of an AmigaDOS path.
 * 
 * Examples:
 *   GetParentPath("DH0:Projects/MyFolder", buf)
 *     → "DH0:Projects"
 *   
 *   GetParentPath("DH0:Projects/Client/Work", buf)
 *     → "DH0:Projects/Client"
 *   
 *   GetParentPath("DH0:TopLevel", buf)
 *     → "DH0:"
 *   
 *   GetParentPath("DH0:", buf)
 *     → FALSE (already at root)
 * 
 * @param fullPath Input path
 * @param parentPath Buffer for parent path (min MAX_BACKUP_PATH bytes)
 * @return TRUE if parent extracted, FALSE if already at root
 */
BOOL GetParentPath(const char *fullPath, char *parentPath);

/**
 * @brief Get just the folder name from a full path
 * 
 * Extracts the final component (folder/file name) from a path.
 * 
 * Examples:
 *   GetFolderName("DH0:Projects/MyFolder", buf)
 *     → "MyFolder"
 *   
 *   GetFolderName("Work:Documents/Letters/Personal", buf)
 *     → "Personal"
 *   
 *   GetFolderName("DH0:TopLevel", buf)
 *     → "TopLevel"
 *   
 *   GetFolderName("DH0:", buf)
 *     → "" (empty, at root)
 * 
 * @param fullPath Input path
 * @param folderName Buffer for folder name (min 64 bytes recommended)
 */
void GetFolderName(const char *fullPath, char *folderName);

/**
 * @brief Get the drawer icon path for a folder
 * 
 * Calculates the path to a folder's .info file, handling both root
 * and normal folders correctly.
 * 
 * For root folders (e.g., "DH0:"):
 *   GetDrawerIconPath("DH0:", buf) → "DH0:.info"
 * 
 * For normal folders (e.g., "DH0:Projects/MyFolder"):
 *   GetDrawerIconPath("DH0:Projects/MyFolder", buf)
 *     → "DH0:Projects/MyFolder.info"
 * 
 * Note: For normal folders, this returns the same path + ".info".
 * For root folders, it returns "[volume]:.info".
 * 
 * @param folderPath Full path to the folder
 * @param iconPath Buffer for icon path (min MAX_BACKUP_PATH bytes)
 * @return TRUE on success, FALSE on error
 */
BOOL GetDrawerIconPath(const char *folderPath, char *iconPath);

/*========================================================================*/
/* Path Validation                                                       */
/*========================================================================*/

/**
 * @brief Check if a path length is FFS-safe
 * 
 * Validates that a path doesn't exceed AmigaDOS FFS limits:
 * - Per-component: 30 characters max
 * - Total path: 107 characters max (some systems 256)
 * 
 * @param path Path to validate
 * @return TRUE if path is FFS-safe, FALSE if too long
 */
BOOL IsPathFfsSafe(const char *path);

/**
 * @brief Get the total path length
 * 
 * @param path Path to measure
 * @return Length in characters
 */
UWORD GetPathLength(const char *path);

/*========================================================================*/
/* Path Component Extraction                                             */
/*========================================================================*/

/**
 * @brief Extract the device/volume name from a path
 * 
 * Examples:
 *   GetDeviceName("DH0:Projects/MyFolder", buf) → "DH0"
 *   GetDeviceName("Work:Documents", buf) → "Work"
 *   GetDeviceName("RAM:", buf) → "RAM"
 *   GetDeviceName("InvalidPath", buf) → "" (empty, no colon)
 * 
 * @param path Input path
 * @param deviceName Buffer for device name (min 32 bytes)
 * @return TRUE if device name extracted, FALSE if no device separator
 */
BOOL GetDeviceName(const char *path, char *deviceName);

#endif /* BACKUP_PATHS_H */
