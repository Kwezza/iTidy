# Catalog Source Directory Feature

## Overview
Added the source directory being tidied to the backup catalog.txt file to make it easier for users to identify what was backed up when viewing the restore window.

## Problem
The catalog.txt file was missing information about which folder iTidy was told to tidy. This made it difficult for users to determine what they needed to restore by looking at the catalog in the restore window.

### Before:
```
iTidy Backup Catalog v1.0
========================================
Run Number: 0004
Session Started: 2025-10-27 14:12:08
LhA Path: C:LhA
========================================
```

### After:
```
iTidy Backup Catalog v1.0
========================================
Run Number: 0004
Session Started: 2025-10-27 14:12:08
Source Directory: DH0:Projects/MyWork
LhA Path: C:LhA
========================================
```

## Implementation

### Changes Made

1. **backup_types.h**
   - Added `sourceDirectory` field to `BackupContext` structure
   - Stores the root directory being tidied

2. **backup_session.h**
   - Updated `InitBackupSession()` signature to accept `sourceDirectory` parameter
   - Added documentation for the new parameter

3. **backup_session.c**
   - Modified `InitBackupSession()` to store the source directory in the context
   - Added logging to show source directory when backup session starts

4. **backup_catalog.c**
   - Updated `CreateCatalog()` to write "Source Directory:" line to catalog header
   - Only writes this line if source directory is provided (backward compatible)

5. **layout_processor.c**
   - Updated call to `InitBackupSession()` to pass the sanitized path being processed
   - Added console output showing source directory when backup starts

6. **Test Files**
   - Updated all test files to pass appropriate source directory or NULL
   - Ensures backward compatibility for tests that don't need this feature

## Benefits

1. **User Experience**: Users can now see at a glance which folder was backed up
2. **Restore Clarity**: Makes it much easier to identify the correct backup to restore
3. **Debugging**: Helps troubleshoot backup issues by clearly showing what was backed up
4. **Backward Compatible**: Optional parameter - NULL can be passed if not needed

## Usage Example

When processing a directory:
```c
if (InitBackupSession(&localContext, &backupPrefs, "DH0:Projects/MyWork"))
{
    // Backup session started with source directory
    // Catalog will show: "Source Directory: DH0:Projects/MyWork"
}
```

## Notes

- The source directory field is optional - if NULL or empty, it won't be written to the catalog
- The feature is automatically enabled when using the GUI or ProcessDirectoryWithPreferences
- The restore window can now read and display this information
- All existing functionality remains unchanged
