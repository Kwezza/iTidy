/**
 * aspect_ratio_layout.c - Aspect Ratio and Window Overflow Algorithms
 * 
 * Implementation of aspect-ratio-based layout calculations and
 * window overflow handling.
 */

#include <platform/platform.h>
#include <platform/amiga_headers.h>

#include <stdio.h>
#include <math.h>

#include "aspect_ratio_layout.h"
#include "layout_preferences.h"
#include "icon_management.h"
#include "writeLog.h"

/* External screen dimensions (from window_management.c or platform) */
extern int screenWidth;
extern int screenHight;  /* Note: Misspelled 'screenHight' in original codebase */

/*========================================================================*/
/* Helper Functions: Average Icon Dimensions                            */
/*========================================================================*/

int CalculateAverageWidth(const IconArray *iconArray)
{
    int totalWidth = 0;
    int i;
    
    if (!iconArray || iconArray->size == 0)
    {
        return 80; /* Default icon width */
    }
    
    for (i = 0; i < iconArray->size; i++)
    {
        totalWidth += iconArray->array[i].icon_max_width;
    }
    
    return totalWidth / iconArray->size;
}

int CalculateAverageHeight(const IconArray *iconArray)
{
    int totalHeight = 0;
    int i;
    
    if (!iconArray || iconArray->size == 0)
    {
        return 80; /* Default icon height */
    }
    
    for (i = 0; i < iconArray->size; i++)
    {
        totalHeight += iconArray->array[i].icon_max_height;
    }
    
    return totalHeight / iconArray->size;
}

/*========================================================================*/
/* Custom Aspect Ratio Validation                                       */
/*========================================================================*/

BOOL ValidateCustomAspectRatio(int width, int height, float *outRatio)
{
    float ratio;
    
    if (!outRatio)
    {
        return FALSE;
    }
    
    /* Check for invalid values (zero or negative) */
    if (width <= 0 || height <= 0)
    {
        printf("ERROR: Aspect ratio values must be positive integers.\n");
        printf("       Received: %d:%d\n", width, height);
#ifdef DEBUG
        append_to_log("ValidateCustomAspectRatio: Invalid values %d:%d (must be positive)\n",
                      width, height);
#endif
        return FALSE;
    }
    
    /* Calculate ratio */
    ratio = (float)width / (float)height;
    *outRatio = ratio;
    
    /* Warn about extreme ratios but allow them */
    if (ratio > 5.0f || ratio < 0.2f)
    {
        printf("WARNING: Very extreme aspect ratio (%.2f).\n", ratio);
        printf("         Window may have unusual proportions.\n");
#ifdef DEBUG
        append_to_log("ValidateCustomAspectRatio: Extreme ratio %.2f from %d:%d\n",
                      ratio, width, height);
#endif
    }
    
#ifdef DEBUG
    append_to_log("ValidateCustomAspectRatio: %d:%d = %.2f (valid)\n",
                  width, height, ratio);
#endif
    
    return TRUE;
}

/*========================================================================*/
/* Optimal Column Calculation                                           */
/*========================================================================*/

int CalculateOptimalIconsPerRow(int totalIcons, 
                                 int averageIconWidth,
                                 int averageIconHeight, 
                                 float targetAspectRatio,
                                 int minAllowedIconsPerRow,
                                 int maxAllowedIconsPerRow)
{
    int bestColumns = minAllowedIconsPerRow;
    float bestRatioDiff = 999.0f;
    int cols, rows;
    float estimatedWidth, estimatedHeight, actualRatio, ratioDiff;
    
#ifdef DEBUG
    append_to_log("CalculateOptimalIconsPerRow:\n");
    append_to_log("  totalIcons=%d, avgWidth=%d, avgHeight=%d\n",
                  totalIcons, averageIconWidth, averageIconHeight);
    append_to_log("  targetRatio=%.2f, minCols=%d, maxCols=%d\n",
                  targetAspectRatio, minAllowedIconsPerRow, maxAllowedIconsPerRow);
#endif
    
    /* Clamp search range to valid values */
    if (minAllowedIconsPerRow < 1)
        minAllowedIconsPerRow = 1;
    
    if (maxAllowedIconsPerRow < minAllowedIconsPerRow)
        maxAllowedIconsPerRow = minAllowedIconsPerRow;
    
    /* Special case: if we have fewer icons than minimum columns */
    if (totalIcons < minAllowedIconsPerRow)
    {
#ifdef DEBUG
        append_to_log("  Special case: Only %d icons, using that as columns\n", totalIcons);
#endif
        return totalIcons;
    }
    
    /* Try different column counts to find best aspect ratio match */
    for (cols = minAllowedIconsPerRow; cols <= maxAllowedIconsPerRow; cols++)
    {
        /* Calculate rows needed (ceiling division) */
        rows = (totalIcons + cols - 1) / cols;
        
        /* Skip if this creates only one row with excess columns */
        if (rows == 1 && cols > totalIcons)
            continue;
        
        /* Estimate dimensions */
        estimatedWidth = (float)(cols * averageIconWidth);
        estimatedHeight = (float)(rows * averageIconHeight);
        
        /* Calculate actual ratio */
        actualRatio = estimatedWidth / estimatedHeight;
        
        /* Calculate difference from target */
        ratioDiff = fabs(actualRatio - targetAspectRatio);
        
#ifdef DEBUG
        append_to_log("  Try %d cols: %d rows, %.0f×%.0f = %.2f ratio (diff: %.2f)%s\n",
                      cols, rows, estimatedWidth, estimatedHeight, 
                      actualRatio, ratioDiff,
                      (ratioDiff < bestRatioDiff) ? " ← NEW BEST" : "");
#endif
        
        /* Track best match */
        if (ratioDiff < bestRatioDiff)
        {
            bestRatioDiff = ratioDiff;
            bestColumns = cols;
        }
    }
    
#ifdef DEBUG
    append_to_log("  Result: %d columns (ratio diff: %.2f)\n", bestColumns, bestRatioDiff);
#endif
    
    return bestColumns;
}

/*========================================================================*/
/* Main Layout Calculation with Aspect Ratio & Overflow                 */
/*========================================================================*/

void CalculateLayoutWithAspectRatio(const IconArray *iconArray, 
                                    const LayoutPreferences *prefs,
                                    int *finalColumns,
                                    int *finalRows)
{
    int avgWidth, avgHeight;
    int idealColumns, idealRows;
    int idealWidth, idealHeight;
    int maxUsableHeight, maxUsableWidth;
    int workbenchTitleHeight = 11; /* Default Workbench title bar height */
    BOOL fitsWidth, fitsHeight;
    int chrome = 20; /* Window chrome estimate (borders, scrollbars) */
    int maxCols, minCols;
    float effectiveAspectRatio;
    
    if (!iconArray || !prefs || !finalColumns || !finalRows)
    {
        return;
    }
    
    /* Handle empty icon array */
    if (iconArray->size == 0)
    {
        *finalColumns = 1;
        *finalRows = 0;
        return;
    }
    
#ifdef DEBUG
    append_to_log("\n==== CalculateLayoutWithAspectRatio ====\n");
    append_to_log("Icons: %d\n", iconArray->size);
    append_to_log("Preferences:\n");
    append_to_log("  aspectRatio: %.2f\n", prefs->aspectRatio);
    append_to_log("  overflowMode: %d (0=HORIZ, 1=VERT, 2=BOTH)\n", prefs->overflowMode);
    append_to_log("  minIconsPerRow: %d\n", prefs->minIconsPerRow);
    append_to_log("  maxIconsPerRow: %d\n", prefs->maxIconsPerRow);
    append_to_log("  iconSpacing: %d×%d\n", prefs->iconSpacingX, prefs->iconSpacingY);
#endif
    
    /* Calculate average icon dimensions */
    avgWidth = CalculateAverageWidth(iconArray);
    avgHeight = CalculateAverageHeight(iconArray);
    
#ifdef DEBUG
    append_to_log("Average icon size: %d×%d pixels\n", avgWidth, avgHeight);
#endif
    
    /* Determine effective aspect ratio (use custom if enabled) */
    if (prefs->useCustomAspectRatio)
    {
        ValidateCustomAspectRatio(prefs->customAspectWidth, 
                                  prefs->customAspectHeight,
                                  &effectiveAspectRatio);
#ifdef DEBUG
        append_to_log("Using custom aspect ratio: %d:%d = %.2f\n",
                      prefs->customAspectWidth, prefs->customAspectHeight,
                      effectiveAspectRatio);
#endif
    }
    else
    {
        effectiveAspectRatio = prefs->aspectRatio;
    }
    
    /* Calculate screen constraints */
    maxUsableWidth = screenWidth > 0 ? screenWidth : 640;
    maxUsableHeight = screenHight > 0 ? (screenHight - workbenchTitleHeight) : 500;
    
#ifdef DEBUG
    append_to_log("Screen constraints: %d×%d (usable)\n", 
                  maxUsableWidth, maxUsableHeight);
#endif
    
    /* Set column constraints */
    minCols = prefs->minIconsPerRow > 0 ? prefs->minIconsPerRow : 1;
    
    /* Handle Auto mode: maxIconsPerRow = 0 means calculate from screen width */
    if (prefs->maxIconsPerRow == 0)
    {
        /* AUTO MODE: Calculate maximum columns that fit on screen */
        int baseMaxCols;
        int effectiveWidth;
        int margins;
        
        /* Apply maxWindowWidthPct if set */
        if (prefs->maxWindowWidthPct > 0 && prefs->maxWindowWidthPct < 100)
        {
            effectiveWidth = (maxUsableWidth * prefs->maxWindowWidthPct) / 100;
        }
        else
        {
            effectiveWidth = maxUsableWidth;
        }
        
        /* Calculate margins (left + right spacing) */
        margins = (prefs->iconSpacingX * 2) + chrome;
        
        /* Calculate base max columns from screen width */
        baseMaxCols = (effectiveWidth - margins) / (avgWidth + prefs->iconSpacingX);
        
        /* For wide aspect ratios, allow MORE columns to achieve the desired ratio
         * The constraint should be screen width, not a pre-calculated column count.
         * We'll use a generous upper bound and let the aspect ratio algorithm decide. */
        maxCols = (maxUsableWidth - margins) / (avgWidth + prefs->iconSpacingX);
        
        /* Apply sanity limits */
        if (maxCols < minCols)
            maxCols = minCols;
        if (maxCols > 20)  /* Absolute upper limit to prevent extreme layouts */
            maxCols = 20;
        
#ifdef DEBUG
        append_to_log("Auto mode: effectiveWidth=%d, margins=%d, baseMaxCols=%d, maxCols=%d\n",
                      effectiveWidth, margins, baseMaxCols, maxCols);
#endif
    }
    else
    {
        /* MANUAL MODE: Use user-specified maximum */
        maxCols = prefs->maxIconsPerRow;
        
#ifdef DEBUG
        append_to_log("Manual mode: maxCols=%d (from preferences)\n", maxCols);
#endif
    }
    
    /* STAGE 1: Calculate ideal layout based on aspect ratio */
#ifdef DEBUG
    append_to_log("\n--- STAGE 1: Calculate Ideal Layout ---\n");
#endif
    
    idealColumns = CalculateOptimalIconsPerRow(
        iconArray->size, 
        avgWidth, 
        avgHeight,
        effectiveAspectRatio, 
        minCols,
        maxCols
    );
    
    idealRows = (iconArray->size + idealColumns - 1) / idealColumns;
    
    /* Estimate window dimensions (including spacing and chrome) */
    idealWidth = (idealColumns * avgWidth) + 
                 ((idealColumns + 1) * prefs->iconSpacingX) + 
                 chrome;
    idealHeight = (idealRows * avgHeight) + 
                  ((idealRows + 1) * prefs->iconSpacingY) + 
                  chrome;
    
#ifdef DEBUG
    append_to_log("Ideal layout: %d cols × %d rows\n", idealColumns, idealRows);
    append_to_log("Ideal window: %d×%d pixels\n", idealWidth, idealHeight);
#endif
    
    /* STAGE 2: Check if ideal layout fits on screen */
#ifdef DEBUG
    append_to_log("\n--- STAGE 2: Check Screen Fit ---\n");
#endif
    
    fitsWidth = (idealWidth <= maxUsableWidth);
    fitsHeight = (idealHeight <= maxUsableHeight);
    
#ifdef DEBUG
    append_to_log("Fits width: %s (%d <= %d)\n", 
                  fitsWidth ? "YES" : "NO", idealWidth, maxUsableWidth);
    append_to_log("Fits height: %s (%d <= %d)\n", 
                  fitsHeight ? "YES" : "NO", idealHeight, maxUsableHeight);
#endif
    
    if (fitsWidth && fitsHeight)
    {
        /* Perfect fit! Use ideal aspect ratio layout */
#ifdef DEBUG
        append_to_log("Result: FITS - Using ideal aspect ratio layout\n");
#endif
        *finalColumns = idealColumns;
        *finalRows = idealRows;
    }
    else
    {
        /* Doesn't fit - apply overflow strategy */
#ifdef DEBUG
        append_to_log("\n--- STAGE 3: Apply Overflow Strategy ---\n");
        append_to_log("Overflow mode: ");
        switch (prefs->overflowMode)
        {
            case OVERFLOW_HORIZONTAL:
                append_to_log("HORIZONTAL (expand width)\n");
                break;
            case OVERFLOW_VERTICAL:
                append_to_log("VERTICAL (expand height)\n");
                break;
            case OVERFLOW_BOTH:
                append_to_log("BOTH (maintain aspect ratio)\n");
                break;
        }
#endif
        
        switch (prefs->overflowMode)
        {
            case OVERFLOW_HORIZONTAL:
                /* Height first: maximize rows, expand columns as needed */
                *finalRows = maxUsableHeight / (avgHeight + prefs->iconSpacingY);
                if (*finalRows < 1)
                    *finalRows = 1;
                
                *finalColumns = (iconArray->size + *finalRows - 1) / *finalRows;
                
                /* Clamp to maxIconsPerRow if set */
                if (prefs->maxIconsPerRow > 0 && *finalColumns > prefs->maxIconsPerRow)
                {
                    *finalColumns = prefs->maxIconsPerRow;
                    /* Recalculate rows with clamped columns */
                    *finalRows = (iconArray->size + *finalColumns - 1) / *finalColumns;
                }
                
#ifdef DEBUG
                append_to_log("  Calculated: %d cols × %d rows (horizontal scrolling)\n",
                              *finalColumns, *finalRows);
#endif
                break;
                
            case OVERFLOW_VERTICAL:
                /* Width first: maximize columns, expand rows as needed */
                *finalColumns = maxUsableWidth / (avgWidth + prefs->iconSpacingX);
                if (*finalColumns < 1)
                    *finalColumns = 1;
                
                /* Clamp to maxIconsPerRow if set */
                if (prefs->maxIconsPerRow > 0 && *finalColumns > prefs->maxIconsPerRow)
                    *finalColumns = prefs->maxIconsPerRow;
                
                /* Ensure minimum columns */
                if (*finalColumns < minCols)
                    *finalColumns = minCols;
                
                *finalRows = (iconArray->size + *finalColumns - 1) / *finalColumns;
                
#ifdef DEBUG
                append_to_log("  Calculated: %d cols × %d rows (vertical scrolling)\n",
                              *finalColumns, *finalRows);
#endif
                break;
                
            case OVERFLOW_BOTH:
                /* Use ideal aspect ratio even if it means scrolling both ways */
                *finalColumns = idealColumns;
                *finalRows = idealRows;
                
                /* Still respect maxIconsPerRow as absolute constraint */
                if (prefs->maxIconsPerRow > 0 && *finalColumns > prefs->maxIconsPerRow)
                {
                    *finalColumns = prefs->maxIconsPerRow;
                    *finalRows = (iconArray->size + *finalColumns - 1) / *finalColumns;
                }
                
#ifdef DEBUG
                append_to_log("  Calculated: %d cols × %d rows (both scrollbars)\n",
                              *finalColumns, *finalRows);
#endif
                break;
        }
    }
    
#ifdef DEBUG
    append_to_log("\n==== Final Layout Decision ====\n");
    append_to_log("Columns: %d\n", *finalColumns);
    append_to_log("Rows: %d\n", *finalRows);
    append_to_log("Total positions: %d (icons: %d)\n", 
                  (*finalColumns) * (*finalRows), iconArray->size);
#endif
}

/* End of aspect_ratio_layout.c */
