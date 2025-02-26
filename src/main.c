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
//#include "icon_management.h"
#include "window_management.h"
#include "utilities.h"
#include "spinner.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "dos/getDiskDetails.h"
#include "Settings/get_fonts.h"
//#include "file_directory_handling.h"


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
int ICON_SPACING_Y = 7;
int icon_type_standard = 0;
int icon_type_newIcon = 1;
int icon_type_os35 = 2;


char filePath[256];

int count_icon_type_standard = 0;
int count_icon_type_newIcon = 0;
int count_icon_type_os35 = 0;
int count_icon_corrupted = 0;

BOOL user_dontResize;
BOOL user_cleanupWHDLoadFolders;
BOOL user_folderViewMode;
BOOL user_folderFlags;
BOOL user_stripIconPosition;
BOOL user_forceStandardIcons;




// Forward declaration for IsDeviceReadOnly
BOOL IsDeviceReadOnly(const char *deviceName);

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

    // Program Title
    printf(textBold textBlue "iTidy V1.0.0" textReset " - Tidy icons and resize folder windows from CLI.\n\n");
    
    // Usage Section
    printf(textBold "Usage:\n" textReset);
    printf("  " textWhite "iTidy <directory> [options]" textReset "\n\n");
    
    // Options Section
    printf(textBold "Options:\n" textReset);
    printf("  " textBold "-subdirs     " textReset "Recursively process subfolders.\n");
    printf("  " textBold "-dontResize  " textReset "Do not resize and center the window.\n");
    printf("  " textBold "-viewIcons   " textReset "Only show files with icons.\n");
    printf("  " textBold "-viewShowAll " textReset "Show all files, including those without icons.\n");
    printf("  " textBold "-viewDefault " textReset "Use default view settings.\n");
    printf("  " textBold "-viewByName  " textReset "List mode sorted by name.\n");
    printf("  " textBold "-viewByType  " textReset "List mode sorted by type.\n");
    printf("  " textBold "-resetIcons  " textReset "Remove saved icon positions.\n");
    printf("  " textBold "-skipWHD     " textReset "Keep WHDLoad positions but resize.\n");
    printf("  " textBold "-forceStd    " textReset "Ignore advanced icon sizes, use classic size for machines \n");
    printf("               " textBlack "without NewIcons support. May remove some NewIcon data." textReset "\n\n");
    
    
    // Examples Section
    printf(textBold "Examples:\n" textReset);
    printf("  " textWhite "iTidy Work:Projects -subdirs" textReset " " textBlack "Recursively tidy Work:Projects and subfolders\n"
           "  " textWhite "iTidy DF0: -viewByName -viewShowAll" textReset " " textBlack "Tidy DF0:, change the folder view to\n                                      text, and show all files\n\n");
    
    // Important Note and Disclaimer Combined
    printf(textBold "Important & Disclaimer:\n" textReset);
    printf("  " textBlack "Back up your system before running." textReset "\n");
    printf("  " textItalic "Provided 'as is' without any warranty. No responsibility for issues." textReset "\n");
    

}

int main(int argc, char **argv)
{
    
    int i;
    int workbenchVersion = GetWorkbenchVersion();
    int numLeftOutIcons = 0;
    char deviceName[25];
    BOOL iterateDIRs = FALSE;
    long elapsed_seconds, hours, minutes, seconds, start_time;
    //DeviceInfo diskInfo;
    int fontNameSet = 0;
    int fontSizeSet = 0;

#ifdef DEBUG
    char *stringWBVersion;
#endif
    user_dontResize = FALSE;
    user_folderViewMode = DDVM_BYICON;
    user_folderFlags = DDFLAGS_SHOWICONS;
    user_cleanupWHDLoadFolders = TRUE;
    user_stripIconPosition = FALSE;
    //fontPrefs->overRide = 0;
    printf("\n");

    if (setupTimer() != 0)
    {
        printf("Failed to setup timer\n");
        return 1;
    }

    //used to store the current font the user has set in the workbench prefs
    fontPrefs = AllocVec(sizeof(struct FontPrefs), MEMF_CLEAR);
   if (!fontPrefs) {
        printf("Error: Failed to allocate memory for fontPrefs\n");
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
    printf( "==================================================\n" );
    printf(textBold "   iTidy" textReset " V%s - Amiga Workbench Icon Tidier\n", VERSION);
    printf("==================================================\n\n" );
    printf("   by Kerry Thompson\n");
    printf("   compiled: %s at %s\n\n", __DATE__, __TIME__);


    initialize_logfile();
#ifdef DEBUG
    initialize_logfile();
    append_to_log("iTidy V%s\n - Amiga Workbench Icon Tidier", VERSION);
    append_to_log("by Kerry Thompson\n");
    append_to_log("Version compiled on %s at %s\n", __DATE__, __TIME__);
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
        if (strncasecmp_custom(argv[i], "-forceStandardIcons", strlen(argv[i])) == 0) /* test on Workbench 2 that didnt have NewIcons or OS3.5 support meant that any NewIcons were postioien using icon sizes that werent visible.  this uses only the original height and width of the icon. */
            user_forceStandardIcons=true;
        if (strncasecmp_custom(argv[i], "-skipWHD", strlen(argv[i])) == 0) /* By design, this program was created to clean up extracted WHDLoad folders, and skip rearranging the original author's layout.
                                                                                            This option forces the program to rearrange the icons if it detects a .slave file is found in the folder.  Without this option,
                                                                                            the fodler's window will just be resized and centered. */
            user_cleanupWHDLoadFolders = FALSE;

                // Handle xPadding and yPadding
        if (strncasecmp_custom(argv[i], "-xPadding:", 10) == 0)
        {
            int value = atoi(argv[i] + 10);
            if (value >= 0 && value <= 30)
            {
                ICON_SPACING_X = value;
                printf("xPadding overridden: %d\n", ICON_SPACING_X);
            }
            else
            {
                printf("Error: -xPadding value must be between 0 and 30.\n");
                return 1; // Exit with an error
            }
        }

        if (strncasecmp_custom(argv[i], "-yPadding:", 10) == 0)
        {
            int value = atoi(argv[i] + 10);
            if (value >= 0 && value <= 30)
            {
                ICON_SPACING_Y = value;
                printf("yPadding overridden: %d\n", ICON_SPACING_Y);
            }
            else
            {
                printf("Error: -yPadding value must be between 0 and 30.\n");
                return 1; // Exit with an error
            }
        }
        if (strncasecmp_custom(argv[i], "-fontName:", 10) == 0) {
            char *fontValue = argv[i] + 10;
            if (endsWithFont(fontValue)) {
                strncpy(fontPrefs->name, fontValue, 255);
                fontPrefs->name[31] = '\0';  // Ensure null termination
                fontPrefs->overRide = 1;
            } else {
                printf("Error: -fontName must end with '.font'.\n");
                return 1;
            }
        }

        if (strncasecmp_custom(argv[i], "-fontSize:", 10) == 0) {
            int value = atoi(argv[i] + 10);
            if (value >= 1 && value <= 30) {
                fontPrefs->size = value;
                fontPrefs->overRide = 1;
            } else {
                printf("Error: -fontSize must be between 1 and 30.\n");
                return 1;
            }
        }
    

    }

    // Ensure both fontName and fontSize are set together
    if ((fontNameSet && !fontSizeSet) || (!fontNameSet && fontSizeSet)) {
        printf("Error: Both -fontName and -fontSize must be set together.\n");
        return 1;
    }

    // Debug output

    if (fontNameSet && fontSizeSet) {
        printf("Font: %s, Size: %d\n", fontPrefs->name, fontPrefs->size);
        #ifdef DEBUG
        append_to_log("Font: %s, Size: %d\n", fontPrefs->name, fontPrefs->size);
        #endif
    }

    #ifdef DEBUG
    append_to_log("ICON_SPACING_Y: %d\n", ICON_SPACING_Y);
    append_to_log("ICON_SPACING_X: %d\n", ICON_SPACING_X);
    #endif

    
 //   diskInfo = GetDeviceInfo(filePath);
 //   if (diskInfo.writeProtected)
 //   {
 //       printf("Device %s is read-only.  Exiting.\n\n", diskInfo.deviceName);
 //       return RETURN_FAIL;
 //   }
 //   printf("Tidying icons in %s\n", filePath);

    //printf("\ntest Device %s has %d%s\n", diskInfo.deviceName,diskInfo.free,diskInfo.storageUnits);

    /* Start timer */


    getDeviceNameFromPath(filePath, deviceName, 25);
    if (deviceName[0] == '\0') {
        printf("\nError: unable to get the device name from the file path '%s'.  PLease use a full file path.  For example DF0:MyDir %s\n", filePath);
        #ifdef DEBUG
        append_to_log("\nUnable to find the device in the user supplied path of '%s'\n", deviceName);
        #endif
        return RETURN_FAIL;
        // String is empty
    }
//printf("is device %s read only? %d\n",deviceName,IsDeviceReadOnly(deviceName));

    if (IsDeviceReadOnly(deviceName))
    {
        printf("\nDevice %s is read-only.  Exiting.\n", deviceName);
        #ifdef DEBUG
            append_to_log("\nDevice %s is read-only.  Exiting.\n", deviceName);
        #endif
        return RETURN_FAIL;
    }

    start_time = time(NULL);
//*/
//InitializeDefaultWorkbenchSettings(&prefsWorkbench);

    fontPrefs=getIconFont();

    fetchWorkbenchSettings(&prefsWorkbench);
    #ifdef DEBUG
    DumpWorkbenchSettings(&prefsWorkbench);
    #endif
    fetchIControlSettings(&prefsIControl);
    #ifdef DEBUG
    dumpIControlPrefs(&prefsIControl);
    #endif

    loadLeftOutIcons(filePath);
    numLeftOutIcons = countLeftOutIcons();

    if (numLeftOutIcons > 0)
    {
        printf("Found %d icons placed on Workbench from '%s'\n\n", numLeftOutIcons, deviceName);
    }

#ifdef DEBUG
    //append_to_log("Wokbench Icon border:  %d\n",prefsWorkbench.embossRectangleSize);
    //append_to_log("IControl prefs:  Window: border width: %d, border height: %d\n", prefsIControl.currentBarWidth, prefsIControl.currentBarHeight);
    //append_to_log("IControl prefs:  title height: %d, Window height: %d\n", prefsIControl.currentTitleBarHeight, prefsIControl.currentWindowBarHeight);
    //append_to_log("IControl prefs:  volume guage width: %d\n", prefsIControl.currentCGaugeWidth);
    
    // Print the loaded left out icons
    dumpLeftOutIcons();
#endif



    InitializeWindow();
    #ifdef DEBUG
    append_to_log("Workbench screen width %d, height %d\n", screenWidth,screenHight);
    #endif
    printf("\nTidying folders under: %s\n",filePath);
    ProcessDirectory(filePath, iterateDIRs, 0);
    CleanupWindow();
    disposeTimer();

    /* Calculate elapsed time */
    elapsed_seconds = time(NULL) - start_time;
    hours = elapsed_seconds / 3600;
    minutes = (elapsed_seconds % 3600) / 60;
    seconds = elapsed_seconds % 60;

    printf( "\n\n==================================================\n" );
    printf(textBold "Icons Tidied Summary:\n" textReset);
    printf(textWhite "  " textReset " Standard icons:    %d\n", count_icon_type_standard);
    printf(textWhite "  " textReset " NewIcon icons:     %d\n", count_icon_type_newIcon);
    printf(textWhite "  " textReset " OS3.5 style icons: %d\n", count_icon_type_os35);
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
    printf( "==================================================\n\n" );
    /* Clean up memory */
    

    printf("\niTidy ");
    if (iconsErrorTracker.count > 0)
    {
        printf("completed, with some errors, in ");
    }
    else
    {
        printf(textBold "completed successfully " textReset "in ");
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

#ifdef DEBUG
// Log total icons tidied
append_to_log("\n\nTotal icons tidied: %d\n", count_icon_type_newIcon + count_icon_type_standard + count_icon_type_os35);
append_to_log("  - Standard icons:    %d\n", count_icon_type_standard);
append_to_log("  - NewIcon icons:     %d\n", count_icon_type_newIcon);
append_to_log("  - OS3.5 style icons: %d\n", count_icon_type_os35);

if (iconsErrorTracker.count > 0)
{
    append_to_log("\nTotal corrupted icons found: %d\n", iconsErrorTracker.count);
    
    for (i = 0; i < iconsErrorTracker.count; i++)
    {
        append_to_log("  - %s\n", iconsErrorTracker.list[i]);
    }

    append_to_log("\nPossible issues:\n");
    append_to_log("  - Files may be unreadable or corrupted.\n\n");

    append_to_log("Suggested actions:\n");
    append_to_log("  1. Replace the icon files.\n");
    append_to_log("  2. Restore from a backup if available.\n");
}

// Log completion message
append_to_log("\niTidy completed");

if (iconsErrorTracker.count > 0)
{
    append_to_log(", with some errors, in ");
}
else
{
    append_to_log(" successfully in ");
}

// Log time taken
if (hours > 0)
{
    append_to_log("%ld hour", hours);
    if (hours > 1)
    {
        append_to_log("s");
    }
    append_to_log(" ");
}

if (minutes > 0 || hours > 0)
{
    append_to_log("%ld minute", minutes);
    if (minutes > 1)
    {
        append_to_log("s");
    }
    append_to_log(" and ");
}

append_to_log("%ld seconds\n", seconds);
#endif


    FreeIconErrorList(&iconsErrorTracker);

    return RETURN_OK;
}

// Check if string ends with ".font"
int endsWithFont(const char *str) {
    size_t len = strlen(str);
    return (len > 5 && strcmp(str + len - 5, ".font") == 0);
}