// opens a small window to get the font width settings

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
#include "file_directory_handling.h"
#include "utilities.h"
#include "writeLog.h"

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
    int newWidth = 320;
    int minAverageWidthPercent = -100;

    lock = Lock(folder, ACCESS_READ);
        #ifdef DEBUGLocks
    append_to_log("Locking directory (FormatIconsAndWindow): %s\n", folder);
    #endif
    if (lock)
    {
        minAverageWidthPercent = ArrangeIcons(lock, folder, newWidth);
    }
    else
    {
        printf("Failed to lock the directory: %s\n", folder);
        return 1;
    }
            #ifdef DEBUGLocks
    append_to_log("Unlocking directory: %s\n", folder);
    #endif
    UnLock(lock);
    return 0;
}

void resizeFolderToContents(char *dirPath, IconArray *iconArray)
{
    int i, maxWidth = 0, maxHeight = 0;

    if (user_folderViewMode == DDVM_BYICON)
    {
        for (i = 0; i < iconArray->size; i++)
        {
            maxWidth = MAX(maxWidth, iconArray->array[i].icon_x + iconArray->array[i].icon_max_width);
            maxHeight = MAX(maxHeight, iconArray->array[i].icon_y + iconArray->array[i].icon_max_height);
        }
    }
    else

    {
        maxWidth = WindowWidthTextOnly;
        maxHeight = WindowHeightTextOnly;
    }

    repoistionWindow(dirPath, maxWidth, maxHeight);
}

int InitializeWindow()
{
    int windowWidth = 1;
    int windowHeight = 1;
    int windowLeft;
    int windowTop;

    screen = LockPubScreen("Workbench");
    if (!screen)
    {
        printf("Failed to lock Workbench screen.\n");
        return 0;
    }
    screenHight = screen->Height;
    screenWidth = screen->Width;
    windowLeft = screen->Width - windowWidth;
    windowTop = screen->Height - windowHeight;

    window = OpenWindowTags(NULL,
                            WA_Left, windowLeft,
                            WA_Top, windowTop,
                            WA_Width, windowWidth,
                            WA_Height, windowHeight,
                            WA_Flags, WFLG_BACKDROP,
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
    font = screen->RastPort.Font;
    if (font)
    {
        SetFont(rastPort, font);
    }
    else
    {
        printf("Failed to get default font from screen.\n");
    }
    return 1;
}

void repoistionWindow(char *dirPath, int winWidth, int winHeight)
{
    int posTop = 0, posLeft = 0;
    folderWindowSize newFolderInfo;

    winWidth += prefsIControl.currentBarWidth + prefsIControl.currentLeftBarWidth + (PADDING_WIDTH * 2);
    if (prefsWorkbench.disableVolumeGauge && IsRootDirectorySimple(dirPath))
        winWidth += prefsIControl.currentCGaugeWidth;
    winHeight += prefsIControl.currentWindowBarHeight + prefsIControl.currentBarHeight + (PADDING_HEIGHT * 2);
    if (winWidth > screenWidth)
        winWidth = screenWidth;
    if (winHeight > screenHight - (prefsIControl.currentTitleBarHeight * 2))
        winHeight = screenHight - (prefsIControl.currentTitleBarHeight * 2);

    posTop = (screenHight - winHeight) / 2;
    posLeft = (screenWidth - winWidth) / 2;

    newFolderInfo.left = posLeft;
    newFolderInfo.top = posTop;
    newFolderInfo.width = winWidth;
    newFolderInfo.height = winHeight;

    SaveFolderSettings(dirPath, &newFolderInfo);
}
