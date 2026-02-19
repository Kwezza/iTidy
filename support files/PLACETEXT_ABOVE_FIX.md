# PLACETEXT_ABOVE Label Spacing Fix

## Issue Identified

The ListView label "List" was appearing too close to the top of the window border, as shown in the test screenshot. This violated the recommendations in `AI_AGENT_LAYOUT_GUIDE.md`.

## Root Cause

According to the AI_AGENT_LAYOUT_GUIDE.md (lines 916-996):

> **THE PROBLEM:** ListView gadgets with `PLACETEXT_ABOVE` labels add extra vertical space above the ListView that varies with font size. If you don't account for this, your layout calculations will be wrong and gadgets will overlap or have incorrect spacing.

> **ROOT CAUSE:** When you use `ng.ng_GadgetText` with `PLACETEXT_ABOVE`, GadTools automatically adds vertical space above the ListView for the label text (typically `font_height + small_gap`). **This space is NOT included in `ng.ng_Height`** and must be accounted for separately.

## The Fix

### Before (Incorrect)
```c
current_y = font_dims->window_top_edge;

/* Main ListView - positioned at left side of window */
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;  /* ❌ No space for label! */
ng.ng_GadgetText = "List";
ng.ng_Flags = PLACETEXT_ABOVE;
```

**Problem:** The ListView was positioned directly at `window_top_edge`, leaving no room for the "List" label above it, causing the label to appear cramped against the window border.

### After (Correct)
```c
current_y = font_dims->window_top_edge;

/* Add space for the "List" label that will appear above the ListView */
current_y += font_dims->font_height + 4;  /* Label height + gap */

/* Main ListView - positioned at left side of window */
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;  /* ✅ Now includes space for label! */
ng.ng_GadgetText = "List";
ng.ng_Flags = PLACETEXT_ABOVE;
```

**Solution:** Add `font_height + 4` pixels to `current_y` BEFORE positioning the ListView. This provides proper spacing for the label.

## Window Height Calculation Update

Also updated the window height calculation to explicitly document the label spacing:

```c
window_height = font_dims.window_top_edge +      /* Top margin with title bar */
                font_dims.font_height + 4 +      /* ListView label "List" + gap (PLACETEXT_ABOVE spacing) */
                actual_listview_height +         /* ACTUAL ListView height (post-snapping) */
                (TEMPLATE_SPACE_Y * 2) +         /* Spacing before buttons */
                font_dims.button_height +        /* Button height */
                TEMPLATE_SPACE_Y +               /* Bottom margin */
                20;                              /* Extra border space */
```

## Documentation Added

Added important layout rules to the template header comments:

```c
/* IMPORTANT LAYOUT RULES (see AI_AGENT_LAYOUT_GUIDE.md):
 * - When using PLACETEXT_ABOVE with a label, you MUST add space BEFORE
 *   positioning the gadget: current_y += font_height + 4
 * - The label space is NOT included in ng.ng_Height
 * - Account for this spacing in window height calculations
 * - Alternative: Use empty label ("") and create separate TEXT gadget
 */
```

## Recommended Best Practice (per AI_AGENT_LAYOUT_GUIDE.md)

The guide recommends using an **empty label** and creating a separate TEXT gadget for even more control:

```c
/* Step 1: Create separate TEXT gadget for the label */
ng.ng_GadgetText = "List";
label_gadget = CreateGadget(TEXT_KIND, gad, &ng, ...);
current_y += label_height + 4;

/* Step 2: Create ListView with EMPTY label */
ng.ng_TopEdge = current_y;
ng.ng_GadgetText = "";  /* EMPTY STRING - no built-in label! */
ng.ng_Flags = PLACETEXT_ABOVE;
listview = CreateGadget(LISTVIEW_KIND, gad, &ng, ...);
```

This approach is more flexible but adds complexity. For this template, the simpler fix (adding space before positioning) is adequate.

## Verification

✅ **Compilation Status:** Clean build, zero warnings, zero errors
✅ **Follows Guide:** Now complies with AI_AGENT_LAYOUT_GUIDE.md recommendations
✅ **Documentation:** Template includes clear comments explaining the pattern
✅ **Consistency:** Window height calculation properly accounts for label space

## Impact

This fix ensures:
- Proper visual spacing between window border and ListView label
- Layout works correctly with different font sizes
- Template demonstrates correct PLACETEXT_ABOVE usage
- AI agents learn the proper pattern from this example

---

**Fixed:** November 14, 2025
**Files Modified:** `src/templates/amiga_window_template.c`
**Status:** ✅ VERIFIED AND COMPILED
