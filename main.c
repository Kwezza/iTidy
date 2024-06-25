
#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/wb.h>
#include <proto/diskfont.h> // Include diskfont library
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <exec/memory.h>
#include <dos/dos.h>

#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/icon.h>

#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>
#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <exec/types.h>
#include <graphics/text.h>
#include <graphics/rastport.h>
#include <graphics/gfxbase.h>
#include <proto/graphics.h>

#include <ctype.h>

#include "Settings/IControlPrefs.h"
#include "Settings/WorkbenchPrefs.h"
#include "main.h"
#include "icon_management.h"
#include "window_management.h"

// Define global variables
struct Screen *screen = NULL;
struct Window *window = NULL;
struct RastPort *rastPort = NULL;
struct TextFont *font = NULL;
struct WorkbenchSettings prefsWorkbench;
struct IControlPrefsDetails prefsIControl;
int screenHight = 0;
int screenWidth = 0;



int main(int argc, char **argv)
{

        // Open the dos.library


    if (argc < 2)
    {
        Printf("Usage: %s <directory>\n", argv[0]);
        return RETURN_FAIL;
    }
    fetchWorkbenchSettings(&prefsWorkbench);
    fetchIControlSettings(&prefsIControl);
    printf("IControl prefs:  border width: %d, border height: %d\n", prefsIControl.currentBarWidth, prefsIControl.currentBarHeight);
    printf("IControl prefs:  title height: %d, Window height: %d\n", prefsIControl.currentTitleBarHeight, prefsIControl.currentWindowBarHeight);
    printf("IControl prefs:  volume guage width: %d\n", prefsIControl.currentCGaugeWidth);

    InitializeWindow();
    ProcessDirectory(argv[1], FALSE);
    CleanupWindow();
    return RETURN_OK;
}




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
            { /* It's a file */
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


    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock == 0)
    {
        Printf("Failed to lock directory: %s\n", path);
        return;
    }

    if (HasSlaveFile(path))
    {

        resizeFolderToContents(path, CreateIconArrayFromPath(lock, path));
        UnLock(lock);
        return;
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
                { /* Only process directories */
                    sprintf(subdir, "%s/%s", path, fib->fib_FileName);
                    ProcessDirectory(subdir, TRUE); /* Recursively process subdirectories */
                }
            }
        }
    }

    FreeMem(fib, sizeof(struct FileInfoBlock));
    UnLock(lock);
}



void CalculateTextExtent(const char *text, struct TextExtent *textExtent)
{
    // struct TextExtent textExtent;
    ULONG textLength = strlen(text);

    if (!rastPort)
    {
        printf("RastPort is not initialized.\n");
        return;
    }

    // Calculate text extent
    TextExtent(rastPort, text, textLength, textExtent);
    
}

void GetFullPath(const char *directory, struct FileInfoBlock *fib, char *fullPath, int fullPathSize)
{
    int dirLen;

    /* Ensure we have valid inputs */
    if (directory == NULL || fib == NULL || fullPath == NULL || fullPathSize <= 0)
    {
        return;
    }

    /* Copy the directory path into fullPath */
    strncpy(fullPath, directory, fullPathSize - 1);
    fullPath[fullPathSize - 1] = '\0'; /* Ensure null-termination */

    /* Check if directory path ends with '/' */
    dirLen = strlen(fullPath);
    if (dirLen > 0 && fullPath[dirLen - 1] != '/' && fullPath[dirLen - 1] != ':')
    {
        /* Append a slash if not present */
        if (dirLen + 1 < fullPathSize)
        {
            strncat(fullPath, "/", fullPathSize - dirLen - 1);
            dirLen++;
        }
    }

    /* Concatenate the file name */
    strncat(fullPath, fib->fib_FileName, fullPathSize - dirLen - 1);

    /* Ensure null-termination */
    fullPath[fullPathSize - 1] = '\0';
}


int IsRootDirectorySimple(char *path)
{
    size_t length = strlen(path);

    // Check if the last character is a colon
    if (length > 0 && path[length - 1] == ':')
    {
        return 1; // True - it is a root directory
    }
    return 0; // False - it is not a root directory
}





void dumpIconArrayToScreen(IconArray *iconArray)
{
    int i;
    printf("Dumping Icon Array of %d icons, max width is %dpx\n", iconArray->size, iconArray->BiggestWidthPX);
    for (i = 0; i < iconArray->size; i++)
    {
        printf("Icon %d: Width = %d, Height = %d, Text Width = %d, Text Height = %d, Max Width = %d, Max Height = %d, x = %d, y = %d, Text = %s, Path = %s\n", i, iconArray->array[i].icon_width, iconArray->array[i].icon_height, iconArray->array[i].text_width, iconArray->array[i].text_height, iconArray->array[i].icon_max_width, iconArray->array[i].icon_max_height, iconArray->array[i].icon_x, iconArray->array[i].icon_y, iconArray->array[i].icon_text, iconArray->array[i].icon_full_path);
    }
}



int saveIconsPositionsToDisk(IconArray *iconArray)
{
    struct DiskObject *diskObject;
    int i;
    char fileNameNoInfo[256];
    int iconArraySize;

    // Pre-calculate the size to avoid multiple dereferences
    iconArraySize = iconArray->size;

    for (i = 0; i < iconArraySize; i++)
    {
        // Access the icon's full path and coordinates once to reduce array access
        FullIconDetails *currentIcon = &iconArray->array[i];

        removeInfoExtension(currentIcon->icon_full_path, fileNameNoInfo);

        // Use GetDiskObjectNew() for better performance if available
        diskObject = GetDiskObject(fileNameNoInfo);

        if (diskObject)
        {
            // Set the new positions
            diskObject->do_CurrentX = currentIcon->icon_x;
            diskObject->do_CurrentY = currentIcon->icon_y;

            // Save the updated DiskObject back to disk
            if (!PutDiskObject(fileNameNoInfo, diskObject))
            {
                fprintf(stderr, "Error: Failed to save icon position for file: %s\n", fileNameNoInfo);
            }

            // Always free the DiskObject to prevent memory leaks
            FreeDiskObject(diskObject);
        }
        else
        {
            fprintf(stderr, "Error: Failed to get DiskObject for file: %s\n", currentIcon->icon_full_path);
        }
    }
    return 0; // Return success or an error code as needed
}



int Compare(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}








void SaveFolderSettings(const char *folderPath, folderWindowSize *newFolderInfo)
{
    char diskInfoPath[256]; // Buffer for the disk.info path
    struct DiskObject *diskObject;
    struct DrawerData *drawerData;
    int folderPathLen;

    strcpy(diskInfoPath, folderPath);

    // Check if folder path ends with a colon or slash
    folderPathLen = strlen(diskInfoPath);
    if (folderPathLen  > 0 && diskInfoPath[folderPathLen - 1] == ':') {
        strcat(diskInfoPath, "Disk");
    } 
   
    
    diskObject = GetDiskObject(diskInfoPath);
    if (diskObject == NULL)
    {
        Printf("Unable to load disk.info for folder: %s\n", (ULONG)folderPath);
        return;
    }

    // Modify the dd_ViewModes and dd_Flags to set 'Show only icons' and 'Show all files'
    if ((diskObject->do_Type == WBDRAWER || diskObject->do_Type == WBDISK) && diskObject->do_DrawerData)
    {
        printf("Exisiting LeftEdge, TopEdge, Width, Height: %d, %d, %d, %d\n", diskObject->do_DrawerData->dd_NewWindow.LeftEdge, diskObject->do_DrawerData->dd_NewWindow.TopEdge, diskObject->do_DrawerData->dd_NewWindow.Width, diskObject->do_DrawerData->dd_NewWindow.Height);
        drawerData = (struct DrawerData *)diskObject->do_DrawerData;
        drawerData->dd_ViewModes = DDVM_BYICON;
        drawerData->dd_Flags = DDFLAGS_SHOWICONS;
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

int strncasecmp_custom(const char *s1, const char *s2, size_t n)
{
    while (n-- > 0 && *s1 != '\0' && *s2 != '\0')
    {
        char c1 = tolower((unsigned char)*s1);
        char c2 = tolower((unsigned char)*s2);

        if (c1 != c2)
        {
            return c1 - c2;
        }

        s1++;
        s2++;
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

BOOL checkIconFrame(const char *iconName)
{
    struct DiskObject *icon = NULL;
    LONG frameStatus;
    LONG errorCode;
    struct Library *IconBase = NULL;
    const char *extension = ".info";
    size_t len = strlen(iconName);
    size_t ext_len = strlen(extension);

    // Calculate the length of the new icon name without the ".info" extension if present
    size_t new_len = (len >= ext_len && strcmp(iconName + len - ext_len, extension) == 0) ? len - ext_len : len;

    // Allocate memory for the new icon name
    char *newIconName = (char *)malloc((new_len + 1) * sizeof(char));
    if (newIconName == NULL)
    {
        printf("Memory allocation failed\n");
        return FALSE;
    }

    // Copy the icon name up to the new length and null-terminate it
    strncpy(newIconName, iconName, new_len);
    newIconName[new_len] = '\0';

    // Open the icon.library
    IconBase = OpenLibrary("icon.library", 0);
    if (!IconBase)
    {
        printf("Failed to open icon.library\n");
        free(newIconName);
        return TRUE; // Assume it has a frame if library can't be opened
    }

    // Load the icon using the new name without the ".info" extension
    icon = GetIconTags(newIconName, TAG_END);
    if (!icon)
    {
        printf("Failed to load icon for border checks: %s\n", newIconName);
        CloseLibrary(IconBase);
        free(newIconName);
        return TRUE; // Assume it has a frame if icon can't be loaded
    }

    // Use IconControl to check the frame status of the icon
    if (IconControl(icon,
                    ICONCTRLA_GetFrameless, &frameStatus,
                    ICONA_ErrorCode, &errorCode,
                    TAG_END) == 1)
    {
        // A frameStatus of 0 means it has a frame, any other value means it does not
        BOOL hasFrame = (frameStatus == 0);

        // Cleanup
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

    // Cleanup
    if (icon)
    {
        FreeDiskObject(icon);
    }
    CloseLibrary(IconBase);
    free(newIconName);
    return TRUE; // Default to having a frame if there's an error
}
