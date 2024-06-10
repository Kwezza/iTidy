
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
#define SCROLLBAR_WIDTH 30
#define SCROLLBAR_HEIGHT 20
#define WINDOW_TITLE_HEIGHT 18
#define WORKBENCH_BAR 20

#define FONT_PREFS_FILE "ENV:sys/font.prefs"
#define const_fontIcon 0
#define const_fontDefault 1
#define const_fontScreen 2

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
    int icon_width;
    int icon_height;
    int text_width;
    int text_height;
    char icon_text[40];
    BOOL is_folder;

} FullIconDetails;


struct Screen *screen;
struct Window *window;
struct RastPort *rastPort;
struct TextFont *font;

int screenHight=0;
int screenWidth=0;

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
void removeInfoExtension(const char *input, char *output);
void ResizeAndMoveWindow(const char *windowTitle, int newX, int newY, int newWidth, int newHeight);
int isOS35IconFormat(const char *filename);
int getOS35IconSize(const char *filename, IconSize *size);
int InitializeWindow(void);
void CleanupWindow(void);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        Printf("Usage: %s <directory>\n", argv[0]);
        return RETURN_FAIL;
    }
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
    screenHight=screen->Height;
    screenWidth=screen->Width;
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

    if (HasSlaveFile(path))
    {
        // printf("Found .slave file in %s, resizing windows only and skipping.\n", path);
        //FormatIconsAndWindow(path, TRUE);
        return;
    }

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
        FormatIconsAndWindow(path, FALSE);
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
        // printf("Locked directory: %s\n", folder);
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

void CalculateTextExtent(const char *text,struct TextExtent *textExtent) {
    //struct TextExtent textExtent;
    ULONG textLength = strlen(text);

    if (!rastPort) {
        printf("RastPort is not initialized.\n");
        return;
    }

    // Calculate text extent
    TextExtent(rastPort, text, textLength, textExtent);
    //printf("Text extent: Width = %d, Height = %d\n", textExtent->te_Width, textExtent->te_Height);
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
    char fullPathWithIcon[512];
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
    int textWidth = 0;
    int newTextWidth = 0;
    char fileNameNoInfo[128];
    const char *windowTitle = "TarghanFastFr";

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

    /* if (!(screen = LockPubScreen("Workbench")))
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
                                   WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_SIZEBRIGHT | WFLG_CLOSEGADGET | WFLG_SIZEGADGET,
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
         font = OpenDiskFont((struct TextAttr *)screen->RastPort.Font);
     if (font)
     {
         SetFont(rastPort, font);
     }
     */

    windowWidth = newWidth;
    // Set the font to match the Workbench font


    printf("screen width: %d\n", screenWidth);   
    printf("windowWidth: %d\n", windowWidth);  

    if (Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            if (fib->fib_DirEntryType < 0) /* It's a file */
            {
                const char *fileExtension = strrchr(fib->fib_FileName, '.');
                // printf("File: %s\n", fib->fib_FileName);
                // printf("File Extension: %s\n", fileExtension);
                if (fileExtension != NULL && strncasecmp_custom(fileExtension, ".info", 5) == 0)
                {
                    fileNames = realloc(fileNames, sizeof(char *) * (fileCount + 1));
                    if (fileNames == NULL)
                    {
                        printf("Failed to allocate memory for fileNames\n");
                        break;
                    }

                    fileNames[fileCount] = strdup(fib->fib_FileName);
                    if (fileNames[fileCount] == NULL)
                    {
                        printf("Failed to duplicate file name\n");
                        break;
                    }

                    // printf("fileNames: %s\n", fileNames[fileCount]);
                    fileCount++;
                } /* if file has .info extension */
            } /* if it's a file */
        } /* while ExNext */
    } /* if Examine */

    printf("screen width: %d\n", screenWidth);   
    printf("windowWidth: %d\n", windowWidth);  

    if (!resizeOnly)
    {
        qsort(fileNames, fileCount, sizeof(char *), Compare);
    }

    if (fileCount > MaxIconsToAlign || newWidth > screenWidth - SCROLLBAR_WIDTH)
    {
        windowWidth = screenWidth - SCROLLBAR_WIDTH; // Adjusted window width
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
        strcpy(fullPathWithIcon, fullPath);
        dotInfo = strstr(fullPath, ".info");
        if (dotInfo)
        {
            *dotInfo = '\0'; // Remove ".info" from the path
        }

        removeInfoExtension(fileNames[i], fileNameNoInfo);

        diskObject = GetDiskObject(fullPath);

        // printf("Is os35 icon format: %d\n", isOS35IconFormat(fullPathWithIcon)  );
        if (diskObject)
        {
            
            
                CalculateTextExtent(fileNameNoInfo,&textExtent);

                
                
            /*if (TextExtent(rastPort, fileNameNoInfo, strlen(fileNameNoInfo), &textExtent))
            {
                
                textWidth = textExtent.te_Width;
                printf("Text Width: %d\n", textWidth);
                printf("Text extent: Width = %d, Height = %d\n", textExtent.te_Width, textExtent.te_Height);

                CalculateTextExtent(fileNameNoInfo,&textExtent);
                printf("Text extent: Width = %d, Height = %d\n", textExtent.te_Width, textExtent.te_Height);
            }*/
            // printf("fullPath: %s filename: %s noinfo: %s\n", fullPath, fileNames[i], fileNameNoInfo);
            // newTextWidth=GetIconTextWidth(fileNames[i]);
            // printf("Text Width: %d, new text width %d\n", textWidth, newTextWidth);

            if (IsNewIcon(diskObject))
            {
                // printf("Is new icon\n");
                GetNewIconSize(diskObject, &iconSize);
            }
            else if (isOS35IconFormat(fullPathWithIcon))
            {
                // printf("Is OS3.5 Icon\n");
                getOS35IconSize(fullPathWithIcon, &iconSize);
            }
            else if (diskObject->do_Gadget.Width != 0 && diskObject->do_Gadget.Height != 0)
            {
                // printf("Normal Icon\n");
                iconSize.width = diskObject->do_Gadget.Width;
                iconSize.height = diskObject->do_Gadget.Height;
            }
            else
            {
                // printf("Normal Icon\n");
                iconSize.width = diskObject->do_CurrentX;
                iconSize.height = diskObject->do_CurrentY;
            }
//printf("1  X: %d, Y: %d,  Saved icon: %s \n", x,y,fullPath);
            /* printf("Width (W=%d T=%d), Height (H=%d T=%d) Box: X%d, Y%d to X%d,Y%d Name: %s\n",
                    iconSize.width,
                    textExtent.te_Width,
                    iconSize.height,
                    iconSize.height + textExtent.te_Height,
                    diskObject->do_CurrentX,
                    diskObject->do_CurrentY,
                    diskObject->do_CurrentX + MAX(textExtent.te_Width, iconSize.width),
                    diskObject->do_CurrentY + iconSize.height + textExtent.te_Height,
                    fileNameNoInfo);*/
            // fileNames[i]);

            // Check if the icon text or icon itself exceeds the window width
            printf("3  X: %d, Y: %d, win width: %d, rowcount %d, Current endX %d,  Saved icon: %s \n", x,y,windowWidth,rowcount,x + MAX(textExtent.te_Width, iconSize.width),fullPath);
            if (x + MAX(textExtent.te_Width, iconSize.width) > windowWidth)
            {
                if (rowcount < maxWidthsToCheck)
                {
                    iconWidths[rowcount] = x;
                }
                x = ICON_START_X;
                y += iconSize.height + textExtent.te_Height + ICON_SPACING_Y;
                rowcount++;
            }
//printf("3  X: %d, Y: %d, window width: %d, rowcount %d, Current endX %d,  Saved icon: %s \n", x,y,windowWidth,rowcount,x + MAX(textExtent.te_Width, iconSize.width),fullPath);
            // Center the icon within the space calculated based on text width or icon width
            centerX = x + (MAX(textExtent.te_Width, iconSize.width) - iconSize.width) / 2;

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
                    printf("4  X: %d, Y: %d, window width: %d, Saved icon: %s \n", x,y,windowWidth,fullPath);
                }
            }
            else
            {
                x = diskObject->do_CurrentX;
                y = diskObject->do_CurrentY;
            }

            // if (x + MAX(textExtent.te_Width, iconSize.width) > maxX)
            //{
            maxX = MAX(maxX, (diskObject->do_CurrentX + MAX(textExtent.te_Width, iconSize.width)));
            maxY = MAX(maxY, (diskObject->do_CurrentY + iconSize.height + textExtent.te_Height));
            //}
            // if (y + iconSize.height + textExtent.te_Height > maxY)
            //{
            //    maxY = y + iconSize.height + textExtent.te_Height;
            //}
            // printf("maxX: %d, maxY: %d\n ", maxX, maxY);
            // Increment x position
            x += MAX(textExtent.te_Width, iconSize.width) + ICON_SPACING_X;

            FreeDiskObject(diskObject);
        }
        else
        {
        }
        free(fileNames[i]);
        // printf("\n");
        //  printf("MaxX: %d, MaxY: %d\n", maxX, maxY);
        //  printf("fileCount: %d\n", fileCount);
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
    // maxX = maxX + SCROLLBAR_WIDTH; // Ensure width is the window width
    // maxY += SCROLLBAR_HEIGHT;             // Add height for scrollbar if needed
    posTop = (screenHight - maxY) / 2;
    posLeft = (screenWidth - maxX) / 2;

    maxX = maxX + SCROLLBAR_WIDTH;
    maxY = maxY + SCROLLBAR_HEIGHT + WINDOW_TITLE_HEIGHT;
    // WindowToFront(myWindow);

    // printf("Resized window to fit all icons: Width = %ld, Height = %ld\n", maxX, maxY);
    newFolderInfo.left = posTop;
    newFolderInfo.top = posLeft;
    newFolderInfo.width = maxX;

    // if (!resizeOnly)
    //{
    //     if (maxY > screen->Height - WORKBENCH_BAR)
    //     {
    //         maxY = screen->Height + WORKBENCH_BAR;
    //     }
    //     else
    //     {
    //         //maxY = maxY + WORKBENCH_BAR;
    //     }
    // }

    newFolderInfo.height = maxY;
    // SaveFolderSettings(dirPath, &newFolderInfo);
    //  GetFolderInfo(dirPath, &newFolderInfo);
    //   printf("Folder Info: left = %d, top = %d, width = %d, height = %d\n", newFolderInfo.left, newFolderInfo.top, newFolderInfo.width, newFolderInfo.height);
    //    Save new window size to DiskObject
    //    SaveNewWindowSize(diskObject, dirPath, maxX, maxY);

    // printf("Window resized to fit all icons: Width = %ld, Height = %ld\n", maxX, maxY);
    // printf("Window position: Left =  %d, Top = %d \n", posTop, posLeft);
    // printf("BorderLeft: %d, BorderRight: %d, BorderTop: %d,BorderBottom: %d\n", window->BorderLeft, window->BorderRight, window->BorderTop, window->BorderBottom);
    // printf("Screen dimensions: Width = %ld, Height = %ld\n", screen->Width, screen->Height);
    // CloseWindow(window);
    SaveFolderSettings(dirPath, &newFolderInfo);
    // printf("Closed window on Workbench screen.\n");
    // UnlockPubScreen("Workbench", screen);
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
    // if (font)
    //{
    //    CloseFont(font);
    //}

    if (SkipAutoResize)
        minAverageWidthPercent = 0;
    // printf("newindow width: %d\n", newWidth);

    // newFolderInfo.left = posTop;
    // ResizeAndMoveWindow(windowTitle, newFolderInfo.left, newFolderInfo.height, newFolderInfo.width, newFolderInfo.height);

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
void ResizeAndMoveWindow(const char *windowTitle, int newX, int newY, int newWidth, int newHeight)
{
    struct Screen *screen;
    struct Window *window;

    /* Lock the public screen */
    screen = LockPubScreen(NULL);
    if (screen == NULL)
    {
        printf("Failed to lock public screen.\n");
        return;
    }

    /* Find the window with the specified title */
    window = screen->FirstWindow;
    while (window != NULL)
    {
        if (window->Title != NULL && strcmp(window->Title, windowTitle) == 0)
        {
            /* Resize the window */
            SizeWindow(window, newWidth - window->Width, newHeight - window->Height);

            /* Move the window to the new position */
            MoveWindow(window, newX - window->LeftEdge, newY - window->TopEdge);

            /* Print the window details */
            printf("Window Title: %s\n", window->Title);
            printf("Window Left: %d\n", window->LeftEdge);
            printf("Window top: %d\n", window->TopEdge);
            printf("Window Width: %d\n", window->Width);
            printf("Window Height: %d\n", window->Height);
            printf("Border Left: %d\n", window->BorderLeft);
            printf("Border Top: %d\n", window->BorderTop);
            printf("Border Right: %d\n", window->BorderRight);
            printf("Border Bottom: %d\n", window->BorderBottom);
            break;
        }
        window = window->NextWindow;
    }

    /* Unlock the public screen */
    UnlockPubScreen(NULL, screen);
}
