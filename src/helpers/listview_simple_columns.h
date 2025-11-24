/**
 * listview_simple_columns.h - Lightweight ListView Column Formatter
 * 
 * A minimal, performance-focused API for creating columnar ListViews on
 * Workbench 3.x systems. Designed for stock Amiga 500/600 (68000 @ 7MHz).
 * 
 * ============================================================================
 * DESIGN PHILOSOPHY
 * ============================================================================
 * 
 * This API is intentionally LIMITED to what's practical on stock hardware:
 * 
 * ✅ Fixed-width columns (you specify exact widths)
 * ✅ Smart path truncation with /../ notation
 * ✅ Column alignment (LEFT/RIGHT/CENTER)
 * ✅ Header and separator row generation
 * ✅ Optional click-to-sort column detection
 * ✅ ONE allocation per row (the formatted ln_Name string)
 * ✅ NO state tracking, NO preprocessing, NO dataset scanning
 * 
 * ❌ NO auto-width calculation (too slow - O(n) dataset scan)
 * ❌ NO flexible columns (adds complexity)
 * ❌ NO separate sort keys (sort your original data)
 * ❌ NO pagination (GadTools handles scrolling)
 * ❌ NO display list wrappers (use raw struct Node)
 * 
 * PERFORMANCE TARGET: 1000 rows formatted in < 2 seconds on 68000 @ 7MHz
 * MEMORY TARGET: ~200KB for 1000 rows (vs 500KB with full API)
 * 
 * ============================================================================
 * USAGE EXAMPLE
 * ============================================================================
 * 
 * ```c
 * // 1. Define columns (static const - zero runtime cost)
 * static const iTidy_SimpleColumn backup_columns[] = {
 *     {"Run",       4,  ITIDY_ALIGN_RIGHT, FALSE},
 *     {"Date/Time", 20, ITIDY_ALIGN_LEFT,  FALSE},
 *     {"Folders",   7,  ITIDY_ALIGN_RIGHT, FALSE},
 *     {"Size",      8,  ITIDY_ALIGN_RIGHT, FALSE},
 *     {"Status",    10, ITIDY_ALIGN_LEFT,  FALSE}
 * };
 * 
 * // 2. Build ListView list
 * struct List entry_list;
 * NewList(&entry_list);
 * 
 * // Add header
 * struct Node *header = AllocVec(sizeof(struct Node), MEMF_CLEAR);
 * header->ln_Name = iTidy_FormatHeader(backup_columns, 5);
 * AddTail(&entry_list, header);
 * 
 * // Add separator
 * struct Node *separator = AllocVec(sizeof(struct Node), MEMF_CLEAR);
 * separator->ln_Name = iTidy_FormatSeparator(backup_columns, 5);
 * AddTail(&entry_list, separator);
 * 
 * // Add data rows
 * while (scan_backups(...)) {
 *     struct Node *entry = AllocVec(sizeof(struct Node), MEMF_CLEAR);
 *     
 *     const char *values[] = {run_num, date, folders, size, status};
 *     entry->ln_Name = iTidy_FormatRow(backup_columns, 5, values);
 *     
 *     AddTail(&entry_list, entry);
 * }
 * 
 * // 3. Attach to ListView
 * GT_SetGadgetAttrs(listview, window, NULL, GTLV_Labels, &entry_list, TAG_DONE);
 * 
 * // 4. Handle clicks (optional - for sorting)
 * case IDCMP_GADGETUP:
 *     if (gadget->GadgetID == GID_BACKUP_LIST) {
 *         int col = iTidy_DetectClickedColumn(backup_columns, 5,
 *                                          msg->MouseX,
 *                                          gadget->LeftEdge,
 *                                          font_width);
 *         
 *         if (col >= 0) {
 *             // Re-sort your data, rebuild list
 *             sort_backup_list_by_column(col);
 *             rebuild_listview();
 *         }
 *     }
 *     break;
 * 
 * // 5. Cleanup (same as any ListView)
 * GT_SetGadgetAttrs(listview, window, NULL, GTLV_Labels, ~0, TAG_DONE);
 * while ((node = RemHead(&entry_list))) {
 *     if (node->ln_Name) whd_free(node->ln_Name);
 *     FreeVec(node);
 * }
 * ```
 * 
 * ============================================================================
 * VISUAL OUTPUT
 * ============================================================================
 * 
 * Run | Date/Time           | Folders | Size    | Status
 * ----+---------------------+---------+---------+----------
 *   16| 08-Nov-2025 11:47   |       1 | 10.0 kB | Complete
 *   15| 30-Oct-2025 08:42   |       1 |      0 B| Complete
 *   14| 30-Oct-2025 08:36   |       1 |    621 B| Complete
 * 
 * ============================================================================
 */

#ifndef ITIDY_LISTVIEW_SIMPLE_COLUMNS_H
#define ITIDY_LISTVIEW_SIMPLE_COLUMNS_H

#include <exec/types.h>
#include <exec/nodes.h>

/*---------------------------------------------------------------------------*/
/* Types                                                                     */
/*---------------------------------------------------------------------------*/

/**
 * @brief Column alignment options
 */
typedef enum {
    ITIDY_ALIGN_LEFT = 0,    /* "text     " */
    ITIDY_ALIGN_RIGHT = 1,   /* "     text" */
    ITIDY_ALIGN_CENTER = 2   /* "  text   " */
} iTidy_SimpleAlign;

/**
 * @brief Simple column definition
 * 
 * Defines a single column's properties. Create an array of these as
 * static const to avoid runtime allocation.
 */
typedef struct {
    const char *title;        /* Column header text (e.g., "Date/Time") */
    int char_width;           /* Column width in characters */
    iTidy_SimpleAlign align;  /* Text alignment within column */
    BOOL smart_path_truncate; /* TRUE = use /../ notation for paths */
} iTidy_SimpleColumn;

/*---------------------------------------------------------------------------*/
/* Core Formatting Functions                                                */
/*---------------------------------------------------------------------------*/

/**
 * @brief Format header row with column titles
 * 
 * Creates a formatted header string like:
 *   "Run | Date/Time           | Folders | Size    | Status"
 * 
 * @param columns Column definitions array
 * @param num_columns Number of columns
 * @return Allocated string (caller must whd_free), or NULL on error
 * 
 * @note Header text is aligned according to column->align setting
 */
char *iTidy_FormatHeader(
    const iTidy_SimpleColumn *columns,
    int num_columns
);

/**
 * @brief Format separator row with dashes and + connectors
 * 
 * Creates a separator string like:
 *   "----+---------------------+---------+---------+----------"
 * 
 * @param columns Column definitions array
 * @param num_columns Number of columns
 * @return Allocated string (caller must whd_free), or NULL on error
 */
char *iTidy_FormatSeparator(
    const iTidy_SimpleColumn *columns,
    int num_columns
);

/**
 * @brief Format data row with cell values
 * 
 * Creates a formatted data row like:
 *   "  16| 08-Nov-2025 11:47   |       1 | 10.0 kB | Complete"
 * 
 * @param columns Column definitions array
 * @param num_columns Number of columns
 * @param cell_values Array of strings (one per column, can be NULL)
 * @return Allocated string (caller must whd_free), or NULL on error
 * 
 * @note If cell_values[i] is NULL, column is filled with spaces
 * @note If smart_path_truncate is TRUE for a column, path is abbreviated
 *       using /../ notation (e.g., "Work:Projects/../file.txt")
 */
char *iTidy_FormatRow(
    const iTidy_SimpleColumn *columns,
    int num_columns,
    const char **cell_values
);

/*---------------------------------------------------------------------------*/
/* Optional Helpers                                                          */
/*---------------------------------------------------------------------------*/

/**
 * @brief Detect which column was clicked based on mouse X position
 * 
 * Calculates which column the user clicked by converting mouse coordinates
 * to column index. Useful for implementing click-to-sort.
 * 
 * @param columns Column definitions array
 * @param num_columns Number of columns
 * @param mouse_x Mouse X coordinate from IntuiMessage
 * @param gadget_left Gadget->LeftEdge (ListView left position)
 * @param font_width Font character width (e.g., prefsIControl.systemFontCharWidth)
 * @return Column index (0-based), or -1 if clicked on separator or outside
 * 
 * @note This is a FREE bonus - lets you sort without the heavy API overhead
 * @note You still need to sort your original data and rebuild the ListView
 */
int iTidy_DetectClickedColumn(
    const iTidy_SimpleColumn *columns,
    int num_columns,
    WORD mouse_x,
    WORD gadget_left,
    WORD font_width
);

/**
 * @brief Calculate ListView width in characters from gadget dimensions
 * 
 * Helper to convert ListView pixel width to character width for column planning.
 * 
 * @param listview_pixel_width ListView width in pixels
 * @param font_width Font character width (e.g., prefsIControl.systemFontCharWidth)
 * @return Usable width in characters (accounts for scrollbar + borders)
 * 
 * @note Deducts 36 pixels for scrollbar (20px) + borders (16px)
 * 
 * Example:
 *   int lv_width = 584;  // ListView pixel width
 *   int chars = iTidy_CalcListViewChars(lv_width, 8);  // Returns ~68 chars
 *   // Now distribute: Date(20) + Mode(8) + Path(40) = 68 chars total
 */
int iTidy_CalcListViewChars(
    int listview_pixel_width,
    WORD font_width
);

#endif /* ITIDY_LISTVIEW_SIMPLE_COLUMNS_H */
