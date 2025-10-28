# Professional Amiga Window Layout System

## Overview

The iTidy project has developed a highly portable, font-sensitive layout system for creating professional GadTools windows on AmigaOS 2.04+. This system ensures perfect alignment regardless of font choice, screen resolution, or IControl preferences.

## Why This System Exists

Traditional Amiga GUI programming has several pitfalls:
- ListView gadgets adjust their height automatically (requested ≠ actual)
- Labels with `PLACETEXT_LEFT` are drawn to the left of the gadget position
- Modifying gadget dimensions after creation doesn't trigger visual updates
- Direct access to `screen->RastPort.Font` bypasses the proper font system
- `strlen() * font_width` is inaccurate for proportional fonts

Our system solves all of these issues through **pre-calculated dimensions** and **AmigaOS 3.0 Style Guide compliance**.

## Key Features

### ✅ Font-Sensitive Layout
- Uses `GetScreenDrawInfo()` for proper font access (not `screen->RastPort.Font`)
- Uses `TextLength()` for accurate text width measurements (not `strlen() * font_width`)
- Works with proportional and monospaced fonts
- Automatically scales with different font sizes

### ✅ Pre-Calculated Dimensions
- All gadget dimensions calculated **before** `CreateGadget()` calls
- Buttons receive final dimensions at creation time
- No post-creation modification (which doesn't work anyway)
- Eliminates layout bugs caused by gadget dimension changes

### ✅ Professional Alignment
- Buttons can extend to the right edge of reference gadgets
- Button rows with equal widths spanning full window width
- Proper spacing and margins throughout
- Left and right edges perfectly aligned

### ✅ IControl Preference Integration
- Respects window border widths from IControl preferences
- Adjusts for custom title bar heights
- Works with all IControl settings

## Core Components

### 1. Font Information (AmigaOS 3.0 Style)
```c
struct DrawInfo *draw_info;
struct RastPort temp_rp;
struct TextFont *font;

draw_info = GetScreenDrawInfo(screen);
font = draw_info->dri_Font;
font_width = font->tf_XSize;
font_height = font->tf_YSize;

InitRastPort(&temp_rp);
SetFont(&temp_rp, font);

// Use TextLength() for measurements
UWORD label_width = TextLength(&temp_rp, "Label:", 6);

// Clean up when done
FreeScreenDrawInfo(screen, draw_info);
```

### 2. Pre-Calculation Block
```c
/* Establish reference width (widest gadget) */
UWORD listview_width = font_width * 65;
UWORD precalc_max_right = current_x + listview_width;

/* Pre-calculate button to extend to right edge */
UWORD string_right = string_left + string_width;
UWORD button_left = string_right + spacing;
UWORD button_width = precalc_max_right - button_left;
```

### 3. Equal-Width Button Row
```c
UWORD available_width = listview_width;

/* Find maximum text width using TextLength() */
UWORD max_text_width = TextLength(&temp_rp, "Longest Button", 14);

/* Calculate equal width */
UWORD equal_width = (available_width - (2 * spacing)) / 3;

/* Ensure minimum width for text */
if (equal_width < max_text_width + 8)
    equal_width = max_text_width + 8;

/* Position buttons */
button1->LeftEdge = current_x;
button2->LeftEdge = current_x + equal_width + spacing;
button3->LeftEdge = current_x + (2 * equal_width) + (2 * spacing);
```

### 4. ListView Actual Height
```c
/* Create ListView */
listview = CreateGadget(LISTVIEW_KIND, gad, &ng, ...);

/* Get ACTUAL height (not requested height) */
UWORD actual_height = listview->Height;

/* Position next gadget based on actual height */
next_gadget_y = listview->TopEdge + actual_height + spacing;
```

## Files to Study

### Primary Implementation
- **`src/GUI/restore_window.c`** - Complete working example with all patterns
  - Lines 814-843: Pre-calculation block
  - Lines 845-883: Gadgets created with pre-calculated dimensions
  - Lines 959-1012: Equal-width button row
  
### Templates
- **`src/templates/PROFESSIONAL_WINDOW_LAYOUT_TEMPLATE.c`** - Reusable template with detailed comments
- **`src/templates/AI_AGENT_LAYOUT_GUIDE.md`** - Critical patterns and common mistakes

### Documentation
- **`docs/GUI_Styling.md`** - AmigaOS 3.0 font-sensitive GUI scaling guide
- **`docs/LAYOUT_FEATURES_SUMMARY.md`** - History of layout system development

## Usage Pattern

1. **Get font information using DrawInfo**
   ```c
   draw_info = GetScreenDrawInfo(screen);
   font = draw_info->dri_Font;
   InitRastPort(&temp_rp);
   SetFont(&temp_rp, font);
   ```

2. **Pre-calculate all dimensions**
   ```c
   UWORD listview_width = font_width * 65;
   UWORD precalc_max_right = current_x + listview_width;
   UWORD label_width = TextLength(&temp_rp, "Label:", 6);
   UWORD button_width = precalc_max_right - button_left;
   ```

3. **Create gadgets with pre-calculated values**
   ```c
   ng.ng_LeftEdge = button_left;
   ng.ng_Width = button_width;  // Pre-calculated!
   CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
   ```

4. **Use actual ListView height for positioning**
   ```c
   UWORD actual_height = listview->Height;
   next_y = listview->TopEdge + actual_height + spacing;
   ```

5. **Clean up DrawInfo**
   ```c
   FreeScreenDrawInfo(screen, draw_info);
   ```

## Portability Checklist

When creating a new window, verify:

- [ ] Uses `GetScreenDrawInfo()` for font access
- [ ] Uses `TextLength()` for all text measurements
- [ ] All dimensions pre-calculated before `CreateGadget()`
- [ ] ListView actual height used for positioning
- [ ] IControl preferences respected for margins
- [ ] Tested with Topaz 8 and Topaz 9 fonts
- [ ] No gadget overlap in any font configuration
- [ ] Window size accommodates all content
- [ ] Proper spacing between all elements
- [ ] `FreeScreenDrawInfo()` called on all exit paths

## Benefits

### For Developers
- **Predictable** - Layout calculations are explicit and upfront
- **Debuggable** - Log output shows all calculated dimensions
- **Maintainable** - Layout logic is centralized and well-documented
- **Reusable** - Template can be adapted to any window design

### For Users
- **Professional** - Perfect alignment with equal-width buttons
- **Consistent** - Works identically regardless of font choice
- **Compatible** - Respects all system preferences and settings
- **Accessible** - Scales properly with larger fonts for readability

## Example Results

The restore window (`src/GUI/restore_window.c`) demonstrates:
- ✅ "Change" button extends to right edge of ListView
- ✅ Three bottom buttons have equal widths
- ✅ All buttons span the full window width with proper spacing
- ✅ Left edges perfectly aligned
- ✅ Works with any font size

## Common Mistakes to Avoid

1. ❌ **Don't** modify gadget dimensions after creation
2. ❌ **Don't** use `screen->RastPort.Font` directly
3. ❌ **Don't** use `strlen() * font_width` for measurements
4. ❌ **Don't** use requested ListView height for positioning
5. ❌ **Don't** forget to call `FreeScreenDrawInfo()`

Instead:

1. ✅ **Do** pre-calculate all dimensions before creation
2. ✅ **Do** use `GetScreenDrawInfo()` and `dri_Font`
3. ✅ **Do** use `TextLength()` for accurate measurements
4. ✅ **Do** use actual ListView height from `gadget->Height`
5. ✅ **Do** clean up DrawInfo on all exit paths

## Future Applications

This system can be used for:
- **New GUI windows** - Any new window requiring professional layout
- **Dialog boxes** - Requesters with properly aligned buttons
- **Preferences windows** - Settings panels with complex layouts
- **File requesters** - Custom file selection dialogs
- **Progress windows** - Status displays with aligned controls

The template is fully portable and can be adapted to any AmigaOS application requiring professional, font-sensitive GUI layout.

## Related Resources

- AmigaOS 3.0 User Interface Style Guide
- GadTools Library Documentation (`libraries/gadtools.h`)
- Intuition Library Documentation (`intuition/intuition.h`)
- Graphics Library Documentation (`graphics/text.h`)

---

**Conclusion:** This layout system represents the culmination of extensive debugging and testing. It follows AmigaOS 3.0 Style Guide best practices and provides a solid foundation for creating professional, portable GUI applications.
