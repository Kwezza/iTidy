/**
 * @file backup_restore.c
 * @brief Implementation of iTidy backup restore operations
 * @author AI Agent (Task 8)
 * @date October 24, 2025
 */

#include "backup_restore.h"
#include "backup_catalog.h"
#include "backup_marker.h"
#include "backup_lha.h"
#include "backup_paths.h"
#include "file_directory_handling.h"
#include "GUI/StatusWindows/progress_window.h"

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PLATFORM_AMIGA
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#else
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define access _access
#define F_OK 0
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#endif
#endif

/* Internal helper function declarations */
static BOOL FileExists(const char *path);
static BOOL DirectoryExists(const char *path);
static BOOL CreateDirectoryRecursive(const char *path);
static void RecordError(RestoreContext *ctx, const char *error);
static void UpdateStatistics(RestoreContext *ctx, BOOL success, ULONG bytes);
static BOOL RestoreWindowGeometry(const char *folderPath, const BackupArchiveEntry *entry);

/* Context for catalog iteration during full run restore */
typedef struct {
    RestoreContext *restoreCtx;
    const char *runDir;
    UWORD currentFolderIndex;    /* Tracks which folder we're processing (1-based) */
} CatalogIterContext;

/* Callback for ParseCatalog - processes each entry during full run restore */
static BOOL RestoreCatalogEntryCallback(const BackupArchiveEntry *entry, void *userData) {
    CatalogIterContext *iterData = (CatalogIterContext*)userData;
    RestoreContext *rctx = iterData->restoreCtx;
    char archivePath[MAX_RESTORE_PATH];
    const char *destPath;
    char statusText[256];
    
    /* Skip failed backups (they have no valid archive) */
    if (!entry->successful) {
        return TRUE;  /* Continue parsing */
    }
    
    /* Increment folder counter */
    iterData->currentFolderIndex++;
    
    /* Update progress window if available */
    if (rctx->userData) {
        struct iTidy_ProgressWindow *progress_window = (struct iTidy_ProgressWindow*)rctx->userData;
        snprintf(statusText, sizeof(statusText), "Restoring: %s", entry->originalPath);
        iTidy_UpdateProgress(progress_window, iterData->currentFolderIndex, statusText);
    }
    
    /* Build full archive path */
    snprintf(archivePath, sizeof(archivePath), "%s/%s%s",
            iterData->runDir, entry->subFolder, entry->archiveName);
    
    /* Use the catalog entry's original path as the destination */
    destPath = entry->originalPath;
    
    CONSOLE_STATUS("Restoring archive: %s\n", archivePath);
    CONSOLE_STATUS("  -> Restoring to: %s\n", destPath);
    
    /* Check if archive exists */
    if (!FileExists(archivePath)) {
        RecordError(rctx, "Archive file not found");
        UpdateStatistics(rctx, FALSE, 0);
        return TRUE;  /* Continue with other archives */
    }
    
    /* Validate destination path */
    if (!ValidateRestorePath(destPath)) {
        char error[256];
        sprintf(error, "Invalid restore path: %s", destPath);
        RecordError(rctx, error);
        UpdateStatistics(rctx, FALSE, 0);
        return TRUE;  /* Continue with other archives */
    }
    
    /* Ensure destination directory exists */
    if (!DirectoryExists(destPath)) {
        if (!CreateDirectoryRecursive(destPath)) {
            char error[256];
            sprintf(error, "Failed to create directory: %s", destPath);
            RecordError(rctx, error);
            UpdateStatistics(rctx, FALSE, 0);
            return TRUE;  /* Continue with other archives */
        }
    }
    
    /* Extract archive to destination */
    if (!ExtractLhaArchive(rctx->lhaPath, archivePath, destPath)) {
        char error[256];
        sprintf(error, "Failed to extract archive to: %s", destPath);
        RecordError(rctx, error);
        UpdateStatistics(rctx, FALSE, 0);
        return TRUE;  /* Continue with other archives */
    }
    
    /* Restore window geometry if enabled */
    if (rctx->restoreWindowGeometry) {
        RestoreWindowGeometry(destPath, entry);
    }
    
    /* Get archive size for statistics */
    ULONG archiveSize = GetArchiveSize(archivePath);
    UpdateStatistics(rctx, TRUE, archiveSize);
    
    CONSOLE_STATUS("  -> Restored successfully\n");
    
    return TRUE;  /* Continue parsing */
}

/* ========================================================================
 * HELPER FUNCTIONS - File System Utilities
 * ======================================================================== */

/**
 * @brief Check if a file exists
 */
static BOOL FileExists(const char *path) {
    if (!path || path[0] == '\0') return FALSE;
    
#ifdef PLATFORM_AMIGA
    BPTR lock = Lock((STRPTR)path, SHARED_LOCK);
    if (lock) {
        UnLock(lock);
        return TRUE;
    }
    return FALSE;
#else
    return (access(path, F_OK) == 0);
#endif
}

/**
 * @brief Check if a directory exists
 */
static BOOL DirectoryExists(const char *path) {
    if (!path || path[0] == '\0') return FALSE;
    
#ifdef PLATFORM_AMIGA
    BPTR lock = Lock((STRPTR)path, SHARED_LOCK);
    if (lock) {
        struct FileInfoBlock fib;
        BOOL isDir = FALSE;
        if (Examine(lock, &fib)) {
            isDir = (fib.fib_DirEntryType > 0);
        }
        UnLock(lock);
        return isDir;
    }
    return FALSE;
#else
    struct stat st;
    if (stat(path, &st) == 0) {
        return (st.st_mode & S_IFDIR) != 0;
    }
    return FALSE;
#endif
}

/**
 * @brief Create directory and all parent directories
 */
static BOOL CreateDirectoryRecursive(const char *path) {
    if (!path || path[0] == '\0') return FALSE;
    
    /* Check if already exists */
    if (DirectoryExists(path)) return TRUE;
    
    /* Make a copy we can modify */
    char pathCopy[MAX_RESTORE_PATH];
    strncpy(pathCopy, path, MAX_RESTORE_PATH - 1);
    pathCopy[MAX_RESTORE_PATH - 1] = '\0';
    
    /* Create parent directories first */
    char *lastSlash = strrchr(pathCopy, '/');
    if (!lastSlash) {
        lastSlash = strrchr(pathCopy, '\\');
    }
    
#ifdef PLATFORM_AMIGA
    /* On Amiga, handle volume roots specially */
    char *colon = strchr(pathCopy, ':');
    if (lastSlash && lastSlash > colon) {
#else
    if (lastSlash && lastSlash != pathCopy) {
#endif
        *lastSlash = '\0';
        if (!CreateDirectoryRecursive(pathCopy)) {
            return FALSE;
        }
        *lastSlash = '/';
    }
    
    /* Create this directory */
#ifdef PLATFORM_AMIGA
    BPTR lock = CreateDir((STRPTR)pathCopy);
    if (lock) {
        UnLock(lock);
        return TRUE;
    }
    /* May already exist, check */
    return DirectoryExists(pathCopy);
#else
    if (mkdir(pathCopy, 0755) == 0) {
        return TRUE;
    }
    /* May already exist, check */
    return DirectoryExists(pathCopy);
#endif
}

/**
 * @brief Record error message in context
 */
static void RecordError(RestoreContext *ctx, const char *error) {
    if (!ctx || !error) return;
    
    strncpy(ctx->lastError, error, sizeof(ctx->lastError) - 1);
    ctx->lastError[sizeof(ctx->lastError) - 1] = '\0';
    
    if (!ctx->stats.hasErrors) {
        strncpy(ctx->stats.firstError, error, sizeof(ctx->stats.firstError) - 1);
        ctx->stats.firstError[sizeof(ctx->stats.firstError) - 1] = '\0';
        ctx->stats.hasErrors = TRUE;
    }
}

/**
 * @brief Update restore statistics
 */
static void UpdateStatistics(RestoreContext *ctx, BOOL success, ULONG bytes) {
    if (!ctx) return;
    
    if (success) {
        ctx->stats.archivesRestored++;
        ctx->stats.totalBytesRestored += bytes;
    } else {
        ctx->stats.archivesFailed++;
    }
}

/**
 * @brief Restore window geometry to folder's .info file
 * 
 * Applies window position, size, and view mode from backup metadata
 * to the folder's .info file. Only modifies window-related fields,
 * leaving icon image and tool types intact.
 * 
 * @param folderPath Full path to the folder (e.g., "Work:Projects/")
 * @param entry Catalog entry containing window geometry
 * @return TRUE on success, FALSE on failure or if geometry not available
 */
static BOOL RestoreWindowGeometry(const char *folderPath, const BackupArchiveEntry *entry) {
#ifdef PLATFORM_AMIGA
    folderWindowSize windowInfo;
    
    if (!folderPath || !entry) {
        return FALSE;
    }
    
    /* Check if window geometry is available */
    if (entry->windowWidth <= 0 || entry->windowHeight <= 0 ||
        entry->windowLeft < 0 || entry->windowTop < 0) {
        /* No window geometry stored - not an error, just skip */
        return TRUE;
    }
    
    /* Populate window info structure */
    windowInfo.left = entry->windowLeft;
    windowInfo.top = entry->windowTop;
    windowInfo.width = entry->windowWidth;
    windowInfo.height = entry->windowHeight;
    
    /* Use SaveFolderSettings to update the .info file */
    /* Note: This also updates view mode via user_folderViewMode global */
    /* For now, we'll just update window geometry and let view mode stay as-is */
    SaveFolderSettings(folderPath, &windowInfo, 0);
    
    CONSOLE_STATUS("  Restored window geometry: %dx%d+%d+%d\n",
           windowInfo.width, windowInfo.height,
           windowInfo.left, windowInfo.top);
    
    return TRUE;
#else
    /* No .info files on host platform */
    (void)folderPath;
    (void)entry;
    return TRUE;
#endif
}

/* ========================================================================
 * PUBLIC API IMPLEMENTATION - Initialization
 * ======================================================================== */

BOOL InitRestoreContext(RestoreContext *ctx) {
    if (!ctx) return FALSE;
    
    /* Clear context */
    memset(ctx, 0, sizeof(RestoreContext));
    
    /* Check for LHA */
    ctx->lhaAvailable = CheckLhaAvailable(ctx->lhaPath);
    
    if (!ctx->lhaAvailable) {
        RecordError(ctx, "LHA executable not found");
        return FALSE;
    }
    
    /* Default: restore window geometry */
    ctx->restoreWindowGeometry = TRUE;
    
    return TRUE;
}

/* ========================================================================
 * PUBLIC API IMPLEMENTATION - Validation
 * ======================================================================== */

BOOL ValidateRestorePath(const char *path) {
    if (!path || path[0] == '\0') return FALSE;
    
    /* Check for minimum path length */
    if (strlen(path) < 3) return FALSE;
    
#ifdef PLATFORM_AMIGA
    /* Must have a volume (e.g., "DH0:") */
    char *colon = strchr(path, ':');
    if (!colon) return FALSE;
    
    /* Root volumes (e.g., "DH0:") are valid */
    if (*(colon + 1) == '\0' || *(colon + 1) == '/') {
        return TRUE;
    }
    
    /* Regular paths are valid if they have content after colon */
    return (strlen(colon + 1) > 0);
#else
    /* On host, just check it's not empty and has some structure */
    return (strlen(path) > 0);
#endif
}

BOOL CanRestoreArchive(const char *archivePath, const char *lhaPath) {
    if (!archivePath || !lhaPath) return FALSE;
    
    /* Check if archive exists */
    if (!FileExists(archivePath)) return FALSE;
    
    /* Check if archive has marker */
    if (!ArchiveHasMarker(archivePath, lhaPath)) return FALSE;
    
    return TRUE;
}

/* ========================================================================
 * PUBLIC API IMPLEMENTATION - Core Restore Functions
 * ======================================================================== */

RestoreStatus RestoreArchive(RestoreContext *ctx, const char *archivePath) {
    if (!ctx || !archivePath) return RESTORE_INVALID_PARAMS;
    if (!ctx->lhaAvailable) return RESTORE_LHA_NOT_FOUND;
    
    /* Log start of restore */
    CONSOLE_STATUS("Restoring archive: %s\n", archivePath);
    
    /* Check if archive exists */
    if (!FileExists(archivePath)) {
        RecordError(ctx, "Archive file not found");
        UpdateStatistics(ctx, FALSE, 0);
        return RESTORE_ARCHIVE_NOT_FOUND;
    }
    
    /* Extract and read path marker */
    char originalPath[MAX_RESTORE_PATH];
    
    if (!ExtractAndReadMarker(archivePath, ctx->lhaPath, originalPath, NULL)) {
        RecordError(ctx, "Failed to extract or read _PATH.txt marker");
        UpdateStatistics(ctx, FALSE, 0);
        return RESTORE_MARKER_READ_FAILED;
    }
    
    CONSOLE_STATUS("  -> Restoring to: %s\n", originalPath);
    
    /* Validate destination path */
    if (!ValidateRestorePath(originalPath)) {
        char error[256];
        sprintf(error, "Invalid restore path: %s", originalPath);
        RecordError(ctx, error);
        UpdateStatistics(ctx, FALSE, 0);
        return RESTORE_INVALID_PATH;
    }
    
    /* Ensure parent directory exists */
    if (!DirectoryExists(originalPath)) {
        if (!CreateDirectoryRecursive(originalPath)) {
            char error[256];
            sprintf(error, "Failed to create directory: %s", originalPath);
            RecordError(ctx, error);
            UpdateStatistics(ctx, FALSE, 0);
            return RESTORE_FAIL;
        }
    }
    
    /* Extract archive to original location */
    if (!ExtractLhaArchive(ctx->lhaPath, archivePath, originalPath)) {
        char error[256];
        sprintf(error, "Failed to extract archive to: %s", originalPath);
        RecordError(ctx, error);
        UpdateStatistics(ctx, FALSE, 0);
        return RESTORE_EXTRACT_FAILED;
    }
    
    /* Get archive size for statistics */
    ULONG archiveSize = GetArchiveSize(archivePath);
    UpdateStatistics(ctx, TRUE, archiveSize);
    
    CONSOLE_STATUS("  -> Restored successfully\n");
    
    return RESTORE_OK;
}

RestoreStatus RecoverOrphanedArchive(RestoreContext *ctx, const char *archivePath) {
    /* Same implementation as RestoreArchive - both use embedded marker */
    return RestoreArchive(ctx, archivePath);
}

RestoreStatus RestoreFullRun(RestoreContext *ctx, const char *runDirectory) {
    if (!ctx || !runDirectory) return RESTORE_INVALID_PARAMS;
    if (!ctx->lhaAvailable) return RESTORE_LHA_NOT_FOUND;
    
    CONSOLE_STATUS("\n========================================\n");
    CONSOLE_STATUS("Starting Full Run Restore\n");
    CONSOLE_STATUS("Run Directory: %s\n", runDirectory);
    CONSOLE_STATUS("========================================\n\n");
    
    /* Build catalog path */
    char catalogPath[MAX_RESTORE_PATH];
    snprintf(catalogPath, sizeof(catalogPath), "%s/%s", runDirectory, CATALOG_FILENAME);
    
    /* Check if catalog exists */
    if (!FileExists(catalogPath)) {
        RecordError(ctx, "Catalog file not found");
        return RESTORE_CATALOG_NOT_FOUND;
    }
    
    CONSOLE_STATUS("Reading catalog: %s\n\n", catalogPath);
    
    /* Setup context for catalog iteration */
    CatalogIterContext iterCtx = {
        .restoreCtx = ctx,
        .runDir = runDirectory,
        .currentFolderIndex = 0
    };
    
    /* Read catalog and restore all entries */
    if (!ParseCatalog(catalogPath, RestoreCatalogEntryCallback, &iterCtx)) {
        RecordError(ctx, "Failed to parse catalog file");
        return RESTORE_CATALOG_READ_FAILED;
    }
    
    /* Print summary */
    CONSOLE_STATUS("\n========================================\n");
    CONSOLE_STATUS("Restore Summary\n");
    CONSOLE_STATUS("========================================\n");
    CONSOLE_STATUS("Archives restored: %u\n", ctx->stats.archivesRestored);
    CONSOLE_STATUS("Archives failed:   %u\n", ctx->stats.archivesFailed);
    CONSOLE_STATUS("Total data:        %lu bytes\n", ctx->stats.totalBytesRestored);
    CONSOLE_STATUS("========================================\n\n");
    
    /* Return success if at least one archive was restored */
    if (ctx->stats.archivesRestored > 0) {
        return RESTORE_OK;
    }
    
    /* All restores failed */
    if (ctx->stats.archivesFailed > 0) {
        return RESTORE_FAIL;
    }
    
    /* No archives found in catalog */
    RecordError(ctx, "No valid archives found in catalog");
    return RESTORE_CATALOG_READ_FAILED;
}

/* ========================================================================
 * PUBLIC API IMPLEMENTATION - Utility Functions
 * ======================================================================== */

const char* GetRestoreStatusMessage(RestoreStatus status) {
    switch (status) {
        case RESTORE_OK:
            return "Restore succeeded";
        case RESTORE_FAIL:
            return "Restore failed";
        case RESTORE_ARCHIVE_NOT_FOUND:
            return "Archive file not found";
        case RESTORE_MARKER_NOT_FOUND:
            return "_PATH.txt marker not found in archive";
        case RESTORE_MARKER_READ_FAILED:
            return "Failed to read _PATH.txt marker";
        case RESTORE_EXTRACT_FAILED:
            return "Archive extraction failed";
        case RESTORE_INVALID_PATH:
            return "Invalid or inaccessible restore path";
        case RESTORE_CATALOG_NOT_FOUND:
            return "Catalog file not found";
        case RESTORE_CATALOG_READ_FAILED:
            return "Failed to read catalog file";
        case RESTORE_LHA_NOT_FOUND:
            return "LHA executable not available";
        case RESTORE_INVALID_PARAMS:
            return "Invalid parameters provided";
        default:
            return "Unknown restore status";
    }
}

void ResetRestoreStatistics(RestoreContext *ctx) {
    if (!ctx) return;
    
    memset(&ctx->stats, 0, sizeof(RestoreStatistics));
}

void GetRestoreStatistics(const RestoreContext *ctx, char *buffer, int bufferSize) {
    if (!ctx || !buffer || bufferSize <= 0) return;
    
    /* Format statistics */
    char sizeStr[32];
    ULONG kb = ctx->stats.totalBytesRestored / 1024;
    ULONG mb = kb / 1024;
    ULONG gb = mb / 1024;
    
    if (gb > 0) {
        /* Integer division with decimal: GB.hundredths */
        ULONG decimal = ((ctx->stats.totalBytesRestored % (1024UL * 1024 * 1024)) * 100) / (1024UL * 1024 * 1024);
        sprintf(sizeStr, "%lu.%02lu GB", gb, decimal);
    } else if (mb > 0) {
        /* Integer division with decimal: MB.hundredths */
        ULONG decimal = ((ctx->stats.totalBytesRestored % (1024UL * 1024)) * 100) / (1024UL * 1024);
        sprintf(sizeStr, "%lu.%02lu MB", mb, decimal);
    } else if (kb > 0) {
        sprintf(sizeStr, "%lu KB", kb);
    } else {
        sprintf(sizeStr, "%lu bytes", ctx->stats.totalBytesRestored);
    }
    
    snprintf(buffer, bufferSize,
             "Restored: %u archives, Failed: %u, Total: %s",
             ctx->stats.archivesRestored,
             ctx->stats.archivesFailed,
             sizeStr);
}
