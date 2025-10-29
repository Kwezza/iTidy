/* VBCC MIGRATION NOTE (Stage 2): Modernized for AmigaDOS and VBCC C99
 * 
 * Key changes throughout this file:
 * - Replaced AllocMem/FreeMem with AllocDosObject/FreeDosObject for FileInfoBlock
 * - Replaced sprintf with snprintf for safety
 * - Added proper error handling with IoErr()
 * - Improved lock/unlock consistency
 * - C99 features: inline, //, mixed declarations
 */

#include <devices/trackdisk.h>
#include <devices/timer.h>
#include <clib/timer_protos.h>

#include "file_directory_handling.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "dos/getDiskDetails.h"
#include "icon_management.h"

#define MAX_DEVICE_NAME_LEN 24  // Define a maximum size for the device name buffer

/* External timer base for performance measurement */
extern struct Device* TimerBase;

int HasSlaveFile(char *path)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    int hasSlave = 0;
    int fileCount = 0;

#ifdef DEBUG
    append_to_log("DEBUG: HasSlaveFile ENTRY: path='%s'\n", path);
#endif
#ifdef DEBUGLocks
    append_to_log("Locking directory (HasSlaveFile): %s\n", path);
#endif

    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock) {
#ifdef DEBUG
        append_to_log("DEBUG: HasSlaveFile - Lock failed for '%s'\n", path);
#endif
        // Assume it has no slave file if we can't lock the directory
        return 0;
    }

    // VBCC MIGRATION: Use AllocDosObject instead of AllocMem
    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib) {
        Printf("Failed to allocate FileInfoBlock\n");
#ifdef DEBUG
        append_to_log("DEBUG: HasSlaveFile - AllocDosObject failed\n");
#endif
#ifdef DEBUGLocks
        append_to_log("Unlocking directory: %s\n", path);
#endif
        UnLock(lock);
        return 0;
    }

#ifdef DEBUG
    append_to_log("DEBUG: HasSlaveFile - calling Examine()\n");
#endif
    if (Examine(lock, fib)) {
#ifdef DEBUG
        append_to_log("DEBUG: HasSlaveFile - Examine() successful, starting ExNext() loop\n");
#endif
        while (ExNext(lock, fib)) {
            fileCount++;
            if (fib->fib_DirEntryType < 0) {  // File, not directory
                const char *ext = strrchr(fib->fib_FileName, '.');
                if (ext && strncasecmp_custom(ext, ".slave", 6) == 0) {
#ifdef DEBUG
                    append_to_log("DEBUG: HasSlaveFile - Found .slave file: '%s'\n", fib->fib_FileName);
#endif
                    hasSlave = 1;
                    break;
                }
            }
        }
#ifdef DEBUG
        append_to_log("DEBUG: HasSlaveFile - ExNext() loop completed, checked %d entries\n", fileCount);
#endif
    } else {
#ifdef DEBUG
        append_to_log("DEBUG: HasSlaveFile - Examine() failed\n");
#endif
    }

    // VBCC MIGRATION: Use FreeDosObject instead of FreeMem
    FreeDosObject(DOS_FIB, fib);
#ifdef DEBUGLocks
    append_to_log("Unlocking directory: %s\n", path);
#endif
    UnLock(lock);
    return hasSlave;
}

void ProcessDirectory(char *path, BOOL processSubDirs, int recursion_level)
{
    BPTR lock = 0;  // VBCC MIGRATION: BPTR initialized to 0, not NULL
    struct FileInfoBlock *fib = NULL;
    char subdir[4096];

#ifdef DEBUG
    append_to_log("ProcessDirectory ENTRY: path='%s', processSubDirs=%d, recursion_level=%d\n", 
                  path, processSubDirs, recursion_level);
#endif

    // Sanitize the path to ensure it's properly formatted for the Amiga file system
    sanitizeAmigaPath(path);
    
#ifdef DEBUG
    append_to_log("After sanitizeAmigaPath: path='%s'\n", path);
#endif

#ifdef DEBUGLocks
    // Log the current directory and recursion level for debugging purposes
    append_to_log("Locking directory at level %d: %s\n", recursion_level, path);
#endif

    // Attempt to lock the directory for reading
    lock = Lock((STRPTR)path, ACCESS_READ);
    
#ifdef DEBUG
    append_to_log("Lock() returned: %ld\n", (LONG)lock);
#endif
    
    if (!lock) {
        // If locking fails, log error and return
        LONG error = IoErr();
#ifdef DEBUG
        append_to_log("Failed to lock %s at level %d, error: %ld\n", path, recursion_level, error);
#endif
        return;
    }

#ifdef DEBUG
    append_to_log("Lock successful, checking WHDLoad cleanup flag: %d\n", user_cleanupWHDLoadFolders);
#endif

    // If WHDLoad cleanup is enabled, check for .slave files in the directory
    if (user_cleanupWHDLoadFolders == TRUE) {
#ifdef DEBUG
        append_to_log("Calling HasSlaveFile('%s')\n", path);
#endif
        if (HasSlaveFile(path)) {
            // Resize the folder to fit its contents if resizing is allowed
            if (user_dontResize == FALSE) {
                IconArray *tempIconArray = CreateIconArrayFromPath(lock, path);
                if (tempIconArray) {
                    resizeFolderToContents(path, tempIconArray);
                    FreeIconArray(tempIconArray);
                }
            }
#ifdef DEBUGLocks
            append_to_log("Unlocking directory at level %d: %s\n", recursion_level, path);
#endif
            // Always unlock the directory before returning
            UnLock(lock);
            return;
        }
    }

    // VBCC MIGRATION: Use AllocDosObject for FileInfoBlock
    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib) {
        // Log a message if memory allocation fails
        Printf("Failed to allocate FileInfoBlock at level %d\n", recursion_level);
#ifdef DEBUGLocks
        append_to_log("Unlocking directory at level %d: %s\n", recursion_level, path);
#endif
        // Unlock the directory before returning
        UnLock(lock);
        return;
    }

    // Examine the directory to check for contents
    if (Examine(lock, fib)) {
        // Format the icons for the current directory
        FormatIconsAndWindow(path);

        // If processing subdirectories is allowed, handle them recursively
        if (processSubDirs == TRUE) {
            while (ExNext(lock, fib)) {
                if (fib->fib_DirEntryType > 0) { // Process only directories
                    // VBCC MIGRATION: Use snprintf for safety
                    // Check if path ends with ':' (root device) - if so, don't add '/'
                    if (path[strlen(path) - 1] == ':') {
                        snprintf(subdir, sizeof(subdir), "%s%s", path, fib->fib_FileName);
                    } else {
                        snprintf(subdir, sizeof(subdir), "%s/%s", path, fib->fib_FileName);
                    }
                    // Recursively process the subdirectory and increment the recursion level
                    ProcessDirectory(subdir, TRUE, recursion_level + 1);
                }
            }
        }
    } else {
        // Log an error message if directory examination fails
        LONG error = IoErr();
        Printf("Failed to examine directory at level %d: %s (error: %ld)\n", 
               recursion_level, path, error);
    }

    // Cleanup: Free allocated memory and unlock the directory
    // VBCC MIGRATION: Use FreeDosObject instead of FreeMem
    FreeDosObject(DOS_FIB, fib);
#ifdef DEBUGLocks
    append_to_log("Unlocking directory at level %d: %s\n", recursion_level, path);
#endif
    UnLock(lock);
}

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

            // Save the updated DiskObject back to disk
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

            FreeDiskObject(diskObject);
            /* Reinstate the original write and delete protection if it was modified. */
            if (is_write_protected_icon)
                SetWriteProtection(iconArray->array[i].icon_full_path, 1);
            if (is_delete_protected_icon)
                SetDeleteProtection(iconArray->array[i].icon_full_path, 1);
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
            fprintf(stderr, "Error: Invalid icon: %s\n", currentIcon->icon_full_path);
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
        printf("  [TIMING] Icon saving: %lu.%03lu ms for %d icons\n",
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

#ifdef DEBUG_MAX
    append_to_log("Reading folder window settings for: %s\n", folderPath);
#endif

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
#ifdef DEBUG
        append_to_log("Unable to load .info file for folder: %s\n", diskInfoPath);
#endif
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

#ifdef DEBUG_MAX
        append_to_log("Folder window settings: LeftEdge=%d, TopEdge=%d, Width=%d, Height=%d, ViewMode=%d\n",
                      folderInfo->left, folderInfo->top, folderInfo->width, folderInfo->height,
                      viewMode ? *viewMode : 0);
#endif
        result = TRUE;
    }
    else
    {
#ifdef DEBUG
        append_to_log("Not a drawer/disk or missing DrawerData: %s\n", diskInfoPath);
#endif
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
    
    // VBCC MIGRATION: AllocVec is already correct here
    sanitizedPath = (char *)AllocVec(len, MEMF_CLEAR);
    if (!sanitizedPath) {
        // Memory allocation failed
        Printf("Failed to allocate memory for path sanitization\n");
        return;
    }

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
    FreeVec(sanitizedPath);
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
        printf("Error: Unable to allocate FileInfoBlock.\n");
        return FALSE;
    }

    /* Check if the path ends with ".info" and remove it */
    if (pathLen > infoLen && strncasecmp_custom(path + pathLen - infoLen, infoExtension, infoLen) == 0)
    {
        /* Allocate memory for the new path without ".info" */
        pathWithoutInfo = (char *)malloc(pathLen - infoLen + 1);
        if (pathWithoutInfo == NULL)
        {
            printf("Error: Unable to allocate memory for pathWithoutInfo.\n");
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
            printf("Error: Unable to allocate memory for pathWithoutInfo.\n");
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
        printf("Error: Examine() failed.\n");
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