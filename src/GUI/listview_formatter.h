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
/* Column Configuration                                                      */
/*---------------------------------------------------------------------------*/

typedef struct {
    const char *title;         /* Column header text */
    int min_width;             /* Minimum width in characters (0 = auto from title) */
    int max_width;             /* Maximum width in characters (0 = unlimited) */
    iTidy_ColumnAlign align;   /* Text alignment */
    BOOL flexible;             /* TRUE if column should absorb extra space */
} iTidy_ColumnConfig;

/*---------------------------------------------------------------------------*/
/* Main Formatting Function                                                  */
/*---------------------------------------------------------------------------*/

/**
 * @brief Format data into a columnar ListView with automatic width calculation
 * 
 * Takes column definitions and data rows, calculates optimal column widths,
 * and returns a formatted struct List ready for use with GadTools ListView.
 * 
 * The returned list includes:
 * - Header row with column titles
 * - Separator row (dashes)
 * - Data rows with proper alignment and padding
 * 
 * @param columns Array of column configurations
 * @param num_columns Number of columns
 * @param data_rows 2D array of data: data_rows[row][column]
 * @param num_rows Number of data rows
 * @param total_char_width Total character width available (0 = calculate from data)
 * @return Allocated struct List ready for GTLV_Labels, or NULL on error
 * 
 * @note Caller must free the returned list with iTidy_FreeFormattedList()
 * @note Each column is separated by " | " in the output
 * @note Exactly one column should have flexible=TRUE to absorb remaining space
 * 
 * Example:
 * @code
 * iTidy_ColumnConfig cols[] = {
 *     {"Date/Time", 20, 20, ITIDY_ALIGN_LEFT, FALSE},
 *     {"Mode", 6, 8, ITIDY_ALIGN_LEFT, FALSE},
 *     {"Path", 30, 0, ITIDY_ALIGN_LEFT, TRUE},  // Flexible
 *     {"Count", 4, 6, ITIDY_ALIGN_RIGHT, FALSE}
 * };
 * 
 * const char *row1[] = {"20251124_111654", "Batch", "PC:Workbench", "2"};
 * const char *row2[] = {"20251124_111711", "Single", "PC:dump.txt.info", "1"};
 * const char **rows[] = {row1, row2};
 * 
 * struct List *list = iTidy_FormatListViewColumns(cols, 4, rows, 2, 80);
 * GT_SetGadgetAttrs(listview, window, NULL, GTLV_Labels, list, TAG_DONE);
 * @endcode
 */
struct List *iTidy_FormatListViewColumns(
    iTidy_ColumnConfig *columns,
    int num_columns,
    const char ***data_rows,
    int num_rows,
    int total_char_width
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

/*---------------------------------------------------------------------------*/
/* Helper Functions                                                          */
/*---------------------------------------------------------------------------*/

/**
 * @brief Calculate actual column widths from configuration and data
 * 
 * Internal helper - analyzes data and applies min/max constraints to
 * determine final column widths. Exposed for advanced use cases.
 * 
 * @param columns Column configurations
 * @param num_columns Number of columns
 * @param data_rows Data to analyze
 * @param num_rows Number of rows
 * @param total_char_width Total available width (0 = auto)
 * @param out_widths Output array to receive calculated widths (must be pre-allocated)
 * @return TRUE on success, FALSE on error
 */
BOOL iTidy_CalculateColumnWidths(
    iTidy_ColumnConfig *columns,
    int num_columns,
    const char ***data_rows,
    int num_rows,
    int total_char_width,
    int *out_widths
);

#endif /* LISTVIEW_FORMATTER_H */
