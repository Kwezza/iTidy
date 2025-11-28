# iTidy Advanced Options Reference

This document describes all options available in the **Advanced Settings** window and **Beta Options** window of iTidy.

---

## Advanced Settings Window

The Advanced Settings window provides fine-grained control over icon layout, window behavior, and display options.

### Layout & Aspect Ratio

#### **Aspect Ratio**
Controls the target width-to-height ratio when automatically arranging icons.

**Available Presets:**
- **Tall (0.75)** - Creates narrower, taller window layouts
- **Square (1.0)** - Equal width and height proportions
- **Compact (1.3)** - Slightly wider than tall
- **Classic (1.6)** - Traditional icon layout ratio
- **Wide (2.0)** - Wide layout, fewer rows
- **Ultrawide (2.4)** - Very wide layout, minimal rows
- **Custom** - Define your own ratio using width:height values

**What it does:**  
When iTidy calculates the optimal number of columns and rows for your icons, it tries to achieve this target aspect ratio. For example, with 20 icons:
- **Tall (0.75)**: Might arrange as 4 columns × 5 rows (window is taller)
- **Square (1.0)**: Might arrange as 5 columns × 4 rows (roughly square)
- **Wide (2.0)**: Might arrange as 10 columns × 2 rows (wide window)

**Code Implementation:**  
The algorithm in `aspect_ratio_layout.c` (function `CalculateOptimalIconsPerRow`) iterates through different column counts to find the layout that best matches your target aspect ratio.

---

#### **Custom Width / Custom Height**
When **Aspect Ratio** is set to "Custom", these fields let you define your own ratio.

**Example:**
- Width: **16**, Height: **9** = Widescreen ratio (16:9 ≈ 1.78)
- Width: **4**, Height: **3** = Classic monitor ratio (4:3 ≈ 1.33)

**What it does:**  
iTidy calculates the aspect ratio as `(width × 1000) / height` using fixed-point math. This ratio is then used in the layout algorithm to determine optimal columns and rows.

**Enabled:** Only when "Aspect Ratio" is set to "Custom"

---

### Window Overflow Behavior

#### **Window Overflow**
Controls what happens when icons don't fit within the maximum allowed window size.

**Options:**
- **Expand Horizontally** - Add more columns, allow horizontal scrollbar if needed
- **Expand Vertically** - Add more rows, allow vertical scrollbar if needed
- **Expand Both** - Maintain aspect ratio even if both scrollbars are needed

**What it does:**  
When the calculated layout exceeds the maximum window size:

- **Expand Horizontally (`OVERFLOW_HORIZONTAL`):**  
  Maximizes the number of rows to use available screen height, then expands columns as needed. This creates wide windows that may require horizontal scrolling but minimize vertical scrolling.
  
  **Code:** `aspect_ratio_layout.c` lines 418-437  
  ```
  Height first: maximize rows, expand columns as needed
  finalRows = maxUsableHeight / (avgHeight + spacing)
  finalColumns = (totalIcons + finalRows - 1) / finalRows
  ```

- **Expand Vertically (`OVERFLOW_VERTICAL`):**  
  Maximizes the number of columns to use available screen width, then expands rows as needed. This creates tall windows that may require vertical scrolling but minimize horizontal scrolling.
  
  **Code:** `aspect_ratio_layout.c` lines 443-464  
  ```
  Width first: maximize columns, expand rows as needed
  finalColumns = maxUsableWidth / (avgWidth + spacing)
  finalRows = (totalIcons + finalColumns - 1) / finalColumns
  ```

- **Expand Both (`OVERFLOW_BOTH`):**  
  Maintains the target aspect ratio even if the window becomes larger than the screen in both dimensions. This prioritizes layout aesthetics over window size.
  
  **Code:** `aspect_ratio_layout.c` lines 470-484  
  ```
  Use ideal aspect ratio regardless of screen size
  finalColumns = idealColumns
  finalRows = idealRows
  ```

---

### Spacing Controls

#### **Horizontal Spacing**
Sets the number of pixels between icon columns.

**Range:** 0-20 pixels  
**Default:** Depends on system settings

**What it does:**  
Adds horizontal padding between columns of icons. Larger values create more "breathing room" between icons but make windows wider.

**Code:** Used in `layout_processor.c` when calculating icon positions:
```c
currentX += icon_width + prefs->iconSpacingX;
```

---

#### **Vertical Spacing**
Sets the number of pixels between icon rows.

**Range:** 0-20 pixels  
**Default:** Depends on system settings

**What it does:**  
Adds vertical padding between rows of icons. Larger values create more visual separation but make windows taller.

**Code:** Used in `layout_processor.c` when calculating icon positions:
```c
currentY += max_row_height + prefs->iconSpacingY;
```

---

### Column Constraints

#### **Min Icons/Row**
Sets the minimum number of icons per row.

**Range:** 1-99  
**Default:** 1

**What it does:**  
Ensures that even with a tall aspect ratio or small number of icons, you'll have at least this many icons per row. Prevents excessively narrow layouts.

**Code:** Used in `aspect_ratio_layout.c` as a constraint:
```c
if (finalColumns < prefs->minIconsPerRow)
    finalColumns = prefs->minIconsPerRow;
```

---

#### **Auto Max Icons/Row**
When checked, iTidy automatically determines the maximum icons per row based on screen width.

**What it does:**  
- **When CHECKED:** iTidy calculates the maximum number of icons that can fit horizontally based on the "Max Window Width" percentage setting. This allows dynamic layouts that adapt to your screen size.
  
- **When UNCHECKED:** You manually specify the exact maximum number of icons per row using the "Max Icons/Row" field below.

**Code:** In preferences, `maxIconsPerRow = 0` indicates auto mode.

---

#### **Max Icons/Row**
Manually sets the maximum number of icons per row.

**Range:** 1-99  
**Enabled:** Only when "Auto Max Icons/Row" is UNCHECKED

**What it does:**  
Hard limit on the number of columns. Useful for forcing multi-row layouts or limiting window width.

**Example:** With 30 icons and "Max Icons/Row" set to 10:
- Result: 10 columns × 3 rows (instead of potentially 15 × 2)

**Code:** Used in `aspect_ratio_layout.c` as a constraint:
```c
if (prefs->maxIconsPerRow > 0 && finalColumns > prefs->maxIconsPerRow)
    finalColumns = prefs->maxIconsPerRow;
```

---

#### **Max Window Width**
Sets the maximum window width as a percentage of screen width.

**Options:** Auto, 30%, 50%, 70%, 90%, 100%  
**Enabled:** Only when "Auto Max Icons/Row" is CHECKED

**What it does:**  
Limits how wide iTidy will make a window. Prevents giant windows from consuming the entire screen.

- **Auto (0%):** Uses a preset default based on aspect ratio preset
- **30% - 100%:** Limits window width to that percentage of screen width

**Code:** Used in `aspect_ratio_layout.c` to calculate maximum usable width:
```c
if (prefs->maxWindowWidthPct > 0) {
    maxUsableWidth = (screenWidth * prefs->maxWindowWidthPct) / 100;
}
```

---

### Display Options

#### **Vertical Alignment**
Controls how icons of different heights are aligned within the same row.

**Options:**
- **Top** - All icons align to the top of the row
- **Middle** - Icons are centered vertically within the row
- **Bottom** - All icons align to the bottom of the row

**What it does:**  
When icons in the same row have different heights (e.g., different sized icon images or multi-line text), this setting determines their vertical positioning relative to each other.

**Example:**  
In a row with a tall icon (60px) and a short icon (40px):
- **Top:** Both icons start at the same Y position
- **Middle:** Short icon is shifted down 10px to center it
- **Bottom:** Short icon is shifted down 20px to bottom-align

**Code:** Implemented in `layout_processor.c` lines 672-688:
```c
switch (prefs->textAlignment) {
    case TEXT_ALIGN_TOP:    adjustmentOffset = 0; break;
    case TEXT_ALIGN_MIDDLE: adjustmentOffset = (maxHeight - iconHeight) / 2; break;
    case TEXT_ALIGN_BOTTOM: adjustmentOffset = maxHeight - iconHeight; break;
}
icon_y += adjustmentOffset;
```

---

### Sorting & Filtering

#### **Reverse Sort (Z→A)**
When checked, reverses the sort order of icons.

**What it does:**  
- **Unchecked:** Normal sort order (A→Z for names, oldest→newest for dates)
- **Checked:** Reverse sort order (Z→A for names, newest→oldest for dates)

**Affects:** All sort modes (by name, by type, by date, by size)

**Code:** Implemented in `layout_processor.c` lines 1754-1757:
```c
if (prefs->reverseSort) {
    result = -result;  /* Negate comparison to reverse order */
}
```

---

#### **Optimize Column Widths**
When checked, each column uses only the width needed for its widest icon.

**What it does:**  
- **When CHECKED:** Columns have different widths based on their contents. This saves horizontal space when icons vary in width.
  
  **Example:** If column 1 has narrow icons (60px) and column 2 has wide icons (100px):
  - Column 1 width = 60px
  - Column 2 width = 100px
  - **Total saved:** 40px compared to uniform width

- **When UNCHECKED:** All columns use the same width (the widest icon in the entire layout). This creates a more uniform, grid-like appearance.

**Code:** Implemented in `layout_processor.c` lines 964-1003:
```c
if (prefs->useColumnWidthOptimization) {
    /* Calculate max width per column */
    for (i = 0; i < iconCount; i++) {
        col = i % numColumns;
        if (icon_width[i] > columnWidths[col])
            columnWidths[col] = icon_width[i];
    }
} else {
    /* Use uniform width for all columns */
    uniformWidth = max_icon_width_overall;
    for (col = 0; col < numColumns; col++)
        columnWidths[col] = uniformWidth;
}
```

---

#### **Skip Hidden Folders**
When checked, iTidy ignores subdirectories that don't have `.info` files (hidden folders).

**What it does:**  
During recursive processing, AmigaOS directories without `.info` files are typically "hidden" (invisible in Workbench). This option prevents iTidy from processing these invisible folders.

**Use Case:** System directories and temporary folders often lack `.info` files. Skipping them speeds up processing and avoids errors.

**Code:** Implemented in `layout_processor.c` lines 1490-1508:
```c
if (prefs->skipHiddenFolders) {
    snprintf(iconPath, sizeof(iconPath), "%s.info", subdir);
    iconLock = Lock(iconPath, ACCESS_READ);
    
    if (!iconLock) {
        /* No .info file - folder is hidden, skip it */
        printf("Skipping hidden folder: %s\n", subdir);
        continue;
    }
    UnLock(iconLock);
}
```

---

#### **Column Layout**
When checked, uses column-centered layout with optimized column widths.

**What it does:**  
Enables the advanced column-centering layout algorithm instead of the standard row-based layout.

**Key Differences:**
- **Standard Layout:** Icons positioned left-to-right, row-by-row, all using the same baseline
- **Column Layout:** Each column calculates its own width, and icons can be centered within their column

**When to use:**
- **Column Layout:** When you have icons of varying widths and want to maximize space efficiency
- **Standard Layout:** When you prefer a uniform grid appearance

**Code:** Determines which layout function to call in `layout_processor.c` lines 1328-1341:
```c
if (prefs->centerIconsInColumn) {
    CalculateLayoutPositionsWithColumnCentering(iconArray, prefs, finalColumns);
} else {
    CalculateLayoutPositions(iconArray, prefs, finalColumns);
}
```

---

### Beta Options Button

Opens the **Beta Options** window (see section below).

---

## Beta Options Window

Accessed via the "Beta Options..." button in Advanced Settings. These are experimental features that may change or be removed in future versions.

### Experimental Features

#### **Auto-open folders after processing**
When checked, iTidy automatically opens folder windows in Workbench after tidying their icons.

**What it does:**  
After iTidy finishes processing a folder (especially in recursive mode), it calls Workbench's `OpenWorkbenchObjectA()` to open that folder's window. This lets you see the results immediately.

**Use Case:** Helpful during recursive operations to see progress in real-time.

**Preference:** `beta_openFoldersAfterProcessing`

**Code:** Implemented in `layout_processor.c` lines 1361-1383:
```c
if (prefs->beta_openFoldersAfterProcessing) {
    OpenWorkbenchObjectA((STRPTR)path, NULL);
}
```

---

#### **Find and update open folder windows**
When checked, iTidy searches for already-open Workbench windows and updates their geometry.

**What it does:**  
iTidy scans the list of open Workbench windows, finds the window matching the folder being processed, and resizes/refreshes it to reflect the new icon layout.

**Technical Details:**  
- Uses `BuildFolderWindowList()` to enumerate open windows
- Calls `UpdateWorkbenchWindow()` to refresh window contents
- Only works if the folder window is already open

**Preference:** `beta_FindWindowOnWorkbenchAndUpdate`

**Status:** Beta feature - may have stability issues on some systems

---

### Logging Options

#### **Log Level**
Controls the verbosity of log output.

**Options:**
- **DEBUG (Verbose)** - All messages including detailed debug information
- **INFO (Normal)** - Informational messages and above
- **WARNING (Warnings only)** - Only warnings and errors
- **ERROR (Errors only)** - Only critical errors

**What it does:**  
Filters which messages are written to log files in `Bin/Amiga/logs/`. Lower verbosity creates smaller log files but provides less troubleshooting information.

**Preference:** `logLevel` (0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR)

**Log Files:**
- `general_YYYY-MM-DD_HH-MM-SS.log`
- `gui_YYYY-MM-DD_HH-MM-SS.log`
- `icons_YYYY-MM-DD_HH-MM-SS.log`
- `memory_YYYY-MM-DD_HH-MM-SS.log`
- `backup_YYYY-MM-DD_HH-MM-SS.log`
- `errors_YYYY-MM-DD_HH-MM-SS.log` (consolidated errors)

---

#### **Enable memory allocation logging**
When checked, logs all memory allocations and deallocations.

**What it does:**  
Tracks every `whd_malloc()` and `whd_free()` call with file/line information. Creates a detailed memory report showing:
- Total allocations/deallocations
- Memory leaks (allocations without matching frees)
- Allocation source locations

**Use Case:** Debugging memory leaks during development

**Preference:** `memoryLoggingEnabled`

**Output:** `Bin/Amiga/logs/memory_YYYY-MM-DD_HH-MM-SS.log`

**Performance Impact:** Moderate (each allocation writes to log file)

---

#### **Enable performance timing logs**
When checked, logs execution time for major operations.

**What it does:**  
Measures and logs how long different phases of icon processing take:
- Directory scanning
- Icon loading
- Sorting
- Layout calculation
- Icon saving
- Window resizing

**Use Case:** 
- Identifying performance bottlenecks
- Comparing different settings for speed
- Validating optimization improvements

**Preference:** `enable_performance_logging`

**Performance Impact:** Minimal (uses high-resolution timer)

---

## Presets and Settings Interaction

### How Presets Affect Advanced Options

When you select a **Preset** in the main window (Compact, Efficient, Relaxed, Gaming, Custom), iTidy automatically configures many Advanced Options. Here's what each preset sets:

#### **Compact Preset**
- Aspect Ratio: Classic (1.6)
- Overflow Mode: Vertical (expand rows)
- Center Icons in Column: Yes
- Optimize Column Widths: Yes

#### **Efficient Preset**
- Aspect Ratio: Compact (1.3)
- Overflow Mode: Vertical (maximize width)
- Center Icons in Column: Yes
- Optimize Column Widths: Yes

#### **Relaxed Preset**
- Aspect Ratio: Wide (2.0)
- Overflow Mode: Both (maintain proportions)
- Center Icons in Column: Yes
- Optimize Column Widths: Yes

#### **Gaming Preset**
- Aspect Ratio: Ultrawide (2.4)
- Overflow Mode: Horizontal (wide scrolling)
- Center Icons in Column: Yes
- Optimize Column Widths: Yes

#### **Custom Preset**
All Advanced Options remain at their manually configured values.

**Important:** Changing a preset will **overwrite** your current Advanced Options settings. If you've customized Advanced Options, select "Custom" preset to preserve your changes.

---

## Settings Persistence

All Advanced Options and Beta Options are saved to the global preferences structure and persist across iTidy sessions (when Save/Load Preset features are used).

**Storage Location:** Preferences are stored in the `LayoutPreferences` structure defined in `src/layout_preferences.h`.

**Preference Fields:**
- `aspectRatio` - Target aspect ratio (fixed-point, scaled by 1000)
- `useCustomAspectRatio` - Boolean flag for custom ratio
- `customAspectWidth` / `customAspectHeight` - Custom ratio values
- `overflowMode` - Window overflow strategy (0=Horiz, 1=Vert, 2=Both)
- `iconSpacingX` / `iconSpacingY` - Pixel spacing values
- `minIconsPerRow` / `maxIconsPerRow` - Column constraints
- `maxWindowWidthPct` - Maximum window width percentage
- `textAlignment` - Vertical alignment mode (0=Top, 1=Middle, 2=Bottom)
- `reverseSort` - Reverse sort flag
- `useColumnWidthOptimization` - Optimize column widths flag
- `skipHiddenFolders` - Skip hidden folders flag
- `centerIconsInColumn` - Column layout flag
- `beta_openFoldersAfterProcessing` - Auto-open folders flag
- `beta_FindWindowOnWorkbenchAndUpdate` - Update open windows flag
- `logLevel` - Logging verbosity level
- `memoryLoggingEnabled` - Memory logging flag
- `enable_performance_logging` - Performance logging flag

---

## Tips and Best Practices

### For Small Collections (< 20 icons)
- Use **Square (1.0)** or **Tall (0.75)** aspect ratio
- Enable **Column Layout**
- Enable **Optimize Column Widths**
- Set **Min Icons/Row** to 2-3 to avoid single-column layouts

### For Large Collections (> 100 icons)
- Use **Wide (2.0)** or **Ultrawide (2.4)** aspect ratio
- Set **Overflow Mode** to "Expand Horizontally"
- Use **Auto Max Icons/Row** with **70% Max Width**
- Consider **Skip Hidden Folders** for recursive operations

### For Mixed Icon Sizes
- Enable **Column Layout**
- Enable **Optimize Column Widths**
- Use **Middle** vertical alignment for visual balance

### For Uniform Grid Appearance
- Disable **Optimize Column Widths**
- Use **Top** vertical alignment
- Use standard row-based layout (disable **Column Layout**)

---

## Keyboard Shortcuts

All windows in iTidy support these standard AmigaOS shortcuts:
- **Escape** - Close window (same as Cancel)
- **Return** - Activate default button (OK)
- **Amiga+M** - Move window (drag with mouse)

---

## Troubleshooting

### Icons Still Overlap After Optimization
- Increase **Horizontal Spacing** and **Vertical Spacing**
- Disable **Optimize Column Widths** to use uniform column width
- Check that icon `.info` files have correct bounding boxes

### Window Too Wide for Screen
- Reduce **Max Window Width** percentage
- Decrease **Max Icons/Row** if using manual mode
- Change **Overflow Mode** to "Expand Vertically"

### Recursive Processing Too Slow
- Enable **Skip Hidden Folders** to avoid system directories
- Disable **Find and update open folder windows**
- Reduce **Log Level** to WARNING or ERROR

### Memory Warnings in Logs
- Disable **Enable memory allocation logging** when not debugging
- Check for leaked memory using the memory report
- Report issues with memory log attached

---

## Related Documentation

- `docs/ASPECT_RATIO_AND_OVERFLOW_FEATURE.md` - Technical details on aspect ratio algorithm
- `docs/COLUMN_CENTERING_FEATURE.md` - Column layout implementation details
- `docs/BETA_OPTIONS_WINDOW.md` - Beta features architecture
- `docs/ENHANCED_LOGGING_SYSTEM.md` - Logging system documentation
- `src/templates/AI_AGENT_GETTING_STARTED.md` - Window implementation guide

---

**Document Version:** 1.0  
**iTidy Version:** 2.0  
**Last Updated:** 2025-11-27
