# ListView Smart Helper - Usage Guide

## Overview

The ListView API provides a complete solution for sortable, paginated ListViews with:
- **Automatic pagination** - Sorts enabled for small lists, pagination for large lists
- **Smart column sorting** - Click headers to sort, automatic arrow indicators
- **Type-aware click handling** - DATE/NUMBER/TEXT detection for context-aware actions
- **Performance optimization** - Page-based slicing for 100+ entry lists on 7MHz hardware
- **Event-driven API** - Single handler function returns structured events (HEADER_SORTED, ROW_CLICK, NAV_HANDLED)

This eliminates the need for callers to manually handle pagination logic, header offsets, list traversal, or column type mappings.

---

## Quick Start Example

```c
#include "helpers/listview_columns_api.h"

/* STEP 1: Define columns */
iTidy_ColumnConfig columns[] = {
    {"Date/Time", 20, 20, ITIDY_ALIGN_LEFT, FALSE, FALSE, 
     ITIDY_SORT_DESCENDING, ITIDY_COLTYPE_DATE},  /* Default sort: newest first */
    {"Changes", 4, 6, ITIDY_ALIGN_RIGHT, FALSE, FALSE,
     ITIDY_SORT_NONE, ITIDY_COLTYPE_NUMBER},
    {"Path", 30, 0, ITIDY_ALIGN_LEFT, TRUE, TRUE,
     ITIDY_SORT_NONE, ITIDY_COLTYPE_TEXT}
};

/* STEP 2: Build entry list */
struct List entry_list;
NewList(&entry_list);
/* ... populate entries ... */

/* STEP 3: Format with auto-pagination (sorting when ≤10 entries, pagination when >10) */
iTidy_ListViewState *state = NULL;
int total_pages = 0;
struct List *display_list = iTidy_FormatListViewColumns(
    columns, 3,              /* Column config */
    &entry_list,             /* Your data */
    65,                      /* Width in characters */
    &state,                  /* Returns state (for events and cleanup) */
    ITIDY_MODE_FULL,         /* Mode: enable sorting + optional pagination */
    10,                      /* page_size: Auto-pagination threshold */
    1,                       /* current_page: Start on page 1 */
    &total_pages,            /* Returns total pages */
    0                        /* nav_direction: 0=none, -1=Previous, +1=Next */
);

GT_SetGadgetAttrs(listview, window, NULL, GTLV_Labels, display_list, TAG_DONE);

/* STEP 4: Handle events with single function */
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        /* Process click - returns structured event */
        iTidy_ListViewEvent event;
        if (iTidy_HandleListViewGadgetUp(
                window, gadget,
                msg->MouseX, msg->MouseY,
                &entry_list, display_list, state,
                prefsIControl.systemFontSize, prefsIControl.systemFontCharWidth,
                columns, 3, &event)) {
            
            switch (event.type) {
                case ITIDY_LV_EVENT_HEADER_SORTED:
                    /* Sorting happened automatically - just rebuild display */
                    rebuild_listview();  /* Re-format and refresh */
                    break;
                    
                case ITIDY_LV_EVENT_NAV_HANDLED:
                    /* Page navigation happened - rebuild on new page */
                    rebuild_listview();  /* Uses state->current_page */
                    break;
                    
                case ITIDY_LV_EVENT_ROW_CLICK:
                    /* User clicked a data row */
                    printf("Clicked: %s in column %d\n", 
                           event.entry->display_data[event.column],
                           event.column);
                    break;
            }
        }
    }
    break;

/* STEP 5: Cleanup on window close */
if (display_list) iTidy_FreeFormattedList(display_list);
if (state) iTidy_FreeListViewState(state);
/* Free entry_list and your data */
```

---

## Auto-Pagination Feature

### How It Works

The API uses **two parameters** to control behavior:

1. **`mode`** - Selects feature set (sorting, state, pagination support)
2. **`page_size`** - Controls pagination threshold (only used with paginated modes)

```c
struct List *display_list = iTidy_FormatListViewColumns(
    columns, num_columns,
    &entry_list,
    width,
    &state,
    ITIDY_MODE_FULL,  /* Mode: FULL/FULL_NO_SORT/SIMPLE/SIMPLE_PAGINATED */
    10,               /* page_size: Entries per page (0 = auto-calculate) */
    1,                /* current_page */
    &total_pages,
    0                 /* nav_direction */
);
```

**Behavior by Mode:**

| Mode | Sorting | State | Pagination | page_size Effect |
|------|---------|-------|------------|------------------|
| `ITIDY_MODE_FULL` | ✅ Yes | ✅ Yes | Optional | `page_size=0`: No pagination<br>`page_size>0`: Paginate if entries > page_size |
| `ITIDY_MODE_FULL_NO_SORT` | ❌ No | ✅ Yes | Optional | Same as FULL but sorting disabled |
| `ITIDY_MODE_SIMPLE` | ❌ No | ❌ No | ❌ No | Ignored - full list always shown |
| `ITIDY_MODE_SIMPLE_PAGINATED` | ❌ No | ❌ No | ✅ Yes | Required - sets entries per page |

### Why Use Pagination?

**Problem:** Formatting 300 ListView rows takes 4.7 seconds on 68000 @ 7MHz (O(n²) complexity)

**Solution:** 
- Small lists: Use `ITIDY_MODE_FULL` with `page_size=0` (sorting enabled, no pagination)
- Large lists needing sort: Use `ITIDY_MODE_FULL` with `page_size=10` (paginate when > 10 entries)
- Large lists no sort: Use `ITIDY_MODE_SIMPLE_PAGINATED` (fastest, minimal memory)

### Configuration Example

```c
/* In window initialization */
typedef struct {
    struct List entry_list;
    struct List *display_list;
    iTidy_ListViewState *state;
    int current_page;      /* Tracks page between rebuilds */
    int page_size;         /* Entries per page */
} WindowData;

/* For small lists with sorting */
data->display_list = iTidy_FormatListViewColumns(
    columns, num_columns, &data->entry_list, width, &data->state,
    ITIDY_MODE_FULL,  /* Enable sorting */
    0,                /* No pagination */
    1, &total_pages, 0
);

/* For large lists with sorting */
data->display_list = iTidy_FormatListViewColumns(
    columns, num_columns, &data->entry_list, width, &data->state,
    ITIDY_MODE_FULL,  /* Enable sorting */
    10,               /* Paginate when > 10 entries */
    1, &total_pages, 0
);

/* For large lists without sorting (fastest) */
data->display_list = iTidy_FormatListViewColumns(
    columns, num_columns, &data->entry_list, width, NULL,
    ITIDY_MODE_SIMPLE_PAGINATED,  /* No sorting, minimal memory */
    10,                           /* 10 entries per page */
    1, &total_pages, 0
);
```

---

## Simple Mode

Simple mode trades features for performance and memory efficiency. Use it when you don't need interactive sorting and want the fastest possible window close times on stock Amigas.

### When to Use Simple Mode

Use **Simple Mode** for display-only ListViews where you:
- ✅ **Never need to sort** - Data shown in original order only
- ✅ **Want fastest performance** - Minimal overhead on 68000
- ✅ **Need fast window close** - Critical for stock Amigas!

**Two Simple Mode Variants:**
1. **Simple Non-Paginated** (`ITIDY_MODE_SIMPLE`) - Full list always visible
2. **Simple Paginated** (`ITIDY_MODE_SIMPLE_PAGINATED`) - Pages shown with navigation rows

**Performance Benefits on Stock 68000 @ 7MHz:**
- **No sorting overhead** - Skips O(n²) merge sort entirely
- **Minimal allocations** - Only header + separator + ln_Name strings
- **Dramatically faster cleanup** - 300 rows: **51 seconds → <1 second!**

### What Simple Mode Skips

```c
/* SIMPLE MODES (ITIDY_MODE_SIMPLE / ITIDY_MODE_SIMPLE_PAGINATED) do NOT create: */
❌ iTidy_ListViewState structure (no state tracking)
❌ Sorting machinery (no iTidy_SortListViewEntries calls)
❌ Header click sorting (headers display-only, not interactive)
❌ Column metadata tracking (no column detection for clicks)
❌ Background data retention (entry->display_data not copied to display list)

/* SIMPLE MODE still provides: */
✅ Column width calculation (same algorithm)
✅ Text alignment and truncation (same quality)
✅ Path abbreviation support (is_path=TRUE works)
✅ Header and separator rows (visual structure)
✅ Professional formatting (same output quality)
✅ Row click events (ITIDY_LV_EVENT_ROW_CLICK with entry data)
✅ Pagination support (Simple Paginated mode only, mode=3)
✅ Navigation rows (Simple Paginated mode, "<- Previous / Next ->")
```

### Simple Mode Variants

#### Mode Parameter Contract

The API uses an **explicit `mode` parameter** (not page_size flags):

```c
typedef enum {
    ITIDY_MODE_FULL,              /* Full mode: sorting + state + optional pagination */
    ITIDY_MODE_FULL_NO_SORT,      /* Full mode but disable sorting (performance) */
    ITIDY_MODE_SIMPLE,            /* Simple mode: display-only, no sorting/state (fast) */
    ITIDY_MODE_SIMPLE_PAGINATED   /* Simple mode + pagination (best for large lists) */
} iTidy_ListViewMode;

/* Function signature */
struct List *iTidy_FormatListViewColumns(
    iTidy_ColumnConfig *columns,
    int num_columns,
    struct List *entries,
    int total_char_width,
    iTidy_ListViewState **out_state,
    iTidy_ListViewMode mode,     /* ← Explicit mode parameter */
    int page_size,               /* Entries per page (used with paginated modes) */
    int current_page,
    int *out_total_pages,
    int nav_direction
);
```

**Mode Selection Guide:**
- **`ITIDY_MODE_SIMPLE`**: Non-paginated, full list display (no state, no sorting)
- **`ITIDY_MODE_SIMPLE_PAGINATED`**: Paginated, minimal state for auto-select, no sorting
- **`ITIDY_MODE_FULL`**: Full features (state, sorting, optional pagination based on page_size)
- **`ITIDY_MODE_FULL_NO_SORT`**: State allocated but sorting disabled (rare use case)

**Parameter Interaction:**
- **`mode = ITIDY_MODE_SIMPLE`**: `page_size` is ignored, full list shown, `out_state` = NULL
- **`mode = ITIDY_MODE_SIMPLE_PAGINATED`**: `page_size` sets entries per page, `out_state` = minimal state (~40 bytes)
- **`mode = ITIDY_MODE_FULL`**: `page_size` determines pagination:
  - `page_size = 0`: No pagination, sorting enabled
  - `page_size > 0`: Pagination active if entry count > page_size
- **`mode = ITIDY_MODE_FULL_NO_SORT`**: Like FULL but header clicks ignored

#### Simple Non-Paginated (ITIDY_MODE_SIMPLE)
- **Use case**: Small lists that fit in viewport (≤20 entries)
- **Features**: No state, no pagination, full list always visible
- **Navigation rows**: None
- **Memory**: Minimal (1 allocation per row)
- **Entry selection**: Direct index lookup from entry_list

#### Simple Paginated (ITIDY_MODE_SIMPLE_PAGINATED)
- **Use case**: Large lists requiring pagination without sorting
- **Features**: Minimal state (~40 bytes) for auto-select, pagination enabled, navigation rows shown
- **Navigation rows**: "<- Previous (Page X of Y)" and "Next -> (Page X of Y)"
- **Memory**: Lightweight (1 allocation per row, minimal state for tracking)
- **Entry selection**: Auto-select calculated by API, consistent navigation UX
- **Trade-off**: Small state object (~40 bytes) vs. fully stateless SIMPLE mode

**Key Difference from Full Paginated Mode:**
- Full paginated mode (`ITIDY_MODE_FULL` with `page_size > 0`): Has full `state` with column tracking
- Simple paginated mode (`ITIDY_MODE_SIMPLE_PAGINATED`): Minimal state for auto-select only

### Memory Footprint Comparison

**Full Mode (ITIDY_MODE_FULL or ITIDY_MODE_FULL_NO_SORT):**
```
Per Entry Allocations:
1. entry->node.ln_Name (formatted display string)
2-5. entry->display_data[0-3] (4 column strings)
6-9. entry->sort_keys[0-3] (4 sort key strings)
10. entry->display_data array
11. entry->sort_keys array
12. entry structure itself
= ~12 allocations × 300 entries = 3600 allocations
Cleanup time: ~51 seconds (0.16s per entry)
```

**Simple Non-Paginated (ITIDY_MODE_SIMPLE):**
```
Per Row Allocations:
1. display_node->ln_Name (formatted string only)
= 1 allocation × 300 rows = 300 allocations
State object: None
Cleanup time: <1 second (0.003s per row)
```

**Simple Paginated (ITIDY_MODE_SIMPLE_PAGINATED):**
```
Per Row Allocations:
1. display_node->ln_Name (formatted string only)
= 1 allocation × (page_size + 2 nav rows) per page
Example: 1 allocation × 6 rows (4 data + 2 nav) = 6 allocations per page
State object: None (page number parsed from nav row text)
Cleanup time: <1 second (0.003s per row)
```

### Usage Example

#### Simple Non-Paginated Mode (ITIDY_MODE_SIMPLE)

```c
/* Initialize ListView in simple non-paginated mode */
WindowData *data;
iTidy_ColumnConfig cols[] = {
    {"Date/Time", 20, 20, ITIDY_ALIGN_LEFT, FALSE, FALSE, 
     ITIDY_SORT_NONE, ITIDY_COLTYPE_DATE},  /* default_sort ignored in simple mode */
    {"Changes", 4, 6, ITIDY_ALIGN_RIGHT, FALSE, FALSE,
     ITIDY_SORT_NONE, ITIDY_COLTYPE_NUMBER},
    {"Path", 30, 0, ITIDY_ALIGN_LEFT, TRUE, TRUE,
     ITIDY_SORT_NONE, ITIDY_COLTYPE_TEXT}
};

/* Build entry list (your data) */
struct List entry_list;
NewList(&entry_list);
/* ... populate entries ... */

/* Format in SIMPLE MODE (no pagination) */
data->display_list = iTidy_FormatListViewColumns(
    cols, 3,
    &entry_list,
    65,                      /* Width in characters */
    NULL,                    /* out_state = NULL (SIMPLE mode - no state) */
    ITIDY_MODE_SIMPLE,       /* ← Explicit mode parameter */
    0,                       /* page_size: ignored in SIMPLE mode */
    0,                       /* current_page: ignored */
    NULL,                    /* out_total_pages: not needed */
    0                        /* nav_direction: ignored */
);

/* Set ListView gadget */
GT_SetGadgetAttrs(listview, window, NULL, 
                  GTLV_Labels, data->display_list, 
                  TAG_DONE);

/* ... later cleanup (FAST!) ... */
GT_SetGadgetAttrs(listview, window, NULL, GTLV_Labels, ~0, TAG_DONE);
if (data->display_list) {
    iTidy_FreeFormattedList(data->display_list);  /* <1 second for 300 rows! */
    data->display_list = NULL;
}

/* Free your entry list (you own this) */
while ((node = RemHead(&entry_list)) != NULL) {
    iTidy_ListViewEntry *entry = (iTidy_ListViewEntry *)node;
    /* Free display_data, sort_keys, entry itself */
    whd_free(entry);
}

/* Row clicks still work in simple mode! */
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        iTidy_ListViewEvent event;
        
        if (iTidy_HandleListViewGadgetUp(
                window, gadget,
                mouseX, mouseY,
                &entry_list,           /* Your data entries */
                data->display_list,    /* Display list from format call */
                NULL,                  /* state is NULL in simple mode */
                font_height, font_width,
                cols, 3, &event))
        {
            if (event.type == ITIDY_LV_EVENT_ROW_CLICK) {
                /* User clicked a data row - entry is valid! */
                process_selection(event.entry);
            }
            /* Header/separator clicks return FALSE (ignored) */
        }
    }
    break;
```

#### Simple Paginated Mode (ITIDY_MODE_SIMPLE_PAGINATED)

```c
/* Initialize ListView in simple paginated mode */
RestoreWindowData *restore_data;

/* Set mode and page size */
restore_data->page_size = 4;                       /* 4 rows per page */
restore_data->current_page = 1;                    /* Start on page 1 */

/* Format with pagination (state=NULL, navigation rows added) */
restore_data->display_list = iTidy_FormatListViewColumns(
    cols, 3,
    &restore_data->run_entries_list,  /* Your entry list */
    65,                               /* Width in characters */
    NULL,                             /* out_state = NULL (no state in simple paginated) */
    ITIDY_MODE_SIMPLE_PAGINATED,      /* ← Explicit mode parameter */
    restore_data->page_size,          /* 4 rows per page */
    restore_data->current_page,       /* Current page number */
    &restore_data->total_pages,       /* Returns total pages */
    0                                 /* nav_direction: 0 on first load */
);

/* Set ListView with formatted list */
GT_SetGadgetAttrs(restore_data->listview, window, NULL,
                  GTLV_Labels, restore_data->display_list,
                  TAG_DONE);

/* Handle events with pagination support */
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_SESSION_LISTVIEW) {
        iTidy_ListViewEvent event;
        if (iTidy_HandleListViewGadgetUp(
                window, gadget,
                msg->MouseX, msg->MouseY,
                &restore_data->run_entries_list,
                restore_data->display_list,
                NULL,              /* state is NULL in SIMPLE_PAGINATED mode */
                font_height, font_width,
                cols, 3, &event))
        {
            switch (event.type) {
                case ITIDY_LV_EVENT_NAV_HANDLED:
                    /* Navigation clicked - API parsed page from nav row text */
                    /* Update page tracking */
                    if (event.nav_direction == 1) {
                        restore_data->current_page++;
                    } else if (event.nav_direction == -1) {
                        restore_data->current_page--;
                    }
                    
                    /* Rebuild list for new page */
                    rebuild_paginated_list(restore_data);
                    break;
                    
                case ITIDY_LV_EVENT_ROW_CLICK:
                    /* Data row clicked - API calculated correct entry_index */
                    /* using page number parsed from nav row: */
                    /* entry_index = ((page - 1) × page_size) + clicked_row */
                    process_entry_selection(event.entry);
                    break;
            }
        }
    }
    break;

void rebuild_paginated_list(RestoreWindowData *restore_data) {
    /* Detach old list */
    GT_SetGadgetAttrs(restore_data->listview, window, NULL, 
                     GTLV_Labels, ~0, TAG_DONE);
    
    /* Free old display list */
    if (restore_data->display_list) {
        iTidy_FreeFormattedList(restore_data->display_list);
        restore_data->display_list = NULL;
    }
    
    /* Rebuild for new page (no state to free in mode=3) */
    restore_data->display_list = iTidy_FormatListViewColumns(
        cols, 3,
        &restore_data->run_entries_list,
        65,
        NULL,                          /* out_state still NULL */
        ITIDY_MODE_SIMPLE_PAGINATED,
        restore_data->page_size,
        restore_data->current_page,    /* Use updated page number */
        &restore_data->total_pages,
        0
    );
    
    /* Reattach new list */
    GT_SetGadgetAttrs(restore_data->listview, window, NULL,
                     GTLV_Labels, restore_data->display_list,
                     TAG_DONE);
}
```

**Key Differences:**
- **ITIDY_MODE_SIMPLE**: No navigation rows, full list shown, direct entry lookup
- **ITIDY_MODE_SIMPLE_PAGINATED**: Navigation rows shown, page offset calculated via text parsing

### When NOT to Use Simple Mode

❌ **Don't use simple mode if you need:**
- Click-to-sort headers (use `ITIDY_MODE_FULL` with `page_size=0`)
- Column-specific click metadata with state tracking (use `ITIDY_MODE_FULL`)
- Dynamic re-sorting (simple mode is display-once only)

✅ **Simple mode IS sufficient if you need:**
- Row selection (clicking rows to get entry data works fine)
- Display-only headers (non-interactive column titles)
- Static display of pre-sorted data
- Pagination without sorting (use `ITIDY_MODE_SIMPLE_PAGINATED`)

**Choose the right mode:**
- **ITIDY_MODE_SIMPLE**: Small lists (≤20 entries), no pagination needed
- **ITIDY_MODE_SIMPLE_PAGINATED**: Large lists requiring pagination without sorting overhead

### How Simple Paginated Mode Works (ITIDY_MODE_SIMPLE_PAGINATED)

**The Challenge:** Pagination normally requires a state object to track `current_page`, but simple mode deliberately avoids allocating state to save memory (~100 bytes).

**The Solution:** Embed page information in the navigation row text itself, then parse it back when needed.

**Navigation Row Format:**
```
"<- Previous (Page 2 of 5)"
"Next -> (Page 2 of 5)"
```

**When User Clicks Entry (Not Nav Row):**
1. API detects this is a data row (not header/separator/nav)
2. API scans `display_list` for navigation rows
3. Uses `strstr()` to find "Page" keyword in nav row text
4. Uses `sscanf(page_str, "Page %d of %d", &current_page, &total_pages)` to extract page number
5. Calculates actual entry index: `entry_index = ((current_page - 1) × page_size) + clicked_display_row`
6. Looks up entry from `entry_list` at calculated index
7. Returns `ITIDY_LV_EVENT_ROW_CLICK` with correct `event.entry` pointer

**When User Clicks Navigation Row:**
1. API detects nav row via `strstr(node_text, "Previous")` or `strstr(node_text, "Next")`
2. Returns `ITIDY_LV_EVENT_NAV_HANDLED` with `event.nav_direction` (-1 or +1)
3. Caller updates `restore_data->current_page` manually
4. Caller rebuilds list by calling `iTidy_FormatListViewColumns()` with new page number

**Trade-off Analysis:**
- **Memory saved**: ~100 bytes (no iTidy_ListViewState structure)
- **CPU cost**: 2-3 string scans per click (~0.5ms on 68000 @ 7MHz)
- **Complexity**: Slightly more logic in API, but transparent to caller
- **Benefit**: Same memory efficiency as simple mode, with pagination support

**Log Messages (ITIDY_MODE_SIMPLE_PAGINATED):**
```
Detected pagination from nav row: page 2 of 3
Paginated: clicked row 1 on page 2 (page_size=4) -> entry_index=5
```

**Example Entry Index Calculation:**
```
User on page 2, clicks row 3 (4 rows per page):
  entry_index = ((2 - 1) × 4) + 1 = 4 + 1 = 5
  Returns entry #6 (0-based index 5) from entry_list
```

This approach maintains simple mode's memory efficiency while enabling pagination for large lists.

### Simple Mode Performance Table (68000 @ 7MHz, Chip RAM)

| Rows | Format Time | Cleanup Time | Full Mode Cleanup |
|------|-------------|--------------|-------------------|
| 50   | ~300ms      | ~20ms        | ~8s               |
| 100  | ~1s         | ~40ms        | ~16s              |
| 150  | ~1.6s       | ~60ms        | ~24s              |
| 200  | ~2.5s       | ~80ms        | ~32s              |
| 250  | ~3.5s       | ~100ms       | ~41s              |
| 300  | ~4.5s       | ~120ms       | **~51s**          |

**Key Insight:** Full mode's cleanup cost **dwarfs** format time. Simple mode makes cleanup negligible.

---

### Configuration Example

```c
/* In window initialization */
typedef struct {
    struct List entry_list;
    struct List *display_list;
    iTidy_ListViewState *state;
    int current_page;      /* Tracks page between rebuilds */
    int page_size;         /* Auto-pagination threshold */
} WindowData;

data->current_page = 1;
data->page_size = 10;  /* Show full list up to 10 entries, paginate beyond that */
```

### Rebuilding After Navigation

When pagination is active, clicking Previous/Next returns `ITIDY_LV_EVENT_NAV_HANDLED`:

```c
case ITIDY_LV_EVENT_NAV_HANDLED:
    /* API updated state->current_page and state->last_nav_direction */
    /* Just rebuild the list - formatter handles everything */
    rebuild_listview(data);
    break;

void rebuild_listview(WindowData *data) {
    /* Save navigation state BEFORE freeing */
    int current_page = data->state ? data->state->current_page : 1;
    int nav_direction = data->state ? data->state->last_nav_direction : 0;
    
    /* Detach and free old list */
    GT_SetGadgetAttrs(data->listview, window, NULL, GTLV_Labels, ~0, TAG_DONE);
    if (data->display_list) iTidy_FreeFormattedList(data->display_list);
    if (data->state) iTidy_FreeListViewState(data->state);
    
    /* Rebuild with saved navigation state */
    int total_pages;
    data->display_list = iTidy_FormatListViewColumns(
        columns, num_columns,
        &data->entry_list,
        width,
        &data->state,
        ITIDY_MODE_FULL,
        data->page_size,
        current_page,        /* Use saved page */
        &total_pages,
        nav_direction        /* Use saved direction for auto-selection */
    );
    
    /* Reattach and select appropriate row */
    GT_SetGadgetAttrs(data->listview, window, NULL,
                     GTLV_Labels, data->display_list,
                     GTLV_Selected, data->state->auto_select_row,
                     TAG_DONE);
}
```

### Visual Behavior

**With ≤10 entries** (sorting enabled):
```
┌─────────────────────────────────────┐
│ Date/Time ^      Changes  Source    │ ← Click to sort
├─────────────────────────────────────┤
│ 24-Nov-2025 15:19    13   Work/...  │
│ 23-Nov-2025 10:32     7   Projects/ │
│ 22-Nov-2025 18:45    42   Desktop   │
│ ...                                  │
└─────────────────────────────────────┘
```

**With >10 entries** (pagination active):
```
┌─────────────────────────────────────┐
│ Date/Time        Changes  Source    │ ← Not clickable
├─────────────────────────────────────┤
│ <- Previous (Page 2 of 5)           │ ← Navigation row
│ 14-Nov-2025 15:19    13   Work/...  │
│ 13-Nov-2025 10:32     7   Projects/ │
│ 12-Nov-2025 18:45    42   Desktop   │
│ ...                                  │
│ Next -> (Page 2 of 5)               │ ← Navigation row
└─────────────────────────────────────┘
```

### Log Messages

The API logs its auto-pagination decisions:

```
Auto-Pagination DISABLED: 8 entries <= page_size, sorting enabled
```

```
Auto-Pagination ACTIVE: 47 entries > page_size 10, showing page 1 of 5 (entries 0-9)
```

---

## Event Types

### iTidy_ListViewEvent Structure

```c
typedef enum {
    ITIDY_LV_EVENT_NONE = 0,       /* No action needed */
    ITIDY_LV_EVENT_HEADER_SORTED,  /* Header clicked, list was sorted */
    ITIDY_LV_EVENT_ROW_CLICK,      /* Data row clicked */
    ITIDY_LV_EVENT_NAV_HANDLED     /* Navigation row clicked, page changed */
} iTidy_ListViewEventType;

typedef struct {
    iTidy_ListViewEventType type;
    
    /* All events */
    iTidy_ListViewEntry *entry;    /* Selected entry (NULL for HEADER_SORTED/NAV_HANDLED) */
    int column;                     /* Clicked column (-1 if N/A) */
    
    /* ROW_CLICK event fields */
    const char *display_value;      /* "2.8 MB", "24-Nov-2025" */
    const char *sort_key;           /* "0002949120", "20251124_151900" */
    iTidy_ColumnType column_type;   /* DATE/NUMBER/TEXT */
    
    /* HEADER_SORTED event fields */
    BOOL did_sort;                  /* TRUE if sort occurred */
    int sorted_column;              /* Which column was sorted */
    iTidy_SortOrder sort_order;     /* ASCENDING/DESCENDING/NONE */
} iTidy_ListViewEvent;
```

### Event Handler Pattern

```c
iTidy_ListViewEvent event;
if (iTidy_HandleListViewGadgetUp(
    window, gadget,
    mouse_x, mouse_y,
    &entry_list, display_list, state,
    font_height, font_width,
        columns, num_columns, &event)) {
    
    switch (event.type) {
        case ITIDY_LV_EVENT_NONE:
            /* Separator row or invalid click - ignore */
            break;
            
        case ITIDY_LV_EVENT_HEADER_SORTED:
            /* List was sorted in-place, display_list rebuilt */
            GT_SetGadgetAttrs(listview, window, NULL,
                             GTLV_Labels, ~0,
                             TAG_DONE);
            GT_SetGadgetAttrs(listview, window, NULL,
                             GTLV_Labels, display_list,
                             GTLV_Top, 0,
                             TAG_DONE);
            log_info(LOG_GUI, "Sorted by column %d (%s)\n",
                     event.sorted_column,
                     event.sort_order == ITIDY_SORT_ASCENDING ? "ASC" : "DESC");
            break;
            
        case ITIDY_LV_EVENT_NAV_HANDLED:
            /* Pagination navigation occurred, rebuild list */
            rebuild_listview();
            log_info(LOG_GUI, "Navigated to page %d\n", state->current_page);
            break;
            
        case ITIDY_LV_EVENT_ROW_CLICK:
            /* User clicked a data row */
            switch (event.column_type) {
                case ITIDY_COLTYPE_DATE:
                    show_date_details(event.sort_key);
                    break;
                case ITIDY_COLTYPE_NUMBER:
                    show_number_editor(atoi(event.sort_key));
                    break;
                case ITIDY_COLTYPE_TEXT:
                    copy_to_clipboard(event.display_value);
                    break;
            }
            break;
    }
}
```

---

## The iTidy_ListViewClick Structure (Legacy Helper)

For compatibility, the old `iTidy_GetListViewClick()` helper is still available:

```c
typedef struct {
    iTidy_ListViewEntry *entry;       /* Selected entry (NULL if header row) */
    int column;                        /* Clicked column index (0-based, -1 if invalid) */
    const char *display_value;         /* Human-readable: "2.8 MB", "24-Nov-2025" */
    const char *sort_key;              /* Machine-readable: "0002949120", "20251124_151900" */
    iTidy_ColumnType column_type;      /* DATE/NUMBER/TEXT */
} iTidy_ListViewClick;
```

**Note:** For new code, prefer `iTidy_HandleListViewGadgetUp()` which handles sorting and pagination automatically.

---

## Performance Characteristics

### Auto-Pagination Performance Impact

**Without Pagination** (300 entries):
- Initial format: ~4.7 seconds (O(n²) formatting)
- Each sort: ~4.7 seconds (full list rebuild)
- User experience: Sluggish, frustrating on 68000 @ 7MHz

**With Auto-Pagination** (page_size=10):
- Initial format: ~160ms (only formats 10 entries)
- Each page change: ~160ms (formats next 10 entries)
- Sorting: Disabled (prevents full-list rebuild)
- User experience: Snappy, responsive

**Auto Mode Benefits**:
- ≤10 entries: Full sorting capability (fast enough)
- >10 entries: Pagination kicks in automatically (maintains responsiveness)

### Typical Timings (68000 @ 7MHz, 2MB Chip RAM)

| Operation | Small List (≤10) | Large List (>100) | Auto-Pagination |
|-----------|------------------|-------------------|-----------------|
| Initial format | 50ms | 4700ms | 160ms |
| Sort by column | 50ms | 4700ms | N/A (disabled) |
| Page navigation | N/A | N/A | 160ms |
| Row click | <1ms | <1ms | <1ms |

---

## Column Sorting Behavior

### When Sorting is Enabled

Sorting is **enabled** when:
- `page_size = 0` (pagination disabled), OR
- Entry count ≤ `page_size` (auto-pagination disabled for small lists)

**Sorting is DISABLED when:**
- `page_size = -1` (explicitly disabled by user preference), OR
- Pagination is active (entry count > `page_size`)

**Visual indicator:** Header row clickable, shows sort arrows (^ or v)

```
┌─────────────────────────────────────┐
│ Date/Time ^      Changes  Source    │ ← Clicking toggles sort direction
├─────────────────────────────────────┤
│ 24-Nov-2025 15:19    13   Work/...  │
│ 23-Nov-2025 10:32     7   Projects/ │
└─────────────────────────────────────┘
```

### When Sorting is Disabled

Sorting is **disabled** when:
- `page_size = -1` (explicitly disabled by user preference), OR
- Pagination is active (entry count > `page_size`)

**Visual indicator:** Header row not clickable, no arrows

```
┌─────────────────────────────────────┐
│ Date/Time        Changes  Source    │ ← Not clickable
├─────────────────────────────────────┤
│ 14-Nov-2025 15:19    13   Work/...  │
└─────────────────────────────────────┘
```

**Log messages when header clicked:**
```
Header click ignored - sorting disabled during pagination
```
OR
```
Header click ignored - sorting disabled by user preference
```

### Sort Behavior

1. **First click on column**: Sort ascending (^ arrow)
2. **Second click on same column**: Sort descending (v arrow)
3. **Click on different column**: Sort by new column ascending

**Event returned:** `ITIDY_LV_EVENT_HEADER_SORTED`

```c
case ITIDY_LV_EVENT_HEADER_SORTED:
    /* List was sorted automatically by API */
    log_info(LOG_GUI, "Sorted by column %d %s\n",
             event.sorted_column,
             event.sort_order == ITIDY_SORT_ASCENDING ? "ascending" : "descending");
    
    /* Just refresh the ListView - sorting already done */
    GT_SetGadgetAttrs(listview, window, NULL, GTLV_Labels, ~0, TAG_DONE);
    GT_SetGadgetAttrs(listview, window, NULL, 
                     GTLV_Labels, display_list,
                     GTLV_Top, 0,  /* Reset scroll */
                     TAG_DONE);
    break;
```

---

## Navigation Row Behavior

### Auto-Selection After Navigation

The API automatically selects an appropriate row after page navigation to maintain visual stability:

**After clicking "Next ->":**
- Selects **last data row** on new page
- Prevents viewport jumping
- User sees smooth downward progression

**After clicking "<- Previous":**
- Selects **first data row after navigation row** (row 2)
- Prevents viewport jumping
- User sees smooth upward progression

### Implementation Detail

The formatter calculates `state->auto_select_row` based on `nav_direction` parameter:

```c
/* After formatting, in caller's rebuild function */
GT_SetGadgetAttrs(listview, window, NULL,
                 GTLV_Labels, display_list,
                 GTLV_Selected, state->auto_select_row,  /* API provides this */
                 TAG_DONE);
```

**Values:**
- `nav_direction = 1` (Next): `auto_select_row = last_row` (Next button for easy repeat clicks)
- `nav_direction = -1` (Previous): `auto_select_row = 2` (first data row)
- `nav_direction = 0` (initial/refresh): `auto_select_row = 2` (first data row)

**Applies to both FULL and SIMPLE_PAGINATED modes** - both create state for consistent auto-select.

### Navigation Row Format

```
"<- Previous (Page X of Y)"    /* When current_page > 1 */
"Next -> (Page X of Y)"        /* When current_page < total_pages */
```

Full-width rows, not column-formatted. ASCII arrows for Workbench font compatibility.

---

## Type-Aware Processing Pattern

### Example: Different Actions per Column Type

```c
iTidy_ListViewClick click = iTidy_GetListViewClick(...);

if (click.entry != NULL && click.column >= 0) {
    switch (click.column_type) {
        case ITIDY_COLTYPE_DATE:
            /* Parse date from sort_key for calendar widget */
            timestamp = parse_date_sortkey(click.sort_key);  /* "20251124_151900" */
            show_calendar_popup(timestamp);
            break;
            
        case ITIDY_COLTYPE_NUMBER:
            /* Parse number from sort_key for numeric operations */
            int value = atoi(click.sort_key);  /* "0000000013" -> 13 */
            show_number_editor(value);
            break;
            
        case ITIDY_COLTYPE_TEXT:
            /* Use display_value for text operations */
            copy_to_clipboard(click.display_value);
            show_notification("Copied: %s", click.display_value);
            break;
    }
}
```

---

## Complete Integration Example

### Window Data Structure

```c
typedef struct {
    /* Window/gadget pointers */
    struct Window *window;
    struct Gadget *listview;
    APTR visual_info;
    
    /* ListView API data structures */
    struct List entry_list;          /* Your data (you own this) */
    struct List *display_list;       /* Formatted list (API owns this) */
    iTidy_ListViewState *state;      /* State tracking (API owns this) */
    
    /* Configuration */
    int current_page;                /* Initial page (API manages after first format) */
    int page_size;                   /* Auto-pagination threshold */
    
    /* Double-click tracking (optional) */
    ULONG last_click_secs;
    ULONG last_click_micros;
} MyWindowData;
```

### Initialization

```c
BOOL init_window(MyWindowData *data) {
    /* Initialize entry list */
    NewList(&data->entry_list);
    
    /* Set pagination config */
    data->current_page = 1;
    data->page_size = 10;  /* Auto-pagination at 10 entries */
    
    /* Populate entry list */
    populate_entries(&data->entry_list);
    
    /* Format ListView */
    iTidy_ColumnConfig columns[] = {
        {"Date", 20, 20, ITIDY_ALIGN_LEFT, FALSE, FALSE, 
         ITIDY_SORT_DESCENDING, ITIDY_COLTYPE_DATE},
        {"Size", 10, 12, ITIDY_ALIGN_RIGHT, FALSE, FALSE,
         ITIDY_SORT_NONE, ITIDY_COLTYPE_NUMBER},
        {"Name", 30, 0, ITIDY_ALIGN_LEFT, TRUE, TRUE,
         ITIDY_SORT_NONE, ITIDY_COLTYPE_TEXT}
    };
    
    int total_pages;
    data->display_list = iTidy_FormatListViewColumns(
        columns, 3,
        &data->entry_list,
        65,  /* Character width */
        &data->state,
        ITIDY_MODE_FULL,
        data->page_size,
        data->current_page,
        &total_pages,
        0    /* No navigation yet */
    );
    
    if (!data->display_list) return FALSE;
    
    /* Create ListView gadget */
    data->listview = CreateGadget(LISTVIEW_KIND, prev_gadget,
        &ng,
        GTLV_Labels, data->display_list,  /* NOT &data->display_list! */
        GTLV_ShowSelected, NULL,
        TAG_DONE);
    
    /* Open window, etc. */
    return TRUE;
}
```

### Event Loop

```c
void handle_events(MyWindowData *data) {
    struct IntuiMessage *msg;
    BOOL running = TRUE;
    
    while (running) {
        WaitPort(data->window->UserPort);
        
        while ((msg = GT_GetIMsg(data->window->UserPort))) {
            ULONG class = msg->Class;
            UWORD code = msg->Code;
            struct Gadget *gadget = (struct Gadget *)msg->IAddress;
            WORD mouse_x = msg->MouseX;  /* CAPTURE BEFORE REPLYING! */
            WORD mouse_y = msg->MouseY;
            
            GT_ReplyIMsg(msg);
            
            switch (class) {
                case IDCMP_CLOSEWINDOW:
                    running = FALSE;
                    break;
                    
                case IDCMP_GADGETUP:
                    if (gadget->GadgetID == GID_LISTVIEW) {
                        handle_listview_click(data, gadget, mouse_x, mouse_y);
                    }
                    break;
            }
        }
    }
}

void handle_listview_click(MyWindowData *data, struct Gadget *gadget, WORD mouse_x, WORD mouse_y) {
    /* Define columns (must match initialization) */
    iTidy_ColumnConfig columns[] = {
        {"Date", 20, 20, ITIDY_ALIGN_LEFT, FALSE, FALSE, 
         ITIDY_SORT_DESCENDING, ITIDY_COLTYPE_DATE},
        {"Size", 10, 12, ITIDY_ALIGN_RIGHT, FALSE, FALSE,
         ITIDY_SORT_NONE, ITIDY_COLTYPE_NUMBER},
        {"Name", 30, 0, ITIDY_ALIGN_LEFT, TRUE, TRUE,
         ITIDY_SORT_NONE, ITIDY_COLTYPE_TEXT}
    };
    
    /* Process click with API */
    iTidy_ListViewEvent event;
    if (iTidy_HandleListViewGadgetUp(
            data->window,
            gadget,
            mouse_x,
            mouse_y,
            &data->entry_list,
            data->display_list,
            data->state,
            prefsIControl.systemFontSize,
            prefsIControl.systemFontCharWidth,
            columns,
            3,
            &event)) {
        
        switch (event.type) {
            case ITIDY_LV_EVENT_HEADER_SORTED:
                /* Sorting happened - refresh ListView */
                GT_SetGadgetAttrs(gadget, data->window, NULL, GTLV_Labels, ~0, TAG_DONE);
                GT_SetGadgetAttrs(gadget, data->window, NULL,
                                 GTLV_Labels, data->display_list,
                                 GTLV_Top, 0,
                                 TAG_DONE);
                break;
                
            case ITIDY_LV_EVENT_NAV_HANDLED:
                /* Page navigation - rebuild list */
                rebuild_listview(data, columns, 3, gadget);
                break;
                
            case ITIDY_LV_EVENT_ROW_CLICK:
                /* Data row clicked - process it */
                process_row_click(data, &event);
                break;
        }
    }
}

void rebuild_listview(MyWindowData *data, iTidy_ColumnConfig *columns, int num_columns, struct Gadget *gadget) {
    /* Save navigation state BEFORE freeing */
    int current_page = data->state ? data->state->current_page : 1;
    int nav_direction = data->state ? data->state->last_nav_direction : 0;
    
    /* Detach and free old display list */
    GT_SetGadgetAttrs(gadget, data->window, NULL, GTLV_Labels, ~0, TAG_DONE);
    
    if (data->display_list) {
        iTidy_FreeFormattedList(data->display_list);
        data->display_list = NULL;
    }
    
    if (data->state) {
        iTidy_FreeListViewState(data->state);
        data->state = NULL;
    }
    
    /* Rebuild with saved navigation state */
    int total_pages;
    data->display_list = iTidy_FormatListViewColumns(
        columns, num_columns,
        &data->entry_list,
        65,
        &data->state,
        ITIDY_MODE_FULL,
        data->page_size,
        current_page,        /* Use saved page */
        &total_pages,
        nav_direction        /* Use saved direction */
    );
    
    /* Reattach and select appropriate row */
    GT_SetGadgetAttrs(gadget, data->window, NULL,
                     GTLV_Labels, data->display_list,
                     GTLV_Selected, data->state ? data->state->auto_select_row : -1,
                     TAG_DONE);
}

void process_row_click(MyWindowData *data, iTidy_ListViewEvent *event) {
    /* Check for double-click */
    struct timeval current_time;
    GetSysTime(&current_time);
    
    ULONG delta_secs = current_time.tv_secs - data->last_click_secs;
    ULONG delta_micros = current_time.tv_micro - data->last_click_micros;
    
    if (delta_secs == 0 && delta_micros < 500000) {  /* 0.5 seconds */
        /* Double-click detected */
        handle_double_click(event->entry);
    } else {
        /* Single click - show details */
        show_details_panel(event->entry);
    }
    
    data->last_click_secs = current_time.tv_secs;
    data->last_click_micros = current_time.tv_micro;
}
```

### Cleanup

```c
void cleanup_window(MyWindowData *data) {
    /* Close window first */
    if (data->window) {
        CloseWindow(data->window);
        data->window = NULL;
    }
    
    /* Free gadgets */
    if (data->gadget_list) {
        FreeGadgets(data->gadget_list);
        data->gadget_list = NULL;
    }
    
    /* Free visual info */
    if (data->visual_info) {
        FreeVisualInfo(data->visual_info);
        data->visual_info = NULL;
    }
    
    /* Free ListView API structures */
    if (data->display_list) {
        iTidy_FreeFormattedList(data->display_list);
        data->display_list = NULL;
    }
    
    if (data->state) {
        iTidy_FreeListViewState(data->state);
        data->state = NULL;
    }
    
    /* Free entry list (YOU own this!) */
    struct Node *node;
    while ((node = RemHead(&data->entry_list))) {
        iTidy_ListViewEntry *entry = (iTidy_ListViewEntry *)node;
        
        /* Free display_data strings */
        for (int i = 0; i < entry->num_columns; i++) {
            if (entry->display_data && entry->display_data[i]) {
                whd_free((void *)entry->display_data[i]);
            }
            if (entry->sort_keys && entry->sort_keys[i]) {
                whd_free((void *)entry->sort_keys[i]);
            }
        }
        
        /* Free arrays */
        if (entry->display_data) whd_free((void *)entry->display_data);
        if (entry->sort_keys) whd_free((void *)entry->sort_keys);
        
        /* Free entry node itself */
        whd_free(entry);
    }
}
```

---

## API Reference Summary

### Core Functions

```c
/* Format ListView with mode control and optional pagination */
struct List *iTidy_FormatListViewColumns(
    iTidy_ColumnConfig *columns,    /* Column definitions */
    int num_columns,                 /* Number of columns */
    struct List *entries,            /* Your entry list */
    int total_char_width,            /* ListView width in characters */
    iTidy_ListViewState **out_state, /* Returns state (NULL for simple modes) */
    iTidy_ListViewMode mode,         /* FULL/FULL_NO_SORT/SIMPLE/SIMPLE_PAGINATED */
    int page_size,                   /* Entries per page (used with paginated modes) */
    int current_page,                /* 1-based page number */
    int *out_total_pages,            /* Returns total pages (can be NULL) */
    int nav_direction                /* -1=Previous, 0=None, +1=Next */
);

/* Handle all ListView clicks (sorting, pagination, data rows) */
BOOL iTidy_HandleListViewGadgetUp(
    struct List *display_list,       /* Formatted list from formatter */
    struct List *entry_list,         /* Original entry list */
    iTidy_ListViewState *state,      /* State from formatter */
    LONG selected,                   /* Row from GTLV_Selected */
    WORD mouse_x,                    /* Mouse X from IntuiMessage */
    WORD gadget_left,                /* Gadget->LeftEdge */
    WORD font_height,                /* prefsIControl.systemFontSize */
    WORD font_width,                 /* prefsIControl.systemFontCharWidth */
    iTidy_ColumnConfig *columns,     /* Column definitions */
    int num_columns,                 /* Number of columns */
    iTidy_ListViewEvent *out_event   /* Returns event information */
);

/* Cleanup functions */
void iTidy_FreeFormattedList(struct List *list);
void iTidy_FreeListViewState(iTidy_ListViewState *state);
```

### Configuration Structures

```c
typedef struct {
    const char *title;               /* Column header text */
    int min_width;                   /* Minimum width in characters */
    int max_width;                   /* Maximum width (0 = no limit) */
    iTidy_TextAlign align;           /* LEFT/RIGHT/CENTER */
    BOOL flexible;                   /* TRUE = absorbs remaining space */
    BOOL truncate;                   /* TRUE = truncate with "..." */
    iTidy_SortOrder default_sort;    /* NONE/ASCENDING/DESCENDING */
    iTidy_ColumnType sort_type;      /* DATE/NUMBER/TEXT */
} iTidy_ColumnConfig;

typedef struct {
    struct Node node;                /* Exec list node (ln_Name used by formatter) */
    int num_columns;                 /* Number of columns */
    const char **display_data;       /* Formatted strings for display */
    const char **sort_keys;          /* Machine-readable sort values */
    iTidy_RowType row_type;          /* DATA/NAV_PREV/NAV_NEXT */
} iTidy_ListViewEntry;
```

### Event Structure

```c
typedef struct {
    iTidy_ListViewEventType type;    /* NONE/HEADER_SORTED/ROW_CLICK/NAV_HANDLED */
    iTidy_ListViewEntry *entry;      /* Selected entry (NULL for header/nav) */
    int column;                      /* Clicked column (-1 if N/A) */
    const char *display_value;       /* Display string (ROW_CLICK only) */
    const char *sort_key;            /* Sort key (ROW_CLICK only) */
    iTidy_ColumnType column_type;    /* Column type (ROW_CLICK only) */
    BOOL did_sort;                   /* TRUE if sort occurred (HEADER_SORTED only) */
    int sorted_column;               /* Which column (HEADER_SORTED only) */
    iTidy_SortOrder sort_order;      /* Sort direction (HEADER_SORTED only) */
} iTidy_ListViewEvent;
```

---

## Best Practices

### Mode and Page Size Selection

**Mode Selection Guide:**
```c
/* Small lists (≤20 entries), no pagination */
mode = ITIDY_MODE_SIMPLE;       /* Fastest, minimal memory */
page_size = 0;                  /* Ignored */

/* Small lists (≤20 entries), need sorting */
mode = ITIDY_MODE_FULL;         /* Full features */
page_size = 0;                  /* No pagination */

/* Large lists (>20 entries), need sorting */
mode = ITIDY_MODE_FULL;         /* Full features */
page_size = 10;                 /* Paginate when > 10 */

/* Large lists (>20 entries), no sorting needed */
mode = ITIDY_MODE_SIMPLE_PAGINATED;  /* Fastest paginated mode */
page_size = 10;                      /* 10 entries per page */

/* Disable sorting for performance (rare) */
mode = ITIDY_MODE_FULL_NO_SORT;  /* State allocated, sorting disabled */
page_size = 0;                   /* Optional pagination */
```

**Page Size Recommendations by Hardware:**

```c
/* Conservative (68000 @ 7MHz, 2MB RAM) */
page_size = 10;

/* Moderate (68020 @ 14MHz, 4MB RAM) */
page_size = 20;

/* Aggressive (68030+, 8MB+ RAM) */
page_size = 50;

/* Simple modes with pagination */
page_size = 4-10;  /* Small pages for responsiveness */
```

### 2. Column Configuration

**Always have exactly one flexible column:**
```c
iTidy_ColumnConfig columns[] = {
    {"Date", 20, 20, ITIDY_ALIGN_LEFT, FALSE, FALSE, ...},
    {"Size", 10, 12, ITIDY_ALIGN_RIGHT, FALSE, FALSE, ...},
    {"Name", 30, 0, ITIDY_ALIGN_LEFT, TRUE, TRUE, ...}  /* flexible=TRUE */
};
```

**Default sort column:**
```c
/* Set ONE column to default_sort != ITIDY_SORT_NONE */
{"Date", 20, 20, ITIDY_ALIGN_LEFT, FALSE, FALSE, 
 ITIDY_SORT_DESCENDING,  /* This column sorted by default */
 ITIDY_COLTYPE_DATE}
```

### 3. Memory Management

**Critical order:**
1. Detach list from gadget: `GT_SetGadgetAttrs(..., GTLV_Labels, ~0, ...)`
2. Free display_list: `iTidy_FreeFormattedList(display_list)`
3. Free state: `iTidy_FreeListViewState(state)`
4. Free entry_list: Your responsibility (see cleanup example)

**Common mistakes:**
- ❌ Freeing state before saving `current_page` and `last_nav_direction`
- ❌ Not freeing `entry->node.ln_Name` (formatter allocates this)
- ❌ Passing `&display_list` instead of `display_list` to GTLV_Labels

### 4. Navigation State Preservation

**Always save before freeing:**
```c
/* CORRECT */
int current_page = state->current_page;
int nav_direction = state->last_nav_direction;
iTidy_FreeListViewState(state);
/* Now use saved values */

/* WRONG */
iTidy_FreeListViewState(state);
int current_page = state->current_page;  /* Use-after-free! */
```

### 5. Mouse Coordinates

**Capture BEFORE replying to message:**
```c
/* CORRECT */
WORD mouse_x = msg->MouseX;
WORD mouse_y = msg->MouseY;
GT_ReplyIMsg(msg);
/* Now use mouse_x */

/* WRONG */
GT_ReplyIMsg(msg);
WORD mouse_x = msg->MouseX;  /* Message freed - garbage data! */
```

### 6. Column Definitions

**Keep column definitions consistent:**
```c
/* Define once as static const */
static iTidy_ColumnConfig session_columns[] = {
    {"Date", 20, 20, ITIDY_ALIGN_LEFT, FALSE, FALSE, 
     ITIDY_SORT_DESCENDING, ITIDY_COLTYPE_DATE},
    /* ... */
};

/* Use same definition everywhere */
display_list = iTidy_FormatListViewColumns(session_columns, 3, ...);
iTidy_HandleListViewGadgetUp(..., session_columns, 3, ...);
```

---

## Troubleshooting

### Issue: Sorting doesn't work

**Symptom:** Clicking headers does nothing

**Causes:**
1. Pagination is active (entry count > page_size)
   - **Solution:** This is expected behavior - increase page_size or disable pagination
   - **Check logs:** Look for "Auto-Pagination ACTIVE" message

2. Not handling `ITIDY_LV_EVENT_HEADER_SORTED`
   - **Solution:** Add case for this event type and refresh ListView

3. Wrong column definition passed to event handler
   - **Solution:** Use exact same `iTidy_ColumnConfig` array for format and event handling

### Issue: Navigation rows don't appear

**Symptom:** Large list shows all entries, no Previous/Next rows

**Causes:**
1. Auto-pagination disabled (entry count ≤ page_size)
   - **Check logs:** Look for "Auto-Pagination DISABLED" message
   - **Solution:** Reduce page_size or add more entries

2. page_size set to 0
   - **Solution:** Set page_size to non-zero value (e.g., 10)

### Issue: Clicking navigation rows crashes

**Symptom:** Guru meditation when clicking Previous/Next

**Causes:**
1. Not saving navigation state before freeing
   - **Solution:** Save `current_page` and `last_nav_direction` BEFORE calling `iTidy_FreeListViewState()`

2. Not handling `ITIDY_LV_EVENT_NAV_HANDLED`
   - **Solution:** Add case for NAV_HANDLED and rebuild list

### Issue: Wrong row selected after navigation

**Symptom:** After Previous/Next, wrong row is highlighted

**Causes:**
1. Not using `state->auto_select_row`
   - **Solution:** Use this value when setting GTLV_Selected after rebuild

2. Passing wrong nav_direction to formatter
   - **Solution:** Pass saved `last_nav_direction` from state

### Issue: ListView shows garbage

**Symptom:** Garbled text or system crash

**Causes:**
1. Passing `&display_list` instead of `display_list` to GTLV_Labels
   - **Solution:** Use `GTLV_Labels, display_list` (not `&display_list`)

2. Using display_list after freeing
   - **Solution:** Set to NULL after freeing, check before use

3. Entry list freed while still attached to gadget
   - **Solution:** Detach first with `GTLV_Labels, ~0`

---

## Migration from Old Helper

If you're using the old `iTidy_GetListViewClick()` helper:

**Old code:**
```c
iTidy_ListViewClick click = iTidy_GetListViewClick(...);
if (click.entry && click.column >= 0) {
    process_click(&click);
}
```

**New code:**
```c
iTidy_ListViewEvent event;
if (iTidy_HandleListViewGadgetUp(..., &event)) {
    if (event.type == ITIDY_LV_EVENT_ROW_CLICK) {
        /* event has same fields as old click structure */
        process_click(event.entry, event.column, event.display_value);
    }
}
```

**Benefits of migration:**
- Automatic sorting support
- Automatic pagination support
- No need to manually rebuild list after sorting
- Consistent event handling pattern

---

## Summary

The ListView API provides:

✅ **Auto-pagination** - Automatically enables pagination for large lists, keeps sorting for small lists  
✅ **Event-driven** - Single handler function returns structured events (HEADER_SORTED, ROW_CLICK, NAV_HANDLED)  
✅ **Performance** - Page-based slicing reduces format time from 4.7s to 160ms for 300 entries  
✅ **Smart navigation** - Auto-selection after page changes maintains visual stability  
✅ **Type-aware** - Column types (DATE/NUMBER/TEXT) provided automatically  
✅ **Dual values** - Both display and sort values available for each cell  
✅ **NULL-safe** - Designed to handle all edge cases gracefully  
✅ **Reusable** - Works for any ListView with minimal boilerplate  

**Recommendation:** Use this API for all multi-column ListViews. It's simpler, safer, faster, and more maintainable than manual approaches.
