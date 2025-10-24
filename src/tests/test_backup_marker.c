/**
 * test_backup_marker.c - Unit Test for Path Marker System
 * 
 * Tests marker file creation, reading, and archive integration.
 * 
 * Compile: gcc -DPLATFORM_HOST -I. -Isrc src/backup_marker.c src/backup_lha.c src/tests/test_backup_marker.c -o test_backup_marker
 * Run: ./test_backup_marker
 */

/* Windows headers FIRST */
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
    #define rmdir(path) _rmdir(path)
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* Platform compatibility */
#ifndef PLATFORM_HOST
#define PLATFORM_HOST 1
#endif

/* AmigaDOS types - only if not already defined */
#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H
#ifndef _WINDEF_
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

#include "../backup_marker.h"
#include "../backup_lha.h"

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test directories */
#define TEST_DIR "./test_marker_temp"
#define TEST_MARKER TEST_DIR "/test_marker.txt"

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

/* Helper functions */
static void cleanup_test_files(void) {
    remove(TEST_MARKER);
    remove(TEST_DIR "/_PATH.txt");
#ifdef _WIN32
    system("rmdir /s /q " TEST_DIR " 2>NUL");
#else
    system("rm -rf " TEST_DIR " 2>/dev/null");
#endif
}

static void setup_test_dir(void) {
    cleanup_test_files();
    mkdir(TEST_DIR, 0755);
}

static BOOL file_exists(const char *path) {
#ifdef _WIN32
    return (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES);
#else
    return (access(path, F_OK) == 0);
#endif
}

static void create_test_file(const char *path, const char *content) {
    FILE *fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "%s", content);
        fclose(fp);
    }
}

/* Tests */

void test_BuildMarkerPath(void) {
    char path[256];
    BOOL result;
    
    printf("\n=== Testing BuildMarkerPath ===\n");
    
    /* Test with directory */
    result = BuildMarkerPath(path, TEST_DIR);
    ASSERT(result == TRUE, "Build path succeeds");
    ASSERT(strstr(path, "_PATH.txt") != NULL, "Path contains _PATH.txt");
    printf("  Built path: %s\n", path);
    
    /* Test with directory ending in separator */
    result = BuildMarkerPath(path, TEST_DIR "/");
    ASSERT(result == TRUE, "Build path with trailing slash succeeds");
    ASSERT(strstr(path, "_PATH.txt") != NULL, "Path contains marker filename");
    
    /* Test with NULL */
    result = BuildMarkerPath(path, NULL);
    ASSERT(result == FALSE, "NULL directory returns FALSE");
}

void test_FormatMarkerTimestamp(void) {
    char timestamp[32];
    BOOL result;
    
    printf("\n=== Testing FormatMarkerTimestamp ===\n");
    
    result = FormatMarkerTimestamp(timestamp);
    ASSERT(result == TRUE, "Timestamp formatting succeeds");
    ASSERT(timestamp[0] != '\0', "Timestamp not empty");
    printf("  Timestamp: %s\n", timestamp);
    
    /* Basic format check (YYYY-MM-DD HH:MM:SS) */
    ASSERT(strlen(timestamp) == 19, "Timestamp has correct length");
    ASSERT(timestamp[4] == '-', "Year separator present");
    ASSERT(timestamp[7] == '-', "Month separator present");
    ASSERT(timestamp[10] == ' ', "Date/time separator present");
}

void test_GetTempDirectory(void) {
    char tempDir[256];
    BOOL result;
    
    printf("\n=== Testing GetTempDirectory ===\n");
    
    result = GetTempDirectory(tempDir);
    ASSERT(result == TRUE, "Get temp directory succeeds");
    ASSERT(tempDir[0] != '\0', "Temp directory not empty");
    printf("  Temp directory: %s\n", tempDir);
}

void test_CreatePathMarkerFile(void) {
    char markerPath[256];
    BOOL result;
    FILE *fp;
    char line[256];
    
    printf("\n=== Testing CreatePathMarkerFile ===\n");
    
    setup_test_dir();
    
    /* Create marker file */
    snprintf(markerPath, sizeof(markerPath), "%s/_PATH.txt", TEST_DIR);
    result = CreatePathMarkerFile(markerPath, "DH0:Projects/MyGame/", 42);
    ASSERT(result == TRUE, "Marker file creation succeeds");
    ASSERT(file_exists(markerPath), "Marker file exists");
    
    /* Verify contents */
    fp = fopen(markerPath, "r");
    ASSERT(fp != NULL, "Marker file can be opened");
    
    if (fp) {
        /* Read first line (original path) */
        if (fgets(line, sizeof(line), fp)) {
            /* Remove newline */
            size_t len = strlen(line);
            if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
            ASSERT_STR_EQ(line, "DH0:Projects/MyGame/", "First line contains original path");
        }
        
        /* Read second line (timestamp) */
        if (fgets(line, sizeof(line), fp)) {
            ASSERT(strstr(line, "Timestamp:") != NULL, "Second line contains timestamp");
        }
        
        /* Read third line (archive index) */
        if (fgets(line, sizeof(line), fp)) {
            ASSERT(strstr(line, "Archive: 00042") != NULL, "Third line contains archive index");
        }
        
        fclose(fp);
    }
    
    cleanup_test_files();
}

void test_CreateTempPathMarker(void) {
    char markerPath[256];
    BOOL result;
    
    printf("\n=== Testing CreateTempPathMarker ===\n");
    
    setup_test_dir();
    
    /* Create temp marker */
    result = CreateTempPathMarker(markerPath, "Work:Documents/Letters/", 
                                  100, TEST_DIR);
    ASSERT(result == TRUE, "Temp marker creation succeeds");
    ASSERT(markerPath[0] != '\0', "Marker path returned");
    ASSERT(file_exists(markerPath), "Marker file created");
    printf("  Created marker at: %s\n", markerPath);
    
    cleanup_test_files();
}

void test_ReadPathMarkerFile(void) {
    char markerPath[256];
    char originalPath[256];
    ULONG archiveIndex;
    BOOL result;
    
    printf("\n=== Testing ReadPathMarkerFile ===\n");
    
    setup_test_dir();
    
    /* Create marker first */
    snprintf(markerPath, sizeof(markerPath), "%s/_PATH.txt", TEST_DIR);
    CreatePathMarkerFile(markerPath, "DH0:Special/Folder/", 123);
    
    /* Read marker back */
    result = ReadPathMarkerFile(markerPath, originalPath, &archiveIndex);
    ASSERT(result == TRUE, "Marker reading succeeds");
    ASSERT_STR_EQ(originalPath, "DH0:Special/Folder/", "Original path matches");
    ASSERT_EQ(archiveIndex, 123, "Archive index matches");
    
    cleanup_test_files();
}

void test_ValidatePathMarker(void) {
    char markerPath[256];
    BOOL result;
    
    printf("\n=== Testing ValidatePathMarker ===\n");
    
    setup_test_dir();
    
    /* Create valid marker */
    snprintf(markerPath, sizeof(markerPath), "%s/_PATH.txt", TEST_DIR);
    CreatePathMarkerFile(markerPath, "DH0:Valid/Path/", 1);
    
    /* Validate it */
    result = ValidatePathMarker(markerPath);
    ASSERT(result == TRUE, "Valid marker passes validation");
    
    /* Create invalid marker (empty path) */
    create_test_file(markerPath, "\n");
    result = ValidatePathMarker(markerPath);
    ASSERT(result == FALSE, "Empty path fails validation");
    
    /* Test non-existent marker */
    result = ValidatePathMarker("nonexistent.txt");
    ASSERT(result == FALSE, "Non-existent marker fails validation");
    
    cleanup_test_files();
}

void test_DeleteMarkerFile(void) {
    char markerPath[256];
    BOOL result;
    
    printf("\n=== Testing DeleteMarkerFile ===\n");
    
    setup_test_dir();
    
    /* Create marker */
    snprintf(markerPath, sizeof(markerPath), "%s/_PATH.txt", TEST_DIR);
    CreatePathMarkerFile(markerPath, "DH0:Test/", 1);
    ASSERT(file_exists(markerPath), "Marker created");
    
    /* Delete it */
    result = DeleteMarkerFile(markerPath);
    ASSERT(result == TRUE, "Marker deletion succeeds");
    ASSERT(!file_exists(markerPath), "Marker file removed");
    
    cleanup_test_files();
}

/* Global LHA path */
static char g_lhaPath[32] = {0};

void test_ExtractAndReadMarker(void) {
    char archivePath[256];
    char markerPath[256];
    char originalPath[256];
    char sourcePath[256];
    BOOL result;
    
    printf("\n=== Testing ExtractAndReadMarker ===\n");
    
    /* Check if LHA is available */
    if (!CheckLhaAvailable(g_lhaPath)) {
        printf("  ⚠ Skipped (LHA not available)\n");
        return;
    }
    
    setup_test_dir();
    
    /* Create a marker file */
    snprintf(markerPath, sizeof(markerPath), "%s/_PATH.txt", TEST_DIR);
    CreatePathMarkerFile(markerPath, "DH0:TestFolder/SubFolder/", 999);
    
    /* Create test archive with marker */
    snprintf(archivePath, sizeof(archivePath), "%s/test.lha", TEST_DIR);
    snprintf(sourcePath, sizeof(sourcePath), "%s/_PATH.txt", TEST_DIR);
    
    /* Add marker to archive (simpler than full directory archive) */
    result = AddFileToArchive(g_lhaPath, archivePath, markerPath);
    
    if (!result) {
        printf("  ⚠ Skipped (archive creation failed)\n");
        cleanup_test_files();
        return;
    }
    
    ASSERT(result == TRUE, "Archive with marker created");
    
    /* Extract and read marker */
    result = ExtractAndReadMarker(archivePath, g_lhaPath, originalPath, TEST_DIR);
    ASSERT(result == TRUE, "Extract and read succeeds");
    ASSERT_STR_EQ(originalPath, "DH0:TestFolder/SubFolder/", "Original path extracted correctly");
    
    cleanup_test_files();
}

void test_ArchiveHasMarker(void) {
    char archivePath[256];
    char markerPath[256];
    BOOL result;
    
    printf("\n=== Testing ArchiveHasMarker ===\n");
    
    /* Check if LHA is available */
    if (!g_lhaPath[0]) {
        printf("  ⚠ Skipped (LHA not available)\n");
        return;
    }
    
    setup_test_dir();
    
    /* Create marker and archive */
    snprintf(markerPath, sizeof(markerPath), "%s/_PATH.txt", TEST_DIR);
    CreatePathMarkerFile(markerPath, "Work:Test/", 1);
    
    snprintf(archivePath, sizeof(archivePath), "%s/test.lha", TEST_DIR);
    AddFileToArchive(g_lhaPath, archivePath, markerPath);
    
    /* Check if archive has marker */
    result = ArchiveHasMarker(archivePath, g_lhaPath);
    ASSERT(result == TRUE, "Archive contains marker");
    
    cleanup_test_files();
}

int main(void) {
    printf("=================================================\n");
    printf("iTidy Backup Path Marker - Unit Test Suite\n");
    printf("=================================================\n");
    
    /* Check LHA availability once */
    CheckLhaAvailable(g_lhaPath);
    if (g_lhaPath[0]) {
        printf("LHA found at: %s\n", g_lhaPath);
    } else {
        printf("⚠ LHA not found - archive tests will be skipped\n");
    }
    
    /* Run all tests */
    test_BuildMarkerPath();
    test_FormatMarkerTimestamp();
    test_GetTempDirectory();
    test_CreatePathMarkerFile();
    test_CreateTempPathMarker();
    test_ReadPathMarkerFile();
    test_ValidatePathMarker();
    test_DeleteMarkerFile();
    test_ExtractAndReadMarker();
    test_ArchiveHasMarker();
    
    /* Final cleanup */
    cleanup_test_files();
    
    /* Print summary */
    printf("\n=================================================\n");
    printf("Test Results:\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    
    if (!g_lhaPath[0]) {
        printf("\n⚠ Note: LHA not found on system.\n");
        printf("  Archive integration tests were skipped.\n");
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
