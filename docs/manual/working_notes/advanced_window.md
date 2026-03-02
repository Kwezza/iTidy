# Advanced Settings

**Rebuild window:** `advanced_settings`

**Window title:** iTidy - Advanced Settings
**Navigation:** Main Window > Advanced... button, or Main Window > Settings menu > Advanced Settings...

The Advanced Settings window gives you finer control over how iTidy arranges icons and sizes windows. Settings are organised into five tabs: Layout, Density, Limits, Columns & Groups, and Filters & Misc.

Changes made here are applied when you click OK, or discarded if you click Cancel (or close the window, or press Escape). To save settings permanently to disk, use the Presets > Save menu on the main window.

This window is modal -- the main window does not respond to input while Advanced Settings is open.

---

## Layout Tab

### Layout Aspect
*Rebuild IDENT: `adv_ch_layout_aspect` | Name: "Layout Aspect" | Type: Chooser (cycle/popup) gadget*

Sets the target width-to-height shape for drawer windows. This controls the overall proportions iTidy aims for when resizing windows to fit their contents.

| Option | Description |
|--------|-------------|
| Tall (0.75) | Tall, narrow windows. |
| Square (1.0) | Roughly square windows. |
| Compact (1.3) | Slightly wider than tall. |
| Classic (1.0) | Traditional Workbench-like proportions. |
| Wide (2.0) | Wider, landscape-oriented windows. **(Default)** |

**Hint:** "Sets the target width-to-height proportions for drawer windows when iTidy resizes them."

### When Full
*Rebuild IDENT: `adv_ch_layout_when_full` | Name: "When Full" | Type: Chooser (cycle/popup) gadget*

Controls what iTidy does when a drawer has more icons than fit comfortably in the window at its target proportions.

| Option | Description |
|--------|-------------|
| Expand Horizontally | Adds more columns. You will scroll left and right. **(Default)** |
| Expand Vertically | Adds more rows. You will scroll up and down. |
| Expand Both | Balances expansion in both directions. |

**Hint:** "Controls how iTidy expands a drawer window when it has more icons than fit at the target proportions."

### Align Vertically
*Rebuild IDENT: `adv_ch_layout_align_vertically` | Name: "Align Vertically" | Type: Chooser (cycle/popup) gadget*

Sets how icons are aligned vertically when a row contains icons of different heights.

| Option | Description |
|--------|-------------|
| Top | Aligns icons to the top of the row. |
| Middle | Aligns icons to the middle of the row. **(Default)** |
| Bottom | Aligns icons to the bottom of the row. |

**Hint:** "Sets how icons are aligned vertically within a row when icons of different heights appear in the same row."

---

## Density Tab

### Horizontal Spacing
*Rebuild IDENT: `adv_int_iconspacing_x` | Name: "Horizontal Spacing" | Type: Integer input gadget*

Sets the horizontal gap between icons, in pixels. Lower values pack icons tighter; higher values give icons more breathing room.

- Range: 0 to 20
- Default: 8

**Hint:** "Sets the horizontal gap between icons in pixels. Lower values pack icons tighter; higher values give more breathing room."

### Vertical Spacing
*Rebuild IDENT: `adv_int_iconspacing_y` | Name: "Vertical Spacing" | Type: Integer input gadget*

Sets the vertical gap between icons, in pixels.

- Range: 0 to 20
- Default: 8

**Hint:** "Sets the vertical gap between icons in pixels. Lower values pack icons tighter; higher values give more breathing room."

---

## Limits Tab

### Icons Per Row: Min
*Rebuild IDENT: `adv_int_icons_per_row_min` | Name: "Icons Per Row: Min" | Type: Integer input gadget*

Sets the minimum number of columns in a drawer window. This prevents drawers from becoming one long vertical list when a window is narrow or icons are wide.

- Range: 0 to 30
- Default: 2

**Hint:** "Sets the minimum number of columns in a drawer window, preventing icons from being arranged in one long vertical list."

### Auto-Calc Max Icons
*Rebuild IDENT: `adv_cb_icons_per_row_autolayout` | Name: "Auto-Calc Max Icons" | Type: Checkbox gadget*

When enabled, iTidy automatically calculates the maximum number of columns based on the window width and screen size. This is the recommended setting for most use cases.

Default: On

When this is enabled, the Max field below is disabled (greyed out).

**Hint:** "When enabled, the maximum number of columns is calculated automatically based on window width and screen size."

### Max
*Rebuild IDENT: `adv_int_icons_per_row_max` | Name: "Max" | Type: Integer input gadget*

Sets the maximum number of columns. This field is only used when Auto-Calc Max Icons is turned off.

- Range: 2 to 40
- Default: 10 (shown as placeholder when Auto-Calc is enabled)

**Hint:** "Sets the maximum number of columns. Only used when \"Auto-Calc Max Icons\" is turned off."

### Max Window Width
*Rebuild IDENT: `adv_ch_layout_max_width` | Name: "Max Window Width" | Type: Chooser (cycle/popup) gadget*

Limits how wide drawer windows may become, as a percentage of screen width. This prevents windows from stretching across the entire screen on high-resolution displays.

| Option | Description |
|--------|-------------|
| Auto | No fixed limit; iTidy decides automatically. |
| 30% | Window can use up to 30% of screen width. |
| 50% | Window can use up to 50% of screen width. |
| 70% | Window can use up to 70% of screen width. **(Default)** |
| 90% | Window can use up to 90% of screen width. |
| 100% | Window can use the full screen width. |

**Hint:** "Limits how wide drawer windows may become, as a percentage of screen width. Prevents windows from filling the entire screen."

---

## Columns & Groups Tab

### Column Layout
*Rebuild IDENT: `adv_cb_column_layout` | Name: "Column Layout" | Type: Checkbox gadget*

When enabled, iTidy uses a column-based layout where icons are arranged in strict columns. When disabled, icons use a free-flow row-based layout.

Default: Off

**Hint:** "When enabled, icons are arranged in strict vertical columns. When disabled, a free-flow row-based layout is used."

### Gap Between Groups
*Rebuild IDENT: `adv_ch_layout_gap_between_groups` | Name: "Gap Between Groups" | Type: Chooser (cycle/popup) gadget*

Controls the spacing between icon groups when the main window's Grouping option is set to "Grouped By Type". Each group (e.g. Drawers, Tools, Projects) is separated by a visual gap.

| Option | Description |
|--------|-------------|
| Small | Small gap between groups. |
| Medium | Medium gap between groups. **(Default)** |
| Large | Large gap between groups. |

**Note:** If a group contains no icons (for example, a folder with no Tools), that group is skipped and no gap is added.

**Hint:** "Controls the spacing between icon groups when \"Grouping\" on the main window is set to \"Grouped By Type\"."

---

## Filters & Misc Tab

### Auto-Fit Columns
*Rebuild IDENT: `adv_cb_optimize_columns` | Name: "Auto-Fit Columns" | Type: Checkbox gadget*

When enabled, each column is sized to fit its widest icon, rather than forcing all columns to the same width. This produces a more compact and visually balanced layout.

Default: On

**Hint:** "When enabled, each column is sized to fit its widest icon, producing a more compact and visually balanced layout."

### Reverse Sort Order
*Rebuild IDENT: `adv_cb_reverse_sort` | Name: "Reverse Sort Order" | Type: Checkbox gadget*

Reverses the current sort direction. The effect depends on the sort mode selected on the main window:

- Name: Z to A instead of A to Z
- Date: Newest first instead of oldest first
- Size: Largest first instead of smallest first

Default: Off

**Hint:** "Reverses the sort direction. Applies to name (Z-A), date (newest first), and size (largest first) sorting."

### Strip NewIcon Borders
*Rebuild IDENT: `adv_cb_strip_newicon_borders` | Name: "Strip NewIcon Borders" | Type: Checkbox gadget*

Removes the border that Workbench draws around NewIcon images by marking them as frameless. This does not alter the icon's pixel data -- it sets a flag that tells Workbench not to draw the surrounding border frame.

To restore the border on an individual icon afterwards, open the icon's Information window in Workbench (select the icon, then choose "Information..." from the Icons menu), switch to "Icon Image" from the menu, and untick the "Frameless?" checkbox.

Requires icon.library v44 or later.

Default: Off

**Hint:** "Removes the Workbench-drawn border from NewIcon images by setting them frameless. Reversible via Icon Information > Icon Image > Frameless."

### Skip Hidden Folders
*Rebuild IDENT: `adv_cb_skip_hidden` | Name: "Skip Hidden Folders" | Type: Checkbox gadget*

When enabled, iTidy skips folders that do not have a corresponding `.info` file during recursive processing. Folders without `.info` files are considered "hidden" from Workbench and are typically not visible to the user, so processing them serves no purpose.

Default: On

**Hint:** "When enabled, folders without a .info file are skipped during recursive processing, as they are not visible in Workbench."

---

## Buttons

### Defaults
*Rebuild IDENT: `adv_btn_defaults` | Name: "Defaults" | Type: Button gadget*

Resets all settings in this window to their factory default values. A confirmation requester is shown before the reset takes place. This does not save anything to disk -- it only resets the gadgets in this window.

**Hint:** "Resets all settings in this window to their factory defaults. A confirmation requester is shown before the reset takes place."

### Cancel
*Rebuild IDENT: `adv_btn_cancel` | Name: "Cancel" | Type: Button gadget*

Discards all changes and returns to the main window. The original settings are preserved unchanged.

**Hint:** "Discards all changes and returns to the main window. The original settings are preserved unchanged."

### OK
*Rebuild IDENT: `adv_btn_ok` | Name: "OK" | Type: Button gadget*

Accepts the current settings and closes the window. The changes are applied to the active session but are not saved to disk. To save permanently, use Presets > Save on the main window.

**Hint:** "Accepts the current settings and closes the window. Changes are applied to the session but not saved to disk."

---

## Default Values Summary

| Setting | Default |
|---------|---------|
| Layout Aspect | Wide (2.0) |
| When Full | Expand Horizontally |
| Align Vertically | Middle |
| Horizontal Spacing | 8 |
| Vertical Spacing | 8 |
| Icons Per Row: Min | 2 |
| Auto-Calc Max Icons | On |
| Max | 10 (placeholder, not used when Auto-Calc is on) |
| Max Window Width | 70% |
| Column Layout | Off |
| Gap Between Groups | Medium |
| Auto-Fit Columns | On |
| Reverse Sort Order | Off |
| Strip NewIcon Borders | Off |
| Skip Hidden Folders | On |
