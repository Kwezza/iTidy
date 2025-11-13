/*
    iTidy - Icon Cleanup Tool
    GUI Version for Workbench 3.0+

    Migrated from CLI to GUI interface while preserving all functionality.
    Uses pure Intuition - no MUI required.

    v2.0.0 - GUI version
    */

/* VBCC MIGRATION NOTE: Standard C headers */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

/* VBCC MIGRATION NOTE: Platform abstraction */
#include <platform/platform.h>
#include <platform/amiga_headers.h>

/* VBCC MIGRATION NOTE: Amiga-specific headers wrapped in guard */
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

/* VBCC MIGRATION NOTE: Project headers */
#include "itidy_types.h"
#include "Settings/IControlPrefs.h"
#include "Settings/WorkbenchPrefs.h"
#include "icon_management.h"
#include "window_management.h"
#include "utilities.h"
#include "spinner.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "dos/getDiskDetails.h"
#include "Settings/get_fonts.h"
#include "cli_utilities.h"
#include "file_directory_handling.h"
#include "GUI/main_window.h"

/* VBCC: Set stack size to 20KB at compile time */
#ifdef __AMIGA__
long __stack = 80000L;
#endif

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

BOOL user_folderViewMode;
BOOL user_folderFlags;
BOOL user_stripIconPosition;
BOOL user_forceStandardIcons;

#define VERSION_STRING "$VER: iTidy 2.0 (20.10.2025)"
const char version[] = VERSION_STRING;

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
        newList = whd_malloc(newSize * sizeof(STRPTR));
        if (newList == NULL)
        {
            printf("Error reallocating memory for icon error list\n");
            return;
        }
        memset(newList, 0, newSize * sizeof(STRPTR));

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
    tracker->list[tracker->count] = whd_malloc(strlen(filePath) + 1);
    if (tracker->list[tracker->count] != NULL)
    {
        memset(tracker->list[tracker->count], 0, strlen(filePath) + 1);
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

int main(int argc, char **argv)
{
    /* GUI MIGRATION: New GUI window structure */
    struct iTidyMainWindow gui_window;
    BOOL keep_running;
    
    int workbenchVersion = GetWorkbenchVersion();
    int fontNameSet = 0;
    int fontSizeSet = 0;

#ifdef DEBUG
    char *stringWBVersion;
#endif

    /* GUI MIGRATION: Initialize default settings (previously set from CLI args) */
    user_folderViewMode = DDVM_BYICON;
    user_folderFlags = DDFLAGS_SHOWICONS;
    user_stripIconPosition = FALSE;
    user_forceStandardIcons = FALSE;
    
    printf("\n");

    /* KEEP: Timer setup */
    if (setupTimer() != 0)
    {
        printf("Failed to setup timer\n");
        return 1;
    }

    /* KEEP: Font preferences allocation */
    fontPrefs = whd_malloc(sizeof(struct FontPrefs));
    if (!fontPrefs)
    {
        printf("Error: Failed to allocate memory for fontPrefs\n");
        return 1;
    }
    memset(fontPrefs, 0, sizeof(struct FontPrefs));

    /* KEEP: Initialize the error tracker */
    iconsErrorTracker.size = ERROR_LIST_INITIAL_SIZE;
    iconsErrorTracker.count = 0;
    iconsErrorTracker.list = whd_malloc(iconsErrorTracker.size * sizeof(STRPTR));

    if (iconsErrorTracker.list == NULL)
    {
        printf("Error allocating memory for icon error list\n");
        return 1;
    }

    /* KEEP: Banner display */
    printf("==================================================\n");
    printf("   iTidy GUI V%s - Amiga Workbench Icon Tidier\n", VERSION);
    printf("==================================================\n\n");
    printf("   by Kerry Thompson\n");
    printf("   compiled: %s at %s\n\n", __DATE__, __TIME__);

    /* Initialize enhanced logging system (TRUE = clean old logs) */
    initialize_log_system(TRUE);
    
    /* Set default log level (DEBUG for testing, can be changed via Beta Options) */
    set_global_log_level(LOG_LEVEL_DEBUG);
    
    /* Disable memory logging by default (can be enabled via Beta Options) */
    set_memory_logging_enabled(FALSE);
    
    /* Disable performance logging by default (can be enabled via Beta Options) */
    set_performance_logging_enabled(FALSE);
    
    /* Initialize memory tracking if enabled */
    whd_memory_init();
    
    log_info(LOG_GENERAL, "=== iTidy GUI version starting up (VBCC build) ===\n");

#ifdef DEBUG
    log_debug(LOG_GENERAL, "iTidy GUI V%s - Amiga Workbench Icon Tidier\n", VERSION);
    log_debug(LOG_GENERAL, "by Kerry Thompson\n");
    log_debug(LOG_GENERAL, "Version compiled on %s at %s\n", __DATE__, __TIME__);
    log_debug(LOG_GENERAL, "Debug build\n");

    stringWBVersion = convertWBVersionWithDot(workbenchVersion);
    log_debug(LOG_GENERAL, "Workbench version: %s\n", stringWBVersion);
    whd_free(stringWBVersion);
#endif

    /* KEEP: Workbench version check */
    if (workbenchVersion < 36000)
    {
        printf("This program requires Workbench 2.0 or higher.\n");
        return RETURN_FAIL;
    }

    /* KEEP: Icon spacing adjustments for older Workbench */
    if (workbenchVersion < 44500)
    {
        ICON_SPACING_X = 15;
        ICON_SPACING_Y = 10;
#ifdef DEBUG
        log_debug(LOG_GENERAL, "Workbench 2.x detected. Icon spacing increased to x: %d y: %d\n", ICON_SPACING_X, ICON_SPACING_Y);
#endif
    }

    /* GUI MIGRATION: No more CLI argument checking - removed argc < 2 check */
    /* GUI MIGRATION: No more command-line parsing loop - all settings from GUI */

    /* KEEP: Load font and preferences */
    fontPrefs = getIconFont();
    fetchWorkbenchSettings(&prefsWorkbench);
#ifdef DEBUG
    DumpWorkbenchSettings(&prefsWorkbench);
#endif
    fetchIControlSettings(&prefsIControl);
#ifdef DEBUG
    dumpIControlPrefs(&prefsIControl);
#endif

    /* KEEP: Initialize window system (for icon processing, not GUI) */
    InitializeWindow();
#ifdef DEBUG
    log_debug(LOG_GENERAL, "Workbench screen width %d, height %d\n", screenWidth, screenHight);
#endif

    /* GUI MIGRATION: Open the GUI window */
    printf("Opening iTidy GUI window...\n");
    if (!open_itidy_main_window(&gui_window))
    {
        printf("Failed to open GUI window\n");
        /* Cleanup and exit */
        CleanupWindow();
        disposeTimer();
        FreeIconErrorList(&iconsErrorTracker);
        if (fontPrefs)
        {
            FreeVec(fontPrefs);
            fontPrefs = NULL;
        }
        return RETURN_FAIL;
    }

    printf("GUI window opened successfully.\n");
    printf("Click the close gadget to exit.\n\n");

    /* GUI MIGRATION: Main GUI event loop */
    keep_running = TRUE;
    while (keep_running)
    {
        keep_running = handle_itidy_window_events(&gui_window);
        
        /* TODO: Future enhancement - check for "Start" button click */
        /* TODO: When Start clicked, call ProcessDirectory() with GUI settings */
        /* TODO: Display progress during processing */
        /* TODO: Show summary in GUI when complete */
    }

    /* GUI MIGRATION: Close the GUI window */
    printf("\nClosing GUI window...\n");
    close_itidy_main_window(&gui_window);

    /* KEEP: Cleanup window system */
#ifdef DEBUG
    log_debug(LOG_GENERAL, "DEBUG: About to call CleanupWindow()...\n");
#endif
    CleanupWindow();
#ifdef DEBUG
    log_debug(LOG_GENERAL, "DEBUG: CleanupWindow() completed\n");
#endif

    /* KEEP: Dispose timer */
#ifdef DEBUG
    log_debug(LOG_GENERAL, "DEBUG: About to call disposeTimer()...\n");
#endif
    disposeTimer();
#ifdef DEBUG
    log_debug(LOG_GENERAL, "DEBUG: disposeTimer() completed\n");
#endif

    /* KEEP: Free allocated resources */
#ifdef DEBUG
    log_debug(LOG_GENERAL, "DEBUG: About to call FreeIconErrorList()...\n");
#endif
    FreeIconErrorList(&iconsErrorTracker);
#ifdef DEBUG
    log_debug(LOG_GENERAL, "DEBUG: FreeIconErrorList() completed\n");
#endif

#ifdef DEBUG
    log_debug(LOG_GENERAL, "DEBUG: About to free fontPrefs (ptr=%ld)...\n", (LONG)fontPrefs);
#endif
    if (fontPrefs)
    {
        FreeVec(fontPrefs);
        fontPrefs = NULL;
#ifdef DEBUG
        log_debug(LOG_GENERAL, "DEBUG: fontPrefs freed successfully\n");
#endif
    }

    /* Free tool cache if it exists */
#ifdef DEBUG
    log_debug(LOG_GENERAL, "DEBUG: Checking for tool cache to free...\n");
#endif
    FreeToolCache();
#ifdef DEBUG
    log_debug(LOG_GENERAL, "DEBUG: Tool cache cleanup completed\n");
#endif

    /* Report memory status before shutdown */
    whd_memory_report();
    
    /* Print logging performance statistics */
    print_log_performance_stats();
    
    /* Shutdown logging system */
    shutdown_log_system();

    printf("\niTidy GUI shutdown complete\n\n");

    /* KEEP: VBCC -lauto workaround */
#ifdef __AMIGA__
    RemTask(NULL);
    return RETURN_OK;
#else
    return RETURN_OK;
#endif
}

/* VBCC MIGRATION NOTE: Function implementation for font name validation */
int endsWithFont(const char *str)
{
    size_t len;

    if (str == NULL)
    {
        return 0;
    }

    len = strlen(str);
    return (len > 5 && strcmp(str + len - 5, ".font") == 0);
}

/* End of main_gui.c */
