/**
 * listview_columns_api.c - ListView Column Layout & Sorting API Implementation
 * 
 * Two-pass algorithm:
 * Pass 1: Scan all data to determine optimal column widths
 * Pass 2: Format and create display nodes with proper alignment
 * 
 * ============================================================================
 * FEATURE: Intelligent Path Abbreviation (Added 2025-11-18)
 * ============================================================================
 * 
 * Columns can be marked with is_path=TRUE to enable Amiga-style path abbreviation
 * using "/../" notation instead of simple "..." truncation. This dramatically
 * improves readability for file paths in ListViews.
 * 
 * Example - Before (standard truncation):
 *   "PC:Workbench/Tools/Programs/Copy_Of_m8.ww..."
 * 
 * Example - After (path abbreviation):
 *   "PC:Workbench/../Copy_Of_m8.ww.info"
 * 
 * The user can now see:
 *   ✓ Device name (PC:)
 *   ✓ First directory (Workbench)
 *   ✓ Full filename (Copy_Of_m8.ww.info)
 * 
 * Usage:
 *   columns[2].is_path = TRUE;  // Enable for path column
 *   columns[2].is_path = FALSE; // Standard truncation for other columns
 * 
 * ============================================================================
 * IMPORTANT USAGE NOTES FOR AI AGENTS:
 * ============================================================================
 * 
 * 1. TIMING - Calculate ListView Width BEFORE Creating Window:
 *    ❌ WRONG: Access gadget->Width after window created (CRASH - Guru 8100 0005)
 *    ✅ RIGHT: Calculate width from NewGadget dimensions BEFORE OpenWindowTags()
 * 
 *    Example (from default_tool_restore_window.c):
 *    ```c
 *    // STEP 1: Calculate ListView width from NewGadget (before window)
 *    ng.ng_LeftEdge = 10;
 *    ng.ng_Width = win_width - 20;  // 600 - 20 = 580 pixels
 *    list_width = ng.ng_Width;      // Save this BEFORE window creation!
 * 
 *    // STEP 2: Convert pixels to characters
 *    UWORD font_char_width = prefsIControl.systemFontCharWidth;  // e.g., 8 pixels
 *    data->listview_width = (list_width - 36) / font_char_width;
 *    // Result: (580 - 36) / 8 = 68 characters
 * 
 *    // STEP 3: Create window (gadget->Width now accessible but we don't need it)
 *    window = OpenWindowTags(...);
 *    ```
 * 
 * 2. WIDTH UNITS - Must Pass CHARACTER Width, NOT Pixel Width:
 *    ❌ WRONG: iTidy_FormatListViewColumns(cols, 4, rows, 10, 584, ...)  // Pixels!
 *    ✅ RIGHT: iTidy_FormatListViewColumns(cols, 4, rows, 10, 73, ...)   // Characters!
 * 
 *    The 36-pixel deduction accounts for:
 *    - Scrollbar: ~20 pixels
 *    - Borders/padding: ~16 pixels
 * 
 * 3. FONT METRICS - Use Cached Font Info from prefsIControl:
 *    ```c
 *    #include "../Settings/IControlPrefs.h"
 *    
 *    UWORD char_width = prefsIControl.systemFontCharWidth;  // tf_XSize from font
 *    int listview_chars = (pixel_width - 36) / char_width;
 *    ```
 * 
 * 4. MEMORY - CRITICAL: Complete Cleanup Required When Window Closes:
 *    
 *    Three separate data structures must be freed - each has different ownership:
 *    
 *    a) Display List (owned by formatter):
 *       ```c
 *       if (data->session_display_list) {
 *           iTidy_FreeFormattedList(data->session_display_list);
 *           data->session_display_list = NULL;
 *       }
 *       ```
 *    
 *    b) ListView State (owned by formatter):
 *       ```c
 *       if (data->session_lv_state) {
 *           iTidy_FreeListViewState(data->session_lv_state);
 *           data->session_lv_state = NULL;
 *       }
 *       ```
 *    
 *    c) Entry List (owned by caller - YOU must free this!):
 *       ⚠️ MOST COMMON LEAK - The entry list is NOT freed by formatter!
 *       Each iTidy_ListViewEntry has 11 allocations that must be freed:
 *       ```c
 *       struct Node *node;
 *       while ((node = RemHead(&data->session_entry_list)) != NULL) {
 *           iTidy_ListViewEntry *entry = (iTidy_ListViewEntry *)node;
 *           int i;
 *           
 *           // 1. Free formatted display string (ln_Name)
 *           if (entry->node.ln_Name) {
 *               whd_free(entry->node.ln_Name);  // ~92 bytes - set by formatter
 *           }
 *           
 *           // 2-5. Free display_data strings (4 columns)
 *           for (i = 0; i < entry->num_columns; i++) {
 *               if (entry->display_data && entry->display_data[i]) {
 *                   whd_free((void *)entry->display_data[i]);
 *               }
 *               // 6-9. Free sort_keys strings (4 columns)
 *               if (entry->sort_keys && entry->sort_keys[i]) {
 *                   whd_free((void *)entry->sort_keys[i]);
 *               }
 *           }
 *           
 *           // 10. Free display_data array
 *           if (entry->display_data) whd_free((void *)entry->display_data);
 *           // 11. Free sort_keys array
 *           if (entry->sort_keys) whd_free((void *)entry->sort_keys);
 *           
 *           // 12. Free entry node itself
 *           whd_free(entry);
 *       }
 *       ```
 *    
 *    Memory Leak Warning:
 *    - Failing to free entry->node.ln_Name causes 5 leaks of ~92 bytes each (460 bytes total)
 *    - Failing to free entire entry list causes 100+ leaks (3000+ bytes)
 *    - Always use memory tracking (#define DEBUG_MEMORY_TRACKING) during development
 * 
 * 5. SORTING - Click-to-Sort Implementation:
 *    
 *    To enable column header sorting, you must:
 *    
 *    a) Detect header clicks in IDCMP_GADGETUP event:
 *       ```c
 *       if (selected_index < 2) {  // Indices 0-1 are header rows
 *           // Calculate which column was clicked using mouse X position
 *           WORD mouse_x = msg->MouseX;
 *           WORD relative_x = mouse_x - gadget->LeftEdge;
 *           int char_pos = relative_x / font_char_width;
 *           
 *           // Find column containing char_pos
 *           for (int i = 0; i < num_columns; i++) {
 *               if (char_pos >= state->columns[i].char_start &&
 *                   char_pos < state->columns[i].char_end) {
 *                   clicked_column = i;
 *                   break;
 *               }
 *           }
 *       }
 *       ```
 *    
 *    b) Sort entry list BEFORE formatting:
 *       ```c
 *       iTidy_SortListViewEntries(&entry_list, column, type, ascending);
 *       ```
 *    
 *    c) Set columns[col].default_sort BEFORE calling formatter:
 *       ⚠️ CRITICAL: Formatter reads default_sort to add arrow indicators (^ v)
 *       ```c
 *       // Clear all columns first
 *       for (int i = 0; i < 4; i++) {
 *           columns[i].default_sort = ITIDY_SORT_NONE;
 *       }
 *       // Set only the sorted column
 *       columns[clicked_col].default_sort = new_order;
 *       
 *       // NOW call formatter - header will show arrow
 *       display_list = iTidy_FormatListViewColumns(columns, 4, &entry_list, ...);
 *       ```
 *    
 *    Common Mistakes:
 *    - ❌ Using column cycling instead of mouse position detection
 *    - ❌ Setting default_sort AFTER formatting (arrows won't appear)
 *    - ❌ Passing &display_list instead of display_list to GTLV_Labels (shows garbage)
 *    - ❌ Not toggling direction when same column clicked twice
 * 
 * 6. GADTOOLS LISTVIEW - Critical Tag Usage:
 *    
 *    ❌ WRONG: CreateGadget(LISTVIEW_KIND, ..., GTLV_Labels, &list_pointer, ...)
 *    ✅ RIGHT: CreateGadget(LISTVIEW_KIND, ..., GTLV_Labels, list_pointer, ...)
 *    
 *    GadTools expects struct List *, NOT struct List **!
 *    Passing address-of-pointer causes ListView to read garbage memory.
 *    
 *    Proper refresh after sorting:
 *    ```c
 *    GT_SetGadgetAttrs(listview, window, NULL,
 *                      GTLV_Labels, ~0,  // Detach first
 *                      TAG_DONE);
 *    GT_SetGadgetAttrs(listview, window, NULL,
 *                      GTLV_Labels, display_list,  // Reattach with new data
 *                      GTLV_Top, 0,                // Reset scroll
 *                      GTLV_MakeVisible, 0,        // Force refresh
 *                      TAG_DONE);
 *    ```
 * 
 * 7. COLUMN CONFIG - Example Setup:
 *    ```c
 *    iTidy_ColumnConfig cols[] = {
 *        {"Date/Time", 20, 30, ITIDY_ALIGN_LEFT, FALSE, ITIDY_SORT_DESCENDING, ITIDY_COLTYPE_DATE},
 *        {"Mode", 6, 8, ITIDY_ALIGN_LEFT, FALSE, ITIDY_SORT_NONE, ITIDY_COLTYPE_TEXT},
 *        {"Path", 30, 200, ITIDY_ALIGN_LEFT, TRUE, ITIDY_SORT_NONE, ITIDY_COLTYPE_TEXT},  // Flexible
 *        {"Changed", 7, 12, ITIDY_ALIGN_RIGHT, FALSE, ITIDY_SORT_NONE, ITIDY_COLTYPE_NUMBER}
 *    };
 *    ```
 * 
 * ============================================================================
 */

#include "platform/platform.h"
#include "listview_columns_api.h"
#include "../writeLog.h"
#include "../path_utilities.h"

#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <devices/timer.h>
#include <string.h>
#include <stdio.h>

/* External timer device for performance measurements */
extern struct Device* TimerBase;

/* Separator between columns */
#define COLUMN_SEPARATOR " | "
#define SEPARATOR_WIDTH 3

/*---------------------------------------------------------------------------*/
/* Sorting Support - Stable Merge Sort                                       */
/*---------------------------------------------------------------------------*/

/**
 * @brief Type-aware comparison function for ListView entries
 * 
 * Compares two entries by the specified column using the appropriate
 * comparison method for the column type.
 * 
 * @param a First entry
 * @param b Second entry
 * @param col Column index to compare
 * @param type Column type (TEXT, NUMBER, or DATE)
 * @return <0 if a<b, 0 if a==b, >0 if a>b
 */
static int compare_entries(iTidy_ListViewEntry *a, iTidy_ListViewEntry *b, int col, iTidy_ColumnType type)
{
    const char *key_a;
    const char *key_b;
    
    if (!a || !b) {
        return 0;
    }
    
    /* Get sort keys */
    key_a = (a->sort_keys && col < a->num_columns) ? a->sort_keys[col] : NULL;
    key_b = (b->sort_keys && col < b->num_columns) ? b->sort_keys[col] : NULL;
    
    /* Handle NULL keys - NULL sorts before any content */
    if (key_a == NULL && key_b == NULL) return 0;
    if (key_a == NULL) return -1;
    if (key_b == NULL) return 1;
    
    /* Handle empty strings - empty sorts before any content */
    if (key_a[0] == '\0' && key_b[0] == '\0') return 0;
    if (key_a[0] == '\0') return -1;
    if (key_b[0] == '\0') return 1;
    
    /* Type-specific comparison */
    switch (type) {
        case ITIDY_COLTYPE_NUMBER:
            /* Numeric comparison */
            return atoi(key_a) - atoi(key_b);
            
        case ITIDY_COLTYPE_DATE:
            /* Date comparison - YYYYMMDD_HHMMSS format sorts correctly as string */
            return strcmp(key_a, key_b);
            
        case ITIDY_COLTYPE_TEXT:
        default:
            /* Alphabetical comparison */
            return strcmp(key_a, key_b);
    }
}

/**
 * @brief Merge two sorted sublists
 * 
 * Merges two sorted sublists into a single sorted list.
 * This is the core of the stable merge sort algorithm.
 * 
 * @param left First sorted sublist
 * @param right Second sorted sublist
 * @param col Column to sort by
 * @param type Column type
 * @param ascending TRUE for ascending, FALSE for descending
 * @return Merged sorted list
 */
static struct List *merge_lists(struct List *left, struct List *right, int col, iTidy_ColumnType type, BOOL ascending)
{
    struct List *result;
    struct Node *node_left, *node_right;
    iTidy_ListViewEntry *entry_left, *entry_right;
    int cmp;
    
    result = (struct List *)whd_malloc(sizeof(struct List));
    if (!result) return NULL;
    
    NewList(result);
    
    node_left = left->lh_Head;
    node_right = right->lh_Head;
    
    /* Merge until one list is empty */
    while (node_left->ln_Succ && node_right->ln_Succ) {
        entry_left = (iTidy_ListViewEntry *)node_left;
        entry_right = (iTidy_ListViewEntry *)node_right;
        
        cmp = compare_entries(entry_left, entry_right, col, type);
        
        /* Apply sort direction */
        if (!ascending) {
            cmp = -cmp;
        }
        
        if (cmp <= 0) {
            /* Take from left (stable sort: equal elements keep original order) */
            struct Node *next = node_left->ln_Succ;
            Remove(node_left);
            AddTail(result, node_left);
            node_left = next;
        } else {
            /* Take from right */
            struct Node *next = node_right->ln_Succ;
            Remove(node_right);
            AddTail(result, node_right);
            node_right = next;
        }
    }
    
    /* Append remaining nodes from left list */
    while (node_left->ln_Succ) {
        struct Node *next = node_left->ln_Succ;
        Remove(node_left);
        AddTail(result, node_left);
        node_left = next;
    }
    
    /* Append remaining nodes from right list */
    while (node_right->ln_Succ) {
        struct Node *next = node_right->ln_Succ;
        Remove(node_right);
        AddTail(result, node_right);
        node_right = next;
    }
    
    return result;
}

/**
 * @brief Stable merge sort for ListView entries
 * 
 * Sorts a List of iTidy_ListViewEntry nodes by the specified column.
 * Uses merge sort to guarantee stability (equal elements preserve order).
 * 
 * @param list List to sort (modified in-place)
 * @param col Column index to sort by
 * @param type Column type
 * @param ascending TRUE for ascending, FALSE for descending
 */
void iTidy_SortListViewEntries(struct List *list, int col, iTidy_ColumnType type, BOOL ascending)
{
    struct List *left, *right, *sorted;
    struct Node *node, *mid;
    int count, i;
    struct timeval startTime, endTime;
    ULONG elapsedMicros, elapsedMillis;
    static int sort_depth = 0;  /* Track recursion depth */
    BOOL is_top_level = (sort_depth == 0);
    
    /* Start performance timing (top level only) */
    if (is_top_level && TimerBase) {
        GetSysTime(&startTime);
    }
    
    sort_depth++;
    
    if (!list) {
        sort_depth--;
        return;
    }
    
    /* Count nodes */
    count = 0;
    for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ) {
        count++;
    }
    
    /* Base case: 0 or 1 elements already sorted */
    if (count <= 1) {
        return;
    }
    
    /* Split list in half */
    left = (struct List *)whd_malloc(sizeof(struct List));
    right = (struct List *)whd_malloc(sizeof(struct List));
    
    if (!left || !right) {
        if (left) whd_free(left);
        if (right) whd_free(right);
        return;
    }
    
    NewList(left);
    NewList(right);
    
    /* Move first half to left list */
    for (i = 0; i < count / 2; i++) {
        node = RemHead(list);
        if (node) {
            AddTail(left, node);
        }
    }
    
    /* Move second half to right list */
    while ((node = RemHead(list)) != NULL) {
        AddTail(right, node);
    }
    
    /* Recursively sort sublists */
    iTidy_SortListViewEntries(left, col, type, ascending);
    iTidy_SortListViewEntries(right, col, type, ascending);
    
    /* Merge sorted sublists */
    sorted = merge_lists(left, right, col, type, ascending);
    
    /* Move sorted nodes back to original list */
    if (sorted) {
        while ((node = RemHead(sorted)) != NULL) {
            AddTail(list, node);
        }
        whd_free(sorted);
    }
    
    /* Cleanup temporary lists */
    whd_free(left);
    whd_free(right);
    
    sort_depth--;
    
    /* End performance timing (top level only) */
    if (is_top_level && TimerBase) {
        GetSysTime(&endTime);
        
        /* Calculate elapsed time in microseconds */
        elapsedMicros = ((endTime.tv_secs - startTime.tv_secs) * 1000000) +
                        (endTime.tv_micro - startTime.tv_micro);
        elapsedMillis = elapsedMicros / 1000;
        
        /* Log performance metrics only if enabled */
        if (is_performance_logging_enabled()) {
            const char *type_str = (type == ITIDY_COLTYPE_NUMBER) ? "NUMBER" :
                                   (type == ITIDY_COLTYPE_DATE) ? "DATE" : "TEXT";
            const char *dir_str = ascending ? "ASC" : "DESC";
            
            log_info(LOG_GUI, "[PERF] ListView sort (col %d, %s, %s): %lu.%03lu ms\n",
                     col, type_str, dir_str, elapsedMillis, elapsedMicros % 1000);
            log_info(LOG_GUI, "       Entries: %d\n", count);
            if (count > 0) {
                log_info(LOG_GUI, "       Time per entry: %lu microseconds\n",
                         elapsedMicros / count);
            }
            
            printf("  [TIMING] Sort (%s/%s): %lu.%03lu ms (%d entries)\n",
                   type_str, dir_str, elapsedMillis, elapsedMicros % 1000, count);
        }
    }
}

/*---------------------------------------------------------------------------*/
/* Helper: Calculate actual column widths                                    */
/*---------------------------------------------------------------------------*/

BOOL iTidy_CalculateColumnWidths(
    iTidy_ColumnConfig *columns,
    int num_columns,
    struct List *entries,
    int total_char_width,
    int *out_widths)
{
    int i, col;
    int separator_total;
    int used_width;
    int available_width;
    int flexible_col = -1;
    int flexible_count = 0;  /* Track total flexible columns for validation */
    int max_len;
    struct Node *node;
    iTidy_ListViewEntry *entry;
    struct timeval startTime, endTime;
    ULONG elapsedMicros, elapsedMillis;
    int entry_count = 0;
    
    /* Start performance timing */
    if (TimerBase) {
        GetSysTime(&startTime);
    }
    
    if (!columns || !out_widths || num_columns <= 0) {
        log_error(LOG_GUI, "iTidy_CalculateColumnWidths: Invalid parameters (columns=%p, out_widths=%p, num_columns=%d)\n",
                 columns, out_widths, num_columns);
        return FALSE;
    }
    
    /* SAFETY: Pre-validate column configuration to detect corruption/uninitialized memory */
    flexible_count = 0;
    for (col = 0; col < num_columns; col++) {
        /* Check for NULL title (strong indicator of uninitialized memory) */
        if (columns[col].title == NULL) {
            log_error(LOG_GUI, "iTidy_CalculateColumnWidths: Column %d has NULL title - likely uninitialized!\n", col);
            return FALSE;
        }
        
        /* Check for unreasonable width values (sanity check) */
        if (columns[col].min_width < 0 || columns[col].min_width > 500 ||
            columns[col].max_width < 0 || columns[col].max_width > 500) {
            log_error(LOG_GUI, "iTidy_CalculateColumnWidths: Column %d (%s) has invalid width constraints (min=%d, max=%d)\n",
                     col, columns[col].title, columns[col].min_width, columns[col].max_width);
            return FALSE;
        }
        
        /* Check for invalid alignment (enum should be 0-2) */
        if (columns[col].align < 0 || columns[col].align > 2) {
            log_error(LOG_GUI, "iTidy_CalculateColumnWidths: Column %d (%s) has invalid alignment value %d\n",
                     col, columns[col].title, (int)columns[col].align);
            return FALSE;
        }
        
        /* Count flexible columns */
        if (columns[col].flexible) {
            flexible_count++;
        }
    }
    
    /* SAFETY: Detect multiple flexible columns (indicates uninitialized memory or config error) */
    if (flexible_count > 1) {
        log_error(LOG_GUI, "iTidy_CalculateColumnWidths: Invalid config - %d flexible columns detected!\n", flexible_count);
        log_error(LOG_GUI, "  This usually means the column array is uninitialized or corrupted.\n");
        log_error(LOG_GUI, "  Only ONE column should have flexible=TRUE.\n");
        
        /* Show which columns claim to be flexible */
        for (col = 0; col < num_columns; col++) {
            if (columns[col].flexible) {
                log_error(LOG_GUI, "    Column %d (%s): flexible=TRUE\n", col, columns[col].title);
            }
        }
        return FALSE;
    }
    
    /* Pass 1: Measure each column's required width */
    for (col = 0; col < num_columns; col++) {
        /* Start with title width, reserving space for sort indicator */
        /* Add 2 chars for potential " ^" or " v" to prevent header truncation */
        max_len = columns[col].title ? strlen(columns[col].title) + 2 : 2;
        
        /* Check all entries for this column */
        if (entries) {
            for (node = entries->lh_Head; node->ln_Succ; node = node->ln_Succ) {
                entry = (iTidy_ListViewEntry *)node;
                
                if (entry->display_data && col < entry->num_columns && entry->display_data[col]) {
                    int len = strlen(entry->display_data[col]);
                    if (len > max_len) {
                        max_len = len;
                    }
                }
            }
        }
        
        /* Apply minimum width constraint */
        if (columns[col].min_width > 0 && max_len < columns[col].min_width) {
            max_len = columns[col].min_width;
        }
        
        /* Apply maximum width constraint */
        if (columns[col].max_width > 0 && max_len > columns[col].max_width) {
            max_len = columns[col].max_width;
        }
        
        out_widths[col] = max_len;
        
        /* Track first flexible column (we already validated there's only 0 or 1) */
        if (columns[col].flexible) {
            if (flexible_col >= 0) {
                /* This should never happen now due to pre-validation, but log if it does */
                log_warning(LOG_GUI, "Multiple flexible columns detected (col %d and %d), using first\n", 
                           flexible_col, col);
            } else {
                flexible_col = col;
            }
        }
    }
    
    /* Calculate total separator width */
    separator_total = (num_columns - 1) * SEPARATOR_WIDTH;
    
    /* Calculate used width */
    used_width = separator_total;
    for (col = 0; col < num_columns; col++) {
        used_width += out_widths[col];
    }
    
    /* If we have a total width constraint and a flexible column, adjust */
    if (total_char_width > 0 && flexible_col >= 0) {
        available_width = total_char_width - separator_total;
        
        /* Calculate width used by non-flexible columns */
        int fixed_width = 0;
        for (col = 0; col < num_columns; col++) {
            if (col != flexible_col) {
                fixed_width += out_widths[col];
            }
        }
        
        /* Give remaining space to flexible column */
        int flex_width = available_width - fixed_width;
        
        /* Respect min/max constraints */
        if (columns[flexible_col].min_width > 0 && flex_width < columns[flexible_col].min_width) {
            flex_width = columns[flexible_col].min_width;
        }
        if (columns[flexible_col].max_width > 0 && flex_width > columns[flexible_col].max_width) {
            flex_width = columns[flexible_col].max_width;
        }
        
        out_widths[flexible_col] = flex_width;
    }
    
    /* End performance timing */
    if (TimerBase) {
        GetSysTime(&endTime);
        
        /* Count entries for per-entry timing */
        if (entries) {
            for (node = entries->lh_Head; node->ln_Succ; node = node->ln_Succ) {
                entry_count++;
            }
        }
        
        /* Calculate elapsed time in microseconds */
        elapsedMicros = ((endTime.tv_secs - startTime.tv_secs) * 1000000) +
                        (endTime.tv_micro - startTime.tv_micro);
        elapsedMillis = elapsedMicros / 1000;
        
        /* Log performance metrics only if enabled */
        if (is_performance_logging_enabled()) {
            log_info(LOG_GUI, "[PERF] Column width calculation: %lu.%03lu ms\n",
                     elapsedMillis, elapsedMicros % 1000);
            log_info(LOG_GUI, "       Entries: %d, Columns: %d\n", entry_count, num_columns);
            if (entry_count > 0) {
                log_info(LOG_GUI, "       Time per entry: %lu microseconds\n",
                         elapsedMicros / entry_count);
            }
            
            printf("  [TIMING] Column width calc: %lu.%03lu ms (%d entries x %d cols)\n",
                   elapsedMillis, elapsedMicros % 1000, entry_count, num_columns);
        }
    }
    
    return TRUE;
}

/*---------------------------------------------------------------------------*/
/* Helper: Format a single cell with alignment                               */
/*---------------------------------------------------------------------------*/
/**
 * @brief Format a cell with alignment and optional path abbreviation
 * 
 * Formats cell data to fit within the specified width. If the data is too long,
 * it will be truncated. For path columns (is_path=TRUE), uses intelligent Amiga-style
 * path abbreviation with "/../" notation before falling back to standard truncation.
 * 
 * Path Abbreviation Strategy:
 * - Preserves device name (e.g., "Work:")
 * - Preserves first directory for context
 * - Preserves filename (last component)
 * - Collapses middle directories with "/../"
 * - Example: "Work:Projects/Programming/Amiga/iTidy/src/tool.c" 
 *            becomes "Work:Projects/../tool.c"
 * 
 * @param output Buffer to receive formatted text (must be pre-allocated)
 * @param text Input text to format (can be NULL - treated as empty string)
 * @param width Target width in characters
 * @param align Alignment style (left, right, or center)
 * @param is_path TRUE to enable intelligent path abbreviation, FALSE for standard truncation
 * 
 * @note Output is always null-terminated and padded/truncated to exactly 'width' characters
 * @note Requires path_utilities module when is_path=TRUE
 * @note For non-path data or when path abbreviation doesn't help, falls back to "..." truncation
 */
static void format_cell(char *output, const char *text, int width, iTidy_ColumnAlign align, BOOL is_path)
{
    int len;
    int padding;
    int i;
    char truncated[256];
    char path_abbreviated[256];
    const char *display_text;
    
    if (!output) return;
    
    /* Handle NULL text */
    if (!text) {
        text = "";
    }
    
    len = strlen(text);
    
    /* Handle path formatting first if needed */
    if (is_path && len > width) {
        /* Try intelligent path abbreviation with /../ notation */
        if (iTidy_ShortenPathWithParentDir(text, path_abbreviated, width)) {
            /* Path was successfully abbreviated */
            display_text = path_abbreviated;
            len = strlen(path_abbreviated);
        } else {
            /* Path couldn't be abbreviated or already fits, use original */
            display_text = text;
        }
    } else {
        display_text = text;
    }
    
    /* Truncate if still too long */
    if (len > width) {
        if (width >= 3) {
            /* Truncate with "..." */
            strncpy(truncated, display_text, width - 3);
            truncated[width - 3] = '\0';
            strcat(truncated, "...");
        } else {
            /* Too narrow for "...", just truncate */
            strncpy(truncated, display_text, width);
            truncated[width] = '\0';
        }
        display_text = truncated;
        len = width;
    }
    
    /* Calculate padding */
    padding = width - len;
    
    /* Format based on alignment */
    switch (align) {
        case ITIDY_ALIGN_RIGHT:
            /* Right-aligned: "   text" */
            for (i = 0; i < padding; i++) {
                output[i] = ' ';
            }
            strcpy(output + padding, display_text);
            break;
            
        case ITIDY_ALIGN_CENTER:
            /* Center-aligned: " text  " */
            {
                int left_pad = padding / 2;
                int right_pad = padding - left_pad;
                for (i = 0; i < left_pad; i++) {
                    output[i] = ' ';
                }
                strcpy(output + left_pad, display_text);
                for (i = 0; i < right_pad; i++) {
                    output[left_pad + len + i] = ' ';
                }
                output[width] = '\0';
            }
            break;
            
        case ITIDY_ALIGN_LEFT:
        default:
            /* Left-aligned: "text   " */
            strcpy(output, display_text);
            for (i = len; i < width; i++) {
                output[i] = ' ';
            }
            output[width] = '\0';
            break;
    }
}

/*---------------------------------------------------------------------------*/
/* Helper: Create a display node                                             */
/*---------------------------------------------------------------------------*/

static struct Node *create_display_node(const char *text)
{
    struct Node *node;
    char *text_copy;
    
    if (!text) return NULL;
    
    node = (struct Node *)whd_malloc(sizeof(struct Node));
    if (!node) {
        log_error(LOG_GUI, "Failed to allocate display node\n");
        return NULL;
    }
    
    memset(node, 0, sizeof(struct Node));
    
    text_copy = (char *)whd_malloc(strlen(text) + 1);
    if (!text_copy) {
        log_error(LOG_GUI, "Failed to allocate node text\n");
        whd_free(node);
        return NULL;
    }
    
    strcpy(text_copy, text);
    node->ln_Name = text_copy;
    
    return node;
}

/*---------------------------------------------------------------------------*/
/* Helper: Format header row with sort indicators                            */
/*---------------------------------------------------------------------------*/

static void format_header_row(char *row_buffer, char *cell_buffer, iTidy_ColumnConfig *columns,
                              int num_columns, int *col_widths, iTidy_ListViewState *state)
{
    int col, pos;
    char title_with_indicator[128];
    
    pos = 0;
    for (col = 0; col < num_columns; col++) {
        /* Add sort indicator if this column is sorted */
        if (state && state->columns[col].sort_state != ITIDY_SORT_NONE) {
            const char *indicator = (state->columns[col].sort_state == ITIDY_SORT_ASCENDING) ? " ^" : " v";
            snprintf(title_with_indicator, sizeof(title_with_indicator), "%s%s", 
                    columns[col].title ? columns[col].title : "", indicator);
            format_cell(cell_buffer, title_with_indicator, col_widths[col], columns[col].align, FALSE);
        } else {
            format_cell(cell_buffer, columns[col].title, col_widths[col], columns[col].align, FALSE);
        }
        
        strcpy(row_buffer + pos, cell_buffer);
        pos += col_widths[col];
        
        if (col < num_columns - 1) {
            strcpy(row_buffer + pos, COLUMN_SEPARATOR);
            pos += SEPARATOR_WIDTH;
        }
    }
    row_buffer[pos] = '\0';
}

/*---------------------------------------------------------------------------*/
/* Main Formatting Function                                                  */
/*---------------------------------------------------------------------------*/

struct List *iTidy_FormatListViewColumns(
    iTidy_ColumnConfig *columns,
    int num_columns,
    struct List *entries,
    int total_char_width,
    iTidy_ListViewState **out_state)
{
    struct List *list;
    struct Node *node, *entry_node;
    iTidy_ListViewEntry *entry;
    iTidy_ListViewState *state = NULL;
    int *col_widths;
    char *row_buffer;
    char *cell_buffer;
    int col, row_count;
    int buffer_size;
    int pos, char_pos;
    int default_sort_col = -1;
    struct timeval startTime, endTime, cp1Time, cp2Time, cp3Time;
    ULONG elapsedMicros, elapsedMillis;
    ULONG sortMicros = 0, widthMicros = 0, formatMicros = 0;
    int entry_count = 0;
    
    /* Start performance timing */
    if (TimerBase) {
        GetSysTime(&startTime);
    }
    
    log_info(LOG_GUI, "iTidy_FormatListViewColumns: num_columns=%d, width=%d\n",
             num_columns, total_char_width);
    
    /* SAFETY: Validate input parameters */
    if (!columns || num_columns <= 0) {
        log_error(LOG_GUI, "iTidy_FormatListViewColumns: Invalid parameters (columns=%p, num_columns=%d)\n",
                 columns, num_columns);
        return NULL;
    }
    
    if (total_char_width <= 0 || total_char_width > 500) {
        log_error(LOG_GUI, "iTidy_FormatListViewColumns: Invalid total_char_width=%d (expected 10-500)\n",
                 total_char_width);
        return NULL;
    }
    
    /* Find default sort column */
    for (col = 0; col < num_columns; col++) {
        if (columns[col].default_sort != ITIDY_SORT_NONE) {
            default_sort_col = col;
            break;
        }
    }
    
    /* Sort entries if default sort is specified */
    if (default_sort_col >= 0 && entries) {
        BOOL ascending = (columns[default_sort_col].default_sort == ITIDY_SORT_ASCENDING);
        iTidy_SortListViewEntries(entries, default_sort_col, columns[default_sort_col].sort_type, ascending);
    }
    
    /* Checkpoint 1: After initial sort */
    if (TimerBase) {
        GetSysTime(&cp1Time);
        sortMicros = ((cp1Time.tv_secs - startTime.tv_secs) * 1000000) +
                     (cp1Time.tv_micro - startTime.tv_micro);
    }
    
    /* Allocate list for formatted output */
    list = (struct List *)whd_malloc(sizeof(struct List));
    if (!list) {
        log_error(LOG_GUI, "Failed to allocate list\n");
        return NULL;
    }
    NewList(list);
    
    /* Allocate column widths array */
    col_widths = (int *)whd_malloc(sizeof(int) * num_columns);
    if (!col_widths) {
        log_error(LOG_GUI, "Failed to allocate column widths\n");
        whd_free(list);
        return NULL;
    }
    
    /* Calculate column widths */
    if (!iTidy_CalculateColumnWidths(columns, num_columns, entries, 
                                     total_char_width, col_widths)) {
        log_error(LOG_GUI, "Failed to calculate column widths\n");
        whd_free(col_widths);
        whd_free(list);
        return NULL;
    }
    
    /* Checkpoint 2: After column width calculation */
    if (TimerBase) {
        GetSysTime(&cp2Time);
        widthMicros = ((cp2Time.tv_secs - cp1Time.tv_secs) * 1000000) +
                      (cp2Time.tv_micro - cp1Time.tv_micro);
    }
    
    /* Create state tracking if requested */
    if (out_state) {
        state = (iTidy_ListViewState *)whd_malloc(sizeof(iTidy_ListViewState));
        if (state) {
            state->num_columns = num_columns;
            state->separator_width = SEPARATOR_WIDTH;
            state->columns = (iTidy_ColumnState *)whd_malloc(sizeof(iTidy_ColumnState) * num_columns);
            
            if (state->columns) {
                /* Calculate column positions */
                char_pos = 0;
                for (col = 0; col < num_columns; col++) {
                    state->columns[col].column_index = col;
                    state->columns[col].char_start = char_pos;
                    state->columns[col].char_width = col_widths[col];
                    char_pos += col_widths[col];
                    state->columns[col].char_end = char_pos;
                    
                    /* Pixel positions will be calculated by caller using font width */
                    state->columns[col].pixel_start = 0;
                    state->columns[col].pixel_end = 0;
                    
                    /* Set initial sort state */
                    if (col == default_sort_col) {
                        state->columns[col].sort_state = columns[col].default_sort;
                    } else {
                        state->columns[col].sort_state = ITIDY_SORT_NONE;
                    }
                    
                    /* Store column type for smart helpers */
                    state->columns[col].column_type = columns[col].sort_type;
                    
                    char_pos += SEPARATOR_WIDTH;  /* Add separator */
                }
                *out_state = state;
            } else {
                whd_free(state);
                state = NULL;
            }
        }
    }
    
    /* Calculate buffer size needed */
    buffer_size = 0;
    for (col = 0; col < num_columns; col++) {
        buffer_size += col_widths[col];
    }
    buffer_size += (num_columns - 1) * SEPARATOR_WIDTH;
    buffer_size += 10;  /* Extra space for indicators and separator row */
    
    /* Allocate buffers */
    row_buffer = (char *)whd_malloc(buffer_size);
    cell_buffer = (char *)whd_malloc(256);  /* Temp buffer for cell formatting */
    
    if (!row_buffer || !cell_buffer) {
        log_error(LOG_GUI, "Failed to allocate formatting buffers\n");
        if (row_buffer) whd_free(row_buffer);
        if (cell_buffer) whd_free(cell_buffer);
        whd_free(col_widths);
        whd_free(list);
        if (state) {
            if (state->columns) whd_free(state->columns);
            whd_free(state);
        }
        return NULL;
    }
    
    /* ===== CREATE HEADER ROW ===== */
    format_header_row(row_buffer, cell_buffer, columns, num_columns, col_widths, state);
    
    node = create_display_node(row_buffer);
    if (node) {
        AddTail(list, node);
    }
    
    /* ===== CREATE SEPARATOR ROW ===== */
    pos = strlen(row_buffer);
    memset(row_buffer, '-', pos + 4);
    row_buffer[pos + 4] = '\0';
    
    node = create_display_node(row_buffer);
    if (node) {
        AddTail(list, node);
    }
    
    /* ===== CREATE DATA ROWS FROM ENTRIES ===== */
    row_count = 0;
    if (entries) {
        for (entry_node = entries->lh_Head; entry_node->ln_Succ; entry_node = entry_node->ln_Succ) {
            entry = (iTidy_ListViewEntry *)entry_node;
            pos = 0;
            
            for (col = 0; col < num_columns; col++) {
                const char *cell_data = (entry->display_data && col < entry->num_columns && entry->display_data[col]) ? 
                                        entry->display_data[col] : "";
                
                format_cell(cell_buffer, cell_data, col_widths[col], columns[col].align, columns[col].is_path);
                strcpy(row_buffer + pos, cell_buffer);
                pos += col_widths[col];
                
                if (col < num_columns - 1) {
                    strcpy(row_buffer + pos, COLUMN_SEPARATOR);
                    pos += SEPARATOR_WIDTH;
                }
            }
            row_buffer[pos] = '\0';
            
            /* Store formatted string in entry's ln_Name for future sorting */
            if (entry->node.ln_Name) {
                whd_free(entry->node.ln_Name);
            }
            entry->node.ln_Name = (char *)whd_malloc(strlen(row_buffer) + 1);
            if (entry->node.ln_Name) {
                strcpy(entry->node.ln_Name, row_buffer);
            }
            
            /* Also create a display node in the formatted list */
            node = create_display_node(row_buffer);
            if (node) {
                AddTail(list, node);
            }
            
            row_count++;
        }
    }
    
    /* Cleanup */
    whd_free(row_buffer);
    whd_free(cell_buffer);
    whd_free(col_widths);
    
    /* Checkpoint 3: After entry formatting */
    if (TimerBase) {
        GetSysTime(&cp3Time);
        formatMicros = ((cp3Time.tv_secs - cp2Time.tv_secs) * 1000000) +
                       (cp3Time.tv_micro - cp2Time.tv_micro);
    }
    
    /* Count entries */
    if (entries) {
        for (entry_node = entries->lh_Head; entry_node->ln_Succ; entry_node = entry_node->ln_Succ) {
            entry_count++;
        }
    }
    
    /* End performance timing */
    if (TimerBase) {
        GetSysTime(&endTime);
        
        /* Calculate total elapsed time */
        elapsedMicros = ((endTime.tv_secs - startTime.tv_secs) * 1000000) +
                        (endTime.tv_micro - startTime.tv_micro);
        elapsedMillis = elapsedMicros / 1000;
        
        /* Log performance metrics only if enabled */
        if (is_performance_logging_enabled()) {
            log_info(LOG_GUI, "==== LISTVIEW FORMAT PERFORMANCE ====\n");
            log_info(LOG_GUI, "iTidy_FormatListViewColumns() total time: %lu.%03lu ms\n",
                     elapsedMillis, elapsedMicros % 1000);
            log_info(LOG_GUI, "  Entries: %d, Columns: %d, Rows created: %d\n",
                     entry_count, num_columns, row_count);
            log_info(LOG_GUI, "  Breakdown:\n");
            log_info(LOG_GUI, "    Initial sort:    %lu.%03lu ms (%lu%%)\n",
                     sortMicros / 1000, sortMicros % 1000,
                     elapsedMicros > 0 ? (sortMicros * 100 / elapsedMicros) : 0);
            log_info(LOG_GUI, "    Width calc:      %lu.%03lu ms (%lu%%)\n",
                     widthMicros / 1000, widthMicros % 1000,
                     elapsedMicros > 0 ? (widthMicros * 100 / elapsedMicros) : 0);
            log_info(LOG_GUI, "    Entry format:    %lu.%03lu ms (%lu%%)\n",
                     formatMicros / 1000, formatMicros % 1000,
                     elapsedMicros > 0 ? (formatMicros * 100 / elapsedMicros) : 0);
            if (entry_count > 0) {
                log_info(LOG_GUI, "  Time per entry: %lu microseconds\n",
                         elapsedMicros / entry_count);
            }
            log_info(LOG_GUI, "====================================\n");
            
            printf("  [TIMING] ListView format: %lu.%03lu ms total (%d entries)\n",
                   elapsedMillis, elapsedMicros % 1000, entry_count);
            printf("           Sort: %lu.%03lu ms | Width: %lu.%03lu ms | Format: %lu.%03lu ms\n",
                   sortMicros / 1000, sortMicros % 1000,
                   widthMicros / 1000, widthMicros % 1000,
                   formatMicros / 1000, formatMicros % 1000);
        }
    }
    
    return list;
}

/*---------------------------------------------------------------------------*/
/* Map ListView Row to Entry                                                 */
/*---------------------------------------------------------------------------*/

/**
 * @brief Map a ListView row selection to the corresponding entry
 * 
 * Handles header offset and sorted list traversal automatically.
 * ListView structure: Row 0 = header, Row 1 = separator, Row 2+ = data
 */
iTidy_ListViewEntry *iTidy_GetSelectedEntry(struct List *entry_list, LONG listview_row)
{
    struct Node *node;
    LONG data_index;
    LONG current_index;
    
    if (!entry_list) {
        return NULL;
    }
    
    /* Check if row is in header area (0 = header, 1 = separator) */
    if (listview_row < 2) {
        return NULL;
    }
    
    /* Convert ListView row to data index (subtract header rows) */
    data_index = listview_row - 2;
    
    /* Walk the sorted entry list to find the Nth data entry */
    node = entry_list->lh_Head;
    current_index = 0;
    
    while (node->ln_Succ) {
        if (current_index == data_index) {
            /* Found the entry at the requested position */
            return (iTidy_ListViewEntry *)node;
        }
        node = node->ln_Succ;
        current_index++;
    }
    
    /* Row index was beyond the end of the list */
    return NULL;
}

/*---------------------------------------------------------------------------*/
/* Smart Click Detection Helpers                                             */
/*---------------------------------------------------------------------------*/

/**
 * @brief Detect which column was clicked based on mouse X position
 * 
 * Uses pixel-based hit-testing against column boundaries. Reuses the same
 * logic as header click detection for consistency.
 */
int iTidy_GetClickedColumn(iTidy_ListViewState *state, WORD mouse_x, WORD gadget_left)
{
    WORD local_x;
    int col;
    
    if (state == NULL) {
        return -1;
    }
    
    /* Adjust to gadget-relative coordinates */
    local_x = mouse_x - gadget_left;
    
    /* Search column boundaries using pixel positions */
    for (col = 0; col < state->num_columns; col++) {
        if (local_x >= state->columns[col].pixel_start && 
            local_x < state->columns[col].pixel_end) {
            return col;  /* Found the clicked column */
        }
    }
    
    return -1;  /* Click outside all columns */
}

/**
 * @brief Get complete click information for a ListView selection
 * 
 * This is the recommended high-level helper that combines entry lookup,
 * column detection, and value extraction in a single call.
 */
iTidy_ListViewClick iTidy_GetListViewClick(
    struct List *entry_list,
    iTidy_ListViewState *state,
    LONG listview_row,
    WORD mouse_x,
    WORD gadget_left)
{
    iTidy_ListViewClick result;
    
    /* Initialize with safe defaults */
    result.entry = NULL;
    result.column = -1;
    result.display_value = NULL;
    result.sort_key = NULL;
    result.column_type = ITIDY_COLTYPE_TEXT;  /* Safe default */
    
    /* Get the selected entry (handles header offset automatically) */
    result.entry = iTidy_GetSelectedEntry(entry_list, listview_row);
    
    /* Detect which column was clicked */
    result.column = iTidy_GetClickedColumn(state, mouse_x, gadget_left);
    
    /* Populate values if we have a valid entry and column */
    if (result.entry != NULL && 
        result.column >= 0 && 
        result.column < result.entry->num_columns) {
        
        /* Get display value (formatted for UI) */
        result.display_value = result.entry->display_data[result.column];
        
        /* Get sort key (machine-readable value) */
        result.sort_key = result.entry->sort_keys[result.column];
        
        /* Get column type from state (if available) */
        if (state != NULL && result.column < state->num_columns) {
            result.column_type = state->columns[result.column].column_type;
        }
    }
    
    return result;
}

/*---------------------------------------------------------------------------*/
/* High-Level IDCMP Event Handler                                           */
/*---------------------------------------------------------------------------*/

/**
 * @brief High-level handler for ListView GADGETUP events
 * 
 * Combines header click detection, sorting, and row click detection into
 * a single unified handler that returns a complete event structure.
 */
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
    iTidy_ListViewEvent *out_event)
{
    int header_top, header_height;
    LONG selected;
    
    /* Validate parameters */
    if (!window || !gadget || !entry_list || !display_list || !state || !columns || !out_event) {
        if (out_event) {
            out_event->type = ITIDY_LV_EVENT_NONE;
            out_event->did_sort = FALSE;
        }
        return FALSE;
    }
    
    /* Initialize event with safe defaults */
    out_event->type = ITIDY_LV_EVENT_NONE;
    out_event->did_sort = FALSE;
    out_event->sorted_column = -1;
    out_event->sort_order = ITIDY_SORT_NONE;
    out_event->entry = NULL;
    out_event->column = -1;
    out_event->display_value = NULL;
    out_event->sort_key = NULL;
    out_event->column_type = ITIDY_COLTYPE_TEXT;
    
    /* Calculate pixel positions from character positions (needed for column detection) */
    {
        int col;
        for (col = 0; col < num_columns; col++) {
            state->columns[col].pixel_start = state->columns[col].char_start * font_width;
            state->columns[col].pixel_end = state->columns[col].char_end * font_width;
        }
    }
    
    /* Get selected row from gadget FIRST - this accounts for scroll position */
    selected = -1;
    GT_GetGadgetAttrs(gadget, window, NULL, GTLV_Selected, &selected, TAG_END);
    
    if (selected < 0) {
        /* No selection */
        return FALSE;
    }
    
    /* Check if selected row is the header (row 0 only - NOT the separator at row 1) */
    /* This correctly handles scrolling - GTLV_Selected returns logical row index */
    if (selected == 0) {
        /* HEADER CLICK - Handle sorting */
        int clicked_col;
        BOOL sorted;
        
        /* Calculate header bounds for mouse position check */
        header_top = gadget->TopEdge;
        header_height = font_height;
        
        /* Detect which column was clicked using mouse X position */
        clicked_col = iTidy_GetClickedColumn(state, mouse_x, gadget->LeftEdge);
        
        if (clicked_col >= 0 && clicked_col < num_columns) {
            /* Valid column - perform sort */
            sorted = iTidy_ResortListViewByClick(
                display_list,
                entry_list,
                state,
                mouse_x,
                mouse_y,
                header_top,
                header_height,
                gadget->LeftEdge,
                font_width,
                columns
            );
            
            if (sorted) {
                /* Fill in header event details */
                out_event->type = ITIDY_LV_EVENT_HEADER_SORTED;
                out_event->did_sort = TRUE;
                out_event->sorted_column = clicked_col;
                out_event->sort_order = state->columns[clicked_col].sort_state;
                return TRUE;
            }
        }
        
        /* Header clicked but no valid sort */
        return FALSE;
    }
    
    /* Check if selected row is the separator (row 1) - ignore these clicks */
    if (selected == 1) {
        /* Separator row clicked - do nothing */
        return FALSE;
    }
    /* Check if selected row is the separator (row 1) - ignore these clicks */
    if (selected == 1) {
        /* Separator row clicked - do nothing */
        return FALSE;
    }
    
    /* NOT HEADER/SEPARATOR - Must be data row click (selected >= 2) */
    /* Use smart helper to get complete click information */
    iTidy_ListViewClick click = iTidy_GetListViewClick(
        entry_list,
        state,
        selected,
        mouse_x,
        gadget->LeftEdge
    );
    
    /* Check if we got a valid entry (not header row) */
    if (click.entry != NULL) {
        /* Fill in row click event details */
        out_event->type = ITIDY_LV_EVENT_ROW_CLICK;
        out_event->entry = click.entry;
        out_event->column = click.column;
        out_event->display_value = click.display_value;
        out_event->sort_key = click.sort_key;
        out_event->column_type = click.column_type;
        return TRUE;
    }
    
    /* Click was on header/separator row or invalid */
    return FALSE;
}

/*---------------------------------------------------------------------------*/
/* Free ListView Entries (Complete Cleanup)                                  */
/*---------------------------------------------------------------------------*/

/**
 * @brief Free all resources associated with a ListView entry list
 * 
 * This function performs complete cleanup of all ListView resources in the
 * correct order. It handles NULL parameters gracefully.
 * 
 * Cleanup order (CRITICAL):
 * 1. Entry list entries:
 *    - ln_Name (formatted display string allocated by formatter)
 *    - display_data[col] strings for each column
 *    - sort_keys[col] strings for each column
 *    - display_data array
 *    - sort_keys array
 *    - Entry structure itself
 * 2. Entry list structure
 * 3. Display list structure (formatted ListView nodes)
 * 4. ListView state structure
 * 
 * @param entry_list    List of iTidy_ListViewEntry nodes (can be NULL)
 * @param display_list  Display list for ListView gadget (can be NULL)
 * @param state         ListView state with column info (can be NULL)
 * 
 * @note MUST detach lists from gadgets BEFORE calling this function:
 *       GT_SetGadgetAttrs(gadget, window, NULL, GTLV_Labels, ~0, TAG_DONE);
 * @note This function does NOT close windows or free gadgets - caller's responsibility
 * @note Handles NULL parameters gracefully - safe to call with any combination
 * 
 * Example:
 * @code
 * // Detach from gadget first (REQUIRED!)
 * GT_SetGadgetAttrs(data->session_listview, data->window, NULL,
 *                   GTLV_Labels, ~0, TAG_DONE);
 * 
 * // Free everything in one call
 * itidy_free_listview_entries(&data->session_entry_list,
 *                             data->session_display_list,
 *                             data->session_lv_state);
 * 
 * // NULL out pointers
 * data->session_display_list = NULL;
 * data->session_lv_state = NULL;
 * @endcode
 */
void itidy_free_listview_entries(struct List *entry_list,
                                 struct List *display_list,
                                 iTidy_ListViewState *state)
{
    struct Node *node;
    iTidy_ListViewEntry *entry;
    int num_columns;
    int i;
    
    /* Determine number of columns from state (needed for cleanup) */
    num_columns = (state && state->num_columns > 0) ? state->num_columns : 0;
    
    /* Free entry list (display_data and sort_keys) */
    if (entry_list) {
        while ((node = RemHead(entry_list)) != NULL) {
            entry = (iTidy_ListViewEntry *)node;
            
            /* Free ln_Name (formatted display string allocated by formatter) */
            if (entry->node.ln_Name) {
                whd_free(entry->node.ln_Name);
                entry->node.ln_Name = NULL;
            }
            
            /* Use entry's num_columns if state not available */
            if (num_columns == 0 && entry->num_columns > 0) {
                num_columns = entry->num_columns;
            }
            
            /* Free display_data and sort_keys arrays */
            for (i = 0; i < entry->num_columns; i++) {
                if (entry->display_data && entry->display_data[i]) {
                    whd_free((void *)entry->display_data[i]);
                }
                if (entry->sort_keys && entry->sort_keys[i]) {
                    whd_free((void *)entry->sort_keys[i]);
                }
            }
            
            /* Free array pointers */
            if (entry->display_data) {
                whd_free((void *)entry->display_data);
            }
            if (entry->sort_keys) {
                whd_free((void *)entry->sort_keys);
            }
            
            /* Free entry structure itself */
            whd_free(entry);
        }
    }
    
    /* Free display list (formatted ListView nodes) */
    if (display_list) {
        iTidy_FreeFormattedList(display_list);
    }
    
    /* Free ListView state */
    if (state) {
        iTidy_FreeListViewState(state);
    }
}

/*---------------------------------------------------------------------------*/
/* Free Formatted List                                                       */
/*---------------------------------------------------------------------------*/

void iTidy_FreeFormattedList(struct List *list)
{
    struct Node *node, *next;
    
    if (!list) return;
    
    node = list->lh_Head;
    while (node->ln_Succ) {
        next = node->ln_Succ;
        
        if (node->ln_Name) {
            whd_free(node->ln_Name);
        }
        
        Remove(node);
        whd_free(node);
        
        node = next;
    }
    
    whd_free(list);
}

/*---------------------------------------------------------------------------*/
/* Free ListView State                                                       */
/*---------------------------------------------------------------------------*/

void iTidy_FreeListViewState(iTidy_ListViewState *state)
{
    if (!state) return;
    
    if (state->columns) {
        whd_free(state->columns);
    }
    
    whd_free(state);
}

/*---------------------------------------------------------------------------*/
/* Backward Compatibility - Legacy API (No Sorting)                         */
/*---------------------------------------------------------------------------*/

struct List *iTidy_FormatListViewColumns_Legacy(
    iTidy_ColumnConfig *columns,
    int num_columns,
    const char ***data_rows,
    int num_rows,
    int total_char_width)
{
    struct List *list;
    struct Node *node;
    int *col_widths;
    char *row_buffer;
    char *cell_buffer;
    int row, col;
    int buffer_size;
    int pos;
    
    log_info(LOG_GUI, "iTidy_FormatListViewColumns_Legacy: num_columns=%d, num_rows=%d, width=%d\n",
             num_columns, num_rows, total_char_width);
    
    if (!columns || num_columns <= 0) {
        log_error(LOG_GUI, "Invalid column configuration\n");
        return NULL;
    }
    
    /* Allocate list */
    list = (struct List *)whd_malloc(sizeof(struct List));
    if (!list) {
        log_error(LOG_GUI, "Failed to allocate list\n");
        return NULL;
    }
    NewList(list);
    
    /* Allocate column widths array */
    col_widths = (int *)whd_malloc(sizeof(int) * num_columns);
    if (!col_widths) {
        log_error(LOG_GUI, "Failed to allocate column widths\n");
        whd_free(list);
        return NULL;
    }
    
    /* Calculate column widths - pass NULL for entries to use old calculation path */
    /* We need a different calculation function for the old API */
    int flexible_col = -1;
    int separator_total = (num_columns - 1) * SEPARATOR_WIDTH;
    
    /* Measure each column */
    for (col = 0; col < num_columns; col++) {
        int max_len = columns[col].title ? strlen(columns[col].title) : 0;
        
        if (data_rows && num_rows > 0) {
            for (row = 0; row < num_rows; row++) {
                if (data_rows[row] && data_rows[row][col]) {
                    int len = strlen(data_rows[row][col]);
                    if (len > max_len) {
                        max_len = len;
                    }
                }
            }
        }
        
        if (columns[col].min_width > 0 && max_len < columns[col].min_width) {
            max_len = columns[col].min_width;
        }
        if (columns[col].max_width > 0 && max_len > columns[col].max_width) {
            max_len = columns[col].max_width;
        }
        
        col_widths[col] = max_len;
        
        if (columns[col].flexible && flexible_col < 0) {
            flexible_col = col;
        }
    }
    
    /* Adjust flexible column if needed */
    if (total_char_width > 0 && flexible_col >= 0) {
        int fixed_width = 0;
        for (col = 0; col < num_columns; col++) {
            if (col != flexible_col) {
                fixed_width += col_widths[col];
            }
        }
        
        int flex_width = total_char_width - separator_total - fixed_width;
        if (columns[flexible_col].min_width > 0 && flex_width < columns[flexible_col].min_width) {
            flex_width = columns[flexible_col].min_width;
        }
        if (columns[flexible_col].max_width > 0 && flex_width > columns[flexible_col].max_width) {
            flex_width = columns[flexible_col].max_width;
        }
        col_widths[flexible_col] = flex_width;
    }
    
    /* Calculate buffer size needed */
    buffer_size = 0;
    for (col = 0; col < num_columns; col++) {
        buffer_size += col_widths[col];
    }
    buffer_size += separator_total;
    buffer_size += 10;
    
    /* Allocate buffers */
    row_buffer = (char *)whd_malloc(buffer_size);
    cell_buffer = (char *)whd_malloc(256);
    
    if (!row_buffer || !cell_buffer) {
        log_error(LOG_GUI, "Failed to allocate formatting buffers\n");
        if (row_buffer) whd_free(row_buffer);
        if (cell_buffer) whd_free(cell_buffer);
        whd_free(col_widths);
        whd_free(list);
        return NULL;
    }
    
    /* CREATE HEADER ROW */
    pos = 0;
    for (col = 0; col < num_columns; col++) {
        format_cell(cell_buffer, columns[col].title, col_widths[col], ITIDY_ALIGN_LEFT, FALSE);
        strcpy(row_buffer + pos, cell_buffer);
        pos += col_widths[col];
        
        if (col < num_columns - 1) {
            strcpy(row_buffer + pos, COLUMN_SEPARATOR);
            pos += SEPARATOR_WIDTH;
        }
    }
    row_buffer[pos] = '\0';
    
    node = create_display_node(row_buffer);
    if (node) {
        AddTail(list, node);
    }
    
    /* CREATE SEPARATOR ROW */
    memset(row_buffer, '-', pos + 4);
    row_buffer[pos + 4] = '\0';
    
    node = create_display_node(row_buffer);
    if (node) {
        AddTail(list, node);
    }
    
    /* CREATE DATA ROWS */
    if (data_rows && num_rows > 0) {
        for (row = 0; row < num_rows; row++) {
            pos = 0;
            
            for (col = 0; col < num_columns; col++) {
                const char *cell_data = (data_rows[row] && data_rows[row][col]) ? 
                                        data_rows[row][col] : "";
                
                format_cell(cell_buffer, cell_data, col_widths[col], columns[col].align, columns[col].is_path);
                strcpy(row_buffer + pos, cell_buffer);
                pos += col_widths[col];
                
                if (col < num_columns - 1) {
                    strcpy(row_buffer + pos, COLUMN_SEPARATOR);
                    pos += SEPARATOR_WIDTH;
                }
            }
            row_buffer[pos] = '\0';
            
            node = create_display_node(row_buffer);
            if (node) {
                AddTail(list, node);
            }
        }
    }
    
    /* Cleanup */
    whd_free(row_buffer);
    whd_free(cell_buffer);
    whd_free(col_widths);
    
    log_info(LOG_GUI, "Formatted %d columns x %d rows (legacy API)\n", num_columns, num_rows);
    
    return list;
}

/*---------------------------------------------------------------------------*/
/* Re-sort ListView by Column Click                                          */
/*---------------------------------------------------------------------------*/

BOOL iTidy_ResortListViewByClick(
    struct List *formatted_list,
    struct List *entry_list,
    iTidy_ListViewState *state,
    int mouse_x,
    int mouse_y,
    int header_top,
    int header_height,
    int gadget_left,
    int font_width,
    iTidy_ColumnConfig *columns)
{
    int col, local_x, pos;
    int clicked_col = -1;
    struct Node *header_node, *separator_node, *data_node, *entry_node;
    iTidy_ListViewEntry *entry;
    char *row_buffer, *cell_buffer;
    int *col_widths;
    int buffer_size;
    BOOL ascending;
    struct timeval startTime, endTime, sortEndTime;
    ULONG elapsedMicros, elapsedMillis, sortMicros = 0, rebuildMicros = 0;
    int entry_count = 0;
    
    log_debug(LOG_GUI, "iTidy_ResortListViewByClick: mouse_x=%d, mouse_y=%d, header_top=%d, header_height=%d, gadget_left=%d, font_width=%d\n",
              mouse_x, mouse_y, header_top, header_height, gadget_left, font_width);
    
    if (!formatted_list || !entry_list || !state || !columns) {
        log_error(LOG_GUI, "iTidy_ResortListViewByClick: NULL parameter! formatted_list=%p, entry_list=%p, state=%p, columns=%p\n",
                  formatted_list, entry_list, state, columns);
        return FALSE;
    }
    
    /* Start performance timing */
    if (TimerBase) {
        GetSysTime(&startTime);
    }
    
    log_debug(LOG_GUI, "iTidy_ResortListViewByClick: num_columns=%d\n", state->num_columns);
    
    /* Check if click is within header Y-range */
    if (mouse_y < header_top || mouse_y >= header_top + header_height) {
        log_debug(LOG_GUI, "iTidy_ResortListViewByClick: Click outside header Y-range (y=%d, top=%d, bottom=%d)\n",
                  mouse_y, header_top, header_top + header_height);
        return FALSE;
    }
    
    /* Adjust X coordinate to gadget-relative */
    local_x = mouse_x - gadget_left;
    log_debug(LOG_GUI, "iTidy_ResortListViewByClick: local_x=%d (mouse_x=%d - gadget_left=%d)\n",
              local_x, mouse_x, gadget_left);
    
    /* Calculate pixel positions if not already done */
    for (col = 0; col < state->num_columns; col++) {
        state->columns[col].pixel_start = state->columns[col].char_start * font_width;
        state->columns[col].pixel_end = state->columns[col].char_end * font_width;
        log_debug(LOG_GUI, "Column %d: char_start=%d, char_end=%d, pixel_start=%d, pixel_end=%d\n",
                  col, state->columns[col].char_start, state->columns[col].char_end,
                  state->columns[col].pixel_start, state->columns[col].pixel_end);
    }
    
    /* Detect which column was clicked */
    for (col = 0; col < state->num_columns; col++) {
        if (local_x >= state->columns[col].pixel_start && local_x < state->columns[col].pixel_end) {
            clicked_col = col;
            log_debug(LOG_GUI, "iTidy_ResortListViewByClick: Matched column %d (local_x=%d in range %d-%d)\n",
                      col, local_x, state->columns[col].pixel_start, state->columns[col].pixel_end);
            break;
        }
    }
    
    if (clicked_col < 0) {
        log_debug(LOG_GUI, "iTidy_ResortListViewByClick: Click outside all columns (local_x=%d)\n", local_x);
        return FALSE;  /* Click was outside all columns */
    }
    
    log_info(LOG_GUI, "Column %d clicked (x=%d, local_x=%d)\n", clicked_col, mouse_x, local_x);
    
    /* Toggle sort order */
    if (state->columns[clicked_col].sort_state == ITIDY_SORT_ASCENDING) {
        state->columns[clicked_col].sort_state = ITIDY_SORT_DESCENDING;
        ascending = FALSE;
    } else {
        state->columns[clicked_col].sort_state = ITIDY_SORT_ASCENDING;
        ascending = TRUE;
    }
    
    /* Clear other columns' sort state */
    for (col = 0; col < state->num_columns; col++) {
        if (col != clicked_col) {
            state->columns[col].sort_state = ITIDY_SORT_NONE;
        }
    }
    
    /* Sort the entry list in-place */
    iTidy_SortListViewEntries(entry_list, clicked_col, columns[clicked_col].sort_type, ascending);
    
    /* Checkpoint: After sort */
    if (TimerBase) {
        GetSysTime(&sortEndTime);
        sortMicros = ((sortEndTime.tv_secs - startTime.tv_secs) * 1000000) +
                     (sortEndTime.tv_micro - startTime.tv_micro);
    }
    
    /* Get column widths from state */
    col_widths = (int *)whd_malloc(sizeof(int) * state->num_columns);
    if (!col_widths) {
        return FALSE;
    }
    
    for (col = 0; col < state->num_columns; col++) {
        col_widths[col] = state->columns[col].char_width;
    }
    
    /* Calculate buffer size */
    buffer_size = 0;
    for (col = 0; col < state->num_columns; col++) {
        buffer_size += col_widths[col];
    }
    buffer_size += (state->num_columns - 1) * SEPARATOR_WIDTH;
    buffer_size += 10;
    
    row_buffer = (char *)whd_malloc(buffer_size);
    cell_buffer = (char *)whd_malloc(256);
    
    if (!row_buffer || !cell_buffer) {
        if (row_buffer) whd_free(row_buffer);
        if (cell_buffer) whd_free(cell_buffer);
        whd_free(col_widths);
        return FALSE;
    }
    
    /* Update header row with new sort indicators */
    header_node = formatted_list->lh_Head;
    if (header_node && header_node->ln_Name) {
        whd_free(header_node->ln_Name);
        format_header_row(row_buffer, cell_buffer, columns, state->num_columns, col_widths, state);
        header_node->ln_Name = (char *)whd_malloc(strlen(row_buffer) + 1);
        if (header_node->ln_Name) {
            strcpy(header_node->ln_Name, row_buffer);
        }
    }
    
    /* Skip separator node */
    separator_node = (header_node && header_node->ln_Succ) ? header_node->ln_Succ : NULL;
    
    /* Rebuild data rows in formatted list */
    /* First, remove all old data nodes */
    if (separator_node && separator_node->ln_Succ) {
        data_node = separator_node->ln_Succ;
        while (data_node->ln_Succ) {
            struct Node *next = data_node->ln_Succ;
            if (data_node->ln_Name) {
                whd_free(data_node->ln_Name);
            }
            Remove(data_node);
            whd_free(data_node);
            data_node = next;
        }
    }
    
    /* Now rebuild from sorted entry list */
    for (entry_node = entry_list->lh_Head; entry_node->ln_Succ; entry_node = entry_node->ln_Succ) {
        entry = (iTidy_ListViewEntry *)entry_node;
        pos = 0;
        
        for (col = 0; col < state->num_columns; col++) {
            const char *cell_data = (entry->display_data && col < entry->num_columns && entry->display_data[col]) ? 
                                    entry->display_data[col] : "";
            
            format_cell(cell_buffer, cell_data, col_widths[col], columns[col].align, columns[col].is_path);
            strcpy(row_buffer + pos, cell_buffer);
            pos += col_widths[col];
            
            if (col < state->num_columns - 1) {
                strcpy(row_buffer + pos, COLUMN_SEPARATOR);
                pos += SEPARATOR_WIDTH;
            }
        }
        row_buffer[pos] = '\0';
        
        /* Create new display node */
        data_node = create_display_node(row_buffer);
        if (data_node) {
            AddTail(formatted_list, data_node);
        }
    }
    
    /* Cleanup */
    whd_free(row_buffer);
    whd_free(cell_buffer);
    whd_free(col_widths);
    
    /* Count entries */
    for (entry_node = entry_list->lh_Head; entry_node->ln_Succ; entry_node = entry_node->ln_Succ) {
        entry_count++;
    }
    
    /* End performance timing */
    if (TimerBase) {
        GetSysTime(&endTime);
        
        /* Calculate total elapsed time */
        elapsedMicros = ((endTime.tv_secs - startTime.tv_secs) * 1000000) +
                        (endTime.tv_micro - startTime.tv_micro);
        elapsedMillis = elapsedMicros / 1000;
        rebuildMicros = elapsedMicros - sortMicros;
        
        /* Log performance metrics only if enabled */
        if (is_performance_logging_enabled()) {
            const char *type_str = (columns[clicked_col].sort_type == ITIDY_COLTYPE_NUMBER) ? "NUMBER" :
                                   (columns[clicked_col].sort_type == ITIDY_COLTYPE_DATE) ? "DATE" : "TEXT";
            const char *dir_str = ascending ? "ASC" : "DESC";
            
            log_info(LOG_GUI, "==== LISTVIEW RESORT PERFORMANCE ====\n");
            log_info(LOG_GUI, "iTidy_ResortListViewByClick() total time: %lu.%03lu ms\n",
                     elapsedMillis, elapsedMicros % 1000);
            log_info(LOG_GUI, "  Column: %d (%s, %s)\n", clicked_col, type_str, dir_str);
            log_info(LOG_GUI, "  Entries: %d, Columns: %d\n", entry_count, state->num_columns);
            log_info(LOG_GUI, "  Breakdown:\n");
            log_info(LOG_GUI, "    Sort:      %lu.%03lu ms (%lu%%)\n",
                     sortMicros / 1000, sortMicros % 1000,
                     elapsedMicros > 0 ? (sortMicros * 100 / elapsedMicros) : 0);
            log_info(LOG_GUI, "    Rebuild:   %lu.%03lu ms (%lu%%)\n",
                     rebuildMicros / 1000, rebuildMicros % 1000,
                     elapsedMicros > 0 ? (rebuildMicros * 100 / elapsedMicros) : 0);
            if (entry_count > 0) {
                log_info(LOG_GUI, "  Time per entry: %lu microseconds\n",
                         elapsedMicros / entry_count);
            }
            log_info(LOG_GUI, "====================================\n");
            
            printf("  [TIMING] ListView resort: %lu.%03lu ms total (col %d, %s/%s, %d entries)\n",
                   elapsedMillis, elapsedMicros % 1000, clicked_col, type_str, dir_str, entry_count);
            printf("           Sort: %lu.%03lu ms | Rebuild: %lu.%03lu ms\n",
                   sortMicros / 1000, sortMicros % 1000,
                   rebuildMicros / 1000, rebuildMicros % 1000);
        }
    }
    
    log_info(LOG_GUI, "Resorted by column %d (%s)\n", clicked_col, ascending ? "ASC" : "DESC");
    
    return TRUE;
}

/*---------------------------------------------------------------------------*/
/* High-Level Sort Handler (Convenience Wrapper)                            */
/*---------------------------------------------------------------------------*/

BOOL iTidy_HandleListViewSort(
    struct Window *window,
    struct Gadget *listview_gadget,
    struct List *formatted_list,
    struct List *entry_list,
    iTidy_ListViewState *state,
    int mouse_x,
    int mouse_y,
    int font_height,
    int font_width,
    iTidy_ColumnConfig *columns,
    int num_columns)
{
    BOOL did_sort;
    int header_top, header_height, gadget_left;
    
    /* Validate parameters */
    if (!window || !listview_gadget || !formatted_list || !entry_list || !state || !columns) {
        log_debug(LOG_GUI, "iTidy_HandleListViewSort: NULL parameter\n");
        return FALSE;
    }
    
    /* Calculate ListView header position */
    header_top = listview_gadget->TopEdge;
    header_height = font_height;
    gadget_left = listview_gadget->LeftEdge;
    
    /* Quick check: is click even in header Y-range? */
    if (mouse_y < header_top || mouse_y >= header_top + header_height) {
        return FALSE;  /* Not in header - don't waste time */
    }
    
    /* Set busy pointer during sort operation */
    SetWindowPointer(window, WA_BusyPointer, TRUE, TAG_DONE);
    
    /* Attempt to resort based on click position */
    did_sort = iTidy_ResortListViewByClick(
        formatted_list,
        entry_list,
        state,
        mouse_x, mouse_y,
        header_top, header_height,
        gadget_left,
        font_width,
        columns
    );
    
    /* Update display if sorting occurred */
    if (did_sort) {
        log_debug(LOG_GUI, "iTidy_HandleListViewSort: List resorted, refreshing gadget\n");
        
        /* Refresh gadget with re-sorted list */
        GT_SetGadgetAttrs(listview_gadget, window, NULL,
                          GTLV_Labels, ~0,  /* Detach */
                          TAG_DONE);
        GT_SetGadgetAttrs(listview_gadget, window, NULL,
                          GTLV_Labels, formatted_list,
                          TAG_DONE);
    }
    
    /* Clear busy pointer */
    SetWindowPointer(window, WA_BusyPointer, FALSE, TAG_DONE);
    
    return did_sort;
}

