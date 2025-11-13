# GadTools Layout and Alignment Strategy

## Overview
This document describes the systematic approach for creating well-aligned GadTools windows in iTidy. These patterns ensure consistent, professional-looking interfaces that work across different Workbench screen fonts.

## Key Principles

### 1. Gadget Heights Are Available Immediately
After calling `CreateGadget()`, the actual rendered height is immediately available via `gad->Height`. This allows for:
- Dynamic vertical alignment calculations
- Responsive layout based on actual dimensions
- No need for hardcoded pixel offsets

```c
data->some_gadget = gad = CreateGadget(STRING_KIND, gad, &ng, ...);
UWORD actual_height = gad->Height;  /* Available immediately */
current_y += gad->Height + spacing;  /* Use for next row */
```

### 2. Pre-Calculated Heights for Common Gadgets
Standard gadget types have predictable heights based on font metrics:

```c
/* Calculate gadget heights */
button_height = font_height + 6;
string_height = font_height + 6;
listview_line_height = font_height + 2;
checkbox_height = font_height;  /* Usually matches font height */
```

## Vertical Alignment Strategy

### Aligning Labels with Input Gadgets

When placing a TEXT gadget (label) next to a taller input gadget (STRING, BUTTON, etc.), use vertical centering:

```c
/* PATTERN: Label + Input Gadget on Same Row */

/* Step 1: Define row baseline at current_y */
UWORD row_baseline = current_y;

/* Step 2: Create label with vertical offset for centering */
ng.ng_LeftEdge = current_x;  /* Left margin */
ng.ng_TopEdge = row_baseline + (string_height - font_height) / 2;  /* Centered */
ng.ng_Width = label_width;
ng.ng_Height = font_height;
ng.ng_GadgetText = NULL;
ng.ng_GadgetID = 0;
ng.ng_Flags = 0;

label_gad = gad = CreateGadget(TEXT_KIND, gad, &ng,
    GTTX_Text, "Label:",
    GTTX_Border, FALSE,
    TAG_END);

/* Step 3: Create input gadget at row baseline */
ng.ng_LeftEdge = current_x + label_width + 4;  /* After label + gap */
ng.ng_TopEdge = row_baseline;  /* Baseline */
ng.ng_Width = input_width;
ng.ng_Height = string_height;
ng.ng_GadgetText = NULL;
ng.ng_GadgetID = GID_SOME_STRING;
ng.ng_Flags = 0;

input_gad = gad = CreateGadget(STRING_KIND, gad, &ng, ...);

/* Step 4: Advance to next row using tallest gadget's height */
current_y = row_baseline + string_height + SPACING_Y;
```

### Formula Reference

**Vertical centering formula:**
```
label_top = row_baseline + (taller_gadget_height - label_height) / 2
```

**Common combinations:**
- Label + String: `label_top = baseline + (string_height - font_height) / 2` → typically +3 pixels
- Label + Button: `label_top = baseline + (button_height - font_height) / 2` → typically +3 pixels
- Label + Checkbox: Usually equal heights, no offset needed

## Horizontal Alignment Strategy

### Left-Aligned Gadgets
All gadgets should align to a consistent left margin:

```c
UWORD current_x = WINDOW_MARGIN_LEFT + prefsIControl.currentLeftBarWidth;

/* All gadgets start at current_x for consistency */
ng.ng_LeftEdge = current_x;
```

### Multi-Column Layouts
For labels followed by input gadgets:

```c
/* Calculate widths */
UWORD label_width = TextLength(&temp_rp, "Label text:", 11);
UWORD gap = 4;  /* Standard gap between label and input */

/* Label column */
ng.ng_LeftEdge = current_x;
ng.ng_Width = label_width;

/* Input column */
ng.ng_LeftEdge = current_x + label_width + gap;
ng.ng_Width = input_width;

/* Button column (to right of input) */
ng.ng_LeftEdge = current_x + label_width + gap + input_width + SPACING_X;
ng.ng_Width = button_width;
```

## TEXT Gadget Refresh Requirements

**CRITICAL:** All `TEXT_KIND` gadgets require explicit refresh to display their content:

```c
/* After opening window, refresh all TEXT gadgets */
GT_RefreshWindow(window, NULL);
RefreshGList(text_gadget_1, window, NULL, 1);
RefreshGList(text_gadget_2, window, NULL, 1);
/* ... refresh each TEXT gadget ... */
```

## Complete Row Pattern Examples

### Example 1: Label + String + Button Row

```c
/*--------------------------------------------------------------------*/
/* INPUT ROW: Label + String + Button                                */
/*--------------------------------------------------------------------*/
UWORD row_baseline = current_y;
UWORD label_width = TextLength(&temp_rp, "Change to:", 10);
UWORD gap = 4;

/* Label (vertically centered) */
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = row_baseline + (string_height - font_height) / 2;
ng.ng_Width = label_width;
ng.ng_Height = font_height;
ng.ng_GadgetText = NULL;
ng.ng_GadgetID = 0;
ng.ng_Flags = 0;

data->label = gad = CreateGadget(TEXT_KIND, gad, &ng,
    GTTX_Text, "Change to:",
    GTTX_Border, FALSE,
    TAG_END);

/* String gadget (at baseline) */
ng.ng_LeftEdge = current_x + label_width + gap;
ng.ng_TopEdge = row_baseline;
ng.ng_Width = input_width;
ng.ng_Height = string_height;
ng.ng_GadgetText = NULL;
ng.ng_GadgetID = GID_INPUT;
ng.ng_Flags = 0;

data->input = gad = CreateGadget(STRING_KIND, gad, &ng,
    GTST_String, buffer,
    GTST_MaxChars, 255,
    TAG_END);

/* Browse button (at baseline) */
ng.ng_LeftEdge = current_x + label_width + gap + input_width + SPACING_X;
ng.ng_TopEdge = row_baseline;
ng.ng_Width = browse_width;
ng.ng_Height = button_height;
ng.ng_GadgetText = "Browse...";
ng.ng_GadgetID = GID_BROWSE;
ng.ng_Flags = PLACETEXT_IN;

data->browse = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

/* Advance to next row */
current_y = row_baseline + button_height + SPACING_Y;
```

### Example 2: Full-Width Button Row

```c
/*--------------------------------------------------------------------*/
/* BUTTON ROW: Full-Width Button                                     */
/*--------------------------------------------------------------------*/
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = reference_width;  /* Full width */
ng.ng_Height = button_height;
ng.ng_GadgetText = "Update";
ng.ng_GadgetID = GID_UPDATE;
ng.ng_Flags = PLACETEXT_IN;

data->update_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

current_y += gad->Height + SPACING_Y;
```

### Example 3: Checkbox Row

```c
/*--------------------------------------------------------------------*/
/* CHECKBOX ROW: Checkbox with Right-Aligned Label                   */
/*--------------------------------------------------------------------*/
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = font_height;  /* Checkbox is square */
ng.ng_Height = font_height;
ng.ng_GadgetText = "Enable backup feature";
ng.ng_GadgetID = GID_BACKUP;
ng.ng_Flags = PLACETEXT_RIGHT;

data->backup = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
    GTCB_Checked, FALSE,
    TAG_END);

current_y += gad->Height + SPACING_Y;
```

## Why Separate Labels vs. PLACETEXT_LEFT

### PLACETEXT_LEFT Issues
- GadTools places the label to the LEFT of `ng.ng_LeftEdge`
- This makes it impossible to align labels at a consistent left margin
- Label position is relative to the input gadget, not absolute

```c
/* AVOID: PLACETEXT_LEFT breaks left margin alignment */
ng.ng_LeftEdge = current_x + label_width + gap;  /* Input position */
ng.ng_GadgetText = "Label:";  /* Appears BEFORE current_x - misaligned! */
ng.ng_Flags = PLACETEXT_LEFT;
```

### Separate TEXT Gadget Benefits
- Absolute positioning control
- Consistent left margin alignment
- Independent vertical centering
- Easier to maintain and debug

```c
/* PREFERRED: Separate TEXT gadget for precise control */
ng.ng_LeftEdge = current_x;  /* Exactly at left margin */
ng.ng_GadgetText = NULL;
ng.ng_Flags = 0;
CreateGadget(TEXT_KIND, gad, &ng, GTTX_Text, "Label:", TAG_END);
```

## Layout Workflow

1. **Calculate Heights**
   ```c
   button_height = font_height + 6;
   string_height = font_height + 6;
   ```

2. **Set Row Baseline**
   ```c
   UWORD row_baseline = current_y;
   ```

3. **Create Label (Centered)**
   ```c
   ng.ng_TopEdge = row_baseline + (taller_height - font_height) / 2;
   ```

4. **Create Input (At Baseline)**
   ```c
   ng.ng_TopEdge = row_baseline;
   ```

5. **Advance Current Y**
   ```c
   current_y = row_baseline + tallest_gadget_height + spacing;
   ```

## Font Considerations

### Proportional vs. Fixed-Width Fonts
- Screen font may be proportional
- Use `TextLength()` for accurate width calculations
- For columnar data (ListView), consider using system default fixed font

```c
/* Check if screen font is proportional */
if (font->tf_Flags & FPF_PROPORTIONAL)
{
    /* Open system default fixed font */
    data->system_font = OpenFont(&system_font_attr);
    font = data->system_font;
}
```

### Measuring Text Width

```c
struct RastPort temp_rp;
InitRastPort(&temp_rp);
SetFont(&temp_rp, font);

UWORD text_width = TextLength(&temp_rp, "Text", 4);
```

## Common Spacing Values

```c
#define WINDOW_MARGIN_LEFT      10
#define WINDOW_MARGIN_TOP       10
#define WINDOW_MARGIN_RIGHT     10
#define WINDOW_MARGIN_BOTTOM    10
#define SPACING_X               8   /* Horizontal gap between gadgets */
#define SPACING_Y               8   /* Vertical gap between rows */
#define LABEL_INPUT_GAP         4   /* Gap between label and input */
#define BUTTON_PADDING          8   /* Extra padding inside buttons */
```

## Implementation Checklist

When creating a new GadTools window:

- [ ] Calculate font-based heights (button, string, etc.)
- [ ] Initialize RastPort for TextLength measurements
- [ ] Set consistent left margin with `current_x`
- [ ] For each row:
  - [ ] Set `row_baseline = current_y`
  - [ ] Create labels with vertical centering offset
  - [ ] Create input gadgets at baseline
  - [ ] Advance `current_y` using tallest gadget height
- [ ] Refresh all TEXT gadgets after window opens
- [ ] Use `gad->Height` for dynamic spacing (not hardcoded values)

## Reference Implementation

See `src/GUI/default_tool_update_window.c` for a complete working example of these patterns, particularly the "Change to:" label + string + button row.

## Benefits of This Strategy

1. **Consistent alignment** across all window elements
2. **Font-independent** layouts that work with different screen fonts
3. **Maintainable code** with clear spacing logic
4. **Reusable patterns** for future windows
5. **Professional appearance** matching native Workbench UI
6. **AI-friendly** - clear patterns that can be automatically applied

---
*Last updated: November 13, 2025*
