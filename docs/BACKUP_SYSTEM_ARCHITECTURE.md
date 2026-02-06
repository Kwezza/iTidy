# iTidy Backup System Architecture

**Version:** 2.0 (ReAction)  
**Last Updated:** February 6, 2026  
**Author:** Kerry Thompson  
**Purpose:** Document the current iTidy backup system architecture for future enhancements

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture Overview](#architecture-overview)
3. [Data Structures](#data-structures)
4. [Backup Process Flow](#backup-process-flow)
5. [Restore Process Flow](#restore-process-flow)
6. [Directory Structure](#directory-structure)
7. [File Format Specifications](#file-format-specifications)
8. [Module Descriptions](#module-descriptions)
9. [Integration Points](#integration-points)
10. [User Interface](#user-interface)
11. [Key Design Decisions](#key-design-decisions)
12. [Known Limitations](#known-limitations)

---

## Overview

iTidy features a comprehensive backup system that creates LHA-compressed archives of icon layout files (`.info` files) before modifying folder layouts. This system provides users with an "undo" mechanism to restore previous icon positions and window snapshots.

### Key Features

- **Automatic Backup Creation**: Creates LHA archives before processing folders
- **Sequential Archive Numbering**: Uses a 5-digit index system (00001-99999)
- **Hierarchical Storage**: Organizes archives in subdirectories (100 files per folder) for FFS performance
- **Session-Based Organization**: Groups backups by "runs" with sequential Run_NNNN directories
- **Path Recovery**: Embeds `_PATH.txt` marker files in archives to enable correct restoration
- **Catalog System**: Maintains `catalog.txt` manifest for each run with metadata and statistics
- **Full Run Restore**: Can restore all folders from a single backup session
- **Individual Archive Restore**: Can restore specific folders
- **Window Geometry Preservation**: Backs up and restores window positions and sizes

### System Requirements

- **LHA Archiver**: Requires `LhA` executable (typically in `C:` or on `PATH`)
- **Workbench Version**: 3.2+ (for ReAction GUI components)
- **Storage Space**: Variable, depends on number of icons and backup retention settings

---

## Architecture Overview

The backup system is organized into **8 specialized modules**, each handling a specific aspect of backup/restore functionality:

```
┌─────────────────────────────────────────────────────────────┐
│                     BACKUP SYSTEM LAYERS                     │
├─────────────────────────────────────────────────────────────┤
│  Layer 4: High-Level API                                     │
│    - backup_session.h/c (Session Manager)                    │
│    - backup_restore.h/c (Restore Manager)                    │
├─────────────────────────────────────────────────────────────┤
│  Layer 3: Subsystem Managers                                 │
│    - backup_catalog.h/c (Catalog Management)                 │
│    - backup_runs.h/c (Run Directory Management)              │
│    - backup_marker.h/c (Path Marker Files)                   │
├─────────────────────────────────────────────────────────────┤
│  Layer 2: Utilities                                          │
│    - backup_lha.h/c (LHA Wrapper)                            │
│    - backup_paths.h/c (Path Utilities)                       │
├─────────────────────────────────────────────────────────────┤
│  Layer 1: Core Types                                         │
│    - backup_types.h (Data Structures & Constants)            │
└─────────────────────────────────────────────────────────────┘
```

### Module Dependencies

```
                    ┌──────────────────┐
                    │  layout_processor│
                    │  (main engine)   │
                    └────────┬─────────┘
                             │
                    ┌────────▼─────────┐
                    │ backup_session   │ ◄───────┐
                    │ (High-Level API) │         │
                    └────────┬─────────┘         │
                             │                   │
           ┌─────────────────┼───────────────┐   │
           │                 │               │   │
    ┌──────▼──────┐   ┌─────▼──────┐  ┌────▼────────┐
    │backup_catalog│   │backup_runs │  │backup_marker│
    └──────┬──────┘   └─────┬──────┘  └────┬────────┘
           │                 │               │
           │         ┌───────┴───────┬───────┘
           │         │               │
       ┌───▼─────────▼───┐     ┌────▼──────┐
       │   backup_lha     │     │backup_paths│
       │  (LHA Wrapper)   │     │ (Utilities)│
       └──────────────────┘     └────────────┘
                   │
            ┌──────▼──────┐
            │backup_types │
            │(Core Types) │
            └─────────────┘
```

---

## Data Structures

### BackupContext (Session State)

Maintains state for an active backup session (defined in `backup_types.h`):

```c
typedef struct {
    /* Run identification */
    UWORD runNumber;                    /* Sequential run number (0001-9999) */
    ULONG archiveIndex;                 /* Next archive number (00001-99999) */
    
    /* Paths */
    char runDirectory[MAX_BACKUP_PATH]; /* Full path to Run_NNNN/ directory */
    char backupRoot[MAX_BACKUP_PATH];   /* Root backup directory */
    char sourceDirectory[MAX_BACKUP_PATH]; /* Source being tidied */
    char lhaPath[32];                   /* Path to LhA executable */
    
    /* Catalog file handle */
    BPTR catalogFile;                   /* AmigaDOS file handle */
    
    /* Statistics */
    ULONG startTime;                    /* Session start time */
    ULONG endTime;                      /* Session end time */
    UWORD foldersBackedUp;              /* Successful backup count */
    UWORD failedBackups;                /* Failed backup count */
    ULONG totalBytesArchived;           /* Total size of archives */
    
    /* Flags */
    BOOL lhaAvailable;                  /* LHA found and usable */
    BOOL catalogOpen;                   /* Catalog file is open */
    BOOL sessionActive;                 /* Session initialized */
} BackupContext;
```

### BackupArchiveEntry (Archive Metadata)

Represents a single backup archive (defined in `backup_types.h`):

```c
typedef struct {
    /* Archive identification */
    ULONG archiveIndex;                 /* Archive number (1-99999) */
    char archiveName[MAX_ARCHIVE_NAME]; /* "00001.lha" */
    char subFolder[MAX_FOLDER_NAME];    /* "000/" */
    
    /* Archive metadata */
    ULONG sizeBytes;                    /* Archive file size */
    ULONG timestamp;                    /* Creation timestamp */
    UWORD iconCount;                    /* Number of .info files */
    
    /* Original path (for restore) */
    char originalPath[MAX_BACKUP_PATH]; /* Full path to original folder */
    
    /* Window geometry */
    WORD windowLeft;                    /* Window X position */
    WORD windowTop;                     /* Window Y position */
    WORD windowWidth;                   /* Window width */
    WORD windowHeight;                  /* Window height */
    UWORD viewMode;                     /* Drawer view mode */
    
    /* Status */
    BOOL successful;                    /* Backup completed successfully */
} BackupArchiveEntry;
```

### BackupPreferences (User Settings)

User-configurable backup settings (embedded in `LayoutPreferences`):

```c
typedef struct {
    BOOL enableUndoBackup;           /* Create backup before processing */
    BOOL useLha;                     /* Use LhA compression */
    char backupRootPath[108];        /* Root backup directory path */
    UWORD maxBackupsPerFolder;       /* Maximum archives to retain */
} BackupPreferences;
```

---

## Backup Process Flow

### 1. User Initiates Backup

**Location:** `src/GUI/main_window.c`

- User checks "Backup layouts" checkbox on main window
- Setting is stored in `LayoutPreferences.backupPrefs.enableUndoBackup`
- Preferences passed to `ProcessDirectoryWithPreferences()` in `layout_processor.c`

### 2. Session Initialization

**Function:** `InitBackupSession()` in `src/backup_session.c`

```
1. Validate backup preferences
2. Check for LhA availability using CheckLhaAvailable()
3. Scan backup root directory for existing runs
4. Create new Run_NNNN directory (sequential numbering)
5. Create catalog.txt file and write header
6. Initialize BackupContext structure
7. Return TRUE if successful, FALSE if LhA not found
```

**Directory Creation Example:**
```
Existing: PROGDIR:Backups/Run_0001/
          PROGDIR:Backups/Run_0002/
Created:  PROGDIR:Backups/Run_0003/
```

### 3. Per-Folder Backup

**Function:** `BackupFolder()` in `src/backup_session.c`

Called once for each folder being processed by iTidy:

```
1. Check if folder contains .info files
   - If no icons, skip backup and return BACKUP_NO_ICONS
   
2. Calculate archive path
   - archiveIndex → folderNum = archiveIndex / 100
   - Example: archive 12345 → PROGDIR:Backups/Run_0003/123/12345.lha
   
3. Create hierarchical subfolder if needed
   - Example: Create "123/" directory
   
4. Detect if path is root folder (e.g., "DH0:")
   - Root folders need special handling for .info file location
   
5. Create LHA archive
   - Root: "LhA a 00001.lha DH0:#?.info"
   - Non-root: "LhA a 00001.lha DH0:Projects/#?.info"
   
6. Create _PATH.txt marker file
   - Contains original folder path for restore
   - Example: "DH0:Projects/MyGame/"
   
7. Add _PATH.txt to archive
   - "LhA a 00001.lha _PATH.txt"
   
8. Write entry to catalog.txt
   - Pipe-delimited format with metadata
   
9. Update BackupContext statistics
   - Increment archiveIndex
   - Update foldersBackedUp counter
   - Add archive size to totalBytesArchived
```

**Archive Numbering Flow:**
```
First folder:   archiveIndex=1  → 000/00001.lha
Second folder:  archiveIndex=2  → 000/00002.lha
...
100th folder:   archiveIndex=100 → 001/00100.lha
101st folder:   archiveIndex=101 → 001/00101.lha
...
12345th folder: archiveIndex=12345 → 123/12345.lha
```

### 4. Session Closure

**Function:** `CloseBackupSession()` in `src/backup_session.c`

```
1. Write catalog footer with statistics:
   - Total archives created
   - Successful vs. failed count
   - Total size
   - Session end timestamp
   
2. Close catalog file handle
3. Mark session inactive
```

---

## Restore Process Flow

### 1. User Opens Restore Window

**Location:** `src/GUI/RestoreBackups/restore_window.c`

- User clicks "Restore backups..." button on main window
- `open_restore_window()` is called
- ReAction window opens with two listbrowsers:
  - **Left:** List of available backup runs
  - **Right:** Details of selected run

### 2. Scan Backup Runs

**Function:** `scan_backup_runs()` in `restore_window.c`

```
1. Find highest run number in backup root
2. Iterate through Run_NNNN directories
3. For each run:
   - Read catalog.txt header
   - Extract run number, date, source directory
   - Parse catalog entries to count folders and total size
   - Extract session status (completed/in progress)
4. Build array of RestoreRunEntry structures
5. Populate left listbrowser with run summaries
```

**Run List Display:**
```
Run  | Date/Time           | Folders | Size    | Status
-----+---------------------+---------+---------+----------
0001 | 2025-10-24 14:30:00 | 5       | 125 KB  | Complete
0002 | 2025-10-25 09:15:23 | 12      | 384 KB  | Complete
0003 | 2025-10-26 16:45:10 | 8       | 256 KB  | Complete
```

### 3. Display Run Details

**Function:** `update_details_list()` in `restore_window.c`

When user selects a run from the left list:

```
1. Clear right listbrowser
2. Read selected run's catalog.txt
3. Extract metadata:
   - Run number
   - Session start/end timestamps
   - Source directory
   - LhA version
   - Statistics (folders backed up, failed, total size)
4. Parse catalog entries to build folder list
5. Populate right listbrowser with field/value pairs
```

**Details Display:**
```
Field            | Value
-----------------+----------------------------------------
Run Number       | 0003
Session Started  | 2025-10-26 16:45:10
Session Ended    | 2025-10-26 16:47:32
Source Directory | DH0:Projects/
Total Folders    | 8
Total Size       | 256 KB
Status           | Complete
```

### 4. Restore Entire Run

**Function:** `handle_restore_run()` in `restore_window.c`

User clicks "Restore Run" button:

```
1. Show confirmation requester:
   - "This will restore ALL folders from Run_NNNN"
   - "Original icon positions will be overwritten"
   
2. Initialize RestoreContext
   - Check for LhA availability
   - Set restore preferences (window geometry, etc.)
   
3. Open recursive progress window
   - Shows current folder being restored
   - Displays progress bar
   
4. Call RestoreFullRun()
   - Parse catalog.txt
   - For each entry:
     a. Build archive path (Run_NNNN/subfolder/archive.lha)
     b. Extract original path from catalog entry
     c. Extract archive to original location
     d. Restore window geometry if enabled
     e. Update progress window
     f. Update statistics
     
5. Display completion requester
   - Show success/failure counts
   - Display first error if any
   
6. Refresh run list to reflect updated backup status
```

### 5. Restore Individual Archive

**Function:** `RestoreSingleArchive()` in `src/backup_restore.c`

```
1. Read _PATH.txt from archive
   - Extract original folder path
   
2. Validate restore path
   - Check for invalid characters
   - Verify path format
   
3. Create destination directory if needed
   - Create parent directories recursively
   
4. Extract archive to destination
   - "LhA x archive.lha destpath/"
   
5. Restore window geometry (optional)
   - Read .info file
   - Apply window position and size from BackupArchiveEntry
```

### 6. Delete Backup Run

**Function:** `handle_delete_run()` in `restore_window.c`

```
1. Show confirmation requester
   - "Delete Run_NNNN and all its archives?"
   - Display folder count and total size
   
2. Delete directory recursively
   - Remove all archive files
   - Remove catalog.txt
   - Remove hierarchical subdirectories
   - Remove Run_NNNN directory
   
3. Refresh run list
```

---

## Directory Structure

### Backup Root Organization

```
PROGDIR:Backups/               (Root backup directory)
├── Run_0001/                  (First backup session)
│   ├── catalog.txt            (Manifest file)
│   ├── 000/                   (Archives 00001-00099)
│   │   ├── 00001.lha          (First folder backup)
│   │   ├── 00002.lha
│   │   └── 00099.lha
│   ├── 001/                   (Archives 00100-00199)
│   │   ├── 00100.lha
│   │   └── 00199.lha
│   └── 123/                   (Archives 12300-12399)
│       └── 12345.lha
├── Run_0002/                  (Second backup session)
│   ├── catalog.txt
│   ├── 000/
│   │   ├── 00001.lha
│   │   └── ...
│   └── ...
└── Run_0003/                  (Third backup session)
    └── ...
```

### Hierarchical Folder Calculation

**Purpose:** Avoid FFS performance degradation with >100 files per directory

**Algorithm:**
```c
folderNum = archiveIndex / 100;
sprintf(folderPath, "%03d/", folderNum);
```

**Examples:**
```
Archive Index → Folder Path
    1        →    000/
   42        →    000/
  100        →    001/
  255        →    002/
12345        →    123/
99999        →    999/
```

---

## File Format Specifications

### catalog.txt Format

**Header Section:**
```
iTidy Backup Catalog v1.0
========================================
Run Number: 0003
Session Started: 2025-10-26 16:45:10
Session Ended: 2025-10-26 16:47:32
Source Directory: DH0:Projects/
LhA Version: LhA 1.38
========================================

# Index    | Subfolder | Size    | Original Path
-----------+-----------+---------+-----------------------------------
```

**Entry Section (Pipe-Delimited):**
```
00001.lha  | 000/      | 15 KB   | DH0:Projects/ClientWork/WebDesign/
00002.lha  | 000/      | 22 KB   | Work:Development/SourceCode/
00042.lha  | 000/      | 8 KB    | DH0:Documents/Letters/
12345.lha  | 123/      | 5 KB    | Work:WHDLoad/Games/Z/Zool/
```

**Footer Section:**
```
========================================
Session Summary:
  Total Archives: 12345
  Successful: 12300
  Failed: 45
  Total Size: 1.2 GB
========================================
```

### _PATH.txt Format

**Purpose:** Embedded in each archive to enable correct restoration

**Content:**
```
DH0:Projects/MyGame/
Archive: 00042
Timestamp: 2025-10-26 16:45:15
```

**Line Breakdown:**
- **Line 1:** Original folder path (absolute AmigaDOS path)
- **Line 2:** Archive index (5-digit number, optional)
- **Line 3:** Backup timestamp (optional)

---

## Module Descriptions

### backup_types.h

**Purpose:** Core type definitions and constants

**Key Contents:**
- `BackupContext` structure
- `BackupArchiveEntry` structure
- `BackupStatus` enumeration
- Path and filename constants
- Validation macros

**No Implementation File:** Header-only module

---

### backup_session.h/c

**Purpose:** High-level session management API

**Key Functions:**
- `InitBackupSession()` - Initialize backup run
- `BackupFolder()` - Backup single folder
- `CloseBackupSession()` - Finalize session
- `FolderHasInfoFiles()` - Check for .info files
- `CountInfoFiles()` - Count .info files

**Usage Pattern:**
```c
BackupContext ctx;
InitBackupSession(&ctx, &prefs, sourceDir);
BackupFolder(&ctx, "DH0:Projects/", 0);
BackupFolder(&ctx, "DH0:Documents/", 0);
CloseBackupSession(&ctx);
```

---

### backup_restore.h/c

**Purpose:** Restore operations

**Key Functions:**
- `InitRestoreContext()` - Initialize restore session
- `RestoreSingleArchive()` - Restore one archive
- `RestoreFullRun()` - Restore entire backup run
- `RestoreArchiveFromPath()` - Restore using _PATH.txt
- `ValidateRestorePath()` - Validate destination path
- `GetArchiveSize()` - Get archive file size

**Usage Pattern:**
```c
RestoreContext rctx;
InitRestoreContext(&rctx);
RestoreFullRun(&rctx, "PROGDIR:Backups/Run_0003");
```

---

### backup_catalog.h/c

**Purpose:** Catalog file management

**Key Functions:**
- `CreateCatalog()` - Create catalog.txt with header
- `AppendCatalogEntry()` - Add entry to catalog
- `CloseCatalog()` - Write footer and close file
- `ParseCatalog()` - Read and parse catalog entries
- `ExtractCatalogMetadata()` - Read header metadata

**File Format:** Pipe-delimited text with header/footer

---

### backup_runs.h/c

**Purpose:** Run directory management

**Key Functions:**
- `FindHighestRunNumber()` - Scan for existing runs
- `CreateNextRunDirectory()` - Create Run_NNNN directory
- `FormatRunDirectoryName()` - Format "Run_NNNN" string
- `ParseRunNumber()` - Extract number from directory name
- `GetRunDirectoryPath()` - Build full run path

**Naming Convention:** `Run_NNNN` (4-digit zero-padded)

---

### backup_marker.h/c

**Purpose:** Path marker file management

**Key Functions:**
- `CreatePathMarkerFile()` - Create _PATH.txt file
- `CreateTempPathMarker()` - Create marker in temp location
- `ReadPathMarkerFile()` - Read marker from file
- `ExtractPathFromArchive()` - Extract and read _PATH.txt
- `GetTempDirectory()` - Get platform temp directory

**File Format:** Plain text with original path on line 1

---

### backup_lha.h/c

**Purpose:** LHA archiver interface

**Key Functions:**
- `CheckLhaAvailable()` - Detect LhA executable
- `GetLhaVersion()` - Query LhA version string
- `CreateLhaArchive()` - Create archive from directory
- `AddFileToArchive()` - Add file to existing archive
- `ExtractLhaArchive()` - Extract archive to destination
- `ListArchiveContents()` - List files in archive

**Platform Support:** Amiga (Execute) and Host (system)

**Root Folder Handling:**
```c
// Root folders: "SYS:#?.info" (all icons in root)
CreateLhaArchive(lha, "archive.lha", "SYS:", TRUE);

// Subdirectories: "SYS:Programs/#?.info"
CreateLhaArchive(lha, "archive.lha", "SYS:Programs/", FALSE);
```

---

### backup_paths.h/c

**Purpose:** Path manipulation utilities

**Key Functions:**
- `IsRootFolder()` - Detect root/volume paths
- `CalculateArchivePath()` - Build hierarchical archive path
- `CalculateSubfolderPath()` - Calculate subfolder path
- `FormatArchiveFilename()` - Format "NNNNN.lha"
- `ValidateBackupPath()` - Check path length/validity
- `NormalizePath()` - Normalize AmigaDOS path

**Root Detection:**
```c
IsRootFolder("DH0:")           → TRUE
IsRootFolder("Work:")          → TRUE
IsRootFolder("DH0:Projects/")  → FALSE
```

---

## Integration Points

### 1. Main GUI Window

**File:** `src/GUI/main_window.c`

**Integration:**
- Checkbox gadget: "Backup layouts"
- Button gadget: "Restore backups..."
- Reads/writes `LayoutPreferences.backupPrefs.enableUndoBackup`

**Code Locations:**
```c
// Line ~404: Initialize backup flag
win_data->enable_backup = FALSE;

// Line ~646: Create backup checkbox
LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_BACKUP_CHECKBOX]

// Line ~1080: Load backup preference
win_data->enable_backup = prefs->enable_backup;

// Line ~1175: Save backup preference
prefs->enable_backup = win_data->enable_backup;
prefs->backupPrefs.enableUndoBackup = win_data->enable_backup;
```

### 2. Layout Processor

**File:** `src/layout_processor.c`

**Integration:**
- Initializes backup session before processing
- Calls `BackupFolder()` for each directory
- Closes session after processing complete

**Code Flow:**
```c
// Line ~471: Declare local context
BackupContext localContext;

// Line ~610: Initialize if enabled
if (prefs->backupPrefs.enableUndoBackup) {
    InitBackupSession(&localContext, &prefs->backupPrefs, rootPath);
    g_backupContext = &localContext;
}

// Per-folder processing:
if (g_backupContext && g_backupContext->sessionActive) {
    BackupFolder(g_backupContext, folderPath, iconCount);
}

// After processing:
if (g_backupContext) {
    CloseBackupSession(g_backupContext);
}
```

### 3. Restore Window

**File:** `src/GUI/RestoreBackups/restore_window.c`

**Integration:**
- Scans backup directory for runs
- Displays run list in listbrowser
- Shows run details in second listbrowser
- Handles restore/delete operations
- Uses recursive progress window for feedback

**Key Functions:**
```c
// Line ~362: Scan backup runs
scan_backup_runs(backup_root, &entries)

// Line ~500+: Handle restore button
handle_restore_run(win_data)

// Line ~700+: Handle delete button
handle_delete_run(win_data)
```

---

## User Interface

### Main Window

**Backup Control:**
```
┌─────────────────────────────────────────┐
│ iTidy - Tidy Desktop Icons              │
├─────────────────────────────────────────┤
│                                         │
│ [✓] Backup layouts                      │
│     Creates an LhA backup of .info      │
│     files before changes.               │
│                                         │
│ [Restore backups...]                    │
│                                         │
│ [Apply]  [Cancel]                       │
└─────────────────────────────────────────┘
```

### Restore Window

**Two-Panel Layout:**
```
┌──────────────────────────────────────────────────────────────┐
│ iTidy - Restore Backups                                      │
├──────────────────────────────────────────────────────────────┤
│ ┌──────────────────────┬──────────────────────────────────┐  │
│ │ Run List             │ Details                          │  │
│ ├──────────────────────┼──────────────────────────────────┤  │
│ │ 0001 | 2025-10-24    │ Run Number: 0003                 │  │
│ │ 0002 | 2025-10-25    │ Session Started: 2025-10-26...   │  │
│ │►0003 | 2025-10-26    │ Source Directory: DH0:Projects/  │  │
│ │                      │ Total Folders: 8                 │  │
│ │                      │ Total Size: 256 KB               │  │
│ └──────────────────────┴──────────────────────────────────┘  │
│                                                              │
│ [Delete Run] [Restore Run] [View Folders] [Cancel]          │
└──────────────────────────────────────────────────────────────┘
```

### Restore Progress Window

**During Restore Operation:**
```
┌──────────────────────────────────────────┐
│ Restoring Backup Run 0003                │
├──────────────────────────────────────────┤
│ Current Folder:                          │
│ DH0:Projects/ClientWork/WebDesign/       │
│                                          │
│ [████████████░░░░░░░░░] 62%              │
│                                          │
│ Folder 5 of 8                            │
│                                          │
│ [Cancel]                                 │
└──────────────────────────────────────────┘
```

---

## Key Design Decisions

### 1. Sequential Archive Numbering

**Decision:** Use global sequential numbering (00001-99999) instead of per-folder numbering

**Rationale:**
- Simplifies catalog management (single sequence)
- Prevents collisions when backing up multiple folders
- Makes archive ordering unambiguous
- Supports up to 99,999 folders per run

**Tradeoff:** Archive numbers are not reused, limiting each run to 99,999 folders

### 2. Hierarchical Storage (100 Files Per Folder)

**Decision:** Split archives into subdirectories with 100 files each

**Rationale:**
- Avoids FFS performance degradation with large directories
- AmigaOS FFS slows significantly with >100 files per directory
- Maintains reasonable directory traversal times
- Compatible with FFS block size limits

**Implementation:** `folderNum = archiveIndex / 100`

### 3. Pipe-Delimited Catalog Format

**Decision:** Use pipe-delimited text instead of binary format

**Rationale:**
- Human-readable for debugging and manual inspection
- Easy to parse with simple string operations
- Platform-independent (no endianness issues)
- Can be edited manually if needed
- Compatible with standard text tools

**Tradeoff:** Slightly larger file size than binary format

### 4. Embedded Path Markers

**Decision:** Store original path inside each archive as `_PATH.txt`

**Rationale:**
- Enables recovery even if catalog.txt is lost or corrupted
- Archives are self-contained and relocatable
- Allows restore without catalog lookup
- Provides redundancy for critical data

**Tradeoff:** Small storage overhead (~100 bytes per archive)

### 5. Session-Based Organization

**Decision:** Group backups by "runs" (backup sessions)

**Rationale:**
- Logical grouping of related backups
- Enables "undo entire tidy session" functionality
- Makes cleanup easier (delete entire run)
- Provides clear audit trail

**Tradeoff:** More complex directory structure

### 6. LhA Dependency

**Decision:** Require external LhA executable

**Rationale:**
- LhA is standard Amiga archiver (widely available)
- Good compression ratio (saves disk space)
- Fast compression/decompression
- Avoids reimplementing compression algorithms
- Maintains compatibility with existing Amiga tools

**Tradeoff:** Backup feature unavailable if LhA not installed

### 7. Window Geometry Preservation

**Decision:** Store window position/size in catalog entries

**Rationale:**
- Users expect window snapshots to be restored
- Drawer .info files contain window geometry
- Provides complete "undo" experience
- Minimal storage overhead

**Implementation:** Extract from folder's .info file during backup, restore to .info during restore

---

## Known Limitations

### 1. Archive Count Limit

**Limitation:** Maximum 99,999 archives per run

**Reason:** 5-digit archive numbering

**Workaround:** Start new backup run (automatically happens on next tidy operation)

**Impact:** Minimal - extremely rare to process 99,999 folders in one session

### 2. Run Number Limit

**Limitation:** Maximum 9,999 backup runs

**Reason:** 4-digit run numbering (Run_NNNN)

**Workaround:** Manually delete old runs or move backup directory

**Impact:** Low - represents thousands of backup sessions

### 3. LhA Dependency

**Limitation:** Backup unavailable if LhA not installed

**Reason:** External dependency for compression

**Workaround:** User must install LhA (C: or PATH)

**Detection:** `CheckLhaAvailable()` verifies presence at session init

### 4. No Compression Level Customization

**Limitation:** Uses fixed compression level (`-m1`)

**Reason:** Balance between speed and size

**Workaround:** None currently

**Future Enhancement:** Could add preference setting

### 5. No Archive Encryption

**Limitation:** Archives not encrypted or password-protected

**Reason:** Standard LhA doesn't support encryption well

**Security:** Backups stored in user-accessible location

**Mitigation:** Relies on file system permissions

### 6. Single Backup Root

**Limitation:** All backups stored in one root directory

**Reason:** Simplified management and lookup

**Workaround:** User can change root path in preferences

**Impact:** May fill up single volume if many backups

### 7. No Automatic Cleanup

**Limitation:** Old runs not automatically deleted

**Reason:** User control over backup retention

**Workaround:** Manual deletion via Restore Window or file system

**Future Enhancement:** Could implement automatic pruning based on age/count

### 8. No Incremental Backups

**Limitation:** Each backup is full copy of all .info files

**Reason:** Simpler implementation and restore logic

**Tradeoff:** More storage space vs. complexity

**Benefit:** Individual archives are self-contained

### 9. No Backup Verification

**Limitation:** No checksum verification after backup creation

**Reason:** Performance considerations

**Risk:** Corrupted archives not detected until restore

**Mitigation:** LhA has built-in CRC checking

### 10. Root Folder Icon Handling

**Limitation:** Root folder .info files require special handling

**Reason:** AmigaDOS stores root drawer info differently

**Implementation:** `IsRootFolder()` detection in `backup_paths.c`

**Pattern:** "DH0:#?.info" vs "DH0:Dir/#?.info"

---

## Future Enhancement Opportunities

### Potential Improvements

1. **Incremental Backups**
   - Only backup changed .info files
   - Requires timestamp tracking
   - Reduces storage space

2. **Automatic Cleanup**
   - Delete runs older than N days
   - Keep only last N runs per folder
   - User-configurable retention policy

3. **Compression Level Selection**
   - User preference for speed vs. size
   - Options: -m0 (store), -m1 (medium), -m2 (max)

4. **Backup Comparison Tool**
   - Compare two backup runs
   - Show differences in icon positions
   - Highlight layout changes

5. **Selective Restore**
   - Choose specific folders from a run
   - Multi-select in restore window
   - Partial run restoration

6. **Backup Annotations**
   - User notes for each run
   - Stored in catalog.txt header
   - Helps identify important backups

7. **Export/Import Archives**
   - Export run to external media
   - Import run from another system
   - Facilitates backup migration

8. **Backup Statistics Dashboard**
   - Total storage used
   - Oldest/newest backups
   - Most frequently backed up folders
   - Compression ratios

9. **Backup Preview**
   - Show icon positions before restore
   - Visual comparison of current vs. backup layout
   - Confirm before overwriting

10. **Alternative Compression**
    - Support for xz, gzip, or zip formats
    - Fallback if LhA not available
    - Wider compatibility

---

## Conclusion

The iTidy backup system provides a robust, hierarchical solution for preserving icon layouts and enabling undo functionality. Its modular architecture allows for easy enhancement and maintenance, while the session-based organization provides logical grouping and audit trails.

The system's design balances performance (hierarchical storage), reliability (embedded path markers), and usability (human-readable catalogs) to create a comprehensive backup solution tailored to AmigaOS's unique requirements.

---

## Document History

| Version | Date       | Author          | Changes                          |
|---------|------------|-----------------|----------------------------------|
| 1.0     | 2026-02-06 | Kerry Thompson  | Initial architecture document    |

---

**End of Document**
