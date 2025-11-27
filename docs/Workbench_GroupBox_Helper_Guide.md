# Workbench 2/3 Group Box Helper (Bevel Box + Title) – Implementation Guide

This document describes a reusable pattern for drawing **Workbench‑style group frames** around one or more GadTools gadgets, using `DrawBevelBox()` plus a centered caption that adapts to the **current screen font size**.

You can give this to an AI agent as a design/implementation guide, or just drop the code straight into your project and tweak names/spacing.

---

## Goals

- Draw a **3D bevel frame** (using `DrawBevelBox()`) around a logical group of gadgets.
- Automatically compute the **frame rectangle** based on the gadgets' positions and sizes.
- Make the frame and caption **font aware**:
  - Works cleanly with large screen fonts (e.g. 1080p RTG setups).
  - Ensures the caption is centered on the top border line, not overlapping gadgets.
- Provide the final frame rectangle back to the caller so you can layout subsequent gadgets underneath it cleanly.

We split the logic into two functions:

1. `CalcGadgetGroupBoxRect()` – computes the frame rectangle.
2. `DrawGroupBoxWithLabel()` – draws the bevel frame and its caption centered on the top border line.

---

## Configurable Padding Defines

**CRITICAL**: Add these defines at the top of your implementation file to allow easy adjustment of spacing:

```c
/*------------------------------------------------------------------------*/
/* Configurable padding values for group box appearance                  */
/*------------------------------------------------------------------------*/
#define GROUPBOX_PADDING_LEFT    0   /* Left edge padding (pixels) */
#define GROUPBOX_PADDING_RIGHT   4   /* Right edge padding (pixels) */
#define GROUPBOX_PADDING_TOP     0   /* Top edge padding (pixels) */
#define GROUPBOX_PADDING_BOTTOM  5   /* Bottom edge padding (pixels) */

#define GROUPBOX_TITLE_BAND_EXTRA 2  /* Fine-tune vertical alignment of title text */
```

**These values were determined through real-world testing on Workbench 3.x and provide proper visual spacing.**

---

## Required Includes

Make sure you have at least:

```c
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/text.h>
#include <libraries/gadtools.h>

#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/gadtools_protos.h>
```

You must also have a valid `VisualInfo *` from `GetVisualInfo()` for the screen that owns the window (standard GadTools requirement).

---

## 1. Calculating a Font‑Aware Group Box Rectangle

`CalcGadgetGroupBoxRect()` inspects a **contiguous range of gadgets** (from `first` to `last` in the `NextGadget` chain), finds their union rectangle, and then expands it with font‑relative margins to accommodate the caption text on the top border line.

**CRITICAL**: Do NOT call `SetFont(rp, scr->Font)` - this causes crashes! The RastPort already has the correct font set when the window is opened. Simply use the existing font metrics from `rp->TxHeight`.

```c
VOID CalcGadgetGroupBoxRect(struct Window    *win,
                            struct Gadget    *first,
                            struct Gadget    *last,
                            struct Rectangle *box)
{
    struct RastPort *rp  = win->RPort;
    struct Screen   *scr = win->WScreen;
    struct Gadget   *g;
    WORD minX, minY, maxX, maxY;
    WORD fh;          /* font height      */
    WORD hMargin;     /* horizontal pad   */
    WORD innerPad;    /* inside top/bot   */

    if (!win || !first || !last || !box)
        return;

    /* Union of gadget rects */
    minX =  32767; minY =  32767;
    maxX = -32768; maxY = -32768;

    for (g = first; g; g = g->NextGadget)
    {
        WORD l = g->LeftEdge;
        WORD t = g->TopEdge;
        WORD r = g->LeftEdge + g->Width  - 1;
        WORD b = g->TopEdge  + g->Height - 1;

        if (l < minX) minX = l;
        if (t < minY) minY = t;
        if (r > maxX) maxX = r;
        if (b > maxY) maxY = b;

        if (g == last)
            break;
    }

    /* Use existing RastPort font - DO NOT call SetFont()! */
    fh        = rp->TxHeight;
    hMargin   = fh / 2;
    innerPad  = fh / 4;  /* Reduced from fh/3 for tighter fit */

    /* Calculate box bounds with configurable padding */
    box->MinX = minX - hMargin - GROUPBOX_PADDING_LEFT;
    box->MaxX = maxX + hMargin + GROUPBOX_PADDING_RIGHT;
    box->MinY = minY - (fh + 6) - GROUPBOX_PADDING_TOP;  /* Space above gadgets for label */
    box->MaxY = maxY + innerPad + GROUPBOX_PADDING_BOTTOM;
}
```

**Key points:**

- **DO NOT use `SetFont()`** - the font is already set in the RastPort. Calling `SetFont(rp, scr->Font)` with a `TextAttr *` instead of a `TextFont *` causes crashes.
- We use **existing font metrics** via `rp->TxHeight` directly.
- Margins (`hMargin`, `innerPad`) are proportional to font height for automatic scaling.
- Configurable padding values allow fine-tuning the appearance.

This function is purely layout: it does *not* draw anything. It just fills a `struct Rectangle` that describes where the group frame should go.

---

## 2. Drawing the Bevel Frame and Caption

`DrawGroupBoxWithLabel()` uses the rectangle from `CalcGadgetGroupBoxRect()` to:

1. Draw a recessed bevel frame with `DrawBevelBox()`.
2. Draw a centered caption on the top border line, creating a visual "gap" in the border (classic Workbench style).

**Text Positioning Formula (Tested and Validated):**

```c
textY = box->MinY - fb + (fh / 2) - GROUPBOX_TITLE_BAND_EXTRA;
```

Where:
- `fb` = Font baseline offset (`rp->TxBaseline`)
- `fh` = Font height (`rp->TxHeight`)
- `GROUPBOX_TITLE_BAND_EXTRA` = Fine-tuning offset (2 pixels, tested with descenders like "g")

This formula was refined through multiple iterations to achieve perfect vertical centering on the top border line.

**CRITICAL**: Do NOT call `SetFont(rp, scr->Font)` - the RastPort already has the correct font set. Calling SetFont with a `TextAttr *` instead of a `TextFont *` causes crashes.

```c
/*
 * Draw a Workbench‑style group box:
 *
 *  - win      : window to draw into
 *  - box      : rectangle (from CalcGadgetGroupBoxRect)
 *  - label    : caption text ("Folder", "Options", etc.)
 *  - recessed : TRUE for recessed bevel, FALSE for raised
 */
VOID DrawGroupBoxWithLabel(struct Window    *win,
                           struct Rectangle *box,
                           const char       *label,
                           BOOL              recessed)
{
    struct RastPort *rp  = win->RPort;
    struct Screen   *scr = win->WScreen;
    struct DrawInfo *di  = GetScreenDrawInfo(scr);
    struct IntuiText itext;
    WORD textW, textX, textY;
    WORD fh, fb;
    WORD gapLeft, gapRight;

    if (!win || !box || !label || !di)
    {
        if (di) FreeScreenDrawInfo(scr, di);
        return;
    }

    /* Draw the bevel box */
    DrawBevelBox(rp, box->MinX, box->MinY, 
                 box->MaxX - box->MinX + 1,
                 box->MaxY - box->MinY + 1,
                 GT_VisualInfo, di,
                 GTBB_Recessed, recessed,
                 TAG_END);

    /* Set up text appearance - use BACKGROUNDPEN for the "gap" effect */
    itext.FrontPen    = di->dri_Pens[TEXTPEN];
    itext.BackPen     = di->dri_Pens[BACKGROUNDPEN];
    itext.DrawMode    = JAM2;
    itext.LeftEdge    = 0;
    itext.TopEdge     = 0;
    itext.ITextFont   = NULL;
    itext.IText       = (UBYTE *)label;
    itext.NextText    = NULL;

    /* Measure label */
    textW = IntuiTextLength(&itext);

    /* Use existing font metrics - DO NOT call SetFont()! */
    fh = rp->TxHeight;
    fb = rp->TxBaseline;

    /* Center horizontally on the box's top border */
    textX = box->MinX + ((box->MaxX - box->MinX + 1 - textW) / 2);

    /* Position vertically on top border line (tested formula) */
    textY = box->MinY - fb + (fh / 2) - GROUPBOX_TITLE_BAND_EXTRA;

    /* Clear background behind label to create "gap" in border */
    gapLeft  = textX - 3;
    gapRight = textX + textW + 3;
    SetAPen(rp, di->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp, gapLeft, box->MinY - 1, gapRight, box->MinY + 1);

    /* Print the text */
    PrintIText(rp, &itext, textX, textY);

    FreeScreenDrawInfo(scr, di);
}
```

**Key points:**

- **DO NOT use `SetFont()`** - causes crashes! The font is already set in the RastPort.
- Text positioning uses the **tested formula** with `GROUPBOX_TITLE_BAND_EXTRA` offset.
- **RectFill** with `BACKGROUNDPEN` creates a visual "gap" in the top border behind the text.
- **JAM2** draw mode ensures the background is filled properly.
- The `recessed` parameter allows for recessed (TRUE) or raised (FALSE) bevel boxes.

This function should be called **during a refresh** (e.g. inside your `IDCMP_REFRESHWINDOW` handler).

---

## 3. Typical Usage in a Window

**Important Note About Gadget Labels:**

When creating gadgets that will be grouped in a box, do NOT use `PLACETEXT_LEFT` on the gadgets themselves. Instead, create separate TEXT gadgets for labels. This is because:
- Labels created with `PLACETEXT_LEFT` are NOT included in the gadget's bounding rectangle
- `CalcGadgetGroupBoxRect()` won't account for them, causing misalignment
- Separate TEXT gadgets are positioned explicitly and included in the box calculation

**Example: Creating a "Folder" Group**

```c
struct Rectangle folder_group_box;
struct Gadget *folder_label;
struct Gadget *folder_path_gadget;
struct Gadget *browse_btn;

/* Create TEXT gadget for "Folder:" label (NOT PLACETEXT_LEFT!) */
ng.ng_LeftEdge   = current_x;
ng.ng_TopEdge    = current_y;
ng.ng_Width      = 55;  /* Width for "Folder:" text */
ng.ng_Height     = fh;
ng.ng_GadgetText = "Folder:";
ng.ng_Flags      = 0;

folder_label = CreateGadget(TEXT_KIND, prev_gadget, &ng,
                           GT_Underscore, '_',
                           TAG_DONE);

/* Create STRING gadget for path (no label!) */
ng.ng_LeftEdge   = current_x + 60;
ng.ng_Width      = 400;
ng.ng_GadgetText = NULL;  /* No PLACETEXT_LEFT! */

folder_path_gadget = CreateGadget(STRING_KIND, folder_label, &ng,
                                 GTST_String, "RAM:",
                                 GTST_MaxChars, 255,
                                 TAG_DONE);

/* Create BUTTON gadget for browse */
ng.ng_LeftEdge   = current_x + 465;
ng.ng_Width      = 80;
ng.ng_GadgetText = "Browse...";

browse_btn = CreateGadget(BUTTON_KIND, folder_path_gadget, &ng,
                         TAG_DONE);

/* AFTER window is opened, calculate the groupbox */
CalcGadgetGroupBoxRect(win, folder_label, browse_btn, &folder_group_box);

/* Draw during IDCMP_REFRESHWINDOW */
case IDCMP_REFRESHWINDOW:
    GT_BeginRefresh(win);
    DrawGroupBoxWithLabel(win, &folder_group_box, "Folder", TRUE);
    GT_EndRefresh(win, TRUE);
    break;

/* Layout next gadgets below the groupbox */
WORD next_y = folder_group_box.MaxY + fh;  /* One font-height spacing */
```

---

## 4. Using Group Box Dimensions for Dynamic Layout

The `CalcGadgetGroupBoxRect()` function fills in a `struct Rectangle *box` parameter with the calculated frame coordinates. You can use these values to position subsequent gadgets dynamically.

**Important Timing Constraint:**
- Gadgets must be created **before** the window opens (during `CreateContext()`/`CreateGadget()` calls)
- `CalcGadgetGroupBoxRect()` must be called **after** the window opens (when gadgets have final geometry)
- Therefore, you **cannot** use groupbox dimensions during initial gadget creation

**What You CAN Do:**

### Position Gadgets Below the Groupbox
After calculating the groupbox, use `box->MaxY` for positioning elements that are created/updated after window open:

```c
/* After window opens and groupbox is calculated */
CalcGadgetGroupBoxRect(win, first_gad, last_gad, &group_box);

/* Use MaxY to position next row of gadgets */
WORD next_row_y = group_box.MaxY + (font_height * 2);  /* 2 line spacing */

/* This works for:
 * - Dynamically created gadgets (rare in GadTools)
 * - Drawing custom graphics below the groupbox
 * - Calculating window height dynamically
 * - Positioning other groupboxes
 */
```

### Position Elements to the Right of Groupbox
Use `box->MaxX` to place elements horizontally adjacent:

```c
CalcGadgetGroupBoxRect(win, first_gad, last_gad, &left_group_box);

/* Position another groupbox or gadget to the right */
WORD right_column_x = left_group_box.MaxX + 20;  /* 20 pixel gap */

/* Useful for multi-column layouts */
```

### Calculate Window Dimensions
Use groupbox dimensions to determine optimal window size:

```c
CalcGadgetGroupBoxRect(win, first_gad, last_gad, &bottom_group_box);

/* Calculate required window height */
WORD required_height = bottom_group_box.MaxY + border_bottom + 10;
```

**Why This Ordering Matters:**
GadTools requires all gadgets to be created before opening the window. The groupbox is a visual decoration drawn **on top of** existing gadgets, not a container that controls layout. Think of it as "chrome" that highlights existing gadget groups rather than a layout manager.

**Best Practice for Static Layouts:**
For most GadTools windows, gadgets are positioned with fixed offsets during creation, then groupboxes are drawn around them after window open. Use `current_y` tracking during gadget creation, then use groupbox dimensions for spacing calculations in comments or for runtime adjustments.

---

## 5. Why This Scales on RTG / Big Fonts

Because the layout is derived from the **current screen font metrics**:

- Larger fonts increase:
  - The gadgets' natural size.
  - The text positioning calculations.
  - The horizontal/vertical margins around the gadgets.
- The group frame simply grows with the content.
- The caption is always positioned on the top border line, so it does not overlap even with very big fonts on high‑resolution RTG screens.

This makes the group frame **future‑proof** for users who prefer chunky fonts on 1080p+ displays, while still looking correct on classic 640×256 Workbench with small fonts.

---

## 6. Common Pitfalls and Solutions

### SetFont() Crash
**Problem**: Calling `SetFont(rp, scr->Font)` causes Guru 8100 0005.  
**Solution**: Don't call `SetFont()` at all. The RastPort already has the correct font set. Just use `rp->TxHeight` and `rp->TxBaseline` directly.

### Text Misaligned Vertically
**Problem**: Text appears too high or too low on the border line.  
**Solution**: Use the tested formula: `textY = box->MinY - fb + (fh / 2) - GROUPBOX_TITLE_BAND_EXTRA` where `GROUPBOX_TITLE_BAND_EXTRA` = 2.

### Labels Not Included in Groupbox
**Problem**: Using `PLACETEXT_LEFT` on gadgets causes labels to be cut off by the groupbox.  
**Solution**: Create separate TEXT gadgets for labels instead of using `PLACETEXT_LEFT`.

### Top Border Clipped
**Problem**: The top border line is cut off or too close to gadgets.  
**Solution**: Use `box->MinY = minY - (fh + 6)` in the calculation, not a fixed title band height.

---

## 7. Possible Extensions

Things you (or an AI agent) might want to add later:

- A parameter or flags to choose **recessed vs. raised** frames (`GTBB_Recessed` vs. normal). ✅ *Already implemented!*
- Left‑aligned captions instead of centered (for some preference panes).
- Optional **inner margin overrides** (e.g. special groups that need extra padding).
- RTL / mirrored layouts (if you ever go down that road).

But the core pattern above should cover the common "Workbench 3.x group box" use‑case very nicely.

---

## 8. Implementation Checklist

When implementing this in your own code:

- [ ] Add the configurable padding `#define` statements
- [ ] Include all required headers
- [ ] Create TEXT gadgets for labels (not PLACETEXT_LEFT)
- [ ] Call `CalcGadgetGroupBoxRect()` AFTER window is opened
- [ ] Draw groupbox in `IDCMP_REFRESHWINDOW` handler
- [ ] Use `box->MaxY + fh` for positioning gadgets below the groupbox
- [ ] Test with characters that have descenders (g, j, p, q, y) to verify vertical alignment
- [ ] Verify no crashes on window open/refresh

---

**This guide was created through real-world testing and bug fixes on AmigaOS 3.x/Workbench 3.0. All formulas and values have been validated in production code.**
