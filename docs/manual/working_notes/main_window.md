# Main Window

**Rebuild window:** `main_window`

**Window title:** iTidy v2.0 - Icon Cleanup Tool
**Navigation:** This is the first window shown when iTidy launches.

The main window is where you choose what to tidy and set the basic options. It is designed to make the common "point it at a folder and tidy it" job quick, without getting in the way.

From here you can access all of iTidy's features: advanced layout settings, icon creation, default tool analysis, backup restoration, and the tidying process itself.

---

## Requirements & Startup

iTidy requires:

- AmigaOS Workbench 3.2 or newer (ReAction GUI)
- 68000 CPU or better
- At least 1 MB of free memory
- At least 1 MB of free storage space in the installation location (more as backups accumulate)
- For backup and restore features: LhA must be installed in `C:`

When launched, iTidy checks the Workbench version, initialises logging, loads system preferences (icon font, Workbench settings, IControl settings), and parses the DefIcons type tree cache if Workbench 3.2's `ENV:Sys/deficons.prefs` is available.

### ToolTypes

iTidy reads the following tooltypes from its program icon when launched from Workbench:

| ToolType | Values | Default | Description |
|----------|--------|---------|-------------|
| DEBUGLEVEL | 0-4 | 4 (Disabled) | Sets the log level. 0=Debug, 1=Info, 2=Warning, 3=Error, 4=Disabled. This overrides any log level stored in a loaded preferences file. |
| LOADPREFS | File path | (none) | Automatically loads a saved preferences file at startup. Paths without a device name are resolved relative to `PROGDIR:userdata/Settings/`. |
| PERFLOG | YES / NO | NO | Enables performance timing logs for benchmarking. |

---

## Folder Selection

**Folder to tidy**
*Rebuild IDENT: `main_gf_target_path` | Name: "Folder to tidy:" | Type: File requester gadget*

A file requester gadget that lets you select the drawer (folder) you want iTidy to process. Clicking the folder button opens a requester that shows drawers only (not files). The selected path is shown in the field and is read-only -- you must use the requester to change it.

Default: `SYS:`

**Hint:** "Click to open a directory requester and choose the drawer iTidy will process."

**Tip:** You can select a whole partition (e.g. `Work:`) to process everything on that volume.

---

## Tidy Options

### Grouping
*Rebuild IDENT: `main_ch_sort_primary` | Name: "Grouping" | Type: Chooser (cycle/popup) gadget*

Sets how icons are grouped before sorting. This determines the overall arrangement order.

| Option | Description |
|--------|-------------|
| Folders First | Drawer icons are laid out before file icons. **(Default)** |
| Files First | File icons are laid out before drawer icons. |
| Mixed | Folders and files are sorted together with no grouping. |
| Grouped By Type | Icons are arranged in visual groups (e.g. Drawers, Tools, Projects, Other). Each group is laid out independently and separated by a configurable gap (see Advanced Settings). |

**Note:** When "Grouped By Type" is selected, the Sort By option is disabled because the group order is fixed. Icons within each group are still sorted by name.

**Hint:** "Sets how icons are grouped before sorting. Choose folders first, files first, mixed, or grouped by file type."

### Sort By
*Rebuild IDENT: `main_ch_sort_secondary` | Name: "Sort By" | Type: Chooser (cycle/popup) gadget*

Selects the sort key used within the chosen grouping mode.

| Option | Description |
|--------|-------------|
| Name | Sort alphabetically by icon name. **(Default)** |
| Type | Sort by file type / kind. |
| Date | Sort by file modification date. |
| Size | Sort by file size. |

This option is disabled when Grouping is set to "Grouped By Type".

**Hint:** "Sets the sort order within the current grouping mode. Disabled when \"Grouping\" is set to \"Grouped By Type\"."

### Include Subfolders
*Rebuild IDENT: `main_cb_cleanup_subfolders` | Name: "Include Subfolders" | Type: Checkbox gadget*

When enabled, iTidy also processes all subfolders under the selected folder. This is the option you would use for tidying a whole partition or a large folder tree in one pass.

Default: Off

**Note:** On classic hardware, very large directory trees can take a while to process.

**Hint:** "When enabled, all subfolders under the selected folder are also processed. Use with care on large directory trees."

### Create Icons During Tidy
*Rebuild IDENT: `main_cb_create_new_icons` | Name: "Create Icons During Tidy" | Type: Checkbox gadget*

When enabled, iTidy will create new icons for files and folders that do not already have `.info` files, using the DefIcons system. This happens during the tidying process -- files that gain new icons are then included in the layout.

You can configure which file types get icons, thumbnail settings, and other creation options via the **Icon Creation...** button.

Default: Off

**Hint:** "When enabled, iTidy creates new icons for files that do not already have .info files, using the DefIcons system."

### Back Up Layout Before Changes
*Rebuild IDENT: `main_cb_backup_icons` | Name: "Back Up Layout Before Changes" | Type: Checkbox gadget*

When enabled, iTidy creates an LhA backup of each folder's `.info` files before making any changes. This gives you something to roll back to later using the Restore backups feature.

Default: Off

**Requirement:** LhA must be installed in `C:`. If LhA is not found when you click Start with backups enabled, iTidy will warn you and offer to continue without backups or cancel.

This checkbox is kept in sync with the menu option at Settings > Backups > Back Up Layout Before Changes. Changing one updates the other.

**Hint:** "When enabled, an LhA backup of each folder's .info files is created before making changes. Requires LhA in C:."

### Window Position
*Rebuild IDENT: `main_ch_positioning` | Name: "Window Position" | Type: Chooser (cycle/popup) gadget*

Controls where drawer windows are placed after iTidy resizes them. This only affects windows that iTidy actually changes.

| Option | Description |
|--------|-------------|
| Center Screen | Centres the window on the Workbench screen. **(Default)** |
| Keep Position | Keeps the window at its current location. |
| Near Parent | Places the window slightly down and right of the parent window (cascading style). |
| No Change | Resizes the window but does not move it. |

**Hint:** "Controls where drawer windows are placed after iTidy resizes them. Only affects windows that iTidy actually changes."

---

## Tool Buttons

### Advanced...
*Rebuild IDENT: `main_btn_advanced` | Name: "Advanced..." | Type: Button gadget*

Opens the Advanced Settings window for finer control over layout, density, limits, columns, grouping gaps, and filtering. Changes made in the Advanced Settings window are applied when you click OK, or discarded if you click Cancel.

**Tooltip:** "Opens Advanced Settings for finer control over layout and sizing."

**Hint:** "Opens the Advanced Settings window for finer control over layout, density, limits, columns, grouping gaps, and filtering."

### Fix default tools...
*Rebuild IDENT: `main_btn_default_tools` | Name: "Fix Default Tools..." | Type: Button gadget*

Opens the Default Tool Analysis window, which scans icons for missing or invalid default tools (the program that Workbench runs when you double-click an icon). From there you can review, batch-replace, or single-replace broken tool paths.

The scan targets the currently selected folder and respects the Include Subfolders setting.

**Tooltip:** "Scans icons for missing or invalid Default Tools. Lets you fix them or batch-replace one tool with another."

**Hint:** "Scans icons for missing or invalid default tools. Lets you review and fix broken tool paths in batch or one at a time."

### Restore backups...
*Rebuild IDENT: `main_btn_restore_backups` | Name: "Restore Backups..." | Type: Button gadget*

Opens the Restore Backups window, which shows previous backup runs created by iTidy. From there you can restore icon positions and window layouts to a previous state, view which folders were included in a run, or delete old backups.

**Note:** This restores icon/layout backups only. To undo default tool changes, use the Restore Default Tools feature within the Fix Default Tools window instead.

**Tooltip:** "Restores icon positions and window snapshots from iTidy backups. Only available if you previously enabled backups."

**Hint:** "Opens the Restore Backups window to restore icon positions and window layouts from a previous iTidy backup run."

### Icon Creation...
*Rebuild IDENT: `main_btn_icon_settings` | Name: "Icon Creation..." | Type: Button gadget*

Opens the Icon Creation Settings window, where you can configure how iTidy creates new icons for files that don't have them. Settings include thumbnail generation (for images and text files), folder icon creation mode, and DefIcons category management.

**Tooltip:** "Opens the icon creation settings (thumbnails, text previews, folder icons)"

**Hint:** "Opens the Icon Creation Settings window to configure thumbnail generation, text previews, folder icons, and DefIcons categories."

---

## Action Buttons

### Start
*Rebuild IDENT: `main_btn_start` | Name: "Start" | Type: Button gadget*

Begins processing using the current settings. When you click Start:

1. All current GUI settings are saved to the active preferences.
2. If backups are enabled, iTidy checks that LhA is available. If LhA is not found, a warning is shown with the option to continue without backups or cancel.
3. A progress window opens showing the current status.
4. iTidy processes the selected folder (and subfolders if enabled), arranging icons and resizing windows.
5. When processing completes, a summary is shown in the progress window. The Cancel button changes to Close so you can review the results before dismissing the window.

**Tooltip:** "Starts tidying the selected folder using the current settings."

**Hint:** "Starts tidying the selected folder using the current settings. A progress window shows status during processing."

### Exit
*Rebuild IDENT: `main_btn_exit` | Name: "Exit" | Type: Button gadget*

Closes iTidy.

**Tooltip:** "Closes iTidy."

**Hint:** "Closes iTidy."

---

## Menus

### Presets

| Menu Item | Shortcut | Description |
|-----------|----------|-------------|
| Reset To Defaults | Amiga+N | Resets all settings to their default values. The current log level is preserved. |
| Open Preset... | Amiga+O | Opens a file requester to load a previously saved preferences file. The default location is `PROGDIR:userdata/Settings/`. The loaded file must have a valid `ITIDYPREFS` header. A success or error message is shown after loading. |
| Save Preset | Amiga+S | Saves the current settings to the last-used file path. If no file has been saved or loaded yet, this behaves the same as Save Preset As. |
| Save Preset As... | Amiga+A | Opens a file requester to choose where to save the current settings. The default filename is `iTidy.prefs` and the default location is `PROGDIR:userdata/Settings/`. If the file already exists, you are asked whether to replace it. |
| Quit | Amiga+Q | Closes iTidy. |

### Settings

| Menu Item | Description |
|-----------|-------------|
| Advanced Settings... | Opens the Advanced Settings window (same as the Advanced button). |
| **DefIcons Categories...** (submenu) | |
| - DefIcons Categories... | Opens the DefIcons category selection window, where you can choose which file types should receive icons during DefIcons processing. Shows a tree view of all available file types with checkboxes. |
| - Preview Icons... | Opens the Text Templates management window for configuring text file preview templates. |
| - DefIcons Excluded Folders... | Opens the Exclude Paths window for managing folders that should be skipped during DefIcons icon creation. |
| **Backups...** (submenu) | |
| - Back Up Layout Before Changes? | Toggle checkmark. Enables or disables automatic LhA backups before tidying. This is kept in sync with the "Back Up Layout Before Changes" checkbox on the main window. |
| **Logging** (submenu) | |
| - Disabled (Recommended) | Turns off all logging. **(Default)** |
| - Debug | Most verbose logging level -- logs everything. |
| - Info | Logs informational messages and above. |
| - Warning | Logs warnings and errors only. |
| - Error | Logs errors only. |
| - Open Log Folder | Opens the `PROGDIR:logs/` folder as a Workbench drawer window, creating it if it does not exist. |

The five logging level options are mutually exclusive -- selecting one deselects the others.

### Tools

| Menu Item | Description |
|-----------|-------------|
| **Restore...** (submenu) | |
| - Restore Layouts... | Opens the Restore Backups window (same as the Restore backups button). |
| - Restore Default Tools... | Opens a standalone window for restoring default tool changes from iTidy's backup system. This is separate from the icon/layout backup system. |

### Help

| Menu Item | Description |
|-----------|-------------|
| iTidy Guide... | Opens `PROGDIR:iTidy2.guide` using AmigaGuide. A warning is shown if the guide file is not found. |
| About | Shows an information requester with the version number, description, and copyright notice. |

---

## Window Behaviour

- The window opens at position (50, 30) on the Workbench public screen.
- It can be resized. Minimum size is 350 x 200 pixels.
- Closing the window (via the close gadget) exits iTidy.
- Pressing Ctrl+C also exits cleanly.
- Gadget help tooltips are available for all interactive gadgets.

---

## Default Values Summary

| Setting | Default |
|---------|---------|
| Folder | SYS: |
| Grouping | Folders First |
| Sort By | Name |
| Include Subfolders | Off |
| Create Icons During Tidy | Off |
| Back Up Layout Before Changes | Off |
| Window Position | Center Screen |
| Logging | Disabled |

---

## Tips

- The main window is intended for quick "point it at a folder and tidy it" runs. If you need more control over layout and window sizing, use Advanced.
- All changes apply to the selected folder and (if enabled) everything underneath it.
- If you are experimenting, enable backups and start with a small test folder first.
- Save your preferred settings as a preset so you can quickly reload them later.
- You can set the LOADPREFS tooltype in the program icon to automatically load your favourite settings every time iTidy starts.
