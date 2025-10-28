#ifndef ITIDY_TYPES_H
#define ITIDY_TYPES_H

/* Common type definitions for iTidy
 * These types are shared between main.c and main_gui.c
 */

#include <exec/types.h>

/* Constants */
#define ERROR_LIST_INITIAL_SIZE 10
#define VERSION "2.0"
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define GAP_BETWEEN_ICON_AND_TEXT 2
#define ICON_START_X 10
#define ICON_START_Y 8
#define ICON_SPACING_X 9
#define ICON_SPACING_Y 7
#define PADDING_WIDTH 10
#define PADDING_HEIGHT 10
#define ID_FONT MAKE_ID('F','O','N','T')

/* Forward declarations for external structures */
struct WorkbenchSettings;
struct IControlPrefsDetails;

/* Window size information */
typedef struct {
    int left;
    int top;
    int width;
    int height;
} folderWindowSize;

/* Icon position */
typedef struct {
    int x;
    int y;
} IconPosition;

/* Icon size */
typedef struct {
    int width;
    int height;
} IconSize;

/* Full icon details */
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
    ULONG file_size;
    struct DateStamp file_date;
} FullIconDetails;

/* Icon array */
typedef struct {
    FullIconDetails *array;
    size_t size;
    size_t capacity;
    size_t BiggestWidthPX;
    BOOL hasOnlyBorderlessIcons;
} IconArray;

/* Icon error tracker */
typedef struct IconErrorTrackerStruct {
    STRPTR *list;   // Array of string pointers
    ULONG size;     // Allocated size of the list
    ULONG count;    // Number of errors stored
} IconErrorTrackerStruct;

/* External global variables (defined in main_gui.c or main.c) */
extern int icon_type_standard;
extern int icon_type_newIcon;
extern int icon_type_os35;

extern int count_icon_type_standard;
extern int count_icon_type_newIcon;
extern int count_icon_type_os35;
extern int count_icon_corrupted;

extern BOOL user_dontResize;
extern BOOL user_cleanupWHDLoadFolders;
extern BOOL user_folderViewMode;
extern BOOL user_folderFlags;
extern BOOL user_stripIconPosition;
extern BOOL user_forceStandardIcons;

extern struct WorkbenchSettings prefsWorkbench;
extern struct IControlPrefsDetails prefsIControl;
extern struct IconErrorTrackerStruct iconsErrorTracker;

extern int screenHight;
extern int screenWidth;
extern int WindowWidthTextOnly;
extern int WindowHeightTextOnly;

extern char filePath[256];

extern struct Screen *screen;
extern struct Window *window;
extern struct RastPort *rastPort;
extern struct TextFont *font;

/* Function declarations */
void AddIconError(IconErrorTrackerStruct *tracker, STRPTR filePath);

#endif // ITIDY_TYPES_H
