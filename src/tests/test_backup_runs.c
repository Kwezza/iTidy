/**
 * test_backup_runs.c - Unit Test for Backup Run Management
 * 
 * Tests run number scanning, parsing, and directory creation.
 * 
 * Compile: gcc -DPLATFORM_HOST -I. -Isrc src/backup_runs.c src/backup_paths.c src/tests/test_backup_runs.c -o test_backup_runs
 * Run: ./test_backup_runs
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
    #define rmdir(path) _rmdir(path)
#else
    #include <unistd.h>
#endif

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

#include "../backup_runs.h"

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test directory root */
#define TEST_ROOT "./test_backup_runs_temp"

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

#define ASSERT_EQ(actual, expected, description) \
    do { \
        if ((actual) == (expected)) { \
            printf("  ✓ %s: %u\n", description, (unsigned int)(actual)); \
            tests_passed++; \
        } else { \
            printf("  ✗ FAIL: %s\n", description); \
            printf("    Expected: %u\n", (unsigned int)(expected)); \
            printf("    Got:      %u\n", (unsigned int)(actual)); \
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

/* Helper function to clean up test directories */
static void cleanup_test_dirs(void) {
    char path[256];
    int i;
    
    /* Remove test run directories */
    for (i = 1; i <= 10; i++) {
        snprintf(path, sizeof(path), "%s/Run_%04d", TEST_ROOT, i);
        rmdir(path);
    }
    
    /* Remove test root */
    rmdir(TEST_ROOT);
}

/* Helper function to create test run directories */
static void create_test_run(int runNum) {
    char path[256];
    snprintf(path, sizeof(path), "%s/Run_%04d", TEST_ROOT, runNum);
    mkdir(path, 0755);
}

void test_FormatRunDirectoryName(void) {
    char name[MAX_RUN_DIR_NAME];
    
    printf("\n=== Testing FormatRunDirectoryName ===\n");
    
    FormatRunDirectoryName(name, 1);
    ASSERT_STR_EQ(name, "Run_0001", "Run 1 formatted correctly");
    
    FormatRunDirectoryName(name, 42);
    ASSERT_STR_EQ(name, "Run_0042", "Run 42 formatted correctly");
    
    FormatRunDirectoryName(name, 123);
    ASSERT_STR_EQ(name, "Run_0123", "Run 123 formatted correctly");
    
    FormatRunDirectoryName(name, 9999);
    ASSERT_STR_EQ(name, "Run_9999", "Run 9999 formatted correctly");
}

void test_ParseRunNumber(void) {
    printf("\n=== Testing ParseRunNumber ===\n");
    
    ASSERT_EQ(ParseRunNumber("Run_0001"), 1, "Parse Run_0001");
    ASSERT_EQ(ParseRunNumber("Run_0042"), 42, "Parse Run_0042");
    ASSERT_EQ(ParseRunNumber("Run_0123"), 123, "Parse Run_0123");
    ASSERT_EQ(ParseRunNumber("Run_9999"), 9999, "Parse Run_9999");
    
    /* Invalid formats */
    ASSERT_EQ(ParseRunNumber("InvalidName"), 0, "Invalid name returns 0");
    ASSERT_EQ(ParseRunNumber("Run_ABCD"), 0, "Non-numeric returns 0");
    ASSERT_EQ(ParseRunNumber("Run_001"), 0, "Too few digits returns 0");
    ASSERT_EQ(ParseRunNumber("Run_00001"), 0, "Too many digits returns 0");
    ASSERT_EQ(ParseRunNumber("Run_0000"), 0, "Zero returns 0 (invalid)");
    ASSERT_EQ(ParseRunNumber(""), 0, "Empty string returns 0");
    ASSERT_EQ(ParseRunNumber(NULL), 0, "NULL returns 0");
}

void test_GetRunDirectoryPath(void) {
    char path[MAX_BACKUP_PATH];
    BOOL result;
    
    printf("\n=== Testing GetRunDirectoryPath ===\n");
    
    result = GetRunDirectoryPath(path, "PROGDIR:Backups", 1);
    ASSERT(result == TRUE, "Get path for run 1 succeeds");
    ASSERT_STR_EQ(path, "PROGDIR:Backups/Run_0001", "Run 1 path correct");
    
    result = GetRunDirectoryPath(path, "Work:iTidyBackups", 42);
    ASSERT(result == TRUE, "Get path for run 42 succeeds");
    ASSERT_STR_EQ(path, "Work:iTidyBackups/Run_0042", "Run 42 path correct");
    
    result = GetRunDirectoryPath(path, "DH0:Backups", 9999);
    ASSERT(result == TRUE, "Get path for run 9999 succeeds");
    ASSERT_STR_EQ(path, "DH0:Backups/Run_9999", "Run 9999 path correct");
    
    /* Invalid run numbers */
    result = GetRunDirectoryPath(path, "PROGDIR:Backups", 0);
    ASSERT(result == FALSE, "Run number 0 fails");
    
    result = GetRunDirectoryPath(path, "PROGDIR:Backups", 10000);
    ASSERT(result == FALSE, "Run number 10000 fails (too large)");
}

void test_BackupRootExists(void) {
    printf("\n=== Testing BackupRootExists ===\n");
    
    /* Clean up any existing test directory */
    cleanup_test_dirs();
    
    ASSERT(BackupRootExists(TEST_ROOT) == FALSE, "Non-existent root returns FALSE");
    
    /* Create test root */
    mkdir(TEST_ROOT, 0755);
    
    ASSERT(BackupRootExists(TEST_ROOT) == TRUE, "Existing root returns TRUE");
    
    /* Clean up */
    cleanup_test_dirs();
}

void test_CreateBackupRoot(void) {
    printf("\n=== Testing CreateBackupRoot ===\n");
    
    /* Clean up */
    cleanup_test_dirs();
    
    ASSERT(CreateBackupRoot(TEST_ROOT) == TRUE, "Create backup root succeeds");
    ASSERT(BackupRootExists(TEST_ROOT) == TRUE, "Root directory now exists");
    
    /* Should succeed if already exists */
    ASSERT(CreateBackupRoot(TEST_ROOT) == TRUE, "Create existing root succeeds");
    
    /* Clean up */
    cleanup_test_dirs();
}

void test_FindHighestRunNumber(void) {
    UWORD highest;
    
    printf("\n=== Testing FindHighestRunNumber ===\n");
    
    /* Clean up and create test root */
    cleanup_test_dirs();
    mkdir(TEST_ROOT, 0755);
    
    highest = FindHighestRunNumber(TEST_ROOT);
    ASSERT_EQ(highest, 0, "Empty directory returns 0");
    
    /* Create some run directories */
    create_test_run(1);
    highest = FindHighestRunNumber(TEST_ROOT);
    ASSERT_EQ(highest, 1, "Found Run_0001");
    
    create_test_run(2);
    highest = FindHighestRunNumber(TEST_ROOT);
    ASSERT_EQ(highest, 2, "Found Run_0002 (highest)");
    
    create_test_run(5);
    highest = FindHighestRunNumber(TEST_ROOT);
    ASSERT_EQ(highest, 5, "Found Run_0005 (highest, non-sequential)");
    
    /* Create out-of-order */
    create_test_run(3);
    highest = FindHighestRunNumber(TEST_ROOT);
    ASSERT_EQ(highest, 5, "Still returns 5 (highest overall)");
    
    /* Clean up */
    cleanup_test_dirs();
}

void test_CountRunDirectories(void) {
    UWORD count;
    
    printf("\n=== Testing CountRunDirectories ===\n");
    
    /* Clean up and create test root */
    cleanup_test_dirs();
    mkdir(TEST_ROOT, 0755);
    
    count = CountRunDirectories(TEST_ROOT);
    ASSERT_EQ(count, 0, "Empty directory has 0 runs");
    
    create_test_run(1);
    count = CountRunDirectories(TEST_ROOT);
    ASSERT_EQ(count, 1, "One run directory found");
    
    create_test_run(2);
    create_test_run(5);
    count = CountRunDirectories(TEST_ROOT);
    ASSERT_EQ(count, 3, "Three run directories found");
    
    /* Clean up */
    cleanup_test_dirs();
}

void test_CreateNextRunDirectory(void) {
    char runPath[MAX_BACKUP_PATH];
    UWORD runNumber;
    BOOL result;
    struct stat st;
    
    printf("\n=== Testing CreateNextRunDirectory ===\n");
    
    /* Clean up */
    cleanup_test_dirs();
    
    /* Create first run (no existing runs) */
    result = CreateNextRunDirectory(TEST_ROOT, runPath, &runNumber);
    ASSERT(result == TRUE, "Create first run succeeds");
    ASSERT_EQ(runNumber, 1, "First run number is 1");
    ASSERT(stat(runPath, &st) == 0, "Run directory was created");
    printf("    Created: %s\n", runPath);
    
    /* Create second run */
    result = CreateNextRunDirectory(TEST_ROOT, runPath, &runNumber);
    ASSERT(result == TRUE, "Create second run succeeds");
    ASSERT_EQ(runNumber, 2, "Second run number is 2");
    ASSERT(stat(runPath, &st) == 0, "Second run directory was created");
    printf("    Created: %s\n", runPath);
    
    /* Create third run */
    result = CreateNextRunDirectory(TEST_ROOT, runPath, &runNumber);
    ASSERT(result == TRUE, "Create third run succeeds");
    ASSERT_EQ(runNumber, 3, "Third run number is 3");
    printf("    Created: %s\n", runPath);
    
    /* Verify all directories exist */
    ASSERT_EQ(CountRunDirectories(TEST_ROOT), 3, "Three run directories exist");
    ASSERT_EQ(FindHighestRunNumber(TEST_ROOT), 3, "Highest run number is 3");
    
    /* Clean up */
    cleanup_test_dirs();
}

void test_SequentialCreation(void) {
    char runPath[MAX_BACKUP_PATH];
    UWORD runNumber;
    int i;
    
    printf("\n=== Testing Sequential Run Creation ===\n");
    
    /* Clean up */
    cleanup_test_dirs();
    
    /* Create 5 runs sequentially */
    for (i = 1; i <= 5; i++) {
        if (CreateNextRunDirectory(TEST_ROOT, runPath, &runNumber)) {
            ASSERT_EQ(runNumber, i, "Sequential run number correct");
        } else {
            printf("  ✗ Failed to create run %d\n", i);
            tests_failed++;
        }
    }
    
    ASSERT_EQ(CountRunDirectories(TEST_ROOT), 5, "Five runs created");
    ASSERT_EQ(FindHighestRunNumber(TEST_ROOT), 5, "Highest is 5");
    
    /* Clean up */
    cleanup_test_dirs();
}

int main(void) {
    printf("=================================================\n");
    printf("iTidy Backup Runs - Unit Test Suite\n");
    printf("=================================================\n");
    
    /* Run all tests */
    test_FormatRunDirectoryName();
    test_ParseRunNumber();
    test_GetRunDirectoryPath();
    test_BackupRootExists();
    test_CreateBackupRoot();
    test_FindHighestRunNumber();
    test_CountRunDirectories();
    test_CreateNextRunDirectory();
    test_SequentialCreation();
    
    /* Final cleanup */
    cleanup_test_dirs();
    
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
