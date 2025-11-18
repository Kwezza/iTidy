# ListView Column Sorting Architecture

## Overview

This document details the architectural design for adding click-to-sort functionality to iTidy's ListView formatter module. The feature allows users to click column headers to sort data in ascending or descending order, with visual indicators showing the current sort state.

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

### Header Click Detection

The header row is the **first line** of the ListView gadget itself:

```c
/* Detect if click is in header region */
int header_top = lv_gad->TopEdge;
int header_height = prefsIControl->ic_Font->tf_YSize;  /* One text row */

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

/* 7b. Free formatted list (formatter's responsibility) */
iTidy_FreeFormattedList(formatted_list);

/* 7c. Free state tracking */
iTidy_FreeListViewState(lv_state);

/* 7d. Free entry data (caller's responsibility) */
struct Node *node;
while ((node = RemHead(&entry_list))) {
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

---

## Performance Analysis

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

### 3. Clicking Different Columns
**Behavior:** Clear previous sort, apply new sort
```
Click Date:    Sort by Date ascending (Date shows ^)
Click Changes: Sort by Changes ascending (Changes shows ^, Date indicator cleared)
```

### 4. Empty or Missing Data
**Behavior:** NULL or empty strings sort first (before any content)
**Sort Order:** `NULL` → `""` → `"A"` → `"Z"`
**Implementation:** Always check for NULL before calling `strcmp()`:
```c
if (a->sort_keys[col] == NULL && b->sort_keys[col] == NULL) return 0;
if (a->sort_keys[col] == NULL) return -1;  /* NULL sorts first */
if (b->sort_keys[col] == NULL) return 1;
return strcmp(a->sort_keys[col], b->sort_keys[col]);
```

### 5. Identical Sort Keys
**Behavior:** Maintain original insertion order (stable sort)
**Implementation Note:** Use merge sort or insertion sort, NOT quicksort

### 5. Very Long Column Titles with Indicators
**Behavior:** Indicator may be truncated if title fills column width
**Solution:** Reserve 2 extra characters for " ^" or " v" when calculating minimum column width

### 6. Null or Empty Sort Keys
**Behavior:** NULL or empty string sort keys are treated as "less than" any content
**Sort Order:** `NULL` → `""` → `"A"` → `"Z"`
**Safety:** Always check for NULL before calling `strcmp()` to avoid crashes

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

**THE BUG:**
```c
/* Wrong order: Format BEFORE setting sort state */
itidy_format_listview_columns(columns, num_cols, entry_list, ...);

/* Set sort state AFTER formatting */
state->columns[clicked_col].default_sort = new_order;  /* TOO LATE! */
```

**WHAT HAPPENS:**
- Formatter reads `default_sort` field during `itidy_format_listview_columns()`
- Sort state updated **after** formatting completes
- Headers generated without arrow indicators
- Next click shows **previous** column's arrow (off-by-one bug)

**THE FIX:**
```c
/* Set sort state FIRST */
for (int i = 0; i < state->num_columns; i++)
{
    state->columns[i].default_sort = ITIDY_SORT_NONE;  /* Clear all arrows */
}
state->columns[clicked_col].default_sort = new_order;  /* Set clicked column arrow */

/* NOW format with updated state */
itidy_format_listview_columns(columns, num_cols, entry_list, ...);
```

**KEY INSIGHT:**
- Formatter is **read-only** on state
- State must be updated **before** calling formatter
- Think of formatter as a "render" function that reads current state

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

## PROPOSED ENHANCEMENT: Centralized Cleanup Function

**Date:** 2025-11-18  
**Status:** PROPOSED (not yet implemented)  
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

### Proposed Solution

Add cleanup function to `src/GUI/listview_formatter.c`:

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

**What it frees (in order):**
1. For each entry in `entry_list`:
   - `entry->node.ln_Name` (strdup allocation)
   - `entry->display_data[col]` for each column
   - `entry->sort_keys[col]` for each column
   - `entry->display_data` array
   - `entry->sort_keys` array
   - `entry` structure itself
2. `entry_list` structure
3. `display_list` structure
4. `state` (via existing `itidy_free_listview_state()`)

**Window cleanup becomes:**
```c
/* Before: ~20 lines of error-prone cleanup */
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
whd_free(entry_list);
whd_free(display_list);
itidy_free_listview_state(state);

/* After: 1 line */
itidy_free_listview_entries(data->entry_list, data->display_list, data->state);
```

### Benefits

✅ **Single source of truth** - cleanup logic centralized in formatter module  
✅ **Symmetry** - formatter creates data structures, formatter destroys them  
✅ **Error prevention** - impossible to forget `ln_Name` or other allocations  
✅ **Code reuse** - Restore Window has 2 ListViews, Folder View might add more  
✅ **Maintainability** - future fields added to `iTidy_ListViewEntry` only need cleanup in one place  
✅ **Null safety** - function handles NULL parameters gracefully  

### Considerations

⚠️ **Window still responsible for:**
- Detaching lists from gadgets (`GT_SetGadgetAttrs` with `GTLV_Labels, ~0`)
- Closing window before freeing gadgets
- Freeing gadget list
- Freeing visual info

⚠️ **Design question:** Should function accept gadget pointer and detach automatically?  
⚠️ **Design question:** Need separate function to clear entries without freeing state (for repopulation)?  

### Alternative: Two-Level Cleanup

```c
/* Level 1: Clear entries only (keep lists/state for repopulation) */
void itidy_clear_listview_entries(struct List *entry_list, int num_columns);

/* Level 2: Free everything (window close) */
void itidy_free_listview_data(struct List *entry_list,
                              struct List *display_list,
                              iTidy_ListViewState *state);
```

Allows reusing lists/state if window needs to refresh ListView without closing.

### Implementation Tasks

- [ ] Add function to `src/GUI/listview_formatter.c`
- [ ] Add declaration to `include/GUI/listview_formatter.h`
- [ ] Update Restore Window to use new cleanup function
- [ ] Update Folder View Window (if applicable)
- [ ] Test with memory tracking (verify zero leaks)
- [ ] Update this architecture doc with final implementation
- [ ] Add example to header file documentation

### Decision Points

1. **Function name:** `itidy_free_listview_entries()` vs `itidy_cleanup_listview_data()`?
2. **Parameter requirements:** All three required, or accept NULL for any?
3. **Gadget integration:** Accept gadget pointer to auto-detach, or keep separate?
4. **Two-level approach:** Need separate clear vs free functions?

**See also:** `docs/DEVELOPMENT_LOG.md` (2025-11-18 entry on ListView sorting implementation)
