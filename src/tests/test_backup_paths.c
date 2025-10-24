/**
 * test_backup_paths.c - Unit Test for Backup Path Utilities
 * 
 * Tests all path manipulation and validation functions.
 * 
 * Compile: gcc -DPLATFORM_HOST -I. -Isrc src/backup_paths.c src/tests/test_backup_paths.c -o test_backup_paths
 * Run: ./test_backup_paths
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Platform compatibility */
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

#include "../backup_paths.h"

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test assertion macro */
#define ASSERT(condition, description) \
    do { \
        if (condition) { \
            printf("  ✓ %s\n", description); \
            tests_passed++; \
        } else { \
            printf("  ✗ FAIL: %s\n", description); \
            tests_failed++; \
        } \
    } while(0)

#define ASSERT_STR_EQ(actual, expected, description) \
    do { \
        if (strcmp(actual, expected) == 0) { \
            printf("  ✓ %s: \"%s\"\n", description, actual); \
            tests_passed++; \
        } else { \
            printf("  ✗ FAIL: %s\n", description); \
            printf("    Expected: \"%s\"\n", expected); \
            printf("    Got:      \"%s\"\n", actual); \
            tests_failed++; \
        } \
    } while(0)

void test_IsRootFolder(void) {
    printf("\n=== Testing IsRootFolder ===\n");
    
    ASSERT(IsRootFolder("DH0:") == TRUE, "DH0: is root");
    ASSERT(IsRootFolder("Work:") == TRUE, "Work: is root");
    ASSERT(IsRootFolder("RAM:") == TRUE, "RAM: is root");
    ASSERT(IsRootFolder("DH0: ") == TRUE, "DH0: with trailing space is root");
    ASSERT(IsRootFolder("DH0:/") == TRUE, "DH0:/ is root");
    
    ASSERT(IsRootFolder("DH0:Projects") == FALSE, "DH0:Projects is not root");
    ASSERT(IsRootFolder("Work:Documents/Letters") == FALSE, "Work:Documents/Letters is not root");
    ASSERT(IsRootFolder("DH0:Folder") == FALSE, "DH0:Folder is not root");
    ASSERT(IsRootFolder("InvalidPath") == FALSE, "InvalidPath (no colon) is not root");
    ASSERT(IsRootFolder("") == FALSE, "Empty string is not root");
    ASSERT(IsRootFolder(NULL) == FALSE, "NULL is not root");
}

void test_CalculateArchivePath(void) {
    char path[MAX_BACKUP_PATH];
    BOOL result;
    
    printf("\n=== Testing CalculateArchivePath ===\n");
    
    result = CalculateArchivePath(path, "PROGDIR:Backups/Run_0001", 1);
    ASSERT(result == TRUE, "Archive 1 calculation succeeds");
    ASSERT_STR_EQ(path, "PROGDIR:Backups/Run_0001/000/00001.lha", "Archive 1 path correct");
    
    result = CalculateArchivePath(path, "PROGDIR:Backups/Run_0001", 42);
    ASSERT(result == TRUE, "Archive 42 calculation succeeds");
    ASSERT_STR_EQ(path, "PROGDIR:Backups/Run_0001/000/00042.lha", "Archive 42 path correct");
    
    result = CalculateArchivePath(path, "Work:iTidyBackups/Run_0042", 12345);
    ASSERT(result == TRUE, "Archive 12345 calculation succeeds");
    ASSERT_STR_EQ(path, "Work:iTidyBackups/Run_0042/123/12345.lha", "Archive 12345 path correct");
    
    result = CalculateArchivePath(path, "DH0:Backups/Run_0001", 99999);
    ASSERT(result == TRUE, "Archive 99999 calculation succeeds");
    ASSERT_STR_EQ(path, "DH0:Backups/Run_0001/999/99999.lha", "Archive 99999 path correct");
    
    result = CalculateArchivePath(path, "DH0:Backups/Run_0001", 0);
    ASSERT(result == FALSE, "Archive index 0 fails (invalid)");
    
    result = CalculateArchivePath(path, "DH0:Backups/Run_0001", 100000);
    ASSERT(result == FALSE, "Archive index 100000 fails (too large)");
}

void test_CalculateSubfolderPath(void) {
    char path[MAX_BACKUP_PATH];
    BOOL result;
    
    printf("\n=== Testing CalculateSubfolderPath ===\n");
    
    result = CalculateSubfolderPath(path, "PROGDIR:Backups/Run_0001", 1);
    ASSERT(result == TRUE, "Subfolder 1 calculation succeeds");
    ASSERT_STR_EQ(path, "PROGDIR:Backups/Run_0001/000", "Subfolder for archive 1 is 000");
    
    result = CalculateSubfolderPath(path, "PROGDIR:Backups/Run_0001", 99);
    ASSERT_STR_EQ(path, "PROGDIR:Backups/Run_0001/000", "Subfolder for archive 99 is 000");
    
    result = CalculateSubfolderPath(path, "PROGDIR:Backups/Run_0001", 100);
    ASSERT_STR_EQ(path, "PROGDIR:Backups/Run_0001/001", "Subfolder for archive 100 is 001");
    
    result = CalculateSubfolderPath(path, "Work:Backups/Run_0001", 12345);
    ASSERT_STR_EQ(path, "Work:Backups/Run_0001/123", "Subfolder for archive 12345 is 123");
}

void test_FormatArchiveName(void) {
    char name[MAX_ARCHIVE_NAME];
    
    printf("\n=== Testing FormatArchiveName ===\n");
    
    FormatArchiveName(name, 1);
    ASSERT_STR_EQ(name, "00001.lha", "Archive 1 formatted correctly");
    
    FormatArchiveName(name, 42);
    ASSERT_STR_EQ(name, "00042.lha", "Archive 42 formatted correctly");
    
    FormatArchiveName(name, 12345);
    ASSERT_STR_EQ(name, "12345.lha", "Archive 12345 formatted correctly");
    
    FormatArchiveName(name, 99999);
    ASSERT_STR_EQ(name, "99999.lha", "Archive 99999 formatted correctly");
}

void test_GetParentPath(void) {
    char parent[MAX_BACKUP_PATH];
    BOOL result;
    
    printf("\n=== Testing GetParentPath ===\n");
    
    result = GetParentPath("DH0:Projects/MyFolder", parent);
    ASSERT(result == TRUE, "Parent of DH0:Projects/MyFolder succeeds");
    ASSERT_STR_EQ(parent, "DH0:Projects", "Parent is DH0:Projects");
    
    result = GetParentPath("DH0:Projects/Client/Work", parent);
    ASSERT(result == TRUE, "Parent of DH0:Projects/Client/Work succeeds");
    ASSERT_STR_EQ(parent, "DH0:Projects/Client", "Parent is DH0:Projects/Client");
    
    result = GetParentPath("DH0:TopLevel", parent);
    ASSERT(result == TRUE, "Parent of DH0:TopLevel succeeds");
    ASSERT_STR_EQ(parent, "DH0:", "Parent is DH0: (root)");
    
    result = GetParentPath("DH0:", parent);
    ASSERT(result == FALSE, "Parent of DH0: fails (already at root)");
    
    result = GetParentPath("Work:Documents/Letters/Personal", parent);
    ASSERT(result == TRUE, "Parent of Work:Documents/Letters/Personal succeeds");
    ASSERT_STR_EQ(parent, "Work:Documents/Letters", "Parent is Work:Documents/Letters");
}

void test_GetFolderName(void) {
    char name[64];
    
    printf("\n=== Testing GetFolderName ===\n");
    
    GetFolderName("DH0:Projects/MyFolder", name);
    ASSERT_STR_EQ(name, "MyFolder", "Folder name is MyFolder");
    
    GetFolderName("Work:Documents/Letters/Personal", name);
    ASSERT_STR_EQ(name, "Personal", "Folder name is Personal");
    
    GetFolderName("DH0:TopLevel", name);
    ASSERT_STR_EQ(name, "TopLevel", "Folder name is TopLevel");
    
    GetFolderName("DH0:", name);
    ASSERT_STR_EQ(name, "", "Folder name for root is empty");
    
    GetFolderName("RAM:T", name);
    ASSERT_STR_EQ(name, "T", "Folder name is T");
}

void test_GetDrawerIconPath(void) {
    char iconPath[MAX_BACKUP_PATH];
    BOOL result;
    
    printf("\n=== Testing GetDrawerIconPath ===\n");
    
    result = GetDrawerIconPath("DH0:", iconPath);
    ASSERT(result == TRUE, "Drawer icon path for DH0: succeeds");
    ASSERT_STR_EQ(iconPath, "DH0:.info", "Root drawer icon is DH0:.info");
    
    result = GetDrawerIconPath("Work:", iconPath);
    ASSERT(result == TRUE, "Drawer icon path for Work: succeeds");
    ASSERT_STR_EQ(iconPath, "Work:.info", "Root drawer icon is Work:.info");
    
    result = GetDrawerIconPath("DH0:Projects/MyFolder", iconPath);
    ASSERT(result == TRUE, "Drawer icon path for DH0:Projects/MyFolder succeeds");
    ASSERT_STR_EQ(iconPath, "DH0:Projects/MyFolder.info", "Normal drawer icon is DH0:Projects/MyFolder.info");
    
    result = GetDrawerIconPath("Work:Documents/Letters", iconPath);
    ASSERT(result == TRUE, "Drawer icon path for Work:Documents/Letters succeeds");
    ASSERT_STR_EQ(iconPath, "Work:Documents/Letters.info", "Normal drawer icon is Work:Documents/Letters.info");
}

void test_GetDeviceName(void) {
    char device[32];
    BOOL result;
    
    printf("\n=== Testing GetDeviceName ===\n");
    
    result = GetDeviceName("DH0:Projects/MyFolder", device);
    ASSERT(result == TRUE, "Device extraction from DH0:Projects/MyFolder succeeds");
    ASSERT_STR_EQ(device, "DH0", "Device name is DH0");
    
    result = GetDeviceName("Work:Documents", device);
    ASSERT(result == TRUE, "Device extraction from Work:Documents succeeds");
    ASSERT_STR_EQ(device, "Work", "Device name is Work");
    
    result = GetDeviceName("RAM:", device);
    ASSERT(result == TRUE, "Device extraction from RAM: succeeds");
    ASSERT_STR_EQ(device, "RAM", "Device name is RAM");
    
    result = GetDeviceName("InvalidPath", device);
    ASSERT(result == FALSE, "Device extraction from InvalidPath fails (no colon)");
    ASSERT_STR_EQ(device, "", "Device name is empty");
}

void test_IsPathFfsSafe(void) {
    printf("\n=== Testing IsPathFfsSafe ===\n");
    
    ASSERT(IsPathFfsSafe("DH0:Projects/MyFolder") == TRUE, "Normal path is FFS-safe");
    ASSERT(IsPathFfsSafe("Work:Documents/Letters/Personal") == TRUE, "Multi-level path is FFS-safe");
    ASSERT(IsPathFfsSafe("DH0:") == TRUE, "Root path is FFS-safe");
    
    /* Test 30-character component limit */
    /* "ThisIsAVeryLongFolderNameThatIs35" is exactly 35 chars */
    ASSERT(IsPathFfsSafe("DH0:ThisIsAVeryLongFolderNameThatIs35") == FALSE, 
           "Path with 35-char component exceeds FFS limit");
    
    /* "ExactlyThirtyCharactersInName" is exactly 30 chars */
    ASSERT(IsPathFfsSafe("DH0:ExactlyThirtyCharactersInName") == TRUE,
           "Path with exactly 30-char component is FFS-safe");
    
    /* "ThisIsExactly31CharactersLong12" is 31 chars */
    ASSERT(IsPathFfsSafe("DH0:ThisIsExactly31CharactersLong12") == FALSE,
           "Path with 31-char component exceeds FFS limit");
    
    /* "TwentyNineCharsHereInTotal!" is 29 chars */
    ASSERT(IsPathFfsSafe("DH0:TwentyNineCharsHereInTotal!") == TRUE,
           "Path with 29-char component is FFS-safe");
}

int main(void) {
    printf("=================================================\n");
    printf("iTidy Backup Paths - Unit Test Suite\n");
    printf("=================================================\n");
    
    /* Run all tests */
    test_IsRootFolder();
    test_CalculateArchivePath();
    test_CalculateSubfolderPath();
    test_FormatArchiveName();
    test_GetParentPath();
    test_GetFolderName();
    test_GetDrawerIconPath();
    test_GetDeviceName();
    test_IsPathFfsSafe();
    
    /* Print summary */
    printf("\n=================================================\n");
    printf("Test Results:\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("=================================================\n");
    
    if (tests_failed == 0) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed.\n");
        return 1;
    }
}
