/**
 * backup_session.c - iTidy Backup Session Manager Implementation
 * 
 * Integrates all backup subsystems into a cohesive session API.
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 * Task: 7 - Session Manager
 */

/* Platform-specific includes */
#ifdef PLATFORM_HOST
    #ifdef _WIN32
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
        #include <direct.h>
    #endif
    #include <sys/stat.h>
    #include <dirent.h>
    #include <unistd.h>
    #define DEBUG_LOG(fmt, ...) printf("[DEBUG] " fmt "\n", __VA_ARGS__)
#else
    #include <dos/dos.h>
    #include <dos/dosasl.h>
    #include <proto/dos.h>
    #include <proto/exec.h>
    #include "writeLog.h"
    #define DEBUG_LOG(...) /* disabled on Amiga */
#endif

#include "backup_session.h"
#include "layout_preferences.h"
#include "backup_paths.h"
#include "backup_runs.h"
#include "backup_catalog.h"
#include "backup_lha.h"
#include "backup_marker.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*========================================================================*/
/* Session Management                                                     */
/*========================================================================*/

BOOL InitBackupSession(BackupContext *ctx, const BackupPreferences *prefs, const char *sourceDirectory) {
    if (!ctx || !prefs) {
        DEBUG_LOG("Invalid parameters for InitBackupSession");
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] ERROR: Invalid parameters for InitBackupSession\n");
#endif
        return FALSE;
    }
    
    /* Validate preferences */
    if (!prefs->enableUndoBackup || !prefs->useLha) {
        DEBUG_LOG("Backup disabled in preferences");
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] Backup disabled in preferences\n");
#endif
        return FALSE;
    }
    
    if (strlen(prefs->backupRootPath) == 0) {
        DEBUG_LOG("Backup root path not set");
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] ERROR: Backup root path not set\n");
#endif
        return FALSE;
    }
    
    /* Initialize context to known state */
    memset(ctx, 0, sizeof(BackupContext));
    
    /* Store source directory if provided */
    if (sourceDirectory && sourceDirectory[0] != '\0') {
        strncpy(ctx->sourceDirectory, sourceDirectory, sizeof(ctx->sourceDirectory) - 1);
        ctx->sourceDirectory[sizeof(ctx->sourceDirectory) - 1] = '\0';
    }
    
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Initializing backup session...\n");
    append_to_log("[BACKUP] Root path: %s\n", prefs->backupRootPath);
    if (ctx->sourceDirectory[0] != '\0') {
        append_to_log("[BACKUP] Source directory: %s\n", ctx->sourceDirectory);
    }
#endif
    
    /* Check if LHA is available */
    if (!CheckLhaAvailable(ctx->lhaPath)) {
        DEBUG_LOG("LHA not found - backup disabled");
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] ERROR: LHA not found - backup disabled\n");
#endif
        ctx->lhaAvailable = FALSE;
        return FALSE;
    }
    ctx->lhaAvailable = TRUE;
    DEBUG_LOG("LHA found at: %s", ctx->lhaPath);
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] LHA found at: %s\n", ctx->lhaPath);
#endif
    
    /* Get next run number and create run directory */
    if (!CreateNextRunDirectory(prefs->backupRootPath, ctx->runDirectory, &ctx->runNumber)) {
        DEBUG_LOG("Failed to create run directory");
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] ERROR: Failed to create run directory\n");
#endif
        return FALSE;
    }
    DEBUG_LOG("Created run directory: %s (run %d)", ctx->runDirectory, ctx->runNumber);
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Created run directory: %s (run %u)\n", ctx->runDirectory, ctx->runNumber);
#endif
    
    /* Create catalog */
    if (!CreateCatalog(ctx)) {
        DEBUG_LOG("Failed to create catalog");
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] ERROR: Failed to create catalog\n");
#endif
        return FALSE;
    }
    DEBUG_LOG("Catalog created successfully");
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Catalog created successfully\n");
#endif
    
    /* Initialize session state */
    ctx->archiveIndex = 1;
    ctx->foldersBackedUp = 0;
    ctx->failedBackups = 0;
    ctx->totalBytesArchived = 0;
    ctx->sessionActive = TRUE;
    
    DEBUG_LOG("Backup session initialized successfully");
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Session initialized successfully\n");
#endif
    return TRUE;
}

void CloseBackupSession(BackupContext *ctx) {
    if (!ctx || !ctx->sessionActive) {
        return;
    }
    
    DEBUG_LOG("Closing backup session...");
    DEBUG_LOG("  Folders backed up: %d", ctx->foldersBackedUp);
    DEBUG_LOG("  Failed backups: %d", ctx->failedBackups);
    DEBUG_LOG("  Total bytes: %lu", ctx->totalBytesArchived);
    
    /* Close catalog (writes footer with statistics) */
    if (ctx->catalogOpen) {
        CloseCatalog(ctx);
    }
    
    /* Mark session inactive */
    ctx->sessionActive = FALSE;
    ctx->catalogOpen = FALSE;
    
    DEBUG_LOG("Backup session closed");
}

BackupStatus BackupFolder(BackupContext *ctx, const char *folderPath, UWORD iconCount) {
    char archivePath[MAX_BACKUP_PATH];
    char subfolderPath[MAX_BACKUP_PATH];
    char markerPath[MAX_BACKUP_PATH];
    BackupArchiveEntry entry;
    BOOL isRoot;
    
    /* Validate parameters */
    if (!ctx || !folderPath) {
        DEBUG_LOG("Invalid parameters for BackupFolder");
        return BACKUP_INVALID_PARAMS;
    }
    
    if (!BACKUP_CONTEXT_VALID(ctx)) {
        DEBUG_LOG("Invalid backup context");
        return BACKUP_INVALID_PARAMS;
    }
    
    if (!ctx->sessionActive) {
        DEBUG_LOG("Backup session not active");
        return BACKUP_FAIL;
    }
    
    /* Validate archive index is within range */
    if (!ARCHIVE_INDEX_VALID(ctx->archiveIndex)) {
        DEBUG_LOG("Archive index out of range: %d", ctx->archiveIndex);
        return BACKUP_FAIL;
    }
    
    DEBUG_LOG("Backing up folder: %s (index: %d)", folderPath, ctx->archiveIndex);
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Backing up folder: %s (index: %u)\n", folderPath, ctx->archiveIndex);
#endif
    
    /* Check if folder has .info files */
    if (!FolderHasInfoFiles(folderPath)) {
        DEBUG_LOG("Folder has no .info files, skipping");
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] Folder has no .info files, skipping\n");
#endif
        return BACKUP_NO_ICONS;
    }
    
    /* If icon count not provided, count them now */
    if (iconCount == 0) {
        iconCount = CountInfoFiles(folderPath);
        DEBUG_LOG("Counted %hu .info files", iconCount);
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] Counted %hu .info files\n", iconCount);
#endif
    } else {
        DEBUG_LOG("Using provided icon count: %hu", iconCount);
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] Using provided icon count: %hu\n", iconCount);
#endif
    }
    
    /* Determine if this is a root folder */
    isRoot = IsRootFolder(folderPath);
    DEBUG_LOG("Root folder: %s", isRoot ? "YES" : "NO");
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Root folder: %s\n", isRoot ? "YES" : "NO");
#endif
    
    /* Calculate archive path */
    CalculateArchivePath(archivePath, ctx->runDirectory, ctx->archiveIndex);
    DEBUG_LOG("Archive path: %s", archivePath);
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Archive path: %s\n", archivePath);
#endif
    
    /* Calculate and ensure subfolder exists */
    CalculateSubfolderPath(subfolderPath, ctx->runDirectory, ctx->archiveIndex);
    DEBUG_LOG("Ensuring subfolder exists: %s", subfolderPath);
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Ensuring subfolder exists: %s\n", subfolderPath);
#endif
    
    /* Create subfolder directory if needed */
#ifdef PLATFORM_HOST
    #ifdef _WIN32
    _mkdir(subfolderPath);
    #else
    mkdir(subfolderPath, 0755);
    #endif
#else
    {
        BPTR lock = CreateDir((STRPTR)subfolderPath);
        if (lock) {
            UnLock(lock);
        }
    }
#endif
    
    /* Create LHA archive of folder contents */
    DEBUG_LOG("Creating archive...");
    if (!CreateLhaArchive(ctx->lhaPath, archivePath, folderPath)) {
        DEBUG_LOG("Failed to create archive");
        ctx->failedBackups++;
        
        /* Log failed backup to catalog */
        memset(&entry, 0, sizeof(entry));
        entry.archiveIndex = ctx->archiveIndex;
        FormatArchiveName(entry.archiveName, ctx->archiveIndex);
        
        /* Extract subfolder number from archive index */
        {
            UWORD folderNum = ARCHIVE_FOLDER_NUM(ctx->archiveIndex);
            snprintf(entry.subFolder, sizeof(entry.subFolder), "%03hu/", folderNum);
        }
        
        entry.sizeBytes = 0;
        entry.iconCount = iconCount;
        strncpy(entry.originalPath, folderPath, sizeof(entry.originalPath) - 1);
        entry.successful = FALSE;
        AppendCatalogEntry(ctx, &entry);
        
        ctx->archiveIndex++;
        return BACKUP_ARCHIVE_ERROR;
    }
    DEBUG_LOG("Archive created successfully");
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Archive created successfully\n");
#endif
    
    /* Create path marker */
    DEBUG_LOG("Creating path marker...");
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Creating path marker...\n");
#endif
    if (!CreateTempPathMarker(markerPath, folderPath, ctx->archiveIndex, NULL)) {
        DEBUG_LOG("Failed to create path marker");
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] WARNING: Failed to create path marker (non-fatal)\n");
#endif
        /* Non-fatal - archive exists, just missing marker */
    } else {
        DEBUG_LOG("Path marker created: %s", markerPath);
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] Path marker created: %s\n", markerPath);
#endif
        
        /* Add marker to archive */
        DEBUG_LOG("Adding marker to archive...");
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] Adding marker to archive...\n");
#endif
        if (!AddFileToArchive(ctx->lhaPath, archivePath, markerPath)) {
            DEBUG_LOG("Failed to add marker to archive");
#ifndef PLATFORM_HOST
            append_to_log("[BACKUP] WARNING: Failed to add marker to archive (non-fatal)\n");
#endif
            /* Non-fatal - can still restore using catalog */
        } else {
            DEBUG_LOG("Marker added successfully");
#ifndef PLATFORM_HOST
            append_to_log("[BACKUP] Marker added successfully\n");
#endif
        }
        
        /* Clean up temporary marker file */
        DeleteMarkerFile(markerPath);
    }
    
    /* Create catalog entry */
    memset(&entry, 0, sizeof(entry));
    entry.archiveIndex = ctx->archiveIndex;
    FormatArchiveName(entry.archiveName, ctx->archiveIndex);
    
    /* Extract subfolder number from archive index */
    {
        UWORD folderNum = ARCHIVE_FOLDER_NUM(ctx->archiveIndex);
        snprintf(entry.subFolder, sizeof(entry.subFolder), "%03hu/", folderNum);
    }
    
    entry.sizeBytes = GetArchiveSize(archivePath);
    entry.iconCount = iconCount;
    strncpy(entry.originalPath, folderPath, sizeof(entry.originalPath) - 1);
    entry.originalPath[sizeof(entry.originalPath) - 1] = '\0';
    entry.successful = TRUE;
    
    /* Append to catalog */
    if (!AppendCatalogEntry(ctx, &entry)) {
        DEBUG_LOG("Failed to append catalog entry");
        /* Non-fatal - archive exists and can be recovered */
    }
    
    /* Update statistics */
    ctx->archiveIndex++;
    ctx->foldersBackedUp++;
    ctx->totalBytesArchived += entry.sizeBytes;
    
    DEBUG_LOG("Folder backed up successfully");
    return BACKUP_OK;
}

/*========================================================================*/
/* Utility Functions                                                      */
/*========================================================================*/

BOOL FolderHasInfoFiles(const char *folderPath) {
    if (!folderPath) {
        return FALSE;
    }
    
#ifdef PLATFORM_HOST
    /* Host implementation using opendir/readdir */
    DIR *dir;
    struct dirent *entry;
    BOOL found = FALSE;
    
    dir = opendir(folderPath);
    if (!dir) {
        DEBUG_LOG("Failed to open directory: %s", folderPath);
        return FALSE;
    }
    
    /* Scan for .info files */
    while ((entry = readdir(dir)) != NULL) {
        size_t len = strlen(entry->d_name);
        if (len > 5 && strcmp(entry->d_name + len - 5, ".info") == 0) {
            found = TRUE;
            break;
        }
    }
    
    closedir(dir);
    return found;
    
#else
    /* Amiga implementation using pattern matching */
    struct AnchorPath *anchor;
    BOOL found = FALSE;
    LONG result;
    char pattern[512];
    
    /* Build pattern: "folderPath/#?.info" */
    snprintf(pattern, sizeof(pattern), "%s#?.info", folderPath);
    
    /* Allocate anchor structure with larger buffer to prevent overflow
     * MatchFirst can write full paths back into ap_Buf, so we need room for:
     * - The full folder path
     * - The filename (up to 107 chars on FFS)
     * - Pattern characters
     * Allocating 512 bytes provides safe margin */
    anchor = (struct AnchorPath *)AllocVec(sizeof(struct AnchorPath) + 512, MEMF_CLEAR);
    if (!anchor) {
        return FALSE;
    }
    
    anchor->ap_BreakBits = 0;
    anchor->ap_Strlen = 512;
    
    /* Match first .info file */
    result = MatchFirst((STRPTR)pattern, anchor);
    if (result == 0) {
        found = TRUE;
        MatchEnd(anchor);
    }
    
    FreeVec(anchor);
    return found;
#endif
}

UWORD CountInfoFiles(const char *folderPath) {
    UWORD count = 0;
    
    if (!folderPath) {
        return 0;
    }
    
#ifdef PLATFORM_HOST
    /* Host implementation using opendir/readdir */
    DIR *dir;
    struct dirent *entry;
    
    dir = opendir(folderPath);
    if (!dir) {
        DEBUG_LOG("Failed to open directory: %s", folderPath);
        return 0;
    }
    
    /* Count .info files */
    while ((entry = readdir(dir)) != NULL) {
        size_t len = strlen(entry->d_name);
        if (len > 5 && strcmp(entry->d_name + len - 5, ".info") == 0) {
            count++;
        }
    }
    
    closedir(dir);
    return count;
    
#else
    /* Amiga implementation using pattern matching */
    struct AnchorPath *anchor;
    LONG result;
    char pattern[512];
    
    /* Build pattern: "folderPath/#?.info" */
    snprintf(pattern, sizeof(pattern), "%s#?.info", folderPath);
    
    /* Allocate anchor structure with larger buffer */
    anchor = (struct AnchorPath *)AllocVec(sizeof(struct AnchorPath) + 512, MEMF_CLEAR);
    if (!anchor) {
        return 0;
    }
    
    anchor->ap_BreakBits = 0;
    anchor->ap_Strlen = 512;
    
    /* Count all matching .info files */
    result = MatchFirst((STRPTR)pattern, anchor);
    while (result == 0) {
        count++;
        result = MatchNext(anchor);
    }
    
    MatchEnd(anchor);
    FreeVec(anchor);
    
    return count;
#endif
}

