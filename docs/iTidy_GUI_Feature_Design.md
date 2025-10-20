# iTidy GUI Feature Design - Unified Layout, Aspect Ratio, and Stretch Goals

**Date:** October 20, 2025  
**Status:** Design Document (Consolidated & Updated)  
**Target:** Workbench 3.0+ Classic Intuition GUI  
**Author:** Kerry Thompson

---

## Executive Summary

iTidy is evolving from a CLI-only Workbench tidying utility into a **Workbench-native GUI application** that intelligently arranges icons, resizes folder windows, and maintains visual consistency across systems ranging from PAL Workbench to RTG setups.  

This document merges the prior design work from:
- **COLUMN_LAYOUT_AND_SORTING_FEATURE.md**  
- **WINDOW_LAYOUT_ASPECT_RATIO_FEATURE.md**  

and incorporates new forward-looking **stretch goals** that prepare for future releases (automatic icon upgrading, default tool validation, and safe undo backups).

---

## Table of Contents
1. [Core Purpose](#core-purpose)
2. [Layout & Sorting System](#layout--sorting-system)
3. [Window Aspect Ratio Logic](#window-aspect-ratio-logic)
4. [Backup & Undo Framework](#backup--undo-framework)
5. [Stretch Goals](#stretch-goals)
6. [Updated LayoutPreferences Structure](#updated-layoutpreferences-structure)
7. [GUI Design Summary](#gui-design-summary)
8. [Implementation Roadmap](#implementation-roadmap)

---

## Core Purpose

iTidy’s GUI expands beyond Workbench 3.2’s built-in “Clean Up” by offering:
- Full control over **layout direction, sorting, and aspect ratio**
- **Recursive** subfolder processing (batch mode)
- **Presets** for common layouts (Classic, Compact, WHDLoad, etc.)
- **Undo/Backup** before changes
- **Future extensibility** for automatic icon upgrades and tool corrections

The goal is to provide a **Workbench-native, user-friendly interface** with automation power normally reserved for CLI tools.

---

## Layout & Sorting System

### Overview

Icons can be arranged in either **row-major** or **column-major** order, with full control over how folders and files are sorted.

```c
typedef enum {
    LAYOUT_MODE_ROW = 0,      /* Horizontal-first */
    LAYOUT_MODE_COLUMN = 1    /* Vertical-first */
} LayoutMode;

typedef enum {
    SORT_ORDER_HORIZONTAL = 0,
    SORT_ORDER_VERTICAL = 1
} SortOrder;

typedef enum {
    SORT_PRIORITY_FOLDERS_FIRST = 0,
    SORT_PRIORITY_FILES_FIRST = 1,
    SORT_PRIORITY_MIXED = 2
} SortPriority;

typedef enum {
    SORT_BY_NAME = 0,
    SORT_BY_TYPE = 1,
    SORT_BY_DATE = 2,
    SORT_BY_SIZE = 3
} SortBy;
```

### Features
- Horizontal and vertical sort modes
- Folders-first, files-first, or mixed
- Sorting by name, type, date, or size
- Adjustable per-column width optimization
- Centering toggle for uniform layouts

### Default Presets
| Preset | Layout | Sort | Folder Order | Use Case |
|--------|---------|-------|---------------|-----------|
| Classic | Row | Horizontal | Folders First | Default Workbench look |
| Compact | Column | Vertical | Folders First | Tight, RTG-friendly layout |
| Modern | Row | Horizontal | Mixed | No folder priority |
| WHDLoad | Column | Vertical | Folders First | Handles long filenames |

---

## Window Aspect Ratio Logic

### Problem
Classic Workbench assumes a fixed 320px target window width. On RTG screens this results in *ultra-wide single-row windows*. Conversely, on PAL 640x256 displays, windows are too narrow.

### Solution
Implement a **resolution-aware width calculation** and **maximum icons per row limit**:

```c
int user_maxIconsPerRow;
float user_aspectRatio;
int user_maxWidthPercent;   // Percentage of screen width
```

### Dynamic Width Scaling (Adaptive Percentage)
Automatically adjust the maximum window width percentage based on detected screen resolution:

```c
if (screenWidth <= 640)
    layoutPrefs.maxWindowWidthPct = 70;   // PAL/NTSC classic
else if (screenWidth <= 1024)
    layoutPrefs.maxWindowWidthPct = 55;   // AGA/mid-RTG
else
    layoutPrefs.maxWindowWidthPct = 40;   // High-res RTG
```

Users may override this via the GUI (Auto, Compact, Balanced, Wide, Full).

### Layout Algorithm (Excerpt)
```c
while (windowWidth < maxWindowWidth && 
       iconsPerRow < layoutPrefs.maxIconsPerRow &&
       (totalIcons / iconsPerRow) > (iconsPerRow * layoutPrefs.aspectRatio))
{
    iconsPerRow++;
    windowWidth = ICON_START_X + iconsPerRow * (largestIconWidth + iconSpacingX);
}
```

---

## Backup & Undo Framework

### Purpose
Amiga users often meticulously hand-arrange their Workbench. iTidy must never permanently alter that layout without offering rollback.

### Implementation
- **Per-folder `.lha` backup** created *before* tidying.
- Stored under `Work:iTidyBackups/Run_YYYY-MM-DD_HH-MM/`
- One archive per folder (e.g., `DH0_Projects_2025-10-20_21-00.lha`)
- Contains all `.info` files and the folder’s drawer icon (window state)

**Example Command:**
```c
C:LhA a -q -r -m1 "Work:iTidyBackups/Run_2025-10-20_21-00/DH0_Projects_2025-10-20_21-00.lha" "DH0:Projects/#?.info"
```

### Restore Example
```bash
lha x -r Work:iTidyBackups/Run_2025-10-20_21-00/DH0_Projects_2025-10-20_21-00.lha DH0:Projects/
```

**Detected via:**
```c
BOOL LhaAvailable(void) { return (Open("C:LhA", MODE_OLDFILE) != NULL); }
```

If `C:LhA` is not found, backup options are greyed out in GUI.

### BackupPreferences Structure
```c
typedef struct {
    BOOL enableUndoBackup;
    BOOL useLha;
    char backupRootPath[108];
    UWORD maxBackupsPerFolder;
} BackupPreferences;
```

---

## Stretch Goals
Stretch goals will not be in the initial release but the architecture must anticipate them.

### 1. Icon Upgrade Framework
Many Amiga applications install outdated icons (Workbench 1.3/2.x/3.0-era drawers, or old ReadMe icons). The stretch goal is to detect and replace these with the modern default icons from the user’s current Workbench or a chosen icon set.

**Concept:**
- Create CRC32 fingerprints of known system drawer and document icons from older Workbench releases.
- Maintain an editable signature map (CSV or INI format):
  ```
  # Old icon CRC → Replacement key
  0xA1B2C3D4,DEFAULT_DRAWER
  0xCC77A012,DEFAULT_README
  ```
- Users specify replacement source directory (e.g., `SYS:Prefs/Env-Archive/Sys/`).
- Replacement executed **after backup, before tidy**.
- WHDLoad folders are skipped unless explicitly enabled (`-iconUpgradeWHD`).

**Future API Stubs:**
```c
BOOL LoadIconMap(const char *path);
BOOL FindReplacement(const IconSignature *sig, char outKey[32]);
BOOL ReplaceIconFile(const char *targetPath, const char *replacementPath);
```

### 2. Default Tool Validation
Classic icons often reference obsolete tools (e.g., `More` instead of `MultiView`).  
Future versions will scan `.info` files, read `DefaultTool`, and verify that it exists in the current Workbench installation.

If invalid, iTidy will update it to a modern equivalent based on a lookup table:
```
more → multiview
ed → c:ed
view → multiview
```
This feature will leverage the same signature database as icon upgrades.

### 3. Enhanced Undo / Version History
Future versions may track multiple backups per folder and allow browsing through a GUI-based Undo Manager:
```
Work:iTidyBackups/
 ├── Run_2025-10-20_21-00/
 ├── Run_2025-10-27_21-00/
 └── Run_2025-11-03_21-00/
```
Users can restore any snapshot by selecting a timestamp.

---

## Updated LayoutPreferences Structure

### Unified Structure
This version consolidates sorting, layout, and window management preferences, ready for GUI serialization and ENV: persistence.

```c
typedef struct {
    LayoutMode layoutMode;          /* Row-major or column-major */
    SortOrder sortOrder;            /* Horizontal or vertical */
    SortPriority sortPriority;      /* Folder/file priority */
    SortBy sortBy;                  /* Sort criteria */
    BOOL reverseSort;               /* Reverse order toggle */
    BOOL centerIconsInColumn;       /* Center icons */
    BOOL useColumnWidthOptimization;/* Per-column sizing */
    BOOL resizeWindows;             /* Auto-resize drawer windows */
    UWORD maxIconsPerRow;           /* Limit columns */
    UWORD maxWindowWidthPct;        /* Auto-scaling width percentage */
    float aspectRatio;              /* Target shape ratio */
    BackupPreferences backupPrefs;  /* Embedded backup settings */
} LayoutPreferences;
```

---

## GUI Design Summary

### Main Window Layout (Workbench GadTools / Classic Intuition)
```
╔═══════════════════════════════════════════════════╗
║                     iTidy v1.2                    ║
╠═══════════════════════════════════════════════════╣
║ Folder: [DH0:Projects__________] [Browse...]     ║
║                                                   ║
║  Preset: (Classic • Compact • Modern • WHDLoad)   ║
║                                                   ║
║  Layout: [Row ▼]  Sort: [Horizontal ▼]            ║
║  Order: [Folders First ▼]  By: [Name ▼]           ║
║  [✓] Center icons  [✓] Optimize columns           ║
║                                                   ║
║  [✓] Recursive Subfolders                         ║
║  [✓] Backup (LHA)                                 ║
║  [ ] Icon Upgrade (future)                        ║
║                                                   ║
║  [Advanced...]  [Apply]  [Cancel]                 ║
╚═══════════════════════════════════════════════════╝
```

### Advanced Window
```
╔═══════════════════════════════════════════════════╗
║                 Advanced Settings                 ║
╠═══════════════════════════════════════════════════╣
║ Aspect Ratio: [Auto ▼]                            ║
║ Max Width: [Auto ▼] (40–80%)                      ║
║ Max Icons/Row: [10 ]                              ║
║ Resize Windows: [✓]                               ║
║                                                   ║
║ Backup Folder: [Work:iTidyBackups/]               ║
║ Retention: [3] previous runs                      ║
╚═══════════════════════════════════════════════════╝
```

---

## Implementation Roadmap

| Phase | Description |
|--------|--------------|
| **1. Core GUI** | Implement main window using Intuition GadTools; add presets and layout preferences. |
| **2. Sorting & Layout Integration** | Replace old `CompareByFolderAndName()` with new `CompareIcons()` using LayoutPreferences. |
| **3. Aspect Ratio Handling** | Add adaptive `maxWindowWidthPct` calculation; implement aspect ratio loop logic. |
| **4. LhA Backup** | Integrate per-folder `.lha` creation with manifest logging. |
| **5. Undo CLI** | Add `-undo` to restore from latest backup. |
| **6. Stretch Hooks** | Stub in functions for Icon Upgrade and Tool Validation. |
| **7. Testing** | PAL, NTSC, RTG screens; WHDLoad collections; large directory trees. |
| **8. Documentation** | Update README, add GUI usage examples and troubleshooting. |

---

## Summary
This unified design positions iTidy as a powerful, future-proof Workbench tool. It merges robust icon layout control, adaptive window sizing, and safety-first undo functionality while laying the foundation for automatic icon modernization and smarter Workbench maintenance.

The first GUI release will prioritize **Layout, Sorting, Aspect Ratio, and Backup**, with stretch goals prepared but disabled.  

> *Goal: iTidy should feel like the tool Commodore would have shipped if they had one more release.*
