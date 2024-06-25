#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stddef.h>
#include <exec/memory.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "icon_management.h"
#include "window_management.h"

void CleanupWindow()
{
    if (font)
    {
        CloseFont(font); // Close the opened font
    }
    if (window)
    {
        CloseWindow(window);
    }
    if (screen)
    {
        UnlockPubScreen("Workbench", screen);
    }
}

int FormatIconsAndWindow(char *folder)
{
    BPTR lock;

    int newWidth;
    int minAverageWidthPercent;
    // int loopCount = 0;
    int CurrentWidth = 0;
    // int maxLoops = 30;

    lock = Lock(folder, ACCESS_READ);

    newWidth = 320;
    CurrentWidth = 320;
    minAverageWidthPercent = -100;
    if (lock)
    {
        minAverageWidthPercent = ArrangeIcons(lock, folder, CurrentWidth);
    }
    else
    {
        printf("Failed to lock the directory: %s\n", folder);
        return 1;
    }

    return 0;
}

void resizeFolderToContents(char *dirPath, IconArray *iconArray)
{
    int i, maxWidth = 0, maxHeight = 0;

    for (i = 0; i < iconArray->size; i++)
    {
        maxWidth = MAX(maxWidth, iconArray->array[i].icon_x + iconArray->array[i].icon_max_width);
        maxHeight = MAX(maxHeight, iconArray->array[i].icon_y + iconArray->array[i].icon_max_height);
    }
    repoistionWindow(dirPath, maxWidth, maxHeight);
}

// Function to open a tiny, non-intrusive window and initialize the RastPort
int InitializeWindow()
{

    int windowWidth;
    int windowHeight;
    int windowLeft;
    int windowTop;

    screen = LockPubScreen("Workbench");
    if (!screen)
    {
        printf("Failed to lock Workbench screen.\n");
        return 0;
    }
    printf("screen width: %d\n", screen->Width);
    printf("Screen height: %d\n", screen->Height);
    screenHight = screen->Height;
    screenWidth = screen->Width;
    // Set tiny dimensions and place the window at the bottom right corner
    windowWidth = 1;                          // Minimal width
    windowHeight = 1;                         // Minimal height
    windowLeft = screen->Width - windowWidth; // Bottom right corner
    windowTop = screen->Height - windowHeight;

    window = OpenWindowTags(NULL,
                            WA_Left, windowLeft,
                            WA_Top, windowTop,
                            WA_Width, windowWidth,
                            WA_Height, windowHeight,
                            WA_Flags, WFLG_BACKDROP, // Makes it a backdrop window
                            WA_CustomScreen, screen,
                            TAG_DONE);
    if (!window)
    {
        printf("Failed to open window.\n");
        UnlockPubScreen("Workbench", screen);
        return 0;
    }

    rastPort = window->RPort;
    if (!rastPort)
    {
        printf("RastPort failed to create.\n");
    }

    // Use the default Workbench font directly from the screen's RastPort
    font = screen->RastPort.Font;
    if (font)
    {
        SetFont(rastPort, font);
    }
    else
    {
        printf("Failed to get default font from screen.\n");
    }

    return 1; // Success
}

void repoistionWindow(char *dirPath, int winWidth, int winHeight)
{
    int posTop = 0, posLeft = 0;
    folderWindowSize newFolderInfo;

    winWidth = winWidth + prefsIControl.currentBarWidth + prefsIControl.currentLeftBarWidth + (PADDING_WIDTH * 2);
    /* is it the root directory and does it have the size guage? */
    if (prefsWorkbench.disableVolumeGauge && IsRootDirectorySimple(dirPath))
        winWidth = winWidth + prefsIControl.currentCGaugeWidth;

    winHeight = winHeight + prefsIControl.currentWindowBarHeight + prefsIControl.currentBarHeight + (PADDING_HEIGHT * 2); // SCROLLBAR_HEIGHT + WINDOW_TITLE_HEIGHT;

    if (winWidth > screenWidth)
    {
        winWidth = screenWidth;
    }

    if (winHeight > screenHight - (prefsIControl.currentTitleBarHeight * 2))
    {
        winHeight = screenHight - (prefsIControl.currentTitleBarHeight * 2); /* posistion the height of the box so its just under the title bar, and an equel amount of the bottom*/
    }

    posTop = (screenHight - winHeight) / 2;
    posLeft = (screenWidth - winWidth) / 2;

    newFolderInfo.left = posLeft;
    newFolderInfo.top = posTop;
    newFolderInfo.width = winWidth;
    newFolderInfo.height = winHeight;

    SaveFolderSettings(dirPath, &newFolderInfo);
}
