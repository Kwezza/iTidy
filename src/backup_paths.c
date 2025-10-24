/**
 * backup_paths.c - iTidy Backup System Path Utilities Implementation
 * 
 * Implements path manipulation and validation functions for the backup system.
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

#include "backup_paths.h"
#include <string.h>
#include <stdio.h>

#ifndef PLATFORM_HOST
#include "writeLog.h"
#define DEBUG_LOG(fmt, ...) writeLog(LOG_DEBUG, fmt, ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#endif

/*========================================================================*/
/* Root Folder Detection                                                 */
/*========================================================================*/

BOOL IsRootFolder(const char *path) {
    const char *colon;
    
    if (!path) {
        return FALSE;
    }
    
    /* Find the device separator */
    colon = strchr(path, ':');
    if (!colon) {
        /* No colon = not a valid AmigaDOS path */
        return FALSE;
    }
    
    /* Check if there's anything after the colon */
    /* Root paths end immediately after ':' or only have whitespace */
    colon++; /* Move past the colon */
    
    /* Skip any whitespace */
    while (*colon == ' ' || *colon == '\t') {
        colon++;
    }
    
    /* If we're at the end or only have trailing slash, it's a root */
    if (*colon == '\0' || (*colon == '/' && *(colon + 1) == '\0')) {
        return TRUE;
    }
    
    return FALSE;
}

/*========================================================================*/
/* Archive Path Calculation                                              */
/*========================================================================*/

BOOL CalculateArchivePath(char *outPath, const char *runDirectory, ULONG archiveIndex) {
    ULONG folderNum;
    char archiveName[MAX_ARCHIVE_NAME];
    int length;
    
    if (!outPath || !runDirectory) {
        return FALSE;
    }
    
    if (!ARCHIVE_INDEX_VALID(archiveIndex)) {
        DEBUG_LOG("Invalid archive index: %lu", archiveIndex);
        return FALSE;
    }
    
    /* Calculate hierarchical folder number (archiveIndex / 100) */
    folderNum = ARCHIVE_FOLDER_NUM(archiveIndex);
    
    /* Format archive filename */
    FormatArchiveName(archiveName, archiveIndex);
    
    /* Build full path: runDirectory/folderNum/archiveName */
    length = snprintf(outPath, MAX_BACKUP_PATH, 
                     "%s/%03u/%s", 
                     runDirectory, folderNum, archiveName);
    
    /* Check if path would be too long */
    if (length >= MAX_BACKUP_PATH) {
        DEBUG_LOG("Archive path too long: %d chars", length);
        return FALSE;
    }
    
    return TRUE;
}

BOOL CalculateSubfolderPath(char *outPath, const char *runDirectory, ULONG archiveIndex) {
    ULONG folderNum;
    int length;
    
    if (!outPath || !runDirectory) {
        return FALSE;
    }
    
    if (!ARCHIVE_INDEX_VALID(archiveIndex)) {
        return FALSE;
    }
    
    /* Calculate hierarchical folder number */
    folderNum = ARCHIVE_FOLDER_NUM(archiveIndex);
    
    /* Build subfolder path: runDirectory/folderNum */
    length = snprintf(outPath, MAX_BACKUP_PATH, 
                     "%s/%03u", 
                     runDirectory, folderNum);
    
    if (length >= MAX_BACKUP_PATH) {
        return FALSE;
    }
    
    return TRUE;
}

void FormatArchiveName(char *outName, ULONG archiveIndex) {
    if (!outName) {
        return;
    }
    
    /* Format as 5-digit number with .lha extension */
    snprintf(outName, MAX_ARCHIVE_NAME, "%05lu.lha", archiveIndex);
}

/*========================================================================*/
/* Path Manipulation                                                     */
/*========================================================================*/

BOOL GetParentPath(const char *fullPath, char *parentPath) {
    const char *lastSlash;
    const char *colon;
    int length;
    
    if (!fullPath || !parentPath) {
        return FALSE;
    }
    
    /* Copy the input path */
    strncpy(parentPath, fullPath, MAX_BACKUP_PATH - 1);
    parentPath[MAX_BACKUP_PATH - 1] = '\0';
    
    /* Find the last slash */
    lastSlash = strrchr(parentPath, '/');
    
    if (lastSlash) {
        /* Truncate at the last slash */
        length = lastSlash - parentPath;
        parentPath[length] = '\0';
        return TRUE;
    }
    
    /* No slash found - check for device separator */
    colon = strchr(parentPath, ':');
    if (colon) {
        /* Check if there's something after the colon */
        if (*(colon + 1) != '\0') {
            /* Truncate to just the device name + colon */
            length = (colon - parentPath) + 1;
            parentPath[length] = '\0';
            return TRUE;
        }
        
        /* Already at root (e.g., "DH0:") */
        return FALSE;
    }
    
    /* Invalid path - no device separator */
    return FALSE;
}

void GetFolderName(const char *fullPath, char *folderName) {
    const char *lastSlash;
    const char *colon;
    const char *start;
    
    if (!fullPath || !folderName) {
        if (folderName) {
            folderName[0] = '\0';
        }
        return;
    }
    
    /* Find the last slash */
    lastSlash = strrchr(fullPath, '/');
    
    if (lastSlash) {
        /* Copy everything after the last slash */
        strcpy(folderName, lastSlash + 1);
        return;
    }
    
    /* No slash - check for device separator */
    colon = strchr(fullPath, ':');
    if (colon) {
        start = colon + 1;
        
        /* Skip any leading slashes after colon */
        while (*start == '/') {
            start++;
        }
        
        strcpy(folderName, start);
        return;
    }
    
    /* No device separator - copy entire string */
    strcpy(folderName, fullPath);
}

BOOL GetDrawerIconPath(const char *folderPath, char *iconPath) {
    int length;
    
    if (!folderPath || !iconPath) {
        return FALSE;
    }
    
    if (IsRootFolder(folderPath)) {
        /* Root folder: "DH0:" → "DH0:.info" */
        length = snprintf(iconPath, MAX_BACKUP_PATH, "%s.info", folderPath);
    } else {
        /* Normal folder: "DH0:Projects/MyFolder" → "DH0:Projects/MyFolder.info" */
        length = snprintf(iconPath, MAX_BACKUP_PATH, "%s.info", folderPath);
    }
    
    if (length >= MAX_BACKUP_PATH) {
        DEBUG_LOG("Drawer icon path too long: %d chars", length);
        return FALSE;
    }
    
    return TRUE;
}

/*========================================================================*/
/* Path Validation                                                       */
/*========================================================================*/

BOOL IsPathFfsSafe(const char *path) {
    const char *p;
    const char *componentStart;
    int componentLength;
    int totalLength;
    
    if (!path) {
        return FALSE;
    }
    
    totalLength = strlen(path);
    
    /* Check total path length (FFS limit is typically 107, we use 256 for safety) */
    if (totalLength >= MAX_BACKUP_PATH) {
        return FALSE;
    }
    
    /* Check each component for 30-character FFS limit */
    p = path;
    componentStart = path;
    
    /* Skip device name (before colon) */
    while (*p && *p != ':') {
        p++;
    }
    
    if (*p == ':') {
        p++; /* Move past colon */
        componentStart = p;
    }
    
    /* Check each path component */
    while (*p) {
        if (*p == '/' || *p == '\0') {
            componentLength = p - componentStart;
            
            /* FFS component limit is 30 characters */
            if (componentLength > 30) {
                DEBUG_LOG("Path component too long: %d chars", componentLength);
                return FALSE;
            }
            
            if (*p == '/') {
                componentStart = p + 1;
            }
        }
        p++;
    }
    
    /* Check final component */
    componentLength = p - componentStart;
    if (componentLength > 30) {
        DEBUG_LOG("Path component too long: %d chars (final)", componentLength);
        return FALSE;
    }
    
    return TRUE;
}

UWORD GetPathLength(const char *path) {
    if (!path) {
        return 0;
    }
    
    return (UWORD)strlen(path);
}

/*========================================================================*/
/* Path Component Extraction                                             */
/*========================================================================*/

BOOL GetDeviceName(const char *path, char *deviceName) {
    const char *colon;
    int length;
    
    if (!path || !deviceName) {
        if (deviceName) {
            deviceName[0] = '\0';
        }
        return FALSE;
    }
    
    /* Find the device separator */
    colon = strchr(path, ':');
    
    if (!colon) {
        /* No device separator */
        deviceName[0] = '\0';
        return FALSE;
    }
    
    /* Extract device name */
    length = colon - path;
    
    /* Ensure we don't overflow (device names are typically short: DH0, Work, etc.) */
    if (length >= 32) {
        length = 31;
    }
    
    strncpy(deviceName, path, length);
    deviceName[length] = '\0';
    
    return TRUE;
}
