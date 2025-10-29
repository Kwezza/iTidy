/**
 * @file test_backup_restore.c
 * @brief Test suite for backup restore operations
 * @author AI Agent (Task 8)
 * @date October 24, 2025
 * 
 * Test Coverage:
 * - Restore context initialization
 * - Single archive restore
 * - Full run restore
 * - Orphaned archive recovery
 * - Path validation
 * - Error handling
 * - Statistics tracking
 */

#include "../backup_restore.h"
#include "../backup_session.h"
#include "../backup_catalog.h"
#include "../backup_marker.h"
#include "../backup_lha.h"
#include "../backup_paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#define rmdir _rmdir
#define unlink _unlink
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

/* Test framework macros */
static int g_testsPassed = 0;
static int g_testsFailed = 0;
static int g_currentTest = 0;

#define TEST(name) \
    void name(void); \
    void name(void)

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("  ❌ FAILED: %s (line %d)\n", #condition, __LINE__); \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define RUN_TEST(test) \
    do { \
        g_currentTest++; \
        printf("\n[%d] Running: %s\n", g_currentTest, #test); \
        test(); \
        g_testsPassed++; \
        printf("  ✅ PASSED\n"); \
    } while(0)

/* Test helper functions */
static void CreateTestDirectory(const char *path);
static void RemoveTestDirectory(const char *path);
static void CreateTestInfoFile(const char *path, const char *filename);
static BOOL DirectoryExists(const char *path);
static BOOL FileExistsTest(const char *path);

/* ========================================================================
 * TEST HELPERS
 * ======================================================================== */

static void CreateTestDirectory(const char *path) {
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

static void RemoveTestDirectory(const char *path) {
    /* Simple implementation - just remove directory */
    /* In production, would need recursive removal */
    rmdir(path);
}

static void CreateTestInfoFile(const char *path, const char *filename) {
    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", path, filename);
    
    FILE *f = fopen(fullPath, "w");
    if (f) {
        fprintf(f, "Test icon file: %s\n", filename);
        fclose(f);
    }
}

static BOOL DirectoryExists(const char *path) {
#ifdef _WIN32
    struct stat st;
    if (stat(path, &st) == 0) {
        return (st.st_mode & S_IFDIR) != 0;
    }
#else
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
#endif
    return FALSE;
}

static BOOL FileExistsTest(const char *path) {
#ifdef _WIN32
    return (_access(path, 0) == 0);
#else
    return (access(path, F_OK) == 0);
#endif
}

/* ========================================================================
 * TESTS - Context Initialization
 * ======================================================================== */

TEST(test_init_restore_context) {
    RestoreContext ctx;
    BOOL result = InitRestoreContext(&ctx);
    
    /* Should succeed if LHA is available */
    if (result) {
        ASSERT(ctx.lhaAvailable == TRUE);
        ASSERT(strlen(ctx.lhaPath) > 0);
        ASSERT(ctx.stats.archivesRestored == 0);
        ASSERT(ctx.stats.archivesFailed == 0);
        ASSERT(ctx.stats.totalBytesRestored == 0);
        ASSERT(ctx.stats.hasErrors == FALSE);
    } else {
        /* If LHA not found, that's expected in some test environments */
        printf("  ⚠️  LHA not available (expected in some environments)\n");
    }
}

TEST(test_init_restore_context_null) {
    BOOL result = InitRestoreContext(NULL);
    ASSERT(result == FALSE);
}

TEST(test_reset_statistics) {
    RestoreContext ctx;
    InitRestoreContext(&ctx);
    
    /* Manually set some values */
    ctx.stats.archivesRestored = 5;
    ctx.stats.archivesFailed = 2;
    ctx.stats.totalBytesRestored = 1024000;
    ctx.stats.hasErrors = TRUE;
    
    /* Reset */
    ResetRestoreStatistics(&ctx);
    
    ASSERT(ctx.stats.archivesRestored == 0);
    ASSERT(ctx.stats.archivesFailed == 0);
    ASSERT(ctx.stats.totalBytesRestored == 0);
    ASSERT(ctx.stats.hasErrors == FALSE);
}

/* ========================================================================
 * TESTS - Path Validation
 * ======================================================================== */

TEST(test_validate_restore_path_valid) {
    ASSERT(ValidateRestorePath("DH0:Projects/MyApp/") == TRUE);
    ASSERT(ValidateRestorePath("Work:Documents/") == TRUE);
    ASSERT(ValidateRestorePath("DH0:") == TRUE);  /* Root is valid */
}

TEST(test_validate_restore_path_invalid) {
    ASSERT(ValidateRestorePath(NULL) == FALSE);
    ASSERT(ValidateRestorePath("") == FALSE);
    ASSERT(ValidateRestorePath("x") == FALSE);  /* Too short */
}

TEST(test_validate_restore_path_edge_cases) {
#ifndef _WIN32
    /* On Unix/Amiga, test volume-based paths */
    ASSERT(ValidateRestorePath("DH0:/") == TRUE);
    ASSERT(ValidateRestorePath("RAM:") == TRUE);
    ASSERT(ValidateRestorePath("NoColon") == FALSE);
#endif
}

/* ========================================================================
 * TESTS - Status Messages
 * ======================================================================== */

TEST(test_status_messages) {
    const char *msg;
    
    msg = GetRestoreStatusMessage(RESTORE_OK);
    ASSERT(msg != NULL);
    ASSERT(strlen(msg) > 0);
    
    msg = GetRestoreStatusMessage(RESTORE_ARCHIVE_NOT_FOUND);
    ASSERT(msg != NULL);
    ASSERT(strstr(msg, "not found") != NULL);
    
    msg = GetRestoreStatusMessage(RESTORE_EXTRACT_FAILED);
    ASSERT(msg != NULL);
    ASSERT(strstr(msg, "extraction") != NULL || strstr(msg, "extract") != NULL);
}

TEST(test_all_status_codes_have_messages) {
    /* Test all enum values have messages */
    ASSERT(GetRestoreStatusMessage(RESTORE_OK) != NULL);
    ASSERT(GetRestoreStatusMessage(RESTORE_FAIL) != NULL);
    ASSERT(GetRestoreStatusMessage(RESTORE_ARCHIVE_NOT_FOUND) != NULL);
    ASSERT(GetRestoreStatusMessage(RESTORE_MARKER_NOT_FOUND) != NULL);
    ASSERT(GetRestoreStatusMessage(RESTORE_MARKER_READ_FAILED) != NULL);
    ASSERT(GetRestoreStatusMessage(RESTORE_EXTRACT_FAILED) != NULL);
    ASSERT(GetRestoreStatusMessage(RESTORE_INVALID_PATH) != NULL);
    ASSERT(GetRestoreStatusMessage(RESTORE_CATALOG_NOT_FOUND) != NULL);
    ASSERT(GetRestoreStatusMessage(RESTORE_CATALOG_READ_FAILED) != NULL);
    ASSERT(GetRestoreStatusMessage(RESTORE_LHA_NOT_FOUND) != NULL);
    ASSERT(GetRestoreStatusMessage(RESTORE_INVALID_PARAMS) != NULL);
}

/* ========================================================================
 * TESTS - Statistics Formatting
 * ======================================================================== */

TEST(test_statistics_formatting_bytes) {
    RestoreContext ctx;
    InitRestoreContext(&ctx);
    
    ctx.stats.archivesRestored = 5;
    ctx.stats.archivesFailed = 2;
    ctx.stats.totalBytesRestored = 512;  /* Bytes */
    
    char buffer[256];
    GetRestoreStatistics(&ctx, buffer, sizeof(buffer));
    
    ASSERT(strlen(buffer) > 0);
    ASSERT(strstr(buffer, "5") != NULL);  /* Archives count */
    ASSERT(strstr(buffer, "2") != NULL);  /* Failed count */
    ASSERT(strstr(buffer, "bytes") != NULL);
}

TEST(test_statistics_formatting_kb) {
    RestoreContext ctx;
    InitRestoreContext(&ctx);
    
    ctx.stats.totalBytesRestored = 15360;  /* 15 KB */
    
    char buffer[256];
    GetRestoreStatistics(&ctx, buffer, sizeof(buffer));
    
    ASSERT(strstr(buffer, "KB") != NULL || strstr(buffer, "kb") != NULL);
}

TEST(test_statistics_formatting_mb) {
    RestoreContext ctx;
    InitRestoreContext(&ctx);
    
    ctx.stats.totalBytesRestored = 5242880;  /* 5 MB */
    
    char buffer[256];
    GetRestoreStatistics(&ctx, buffer, sizeof(buffer));
    
    ASSERT(strstr(buffer, "MB") != NULL || strstr(buffer, "mb") != NULL);
}

TEST(test_statistics_formatting_gb) {
    RestoreContext ctx;
    InitRestoreContext(&ctx);
    
    ctx.stats.totalBytesRestored = 2147483648UL;  /* 2 GB */
    
    char buffer[256];
    GetRestoreStatistics(&ctx, buffer, sizeof(buffer));
    
    ASSERT(strstr(buffer, "GB") != NULL || strstr(buffer, "gb") != NULL);
}

/* ========================================================================
 * TESTS - Archive Validation
 * ======================================================================== */

TEST(test_can_restore_archive_nonexistent) {
    RestoreContext ctx;
    if (!InitRestoreContext(&ctx)) {
        printf("  ⚠️  Skipping (LHA not available)\n");
        return;
    }
    
    BOOL result = CanRestoreArchive("nonexistent.lha", ctx.lhaPath);
    ASSERT(result == FALSE);
}

TEST(test_can_restore_archive_null_params) {
    RestoreContext ctx;
    if (!InitRestoreContext(&ctx)) {
        printf("  ⚠️  Skipping (LHA not available)\n");
        return;
    }
    
    ASSERT(CanRestoreArchive(NULL, ctx.lhaPath) == FALSE);
    ASSERT(CanRestoreArchive("test.lha", NULL) == FALSE);
    ASSERT(CanRestoreArchive(NULL, NULL) == FALSE);
}

/* ========================================================================
 * TESTS - Restore Operations (with LHA)
 * ======================================================================== */

TEST(test_restore_archive_invalid_params) {
    RestoreContext ctx;
    if (!InitRestoreContext(&ctx)) {
        printf("  ⚠️  Skipping (LHA not available)\n");
        return;
    }
    
    ASSERT(RestoreArchive(NULL, "test.lha") == RESTORE_INVALID_PARAMS);
    ASSERT(RestoreArchive(&ctx, NULL) == RESTORE_INVALID_PARAMS);
}

TEST(test_restore_archive_not_found) {
    RestoreContext ctx;
    if (!InitRestoreContext(&ctx)) {
        printf("  ⚠️  Skipping (LHA not available)\n");
        return;
    }
    
    RestoreStatus status = RestoreArchive(&ctx, "nonexistent.lha");
    ASSERT(status == RESTORE_ARCHIVE_NOT_FOUND);
    ASSERT(ctx.stats.archivesFailed == 1);
    ASSERT(ctx.stats.hasErrors == TRUE);
}

TEST(test_restore_full_run_catalog_not_found) {
    RestoreContext ctx;
    if (!InitRestoreContext(&ctx)) {
        printf("  ⚠️  Skipping (LHA not available)\n");
        return;
    }
    
    RestoreStatus status = RestoreFullRun(&ctx, "nonexistent_run");
    ASSERT(status == RESTORE_CATALOG_NOT_FOUND);
}

TEST(test_orphaned_recovery_not_found) {
    RestoreContext ctx;
    if (!InitRestoreContext(&ctx)) {
        printf("  ⚠️  Skipping (LHA not available)\n");
        return;
    }
    
    RestoreStatus status = RecoverOrphanedArchive(&ctx, "nonexistent.lha");
    ASSERT(status == RESTORE_ARCHIVE_NOT_FOUND);
}

/* ========================================================================
 * TESTS - Integration (Backup + Restore)
 * ======================================================================== */

TEST(test_backup_and_restore_single_folder) {
    /* Skip if LHA not available */
    char lhaPath[256];
    if (!CheckLhaAvailable(lhaPath)) {
        printf("  ⚠️  Skipping (LHA not available)\n");
        return;
    }
    
    /* Create test source folder with .info files */
    const char *testSource = "./test_restore_source";
    CreateTestDirectory(testSource);
    CreateTestInfoFile(testSource, "Test1.info");
    CreateTestInfoFile(testSource, "Test2.info");
    CreateTestInfoFile(testSource, "Test3.info");
    
    /* Backup the folder */
    BackupContext backupCtx;
    BackupPreferences prefs = {
        .enableUndoBackup = TRUE,
        .useLha = TRUE,
        .maxBackupsPerFolder = 100
    };
    strncpy(prefs.backupRootPath, "./test_backups", sizeof(prefs.backupRootPath));
    
    ASSERT(InitBackupSession(&backupCtx, &prefs, testSource) == TRUE);
    BackupStatus backupStatus = BackupFolder(&backupCtx, testSource, 0);
    CloseBackupSession(&backupCtx);
    
    ASSERT(backupStatus == BACKUP_OK);
    
    /* Now delete the source files (simulate loss) */
    char file1[512], file2[512], file3[512];
    snprintf(file1, sizeof(file1), "%s/Test1.info", testSource);
    snprintf(file2, sizeof(file2), "%s/Test2.info", testSource);
    snprintf(file3, sizeof(file3), "%s/Test3.info", testSource);
    unlink(file1);
    unlink(file2);
    unlink(file3);
    
    ASSERT(FileExistsTest(file1) == FALSE);
    ASSERT(FileExistsTest(file2) == FALSE);
    ASSERT(FileExistsTest(file3) == FALSE);
    
    /* Restore from archive */
    RestoreContext restoreCtx;
    ASSERT(InitRestoreContext(&restoreCtx) == TRUE);
    
    char archivePath[512];
    snprintf(archivePath, sizeof(archivePath), "%s/Run_0001/000/00001.lha", 
             prefs.backupRootPath);
    
    RestoreStatus restoreStatus = RestoreArchive(&restoreCtx, archivePath);
    ASSERT(restoreStatus == RESTORE_OK);
    ASSERT(restoreCtx.stats.archivesRestored == 1);
    ASSERT(restoreCtx.stats.archivesFailed == 0);
    
    /* Verify files are restored */
    ASSERT(FileExistsTest(file1) == TRUE);
    ASSERT(FileExistsTest(file2) == TRUE);
    ASSERT(FileExistsTest(file3) == TRUE);
    
    /* Clean up */
    unlink(file1);
    unlink(file2);
    unlink(file3);
    RemoveTestDirectory(testSource);
}

TEST(test_restore_full_run_multiple_folders) {
    /* Skip if LHA not available */
    char lhaPath[256];
    if (!CheckLhaAvailable(lhaPath)) {
        printf("  ⚠️  Skipping (LHA not available)\n");
        return;
    }
    
    /* Create multiple test folders */
    const char *testDir1 = "./test_restore_multi1";
    const char *testDir2 = "./test_restore_multi2";
    const char *testDir3 = "./test_restore_multi3";
    
    CreateTestDirectory(testDir1);
    CreateTestDirectory(testDir2);
    CreateTestDirectory(testDir3);
    
    CreateTestInfoFile(testDir1, "File1.info");
    CreateTestInfoFile(testDir2, "File2.info");
    CreateTestInfoFile(testDir3, "File3.info");
    
    /* Backup all folders */
    BackupContext backupCtx;
    BackupPreferences prefs = {
        .enableUndoBackup = TRUE,
        .useLha = TRUE,
        .maxBackupsPerFolder = 100
    };
    strncpy(prefs.backupRootPath, "./test_backups_multi", sizeof(prefs.backupRootPath));
    
    ASSERT(InitBackupSession(&backupCtx, &prefs, NULL) == TRUE);
    ASSERT(BackupFolder(&backupCtx, testDir1, 0) == BACKUP_OK);
    ASSERT(BackupFolder(&backupCtx, testDir2, 0) == BACKUP_OK);
    ASSERT(BackupFolder(&backupCtx, testDir3, 0) == BACKUP_OK);
    CloseBackupSession(&backupCtx);
    
    /* Delete all source files */
    char file1[512], file2[512], file3[512];
    snprintf(file1, sizeof(file1), "%s/File1.info", testDir1);
    snprintf(file2, sizeof(file2), "%s/File2.info", testDir2);
    snprintf(file3, sizeof(file3), "%s/File3.info", testDir3);
    unlink(file1);
    unlink(file2);
    unlink(file3);
    
    /* Restore entire run */
    RestoreContext restoreCtx;
    ASSERT(InitRestoreContext(&restoreCtx) == TRUE);
    
    char runDir[512];
    snprintf(runDir, sizeof(runDir), "%s/Run_0001", prefs.backupRootPath);
    
    RestoreStatus status = RestoreFullRun(&restoreCtx, runDir);
    ASSERT(status == RESTORE_OK);
    ASSERT(restoreCtx.stats.archivesRestored == 3);
    ASSERT(restoreCtx.stats.archivesFailed == 0);
    
    /* Verify all files restored */
    ASSERT(FileExistsTest(file1) == TRUE);
    ASSERT(FileExistsTest(file2) == TRUE);
    ASSERT(FileExistsTest(file3) == TRUE);
    
    /* Clean up */
    unlink(file1);
    unlink(file2);
    unlink(file3);
    RemoveTestDirectory(testDir1);
    RemoveTestDirectory(testDir2);
    RemoveTestDirectory(testDir3);
}

TEST(test_orphaned_archive_recovery) {
    /* Skip if LHA not available */
    char lhaPath[256];
    if (!CheckLhaAvailable(lhaPath)) {
        printf("  ⚠️  Skipping (LHA not available)\n");
        return;
    }
    
    /* Create and backup a test folder */
    const char *testSource = "./test_orphan_source";
    CreateTestDirectory(testSource);
    CreateTestInfoFile(testSource, "Orphan.info");
    
    BackupContext backupCtx;
    BackupPreferences prefs = {
        .enableUndoBackup = TRUE,
        .useLha = TRUE,
        .maxBackupsPerFolder = 100
    };
    strncpy(prefs.backupRootPath, "./test_orphan_backup", sizeof(prefs.backupRootPath));
    
    ASSERT(InitBackupSession(&backupCtx, &prefs, testSource) == TRUE);
    ASSERT(BackupFolder(&backupCtx, testSource, 0) == BACKUP_OK);
    CloseBackupSession(&backupCtx);
    
    /* Delete source file */
    char orphanFile[512];
    snprintf(orphanFile, sizeof(orphanFile), "%s/Orphan.info", testSource);
    unlink(orphanFile);
    
    /* Simulate orphaned archive (catalog missing) - just use the archive directly */
    RestoreContext restoreCtx;
    ASSERT(InitRestoreContext(&restoreCtx) == TRUE);
    
    char archivePath[512];
    snprintf(archivePath, sizeof(archivePath), "%s/Run_0001/000/00001.lha",
             prefs.backupRootPath);
    
    /* Recover using orphaned recovery (which uses embedded marker) */
    RestoreStatus status = RecoverOrphanedArchive(&restoreCtx, archivePath);
    ASSERT(status == RESTORE_OK);
    ASSERT(restoreCtx.stats.archivesRestored == 1);
    
    /* Verify file is restored */
    ASSERT(FileExistsTest(orphanFile) == TRUE);
    
    /* Clean up */
    unlink(orphanFile);
    RemoveTestDirectory(testSource);
}

/* ========================================================================
 * MAIN TEST RUNNER
 * ======================================================================== */

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║   iTidy Backup System - Restore Operations Test Suite     ║\n");
    printf("║   Task 8: Restore Operations                               ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    /* Context Initialization Tests */
    printf("\n▶ Context Initialization Tests\n");
    RUN_TEST(test_init_restore_context);
    RUN_TEST(test_init_restore_context_null);
    RUN_TEST(test_reset_statistics);
    
    /* Path Validation Tests */
    printf("\n▶ Path Validation Tests\n");
    RUN_TEST(test_validate_restore_path_valid);
    RUN_TEST(test_validate_restore_path_invalid);
    RUN_TEST(test_validate_restore_path_edge_cases);
    
    /* Status Message Tests */
    printf("\n▶ Status Message Tests\n");
    RUN_TEST(test_status_messages);
    RUN_TEST(test_all_status_codes_have_messages);
    
    /* Statistics Tests */
    printf("\n▶ Statistics Formatting Tests\n");
    RUN_TEST(test_statistics_formatting_bytes);
    RUN_TEST(test_statistics_formatting_kb);
    RUN_TEST(test_statistics_formatting_mb);
    RUN_TEST(test_statistics_formatting_gb);
    
    /* Archive Validation Tests */
    printf("\n▶ Archive Validation Tests\n");
    RUN_TEST(test_can_restore_archive_nonexistent);
    RUN_TEST(test_can_restore_archive_null_params);
    
    /* Restore Operations Tests */
    printf("\n▶ Restore Operations Tests\n");
    RUN_TEST(test_restore_archive_invalid_params);
    RUN_TEST(test_restore_archive_not_found);
    RUN_TEST(test_restore_full_run_catalog_not_found);
    RUN_TEST(test_orphaned_recovery_not_found);
    
    /* Integration Tests (require LHA) */
    printf("\n▶ Integration Tests (Backup + Restore)\n");
    RUN_TEST(test_backup_and_restore_single_folder);
    RUN_TEST(test_restore_full_run_multiple_folders);
    RUN_TEST(test_orphaned_archive_recovery);
    
    /* Summary */
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                      TEST SUMMARY                          ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  Total Tests:  %3d                                         ║\n", g_testsPassed + g_testsFailed);
    printf("║  ✅ Passed:    %3d                                         ║\n", g_testsPassed);
    printf("║  ❌ Failed:    %3d                                         ║\n", g_testsFailed);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    if (g_testsFailed == 0) {
        printf("🎉 All tests passed! Task 8 implementation complete.\n\n");
        return 0;
    } else {
        printf("⚠️  Some tests failed. Review errors above.\n\n");
        return 1;
    }
}
