# DefIcons Categories

**Window title:** DefIcons: Icon Creation Setup
**Navigation:** Main Window > Icon Creation... > DefIcons tab > Icon Creation Setup... button, or Main Window > Settings menu > DefIcons Categories... > DefIcons Categories...

This window lets you choose which file types iTidy should create icons for during DefIcons processing. It shows a tree view of all file types defined in the system's DefIcons preferences, with checkboxes to enable or disable each type. You can also view and change the default tool assigned to each type.

The type tree is read from `ENV:deficons.prefs`, which is managed by the "DefaultIcons" Preferences tool in the Workbench Prefs drawer. If the DefIcons system is not available (e.g. on pre-3.2 systems), this window cannot be used.

This window is modal -- the calling window does not respond to input while it is open.

---

## Type Tree

The main area of the window shows a hierarchical tree of file types. Each top-level entry represents a category (e.g. music, picture, tool), and expanding a category reveals the individual file types within it.

### How types are organised

| Level | Description | Example |
|-------|-------------|---------|
| Root category | Top-level grouping. Shown as bold expandable nodes. | music, picture, tool |
| File type | Individual type within a category. Has a checkbox. | mod, med, ilbm, png, jpeg |
| Sub-type | Deeper specialisation of a file type. No checkbox. | jfif (under jpeg) |

Only file types at the second level have checkboxes. Root categories and deeper sub-types do not have individual checkboxes.

### Enabling and disabling types

- A **checked** type means iTidy will create icons for files of that type.
- An **unchecked** type means iTidy will skip files of that type during icon creation.

Click a checkbox to toggle it. The change takes effect immediately in the working preferences.

**Hint:** "Check a type to create icons for files of that type during processing. Uncheck to skip it."

### Default disabled types

By default, the following categories are disabled because creating icons for them is rarely useful or could affect system files:

- **tool** -- Executable programs (already have icons in most cases)
- **prefs** -- Preferences files
- **iff** -- Generic IFF files
- **key** -- Keyfiles
- **kickstart** -- Kickstart ROM images

All other types are enabled by default.

---

## Buttons

### Select All

Enables all file types for icon creation. All checkboxes in the tree are ticked.

**Note:** This may create icons for any files that do not already have an icon, including system files. Use with care.

**Hint:** "Enables all file types for icon creation. All checkboxes in the tree are ticked."

### Select None

Disables all file types. All checkboxes in the tree are unticked. No icons will be created for any type.

**Hint:** "Disables all file types. No icons will be created for any file type during icon creation."

### Show Default Tools

Scans the default icon templates in `ENVARC:Sys/` and displays the default tool (the program Workbench would launch) next to each type name. For example:

- `music  [MultiView]`
- `  mod  [EaglePlayer]`

If a type does not have its own template icon, no tool is shown for that entry (it inherits from its parent category at runtime, but this is not displayed).

This is a one-shot action -- click it to scan and display the tools. The display updates automatically after changing a default tool.

**Hint:** "Scans the DefIcons template icons in ENVARC:Sys/ and shows the assigned default tool next to each type name."

### Change Default Tool...

Opens a file requester to select a new default tool for the currently selected type. The selected program is written directly to the type's template icon file in `ENVARC:Sys/`.

If the selected type does not yet have its own template icon (it inherits from its parent), iTidy will automatically create one by cloning the parent's template icon before setting the new tool.

**Important:** Default tool changes are applied immediately and saved directly to disk. They are NOT subject to the OK/Cancel buttons. The Cancel button's tooltip explicitly notes this.

If no type is selected in the tree, a warning is shown asking you to select one first.

**Hint:** "Opens a file requester to assign a new default tool to the selected type. Changes are saved to disk immediately and are not affected by Cancel."

### OK

Accepts the current type selections (enabled/disabled checkboxes) and closes the window. The changes are applied to the working preferences.

**Hint:** "Accepts the current type selections and closes the window. Checkbox changes are applied to the working preferences."

### Cancel

Closes the window without saving type selection changes. Any checkbox changes are discarded.

**Note:** Default tool changes made via the "Change Default Tool..." button are already saved to disk and are not affected by Cancel.

**Hint:** "Closes the window without saving type selection changes. Note: default tool changes made via \"Change Default Tool...\" are already saved to disk."

---

## Default Values Summary

| Setting | Default |
|---------|---------|
| tool | Disabled |
| prefs | Disabled |
| iff | Disabled |
| key | Disabled |
| kickstart | Disabled |
| All other types | Enabled |
