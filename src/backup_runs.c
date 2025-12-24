/**
 * backup_runs.c - iTidy Backup System Run Management Implementation
 * 
 * Implements run directory scanning and creation.
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

#include "platform/platform.h"
#include "backup_runs.h"
#include "backup_paths.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

/* Amiga-specific includes */
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include "writeLog.h"
#define DEBUG_LOG(...) /* disabled on Amiga */

/*========================================================================*/
/* Internal Helper Functions                                             */
/*========================================================================*/

/**
 * @brief Create a single directory (not recursive)
 */
static BOOL CreateSingleDirectory(const char *path) {
    BPTR lock;
    
    /* Check if already exists */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock) {
        UnLock(lock);
        return TRUE; /* Already exists */
    }
    
    /* Create directory */
    lock = CreateDir((STRPTR)path);
    if (lock) {
        UnLock(lock);
        /* DEBUG_LOG("Created directory: %s", path); */
        return TRUE;
    }
    
    /* Check if it exists now (might have been created) */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock) {
        UnLock(lock);
        return TRUE;
    }
    
    /* DEBUG_LOG("Failed to create directory: %s", path); */
    return FALSE;
}

/*========================================================================*/
/* Run Number Parsing and Formatting                                     */
/*========================================================================*/

void FormatRunDirectoryName(char *outName, UWORD runNumber) {
    if (!outName) {
        return;
    }
    
    /* Format as "Run_NNNN" with 4-digit zero-padded number */
    snprintf(outName, MAX_RUN_DIR_NAME, "Run_%04u", runNumber);
}

UWORD ParseRunNumber(const char *dirName) {
    const char *p;
    UWORD number;
    int digitCount;
    
    if (!dirName) {
        return 0;
    }
    
    /* Check for "Run_" prefix */
    if (strncmp(dirName, "Run_", 4) != 0) {
        return 0;
    }
    
    /* Move past "Run_" */
    p = dirName + 4;
    
    /* Parse numeric portion */
    number = 0;
    digitCount = 0;
    
    while (*p >= '0' && *p <= '9') {
        number = number * 10 + (*p - '0');
        digitCount++;
        p++;
    }
    
    /* Must have exactly 4 digits and be at end of string */
    if (digitCount != 4 || *p != '\0') {
        return 0;
    }
    
    /* Validate range */
    if (!RUN_NUMBER_VALID(number)) {
        return 0;
    }
    
    return number;
}

/*========================================================================*/
/* Directory Scanning                                                    */
/*========================================================================*/

UWORD FindHighestRunNumber(const char *backupRoot) {
    UWORD highest = 0;
    UWORD current;
    
    if (!backupRoot) {
        return 0;
    }
    
    /* AmigaDOS version using pattern matching */
    struct AnchorPath *anchor;
    LONG result;
    char pattern[MAX_BACKUP_PATH * 2];  /* Larger to accommodate full paths */
    
    /* Allocate anchor structure with extra space for full pathnames */
    anchor = (struct AnchorPath *)whd_malloc(sizeof(struct AnchorPath) + 512);
    if (!anchor) {
        /* DEBUG_LOG("Failed to allocate AnchorPath"); */
        return 0;
    }
    memset(anchor, 0, sizeof(struct AnchorPath) + 512);
    
    anchor->ap_BreakBits = 0;
    anchor->ap_Strlen = 512;  /* BUGFIX: Must set buffer length! */
    
    /* Build pattern: "backupRoot/Run_#?" */
    snprintf(pattern, sizeof(pattern), "%s/Run_#?", backupRoot);
    
    /* Scan for matching directories */
    result = MatchFirst(pattern, anchor);
    
    while (result == 0) {
        /* Check if it's a directory */
        if (anchor->ap_Info.fib_DirEntryType > 0) {
            /* Try to parse run number from name */
            current = ParseRunNumber(anchor->ap_Info.fib_FileName);
            if (current > 0 && current > highest) {
                highest = current;
                DEBUG_LOG("Found run: %s (number=%u)", 
                         anchor->ap_Info.fib_FileName, current);
            }
        }
        
        result = MatchNext(anchor);
    }
    
    MatchEnd(anchor);
    FreeVec(anchor);
    
    /* DEBUG_LOG("Highest run number found: %u", highest); */
    return highest;
}

UWORD CountRunDirectories(const char *backupRoot) {
    UWORD count = 0;
    
    if (!backupRoot) {
        return 0;
    }
    
    struct AnchorPath *anchor;
    LONG result;
    char pattern[MAX_BACKUP_PATH * 2];  /* Larger to accommodate full paths */
    
    anchor = (struct AnchorPath *)whd_malloc(sizeof(struct AnchorPath) + 512);
    if (!anchor) {
        return 0;
    }
    memset(anchor, 0, sizeof(struct AnchorPath) + 512);
    
    anchor->ap_BreakBits = 0;
    anchor->ap_Strlen = 512;  /* BUGFIX: Must set buffer length! */
    
    snprintf(pattern, sizeof(pattern), "%s/Run_#?", backupRoot);
    result = MatchFirst(pattern, anchor);
    
    while (result == 0) {
        if (anchor->ap_Info.fib_DirEntryType > 0) {
            if (ParseRunNumber(anchor->ap_Info.fib_FileName) > 0) {
                count++;
            }
        }
        result = MatchNext(anchor);
    }
    
    MatchEnd(anchor);
    FreeVec(anchor);
    
    return count;
}

/*========================================================================*/
/* Directory Creation                                                    */
/*========================================================================*/

BOOL BackupRootExists(const char *backupRoot) {
    if (!backupRoot) {
        return FALSE;
    }
    
    BPTR lock = Lock((STRPTR)backupRoot, ACCESS_READ);
    if (lock) {
        UnLock(lock);
        return TRUE;
    }
    return FALSE;
}

BOOL CreateBackupRoot(const char *backupRoot) {
    char tempPath[MAX_BACKUP_PATH];
    char *p;
    int length;
    
    if (!backupRoot) {
        return FALSE;
    }
    
    /* Check if already exists */
    if (BackupRootExists(backupRoot)) {
        return TRUE;
    }
    
    /* Copy path for manipulation */
    strncpy(tempPath, backupRoot, sizeof(tempPath) - 1);
    tempPath[sizeof(tempPath) - 1] = '\0';
    
    /* Find device separator */
    p = strchr(tempPath, ':');
    if (p) {
        p++; /* Start after colon */
    } else {
        p = tempPath;
    }
    
    /* Create each directory level */
    while (*p) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            if (!CreateSingleDirectory(tempPath)) {
                /* Ignore error if directory exists */
                if (!BackupRootExists(tempPath)) {
                    /* DEBUG_LOG("Failed to create intermediate directory: %s", tempPath); */
                }
            }
            *p = '/';
        }
        p++;
    }
    
    /* Create final directory */
    if (!CreateSingleDirectory(tempPath)) {
        if (!BackupRootExists(tempPath)) {
            /* DEBUG_LOG("Failed to create backup root: %s", tempPath); */
            return FALSE;
        }
    }
    
    /* DEBUG_LOG("Backup root ready: %s", backupRoot); */
    return TRUE;
}

BOOL GetRunDirectoryPath(char *outPath, const char *backupRoot, UWORD runNumber) {
    char runDirName[MAX_RUN_DIR_NAME];
    int length;
    size_t rootLen;
    BOOL hasTrailingSlash;
    
    if (!outPath || !backupRoot) {
        return FALSE;
    }
    
    if (!RUN_NUMBER_VALID(runNumber)) {
        /* DEBUG_LOG("Invalid run number: %u", runNumber); */
        return FALSE;
    }
    
    /* Format run directory name */
    FormatRunDirectoryName(runDirName, runNumber);
    
    /* Check if backupRoot has trailing slash */
    rootLen = strlen(backupRoot);
    hasTrailingSlash = (rootLen > 0 && (backupRoot[rootLen - 1] == '/' || backupRoot[rootLen - 1] == ':'));
    
    /* Build full path - add separator only if needed */
    if (hasTrailingSlash) {
        length = snprintf(outPath, MAX_BACKUP_PATH, "%s%s", backupRoot, runDirName);
    } else {
        length = snprintf(outPath, MAX_BACKUP_PATH, "%s/%s", backupRoot, runDirName);
    }
    
    if (length >= MAX_BACKUP_PATH) {
        /* DEBUG_LOG("Run directory path too long: %d chars", length); */
        return FALSE;
    }
    
    return TRUE;
}

BOOL CreateNextRunDirectory(const char *backupRoot, char *outRunPath, UWORD *outRunNumber) {
    UWORD highestRun;
    UWORD nextRun;
    
    if (!backupRoot || !outRunPath || !outRunNumber) {
        /* DEBUG_LOG("Invalid parameters to CreateNextRunDirectory"); */
        return FALSE;
    }
    
    /* Ensure backup root exists */
    if (!CreateBackupRoot(backupRoot)) {
        /* DEBUG_LOG("Failed to create backup root"); */
        return FALSE;
    }
    
    /* Find highest existing run number */
    highestRun = FindHighestRunNumber(backupRoot);
    
    /* Calculate next run number */
    if (highestRun >= MAX_RUN_NUMBER) {
        /* DEBUG_LOG("Maximum run number reached: %u", highestRun); */
        return FALSE;
    }
    
    nextRun = highestRun + 1;
    
    /* Build run directory path */
    if (!GetRunDirectoryPath(outRunPath, backupRoot, nextRun)) {
        return FALSE;
    }
    
    /* Create the run directory */
    if (!CreateSingleDirectory(outRunPath)) {
        /* DEBUG_LOG("Failed to create run directory: %s", outRunPath); */
        return FALSE;
    }
    
    *outRunNumber = nextRun;
    /* DEBUG_LOG("Created new run: %s (number=%u)", outRunPath, nextRun); */
    
    return TRUE;
}
