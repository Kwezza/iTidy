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
#include "main.h"
#include "icon_management.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "utilities.h"

/* Define global variables */
struct Screen *screen = NULL;
struct Window *window = NULL;
struct RastPort *rastPort = NULL;
struct TextFont *font = NULL;
struct WorkbenchSettings prefsWorkbench;
struct IControlPrefsDetails prefsIControl;
int screenHight = 0;
int screenWidth = 0;
int WindowWidthTextOnly = 430;
int WindowHeightTextOnly = 256;
int ICON_SPACING_X = 5;
int ICON_SPACING_Y = 5;
BOOL user_dontResize;
BOOL user_cleanupWHDLoadFolders;
BOOL user_folderViewMode;
BOOL user_folderFlags;

void print_usage(const char *program_name);

void print_usage(const char *program_name) {
    printf("Usage: %s <directory> -iterateSubDIRs -dontResizeWindow -folderViewShowAll -folderViewDefault -folderViewByName -folderViewByType -cleanupWHDFolders\n", program_name);
}

int main(int argc, char **argv)
{
    char filePath[256];
    int i;
    int workbenchVersion = GetWorkbenchVersion();
    BOOL iterateDIRs = FALSE;
    char *stringWBVersion;

    user_dontResize = FALSE;
    user_folderViewMode = DDVM_BYICON;
    user_folderFlags = DDFLAGS_SHOWICONS;
    user_cleanupWHDLoadFolders = FALSE;

#ifdef DEBUG
    printf("Debug build\n");

    stringWBVersion = convertWBVersionWithDot(workbenchVersion);
    printf("Workbench version: %s\n", stringWBVersion);
    free(stringWBVersion);
#endif
    if (workbenchVersion < 36000)
    {
        printf("This program requires Workbench 2.0 or higher.\n"); /* The official librarys dont give me access to the function i use to position icons */
        return RETURN_FAIL;                                         /* I might be able to open the icon in binary mode and read the position data, but thats a ponder for another day. */
    }

    if (workbenchVersion < 44500)
    {
        ICON_SPACING_X = 15; /*  Limited icon library support pre workbench 3.1.4 */
        ICON_SPACING_Y = 10; /*  Some functions are disabled */
#ifdef DEBUG
        printf("Workbench 2.x detected. Icon spacing increased to x: %d y: %d\n", ICON_SPACING_X, ICON_SPACING_Y);
#endif
    }

    if (argc < 2)
    {
        print_usage(argv[0]);
        return RETURN_FAIL;
    }
    strcpy(filePath, argv[1]);
    sanitizeAmigaPath(filePath);
    if (does_file_or_folder_exist(filePath, 0) == FALSE)
    {
        printf("The directory %s does not exist\n", filePath);
        return RETURN_FAIL;
    }

    for (i = 1; i < argc; i++) /* Start from 1 to skip program name */
    {
        if (strncasecmp_custom(argv[i], "-iterateSubDIRs", strlen(argv[i])) == 0) /* Walk through any subfolders from the parent folder */
            iterateDIRs = TRUE;
        else if (strncasecmp_custom(argv[i], "-dontResizeWindow", strlen(argv[i])) == 0) /* Don't resize and center the Workbench window to fit the icons */
            user_dontResize = TRUE;
        else if (strncasecmp_custom(argv[i], "-folderViewShowAll", strlen(argv[i])) == 0) /* Set the Workbench folder view to show all files, even those without icons */
            user_folderFlags = DDFLAGS_SHOWALL;
        else if (strncasecmp_custom(argv[i], "-folderViewDefault", strlen(argv[i])) == 0) /* Set the Workbench folder view to default (inherit parent's view mode) */
            user_folderViewMode = DDFLAGS_SHOWDEFAULT;
        else if (strncasecmp_custom(argv[i], "-folderViewByName", strlen(argv[i])) == 0) /* Set the Workbench folder view as text, sorted by name */
            user_folderViewMode = DDVM_BYNAME;
        else if (strncasecmp_custom(argv[i], "-folderViewByType", strlen(argv[i])) == 0) /* Set the Workbench folder view as text, sorted by type */
            user_folderViewMode = DDVM_BYTYPE;
        else if (strncasecmp_custom(argv[i], "-cleanupWHDFolders", strlen(argv[i])) == 0) /* By design, this program was created to clean up extracted WHDLoad folders, and skip rearranging the original author's layout. This option forces the program to rearrange the icons. */
            user_cleanupWHDLoadFolders = TRUE;
        else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return RETURN_FAIL;
        }
    }

    fetchWorkbenchSettings(&prefsWorkbench);
    fetchIControlSettings(&prefsIControl);

#ifdef DEBUG
    printf("IControl prefs:  border width: %d, border height: %d\n", prefsIControl.currentBarWidth, prefsIControl.currentBarHeight);
    printf("IControl prefs:  title height: %d, Window height: %d\n", prefsIControl.currentTitleBarHeight, prefsIControl.currentWindowBarHeight);
    printf("IControl prefs:  volume guage width: %d\n", prefsIControl.currentCGaugeWidth);
#endif

    InitializeWindow();
    ProcessDirectory(filePath, iterateDIRs);
    CleanupWindow();
    return RETURN_OK;
}