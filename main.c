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
    struct WBStartup *wbStartup = (struct WBStartup *)argv;
    struct WBArg *wbArg;

    if (argc == 0 && wbStartup != NULL)
    {
        wbArg = wbStartup->sm_ArgList;
        if (wbArg->wa_Lock)
        {
            ArrangeIcons(wbArg->wa_Lock, FALSE); // Change FALSE to TRUE if you want to only resize the window
        }
    }
    else if (argc > 2)
    {
        BPTR lock = Lock(argv[1], ACCESS_READ);
        BOOL resizeOnly = (BOOL)atoi(argv[2]); // Use the second argument to indicate resizeOnly
        if (lock)
        {
            ArrangeIcons(lock, resizeOnly);
            UnLock(lock);
        }
    }
    else
    {
        printf("Usage: %s <directory> <resizeOnly>\n", argv[0]);
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
    char **fileNames = NULL;
    int fileCount = 0;
    int i;
    LONG x = ICON_START_X;
    LONG y = ICON_START_Y;
    LONG maxX = 0;
    LONG maxY = 0;

    if (!(fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL)))
    {
        return;
    }

    if (!(screen = LockPubScreen("Workbench")))
    {
        FreeDosObject(DOS_FIB, fib);
        return;
    }

    if (!(window = OpenWindowTags(NULL,
                                  WA_Left, 0,
                                  WA_Top, 0,
                                  WA_Width, screen->Width,
                                  WA_Height, screen->Height,
                                  WA_Flags, WFLG_NOCAREREFRESH | WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_SIMPLE_REFRESH,
                                  WA_CustomScreen, screen,
                                  TAG_DONE)))
    {
        UnlockPubScreen("Workbench", screen);
        FreeDosObject(DOS_FIB, fib);
        return;
    }

    rastPort = window->RPort;

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
        diskObject = GetDiskObject(fileNames[i]);
        if (diskObject)
        {
            struct TextExtent textExtent;
            WORD textWidth = 0;

            if (TextExtent(rastPort, fileNames[i], strlen(fileNames[i]), &textExtent))
            {
                textWidth = textExtent.te_Width;
            }

            if (!resizeOnly)
            {
                diskObject->do_CurrentX = x;
                diskObject->do_CurrentY = y;
                PutDiskObject(fileNames[i], diskObject);
            }
            else
            {
                x = diskObject->do_CurrentX;
                y = diskObject->do_CurrentY;
            }

            if (x + textWidth > maxX)
            {
                maxX = x + textWidth;
            }
            if (y + ICON_SPACING_Y > maxY)
            {
                maxY = y + ICON_SPACING_Y;
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
    SetWindowTitles(window, "Icons Arranged", (UBYTE *)-1);
    ChangeWindowBox(window, window->LeftEdge, window->TopEdge, maxX + SCROLLBAR_WIDTH, maxY + SCROLLBAR_HEIGHT);

    CloseWindow(window);
    UnlockPubScreen("Workbench", screen);
    FreeDosObject(DOS_FIB, fib);
}

int Compare(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}
