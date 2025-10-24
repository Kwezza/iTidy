/**
 * test_backup_types.c - Unit Test for Backup Types
 * 
 * Verifies that backup_types.h compiles correctly and that
 * structure sizes are within acceptable limits.
 * 
 * Compile: gcc -I. -Iinclude test_backup_types.c -o test_backup_types
 * Run: ./test_backup_types
 */

#include <stdio.h>
#include <stdint.h>

/* Platform compatibility for host testing */
#ifndef PLATFORM_HOST
#define PLATFORM_HOST 1
#endif

/* AmigaDOS types for host compilation */
#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H
typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned long ULONG;
typedef signed char BYTE;
typedef signed short WORD;
typedef signed long LONG;
typedef int BOOL;
typedef long BPTR;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif
#endif

#include "backup_types.h"

int main(void) {
    printf("=================================================\n");
    printf("iTidy Backup Types - Structure Size Verification\n");
    printf("=================================================\n\n");
    
    /* Structure sizes */
    printf("Structure Sizes:\n");
    printf("  BackupContext:       %lu bytes\n", (unsigned long)sizeof(BackupContext));
    printf("  BackupArchiveEntry:  %lu bytes\n", (unsigned long)sizeof(BackupArchiveEntry));
    printf("  BackupStatus enum:   %lu bytes\n\n", (unsigned long)sizeof(BackupStatus));
    
    /* Constants */
    printf("Constants:\n");
    printf("  MAX_ARCHIVES_PER_RUN:    %u\n", MAX_ARCHIVES_PER_RUN);
    printf("  MAX_ARCHIVES_PER_FOLDER: %u\n", MAX_ARCHIVES_PER_FOLDER);
    printf("  MAX_RUN_NUMBER:          %u\n", MAX_RUN_NUMBER);
    printf("  MAX_BACKUP_PATH:         %u\n\n", MAX_BACKUP_PATH);
    
    /* Macro tests */
    printf("Macro Tests:\n");
    
    /* Test ARCHIVE_FOLDER_NUM macro */
    UWORD testIndex1 = 1;
    UWORD testIndex2 = 42;
    UWORD testIndex3 = 12345;
    
    printf("  ARCHIVE_FOLDER_NUM(%u) = %u (expected: 0)\n", 
           testIndex1, ARCHIVE_FOLDER_NUM(testIndex1));
    printf("  ARCHIVE_FOLDER_NUM(%u) = %u (expected: 0)\n", 
           testIndex2, ARCHIVE_FOLDER_NUM(testIndex2));
    printf("  ARCHIVE_FOLDER_NUM(%u) = %u (expected: 123)\n\n", 
           testIndex3, ARCHIVE_FOLDER_NUM(testIndex3));
    
    /* Test validation macros */
    printf("  ARCHIVE_INDEX_VALID(0):     %s (expected: false)\n", 
           ARCHIVE_INDEX_VALID(0) ? "true" : "false");
    printf("  ARCHIVE_INDEX_VALID(1):     %s (expected: true)\n", 
           ARCHIVE_INDEX_VALID(1) ? "true" : "false");
    printf("  ARCHIVE_INDEX_VALID(99999): %s (expected: true)\n", 
           ARCHIVE_INDEX_VALID(99999) ? "true" : "false");
    printf("  ARCHIVE_INDEX_VALID(100000):%s (expected: false)\n\n", 
           ARCHIVE_INDEX_VALID(100000) ? "true" : "false");
    
    printf("  RUN_NUMBER_VALID(0):    %s (expected: false)\n", 
           RUN_NUMBER_VALID(0) ? "true" : "false");
    printf("  RUN_NUMBER_VALID(1):    %s (expected: true)\n", 
           RUN_NUMBER_VALID(1) ? "true" : "false");
    printf("  RUN_NUMBER_VALID(9999): %s (expected: true)\n", 
           RUN_NUMBER_VALID(9999) ? "true" : "false");
    printf("  RUN_NUMBER_VALID(10000):%s (expected: false)\n\n", 
           RUN_NUMBER_VALID(10000) ? "true" : "false");
    
    /* Initialize a test context */
    BackupContext ctx = {0};
    ctx.sessionActive = 1;
    ctx.catalogOpen = 1;
    
    printf("  BACKUP_CONTEXT_VALID(&ctx): %s (expected: true)\n", 
           BACKUP_CONTEXT_VALID(&ctx) ? "true" : "false");
    
    ctx.sessionActive = 0;
    printf("  BACKUP_CONTEXT_VALID(&ctx): %s (expected: false, sessionActive=0)\n", 
           BACKUP_CONTEXT_VALID(&ctx) ? "true" : "false");
    
    printf("\n=================================================\n");
    
    /* Check if sizes are acceptable */
    if (sizeof(BackupContext) > 1024) {
        printf("WARNING: BackupContext is large (%lu bytes)\n", 
               (unsigned long)sizeof(BackupContext));
        printf("Consider heap allocation for production use.\n");
    }
    
    if (sizeof(BackupArchiveEntry) > 512) {
        printf("WARNING: BackupArchiveEntry is large (%lu bytes)\n", 
               (unsigned long)sizeof(BackupArchiveEntry));
    }
    
    printf("\n✓ All structure definitions compiled successfully!\n");
    printf("=================================================\n");
    
    return 0;
}
