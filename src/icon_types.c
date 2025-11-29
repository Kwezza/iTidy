#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __AMIGA__
#include <dos/dosextens.h>
#endif

#include "itidy_types.h"
#include "icon_management.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "Settings/WorkbenchPrefs.h"

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

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
        CONSOLE_ERROR("Invalid filePath or newIconSize pointer.\n");
        return;
    }

    /* Make a copy of the file path to work with */
    filePathLen = strlen(filePath);
    filePathCopy = (char *)whd_malloc(filePathLen + 1);
    if (!filePathCopy)
    {
        CONSOLE_ERROR("Memory allocation failed.\n");
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
        CONSOLE_ERROR("Failed to get DiskObject for the file: %s\n", filePathCopy);
        whd_free(filePathCopy);
        return;
    }

    toolTypes = diskObject->do_ToolTypes;
    if (!toolTypes)
    {
        CONSOLE_WARNING("Invalid or missing ToolTypes in DiskObject.\n");
        FreeDiskObject(diskObject);
        whd_free(filePathCopy);
        return;
    }

    /* Process ToolTypes to find the "IM1=" prefix and get icon size */
    for (i = 0; (toolType = toolTypes[i]); i++)
    {
#ifdef DEBUG
        if (strncmp(toolType, "IM", 2) == 0)
        {
            append_to_log("Found IM tooltype: '%s' (length: %d)\n", toolType, strlen(toolType));
        }
#endif
        if (strncmp(toolType, prefix, strlen(prefix)) == 0)
        {
            if (strlen(toolType) >= 7)
            {
                /* NewIcon format: "IM1=" (4 chars) + Transparency (1 char) + Width (1 char) + Height (1 char) + ...
                 * Position [4] = Transparency ('B' or 'C')
                 * Position [5] = Width character (decode by subtracting '!' which is 0x21 or 33)
                 * Position [6] = Height character (decode by subtracting '!')
                 */
                newIconSize->width = (int)(unsigned char)toolType[5] - '!';
                newIconSize->height = (int)(unsigned char)toolType[6] - '!';
#ifdef DEBUG
                append_to_log("Found icon size from IM1=: width = %d (char '%c'=0x%02X), height = %d (char '%c'=0x%02X)\n", 
                             newIconSize->width, toolType[5], (unsigned char)toolType[5],
                             newIconSize->height, toolType[6], (unsigned char)toolType[6]);
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
BOOL GetIconSizeFromFile(const char *filePath, IconSize *iconSize)
{
    /* VBCC MIGRATION NOTE (Stage 4): Fixed BPTR type for file handle */
    BPTR fileHandle;
    uint8_t buffer[12];  /* Buffer to read the file header */
    int32_t bytesRead;

    /* Open the icon file */
    fileHandle = Open(filePath, MODE_OLDFILE);
    if (fileHandle == 0)
    {
        return FALSE;  /* Failed to open the file */
    }

    /* Read the first 12 bytes (just enough to get the size at 0x08 and 0x0A) */
    bytesRead = Read(fileHandle, buffer, sizeof(buffer));
    if (bytesRead != sizeof(buffer))
    {
        Close(fileHandle);
        return FALSE;  /* Failed to read the necessary bytes */
    }

    /* Extract the width and height from the file at offset 0x08 and 0x0A */
    iconSize->width = (buffer[8] << 8) | buffer[9];
    iconSize->height = (buffer[10] << 8) | buffer[11];
#ifdef DEBUG_MAX
    append_to_log("Hex read of format size: width = %d, height = %d\n", iconSize->width, iconSize->height);
#endif
    Close(fileHandle);
    return TRUE;
}

BOOL GetStandardIconSize(const char *filePath, IconSize *iconSize)
{
    struct DiskObject *diskObject;
    char filePathCopy[256]; /* Buffer to hold the modified file path */
    int filePathLength;
    //int32_t error; /* To store the IoErr() value */

    /* Check for NULL pointers */
    if (filePath == NULL || iconSize == NULL)
    {
        return FALSE;
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
        CONSOLE_ERROR("Failed to allocate memory for path copy.\n");
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
        CONSOLE_ERROR("Unable to load icon. Corrupted or unknown format: %s.info\n", pathCopy);
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

BOOL IsNewIconPath(const STRPTR filePath)
{
    BOOL newIconFormat = FALSE;
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
            return FALSE;
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
            return FALSE;
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
        return FALSE;
    }

    /* Get the ToolTypes */
    toolTypes = diskObject->do_ToolTypes;

    /* Check for the specific tool type indicating new icon format */
    if (toolTypes != NULL)
    {
#ifdef DEBUG
        log_debug(LOG_ICONS, "Checking tooltypes for NewIcon signature...\n");
#endif
        while (*toolTypes != NULL)
        {
#ifdef DEBUG
            log_debug(LOG_ICONS, "  ToolType: '%s'\n", *toolTypes);
#endif
            if (platform_stricmp(*toolTypes, "*** DON'T EDIT THE FOLLOWING LINES!! ***") == 0)
            {
                newIconFormat = TRUE;
#ifdef DEBUG
                log_debug(LOG_ICONS, "  -> NewIcon signature found!\n");
#endif
                break;
            }

            toolTypes++;
        }
#ifdef DEBUG
        if (!newIconFormat)
        {
            log_debug(LOG_ICONS, "  -> No NewIcon signature found (standard icon)\n");
        }
#endif
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
        CONSOLE_ERROR("Error opening file: %s\n", filename);
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
        CONSOLE_ERROR("Error opening file: %s\n", filename);
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
        CONSOLE_ERROR("Error reading file: %s\n", filename);
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
BOOL IsNewIcon(struct DiskObject *diskObject)
{
    char **toolTypes;
    BOOL newIconFormat = FALSE;

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
        CONSOLE_ERROR("Error opening file: %s\n", filename);
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

/**
 * GetIconDetailsFromDisk - Optimized function to read ALL icon details in ONE disk operation
 * 
 * This function performs a SINGLE GetDiskObject() call and extracts all necessary information:
 * - Icon position (X, Y coordinates)
 * - Icon size (width, height)
 * - Icon type detection (Standard, NewIcon, OS3.5)
 * - Frame status (border presence)
 * - Default tool path
 * 
 * This replaces multiple separate disk reads and dramatically improves performance,
 * especially on floppy-based systems.
 * 
 * @param filePath Path to icon file (with or without .info extension)
 * @param details Pointer to IconDetailsFromDisk structure to fill
 * @return TRUE if successful, FALSE on error
 * 
 * NOTE: Caller must free details->defaultTool if not NULL
 */
BOOL GetIconDetailsFromDisk(const char *filePath, IconDetailsFromDisk *details, const char *iconTextForFont)
{
    struct DiskObject *diskObj = NULL;
    char *pathCopy = NULL;
    size_t pathLength;
    BOOL success = FALSE;
    struct TextExtent textExtent;
    
    /* Access global settings and RastPort */
    extern struct WorkbenchSettings prefsWorkbench;
    extern struct RastPort *rastPort;
    
    if (filePath == NULL || details == NULL)
    {
        return FALSE;
    }
    
    /* Initialize the details structure */
    memset(details, 0, sizeof(IconDetailsFromDisk));
    details->position.x = NO_ICON_POSITION;
    details->position.y = NO_ICON_POSITION;
    details->hasFrame = TRUE;  /* Default assumption */
    details->iconType = icon_type_standard;  /* Default assumption */
    
    /* Create a copy of the path and remove .info extension if present */
    pathLength = strlen(filePath);
    pathCopy = (char *)whd_malloc(pathLength + 1);
    if (pathCopy == NULL)
    {
        return FALSE;
    }
    
    strncpy(pathCopy, filePath, pathLength);
    pathCopy[pathLength] = '\0';
    
    /* Remove .info extension if present */
    if (pathLength > 5 && strncasecmp_custom(pathCopy + pathLength - 5, ".info", 5) == 0)
    {
        pathCopy[pathLength - 5] = '\0';
    }
    
    /* ===== SINGLE DISK READ - Get all icon data ===== */
    diskObj = GetDiskObject(pathCopy);
    if (diskObj == NULL)
    {
#ifdef DEBUG
        append_to_log("GetIconDetailsFromDisk: Unable to load icon: %s.info\n", pathCopy);
#endif
        whd_free(pathCopy);
        return FALSE;
    }
    
    /* Extract position */
    details->position.x = diskObj->do_CurrentX;
    details->position.y = diskObj->do_CurrentY;
    
    /* Extract size from gadget structure */
    details->size.width = diskObj->do_Gadget.Width;
    details->size.height = diskObj->do_Gadget.Height;
    
    /* Extract and copy default tool if present */
    if (diskObj->do_DefaultTool != NULL && diskObj->do_DefaultTool[0] != '\0')
    {
        size_t toolLen = strlen(diskObj->do_DefaultTool);
        details->defaultTool = (char *)whd_malloc(toolLen + 1);
        if (details->defaultTool != NULL)
        {
            strcpy(details->defaultTool, diskObj->do_DefaultTool);
#ifdef DEBUG
            log_debug(LOG_ICONS, "GetIconDetailsFromDisk: Found default tool '%s' in %s\n", 
                     diskObj->do_DefaultTool, pathCopy);
#endif
        }
    }
    else
    {
        details->defaultTool = NULL;
#ifdef DEBUG
        log_debug(LOG_ICONS, "GetIconDetailsFromDisk: No default tool in %s\n", pathCopy);
#endif
    }
    
    /* Detect icon type - check for NewIcon format */
    details->isNewIcon = IsNewIcon(diskObj);
    if (details->isNewIcon)
    {
        details->iconType = icon_type_newIcon;
        /* Get actual NewIcon bitmap size from IM1= tooltype */
        /* This fixes size discrepancies where DiskObject gadget size != actual bitmap size */
        GetNewIconSizePath(filePath, &details->size);
    }
    
    /* Check for OS3.5 icon format */
    if (!details->isNewIcon)
    {
        details->isOS35Icon = (isOS35IconFormat(filePath) == 1);
        if (details->isOS35Icon)
        {
            details->iconType = icon_type_os35;
            /* Get more accurate size for OS3.5 icons */
            getOS35IconSize(filePath, &details->size);
        }
    }
    
    /* Determine frame status (only on Amiga with icon.library v44+) */
#if PLATFORM_AMIGA
    /* Only attempt frameless detection if icon.library v44+ is available */
    if (prefsWorkbench.iconLibraryVersion >= 44)
    {
        struct Library *IconBase = OpenLibrary("icon.library", 44);
        if (IconBase != NULL)
        {
            int32_t frameStatus = 0;
            int32_t errorCode = 0;
            
            if (IconControl(diskObj,
                           ICONCTRLA_GetFrameless, &frameStatus,
                           ICONA_ErrorCode, &errorCode,
                           TAG_END) == 1)
            {
                /* frameStatus == 0 means it HAS a frame */
                details->hasFrame = (frameStatus == 0) ? TRUE : FALSE;
            }
            
            CloseLibrary(IconBase);
        }
    }
#endif
    
    /* ===== BATTLE-TESTED SIZE CALCULATIONS ===== */
    /* From icon_management.c lines 550-586 - proven formulas for all border sizes */
    
    /* Step 1: Apply emboss rectangle size to one side */
    if (prefsWorkbench.embossRectangleSize > 0)
    {
        details->iconWithEmboss.width = details->size.width + prefsWorkbench.embossRectangleSize;
        details->iconWithEmboss.height = details->size.height + prefsWorkbench.embossRectangleSize;
    }
    else
    {
        details->iconWithEmboss.width = details->size.width;
        details->iconWithEmboss.height = details->size.height;
    }
    
    /* Step 2: Determine border width based on frame status and icon type */
    if (details->hasFrame || details->iconType == icon_type_standard)
    {
        details->borderWidth = prefsWorkbench.embossRectangleSize;
    }
    else
    {
        details->borderWidth = 0;  /* Frameless icon */
    }
    
    /* Step 3: Calculate visual size - bitmap + borders on ALL FOUR SIDES */
    /* This is the critical fix: emboss adds borderWidth on BOTH left+right and top+bottom */
    details->iconVisualSize.width = details->iconWithEmboss.width + details->borderWidth;
    details->iconVisualSize.height = details->iconWithEmboss.height + details->borderWidth;
    
    /* Step 4: Calculate text size if text provided */
    if (iconTextForFont != NULL && iconTextForFont[0] != '\0' && rastPort != NULL)
    {
        CalculateTextExtent(iconTextForFont, &textExtent);
        details->textSize.width = textExtent.te_Width;
        details->textSize.height = textExtent.te_Height;
        
        /* Step 5: Calculate total display size using proven MAX formula */
        /* Icon + second border OR text, whichever is wider */
        details->totalDisplaySize.width = MAX(details->iconVisualSize.width, details->textSize.width);
        /* Icon + border + gap + text height */
        details->totalDisplaySize.height = details->iconVisualSize.height + GAP_BETWEEN_ICON_AND_TEXT + details->textSize.height;
    }
    else
    {
        /* No text - total display size equals visual icon size */
        details->textSize.width = 0;
        details->textSize.height = 0;
        details->totalDisplaySize.width = details->iconVisualSize.width;
        details->totalDisplaySize.height = details->iconVisualSize.height;
    }
    
    success = TRUE;
    
    /* Cleanup */
    FreeDiskObject(diskObj);
    whd_free(pathCopy);
    
    return success;
}

/**
 * SetIconDefaultTool - Update the default tool for an icon
 * 
 * @param iconPath Path to icon file (with or without .info extension)
 * @param newDefaultTool New default tool path (or NULL to clear)
 * @return TRUE if successful, FALSE on error
 */
BOOL SetIconDefaultTool(const char *iconPath, const char *newDefaultTool)
{
    struct DiskObject *diskObj = NULL;
    char *pathCopy = NULL;
    size_t pathLength;
    BOOL success = FALSE;
    
    if (iconPath == NULL)
    {
        return FALSE;
    }
    
    /* Create a copy of the path and remove .info extension if present */
    pathLength = strlen(iconPath);
    pathCopy = (char *)whd_malloc(pathLength + 1);
    if (pathCopy == NULL)
    {
        return FALSE;
    }
    
    strncpy(pathCopy, iconPath, pathLength);
    pathCopy[pathLength] = '\0';
    
    /* Remove .info extension if present */
    if (pathLength > 5 && strncasecmp_custom(pathCopy + pathLength - 5, ".info", 5) == 0)
    {
        pathCopy[pathLength - 5] = '\0';
    }
    
    /* Read the icon */
    diskObj = GetDiskObject(pathCopy);
    if (diskObj == NULL)
    {
#ifdef DEBUG
        append_to_log("SetIconDefaultTool: Unable to load icon: %s.info\n", pathCopy);
#endif
        whd_free(pathCopy);
        return FALSE;
    }
    
    /* Free old default tool if it exists */
    if (diskObj->do_DefaultTool != NULL)
    {
        /* Note: do_DefaultTool memory is managed by icon.library, don't free it manually */
    }
    
    /* Set new default tool */
    if (newDefaultTool != NULL && newDefaultTool[0] != '\0')
    {
        /* Allocate and copy new tool string */
        size_t toolLen = strlen(newDefaultTool);
        char *newTool = (char *)whd_malloc(toolLen + 1);
        if (newTool != NULL)
        {
            strcpy(newTool, newDefaultTool);
            diskObj->do_DefaultTool = newTool;
        }
        else
        {
            FreeDiskObject(diskObj);
            whd_free(pathCopy);
            return FALSE;
        }
    }
    else
    {
        diskObj->do_DefaultTool = NULL;
    }
    
    /* Write the icon back to disk */
    if (PutDiskObject(pathCopy, diskObj))
    {
        success = TRUE;
#ifdef DEBUG
        append_to_log("SetIconDefaultTool: Updated %s -> %s\n", 
                     pathCopy, newDefaultTool ? newDefaultTool : "(none)");
#endif
    }
    else
    {
#ifdef DEBUG
        append_to_log("SetIconDefaultTool: Failed to write icon: %s.info\n", pathCopy);
#endif
    }
    
    /* Cleanup */
    FreeDiskObject(diskObj);
    whd_free(pathCopy);
    
    return success;
}

/*========================================================================*/
/* Forward Declarations for Tool Cache Functions                         */
/*========================================================================*/
static ToolCacheEntry *SearchToolCache(const char *toolName);
static char *GetToolVersion(const char *filePath);
static ToolCacheEntry *AddToolToCache(const char *toolName, BOOL exists, const char *fullPath, const char *versionString);

/**
 * ValidateDefaultTool - Check if a default tool command/executable exists
 * 
 * This function validates whether a default tool specified in an icon can be found.
 * Uses a cache to avoid repeated disk access for the same tools.
 * 
 * Enhanced with:
 * - Tool cache for performance (avoids redundant lookups)
 * - Version string extraction from executables
 * - Full path tracking
 * 
 * Search Strategy:
 * 1. Check cache first (fast path)
 * 2. If path contains a colon (:) or slash (/), treat as explicit path - check directly
 * 3. Otherwise, treat as command name and search the system PATH
 * 4. Extract version information if found
 * 5. Add result to cache
 * 
 * @param defaultTool The default tool string to validate (e.g., "MultiView" or "SYS:Utilities/More")
 * @return TRUE if the tool exists and is accessible, FALSE otherwise
 * 
 * Examples:
 *   ValidateDefaultTool("MultiView")              -> Searches PATH, caches result with version
 *   ValidateDefaultTool("SYS:Utilities/MultiView") -> Checks exact path
 *   ValidateDefaultTool("InvalidTool")             -> Returns FALSE, caches as missing
 */
BOOL ValidateDefaultTool(const char *defaultTool)
{
#if PLATFORM_AMIGA
    BPTR lock = 0;
    BOOL isExplicitPath = FALSE;
    BOOL toolExists = FALSE;
    char testPath[256];
    char *foundPath = NULL;
    char *versionStr = NULL;
    ToolCacheEntry *cacheEntry;
    int i;
    struct Process *proc;
    APTR oldWindowPtr;
    
    if (defaultTool == NULL || defaultTool[0] == '\0')
    {
        return FALSE;  /* No tool specified */
    }
    
    /* ================================================================
     * DISABLE VOLUME REQUESTERS
     * ================================================================
     * When validating default tools, we may encounter paths to devices
     * that aren't currently mounted (e.g., "DeluxePaintIII:DPaint" when
     * the floppy/CD isn't inserted).
     * 
     * Normally, AmigaDOS Lock() would display a volume requester asking
     * the user to insert the disk. This would:
     * - Block the entire validation process
     * - Require user interaction for every unmounted device
     * - Make processing large directories with old files impractical
     * 
     * By setting pr_WindowPtr to -1, we disable all DOS requesters for
     * this process. Lock() will simply fail and return NULL instead of
     * showing the "Please insert volume..." requester.
     * 
     * This is restored at the end of the function to maintain normal
     * DOS behavior for other operations.
     * ================================================================
     */
    proc = (struct Process *)FindTask(NULL);
    oldWindowPtr = proc->pr_WindowPtr;
    proc->pr_WindowPtr = (APTR)-1;  /* Disable DOS requesters */
    
    /* Check cache first */
    cacheEntry = SearchToolCache(defaultTool);
    if (cacheEntry)
    {
#ifdef DEBUG
        log_debug(LOG_ICONS, "ValidateDefaultTool: Cache hit for '%s' -> %s\n", 
                 defaultTool, cacheEntry->exists ? "EXISTS" : "MISSING");
#endif
        /* Restore DOS requesters before returning */
        proc->pr_WindowPtr = oldWindowPtr;
        return cacheEntry->exists;
    }
    
    /* Not in cache - perform validation */
    
    /* Check if this is an explicit path (contains : or /) */
    if (strchr(defaultTool, ':') != NULL || strchr(defaultTool, '/') != NULL)
    {
        isExplicitPath = TRUE;
    }
    
    if (isExplicitPath)
    {
        /* Explicit path - check directly */
#ifdef DEBUG
        log_debug(LOG_ICONS, "ValidateDefaultTool: Checking explicit path '%s'\n", defaultTool);
#endif
        lock = Lock((CONST_STRPTR)defaultTool, ACCESS_READ);
        if (lock)
        {
            UnLock(lock);
            toolExists = TRUE;
            foundPath = (char *)defaultTool;  /* Use the provided path as-is */
#ifdef DEBUG
            log_debug(LOG_ICONS, "  -> Found at explicit path\n");
#endif
        }
        else
        {
#ifdef DEBUG
            log_debug(LOG_ICONS, "  -> NOT found at explicit path\n");
#endif
        }
    }
    else
    {
        /* Simple command name - search system PATH */
#ifdef DEBUG
        log_debug(LOG_ICONS, "ValidateDefaultTool: Searching PATH for command '%s'\n", defaultTool);
#endif
        
        /* Use global PATH list if available */
        if (g_PathSearchList && g_PathSearchCount > 0)
        {
            for (i = 0; i < g_PathSearchCount && !toolExists; i++)
            {
                /* Build test path */
                snprintf(testPath, sizeof(testPath), "%s%s", g_PathSearchList[i], defaultTool);
                
#ifdef DEBUG
                log_debug(LOG_ICONS, "  -> Trying: %s\n", testPath);
#endif
                
                lock = Lock((CONST_STRPTR)testPath, ACCESS_READ);
                if (lock)
                {
                    UnLock(lock);
                    toolExists = TRUE;
                    foundPath = testPath;
#ifdef DEBUG
                    log_debug(LOG_ICONS, "  -> FOUND at: %s\n", testPath);
#endif
                }
            }
        }
        else
        {
            /* Fallback: PATH list not built, try C: directory only */
            log_warning(LOG_ICONS, "ValidateDefaultTool: PATH list not available, checking C: only\n");
            snprintf(testPath, sizeof(testPath), "C:%s", defaultTool);
            
            lock = Lock((CONST_STRPTR)testPath, ACCESS_READ);
            if (lock)
            {
                UnLock(lock);
                toolExists = TRUE;
                foundPath = testPath;
            }
        }
        
        if (!toolExists)
        {
#ifdef DEBUG
            log_debug(LOG_ICONS, "  -> NOT found in any PATH directory\n");
#endif
        }
    }
    
    /* Extract version if tool was found */
    if (toolExists && foundPath)
    {
        versionStr = GetToolVersion(foundPath);
#ifdef DEBUG
        if (versionStr)
        {
            log_debug(LOG_ICONS, "  -> Version: %s\n", versionStr);
        }
#endif
    }
    
    /* Add to cache */
    AddToolToCache(defaultTool, toolExists, foundPath, versionStr);
    
    /* Free version string (it's been copied into cache) */
    if (versionStr)
    {
        whd_free(versionStr);
    }
    
    /* Restore DOS requesters */
    proc->pr_WindowPtr = oldWindowPtr;
    
    return toolExists;
#else
    /* Host platform stub - assume tool exists */
    (void)defaultTool;
    return TRUE;
#endif
}

/*========================================================================*/
/* PATH Search List Implementation                                       */
/*========================================================================*/

/* Global PATH search list variables */
char **g_PathSearchList = NULL;
int g_PathSearchCount = 0;

/**
 * @brief Build PATH search list from AmigaDOS Process->pr_Path
 * 
 * Reads the system PATH from the current process's CLI structure
 * and builds a global array of directory paths for tool validation.
 * 
 * This implementation:
 * 1. Gets the current process structure
 * 2. Gets the CLI structure from pr_CLI
 * 3. Walks the cli_CommandDir BPTR linked list (the PATH)
 * 4. Uses NameFromLock() to convert each BPTR to a path string
 * 5. Stores paths in a dynamically allocated array
 * 
 * The cli_CommandDir is a BPTR linked list where each node contains:
 *   - Next pointer (BPTR to next node)
 *   - Lock (BPTR lock to the directory)
 * 
 * @return TRUE if PATH list was built successfully, FALSE on error
 */
BOOL BuildPathSearchList(void)
{
#if PLATFORM_AMIGA
    struct Process *proc;
    struct CommandLineInterface *cli;
    BPTR pathPtr;
    BPTR currentPath;
    int count = 0;
    int i;
    char pathBuffer[256];
    BPTR lock;
    
    /* Get current process */
    proc = (struct Process *)FindTask(NULL);
    if (!proc)
    {
        log_error(LOG_GENERAL, "BuildPathSearchList: Failed to get current process\n");
        return FALSE;
    }
    
    /* Get CLI structure - pr_CLI is a BPTR */
    if (!proc->pr_CLI)
    {
        log_warning(LOG_GENERAL, "BuildPathSearchList: Not running from CLI (no pr_CLI)\n");
        log_info(LOG_GENERAL, "BuildPathSearchList: Using C: as default PATH\n");
        
        /* Allocate array for just "C:" */
        g_PathSearchList = (char **)whd_malloc(sizeof(char *));
        if (!g_PathSearchList)
        {
            log_error(LOG_GENERAL, "BuildPathSearchList: Failed to allocate PATH array\n");
            return FALSE;
        }
        memset(g_PathSearchList, 0, sizeof(char *));
        
        g_PathSearchList[0] = (char *)whd_malloc(3);  /* "C:" + null */
        if (!g_PathSearchList[0])
        {
            whd_free(g_PathSearchList);
            g_PathSearchList = NULL;
            log_error(LOG_GENERAL, "BuildPathSearchList: Failed to allocate C: string\n");
            return FALSE;
        }
        memset(g_PathSearchList[0], 0, 3);
        
        strcpy(g_PathSearchList[0], "C:");
        g_PathSearchCount = 1;
        return TRUE;
    }
    
    cli = (struct CommandLineInterface *)BADDR(proc->pr_CLI);
    if (!cli)
    {
        log_error(LOG_GENERAL, "BuildPathSearchList: Failed to get CLI structure\n");
        return FALSE;
    }
    
    /* Get cli_CommandDir (this is the PATH list) */
    pathPtr = cli->cli_CommandDir;
    
    /* First pass: count the number of path entries */
    currentPath = pathPtr;
    while (currentPath)
    {
        /* Convert BPTR to actual pointer */
        struct PathNode {
            BPTR pn_Next;
            BPTR pn_Lock;
        } *pathNode;
        
        pathNode = (struct PathNode *)BADDR(currentPath);
        if (!pathNode)
            break;
            
        count++;
        currentPath = pathNode->pn_Next;
        
        /* Safety limit */
        if (count > 100)
        {
            log_warning(LOG_GENERAL, "BuildPathSearchList: Path list exceeds 100 entries, truncating\n");
            break;
        }
    }
    
    if (count == 0)
    {
        log_info(LOG_GENERAL, "BuildPathSearchList: No PATH entries found, using C: as default\n");
        
        /* Allocate array for just "C:" */
        g_PathSearchList = (char **)whd_malloc(sizeof(char *));
        if (!g_PathSearchList)
        {
            log_error(LOG_GENERAL, "BuildPathSearchList: Failed to allocate PATH array\n");
            return FALSE;
        }
        memset(g_PathSearchList, 0, sizeof(char *));
        
        g_PathSearchList[0] = (char *)whd_malloc(3);  /* "C:" + null */
        if (!g_PathSearchList[0])
        {
            whd_free(g_PathSearchList);
            g_PathSearchList = NULL;
            log_error(LOG_GENERAL, "BuildPathSearchList: Failed to allocate C: string\n");
            return FALSE;
        }
        memset(g_PathSearchList[0], 0, 3);
        
        strcpy(g_PathSearchList[0], "C:");
        g_PathSearchCount = 1;
        log_info(LOG_GENERAL, "BuildPathSearchList: Using default PATH: C:\n");
        return TRUE;
    }
    
    /* Allocate array for path strings */
    g_PathSearchList = (char **)whd_malloc(count * sizeof(char *));
    if (!g_PathSearchList)
    {
        log_error(LOG_GENERAL, "BuildPathSearchList: Failed to allocate PATH array for %d entries\n", count);
        return FALSE;
    }
    memset(g_PathSearchList, 0, count * sizeof(char *));
    
    /* Second pass: extract path strings */
    currentPath = pathPtr;
    i = 0;
    
    log_info(LOG_GENERAL, "BuildPathSearchList: Reading system PATH (%d entries):\n", count);
    
    while (currentPath && i < count)
    {
        struct PathNode {
            BPTR pn_Next;
            BPTR pn_Lock;
        } *pathNode;
        
        pathNode = (struct PathNode *)BADDR(currentPath);
        if (!pathNode)
            break;
        
        /* Get the lock to this directory */
        lock = pathNode->pn_Lock;
        if (lock)
        {
            /* Convert lock to path string */
            if (NameFromLock(lock, pathBuffer, sizeof(pathBuffer)))
            {
                int len = strlen(pathBuffer);
                
                /* Ensure path ends with / or : */
                if (len > 0 && pathBuffer[len - 1] != '/' && pathBuffer[len - 1] != ':')
                {
                    if (len < sizeof(pathBuffer) - 1)
                    {
                        pathBuffer[len] = '/';
                        pathBuffer[len + 1] = '\0';
                        len++;
                    }
                }
                
                /* Allocate and copy path string */
                g_PathSearchList[i] = (char *)whd_malloc(len + 1);
                if (g_PathSearchList[i])
                {
                    memset(g_PathSearchList[i], 0, len + 1);
                    strcpy(g_PathSearchList[i], pathBuffer);
                    log_info(LOG_GENERAL, "  [%d] %s\n", i + 1, pathBuffer);
                    i++;
                }
                else
                {
                    log_error(LOG_GENERAL, "BuildPathSearchList: Failed to allocate string for path %d\n", i);
                }
            }
            else
            {
                log_warning(LOG_GENERAL, "BuildPathSearchList: NameFromLock failed for path entry %d\n", i);
            }
        }
        
        currentPath = pathNode->pn_Next;
    }
    
    g_PathSearchCount = i;
    
    if (g_PathSearchCount == 0)
    {
        log_error(LOG_GENERAL, "BuildPathSearchList: Failed to extract any valid paths\n");
        whd_free(g_PathSearchList);
        g_PathSearchList = NULL;
        return FALSE;
    }
    
    log_info(LOG_GENERAL, "BuildPathSearchList: Successfully built PATH list with %d directories\n", 
             g_PathSearchCount);
    
    return TRUE;
#else
    /* Host platform stub */
    log_info(LOG_GENERAL, "BuildPathSearchList: Not supported on host platform\n");
    return FALSE;
#endif
}

/**
 * @brief Free the PATH search list
 * 
 * Releases all memory allocated by BuildPathSearchList().
 */
void FreePathSearchList(void)
{
#if PLATFORM_AMIGA
    int i;
    
    if (g_PathSearchList)
    {
        /* Free each path string */
        for (i = 0; i < g_PathSearchCount; i++)
        {
            if (g_PathSearchList[i])
            {
                whd_free(g_PathSearchList[i]);
            }
        }
        
        /* Free the array itself */
        whd_free(g_PathSearchList);
        
        g_PathSearchList = NULL;
        g_PathSearchCount = 0;
        
        log_info(LOG_GENERAL, "FreePathSearchList: PATH list freed\n");
    }
#endif
}

/*========================================================================*/
/* Tool Cache Implementation                                             */
/*========================================================================*/

/* Global tool cache variables */
ToolCacheEntry *g_ToolCache = NULL;
int g_ToolCacheCount = 0;
int g_ToolCacheCapacity = 0;

/**
 * @brief Initialize the tool cache with initial capacity
 * 
 * @return TRUE if initialized successfully, FALSE on error
 */
BOOL InitToolCache(void)
{
#if PLATFORM_AMIGA
    g_ToolCacheCapacity = 20;  /* Start with capacity for 20 tools */
    g_ToolCache = (ToolCacheEntry *)whd_malloc(g_ToolCacheCapacity * sizeof(ToolCacheEntry));
    
    if (!g_ToolCache)
    {
        log_error(LOG_GENERAL, "InitToolCache: Failed to allocate cache array\n");
        return FALSE;
    }
    
    memset(g_ToolCache, 0, g_ToolCacheCapacity * sizeof(ToolCacheEntry));
    
    g_ToolCacheCount = 0;
    log_info(LOG_GENERAL, "InitToolCache: Cache initialized (capacity: %d)\n", g_ToolCacheCapacity);
    return TRUE;
#else
    return FALSE;
#endif
}

/**
 * @brief Search for a tool in the cache
 * 
 * Uses case-insensitive comparison since AmigaDOS paths are case-insensitive.
 * Increments the hit count when a match is found.
 * 
 * @param toolName The tool name to search for
 * @return Pointer to cache entry if found, NULL otherwise
 */
static ToolCacheEntry *SearchToolCache(const char *toolName)
{
#if PLATFORM_AMIGA
    int i;
    
    if (!toolName || !g_ToolCache)
        return NULL;
    
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        if (g_ToolCache[i].toolName && platform_stricmp(g_ToolCache[i].toolName, toolName) == 0)
        {
            /* Increment hit counter */
            g_ToolCache[i].hitCount++;
            return &g_ToolCache[i];
        }
    }
#endif
    return NULL;
}

/**
 * @brief Extract version string from an executable file
 * 
 * Reads the embedded version string from an Amiga executable.
 * 
 * @param filePath Full path to the executable
 * @return Allocated string with version info, or NULL if not found (caller must free)
 */
static char *GetToolVersion(const char *filePath)
{
#if PLATFORM_AMIGA
    BPTR file;
    char buffer[512];
    char *versionStr = NULL;
    LONG bytesRead;
    int i;
    const char *versionTag = "$VER:";
    int tagLen = 5;
    
    file = Open((CONST_STRPTR)filePath, MODE_OLDFILE);
    if (!file)
    {
        return NULL;
    }
    
    /* Read file in chunks looking for version string */
    while ((bytesRead = Read(file, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytesRead] = '\0';
        
        /* Search for $VER: tag */
        for (i = 0; i < bytesRead - tagLen; i++)
        {
            if (memcmp(&buffer[i], versionTag, tagLen) == 0)
            {
                /* Found version tag, extract the string */
                char *start = &buffer[i + tagLen];
                char *end = start;
                int len;
                
                /* Skip leading whitespace */
                while (*end == ' ' || *end == '\t')
                    end++;
                start = end;
                
                /* Find end of version string (newline, null, or control char) */
                while (*end && *end != '\n' && *end != '\r' && *end >= 32 && *end < 127)
                    end++;
                
                len = end - start;
                if (len > 0 && len < 200)
                {
                    versionStr = (char *)whd_malloc(len + 1);
                    if (versionStr)
                    {
                        memset(versionStr, 0, len + 1);
                        memcpy(versionStr, start, len);
                        versionStr[len] = '\0';
                    }
                }
                
                Close(file);
                return versionStr;
            }
        }
    }
    
    Close(file);
#endif
    return NULL;
}

/**
 * @brief Add a tool to the cache
 * 
 * @param toolName Simple tool name
 * @param exists TRUE if tool was found
 * @param fullPath Full path to tool (or NULL)
 * @param versionString Version info (or NULL)
 * @return Pointer to new cache entry, or NULL on error
 */
static ToolCacheEntry *AddToolToCache(const char *toolName, BOOL exists, const char *fullPath, const char *versionString)
{
#if PLATFORM_AMIGA
    ToolCacheEntry *newEntry;
    int nameLen, pathLen, verLen;
    
    if (!toolName || !g_ToolCache)
        return NULL;
    
    /* Check if we need to expand the cache */
    if (g_ToolCacheCount >= g_ToolCacheCapacity)
    {
        ToolCacheEntry *newCache;
        int newCapacity = g_ToolCacheCapacity * 2;
        
        newCache = (ToolCacheEntry *)whd_malloc(newCapacity * sizeof(ToolCacheEntry));
        if (!newCache)
        {
            log_error(LOG_GENERAL, "AddToolToCache: Failed to expand cache\n");
            return NULL;
        }
        
        memset(newCache, 0, newCapacity * sizeof(ToolCacheEntry));
        
        /* Copy old entries */
        memcpy(newCache, g_ToolCache, g_ToolCacheCount * sizeof(ToolCacheEntry));
        
        /* Free old cache */
        whd_free(g_ToolCache);
        
        g_ToolCache = newCache;
        g_ToolCacheCapacity = newCapacity;
        
        log_info(LOG_GENERAL, "AddToolToCache: Cache expanded to %d entries\n", newCapacity);
    }
    
    /* Add new entry */
    newEntry = &g_ToolCache[g_ToolCacheCount];
    
    /* Allocate and copy tool name */
    nameLen = strlen(toolName);
    newEntry->toolName = (char *)whd_malloc(nameLen + 1);
    if (!newEntry->toolName)
    {
        return NULL;
    }
    memset(newEntry->toolName, 0, nameLen + 1);
    strcpy(newEntry->toolName, toolName);
    
    newEntry->exists = exists;
    
    /* Allocate and copy full path if provided */
    if (fullPath)
    {
        pathLen = strlen(fullPath);
        newEntry->fullPath = (char *)whd_malloc(pathLen + 1);
        if (newEntry->fullPath)
        {
            memset(newEntry->fullPath, 0, pathLen + 1);
            strcpy(newEntry->fullPath, fullPath);
        }
    }
    else
    {
        newEntry->fullPath = NULL;
    }
    
    /* Allocate and copy version string if provided */
    if (versionString)
    {
        verLen = strlen(versionString);
        newEntry->versionString = (char *)whd_malloc(verLen + 1);
        if (newEntry->versionString)
        {
            memset(newEntry->versionString, 0, verLen + 1);
            strcpy(newEntry->versionString, versionString);
        }
    }
    else
    {
        newEntry->versionString = NULL;
    }
    
    /* Initialize hit count to 0 (first access will increment it) */
    newEntry->hitCount = 0;
    
    /* Initialize file reference tracking */
    newEntry->referencingFiles = NULL;
    newEntry->fileCount = 0;
    newEntry->fileCapacity = 0;
    
    g_ToolCacheCount++;
    
    log_debug(LOG_ICONS, "AddToolToCache: Cached '%s' -> %s [%s] v:%s\n",
             toolName,
             exists ? "EXISTS" : "MISSING",
             fullPath ? fullPath : "(none)",
             versionString ? versionString : "(no version)");
    
    return newEntry;
#else
    return NULL;
#endif
}

/**
 * @brief Add a file reference to a tool cache entry
 * 
 * Tracks which files use a particular default tool. This builds up a list
 * of file paths that reference each tool, up to TOOL_CACHE_MAX_FILES_PER_TOOL.
 * 
 * @param toolName The tool name to add a file reference for
 * @param filePath The file path that uses this tool
 * @return TRUE if added successfully, FALSE on error or if limit reached
 */
BOOL AddFileReferenceToToolCache(const char *toolName, const char *filePath)
{
#if PLATFORM_AMIGA
    ToolCacheEntry *entry;
    char *newFilePath;
    int i, filePathLen;
    
    if (!toolName || !filePath || !g_ToolCache)
        return FALSE;
    
    /* Find the tool in the cache */
    entry = NULL;
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        if (g_ToolCache[i].toolName && strcmp(g_ToolCache[i].toolName, toolName) == 0)
        {
            entry = &g_ToolCache[i];
            break;
        }
    }
    
    if (!entry)
    {
        log_debug(LOG_ICONS, "AddFileReferenceToToolCache: Tool '%s' not found in cache\n", toolName);
        return FALSE;
    }
    
    /* Check if we've reached the limit */
    if (entry->fileCount >= TOOL_CACHE_MAX_FILES_PER_TOOL)
    {
        log_debug(LOG_ICONS, "AddFileReferenceToToolCache: Max file limit reached for '%s'\n", toolName);
        return FALSE;  /* Silently stop adding files */
    }
    
    /* Allocate array if needed */
    if (entry->referencingFiles == NULL)
    {
        entry->fileCapacity = 20;  /* Start with 20, grow as needed */
        entry->referencingFiles = (char **)whd_malloc(entry->fileCapacity * sizeof(char *));
        if (!entry->referencingFiles)
        {
            log_error(LOG_GENERAL, "AddFileReferenceToToolCache: Failed to allocate file array\n");
            return FALSE;
        }
        memset(entry->referencingFiles, 0, entry->fileCapacity * sizeof(char *));
    }
    
    /* Expand array if needed */
    if (entry->fileCount >= entry->fileCapacity)
    {
        char **newArray;
        int newCapacity = entry->fileCapacity * 2;
        
        /* Cap at max files */
        if (newCapacity > TOOL_CACHE_MAX_FILES_PER_TOOL)
            newCapacity = TOOL_CACHE_MAX_FILES_PER_TOOL;
        
        newArray = (char **)whd_malloc(newCapacity * sizeof(char *));
        if (!newArray)
        {
            log_error(LOG_GENERAL, "AddFileReferenceToToolCache: Failed to expand file array\n");
            return FALSE;
        }
        
        memset(newArray, 0, newCapacity * sizeof(char *));
        
        /* Copy existing pointers */
        memcpy(newArray, entry->referencingFiles, entry->fileCount * sizeof(char *));
        
        /* Free old array */
        whd_free(entry->referencingFiles);
        
        entry->referencingFiles = newArray;
        entry->fileCapacity = newCapacity;
    }
    
    /* Allocate and copy file path */
    filePathLen = strlen(filePath);
    newFilePath = (char *)whd_malloc(filePathLen + 1);
    if (!newFilePath)
    {
        log_error(LOG_GENERAL, "AddFileReferenceToToolCache: Failed to allocate file path\n");
        return FALSE;
    }
    memset(newFilePath, 0, filePathLen + 1);
    strcpy(newFilePath, filePath);
    
    /* Add to array */
    entry->referencingFiles[entry->fileCount] = newFilePath;
    entry->fileCount++;
    
    log_debug(LOG_ICONS, "AddFileReferenceToToolCache: Added file '%s' to tool '%s' (%d/%d)\n",
             filePath, toolName, entry->fileCount, TOOL_CACHE_MAX_FILES_PER_TOOL);
    
    return TRUE;
#else
    return FALSE;
#endif
}

/**
 * @brief Dump tool cache contents to log
 */
void DumpToolCache(void)
{
#if PLATFORM_AMIGA
    int i, j;
    
    if (!g_ToolCache || g_ToolCacheCount == 0)
    {
        log_info(LOG_GENERAL, "\n*** Tool Cache: Empty ***\n");
        return;
    }
    
    log_info(LOG_GENERAL, "\n========================================\n");
    log_info(LOG_GENERAL, "Tool Validation Cache Summary\n");
    log_info(LOG_GENERAL, "========================================\n");
    log_info(LOG_GENERAL, "Total tools cached: %d\n", g_ToolCacheCount);
    log_info(LOG_GENERAL, "Cache capacity: %d\n\n", g_ToolCacheCapacity);
    
    log_info(LOG_GENERAL, "Tool Name             | Hits | Status  | Files | Full Path                              | Version\n");
    log_info(LOG_GENERAL, "----------------------|------|---------|-------|----------------------------------------|---------------------------\n");
    
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        log_info(LOG_GENERAL, "%-21s | %4d | %-7s | %5d | %-38s | %s\n",
                g_ToolCache[i].toolName ? g_ToolCache[i].toolName : "(null)",
                g_ToolCache[i].hitCount,
                g_ToolCache[i].exists ? "EXISTS" : "MISSING",
                g_ToolCache[i].fileCount,
                g_ToolCache[i].fullPath ? g_ToolCache[i].fullPath : "(not found)",
                g_ToolCache[i].versionString ? g_ToolCache[i].versionString : "(no version)");
        
        /* If tool has file references, dump them */
        if (g_ToolCache[i].fileCount > 0 && g_ToolCache[i].referencingFiles)
        {
            log_info(LOG_GENERAL, "    Files using this tool:\n");
            for (j = 0; j < g_ToolCache[i].fileCount; j++)
            {
                if (g_ToolCache[i].referencingFiles[j])
                {
                    log_info(LOG_GENERAL, "      [%3d] %s\n", j + 1, g_ToolCache[i].referencingFiles[j]);
                }
            }
            if (g_ToolCache[i].fileCount >= TOOL_CACHE_MAX_FILES_PER_TOOL)
            {
                log_info(LOG_GENERAL, "      (max capacity reached - %d files)\n", TOOL_CACHE_MAX_FILES_PER_TOOL);
            }
            log_info(LOG_GENERAL, "\n");
        }
    }
    
    log_info(LOG_GENERAL, "========================================\n\n");
#endif
}

/**
 * @brief Free the tool cache
 */
void FreeToolCache(void)
{
#if PLATFORM_AMIGA
    int i, j;
    
    if (g_ToolCache)
    {
        /* Free all strings in each entry */
        for (i = 0; i < g_ToolCacheCount; i++)
        {
            if (g_ToolCache[i].toolName)
                whd_free(g_ToolCache[i].toolName);
            if (g_ToolCache[i].fullPath)
                whd_free(g_ToolCache[i].fullPath);
            if (g_ToolCache[i].versionString)
                whd_free(g_ToolCache[i].versionString);
            
            /* Free file references */
            if (g_ToolCache[i].referencingFiles)
            {
                for (j = 0; j < g_ToolCache[i].fileCount; j++)
                {
                    if (g_ToolCache[i].referencingFiles[j])
                        whd_free(g_ToolCache[i].referencingFiles[j]);
                }
                whd_free(g_ToolCache[i].referencingFiles);
            }
        }
        
        /* Free the array itself */
        whd_free(g_ToolCache);
        
        g_ToolCache = NULL;
        g_ToolCacheCount = 0;
        g_ToolCacheCapacity = 0;
        
        log_info(LOG_GENERAL, "FreeToolCache: Tool cache freed\n");
    }
#endif
}
