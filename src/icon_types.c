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

#include "main.h"
#include "icon_management.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "utilities.h"

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

    /* Make a copy of the file path to work with */
    filePathLen = strlen(filePath);
    filePathCopy = (char *)AllocVec(filePathLen + 1, MEMF_CLEAR);
    if (!filePathCopy)
    {
        printf("Memory allocation failed.\n");
        return;
    }

    strncpy(filePathCopy, filePath, filePathLen);
    filePathCopy[filePathLen] = '\0'; /* Ensure null termination */

#ifdef DEBUG
    printf("File path copy: %s\n", filePathCopy);
#endif

    /* Remove the ".info" suffix if it exists */
    if (filePathLen > 5 && strcmp(filePathCopy + filePathLen - 5, ".info") == 0)
    {
        filePathCopy[filePathLen - 5] = '\0'; /* Truncate the .info part */
#ifdef DEBUG
        printf(".info suffix removed: %s\n", filePathCopy);
#endif
    }

    /* Load the DiskObject from the file path */
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

    /* Process ToolTypes to find the "IM1=" prefix and get icon size */
    for (i = 0; (toolType = toolTypes[i]); i++)
    {
#ifdef DEBUG
        printf("Processing ToolType: %s\n", toolType);
#endif
        if (strncmp(toolType, prefix, strlen(prefix)) == 0)
        {
            if (strlen(toolType) >= 7)
            {
                newIconSize->width = (int)toolType[4] - '!';
                newIconSize->height = (int)toolType[5] - '!';
#ifdef DEBUG
                printf("Found icon size: width = %d, height = %d\n", newIconSize->width, newIconSize->height);
#endif
            }
            break;
        }
    }

    /* Cleanup */
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
#ifdef DEBUG
    printf("Getting disk Object: %s\n", pathCopy);
#endif

    /* Load the DiskObject from the modified path (without .info) */
    diskObject = GetDiskObject(pathCopy);
    if (diskObject == NULL)
    {
        printf("Failed to load .info file: %s.info\n", pathCopy);
        FreeVec(pathCopy); /* Free the allocated memory */
        return position;
    }
#ifdef DEBUG
    printf("Getting disk position x and y\n", pathCopy);
#endif
    /* Get the current X and Y positions */
    position.x = diskObject->do_CurrentX;
    position.y = diskObject->do_CurrentY;

#ifdef DEBUG
    printf("x: %d and y: %d\n", position.x, position.y);
#endif

    /* Free the DiskObject and the path copy */
    FreeDiskObject(diskObject);
    FreeVec(pathCopy);
#ifdef DEBUG
    printf("Returning from GetIconPositionFromPath\n", pathCopy);
#endif
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
            /* Memory allocation failed */
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
            /* Memory allocation failed */
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
#ifdef DEBUG
                printf("New icon format detected.\n");
#endif
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



#define CHUNK_SIZE 1024
#define HEADER_SIZE 12


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
    int iterationCount = 0;


#ifdef DEBUG
    printf("Checking if it is a OS35 icon: %s\n", filename);
#endif

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

#ifdef DEBUG
    printf("icon size is: %ld\n", fileSize);
#endif

    /* Check if the file size is at least the size of the header */
    if (fileSize < HEADER_SIZE)
    {
        fclose(file);
#ifdef DEBUG
        printf("File too small to be OS3.5 icon\n");
#endif
        return 0; /* File too small to be OS3.5 icon */
    }

    /* Read the file in chunks and search for the headers */
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, file)) > 0)
    {
#ifdef DEBUG
        printf("processing os35 file chunk, bytesRead: %lu, iterationCount: %d, current ftell: %ld\n",
               (unsigned long)bytesRead, iterationCount, ftell(file));
#endif

        for (i = 0; i <= bytesRead - HEADER_SIZE; i++)
        {
            /* Check for "FORM" header */
            if (!foundForm && memcmp(buffer + i, "FORM", 4) == 0)
            {
                foundForm = 1; /* Found "FORM" */
                offset = ftell(file) - bytesRead + i;
#ifdef DEBUG
                printf("\"FORM\" header found at offset: %ld (ftell: %ld, i: %zu, bytesRead: %lu)\n",
                       offset, ftell(file), i, (unsigned long)bytesRead);
#endif
                /* Additional sanity check */
                if (offset < 0 || offset > fileSize)
                {
#ifdef DEBUG
                    printf("Error: Calculated offset out of bounds. offset: %ld, fileSize: %ld\n", offset, fileSize);
#endif
                    fclose(file);
                    return 0; /* Exit if offset calculation is invalid */
                }
            }

            /* If "FORM" was found, look for "ICON" */
            if (foundForm && i + 8 <= bytesRead - 4)
            {
                if (memcmp(buffer + i + 8, "ICON", 4) == 0)
                {
                    foundIcon = 1; /* Found "ICON" */
#ifdef DEBUG
                    printf("\"ICON\" header found after \"FORM\" at position: %d\n", (int)i + 8);
#endif
                    break; /* Break the loop as we've found the required headers */
                }
                else
                {
#ifdef DEBUG
                    printf("Checked position %d, did not find \"ICON\"\n", (int)i + 8);
#endif
                }
            }
        }

        if (foundIcon)
        {
            break; /* Exit the loop once "FORM" and "ICON" are found */
        }

        iterationCount++;
        if (iterationCount > 10000)
        {
#ifdef DEBUG
            printf("Maximum iteration count reached. Exiting loop.\n");
#endif
            break; /* Force exit if maximum iteration count is reached */
        }
    }

    /* Check for errors after fread loop */
    if (ferror(file))
    {
        perror("Error reading file");
        fclose(file);
        return 0;
    }

    /* Close the file */
    fclose(file);

#ifdef DEBUG
    if (!foundForm)
    {
        printf("FORM header not found in the file.\n");
    }
    else if (!foundIcon)
    {
        printf("ICON header not found after FORM header in the file.\n");
    }
#endif

    /* Return true if both "FORM" and "ICON" were found */
    return (foundForm && foundIcon);
}

BOOL IsNewIcon(struct DiskObject *diskObject)
{
    STRPTR *toolTypes;
    BOOL newIconFormat = FALSE;

    toolTypes = diskObject->do_ToolTypes;
#ifdef DEBUG
    printf("Is it a New Icon?\n");
#endif
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
#ifdef DEBUG
        printf("filename: %s\n", filename);
#endif
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
