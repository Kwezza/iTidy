# iTidy Preference Settings Guide

This document explains all user-configurable settings in iTidy and how they affect icon layout and window sizing.

---

## Main Window Settings

### Folder Path
**Type:** String  
**Default:** (empty)  
**Description:** The path to the folder containing icons to be tidied. Can be a drawer, volume, or any valid AmigaDOS path.

### Recursive Subdirectories
**Type:** Checkbox  
**Default:** OFF  
**Description:** When enabled, iTidy will process the selected folder and all subdirectories recursively.

### Enable Backup
**Type:** Checkbox  
**Default:** OFF  
**Description:** Creates LHA compressed backups of icon positions before making changes. Backups are stored in the backup catalog system.

### Enable Icon Upgrade
**Type:** Checkbox  
**Default:** OFF  
**Description:** Upgrades icon formats during processing (e.g., updating old icon formats to newer standards while preserving appearance).

---

## Advanced Window Settings

### Max Window Width

**Type:** Cycle Gadget  
**Options:** Auto, 30%, 50%, 70%, 90%, 100%  
**Default:** Auto (0)  
**Preference Field:** `prefs->maxWindowWidthPct`

**What It Does:**

Sets the **maximum width constraint** for folder windows as a percentage of the screen width. This is an **upper limit**, not a target width.

- **Auto (0):** No width constraint - iTidy uses the full screen width when calculating layouts
- **30% - 100%:** Limits the window to the specified percentage of screen width

**Important:** This setting constrains the maximum number of columns iTidy will use. The actual window width may be **less** than this percentage if the Aspect Ratio setting determines that fewer columns produce a better layout shape.

**Example:**
- Screen width: 640 pixels
- Max Window Width: 90%
- Effective constraint: 576 pixels maximum
- Actual result: Window might be 400 pixels wide if aspect ratio prefers 4 columns over 6

**Interaction with Aspect Ratio:**

The aspect ratio algorithm operates **within** the width constraint set by this preference. If a narrower layout produces a better aspect ratio match, iTidy will choose the narrower layout even if there's room for more columns.

**Use Cases:**
- **Auto:** Best for most situations - lets iTidy use the full screen
- **70-90%:** Useful for preventing extremely wide windows on large screens
- **30-50%:** Creates narrow windows for specialized workflows or small icon sets

---

### Aspect Ratio

**Type:** Cycle Gadget  
**Options:** Square (1:1), Portrait (3:4), Classic (4:3), Wide (16:10), Widescreen (16:9), Ultrawide (21:9), Custom  
**Default:** Classic (4:3)  
**Preference Field:** `prefs->aspectRatio` (stored as fixed-point: ratio × 1000)

**Preset Values:**
- **Square (1:1):** 1000 (perfectly square grid)
- **Portrait (3:4):** 750 (taller than wide)
- **Classic (4:3):** 1333 (traditional monitor ratio)
- **Wide (16:10):** 1600 (common widescreen)
- **Widescreen (16:9):** 1778 (modern widescreen)
- **Ultrawide (21:9):** 2333 (ultra-wide monitor)
- **Custom:** User-defined width:height ratio

**What It Does:**

Determines the **target shape** (width:height ratio) of the icon grid. iTidy tries different column counts and selects the one that produces a window shape **closest to the specified aspect ratio**.

**How It Works:**

1. iTidy calculates the average icon size (width × height)
2. For each possible column count (within min/max constraints):
   - Calculates rows needed: `rows = totalIcons / columns`
   - Estimates window dimensions: `width = columns × avgWidth`, `height = rows × avgHeight`
   - Calculates actual ratio: `actualRatio = width / height`
   - Measures difference from target: `diff = |actualRatio - targetRatio|`
3. Selects the column count with the **smallest difference**

**Example:**

For 24 icons with aspect ratio set to **Wide (16:10 / 1.6)**:
- 3 columns × 8 rows = 1.6 ratio ✓ **Best match**
- 4 columns × 6 rows = 1.3 ratio (less optimal)
- 6 columns × 4 rows = 2.0 ratio (too wide)
- Result: iTidy chooses 3 columns

**Interaction with Max Window Width:**

The aspect ratio algorithm **respects** the Max Window Width constraint:
1. Max Window Width sets the **ceiling** for columns
2. Aspect Ratio picks the **optimal** column count within that ceiling
3. The narrower constraint wins

**Use Cases:**
- **Square (1:1):** Icon collections that look best in compact grids
- **Classic (4:3):** General purpose, matches traditional Amiga screen ratios
- **Wide/Widescreen:** Modern workflows, takes advantage of wide displays
- **Portrait (3:4):** Vertical toolbars or narrow side panels
- **Custom:** Fine-tuned control for specific aesthetic requirements

---

### Overflow Mode

**Type:** Cycle Gadget  
**Options:** Horizontal, Vertical, Both  
**Default:** Horizontal  
**Preference Field:** `prefs->overflowMode`

**What It Does:**

Determines what happens when icons **cannot fit** within the screen boundaries using the ideal aspect ratio layout.

**Modes:**

1. **Horizontal (Expand Width)**
   - Maximizes rows to fit screen height
   - Allows window to extend beyond screen width (horizontal scrollbar)
   - **Use case:** Prefer vertical compactness, don't mind scrolling sideways

2. **Vertical (Expand Height)**
   - Maximizes columns to fit screen width
   - Allows window to extend beyond screen height (vertical scrollbar)
   - **Use case:** Prefer seeing all columns at once, accept vertical scrolling

3. **Both (Maintain Aspect Ratio)**
   - Maintains the target aspect ratio even if window exceeds screen size
   - May require both horizontal and vertical scrolling
   - **Use case:** Aesthetic consistency is more important than fitting on screen

**Example:**

With 100 icons and Wide (16:10) aspect ratio:
- **Horizontal:** Might create 20 columns × 5 rows (fits height, scrolls width)
- **Vertical:** Might create 5 columns × 20 rows (fits width, scrolls height)
- **Both:** Might create 12 columns × 8 rows (maintains ~1.6 ratio, scrolls both)

---

### Vertical Alignment

**Type:** Cycle Gadget  
**Options:** Top, Center, Bottom  
**Default:** Center  
**Preference Field:** `prefs->textAlignment`

**What It Does:**

Controls the vertical alignment of icon text labels beneath icons.

**Not Fully Implemented:** This setting is prepared for future functionality where icon text positioning can be adjusted. Currently has minimal effect on most layouts.

---

### Reverse Sort

**Type:** Checkbox  
**Default:** OFF  
**Preference Field:** `prefs->reverseSort`

**What It Does:**

When enabled, reverses the sort order determined by the Sort Order and Sort By settings in the main window.

**Examples:**
- Sort By: Name, Sort Order: A-Z → **Reverse:** Z-A
- Sort By: Date, Sort Order: Newest First → **Reverse:** Oldest First
- Sort By: Size, Sort Order: Largest First → **Reverse:** Smallest First

---

## Main Window Settings (Additional)

### Center Icons

**Type:** Checkbox  
**Default:** ON  
**Preference Field:** `prefs->centerIconsInColumn`

**What It Does:**

When enabled, centers icons horizontally within their column width. This creates visually balanced layouts when icons have different widths.

**How It Works:**

Without centering (OFF):
- All icons align to the left edge of their column
- Creates a ragged appearance when icons vary in width

With centering (ON):
- Each icon is centered within its column's maximum width
- The column width is determined by the widest icon in that column
- Narrower icons are positioned in the center of their column space

**Example:**

Column with 3 icons (widths: 50px, 80px, 50px):
- **Centering OFF:** All icons left-aligned (0px offset)
- **Centering ON:** 
  - Wide icon (80px): Centered in 80px column (0px offset)
  - Narrow icons (50px): Centered in 80px column (15px offset from left)

**Interaction with Optimize Columns:**

This setting works **together** with "Optimize columns":
- **Center Icons** controls **horizontal positioning** within columns
- **Optimize Columns** controls **column width calculation**

Best used: **Both enabled** for professional, balanced layouts

---

### Optimize Columns

**Type:** Checkbox  
**Default:** ON  
**Preference Field:** `prefs->useColumnWidthOptimization`

**IMPORTANT:** This setting **only works when "Center Icons" is enabled**. If "Center Icons" is OFF, iTidy uses a simpler layout algorithm that doesn't support column optimization.

**What It Does:**

When enabled (and "Center Icons" is ON), iTidy calculates **individual width for each column** based on the widest icon in that column. This creates more compact layouts by preventing narrow columns from being unnecessarily wide.

**How It Works:**

**Without optimization (OFF):**
- All columns use the **same width** (based on average icon width)
- Wastes horizontal space when some columns contain only narrow icons
- Simpler calculation, uniform appearance

**With optimization (ON):**
- Each column width = **widest icon in that column**
- Columns with narrow icons take less horizontal space
- More efficient use of screen real estate
- Creates variable-width columns

**Algorithm (Phase 1):**

1. Calculate average icon width
2. Determine number of columns (from aspect ratio or screen width)
3. **Assign icons to columns** (icon 0→col 0, icon 1→col 1, etc.)
4. **Find maximum width per column:**
   ```c
   for each icon:
       col = icon_index % num_columns
       if icon_width > columnWidths[col]:
           columnWidths[col] = icon_width
   ```
5. Calculate X position for each column start:
   ```c
   columnXPositions[0] = startX
   for col = 1 to num_columns:
       columnXPositions[col] = columnXPositions[col-1] + 
                              columnWidths[col-1] + 
                              iconSpacingX
   ```

**Algorithm (Phase 2):**

Position each icon within its column using the calculated column width.

**Example:**

4 columns with icons of varying widths:

**Without Optimization (Uniform Width = 80px):**
```
Col 0    Col 1    Col 2    Col 3
[Icon]   [Icon]   [Icon]   [Icon]   ← Each column 80px wide
 50px     80px     50px     60px
 (wasted) (fits)  (wasted) (wasted)
```
Total width: 4 × 80px = 320px

**With Optimization (Variable Widths):**
```
Col 0  Col 1    Col 2  Col 3
[Icon] [Icon]   [Icon] [Icon]
 50px   80px     50px   60px
```
Total width: 50 + 80 + 50 + 60 = 240px (25% narrower!)

**Performance:**

- **Maximum 2 calculation passes** to determine column widths
- Efficient even with 1000+ icons
- Slightly more complex than uniform width, but negligible overhead

**Interaction with Center Icons:**

These two settings work together:
- **Optimize Columns:** Calculates the **width of each column**
- **Center Icons:** Positions icons **within** those column widths

**Recommended combinations:**

| Optimize | Center | Result |
|----------|--------|--------|
| ON | ON | **Best:** Compact, balanced layout (recommended) |
| ON | OFF | Compact but left-aligned (ragged appearance) |
| OFF | ON | Uniform columns, centered icons (wastes space) |
| OFF | OFF | Uniform columns, left-aligned (classic grid) |

**Use Cases:**

- **Optimize ON:** Icon collections with varying widths (games, tools, documents)
- **Optimize OFF:** Icons of similar width (prefer uniform appearance)

**Technical Notes:**

- Optimization respects `maxWindowWidthPct` constraint
- If optimized layout exceeds screen width, iTidy reduces column count and recalculates
- Works with both aspect ratio and overflow mode settings

---

## How Settings Work Together

### Layout Calculation Flow

When you click **Apply**, iTidy follows this process:

1. **Scan Directory**
   - Loads all icons from the specified folder
   - Calculates average icon dimensions

2. **Sort Icons**
   - Apply Sort By criteria (Name, Type, Date, Size)
   - Apply Sort Priority (Folders First, Files First, Mixed)
   - Apply Sort Order (Horizontal, Vertical)
   - Apply Reverse Sort (if enabled)

3. **Determine Maximum Columns**
   - If `maxWindowWidthPct > 0`: Calculate max columns from `(screenWidth × percentage) / iconWidth`
   - If `maxWindowWidthPct = 0` (Auto): Calculate max columns from full screen width

4. **Find Optimal Columns (Aspect Ratio)**
   - Try column counts from `minIconsPerRow` to calculated max
   - For each column count:
     - Calculate rows: `rows = totalIcons / columns`
     - Calculate window ratio: `ratio = (columns × avgWidth) / (rows × avgHeight)`
     - Measure difference from `aspectRatio` target
   - Select column count with smallest difference

5. **Check Screen Fit**
   - Does the calculated layout fit on screen?
   - **YES:** Use the ideal aspect ratio layout
   - **NO:** Apply overflow strategy (Horizontal/Vertical/Both)

6. **Position Icons**
   - **Choose positioning algorithm:**
     - If `centerIconsInColumn = ON`: Use column-centered algorithm
     - If `centerIconsInColumn = OFF`: Use standard row-based algorithm
   
   - **If Optimize Columns is ON** (and using column-centered algorithm):
     - **Phase 1: Calculate column widths**
       - Assign icons to columns
       - Find widest icon per column
       - Calculate X position for each column start
     - **Phase 2: Position icons**
       - Place each icon in its column
       - Center narrow icons within their column width
       - Apply vertical text alignment (Top/Middle/Bottom)
   
   - **If Optimize Columns is OFF**:
     - Use uniform column width (based on average)
     - Position icons in standard grid
   
   - Apply spacing settings
   - Save new positions to .info files

7. **Resize Window** (if enabled)
   - Adjust drawer window to match icon grid dimensions
   - Respects screen boundaries

8. **Update Workbench** (if beta feature enabled)
   - Find open folder window on Workbench
   - Move/resize to match saved geometry

9. **Open Folders** (if beta feature enabled)
   - Auto-open processed folders via Workbench
   - Useful during recursive operations

### Priority Order

When settings conflict, this is the priority:

1. **Physical Constraints:** Screen size and icon dimensions (cannot be violated)
2. **Max Window Width:** Hard limit on window width (if set)
3. **Aspect Ratio:** Preferred shape (operates within width constraint)
4. **Overflow Mode:** Fallback behavior when ideal layout doesn't fit

---

## Common Scenarios

### "I want icons to fill 90% of my screen width"

**Settings:**
- Max Window Width: **90%**
- Aspect Ratio: **Widescreen (16:9)** or **Ultrawide (21:9)**

The wide aspect ratio will encourage more columns, and 90% sets the upper limit.

**Note:** If you have few icons, they may not reach 90% width because the aspect ratio algorithm prioritizes shape over width.

---

### "I want compact, square grids"

**Settings:**
- Max Window Width: **Auto**
- Aspect Ratio: **Square (1:1)**
- Overflow Mode: **Vertical**

This creates compact square layouts that expand downward if needed.

---

### "I want tall, narrow windows"

**Settings:**
- Max Window Width: **30%**
- Aspect Ratio: **Portrait (3:4)**
- Overflow Mode: **Vertical**

Creates narrow columns that extend vertically.

---

### "I don't care about aspect ratio, just fit everything on screen"

**Settings:**
- Max Window Width: **100%**
- Aspect Ratio: **Auto** (choose Square as a neutral option)
- Overflow Mode: **Horizontal** or **Vertical** (depending on preference)

This maximizes screen usage in one dimension.

---

### "I want the most compact, professional layout"

**Settings:**
- Max Window Width: **Auto**
- Aspect Ratio: **Wide (16:10)** or **Widescreen (16:9)**
- Center Icons: **ON**
- Optimize Columns: **ON**
- Overflow Mode: **Vertical**

This creates efficient layouts that:
- Use column-specific widths (no wasted space)
- Center icons within their columns (balanced appearance)
- Prefer wide layouts that fit screen width
- Allow vertical scrolling for large folders

**Why this works:**
- **Optimize Columns** eliminates wasted horizontal space
- **Center Icons** prevents ragged left-aligned appearance
- **Wide aspect ratio** maximizes columns while maintaining readability
- Result: Professional, compact layouts that use screen space efficiently

---

### "I want classic Workbench appearance"

**Settings:**
- Max Window Width: **55%** (DEFAULT)
- Aspect Ratio: **Classic (4:3)**
- Center Icons: **ON**
- Optimize Columns: **ON**
- Overflow Mode: **Horizontal**

This recreates the traditional Amiga Workbench look with modern optimizations.

---

## Technical Details

### Fixed-Point Aspect Ratio Storage

Aspect ratios are stored as integers scaled by 1000 to avoid floating-point math on 68k systems:

- `aspectRatio = 1600` means 1.6:1 (16:10)
- `aspectRatio = 1333` means 1.333:1 (4:3)
- `aspectRatio = 1000` means 1.0:1 (1:1 square)

### Calculation Formula

```c
// Calculate actual ratio of a layout
actualRatio = (columns × avgIconWidth × 1000) / (rows × avgIconHeight)

// Find difference from target
difference = abs(actualRatio - targetAspectRatio)

// Select layout with smallest difference
```

### Max Width Constraint

```c
// Determine effective width
if (maxWindowWidthPct > 0 && maxWindowWidthPct < 100)
{
    effectiveWidth = (screenWidth × maxWindowWidthPct) / 100;
}
else
{
    effectiveWidth = screenWidth;  // Use full screen
}

// Calculate maximum columns
maxColumns = (effectiveWidth - margins) / (avgIconWidth + spacing);
```

---

## Future Enhancements

The following settings are planned or partially implemented:

- **Vertical Alignment:** Full control over icon text positioning
- **Custom Aspect Ratios:** UI for entering custom width:height values
- **Per-Folder Presets:** Save different settings for different folders
- **Live Preview:** See layout changes before applying

---

## Version Information

- **Document Version:** 1.0
- **iTidy Version:** 2.0
- **Last Updated:** November 25, 2025
- **Author:** iTidy Development Team

---

## See Also

- `docs/ASPECT_RATIO_AND_OVERFLOW_FEATURE.md` - Technical implementation details
- `docs/DEVELOPMENT_LOG.md` - Change history
- `.github/copilot-instructions.md` - Developer guidance
