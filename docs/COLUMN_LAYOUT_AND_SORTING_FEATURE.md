# Column Layout and Advanced Sorting Feature Design

**Date:** October 19, 2025  
**Status:** Proposed Feature - GUI Application  
**Related:** Window Layout & Aspect Ratio Feature  
**Target:** Workbench GUI Application

---

## Executive Summary

iTidy fills a critical gap in Workbench folder management. While **Workbench 3.2** introduced a "Clean Up" menu option that can auto-tidy a single folder, it has significant limitations:

- **No customization** - You can't adjust how icons are arranged
- **No batch processing** - Only one folder at a time (no subdirectories)
- **No layout options** - Fixed algorithm with no user control

**iTidy's Key Differentiators:**
- ✅ **Recursive subdirectory processing** - Tidy an entire directory tree with one operation
- ✅ **Fully customizable layouts** - Choose row/column modes, sort orders, and aspect ratios
- ✅ **Preset system** - Save and reuse configurations for different folder types
- ✅ **WHDLoad optimization** - Special handling for folders with long filenames
- ✅ **Batch operations** - Process multiple folder hierarchies efficiently

This makes iTidy essential for users managing large file collections (WHDLoad archives, work projects, media libraries) where manually tidying each subfolder would be impractical.

---

## Table of Contents
1. [Current Behavior Analysis](#current-behavior-analysis)
2. [Identified Issues](#identified-issues)
3. [Proposed Solution](#proposed-solution)
4. [Implementation Design](#implementation-design)
5. [GUI User Interface Design](#gui-user-interface-design)
6. [Example Scenarios](#example-scenarios)

---

## Current Behavior Analysis

### How Icon Layout Currently Works

#### Current Sorting (in `icon_management.c`)
```c
/* Sort icons by name and folder for a consistent layout */
qsort(iconArray->array, totalIcons, sizeof(FullIconDetails), CompareByFolderAndName);
```

**Current Comparison Function:**
```c
int CompareByFolderAndName(const void *a, const void *b)
{
    const FullIconDetails *iconA = (const FullIconDetails *)a;
    const FullIconDetails *iconB = (const FullIconDetails *)b;

    /* Compare by is_folder: folders should come before files */
    if (iconA->is_folder != iconB->is_folder)
    {
        return iconA->is_folder ? -1 : 1;  // Folders first
    }

    /* If both are either folders or files, compare by icon_text */
    return strncasecmp_custom(iconA->icon_text, iconB->icon_text, ...);
}
```

**What This Does:**
1. **Folders always come first** (sorted alphabetically)
2. **Files come after folders** (sorted alphabetically)
3. **Sorting is horizontal** (left-to-right, then top-to-bottom)

#### Current Column Width Calculation
```c
int largestIconWidth, columnWidths[100];

/* Determine the maximum width for each column */
for (i = 0; i < totalIcons; i++)
{
    column = i % iconsPerRow;
    columnWidths[column] = MAX(columnWidths[column], iconArray->array[i].icon_max_width);
}
```

**What This Does:**
- Calculates the widest icon in each column
- Uses `icon_max_width` which is: `MAX(icon_width + border*2, text_width)`
- Icons are then centered within their column's width

```c
/* Center the icon within the column width */
centerX = ((columnWidths[column] - (iconArray->array[i].icon_width + border*2)) / 2);
iconArray->array[i].icon_x = x + centerX;
```

### Current Layout Behavior

**Example with 12 icons, 4 per row:**
```
Row 1: [Icon1] [Icon2] [Icon3] [Icon4]
Row 2: [Icon5] [Icon6] [Icon7] [Icon8]
Row 3: [Icon9] [Icon10][Icon11][Icon12]
```

- Each column has consistent width (widest icon in that column)
- Icons are centered horizontally within their column
- Sorting flows **horizontally** (row-major order)

---

## Identified Issues

### Issue 1: WHDLoad Long Filenames

**Problem:**
WHDLoad archives often have very descriptive names with metadata:
- `GameName_v1.2_AGA_1MB_Disk1of3_NoIntro_en.lha`
- `Application_68020_4MB_FastRAM_PAL_v2.5.lha`

**Result:**
- Text width can be 200-400+ pixels
- Much wider than the icon itself (typically 40-80px)
- Creates very wide columns
- Layout becomes unbalanced and ugly
- Window becomes unnecessarily wide

**Current Calculation:**
```c
newIcon.icon_max_width = MAX(iconSize.width + (border * 2), textExtent.te_Width);
```
The text width dominates, creating disproportionately wide icons.

### Issue 2: No Vertical Sorting Option

**Current:** Icons sort horizontally (row-major):
```
Folder1  Folder2  Folder3
File1    File2    File3
File4    File5    File6
```

**Some Users Prefer:** Vertical sorting (column-major):
```
Folder1  Folder3  File2
Folder2  File1    File3
         File4    File6
```

**Why It Matters:**
- Easier to scan alphabetically in columns
- More like traditional file managers (Opus, Scalos)
- Better for narrow, tall windows

### Issue 3: Limited Sorting Flexibility

**Current:** Hard-coded to "folders first, then files"

**User Needs:**
- Some want all items sorted together (no folder preference)
- Some want files first (for tool drawers)
- Some want to sort by date, size, or type (future)

### Issue 4: No Column Mode

**Current:** Row-based layout with column width optimization

**Missing:** True column-based layout where:
- Icons are arranged vertically first
- Each column's width is independent
- Can have uneven column heights (better space usage)

---

## Proposed Solution

### Multi-Faceted Approach

1. **Layout Preferences Structure** - Encapsulate all layout options
2. **Column vs. Row Layout Modes** - Two distinct layout algorithms
3. **Flexible Sorting Options** - Configurable sort order and priority
4. **Multiple Comparison Functions** - Pluggable sorting strategies

---

## Implementation Design

### 1. New Layout Preferences Structure

Create a comprehensive preferences struct to hold all layout options:

```c
/* Layout and sorting preferences */
typedef enum {
    LAYOUT_MODE_ROW = 0,      /* Current behavior: row-major (horizontal first) */
    LAYOUT_MODE_COLUMN = 1     /* New: column-major (vertical first) */
} LayoutMode;

typedef enum {
    SORT_ORDER_HORIZONTAL = 0, /* Sort left-to-right, top-to-bottom (current) */
    SORT_ORDER_VERTICAL = 1    /* Sort top-to-bottom, left-to-right */
} SortOrder;

typedef enum {
    SORT_PRIORITY_FOLDERS_FIRST = 0,  /* Folders before files (current) */
    SORT_PRIORITY_FILES_FIRST = 1,    /* Files before folders */
    SORT_PRIORITY_MIXED = 2           /* All items sorted together */
} SortPriority;

typedef enum {
    SORT_BY_NAME = 0,         /* Alphabetical by name (current) */
    SORT_BY_TYPE = 1,         /* By file type */
    SORT_BY_DATE = 2,         /* By modification date (future) */
    SORT_BY_SIZE = 3          /* By file size (future) */
} SortBy;

typedef struct {
    LayoutMode layoutMode;          /* Row-major or column-major layout */
    SortOrder sortOrder;            /* Horizontal or vertical sort direction */
    SortPriority sortPriority;      /* Folder/file priority */
    SortBy sortBy;                  /* Sort criteria */
    BOOL centerIconsInColumn;       /* Center icons within column width (current behavior) */
    BOOL useColumnWidthOptimization; /* Calculate per-column widths (current behavior) */
} LayoutPreferences;
```

### 2. Global Variable Declaration

Add to `main.c`:
```c
/* Layout and sorting preferences */
LayoutPreferences layoutPrefs = {
    LAYOUT_MODE_ROW,              /* Default: row-major (current) */
    SORT_ORDER_HORIZONTAL,        /* Default: horizontal sort */
    SORT_PRIORITY_FOLDERS_FIRST,  /* Default: folders first (current) */
    SORT_BY_NAME,                 /* Default: alphabetical */
    TRUE,                         /* Default: center icons in columns */
    TRUE                          /* Default: optimize column widths */
};
```

Add to `main.h`:
```c
extern LayoutPreferences layoutPrefs;
```

### 3. Enhanced Comparison Functions

Create a family of comparison functions for different sorting strategies:

```c
/* Generic comparison function that uses layoutPrefs */
int CompareIcons(const void *a, const void *b)
{
    const FullIconDetails *iconA = (const FullIconDetails *)a;
    const FullIconDetails *iconB = (const FullIconDetails *)b;
    int result = 0;

    /* Apply folder/file priority first */
    if (layoutPrefs.sortPriority == SORT_PRIORITY_FOLDERS_FIRST)
    {
        if (iconA->is_folder != iconB->is_folder)
            return iconA->is_folder ? -1 : 1;  /* Folders first */
    }
    else if (layoutPrefs.sortPriority == SORT_PRIORITY_FILES_FIRST)
    {
        if (iconA->is_folder != iconB->is_folder)
            return iconA->is_folder ? 1 : -1;  /* Files first */
    }
    /* SORT_PRIORITY_MIXED: no folder/file distinction, fall through to name sort */

    /* Then sort by the chosen criteria */
    switch (layoutPrefs.sortBy)
    {
        case SORT_BY_NAME:
            result = strncasecmp_custom(iconA->icon_text, iconB->icon_text,
                                       MAX(strlen(iconA->icon_text), strlen(iconB->icon_text)));
            break;
        
        case SORT_BY_TYPE:
            /* Sort by file extension, then name */
            result = CompareByType(iconA, iconB);
            break;
        
        case SORT_BY_DATE:
            /* Future: compare modification dates */
            result = CompareByDate(iconA, iconB);
            break;
        
        case SORT_BY_SIZE:
            /* Future: compare file sizes */
            result = CompareBySize(iconA, iconB);
            break;
    }

    return result;
}

/* Helper: Compare by file type (extension) */
int CompareByType(const FullIconDetails *iconA, const FullIconDetails *iconB)
{
    const char *extA = strrchr(iconA->icon_text, '.');
    const char *extB = strrchr(iconB->icon_text, '.');
    int extResult;

    /* Files without extensions sort before files with extensions */
    if (!extA && extB) return -1;
    if (extA && !extB) return 1;
    if (!extA && !extB) return strncasecmp_custom(iconA->icon_text, iconB->icon_text, 256);

    /* Compare extensions */
    extResult = strncasecmp_custom(extA, extB, 32);
    if (extResult != 0) return extResult;

    /* Same extension, compare full names */
    return strncasecmp_custom(iconA->icon_text, iconB->icon_text, 256);
}
```

### 4. Layout Mode: Column-Major

Add a new column-major layout mode alongside the existing row-major mode:

```c
/* Column-major layout: arrange icons vertically first */
void ArrangeIconsColumnMajor(IconArray *iconArray, int iconsPerRow, int totalIcons)
{
    int col, row, iconIndex, maxRowsPerColumn;
    int x, y, columnWidths[100];
    
    /* Calculate how many rows we'll have per column */
    maxRowsPerColumn = (totalIcons + iconsPerRow - 1) / iconsPerRow;
    
    /* Initialize column widths */
    for (col = 0; col < iconsPerRow; col++)
    {
        columnWidths[col] = 0;
    }
    
    /* First pass: Calculate column widths */
    for (col = 0; col < iconsPerRow; col++)
    {
        for (row = 0; row < maxRowsPerColumn; row++)
        {
            iconIndex = col * maxRowsPerColumn + row;
            if (iconIndex >= totalIcons) break;
            
            columnWidths[col] = MAX(columnWidths[col], 
                                   iconArray->array[iconIndex].icon_max_width);
        }
    }
    
    /* Second pass: Position icons */
    x = ICON_START_X;
    for (col = 0; col < iconsPerRow; col++)
    {
        y = ICON_START_Y;
        
        for (row = 0; row < maxRowsPerColumn; row++)
        {
            iconIndex = col * maxRowsPerColumn + row;
            if (iconIndex >= totalIcons) break;
            
            /* Center icon in column if enabled */
            if (layoutPrefs.centerIconsInColumn)
            {
                int centerX = (columnWidths[col] - 
                              iconArray->array[iconIndex].icon_max_width) / 2;
                iconArray->array[iconIndex].icon_x = x + centerX;
            }
            else
            {
                iconArray->array[iconIndex].icon_x = x;
            }
            
            iconArray->array[iconIndex].icon_y = y;
            
            y += iconArray->array[iconIndex].icon_max_height + ICON_SPACING_Y;
        }
        
        x += columnWidths[col] + ICON_SPACING_X;
    }
}
```

### 5. Modified ArrangeIcons Function

Update the main function to use the preferences:

```c
int ArrangeIcons(BPTR lock, char *dirPath, int newWidth)
{
    /* ... existing variable declarations ... */
    
    iconArray = CreateIconArrayFromPath(lock, dirPath);
    if (iconArray == NULL || iconArray->array == NULL || iconArray->size <= 0)
        return -1;

    totalIcons = iconArray->size;

    /* Sort icons using the new comparison function */
    qsort(iconArray->array, totalIcons, sizeof(FullIconDetails), CompareIcons);

    /* ... existing width calculations ... */

    /* Choose layout mode */
    if (layoutPrefs.layoutMode == LAYOUT_MODE_COLUMN)
    {
        ArrangeIconsColumnMajor(iconArray, iconsPerRow, totalIcons);
    }
    else
    {
        /* Existing row-major layout code */
        ArrangeIconsRowMajor(iconArray, iconsPerRow, totalIcons);
    }

    /* ... rest of function ... */
}
```

---

## GUI User Interface Design

### Overview
iTidy will be a **Workbench GUI application** providing an intuitive interface for users to configure icon layout and sorting preferences. The GUI will offer:
- **Simple presets** for common use cases (recommended for most users)
- **Advanced settings panel** for power users who want fine-grained control
- **Live preview** (optional) showing how the layout will appear
- **Save/Load preset profiles** for different folders or workflows

---

### Main Window Design

#### Window Structure
```
╔═══════════════════════════════════════════════════════════╗
║                         iTidy v1.1                         ║
║          Batch Folder Organizer with Recursive Tidy        ║
╠═══════════════════════════════════════════════════════════╣
║  Folder: [___DH0:MyFolder________________] [Browse...]    ║
║                                                            ║
║  ┌─ Quick Presets ───────────────────────────────────┐    ║
║  │  ○ Classic (Row layout, Folders first)            │    ║
║  │  ○ Compact (Column layout, Vertical sort)         │    ║
║  │  ○ Modern (Mixed sort, Row layout)                │    ║
║  │  ○ WHDLoad (Column layout, handles long names)    │    ║
║  │  ● Custom (Configure below)                       │    ║
║  └────────────────────────────────────────────────────┘    ║
║                                                            ║
║  ┌─ Layout Settings ──────────────────────────────────┐   ║
║  │  Layout Mode:   [Row-based ▼]                      │   ║
║  │  Sort Order:    [Horizontal ▼]                     │   ║
║  │  Sort By:       [Name      ▼]                      │   ║
║  │  Folder Order:  [Folders First ▼]                  │   ║
║  │                                                     │   ║
║  │  [✓] Center icons in columns                       │   ║
║  │  [✓] Optimize column widths                        │   ║
║  └─────────────────────────────────────────────────────┘   ║
║                                                            ║
║  ┌─ Batch Processing (iTidy's Key Feature!) ──────────┐   ║
║  │  [✓] Include subdirectories (recursive)            │   ║
║  │      Process all folders in the entire tree        │   ║
║  │                                                     │   ║
║  │  [ ] Skip WHDLoad folders (preserve author layout) │   ║
║  │      When checked, only resize/center WHD windows  │   ║
║  └─────────────────────────────────────────────────────┘   ║
║                                                            ║
║  [  Advanced...  ] [  Save Preset...  ] [ Load Preset ]   ║
║                                                            ║
║              [     Apply     ]  [    Cancel    ]           ║
╚═══════════════════════════════════════════════════════════╝
```

---

### Quick Presets (Recommended for Most Users)

The main window provides **four quick presets** that encode sensible defaults:

#### 1. **Classic Preset** (Default)
*Current iTidy behavior - familiar to existing users*
- **Layout Mode:** Row-based (horizontal first)
- **Sort Order:** Horizontal (left-to-right, then down)
- **Sort By:** Name (alphabetical)
- **Folder Order:** Folders First
- **Best For:** Traditional Amiga users, PAL/NTSC screens, standard folders

#### 2. **Compact Preset**
*Optimized for RTG screens and efficient space usage*
- **Layout Mode:** Column-based (vertical first)
- **Sort Order:** Vertical (top-to-bottom, then right)
- **Sort By:** Name (alphabetical)
- **Folder Order:** Folders First
- **Best For:** RTG screens, small folders, modern look

#### 3. **Modern Preset**
*No folder priority - everything sorted together*
- **Layout Mode:** Row-based (horizontal first)
- **Sort Order:** Horizontal (left-to-right, then down)
- **Sort By:** Name (alphabetical)
- **Folder Order:** Mixed (no folder priority)
- **Best For:** Users who want all items sorted together, tool drawers

#### 4. **WHDLoad Preset**
*Specifically designed for folders with very long filenames*
- **Layout Mode:** Column-based (vertical first)
- **Sort Order:** Vertical (top-to-bottom, then right)
- **Sort By:** Name (alphabetical)
- **Folder Order:** Folders First
- **Best For:** WHDLoad game collections, archives with descriptive names

#### 5. **Custom**
*Enables manual configuration using the settings panel below*
- User configures all options manually
- Settings are remembered for next launch

---

### Batch Processing Panel (iTidy's Core Advantage)

This section highlights iTidy's primary strength over Workbench 3.2's built-in "Clean Up" feature:

#### Include Subdirectories Checkbox
```
[✓] Include subdirectories (recursive)
    Process all folders in the entire tree
```

**When CHECKED (default):**
- iTidy recursively processes the selected folder AND all subfolders
- Applies the same layout settings to every folder in the hierarchy
- Perfect for organizing large directory trees (e.g., `DH0:Games/WHDLoad` with 100+ game folders)
- Shows progress: "Processing folder 23 of 147..."

**When UNCHECKED:**
- Works like Workbench 3.2 "Clean Up" - only processes the selected folder
- Faster for single-folder tidying
- Useful when you only want to organize one specific folder

**Example Use Cases:**
- ✅ **Checked:** Tidy `Work:Projects` with 50 project subfolders
- ✅ **Checked:** Organize `DH0:WHDLoad` with 200 game subdirectories
- ✅ **Checked:** Clean up `Downloads:` and all its category subfolders
- ❌ **Unchecked:** Just tidy `SYS:Tools` without touching SYS:Tools/Commodities, etc.

#### Skip WHDLoad Folders Option
```
[ ] Skip WHDLoad folders (preserve author layout)
    When checked, only resize/center WHD windows
```

**When CHECKED:**
- If a folder contains `.slave` files (WHDLoad games), preserve the original icon layout
- Still resizes and centers the window to fit properly
- Respects the game author's carefully crafted icon arrangements
- Useful for WHDLoad archives where authors placed icons artistically

**When UNCHECKED:**
- Tidy all folders including WHDLoad games
- Use when you want consistent layout across ALL folders
- Good for collections where you don't care about preserving original layouts

---

### Layout Settings Panel

When **Custom** is selected, all settings become active:

#### Layout Mode Cycle Gadget
```
Layout Mode: [Row-based ▼]
```
Options:
- **Row-based** - Icons arranged left-to-right, then down (current behavior)
- **Column-based** - Icons arranged top-to-bottom, then right (new)

#### Sort Order Cycle Gadget
```
Sort Order: [Horizontal ▼]
```
Options:
- **Horizontal** - Sort left-to-right, top-to-bottom (current)
- **Vertical** - Sort top-to-bottom, left-to-right (new)

#### Sort By Cycle Gadget
```
Sort By: [Name ▼]
```
Options:
- **Name** - Alphabetical by filename (default)
- **Type** - By file extension, then name
- **Date** - By modification date (future)
- **Size** - By file size (future)

#### Folder Order Cycle Gadget
```
Folder Order: [Folders First ▼]
```
Options:
- **Folders First** - All folders before all files (current)
- **Files First** - All files before all folders
- **Mixed** - All items sorted together (no folder/file distinction)

#### Checkboxes
```
[✓] Center icons in columns
[✓] Optimize column widths
[✓] Include subdirectories
```

---

### Advanced Settings Window

Clicking **[Advanced...]** opens a secondary window with additional options:

```
╔═══════════════════════════════════════════════════════════╗
║                    Advanced Settings                       ║
╠═══════════════════════════════════════════════════════════╣
║  ┌─ Window Dimensions ────────────────────────────────┐   ║
║  │  Aspect Ratio:   [Balanced ▼]                      │   ║
║  │  Max Width:      [50% ▼] or [____] pixels          │   ║
║  │  Max Icons/Row:  [10    ]                          │   ║
║  └─────────────────────────────────────────────────────┘   ║
║                                                            ║
║  ┌─ Icon Spacing ─────────────────────────────────────┐   ║
║  │  Horizontal:     [9  ] pixels                      │   ║
║  │  Vertical:       [7  ] pixels                      │   ║
║  └─────────────────────────────────────────────────────┘   ║
║                                                            ║
║  ┌─ Special Options ──────────────────────────────────┐   ║
║  │  [✓] Resize folder windows to fit                  │   ║
║  │  [✓] Center windows on screen                      │   ║
║  │  [ ] Skip WHDLoad folders (preserve author layout) │   ║
║  │  [ ] Reset icon positions                          │   ║
║  │  [ ] Force standard icon sizes                     │   ║
║  └─────────────────────────────────────────────────────┘   ║
║                                                            ║
║                    [   OK   ]  [  Cancel  ]                ║
╚═══════════════════════════════════════════════════════════╝
```

---

### Preset Management

Users can save and load their custom configurations:

#### Save Preset Dialog
```
╔═══════════════════════════════════════════════════════════╗
║                      Save Preset                           ║
╠═══════════════════════════════════════════════════════════╣
║  Preset Name: [My WHDLoad Setup____________________]      ║
║                                                            ║
║  Description: [Column layout optimized for long    ]      ║
║               [filenames with vertical sorting     ]      ║
║               [________________________________    ]      ║
║                                                            ║
║  Save Location: [ENV:iTidy/Presets/ ▼]                    ║
║                                                            ║
║               [   Save   ]  [  Cancel  ]                   ║
╚═══════════════════════════════════════════════════════════╝
```

#### Load Preset Dialog
```
╔═══════════════════════════════════════════════════════════╗
║                      Load Preset                           ║
╠═══════════════════════════════════════════════════════════╣
║  Available Presets:                                        ║
║  ┌────────────────────────────────────────────────────┐   ║
║  │ > Classic (Built-in)                               │   ║
║  │ > Compact (Built-in)                               │   ║
║  │ > Modern (Built-in)                                │   ║
║  │ > WHDLoad (Built-in)                               │   ║
║  │   ────────────────────────────────────────         │   ║
║  │ > My WHDLoad Setup                                 │   ║
║  │ > Tool Drawer Layout                               │   ║
║  │ > Document Folder Config                           │   ║
║  └────────────────────────────────────────────────────┘   ║
║                                                            ║
║  Description: Column layout optimized for long             ║
║               filenames with vertical sorting              ║
║                                                            ║
║               [   Load   ]  [  Delete  ]  [  Cancel  ]     ║
╚═══════════════════════════════════════════════════════════╝
```

---

### Workflow Examples

#### Example 1: Quick Use - Batch Process Entire Directory Tree
**Scenario:** Organize a WHDLoad collection with 150 game subdirectories

1. Launch iTidy from Workbench
2. Click **[Browse...]** and select `DH0:Games/WHDLoad`
3. Select **"WHDLoad"** preset (radio button)
4. Ensure **[✓] Include subdirectories** is checked (default)
5. Click **[Apply]**
6. iTidy processes ALL 150 subfolders automatically!
7. Progress shown: "Processing folder 23 of 150: Bubble Bobble_AGA..."
8. Done in seconds - entire collection organized with one click!

**Compare to Workbench 3.2 Clean Up:**
- WB 3.2: Would require manually opening each of 150 folders and selecting "Clean Up" (tedious!)
- iTidy: One click processes the entire tree (massive time saver!)

#### Example 2: Custom Configuration (Power User)
1. Launch iTidy
2. Select folder: `Work:Projects`
3. Select **"Custom"** preset
4. Change **Layout Mode** to "Column-based"
5. Change **Sort Order** to "Vertical"
6. Change **Folder Order** to "Mixed"
7. Click **[Save Preset...]**
8. Name it "My Project Layout"
9. Click **[Apply]**
10. Next time: Just load "My Project Layout" preset

#### Example 3: Single Folder Mode (Like WB 3.2)
**Scenario:** Just want to tidy one specific folder without touching subfolders

1. Launch iTidy
2. Select folder: `SYS:Utilities`
3. Select **"Classic"** preset
4. **Uncheck** [ ] Include subdirectories
5. Click **[Apply]**
6. Only `SYS:Utilities` is tidied, subfolders like `SYS:Utilities/Clock` are untouched

**This mode works like Workbench 3.2's "Clean Up" but with customization!**

---

### GUI Implementation Notes

#### Technology Options

**Option 1: MUI (Magic User Interface)** - Recommended
- Modern, flexible GUI framework
- Native Amiga look and feel
- Object-oriented design
- Excellent for complex layouts
- Wide compatibility (OS 2.0+)
- Example: YAM, SimpleMail use MUI

**Option 2: ReAction (ClassAct)** - Alternative
- Official OS 3.5+ GUI framework
- Modern gadgets and layouts
- Integrated with Workbench
- Requires OS 3.5+ or ReAction installation

**Option 3: Intuition/GadTools** - Classic
- Built into all Amiga OS versions
- More coding required
- More limited visually
- Maximum compatibility

**Recommendation:** **MUI** for its flexibility, modern appearance, and wide compatibility.

---

### Data Storage

#### Preset Files (ENV:iTidy/Presets/)
Each preset saved as a plain text file:

```
# iTidy Preset File
# Name: My WHDLoad Setup
# Created: 2025-10-19

[Layout]
LayoutMode=Column
SortOrder=Vertical
SortBy=Name
FolderOrder=FoldersFirst

[Options]
CenterIcons=Yes
OptimizeColumns=Yes
IncludeSubdirs=Yes

[Window]
AspectRatio=Balanced
MaxWidthPercent=50
MaxIconsPerRow=10

[Spacing]
HorizontalSpacing=9
VerticalSpacing=7

[Special]
ResizeFolderWindows=Yes
CenterWindows=Yes
SkipWHDLoad=No
ResetIconPositions=No
ForceStandardIcons=No
```

#### Last Used Settings (ENV:iTidy/LastUsed)
Automatically saved when user clicks Apply:
```
[LastUsed]
FolderPath=DH0:Games/WHDLoad
PresetName=WHDLoad
Timestamp=2025-10-19 14:23:45
```

---

### iTidy vs. Workbench 3.2 "Clean Up"

| Feature | WB 3.2 Clean Up | iTidy |
|---------|----------------|-------|
| **Recursive Processing** | ❌ No - One folder at a time | ✅ Yes - Entire directory trees |
| **Customizable Layout** | ❌ Fixed algorithm only | ✅ Row/Column modes, sort orders |
| **Sort Options** | ❌ No control | ✅ By name, type, date, size |
| **Folder Priority** | ❌ Fixed | ✅ Folders first/last/mixed |
| **Aspect Ratio Control** | ❌ No | ✅ Square, wide, tall options |
| **Presets** | ❌ No | ✅ Built-in + user-defined |
| **WHDLoad Handling** | ❌ No | ✅ Smart long-name optimization |
| **Column Optimization** | ❌ No | ✅ Per-column width calculation |
| **Save Configurations** | ❌ No | ✅ Save/load preset files |
| **Batch Processing** | ❌ No | ✅ Queue multiple root folders |
| **Progress Feedback** | ❌ No | ✅ Shows folder count, current folder |
| **Speed** | ⚠️ Slow (one at a time) | ✅ Fast (batch processing) |

**Key Insight:** iTidy is designed for **power users managing large collections**, while WB 3.2 Clean Up is for casual single-folder tidying.

---

### GUI Advantages

✅ **Batch Processing** - Process entire directory trees in one operation (iTidy's killer feature!)  
✅ **Discoverable** - Users can see all available options  
✅ **No memorization** - No need to remember command syntax  
✅ **Visual feedback** - Clear indication of current settings  
✅ **Presets** - One-click access to common configurations  
✅ **Save/Load** - Easy to manage multiple configurations  
✅ **Help** - Context-sensitive help available  
✅ **Validation** - GUI can prevent invalid combinations  
✅ **Workbench integration** - Native Amiga experience  
✅ **Progress tracking** - See which folder is being processed

---

### GUI Features for Future Enhancement

1. **Live Preview Window**
   - Show how icons will be arranged before applying
   - Miniature representation of the layout
   - Update dynamically as settings change

2. **Tooltips/Bubble Help**
   - Hover over any gadget to see description
   - Explain what each setting does
   - Show examples

3. **Drag & Drop**
   - Drag folder icon onto iTidy window
   - Automatically populate folder path
   - Apply last-used or default settings

4. **AppIcon Mode**
   - iTidy runs as an AppIcon on Workbench
   - Drag folders onto icon to tidy with last settings
   - Right-click to open full GUI

5. **Context Menu Integration**
   - Add "iTidy This Folder" to folder context menus
   - Quick access from Workbench

6. **Batch Queue (Multi-Root Processing)**
   - Add multiple root folders to a queue (e.g., DH0:Games, DH1:Work, PC:Downloads)
   - Process them all with one click
   - Each root folder processed recursively
   - Progress indicator: "Queue 2 of 3: Processing DH1:Work, folder 45 of 89..."
   - Example: Tidy your entire system in one operation!

7. **Progress Window**
   - Show detailed progress during batch operations
   - Current folder path being processed
   - Folder count (e.g., "23 of 147")
   - Icons processed in current folder
   - Estimated time remaining
   - Cancel button to stop processing
   - **Critical for large directory trees** (reassures user that iTidy is working)

8. **Undo Function**
   - Remember previous icon positions
   - "Undo Last Tidy" button
   - Restore original layout if user doesn't like result

---

## Example Scenarios

### Scenario 1: WHDLoad Collection with 200 Subdirectories

**Setup:**
- Main folder: `DH0:Games/WHDLoad`
- 200 game subdirectories, each with 2-10 icons
- Very long descriptive folder names (e.g., `BubbleBobble_v1.0_AGA_1MB_2Disk_WHDLoad_v18`)
- User wants consistent layout across ALL game folders

**Without iTidy (Manual WB 3.2 Clean Up):**
1. Open `DH0:Games/WHDLoad`
2. Double-click first game folder (e.g., `Amos_Professional`)
3. Menu: Clean Up By → Name
4. Close folder
5. Repeat steps 2-4 for EACH of the remaining 199 folders
6. **Time required:** ~30-60 minutes of tedious clicking
7. **Result:** Inconsistent layouts (WB 3.2 algorithm varies by icon count)

**With iTidy (Batch Recursive Processing):**
1. Launch iTidy
2. Browse to `DH0:Games/WHDLoad`
3. Select "WHDLoad" preset
4. Ensure [✓] Include subdirectories is checked
5. Click Apply
6. **Time required:** ~30-60 seconds (automatic!)
7. **Result:** ALL 200 folders organized with consistent column layout optimized for long names
8. iTidy shows progress: "Processing folder 147 of 200: Zool_AGA_v1.2..."

**Column Layout + Folders First (applied to each subfolder):**
```
[SaveGames     ] [LongGameFile1_Disk1...] [Manual.txt]
[Screenshots   ] [LongGameFile2_Disk2...] [ReadMe    ]
[Extras        ] [LongGameFile3_HD...]    [WHDLoad   ]
```
- Each game folder gets optimized column layout
- Handles long filenames elegantly
- Consistent experience across entire collection

### Scenario 2: Tool Drawer with Many Small Icons

**Setup:**
- 30 small utility icons
- Mix of drawers and tools
- User prefers to see tools first

**With Files First + Column Layout:**
```
[UtilityA] [UtilityH] [UtilityO] [DrawerA]
[UtilityB] [UtilityI] [UtilityP] [DrawerB]
[UtilityC] [UtilityJ] [UtilityQ] [DrawerC]
[UtilityD] [UtilityK] [UtilityR]
[UtilityE] [UtilityL] [UtilityS]
[UtilityF] [UtilityM] [UtilityT]
[UtilityG] [UtilityN] [UtilityU]
```
- Tools are immediately visible
- Drawers at the end (less frequently accessed)
- Easy to scan down columns

### Scenario 3: Mixed Content Folder

**Setup:**
- Documents, images, folders mixed together
- User wants all sorted alphabetically (no folder priority)

**With Mixed Sorting + Vertical Sort:**
```
Column 1:        Column 2:        Column 3:
[Archive]        [Letter.txt]     [Vacation_Pic]
[Drawing.iff]    [Music]          [Video.avi]
[Games]          [Notes.doc]      [Work_Docs]
[Invoice.pdf]    [Pictures]
```
- All items treated equally
- Pure alphabetical order
- Vertical scanning is natural

### Scenario 4: Comparison - Row vs Column Layout

**12 icons, 4 per row:**

**Row-Major (Current):**
```
Sort order: 1  2  3  4
            5  6  7  8
            9  10 11 12
```
Reading pattern: →→→ down, →→→ down

**Column-Major (New):**
```
Sort order: 1  4  7  10
            2  5  8  11
            3  6  9  12
```
Reading pattern: ↓↓↓ right, ↓↓↓ right

---

## Implementation Checklist

### Phase 1: Core Infrastructure
- [ ] Create `LayoutPreferences` struct in `main.h`
- [ ] Add enum types for layout modes, sort orders, priorities
- [ ] Create global `layoutPrefs` variable in `main.c`
- [ ] Add extern declaration in `main.h`

### Phase 2: Comparison Functions
- [ ] Create generic `CompareIcons()` function
- [ ] Implement `CompareByType()` helper
- [ ] Add stub functions for `CompareByDate()` and `CompareBySize()`
- [ ] Replace `qsort` call to use `CompareIcons`

### Phase 3: Column Layout
- [ ] Create `ArrangeIconsColumnMajor()` function
- [ ] Refactor existing code into `ArrangeIconsRowMajor()`
- [ ] Add mode selection in `ArrangeIcons()`
- [ ] Test column width calculations for vertical layout

### Phase 4: GUI Implementation
- [ ] Design MUI class hierarchy for main window
- [ ] Create preset selection radio button group
- [ ] Implement cycle gadgets for each setting
- [ ] Create checkbox gadgets for options
- [ ] Build Advanced Settings window
- [ ] Implement Save/Load Preset dialogs
- [ ] Add folder selection (ASL file requester)
- [ ] Implement Apply button handler
- [ ] Add settings validation

### Phase 5: Testing
- [ ] Test row-major with all sort options
- [ ] Test column-major with all sort options
- [ ] Test with WHDLoad long names
- [ ] Test with mixed icon counts (3, 10, 30, 100 icons)
- [ ] Test folders-first vs files-first vs mixed
- [ ] Test vertical vs horizontal sort direction
- [ ] Verify column width calculations in both modes

### Phase 6: GUI Polish & Documentation
- [ ] Add keyboard shortcuts (Amiga-Q, Amiga-A, etc.)
- [ ] Implement context-sensitive help
- [ ] Create user manual with screenshots
- [ ] Add tooltips for all gadgets
- [ ] Test on different screen resolutions
- [ ] Create installation guide
- [ ] Write GUI usage examples
- [ ] Add troubleshooting section

### Phase 7: Preset System
- [ ] Implement preset file format (plain text)
- [ ] Create built-in presets (Classic, Compact, Modern, WHDLoad)
- [ ] Add Save Preset functionality
- [ ] Add Load Preset functionality
- [ ] Implement preset deletion
- [ ] Auto-save last used settings
- [ ] Create preset import/export

---

## Benefits

### For Users
✅ **Better layout for long filenames** - Column mode handles WHDLoad names elegantly  
✅ **Familiar sorting options** - Matches other file managers (Opus, Scalos)  
✅ **Flexible prioritization** - Choose folders-first, files-first, or mixed  
✅ **Vertical scanning** - Natural reading pattern for column layouts  
✅ **Better space usage** - Column mode uses vertical space more efficiently

### For Code
✅ **Clean abstraction** - `LayoutPreferences` struct encapsulates all options  
✅ **Pluggable sorting** - Easy to add new sort criteria  
✅ **Backwards compatible** - Default settings match current behavior  
✅ **Testable** - Each component can be tested independently  
✅ **Extensible** - Easy to add date/size sorting later

---

## Future Enhancements

### Phase 7: Advanced Features (Future)
1. **Sort by date** - Implement `CompareByDate()` using file modification time
2. **Sort by size** - Implement `CompareBySize()` using file sizes
3. **Reverse sort** - Add `-reverse` option to reverse any sort order
4. **Custom column spacing** - Allow user to specify column padding
5. **Smart wrapping** - Truncate long names with ellipsis (e.g., "LongName...v1.2")
6. **Save layout preference** - Store in .info file tooltype
7. **Per-folder defaults** - Remember last-used layout for each folder

### Phase 8: Visual Enhancements
1. **Group separators** - Visual line between folders and files
2. **Icon alignment options** - Top, middle, bottom alignment within row
3. **Text truncation modes** - Beginning, middle, or end ellipsis
4. **Tooltips** - Show full name on hover (if Workbench supports it)

---

## Technical Considerations

### Sorting Performance
- Current `qsort` is efficient for small icon counts (<100)
- Comparison function adds minimal overhead
- Pre-sorting by folder type is still fast

### Memory Usage
- `LayoutPreferences` struct is small (~20 bytes)
- No additional dynamic allocations needed
- Column width array already exists (100 ints = 400 bytes)

### Compatibility
- All new features are opt-in (defaults match current behavior)
- No changes to .info file format
- Works on all Amiga OS versions (2.0+)

---

## Addressing the WHDLoad Long Name Issue

### The Core Problem
WHDLoad names like `GameName_v1.2_AGA_1MB_Disk1of3_NoIntro_en` create:
- `icon_max_width` of 300-400+ pixels
- Entire column becomes that wide
- Window grows unnecessarily wide
- Layout looks unbalanced

### Possible Solutions

#### Option 1: Column Layout (Recommended)
- Long-name files go in their own columns
- Other columns remain narrow
- Better vertical space usage
- **No code changes to width calculation needed**

#### Option 2: Text Truncation (Future)
```c
/* Truncate text if wider than threshold */
if (textExtent.te_Width > MAX_TEXT_WIDTH_THRESHOLD)
{
    /* Truncate with ellipsis */
    newIcon.text_width = MAX_TEXT_WIDTH_THRESHOLD;
    newIcon.display_text = TruncateWithEllipsis(iconName, MAX_TEXT_WIDTH_THRESHOLD);
}
```
**Pros:** Limits column width  
**Cons:** User can't see full name, may lose important info

#### Option 3: Multi-line Text (Complex)
```c
/* Word-wrap text to multiple lines */
if (textExtent.te_Width > MAX_TEXT_WIDTH_THRESHOLD)
{
    newIcon.text_lines = WrapText(iconName, MAX_TEXT_WIDTH_THRESHOLD);
    newIcon.text_height = lineHeight * newIcon.text_lines;
}
```
**Pros:** Shows full name  
**Cons:** Complex, non-standard Workbench behavior, increases icon height

#### Recommendation
**Use Option 1 (Column Layout)** as the primary solution:
- Naturally handles long names
- No information loss
- Matches user expectations
- Clean implementation

---
1. **Recursive Processing Default:** Should "Include subdirectories" be checked by default?
   - **Pro (checked):** This is iTidy's core feature - why use iTidy if not for batch processing?
   - **Pro (checked):** Most users will want recursive mode (organizing collections)
   - **Con (checked):** Might surprise users who expect single-folder behavior
   - **Con (checked):** Could accidentally process too many folders
   - **Recommendation:** Default to CHECKED with clear UI indication ("This will process ALL subfolders!")

2. **Default Preset:** Should "Classic" remain default, or should we analyze the folder and auto-select?
   - Classic (current behavior) - familiar to existing users
   - Auto-detect: If long filenames detected → suggest WHDLoad preset
   - Remember last-used preset per userssues (WHDLoad long names, lack of vertical sorting) while providing a flexible framework for future enhancements. The `LayoutPreferences` struct approach is clean, extensible, and maintains backwards compatibility.

The key insight is that **column-major layout naturally solves the long filename problem** without requiring text truncation or other workarounds. Combined with flexible sorting options, this gives users the control they need for different folder types and workflows.

---

## Design Decisions & Questions

1. **Default Preset:** Should "Classic" remain default, or should we analyze the folder and auto-select?
   - Classic (current behavior) - familiar to existing users
   - Auto-detect: If long filenames detected → suggest WHDLoad preset
   - Remember last-used preset per user

2. **Preset Organization:** How should built-in vs user presets be displayed?
   - Separate sections in preset list
   - Built-in presets non-deletable
   - User presets stored in ENV:iTidy/Presets/

3. **GUI Complexity:** Should Advanced Settings be separate or integrated?
   - Current design: Separate window to avoid overwhelming casual users
   - Alternative: Expandable panel in main window
   - Could add "Show Advanced" checkbox

4. **Live Preview:** Is this feasible/desirable?
   - Pro: User can see result before applying
   - Con: Complex to implement, may be slow
   - Alternative: Show example diagrams for each preset
6. **Backward Compatibility:** Should CLI mode still exist?
   - Keep CLI for scripting/batch operations (e.g., startup-sequence automation)
   - GUI launches CLI backend
   - Both modes share same LayoutPreferences struct
   - CLI gets `-subdirs` flag for recursive processing (already implemented)

7. **Progress Cancellation:** How to handle stopping mid-batch?
   - Simple: Just stop, leave already-processed folders in new state
   - Complex: Rollback all changes (requires undo tracking)
   - Hybrid: Stop cleanly after current folder completes
   - **Recommendation:** Hybrid approach (clean stop point, no rollback needed)

8. **Multi-Root Queue:** Should this be Phase 1 or Future Enhancement?
   - **Phase 1:** Just support single root with recursive subdirs (simpler, covers 90% of use cases)
   - **Future:** Add queue for processing multiple root folders (e.g., DH0:, DH1:, Work:)
   - **Recommendation:** Phase 1 = single root, Phase 2 = multi-root queue
   - Commodity (global hotkey)
   - Workbench tool type options

6. **Backward Compatibility:** Should CLI mode still exist?
   - Keep CLI for scripting/batch operations
   - GUI launches CLI backend
   - Both modes share same LayoutPreferences struct
