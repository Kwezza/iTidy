# DefIcons Integration: Backup/Restore Consistency Issue

## Summary

The new DefIcons feature introduces a new class of change that the current undo backup system does not fully reverse.

Historically, iTidy’s “undo” backups worked because iTidy only repositioned existing `.info` files. A restore simply put the original `.info` files back, returning the folder layout to its previous state.

With DefIcons enabled, iTidy can now **create new `.info` files** that did not exist at the time the backup was taken. Restoring an earlier LhA backup does not remove these new `.info` files. That means a restore can leave extra icons behind, and those icons may overlap or conflict with the original restored layout.

---

## Current Behaviour

### Backup system (current design intent)

- Before iTidy modifies a folder layout, it creates an LhA backup containing the folder’s `.info` files.
- Backups are grouped into `Run_NNNN` directories with a `catalog.txt` manifest.
- Each archive embeds `_PATH.txt` so it can be restored without relying only on the catalog.

### Restore system (current user experience)

The Restore Backups window shows a run list with columns:

- Run
- Date/Time
- Folders
- Size
- Status

Selecting a run displays details below (run number, date, source directory, totals, and location).

**UI improvement suggestion:** add a new run-list column: **Icons Created** (numeric) to show how many `.info` files DefIcons created during that run.

---

## The New Problem Introduced by DefIcons

### Root cause

- Backups are taken to preserve the pre-change state.
- DefIcons can create new `.info` files after that point.
- Restore replays the old `.info` state, but it does not delete new `.info` files created later.

### Symptoms

After a restore:

- The original icons and their positions are restored.
- Any newly-created `.info` files remain in place, because they were not part of the backup.
- This can lead to extra icons, overlapping positions, and a folder that is not truly returned to the pre-iTidy state.

---

## Requirements for a Correct Fix

A restore must return a folder to the **exact pre-iTidy state** for that run.

That means restore must do two things:

1. **Restore** the backed-up `.info` files (current behaviour).
2. **Remove** any `.info` files that iTidy created during the run (new requirement).

---

## Proposed Fix: Track and Undo “Created Icons” Per Run

### 1) Record created `.info` paths as part of the run

Implement a “Created Icons Manifest” for the run:

- One `.info` path per line.
- Append immediately when an icon is created.
- Flush writes so crashes still leave usable data.

### 2) Store the manifest inside the backup run (and optionally inside each archive)

Recommended storage layers:

**Run-level**
- `PROGDIR:Backups/Run_NNNN/created_icons.txt`
- Contains every created `.info` across the run.

**Archive-level (optional but robust)**
- Include `_CREATED.txt` inside each archive, listing only the created `.info` that relate to that archive’s folder.

Run-level alone is enough to fix restore correctness. Archive-level improves portability and per-folder restore accuracy.

### 3) Restore logic: delete created icons before extraction

When restoring a run (or a single archive/folder):

1. Load the created-icons list relevant to what is being restored.
2. For each `.info` path:
   - Delete the file if it exists.
   - Ignore errors for missing files (safe and idempotent).
3. Perform the normal LhA extraction of the backed-up `.info` files.
4. Restore window geometry as usual.

This ensures restored layout cannot be polluted by extra icons that did not exist when the backup was taken.

---

## Proposed Updates to the Catalog and UI

### Catalog enhancements

Extend `catalog.txt` metadata to include:

- `Icons Created: <count>`
- Optionally: `Created Manifest: created_icons.txt`

Optionally, per archive entry could include a created count if you implement archive-level manifests.

### Restore window (run list)

Add a new column:

- **Icons Created** (numeric)

### Details panel

Add a line such as:

- `Icons Created: 37`
- Optionally: `Created Icons Manifest: created_icons.txt`

---

## Edge Cases and Safety Notes

- Only delete icons explicitly listed in the manifest (never wildcard-delete).
- If a listed `.info` is missing, ignore and continue (restores should be resilient).
- If you support partial restores, archive-level `_CREATED.txt` is ideal; otherwise filter run-level entries by folder prefix.

---

# Restore Backups Warning Text

## Purpose

Restoring a backup is intended to return selected folder(s) to the **exact state they were in when the backup was taken**.

## Warning to display to the user

**Warning:** Restoring a backup will overwrite the current icon metadata in the selected folder(s) with the versions stored in the backup.

This means:

- Any changes made **after** the backup was taken will be lost.
- This includes (but is not limited to) changes to:
  - `.info` files (icon position, size, image, and snapshot data)
  - ToolTypes
  - Default tool / stack / priority settings stored in icons
  - Icon comments and any other icon metadata

If you have manually adjusted icons, replaced icons, edited ToolTypes, or made other icon-related changes since the backup date, those changes will be replaced by the backed-up versions.

Proceed only if you are happy to revert the folder(s) to the backup state.
