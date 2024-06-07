#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
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

#include <ctype.h>

typedef struct
{
    int left;
    int top;
    int width;
    int height;
} folderWindowSize;

typedef struct AmigaBorder {
    int borderLeft;
    int borderTop;
    int borderRight;
    int borderBottom;
};

struct AmigaBorder amigaBorder = {1, 15, 15, 15}; 

#define ICON_SPACING_X 10
#define ICON_SPACING_Y 10
#define ICON_START_X 10
#define ICON_START_Y 10
#define SCROLLBAR_WIDTH 20
#define SCROLLBAR_HEIGHT 20
#define WORKBENCH_BAR 20

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct
{
    int width;
    int height;
} IconSize;

typedef struct
{
    LONG Xpos;
    LONG Ypos;
} SizeStruct;

int ArrangeIcons(BPTR lock, const char *dirPath, BOOL resizeOnly, int newWidth);
int Compare(const void *a, const void *b);
BOOL IsNewIcon(struct DiskObject *diskObject);
void GetNewIconSize(struct DiskObject *diskObject, IconSize *newIconSize);
void SaveFolderSettings(const char *folderPath, folderWindowSize *newFolderInfo);
void GetFolderInfo(const char *folderPath, folderWindowSize *folderDeatils);
void pause_program(void);
int FormatIconsAndWindow(const char *folder, int resizeOnly);
void ProcessDirectory(const char *path);
int HasSlaveFile(const char *path);
int strncasecmp_custom(const char *s1, const char *s2, size_t n);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        Printf("Usage: %s <directory>\n", argv[0]);
        return RETURN_FAIL;
    }

    // ProcessDirectory(argv[1]);
    ProcessDirectory("PC:MyProjects/Retroplay-WHDLoad-downloader-1/Extracted/Games/T/TarghanFastFr");
    return RETURN_OK;
}
int HasSlaveFile(const char *path)
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

void ProcessDirectory(const char *path)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    char subdir[4096];
    int isSlave = 0;

    printf("entry Processing directory: %s\n", path);

    if (HasSlaveFile(path))
    {
        printf("Found .slave file in %s, skipping this directory and subdirectories\n", path);
        FormatIconsAndWindow(path,TRUE);
    }
    else
    {
        lock = Lock((STRPTR)path, ACCESS_READ);
        if (lock == 0)
        {
            Printf("Failed to lock directory: %s\n", path);
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
            while (ExNext(lock, fib))
            {
                if (fib->fib_DirEntryType > 0)
                { /* Only process directories */
                    sprintf(subdir, "%s/%s", path, fib->fib_FileName);
                    // printf("Processing directory: %s\n", subdir);
                    ProcessDirectory(subdir); /* Recursively process subdirectories */
                }
            }
        }

        FreeMem(fib, sizeof(struct FileInfoBlock));
        UnLock(lock);
    }
}

int FormatIconsAndWindow(const char *folder, int resizeOnly)
{
    BPTR lock;

    int newWidth;
    int minAverageWidthPercent;
    int loopCount = 0;
    int CurrentWidth = 0;
    int maxLoops = 30;

    lock = Lock(folder, ACCESS_READ);

    newWidth = 320;
    CurrentWidth = 320;
    minAverageWidthPercent = -100;
    if (lock)
    {
        printf("Locked directory: %s\n", folder);
        if (resizeOnly)
        {
            minAverageWidthPercent = ArrangeIcons(lock, folder, resizeOnly, CurrentWidth);
        }
        else
        {
            while (minAverageWidthPercent < -10 || loopCount < maxLoops)
            {

                minAverageWidthPercent = ArrangeIcons(lock, folder, resizeOnly, CurrentWidth);
                // printf("\n ################\nMin Average Width Percent: %d\n#################\n", minAverageWidthPercent);
                //  pause_program();
                if (minAverageWidthPercent < -10 || minAverageWidthPercent > 10)
                {
                    CurrentWidth = CurrentWidth + 40;
                }
                else
                {
                    break;
                }
                loopCount++;
            }
        }

        // printf("Unlocked directory: %s\n", folder);
        // printf("Min Average Width Percent: %d\n", minAverageWidthPercent);
    }
    else
    {
        printf("Failed to lock the directory: %s\n", folder);
        return 1;
    }

    return 0;
}

int ArrangeIcons(BPTR lock, const char *dirPath, BOOL resizeOnly, int newWidth)

{
    struct FileInfoBlock *fib;
    struct DiskObject *diskObject;
    struct Screen *screen;
    struct Window *window;
    struct RastPort *rastPort;
    char **fileNames;
    int fileCount;
    int i;
    LONG x;
    LONG y;
    LONG maxX;
    LONG maxY;
    LONG windowWidth;
    char fullPath[512];
    char *dotInfo;
    struct TextFont *font;
    folderWindowSize newFolderInfo;
    int iconWidths[10] = {0};
    int rowcount = 0;
    int maxWidthsToCheck = 10;
    int totalWidth = 0;
    int averageWidth = 0;
    int maxRowWidth = 0;
    int contineProcessing = 1;
    int minAverageWidthPercent = 0;
    int currentLoopCount = 0;
    int MaxIconsToAlign = 50;
    BOOL SkipAutoResize = FALSE;
    int posTop = 0;
    int posLeft = 0;

    fileNames = NULL;
    fileCount = 0;
    x = ICON_START_X;
    y = ICON_START_Y;
    maxX = 0;
    maxY = 0;

    // GetFolderInfo(dirPath, &newFolderInfo);
    //  printf("Folder Info: left = %d, top = %d, width = %d, height = %d\n", newFolderInfo.left, newFolderInfo.top, newFolderInfo.width, newFolderInfo.height);

    if (!(fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL)))
    {
        printf("Failed to allocate FileInfoBlock.\n");
        return 0;
    }

    if (!(screen = LockPubScreen("Workbench")))
    {
        printf("Failed to lock Workbench screen.\n");
        FreeDosObject(DOS_FIB, fib);
        return 0;
    }

    // printf("Workbench screen dimensions: Width = %ld, Height = %ld\n", screen->Width, screen->Height);

    if (!(window = OpenWindowTags(NULL,
                                  WA_Left, 0,
                                  WA_Top, 0,
                                  WA_Width, screen->Width,
                                  WA_Height, screen->Height,
                                  WA_Flags, WFLG_NOCAREREFRESH | WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_SIMPLE_REFRESH,
                                  WA_CustomScreen, screen,
                                  TAG_DONE)))
    {
        printf("Failed to open window on Workbench screen.\n");
        UnlockPubScreen("Workbench", screen);
        FreeDosObject(DOS_FIB, fib);
        return 0;
    }

    // printf("Window opened on Workbench screen.\n");

    rastPort = window->RPort;

    windowWidth = newWidth;
    // Set the font to match the Workbench font
    font = OpenDiskFont((struct TextAttr *)screen->RastPort.Font);
    if (font)
    {
        SetFont(rastPort, font);
    }

    Examine(lock, fib);
    while (ExNext(lock, fib))
    {
        if (fib->fib_DirEntryType < 0) // It's a file
        {
            if (sizeof(fib->fib_FileName) > 5)
            {
                if (strncasecmp_custom(fib->fib_FileName + strlen(fib->fib_FileName) - 5, ".info", 5) == 0)
                {
            fileNames = realloc(fileNames, sizeof(char *) * (fileCount + 1));
            fileNames[fileCount] = strdup(fib->fib_FileName);

            fileCount++;
                }
            }

        }
    }

    if (!resizeOnly)
    {
        qsort(fileNames, fileCount, sizeof(char *), Compare);
    }

    if (fileCount > MaxIconsToAlign || newWidth > window->Width - SCROLLBAR_WIDTH)
    {
        windowWidth = window->Width - SCROLLBAR_WIDTH; // Adjusted window width
        SkipAutoResize = TRUE;
        // printf("Skipping auto resize due to large number of icons or new width.\n");
        // printf("MaxIconsToAlign: %d  window width: %d\n", MaxIconsToAlign, windowWidth);
    }

    for (i = 0; i < fileCount; i++)
    {
        struct TextExtent textExtent;
        WORD textWidth;
        LONG centerX;
        IconSize iconSize = {0, 0};
        textWidth = 0;

        // Create full path without ".info" extension
        sprintf(fullPath, "%s/%s", dirPath, fileNames[i]);
        dotInfo = strstr(fullPath, ".info");
        if (dotInfo)
        {
            *dotInfo = '\0'; // Remove ".info" from the path
        }

        diskObject = GetDiskObject(fullPath);
        if (diskObject)
        {
            char file_name_no_icon[128]={0};
            strncpy(file_name_no_icon, fileNames[i], strlen(fileNames[i])-5);
            if (TextExtent(rastPort, file_name_no_icon, strlen(file_name_no_icon), &textExtent))
            {
                textWidth = textExtent.te_Width;
            }

            if (IsNewIcon(diskObject))
            {
                GetNewIconSize(diskObject, &iconSize);
            }
            else
            {
                iconSize.width = diskObject->do_Gadget.Width;
                iconSize.height = diskObject->do_Gadget.Height;
            }

            // Check if the icon text or icon itself exceeds the window width
            if (x + MAX(textWidth, iconSize.width) > windowWidth)
            {
                if (rowcount < maxWidthsToCheck)
                {
                    iconWidths[rowcount] = x;
                }
                x = ICON_START_X;
                y += iconSize.height + textExtent.te_Height + ICON_SPACING_Y;
                rowcount++;
            }

            // Center the icon within the space calculated based on text width or icon width
            centerX = x + (MAX(textWidth, iconSize.width) - iconSize.width) / 2;

            if (!resizeOnly)
            {
                diskObject->do_CurrentX = centerX;
                diskObject->do_CurrentY = y;

                if (!PutDiskObject(fullPath, diskObject))
                {
                    printf("Failed to save icon position for file: %s\n", fullPath);
                }
                else
                {
                }
            }
            else
            {
                x = diskObject->do_CurrentX;
                y = diskObject->do_CurrentY;
            }
            printf("Iconx: %d, Icony: %d maxWidth: %d\n", diskObject->do_CurrentX, diskObject->do_CurrentY,MAX(textWidth, iconSize.width));

            if (x + MAX(textWidth, iconSize.width) > maxX)
            {
                maxX = x + MAX(textWidth, iconSize.width);
            }

            if (y + iconSize.height + textExtent.te_Height > maxY)
            {
                maxY = y + iconSize.height + textExtent.te_Height;
            }

            printf("MaxX: %d, MaxY: %d filename: %s rightpos: %d\n", maxX, maxY,file_name_no_icon,diskObject->do_CurrentX+maxX);

            // Increment x position
            x += MAX(textWidth, iconSize.width) + ICON_SPACING_X;

            FreeDiskObject(diskObject);
        }
        else
        {
        }
        free(fileNames[i]);
    }

    free(fileNames);

    if (rowcount < maxWidthsToCheck)
    {
        iconWidths[rowcount] = x;
    }

    if (!resizeOnly)
    {
        maxY += 25;
    }
    // Adjust the window size to fit all icons
    //maxX = windowWidth + SCROLLBAR_WIDTH; // Ensure width is the window width
    //maxY += SCROLLBAR_HEIGHT;             // Add height for scrollbar if needed
    posTop = (screen->Height - maxY) / 2;
    posLeft = (screen->Width - maxX) / 2;
    //ChangeWindowBox(window, posLeft, posTop, maxX, maxY);
    //ScrollWindowRaster(window, -window->LeftEdge, -window->TopEdge, 0, 0, window->Width - 1, window->Height - 1);
    // WindowToFront(myWindow);

    // printf("Resized window to fit all icons: Width = %ld, Height = %ld\n", maxX, maxY);

    newFolderInfo.width = maxX;
    newFolderInfo.height = maxY;

    newFolderInfo.left = 10;
    newFolderInfo.top = 10;

    newFolderInfo.left = (screen->Width - newFolderInfo.width) / 2;
    newFolderInfo.top = (screen->Height - newFolderInfo.height) / 2;


    newFolderInfo.width = newFolderInfo.width + amigaBorder.borderLeft + amigaBorder.borderRight;
    newFolderInfo.height = newFolderInfo.height + amigaBorder.borderTop + amigaBorder.borderBottom;
    

    if (!resizeOnly)
    {
        if (newFolderInfo.width > newFolderInfo.width - WORKBENCH_BAR)
        {
            maxY = screen->Height + WORKBENCH_BAR;
        }
        else
        {
            maxY = maxY + WORKBENCH_BAR;
        }
    }

    newFolderInfo.height = maxY + WORKBENCH_BAR;
    SaveFolderSettings(dirPath, &newFolderInfo);
    // GetFolderInfo(dirPath, &newFolderInfo);
    //  printf("Folder Info: left = %d, top = %d, width = %d, height = %d\n", newFolderInfo.left, newFolderInfo.top, newFolderInfo.width, newFolderInfo.height);
    //   Save new window size to DiskObject
    //   SaveNewWindowSize(diskObject, dirPath, maxX, maxY);

    CloseWindow(window);
    // printf("Closed window on Workbench screen.\n");
    UnlockPubScreen("Workbench", screen);
    // printf("Unlocked Workbench screen.\n");
    FreeDosObject(DOS_FIB, fib);

    for (i = 0; i <= rowcount; i++)
    {
        if (iconWidths[i] == 0)
            break;
        // printf("Row %d: %d\n", i, iconWidths[i]);
        totalWidth += iconWidths[i];
        if (iconWidths[i] > maxRowWidth)
        {
            maxRowWidth = iconWidths[i];
        }
    }

    averageWidth = totalWidth / (rowcount + 1);
    // printf("Row Count: %d\n", rowcount);
    // printf("Total Width: %d\n", totalWidth);
    // printf("Average Width: %d\n", averageWidth);
    // printf("Max Row Width: %d\n", maxRowWidth);

    for (i = 0; i <= rowcount; i++)
    {
        if (iconWidths[i] == 0)
            break;
        // printf("Row %d against overall average: %d\n", i, iconWidths[i] - averageWidth);
        minAverageWidthPercent = (iconWidths[i] - averageWidth) * 100 / averageWidth;
        // printf("minumum average row %d: %d\n", i, minAverageWidthPercent);
    }
    currentLoopCount++;

    // Restore the original font
    if (font)
    {
        CloseFont(font);
    }

    if (SkipAutoResize)
        minAverageWidthPercent = 0;
    // printf("newindow width: %d\n", newWidth);
    return minAverageWidthPercent;
}

int Compare(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

BOOL IsNewIcon(struct DiskObject *diskObject)
{
    STRPTR *toolTypes = diskObject->do_ToolTypes;
    if (toolTypes && strcmp(toolTypes[0], " ") == 0 &&
        strcmp(toolTypes[1], "*** DON'T EDIT THE FOLLOWING LINES!! ***") == 0)
    {
        return TRUE;
    }
    return FALSE;
}

void GetNewIconSize(struct DiskObject *diskObject, IconSize *newIconSize)
{
    STRPTR *toolTypes;
    STRPTR toolType;
    char *prefix = "IM1=";
    int i;

    if (!diskObject || !(toolTypes = diskObject->do_ToolTypes))
    {
        printf("Invalid DiskObject or ToolTypes.\n");
        return;
    }

    for (i = 0; (toolType = toolTypes[i]); i++)
    {
        if (strncmp(toolType, prefix, strlen(prefix)) == 0)
        {
            if (toolType && strlen(toolType) >= 7)
            {
                newIconSize->width = (int)toolType[5] - 33;
                newIconSize->height = (int)toolType[6] - 33;
                // printf("Found NewIcon data: Width = %d, Height = %d\n", newIconSize->width, newIconSize->height);
            }
            break;
        }
    }
}

void SaveFolderSettings(const char *folderPath, folderWindowSize *newFolderInfo)
{
    char diskInfoPath[256]; // Buffer for the disk.info path
    struct DiskObject *diskObject;
    struct DrawerData *drawerData;
    int folderPathLen;
    // printf("folderPath: %s\n", folderPath);
    // printf("newFolderInfo->left: %d\n", newFolderInfo->left);
    // printf("newFolderInfo->top: %d\n", newFolderInfo->top);
    // printf("newFolderInfo->width: %d\n", newFolderInfo->width);
    // printf("newFolderInfo->height: %d\n", newFolderInfo->height);

    strcpy(diskInfoPath, folderPath);

    folderPathLen = strlen(diskInfoPath);
    // Check if folder path ends with a colon or slash

    // printf(".info path: %s\n", diskInfoPath);
    //  Load the disk.info file
    diskObject = GetDiskObject(diskInfoPath);
    if (diskObject == NULL)
    {
        Printf("Unable to load disk.info for folder: %s\n", (ULONG)folderPath);
        return;
    }

    // Modify the dd_ViewModes and dd_Flags to set 'Show only icons' and 'Show all files'
    if (diskObject->do_Type == WBDRAWER && diskObject->do_DrawerData)
    {
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
        else
        {
            // Printf("Successfully modified info for folder: %s\n", (ULONG)diskInfoPath);
        }
    }
    else
    {
        // Printf("Invalid DiskObject type or missing DrawerData for: %s\n", (ULONG)diskInfoPath);
    }

    FreeDiskObject(diskObject);
}

void GetFolderInfo(const char *folderPath, folderWindowSize *folderDeatils)
{

    char diskInfoPath[256]; // Buffer for the disk.info path
    struct DiskObject *diskObject;
    folderDeatils->left = 0;
    folderDeatils->top = 0;
    folderDeatils->width = 0;
    folderDeatils->height = 0;
    strcpy(diskInfoPath, folderPath);

    // strcat(diskInfoPath, ".info");

    // sanitize_amiga_file_path(diskInfoPath);
    // if(does_file_or_folder_exist(diskInfoPath, 0) == 0)
    //{
    //     Printf("File does not exist: %s\n", (ULONG)diskInfoPath);
    //     return;
    // }
    printf(".info path: %s\n", diskInfoPath);
    // Load the disk.info file
    diskObject = GetDiskObject(diskInfoPath);
    if (diskObject == NULL)
    {
        Printf("Unable to load disk.info for folder: %s\n", diskInfoPath);
        return;
    }

    if (diskObject->do_Type == WBDRAWER && diskObject->do_DrawerData)
    {
        folderDeatils->left = diskObject->do_DrawerData->dd_NewWindow.LeftEdge;
        folderDeatils->top = diskObject->do_DrawerData->dd_NewWindow.TopEdge;
        folderDeatils->width = diskObject->do_DrawerData->dd_NewWindow.Width;
        folderDeatils->height = diskObject->do_DrawerData->dd_NewWindow.Height;
    }
    else
    {
        Printf("Invalid DiskObject type or missing DrawerData for: %s\n", (ULONG)diskInfoPath);
    }

    FreeDiskObject(diskObject);
}

void pause_program(void)
{
    char buffer[2]; /* Buffer to store the user's input */

    printf("Press RETURN to continue...\n");

    /* Read a line from stdin, effectively pausing the program until the user presses Return */
    fgets(buffer, sizeof(buffer), stdin);
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