/* VBCC MIGRATION NOTE (Stage 2): Modernized for AmigaDOS and VBCC C99
 * 
 * Key changes throughout this file:
 * - Replaced AllocMem/FreeMem with AllocDosObject/FreeDosObject for FileInfoBlock
 * - Replaced sprintf with snprintf for safety
 * - Added proper error handling with IoErr()
 * - Improved lock/unlock consistency
 * - C99 features: inline, //, mixed declarations
 */

#include "platform/platform.h"
#include <devices/trackdisk.h>
#include <devices/timer.h>
#include <clib/timer_protos.h>
#include <proto/wb.h>

#include "file_directory_handling.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "layout_preferences.h"
#include "Settings/WorkbenchPrefs.h"
#include "layout_processor.h"
#include "GUI/StatusWindows/main_progress_window.h"

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>
#include "dos/getDiskDetails.h"
#include "icon_management.h"

#define MAX_DEVICE_NAME_LEN 24  // Define a maximum size for the device name buffer

/* External timer base for performance measurement */
extern struct Device* TimerBase;

void GetFullPath(const char *directory, struct FileInfoBlock *fib, char *fullPath, int fullPathSize)
{
    int dirLen;

    if (directory == NULL || fib == NULL || fullPath == NULL || fullPathSize <= 0)
    {
        return;
    }

    strncpy(fullPath, directory, fullPathSize - 1);
    fullPath[fullPathSize - 1] = '\0';

    dirLen = strlen(fullPath);
    if (dirLen > 0 && fullPath[dirLen - 1] != '/' && fullPath[dirLen - 1] != ':')
    {
        if (dirLen + 1 < fullPathSize)
        {
            strncat(fullPath, "/", fullPathSize - dirLen - 1);
            dirLen++;
        }
    }

    strncat(fullPath, fib->fib_FileName, fullPathSize - dirLen - 1);
    fullPath[fullPathSize - 1] = '\0';
}

int IsRootDirectorySimple(char *path)
{
    size_t length = strlen(path);
    if (length > 0 && path[length - 1] == ':')
    {
        return 1;
    }
    return 0;
}

/* Function to remove the ".info" extension from a string */
void removeInfoExtension(const char *input, char *output)
{
    const char *extension = ".info";
    size_t inputLen = strlen(input);
    size_t extLen = strlen(extension);

    /* Check if the input string ends with the ".info" extension */
    if (inputLen >= extLen && strncasecmp_custom(input + inputLen - extLen, extension, extLen) == 0)
    {
        /* Copy the input string up to but not including the ".info" part */
        strncpy(output, input, inputLen - extLen);
        output[inputLen - extLen] = '\0'; /* Null-terminate the output string */
    }
    else
    {
        /* Copy the entire input string to the output if ".info" is not found */
        strcpy(output, input);
    }
} /* removeInfoExtension */

int saveIconsPositionsToDisk(IconArray *iconArray)
{
    struct DiskObject *diskObject;
    int i;
    char fileNameNoInfo[256];
    int iconArraySize;
    int  is_write_protected_icon, is_delete_protected_icon;
    struct timeval startTime, endTime;
    ULONG elapsedMicros, elapsedMillis;

    int sanityCheckX = 0;
    int sanityCheckY = 0;

    #ifdef DEBUG_MAX
    IconPosition iconPosition; // Only used for debugging
    #endif

    /* Start timing - capture system time */
    if (TimerBase)
    {
        GetSysTime(&startTime);
    }

#ifdef DEBUG
    append_to_log("saveIconsPositionsToDisk: ENTRY, iconArray=%p\n", iconArray);
#endif

    // Precompute the array size to avoid redundant dereferencing
    iconArraySize = iconArray->size;
    
#ifdef DEBUG
    append_to_log("saveIconsPositionsToDisk: iconArraySize=%d\n", iconArraySize);
    
    // Print the header
append_to_log("%-3s | %-4s | %-4s | %-40s\n", "ID", "X", "Y", "Icon");
append_to_log("--------------------------------------------------------------\n");
#endif

    for (i = 0; i < iconArraySize; i++)
    {

        // Access the icon's full path and coordinates once to minimize array lookups
        FullIconDetails *currentIcon = &iconArray->array[i];
        removeInfoExtension(currentIcon->icon_full_path, fileNameNoInfo);

#ifdef DEBUG
        append_to_log("Processing icon %d: %s\n", i, currentIcon->icon_text);
#endif

        /* Temporarily enable write and delete permissions if they are disabled,
           so the icon can be updated. */
        is_write_protected_icon = GetWriteProtection(iconArray->array[i].icon_full_path);
        if (is_write_protected_icon)
            SetWriteProtection(iconArray->array[i].icon_full_path, 0);

        is_delete_protected_icon = GetDeleteProtection(iconArray->array[i].icon_full_path);
        if (is_delete_protected_icon)
            SetDeleteProtection(iconArray->array[i].icon_full_path, 0);

        // Use GetDiskObjectNew() if available for improved performance
        diskObject = GetDiskObject(fileNameNoInfo);

        if (diskObject)
        {

            // Set new icon coordinates unless stripping positions is enabled
            if (user_stripIconPosition == FALSE)
            {
                diskObject->do_CurrentX = currentIcon->icon_x;
                diskObject->do_CurrentY = currentIcon->icon_y;

                sanityCheckX = diskObject->do_CurrentX;
                sanityCheckY = diskObject->do_CurrentY;

#ifdef DEBUG
append_to_log("%-3d | %-4d | %-4d | %-40s\n", i, currentIcon->icon_x, currentIcon->icon_y, currentIcon->icon_text);
#endif
            }
            else
            {
                diskObject->do_CurrentX = NO_ICON_POSITION;
                diskObject->do_CurrentY = NO_ICON_POSITION;
#ifdef DEBUG
                append_to_log("Setting icon position for %s to NO_ICON_POSITION\n", fileNameNoInfo);
#endif
            }

            /* Strip icon borders if enabled (icon.library v44+ only) */
            /* Works on any icon format: classic NewIcons, OS3.5 color icons, or standard icons with borders */
            {
                const LayoutPreferences *prefs = GetGlobalPreferences();
                BOOL borderStripped = FALSE;
                
                if (prefs->stripNewIconBorders && 
                    prefsWorkbench.iconLibraryVersion >= 44)
                {
                    /* Use TagItem array approach as per OS 3.2+ documentation */
                    struct TagItem icTags[2];
                    icTags[0].ti_Tag  = ICONCTRLA_SetFrameless;
                    icTags[0].ti_Data = TRUE;
                    icTags[1].ti_Tag  = TAG_DONE;
                    icTags[1].ti_Data = 0;
                    
                    if (IconControlA(diskObject, icTags) != 0)  /* non-zero = success */
                    {
                        borderStripped = TRUE;
#ifdef DEBUG
                        append_to_log("   Set frameless flag on icon: %s\n", fileNameNoInfo);
#endif
                    }
#ifdef DEBUG
                    else
                    {
                        append_to_log("   Warning: Failed to set frameless flag on: %s\n", fileNameNoInfo);
                    }
#endif
                }
                
                /* Save the updated DiskObject back to disk */
                /* Use PutIconTagList for v44+ to preserve frameless attribute, otherwise use PutDiskObject */
                if (borderStripped && prefsWorkbench.iconLibraryVersion >= 44)
                {
                    /* Notify Workbench of the change - critical for persistence */
                    struct TagItem putTags[2];
                    putTags[0].ti_Tag  = ICONPUTA_NotifyWorkbench;
                    putTags[0].ti_Data = TRUE;
                    putTags[1].ti_Tag  = TAG_DONE;
                    putTags[1].ti_Data = 0;
                    
                    /* Use PutIconTagList to preserve the frameless attribute set by IconControl */
                    if (PutIconTagList(fileNameNoInfo, diskObject, putTags))
                    {
#ifdef DEBUG
                        append_to_log("   Saved OK (with frameless): %s\n", fileNameNoInfo);
#endif
                    }
#ifdef DEBUG
                    else
                    {
                        append_to_log("!! Failed to save icon with PutIconTagList: %s\n", fileNameNoInfo);
                    }
#endif
                }
                else
                {
                    /* Standard save with PutDiskObject */
                    if (!PutDiskObject(fileNameNoInfo, diskObject))
                    {
#ifdef DEBUG
                        append_to_log("!! Failed to save icon %s\n", fileNameNoInfo);
#endif
                    }
#ifdef DEBUG
                    else
                    {
                        append_to_log("   Saved OK: %s\n", fileNameNoInfo);
                    }
#endif
                }
            }

            FreeDiskObject(diskObject);
            /* Reinstate the original write and delete protection if it was modified. */
            if (is_write_protected_icon)
                SetWriteProtection(iconArray->array[i].icon_full_path, 1);
            if (is_delete_protected_icon)
                SetDeleteProtection(iconArray->array[i].icon_full_path, 1);
            
            /* Update heartbeat status for progress feedback during save */
            {
                struct iTidyMainProgressWindow *pw = GetCurrentProgressWindow();
                if (pw != NULL)
                {
                    itidy_main_progress_update_heartbeat(pw, "Saving", i + 1, iconArraySize);
                    /* Also pump events to allow cancellation */
                    itidy_main_progress_window_handle_events(pw);
                    
                    /* Check if user requested cancellation */
                    if (pw->cancel_requested)
                    {
                        log_warning(LOG_GENERAL, "User cancelled during icon saving at %d/%d icons\n", i + 1, iconArraySize);
                        /* Free DiskObject if we have one open */
                        if (diskObject != NULL)
                        {
                            FreeDiskObject(diskObject);
                        }
                        goto save_complete;  /* Skip remaining icons but do cleanup */
                    }
                }
            }
#ifdef DEBUG_MAX
            iconPosition = GetIconPositionFromPath(fileNameNoInfo); // get the current icon position
            if (sanityCheckX != iconPosition.x || sanityCheckY != iconPosition.y)
            {
                append_to_log("!!! Sanity check failed: x: %d and y: %d reported for %s after saving. should have been x: %d and y: %d\n", iconPosition.x, iconPosition.y, fileNameNoInfo, sanityCheckX, sanityCheckY);
            }
            else
            {
                append_to_log("Sanity check ok: x: %d and y: %d reported for %s after saving\n", iconPosition.x, iconPosition.y, fileNameNoInfo);
            }

#endif
        }
        else
        {
            CONSOLE_ERROR("Invalid icon: %s\n", currentIcon->icon_full_path);
        }
    }
    
save_complete:
    /* Clear heartbeat status now that saving is complete */
    {
        struct iTidyMainProgressWindow *pw = GetCurrentProgressWindow();
        if (pw != NULL)
        {
            itidy_main_progress_clear_heartbeat(pw);
        }
    }
    
    /* End timing - calculate elapsed time */
    if (TimerBase)
    {
        GetSysTime(&endTime);
        
        /* Calculate elapsed time in microseconds */
        elapsedMicros = ((endTime.tv_secs - startTime.tv_secs) * 1000000) +
                        (endTime.tv_micro - startTime.tv_micro);
        elapsedMillis = elapsedMicros / 1000;
        
        /* Log performance metrics */
        append_to_log("\n==== ICON SAVING PERFORMANCE ====\n");
        append_to_log("saveIconsPositionsToDisk() execution time:\n");
        append_to_log("  %lu microseconds (%lu.%03lu ms)\n", 
                      elapsedMicros, elapsedMillis, elapsedMicros % 1000);
        append_to_log("  Icons saved: %d\n", iconArraySize);
        if (iconArraySize > 0)
        {
            append_to_log("  Time per icon: %lu microseconds\n", 
                          elapsedMicros / iconArraySize);
        }
        append_to_log("=================================\n\n");
        
        /* Also print to console for immediate visibility */
        CONSOLE_STATUS("  [TIMING] Icon saving: %lu.%03lu ms for %d icons\n",
               elapsedMillis, elapsedMicros % 1000, iconArraySize);
    }
    
    return 0; // Return success
}

void SaveFolderSettings(const char *folderPath, folderWindowSize *newFolderInfo, int sanityCheck)
{
    char diskInfoPath[256]; // Buffer for the disk.info path
    struct DiskObject *diskObject;
    struct DrawerData *drawerData;
    int folderPathLen;
    int is_write_protected, is_delete_protected, is_write_protected_icon, is_delete_protected_icon;

    char folderPathWithInfo[512];

    /* Temporarily enable write and delete permissions if they are disabled,
       so the icon can be updated. */
    is_write_protected = GetWriteProtection(folderPath);
    if (is_write_protected)
        SetWriteProtection(folderPath, 0);

    is_delete_protected = GetDeleteProtection(folderPath);
    if (is_delete_protected)
        SetDeleteProtection(folderPath, 0);

    strcpy(folderPathWithInfo, folderPath);
    strcat(folderPathWithInfo, ".info");

    is_write_protected_icon = GetWriteProtection(folderPathWithInfo);
    if (is_write_protected_icon)
        SetWriteProtection(folderPathWithInfo, 0);

    is_delete_protected_icon = GetDeleteProtection(folderPathWithInfo);
    if (is_delete_protected_icon)
        SetDeleteProtection(folderPathWithInfo, 0);

    

#ifdef DEBUG_MAX
    append_to_log("Saving updated folder size and position for: %s\n", folderPath);

    append_to_log("Is folder write protected? %d\n", is_write_protected);
    append_to_log("Is folder delete protected? %d\n",is_delete_protected);
    append_to_log("Is folder icon write protected? %d\n", is_write_protected_icon);
    append_to_log("Is folder icon delete protected? %d\n", is_delete_protected_icon);
#endif

#ifdef DEBUG_MAX
    append_to_log("endswithinfo output %d\n", endsWithInfo(folderPath));
#endif

    if (!endsWithInfo(folderPath) == 0)
    {
#ifdef DEBUG_MAX
        append_to_log("Aborting - ends with .info: %s\n", folderPath);
#endif
        // return;
    }
    strcpy(diskInfoPath, folderPath);

    // Check if folder path ends with a colon
    folderPathLen = strlen(diskInfoPath);
    if (folderPathLen > 0 && diskInfoPath[folderPathLen - 1] == ':')
    {
        strcat(diskInfoPath, "Disk");
    }

    diskObject = GetDiskObject(diskInfoPath);
    if (diskObject == NULL)
    {
#ifdef DEBUG
        append_to_log("Unable to load disk.info for folder: %s\n", diskInfoPath);
#endif

        return;
    }

    // Modify the dd_ViewModes and dd_Flags to set 'Show only icons' and 'Show all files'
    if (((diskObject->do_Type == WBDRAWER || diskObject->do_Type == WBDISK)) && diskObject->do_DrawerData)
    {

#ifdef DEBUG_MAX
        append_to_log("Existing folder LeftEdge, TopEdge, Width, Height: %d, %d, %d, %d\n",
                      diskObject->do_DrawerData->dd_NewWindow.LeftEdge,
                      diskObject->do_DrawerData->dd_NewWindow.TopEdge,
                      diskObject->do_DrawerData->dd_NewWindow.Width,
                      diskObject->do_DrawerData->dd_NewWindow.Height);
#endif
        drawerData = (struct DrawerData *)diskObject->do_DrawerData;
        drawerData->dd_ViewModes = user_folderViewMode;
        drawerData->dd_Flags = user_folderFlags;
        // Modify the NewWindow structure within DrawerData
        drawerData->dd_NewWindow.LeftEdge = newFolderInfo->left;
        drawerData->dd_NewWindow.TopEdge = newFolderInfo->top;
        drawerData->dd_NewWindow.Width = newFolderInfo->width;
        drawerData->dd_NewWindow.Height = newFolderInfo->height;

        // Save the modified disk.info back to disk
        if (!PutDiskObject(diskInfoPath, diskObject))
        {
            Printf("Failed to save modified info for folder: %s\n", (ULONG)diskInfoPath);
#ifdef DEBUG
            append_to_log("FAILED to Save folder settings for %s\n", diskInfoPath);
#endif
        }
        else
        {
#ifdef DEBUG
            append_to_log("Saved folder settings ok for %s\n", diskInfoPath);
#endif

            // If sanityCheck is true, perform the sanity check
            if (sanityCheck)
            {
                struct DiskObject *reloadedDiskObject;
                struct DrawerData *reloadedDrawerData;

                // Re-load the disk object from disk
                reloadedDiskObject = GetDiskObject(diskInfoPath);
                if (reloadedDiskObject == NULL)
                {
                    // Log that we failed to reload the disk object
#ifdef DEBUG
                    append_to_log("Sanity Check: Failed to reload disk object for %s\n", diskInfoPath);
#endif
                }
                else
                {
                    // Perform the sanity check
                    if ((reloadedDiskObject->do_Type == WBDRAWER || reloadedDiskObject->do_Type == WBDISK) && reloadedDiskObject->do_DrawerData)
                    {
                        reloadedDrawerData = (struct DrawerData *)reloadedDiskObject->do_DrawerData;

                        // Compare the relevant fields
                        if (reloadedDrawerData->dd_NewWindow.LeftEdge == newFolderInfo->left &&
                            reloadedDrawerData->dd_NewWindow.TopEdge == newFolderInfo->top &&
                            reloadedDrawerData->dd_NewWindow.Width == newFolderInfo->width &&
                            reloadedDrawerData->dd_NewWindow.Height == newFolderInfo->height &&
                            reloadedDrawerData->dd_ViewModes == user_folderViewMode &&
                            reloadedDrawerData->dd_Flags == user_folderFlags)
                        {
                            // The settings match
#ifdef DEBUG
                            append_to_log("Sanity Check Passed for %s\n", diskInfoPath);
#endif
                        }
                        else
                        {
                            // The settings do not match, log the differences
#ifdef DEBUG
                            append_to_log("Sanity Check Failed for %s\n", diskInfoPath);
                            append_to_log("Expected LeftEdge: %d, Got: %d\n", newFolderInfo->left, reloadedDrawerData->dd_NewWindow.LeftEdge);
                            append_to_log("Expected TopEdge: %d, Got: %d\n", newFolderInfo->top, reloadedDrawerData->dd_NewWindow.TopEdge);
                            append_to_log("Expected Width: %d, Got: %d\n", newFolderInfo->width, reloadedDrawerData->dd_NewWindow.Width);
                            append_to_log("Expected Height: %d, Got: %d\n", newFolderInfo->height, reloadedDrawerData->dd_NewWindow.Height);
                            append_to_log("Expected ViewModes: %d, Got: %d\n", user_folderViewMode, reloadedDrawerData->dd_ViewModes);
                            append_to_log("Expected Flags: %d, Got: %d\n", user_folderFlags, reloadedDrawerData->dd_Flags);
#endif
                        }
                    }
                    else
                    {
                        // Log that the reloaded disk object does not have the expected type or DrawerData
#ifdef DEBUG
                        append_to_log("Sanity Check: Unexpected disk object type or missing DrawerData for %s\n", diskInfoPath);
#endif
                    }

                    // Free the reloaded disk object
                    FreeDiskObject(reloadedDiskObject);
                }
            }
        }
    }
    else
    {
#ifdef DEBUG
        append_to_log("Requirements not met to update folder. %s\n", folderPath);
#endif
    }
#ifdef DEBUG_MAX
    append_to_log("Folder save completed for %s\n", folderPath);
#endif
    FreeDiskObject(diskObject);
    if (is_write_protected)
        SetWriteProtection(folderPath, 1);
    if (is_delete_protected)
        SetDeleteProtection(folderPath, 1);

    if (is_write_protected_icon)
        SetWriteProtection(folderPathWithInfo, 1);
    if (is_delete_protected_icon)
        SetDeleteProtection(folderPathWithInfo, 1);
}

BOOL GetFolderWindowSettings(const char *folderPath, folderWindowSize *folderInfo, UWORD *viewMode)
{
    char diskInfoPath[256];
    struct DiskObject *diskObject;
    struct DrawerData *drawerData;
    int folderPathLen;
    BOOL result = FALSE;

    if (!folderPath || !folderInfo) {
        return FALSE;
    }

    /* Initialize output to invalid values */
    folderInfo->left = -1;
    folderInfo->top = -1;
    folderInfo->width = -1;
    folderInfo->height = -1;
    if (viewMode) {
        *viewMode = 0;
    }

    log_info(LOG_BACKUP, "Reading folder window settings for: %s", folderPath);

    strcpy(diskInfoPath, folderPath);

    /* Check if folder path ends with a colon (root directory) */
    folderPathLen = strlen(diskInfoPath);
    if (folderPathLen > 0 && diskInfoPath[folderPathLen - 1] == ':')
    {
        strcat(diskInfoPath, "Disk");
    }

    diskObject = GetDiskObject(diskInfoPath);
    if (diskObject == NULL)
    {
        /* Always log this to help diagnose backup issues */
        log_info(LOG_BACKUP, "Unable to load .info file for folder: %s", diskInfoPath);
        return FALSE;
    }

    /* Check if it's a drawer or disk with DrawerData */
    if (((diskObject->do_Type == WBDRAWER || diskObject->do_Type == WBDISK)) && diskObject->do_DrawerData)
    {
        drawerData = (struct DrawerData *)diskObject->do_DrawerData;
        
        /* Read window geometry */
        folderInfo->left = drawerData->dd_NewWindow.LeftEdge;
        folderInfo->top = drawerData->dd_NewWindow.TopEdge;
        folderInfo->width = drawerData->dd_NewWindow.Width;
        folderInfo->height = drawerData->dd_NewWindow.Height;
        
        /* Read view mode if requested */
        if (viewMode) {
            *viewMode = drawerData->dd_ViewModes;
        }

        log_info(LOG_BACKUP, "Folder window settings: LeftEdge=%d, TopEdge=%d, Width=%d, Height=%d, ViewMode=%d",
                 folderInfo->left, folderInfo->top, folderInfo->width, folderInfo->height,
                 viewMode ? *viewMode : 0);
        result = TRUE;
    }
    else
    {
        log_info(LOG_BACKUP, "Not a drawer/disk or missing DrawerData: %s (type=%d)", 
                 diskInfoPath, diskObject->do_Type);
    }

    FreeDiskObject(diskObject);
    return result;
}

void sanitizeAmigaPath(char *path)
{
    ULONG len;
    char *sanitizedPath;
    int i, j;

    if (!path) {
        return;
    }

    len = strlen(path) + 1; // Include null terminator
    
    sanitizedPath = (char *)whd_malloc(len);
    if (!sanitizedPath) {
        // Memory allocation failed
        Printf("Failed to allocate memory for path sanitization\n");
        return;
    }
    memset(sanitizedPath, 0, len);

    i = 0;
    j = 0;
    while (path[i] != '\0') {
        if (path[i] == ':') {
            // Copy the colon
            sanitizedPath[j++] = path[i++];
            // Skip all following slashes
            while (path[i] == '/') {
                i++;
            }
        } else if (path[i] == '/' && path[i + 1] == '/') {
            // Skip redundant slashes
            i++;
        } else {
            // Copy other characters
            sanitizedPath[j++] = path[i++];
        }
    }

    sanitizedPath[j] = '\0'; // Ensure the result is null-terminated

    // VBCC MIGRATION: Use strncpy for safety
    strncpy(path, sanitizedPath, len);
    path[len - 1] = '\0';  // Ensure null termination
    
    // Copy back to the original path and free the allocated memory
    whd_free(sanitizedPath);
}

BOOL GetWriteProtection(const char *filename)
{
    struct FileInfoBlock *fib;
    BPTR lock;
    BOOL isProtected = FALSE;
    if (!does_file_or_folder_exist(filename, 0)) return FALSE;
    /* Lock the file to retrieve its protection bits */
    lock = Lock(filename, ACCESS_READ);
#ifdef DEBUGLocks
    append_to_log("Locking directory (GetWriteProtection): %s\n", filename);
#endif
    if (lock == 0)
    {
        Printf("Error: Unable to lock the file '%s'.\n", filename);
        return FALSE; /* Return false if the file cannot be accessed */
    }

    /* Allocate memory for FileInfoBlock */
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL)
    {
        Printf("Error: Unable to allocate FileInfoBlock.\n");
#ifdef DEBUGLocks
        append_to_log("Unlocking directory: %s\n", filename);
#endif
        UnLock(lock);
        return FALSE;
    }

    /* Examine the file to retrieve its current attributes */
    if (Examine(lock, fib) == DOSFALSE)
    {
        Printf("Error: Examine failed for file '%s'.\n", filename);
        FreeDosObject(DOS_FIB, fib);
#ifdef DEBUGLocks
        append_to_log("Unlocking directory: %s\n", filename);
#endif
        UnLock(lock);
        return FALSE;
    }

    /* Check the write protection bit (bit 2) */
    if (fib->fib_Protection & FIBF_WRITE)
    {
        isProtected = TRUE;
    }

    /* Cleanup */
    FreeDosObject(DOS_FIB, fib);
#ifdef DEBUGLocks
    append_to_log("Unlocking directory: %s\n", filename);
#endif
    UnLock(lock);

    return isProtected;
}

/* Function to set or unset the write protection bit on a file */
void SetWriteProtection(const char *filename, BOOL protect)
{
    struct FileInfoBlock *fib;
    BPTR lock;
    if (!does_file_or_folder_exist(filename, 0)) return;
    /* Lock the file to retrieve its protection bits */
    lock = Lock(filename, ACCESS_READ);
#ifdef DEBUGLocks
    append_to_log("Locking directory (SetWriteProtection): %s\n", filename);
#endif
    if (lock == 0)
    {
        Printf("Error: Unable to lock the file '%s'.\n", filename);
        return;
    }

    /* Allocate memory for FileInfoBlock */
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL)
    {
        Printf("Error: Unable to allocate FileInfoBlock.\n");
        UnLock(lock);
#ifdef DEBUGLocks
        append_to_log("Unlocking directory: %s\n", filename);
#endif
        return;
    }

    /* Examine the file to retrieve its current attributes */
    if (Examine(lock, fib) == DOSFALSE)
    {
        Printf("Error: Examine failed for file '%s'.\n", filename);
        FreeDosObject(DOS_FIB, fib);
#ifdef DEBUGLocks
        append_to_log("Unlocking directory: %s\n", filename);
#endif
        UnLock(lock);
        return;
    }

    /* Set or unset the write protection bit (bit 2) */
    if (protect)
    {
        fib->fib_Protection |= FIBF_WRITE; /* Set write-protection */
    }
    else
    {
        fib->fib_Protection &= ~FIBF_WRITE; /* Unset write-protection */
    }

    /* Apply the new protection bits */
    SetProtection(filename, fib->fib_Protection);

    /* Cleanup */
    FreeDosObject(DOS_FIB, fib);
#ifdef DEBUGLocks
    append_to_log("Unlocking directory: %s\n", filename);
#endif
    UnLock(lock);
}

/* Function to get the delete protection status of a file */
BOOL GetDeleteProtection(const char *filename)
{
    struct FileInfoBlock *fib;
    BPTR lock;
    BOOL isProtected = FALSE;
    if (!does_file_or_folder_exist(filename, 0)) return FALSE;
    /* Lock the file to retrieve its protection bits */
    lock = Lock(filename, ACCESS_READ);
#ifdef DEBUGLocks
    append_to_log("Locking directory (GetDeleteProtection): %s\n", filename);
#endif
    if (lock == 0)
    {
        Printf("Error: Unable to lock the file '%s'.\n", filename);
        return FALSE; /* Return false if the file cannot be accessed */
    }

    /* Allocate memory for FileInfoBlock */
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL)
    {
        Printf("Error: Unable to allocate FileInfoBlock.\n");
#ifdef DEBUGLocks
        append_to_log("Unlocking directory: %s\n", filename);
#endif
        UnLock(lock);
        return FALSE;
    }

    /* Examine the file to retrieve its current attributes */
    if (Examine(lock, fib) == DOSFALSE)
    {
        Printf("Error: Examine failed for file '%s'.\n", filename);
        FreeDosObject(DOS_FIB, fib);
#ifdef DEBUGLocks
        append_to_log("Unlocking directory: %s\n", filename);
#endif
        UnLock(lock);
        return FALSE;
    }

    /* Check the delete protection bit (bit 0) */
    if (fib->fib_Protection & FIBF_DELETE)
    {
        isProtected = TRUE;
    }

    /* Cleanup */
    FreeDosObject(DOS_FIB, fib);
#ifdef DEBUGLocks
    append_to_log("Unlocking directory: %s\n", filename);
#endif
    UnLock(lock);

    return isProtected;
}

/* Function to set or unset the delete protection bit on a file */
void SetDeleteProtection(const char *filename, BOOL protect)
{
    struct FileInfoBlock *fib;
    BPTR lock;

    if (!does_file_or_folder_exist(filename, 0)) return;

    /* Lock the file to retrieve its protection bits */
    lock = Lock(filename, ACCESS_READ);
#ifdef DEBUGLocks
    append_to_log("Locking directory (SetDeleteProtection): %s\n", filename);
#endif
    if (lock == 0)
    {
        Printf("Error: Unable to lock the file '%s'.\n", filename);
        return;
    }

    /* Allocate memory for FileInfoBlock */
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL)
    {
        Printf("Error: Unable to allocate FileInfoBlock.\n");
#ifdef DEBUGLocks
        append_to_log("Unlocking directory: %s\n", filename);
#endif
        UnLock(lock);
        return;
    }

    /* Examine the file to retrieve its current attributes */
    if (Examine(lock, fib) == DOSFALSE)
    {
        Printf("Error: Examine failed for file '%s'.\n", filename);
        FreeDosObject(DOS_FIB, fib);
#ifdef DEBUGLocks
        append_to_log("Unlocking directory: %s\n", filename);
#endif
        UnLock(lock);
        return;
    }

    /* Set or unset the delete protection bit (bit 0) */
    if (protect)
    {
        fib->fib_Protection |= FIBF_DELETE; /* Set delete-protection */
    }
    else
    {
        fib->fib_Protection &= ~FIBF_DELETE; /* Unset delete-protection */
    }

    /* Apply the new protection bits */
    SetProtection(filename, fib->fib_Protection);

    /* Cleanup */
    FreeDosObject(DOS_FIB, fib);
#ifdef DEBUGLocks
    append_to_log("Unlocking directory: %s\n", filename);
#endif
    UnLock(lock);
}

/* Function to check if a given path is a folder, removing ".info" if present */
BOOL isDirectory(const char *path)
{
    struct FileInfoBlock *fib;
    BPTR lock;
    BOOL result;
    char *pathWithoutInfo;
    const char *infoExtension;
    size_t pathLen;
    size_t infoLen;

    /* Initialize variables */
    result = FALSE;
    pathWithoutInfo = NULL;
    infoExtension = ".info";
    pathLen = strlen(path);
    infoLen = strlen(infoExtension);

    /* Allocate a FileInfoBlock */
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL)
    {
        CONSOLE_ERROR("Unable to allocate FileInfoBlock.\n");
        return FALSE;
    }

    /* Check if the path ends with ".info" and remove it */
    if (pathLen > infoLen && strncasecmp_custom(path + pathLen - infoLen, infoExtension, infoLen) == 0)
    {
        /* Allocate memory for the new path without ".info" */
        pathWithoutInfo = (char *)malloc(pathLen - infoLen + 1);
        if (pathWithoutInfo == NULL)
        {
            CONSOLE_ERROR("Unable to allocate memory for pathWithoutInfo.\n");
            FreeDosObject(DOS_FIB, fib);
            return FALSE;
        }

        /* Copy the path minus the ".info" */
        strncpy(pathWithoutInfo, path, pathLen - infoLen);
        pathWithoutInfo[pathLen - infoLen] = '\0'; /* Null-terminate the new string */
    }
    else
    {
        /* If no ".info" extension, just use the original path */
        pathWithoutInfo = (char *)malloc(pathLen + 1);
        if (pathWithoutInfo == NULL)
        {
            CONSOLE_ERROR("Unable to allocate memory for pathWithoutInfo.\n");
            FreeDosObject(DOS_FIB, fib);
            return FALSE;
        }
        strcpy(pathWithoutInfo, path); /* Copy the original path */
    }

    /* Lock the file/directory */
    lock = Lock(pathWithoutInfo, ACCESS_READ);
#ifdef DEBUGLocks
    append_to_log("Locking directory (isDirectory): %s\n", path);
#endif
    if (lock == 0)
    {
#ifdef DEBUG
        append_to_log("Error: Unable to lock path: %s\n", pathWithoutInfo);
#endif
        FreeDosObject(DOS_FIB, fib);
        free(pathWithoutInfo);
        return FALSE;
    }

    /* Examine the lock to populate the FileInfoBlock */
    if (Examine(lock, fib))
    {
        /* Check if it is a directory */
        if (fib->fib_DirEntryType > 0) /* Directories have positive fib_DirEntryType */
        {
            result = TRUE;
        }
    }
    else
    {
        CONSOLE_ERROR("Examine() failed.\n");
    }

    /* Unlock the path and free the FileInfoBlock */
#ifdef DEBUGLocks
    append_to_log("Unlocking directory: %s\n", path);
#endif
    UnLock(lock);
    FreeDosObject(DOS_FIB, fib);
    free(pathWithoutInfo); /* Free the allocated memory */

    return result;
}

BOOL IsDeviceReadOnly(const char *deviceName) {
    BPTR lock;
    struct InfoData infoData;
    BOOL isReadOnly = FALSE;

    // Attempt to lock the device with a shared lock
    lock = Lock(deviceName, SHARED_LOCK);
    if (lock) {
        // Get disk information
        if (Info(lock, &infoData)) {
            // Check if the disk state indicates write-protection
            if (infoData.id_DiskState == ID_WRITE_PROTECTED) {
                isReadOnly = TRUE;  // Disk is write-protected
            } else {
                isReadOnly = FALSE;  // Disk is not write-protected
            }
        } else {
            // Info() failed; assume not read-only unless proven otherwise
            isReadOnly = FALSE;
        }
        UnLock(lock);
    } else {
        // Lock failed (e.g., device not present); treat as not read-only
        isReadOnly = FALSE;
    }

    return isReadOnly;
}