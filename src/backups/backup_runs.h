/**
 * backup_runs.h - iTidy Backup System Run Management
 * 
 * Manages backup run directories and sequential run numbering.
 * Handles scanning for existing runs and creating new Run_NNNN directories.
 * 
 * Design: BACKUP_SYSTEM_PROPOSAL.md
 * Implementation: BACKUP_IMPLEMENTATION_GUIDE.md - Phase 1, Task 3
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

#ifndef BACKUP_RUNS_H
#define BACKUP_RUNS_H

#include "backup_types.h"

/*========================================================================*/
/* Run Number Management                                                 */
/*========================================================================*/

/**
 * @brief Find the highest existing run number in backup directory
 * 
 * Scans the backup root directory for Run_NNNN subdirectories and
 * returns the highest run number found. Used to determine the next
 * sequential run number.
 * 
 * Examples:
 *   Directory contains: Run_0001/, Run_0002/, Run_0005/
 *   FindHighestRunNumber("PROGDIR:Backups") → 5
 * 
 *   Directory is empty or has no Run_NNNN folders:
 *   FindHighestRunNumber("PROGDIR:Backups") → 0
 * 
 * The function ignores:
 * - Non-directory entries
 * - Directories not matching "Run_NNNN" pattern
 * - Invalid run numbers (e.g., Run_ABCD, Run_00000)
 * 
 * @param backupRoot Full path to backup root directory
 * @return Highest run number found, or 0 if none found
 */
UWORD FindHighestRunNumber(const char *backupRoot);

/**
 * @brief Create the next sequential run directory
 * 
 * Scans for existing runs, determines next run number, and creates
 * the new Run_NNNN directory. Also creates the backup root if needed.
 * 
 * Process:
 * 1. Create backup root directory if it doesn't exist
 * 2. Scan for highest existing run number
 * 3. Increment to get next run number
 * 4. Create Run_NNNN directory
 * 5. Return full path and run number
 * 
 * Examples:
 *   Existing runs: Run_0001/, Run_0002/
 *   CreateNextRunDirectory("PROGDIR:Backups", path, &runNum)
 *     → Creates "PROGDIR:Backups/Run_0003"
 *     → Sets runNum = 3
 *     → Returns TRUE
 * 
 *   No existing runs:
 *   CreateNextRunDirectory("PROGDIR:Backups", path, &runNum)
 *     → Creates "PROGDIR:Backups/Run_0001"
 *     → Sets runNum = 1
 *     → Returns TRUE
 * 
 * @param backupRoot Full path to backup root directory
 * @param outRunPath Buffer for new run directory path (min MAX_BACKUP_PATH)
 * @param outRunNumber Pointer to receive the new run number
 * @return TRUE on success, FALSE on failure
 */
BOOL CreateNextRunDirectory(const char *backupRoot, char *outRunPath, UWORD *outRunNumber);

/**
 * @brief Format a run directory name from run number
 * 
 * Generates the Run_NNNN directory name with zero-padded 4-digit number.
 * 
 * Examples:
 *   FormatRunDirectoryName(buf, 1) → "Run_0001"
 *   FormatRunDirectoryName(buf, 42) → "Run_0042"
 *   FormatRunDirectoryName(buf, 9999) → "Run_9999"
 * 
 * @param outName Buffer for directory name (min MAX_RUN_DIR_NAME bytes)
 * @param runNumber Run number (1-9999)
 */
void FormatRunDirectoryName(char *outName, UWORD runNumber);

/**
 * @brief Parse run number from Run_NNNN directory name
 * 
 * Extracts the numeric portion from a run directory name.
 * Returns 0 if the name doesn't match the expected pattern.
 * 
 * Examples:
 *   ParseRunNumber("Run_0001") → 1
 *   ParseRunNumber("Run_0042") → 42
 *   ParseRunNumber("Run_9999") → 9999
 *   ParseRunNumber("InvalidName") → 0
 *   ParseRunNumber("Run_ABCD") → 0
 * 
 * @param dirName Directory name to parse
 * @return Run number, or 0 if invalid
 */
UWORD ParseRunNumber(const char *dirName);

/**
 * @brief Check if backup root directory exists
 * 
 * Verifies that the backup root directory exists and is accessible.
 * 
 * @param backupRoot Full path to backup root directory
 * @return TRUE if exists and accessible, FALSE otherwise
 */
BOOL BackupRootExists(const char *backupRoot);

/**
 * @brief Create backup root directory if it doesn't exist
 * 
 * Creates the backup root directory and any parent directories needed.
 * Uses recursive directory creation (like mkdir -p).
 * 
 * @param backupRoot Full path to backup root directory
 * @return TRUE on success or if already exists, FALSE on failure
 */
BOOL CreateBackupRoot(const char *backupRoot);

/**
 * @brief Get full path to a specific run directory
 * 
 * Builds the full path to a run directory given the backup root
 * and run number.
 * 
 * Example:
 *   GetRunDirectoryPath(buf, "PROGDIR:Backups", 42)
 *     → "PROGDIR:Backups/Run_0042"
 * 
 * @param outPath Buffer for full path (min MAX_BACKUP_PATH bytes)
 * @param backupRoot Backup root directory
 * @param runNumber Run number
 * @return TRUE on success, FALSE if path too long
 */
BOOL GetRunDirectoryPath(char *outPath, const char *backupRoot, UWORD runNumber);

/**
 * @brief Count total number of run directories
 * 
 * Scans backup root and counts all valid Run_NNNN directories.
 * Useful for implementing retention policies and statistics.
 * 
 * @param backupRoot Full path to backup root directory
 * @return Number of run directories found
 */
UWORD CountRunDirectories(const char *backupRoot);

#endif /* BACKUP_RUNS_H */
