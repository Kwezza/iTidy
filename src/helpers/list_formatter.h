/**
 * list_formatter.h - Justified String List Formatter
 * 
 * Helper functions for creating Exec Lists with strings aligned by a delimiter.
 * Useful for displaying formatted detail panels in ListViews.
 * 
 * Example:
 *   Input: ["Name: John", "Age: 25", "Location: New York"]
 *   Output (justified by ':'): 
 *     "    Name: John"
 *     "     Age: 25"
 *     "Location: New York"
 */

#ifndef LIST_FORMATTER_H
#define LIST_FORMATTER_H

#include <exec/types.h>
#include <exec/lists.h>

/**
 * @brief Create a justified list of strings aligned by a delimiter character
 * 
 * Scans input strings for the first occurrence of justify_char, calculates
 * the maximum prefix length, and creates a List of Nodes with formatted strings
 * where all delimiter characters are vertically aligned.
 * 
 * Strings without the delimiter are copied as-is (no formatting).
 * 
 * @param input_strings Array of input strings (NULL-terminated C strings)
 * @param num_strings Number of strings in array
 * @param justify_char Character to align by (typically ':')
 * @param max_width Maximum width for formatted strings (0 = no limit)
 *                  If a formatted string exceeds max_width, it will be truncated
 * @return struct List* Newly allocated List with formatted Nodes, or NULL on error
 *         Caller must free with itidy_free_justified_list()
 */
struct List *itidy_create_justified_list(const char **input_strings,
                                          UWORD num_strings,
                                          char justify_char,
                                          UWORD max_width);

/**
 * @brief Free a list created by itidy_create_justified_list()
 * 
 * Frees all nodes and their string data, then frees the List structure itself.
 * 
 * @param list List to free (can be NULL - will be safely ignored)
 */
void itidy_free_justified_list(struct List *list);

#endif /* LIST_FORMATTER_H */
