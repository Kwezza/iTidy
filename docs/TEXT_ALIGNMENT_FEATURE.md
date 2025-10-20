# Text Alignment Feature

## Overview
This document describes the new text alignment feature added to iTidy's layout system. This feature allows users to choose how icon text labels are vertically aligned within each row.

## Feature Description

### Problem Solved
Previously, when icons in a row had different heights (e.g., one icon is 80 pixels tall while another is 40 pixels tall), their text labels would appear at different vertical positions. This created a visually inconsistent appearance where text labels were scattered at varying heights across the row.

### Solution
A new text alignment preference allows users to choose between two alignment modes:

1. **Top-Aligned (TEXT_ALIGN_TOP = 0)**: 
   - Each icon is positioned at the top of the row
   - Text labels appear at their natural position below each icon
   - This is the classic behavior where taller icons push their text lower

2. **Bottom-Aligned (TEXT_ALIGN_BOTTOM = 1)** *(Default)*:
   - All text labels in a row are aligned to the same vertical position
   - Based on the tallest icon in that row
   - Shorter icons are shifted down so their text aligns with the tallest icon's text
   - Creates a clean, consistent baseline for text across each row

## Implementation Details

### Data Structures

#### New Enumeration (`layout_preferences.h`)
```c
typedef enum {
    TEXT_ALIGN_TOP = 0,     /* Text at natural position (top-aligned icons) */
    TEXT_ALIGN_BOTTOM = 1   /* Text aligned to tallest icon in row (bottom-aligned) */
} TextAlignment;
```

#### Updated Structure
Added `textAlignment` field to `LayoutPreferences` structure:
```c
typedef struct {
    /* ... existing fields ... */
    
    /* Visual Settings */
    BOOL centerIconsInColumn;
    BOOL useColumnWidthOptimization;
    TextAlignment textAlignment;     /* NEW: Vertical alignment of text labels */
    
    /* ... other fields ... */
} LayoutPreferences;
```

### Algorithm

The layout calculation now uses a two-pass approach:

#### First Pass: Initial Positioning
1. Position each icon in the row from left to right
2. Track the tallest icon height (`maxHeightInRow`) in the current row
3. Track the starting index of the current row (`rowStartIndex`)
4. When wrapping to a new row, apply text alignment adjustment to the previous row

#### Second Pass: Final Row Adjustment
- After all icons are positioned, adjust the final row (which didn't trigger the wrap condition)
- Only performs adjustment if `TEXT_ALIGN_BOTTOM` is selected

#### Adjustment Calculation
For each icon in a completed row:
```c
adjustmentOffset = maxHeightInRow - icon->icon_max_height;
if (adjustmentOffset > 0) {
    icon->icon_y += adjustmentOffset;
}
```

This moves shorter icons downward so their text aligns with the tallest icon's text.

### Code Location

**File**: `src/layout_processor.c`  
**Function**: `CalculateLayoutPositions()`

Key code sections:
- **Lines ~70-75**: Track row boundaries (`rowStartIndex`, `rowStartY`, `maxHeightInRow`)
- **Lines ~85-100**: Adjustment logic when wrapping to new row
- **Lines ~135-155**: Final row adjustment after main loop

### Default Values

```c
#define DEFAULT_TEXT_ALIGNMENT  TEXT_ALIGN_BOTTOM  /* Default: align text to bottom of row */
```

All presets (Classic, Compact, Modern, WHDLoad) default to `TEXT_ALIGN_BOTTOM` for consistent appearance.

## Usage Example

```c
LayoutPreferences prefs;
InitLayoutPreferences(&prefs);

/* Default is TEXT_ALIGN_BOTTOM */
/* To use top alignment: */
prefs.textAlignment = TEXT_ALIGN_TOP;

ProcessDirectoryWithPreferences("Work:MyFolder", FALSE, &prefs);
```

## Debug Output

When `DEBUG` is defined, the layout processor logs:
- Current text alignment mode ("BOTTOM" or "TOP")
- Adjustment calculations for each icon
- Before/after positions for verification

Example debug output:
```
CalculateLayoutPositions: Positioning 5 icons with dynamic width wrapping
  screenWidth=640, maxWindowWidthPct=55, effectiveMaxWidth=352
  textAlignment=BOTTOM

Adjusting final row (indices 0 to 4) for bottom text alignment
  maxHeightInRow = 80
  Adjusted icon 0 ('Document.info') down by 0 pixels for text alignment
  Adjusted icon 1 ('Picture.info') down by 15 pixels for text alignment
  Adjusted icon 2 ('Tool.info') down by 20 pixels for text alignment
```

## Visual Comparison

### Before (TOP Alignment)
```
[Tall Icon]          [Short Icon]    [Medium Icon]
    |                     |                |
 Long Name            Short           Medium
```
Text at varying heights - inconsistent appearance.

### After (BOTTOM Alignment - Default)
```
[Tall Icon]          [Short Icon]    [Medium Icon]
    |                     ↓                ↓
 Long Name            Short           Medium
```
All text aligned to same baseline - clean, professional appearance.

## Testing

To verify the feature:

1. Create a test directory with icons of varying heights
2. Run iTidy with default settings (BOTTOM alignment)
3. Verify text labels align horizontally across each row
4. Change to TOP alignment and verify icons align to top of row

## Future Enhancements

Potential improvements:
- Add GUI control to toggle between alignment modes
- Support CENTER alignment (align to middle of tallest icon)
- Allow per-directory alignment preferences
- Add visual indicator in GUI showing current alignment mode

## Compatibility

- **Binary Compatibility**: Maintained (structure size increased but initialized correctly)
- **Configuration Files**: Backward compatible (defaults to BOTTOM if not specified)
- **Amiga OS**: Compatible with OS 2.0+ (no special requirements)

## Files Modified

1. `src/layout_preferences.h` - Added enum and structure field
2. `src/layout_preferences.c` - Updated initialization and presets
3. `src/layout_processor.c` - Implemented two-pass layout algorithm

## Build Status

✅ Successfully compiles with VBCC (no errors, warnings unchanged)  
✅ All existing functionality preserved  
✅ Default behavior provides improved visual consistency
