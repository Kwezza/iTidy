# iTidy Restore Backups Window

This document explains the Restore Backups window, accessible by clicking "Restore Backups..." in the main iTidy window.

---

## Purpose

The Restore Backups window allows you to restore folders to their previous state from iTidy backup archives. This feature is only available if you previously ran iTidy with "Backup icons using LhA" enabled.

---

## Window Layout

### Run List (Top ListView)

Shows all previous iTidy backup runs stored on your system.

**Columns:**
| Column | Description |
|--------|-------------|
| Run | Run number (sequential identifier) |
| Date/Time | When the backup was created |
| Folders | Number of folders backed up in this run |
| Size | Total size of all archives in this run |
| Status | Complete or Incomplete |

**Usage:**
- Click a run to select it and view its details
- Buttons are disabled until a run is selected
- Selecting a run populates the Details Panel below

### Details Panel (Bottom ListView)

Shows detailed information about the currently selected run.

**Information displayed:**
- **Run Number** - The sequential run identifier (e.g., 0001, 0002)
- **Date Created** - Full timestamp of when the backup was created
- **Source Directory** - The folder that was originally processed (e.g., `PC:`, `Work:Projects`)
- **Total Archives** - Number of folder backups in this run
- **Total Size** - Combined size of all archive files
- **Status** - Whether the run is complete and if the catalog file is present
- **Location** - Path to the backup run directory (e.g., `PROGDIR:Backups/Run_0001`)

---

## Controls

### Restore window positions Checkbox

**Default:** Checked (enabled)

When enabled, restores the folder's window geometry (position and size) as it was when backed up. This updates the drawer icon's stored window settings.

**Note:** A Workbench restart may be required to see the restored window positions take effect, as Workbench caches window geometry for open folders.

---

## Buttons

### Restore Run

Restores all folders from the selected backup run.

**What happens:**
1. Extracts all archived `.info` files back to their original locations
2. Restores icon positions (X, Y coordinates stored in the icon)
3. If "Restore window positions" is checked, also restores window geometry
4. Shows progress during the restore operation

**Note:** This overwrites current icons with the backed-up versions.

### Delete Run

Permanently deletes the selected backup run.

**What happens:**
1. Shows a confirmation dialog before deleting
2. Removes all archive files in the run
3. Deletes the catalog file
4. Removes the run directory

**⚠️ Warning:** This action cannot be undone! Once deleted, the backup cannot be recovered.

### View Folders...

Opens a new window showing all folders included in the selected backup run.

**Information displayed:**
- Folder paths
- Icon counts per folder
- Window geometry information

**Note:** This button is only enabled if the run has a valid catalog file. Runs without catalogs (incomplete backups) cannot display folder details.

### Cancel

Closes the Restore Backups window and returns to the main iTidy window. No changes are made.

---

## What Gets Restored

When you restore a backup run, the following are restored for each folder:

| Item | Description |
|------|-------------|
| `.info` files | All icon files in each folder |
| Icon positions | X and Y coordinates stored in the icon |
| Window geometry | Position and size (if checkbox enabled) |
| Folder view mode | Icon/list view settings |

---

## Requirements

For the Restore Backups feature to work:

- **LhA archiver** must be available in `C:` directory
- **Backup archives** must exist in `PROGDIR:Backups/` or `ENVARC:iTidy/Backups/`
- **Sufficient disk space** for extracted icons

---

## Backup Storage Location

iTidy stores backups in a structured directory hierarchy:

```
PROGDIR:Backups/
├── Run_0001/
│   ├── catalog.dat         (folder listing and metadata)
│   ├── folder_001.lha      (archived icons for first folder)
│   ├── folder_002.lha      (archived icons for second folder)
│   └── ...
├── Run_0002/
│   └── ...
└── ...
```

Each run directory contains:
- A catalog file with metadata about all backed-up folders
- Individual LhA archives for each folder's icons

---

## Troubleshooting

### "No backup runs found"

- Ensure you have previously run iTidy with "Backup icons using LhA" enabled
- Check that LhA is installed in `C:` directory
- Verify the backup directory exists and contains run folders

### Buttons remain disabled

- You must select a run from the Run List before buttons become active
- "View Folders..." remains disabled for runs without a catalog file

### Restored positions not visible

- Workbench caches window positions for open folders
- Close and reopen the affected folders, or restart Workbench
- In some cases, a full system restart may be required

### Restore fails

- Ensure the original folder paths still exist
- Check that you have write permission to the target folders
- Verify sufficient disk space is available
- Check the log files in `PROGDIR:logs/` for detailed error messages
