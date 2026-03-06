/**
 * block_layout.h - Block-Based Icon Grouping Layout
 *
 * Provides CalculateBlockLayout(), which separates icons into three
 * type-based groups (drawers, tools, other) and stacks them vertically
 * with configurable gaps between groups.
 *
 * Part of the layout subsystem (src/layout/).
 */

#ifndef BLOCK_LAYOUT_H
#define BLOCK_LAYOUT_H

#include <exec/types.h>
#include "layout_preferences.h"
#include "icon_management.h"

/**
 * @brief Calculate layout positions for block-based type grouping
 *
 * Groups icons into up to three blocks:
 *   Block 0 - Drawers  (WBDRAWER)
 *   Block 1 - Tools    (WBTOOL)
 *   Block 2 - Other    (WBPROJECT, WBDISK, etc.)
 *
 * Algorithm:
 *   Pass 1 - Calculate optimal column count for each block.
 *   Pass 2 - Re-position all blocks using the widest block's column count
 *            (uniform appearance).
 *   Pass 3 - Stack blocks vertically with gaps; centre narrower blocks
 *            horizontally within the widest block's width.
 *
 * Internally calls CalculateLayoutPositions() or
 * CalculateLayoutPositionsWithColumnCentering() per block, then
 * re-applies vertical and horizontal offsets.
 *
 * @param iconArray  Full icon array (positions updated in place)
 * @param prefs      Layout preferences
 * @param outWidth   Output: total content width
 * @param outHeight  Output: total content height
 * @return TRUE on success, FALSE on memory error
 */
BOOL CalculateBlockLayout(IconArray *iconArray,
                          const LayoutPreferences *prefs,
                          int *outWidth,
                          int *outHeight);

#endif /* BLOCK_LAYOUT_H */
