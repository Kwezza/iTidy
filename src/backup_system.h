/**
 * backup_system.h - iTidy Backup & Undo System
 * 
 * Provides safe backup functionality before tidying operations.
 * Creates LhA archives of icon files and drawer window settings.
 * 
 * Author: Kerry Thompson
 * Date: October 23, 2025
 */

#ifndef BACKUP_SYSTEM_H
#define BACKUP_SYSTEM_H

#include <exec/types.h>
#include "layout_preferences.h"

/*========================================================================*/
/* Constants                                                             */
/*========================================================================*/

#define BACKUP_DEFAULT_SUBDIR "Backups"
#define BACKUP_RUN_PREFIX "Run_"
#define BACKUP_TIMESTAMP_FORMAT "%Y-%m-%d_%H-%M"
#define BACKUP_MANIFEST_FILE "manifest.txt"

/* LhA command paths to check */
#define LHA_PATH_1 "C:LhA"
#define LHA_PATH_2 "C:lha"
#define LHA_PATH_3 "C:LHA"

/* Maximum path lengths */
#define MAX_BACKUP_PATH 256
#define MAX_COMMAND_LEN 512

/*========================================================================*/
/* Backup Context Structure                                              */
/*========================================================================*/

/**
 * @brief Context for a single backup session
 * 
 * Maintains state for the current backup run, including paths,
 * timestamps, and statistics.
 */
typedef struct {
    char runDirectory[MAX_BACKUP_PATH];    /* Full path to Run_YYYY-MM-DD_HH-MM/ */
    char lhaPath[32];                      /* Path to LhA executable */
    BOOL lhaAvailable;                     /* TRUE if LhA was found */
    ULONG startTime;                       /* Backup session start time (seconds) */
    UWORD foldersBackedUp;                 /* Count of backed-up folders */
    UWORD failedBackups;                   /* Count of failed backups */
    BPTR manifestFile;                     /* Open file handle for manifest */
} BackupContext;

/*========================================================================*/
/* Function Prototypes                                                   */
/*========================================================================*/

/**
 * @brief Check if LhA archiver is available on the system
 * 
 * Searches common locations for the LhA executable.
 * 
 * @return TRUE if LhA is found, FALSE otherwise
 */
BOOL IsLhaAvailable(void);

/**
 * @brief Get the full path to the LhA executable
 * 
 * @param outPath Buffer to store the path (min 32 bytes)
 * @return TRUE if found and path stored, FALSE otherwise
 */
BOOL GetLhaPath(char *outPath);

/**
 * @brief Initialize a backup session
 * 
 * Creates the backup directory structure for this run.
 * Format: <backupRoot>/Run_YYYY-MM-DD_HH-MM/
 * 
 * @param ctx Pointer to BackupContext to initialize
 * @param prefs Layout preferences containing backup settings
 * @return TRUE on success, FALSE on failure
 */
BOOL InitBackupSession(BackupContext *ctx, const BackupPreferences *prefs);

/**
 * @brief Close and finalize a backup session
 * 
 * Closes manifest file and performs cleanup.
 * 
 * @param ctx Pointer to BackupContext to close
 */
void CloseBackupSession(BackupContext *ctx);

/**
 * @brief Create backup archive for a single folder
 * 
 * Backs up:
 * 1. All .info files inside the target folder
 * 2. The folder's own .info file from the parent directory
 *    (contains window position, size, and view mode)
 * 
 * Archive naming: Device_Path_YYYY-MM-DD_HH-MM.lha
 * Example: DH0_Projects_MyFolder_2025-10-23_14-30.lha
 * 
 * @param ctx Pointer to active BackupContext
 * @param folderPath Full path to folder being backed up (e.g., "DH0:Projects/MyFolder")
 * @return TRUE on success, FALSE on failure
 */
BOOL BackupFolder(BackupContext *ctx, const char *folderPath);

/**
 * @brief Restore a folder from backup archive
 * 
 * Extracts all .info files from the backup archive back to
 * their original locations, restoring icon positions and
 * window settings.
 * 
 * @param archivePath Full path to backup .lha file
 * @param targetPath Target folder path (should match original)
 * @return TRUE on success, FALSE on failure
 */
BOOL RestoreFromBackup(const char *archivePath, const char *targetPath);

/**
 * @brief Clean up old backup runs
 * 
 * Removes oldest backup run directories if the count exceeds
 * maxBackupsPerFolder setting.
 * 
 * @param prefs Layout preferences containing backup settings
 * @return Number of backup runs deleted
 */
UWORD CleanupOldBackups(const BackupPreferences *prefs);

/**
 * @brief Generate archive filename from folder path
 * 
 * Converts path like "DH0:Projects/MyFolder" to
 * "DH0_Projects_MyFolder_2025-10-23_14-30.lha"
 * 
 * @param folderPath Input folder path
 * @param outName Buffer for output filename (min 108 bytes)
 * @param timestamp Time to use for filename (or 0 for current)
 */
void GenerateArchiveName(const char *folderPath, char *outName, ULONG timestamp);

/**
 * @brief Write entry to backup manifest
 * 
 * Logs backup operations for debugging and restore purposes.
 * 
 * @param ctx Backup context with open manifest file
 * @param folderPath Path that was backed up
 * @param archiveName Name of created archive
 * @param success TRUE if backup succeeded
 */
void LogBackupManifest(BackupContext *ctx, const char *folderPath,
                       const char *archiveName, BOOL success);

/**
 * @brief Get the parent directory path from a full path
 * 
 * Example: "DH0:Projects/MyFolder" -> "DH0:Projects"
 * 
 * @param fullPath Input path
 * @param parentPath Output buffer for parent (min 108 bytes)
 * @return TRUE if parent path extracted, FALSE if at root
 */
BOOL GetParentPath(const char *fullPath, char *parentPath);

/**
 * @brief Get just the folder name from a full path
 * 
 * Example: "DH0:Projects/MyFolder" -> "MyFolder"
 * 
 * @param fullPath Input path
 * @param folderName Output buffer for name (min 32 bytes)
 */
void GetFolderName(const char *fullPath, char *folderName);

/**
 * @brief Create directory recursively (like mkdir -p)
 * 
 * @param path Directory path to create
 * @return TRUE on success, FALSE on failure
 */
BOOL CreateDirectoryRecursive(const char *path);

#endif /* BACKUP_SYSTEM_H */
