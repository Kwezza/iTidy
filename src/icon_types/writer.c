/**
 * @file writer.c
 * @brief Icon modification implementation
 * 
 * This module implements functions to modify icon properties and save
 * them back to disk.
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <proto/dos.h>
#include <proto/icon.h>

#include "../itidy_types.h"
#include "../utilities.h"
#include "../writeLog.h"
#include "writer.h"

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

/*========================================================================*/
/* Icon Default Tool Modification                                         */
/*========================================================================*/

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
