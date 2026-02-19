# QUICK REFERENCE FOR AI AGENTS

## ⚠️ MANDATORY PATTERNS - DO NOT BREAK! ⚠️

### ListView Height Discovery
```c
// 1. Create ListView first
ng.ng_Height = calculated_height;  // This is just a request
listview = CreateGadget(LISTVIEW_KIND, ...);

// 2. Get ACTUAL height (ListView snaps to complete rows)
UWORD actual_height = listview->Height;  // NOT ng.ng_Height!

// 3. Position other gadgets using actual height
button_y = listview->TopEdge + actual_height + spacing;
```

### PLACETEXT_LEFT Positioning
```c
// 1. Calculate label space
STRPTR label = "Input:";
UWORD label_width = strlen(label) * font_width;
UWORD spacing = 4;

// 2. Adjust gadget position for label
ng.ng_LeftEdge = base_x + label_width + spacing;  // NOT base_x!
ng.ng_GadgetText = label;
ng.ng_Flags = PLACETEXT_LEFT;
```

### Window Size Calculation
```c
// 1. Use ACTUAL gadget dimensions after creation
UWORD actual_listview_height = listview_gadget->Height;
UWORD string_label_width = strlen("Input:") * font_width + 4;

// 2. Include ALL components in size calculation
window_width = listview_width + gap + string_label_width + 
               string_width + margins + padding;
window_height = top_margin + label_height + actual_listview_height + 
                spacing + button_height + bottom_margin;
```

### Required Order
1. Create ListView FIRST
2. Get actual ListView height
3. Position other gadgets based on actual dimensions
4. Calculate window size using actual dimensions
5. Open window

### Reliable Geometry Access
```c
// ✅ CORRECT - Direct structure access
x = gadget->LeftEdge;
y = gadget->TopEdge;
w = gadget->Width;
h = gadget->Height;

// ❌ WRONG - GT_GetGadgetAttrs unreliable for basic geometry
GT_GetGadgetAttrs(gadget, window, NULL, GA_Width, &w, TAG_END);
```

### Debug Pattern
```c
// Always use debug output to verify layout
debug_print_gadget_positions(data);
printf("DEBUG: ListView requested: %d, actual: %d\n", 
       requested_height, gadget->Height);
```

## Common Mistakes That Break Layout

1. **Using calculated ListView height** → Button overlap
2. **Not accounting for PLACETEXT_LEFT labels** → Label overlap  
3. **Wrong gadget creation order** → Can't get actual dimensions
4. **Using GT_GetGadgetAttrs for geometry** → Unreliable results
5. **Forgetting label widths in window size** → Window too narrow
6. **Not testing different fonts** → Layout breaks with larger fonts

## Emergency Debugging

If layout is broken:
1. Add `debug_print_gadget_positions(data)` after window creation
2. Check ListView actual vs requested height
3. Verify gadget positions don't overlap
4. Ensure window size contains all gadgets + margins
5. Test with Topaz 8 and Topaz 9 fonts

Remember: These patterns exist because they solve real problems. Don't "optimize" them!
