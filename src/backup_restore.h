/**
 * @file backup_restore.h
 * @brief Restore operations for iTidy backup system
 * @author AI Agent (Task 8)
 * @date October 24, 2025
 * 
 * This module provides restore and recovery functionality for iTidy backups:
 * - Restore single archives to their original locations
 * - Restore entire backup runs (undo complete tidy session)
 * - Recover orphaned archives (when catalog.txt is missing)
 * 
 * Dependencies: backup_types.h, backup_catalog.h, backup_marker.h, backup_lha.h
 */

#ifndef BACKUP_RESTORE_H
#define BACKUP_RESTORE_H

#include "backup_types.h"

#ifdef PLATFORM_AMIGA
#include <exec/types.h>
#else
#include <stdint.h>
#include <stdbool.h>
typedef int BOOL;
typedef unsigned short UWORD;
typedef unsigned long ULONG;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#endif

/* Constants */
#define MAX_RESTORE_PATH 256
#define MAX_RESTORE_ERRORS 100

/**
 * @brief Restore status codes
 */
typedef enum {
    RESTORE_OK = 0,              /* Restore succeeded */
    RESTORE_FAIL,                /* Generic failure */
    RESTORE_ARCHIVE_NOT_FOUND,   /* Archive file doesn't exist */
    RESTORE_MARKER_NOT_FOUND,    /* _PATH.txt not in archive */
    RESTORE_MARKER_READ_FAILED,  /* Could not read _PATH.txt */
    RESTORE_EXTRACT_FAILED,      /* LHA extraction failed */
    RESTORE_INVALID_PATH,        /* Original path is invalid */
    RESTORE_CATALOG_NOT_FOUND,   /* catalog.txt not found */
    RESTORE_CATALOG_READ_FAILED, /* Could not parse catalog.txt */
    RESTORE_LHA_NOT_FOUND,       /* LHA executable not available */
    RESTORE_INVALID_PARAMS       /* NULL or invalid parameters */
} RestoreStatus;

/**
 * @brief Statistics for restore operations
 */
typedef struct {
    UWORD archivesRestored;      /* Number of archives successfully restored */
    UWORD archivesFailed;        /* Number of archives that failed to restore */
    ULONG totalBytesRestored;    /* Total bytes restored */
    char firstError[256];        /* First error message encountered */
    BOOL hasErrors;              /* TRUE if any errors occurred */
} RestoreStatistics;

/**
 * @brief Context for restore operations
 */
typedef struct {
    char lhaPath[MAX_RESTORE_PATH];    /* Path to LHA executable */
    BOOL lhaAvailable;                  /* TRUE if LHA was found */
    BOOL restoreWindowGeometry;         /* TRUE to restore window positions/sizes (default TRUE) */
    RestoreStatistics stats;            /* Operation statistics */
    char lastError[256];                /* Last error message */
} RestoreContext;

/* ========================================================================
 * PUBLIC API - Core Restore Functions
 * ======================================================================== */

/**
 * @brief Initialize a restore context
 * 
 * Checks for LHA availability and prepares restore context.
 * 
 * @param ctx Restore context to initialize (output)
 * @return TRUE if initialized successfully, FALSE if LHA not found
 * 
 * @example
 *   RestoreContext ctx;
 *   if (InitRestoreContext(&ctx)) {
 *       // Ready to restore
 *   }
 */
BOOL InitRestoreContext(RestoreContext *ctx);

/**
 * @brief Restore a single archive to its original location
 * 
 * Algorithm:
 * 1. Check if archive exists
 * 2. Extract _PATH.txt marker from archive
 * 3. Read original path from marker
 * 4. Validate destination path
 * 5. Extract archive to original location
 * 6. Update statistics
 * 
 * @param ctx Restore context
 * @param archivePath Full path to .lha archive (e.g., "Run_0001/000/00001.lha")
 * @return RestoreStatus code
 * 
 * @example
 *   RestoreContext ctx;
 *   InitRestoreContext(&ctx);
 *   RestoreStatus status = RestoreArchive(&ctx, "PROGDIR:Backups/Run_0001/000/00001.lha");
 *   if (status == RESTORE_OK) {
 *       printf("Restored to: %s\n", originalPath);
 *   }
 */
RestoreStatus RestoreArchive(RestoreContext *ctx, const char *archivePath);

/**
 * @brief Restore entire backup run using catalog
 * 
 * Algorithm:
 * 1. Locate and read catalog.txt in run directory
 * 2. Parse all catalog entries
 * 3. For each entry, restore archive to original path
 * 4. Track success/failure statistics
 * 5. Continue even if individual archives fail
 * 
 * @param ctx Restore context
 * @param runDirectory Path to run directory (e.g., "PROGDIR:Backups/Run_0001")
 * @return RestoreStatus code (RESTORE_OK if at least one archive restored)
 * 
 * @example
 *   RestoreContext ctx;
 *   InitRestoreContext(&ctx);
 *   RestoreStatus status = RestoreFullRun(&ctx, "PROGDIR:Backups/Run_0001");
 *   printf("Restored %d archives, %d failed\n", 
 *          ctx.stats.archivesRestored, ctx.stats.archivesFailed);
 */
RestoreStatus RestoreFullRun(RestoreContext *ctx, const char *runDirectory);

/**
 * @brief Restore archive without catalog (orphaned recovery)
 * 
 * Use when catalog.txt is missing or corrupted. Extracts _PATH.txt
 * from archive and uses embedded path information.
 * 
 * Algorithm:
 * 1. Extract _PATH.txt to temp directory
 * 2. Read original path from marker
 * 3. Validate destination
 * 4. Extract archive to original location
 * 5. Clean up temp marker
 * 
 * @param ctx Restore context
 * @param archivePath Full path to .lha archive
 * @return RestoreStatus code
 * 
 * @example
 *   RestoreContext ctx;
 *   InitRestoreContext(&ctx);
 *   RestoreStatus status = RecoverOrphanedArchive(&ctx, "orphan.lha");
 *   if (status == RESTORE_OK) {
 *       printf("Recovered orphaned archive\n");
 *   }
 */
RestoreStatus RecoverOrphanedArchive(RestoreContext *ctx, const char *archivePath);

/* ========================================================================
 * PUBLIC API - Utility Functions
 * ======================================================================== */

/**
 * @brief Get human-readable error message for restore status
 * 
 * @param status RestoreStatus code
 * @return Error message string (static, do not free)
 */
const char* GetRestoreStatusMessage(RestoreStatus status);

/**
 * @brief Reset restore statistics
 * 
 * @param ctx Restore context
 */
void ResetRestoreStatistics(RestoreContext *ctx);

/**
 * @brief Get formatted statistics string
 * 
 * @param ctx Restore context
 * @param buffer Output buffer for formatted string
 * @param bufferSize Size of output buffer
 * 
 * @example
 *   char stats[256];
 *   GetRestoreStatistics(&ctx, stats, sizeof(stats));
 *   printf("%s\n", stats);
 *   // Output: "Restored: 42 archives, Failed: 3, Total: 1.2 MB"
 */
void GetRestoreStatistics(const RestoreContext *ctx, char *buffer, int bufferSize);

/**
 * @brief Check if an archive can be restored
 * 
 * Verifies:
 * - Archive file exists
 * - Archive contains _PATH.txt marker
 * - Marker is readable
 * 
 * @param archivePath Path to archive
 * @param lhaPath Path to LHA executable
 * @return TRUE if archive appears restorable
 */
BOOL CanRestoreArchive(const char *archivePath, const char *lhaPath);

/**
 * @brief Validate destination path before restore
 * 
 * Checks:
 * - Path format is valid
 * - Parent directory exists (or can be created)
 * - Path is not root-only (e.g., "DH0:")
 * 
 * @param path Path to validate
 * @return TRUE if path is valid for restore
 */
BOOL ValidateRestorePath(const char *path);

#endif /* BACKUP_RESTORE_H */
