/**
 * test_backup_session.c - Test Suite for Backup Session Manager
 * 
 * Tests the high-level backup session API that integrates all
 * backup subsystems.
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #include <windows.h>
    #define mkdir(path, mode) _mkdir(path)
    #define rmdir(path) _rmdir(path)
    #define unlink(path) _unlink(path)
#else
    #include <unistd.h>
    #include <dirent.h>
#endif

#define PLATFORM_HOST
#include "backup_session.h"
#include "backup_paths.h"
#include "backup_runs.h"
#include "backup_catalog.h"
#include "backup_lha.h"

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Test macros */
#define TEST_START(name) \
    do { \
        tests_run++; \
        printf("TEST: %s ... ", name); \
        fflush(stdout); \
    } while(0)

#define TEST_PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        tests_failed++; \
        printf("FAIL: %s\n", msg); \
    } while(0)

#define ASSERT(condition, msg) \
    do { \
        if (!(condition)) { \
            TEST_FAIL(msg); \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual, msg) \
    do { \
        if ((expected) != (actual)) { \
            char buf[256]; \
            snprintf(buf, sizeof(buf), "%s (expected: %d, actual: %d)", \
                    msg, (int)(expected), (int)(actual)); \
            TEST_FAIL(buf); \
            return; \
        } \
    } while(0)

#define ASSERT_STR_EQ(expected, actual, msg) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            char buf[512]; \
            snprintf(buf, sizeof(buf), "%s (expected: \"%s\", actual: \"%s\")", \
                    msg, (expected), (actual)); \
            TEST_FAIL(buf); \
            return; \
        } \
    } while(0)

/* Test directories */
#define TEST_ROOT "./test_session"
#define TEST_BACKUPS "./test_session/backups"
#define TEST_SOURCE "./test_session/source"

/*========================================================================*/
/* Test Utilities                                                         */
/*========================================================================*/

static void cleanup_test_dirs(void) {
    char cmd[512];
    
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "rmdir /s /q \"%s\" 2>nul", TEST_ROOT);
#else
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\" 2>/dev/null", TEST_ROOT);
#endif
    system(cmd);
}

static void create_test_dirs(void) {
    cleanup_test_dirs();
    mkdir(TEST_ROOT, 0755);
    mkdir(TEST_BACKUPS, 0755);
    mkdir(TEST_SOURCE, 0755);
}

static void create_test_folder_with_icons(const char *folderPath, int numIcons) {
    char path[512];
    FILE *fp;
    int i;
    
    mkdir(folderPath, 0755);
    
    for (i = 0; i < numIcons; i++) {
        snprintf(path, sizeof(path), "%s/File%d.info", folderPath, i);
        fp = fopen(path, "w");
        if (fp) {
            fprintf(fp, "Fake icon data %d\n", i);
            fclose(fp);
        }
    }
}

static BOOL file_exists(const char *path) {
#ifdef _WIN32
    return _access(path, 0) == 0;
#else
    return access(path, F_OK) == 0;
#endif
}

static BOOL dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return FALSE;
    }
    return S_ISDIR(st.st_mode);
}

static int count_files_in_dir(const char *dirPath) {
    int count = 0;
    
#ifdef _WIN32
    WIN32_FIND_DATA findData;
    HANDLE hFind;
    char searchPath[512];
    
    snprintf(searchPath, sizeof(searchPath), "%s/*", dirPath);
    hFind = FindFirstFile(searchPath, &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") != 0 && 
                strcmp(findData.cFileName, "..") != 0) {
                count++;
            }
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR *dir = opendir(dirPath);
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

/*========================================================================*/
/* Session Lifecycle Tests                                               */
/*========================================================================*/

static void test_init_session_success(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    
    TEST_START("InitBackupSession with valid preferences");
    
    create_test_dirs();
    
    memset(&prefs, 0, sizeof(prefs));
    prefs.enableUndoBackup = TRUE;
    prefs.useLha = TRUE;
    strcpy(prefs.backupRootPath, TEST_BACKUPS);
    prefs.maxBackupsPerFolder = 100;
    
    ASSERT(InitBackupSession(&ctx, &prefs, NULL), "InitBackupSession failed");
    ASSERT_EQ(1, ctx.runNumber, "Run number should be 1");
    ASSERT_EQ(1, ctx.archiveIndex, "Archive index should start at 1");
    ASSERT(ctx.sessionActive, "Session should be active");
    ASSERT(ctx.catalogOpen, "Catalog should be open");
    ASSERT(ctx.lhaAvailable, "LHA should be available");
    ASSERT_EQ(0, ctx.foldersBackedUp, "No folders backed up yet");
    ASSERT_EQ(0, ctx.failedBackups, "No failed backups yet");
    
    /* Check run directory exists */
    ASSERT(dir_exists(ctx.runDirectory), "Run directory should exist");
    
    /* Check catalog file exists */
    char catalogPath[512];
    snprintf(catalogPath, sizeof(catalogPath), "%s/catalog.txt", ctx.runDirectory);
    ASSERT(file_exists(catalogPath), "Catalog file should exist");
    
    CloseBackupSession(&ctx);
    
    TEST_PASS();
}

static void test_init_session_invalid_params(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    
    TEST_START("InitBackupSession with NULL parameters");
    
    /* NULL context */
    ASSERT(!InitBackupSession(NULL, &prefs, NULL), "Should fail with NULL context");
    
    /* NULL preferences */
    ASSERT(!InitBackupSession(&ctx, NULL, NULL), "Should fail with NULL preferences");
    
    TEST_PASS();
}

static void test_init_session_backup_disabled(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    
    TEST_START("InitBackupSession with backup disabled");
    
    create_test_dirs();
    
    memset(&prefs, 0, sizeof(prefs));
    prefs.enableUndoBackup = FALSE;  /* Disabled */
    prefs.useLha = TRUE;
    strcpy(prefs.backupRootPath, TEST_BACKUPS);
    
    ASSERT(!InitBackupSession(&ctx, &prefs, NULL), "Should fail when backup disabled");
    
    TEST_PASS();
}

static void test_close_session(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    
    TEST_START("CloseBackupSession");
    
    create_test_dirs();
    
    memset(&prefs, 0, sizeof(prefs));
    prefs.enableUndoBackup = TRUE;
    prefs.useLha = TRUE;
    strcpy(prefs.backupRootPath, TEST_BACKUPS);
    
    ASSERT(InitBackupSession(&ctx, &prefs, NULL), "InitBackupSession failed");
    
    CloseBackupSession(&ctx);
    
    ASSERT(!ctx.sessionActive, "Session should be inactive");
    ASSERT(!ctx.catalogOpen, "Catalog should be closed");
    
    TEST_PASS();
}

/*========================================================================*/
/* Backup Folder Tests                                                   */
/*========================================================================*/

static void test_backup_single_folder(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    BackupStatus status;
    char testFolder[512];
    char archivePath[512];
    
    TEST_START("BackupFolder with single folder");
    
    create_test_dirs();
    
    /* Create test folder with icons */
    snprintf(testFolder, sizeof(testFolder), "%s/TestFolder", TEST_SOURCE);
    create_test_folder_with_icons(testFolder, 5);
    
    /* Initialize session */
    memset(&prefs, 0, sizeof(prefs));
    prefs.enableUndoBackup = TRUE;
    prefs.useLha = TRUE;
    strcpy(prefs.backupRootPath, TEST_BACKUPS);
    
    ASSERT(InitBackupSession(&ctx, &prefs, testFolder), "InitBackupSession failed");
    
    /* Backup folder */
    status = BackupFolder(&ctx, testFolder, 0);
    ASSERT_EQ(BACKUP_OK, status, "BackupFolder should succeed");
    ASSERT_EQ(2, ctx.archiveIndex, "Archive index should increment");
    ASSERT_EQ(1, ctx.foldersBackedUp, "Should have 1 folder backed up");
    ASSERT_EQ(0, ctx.failedBackups, "Should have no failures");
    
    /* Check archive exists */
    snprintf(archivePath, sizeof(archivePath), "%s/000/00001.lha", ctx.runDirectory);
    ASSERT(file_exists(archivePath), "Archive file should exist");
    
    CloseBackupSession(&ctx);
    
    TEST_PASS();
}

static void test_backup_multiple_folders(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    BackupStatus status;
    char folder1[512], folder2[512], folder3[512];
    
    TEST_START("BackupFolder with multiple folders");
    
    create_test_dirs();
    
    /* Create test folders with icons */
    snprintf(folder1, sizeof(folder1), "%s/Folder1", TEST_SOURCE);
    snprintf(folder2, sizeof(folder2), "%s/Folder2", TEST_SOURCE);
    snprintf(folder3, sizeof(folder3), "%s/Folder3", TEST_SOURCE);
    
    create_test_folder_with_icons(folder1, 3);
    create_test_folder_with_icons(folder2, 5);
    create_test_folder_with_icons(folder3, 2);
    
    /* Initialize session */
    memset(&prefs, 0, sizeof(prefs));
    prefs.enableUndoBackup = TRUE;
    prefs.useLha = TRUE;
    strcpy(prefs.backupRootPath, TEST_BACKUPS);
    
    ASSERT(InitBackupSession(&ctx, &prefs, NULL), "InitBackupSession failed");
    
    /* Backup all folders */
    status = BackupFolder(&ctx, folder1, 0);
    ASSERT_EQ(BACKUP_OK, status, "Backup folder1 should succeed");
    
    status = BackupFolder(&ctx, folder2, 0);
    ASSERT_EQ(BACKUP_OK, status, "Backup folder2 should succeed");
    
    status = BackupFolder(&ctx, folder3, 0);
    ASSERT_EQ(BACKUP_OK, status, "Backup folder3 should succeed");
    
    ASSERT_EQ(4, ctx.archiveIndex, "Archive index should be 4");
    ASSERT_EQ(3, ctx.foldersBackedUp, "Should have 3 folders backed up");
    ASSERT_EQ(0, ctx.failedBackups, "Should have no failures");
    
    CloseBackupSession(&ctx);
    
    TEST_PASS();
}

static void test_backup_empty_folder(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    BackupStatus status;
    char emptyFolder[512];
    
    TEST_START("BackupFolder with empty folder (no icons)");
    
    create_test_dirs();
    
    /* Create empty folder */
    snprintf(emptyFolder, sizeof(emptyFolder), "%s/EmptyFolder", TEST_SOURCE);
    mkdir(emptyFolder, 0755);
    
    /* Initialize session */
    memset(&prefs, 0, sizeof(prefs));
    prefs.enableUndoBackup = TRUE;
    prefs.useLha = TRUE;
    strcpy(prefs.backupRootPath, TEST_BACKUPS);
    
    ASSERT(InitBackupSession(&ctx, &prefs), "InitBackupSession failed");
    
    /* Backup empty folder - should be skipped */
    status = BackupFolder(&ctx, emptyFolder, 0);
    ASSERT_EQ(BACKUP_NO_ICONS, status, "Should return BACKUP_NO_ICONS");
    ASSERT_EQ(1, ctx.archiveIndex, "Archive index should not increment");
    ASSERT_EQ(0, ctx.foldersBackedUp, "Should have no folders backed up");
    
    CloseBackupSession(&ctx);
    
    TEST_PASS();
}

static void test_backup_invalid_params(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    BackupStatus status;
    
    TEST_START("BackupFolder with invalid parameters");
    
    create_test_dirs();
    
    memset(&prefs, 0, sizeof(prefs));
    prefs.enableUndoBackup = TRUE;
    prefs.useLha = TRUE;
    strcpy(prefs.backupRootPath, TEST_BACKUPS);
    
    ASSERT(InitBackupSession(&ctx, &prefs), "InitBackupSession failed");
    
    /* NULL context */
    status = BackupFolder(NULL, TEST_SOURCE, 0);
    ASSERT_EQ(BACKUP_INVALID_PARAMS, status, "Should fail with NULL context");
    
    /* NULL folder path */
    status = BackupFolder(&ctx, NULL, 0);
    ASSERT_EQ(BACKUP_INVALID_PARAMS, status, "Should fail with NULL path");
    
    CloseBackupSession(&ctx);
    
    TEST_PASS();
}

/*========================================================================*/
/* Utility Function Tests                                                */
/*========================================================================*/

static void test_folder_has_info_files_yes(void) {
    char testFolder[512];
    
    TEST_START("FolderHasInfoFiles with icons present");
    
    create_test_dirs();
    
    snprintf(testFolder, sizeof(testFolder), "%s/WithIcons", TEST_SOURCE);
    create_test_folder_with_icons(testFolder, 3);
    
    ASSERT(FolderHasInfoFiles(testFolder), "Should find .info files");
    
    TEST_PASS();
}

static void test_folder_has_info_files_no(void) {
    char testFolder[512];
    FILE *fp;
    char filePath[512];
    
    TEST_START("FolderHasInfoFiles with no icons");
    
    create_test_dirs();
    
    snprintf(testFolder, sizeof(testFolder), "%s/NoIcons", TEST_SOURCE);
    mkdir(testFolder, 0755);
    
    /* Create regular files (not .info) */
    snprintf(filePath, sizeof(filePath), "%s/readme.txt", testFolder);
    fp = fopen(filePath, "w");
    if (fp) {
        fprintf(fp, "test\n");
        fclose(fp);
    }
    
    ASSERT(!FolderHasInfoFiles(testFolder), "Should not find .info files");
    
    TEST_PASS();
}

static void test_get_drawer_icon_path_normal(void) {
    char drawerIconPath[256];
    
    TEST_START("GetDrawerIconPath for normal folder");
    
    ASSERT(GetDrawerIconPath("DH0:Projects/MyApp", drawerIconPath),
           "Should succeed");
    
    /* Expected: parent directory + folder name + .info */
    ASSERT_STR_EQ("DH0:Projects/MyApp.info", drawerIconPath, 
                  "Drawer icon path incorrect");
    
    TEST_PASS();
}

static void test_get_drawer_icon_path_root(void) {
    char drawerIconPath[256];
    
    TEST_START("GetDrawerIconPath for root folder");
    
    ASSERT(GetDrawerIconPath("DH0:", drawerIconPath),
           "Should succeed");
    
    /* Expected: root + .info */
    ASSERT_STR_EQ("DH0:.info", drawerIconPath, "Root drawer icon path incorrect");
    
    TEST_PASS();
}

/*========================================================================*/
/* Integration Tests                                                     */
/*========================================================================*/

static void test_full_session_workflow(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    BackupStatus status;
    char folder1[512], folder2[512];
    char catalogPath[512];
    FILE *fp;
    char line[512];
    int entryCount = 0;
    
    TEST_START("Full session workflow (init, backup, close)");
    
    create_test_dirs();
    
    /* Create test folders */
    snprintf(folder1, sizeof(folder1), "%s/App1", TEST_SOURCE);
    snprintf(folder2, sizeof(folder2), "%s/App2", TEST_SOURCE);
    create_test_folder_with_icons(folder1, 4);
    create_test_folder_with_icons(folder2, 6);
    
    /* Initialize session */
    memset(&prefs, 0, sizeof(prefs));
    prefs.enableUndoBackup = TRUE;
    prefs.useLha = TRUE;
    strcpy(prefs.backupRootPath, TEST_BACKUPS);
    
    ASSERT(InitBackupSession(&ctx, &prefs), "Session init failed");
    
    /* Backup folders */
    status = BackupFolder(&ctx, folder1, 0);
    ASSERT_EQ(BACKUP_OK, status, "Backup folder1 failed");
    
    status = BackupFolder(&ctx, folder2, 0);
    ASSERT_EQ(BACKUP_OK, status, "Backup folder2 failed");
    
    /* Close session */
    CloseBackupSession(&ctx);
    
    /* Verify catalog file has entries */
    snprintf(catalogPath, sizeof(catalogPath), "%s/catalog.txt", ctx.runDirectory);
    fp = fopen(catalogPath, "r");
    ASSERT(fp != NULL, "Catalog file should exist");
    
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, ".lha")) {
            entryCount++;
        }
    }
    fclose(fp);
    
    ASSERT_EQ(2, entryCount, "Catalog should have 2 entries");
    
    TEST_PASS();
}

/*========================================================================*/
/* Real Workbench Tests                                                  */
/*========================================================================*/

static void test_backup_real_workbench_prefs(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    BackupStatus status;
    const char *workbenchPrefs = "../../workbench/Prefs";
    char archivePath[512];
    
    TEST_START("Backup real Workbench/Prefs folder");
    
    /* Check if workbench folder exists */
    if (!dir_exists(workbenchPrefs)) {
        printf("SKIPPED (workbench/Prefs not found at %s)\n", workbenchPrefs);
        tests_run--; /* Don't count as run */
        return;
    }
    
    create_test_dirs();
    
    /* Initialize session */
    memset(&prefs, 0, sizeof(prefs));
    prefs.enableUndoBackup = TRUE;
    prefs.useLha = TRUE;
    strcpy(prefs.backupRootPath, TEST_BACKUPS);
    
    ASSERT(InitBackupSession(&ctx, &prefs), "Session init failed");
    
    /* Backup real Workbench Prefs folder */
    status = BackupFolder(&ctx, workbenchPrefs);
    ASSERT_EQ(BACKUP_OK, status, "Backup of Workbench/Prefs should succeed");
    ASSERT_EQ(1, ctx.foldersBackedUp, "Should have backed up 1 folder");
    ASSERT(ctx.totalBytesArchived > 0, "Archive should have non-zero size");
    
    /* Check archive exists */
    snprintf(archivePath, sizeof(archivePath), "%s/000/00001.lha", ctx.runDirectory);
    ASSERT(file_exists(archivePath), "Archive should exist");
    
    /* Report statistics */
    printf("\n");
    printf("  Archive size: %lu bytes\n", ctx.totalBytesArchived);
    printf("  Archive path: %s\n", archivePath);
    
    CloseBackupSession(&ctx);
    
    TEST_PASS();
}

static void test_backup_multiple_workbench_folders(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    BackupStatus status;
    const char *folders[] = {
        "../../workbench/Prefs",
        "../../workbench/Tools",
        "../../workbench/Utilities"
    };
    int i, validFolders = 0;
    
    TEST_START("Backup multiple real Workbench folders");
    
    /* Count valid folders */
    for (i = 0; i < 3; i++) {
        if (dir_exists(folders[i])) {
            validFolders++;
        }
    }
    
    if (validFolders == 0) {
        printf("SKIPPED (no workbench folders found)\n");
        tests_run--; /* Don't count as run */
        return;
    }
    
    create_test_dirs();
    
    /* Initialize session */
    memset(&prefs, 0, sizeof(prefs));
    prefs.enableUndoBackup = TRUE;
    prefs.useLha = TRUE;
    strcpy(prefs.backupRootPath, TEST_BACKUPS);
    
    ASSERT(InitBackupSession(&ctx, &prefs), "Session init failed");
    
    /* Backup all available folders */
    for (i = 0; i < 3; i++) {
        if (dir_exists(folders[i])) {
            status = BackupFolder(&ctx, folders[i]);
            if (status == BACKUP_OK) {
                printf("\n  ✓ Backed up: %s", folders[i]);
            }
        }
    }
    
    ASSERT(ctx.foldersBackedUp > 0, "Should have backed up at least one folder");
    ASSERT(ctx.totalBytesArchived > 0, "Total size should be non-zero");
    
    printf("\n");
    printf("  Total folders: %d\n", ctx.foldersBackedUp);
    printf("  Total size: %lu bytes\n", ctx.totalBytesArchived);
    
    CloseBackupSession(&ctx);
    
    TEST_PASS();
}

/*========================================================================*/
/* Test Runner                                                            */
/*========================================================================*/

int main(void) {
    printf("==============================================\n");
    printf("Backup Session Manager Test Suite\n");
    printf("==============================================\n\n");
    
    /* Session lifecycle tests */
    test_init_session_success();
    test_init_session_invalid_params();
    test_init_session_backup_disabled();
    test_close_session();
    
    /* Backup folder tests */
    test_backup_single_folder();
    test_backup_multiple_folders();
    test_backup_empty_folder();
    test_backup_invalid_params();
    
    /* Utility function tests */
    test_folder_has_info_files_yes();
    test_folder_has_info_files_no();
    test_get_drawer_icon_path_normal();
    test_get_drawer_icon_path_root();
    
    /* Integration tests */
    test_full_session_workflow();
    
    /* Real Workbench tests */
    printf("\n--- Real Workbench Tests ---\n");
    test_backup_real_workbench_prefs();
    test_backup_multiple_workbench_folders();
    
    /* Cleanup */
    cleanup_test_dirs();
    
    /* Report results */
    printf("\n==============================================\n");
    printf("Test Results:\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("==============================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
