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

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

/* VBCC MIGRATION NOTE: Amiga-specific headers */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
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
#include <itidy_types.h>

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
#include "file_directory_handling.h"
#include "layout_preferences.h"
#include "GUI/main_window.h"
#include "deficons/deficons_parser.h"

/* VBCC: Set stack size to 80KB at compile time */
long __stack = 80000L;

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

/* DefIcons type tree cache (parsed once at startup, accessible globally) */
DeficonTypeTreeNode *g_cached_deficons_tree = NULL;
int g_cached_deficons_count = 0;

BOOL user_forceStandardIcons;

//#define VERSION_STRING "$VER: iTidy 2.0 (20.10.2025)"
static const char version_string[] ="$VER: iTidy " ITIDY_VERSION " (" __DATE__ ")";
const char version[] = ITIDY_VERSION;

/* External reference to SysBase (provided by VBCC runtime) */
extern struct ExecBase *SysBase;
/* VBCC's -lauto provides this global for Workbench startup */
extern struct WBStartup *_WBenchMsg;

/*---------------------------------------------------------------------------*/
/* Workbench Version Check System                                            */
/*---------------------------------------------------------------------------*/

/**
 * @brief Check if Workbench 3.0 or later is running
 * 
 * iTidy requires Workbench 3.0 (Kickstart 3.0, v39+) minimum.
 * Earlier versions will crash due to missing GadTools features.
 * 
 * This check uses SysBase->LibNode.lib_Version which reflects the
 * Kickstart/exec.library version:
 *   - v37 = Kickstart 2.04 (Workbench 2.0)
 *   - v38 = Kickstart 2.1 (Workbench 2.1)
 *   - v39 = Kickstart 3.0 (Workbench 3.0)
 *   - v40 = Kickstart 3.1 (Workbench 3.1)
 * 
 * @return 1 if OK to run (v39+), 0 if version too old (should quit)
 */
static int RequireWB3OrBetter(void)
{
    UWORD detected_version;
    /* BPTR log_file; */
    char version_msg[256];
    
    /* Get the exec.library version */
    detected_version = SysBase->LibNode.lib_Version;
    
    /* NOTE: Diagnostic log file creation disabled - no longer needed for WB 3.0+ only target */
    /*
    log_file = Open((STRPTR)"PROGDIR:version_check.log", MODE_NEWFILE);
    if (log_file)
    {
        sprintf(version_msg, "iTidy Version Check\n");
        Write(log_file, version_msg, strlen(version_msg));
        sprintf(version_msg, "===================\n");
        Write(log_file, version_msg, strlen(version_msg));
        sprintf(version_msg, "SysBase->LibNode.lib_Version = %u\n", detected_version);
        Write(log_file, version_msg, strlen(version_msg));
        sprintf(version_msg, "Required: 39 (Kickstart/WB 3.0) or higher\n");
        Write(log_file, version_msg, strlen(version_msg));
        
        if (detected_version >= 39)
        {
            sprintf(version_msg, "Result: OK - Version check passed\n");
            Write(log_file, version_msg, strlen(version_msg));
        }
        else
        {
            sprintf(version_msg, "Result: FAILED - Version too old\n");
            Write(log_file, version_msg, strlen(version_msg));
            sprintf(version_msg, "Displaying error alert and exiting...\n");
            Write(log_file, version_msg, strlen(version_msg));
        }
        
        Close(log_file);
    }
    */
    
    /* Exec v39 == Kickstart 3.0 (Workbench 3.0 era) */
    if (detected_version >= 39)
    {
        /* Version OK - write to console and continue */
        sprintf(version_msg, "[Version Check] SysBase version %u - OK\n", detected_version);
        PutStr(version_msg);
        return 1;
    }

    /* Version too old - display error */
    sprintf(version_msg, "[Version Check] SysBase version %u - TOO OLD (need 39+)\n", detected_version);
    PutStr(version_msg);
    
    /* Display error alert using AutoRequest (compatible with WB1.x+) */
    {
        struct IntuiText body_text;
        struct IntuiText pos_text;
        
        /* Initialize body text (keep it simple - single line) */
        body_text.FrontPen = 1;
        body_text.BackPen = 0;
        body_text.DrawMode = JAM1;
        body_text.LeftEdge = 10;
        body_text.TopEdge = 10;
        body_text.ITextFont = NULL;
        body_text.IText = (UBYTE *)"iTidy requires Workbench 3.0 or later";
        body_text.NextText = NULL;
        
        /* Initialize positive button text */
        pos_text.FrontPen = 1;
        pos_text.BackPen = 0;
        pos_text.DrawMode = JAM1;
        pos_text.LeftEdge = 6;
        pos_text.TopEdge = 3;
        pos_text.ITextFont = NULL;
        pos_text.IText = (UBYTE *)"Exit";
        pos_text.NextText = NULL;
        
        /* Try to show requester */
        AutoRequest(NULL, &body_text, &pos_text, NULL, 0, 0, 300, 60);
        
        /* Always echo to Shell (primary output for older systems) */
        PutStr("\n");
        PutStr("===============================================\n");
        PutStr("  iTidy - Version Check Error\n");
        PutStr("===============================================\n");
        PutStr("\n");
        PutStr("iTidy requires Workbench 3.0 or later.\n");
        PutStr("You appear to be running an older version.\n");
        PutStr("\n");
        PutStr("If you need a WB2/WB1 build, please contact:\n");
        PutStr("GitHub: Kwezza/iTidy\n");
        PutStr("\n");
    }

    return 0;
}

/*---------------------------------------------------------------------------*/
/* ToolType Processing System                                               */
/*---------------------------------------------------------------------------*/

/**
 * @brief Structure to hold parsed tooltype settings
 * 
 * This structure stores all tooltype values extracted from the program's
 * icon when launched from Workbench. Values are parsed once at startup.
 */
typedef struct ToolTypeSettings_tag {
    BOOL tooltypes_loaded;      /* TRUE if tooltypes were successfully parsed */
    UWORD debug_level;          /* DEBUGLEVEL=n (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR) */
    BOOL debug_level_set;       /* TRUE if DEBUGLEVEL was found in tooltypes */
    
    /* LOADPREFS tooltype support */
    char loadprefs_path[256];   /* Path from LOADPREFS=... */
    BOOL loadprefs_set;         /* TRUE if LOADPREFS was found in tooltypes */

    /* PERFLOG tooltype support */
    BOOL performance_log;       /* PERFLOG=YES/NO - enable performance timing logs */
    BOOL performance_log_set;   /* TRUE if PERFLOG was found in tooltypes */
} ToolTypeSettings;

/* Global tooltype settings (initialized at startup) - exported for use by main_window.c */
ToolTypeSettings g_tooltypes = { FALSE, 4, FALSE, "", FALSE, FALSE, FALSE }; /* Default: DISABLED level, no LOADPREFS, no PERFLOG */

/**
 * @brief Case-insensitive string comparison (snake_case naming)
 * 
 * @param str1 First string to compare
 * @param str2 Second string to compare
 * @param len Maximum number of characters to compare
 * @return int 0 if equal (ignoring case), non-zero otherwise
 */
static int stricmp_n(const char *str1, const char *str2, int len)
{
    int i;
    for (i = 0; i < len && str1[i] && str2[i]; i++)
    {
        char c1 = (char)tolower((unsigned char)str1[i]);
        char c2 = (char)tolower((unsigned char)str2[i]);
        if (c1 != c2)
        {
            return c1 - c2;
        }
    }
    /* If we reached the end of comparison length, strings are equal */
    if (i == len)
    {
        return 0;
    }
    /* Otherwise, compare the final characters */
    return tolower((unsigned char)str1[i]) - tolower((unsigned char)str2[i]);
}

/**
 * @brief Parse program tooltypes from Workbench startup
 * 
 * Reads the program's icon tooltypes and extracts configuration values.
 * Called once at program startup if launched from Workbench.
 * 
 * Supported tooltypes:
 *   DEBUGLEVEL=n  - Set log level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=DISABLED)
 * 
 * @param wb_startup Pointer to WBStartup message (NULL if launched from CLI)
 */
static void parse_program_tooltypes(struct WBStartup *wb_startup)
{
    struct DiskObject *dobj = NULL;
    struct Library *IconBase = NULL;
    BPTR old_dir = (BPTR)-1;
    STRPTR *tool_types = NULL;
    STRPTR tool_value;
    int value;
    
    CONSOLE_DEBUG("DEBUG: parse_program_tooltypes() called\n");
    
    /* Reset to defaults */
    g_tooltypes.tooltypes_loaded = FALSE;
    g_tooltypes.debug_level = 4;        /* Default: DISABLED (recommended) */
    g_tooltypes.debug_level_set = FALSE;
    g_tooltypes.performance_log = FALSE;
    g_tooltypes.performance_log_set = FALSE;
    
    /* If not launched from Workbench, nothing to parse */
    if (wb_startup == NULL)
    {
        CONSOLE_DEBUG("DEBUG: wb_startup is NULL - launched from CLI\n");
        /* Note: Logging not available yet during early initialization */
        return;
    }
    
    CONSOLE_DEBUG("DEBUG: Launched from Workbench, wb_startup=0x%08lx\n", (unsigned long)wb_startup);
    CONSOLE_DEBUG("DEBUG: wb_startup->sm_NumArgs = %ld\n", (long)wb_startup->sm_NumArgs);
    
    /* Open icon.library to use GetDiskObject/FreeDiskObject */
    IconBase = OpenLibrary((STRPTR)"icon.library", 0);
    if (IconBase == NULL)
    {
        CONSOLE_ERROR("ERROR: Failed to open icon.library!\n");
        /* Can't parse tooltypes without icon.library - use defaults */
        return;
    }
    
    CONSOLE_DEBUG("DEBUG: icon.library opened successfully\n");
    
    /* Get the program's directory and icon */
    if (wb_startup->sm_NumArgs > 0)
    {
        CONSOLE_DEBUG("DEBUG: Program name: %s\n", wb_startup->sm_ArgList[0].wa_Name);
        
        /* Change to program's directory */
        old_dir = CurrentDir(wb_startup->sm_ArgList[0].wa_Lock);
        CONSOLE_DEBUG("DEBUG: Changed to program directory\n");
        
        /* Get the program's icon */
        dobj = GetDiskObject((STRPTR)wb_startup->sm_ArgList[0].wa_Name);
        
        if (dobj != NULL)
        {
            CONSOLE_DEBUG("DEBUG: Got DiskObject successfully\n");
            
            /* Get the tooltypes array */
            tool_types = (STRPTR *)dobj->do_ToolTypes;
            
            if (tool_types != NULL)
            {
                CONSOLE_DEBUG("DEBUG: ToolTypes array exists\n");
                
                /* Parse DEBUGLEVEL=n */
                tool_value = (STRPTR)FindToolType(tool_types, (STRPTR)"DEBUGLEVEL");
                if (tool_value != NULL)
                {
                    CONSOLE_DEBUG("DEBUG: Found DEBUGLEVEL tooltype: %s\n", tool_value);
                    value = atoi(tool_value);
                    /* Validate range 0-4 (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=DISABLED) */
                    if (value >= 0 && value <= 4)
                    {
                        g_tooltypes.debug_level = (UWORD)value;
                        g_tooltypes.debug_level_set = TRUE;
                        CONSOLE_DEBUG("DEBUG: Set debug level to %d\n", value);
                        /* Logging will report this after system is initialized */
                    }
                    else
                    {
                        CONSOLE_WARNING("WARNING: DEBUGLEVEL=%d out of range (0-4)\n", value);
                    }
                }
                else
                {
                    CONSOLE_DEBUG("DEBUG: No DEBUGLEVEL tooltype found\n");
                }
                
                /* Parse LOADPREFS tooltype */
                tool_value = (STRPTR)FindToolType(tool_types, (STRPTR)"LOADPREFS");
                if (tool_value != NULL)
                {
                    /* Strip leading/trailing whitespace and copy path */
                    const char *start = tool_value;
                    const char *end;
                    
                    /* Skip leading whitespace */
                    while (*start && (*start == ' ' || *start == '\t'))
                        start++;
                    
                    /* Find end of string (before trailing whitespace) */
                    end = start + strlen(start) - 1;
                    while (end > start && (*end == ' ' || *end == '\t'))
                        end--;
                    
                    /* Copy trimmed path */
                    size_t len = (size_t)(end - start + 1);
                    if (len > 0 && len < sizeof(g_tooltypes.loadprefs_path))
                    {
                        strncpy(g_tooltypes.loadprefs_path, start, len);
                        g_tooltypes.loadprefs_path[len] = '\0';
                        g_tooltypes.loadprefs_set = TRUE;
                        CONSOLE_DEBUG("DEBUG: LOADPREFS tooltype found: %s\n", g_tooltypes.loadprefs_path);
                    }
                }
                else
                {
                    CONSOLE_DEBUG("DEBUG: No LOADPREFS tooltype found\n");
                }
                
                /* Parse PERFLOG=YES/NO */
                tool_value = (STRPTR)FindToolType(tool_types, (STRPTR)"PERFLOG");
                if (tool_value != NULL)
                {
                    if (MatchToolValue(tool_value, (STRPTR)"YES"))
                    {
                        g_tooltypes.performance_log = TRUE;
                        g_tooltypes.performance_log_set = TRUE;
                        CONSOLE_DEBUG("DEBUG: PERFLOG=YES found\n");
                    }
                    else if (MatchToolValue(tool_value, (STRPTR)"NO"))
                    {
                        g_tooltypes.performance_log = FALSE;
                        g_tooltypes.performance_log_set = TRUE;
                        CONSOLE_DEBUG("DEBUG: PERFLOG=NO found\n");
                    }
                }
                else
                {
                    CONSOLE_DEBUG("DEBUG: No PERFLOG tooltype found\n");
                }

                g_tooltypes.tooltypes_loaded = TRUE;
                CONSOLE_DEBUG("DEBUG: ToolTypes loaded successfully\n");
                /* Logging will report tooltype status after system is initialized */
            }
            else
            {
                CONSOLE_WARNING("WARNING: ToolTypes array is NULL\n");
            }
            
            /* Free the icon */
            FreeDiskObject(dobj);
            CONSOLE_DEBUG("DEBUG: DiskObject freed\n");
        }
        else
        {
            CONSOLE_ERROR("ERROR: GetDiskObject() returned NULL!\n");
        }
        
        /* Restore original directory */
        if (old_dir != (BPTR)-1)
        {
            CurrentDir(old_dir);
        }
    }
    
    /* Close icon.library */
    if (IconBase != NULL)
    {
        CloseLibrary(IconBase);
    }
}

/**
 * @brief Check if a specific tooltype with value exists (case-insensitive)
 * 
 * This function allows checking for specific tooltype=value combinations.
 * Comparison is case-insensitive for both key and value.
 * 
 * Example usage:
 *   if (check_tooltype_value("DEBUGLEVEL", "0")) { ... }
 *   if (check_tooltype_value("debuglevel", "3")) { ... }
 * 
 * @param key ToolType key to check (case-insensitive)
 * @param value ToolType value to match (case-insensitive)
 * @return BOOL TRUE if tooltype exists and matches value, FALSE otherwise
 */
static BOOL check_tooltype_value(const char *key, const char *value)
{
    /* ToolTypes must be loaded first */
    if (!g_tooltypes.tooltypes_loaded)
    {
        return FALSE;
    }
    
    /* Check DEBUGLEVEL (case-insensitive) */
    if (stricmp_n(key, "DEBUGLEVEL", 10) == 0)
    {
        if (g_tooltypes.debug_level_set)
        {
            char level_str[4];
            sprintf(level_str, "%u", (unsigned int)g_tooltypes.debug_level);
            return (stricmp_n(value, level_str, 3) == 0);
        }
        return FALSE;
    }
    
    /* Future tooltype checks can be added here */
    /* Example:
     * if (stricmp_n(key, "OTHEROPTION", 11) == 0) {
     *     return (g_tooltypes.other_option && stricmp_n(value, "yes", 3) == 0);
     * }
     */
    
    return FALSE;
}

/**
 * @brief Get the debug level from tooltypes (public accessor)
 * 
 * @return UWORD Debug level (0-3), or 3 (ERROR) if not set
 */
UWORD get_tooltype_debug_level(void)
{
    if (g_tooltypes.debug_level_set)
    {
        return g_tooltypes.debug_level;
    }
    return 3; /* Default: ERROR */
}

/**
 * @brief Get CPU name from ExecBase AttnFlags
 * @return String describing the detected CPU
 */
static const char *get_cpu_name(void)
{
    ULONG flags = SysBase->AttnFlags;

    if (flags & AFF_68060) return "MC68060";
    if (flags & AFF_68040) return "MC68040";
    if (flags & AFF_68030) return "MC68030";
    if (flags & AFF_68020) return "MC68020";
    if (flags & AFF_68010) return "MC68010";
    
    return "MC68000";
}

/**
 * @brief Log system information (CPU and memory) to general log
 */
static void log_system_info(void)
{
    ULONG chip_free_total, chip_free_largest;
    ULONG fast_free_total, fast_free_largest;
    ULONG chip_kb, fast_kb;
    const char *cpu_name;

    CONSOLE_DEBUG("DEBUG: log_system_info() called\n");
    
    cpu_name = get_cpu_name();
    CONSOLE_DEBUG("DEBUG: CPU name: %s\n", cpu_name);
    
    /* Get memory info using AvailMem() */
    chip_free_total = AvailMem(MEMF_CHIP);
    chip_free_largest = AvailMem(MEMF_CHIP | MEMF_LARGEST);
    fast_free_total = AvailMem(MEMF_FAST);
    fast_free_largest = AvailMem(MEMF_FAST | MEMF_LARGEST);
    
    CONSOLE_DEBUG("DEBUG: Chip=%lu, Fast=%lu\n", chip_free_total, fast_free_total);
    
    /* Convert to KB (round up) */
    chip_kb = (chip_free_total + 1023UL) / 1024UL;
    fast_kb = (fast_free_total + 1023UL) / 1024UL;
    
    CONSOLE_DEBUG("DEBUG: About to log system info...\n");
    log_info(LOG_GENERAL, "=== SYSTEM INFORMATION ===\n");
    log_info(LOG_GENERAL, "CPU: %s\n", cpu_name);
    log_info(LOG_GENERAL, "Chip RAM: %lu KB free (%lu bytes total, %lu bytes largest block)\n", 
             chip_kb, chip_free_total, chip_free_largest);
    
    if (fast_free_total == 0) {
        log_info(LOG_GENERAL, "Fast RAM: None detected\n");
    } else {
        log_info(LOG_GENERAL, "Fast RAM: %lu KB free (%lu bytes total, %lu bytes largest block)\n",
                 fast_kb, fast_free_total, fast_free_largest);
    }
    log_info(LOG_GENERAL, "==========================\n");
    CONSOLE_DEBUG("DEBUG: System info logging complete\n");
}

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
            CONSOLE_ERROR("Error reallocating memory for icon error list\n");
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
        CONSOLE_ERROR("Error allocating memory for icon file path\n");
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
    /* CRITICAL: Check Workbench version FIRST (before any initialization) */
    if (!RequireWB3OrBetter())
    {
        /* Version check failed - alert displayed, exit cleanly */
        return RETURN_FAIL;
    }

    /* GUI MIGRATION: New GUI window structure */
    struct iTidyMainWindow gui_window;
    BOOL keep_running;
    struct WBStartup *wb_startup = NULL;
    
    int workbenchVersion = GetWorkbenchVersion();
    int fontNameSet = 0;
    int fontSizeSet = 0;

#ifdef DEBUG
    char *stringWBVersion;
#endif

    CONSOLE_PRINTF("\n*** iTidy starting up ***\n");
    CONSOLE_PRINTF("argc = %d\n", argc);

    /* Detect if launched from Workbench (argc == 0) */
    /* VBCC with -lauto: Use _WBenchMsg instead of manual message handling */
    if (argc == 0 && _WBenchMsg != NULL)
    {
        CONSOLE_PRINTF("Detected Workbench launch via _WBenchMsg\n");
        wb_startup = _WBenchMsg;
        CONSOLE_PRINTF("WBStartup message: 0x%08lx\n", (unsigned long)wb_startup);
    }
    else if (argc == 0)
    {
        CONSOLE_PRINTF("ERROR: argc==0 but _WBenchMsg is NULL!\n");
    }
    else
    {
        CONSOLE_PRINTF("Launched from CLI\n");
    }

    /* GUI MIGRATION: Initialize default settings (previously set from CLI args) */
    user_folderViewMode = DDVM_BYICON;
    user_folderFlags = DDFLAGS_SHOWICONS;
    user_stripIconPosition = FALSE;
    user_forceStandardIcons = FALSE;
    
    CONSOLE_NEWLINE();

    /* KEEP: Timer setup */
    if (setupTimer() != 0)
    {
        CONSOLE_ERROR("Failed to setup timer\n");
        return 1;
    }

    /* KEEP: Font preferences allocation */
    fontPrefs = whd_malloc(sizeof(struct FontPrefs));
    if (!fontPrefs)
    {
        CONSOLE_ERROR("Error: Failed to allocate memory for fontPrefs\n");
        return 1;
    }
    memset(fontPrefs, 0, sizeof(struct FontPrefs));

    /* KEEP: Initialize the error tracker */
    iconsErrorTracker.size = ERROR_LIST_INITIAL_SIZE;
    iconsErrorTracker.count = 0;
    iconsErrorTracker.list = whd_malloc(iconsErrorTracker.size * sizeof(STRPTR));

    if (iconsErrorTracker.list == NULL)
    {
        CONSOLE_ERROR("Error allocating memory for icon error list\n");
        return 1;
    }

    /* KEEP: Banner display */
    CONSOLE_SEPARATOR();
    CONSOLE_BANNER("   iTidy GUI V%s - Amiga Workbench Icon Tidier\n", VERSION);
    CONSOLE_SEPARATOR();
    CONSOLE_NEWLINE();
    CONSOLE_BANNER("   by Kerry Thompson\n");
    CONSOLE_BANNER("   compiled: %s at %s\n\n", __DATE__, __TIME__);

    /* Initialize enhanced logging system (TRUE = clean old logs) */
    initialize_log_system(TRUE);
    
    /* Parse program tooltypes from Workbench (if launched from WB) */
    parse_program_tooltypes(wb_startup);
    
    /* Set log level IMMEDIATELY to avoid logging at DEBUG level during initialization
     * Note: This will be reset later by InitializeGlobalPreferences(), but we need it
     * now to control logging during startup. The tooltype will be applied to preferences
     * after GUI window opens. */
    if (g_tooltypes.debug_level_set)
    {
        /* Apply tooltype debug level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR) */
        set_global_log_level((LogLevel)g_tooltypes.debug_level);
        log_info(LOG_GENERAL, "ToolType DEBUGLEVEL=%u found - log level applied for startup\n", (unsigned int)g_tooltypes.debug_level);
    }
    else
    {
        /* Set default log level (DISABLED - no logging, no tooltype found) */
        set_global_log_level(LOG_LEVEL_DISABLED);
        /* Don't log anything here - we're at DISABLED level, these would be filtered anyway */
    }
    
    /* Disable memory logging by default (controlled by DEBUG_MEMORY_TRACKING build flag in platform.h) */
    
    /* Apply PERFLOG tooltype (default: disabled) */
    set_performance_logging_enabled(g_tooltypes.performance_log_set ? g_tooltypes.performance_log : FALSE);
    
    /* Dump Workbench screen palette for diagnostic purposes */
    DumpWorkbenchScreenPalette();
    
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

    /* KEEP: Secondary version check (primary WB3.0+ check is at startup) */
    /* This check is now redundant (RequireWB3OrBetter() already enforced v39+) */
    /* but kept for belt-and-suspenders safety in case GetWorkbenchVersion() differs */
    if (workbenchVersion < 39000)
    {
        CONSOLE_ERROR("This program requires Workbench 3.0 or higher.\n");
        log_error(LOG_GENERAL, "Workbench version check failed: %d (requires 39000+)\n", workbenchVersion);
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

    /* NOTE: PATH search list is now built AFTER window opens (see main_window.c) */
    /* This prevents slow startup when parsing S:Startup-Sequence and S:User-Startup */

    /* KEEP: Initialize window system (for icon processing, not GUI) */
    InitializeWindow();
#ifdef DEBUG
    log_debug(LOG_GENERAL, "Workbench screen width %d, height %d\n", screenWidth, screenHight);
#endif

    /* Initialize global preferences with default values */
    CONSOLE_STATUS("Initializing preferences with defaults...\n");
    InitializeGlobalPreferences();
    
    /* CRITICAL: Apply tooltype debug level to preferences AFTER initialization
     * InitializeGlobalPreferences() resets logLevel to DEFAULT_LOG_LEVEL,
     * so we must update the preference structure to preserve the tooltype setting.
     * This ensures layout_processor.c respects the user's tooltype choice. */
    if (g_tooltypes.debug_level_set)
    {
        LayoutPreferences *prefs = (LayoutPreferences *)GetGlobalPreferences();
        ((LayoutPreferences *)prefs)->logLevel = g_tooltypes.debug_level;
        log_info(LOG_GENERAL, "ToolType DEBUGLEVEL=%u applied to preferences\n", (unsigned int)g_tooltypes.debug_level);
    }

    /* Initialize DefIcons cache (non-fatal if fails) */
    CONSOLE_STATUS("Loading DefIcons type tree cache...\n");
    if (parse_deficons_prefs(&g_cached_deficons_tree, &g_cached_deficons_count))
    {
        log_info(LOG_GUI, "DefIcons cache loaded successfully: %d types\n", g_cached_deficons_count);
        CONSOLE_STATUS("DefIcons cache loaded: %d types\n", g_cached_deficons_count);
        
#ifdef DEBUG_DEFICONS_TREE_DUMP
        /* Dump DefIcons tree structure for debugging (disabled by default to avoid log flooding) */
        {
            int i, j;
            int root_count = 0;
            int child_count = 0;
            int grandchild_count = 0;
            
            log_debug(LOG_GUI, "\n=== DefIcons Type Tree Structure ===\n");
            
            /* Count nodes by generation */
            for (i = 0; i < g_cached_deficons_count; i++)
            {
                if (g_cached_deficons_tree[i].generation == 1)
                    root_count++;
                else if (g_cached_deficons_tree[i].generation == 2)
                    child_count++;
                else if (g_cached_deficons_tree[i].generation == 3)
                    grandchild_count++;
            }
            
            log_debug(LOG_GUI, "Total types: %d (Roots: %d, Children: %d, Grandchildren: %d)\n",
                     g_cached_deficons_count, root_count, child_count, grandchild_count);
            log_debug(LOG_GUI, "\n");
            
            /* Dump hierarchical tree */
            for (i = 0; i < g_cached_deficons_count; i++)
            {
                DeficonTypeTreeNode *node = &g_cached_deficons_tree[i];
                
                /* Print indentation based on generation */
                for (j = 0; j < node->generation; j++)
                {
                    log_debug(LOG_GUI, "  ");
                }
                
                /* Print node info */
                if (node->has_children)
                {
                    log_debug(LOG_GUI, "%s%s (gen=%d, has_children=YES, parent_idx=%d)\n",
                             (node->generation == 1) ? "▼ " : "├─ ",
                             node->type_name,
                             node->generation,
                             node->parent_index);
                }
                else
                {
                    log_debug(LOG_GUI, "%s%s (gen=%d, parent_idx=%d)\n",
                             (node->generation == 1) ? "• " : "└─ ",
                             node->type_name,
                             node->generation,
                             node->parent_index);
                }
            }
            
            log_debug(LOG_GUI, "\n=== End DefIcons Tree ===\n\n");
            
            /* Sample query: Find parent of a known type */
            if (g_cached_deficons_count > 0)
            {
                const char *sample_type = "mod";  /* Music module type */
                const char *parent = get_parent_type_name(g_cached_deficons_tree, 
                                                          g_cached_deficons_count, 
                                                          sample_type);
                if (parent)
                {
                    log_debug(LOG_GUI, "Sample query: Parent of '%s' is '%s'\n", sample_type, parent);
                }
            }
        }
#endif
    }
    else
    {
        log_warning(LOG_GUI, "DefIcons not available (ENV:Sys/deficons.prefs not found)\n");
        CONSOLE_STATUS("DefIcons not available (requires Workbench 3.2+)\n");
    }

    /* Initialize ReAction libraries (required for Workbench 3.2+ GUI) */
    CONSOLE_STATUS("Initializing ReAction libraries...\n");
    if (!init_reaction_libs())
    {
        CONSOLE_ERROR("Failed to initialize ReAction libraries\n");
        CONSOLE_ERROR("iTidy requires Workbench 3.2 or later with ReAction classes\n");
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

    /* GUI MIGRATION: Open the GUI window */
    CONSOLE_STATUS("Opening iTidy GUI window...\n");
    if (!open_itidy_main_window(&gui_window))
    {
        CONSOLE_ERROR("Failed to open GUI window\n");
        /* Cleanup and exit */
        cleanup_reaction_libs();
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

    CONSOLE_STATUS("GUI window opened successfully.\n");
    
    /* Check for LOADPREFS tooltype and auto-load preferences if specified */
    if (g_tooltypes.loadprefs_set && g_tooltypes.loadprefs_path[0] != '\0')
    {
        handle_tooltype_loadprefs(&gui_window);
    }
    
    CONSOLE_STATUS("Click the close gadget to exit.\n\n");
    
    /* Log CPU and memory information now that logging is fully initialized */
    CONSOLE_DEBUG("DEBUG: About to call log_system_info()...\n");
    log_system_info();
    CONSOLE_DEBUG("DEBUG: log_system_info() completed\n");

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
    CONSOLE_STATUS("\nClosing GUI window...\n");
    close_itidy_main_window(&gui_window);

    /* Cleanup ReAction libraries */
    CONSOLE_STATUS("Cleaning up ReAction libraries...\n");
    cleanup_reaction_libs();

    /* Cleanup DefIcons cache */
    CONSOLE_STATUS("Cleaning up DefIcons cache...\n");
    if (g_cached_deficons_tree)
    {
        log_debug(LOG_GUI, "Freeing DefIcons cache (%d types)\n", g_cached_deficons_count);
        free_deficons_type_tree(g_cached_deficons_tree);
        g_cached_deficons_tree = NULL;
        g_cached_deficons_count = 0;
    }

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

    /* Free PATH search list if it exists */
#ifdef DEBUG
    log_debug(LOG_GENERAL, "DEBUG: Freeing PATH search list...\n");
#endif
    FreePathSearchList();
#ifdef DEBUG
    log_debug(LOG_GENERAL, "DEBUG: PATH search list freed\n");
#endif

    /* Report memory status before shutdown */
    whd_memory_report();
    
    /* Print logging performance statistics */
    print_log_performance_stats();
    
    /* Shutdown logging system */
    shutdown_log_system();

    CONSOLE_STATUS("\niTidy GUI shutdown complete\n\n");

    /* KEEP: VBCC -lauto workaround */
#ifdef __AMIGA__
    /* NOTE: DO NOT reply to WBStartup message - VBCC's -lauto does this automatically */
    /* Replying manually causes the program to hang when launched from Workbench */
    
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
