/**
 * backup_lha.c - iTidy Backup System LHA Wrapper Implementation
 * 
 * Implements platform-independent LHA archiver operations.
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

/* Platform-specific includes FIRST to avoid type conflicts */
/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "writeLog.h"
#define DEBUG_LOG(...) /* disabled on Amiga */
#define PATH_SEP "/"
#define NULL_DEVICE "NIL:"

#include "backup_lha.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Command buffer size */
#define MAX_COMMAND_LEN 512

/*========================================================================*/
/* Path Expansion Helper                                                  */
/*========================================================================*/

/**
 * Expand PROGDIR: to absolute path on Amiga
 * Returns TRUE if expansion successful, FALSE otherwise
 */
static BOOL ExpandProgDir(const char *path, char *expanded, size_t maxLen) {
    BPTR lock;
    char buffer[512];
    BOOL success = FALSE;
    
    if (!path || !expanded || maxLen == 0) {
        return FALSE;
    }
    
    /* If path doesn't start with PROGDIR:, just copy it */
    if (strncmp(path, "PROGDIR:", 8) != 0) {
        strncpy(expanded, path, maxLen - 1);
        expanded[maxLen - 1] = '\0';
        return TRUE;
    }
    
    /* Lock PROGDIR: to get its absolute path */
    lock = Lock((STRPTR)"PROGDIR:", ACCESS_READ);
    if (lock) {
        /* Get the full path name */
        if (NameFromLock(lock, (STRPTR)buffer, sizeof(buffer))) {
            /* Combine the expanded PROGDIR with the rest of the path */
            const char *remainder = path + 8;  /* Skip "PROGDIR:" */
            if (*remainder == '/' || *remainder == ':') {
                remainder++;  /* Skip separator */
            }
            
            /* Build the full path */
            if (*remainder) {
                snprintf(expanded, maxLen, "%s/%s", buffer, remainder);
            } else {
                strncpy(expanded, buffer, maxLen - 1);
                expanded[maxLen - 1] = '\0';
            }
            success = TRUE;
            
            append_to_log("[BACKUP] Expanded '%s' to '%s'\n", path, expanded);
        }
        UnLock(lock);
    }
    
    if (!success) {
        /* Fallback: just copy the original path */
        strncpy(expanded, path, maxLen - 1);
        expanded[maxLen - 1] = '\0';
        append_to_log("[BACKUP] WARNING: Could not expand PROGDIR:, using as-is\n");
    }
    
    return success;
}

/*========================================================================*/
/* LHA Detection                                                          */
/*========================================================================*/

BOOL CheckLhaAvailable(char *lhaPath) {
    if (!lhaPath) {
        return FALSE;
    }
    
    /* Amiga: Check common LHA locations */
    BPTR lock;
    const char *locations[] = {
        "C:LhA",
        "SYS:C/LhA",
        "SYS:Tools/LhA",
        NULL
    };
    
    for (int i = 0; locations[i] != NULL; i++) {
        lock = Lock((STRPTR)locations[i], ACCESS_READ);
        if (lock) {
            UnLock(lock);
            strcpy(lhaPath, locations[i]);
            DEBUG_LOG("Found LHA at: %s", locations[i]);
            return TRUE;
        }
    }
    
    DEBUG_LOG("LHA not found in common locations");
    return FALSE;
}

BOOL GetLhaVersion(const char *lhaPath, char *versionBuffer) {
    if (!lhaPath || !versionBuffer) {
        return FALSE;
    }
    
    /* On Amiga, version detection is more complex */
    /* For now, just return a placeholder */
    strcpy(versionBuffer, "LhA (Amiga)");
    return TRUE;
}

/*========================================================================*/
/* Utility Functions                                                      */
/*========================================================================*/

BOOL BuildLhaCommand(char *command, const char *lhaPath, 
                     const char *operation, const char *archivePath,
                     const char *targetPath) {
    int len;
    
    if (!command || !lhaPath || !operation || !archivePath) {
        return FALSE;
    }
    
    /* Basic command: lha <operation> <archive> [target] */
    if (targetPath && targetPath[0]) {
        len = snprintf(command, MAX_COMMAND_LEN, "%s %s \"%s\" \"%s\"",
                      lhaPath, operation, archivePath, targetPath);
    } else {
        len = snprintf(command, MAX_COMMAND_LEN, "%s %s \"%s\"",
                      lhaPath, operation, archivePath);
    }
    
    if (len >= MAX_COMMAND_LEN) {
        DEBUG_LOG("Command too long");
        return FALSE;
    }
    
    return TRUE;
}

BOOL ExecuteLhaCommand(const char *command) {
    int result;
    
    if (!command || !command[0]) {
        return FALSE;
    }
    
    DEBUG_LOG("Executing: %s", command);
    append_to_log("[BACKUP] Executing LHA: %s\n", command);
    
    /* Amiga: Use Execute() */
    BPTR input = Open((STRPTR)"NIL:", MODE_OLDFILE);
    BPTR output = Open((STRPTR)"NIL:", MODE_NEWFILE);
    
    if (!input || !output) {
        if (input) Close(input);
        if (output) Close(output);
        append_to_log("[BACKUP] ERROR: Failed to open NIL: for LHA execution\n");
        return FALSE;
    }
    
    result = Execute((STRPTR)command, input, output);
    
    Close(input);
    Close(output);
    
    if (result) {
        append_to_log("[BACKUP] LHA command succeeded\n");
    } else {
        append_to_log("[BACKUP] ERROR: LHA command failed (Execute returned 0)\n");
    }
    
    return (result != 0);  /* Execute() returns non-zero on success */
}

/*========================================================================*/
/* Archive Creation                                                       */
/*========================================================================*/

BOOL CreateLhaArchive(const char *lhaPath, const char *archivePath, 
                      const char *sourceDir) {
    char command[MAX_COMMAND_LEN];
    char absArchivePath[MAX_COMMAND_LEN];
    int len;
    BOOL result;
    
    if (!lhaPath || !archivePath || !sourceDir) {
        DEBUG_LOG("Invalid parameters for CreateLhaArchive");
        append_to_log("[BACKUP] ERROR: Invalid parameters for CreateLhaArchive\n");
        return FALSE;
    }
    
    DEBUG_LOG("Creating archive: %s from %s", archivePath, sourceDir);
    append_to_log("[BACKUP] Creating archive: %s from %s\n", archivePath, sourceDir);
    
    /* On Amiga, expand PROGDIR: to absolute path */
    if (!ExpandProgDir(archivePath, absArchivePath, sizeof(absArchivePath))) {
        append_to_log("[BACKUP] ERROR: Failed to expand archive path\n");
        return FALSE;
    }
    
    /* Amiga: Pass full paths directly to LHA */
    /* Archive only .info files in the root of the source directory (non-recursive) */
    /* Format: C:LhA a "archive.lha" source/dir/*.info */
    len = snprintf(command, sizeof(command),
                  "%s a \"%s\" %s/*.info",
                  lhaPath, absArchivePath, sourceDir);
    
    if (len >= MAX_COMMAND_LEN) {
        append_to_log("[BACKUP] ERROR: Command too long\n");
        return FALSE;
    }
    
    result = ExecuteLhaCommand(command);
    
    /* DEBUG: List archive contents to verify what was archived */
    if (result) {
        char listFile[MAX_COMMAND_LEN];
        char listCommand[MAX_COMMAND_LEN];
        const char *lastSlash;
        
        /* Build output filename: replace .lha with .txt */
        strncpy(listFile, absArchivePath, sizeof(listFile) - 1);
        listFile[sizeof(listFile) - 1] = '\0';
        
        /* Find the last dot and replace extension */
        lastSlash = strrchr(listFile, '.');
        if (lastSlash && strcmp(lastSlash, ".lha") == 0) {
            strcpy((char*)lastSlash, ".txt");
        } else {
            strcat(listFile, ".txt");
        }
        
        /* Build list command: lha l "archive.lha" > "output.txt" */
        len = snprintf(listCommand, sizeof(listCommand),
                      "%s l \"%s\" > \"%s\"",
                      lhaPath, absArchivePath, listFile);
        
        if (len < MAX_COMMAND_LEN) {
            log_debug(LOG_BACKUP, "Listing archive contents to: %s\n", listFile);
            ExecuteLhaCommand(listCommand);
        }
    }
    
    return result;
}

BOOL AddFileToArchive(const char *lhaPath, const char *archivePath,
                      const char *markerFile) {
    char command[MAX_COMMAND_LEN];
    char dirPath[MAX_COMMAND_LEN];
    char fileName[MAX_COMMAND_LEN];
    char absArchivePath[MAX_COMMAND_LEN];
    const char *lastSlash;
    int len;
    BOOL result;
    
    if (!lhaPath || !archivePath || !markerFile) {
        return FALSE;
    }
    
    DEBUG_LOG("Adding file to archive: %s", markerFile);
    
    /* Extract directory and filename from marker path */
    lastSlash = strrchr(markerFile, '/');
    if (!lastSlash) {
        lastSlash = strrchr(markerFile, '\\');
    }
    
    if (lastSlash) {
        /* Split into directory and filename */
        len = lastSlash - markerFile;
        if (len >= MAX_COMMAND_LEN) return FALSE;
        strncpy(dirPath, markerFile, len);
        dirPath[len] = '\0';
        strcpy(fileName, lastSlash + 1);
    } else {
        /* No directory, just filename */
        strcpy(dirPath, ".");
        strcpy(fileName, markerFile);
    }
    
    /* Amiga: Use CurrentDir() to change directory (proper AmigaDOS way) */
    BPTR oldDir, newDir;
    
    /* Use archive path as-is on Amiga (already in AmigaDOS format) */
    strcpy(absArchivePath, archivePath);
    
    newDir = Lock((STRPTR)dirPath, SHARED_LOCK);
    if (!newDir) {
        DEBUG_LOG("Failed to lock directory: %s", dirPath);
        return FALSE;
    }
    
    oldDir = CurrentDir(newDir);
    
    /* Build command without path changes */
    len = snprintf(command, sizeof(command), 
                  "%s a \"%s\" \"%s\"",
                  lhaPath, absArchivePath, fileName);
    
    if (len >= MAX_COMMAND_LEN) {
        CurrentDir(oldDir);
        UnLock(newDir);
        return FALSE;
    }
    
    result = ExecuteLhaCommand(command);
    
    /* Restore original directory */
    CurrentDir(oldDir);
    UnLock(newDir);
    
    return result;
}

ULONG GetArchiveSize(const char *archivePath) {
    if (!archivePath) {
        return 0;
    }
    
    BPTR lock;
    struct FileInfoBlock *fib;
    ULONG size = 0;
    
    lock = Lock((STRPTR)archivePath, ACCESS_READ);
    if (!lock) {
        return 0;
    }
    
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (fib) {
        if (Examine(lock, fib)) {
            size = (ULONG)fib->fib_Size;
        }
        FreeDosObject(DOS_FIB, fib);
    }
    
    UnLock(lock);
    return size;
}

/*========================================================================*/
/* Archive Extraction                                                     */
/*========================================================================*/

BOOL ExtractLhaArchive(const char *lhaPath, const char *archivePath,
                       const char *destDir) {
    char command[MAX_COMMAND_LEN];
    char absArchivePath[512];
    int len;
    
    if (!lhaPath || !archivePath || !destDir) {
        DEBUG_LOG("Invalid parameters for ExtractLhaArchive");
        return FALSE;
    }
    
    DEBUG_LOG("Extracting archive: %s to %s", archivePath, destDir);
    
    /* On Amiga, expand PROGDIR: to absolute path */
    if (!ExpandProgDir(archivePath, absArchivePath, sizeof(absArchivePath))) {
        append_to_log("[BACKUP] ERROR: Failed to expand archive path for extraction\n");
        return FALSE;
    }
    
    /* Amiga: Use CurrentDir() to change to destination directory first */
    /* LHA on Amiga extracts to current directory, not to a specified path */
    BPTR oldDir, destLock;
    BOOL result;
    
    destLock = Lock((STRPTR)destDir, SHARED_LOCK);
    if (!destLock) {
        append_to_log("[BACKUP] ERROR: Failed to lock destination directory: %s\n", destDir);
        return FALSE;
    }
    
    oldDir = CurrentDir(destLock);
    
    /* Build command: lha x archive.lha (extracts to current dir) */
    len = snprintf(command, sizeof(command), "%s x %s",
                  lhaPath, absArchivePath);
    
    if (len >= MAX_COMMAND_LEN) {
        CurrentDir(oldDir);
        UnLock(destLock);
        append_to_log("[BACKUP] ERROR: Command too long\n");
        return FALSE;
    }
    
    append_to_log("[BACKUP] Extracting to: %s\n", destDir);
    result = ExecuteLhaCommand(command);
    
    /* Restore original directory */
    CurrentDir(oldDir);
    UnLock(destLock);
    
    if (result) {
        append_to_log("[BACKUP] Extraction succeeded\n");
    } else {
        append_to_log("[BACKUP] ERROR: Extraction failed\n");
    }
    
    return result;
}

BOOL ExtractFileFromArchive(const char *lhaPath, const char *archivePath,
                             const char *fileName, const char *destDir) {
    char command[MAX_COMMAND_LEN];
    char absArchivePath[512];
    int len;
    
    if (!lhaPath || !archivePath || !fileName || !destDir) {
        return FALSE;
    }
    
    DEBUG_LOG("Extracting file: %s from %s", fileName, archivePath);
    
    /* On Amiga, expand PROGDIR: to absolute path */
    if (!ExpandProgDir(archivePath, absArchivePath, sizeof(absArchivePath))) {
        append_to_log("[BACKUP] ERROR: Failed to expand archive path for file extraction\n");
        return FALSE;
    }
    
    /* Amiga: Use CurrentDir() to change to destination directory first */
    BPTR oldDir, destLock;
    BOOL result;
    
    destLock = Lock((STRPTR)destDir, SHARED_LOCK);
    if (!destLock) {
        append_to_log("[BACKUP] ERROR: Failed to lock destination directory: %s\n", destDir);
        return FALSE;
    }
    
    oldDir = CurrentDir(destLock);
    
    /* Build command: lha x archive.lha filename (extracts to current dir) */
    len = snprintf(command, sizeof(command), "%s x %s %s",
                  lhaPath, absArchivePath, fileName);
    
    if (len >= MAX_COMMAND_LEN) {
        CurrentDir(oldDir);
        UnLock(destLock);
        return FALSE;
    }
    
    result = ExecuteLhaCommand(command);
    
    /* Restore original directory */
    CurrentDir(oldDir);
    UnLock(destLock);
    
    return result;
}

BOOL TestLhaArchive(const char *lhaPath, const char *archivePath) {
    char command[MAX_COMMAND_LEN];
    int len;
    
    if (!lhaPath || !archivePath) {
        return FALSE;
    }
    
    DEBUG_LOG("Testing archive: %s", archivePath);
    
    /* Build command: lha t archive.lha */
    len = snprintf(command, sizeof(command), "%s t \"%s\" >%s 2>&1",
                  lhaPath, archivePath, NULL_DEVICE);
    
    if (len >= MAX_COMMAND_LEN) {
        return FALSE;
    }
    
    return ExecuteLhaCommand(command);
}

/*========================================================================*/
/* Archive Information                                                    */
/*========================================================================*/

BOOL ListLhaArchive(const char *lhaPath, const char *archivePath,
                    LhaListCallback callback) {
    (void)lhaPath;
    (void)archivePath;
    (void)callback;
    return FALSE;  /* Not implemented for Amiga yet */
}
