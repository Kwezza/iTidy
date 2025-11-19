# ListView Column Sorting Architecture

## Overview

This document details the architectural design for adding click-to-sort functionality to iTidy's ListView formatter module. The feature allows users to click column headers to sort data in ascending or descending order, with visual indicators showing the current sort state.

## Quick Reference: Core API

**Main Functions (call these):**
- `iTidy_FormatListViewColumns()` – Create formatted list + initial sort
- `iTidy_HandleListViewGadgetUp()` – **Unified IDCMP handler for ALL ListView events** (RECOMMENDED)
- `iTidy_HandleListViewSort()` – Handle header click + resort (high-level)
- `iTidy_ResortListViewByClick()` – Handle header click + resort (low-level)
- `iTidy_GetSelectedEntry()` – Map ListView row to entry (handles header offset)
- `iTidy_GetClickedColumn()` – Detect which column was clicked (pixel-based)
- `iTidy_GetListViewClick()` – Smart helper: Get entry + column + values + type
- `itidy_free_listview_entries()` – Free entry list, display list, state

**Data Structures:**
- `iTidy_ColumnConfig` – Column definition with `default_sort` (initial setting)
- `iTidy_ColumnState` – Runtime state with `sort_state` + `column_type` (current sort and type)
- `iTidy_ListViewEntry` – Entry with `display_data` + `sort_keys`
- `iTidy_ListViewState` – Overall state tracking for sorting
- `iTidy_ListViewClick` – Smart helper return struct with entry, column, values, type
- `iTidy_ListViewEvent` – **Unified event structure from GADGETUP handler**
- `iTidy_ListViewEventType` – **Event type enum (NONE/HEADER_SORTED/ROW_CLICK)**

### High-Level Sorting API (Recommended)

**`iTidy_HandleListViewSort()` - One-Call Sort Handler**

This is the recommended high-level function that handles all aspects of column sorting:

```c
BOOL iTidy_HandleListViewSort(
    struct Window *window,
    struct Gadget *listview_gadget,
    struct List *formatted_list,
    struct List *entry_list,
    iTidy_ListViewState *state,
    int mouse_x, int mouse_y,
    int font_height, int font_width,
    iTidy_ColumnConfig *columns,
    int num_columns
);
```

**What it does:**
1. Checks if click is within ListView header
2. Sets busy pointer during operation
3. Detects which column was clicked
4. Sorts entry list and rebuilds display
5. Refreshes the ListView gadget
6. Clears busy pointer
7. Returns TRUE if sorted, FALSE if click outside header

**Usage (replaces ~100 lines of boilerplate):**
```c
case IDCMP_MOUSEBUTTONS:
    if (code == SELECTDOWN) {
        if (iTidy_HandleListViewSort(
                window,
                data->session_listview,
                data->session_display_list,
                &data->session_entry_list,
                data->session_lv_state,
                msg->MouseX, msg->MouseY,
                prefsIControl.systemFontSize,  /* From global preferences */
                prefsIControl.systemFontCharWidth,
                session_columns, 4)) {
            /* List was sorted - done! */
        }
    }
    break;
```

**When to use:**
- ✅ Most common case - standard column sorting
- ✅ Want automatic busy pointer management
- ✅ Want automatic gadget refresh
- ✅ Prefer simplicity over control

**When to use low-level `iTidy_ResortListViewByClick()` instead:**
- Need custom busy pointer handling
- Need to perform actions between sort and refresh
- Building a custom UI flow

## Design Goals

1. **Reusability** - Any ListView in iTidy can use this sorting feature
2. **Performance** - Sorting should be instant even with 100+ entries
3. **Correctness** - Different data types (text, numbers, dates) must sort properly
4. **User Experience** - Clear visual feedback and intuitive interaction
5. **Maintainability** - Clean separation between display and sort logic

---

## Core Architecture

### Two-Function Design

The sorting system is built around two complementary functions:

#### 1. Initial Creation with Optional Default Sort
```c
struct List *iTidy_FormatListViewColumns(
    iTidy_ColumnConfig *columns,
    int num_columns,
    struct List *entries,          /* List of iTidy_ListViewEntry nodes */
    iTidy_ListViewState **out_state
);
```

**Purpose:** Create formatted ListView with optional initial sorting.

**Input:** A `struct List` containing pre-allocated `iTidy_ListViewEntry` nodes. Each entry must have:
- `display_data[]` - Human-friendly formatted strings for each column
- `sort_keys[]` - Machine-sortable keys for each column

**Behavior:**
- Checks if any column has `default_sort` set
- If yes, sorts entry List nodes **before** formatting (single pass!)
- Formats each entry into a display string (stored in `node.ln_Name`)
- Calculates column positions for hit-testing
- Returns state tracking structure for runtime sorting

**Note:** This is a change from the original `data_rows` approach. Using pre-built `iTidy_ListViewEntry` nodes allows the formatter to access both display and sort data without ambiguity.

#### 2. On-Demand Re-sorting
```c
BOOL iTidy_ResortListViewByClick(
    struct List *list,
    iTidy_ListViewState *state,
    int mouse_x,                   /* Window-relative X coordinate (e.g., imsg->MouseX) */
    int mouse_y,                   /* Window-relative Y coordinate (for header detection) */
    int header_top,                /* Y position of header row in window coordinates */
    int header_height,             /* Height of header row in pixels */
    int (*comparator)(const void*, const void*, int, iTidy_ColumnType)
);
```

**Purpose:** Resort existing ListView based on column click.

**Coordinate Requirements:**
- `mouse_x` and `mouse_y` are raw window coordinates (from `IntuiMessage`)
- `header_top` is the Y position where the header row begins (typically `gadget->TopEdge`)
- `header_height` is the pixel height of one text row (use `prefsIControl->ic_Font->tf_YSize`)
- All `pixel_start`/`pixel_end` values in `iTidy_ColumnState` are already in window coordinates

**Behavior:**
- First checks if click is within header Y-range (`mouse_y >= header_top && mouse_y < header_top + header_height`)
- Detects which column was clicked using mouse X position vs `pixel_start`/`pixel_end`
- Re-sorts List nodes in-place (no reformatting!)
- Updates sort state and visual indicators in header
- Rebuilds only the tag array for gadget update
- Returns TRUE if resorted, FALSE if click was outside header or invalid

---

## Data Structures

### Column Configuration (Extended)

```c
typedef enum {
    ITIDY_SORT_NONE,       /* Column not sorted */
    ITIDY_SORT_ASCENDING,  /* Sort ascending (A-Z, 0-9, oldest-newest) */
    ITIDY_SORT_DESCENDING  /* Sort descending (Z-A, 9-0, newest-oldest) */
} iTidy_SortOrder;

typedef enum {
    ITIDY_COLTYPE_TEXT,    /* String comparison */
    ITIDY_COLTYPE_NUMBER,  /* Numeric comparison */
    ITIDY_COLTYPE_DATE     /* Date comparison using sort keys */
} iTidy_ColumnType;

typedef struct {
    const char *title;
    int min_width;
    int max_width;
    iTidy_ColumnAlign align;
    BOOL flexible;
    BOOL is_path;
    iTidy_SortOrder default_sort;  /* NEW: Initial sort order */
    iTidy_ColumnType sort_type;    /* NEW: How to compare values */
} iTidy_ColumnConfig;
```

**Why `default_sort`?**
- Allows ListView to display pre-sorted on creation (e.g., newest sessions first)
- Avoids requiring external pre-sorting logic
- One-pass operation: sort then format

**Why `sort_type`?**
- Different data types require different comparison logic
- Text: alphabetical (`strcmp`)
- Numbers: numeric (`atoi` comparison)
- Dates: chronological (using YYYYMMDD_HHMMSS format)

### Column State Tracking

```c
typedef struct {
    int column_index;           /* Which column (0-based) */
    int char_start;             /* Character position where column starts */
    int char_end;               /* Character position where column ends */
    int char_width;             /* Width in characters */
    int pixel_start;            /* Pixel position (char_start × font_width) */
    int pixel_end;              /* Pixel position (char_end × font_width) */
    iTidy_SortOrder sort_state; /* Current sort state */
} iTidy_ColumnState;

typedef struct {
    iTidy_ColumnState *columns; /* Array of column states */
    int num_columns;            /* Number of columns */
    int separator_width;        /* Width of " | " separator (3 chars) */
} iTidy_ListViewState;
```

**Why track both character AND pixel positions?**
- **Character positions** - For extracting column text from formatted strings (if needed)
- **Pixel positions** - For hit-testing mouse clicks accurately

**Why track per-column sort state?**
- Allows visual indicators to show which column is sorted
- Supports toggling between ascending/descending on same column
- Enables "shift-click for secondary sort" in future (multi-column sorting)

### ListView Entry Structure

```c
typedef struct {
    struct Node node;              /* Embedded Node (ln_Name = formatted display) */
    const char **display_data;     /* Pretty formatted: "24-Nov-2025 15:19" */
    const char **sort_keys;        /* Machine-sortable: "20251124_151900" */
    int num_columns;               /* Number of columns */
} iTidy_ListViewEntry;
```

**Why store both display_data AND sort_keys?**

This is the **critical design decision** that makes sorting work correctly:

1. **Display data** - What users see (human-friendly)
   - Dates: `"24-Nov-2025 15:19"` ← Readable but sorts wrong alphabetically
   - Paths: `"PC:Workbench/../tool.info"` ← Abbreviated for display
   - Numbers: `"2"` ← No leading zeros

2. **Sort keys** - What we sort by (machine-friendly)
   - Dates: `"20251124_151900"` ← Sorts correctly as string
   - Paths: `"PC:Workbench/Tools/tool.info"` ← Full unabbreviated path
   - Numbers: `"2"` or `"00002"` ← Can be zero-padded or parsed

**Example Problem Without Sort Keys:**
```
Formatted dates (alphabetical sort):
  "01-Dec-2025"  ← Appears first (wrong!)
  "24-Nov-2025"  ← Appears second (wrong!)
  
Sort keys (correct chronological sort):
  "20251124_000000"  ← November 24 (correct!)
  "20251201_000000"  ← December 1 (correct!)
```

---

## Data Ownership & Lifetime

### Memory Management Rules

**Caller Responsibilities (Before Formatting):**
```c
/* 1. Allocate entry structures */
iTidy_ListViewEntry *entry = whd_malloc(sizeof(iTidy_ListViewEntry));

/* 2. Allocate column arrays */
entry->display_data = whd_malloc(sizeof(char*) * num_columns);
entry->sort_keys = whd_malloc(sizeof(char*) * num_columns);

/* 3. Allocate individual strings (caller owns these!) */
entry->display_data[0] = whd_malloc(strlen("24-Nov-2025 15:19") + 1);
strcpy(entry->display_data[0], "24-Nov-2025 15:19");

entry->sort_keys[0] = whd_malloc(strlen("20251124_151900") + 1);
strcpy(entry->sort_keys[0], "20251124_151900");

/* 4. Add to input list */
AddTail(&input_list, (struct Node*)entry);
```

**Formatter Responsibilities (During Formatting):**
```c
/* 1. Reads display_data to create formatted string */
/* 2. Allocates ln_Name for formatted display (e.g., "24-Nov-2025 15:19 |   2 | PC:...") */
/* 3. Does NOT duplicate or free display_data or sort_keys */
/* 4. Returns formatted list with ln_Name strings owned by formatter */
```

**Cleanup Sequence (Important Order!):**
```c
/* 1. Detach list from gadget */
GT_SetGadgetAttrs(lv_gad, win, NULL, GTLV_Labels, ~0, TAG_DONE);

/* 2. Free formatted list (frees ln_Name strings and List structure) */
iTidy_FreeFormattedList(list);

/* 3. Free state tracking */
iTidy_FreeListViewState(lv_state);

/* 4. Free entry structures and their strings (caller's responsibility!) */
struct Node *node;
while ((node = RemHead(&input_list))) {
    iTidy_ListViewEntry *entry = (iTidy_ListViewEntry*)node;
    for (int i = 0; i < entry->num_columns; i++) {
        whd_free(entry->display_data[i]);
        whd_free(entry->sort_keys[i]);
    }
    whd_free(entry->display_data);
    whd_free(entry->sort_keys);
    whd_free(entry);
}
```

**Key Points:**
- ✅ **Caller owns:** Entry structs, display_data arrays, sort_keys arrays, individual strings
- ✅ **Formatter owns:** `ln_Name` formatted strings, internal List structure
- ✅ **Never:** Free the list while attached to a gadget (causes crashes!)
- ✅ **Never:** Free entry strings before calling `iTidy_FreeFormattedList()` (formatter reads them during resort)

### Lifetime Diagram
```
[Caller creates entries] ──→ [Formatter reads & formats] ──→ [Sorting reads sort_keys] ──→ [Cleanup]
     ↓                              ↓                                                           ↓
  Entry structs               ln_Name allocated                                    Free ln_Name first
  display_data                (formatter owns)                                     Then free entries
  sort_keys                                                                        Then free strings
  (caller owns)
```

### Ownership of struct List Containers

**IMPORTANT CLARIFICATION:**

The `itidy_free_listview_entries()` function frees:
- ✅ All **nodes** in `entry_list` (iTidy_ListViewEntry structures)
- ✅ All **data** in nodes (display_data, sort_keys, strings, ln_Name)
- ✅ The **display_list** structure itself (via iTidy_FreeFormattedList)
- ✅ The **state** structure itself (via iTidy_FreeListViewState)
- ❌ **NOT** the `entry_list` struct List itself

**Why entry_list is different:**
- `entry_list` is typically a **stack-allocated** `struct List` in window data
- `display_list` is **heap-allocated** by the formatter (via whd_malloc)
- `state` is **heap-allocated** by the formatter (via whd_malloc)

**Rules:**
1. If you allocated a `struct List` with `whd_malloc()`, you must free it yourself AFTER calling `itidy_free_listview_entries()`
2. If you declared a `struct List` on the stack or embedded in a struct, do NOT free it
3. The function will empty the list (remove all nodes) but not free the container

**Example:**
```c
/* Stack-allocated entry list (most common) */
typedef struct {
    struct List entry_list;  /* Embedded - do NOT free */
    struct List *display_list;  /* Pointer - will be freed */
    iTidy_ListViewState *state;  /* Pointer - will be freed */
} WindowData;

WindowData *data = AllocVec(sizeof(WindowData), MEMF_CLEAR);
NewList(&data->entry_list);  /* Initialize stack list */

/* ... populate ... */

/* Cleanup */
itidy_free_listview_entries(&data->entry_list,  /* Empties, doesn't free */
                            data->display_list,  /* Frees the list */
                            data->state);         /* Frees the state */
data->display_list = NULL;
data->state = NULL;
/* entry_list still exists (embedded), just empty now */
```

---

## Coordinate Conventions

### Window Coordinate System

All positions are in **window inner coordinates** (relative to window's content area):

```c
/* Example ListView gadget positioned in window */
Gadget *lv_gad = CreateGadget(...);
lv_gad->LeftEdge = 10;   /* 10 pixels from window left */
lv_gad->TopEdge = 30;    /* 30 pixels from window top */

/* Column state pixel positions are relative to gadget */
iTidy_ColumnState.pixel_start = 0;     /* First column starts at gadget left edge */
iTidy_ColumnState.pixel_end = 150;     /* First column ends 150 pixels from gadget left */
```

### Canonical Hit-Testing Method

**OFFICIAL APPROACH:** Always use **gadget-relative pixel X** vs `pixel_start`/`pixel_end` for click detection.

**Character positions (`char_start`, `char_width`) are for text slicing only**, not for hit-testing. While you can convert pixel → char position, this adds unnecessary complexity and rounding errors.

```c
/* CANONICAL: Pixel-based hit testing (USE THIS) */
WORD local_x = mouse_x - gadget->LeftEdge;
for (int i = 0; i < num_columns; i++) {
    if (local_x >= state->columns[i].pixel_start && 
        local_x < state->columns[i].pixel_end) {
        /* Clicked column i */
        return i;
    }
}

/* Alternative char-based method (works, but NOT recommended) */
WORD char_pos = local_x / font_width;
for (int i = 0; i < num_columns; i++) {
    if (char_pos >= state->columns[i].char_start &&
        char_pos < state->columns[i].char_end) {
        return i;  /* Same result, more conversions */
    }
}
```

**Why pixel-based is canonical:**
- ✅ Direct comparison (no division/rounding)
- ✅ Works with proportional fonts
- ✅ Accurate to the pixel
- ✅ Matches Intuition coordinate system
- ✅ One less conversion step

### Header Click Detection

The header row is the **first line** of the ListView gadget itself:

```c
/* Detect if click is in header region */
int header_top = lv_gad->TopEdge;
int header_height = prefsIControl->ic_Font->tf_YSize;  /* One text row - from global system preferences */

if (imsg->MouseY >= header_top && imsg->MouseY < header_top + header_height) {
    /* Click is in header row - check which column */
    int local_x = imsg->MouseX - lv_gad->LeftEdge;  /* Adjust to gadget-relative */
    
    for (int i = 0; i < state->num_columns; i++) {
        if (local_x >= state->columns[i].pixel_start && 
            local_x < state->columns[i].pixel_end) {
            /* Clicked column i */
            sort_by_column(i);
            break;
        }
    }
}
```

**Key Points:**
- Header is the **first ListView entry** (rendered as part of the gadget)
- Y-range is exactly one text row tall
- X positions must be adjusted from window coords to gadget-relative coords
- Separator characters (`" | "`) are part of the clickable area of their preceding column

### Hit-Testing Example

```
Window:
┌─────────────────────────────────────┐
│  [ListView at (10, 30)]             │
│  Date/Time ^     | Changes | Path   │  ← Header row (Y: 30-38)
│  ────────────────┼─────────┼─────   │
│  24-Nov-2025     |       2 | PC:... │  ← Data rows (Y: 38+)
│  ...                                 │
└─────────────────────────────────────┘

Click at (100, 35):
  1. Y=35 is within [30, 38) → Header click ✓
  2. X=100, gadget left=10 → local_x = 90
  3. Check columns:
     - Col 0: pixel_start=0, pixel_end=60   → 90 NOT in [0, 60)
     - Col 1: pixel_start=60, pixel_end=90  → 90 NOT in [60, 90)
     - Col 2: pixel_start=90, pixel_end=200 → 90 in [90, 200) ✓
  4. Sort by column 2 (Path)
```

---

## Sorting Strategy

### Option 1: Re-sort and Rebuild Tag List (CHOSEN)

**Process:**
1. Sort the existing List nodes in-place (rearrange pointers)
2. Recreate the `STRPTR` array for `GT_SetGadgetAttrs()`
3. Call `GT_SetGadgetAttrs()` with new array

**Performance:**
- O(n log n) sort + O(n) array rebuild
- ~0.01s for 100 entries (ballpark estimate on 030/50 or accelerated A1200)
- No recalculation of widths, truncation, or formatting

**Implementation Note:** The sort **must be stable** (preserve original order for equal keys). Use merge sort or insertion sort, not quicksort, to guarantee stability.

**Advantages:**
- ✅ **Fast** - Reuses already-formatted strings
- ✅ **Clean** - Sorting separate from formatting
- ✅ **Memory efficient** - No duplication
- ✅ **Architecturally sound** - Single responsibility principle

### Option 2: Full Rebuild (REJECTED)

**Process:**
1. Sort the source data
2. Recalculate everything (widths, truncation, formatting)
3. Rebuild entire tag list

**Performance:**
- O(n log n) sort + O(n × columns) formatting
- ~0.05-0.1s for 100 entries (ballpark estimate on 030/50 or accelerated A1200)
- Wasteful recalculation

**Disadvantages:**
- ❌ **Slower** - Unnecessary formatting overhead
- ❌ **Complex** - Mixes sorting with formatting
- ❌ **Memory wasteful** - Creates new formatted strings

**Verdict:** Option 1 is superior in every way.

---

## Smart Comparison Logic

### Type-Aware Comparator

```c
int compare_by_column(iTidy_ListViewEntry *a, iTidy_ListViewEntry *b, 
                      int col, iTidy_ColumnType type)
{
    /* NOTE: This assumes non-NULL sort_keys; see "Empty or Missing Data" section
     * for NULL handling pattern. Production code must check for NULL before
     * calling strcmp() to avoid crashes. */
    
    switch (type) {
        case ITIDY_COLTYPE_TEXT:
            /* Alphabetical comparison */
            return strcmp(a->sort_keys[col], b->sort_keys[col]);
            
        case ITIDY_COLTYPE_NUMBER:
            /* Numeric comparison */
            return atoi(a->sort_keys[col]) - atoi(b->sort_keys[col]);
            
        case ITIDY_COLTYPE_DATE:
            /* Chronological comparison (YYYYMMDD_HHMMSS format) */
            return strcmp(a->sort_keys[col], b->sort_keys[col]);
    }
}
```

**Why this works:**

- **Text:** Standard string comparison
- **Numbers:** Parse as integers and subtract (handles "2" vs "10" correctly)
- **Dates:** String comparison works because format is YYYYMMDD_HHMMSS
  - `"20251124_151900"` < `"20251201_120000"` ✓

### Implementing Descending Order

Descending sort is handled by **negating the comparator result**. The sort algorithm itself always assumes "positive = a > b" for ascending order. To reverse the order, we simply flip the sign:

```c
/* In the merge sort comparison function */
int result = compare_by_column(a, b, col, type);

if (!ascending) {  /* If descending */
    result = -result;  /* Flip the comparison */
}

return result;
```

**Why negate instead of swapping a/b?**
- Clearer intent in code (explicit negation)
- Handles NULL checks consistently
- Works with all comparison types (text, number, date)
- Standard practice in sort implementations

**What NOT to do:**
- ❌ Don't reverse the list after sorting (loses stability)
- ❌ Don't swap a/b parameters (confusing, error-prone)
- ❌ Don't reverse during merge step (complex, fragile)
- ✅ Always negate at comparator level (simple, correct)

---

## Visual Sort Indicators

### Header Row Enhancement

When a column is sorted, its header shows a visual indicator:

```
Unsorted:  "Date/Time        | Changes | Path"
Ascending: "Date/Time ^      | Changes | Path"
Descending:"Date/Time v      | Changes | Path"
```

**Characters:**
- `^` = Ascending (up arrow approximation)
- `v` = Descending (down arrow)
- None = Unsorted

**Implementation:**
- Indicator added to title during header row generation
- Only the sorted column shows an indicator
- Indicator placed at end of title with one space padding

---

## Common Pitfalls (for AI Agents and Humans)

### ❌ Don't Do This

1. **Don't re-format rows on each sort**
   ```c
   /* WRONG - Wasteful! */
   iTidy_FreeFormattedList(list);
   list = iTidy_FormatListViewColumns(cols, 3, entries, &state);  /* Recalculates everything */
   
   /* RIGHT - Efficient! */
   iTidy_ResortListViewByClick(list, state, mouse_x, mouse_y, ...);  /* Just rearranges */
   ```

2. **Don't mutate list while sorting**
   ```c
   /* WRONG - Undefined behavior during sort! */
   while (sorting_in_progress) {
       AddTail(list, new_entry);  /* BAD! */
   }
   ```

3. **Don't free list while attached to gadget**
   ```c
   /* WRONG - Crashes! Gadget points to freed memory */
   iTidy_FreeFormattedList(list);
   
   /* RIGHT - Detach first */
   GT_SetGadgetAttrs(lv_gad, win, NULL, GTLV_Labels, ~0, TAG_DONE);
   iTidy_FreeFormattedList(list);
   ```

4. **Don't free entry data before formatter is done**
   ```c
   /* WRONG - Formatter needs sort_keys for sorting! */
   free_all_entry_strings();
   iTidy_ResortListViewByClick(list, ...);  /* Reads freed memory! */
   
   /* RIGHT - Keep entry data until final cleanup */
   iTidy_FreeFormattedList(list);      /* Formatter done now */
   free_all_entry_strings();           /* Safe to free */
   ```

5. **Don't assume quicksort stability**
   ```c
   /* WRONG - May reorder equal elements unpredictably */
   qsort(entries, count, sizeof(void*), compare_func);
   
   /* RIGHT - Use stable sort (merge sort, insertion sort) */
   stable_sort(entries, count, compare_func);
   ```

6. **Don't use raw MouseX for column detection**
   ```c
   /* WRONG - Doesn't account for gadget position */
   if (mouse_x >= col->pixel_start && mouse_x < col->pixel_end) ...
   
   /* RIGHT - Adjust to gadget-relative coordinates */
   int local_x = mouse_x - gadget->LeftEdge;
   if (local_x >= col->pixel_start && local_x < col->pixel_end) ...
   ```

---

## Usage Example

### Restore Window with Sortable Sessions

```c
/* 1. Define columns with sort metadata */
iTidy_ColumnConfig cols[] = {
    /* title, min, max, align, flex, path, default_sort, sort_type */
    {"Date/Time", 20, 20, ITIDY_ALIGN_LEFT, FALSE, FALSE, 
     ITIDY_SORT_DESCENDING, ITIDY_COLTYPE_DATE},  /* Newest first by default */
     
    {"Changes", 6, 6, ITIDY_ALIGN_RIGHT, FALSE, FALSE, 
     ITIDY_SORT_NONE, ITIDY_COLTYPE_NUMBER},
     
    {"Path", 30, 0, ITIDY_ALIGN_LEFT, TRUE, TRUE, 
     ITIDY_SORT_NONE, ITIDY_COLTYPE_TEXT}
};

/* 2. Build entry list with display and sort data */
struct List entry_list;
NewList(&entry_list);

for (i = 0; i < num_sessions; i++) {
    /* Allocate entry structure */
    iTidy_ListViewEntry *entry = whd_malloc(sizeof(iTidy_ListViewEntry));
    entry->num_columns = 3;
    
    /* Allocate column arrays */
    entry->display_data = whd_malloc(sizeof(char*) * 3);
    entry->sort_keys = whd_malloc(sizeof(char*) * 3);
    
    /* Allocate and populate display data (human-friendly) */
    entry->display_data[0] = whd_malloc(strlen("24-Nov-2025 15:19") + 1);
    strcpy(entry->display_data[0], "24-Nov-2025 15:19");
    
    entry->display_data[1] = whd_malloc(strlen("2") + 1);
    strcpy(entry->display_data[1], "2");
    
    entry->display_data[2] = whd_malloc(strlen("PC:Workbench/../tool.info") + 1);
    strcpy(entry->display_data[2], "PC:Workbench/../tool.info");
    
    /* Allocate and populate sort keys (machine-sortable) */
    entry->sort_keys[0] = whd_malloc(strlen("20251124_151900") + 1);
    strcpy(entry->sort_keys[0], "20251124_151900");
    
    entry->sort_keys[1] = whd_malloc(strlen("2") + 1);
    strcpy(entry->sort_keys[1], "2");
    
    entry->sort_keys[2] = whd_malloc(strlen("PC:Workbench/Tools/tool.info") + 1);
    strcpy(entry->sort_keys[2], "PC:Workbench/Tools/tool.info");
    
    /* Add to list */
    AddTail(&entry_list, (struct Node*)entry);
}

/* 3. Create formatted ListView (auto-sorted by Date descending) */
iTidy_ListViewState *lv_state = NULL;
struct List *formatted_list = iTidy_FormatListViewColumns(cols, 3, &entry_list, &lv_state);

/* 4. Attach to gadget */
GT_SetGadgetAttrs(lv_gad, win, NULL, GTLV_Labels, formatted_list, TAG_DONE);

/* 5. Store state in window data for event handling */
restore_data->listview_state = lv_state;
restore_data->entry_list = &entry_list;  /* Keep entries alive! */
restore_data->formatted_list = formatted_list;

/* 6. Handle mouse clicks in event loop */
if (imsg->Class == IDCMP_MOUSEBUTTONS && imsg->Code == SELECTDOWN) {
    int header_top = lv_gad->TopEdge;
    int header_height = prefsIControl->ic_Font->tf_YSize;
    
    if (iTidy_ResortListViewByClick(formatted_list, lv_state, 
                                     imsg->MouseX, imsg->MouseY,
                                     header_top, header_height,
                                     compare_func)) {
        /* Refresh gadget with re-sorted list */
        GT_SetGadgetAttrs(lv_gad, win, NULL, GTLV_Labels, formatted_list, TAG_DONE);
    }
}

/* 7. Cleanup (important order!) */
/* 7a. Detach from gadget first */
GT_SetGadgetAttrs(lv_gad, win, NULL, GTLV_Labels, ~0, TAG_DONE);

/* 7b. Free all ListView resources with centralized cleanup (RECOMMENDED) */
itidy_free_listview_entries(&entry_list,
                            formatted_list,
                            lv_state);
formatted_list = NULL;
lv_state = NULL;

/* Note: Manual cleanup is possible but NOT recommended:
 *   - Free formatted list: iTidy_FreeFormattedList(formatted_list)
 *   - Free state: iTidy_FreeListViewState(lv_state)
 *   - Free each entry: loop removing nodes, freeing ln_Name, display_data, sort_keys, entry
 * The centralized function handles all of this correctly. */
```

---

## Performance Analysis

**TL;DR:** For up to ~100 entries on a real 7MHz 68000, current implementation is fast enough. Don't optimize unless you reliably hit larger datasets or profiling shows a real bottleneck.

---

### Speed Comparison (100 entries)

| Operation | Option 1 (Resort Only) | Option 2 (Full Rebuild) |
|-----------|------------------------|-------------------------|
| Sort      | ~0.005s (O(n log n))   | ~0.005s (O(n log n))    |
| Format    | 0s (skip!)             | ~0.045s (recalculate)   |
| Array rebuild | ~0.005s (O(n))      | ~0.050s (recreate)      |
| **Total** | **~0.01s**             | **~0.10s**              |

**Does speed matter?**
- **Small lists (< 50):** Both feel instant
- **Large lists (100+):** Option 1 feels instant, Option 2 has noticeable lag
- **Perception:** Even 50ms lag feels sluggish on Amiga hardware

**Conclusion:** Option 1 is 10× faster and architecturally cleaner.

---

## Edge Cases and Considerations

### 1. Multiple Clicks on Same Column
**Behavior:** Toggle between ascending ↔ descending
```
First click:  Unsorted → Ascending
Second click: Ascending → Descending  
Third click:  Descending → Ascending
```

### 2. Clicking Different Columns
**Behavior:** Clear previous sort, apply new sort
```
Click Date:    Sort by Date ascending (Date shows ^)
Click Changes: Sort by Changes ascending (Changes shows ^, Date indicator cleared)
```

### 3. Empty or Missing Data (NULL or Empty Sort Keys)
**Behavior:** NULL or empty strings sort first (before any content)

**Sort Order:** `NULL` → `""` → `"A"` → `"Z"`

**Implementation:** Always check for NULL before calling `strcmp()`:
```c
if (a->sort_keys[col] == NULL && b->sort_keys[col] == NULL) return 0;
if (a->sort_keys[col] == NULL) return -1;  /* NULL sorts first */
if (b->sort_keys[col] == NULL) return 1;
return strcmp(a->sort_keys[col], b->sort_keys[col]);
```

**Safety:** Always check for NULL to avoid crashes

### 4. Identical Sort Keys
**Behavior:** Maintain original insertion order (stable sort)

**Implementation Note:** Use merge sort or insertion sort, NOT quicksort

### 5. Very Long Column Titles with Indicators
**Behavior:** Indicator may be truncated if title fills column width
**Solution:** Reserve 2 extra characters for " ^" or " v" when calculating minimum column width

---

## Smart Click Detection (Column + Entry Helpers)

### Overview

The ListView formatter provides **smart helper functions** that eliminate boilerplate code when handling ListView clicks. These helpers automatically:
- Detect which column was clicked (pixel-based hit-testing)
- Map ListView rows to actual entries (handles header offset)
- Extract both display and sort values
- Provide column type information (DATE/NUMBER/TEXT)

**Benefits:**
- ✅ **Reduces code from ~50 lines to ~10 lines** per click handler
- ✅ **Prevents header offset bugs** (automatic row-2 adjustment)
- ✅ **Type-aware processing** (column type provided automatically)
- ✅ **Dual values** (display + sort key in one call)
- ✅ **NULL-safe** (handles all edge cases gracefully)

### The iTidy_ListViewClick Structure

```c
typedef struct {
    iTidy_ListViewEntry *entry;       /* Selected entry (NULL if header row) */
    int column;                        /* Clicked column index (0-based, -1 if invalid) */
    const char *display_value;         /* Human-readable: "2.8 MB", "24-Nov-2025" */
    const char *sort_key;              /* Machine-readable: "0002949120", "20251124_151900" */
    iTidy_ColumnType column_type;      /* DATE/NUMBER/TEXT */
} iTidy_ListViewClick;
```

**Field Meanings:**

| Field | Description | NULL/-1 Meaning | Example Values |
|-------|-------------|-----------------|----------------|
| `entry` | Full entry pointer | Header row clicked | Valid `iTidy_ListViewEntry*` |
| `column` | Column index | Click outside bounds | `0`, `1`, `2`, `-1` |
| `display_value` | Formatted string | Invalid entry/column | `"2.8 MB"`, `"24-Nov-2025"` |
| `sort_key` | Machine value | Invalid entry/column | `"0002949120"`, `"20251124_151900"` |
| `column_type` | Data type | Never NULL | `ITIDY_COLTYPE_DATE`, `ITIDY_COLTYPE_NUMBER`, `ITIDY_COLTYPE_TEXT` |

### Smart Helper API: iTidy_GetListViewClick()

**Signature:**
```c
iTidy_ListViewClick iTidy_GetListViewClick(
    struct List *entry_list,        /* List of iTidy_ListViewEntry nodes */
    iTidy_ListViewState *state,     /* State from iTidy_FormatListViewColumns() */
    LONG listview_row,              /* Row index from GT_GetGadgetAttrs(GTLV_Selected) */
    WORD mouse_x,                   /* Window-relative X from IntuiMessage->MouseX */
    WORD gadget_left                /* Gadget's LeftEdge position */
);
```

**What it does:**
1. Maps `listview_row` to actual entry (handles header offset automatically)
2. Detects which column was clicked using `mouse_x` (pixel-based hit-testing)
3. Extracts both `display_value` and `sort_key` from the entry
4. Retrieves column type from state (DATE/NUMBER/TEXT)
5. Returns all information in one structure

**Usage Example:**
```c
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        LONG selected;
        GT_GetGadgetAttrs(gadget, window, NULL, GTLV_Selected, &selected, TAG_END);
        
        /* Get complete click information in one call */
        iTidy_ListViewClick click = iTidy_GetListViewClick(
            &data->entry_list,
            data->lv_state,
            selected,
            msg->MouseX,
            gadget->LeftEdge
        );
        
        /* Check if valid data row and column were clicked */
        if (click.entry != NULL && click.column >= 0) {
            /* Type-aware processing */
            switch (click.column_type) {
                case ITIDY_COLTYPE_DATE:
                    /* Use sort_key for machine-readable date */
                    timestamp = parse_date(click.sort_key);  /* "20251124_151900" */
                    show_calendar_popup(timestamp);
                    break;
                    
                case ITIDY_COLTYPE_NUMBER:
                    /* Use sort_key for accurate number */
                    int value = atoi(click.sort_key);  /* "0000000013" -> 13 */
                    show_number_editor(value);
                    break;
                    
                case ITIDY_COLTYPE_TEXT:
                    /* Use display_value for text */
                    copy_to_clipboard(click.display_value);
                    break;
            }
        }
    }
    break;
```

### Return Value Cases

#### Valid Data Cell Clicked
```c
iTidy_ListViewClick click = iTidy_GetListViewClick(...);

/* User clicked on Row 5, Column 2 (a number column) */
click.entry         = <valid pointer>
click.column        = 2
click.display_value = "10.0 KB"
click.sort_key      = "0000010240"
click.column_type   = ITIDY_COLTYPE_NUMBER
```

**Validation:** `if (click.entry != NULL && click.column >= 0)`

#### Header Row Clicked
```c
/* User clicked on Row 0 (column titles) or Row 1 (separator) */
click.entry         = NULL
click.column        = <varies, could be valid column or -1>
click.display_value = NULL
click.sort_key      = NULL
click.column_type   = ITIDY_COLTYPE_TEXT  /* Default */
```

**Validation:** `if (click.entry == NULL)` - Header row, ignore or handle separately

#### Click Outside Column Bounds
```c
/* User clicked on data row but in padding/separator area */
click.entry         = <valid pointer>
click.column        = -1
click.display_value = NULL
click.sort_key      = NULL
click.column_type   = ITIDY_COLTYPE_TEXT  /* Default */
```

**Validation:** `if (click.column == -1)` - Valid row but outside columns

#### NULL Parameters Passed
```c
/* Called with entry_list=NULL or state=NULL */
click.entry         = NULL
click.column        = -1
click.display_value = NULL
click.sort_key      = NULL
click.column_type   = ITIDY_COLTYPE_TEXT  /* Default */
```

**Validation:** Function handles NULL parameters gracefully, always returns safe defaults

### Column Detection Only: iTidy_GetClickedColumn()

If you only need to detect which column was clicked (without entry lookup):

**Signature:**
```c
int iTidy_GetClickedColumn(
    iTidy_ListViewState *state,     /* State from iTidy_FormatListViewColumns() */
    WORD mouse_x,                   /* Window-relative X from IntuiMessage->MouseX */
    WORD gadget_left                /* Gadget's LeftEdge position */
);
```

**Returns:**
- Column index (0-based) if click is within column bounds
- `-1` if click is outside all columns or state is NULL

**Usage:**
```c
int column = iTidy_GetClickedColumn(lv_state, msg->MouseX, gadget->LeftEdge);
if (column >= 0) {
    printf("Clicked column %d\n", column);
}
```

**When to use this over iTidy_GetListViewClick():**
- You only need column detection (no entry lookup needed)
- You're handling header clicks separately
- Performance-critical code (skips entry traversal)

### Real-World Test Results

From actual testing with 12 backup runs (5 columns):

**Column 0 (Run - NUMBER):**
```
Display Value: '16'
Sort Key:      '0000000016'
Column Type:   NUMBER
```

**Column 1 (Date/Time - DATE):**
```
Display Value: '06-Nov-2025 11:47'
Sort Key:      '20251106_114700'
Column Type:   DATE
```

**Column 2 (Folders - NUMBER):**
```
Display Value: '1'
Sort Key:      '0000000001'
Column Type:   NUMBER
```

**Column 3 (Size - NUMBER):**
```
Display Value: '10.0 KB'
Sort Key:      '0000010240'
Column Type:   NUMBER
```

**Column 4 (Status - TEXT):**
```
Display Value: 'Complete'
Sort Key:      'Complete'
Column Type:   TEXT
```

**Click Outside Columns:**
```
Entry:  <valid pointer>
Column: -1
```

**Header Click:**
```
Entry:  NULL
Column: <varies>
```

### Type-Aware Processing Patterns

#### Pattern 1: Copy to Clipboard
```c
if (click.entry && click.column >= 0) {
    /* Always use display_value for clipboard (human-readable) */
    copy_to_clipboard(click.display_value);
    show_notification("Copied: %s", click.display_value);
}
```

#### Pattern 2: Parsing by Type
```c
if (click.entry && click.column >= 0) {
    switch (click.column_type) {
        case ITIDY_COLTYPE_DATE:
            /* Parse YYYYMMDD_HHMMSS from sort_key */
            struct DateStamp ds;
            if (parse_timestamp(click.sort_key, &ds)) {
                show_date_details(&ds);
            }
            break;
            
        case ITIDY_COLTYPE_NUMBER:
            /* Parse as integer from sort_key */
            long value = atol(click.sort_key);  /* Zero-padded string -> number */
            show_number_chart(value);
            break;
            
        case ITIDY_COLTYPE_TEXT:
            /* Use display_value or sort_key (often identical for text) */
            show_text_editor(click.display_value);
            break;
    }
}
```

#### Pattern 3: Column-Specific Actions
```c
if (click.entry && click.column >= 0) {
    /* Combine column index and type for specific actions */
    if (click.column == 2 && click.column_type == ITIDY_COLTYPE_NUMBER) {
        /* Size column clicked - show breakdown */
        ULONG bytes = atol(click.sort_key);
        show_size_breakdown(bytes);
    }
}
```

### Critical Pitfalls to Avoid

#### ❌ PITFALL 1: Not Checking entry AND column
```c
/* WRONG - Only checks entry */
if (click.entry != NULL) {
    printf("%s\n", click.display_value);  /* CRASH if column=-1! */
}

/* CORRECT - Check both */
if (click.entry != NULL && click.column >= 0) {
    printf("%s\n", click.display_value);  /* Safe */
}
```

**Why:** Entry can be valid (user clicked data row) but column can be -1 (clicked padding area). In this case, `display_value` and `sort_key` are NULL.

#### ❌ PITFALL 2: Using display_value for Parsing
```c
/* WRONG - Parsing formatted display string */
if (click.column_type == ITIDY_COLTYPE_DATE) {
    parse_date(click.display_value);  /* "06-Nov-2025 11:47" - complex parsing! */
}

/* CORRECT - Use sort_key for machine format */
if (click.column_type == ITIDY_COLTYPE_DATE) {
    parse_date(click.sort_key);  /* "20251106_114700" - simple parsing! */
}
```

**Why:** `display_value` is formatted for humans (variable format), `sort_key` is formatted for machines (fixed format).

#### ❌ PITFALL 3: Using sort_key for Display
```c
/* WRONG - Showing machine format to user */
printf("Size: %s\n", click.sort_key);  /* Shows "0000010240" - confusing! */

/* CORRECT - Use display_value for UI */
printf("Size: %s\n", click.display_value);  /* Shows "10.0 KB" - readable! */
```

**Why:** `sort_key` is optimized for sorting (zero-padded, unformatted), `display_value` is optimized for readability.

#### ❌ PITFALL 4: Forgetting to Extract mouseX Before GT_ReplyIMsg()
```c
/* WRONG - mouseX invalid after reply */
struct IntuiMessage *msg = GT_GetIMsg(...);
GT_ReplyIMsg(msg);  /* msg is now invalid! */
iTidy_ListViewClick click = iTidy_GetListViewClick(..., msg->MouseX, ...);  /* CRASH! */

/* CORRECT - Extract before reply */
struct IntuiMessage *msg = GT_GetIMsg(...);
WORD mouseX = msg->MouseX;  /* Save before reply */
GT_ReplyIMsg(msg);
iTidy_ListViewClick click = iTidy_GetListViewClick(..., mouseX, ...);  /* Safe */
```

**Why:** After `GT_ReplyIMsg()`, the message structure is freed and accessing `msg->MouseX` causes undefined behavior.

#### ❌ PITFALL 5: Assuming column_type is Never NULL
```c
/* MOSTLY SAFE - column_type is always set to a valid value */
/* But still check entry/column first to avoid logic errors */

if (click.column_type == ITIDY_COLTYPE_DATE) {  /* BAD: No entry/column check */
    parse_date(click.sort_key);  /* sort_key is NULL if entry/column invalid! */
}

/* CORRECT - Check entry/column first */
if (click.entry && click.column >= 0) {
    if (click.column_type == ITIDY_COLTYPE_DATE) {
        parse_date(click.sort_key);  /* Safe - sort_key is valid */
    }
}
```

**Why:** `column_type` is always set (defaults to TEXT), but the values (`display_value`, `sort_key`) are NULL if `entry` or `column` are invalid.

### Performance Characteristics

**Cost per call:** ~0.5-1.0ms on 7MHz 68000

**Breakdown:**
- Column detection: O(num_columns) - typically 3-5 columns
- Entry lookup: O(selected_row) - walks list from start to selected row
- Value extraction: O(1) - direct pointer access
- Type lookup: O(1) - direct array access

**Typical case (row 5, 4 columns):**
- 5 list node traversals
- 4 pixel boundary checks
- Total: < 1ms on stock Amiga

**Worst case (row 100, 10 columns):**
- 100 list node traversals
- 10 pixel boundary checks
- Total: ~2-3ms on stock Amiga

**Optimization note:** If processing many clicks in a loop, cache the entry pointer and only call `iTidy_GetClickedColumn()` for each click.

### Integration Checklist

To use the smart helper in your window:

- [ ] **Include header:**
  ```c
  #include "GUI/listview_formatter.h"
  ```

- [ ] **Store state in window data:**
  ```c
  typedef struct {
      struct List entry_list;
      struct List *display_list;
      iTidy_ListViewState *lv_state;  /* REQUIRED for smart helper */
  } WindowData;
  ```

- [ ] **Capture state during formatting:**
  ```c
  data->display_list = iTidy_FormatListViewColumns(
      columns, num_columns, &data->entry_list, width, &data->lv_state
  );
  ```

- [ ] **Extract mouse coords in event loop (BEFORE GT_ReplyIMsg):**
  ```c
  WORD mouseX = msg->MouseX;
  WORD mouseY = msg->MouseY;
  GT_ReplyIMsg(msg);
  ```

- [ ] **Use helper in GADGETUP:**
  ```c
  iTidy_ListViewClick click = iTidy_GetListViewClick(
      &data->entry_list, data->lv_state, selected,
      mouseX, gadget->LeftEdge
  );
  
  if (click.entry && click.column >= 0) {
      /* Process valid click */
  }
  ```

- [ ] **Cleanup state on window close:**
  ```c
  iTidy_FreeListViewState(data->lv_state);
  ```

### Comparison: Old vs New Approach

**OLD WAY (Manual - ~50 lines):**
```c
case IDCMP_GADGETUP:
    GT_GetGadgetAttrs(gadget, window, NULL, GTLV_Selected, &selected, TAG_END);
    
    /* Manual header offset */
    if (selected < 2) return;
    int data_index = selected - 2;
    
    /* Manual list traversal */
    struct Node *node = entry_list.lh_Head;
    int current_index = 0;
    iTidy_ListViewEntry *entry = NULL;
    while (node->ln_Succ) {
        if (current_index == data_index) {
            entry = (iTidy_ListViewEntry *)node;
            break;
        }
        node = node->ln_Succ;
        current_index++;
    }
    
    /* Manual column detection */
    WORD local_x = msg->MouseX - gadget->LeftEdge;
    int column = -1;
    for (int i = 0; i < state->num_columns; i++) {
        if (local_x >= state->columns[i].pixel_start &&
            local_x < state->columns[i].pixel_end) {
            column = i;
            break;
        }
    }
    
    /* Manual type checking (caller maintains column->type mapping!) */
    if (entry && column == 0) {  /* Hardcoded: column 0 is date */
        parse_date(entry->sort_keys[0]);
    }
    break;
```

**NEW WAY (Smart Helper - ~10 lines):**
```c
case IDCMP_GADGETUP:
    GT_GetGadgetAttrs(gadget, window, NULL, GTLV_Selected, &selected, TAG_END);
    
    iTidy_ListViewClick click = iTidy_GetListViewClick(
        &entry_list, state, selected, mouseX, gadget->LeftEdge
    );
    
    if (click.entry && click.column >= 0) {
        if (click.column_type == ITIDY_COLTYPE_DATE) {
            parse_date(click.sort_key);
        }
    }
    break;
```

**Improvements:**
- ✅ 80% less code
- ✅ No manual header offset
- ✅ No manual list traversal
- ✅ No manual column detection
- ✅ Type information provided automatically
- ✅ Self-documenting code

---

## High-Level IDCMP Event Handler

### The Problem: 100+ Lines of Boilerplate

Each window using sortable ListViews needs to handle GADGETUP events with complex logic:
- Extract mouse coordinates BEFORE replying to IntuiMessage
- Detect if click was in header vs data row (Y-coordinate test)
- Calculate header bounds using font metrics
- Call different helpers for header vs row clicks
- Handle sorting and gadget refresh for header clicks
- Extract entry and column information for row clicks
- Manually switch between code paths

This results in ~100 lines of repetitive, error-prone code per window.

### The Solution: `iTidy_HandleListViewGadgetUp()`

A unified high-level handler that consolidates ALL ListView GADGETUP handling into a single function call, returning a unified event structure.

**Function Signature:**
```c
BOOL iTidy_HandleListViewGadgetUp(
    struct Window *window,
    struct Gadget *gadget,
    WORD mouse_x,
    WORD mouse_y,
    struct List *entry_list,
    struct List *display_list,
    iTidy_ListViewState *state,
    int font_height,
    int font_width,
    iTidy_ColumnConfig *columns,
    int num_columns,
    iTidy_ListViewEvent *out_event
);
```

**Event Types:**
```c
typedef enum {
    ITIDY_LV_EVENT_NONE = 0,        /* No valid event (click outside, error) */
    ITIDY_LV_EVENT_HEADER_SORTED,   /* User clicked header, list was sorted */
    ITIDY_LV_EVENT_ROW_CLICK        /* User clicked data row */
} iTidy_ListViewEventType;
```

**Event Structure:**
```c
typedef struct {
    iTidy_ListViewEventType type;   /* What happened? */
    BOOL did_sort;                  /* TRUE if list was resorted (caller must refresh) */
    
    /* Header event fields (type == HEADER_SORTED) */
    int sorted_column;              /* Which column was sorted */
    iTidy_SortOrder sort_order;     /* ASC or DESC */
    
    /* Row event fields (type == ROW_CLICK) */
    iTidy_ListViewEntry *entry;     /* Clicked entry (NULL if no entry) */
    int column;                     /* Clicked column (-1 if none) */
    const char *display_value;      /* Display text (NULL if invalid) */
    const char *sort_key;           /* Sort key (NULL if invalid) */
    iTidy_ColumnType column_type;   /* DATE, NUMBER, or TEXT */
} iTidy_ListViewEvent;
```

### Usage Pattern (Replaces ~100 Lines with ~20)

**BEFORE (Manual Approach - ~100 lines):**
```c
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        WORD mouseX = msg->MouseX;
        WORD mouseY = msg->MouseY;
        
        /* Manual header detection */
        struct Screen *screen = window->WScreen;
        WORD header_top = gadget->TopEdge;
        WORD header_height = screen->Font->ta_YSize;
        
        if (mouseY >= header_top && mouseY < header_top + header_height) {
            /* Header click - manual column detection */
            WORD local_x = mouseX - gadget->LeftEdge;
            int column = -1;
            for (int i = 0; i < state->num_columns; i++) {
                if (local_x >= state->columns[i].pixel_start &&
                    local_x < state->columns[i].pixel_end) {
                    column = i;
                    break;
                }
            }
            
            if (column >= 0) {
                /* Manual sort */
                iTidy_ResortListViewByClick(&entry_list, display_list,
                                            state, columns, num_columns,
                                            mouseX, mouseY, gadget->LeftEdge,
                                            gadget->TopEdge);
                
                /* Manual refresh */
                GT_SetGadgetAttrs(gadget, window, NULL,
                                 GTLV_Labels, ~0, TAG_DONE);
                GT_SetGadgetAttrs(gadget, window, NULL,
                                 GTLV_Labels, display_list, TAG_DONE);
            }
        } else {
            /* Data row click - manual entry lookup */
            LONG selected = -1;
            GT_GetGadgetAttrs(gadget, window, NULL,
                             GTLV_Selected, &selected, TAG_END);
            
            iTidy_ListViewClick click = iTidy_GetListViewClick(
                &entry_list, state, selected, mouseX, gadget->LeftEdge
            );
            
            if (click.entry && click.column >= 0) {
                /* Process click with manual type checking */
                if (click.column_type == ITIDY_COLTYPE_DATE) {
                    process_date(click.sort_key);
                } else if (click.column_type == ITIDY_COLTYPE_NUMBER) {
                    process_number(click.sort_key);
                }
            }
        }
    }
    break;
```

**AFTER (High-Level Handler - ~20 lines):**
```c
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        iTidy_ListViewEvent event;
        
        if (iTidy_HandleListViewGadgetUp(
                window, gadget, msg->MouseX, msg->MouseY,
                &entry_list, display_list, state,
                font->tf_YSize, font->tf_XSize,
                columns, num_columns, &event)) {
            
            /* Refresh if sorted */
            if (event.did_sort) {
                GT_SetGadgetAttrs(gadget, window, NULL,
                                 GTLV_Labels, ~0, TAG_DONE);
                GT_SetGadgetAttrs(gadget, window, NULL,
                                 GTLV_Labels, display_list, TAG_DONE);
            }
            
            /* Handle event */
            switch (event.type) {
                case ITIDY_LV_EVENT_HEADER_SORTED:
                    log_info(LOG_GUI, "Sorted by column %d, order %d\n",
                            event.sorted_column, event.sort_order);
                    break;
                    
                case ITIDY_LV_EVENT_ROW_CLICK:
                    if (event.entry && event.column >= 0) {
                        if (event.column_type == ITIDY_COLTYPE_DATE) {
                            process_date(event.sort_key);
                        } else if (event.column_type == ITIDY_COLTYPE_NUMBER) {
                            process_number(event.sort_key);
                        }
                    }
                    break;
            }
        }
    }
    break;
```

### What the Handler Does

**Automatic Processing:**
1. ✅ Validates all parameters (NULL checks)
2. ✅ Initializes event structure with safe defaults
3. ✅ Calculates header bounds (TopEdge + font_height)
4. ✅ **Calculates pixel positions from character positions** (char_start/end × font_width)
5. ✅ Detects header vs data row (Y-coordinate test)
6. ✅ Calls `iTidy_GetClickedColumn()` for header clicks (using pixel positions)
7. ✅ Calls `iTidy_ResortListViewByClick()` for sorting
8. ✅ Calls `GT_GetGadgetAttrs(GTLV_Selected)` for row clicks
9. ✅ Calls `iTidy_GetListViewClick()` for entry extraction
10. ✅ Fills event structure with all relevant information
11. ✅ Returns TRUE if event processed, FALSE on error

**What Caller Still Does:**
- Extract `mouseX/mouseY` BEFORE `GT_ReplyIMsg()` (required by GadTools)
- Check `event.did_sort` and refresh gadget if TRUE
- Switch on `event.type` and handle HEADER_SORTED or ROW_CLICK

### Key Design Decisions

**Why Handler Doesn't Auto-Refresh Gadget:**
- Caller may want to update other UI elements first
- Caller may want to batch multiple refreshes
- Caller may want to add animations or effects
- Preserves caller control over UI timing

**Why Pass `font_height` Instead of `struct TextFont*`:**
- Avoids font management complexity in handler
- Caller already has font open for ListView creation
- Simpler signature (one int vs struct pointer)
- Matches existing pattern in `iTidy_HandleListViewSort()`

**Why Use `GT_GetGadgetAttrs()` Instead of `msg->Code`:**
- More reliable (gets actual selected item from gadget state)
- Works even if selection changed between click and processing
- Consistent with GadTools best practices
- Eliminates timing-related bugs

### Critical Usage Notes

**⚠️ PITFALL 1: Extract Mouse Coordinates BEFORE GT_ReplyIMsg()**
```c
/* WRONG - coordinates invalid after reply */
GT_ReplyIMsg(msg);
iTidy_HandleListViewGadgetUp(..., msg->MouseX, msg->MouseY, ...);

/* RIGHT - extract before reply */
WORD mouseX = msg->MouseX;
WORD mouseY = msg->MouseY;
GT_ReplyIMsg(msg);
iTidy_HandleListViewGadgetUp(..., mouseX, mouseY, ...);
```

**⚠️ PITFALL 2: Always Refresh Gadget if did_sort==TRUE**
```c
/* WRONG - sorted list not visible */
if (iTidy_HandleListViewGadgetUp(..., &event)) {
    switch (event.type) { ... }  /* Forgot to check did_sort! */
}

/* RIGHT - refresh before processing */
if (iTidy_HandleListViewGadgetUp(..., &event)) {
    if (event.did_sort) {
        GT_SetGadgetAttrs(gadget, window, NULL, GTLV_Labels, ~0, TAG_DONE);
        GT_SetGadgetAttrs(gadget, window, NULL, GTLV_Labels, display_list, TAG_DONE);
    }
    switch (event.type) { ... }
}
```

**⚠️ PITFALL 3: Always Check event.type Before Accessing Fields**
```c
/* WRONG - assumes ROW_CLICK without checking */
if (iTidy_HandleListViewGadgetUp(..., &event)) {
    process_entry(event.entry);  /* May be NULL! */
}

/* RIGHT - switch on type first */
if (iTidy_HandleListViewGadgetUp(..., &event)) {
    switch (event.type) {
        case ITIDY_LV_EVENT_ROW_CLICK:
            if (event.entry) process_entry(event.entry);
            break;
        case ITIDY_LV_EVENT_HEADER_SORTED:
            /* sorted_column valid here */
            break;
    }
}
```

### Event Structure Field Validity

**Type: ITIDY_LV_EVENT_NONE**
- `did_sort` = FALSE
- All other fields are zeroed/NULL (safe to ignore)

**Type: ITIDY_LV_EVENT_HEADER_SORTED**
- `did_sort` = TRUE (always)
- `sorted_column` = valid column index (0-based)
- `sort_order` = ITIDY_SORT_ASC or ITIDY_SORT_DESC
- Row fields (entry, column, values, type) = undefined (do not use)

**Type: ITIDY_LV_EVENT_ROW_CLICK**
- `did_sort` = FALSE
- `entry` = valid entry pointer (never NULL for this type)
- `column` = valid column index (0-based, never -1 for this type)
- `display_value` = valid string pointer (may be empty string "")
- `sort_key` = valid string pointer (may be empty string "")
- `column_type` = ITIDY_COLTYPE_DATE, ITIDY_COLTYPE_NUMBER, or ITIDY_COLTYPE_TEXT
- Header fields (sorted_column, sort_order) = undefined (do not use)

### Performance Characteristics

**Function Call Overhead:**
- Single function call vs ~100 lines of inline code
- No performance difference (compiler can inline if desired)

**Memory Overhead:**
- Event structure: 40 bytes on stack (temporary)
- No heap allocations
- No performance impact

**Execution Path:**
- Header click: 2 helper calls + sorting + rebuild
- Row click: 2 helper calls (no sorting)
- Same performance as manual approach (just cleaner)

### Integration Checklist

When converting existing code to use the high-level handler:

- [ ] Extract `mouseX` and `mouseY` BEFORE `GT_ReplyIMsg()`
- [ ] Replace header detection code with handler call
- [ ] Replace sorting code with handler call
- [ ] Replace row click code with handler call
- [ ] Add `if (event.did_sort)` check and gadget refresh
- [ ] Add `switch (event.type)` statement
- [ ] Move header processing to `HEADER_SORTED` case
- [ ] Move row processing to `ROW_CLICK` case
- [ ] Remove old coordinate calculation code
- [ ] Remove old column detection loops
- [ ] Remove old manual sorting calls
- [ ] Remove old manual entry lookup code
- [ ] Test header clicks (verify sorting)
- [ ] Test row clicks (verify entry/column extraction)
- [ ] Test clicks outside bounds (verify NONE event)

### Benefits

**Code Reduction:**
- Before: ~100 lines per window
- After: ~20 lines per window
- **80% reduction in client code**

**Maintainability:**
- Centralized logic (fix bugs once, benefit everywhere)
- Self-documenting (event structure describes what happened)
- Type-safe (enum prevents invalid event types)

**Reliability:**
- All coordinate conversions handled correctly
- All NULL checks performed automatically
- Consistent behavior across all windows

**Flexibility:**
- Caller controls gadget refresh timing
- Caller can add custom processing per event type
- Easy to extend with new event types in future

---

## Future Enhancements

### Multi-Column Sorting (Phase 2)
- Shift+Click to add secondary sort columns
- Visual indicators show sort priority (1, 2, 3...)
- Example: Sort by Date (primary), then by Path (secondary)

### Custom Sort Functions (Phase 2)
- Allow caller to provide custom comparator per column
- Useful for domain-specific sorting (e.g., version numbers)

### Sort State Persistence (Phase 3)
- Save sort column and direction to preferences
- Restore sort state when window reopens

---

## Implementation Checklist

### Phase 1: Core Functionality
- [ ] Add `iTidy_SortOrder` and `iTidy_ColumnType` enums to header
- [ ] Extend `iTidy_ColumnConfig` with `default_sort` and `sort_type` fields
- [ ] Create `iTidy_ColumnState` and `iTidy_ListViewState` structures
- [ ] Define `iTidy_ListViewEntry` structure with `display_data` and `sort_keys`
- [ ] Update `iTidy_FormatListViewColumns()` signature to accept `struct List *entries` and `out_state` parameter
- [ ] Implement stable sort (merge sort or insertion sort) for initial sorting
- [ ] Add logic to detect and apply `default_sort` during formatting
- [ ] Calculate and store column pixel/character positions in `iTidy_ColumnState`
- [ ] Implement `iTidy_ResortListViewByClick()` function with coordinate parameters
- [ ] Add Y-range checking for header click detection
- [ ] Add X-range checking with gadget-relative coordinate adjustment
- [ ] Implement type-aware comparator with NULL checks
- [ ] Add visual sort indicators (`^` / `v`) to header row generation
- [ ] Implement `iTidy_FreeListViewState()` cleanup function
- [ ] Add comprehensive NULL checks throughout sorting code

### Phase 2: Integration
- [ ] Create helper function to build `iTidy_ListViewEntry` from session data
- [ ] Update Restore Window to allocate and populate entry list
- [ ] Update Restore Window to store `listview_state` in window data
- [ ] Add IDCMP_MOUSEBUTTONS handling with header Y-range detection
- [ ] Add coordinate adjustment (window → gadget-relative)
- [ ] Update Folder View Window to use new sorting API (if applicable)
- [ ] Test with various data types (dates, numbers, paths, text)
- [ ] Test with NULL/empty sort keys
- [ ] Verify performance with 100+ entries on target hardware
- [ ] Verify memory cleanup with no leaks

### Phase 3: Polish
- [ ] Add comprehensive documentation comments to all new functions
- [ ] Document data ownership and lifetime rules in header
- [ ] Document coordinate conventions in header
- [ ] Update examples in header files with complete allocation/cleanup
- [ ] Test edge cases (empty lists, null data, single entry, etc.)
- [ ] Test all three column types (TEXT, NUMBER, DATE)
- [ ] Test stability of sort with duplicate values
- [ ] Profile memory usage and verify no leaks
- [ ] Add debug logging for sort operations (optional)
- [ ] Verify behavior on different Amiga models (A1200, A4000, etc.)

---

## Implementation Corrections and Lessons Learned

**NOTE:** This section documents critical bugs and issues discovered during actual implementation. **READ THIS FIRST** before implementing ListView sorting.

### CRITICAL BUG #1: Pointer-to-Pointer in CreateGadget

**THE BUG:**
```c
/* WRONG - Passes address-of-pointer instead of pointer */
gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
    GTLV_Labels, &data->session_display_list,  /* CRASH! Wrong pointer type */
    TAG_DONE);
```

**WHAT HAPPENS:**
- GadTools reads `&data->session_display_list` as a memory address
- That address points to the **pointer variable**, not the **list itself**
- ListView interprets random memory as list nodes
- Result: Garbage data, crashes, or blank ListView

**THE FIX:**
```c
/* CORRECT - Pass the list pointer itself */
gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
    GTLV_Labels, data->session_display_list,  /* Correct! */
    TAG_DONE);
```

**WHY THIS IS CRITICAL:**
- This bug caused **every sort operation to appear broken**
- Sort was working correctly, but ListView was reading garbage memory
- Data was sorted properly, but display showed wrong order
- **Check EVERY CreateGadget call with GTLV_Labels tag!**

---

### CRITICAL BUG #2: Memory Leak in Entry List Cleanup

**THE BUG:**
```c
/* Incomplete cleanup - leaks ln_Name field */
iTidy_ListViewEntry *entry;
while ((entry = (iTidy_ListViewEntry *)RemHead(entry_list)) != NULL)
{
    /* Free display_data and sort_keys arrays */
    for (int col = 0; col < num_cols; col++)
    {
        whd_free(entry->display_data[col]);
        whd_free(entry->sort_keys[col]);
    }
    whd_free(entry->display_data);
    whd_free(entry->sort_keys);
    whd_free(entry);  /* BUG: Forgot to free entry->node.ln_Name! */
}
```

**WHAT HAPPENS:**
- Each entry allocates `entry->node.ln_Name` with `whd_malloc(strlen(name) + 1)`
- ln_Name is never freed
- **Result: 100+ memory leaks (92 bytes each) for typical session**

**THE FIX:**
```c
/* Complete cleanup - free ALL allocations */
iTidy_ListViewEntry *entry;
while ((entry = (iTidy_ListViewEntry *)RemHead(entry_list)) != NULL)
{
    /* Free ln_Name FIRST (allocated separately) */
    if (entry->node.ln_Name != NULL)
    {
        whd_free(entry->node.ln_Name);
        entry->node.ln_Name = NULL;
    }
    
    /* Free display_data and sort_keys arrays */
    for (int col = 0; col < num_cols; col++)
    {
        whd_free(entry->display_data[col]);
        whd_free(entry->sort_keys[col]);
    }
    whd_free(entry->display_data);
    whd_free(entry->sort_keys);
    
    /* Finally free the entry itself */
    whd_free(entry);
}

/* Free the lists themselves */
whd_free(entry_list);
whd_free(display_list);
```

**ACTUAL LEAK NUMBERS (from memory tracking):**
```
LEAK: 5 allocations of 92 bytes at listview_formatter.c:181
LEAK: 1 allocation of 556 bytes at listview_formatter.c:116 (entry->display_data array)
LEAK: 1 allocation of 556 bytes at listview_formatter.c:127 (entry->sort_keys array)
Total: 100+ leaks, ~3000+ bytes leaked per window open/close
```

**WHY THIS MATTERS:**
- Memory tracking shows **exactly** where leaks occur (file:line)
- **Always check memory reports after implementing sorting**
- Use `DEBUG_MEMORY_TRACKING` during development

---

### CRITICAL BUG #3: Sort Indicators Not Showing

**HISTORICAL NOTE:** This bug applied to an older design where we re-ran `iTidy_FormatListViewColumns()` on each header click (the "reformat everything" approach). In the **current design (Option 1: resort only)**, header indicators are updated without reformatting all rows by rebuilding just the tag array. However, the sequencing lesson still applies: sort state must be set **before** any formatting/rendering operation.

**THE BUG:**
```c
/* Wrong order: Format BEFORE setting sort state */
itidy_format_listview_columns(columns, num_cols, entry_list, ...);

/* Set sort state AFTER formatting */
columns[clicked_col].default_sort = new_order;  /* TOO LATE! */
```

**WHAT HAPPENS:**
- Formatter reads `default_sort` field from **ColumnConfig** during formatting
- Sort state updated **after** formatting completes
- Headers generated without arrow indicators
- Next click shows **previous** column's arrow (off-by-one bug)

**THE FIX:**
```c
/* Set default_sort in ColumnConfig FIRST (formatter reads this!) */
for (int i = 0; i < num_columns; i++)
{
    columns[i].default_sort = ITIDY_SORT_NONE;  /* Clear all arrows */
}
columns[clicked_col].default_sort = new_order;  /* Set clicked column arrow */

/* NOW format with updated config */
itidy_format_listview_columns(columns, num_cols, entry_list, ...);
```

**KEY INSIGHT:**
- **ColumnConfig.default_sort** = Initial sort setting (formatter reads this)
- **ColumnState.sort_state** = Runtime state (formatter writes this from default_sort)
- Formatter copies `columns[col].default_sort` → `state->columns[col].sort_state`
- Think of formatter as a "render" function that reads config, writes state

---

### PERFORMANCE ISSUE: Verbose Logging on 7MHz Amiga

**THE PROBLEM:**
```c
/* Debug logging during sort operations */
log_debug(LOG_GUI, "Comparing col %d: '%s' vs '%s'\n", col, a_key, b_key);
log_debug(LOG_GUI, "Merge sort completed: %d entries\n", total);
```

**WHAT HAPPENS:**
- Each log call writes to disk (sync I/O on AmigaOS)
- Sorting 50 entries = 1000+ log writes
- On 7MHz Amiga with slow floppy/HD: **2-3 seconds per click**
- User perceives sorting as "broken" or "hanging"

**THE FIX:**
```c
/* Remove ALL verbose debug logging from sort/comparison functions */
/* Use logging ONLY for errors or one-time events */
```

**PERFORMANCE RESULT:**
- Before: 2-3 seconds per click on 7MHz A1200
- After: **Instant** (< 100ms) on same hardware
- **20x-30x speedup** by removing disk I/O from hot path

**LESSON LEARNED:**
- Debug logging is expensive on vintage hardware
- Use logging to **understand** behavior during development
- **Remove** verbose logging before release
- Keep error logging only

---

### UI BUG #4: Header Truncation with Sort Indicators

**THE PROBLEM:**
```c
/* Original column width calculation */
max_len = max(max_len, strlen(columns[col].title));  /* Exact fit */
```

**WHAT HAPPENS:**
- Column width exactly fits title: `"Changed"` = 7 chars
- Sort indicator adds 2 chars: `"Changed v"` = 9 chars
- Title truncated to `"Change v"` (7 chars total)
- User cannot read full column name

**THE FIX:**
```c
/* Reserve space for sort indicator */
WORD title_length = strlen(columns[col].title);
max_len = max(max_len, title_length + 2);  /* +2 for " ^" or " v" */
```

**RESULT:**
- All column titles display fully with sort indicators
- No truncation on any column

---

### UI BUG #5: Header Alignment Mismatch

**THE PROBLEM:**
```c
/* Hardcoded left alignment for headers */
itidy_format_text_field(header_buffer + offset, 
                        columns[col].char_width,
                        columns[col].title,
                        ITIDY_ALIGN_LEFT);  /* WRONG for numeric columns */
```

**WHAT HAPPENS:**
- "Changed" column header is left-aligned
- "Changed" column data is right-aligned (numbers)
- Visual misalignment between header and data

**THE FIX:**
```c
/* Use column's alignment for header */
itidy_format_text_field(header_buffer + offset,
                        columns[col].char_width,
                        columns[col].title,
                        columns[col].align);  /* Match data alignment */
```

**RESULT:**
- "Changed" header right-aligned (matches right-aligned numbers)
- "Path" header left-aligned (matches left-aligned text)
- Consistent visual hierarchy

---

### IMPLEMENTATION PITFALL: Mouse Position Detection

**NOTE:** This section reflects the **earlier char-based approach** we initially used. The **current canonical solution uses pixel-based `pixel_start`/`pixel_end`** as documented in "Canonical Hit-Testing Method" above. The boundary-search principles below still apply, but prefer pixel coordinates throughout.

**ORIGINAL ASSUMPTION:**
"Click detection is simple - just check if mouse X is within gadget bounds."

**REALITY:**
Mouse position detection requires **four coordinate transformations:**

1. **Window coordinates** → Gadget-relative coordinates
   ```c
   WORD mouse_x = window->MouseX;
   WORD gadget_left = listview_gadget->LeftEdge;
   WORD gadget_relative_x = mouse_x - gadget_left;
   ```

2. **Pixel coordinates** → Character position
   ```c
   WORD char_pos = gadget_relative_x / font_char_width;
   ```

3. **Character position** → Column search
   ```c
   /* Cannot do direct math - must search column boundaries */
   for (int col = 0; col < num_cols; col++)
   {
       if (char_pos >= columns[col].char_start &&
           char_pos < columns[col].char_start + columns[col].char_width)
       {
           clicked_column = col;
           break;
       }
   }
   ```

4. **Boundary edge cases**
   ```c
   /* Click in separator or padding between columns */
   if (clicked_column == -1)
   {
       /* Not in any column - ignore click */
       return;
   }
   ```

**KEY LESSON:**
- Column detection is **more complex** than it appears
- Requires font metrics, gadget geometry, and boundary checking
- **Cannot** use simple division or modulo arithmetic
- **Must** search column boundaries with char_start/char_width

---

### CODE QUALITY: Complete Cleanup Example

**CORRECT cleanup sequence (reverse of creation):**

```c
void cleanup_restore_window(RestoreWindowData *data)
{
    /* 1. Detach lists from gadgets FIRST */
    if (data->session_display_list != NULL)
    {
        GT_SetGadgetAttrs(data->session_gadget, data->window, NULL,
                         GTLV_Labels, ~0,  /* Detach list */
                         TAG_DONE);
    }
    
    if (data->changes_display_list != NULL)
    {
        GT_SetGadgetAttrs(data->changes_gadget, data->window, NULL,
                         GTLV_Labels, ~0,  /* Detach list */
                         TAG_DONE);
    }
    
    /* 2. Close window BEFORE freeing gadgets */
    if (data->window != NULL)
    {
        CloseWindow(data->window);
        data->window = NULL;
    }
    
    /* 3. Free gadgets after window closed */
    if (data->gadget_list != NULL)
    {
        FreeGadgets(data->gadget_list);
        data->gadget_list = NULL;
    }
    
    /* 4. Free visual info */
    if (data->vi != NULL)
    {
        FreeVisualInfo(data->vi);
        data->vi = NULL;
    }
    
    /* 5. Free entry lists (COMPLETE cleanup) */
    if (data->session_entry_list != NULL)
    {
        iTidy_ListViewEntry *entry;
        while ((entry = (iTidy_ListViewEntry *)RemHead(data->session_entry_list)) != NULL)
        {
            /* Free ln_Name FIRST */
            if (entry->node.ln_Name != NULL)
            {
                whd_free(entry->node.ln_Name);
            }
            
            /* Free display_data and sort_keys arrays */
            for (int col = 0; col < data->session_state->num_columns; col++)
            {
                whd_free(entry->display_data[col]);
                whd_free(entry->sort_keys[col]);
            }
            whd_free(entry->display_data);
            whd_free(entry->sort_keys);
            
            /* Finally free entry */
            whd_free(entry);
        }
        whd_free(data->session_entry_list);
        whd_free(data->session_display_list);
    }
    
    /* 6. Free listview state */
    if (data->session_state != NULL)
    {
        itidy_free_listview_state(data->session_state);
        data->session_state = NULL;
    }
    
    /* 7. Free window data itself */
    whd_free(data);
}
```

**CRITICAL POINTS:**
- Detach lists from gadgets **before** closing window
- Close window **before** freeing gadgets (gadgets attached to window)
- Free ln_Name **before** freeing entry (separate allocation)
- Free arrays **before** freeing parent structures
- Check for NULL on **every** pointer before freeing

---

### TESTING CHECKLIST (Based on Real Bugs Found)

When implementing ListView sorting, **test these specific scenarios:**

1. **Visual Verification:**
   - [ ] Click each column header - does data reorder correctly?
   - [ ] Are arrow indicators visible on sorted column?
   - [ ] Are arrows cleared when clicking different column?
   - [ ] Do column titles display fully (no truncation with arrows)?
   - [ ] Are numeric columns right-aligned (header and data)?

2. **Memory Testing (with DEBUG_MEMORY_TRACKING):**
   - [ ] Open window, close window - any leaks?
   - [ ] Click column 5 times - leaks growing?
   - [ ] Check memory log for ln_Name leaks (92 bytes each)
   - [ ] Check for display_data/sort_keys array leaks (100s of bytes)
   - [ ] Verify zero leaks after complete cleanup

3. **Performance Testing (on target hardware):**
   - [ ] Sort 50 entries - is response instant (< 500ms)?
   - [ ] Remove verbose debug logging if sorting feels slow
   - [ ] Test on slowest target hardware (7MHz A500/A1200)

4. **Edge Cases:**
   - [ ] Sort empty list (0 entries) - crash?
   - [ ] Sort single entry list - behaves correctly?
   - [ ] Sort with NULL sort keys - crash or sort correctly?
   - [ ] Sort with empty string sort keys - correct order?
   - [ ] Multiple rapid clicks - stable or crash?

5. **CreateGadget Verification:**
   - [ ] Double-check EVERY ListView uses `list` NOT `&list` in GTLV_Labels
   - [ ] This is the **#1 most common bug** - verify before testing anything else

---

## Conclusion

This architecture provides:
- **Efficient sorting** through pointer manipulation (no reformatting)
- **Correct sorting** via type-aware comparisons and sort keys
- **Clean design** with separation of concerns
- **Reusability** across all iTidy ListViews
- **Extensibility** for future enhancements

The dual-data approach (display_data + sort_keys) is the key insight that makes this work correctly for dates, paths, and formatted numbers while maintaining human-friendly display.

**IMPLEMENTATION WARNING:** The "Corrections and Lessons Learned" section above documents **critical bugs** found during actual implementation. These bugs caused 100+ memory leaks, performance degradation, and UI issues that were not obvious during design phase. **Read and verify each correction** before implementing sorting in your own code.

---

## IMPLEMENTED: Centralized Cleanup Function

**Date:** 2025-11-19  
**Status:** ✅ IMPLEMENTED (tested, no memory leaks)  
**Priority:** Medium (code quality improvement)

### Problem Statement

Currently, each window using ListViews must manually free all entry list resources:
- `entry->node.ln_Name` (easily forgotten, caused 100+ leaks)
- `entry->display_data[col]` arrays (per column)
- `entry->sort_keys[col]` arrays (per column)
- `entry->display_data` array pointer
- `entry->sort_keys` array pointer
- Entry itself
- Entry list structure
- Display list structure
- ListView state structure

**Risk:** Complex cleanup sequence is error-prone and duplicated across multiple windows.

### Implemented Solution

Added cleanup function to `src/GUI/listview_formatter.c`:

```c
/**
 * Free all resources associated with a ListView entry list.
 * 
 * @param entry_list    List of iTidy_ListViewEntry nodes (can be NULL)
 * @param display_list  Display list for ListView gadget (can be NULL)
 * @param state         ListView state with column info (can be NULL)
 * 
 * @note Handles NULL parameters gracefully.
 * @note Frees ALL allocations: ln_Name, display_data, sort_keys, arrays, entries, lists, state.
 */
void itidy_free_listview_entries(struct List *entry_list, 
                                  struct List *display_list,
                                  iTidy_ListViewState *state);
```

### Implementation Details

**What it frees (in correct order):**
1. For each entry in `entry_list`:
   - `entry->node.ln_Name` (formatted display string allocated by formatter)
   - `entry->display_data[col]` for each column
   - `entry->sort_keys[col]` for each column
   - `entry->display_data` array
   - `entry->sort_keys` array
   - `entry` structure itself
2. `display_list` (via `iTidy_FreeFormattedList()`)
3. `state` (via `iTidy_FreeListViewState()`)

**Window cleanup reduced from ~30 lines to 3 lines:**
```c
/* Before: ~30 lines of error-prone manual cleanup */
iTidy_ListViewEntry *entry;
while ((entry = (iTidy_ListViewEntry *)RemHead(entry_list)) != NULL)
{
    if (entry->node.ln_Name) whd_free(entry->node.ln_Name);  /* Easy to forget! */
    for (int col = 0; col < num_cols; col++)
    {
        whd_free(entry->display_data[col]);
        whd_free(entry->sort_keys[col]);
    }
    whd_free(entry->display_data);
    whd_free(entry->sort_keys);
    whd_free(entry);
}
if (display_list) iTidy_FreeFormattedList(display_list);
if (state) iTidy_FreeListViewState(state);

/* After: 3 lines (plus NULL assignments) */
itidy_free_listview_entries(&data->entry_list,
                            data->display_list,
                            data->state);
data->display_list = NULL;
data->state = NULL;
```

### Benefits (Achieved)

✅ **Single source of truth** - cleanup logic centralized in formatter module  
✅ **Symmetry** - formatter creates data structures, formatter destroys them  
✅ **Error prevention** - impossible to forget `ln_Name` or other allocations  
✅ **Code reuse** - Used in 3 locations in default_tool_restore_window.c  
✅ **Maintainability** - future fields added to `iTidy_ListViewEntry` only need cleanup in one place  
✅ **Null safety** - function handles NULL parameters gracefully  
✅ **Zero memory leaks** - Tested with DEBUG_MEMORY_TRACKING, no leaks detected  

### Final Design Decisions

✅ **Function name:** `itidy_free_listview_entries()` (chosen for clarity)  
✅ **Parameter handling:** All three parameters accepted, NULL-safe for each  
✅ **Gadget integration:** Caller must detach before calling (keeps separation of concerns)  
✅ **Single-level cleanup:** One function frees everything (no need for two-level approach)  

**Window responsibilities (unchanged):**
- Detaching lists from gadgets (`GT_SetGadgetAttrs` with `GTLV_Labels, ~0`) **BEFORE** calling cleanup
- Closing window before freeing gadgets
- Freeing gadget list
- Freeing visual info

### Implementation Completed

✅ **Function added** to `src/GUI/listview_formatter.c` (117 lines with documentation)  
✅ **Declaration added** to `src/GUI/listview_formatter.h` (56 lines with full API docs)  
✅ **Restore Window updated** - Replaced 3 manual cleanup sections with function calls  
✅ **Memory tracking verified** - Zero leaks with DEBUG_MEMORY_TRACKING enabled  
✅ **Compilation verified** - Clean build with no new warnings  
✅ **Runtime tested** - No crashes, proper cleanup confirmed  
✅ **Documentation updated** - This section and DEVELOPMENT_LOG.md  

### Usage Example (Final)

```c
/* Proper cleanup sequence in window close function */
void cleanup_restore_window(iTidy_ToolRestoreData *data)
{
    /* 1. Detach lists from gadgets FIRST (CRITICAL!) */
    if (data->session_listview && data->window) {
        GT_SetGadgetAttrs(data->session_listview, data->window, NULL,
                         GTLV_Labels, ~0,  /* Detach */
                         TAG_DONE);
    }
    
    /* 2. Close window BEFORE freeing gadgets */
    if (data->window) {
        CloseWindow(data->window);
        data->window = NULL;
    }
    
    /* 3. Free gadgets */
    if (data->gadget_list) {
        FreeGadgets(data->gadget_list);
        data->gadget_list = NULL;
    }
    
    /* 4. Free visual info */
    if (data->visual_info) {
        FreeVisualInfo(data->visual_info);
        data->visual_info = NULL;
    }
    
    /* 5. Free all ListView resources (NEW - single call!) */
    itidy_free_listview_entries(&data->session_entry_list,
                                data->session_display_list,
                                data->session_lv_state);
    data->session_display_list = NULL;
    data->session_lv_state = NULL;
}
```

**See also:** `docs/DEVELOPMENT_LOG.md` (2025-11-18 entry on ListView sorting implementation)

---

## Performance Timing Analysis (7MHz 68000)

**Date:** 2025-11-19  
**Status:** ✅ MEASURED (instrumented with timer.device)  
**Priority:** Low (optimization data for future work)

### Instrumentation

Performance timing added to all critical ListView functions using AmigaOS `timer.device`:
- `iTidy_FormatListViewColumns()` - Main formatter with checkpoints
- `iTidy_ResortListViewByClick()` - Re-sort on column click
- `iTidy_SortListViewEntries()` - Merge sort algorithm
- `iTidy_CalculateColumnWidths()` - Column width calculation

**Timing methodology:**
```c
struct timeval startTime, endTime;
ULONG elapsedMicros, elapsedMillis;

GetSysTime(&startTime);
/* ... operation ... */
GetSysTime(&endTime);

elapsedMicros = ((endTime.tv_secs - startTime.tv_secs) * 1000000) +
                (endTime.tv_micro - startTime.tv_micro);
```

**Conditional logging:** Only active when `enable_performance_logging` flag is enabled via Beta Options Window.

### Test Results Summary

#### Test Environment
- **Hardware:** WinUAE emulating 7MHz 68000 (A1200 chipset, no cache)
- **Display:** Workbench 3.2, XEN.font 11pt, 7-pixel character width
- **ListView:** 4 columns (Date/Time, Mode, Path, Changes), 78 characters wide

#### Small Dataset (5 entries)

| Operation | Total Time | Breakdown | % |
|-----------|-----------|-----------|---|
| **Initial Load** | 160-200ms | | |
| - Sort | 27-43ms | | 16-23% |
| - Width calc | 106-133ms | | 62-69% |
| - Entry format | 21-27ms | | 12-14% |
| **Per entry cost** | **33-40ms** | | |
| **Actual work estimate** | ~30-35ms | (without timing overhead) | |

#### Medium Dataset (15 entries)

| Operation | Total Time | Breakdown | % |
|-----------|-----------|-----------|---|
| **Initial Load** | 295-370ms | | |
| - Sort | 65-130ms | | 20-35% |
| - Width calc | 165-197ms | | 47-60% |
| - Entry format | 49-88ms | | 15-25% |
| **Per entry cost** | **20-25ms** | | |
| **Actual work estimate** | ~50-80ms | (without timing overhead) | |

### Key Findings

#### 1. **Timing Overhead is Significant**
Each `GetSysTime()` call on 7MHz 68000:
- Requires system call to `timer.device`
- Performs context switch to exec.library
- Estimated cost: **20-50ms per call**

**Overhead breakdown:**
- `iTidy_FormatListViewColumns()`: 5 GetSysTime calls (start, 3 checkpoints, end)
- `iTidy_SortListViewEntries()`: 2 GetSysTime calls (nested in format)
- `iTidy_CalculateColumnWidths()`: 2 GetSysTime calls (nested in format)
- **Total per operation: ~200-250ms of pure timing overhead**

#### 2. **Scaling Behavior is Excellent**
| Metric | 5 Entries | 15 Entries | Scaling Factor |
|--------|-----------|------------|----------------|
| Total time | 160-200ms | 295-370ms | **1.8x** (3x entries) |
| Per-entry cost | 33-40ms | 20-25ms | **0.6x (faster!)** |

**Analysis:**
- **Near-linear scaling:** 3x more entries = 1.8x more time
- **Improved efficiency:** Per-entry cost **decreased by 40%** with larger dataset
- **Fixed overhead dominance:** Timing overhead amortized across more entries

#### 3. **Actual Bottlenecks Identified**

**Standalone measurements (low timing overhead):**
- Width calculation: **1.5-7.7ms** (not 165-197ms!)
- Sort operation: **8-9ms** for 5 entries (not 150ms!)
- Entry formatting: **3-6ms per entry** (actual string work)

**Real bottleneck:** Width calculation loop (O(n × columns))
```c
for (col = 0; col < num_columns; col++) {
    for (entry in entries) {
        measure_text_length(entry->display_data[col]);
    }
}
```
- 15 entries × 4 columns = 60 iterations
- Each `strlen()` or `TextLength()` call is costly on 7MHz
- Still only **~3.5-7.7ms** of actual work

#### 4. **Sort Algorithm Performance**
- **Algorithm:** Merge sort (stable, O(n log n))
- **5 entries:** 8-9ms standalone, ~27-43ms with overhead
- **15 entries:** ~65-130ms with overhead
- **Per-entry work:** ~7.5-8.6ms per entry (consistent)
- **Behavior:** Correct O(n log n) scaling, variance due to system load

#### 5. **String Operations Scaling**
- Entry formatting (sprintf, AbbreviatePath): **3-6ms per entry**
- Scales linearly with entry count
- Variation due to path length and abbreviation complexity

### Performance Recommendations

#### Production Performance (Timing Disabled)
**Estimated real-world performance:**
- **15 entries:** ~50-80ms total (without timing overhead)
- **50 entries:** ~150-250ms (predicted)
- **100 entries:** ~300-500ms (predicted)

**User experience:**
- ✅ **Under 500ms:** Feels instant to users
- ✅ **Current performance:** Totally acceptable for production
- ✅ **No optimization needed** unless targeting 100+ entries regularly

#### Optimization Opportunities (If Needed)

**Low-hanging fruit (if targeting 100+ entries):**

1. **Width Calculation Caching**
   ```c
   /* Current: Recalculates on every format */
   for (col = 0; col < num_columns; col++) {
       max_len = 0;
       for (entry in entries) {
           max_len = max(max_len, strlen(entry->display_data[col]));
       }
   }
   
   /* Optimized: Cache max lengths per column */
   if (!cached) {
       calculate_column_widths();  /* Once */
       cache_widths = TRUE;
   }
   ```
   **Potential savings:** 30-40% for large datasets

2. **String Length Caching in Entry**
   ```c
   typedef struct {
       const char *text;
       WORD text_length;  /* Pre-calculated strlen() */
   } CachedString;
   ```
   **Potential savings:** 10-20% for width calculation

3. **Reduce GetSysTime Calls**
   - Remove checkpoint timings (cp1, cp2, cp3)
   - Keep only start/end per function
   **Savings:** ~60-70% reduction in timing overhead (for profiling builds)

#### When NOT to Optimize

- ❌ **Don't optimize** if typical usage is < 50 entries
- ❌ **Don't cache** if memory is tight (adds overhead)
- ❌ **Don't micro-optimize** string operations (already fast)
- ✅ **Current performance is excellent** for target use cases

### Timing Overhead Impact

**Summary:** The act of measuring performance **halves execution speed**:
- With timing: 295-370ms (15 entries)
- Without timing: ~50-80ms (estimated)
- **Overhead ratio: 4x-6x slowdown**

**Why this is acceptable:**
- Timing only enabled during profiling (Beta Options Window)
- Relative performance still valid (percentages accurate)
- Absolute timings reveal bottlenecks despite overhead
- User never sees timing overhead in production

### User Experience: Busy Pointer Feedback

**Problem:** On slow 7MHz Amiga hardware, loading and sorting operations can take 1-2+ seconds. During this time, the screen appears frozen with no visual feedback, making users think the application has crashed.

**Solution:** Use `SetWindowPointer()` to show busy pointer during blocking operations.

#### Implementation Pattern

```c
/* At start of expensive operation */
if (window) {
    SetWindowPointer(window, WA_BusyPointer, TRUE, TAG_DONE);
}

/* Perform expensive operation (loading, sorting, formatting) */
// ... iTidy_FormatListViewColumns() or iTidy_ResortListViewByClick() ...

/* After operation completes */
if (window) {
    SetWindowPointer(window, WA_BusyPointer, FALSE, TAG_DONE);
}
```

#### Where to Apply Busy Pointer

**CRITICAL LOCATIONS:**

1. **Initial ListView Population** (e.g., `populate_session_list()`)
   ```c
   static void populate_session_list(WindowData *data)
   {
       /* Set busy pointer IMMEDIATELY */
       if (data->window) {
           SetWindowPointer(data->window, WA_BusyPointer, TRUE, TAG_DONE);
       }
       
       /* Scan backups, build entries, format ListView */
       count = ScanBackupSessions(&data->session_list);
       // ... build entry list ...
       display_list = iTidy_FormatListViewColumns(columns, 4, &entry_list, ...);
       
       /* Update gadget */
       GT_SetGadgetAttrs(data->listview, data->window, NULL,
                         GTLV_Labels, display_list,
                         TAG_DONE);
       
       /* Clear busy pointer AFTER ListView is visible */
       if (data->window) {
           SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
       }
   }
   ```

2. **Column Click Sorting** (in IDCMP_MOUSEBUTTONS handler)
   ```c
   case IDCMP_MOUSEBUTTONS:
       if (code == SELECTDOWN && gadget_is_listview) {
           /* Set busy pointer BEFORE sorting */
           if (window) {
               SetWindowPointer(window, WA_BusyPointer, TRUE, TAG_DONE);
           }
           
           /* Attempt to sort */
           did_sort = iTidy_ResortListViewByClick(
               display_list, entry_list, state,
               mouse_x, mouse_y, header_top, header_height,
               gadget_left, font_width, columns
           );
           
           /* Update gadget if sorted */
           if (did_sort) {
               GT_SetGadgetAttrs(listview, window, NULL, GTLV_Labels, ~0, TAG_DONE);
               GT_SetGadgetAttrs(listview, window, NULL, GTLV_Labels, display_list, TAG_DONE);
           }
           
           /* Clear busy pointer AFTER sort completes (sorted or not) */
           if (window) {
               SetWindowPointer(window, WA_BusyPointer, FALSE, TAG_DONE);
           }
       }
       break;
   ```

#### Benefits

✅ **Immediate visual feedback** - User sees pointer change instantly  
✅ **Prevents "frozen" perception** - User knows system is working  
✅ **Standard AmigaOS pattern** - Users expect this behavior  
✅ **No code complexity** - Simple 2-line addition before/after operations  
✅ **Works on all Amiga hardware** - From 7MHz A500 to accelerated systems  

#### Performance Impact

- **SetWindowPointer() overhead:** ~1-2ms (negligible)
- **User perception improvement:** Massive (feels responsive instead of frozen)
- **When to skip:** Only if operation is guaranteed < 100ms (instant)

**RECOMMENDATION:** Always use busy pointer for ListView operations. Even on fast hardware, it's good UX practice and costs almost nothing.

#### Real-World Results

**Before (no busy pointer):**
- User: "Is it frozen? Should I click again?"
- Experience: Blank screen for 1-2 seconds
- Perception: Application crashed or hung

**After (with busy pointer):**
- User: "It's working, I'll wait"
- Experience: Immediate visual feedback
- Perception: Application is responsive and professional

**See also:** `src/GUI/default_tool_restore_window.c` for complete implementation example.

---

### Testing Methodology

**For accurate profiling:**
1. Enable "Performance Timing Logs" in Beta Options Window
2. Perform operations (open window, sort columns)
3. Review `Bin/Amiga/logs/gui_*.log` for timing data
4. **Focus on percentages** (overhead affects all operations equally)
5. **Compare standalone vs checkpoint timings** to isolate real work
6. Disable timing for production use

### Conclusion

**Performance Status:** ✅ **Production Ready**
- ListView sorting is fast enough for real-world Amiga hardware
- Timing reveals width calculation as primary bottleneck (still acceptable)
- Sort algorithm scales correctly (O(n log n) verified)
- No optimization needed unless targeting 100+ entry datasets

**Timing System Status:** ✅ **Effective for Profiling**
- Successfully identified bottlenecks despite overhead
- Provides actionable data for future optimization
- Conditional logging prevents production slowdown
- Percentages remain valid despite absolute timing overhead

**Next Steps (Optional):**
- If targeting 100+ entries: Implement width caching
- If memory allows: Cache string lengths in entry structures
- If profiling frequently: Reduce checkpoint calls to minimize overhead

**Files Instrumented:**
- `src/GUI/listview_formatter.c` (all timing code)
- `src/writeLog.c/h` (performance logging API)
- `docs/PERFORMANCE_TIMING_INVESTIGATION.md` (detailed analysis)

---

## CRITICAL: ListView Selection with Header Rows

### Problem: ListView Row Selection Offset Bug

**Issue:** When implementing ListView item selection handlers, you may encounter an off-by-two bug where clicking on a visual row selects the wrong data entry. For example, clicking the 3rd visible row returns data from the 1st entry in your array.

**Root Cause:** The sortable ListView formatter adds **two header rows** at the top of the ListView:
1. **Row 0:** Column titles with sort indicators (e.g., `"Date/Time ^     | Run | Size"`)
2. **Row 1:** Separator line (e.g., `"────────────────┼─────┼─────"`)
3. **Row 2+:** Actual data entries

When GadTools returns `GTLV_Selected`, it returns the **absolute row index** including these header rows. Your code must account for this offset when mapping to your data structures.

### ❌ WRONG Implementation (Off-by-Two Bug)

```c
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        GT_GetAttrs(gadget, GTLV_Selected, &selected, TAG_DONE);
        
        /* BUG: Treats selected as direct array index */
        if (selected >= 0 && selected < num_entries) {
            show_details(my_data_array[selected]);  /* WRONG! Off by 2 rows */
        }
    }
    break;
```

**Why this fails:**
- User clicks visual row 3 (first data row after headers)
- GadTools returns `selected = 2` (row index 2)
- Code accesses `my_data_array[2]` (3rd entry in array)
- **Result:** Shows data for entry 2 when user clicked entry 0

### ✅ CORRECT Implementation (Using Helper Function)

The `listview_formatter` module provides `iTidy_GetSelectedEntry()` to handle this automatically:

```c
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        GT_GetAttrs(gadget, GTLV_Selected, &selected, TAG_DONE);
        
        /* Use helper function to get correct entry */
        iTidy_ListViewEntry *entry = iTidy_GetSelectedEntry(&entry_list, selected);
        
        if (entry != NULL) {
            /* Extract data from entry and show details */
            int item_id = extract_id_from_entry(entry);  /* Your logic */
            show_details(my_data_array, item_id);
        } else {
            /* User clicked header row or invalid index */
            clear_details_panel();
        }
    }
    break;
```

**What the helper does:**
1. **Checks for header rows:** Returns `NULL` if `selected < 2` (row 0 or 1)
2. **Adjusts index:** Subtracts 2 to get actual data index (`data_index = selected - 2`)
3. **Traverses sorted list:** Walks the sorted entry list to find the Nth entry
4. **Returns entry pointer:** Direct access to `iTidy_ListViewEntry` with all data

### Helper Function API

```c
/**
 * Get the selected entry from a formatted ListView, accounting for header rows.
 * 
 * @param entry_list    List of iTidy_ListViewEntry nodes (sorted order)
 * @param listview_row  Row index from GTLV_Selected (absolute, includes headers)
 * @return Entry pointer if valid data row, NULL if header row or invalid
 * 
 * @note Row 0 = header titles, Row 1 = separator, Row 2+ = data
 * @note Automatically handles header offset (subtracts 2)
 * @note Returns NULL for clicks on header rows (row < 2)
 */
iTidy_ListViewEntry *iTidy_GetSelectedEntry(struct List *entry_list, LONG listview_row);
```

**Include in your code:**
```c
#include "GUI/listview_formatter.h"  /* For iTidy_GetSelectedEntry() */
```

### Why Use the Helper?

**Benefits:**
- ✅ **Automatic header offset handling** - No manual subtraction needed
- ✅ **Null safety** - Returns NULL for header rows (< 2) or invalid indices
- ✅ **Sorted list aware** - Traverses actual sorted list, not original order
- ✅ **Reusable** - Works for any ListView using the formatter
- ✅ **Less code** - Reduces caller from ~50 lines to ~10 lines

**Without helper (manual approach):**
```c
/* Complex manual logic (error-prone) */
if (selected < 2) {
    /* Header row clicked */
    return;
}

int data_index = selected - 2;  /* Manual offset */

/* Walk sorted list manually */
struct Node *node = entry_list->lh_Head;
int current_index = 0;
while (node->ln_Succ) {
    if (current_index == data_index) {
        iTidy_ListViewEntry *entry = (iTidy_ListViewEntry *)node;
        /* Now process entry */
        break;
    }
    node = node->ln_Succ;
    current_index++;
}
```

**With helper (simple):**
```c
/* One-line retrieval */
iTidy_ListViewEntry *entry = iTidy_GetSelectedEntry(&entry_list, selected);
if (entry != NULL) {
    /* Process entry */
}
```

### Real-World Example

From `src/GUI/restore_window.c` (lines ~1895-1920):

```c
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_RESTORE_RUN_LIST) {
        GT_GetAttrs(gadget, GTLV_Selected, &selected, TAG_DONE);
        
        /* Use helper to get selected entry (handles header offset) */
        iTidy_ListViewEntry *selected_lv_entry = 
            iTidy_GetSelectedEntry(&restore_data->run_entry_list, selected);
        
        if (selected_lv_entry != NULL) {
            /* Extract runNumber from entry's display data */
            unsigned int runNumber = atoi(selected_lv_entry->display_data[0]);
            
            /* Find matching entry in data array */
            for (int i = 0; i < count; i++) {
                if (run_entries[i].runNumber == runNumber) {
                    /* Update details panel with correct data */
                    update_details_panel(&run_entries[i]);
                    break;
                }
            }
        } else {
            /* User clicked header row - clear details */
            clear_details_panel();
        }
    }
    break;
```

### Debugging Checklist

If selection shows wrong data:

1. **Check for header offset:** Are you subtracting 2 from `GTLV_Selected`?
2. **Use helper function:** Call `iTidy_GetSelectedEntry()` instead of manual indexing
3. **Verify NULL handling:** Check if helper returns NULL (header click)
4. **Log selected values:** Debug with `append_to_log("Selected: %d, Entry: %p\n", selected, entry);`
5. **Check data mapping:** Ensure you're extracting correct ID from entry to find in your array

### Summary

**Golden Rule:** For sortable ListViews with headers, **always use `iTidy_GetSelectedEntry()`** to map `GTLV_Selected` to actual data entries. Never use the row index directly as an array index.

**Pattern:**
```c
/* 1. Get selected row from gadget */
GT_GetAttrs(gadget, GTLV_Selected, &selected, TAG_DONE);

/* 2. Get actual entry (handles headers, sorting, NULL) */
iTidy_ListViewEntry *entry = iTidy_GetSelectedEntry(&entry_list, selected);

/* 3. Extract your data and process */
if (entry != NULL) {
    /* Your logic here */
}
```

---

## CRITICAL: GadTools ListView Event Handling

### Problem: IDCMP_MOUSEBUTTONS Events Not Received

**Issue:** When implementing sortable ListViews, you may find that `IDCMP_MOUSEBUTTONS` events are **NOT** generated when clicking on the ListView gadget's header row, even though the window's IDCMP flags include `IDCMP_MOUSEBUTTONS`.

**Root Cause:** GadTools ListView gadgets **consume all mouse events** that occur within their bounds, including clicks on the header row. Instead of generating `IDCMP_MOUSEBUTTONS` events, they only generate `IDCMP_GADGETUP` events.

**Evidence:**
- Clicking **on** the ListView (including header): Only `IDCMP_GADGETUP` events
- Clicking **outside** the ListView bounds: Both `IDCMP_GADGETUP` and `IDCMP_MOUSEBUTTONS` events
- This is **intentional GadTools behavior**, not a bug

### ❌ WRONG Implementation (Will Not Work)

```c
case IDCMP_MOUSEBUTTONS:
    /* This code will NEVER execute for clicks on ListView! */
    if (msgCode == SELECTDOWN) {
        iTidy_HandleListViewSort(window, listview_gadget, ...);
    }
    break;

case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        /* Process item selection only */
        handle_item_selection();
    }
    break;
```

**Why this fails:** The `IDCMP_MOUSEBUTTONS` handler is never called when clicking within the ListView bounds.

### ✅ CORRECT Implementation (Handle in GADGETUP)

```c
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        /* Detect if click was in header row */
        if (listview_state != NULL) {
            WORD header_top = listview_gadget->TopEdge;
            WORD header_height = font_height;  /* e.g., prefsIControl.systemFontSize */
            
            if (mouseY >= header_top && mouseY < header_top + header_height) {
                /* Click was in header - handle sorting */
                append_to_log("GADGETUP: Click in header area (%d,%d) - handling sort\n",
                             mouseX, mouseY);
                
                if (iTidy_HandleListViewSort(
                        window, listview_gadget,
                        display_list, &entry_list, listview_state,
                        mouseX, mouseY,
                        font_height, font_width,
                        columns, num_columns)) {
                    append_to_log("List sorted by column click\n");
                }
                break;  /* Don't process as item selection */
            }
        }
        
        /* Not in header - handle normal item selection */
        handle_item_selection();
    }
    break;
```

**Key points:**
1. **Extract mouseX/mouseY** from the `IntuiMessage` at the top of the event loop
2. **Check Y-coordinate** to determine if click is in header row
3. **Call sorting function** if in header, then `break` to skip item selection
4. **Process normally** if not in header (regular list item click)

### Mouse Coordinate Extraction

**CRITICAL:** You must extract mouse coordinates **before** replying to the message:

```c
BOOL handle_window_events(WindowData *data) {
    struct IntuiMessage *imsg;
    ULONG msgClass;
    UWORD msgCode;
    struct Gadget *gadget;
    WORD mouseX, mouseY;  /* Declare these */
    
    while ((imsg = GT_GetIMsg(data->window->UserPort)) != NULL) {
        msgClass = imsg->Class;
        msgCode = imsg->Code;
        gadget = (struct Gadget *)imsg->IAddress;
        mouseX = imsg->MouseX;  /* Extract BEFORE GT_ReplyIMsg */
        mouseY = imsg->MouseY;
        
        GT_ReplyIMsg(imsg);  /* Reply first, then use extracted values */
        
        switch (msgClass) {
            /* Now mouseX/mouseY are available in all handlers */
        }
    }
}
```

### Debugging Checklist

If sorting doesn't work:

1. **Check event flags:** Window must include `IDCMP_GADGETUP`
   ```c
   WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | ..., TAG_END
   ```

2. **Verify coordinate extraction:** Log mouseX/mouseY in GADGETUP handler
   ```c
   append_to_log("GADGETUP: mouseX=%d, mouseY=%d, TopEdge=%d\n",
                mouseX, mouseY, gadget->TopEdge);
   ```

3. **Check header calculation:** Ensure header_top matches actual gadget position
   ```c
   WORD header_top = listview_gadget->TopEdge;  /* Correct */
   WORD header_top = 20;  /* WRONG - hardcoded value won't work */
   ```

4. **Verify font height:** Use actual system font height, not guessed value
   ```c
   WORD header_height = prefsIControl.systemFontSize;  /* Correct */
   WORD header_height = 11;  /* WRONG - may not match actual font */
   ```

### Real-World Example

See `src/GUI/restore_window.c` for a complete working implementation:
- Lines ~1854-1895: GADGETUP handler with header detection
- Lines ~1826-1831: Mouse coordinate extraction from IntuiMessage
- Lines ~2128-2177: Vestigial MOUSEBUTTONS handler (not used for ListView)

### Summary

**Golden Rule:** For GadTools ListView sorting, **always handle sorting in IDCMP_GADGETUP**, never in IDCMP_MOUSEBUTTONS.

---

````
