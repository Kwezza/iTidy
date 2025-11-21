/**
 * listview_columns_api.h - ListView Column Layout & Sorting API
 * 
 * Provides utilities for creating properly aligned, multi-column ListView displays
 * with automatic width calculation, professional formatting, and sortable columns.
 * 
 * Features:
 * - Auto-calculates optimal column widths from data
 * - Generates header and separator rows automatically
 * - Supports left/right/center alignment per column
 * - Flexible columns that expand to fill available space
 * - Smart truncation with "..." for overflow
 * - Column-based sorting with visual indicators
 * - Returns ready-to-use struct List for GadTools GTLV_Labels
 */

#ifndef LISTVIEW_COLUMNS_API_H
#define LISTVIEW_COLUMNS_API_H

#include <exec/types.h>
#include <exec/lists.h>

/*---------------------------------------------------------------------------*/
/* ListView Display Mode                                                     */
/*---------------------------------------------------------------------------*/

typedef enum {
    ITIDY_MODE_FULL,              /* Full mode: sorting + state + optional pagination */
    ITIDY_MODE_FULL_NO_SORT,      /* Full mode but disable sorting (was page_size=-1) */
    ITIDY_MODE_SIMPLE,            /* Simple mode: display-only, no sorting/state (fast) */
    ITIDY_MODE_SIMPLE_PAGINATED   /* Simple mode + pagination (best for large lists) */
} iTidy_ListViewMode;

/*---------------------------------------------------------------------------*/
/* Column Alignment                                                          */
/*---------------------------------------------------------------------------*/

typedef enum {
    ITIDY_ALIGN_LEFT,     /* Left-aligned (default for text) */
    ITIDY_ALIGN_RIGHT,    /* Right-aligned (good for numbers) */
    ITIDY_ALIGN_CENTER    /* Center-aligned (rarely used) */
} iTidy_ColumnAlign;

/*---------------------------------------------------------------------------*/
/* Sorting Support                                                           */
/*---------------------------------------------------------------------------*/

typedef enum {
    ITIDY_SORT_NONE,       /* Column not sorted */
    ITIDY_SORT_ASCENDING,  /* Sort ascending (A-Z, 0-9, oldest-newest) */
    ITIDY_SORT_DESCENDING  /* Sort descending (Z-A, 9-0, newest-oldest) */
} iTidy_SortOrder;

typedef enum {
    ITIDY_COLTYPE_TEXT,    /* String comparison */
    ITIDY_COLTYPE_NUMBER,  /* Numeric comparison */
    ITIDY_COLTYPE_DATE     /* Date comparison using sort keys (YYYYMMDD_HHMMSS format) */
} iTidy_ColumnType;

/*---------------------------------------------------------------------------*/
/* Column Configuration                                                      */
/*---------------------------------------------------------------------------*/

typedef struct {
    const char *title;         /* Column header text */
    int min_width;             /* Minimum width in characters (0 = auto from title) */
    int max_width;             /* Maximum width in characters (0 = unlimited) */
    iTidy_ColumnAlign align;   /* Text alignment */
    BOOL flexible;             /* TRUE if column should absorb extra space */
    BOOL is_path;              /* TRUE if column contains Amiga paths - uses /../ notation
                                * When TRUE: Long paths are intelligently abbreviated using
                                * Amiga-style "/../" notation (e.g., "Work:Projects/../tool.c")
                                * instead of simple truncation with "...". This preserves
                                * device name, first directory, and filename for better
                                * readability. Requires path_utilities module.
                                * When FALSE: Standard text truncation is used.
                                */
    iTidy_SortOrder default_sort;  /* Initial sort order (ITIDY_SORT_NONE for unsorted) */
    iTidy_ColumnType sort_type;    /* How to compare values when sorting */
} iTidy_ColumnConfig;

/*---------------------------------------------------------------------------*/
/* ListView Row Type                                                         */
/*---------------------------------------------------------------------------*/

/**
 * @brief Type of row in ListView (data vs navigation)
 * 
 * Used to distinguish between actual data rows and pagination navigation rows.
 * Navigation rows don't get formatted into columns - they span the full width.
 */
typedef enum {
    ITIDY_ROW_DATA = 0,           /* Normal data row (default) */
    ITIDY_ROW_NAV_PREV = 1,       /* "Previous Page" navigation row */
    ITIDY_ROW_NAV_NEXT = 2        /* "Next Page" navigation row */
} iTidy_RowType;

/*---------------------------------------------------------------------------*/
/* ListView Entry Structure                                                  */
/*---------------------------------------------------------------------------*/

/**
 * @brief ListView entry with dual-data design for correct sorting
 * 
 * Each entry contains both display data (human-readable) and sort keys
 * (machine-sortable). This allows dates, paths, and numbers to display
 * nicely while sorting correctly.
 * 
 * Example:
 * - Display: "24-Nov-2025 15:19" (readable)
 * - Sort key: "20251124_151900" (sorts chronologically as string)
 * 
 * Memory ownership:
 * - Caller allocates and owns this structure
 * - Caller allocates and owns display_data and sort_keys arrays
 * - Caller allocates and owns individual strings
 * - Formatter reads these to create ln_Name formatted strings
 * - Formatter owns ln_Name strings (freed by iTidy_FreeFormattedList)
 */
typedef struct {
    struct Node node;              /* Embedded Node (ln_Name = formatted display) */
    const char **display_data;     /* Pretty formatted: "24-Nov-2025 15:19" */
    const char **sort_keys;        /* Machine-sortable: "20251124_151900" */
    int num_columns;               /* Number of columns */
    iTidy_RowType row_type;        /* Row type (DATA, NAV_PREV, NAV_NEXT) */
} iTidy_ListViewEntry;

/*---------------------------------------------------------------------------*/
/* Column State Tracking                                                     */
/*---------------------------------------------------------------------------*/

/**
 * @brief Runtime state for a single column
 * 
 * Tracks position and sort state for click detection and visual indicators.
 * All positions are in window inner coordinates.
 */
typedef struct {
    int column_index;           /* Which column (0-based) */
    int char_start;             /* Character position where column starts */
    int char_end;               /* Character position where column ends */
    int char_width;             /* Width in characters */
    int pixel_start;            /* Pixel position (char_start × font_width) */
    int pixel_end;              /* Pixel position (char_end × font_width) */
    iTidy_SortOrder sort_state; /* Current sort state */
    iTidy_ColumnType column_type; /* Column type (for smart helpers) */
} iTidy_ColumnState;

/**
 * @brief Overall ListView state for sorting and pagination
 * 
 * Returned by iTidy_FormatListViewColumns and used by iTidy_HandleListViewGadgetUp.
 * Tracks all columns and their current sort states, plus pagination state.
 * 
 * IMPORTANT: The API owns pagination state and handles navigation automatically.
 * Callers don't need to track page numbers or navigation direction.
 */
typedef struct {
    iTidy_ColumnState *columns; /* Array of column states */
    int num_columns;            /* Number of columns */
    int separator_width;        /* Width of " | " separator (3 chars) */
    
    /* Pagination state (managed by API) */
    int current_page;           /* Current page (1-based, 0 = no pagination) */
    int total_pages;            /* Total number of pages */
    int last_nav_direction;     /* -1 = Previous, +1 = Next, 0 = None/Sort */
    int auto_select_row;        /* Row to auto-select after format (-1 = none) */
    BOOL sorting_disabled;      /* TRUE if sorting explicitly disabled (page_size=-1) */
} iTidy_ListViewState;

/*---------------------------------------------------------------------------*/
/* Main Formatting Function                                                  */
/*---------------------------------------------------------------------------*/

/**
 * @brief Format data into a columnar ListView with optional sorting
 * 
 * Takes column definitions and entry list, calculates optimal column widths,
 * optionally sorts by default_sort column, and returns a formatted struct List
 * ready for use with GadTools ListView.
 * 
 * The returned list includes:
 * - Header row with column titles (with sort indicators if sorted)
 * - Separator row (dashes)
 * - Data rows with proper alignment and padding
 * 
 * @param columns Array of column configurations
 * @param num_columns Number of columns
 * @param entries Pointer to List of iTidy_ListViewEntry nodes (can be NULL for empty list)
 * @param total_char_width Total character width available (0 = calculate from data)
 * @param out_state Output pointer to receive state tracking structure (can be NULL if not needed)
 * @return Allocated struct List ready for GTLV_Labels, or NULL on error
 * 
 * @note Caller must free the returned list with iTidy_FreeFormattedList()
 * @note Caller must free the state with iTidy_FreeListViewState() when done
 * @note Each column is separated by " | " in the output
 * @note Exactly one column should have flexible=TRUE to absorb remaining space
 * @note Entry list nodes are sorted in-place if default_sort is specified
 * @note Caller owns entry structures and their strings - keep them alive until cleanup!
 * 
 * Example:
 * @code
 * iTidy_ColumnConfig cols[] = {
 *     {"Date/Time", 20, 20, ITIDY_ALIGN_LEFT, FALSE, FALSE, 
 *      ITIDY_SORT_DESCENDING, ITIDY_COLTYPE_DATE},  // Sort newest first
 *     {"Changes", 4, 6, ITIDY_ALIGN_RIGHT, FALSE, FALSE,
 *      ITIDY_SORT_NONE, ITIDY_COLTYPE_NUMBER},
 *     {"Path", 30, 0, ITIDY_ALIGN_LEFT, TRUE, TRUE,
 *      ITIDY_SORT_NONE, ITIDY_COLTYPE_TEXT}
 * };
 * 
 * // Build entry list
 * struct List entry_list;
 * NewList(&entry_list);
 * iTidy_ListViewEntry *entry = whd_malloc(sizeof(iTidy_ListViewEntry));
 * entry->num_columns = 3;
 * entry->display_data = whd_malloc(sizeof(char*) * 3);
 * entry->sort_keys = whd_malloc(sizeof(char*) * 3);
 * entry->display_data[0] = strdup("24-Nov-2025 15:19");
 * entry->sort_keys[0] = strdup("20251124_151900");
 * // ... populate other columns ...
 * AddTail(&entry_list, (struct Node*)entry);
 * 
 * // Format with sorting
 * iTidy_ListViewState *state = NULL;
 * struct List *formatted = iTidy_FormatListViewColumns(cols, 3, &entry_list, 80, &state);
 * GT_SetGadgetAttrs(listview, window, NULL, GTLV_Labels, formatted, TAG_DONE);
 * 
 * // Later cleanup (important order!):
 * GT_SetGadgetAttrs(listview, window, NULL, GTLV_Labels, ~0, TAG_DONE);
 * iTidy_FreeFormattedList(formatted);
 * iTidy_FreeListViewState(state);
 * // Now free entry structures and their strings
 * @endcode
 */
struct List *iTidy_FormatListViewColumns(
    iTidy_ColumnConfig *columns,
    int num_columns,
    struct List *entries,
    int total_char_width,
    iTidy_ListViewState **out_state,
    iTidy_ListViewMode mode,    /* Display mode (FULL/FULL_NO_SORT/SIMPLE/SIMPLE_PAGINATED) */
    int page_size,              /* Entries per page (used with paginated modes, 0=auto-calculate) */
    int current_page,           /* 1-based page number (ignored if no pagination) */
    int *out_total_pages,       /* Returns total pages (can be NULL) */
    int nav_direction           /* Navigation direction for auto-select (-1=Prev, 0=None, +1=Next) */
);

/**
 * @brief Re-sort ListView based on column header click
 * 
 * Detects which column was clicked, sorts the entry list, and rebuilds
 * the formatted display list. Does NOT re-calculate widths - just rearranges.
 * 
 * Coordinate system:
 * - mouse_x, mouse_y are window-relative (from IntuiMessage)
 * - header_top is Y position of ListView gadget (gadget->TopEdge)
 * - header_height is one text row (font->tf_YSize)
 * - Column pixel positions in state are gadget-relative
 * 
 * @param formatted_list The formatted List returned by iTidy_FormatListViewColumns
 * @param entry_list The original entry list (will be sorted in-place)
 * @param state The state returned by iTidy_FormatListViewColumns
 * @param mouse_x Window-relative X coordinate
 * @param mouse_y Window-relative Y coordinate
 * @param header_top Y position of header row (gadget->TopEdge)
 * @param header_height Height of header row (font->tf_YSize)
 * @param gadget_left X position of ListView gadget (gadget->LeftEdge)
 * @param font_width Character width in pixels (font->tf_XSize)
 * @param columns Original column config (for sort_type)
 * @return TRUE if list was re-sorted, FALSE if click was outside header or invalid
 * 
 * @note Call GT_SetGadgetAttrs() with formatted_list after this to refresh display
 * @note Updates header row with new sort indicators (^ or v)
 * @note Entry list is sorted in-place and formatted list is rebuilt
 * 
 * Example:
 * @code
 * if (imsg->Class == IDCMP_MOUSEBUTTONS && imsg->Code == SELECTDOWN) {
 *     int header_top = lv_gadget->TopEdge;
 *     int header_height = font->tf_YSize;
 *     
 *     if (iTidy_ResortListViewByClick(formatted_list, entry_list, lv_state,
 *                                      imsg->MouseX, imsg->MouseY,
 *                                      header_top, header_height,
 *                                      lv_gadget->LeftEdge,
 *                                      font->tf_XSize,
 *                                      columns)) {
 *         // Refresh gadget
 *         GT_SetGadgetAttrs(lv_gadget, window, NULL,
 *                          GTLV_Labels, formatted_list,
 *                          TAG_DONE);
 *     }
 * }
 * @endcode
 */
/**
 * @brief Sort a list of ListView entries by a specific column
 * 
 * @param list List of iTidy_ListViewEntry to sort in-place
 * @param col Column index to sort by
 * @param type Type of column (TEXT, NUMBER, or DATE)
 * @param ascending TRUE for ascending, FALSE for descending
 */
void iTidy_SortListViewEntries(struct List *list, int col, iTidy_ColumnType type, BOOL ascending);

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
    iTidy_ColumnConfig *columns
);

/**
 * @brief High-level handler for ListView column sorting on mouse click
 * 
 * This is a convenience wrapper that handles ALL aspects of column-click sorting:
 * 1. Checks if click is within ListView header
 * 2. Sets busy pointer during operation
 * 3. Calls iTidy_ResortListViewByClick()
 * 4. Refreshes the ListView gadget
 * 5. Clears busy pointer
 * 
 * Call this from your IDCMP_MOUSEBUTTONS handler with SELECTDOWN.
 * 
 * @param window Intuition window containing the ListView
 * @param listview_gadget The ListView gadget to check/sort
 * @param formatted_list The formatted List returned by iTidy_FormatListViewColumns
 * @param entry_list The original entry list (will be sorted in-place)
 * @param state The state returned by iTidy_FormatListViewColumns
 * @param mouse_x Window-relative X coordinate (from IntuiMessage)
 * @param mouse_y Window-relative Y coordinate (from IntuiMessage)
 * @param font_height Font height for header (from prefsIControl.systemFontSize)
 * @param font_width Character width in pixels (from prefsIControl.systemFontCharWidth)
 * @param columns Original column config (for sort_type)
 * @param num_columns Number of columns
 * @return TRUE if list was re-sorted and gadget refreshed, FALSE if click was outside header
 * 
 * Example usage (replaces ~100 lines of boilerplate):
 * @code
 * case IDCMP_MOUSEBUTTONS:
 *     if (code == SELECTDOWN) {
 *         if (iTidy_HandleListViewSort(
 *                 window,
 *                 data->session_listview,
 *                 data->session_display_list,
 *                 &data->session_entry_list,
 *                 data->session_lv_state,
 *                 msg->MouseX, msg->MouseY,
 *                 prefsIControl.systemFontSize,
 *                 prefsIControl.systemFontCharWidth,
 *                 session_columns, 4)) {
 *             // List was sorted - that's it!
 *         }
 *     }
 *     break;
 * @endcode
 * 
 * @note Column definitions must match those used in iTidy_FormatListViewColumns()
 * @note This function handles busy pointer automatically - no need to set it manually
 * @note Gadget is refreshed automatically if sorting occurred
 */
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
    int num_columns
);

/**
 * @brief Map a ListView row selection to the corresponding entry in the sorted list
 * 
 * When a user clicks on a ListView row, GadTools returns a row index that includes
 * header rows (row 0 = column titles, row 1 = separator). This function:
 * 1. Subtracts header offset (2 rows)
 * 2. Walks the sorted entry list to find the Nth data entry
 * 3. Returns a pointer to that entry
 * 
 * This eliminates the need for callers to manually handle header offset and
 * traverse sorted lists.
 * 
 * @param entry_list The sorted entry list (as passed to iTidy_FormatListViewColumns)
 * @param listview_row The row index from GT_GetGadgetAttrs(GTLV_Selected)
 * @return Pointer to the selected entry, or NULL if row is invalid or in header
 * 
 * @note Returns NULL if listview_row < 2 (clicked on header/separator)
 * @note Returns NULL if listview_row exceeds the number of data entries
 * @note The returned pointer is valid as long as the entry_list remains intact
 * 
 * Example:
 * @code
 * LONG selected = -1;
 * GT_GetGadgetAttrs(listview, window, NULL, GTLV_Selected, &selected, TAG_END);
 * 
 * if (selected >= 0) {
 *     iTidy_ListViewEntry *entry = iTidy_GetSelectedEntry(&entry_list, selected);
 *     if (entry) {
 *         // Use entry->display_data or custom fields
 *         printf("Selected: %s\n", entry->display_data[0]);
 *     }
 * }
 * @endcode
 */
iTidy_ListViewEntry *iTidy_GetSelectedEntry(struct List *entry_list, LONG listview_row);

/*---------------------------------------------------------------------------*/
/* Smart Click Detection Helpers                                             */
/*---------------------------------------------------------------------------*/

/**
 * @brief Complete click information for a ListView row/column
 * 
 * This structure contains all relevant data about a ListView click,
 * eliminating the need for callers to manually extract values or
 * maintain column type mappings.
 */
typedef struct {
    iTidy_ListViewEntry *entry;       /* Selected entry (NULL if header row) */
    int column;                        /* Clicked column index (0-based, -1 if invalid) */
    const char *display_value;         /* Human-readable value: "2.8 MB", "24-Nov-2025" */
    const char *sort_key;              /* Machine-readable value: "0002949120", "20251124_151900" */
    iTidy_ColumnType column_type;      /* Column type: DATE/NUMBER/TEXT */
} iTidy_ListViewClick;

/*---------------------------------------------------------------------------*/
/* High-Level IDCMP Event Handler                                           */
/*---------------------------------------------------------------------------*/

/**
 * @brief Event types returned by iTidy_HandleListViewGadgetUp
 */
typedef enum {
    ITIDY_LV_EVENT_NONE,           /* No valid event (click outside, error, etc.) */
    ITIDY_LV_EVENT_HEADER_SORTED,  /* Header clicked, list was sorted */
    ITIDY_LV_EVENT_ROW_CLICK,      /* Data row clicked */
    ITIDY_LV_EVENT_NAV_HANDLED     /* Navigation row clicked, page changed internally (caller should rebuild list) */
} iTidy_ListViewEventType;

/**
 * @brief Complete event information from a ListView GADGETUP event
 * 
 * This structure contains all information about what happened when the user
 * clicked on a ListView gadget. The caller can switch on 'type' to determine
 * what action to take.
 */
typedef struct {
    iTidy_ListViewEventType type;  /* What kind of event occurred */
    
    /* Common fields */
    BOOL did_sort;                  /* TRUE if list was re-sorted (header click only) */
    
    /* Header event fields (valid when type == ITIDY_LV_EVENT_HEADER_SORTED) */
    int sorted_column;              /* Which column was clicked for sorting (0-based) */
    iTidy_SortOrder sort_order;     /* New sort order (ASCENDING/DESCENDING) */
    
    /* Row click fields (valid when type == ITIDY_LV_EVENT_ROW_CLICK) */
    iTidy_ListViewEntry *entry;     /* The clicked entry */
    int column;                      /* Which column was clicked (0-based, -1 if outside) */
    const char *display_value;       /* Human-readable value */
    const char *sort_key;            /* Machine-readable value */
    iTidy_ColumnType column_type;    /* Column type (DATE/NUMBER/TEXT) */
    
    /* Navigation fields (valid when type == ITIDY_LV_EVENT_NAV_HANDLED) */
    int nav_direction;               /* -1 = Previous, +1 = Next */
} iTidy_ListViewEvent;

/**
 * @brief High-level handler for ListView GADGETUP events
 * 
 * This function handles ALL ListView interaction in one call:
 * - Detects header vs data row clicks
 * - Performs sorting on header clicks
 * - Extracts clicked cell information on row clicks
 * - Returns a unified event structure
 * 
 * This eliminates ~100 lines of boilerplate from client code.
 * 
 * @param window Window containing the ListView
 * @param gadget The ListView gadget that was clicked
 * @param mouse_x Mouse X coordinate (from IntuiMessage->MouseX, BEFORE GT_ReplyIMsg)
 * @param mouse_y Mouse Y coordinate (from IntuiMessage->MouseY, BEFORE GT_ReplyIMsg)
 * @param entry_list List of iTidy_ListViewEntry nodes
 * @param display_list Formatted display list (from iTidy_FormatListViewColumns)
 * @param state ListView state (from iTidy_FormatListViewColumns)
 * @param font_height Height of font in pixels (e.g., font->tf_YSize)
 * @param font_width Width of font in pixels (e.g., font->tf_XSize)
 * @param columns Column configuration array
 * @param num_columns Number of columns
 * @param out_event Output event structure (filled by this function)
 * @return TRUE if event was processed, FALSE if error or no valid event
 * 
 * @note Caller must extract mouse_x/mouse_y BEFORE replying to IntuiMessage
 * @note If did_sort==TRUE, caller must refresh gadget with GT_SetGadgetAttrs
 * @note out_event is always filled with valid data (check 'type' field)
 * 
 * Usage example:
 * @code
 * case IDCMP_GADGETUP:
 *     if (gadget->GadgetID == GID_MY_LISTVIEW) {
 *         iTidy_ListViewEvent event;
 *         
 *         if (iTidy_HandleListViewGadgetUp(
 *                 window, gadget, mouseX, mouseY,
 *                 &entry_list, display_list, state,
 *                 font->tf_YSize, font->tf_XSize,
 *                 columns, num_columns, &event)) {
 *             
 *             // Refresh if sorted
 *             if (event.did_sort) {
 *                 GT_SetGadgetAttrs(gadget, window, NULL,
 *                                  GTLV_Labels, ~0, TAG_DONE);
 *                 GT_SetGadgetAttrs(gadget, window, NULL,
 *                                  GTLV_Labels, display_list, TAG_DONE);
 *             }
 *             
 *             // Handle event
 *             switch (event.type) {
 *                 case ITIDY_LV_EVENT_HEADER_SORTED:
 *                     log("Sorted by column %d, order %d\n",
 *                         event.sorted_column, event.sort_order);
 *                     break;
 *                     
 *                 case ITIDY_LV_EVENT_ROW_CLICK:
 *                     if (event.entry && event.column >= 0) {
 *                         process_cell_click(&event);
 *                     }
 *                     break;
 *                     
 *                 case ITIDY_LV_EVENT_NONE:
 *                     // Ignore
 *                     break;
 *             }
 *         }
 *     }
 *     break;
 * @endcode
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
    iTidy_ListViewEvent *out_event
);

/**
 * @brief Detect which column was clicked based on mouse X position
 * 
 * Uses pixel-based hit-testing against column boundaries stored in the
 * ListView state. Works for any row (header or data).
 * 
 * @param state ListView state with column boundary info (from iTidy_FormatListViewColumns)
 * @param mouse_x Window-relative X coordinate (from IntuiMessage->MouseX)
 * @param gadget_left Gadget's LeftEdge position (for coordinate adjustment)
 * @return Column index (0-based), or -1 if click is outside all column bounds
 * 
 * @note Automatically adjusts window coordinates to gadget-relative
 * @note Uses pixel_start/pixel_end for accurate hit-testing
 * @note Returns -1 if state is NULL or click is outside columns
 * 
 * Example:
 * @code
 * int column = iTidy_GetClickedColumn(lv_state, msg->MouseX, listview_gad->LeftEdge);
 * if (column >= 0) {
 *     printf("Clicked column %d\n", column);
 * }
 * @endcode
 */
int iTidy_GetClickedColumn(iTidy_ListViewState *state, WORD mouse_x, WORD gadget_left);

/**
 * @brief Get complete click information for a ListView selection (SMART HELPER)
 * 
 * This is the recommended high-level function for handling ListView clicks.
 * It combines entry lookup, column detection, and data extraction into a
 * single call, returning all relevant information in one structure.
 * 
 * What it provides:
 * - Entry pointer (NULL if header row clicked)
 * - Column index (0-based, -1 if outside bounds)
 * - Display value (formatted string for UI)
 * - Sort key (machine-readable value for parsing)
 * - Column type (DATE/NUMBER/TEXT for type-aware processing)
 * 
 * @param entry_list List of iTidy_ListViewEntry nodes (sorted order)
 * @param state ListView state with column info (from iTidy_FormatListViewColumns)
 * @param listview_row Row index from GT_GetGadgetAttrs(GTLV_Selected)
 * @param mouse_x Window-relative X coordinate (from IntuiMessage->MouseX)
 * @param gadget_left Gadget's LeftEdge position (for coordinate adjustment)
 * @return Structure with entry, column, values, and type (safe to use even if NULLs)
 * 
 * @note Returns entry=NULL if row < 2 (header/separator clicked)
 * @note Returns column=-1 if click is outside column bounds
 * @note Returns value=NULL and sort_key=NULL if entry or column is invalid
 * @note Column type is always set (defaults to TEXT if invalid)
 * 
 * Example (type-aware processing):
 * @code
 * iTidy_ListViewClick click = iTidy_GetListViewClick(
 *     &entry_list, lv_state, selected, msg->MouseX, listview_gad->LeftEdge
 * );
 * 
 * if (click.entry && click.column >= 0) {
 *     switch (click.column_type) {
 *         case ITIDY_COLTYPE_DATE:
 *             // Use sort_key for machine-readable date
 *             timestamp = parse_date(click.sort_key);  // "20251124_151900"
 *             break;
 *         case ITIDY_COLTYPE_NUMBER:
 *             // Use sort_key for accurate number
 *             int value = atoi(click.sort_key);  // "0000000013" -> 13
 *             break;
 *         case ITIDY_COLTYPE_TEXT:
 *             // Use display_value for text
 *             copy_to_clipboard(click.display_value);
 *             break;
 *     }
 * }
 * @endcode
 */
iTidy_ListViewClick iTidy_GetListViewClick(
    struct List *entry_list,
    iTidy_ListViewState *state,
    LONG listview_row,
    WORD mouse_x,
    WORD gadget_left
);

/**
 * @brief Free a formatted ListView list
 * 
 * Frees all nodes and their allocated ln_Name strings in the list.
 * The list must have been created by iTidy_FormatListViewColumns().
 * 
 * @param list List to free (can be NULL)
 * 
 * @note Always detach the list from the ListView gadget before freeing:
 * @code
 * GT_SetGadgetAttrs(listview, window, NULL, GTLV_Labels, ~0, TAG_DONE);
 * iTidy_FreeFormattedList(list);
 * @endcode
 */
void iTidy_FreeFormattedList(struct List *list);

/**
 * @brief Free ListView state tracking structure
 * 
 * Frees the state structure returned by iTidy_FormatListViewColumns().
 * 
 * @param state State to free (can be NULL)
 */
void iTidy_FreeListViewState(iTidy_ListViewState *state);

/**
 * @brief Free all resources associated with a ListView entry list
 * 
 * Performs complete cleanup of ListView resources in the correct order.
 * This is the recommended cleanup function - use it instead of manually
 * freeing each component separately.
 * 
 * Frees (in order):
 * 1. All entries in entry_list:
 *    - entry->node.ln_Name (formatted display string)
 *    - entry->display_data[col] for each column
 *    - entry->sort_keys[col] for each column  
 *    - entry->display_data array
 *    - entry->sort_keys array
 *    - entry structure itself
 * 2. display_list (via iTidy_FreeFormattedList)
 * 3. state (via iTidy_FreeListViewState)
 * 
 * @param entry_list    List of iTidy_ListViewEntry nodes (can be NULL)
 * @param display_list  Display list for ListView gadget (can be NULL)
 * @param state         ListView state with column info (can be NULL)
 * 
 * @note CRITICAL: Detach lists from gadgets BEFORE calling:
 *       GT_SetGadgetAttrs(gadget, window, NULL, GTLV_Labels, ~0, TAG_DONE);
 * @note Safe to call with NULL parameters - handles them gracefully
 * @note Does NOT close windows or free gadgets - caller must do that first
 * 
 * Example:
 * @code
 * // 1. Detach from gadget FIRST
 * GT_SetGadgetAttrs(data->listview, data->window, NULL,
 *                   GTLV_Labels, ~0, TAG_DONE);
 * 
 * // 2. Free all ListView resources
 * itidy_free_listview_entries(&data->entry_list,
 *                             data->display_list,
 *                             data->lv_state);
 * 
 * // 3. NULL out pointers
 * data->display_list = NULL;
 * data->lv_state = NULL;
 * @endcode
 */
void itidy_free_listview_entries(struct List *entry_list,
                                 struct List *display_list,
                                 iTidy_ListViewState *state);

/*---------------------------------------------------------------------------*/
/* Backward Compatibility (Old API - No Sorting)                            */
/*---------------------------------------------------------------------------*/

/**
 * @brief Format data into a columnar ListView (Legacy API - No Sorting)
 * 
 * This is the old API for backward compatibility. It does not support sorting.
 * For new code, use the entry-based API with iTidy_ListViewEntry.
 * 
 * @param columns Array of column configurations (ignore default_sort/sort_type)
 * @param num_columns Number of columns
 * @param data_rows 2D array of data: data_rows[row][column]
 * @param num_rows Number of data rows
 * @param total_char_width Total character width available (0 = calculate from data)
 * @return Allocated struct List ready for GTLV_Labels, or NULL on error
 * 
 * @note Caller must free the returned list with iTidy_FreeFormattedList()
 * @note This API cannot be sorted - use iTidy_FormatListViewColumns() for sorting
 */
struct List *iTidy_FormatListViewColumns_Legacy(
    iTidy_ColumnConfig *columns,
    int num_columns,
    const char ***data_rows,
    int num_rows,
    int total_char_width
);

/*---------------------------------------------------------------------------*/
/* Helper Functions                                                          */
/*---------------------------------------------------------------------------*/

/**
 * @brief Calculate actual column widths from configuration and entry data
 * 
 * Internal helper - analyzes entry data and applies min/max constraints to
 * determine final column widths. Exposed for advanced use cases.
 * 
 * @param columns Column configurations
 * @param num_columns Number of columns
 * @param entries List of iTidy_ListViewEntry nodes
 * @param total_char_width Total available width (0 = auto)
 * @param out_widths Output array to receive calculated widths (must be pre-allocated)
 * @return TRUE on success, FALSE on error
 */
BOOL iTidy_CalculateColumnWidths(
    iTidy_ColumnConfig *columns,
    int num_columns,
    struct List *entries,
    int total_char_width,
    int *out_widths
);

#endif /* LISTVIEW_COLUMNS_API_H */
