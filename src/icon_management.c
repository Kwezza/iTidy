#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "icon_management.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "icon_types.h"
#include "utilities.h"
#include "spinner.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "dos/getDiskDetails.h"
#include "Settings/WorkbenchPrefs.h"
#include "Settings/IControlPrefs.h"

void FreeIconArray(IconArray *iconArray)
{

    size_t i;

    if (iconArray != NULL)
    {
        if (iconArray->array != NULL)
        {
            for (i = 0; i < iconArray->size; i++)
            {
                if (iconArray->array[i].icon_text != NULL)
                {
                    whd_free(iconArray->array[i].icon_text);
                } /* if */
                if (iconArray->array[i].icon_full_path != NULL)
                {
                    whd_free(iconArray->array[i].icon_full_path);
                } /* if */
            } /* for */
            whd_free(iconArray->array);
        } /* if */
        whd_free(iconArray);
    } /* if */
}

IconArray *CreateIconArray(void)
{
    IconArray *iconArray = (IconArray *)whd_malloc(sizeof(IconArray));
    if (iconArray != NULL)
    {
        memset(iconArray, 0, sizeof(IconArray));
        iconArray->array = NULL; /* Start with no allocated array */
        iconArray->size = 0;
        iconArray->capacity = 0;
    } /* if */
    return iconArray;
}

bool AddIconToArray(IconArray *iconArray, const FullIconDetails *newIcon)
{
    FullIconDetails *newArray;
    size_t newCapacity;

    if (iconArray == NULL || newIcon == NULL)
    {
        return false;
    } /* if */

    if (iconArray->size >= iconArray->capacity)
    {
        /* Increase capacity (start with 1 if currently 0) */
        newCapacity = (iconArray->capacity == 0) ? 1 : iconArray->capacity * 2;
        newArray = (FullIconDetails *)whd_malloc(newCapacity * sizeof(FullIconDetails));
        if (newArray == NULL)
        {
            return false;
        } /* if */

        memset(newArray, 0, newCapacity * sizeof(FullIconDetails));

        /* Copy existing elements to new array */
        if (iconArray->array != NULL)
        {
            memcpy(newArray, iconArray->array, iconArray->size * sizeof(FullIconDetails));
            whd_free(iconArray->array);
        } /* if */

        iconArray->array = newArray;
        iconArray->capacity = newCapacity;
    } /* if */

    /* Add the new icon details to the array */
    iconArray->array[iconArray->size] = *newIcon;
    iconArray->size += 1;

    return true;
}

IconArray *CreateIconArrayFromPath(BPTR lock, const char *dirPath)
{
    /* VBCC MIGRATION NOTE (Stage 4): Fixed BPTR type for lock parameter */
    struct TextExtent textExtent;
    FullIconDetails newIcon;
    struct FileInfoBlock *fib;
    IconSize iconSize = {0, 0};
    IconArray *iconArray = CreateIconArray();
    char fullPathAndFile[512];
    char fullPathAndFileNoInfo[512];
    char fileNameNoInfo[128];
    int textLength, fileCount = 0, maxWidth = 0;
    IconPosition iconPosition;

#ifdef DEBUG
    append_to_log("CreateIconArrayFromPath ENTRY: lock=%ld, dirPath='%s'\n", (LONG)lock, dirPath);
#endif

    iconArray->hasOnlyBorderlessIcons = false;

    if (!iconArray)
    {
        fprintf(stderr, "Error: Failed to create icon array.\n");
#ifdef DEBUG
        append_to_log("ERROR: Failed to create icon array\n");
#endif
        return NULL;
    }

#ifdef DEBUG
    append_to_log("Icon array created successfully\n");
#endif

    if (!(fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL)))
    {
        fprintf(stderr, "Error: Failed to allocate FileInfoBlock.\n");
#ifdef DEBUG
        append_to_log("ERROR: Failed to allocate FileInfoBlock\n");
#endif
        FreeIconArray(iconArray);
        return NULL;
    }
    
#ifdef DEBUG
    append_to_log("FileInfoBlock allocated successfully, calling Examine()\n");
#endif

#ifdef DEBUG_MAX
    append_to_log("Starting to process folder %s.\n", dirPath);
#endif
    if (Examine(lock, fib))
    {
#ifdef DEBUG
        append_to_log("Examine() successful, starting ExNext() loop\n");
#endif
        while (ExNext(lock, fib))
        {
#ifdef DEBUG
            append_to_log("DEBUG: ExNext returned, processing file: '%s'\n", fib->fib_FileName);
#endif
            updateCursor();                                             /* update progress spinner */
            if (fib->fib_DirEntryType < 0 || fib->fib_DirEntryType > 0) /* It's a file or a directory */
            {
#ifdef DEBUG
                append_to_log("DEBUG: File/dir type check passed, calling GetFullPath()\n");
#endif
                GetFullPath(dirPath, fib, fullPathAndFile, sizeof(fullPathAndFile));
#ifdef DEBUG
                append_to_log("DEBUG: GetFullPath() returned: '%s'\n", fullPathAndFile);
#endif

                // if (stricmp(fib->fib_FileName, "Disk.info") != 0)
                if (isIconTypeDisk(fullPathAndFile, fib->fib_DirEntryType) == 0)
                {
                    const char *fileExtension = strrchr(fib->fib_FileName, '.');

                    if (fileExtension != NULL && strlen(fileExtension) == 5 && strncasecmp_custom(fileExtension, ".info", 5) == 0 && strlen(fib->fib_FileName) > 5)
                    {
                        if (isIconLeftOut(fullPathAndFile) == false)
                        {
                            removeInfoExtension(fullPathAndFile, fullPathAndFileNoInfo);
                            if (IsValidIcon(fullPathAndFileNoInfo))
                            {
removeInfoExtension(fib->fib_FileName, fileNameNoInfo);
                                /* Reset newIcon to known defaults */
                                newIcon.icon_type = icon_type_standard;
                                newIcon.icon_height = 0;
                                newIcon.icon_width = 0;
                                newIcon.border_width = 0;
                                newIcon.text_width = 0;
                                newIcon.text_height = 0;
                                newIcon.icon_max_width = 0;
                                newIcon.icon_max_height = 0;
                                newIcon.icon_x = 0;
                                newIcon.icon_y = 0;
                                newIcon.icon_text = NULL;
                                newIcon.icon_full_path = NULL;
                                newIcon.is_folder = false;

#ifdef DEBUG_MAX
append_to_log("-------------------------\n");
                                append_to_log("Adding to %s Icon array.\n ", fullPathAndFile);
#endif
                                removeInfoExtension(fib->fib_FileName, fileNameNoInfo);
#ifdef DEBUG_MAX
                                append_to_log("\nCalculating text extent.\n", fullPathAndFile);
#endif
                                CalculateTextExtent(fileNameNoInfo, &textExtent);
#ifdef DEBUG_MAX
                                append_to_log("\nGetting current icon position.\n", fullPathAndFile);
#endif
                                iconPosition = GetIconPositionFromPath(fullPathAndFile);

                                /* Determine icon size based on format */

                                if (IsNewIconPath(fullPathAndFile) && user_forceStandardIcons==0)
                                {
#ifdef DEBUG_MAX
                                    append_to_log("Seems to be a NewIcon.\n", fullPathAndFile);
#endif
                                    /* printf("New Icon Format\n"); */
                                    newIcon.icon_type = icon_type_newIcon;
                                    GetNewIconSizePath(fullPathAndFile, &iconSize);
                                    count_icon_type_newIcon++;
                                }
                                else if

                                    (isOS35IconFormat(fullPathAndFile) && user_forceStandardIcons==0)
                                {
                                    newIcon.icon_type = icon_type_os35;
/* printf("OS3.5 Icon Format\n"); */
#ifdef DEBUG_MAX
                                    append_to_log("Seems to be a OS35 icon.\n", fullPathAndFile);
#endif
                                    getOS35IconSize(fullPathAndFile, &iconSize);
                                    count_icon_type_os35++;
                                }
                                else
                                {
                                    newIcon.icon_type = icon_type_standard;
#ifdef DEBUG_MAX
                                    append_to_log("Seems to be a standard icon\n");
#endif
                                    /* printf("Standard Icon Format\n"); */
                                    GetStandardIconSize(fullPathAndFile, &iconSize);
                                    count_icon_type_standard++;
                                }



                                if (prefsWorkbench.embossRectangleSize > 0)
                                {
                                    newIcon.icon_height = iconSize.height + prefsWorkbench.embossRectangleSize;
                                    newIcon.icon_width = iconSize.width + prefsWorkbench.embossRectangleSize;
                                }
                                else
                                {
                                    newIcon.icon_height = iconSize.height;
                                    newIcon.icon_width = iconSize.width;
                                }
#ifdef DEBUG_MAX
                                append_to_log("Checking for icon frame\n");
#endif
                                if (checkIconFrame(fullPathAndFile) == 1 || newIcon.icon_type == icon_type_standard)
                                {
                                    newIcon.border_width = prefsWorkbench.embossRectangleSize;
                                }
                                // else
                                {
                                }
                                // newIcon.has_border = checkIconFrame(fullPathAndFile);
                                #ifdef DEBUG_MAX
                                append_to_log("Icons size x: %d, y: %d, current at pos x: %d, y: %d border size:%d\n", iconSize.width, iconSize.height,newIcon.icon_x ,newIcon.icon_y,newIcon.border_width);

                                #endif

                                if (newIcon.border_width == 0)
                                {
                                    iconArray->hasOnlyBorderlessIcons = true;
                                }

                                newIcon.text_width = textExtent.te_Width;
                                newIcon.text_height = textExtent.te_Height;
                                newIcon.icon_max_width = MAX(iconSize.width + (newIcon.border_width * 2), textExtent.te_Width);
                                newIcon.icon_max_height = iconSize.height + GAP_BETWEEN_ICON_AND_TEXT + textExtent.te_Height;
                                newIcon.icon_x = iconPosition.x;
                                newIcon.icon_y = iconPosition.y;

#if PLATFORM_AMIGA
                                newIcon.is_write_protected = (fib->fib_Protection & FIBF_WRITE) ? true : false;
#else
                                newIcon.is_write_protected = false; /* Host stub */
#endif

#ifdef DEBUG_MAX
                                append_to_log("Icon is write protected: %d\n", newIcon.is_write_protected);
                                #endif
#ifdef DEBUG_MAX
                                append_to_log("calculated border: %d\n", (newIcon.border_width * 2));
#endif

                                /* Determine if it's a folder or a file */
                                newIcon.is_folder = isDirectory(fullPathAndFile);
#ifdef DEBUG_MAX
                                append_to_log("Allocating memory for icon path.\n");

#endif
                                /* Allocate and copy the full path and icon text */
                                newIcon.icon_full_path = (char *)whd_malloc(strlen(fullPathAndFile) + 1);
                                if (!newIcon.icon_full_path)
                                {
                                    fprintf(stderr, "Error: Failed to allocate memory for icon full path.\n");
                                    FreeIconArray(iconArray);
#if PLATFORM_AMIGA
                                    FreeDosObject(DOS_FIB, fib);
#else
                                    whd_free(fib);
#endif
                                    return NULL;
                                }
                                memset(newIcon.icon_full_path, 0, strlen(fullPathAndFile) + 1);
                                strcpy(newIcon.icon_full_path, fullPathAndFile);

                                textLength = strlen(fib->fib_FileName) + 1;
                                newIcon.icon_text = (char *)whd_malloc(textLength);
                                if (!newIcon.icon_text)
                                {
                                    fprintf(stderr, "Error: Failed to allocate memory for icon text.\n");
                                    whd_free(newIcon.icon_full_path);
                                    FreeIconArray(iconArray);
#if PLATFORM_AMIGA
                                    FreeDosObject(DOS_FIB, fib);
#else
                                    whd_free(fib);
#endif
                                    return NULL;
                                }
                                memset(newIcon.icon_text, 0, textLength);
                                strcpy(newIcon.icon_text, fib->fib_FileName);

                                /* Update the maximum width */
                                if (newIcon.icon_max_width > maxWidth)
                                {
                                    maxWidth = newIcon.icon_max_width;
                                }

                                /* Add new icon to the array */
                                if (!AddIconToArray(iconArray, &newIcon))
                                {
                                    fprintf(stderr, "Error: Failed to add icon to array.\n");
                                    whd_free(newIcon.icon_text);
                                    whd_free(newIcon.icon_full_path);
                                    FreeIconArray(iconArray);
#if PLATFORM_AMIGA
                                    FreeDosObject(DOS_FIB, fib);
#else
                                    whd_free(fib);
#endif
                                    return NULL;
                                }

                                fileCount++;
                            }
                            else
                            {
                                fprintf(stderr, "Error: Unknown or corrupted icon file: %s\n", fullPathAndFile);

                                //iconsErrorTracker.count++;
                                AddIconError(&iconsErrorTracker, fullPathAndFile);
                            }
                        }
                    }
                }
            }
            eraseSpinner();
        }
    }

    /* Set the largest icon width */
    iconArray->BiggestWidthPX = maxWidth;

    /* Free the FileInfoBlock before returning */
    FreeDosObject(DOS_FIB, fib);
#ifdef DEBUG_MAX
append_to_log("Has only borderless icons: %d\n", iconArray->hasOnlyBorderlessIcons);
#endif
#ifdef DEBUG

    dumpIconArrayToScreen(iconArray);
#endif

    return iconArray;
}

/* Comparison function for sorting by is_folder and then by icon_text */
int CompareByFolderAndName(const void *a, const void *b)
{
    int nameComparisonResult;
    const FullIconDetails *iconA = (const FullIconDetails *)a;
    const FullIconDetails *iconB = (const FullIconDetails *)b;

    /* Debug: Print comparison details /
    printf("Comparing: %s (folder: %d) vs %s (folder: %d)\n",
           iconA->icon_text, iconA->is_folder,
           iconB->icon_text, iconB->is_folder);*

    /* Compare by is_folder: folders should come before files */
    if (iconA->is_folder != iconB->is_folder)
    {
        int result = iconA->is_folder ? -1 : 1;
        /*printf("Folder comparison result: %d\n", result);  Debug */
        return result;
    }

    /* If both are either folders or files, compare by icon_text */
    nameComparisonResult = strncasecmp_custom(iconA->icon_text, iconB->icon_text,
                                              strlen(iconA->icon_text) > strlen(iconB->icon_text) ? strlen(iconB->icon_text) : strlen(iconA->icon_text));

    /* Debug: Print name comparison result /
    printf("Name comparison result for %s vs %s: %d\n", iconA->icon_text, iconB->icon_text, nameComparisonResult);*/

    return nameComparisonResult;
}

int ArrangeIcons(BPTR lock, char *dirPath, int newWidth)
{
    /* VBCC MIGRATION NOTE (Stage 4): Fixed BPTR type for lock parameter */
    /* Initial declarations */
    int32_t x, y, maxX, maxY, windowWidth, maxWindowWidth;
    IconArray *iconArray;
    bool SkipAutoResize;
    int i, totalIcons, iconsPerRow;
    int iconSpacingX, iconSpacingY;
    char fileNameNoInfo[256];
    int largestIconWidth, columnWidths[100]; /* Assuming a max of 100 columns for simplicity */
    int centerX;
    int minIconsPerRow;
    // int screenWidth =screenWidth ;                   /* Standard Amiga Workbench resolution width */
    int rowCount;                    /* For tracking the number of rows */
    int dynamicPaddingY;             /* For dynamic bottom padding */
    int column;                      /* For storing column index within the loop */
    int iconHeight;                  /* For storing the height of the current icon */
    int rowStartIndex, maxRowHeight; /* For storing the starting index and max height of the current row */
    int borderSpacingForIconsWithNoSpacing = 0;

    char *temp1;
    char *adjustedTemp ;

    /* Initialize variables */
    x = ICON_START_X;
    y = ICON_START_Y;
    maxX = 0;
    maxY = 0;
    iconSpacingX = ICON_SPACING_X;
    iconSpacingY = ICON_SPACING_Y;
    SkipAutoResize = false;
    minIconsPerRow = 3;
    rowCount = 0;
    rowStartIndex = 0;



    temp1= removeTextFromStartOfString(dirPath, filePath);
    if (temp1 && *temp1 != '\0') {  /* Ensure temp1 is not NULL and not empty */
        /* If the first character is '/', adjust the pointer */
        adjustedTemp = (*temp1 == '/') ? temp1 + 1 : temp1;

        printf("  - %s\n", adjustedTemp);
    }

    if (temp1) {
        whd_free(temp1);  /* Free allocated memory */
    }

    //printf("  %s\n", dirPath);
    #ifdef DEBUG
        append_to_log("======================================================\n");
        append_to_log("Tidying folder: %s\n", dirPath);
        append_to_log("======================================================\n", dirPath);
    #endif

    iconArray = CreateIconArrayFromPath(lock, dirPath);

    if (iconArray == NULL || iconArray->array == NULL || iconArray->size <= 0)
    {
        // fprintf(stderr, "Error: Failed to create icon array.\n");  //this often is the case if the folder has no icons
        return -1;
    }

    totalIcons = iconArray->size;

    #ifdef DEBUG_MAX
    append_to_log("Arranging %d icons in %s\n", totalIcons, dirPath);
    append_to_log("Start of icon tidy routine\n");
#endif

    /* Sort icons by name and folder for a consistent layout */
    qsort(iconArray->array, totalIcons, sizeof(FullIconDetails), CompareByFolderAndName);

    /* Check the largest width of any icon to determine spacing needs */
    largestIconWidth = iconArray->BiggestWidthPX;
#ifdef DEBUG_MAX
    append_to_log("Largest icon width: %d\n", largestIconWidth);
#endif

    /* Ensure the largest icon width is reasonable */
    if (largestIconWidth <= 0)
    {
        fprintf(stderr, "Error: Invalid icon width.\n");
        FreeIconArray(iconArray);
        return -1;
    }

    /* Calculate the maximum allowable window width based on screen width and preferences */
    maxWindowWidth = screenWidth - prefsIControl.currentBarWidth - prefsIControl.currentLeftBarWidth - (PADDING_WIDTH * 2);
    if (prefsWorkbench.disableVolumeGauge && IsRootDirectorySimple(dirPath))
        maxWindowWidth = maxWindowWidth - prefsIControl.currentCGaugeWidth;

    /* Correct the max window width if it exceeds the screen width */
    if (maxWindowWidth > screenWidth)
    {
        maxWindowWidth = screenWidth - prefsIControl.currentBarWidth - prefsIControl.currentLeftBarWidth - (PADDING_WIDTH * 2);
#ifdef DEBUG_MAX
        append_to_log("Adjusted max window width to fit screen: %d\n", maxWindowWidth);
#endif
    }
    else
    {
#ifdef DEBUG_MAX
        append_to_log("Calculated max window width: %d\n", maxWindowWidth);
#endif
    }

    /* Determine the optimal number of icons per row */
    iconsPerRow = MAX((newWidth - ICON_START_X) / (largestIconWidth + iconSpacingX), minIconsPerRow);
#ifdef DEBUG_MAX
    append_to_log("Initial icons per row calculated: %d\n", iconsPerRow);
#endif
    /* Adjust window width to fit these icons evenly */
    windowWidth = ICON_START_X + iconsPerRow * (largestIconWidth + iconSpacingX);
#ifdef DEBUG_MAX
    append_to_log("Adjusted initial window width: %d\n", windowWidth);
#endif
    /* Expand window width to fit more icons if necessary and possible */
    while (windowWidth < maxWindowWidth && (totalIcons / iconsPerRow) > (iconsPerRow / 2))
    {
        iconsPerRow++;
        windowWidth = ICON_START_X + iconsPerRow * (largestIconWidth + iconSpacingX);
#ifdef DEBUG_MAX
        append_to_log("Expanding window width: %d, Icons per row: %d\n", windowWidth, iconsPerRow);
#endif
    }

    /* Adjust window width to ensure it doesn't exceed maxWindowWidth */
    if (windowWidth > maxWindowWidth)
    {
        windowWidth = maxWindowWidth;
        iconsPerRow = MAX((maxWindowWidth - ICON_START_X) / (largestIconWidth + iconSpacingX), minIconsPerRow);
#ifdef DEBUG_MAX
        append_to_log("Adjusted to max window width: %d, Icons per row: %d\n", windowWidth, iconsPerRow);
#endif
    }

    /* Initialize column widths */
    for (i = 0; i < iconsPerRow; i++)
    {
        columnWidths[i] = 0;
    }

    /* Determine the maximum width for each column */
    for (i = 0; i < totalIcons; i++)
    {
        column = i % iconsPerRow;
        columnWidths[column] = MAX(columnWidths[column], iconArray->array[i].icon_max_width);
    }

    /* Arrange the icons in rows and columns */
    x = ICON_START_X;
    y = ICON_START_Y;
    rowCount = 0;      /* Reset rowCount for tracking */
    rowStartIndex = 0; /* Start index for the current row */

    while (rowStartIndex < totalIcons)
    {
        updateCursor(); /* update progress spinner */
        /* Determine the maximum height of the current row */
        maxRowHeight = 0;

        /* Determine the additional padding based on workbench and icon border settings */

        for (i = rowStartIndex; i < rowStartIndex + iconsPerRow && i < totalIcons; i++)
        {
            maxRowHeight = MAX(maxRowHeight, iconArray->array[i].icon_max_height) + borderSpacingForIconsWithNoSpacing;
        }
#ifdef DEBUG_MAX
        append_to_log("Row %d max height: %d\n", rowCount, maxRowHeight);
#endif
        for (i = rowStartIndex; i < rowStartIndex + iconsPerRow && i < totalIcons; i++)
        {
            if (iconArray->hasOnlyBorderlessIcons == 0)
            {

                if (!prefsWorkbench.borderless && iconArray->array[i].border_width == 0)
                {
                    borderSpacingForIconsWithNoSpacing = 0;
                }
                else
                {
                    if (!prefsWorkbench.borderless && iconArray->array[i].border_width > 0)
                    {
                        borderSpacingForIconsWithNoSpacing = iconArray->array[i].border_width;
                    }
                    else
                    {
                        borderSpacingForIconsWithNoSpacing = prefsWorkbench.embossRectangleSize;
                    }
                }
            }
            else
            {
                borderSpacingForIconsWithNoSpacing = 0;
            }

            column = i % iconsPerRow;
            iconHeight = iconArray->array[i].icon_max_height + (borderSpacingForIconsWithNoSpacing); // experimenting with icon spacing by dividing the y height by 2

            removeInfoExtension(iconArray->array[i].icon_full_path, fileNameNoInfo);

            /* Center the icon within the column width and adjust for max row height */
            centerX = ((columnWidths[column] - (iconArray->array[i].icon_width + (+borderSpacingForIconsWithNoSpacing * 2))) / 2);
            iconArray->array[i].icon_x = x + centerX;
            /* Align to the bottom of the row by setting y based on maxRowHeight */
            iconArray->array[i].icon_y = y + (maxRowHeight - iconHeight);
#ifdef DEBUG_MAX
            append_to_log("Placing icon %d at (x: %ld, y: %ld) with border of: %dpx, width %d and height %d name: %s\n",

                          i, iconArray->array[i].icon_x, iconArray->array[i].icon_y + borderSpacingForIconsWithNoSpacing, borderSpacingForIconsWithNoSpacing,
                          iconArray->array[i].icon_max_width + borderSpacingForIconsWithNoSpacing, iconArray->array[i].icon_max_height + borderSpacingForIconsWithNoSpacing,
                          iconArray->array[i].icon_text);
#endif

            maxX = MAX(maxX, (iconArray->array[i].icon_x + iconArray->array[i].icon_max_width) + borderSpacingForIconsWithNoSpacing);
            maxY = MAX(maxY, (iconArray->array[i].icon_y + iconArray->array[i].icon_max_height) + borderSpacingForIconsWithNoSpacing);
#ifdef DEBUG_MAX
            append_to_log("Updated maxX: %ld, maxY: %ld\n", maxX, maxY);
#endif

            x += columnWidths[column] + iconSpacingX; /* Use the specific width for this column */
        }

        /* Move to the next row */
        x = ICON_START_X;
        y += maxRowHeight + iconSpacingY;
        rowStartIndex += iconsPerRow;
        rowCount++;
#ifdef DEBUG_MAX
        append_to_log("Moving to next row: y = %ld, rowStartIndex = %d, rowCount = %d\n", y, rowStartIndex, rowCount);
#endif
    }
    eraseSpinner();

    /* Calculate dynamic padding */
    dynamicPaddingY = MAX(10, ICON_START_Y / 2); /* Ensure at least 10 pixels padding */

    /* Adjust maxY to avoid excessive gap at the bottom */
    maxY += dynamicPaddingY; /* Add dynamic padding at the bottom */
#ifdef DEBUG_MAX
    append_to_log("Final adjusted maxY with padding: %ld\n", maxY);
    append_to_log("Final maxX: %ld, maxY: %ld\n", maxX, maxY);
#endif
    /* Reposition the window to accommodate all icons neatly */
    if (user_dontResize == false)
        repoistionWindow(dirPath, maxX, maxY);

    /* Save the icon positions to disk */
    saveIconsPositionsToDisk(iconArray);

    FreeIconArray(iconArray);
#ifdef DEBUG


    append_to_log("Completed arranging icons in %d rows with %d icons per row. Path: %s\n",
                  rowCount, iconsPerRow, dirPath); /* Correct the row count display */
#endif
    return 0;
}

bool checkIconFrame(const char *iconName)
{
#if PLATFORM_AMIGA
    struct DiskObject *icon = NULL;
    int32_t frameStatus;
    int32_t errorCode;
    struct Library *IconBase = NULL;
    const char *extension = ".info";
    size_t len = strlen(iconName);
    size_t ext_len = strlen(extension);

    /* Calculate the length of the new icon name without the ".info" extension if present */
    size_t new_len = (len >= ext_len && platform_stricmp(iconName + len - ext_len, extension) == 0) ? len - ext_len : len;

    /* Allocate memory for the new icon name */
    char *newIconName = (char *)malloc((new_len + 1) * sizeof(char));
    if (newIconName == NULL)
    {
        printf("Memory allocation failed\n");
        return false;
    }

    /* Copy the icon name up to the new length and null-terminate it */
    strncpy(newIconName, iconName, new_len);
    newIconName[new_len] = '\0';
#ifdef DEBUG_MAX
    append_to_log("Opening icon library\n");
#endif
    /* Open the icon.library */
    IconBase = OpenLibrary("icon.library", 0);
    if (!IconBase)
    {
        printf("Failed to open icon.library\n");
        free(newIconName);
        return true; /* Assume it has a frame if library can't be opened */
    }

    if (IconBase->lib_Version < 44)
    {
#ifdef DEBUG
        append_to_log("icon.library version: %ld.%ld\n", IconBase->lib_Version, IconBase->lib_Revision);
        append_to_log("icon.library version 44 or higher is required to check for frames.\n");
#endif
        CloseLibrary(IconBase);
        free(newIconName);
        return true; /* Assume it has a frame if library version is too low */
    }

    /* Load the icon using the new name without the ".info" extension */
#ifdef DEBUG_MAX
    append_to_log("icon.library version: %ld.%ld\n", IconBase->lib_Version, IconBase->lib_Revision);
    append_to_log("icon for border checks: %s\n", newIconName);
    append_to_log("Getting icon tags\n");
#endif

    icon = GetIconTags(newIconName, TAG_END);
    if (!icon)
    {
        // printf("Failed to load icon for border checks: %s\n", newIconName);
        CloseLibrary(IconBase);
        free(newIconName);
        return true; /* Assume it has a frame if icon can't be loaded */
    }
#ifdef DEBUG_MAX

    append_to_log("Checking to see if it has a border\n");
#endif
    /* Use IconControl to check the frame status of the icon */
    if (IconControl(icon,
                    ICONCTRLA_GetFrameless, &frameStatus,
                    ICONA_ErrorCode, &errorCode,
                    TAG_END) == 1)
    {
        /* A frameStatus of 0 means it has a frame, any other value means it does not */
        bool hasFrame = (frameStatus == 0);

        /* Cleanup */
        FreeDiskObject(icon);
        CloseLibrary(IconBase);
        free(newIconName);

        return hasFrame;
    }
    else
    {
        printf("Failed to retrieve frame information; error code: %ld\n", errorCode);
        PrintFault(errorCode, NULL);
    }

    /* Cleanup */
    if (icon)
    {
        FreeDiskObject(icon);
    }
    CloseLibrary(IconBase);
    free(newIconName);
    return true; /* Default to having a frame if there's an error */
#else
    /* Host build stub - assume frames */
    (void)iconName;
    return true;
#endif
}
void dumpIconArrayToScreen(IconArray *iconArray)
{
    int i;
    const char *iconTypeStr;
    char xStr[12], yStr[12]; // Buffers for X and Y, can hold "None" or numbers

    #ifdef DEBUG
    if(iconArray->size==0)
    {
        append_to_log("No icons in the folder that can be arranged\n");
    }
    else
    {
    append_to_log("%-3s | %-6s | %-6s | %-6s | %-6s | %-6s | %-6s | %-8s | %-8s | %-6s | %-8s | %-20s\n",
                  "ID", "Width", "Height", "TxtW", "TxtH", "MaxW", "MaxH", "X", "Y", "Border", "Type", "Icon");
    append_to_log("----------------------------------------------------------------------------------------------------------------\n");
    }
#endif

    for (i = 0; i < iconArray->size; i++)
    {
        // Determine the icon type string
        switch (iconArray->array[i].icon_type)
        {
            case 1:
                iconTypeStr = "NewIcon";
                break;
            case 2:
                iconTypeStr = "OS3.5";
                break;
            default:
                iconTypeStr = "Standard";
                break;
        }

        // Convert X and Y to "None" if they are -2147483648, otherwise store the number
        if (iconArray->array[i].icon_x == -2147483648)
            strcpy(xStr, "None");
        else
            sprintf(xStr, "%d", iconArray->array[i].icon_x);

        if (iconArray->array[i].icon_y == -2147483648)
            strcpy(yStr, "None");
        else
            sprintf(yStr, "%d", iconArray->array[i].icon_y);

        append_to_log("%-3d | %-6d | %-6d | %-6d | %-6d | %-6d | %-6d | %-8s | %-8s | %-6d | %-8s | %-20s\n",
                      i,
                      iconArray->array[i].icon_width,
                      iconArray->array[i].icon_height,
                      iconArray->array[i].text_width,
                      iconArray->array[i].text_height,
                      iconArray->array[i].icon_max_width,
                      iconArray->array[i].icon_max_height,
                      xStr,  // X position (or "None")
                      yStr,  // Y position (or "None")
                      iconArray->array[i].border_width,
                      iconTypeStr,
                      iconArray->array[i].icon_text);
    }
}







bool IsValidIcon(const char *iconPath)
{
    struct DiskObject *diskObj;

    /* Attempt to get the DiskObject from the icon file */
    diskObj = GetDiskObject(iconPath);
    if (diskObj != NULL)
    {
#ifdef DEBUG_MAX
        append_to_log("Icon is a valid icon: %s\n", iconPath);
#endif
        /* Successfully retrieved the DiskObject, so the icon is valid */
        FreeDiskObject(diskObj); /* Clean up to prevent memory leaks */
        return true;
    }
    else
    {
        /* Failed to retrieve the DiskObject, so the icon is invalid */
        append_to_log("Icon is NOT a valid icon: %s\n", iconPath);
        return false;
    }
}
