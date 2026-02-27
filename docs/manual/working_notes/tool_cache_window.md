# Default Tool Analysis Window — Working Notes

**Source**: `src/GUI/DefaultTools/tool_cache_window.c` (3129 lines), `tool_cache_window.h`
**Title bar**: "iTidy - Default Tool Analysis"
**Opened from**: Main window -> Tools button (or menu)
**Modal**: Effectively modal (blocks caller via event loop)
**Resizable**: Yes (initial 550x400, min 450x300)
**Menus**: Yes — Project, File, View (with keyboard shortcuts)

---

## Purpose

Scans a directory tree to find all default tools referenced by icons, then displays them in a sortable list. Allows the user to replace incorrect or missing tools (batch or single file), export reports, and restore backed-up default tools. The tool cache can be saved to and loaded from disk for reuse.

---

## Window Layout

```
+----------------------------------------------------+
| Folder: [path field................] [Scan]         |
| [Show All v]                                       |
| +------------------------------------------------+ |
| | Tool              | Files | Status             | |  <- upper list (45%)
| | MultiView         |    23 | EXISTS             | |
| | IconEdit          |     5 | MISSING            | |
| +------------------------------------------------+ |
| +------------------------------------------------+ |
| | Tool: MultiView                                | |  <- details (35%)
| | Status: EXISTS  |  Version: 45.10              | |
| | ----------------------------------------       | |
| | SYS:Utilities/MultiView                        | |
| | Work:Projects/MyApp                            | |
| +------------------------------------------------+ |
| [Replace Tool (Batch)...] [Replace Tool (Single)..] |
| [Restore Default Tools Backups...]        [Close]  |
+----------------------------------------------------+
```

---

## Menus

### Project Menu

| Item | Shortcut | Action |
|------|----------|--------|
| New | Amiga+N | Clears the tool cache, empties both lists, resets filter |
| Open... | Amiga+O | Opens a `.dat` file to load a previously saved tool cache |
| Save | Amiga+S | Saves the current cache to the default location (`PROGDIR:userdata/DTools/toolcache.dat`) |
| Save as... | Amiga+A | Saves the cache to a user-chosen location (ASL file requester) |
| Close | Amiga+C | Closes the window |

### File Menu

| Item | Shortcut | Action |
|------|----------|--------|
| Export list of tools | Amiga+T | Exports a text report of all tools |
| Export list of files and tools | Amiga+F | Exports a detailed report of all files and their tools |

### View Menu

| Item | Shortcut | Action |
|------|----------|--------|
| System PATH... | Amiga+P | Shows the list of directories iTidy searches for default tools |

---

## Gadgets

### Folder (GetFile)

Directory selection field. Initialised from the current scan folder in preferences. Drawers only (no files). The selected folder becomes the root for the Scan operation.

### Scan

Starts a recursive directory scan of the selected folder. Opens a progress window showing scan statistics. During scanning:
- A busy pointer is shown on the tool cache window
- The progress window shows per-category statistics (valid/invalid tools with file counts)
- After scan completes, the progress Cancel button changes to "Close"
- The user must close the progress window before returning to the tool list

After scanning, the tool list is populated and the filter chooser is enabled.

### Filter Chooser

Filters the tool list display. Disabled until tool data is loaded.

| Option | Shows |
|--------|-------|
| Show All | Every tool found (default) |
| Show Valid Only | Only tools that exist on disk |
| Show Missing Only | Only tools that were not found |

Changing the filter clears the current selection and details panel.

### Tool List (Upper ListBrowser)

Displays all discovered default tools in three sortable columns:

| Column | Content |
|--------|---------|
| Tool | Tool name/path (truncated with `/../` notation if long) |
| Files | Number of icons referencing this tool |
| Status | "EXISTS" or "MISSING" |

Click a column header to sort by that column. Click again to reverse the sort order. Selecting a row populates the details panel below.

### Details Panel (Lower ListBrowser)

Shows information about the selected tool:

- **Row 1**: Tool name
- **Row 2**: Status (EXISTS/MISSING) and version string if available
- **Row 3**: Separator line
- **Rows 4+**: List of icon files that reference this tool (paths truncated to 70 characters)

If a tool has more than 200 referencing files, only the first 200 are shown with a note.

### Replace Tool (Batch)...

Opens the Default Tool Update window in batch mode. Replaces the default tool in ALL icons that reference the selected tool. Only enabled when a tool is selected in the upper list.

### Replace Tool (Single)...

Opens the Default Tool Update window in single mode. Replaces the default tool in ONE specific icon file. Only enabled when a file row is selected in the details panel (rows 3+, not the header rows).

### Restore Default Tools Backups...

Opens the Default Tool Restore window where previously backed-up default tools can be restored. Always enabled. If any restores are performed, the tool cache is cleared on return and the user is prompted to re-scan.

### Close

Closes the window.

---

## Button State Logic

| Gadget | Enabled When |
|--------|-------------|
| Filter Chooser | Tool data exists (cache not empty) |
| Replace Tool (Batch) | A tool is selected in the upper list |
| Replace Tool (Single) | A file row is selected in the details panel (row index >= 3) |
| Scan, Restore, Close | Always enabled |

---

## Tool Cache File Format

Binary format (version 2) with `"ITIDYTOOLCACHE"` header. Contains the scan folder path, followed by each tool's name, existence status, full path, version string, hit count, referencing file count, and all referencing file paths. Default save location: `PROGDIR:userdata/DTools/toolcache.dat`.

File loading validates the header magic, version (must be 2), and applies limits: max 10,000 tools, max 200 files per tool, string lengths capped at 256-1024 characters.

---

## View System PATH

Shows a numbered list of all directories that iTidy searches when looking for default tools. The PATH is built during the Scan operation. If no scan has been performed, an information requester explains this.

---

## Notes for Manual

- The Scan button always forces recursive scanning regardless of the main window's recursive setting.
- Tool paths are truncated using `/../` notation for display — this is purely cosmetic.
- The Files column sorts numerically (not alphabetically) since it uses native integer display.
- After using Restore Default Tools Backups, the cache is cleared and must be rebuilt with a new scan.
- Save/Open allows tool cache data to be preserved between sessions without re-scanning.
- The Export functions create text reports useful for documenting which tools are used across a volume.
