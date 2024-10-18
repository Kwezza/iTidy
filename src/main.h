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
#include <proto/diskfont.h> // Include diskfont library
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/utility.h>
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>
#include <proto/iffparse.h>
#include <graphics/text.h>
#include <graphics/rastport.h>
#include <ctype.h>

#include "Settings/IControlPrefs.h"
#include "Settings/WorkbenchPrefs.h"
#include "Settings/IControlPrefs.h"
#include "Settings/WorkbenchPrefs.h"
#include "utilities.h"
#include "spinner.h"
#include "writeLog.h"
#include "icon_misc.h"

// Define the DEBUG macro to enable debug prints
//#define DEBUG
//#define DEBUGLocks


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



#define textBlack "\x1B[31m"
#define textBlue "\x1B[33m"
#define textBold "\x1B[1m"
#define textGrey "\x1B[30m"
#define textItalic "\x1B[3m"
#define textReset "\x1B[0m"
#define textReverse "\x1B[7m"
#define textUnderline "\x1B[4m"
#define textWhite "\x1B[32m"



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
    int icon_type;
    int border_width;
    char *icon_text;
    char *icon_full_path;
    BOOL is_folder;
    BOOL is_write_protected;
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
extern int WindowWidthTextOnly;
extern int WindowHeightTextOnly;
extern int ICON_SPACING_X;
extern int ICON_SPACING_Y;
extern int icon_type_standard;
extern int icon_type_newIcon;
extern int icon_type_os35;

extern BOOL user_dontResize;
extern BOOL user_cleanupWHDLoadFolders;
extern BOOL user_folderViewMode;
extern BOOL user_folderFlags;
extern BOOL user_stripIconPosition;

//used to store a list of icons that have been marked as to be left out on the workbench desktop
extern char **left_out_icons;
extern int left_out_count;
extern int left_out_size;


int Compare(const void *a, const void *b);
int strncasecmp_custom(const char *s1, const char *s2, size_t n);
void CalculateTextExtent(const char *text, struct TextExtent *textExtent);

#endif // MAIN_H
