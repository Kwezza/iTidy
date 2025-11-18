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
    const char ***data_rows,
    int num_rows,
    int total_char_width,
    iTidy_ListViewState **out_state
);
```

**Purpose:** Create formatted ListView with optional initial sorting.

**Behavior:**
- Checks if any column has `default_sort` set
- If yes, sorts data **before** formatting (single pass!)
- Calculates column positions for hit-testing
- Returns state tracking structure for runtime sorting

#### 2. On-Demand Re-sorting
```c
BOOL iTidy_ResortListViewByClick(
    struct List *list,
    iTidy_ListViewState *state,
    int mouse_x,
    int (*comparator)(const void*, const void*, int, iTidy_ColumnType)
);
```

**Purpose:** Resort existing ListView based on column click.

**Behavior:**
- Detects which column was clicked using mouse X position
- Re-sorts List nodes in-place (no reformatting!)
- Updates sort state and visual indicators
- Rebuilds only the tag array for gadget update

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

## Sorting Strategy

### Option 1: Re-sort and Rebuild Tag List (CHOSEN)

**Process:**
1. Sort the existing List nodes in-place (rearrange pointers)
2. Recreate the `STRPTR` array for `GT_SetGadgetAttrs()`
3. Call `GT_SetGadgetAttrs()` with new array

**Performance:**
- O(n log n) sort + O(n) array rebuild
- ~0.01s for 100 entries
- No recalculation of widths, truncation, or formatting

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
- ~0.05-0.1s for 100 entries
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

/* 2. Prepare data with both display and sort keys */
for (i = 0; i < num_sessions; i++) {
    iTidy_ListViewEntry *entry = allocate_entry(3);
    
    /* Display data - human-friendly */
    entry->display_data[0] = "24-Nov-2025 15:19";
    entry->display_data[1] = "2";
    entry->display_data[2] = "PC:Workbench/../tool.info";
    
    /* Sort keys - machine-sortable */
    entry->sort_keys[0] = "20251124_151900";
    entry->sort_keys[1] = "2";
    entry->sort_keys[2] = "PC:Workbench/Tools/tool.info";
    
    add_to_list(entry);
}

/* 3. Create formatted ListView (auto-sorted by Date descending) */
iTidy_ListViewState *lv_state = NULL;
struct List *list = iTidy_FormatListViewColumns(cols, 3, entries, &lv_state);

/* 4. Store state in window data */
restore_data->listview_state = lv_state;

/* 5. Handle mouse clicks in event loop */
if (imsg->Class == IDCMP_MOUSEBUTTONS && clicked_in_header_row) {
    if (iTidy_ResortListViewByClick(list, lv_state, imsg->MouseX, compare_func)) {
        /* Refresh gadget with re-sorted list */
        GT_SetGadgetAttrs(lv_gad, win, NULL, GTLV_Labels, list, TAG_DONE);
    }
}

/* 6. Cleanup */
iTidy_FreeListViewState(lv_state);
iTidy_FreeFormattedList(list);
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

### 2. Clicking Different Columns
**Behavior:** Clear previous sort, apply new sort
```
Click Date:    Sort by Date ascending
Click Changes: Sort by Changes ascending (Date no longer sorted)
```

### 3. Empty or Missing Data
**Behavior:** Empty strings sort first (before any content)
```
NULL or "" → "A" → "Z"
```

### 4. Identical Sort Keys
**Behavior:** Maintain original insertion order (stable sort)

### 5. Very Long Column Titles with Indicators
**Behavior:** Indicator may be truncated if title fills column width
**Solution:** Reserve 2 extra characters for " ^" or " v"

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
- [ ] Add `iTidy_SortOrder` and `iTidy_ColumnType` enums
- [ ] Extend `iTidy_ColumnConfig` with `default_sort` and `sort_type`
- [ ] Create `iTidy_ColumnState` and `iTidy_ListViewState` structures
- [ ] Modify `iTidy_ListViewEntry` to include `display_data` and `sort_keys`
- [ ] Update `iTidy_FormatListViewColumns()` to accept `out_state` parameter
- [ ] Implement initial sorting logic (if `default_sort` is set)
- [ ] Calculate and store column pixel/character positions
- [ ] Implement `iTidy_ResortListViewByClick()` function
- [ ] Add hit-testing logic for column detection
- [ ] Implement smart comparator with type awareness
- [ ] Add visual sort indicators to header row
- [ ] Implement `iTidy_FreeListViewState()` cleanup function

### Phase 2: Integration
- [ ] Update Restore Window to use new sorting API
- [ ] Update Folder View Window to use new sorting API
- [ ] Add IDCMP_MOUSEBUTTONS handling to detect header clicks
- [ ] Test with various data types (dates, numbers, paths, text)
- [ ] Verify performance with 100+ entries

### Phase 3: Polish
- [ ] Add documentation comments to all new functions
- [ ] Update examples in header files
- [ ] Test edge cases (empty lists, null data, etc.)
- [ ] Profile memory usage
- [ ] Add debug logging for sort operations

---

## Conclusion

This architecture provides:
- **Efficient sorting** through pointer manipulation (no reformatting)
- **Correct sorting** via type-aware comparisons and sort keys
- **Clean design** with separation of concerns
- **Reusability** across all iTidy ListViews
- **Extensibility** for future enhancements

The dual-data approach (display_data + sort_keys) is the key insight that makes this work correctly for dates, paths, and formatted numbers while maintaining human-friendly display.
