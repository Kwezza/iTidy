/**
 * backup_lha.c - iTidy Backup System LHA Wrapper Implementation
 * 
 * Implements platform-independent LHA archiver operations.
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

/* Platform-specific includes FIRST to avoid type conflicts */
#ifdef PLATFORM_HOST
    #ifdef _WIN32
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
        #define PATH_SEP "\\"
        #define NULL_DEVICE "NUL"
    #else
        #define PATH_SEP "/"
        #define NULL_DEVICE "/dev/null"
    #endif
    #include <sys/stat.h>
    #include <unistd.h>
    #define DEBUG_LOG(fmt, ...) printf("[DEBUG] " fmt "\n", __VA_ARGS__)
#else
    #include <dos/dos.h>
    #include <proto/dos.h>
    #include <proto/exec.h>
    #include "writeLog.h"
    #define DEBUG_LOG(...) /* disabled on Amiga */
    #define PATH_SEP "/"
    #define NULL_DEVICE "NIL:"
#endif

#include "backup_lha.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Command buffer size */
#define MAX_COMMAND_LEN 512

/*========================================================================*/
/* Path Expansion Helper                                                  */
/*========================================================================*/

#ifndef PLATFORM_HOST
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
#endif

/*========================================================================*/
/* LHA Detection                                                          */
/*========================================================================*/

BOOL CheckLhaAvailable(char *lhaPath) {
    char command[MAX_COMMAND_LEN];
    int result;
    
    if (!lhaPath) {
        return FALSE;
    }
    
#ifdef PLATFORM_HOST
    /* On host, try 'lha' command */
#ifdef _WIN32
    /* Windows: Try 'lha.exe' in PATH */
    snprintf(command, sizeof(command), "lha --version >%s 2>&1", NULL_DEVICE);
    result = system(command);
    
    if (result == 0) {
        strcpy(lhaPath, "lha");
        DEBUG_LOG("Found LHA in PATH");
        return TRUE;
    }
    
    /* Try 'lha.exe' explicitly */
    snprintf(command, sizeof(command), "lha.exe --version >%s 2>&1", NULL_DEVICE);
    result = system(command);
    
    if (result == 0) {
        strcpy(lhaPath, "lha.exe");
        DEBUG_LOG("Found lha.exe in PATH");
        return TRUE;
    }
#else
    /* Unix/Linux: Try 'lha' command */
    snprintf(command, sizeof(command), "which lha >%s 2>&1", NULL_DEVICE);
    result = system(command);
    
    if (result == 0) {
        strcpy(lhaPath, "lha");
        DEBUG_LOG("Found LHA in PATH");
        return TRUE;
    }
#endif
    
    DEBUG_LOG("LHA not found in PATH");
    return FALSE;
    
#else
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
#endif
}

BOOL GetLhaVersion(const char *lhaPath, char *versionBuffer) {
    char command[MAX_COMMAND_LEN];
    FILE *fp;
    
    if (!lhaPath || !versionBuffer) {
        return FALSE;
    }
    
#ifdef PLATFORM_HOST
    /* Execute lha --version and capture output */
    snprintf(command, sizeof(command), "%s --version 2>&1", lhaPath);
    
    fp = popen(command, "r");
    if (!fp) {
        return FALSE;
    }
    
    /* Read first line of output */
    if (fgets(versionBuffer, 64, fp) != NULL) {
        /* Remove trailing newline */
        size_t len = strlen(versionBuffer);
        if (len > 0 && versionBuffer[len-1] == '\n') {
            versionBuffer[len-1] = '\0';
        }
        pclose(fp);
        return TRUE;
    }
    
    pclose(fp);
    return FALSE;
#else
    /* On Amiga, version detection is more complex */
    /* For now, just return a placeholder */
    strcpy(versionBuffer, "LhA (Amiga)");
    return TRUE;
#endif
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
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Executing LHA: %s\n", command);
#endif
    
#ifdef PLATFORM_HOST
    result = system(command);
    
    /* system() returns 0 on success */
    if (result == 0) {
        DEBUG_LOG("Command succeeded");
        return TRUE;
    } else {
        DEBUG_LOG("Command failed with code: %d", result);
        return FALSE;
    }
#else
    /* Amiga: Use Execute() */
    BPTR input = Open((STRPTR)"NIL:", MODE_OLDFILE);
    BPTR output = Open((STRPTR)"NIL:", MODE_NEWFILE);
    
    if (!input || !output) {
        if (input) Close(input);
        if (output) Close(output);
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] ERROR: Failed to open NIL: for LHA execution\n");
#endif
        return FALSE;
    }
    
    result = Execute((STRPTR)command, input, output);
    
    Close(input);
    Close(output);
    
#ifndef PLATFORM_HOST
    if (result) {
        append_to_log("[BACKUP] LHA command succeeded\n");
    } else {
        append_to_log("[BACKUP] ERROR: LHA command failed (Execute returned 0)\n");
    }
#endif
    
    return (result != 0);  /* Execute() returns non-zero on success */
#endif
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
#ifndef PLATFORM_HOST
        append_to_log("[BACKUP] ERROR: Invalid parameters for CreateLhaArchive\n");
#endif
        return FALSE;
    }
    
    DEBUG_LOG("Creating archive: %s from %s", archivePath, sourceDir);
#ifndef PLATFORM_HOST
    append_to_log("[BACKUP] Creating archive: %s from %s\n", archivePath, sourceDir);
#endif
    
#ifdef PLATFORM_HOST
    /* Convert archive path to absolute path for host builds */
    #ifdef _WIN32
    if (_fullpath(absArchivePath, archivePath, sizeof(absArchivePath)) == NULL) {
        strcpy(absArchivePath, archivePath);
    }
    #else
    if (realpath(archivePath, absArchivePath) == NULL) {
        strcpy(absArchivePath, archivePath);
    }
    #endif
#else
    /* On Amiga, expand PROGDIR: to absolute path */
    if (!ExpandProgDir(archivePath, absArchivePath, sizeof(absArchivePath))) {
        append_to_log("[BACKUP] ERROR: Failed to expand archive path\n");
        return FALSE;
    }
#endif
    
    /* Build command: change to source dir and archive from there */
    /* This avoids issues with wildcards in paths */
#ifdef PLATFORM_HOST
    #ifdef _WIN32
        /* Windows: Use cmd.exe for proper wildcard expansion (PowerShell doesn't expand wildcards in external commands)
         * Note: -r flag removed as this version of LHA recurses by default */
        len = snprintf(command, sizeof(command), 
                      "cmd /c \"cd /d \"%s\" && %s a \"%s\" *\"",
                      sourceDir, lhaPath, absArchivePath);
    #else
        /* Unix: use cd and archive (removed -r flag) */
        len = snprintf(command, sizeof(command),
                      "cd \"%s\" && %s a \"%s\" *",
                      sourceDir, lhaPath, absArchivePath);
    #endif
    
    if (len >= MAX_COMMAND_LEN) {
        DEBUG_LOG("Command too long");
        return FALSE;
    }
    
    return ExecuteLhaCommand(command);
#else
    /* Amiga: Pass full paths directly to LHA */
    /* Archive only .info files with recursive search */
    /* Format: C:LhA a -r "archive.lha" source/dir/ *.info */
    /* The trailing slash makes it a home directory, and *.info is the pattern */
    len = snprintf(command, sizeof(command),
                  "%s a -r \"%s\" %s/ *.info",
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
            append_to_log("[BACKUP] DEBUG: Listing archive contents to: %s\n", listFile);
            ExecuteLhaCommand(listCommand);
        }
    }
    
    return result;
#endif
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
    
#ifdef PLATFORM_HOST
    /* Convert archive path to absolute path for host builds */
    #ifdef _WIN32
    if (_fullpath(absArchivePath, archivePath, sizeof(absArchivePath)) == NULL) {
        strcpy(absArchivePath, archivePath);
    }
    #else
    if (realpath(archivePath, absArchivePath) == NULL) {
        strcpy(absArchivePath, archivePath);
    }
    #endif
    
    /* Build command: change to directory and add just the filename */
    /* This ensures the file is stored without path in the archive */
    #ifdef _WIN32
    len = snprintf(command, sizeof(command), 
                  "pushd \"%s\" & %s a \"%s\" \"%s\" & popd",
                  dirPath, lhaPath, absArchivePath, fileName);
    #else
    len = snprintf(command, sizeof(command), 
                  "cd \"%s\" && %s a \"%s\" \"%s\"",
                  dirPath, lhaPath, absArchivePath, fileName);
    #endif
    
    if (len >= MAX_COMMAND_LEN) {
        return FALSE;
    }
    
    return ExecuteLhaCommand(command);
#else
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
#endif
}

ULONG GetArchiveSize(const char *archivePath) {
    if (!archivePath) {
        return 0;
    }
    
#ifdef PLATFORM_HOST
    struct stat st;
    if (stat(archivePath, &st) == 0) {
        return (ULONG)st.st_size;
    }
    return 0;
#else
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
#endif
}

/*========================================================================*/
/* Archive Extraction                                                     */
/*========================================================================*/

BOOL ExtractLhaArchive(const char *lhaPath, const char *archivePath,
                       const char *destDir) {
    char command[MAX_COMMAND_LEN];
    int len;
    
    if (!lhaPath || !archivePath || !destDir) {
        DEBUG_LOG("Invalid parameters for ExtractLhaArchive");
        return FALSE;
    }
    
    DEBUG_LOG("Extracting archive: %s to %s", archivePath, destDir);
    
    /* Build command: lha x archive.lha destdir/ */
#ifdef PLATFORM_HOST
    len = snprintf(command, sizeof(command), "%s x \"%s\" -w=\"%s\"",
                  lhaPath, archivePath, destDir);
#else
    /* Amiga LhA uses different syntax for destination */
    len = snprintf(command, sizeof(command), "%s x %s %s",
                  lhaPath, archivePath, destDir);
#endif
    
    if (len >= MAX_COMMAND_LEN) {
        DEBUG_LOG("Command too long");
        return FALSE;
    }
    
    return ExecuteLhaCommand(command);
}

BOOL ExtractFileFromArchive(const char *lhaPath, const char *archivePath,
                             const char *fileName, const char *destDir) {
    char command[MAX_COMMAND_LEN];
    int len;
    
    if (!lhaPath || !archivePath || !fileName || !destDir) {
        return FALSE;
    }
    
    DEBUG_LOG("Extracting file: %s from %s", fileName, archivePath);
    
    /* Build command: lha x archive.lha filename -w=destdir */
#ifdef PLATFORM_HOST
    len = snprintf(command, sizeof(command), "%s x \"%s\" \"%s\" -w=\"%s\"",
                  lhaPath, archivePath, fileName, destDir);
#else
    len = snprintf(command, sizeof(command), "%s x %s %s %s",
                  lhaPath, archivePath, fileName, destDir);
#endif
    
    if (len >= MAX_COMMAND_LEN) {
        return FALSE;
    }
    
    return ExecuteLhaCommand(command);
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
    char command[MAX_COMMAND_LEN];
    FILE *fp;
    char line[512];
    int len;
    
    if (!lhaPath || !archivePath || !callback) {
        return FALSE;
    }
    
#ifdef PLATFORM_HOST
    /* Build command: lha l archive.lha */
    len = snprintf(command, sizeof(command), "%s l \"%s\" 2>&1",
                  lhaPath, archivePath);
    
    if (len >= MAX_COMMAND_LEN) {
        return FALSE;
    }
    
    DEBUG_LOG("Listing archive: %s", archivePath);
    
    fp = popen(command, "r");
    if (!fp) {
        return FALSE;
    }
    
    /* Parse output - format varies by LHA version */
    /* Typically: filename, size, ratio, date, time */
    while (fgets(line, sizeof(line), fp)) {
        char fileName[256];
        ULONG size;
        
        /* Skip header lines */
        if (line[0] == '-' || strstr(line, "PERMSSN") || 
            strstr(line, "files") || strlen(line) < 10) {
            continue;
        }
        
        /* Try to parse filename and size */
        /* This is simplified - real parsing depends on LHA output format */
        if (sscanf(line, "%*s %lu %*s %*s %*s %255s", &size, fileName) == 2) {
            if (!callback(fileName, size)) {
                break;  /* Callback requested stop */
            }
        }
    }
    
    pclose(fp);
    return TRUE;
#else
    /* Amiga implementation would use similar approach */
    /* but with Execute() and capturing output differently */
    return FALSE;  /* Not implemented for Amiga yet */
#endif
}
