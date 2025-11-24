/**
 * aspect_ratio_layout.h - Aspect Ratio and Window Overflow Algorithms
 * 
 * Provides functions for calculating optimal icon layouts based on
 * target aspect ratios and handling window overflow scenarios.
 * 
 * Part of the Aspect Ratio and Window Overflow feature implementation.
 * See: docs/ASPECT_RATIO_AND_OVERFLOW_FEATURE.md
 */

#ifndef ASPECT_RATIO_LAYOUT_H
#define ASPECT_RATIO_LAYOUT_H

#include <exec/types.h>
#include "layout_preferences.h"
#include "icon_management.h"

/*========================================================================*/
/* Helper Functions                                                      */
/*========================================================================*/

/**
 * @brief Calculate average icon width from icon array
 * 
 * @param iconArray Array of icons to measure
 * @return Average width in pixels (icon_max_width), or 80 if array empty
 */
int CalculateAverageWidth(const IconArray *iconArray);

/**
 * @brief Calculate average icon height from icon array
 * 
 * @param iconArray Array of icons to measure
 * @return Average height in pixels (icon_max_height), or 80 if array empty
 */
int CalculateAverageHeight(const IconArray *iconArray);

/*========================================================================*/
/* Custom Aspect Ratio Validation                                       */
/*========================================================================*/

/**
 * @brief Validate custom aspect ratio input values
 * 
 * Checks that width and height values are valid (positive integers).
 * Warns about extreme ratios but allows them.
 * 
 * @param width Custom ratio numerator (e.g., 16)
 * @param height Custom ratio denominator (e.g., 10)
 * @param outRatio Output parameter: calculated ratio (width*1000/height), e.g., 1777 for 16:9
 * @return TRUE if valid, FALSE if invalid (zero or negative)
 * 
 * @note Extreme ratios (>5000 or <200) generate warnings but are allowed
 * @note Ratio is scaled by 1000 (fixed-point): 1.777 → 1777
 */
BOOL ValidateCustomAspectRatio(int width, int height, int *outRatio);

/*========================================================================*/
/* Optimal Column Calculation                                           */
/*========================================================================*/

/**
 * @brief Calculate optimal icons per row based on aspect ratio
 * 
 * Iterates through possible column counts and finds the one that
 * produces a window shape closest to the target aspect ratio.
 * 
 * Algorithm:
 * - For each possible column count (within min/max constraints)
 * - Calculate resulting rows (ceiling division)
 * - Calculate resulting aspect ratio (width/height)
 * - Track column count with smallest difference from target ratio
 * 
 * @param totalIcons Total number of icons to arrange
 * @param averageIconWidth Average width of icons (including text)
 * @param averageIconHeight Average height of icons (including text)
 * @param targetAspectRatio Desired width/height ratio scaled by 1000 (e.g., 1600 for 1.6)
 * @param minAllowedIconsPerRow Lower limit on columns (prevent 1×N layouts)
 * @param maxAllowedIconsPerRow Upper limit on columns (from maxIconsPerRow pref)
 * @return Optimal number of icons per row
 * 
 * @example
 * // Calculate optimal layout for 20 icons with 1.6 aspect ratio
 * int optimalCols = CalculateOptimalIconsPerRow(
 *     20,      // total icons
 *     80, 80,  // average icon size
 *     1600,    // target ratio (Classic Workbench: 1.6 * 1000)
 *     2,       // minimum 2 columns
 *     10       // maximum 10 columns
 * );
 * // Returns: 6 (produces ~1500 ratio, close to 1600)
 */
int CalculateOptimalIconsPerRow(int totalIcons, 
                                 int averageIconWidth,
                                 int averageIconHeight, 
                                 int targetAspectRatio,
                                 int minAllowedIconsPerRow,
                                 int maxAllowedIconsPerRow);

/*========================================================================*/
/* Main Layout Calculation with Aspect Ratio & Overflow                 */
/*========================================================================*/

/**
 * @brief Calculate layout with aspect ratio and overflow support
 * 
 * This is the main entry point for aspect-ratio-aware layout calculation.
 * It determines the optimal number of columns based on:
 * - Target aspect ratio
 * - Available screen space
 * - Window overflow mode preferences
 * - Min/max column constraints
 * 
 * The function then delegates to the appropriate positioning algorithm
 * (standard or column-centered).
 * 
 * Algorithm Flow:
 * 1. Calculate ideal layout based on aspect ratio
 * 2. Check if ideal layout fits on screen
 * 3. If doesn't fit, apply overflow strategy:
 *    - HORIZONTAL: Maximize rows, expand columns as needed
 *    - VERTICAL: Maximize columns, expand rows as needed
 *    - BOTH: Use ideal ratio even if huge
 * 4. Apply final column/row constraints
 * 5. Call positioning algorithm with calculated column count
 * 
 * @param iconArray Array of icons to layout
 * @param prefs Layout preferences including aspect ratio and overflow mode
 * @param finalColumns Output: calculated number of columns
 * @param finalRows Output: calculated number of rows
 * 
 * @note This function calculates the layout structure but does NOT
 *       actually position icons. Call CalculateLayoutPositions() or
 *       CalculateLayoutPositionsWithColumnCentering() with the returned
 *       column count to apply positions.
 */
void CalculateLayoutWithAspectRatio(const IconArray *iconArray, 
                                    const LayoutPreferences *prefs,
                                    int *finalColumns,
                                    int *finalRows);

#endif /* ASPECT_RATIO_LAYOUT_H */
