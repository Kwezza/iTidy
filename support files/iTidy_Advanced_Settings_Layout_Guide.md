# iTidy – Advanced Settings Window  
## Font-Aware Label & Layout Guide for AI Layout Agent

This document tells an AI layout agent **exactly** how to build the *Advanced Settings* window for iTidy, in a way that:

- Obeys the global **AI Agent Layout Guide** (border maths, font handling, pre-calculation).  
- Keeps the window **short enough for PAL** (≈ ≤ 250 px for Topaz-style fonts) by using font-aware vertical layout.
- Uses **fixed horizontal columns** so Kerry can tweak a few constants and recompile.
- Avoids GadTools’ left-hand labels by using **separate TEXT gadgets** (label-less controls).
- Remains **pragmatic and flexible** when documentation or historical knowledge does not perfectly match reality on a real Amiga system.

This file is **specific to the Advanced Settings window**; it does not redefine the generic patterns, but builds on them.

---

## 0. Mandatory Dependencies

Before doing anything, the agent **must** follow these global rules from `AI_AGENT_LAYOUT_GUIDE.md`:

1. **Coordinate system & borders**

   - Window and gadgets use **window-relative coordinates**; (0,0) is the outer top-left, not the client area.
   - Compute `border_top` and friends like this:

     ```c
     struct Screen *screen = LockPubScreen("Workbench");

     WORD border_top    = screen->WBorTop + screen->Font->ta_YSize + 1;
     WORD border_left   = screen->WBorLeft;
     WORD border_right  = screen->WBorRight;
     WORD border_bottom = screen->WBorBottom;

     UnlockPubScreen(NULL, screen);
     ```

   - All gadget `ng_TopEdge` values must be `border_top + something`, **never** just a magic constant.

2. **Font & measurement**

   - Use `GetScreenDrawInfo()` → `draw_info->dri_Font` and `TextLength()` for text measurement.  
   - Initialise a temporary `RastPort` (`InitRastPort` + `SetFont`) for measuring label widths, button text, etc.

3. **Pre-calculation first, gadgets second**

   - Pre-compute all widths, column X positions, equal button widths, etc., **before** `CreateGadget()`.  
   - Gadgets should be created with their final widths; do not try to resize gadgets after creation.

4. **ListView / IDCMP rules**

   - Not needed for this particular window (no ListView), but the general rules still apply in other windows.

---

## 1. Label Renaming (User-Visible Text)

These are the **labels the user sees**. Underlying struct fields and enums do **not** change.

| Original Label          | New UI Label / Group Text                     | Reason / Notes                                                                                 |
|-------------------------|-----------------------------------------------|-----------------------------------------------------------------------------------------------|
| Aspect Ratio            | **Layout Aspect Ratio**                       | Makes it clear this is about the *icon layout* shape, not the monitor.                       |
| Custom Width            | **Custom Ratio: W**                           | Treated as part of a single “Custom Ratio” control (W/H).                                    |
| Custom Height           | **Custom Ratio: H**                           | Shown only when aspect ratio mode = “Custom”.                                                 |
| Window Overflow         | **Overflow Strategy**                         | Emphasises behaviour when icons don’t fit, not an error condition.                           |
| Horizontal Spacing      | **Icon Spacing X**                            | Clarifies that we’re spacing icons, not windows; grouped with Y.                             |
| Vertical Spacing        | **Icon Spacing Y**                            | See above; X/Y appear on one line labelled “Spacing:”.                                        |
| Min Icons/Row           | **Minimum Icons/Row**                         | Full word, clearer to non-technical users.                                                   |
| Auto Max Icons/Row      | **Automatic Max Icons/Row** (checkbox label)  | Explicit about what is automatic.                                                            |
| Max Icons/Row           | **Maximum Icons/Row**                         | Symmetric with “Minimum” and “Automatic Max”.                                                |
| Max Window Width        | **Max Window Width (% Screen)**               | Explains that the value is a percentage of screen width.                                     |
| Vertical Alignment      | **Align Icons Vertically**                    | Describes the effect directly.                                                               |
| Reverse Sort (Z->A)     | **Reverse Sort (Z→A)**                        | Same meaning, better arrow glyph.                                                            |
| Optimize Column Widths  | **Optimize Column Widths**                    | Already clear; no change.                                                                    |
| Skip Hidden Folders     | **Skip Hidden Folders**                       | Already clear.                                                                               |
| Column Layout           | **Column Layout (centered columns)**          | Adds a short hint about what “Column Layout” actually does.                                  |

**Group-level labels** that are pure UI headers (not prefs):

- `"Layout & Spacing"`  
- `"Window Size"`  
- `"Sorting & Behaviour"`

---

## 2. Window Class & Global Constants

The Advanced Settings window is effectively a **“Medium” size window** in the global guide (≈ 60 chars wide).

### 2.1 Title & logical width

```c
#define ITIDY_ADV_WINDOW_TITLE           "iTidy - Advanced Settings"

/* Logical content width in characters (used to derive reference width) */
#define ITIDY_ADV_WINDOW_WIDTH_CHARS     60    /* Medium window */
```

### 2.2 Margins & spacing (Advanced-window specific)

These mirror the general `WINDOW_MARGIN_*` and `WINDOW_SPACE_*` constants but are scoped for this window so they can be tuned independently.

```c
#define ITIDY_ADV_MARGIN_LEFT            15
#define ITIDY_ADV_MARGIN_TOP             10
#define ITIDY_ADV_MARGIN_RIGHT           15
#define ITIDY_ADV_MARGIN_BOTTOM          10

#define ITIDY_ADV_SPACE_X                8    /* Horizontal gap between columns */
#define ITIDY_ADV_SPACE_Y                6    /* Vertical gap between rows */
#define ITIDY_ADV_GROUP_VGAP_ROWS        1    /* Extra blank rows between groups */
```

> Implementation note: you may either:
> - Map these onto the global `WINDOW_MARGIN_*`/`WINDOW_SPACE_*`, **or**
> - Keep them as Advanced-specific constants and use them only in the pre-calc block.

### 2.3 Row metrics (font-aware)

Per the layout guide, height must be derived from the current font:

```c
UWORD font_width  = font->tf_XSize;
UWORD font_height = font->tf_YSize;

UWORD string_height   = font_height + 6;
UWORD checkbox_height = font_height + 4;
UWORD button_height   = font_height + 6;

#define ITIDY_ADV_ROW_HEIGHT(string_height) (string_height)
```

The logical row step is:

```c
UWORD row_step = ITIDY_ADV_ROW_HEIGHT(string_height) + ITIDY_ADV_SPACE_Y;
```

Content Y for row `row_index`:

```c
UWORD content_top = border_top + ITIDY_ADV_MARGIN_TOP;
UWORD row_y = content_top + (row_index * row_step);
```

This keeps the layout **font-height aware** while still giving a predictable vertical structure.

---

## 3. Column Strategy (Font-Aware Labels, Fixed Gadget Columns)

We want:

- Labels measured from the actual screen font.
- Gadgets aligned in **fixed columns**, controlled by a couple of constants.

### 3.1 Pre-scan label widths

Collect all **non-checkbox** labels used in this window:

```c
static const char *adv_labels[] = {
    "Layout Aspect Ratio:",
    "Overflow Strategy:",
    "Spacing:",
    "Icons per Row:",
    "Max Window Width:",
    "Align Icons Vertically:",
    "Layout & Spacing",
    "Window Size",
    "Sorting & Behaviour"
};
```

Compute the maximum label width:

```c
UWORD adv_max_label_width = 0;
for (i = 0; i < NUM_ADV_LABELS; i++) {
    UWORD w = TextLength(&temp_rp, adv_labels[i], strlen(adv_labels[i]));
    if (w > adv_max_label_width) adv_max_label_width = w;
}
```

### 3.2 Column X positions

Use three logical columns:

1. **Label column** – left-aligned labels.  
2. **Gadget column 1** – main gadgets.  
3. **Gadget column 2** – secondary gadgets on the same row (e.g. Overflow).

We define:

```c
/* Left edge of label column (inside borders) */
UWORD adv_label_column_x =
    border_left + ITIDY_ADV_MARGIN_LEFT;

/* First gadget column immediately right of the widest label */
UWORD adv_gadget_col1_x =
    adv_label_column_x + adv_max_label_width + ITIDY_ADV_SPACE_X;

/* Reference content width in pixels (logical 60 chars wide) */
UWORD adv_reference_width =
    font_width * ITIDY_ADV_WINDOW_WIDTH_CHARS;

/* Right-most content edge */
UWORD adv_content_right =
    adv_label_column_x + adv_reference_width;

/* Second gadget column: pushed towards the right, but aligned on font grid */
UWORD adv_gadget_col2_x =
    adv_gadget_col1_x + (adv_reference_width / 2);  /* approx mid + */
```

All gadgets in the same column must share the same X.

---

## 4. Gadget & Label Rules (GadTools)

### 4.1 Absolutely no built-in left-hand labels

For any gadget type that would normally use a label placed to the **left** (cycle, string, integer):

> **Rule:**  
> - Create a **separate `TEXT_KIND` label gadget** in the label column.  
> - Set `ng.ng_GadgetText = NULL` or `""` for the actual control gadget.  
> - Do **not** use `PLACETEXT_LEFT` or GadTools’ automatic left label.

**Example pattern for one row (“Max Window Width”):**

```c
/* Label: "Max Window Width:" */
ng.ng_LeftEdge  = adv_label_column_x;
ng.ng_TopEdge   = row_y;
ng.ng_Width     = TextLength(&temp_rp, "Max Window Width:", 17);
ng.ng_Height    = string_height;
ng.ng_GadgetText = "Max Window Width:";
ng.ng_Flags     = PLACETEXT_IN;

label_gad = CreateGadget(TEXT_KIND, gad, &ng,
    GTTX_Text, "Max Window Width:",
    GTTX_Border, FALSE,
    TAG_END);

/* Cycle gadget: label-less */
ng.ng_LeftEdge   = adv_gadget_col1_x;
ng.ng_TopEdge    = row_y;
ng.ng_Width      = /* pre-calculated width for cycle */;
ng.ng_Height     = string_height;
ng.ng_GadgetText = NULL;  /* IMPORTANT: no built-in label */
ng.ng_Flags      = 0;

max_width_cycle = CreateGadget(CYCLE_KIND, gad, &ng,
    GTCY_Labels, maxWidthLabels,
    TAG_END);
```

### 4.2 Checkbox exception

For **checkbox** gadgets (sorting options, etc.) it is acceptable to use the built-in label text, because the text appears to the right and is part of the click area. The left edges of these checkboxes should all be `adv_label_column_x`.

---

## 5. Group & Row Layout (Font-Aware)

The window has three logical groups:

1. **Layout & Spacing**
2. **Window Size**
3. **Sorting & Behaviour**

We’ll describe the **row index** for each line. Row indices are 0-based relative to `content_top`.

### 5.1 Layout & Spacing Group

**Group header** (optional):

- **Row 0**: “Layout & Spacing” label at `adv_label_column_x`.  
  - `row_y = content_top + 0 * row_step`.

#### Row 1 – Aspect Ratio & Overflow

- Label `"Layout Aspect Ratio:"` at `adv_label_column_x`, `row_y`.
- Gadget 1: **Aspect Ratio cycle** at `adv_gadget_col1_x`, `row_y`.
- Label 2 `"Overflow:"` (short) placed slightly to the left of column 2 (`adv_gadget_col2_x - small_offset`).
- Gadget 2: **Overflow Strategy cycle** at `adv_gadget_col2_x`, `row_y`.

Row index: `row = 1`.

#### Row 2 – Spacing (X & Y)

- Label `"Spacing:"` at label column.
- Gadget A: integer field, **Icon Spacing X**, at `adv_gadget_col1_x`, `row_y`.
- Small inline label `"X"` may be placed just to the left or inside the field if desired.
- Gadget B: integer field, **Icon Spacing Y**, to the right of X field:

  ```c
  x = adv_gadget_col1_x + x_field_width + ITIDY_ADV_SPACE_X;
  ```

Row index: `row = 2`.

#### Row 3 – Icons per Row (Min / Auto / Max)

- Label `"Icons per Row:"` at label column.
- Gadget A: integer field, **Minimum Icons/Row**, at `adv_gadget_col1_x`.
- Gadget B: checkbox, **Automatic Max Icons/Row**, to the right of Min field.
- Gadget C: integer field, **Maximum Icons/Row**, to the right of checkbox field.  
  - Disabled (`GA_Disabled`) when “Automatic Max” is checked.  

Row index: `row = 3`.

### 5.2 Window Size Group

Insert a **group gap** between Layout & Spacing and Window Size by advancing the row:

```c
row += 1 + ITIDY_ADV_GROUP_VGAP_ROWS;
```

#### Row 5 – Max Window Width

- Label `"Max Window Width:"` at label column.
- Gadget: cycle, **Max Window Width (% Screen)**, at `adv_gadget_col1_x`.

Row index: `row = 5` (assuming group gap of 1 row).

#### Row 6 – Align Icons Vertically

- Label `"Align Icons Vertically:"` at label column.
- Gadget: cycle, entries **Top / Middle / Bottom**, at `adv_gadget_col1_x`.

Row index: `row = 6`.

### 5.3 Sorting & Behaviour Group

Add another group gap:

```c
row += 1 + ITIDY_ADV_GROUP_VGAP_ROWS;
```

Checkboxes are stacked, all left-aligned at `adv_label_column_x` and using built-in text labels.

Rows 8–11 (for example):

1. `[ ] Reverse Sort (Z→A)`
2. `[ ] Optimize Column Widths`
3. `[ ] Column Layout (centered columns)`
4. `[ ] Skip Hidden Folders`

Checkbox `ng.ng_LeftEdge = adv_label_column_x`;  
Checkbox `ng.ng_Width` can be `adv_reference_width` or the width of the longest checkbox label + padding.

### 5.4 Buttons (OK / Cancel)

Buttons follow the **equal-button-row** pattern from the layout guide, but with **two equal-width buttons**.

1. Compute available width:

   ```c
   UWORD button_count    = 2;
   UWORD available_width = adv_reference_width;
   ```

2. Find maximum text width for `"OK"` and `"Cancel"` using `TextLength()`.

3. Compute equal button width:

   ```c
   UWORD equal_button_width =
       (available_width - (button_count - 1) * ITIDY_ADV_SPACE_X) / button_count;

   if (equal_button_width < max_btn_text_width + BUTTON_TEXT_PADDING)
       equal_button_width = max_btn_text_width + BUTTON_TEXT_PADDING;
   ```

4. Place buttons on final button row:

   ```c
   UWORD button_row_y = row_y_for_last + checkbox_height + ITIDY_ADV_SPACE_Y;

   UWORD button1_x = adv_label_column_x;
   UWORD button2_x = button1_x + equal_button_width + ITIDY_ADV_SPACE_X;
   ```

   - Button 1: `"OK"`, default action (Return).
   - Button 2: `"Cancel"` (Esc).

---

## 6. Window Height Calculation (PAL-Friendly, Font-Aware)

After gadgets are created:

1. Determine the **last occupied Y**:

   ```c
   UWORD last_y = button_row_y + button_height;
   ```

2. Convert from content coordinates back to **total window height**:

   ```c
   UWORD content_height = (last_y - (border_top + ITIDY_ADV_MARGIN_TOP))
                        + ITIDY_ADV_MARGIN_BOTTOM;

   UWORD window_height = border_top + content_height + border_bottom;
   ```

3. **PAL target:** with Topaz-style fonts (8–9 px), the resulting `window_height` should be ≲ 240–250 px.  
   For larger fonts, the window may grow but must still fit typical PAL/RTG screens.

---

## 7. Summary of Agent Rules (Advanced Window)

1. Use the global AI layout guide for: border calculation, font access, `TextLength()`, pre-calculation, and equal button rows.  
2. Label text must use the “New UI Label” names in section 1.  
3. Pre-scan labels with `TextLength()` to find `adv_max_label_width` and derive column X positions.  
4. Use three columns: label, gadget column 1, gadget column 2; columns are shared across rows.  
5. Vertical positions are font-height based – use `row_step = string_height + ITIDY_ADV_SPACE_Y`.  
6. For non-checkbox controls, never use built-in GadgetText: create a separate `TEXT_KIND` label in the label column and leave gadget text empty.  
7. Checkboxes may use built-in labels, left-aligned at the label column.  
8. Compute button widths with the equal-button pattern and align them to the content width.  
9. Compute `window_height` from actual gadget heights and borders; do not hardcode PAL pixel values.  
10. Keep the final window compact; with default fonts, height should be around ≤ 250 px.

---

## 8. Pragmatic Flexibility & Fallback Rules

The Amiga is an old and sometimes inconsistent platform. Documentation, example code, and even this guide may not be 100% accurate for every combination of:

- Kickstart / Workbench version  
- Graphics card / RTG environment  
- Custom fonts and IControl settings  

To avoid an AI agent “spinning its wheels” trying to obey instructions that do not quite match reality, follow these **pragmatic fallback rules**:

1. **Working code beats theoretical perfection**

   - If strictly following these rules produces **broken layout**, compile errors, or gadgets that render off-screen or unusable, you may **relax the rules**.
   - Prefer a layout that looks correct on a real Amiga over one that is mathematically perfect but wrong in practice.

2. **Border and font quirks**

   - If the computed borders (`WBorTop`, `WBorLeft`, etc.) or font heights clearly misalign gadgets, it is acceptable to:
     - Add or remove a pixel or two of padding, or  
     - Replace a derived value with a small fixed adjustment (e.g. `border_top + 2`) **for that window only**.
   - Keep the spirit of “font-aware and border-aware”, but do not get stuck chasing one-pixel differences forever.

3. **Missing or incompatible APIs**

   - If a recommended function or tag is not available in the target include files or SDK version, fall back to the closest working equivalent instead of failing the task.
   - Example: if a specific GadTools tag is unavailable, use manual sizing and positioning rather than abandoning the gadget.

4. **Cycle / checkbox / label behaviour differences**

   - If in testing a given gadget type behaves differently than expected (e.g. minimum sizes, text placement), you may:
     - Slightly increase its width or height, or  
     - Adjust its X/Y within its row, **as long as the overall column and grouping logic stays recognisable**.

5. **PAL height limit is a guideline**

   - The “≤ 250 px” target is a **soft goal**, not a strict hard limit.
   - If respecting font metrics on a particular system makes the window 10–20 pixels taller than expected but still usable, prefer **correct layout** over hard clipping to 250.

6. **Document any deliberate deviations in comments**

   - Whenever you intentionally relax these rules, add a short code comment explaining why. Example:

     ```c
     /* NOTE: Added +2 pixels to top margin because WB 2.04 Topaz appears slightly higher
      * than expected and clips the first row on real hardware. */
     ```

   - This makes it clear that the deviation is intentional and can be revisited later.

7. **Stop, simplify, and fall back if stuck**

   - If you find yourself making increasingly complex calculations to satisfy this document, that is a sign to **simplify**:
     - Use fewer rows, or merge two controls onto one line.  
     - Replace a dynamic calculation with a simple constant that looks good in practice.  
   - It is better to produce a stable, readable layout with slightly less clever maths than to fail or loop forever.

By following these pragmatic rules, the AI agent should aim for a layout that is:

- **Consistent and tidy**,  
- **Font and border aware**, but also  
- **Robust on real hardware**, even when historic documentation is imperfect.
