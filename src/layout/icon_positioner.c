/**
 * icon_positioner.c - Icon Grid Positioning Implementation
 *
 * Extracted from layout_processor.c to keep each module focused.
 * Contains the row-based and column-centred positioning algorithms,
 * plus the shared vertical-alignment helper.
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <console_output.h>

#include "itidy_types.h"
#include "icon_types.h"
#include "icon_management.h"
#include "layout_preferences.h"
#include "window_management.h"
#include "writeLog.h"

#include "layout/icon_positioner.h"

/*========================================================================*/
/* Row Vertical Alignment Helper                                         */
/*========================================================================*/

/**
 * @brief Apply vertical alignment offsets to all icons in a row
 *
 * Adjusts each icon's Y position so icons shorter than maxRowHeight are
 * shifted down according to the textAlignment preference (top/middle/bottom).
 *
 * @param iconArray    The full icon array
 * @param rowStart     Index of first icon in the row (inclusive)
 * @param rowEnd       Index past the last icon in the row (exclusive)
 * @param maxRowHeight Height of the tallest icon in the row
 * @param prefs        Layout preferences (textAlignment field)
 */
static void apply_row_vertical_alignment(IconArray *iconArray,
                                         int rowStart, int rowEnd,
                                         int maxRowHeight,
                                         const LayoutPreferences *prefs)
{
    int j;

    for (j = rowStart; j < rowEnd; j++)
    {
        FullIconDetails *rowIcon = &iconArray->array[j];
        int adjustmentOffset = 0;

        switch (prefs->textAlignment)
        {
            case TEXT_ALIGN_TOP:
                /* No adjustment needed - icons already at top */
                adjustmentOffset = 0;
                break;

            case TEXT_ALIGN_MIDDLE:
                /* Center icon vertically within row height */
                adjustmentOffset = (maxRowHeight - rowIcon->icon_max_height) / 2;
                break;

            case TEXT_ALIGN_BOTTOM:
                /* Align to bottom of row */
                adjustmentOffset = maxRowHeight - rowIcon->icon_max_height;
                break;

            default:
                adjustmentOffset = 0;
                break;
        }

        if (adjustmentOffset > 0)
        {
            rowIcon->icon_y += adjustmentOffset;
        }
    }
}

/*========================================================================*/
/* Calculate Icon Positions Based on Layout Preferences                  */
/*========================================================================*/

void CalculateLayoutPositions(IconArray *iconArray,
                              const LayoutPreferences *prefs,
                              int targetColumns)
{
    int i;
    int iconSpacingX;   /* Horizontal spacing between icons */
    int iconSpacingY;   /* Vertical spacing between icons */
    int currentX;       /* Current X position */
    int currentY;       /* Current Y position */
    int maxWidth = 640; /* Default screen width */
    int effectiveMaxWidth;
    int rightMargin;    /* Safety margin on right edge */
    int nextX;
    int maxHeightInRow = 0; /* Track tallest icon in current row */
    int rowStartY;      /* Y position where current row started */
    int rowStartIndex = 0; /* Index of first icon in current row */
    int iconsInCurrentRow = 0; /* Track icons placed in current row */

    /* Use spacing from preferences */
    iconSpacingX = prefs->iconSpacingX;
    iconSpacingY = prefs->iconSpacingY;

    /* Starting positions use spacing as margin */
    currentX = iconSpacingX;
    currentY = iconSpacingY;
    rowStartY = iconSpacingY;
    rightMargin = iconSpacingX;

    if (!iconArray || iconArray->size == 0)
        return;

    /* Use actual screen width if available */
    if (screenWidth > 0)
        maxWidth = screenWidth;

    /* Apply maxWindowWidthPct to limit the usable width */
    if (prefs->maxWindowWidthPct > 0 && prefs->maxWindowWidthPct < 100)
    {
        effectiveMaxWidth = (maxWidth * prefs->maxWindowWidthPct) / 100;
    }
    else
    {
        effectiveMaxWidth = maxWidth;
    }

#ifdef DEBUG
    append_to_log("CalculateLayoutPositions: Positioning %d icons (targetColumns=%d)\n",
                  iconArray->size, targetColumns);
    append_to_log("  screenWidth=%d, maxWindowWidthPct=%d, effectiveMaxWidth=%d\n",
                  maxWidth, prefs->maxWindowWidthPct, effectiveMaxWidth);
    append_to_log("  textAlignment=%s\n",
                  prefs->textAlignment == TEXT_ALIGN_TOP ? "TOP" :
                  prefs->textAlignment == TEXT_ALIGN_MIDDLE ? "MIDDLE" : "BOTTOM");
    append_to_log("%-3s | %-30s | %-4s | %-4s | %-5s | %-5s | %-6s\n",
                  "ID", "Icon", "NewX", "NewY", "Width", "Height", "Offset");
    append_to_log("------------------------------------------------------------------------------------\n");
#endif

    /* FIRST PASS: Position icons in rows, tracking row boundaries */
    for (i = 0; i < iconArray->size; i++)
    {
        FullIconDetails *icon = &iconArray->array[i];
        int iconOffset = 0;
        BOOL shouldWrap = FALSE;

        /* Calculate where the NEXT icon would start */
        nextX = currentX + icon->icon_max_width + iconSpacingX;

        /* Check if we should wrap based on target columns or width constraint */
        if (targetColumns > 0)
        {
            /* Aspect ratio mode: wrap based on column count */
            shouldWrap = (iconsInCurrentRow >= targetColumns);
        }
        else
        {
            /* Traditional mode: wrap based on width */
            shouldWrap = (nextX > (effectiveMaxWidth - rightMargin));
        }

        /* If should wrap to next row (but always place at least one icon per row) */
        if (currentX > iconSpacingX && shouldWrap)
        {
            /* Before starting new row, apply vertical alignment to previous row */
            if (maxHeightInRow > 0)
            {
                apply_row_vertical_alignment(iconArray, rowStartIndex, i, maxHeightInRow, prefs);
            }

            /* Start new row - use the tallest icon from previous row */
            currentX = iconSpacingX;
            currentY = rowStartY + maxHeightInRow + iconSpacingY;
            rowStartY = currentY;
            rowStartIndex = i; /* Mark start of new row */
            maxHeightInRow = 0; /* Reset for new row */
            iconsInCurrentRow = 0; /* Reset column counter for new row */
        }

        /* If text is wider than icon, center the icon within the text width */
        if (icon->text_width > icon->icon_width)
        {
            iconOffset = (icon->text_width - icon->icon_width) / 2;
        }

        /* Set position for this icon (with centering offset if needed) */
        icon->icon_x = currentX + iconOffset;
        icon->icon_y = currentY;

        /* Track the tallest icon in this row */
        if (icon->icon_max_height > maxHeightInRow)
            maxHeightInRow = icon->icon_max_height;

#ifdef DEBUG
        append_to_log("%-3d | %-30s | %-4d | %-4d | %-5d | %-5d | %-3d\n",
                      i, icon->icon_text, icon->icon_x, icon->icon_y,
                      icon->icon_max_width, icon->icon_max_height, iconOffset);
#endif

        /* Advance X position for next icon (using max width, not offset position) */
        currentX += icon->icon_max_width + iconSpacingX;
        iconsInCurrentRow++; /* Count icons in current row */
    }

    /* SECOND PASS: Adjust the last row with vertical alignment */
    /* (This handles the final row that didn't trigger the wrap condition) */
    if (maxHeightInRow > 0)
    {
        apply_row_vertical_alignment(iconArray, rowStartIndex, iconArray->size, maxHeightInRow, prefs);
    }

#ifdef DEBUG
    append_to_log("\nFinal icon positions:\n");
    append_to_log("%-3s | %-30s | %-4s | %-4s\n", "ID", "Icon", "X", "Y");
    append_to_log("--------------------------------------------\n");
    for (i = 0; i < iconArray->size; i++)
    {
        append_to_log("%-3d | %-30s | %-4d | %-4d\n",
                      i, iconArray->array[i].icon_text,
                      iconArray->array[i].icon_x, iconArray->array[i].icon_y);
    }
#endif
}

/*========================================================================*/
/* Calculate Icon Positions With Column Centering                        */
/*========================================================================*/

/**
 * @brief Position icons with column-based centering
 *
 * This function implements a two-pass algorithm for centering icons within
 * columns where each column's width is determined by its widest icon.
 *
 * Algorithm Overview:
 * Phase 1: Statistics & Column Width Calculation
 *   - Calculate average icon width for initial column estimate
 *   - Determine how many columns can fit in window
 *   - Assign icons to columns and calculate actual max width per column
 *   - Validate that total width fits in window (adjust if needed)
 *
 * Phase 2: Icon Positioning
 *   - Position each icon within its column
 *   - Center narrower icons within their column width
 *   - Apply text alignment adjustments
 *
 * Performance: Maximum 2 calculation passes, efficient for 1000+ icons
 *
 * @param iconArray Array of icons to position
 * @param prefs Layout preferences including centerIconsInColumn flag
 */
void CalculateLayoutPositionsWithColumnCentering(IconArray *iconArray,
                                                 const LayoutPreferences *prefs,
                                                 int targetColumns)
{
    int i, col, row;
    int iconSpacingX;   /* Horizontal spacing between icons */
    int iconSpacingY;   /* Vertical spacing between icons */
    int maxWidth = 640;
    int effectiveMaxWidth;
    int rightMargin;
    int startX;
    int startY;

    /* Use spacing from preferences */
    iconSpacingX = prefs->iconSpacingX;
    iconSpacingY = prefs->iconSpacingY;

    /* Starting positions use defined margins (ICON_START_X/Y) for proper window padding */
    startX = ICON_START_X;
    startY = ICON_START_Y;
    rightMargin = iconSpacingX;

    /* Phase 1 variables: Statistics and column calculation */
    int totalWidth = 0;
    int averageWidth = 0;
    int estimatedColumns = 0;
    int finalColumns = targetColumns; /* Use aspect ratio calculated columns */
    int *columnWidths = NULL;
    int *columnXPositions = NULL;
    int calculationAttempts = 0;

    /* Phase 2 variables: Positioning */
    int maxHeightInRow = 0;
    int rowStartIndex = 0;
    int rowStartY = startY;
    int iconX, iconY;
    int centerOffset;

    if (!iconArray || iconArray->size == 0)
        return;

    /* Use actual screen width if available */
    if (screenWidth > 0)
        maxWidth = screenWidth;

    /* Apply maxWindowWidthPct to limit the usable width */
    if (prefs->maxWindowWidthPct > 0 && prefs->maxWindowWidthPct < 100)
    {
        effectiveMaxWidth = (maxWidth * prefs->maxWindowWidthPct) / 100;
    }
    else
    {
        effectiveMaxWidth = maxWidth;
    }

#ifdef DEBUG
    append_to_log("CalculateLayoutPositionsWithColumnCentering: Processing %d icons\n",
                  iconArray->size);
    append_to_log("  screenWidth=%d, maxWindowWidthPct=%d, effectiveMaxWidth=%d\n",
                  maxWidth, prefs->maxWindowWidthPct, effectiveMaxWidth);
#endif

    /*====================================================================*/
    /* PHASE 1: STATISTICS & COLUMN WIDTH CALCULATION                    */
    /*====================================================================*/

    /* Step 1: Calculate average icon width for initial estimate */
    totalWidth = 0;
    for (i = 0; i < iconArray->size; i++)
    {
        totalWidth += iconArray->array[i].icon_max_width;
    }
    averageWidth = totalWidth / iconArray->size;

#ifdef DEBUG
    append_to_log("Phase 1: Statistics\n");
    append_to_log("  Average icon width: %d pixels\n", averageWidth);
    append_to_log("  Total icons: %d\n", iconArray->size);
#endif

    /* Step 2: Estimate initial column count (or use target from aspect ratio) */
    if (targetColumns > 0)
    {
        /* Use aspect ratio calculated columns */
        estimatedColumns = targetColumns;
        finalColumns = targetColumns;
#ifdef DEBUG
        append_to_log("  Using aspect ratio target columns: %d\n", targetColumns);
#endif
    }
    else
    {
        /* Calculate columns based on width */
        estimatedColumns = (effectiveMaxWidth - startX - rightMargin) / (averageWidth + iconSpacingX);
        if (estimatedColumns < 1)
            estimatedColumns = 1;
        finalColumns = estimatedColumns;
#ifdef DEBUG
        append_to_log("  Initial estimated columns: %d\n", estimatedColumns);
#endif
    }

    /* Allocate arrays for column widths and positions */
    columnWidths = (int *)malloc(estimatedColumns * sizeof(int));
    columnXPositions = (int *)malloc(estimatedColumns * sizeof(int));

    if (!columnWidths || !columnXPositions)
    {
        /* Fallback: use standard layout if allocation fails */
#ifdef DEBUG
        append_to_log("ERROR: Failed to allocate column arrays, falling back to standard layout\n");
#endif
        if (columnWidths) free(columnWidths);
        if (columnXPositions) free(columnXPositions);
        CalculateLayoutPositions(iconArray, prefs, targetColumns);
        return;
    }

    /* Step 3 & 4: Calculate actual column widths (with potential adjustment) */
    /* Skip adjustment loop if using targetColumns */

    do {
        calculationAttempts++;

        /* Clear column width array */
        for (col = 0; col < finalColumns; col++)
        {
            columnWidths[col] = 0;
        }

        /* Calculate column widths based on optimization setting */
        if (prefs->useColumnWidthOptimization)
        {
            /* OPTIMIZED: Calculate maximum width for each column individually */
            for (i = 0; i < iconArray->size; i++)
            {
                col = i % finalColumns;
                if (iconArray->array[i].icon_max_width > columnWidths[col])
                {
                    columnWidths[col] = iconArray->array[i].icon_max_width;
                }
            }

#ifdef DEBUG
            append_to_log("Using optimized column widths (per-column max)\n");
#endif
        }
        else
        {
            /* UNIFORM: Use the same width for all columns (widest icon overall) */
            int uniformWidth = 0;

            /* Find widest icon across all icons */
            for (i = 0; i < iconArray->size; i++)
            {
                if (iconArray->array[i].icon_max_width > uniformWidth)
                {
                    uniformWidth = iconArray->array[i].icon_max_width;
                }
            }

            /* Apply uniform width to all columns */
            for (col = 0; col < finalColumns; col++)
            {
                columnWidths[col] = uniformWidth;
            }

#ifdef DEBUG
            append_to_log("Using uniform column width: %d pixels (widest icon)\n", uniformWidth);
#endif
        }

        /* Step 5: Validate total width */
        totalWidth = startX;
        for (col = 0; col < finalColumns; col++)
        {
            totalWidth += columnWidths[col];
            if (col < finalColumns - 1)
                totalWidth += iconSpacingX;
        }
        totalWidth += rightMargin;

#ifdef DEBUG
        append_to_log("Attempt %d: %d columns, total width = %d (max = %d)\n",
                      calculationAttempts, finalColumns, totalWidth, effectiveMaxWidth);
#endif

        /* If using targetColumns, accept width even if it exceeds screen */
        if (targetColumns > 0)
        {
#ifdef DEBUG
            append_to_log("  Using aspect ratio target, accepting width\n");
#endif
            break;
        }

        /* If doesn't fit and we have more than 1 column, reduce and recalculate */
        if (totalWidth > effectiveMaxWidth && finalColumns > 1)
        {
            finalColumns--;
#ifdef DEBUG
            append_to_log("  Width exceeded, reducing to %d columns\n", finalColumns);
#endif
        }
        else
        {
            /* Fits! Break out of loop */
            break;
        }

    } while (calculationAttempts < 2); /* Maximum 2 attempts */

#ifdef DEBUG
    append_to_log("Final configuration: %d columns, total width = %d\n",
                  finalColumns, totalWidth);
    append_to_log("Column widths: ");
    for (col = 0; col < finalColumns; col++)
    {
        append_to_log("%d ", columnWidths[col]);
    }
    append_to_log("\n");
#endif

    /* Calculate X position for start of each column */
    columnXPositions[0] = startX;
    for (col = 1; col < finalColumns; col++)
    {
        columnXPositions[col] = columnXPositions[col - 1] + columnWidths[col - 1] + iconSpacingX;
    }

    /*====================================================================*/
    /* PHASE 2: ICON POSITIONING WITH CENTERING                          */
    /*====================================================================*/

#ifdef DEBUG
    append_to_log("\nPhase 2: Icon Positioning\n");
    append_to_log("%-3s | %-30s | Col | Row | %-4s | %-4s | Center\n",
                  "ID", "Icon", "X", "Y");
    append_to_log("-------------------------------------------------------------------------\n");
#endif

    /* Position each icon within its column */
    for (i = 0; i < iconArray->size; i++)
    {
        FullIconDetails *icon = &iconArray->array[i];

        /* Determine column and row */
        col = i % finalColumns;
        row = i / finalColumns;

        /* Start a new row? Track row boundaries for text alignment */
        if (col == 0 && i > 0)
        {
            /* Apply vertical alignment to previous row */
            if (maxHeightInRow > 0)
            {
                apply_row_vertical_alignment(iconArray, rowStartIndex, i, maxHeightInRow, prefs);
            }

            /* Move to next row */
            rowStartY += maxHeightInRow + iconSpacingY;
            rowStartIndex = i;
            maxHeightInRow = 0;
        }

        /* Calculate Y position */
        iconY = rowStartY;

        /* Calculate X position with centering within column */
        centerOffset = (columnWidths[col] - icon->icon_max_width) / 2;
        if (centerOffset < 0)
            centerOffset = 0;

        iconX = columnXPositions[col] + centerOffset;

        /* Additional centering if text is wider than icon */
        if (icon->text_width > icon->icon_width)
        {
            int textCenterOffset = (icon->text_width - icon->icon_width) / 2;
            iconX += textCenterOffset;
        }

        /* Set icon position */
        icon->icon_x = iconX;
        icon->icon_y = iconY;

        /* Track tallest icon in current row */
        if (icon->icon_max_height > maxHeightInRow)
            maxHeightInRow = icon->icon_max_height;

#ifdef DEBUG
        append_to_log("%-3d | %-30s | %-3d | %-3d | %-4d | %-4d | %-6d\n",
                      i, icon->icon_text, col, row,
                      icon->icon_x, icon->icon_y, centerOffset);
#endif
    }

    /* Apply vertical alignment to final row */
    if (maxHeightInRow > 0)
    {
        apply_row_vertical_alignment(iconArray, rowStartIndex, iconArray->size, maxHeightInRow, prefs);
    }

#ifdef DEBUG
    append_to_log("\nColumn-centered layout complete!\n");
#endif

    /* Cleanup */
    free(columnWidths);
    free(columnXPositions);
}
