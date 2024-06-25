#ifndef MAIN_H
#define MAIN_H

#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/wb.h>
#include <proto/diskfont.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>
#include <graphics/text.h>
#include <graphics/rastport.h>
#include <ctype.h>
#include "Settings/IControlPrefs.h"
#include "Settings/WorkbenchPrefs.h"

#define ICON_SPACING_X 10
#define ICON_SPACING_Y 10
#define ICON_START_X 10
#define ICON_START_Y 10
#define SCROLLBAR_WIDTH 50
#define SCROLLBAR_HEIGHT 30
#define WINDOW_TITLE_HEIGHT 30
#define WORKBENCH_BAR 20
#define GAP_BETWEEN_ICON_AND_TEXT 2
#define PADDING_WIDTH 4
#define PADDING_HEIGHT 4
#define FONT_PREFS_FILE "ENV:sys/font.prefs"
#define const_fontIcon 0
#define const_fontDefault 1
#define const_fontScreen 2
#define MAX_PATH_LEN 256
#define CHUNK_SIZE 1024
#define HEADER_SIZE 12
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX_ICONS_TO_ALLIGN 50

typedef struct {
    int left;
    int top;
    int width;
    int height;
} folderWindowSize;

typedef struct {
    int x;
    int y;
} IconPosition;

typedef struct {
    int icon_x;
    int icon_y;
    int icon_width;
    int icon_height;
    int text_width;
    int text_height;
    int icon_max_width;
    int icon_max_height;
    BOOL has_border;
    char *icon_text;
    char *icon_full_path;
    BOOL is_folder;
} FullIconDetails;

typedef struct {
    FullIconDetails *array;
    size_t size;
    size_t capacity;
    size_t BiggestWidthPX;
    BOOL hasOnlyBorderlessIcons;
} IconArray;

typedef struct {
    int width;
    int height;
} IconSize;

extern struct Screen *screen;
extern struct Window *window;
extern struct RastPort *rastPort;
extern struct TextFont *font;
extern struct WorkbenchSettings prefsWorkbench;
extern struct IControlPrefsDetails prefsIControl;
extern int screenHight;
extern int screenWidth;

BOOL checkIconFrame(const char *iconName);
int Compare(const void *a, const void *b);
int HasSlaveFile(char *path);
int IsRootDirectorySimple(char *path);
int strncasecmp_custom(const char *s1, const char *s2, size_t n);
void CalculateTextExtent(const char *text, struct TextExtent *textExtent);
void dumpIconArrayToScreen(IconArray *iconArray);
void GetFullPath(const char *directory, struct FileInfoBlock *fib, char *fullPath, int fullPathSize);
void pause_program(void);
void ProcessDirectory(char *path, BOOL processSubDirs);
void removeInfoExtension(const char *input, char *output);
void SaveFolderSettings(const char *folderPath, folderWindowSize *newFolderInfo);
int saveIconsPositionsToDisk(IconArray *iconArray);

#endif // MAIN_H
