#ifndef MAIN_H
#define MAIN_H

/* VBCC MIGRATION NOTE: Standard C headers first */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

/* VBCC MIGRATION NOTE: Platform abstraction layer */
#include <platform/platform.h>
#include <platform/amiga_headers.h>

/* VBCC MIGRATION NOTE: Amiga-specific includes wrapped in platform guard */
#ifdef __AMIGA__
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/dos.h>
#include <dos/dos.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/wb.h>
#include <proto/diskfont.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#endif

#include "Settings/IControlPrefs.h"
#include "Settings/WorkbenchPrefs.h"
#include "Settings/IControlPrefs.h"
#include "Settings/WorkbenchPrefs.h"
#include "utilities.h"
#include "spinner.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "dos/getDiskDetails.h"
#include "Settings/get_fonts.h"
#include "cli_utilities.h"

//#include "icon_management.h"
//#include "file_directory_handling.h"

// Define the DEBUG macro to enable debug prints
//#define DEBUG
//#define DEBUGLocks

#define VERSION "1.0.0"

#define ICON_START_X 10
#define ICON_START_Y 10
/* DEPRECATED: Scrollbar sizes now obtained from IControl preferences (currentBarWidth/currentBarHeight) */
/* #define SCROLLBAR_WIDTH 50 */
/* #define SCROLLBAR_HEIGHT 30 */
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
#define ERROR_LIST_INITIAL_SIZE 10

#define textBlack "\x1B[31m"
#define textBlue "\x1B[33m"
#define textBold "\x1B[1m"
#define textGrey "\x1B[30m"
#define textItalic "\x1B[3m"
#define textReset "\x1B[0m"
#define textReverse "\x1B[7m"
#define textUnderline "\x1B[4m"
#define textWhite "\x1B[32m"
#define textClearLine "\x1B[1G\x1B[K"

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
    ULONG file_size;              /* File size in bytes (0 for folders) */
    struct DateStamp file_date;   /* File modification date */
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

typedef struct IconErrorTrackerStruct {
    STRPTR *list;   // Array of string pointers
    ULONG size;     // Allocated size of the list
    ULONG count;    // Number of errors stored
} IconErrorTrackerStruct;

extern struct Screen *screen;
extern struct Window *window;
extern struct RastPort *rastPort;
extern struct TextFont *font;
extern struct WorkbenchSettings prefsWorkbench;
extern struct IControlPrefsDetails prefsIControl;
extern struct IconErrorTrackerStruct iconsErrorTracker;
extern int screenHight;
extern int screenWidth;
extern int WindowWidthTextOnly;
extern int WindowHeightTextOnly;
extern int ICON_SPACING_X;
extern int ICON_SPACING_Y;
extern int icon_type_standard;
extern int icon_type_newIcon;
extern int icon_type_os35;

extern int count_icon_type_standard;
extern int count_icon_type_newIcon;
extern int count_icon_type_os35;
extern int count_icon_corrupted;

extern char filePath[256];

extern BOOL user_dontResize;
extern BOOL user_cleanupWHDLoadFolders;
extern BOOL user_folderViewMode;
extern BOOL user_folderFlags;
extern BOOL user_stripIconPosition;
extern BOOL user_forceStandardIcons;



void AddIconError(IconErrorTrackerStruct *tracker, STRPTR filePath);
void FreeIconErrorList(IconErrorTrackerStruct *tracker);
int endsWithFont(const char *str) ;
#endif // MAIN_H