
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

#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>
#include <stdio.h>
#include <string.h>
// #include <libraries/icon.h>
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
#define SCROLLBAR_WIDTH 50
#define SCROLLBAR_HEIGHT 30
#define WINDOW_TITLE_HEIGHT 30
#define WORKBENCH_BAR 20

#define PADDING_WIDTH 2
#define PADDING_HEIGHT 2

#define FONT_PREFS_FILE "ENV:sys/font.prefs"
#define const_fontIcon 0
#define const_fontDefault 1
#define const_fontScreen 2

#define MAX_PATH_LEN 256

#define CHUNK_SIZE 1024                                                  /* Size of the buffer to read in each iteration */
#define HEADER_SIZE 12 /* Size of IFF header ("FORM" + size + "ICON") */ /* Size of IFF header ("FORM" + size + "ICON")  for OS3.5 Icons*/

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct
{
    int width;
    int height;
} IconSize;

typedef struct
{
    int x;
    int y;
} IconPosition;

typedef struct
{
    int icon_x;
    int icon_y;
    int icon_width;
    int icon_height;
    int text_width;
    int text_height;
    int icon_max_width;
    int icon_max_height;
    char *icon_text;
    char *icon_full_path;
    BOOL is_folder;

} FullIconDetails;

typedef struct
{
    FullIconDetails *array; /* Pointer to the array of FullIconDetails */
    size_t size;            /* Current number of elements */
    size_t capacity;        /* Allocated capacity of the array */
    size_t BiggestWidthPX;
} IconArray;

struct Screen *screen;
struct Window *window;
struct RastPort *rastPort;
struct TextFont *font;
struct WorkbenchSettings prefsWorkbench;
struct IControlPrefsDetails prefsIControl;

const MAX_ICONS_TO_ALLIGN = 50;

int screenHight = 0;
int screenWidth = 0;

int ArrangeIcons(BPTR lock, const char *dirPath, BOOL resizeOnly, int newWidth);
int Compare(const void *a, const void *b);
BOOL IsNewIcon(struct DiskObject *diskObject);
BOOL IsNewIconPath(const STRPTR filePath);
void SaveFolderSettings(const char *folderPath, folderWindowSize *newFolderInfo);
void pause_program(void);
int FormatIconsAndWindow(const char *folder, int resizeOnly);
void ProcessDirectory(const char *path);
int HasSlaveFile(const char *path);
int strncasecmp_custom(const char *s1, const char *s2, size_t n);
void removeInfoExtension(const char *input, char *output);
int isOS35IconFormat(const char *filename);
int getOS35IconSize(const char *filename, IconSize *size);
int InitializeWindow(void);
void CleanupWindow(void);
BOOL GetStandardIconSize(const char *filePath, IconSize *iconSize);
void GetNewIconSizePath(const char *filePath, IconSize *newIconSize);
void GetFullPath(const char *directory, struct FileInfoBlock *fib, char *fullPath, int fullPathSize);
IconArray *CreateIconArrayFromPath(BPTR lock, const char *dirPath);
void dumpIconArrayToScreen(IconArray *iconArray);
void resizeFolderToContents(char *dirPath, IconArray *iconArray);
void repoistionWindow(char *dirPath, int winWidth, int winHeight);
IconPosition GetIconPositionFromPath(const char *iconPath);
int IsRootDirectorySimple(const char *path);

int main(int argc, char **argv)
{
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
    ProcessDirectory(argv[1]);
    CleanupWindow();
    return RETURN_OK;
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

    printf("entry Processing directory: %s\n", path);
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock == 0)
    {
        Printf("Failed to lock directory: %s\n", path);
        return;
    }

    if (HasSlaveFile(path))
    {
        // printf("Found .slave file in %s, resizing windows only and skipping.\n", path);
        //FormatIconsAndWindow(path, TRUE);
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
        FormatIconsAndWindow(path, FALSE);
        while (ExNext(lock, fib))
        {
            if (fib->fib_DirEntryType > 0)
            { /* Only process directories */
                sprintf(subdir, "%s/%s", path, fib->fib_FileName);
                ProcessDirectory(subdir); /* Recursively process subdirectories */
            }
        }
    }

    FreeMem(fib, sizeof(struct FileInfoBlock));
    UnLock(lock);
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
        if (resizeOnly)
        {
            minAverageWidthPercent = ArrangeIcons(lock, folder, resizeOnly, CurrentWidth);
        }
        else
        {
            while (minAverageWidthPercent < -10 || loopCount < maxLoops)
            {

                minAverageWidthPercent = ArrangeIcons(lock, folder, resizeOnly, CurrentWidth);
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
    }
    else
    {
        printf("Failed to lock the directory: %s\n", folder);
        return 1;
    }

    return 0;
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
    // printf("Text extent: Width = %d, Height = %d\n", textExtent->te_Width, textExtent->te_Height);
}

IconArray *CreateIconArray(void)
{
    IconArray *iconArray = (IconArray *)AllocVec(sizeof(IconArray), MEMF_CLEAR);
    if (iconArray != NULL)
    {
        iconArray->array = NULL; /* Start with no allocated array */
        iconArray->size = 0;
        iconArray->capacity = 0;
    } /* if */
    return iconArray;
}

BOOL AddIconToArray(IconArray *iconArray, const FullIconDetails *newIcon)
{
    FullIconDetails *newArray;
    size_t newCapacity;

    if (iconArray == NULL || newIcon == NULL)
    {
        return FALSE;
    } /* if */

    if (iconArray->size >= iconArray->capacity)
    {
        /* Increase capacity (start with 1 if currently 0) */
        newCapacity = (iconArray->capacity == 0) ? 1 : iconArray->capacity * 2;
        newArray = (FullIconDetails *)AllocVec(newCapacity * sizeof(FullIconDetails), MEMF_CLEAR);
        if (newArray == NULL)
        {
            return FALSE;
        } /* if */

        /* Copy existing elements to new array */
        if (iconArray->array != NULL)
        {
            CopyMem(iconArray->array, newArray, iconArray->size * sizeof(FullIconDetails));
            FreeVec(iconArray->array);
        } /* if */

        iconArray->array = newArray;
        iconArray->capacity = newCapacity;
    } /* if */

    /* Add the new icon details to the array */
    iconArray->array[iconArray->size] = *newIcon;
    iconArray->size += 1;

    return TRUE;
}

void FreeIconArray(IconArray *iconArray)
{

    size_t i;
    printf("Freeing Icon Array\n");
    if (iconArray != NULL)
    {
        if (iconArray->array != NULL)
        {
            for (i = 0; i < iconArray->size; i++)
            {
                if (iconArray->array[i].icon_text != NULL)
                {
                    FreeVec(iconArray->array[i].icon_text);
                } /* if */
                if (iconArray->array[i].icon_full_path != NULL)
                {
                    FreeVec(iconArray->array[i].icon_full_path);
                } /* if */
            } /* for */
            FreeVec(iconArray->array);
        } /* if */
        FreeVec(iconArray);
    } /* if */
}

/* Comparison function for sorting by is_folder and then by icon_text */
int CompareByFolderAndName(const void *a, const void *b)
{
    const FullIconDetails *iconA = (const FullIconDetails *)a;
    const FullIconDetails *iconB = (const FullIconDetails *)b;

    /* Compare by is_folder */
    if (iconA->is_folder != iconB->is_folder)
    {
        /* Sort folders (TRUE) before files (FALSE) */
        return iconB->is_folder - iconA->is_folder;
    }

    /* If both have the same is_folder status, compare by icon_text */
    return strncasecmp_custom(iconA->icon_text, iconB->icon_text, strlen(iconA->icon_text));
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
    if (dirLen > 0 && fullPath[dirLen - 1] != '/')
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

IconArray *CreateIconArrayFromPath(BPTR lock, const char *dirPath)
{
    struct TextExtent textExtent;
    FullIconDetails newIcon;
    struct FileInfoBlock *fib;
    IconSize iconSize = {0, 0};
    IconArray *iconArray = CreateIconArray();
    char fullPathAndFile[512];
    char fileNameNoInfo[128];
    int textLength, fileCount = 0,  maxWidth=0;
    IconPosition iconPosition;

    if (!(fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL)))
    {
        printf("Failed to allocate FileInfoBlock.\n");
        return 0;
    }

    if (Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            if (fib->fib_DirEntryType < 0 || fib->fib_DirEntryType > 0) /* It's a file or a directory */
            {
                const char *fileExtension = strrchr(fib->fib_FileName, '.');

                if (fileExtension != NULL && strncasecmp_custom(fileExtension, ".info", 5) == 0)
                {

                    GetFullPath(dirPath, fib, fullPathAndFile, sizeof(fullPathAndFile));
                    removeInfoExtension(fib->fib_FileName, fileNameNoInfo);
                    CalculateTextExtent(fileNameNoInfo, &textExtent);
                    iconPosition = GetIconPositionFromPath(fullPathAndFile);


                    if (IsNewIconPath(fullPathAndFile))
                    {
                        GetNewIconSizePath(fullPathAndFile, &iconSize);
                    }
                    else if (isOS35IconFormat(fullPathAndFile))
                    {
                        getOS35IconSize(fullPathAndFile, &iconSize);
                    }
                    else
                    {
                        GetStandardIconSize(fullPathAndFile, &iconSize);
                    }
                    newIcon.icon_width = iconSize.width;
                    newIcon.icon_height = iconSize.height;
                    newIcon.text_width = textExtent.te_Width;
                    newIcon.text_height = textExtent.te_Height;
                    newIcon.icon_max_width = MAX(iconSize.width, textExtent.te_Width);
                    newIcon.icon_max_height = iconSize.height + textExtent.te_Height;
                    newIcon.icon_x = iconPosition.x;
                    newIcon.icon_y = iconPosition.y;

                    /* Determine if it's a folder or a file based on fib_DirEntryType */
                    newIcon.is_folder = (fib->fib_DirEntryType > 0) ? TRUE : FALSE;

                    newIcon.icon_full_path = (char *)AllocVec(strlen(fullPathAndFile) + 1, MEMF_CLEAR);
                    if (newIcon.icon_full_path != NULL)
                    {
                        strncpy(newIcon.icon_full_path, fullPathAndFile, strlen(fullPathAndFile) + 1);
                    }

                    textLength = strlen(fib->fib_FileName) + 1;
                    newIcon.icon_text = (char *)AllocVec(textLength * sizeof(char), MEMF_CLEAR);
                    if (newIcon.icon_text != NULL)
                    {
                        strncpy(newIcon.icon_text, fib->fib_FileName, textLength);
                    }

                    if (newIcon.icon_max_width > maxWidth)
                    {
                         maxWidth = newIcon.icon_max_width;
                    }
                    /* Add new icon to the array */
                    if (!AddIconToArray(iconArray, &newIcon))
                    {
                        /* Handle the error of adding the icon */
                        FreeVec(newIcon.icon_text); /* Free the icon text memory if adding fails */
                        FreeIconArray(iconArray);   /* Free the entire array */
                        return NULL;                /* Return or handle error */
                    } /* if */

                    fileCount++;
                }
            }
        }
    }
     iconArray->BiggestWidthPX =  maxWidth;
     dumpIconArrayToScreen(iconArray);
    return iconArray;
}

int IsRootDirectorySimple(const char *path) {
    size_t length = strlen(path);

    // Check if the last character is a colon
    if (length > 0 && path[length - 1] == ':') {
        return 1; // True - it is a root directory
    }
    return 0; // False - it is not a root directory
}

void resizeFolderToContents(char *dirPath, IconArray *iconArray)
{
    int i, maxWidth = 0, maxHeight = 0;

    for(i=0; i<iconArray->size; i++)
    {
        maxWidth = MAX(maxWidth, iconArray->array[i].icon_x + iconArray->array[i].icon_max_width);
        maxHeight = MAX(maxHeight, iconArray->array[i].icon_y +iconArray->array[i].icon_max_height);
    }
    printf("Max Width: %d, Max Height: %d folder %s\n", maxWidth, maxHeight, dirPath);
    repoistionWindow(dirPath, maxWidth, maxHeight);
}

void repoistionWindow(char *dirPath, int winWidth, int winHeight)
{
    int posTop = 0, posLeft = 0;
    folderWindowSize newFolderInfo;

    winWidth = winWidth + prefsIControl.currentBarWidth + prefsIControl.currentLeftBarWidth + (PADDING_WIDTH*2);
    /* is it the root directory and does it have the size guage? */ 
    if (prefsWorkbench.disableVolumeGauge && IsRootDirectorySimple(*dirPath)) winWidth= winWidth + prefsIControl.currentCGaugeWidth;

    winHeight = winHeight + prefsIControl.currentWindowBarHeight + prefsIControl.currentBarHeight + (PADDING_HEIGHT*2); // SCROLLBAR_HEIGHT + WINDOW_TITLE_HEIGHT;

    posTop = (screenHight - winHeight) / 2;
    posLeft = (screenWidth - winWidth) / 2;

    printf("Calculated screen Height: %d, Screen Width: %d\n", screenHight, screenWidth);

    newFolderInfo.left = posLeft;
    newFolderInfo.top = posTop;
    newFolderInfo.width = winWidth ;
    newFolderInfo.height = winHeight;

    printf("Window resized to fit all icons: top = %d, left = %d, Width = %d, Height = %d\n",newFolderInfo.top, newFolderInfo.left, winWidth, winHeight);

    SaveFolderSettings(dirPath, &newFolderInfo);
}

void dumpIconArrayToScreen(IconArray *iconArray)
{
    int i;
    printf("Dumping Icon Array of %d icons, max width is %dpx\n", iconArray->size, iconArray->BiggestWidthPX);
    for (i = 0; i < iconArray->size; i++)
    {
        printf("Icon %d: Width = %d, Height = %d, Text Width = %d, Text Height = %d, Max Width = %d, Max Height = %d, x = %d, y = %d, Text = %s, Path = %s\n", i, iconArray->array[i].icon_width, iconArray->array[i].icon_height, iconArray->array[i].text_width, iconArray->array[i].text_height, iconArray->array[i].icon_max_width, iconArray->array[i].icon_max_height, iconArray->array[i].icon_x,iconArray->array[i].icon_y,  iconArray->array[i].icon_text, iconArray->array[i].icon_full_path);
    }
}


/* Function to get the X and Y position from an .info file */
IconPosition GetIconPositionFromPath(const char *iconPath)
{
    IconPosition position;        /* Structure to hold X and Y positions */
    char *pathCopy;               /* Copy of the icon path */
    size_t pathLength;            /* Length of the icon path */
    struct DiskObject *diskObject; /* Pointer to the DiskObject structure */

    /* Initialize the IconPosition structure with invalid coordinates */
    position.x = -1;
    position.y = -1;

    /* Determine the length of the provided path */
    pathLength = strlen(iconPath);

    /* Allocate memory for the path copy */
    pathCopy = (char *)AllocVec(pathLength + 1, MEMF_CLEAR);
    if (pathCopy == NULL)
    {
        printf("Failed to allocate memory for path copy.\n");
        return position;
    }

    /* Copy the original icon path to the allocated memory */
    strncpy(pathCopy, iconPath, pathLength);

    /* Ensure the path copy is null-terminated */
    pathCopy[pathLength] = '\0';

    /* Check if the path ends with .info and remove it */
    if (pathLength > 5 && strncasecmp_custom(pathCopy + pathLength - 5, ".info", 5) == 0)
    {
        pathCopy[pathLength - 5] = '\0'; /* Remove the .info extension */
    }

    /* Load the DiskObject from the modified path (without .info) */
    diskObject = GetDiskObject(pathCopy);
    if (diskObject == NULL)
    {
        printf("Failed to load .info file: %s.info\n", pathCopy);
        FreeVec(pathCopy); /* Free the allocated memory */
        return position;
    }

    /* Get the current X and Y positions */
    position.x = diskObject->do_CurrentX;
    position.y = diskObject->do_CurrentY;

    /* Free the DiskObject and the path copy */
    FreeDiskObject(diskObject);
    FreeVec(pathCopy);

    return position;
}

int ArrangeIcons(BPTR lock, const char *dirPath, BOOL resizeOnly, int newWidth)
{
    struct DiskObject *diskObject;
    LONG x;
    LONG y;
    LONG maxX;
    LONG maxY;
    LONG windowWidth;
    IconArray *iconArray = CreateIconArray();
    BOOL SkipAutoResize = FALSE;
    int i;
    int iconWidths[10] = {0};
    int rowcount = 0;
    int maxWidthsToCheck = 10;
    int minAverageWidthPercent = 0;
    LONG centerX;
    char fileNameNoInfo[256];
    folderWindowSize newFolderInfo;
    int totalWidth = 0, posTop = 0, posLeft = 0, maxRowWidth = 0, averageWidth = 0;
    x = ICON_START_X;
    y = ICON_START_Y;
    maxX = 0;
    maxY = 0;



    windowWidth = newWidth;
    // Set the font to match the Workbench font

    printf("screen width: %d\n", screenWidth);
    printf("windowWidth: %d\n", windowWidth);

    iconArray = CreateIconArrayFromPath(lock, dirPath);
    //dumpIconArrayToScreen(iconArray);
    return 0;
    if (!resizeOnly)
    {
        if (iconArray != NULL && iconArray->array != NULL && iconArray->size > 0)
        {
            qsort(iconArray->array, iconArray->size, sizeof(FullIconDetails), CompareByFolderAndName);
        }
    }

    if (iconArray->size > MAX_ICONS_TO_ALLIGN || newWidth > screenWidth - SCROLLBAR_WIDTH)
    {
        windowWidth = screenWidth - SCROLLBAR_WIDTH; // Adjusted window width
        SkipAutoResize = TRUE;
    }

    printf("Array size %d\n", iconArray->size);
    for (i = 0; i < iconArray->size; i++)
    {
        removeInfoExtension(iconArray->array[i].icon_full_path, fileNameNoInfo);
        diskObject = GetDiskObject(fileNameNoInfo);

        if (diskObject)
        {
            // Check if the icon text or icon itself exceeds the window width
            printf("X: %d, Y: %d, win width: %d, rowcount %d, Current endX %d,  Saved icon: %s \n", x, y, windowWidth, rowcount, x + MAX(iconArray->array[i].text_width, iconArray->array[i].icon_width), iconArray->array[i].icon_full_path);
            if (x + MAX(iconArray->array[i].text_width, iconArray->array[i].icon_width) > windowWidth)
            {
                if (rowcount < maxWidthsToCheck)
                {
                    iconWidths[rowcount] = x;
                }
                x = ICON_START_X;
                y += iconArray->array[i].icon_max_height+ ICON_SPACING_Y;
                rowcount++;
            }
            // printf("3  X: %d, Y: %d, window width: %d, rowcount %d, Current endX %d,  Saved icon: %s \n", x, y, windowWidth, rowcount, x + MAX(iconArray->array[i].text_width, iconArray->array[i].icon_width), iconArray->array[i].icon_full_path);
            //   Center the icon within the space calculated based on text width or icon width
            centerX = x + (MAX(iconArray->array[i].text_width, iconArray->array[i].icon_width) - iconArray->array[i].icon_width) / 2;

            if (!resizeOnly)
            {
                diskObject->do_CurrentX = centerX;
                diskObject->do_CurrentY = y;

                if (!PutDiskObject(fileNameNoInfo, diskObject))
                {
                    printf("Failed to save icon position for file: %s\n", fileNameNoInfo);
                }
                else
                {
                    printf("4  X: %d, Y: %d, window width: %d, Saved icon: %s \n", x, y, windowWidth, fileNameNoInfo);
                }
            }
            else
            {
                x = diskObject->do_CurrentX;
                y = diskObject->do_CurrentY;
            }

            maxX = MAX(maxX, (diskObject->do_CurrentX + iconArray->array[i].icon_max_width));
            maxY = MAX(maxY, (diskObject->do_CurrentY + iconArray->array[i].icon_max_height));
            x += iconArray->array[i].icon_max_width + ICON_SPACING_X;

            FreeDiskObject(diskObject);
        }
        else
        {
            printf("Failed to get DiskObject for file: %s\n", iconArray->array[i].icon_full_path);
        }
    }

    if (rowcount < maxWidthsToCheck)
    {
        iconWidths[rowcount] = x;
    }

    if (!resizeOnly)
    {
        maxY += 25;
    }
    // Adjust the window size to fit all icons

    repoistionWindow(dirPath, maxX, maxY);
    /*
    posTop = (screenHight - maxY) / 2;
    posLeft = (screenWidth - maxX) / 2;

    maxX = maxX + SCROLLBAR_WIDTH;
    maxY = maxY + SCROLLBAR_HEIGHT + WINDOW_TITLE_HEIGHT;

    newFolderInfo.left = posTop;
    newFolderInfo.top = posLeft;
    newFolderInfo.width = maxX;
    newFolderInfo.height = maxY;
        */
    // printf("Window resized to fit all icons: Width = %ld, Height = %ld\n", maxX, maxY);
    // printf("Window position: Left =  %d, Top = %d \n", posTop, posLeft);
    // printf("BorderLeft: %d, BorderRight: %d, BorderTop: %d,BorderBottom: %d\n", window->BorderLeft, window->BorderRight, window->BorderTop, window->BorderBottom);
    // printf("Screen dimensions: Width = %ld, Height = %ld\n", screen->Width, screen->Height);

    SaveFolderSettings(dirPath, &newFolderInfo);

    for (i = 0; i <= iconArray->size; i++)
    {
        if (iconWidths[i] == 0)
            break;
        totalWidth += MAX(iconArray->array[i].text_width, iconArray->array[i].icon_width);
        if (MAX(iconArray->array[i].text_width, iconArray->array[i].icon_width) > maxRowWidth)
        {
            maxRowWidth = MAX(iconArray->array[i].text_width, iconArray->array[i].icon_width);
        }
    }

    averageWidth = totalWidth / (rowcount + 1);
    // printf("Row Count: %d\n", rowcount);
    // printf("Total Width: %d\n", totalWidth);
    // printf("Average Width: %d\n", averageWidth);
    // printf("Max Row Width: %d\n", maxRowWidth);

    for (i = 0; iconArray->size; i++)
    {
        if (iconWidths[i] == 0)
            break;
        // printf("Row %d against overall average: %d\n", i, iconWidths[i] - averageWidth);
        minAverageWidthPercent = ( iconArray->array[i].icon_max_width - averageWidth) * 100 / averageWidth;
        // printf("minumum average row %d: %d\n", i, minAverageWidthPercent);
    }

    if (SkipAutoResize)
        minAverageWidthPercent = 0;
    printf("Freeing Icon Array\n");
    FreeIconArray(iconArray);
    return minAverageWidthPercent;
}

int Compare(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

BOOL IsNewIcon(struct DiskObject *diskObject)
{
    STRPTR *toolTypes;
    BOOL newIconFormat = FALSE;

    toolTypes = diskObject->do_ToolTypes;

    /* Print all tooltypes */
    if (toolTypes != NULL)
    {

        while (*toolTypes != NULL)
        {
            // printf("ToolType: %s\n", *toolTypes);
            if (strcmp(*toolTypes, "*** DON'T EDIT THE FOLLOWING LINES!! ***") == 0)
            {
                newIconFormat = TRUE;
            } /* if */
            toolTypes++;
        } /* while */
    } /* if */

    if (newIconFormat)
    {
        // printf("New icon format detected.\n");
        return TRUE;
    } /* if */

    // printf("Old icon format detected.\n");
    return FALSE;
} /* IsNewIcon */

BOOL IsNewIconPath(const STRPTR filePath)
{
    BOOL newIconFormat = FALSE;
    struct DiskObject *diskObject = NULL;
    STRPTR adjustedFilePath = NULL;
    STRPTR *toolTypes;

    /* Check if the provided filepath ends with ".info" */
    size_t len = strlen(filePath);
    if (len >= 5 && strcmp(filePath + len - 5, ".info") == 0)
    {
        /* Allocate memory for the new path without ".info" */
        adjustedFilePath = (STRPTR)AllocVec(len - 4, MEMF_CLEAR);
        if (adjustedFilePath == NULL)
        {
            // Memory allocation failed
            return FALSE;
        }

        /* Create a new string without the ".info" extension */
        strncpy(adjustedFilePath, filePath, len - 5);
        adjustedFilePath[len - 5] = '\0';
    }
    else
    {
        /* Allocate memory for the original path */
        adjustedFilePath = (STRPTR)AllocVec(len + 1, MEMF_CLEAR);
        if (adjustedFilePath == NULL)
        {
            // Memory allocation failed
            return FALSE;
        }

        /* Copy the original filepath */
        strncpy(adjustedFilePath, filePath, len);
        adjustedFilePath[len] = '\0'; /* Ensure null-termination */
    }

    /* Load the DiskObject from the adjusted file path */
    diskObject = GetDiskObject(adjustedFilePath);
    if (diskObject == NULL)
    {
        FreeVec(adjustedFilePath);
        return FALSE;
    }

    /* Get the ToolTypes */
    toolTypes = diskObject->do_ToolTypes;

    /* Check for the specific tool type indicating new icon format */
    if (toolTypes != NULL)
    {
        while (*toolTypes != NULL)
        {
            if (strcmp(*toolTypes, "*** DON'T EDIT THE FOLLOWING LINES!! ***") == 0)
            {
                newIconFormat = TRUE;
                break;
            }
            toolTypes++;
        }
    }

    /* Clean up */
    FreeDiskObject(diskObject);
    FreeVec(adjustedFilePath);

    /* Return the result */
    return newIconFormat;
} /* IsNewIcon */

int isOS35IconFormat(const char *filename)
{
    FILE *file;
    UBYTE buffer[CHUNK_SIZE];
    size_t bytesRead;
    long fileSize;
    int foundForm = 0; /* Flag to indicate if "FORM" has been found */
    int foundIcon = 0; /* Flag to indicate if "ICON" has been found after "FORM" */
    long offset = 0;
    size_t i;

    /* Open the file in binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("filename: %s\n", filename);
        perror("Error opening file");
        return 0; /* Return false on error */
    }

    /* Get the file size */
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Check if the file size is at least the size of the header */
    if (fileSize < HEADER_SIZE)
    {
        fclose(file);
        return 0; /* File too small to be OS3.5 icon */
    }

    /* Read the file in chunks and search for the headers */
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, file)) > 0)
    {
        for (i = 0; i < bytesRead - HEADER_SIZE; i++)
        {
            /* Check for "FORM" header */
            if (!foundForm && memcmp(buffer + i, "FORM", 4) == 0)
            {
                foundForm = 1; /* Found "FORM" */
                offset = ftell(file) - bytesRead + i;
            }

            /* If "FORM" was found, look for "ICON" */
            if (foundForm && memcmp(buffer + i + 8, "ICON", 4) == 0)
            {
                foundIcon = 1; /* Found "ICON" */
                break;         /* Break the loop as we've found the required headers */
            }
        }

        if (foundIcon)
        {
            break; /* Exit the loop once "FORM" and "ICON" are found */
        }
    }

    /* Close the file */
    fclose(file);

    /* Return true if both "FORM" and "ICON" were found */
    return (foundForm && foundIcon);
}

void GetNewIconSizePath(const char *filePath, IconSize *newIconSize)
{
    struct DiskObject *diskObject;
    STRPTR *toolTypes;
    STRPTR toolType;
    char *prefix = "IM1=";
    int i;
    char *filePathCopy;
    size_t filePathLen;

    if (!filePath || !newIconSize)
    {
        printf("Invalid filePath or newIconSize pointer.\n");
        return;
    }

    // Make a copy of the file path to work with
    filePathLen = strlen(filePath);
    filePathCopy = (char *)AllocVec(filePathLen + 1, MEMF_CLEAR);
    if (!filePathCopy)
    {
        printf("Memory allocation failed.\n");
        return;
    }

    strncpy(filePathCopy, filePath, filePathLen);

    // Remove the ".info" suffix if it exists
    if (filePathLen > 5 && strcmp(filePathCopy + filePathLen - 5, ".info") == 0)
    {
        filePathCopy[filePathLen - 5] = '\0'; // Truncate the .info part
    }

    // Load the DiskObject from the file path
    diskObject = GetDiskObject(filePathCopy);
    if (!diskObject)
    {
        printf("Failed to get DiskObject for the file: %s\n", filePathCopy);
        FreeVec(filePathCopy);
        return;
    }

    toolTypes = diskObject->do_ToolTypes;
    if (!toolTypes)
    {
        printf("Invalid or missing ToolTypes in DiskObject.\n");
        FreeDiskObject(diskObject);
        FreeVec(filePathCopy);
        return;
    }

    // Process ToolTypes to find the "IM1=" prefix and get icon size
    for (i = 0; (toolType = toolTypes[i]); i++)
    {
        if (strncmp(toolType, prefix, strlen(prefix)) == 0)
        {
            if (strlen(toolType) >= 7)
            {
                newIconSize->width = (int)toolType[4] - '!';
                newIconSize->height = (int)toolType[5] - '!';
                // printf("Found NewIcon data: Width = %d, Height = %d\n", newIconSize->width, newIconSize->height);
            }
            break;
        }
    }

    // Cleanup
    FreeDiskObject(diskObject);
    FreeVec(filePathCopy);
}

/* Function to check if the icon file is in the OS3.5 format and return its size */
int getOS35IconSize(const char *filename, IconSize *size)
{
    FILE *file;
    UBYTE buffer[CHUNK_SIZE];
    size_t bytesRead;
    long fileSize;
    int foundForm = 0; /* Flag to indicate if "FORM" has been found */
    int foundIcon = 0; /* Flag to indicate if "ICON" has been found after "FORM" */
    int foundFace = 0; /* Flag to indicate if "FACE" chunk has been found */
    long offset = 0;

    /* Initialize the size */
    size->width = 0;
    size->height = 0;

    /* Open the file in binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("filename: %s\n", filename);
        perror("Error opening file");
        return 0; /* Return false on error */
    }

    /* Get the file size */
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Check if the file size is at least the size of the header */
    if (fileSize < HEADER_SIZE)
    {
        fclose(file);
        return 0; /* File too small to be OS3.5 icon */
    }

    /* Read the file in chunks and search for the headers */
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, file)) > 0)
    {
        int i;

        for (i = 0; i < bytesRead - HEADER_SIZE; i++)
        {
            /* Check for "FORM" header */
            if (!foundForm && memcmp(buffer + i, "FORM", 4) == 0)
            {
                foundForm = 1; /* Found "FORM" */
                offset = ftell(file) - bytesRead + i;
            }

            /* If "FORM" was found, look for "ICON" */
            if (foundForm && memcmp(buffer + i + 8, "ICON", 4) == 0)
            {
                foundIcon = 1; /* Found "ICON" */
            }

            /* If "ICON" was found, look for "FACE" */
            if (foundIcon && memcmp(buffer + i, "FACE", 4) == 0)
            {
                foundFace = 1; /* Found "FACE" chunk */

                /* Read the width and height from the "FACE" chunk */
                size->width = (buffer[i + 8] & 0xFF) + 1;
                size->height = (buffer[i + 9] & 0xFF) + 1;

                break; /* We have what we need, so break the loop */
            }
        }

        if (foundFace)
        {
            break; /* Exit the loop once "FACE" chunk is found */
        }
    }

    /* Close the file */
    fclose(file);

    /* Return true if the "FACE" chunk was found and size was set */
    return foundFace;
}

/* Function to get the standard icon size */
BOOL GetStandardIconSize(const char *filePath, IconSize *iconSize)
{
    struct DiskObject *diskObject;
    char filePathCopy[256]; /* Buffer to hold the modified file path */
    int filePathLength;

    /* Check for NULL pointers */
    if (filePath == NULL || iconSize == NULL)
    {
        return FALSE;
    }

    /* Copy the file path to a local buffer */
    strncpy(filePathCopy, filePath, sizeof(filePathCopy) - 1);
    filePathCopy[sizeof(filePathCopy) - 1] = '\0'; /* Ensure null-termination */
    filePathLength = strlen(filePathCopy);

    /* Remove the ".info" suffix if present */
    if (filePathLength > 5 && strcmp(filePathCopy + filePathLength - 5, ".info") == 0)
    {
        filePathCopy[filePathLength - 5] = '\0';
    }

    /* Attempt to get the DiskObject for the provided (or modified) file path */
    diskObject = GetDiskObject(filePathCopy);
    if (diskObject == NULL)
    {
        return FALSE; /* Failed to get the DiskObject */
    }

    /* Retrieve the width and height from the DiskObject's GfxImage structure */
    iconSize->width = diskObject->do_Gadget.Width;
    iconSize->height = diskObject->do_Gadget.Height;

    /* Free the DiskObject to avoid memory leaks */
    FreeDiskObject(diskObject);

    return TRUE; /* Successfully retrieved and stored the icon size */
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

/* Function to resize a window and move it to a new position */
