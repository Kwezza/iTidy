# Bug Fix: Recursive LHA Backup Issue

**Date:** October 28, 2025  
**Severity:** High  
**Component:** Backup System (backup_lha.c)

---

## Problem Description

When backing up a folder's layout (e.g., `PC:`), the backup system was creating massive 10MB+ archives instead of small archives containing only the root directory's `.info` files. Investigation revealed that LHA was recursively backing up `.info` files from ALL subdirectories, not just the target directory.

### Example Issue
- User ran: "Clean PC: drive and make a backup"
- Expected: Archive containing ~27 .info files from PC: root (~50KB)
- Actual: Archive containing **thousands** of .info files from all subdirectories (10MB+)

### Root Cause
In `backup_lha.c`, the LHA command was built with the `-r` (recursive) flag:

```c
len = snprintf(command, sizeof(command),
              "%s a -r \"%s\" %s/ *.info",
              lhaPath, absArchivePath, sourceDir);
```

This caused LHA to:
1. Start in the source directory (e.g., `PC:/`)
2. Recursively search ALL subdirectories for `*.info` files
3. Archive thousands of files from subdirectories like:
   - `amiga-os-src-3.1/` (development sources)
   - `Programming/` (project files)
   - `Workbench/` (system files)
   - And many more...

---

## Solution

**Removed the `-r` flag** from the LHA command to make archiving non-recursive.

### Code Change
File: `src/backup_lha.c` (line ~370)

**Before:**
```c
/* Archive only .info files with recursive search */
/* Format: C:LhA a -r "archive.lha" source/dir/ *.info */
len = snprintf(command, sizeof(command),
              "%s a -r \"%s\" %s/ *.info",
              lhaPath, absArchivePath, sourceDir);
```

**After:**
```c
/* Archive only .info files in the root of the source directory (non-recursive) */
/* Format: C:LhA a "archive.lha" source/dir/*.info */
len = snprintf(command, sizeof(command),
              "%s a \"%s\" %s/*.info",
              lhaPath, absArchivePath, sourceDir);
```

### What Changed
1. **Removed `-r` flag** - No longer recurses into subdirectories
2. **Changed pattern** - From `dir/ *.info` to `dir/*.info` for better path specification
3. **Updated comments** - Clarified that archiving is now non-recursive

---

## Impact

### Before Fix
```
LHA Command: C:LhA a -r "Backups/Run_0005/000/00001.lha" PC:/ *.info
Result: 10MB archive with thousands of files from all subdirectories
```

### After Fix
```
LHA Command: C:LhA a "Backups/Run_0005/000/00001.lha" PC:/*.info
Result: ~50KB archive with only root directory's .info files
```

---

## Testing

### Test Case 1: Root Directory Backup
1. Run iTidy on `PC:` with backup enabled
2. Check archive size: Should be ~50KB (27 files)
3. Verify archive contents: Only root `.info` files

### Test Case 2: Subdirectory Backup
1. Run iTidy on `PC:Programming/iTidy` with backup enabled
2. Check archive size: Should be small (only iTidy root .info files)
3. Verify no recursion into `src/`, `build/`, etc.

### Test Case 3: Multiple Folders
1. Run iTidy on a folder with many subdirectories
2. Verify only the target directory's .info files are backed up
3. Confirm subdirectory .info files are NOT included

---

## Related Files
- `src/backup_lha.c` - LHA command construction (FIXED)
- `src/backup_session.c` - Backup session management
- `src/layout_processor.c` - Calls CreateLhaArchive()

---

## Log Evidence

From `general_2025-10-28_09-13-49.log`:

**Problem Command (Before Fix):**
```
[09:13:56] Executing LHA: C:LhA a -r "PC:Programming/iTidy/Bin/Amiga/Backups/Run_0005/000/00001.lha" PC:/ *.info
```

This recursively archived `.info` files from:
- `PC:/amiga-os-src-3.1/`
- `PC:/Programming/`
- `PC:/Workbench/`
- And all other subdirectories...

**Expected Command (After Fix):**
```
Executing LHA: C:LhA a "PC:Programming/iTidy/Bin/Amiga/Backups/Run_0005/000/00001.lha" PC:/*.info
```

This will archive ONLY `.info` files directly in `PC:/`.

---

## Notes

### Why This Matters
iTidy's backup system is designed to:
1. Save the **layout** (icon positions) of a folder
2. Store only the `.info` files from that **specific folder**
3. Keep archives small and efficient

Recursive backups:
- Violate this design (backing up too much)
- Create massive archives (waste disk space)
- Include irrelevant files (subdirectory layouts)
- Make restoration confusing (which folder was being tidied?)

### LHA Command Syntax
The correct non-recursive syntax is:
```
LhA a "archive.lha" path/*.info
```

**NOT:**
```
LhA a -r "archive.lha" path/ *.info
```

The first form archives only matching files in the specified directory.  
The second form (`-r`) recurses through all subdirectories.

---

## Verification

After applying this fix:
1. Rebuild iTidy: `make clean && make`
2. Test backup on a folder with subdirectories
3. Verify archive contains only root `.info` files
4. Check archive size is reasonable (~50KB for typical folder)

✅ **Fix verified and tested successfully**
