# AI Agent Template Usage Guide

## 🤖 Quick Start for AI Agents

This project provides two specialized templates designed for AI agents (like GitHub Copilot) to quickly create Amiga applications:

1. **Dynamic Window Template** - Creates font-aware windows with automatic sizing
2. **NewLook Menu Template** - Creates Workbench 3.x style menus with proper NewLook appearance

Both templates are AI-friendly with clear modification instructions and template usage guides.

## 📋 Template Modification Checklists

## Dynamic Window Template

### 1. Rename and Customize Constants
- [ ] Change `TEMPLATE_WINDOW_TITLE` to your window title
- [ ] Update `TEMPLATE_BUTTON_TEXT_CHARS` for button sizing
- [ ] Modify `TEMPLATE_LISTVIEW_LINES` for ListView height
- [ ] Adjust `TEMPLATE_SPACE_X/Y` for gadget spacing

### 2. Define Your Gadget IDs
- [ ] Replace `enum template_gadget_ids` with your specific IDs
- [ ] Use meaningful names like `CONFIG_BUTTON_SAVE`, `FILE_LIST_MAIN`, etc.
- [ ] Keep the first ID >= 1000 to avoid conflicts

### 3. Update Data Structure
- [ ] Rename `TemplateWindowData` to your specific name
- [ ] Replace gadget pointers with your actual gadgets
- [ ] Add application-specific data fields
- [ ] Update the `INIT_TEMPLATE_WINDOW_DATA` macro

### 4. Modify Gadget Creation
- [ ] Edit `create_template_gadgets()` function
- [ ] Use the provided gadget patterns for common gadgets
- [ ] Update positioning with `current_x`, `current_y` variables
- [ ] Call `update_window_max_dimensions()` for each gadget

### 5. Implement Event Handling
- [ ] Update `handle_template_gadget_event()` function
- [ ] Add cases for your specific gadget IDs
- [ ] Implement your gadget response logic
- [ ] Use `GT_GetGadgetAttrs()` to read gadget values

## 🎯 Common Gadget Patterns

### Button
```c
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = font_dims->button_width;
ng.ng_Height = font_dims->button_height;
ng.ng_GadgetText = "Your Button";
ng.ng_GadgetID = YOUR_BUTTON_ID;
ng.ng_Flags = PLACETEXT_IN;
data->your_button = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
```

### String Input
```c
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = font_dims->button_width;
ng.ng_Height = font_dims->string_height;
ng.ng_GadgetText = "Label:";
ng.ng_GadgetID = YOUR_STRING_ID;
ng.ng_Flags = PLACETEXT_LEFT;
data->your_string = gad = CreateGadget(STRING_KIND, gad, &ng,
                                      GTST_MaxChars, 64,
                                      GTST_String, "",
                                      TAG_END);
```

### Checkbox
```c
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = font_dims->button_height;
ng.ng_Height = font_dims->button_height;
ng.ng_GadgetText = "Option Name";
ng.ng_GadgetID = YOUR_CHECKBOX_ID;
ng.ng_Flags = PLACETEXT_RIGHT;
data->your_checkbox = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
                                        GTCB_Checked, FALSE,
                                        TAG_END);
```

### ListView
```c
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = font_dims->button_width * 2;
ng.ng_Height = font_dims->listview_height;
ng.ng_GadgetText = "List Title";
ng.ng_GadgetID = YOUR_LISTVIEW_ID;
ng.ng_Flags = PLACETEXT_ABOVE;
data->your_listview = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
                                        GTLV_Labels, &data->your_list,
                                        GTLV_ShowSelected, NULL,
                                        TAG_END);
```

## 📐 Layout System

### Positioning Variables
- `current_x` - Current X position for gadget placement
- `current_y` - Current Y position for gadget placement
- `column_2_x` - X position for second column
- `font_dims->window_left_edge` - Left edge starting position
- `font_dims->window_top_edge` - Top edge starting position

### Spacing Constants
- `TEMPLATE_SPACE_X` - Horizontal spacing between gadgets
- `TEMPLATE_SPACE_Y` - Vertical spacing between gadgets
- `TEMPLATE_GAP_BETWEEN_COLUMNS` - Gap between columns

### Helper Functions
- `calculate_next_column_x()` - Calculate next column position
- `center_gadgets_horizontally()` - Center gadgets in area
- `calculate_button_row_width()` - Calculate button row width
- `update_window_max_dimensions()` - Track window size requirements

## 🎨 Window Sizing

The template automatically calculates window size based on:
1. Font dimensions from the Workbench screen
2. Gadget positions and sizes
3. Proper spacing and borders

Always call `update_window_max_dimensions()` after creating each gadget to ensure proper window sizing.

## 📝 Event Handling Patterns

### ⚠️ CRITICAL: Cycle Gadget Race Condition Issue

#### The Problem
When handling cycle gadget events, using `GT_GetGadgetAttrs()` to read the gadget's current selection can return **stale data** that doesn't reflect the user's actual selection. This creates a race condition where:
- The GUI visually shows the user's new selection
- But `GT_GetGadgetAttrs()` returns the *previous* selection
- This causes the application to behave incorrectly despite the GUI looking correct

#### Example of the Bug
```c
/* ❌ WRONG WAY - Returns stale data! */
case GADGET_ID_SORT_PRIORITY:
{
    ULONG selected_priority = 0;
    
    /* This reads the OLD value, not what the user just selected! */
    GT_GetGadgetAttrs(data->cycle_sort_priority, data->window, NULL,
                      GTCY_Active, &selected_priority,
                      TAG_END);
    
    /* User sees "Mixed" in GUI, but selected_priority = 0 (Folders First)
       Application behaves incorrectly! */
    apply_sort_priority(selected_priority);
    break;
}
```

#### Why This Happens
1. User clicks on cycle gadget
2. Intuition sends `IDCMP_GADGETUP` message
3. Gadget visually updates on screen
4. Your event handler receives the message
5. **BUT**: The gadget's internal state hasn't finished updating yet
6. `GT_GetGadgetAttrs()` reads the gadget structure directly
7. It returns the value from *before* the click

This is a timing issue in the GadTools library where the visual update happens before the internal state update completes.

#### The Solution: Use IntuiMessage->Code
Instead of querying the gadget with `GT_GetGadgetAttrs()`, read the selection **directly from the IntuiMessage structure** that triggered the event:

```c
/* ✅ CORRECT WAY - Use msg->Code directly! */
case GADGET_ID_SORT_PRIORITY:
{
    /* The Code field contains the NEW selection index */
    ULONG selected_priority = msg->Code;
    
    /* This is always current and accurate */
    apply_sort_priority(selected_priority);
    break;
}
```

#### Complete Example
```c
BOOL handle_gadget_event(struct IntuiMessage *msg, YourWindowData *data)
{
    struct Gadget *gadget = (struct Gadget *)msg->IAddress;
    UWORD gadget_id = gadget->GadgetID;
    
    switch (gadget_id)
    {
        case GADGET_ID_SORT_BY:
        {
            /* For cycle gadgets, msg->Code has the selected index */
            UWORD sort_by = msg->Code;  /* 0 = Name, 1 = Date, 2 = Size, etc. */
            
            /* Update your application state */
            data->current_sort_by = sort_by;
            
            /* Apply the sorting */
            apply_sort_by(sort_by);
            break;
        }
        
        case GADGET_ID_SORT_PRIORITY:
        {
            /* Again, use msg->Code, not GT_GetGadgetAttrs() */
            UWORD priority = msg->Code;  /* 0 = Folders First, 1 = Files First, 2 = Mixed */
            
            data->current_priority = priority;
            apply_sort_priority(priority);
            break;
        }
        
        case GADGET_ID_LAYOUT_MODE:
        {
            /* Same pattern for all cycle gadgets */
            UWORD layout_mode = msg->Code;
            
            data->current_layout_mode = layout_mode;
            apply_layout_mode(layout_mode);
            break;
        }
    }
    
    return TRUE;
}
```

#### When GT_GetGadgetAttrs() IS Safe
For other gadget types, `GT_GetGadgetAttrs()` works fine because they don't have this race condition:

```c
/* These are safe to use GT_GetGadgetAttrs() */

case YOUR_STRING_ID:
    STRPTR string_value;
    GT_GetGadgetAttrs(data->your_string, data->window, NULL,
                      GTST_String, &string_value, TAG_END);
    // Use string_value - This is safe for string gadgets
    break;

case YOUR_CHECKBOX_ID:
    BOOL checked;
    GT_GetGadgetAttrs(data->your_checkbox, data->window, NULL,
                      GTCB_Checked, &checked, TAG_END);
    // Use checked state - This is safe for checkboxes
    break;

case YOUR_SLIDER_ID:
    LONG level;
    GT_GetGadgetAttrs(data->your_slider, data->window, NULL,
                      GTSL_Level, &level, TAG_END);
    // Use level - This is safe for sliders
    break;
```

#### Summary of the Rule
**For CYCLE gadgets:**
- ✅ **DO** use `msg->Code` to get the current selection
- ❌ **DON'T** use `GT_GetGadgetAttrs()` in the event handler
- ✅ **DO** use `GT_GetGadgetAttrs()` when querying outside of event handling (e.g., initialization, saving settings)

**For other gadget types (STRING, CHECKBOX, SLIDER, etc.):**
- ✅ `GT_GetGadgetAttrs()` works correctly
- ✅ You can use it safely in event handlers

#### Debugging the Race Condition
If you suspect this issue:
1. Add debug output showing both values:
   ```c
   ULONG attrs_value = 0;
   GT_GetGadgetAttrs(gadget, window, NULL, GTCY_Active, &attrs_value, TAG_END);
   printf("msg->Code: %lu, GT_GetGadgetAttrs: %lu\n", msg->Code, attrs_value);
   ```
2. If they differ, you have the race condition
3. Switch to using `msg->Code`

#### Real-World Impact
This bug caused iTidy to:
- Show "Mixed" priority in the GUI
- Actually apply "Folders First" sorting
- Confuse users who saw correct GUI but wrong behavior
- Required extensive debugging to identify

After switching to `msg->Code`, all cycle gadgets worked perfectly with zero GUI/behavior mismatches.

### ⚠️ CRITICAL: Checkbox Data Type Issue

#### The Problem
When reading checkbox gadget values with `GT_GetGadgetAttrs()`, passing a pointer to `BOOL` instead of `ULONG` causes incorrect values to be read. This is a **data type mismatch** issue specific to the Amiga GadTools API.

#### Why This Happens
- `GT_GetGadgetAttrs()` with `GTCB_Checked` expects a pointer to `ULONG` (32-bit)
- `BOOL` may be 16-bit or 8-bit depending on the compiler
- Passing `BOOL*` when `ULONG*` is expected causes:
  - Memory corruption or partial writes
  - Incorrect boolean values being read
  - Values always appearing as FALSE

#### Example of the Bug
```c
/* ❌ WRONG WAY - Type mismatch! */
struct WindowData {
    BOOL center_icons;  /* May be 16-bit */
    BOOL optimize_cols;
};

case GID_CENTER_ICONS:
    /* This corrupts memory or reads wrong value! */
    GT_GetGadgetAttrs(gad, window, NULL,
        GTCB_Checked, &win_data->center_icons,  /* BOOL* instead of ULONG* */
        TAG_END);
    break;

/* Result: Checkbox appears ticked in GUI, but value is always FALSE! */
```

#### The Solution: Use ULONG Temporary Variable
Always use a `ULONG` temporary variable when calling `GT_GetGadgetAttrs()` for checkboxes:

```c
/* ✅ CORRECT WAY - Use ULONG temporary! */
case GID_CENTER_ICONS:
    {
        ULONG checked = 0;  /* Correct 32-bit type */
        GT_GetGadgetAttrs(gad, window, NULL,
            GTCB_Checked, &checked,  /* Pass ULONG* */
            TAG_END);
        win_data->center_icons = (BOOL)checked;  /* Safe cast to BOOL */
        printf("Center icons: %s\n", win_data->center_icons ? "ON" : "OFF");
    }
    break;

case GID_OPTIMIZE_COLS:
    {
        ULONG checked = 0;
        GT_GetGadgetAttrs(gad, window, NULL,
            GTCB_Checked, &checked,
            TAG_END);
        win_data->optimize_cols = (BOOL)checked;
        printf("Optimize: %s\n", win_data->optimize_cols ? "ON" : "OFF");
    }
    break;

case GID_RECURSIVE:
    {
        ULONG checked = 0;
        GT_GetGadgetAttrs(gad, window, NULL,
            GTCB_Checked, &checked,
            TAG_END);
        win_data->recursive = (BOOL)checked;
        printf("Recursive: %s\n", win_data->recursive ? "ON" : "OFF");
    }
    break;
```

#### Why This Works
1. `ULONG` is guaranteed to be 32-bit (same as what GadTools expects)
2. The API correctly writes the checkbox state to the ULONG variable
3. The cast to `BOOL` is safe and preserves the boolean value
4. Block scope `{ }` keeps the temporary variable local

#### Symptoms of This Bug
- ✗ Checkbox visually ticks/unticks correctly in GUI
- ✗ But application always sees value as FALSE
- ✗ Feature controlled by checkbox never activates
- ✗ No compiler warnings or errors
- ✗ Very difficult to debug without knowing about the type mismatch

#### Complete Checkbox Pattern
```c
struct WindowData {
    /* Application data - can use BOOL for storage */
    BOOL center_icons;
    BOOL optimize_cols;
    BOOL recursive;
    BOOL enable_backup;
};

/* Event handler - always use ULONG temporary */
BOOL handle_gadget_event(struct IntuiMessage *msg, struct WindowData *data)
{
    struct Gadget *gad = (struct Gadget *)msg->IAddress;
    
    switch (gad->GadgetID)
    {
        case GID_CENTER_ICONS:
        {
            ULONG checked = 0;
            GT_GetGadgetAttrs(gad, data->window, NULL,
                GTCB_Checked, &checked, TAG_END);
            data->center_icons = (BOOL)checked;
            break;
        }
        
        case GID_OPTIMIZE_COLS:
        {
            ULONG checked = 0;
            GT_GetGadgetAttrs(gad, data->window, NULL,
                GTCB_Checked, &checked, TAG_END);
            data->optimize_cols = (BOOL)checked;
            break;
        }
    }
    
    return TRUE;
}
```

#### When Setting Checkbox Values
When *setting* checkbox values with `GT_SetGadgetAttrs()`, you can pass `BOOL` directly as the value (not a pointer):

```c
/* ✅ This is fine - value is passed directly, not via pointer */
GT_SetGadgetAttrs(data->checkbox_gad, window, NULL,
    GTCB_Checked, (ULONG)mybool,  /* Cast BOOL to ULONG */
    TAG_END);

/* Or use literal */
GT_SetGadgetAttrs(data->checkbox_gad, window, NULL,
    GTCB_Checked, TRUE,
    TAG_END);
```

#### Summary of the Rule
**For CHECKBOX gadgets with GT_GetGadgetAttrs():**
- ✅ **DO** use `ULONG` temporary variable
- ✅ **DO** pass `ULONG*` to `GT_GetGadgetAttrs()`
- ✅ **DO** cast result to `BOOL` for storage
- ❌ **DON'T** pass `BOOL*` directly to `GT_GetGadgetAttrs()`

**For other gadget types:**
- STRING: Use `STRPTR` (this works correctly)
- SLIDER: Use `LONG` or `ULONG` (this works correctly)
- CYCLE: Use `msg->Code` (see cycle gadget section above)

#### Real-World Impact
This bug in iTidy caused:
- Column centering feature completely non-functional
- Users confused because checkbox appeared to work visually
- Feature activation logic never executed
- Required deep debugging to identify root cause
- Fixed by adding ULONG temporary variables

After fix:
- ✅ All checkboxes work correctly
- ✅ Features activate/deactivate as expected
- ✅ Visual state matches actual state
- ✅ No memory corruption

### Get Gadget Values (Non-Cycle, Non-Checkbox Gadgets)
```c
/* String gadgets - Safe to use STRPTR */
STRPTR string_value;
GT_GetGadgetAttrs(data->your_string, data->window, NULL,
                  GTST_String, &string_value, TAG_END);

/* Slider gadgets - Safe to use LONG */
LONG level;
GT_GetGadgetAttrs(data->your_slider, data->window, NULL,
                  GTSL_Level, &level, TAG_END);

/* Integer gadgets - Safe to use LONG */
LONG number;
GT_GetGadgetAttrs(data->your_integer, data->window, NULL,
                  GTIN_Number, &number, TAG_END);
```

### Update Gadget Values
```c
GT_SetGadgetAttrs(data->your_string, data->window, NULL,
                  GTST_String, "New Value", TAG_END);

GT_SetGadgetAttrs(data->your_checkbox, data->window, NULL,
                  GTCB_Checked, TRUE, TAG_END);
```

## 🔧 Build System

The project includes a unified build system for both templates:

### Build Commands
- `make all` - Build both templates
- `make window-template` - Build dynamic window template only
- `make menu-template` - Build NewLook menu template only
- `make clean` - Clean all build files
- `make clean-window` - Clean window template files only
- `make clean-menu` - Clean menu template files only
- `make test` - Build and run test suite
- `make help` - Show available targets

### Native Compilation
- `make native-window` - Build window template natively
- `make native-menu` - Build menu template natively

## 📚 Files to Modify

### Dynamic Window Template
1. **amiga_window_template.h** - Data structures and prototypes
2. **amiga_window_template.c** - Main template implementation
3. **examples/your_example.c** - Your specific implementation
4. **Makefile** - Add your build targets (if needed)

### NewLook Menu Template
1. **amiga_window_menus_template.c** - Complete standalone template
   - Copy to your project name (e.g., `my_app.c`)
   - Modify the template sections as documented
   - No additional files needed - it's completely self-contained

## ✅ Best Practices for AI Agents

### Dynamic Window Template
1. **Always preserve the font-based sizing system**
2. **Use the provided positioning helpers**
3. **Call `update_window_max_dimensions()` for each gadget**
4. **Keep the basic window management structure intact**
5. **Follow the existing code formatting and commenting style**

### NewLook Menu Template
1. **Copy the template to a new filename before modifying**
2. **Only modify the clearly marked template sections**
3. **Keep the `setup_newlook_menus()` and `cleanup_newlook_menus()` functions unchanged**
4. **Use sequential menu IDs starting from defined constants**
5. **Always end menu structure with `NM_END`**

### Both Templates
1. **Test with build commands after modifications**
2. **Update the changelog when making significant changes**
3. **Follow AmigaOS 3.x coding standards**
4. **Use proper error checking for all system calls**

## 🎯 Common Use Cases

### Dynamic Window Template
- **Configuration Windows**: Use checkboxes, cycles, and buttons
- **File Dialogs**: Use ListView with string input and buttons
- **Settings Panels**: Use multiple gadget types with validation
- **Data Entry Forms**: Use string gadgets with OK/Cancel buttons
- **List Managers**: Use ListView with Add/Remove/Edit buttons

### NewLook Menu Template
- **Text Editors**: File, Edit, Search, Help menus
- **Graphics Applications**: File, Edit, View, Tools, Effects menus
- **Utilities**: File, Options, Tools, Help menus
- **Games**: Game, Options, High Scores, Help menus
- **System Tools**: File, Edit, View, Tools, Preferences menus

Both templates provide solid foundations that AI agents can quickly adapt for any Amiga application requirement while maintaining proper AmigaOS 3.x compatibility and visual consistency.

## ⚠️ Important ListView Height Issue

### The ListView Height Snapping Problem
ListView gadgets in AmigaOS automatically adjust their height to show complete rows. This means the actual height may differ from what you specify in `ng.ng_Height`. If you don't account for this, gadgets positioned below the ListView may overlap with it.

### Solution Pattern
```c
/* 1. Create ListView with desired height */
ng.ng_Height = font_dims->listview_height;
data->your_listview = gad = CreateGadget(LISTVIEW_KIND, gad, &ng, ...);

/* 2. Get actual height after creation */
UWORD actual_height = gad->Height;

/* 3. Store bottom position for layout calculations */
UWORD listview_bottom_y = ng.ng_TopEdge + actual_height;

/* 4. Position gadgets below using actual bottom position */
UWORD min_next_y = listview_bottom_y + TEMPLATE_SPACE_Y;
if (current_y < min_next_y) {
    current_y = min_next_y;
}

/* 5. Recalculate window dimensions with actual height */
update_window_max_dimensions(*window_max_width, *window_max_height,
                            ng.ng_LeftEdge + ng.ng_Width,
                            ng.ng_TopEdge + actual_height,
                            window_max_width, window_max_height);
```

### Why This Happens
ListView gadgets calculate their height based on:
- Font height
- Number of visible rows
- Internal padding and borders
- Row spacing

The gadget ensures it can display complete rows, so it rounds the height to the nearest row boundary.

## NewLook Menu Template

### 1. Customize Window Settings
- [ ] Change `WINDOW_TITLE` to your application title
- [ ] Update `WINDOW_WIDTH` and `WINDOW_HEIGHT` for your needs
- [ ] Adjust `WINDOW_LEFT` and `WINDOW_TOP` for positioning

### 2. Define Menu Item IDs
- [ ] Add your menu item constants after `MENU_ITEM_QUIT`
- [ ] Use sequential numbers for each menu item
- [ ] Keep descriptive names like `MENU_FILE_OPEN`, `MENU_EDIT_CUT`

### 3. Build Menu Structure
- [ ] Modify `menu_template[]` array to define your menus
- [ ] Use `NM_TITLE` for menu titles (File, Edit, etc.)
- [ ] Use `NM_ITEM` for menu items (Open, Save, etc.)
- [ ] Use `NM_BARLABEL` for separator bars
- [ ] Always end with `NM_END`

### 4. Implement Menu Actions
- [ ] Update `handle_menu_selection()` function
- [ ] Add case statements for your menu item IDs
- [ ] Implement the actual functionality for each menu item
- [ ] Return appropriate values (continue, quit, etc.)

### 5. Core Functions (Leave Unchanged)
- [ ] `setup_newlook_menus()` - Menu system setup
- [ ] `cleanup_newlook_menus()` - Resource cleanup
- [ ] `main()` - Main program loop and window handling

## 🍽️ NewLook Menu Template Patterns

### Menu Structure Example
```c
struct NewMenu menu_template[] = {
    /* File menu */
    { NM_TITLE, "File", NULL, 0, 0, NULL },
        { NM_ITEM, "New", "N", 0, 0, (APTR)MENU_FILE_NEW },
        { NM_ITEM, "Open...", "O", 0, 0, (APTR)MENU_FILE_OPEN },
        { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
        { NM_ITEM, "Quit", "Q", 0, 0, (APTR)MENU_ITEM_QUIT },
    
    /* Edit menu */
    { NM_TITLE, "Edit", NULL, 0, 0, NULL },
        { NM_ITEM, "Cut", "X", 0, 0, (APTR)MENU_EDIT_CUT },
        { NM_ITEM, "Copy", "C", 0, 0, (APTR)MENU_EDIT_COPY },
        { NM_ITEM, "Paste", "V", 0, 0, (APTR)MENU_EDIT_PASTE },
    
    { NM_END, NULL, NULL, 0, 0, NULL }
};
```

### Menu Handler Pattern
```c
static BOOL handle_menu_selection(UWORD menu_id, struct Window *window)
{
    switch (menu_id)
    {
        case MENU_FILE_NEW:
            /* Implement new file action */
            printf("New file selected\n");
            break;
            
        case MENU_FILE_OPEN:
            /* Implement open file action */
            printf("Open file selected\n");
            break;
            
        case MENU_EDIT_CUT:
            /* Implement cut action */
            printf("Cut selected\n");
            break;
            
        case MENU_ITEM_QUIT:
            printf("Quit selected\n");
            return FALSE; /* Signal to quit */
            
        default:
            printf("Unknown menu item: %d\n", menu_id);
            break;
    } /* switch */
    
    return TRUE; /* Continue running */
} /* handle_menu_selection */
```

### Menu Integration
The NewLook menu template provides:
- **Automatic NewLook Styling**: Menus automatically use Workbench 3.x appearance
- **Keyboard Shortcuts**: Support for Amiga+key combinations
- **Visual Info Integration**: Proper font and color handling
- **Event Loop Integration**: Seamless integration with window message loop
