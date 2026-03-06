/**
 * icon_sorter.h - Icon Array Sorting
 *
 * Provides preference-aware sorting for icon arrays.
 * Supports sorting by name, type, date, and size with optional
 * folder/file priority and reverse ordering.
 *
 * Part of the layout subsystem (src/layout/).
 */

#ifndef ICON_SORTER_H
#define ICON_SORTER_H

#include <exec/types.h>
#include "layout_preferences.h"
#include "icon_management.h"

/**
 * @brief Sort icon array according to layout preferences
 *
 * Sorts in-place using qsort with the multi-criteria comparator defined
 * by sortBy, sortPriority, and reverseSort in the preferences.
 *
 * Sort modes (prefs->sortBy):
 *   SORT_BY_NAME  - Alphabetical (case-insensitive)
 *   SORT_BY_TYPE  - By file extension, then by name
 *   SORT_BY_DATE  - Newest first, then by name
 *   SORT_BY_SIZE  - Largest first, then by name
 *
 * Priority modes (prefs->sortPriority):
 *   SORT_PRIORITY_FOLDERS_FIRST - Folders before files
 *   SORT_PRIORITY_FILES_FIRST   - Files before folders
 *   SORT_PRIORITY_MIXED         - No separation
 *
 * @param iconArray Array to sort (modified in place)
 * @param prefs     Layout preferences controlling sort behaviour
 */
void SortIconArrayWithPreferences(IconArray *iconArray,
                                  const LayoutPreferences *prefs);

#endif /* ICON_SORTER_H */
