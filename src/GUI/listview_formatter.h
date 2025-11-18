/**
 * listview_formatter.h - Automatic ListView Column Formatter
 * 
 * Provides utilities for creating properly aligned, multi-column ListView displays
 * with automatic width calculation and professional formatting.
 * 
 * Features:
 * - Auto-calculates optimal column widths from data
 * - Generates header and separator rows automatically
 * - Supports left/right/center alignment per column
 * - Flexible columns that expand to fill available space
 * - Smart truncation with "..." for overflow
 * - Returns ready-to-use struct List for GadTools GTLV_Labels
 */

#ifndef LISTVIEW_FORMATTER_H
#define LISTVIEW_FORMATTER_H

#include <exec/types.h>
#include <exec/lists.h>

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
} iTidy_ColumnState;

/**
 * @brief Overall ListView state for sorting
 * 
 * Returned by iTidy_FormatListViewColumns and used by iTidy_ResortListViewByClick.
 * Tracks all columns and their current sort states.
 */
typedef struct {
    iTidy_ColumnState *columns; /* Array of column states */
    int num_columns;            /* Number of columns */
    int separator_width;        /* Width of " | " separator (3 chars) */
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
    iTidy_ListViewState **out_state
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

#endif /* LISTVIEW_FORMATTER_H */
