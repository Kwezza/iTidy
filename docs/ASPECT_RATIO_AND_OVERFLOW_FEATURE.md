# Aspect Ratio and Window Overflow Feature Design

**Status:** 📋 Design Specification  
**Phase:** Pre-Implementation  
**Related Files:** `layout_preferences.h`, `layout_preferences.c`, `layout_processor.c`, `window_management.c`

---

## Overview

This document specifies the implementation of two related features for iTidy:

1. **Aspect Ratio Control** - Calculate optimal window dimensions based on a target width-to-height ratio
2. **Window Overflow Mode** - User-controlled behavior when folders contain more icons than fit on screen

These features work together to provide intelligent, user-controlled window sizing for folders of any size, from small directories to massive WHDLoad collections.

---

## Problem Statement

### Current Limitations

1. **No Aspect Ratio Awareness**
   - Current layout only considers width constraints (`maxWindowWidthPct`)
   - Results in ultra-wide single-row windows on RTG screens
   - No consideration of optimal window "shape"

2. **No Overflow Control**
   - Large folders overflow unpredictably
   - Users have no control over scrolling direction preference
   - WHDLoad folders with 500+ icons need explicit handling

3. **Scaffolded But Not Implemented**
   - `aspectRatio` field exists in `LayoutPreferences` but is never used
   - Default values set in presets but no algorithm uses them

### Goals

- ✅ Calculate optimal icon-per-row count based on target aspect ratio
- ✅ Handle folders with more icons than fit on screen gracefully
- ✅ Give users explicit control over overflow scrolling behavior
- ✅ Maintain compatibility with existing layout algorithms
- ✅ Work seamlessly with `centerIconsInColumn` feature
- ✅ Respect existing constraints (`maxIconsPerRow`, `maxWindowWidthPct`)

---

## Feature 3: Custom Aspect Ratio Input

### User-Facing Feature

Power users can enter custom aspect ratios not available in the preset list.

### GUI Design

**Aspect Ratio Cycle Gadget:**
```
Aspect Ratio: [Classic (1.6) ▼]
```

**Preset Options:**
- Square (1.0)
- Compact (1.3)
- Classic (1.6) ← Default
- Wide (2.0)
- Ultrawide (2.4)
- Custom

**Custom Ratio Input:**
When "Custom" is selected, two integer gadgets become enabled:
```
Aspect Ratio: [Custom ▼] [16]:[10]
                         ↑    ↑
                       Width Height
```

### Calculation

```c
/* User enters: 16:10 */
float customAspectRatio = (float)widthValue / (float)heightValue;
/* Result: 16/10 = 1.6 */
```

### Common Custom Ratios

```
16:10 = 1.60  (Classic 4:3-era monitors)
16:9  = 1.78  (Modern widescreen)
21:9  = 2.33  (Ultrawide monitors)
4:3   = 1.33  (Original Amiga)
3:2   = 1.50  (Photo-style)
1:1   = 1.00  (Perfect square)
```

### GUI Behavior

1. User selects preset → Custom gadgets **disabled**, preset value used
2. User selects "Custom" → Custom gadgets **enabled**
3. User enters values → Calculated ratio shown: `[16]:[10] = 1.60`
4. Invalid values (0, negative) → Show warning, use previous valid ratio
5. Extreme ratios (>5.0 or <0.2) → Show warning but allow

### Validation

```c
BOOL ValidateCustomAspectRatio(int width, int height, float *outRatio)
{
    if (width <= 0 || height <= 0)
    {
        /* Invalid - cannot be zero or negative */
        DisplayError("Aspect ratio values must be positive integers.");
        return FALSE;
    }
    
    *outRatio = (float)width / (float)height;
    
    if (*outRatio > 5.0f || *outRatio < 0.2f)
    {
        /* Extreme but not invalid - warn but allow */
        DisplayWarning("Very extreme aspect ratio (%.2f). Window may be unusual.", *outRatio);
    }
    
    return TRUE;
}
```

### Storage in Preferences

```c
typedef struct {
    /* ... existing fields ... */
    
    /* Aspect Ratio Settings */
    float aspectRatio;           /* Calculated or preset ratio */
    UWORD customAspectWidth;     /* Custom ratio numerator (e.g., 16) */
    UWORD customAspectHeight;    /* Custom ratio denominator (e.g., 10) */
    BOOL useCustomAspectRatio;   /* TRUE if "Custom" selected */
    
    /* ... rest of fields ... */
} LayoutPreferences;
```

When loading preferences:
```c
if (prefs->useCustomAspectRatio)
{
    prefs->aspectRatio = (float)prefs->customAspectWidth / 
                         (float)prefs->customAspectHeight;
}
```

---

## Feature 4: Icon Spacing Control

### User-Facing Feature

Users can fine-tune the spacing between icons and around window edges.

### GUI Design

**Horizontal Spacing Slider:**
```
Icon Spacing:
  Horizontal: [━━━━●━━━━] 8 px
```

**Vertical Spacing Slider:**
```
  Vertical:   [━━━━●━━━━] 8 px
```

**Range:** 4 to 20 pixels (practical limits for Amiga screens)

### Purpose

Icon spacing affects:
1. **Between Icons** - Gap between adjacent icons (horizontal and vertical)
2. **Window Margins** - Padding around all edges (uses same values)

```
┌─────────────────────────┐
│ ← iconSpacingX →        │  ↑
│    ┌────┐  ┌────┐       │  iconSpacingY
│    │Icon│  │Icon│       │  ↓
│    └────┘  └────┘       │
│      ↑                  │
│    iconSpacingX         │
│    ┌────┐  ┌────┐       │
│    │Icon│  │Icon│       │
│    └────┘  └────┘       │
│                         │
└─────────────────────────┘
```

### Integration with Layout

Update all layout functions to use `prefs->iconSpacingX` and `prefs->iconSpacingY` instead of hardcoded `8`:

**Current Code:**
```c
int iconSpacing = 8;  /* Hardcoded */
int startX = 8;
int startY = 4;
```

**Updated Code:**
```c
int iconSpacingX = prefs->iconSpacingX;  /* User-controlled */
int iconSpacingY = prefs->iconSpacingY;  /* User-controlled */
int startX = prefs->iconSpacingX;        /* Left margin = spacing */
int startY = prefs->iconSpacingY;        /* Top margin = spacing */
int rightMargin = prefs->iconSpacingX;   /* Right margin = spacing */
int bottomMargin = prefs->iconSpacingY;  /* Bottom margin = spacing */
```

### Effect on Calculations

**Horizontal Layout:**
```c
/* Calculate where next icon would start */
nextX = currentX + icon->icon_max_width + prefs->iconSpacingX;

/* Check wrap condition */
if (nextX > (effectiveMaxWidth - prefs->iconSpacingX))
{
    /* Wrap to next row */
    currentX = prefs->iconSpacingX;
    currentY += maxHeightInRow + prefs->iconSpacingY;
}
```

**Column-Centered Layout:**
```c
/* Account for spacing in column calculations */
estimatedColumns = (effectiveMaxWidth - prefs->iconSpacingX * 2) / 
                   (averageWidth + prefs->iconSpacingX);
```

**Window Sizing:**
```c
/* Add margins to content dimensions */
windowWidth = contentWidth + (prefs->iconSpacingX * 2) + chrome;
windowHeight = contentHeight + (prefs->iconSpacingY * 2) + chrome;
```

### Default Values

```c
#define DEFAULT_ICON_SPACING_X     8    /* 8px horizontal (current behavior) */
#define DEFAULT_ICON_SPACING_Y     8    /* 8px vertical (current behavior) */
#define MIN_ICON_SPACING           4    /* Minimum 4px (icons too close) */
#define MAX_ICON_SPACING          20    /* Maximum 20px (too much wasted space) */
```

### Preset Values

Different presets can have different spacing:

```c
/* Classic Preset - Standard Workbench spacing */
prefs->iconSpacingX = 8;
prefs->iconSpacingY = 8;

/* Compact Preset - Tight spacing for many icons */
prefs->iconSpacingX = 6;
prefs->iconSpacingY = 6;

/* Modern Preset - Generous spacing */
prefs->iconSpacingX = 12;
prefs->iconSpacingY = 10;

/* WHDLoad Preset - Tight horizontal, standard vertical */
prefs->iconSpacingX = 6;
prefs->iconSpacingY = 8;
```

### Visual Examples

**Tight Spacing (4px):**
```
More icons fit on screen
Icons feel crowded
Good for maximizing space
```

**Normal Spacing (8px):**
```
Balanced appearance
Current default behavior
Good general-purpose setting
```

**Generous Spacing (16px):**
```
Icons feel spacious
Easier to see individual items
Better for large icons
Uses more screen space
```

### User-Facing Documentation

**Tooltip Text:**
```
Icon Spacing:
Controls the gap between icons and window edges.

• Horizontal: Left/right gaps and spacing between columns
• Vertical: Top/bottom gaps and spacing between rows

Smaller values = more icons fit on screen
Larger values = more spacious, easier to read

Range: 4-20 pixels
Default: 8 pixels
```

---

## Interaction Between Features

### minIconsPerRow + Aspect Ratio
- Prevents ultra-tall 1×N layouts for small folders
- Ensures minimum width even with square aspect ratios
- Default: 2 (prevents single column)

**Example:** 3 icons, aspect ratio 1.0
- Without min: 1×3 layout (tall and narrow)
- With min=2: 2×2 layout (one empty slot, better proportions)

### Icon Spacing + Window Sizing
- Tighter spacing → More icons fit → Smaller windows
- Generous spacing → Fewer icons fit → Larger windows or more scrolling
- Affects overflow calculations (less usable space per icon)

**Example:** 100 icons on 640px width
- 4px spacing: ~10 icons per row → 10 rows
- 12px spacing: ~6 icons per row → 17 rows (taller window)

### Custom Aspect Ratio + Overflow Mode
- Custom ratios work identically to preset ratios
- Overflow mode still applies when layout exceeds screen
- Extreme custom ratios (e.g., 21:9) may trigger overflow more often

### Min/Max Icons Per Row + All Features
- **Absolute constraints** on all calculations
- minIconsPerRow prevents ultra-narrow windows
- maxIconsPerRow prevents ultra-wide windows
- Aspect ratio can only suggest within this range

---

## Feature 1: Aspect Ratio Control

### What is Aspect Ratio?

Aspect ratio is the relationship between window width and height:

```
aspectRatio = windowWidth / windowHeight

Examples:
- 1.6 = 640px wide × 400px tall (classic Workbench look)
- 1.3 = compact/squarer windows
- 1.0 = perfect square window
```

### Algorithm: Calculate Optimal Icons Per Row

Given a target aspect ratio, calculate how many icons per row creates a window closest to that ratio:

```c
/**
 * @brief Calculate optimal icons per row based on aspect ratio
 * 
 * Iterates through possible column counts and finds the one that
 * produces a window shape closest to the target aspect ratio.
 * 
 * @param totalIcons Total number of icons to arrange
 * @param averageIconWidth Average width of icons (including text)
 * @param averageIconHeight Average height of icons (including text)
 * @param targetAspectRatio Desired width/height ratio (e.g., 1.6)
 * @param minAllowedIconsPerRow Lower limit on columns (prevent 1×N layouts)
 * @param maxAllowedIconsPerRow Upper limit on columns (from maxIconsPerRow pref)
 * @return Optimal number of icons per row
 */
int CalculateOptimalIconsPerRow(int totalIcons, 
                                 int averageIconWidth,
                                 int averageIconHeight, 
                                 float targetAspectRatio,
                                 int minAllowedIconsPerRow,
                                 int maxAllowedIconsPerRow)
{
    int bestColumns = minAllowedIconsPerRow;  /* Start at minimum */
    float bestRatioDiff = 999.0f;
    
    /* Clamp search range */
    if (minAllowedIconsPerRow < 1)
        minAllowedIconsPerRow = 1;
    if (maxAllowedIconsPerRow < minAllowedIconsPerRow)
        maxAllowedIconsPerRow = minAllowedIconsPerRow;
    
    /* Try different column counts */
    for (int cols = minAllowedIconsPerRow; cols <= maxAllowedIconsPerRow; cols++)
    {
        int rows = (totalIcons + cols - 1) / cols;  /* Ceiling division */
        
        /* Skip if this creates only one row (unless forced to) */
        if (rows == 1 && cols > totalIcons)
            continue;
        
        float estimatedWidth = cols * averageIconWidth;
        float estimatedHeight = rows * averageIconHeight;
        float actualRatio = estimatedWidth / estimatedHeight;
        
        float ratioDiff = fabs(actualRatio - targetAspectRatio);
        
        if (ratioDiff < bestRatioDiff)
        {
            bestRatioDiff = ratioDiff;
            bestColumns = cols;
        }
    }
    
    return bestColumns;
}
```

### Example Calculations

**20 icons, aspect ratio 1.6, average icon size 80×80px:**

```
Try 1 column:  80 × 1600 = 0.05 ratio (diff: 1.55) ← Rejected if minIconsPerRow = 2
Try 2 columns: 160 × 800 = 0.20 ratio (diff: 1.40)
Try 3 columns: 240 × 560 = 0.43 ratio (diff: 1.17)
Try 4 columns: 320 × 400 = 0.80 ratio (diff: 0.80)
Try 5 columns: 400 × 320 = 1.25 ratio (diff: 0.35) ← Best!
Try 6 columns: 480 × 320 = 1.50 ratio (diff: 0.10) ← Even better!
Try 7 columns: 560 × 240 = 2.33 ratio (diff: 0.73)

Result: 6 columns × 4 rows = aspect ratio ~1.5 (close to 1.6)
```

**5 icons, aspect ratio 1.0 (square), no minimum constraint:**
```
Try 1 column:  80 × 400 = 0.20 ratio (diff: 0.80) ← Best without minimum
Result: 1 column × 5 rows (ultra-tall!)
```

**5 icons, aspect ratio 1.0 (square), minIconsPerRow = 2:**
```
Try 1 column:  SKIPPED (below minimum)
Try 2 columns: 160 × 240 = 0.67 ratio (diff: 0.33) ← Best
Try 3 columns: 240 × 160 = 1.50 ratio (diff: 0.50)
Result: 2 columns × 3 rows (better proportions)
```

---

## Feature 2: Window Overflow Mode

### User-Facing Feature

Add a new cycle gadget to the **Advanced Settings** window that controls what happens when a folder has more icons than fit on screen.

### GUI Addition

```
╔═══════════════════════════════════════════════════╗
║                 Advanced Settings                 ║
╠═══════════════════════════════════════════════════╣
║ Aspect Ratio: [Classic (1.6) ▼] [  ]:[  ]        ║
║   • Square (1.0)                                  ║
║   • Compact (1.3)                                 ║
║   • Classic (1.6)            ← Custom ratio only  ║
║   • Wide (2.0)                  enabled when      ║
║   • Ultrawide (2.4)             "Custom" selected ║
║   • Custom                                        ║
║                                                   ║
║ Window Overflow: [Expand Horizontally ▼]          ║
║   • Expand Horizontally (scroll left/right)       ║
║   • Expand Vertically (scroll up/down)            ║
║   • Expand Both (scroll both directions)          ║
║                                                   ║
║ Icon Spacing:                                     ║
║   Horizontal: [━━━━●━━━━] 8 px                    ║
║   Vertical:   [━━━━●━━━━] 8 px                    ║
║   (Range: 4-20 pixels)                            ║
║                                                   ║
║ Column Limits:                                    ║
║   Min Icons/Row: [2 ]  Max Icons/Row: [10 ]      ║
║                                                   ║
║ Max Width: [Auto ▼] (40–80%)                      ║
║ Resize Windows: [✓]                               ║
║                                                   ║
║  [OK]  [Cancel]                                   ║
╚═══════════════════════════════════════════════════╝
```

**Notes:**
- Aspect Ratio cycle gadget has predefined options plus "Custom"
- Two integer gadgets (width:height) appear when "Custom" is selected
- Icon spacing controlled by horizontal sliders (4-20 pixel range)
- Min/Max Icons per Row use integer gadgets
- Live preview values shown next to sliders

### Enumeration Definition

```c
/**
 * @brief Window overflow behavior when icons exceed screen size
 */
typedef enum {
    OVERFLOW_HORIZONTAL = 0,  /* Expand width, add horizontal scrollbar */
    OVERFLOW_VERTICAL = 1,    /* Expand height, add vertical scrollbar */
    OVERFLOW_BOTH = 2         /* Expand both, add both scrollbars */
} WindowOverflowMode;
```

### Add to LayoutPreferences Structure

```c
typedef struct {
    /* Layout Settings */
    LayoutMode layoutMode;
    SortOrder sortOrder;
    SortPriority sortPriority;
    SortBy sortBy;
    BOOL reverseSort;
    
    /* Visual Settings */
    BOOL centerIconsInColumn;
    BOOL useColumnWidthOptimization;
    TextAlignment textAlignment;
    
    /* Window Management */
    BOOL resizeWindows;
    UWORD minIconsPerRow;            /* ← NEW: Minimum columns (prevent 1×N layouts) */
    UWORD maxIconsPerRow;            /* Maximum columns (0 = no limit) */
    UWORD maxWindowWidthPct;         /* Max window width as % of screen */
    float aspectRatio;               /* Target window aspect ratio */
    WindowOverflowMode overflowMode; /* ← NEW: Overflow behavior */
    
    /* Spacing Settings */
    UWORD iconSpacingX;              /* ← NEW: Horizontal spacing between icons (pixels) */
    UWORD iconSpacingY;              /* ← NEW: Vertical spacing between icons (pixels) */
    
    /* Advanced Settings */
    BOOL skipHiddenFolders;          /* Skip folders without .info files (hidden) */
    
    /* Backup Settings */
    BackupPreferences backupPrefs;   /* Embedded backup configuration */
} LayoutPreferences;
```

### Default Values

```c
#define DEFAULT_OVERFLOW_MODE      OVERFLOW_HORIZONTAL  /* Classic behavior */
#define DEFAULT_MIN_ICONS_PER_ROW  2                    /* Prevent 1×N columns */
#define DEFAULT_ICON_SPACING_X     8                    /* 8px horizontal spacing */
#define DEFAULT_ICON_SPACING_Y     8                    /* 8px vertical spacing */
```

---

## Overflow Mode Behaviors

### Mode 1: OVERFLOW_HORIZONTAL (Default)

**Description:** "When screen height is full, expand width"

**Priority:** Height first, then width

**Algorithm:**
```
1. Calculate columns for target aspect ratio
2. If estimated height > screen height:
   - Use maximum rows that fit screen height
   - Calculate required columns (may exceed screen width)
   - Window gets horizontal scrollbar if needed
3. Result: Tall folders become wide with horizontal scrolling
```

**Example:** 500 icons on 640×512 screen
- Max rows: ~6 rows (fit in 512px height)
- Required columns: 500 / 6 = 84 columns
- Window: 640px tall × ~6720px wide (horizontal scrollbar)

**Best for:**
- WHDLoad folders
- Game collections
- File browsers where vertical scanning is natural

---

### Mode 2: OVERFLOW_VERTICAL

**Description:** "When screen width is full, expand height"

**Priority:** Width first, then height

**Algorithm:**
```
1. Calculate columns for target aspect ratio
2. If estimated width > screen width:
   - Use maximum columns that fit screen width
   - Calculate required rows (may exceed screen height)
   - Window gets vertical scrollbar if needed
3. Result: Wide folders become tall with vertical scrolling
```

**Example:** 500 icons on 640×512 screen
- Max columns: ~8 columns (fit in 640px width)
- Required rows: 500 / 8 = 63 rows
- Window: 640px wide × ~5040px tall (vertical scrollbar)

**Best for:**
- Document folders
- Project folders
- Users who want to see more columns at once
- Narrow screens where horizontal space is limited

---

### Mode 3: OVERFLOW_BOTH

**Description:** "Balance width and height, allow scrolling in both directions if needed"

**Priority:** Maintain aspect ratio, expand both dimensions

**Algorithm:**
```
1. Calculate columns for target aspect ratio
2. Allow both width AND height to exceed screen
3. Window may get both scrollbars
4. Result: Maintains intended window shape even when huge
```

**Example:** 500 icons on 640×512 screen, aspect ratio 1.6
- Optimal layout: ~28 columns × ~18 rows
- Window: ~2240px wide × ~1440px tall (both scrollbars)

**Best for:**
- Users who want consistent window proportions
- Users who don't mind scrolling in both directions
- Maintaining visual consistency across all folder sizes

---

## Combined Algorithm Implementation

### High-Level Flow

```c
void CalculateLayoutWithAspectRatio(IconArray *iconArray, 
                                   const LayoutPreferences *prefs)
{
    int finalColumns, finalRows;
    int avgWidth = CalculateAverageWidth(iconArray);
    int avgHeight = CalculateAverageHeight(iconArray);
    int workbenchTitleHeight = prefsIControl.currentTitleBarHeight;
    int maxUsableHeight = screenHeight - workbenchTitleHeight;
    int maxUsableWidth = screenWidth;
    
    /* STAGE 1: Calculate ideal layout based on aspect ratio */
    int idealColumns = CalculateOptimalIconsPerRow(
        iconArray->size, avgWidth, avgHeight,
        prefs->aspectRatio, 
        prefs->maxIconsPerRow > 0 ? prefs->maxIconsPerRow : 999);
    
    int idealRows = (iconArray->size + idealColumns - 1) / idealColumns;
    int idealWidth = idealColumns * avgWidth + chrome;
    int idealHeight = idealRows * avgHeight + chrome;
    
    /* STAGE 2: Check if ideal layout fits on screen */
    BOOL fitsWidth = (idealWidth <= maxUsableWidth);
    BOOL fitsHeight = (idealHeight <= maxUsableHeight);
    
    if (fitsWidth && fitsHeight)
    {
        /* ✅ Perfect! Use ideal aspect ratio layout */
        finalColumns = idealColumns;
        finalRows = idealRows;
    }
    else
    {
        /* ❌ Doesn't fit - apply overflow strategy */
        switch (prefs->overflowMode)
        {
            case OVERFLOW_HORIZONTAL:
                /* Height first: maximize rows, expand columns as needed */
                finalRows = maxUsableHeight / (avgHeight + spacing);
                finalColumns = (iconArray->size + finalRows - 1) / finalRows;
                
                /* Clamp to maxIconsPerRow if set */
                if (prefs->maxIconsPerRow > 0 && finalColumns > prefs->maxIconsPerRow)
                {
                    finalColumns = prefs->maxIconsPerRow;
                    /* Recalculate rows */
                    finalRows = (iconArray->size + finalColumns - 1) / finalColumns;
                }
                break;
                
            case OVERFLOW_VERTICAL:
                /* Width first: maximize columns, expand rows as needed */
                finalColumns = maxUsableWidth / (avgWidth + spacing);
                
                /* Clamp to maxIconsPerRow if set */
                if (prefs->maxIconsPerRow > 0 && finalColumns > prefs->maxIconsPerRow)
                    finalColumns = prefs->maxIconsPerRow;
                
                finalRows = (iconArray->size + finalColumns - 1) / finalColumns;
                break;
                
            case OVERFLOW_BOTH:
                /* Use ideal aspect ratio even if it means scrolling both ways */
                finalColumns = idealColumns;
                finalRows = idealRows;
                
                /* Still respect maxIconsPerRow */
                if (prefs->maxIconsPerRow > 0 && finalColumns > prefs->maxIconsPerRow)
                {
                    finalColumns = prefs->maxIconsPerRow;
                    finalRows = (iconArray->size + finalColumns - 1) / finalColumns;
                }
                break;
        }
    }
    
    /* STAGE 3: Use finalColumns/finalRows in layout algorithm */
    if (prefs->centerIconsInColumn)
    {
        CalculateLayoutWithColumnCentering(iconArray, prefs, finalColumns);
    }
    else
    {
        CalculateLayoutStandard(iconArray, prefs, finalColumns);
    }
}
```

### Window Positioning for Overflow Windows

```c
void CalculateWindowBounds(int contentWidth, int contentHeight,
                          int *outLeft, int *outTop, 
                          int *outWidth, int *outHeight)
{
    int workbenchTitleHeight = prefsIControl.currentTitleBarHeight;
    int maxUsableHeight = screenHeight - workbenchTitleHeight;
    
    /* Add window chrome */
    int winWidth = contentWidth + chromeWidth;
    int winHeight = contentHeight + chromeHeight;
    
    /* Clamp to screen size (will create scrollbars) */
    if (winHeight > maxUsableHeight)
    {
        winHeight = maxUsableHeight;
        /* Window will have vertical scrollbar */
    }
    
    if (winWidth > screenWidth)
    {
        winWidth = screenWidth;
        /* Window will have horizontal scrollbar */
    }
    
    /* Position vertically */
    if (winHeight >= maxUsableHeight)
    {
        /* Tall window - position just below Workbench title bar */
        *outTop = workbenchTitleHeight;
        *outHeight = winHeight;
    }
    else
    {
        /* Normal window - center vertically */
        *outTop = (screenHeight - winHeight) / 2;
        *outHeight = winHeight;
    }
    
    /* Center horizontally */
    *outLeft = (screenWidth - winWidth) / 2;
    *outWidth = winWidth;
}
```

---

## Interaction with Existing Features

### With `maxIconsPerRow`
- Acts as **hard upper limit** on columns in ALL modes
- If overflow mode suggests 84 columns but `maxIconsPerRow = 10`, use 10
- Takes absolute priority over aspect ratio suggestions

### With `minIconsPerRow` (NEW)
- Acts as **hard lower limit** on columns in ALL modes
- Prevents ultra-tall single-column layouts
- Default: 2 (minimum 2 columns unless folder has <2 icons)
- User can set to 1 to allow single-column layouts

### With `maxWindowWidthPct`
- **Enforced when window FITS on screen**
- **Ignored when using OVERFLOW modes** (overflow is explicit user choice)
- Rationale: User explicitly chose overflow behavior to handle large folders

### With `aspectRatio`
- **OVERFLOW_HORIZONTAL**: Aspect ratio is initial suggestion, then height-prioritized
- **OVERFLOW_VERTICAL**: Aspect ratio is initial suggestion, then width-prioritized  
- **OVERFLOW_BOTH**: Aspect ratio is maintained even when huge

### With `centerIconsInColumn`
- Completely orthogonal - affects HOW icons are positioned within columns
- Does not affect HOW MANY columns are calculated
- Works identically with all overflow modes
- Column centering happens AFTER column count is determined

### With `layoutMode` (Row/Column)
- Currently not fully implemented
- Should affect the order icons are assigned to positions
- Does not affect overflow behavior or aspect ratio calculations

### With Icon Spacing (NEW)
- Spacing affects usable space calculations
- Tighter spacing → More icons fit → May avoid overflow
- Generous spacing → Fewer icons fit → May trigger overflow sooner
- All margin and positioning calculations use spacing values

---

## Preset Configurations

Update the four presets to include sensible overflow defaults:

```c
/* Classic Preset */
prefs->aspectRatio = 1.6f;
prefs->overflowMode = OVERFLOW_HORIZONTAL;  /* Wide scrolling */
prefs->minIconsPerRow = 2;                  /* Prevent single column */
prefs->maxIconsPerRow = 5;
prefs->maxWindowWidthPct = 55;
prefs->iconSpacingX = 8;                    /* Standard spacing */
prefs->iconSpacingY = 8;

/* Compact Preset */
prefs->aspectRatio = 1.3f;
prefs->overflowMode = OVERFLOW_VERTICAL;  /* Maximize width usage */
prefs->minIconsPerRow = 2;
prefs->maxIconsPerRow = 6;
prefs->maxWindowWidthPct = 45;
prefs->iconSpacingX = 6;                  /* Tight spacing */
prefs->iconSpacingY = 6;

/* Modern Preset */
prefs->aspectRatio = 1.5f;
prefs->overflowMode = OVERFLOW_BOTH;  /* Maintain proportions */
prefs->minIconsPerRow = 3;            /* Wider minimum */
prefs->maxIconsPerRow = 8;
prefs->maxWindowWidthPct = 60;
prefs->iconSpacingX = 12;             /* Generous spacing */
prefs->iconSpacingY = 10;

/* WHDLoad Preset */
prefs->aspectRatio = 1.4f;
prefs->overflowMode = OVERFLOW_HORIZONTAL;  /* Wide game lists */
prefs->minIconsPerRow = 2;
prefs->maxIconsPerRow = 4;
prefs->maxWindowWidthPct = 40;
prefs->iconSpacingX = 6;                    /* Tight horizontal */
prefs->iconSpacingY = 8;                    /* Standard vertical */
```

---

## User-Facing Documentation

### Tooltip/Help Text for Controls

**Aspect Ratio Cycle Gadget:**
```
Aspect Ratio:
Controls the target shape of drawer windows.

Presets:
• Square (1.0) - Equal width and height
• Compact (1.3) - Slightly wider than tall
• Classic (1.6) - Traditional Workbench (default)
• Wide (2.0) - Wide letterbox style
• Ultrawide (2.4) - Very wide, good for RTG screens
• Custom - Enter your own ratio (e.g., 16:9)

Select "Custom" to use the width:height input boxes.
```

**Custom Aspect Ratio Inputs:**
```
Custom Aspect Ratio:
Enter width and height values to create a custom ratio.

Examples:
  16:10 = 1.60 (Classic 4:3 monitors)
  16:9  = 1.78 (Modern widescreen)
  21:9  = 2.33 (Ultrawide)
  4:3   = 1.33 (Original Amiga)

The ratio is calculated as width ÷ height.
```

**Window Overflow Cycle Gadget:**
```
Window Overflow:
Controls how windows handle folders with many icons:

• Expand Horizontally - Fill screen height, scroll left/right
  (Best for game collections, file browsers)

• Expand Vertically - Fill screen width, scroll up/down
  (Best for document folders, narrow screens)

• Expand Both - Maintain window proportions, scroll both ways
  (Best for consistent window shapes)
```

**Icon Spacing Sliders:**
```
Icon Spacing:
Controls gaps between icons and window edges.

Horizontal: Space between columns and left/right margins
Vertical: Space between rows and top/bottom margins

Smaller values (4px):
  + More icons fit on screen
  - Icons may feel crowded

Larger values (16px):
  + More spacious, easier to read
  - Fewer icons fit, may need scrolling

Default: 8 pixels (balanced)
```

**Min/Max Icons Per Row:**
```
Column Limits:

Min Icons/Row: Minimum columns (prevents ultra-narrow windows)
  Default: 2
  Set to 1 to allow single-column layouts

Max Icons/Row: Maximum columns (prevents ultra-wide windows)
  Default: 10
  Set to 0 for no limit

These are absolute constraints - aspect ratio works within these limits.
```

### Manual Entry

```
Aspect Ratio
-----------
Controls the target shape of drawer windows. A ratio of 1.6 creates
windows that are 1.6 times wider than they are tall (e.g., 640×400).

Higher values create wider windows, lower values create squarer windows.

Common values:
  1.3 - Compact, squarer windows
  1.6 - Classic Workbench appearance (default)
  2.0 - Wide, letterbox-style windows

Window Overflow
--------------
When a folder has more icons than fit on screen, iTidy can handle
the overflow in three ways:

  Expand Horizontally (default)
    Fill the screen height completely, then expand width as needed.
    Creates wide windows with horizontal scrollbars.
    Best for WHDLoad folders and game collections.

  Expand Vertically
    Fill the screen width completely, then expand height as needed.
    Creates tall windows with vertical scrollbars.
    Best for document folders and narrow screens.

  Expand Both
    Maintain the target aspect ratio even when it creates huge windows.
    May require scrolling in both directions.
    Best for users who want consistent window proportions.

Note: The "Max Icons/Row" setting always takes priority as an upper limit.
```

---

## Implementation Checklist

### Phase 1: Data Structures
- [ ] Add `WindowOverflowMode` enum to `layout_preferences.h`
- [ ] Add `overflowMode` field to `LayoutPreferences` structure
- [ ] Add `minIconsPerRow` field to `LayoutPreferences` structure
- [ ] Add `iconSpacingX` field to `LayoutPreferences` structure
- [ ] Add `iconSpacingY` field to `LayoutPreferences` structure
- [ ] Add `customAspectWidth` field to `LayoutPreferences` structure
- [ ] Add `customAspectHeight` field to `LayoutPreferences` structure
- [ ] Add `useCustomAspectRatio` field to `LayoutPreferences` structure
- [ ] Add all default constants (`DEFAULT_OVERFLOW_MODE`, etc.)
- [ ] Update `InitLayoutPreferences()` to set all new defaults
- [ ] Update `ApplyPreset()` to set new fields for each preset
- [ ] Update `MapGuiToPreferences()` to include new parameters

### Phase 2: Algorithm Functions
- [ ] Implement `CalculateOptimalIconsPerRow()` with min/max constraints
- [ ] Implement `CalculateAverageWidth()` helper function
- [ ] Implement `CalculateAverageHeight()` helper function
- [ ] Implement `ValidateCustomAspectRatio()` validation function
- [ ] Implement `CalculateLayoutWithAspectRatio()` main function
- [ ] Update `CalculateLayoutPositions()` to accept column count + spacing
- [ ] Update `CalculateLayoutPositionsWithColumnCentering()` to accept column count + spacing
- [ ] Replace all hardcoded spacing values with `prefs->iconSpacingX/Y`

### Phase 3: Window Management
- [ ] Update `resizeFolderToContents()` to handle overflow windows
- [ ] Update `repoistionWindow()` to position overflow windows correctly
- [ ] Implement `CalculateWindowBounds()` or integrate into existing function
- [ ] Update margin calculations to use spacing preferences
- [ ] Test window positioning with Workbench title bar offset

### Phase 4: GUI Integration - Main Window
- [ ] No changes needed (aspect ratio in Advanced window only)

### Phase 5: GUI Integration - Advanced Window
- [ ] Create Advanced Settings window layout (if doesn't exist)
- [ ] Add "Aspect Ratio" cycle gadget with presets
- [ ] Add two integer gadgets for custom ratio (width:height)
- [ ] Add logic to enable/disable custom gadgets based on cycle selection
- [ ] Add "Window Overflow" cycle gadget with three modes
- [ ] Add "Horizontal Spacing" slider gadget (range 4-20)
- [ ] Add "Vertical Spacing" slider gadget (range 4-20)
- [ ] Add "Min Icons/Row" integer gadget
- [ ] Add "Max Icons/Row" integer gadget
- [ ] Wire all gadgets to preferences structure
- [ ] Add validation for custom aspect ratio inputs
- [ ] Update GUI logging to display all new settings
- [ ] Add tooltips/help text for all new controls

### Phase 6: Testing
- [ ] Test with small folders (10-20 icons) - aspect ratio functionality
- [ ] Test with medium folders (50-100 icons) - may trigger overflow
- [ ] Test with large folders (500+ icons) - definitely overflow
- [ ] Test all three overflow modes with same large folder
- [ ] Test `minIconsPerRow` with very small folders (2-5 icons)
- [ ] Test interaction with `maxIconsPerRow` constraint
- [ ] Test with `centerIconsInColumn` enabled
- [ ] Test custom aspect ratios (16:9, 21:9, 1:1, etc.)
- [ ] Test extreme custom ratios (0.2, 5.0) - should warn but work
- [ ] Test invalid custom ratios (0:10, negative) - should reject
- [ ] Test tight icon spacing (4px) vs generous (20px)
- [ ] Test on different screen resolutions (320×256, 640×512, 1920×1080)
- [ ] Test with varying icon sizes (small text icons vs large image icons)
- [ ] Test preset configurations (Classic, Compact, Modern, WHDLoad)

### Phase 7: Documentation
- [ ] Update `LAYOUT_PREFERENCES_MODULE.md` with new fields
- [ ] Update `GUI_CONTROLS_REFERENCE.md` with new gadgets
- [ ] Add example configurations showing different spacing effects
- [ ] Add debug logging for aspect ratio calculations
- [ ] Add debug logging for overflow mode decisions
- [ ] Add debug logging for custom ratio validation
- [ ] Document spacing effects in layout algorithm notes

---

## Example Scenarios

### Scenario 1: Small Folder - Aspect Ratio Perfect Fit

```
Folder: 20 icons
Screen: 640×512
Aspect ratio: 1.6
Average icon: 80×80px

Calculation:
- Ideal: 5 columns × 4 rows
- Dimensions: 400×320 = ratio 1.25 (close to 1.6)
- Fits screen: ✅ Yes (400×320 < 640×512)
- Result: Use ideal layout, no scrollbars

Final: 5 columns × 4 rows, centered on screen
```

### Scenario 2: Large Folder - Horizontal Overflow

```
Folder: 500 icons
Screen: 640×512
Aspect ratio: 1.6
Average icon: 80×80px
Overflow mode: OVERFLOW_HORIZONTAL

Calculation:
- Ideal: 28 columns × 18 rows
- Dimensions: 2240×1440 = ratio 1.56
- Fits screen: ❌ No (1440 > 512)
- Apply horizontal overflow:
  - Max rows: 512 / 80 = 6 rows
  - Required columns: 500 / 6 = 84 columns
  - Window: 640×512 with horizontal scrollbar

Final: 84 columns × 6 rows, horizontal scrolling
```

### Scenario 3: Large Folder - Vertical Overflow

```
Folder: 500 icons
Screen: 640×512
Aspect ratio: 1.6
Average icon: 80×80px
Overflow mode: OVERFLOW_VERTICAL

Calculation:
- Ideal: 28 columns × 18 rows
- Dimensions: 2240×1440 = ratio 1.56
- Fits screen: ❌ No (2240 > 640)
- Apply vertical overflow:
  - Max columns: 640 / 80 = 8 columns
  - Required rows: 500 / 8 = 63 rows
  - Window: 640×512 with vertical scrollbar

Final: 8 columns × 63 rows, vertical scrolling
```

### Scenario 4: Large Folder - Both Overflow

```
Folder: 500 icons
Screen: 640×512
Aspect ratio: 1.6
Average icon: 80×80px
Overflow mode: OVERFLOW_BOTH

Calculation:
- Ideal: 28 columns × 18 rows
- Dimensions: 2240×1440 = ratio 1.56
- Fits screen: ❌ No
- Apply both overflow:
  - Use ideal layout anyway
  - Window: 640×512 with both scrollbars

Final: 28 columns × 18 rows, both scrollbars
Virtual window: 2240×1440 (maintains aspect ratio)
```

### Scenario 5: Tiny Folder - minIconsPerRow Protection

```
Folder: 3 icons
Screen: 640×512
Aspect ratio: 1.0 (square)
Average icon: 80×80px
minIconsPerRow: 2

Calculation:
- Ideal for 1.0 ratio: 2 columns × 2 rows (3 icons, 1 empty)
- Without minIconsPerRow: Could suggest 1 column × 3 rows
- Dimensions: 160×160 = ratio 1.0
- Fits screen: ✅ Yes

Final: 2 columns × 2 rows (prevents ultra-tall layout)
```

### Scenario 6: Custom Aspect Ratio - Ultrawide

```
Folder: 100 icons
Screen: 1920×1080 (RTG)
Custom aspect ratio: 21:9 = 2.33
Average icon: 80×80px
Overflow mode: OVERFLOW_HORIZONTAL

Calculation:
- Ideal for 2.33 ratio: 15 columns × 7 rows
- Dimensions: 1200×560 = ratio 2.14 (close to 2.33)
- Fits screen: ✅ Yes (1200×560 < 1920×1080)

Final: 15 columns × 7 rows, ultrawide layout
Perfect for widescreen monitors!
```

### Scenario 7: Tight Spacing - Fit More Icons

```
Folder: 50 icons
Screen: 640×512
Aspect ratio: 1.6
Average icon: 80×80px
Icon spacing: 4px (tight)
minIconsPerRow: 2
maxIconsPerRow: 10

Calculation with 4px spacing:
- More usable width: 640 - (4 × 2 margins) = 632px
- Icons per row: 632 / (80 + 4) = 7 columns
- Suggested: 9 columns × 6 rows (close to 1.6 ratio)
- Clamped to max: 7 columns (respects maxIconsPerRow=10)
- Dimensions: 588×504 fits screen

Comparison with 8px spacing:
- Would fit only 6 columns
- Would need 9 rows
- Taller window

Result: Tight spacing allows more icons per row!
```

---

## Future Enhancements

### Smart Aspect Ratio by Folder Type
```c
/* Auto-detect folder type and suggest aspect ratio */
if (IsWHDLoadFolder(path))
    suggestedRatio = 1.2f;  /* Compact for games */
else if (IsDocumentFolder(path))
    suggestedRatio = 1.8f;  /* Wide for documents */
```

### Remember Per-Folder Overflow Preferences
```c
/* Store overflow preference in .info file for this folder */
SaveFolderOverflowMode(path, OVERFLOW_HORIZONTAL);
```

### Auto-Adjust Spacing Based on Icon Density
```c
/* Automatically reduce spacing when many icons present */
if (iconCount > 200)
    suggestedSpacing = 4;  /* Tight for large collections */
else if (iconCount < 20)
    suggestedSpacing = 12; /* Generous for small folders */
```

### Aspect Ratio Preview
```c
/* Show live preview of window shape in Advanced settings */
DrawAspectRatioPreview(aspectRatio, "This is how your windows will look");
```

---

## Notes for Implementation

1. **Floating Point Math**: The Amiga compiler supports `float` for aspect ratio calculations. The calculations are simple enough for 68000.

2. **Performance**: The `CalculateOptimalIconsPerRow()` function loops through possible column counts. With `maxIconsPerRow` typically ≤ 20, this is negligible overhead.

3. **Compatibility**: This feature is **additive** - existing code paths remain unchanged if aspect ratio is not used.

4. **Testing Priority**: Focus testing on OVERFLOW_HORIZONTAL mode first, as it's the default and most common use case.

5. **GUI Space**: The Advanced Settings window will need to be expanded to accommodate the new overflow cycle gadget.

---

## Conclusion

This design provides:
- ✅ Intelligent aspect-ratio-based window sizing
- ✅ User control over overflow behavior
- ✅ Graceful handling of massive folders
- ✅ Compatibility with all existing layout features
- ✅ Sensible defaults that "just work"
- ✅ Advanced control for power users

The implementation scaffolds are already in place (aspect ratio field exists), making this a natural evolution of the existing window management system.
