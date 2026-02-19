# Amiga Resizable GadTools Window Template

## Overview

This template demonstrates best practices for creating dynamically resizable GadTools windows on Amiga Workbench 3.0/3.1. It provides a complete, working example that shows how to:

- Create a standard Workbench window with manual resize support
- Properly anchor gadgets to window edges (top-left, bottom, right)
- Implement flicker-free resizing without ghost gadgets or trails
- Handle all required IDCMP events correctly
- Log window and gadget geometry to console for debugging

## Critical Reference Documents

This template is based on two essential reference documents in `src/templates/`:

1. **Resizable GadTools Forms on Amiga Workbench 3.0.txt**
   - Ultimate source of truth for layout and resizing calculations
   - Explains inner client area computation, border formulas, anchoring patterns
   - Documents the "manual resize" approach (no GFLG_RELWIDTH/RELHEIGHT)

2. **Clearing Window Contents Before Gadget Re-Add in GadTools.txt**
   - Ultimate source of truth for clean redraws without artifacts
   - Explains proper use of EraseRect/RectFill to clear old gadget imagery
   - Documents IDCMP_SIZEVERIFY/NEWSIZE/REFRESHWINDOW handling

**If anything conflicts, these documents win.**

## Window Layout

The template window contains:

### Left Side
- **ListView**: Anchored top-left, stretches both horizontally and vertically
  - Fills all available space above bottom button row
  - Leaves room for right-hand column gadgets
  - Contains 19 test items for demonstration

### Bottom Row
- **OK Button**: Anchored bottom-left
- **Cancel Button**: Anchored bottom-left, positioned after OK button
  - Closes window and exits program

### Right-Hand Column
- **Input String Gadget**: Anchored to right side, near top
  - Label: "Input:"
  - Stays fixed distance from right edge when window resizes
- **Checkbox**: Anchored to right side, below input gadget
  - Label: "Enable option"
  - Stays fixed distance from right edge when window resizes

## Resize Behavior

### Window Properties
- **Refresh Mode**: Simple Refresh (WA_SimpleRefresh)
  - Application explicitly redraws on expose/resize
  - No backing store overhead
- **Size Gadget**: Standard Workbench size gadget (bottom-right corner)
- **Minimum Size**: 350×200 pixels (inner area)
- **Initial Size**: 500×300 pixels (inner area)

### Resize Strategy

The template uses the **"full rebuild"** approach recommended in the reference documents:

1. **IDCMP_SIZEVERIFY**:
   - Remove all gadgets from window BEFORE resize starts
   - Prevents flicker during drag operation
   - Reply immediately so Intuition can proceed

2. **IDCMP_NEWSIZE**:
   - Recompute inner client area from new window dimensions
   - Clear entire inner region with RectFill (removes old gadget imagery)
   - Destroy old gadgets completely
   - Create new gadgets with updated positions/sizes
   - Add gadgets back to window
   - Call GT_RefreshWindow() to draw all gadgets

3. **IDCMP_REFRESHWINDOW**:
   - Call GT_BeginRefresh()/GT_EndRefresh() to satisfy Intuition
   - Required for GadTools windows (cannot use WA_NoCareRefresh)

### Why No GFLG_RELWIDTH/RELHEIGHT?

GadTools **forbids** the use of GFLG_RELWIDTH, GFLG_RELHEIGHT, GFLG_RELRIGHT, and GFLG_RELBOTTOM flags on GadTools gadgets. These flags are only for raw Intuition gadgets.

Instead, this template implements **manual anchoring**:
- Store each gadget's distance from window edges
- Recompute positions/sizes when window changes
- Use the "rebuild" pattern to ensure clean redraws

## Console Logging

The template provides extensive console output for debugging and learning:

### At Window Open
- Screen font name and size
- Font metrics (width, height)
- Window outer size and position
- Border sizes (Left, Top, Right, Bottom)
- Inner client area dimensions
- All gadget positions and sizes

### On Every Resize
- New window outer size
- New inner client area size
- Confirmation of region clearing
- Updated gadget positions and sizes

### On User Interaction
- ListView: "ListView: Selected index N" + item text
- OK button: "OK button pressed"
- Cancel button: "Cancel button pressed - closing window"
- Input: "Input gadget changed: \"<text>\""
- Checkbox: "Checkbox enabled" or "Checkbox disabled"

## Building the Template

### From src/templates/ directory:

```bash
make clean
make
```

This builds the executable: `amiga_window_resize_template`

### Compiler Requirements:
- **VBCC** v0.9x with +aos68k target
- **Workbench 3.0+ SDK** (NDK 3.x headers)
- **C89** compliance (no C99-specific features used except in -c99 mode)

### Build Output:
- Executable: `amiga_window_resize_template` (68020 code with debug symbols)
- Object file: `amiga_window_resize_template.o`

## Running the Template

### From CLI (Shell):
```
amiga_window_resize_template
```
Output appears in the shell window.

### From Workbench:
Double-click the icon. The template automatically opens a console window for debug output.

### Testing Checklist:
1. Open the window and observe initial console output
2. Try resizing the window larger - ListView should expand
3. Try resizing smaller (but not below minimum)
4. Check that right-hand gadgets stay anchored to right edge
5. Check that bottom buttons stay anchored to bottom
6. Verify no "ghost" gadgets or trails remain after resize
7. Select items in the ListView - observe console messages
8. Type in the Input gadget - observe console messages
9. Toggle the checkbox - observe console messages
10. Click Cancel or close gadget - window exits cleanly

## Technical Details

### Inner Client Area Calculation

The template follows the standard Amiga pattern for non-GimmeZeroZero windows:

```c
inner_width = window->Width - window->BorderLeft - window->BorderRight;
inner_height = window->Height - window->BorderTop - window->BorderBottom;
```

Border sizes are read AFTER the window opens (they depend on screen font, title bar, gadgets).

### BorderTop Estimation

Before opening the window, we estimate BorderTop for initial sizing:

```c
BorderTop = screen->WBorTop + screen->Font->ta_YSize + 1
```

This formula accounts for the title bar height.

### Gadget Anchoring Pattern

Each gadget stores anchor information:
- `left_margin`: Distance from left edge of inner area
- `top_margin`: Distance from top edge
- `right_margin`: Distance from right edge
- `bottom_margin`: Distance from bottom edge
- `stretch_h`: TRUE if gadget stretches horizontally
- `stretch_v`: TRUE if gadget stretches vertically
- `anchor_right`: TRUE if anchored to right edge
- `anchor_bottom`: TRUE if anchored to bottom edge

On resize, gadget positions are recomputed:
```c
if (anchor_bottom)
    gadget->TopEdge = inner_height - bottom_margin;
else
    gadget->TopEdge = top_margin;

if (anchor_right)
    gadget->LeftEdge = inner_width - right_margin;
else
    gadget->LeftEdge = left_margin;

if (stretch_h)
    gadget->Width = inner_width - left_margin - right_margin;

if (stretch_v)
    gadget->Height = inner_height - top_margin - bottom_margin;
```

### Clearing Strategy

To prevent ghost gadgets, the template clears the entire inner region before redrawing:

```c
SetAPen(rp, 0);  /* Background pen */
RectFill(rp,
    border_left,
    border_top,
    window->Width - border_right - 1,
    window->Height - border_bottom - 1);
```

This ensures no old gadget imagery remains when gadgets are recreated in new positions.

## Code Structure

### Main Data Structure: `struct WindowData`
- Holds all Intuition/GadTools resources
- Individual gadget pointers for easy access
- ListView data and labels
- Font metrics
- Border sizes
- Current inner dimensions
- Anchor information for all gadgets

### Key Functions:

- `init_window_data()`: Lock screen, get visual info, initialize lists
- `create_gadgets()`: Create all GadTools gadgets with current dimensions
- `destroy_gadgets()`: Free all gadget resources
- `open_window()`: Open window with initial gadgets
- `compute_gadget_positions()`: Calculate anchor information
- `handle_resize()`: Full rebuild on IDCMP_NEWSIZE
- `clear_window_interior()`: Erase inner region with RectFill
- `event_loop()`: Handle all IDCMP events
- `log_window_geometry()`: Debug output for window size
- `log_gadget_geometry()`: Debug output for gadget positions

## Customization Guide

To adapt this template for your own resizable window:

1. **Modify Constants** (lines 45-70):
   - Change `WINDOW_TITLE`
   - Adjust margins and spacing
   - Set initial/minimum window dimensions
   - Customize right column width, button row height

2. **Define Your Gadget IDs** (enum starting line 75)

3. **Update Gadget Anchor Info** (struct starting line 80):
   - Add new anchor structures for your gadgets
   - Remove unused ones

4. **Modify `create_gadgets()`** (line 280):
   - Add/remove/modify gadget creation code
   - Set appropriate PLACETEXT flags for labels
   - Configure gadget-specific tags (GTST_MaxChars, GTCB_Checked, etc.)

5. **Update `compute_gadget_positions()`** (line 410):
   - Define anchor margins for each gadget
   - Specify which edges each gadget anchors to
   - Set stretch flags as appropriate

6. **Customize `handle_gadget_event()`** (line 790):
   - Add your application logic for each gadget

7. **Test thoroughly** with different window sizes and screen fonts

## Compatibility

- **Tested on**: Amiga Workbench 3.0, 3.1
- **CPU**: 68020+ (can be changed to 68000 in Makefile)
- **Memory**: Minimal (all allocations are gadget-related)
- **Screen**: Workbench public screen only

**Not compatible with**:
- AmigaOS 1.x (no GadTools)
- Custom screens without adaptation
- GimmeZeroZero mode (would need coordinate adjustments)

## Known Limitations

1. **No live resize**: On OS 3.0/3.1, resize is not live. Window contents update only when user releases size gadget.

2. **Format string warnings**: VBCC may warn about UWORD format specifiers - these are harmless.

3. **ListView state loss**: When gadgets are destroyed/recreated, ListView scroll position resets. Production code should save/restore with GT_GetGadgetAttrs/GT_SetGadgetAttrs.

4. **Font assumptions**: Code assumes roughly square characters for initial sizing. Proportional fonts may require adjustments.

## Further Reading

- **Amiga ROM Kernel Reference Manual: Libraries** - GadTools chapter
- **Amiga ROM Kernel Reference Manual: Intuition** - Window and gadget chapters
- Both reference documents in `src/templates/`
- `AI_AGENT_LAYOUT_GUIDE.md` - iTidy-specific layout patterns
- `AI_AGENT_GUIDE.md` - General Amiga development guidelines

## License

This template is part of the iTidy project. Use freely for learning and as a basis for your own Amiga applications.

## Credits

Based on patterns and best practices documented by:
- Commodore Amiga ROM Kernel Reference Manuals
- Experienced Amiga developers (Thomas Rapp, "broadblues", and forum contributors)
- Insights from Amiga developer communities (amigans.net, EAB, etc.)

Template created as a working reference for resizable GadTools windows on classic Amiga systems.
