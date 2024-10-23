/*
    Icon Cleanup

    Icon Cleanup is an Amiga CLI tool that tidies up icons, then centres and resizes windows to fit the icons.

    It has the following features:
    * Iterates through a directory and its subdirectories, cleaning up icons
    * Can resize and center windows to fit icons relative to the screen resolution
    * Supports Workbench 2.0 and higher
    * Takes the current font settings into account when working out icon widths
    * Considers any custom border, window, and title settings when calculating the window size
    * Has the option to skip WHDLoad folders if the author's layout is preferred (Window will still be resized and centred)
    * Can set folder view mode to View by icon, or view by name only

    I wrote this program because I wasn't happy with the default placement of icons when copied or extracted from archives.
    Also, I was not too fond of opening folders with a few icons and then having to scroll or resize the window to see them all.
    Originally designed to be used with an update to WHDArchiveExtractor (for extracting WHDLoad archives en masse), I have since
    found it useful for keeping my system tidy and decided to release it for others to, hopefully, find it useful. As it runs from
    the CLI, it can be incorporated into larger scripts that preinstall other software and then used to tidy up the icons and
    windows afterwards.

    The program is designed to be run from the CLI and takes the following arguments:
    Usage: IconCleanup <directory> -iterateSubDIRs -dontResizeWindow -folderViewShowAll -folderViewDefault -folderViewByName
                       -folderViewByType -dontCleanupWHDFolders
    Where:
        <directory> is the folder to start the cleanup from
        -subdirs will walk through any subfolders from the parent folder
        -dontResize will not resize and centre the Workbench window to fit the icons
        -ViewShowAll will set the Workbench folder view to show all files, even those without icons
        -ViewDefault will set the Workbench folder view to default (inherit parent's view mode)
        -ViewByName will set the Workbench folder view as text, sorted by name
        -ViewByType will set the Workbench folder view as text, sorted by type
        -resetIcons will remove saved icon positions.  Instructs Workbench to pick a reasonable place for the icon when it opens the folder
        -dontCleanupWHDFolders will force the program to maintain the icon position in WHDLoad folders but will resize and centre the window to fit the icons

    I have been using this program on Workbench 3.2, and have only done limited testing on Workbench 2.
    Please let me know if you have any issues via email, or better still, open a ticket on the GitHub page.
    As always, although it works for me, this program is provided as is, and I take no responsibility for any issues it may cause.
    Please back up your system before running this program.

    v1.0.0 - 2024-07-11 - First public release.
    */

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
#include <time.h>

#include "Settings/IControlPrefs.h"
#include "Settings/WorkbenchPrefs.h"
#include "main.h"
#include "icon_management.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "utilities.h"
#include "spinner.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "dos/getDiskDetails.h"

/* Define global variables */
struct Screen *screen = NULL;
struct Window *window = NULL;
struct RastPort *rastPort = NULL;
struct TextFont *font = NULL;
struct WorkbenchSettings prefsWorkbench;
struct IControlPrefsDetails prefsIControl;
struct IconErrorTrackerStruct iconsErrorTracker;
int screenHight = 0;
int screenWidth = 0;
int WindowWidthTextOnly = 430;
int WindowHeightTextOnly = 256;
int ICON_SPACING_X = 9;
int ICON_SPACING_Y = 9;
int icon_type_standard = 0;
int icon_type_newIcon = 1;
int icon_type_os35 = 2;

int count_icon_type_standard = 0;
int count_icon_type_newIcon = 0;
int count_icon_type_os35 = 0;
int count_icon_corrupted = 0;

BOOL user_dontResize;
BOOL user_cleanupWHDLoadFolders;
BOOL user_folderViewMode;
BOOL user_folderFlags;
BOOL user_stripIconPosition;

#define VERSION_STRING "$VER: iTidy 1.0 (15.07.2024)"
const char version[] = VERSION_STRING;

void print_usage(const char *program_name);

/* Function to add an icon file path to the error list */
void AddIconError(IconErrorTrackerStruct *tracker, STRPTR filePath)
{
    STRPTR *newList;
    ULONG i;

    /* Check if we need to expand the list */
    if (tracker->count == tracker->size)
    {
        /* Double the size of the array */
        ULONG newSize = tracker->size * 2;
        newList = AllocVec(newSize * sizeof(STRPTR), MEMF_CLEAR);
        if (newList == NULL)
        {
            printf("Error reallocating memory for icon error list\n");
            return;
        }

        /* Copy old data */
        for (i = 0; i < tracker->count; i++)
        {
            newList[i] = tracker->list[i];
        }

        /* Free the old array */
        FreeVec(tracker->list);

        /* Update tracker */
        tracker->list = newList;
        tracker->size = newSize;
    }

    /* Allocate memory for the file path and store it */
    tracker->list[tracker->count] = AllocVec(strlen(filePath) + 1, MEMF_CLEAR);
    if (tracker->list[tracker->count] != NULL)
    {
        strcpy(tracker->list[tracker->count], filePath);
        tracker->count++;
    }
    else
    {
        printf("Error allocating memory for icon file path\n");
    }
}

/* Function to free the memory used by the error list */
void FreeIconErrorList(IconErrorTrackerStruct *tracker)
{
    ULONG i;

    /* Free all the strings */
    for (i = 0; i < tracker->count; i++)
    {
        if (tracker->list[i] != NULL)
        {
            FreeVec(tracker->list[i]);
        }
    }

    /* Free the array */
    FreeVec(tracker->list);

    /* Reset tracker fields */
    tracker->list = NULL;
    tracker->size = 0;
    tracker->count = 0;
}

void print_usage(const char *program_name)
{
 /*
    printf("\n");
    printf("" textReset textBold "Icon Tidy V1.0" textReset ".  A program to tidy icons, and resize folder windows from CLI.\n");
    printf("Usage: iTidy <directory> [options]\n");
    printf("Where:\n");
    printf("    " textReset textBold "<directory>   " textReset "The folder to start the cleanup from (mandatory)\n");
    printf("    " textReset textBold "-subdirs      " textReset "Recursively process subfolders\n");
    printf("    " textReset textBold "-dontResize   " textReset "Do not resize and centre the folder\n");
    printf("    " textReset textBold "-ViewShowAll  " textReset "Show all files, including those without icons\n");
    printf("    " textReset textBold "-ViewDefault  " textReset "Use default view settings\n");
    printf("    " textReset textBold "-ViewByName   " textReset "Set folder view as text, sorted by name\n");
    printf("    " textReset textBold "-ViewByType   " textReset "Set folder view as text, sorted by type\n");
    printf("    " textReset textBold "-resetIcons   " textReset "Remove saved icon posistions.\n");
    printf("    " textReset textBold "-skipWHD      " textReset "Keep WHDLoad folder icon positions, but do resize\n");
    printf("\n");
    printf("This program is provided 'as is' without any warranty of any kind. The \n");
    printf("author assumes no responsibility for any damage or issues that may arise \n");
    printf("from using this program. It is strongly recommended that you back up your\n");
    printf("system before running this program.\n\n");*/
printf("iTidy V1.0.0 - Tidy icons and resize folder windows from the CLI.\n");
printf("\n");
printf("Usage:\n");
printf("  iTidy <directory> [options]\n");
printf("\n");
printf("Options:\n");
printf("  <directory>    (Mandatory) The folder to start the cleanup from.\n");
printf("  -subdirs       Recursively process subfolders.\n");
printf("  -dontResize    Do not resize and center the window.\n");
printf("  -viewShowAll   Show all files, including those without icons.\n");
printf("  -viewDefault   Use default view settings.\n");
printf("  -viewByName    Set view to list mode, sorted by name.\n");
printf("  -viewByType    Set view to list mode, sorted by type.\n");
printf("  -resetIcons    Remove saved icon positions.\n");
printf("  -skipWHD       Keep WHDLoad icon positions but do resize.\n");
printf("\n");
printf("Examples:\n");
printf("  iTidy Work:Projects -subdirs\n");
printf("    Recursively tidy 'Work:Projects' and its subfolders.\n");
printf("\n");
printf("  iTidy DF0: -viewByName -resetIcons\n");
printf("    Tidy 'DF0:' folder, set view by name, and remove icon positions.\n");
printf("\n");
printf("Important: Back up your system before running.\n");
printf("\n");
printf("Disclaimer: This program is provided 'as is' without any warranty.\n");
printf("The author assumes no responsibility for any issues that may arise.\n");
}

int main(int argc, char **argv)
{
    char filePath[256];
    int i;
    int workbenchVersion = GetWorkbenchVersion();
    int numLeftOutIcons = 0;
    char deviceName[25];
    BOOL iterateDIRs = FALSE;
    long elapsed_seconds, hours, minutes, seconds, start_time;
    DeviceInfo diskInfo;

#ifdef DEBUG
    char *stringWBVersion;
#endif
    user_dontResize = FALSE;
    user_folderViewMode = DDVM_BYICON;
    user_folderFlags = DDFLAGS_SHOWICONS;
    user_cleanupWHDLoadFolders = TRUE;
    user_stripIconPosition = FALSE;

    if (setupTimer() != 0)
    {
        printf("Failed to setup timer\n");
        return 1;
    }

    /* Initialize the error tracker */
    iconsErrorTracker.size = ERROR_LIST_INITIAL_SIZE;
    iconsErrorTracker.count = 0;
    iconsErrorTracker.list = AllocVec(iconsErrorTracker.size * sizeof(STRPTR), MEMF_CLEAR);

    if (iconsErrorTracker.list == NULL)
    {
        printf("Error allocating memory for icon error list\n");
        return 1;
    }

    /* Example of adding errors */
    // AddIconError(&iconsErrorTracker, "TH0:bad_icon.info");
    // AddIconError(&iconsErrorTracker, "TH0:corrupt_icon.info");
    // count_icon_corrupted=2;

    printf(textBold "\niTidy" textReset " V%s\nby Kerry Thompson\n", VERSION);
    printf("Version compiled on %s at %s\n\n", __DATE__, __TIME__);

#ifdef DEBUG
    initialize_logfile();
    append_to_log("Debug build\n");

    stringWBVersion = convertWBVersionWithDot(workbenchVersion);
    append_to_log("Workbench version: %s\n", stringWBVersion);
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
        append_to_log("Workbench 2.x detected. Icon spacing increased to x: %d y: %d\n", ICON_SPACING_X, ICON_SPACING_Y);
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

    for (i = 0; i < argc; i++)
    {
        if (strncasecmp_custom(argv[i], "-subdirs", strlen(argv[i])) == 0) /* Walk through all folders found in the the parent folder */
            iterateDIRs = TRUE;
        if (strncasecmp_custom(argv[i], "-dontResize", strlen(argv[i])) == 0) /* Don't resize and center the Workbench window to fit after arranging icons */
            user_dontResize = TRUE;
        if (strncasecmp_custom(argv[i], "-ViewShowAll", strlen(argv[i])) == 0) /* Set the Workbench folder view to show all files, even those without icons.  May require a  */
            user_folderFlags = DDFLAGS_SHOWALL;
        if (strncasecmp_custom(argv[i], "-ViewDefault", strlen(argv[i])) == 0) /* Set the Workbench folder view to default (inherit parent's view mode) */
            user_folderViewMode = DDFLAGS_SHOWDEFAULT;
        if (strncasecmp_custom(argv[i], "-ViewByName", strlen(argv[i])) == 0) /* Set the Workbench folder view as text only (no icons), sorted by name */
            user_folderViewMode = DDVM_BYNAME;
        if (strncasecmp_custom(argv[i], "-ViewByType", strlen(argv[i])) == 0) /* Set the Workbench folder view as text only (no icons), sorted by type.  Handy for sorting directories first.*/
            user_folderViewMode = DDVM_BYTYPE;
        if (strncasecmp_custom(argv[i], "-resetIcons", strlen(argv[i])) == 0) /* Removes each icon's x and y position.  Instructs Workbench to pick a reasonable place for the icon whens it opends the folder*/
            user_stripIconPosition = TRUE;
        if (strncasecmp_custom(argv[i], "-skipWHD", strlen(argv[i])) == 0) /* By design, this program was created to clean up extracted WHDLoad folders, and skip rearranging the original author's layout.
                                                                                            This option forces the program to rearrange the icons if it detects a .slave file is found in the folder.  Without this option,
                                                                                            the fodler's window will just be resized and centered. */
            user_cleanupWHDLoadFolders = FALSE;
    }

    
    diskInfo = GetDeviceInfo(filePath);
    if (diskInfo.writeProtected)
    {
        printf("Device %s is read-only.  Exiting.\n\n", diskInfo.deviceName);
        return RETURN_FAIL;
    }
    printf("Tidying icons in %s\n", filePath);

    //printf("\ntest Device %s has %d%s\n", diskInfo.deviceName,diskInfo.free,diskInfo.storageUnits);

    /* Start timer */
    start_time = time(NULL);

    /*getDeviceNameFromPath(filePath, deviceName, 25);
printf("is device %s read only? %d\n",deviceName,IsDeviceReadOnly(deviceName));

    if (IsDeviceReadOnly(deviceName))
    {
        printf("\nDevice %s is read-only.  Exiting.\n", deviceName);
        return RETURN_FAIL;
    }
*/
    fetchWorkbenchSettings(&prefsWorkbench);
    fetchIControlSettings(&prefsIControl);

    loadLeftOutIcons(filePath);
    numLeftOutIcons = countLeftOutIcons();

    if (numLeftOutIcons > 0)
    {
        printf("\nFound %d icons left out on the Workbench on device '%s'\n\n", numLeftOutIcons, deviceName);
    }

#ifdef DEBUG
    append_to_log("IControl prefs:  border width: %d, border height: %d\n", prefsIControl.currentBarWidth, prefsIControl.currentBarHeight);
    append_to_log("IControl prefs:  title height: %d, Window height: %d\n", prefsIControl.currentTitleBarHeight, prefsIControl.currentWindowBarHeight);
    append_to_log("IControl prefs:  volume guage width: %d\n", prefsIControl.currentCGaugeWidth);
    // Print the loaded left out icons
    dumpLeftOutIcons();
#endif



    InitializeWindow();
    ProcessDirectory(filePath, iterateDIRs, 0);
    CleanupWindow();
    disposeTimer();

    /* Calculate elapsed time */
    elapsed_seconds = time(NULL) - start_time;
    hours = elapsed_seconds / 3600;
    minutes = (elapsed_seconds % 3600) / 60;
    seconds = elapsed_seconds % 60;

    printf("\n\nTotal icons tidied: %d\n" textReset, count_icon_type_newIcon + count_icon_type_standard + count_icon_type_os35);
    printf(textWhite "  -" textReset " Standard icons:    %d\n", count_icon_type_standard);
    printf(textWhite "  -" textReset " NewIcon icons:     %d\n", count_icon_type_newIcon);
    printf(textWhite "  -" textReset " OS3.5 style icons: %d\n", count_icon_type_os35);
    if (iconsErrorTracker.count > 0)
    {
        printf("\nTotal corrupted icons found: %d\n" textReset, iconsErrorTracker.count);
        for (i = 0; i < iconsErrorTracker.count; i++)
        {
            printf(textWhite "  -" textReset " %s\n", iconsErrorTracker.list[i]);
        }

        printf("\nPossible issues:\n");
        printf(textWhite "  -" textReset " Files may be unreadable or corrupted.\n\n");

        printf("Suggested actions:\n");
        printf(textWhite "  1" textReset " Replace the icon files.\n");
        printf(textWhite "  2" textReset " Restore from a backup if available.\n");
    }

    /* Clean up memory */
    

    printf("\niTidy completed");
    if (iconsErrorTracker.count > 0)
    {
        printf(", with some errors, in ");
    }
    else
    {
        printf(" successfully in ");
    }

    if (hours > 0)
    {
        printf("%ld hour", hours);
        if (hours > 1)
        {
            printf("s ");
        }
        {
            printf(" ");
        }
    }

    if (minutes > 0 || hours > 0)
    {
        printf("%ld minutes and", minutes);
        if (minutes > 1)
        {
            printf("s and");
        }
        {
            printf(" and ");
        }
    }

    printf("%ld seconds", seconds);
    printf("\n\n");

    FreeIconErrorList(&iconsErrorTracker);

    return RETURN_OK;
}