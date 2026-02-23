# Plan: Workbench Screen Manager (Backdrop Inventory + Cleaner)

A new ReAction-based window accessible from the main iTidy toolbar that scans all mounted volumes, audits `.backdrop` files for orphaned entries, and can re-layout device icons and left-out icons on the Workbench screen in tidy horizontal rows (devices first, then left-outs sorted by type/name). Removable media is probed silently via `pr_WindowPtr = -1` suppression. RAM: icon position changes are persisted to `ENVARC:Sys/def_RAM.info` with user confirmation. A reboot-required requester is shown after `.backdrop` modifications.

---

## Steps

### 1. Add new gadget ID and button slot in main window

- In `src/GUI/main_window_reaction.h`: add `ITIDY_GAD_IDX_BACKDROP_BUTTON` to the enum (before `ITIDY_GAD_IDX_COUNT`), define `GID_MAIN_BACKDROP_BUTTON` as 1012, and add the `ITIDY_GAID_*` alias.
- In `src/GUI/main_window.c` around the TOOLS_LAYOUT section (~line 790): add a fourth button `"Backdrop cleaner..."` using `LAYOUT_AddChild` + `NewObject(BUTTON_GetClass(), ...)`, matching the existing Restore/Advanced/DefTools button pattern.
- In the `handle_gadget_event()` switch (~line 2148): add a `case ITIDY_GAID_BACKDROP_BUTTON` that declares a local `iTidyBackdropWindow` struct, sets busy pointer, opens/runs/closes the backdrop window, clears busy pointer -- same pattern as the restore button case.

### 2. Create new module: `src/DOS/device_scanner.c/.h`

New DOS-level module for enumerating all mounted volumes and probing their status. Key functions:

- `itidy_scan_devices(iTidy_DeviceList *out)` -- Uses `LockDosList(LDF_VOLUMES | LDF_READ)` + `NextDosEntry()` to enumerate all mounted volumes. For each volume: extracts name from BSTR (`BADDR(dl->dol_Name) + 1`), calls `IsFileSystem()` to filter non-filesystem entries.
- `itidy_probe_device(const char *device_name, iTidy_DeviceStatus *out)` -- Suppresses DOS requesters (`pr_WindowPtr = (APTR)-1` per the pattern in `src/icon_types/tool_cache.c`), attempts `Lock()` on `"<device>:"`. If Lock fails -> `ITIDY_DEV_NO_MEDIA`. If succeeds -> calls `Info()` to get `id_DiskState` (write-protected?), `id_DiskType` (filesystem type), and frees lock. Restores `pr_WindowPtr` after.
- `itidy_classify_device(const char *device_name)` -- Returns whether device is likely removable (heuristic: `DF0`-`DF3`, `CD0`-`CD9`, or unknown handler types) or fixed.
- `itidy_free_device_list(iTidy_DeviceList *list)` -- Cleanup.

Key struct:
```c
typedef struct {
    char device_name[64];
    char volume_name[64];
    BOOL is_filesystem;
    BOOL has_media;
    BOOL is_write_protected;
    BOOL is_removable;
    LONG total_kb;
    LONG free_kb;
} iTidy_DeviceInfo;
```

### 3. Create new module: `src/backups/backdrop_parser.c/.h`

Enhanced `.backdrop` file parser expanding on the limited existing code in `src/icon_misc.c`. The existing parser is limited to 20 entries with 128-char paths and only handles one volume at a time. The new module provides:

- `iTidy_BackdropEntry` struct -- `entry_path[256]`, `full_path[320]`, `device_name[64]`, `exists` (BOOL), `icon_type` (WBDISK/WBTOOL/etc.), `icon_x`, `icon_y`
- `iTidy_BackdropList` -- dynamic array of entries + count + capacity (using `whd_malloc`)
- `itidy_parse_backdrop(const char *device_name, iTidy_BackdropList *out)` -- Opens `<device>:.backdrop`, reads all `:path` entries (no line limit), constructs full paths, returns list. Handles missing `.backdrop` (returns empty list, not an error).
- `itidy_validate_backdrop_entries(iTidy_BackdropList *list)` -- For each entry, suppress requesters and attempt `Lock()` on the full path (with `.info`). Sets `exists` flag. Also loads via `GetDiskObject()` to read `do_Type`, `do_CurrentX`, `do_CurrentY`.
- `itidy_remove_backdrop_entry(const char *device_name, int entry_index, iTidy_BackdropList *list)` -- Rewrites `.backdrop` file omitting the specified entry. Must first call `itidy_backup_backdrop()`.
- `itidy_backup_backdrop(const char *device_name)` -- Copies `<device>:.backdrop` to `<device>:.backdrop.bak` (simple file copy, not LHA -- `.backdrop` files are small text files).
- `itidy_free_backdrop_list(iTidy_BackdropList *list)` -- Cleanup.

#### .backdrop file format reference

The `.backdrop` file is a plain text file at the root of each volume (e.g., `Workbench:.backdrop`):
- One entry per line, newline-delimited
- Each entry starts with `:` (colon representing the volume root)
- Path is relative to the volume root, **without** `.info` extension
- Example: `:Prefs/ScreenMode` means `Workbench:Prefs/ScreenMode`
- Lines not starting with `:` are unexpected/invalid

### 4. Create new module: `src/layout/workbench_layout.c/.h`

Workbench screen icon positioning logic:

- `itidy_calculate_wb_layout(iTidy_WBLayoutParams *params, iTidy_WBLayoutResult *out)` -- Pure calculation function. Takes: screen width/height, icon dimensions (from label-aware metrics), list of device icons + left-out icons. Produces: new X/Y coordinates for each icon. Layout algorithm:
  - **Row 1+**: Device icons sorted alphabetically by volume name, laid out left-to-right with standard Workbench grid spacing. Wrap to next row when exceeding screen width minus margin.
  - **Below devices**: Left-out icons sorted by type (`WBTOOL` first, then `WBPROJECT`, then others), then alphabetically within type. Same horizontal row wrapping.
  - Grid spacing derived from `prefsWorkbench` icon spacing settings or sensible defaults (e.g., 80px horizontal, 50px vertical).
  - Start position offset from top-left to avoid screen title bar area.
- `itidy_apply_wb_layout(iTidy_WBLayoutResult *result)` -- Calls `GetDiskObject()` / modify `do_CurrentX`/`do_CurrentY` / `PutDiskObject()` for each entry. For volume root icons, operates on `<device>:Disk.info`. Uses `PutIconTagList()` with `ICONPUTA_NotifyWorkbench` tag (V44+) so Workbench updates live for device icons.
- Special case: RAM: icon -- after saving `RAM:Disk.info`, prompt user via `ShowReActionRequester()` whether to copy to `ENVARC:Sys/def_RAM.info` for persistence.

### 5. Create new GUI window: `src/GUI/BackdropCleaner/backdrop_window.c/.h`

ReAction window following the exact pattern from `src/GUI/RestoreBackups/restore_window.c`:

- **Library base isolation**: `#define WindowBase iTidy_Backdrop_WindowBase` etc.
- **Window layout** (single window using layout.gadget with vertical orientation):
  - **Top section** -- "Scan" button + status label showing scan state
  - **Middle section** -- ListBrowser with columns: Status icon (checkmark/warning/error), Device, Path, Type (Tool/Project/Disk/etc.), Position (X,Y). Shows all device icons and all `.backdrop` entries across all volumes. Colour/icon coding: green checkmark = valid, red X = orphan/missing, yellow = no media.
  - **Bottom button bar** -- "Remove Selected" (removes orphan entries from `.backdrop`), "Tidy Layout" (repositions all icons), "Close"

- `open_backdrop_window(iTidyBackdropWindow *data)` -- Opens ReAction classes, creates window object tree, opens window. Returns BOOL.
- `handle_backdrop_window_events(iTidyBackdropWindow *data)` -- Event loop. Handles:
  - **Scan button**: Calls `itidy_scan_devices()`, then for each device with media: `itidy_parse_backdrop()` + `itidy_validate_backdrop_entries()`. Also enumerates device root icons (`<device>:Disk.info`). Populates ListBrowser. Uses progress heartbeat during scan.
  - **ListBrowser selection**: Enable/disable "Remove Selected" based on whether an orphan is selected.
  - **Remove Selected**: Confirmation requester ("Remove N orphaned entries from .backdrop? A backup will be created."). Calls `itidy_backup_backdrop()` then `itidy_remove_backdrop_entry()` for each. Shows reboot requester afterward.
  - **Tidy Layout**: Calls `itidy_calculate_wb_layout()` + `itidy_apply_wb_layout()`. Shows summary requester with count of repositioned icons. If RAM: was repositioned, triggers RAM persistence prompt.
  - **Close / WMHI_CLOSEWINDOW**: Exit loop.
- `close_backdrop_window(iTidyBackdropWindow *data)` -- Detach ListBrowser labels, `DisposeObject(window_obj)`, close all class libraries.

### 6. Handle iconified programs (informational only)

Iconified (AppIcon) programs on the Workbench screen are managed by their host application via `AddAppIconA()`/`RemoveAppIcon()`. They cannot be reliably repositioned by iTidy because:
- Their position is controlled by the running program
- They appear/disappear dynamically
- Moving them would be overridden when the program next updates the icon

The scan should **detect and skip** AppIcons (identifiable by `do_Type == WBAPPICON` value 8). If any are found during scan, display them in the ListBrowser as grey/italic with status "Running program (skipped)".

### 7. Removable media handling

During `itidy_scan_devices()`:
- For each volume returned by `NextDosEntry()`, call `itidy_probe_device()` which uses `pr_WindowPtr = (APTR)-1` before any `Lock()` attempt.
- If `Lock()` fails: the volume is listed in the ListBrowser with a "No media" status. Its `.backdrop` entries (if any were cached from a previous session) are shown as "Cannot verify".
- If the device is write-protected: "Remove" and "Tidy" actions are disabled for that device, with tooltip/status explaining why.
- The user never sees an AmigaDOS "Please insert volume" requester.

### 8. Wire up to Makefile

Add the new `.c` files to the `OBJS` list in the Makefile: `device_scanner.o`, `backdrop_parser.o`, `workbench_layout.o`, `backdrop_window.o`. No new library linking should be needed -- all required ReAction classes and DOS functions are already linked.

---

## Verification

- **Build**: `make clean && make` -- verify clean compilation with VBCC, check `build_output_latest.txt` for errors/warnings
- **Scan test**: Run in WinUAE with multiple volumes mounted (Workbench:, Work:, RAM:). Verify all volumes appear in the ListBrowser with correct media status.
- **Removable test**: Configure a floppy device (DF0:) in WinUAE with no disk inserted. Verify no "insert disk" requester appears -- device shows "No media" status.
- **Orphan test**: Manually add a bogus entry to `Workbench:.backdrop` (e.g., `:NonExistent/Fake`). Verify it appears as an orphan (red status). Test "Remove Selected" -- verify `.backdrop.bak` is created and entry is removed from `.backdrop`.
- **Layout test**: Leave out several icons, run "Tidy Layout". Verify device icons appear first in sorted rows, left-outs below, properly spaced. Reboot WinUAE to confirm positions persist.
- **RAM test**: Verify RAM: icon repositioning prompts for ENVARC persistence. After confirming and rebooting, verify the RAM icon retains its position.
- **Write-protected test**: Set a volume to read-only. Verify "Remove" and "Tidy" are disabled for that device's entries.
- **Memory**: Enable `DEBUG_MEMORY_TRACKING`, run scan + tidy + close. Check memory log for leaks.

---

## Decisions

- **Single window**: Audit and layout live in one "Backdrop cleaner..." window (not separate features)
- **Simple backup**: `.backdrop.bak` file copy rather than LHA archive (files are tiny text)
- **No programmatic Leave Out/Put Away**: AmigaOS has no API for this; we only edit `.backdrop` files directly
- **Reboot notification**: Modal requester after `.backdrop` changes, explaining reboot is needed
- **AppIcons (iconified programs)**: Detected, displayed as informational, but skipped for repositioning -- out of scope
- **Grid spacing**: Derived from existing Workbench preference settings where possible, with sensible fallback defaults
- **Existing `loadLeftOutIcons()` in `icon_misc.c`**: Left untouched -- the new `backdrop_parser.c` is a separate, more capable implementation for this feature; the old one continues serving the layout processor's needs
- **RAM icon persistence**: Auto-persist with user confirmation via requester after repositioning
- **Removable media**: Silent probe with `pr_WindowPtr` suppression, no user interaction required

---

## Key File References

| Existing File | Relevance |
|---|---|
| `src/GUI/main_window.c` | Add button (~line 790), add event handler (~line 2148) |
| `src/GUI/main_window_reaction.h` | Add gadget ID enum + GID define |
| `src/GUI/RestoreBackups/restore_window.c` | Reference pattern for ListBrowser window |
| `src/icon_types/tool_cache.c` | `pr_WindowPtr` suppression pattern (lines 458-468) |
| `src/icon_misc.c` | Existing `.backdrop` parser (lines 45-152) |
| `src/DOS/getDiskDetails.c/.h` | Existing `DeviceInfo` struct and `GetDeviceInfo()` |
| `src/GUI/gui_utilities.c` | `ShowReActionRequester()`, `safe_set_window_pointer()` |
| `docs/Icon-issues-deep-research-report.md` | Original research and feature specification |
| `docs/AutoDocs/dos.doc` | `LockDosList`, `NextDosEntry`, `IsFileSystem` reference |
| `docs/AutoDocs/wb.doc` | `UpdateWorkbench`, AppIcon reference |
| `docs/AutoDocs/icon.doc` | `GetDiskObject`, `PutIconTagList`, `do_Type` values |

## New Files to Create

| New File | Purpose |
|---|---|
| `src/DOS/device_scanner.c` | Volume enumeration and media probing |
| `src/DOS/device_scanner.h` | Device scanner types and API |
| `src/backups/backdrop_parser.c` | Enhanced `.backdrop` file parsing and editing |
| `src/backups/backdrop_parser.h` | Backdrop parser types and API |
| `src/layout/workbench_layout.c` | WB screen icon positioning calculations |
| `src/layout/workbench_layout.h` | WB layout types and API |
| `src/GUI/BackdropCleaner/backdrop_window.c` | ReAction GUI window |
| `src/GUI/BackdropCleaner/backdrop_window.h` | Window data struct and API |
