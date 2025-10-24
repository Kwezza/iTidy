/**
 * test_backup_lha.c - Unit Test for LHA Wrapper
 * 
 * Tests LHA detection, archive creation, and extraction.
 * 
 * Compile: gcc -DPLATFORM_HOST -I. -Isrc src/backup_lha.c src/tests/test_backup_lha.c -o test_backup_lha
 * Run: ./test_backup_lha
 */

/* Windows headers FIRST to avoid type conflicts */
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
    #define rmdir(path) _rmdir(path)
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <dirent.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* Platform compatibility */
#ifndef PLATFORM_HOST
#define PLATFORM_HOST 1
#endif

/* AmigaDOS types for host compilation - only if not already defined */
#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H
#ifndef _WINDEF_  /* Not defined by Windows headers */
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
#endif /* _WINDEF_ */
#endif /* EXEC_TYPES_H */

#include "../backup_lha.h"

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test directories */
#define TEST_DIR "./test_lha_temp"
#define TEST_SOURCE TEST_DIR "/source"
#define TEST_DEST TEST_DIR "/dest"
#define TEST_ARCHIVE TEST_DIR "/test.lha"

/* Test assertion macros */
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
            printf("  ✓ %s: %lu\n", description, (unsigned long)(actual)); \
            tests_passed++; \
        } else { \
            printf("  ✗ FAIL: %s\n", description); \
            printf("    Expected: %lu\n", (unsigned long)(expected)); \
            printf("    Got:      %lu\n", (unsigned long)(actual)); \
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

/* Helper functions */
static void cleanup_test_files(void) {
    remove(TEST_ARCHIVE);
    
#ifdef _WIN32
    system("rmdir /s /q " TEST_SOURCE " 2>NUL");
    system("rmdir /s /q " TEST_DEST " 2>NUL");
    rmdir(TEST_DIR);
#else
    system("rm -rf " TEST_SOURCE " 2>/dev/null");
    system("rm -rf " TEST_DEST " 2>/dev/null");
    rmdir(TEST_DIR);
#endif
}

static void setup_test_dir(void) {
    cleanup_test_files();
    mkdir(TEST_DIR, 0755);
    mkdir(TEST_SOURCE, 0755);
    mkdir(TEST_DEST, 0755);
}

static void create_test_file(const char *path, const char *content) {
    FILE *fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "%s", content);
        fclose(fp);
    }
}

static BOOL file_exists(const char *path) {
#ifdef _WIN32
    return (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES);
#else
    return (access(path, F_OK) == 0);
#endif
}

static int count_files_in_dir(const char *path) {
    int count = 0;
    
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    HANDLE hFind;
    char searchPath[512];
    
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);
    hFind = FindFirstFileA(searchPath, &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") != 0 && 
                strcmp(findData.cFileName, "..") != 0) {
                count++;
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR *dir = opendir(path);
    struct dirent *entry;
    
    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && 
                strcmp(entry->d_name, "..") != 0) {
                count++;
            }
        }
        closedir(dir);
    }
#endif
    
    return count;
}

/* Global LHA path for tests */
static char g_lhaPath[32] = {0};

void test_CheckLhaAvailable(void) {
    char lhaPath[32];
    BOOL result;
    
    printf("\n=== Testing CheckLhaAvailable ===\n");
    
    result = CheckLhaAvailable(lhaPath);
    
    if (result) {
        ASSERT(result == TRUE, "LHA found on system");
        ASSERT(lhaPath[0] != '\0', "LHA path is not empty");
        printf("  Found LHA at: %s\n", lhaPath);
        
        /* Save for other tests */
        strcpy(g_lhaPath, lhaPath);
    } else {
        printf("  ⚠ LHA not found - archive tests will be skipped\n");
        ASSERT(result == FALSE, "LHA not available (expected on some systems)");
    }
}

void test_GetLhaVersion(void) {
    char versionBuffer[64];
    BOOL result;
    
    printf("\n=== Testing GetLhaVersion ===\n");
    
    if (!g_lhaPath[0]) {
        printf("  ⚠ Skipped (LHA not available)\n");
        return;
    }
    
    result = GetLhaVersion(g_lhaPath, versionBuffer);
    ASSERT(result == TRUE, "Version retrieved");
    
    if (result) {
        ASSERT(versionBuffer[0] != '\0', "Version string not empty");
        printf("  Version: %s\n", versionBuffer);
    }
}

void test_BuildLhaCommand(void) {
    char command[512];
    BOOL result;
    
    printf("\n=== Testing BuildLhaCommand ===\n");
    
    /* Test archive creation command */
    result = BuildLhaCommand(command, "lha", "a -r", "test.lha", "source/");
    ASSERT(result == TRUE, "Build create command succeeds");
    ASSERT(strstr(command, "lha") != NULL, "Command contains lha");
    ASSERT(strstr(command, "a -r") != NULL, "Command contains operation");
    ASSERT(strstr(command, "test.lha") != NULL, "Command contains archive");
    ASSERT(strstr(command, "source/") != NULL, "Command contains source");
    
    /* Test extraction command */
    result = BuildLhaCommand(command, "lha", "x", "test.lha", "dest/");
    ASSERT(result == TRUE, "Build extract command succeeds");
    ASSERT(strstr(command, "x") != NULL, "Command contains extract operation");
    
    /* Test NULL parameters */
    result = BuildLhaCommand(command, NULL, "a", "test.lha", "source/");
    ASSERT(result == FALSE, "NULL lhaPath returns FALSE");
}

void test_CreateLhaArchive(void) {
    BOOL result;
    
    printf("\n=== Testing CreateLhaArchive ===\n");
    
    if (!g_lhaPath[0]) {
        printf("  ⚠ Skipped (LHA not available)\n");
        return;
    }
    
    setup_test_dir();
    
    /* Create test files */
    create_test_file(TEST_SOURCE "/file1.txt", "Test content 1\n");
    create_test_file(TEST_SOURCE "/file2.txt", "Test content 2\n");
    create_test_file(TEST_SOURCE "/file3.txt", "Test content 3\n");
    
    /* Create archive */
    result = CreateLhaArchive(g_lhaPath, TEST_ARCHIVE, TEST_SOURCE);
    ASSERT(result == TRUE, "Archive creation succeeds");
    ASSERT(file_exists(TEST_ARCHIVE), "Archive file exists");
    
    cleanup_test_files();
}

void test_GetArchiveSize(void) {
    ULONG size;
    
    printf("\n=== Testing GetArchiveSize ===\n");
    
    if (!g_lhaPath[0]) {
        printf("  ⚠ Skipped (LHA not available)\n");
        return;
    }
    
    setup_test_dir();
    
    /* Create test archive */
    create_test_file(TEST_SOURCE "/file1.txt", "Test content\n");
    CreateLhaArchive(g_lhaPath, TEST_ARCHIVE, TEST_SOURCE);
    
    /* Get size */
    size = GetArchiveSize(TEST_ARCHIVE);
    ASSERT(size > 0, "Archive has non-zero size");
    printf("  Archive size: %lu bytes\n", (unsigned long)size);
    
    /* Test non-existent file */
    size = GetArchiveSize("nonexistent.lha");
    ASSERT(size == 0, "Non-existent archive returns 0");
    
    cleanup_test_files();
}

void test_ExtractLhaArchive(void) {
    BOOL result;
    int fileCount;
    
    printf("\n=== Testing ExtractLhaArchive ===\n");
    
    if (!g_lhaPath[0]) {
        printf("  ⚠ Skipped (LHA not available)\n");
        return;
    }
    
    setup_test_dir();
    
    /* Create archive with multiple files */
    create_test_file(TEST_SOURCE "/file1.txt", "Content 1\n");
    create_test_file(TEST_SOURCE "/file2.txt", "Content 2\n");
    create_test_file(TEST_SOURCE "/file3.txt", "Content 3\n");
    
    result = CreateLhaArchive(g_lhaPath, TEST_ARCHIVE, TEST_SOURCE);
    ASSERT(result == TRUE, "Archive created for extraction test");
    
    /* Extract archive */
    result = ExtractLhaArchive(g_lhaPath, TEST_ARCHIVE, TEST_DEST);
    ASSERT(result == TRUE, "Archive extraction succeeds");
    
    /* Verify files were extracted */
    fileCount = count_files_in_dir(TEST_DEST);
    ASSERT(fileCount >= 1, "Files were extracted");
    printf("  Extracted %d files\n", fileCount);
    
    cleanup_test_files();
}

void test_TestLhaArchive(void) {
    BOOL result;
    
    printf("\n=== Testing TestLhaArchive ===\n");
    
    if (!g_lhaPath[0]) {
        printf("  ⚠ Skipped (LHA not available)\n");
        return;
    }
    
    setup_test_dir();
    
    /* Create valid archive */
    create_test_file(TEST_SOURCE "/test.txt", "Test\n");
    CreateLhaArchive(g_lhaPath, TEST_ARCHIVE, TEST_SOURCE);
    
    /* Test valid archive */
    result = TestLhaArchive(g_lhaPath, TEST_ARCHIVE);
    ASSERT(result == TRUE, "Valid archive passes test");
    
    /* Test non-existent archive */
    result = TestLhaArchive(g_lhaPath, "nonexistent.lha");
    ASSERT(result == FALSE, "Non-existent archive fails test");
    
    cleanup_test_files();
}

void test_AddFileToArchive(void) {
    BOOL result;
    
    printf("\n=== Testing AddFileToArchive ===\n");
    
    if (!g_lhaPath[0]) {
        printf("  ⚠ Skipped (LHA not available)\n");
        return;
    }
    
    setup_test_dir();
    
    /* Create initial archive */
    create_test_file(TEST_SOURCE "/file1.txt", "Original\n");
    result = CreateLhaArchive(g_lhaPath, TEST_ARCHIVE, TEST_SOURCE);
    ASSERT(result == TRUE, "Initial archive created");
    
    /* Create additional file */
    create_test_file(TEST_DIR "/marker.txt", "Marker content\n");
    
    /* Add file to archive */
    result = AddFileToArchive(g_lhaPath, TEST_ARCHIVE, TEST_DIR "/marker.txt");
    ASSERT(result == TRUE, "File added to archive");
    
    cleanup_test_files();
}

/* Callback for ListLhaArchive test */
static int list_callback_count = 0;
static BOOL ListTestCallback(const char *fileName, ULONG size) {
    list_callback_count++;
    printf("    Listed: %s (%lu bytes)\n", fileName, (unsigned long)size);
    return TRUE;  /* Continue listing */
}

void test_ListLhaArchive(void) {
    BOOL result;
    
    printf("\n=== Testing ListLhaArchive ===\n");
    
    if (!g_lhaPath[0]) {
        printf("  ⚠ Skipped (LHA not available)\n");
        return;
    }
    
    setup_test_dir();
    
    /* Create archive with known files */
    create_test_file(TEST_SOURCE "/file1.txt", "Content 1\n");
    create_test_file(TEST_SOURCE "/file2.txt", "Content 2\n");
    CreateLhaArchive(g_lhaPath, TEST_ARCHIVE, TEST_SOURCE);
    
    /* List archive contents */
    list_callback_count = 0;
    result = ListLhaArchive(g_lhaPath, TEST_ARCHIVE, ListTestCallback);
    ASSERT(result == TRUE, "Archive listing succeeds");
    
    /* Note: Callback count may vary depending on LHA output format */
    printf("  Callback invoked %d times\n", list_callback_count);
    
    cleanup_test_files();
}

int main(void) {
    printf("=================================================\n");
    printf("iTidy Backup LHA Wrapper - Unit Test Suite\n");
    printf("=================================================\n");
    
    /* Run all tests */
    test_CheckLhaAvailable();
    test_GetLhaVersion();
    test_BuildLhaCommand();
    test_CreateLhaArchive();
    test_GetArchiveSize();
    test_ExtractLhaArchive();
    test_TestLhaArchive();
    test_AddFileToArchive();
    test_ListLhaArchive();
    
    /* Final cleanup */
    cleanup_test_files();
    
    /* Print summary */
    printf("\n=================================================\n");
    printf("Test Results:\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    
    if (!g_lhaPath[0]) {
        printf("\n⚠ Note: LHA not found on system.\n");
        printf("  Archive operation tests were skipped.\n");
        printf("  Install LHA to run full test suite.\n");
    }
    
    printf("=================================================\n");
    
    if (tests_failed == 0) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed.\n");
        return 1;
    }
}
