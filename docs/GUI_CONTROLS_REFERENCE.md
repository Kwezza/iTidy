# iTidy GUI Controls Quick Reference

**Phase 2 Implementation Complete**  
**All 15 Controls Functional**

---

## 🎛️ Control Inventory

### 📁 Folder Selection
| Control | Type | ID | Default | Max |
|---------|------|-----|---------|-----|
| **Folder Path** | String | GID_FOLDER_PATH | "DH0:" | 255 chars |
| **Browse...** | Button | GID_BROWSE | - | - |

### 🎨 Layout Presets
| Control | Type | ID | Options | Default |
|---------|------|-----|---------|---------|
| **Preset** | Cycle | GID_PRESET | Classic, Compact, Modern, WHDLoad | Classic |

### 📐 Icon Layout
| Control | Type | ID | Options | Default |
|---------|------|-----|---------|---------|
| **Layout** | Cycle | GID_LAYOUT | Row, Column | Row |
| **Sort** | Cycle | GID_SORT | Horizontal, Vertical | Horizontal |
| **Order** | Cycle | GID_ORDER | Folders First, Files First, Mixed | Folders First |
| **Sort By** | Cycle | GID_SORTBY | Name, Type, Date, Size | Name |

### ✅ Options
| Control | Type | ID | Default | Enabled |
|---------|------|-----|---------|---------|
| **Center icons** | Checkbox | GID_CENTER_ICONS | OFF | ✅ |
| **Optimize columns** | Checkbox | GID_OPTIMIZE_COLS | ON | ✅ |
| **Recursive Subfolders** | Checkbox | GID_RECURSIVE | OFF | ✅ |
| **Backup (LHA)** | Checkbox | GID_BACKUP | OFF | ✅ |
| **Icon Upgrade** | Checkbox | GID_ICON_UPGRADE | OFF | ❌ (future) |

### 🔘 Action Buttons
| Control | Type | ID | Action | Enabled |
|---------|------|-----|--------|---------|
| **Advanced...** | Button | GID_ADVANCED | Open advanced window | ❌ (Phase 3) |
| **Apply** | Button | GID_APPLY | Process icons | ✅ |
| **Cancel** | Button | GID_CANCEL | Close window | ✅ |

---

## 📊 Setting Storage

### iTidyMainWindow Structure Members

```c
/* String Buffers */
char folder_path_buffer[256];        /* Folder path from string gadget */

/* Cycle Selections (WORD index values) */
WORD preset_selected;                /* 0=Classic, 1=Compact, 2=Modern, 3=WHDLoad */
WORD layout_selected;                /* 0=Row, 1=Column */
WORD sort_selected;                  /* 0=Horizontal, 1=Vertical */
WORD order_selected;                 /* 0=Folders First, 1=Files First, 2=Mixed */
WORD sortby_selected;                /* 0=Name, 1=Type, 2=Date, 3=Size */

/* Checkbox States (BOOL) */
BOOL center_icons;                   /* Center icons between grid lines */
BOOL optimize_columns;               /* Optimize column count automatically */
BOOL recursive_subdirs;              /* Process subdirectories recursively */
BOOL enable_backup;                  /* Create LHA backup before processing */
BOOL enable_icon_upgrade;            /* Upgrade icon format (future) */
```

---

## 🔄 Preset Configurations

### Classic Preset (Index 0)
```
Layout:  Row
Sort:    Horizontal
Order:   Folders First
Sort By: Name
Center:  OFF
Optimize: ON
```

### Compact Preset (Index 1)
```
Layout:  Column
Sort:    Vertical
Order:   Mixed
Sort By: Name
Center:  OFF
Optimize: ON
```

### Modern Preset (Index 2)
```
Layout:  Row
Sort:    Horizontal
Order:   Mixed
Sort By: Date
Center:  OFF
Optimize: ON
```

### WHDLoad Preset (Index 3)
```
Layout:  Column
Sort:    Vertical
Order:   Folders First
Sort By: Name
Center:  OFF
Optimize: ON
Recursive: ON (special for WHDLoad cleanup)
```

---

## 🎯 Event Handling

### Gadget Event Types

#### IDCMP_GADGETUP
Triggered when:
- User releases mouse button over gadget
- Cycle gadget selection changes
- Checkbox toggles
- Button clicked
- String gadget loses focus (Enter pressed)

#### Reading Gadget Values

**Cycle Gadgets:**
```c
WORD selection;
GT_GetGadgetAttrs(gadget, window, NULL,
    GTCY_Active, &selection,
    TAG_END);
```

**Checkboxes:**
```c
BOOL checked;
GT_GetGadgetAttrs(gadget, window, NULL,
    GTCB_Checked, &checked,
    TAG_END);
```

**String Gadget:**
```c
/* Direct access to buffer */
printf("%s\n", win_data->folder_path_buffer);
```

---

## 🔌 Integration Points

### Global Variable Mapping (Phase 3)

| GUI Setting | Global Variable | Type |
|-------------|-----------------|------|
| folder_path_buffer | ProcessDirectory() param | char* |
| layout_selected | user_folderViewMode | int |
| sort_selected | user_folderViewSort | int |
| order_selected | user_folderViewOrder | int |
| sortby_selected | user_folderViewBy | int |
| optimize_columns | user_dontResize (inverted!) | int |
| recursive_subdirs | ProcessDirectory() recursive param | BOOL |
| enable_backup | user_enableBackup | BOOL |
| center_icons | (new feature) | - |
| enable_icon_upgrade | (future) | - |

---

## 🧪 Testing Checklist

### Visual Testing
- [ ] All gadgets visible and properly aligned
- [ ] Labels readable and properly positioned
- [ ] Gadgets respond to mouse clicks
- [ ] Cycle gadgets show dropdown lists
- [ ] Checkboxes show checkmarks when selected
- [ ] Buttons highlight when clicked
- [ ] String gadget accepts text input
- [ ] Window dragable and depth-able

### Functional Testing
- [ ] Folder path can be edited
- [ ] Browse button responds (TODO: ASL requester)
- [ ] Preset cycle changes visible
- [ ] Layout cycle changes visible
- [ ] Sort cycle changes visible
- [ ] Order cycle changes visible
- [ ] Sort By cycle changes visible
- [ ] Center icons checkbox toggles
- [ ] Optimize columns checkbox toggles
- [ ] Recursive checkbox toggles
- [ ] Backup checkbox toggles
- [ ] Icon Upgrade checkbox disabled (gray)
- [ ] Advanced button disabled (gray)
- [ ] Apply button shows debug output
- [ ] Cancel button closes window
- [ ] Close gadget closes window

### Debug Output Testing
- [ ] Preset changes logged
- [ ] Layout changes logged
- [ ] Sort changes logged
- [ ] Order changes logged
- [ ] Sort By changes logged
- [ ] Checkbox toggles logged
- [ ] Apply button shows all current settings
- [ ] Folder path changes logged

### Resource Testing
- [ ] Window opens without errors
- [ ] All gadgets created successfully
- [ ] No memory leaks on close
- [ ] Clean shutdown logged
- [ ] Can open/close multiple times

---

## 🐛 Known Limitations (Phase 2)

1. **Browse Button** - Not yet connected to ASL requester
2. **Apply Button** - Prints debug info but doesn't process icons yet
3. **Advanced Button** - Disabled (Phase 3/4 feature)
4. **Icon Upgrade** - Disabled (future feature)
5. **Preset Changes** - Don't update other gadgets yet
6. **No Validation** - Folder path not validated
7. **No Progress Feedback** - Will be added when processing connected
8. **No Error Messages** - Will be added with requester system

---

## 📈 Phase 3 TODO

### Browse Button (ASL Integration)
```c
case GID_BROWSE:
    if (request_directory(win_data->folder_path_buffer, 256))
    {
        /* Update string gadget display */
        GT_SetGadgetAttrs(win_data->folder_path, win_data->window, NULL,
            GTST_String, win_data->folder_path_buffer,
            TAG_END);
    }
    break;
```

### Apply Button (Processing)
```c
case GID_APPLY:
    /* Validate folder */
    if (!validate_path(win_data->folder_path_buffer))
    {
        show_error("Invalid folder path!");
        break;
    }
    
    /* Map settings to globals */
    user_folderViewMode  = win_data->layout_selected;
    user_folderViewSort  = win_data->sort_selected;
    user_folderViewOrder = win_data->order_selected;
    user_folderViewBy    = win_data->sortby_selected;
    user_dontResize      = !win_data->optimize_columns;
    
    /* Process directory */
    ProcessDirectory(win_data->folder_path_buffer, 
                     win_data->recursive_subdirs);
    
    /* Show completion */
    show_info("Icon cleanup complete!");
    break;
```

### Preset System (Auto-Update)
```c
case GID_PRESET:
    /* Get new preset */
    GT_GetGadgetAttrs(gad, win_data->window, NULL,
        GTCY_Active, &win_data->preset_selected,
        TAG_END);
    
    /* Apply preset settings */
    apply_preset(win_data, win_data->preset_selected);
    
    /* Update all gadgets to reflect preset */
    update_all_gadgets(win_data);
    break;
```

---

## 📝 Code Examples

### Opening the Window
```c
struct iTidyMainWindow gui_window;

if (open_itidy_main_window(&gui_window))
{
    printf("GUI window opened successfully\n");
}
```

### Event Loop
```c
BOOL keep_running = TRUE;
while (keep_running)
{
    keep_running = handle_itidy_window_events(&gui_window);
}
```

### Closing the Window
```c
close_itidy_main_window(&gui_window);
```

### Reading Current Settings
```c
printf("Folder: %s\n", gui_window.folder_path_buffer);
printf("Preset: %s\n", preset_labels[gui_window.preset_selected]);
printf("Layout: %s\n", layout_labels[gui_window.layout_selected]);
printf("Recursive: %s\n", gui_window.recursive_subdirs ? "Yes" : "No");
```

---

## 🔗 Related Files

- **Header:** `src/GUI/main_window.h`
- **Implementation:** `src/GUI/main_window.c`
- **Main Program:** `src/main_gui.c`
- **Design Doc:** `docs/iTidy_GUI_Feature_Design.md`
- **Phase 2 Report:** `docs/PHASE2_COMPLETE.md`

---

**Quick Reference Version:** 2.0  
**Last Updated:** Phase 2 Completion  
**Status:** ✅ All Controls Implemented and Functional
