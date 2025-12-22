# Dynamic Two-Column Layout System

## Overview

This document describes the font-aware, dynamic two-column layout system implemented for the iTidy main window's "Tidy Options" section. This approach ensures proper visual alignment and spacing regardless of the system font (Topaz, custom fonts, etc.) or window width.

## The Problem

### Original Hardcoded Approach

The original implementation used fixed pixel positions that did not adapt to different fonts or window sizes:

```c
/* Hardcoded positions - brittle and font-unaware */
#define ITIDY_WINDOW_LEFT_GROUP_GADETS 95
#define ITIDY_WINDOW_LEFT_GROUP_GADETS_COLUMN_2 295

ng.ng_LeftEdge = 260;        /* "By:" label - arbitrary position */
ng.ng_Width = 140;           /* Order cycle - fixed width */
ng.ng_Width = 90;            /* By cycle - different fixed width */
```

**Issues:**
1. **No font awareness** - Labels and gadgets didn't account for actual text width in different fonts
2. **Unequal columns** - Left and right columns had different widths (205px vs 190px)
3. **Poor spacing** - Inconsistent gaps between labels and gadgets
4. **No window scaling** - Didn't utilize full available width when window size changed
5. **Topaz font overflow** - Default Topaz font's wider characters caused text to overflow buttons

### Visual Problems
- "Order:" label too far from its gadget
- "By:" label too far from its gadget  
- "Position:" label too close to its gadget
- Right column gadgets extending beyond visible area

## The Solution

### Dynamic Midpoint-Based Layout

The new system calculates everything dynamically based on:
1. **Actual font metrics** using `TextLength()`
2. **Groupbox boundaries** for proper containment
3. **Midpoint calculation** for equal column division
4. **Label width measurement** for perfect alignment

### Implementation Details

#### Step 1: Measure Label Widths
```c
struct RastPort *rp = &win_data->screen->RastPort;

/* Measure actual pixel width of each label text */
WORD order_label_width = TextLength(rp, "Order:", 6);
WORD by_label_width = TextLength(rp, "By:", 3);
WORD position_label_width = TextLength(rp, "Position:", 9);

/* Find widest label in each column */
WORD left_label_width = (order_label_width > position_label_width) ? order_label_width : position_label_width;
WORD right_label_width = by_label_width;
```

**Why:** Different fonts have different character widths. Measuring ensures labels are properly sized for any font.

#### Step 2: Calculate Midpoint Using Bit Shift
```c
/* Calculate usable content area (groupbox minus padding) */
available_width = ITIDY_GROUPBOX_RIGHT_EDGE - ITIDY_GROUPBOX_LEFT_EDGE - (2 * ITIDY_WINDOW_STANDARD_PADDING);
WORD content_left = ITIDY_GROUPBOX_LEFT_EDGE + ITIDY_WINDOW_STANDARD_PADDING;

/* Find midpoint using bit shift (avoids division - no math library needed) */
WORD midpoint = content_left + (available_width >> 1);  /* >> 1 = divide by 2 */
```

**Why:** 
- Bit shift (`>> 1`) divides by 2 without linking math library
- Midpoint ensures equal column widths
- Works for any window width (supports future resizing)

#### Step 3: Position Gadgets Based on Midpoint
```c
/* Left column: starts after content padding + label width + small gap */
left_col_gadget_x = content_left + left_label_width + 8;

/* Right column: starts at midpoint + spacing + label width + gap */
right_col_gadget_x = midpoint + 15 + right_label_width + 8;
```

**Why:**
- Gadget position = label space + gap (GadTools places label left of gadget automatically)
- Left column gadgets line up at same X position
- Right column gadgets line up at same X position
- Perfect vertical alignment of labels in each column

#### Step 4: Calculate Gadget Widths to Fill Columns
```c
/* Left column gadgets fill from their position to midpoint */
left_cycle_width = midpoint - left_col_gadget_x - 15;

/* Right column gadgets fill from their position to right edge */
right_cycle_width = ITIDY_GROUPBOX_RIGHT_EDGE - ITIDY_WINDOW_STANDARD_PADDING - right_col_gadget_x;
```

**Why:**
- Gadgets expand to use available space
- Maintains visual balance
- Adapts to different label widths

#### Step 5: Use GadTools Built-In Label Support
```c
ng.ng_GadgetText = "Order:";
ng.ng_Flags = PLACETEXT_LEFT;  /* GadTools positions label automatically */

CreateGadget(CYCLE_KIND, gad, &ng, ...);
```

**Why:**
- GadTools handles label positioning and right-justification automatically
- Consistent spacing (8px gap) managed by system
- No need for separate TEXT gadgets
- Reduces code complexity (removed ~100 lines)

## Key Benefits

### 1. Font Independence
Labels and gadgets adapt to any font:
- Topaz 8 (default Workbench font)
- Custom TTF fonts
- Larger accessibility fonts
- Proportional vs. fixed-width fonts

### 2. Perfect Alignment
- Labels in left column align vertically
- Labels in right column align vertically
- Gadgets in left column align vertically
- Gadgets in right column align vertically
- Consistent spacing throughout

### 3. Scalability
System adapts to:
- Window width changes (if made resizable later)
- Different screen resolutions
- Different Workbench versions (2.x, 3.x)

### 4. Cleaner Code
- No hardcoded magic numbers
- Self-documenting calculations
- Easier to maintain
- Follows Amiga UI conventions

## Technical Notes

### Avoiding Division
The code uses bit shift (`>> 1`) instead of division to avoid linking the math library:
```c
midpoint = content_left + (available_width >> 1);  // Bit shift = divide by 2
```

This is a common Amiga optimization technique.

### GadTools Label Placement
When `PLACETEXT_LEFT` is used:
1. GadTools measures the label text width
2. Reserves space to the LEFT of `ng_LeftEdge`
3. Right-justifies text in that space
4. Adds consistent gap before gadget
5. All labels in column align perfectly

### Why Not Equal Column Division?
Early attempts divided available space equally between columns, but this didn't account for:
- Different label widths in each column ("Position:" vs "By:")
- GadTools automatic label spacing
- Need for visual balance

The midpoint approach creates visually equal columns while allowing labels to have different widths.

## Future Enhancements

This system is ready for:
- Window resizing (recalculate on WA_SizeChange)
- Additional columns (use `>> 2` for 4 columns)
- Vertical centering within columns
- Font change notifications

## Conclusion

This dynamic layout system demonstrates proper Amiga GUI programming:
- **Font-aware** - Uses actual text metrics
- **Resolution-independent** - Adapts to any window size
- **System-compliant** - Follows GadTools conventions
- **Performance-optimized** - No division, efficient calculations
- **Maintainable** - Self-documenting, no magic numbers

The result is a professional, polished interface that works correctly with any system font and looks native on any Amiga configuration.
