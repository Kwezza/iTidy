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
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"

void GetNewIconSizePath(const char *filePath, IconSize *newIconSize)
{
    struct DiskObject *diskObject;
    char **toolTypes;
    char *toolType;
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
    filePathCopy = (char *)whd_malloc(filePathLen + 1);
    if (!filePathCopy)
    {
        printf("Memory allocation failed.\n");
        return;
    }

    memset(filePathCopy, 0, filePathLen + 1);
    strncpy(filePathCopy, filePath, filePathLen);
    filePathCopy[filePathLen] = '\0'; /* Ensure null termination */

#ifdef DEBUG
    append_to_log("File path copy: %s\n", filePathCopy);
#endif

    /* Remove the ".info" suffix if it exists */
    if (filePathLen > 5 && platform_stricmp(filePathCopy + filePathLen - 5, ".info") == 0)
    {
        filePathCopy[filePathLen - 5] = '\0'; /* Truncate the .info part */
#ifdef DEBUG_MAX
        append_to_log(".info suffix removed: %s\n", filePathCopy);
#endif
    }

    /* Load the DiskObject from the file path */
    diskObject = GetDiskObject(filePathCopy);
    if (!diskObject)
    {
        printf("Failed to get DiskObject for the file: %s\n", filePathCopy);
        whd_free(filePathCopy);
        return;
    }

    toolTypes = diskObject->do_ToolTypes;
    if (!toolTypes)
    {
        printf("Invalid or missing ToolTypes in DiskObject.\n");
        FreeDiskObject(diskObject);
        whd_free(filePathCopy);
        return;
    }

    /* Process ToolTypes to find the "IM1=" prefix and get icon size */
    for (i = 0; (toolType = toolTypes[i]); i++)
    {
#ifdef DEBUG_MAX
        append_to_log("Processing ToolType: %s\n", toolType);
#endif
        if (strncmp(toolType, prefix, strlen(prefix)) == 0)
        {
            if (strlen(toolType) >= 7)
            {
                newIconSize->width = (int)toolType[4] - '!';
                newIconSize->height = (int)toolType[5] - '!';
#ifdef DEBUG
                append_to_log("Found icon size: width = %d, height = %d\n", newIconSize->width, newIconSize->height);
#endif
            }
            break;
        }
    }

    /* Cleanup */
    FreeDiskObject(diskObject);
    whd_free(filePathCopy);
}

/* Function to read the icon size directly from the file */
bool GetIconSizeFromFile(const char *filePath, IconSize *iconSize)
{
    /* VBCC MIGRATION NOTE (Stage 4): Fixed BPTR type for file handle */
    BPTR fileHandle;
    uint8_t buffer[12];  /* Buffer to read the file header */
    int32_t bytesRead;

    /* Open the icon file */
    fileHandle = Open(filePath, MODE_OLDFILE);
    if (fileHandle == 0)
    {
        return false;  /* Failed to open the file */
    }

    /* Read the first 12 bytes (just enough to get the size at 0x08 and 0x0A) */
    bytesRead = Read(fileHandle, buffer, sizeof(buffer));
    if (bytesRead != sizeof(buffer))
    {
        Close(fileHandle);
        return false;  /* Failed to read the necessary bytes */
    }

    /* Extract the width and height from the file at offset 0x08 and 0x0A */
    iconSize->width = (buffer[8] << 8) | buffer[9];
    iconSize->height = (buffer[10] << 8) | buffer[11];
#ifdef DEBUG_MAX
    append_to_log("Hex read of format size: width = %d, height = %d\n", iconSize->width, iconSize->height);
#endif
    Close(fileHandle);
    return true;
}

bool GetStandardIconSize(const char *filePath, IconSize *iconSize)
{
    struct DiskObject *diskObject;
    char filePathCopy[256]; /* Buffer to hold the modified file path */
    int filePathLength;
    //int32_t error; /* To store the IoErr() value */

    /* Check for NULL pointers */
    if (filePath == NULL || iconSize == NULL)
    {
        return false;
    }

    /* Copy the file path to a local buffer */
    strncpy(filePathCopy, filePath, sizeof(filePathCopy) - 1);
    filePathCopy[sizeof(filePathCopy) - 1] = '\0'; /* Ensure null-termination */
    filePathLength = strlen(filePathCopy);

    /* Remove the ".info" suffix if present (case-insensitive check) */
    if (filePathLength > 5 && platform_stricmp(filePathCopy + filePathLength - 5, ".info") == 0)
    {
        filePathCopy[filePathLength - 5] = '\0';
    }

#ifdef DEBUG_MAX
    append_to_log("Attempting to load DiskObject from path: %s\n", filePathCopy);
#endif
    /* Attempt to get the DiskObject for the provided (or modified) file path */
    diskObject = GetDiskObject(filePathCopy);
    if (diskObject == NULL)
    {
        #ifdef DEBUG
        /* VBCC MIGRATION NOTE (Stage 4): Added error variable for debug logging */
        LONG error;
        append_to_log("Failed to get DiskObject for the file: %s\n", filePathCopy);
        /* Use IoErr() to get more information about why it failed */
        error = IoErr();
        append_to_log("GetDiskObject failed. IoErr: %ld\n", error);
        #endif
        return false; /* Failed to get the DiskObject */
    }

    /* Retrieve the width and height from the DiskObject's GfxImage structure */
    iconSize->width = diskObject->do_Gadget.Width;
    iconSize->height = diskObject->do_Gadget.Height;

    /* Free the DiskObject to avoid memory leaks */
    FreeDiskObject(diskObject);

    return true; /* Successfully retrieved and stored the icon size */
}

/* Function to get the X and Y position from an .info file */
IconPosition GetIconPositionFromPath(const char *iconPath)
{
    IconPosition position;         /* Structure to hold X and Y positions */
    char *pathCopy;                /* Copy of the icon path */
    size_t pathLength;             /* Length of the icon path */
    struct DiskObject *diskObject; /* Pointer to the DiskObject structure */

    if(does_file_or_folder_exist(iconPath,0) == false)
    {
        position.x = -1;
        position.y = -1;
        return position;
    }

    /* Initialize the IconPosition structure with invalid coordinates */
    position.x = -1;
    position.y = -1;

    /* Determine the length of the provided path */
    pathLength = strlen(iconPath);

    /* Allocate memory for the path copy */
    pathCopy = (char *)whd_malloc(pathLength + 1);
    if (pathCopy == NULL)
    {
        printf("Failed to allocate memory for path copy.\n");
        return position;
    }

    memset(pathCopy, 0, pathLength + 1);
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
        printf("Error: Unable to load icon. Corrupted or unknown format: %s.info\n", pathCopy);
        whd_free(pathCopy); /* Free the allocated memory */
        return position;
    }
    /* Get the current X and Y positions */
    position.x = diskObject->do_CurrentX;
    position.y = diskObject->do_CurrentY;

#ifdef DEBUG_MAX
    append_to_log("Icon position x: %d and y: %d for %s\n", position.x, position.y,iconPath);
#endif

    /* Free the DiskObject and the path copy */
    FreeDiskObject(diskObject);
    whd_free(pathCopy);

    return position;
}

bool IsNewIconPath(const char *filePath)
{
    bool newIconFormat = false;
    struct DiskObject *diskObject = NULL;
    char *adjustedFilePath = NULL;
    char **toolTypes;

    /* Check if the provided filepath ends with ".info" */
    size_t len = strlen(filePath);
    if (len >= 5 && platform_stricmp(filePath + len - 5, ".info") == 0)
    {
        /* Allocate memory for the new path without ".info" */
        adjustedFilePath = (char *)whd_malloc(len - 4);
        if (adjustedFilePath == NULL)
        {
            /* Memory allocation failed */
            return false;
        }

        memset(adjustedFilePath, 0, len - 4);
        /* Create a new string without the ".info" extension */
        strncpy(adjustedFilePath, filePath, len - 5);
        adjustedFilePath[len - 5] = '\0';
    }
    else
    {
        /* Allocate memory for the original path */
        adjustedFilePath = (char *)whd_malloc(len + 1);
        if (adjustedFilePath == NULL)
        {
            /* Memory allocation failed */
            return false;
        }

        memset(adjustedFilePath, 0, len + 1);
        /* Copy the original filepath */
        strncpy(adjustedFilePath, filePath, len);
        adjustedFilePath[len] = '\0'; /* Ensure null-termination */
    }

    /* Load the DiskObject from the adjusted file path */
    diskObject = GetDiskObject(adjustedFilePath);
    if (diskObject == NULL)
    {
        whd_free(adjustedFilePath);
        return false;
    }

    /* Get the ToolTypes */
    toolTypes = diskObject->do_ToolTypes;

    /* Check for the specific tool type indicating new icon format */
    if (toolTypes != NULL)
    {
        while (*toolTypes != NULL)
        {

            if (platform_stricmp(*toolTypes, "*** DON'T EDIT THE FOLLOWING LINES!! ***") == 0)
            {
                newIconFormat = true;
#ifdef DEBUG
                append_to_log("New icon format detected.\n");
#endif
                break;
            }

            toolTypes++;
        }
    }

    /* Clean up */
    FreeDiskObject(diskObject);
    whd_free(adjustedFilePath);

    /* Return the result */
    return newIconFormat;
} /* IsNewIconPath */



#define CHUNK_SIZE 1024
#define HEADER_SIZE 12


int isIconTypeDisk(const char *filename,long fib_DirEntryType)
{
    FILE *file;
    uint8_t ic_Type;
    long fileSize;


if (fib_DirEntryType > 0) return 0;

    /* Open the file in binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Error opening file");
        return 0; /* Return false on error */
    }

    /* Get the file size */
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Check if the file size is at least large enough to contain the ic_Type */
    if (fileSize < 0x31) /* 0x30 is the offset of ic_Type, so the file must be at least this size */
    {
        fclose(file);
        return 0; /* File too small */
    }

    /* Seek to the ic_Type field (offset 0x30) */
    fseek(file, 0x30, SEEK_SET);

    /* Read the ic_Type byte */
    if (fread(&ic_Type, 1, 1, file) != 1)
    {
        fclose(file);
        return 0; /* Error reading the file */
    }

    /* Close the file */
    fclose(file);

    /* Check if ic_Type is DISK (1) */
    if (ic_Type == 1) /* 1 corresponds to DISK */
    {
        return 1; /* Return true */
    }

    return 0; /* Return false if it's not DISK */
}

int isOS35IconFormat(const char *filename)
{
    FILE *file;
    uint8_t buffer[CHUNK_SIZE];
    size_t bytesRead;
    long fileSize;
    int foundForm = 0; /* Flag to indicate if "FORM" has been found */
    int foundIcon = 0; /* Flag to indicate if "ICON" has been found after "FORM" */
    long offset = 0;
    size_t i;
    int iterationCount = 0;
    int lastBytes = 0; /* For handling overlap between buffer reads */

#ifdef DEBUG_MAX
    append_to_log("Checking if it is a OS35 icon: %s\n", filename);
#endif

    /* Open the file in binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        perror("Error opening file");
        return 0; /* Return false on error */
    }

    /* Get the file size */
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

#ifdef DEBUG_MAX
    append_to_log("icon size is: %ld\n", fileSize);
#endif

    /* Check if the file size is at least the size of the header */
    if (fileSize < HEADER_SIZE)
    {
        fclose(file);
#ifdef DEBUG
        append_to_log("File too small to be OS3.5 icon\n");
#endif
        return 0; /* File too small to be OS3.5 icon */
    }

    /* Read the file in chunks and search for the headers */
    while ((bytesRead = fread(buffer + lastBytes, 1, CHUNK_SIZE - lastBytes, file)) > 0)
    {
        bytesRead += lastBytes; /* Adjust for any leftover bytes from the last read */

#ifdef DEBUG
        //append_to_log("Processing OS35 file chunk, bytesRead: %lu, iterationCount: %d, current ftell: %ld\n",
        //       (unsigned long)bytesRead, iterationCount, ftell(file));
#endif

        for (i = 0; i <= bytesRead - HEADER_SIZE; i++)
        {
            /* Check for "FORM" header */
            if (!foundForm && memcmp(buffer + i, "FORM", 4) == 0)
            {
                foundForm = 1;
                offset = ftell(file) - bytesRead + i;

#ifdef DEBUG_MAX
                append_to_log("\"FORM\" header found at offset: %ld (ftell: %ld, i: %zu, bytesRead: %lu)\n",
                       offset, ftell(file), i, (unsigned long)bytesRead);
#endif

                /* Additional sanity check */
                if (offset < 0 || offset > fileSize)
                {
#ifdef DEBUG_MAX
                    append_to_log("Error: Calculated offset out of bounds. offset: %ld, fileSize: %ld\n", offset, fileSize);
#endif
                    fclose(file);
                    return 0;
                }
            }

            /* If "FORM" was found, look for "ICON" */
            if (foundForm && i + 8 <= bytesRead - 4)
            {
                if (memcmp(buffer + i + 8, "ICON", 4) == 0)
                {
                    foundIcon = 1;
#ifdef DEBUG_MAX
                    append_to_log("\"ICON\" header found after \"FORM\" at position: %d\n", (int)(i + 8));
#endif
                    break;
                }
#ifdef DEBUG_MAX
                else
                {
                    append_to_log("Checked position %d, did not find \"ICON\"\n", (int)(i + 8));
                }
#endif
            }
        }

        if (foundIcon)
        {
            break; /* Exit the loop once both "FORM" and "ICON" are found */
        }

        /* Handle buffer overlap */
        lastBytes = HEADER_SIZE; /* Keep last few bytes in case header is split */
        memcpy(buffer, buffer + CHUNK_SIZE - lastBytes, lastBytes);

        iterationCount++;
        if (iterationCount > 10000)
        {
#ifdef DEBUG_MAX
            append_to_log("Maximum iteration count reached. Exiting loop.\n");
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

#ifdef DEBUG_MAX
    if (!foundForm)
    {
        append_to_log("FORM header not found in the file.\n");
    }
    else if (!foundIcon)
    {
        append_to_log("ICON header not found after FORM header in the file.\n");
    }
#endif

    /* Return true if both "FORM" and "ICON" were found */
    return (foundForm && foundIcon);
}
bool IsNewIcon(struct DiskObject *diskObject)
{
    char **toolTypes;
    bool newIconFormat = false;

    toolTypes = diskObject->do_ToolTypes;
#ifdef DEBUG_MAX
    append_to_log("Is it a New Icon?\n");
#endif
    /* Print all tooltypes */
    if (toolTypes != NULL)
    {

        while (*toolTypes != NULL)
        {
            if (platform_stricmp(*toolTypes, "*** DON'T EDIT THE FOLLOWING LINES!! ***") == 0)
            {
                newIconFormat = true;
            } /* if */
            toolTypes++;
        } /* while */
    } /* if */

    if (newIconFormat)
    {
        return true;
    } /* if */

    return false;
} /* IsNewIcon */

int getOS35IconSize(const char *filename, IconSize *size)
{
    FILE *file;
    uint8_t buffer[CHUNK_SIZE];
    size_t bytesRead;
    long fileSize;
    int foundForm = 0; /* Flag to indicate if "FORM" has been found */
    int foundIcon = 0; /* Flag to indicate if "ICON" has been found after "FORM" */
    int foundFace = 0; /* Flag to indicate if "FACE" chunk has been found */
    long offset = 0;
    int lastBytes = 0; /* For handling overlap between buffer reads */

    /* Initialize the size */
    size->width = 0;
    size->height = 0;

    /* Open the file in binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
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
    while ((bytesRead = fread(buffer + lastBytes, 1, CHUNK_SIZE - lastBytes, file)) > 0)
    {
        int i;
        bytesRead += lastBytes; /* Adjust for any leftover bytes */

        for (i = 0; i < bytesRead - HEADER_SIZE; i++)
        {
            /* Check for "FORM" header */
            if (!foundForm && memcmp(buffer + i, "FORM", 4) == 0)
            {
                foundForm = 1;
                offset = ftell(file) - bytesRead + i;
            }

            /* If "FORM" was found, look for "ICON" */
            if (foundForm && !foundIcon && memcmp(buffer + i + 4, "ICON", 4) == 0)
            {
                foundIcon = 1;
            }

            /* If "ICON" was found, look for "FACE" */
            if (foundIcon && memcmp(buffer + i, "FACE", 4) == 0)
            {
                foundFace = 1;

                /* Read the width and height from the "FACE" chunk */
                size->width = (buffer[i + 8] & 0xFF) + 1;
                size->height = (buffer[i + 9] & 0xFF) + 1;

                break; /* We have what we need, so break the loop */
            }
        }

        /* Handle buffer overlap */
        lastBytes = HEADER_SIZE; /* Keep last few bytes in case header is split */
        memcpy(buffer, buffer + CHUNK_SIZE - lastBytes, lastBytes);

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
