/**
 * backup_catalog.c - iTidy Backup System Catalog Management Implementation
 * 
 * Implements human-readable catalog.txt file management.
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

#include "backup_catalog.h"
#include "backup_paths.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

/* Platform-specific includes */
#ifdef PLATFORM_HOST
    #include <sys/stat.h>
    #define DEBUG_LOG printf
#else
    #include <dos/dos.h>
    #include <proto/dos.h>
    #include "writeLog.h"
    /* VBCC C99 mode has issues with variadic macros, just disable for now */
    #define DEBUG_LOG(...) /* disabled */
#endif

/*========================================================================*/
/* Constants                                                             */
/*========================================================================*/

#define CATALOG_VERSION "iTidy Backup Catalog v1.0"
#define CATALOG_SEPARATOR "========================================"
#define CATALOG_COLUMN_HEADER "# Index    | Subfolder | Size    | Original Path"
#define CATALOG_COLUMN_DIVIDER "-----------+-----------+---------+------------------"
#define MAX_LINE_LENGTH 512

/*========================================================================*/
/* Helper Functions                                                      */
/*========================================================================*/

/**
 * @brief Get current timestamp as string
 */
static void GetTimestampString(char *buffer, size_t bufSize) {
    time_t now;
    struct tm *timeinfo;
    
    time(&now);
    timeinfo = localtime(&now);
    
    if (timeinfo) {
        strftime(buffer, bufSize, "%Y-%m-%d %H:%M:%S", timeinfo);
    } else {
        snprintf(buffer, bufSize, "Unknown");
    }
}

/**
 * @brief Write a line to file (platform-specific)
 */
#ifdef PLATFORM_HOST
static BOOL WriteLineToFile(void *file, const char *line) {
    FILE *fp = (FILE *)file;
    if (!fp || !line) {
        return FALSE;
    }
    if (fprintf(fp, "%s\n", line) < 0) {
        return FALSE;
    }
    return TRUE;
}
#else
static BOOL WriteLineToFile(BPTR file, const char *line) {
    LONG len = strlen(line);
    if (Write(file, (APTR)line, len) != len) {
        return FALSE;
    }
    if (Write(file, (APTR)"\n", 1) != 1) {
        return FALSE;
    }
    return TRUE;
}
#endif

/*========================================================================*/
/* Size Formatting                                                       */
/*========================================================================*/

void FormatSizeForCatalog(char *outStr, ULONG sizeBytes) {
    if (!outStr) {
        return;
    }
    
    if (sizeBytes == 0) {
        strcpy(outStr, "N/A");
        return;
    }
    
    if (sizeBytes < 1024) {
        snprintf(outStr, 16, "%lu B", sizeBytes);
    } else if (sizeBytes < 1024 * 1024) {
        snprintf(outStr, 16, "%lu KB", sizeBytes / 1024);
    } else if (sizeBytes < 1024 * 1024 * 1024) {
        snprintf(outStr, 16, "%lu MB", sizeBytes / (1024 * 1024));
    } else {
        snprintf(outStr, 16, "%.1f GB", (float)sizeBytes / (1024.0f * 1024.0f * 1024.0f));
    }
}

/*========================================================================*/
/* Path Building                                                         */
/*========================================================================*/

BOOL GetCatalogPath(char *outPath, const char *runDirectory) {
    int length;
    
    if (!outPath || !runDirectory) {
        return FALSE;
    }
    
    length = snprintf(outPath, MAX_BACKUP_PATH, "%s/%s", 
                     runDirectory, CATALOG_FILENAME);
    
    if (length >= MAX_BACKUP_PATH) {
        DEBUG_LOG("Catalog path too long: %d chars", length);
        return FALSE;
    }
    
    return TRUE;
}

/*========================================================================*/
/* Catalog Creation and Writing                                          */
/*========================================================================*/

BOOL CreateCatalog(BackupContext *ctx) {
    char catalogPath[MAX_BACKUP_PATH];
    char timestamp[64];
    char line[256];
#ifndef PLATFORM_HOST
    BPTR file;  /* Only needed on Amiga */
#endif
    
    if (!ctx || !ctx->runDirectory[0]) {
        DEBUG_LOG("Invalid context for catalog creation");
        return FALSE;
    }
    
    /* Build catalog path */
    if (!GetCatalogPath(catalogPath, ctx->runDirectory)) {
        return FALSE;
    }
    
    DEBUG_LOG("Creating catalog: %s", catalogPath);
    
    /* Open catalog file for writing */
#ifdef PLATFORM_HOST
    {
        FILE *fp = fopen(catalogPath, "w");
        if (!fp) {
            DEBUG_LOG("Failed to create catalog file: %s", catalogPath);
            return FALSE;
        }
        ctx->catalogFile = (void *)fp;  /* Store FILE* directly as void* */
    }
#else
    {
        BPTR file = Open((STRPTR)catalogPath, MODE_NEWFILE);
        if (!file) {
            DEBUG_LOG("Failed to create catalog file: %s", catalogPath);
            return FALSE;
        }
        ctx->catalogFile = file;
    }
#endif
    ctx->catalogOpen = TRUE;
    
    /* Write header */
    GetTimestampString(timestamp, sizeof(timestamp));
    
    if (!WriteLineToFile(ctx->catalogFile, CATALOG_VERSION)) {
        return FALSE;
    }
    if (!WriteLineToFile(ctx->catalogFile, CATALOG_SEPARATOR)) {
        return FALSE;
    }
    
    snprintf(line, sizeof(line), "Run Number: %04u", ctx->runNumber);
    WriteLineToFile(ctx->catalogFile, line);
    
    snprintf(line, sizeof(line), "Session Started: %s", timestamp);
    WriteLineToFile(ctx->catalogFile, line);
    
    /* LhA version (if available) */
    if (ctx->lhaAvailable) {
        snprintf(line, sizeof(line), "LhA Path: %s", ctx->lhaPath);
        WriteLineToFile(ctx->catalogFile, line);
    } else {
        WriteLineToFile(ctx->catalogFile, "LhA: Not available");
    }
    
    WriteLineToFile(ctx->catalogFile, CATALOG_SEPARATOR);
    WriteLineToFile(ctx->catalogFile, "");
    WriteLineToFile(ctx->catalogFile, CATALOG_COLUMN_HEADER);
    WriteLineToFile(ctx->catalogFile, CATALOG_COLUMN_DIVIDER);
    
    DEBUG_LOG("Catalog header written");
    return TRUE;
}

BOOL AppendCatalogEntry(BackupContext *ctx, const BackupArchiveEntry *entry) {
    char line[MAX_LINE_LENGTH];
    char sizeStr[16];
    
    if (!ctx || !entry || !ctx->catalogFile || !ctx->catalogOpen) {
        DEBUG_LOG("Invalid parameters for catalog entry");
        return FALSE;
    }
    
    /* Format size */
    FormatSizeForCatalog(sizeStr, entry->sizeBytes);
    
    /* Build entry line: "00042.lha  | 000/      | 22 KB   | DH0:Projects/MyFolder/" */
    snprintf(line, sizeof(line), "%-10s | %-9s | %-7s | %s",
             entry->archiveName,
             entry->subFolder,
             sizeStr,
             entry->originalPath);
    
    if (!WriteLineToFile(ctx->catalogFile, line)) {
        DEBUG_LOG("Failed to write catalog entry");
        return FALSE;
    }
    
    return TRUE;
}

BOOL CloseCatalog(BackupContext *ctx) {
    char timestamp[64];
    char line[256];
    char totalSize[16];
    
    if (!ctx || !ctx->catalogFile || !ctx->catalogOpen) {
        return FALSE;
    }
    
    DEBUG_LOG("Closing catalog");
    
    /* Write footer */
    GetTimestampString(timestamp, sizeof(timestamp));
    FormatSizeForCatalog(totalSize, ctx->totalBytesArchived);
    
    WriteLineToFile(ctx->catalogFile, "");
    WriteLineToFile(ctx->catalogFile, CATALOG_SEPARATOR);
    
    snprintf(line, sizeof(line), "Session Ended: %s", timestamp);
    WriteLineToFile(ctx->catalogFile, line);
    
    snprintf(line, sizeof(line), "Total Archives: %hu", 
             (UWORD)(ctx->foldersBackedUp + ctx->failedBackups));
    WriteLineToFile(ctx->catalogFile, line);
    
    snprintf(line, sizeof(line), "Successful: %hu", ctx->foldersBackedUp);
    WriteLineToFile(ctx->catalogFile, line);
    
    snprintf(line, sizeof(line), "Failed: %u", ctx->failedBackups);
    WriteLineToFile(ctx->catalogFile, line);
    
    snprintf(line, sizeof(line), "Total Size: %s", totalSize);
    WriteLineToFile(ctx->catalogFile, line);
    
    WriteLineToFile(ctx->catalogFile, CATALOG_SEPARATOR);
    
    /* Close file */
#ifdef PLATFORM_HOST
    fclose((FILE *)ctx->catalogFile);
#else
    Close(ctx->catalogFile);
#endif
    
    ctx->catalogFile = 0;
    ctx->catalogOpen = FALSE;
    
    DEBUG_LOG("Catalog closed");
    return TRUE;
}

/*========================================================================*/
/* Catalog Parsing                                                       */
/*========================================================================*/

BOOL ParseCatalogLine(const char *line, BackupArchiveEntry *outEntry) {
    char tempLine[MAX_LINE_LENGTH];
    char *token;
    char *saveptr = NULL;
    int field = 0;
    
    if (!line || !outEntry) {
        return FALSE;
    }
    
    /* Skip empty lines and comments */
    if (line[0] == '\0' || line[0] == '#' || line[0] == '=' || line[0] == '-') {
        return FALSE;
    }
    
    /* Copy line for tokenization */
    strncpy(tempLine, line, sizeof(tempLine) - 1);
    tempLine[sizeof(tempLine) - 1] = '\0';
    
    /* Clear output entry */
    memset(outEntry, 0, sizeof(BackupArchiveEntry));
    
    /* Parse pipe-delimited fields */
#ifdef PLATFORM_HOST
    token = strtok(tempLine, "|");
#else
    token = strtok(tempLine, "|");
#endif
    
    while (token && field < 4) {
        /* Trim leading/trailing whitespace */
        while (*token == ' ' || *token == '\t') token++;
        
        char *end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
            *end = '\0';
            end--;
        }
        
        /* Parse field based on position */
        switch (field) {
            case 0: /* Archive name */
                strncpy(outEntry->archiveName, token, MAX_ARCHIVE_NAME - 1);
                /* Extract archive index from name (e.g., "00042.lha" -> 42) */
                sscanf(token, "%lu", &outEntry->archiveIndex);
                break;
                
            case 1: /* Subfolder */
                strncpy(outEntry->subFolder, token, MAX_FOLDER_NAME - 1);
                break;
                
            case 2: /* Size */
                /* Parse size string (e.g., "22 KB") */
                if (strcmp(token, "N/A") == 0) {
                    outEntry->sizeBytes = 0;
                    outEntry->successful = FALSE;
                } else {
                    ULONG value;
                    char unit[8];
                    if (sscanf(token, "%lu %s", &value, unit) == 2) {
                        if (strcmp(unit, "KB") == 0) {
                            outEntry->sizeBytes = value * 1024;
                        } else if (strcmp(unit, "MB") == 0) {
                            outEntry->sizeBytes = value * 1024 * 1024;
                        } else if (strcmp(unit, "GB") == 0) {
                            outEntry->sizeBytes = (ULONG)(value * 1024.0 * 1024.0 * 1024.0);
                        } else {
                            outEntry->sizeBytes = value; /* Assume bytes */
                        }
                        outEntry->successful = TRUE;
                    }
                }
                break;
                
            case 3: /* Original path */
                strncpy(outEntry->originalPath, token, MAX_BACKUP_PATH - 1);
                break;
        }
        
        field++;
#ifdef PLATFORM_HOST
        token = strtok(NULL, "|");
#else
        token = strtok(NULL, "|");
#endif
    }
    
    /* Valid entry must have all 4 fields */
    if (field == 4 && outEntry->archiveName[0] && outEntry->originalPath[0]) {
        return TRUE;
    }
    
    return FALSE;
}

BOOL ParseCatalog(const char *catalogPath, 
                  BOOL (*callback)(const BackupArchiveEntry *entry, void *userData),
                  void *userData) {
    char line[MAX_LINE_LENGTH];
    BackupArchiveEntry entry;
    BPTR file;
    BOOL result = TRUE;
    
    if (!catalogPath || !callback) {
        return FALSE;
    }
    
#ifdef PLATFORM_HOST
    FILE *fp = fopen(catalogPath, "r");
    if (!fp) {
        DEBUG_LOG("Failed to open catalog: %s", catalogPath);
        return FALSE;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        if (ParseCatalogLine(line, &entry)) {
            if (!callback(&entry, userData)) {
                result = FALSE; /* Callback requested stop */
                break;
            }
        }
    }
    
    fclose(fp);
#else
    file = Open((STRPTR)catalogPath, MODE_OLDFILE);
    if (!file) {
        DEBUG_LOG("Failed to open catalog: %s", catalogPath);
        return FALSE;
    }
    
    while (FGets(file, line, sizeof(line))) {
        if (ParseCatalogLine(line, &entry)) {
            if (!callback(&entry, userData)) {
                result = FALSE; /* Callback requested stop */
                break;
            }
        }
    }
    
    Close(file);
#endif
    
    return result;
}

/*========================================================================*/
/* Helper structures for ParseCatalog callbacks                          */
/*========================================================================*/

typedef struct {
    ULONG targetArchiveIndex;
    BackupArchiveEntry *outEntry;
    BOOL found;
} FindEntryContext;

typedef struct {
    UWORD count;
} CountEntryContext;

/*========================================================================*/
/* Callback functions (no longer nested)                                 */
/*========================================================================*/

static BOOL FindEntryCallback(const BackupArchiveEntry *entry, void *userData) {
    FindEntryContext *ctx = (FindEntryContext *)userData;
    if (entry->archiveIndex == ctx->targetArchiveIndex) {
        memcpy(ctx->outEntry, entry, sizeof(BackupArchiveEntry));
        ctx->found = TRUE;
        return FALSE; /* Stop parsing */
    }
    return TRUE; /* Continue */
}

static BOOL CountEntryCallback(const BackupArchiveEntry *entry, void *userData) {
    CountEntryContext *ctx = (CountEntryContext *)userData;
    ctx->count++;
    (void)entry; /* Unused but required by callback signature */
    return TRUE; /* Continue */
}

/*========================================================================*/
/* Public API implementations                                            */
/*========================================================================*/

BOOL FindCatalogEntry(const char *catalogPath, ULONG archiveIndex,
                      BackupArchiveEntry *outEntry) {
    FindEntryContext ctx;
    
    if (!catalogPath || !outEntry) {
        return FALSE;
    }
    
    ctx.targetArchiveIndex = archiveIndex;
    ctx.outEntry = outEntry;
    ctx.found = FALSE;
    
    ParseCatalog(catalogPath, FindEntryCallback, &ctx);
    
    return ctx.found;
}

UWORD CountCatalogEntries(const char *catalogPath) {
    CountEntryContext ctx;
    
    ctx.count = 0;
    
    ParseCatalog(catalogPath, CountEntryCallback, &ctx);
    
    return ctx.count;
}
