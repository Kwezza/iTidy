/**
 * listview_formatter.c - Automatic ListView Column Formatter Implementation
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
 * 4. MEMORY - Always Free the Returned List:
 *    ```c
 *    struct List *formatted = iTidy_FormatListViewColumns(...);
 *    // ... use the list ...
 *    iTidy_FreeFormattedList(formatted);  // Don't forget!
 *    ```
 * 
 * 5. COLUMN CONFIG - Example Setup:
 *    ```c
 *    iTidy_ColumnConfig cols[] = {
 *        {"Date/Time", 20, 30, ITIDY_ALIGN_LEFT, FALSE},  // Fixed width
 *        {"Mode", 6, 8, ITIDY_ALIGN_LEFT, FALSE},         // Fixed width
 *        {"Path", 30, 200, ITIDY_ALIGN_LEFT, TRUE},       // Flexible (takes remaining)
 *        {"Changed", 7, 12, ITIDY_ALIGN_RIGHT, FALSE}     // Right-aligned numbers
 *    };
 *    ```
 * 
 * ============================================================================
 */

#include "platform/platform.h"
#include "listview_formatter.h"
#include "../writeLog.h"
#include "../path_utilities.h"

#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <string.h>
#include <stdio.h>

/* Separator between columns */
#define COLUMN_SEPARATOR " | "
#define SEPARATOR_WIDTH 3

/*---------------------------------------------------------------------------*/
/* Helper: Calculate actual column widths                                    */
/*---------------------------------------------------------------------------*/

BOOL iTidy_CalculateColumnWidths(
    iTidy_ColumnConfig *columns,
    int num_columns,
    const char ***data_rows,
    int num_rows,
    int total_char_width,
    int *out_widths)
{
    int i, row, col;
    int separator_total;
    int used_width;
    int available_width;
    int flexible_col = -1;
    int max_len;
    
    if (!columns || !out_widths || num_columns <= 0) {
        return FALSE;
    }
    
    /* Pass 1: Measure each column's required width */
    for (col = 0; col < num_columns; col++) {
        /* Start with title width */
        max_len = columns[col].title ? strlen(columns[col].title) : 0;
        
        /* Check all data rows for this column */
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
        
        /* Apply minimum width constraint */
        if (columns[col].min_width > 0 && max_len < columns[col].min_width) {
            max_len = columns[col].min_width;
        }
        
        /* Apply maximum width constraint */
        if (columns[col].max_width > 0 && max_len > columns[col].max_width) {
            max_len = columns[col].max_width;
        }
        
        out_widths[col] = max_len;
        
        /* Track flexible column */
        if (columns[col].flexible) {
            if (flexible_col >= 0) {
                log_warning(LOG_GUI, "Multiple flexible columns defined (col %d and %d), using first\n", 
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
        
        log_debug(LOG_GUI, "Flexible column %d adjusted to %d chars (total=%d, fixed=%d)\n",
                 flexible_col, flex_width, total_char_width, fixed_width);
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
/* Main Formatting Function                                                  */
/*---------------------------------------------------------------------------*/

struct List *iTidy_FormatListViewColumns(
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
    
    log_info(LOG_GUI, "iTidy_FormatListViewColumns: num_columns=%d, num_rows=%d, width=%d\n",
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
    
    /* Calculate column widths */
    if (!iTidy_CalculateColumnWidths(columns, num_columns, data_rows, num_rows, 
                                     total_char_width, col_widths)) {
        log_error(LOG_GUI, "Failed to calculate column widths\n");
        whd_free(col_widths);
        whd_free(list);
        return NULL;
    }
    
    /* Calculate buffer size needed */
    buffer_size = 0;
    for (col = 0; col < num_columns; col++) {
        buffer_size += col_widths[col];
    }
    buffer_size += (num_columns - 1) * SEPARATOR_WIDTH;
    buffer_size += 5;  /* Null terminator + 4 extra chars for separator row */
    
    /* Allocate buffers */
    row_buffer = (char *)whd_malloc(buffer_size);
    cell_buffer = (char *)whd_malloc(256);  /* Temp buffer for cell formatting */
    
    if (!row_buffer || !cell_buffer) {
        log_error(LOG_GUI, "Failed to allocate formatting buffers\n");
        if (row_buffer) whd_free(row_buffer);
        if (cell_buffer) whd_free(cell_buffer);
        whd_free(col_widths);
        whd_free(list);
        return NULL;
    }
    
    /* ===== CREATE HEADER ROW ===== */
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
        log_debug(LOG_GUI, "Header: %s\n", row_buffer);
    }
    
    /* ===== CREATE SEPARATOR ROW ===== */
    /* Add 4 extra dashes to ensure separator fills the full ListView width */
    memset(row_buffer, '-', pos + 4);
    row_buffer[pos + 4] = '\0';
    
    node = create_display_node(row_buffer);
    if (node) {
        AddTail(list, node);
    }
    
    /* ===== CREATE DATA ROWS ===== */
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
                log_debug(LOG_GUI, "Row %d: %s\n", row, row_buffer);
            }
        }
    }
    
    /* Cleanup */
    whd_free(row_buffer);
    whd_free(cell_buffer);
    whd_free(col_widths);
    
    log_info(LOG_GUI, "Formatted %d columns x %d rows (+2 header) for ListView\n", 
             num_columns, num_rows);
    
    return list;
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
