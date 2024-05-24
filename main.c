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

#define ICON_SPACING_X 10
#define ICON_SPACING_Y 35
#define ICON_START_X 10
#define ICON_START_Y 10
#define SCROLLBAR_WIDTH 20
#define SCROLLBAR_HEIGHT 20

#define MAX(a,b) ((a) > (b) ? (a) : (b))

typedef struct
{
    int width;
    int height;
} IconSize;

void ArrangeIcons(BPTR lock, const char *dirPath, BOOL resizeOnly);
int Compare(const void *a, const void *b);
void GetNewIconSize(struct DiskObject *diskObject, IconSize *iconSize);
BOOL IsNewIcon(struct DiskObject *diskObject);

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
            printf("Found file: %s\n", fib->fib_FileName);
            fileCount++;
        }
    }

    if (!resizeOnly)
    {
        qsort(fileNames, fileCount, sizeof(char *), Compare);
        printf("Sorted file names alphabetically.\n");
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

        printf("Getting disk object for file: %s\n", fullPath);

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
                printf("Icon size for file: %s is (%d, %d)\n", fullPath, iconSize.width, iconSize.height);
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
                printf("Arranging icon for file: %s at (%ld, %ld) with size (%d, %d)\n", fullPath, centerX, y, iconSize.width, iconSize.height);
                if (!PutDiskObject(fullPath, diskObject))
                {
                    printf("Failed to save icon position for file: %s\n", fullPath);
                }
                else
                {
                    printf("Saved icon position for file: %s\n", fullPath);
                }
            }
            else
            {
                x = diskObject->do_CurrentX;
                y = diskObject->do_CurrentY;
                printf("Found existing icon position for file: %s at (%ld, %ld) with size (%d, %d)\n", fileNames[i], x, y, iconSize.width, iconSize.height);
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
            printf("Failed to get disk object for file: %s\n", fullPath);
        }
        free(fileNames[i]);
    }

    free(fileNames);

    // Adjust the window size to fit all icons
    maxX = windowWidth + SCROLLBAR_WIDTH;  // Ensure width is the window width
    maxY += SCROLLBAR_HEIGHT;  // Add height for scrollbar if needed
    ChangeWindowBox(window, window->LeftEdge, window->TopEdge, maxX, maxY);
    printf("Resized window to fit all icons: Width = %ld, Height = %ld\n", maxX, maxY);

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
                printf("Found NewIcon data: Width = %d, Height = %d\n", newIconSize->width, newIconSize->height);
            }
            break;
        }
    }
}
