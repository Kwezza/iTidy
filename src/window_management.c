// opens a small window to get the font width settings

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "icon_management.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "Settings/get_fonts.h"

struct TextFont *iconFont;



#define FP_WBFONT 0

void CleanupWindow(void)
{
#if PLATFORM_AMIGA
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
#endif
}

int FormatIconsAndWindow(char *folder)
{
#if PLATFORM_AMIGA
    void *lock;
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
#else
    (void)folder;
    return 0;
#endif
}

void resizeFolderToContents(char *dirPath, IconArray *iconArray)
{
#if PLATFORM_AMIGA
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
#else
    (void)dirPath;
    (void)iconArray;
#endif
}

void GetWorkbenchIconFont(char *fontName, int *fontSize)
{
#if PLATFORM_AMIGA
    struct IFFHandle *iff;
    struct ContextNode *cn;
    struct FontPrefs fontPrefs;
    void *file;

    // Default fallback font
    strcpy(fontName, "topaz.font");
    *fontSize = 8;

    // Allocate IFF parser
    iff = AllocIFF();
    if (!iff)
    {
        printf("Failed to allocate IFF parser.\n");
        return;
    }

    // Open Workbench font preferences file
    file = Open("ENV:sys/font.prefs", MODE_OLDFILE);
    if (!file)
    {
        printf("Failed to open ENV:sys/font.prefs. Using default: %s, size: %d\n", fontName, *fontSize);
        FreeIFF(iff);
        return;
    }

    iff->iff_Stream = file;
    InitIFFasDOS(iff);

    if (OpenIFF(iff, IFFF_READ) != 0)
    {
        printf("Failed to open IFF structure.\n");
        Close(file);
        FreeIFF(iff);
        return;
    }

    // Scan IFF chunks
    while ((cn = CurrentChunk(iff)) != NULL)
    {
        if (cn->cn_ID == ID_FONT)  // Found a FONT chunk
        {
            if (ReadChunkBytes(iff, &fontPrefs, sizeof(struct FontPrefs)) == sizeof(struct FontPrefs))
            {
                if (fontPrefs.fp_Type == FP_WBFONT)  // Workbench Icon Font
                {
                    strncpy(fontName, fontPrefs.fp_Name, FONTNAMESIZE - 1);
                    fontName[FONTNAMESIZE - 1] = '\0';  // Ensure null termination
                    *fontSize = fontPrefs.fp_TextAttr.ta_YSize;
                    break;
                }
            }
        }
        if (ParseIFF(iff, IFFPARSE_STEP) == IFFERR_EOF)
            break;
    }

    CloseIFF(iff);
    Close(file);
    FreeIFF(iff);

    // Debug Output
    #ifdef DEBUG
    printf("Workbench Icon Font: %s, Size: %d\n", fontName, *fontSize);
    #endif
#else
    (void)fontName;
    (void)fontSize;
#endif
}

// Function to load and apply the Workbench Icon Font
void SetIconFont(void)
{
#if PLATFORM_AMIGA
    //char fontName[32];
    //int fontSize;
    struct TextAttr textAttr;

    // Get the Workbench icon font settings
    //    GetWorkbenchIconFont(fontName, &fontSize);
    //fontPrefs = (FontPref *)malloc(sizeof(FontPref));
    //fontPrefs = get_fonts();
    //strncpy(fontName, fontPrefs->name, 31);
    //fontSize = fontPrefs->size;


    // Set up TextAttr struct
    textAttr.ta_Name = fontPrefs->name;
    textAttr.ta_YSize = fontPrefs->size;
    textAttr.ta_Style = 0;
    textAttr.ta_Flags = 0;

    // Load the font
    iconFont = OpenDiskFont(&textAttr);

    if (iconFont)
    {
        SetFont(rastPort, iconFont);
        printf( textBold "Workbench Icon Font:" textReset " %s (%dpt) loaded\n", fontPrefs->name, fontPrefs->size);
    }
    else
    {
        printf("Failed to load Workbench Icon Font, using default screen font.\n");
        SetFont(rastPort, screen->RastPort.Font);
    }
#endif
}

// Function to initialize a window and apply the Workbench Icon Font
int InitializeWindow(void)
{
#if PLATFORM_AMIGA
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
        return 0;
    }

    // Apply Workbench Icon Font
    SetIconFont();

    return 1;
#else
    return 0;
#endif
}

void repoistionWindow(char *dirPath, int winWidth, int winHeight)
{
#if PLATFORM_AMIGA
    int posTop = 0, posLeft = 0;
    folderWindowSize newFolderInfo;
    #ifdef DEBUG_MAX
    append_to_log("Repoistion window: %s Padding %d disableVolumeGauge=%d\n", dirPath, PADDING_WIDTH, prefsWorkbench.disableVolumeGauge);
    #endif
    winWidth += prefsIControl.currentBarWidth + prefsIControl.currentLeftBarWidth + (PADDING_WIDTH * 2);
    if (!prefsWorkbench.disableVolumeGauge && IsRootDirectorySimple(dirPath))
    {
        winWidth += prefsIControl.currentCGaugeWidth;

    #ifdef DEBUG_MAX
        append_to_log("Root dir detected and VolumeGauge enabled. Adding CGauge width: %d\n", prefsIControl.currentCGaugeWidth);
    #endif
    }
    else
    {
        #ifdef DEBUG_MAX
        append_to_log("No root dir detected or VolumeGauge disabled. CGauge width: %d\n", prefsIControl.currentCGaugeWidth);
        #endif
    }
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


        #ifdef DEBUG_MAX
    append_to_log("posTop: %d, posLeft: %d, winWidth: %d, winHeight: %d\n", posTop, posLeft, winWidth, winHeight);
    #endif

    SaveFolderSettings(dirPath, &newFolderInfo,0);
#else
    (void)dirPath;
    (void)winWidth;
    (void)winHeight;
#endif
}
