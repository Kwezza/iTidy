/**
 * layout_processor.c - Icon Processing with Layout Preferences
 * 
 * Implements preference-aware directory processing that integrates
 * with the existing iTidy icon management system.
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

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

/* Forward declarations for helper functions */
static int CompareIconsWithPreferences(const void *a, const void *b, 
                                      const LayoutPreferences *prefs);
static void SortIconArrayWithPreferences(IconArray *iconArray, 
                                        const LayoutPreferences *prefs);
static BOOL ProcessSingleDirectory(const char *path, 
                                   const LayoutPreferences *prefs);
static BOOL ProcessDirectoryRecursive(const char *path, 
                                     const LayoutPreferences *prefs, 
                                     int recursion_level);

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
    
    if (!path || !prefs)
    {
        printf("Error: Invalid parameters to ProcessDirectoryWithPreferences\n");
        return FALSE;
    }
    
    /* Copy and sanitize the path */
    strncpy(sanitizedPath, path, sizeof(sanitizedPath) - 1);
    sanitizedPath[sizeof(sanitizedPath) - 1] = '\0';
    sanitizeAmigaPath(sanitizedPath);
    
    printf("\nProcessing: %s\n", sanitizedPath);
    printf("Recursive: %s\n", recursive ? "Yes" : "No");
    
    /* Start processing */
    if (recursive)
    {
        return ProcessDirectoryRecursive(sanitizedPath, prefs, 0);
    }
    else
    {
        return ProcessSingleDirectory(sanitizedPath, prefs);
    }
}

/*========================================================================*/
/* Calculate Icon Positions Based on Layout Preferences                  */
/*========================================================================*/

static void CalculateLayoutPositions(IconArray *iconArray, 
                                    const LayoutPreferences *prefs)
{
    int i;
    int currentX = 8;   /* Starting X position */
    int currentY = 4;   /* Starting Y position */
    int iconSpacing = 8; /* Horizontal spacing between icons */
    int maxWidth = 640;  /* Default screen width */
    int effectiveMaxWidth;
    int rightMargin = 16; /* Safety margin on right edge */
    int nextX;
    int maxHeightInRow = 0; /* Track tallest icon in current row */
    int rowStartY = 4; /* Y position where current row started */
    
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
    append_to_log("CalculateLayoutPositions: Positioning %d icons with dynamic width wrapping\n", 
                  iconArray->size);
    append_to_log("  screenWidth=%d, maxWindowWidthPct=%d, effectiveMaxWidth=%d\n",
                  maxWidth, prefs->maxWindowWidthPct, effectiveMaxWidth);
    append_to_log("%-3s | %-30s | %-4s | %-4s | %-5s | %-5s | %-6s\n", 
                  "ID", "Icon", "NewX", "NewY", "Width", "Height", "Offset");
    append_to_log("------------------------------------------------------------------------------------\n");
#endif
    
    /* Position icons in rows, wrapping based on actual accumulated width */
    for (i = 0; i < iconArray->size; i++)
    {
        FullIconDetails *icon = &iconArray->array[i];
        int iconOffset = 0;
        
        /* Calculate where the NEXT icon would start */
        nextX = currentX + icon->icon_max_width + iconSpacing;
        
        /* If this icon would go past the effective width limit, wrap to next row */
        /* (but always place at least one icon per row) */
        if (currentX > 8 && nextX > (effectiveMaxWidth - rightMargin))
        {
            /* Start new row - use the tallest icon from previous row */
            currentX = 8;
            currentY = rowStartY + maxHeightInRow + 8; /* 8px vertical spacing */
            rowStartY = currentY;
            maxHeightInRow = 0; /* Reset for new row */
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
        currentX += icon->icon_max_width + iconSpacing;
    }
}

/*========================================================================*/
/* Single Directory Processing                                           */
/*========================================================================*/

static BOOL ProcessSingleDirectory(const char *path, 
                                   const LayoutPreferences *prefs)
{
    BPTR lock = 0;
    IconArray *iconArray = NULL;
    BOOL success = FALSE;
    
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
    CalculateLayoutPositions(iconArray, prefs);
    
    /* Resize window if requested */
    if (prefs->resizeWindows)
    {
        printf("  Resizing window...\n");
        resizeFolderToContents((char *)path, iconArray);
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
                                     int recursion_level)
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
    if (!ProcessSingleDirectory(path, prefs))
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
                snprintf(subdir, sizeof(subdir), "%s/%s", path, fib->fib_FileName);
                
                /* Recursively process subdirectory */
                printf("\nEntering: %s\n", subdir);
                ProcessDirectoryRecursive(subdir, prefs, recursion_level + 1);
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
