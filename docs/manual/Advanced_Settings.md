# iTidy Advanced Settings

This document explains all options available in the **Advanced Settings** window, accessible by clicking "More Options..." in the main iTidy window.

---

## Layout Aspect Ratio

**Control Type:** Cycle gadget  
**Options:** Tall (0.75), Square (1.0), Compact (1.3), Classic (1.6), Wide (2.0), Ultrawide (2.4)  
**Default:** Wide (2.0)

Controls the target width-to-height ratio for folder windows. iTidy calculates the optimal number of icon columns to achieve a window shape close to this ratio.

| Preset | Ratio | Description |
|--------|-------|-------------|
| Tall (0.75) | 0.75:1 | Very tall, narrow windows |
| Square (1.0) | 1:1 | Square windows |
| Compact (1.3) | 1.3:1 | Slightly wider than tall |
| Classic (1.6) | 1.6:1 | Traditional Workbench proportions |
| Wide (2.0) | 2:1 | Wide, modern proportions |
| Ultrawide (2.4) | 2.4:1 | Very wide windows |

**How it works:** For each folder, iTidy analyzes the icons' average dimensions and calculates how many columns best approximate the selected aspect ratio. It then arranges icons in that grid pattern.

---

## Overflow Strategy

**Control Type:** Cycle gadget  
**Options:** Expand Horizontally, Expand Vertically, Expand Both  
**Default:** Expand Horizontally

Determines how iTidy handles folders with too many icons to fit on screen at the target aspect ratio.

| Strategy | Behavior |
|----------|----------|
| Expand Horizontally | Adds more columns (wider window), user scrolls horizontally |
| Expand Vertically | Adds more rows (taller window), user scrolls vertically |
| Expand Both | Expands in both directions as needed |

**Example:** If you have 50 icons in a folder and the ideal layout would exceed the screen height:
- **Expand Horizontally** - Adds more columns to reduce rows, making the window wider
- **Expand Vertically** - Keeps fewer columns, allowing the window to extend beyond screen height
- **Expand Both** - Balances expansion in both directions

---

## Icon Spacing: X and Y

**Control Type:** Slider (0-20 pixels each)  
**Default:** 8 pixels for both X and Y

Sets the horizontal (X) and vertical (Y) gap between icons in the layout grid.

- **X Spacing:** Horizontal gap between icon columns
- **Y Spacing:** Vertical gap between icon rows

Lower values create tighter, more compact layouts. Higher values provide more breathing room between icons.

---

## Icons per Row: Min

**Control Type:** Integer input  
**Default:** 2  
**Range:** 1-99

Sets the minimum number of icon columns. Prevents very narrow "1×N" layouts where icons stack vertically in a single column.

**Use case:** Set to 2 or higher to ensure folders always display icons side-by-side rather than in a single vertical stack.

---

## Auto Max Icons

**Control Type:** Checkbox  
**Default:** Checked (enabled)

When enabled, iTidy automatically calculates the maximum columns based on screen width and the **Max Window Width** setting. This is the recommended mode.

When disabled, you must manually specify the maximum columns using the **Max** input field.

---

## Icons per Row: Max

**Control Type:** Integer input  
**Default:** 5 (when Auto is disabled)  
**Range:** 1-99

Sets the maximum number of icon columns. Only active when **Auto Max Icons** is unchecked.

**Note:** This field is grayed out when Auto Max Icons is enabled.

---

## Max Window Width

**Control Type:** Cycle gadget  
**Options:** Auto, 30%, 50%, 70%, 90%, 100%  
**Default:** Auto

Limits the maximum folder window width as a percentage of screen width. Only applies when **Auto Max Icons** is enabled.

| Setting | Behavior |
|---------|----------|
| Auto | Uses a preset default (typically 55% of screen width) |
| 30%-100% | Limits window to that percentage of screen width |

**Example:** On a 640-pixel wide screen with "50%" selected, folder windows will never exceed 320 pixels wide.

---

## Align Icons Vertically

**Control Type:** Cycle gadget  
**Options:** Top, Middle, Bottom  
**Default:** Middle

Controls vertical alignment of icons within each row when icons have different heights.

| Alignment | Behavior |
|-----------|----------|
| Top | Icons align to the top of each row |
| Middle | Icons center vertically in each row |
| Bottom | Icons align to the bottom of each row |

**When it matters:** Rows containing icons of varying heights (e.g., a small disk icon next to a tall drawer icon) will position icons according to this setting.

---

## Reverse Sort (Z→A)

**Control Type:** Checkbox  
**Default:** Unchecked (A→Z sorting)

When enabled, reverses the sort order:
- Alphabetical sorting becomes Z→A instead of A→Z
- Date sorting becomes newest-first instead of oldest-first
- Size sorting becomes largest-first instead of smallest-first

---

## Optimize Column Widths

**Control Type:** Checkbox  
**Default:** Checked (enabled)

When enabled, iTidy calculates the optimal width for each column based on the widest icon in that column, rather than using a uniform width for all columns.

**Benefits:**
- More compact layouts when icon widths vary significantly
- Better use of horizontal space
- Narrower columns where icons are small

**When to disable:** If you prefer a perfectly uniform grid where all columns are the same width.

---

## Column Layout (centered columns)

**Control Type:** Checkbox  
**Default:** Checked (enabled)

When enabled, icons are centered horizontally within their grid cell (column). This creates a visually balanced layout where icons don't all hug the left edge of their column.

**Visual difference:**
- **Enabled:** Icons centered within their column space
- **Disabled:** Icons left-aligned within their column

This works in conjunction with **Optimize Column Widths** to create aesthetically pleasing layouts.

---

## Skip Hidden Folders

**Control Type:** Checkbox  
**Default:** Checked (enabled)

When enabled, iTidy skips folders that don't have an associated `.info` file (icon file). These "hidden" folders are typically system directories or temporary folders that aren't meant to appear on the Workbench.

**When to disable:** If you want to process all folders, including those without visible icons on the Workbench.

**Note:** This setting only affects recursive processing. The root folder specified in the main window is always processed.

---

## Strip NewIcon Borders

**Control Type:** Checkbox  
**Default:** Unchecked (disabled)  
**Requirement:** icon.library v44 or higher (Workbench 3.5+)

When enabled, removes the border/frame around icons during processing. This applies to:
- Classic NewIcons format icons
- OS 3.5+ color icons with borders
- Standard icons with decorative borders

**⚠️ WARNING:** This is a **one-way operation**. Once borders are stripped, they cannot be automatically restored. The checkbox is disabled (grayed out) on systems with icon.library versions below 44.

**Recommendation:** Always enable backups before using this option. The backup system can restore original icons if needed.

---

## Beta Options... Button

Opens a separate window with experimental features that are still under development. These features may change or be removed in future versions.

**Current beta features include:**
- **Open folders after processing** - Automatically opens processed folders on Workbench
- **Find and update open windows** - Moves/resizes already-open folder windows to match saved geometry
- **Logging controls** - Log level selection, memory logging, performance logging toggles
- **Default tool validation** - Validates that default tools exist in system PATH
- **Default tool backup** - Creates CSV backup before modifying default tools

---

## OK and Cancel Buttons

- **OK** - Saves all settings and closes the Advanced Settings window. Settings are applied when you click "Apply" in the main iTidy window.
- **Cancel** - Discards any changes made in the Advanced Settings window and closes it.

---

## Technical Notes

### Aspect Ratio Calculation

Aspect ratios are stored internally as fixed-point integers scaled by 1000:
- Wide (2.0) = 2000 internally
- Classic (1.6) = 1600 internally
- Square (1.0) = 1000 internally

This avoids floating-point math, which is slow on 68000 CPUs without an FPU.

### Preference Storage

All advanced settings are stored in the `LayoutPreferences` structure and persisted through the application session. The relevant fields include:

| Setting | Preference Field |
|---------|------------------|
| Layout Aspect Ratio | `aspectRatio` (int, scaled by 1000) |
| Overflow Strategy | `overflowMode` (enum: 0=Horiz, 1=Vert, 2=Both) |
| Icon Spacing X | `iconSpacingX` (UWORD, pixels) |
| Icon Spacing Y | `iconSpacingY` (UWORD, pixels) |
| Min Icons per Row | `minIconsPerRow` (UWORD) |
| Max Icons per Row | `maxIconsPerRow` (UWORD, 0=Auto) |
| Max Window Width | `maxWindowWidthPct` (UWORD, 0=Auto) |
| Align Icons Vertically | `textAlignment` (enum: 0=Top, 1=Middle, 2=Bottom) |
| Reverse Sort | `reverseSort` (BOOL) |
| Optimize Column Widths | `useColumnWidthOptimization` (BOOL) |
| Column Layout | `centerIconsInColumn` (BOOL) |
| Skip Hidden Folders | `skipHiddenFolders` (BOOL) |
| Strip NewIcon Borders | `stripNewIconBorders` (BOOL) |
