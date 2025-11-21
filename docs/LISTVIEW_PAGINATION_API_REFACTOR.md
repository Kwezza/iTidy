# ListView Pagination API Refactoring - Complete

## Overview

Refactored the ListView pagination system to move navigation logic from the caller into the API layer. This simplifies client code and centralizes pagination state management.

**Status**: ✅ COMPLETE - Compiled successfully

---

## What Changed

### Before (Caller-Managed Pagination):
```c
/* Caller had to track pagination state */
struct RestoreWindow {
    int current_page;
    int total_pages;
    int page_size;
    int last_nav_direction;  /* Track where to auto-select */
};

/* Event handler had to detect navigation clicks manually */
case ITIDY_LV_EVENT_ROW_CLICK:
    if (event.entry->row_type == ITIDY_ROW_NAV_PREV) {
        restore_data->current_page--;
        restore_data->last_nav_direction = -1;
        populate_run_list(...);  /* Rebuild */
    }
    else if (event.entry->row_type == ITIDY_ROW_NAV_NEXT) {
        restore_data->current_page++;
        restore_data->last_nav_direction = +1;
        populate_run_list(...);
    }
    /* Process normal data rows... */

/* Populate function calculated auto-select row manually */
if (restore_data->last_nav_direction < 0) {
    select_row = 2;  /* Top */
} else {
    select_row = row_count - 1;  /* Bottom */
}
```

### After (API-Managed Pagination):
```c
/* Caller only needs page size and initial page */
struct RestoreWindow {
    int current_page;  /* Only for first format call */
    int page_size;     /* Entries per page */
    /* total_pages, last_nav_direction moved to API state */
};

/* Event handler gets new event type for navigation */
case ITIDY_LV_EVENT_NAV_HANDLED:
    /* Navigation already handled by API, just rebuild */
    append_to_log("Page navigation: now on page %d of %d\n",
                 state->current_page, state->total_pages);
    populate_run_list(...);  /* API already updated state */
    break;

case ITIDY_LV_EVENT_ROW_CLICK:
    /* Only process actual data rows - navigation filtered out */
    if (event.entry != NULL) {
        /* Process data row normally... */
    }

/* Populate function uses API-calculated auto-select row */
if (state != NULL && state->auto_select_row >= 0) {
    select_row = state->auto_select_row;  /* API calculated this */
}
```

---

## Modified Files

### 1. **src/helpers/listview_columns_api.h**
- Added `ITIDY_LV_EVENT_NAV_HANDLED` event type
- Added pagination state to `iTidy_ListViewState`:
  ```c
  typedef struct {
      /* Existing fields... */
      
      /* Pagination state (managed internally by API) */
      int current_page;        /* Current page (1-based, 0 = no pagination) */
      int total_pages;         /* Total pages (1 = no pagination) */
      int last_nav_direction;  /* -1=Previous, +1=Next, 0=None/Sort/Initial */
      int auto_select_row;     /* Row to auto-select after navigation (-1=none) */
  } iTidy_ListViewState;
  ```

### 2. **src/helpers/listview_columns_api.c**

#### Formatter Changes (`iTidy_FormatListViewColumns`):
- Initialize pagination state when state is created:
  ```c
  state->current_page = current_page;
  state->total_pages = total_pages;
  state->last_nav_direction = 0;  /* Reset on re-format */
  state->auto_select_row = -1;
  ```
- Calculate auto-select row at end of formatting:
  ```c
  /* Calculate which row to select based on navigation direction */
  if (state->last_nav_direction < 0) {
      /* Previous clicked - select row 2 (first data row) */
      state->auto_select_row = 2;
  } else if (state->last_nav_direction > 0 && display_row_count > 2) {
      /* Next clicked - select last row */
      state->auto_select_row = display_row_count - 1;
  }
  ```

#### Event Handler Changes (`iTidy_HandleListViewGadgetUp`):
- Detect navigation row clicks and handle internally:
  ```c
  if (clicked_entry->row_type == ITIDY_ROW_NAV_PREV && state->current_page > 1) {
      state->current_page--;
      state->last_nav_direction = -1;
      /* Return ITIDY_LV_EVENT_NAV_HANDLED */
  }
  else if (clicked_entry->row_type == ITIDY_ROW_NAV_NEXT && state->current_page < state->total_pages) {
      state->current_page++;
      state->last_nav_direction = +1;
      /* Return ITIDY_LV_EVENT_NAV_HANDLED */
  }
  ```
- Return new event type to signal caller to rebuild list
- Data row clicks return `ITIDY_LV_EVENT_ROW_CLICK` as before

### 3. **src/GUI/restore_window.h**
- Removed `total_pages` and `last_nav_direction` fields
- Updated comments: `current_page` is "initial page for first load"
- Kept `page_size` for caller configuration

### 4. **src/GUI/restore_window.c**

#### Event Handler:
- Added `case ITIDY_LV_EVENT_NAV_HANDLED:` to rebuild list after navigation
- Removed manual navigation detection from `ITIDY_LV_EVENT_ROW_CLICK` case
- Data row handling is now cleaner (no navigation checks)

#### populate_run_list():
- Uses `state->current_page` if state exists (navigation updated it)
- Uses `state->auto_select_row` for selection (API calculated it)
- Removed manual row counting and selection logic
- Changed to local variable for `total_pages` output parameter

#### Initialization:
- Removed `last_nav_direction = 0` initialization (API manages it)
- Removed `total_pages = 1` initialization (not used anymore)

---

## Benefits of Refactoring

### 1. **Simpler Caller Code**
- Caller doesn't need to track `last_nav_direction` or `total_pages`
- Event handler is cleaner (one case for navigation, one for data)
- No manual row counting or auto-select calculation

### 2. **Better Encapsulation**
- All pagination logic lives in the API
- State management is centralized
- Easier to change pagination behavior without updating callers

### 3. **Clearer Event Types**
- `ITIDY_LV_EVENT_NAV_HANDLED` clearly signals "API handled this, just rebuild"
- `ITIDY_LV_EVENT_ROW_CLICK` now only fires for data rows
- No need to check `row_type` in caller

### 4. **Reusability**
- Other windows can use pagination by just:
  1. Passing `page_size` to formatter
  2. Handling `ITIDY_LV_EVENT_NAV_HANDLED` event
  3. Using `state->auto_select_row` for selection
- No need to duplicate pagination tracking logic

---

## Usage Pattern (For Other Windows)

```c
/* 1. Define page size in your window structure */
struct MyWindow {
    int page_size;  /* e.g., 10 entries per page */
    iTidy_ListViewState *state;
    /* ... */
};

/* 2. Format with pagination */
void populate_list(struct MyWindow *win) {
    int current_page = (win->state != NULL) ? win->state->current_page : 1;
    int total_pages = 0;
    
    win->formatted_list = iTidy_FormatListViewColumns(
        columns, num_cols, &win->entry_list, width,
        &win->state,
        win->page_size,    /* Enable pagination */
        current_page,      /* Current page (API updates this) */
        &total_pages       /* Receives total pages */
    );
    
    /* Use API-calculated auto-select row */
    int select_row = (win->state && win->state->auto_select_row >= 0) 
                     ? win->state->auto_select_row 
                     : 2;  /* Default to first data row */
    
    GT_SetGadgetAttrs(win->listview, win->window, NULL,
        GTLV_Labels, win->formatted_list,
        GTLV_Selected, select_row,
        TAG_DONE);
}

/* 3. Handle events */
case IDCMP_GADGETUP:
    iTidy_ListViewEvent event;
    if (iTidy_HandleListViewGadgetUp(..., &event)) {
        switch (event.type) {
            case ITIDY_LV_EVENT_NAV_HANDLED:
                /* API changed page, just rebuild */
                populate_list(win);
                break;
                
            case ITIDY_LV_EVENT_ROW_CLICK:
                /* Process data row click */
                handle_row_click(event.entry);
                break;
        }
    }
```

---

## Testing Notes

- Compile status: ✅ SUCCESS (warnings only)
- Build output: `build_output_latest.txt`
- Ready for testing in WinUAE

**Test Cases:**
1. Previous button: should navigate to previous page, select row 2 (top)
2. Next button: should navigate to next page, select last row (bottom)
3. Page edges: Previous on page 1 should be ignored, Next on last page should be ignored
4. Data row clicks: should work normally without triggering navigation
5. Sorting: should preserve current page but reset navigation direction

---

## Architecture Summary

```
┌─────────────────────────────────────────┐
│  Caller (restore_window.c)              │
├─────────────────────────────────────────┤
│  - Calls formatter with page_size       │
│  - Handles ITIDY_LV_EVENT_NAV_HANDLED   │
│  - Uses state->auto_select_row          │
│  - Processes data row clicks            │
└──────────────────┬──────────────────────┘
                   │
                   │ iTidy_FormatListViewColumns()
                   │ iTidy_HandleListViewGadgetUp()
                   ▼
┌─────────────────────────────────────────┐
│  API (listview_columns_api.c)           │
├─────────────────────────────────────────┤
│  - Manages state->current_page          │
│  - Manages state->last_nav_direction    │
│  - Calculates state->auto_select_row    │
│  - Detects navigation clicks            │
│  - Returns ITIDY_LV_EVENT_NAV_HANDLED   │
└─────────────────────────────────────────┘
```

**Flow on Next Click:**
1. User clicks "Next" row in ListView
2. `iTidy_HandleListViewGadgetUp()` detects it's a navigation row
3. API increments `state->current_page`
4. API sets `state->last_nav_direction = +1`
5. API returns `ITIDY_LV_EVENT_NAV_HANDLED` to caller
6. Caller calls `populate_run_list()`
7. Formatter gets `state->current_page` (incremented by API)
8. Formatter calculates `state->auto_select_row = last_row` (because `last_nav_direction = +1`)
9. Caller uses `state->auto_select_row` for `GTLV_Selected`
10. ListView shows new page with last row selected

---

## Files Modified

- `src/helpers/listview_columns_api.h` - Added event type and state fields
- `src/helpers/listview_columns_api.c` - Implemented internal navigation handling
- `src/GUI/restore_window.h` - Removed now-redundant fields
- `src/GUI/restore_window.c` - Simplified event handling and populate logic

## Author
AI-assisted refactoring  
Date: 2025  
Build: VBCC cross-compile for AmigaOS 3.x (68000)
