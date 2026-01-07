/**
 * list_formatter.c - Justified String List Formatter Implementation
 * 
 * Creates Exec Lists with strings aligned by a delimiter character.
 * Useful for displaying formatted detail panels in ListViews.
 * 
 * ============================================================================
 * USAGE GUIDE
 * ============================================================================
 * 
 * PURPOSE:
 *   Automatically aligns text by a delimiter character (typically ':') by
 *   adding padding spaces. Perfect for details panels showing key-value pairs.
 * 
 * BASIC USAGE:
 * 
 *   1. Include the header:
 *      #include "helpers/list_formatter.h"
 * 
 *   2. Prepare your strings (in buffers or arrays):
 *      char line_buffer[7][256];
 *      sprintf(line_buffer[0], "Run Number: %04u", run_num);
 *      sprintf(line_buffer[1], "Date Created: %s", date_str);
 *      sprintf(line_buffer[2], "Source Directory: %s", source_path);
 *      ...etc...
 * 
 *   3. Create pointer array:
 *      const char *detail_lines[7];
 *      detail_lines[0] = line_buffer[0];
 *      detail_lines[1] = line_buffer[1];
 *      ...etc...
 * 
 *   4. Call the formatter:
 *      struct List *formatted_list = itidy_create_justified_list(
 *          detail_lines,   // Array of string pointers
 *          7,              // Number of strings
 *          ':',            // Character to align by
 *          0               // Max width (0 = unlimited)
 *      );
 * 
 *   5. Use with GadTools ListView:
 *      GT_SetGadgetAttrs(listview_gadget, window, NULL,
 *          GTLV_Labels, formatted_list,
 *          TAG_END);
 * 
 *   6. Cleanup when done (CRITICAL - always call this!):
 *      GT_SetGadgetAttrs(listview_gadget, window, NULL,
 *          GTLV_Labels, ~0,  // Detach list first!
 *          TAG_END);
 *      itidy_free_justified_list(formatted_list);
 * 
 * EXAMPLE OUTPUT:
 * 
 *   Input:
 *     "Run Number: 0015"
 *     "Date Created: 2025-10-30 08:36:01"
 *     "Source Directory: Workbench:Help"
 *     "Total Archives: 1"
 *     "Total Size: 621 B"
 *     "Status: Complete"
 * 
 *   Output (with ':' as justify_char):
 *     "      Run Number: 0015"
 *     "    Date Created: 2025-10-30 08:36:01"
 *     "Source Directory: Workbench:Help"  <- Longest prefix
 *     "  Total Archives: 1"
 *     "      Total Size: 621 B"
 *     "          Status: Complete"
 * 
 * PARAMETERS:
 * 
 *   input_strings:  Array of const char* (your text strings)
 *   num_strings:    Count of strings (max 1000)
 *   justify_char:   Character to align by (typically ':')
 *   max_width:      Maximum line width (0 = unlimited, else truncates)
 * 
 * SAFETY LIMITS:
 * 
 *   - Max strings:      1000 entries
 *   - Max string len:   2048 characters
 *   - Max prefix len:   512 characters (auto-clamped)
 *   - NULL strings:     Handled as empty lines
 *   - Missing delimiter: String copied as-is (no padding)
 * 
 * MEMORY MANAGEMENT:
 * 
 *   - Uses whd_malloc/whd_free for tracking
 *   - Always free with itidy_free_justified_list()
 *   - Safe to pass NULL to free function (no-op)
 * 
 * ERROR HANDLING:
 * 
 *   - Returns NULL on invalid parameters or allocation failure
 *   - Logs warnings/errors for problematic input
 *   - Continues processing on non-critical errors
 * 
 * INTEGRATION EXAMPLE (restore_window.c):
 * 
 *   void update_details_panel(struct iTidyRestoreWindow *restore_data,
 *                             struct RestoreRunEntry *selected_entry)
 *   {
 *       const char *detail_lines[7];
 *       char line_buffer[7][256];
 *       struct List *formatted_list;
 *       
 *       // Detach old list
 *       GT_SetGadgetAttrs(restore_data->details_listview, window, NULL,
 *           GTLV_Labels, ~0, TAG_END);
 *       
 *       // Free previous list
 *       if (restore_data->details_list_strings != NULL) {
 *           itidy_free_justified_list(restore_data->details_list_strings);
 *           restore_data->details_list_strings = NULL;
 *       }
 *       
 *       // Format new strings
 *       sprintf(line_buffer[0], "Run Number: %04u", selected_entry->runNumber);
 *       sprintf(line_buffer[1], "Date Created: %s", selected_entry->dateStr);
 *       ...
 *       
 *       // Build pointer array
 *       for (i = 0; i < 7; i++)
 *           detail_lines[i] = line_buffer[i];
 *       
 *       // Create justified list
 *       formatted_list = itidy_create_justified_list(detail_lines, 7, ':', 0);
 *       if (formatted_list == NULL)
 *           return;
 *       
 *       // Store for cleanup
 *       restore_data->details_list_strings = formatted_list;
 *       
 *       // Attach to ListView
 *       GT_SetGadgetAttrs(restore_data->details_listview, window, NULL,
 *           GTLV_Labels, formatted_list, TAG_END);
 *   }
 * 
 * ============================================================================
 * 
 * Memory management uses whd_malloc/whd_free for tracking.
 */

#include "platform/platform.h"
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>
#include <stdio.h>

#include "list_formatter.h"
#include "exec_list_compat.h"
#include "../writeLog.h"

/**
 * @brief Find the first occurrence of a character in a string
 * 
 * @param str String to search
 * @param ch Character to find
 * @return WORD Position of character (0-based), or -1 if not found
 */
static WORD find_char_position(const char *str, char ch)
{
    WORD pos = 0;
    
    if (str == NULL)
        return -1;
    
    while (str[pos] != '\0')
    {
        if (str[pos] == ch)
            return pos;
        pos++;
    }
    
    return -1;  /* Not found */
}

/**
 * @brief Create a justified list of strings aligned by a delimiter character
 */
struct List *itidy_create_justified_list(const char **input_strings,
                                          UWORD num_strings,
                                          char justify_char,
                                          UWORD max_width)
{
    struct List *list;
    struct Node *node;
    UWORD i;
    WORD max_prefix_len = 0;
    WORD *prefix_positions;
    char *formatted_string;
    UWORD formatted_len;
    UWORD input_len;
    
    /* Validate input parameters */
    if (input_strings == NULL || num_strings == 0)
    {
        log_warning(LOG_GUI, "itidy_create_justified_list: Invalid parameters\n");
        return NULL;
    }
    
    /* Sanity check: prevent excessive allocations */
    if (num_strings > 1000)
    {
        log_error(LOG_GUI, "itidy_create_justified_list: num_strings=%u exceeds limit (1000)\n", 
                  num_strings);
        return NULL;
    }
    
    /* Allocate the List structure */
    list = (struct List *)whd_malloc(sizeof(struct List));
    if (list == NULL)
    {
        log_error(LOG_GUI, "itidy_create_justified_list: Failed to allocate List\n");
        return NULL;
    }
    
    memset(list, 0, sizeof(struct List));
    NewList(list);
    
    /* Allocate array to store prefix positions for each string */
    prefix_positions = (WORD *)whd_malloc(sizeof(WORD) * num_strings);
    if (prefix_positions == NULL)
    {
        log_error(LOG_GUI, "itidy_create_justified_list: Failed to allocate positions array\n");
        whd_free(list);
        return NULL;
    }
    
    /* PASS 1: Find maximum prefix length */
    for (i = 0; i < num_strings; i++)
    {
        if (input_strings[i] == NULL)
        {
            prefix_positions[i] = -1;
            continue;
        }
        
        /* Safety check: validate string length before processing */
        input_len = strlen(input_strings[i]);
        if (input_len > 2048)
        {
            log_warning(LOG_GUI, "itidy_create_justified_list: String %u exceeds 2048 chars, truncating\n", i);
            /* Still process it, but find_char_position will be safe since it checks for '\0' */
        }
        
        prefix_positions[i] = find_char_position(input_strings[i], justify_char);
        
        if (prefix_positions[i] > max_prefix_len)
        {
            max_prefix_len = prefix_positions[i];
        }
    }
    
    /* Sanity check: prevent excessive padding */
    if (max_prefix_len > 512)
    {
        log_warning(LOG_GUI, "itidy_create_justified_list: max_prefix_len=%d exceeds 512, limiting to 512\n",
                    max_prefix_len);
        max_prefix_len = 512;
    }
    
    /* PASS 2: Create formatted strings and nodes */
    for (i = 0; i < num_strings; i++)
    {
        const char *input = input_strings[i];
        WORD prefix_len;
        WORD padding_needed;
        const char *suffix_start;
        
        if (input == NULL)
        {
            /* Create empty node for NULL entries */
            node = (struct Node *)whd_malloc(sizeof(struct Node));
            if (node != NULL)
            {
                memset(node, 0, sizeof(struct Node));
                node->ln_Name = (STRPTR)whd_malloc(1);
                if (node->ln_Name != NULL)
                {
                    node->ln_Name[0] = '\0';
                    AddTail(list, node);
                }
                else
                {
                    whd_free(node);
                }
            }
            continue;
        }
        
        prefix_len = prefix_positions[i];
        
        /* If no delimiter found, copy string as-is */
        if (prefix_len < 0)
        {
            input_len = strlen(input);
            
            /* Safety check: validate string length */
            if (input_len > 2048)
            {
                log_warning(LOG_GUI, "itidy_create_justified_list: String %u too long (%u chars), truncating\n", 
                            i, input_len);
                input_len = 2048;
            }
            
            formatted_len = input_len + 1;
            
            /* Check max_width constraint */
            if (max_width > 0 && formatted_len > max_width + 1)
                formatted_len = max_width + 1;
            
            formatted_string = (char *)whd_malloc(formatted_len);
            if (formatted_string == NULL)
            {
                log_warning(LOG_GUI, "itidy_create_justified_list: Failed to allocate string %u\n", i);
                continue;
            }
            
            strncpy(formatted_string, input, formatted_len - 1);
            formatted_string[formatted_len - 1] = '\0';
        }
        else
        {
            /* Calculate padding needed */
            padding_needed = max_prefix_len - prefix_len;
            
            /* Safety check: ensure padding is reasonable */
            if (padding_needed < 0)
            {
                log_warning(LOG_GUI, "itidy_create_justified_list: Negative padding %d for string %u\n",
                            padding_needed, i);
                padding_needed = 0;
            }
            
            /* Find where suffix starts (character after delimiter) */
            suffix_start = input + prefix_len;
            
            /* Validate input string length */
            input_len = strlen(input);
            if (input_len > 2048)
            {
                log_warning(LOG_GUI, "itidy_create_justified_list: String %u too long (%u chars), truncating\n",
                            i, input_len);
                input_len = 2048;
            }
            
            /* Calculate total formatted string length:
             * padding + prefix + suffix + null terminator
             * Check for potential overflow
             */
            if (padding_needed > 0xFFFF - input_len - 1)
            {
                log_error(LOG_GUI, "itidy_create_justified_list: Overflow calculating length for string %u\n", i);
                continue;  /* Skip this entry */
            }
            
            formatted_len = padding_needed + input_len + 1;
            
            /* Check max_width constraint */
            if (max_width > 0 && formatted_len > max_width + 1)
                formatted_len = max_width + 1;
            
            formatted_string = (char *)whd_malloc(formatted_len);
            if (formatted_string == NULL)
            {
                log_warning(LOG_GUI, "itidy_create_justified_list: Failed to allocate string %u\n", i);
                continue;
            }
            
            /* Build formatted string: [padding][prefix][suffix] */
            memset(formatted_string, ' ', padding_needed);  /* Add padding spaces */
            
            /* Copy prefix and suffix */
            if (max_width > 0)
            {
                /* With max_width, we need to be careful about truncation */
                UWORD remaining = formatted_len - padding_needed - 1;
                strncpy(formatted_string + padding_needed, input, remaining);
                formatted_string[formatted_len - 1] = '\0';
            }
            else
            {
                /* No max_width, copy entire string */
                strcpy(formatted_string + padding_needed, input);
            }
        }
        
        /* Create node and add to list */
        node = (struct Node *)whd_malloc(sizeof(struct Node));
        if (node != NULL)
        {
            memset(node, 0, sizeof(struct Node));
            node->ln_Name = (STRPTR)formatted_string;
            AddTail(list, node);
        }
        else
        {
            whd_free(formatted_string);
            log_warning(LOG_GUI, "itidy_create_justified_list: Failed to allocate node %u\n", i);
        }
    }
    
    /* Cleanup temporary array */
    whd_free(prefix_positions);
    
    return list;
}

/**
 * @brief Free a list created by itidy_create_justified_list()
 */
void itidy_free_justified_list(struct List *list)
{
    struct Node *node;
    
    if (list == NULL)
        return;
    
    /* Free all nodes and their string data */
    while ((node = RemHead(list)) != NULL)
    {
        if (node->ln_Name != NULL)
        {
            whd_free(node->ln_Name);
            node->ln_Name = NULL;
        }
        whd_free(node);
    }
    
    /* Free the list structure itself */
    whd_free(list);
}
