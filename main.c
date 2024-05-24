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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ICON_SPACING_X 10
#define ICON_SPACING_Y 50
#define ICON_START_X 10
#define ICON_START_Y 10
#define SCROLLBAR_WIDTH 20
#define SCROLLBAR_HEIGHT 20

void ArrangeIcons(BPTR lock, BOOL resizeOnly);
int Compare(const void *a, const void *b);

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
        ArrangeIcons(lock, resizeOnly);
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

void ArrangeIcons(BPTR lock, BOOL resizeOnly)
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
        WORD iconWidth, iconHeight;
        textWidth = 0;

        diskObject = GetDiskObject(fileNames[i]);
        if (diskObject)
        {
            if (TextExtent(rastPort, fileNames[i], strlen(fileNames[i]), &textExtent))
            {
                textWidth = textExtent.te_Width;
            }

            iconWidth = diskObject->do_Gadget.Width;
            iconHeight = diskObject->do_Gadget.Height;

            if (!resizeOnly)
            {
                diskObject->do_CurrentX = x;
                diskObject->do_CurrentY = y;
                PutDiskObject(fileNames[i], diskObject);
                printf("Arranged icon for file: %s at (%ld, %ld) with size (%d, %d)\n", fileNames[i], x, y, iconWidth, iconHeight);
            }
            else
            {
                x = diskObject->do_CurrentX;
                y = diskObject->do_CurrentY;
                printf("Found existing icon position for file: %s at (%ld, %ld) with size (%d, %d)\n", fileNames[i], x, y, iconWidth, iconHeight);
            }

            if (x + textWidth > maxX)
            {
                maxX = x + textWidth;
            }
            if (y + iconHeight > maxY)
            {
                maxY = y + iconHeight;
            }

            if (!resizeOnly)
            {
                x += textWidth + ICON_SPACING_X;
                if (x > (window->Width - ICON_SPACING_X))
                {
                    x = ICON_START_X;
                    y += ICON_SPACING_Y;
                }
            }

            FreeDiskObject(diskObject);
        }
        free(fileNames[i]);
    }

    free(fileNames);

    // Adjust the window size to fit all icons
    maxX += SCROLLBAR_WIDTH;
    maxY += SCROLLBAR_HEIGHT;
    ChangeWindowBox(window, window->LeftEdge, window->TopEdge, maxX, maxY);
    printf("Resized window to fit all icons: Width = %ld, Height = %ld\n", maxX, maxY);

    CloseWindow(window);
    printf("Closed window on Workbench screen.\n");
    UnlockPubScreen("Workbench", screen);
    printf("Unlocked Workbench screen.\n");
    FreeDosObject(DOS_FIB, fib);
}

int Compare(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}
