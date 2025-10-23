/**
 * backup_system.c - iTidy Backup & Undo System Implementation
 * 
 * Implements safe backup functionality before tidying operations.
 * Creates LhA archives of icon files and drawer window settings.
 * 
 * Author: Kerry Thompson
 * Date: October 23, 2025
 */

#include "backup_system.h"
#include "writeLog.h"
#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

/*========================================================================*/
/* Local Helper Functions                                                */
/*========================================================================*/

/**
 * @brief Format current time for backup directory names
 */
static void FormatTimestamp(char *buffer, size_t bufSize) {
    time_t now;
    struct tm *timeinfo;
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(buffer, bufSize, BACKUP_TIMESTAMP_FORMAT, timeinfo);
}

/**
 * @brief Sanitize path for filename (replace : and / with _)
 */
static void SanitizePathForFilename(const char *path, char *sanitized) {
    int i = 0;
    while (*path && i < 100) {
        if (*path == ':' || *path == '/') {
            sanitized[i++] = '_';
        } else {
            sanitized[i++] = *path;
        }
        path++;
    }
    sanitized[i] = '\0';
}

/*========================================================================*/
/* Public Functions                                                      */
/*========================================================================*/

/**
 * Check if LhA archiver is available on the system
 */
BOOL IsLhaAvailable(void) {
    BPTR lock;
    
    /* Try common LhA paths */
    const char *paths[] = { LHA_PATH_1, LHA_PATH_2, LHA_PATH_3, NULL };
    
    for (int i = 0; paths[i] != NULL; i++) {
        lock = Lock((STRPTR)paths[i], ACCESS_READ);
        if (lock) {
            UnLock(lock);
            writeLog(LOG_INFO, "Found LhA at: %s", paths[i]);
            return TRUE;
        }
    }
    
    writeLog(LOG_WARNING, "LhA not found - backup functionality disabled");
    return FALSE;
}

/**
 * Get the full path to the LhA executable
 */
BOOL GetLhaPath(char *outPath) {
    BPTR lock;
    const char *paths[] = { LHA_PATH_1, LHA_PATH_2, LHA_PATH_3, NULL };
    
    for (int i = 0; paths[i] != NULL; i++) {
        lock = Lock((STRPTR)paths[i], ACCESS_READ);
        if (lock) {
            UnLock(lock);
            strcpy(outPath, paths[i]);
            return TRUE;
        }
    }
    
    return FALSE;
}

/**
 * Create directory recursively (like mkdir -p)
 */
BOOL CreateDirectoryRecursive(const char *path) {
    char tempPath[MAX_BACKUP_PATH];
    char *p;
    BPTR lock;
    
    /* Check if already exists */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock) {
        UnLock(lock);
        return TRUE;  /* Already exists */
    }
    
    /* Copy path and create each level */
    strncpy(tempPath, path, sizeof(tempPath) - 1);
    tempPath[sizeof(tempPath) - 1] = '\0';
    
    /* Find the volume/device separator */
    p = strchr(tempPath, ':');
    if (p) {
        p++;  /* Start after the colon */
    } else {
        p = tempPath;
    }
    
    /* Create each directory level */
    while (*p) {
        if (*p == '/') {
            *p = '\0';
            lock = CreateDir((STRPTR)tempPath);
            if (lock) {
                UnLock(lock);
            }
            /* Ignore errors - directory might already exist */
            *p = '/';
        }
        p++;
    }
    
    /* Create final directory */
    lock = CreateDir((STRPTR)tempPath);
    if (lock) {
        UnLock(lock);
        writeLog(LOG_INFO, "Created directory: %s", tempPath);
        return TRUE;
    }
    
    /* Check if it exists now (might have been created) */
    lock = Lock((STRPTR)tempPath, ACCESS_READ);
    if (lock) {
        UnLock(lock);
        return TRUE;
    }
    
    writeLog(LOG_ERROR, "Failed to create directory: %s", tempPath);
    return FALSE;
}

/**
 * Initialize a backup session
 */
BOOL InitBackupSession(BackupContext *ctx, const BackupPreferences *prefs) {
    char timestamp[32];
    BPTR manifestHandle;
    
    if (!ctx || !prefs) {
        return FALSE;
    }
    
    /* Clear context */
    memset(ctx, 0, sizeof(BackupContext));
    
    /* Check if backups are enabled */
    if (!prefs->enableUndoBackup) {
        writeLog(LOG_INFO, "Backups disabled in preferences");
        return FALSE;
    }
    
    /* Check if LhA is available */
    if (prefs->useLha && !GetLhaPath(ctx->lhaPath)) {
        writeLog(LOG_ERROR, "LhA not available - cannot create backups");
        return FALSE;
    }
    ctx->lhaAvailable = (ctx->lhaPath[0] != '\0');
    
    /* Generate timestamp for this run */
    FormatTimestamp(timestamp, sizeof(timestamp));
    
    /* Build run directory path */
    /* Format: <backupRoot>/Run_YYYY-MM-DD_HH-MM/ */
    snprintf(ctx->runDirectory, sizeof(ctx->runDirectory),
             "%s/%s%s",
             prefs->backupRootPath,
             BACKUP_RUN_PREFIX,
             timestamp);
    
    writeLog(LOG_INFO, "Initializing backup session: %s", ctx->runDirectory);
    
    /* Create the run directory */
    if (!CreateDirectoryRecursive(ctx->runDirectory)) {
        writeLog(LOG_ERROR, "Failed to create backup run directory");
        return FALSE;
    }
    
    /* Create manifest file */
    char manifestPath[MAX_BACKUP_PATH];
    snprintf(manifestPath, sizeof(manifestPath),
             "%s/%s", ctx->runDirectory, BACKUP_MANIFEST_FILE);
    
    manifestHandle = Open((STRPTR)manifestPath, MODE_NEWFILE);
    if (!manifestHandle) {
        writeLog(LOG_ERROR, "Failed to create manifest file: %s", manifestPath);
        return FALSE;
    }
    
    ctx->manifestFile = manifestHandle;
    ctx->startTime = time(NULL);
    
    /* Write manifest header */
    char header[256];
    snprintf(header, sizeof(header),
             "iTidy Backup Manifest\n"
             "Session: %s\n"
             "Started: %s\n"
             "----------------------------------------\n",
             timestamp, ctime(&ctx->startTime));
    
    Write(manifestHandle, header, strlen(header));
    
    writeLog(LOG_INFO, "Backup session initialized successfully");
    return TRUE;
}

/**
 * Close and finalize a backup session
 */
void CloseBackupSession(BackupContext *ctx) {
    if (!ctx) return;
    
    if (ctx->manifestFile) {
        char footer[256];
        time_t endTime = time(NULL);
        
        snprintf(footer, sizeof(footer),
                 "----------------------------------------\n"
                 "Session ended: %s"
                 "Folders backed up: %u\n"
                 "Failed backups: %u\n",
                 ctime(&endTime),
                 ctx->foldersBackedUp,
                 ctx->failedBackups);
        
        Write(ctx->manifestFile, footer, strlen(footer));
        Close(ctx->manifestFile);
        ctx->manifestFile = 0;
    }
    
    writeLog(LOG_INFO, "Backup session closed. Success: %u, Failed: %u",
             ctx->foldersBackedUp, ctx->failedBackups);
}

/**
 * Get the parent directory path from a full path
 */
BOOL GetParentPath(const char *fullPath, char *parentPath) {
    char *lastSlash;
    
    if (!fullPath || !parentPath) {
        return FALSE;
    }
    
    strcpy(parentPath, fullPath);
    
    /* Find last slash */
    lastSlash = strrchr(parentPath, '/');
    if (lastSlash) {
        *lastSlash = '\0';  /* Truncate at last slash */
        return TRUE;
    }
    
    /* No slash found - might be at root or device level */
    /* Check for colon (device separator) */
    lastSlash = strchr(parentPath, ':');
    if (lastSlash && *(lastSlash + 1) != '\0') {
        /* Has device and something after it */
        *(lastSlash + 1) = '\0';
        return TRUE;
    }
    
    /* Already at root */
    return FALSE;
}

/**
 * Get just the folder name from a full path
 */
void GetFolderName(const char *fullPath, char *folderName) {
    const char *lastSlash;
    
    if (!fullPath || !folderName) {
        folderName[0] = '\0';
        return;
    }
    
    /* Find last slash */
    lastSlash = strrchr(fullPath, '/');
    if (lastSlash) {
        strcpy(folderName, lastSlash + 1);
    } else {
        /* No slash - might be just after device */
        const char *colon = strchr(fullPath, ':');
        if (colon) {
            strcpy(folderName, colon + 1);
        } else {
            strcpy(folderName, fullPath);
        }
    }
}

/**
 * Generate archive filename from folder path
 */
void GenerateArchiveName(const char *folderPath, char *outName, ULONG timestamp) {
    char sanitized[108];
    char timeStr[32];
    
    /* Sanitize path (replace : and / with _) */
    SanitizePathForFilename(folderPath, sanitized);
    
    /* Format timestamp */
    if (timestamp == 0) {
        timestamp = time(NULL);
    }
    struct tm *timeinfo = localtime((time_t*)&timestamp);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d_%H-%M", timeinfo);
    
    /* Build filename: SanitizedPath_YYYY-MM-DD_HH-MM.lha */
    snprintf(outName, 108, "%s_%s.lha", sanitized, timeStr);
}

/**
 * Write entry to backup manifest
 */
void LogBackupManifest(BackupContext *ctx, const char *folderPath,
                       const char *archiveName, BOOL success) {
    if (!ctx || !ctx->manifestFile) return;
    
    char entry[512];
    time_t now = time(NULL);
    
    snprintf(entry, sizeof(entry),
             "[%s] %s: %s -> %s\n",
             success ? "OK" : "FAIL",
             ctime(&now),
             folderPath,
             archiveName);
    
    /* Remove newline from ctime */
    char *newline = strchr(entry, '\n');
    if (newline && *(newline + 1) != '\0') {
        memmove(newline, newline + 1, strlen(newline));
    }
    
    Write(ctx->manifestFile, entry, strlen(entry));
}

/**
 * Create backup archive for a single folder
 */
BOOL BackupFolder(BackupContext *ctx, const char *folderPath) {
    char archiveName[108];
    char archivePath[MAX_BACKUP_PATH];
    char parentPath[256];
    char folderName[64];
    char drawerIconPath[256];
    char command[MAX_COMMAND_LEN];
    LONG result;
    
    if (!ctx || !folderPath) {
        return FALSE;
    }
    
    if (!ctx->lhaAvailable) {
        writeLog(LOG_ERROR, "LhA not available");
        return FALSE;
    }
    
    writeLog(LOG_INFO, "Backing up folder: %s", folderPath);
    
    /* Generate archive name */
    GenerateArchiveName(folderPath, archiveName, ctx->startTime);
    snprintf(archivePath, sizeof(archivePath),
             "%s/%s", ctx->runDirectory, archiveName);
    
    /* Get parent path and folder name */
    if (!GetParentPath(folderPath, parentPath)) {
        writeLog(LOG_ERROR, "Cannot determine parent path for: %s", folderPath);
        ctx->failedBackups++;
        LogBackupManifest(ctx, folderPath, archiveName, FALSE);
        return FALSE;
    }
    
    GetFolderName(folderPath, folderName);
    snprintf(drawerIconPath, sizeof(drawerIconPath),
             "%s/%s.info", parentPath, folderName);
    
    writeLog(LOG_DEBUG, "Archive: %s", archivePath);
    writeLog(LOG_DEBUG, "Parent: %s", parentPath);
    writeLog(LOG_DEBUG, "Folder name: %s", folderName);
    writeLog(LOG_DEBUG, "Drawer icon: %s", drawerIconPath);
    
    /* Build LhA command to backup all .info files in the folder */
    /* Format: LhA a -q -r -m1 "archive.lha" "DH0:Path/Folder/#?.info" */
    snprintf(command, sizeof(command),
             "%s a -q -r -m1 \"%s\" \"%s/#?.info\"",
             ctx->lhaPath,
             archivePath,
             folderPath);
    
    writeLog(LOG_DEBUG, "Executing: %s", command);
    
    /* Execute LhA command */
    result = Execute((STRPTR)command, 0, 0);
    if (result == 0) {
        writeLog(LOG_ERROR, "Failed to backup folder contents: %s", folderPath);
        ctx->failedBackups++;
        LogBackupManifest(ctx, folderPath, archiveName, FALSE);
        return FALSE;
    }
    
    /* Now add the drawer's .info file (window settings) */
    /* This is CRITICAL - it contains window size, position, and view mode */
    BPTR lock = Lock((STRPTR)drawerIconPath, ACCESS_READ);
    if (lock) {
        UnLock(lock);
        
        /* Add drawer icon to archive */
        snprintf(command, sizeof(command),
                 "%s a -q -m1 \"%s\" \"%s\"",
                 ctx->lhaPath,
                 archivePath,
                 drawerIconPath);
        
        writeLog(LOG_DEBUG, "Adding drawer icon: %s", command);
        
        result = Execute((STRPTR)command, 0, 0);
        if (result == 0) {
            writeLog(LOG_WARNING, "Failed to backup drawer icon: %s", drawerIconPath);
            /* Don't fail the whole backup - contents are still backed up */
        }
    } else {
        writeLog(LOG_WARNING, "Drawer icon not found: %s", drawerIconPath);
        /* This is OK - some folders might not have custom icons */
    }
    
    ctx->foldersBackedUp++;
    LogBackupManifest(ctx, folderPath, archiveName, TRUE);
    
    writeLog(LOG_INFO, "Successfully backed up: %s -> %s", folderPath, archiveName);
    return TRUE;
}

/**
 * Restore a folder from backup archive
 */
BOOL RestoreFromBackup(const char *archivePath, const char *targetPath) {
    char command[MAX_COMMAND_LEN];
    char lhaPath[32];
    LONG result;
    
    if (!archivePath || !targetPath) {
        return FALSE;
    }
    
    /* Check if LhA is available */
    if (!GetLhaPath(lhaPath)) {
        writeLog(LOG_ERROR, "LhA not available - cannot restore backup");
        return FALSE;
    }
    
    /* Check if archive exists */
    BPTR lock = Lock((STRPTR)archivePath, ACCESS_READ);
    if (!lock) {
        writeLog(LOG_ERROR, "Backup archive not found: %s", archivePath);
        return FALSE;
    }
    UnLock(lock);
    
    writeLog(LOG_INFO, "Restoring from: %s to: %s", archivePath, targetPath);
    
    /* Build LhA extract command */
    /* Format: LhA x -q -r "archive.lha" "DH0:Path/Folder/" */
    snprintf(command, sizeof(command),
             "%s x -q -r \"%s\" \"%s/\"",
             lhaPath,
             archivePath,
             targetPath);
    
    writeLog(LOG_DEBUG, "Executing: %s", command);
    
    /* Execute extraction */
    result = Execute((STRPTR)command, 0, 0);
    if (result == 0) {
        writeLog(LOG_ERROR, "Failed to restore from backup: %s", archivePath);
        return FALSE;
    }
    
    writeLog(LOG_INFO, "Successfully restored from backup");
    return TRUE;
}

/**
 * Clean up old backup runs
 */
UWORD CleanupOldBackups(const BackupPreferences *prefs) {
    /* TODO: Implement backup retention policy */
    /* This will scan the backup directory and remove oldest runs */
    /* if count exceeds maxBackupsPerFolder */
    
    writeLog(LOG_INFO, "Backup cleanup not yet implemented");
    return 0;
}
