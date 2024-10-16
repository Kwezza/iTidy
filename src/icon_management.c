#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stddef.h>
#include <exec/memory.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <exec/libraries.h>

#include "main.h"
#include "icon_management.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "icon_types.h"
#include "utilities.h"
#include "spinner.h"
#include "writeLog.h"
#include "icon_misc.h"

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
                    FreeVec(iconArray->array[i].icon_text);
                } /* if */
                if (iconArray->array[i].icon_full_path != NULL)
                {
                    FreeVec(iconArray->array[i].icon_full_path);
                } /* if */
            } /* for */
            FreeVec(iconArray->array);
        } /* if */
        FreeVec(iconArray);
    } /* if */
}

IconArray *CreateIconArray(void)
{
    IconArray *iconArray = (IconArray *)AllocVec(sizeof(IconArray), MEMF_CLEAR);
    if (iconArray != NULL)
    {
        iconArray->array = NULL; /* Start with no allocated array */
        iconArray->size = 0;
        iconArray->capacity = 0;
    } /* if */
    return iconArray;
}

BOOL AddIconToArray(IconArray *iconArray, const FullIconDetails *newIcon)
{
    FullIconDetails *newArray;
    size_t newCapacity;

    if (iconArray == NULL || newIcon == NULL)
    {
        return FALSE;
    } /* if */

    if (iconArray->size >= iconArray->capacity)
    {
        /* Increase capacity (start with 1 if currently 0) */
        newCapacity = (iconArray->capacity == 0) ? 1 : iconArray->capacity * 2;
        newArray = (FullIconDetails *)AllocVec(newCapacity * sizeof(FullIconDetails), MEMF_CLEAR);
        if (newArray == NULL)
        {
            return FALSE;
        } /* if */

        /* Copy existing elements to new array */
        if (iconArray->array != NULL)
        {
            CopyMem(iconArray->array, newArray, iconArray->size * sizeof(FullIconDetails));
            FreeVec(iconArray->array);
        } /* if */

        iconArray->array = newArray;
        iconArray->capacity = newCapacity;
    } /* if */

    /* Add the new icon details to the array */
    iconArray->array[iconArray->size] = *newIcon;
    iconArray->size += 1;

    return TRUE;
}

IconArray *CreateIconArrayFromPath(BPTR lock, const char *dirPath)
{
    struct TextExtent textExtent;
    FullIconDetails newIcon;
    struct FileInfoBlock *fib;
    IconSize iconSize = {0, 0};
    IconArray *iconArray = CreateIconArray();
    char fullPathAndFile[512];
    char fileNameNoInfo[128];
    int textLength, fileCount = 0, maxWidth = 0;
    IconPosition iconPosition;

    iconArray->hasOnlyBorderlessIcons = FALSE;

    if (!iconArray)
    {
        fprintf(stderr, "Error: Failed to create icon array.\n");
        return NULL;
    }

    if (!(fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL)))
    {
        fprintf(stderr, "Error: Failed to allocate FileInfoBlock.\n");
        FreeIconArray(iconArray);
        return NULL;
    }
#ifdef DEBUG
    append_to_log("Starting dir scan to add icons to array.\n");
#endif
    if (Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            updateCursor();                                             /* update progress spinner */
            if (fib->fib_DirEntryType < 0 || fib->fib_DirEntryType > 0) /* It's a file or a directory */
            {
                GetFullPath(dirPath, fib, fullPathAndFile, sizeof(fullPathAndFile));

                // if (stricmp(fib->fib_FileName, "Disk.info") != 0)
                if (isIconTypeDisk(fullPathAndFile, fib->fib_DirEntryType) == 0)
                {
                    const char *fileExtension = strrchr(fib->fib_FileName, '.');

                    if (fileExtension != NULL && strlen(fileExtension) == 5 && strncasecmp_custom(fileExtension, ".info", 5) == 0 && strlen(fib->fib_FileName) > 5)
                    {
                        if (isIconLeftOut(fullPathAndFile) == FALSE)
                        {
                            //                        continue;
                            //                    }
                            //                    {

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
                            newIcon.is_folder = FALSE;

#ifdef DEBUG
                            append_to_log("Adding to %s Icon array.\n", fullPathAndFile);
#endif
                            removeInfoExtension(fib->fib_FileName, fileNameNoInfo);
#ifdef DEBUG
                            append_to_log("Calculating text extent.\n", fullPathAndFile);
#endif
                            CalculateTextExtent(fileNameNoInfo, &textExtent);
#ifdef DEBUG
                            append_to_log("Getting current icon position.\n", fullPathAndFile);
#endif
                            iconPosition = GetIconPositionFromPath(fullPathAndFile);

                            /* Determine icon size based on format */

                            if (IsNewIconPath(fullPathAndFile))
                            {
#ifdef DEBUG
                                append_to_log("Getting New Icon details.\n", fullPathAndFile);
#endif
                                /* printf("New Icon Format\n"); */
                                newIcon.icon_type = icon_type_newIcon;
                                GetNewIconSizePath(fullPathAndFile, &iconSize);
                            }
                            else if

                                (isOS35IconFormat(fullPathAndFile))
                            {
                                newIcon.icon_type = icon_type_os35;
/* printf("OS3.5 Icon Format\n"); */
#ifdef DEBUG
                                append_to_log("Getting OS35 icon details.\n", fullPathAndFile);
#endif
                                getOS35IconSize(fullPathAndFile, &iconSize);
                            }
                            else
                            {
                                newIcon.icon_type = icon_type_standard;
#ifdef DEBUG
                                append_to_log("Seems to be a standard icon\n");
#endif
                                /* printf("Standard Icon Format\n"); */
                                GetStandardIconSize(fullPathAndFile, &iconSize);
                            }

#ifdef DEBUG
                            append_to_log("Icons size x: %d, y: %d\n", iconSize.width, iconSize.height);
#endif

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
#ifdef DEBUG
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
#ifdef DEBUG
                            append_to_log("Icon %s has border: %d\n", fullPathAndFile, newIcon.border_width);
#endif
                            if (newIcon.border_width == 0)
                            {
                                iconArray->hasOnlyBorderlessIcons = TRUE;
                            }

                            newIcon.text_width = textExtent.te_Width;
                            newIcon.text_height = textExtent.te_Height;
                            newIcon.icon_max_width = MAX(iconSize.width + (newIcon.border_width * 2), textExtent.te_Width);
                            newIcon.icon_max_height = iconSize.height + GAP_BETWEEN_ICON_AND_TEXT + textExtent.te_Height;
                            newIcon.icon_x = iconPosition.x;
                            newIcon.icon_y = iconPosition.y;

                            newIcon.is_write_protected = (fib->fib_Protection & FIBF_WRITE) ? TRUE : FALSE;

#ifdef DEBUG
                            append_to_log("Icon is write protected: %d\n", newIcon.is_write_protected);
                            append_to_log("calculated border: %d\n", (newIcon.border_width * 2));
#endif

                            /* Determine if it's a folder or a file */
                            newIcon.is_folder = isDirectory(fullPathAndFile);
#ifdef DEBUG
                            append_to_log("Allocating memory for icon path.\n");

#endif
                            /* Allocate and copy the full path and icon text */
                            newIcon.icon_full_path = (char *)AllocVec(strlen(fullPathAndFile) + 1, MEMF_CLEAR);
                            if (!newIcon.icon_full_path)
                            {
                                fprintf(stderr, "Error: Failed to allocate memory for icon full path.\n");
                                FreeIconArray(iconArray);
                                FreeDosObject(DOS_FIB, fib);
                                return NULL;
                            }
                            strcpy(newIcon.icon_full_path, fullPathAndFile);

                            textLength = strlen(fib->fib_FileName) + 1;
                            newIcon.icon_text = (char *)AllocVec(textLength, MEMF_CLEAR);
                            if (!newIcon.icon_text)
                            {
                                fprintf(stderr, "Error: Failed to allocate memory for icon text.\n");
                                FreeVec(newIcon.icon_full_path);
                                FreeIconArray(iconArray);
                                FreeDosObject(DOS_FIB, fib);
                                return NULL;
                            }
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
                                FreeVec(newIcon.icon_text);
                                FreeVec(newIcon.icon_full_path);
                                FreeIconArray(iconArray);
                                FreeDosObject(DOS_FIB, fib);
                                return NULL;
                            }

                            fileCount++;
                        }
                    }
                }
            }
        }
    }

        /* Set the largest icon width */
        iconArray->BiggestWidthPX = maxWidth;

        /* Free the FileInfoBlock before returning */
        FreeDosObject(DOS_FIB, fib);
#ifdef DEBUG
        append_to_log("Has only borderless icons: %d\n", iconArray->hasOnlyBorderlessIcons);
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
        /* Initial declarations */
        LONG x, y, maxX, maxY, windowWidth, maxWindowWidth;
        IconArray *iconArray;
        BOOL SkipAutoResize;
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

        /* Initialize variables */
        x = ICON_START_X;
        y = ICON_START_Y;
        maxX = 0;
        maxY = 0;
        iconSpacingX = ICON_SPACING_X;
        iconSpacingY = ICON_SPACING_Y;
        SkipAutoResize = FALSE;
        minIconsPerRow = 3;
        rowCount = 0;
        rowStartIndex = 0;

        printf("Tidying folder: %s\n", dirPath);

        iconArray = CreateIconArrayFromPath(lock, dirPath);

        if (iconArray == NULL || iconArray->array == NULL || iconArray->size <= 0)
        {
            // fprintf(stderr, "Error: Failed to create icon array.\n");  //this often is the case if the folder has no icons
            return -1;
        }

        totalIcons = iconArray->size;
#ifdef DEBUG
        append_to_log("************************************\n");
        append_to_log("Screen width %d\n", screenWidth);
        append_to_log("Arranging %d icons in %s\n", totalIcons, dirPath);
        append_to_log("Start of icon tidy routine\n");
#endif

        /* Sort icons by name and folder for a consistent layout */
        qsort(iconArray->array, totalIcons, sizeof(FullIconDetails), CompareByFolderAndName);

        /* Check the largest width of any icon to determine spacing needs */
        largestIconWidth = iconArray->BiggestWidthPX;
#ifdef DEBUG
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
#ifdef DEBUG
            append_to_log("Adjusted max window width to fit screen: %d\n", maxWindowWidth);
#endif
        }
        else
        {
#ifdef DEBUG
            append_to_log("Calculated max window width: %d\n", maxWindowWidth);
#endif
        }

        /* Determine the optimal number of icons per row */
        iconsPerRow = MAX((newWidth - ICON_START_X) / (largestIconWidth + iconSpacingX), minIconsPerRow);
#ifdef DEBUG
        append_to_log("Initial icons per row calculated: %d\n", iconsPerRow);
#endif
        /* Adjust window width to fit these icons evenly */
        windowWidth = ICON_START_X + iconsPerRow * (largestIconWidth + iconSpacingX);
#ifdef DEBUG
        append_to_log("Adjusted initial window width: %d\n", windowWidth);
#endif
        /* Expand window width to fit more icons if necessary and possible */
        while (windowWidth < maxWindowWidth && (totalIcons / iconsPerRow) > (iconsPerRow / 2))
        {
            iconsPerRow++;
            windowWidth = ICON_START_X + iconsPerRow * (largestIconWidth + iconSpacingX);
#ifdef DEBUG
            append_to_log("Expanding window width: %d, Icons per row: %d\n", windowWidth, iconsPerRow);
#endif
        }

        /* Adjust window width to ensure it doesn't exceed maxWindowWidth */
        if (windowWidth > maxWindowWidth)
        {
            windowWidth = maxWindowWidth;
            iconsPerRow = MAX((maxWindowWidth - ICON_START_X) / (largestIconWidth + iconSpacingX), minIconsPerRow);
#ifdef DEBUG
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
#ifdef DEBUG
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
                iconHeight = iconArray->array[i].icon_max_height + borderSpacingForIconsWithNoSpacing;

                removeInfoExtension(iconArray->array[i].icon_full_path, fileNameNoInfo);

                /* Center the icon within the column width and adjust for max row height */
                centerX = ((columnWidths[column] - (iconArray->array[i].icon_width + (+borderSpacingForIconsWithNoSpacing * 2))) / 2);
                iconArray->array[i].icon_x = x + centerX;
                /* Align to the bottom of the row by setting y based on maxRowHeight */
                iconArray->array[i].icon_y = y + (maxRowHeight - iconHeight);
#ifdef DEBUG
                append_to_log("Placing icon %d at (x: %ld, y: %ld) with border of: %dpx, width %d and height %d name: %s\n",

                              i, iconArray->array[i].icon_x, iconArray->array[i].icon_y + borderSpacingForIconsWithNoSpacing, borderSpacingForIconsWithNoSpacing,
                              iconArray->array[i].icon_max_width + borderSpacingForIconsWithNoSpacing, iconArray->array[i].icon_max_height + borderSpacingForIconsWithNoSpacing,
                              iconArray->array[i].icon_text);
#endif

                maxX = MAX(maxX, (iconArray->array[i].icon_x + iconArray->array[i].icon_max_width) + borderSpacingForIconsWithNoSpacing);
                maxY = MAX(maxY, (iconArray->array[i].icon_y + iconArray->array[i].icon_max_height) + borderSpacingForIconsWithNoSpacing);
#ifdef DEBUG
                append_to_log("Updated maxX: %ld, maxY: %ld\n", maxX, maxY);
#endif

                x += columnWidths[column] + iconSpacingX; /* Use the specific width for this column */
            }

            /* Move to the next row */
            x = ICON_START_X;
            y += maxRowHeight + iconSpacingY;
            rowStartIndex += iconsPerRow;
            rowCount++;
#ifdef DEBUG
            append_to_log("Moving to next row: y = %ld, rowStartIndex = %d, rowCount = %d\n", y, rowStartIndex, rowCount);
#endif
        }

        /* Calculate dynamic padding */
        dynamicPaddingY = MAX(10, ICON_START_Y / 2); /* Ensure at least 10 pixels padding */

        /* Adjust maxY to avoid excessive gap at the bottom */
        maxY += dynamicPaddingY; /* Add dynamic padding at the bottom */
#ifdef DEBUG
        append_to_log("Final adjusted maxY with padding: %ld\n", maxY);
        append_to_log("Final maxX: %ld, maxY: %ld\n", maxX, maxY);
#endif
        /* Reposition the window to accommodate all icons neatly */
        if (user_dontResize == FALSE)
            repoistionWindow(dirPath, maxX, maxY);

        /* Save the icon positions to disk */
        saveIconsPositionsToDisk(iconArray);

        FreeIconArray(iconArray);
#ifdef DEBUG

        append_to_log("************************************\n");
        append_to_log("Done. Icons arranged in %d rows with %d icons per row. Path: %s\n",
                      rowCount, iconsPerRow, dirPath); /* Correct the row count display */
#endif
        return 0;
    }

    BOOL checkIconFrame(const char *iconName)
    {
        struct DiskObject *icon = NULL;
        LONG frameStatus;
        LONG errorCode;
        struct Library *IconBase = NULL;
        const char *extension = ".info";
        size_t len = strlen(iconName);
        size_t ext_len = strlen(extension);

        /* Calculate the length of the new icon name without the ".info" extension if present */
        size_t new_len = (len >= ext_len && stricmp(iconName + len - ext_len, extension) == 0) ? len - ext_len : len;

        /* Allocate memory for the new icon name */
        char *newIconName = (char *)malloc((new_len + 1) * sizeof(char));
        if (newIconName == NULL)
        {
            printf("Memory allocation failed\n");
            return FALSE;
        }

        /* Copy the icon name up to the new length and null-terminate it */
        strncpy(newIconName, iconName, new_len);
        newIconName[new_len] = '\0';
#ifdef DEBUG
        append_to_log("Opening icon library\n");
#endif
        /* Open the icon.library */
        IconBase = OpenLibrary("icon.library", 0);
        if (!IconBase)
        {
            printf("Failed to open icon.library\n");
            free(newIconName);
            return TRUE; /* Assume it has a frame if library can't be opened */
        }

        if (IconBase->lib_Version < 44)
        {
#ifdef DEBUG
            append_to_log("icon.library version: %ld.%ld\n", IconBase->lib_Version, IconBase->lib_Revision);
            append_to_log("icon.library version 44 or higher is required to check for frames.\n");
#endif
            CloseLibrary(IconBase);
            free(newIconName);
            return TRUE; /* Assume it has a frame if library version is too low */
        }

        /* Load the icon using the new name without the ".info" extension */
#ifdef DEBUG
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
            return TRUE; /* Assume it has a frame if icon can't be loaded */
        }
#ifdef DEBUG

        append_to_log("Checking to see if it has a border\n");
#endif
        /* Use IconControl to check the frame status of the icon */
        if (IconControl(icon,
                        ICONCTRLA_GetFrameless, &frameStatus,
                        ICONA_ErrorCode, &errorCode,
                        TAG_END) == 1)
        {
            /* A frameStatus of 0 means it has a frame, any other value means it does not */
            BOOL hasFrame = (frameStatus == 0);

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
        return TRUE; /* Default to having a frame if there's an error */
    }

    void dumpIconArrayToScreen(IconArray * iconArray)
    {
        int i;
        append_to_log("Dumping Icon Array of %d icons, max width is %dpx\n", iconArray->size, iconArray->BiggestWidthPX);
        for (i = 0; i < iconArray->size; i++)
        {
            append_to_log("Icon %d: Width = %d, Height = %d, Text Width = %d, Text Height = %d, Max Width = %d, Max Height = %d, x = %d, y = %d, Text = %s, Path = %s\n", i, iconArray->array[i].icon_width, iconArray->array[i].icon_height, iconArray->array[i].text_width, iconArray->array[i].text_height, iconArray->array[i].icon_max_width, iconArray->array[i].icon_max_height, iconArray->array[i].icon_x, iconArray->array[i].icon_y, iconArray->array[i].icon_text, iconArray->array[i].icon_full_path);
        }
    }
