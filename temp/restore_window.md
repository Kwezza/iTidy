# Restore Backups Window — Working Notes

**Source**: `src/GUI/RestoreBackups/restore_window.c` (1718 lines)
**Title bar**: "iTidy - Restore Backups"
**Opened from**: Main window -> Restore Backups button; also Main window -> Tools -> Restore -> Restore Layouts
**Modal**: Effectively modal (blocks main window)
**Resizable**: Yes (initial 500x350, min 400x250)

---

## Purpose

Allows the user to view, restore, and delete iTidy's LhA-based backup runs. Each time iTidy processes icons on a folder, it can create a backup run containing LhA archives of the original icon files and window positions. This window lists all backup runs and provides options to restore or delete them.

---

## Window Layout

```
+----------------------------------------------------+
| | Run | Date/Time      | Folders | Size  | Icons+| Status  |  <- runs (45%)
| |   7 | 2025-01-15..   |      12 | 46 KB |     3 | Complete|
| |   6 | 2025-01-14..   |       8 | 22 KB |     - | Complete|
| |   3 | 2025-01-10..   |       2 |  5 KB |     - | NoCAT   |
+----------------------------------------------------+
| Run Number:       0007                              |  <- details (35%)
| Date Created:     2025-01-15 14:32:17               |
| Source Directory:  Work:Projects/MyApp              |
| Total Archives:   12                                |
| Total Size:       46 KB                             |
| Icons Created:    3                                 |
| Status:           Complete (catalog present)        |
| Location:         PROGDIR:Backups/Run_0007          |
+----------------------------------------------------+
| [Delete Run] [Restore Run] [View Folders...] [Cancel]|
+----------------------------------------------------+
```

---

## Gadgets

### Run List (Upper ListBrowser)

Displays all backup runs found in `PROGDIR:Backups/`. Six columns:

| Column | Content |
|--------|---------|
| Run | Run number |
| Date/Time | When the backup was created |
| Folders | Number of folder archives in the run |
| Size | Total size of all archives (human readable: B, KB, MB) |
| Icons+ | Number of icons created by DefIcons during this run ("-" if none) |
| Status | "Complete" (has catalog) or "NoCAT" (orphaned, no catalog file) |

Single-click selects a run and updates the details panel. Double-click opens the Folder View sub-window (if the run has a catalog).

On window open, the first run is automatically selected.

### Details Panel (Lower ListBrowser)

Read-only label/value display showing details of the selected run:
- Run Number, Date Created, Source Directory, Total Archives, Total Size, Icons Created, Status, Location

Status can be: "Complete (catalog present)", "Orphaned (no catalog)", "Incomplete (missing archives)", or "Corrupted (catalog error)".

### Delete Run

Permanently deletes the selected backup run and all its files.

Shows a confirmation requester: "Delete backup run Run_NNNN? This will permanently delete N folder archive(s), Catalog file, Run directory. This action cannot be undone!"

After deletion, the run list is rescanned and repopulated. If no runs remain, all action buttons are disabled.

### Restore Run

Restores the selected backup run's icons to their original state using LhA.

1. Shows a 3-option requester explaining the consequences (bold "Warning" text):
   - **With Windows**: Restores icons AND window positions/sizes
   - **Icons Only**: Restores only icon files (positions, ToolTypes, default tools) without changing window geometry
   - **Cancel**: Aborts
2. If the run included DefIcons-created icons, the requester also notes that those icons will be removed
3. Opens a progress window showing per-folder extraction progress
4. Calls `RestoreFullRun()` which uses LhA to extract the archived `.info` files
5. Shows a completion or failure requester with folder counts

**Requires LhA**: If the LhA executable is not found, an error requester is shown: "LHA executable not found! Restore requires LHA to be installed."

### View Folders...

Opens the Folder View sub-window showing a hierarchical tree of the archive contents. Only enabled when the selected run has a catalog file. A busy pointer is shown on the restore window while the folder view is open.

Also triggered by double-clicking a run in the list.

### Cancel

Closes the window. Same effect as the window close gadget.

---

## Button State Logic

| Condition | Delete | Restore | View Folders | Cancel |
|-----------|--------|---------|--------------|--------|
| No runs found | Disabled | Disabled | Disabled | Enabled |
| Run with catalog selected | Enabled | Enabled | Enabled | Enabled |
| Orphaned run selected (no catalog) | Enabled | Enabled | Disabled | Enabled |
| All runs deleted | Disabled | Disabled | Disabled | Enabled |

---

## Backup Storage

Backups are stored under `PROGDIR:Backups/` in numbered directories (`Run_0001`, `Run_0002`, etc.). Each run directory contains:
- `catalog.txt` — metadata listing all archived folders with sizes and counts
- One `.lha` archive per folder that was processed
- Optional manifest tracking DefIcons-created icons

---

## Notes for Manual

- LhA must be installed for the Restore function to work.
- The "With Windows" vs "Icons Only" restore choice determines whether window sizes and positions are restored alongside icon data.
- "NoCAT" (orphaned) runs have no catalog file — they can be deleted but the View Folders button is disabled for them.
- Restoring overwrites current `.info` files — any changes made after the backup will be lost.
- If DefIcons created icons during the original run, restoring will also remove those created icons.
- The backup system is controlled by the Backup settings in the main window (Settings -> Backup).
- Run numbers may have gaps (e.g. Run_0001, Run_0003) if intermediate runs were deleted.
