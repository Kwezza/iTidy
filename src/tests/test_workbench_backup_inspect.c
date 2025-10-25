/*
 * Standalone test to backup real Workbench folders and inspect results
 * Does NOT clean up so we can examine the created archives
 */

#define PLATFORM_HOST
#include "../backup_session.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    BackupContext ctx;
    BackupPreferences prefs;
    
    printf("===========================================\n");
    printf("Workbench Backup Test (with inspection)\n");
    printf("===========================================\n\n");
    
    /* Initialize preferences */
    memset(&prefs, 0, sizeof(prefs));
    prefs.enableUndoBackup = TRUE;
    prefs.useLha = TRUE;
    strcpy(prefs.backupRootPath, "./wb_backup_test");
    prefs.maxBackupsPerFolder = 100;
    
    /* Initialize session */
    printf("Initializing backup session...\n");
    if (!InitBackupSession(&ctx, &prefs)) {
        printf("ERROR: Failed to initialize session\n");
        return 1;
    }
    printf("Session initialized successfully\n\n");
    
    /* Backup Workbench/Prefs */
    printf("Backing up workbench/Prefs...\n");
    if (BackupFolder(&ctx, "../../workbench/Prefs") == TRUE) {
        printf("  SUCCESS: Prefs backed up\n");
    } else {
        printf("  FAILED\n");
    }
    
    /* Backup Workbench/Tools */
    printf("Backing up workbench/Tools...\n");
    if (BackupFolder(&ctx, "../../workbench/Tools") == TRUE) {
        printf("  SUCCESS: Tools backed up\n");
    } else {
        printf("  FAILED\n");
    }
    
    /* Backup Workbench/Utilities */
    printf("Backing up workbench/Utilities...\n");
    if (BackupFolder(&ctx, "../../workbench/Utilities") == TRUE) {
        printf("  SUCCESS: Utilities backed up\n");
    } else {
        printf("  FAILED\n");
    }
    
    /* Close session */
    printf("\nClosing backup session...\n");
    CloseBackupSession(&ctx);
    
    /* Print summary */
    printf("\n===========================================\n");
    printf("Backup Complete!\n");
    printf("===========================================\n");
    printf("Folders backed up: %d\n", ctx.foldersBackedUp);
    printf("Failed backups:    %d\n", ctx.failedBackups);
    printf("Total bytes:       %ld\n", ctx.totalBytesArchived);
    printf("\nBackup location: %s/\n", prefs.backupRootPath);
    printf("\nTo inspect archives:\n");
    printf("  cd wb_backup_test/Run_0001/000\n");
    printf("  lha l 00001.lha  # View Prefs contents\n");
    printf("  lha l 00002.lha  # View Tools contents\n");
    printf("  lha l 00003.lha  # View Utilities contents\n");
    printf("\nTo view catalog:\n");
    printf("  type wb_backup_test\\Run_0001\\catalog.txt\n");
    printf("\n");
    
    return 0;
}
