/**
 * layout_processor.c - Icon Processing with Layout Preferences
 * 
 * Implements preference-aware directory processing that integrates
 * with the existing iTidy icon management system.
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <proto/wb.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "layout_processor.h"
#include "layout_preferences.h"
#include "file_directory_handling.h"
#include "icon_management.h"
#include "window_management.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "spinner.h"
#include "aspect_ratio_layout.h"

/* Backup system integration */
#include "backup_session.h"
#include "backup_catalog.h"
#include "window_enumerator.h"

/* Global backup context (initialized by ProcessDirectoryWithPreferences) */
static BackupContext *g_backupContext = NULL;

/* Forward declarations for helper functions */
static int CompareIconsWithPreferences(const void *a, const void *b, 
                                      const LayoutPreferences *prefs);
static void SortIconArrayWithPreferences(IconArray *iconArray, 
                                        const LayoutPreferences *prefs);
static BOOL ProcessSingleDirectory(const char *path, 
                                   const LayoutPreferences *prefs,
                                   FolderWindowTracker *windowTracker);
static BOOL ProcessDirectoryRecursive(const char *path, 
                                     const LayoutPreferences *prefs, 
                                     int recursion_level,
                                     FolderWindowTracker *windowTracker);

/* Forward declarations for column centering */
static void CalculateLayoutPositions(IconArray *iconArray, 
                                    const LayoutPreferences *prefs,
                                    int targetColumns);
static void CalculateLayoutPositionsWithColumnCentering(IconArray *iconArray, 
                                                       const LayoutPreferences *prefs,
                                                       int targetColumns);

/* Helper functions for sorting */
static const char* GetFileExtension(const char *filename);
static int CompareDateStamps(const struct DateStamp *date1, const struct DateStamp *date2);

/*========================================================================*/
/* Main Processing Function                                              */
/*========================================================================*/

BOOL ProcessDirectoryWithPreferences(const char *path, 
                                    BOOL recursive, 
                                    const LayoutPreferences *prefs)
{
    char sanitizedPath[512];
    BOOL success = FALSE;
    BackupContext localContext;
    BackupPreferences backupPrefs;
    FolderWindowTracker windowTracker;
    BOOL trackerBuilt = FALSE;
    
    if (!path || !prefs)
    {
        printf("Error: Invalid parameters to ProcessDirectoryWithPreferences\n");
        return FALSE;
    }
    
    /* Copy and sanitize the path */
    strncpy(sanitizedPath, path, sizeof(sanitizedPath) - 1);
    sanitizedPath[sizeof(sanitizedPath) - 1] = '\0';
    sanitizeAmigaPath(sanitizedPath);
    
    /* Build window tracker if window moving is enabled (beta feature) */
    if (prefs->beta_FindWindowOnWorkbenchAndUpdate)
    {
        log_info(LOG_GENERAL, "\n*** Building window tracker for open folder windows ***\n");
        if (BuildFolderWindowList(&windowTracker))
        {
            trackerBuilt = TRUE;
            log_info(LOG_GENERAL, "Window tracker built successfully: %lu window(s) tracked\n", 
                    windowTracker.count);
        }
        else
        {
            log_warning(LOG_GENERAL, "Failed to build window tracker - window moving disabled for this run\n");
            /* Continue without window moving - not a fatal error */
        }
    }
    else
    {
        log_info(LOG_GENERAL, "Window moving disabled in preferences\n");
    }
    
    /* Initialize backup session if backup is enabled */
    if (prefs->backupPrefs.enableUndoBackup)
    {
        printf("\n*** Backup enabled - creating backup session ***\n");
        printf("Backup location: %s\n", prefs->backupPrefs.backupRootPath);
        
        /* Copy backup preferences from layout preferences */
        backupPrefs.enableUndoBackup = prefs->backupPrefs.enableUndoBackup;
        backupPrefs.useLha = prefs->backupPrefs.useLha;
        strncpy(backupPrefs.backupRootPath, prefs->backupPrefs.backupRootPath, 
                sizeof(backupPrefs.backupRootPath) - 1);
        backupPrefs.maxBackupsPerFolder = prefs->backupPrefs.maxBackupsPerFolder;
        
        if (InitBackupSession(&localContext, &backupPrefs, sanitizedPath))
        {
            g_backupContext = &localContext;
            printf("Backup session started successfully (Run #%u)\n", localContext.runNumber);
            printf("Backup directory: %s\n", localContext.runDirectory);
            printf("Source directory: %s\n", sanitizedPath);
            append_to_log("Backup session started: %s\n", localContext.runDirectory);
            append_to_log("Source directory: %s\n", sanitizedPath);
        }
        else
        {
            printf("Warning: Failed to start backup session - continuing without backup\n");
            append_to_log("ERROR: Failed to start backup session\n");
            g_backupContext = NULL;
        }
    }
    else
    {
        printf("\n*** Backup disabled - no backup will be created ***\n");
        g_backupContext = NULL;
    }
    
    /* Load left-out icons from the device's .backdrop file */
    loadLeftOutIcons(sanitizedPath);
    
    printf("\nProcessing: %s\n", sanitizedPath);
    printf("Recursive: %s\n", recursive ? "Yes" : "No");
    
    /* Start processing */
    if (recursive)
    {
        success = ProcessDirectoryRecursive(sanitizedPath, prefs, 0, 
                                           trackerBuilt ? &windowTracker : NULL);
    }
    else
    {
        success = ProcessSingleDirectory(sanitizedPath, prefs,
                                        trackerBuilt ? &windowTracker : NULL);
    }
    
    /* Free window tracker if it was built */
    if (trackerBuilt)
    {
        log_info(LOG_GENERAL, "Freeing window tracker\n");
        FreeFolderWindowList(&windowTracker);
    }
    
    /* End backup session if one was started */
    if (g_backupContext != NULL)
    {
        printf("\n*** Finalizing backup session ***\n");
        CloseBackupSession(g_backupContext);
        printf("Backup session completed successfully\n");
        printf("  Folders backed up: %u\n", g_backupContext->foldersBackedUp);
        printf("  Total bytes archived: %lu\n", (unsigned long)g_backupContext->totalBytesArchived);
        printf("  Location: %s\n", g_backupContext->runDirectory);
        
        append_to_log("\n*** Backup session completed ***\n");
        append_to_log("  Folders backed up: %u\n", g_backupContext->foldersBackedUp);
        append_to_log("  Failed backups: %u\n", g_backupContext->failedBackups);
        append_to_log("  Total bytes: %lu\n", (unsigned long)g_backupContext->totalBytesArchived);
        append_to_log("  Location: %s\n", g_backupContext->runDirectory);
        
        g_backupContext = NULL;
    }
    
    return success;
}

/*========================================================================*/
/* Calculate Icon Positions Based on Layout Preferences                  */
/*========================================================================*/

static void CalculateLayoutPositions(IconArray *iconArray, 
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
    int adjustmentOffset;
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
                /* Adjust all icons in the previous row based on alignment setting */
                for (int j = rowStartIndex; j < i; j++)
                {
                    FullIconDetails *rowIcon = &iconArray->array[j];
                    
                    /* Calculate adjustment based on alignment mode */
                    switch (prefs->textAlignment)
                    {
                        case TEXT_ALIGN_TOP:
                            /* No adjustment needed - icons already at top */
                            adjustmentOffset = 0;
                            break;
                            
                        case TEXT_ALIGN_MIDDLE:
                            /* Center icon vertically within row height */
                            adjustmentOffset = (maxHeightInRow - rowIcon->icon_max_height) / 2;
                            break;
                            
                        case TEXT_ALIGN_BOTTOM:
                            /* Align to bottom of row */
                            adjustmentOffset = maxHeightInRow - rowIcon->icon_max_height;
                            break;
                            
                        default:
                            adjustmentOffset = 0;
                            break;
                    }
                    
                    if (adjustmentOffset > 0)
                    {
                        rowIcon->icon_y += adjustmentOffset;
#ifdef DEBUG
                        append_to_log("  Adjusted icon %d ('%s') down by %d pixels for %s alignment\n",
                                      j, rowIcon->icon_text, adjustmentOffset,
                                      prefs->textAlignment == TEXT_ALIGN_TOP ? "top" :
                                      prefs->textAlignment == TEXT_ALIGN_MIDDLE ? "middle" : "bottom");
#endif
                    }
                }
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
#ifdef DEBUG
        append_to_log("\nAdjusting final row (indices %d to %d) for vertical alignment\n",
                      rowStartIndex, iconArray->size - 1);
        append_to_log("  maxHeightInRow = %d, alignment = %s\n", maxHeightInRow,
                      prefs->textAlignment == TEXT_ALIGN_TOP ? "top" :
                      prefs->textAlignment == TEXT_ALIGN_MIDDLE ? "middle" : "bottom");
#endif
        
        for (int j = rowStartIndex; j < iconArray->size; j++)
        {
            FullIconDetails *rowIcon = &iconArray->array[j];
            
            /* Calculate adjustment based on alignment mode */
            switch (prefs->textAlignment)
            {
                case TEXT_ALIGN_TOP:
                    /* No adjustment needed - icons already at top */
                    adjustmentOffset = 0;
                    break;
                    
                case TEXT_ALIGN_MIDDLE:
                    /* Center icon vertically within row height */
                    adjustmentOffset = (maxHeightInRow - rowIcon->icon_max_height) / 2;
                    break;
                    
                case TEXT_ALIGN_BOTTOM:
                    /* Align to bottom of row */
                    adjustmentOffset = maxHeightInRow - rowIcon->icon_max_height;
                    break;
                    
                default:
                    adjustmentOffset = 0;
                    break;
            }
            
            if (adjustmentOffset > 0)
            {
                rowIcon->icon_y += adjustmentOffset;
#ifdef DEBUG
                append_to_log("  Adjusted icon %d ('%s') down by %d pixels for %s alignment\n",
                              j, rowIcon->icon_text, adjustmentOffset,
                              prefs->textAlignment == TEXT_ALIGN_TOP ? "top" :
                              prefs->textAlignment == TEXT_ALIGN_MIDDLE ? "middle" : "bottom");
#endif
            }
        }
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
static void CalculateLayoutPositionsWithColumnCentering(IconArray *iconArray, 
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
    int adjustmentOffset;
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
        
        /* Calculate maximum width for each column */
        for (i = 0; i < iconArray->size; i++)
        {
            col = i % finalColumns;
            if (iconArray->array[i].icon_max_width > columnWidths[col])
            {
                columnWidths[col] = iconArray->array[i].icon_max_width;
            }
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
                for (int j = rowStartIndex; j < i; j++)
                {
                    FullIconDetails *rowIcon = &iconArray->array[j];
                    
                    /* Calculate adjustment based on alignment mode */
                    switch (prefs->textAlignment)
                    {
                        case TEXT_ALIGN_TOP:
                            /* No adjustment needed - icons already at top */
                            adjustmentOffset = 0;
                            break;
                            
                        case TEXT_ALIGN_MIDDLE:
                            /* Center icon vertically within row height */
                            adjustmentOffset = (maxHeightInRow - rowIcon->icon_max_height) / 2;
                            break;
                            
                        case TEXT_ALIGN_BOTTOM:
                            /* Align to bottom of row */
                            adjustmentOffset = maxHeightInRow - rowIcon->icon_max_height;
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
#ifdef DEBUG
        append_to_log("\nAdjusting final row (indices %d to %d) for vertical alignment\n",
                      rowStartIndex, iconArray->size - 1);
        append_to_log("  maxHeightInRow = %d, alignment = %s\n", maxHeightInRow,
                      prefs->textAlignment == TEXT_ALIGN_TOP ? "top" :
                      prefs->textAlignment == TEXT_ALIGN_MIDDLE ? "middle" : "bottom");
#endif
        for (int j = rowStartIndex; j < iconArray->size; j++)
        {
            FullIconDetails *rowIcon = &iconArray->array[j];
            
            /* Calculate adjustment based on alignment mode */
            switch (prefs->textAlignment)
            {
                case TEXT_ALIGN_TOP:
                    /* No adjustment needed - icons already at top */
                    adjustmentOffset = 0;
                    break;
                    
                case TEXT_ALIGN_MIDDLE:
                    /* Center icon vertically within row height */
                    adjustmentOffset = (maxHeightInRow - rowIcon->icon_max_height) / 2;
                    break;
                    
                case TEXT_ALIGN_BOTTOM:
                    /* Align to bottom of row */
                    adjustmentOffset = maxHeightInRow - rowIcon->icon_max_height;
                    break;
                    
                default:
                    adjustmentOffset = 0;
                    break;
            }
            
            if (adjustmentOffset > 0)
            {
                rowIcon->icon_y += adjustmentOffset;
#ifdef DEBUG
                append_to_log("  Adjusted icon %d down by %d pixels\n", j, adjustmentOffset);
#endif
            }
        }
    }
    
#ifdef DEBUG
    append_to_log("\nColumn-centered layout complete!\n");
#endif
    
    /* Cleanup */
    free(columnWidths);
    free(columnXPositions);
}

/*========================================================================*/
/* Single Directory Processing                                           */
/*========================================================================*/

static BOOL ProcessSingleDirectory(const char *path, 
                                   const LayoutPreferences *prefs,
                                   FolderWindowTracker *windowTracker)
{
    BPTR lock = 0;
    IconArray *iconArray = NULL;
    BOOL success = FALSE;
    
    /* Reset logging performance stats for this folder */
    reset_log_performance_stats();
    
#ifdef DEBUG
    append_to_log("ProcessSingleDirectory: path='%s'\n", path);
#endif
    
    /* Lock the directory */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        LONG error = IoErr();
        printf("Failed to lock directory: %s (error: %ld)\n", path, error);
#ifdef DEBUG
        append_to_log("Failed to lock %s, error: %ld\n", path, error);
#endif
        return FALSE;
    }
    
    /* Create icon array from directory */
    printf("  Reading icons...\n");
#ifdef DEBUG
    append_to_log("==== SECTION: LOADING ICONS ====\n");
#endif
    iconArray = CreateIconArrayFromPath(lock, path);
    
    if (!iconArray || iconArray->size == 0)
    {
        printf("  No icons found or error reading directory\n");
#ifdef DEBUG
        append_to_log("No icons in directory or error: %s\n", path);
#endif
        if (iconArray)
        {
            FreeIconArray(iconArray);
        }
        UnLock(lock);
        return FALSE;
    }
    
    printf("  Found %lu icons\n", (unsigned long)iconArray->size);
    
    /* Backup this directory if backup is enabled */
    if (g_backupContext != NULL)
    {
        printf("  Creating backup...\n");
        BackupStatus status = BackupFolder(g_backupContext, path, (UWORD)iconArray->size);
        if (status == BACKUP_OK)
        {
            printf("  ✓ Backup created successfully\n");
        }
        else
        {
            printf("  Warning: Backup failed (status %d) - continuing anyway\n", status);
        }
    }
    
    /* Sort icons according to preferences */
    printf("  Sorting icons...\n");
#ifdef DEBUG
    append_to_log("==== SECTION: SORTING ICONS ====\n");
#endif
    SortIconArrayWithPreferences(iconArray, prefs);
    
    /* Calculate and apply new positions based on sorted order */
    printf("  Calculating layout...\n");
#ifdef DEBUG
    append_to_log("==== SECTION: CALCULATING LAYOUT ====\n");
#endif
    
    /* Calculate optimal layout based on aspect ratio and overflow preferences */
    {
        int finalColumns = 0;
        int finalRows = 0;
        
        /* Use aspect ratio calculation to determine optimal columns/rows */
        CalculateLayoutWithAspectRatio(iconArray, prefs, &finalColumns, &finalRows);
        
#ifdef DEBUG
        append_to_log("Aspect ratio calculation complete: %d cols × %d rows\n", 
                      finalColumns, finalRows);
#endif
        
        /* Choose layout algorithm based on centerIconsInColumn preference */
        if (prefs->centerIconsInColumn)
        {
#ifdef DEBUG
            append_to_log("Using column-centered layout algorithm\n");
#endif
            CalculateLayoutPositionsWithColumnCentering(iconArray, prefs, finalColumns);
        }
        else
        {
#ifdef DEBUG
            append_to_log("Using standard row-based layout algorithm\n");
#endif
            CalculateLayoutPositions(iconArray, prefs, finalColumns);
        }
    }
    
    /* Resize window if requested */
    if (prefs->resizeWindows)
    {
        printf("  Resizing window...\n");
        resizeFolderToContents((char *)path, iconArray, windowTracker, prefs);
    }
    
    /* Save icon positions to disk */
    printf("  Saving icon positions...\n");
#ifdef DEBUG
    append_to_log("==== SECTION: SAVING ICONS ====\n");
#endif
    if (saveIconsPositionsToDisk(iconArray) == 0)
    {
        success = TRUE;
        printf("  Done!\n");
    }
    else
    {
        printf("  Failed to save icon positions\n");
    }
    
    /* Experimental Feature: Auto-open folders during processing
     * When enabled, this calls Workbench's OpenWorkbenchObjectA() to open
     * the folder window after iTidy has tidied the icons. This allows users
     * to see the results in real-time during recursive operations.
     */
    if (prefs->beta_openFoldersAfterProcessing && success)
    {
        BOOL openResult;
        
#ifdef DEBUG
        append_to_log("Opening folder window via Workbench: %s\n", path);
#endif
        
        /* OpenWorkbenchObjectA() opens a folder window via Workbench.
         * - First parameter: Full path to the folder
         * - Second parameter: TagList (NULL for default behavior)
         * 
         * Note: workbench.library is auto-opened by proto/wb.h,
         * so we don't need to manually OpenLibrary().
         * 
         * Returns: TRUE if successful, FALSE otherwise
         */
        openResult = OpenWorkbenchObjectA((CONST_STRPTR)path, NULL);
        
        if (!openResult)
        {
#ifdef DEBUG
            append_to_log("Warning: Failed to open folder window for: %s\n", path);
#endif
            /* Non-fatal error - continue processing even if window open fails */
        }
    }
    
    /* Print logging performance statistics */
    print_log_performance_stats();
    
    /* Cleanup */
    FreeIconArray(iconArray);
    UnLock(lock);
    
    return success;
}

/*========================================================================*/
/* Recursive Directory Processing                                        */
/*========================================================================*/

static BOOL ProcessDirectoryRecursive(const char *path, 
                                     const LayoutPreferences *prefs, 
                                     int recursion_level,
                                     FolderWindowTracker *windowTracker)
{
    BPTR lock = 0;
    struct FileInfoBlock *fib = NULL;
    char subdir[512];
    BOOL success = FALSE;
    
#ifdef DEBUG
    append_to_log("ProcessDirectoryRecursive: path='%s', level=%d\n", 
                  path, recursion_level);
#endif
    
    /* Safety check for recursion depth */
    if (recursion_level > 50)
    {
        printf("Warning: Maximum recursion depth reached at: %s\n", path);
        return FALSE;
    }
    
    /* Process this directory first */
    if (!ProcessSingleDirectory(path, prefs, windowTracker))
    {
        return FALSE; /* Stop if current directory fails */
    }
    
    /* Now process subdirectories */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        return FALSE;
    }
    
    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        UnLock(lock);
        return FALSE;
    }
    
    if (Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            /* Only process directories, skip files */
            if (fib->fib_DirEntryType > 0)
            {
                /* Build subdirectory path */
                /* Check if path ends with ':' (root device) - if so, don't add '/' */
                if (path[strlen(path) - 1] == ':') {
                    snprintf(subdir, sizeof(subdir), "%s%s", path, fib->fib_FileName);
                } else {
                    snprintf(subdir, sizeof(subdir), "%s/%s", path, fib->fib_FileName);
                }
                
                /* Check if folder is hidden (no .info file) and skip if enabled */
                if (prefs->skipHiddenFolders)
                {
                    char iconPath[520];
                    BPTR iconLock;
                    
                    snprintf(iconPath, sizeof(iconPath), "%s.info", subdir);
                    iconLock = Lock((STRPTR)iconPath, ACCESS_READ);
                    
                    if (!iconLock)
                    {
                        /* No .info file found - folder is hidden, skip it */
#ifdef DEBUG
                        append_to_log("Skipping hidden folder (no .info): %s\n", subdir);
#endif
                        printf("Skipping hidden folder: %s\n", fib->fib_FileName);
                        continue;
                    }
                    UnLock(iconLock);
                }
                
                /* Recursively process subdirectory */
                printf("\nEntering: %s\n", subdir);
                ProcessDirectoryRecursive(subdir, prefs, recursion_level + 1, windowTracker);
            }
        }
        success = TRUE;
    }
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    return success;
}

/*========================================================================*/
/* Helper Functions for Sorting                                          */
/*========================================================================*/

/**
 * @brief Get file extension from filename
 * @param filename The filename to extract extension from
 * @return Pointer to extension (including dot) or empty string if none
 */
static const char* GetFileExtension(const char *filename)
{
    const char *dot;
    
    if (!filename)
        return "";
    
    dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    
    return dot;
}

/**
 * @brief Compare two DateStamp structures
 * @param date1 First date
 * @param date2 Second date
 * @return <0 if date1 < date2, 0 if equal, >0 if date1 > date2
 */
static int CompareDateStamps(const struct DateStamp *date1, const struct DateStamp *date2)
{
    /* Compare days first (most significant) */
    if (date1->ds_Days != date2->ds_Days)
        return (date1->ds_Days > date2->ds_Days) ? 1 : -1;
    
    /* Days are equal, compare minutes */
    if (date1->ds_Minute != date2->ds_Minute)
        return (date1->ds_Minute > date2->ds_Minute) ? 1 : -1;
    
    /* Minutes are equal, compare ticks (1/50th second) */
    if (date1->ds_Tick != date2->ds_Tick)
        return (date1->ds_Tick > date2->ds_Tick) ? 1 : -1;
    
    /* Dates are identical */
    return 0;
}

/*========================================================================*/
/* Icon Sorting with Preferences                                         */
/*========================================================================*/

/* Global preference pointer for qsort comparison */
static const LayoutPreferences *g_sort_prefs = NULL;

/**
 * @brief Wrapper comparison function for qsort
 */
static int CompareIconsWrapper(const void *a, const void *b)
{
    return CompareIconsWithPreferences(a, b, g_sort_prefs);
}

static void SortIconArrayWithPreferences(IconArray *iconArray, 
                                        const LayoutPreferences *prefs)
{
    int i;
    
    if (!iconArray || iconArray->size <= 1 || !prefs)
    {
        return;
    }
    
#ifdef DEBUG
    append_to_log("Sorting %d icons with preferences\n", iconArray->size);
    append_to_log("  Sort by: %d, Priority: %d, Reverse: %d\n",
                  prefs->sortBy, prefs->sortPriority, prefs->reverseSort);
#endif
    
    /* Set global preference pointer for qsort */
    g_sort_prefs = prefs;
    
    /* Sort using preference-aware comparison */
    qsort(iconArray->array, iconArray->size, sizeof(FullIconDetails), 
          CompareIconsWrapper);
    
    /* Clear global pointer */
    g_sort_prefs = NULL;

#ifdef DEBUG
    append_to_log("After sorting, icon order:\n");
    append_to_log("%-3s | %-30s | %-6s\n", "ID", "Icon", "IsDir");
    append_to_log("--------------------------------------------\n");
    for (i = 0; i < iconArray->size; i++)
    {
        append_to_log("%-3d | %-30s | %-6s\n", 
                      i, 
                      iconArray->array[i].icon_text,
                      iconArray->array[i].is_folder ? "Folder" : "File");
    }
#endif
}

/*========================================================================*/
/* Preference-Aware Icon Comparison                                      */
/*========================================================================*/

static int CompareIconsWithPreferences(const void *a, const void *b, 
                                      const LayoutPreferences *prefs)
{
    const FullIconDetails *iconA = (const FullIconDetails *)a;
    const FullIconDetails *iconB = (const FullIconDetails *)b;
    int result = 0;
    const char *extA, *extB;
    size_t lenA, lenB, maxLen;
    
    if (!prefs)
        return 0;
    
    /* Apply folder/file priority first (unless MIXED) */
    if (prefs->sortPriority == SORT_PRIORITY_FOLDERS_FIRST)
    {
        if (iconA->is_folder && !iconB->is_folder) return -1;
        if (!iconA->is_folder && iconB->is_folder) return 1;
    }
    else if (prefs->sortPriority == SORT_PRIORITY_FILES_FIRST)
    {
        if (iconA->is_folder && !iconB->is_folder) return 1;
        if (!iconA->is_folder && iconB->is_folder) return -1;
    }
    /* SORT_PRIORITY_MIXED: No folder/file separation */
    
    /* Apply primary sort criteria */
    switch (prefs->sortBy)
    {
        case SORT_BY_NAME:
            /* Sort alphabetically by name */
            lenA = strlen(iconA->icon_text);
            lenB = strlen(iconB->icon_text);
            maxLen = (lenA > lenB) ? lenA : lenB;
            result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
            break;
            
        case SORT_BY_TYPE:
            /* Sort by file extension (type) */
            extA = GetFileExtension(iconA->icon_text);
            extB = GetFileExtension(iconB->icon_text);
            
            /* Compare extensions */
            lenA = strlen(extA);
            lenB = strlen(extB);
            maxLen = (lenA > lenB) ? lenA : lenB;
            
            if (lenA == 0 && lenB == 0)
            {
                /* Both have no extension, sort by name */
                lenA = strlen(iconA->icon_text);
                lenB = strlen(iconB->icon_text);
                maxLen = (lenA > lenB) ? lenA : lenB;
                result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
            }
            else if (lenA == 0)
            {
                /* A has no extension, put it first */
                result = -1;
            }
            else if (lenB == 0)
            {
                /* B has no extension, put it first */
                result = 1;
            }
            else
            {
                /* Compare extensions */
                result = strncasecmp_custom(extA, extB, maxLen);
                
                /* If extensions are the same, sort by name */
                if (result == 0)
                {
                    lenA = strlen(iconA->icon_text);
                    lenB = strlen(iconB->icon_text);
                    maxLen = (lenA > lenB) ? lenA : lenB;
                    result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
                }
            }
            break;
            
        case SORT_BY_DATE:
            /* Sort by modification date (newest first) */
            result = CompareDateStamps(&iconB->file_date, &iconA->file_date);
            
            /* If dates are the same, sort by name */
            if (result == 0)
            {
                lenA = strlen(iconA->icon_text);
                lenB = strlen(iconB->icon_text);
                maxLen = (lenA > lenB) ? lenA : lenB;
                result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
            }
            break;
            
        case SORT_BY_SIZE:
            /* Sort by file size (largest first for files, folders always 0) */
            if (iconA->file_size > iconB->file_size)
            {
                result = -1;  /* A is larger, comes first */
            }
            else if (iconA->file_size < iconB->file_size)
            {
                result = 1;   /* B is larger, comes first */
            }
            else
            {
                /* Same size, sort by name */
                lenA = strlen(iconA->icon_text);
                lenB = strlen(iconB->icon_text);
                maxLen = (lenA > lenB) ? lenA : lenB;
                result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
            }
            break;
            
        default:
            /* Unknown sort mode, default to name */
            lenA = strlen(iconA->icon_text);
            lenB = strlen(iconB->icon_text);
            maxLen = (lenA > lenB) ? lenA : lenB;
            result = strncasecmp_custom(iconA->icon_text, iconB->icon_text, maxLen);
            break;
    }
    
    /* Apply reverse sort if requested */
    if (prefs->reverseSort)
    {
        result = -result;
    }
    
    return result;
}
