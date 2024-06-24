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

#include "icon_management.h"

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

    iconArray->hasOnlyBorderlessIcons = TRUE;

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

    if (Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            if (fib->fib_DirEntryType < 0 || fib->fib_DirEntryType > 0) // It's a file or a directory
            {
                if (strcmp(fib->fib_FileName, "Disk.info") != 0)
                {
                    const char *fileExtension = strrchr(fib->fib_FileName, '.');

                    if (fileExtension != NULL && strncasecmp_custom(fileExtension, ".info", 5) == 0)
                    {
                        GetFullPath(dirPath, fib, fullPathAndFile, sizeof(fullPathAndFile));
                        removeInfoExtension(fib->fib_FileName, fileNameNoInfo);
                        CalculateTextExtent(fileNameNoInfo, &textExtent);
                        iconPosition = GetIconPositionFromPath(fullPathAndFile);

                        // Determine icon size based on format
                        if (IsNewIconPath(fullPathAndFile))
                        {
                            // printf("New Icon Format\n");
                            GetNewIconSizePath(fullPathAndFile, &iconSize);
                        }
                        else if (isOS35IconFormat(fullPathAndFile))
                        {
                            // printf("OS3.5 Icon Format\n");
                            getOS35IconSize(fullPathAndFile, &iconSize);
                        }
                        else
                        {
                            // printf("Standard Icon Format\n");
                            GetStandardIconSize(fullPathAndFile, &iconSize);
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

                        newIcon.has_border = checkIconFrame(fullPathAndFile);
                        // printf("Icon %s has border: %d\n", fullPathAndFile, newIcon.has_border);
                        if (newIcon.has_border == 0)
                        {
                            iconArray->hasOnlyBorderlessIcons = FALSE;
                            // printf("!! saved as having an icon with a border\n");
                        }

                        newIcon.text_width = textExtent.te_Width;
                        newIcon.text_height = textExtent.te_Height;
                        newIcon.icon_max_width = MAX(iconSize.width, textExtent.te_Width);
                        newIcon.icon_max_height = iconSize.height + GAP_BETWEEN_ICON_AND_TEXT + textExtent.te_Height;
                        newIcon.icon_x = iconPosition.x;
                        newIcon.icon_y = iconPosition.y;

                        // Determine if it's a folder or a file
                        newIcon.is_folder = (fib->fib_DirEntryType > 0) ? TRUE : FALSE;

                        // Allocate and copy the full path and icon text
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

                        // Update the maximum width
                        if (newIcon.icon_max_width > maxWidth)
                        {
                            maxWidth = newIcon.icon_max_width;
                        }

                        // Add new icon to the array
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

    // Set the largest icon width
    iconArray->BiggestWidthPX = maxWidth;

    // Optional: dump icon array to screen for debugging
    // dumpIconArrayToScreen(iconArray);

    // Free the FileInfoBlock before returning
    FreeDosObject(DOS_FIB, fib);

    printf("Has only borderless icons: %d\n", iconArray->hasOnlyBorderlessIcons);
    return iconArray;
}

void GetNewIconSizePath(const char *filePath, IconSize *newIconSize)
{
    struct DiskObject *diskObject;
    STRPTR *toolTypes;
    STRPTR toolType;
    char *prefix = "IM1=";
    int i;
    char *filePathCopy;
    size_t filePathLen;

    if (!filePath || !newIconSize)
    {
        printf("Invalid filePath or newIconSize pointer.\n");
        return;
    }

    // Make a copy of the file path to work with
    filePathLen = strlen(filePath);
    filePathCopy = (char *)AllocVec(filePathLen + 1, MEMF_CLEAR);
    if (!filePathCopy)
    {
        printf("Memory allocation failed.\n");
        return;
    }

    strncpy(filePathCopy, filePath, filePathLen);

    // Remove the ".info" suffix if it exists
    if (filePathLen > 5 && strcmp(filePathCopy + filePathLen - 5, ".info") == 0)
    {
        filePathCopy[filePathLen - 5] = '\0'; // Truncate the .info part
    }

    // Load the DiskObject from the file path
    diskObject = GetDiskObject(filePathCopy);
    if (!diskObject)
    {
        printf("Failed to get DiskObject for the file: %s\n", filePathCopy);
        FreeVec(filePathCopy);
        return;
    }

    toolTypes = diskObject->do_ToolTypes;
    if (!toolTypes)
    {
        printf("Invalid or missing ToolTypes in DiskObject.\n");
        FreeDiskObject(diskObject);
        FreeVec(filePathCopy);
        return;
    }

    // Process ToolTypes to find the "IM1=" prefix and get icon size
    for (i = 0; (toolType = toolTypes[i]); i++)
    {
        if (strncmp(toolType, prefix, strlen(prefix)) == 0)
        {
            if (strlen(toolType) >= 7)
            {
                newIconSize->width = (int)toolType[4] - '!';
                newIconSize->height = (int)toolType[5] - '!';
               
            }
            break;
        }
    }

    // Cleanup
    FreeDiskObject(diskObject);
    FreeVec(filePathCopy);
}
/* Function to get the standard icon size */
BOOL GetStandardIconSize(const char *filePath, IconSize *iconSize)
{
    struct DiskObject *diskObject;
    char filePathCopy[256]; /* Buffer to hold the modified file path */
    int filePathLength;

    /* Check for NULL pointers */
    if (filePath == NULL || iconSize == NULL)
    {
        return FALSE;
    }

    /* Copy the file path to a local buffer */
    strncpy(filePathCopy, filePath, sizeof(filePathCopy) - 1);
    filePathCopy[sizeof(filePathCopy) - 1] = '\0'; /* Ensure null-termination */
    filePathLength = strlen(filePathCopy);

    /* Remove the ".info" suffix if present */
    if (filePathLength > 5 && strcmp(filePathCopy + filePathLength - 5, ".info") == 0)
    {
        filePathCopy[filePathLength - 5] = '\0';
    }

    /* Attempt to get the DiskObject for the provided (or modified) file path */
    diskObject = GetDiskObject(filePathCopy);
    if (diskObject == NULL)
    {
        return FALSE; /* Failed to get the DiskObject */
    }

    /* Retrieve the width and height from the DiskObject's GfxImage structure */
    iconSize->width = diskObject->do_Gadget.Width;
    iconSize->height = diskObject->do_Gadget.Height;

    /* Free the DiskObject to avoid memory leaks */
    FreeDiskObject(diskObject);

    return TRUE; /* Successfully retrieved and stored the icon size */
}

/* Function to get the X and Y position from an .info file */
IconPosition GetIconPositionFromPath(const char *iconPath)
{
    IconPosition position;         /* Structure to hold X and Y positions */
    char *pathCopy;                /* Copy of the icon path */
    size_t pathLength;             /* Length of the icon path */
    struct DiskObject *diskObject; /* Pointer to the DiskObject structure */

    /* Initialize the IconPosition structure with invalid coordinates */
    position.x = -1;
    position.y = -1;

    /* Determine the length of the provided path */
    pathLength = strlen(iconPath);

    /* Allocate memory for the path copy */
    pathCopy = (char *)AllocVec(pathLength + 1, MEMF_CLEAR);
    if (pathCopy == NULL)
    {
        printf("Failed to allocate memory for path copy.\n");
        return position;
    }

    /* Copy the original icon path to the allocated memory */
    strncpy(pathCopy, iconPath, pathLength);

    /* Ensure the path copy is null-terminated */
    pathCopy[pathLength] = '\0';

    /* Check if the path ends with .info and remove it */
    if (pathLength > 5 && strncasecmp_custom(pathCopy + pathLength - 5, ".info", 5) == 0)
    {
        pathCopy[pathLength - 5] = '\0'; /* Remove the .info extension */
    }

    /* Load the DiskObject from the modified path (without .info) */
    diskObject = GetDiskObject(pathCopy);
    if (diskObject == NULL)
    {
        printf("Failed to load .info file: %s.info\n", pathCopy);
        FreeVec(pathCopy); /* Free the allocated memory */
        return position;
    }

    /* Get the current X and Y positions */
    position.x = diskObject->do_CurrentX;
    position.y = diskObject->do_CurrentY;

    /* Free the DiskObject and the path copy */
    FreeDiskObject(diskObject);
    FreeVec(pathCopy);

    return position;
}


BOOL IsNewIconPath(const STRPTR filePath)
{
    BOOL newIconFormat = FALSE;
    struct DiskObject *diskObject = NULL;
    STRPTR adjustedFilePath = NULL;
    STRPTR *toolTypes;

    /* Check if the provided filepath ends with ".info" */
    size_t len = strlen(filePath);
    if (len >= 5 && strcmp(filePath + len - 5, ".info") == 0)
    {
        /* Allocate memory for the new path without ".info" */
        adjustedFilePath = (STRPTR)AllocVec(len - 4, MEMF_CLEAR);
        if (adjustedFilePath == NULL)
        {
            // Memory allocation failed
            return FALSE;
        }

        /* Create a new string without the ".info" extension */
        strncpy(adjustedFilePath, filePath, len - 5);
        adjustedFilePath[len - 5] = '\0';
    }
    else
    {
        /* Allocate memory for the original path */
        adjustedFilePath = (STRPTR)AllocVec(len + 1, MEMF_CLEAR);
        if (adjustedFilePath == NULL)
        {
            // Memory allocation failed
            return FALSE;
        }

        /* Copy the original filepath */
        strncpy(adjustedFilePath, filePath, len);
        adjustedFilePath[len] = '\0'; /* Ensure null-termination */
    }

    /* Load the DiskObject from the adjusted file path */
    diskObject = GetDiskObject(adjustedFilePath);
    if (diskObject == NULL)
    {
        FreeVec(adjustedFilePath);
        return FALSE;
    }

    /* Get the ToolTypes */
    toolTypes = diskObject->do_ToolTypes;

    /* Check for the specific tool type indicating new icon format */
    if (toolTypes != NULL)
    {
        while (*toolTypes != NULL)
        {

            if (strcmp(*toolTypes, "*** DON'T EDIT THE FOLLOWING LINES!! ***") == 0)
            {
                newIconFormat = TRUE;
                printf("New icon format detected.\n");
                break;
            }

            toolTypes++;
        }
    }

    /* Clean up */
    FreeDiskObject(diskObject);
    FreeVec(adjustedFilePath);

    /* Return the result */
    return newIconFormat;
} /* IsNewIcon */

int isOS35IconFormat(const char *filename)
{
    FILE *file;
    UBYTE buffer[CHUNK_SIZE];
    size_t bytesRead;
    long fileSize;
    int foundForm = 0; /* Flag to indicate if "FORM" has been found */
    int foundIcon = 0; /* Flag to indicate if "ICON" has been found after "FORM" */
    long offset = 0;
    size_t i;

    /* Open the file in binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("filename: %s\n", filename);
        perror("Error opening file");
        return 0; /* Return false on error */
    }

    /* Get the file size */
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Check if the file size is at least the size of the header */
    if (fileSize < HEADER_SIZE)
    {
        fclose(file);
        return 0; /* File too small to be OS3.5 icon */
    }

    /* Read the file in chunks and search for the headers */
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, file)) > 0)
    {
        for (i = 0; i < bytesRead - HEADER_SIZE; i++)
        {
            /* Check for "FORM" header */
            if (!foundForm && memcmp(buffer + i, "FORM", 4) == 0)
            {
                foundForm = 1; /* Found "FORM" */
                offset = ftell(file) - bytesRead + i;
            }

            /* If "FORM" was found, look for "ICON" */
            if (foundForm && memcmp(buffer + i + 8, "ICON", 4) == 0)
            {
                foundIcon = 1; /* Found "ICON" */
                break;         /* Break the loop as we've found the required headers */
            }
        }

        if (foundIcon)
        {
            break; /* Exit the loop once "FORM" and "ICON" are found */
        }
    }

    /* Close the file */
    fclose(file);

    /* Return true if both "FORM" and "ICON" were found */
    return (foundForm && foundIcon);
}

BOOL IsNewIcon(struct DiskObject *diskObject)
{
    STRPTR *toolTypes;
    BOOL newIconFormat = FALSE;

    toolTypes = diskObject->do_ToolTypes;

    /* Print all tooltypes */
    if (toolTypes != NULL)
    {

        while (*toolTypes != NULL)
        {
            if (strcmp(*toolTypes, "*** DON'T EDIT THE FOLLOWING LINES!! ***") == 0)
            {
                newIconFormat = TRUE;
            } /* if */
            toolTypes++;
        } /* while */
    } /* if */

    if (newIconFormat)
    {
        return TRUE;
    } /* if */

    return FALSE;
} /* IsNewIcon */

/* Function to check if the icon file is in the OS3.5 format and return its size */
int getOS35IconSize(const char *filename, IconSize *size)
{
    FILE *file;
    UBYTE buffer[CHUNK_SIZE];
    size_t bytesRead;
    long fileSize;
    int foundForm = 0; /* Flag to indicate if "FORM" has been found */
    int foundIcon = 0; /* Flag to indicate if "ICON" has been found after "FORM" */
    int foundFace = 0; /* Flag to indicate if "FACE" chunk has been found */
    long offset = 0;

    /* Initialize the size */
    size->width = 0;
    size->height = 0;

    /* Open the file in binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("filename: %s\n", filename);
        perror("Error opening file");
        return 0; /* Return false on error */
    }

    /* Get the file size */
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Check if the file size is at least the size of the header */
    if (fileSize < HEADER_SIZE)
    {
        fclose(file);
        return 0; /* File too small to be OS3.5 icon */
    }

    /* Read the file in chunks and search for the headers */
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, file)) > 0)
    {
        int i;

        for (i = 0; i < bytesRead - HEADER_SIZE; i++)
        {
            /* Check for "FORM" header */
            if (!foundForm && memcmp(buffer + i, "FORM", 4) == 0)
            {
                foundForm = 1; /* Found "FORM" */
                offset = ftell(file) - bytesRead + i;
            }

            /* If "FORM" was found, look for "ICON" */
            if (foundForm && memcmp(buffer + i + 8, "ICON", 4) == 0)
            {
                foundIcon = 1; /* Found "ICON" */
            }

            /* If "ICON" was found, look for "FACE" */
            if (foundIcon && memcmp(buffer + i, "FACE", 4) == 0)
            {
                foundFace = 1; /* Found "FACE" chunk */

                /* Read the width and height from the "FACE" chunk */
                size->width = (buffer[i + 8] & 0xFF) + 1;
                size->height = (buffer[i + 9] & 0xFF) + 1;

                break; /* We have what we need, so break the loop */
            }
        }

        if (foundFace)
        {
            break; /* Exit the loop once "FACE" chunk is found */
        }
    }

    /* Close the file */
    fclose(file);

    /* Return true if the "FACE" chunk was found and size was set */
    return foundFace;
}
