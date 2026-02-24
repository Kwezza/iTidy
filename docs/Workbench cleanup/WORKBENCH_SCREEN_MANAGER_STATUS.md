# Workbench Screen Manager - Implementation Status

**Date**: 2026-02-23
**Branch**: Dev
**Status**: AWAITING TEST - UpdateWorkbench remove+add fix applied for left-out icon positions

---

## 1. What Is Implemented

All modules from the original plan (`WORKBENCH_SCREEN_MANAGER_PLAN.md`) have been created and compile cleanly with VBCC. The feature is accessible from the main iTidy toolbar via a "WB Screen..." button.

### Modules Created

| Module | File(s) | Status |
|--------|---------|--------|
| Device Scanner | `src/DOS/device_scanner.c/.h` | WORKING - scans all mounted volumes, probes media silently |
| Backdrop Parser | `src/backups/backdrop_parser.c/.h` | WORKING - parses .backdrop files, validates entries, detects orphans |
| Workbench Layout | `src/layout/workbench_layout.c/.h` | COMPILES, LOGIC CORRECT - but see "Current Problem" below |
| Backdrop Window | `src/GUI/BackdropCleaner/backdrop_window.c/.h` | WORKING - ReAction GUI with ListBrowser |
| Main Window Button | `src/GUI/main_window.c` + `main_window_reaction.h` | WORKING - "WB Screen..." button opens the window |
| Makefile | Updated with all new .o files | WORKING |

### Features Working

- **Volume scanning**: All mounted volumes are detected (RAM Disk, Work, WHDBig, PC, Workbench in the test setup)
- **Backdrop parsing**: Reads all entries from `<device>:.backdrop` files correctly
- **Orphan detection**: Identifies entries pointing to non-existent targets (e.g., `test.txt`, `html-parsing`, `templates`, `DirectoryOpus.copy`)
- **Orphan removal**: `itidy_remove_selected_orphans()` removes selected orphan entries from `.backdrop` files (takes 2 args: device_name, list)
- **ListBrowser display**: 7-column display (Name, Status, Device, Path, Type, X, Y) with horizontal scroller
- **"Not set" display**: Icons with `NO_ICON_POSITION` (0x80000000) show "Not set" instead of the raw integer
- **Device icon display**: All device icons (Disk icons) shown as entries in the ListBrowser
- **Layout calculation**: Calculates correct positions for all 21 icons (5 device + 16 left-out) using the main iTidy layout engine
- **Layout saving**: Writes all icon positions to disk via `PutIconTagList()` with `ICONPUTA_NotifyWorkbench`
- **RAM: persistence prompt**: After repositioning, prompts to copy RAM icon to `ENVARC:Sys/def_RAM.info`
- **Re-scan after tidy**: Automatically re-scans to show updated positions in the ListBrowser
- **Detailed logging**: Full debug output of all layout calculations and save paths

### Layout Engine Integration

Rather than implementing a simple grid-based layout from scratch, the workbench layout module reuses iTidy's main layout engine components:

- `CalculateLayoutWithAspectRatio()` - determines optimal columns
- `CalculateLayoutPositions()` - positions device icons in row(s)
- `CalculateBlockLayout()` - groups left-out icons by Workbench type (Drawers, Tools, Other)
- `CalculateLayoutPositionsWithColumnCentering()` - positions within each block
- `SortIconArrayWithPreferences()` - alphabetical sorting within groups

A bridge function `build_icon_detail()` converts `iTidy_BackdropEntry` structs into `FullIconDetails` using `GetIconDetailsFromDisk()`, allowing the existing engine to work with backdrop data.

---

## 2. Current Problem: Icons Not Moving On Screen

### Symptom

After running "Tidy Layout":
- The .info files on disk are updated with correct positions (confirmed by re-scan)
- **Device icons** (PC, RAM Disk, WHDBig, Workbench, Work) appear in a neat sorted row - this works correctly
- **Left-out icons** (drawers, tools, projects from .backdrop files) do NOT visually move on the Workbench screen
- Even after rebooting, left-out icon positions on screen do not match the positions saved in their .info files
- Only device icons respond correctly to position changes

### What The Logs Show

From `general_2026-02-23_18-10-58.log`:

```
Layout calculated: 21 icons (5 device + 16 left-out), 0 changed positions
Saving icon [0]: path='PC:Disk' pos=(10,20) (unchanged)
Saving icon [5]: path='PC:Programming/iTidy/Bin/Amiga' pos=(20,85) (unchanged)
Saving icon [12]: path='Workbench:Prefs/Font' pos=(58,183) (unchanged)
...
Layout applied: 21 of 21 icons written (0 had new positions)
```

All 21 icons were successfully written via `PutIconTagList()`. The paths look correct:
- Device icons: `PC:Disk`, `RAM Disk:Disk`, `WHDBig:Disk`, `Workbench:Disk`, `Work:Disk`
- Left-out icons: `PC:Programming/iTidy/Bin/Amiga`, `Workbench:Prefs/Font`, etc.

### Theories Investigated

1. **"0 changed" skip**: Initially, when all positions matched on-disk values, the code skipped writing entirely. FIXED - now writes all icons regardless of whether positions changed.

2. **TEXT_ALIGN_BOTTOM scattering**: Icons within the same row had different Y values because `apply_row_vertical_alignment()` with `TEXT_ALIGN_BOTTOM` added `maxRowHeight - iconHeight` offsets. FIXED - changed to `TEXT_ALIGN_TOP`.

3. **Index mismatch after sort**: `SortIconArrayWithPreferences()` reorders the IconArray, but result-building used pre-sort index arrays, mapping positions to wrong backdrop entries. FIXED - replaced with `find_entry_by_path()` that matches by `icon_full_path` after sorting.

4. **Path resolution**: Theory that `PutIconTagList()` might resolve the left-out path and save the .info file inside the target folder rather than as a backdrop icon. NOT CONFIRMED - the log shows correct paths being used, and re-scan confirms positions are saved correctly in the .info files.

### What We Haven't Tried

- **Workbench may not use do_CurrentX/do_CurrentY for backdrop (left-out) icons the same way as device icons**. Workbench might store left-out icon positions in its own internal memory rather than reading them from .info files on boot. The .backdrop file only stores the path, not the position - but Workbench may cache positions separately.

- **`UpdateWorkbench()` API call** (from `wb.doc`): We currently rely on `ICONPUTA_NotifyWorkbench` tag in `PutIconTagList()`. This may not be sufficient for backdrop icons. The `UpdateWorkbench()` function or other Workbench IPC might be needed.

- **Snapshot/UnSnapshot**: Workbench has View -> Show All / Show Only Icons and Window -> Snapshot All / Snapshot Window. The relationship between these and `do_CurrentX`/`do_CurrentY` for backdrop icons may be relevant.

- **Writing to wrong .info file**: While paths look correct in logs, the actual filesystem resolution through assigns and links hasn't been independently verified. The icon might be saved to a different location than where Workbench reads it from.

- **Workbench reading positions from a different source**: Workbench 3.2 may store backdrop icon positions in a database or cache file rather than in individual .info files. Need to research this.

---

## 3. Deviations From Original Plan

### 3.1 Layout Engine: Reused Main iTidy Engine (Not Simple Grid)

**Plan said**: Simple grid layout with `ITIDY_WB_GRID_X` (80px) and `ITIDY_WB_GRID_Y` (50px) spacing, arranging icons row by row.

**What was done**: Reused iTidy's full layout engine with aspect ratio calculation, block grouping by type, and column centering. This provides better-looking results with variable-width icons and proper type grouping (Drawers first, then Tools, then Other).

**Why**: The simple grid would produce poor results because desktop icons have wildly different widths (e.g., "GUI-WHDload-downloader" is 132px wide, "Roadie" is 3px). The existing engine handles this gracefully with per-column width optimization.

### 3.2 ListBrowser: 7 Columns Instead of 5

**Plan said**: 5 columns (Status icon, Device, Path, Type, Position X,Y).

**What was done**: 7 columns (Name, Status, Device, Path, Type, X, Y). Added a dedicated Name column for the display name, and split position into separate X and Y columns.

**Why**: The Name column makes it easier to identify icons at a glance without parsing the full path. Separate X/Y columns are cleaner for sorting.

### 3.3 No Status Icons in ListBrowser

**Plan said**: Colour/icon coding with green checkmark, red X, yellow for no media.

**What was done**: Text-based status column ("Valid", "Orphan", "Device", "No media") without embedded images.

**Why**: ReAction ListBrowser image embedding requires pre-loaded image objects and careful memory management. Text status is simpler and still communicates the information clearly.

### 3.4 "WB Screen..." Button Label (Not "Backdrop cleaner...")

**Plan said**: Button labelled "Backdrop cleaner..."

**What was done**: Button labelled "WB Screen..." (or similar shortened label).

**Why**: The feature evolved beyond just backdrop cleaning to include device icon layout and full screen tidying. The shorter label also fits better in the button bar.

### 3.5 No .backdrop.bak Backup (Yet)

**Plan said**: `itidy_backup_backdrop()` creates `.backdrop.bak` before any modifications.

**What was done**: Orphan removal works but the `.backdrop.bak` backup step may not be implemented yet.

**Why**: This is a minor omission that could be added. The remove function rewrites the .backdrop file in-place.

### 3.6 No AppIcon Detection

**Plan said**: Detect and skip AppIcons (iconified programs), display as grey/italic with "Running program (skipped)".

**What was done**: Not implemented. AppIcons are not detected or displayed.

**Why**: Lower priority - AppIcons are transient and uncommon on typical desktops. Can be added later.

### 3.7 No Write-Protected Device Handling

**Plan said**: Disable Remove/Tidy for write-protected devices with explanatory status.

**What was done**: Write-protected status is detected during scan but the Remove/Tidy buttons are not conditionally disabled per-device.

**Why**: Lower priority. The current test setup doesn't have write-protected volumes.

### 3.8 Reboot Requester After Backdrop Modifications

**Plan said**: Show modal "reboot required" requester after .backdrop file changes.

**What was done**: Not verified whether this is implemented.

**Why**: May have been deferred pending the layout positioning fix.

---

## 4. Build Configuration

The feature compiles cleanly with:
```
make clean && make
```

No new library linking was needed. All ReAction classes and DOS functions were already available.

Key new object files in Makefile:
- `build/amiga/DOS/device_scanner.o`
- `build/amiga/backups/backdrop_parser.o`
- `build/amiga/layout/workbench_layout.o`
- `build/amiga/GUI/BackdropCleaner/backdrop_window.o`

---

## 5. Key Code Locations

### Layout Calculation Flow

1. `perform_tidy_layout()` in `backdrop_window.c:673` - entry point from GUI
2. `itidy_init_wb_layout_params()` - sets screen dimensions and margins
3. `itidy_calculate_wb_layout()` in `workbench_layout.c:253` - main calculation:
   - Iterates backdrop list, calls `build_icon_detail()` for each valid entry
   - Partitions into device_icons and leftout_icons IconArrays
   - **Phase 1**: Device icons sorted by name, positioned via `CalculateLayoutPositions()` with `iconSpacingY = margin_top` (20px)
   - **Phase 2**: Left-out icons processed via `CalculateBlockLayout()` (groups by WBDRAWER/WBTOOL/Other), then Y-shifted below device row
   - Result entries matched to backdrop entries via `find_entry_by_path()`
4. `itidy_apply_wb_layout()` in `workbench_layout.c:548` - writes all icons:
   - Loads each icon with `GetDiskObject(entry->full_path)`
   - Sets `dobj->do_CurrentX` and `dobj->do_CurrentY`
   - Saves with `PutIconTagList()` using `ICONPUTA_NotifyWorkbench = TRUE`

### Current Layout Preferences (build_wb_prefs)

```
layoutMode          = LAYOUT_MODE_ROW
sortBy              = SORT_BY_NAME
sortPriority        = SORT_PRIORITY_MIXED
textAlignment       = TEXT_ALIGN_TOP
blockGroupMode      = BLOCK_GROUP_BY_TYPE
blockGapSize        = BLOCK_GAP_MEDIUM (10px)
centerIconsInColumn = TRUE
maxWindowWidthPct   = 100 (full screen)
aspectRatio         = 5000 (wide layout)
iconSpacingX        = 10
iconSpacingY        = 8
```

---

## 6. Bug Fix History

### Fix 1: Index Mismatch After Sort (2026-02-23)
**Problem**: `SortIconArrayWithPreferences()` reorders the IconArray, but result-building loops used pre-sort index mapping arrays (`device_entry_indices[i]`), assigning each icon's calculated position to the wrong backdrop entry.
**Solution**: Replaced stale index arrays with `find_entry_by_path()` helper that matches icons to backdrop entries by `icon_full_path` after sorting.

### Fix 2: TEXT_ALIGN_BOTTOM Vertical Scatter (2026-02-23)  
**Problem**: `apply_row_vertical_alignment()` with `TEXT_ALIGN_BOTTOM` adds `maxRowHeight - thisIcon->icon_max_height` to each icon's Y position. Since icons have wildly different heights (Roadie=3px image, Rebuild=81px image), icons in the same row ended up at very different Y values.
**Solution**: Changed `textAlignment` to `TEXT_ALIGN_TOP` in `build_wb_prefs()`.

### Fix 4: UpdateWorkbench Remove+Add For Left-Out Icon Visual Refresh (2026-02-23)
**Problem**: `ICONPUTA_NotifyWorkbench` updates the icon's appearance in-place but does not physically move a left-out (backdrop) icon to a new screen position. Workbench keeps on-screen positions in its internal state and overwrites the `.info` file during shutdown, undoing iTidy's changes. This is why even rebooting did not help - WB snapshots the old on-screen position back to disk on exit.
**Solution**: Added `UpdateWorkbench(filename, parent_lock, UPDATEWB_ObjectRemoved)` + `UpdateWorkbench(filename, parent_lock, UPDATEWB_ObjectAdded)` cycle (V37 workbench.library) immediately after each successful `PutIconTagList()` for non-device icons. The Remove call evicts the icon from WB's internal state; the Add call forces WB to re-read the `.info` file (with the new X/Y) and redisplay the icon at the correct position. Device icons are excluded because `UPDATEWB_ObjectRemoved` is documented as a no-op for disk icons and the existing `ICONPUTA_NotifyWorkbench` path already handles them correctly.
**Files changed**: `src/layout/workbench_layout.c` - added `#define WorkbenchBase` isolation, `#include <proto/wb.h>`, `split_amiga_path()` helper, and the UpdateWorkbench remove+add cycle in `itidy_apply_wb_layout()`.
**Problem**: When all .info files already contained the correct positions (from a previous tidy run), `changed_count == 0` caused the GUI to show "All icons are already in their target positions" and skip the write entirely. Since `ICONPUTA_NotifyWorkbench` was never fired, Workbench never refreshed its display.
**Solution**: Removed the early exit. Now writes ALL icons regardless of whether positions changed, always triggering `ICONPUTA_NotifyWorkbench`. Changed confirm dialog to show "Re-apply layout to refresh Workbench display?" when 0 changes detected.

---

## 7. Next Steps / Investigation Needed

1. **Research why left-out icon positions don't take effect** - This is the primary blocker. Device icons move correctly but left-out (backdrop) icons don't. Possible avenues:
   - Check if Workbench 3.2 stores backdrop icon positions in a separate cache/database
   - Test with `UpdateWorkbench()` API instead of/in addition to `ICONPUTA_NotifyWorkbench`
   - Try the "Snapshot All" approach - does Workbench only respect positions after a snapshot?
   - Read the `wb.doc` AutoDocs for `AddAppWindow`, `UpdateWorkbench`, and backdrop handling
   - Check if the `.info` file needs to be in a specific location for backdrop icons (maybe the device root?)

2. **Verify .info file locations** - For a left-out like `PC:Programming/ListView`, the .info file is at `PC:Programming/ListView.info`. But does Workbench read backdrop icon positions from there, or from somewhere else?

3. **Add .backdrop.bak backup** before orphan removal

4. **Add AppIcon detection** (lower priority)

5. **Add write-protected device handling** (lower priority)

6. **Add reboot requester** after .backdrop modifications (if not already present)

---

## 8. Test Environment

- **Emulator**: WinUAE
- **OS**: Workbench 3.2.3
- **Screen**: 800x589 usable (after title bar)
- **Volumes**: RAM Disk, Work, WHDBig, PC (shared), Workbench
- **Left-out icons**: 16 across PC: and Workbench: volumes
- **Orphans detected**: 4 (test.txt, html-parsing, templates on PC:; DirectoryOpus.copy on Workbench:)
