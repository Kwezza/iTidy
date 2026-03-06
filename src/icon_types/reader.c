/**
 * @file reader.c
 * @brief Optimized icon metadata reader implementation
 * 
 * This module implements the critical GetIconDetailsFromDisk() function which
 * performs a SINGLE disk read to extract all icon metadata. This is a major
 * performance optimization, especially on floppy-based systems.
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
#include <dos/dostags.h>
#include <workbench/workbench.h>
#include <proto/graphics.h>

#include "../itidy_types.h"
#include "../utilities.h"
#include "../writeLog.h"
#include "../icon_misc.h"
#include "../Settings/WorkbenchPrefs.h"
#include "icon_types_internal.h"
#include "format.h"
#include "reader.h"

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

/*========================================================================*/
/* Optimized Icon Details Reader                                          */
/*========================================================================*/

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
 * @param iconTextForFont Icon text label for font size calculation (or NULL)
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
    
    /* Extract Workbench icon type (WBPROJECT, WBTOOL, WBDRAWER, etc.) */
    details->workbenchType = diskObj->do_Type;
    
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
