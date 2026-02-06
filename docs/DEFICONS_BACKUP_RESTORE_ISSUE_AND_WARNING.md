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

---

# Implementation Details (February 2026)

The system described above has been fully implemented. This section documents what was built, where it lives, and key design decisions discovered during development.

## Files Modified

| File | Change |
|------|--------|
| `src/backup_types.h` | Added `CREATED_ICONS_FILENAME` constant (`"created_icons.txt"`), plus `iconsCreated`, `createdIconsFile`, and `createdIconsOpen` fields to `BackupContext` |
| `src/backup_session.h` | Declared four new manifest functions |
| `src/backup_session.c` | Implemented `OpenCreatedIconsManifest()`, `LogCreatedIconToManifest()`, `CloseCreatedIconsManifest()`, `CountCreatedIconsInManifest()` |
| `src/layout_processor.c` | Opens manifest when DefIcons processing begins; logs each created `.info` path; includes icon count in run summary |
| `src/backup_catalog.c` | Writes `Icons Created: N` to catalog footer when count > 0 |
| `src/backup_restore.c` | Added `DeleteCreatedIconsFromManifest()` which reads the manifest and deletes listed files; called from `RestoreFullRun()` |
| `src/GUI/RestoreBackups/restore_window.h` | Added `iconsCreated` field to `RestoreRunEntry` |
| `src/GUI/RestoreBackups/restore_window.c` | Added "Icons+" column to run list; added "Icons Created" row to details panel; enhanced restore confirmation dialog with warning text about DefIcons-created icons |

## Manifest Format

The file `created_icons.txt` is stored inside each `Run_NNNN` directory:

```
; iTidy Created Icons Manifest
; One .info file path per line
; These files were created by DefIcons during this run
; Restore will delete these files to return folders to pre-iTidy state
;
Work:Music/Silkworm_intro.mod.info
Work:Music/Speedball2_intro.mod.info
...
```

- Lines starting with `;` are comments and are skipped during processing.
- Each data line is a full AmigaOS path to a `.info` file.
- Writes are flushed immediately after each entry for crash safety.
- The manifest is opened once per run and closed when the backup session ends.

## Manifest Functions

- **`OpenCreatedIconsManifest(ctx)`** — Creates the manifest file inside the run directory and writes the comment header.
- **`LogCreatedIconToManifest(ctx, path)`** — Appends a single `.info` path and flushes.
- **`CloseCreatedIconsManifest(ctx)`** — Closes the file handle; called automatically by `CloseBackupSession()`.
- **`CountCreatedIconsInManifest(run_path)`** — Reads the manifest and returns the count of non-comment lines. Used by the restore window to populate the "Icons+" column without needing a live backup session.

## Restore Sequence: Critical Ordering

### The Problem

During iTidy processing, the order of operations is:

1. **DefIcons creates new `.info` files** (Phase 1 in `ProcessDirectoryWithPreferences`)
2. **BackupFolder() archives ALL `.info` files** via LhA (Phase 2, per-folder layout processing)

Because DefIcons runs first, the LhA archive **includes** the newly-created icons alongside the original user icons.

### The Solution

During restore, icons must be deleted **after** archive extraction, not before:

1. **Extract the LhA archive** — restores all `.info` files (originals + DefIcons-created) with their backed-up positions.
2. **Delete DefIcons-created icons** — reads `created_icons.txt` and removes each listed file from disk.

The final state contains only the original user icons with their pre-iTidy positions restored.

### Why Not Delete Before Extraction?

An earlier implementation attempted to delete created icons before LhA extraction. This failed because the archive itself contained the DefIcons-created icons, so extraction immediately re-created the files that had just been deleted.

## Catalog Footer

When `iconsCreated > 0`, `CloseCatalog()` writes an additional line to the catalog footer:

```
Icons Created: 35
```

This is informational. The restore window reads the count primarily from the manifest via `CountCreatedIconsInManifest()`, with a fallback that parses the catalog footer for older runs that may lack a manifest file.

## Restore Window UI Changes

- **Run list**: Added "Icons+" column (column index 5 of 6) showing the count of DefIcons-created icons per run, or `-` if zero.
- **Details panel**: Added "Icons Created" row (row 8 of 8).
- **Confirmation dialog**: When the selected run has created icons, the warning text includes a bold note stating that N DefIcons-created icon files will be removed as part of the restore.

## Safety Design

- Only files explicitly listed in the manifest are deleted — never wildcard deletion.
- Missing files are silently skipped (idempotent; safe for repeated restores).
- Delete failures are logged with `IoErr` codes but do not abort the restore.
- The manifest is write-flushed after every entry so partial data survives crashes.
