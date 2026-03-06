# Exclude Paths Window — Working Notes

**Source**: `src/GUI/exclude_paths_window.c`, `src/GUI/exclude_paths_window.h`
**Title bar**: "DefIcons Exclude Paths"
**Opened from**: Main window -> Settings -> DefIcons Categories... -> DefIcons Excluded Folders...; also from DefIcons Creation Settings window
**Modal**: Effectively modal (blocks caller via synchronous event loop)
**Resizable**: Yes (initial 600x300)
**Keyboard shortcuts**: A (Add), R (Remove), M (Modify), D (Reset to Defaults), O (OK), C (Cancel)

---

## Purpose

Manages the list of directory paths that should be excluded from DefIcons icon creation. When iTidy scans a volume, any folder matching an exclude path is skipped during icon generation. Paths can be stored as absolute paths (e.g. `Workbench:C`) or as portable `DEVICE:` patterns (e.g. `DEVICE:C`) that match the same relative path on any volume.

---

## Key Concepts

### DEVICE: Placeholder

Paths can be stored with a `DEVICE:` prefix instead of a real volume name. At scan time, `DEVICE:` is substituted with whatever volume is being scanned, making the exclude list portable across different drives. For example, `DEVICE:C` would match `Workbench:C`, `Work:C`, or any other volume's `C` directory.

When adding or modifying a path, if the selected directory is on the same volume as the current scan folder, iTidy asks whether to store it as `DEVICE:` or as an absolute path.

### Limits

- Maximum 32 exclude paths
- Each path up to 255 characters

---

## Window Layout

Simple two-part vertical layout:

- **List area** (fills window): Single-column ListBrowser showing all exclude paths
- **Button bar** (bottom): Add, Remove, Modify, and Reset to Defaults on the left; OK and Cancel pushed to the right by a spacer

---

## Gadgets

### Path List (ListBrowser)

Single column titled "Exclude Path". Displays all currently configured exclude paths. Single-select — clicking a row selects it for use by Remove and Modify. Supports 0 to 32 entries.
**Hint:** "Shows all excluded directory paths. Click a row to select it for use by \"Remove\" or \"Modify\"."
### Add...

Opens an ASL directory requester ("Select Directory to Exclude") starting in the current scan folder. After selecting a directory:
1. The path is resolved to an absolute path (assigns like `SYS:` are resolved to real volume names).
2. If the selected path is on the same volume as the scan folder, a requester asks whether to store it as a `DEVICE:` pattern or absolute path.
3. The path is added to the list. Duplicate paths (case-insensitive) are silently rejected.
4. Silently fails if the list is already full (32 entries).

**Hint:** "Opens a directory requester to add a new path to the exclude list. Offers to store it as a portable DEVICE: pattern or as an absolute path."

### Remove

Removes the currently selected path from the list. Does nothing if no row is selected. No confirmation requester.

**Hint:** "Removes the selected path from the exclude list. Does nothing if no row is selected."

### Modify...

Opens an ASL directory requester ("Modify Exclude Path") pre-set to the currently selected path. Same DEVICE:/absolute logic as Add. Rejects the change if it would create a duplicate. Does nothing if no row is selected.

**Hint:** "Opens a directory requester to change the selected exclude path. Does nothing if no row is selected."

### Reset to Defaults

Shows a confirmation requester ("Reset exclude list to defaults? Current list will be lost.") with Reset/Cancel buttons. If confirmed, clears all paths and repopulates with the 17 default entries:

`DEVICE:Fonts`, `DEVICE:Locale`, `DEVICE:Classes`, `DEVICE:Libs`, `DEVICE:C`, `DEVICE:Rexxc`, `DEVICE:T`, `DEVICE:L`, `DEVICE:Devs`, `DEVICE:Resources`, `DEVICE:System`, `DEVICE:Storage`, `DEVICE:Expansion`, `DEVICE:Kickstart`, `DEVICE:Env`, `DEVICE:Envarc`, `DEVICE:Prefs/Env-Archive`

**Hint:** "Replaces the current exclude list with the 17 default AmigaOS system directory paths. A confirmation requester is shown first."

### OK

Accepts the current list and closes the window. The caller commits the changes to the global preferences.

**Hint:** "Accepts the current exclude list and closes the window."

### Cancel

Discards changes and closes the window. Closing via the window close gadget or Ctrl+C also cancels.

**Hint:** "Discards all changes to the exclude list and closes the window."

---

## Button State Logic

All buttons are always enabled regardless of selection state or list count. Remove and Modify silently do nothing if no row is selected. Add silently fails if the list is at capacity.

---

## Requesters

### Reset Confirmation
- Title: "Reset to Defaults"
- Body: "Reset exclude list to defaults? Current list will be lost."
- Buttons: Reset / Cancel
- Image: Warning

### DEVICE: Choice
- Title: "Store Path As"
- Shown when the added/modified path is on the same volume as the scan folder
- Explains that DEVICE: is portable across volumes
- Buttons: DEVICE: / Absolute / Cancel
- Image: Question

---

## Notes for Manual

- The DEVICE: placeholder system is the core feature — explain the portability benefit clearly.
- Default paths cover common AmigaOS system directories that rarely need icons.
- The window edits a working copy; changes only take effect if OK is clicked (when opened from the main window). Note: when opened from the DefIcons Creation window, changes apply immediately — this is a known inconsistency.
- No visual feedback when Add is rejected (list full) or Remove/Modify is clicked without a selection.
