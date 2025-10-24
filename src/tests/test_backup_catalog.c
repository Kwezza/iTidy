/**
 * test_backup_catalog.c - Unit Test for Backup Catalog Management
 * 
 * Tests catalog file creation, writing, and parsing.
 * 
 * Compile: gcc -DPLATFORM_HOST -I. -Isrc src/backup_catalog.c src/backup_paths.c src/tests/test_backup_catalog.c -o test_backup_catalog
 * Run: ./test_backup_catalog
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>

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

#include "../backup_catalog.h"

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test directory */
#define TEST_DIR "./test_catalog_temp"
#define TEST_CATALOG TEST_DIR "/catalog.txt"

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
    remove(TEST_CATALOG);
    rmdir(TEST_DIR);
}

static void setup_test_dir(void) {
    cleanup_test_files();
    mkdir(TEST_DIR, 0755);
}

void test_FormatSizeForCatalog(void) {
    char sizeStr[16];
    
    printf("\n=== Testing FormatSizeForCatalog ===\n");
    
    FormatSizeForCatalog(sizeStr, 0);
    ASSERT_STR_EQ(sizeStr, "N/A", "Zero bytes formats as N/A");
    
    FormatSizeForCatalog(sizeStr, 512);
    ASSERT_STR_EQ(sizeStr, "512 B", "512 bytes formats correctly");
    
    FormatSizeForCatalog(sizeStr, 1024);
    ASSERT_STR_EQ(sizeStr, "1 KB", "1 KB formats correctly");
    
    FormatSizeForCatalog(sizeStr, 15360);
    ASSERT_STR_EQ(sizeStr, "15 KB", "15 KB formats correctly");
    
    FormatSizeForCatalog(sizeStr, 1024 * 1024);
    ASSERT_STR_EQ(sizeStr, "1 MB", "1 MB formats correctly");
    
    FormatSizeForCatalog(sizeStr, 50 * 1024 * 1024);
    ASSERT_STR_EQ(sizeStr, "50 MB", "50 MB formats correctly");
}

void test_GetCatalogPath(void) {
    char path[MAX_BACKUP_PATH];
    BOOL result;
    
    printf("\n=== Testing GetCatalogPath ===\n");
    
    result = GetCatalogPath(path, "PROGDIR:Backups/Run_0001");
    ASSERT(result == TRUE, "Get catalog path succeeds");
    ASSERT_STR_EQ(path, "PROGDIR:Backups/Run_0001/catalog.txt", "Catalog path correct");
    
    result = GetCatalogPath(path, "Work:Backups/Run_0042");
    ASSERT(result == TRUE, "Get catalog path for run 42 succeeds");
    ASSERT_STR_EQ(path, "Work:Backups/Run_0042/catalog.txt", "Run 42 catalog path correct");
}

void test_CreateAndCloseCatalog(void) {
    BackupContext ctx;
    FILE *fp;
    char line[256];
    BOOL foundVersion = FALSE;
    BOOL foundHeader = FALSE;
    
    printf("\n=== Testing CreateCatalog and CloseCatalog ===\n");
    
    setup_test_dir();
    
    /* Initialize context */
    memset(&ctx, 0, sizeof(BackupContext));
    strcpy(ctx.runDirectory, TEST_DIR);
    ctx.runNumber = 1;
    ctx.lhaAvailable = TRUE;
    strcpy(ctx.lhaPath, "lha");
    ctx.foldersBackedUp = 0;
    ctx.failedBackups = 0;
    ctx.totalBytesArchived = 0;
    
    /* Create catalog */
    ASSERT(CreateCatalog(&ctx) == TRUE, "CreateCatalog succeeds");
    ASSERT(ctx.catalogFile != 0, "Catalog file handle set");
    ASSERT(ctx.catalogOpen == TRUE, "Catalog marked as open");
    
    /* Close catalog */
    ASSERT(CloseCatalog(&ctx) == TRUE, "CloseCatalog succeeds");
    ASSERT(ctx.catalogFile == 0, "Catalog file handle cleared");
    ASSERT(ctx.catalogOpen == FALSE, "Catalog marked as closed");
    
    /* Verify file exists and contains header/footer */
    fp = fopen(TEST_CATALOG, "r");
    ASSERT(fp != NULL, "Catalog file exists");
    
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "iTidy Backup Catalog")) {
                foundVersion = TRUE;
            }
            if (strstr(line, "# Index")) {
                foundHeader = TRUE;
            }
        }
        fclose(fp);
    }
    
    ASSERT(foundVersion == TRUE, "Catalog contains version header");
    ASSERT(foundHeader == TRUE, "Catalog contains column header");
    
    cleanup_test_files();
}

void test_AppendCatalogEntry(void) {
    BackupContext ctx;
    BackupArchiveEntry entry1, entry2, entry3;
    FILE *fp;
    char line[512];
    int entryCount = 0;
    
    printf("\n=== Testing AppendCatalogEntry ===\n");
    
    setup_test_dir();
    
    /* Initialize context */
    memset(&ctx, 0, sizeof(BackupContext));
    strcpy(ctx.runDirectory, TEST_DIR);
    ctx.runNumber = 1;
    ctx.lhaAvailable = TRUE;
    strcpy(ctx.lhaPath, "lha");
    
    /* Create catalog */
    ASSERT(CreateCatalog(&ctx) == TRUE, "Catalog created");
    
    /* Prepare first entry */
    memset(&entry1, 0, sizeof(BackupArchiveEntry));
    entry1.archiveIndex = 1;
    strcpy(entry1.archiveName, "00001.lha");
    strcpy(entry1.subFolder, "000/");
    entry1.sizeBytes = 15360; /* 15 KB */
    strcpy(entry1.originalPath, "DH0:Projects/MyFolder");
    entry1.successful = TRUE;
    
    /* Append first entry */
    ASSERT(AppendCatalogEntry(&ctx, &entry1) == TRUE, "First entry appended");
    
    /* Prepare second entry */
    memset(&entry2, 0, sizeof(BackupArchiveEntry));
    entry2.archiveIndex = 42;
    strcpy(entry2.archiveName, "00042.lha");
    strcpy(entry2.subFolder, "000/");
    entry2.sizeBytes = 22528; /* 22 KB */
    strcpy(entry2.originalPath, "Work:Documents/Letters");
    entry2.successful = TRUE;
    
    /* Append second entry */
    ASSERT(AppendCatalogEntry(&ctx, &entry2) == TRUE, "Second entry appended");
    
    /* Prepare failed entry */
    memset(&entry3, 0, sizeof(BackupArchiveEntry));
    entry3.archiveIndex = 100;
    strcpy(entry3.archiveName, "00100.lha");
    strcpy(entry3.subFolder, "001/");
    entry3.sizeBytes = 0; /* Failed backup */
    strcpy(entry3.originalPath, "DH0:FailedFolder");
    entry3.successful = FALSE;
    
    /* Append failed entry */
    ASSERT(AppendCatalogEntry(&ctx, &entry3) == TRUE, "Failed entry appended");
    
    /* Close catalog */
    CloseCatalog(&ctx);
    
    /* Verify entries were written */
    fp = fopen(TEST_CATALOG, "r");
    ASSERT(fp != NULL, "Catalog file exists");
    
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "00001.lha")) entryCount++;
            if (strstr(line, "00042.lha")) entryCount++;
            if (strstr(line, "00100.lha")) entryCount++;
        }
        fclose(fp);
    }
    
    ASSERT_EQ(entryCount, 3, "All three entries found in catalog");
    
    cleanup_test_files();
}

void test_ParseCatalogLine(void) {
    BackupArchiveEntry entry;
    BOOL result;
    
    printf("\n=== Testing ParseCatalogLine ===\n");
    
    /* Test valid line */
    result = ParseCatalogLine("00042.lha  | 000/      | 22 KB   | DH0:Projects/MyFolder", &entry);
    ASSERT(result == TRUE, "Valid line parses successfully");
    ASSERT_STR_EQ(entry.archiveName, "00042.lha", "Archive name parsed");
    ASSERT_EQ(entry.archiveIndex, 42, "Archive index extracted");
    ASSERT_STR_EQ(entry.subFolder, "000/", "Subfolder parsed");
    ASSERT_EQ(entry.sizeBytes, 22528, "Size parsed (22 KB = 22528 bytes)");
    ASSERT_STR_EQ(entry.originalPath, "DH0:Projects/MyFolder", "Original path parsed");
    ASSERT(entry.successful == TRUE, "Marked as successful");
    
    /* Test failed backup line */
    result = ParseCatalogLine("00100.lha  | 001/      | N/A     | DH0:FailedFolder", &entry);
    ASSERT(result == TRUE, "Failed backup line parses");
    ASSERT_EQ(entry.sizeBytes, 0, "Failed backup has 0 size");
    ASSERT(entry.successful == FALSE, "Marked as failed");
    
    /* Test invalid lines */
    result = ParseCatalogLine("", &entry);
    ASSERT(result == FALSE, "Empty line returns FALSE");
    
    result = ParseCatalogLine("# This is a comment", &entry);
    ASSERT(result == FALSE, "Comment line returns FALSE");
    
    result = ParseCatalogLine("========================================", &entry);
    ASSERT(result == FALSE, "Separator line returns FALSE");
}

static int parseCallbackCount = 0;
static BOOL TestParseCallback(const BackupArchiveEntry *entry) {
    parseCallbackCount++;
    printf("    Parsed entry %d: %s -> %s\n", 
           parseCallbackCount, entry->archiveName, entry->originalPath);
    return TRUE; /* Continue parsing */
}

void test_ParseCatalog(void) {
    BackupContext ctx;
    BackupArchiveEntry entry1, entry2, entry3;
    UWORD count;
    
    printf("\n=== Testing ParseCatalog ===\n");
    
    setup_test_dir();
    
    /* Create a test catalog with entries */
    memset(&ctx, 0, sizeof(BackupContext));
    strcpy(ctx.runDirectory, TEST_DIR);
    ctx.runNumber = 1;
    ctx.lhaAvailable = TRUE;
    
    CreateCatalog(&ctx);
    
    /* Add test entries */
    memset(&entry1, 0, sizeof(BackupArchiveEntry));
    entry1.archiveIndex = 1;
    strcpy(entry1.archiveName, "00001.lha");
    strcpy(entry1.subFolder, "000/");
    entry1.sizeBytes = 15360;
    strcpy(entry1.originalPath, "DH0:Projects/Folder1");
    AppendCatalogEntry(&ctx, &entry1);
    
    memset(&entry2, 0, sizeof(BackupArchiveEntry));
    entry2.archiveIndex = 2;
    strcpy(entry2.archiveName, "00002.lha");
    strcpy(entry2.subFolder, "000/");
    entry2.sizeBytes = 20480;
    strcpy(entry2.originalPath, "DH0:Projects/Folder2");
    AppendCatalogEntry(&ctx, &entry2);
    
    memset(&entry3, 0, sizeof(BackupArchiveEntry));
    entry3.archiveIndex = 3;
    strcpy(entry3.archiveName, "00003.lha");
    strcpy(entry3.subFolder, "000/");
    entry3.sizeBytes = 10240;
    strcpy(entry3.originalPath, "DH0:Projects/Folder3");
    AppendCatalogEntry(&ctx, &entry3);
    
    CloseCatalog(&ctx);
    
    /* Parse catalog */
    parseCallbackCount = 0;
    ASSERT(ParseCatalog(TEST_CATALOG, TestParseCallback) == TRUE, "ParseCatalog succeeds");
    ASSERT_EQ(parseCallbackCount, 3, "Three entries parsed");
    
    /* Count entries */
    count = CountCatalogEntries(TEST_CATALOG);
    ASSERT_EQ(count, 3, "CountCatalogEntries returns 3");
    
    cleanup_test_files();
}

void test_FindCatalogEntry(void) {
    BackupContext ctx;
    BackupArchiveEntry entry, found;
    BOOL result;
    
    printf("\n=== Testing FindCatalogEntry ===\n");
    
    setup_test_dir();
    
    /* Create catalog with entries */
    memset(&ctx, 0, sizeof(BackupContext));
    strcpy(ctx.runDirectory, TEST_DIR);
    ctx.runNumber = 1;
    ctx.lhaAvailable = TRUE;
    
    CreateCatalog(&ctx);
    
    /* Add entry with index 42 */
    memset(&entry, 0, sizeof(BackupArchiveEntry));
    entry.archiveIndex = 42;
    strcpy(entry.archiveName, "00042.lha");
    strcpy(entry.subFolder, "000/");
    entry.sizeBytes = 22528;
    strcpy(entry.originalPath, "DH0:Special/Folder42");
    AppendCatalogEntry(&ctx, &entry);
    
    /* Add another entry */
    entry.archiveIndex = 100;
    strcpy(entry.archiveName, "00100.lha");
    strcpy(entry.subFolder, "001/");
    entry.sizeBytes = 30720;
    strcpy(entry.originalPath, "Work:Another/Folder");
    AppendCatalogEntry(&ctx, &entry);
    
    CloseCatalog(&ctx);
    
    /* Find entry 42 */
    result = FindCatalogEntry(TEST_CATALOG, 42, &found);
    ASSERT(result == TRUE, "FindCatalogEntry finds entry 42");
    ASSERT_EQ(found.archiveIndex, 42, "Found entry has correct index");
    ASSERT_STR_EQ(found.originalPath, "DH0:Special/Folder42", "Found entry has correct path");
    
    /* Find entry 100 */
    result = FindCatalogEntry(TEST_CATALOG, 100, &found);
    ASSERT(result == TRUE, "FindCatalogEntry finds entry 100");
    ASSERT_EQ(found.archiveIndex, 100, "Found entry has correct index");
    
    /* Try to find non-existent entry */
    result = FindCatalogEntry(TEST_CATALOG, 999, &found);
    ASSERT(result == FALSE, "Non-existent entry returns FALSE");
    
    cleanup_test_files();
}

int main(void) {
    printf("=================================================\n");
    printf("iTidy Backup Catalog - Unit Test Suite\n");
    printf("=================================================\n");
    
    /* Run all tests */
    test_FormatSizeForCatalog();
    test_GetCatalogPath();
    test_CreateAndCloseCatalog();
    test_AppendCatalogEntry();
    test_ParseCatalogLine();
    test_ParseCatalog();
    test_FindCatalogEntry();
    
    /* Final cleanup */
    cleanup_test_files();
    
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
