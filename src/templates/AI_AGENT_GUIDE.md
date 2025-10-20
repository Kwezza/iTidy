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

### Get Gadget Values
```c
case YOUR_STRING_ID:
    STRPTR string_value;
    GT_GetGadgetAttrs(data->your_string, data->window, NULL,
                      GTST_String, &string_value, TAG_END);
    // Use string_value
    break;

case YOUR_CHECKBOX_ID:
    BOOL checked;
    GT_GetGadgetAttrs(data->your_checkbox, data->window, NULL,
                      GTCB_Checked, &checked, TAG_END);
    // Use checked state
    break;
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
