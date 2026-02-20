/**
 * block_layout.c - Block-Based Icon Grouping Layout Implementation
 *
 * Extracted from layout_processor.c to keep each module focused.
 * Groups icons by Workbench type and stacks them in visually distinct
 * blocks with configurable vertical gaps.
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <console_output.h>

#include "icon_types.h"
#include "icon_management.h"
#include "layout_preferences.h"
#include "writeLog.h"

#include "layout/block_layout.h"
#include "layout/icon_positioner.h"
#include "layout/icon_sorter.h"
#include "layout/aspect_ratio_layout.h"

/*========================================================================*/
/* Block Layout Helpers                                                  */
/*========================================================================*/

/**
 * @brief Get pixel value for block gap size
 *
 * Converts BlockGapSize enum to actual pixel spacing.
 *
 * @param gapSize Gap size enum (SMALL, MEDIUM, LARGE)
 * @return Pixel spacing value
 */
static int GetBlockGapPixels(BlockGapSize gapSize)
{
    switch (gapSize)
    {
        case BLOCK_GAP_SMALL:
            return BLOCK_GAP_SMALL_PX;
        case BLOCK_GAP_LARGE:
            return BLOCK_GAP_LARGE_PX;
        case BLOCK_GAP_MEDIUM:
        default:
            return BLOCK_GAP_MEDIUM_PX;
    }
}

/**
 * @brief Partition icons into three groups by Workbench type
 *
 * Separates icons into drawers (WBDRAWER), tools (WBTOOL), and other types
 * (WBPROJECT, WBDISK, etc.). Returns index arrays rather than copying icons
 * to avoid pointer corruption.
 *
 * @param source        Input array with all icons
 * @param drawerIndices Output: allocated array of drawer icon indices (caller must free)
 * @param drawerCount   Output: number of drawer icons
 * @param toolIndices   Output: allocated array of tool icon indices (caller must free)
 * @param toolCount     Output: number of tool icons
 * @param otherIndices  Output: allocated array of other icon indices (caller must free)
 * @param otherCount    Output: number of other icons
 */
static void PartitionIconsByWorkbenchType(const IconArray *source,
                                          int **drawerIndices, int *drawerCount,
                                          int **toolIndices,   int *toolCount,
                                          int **otherIndices,  int *otherCount)
{
    int i;
    int *drawers = NULL;
    int *tools   = NULL;
    int *other   = NULL;
    int dCount = 0, tCount = 0, oCount = 0;

    if (!source || !source->array || source->size == 0)
    {
        log_warning(LOG_ICONS, "PartitionIconsByWorkbenchType: Invalid or empty source array\n");
        *drawerIndices = NULL; *drawerCount = 0;
        *toolIndices   = NULL; *toolCount   = 0;
        *otherIndices  = NULL; *otherCount  = 0;
        return;
    }

    /* Allocate index arrays (worst case: all icons in one category) */
    drawers = (int *)whd_malloc(source->size * sizeof(int));
    tools   = (int *)whd_malloc(source->size * sizeof(int));
    other   = (int *)whd_malloc(source->size * sizeof(int));

    if (!drawers || !tools || !other)
    {
        log_error(LOG_ICONS, "PartitionIconsByWorkbenchType: Failed to allocate index arrays\n");
        if (drawers) whd_free(drawers);
        if (tools)   whd_free(tools);
        if (other)   whd_free(other);
        *drawerIndices = NULL; *drawerCount = 0;
        *toolIndices   = NULL; *toolCount   = 0;
        *otherIndices  = NULL; *otherCount  = 0;
        return;
    }

    /* Classify icons by Workbench type */
    for (i = 0; i < source->size; i++)
    {
        UBYTE wb_type = source->array[i].workbench_type;

        if (wb_type == WBDRAWER)
        {
            drawers[dCount++] = i;
        }
        else if (wb_type == WBTOOL)
        {
            tools[tCount++] = i;
        }
        else
        {
            /* Everything else: WBPROJECT, WBDISK, WBGARBAGE, WBDEVICE, etc. */
            other[oCount++] = i;
        }
    }

    log_debug(LOG_ICONS, "Partitioned %d icons by Workbench type\n", source->size);
    log_debug(LOG_ICONS, "  Drawers: %d, Tools: %d, Other: %d\n", dCount, tCount, oCount);

    *drawerIndices = drawers; *drawerCount = dCount;
    *toolIndices   = tools;   *toolCount   = tCount;
    *otherIndices  = other;   *otherCount  = oCount;
}

/*========================================================================*/
/* Block Layout                                                          */
/*========================================================================*/

/**
 * @brief Calculate layout positions for block-based grouping
 *
 * ARCHITECTURE: Reuses existing positioning functions (CalculateLayoutPositions or
 * CalculateLayoutPositionsWithColumnCentering) by creating temporary IconArrays
 * for each block. This avoids code duplication and ensures consistent behavior.
 *
 * Each block (drawers, tools, other) gets its own optimal column count and is
 * positioned independently. The widest block determines the window width, and
 * narrower blocks are horizontally centered. Blocks are stacked vertically with
 * configurable gaps between them.
 *
 * @param iconArray   The full icon array (positions will be updated in place)
 * @param prefs       Layout preferences
 * @param outWidth    Output: calculated window content width
 * @param outHeight   Output: calculated window content height
 * @return TRUE on success, FALSE on error
 */
BOOL CalculateBlockLayout(IconArray *iconArray,
                          const LayoutPreferences *prefs,
                          int *outWidth,
                          int *outHeight)
{
    int *drawerIndices = NULL, *toolIndices = NULL, *otherIndices = NULL;
    int drawerCount = 0, toolCount = 0, otherCount = 0;
    IconArray *blockArrays[3] = {NULL, NULL, NULL};
    int *blockIndices[3];
    int blockCounts[3];
    int blockWidths[3]  = {0, 0, 0};
    int blockHeights[3] = {0, 0, 0};
    int maxBlockWidth = 0;
    int totalHeight = prefs->iconSpacingY;  /* Start with top margin */
    int gapPixels;
    int blockIndex;
    BOOL success = FALSE;

    log_debug(LOG_ICONS, "CalculateBlockLayout: Processing %d icons with block grouping\n",
              iconArray->size);

    /* Partition icons into three groups by type */
    PartitionIconsByWorkbenchType(iconArray,
                                  &drawerIndices, &drawerCount,
                                  &toolIndices,   &toolCount,
                                  &otherIndices,  &otherCount);

    if (!drawerIndices || !toolIndices || !otherIndices)
    {
        log_error(LOG_ICONS, "CalculateBlockLayout: Partition failed\n");
        goto cleanup;
    }

    /* Setup block processing arrays */
    blockIndices[0] = drawerIndices; blockCounts[0] = drawerCount;
    blockIndices[1] = toolIndices;   blockCounts[1] = toolCount;
    blockIndices[2] = otherIndices;  blockCounts[2] = otherCount;

    gapPixels = GetBlockGapPixels(prefs->blockGapSize);
    log_debug(LOG_ICONS, "Block gap size: %d pixels\n", gapPixels);

    /* ================================================================
     * PASS 1: Calculate optimal columns for each block
     * ================================================================ */
    log_debug(LOG_ICONS, "PASS 1: Calculating optimal columns for each block\n");

    {
        int blockOptimalColumns[3] = {0, 0, 0};
        int maxColumns = 0;
        int maxColumnsBlockIndex = -1;

        for (blockIndex = 0; blockIndex < 3; blockIndex++)
        {
            int count = blockCounts[blockIndex];
            IconArray *blockArray;
            int j;

            if (count == 0)
            {
                log_debug(LOG_ICONS, "Block %d: empty, skipping\n", blockIndex);
                continue;
            }

            /* Create temporary IconArray for this block */
            blockArray = (IconArray *)whd_malloc(sizeof(IconArray));
            if (!blockArray)
            {
                log_error(LOG_ICONS, "Failed to allocate blockArray for block %d\n", blockIndex);
                goto cleanup;
            }

            blockArray->array = (FullIconDetails *)whd_malloc(count * sizeof(FullIconDetails));
            if (!blockArray->array)
            {
                log_error(LOG_ICONS, "Failed to allocate block icon array for block %d\n", blockIndex);
                whd_free(blockArray);
                goto cleanup;
            }

            blockArray->size     = count;
            blockArray->capacity = count;
            blockArray->BiggestWidthPX = 0;
            blockArray->hasOnlyBorderlessIcons = iconArray->hasOnlyBorderlessIcons;

            /* Copy icons from main array to block array */
            for (j = 0; j < count; j++)
            {
                blockArray->array[j] = iconArray->array[blockIndices[blockIndex][j]];
            }

            blockArrays[blockIndex] = blockArray;

            /* Sort block */
            SortIconArrayWithPreferences(blockArray, prefs);

            /* Calculate optimal columns for this block */
            {
                int finalColumns = 0, finalRows = 0;
                CalculateLayoutWithAspectRatio(blockArray, prefs, &finalColumns, &finalRows);
                blockOptimalColumns[blockIndex] = finalColumns;

                log_debug(LOG_ICONS, "Block %d: %d icons, optimal columns=%d\n",
                          blockIndex, count, finalColumns);

                /* Track widest block */
                if (finalColumns > maxColumns)
                {
                    maxColumns = finalColumns;
                    maxColumnsBlockIndex = blockIndex;
                }
            }
        }

        log_debug(LOG_ICONS, "Widest block: Block %d with %d columns\n",
                  maxColumnsBlockIndex, maxColumns);

        /* ================================================================
         * PASS 2: Re-position all blocks using the widest block's columns
         * ================================================================ */
        log_debug(LOG_ICONS, "PASS 2: Positioning all blocks with %d columns\n", maxColumns);

        for (blockIndex = 0; blockIndex < 3; blockIndex++)
        {
            int *indices   = blockIndices[blockIndex];
            int count      = blockCounts[blockIndex];
            IconArray *blockArray = blockArrays[blockIndex];
            int minX, maxX, minY, maxY;
            int j;

            if (count == 0 || !blockArray)
            {
                continue;
            }

            log_debug(LOG_ICONS, "Repositioning block %d with %d columns\n", blockIndex, maxColumns);

            /* Position icons using uniform column count */
            if (prefs->centerIconsInColumn)
            {
                CalculateLayoutPositionsWithColumnCentering(blockArray, prefs, maxColumns);
            }
            else
            {
                CalculateLayoutPositions(blockArray, prefs, maxColumns);
            }

            /* Calculate block dimensions from positioned icons */
            minX = minY = 10000;
            maxX = maxY = 0;

            log_debug(LOG_ICONS, "Block %d icon positions (count=%d):\n", blockIndex, count);

            for (j = 0; j < count; j++)
            {
                FullIconDetails *icon = &blockArray->array[j];
                int iconRight;
                int iconBottom;

                /* Calculate effective right edge based on what's wider.
                 * FIX: When text is wider than icon, it is centered on the icon.
                 * The simple calculation (icon_x + max_width) assumes left-alignment
                 * of the wider element, creating "phantom padding" on the right.
                 *
                 * Correct calculation for centered text:
                 *   Icon Center = icon_x + (icon_width / 2)
                 *   Text Right  = Icon Center + (text_width / 2)
                 *               = icon_x + (icon_width + text_width) / 2
                 */
                if (icon->text_width > icon->icon_width)
                {
                    iconRight = icon->icon_x + (icon->icon_width + icon->text_width) / 2;
                }
                else
                {
                    iconRight = icon->icon_x + icon->icon_max_width;
                }

                iconBottom = icon->icon_y + icon->icon_max_height;

                log_debug(LOG_ICONS,
                          "  [%d] %-20s: X=%3d, iconW=%3d, textW=%3d, maxW=%3d, rightEdge=%3d\n",
                          j, icon->icon_text, icon->icon_x, icon->icon_width,
                          icon->text_width, icon->icon_max_width, iconRight);

                if (icon->icon_x  < minX) minX = icon->icon_x;
                if (icon->icon_y  < minY) minY = icon->icon_y;
                if (iconRight     > maxX) maxX = iconRight;
                if (iconBottom    > maxY) maxY = iconBottom;
            }

            blockWidths[blockIndex]  = maxX;
            blockHeights[blockIndex] = (maxY - minY) + prefs->iconSpacingY;

            log_debug(LOG_ICONS, "Block %d final dimensions: %dx%d (minX=%d, maxX=%d)\n",
                      blockIndex, blockWidths[blockIndex], blockHeights[blockIndex], minX, maxX);

            /* Track widest block */
            if (blockWidths[blockIndex] > maxBlockWidth)
            {
                maxBlockWidth = blockWidths[blockIndex];
            }
        }
    }

    log_debug(LOG_ICONS, "All blocks positioned with uniform width: %d pixels\n", maxBlockWidth);

    /* Debug: Show block width comparison */
    log_debug(LOG_ICONS, "\n=== Block Width Analysis ===\n");
    for (blockIndex = 0; blockIndex < 3; blockIndex++)
    {
        if (blockCounts[blockIndex] > 0)
        {
            int wastedSpace = maxBlockWidth - blockWidths[blockIndex];
            int efficiencyPercent = (blockWidths[blockIndex] * 100) / maxBlockWidth;

            log_debug(LOG_ICONS,
                      "Block %d: %3dpx wide (%d%% of target %dpx) -> %dpx wasted space\n",
                      blockIndex, blockWidths[blockIndex], efficiencyPercent,
                      maxBlockWidth, wastedSpace);
        }
    }
    log_debug(LOG_ICONS, "Target window content width: %dpx\n", maxBlockWidth);
    log_debug(LOG_ICONS, "============================\n\n");

    /* ================================================================
     * PASS 3: Stack blocks vertically AND center horizontally
     * ================================================================ */
    totalHeight = prefs->iconSpacingY;  /* Start with top margin */

    for (blockIndex = 0; blockIndex < 3; blockIndex++)
    {
        IconArray *blockArray = blockArrays[blockIndex];
        int *indices = blockIndices[blockIndex];
        int count    = blockCounts[blockIndex];
        int j;

        if (count == 0 || !blockArray)
        {
            continue;
        }

        /* Calculate horizontal centering offset */
        int centerOffset = (maxBlockWidth - blockWidths[blockIndex]) / 2;

        log_debug(LOG_ICONS,
                  "Stacking block %d at Y=%d, CenterOffset=%d (Width: %d/%d)\n",
                  blockIndex, totalHeight, centerOffset,
                  blockWidths[blockIndex], maxBlockWidth);

        /* Apply offsets to all icons in block */
        for (j = 0; j < count; j++)
        {
            FullIconDetails *icon = &blockArray->array[j];

            /* Apply vertical stacking.
             * icon->icon_y already includes prefs->iconSpacingY from
             * CalculateLayoutPositions, so subtract it to get relative Y,
             * then add totalHeight (current stack position). */
            icon->icon_y += totalHeight - prefs->iconSpacingY;

            /* Apply horizontal centering */
            icon->icon_x += centerOffset;

            /* Copy transformed icon back to main array at original index */
            iconArray->array[indices[j]] = *icon;
        }

        /* Move Y down for next block */
        totalHeight += blockHeights[blockIndex];

        /* Add gap after this block (if more non-empty blocks follow) */
        {
            int k;
            BOOL moreBlocks = FALSE;

            for (k = blockIndex + 1; k < 3; k++)
            {
                if (blockCounts[k] > 0)
                {
                    moreBlocks = TRUE;
                    break;
                }
            }

            if (moreBlocks)
            {
                log_debug(LOG_ICONS, "Adding %dpx gap after block %d\n", gapPixels, blockIndex);
                totalHeight += gapPixels;
            }
        }
    }

    /* Return final dimensions (margins already included by CalculateLayoutPositions) */
    *outWidth  = maxBlockWidth;
    *outHeight = totalHeight;

    log_info(LOG_ICONS, "Block layout complete: %dx%d content area\n", *outWidth, *outHeight);

    success = TRUE;

cleanup:
    /* Free temporary block arrays */
    for (blockIndex = 0; blockIndex < 3; blockIndex++)
    {
        if (blockArrays[blockIndex])
        {
            if (blockArrays[blockIndex]->array)
            {
                whd_free(blockArrays[blockIndex]->array);
            }
            whd_free(blockArrays[blockIndex]);
        }
    }

    /* Free index arrays */
    if (drawerIndices) whd_free(drawerIndices);
    if (toolIndices)   whd_free(toolIndices);
    if (otherIndices)  whd_free(otherIndices);

    return success;
}
