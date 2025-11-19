# ListView Smart Helper - Usage Guide

## Overview

The `iTidy_GetListViewClick()` smart helper function provides a unified way to handle ListView clicks with automatic:
- Header row detection (returns NULL for header clicks)
- Column detection (pixel-based hit-testing)
- Value extraction (both display and sort values)
- Type information (DATE/NUMBER/TEXT)

This eliminates the need for callers to manually handle header offsets, traverse sorted lists, or maintain column type mappings.

---

## Quick Start Example

```c
#include "GUI/listview_formatter.h"

/* In your GADGETUP handler: */
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        LONG selected = -1;
        GT_GetGadgetAttrs(gadget, window, NULL, GTLV_Selected, &selected, TAG_END);
        
        /* Get complete click information in one call */
        iTidy_ListViewClick click = iTidy_GetListViewClick(
            &data->entry_list,           /* Your entry list */
            data->lv_state,               /* State from formatter */
            selected,                     /* Row from GTLV_Selected */
            msg->MouseX,                  /* Mouse X from IntuiMessage */
            gadget->LeftEdge              /* Gadget position */
        );
        
        /* Check if valid data row was clicked */
        if (click.entry != NULL && click.column >= 0) {
            printf("Clicked: Column %d, Type %d, Value: '%s'\n",
                   click.column,
                   click.column_type,
                   click.display_value);
        }
    }
    break;
```

---

## The iTidy_ListViewClick Structure

```c
typedef struct {
    iTidy_ListViewEntry *entry;       /* Selected entry (NULL if header row) */
    int column;                        /* Clicked column index (0-based, -1 if invalid) */
    const char *display_value;         /* Human-readable: "2.8 MB", "24-Nov-2025" */
    const char *sort_key;              /* Machine-readable: "0002949120", "20251124_151900" */
    iTidy_ColumnType column_type;      /* DATE/NUMBER/TEXT */
} iTidy_ListViewClick;
```

### Field Meanings

| Field | Description | When NULL/-1 | Example Values |
|-------|-------------|--------------|----------------|
| `entry` | Full entry pointer | Header row clicked | Valid `iTidy_ListViewEntry*` |
| `column` | Column index | Click outside bounds | `0`, `1`, `2`, `-1` |
| `display_value` | Formatted string | Invalid entry/column | `"2.8 MB"`, `"24-Nov-2025"` |
| `sort_key` | Machine value | Invalid entry/column | `"0002949120"`, `"20251124_151900"` |
| `column_type` | Data type | Never NULL | `ITIDY_COLTYPE_DATE`, `ITIDY_COLTYPE_NUMBER`, `ITIDY_COLTYPE_TEXT` |

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

## Use Cases

### 1. Copy Column Value to Clipboard

```c
if (click.entry != NULL && click.column >= 0) {
    /* Copy formatted display value */
    copy_to_clipboard(click.display_value);
    
    show_notification("Copied: %s", click.display_value);
}
```

### 2. Column-Specific Details Panel

```c
if (click.entry != NULL && click.column >= 0) {
    /* Show different details based on clicked column */
    switch (click.column) {
        case 0:  /* Date column */
            show_date_details_panel(click.sort_key);  /* Use sort_key for parsing */
            break;
        case 1:  /* Size column */
            show_size_breakdown(atoi(click.sort_key));
            break;
        case 2:  /* Path column */
            open_path_in_folder(click.sort_key);  /* Full path in sort_key */
            break;
    }
}
```

### 3. Context Menu with Column-Aware Options

```c
if (click.entry != NULL && click.column >= 0) {
    /* Build context menu based on column type */
    struct MenuItem *menu_items[10];
    int item_count = 0;
    
    /* Add column-type-specific options */
    if (click.column_type == ITIDY_COLTYPE_DATE) {
        menu_items[item_count++] = create_menu_item("Show in Calendar...", MENU_CALENDAR);
        menu_items[item_count++] = create_menu_item("Copy Timestamp", MENU_COPY_TIMESTAMP);
    } else if (click.column_type == ITIDY_COLTYPE_NUMBER) {
        menu_items[item_count++] = create_menu_item("Edit Value...", MENU_EDIT_NUMBER);
        menu_items[item_count++] = create_menu_item("Copy Number", MENU_COPY_NUMBER);
    }
    
    /* Always add generic options */
    menu_items[item_count++] = create_menu_item("Copy Display Text", MENU_COPY_DISPLAY);
    
    show_context_menu(menu_items, item_count, msg->MouseX, msg->MouseY);
}
```

### 4. Type-Safe Data Extraction

```c
if (click.entry != NULL && click.column >= 0) {
    /* Extract data in type-safe manner */
    switch (click.column_type) {
        case ITIDY_COLTYPE_DATE: {
            /* Parse YYYYMMDD_HHMMSS format */
            struct DateStamp ds;
            if (parse_amiga_timestamp(click.sort_key, &ds)) {
                process_date(&ds);
            }
            break;
        }
        
        case ITIDY_COLTYPE_NUMBER: {
            /* Parse as integer */
            long value = atoi(click.sort_key);
            process_number(value);
            break;
        }
        
        case ITIDY_COLTYPE_TEXT: {
            /* Process as string */
            process_text(click.display_value);  /* Or click.sort_key for full text */
            break;
        }
    }
}
```

---

## Comparing Old vs. New Approach

### OLD WAY (Manual - Error Prone)

```c
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        LONG selected;
        GT_GetGadgetAttrs(gadget, window, NULL, GTLV_Selected, &selected, TAG_END);
        
        /* Manual header offset */
        if (selected < 2) {
            return;  /* Header row */
        }
        
        /* Manual index adjustment */
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
        
        /* Manual type checking (caller maintains mapping!) */
        if (entry && column == 0) {  /* Caller knows column 0 is date */
            parse_date(entry->sort_keys[0]);
        }
    }
    break;
```

**Issues:**
- ❌ 30+ lines of boilerplate
- ❌ Easy to forget header offset
- ❌ Caller must maintain column→type mapping
- ❌ Code duplicated across every ListView
- ❌ No NULL safety checks
- ❌ Hard to maintain if columns change

### NEW WAY (Smart Helper - Clean)

```c
case IDCMP_GADGETUP:
    if (gadget->GadgetID == GID_MY_LISTVIEW) {
        LONG selected;
        GT_GetGadgetAttrs(gadget, window, NULL, GTLV_Selected, &selected, TAG_END);
        
        /* One call, everything automatic */
        iTidy_ListViewClick click = iTidy_GetListViewClick(
            &entry_list, state, selected, msg->MouseX, gadget->LeftEdge
        );
        
        /* Type-aware processing */
        if (click.entry && click.column >= 0) {
            if (click.column_type == ITIDY_COLTYPE_DATE) {
                parse_date(click.sort_key);
            }
        }
    }
    break;
```

**Benefits:**
- ✅ 10 lines instead of 30+
- ✅ Automatic header offset handling
- ✅ Automatic column type detection
- ✅ NULL safety built-in
- ✅ Reusable across all ListViews
- ✅ Self-documenting code

---

## NULL Safety

The helper is designed to be safely usable without excessive NULL checking:

```c
/* Safe to call - will return safe defaults */
iTidy_ListViewClick click = iTidy_GetListViewClick(
    NULL,      /* NULL entry_list -> entry=NULL */
    NULL,      /* NULL state -> column=-1 */
    selected,
    mouse_x,
    gadget_left
);

/* Simple validation */
if (click.entry != NULL && click.column >= 0) {
    /* Both entry and column are valid - safe to use */
    process_click(&click);
}

/* Can also check individual fields */
if (click.entry == NULL) {
    printf("Header row clicked or invalid selection\n");
}

if (click.column == -1) {
    printf("Click outside column bounds\n");
}

if (click.display_value == NULL) {
    printf("No valid data at this position\n");
}
```

---

## Performance

**Cost:** Negligible (< 1ms on 7MHz Amiga)

The helper performs:
1. One list traversal (O(n), but n = selected row number, typically < 20)
2. One loop over columns (O(columns), typically 3-5 columns)
3. A few pointer dereferences

**Overhead vs. manual approach:** Zero (you'd do the same operations manually)

---

## When NOT to Use the Smart Helper

The smart helper is designed for interactive click handling. Don't use it for:

❌ **Batch processing** - Use `iTidy_GetSelectedEntry()` directly if you don't need column detection
❌ **Programmatic selection** - If you're setting selection programmatically, you already know the column
❌ **Performance-critical loops** - If processing 100+ clicks in a loop (unlikely in UI code)

For these cases, use the lower-level functions:
- `iTidy_GetSelectedEntry()` - Just get entry from row index
- `iTidy_GetClickedColumn()` - Just get column from mouse X

---

## Integration Checklist

To use the smart helper in your window:

1. **Include header:**
   ```c
   #include "GUI/listview_formatter.h"
   ```

2. **Store state in window data:**
   ```c
   typedef struct {
       struct List entry_list;
       struct List *display_list;
       iTidy_ListViewState *lv_state;  /* Important! */
       /* ... other fields ... */
   } MyWindowData;
   ```

3. **Capture state during formatting:**
   ```c
   data->display_list = iTidy_FormatListViewColumns(
       columns, num_columns, &data->entry_list, width, &data->lv_state
   );
   ```

4. **Extract mouse coords in event loop:**
   ```c
   WORD mouseX = msg->MouseX;  /* BEFORE GT_ReplyIMsg() */
   WORD mouseY = msg->MouseY;
   GT_ReplyIMsg(msg);
   ```

5. **Use helper in GADGETUP:**
   ```c
   case IDCMP_GADGETUP:
       if (gadget->GadgetID == GID_MY_LISTVIEW) {
           LONG selected;
           GT_GetGadgetAttrs(gadget, window, NULL, GTLV_Selected, &selected, TAG_END);
           
           iTidy_ListViewClick click = iTidy_GetListViewClick(
               &data->entry_list, data->lv_state, selected,
               mouseX, gadget->LeftEdge
           );
           
           if (click.entry && click.column >= 0) {
               handle_click(&click);
           }
       }
       break;
   ```

6. **Cleanup state on window close:**
   ```c
   if (data->lv_state) {
       iTidy_FreeListViewState(data->lv_state);
       data->lv_state = NULL;
   }
   ```

---

## Summary

The `iTidy_GetListViewClick()` smart helper:

✅ **Eliminates boilerplate** - 30+ lines reduced to 10  
✅ **Prevents bugs** - No manual header offset, no manual list traversal  
✅ **Type-aware** - Column types provided automatically  
✅ **Dual values** - Both display and sort values available  
✅ **NULL-safe** - Designed to handle all edge cases  
✅ **Reusable** - Works for any ListView using the formatter  
✅ **Zero overhead** - Same operations you'd do manually  

**Recommendation:** Use this for all interactive ListView click handling. It's simpler, safer, and more maintainable than manual approaches.
