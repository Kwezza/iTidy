/**
 * icon_positioner.h - Icon Grid Positioning Algorithms
 *
 * Provides the two core positioning algorithms:
 *   - Standard row-based layout with width-based wrapping
 *   - Column-centred layout with per-column width optimisation
 *
 * Both algorithms support aspect-ratio column counts (targetColumns > 0)
 * and fall back to screen-width-based wrapping when targetColumns == 0.
 *
 * Part of the layout subsystem (src/layout/).
 */

#ifndef ICON_POSITIONER_H
#define ICON_POSITIONER_H

#include <exec/types.h>
#include "layout_preferences.h"
#include "icon_management.h"

/**
 * @brief Standard row-based icon positioning
 *
 * Places icons left-to-right in rows, wrapping when:
 *   - targetColumns > 0: after targetColumns icons per row (aspect ratio mode)
 *   - targetColumns == 0: when the next icon exceeds effectiveMaxWidth
 *
 * After each completed row the vertical alignment preference is applied
 * (TEXT_ALIGN_TOP / MIDDLE / BOTTOM) to redistribute icons within the
 * tallest icon's height.
 *
 * @param iconArray     Array of icons to position (modified in place)
 * @param prefs         Layout preferences
 * @param targetColumns Desired columns per row, or 0 for width-based wrapping
 */
void CalculateLayoutPositions(IconArray *iconArray,
                              const LayoutPreferences *prefs,
                              int targetColumns);

/**
 * @brief Column-centred icon positioning
 *
 * Two-pass algorithm:
 *   Pass 1 - Determine per-column widths (uniform or optimised).
 *   Pass 2 - Position each icon centred within its column.
 *
 * Falls back to CalculateLayoutPositions() if column array allocation fails.
 *
 * @param iconArray     Array of icons to position (modified in place)
 * @param prefs         Layout preferences (centerIconsInColumn, useColumnWidthOptimization)
 * @param targetColumns Desired columns, or 0 for width-based estimate
 */
void CalculateLayoutPositionsWithColumnCentering(IconArray *iconArray,
                                                 const LayoutPreferences *prefs,
                                                 int targetColumns);

#endif /* ICON_POSITIONER_H */
