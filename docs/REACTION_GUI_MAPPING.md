# iTidy ReAction GUI Mapping Guide

This document maps the old GadTools-based `main_window.c` controls to the new ReAction-based GUI exported from the Amiga GUI creation tool (ReBuild/similar).

---

## Quick Reference: Gadget ID Mapping

| Purpose | Old GadTools ID | New ReAction Index | New ReAction ID | Notes |
|---------|-----------------|-------------------|-----------------|-------|
| Folder Path | `GID_FOLDER_PATH` (1) | `folder_name` | `folder_name_id` (52) | GetFile gadget replaces string gadget |
| Browse Button | `GID_BROWSE` (2) | *(integrated)* | *(integrated)* | Built into GetFile gadget |
| Order Cycle | `GID_ORDER` (6) | `order_selection` | `order_selection_id` (35) | Chooser replaces Cycle |
| Sort By Cycle | `GID_SORTBY` (7) | `order_by_selection` | `order_by_selection_id` (38) | Chooser replaces Cycle |
| Recursive Checkbox | `GID_RECURSIVE` (10) | `cleanup_subfolders` | `cleanup_subfolders_id` (36) | Same function |
| Backup Checkbox | `GID_BACKUP` (11) | `checkbox_39` | `checkbox_39_id` (39) | **Rename needed** |
| Advanced Button | `GID_ADVANCED` (14) | `advanced_button` | `advanced_button_id` (46) | Same function |
| Apply/Start Button | `GID_APPLY` (15) | `start_button` | `start_button_id` (50) | "Apply" → "Start" |
| Cancel/Exit Button | `GID_CANCEL` (16) | `exit_button` | `exit_button_id` (51) | "Cancel" → "Exit" |
| Restore Button | `GID_RESTORE` (17) | `restore_backups_button` | `restore_backups_button_id` (48) | Same function |
| View Tool Cache | `GID_VIEW_TOOL_CACHE` (18) | `default_tools_button` | `default_tools_button_id` (47) | "Fix default tools..." |
| Window Position | `GID_WINDOW_POSITION` (21) | `position_selector` | `position_selector_id` (37) | Chooser replaces Cycle |
| Window Pos Help | `GID_WINDOW_POSITION_HELP` (22) | `button_40` | `button_40_id` (40) | "?" help button **Rename needed** |

---

## ReAction GUI Structure (from ReBuild Export)

### Exported Enums

```c
// Array indices for gadget pointers (main_gadgets[index])
enum main_window_idx { 
    master_layout,           // 0 - Root vertical layout
    folder_layout,           // 1 - Folder selection group
    folder_name,             // 2 - GetFile gadget
    itidy_options,           // 3 - Options group (horizontal)
    left_column,             // 4 - Left options column
    order_selection,         // 5 - Order chooser
    cleanup_subfolders,      // 6 - Recursive checkbox
    position_selector,       // 7 - Window position chooser
    right_column,            // 8 - Right options column
    order_by_selection,      // 9 - Sort by chooser
    checkbox_39,             // 10 - Backup checkbox (needs rename)
    button_40,               // 11 - Help "?" button (needs rename)
    tools_layout,            // 12 - Tools button group
    advanced_button,         // 13 - Advanced button
    default_tools_button,    // 14 - Default tools button
    restore_backups_button,  // 15 - Restore backups button
    main_buttons_layout,     // 16 - Bottom buttons group
    start_button,            // 17 - Start/Apply button
    exit_button              // 18 - Exit/Cancel button
};

// GA_ID values for event handling (result & WMHI_GADGETMASK)
enum main_window_id { 
    master_layout_id = 6,
    folder_layout_id = 28,
    folder_name_id = 52,
    itidy_options_id = 26,
    left_column_id = 33,
    order_selection_id = 35,
    cleanup_subfolders_id = 36,
    position_selector_id = 37,
    right_column_id = 34,
    order_by_selection_id = 38,
    checkbox_39_id = 39,           // Rename to backup_checkbox_id
    button_40_id = 40,             // Rename to help_button_id  
    tools_layout_id = 45,
    advanced_button_id = 46,
    default_tools_button_id = 47,
    restore_backups_button_id = 48,
    main_buttons_layout_id = 49,
    start_button_id = 50,
    exit_button_id = 51
};
```

---

## Integration Strategy

### Step 1: Create New Header File

Create `main_window_reaction.h` with properly named constants:

```c
#ifndef ITIDY_MAIN_WINDOW_REACTION_H
#define ITIDY_MAIN_WINDOW_REACTION_H

/*------------------------------------------------------------------------*/
/* ReAction Gadget IDs (from GUI tool export)                             */
/* These IDs come from ReBuild/GUI tool and should not be changed         */
/*------------------------------------------------------------------------*/

/* Array indices for gadget pointers */
typedef enum {
    ITIDY_GAD_IDX_MASTER_LAYOUT = 0,
    ITIDY_GAD_IDX_FOLDER_LAYOUT,
    ITIDY_GAD_IDX_FOLDER_GETFILE,        // Was: folder_name
    ITIDY_GAD_IDX_OPTIONS_LAYOUT,        // Was: itidy_options
    ITIDY_GAD_IDX_LEFT_COLUMN,
    ITIDY_GAD_IDX_ORDER_CHOOSER,         // Was: order_selection
    ITIDY_GAD_IDX_RECURSIVE_CHECKBOX,    // Was: cleanup_subfolders
    ITIDY_GAD_IDX_POSITION_CHOOSER,      // Was: position_selector
    ITIDY_GAD_IDX_RIGHT_COLUMN,
    ITIDY_GAD_IDX_SORTBY_CHOOSER,        // Was: order_by_selection
    ITIDY_GAD_IDX_BACKUP_CHECKBOX,       // Was: checkbox_39
    ITIDY_GAD_IDX_HELP_BUTTON,           // Was: button_40
    ITIDY_GAD_IDX_TOOLS_LAYOUT,
    ITIDY_GAD_IDX_ADVANCED_BUTTON,
    ITIDY_GAD_IDX_DEFAULT_TOOLS_BUTTON,
    ITIDY_GAD_IDX_RESTORE_BUTTON,
    ITIDY_GAD_IDX_BUTTONS_LAYOUT,
    ITIDY_GAD_IDX_START_BUTTON,          // Was: start_button (Apply)
    ITIDY_GAD_IDX_EXIT_BUTTON,           // Was: exit_button (Cancel)
    ITIDY_GAD_IDX_COUNT                  // Total count
} iTidyGadgetIndex;

/* GA_ID values for WMHI_GADGETUP events */
/* IMPORTANT: These MUST match the exported values from GUI tool */
#define ITIDY_GAID_FOLDER_GETFILE       52
#define ITIDY_GAID_ORDER_CHOOSER        35
#define ITIDY_GAID_RECURSIVE_CHECKBOX   36
#define ITIDY_GAID_POSITION_CHOOSER     37
#define ITIDY_GAID_SORTBY_CHOOSER       38
#define ITIDY_GAID_BACKUP_CHECKBOX      39
#define ITIDY_GAID_HELP_BUTTON          40
#define ITIDY_GAID_ADVANCED_BUTTON      46
#define ITIDY_GAID_DEFAULT_TOOLS_BUTTON 47
#define ITIDY_GAID_RESTORE_BUTTON       48
#define ITIDY_GAID_START_BUTTON         50
#define ITIDY_GAID_EXIT_BUTTON          51

/* Backward compatibility mapping (old GadTools IDs → new ReAction IDs) */
#define GID_FOLDER_PATH         ITIDY_GAID_FOLDER_GETFILE
#define GID_BROWSE              ITIDY_GAID_FOLDER_GETFILE  /* Integrated into GetFile */
#define GID_ORDER               ITIDY_GAID_ORDER_CHOOSER
#define GID_SORTBY              ITIDY_GAID_SORTBY_CHOOSER
#define GID_RECURSIVE           ITIDY_GAID_RECURSIVE_CHECKBOX
#define GID_BACKUP              ITIDY_GAID_BACKUP_CHECKBOX
#define GID_ADVANCED            ITIDY_GAID_ADVANCED_BUTTON
#define GID_APPLY               ITIDY_GAID_START_BUTTON
#define GID_CANCEL              ITIDY_GAID_EXIT_BUTTON
#define GID_RESTORE             ITIDY_GAID_RESTORE_BUTTON
#define GID_VIEW_TOOL_CACHE     ITIDY_GAID_DEFAULT_TOOLS_BUTTON
#define GID_WINDOW_POSITION     ITIDY_GAID_POSITION_CHOOSER
#define GID_WINDOW_POSITION_HELP ITIDY_GAID_HELP_BUTTON

#endif /* ITIDY_MAIN_WINDOW_REACTION_H */
```

### Step 2: Event Handler Mapping

The event handling pattern changes from GadTools to ReAction:

**Old GadTools Pattern:**
```c
case IDCMP_GADGETUP:
    switch (gad->GadgetID)
    {
        case GID_APPLY:
            // Handle apply
            break;
    }
```

**New ReAction Pattern:**
```c
while ((result = RA_HandleInput(window_obj, &code)) != WMHI_LASTMSG)
{
    switch (result & WMHI_CLASSMASK)
    {
        case WMHI_GADGETUP:
            switch (result & WMHI_GADGETMASK)
            {
                case ITIDY_GAID_START_BUTTON:  // Was GID_APPLY
                    // Handle apply
                    break;
            }
            break;
    }
}
```

---

## Importing Updated GUI from Amiga Tool

When you make changes in the GUI creation tool on the Amiga and export new code, follow this process:

### Step 1: Export New Code

Export the updated GUI to a file (e.g., `main_window_export.c`).

### Step 2: Extract Key Sections

The exported file contains these sections you need to extract:

1. **Enum definitions** - Update if gadgets added/removed
2. **Chooser label arrays** - Copy string arrays
3. **Chooser label creation** - The `ChooserLabelsA()` calls
4. **Window object creation** - The big `NewObject(WINDOW_GetClass()...` block
5. **Cleanup code** - The `FreeChooserLabels()` calls

### Step 3: Mapping Script

Create `scripts/update_reaction_gui.py` (run on PC) to automate extraction:

```python
#!/usr/bin/env python3
"""
Extract ReAction GUI definitions from ReBuild export and generate
iTidy-compatible header and source sections.

Usage: python update_reaction_gui.py Tests/ReActon/testcode.c
"""

import re
import sys

def extract_enums(content):
    """Extract enum definitions for gadget indices and IDs"""
    # Find enum main_window_idx
    idx_match = re.search(r'enum\s+main_window_idx\s*\{([^}]+)\}', content)
    # Find enum main_window_id  
    id_match = re.search(r'enum\s+main_window_id\s*\{([^}]+)\}', content)
    return idx_match.group(1) if idx_match else None, id_match.group(1) if id_match else None

def extract_label_arrays(content):
    """Extract UBYTE *labels[] string arrays"""
    pattern = r'UBYTE\s+\*(\w+)\[\]\s*=\s*\{([^}]+)\}'
    return re.findall(pattern, content)

def extract_window_object(content):
    """Extract the NewObject(WINDOW_GetClass()...) block"""
    # Find start
    start = content.find('window_object = NewObject( WINDOW_GetClass()')
    if start == -1:
        return None
    # Find matching TAG_END);
    depth = 0
    end = start
    for i, char in enumerate(content[start:]):
        if char == '(':
            depth += 1
        elif char == ')':
            depth -= 1
            if depth == 0:
                end = start + i + 1
                break
    return content[start:end]

def generate_id_mapping(id_enum_content):
    """Generate #define mapping from parsed enum"""
    # Parse "name_id = value" entries
    pattern = r'(\w+)\s*=\s*(\d+)'
    matches = re.findall(pattern, id_enum_content)
    
    # Known mappings to iTidy names
    name_map = {
        'folder_name_id': 'ITIDY_GAID_FOLDER_GETFILE',
        'order_selection_id': 'ITIDY_GAID_ORDER_CHOOSER',
        'cleanup_subfolders_id': 'ITIDY_GAID_RECURSIVE_CHECKBOX',
        'position_selector_id': 'ITIDY_GAID_POSITION_CHOOSER',
        'order_by_selection_id': 'ITIDY_GAID_SORTBY_CHOOSER',
        'checkbox_39_id': 'ITIDY_GAID_BACKUP_CHECKBOX',
        'button_40_id': 'ITIDY_GAID_HELP_BUTTON',
        'advanced_button_id': 'ITIDY_GAID_ADVANCED_BUTTON',
        'default_tools_button_id': 'ITIDY_GAID_DEFAULT_TOOLS_BUTTON',
        'restore_backups_button_id': 'ITIDY_GAID_RESTORE_BUTTON',
        'start_button_id': 'ITIDY_GAID_START_BUTTON',
        'exit_button_id': 'ITIDY_GAID_EXIT_BUTTON',
    }
    
    output = []
    for name, value in matches:
        if name in name_map:
            output.append(f"#define {name_map[name]:40} {value}")
    
    return '\n'.join(output)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python update_reaction_gui.py <exported_file.c>")
        sys.exit(1)
    
    with open(sys.argv[1], 'r') as f:
        content = f.read()
    
    idx_enum, id_enum = extract_enums(content)
    print("=== Gadget Index Enum ===")
    print(idx_enum)
    print("\n=== Gadget ID Enum ===")
    print(id_enum)
    print("\n=== ID Mapping ===")
    print(generate_id_mapping(id_enum))
    print("\n=== Label Arrays ===")
    for name, values in extract_label_arrays(content):
        print(f"{name}: {values}")
```

### Step 4: Manual Review Checklist

After importing new GUI code:

- [ ] Check all gadget IDs match expected values
- [ ] Verify label strings match preferences enum values
- [ ] Update `main_window_reaction.h` if IDs changed
- [ ] Test all button/checkbox/chooser events work
- [ ] Verify GetFile gadget returns proper paths

---

## Chooser Label Mapping

The Chooser gadgets use string arrays that must match the preference enums:

### Order Chooser (`order_selection`)

```c
// Old GadTools labels (order_labels[])
static STRPTR order_labels[] = {
    "Folders First",    // 0 = SORT_PRIORITY_FOLDERS_FIRST
    "Files First",      // 1 = SORT_PRIORITY_FILES_FIRST
    "Mixed",            // 2 = SORT_PRIORITY_MIXED
    "Grouped by Type",  // 3 = special case → blockGroupMode
    NULL
};

// New ReAction labels (labels35_str[])
UBYTE *labels35_str[] = { 
    "Folders first",    // 0
    "Files first",      // 1
    "Mixed",            // 2
    "Grouped by type",  // 3
    NULL 
};
```

### Sort By Chooser (`order_by_selection`)

```c
// Old GadTools labels (sortby_labels[])
static STRPTR sortby_labels[] = {
    "Name",   // 0 = SORT_BY_NAME
    "Type",   // 1 = SORT_BY_TYPE
    "Date",   // 2 = SORT_BY_DATE
    "Size",   // 3 = SORT_BY_SIZE
    NULL
};

// New ReAction labels (labels38_str[])
UBYTE *labels38_str[] = { 
    "Name",   // 0
    "Type",   // 1
    "Date",   // 2
    "Size",   // 3
    NULL 
};
```

### Window Position Chooser (`position_selector`)

```c
// Old GadTools labels (window_position_labels[])
static STRPTR window_position_labels[] = {
    "Center Screen",   // 0 = WINDOW_POS_CENTER
    "Keep Position",   // 1 = WINDOW_POS_KEEP
    "Near Parent",     // 2 = WINDOW_POS_NEAR_PARENT
    "No Change",       // 3 = WINDOW_POS_NO_CHANGE
    NULL
};

// New ReAction labels (labels37_str[])
UBYTE *labels37_str[] = { 
    "Center screen",   // 0
    "Keep position",   // 1
    "Near parent",     // 2
    "No change",       // 3
    NULL 
};
```

---

## Menu System

The menu system can remain largely unchanged. ReAction windows still use GadTools menus:

```c
// Same menu template works for both versions
static struct NewMenu main_window_menu_template[] = 
{
    { NM_TITLE, "Project",      NULL, 0, 0, NULL },
    { NM_ITEM,  "New",          "N",  0, 0, (APTR)MENU_PROJECT_NEW },
    { NM_ITEM,  "Open...",      "O",  0, 0, (APTR)MENU_PROJECT_OPEN },
    // ... rest unchanged ...
};

// Menu creation uses same GadTools functions
menu_strip = CreateMenusA(menuData, TAG_END);
LayoutMenus(menu_strip, gVisinfo, GTMN_NewLookMenus, TRUE, TAG_DONE);

// Menu handling in event loop
case WMHI_MENUPICK:
    // Same handling as GadTools version
    handle_main_window_menu_selection(Code, win_data);
    break;
```

---

## Data Structure Changes

### Old GadTools Structure

```c
struct iTidyMainWindow
{
    struct Screen *screen;
    struct Window *window;
    APTR visual_info;
    struct Gadget *glist;
    BOOL window_open;
    
    /* Individual gadget pointers */
    struct Gadget *folder_path;
    struct Gadget *browse_btn;
    struct Gadget *order_cycle;
    // ... many more ...
    
    /* State variables */
    WORD order_selected;
    // ...
};
```

### New ReAction Structure

```c
struct iTidyMainWindow
{
    struct Screen *screen;
    struct Window *window;
    Object *window_obj;              /* NEW: BOOPSI window object */
    APTR visual_info;                /* Still needed for menus */
    BOOL window_open;
    
    /* Gadget array (replaces individual pointers) */
    struct Gadget *gadgets[ITIDY_GAD_IDX_COUNT];
    
    /* Chooser label lists (must be freed on cleanup) */
    struct List *order_labels;
    struct List *sortby_labels;
    struct List *position_labels;
    
    /* State variables (unchanged) */
    WORD order_selected;
    WORD sortby_selected;
    BOOL recursive_subdirs;
    BOOL enable_backup;
    WORD window_position_selected;
    char folder_path_buffer[256];
    char last_save_path[256];
};
```

---

## Summary: Key Differences

| Aspect | GadTools | ReAction |
|--------|----------|----------|
| Gadget Creation | `CreateGadget()` chain | Nested `NewObject()` tree |
| Layout | Manual X/Y coordinates | Automatic via `layout.gadget` |
| Event Loop | `IDCMP_*` + `GT_GetIMsg()` | `RA_HandleInput()` + `WMHI_*` |
| Gadget Access | Individual `struct Gadget*` | Array indexed by enum |
| Cycle Gadget | `CYCLE_KIND` | `chooser.gadget` |
| String Gadget | `STRING_KIND` | `getfile.gadget` (with browse) |
| Checkbox | `CHECKBOX_KIND` | `checkbox.gadget` |
| Cleanup | `FreeGadgets(glist)` | `DisposeObject(window_obj)` |

---

## Files to Modify During Transition

1. **`src/GUI/main_window.h`** - Update structure, add ReAction includes
2. **`src/GUI/main_window.c`** - Replace gadget creation and event loop
3. **`src/main_gui.c`** - Add ReAction library open/close
4. **`Makefile`** - May need ReAction class paths

## Files to Keep as Reference

1. **`Tests/ReActon/testcode.c`** - Working ReAction GUI export
2. **`Tests/ReActon/ReBuildTutorial/`** - ReAction examples and documentation
