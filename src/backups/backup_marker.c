/**
 * backup_marker.c - iTidy Backup System Path Marker Implementation
 * 
 * Implements path marker file creation and reading for restore operations.
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

#include "../platform/platform.h"

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

/* Amiga-specific includes */
#include <dos/dos.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "../writeLog.h"
#define DEBUG_LOG(...) /* disabled on Amiga */

#include "backup_marker.h"
#include "backup_lha.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*========================================================================*/
/* Utility Functions                                                      */
/*========================================================================*/

BOOL BuildMarkerPath(char *markerPathOut, const char *directory) {
    size_t len;
    
    if (!markerPathOut || !directory) {
        return FALSE;
    }
    
    /* Build path: directory + "/" + MARKER_FILENAME */
    len = strlen(directory);
    if (len == 0 || len > MAX_MARKER_PATH - 32) {
        return FALSE;
    }
    
    strcpy(markerPathOut, directory);
    
    /* Add path separator if needed */
    if (directory[len-1] != '/' && directory[len-1] != ':' && 
        directory[len-1] != '\\') {
        strcat(markerPathOut, "/");
    }
    
    strcat(markerPathOut, MARKER_FILENAME);
    return TRUE;
}

BOOL FormatMarkerTimestamp(char *timestampOut) {
    time_t now;
    struct tm *timeinfo;
    
    if (!timestampOut) {
        return FALSE;
    }
    
    time(&now);
    timeinfo = localtime(&now);
    
    if (timeinfo) {
        strftime(timestampOut, 32, "%Y-%m-%d %H:%M:%S", timeinfo);
        return TRUE;
    }
    
    strcpy(timestampOut, "Unknown");
    return FALSE;
}

BOOL GetTempDirectory(char *tempDirOut) {
    if (!tempDirOut) {
        return FALSE;
    }
    
    /* Amiga: Try T: (standard temporary) then RAM: */
    BPTR lock = Lock((STRPTR)"T:", ACCESS_READ);
    if (lock) {
        UnLock(lock);
        strcpy(tempDirOut, "T:");
        append_to_log("[BACKUP] Using temp directory: T:\n");
        return TRUE;
    }
    
    lock = Lock((STRPTR)"RAM:", ACCESS_READ);
    if (lock) {
        UnLock(lock);
        strcpy(tempDirOut, "RAM:");
        append_to_log("[BACKUP] Using temp directory: RAM:\n");
        return TRUE;
    }
    
    /* Fallback to current directory */
    strcpy(tempDirOut, "");
    append_to_log("[BACKUP] WARNING: No temp directory available, using current directory\n");
    return TRUE;
}

BOOL DeleteMarkerFile(const char *markerPath) {
    if (!markerPath) {
        return FALSE;
    }
    
    return DeleteFile((STRPTR)markerPath);
}

/*========================================================================*/
/* Path Marker Creation                                                   */
/*========================================================================*/

BOOL CreatePathMarkerFile(const char *markerPath, const char *originalPath,
                          ULONG archiveIndex) {
    char timestamp[32];
    
    if (!markerPath || !originalPath) {
        append_to_log("[BACKUP] Invalid parameters for CreatePathMarkerFile\n");
        return FALSE;
    }
    
    append_to_log("[BACKUP] Creating path marker: %s\n", markerPath);
    append_to_log("[BACKUP] Original path: %s\n", originalPath);
    
    BPTR file = Open((STRPTR)markerPath, MODE_NEWFILE);
    if (!file) {
        /* DEBUG_LOG("Failed to create marker file: %s", markerPath); */
        return FALSE;
    }
    
    /* Write original path */
    LONG len = strlen(originalPath);
    Write(file, (APTR)originalPath, len);
    Write(file, (APTR)"\n", 1);
    
    /* Write timestamp */
    FormatMarkerTimestamp(timestamp);
    char line[128];
    snprintf(line, sizeof(line), "Timestamp: %s\n", timestamp);
    len = strlen(line);
    Write(file, (APTR)line, len);
    
    /* Write archive index */
    snprintf(line, sizeof(line), "Archive: %05lu\n", archiveIndex);
    len = strlen(line);
    Write(file, (APTR)line, len);
    
    Close(file);
    return TRUE;
}

BOOL CreateTempPathMarker(char *markerPathOut, const char *originalPath,
                          ULONG archiveIndex, const char *tempDir) {
    char tempDirectory[MAX_MARKER_PATH];
    
    if (!markerPathOut || !originalPath) {
        return FALSE;
    }
    
    /* Determine temp directory */
    if (tempDir && tempDir[0]) {
        strncpy(tempDirectory, tempDir, sizeof(tempDirectory) - 1);
        tempDirectory[sizeof(tempDirectory) - 1] = '\0';
    } else {
        if (!GetTempDirectory(tempDirectory)) {
            return FALSE;
        }
    }
    
    /* Build marker path */
    if (!BuildMarkerPath(markerPathOut, tempDirectory)) {
        return FALSE;
    }
    
    /* Create marker file */
    return CreatePathMarkerFile(markerPathOut, originalPath, archiveIndex);
}

/*========================================================================*/
/* Path Marker Reading                                                    */
/*========================================================================*/

BOOL ReadPathMarkerFile(const char *markerPath, char *originalPathOut,
                        ULONG *archiveIndexOut) {
    char line[MAX_MARKER_PATH + 32];
    
    if (!markerPath || !originalPathOut) {
        return FALSE;
    }
    
    /* DEBUG_LOG("Reading path marker: %s", markerPath); */
    
    BPTR file = Open((STRPTR)markerPath, MODE_OLDFILE);
    if (!file) {
        /* DEBUG_LOG("Failed to open marker file: %s", markerPath); */
        return FALSE;
    }
    
    /* Read first line */
    LONG bytesRead = Read(file, originalPathOut, MAX_MARKER_PATH - 1);
    if (bytesRead <= 0) {
        Close(file);
        return FALSE;
    }
    
    originalPathOut[bytesRead] = '\0';
    
    /* Find first newline and terminate there */
    char *newline = strchr(originalPathOut, '\n');
    if (newline) {
        *newline = '\0';
    }
    
    /* DEBUG_LOG("Original path from marker: %s", originalPathOut); */
    
    /* Archive index reading on Amiga would require more parsing */
    if (archiveIndexOut) {
        *archiveIndexOut = 0;  /* Not implemented yet */
    }
    
    Close(file);
    return TRUE;
}

BOOL ExtractAndReadMarker(const char *archivePath, const char *lhaPath,
                          char *originalPathOut, const char *tempDir) {
    char tempDirectory[MAX_MARKER_PATH];
    char markerPath[MAX_MARKER_PATH];
    BOOL success;
    
    if (!archivePath || !lhaPath || !originalPathOut) {
        return FALSE;
    }
    
    /* Get temp directory */
    if (tempDir && tempDir[0]) {
        strncpy(tempDirectory, tempDir, sizeof(tempDirectory) - 1);
        tempDirectory[sizeof(tempDirectory) - 1] = '\0';
    } else {
        if (!GetTempDirectory(tempDirectory)) {
            return FALSE;
        }
    }
    
    /* DEBUG_LOG("Extracting marker from: %s", archivePath); */
    /* DEBUG_LOG("Target temp directory: %s", tempDirectory); */
    
    /* Extract entire archive to temp (single file extraction doesn't work reliably) */
    if (!ExtractLhaArchive(lhaPath, archivePath, tempDirectory)) {
        /* DEBUG_LOG("Failed to extract archive"); */
        return FALSE;
    }
    
    /* Build path to extracted marker */
    if (!BuildMarkerPath(markerPath, tempDirectory)) {
        return FALSE;
    }
    
    /* DEBUG_LOG("Expected marker path: %s", markerPath); */
    append_to_log("[BACKUP] Expected marker path: %s\n", markerPath);
    
    /* Amiga: Check if marker file exists */
    {
        BPTR lock = Lock((STRPTR)markerPath, ACCESS_READ);
        if (!lock) {
            append_to_log("[BACKUP] ERROR: Marker file not found: %s\n", markerPath);
            /* List files in temp directory for debugging */
            BPTR dirLock = Lock((STRPTR)tempDirectory, ACCESS_READ);
            if (dirLock) {
                struct FileInfoBlock *fib = (struct FileInfoBlock *)whd_malloc(sizeof(struct FileInfoBlock));
                if (fib) {
                    memset(fib, 0, sizeof(struct FileInfoBlock));
                    if (Examine(dirLock, fib)) {
                        append_to_log("[BACKUP] Files in %s:\n", tempDirectory);
                        while (ExNext(dirLock, fib)) {
                            append_to_log("[BACKUP]   - %s\n", fib->fib_FileName);
                        }
                    }
                }
                if (fib) FreeVec(fib);
                UnLock(dirLock);
            }
            return FALSE;
        }
        UnLock(lock);
        append_to_log("[BACKUP] Marker file found: %s\n", markerPath);
    }
    
    /* Read marker file */
    success = ReadPathMarkerFile(markerPath, originalPathOut, NULL);
    
    /* Clean up temp marker */
    if (success) {
        DeleteMarkerFile(markerPath);
    }
    
    return success;
}

/*========================================================================*/
/* Path Marker Validation                                                 */
/*========================================================================*/

BOOL ValidatePathMarker(const char *markerPath) {
    char originalPath[MAX_MARKER_PATH];
    
    if (!markerPath) {
        return FALSE;
    }
    
    /* Try to read the marker */
    if (!ReadPathMarkerFile(markerPath, originalPath, NULL)) {
        return FALSE;
    }
    
    /* Check that path is not empty */
    if (originalPath[0] == '\0') {
        return FALSE;
    }
    
    /* Basic validation passed */
    return TRUE;
}

BOOL ArchiveHasMarker(const char *archivePath, const char *lhaPath) {
    char tempDir[MAX_MARKER_PATH];
    char markerPath[MAX_MARKER_PATH];
    BOOL hasMarker;
    
    if (!archivePath || !lhaPath) {
        return FALSE;
    }
    
    /* Get temp directory */
    if (!GetTempDirectory(tempDir)) {
        return FALSE;
    }
    
    /* Try to extract marker */
    if (!ExtractFileFromArchive(lhaPath, archivePath, MARKER_FILENAME, tempDir)) {
        return FALSE;
    }
    
    /* Check if marker exists */
    BuildMarkerPath(markerPath, tempDir);
    
    BPTR lock = Lock((STRPTR)markerPath, ACCESS_READ);
    hasMarker = (lock != 0);
    if (lock) UnLock(lock);
    
    /* Clean up */
    DeleteMarkerFile(markerPath);
    
    return hasMarker;
}
