# iTidy Main Window

This document explains all options and controls available in the main iTidy window.

---

## Window Title

**iTidy v1.2 - Icon Cleanup Tool**

The main window provides quick access to the most common icon tidying operations. More detailed settings are available through the Advanced Settings window.

---

## Folder Section

### Folder Path

**Control Type:** Display box (read-only) with Browse button  
**Purpose:** Shows the currently selected folder to process

The folder path display shows the Amiga path (e.g., `SYS:`, `Work:Projects`, `DH0:Games`) that will be processed when you click Start.

### Browse... Button

Opens the standard AmigaDOS file requester configured to select directories. Navigate to the folder you want to tidy and click OK.

**Tips:**
- The requester shows only drawers (folders), not files
- You can type a path directly in the Drawer field
- The last selected path is remembered during the session

---

## Tidy Options Section

### Order

**Control Type:** Cycle gadget  
**Options:** Folders First, Files First, Mixed  
**Default:** Folders First

Controls how folders and files are grouped when sorting icons.

| Order | Behavior |
|-------|----------|
| Folders First | All drawer icons appear before file icons |
| Files First | All file icons appear before drawer icons |
| Mixed | No grouping - folders and files are interspersed based on sort criteria |

### By (Sort By)

**Control Type:** Cycle gadget  
**Options:** Name, Type, Date, Size  
**Default:** Name

Determines the primary sort criteria for arranging icons.

| Sort By | Behavior |
|---------|----------|
| Name | Alphabetical order (A-Z) |
| Type | Groups by file extension (e.g., .txt, .iff, .info) |
| Date | Chronological by modification date (oldest first) |
| Size | By file size (smallest first) |

**Note:** Combine with "Reverse Sort" in Advanced Settings to get Z-A, newest-first, or largest-first ordering.

### Cleanup subfolders

**Control Type:** Checkbox  
**Default:** Unchecked

When enabled, iTidy recursively processes all subdirectories within the selected folder. Each subfolder receives the same tidying treatment as the parent.

**Use cases:**
- Organizing an entire disk or partition
- Tidying a project folder with multiple nested directories
- Cleaning up extracted archive contents with complex folder structures

**Warning:** Processing many folders can take considerable time on slower systems or real Amiga hardware.

### Backup icons using LhA

**Control Type:** Checkbox  
**Default:** Unchecked  
**Requirement:** LhA archiver must be available in `C:` directory

When enabled, iTidy creates an LhA archive backup of each folder's icons and window settings *before* making any changes. This allows you to restore the original layout if needed.

**How backups work:**
1. Before processing each folder, iTidy archives all `.info` files
2. Archives are stored in `ENVARC:iTidy/Backups/` organized by date and run number
3. Each run creates a catalog file listing all backed up folders
4. Window geometry (position, size) is also recorded for restoration

**Benefits:**
- Full undo capability - restore any folder to its pre-tidied state
- Non-destructive workflow - experiment freely knowing you can revert
- Historical record - keep multiple backup runs for comparison

**Note:** If LhA is not found in `C:`, backups will fail silently and processing continues without backup protection.

### Position

**Control Type:** Cycle gadget  
**Options:** Center Screen, Keep Position, Near Parent, No Change  
**Default:** Center Screen

Controls where folder windows are positioned after being resized.

| Position Mode | Behavior |
|---------------|----------|
| Center Screen | Window is centered on the Workbench screen (classic iTidy behavior) |
| Keep Position | Window stays at its current location; pulled back if off-screen |
| Near Parent | Window positioned near the parent drawer (if space allows) |
| No Change | Window is not moved at all - only resized |

### ? (Help Button)

Displays a brief explanation of the window position options. Click to see a popup describing each mode.

---

## Tools Section

This section provides access to specialized tools and features.

### Advanced...

**Purpose:** Opens the Advanced Settings window

The Advanced Settings window provides detailed control over:
- Layout aspect ratio (window proportions)
- Overflow strategy (how to handle many icons)
- Icon spacing (horizontal and vertical gaps)
- Icons per row limits (minimum and maximum columns)
- Maximum window width percentage
- Vertical icon alignment
- Reverse sort order
- Column width optimization
- Column-centered layout
- Skip hidden folders option
- Strip NewIcon borders option
- Beta/experimental features

See [Advanced Settings Documentation](Advanced_Settings.md) for complete details.

### Fix Default Tools...

**Purpose:** Opens the Default Tool Scanner/Fixer window

This feature scans icons in the selected folder to find and optionally repair missing or invalid default tools.

See [Fix Default Tools Documentation](Fix_Default_Tools.md) for complete details.

### Restore Backups...

**Purpose:** Opens the Backup Restore window

This feature allows you to restore folders to their previous state from iTidy backup archives. Only available if you previously ran iTidy with "Backup icons using LhA" enabled.

See [Restore Backups Documentation](Restore_Backups.md) for complete details.

---

## Action Buttons

### Start

**Purpose:** Begin processing the selected folder

Clicking Start:
1. Validates the folder path exists
2. Opens a progress window showing status
3. Creates backups (if enabled)
4. Scans each folder for icons
5. Sorts icons according to settings
6. Calculates optimal layout positions
7. Saves new icon positions
8. Resizes and repositions folder windows
9. Displays completion statistics

**Progress Window:**
While processing, a progress window shows:
- Current folder being processed
- Number of icons found and processed
- Any errors encountered
- Cancel button to abort operation

After processing completes, the progress window shows a summary and waits for you to click Close.

### Exit

**Purpose:** Close iTidy and return to Workbench

Closes the main window and exits the application. Any unsaved preference changes are discarded.

---

## Menu Bar

The main window includes a Project menu for managing preference files.

### Project Menu

| Menu Item | Shortcut | Description |
|-----------|----------|-------------|
| New | Amiga+N | Reset all settings to defaults |
| Open... | Amiga+O | Load preferences from a file |
| Save | Amiga+S | Save preferences to last used file |
| Save as... | Amiga+A | Save preferences to a new file |
| Close | Amiga+C | Close the main window (same as Exit button) |

**Preference Files:**
- Preferences are saved as plain text files
- Default location: `ENVARC:iTidy/` or any chosen directory
- Include all main window and advanced settings
- Can be shared between systems

---

## Quick Start Guide

1. **Select a folder:** Click Browse and navigate to the folder you want to tidy
2. **Choose sorting:** Set Order (Folders First) and By (Name) as desired
3. **Enable subfolders:** Check "Cleanup subfolders" for recursive processing
4. **Enable backup:** Check "Backup icons using LhA" for safety (recommended)
5. **Click Start:** Wait for processing to complete
6. **Review:** Open the folder in Workbench to see the tidied icons

---

## Technical Notes

### Window Behavior

- The main window opens on the Workbench screen
- Modal dialogs (Advanced, Restore, Fix Tools) disable the main window while open
- Window size is fixed and designed for standard Workbench fonts
- Group boxes provide visual organization of related controls

### Preference Storage

Main window settings map to these `LayoutPreferences` fields:

| Control | Preference Field |
|---------|------------------|
| Folder Path | `folder_path` (char[256]) |
| Order | `sortPriority` (enum: 0=Folders First, 1=Files First, 2=Mixed) |
| Sort By | `sortBy` (enum: 0=Name, 1=Type, 2=Date, 3=Size) |
| Cleanup subfolders | `recursive_subdirs` (BOOL) |
| Backup icons using LhA | `backupPrefs.enableUndoBackup` (BOOL) |
| Position | `windowPositionMode` (enum: 0-3) |

### System Requirements

- AmigaOS 3.0 or higher (Workbench 3.0+)
- icon.library (included with Workbench)
- LhA archiver in C: (optional, for backup/restore features)
- Minimum 512KB free RAM (more for large folders)
