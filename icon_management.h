#ifndef ICON_MANAGEMENT_H
#define ICON_MANAGEMENT_H

#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

// Custom struct for icon position (if applicable)
typedef struct {
    int x; // X position of the icon
    int y; // Y position of the icon
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
    BOOL has_border;
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
    BOOL hasOnlyBorderlessIcons;
} IconArray;

typedef struct
{
    int width;
    int height;
} IconSize;

typedef struct
{
    int x;
    int y;
    BOOL isBorderless;
} IconPosition;

// Function Prototypes




//BOOL checkIconFrame(struct DiskObject *icon);

int getOS35IconSize(const char *filename, IconSize *size);

IconPosition GetIconPositionFromPath(const char *iconPath);
IconArray *CreateIconArray(void);
void FreeIconArray(IconArray *iconArray);
BOOL AddIconToArray(IconArray *iconArray, const FullIconDetails *newIcon);
IconArray *CreateIconArrayFromPath(BPTR lock, const char *dirPath);
void GetNewIconSizePath(const char *filePath, IconSize *newIconSize);
BOOL GetStandardIconSize(const char *filePath, IconSize *iconSize);
BOOL IsNewIconPath(const STRPTR filePath);
int isOS35IconFormat(const char *filename);
BOOL IsNewIcon(struct DiskObject *diskObject);
#endif // ICON_MANAGEMENT_H