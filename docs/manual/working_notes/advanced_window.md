# Advanced Settings

**Window title:** iTidy - Advanced Settings
**Navigation:** Main Window > Advanced... button, or Main Window > Settings menu > Advanced Settings...

The Advanced Settings window gives you finer control over how iTidy arranges icons and sizes windows. Settings are organised into five tabs: Layout, Density, Limits, Columns & Groups, and Filters & Misc.

Changes made here are applied when you click OK, or discarded if you click Cancel (or close the window, or press Escape). To save settings permanently to disk, use the Presets > Save menu on the main window.

This window is modal -- the main window does not respond to input while Advanced Settings is open.

---

## Layout Tab

### Layout Aspect

Sets the target width-to-height shape for drawer windows. This controls the overall proportions iTidy aims for when resizing windows to fit their contents.

| Option | Description |
|--------|-------------|
| Tall (0.75) | Tall, narrow windows. |
| Square (1.0) | Roughly square windows. |
| Compact (1.3) | Slightly wider than tall. |
| Classic (1) | Traditional Workbench-like proportions. |
| Wide (2.0) | Wider, landscape-oriented windows. **(Default)** |

### When Full

Controls what iTidy does when a drawer has more icons than fit comfortably in the window at its target proportions.

| Option | Description |
|--------|-------------|
| Expand Horizontally | Adds more columns. You will scroll left and right. **(Default)** |
| Expand Vertically | Adds more rows. You will scroll up and down. |
| Expand Both | Balances expansion in both directions. |

### Align Vertically

Sets how icons are aligned vertically when a row contains icons of different heights.

| Option | Description |
|--------|-------------|
| Top | Aligns icons to the top of the row. |
| Middle | Aligns icons to the middle of the row. **(Default)** |
| Bottom | Aligns icons to the bottom of the row. |

---

## Density Tab

### Horizontal Spacing

Sets the horizontal gap between icons, in pixels. Lower values pack icons tighter; higher values give icons more breathing room.

- Range: 0 to 20
- Default: 8

### Vertical Spacing

Sets the vertical gap between icons, in pixels.

- Range: 0 to 20
- Default: 8

---

## Limits Tab

### Icons Per Row: Min

Sets the minimum number of columns in a drawer window. This prevents drawers from becoming one long vertical list when a window is narrow or icons are wide.

- Range: 0 to 30
- Default: 2

### Auto-Calc Max Icons

When enabled, iTidy automatically calculates the maximum number of columns based on the window width and screen size. This is the recommended setting for most use cases.

Default: On

When this is enabled, the Max field below is disabled (greyed out).

### Max

Sets the maximum number of columns. This field is only used when Auto-Calc Max Icons is turned off.

- Range: 2 to 40
- Default: 10 (shown as placeholder when Auto-Calc is enabled)

### Max Window Width

Limits how wide drawer windows may become, as a percentage of screen width. This prevents windows from stretching across the entire screen on high-resolution displays.

| Option | Description |
|--------|-------------|
| Auto | No fixed limit; iTidy decides automatically. |
| 30% | Window can use up to 30% of screen width. |
| 50% | Window can use up to 50% of screen width. |
| 70% | Window can use up to 70% of screen width. **(Default)** |
| 90% | Window can use up to 90% of screen width. |
| 100% | Window can use the full screen width. |

---

## Columns & Groups Tab

### Column Layout

When enabled, iTidy uses a column-based layout where icons are arranged in strict columns. When disabled, icons use a free-flow row-based layout.

Default: Off

### Gap Between Groups

Controls the spacing between icon groups when the main window's Grouping option is set to "Grouped By Type". Each group (e.g. Drawers, Tools, Projects) is separated by a visual gap.

| Option | Description |
|--------|-------------|
| Small | Small gap between groups. |
| Medium | Medium gap between groups. **(Default)** |
| Large | Large gap between groups. |

**Note:** If a group contains no icons (for example, a folder with no Tools), that group is skipped and no gap is added.

---

## Filters & Misc Tab

### Auto-Fit Columns

When enabled, each column is sized to fit its widest icon, rather than forcing all columns to the same width. This produces a more compact and visually balanced layout.

Default: On

### Reverse Sort Order

Reverses the current sort direction. The effect depends on the sort mode selected on the main window:

- Name: Z to A instead of A to Z
- Date: Newest first instead of oldest first
- Size: Largest first instead of smallest first

Default: Off

### Strip Newicons Borders (Permanent)

Strips the black border from NewIcons during processing. This requires icon.library v44 or later.

**Warning:** This permanently modifies the affected icons. If you might want to restore the original appearance later, enable the backup option on the main window before running iTidy.

Default: Off

### Skip Hidden Folders

When enabled, iTidy skips folders that do not have a corresponding `.info` file during recursive processing. Folders without `.info` files are considered "hidden" from Workbench and are typically not visible to the user, so processing them serves no purpose.

Default: On

---

## Buttons

### Defaults

Resets all settings in this window to their factory default values. A confirmation requester is shown before the reset takes place. This does not save anything to disk -- it only resets the gadgets in this window.

### Cancel

Discards all changes and returns to the main window. The original settings are preserved unchanged.

### OK

Accepts the current settings and closes the window. The changes are applied to the active session but are not saved to disk. To save permanently, use Presets > Save on the main window.

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
| Strip Newicons Borders | Off |
| Skip Hidden Folders | On |
