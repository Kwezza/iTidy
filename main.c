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

typedef struct
{
    int left;
    int top;
    int width;
    int height;
} folderWindowSize;

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

void ArrangeIcons(BPTR lock, const char *dirPath, BOOL resizeOnly);
int Compare(const void *a, const void *b);
BOOL IsNewIcon(struct DiskObject *diskObject);
void GetNewIconSize(struct DiskObject *diskObject, IconSize *newIconSize);
void SaveFolderSettings(const char *folderPath, folderWindowSize *newFolderInfo);

int main(int argc, char **argv)
{
    BPTR lock;
    BOOL resizeOnly;

    if (argc < 3)
    {
        printf("Usage: %s <directory> <resizeOnly>\n", argv[0]);
        return 1;
    }

    lock = Lock(argv[1], ACCESS_READ);
    resizeOnly = (BOOL)atoi(argv[2]); // Use the second argument to indicate resizeOnly
    if (lock)
    {
        printf("Locked directory: %s\n", argv[1]);
        ArrangeIcons(lock, argv[1], resizeOnly);
        UnLock(lock);
        printf("Unlocked directory: %s\n", argv[1]);
    }
    else
    {
        printf("Failed to lock the directory: %s\n", argv[1]);
        return 1;
    }

    return 0;
}

void ArrangeIcons(BPTR lock, const char *dirPath, BOOL resizeOnly)
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
    LONG windowWidth; // Move the declaration to the beginning
    char fullPath[512];
    char *dotInfo;
    struct TextFont *font;
    folderWindowSize newFolderInfo;

    fileNames = NULL;
    fileCount = 0;
    x = ICON_START_X;
    y = ICON_START_Y;
    maxX = 0;
    maxY = 0;

    if (!(fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL)))
    {
        printf("Failed to allocate FileInfoBlock.\n");
        return;
    }

    if (!(screen = LockPubScreen("Workbench")))
    {
        printf("Failed to lock Workbench screen.\n");
        FreeDosObject(DOS_FIB, fib);
        return;
    }

    printf("Workbench screen dimensions: Width = %ld, Height = %ld\n", screen->Width, screen->Height);

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
        return;
    }

    printf("Window opened on Workbench screen.\n");

    rastPort = window->RPort;
    windowWidth = window->Width - SCROLLBAR_WIDTH; // Adjusted window width
    windowWidth = 600;
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
            fileNames = realloc(fileNames, sizeof(char *) * (fileCount + 1));
            fileNames[fileCount] = strdup(fib->fib_FileName);

            fileCount++;
        }
    }

    if (!resizeOnly)
    {
        qsort(fileNames, fileCount, sizeof(char *), Compare);
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
            if (TextExtent(rastPort, fileNames[i], strlen(fileNames[i]), &textExtent))
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
                x = ICON_START_X;
                y += iconSize.height + textExtent.te_Height + ICON_SPACING_Y;
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

            if (x + MAX(textWidth, iconSize.width) > maxX)
            {
                maxX = x + MAX(textWidth, iconSize.width);
            }
            if (y + iconSize.height + textExtent.te_Height > maxY)
            {
                maxY = y + iconSize.height + textExtent.te_Height;
            }

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

    // Adjust the window size to fit all icons
    maxX = windowWidth + SCROLLBAR_WIDTH; // Ensure width is the window width
    maxY += SCROLLBAR_HEIGHT;             // Add height for scrollbar if needed
    ChangeWindowBox(window, window->LeftEdge, window->TopEdge, maxX, maxY);
    printf("Resized window to fit all icons: Width = %ld, Height = %ld\n", maxX, maxY);
    newFolderInfo.left = 10;
    newFolderInfo.top = 10;
    newFolderInfo.width = maxX;

    if (maxY > screen->Height - WORKBENCH_BAR)
    {
        maxY = screen->Height - WORKBENCH_BAR;
    }

    newFolderInfo.height = maxY;
    SaveFolderSettings(dirPath, &newFolderInfo);
    // Save new window size to DiskObject
    // SaveNewWindowSize(diskObject, dirPath, maxX, maxY);

    CloseWindow(window);
    printf("Closed window on Workbench screen.\n");
    UnlockPubScreen("Workbench", screen);
    printf("Unlocked Workbench screen.\n");
    FreeDosObject(DOS_FIB, fib);

    // Restore the original font
    if (font)
    {
        CloseFont(font);
    }
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
    printf("folderPath: %s\n", folderPath);
    printf("newFolderInfo->left: %d\n", newFolderInfo->left);
    printf("newFolderInfo->top: %d\n", newFolderInfo->top);
    printf("newFolderInfo->width: %d\n", newFolderInfo->width);
    printf("newFolderInfo->height: %d\n", newFolderInfo->height);

    strcpy(diskInfoPath, folderPath);

    folderPathLen = strlen(diskInfoPath);
    // Check if folder path ends with a colon or slash

    printf(".info path: %s\n", diskInfoPath);
    // Load the disk.info file
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
            Printf("Successfully modified info for folder: %s\n", (ULONG)diskInfoPath);
        }
    }
    else
    {
        Printf("Invalid DiskObject type or missing DrawerData for: %s\n", (ULONG)diskInfoPath);
    }

    FreeDiskObject(diskObject);
}
