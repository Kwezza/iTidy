# Layout Preferences Module

**Status:** ✅ Implemented  
**Files:** `src/layout_preferences.h`, `src/layout_preferences.c`  
**Added:** Phase 2 - Foundation for Phase 3 Integration

---

## Overview

The Layout Preferences module provides a unified structure and helper functions for managing iTidy's icon layout, sorting, window resizing, and backup preferences. This module bridges the GUI layer with the icon processing engine.

---

## Structure Definitions

### LayoutMode Enum
Controls whether icons are arranged in rows or columns:
```c
typedef enum {
    LAYOUT_MODE_ROW = 0,      /* Row-major: Fill horizontally first */
    LAYOUT_MODE_COLUMN = 1    /* Column-major: Fill vertically first */
} LayoutMode;
```

### SortOrder Enum
Defines the primary sorting direction:
```c
typedef enum {
    SORT_ORDER_HORIZONTAL = 0,  /* Left-to-right, top-to-bottom */
    SORT_ORDER_VERTICAL = 1     /* Top-to-bottom, left-to-right */
} SortOrder;
```

### SortPriority Enum
Controls folder vs file grouping:
```c
typedef enum {
    SORT_PRIORITY_FOLDERS_FIRST = 0,  /* Folders before files */
    SORT_PRIORITY_FILES_FIRST = 1,    /* Files before folders */
    SORT_PRIORITY_MIXED = 2           /* No priority */
} SortPriority;
```

### SortBy Enum
Primary sort criteria:
```c
typedef enum {
    SORT_BY_NAME = 0,   /* Alphabetical by name */
    SORT_BY_TYPE = 1,   /* By file type/extension */
    SORT_BY_DATE = 2,   /* By modification date */
    SORT_BY_SIZE = 3    /* By file size */
} SortBy;
```

### BackupPreferences Structure
Backup configuration:
```c
typedef struct {
    BOOL enableUndoBackup;           /* Enable backup creation */
    BOOL useLha;                     /* Use LhA compression */
    char backupRootPath[108];        /* Backup directory */
    UWORD maxBackupsPerFolder;       /* Retention limit */
} BackupPreferences;
```

### LayoutPreferences Structure
Master preferences structure:
```c
typedef struct {
    /* Layout Settings */
    LayoutMode layoutMode;
    SortOrder sortOrder;
    SortPriority sortPriority;
    SortBy sortBy;
    BOOL reverseSort;
    
    /* Visual Settings */
    BOOL centerIconsInColumn;
    BOOL useColumnWidthOptimization;
    
    /* Window Management */
    BOOL resizeWindows;
    UWORD maxIconsPerRow;
    UWORD maxWindowWidthPct;
    float aspectRatio;
    
    /* Backup Settings */
    BackupPreferences backupPrefs;
} LayoutPreferences;
```

---

## Functions

### InitLayoutPreferences()
Initializes a preferences structure with default values (Classic preset).

**Signature:**
```c
void InitLayoutPreferences(LayoutPreferences *prefs);
```

**Usage:**
```c
LayoutPreferences prefs;
InitLayoutPreferences(&prefs);
```

**Defaults:**
- Layout: Row-major
- Sort: Horizontal, Folders First, By Name
- Optimize columns: ON
- Resize windows: ON
- Max icons per row: 10
- Max window width: 55% of screen
- Aspect ratio: 1.6
- Backup: OFF

---

### ApplyPreset()
Applies a predefined configuration preset.

**Signature:**
```c
void ApplyPreset(LayoutPreferences *prefs, int preset);
```

**Presets:**

| Preset | Layout | Sort | Order | Use Case |
|--------|--------|------|-------|----------|
| `PRESET_CLASSIC` (0) | Row | Horizontal | Folders First | Traditional Workbench |
| `PRESET_COMPACT` (1) | Column | Vertical | Folders First | Many icons, tight layout |
| `PRESET_MODERN` (2) | Row | Horizontal | Mixed | No folder priority |
| `PRESET_WHDLOAD` (3) | Column | Vertical | Folders First | Game collections |

**Usage:**
```c
LayoutPreferences prefs;
InitLayoutPreferences(&prefs);
ApplyPreset(&prefs, PRESET_COMPACT);
```

**Preset Details:**

#### Classic Preset
```
Layout: Row-major
Sort: Horizontal, Folders First, By Name
Max icons/row: 10
Window width: 55%
Aspect ratio: 1.6
```

#### Compact Preset
```
Layout: Column-major
Sort: Vertical, Folders First, By Name
Max icons/row: 6
Window width: 45%
Aspect ratio: 1.3
```

#### Modern Preset
```
Layout: Row-major
Sort: Horizontal, Mixed, By Date
Max icons/row: 8
Window width: 60%
Aspect ratio: 1.5
```

#### WHDLoad Preset
```
Layout: Column-major
Sort: Vertical, Folders First, By Name
Max icons/row: 4
Window width: 40%
Aspect ratio: 1.4
```

---

### MapGuiToPreferences()
Copies GUI control values to preferences structure.

**Signature:**
```c
void MapGuiToPreferences(LayoutPreferences *prefs,
                         int layout, int sort, int order, int sortby,
                         BOOL center, BOOL optimize);
```

**Parameters:**
- `prefs` - Target preferences structure
- `layout` - Layout cycle selection (0=Row, 1=Column)
- `sort` - Sort cycle selection (0=Horizontal, 1=Vertical)
- `order` - Order cycle selection (0=Folders First, 1=Files First, 2=Mixed)
- `sortby` - Sort By cycle selection (0=Name, 1=Type, 2=Date, 3=Size)
- `center` - Center icons checkbox state
- `optimize` - Optimize columns checkbox state

**Usage:**
```c
LayoutPreferences prefs;
InitLayoutPreferences(&prefs);

/* Map from iTidyMainWindow GUI structure */
MapGuiToPreferences(&prefs,
    gui_window.layout_selected,
    gui_window.sort_selected,
    gui_window.order_selected,
    gui_window.sortby_selected,
    gui_window.center_icons,
    gui_window.optimize_columns);
```

---

## Integration with GUI

### Phase 3 Integration Example

When the user clicks "Apply" button:

```c
case GID_APPLY:
{
    LayoutPreferences prefs;
    
    /* Initialize with defaults */
    InitLayoutPreferences(&prefs);
    
    /* Apply preset if selected */
    if (win_data->preset_selected >= 0) {
        ApplyPreset(&prefs, win_data->preset_selected);
    }
    
    /* Override with GUI values */
    MapGuiToPreferences(&prefs,
        win_data->layout_selected,
        win_data->sort_selected,
        win_data->order_selected,
        win_data->sortby_selected,
        win_data->center_icons,
        win_data->optimize_columns);
    
    /* Add folder path and recursive flag */
    prefs.resizeWindows = TRUE;  /* From advanced settings */
    
    /* Call icon processing with preferences */
    ProcessDirectoryWithPreferences(
        win_data->folder_path_buffer,
        win_data->recursive_subdirs,
        &prefs);
    
    break;
}
```

---

## Default Constants

Defined in `layout_preferences.h`:

```c
#define DEFAULT_LAYOUT_MODE         LAYOUT_MODE_ROW
#define DEFAULT_SORT_ORDER          SORT_ORDER_HORIZONTAL
#define DEFAULT_SORT_PRIORITY       SORT_PRIORITY_FOLDERS_FIRST
#define DEFAULT_SORT_BY             SORT_BY_NAME
#define DEFAULT_REVERSE_SORT        FALSE
#define DEFAULT_CENTER_ICONS        FALSE
#define DEFAULT_OPTIMIZE_COLUMNS    TRUE
#define DEFAULT_RESIZE_WINDOWS      TRUE
#define DEFAULT_MAX_ICONS_PER_ROW   10
#define DEFAULT_MAX_WIDTH_PCT       55
#define DEFAULT_ASPECT_RATIO        1.6f
```

---

## Build Information

**Added to Makefile:**
```makefile
CORE_SRCS = \
    $(SRC_DIR)/layout_preferences.c
```

**Size Impact:**
- Previous executable: 71,576 bytes
- With layout_preferences: 72,136 bytes
- **Increase: +560 bytes** (+0.8%)

**Compilation:**
```
✅ Compiles cleanly with VBCC
✅ No warnings or errors
✅ Linked successfully
```

---

## Future Enhancements

### ENV: Persistence (Phase 4)
Save/load preferences to/from ENV: variables:
```c
BOOL SavePreferencesToEnv(LayoutPreferences *prefs);
BOOL LoadPreferencesFromEnv(LayoutPreferences *prefs);
```

### Validation Functions
```c
BOOL ValidatePreferences(LayoutPreferences *prefs);
void ClampPreferencesToLimits(LayoutPreferences *prefs);
```

### Preset Management
```c
BOOL SaveCustomPreset(const char *name, LayoutPreferences *prefs);
BOOL LoadCustomPreset(const char *name, LayoutPreferences *prefs);
```

---

## Testing Checklist

- [x] Structure compiles without errors
- [x] InitLayoutPreferences() sets all defaults
- [x] ApplyPreset() works for all 4 presets
- [x] MapGuiToPreferences() correctly maps GUI values
- [ ] Integration with ProcessDirectory() (Phase 3)
- [ ] ENV: persistence (Phase 4)
- [ ] Custom preset saving/loading (Phase 4+)

---

## References

- **Design Document:** `docs/iTidy_GUI_Feature_Design.md`
- **GUI Integration:** `src/GUI/main_window.c`
- **Icon Processing:** `src/icon_management.c` (Phase 3 integration point)

---

**Module Status:** ✅ Ready for Phase 3 Integration  
**Last Updated:** Phase 2 Completion
