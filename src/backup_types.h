/**
 * backup_types.h - iTidy Backup System Core Data Types
 * 
 * Defines shared data structures for the hierarchical backup and restore system.
 * These types support sequential archive numbering, run management, and catalog tracking.
 * 
 * Design: BACKUP_SYSTEM_PROPOSAL.md
 * Implementation: BACKUP_IMPLEMENTATION_GUIDE.md - Phase 1, Task 1
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

#ifndef BACKUP_TYPES_H
#define BACKUP_TYPES_H

/* Platform-specific includes */
#include <exec/types.h>
#include <dos/dos.h>

/*========================================================================*/
/* Constants                                                             */
/*========================================================================*/

/* Archive numbering limits */
#define MAX_ARCHIVES_PER_RUN    99999   /* 5-digit archive index (00001-99999) */
#define MAX_ARCHIVES_PER_FOLDER 100     /* Max files per hierarchical folder */
#define MAX_RUN_NUMBER          9999    /* 4-digit run number (0001-9999) */

/* Path and filename limits (AmigaDOS FFS-safe) */
#define MAX_BACKUP_PATH         256     /* Maximum full path length */
#define MAX_ARCHIVE_NAME        32      /* "00001.lha" = 9 chars + safety margin */
#define MAX_FOLDER_NAME         8       /* "123/" = 4 chars + safety margin */
#define MAX_RUN_DIR_NAME        16      /* "Run_0001" = 9 chars + safety margin */

/* Catalog and path marker filenames */
#define CATALOG_FILENAME        "catalog.txt"
#define PATH_MARKER_FILENAME    "_PATH.txt"
#define BACKUP_LOG_FILENAME     "iTidyBackup.log"

/* Backup directory structure */
#define BACKUP_RUN_PREFIX       "Run_"
#define BACKUP_DEFAULT_ROOT     "PROGDIR:Backups"

/* LhA command configuration */
#define LHA_COMPRESSION_LEVEL   "-m1"   /* Medium compression (balance speed/size) */
#define LHA_QUIET_FLAG          "-q"    /* Quiet mode (minimal output) */
#define LHA_RECURSIVE_FLAG      "-r"    /* Recursive (include subdirectories) */

/*========================================================================*/
/* Backup Status Enumeration                                             */
/*========================================================================*/

/**
 * @brief Status codes for backup operations
 * 
 * These codes indicate the result of backup/restore operations
 * and guide error handling and user feedback.
 */
typedef enum {
    BACKUP_OK = 0,              /* Operation completed successfully */
    BACKUP_FAIL,                /* General failure (see logs for details) */
    BACKUP_DISKFULL,            /* Insufficient disk space */
    BACKUP_LHA_MISSING,         /* LhA archiver not found */
    BACKUP_PATH_TOO_LONG,       /* Path exceeds FFS limits */
    BACKUP_INVALID_PARAMS,      /* NULL pointer or invalid parameter */
    BACKUP_CATALOG_ERROR,       /* Catalog file I/O error */
    BACKUP_ARCHIVE_ERROR,       /* LhA archive creation/extraction failed */
    BACKUP_NO_ICONS             /* No .info files found to backup */
} BackupStatus;

/*========================================================================*/
/* Backup Context Structure                                              */
/*========================================================================*/

/**
 * @brief Context for a single backup session (one run)
 * 
 * Maintains state for the current backup run, including paths,
 * archive indexing, catalog file handle, and statistics.
 * 
 * A "run" is one complete backup session, potentially backing up
 * multiple folders. All archives created in one run share the same
 * Run_NNNN directory and catalog.txt file.
 * 
 * Example usage:
 *   BackupContext ctx;
 *   InitBackupSession(&ctx, &prefs);
 *   BackupFolder(&ctx, "DH0:Projects/");
 *   BackupFolder(&ctx, "DH0:Documents/");
 *   CloseBackupSession(&ctx);
 */
typedef struct {
    /* Run identification */
    UWORD runNumber;                    /* Sequential run number (0001-9999) */
    ULONG archiveIndex;                 /* Next archive number (00001-99999) */
    
    /* Paths */
    char runDirectory[MAX_BACKUP_PATH]; /* Full path to Run_NNNN/ directory */
    char backupRoot[MAX_BACKUP_PATH];   /* Root backup directory (e.g., "PROGDIR:Backups") */
    char sourceDirectory[MAX_BACKUP_PATH]; /* Source directory being tidied */
    char lhaPath[32];                   /* Path to LhA executable */
    
    /* Catalog file handle */
    BPTR catalogFile;                   /* AmigaDOS file handle */
    
    /* Statistics */
    ULONG startTime;                    /* Session start time (seconds since epoch) */
    ULONG endTime;                      /* Session end time (or 0 if in progress) */
    UWORD foldersBackedUp;              /* Count of successfully backed-up folders */
    UWORD failedBackups;                /* Count of failed backup attempts */
    ULONG totalBytesArchived;           /* Total size of all archives created */
    
    /* Flags */
    BOOL lhaAvailable;                  /* TRUE if LhA was found and is usable */
    BOOL catalogOpen;                   /* TRUE if catalog file is open for writing */
    BOOL sessionActive;                 /* TRUE if session initialized and active */
    
} BackupContext;

/*========================================================================*/
/* Backup Archive Entry Structure                                        */
/*========================================================================*/

/**
 * @brief Metadata for a single backup archive
 * 
 * Represents one entry in the catalog.txt file. Each entry maps
 * an archive index to its original folder path and metadata.
 * 
 * Catalog format (pipe-delimited):
 *   00001.lha | 000/ | 15 KB | DH0:Projects/ClientWork/WebDesign/ | 320x200+100+50 | 1
 *   00042.lha | 000/ | 22 KB | Work:Development/SourceCode/ | 640x400+50+30 | 0
 *   12345.lha | 123/ | 5 KB  | Work:WHDLoad/Games/Z/Zool/ | 320x200+0+11 | 1
 * 
 * Window geometry format: WIDTHxHEIGHT+LEFT+TOP (X11-style)
 * Last field is view mode (0=icons, 1=text list, etc.)
 */
typedef struct {
    /* Archive identification */
    ULONG archiveIndex;                 /* Archive number (1-99999) */
    char archiveName[MAX_ARCHIVE_NAME]; /* "00001.lha" */
    char subFolder[MAX_FOLDER_NAME];    /* "000/" (hierarchical folder) */
    
    /* Archive metadata */
    ULONG sizeBytes;                    /* Archive file size in bytes */
    ULONG timestamp;                    /* Creation timestamp */
    UWORD iconCount;                    /* Number of .info files backed up */
    
    /* Original path (for restore) */
    char originalPath[MAX_BACKUP_PATH]; /* Full path to original folder */
    
    /* Window geometry (from folder's .info file at backup time) */
    WORD windowLeft;                    /* Window X position (-1 if not available) */
    WORD windowTop;                     /* Window Y position (-1 if not available) */
    WORD windowWidth;                   /* Window width (-1 if not available) */
    WORD windowHeight;                  /* Window height (-1 if not available) */
    UWORD viewMode;                     /* Drawer view mode (DDVM_BYICON, etc.) */
    
    /* Status */
    BOOL successful;                    /* TRUE if backup completed successfully */
    
} BackupArchiveEntry;

/*========================================================================*/
/* Backup Preferences Structure (Reference)                              */
/*========================================================================*/

/**
 * @brief Backup preferences structure
 * 
 * NOTE: This structure is defined in layout_preferences.h and embedded
 * in LayoutPreferences. Documented here for reference only.
 * 
 * typedef struct {
 *     BOOL enableUndoBackup;           // Create backup before processing
 *     BOOL useLha;                     // Use LhA compression
 *     char backupRootPath[108];        // Root backup directory path
 *     UWORD maxBackupsPerFolder;       // Maximum backup archives to retain
 * } BackupPreferences;
 */

/*========================================================================*/
/* Validation Macros                                                     */
/*========================================================================*/

/**
 * @brief Validate backup context is initialized and active
 */
#define BACKUP_CONTEXT_VALID(ctx) \
    ((ctx) != NULL && (ctx)->sessionActive && (ctx)->catalogOpen)

/**
 * @brief Validate archive index is within valid range
 */
#define ARCHIVE_INDEX_VALID(index) \
    ((index) > 0 && (index) <= MAX_ARCHIVES_PER_RUN)

/**
 * @brief Validate run number is within valid range
 */
#define RUN_NUMBER_VALID(run) \
    ((run) > 0 && (run) <= MAX_RUN_NUMBER)

/**
 * @brief Calculate hierarchical folder number from archive index
 * Example: archiveIndex 12345 → folderNum 123
 */
#define ARCHIVE_FOLDER_NUM(index) \
    ((index) / MAX_ARCHIVES_PER_FOLDER)

/*========================================================================*/
/* Size Verification                                                     */
/*========================================================================*/

/**
 * @brief Compile-time size checks (for documentation/debugging)
 * 
 * Ensuring structures are reasonably sized for stack allocation:
 * - BackupContext: ~800 bytes (large, but acceptable for stack)
 * - BackupArchiveEntry: ~350 bytes (acceptable)
 */
#ifdef DEBUG
#define BACKUP_TYPES_SIZE_CHECK() \
    do { \
        extern void __backup_types_size_check(void); \
        if (sizeof(BackupContext) > 1024) __backup_types_size_check(); \
        if (sizeof(BackupArchiveEntry) > 512) __backup_types_size_check(); \
    } while(0)
#else
#define BACKUP_TYPES_SIZE_CHECK() ((void)0)
#endif

#endif /* BACKUP_TYPES_H */
