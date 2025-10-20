# AI Agent Layout Guide for Amiga Window Template

This document provides critical guidance for AI agents working with the Amiga window template. These patterns were discovered through extensive debugging and must be followed exactly to avoid layout issues.

## ⚠️ CRITICAL PATTERNS - DO NOT DEVIATE! ⚠️

### 1. ListView Height "Snapping" Issue

**THE PROBLEM:** ListView gadgets automatically adjust their height to show complete rows. The height you request is NOT the height you get.

**WRONG WAY (causes overlap):**
```c
// NEVER DO THIS - using requested height for positioning
ng.ng_Height = font_height * 10;  // Request 10 lines
listview = CreateGadget(LISTVIEW_KIND, ...);
button_y = listview_y + ng.ng_Height;  // WRONG! Uses requested, not actual
```

**CORRECT WAY:**
```c
// ALWAYS DO THIS - use actual height after creation
ng.ng_Height = font_height * 10;  // Request 10 lines
listview = CreateGadget(LISTVIEW_KIND, ...);
UWORD actual_height = listview->Height;  // Get ACTUAL height
button_y = listview->TopEdge + actual_height + spacing;  // Use actual dimensions
```

**KEY RULES:**
- Create ListView gadgets FIRST
- Always use `gadget->Height` for actual dimensions
- DO NOT use `GT_GetGadgetAttrs()` for basic geometry (unreliable)
- Position other gadgets based on actual ListView dimensions

### 2. PLACETEXT_LEFT Label Positioning

**THE PROBLEM:** Labels for `PLACETEXT_LEFT` gadgets are drawn to the LEFT of the specified position, causing overlap with adjacent gadgets.

**WRONG WAY (causes label overlap):**
```c
// NEVER DO THIS - places gadget without accounting for label space
ng.ng_LeftEdge = base_x;  // WRONG! Label will overlap whatever is to the left
ng.ng_GadgetText = "Input:";
ng.ng_Flags = PLACETEXT_LEFT;
```

**CORRECT WAY:**
```c
// ALWAYS DO THIS - calculate label width and adjust position
STRPTR label = "Input:";
UWORD label_width = strlen(label) * font_width;
UWORD label_spacing = 4;  // Small gap between label and gadget
ng.ng_LeftEdge = base_x + label_width + label_spacing;  // Adjusted position
ng.ng_GadgetText = label;
ng.ng_Flags = PLACETEXT_LEFT;
```

**KEY RULES:**
- Calculate label width: `strlen(label) * font_width + spacing`
- Adjust gadget position: `gadget_x = base_x + label_width + spacing`
- Include label widths in window size calculations
- Test with different font sizes to verify spacing

### 3. Window Size Calculation

**THE PROBLEM:** Using calculated or requested dimensions instead of actual gadget dimensions results in windows that are too small or have poor spacing.

**WRONG WAY (causes truncation/overlap):**
```c
// NEVER DO THIS - using requested/calculated values
window_width = listview_requested_width + other_stuff;
window_height = listview_requested_height + other_stuff;
```

**CORRECT WAY:**
```c
// ALWAYS DO THIS - use actual gadget dimensions after creation
UWORD actual_listview_height = listview_gadget->Height;  // ACTUAL height
UWORD string_label_width = strlen("Input:") * font_width + 4;  // Include label
window_width = listview_gadget->Width + gap + string_label_width + 
               string_gadget->Width + margins + padding;
window_height = top_margin + font_height + actual_listview_height + 
                spacing + button_height + bottom_margin;
```

**KEY RULES:**
- Calculate size AFTER all gadgets are created
- Use actual gadget dimensions (`gadget->Width`, `gadget->Height`)
- Include ALL label widths for `PLACETEXT_LEFT` gadgets
- Add appropriate margins, spacing, and borders
- Test with different fonts to ensure proper scaling

### 4. Gadget Creation Order

**THE PROBLEM:** Creating gadgets in the wrong order prevents you from using actual dimensions for positioning calculations.

**CORRECT ORDER:**
1. Create ListView gadgets first (to discover actual height)
2. Create other gadgets based on ListView actual dimensions
3. Calculate window size using all actual gadget dimensions
4. Open window with calculated size

### 5. Reliable Gadget Geometry Access

**THE PROBLEM:** Using `GT_GetGadgetAttrs()` for basic geometry can return 0 or incorrect values.

**RELIABLE METHOD:**
```c
// Use direct gadget structure access for position/size
UWORD x = gadget->LeftEdge;
UWORD y = gadget->TopEdge;
UWORD w = gadget->Width;
UWORD h = gadget->Height;
```

**UNRELIABLE METHOD:**
```c
// DON'T use GT_GetGadgetAttrs for basic geometry
GT_GetGadgetAttrs(gadget, window, NULL, GA_Width, &w, TAG_END);  // Can fail!
```

## Debugging Layout Issues

Always use the `debug_print_gadget_positions()` function to verify:
- ListView actual height matches your calculations
- String gadget labels don't overlap with adjacent gadgets  
- Button positioning accounts for actual ListView height
- Window size properly contains all gadgets with margins

## Testing Checklist

Before considering layout work complete:
- [ ] Test with Topaz 8 font (default)
- [ ] Test with Topaz 9 font (common alternative)  
- [ ] Test with larger fonts if available
- [ ] Verify no gadget overlap in any font configuration
- [ ] Check that window size accommodates all content
- [ ] Ensure labels are not truncated or overlapping
- [ ] Verify proper spacing between all elements

## Common AI Agent Mistakes

1. **Using calculated ListView height instead of actual height** - This is the #1 cause of button overlap
2. **Not accounting for PLACETEXT_LEFT label space** - This causes label overlap with adjacent gadgets
3. **Creating gadgets in wrong order** - ListView must be first to get actual dimensions
4. **Using GT_GetGadgetAttrs() for geometry** - Direct structure access is more reliable
5. **Forgetting label widths in window calculations** - Causes window to be too narrow
6. **Not testing with different fonts** - Layout may work with one font but fail with others

## Template Usage Pattern

When using this template:
1. Follow the documented gadget creation order exactly
2. Use the provided calculation patterns without modification
3. Always include debug output during development
4. Test thoroughly with multiple font configurations
5. Never "optimize" the layout calculations - they exist for good reasons

Remember: These patterns exist because they solve real problems that occur in Amiga GadTools programming. Deviating from them will likely reintroduce the same bugs they were designed to prevent.
