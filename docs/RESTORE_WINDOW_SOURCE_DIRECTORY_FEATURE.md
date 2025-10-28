# Restore Window Enhancement: Source Directory Display

**Date:** October 28, 2025  
**Component:** Restore Window GUI (restore_window.c/h)  
**Feature:** Display source directory in backup run details

---

## Overview

Enhanced the iTidy Restore Window to display the **source directory** (the folder that was tidied) in the Run Details panel. This makes it much easier for users to identify which backup they need to restore by showing them exactly what folder was tidied.

---

## Implementation

### 1. Data Structure Changes

**File:** `src/GUI/restore_window.h`

Added `sourceDirectory` field to `RestoreRunEntry` structure:

```c
struct RestoreRunEntry
{
    UWORD runNumber;
    char displayString[80];
    char runName[16];
    char dateStr[24];
    char sourceDirectory[256];    /* NEW: Source folder that was tidied */
    ULONG folderCount;
    ULONG totalBytes;
    char sizeStr[16];
    UWORD statusCode;
    char statusStr[16];
    char fullPath[256];
    BOOL hasCatalog;
};
```

### 2. Catalog Parsing Function

**File:** `src/GUI/restore_window.c`

Added `extract_source_directory()` helper function to parse the catalog header:

```c
static BOOL extract_source_directory(const char *catalog_path, 
                                     char *buffer, 
                                     ULONG buffer_size)
{
    /* Opens catalog.txt and reads header lines */
    /* Looks for "Source Directory: <path>" line */
    /* Returns the path in the buffer */
}
```

**How it works:**
1. Opens the catalog.txt file
2. Reads lines sequentially
3. Searches for the line starting with `"Source Directory: "`
4. Extracts the path after the label
5. Stops at the table separator (end of header)

### 3. Run Scanning Enhancement

**File:** `src/GUI/restore_window.c` (scan_backup_runs function)

Modified to extract and store source directory:

```c
if (entry->hasCatalog)
{
    /* ... existing stats parsing ... */
    
    /* NEW: Extract source directory from catalog header */
    if (!extract_source_directory(catalog_path, entry->sourceDirectory, 
                                 sizeof(entry->sourceDirectory)))
    {
        strcpy(entry->sourceDirectory, "(Unknown)");
    }
}
else
{
    /* ... existing code ... */
    strcpy(entry->sourceDirectory, "(No catalog)");
}
```

### 4. Details Panel Display

**File:** `src/GUI/restore_window.c` (update_details_panel function)

Updated to display 7 lines instead of 6, with source directory inserted between "Date Created" and "Total Archives":

**Before:**
```
Run Number:        0004
Date Created:      2025-10-27 14:12:08
Total Archives:    0
Total Size:        0 KB
Status:            Complete (catalog present)
Location:          PROGDIR:Backups/Run_0004
```

**After:**
```
Run Number:        0004
Date Created:      2025-10-27 14:12:08
Source Directory:  PC:
Total Archives:    0
Total Size:        0 KB
Status:            Complete (catalog present)
Location:          PROGDIR:Backups/Run_0004
```

### 5. UI Layout Adjustment

**File:** `src/GUI/restore_window.c` (open_restore_window function)

Increased details listview height to accommodate the extra line:

```c
/* Changed from 6 to 7 lines */
ng.ng_Height = (font_height + 2) * 7;  /* 7 lines for details */
```

---

## User Benefits

### Before This Feature
Users had to guess which backup contained which folder layout:
- "Was Run_0004 for my Projects folder or my Work folder?"
- "Which run backed up PC:?"
- Had to restore and check, or examine catalog.txt manually

### After This Feature
Users can immediately see:
- **Source Directory: PC:** - Shows this backup is for the PC: drive
- **Source Directory: DH0:Projects/MyWork** - Shows the exact folder
- **Source Directory: (No catalog)** - Indicates orphaned backup
- **Source Directory: (Unknown)** - Old backups without this field

### Real-World Example

```
Run Details:
Run Number:        0005
Date Created:      2025-10-28 09:13:56
Source Directory:  PC:                    ← NEW! User knows this is the PC: drive backup
Total Archives:    27
Total Size:        46 KB
Status:            Complete (catalog present)
Location:          PROGDIR:Backups/Run_0005
```

Now when the user sees multiple backup runs, they can instantly identify:
- Run_0004: DH0:Work
- Run_0005: PC:
- Run_0006: DH0:Projects/iTidy
- Run_0007: RAM:Temp

---

## Technical Details

### Catalog Header Format

The catalog.txt header includes the source directory line:

```
iTidy Backup Catalog v1.0
========================================
Run Number: 0005
Session Started: 2025-10-28 09:13:56
Source Directory: PC:                    ← This line is parsed
LhA Path: C:LhA
========================================

# Index    | Subfolder | Size    | Original Path
-----------+-----------+---------+----------------
```

### Parsing Strategy

1. **Line-by-line reading** - Efficient for header parsing
2. **Early termination** - Stops at table separator (----)
3. **Fallback handling** - Shows "(Unknown)" if field missing
4. **Backward compatibility** - Works with old catalogs without this field

### Edge Cases Handled

| Scenario | Display Value | Notes |
|----------|--------------|-------|
| Valid catalog with source dir | Actual path | Normal case |
| Valid catalog, missing source dir | "(Unknown)" | Old backups |
| No catalog.txt | "(No catalog)" | Orphaned runs |
| Catalog read error | "(Unknown)" | File access issues |
| Empty path in catalog | "(Unknown)" | Malformed catalog |

---

## Testing

### Test Case 1: New Backup with Source Directory
1. Create a backup of PC: with iTidy
2. Open Restore Window
3. Select the new run
4. **Expected:** Details show "Source Directory: PC:"

### Test Case 2: Old Backup Without Source Directory
1. Select a backup created before this feature
2. Open Restore Window
3. Select the old run
4. **Expected:** Details show "Source Directory: (Unknown)"

### Test Case 3: Orphaned Backup
1. Delete catalog.txt from a run directory
2. Open Restore Window
3. Select the orphaned run
4. **Expected:** 
   - Status: "Orphaned (no catalog)"
   - Source Directory: "(No catalog)"

### Test Case 4: Multiple Runs
1. Create backups of different folders (PC:, DH0:Work, RAM:Temp)
2. Open Restore Window
3. Browse through runs
4. **Expected:** Each run shows its correct source directory

---

## Related Files Modified

1. **src/GUI/restore_window.h**
   - Added `sourceDirectory` field to `RestoreRunEntry`

2. **src/GUI/restore_window.c**
   - Added `extract_source_directory()` function
   - Modified `scan_backup_runs()` to parse source directory
   - Updated `update_details_panel()` to display 7 lines with source directory
   - Updated `open_restore_window()` to size details listview for 7 lines

---

## Dependencies

This feature relies on:
- **Catalog format** - Must have "Source Directory:" line in header
- **backup_catalog.c** - Creates catalog with source directory (already implemented)
- **backup_session.c** - Passes source directory to catalog (already implemented)

---

## Future Enhancements

Possible future improvements:
1. **Clickable paths** - Open the source directory in Workbench
2. **Path abbreviation** - Shorten long paths for display
3. **Volume icons** - Show disk icon for the volume
4. **Search/filter** - Filter runs by source directory
5. **Duplicate detection** - Highlight multiple backups of same folder

---

## Notes

### Why Between "Date Created" and "Total Archives"?

The source directory is placed after the date because:
1. **Logical flow:** When/What/How-much information order
2. **Most important:** Source directory is usually what users look for first
3. **Natural reading:** Date → Location → Statistics → Status
4. **User request:** Specifically requested this position

### String Sizes

- **sourceDirectory[256]** - Matches AmigaDOS path limits
- Safe for all volume:path/to/folder combinations
- Consistent with `fullPath[256]` field

### Memory Impact

- **Per entry:** +256 bytes for sourceDirectory string
- **Typical window:** ~10 runs × 256 bytes = ~2.5 KB
- **Negligible:** Modern Amigas have plenty of RAM for this

---

## Verification

✅ **Build Status:** Clean compilation, no errors  
✅ **Backward Compatibility:** Works with old catalogs (shows "Unknown")  
✅ **Forward Compatibility:** New catalogs include source directory  
✅ **UI Layout:** Details panel properly sized for 7 lines  
✅ **User Experience:** Easy to identify which folder was backed up

---

## See Also

- `CATALOG_SOURCE_DIRECTORY_FEATURE.md` - Catalog format documentation
- `RESTORE_WINDOW_GUI_SPEC.md` - Overall restore window design
- `BACKUP_SYSTEM_PROPOSAL.md` - Backup system architecture
