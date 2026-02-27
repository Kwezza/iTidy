# Workbench Screen Manager (Backdrop Cleaner) — Working Notes

**Source**: `src/GUI/BackdropCleaner/backdrop_window.c` (1709 lines)
**Title bar**: "iTidy - Workbench Screen Manager"
**Opened from**: Currently removed from the build (was accessible from main window)
**Modal**: Effectively modal (blocks caller)
**Resizable**: Yes (initial 560x320, min 450x200)
**Status**: Temporarily removed from the build — source preserved for future use

---

## Purpose

Audits and manages AmigaOS `.backdrop` files across all mounted volumes. A `.backdrop` file lists which icons appear directly on the Workbench screen (desktop). This window scans all devices, validates entries, identifies orphaned references (dead links to items that no longer exist), and can tidy the layout of Workbench screen icons into an organized grid.

---

## Key Concepts

### .backdrop Files

Each volume has a hidden `.backdrop` file at its root (e.g. `Workbench:.backdrop`). Each line references an item "left out" on the Workbench screen. Over time, entries can become orphaned if the referenced files are deleted or moved.

### Entry Statuses

| Status | Meaning |
|--------|---------|
| OK | Entry exists on disk |
| ORPHAN | Entry no longer exists (dead link) |
| No media | Device has no media inserted, cannot verify |
| Device | Represents the device root icon itself |

---

## Window Layout

```
+----------------------------------------------------------+
| Name      | Status | Device    | Path          | Type | X | Y |
| ScreenMode| OK     | Workbench | :Prefs/Screen | Tool |240|100|
| OldApp    | ORPHAN | Work      | :Apps/OldApp  | Proj |  -|  -|
| RAM       | Device | RAM       |               | Disk | 50| 20|
+----------------------------------------------------------+
| Press 'Scan' to begin.                                    |
+----------------------------------------------------------+
| [Scan] [Remove Orphans] [Tidy Layout] [TEST Upd/Redraw] [Close] |
+----------------------------------------------------------+
```

---

## Menu

**Project -> Save list as...**: Exports the scan results to a formatted text file via ASL file requester. Includes header, all entries in column format, and a summary line with counts by status.

---

## Gadgets

### ListBrowser

Displays all backdrop entries across all mounted volumes. Seven columns:

| Column | Content | Sortable |
|--------|---------|----------|
| Name | Icon/file name | Yes |
| Status | OK, ORPHAN, No media, or Device | Yes |
| Device | Volume name (e.g. "Workbench") | Yes |
| Path | Volume-relative path from the .backdrop file | Yes |
| Type | Icon type (Disk, Tool, Project, Drawer, etc.) | Yes |
| X | Horizontal icon position (or "Not set") | No |
| Y | Vertical icon position (or "Not set") | No |

Columns 0-4 are sortable (click header to sort/reverse) and draggable (reorderable). X and Y columns are not sortable.

### Status Label

Read-only text showing current state. Initially "Press 'Scan' to begin." Updates to show scan results (e.g. "42 entries: 30 OK, 4 devices, 6 orphans, 2 unverified").

### Scan

Scans all mounted filesystem devices:
1. Discovers all mounted volumes
2. Parses each volume's `.backdrop` file
3. Adds synthetic "Device" entries for device root icons
4. Validates each entry's existence on disk
5. Populates the list and updates the status

Shows a busy pointer during scanning.

### Remove Orphans

Removes all orphaned entries from `.backdrop` files. Only enabled after scanning finds at least one orphan.

1. Shows confirmation: "Remove N orphaned entries from .backdrop files? A backup (.backdrop.bak) will be created first."
2. Creates a `.backdrop.bak` backup of each affected file
3. Rewrites `.backdrop` files without the orphaned lines
4. Automatically re-scans to refresh the display

### Tidy Layout

Repositions all valid backdrop icons into an organized grid on the Workbench screen. Only enabled after scanning finds valid or device entries.

1. Calculates grid positions based on screen dimensions
2. Shows confirmation with count of icons and how many need repositioning
3. If all icons are already at correct positions, offers to re-apply anyway
4. Updates icon positions in `.info` files
5. Sends ARexx commands to Workbench to snapshot positions (UpdateAll, RedrawAll, Snapshot All)
6. Automatically re-scans to show updated positions

The requester confirms: "Layout N Workbench icons into a tidy grid? (M icons need repositioning) Icon positions will be saved to disk."

### TEST Upd/Redraw

Diagnostic button that sends Workbench UpdateAll and RedrawAll ARexx commands without snapshotting. For testing whether ARexx communication is working.

### Close

Closes the window.

---

## Button State Logic

| Button | Initial State | Enabled When |
|--------|---------------|-------------|
| Scan | Enabled | Always |
| Remove Orphans | Disabled | After scan, if orphans found |
| Tidy Layout | Disabled | After scan, if valid or device entries found |
| TEST Upd/Redraw | Enabled | Always |
| Close | Enabled | Always |

After Remove Orphans or Tidy Layout completes, a re-scan updates the button states based on the new data.

---

## ARexx Integration

The Tidy Layout feature uses dynamically generated ARexx scripts written to `T:` (the AmigaOS temp directory) to send menu commands to Workbench:
- `WORKBENCH.UPDATEALL` — refreshes Workbench's internal state
- `WORKBENCH.REDRAWALL` — forces visual redraw of all icons
- `WINDOW.SNAPSHOT.ALL` — saves positions to disk (only during Tidy, not TEST)

---

## Notes for Manual

- This feature is currently disabled in the build but preserved for future use.
- A `.backdrop.bak` backup is always created before removing orphans.
- The Tidy Layout feature affects all icons on the Workbench screen (left-out items from all volumes plus device icons).
- "No media" entries are not treated as orphans — they're from devices without inserted media (e.g. floppy drives).
- Positions are snapshotted via ARexx, so they survive reboots.
- The Save list function exports a detailed report useful for diagnosing backdrop file issues.
