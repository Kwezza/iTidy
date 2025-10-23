# iTidy Backup System - Technical Proposal

**Date:** October 23, 2025  
**Author:** Kerry Thompson  
**Status:** Design Proposal  
**Purpose:** Safe, scalable backup system for icon layout preservation

---

## Executive Summary

This proposal defines a robust backup system for iTidy that addresses real-world Amiga constraints:
- **Filesystem limitations** (FFS 30-char/component, 107-char total path)
- **Performance issues** (10,000+ files in one directory)
- **Cross-filesystem backups** (PFS data → FFS boot drive)
- **Missing RTC** (Real-Time Clock) on many systems
- **WHDLoad collections** (potentially 50,000+ folders)

---

## Problems Solved

### 1. Filename Length Limits

**Problem:**
```
Path: DH0:Projects/ClientWork/WebDesign/CompanyName/Assets/Images/Products/
Sanitized: DH0_Projects_ClientWork_WebDesign_CompanyName_Assets_Images_Products.lha
Length: 80+ characters → EXCEEDS FFS LIMITS
```

**Solution:** Use short indexed filenames (`00001.lha`) with catalog mapping.

---

### 2. Performance Degradation

**Problem:**
- WHDLoad archives can contain 5,000+ game folders
- Recursive tidying creates one backup per folder
- FFS linear directory scan = O(n) performance
- Result: **System crawls with 10,000+ files in one directory**

**Solution:** Hierarchical storage (max 100 files per folder).

---

### 3. No Real-Time Clock

**Problem:**
- Many Amigas lack battery-backed clocks
- Boot time resets to Jan 1, 1978
- Timestamp-based backups collide or appear out-of-order

**Solution:** Sequential run numbers (guaranteed unique).

---

### 4. Root Folder Special Case

**Problem:**
- Normal folder: `DH0:Projects/MyFolder` → window settings in `DH0:Projects/MyFolder.info`
- Root folder: `DH0:` → window settings in `DH0:.info` (no parent!)

**Solution:** Detect root folders and handle `.info` location accordingly.

---

## Proposed Architecture

### Directory Structure

```
PROGDIR:Backups/
├── Run_0001/
│   ├── catalog.txt          ← Master index of all backups
│   ├── 000/                 ← Archives 00001-00099
│   │   ├── 00001.lha
│   │   ├── 00002.lha
│   │   └── ...              (max 100 files)
│   ├── 001/                 ← Archives 00100-00199
│   │   ├── 00100.lha
│   │   └── ...
│   ├── 123/                 ← Archives 12300-12399
│   │   ├── 12345.lha
│   │   └── ...
│   └── 999/                 ← Archives 99900-99999
├── Run_0002/
│   └── ...
└── Run_0003/
```

### Key Features

| Feature | Implementation |
|---------|----------------|
| **Run Numbering** | Sequential (0001, 0002, ...) |
| **Archive Indexing** | 5 digits (00001-99999) |
| **Folder Structure** | Hierarchical (grouped by hundreds) |
| **Files per Folder** | Max 100 (optimal for FFS) |
| **Catalog** | Human-readable mapping file |
| **Path Recovery** | `_PATH.txt` stored inside each archive |

---

## Archive Naming

### Format
```
[Run]/[Folder]/[Index].lha
```

### Examples
```
Run_0001/000/00001.lha    ← First backup (DH0:Projects/)
Run_0001/000/00042.lha    ← 42nd backup (DH0:Documents/)
Run_0001/123/12345.lha    ← 12,345th backup (WHDLoad game folder)
Run_0002/000/00001.lha    ← New run, resets index
```

### Path Calculation
```c
UWORD archiveIndex = 12345;
UWORD folderNum = archiveIndex / 100;  // 123
sprintf(path, "%s/%03d/%05d.lha", runDir, folderNum, archiveIndex);
// Result: "Run_0001/123/12345.lha"
```

---

## Catalog Format

### Structure
```
iTidy Backup Catalog v1.0
========================================
Run Number: 0001
Session Started: 2025-10-23 14:30:00
Session Ended: 2025-10-23 14:35:12
Total Archives: 12345
LhA Version: 1.38
========================================

# Index    | Subfolder | Size    | Original Path
-----------+-----------+---------+----------------------------------------------
00001.lha  | 000/      | 15 KB   | DH0:Projects/ClientWork/WebDesign/
00002.lha  | 000/      | 8 KB    | DH0:Documents/Letters/Personal/
00042.lha  | 000/      | 22 KB   | Work:Development/SourceCode/
12345.lha  | 123/      | 5 KB    | Work:WHDLoad/Games/Z/Zool/
```

### Features
- Human-readable
- Machine-parseable (tab/pipe delimited)
- Contains verification data (size)
- Maps index → original path

---

## Archive Contents

### Standard Folder
```
00042.lha:
├── _PATH.txt              ← "DH0:Projects/MyFolder" (recovery metadata)
├── #?.info                ← All icon files from inside MyFolder/
└── MyFolder.info          ← Drawer window settings (from parent directory)
```

### Root Folder
```
00001.lha:
├── _PATH.txt              ← "DH0:" (recovery metadata)
├── #?.info                ← All icon files from DH0: root
└── .info                  ← Root window settings (DH0:.info)
```

### `_PATH.txt` Format
```
iTidy Backup Path Marker
Original Path: DH0:Projects/MyFolder
Backup Date: 2025-10-23 14:30:00
Archive Index: 00042
Run Number: 0001
```

---

## Backup Process Flow

### 1. Session Initialization
```
1. Scan PROGDIR:Backups/ for highest Run_NNNN
2. Create Run_[NEXT]/ directory
3. Create catalog.txt
4. Initialize archive index = 00001
```

### 2. Per-Folder Backup
```
For each folder to tidy:
  1. Check if icons exist (skip if empty)
  2. Generate next archive index (e.g., 00042)
  3. Calculate subfolder (00042 / 100 = 0 → 000/)
  4. Create subfolder if needed (PROGDIR:Backups/Run_0001/000/)
  5. Create archive path (000/00042.lha)
  
  6. Backup icon files:
     - Execute: LhA a -q -r -m1 "000/00042.lha" "DH0:MyFolder/#?.info"
  
  7. Backup drawer settings:
     - If root folder: Add "DH0:.info"
     - If normal folder: Add "DH0:Parent/MyFolder.info"
  
  8. Create _PATH.txt with original path
  9. Add _PATH.txt to archive
  
  10. Log to catalog.txt:
      - Archive index
      - Subfolder location
      - Original path
      - Archive size
      - Success/failure status
  
  11. Increment archive index
```

### 3. Session Finalization
```
1. Write catalog footer (total count, end time)
2. Close catalog.txt
3. Log summary to main log
```

---

## Restore Process

### Individual Folder Restore
```
User selects: Run_0001/123/12345.lha

1. Read catalog.txt
2. Find entry for 12345.lha
3. Extract original path: "Work:WHDLoad/Games/Z/Zool/"
4. Execute: LhA x -r "123/12345.lha" "Work:WHDLoad/Games/Z/Zool/"
5. Window settings and icon positions restored
```

### Full Run Undo
```
User selects: Run_0001 (entire run)

1. Read catalog.txt
2. For each entry:
   - Extract archive index and subfolder
   - Read original path
   - Execute: LhA x -r "[subfolder]/[index].lha" "[original path]/"
3. All folders from that session restored
```

### Orphaned Archive Recovery
```
Catalog.txt is lost or corrupted!

1. Scan Run_0001/ for all .lha files
2. For each archive:
   - Extract _PATH.txt temporarily
   - Read original path
   - Restore archive to that path
   - Delete temporary _PATH.txt
3. Recovery complete (slower but functional)
```

---

## Root Folder Detection

### Implementation
```c
/**
 * Check if path is a root/volume path
 * Examples: "DH0:", "Work:", "RAM:"
 */
BOOL IsRootFolder(const char *path) {
    char *colon = strchr(path, ':');
    if (!colon) return FALSE;
    
    // Check if anything after the colon
    if (*(colon + 1) == '\0') {
        return TRUE;  // "DH0:" with nothing after
    }
    
    return FALSE;
}
```

### Drawer Icon Path Logic
```c
char drawerIconPath[256];

if (IsRootFolder(folderPath)) {
    // Root: "DH0:" → "DH0:.info"
    sprintf(drawerIconPath, "%s.info", folderPath);
} else {
    // Normal: "DH0:Projects/MyFolder"
    // → Parent: "DH0:Projects"
    // → Icon: "DH0:Projects/MyFolder.info"
    GetParentPath(folderPath, parentPath);
    GetFolderName(folderPath, folderName);
    sprintf(drawerIconPath, "%s/%s.info", parentPath, folderName);
}
```

---

## Performance Analysis

### File Count Distribution
| Archives | Flat Structure | Hierarchical Structure |
|----------|----------------|------------------------|
| 100 | 100 files in 1 folder | 100 files in 1-2 folders |
| 1,000 | 1,000 files in 1 folder ⚠️ | 100 files × 10 folders ✅ |
| 10,000 | 10,000 files in 1 folder ❌ | 100 files × 100 folders ✅ |
| 50,000 | UNUSABLE ❌ | 100 files × 500 folders ✅ |

### Directory Access Time (FFS)
- **Flat (10,000 files):** 30-60 seconds to open
- **Hierarchical (100 files):** <1 second to open

### Workbench Impact
- Opening `Run_0001/` shows ~1,000 subfolders (acceptable)
- Opening any subfolder shows ~100 files (fast)

---

## Integration Points

### Main iTidy Flow
```
1. User selects folder to tidy
2. iTidy scans folder, builds icon array
3. IF icons found AND backup enabled:
   → Call BackupFolder(ctx, folderPath)
4. Proceed with tidying operation
5. Save tidied icon positions
```

### Backup API
```c
// Initialize backup session
BOOL InitBackupSession(BackupContext *ctx, const BackupPreferences *prefs);

// Backup a single folder
BOOL BackupFolder(BackupContext *ctx, const char *folderPath);

// Close backup session
void CloseBackupSession(BackupContext *ctx);

// Restore operations (future)
BOOL RestoreFolder(const char *archivePath, const char *targetPath);
BOOL RestoreFullRun(UWORD runNumber);
```

---

## Error Handling

### Scenarios

| Error | Handling |
|-------|----------|
| LhA not found | Disable backup, warn user, continue |
| Backup dir creation fails | Abort backup, continue tidying |
| Individual archive fails | Log failure, continue with next folder |
| Disk full during backup | Stop backup, warn user, allow tidy or abort |
| Catalog write fails | Continue backup (can recover from _PATH.txt) |

---

## Future Enhancements

### 1. Backup Retention Policy
```c
// Clean up old runs, keep only N most recent
UWORD CleanupOldBackups(const BackupPreferences *prefs) {
    // Scan for Run_NNNN directories
    // Sort by number
    // Delete oldest if count > maxBackupsPerFolder
}
```

### 2. Compression Level Control
```c
// Allow user to choose LhA compression
// -m0 = Store only (fast, larger)
// -m1 = Medium (default)
// -m2 = Maximum (slow, smaller)
```

### 3. Backup Statistics
```c
typedef struct {
    UWORD totalArchives;
    ULONG totalSizeBytes;
    UWORD failedBackups;
    ULONG durationSeconds;
} BackupStatistics;
```

### 4. Incremental Backups
```c
// Only backup if folder changed since last run
// Compare modification timestamps
// Skip unchanged folders
```

---

## Testing Checklist

### Unit Tests
- [ ] Run number generation (0001, 0002, ...)
- [ ] Archive path calculation (index → subfolder)
- [ ] Root folder detection (`DH0:` vs `DH0:Folder`)
- [ ] Catalog parsing and writing
- [ ] Path sanitization edge cases

### Integration Tests
- [ ] Single folder backup
- [ ] Recursive folder backup (100+ folders)
- [ ] Root folder backup (DH0:, Work:, etc.)
- [ ] Backup with missing drawer icon
- [ ] Backup with no RTC (date = 1978)

### Performance Tests
- [ ] 1,000 folders (verify subfolder creation)
- [ ] 10,000 folders (verify no slowdown)
- [ ] Deep nested paths (256+ char total length)
- [ ] Cross-filesystem (PFS → FFS backup)

### Recovery Tests
- [ ] Restore single folder
- [ ] Restore full run
- [ ] Restore with missing catalog.txt
- [ ] Restore to different path

---

## Implementation Priority

### Phase 1: Core Backup (Required for v1.2)
1. ✅ Run number generation
2. ✅ Hierarchical archive structure
3. ✅ Catalog creation
4. ✅ Path marker in archives
5. ✅ Root folder detection
6. ✅ Integration with main iTidy flow

### Phase 2: Restore (Required for v1.2)
1. ⏳ Restore single folder
2. ⏳ Restore full run
3. ⏳ Catalog recovery mode

### Phase 3: Polish (v1.3+)
1. ⏳ Backup retention policy
2. ⏳ Statistics and reporting
3. ⏳ Compression level control
4. ⏳ Incremental backups

---

## Summary

This backup system provides:
- ✅ **Filesystem-safe** - Works on FFS, OFS, PFS
- ✅ **Scalable** - Handles 99,999 folders per run
- ✅ **Performant** - Max 100 files per directory
- ✅ **Robust** - Multiple recovery mechanisms
- ✅ **RTC-independent** - Sequential numbering
- ✅ **Human-readable** - Catalog for inspection
- ✅ **Cross-filesystem** - PFS data → FFS backup OK

The design prioritizes **reliability and safety** over simplicity, ensuring that Amiga users can confidently use iTidy knowing their carefully arranged layouts are protected.

---

**Next Steps:**
1. Review and approve this proposal
2. Update `backup_system.h` and `backup_system.c` with hierarchical structure
3. Implement root folder detection
4. Add catalog management functions
5. Integrate with main iTidy processing loop
6. Test on real Amiga hardware (FFS boot, PFS data)

---

*"Never lose a Workbench layout again."*
