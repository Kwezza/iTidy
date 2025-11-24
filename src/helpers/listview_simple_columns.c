/**
 * listview_simple_columns.c - Lightweight ListView Column Formatter
 * 
 * Implementation of minimal column formatting API for Workbench 3.x
 * 
 * TARGET: < 2 seconds for 1000 rows on 68000 @ 7MHz
 * MEMORY: ~200KB for 1000 rows (vs 500KB with full API)
 */

#include "listview_simple_columns.h"
#include "../path_utilities.h"
#include "../writeLog.h"
#include "platform/platform.h"

#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>
#include <stdio.h>

/*---------------------------------------------------------------------------*/
/* Constants                                                                 */
/*---------------------------------------------------------------------------*/

#define COLUMN_SEPARATOR " | "  /* Separator between columns */
#define SEPARATOR_WIDTH 3       /* Length of " | " */

/*---------------------------------------------------------------------------*/
/* Internal Helpers                                                          */
/*---------------------------------------------------------------------------*/

/**
 * @brief Calculate total buffer size needed for formatted row
 * 
 * @param columns Column definitions
 * @param num_columns Number of columns
 * @return Required buffer size in bytes (including null terminator)
 */
static int calculate_row_buffer_size(const iTidy_SimpleColumn *columns, int num_columns)
{
    int total = 0;
    int col;
    
    /* Sum all column widths */
    for (col = 0; col < num_columns; col++) {
        total += columns[col].char_width;
    }
    
    /* Add separator widths (num_columns - 1 separators) */
    if (num_columns > 1) {
        total += (num_columns - 1) * SEPARATOR_WIDTH;
    }
    
    /* Add null terminator */
    total += 1;
    
    return total;
}

/**
 * @brief Format a single cell with alignment and optional path truncation
 * 
 * Fills output buffer with padded/truncated cell text. Output is always
 * exactly 'width' characters (not including null terminator).
 * 
 * @param output Pre-allocated buffer (must be at least width+1 bytes)
 * @param text Cell text (can be NULL - treated as empty)
 * @param width Target width in characters
 * @param align Alignment mode
 * @param smart_path TRUE to use /../ notation for paths
 */
static void format_cell(char *output, const char *text, int width, 
                       iTidy_SimpleAlign align, BOOL smart_path)
{
    char temp_buffer[256];
    const char *display_text;
    int len;
    int padding;
    int i;
    
    if (!output) return;
    
    /* Handle NULL text */
    if (!text) {
        text = "";
    }
    
    len = strlen(text);
    
    /* Handle path abbreviation if needed */
    if (smart_path && len > width) {
        /* Try intelligent path abbreviation with /../ notation */
        if (iTidy_ShortenPathWithParentDir(text, temp_buffer, width)) {
            display_text = temp_buffer;
            len = strlen(temp_buffer);  /* Update length after abbreviation */
        } else {
            /* Path abbreviation failed or not applicable */
            display_text = text;
        }
    } else {
        display_text = text;
    }
    
    /* Truncate if still too long (even after path abbreviation) */
    if (len > width) {
        if (width >= 3) {
            /* Truncate with "..." */
            strncpy(temp_buffer, display_text, width - 3);
            temp_buffer[width - 3] = '\0';
            strcat(temp_buffer, "...");
            display_text = temp_buffer;
            len = width;
        } else {
            /* Too narrow for "...", just truncate */
            strncpy(temp_buffer, display_text, width);
            temp_buffer[width] = '\0';
            display_text = temp_buffer;
            len = width;
        }
    }
    
    /* Calculate padding needed */
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
/* Public API Implementation                                                 */
/*---------------------------------------------------------------------------*/

char *iTidy_FormatHeader(const iTidy_SimpleColumn *columns, int num_columns)
{
    char *buffer;
    char cell_buffer[256];
    int buffer_size;
    int pos;
    int col;
    
    /* Validate parameters */
    if (!columns || num_columns <= 0) {
        log_error(LOG_GUI, "iTidy_FormatHeader: Invalid parameters\n");
        return NULL;
    }
    
    /* Calculate buffer size */
    buffer_size = calculate_row_buffer_size(columns, num_columns);
    
    /* Allocate buffer using tracked allocation */
    buffer = (char *)whd_malloc(buffer_size);
    if (!buffer) {
        log_error(LOG_GUI, "iTidy_FormatHeader: Out of memory (%d bytes)\n", buffer_size);
        return NULL;
    }
    
    /* Zero the buffer (whd_malloc doesn't auto-clear) */
    memset(buffer, 0, buffer_size);
    
    /* Format each column header */
    pos = 0;
    for (col = 0; col < num_columns; col++) {
        /* Format header text with alignment */
        format_cell(cell_buffer, columns[col].title, columns[col].char_width,
                   columns[col].align, FALSE);  /* Never path-truncate headers */
        
        /* Copy to output buffer */
        strcpy(buffer + pos, cell_buffer);
        pos += columns[col].char_width;
        
        /* Add separator between columns */
        if (col < num_columns - 1) {
            strcpy(buffer + pos, COLUMN_SEPARATOR);
            pos += SEPARATOR_WIDTH;
        }
    }
    
    return buffer;
}

char *iTidy_FormatSeparator(const iTidy_SimpleColumn *columns, int num_columns)
{
    char *buffer;
    int buffer_size;
    int pos;
    int col;
    int i;
    
    /* Validate parameters */
    if (!columns || num_columns <= 0) {
        log_error(LOG_GUI, "iTidy_FormatSeparator: Invalid parameters\n");
        return NULL;
    }
    
    /* Calculate buffer size */
    buffer_size = calculate_row_buffer_size(columns, num_columns);
    
    /* Allocate buffer using tracked allocation */
    buffer = (char *)whd_malloc(buffer_size);
    if (!buffer) {
        log_error(LOG_GUI, "iTidy_FormatSeparator: Out of memory (%d bytes)\n", buffer_size);
        return NULL;
    }
    
    /* Zero the buffer (whd_malloc doesn't auto-clear) */
    memset(buffer, 0, buffer_size);
    
    /* Build separator row with dashes and + connectors */
    pos = 0;
    for (col = 0; col < num_columns; col++) {
        /* Fill column width with dashes */
        for (i = 0; i < columns[col].char_width; i++) {
            buffer[pos++] = '-';
        }
        
        /* Add connector between columns */
        if (col < num_columns - 1) {
            buffer[pos++] = '-';
            buffer[pos++] = '+';
            buffer[pos++] = '-';
        }
    }
    
    buffer[pos] = '\0';
    return buffer;
}

char *iTidy_FormatRow(const iTidy_SimpleColumn *columns, int num_columns,
                     const char **cell_values)
{
    char *buffer;
    char cell_buffer[256];
    int buffer_size;
    int pos;
    int col;
    
    /* Validate parameters */
    if (!columns || num_columns <= 0) {
        log_error(LOG_GUI, "iTidy_FormatRow: Invalid parameters\n");
        return NULL;
    }
    
    /* cell_values can be NULL (treat all cells as empty) */
    
    /* Calculate buffer size */
    buffer_size = calculate_row_buffer_size(columns, num_columns);
    
    /* Allocate buffer using tracked allocation */
    buffer = (char *)whd_malloc(buffer_size);
    if (!buffer) {
        log_error(LOG_GUI, "iTidy_FormatRow: Out of memory (%d bytes)\n", buffer_size);
        return NULL;
    }
    
    /* Zero the buffer (whd_malloc doesn't auto-clear) */
    memset(buffer, 0, buffer_size);
    
    /* Format each cell */
    pos = 0;
    for (col = 0; col < num_columns; col++) {
        const char *cell_text = (cell_values && cell_values[col]) ? 
                                cell_values[col] : "";
        
        /* Format cell with alignment and optional path truncation */
        format_cell(cell_buffer, cell_text, columns[col].char_width,
                   columns[col].align, columns[col].smart_path_truncate);
        
        /* Copy to output buffer */
        strcpy(buffer + pos, cell_buffer);
        pos += columns[col].char_width;
        
        /* Add separator between columns */
        if (col < num_columns - 1) {
            strcpy(buffer + pos, COLUMN_SEPARATOR);
            pos += SEPARATOR_WIDTH;
        }
    }
    
    return buffer;
}

int iTidy_DetectClickedColumn(const iTidy_SimpleColumn *columns, int num_columns,
                          WORD mouse_x, WORD gadget_left, WORD font_width)
{
    int char_pos;
    int current_char = 0;
    int col;
    
    /* Validate parameters */
    if (!columns || num_columns <= 0 || font_width <= 0) {
        return -1;
    }
    
    /* Convert mouse X to character position within ListView */
    char_pos = (mouse_x - gadget_left) / font_width;
    
    /* Find which column this falls into */
    for (col = 0; col < num_columns; col++) {
        int col_start = current_char;
        int col_end = current_char + columns[col].char_width;
        
        /* Check if click is within this column */
        if (char_pos >= col_start && char_pos < col_end) {
            return col;
        }
        
        /* Move to next column (add column width + separator width) */
        current_char = col_end + SEPARATOR_WIDTH;
    }
    
    /* Click was outside column area (probably on separator or past end) */
    return -1;
}

int iTidy_CalcListViewChars(int listview_pixel_width, WORD font_width)
{
    const int SCROLLBAR_AND_BORDER = 36;  /* 20px scrollbar + 16px borders */
    int usable_width;
    
    if (font_width <= 0) {
        return 0;
    }
    
    usable_width = listview_pixel_width - SCROLLBAR_AND_BORDER;
    if (usable_width < 0) {
        usable_width = 0;
    }
    
    return usable_width / font_width;
}
