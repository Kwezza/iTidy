# Folder View Window — Working Notes

**Source**: `src/GUI/RestoreBackups/folder_view_window.c` (997 lines)
**Title bar**: "Folder View - Run N (date)" (dynamic)
**Opened from**: Restore Backups window -> View Folders button, or double-click on a run
**Modal**: Effectively modal (blocks Restore Backups window)
**Resizable**: Yes (initial 420x300, min 300x200)
**Keyboard shortcuts**: C (Close)

---

## Purpose

A read-only tree viewer that shows the hierarchical folder structure of a backup run's archived contents. Displays which folders were backed up, their sizes, and how many icons each contained.

---

## Window Layout

Simple two-part vertical layout:
- **Tree list** (90% of height): Hierarchical ListBrowser showing the folder tree
- **Close button** (10% of height)

---

## Gadgets

### Folder Tree (ListBrowser)

Hierarchical tree display with disclosure triangles for expanding/collapsing branches. Two columns:

| Column | Width | Content |
|--------|-------|---------|
| Folder | 70% | Folder name (e.g. "Workbench", "Utilities", "Tools") |
| Size / Icons | 30% | Combined info (e.g. "11 KB / 15 icons") |

The tree is built from the backup run's `catalog.txt` file. Folder depth is calculated from the path structure (volume root = depth 0, subfolders increment). The tree starts **fully collapsed** — the user clicks disclosure triangles to explore.

Selecting a tree node currently has no effect (placeholder for potential future details view).

### Close

Closes the window (keyboard shortcut: C).

---

## Data Source

The tree is parsed from `catalog.txt` inside the backup run directory. Each line in the catalog represents one archived folder with pipe-delimited fields including the original path, archive size, and icon count. Header and separator lines are skipped during parsing.

---

## Notes for Manual

- This is a purely informational window — no actions can be performed from here.
- The tree starts collapsed; click the disclosure triangles to expand branches.
- The Size/Icons column shows compressed archive size, not original file sizes.
- Only available for backup runs that have a catalog file ("Complete" status).
