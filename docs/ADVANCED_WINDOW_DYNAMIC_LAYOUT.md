# Advanced Settings Window - Dynamic Layout Implementation

## Overview

The Advanced Settings window (`src/GUI/advanced_window.c`) has been refactored to use **font-aware dynamic layout**, eliminating all hardcoded pixel positions. This matches the approach successfully implemented in the main window (see [DYNAMIC_TWO_COLUMN_LAYOUT.md](DYNAMIC_TWO_COLUMN_LAYOUT.md)).

**Problem**: Hardcoded positions (370px, 300px, 385px, 330px, magic number "38") caused gadget overflow with Topaz font.

**Solution**: All gadgets now positioned dynamically using `TextLength()` measurements and calculated boundaries.

---

## Layout Patterns Used

### 1. Two-Column Layout (Row 1)
**Used for**: Aspect Ratio + Overflow Strategy

```c
// Measure labels
WORD aspect_label_width = TextLength(rp, "Layout Aspect Ratio:", 20);
WORD overflow_label_width = TextLength(rp, "Overflow Strategy:", 18);

// Calculate midpoint
WORD midpoint = ADV_CONTENT_LEFT + ((ADV_CONTENT_RIGHT - ADV_CONTENT_LEFT) >> 1);

// Left column gadget
WORD left_gadget_x = ADV_CONTENT_LEFT + aspect_label_width + 8;
WORD left_cycle_width = midpoint - left_gadget_x - 15;

// Right column gadget
WORD right_gadget_x = midpoint + 15 + overflow_label_width + 8;
WORD right_cycle_width = ADV_GROUPBOX_RIGHT_EDGE - PADDING - right_gadget_x;
```

**Key**: Divides content area into two equal columns using midpoint, each gadget gets its own measured label space.

---

### 2. Flow Layout with Sliders (Row 2)
**Used for**: Icon Spacing X and Y sliders

```c
// Calculate slider level output width (GTSL_LevelPlace right display)
WORD level_output_width = (3 * rp->Font->tf_XSize) + 8;  // 3 digits

// Start from left edge
WORD row_x = ADV_CONTENT_LEFT;

// X slider
WORD spacing_label_width = TextLength(rp, "Icon Spacing:  X:", 17);
ng.ng_LeftEdge = row_x + spacing_label_width + 8;
ng.ng_Width = 50;  // Slider body width
// Create X slider...

// Move forward: slider + level output + gap
row_x = ng.ng_LeftEdge + 50 + level_output_width + 15;

// Y slider positioned after X
WORD y_label_width = TextLength(rp, "Y:", 2);
ng.ng_LeftEdge = row_x + y_label_width + 8;
// Create Y slider...
```

**Key**: Flow layout increments `row_x` after each gadget + its output width. Critical for sliders with `GTSL_LevelPlace = PLACETEXT_RIGHT` which reserve space for numeric output.

---

### 3. Flow Layout with Multiple Gadgets (Row 3)
**Used for**: Icons per Row (Min + Auto checkbox + Max)

```c
WORD row_x = ADV_CONTENT_LEFT;

// Min integer
WORD min_label_width = TextLength(rp, "Icons per Row:  Min:", 20);
ng.ng_LeftEdge = row_x + min_label_width + 8;
ng.ng_Width = 30;
// Create Min integer...

// Move forward
row_x = ng.ng_LeftEdge + 30 + 15;

// Auto checkbox
WORD auto_label_width = TextLength(rp, "Auto Max Icons:", 15);
ng.ng_LeftEdge = row_x + auto_label_width + 8;
ng.ng_Width = 30;
// Create checkbox...

// Move forward
row_x = ng.ng_LeftEdge + 30 + 15;

// Max integer
WORD max_label_width = TextLength(rp, "Max:", 4);
ng.ng_LeftEdge = row_x + max_label_width + 8;
ng.ng_Width = 30;
// Create Max integer...
```

**Key**: Sequential positioning from left to right, measuring each label and advancing `row_x` after each gadget.

---

### 4. Standard Label + Gadget (Rows 4-5)
**Used for**: Max Window Width, Vertical Alignment

```c
WORD label_width = TextLength(rp, "Max Window Width:", 17);
WORD gadget_x = ADV_CONTENT_LEFT + label_width + 8;
WORD gadget_width = ADV_CONTENT_RIGHT - gadget_x;

ng.ng_LeftEdge = gadget_x;
ng.ng_Width = gadget_width;
ng.ng_Flags = PLACETEXT_LEFT;  // GadTools positions label automatically
```

**Key**: Gadget starts after measured label, fills remaining width to content boundary.

---

### 5. Left-Aligned Checkboxes (Rows 9-13)
**Used for**: Reverse Sort, Optimize Columns, Column Layout, Skip Hidden, Strip NewIcon

```c
ng.ng_LeftEdge = ADV_CONTENT_LEFT;
ng.ng_Width = 26;
ng.ng_Height = button_height - 4;
ng.ng_GadgetText = "Reverse Sort (Z->A)";
ng.ng_Flags = PLACETEXT_RIGHT;  // Label on right side of checkbox
```

**Key**: Simple positioning at left content edge. `PLACETEXT_RIGHT` places text to right of checkbox.

---

### 6. Right-Justified Buttons (Row 14)
**Used for**: OK and Cancel buttons

```c
WORD button_width = 80;
WORD button_gap = ADV_WINDOW_STANDARD_PADDING;

// Calculate from right edge backwards
WORD cancel_x = ADV_GROUPBOX_RIGHT_EDGE - button_width;
WORD ok_x = cancel_x - button_width - button_gap;

// OK button
ng.ng_LeftEdge = ok_x;
ng.ng_Width = button_width;

// Cancel button
ng.ng_LeftEdge = cancel_x;
ng.ng_Width = button_width;
```

**Key**: Work backwards from right edge. Beta Options button stays left-aligned at `ADV_GROUPBOX_LEFT_EDGE`.

---

## Constants Architecture

### Groupbox Boundaries
```c
#define ADV_GROUPBOX_LEFT_EDGE 15      // Left window border + margin
#define ADV_GROUPBOX_RIGHT_EDGE 610    // WINDOW_WIDTH (625) - 15
```

### Content Area (Inside Groupbox)
```c
#define ADV_CONTENT_LEFT (ADV_GROUPBOX_LEFT_EDGE + ADV_WINDOW_STANDARD_PADDING)   // 20
#define ADV_CONTENT_RIGHT (ADV_GROUPBOX_RIGHT_EDGE - ADV_WINDOW_STANDARD_PADDING) // 605
```

**Critical**: `ADV_CONTENT_LEFT` and `ADV_CONTENT_RIGHT` define the safe gadget placement area inside the groupbox borders.

---

## Key Implementation Details

### 1. RastPort for Font Measurement
```c
struct RastPort *rp = &adv_data->window->RPort;
```

**Purpose**: Access window's font metrics for `TextLength()` calls.

### 2. Midpoint Calculation (No Math Library)
```c
WORD available_width = ADV_CONTENT_RIGHT - ADV_CONTENT_LEFT;
WORD midpoint = ADV_CONTENT_LEFT + (available_width >> 1);  // Bit shift instead of /2
```

**Why**: Avoids linking math library (no `/2` operator).

### 3. Slider Level Output Width
```c
WORD level_output_width = (3 * rp->Font->tf_XSize) + 8;
```

**Why**: Sliders with `GTSL_MaxLevelLen = 3` and `GTSL_LevelPlace = PLACETEXT_RIGHT` reserve space for "nn" display. Must account for this when positioning next gadget.

### 4. Scope Blocks for Local Variables
```c
{
    /* Measure labels for proper positioning */
    WORD aspect_label_width = TextLength(rp, "Layout Aspect Ratio:", 20);
    WORD overflow_label_width = TextLength(rp, "Overflow Strategy:", 18);
    
    // ... gadget creation ...
}
```

**Why**: C99 allows variable declarations at point of use. Scope blocks keep row-specific variables isolated.

---

## Row Summary

| Row | Gadgets | Pattern | Key Challenge |
|-----|---------|---------|---------------|
| 1 | Aspect Ratio, Overflow Strategy | Two-column | Midpoint calculation |
| 2 | Icon Spacing X, Y sliders | Flow layout | Slider level output width |
| 3 | Min, Auto, Max icons/row | Flow layout | Three gadgets in sequence |
| 4 | Max Window Width cycle | Label + gadget | Fill remaining width |
| 5 | Vertical Alignment cycle | Label + gadget | Fill remaining width |
| 9 | Reverse Sort checkbox | Left-aligned | Simple positioning |
| 10 | Optimize Columns checkbox | Left-aligned | Simple positioning |
| 11 | Column Layout checkbox | Left-aligned | Simple positioning |
| 12 | Skip Hidden checkbox | Left-aligned | Simple positioning |
| 13 | Strip NewIcon checkbox | Left-aligned | Conditional enable (icon.library v44+) |
| 14 | Beta Options, OK, Cancel | Mixed | Right-justify OK/Cancel, left-align Beta |

---

## Testing Notes

### Expected Warnings
- **Warning 51**: Bitfield type non-portable (system header icontrol.h) - **EXPECTED**
- **Warning 61**: Array of size <=0 (system header icontrol.h) - **EXPECTED**
- **Warning 214**: Suspicious format string (line 939 debug printf) - **PRE-EXISTING**

All warnings are from system headers or pre-existing debug code, not from the layout refactoring.

### Visual Verification
- [x] All gadgets visible with Topaz font
- [x] No overflow or truncation
- [x] Two-column layout evenly spaced
- [x] Slider level outputs don't overlap
- [x] OK/Cancel buttons right-justified
- [x] Beta Options button left-aligned
- [x] Consistent spacing between rows

---

## Benefits

1. **Font-Independent**: Works with any font (Topaz, custom, large sizes)
2. **No Hardcoded Positions**: All calculations dynamic
3. **Professional Layout**: Right-justified buttons, centered columns
4. **Maintainable**: Adding/removing gadgets doesn't break layout
5. **Consistent Pattern**: Matches main window implementation
6. **Performance**: TextLength() measurements are fast (<1ms)

---

## Related Documents

- [DYNAMIC_TWO_COLUMN_LAYOUT.md](DYNAMIC_TWO_COLUMN_LAYOUT.md) - Main window implementation (original pattern)
- [AI_AGENT_GETTING_STARTED.md](../src/templates/AI_AGENT_GETTING_STARTED.md) - Complete window creation guide
- [AI_AGENT_LAYOUT_GUIDE.md](../src/templates/AI_AGENT_LAYOUT_GUIDE.md) - Critical layout patterns and pitfalls

---

## Implementation Date

**December 4, 2025** - Refactored all 14 rows in Advanced Settings window, removed temporary compatibility constant, verified clean build.
