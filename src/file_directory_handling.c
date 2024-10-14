#include "file_directory_handling.h"
#include "utilities.h"
#include "spinner.h"

int HasSlaveFile(char *path)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    int hasSlave = 0;

    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock == 0)
    {
        Printf("Failed to lock directory: %s\n", path);
        return 0;
    }

    fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_PUBLIC | MEMF_CLEAR);
    if (fib == NULL)
    {
        Printf("Failed to allocate memory for FileInfoBlock\n");
        UnLock(lock);
        return 0;
    }

    if (Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            if (fib->fib_DirEntryType < 0)
            {
                const char *ext = strrchr(fib->fib_FileName, '.');
                if (ext && strncasecmp_custom(ext, ".slave", 6) == 0)
                {
                    hasSlave = 1;
                    break;
                }
            }
        }
    }

    FreeMem(fib, sizeof(struct FileInfoBlock));
    UnLock(lock);
    return hasSlave;
}

void ProcessDirectory(char *path, BOOL processSubDirs)
{

    BPTR lock;
    struct FileInfoBlock *fib;
    char subdir[4096];
    sanitizeAmigaPath(path);
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock == 0)
    {
        Printf("Failed to lock directory: %s\n", path);
        return;
    }

    if (user_cleanupWHDLoadFolders == TRUE)
    {
        if (HasSlaveFile(path))
        {
            if (user_dontResize == FALSE)
                resizeFolderToContents(path, CreateIconArrayFromPath(lock, path));
            UnLock(lock);
            return;
        }
    }

    fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_PUBLIC | MEMF_CLEAR);
    if (fib == NULL)
    {
        Printf("Failed to allocate memory for FileInfoBlock\n");
        UnLock(lock);
        return;
    }

    if (Examine(lock, fib))
    {
        FormatIconsAndWindow(path);
        if (processSubDirs == TRUE)
        {
            while (ExNext(lock, fib))
            {
                if (fib->fib_DirEntryType > 0)
                {
                    sprintf(subdir, "%s/%s", path, fib->fib_FileName);
                    ProcessDirectory(subdir, TRUE);
                }
            }
        }
    }

    FreeMem(fib, sizeof(struct FileInfoBlock));
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
    int is_write_protected, is_delete_protected, is_write_protected_icon, is_delete_protected_icon;

    // Pre-calculate the size to avoid multiple dereferences
    iconArraySize = iconArray->size;

    for (i = 0; i < iconArraySize; i++)
    {

        // Access the icon's full path and coordinates once to reduce array access
        FullIconDetails *currentIcon = &iconArray->array[i];
        updateCursor(); /* update progress spinner */
        removeInfoExtension(currentIcon->icon_full_path, fileNameNoInfo);

        is_write_protected_icon = GetWriteProtection(iconArray->array[i].icon_full_path);
        if (is_write_protected_icon)
            SetWriteProtection(iconArray->array[i].icon_full_path, 0);

        is_delete_protected_icon = GetDeleteProtection(iconArray->array[i].icon_full_path);
        if (is_delete_protected_icon)
            SetDeleteProtection(iconArray->array[i].icon_full_path, 0);

/*
        if (does_file_or_folder_exist(fileNameNoInfo, 0) == 1)
        {
            is_write_protected = GetWriteProtection(fileNameNoInfo);
            if (is_write_protected)
                SetWriteProtection(fileNameNoInfo, 0);

            is_delete_protected = GetDeleteProtection(fileNameNoInfo);
            if (is_delete_protected)
                SetDeleteProtection(fileNameNoInfo, 0);
        }
        /*
                if(currentIcon->is_write_protected)
                {
                    setWriteProtection(currentIcon->icon_full_path, 0);
                    setWriteProtection(fileNameNoInfo, 0);
                }
        */

        // Use GetDiskObjectNew() for better performance if available
        diskObject = GetDiskObject(fileNameNoInfo);

        if (diskObject)
        {

            // Set the new positions
            if (user_stripIconPosition == FALSE)
            {
                diskObject->do_CurrentX = currentIcon->icon_x;
                diskObject->do_CurrentY = currentIcon->icon_y;
            }
            else
            {
                diskObject->do_CurrentX = NO_ICON_POSITION;
                diskObject->do_CurrentY = NO_ICON_POSITION;
            }

            // Save the updated DiskObject back to disk
            if (!PutDiskObject(fileNameNoInfo, diskObject))
            {
                fprintf(stderr, "Error: Failed to save icon position for file: %s\n", fileNameNoInfo);
                FreeDiskObject(diskObject);
            }

            // Always free the DiskObject to prevent memory leaks!
            
            /*
            printf("icon_full_path: %s\n", iconArray->array[i].icon_full_path);
            printf("is_write_protected: %d\n", is_write_protected);
            printf("is_delete_protected: %d\n", is_delete_protected);
            printf("is_write_protected_icon: %d\n", is_write_protected_icon);
            printf("is_delete_protected_icon: %d\n", is_delete_protected_icon);
            */

            if (is_write_protected_icon)
                SetWriteProtection(iconArray->array[i].icon_full_path, 1);
            if (is_delete_protected_icon)
                SetDeleteProtection(iconArray->array[i].icon_full_path, 1);
            if (does_file_or_folder_exist(fileNameNoInfo, 0) == 1)
            {
                //if (is_write_protected)
                //    SetWriteProtection(fileNameNoInfo, 1);
                //if (is_delete_protected)
                //    SetDeleteProtection(fileNameNoInfo, 1);
            }

            // restore the write protection (if needed)
            /*
            if(currentIcon->is_write_protected)
            {
                setWriteProtection(currentIcon->icon_full_path, 1);
                setWriteProtection(fileNameNoInfo, 1);

            }
            */
        }
        else
        {
            fprintf(stderr, "Error: Failed to get DiskObject for file: %s\n", currentIcon->icon_full_path);
        }
    }
    return 0; // Return success or an error code as needed
}

void SaveFolderSettings(const char *folderPath, folderWindowSize *newFolderInfo)
{
    char diskInfoPath[256]; // Buffer for the disk.info path
    struct DiskObject *diskObject;
    struct DrawerData *drawerData;
    int folderPathLen;

    if (endsWithInfo(folderPath) == 0)
        return;

    strcpy(diskInfoPath, folderPath);

    // Check if folder path ends with a colon or slash
    folderPathLen = strlen(diskInfoPath);
    if (folderPathLen > 0 && diskInfoPath[folderPathLen - 1] == ':')
    {
        strcat(diskInfoPath, "Disk");
    }

    diskObject = GetDiskObject(diskInfoPath);
    if (diskObject == NULL)
    {
        // Printf("Unable to load disk.info for folder: %s\n", (ULONG)folderPath);
        return;
    }

    // Modify the dd_ViewModes and dd_Flags to set 'Show only icons' and 'Show all files'
    if ((diskObject->do_Type == WBDRAWER || diskObject->do_Type == WBDISK) && diskObject->do_DrawerData)
    {

#ifdef DEBUG
        printf("Exisiting LeftEdge, TopEdge, Width, Height: %d, %d, %d, %d\n", diskObject->do_DrawerData->dd_NewWindow.LeftEdge, diskObject->do_DrawerData->dd_NewWindow.TopEdge, diskObject->do_DrawerData->dd_NewWindow.Width, diskObject->do_DrawerData->dd_NewWindow.Height);
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
        }
    }

    FreeDiskObject(diskObject);
}

void sanitizeAmigaPath(char *path)
{
    ULONG len;
    char *sanitizedPath;
    int i, j;

    if (path == NULL)
    {
        return;
    }

    len = strlen(path) + 1; /* Include null terminator*/
    sanitizedPath = (char *)AllocVec(len, MEMF_ANY | MEMF_CLEAR);
    if (sanitizedPath == NULL)
    {
        /* Memory allocation failed */
        return;
    }

    i = 0;
    j = 0;
    while (path[i] != '\0')
    {
        if (path[i] == ':')
        {
            /* Copy the colon */
            sanitizedPath[j++] = path[i++];
            /* Skip all following slashes */
            while (path[i] == '/')
            {
                i++;
            }
        }
        else if (path[i] == '/' && path[i + 1] == '/')
        {
            /* Skip redundant slashes */
            i++;
        }
        else
        {
            /* Copy other characters */
            sanitizedPath[j++] = path[i++];
        }
    }

    sanitizedPath[j] = '\0'; /* Ensure the result is null-terminated */

    /* Copy back to the original path and free the allocated memory */
    strcpy(path, sanitizedPath);
    FreeVec(sanitizedPath);
}

BOOL GetWriteProtection(STRPTR filename)
{
    struct FileInfoBlock *fib;
    BPTR lock;
    BOOL isProtected = FALSE;

    /* Lock the file to retrieve its protection bits */
    lock = Lock(filename, ACCESS_READ);
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
        UnLock(lock);
        return FALSE;
    }

    /* Examine the file to retrieve its current attributes */
    if (Examine(lock, fib) == DOSFALSE)
    {
        Printf("Error: Examine failed for file '%s'.\n", filename);
        FreeDosObject(DOS_FIB, fib);
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
    UnLock(lock);

    return isProtected;
}

/* Function to set or unset the write protection bit on a file */
void SetWriteProtection(STRPTR filename, BOOL protect)
{
    struct FileInfoBlock *fib;
    BPTR lock;

    /* Lock the file to retrieve its protection bits */
    lock = Lock(filename, ACCESS_READ);
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
        return;
    }

    /* Examine the file to retrieve its current attributes */
    if (Examine(lock, fib) == DOSFALSE)
    {
        Printf("Error: Examine failed for file '%s'.\n", filename);
        FreeDosObject(DOS_FIB, fib);
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
    UnLock(lock);
}

/* Function to get the delete protection status of a file */
BOOL GetDeleteProtection(STRPTR filename)
{
    struct FileInfoBlock *fib;
    BPTR lock;
    BOOL isProtected = FALSE;

    /* Lock the file to retrieve its protection bits */
    lock = Lock(filename, ACCESS_READ);
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
        UnLock(lock);
        return FALSE;
    }

    /* Examine the file to retrieve its current attributes */
    if (Examine(lock, fib) == DOSFALSE)
    {
        Printf("Error: Examine failed for file '%s'.\n", filename);
        FreeDosObject(DOS_FIB, fib);
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
    UnLock(lock);

    return isProtected;
}

/* Function to set or unset the delete protection bit on a file */
void SetDeleteProtection(STRPTR filename, BOOL protect)
{
    struct FileInfoBlock *fib;
    BPTR lock;

    /* Lock the file to retrieve its protection bits */
    lock = Lock(filename, ACCESS_READ);
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
        return;
    }

    /* Examine the file to retrieve its current attributes */
    if (Examine(lock, fib) == DOSFALSE)
    {
        Printf("Error: Examine failed for file '%s'.\n", filename);
        FreeDosObject(DOS_FIB, fib);
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
    if (lock == 0)
    {
#ifdef DEBUG
        printf("Error: Unable to lock path: %s\n", pathWithoutInfo);
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
    UnLock(lock);
    FreeDosObject(DOS_FIB, fib);
    free(pathWithoutInfo); /* Free the allocated memory */

    return result;
}