# BACKUP_IMPLEMENTATION_GUIDE.md
**Target:** iTidy v1.2+  
**Purpose:** Step-by-step implementation plan for the new hierarchical backup and restore system  
**Author:** Kerry Thompson  
**Status:** Implementation Guide (AI Agent + Human Developer)  
**Date:** 23 Oct 2025  

---

## 📘 Overview
This guide converts the design in **BACKUP_SYSTEM_PROPOSAL.md** into an actionable build plan.  
Each task includes:

| Field | Meaning |
|--------|----------|
| **Goal** | What this step implements |
| **Files** | Source/header pair to create or modify |
| **Dependencies** | Tasks that must exist first |
| **Deliverables** | Functions, structs, constants |
| **Compile Check** | What should compile cleanly |
| **Unit / CLI Test** | Manual or scripted test |

All new source files reside in `src/backups/`.  
All names follow lower-case with underscore convention.

---

## 💾 Development Environment Notes (Host-Side Testing)

### Host Development Setup
The backup system can be developed and tested directly on the host PC before integration with the full Amiga build.

| Component | Description |
|------------|--------------|
| **Workbench/ directory** | Full copy of your Amiga Workbench hard-disk installation, located in the project workspace. All tests will read and write inside this tree. |
| **LhA executable** | Available in the host system PATH, providing compatible behavior with Amiga’s `C:LhA` tool for compression and extraction. |
| **Cross-compiler** | VBCC or GCC configured for Amiga target (68k). |
| **Testing path base** | `./Workbench/` is treated as equivalent to `DH0:` when testing on the host. |

---

### How the Host-Based Tests Work
During Phases 1–8, modules can be compiled and executed as standard C programs on the host PC:

```bash
# Example host test
./iTidyBackupCLI ./Workbench/Projects/MyApp
```

The code will:
1. Detect `LhA` via `CheckLhaAvailable()`.
2. Treat relative paths under `./Workbench/` as Amiga-style device roots (e.g. “DH0:” → `./Workbench/`).
3. Create archives and catalog files in:
   ```
   ./Workbench/PROGDIR/Backups/Run_0001/
   ```
4. Log activity to:
   ```
   ./Workbench/PROGDIR/Backups/iTidyBackup.log
   ```

When ported back to real Amiga hardware, all relative paths automatically map to `PROGDIR:` or `DH0:`.

---

### Host/Amiga Compatibility Rules
- Use **POSIX I/O** wrappers in the host harness (`fopen`, `mkdir`, etc.) but keep Amiga-compatible path lengths (≤107 chars).
- The `SystemTagList()` calls in production code can be replaced by standard `system()` when compiled for host testing (`#ifdef HOST_BUILD`).
- Maintain file naming in uppercase/lowercase exactly as AmigaDOS would output (`Run_0001`, `_PATH.txt`, `.lha`).

---

### Sample Host Build Commands
```bash
# Build test harness and backup modules
make backups_core
make iTidyBackupCLI

# Run host tests against Workbench copy
./iTidyBackupCLI ./Workbench/DH0:Projects/MyApp

# Inspect results
ls ./Workbench/PROGDIR/Backups/Run_0001/000/
cat ./Workbench/PROGDIR/Backups/Run_0001/catalog.txt
```

---

### When Switching to Real Amiga Testing
After verifying functionality under the host workspace:
1. Copy the `src/backups/` object files and binary to your Amiga development disk.  
2. Replace `system()` stubs with `SystemTagList()` in `backup_lha.c`.  
3. Run the same commands on Workbench; output paths remain valid.

---

## 🧱 Phase 1 – Core Foundation

### **Task 1 – Data Structures**
**Goal:** Define the shared backup/restore data types.  
**Files:** `backup_types.h`  
**Dependencies:** None  

**Deliverables**
```c
typedef enum {
    BACKUP_OK = 0,
    BACKUP_FAIL,
    BACKUP_DISKFULL,
    BACKUP_LHA_MISSING
} BackupStatus;

typedef struct {
    UWORD runNumber;
    UWORD archiveIndex;
    BPTR  catalogFH;
    char  runPath[108];
} BackupContext;

typedef struct {
    char archiveName[32];
    char subFolder[8];
    ULONG sizeBytes;
    char originalPath[108];
} BackupArchiveEntry;
```

**Compile Check:**  
Include in a dummy C file; ensure `sizeof(BackupContext)` < 256 bytes.  

**Unit Test:**  
```
#include "backup_types.h"
printf("Context size=%lu\n", sizeof(BackupContext));
```

---

### **Task 2 – Path Utilities & Root Detection**
**Goal:** Safely detect root volumes and calculate archive paths.  
**Files:** `backup_paths.c`, `backup_paths.h`  
**Dependencies:** Task 1  

**Deliverables**
```c
BOOL  IsRootFolder(const char *path);
void  SanitizePath(const char *in, char *out, int maxLen);
void  CalculateArchivePath(char *outPath, const char *runDir, UWORD index);
```

**Compile Check:** link-only test with dummy main.  
**CLI Test:**  
```
iTidyBackupCLI --test-paths DH0:Projects/MyApp
```

---

### **Task 3 – Run Number Management**
**Goal:** Generate and maintain sequential Run_NNNN directories.  
**Files:** `backup_runs.c`, `backup_runs.h`  
**Dependencies:** Tasks 1-2  

**Deliverables**
```c
UWORD FindHighestRunNumber(const char *root);
BOOL  CreateNextRunDirectory(const char *root, char *outPath, UWORD *outRun);
```

**Compile Check:** Confirm creates folders under `PROGDIR:Backups/`.  
**CLI Test:**  
```
iTidyBackupCLI --newrun
```

---

## 📦 Phase 2 – Backup Operations

### **Task 4 – Catalog Management**
**Goal:** Write and read human-readable catalog.txt.  
**Files:** `backup_catalog.c`, `backup_catalog.h`  
**Dependencies:** Tasks 1-3  

**Deliverables**
```c
BOOL CreateCatalog(BackupContext *ctx);
BOOL AppendCatalogEntry(BackupContext *ctx, const BackupArchiveEntry *entry);
BOOL CloseCatalog(BackupContext *ctx);
BOOL ParseCatalog(const char *catalogPath, void (*callback)(BackupArchiveEntry*));
```

**Compile Check:** Link with no missing symbols.  
**Unit Test:**  
Run and verify that `catalog.txt` contains headers and entries separated by `|`.

---

### **Task 5 – LhA Execution Wrapper**
**Goal:** Run LhA safely using AmigaDOS SystemTagList().  
**Files:** `backup_lha.c`, `backup_lha.h`  
**Dependencies:** Tasks 1-2  

**Deliverables**
```c
BOOL  CheckLhaAvailable(void);
LONG  ExecuteLhaCommand(const char *cmd);
BOOL  CreateArchive(const char *archivePath, const char *sourcePattern);
BOOL  ExtractArchive(const char *archivePath, const char *targetPath);
```

**Compile Check:** Requires `dos/dos.h` include.  
**Unit Test:**  
```
iTidyBackupCLI --test-lha Work:Test/
```
Expect LhA return = 0.

---

### **Task 6 – Path Marker File (_PATH.txt)**
**Goal:** Store original path metadata inside each archive.  
**Files:** `backup_marker.c`, `backup_marker.h`  
**Dependencies:** Tasks 1-2  

**Deliverables**
```c
BOOL CreatePathMarkerFile(const char *destPath, const char *origPath,
                          UWORD run, UWORD index);
BOOL ReadPathMarkerFile(const char *srcPath, char *outOrigPath);
```

**CLI Test:** Check contents of `_PATH.txt` for correct metadata.

---

## 🧭 Phase 3 – Session and Restore Engine

### **Task 7 – Backup Session Manager**
**Goal:** Control a complete backup run.  
**Files:** `backup_session.c`, `backup_session.h`  
**Dependencies:** Tasks 1-6  

**Deliverables**
```c
BOOL InitBackupSession(BackupContext *ctx, const BackupPreferences *prefs);
BOOL BackupFolder(BackupContext *ctx, const char *folderPath);
void CloseBackupSession(BackupContext *ctx);
```

**Compile Check:** Ensure all helper functions resolve.  
**Integration Test:**  
```
iTidyBackupCLI DH0:Projects/MyApp
```
Expect creation of `Backups/Run_0001/000/00001.lha`.

---

### **Task 8 – Restore Operations**
**Goal:** Recreate folders from stored archives.  
**Files:** `backup_restore.c`, `backup_restore.h`  
**Dependencies:** Tasks 1-7  

**Deliverables**
```c
BOOL RestoreSingleFolder(const char *archivePath, const char *targetPath);
BOOL RestoreFullRun(UWORD runNumber);
BOOL OrphanedArchiveRecovery(const char *runDir);
```

**CLI Test:**  
```
iTidyBackupCLI --restore-run 1
```
Verify folders and icons restored.

---

## 💡 Phase 4 – Integration and Error Handling

### **Task 9 – Integration with iTidy Core**
**Goal:** Hook backup calls into existing directory processing.  
**Files:** `file_directory_handling.c`, `layout_processor.c`  
**Dependencies:** Tasks 1-8  

Insert before icon processing:
```c
if (layoutPrefs.backupPrefs.enableUndoBackup)
    BackupFolder(g_backupContext, currentFolderPath);
```

**Compile Check:** full iTidy build succeeds.  

---

### **Task 10 – Logging and Error Handling**
**Goal:** Route all backup errors through `writeLog.c`.  
**Files:** `backup_session.c`, `writeLog.c`  
**Dependencies:** Task 9  

**Deliverables**
- Disk-full detection (`IoErr()==ERROR_DISK_FULL`)
- LhA missing warnings  
- Append failure messages to `iTidy.log`

---

## 🖥 Phase 5 – GUI Extension (Workbench 3.x)

### **Task 11 – Restore Window GUI**
**Goal:** User interface for choosing runs/folders to restore.  
**Files:** `restore_window.c`, `restore_window.h`  
**Dependencies:** Task 8  

**Core Functions**
```c
BOOL OpenRestoreWindow(void);
void BuildRestoreListView(struct List *lv, UWORD runNumber);
void HandleRestoreGadgets(UWORD gid);
```

**Widgets**
- Cycle gadget: backup run selector  
- ListView: indented folder list  
- Checkbox: “Include Subfolders”  
- Buttons: Restore Selected, Restore All, Cancel  

---

## 🧪 Phase 6 – Testing and Documentation

### **Task 12 – Automated Tests**
**Goal:** Verify all scenarios.  
**Folder:** `tests/backups/`

| Script | Purpose |
|---------|----------|
| `test_singlefolder.sh` | One-folder backup + restore |
| `test_rootfolder.sh` | DH0: root backup |
| `test_huge_tree.sh` | 1000+ folders performance |
| `test_catalog_missing.sh` | Orphan recovery |
| `test_lha_missing.sh` | Error fallback |

---

### **Task 13 – Documentation & Help**
**Goal:** Update docs and CLI help.  
**Files:** `docs/BACKUP_SYSTEM_IMPLEMENTATION.md`, `docs/RESTORE_WINDOW_SPEC.md`  
Include usage, known limits, troubleshooting.

---

## 🧩 Build & Test Pipeline

| Stage | Command | Result |
|--------|----------|---------|
| 1 | `make backups_core` | builds Tasks 1–6 |
| 2 | `make backups_restore` | builds Tasks 7–8 |
| 3 | `make iTidyBackupCLI` | test driver |
| 4 | Run test scripts | confirm outputs |
| 5 | `make iTidy` | integrate full program |

---

## ⚙️ Agent Execution Rules

When executed under an AI Agent in VS Code:

1. **Always compile after each Task**  
   → halt on warnings treated as errors.  
2. **Never alter existing iTidy functions** without explicit approval comment (`/* BACKUP-HOOK */`).  
3. **Use AmigaDOS APIs only** (`Lock()`, `Examine()`, `SystemTagList()`).  
4. **Output logs** to `C:Temp/iTidyBackup.log`.  
5. **Commit after every successful task** using git tag `backup_taskNN_done`.

---

## ✅ Completion Checklist

| Task | Description | Status |
|------|--------------|--------|
| 1 | Core structs | ☐ |
| 2 | Path utilities | ☐ |
| 3 | Run management | ☐ |
| 4 | Catalog system | ☐ |
| 5 | LhA wrapper | ☐ |
| 6 | Path marker | ☐ |
| 7 | Session manager | ☐ |
| 8 | Restore engine | ☐ |
| 9 | iTidy integration | ☐ |
|10 | Logging/error handling | ☐ |
|11 | GUI restore window | ☐ |
|12 | Tests | ☐ |
|13 | Documentation | ☐ |

---

### Final Goal
When all tasks pass tests, iTidy gains:
- Hierarchical, FFS-safe backups  
- Seamless undo/restore  
- GUI browser for full or selective recovery  

**Milestone version:** *iTidy v1.2 (Workbench Backup Edition)*  
