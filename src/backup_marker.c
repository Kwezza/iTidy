/**
 * backup_marker.c - iTidy Backup System Path Marker Implementation
 * 
 * Implements path marker file creation and reading for restore operations.
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

/* Platform-specific includes FIRST */
#ifdef PLATFORM_HOST
    #ifdef _WIN32
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
    #endif
    #include <sys/stat.h>
    #include <unistd.h>
    #define DEBUG_LOG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #include <dos/dos.h>
    #include <proto/dos.h>
    #include "writeLog.h"
    #define DEBUG_LOG(fmt, ...) writeLog(LOG_DEBUG, fmt, ##__VA_ARGS__)
#endif

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
#ifdef _WIN32
        strcat(markerPathOut, "\\");
#else
        strcat(markerPathOut, "/");
#endif
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
    
#ifdef PLATFORM_HOST
    #ifdef _WIN32
        /* Windows: Use TEMP environment variable */
        char *temp = getenv("TEMP");
        if (temp) {
            strncpy(tempDirOut, temp, MAX_MARKER_PATH - 1);
            tempDirOut[MAX_MARKER_PATH - 1] = '\0';
            return TRUE;
        }
        /* Fallback to current directory */
        strcpy(tempDirOut, ".");
        return TRUE;
    #else
        /* Unix: Use /tmp */
        strcpy(tempDirOut, "/tmp");
        return TRUE;
    #endif
#else
    /* Amiga: Try TEMP: then RAM: */
    BPTR lock = Lock((STRPTR)"TEMP:", ACCESS_READ);
    if (lock) {
        UnLock(lock);
        strcpy(tempDirOut, "TEMP:");
        return TRUE;
    }
    
    lock = Lock((STRPTR)"RAM:", ACCESS_READ);
    if (lock) {
        UnLock(lock);
        strcpy(tempDirOut, "RAM:");
        return TRUE;
    }
    
    /* Fallback to current directory */
    strcpy(tempDirOut, "");
    return TRUE;
#endif
}

BOOL DeleteMarkerFile(const char *markerPath) {
    if (!markerPath) {
        return FALSE;
    }
    
#ifdef PLATFORM_HOST
    return (remove(markerPath) == 0);
#else
    return DeleteFile((STRPTR)markerPath);
#endif
}

/*========================================================================*/
/* Path Marker Creation                                                   */
/*========================================================================*/

BOOL CreatePathMarkerFile(const char *markerPath, const char *originalPath,
                          ULONG archiveIndex) {
    FILE *fp;
    char timestamp[32];
    
    if (!markerPath || !originalPath) {
        DEBUG_LOG("Invalid parameters for CreatePathMarkerFile");
        return FALSE;
    }
    
    DEBUG_LOG("Creating path marker: %s", markerPath);
    DEBUG_LOG("Original path: %s", originalPath);
    
#ifdef PLATFORM_HOST
    fp = fopen(markerPath, "w");
    if (!fp) {
        DEBUG_LOG("Failed to create marker file: %s", markerPath);
        return FALSE;
    }
    
    /* Write original path (line 1) */
    fprintf(fp, "%s\n", originalPath);
    
    /* Write timestamp (line 2) */
    FormatMarkerTimestamp(timestamp);
    fprintf(fp, "Timestamp: %s\n", timestamp);
    
    /* Write archive index (line 3) */
    fprintf(fp, "Archive: %05lu\n", archiveIndex);
    
    fclose(fp);
    return TRUE;
    
#else
    /* Amiga implementation */
    BPTR file = Open((STRPTR)markerPath, MODE_NEWFILE);
    if (!file) {
        DEBUG_LOG("Failed to create marker file: %s", markerPath);
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
#endif
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
    FILE *fp;
    char line[MAX_MARKER_PATH + 32];
    
    if (!markerPath || !originalPathOut) {
        return FALSE;
    }
    
    DEBUG_LOG("Reading path marker: %s", markerPath);
    
#ifdef PLATFORM_HOST
    fp = fopen(markerPath, "r");
    if (!fp) {
        DEBUG_LOG("Failed to open marker file: %s", markerPath);
        return FALSE;
    }
    
    /* Read first line (original path) */
    if (!fgets(originalPathOut, MAX_MARKER_PATH, fp)) {
        fclose(fp);
        return FALSE;
    }
    
    /* Remove trailing newline */
    size_t len = strlen(originalPathOut);
    if (len > 0 && originalPathOut[len-1] == '\n') {
        originalPathOut[len-1] = '\0';
    }
    
    DEBUG_LOG("Original path from marker: %s", originalPathOut);
    
    /* Optionally read archive index from line 3 */
    if (archiveIndexOut) {
        /* Skip timestamp line */
        fgets(line, sizeof(line), fp);
        
        /* Read archive line */
        if (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "Archive: %lu", archiveIndexOut) != 1) {
                *archiveIndexOut = 0;
            }
        } else {
            *archiveIndexOut = 0;
        }
    }
    
    fclose(fp);
    return TRUE;
    
#else
    /* Amiga implementation */
    BPTR file = Open((STRPTR)markerPath, MODE_OLDFILE);
    if (!file) {
        DEBUG_LOG("Failed to open marker file: %s", markerPath);
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
    
    DEBUG_LOG("Original path from marker: %s", originalPathOut);
    
    /* Archive index reading on Amiga would require more parsing */
    if (archiveIndexOut) {
        *archiveIndexOut = 0;  /* Not implemented yet */
    }
    
    Close(file);
    return TRUE;
#endif
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
    
    DEBUG_LOG("Extracting marker from: %s", archivePath);
    
    /* Extract _PATH.txt from archive */
    if (!ExtractFileFromArchive(lhaPath, archivePath, MARKER_FILENAME, 
                                 tempDirectory)) {
        DEBUG_LOG("Failed to extract marker from archive");
        return FALSE;
    }
    
    /* Build path to extracted marker */
    if (!BuildMarkerPath(markerPath, tempDirectory)) {
        return FALSE;
    }
    
    /* Read marker file */
    success = ReadPathMarkerFile(markerPath, originalPathOut, NULL);
    
    /* Clean up temp marker */
    DeleteMarkerFile(markerPath);
    
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
    
#ifdef PLATFORM_HOST
    hasMarker = (access(markerPath, F_OK) == 0);
#else
    BPTR lock = Lock((STRPTR)markerPath, ACCESS_READ);
    hasMarker = (lock != 0);
    if (lock) UnLock(lock);
#endif
    
    /* Clean up */
    DeleteMarkerFile(markerPath);
    
    return hasMarker;
}
