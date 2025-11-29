/*
 * tool_cache_window.c - iTidy Tool Cache Window Implementation
 * Displays default tool validation cache with filtering and details
 * GadTools-based window for Workbench 2.0+
 */

#include "platform/platform.h"
#include "tool_cache_window.h"
#include "default_tool_update_window.h"
#include "restore_window.h"
#include "default_tool_backup.h"
#include "easy_request_helper.h"
#include "../icon_types.h"
#include "../itidy_types.h"
#include "../Settings/IControlPrefs.h"
#include "../writeLog.h"
#include "../layout_processor.h"
#include "../path_utilities.h"
#include "../utilities.h"

#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <graphics/text.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <string.h>
#include <stdio.h>

/* External graphics base for default font */
extern struct GfxBase *GfxBase;

/*------------------------------------------------------------------------*/
/* Menu Item IDs                                                          */
/*------------------------------------------------------------------------*/
#define MENU_PROJECT_NEW        5001
#define MENU_PROJECT_OPEN       5002
#define MENU_PROJECT_SAVE       5003
#define MENU_PROJECT_SAVE_AS    5004
#define MENU_PROJECT_CLOSE      5005

/*------------------------------------------------------------------------*/
/* Menu System Global Variables                                          */
/*------------------------------------------------------------------------*/
static struct Screen *wb_screen_menu = NULL;     /* Workbench screen for menus */
static struct DrawInfo *draw_info_menu = NULL;   /* Drawing info for menus */
static APTR visual_info_menu = NULL;             /* Visual info for menus */
static struct Menu *menu_strip = NULL;           /* Menu strip */

/*------------------------------------------------------------------------*/
/* Menu Template                                                          */
/*------------------------------------------------------------------------*/
static struct NewMenu tool_cache_menu_template[] = 
{
	{ NM_TITLE, "Project",      NULL, 0, 0, NULL },
	{ NM_ITEM,  "New",          "N",  0, 0, (APTR)MENU_PROJECT_NEW },
	{ NM_ITEM,  "Open...",      "O",  0, 0, (APTR)MENU_PROJECT_OPEN },
	{ NM_ITEM,  NM_BARLABEL,    NULL, 0, 0, NULL },
	{ NM_ITEM,  "Save",         "S",  0, 0, (APTR)MENU_PROJECT_SAVE },
	{ NM_ITEM,  "Save as...",   "A",  0, 0, (APTR)MENU_PROJECT_SAVE_AS },
	{ NM_ITEM,  NM_BARLABEL,    NULL, 0, 0, NULL },
	{ NM_ITEM,  "Close",        "C",  0, 0, (APTR)MENU_PROJECT_CLOSE },
	{ NM_END,   NULL,           NULL, 0, 0, NULL }
};

/*------------------------------------------------------------------------*/
/* External Default Tool Restore Functions                               */
/*------------------------------------------------------------------------*/
extern struct Window *iTidy_CreateToolRestoreWindow(struct Screen *screen, APTR backup_manager);
extern BOOL iTidy_HandleToolRestoreWindowEvent(struct Window *window, struct IntuiMessage *msg);
extern void iTidy_CloseToolRestoreWindow(struct Window *window);

/*------------------------------------------------------------------------*/
/* Layout Configuration                                                   */
/*------------------------------------------------------------------------*/
#define TOOL_NAME_COLUMN_WIDTH  40  /* Maximum characters for tool name column */

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static void populate_tool_list(struct iTidyToolCacheWindow *tool_data);
static void populate_details_panel(struct iTidyToolCacheWindow *tool_data);
static BOOL setup_tool_cache_menus(void);
static void cleanup_tool_cache_menus(void);
static BOOL handle_tool_cache_menu_selection(ULONG menu_number, struct iTidyToolCacheWindow *tool_data);
static BOOL save_tool_cache_to_file(const char *filepath);
static void handle_save_as_menu(struct iTidyToolCacheWindow *tool_data);

/*------------------------------------------------------------------------*/
/* Tool Cache Save/Load Functions                                        */
/*------------------------------------------------------------------------*/

/**
 * save_tool_cache_to_file - Save tool cache to binary file
 * 
 * Writes the global g_ToolCache data to a binary file in a format
 * suitable for reloading. File format:
 *   - Header: "ITIDYTOOLCACHE" (14 bytes)
 *   - Version: ULONG (4 bytes) = 1
 *   - Tool count: LONG (4 bytes)
 *   - For each tool:
 *     - Tool name length: LONG, then string
 *     - Exists flag: LONG (0 or 1)
 *     - Full path length: LONG, then string (or 0 if NULL)
 *     - Version string length: LONG, then string (or 0 if NULL)
 *     - Hit count: LONG
 *     - File count: LONG
 *     - For each file: length: LONG, then string
 * 
 * @param filepath Full path to save file
 * @return TRUE if successful, FALSE on error
 */
static BOOL save_tool_cache_to_file(const char *filepath)
{
    BPTR file;
    LONG i, j;
    LONG len;
    LONG exists_flag;
    const char header[] = "ITIDYTOOLCACHE";
    ULONG version = 1;
    
    if (!filepath || !g_ToolCache)
    {
        log_error(LOG_GUI, "save_tool_cache_to_file: Invalid parameters\\n");
        return FALSE;
    }
    
    /* Open file for writing */
    file = Open((STRPTR)filepath, MODE_NEWFILE);
    if (!file)
    {
        log_error(LOG_GUI, "save_tool_cache_to_file: Failed to create file: %s\\n", filepath);
        return FALSE;
    }
    
    log_info(LOG_GUI, "Saving tool cache to: %s\\n", filepath);
    
    /* Write header */
    if (Write(file, (APTR)header, 14) != 14)
    {
        log_error(LOG_GUI, "Failed to write header\\n");
        Close(file);
        return FALSE;
    }
    
    /* Write version */
    if (Write(file, (APTR)&version, sizeof(ULONG)) != sizeof(ULONG))
    {
        log_error(LOG_GUI, "Failed to write version\\n");
        Close(file);
        return FALSE;
    }
    
    /* Write tool count */
    if (Write(file, (APTR)&g_ToolCacheCount, sizeof(LONG)) != sizeof(LONG))
    {
        log_error(LOG_GUI, "Failed to write tool count\\n");
        Close(file);
        return FALSE;
    }
    
    /* Write each tool entry */
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        /* Write tool name */
        if (g_ToolCache[i].toolName)
        {
            len = strlen(g_ToolCache[i].toolName);
            Write(file, (APTR)&len, sizeof(LONG));
            Write(file, (APTR)g_ToolCache[i].toolName, len);
        }
        else
        {
            len = 0;
            Write(file, (APTR)&len, sizeof(LONG));
        }
        
        /* Write exists flag */
        exists_flag = g_ToolCache[i].exists ? 1 : 0;
        Write(file, (APTR)&exists_flag, sizeof(LONG));
        
        /* Write full path */
        if (g_ToolCache[i].fullPath)
        {
            len = strlen(g_ToolCache[i].fullPath);
            Write(file, (APTR)&len, sizeof(LONG));
            Write(file, (APTR)g_ToolCache[i].fullPath, len);
        }
        else
        {
            len = 0;
            Write(file, (APTR)&len, sizeof(LONG));
        }
        
        /* Write version string */
        if (g_ToolCache[i].versionString)
        {
            len = strlen(g_ToolCache[i].versionString);
            Write(file, (APTR)&len, sizeof(LONG));
            Write(file, (APTR)g_ToolCache[i].versionString, len);
        }
        else
        {
            len = 0;
            Write(file, (APTR)&len, sizeof(LONG));
        }
        
        /* Write hit count */
        Write(file, (APTR)&g_ToolCache[i].hitCount, sizeof(LONG));
        
        /* Write file count */
        Write(file, (APTR)&g_ToolCache[i].fileCount, sizeof(LONG));
        
        /* Write file references */
        for (j = 0; j < g_ToolCache[i].fileCount; j++)
        {
            if (g_ToolCache[i].referencingFiles[j])
            {
                len = strlen(g_ToolCache[i].referencingFiles[j]);
                Write(file, (APTR)&len, sizeof(LONG));
                Write(file, (APTR)g_ToolCache[i].referencingFiles[j], len);
            }
            else
            {
                len = 0;
                Write(file, (APTR)&len, sizeof(LONG));
            }
        }
    }
    
    Close(file);
    log_info(LOG_GUI, "Successfully saved %d tools to cache file\\n", g_ToolCacheCount);
    return TRUE;
}

/**
 * load_tool_cache_from_file - Load tool cache from binary file
 * 
 * Reads and validates a tool cache file, then replaces the global cache.
 * 
 * @param filepath Full path to load file
 * @return TRUE if successful, FALSE on error
 */
static BOOL load_tool_cache_from_file(const char *filepath)
{
    BPTR file;
    LONG i, j;
    LONG len;
    LONG exists_flag;
    LONG tool_count;
    char header[15];
    ULONG version;
    ToolCacheEntry *temp_cache = NULL;
    
    if (!filepath)
    {
        log_error(LOG_GUI, "load_tool_cache_from_file: Invalid filepath\\n");
        return FALSE;
    }
    
    log_info(LOG_GUI, "Loading tool cache from: %s\\n", filepath);
    
    /* Open file for reading */
    file = Open((STRPTR)filepath, MODE_OLDFILE);
    if (!file)
    {
        log_error(LOG_GUI, "Failed to open file for reading: %s\\n", filepath);
        return FALSE;
    }
    
    /* Read and validate header */
    memset(header, 0, sizeof(header));
    if (Read(file, (APTR)header, 14) != 14)
    {
        log_error(LOG_GUI, "Failed to read header\\n");
        Close(file);
        return FALSE;
    }
    
    if (strncmp(header, "ITIDYTOOLCACHE", 14) != 0)
    {
        log_error(LOG_GUI, "Invalid file format (bad header)\\n");
        Close(file);
        return FALSE;
    }
    
    /* Read version */
    if (Read(file, (APTR)&version, sizeof(ULONG)) != sizeof(ULONG))
    {
        log_error(LOG_GUI, "Failed to read version\\n");
        Close(file);
        return FALSE;
    }
    
    if (version != 1)
    {
        log_error(LOG_GUI, "Unsupported file version: %lu\\n", version);
        Close(file);
        return FALSE;
    }
    
    /* Read tool count */
    if (Read(file, (APTR)&tool_count, sizeof(LONG)) != sizeof(LONG))
    {
        log_error(LOG_GUI, "Failed to read tool count\\n");
        Close(file);
        return FALSE;
    }
    
    if (tool_count < 0 || tool_count > 10000)  /* Sanity check */
    {
        log_error(LOG_GUI, "Invalid tool count: %ld\\n", tool_count);
        Close(file);
        return FALSE;
    }
    
    log_debug(LOG_GUI, "Loading %ld tools from cache file\\n", tool_count);
    
    /* Allocate temporary cache array */
    if (tool_count > 0)
    {
        temp_cache = (ToolCacheEntry *)whd_malloc(tool_count * sizeof(ToolCacheEntry));
        if (!temp_cache)
        {
            log_error(LOG_GUI, "Failed to allocate temporary cache\\n");
            Close(file);
            return FALSE;
        }
        memset(temp_cache, 0, tool_count * sizeof(ToolCacheEntry));
    }
    
    /* Read each tool entry */
    for (i = 0; i < tool_count; i++)
    {
        /* Read tool name */
        if (Read(file, (APTR)&len, sizeof(LONG)) != sizeof(LONG))
        {
            log_error(LOG_GUI, "Failed to read tool name length at index %ld\\n", i);
            goto load_error;
        }
        
        if (len > 0)
        {
            if (len > 512)  /* Sanity check */
            {
                log_error(LOG_GUI, "Tool name too long: %ld\\n", len);
                goto load_error;
            }
            
            temp_cache[i].toolName = (char *)whd_malloc(len + 1);
            if (!temp_cache[i].toolName)
            {
                log_error(LOG_GUI, "Failed to allocate tool name\\n");
                goto load_error;
            }
            
            if (Read(file, (APTR)temp_cache[i].toolName, len) != len)
            {
                log_error(LOG_GUI, "Failed to read tool name\\n");
                goto load_error;
            }
            temp_cache[i].toolName[len] = '\0';
        }
        
        /* Read exists flag */
        if (Read(file, (APTR)&exists_flag, sizeof(LONG)) != sizeof(LONG))
        {
            log_error(LOG_GUI, "Failed to read exists flag\\n");
            goto load_error;
        }
        temp_cache[i].exists = (exists_flag != 0);
        
        /* Read full path */
        if (Read(file, (APTR)&len, sizeof(LONG)) != sizeof(LONG))
        {
            log_error(LOG_GUI, "Failed to read path length\\n");
            goto load_error;
        }
        
        if (len > 0)
        {
            if (len > 1024)  /* Sanity check */
            {
                log_error(LOG_GUI, "Path too long: %ld\\n", len);
                goto load_error;
            }
            
            temp_cache[i].fullPath = (char *)whd_malloc(len + 1);
            if (!temp_cache[i].fullPath)
            {
                log_error(LOG_GUI, "Failed to allocate path\\n");
                goto load_error;
            }
            
            if (Read(file, (APTR)temp_cache[i].fullPath, len) != len)
            {
                log_error(LOG_GUI, "Failed to read path\\n");
                goto load_error;
            }
            temp_cache[i].fullPath[len] = '\0';
        }
        
        /* Read version string */
        if (Read(file, (APTR)&len, sizeof(LONG)) != sizeof(LONG))
        {
            log_error(LOG_GUI, "Failed to read version length\\n");
            goto load_error;
        }
        
        if (len > 0)
        {
            if (len > 256)  /* Sanity check */
            {
                log_error(LOG_GUI, "Version string too long: %ld\\n", len);
                goto load_error;
            }
            
            temp_cache[i].versionString = (char *)whd_malloc(len + 1);
            if (!temp_cache[i].versionString)
            {
                log_error(LOG_GUI, "Failed to allocate version string\\n");
                goto load_error;
            }
            
            if (Read(file, (APTR)temp_cache[i].versionString, len) != len)
            {
                log_error(LOG_GUI, "Failed to read version string\\n");
                goto load_error;
            }
            temp_cache[i].versionString[len] = '\0';
        }
        
        /* Read hit count */
        if (Read(file, (APTR)&temp_cache[i].hitCount, sizeof(LONG)) != sizeof(LONG))
        {
            log_error(LOG_GUI, "Failed to read hit count\\n");
            goto load_error;
        }
        
        /* Read file count */
        if (Read(file, (APTR)&temp_cache[i].fileCount, sizeof(LONG)) != sizeof(LONG))
        {
            log_error(LOG_GUI, "Failed to read file count\\n");
            goto load_error;
        }
        
        if (temp_cache[i].fileCount < 0 || temp_cache[i].fileCount > 200)  /* Sanity check */
        {
            log_error(LOG_GUI, "Invalid file count: %ld\\n", temp_cache[i].fileCount);
            goto load_error;
        }
        
        /* Allocate file references array if needed */
        if (temp_cache[i].fileCount > 0)
        {
            temp_cache[i].referencingFiles = (char **)whd_malloc(temp_cache[i].fileCount * sizeof(char *));
            if (!temp_cache[i].referencingFiles)
            {
                log_error(LOG_GUI, "Failed to allocate file references array\\n");
                goto load_error;
            }
            memset(temp_cache[i].referencingFiles, 0, temp_cache[i].fileCount * sizeof(char *));
            
            /* Read each file reference */
            for (j = 0; j < temp_cache[i].fileCount; j++)
            {
                if (Read(file, (APTR)&len, sizeof(LONG)) != sizeof(LONG))
                {
                    log_error(LOG_GUI, "Failed to read file reference length\\n");
                    goto load_error;
                }
                
                if (len > 0)
                {
                    if (len > 1024)  /* Sanity check */
                    {
                        log_error(LOG_GUI, "File reference too long: %ld\\n", len);
                        goto load_error;
                    }
                    
                    temp_cache[i].referencingFiles[j] = (char *)whd_malloc(len + 1);
                    if (!temp_cache[i].referencingFiles[j])
                    {
                        log_error(LOG_GUI, "Failed to allocate file reference\\n");
                        goto load_error;
                    }
                    
                    if (Read(file, (APTR)temp_cache[i].referencingFiles[j], len) != len)
                    {
                        log_error(LOG_GUI, "Failed to read file reference\\n");
                        goto load_error;
                    }
                    temp_cache[i].referencingFiles[j][len] = '\0';
                }
            }
        }
    }
    
    Close(file);
    
    /* Success - replace global cache */
    FreeToolCache();  /* Free old cache */
    
    g_ToolCache = temp_cache;
    g_ToolCacheCount = tool_count;
    g_ToolCacheCapacity = tool_count;
    
    log_info(LOG_GUI, "Successfully loaded %ld tools from cache file\\n", tool_count);
    return TRUE;
    
load_error:
    /* Cleanup on error */
    Close(file);
    
    if (temp_cache)
    {
        for (i = 0; i < tool_count; i++)
        {
            if (temp_cache[i].toolName)
                whd_free(temp_cache[i].toolName);
            if (temp_cache[i].fullPath)
                whd_free(temp_cache[i].fullPath);
            if (temp_cache[i].versionString)
                whd_free(temp_cache[i].versionString);
            
            if (temp_cache[i].referencingFiles)
            {
                for (j = 0; j < temp_cache[i].fileCount; j++)
                {
                    if (temp_cache[i].referencingFiles[j])
                        whd_free(temp_cache[i].referencingFiles[j]);
                }
                whd_free(temp_cache[i].referencingFiles);
            }
        }
        whd_free(temp_cache);
    }
    
    return FALSE;
}

/**
 * handle_save_as_menu - Handle "Save as..." menu selection
 * 
 * Opens an ASL file requester to let user choose save location,
 * checks for file overwrite, and saves the tool cache.
 * 
 * @param tool_data Pointer to tool cache window data
 */
static void handle_save_as_menu(struct iTidyToolCacheWindow *tool_data)
{
    struct FileRequester *freq;
    char full_path[512];
    char expanded_drawer[512];
    BPTR lock;
    
    if (!tool_data || !tool_data->window)
        return;
    
    /* Check if there's data to save */
    if (!g_ToolCache || g_ToolCacheCount == 0)
    {
        ShowEasyRequest(tool_data->window,
            "No Data to Save",
            "The tool cache is empty.\\nPlease scan a directory first.",
            "OK");
        return;
    }
    
    /* Expand PROGDIR: to actual path for ASL requester */
    if (!ExpandProgDir("PROGDIR:userdata/DTools", expanded_drawer, sizeof(expanded_drawer)))
    {
        log_warning(LOG_GUI, "Could not expand PROGDIR:, using fallback path\n");
        strcpy(expanded_drawer, "RAM:");  /* Fallback to RAM: */
    }
    
    log_debug(LOG_GUI, "Using initial drawer: %s\n", expanded_drawer);
    
    /* Allocate file requester */
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Save Tool Cache As...",
        ASLFR_InitialDrawer, expanded_drawer,
        ASLFR_InitialFile, "toolcache.dat",
        ASLFR_DoSaveMode, TRUE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, tool_data->window,
        TAG_END);
    
    if (!freq)
    {
        log_error(LOG_GUI, "Failed to allocate file requester\\n");
        ShowEasyRequest(tool_data->window,
            "Error",
            "Could not open file requester.",
            "OK");
        return;
    }
    
    /* Display requester */
    if (AslRequest(freq, NULL))
    {
        /* Build full path */
        strcpy(full_path, freq->fr_Drawer);
        if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            log_error(LOG_GUI, "Path too long: %s + %s\\n", freq->fr_Drawer, freq->fr_File);
            FreeAslRequest(freq);
            ShowEasyRequest(tool_data->window,
                "Error",
                "File path is too long.",
                "OK");
            return;
        }
        
        log_info(LOG_GUI, "User selected save path: %s\\n", full_path);
        
        /* Check if file exists */
        lock = Lock((STRPTR)full_path, ACCESS_READ);
        if (lock)
        {
            UnLock(lock);
            
            /* File exists - ask for confirmation */
            if (!ShowEasyRequest(tool_data->window,
                "File Exists",
                "File already exists.\\nDo you want to replace it?",
                "Replace|Cancel"))
            {
                log_info(LOG_GUI, "User cancelled overwrite\\n");
                FreeAslRequest(freq);
                return;
            }
        }
        
        /* Save the file */
        if (save_tool_cache_to_file(full_path))
        {
            ShowEasyRequest(tool_data->window,
                "Save Successful",
                "Tool cache saved successfully.",
                "OK");
            log_info(LOG_GUI, "Tool cache saved to: %s\\n", full_path);
        }
        else
        {
            ShowEasyRequest(tool_data->window,
                "Save Failed",
                "Failed to save tool cache file.",
                "OK");
        }
    }
    else
    {
        log_info(LOG_GUI, "User cancelled save operation\\n");
    }
    
    FreeAslRequest(freq);
}

/**
 * handle_open_menu - Handle "Open..." menu selection
 * 
 * Opens an ASL file requester to let user choose a cache file to load,
 * validates the file, loads it, and updates the display.
 * 
 * @param tool_data Pointer to tool cache window data
 */
static void handle_open_menu(struct iTidyToolCacheWindow *tool_data)
{
    struct FileRequester *freq;
    char full_path[512];
    char expanded_drawer[512];
    BPTR lock;
    
    if (!tool_data || !tool_data->window)
        return;
    
    log_debug(LOG_GUI, "Menu: Open... clicked\n");
    
    /* Expand PROGDIR: to actual path for ASL requester */
    if (!ExpandProgDir("PROGDIR:userdata/DTools", expanded_drawer, sizeof(expanded_drawer)))
    {
        log_warning(LOG_GUI, "Could not expand PROGDIR:, using fallback path\n");
        strcpy(expanded_drawer, "RAM:");  /* Fallback to RAM: */
    }
    
    log_debug(LOG_GUI, "Using initial drawer: %s\n", expanded_drawer);
    
    /* Allocate file requester */
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Open Tool Cache File...",
        ASLFR_InitialDrawer, expanded_drawer,
        ASLFR_InitialFile, "toolcache.dat",
        ASLFR_DoSaveMode, FALSE,  /* Open mode, not save */
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, tool_data->window,
        TAG_END);
    
    if (!freq)
    {
        log_error(LOG_GUI, "Failed to allocate file requester\\n");
        ShowEasyRequest(tool_data->window,
            "Error",
            "Could not open file requester.",
            "OK");
        return;
    }
    
    /* Display requester */
    if (AslRequest(freq, NULL))
    {
        /* Build full path */
        strcpy(full_path, freq->fr_Drawer);
        if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            log_error(LOG_GUI, "Path too long: %s + %s\\n", freq->fr_Drawer, freq->fr_File);
            FreeAslRequest(freq);
            ShowEasyRequest(tool_data->window,
                "Error",
                "File path is too long.",
                "OK");
            return;
        }
        
        log_info(LOG_GUI, "User selected file: %s\\n", full_path);
        
        /* Check if file exists */
        lock = Lock((STRPTR)full_path, ACCESS_READ);
        if (!lock)
        {
            log_error(LOG_GUI, "File does not exist: %s\\n", full_path);
            FreeAslRequest(freq);
            ShowEasyRequest(tool_data->window,
                "File Not Found",
                "The selected file does not exist.",
                "OK");
            return;
        }
        UnLock(lock);
        
        /* Load the file */
        if (load_tool_cache_from_file(full_path))
        {
            /* Rebuild the display with new data */
            if (build_tool_cache_display_list(tool_data))
            {
                /* Apply current filter */
                apply_tool_filter(tool_data);
                
                /* Update ListView using populate function */
                populate_tool_list(tool_data);
                
                ShowEasyRequest(tool_data->window,
                    "Load Successful",
                    "Tool cache loaded successfully.",
                    "OK");
                log_info(LOG_GUI, "Tool cache loaded from: %s\\n", full_path);
            }
            else
            {
                ShowEasyRequest(tool_data->window,
                    "Display Error",
                    "Cache loaded but failed to update display.",
                    "OK");
            }
        }
        else
        {
            ShowEasyRequest(tool_data->window,
                "Load Failed",
                "Failed to load tool cache file.\\nFile may be corrupted or invalid.",
                "OK");
        }
    }
    else
    {
        log_info(LOG_GUI, "User cancelled load operation\\n");
    }
    
    FreeAslRequest(freq);
}

/*------------------------------------------------------------------------*/
/* Menu System Functions                                                  */
/*------------------------------------------------------------------------*/

/**
 * setup_tool_cache_menus - Initialize GadTools NewLook menu system
 * 
 * This function sets up GadTools menus with proper Workbench 3.x NewLook
 * appearance. The menus will automatically use system colors and modern
 * white background styling.
 *
 * Returns: TRUE if successful, FALSE on failure
 */
static BOOL setup_tool_cache_menus(void)
{
    /* Lock Workbench screen for menu system */
    wb_screen_menu = LockPubScreen("Workbench");
    if (!wb_screen_menu)
    {
        log_error(LOG_GUI, "Error: Could not lock Workbench screen for menus\\n");
        return FALSE;
    }
    
    /* Get screen drawing information */
    draw_info_menu = GetScreenDrawInfo(wb_screen_menu);
    if (!draw_info_menu)
    {
        log_error(LOG_GUI, "Error: Could not get screen DrawInfo for menus\\n");
        UnlockPubScreen(NULL, wb_screen_menu);
        wb_screen_menu = NULL;
        return FALSE;
    }
    
    /* Get visual information for GadTools */
    visual_info_menu = GetVisualInfo(wb_screen_menu, TAG_END);
    if (!visual_info_menu)
    {
        log_error(LOG_GUI, "Error: Could not get VisualInfo for menus\\n");
        FreeScreenDrawInfo(wb_screen_menu, draw_info_menu);
        UnlockPubScreen(NULL, wb_screen_menu);
        draw_info_menu = NULL;
        wb_screen_menu = NULL;
        return FALSE;
    }
    
    /* Create menu strip from template */
    menu_strip = CreateMenus(tool_cache_menu_template, TAG_END);
    if (!menu_strip)
    {
        log_error(LOG_GUI, "Error: Could not create menu strip\\n");
        FreeVisualInfo(visual_info_menu);
        FreeScreenDrawInfo(wb_screen_menu, draw_info_menu);
        UnlockPubScreen(NULL, wb_screen_menu);
        visual_info_menu = NULL;
        draw_info_menu = NULL;
        wb_screen_menu = NULL;
        return FALSE;
    }
    
    /* Layout menus with NewLook appearance */
    if (!LayoutMenus(menu_strip, visual_info_menu, GTMN_NewLookMenus, TRUE, TAG_END))
    {
        log_error(LOG_GUI, "Error: Could not layout NewLook menus\\n");
        FreeMenus(menu_strip);
        FreeVisualInfo(visual_info_menu);
        FreeScreenDrawInfo(wb_screen_menu, draw_info_menu);
        UnlockPubScreen(NULL, wb_screen_menu);
        menu_strip = NULL;
        visual_info_menu = NULL;
        draw_info_menu = NULL;
        wb_screen_menu = NULL;
        return FALSE;
    }
    
    log_info(LOG_GUI, "Tool cache menus initialized successfully\\n");
    return TRUE;
}

/**
 * cleanup_tool_cache_menus - Release all menu system resources
 * 
 * This function properly releases all resources allocated during
 * menu setup, following proper AmigaOS resource management.
 */
static void cleanup_tool_cache_menus(void)
{
    if (menu_strip)
    {
        FreeMenus(menu_strip);
        menu_strip = NULL;
    }
    
    if (visual_info_menu)
    {
        FreeVisualInfo(visual_info_menu);
        visual_info_menu = NULL;
    }
    
    if (draw_info_menu)
    {
        FreeScreenDrawInfo(wb_screen_menu, draw_info_menu);
        draw_info_menu = NULL;
    }
    
    if (wb_screen_menu)
    {
        UnlockPubScreen(NULL, wb_screen_menu);
        wb_screen_menu = NULL;
    }
    
    log_info(LOG_GUI, "Tool cache menus cleaned up\\n");
}

/**
 * handle_tool_cache_menu_selection - Process menu item selections
 * 
 * This function handles menu item selections by examining the menu
 * item ID and performing the appropriate action using EasyRequest
 * message boxes for demonstration.
 *
 * @param menu_number: The menu selection number from IDCMP_MENUPICK
 * @param tool_data: Pointer to tool cache window data structure
 * @return: TRUE to continue running, FALSE to close window
 */
static BOOL handle_tool_cache_menu_selection(ULONG menu_number, struct iTidyToolCacheWindow *tool_data)
{
    struct MenuItem *menu_item = NULL;
    ULONG item_id = 0;
    BOOL continue_running = TRUE;
    
    while (menu_number != MENUNULL)
    {
        menu_item = ItemAddress(menu_strip, menu_number);
        if (menu_item)
        {
            item_id = (ULONG)GTMENUITEM_USERDATA(menu_item);
            
            /* Handle menu selections */
            switch (item_id)
            {
                case MENU_PROJECT_NEW:
                    ShowEasyRequest(
                        tool_data->window,
                        "Menu Selection",
                        "New menu item selected",
                        "OK");
                    break;
                    
                case MENU_PROJECT_OPEN:
                    handle_open_menu(tool_data);
                    break;
                    
                case MENU_PROJECT_SAVE:
                    ShowEasyRequest(
                        tool_data->window,
                        "Menu Selection",
                        "Save menu item selected",
                        "OK");
                    break;
                    
                case MENU_PROJECT_SAVE_AS:
                    handle_save_as_menu(tool_data);
                    break;
                    
                case MENU_PROJECT_CLOSE:
                    ShowEasyRequest(
                        tool_data->window,
                        "Menu Selection",
                        "Close menu item selected",
                        "OK");
                    continue_running = FALSE;  /* Close window */
                    break;
                    
                default:
                    log_warning(LOG_GUI, "Unknown menu item ID: %ld\\n", item_id);
                    break;
            }
        }
        
        menu_number = menu_item->NextSelect;
    }
    
    return continue_running;
}

/*------------------------------------------------------------------------*/
/* Build Tool Cache Display List                                         */
/*------------------------------------------------------------------------*/
BOOL build_tool_cache_display_list(struct iTidyToolCacheWindow *tool_data)
{
    int i;
    struct ToolCacheDisplayEntry *entry;
    char buffer[256];
    char header_buffer[256];
    char separator_buffer[256];
    
    if (tool_data == NULL)
        return FALSE;
    
    /* Clear any existing entries */
    free_tool_cache_entries(tool_data);
    
    /* Initialize counts */
    tool_data->total_count = 0;
    tool_data->valid_count = 0;
    tool_data->missing_count = 0;
    
    /* Check if cache exists */
    if (g_ToolCache == NULL || g_ToolCacheCount == 0)
    {
        append_to_log("build_tool_cache_display_list: No tools in cache\n");
        return TRUE;  /* Empty list is valid */
    }
    
    append_to_log("build_tool_cache_display_list: Processing %d cached tools\n", g_ToolCacheCount);
    
    /* ---- ADD COLUMN HEADER ROWS ---- */
    
    /* First header row: Column names */
    sprintf(header_buffer, "%-*s| %4s | %s",
        TOOL_NAME_COLUMN_WIDTH,
        "Tool",
        "Refs",
        "Status");
    
    entry = (struct ToolCacheDisplayEntry *)whd_malloc(sizeof(struct ToolCacheDisplayEntry));
    if (entry == NULL)
    {
        append_to_log("ERROR: Failed to allocate header entry\n");
        return FALSE;
    }
    memset(entry, 0, sizeof(struct ToolCacheDisplayEntry));
    
    entry->display_text = (char *)whd_malloc(strlen(header_buffer) + 1);
    if (entry->display_text == NULL)
    {
        FreeVec(entry);
        return FALSE;
    }
    memset(entry->display_text, 0, strlen(header_buffer) + 1);
    strcpy(entry->display_text, header_buffer);
    entry->node.ln_Name = entry->display_text;
    AddTail(&tool_data->tool_entries, (struct Node *)entry);
    
    /* Second header row: Separator line */
    /* Fill with dashes to match the total width */
    /* Format: 40 (tool) + "| " (2) + 4 (refs) + " | " (3) + 7 (status) = 56 total */
    memset(separator_buffer, '-', 56);
    separator_buffer[56] = '\0';
    
    entry = (struct ToolCacheDisplayEntry *)whd_malloc(sizeof(struct ToolCacheDisplayEntry));
    if (entry == NULL)
    {
        append_to_log("ERROR: Failed to allocate separator entry\n");
        free_tool_cache_entries(tool_data);
        return FALSE;
    }
    memset(entry, 0, sizeof(struct ToolCacheDisplayEntry));
    
    entry->display_text = (char *)whd_malloc(strlen(separator_buffer) + 1);
    if (entry->display_text == NULL)
    {
        FreeVec(entry);
        free_tool_cache_entries(tool_data);
        return FALSE;
    }
    memset(entry->display_text, 0, strlen(separator_buffer) + 1);
    strcpy(entry->display_text, separator_buffer);
    entry->node.ln_Name = entry->display_text;
    AddTail(&tool_data->tool_entries, (struct Node *)entry);
    
    /* ---- END HEADER ROWS ---- */
    
    /* Build display entries from global cache */
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        char truncated_name[TOOL_NAME_COLUMN_WIDTH + 1];  /* Buffer for truncated tool name */
        
        /* Allocate entry */
        entry = (struct ToolCacheDisplayEntry *)whd_malloc(sizeof(struct ToolCacheDisplayEntry));
        if (entry == NULL)
        {
            append_to_log("ERROR: Failed to allocate display entry\n");
            free_tool_cache_entries(tool_data);
            return FALSE;
        }
        memset(entry, 0, sizeof(struct ToolCacheDisplayEntry));
        
        /* Copy tool data */
        entry->tool_name = g_ToolCache[i].toolName;  /* Point to cache data - don't duplicate */
        entry->exists = g_ToolCache[i].exists;
        entry->hit_count = g_ToolCache[i].hitCount;
        entry->full_path = g_ToolCache[i].fullPath;  /* Point to cache data */
        entry->version = g_ToolCache[i].versionString;  /* Point to cache data */
        
        /* Abbreviate tool name with /../ notation if needed (max TOOL_NAME_COLUMN_WIDTH chars) */
        if (!iTidy_ShortenPathWithParentDir(
            entry->tool_name ? entry->tool_name : "(unknown)",
            truncated_name,
            TOOL_NAME_COLUMN_WIDTH))
        {
            /* Path already fits or couldn't be abbreviated - copy as-is (up to max length) */
            strncpy(truncated_name, entry->tool_name ? entry->tool_name : "(unknown)", TOOL_NAME_COLUMN_WIDTH);
            truncated_name[TOOL_NAME_COLUMN_WIDTH] = '\0';
        }
        
        /* Log if truncation occurred */
        if (entry->tool_name && strlen(entry->tool_name) > TOOL_NAME_COLUMN_WIDTH)
        {
            append_to_log("  Truncated: '%s' -> '%s'\n", entry->tool_name, truncated_name);
        }
        
        /* Format display text: "Tool Name             | Hits | Status  " */
        /* Use fixed-width format for column alignment */
        /* Add 1 to hit_count for display (user-friendly numbering starting from 1) */
        sprintf(buffer, "%-*s| %4d | %s",
            TOOL_NAME_COLUMN_WIDTH,
            truncated_name,
            entry->hit_count + 1,
            entry->exists ? "EXISTS " : "MISSING");
        
        /* Allocate and copy display text */
        entry->display_text = (char *)whd_malloc(strlen(buffer) + 1);
        if (entry->display_text == NULL)
        {
            FreeVec(entry);
            free_tool_cache_entries(tool_data);
            return FALSE;
        }
        memset(entry->display_text, 0, strlen(buffer) + 1);
        strcpy(entry->display_text, buffer);
        
        /* Set node name for GadTools listview */
        entry->node.ln_Name = entry->display_text;
        
        /* Add to list */
        AddTail(&tool_data->tool_entries, (struct Node *)entry);
        
        /* Update counts */
        tool_data->total_count++;
        if (entry->exists)
            tool_data->valid_count++;
        else
            tool_data->missing_count++;
    }
    
    /* Build summary text */
    sprintf(tool_data->summary_text, "Total Tools: %lu  |  Valid: %lu  |  Missing: %lu",
        tool_data->total_count, tool_data->valid_count, tool_data->missing_count);
    
    append_to_log("build_tool_cache_display_list: Created %lu entries\n", tool_data->total_count);
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Apply Filter                                                           */
/*------------------------------------------------------------------------*/
void apply_tool_filter(struct iTidyToolCacheWindow *tool_data)
{
    struct ToolCacheDisplayEntry *entry;
    struct Node *node;
    ULONG count = 0;
    
    if (tool_data == NULL)
        return;
    
    /* Clear filtered list by rebuilding it */
    /* NOTE: The filtered_entries list uses the filter_node member of each entry.
       This allows the same entry to appear in both tool_entries (via node) 
       and filtered_entries (via filter_node) simultaneously. */
    NewList(&tool_data->filtered_entries);
    
    /* Build filtered list based on current filter */
    for (node = tool_data->tool_entries.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ)
    {
        entry = (struct ToolCacheDisplayEntry *)node;
        
        /* Always include header rows (they have NULL tool_name) */
        if (entry->tool_name == NULL)
        {
            entry->filter_node.ln_Name = entry->display_text;
            AddTail(&tool_data->filtered_entries, &entry->filter_node);
            continue;  /* Skip filter logic for headers */
        }
        
        /* Apply filter and add to filtered list using filter_node */
        switch (tool_data->current_filter)
        {
            case TOOL_FILTER_ALL:
                /* Add all entries using filter_node */
                entry->filter_node.ln_Name = entry->display_text;  /* Point to same display text */
                AddTail(&tool_data->filtered_entries, &entry->filter_node);
                count++;
                break;
                
            case TOOL_FILTER_VALID:
                /* Only add tools that exist */
                if (entry->exists)
                {
                    entry->filter_node.ln_Name = entry->display_text;
                    AddTail(&tool_data->filtered_entries, &entry->filter_node);
                    count++;
                }
                break;
                
            case TOOL_FILTER_MISSING:
                /* Only add missing tools */
                if (!entry->exists)
                {
                    entry->filter_node.ln_Name = entry->display_text;
                    AddTail(&tool_data->filtered_entries, &entry->filter_node);
                    count++;
                }
                break;
        }
    }
    
    append_to_log("apply_tool_filter: Filter=%d, Result count=%lu\n",
        tool_data->current_filter, count);
}

/*------------------------------------------------------------------------*/
/* Update Details Panel                                                   */
/*------------------------------------------------------------------------*/
void update_tool_details(struct iTidyToolCacheWindow *tool_data, LONG selected_index)
{
    struct ToolCacheDisplayEntry *entry;
    struct Node *node;
    struct Node *detail_node;
    LONG index = 0;
    char buffer[512];
    int i;
    
    if (tool_data == NULL)
        return;
    
    log_info(LOG_GUI, "[TOOL_CACHE] update_tool_details() called with selected_index=%ld\n", selected_index);
    
    /* Clear details list */
    while ((detail_node = RemHead(&tool_data->details_list)) != NULL)
    {
        if (detail_node->ln_Name != NULL)
            FreeVec(detail_node->ln_Name);
        FreeVec(detail_node);
    }
    
    /* Store selected index */
    tool_data->selected_index = selected_index;
    
    /* Clear details selection when tool changes */
    tool_data->selected_details_index = -1;
    
    /* If nothing selected, show empty details */
    if (selected_index < 0)
    {
        log_info(LOG_GUI, "[TOOL_CACHE] No tool selected (index < 0), clearing details\n");
        populate_details_panel(tool_data);
        return;
    }
    
    /* Find selected entry in filtered list */
    for (node = tool_data->filtered_entries.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ, index++)
    {
        if (index == selected_index)
        {
            /* Calculate entry pointer from filter_node offset */
            /* filter_node is the second member of ToolCacheDisplayEntry, after node */
            entry = (struct ToolCacheDisplayEntry *)((char *)node - sizeof(struct Node));
            
            log_info(LOG_GUI, "[TOOL_CACHE] Found selected entry: tool_name='%s'\n", 
                         entry->tool_name ? entry->tool_name : "(null)");
            
            /* Find the corresponding cache entry to get file references */
            for (i = 0; i < g_ToolCacheCount; i++)
            {
                if (g_ToolCache[i].toolName && entry->tool_name && 
                    strcmp(g_ToolCache[i].toolName, entry->tool_name) == 0)
                {
                    int j;
                    
                    log_info(LOG_GUI, "[TOOL_CACHE] Matched cache entry %d: fileCount=%d\n", 
                                 i, g_ToolCache[i].fileCount);
                    
                    /* Add tool name on first line */
                    sprintf(buffer, "Tool:   %s",
                        entry->tool_name ? entry->tool_name : "(unknown)");
                    
                    detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                    if (detail_node)
                    {
                        memset(detail_node, 0, sizeof(struct Node));
                        detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                        if (detail_node->ln_Name)
                        {
                            memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                            strcpy(detail_node->ln_Name, buffer);
                            AddTail(&tool_data->details_list, detail_node);
                        }
                        else
                        {
                            FreeVec(detail_node);
                        }
                    }
                    
                    /* Add status and version on second line */
                    sprintf(buffer, "Status: %s  |  Version: %s",
                        entry->exists ? "EXISTS" : "MISSING",
                        entry->version ? entry->version : "(no version)");
                    
                    detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                    if (detail_node)
                    {
                        memset(detail_node, 0, sizeof(struct Node));
                        detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                        if (detail_node->ln_Name)
                        {
                            memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                            strcpy(detail_node->ln_Name, buffer);
                            AddTail(&tool_data->details_list, detail_node);
                        }
                        else
                        {
                            FreeVec(detail_node);
                        }
                    }
                    
                    /* Add separator using simple dashes (Amiga doesn't support Unicode box chars) */
                    sprintf(buffer, "--------------------------------------------------------------------");
                    detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                    if (detail_node)
                    {
                        memset(detail_node, 0, sizeof(struct Node));
                        detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                        if (detail_node->ln_Name)
                        {
                            memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                            strcpy(detail_node->ln_Name, buffer);
                            AddTail(&tool_data->details_list, detail_node);
                        }
                        else
                        {
                            FreeVec(detail_node);
                        }
                    }
                    
                    /* Add file references */
                    if (g_ToolCache[i].fileCount > 0 && g_ToolCache[i].referencingFiles)
                    {
                        for (j = 0; j < g_ToolCache[i].fileCount; j++)
                        {
                            if (g_ToolCache[i].referencingFiles[j])
                            {
                                /* Abbreviate long paths with /../ notation if needed */
                                char truncated_path[80];
                                if (!iTidy_ShortenPathWithParentDir(g_ToolCache[i].referencingFiles[j], 
                                                                     truncated_path, 70))
                                {
                                    /* Path already fits - copy as-is (up to max length) */
                                    strncpy(truncated_path, g_ToolCache[i].referencingFiles[j], 70);
                                    truncated_path[70] = '\0';
                                }
                                
                                sprintf(buffer, "%s", truncated_path);
                                
                                detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                                if (detail_node)
                                {
                                    memset(detail_node, 0, sizeof(struct Node));
                                    detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                                    if (detail_node->ln_Name)
                                    {
                                        memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                                        strcpy(detail_node->ln_Name, buffer);
                                        AddTail(&tool_data->details_list, detail_node);
                                    }
                                    else
                                    {
                                        FreeVec(detail_node);
                                    }
                                }
                            }
                        }
                        
                        /* Add footer if truncated */
                        if (g_ToolCache[i].fileCount >= TOOL_CACHE_MAX_FILES_PER_TOOL)
                        {
                            sprintf(buffer, "(showing first %d files, more may exist)", 
                                   TOOL_CACHE_MAX_FILES_PER_TOOL);
                            detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                            if (detail_node)
                            {
                                memset(detail_node, 0, sizeof(struct Node));
                                detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                                if (detail_node->ln_Name)
                                {
                                    memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                                    strcpy(detail_node->ln_Name, buffer);
                                    AddTail(&tool_data->details_list, detail_node);
                                }
                                else
                                {
                                    FreeVec(detail_node);
                                }
                            }
                        }
                    }
                    else
                    {
                        /* No files found */
                        sprintf(buffer, "(no files using this tool)");
                        detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                        if (detail_node)
                        {
                            memset(detail_node, 0, sizeof(struct Node));
                            detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                            if (detail_node->ln_Name)
                            {
                                memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                                strcpy(detail_node->ln_Name, buffer);
                                AddTail(&tool_data->details_list, detail_node);
                            }
                            else
                            {
                                FreeVec(detail_node);
                            }
                        }
                    }
                    
                    break;  /* Found the tool, exit loop */
                }
            }
            
            break;  /* Found the selected entry, exit loop */
        }
    }
    
    /* Count details list items for logging */
    {
        int detail_count = 0;
        struct Node *count_node;
        for (count_node = tool_data->details_list.lh_Head; 
             count_node->ln_Succ != NULL; 
             count_node = count_node->ln_Succ)
        {
            detail_count++;
        }
        log_info(LOG_GUI, "[TOOL_CACHE] Calling populate_details_panel() with %d items in details_list\n", 
                     detail_count);
    }
    
    /* Update details listview */
    populate_details_panel(tool_data);
}

/*------------------------------------------------------------------------*/
/* Free Tool Cache Entries                                               */
/*------------------------------------------------------------------------*/
void free_tool_cache_entries(struct iTidyToolCacheWindow *tool_data)
{
    struct ToolCacheDisplayEntry *entry;
    struct Node *node;
    
    if (tool_data == NULL)
        return;
    
    /* Free tool entries */
    while ((node = RemHead(&tool_data->tool_entries)) != NULL)
    {
        entry = (struct ToolCacheDisplayEntry *)node;
        if (entry->display_text != NULL)
            FreeVec(entry->display_text);
        /* Note: Don't free tool_name, full_path, version - they point to g_ToolCache */
        FreeVec(entry);
    }
    
    /* Free details list */
    while ((node = RemHead(&tool_data->details_list)) != NULL)
    {
        if (node->ln_Name != NULL)
            FreeVec(node->ln_Name);
        FreeVec(node);
    }
}

/*------------------------------------------------------------------------*/
/* Populate Tool List                                                     */
/*------------------------------------------------------------------------*/
static void populate_tool_list(struct iTidyToolCacheWindow *tool_data)
{
    if (tool_data == NULL || tool_data->window == NULL)
        return;
    
    /* Detach list from gadget (mandatory for safe update) */
    GT_SetGadgetAttrs(tool_data->tool_list, tool_data->window, NULL,
        GTLV_Labels, ~0,
        TAG_END);
    
    /* Attach filtered list */
    GT_SetGadgetAttrs(tool_data->tool_list, tool_data->window, NULL,
        GTLV_Labels, &tool_data->filtered_entries,
        GTLV_Selected, ~0,  /* Clear selection */
        TAG_END);
    
    /* Refresh window */
    GT_RefreshWindow(tool_data->window, NULL);
}

/*------------------------------------------------------------------------*/
/* Populate Details Panel                                                 */
/*------------------------------------------------------------------------*/
static void populate_details_panel(struct iTidyToolCacheWindow *tool_data)
{
    if (tool_data == NULL || tool_data->window == NULL || tool_data->details_listview == NULL)
    {
        log_info(LOG_GUI, "[TOOL_CACHE] populate_details_panel() FAILED: NULL pointer (tool_data=%p, window=%p, details_listview=%p)\n",
                     (void*)tool_data, 
                     (void*)(tool_data ? tool_data->window : NULL),
                     (void*)(tool_data ? tool_data->details_listview : NULL));
        return;
    }
    
    log_info(LOG_GUI, "[TOOL_CACHE] populate_details_panel() called - detaching list\n");
    
    /* Detach list from gadget (mandatory for safe update) */
    GT_SetGadgetAttrs(tool_data->details_listview, tool_data->window, NULL,
        GTLV_Labels, ~0,
        TAG_END);
    
    log_info(LOG_GUI, "[TOOL_CACHE] populate_details_panel() - attaching new list\n");
    
    /* Attach details list */
    GT_SetGadgetAttrs(tool_data->details_listview, tool_data->window, NULL,
        GTLV_Labels, &tool_data->details_list,
        GTLV_Top, 0,  /* Scroll to top */
        TAG_END);
    
    log_info(LOG_GUI, "[TOOL_CACHE] populate_details_panel() - refreshing window\n");
    
    /* Refresh just the details listview gadget, not the entire window */
    RefreshGList(tool_data->details_listview, tool_data->window, NULL, 1);
    
    log_info(LOG_GUI, "[TOOL_CACHE] populate_details_panel() COMPLETE\n");
}

/*------------------------------------------------------------------------*/
/* Open Tool Cache Window                                                 */
/*------------------------------------------------------------------------*/
BOOL open_tool_cache_window(struct iTidyToolCacheWindow *tool_data)
{
    struct NewGadget ng;
    struct Gadget *gad;
    struct DrawInfo *draw_info = NULL;
    struct TextFont *font;
    struct RastPort temp_rp;
    UWORD font_width, font_height;
    UWORD button_height, listview_line_height;
    UWORD current_x, current_y;
    UWORD window_width, window_height;
    UWORD listview_width, listview_height, actual_listview_height;
    UWORD details_listview_height;
    UWORD button_width, equal_button_width;
    UWORD reference_width, precalc_max_right;
    UWORD max_btn_text_width, temp_width;
    BOOL using_system_font = FALSE;
    
    if (tool_data == NULL)
        return FALSE;
    
    /* Initialize structure */
    memset(tool_data, 0, sizeof(struct iTidyToolCacheWindow));
    NewList(&tool_data->tool_entries);
    NewList(&tool_data->filtered_entries);
    NewList(&tool_data->details_list);
    tool_data->current_filter = TOOL_FILTER_ALL;
    tool_data->selected_index = -1;
    tool_data->selected_details_index = -1;
    
    /* Lock Workbench screen */
    tool_data->screen = LockPubScreen(NULL);
    if (tool_data->screen == NULL)
    {
        append_to_log("ERROR: Could not lock Workbench screen\n");
        return FALSE;
    }
    
    /* Get DrawInfo for fonts and pens */
    draw_info = GetScreenDrawInfo(tool_data->screen);
    if (draw_info == NULL)
    {
        append_to_log("ERROR: Could not get DrawInfo\n");
        UnlockPubScreen(NULL, tool_data->screen);
        return FALSE;
    }
    
    font = draw_info->dri_Font;
    font_width = font->tf_XSize;
    font_height = font->tf_YSize;
    
    /* Check if screen font is proportional - use system default if so */
    if (font->tf_Flags & FPF_PROPORTIONAL)
    {
        append_to_log("WARNING: Screen uses proportional font - using system default for columns\n");
        
        /* Open system default text font */
        tool_data->system_font_attr.ta_Name = (STRPTR)GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name;
        tool_data->system_font_attr.ta_YSize = GfxBase->DefaultFont->tf_YSize;
        tool_data->system_font_attr.ta_Style = FS_NORMAL;
        tool_data->system_font_attr.ta_Flags = 0;
        
        tool_data->system_font = OpenFont(&tool_data->system_font_attr);
        if (tool_data->system_font != NULL)
        {
            font = tool_data->system_font;
            font_width = font->tf_XSize;
            font_height = font->tf_YSize;
            using_system_font = TRUE;
            append_to_log("Using system font: %s %d\n", 
                tool_data->system_font_attr.ta_Name,
                tool_data->system_font_attr.ta_YSize);
        }
        else
        {
            append_to_log("WARNING: Could not open system font, using screen font\n");
        }
    }
    
    /* Initialize temp RastPort for TextLength measurements */
    InitRastPort(&temp_rp);
    SetFont(&temp_rp, font);
    
    /* Calculate gadget heights */
    button_height = font_height + 6;
    listview_line_height = font_height + 2;
    
    /*--------------------------------------------------------------------*/
    /* PRE-CALCULATE LAYOUT DIMENSIONS                                   */
    /*--------------------------------------------------------------------*/
    reference_width = font_width * TOOL_WINDOW_WIDTH_CHARS;
    
    current_x = TOOL_WINDOW_MARGIN_LEFT + prefsIControl.currentLeftBarWidth;
    current_y = TOOL_WINDOW_MARGIN_TOP + prefsIControl.currentWindowBarHeight;
    
    precalc_max_right = current_x + reference_width;
    
    /* Calculate listview dimensions */
    listview_width = reference_width;
    listview_height = listview_line_height * 12;  /* Show 12 tools */
    
    /* Calculate details panel height */
    details_listview_height = listview_line_height * 5;  /* 5 detail lines */
    
    /* Pre-calculate equal-width buttons (3 filter buttons + rebuild + close = 5 buttons) */
    UWORD button_count = 5;
    
    /* Find maximum button text width */
    max_btn_text_width = TextLength(&temp_rp, "Show Missing Only", 17);
    temp_width = TextLength(&temp_rp, "Show Valid Only", 15);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    temp_width = TextLength(&temp_rp, "Rebuild Cache", 13);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    temp_width = TextLength(&temp_rp, "Show All", 8);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    temp_width = TextLength(&temp_rp, "Close", 5);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    
    /* Calculate equal button width */
    equal_button_width = (reference_width - ((button_count - 1) * TOOL_WINDOW_SPACE_X)) / button_count;
    if (equal_button_width < max_btn_text_width + TOOL_WINDOW_BUTTON_PADDING)
        equal_button_width = max_btn_text_width + TOOL_WINDOW_BUTTON_PADDING;
    
    append_to_log("=== PRE-CALCULATED LAYOUT ===\n");
    append_to_log("Reference width: %d, max_right: %d\n", reference_width, precalc_max_right);
    append_to_log("Equal button width: %d\n", equal_button_width);
    
    /* Get visual info */
    tool_data->visual_info = GetVisualInfo(tool_data->screen, TAG_END);
    if (tool_data->visual_info == NULL)
    {
        append_to_log("ERROR: Could not get visual info\n");
        if (using_system_font && tool_data->system_font != NULL)
            CloseFont(tool_data->system_font);
        FreeScreenDrawInfo(tool_data->screen, draw_info);
        UnlockPubScreen(NULL, tool_data->screen);
        return FALSE;
    }
    
    /* Create gadgets */
    gad = CreateContext(&tool_data->glist);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create gadget context\n");
        if (using_system_font && tool_data->system_font != NULL)
            CloseFont(tool_data->system_font);
        FreeVisualInfo(tool_data->visual_info);
        FreeScreenDrawInfo(tool_data->screen, draw_info);
        UnlockPubScreen(NULL, tool_data->screen);
        return FALSE;
    }
    
    /* Set text attr for all gadgets if using system font */
    ng.ng_TextAttr = using_system_font ? &tool_data->system_font_attr : tool_data->screen->Font;
    ng.ng_VisualInfo = tool_data->visual_info;
    
    /*--------------------------------------------------------------------*/
    /* MAIN TOOL LISTVIEW                                                */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = listview_width;
    ng.ng_Height = listview_height;
    ng.ng_GadgetText = "";  /* Empty label - we'll draw summary separately */
    ng.ng_GadgetID = GID_TOOL_LIST;
    ng.ng_Flags = PLACETEXT_ABOVE;
    
    tool_data->tool_list = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, NULL,  /* Will populate after building list */
        GTLV_ShowSelected, NULL,
        TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create tool listview\n");
        goto cleanup_error;
    }
    
    /* Get actual listview height */
    actual_listview_height = gad->Height;
    current_y = gad->TopEdge + actual_listview_height + TOOL_WINDOW_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* REPLACE TOOL (BATCH) BUTTON - Under upper listview                */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width * 2;  /* Make it wider */
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Replace Tool (Batch)";
    ng.ng_GadgetID = GID_TOOL_REPLACE_BATCH;
    ng.ng_Flags = PLACETEXT_IN;
    
    tool_data->replace_batch_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Replace Tool (Batch) button\n");
        goto cleanup_error;
    }
    
    /*--------------------------------------------------------------------*/
    /* RESTORE DEFAULT TOOLS BUTTON - Next to Replace Tool (Batch)       */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x + (equal_button_width * 2) + TOOL_WINDOW_SPACE_X;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width * 2;  /* Make it wider */
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Restore Default Tools...";
    ng.ng_GadgetID = GID_TOOL_RESTORE_DEFAULT_TOOLS;
    ng.ng_Flags = PLACETEXT_IN;
    
    tool_data->restore_default_tools_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Restore Default Tools button\n");
        goto cleanup_error;
    }
    
    current_y = gad->TopEdge + gad->Height + TOOL_WINDOW_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* DETAILS PANEL LISTVIEW                                            */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = listview_width;
    ng.ng_Height = details_listview_height;
    ng.ng_GadgetText = "";
    ng.ng_GadgetID = GID_TOOL_DETAILS_LIST;
    ng.ng_Flags = PLACETEXT_ABOVE;
    
    tool_data->details_listview = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, &tool_data->details_list,
        GTLV_ShowSelected, NULL,
        TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create details listview\n");
        goto cleanup_error;
    }
    
    current_y = gad->TopEdge + gad->Height + TOOL_WINDOW_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* REPLACE TOOL (SINGLE) BUTTON - Under details panel                */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width * 2;  /* Make it wider */
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Replace Tool (Single)";
    ng.ng_GadgetID = GID_TOOL_REPLACE_SINGLE;
    ng.ng_Flags = PLACETEXT_IN;
    
    tool_data->replace_single_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Replace Tool (Single) button\n");
        goto cleanup_error;
    }
    
    current_y = gad->TopEdge + gad->Height + TOOL_WINDOW_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* FILTER AND CLOSE BUTTONS (4 equal-width buttons)                  */
    /*--------------------------------------------------------------------*/
    
    /* Show All button */
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Show All";
    ng.ng_GadgetID = GID_TOOL_FILTER_ALL;
    ng.ng_Flags = PLACETEXT_IN;
    
    tool_data->filter_all_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Show All button\n");
        goto cleanup_error;
    }
    
    /* Show Valid Only button */
    ng.ng_LeftEdge = current_x + equal_button_width + TOOL_WINDOW_SPACE_X;
    ng.ng_Width = equal_button_width;
    ng.ng_GadgetText = "Show Valid Only";
    ng.ng_GadgetID = GID_TOOL_FILTER_VALID;
    
    tool_data->filter_valid_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Show Valid button\n");
        goto cleanup_error;
    }
    
    /* Show Missing Only button */
    ng.ng_LeftEdge = current_x + (2 * equal_button_width) + (2 * TOOL_WINDOW_SPACE_X);
    ng.ng_Width = equal_button_width;
    ng.ng_GadgetText = "Show Missing Only";
    ng.ng_GadgetID = GID_TOOL_FILTER_MISSING;
    
    tool_data->filter_missing_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Show Missing button\n");
        goto cleanup_error;
    }
    
    /* Rebuild Cache button */
    ng.ng_LeftEdge = current_x + (3 * equal_button_width) + (3 * TOOL_WINDOW_SPACE_X);
    ng.ng_Width = equal_button_width;
    ng.ng_GadgetText = "Rebuild Cache";
    ng.ng_GadgetID = GID_TOOL_REBUILD_CACHE;
    
    tool_data->rebuild_cache_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Rebuild Cache button\n");
        goto cleanup_error;
    }
    
    /* Close button */
    ng.ng_LeftEdge = current_x + (4 * equal_button_width) + (4 * TOOL_WINDOW_SPACE_X);
    ng.ng_Width = equal_button_width;
    ng.ng_GadgetText = "Close";
    ng.ng_GadgetID = GID_TOOL_CACHE_CLOSE;
    
    tool_data->close_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Close button\n");
        goto cleanup_error;
    }
    
    /* Calculate final window size */
    window_width = precalc_max_right + TOOL_WINDOW_MARGIN_RIGHT;
    window_height = current_y + button_height + TOOL_WINDOW_MARGIN_BOTTOM;
    
    /* Set window title */
    strcpy(tool_data->window_title, "iTidy - Default Tool Analysis");
    
    /* Setup menu system BEFORE opening window */
    if (!setup_tool_cache_menus())
    {
        log_error(LOG_GUI, "ERROR: Could not setup menus\n");
        goto cleanup_error;
    }
    
    /* Open window */
    tool_data->window = OpenWindowTags(NULL,
        WA_Left, (tool_data->screen->Width - window_width) / 2,
        WA_Top, (tool_data->screen->Height - window_height) / 2,
        WA_Width, window_width,
        WA_Height, window_height,
        WA_Title, tool_data->window_title,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, tool_data->screen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_MENUPICK,
        WA_Gadgets, tool_data->glist,
        WA_NewLookMenus, TRUE,  /* Enable NewLook menus */
        TAG_END);
    
    if (tool_data->window == NULL)
    {
        append_to_log("ERROR: Could not open tool cache window\n");
        goto cleanup_error;
    }
    
    /* Attach menu strip to window */
    if (menu_strip)
    {
        SetMenuStrip(tool_data->window, menu_strip);
        log_info(LOG_GUI, "Menu strip attached to tool cache window\n");
    }
    
    /* Free DrawInfo (no longer needed) */
    FreeScreenDrawInfo(tool_data->screen, draw_info);
    draw_info = NULL;
    
    /* Build display list from global cache */
    if (!build_tool_cache_display_list(tool_data))
    {
        append_to_log("ERROR: Failed to build tool cache display list\n");
        close_tool_cache_window(tool_data);
        return FALSE;
    }
    
    /* Apply initial filter (show all) */
    apply_tool_filter(tool_data);
    
    /* Populate listview */
    populate_tool_list(tool_data);
    
    tool_data->window_open = TRUE;
    append_to_log("Tool cache window opened successfully\n");
    
    return TRUE;
    
cleanup_error:
    if (draw_info != NULL)
        FreeScreenDrawInfo(tool_data->screen, draw_info);
    if (tool_data->glist != NULL)
        FreeGadgets(tool_data->glist);
    if (tool_data->visual_info != NULL)
        FreeVisualInfo(tool_data->visual_info);
    if (using_system_font && tool_data->system_font != NULL)
        CloseFont(tool_data->system_font);
    if (tool_data->screen != NULL)
        UnlockPubScreen(NULL, tool_data->screen);
    
    return FALSE;
}

/*------------------------------------------------------------------------*/
/* Close Tool Cache Window                                                */
/*------------------------------------------------------------------------*/
void close_tool_cache_window(struct iTidyToolCacheWindow *tool_data)
{
    if (tool_data == NULL)
        return;
    
    /* Detach lists from listviews FIRST (critical for safe cleanup) */
    if (tool_data->window != NULL)
    {
        /* Clear menu strip before closing window */
        if (menu_strip)
        {
            ClearMenuStrip(tool_data->window);
            log_info(LOG_GUI, "Menu strip cleared from tool cache window\n");
        }
        
        if (tool_data->tool_list != NULL)
        {
            GT_SetGadgetAttrs(tool_data->tool_list, tool_data->window, NULL,
                GTLV_Labels, ~0,
                TAG_END);
        }
    }
    
    /* Free lists */
    free_tool_cache_entries(tool_data);
    
    /* Close window */
    if (tool_data->window != NULL)
    {
        CloseWindow(tool_data->window);
        tool_data->window = NULL;
    }
    
    /* Free gadgets */
    if (tool_data->glist != NULL)
    {
        FreeGadgets(tool_data->glist);
        tool_data->glist = NULL;
    }
    
    /* Free visual info */
    if (tool_data->visual_info != NULL)
    {
        FreeVisualInfo(tool_data->visual_info);
        tool_data->visual_info = NULL;
    }
    
    /* Close system font */
    if (tool_data->system_font != NULL)
    {
        CloseFont(tool_data->system_font);
        tool_data->system_font = NULL;
    }
    
    /* Unlock screen */
    if (tool_data->screen != NULL)
    {
        UnlockPubScreen(NULL, tool_data->screen);
        tool_data->screen = NULL;
    }
    
    /* Clean up menu system */
    cleanup_tool_cache_menus();
    
    tool_data->window_open = FALSE;
    append_to_log("Tool cache window closed\n");
}

/*------------------------------------------------------------------------*/
/* Handle Tool Cache Window Events                                        */
/*------------------------------------------------------------------------*/
BOOL handle_tool_cache_window_events(struct iTidyToolCacheWindow *tool_data)
{
    struct IntuiMessage *msg;
    ULONG msg_class;
    UWORD msg_code;
    UWORD gadget_id;
    struct Gadget *gadget;
    
    if (tool_data == NULL || tool_data->window == NULL)
        return FALSE;
    
    while ((msg = GT_GetIMsg(tool_data->window->UserPort)) != NULL)
    {
        msg_class = msg->Class;
        msg_code = msg->Code;
        gadget = (struct Gadget *)msg->IAddress;
        gadget_id = (gadget != NULL) ? gadget->GadgetID : 0;
        
        GT_ReplyIMsg(msg);
        
        switch (msg_class)
        {
            case IDCMP_CLOSEWINDOW:
                return FALSE;  /* Close window */
                
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(tool_data->window);
                GT_EndRefresh(tool_data->window, TRUE);
                break;
                
            case IDCMP_GADGETUP:
                switch (gadget_id)
                {
                    case GID_TOOL_LIST:
                    {
                        /* Tool selected - update details */
                        LONG selected = ~0;
                        GT_GetGadgetAttrs(tool_data->tool_list, tool_data->window, NULL,
                            GTLV_Selected, &selected,
                            TAG_END);
                        log_info(LOG_GUI, "[TOOL_CACHE] Tool selected: index=%ld\n", selected);
                        update_tool_details(tool_data, selected);
                        break;
                    }
                    
                    case GID_TOOL_DETAILS_LIST:
                    {
                        /* Details list item selected */
                        LONG selected = ~0;
                        GT_GetGadgetAttrs(tool_data->details_listview, tool_data->window, NULL,
                            GTLV_Selected, &selected,
                            TAG_END);
                        tool_data->selected_details_index = selected;
                        log_info(LOG_GUI, "[TOOL_CACHE] Details item selected: index=%ld\n", selected);
                        break;
                    }
                    
                    case GID_TOOL_FILTER_ALL:
                        tool_data->current_filter = TOOL_FILTER_ALL;
                        apply_tool_filter(tool_data);
                        populate_tool_list(tool_data);
                        tool_data->selected_index = -1;
                        update_tool_details(tool_data, -1);
                        break;
                        
                    case GID_TOOL_FILTER_VALID:
                        tool_data->current_filter = TOOL_FILTER_VALID;
                        apply_tool_filter(tool_data);
                        populate_tool_list(tool_data);
                        tool_data->selected_index = -1;
                        update_tool_details(tool_data, -1);
                        break;
                        
                    case GID_TOOL_FILTER_MISSING:
                        tool_data->current_filter = TOOL_FILTER_MISSING;
                        apply_tool_filter(tool_data);
                        populate_tool_list(tool_data);
                        tool_data->selected_index = -1;
                        update_tool_details(tool_data, -1);
                        break;
                    
                    case GID_TOOL_REBUILD_CACHE:
                        log_info(LOG_GUI, "[TOOL_CACHE] Rebuild Cache button clicked\n");
                        
                        /* Use global preferences for scan path and recursive mode */
                        log_info(LOG_GUI, "Rescanning using global preferences\n");
                        
                        /* Rescan the directory to rebuild tool cache */
                        if (ScanDirectoryForToolsOnly())
                        {
                            log_info(LOG_GUI, "Tool cache rebuilt successfully\n");
                            
                            /* Rebuild display list from refreshed cache */
                            build_tool_cache_display_list(tool_data);
                            apply_tool_filter(tool_data);
                            populate_tool_list(tool_data);
                            
                            /* Clear selection and details */
                            tool_data->selected_index = -1;
                            update_tool_details(tool_data, -1);
                            
                            log_info(LOG_GUI, "Display updated with new cache data\n");
                        }
                        else
                        {
                            log_error(LOG_GUI, "Failed to rebuild tool cache\n");
                            /* Could show an error requester here */
                        }
                        break;
                        
                    case GID_TOOL_REPLACE_BATCH:
                    {
                        struct ToolCacheDisplayEntry *entry;
                        struct Node *node;
                        LONG index = 0;
                        int i, j;
                        struct iTidy_DefaultToolUpdateWindow *update_window;
                        struct iTidy_DefaultToolUpdateContext update_ctx;
                        char **icon_paths_array = NULL;
                        
                        log_info(LOG_GUI, "[TOOL_CACHE] Replace Tool (Batch) button clicked\n");
                        
                        /* Check if a tool is selected */
                        if (tool_data->selected_index < 0)
                        {
                            log_info(LOG_GUI, "[TOOL_CACHE] No tool selected\n");
                            ShowEasyRequest(tool_data->window,
                                "No Tool Selected",
                                "Please select a tool from the list first.",
                                "OK");
                            break;
                        }
                        
                        /* Allocate window structure on HEAP to avoid stack overflow */
                        update_window = (struct iTidy_DefaultToolUpdateWindow *)whd_malloc(sizeof(struct iTidy_DefaultToolUpdateWindow));
                        if (update_window == NULL)
                        {
                            log_error(LOG_GUI, "[TOOL_CACHE] Failed to allocate update window structure\n");
                            ShowEasyRequest(tool_data->window,
                                "Memory Error",
                                "Could not allocate memory for update window.",
                                "OK");
                            break;
                        }
                        
                        /* Find selected entry in filtered list */
                        for (node = tool_data->filtered_entries.lh_Head; 
                             node->ln_Succ != NULL; 
                             node = node->ln_Succ, index++)
                        {
                            if (index == tool_data->selected_index)
                            {
                                /* Calculate entry pointer from filter_node offset */
                                entry = (struct ToolCacheDisplayEntry *)((char *)node - sizeof(struct Node));
                                
                                /* Validate this is not a header row */
                                if (entry->tool_name == NULL)
                                {
                                    log_info(LOG_GUI, "[TOOL_CACHE] Header row selected, ignoring\n");
                                    ShowEasyRequest(tool_data->window,
                                        "Invalid Selection",
                                        "Please select a tool entry, not the header.",
                                        "OK");
                                    FreeVec(update_window);
                                    break;
                                }
                                
                                /* Find the corresponding cache entry */
                                for (i = 0; i < g_ToolCacheCount; i++)
                                {
                                    if (g_ToolCache[i].toolName && 
                                        strcmp(g_ToolCache[i].toolName, entry->tool_name) == 0)
                                    {
                                        /* Allocate icon paths array */
                                        icon_paths_array = (char **)whd_malloc(g_ToolCache[i].fileCount * sizeof(char *));
                                        if (icon_paths_array == NULL)
                                        {
                                            log_error(LOG_GUI, "[TOOL_CACHE] Failed to allocate icon paths array\n");
                                            FreeVec(update_window);
                                            break;
                                        }
                                        
                                        /* Copy icon path pointers (they're already allocated in g_ToolCache) */
                                        for (j = 0; j < g_ToolCache[i].fileCount; j++)
                                        {
                                            icon_paths_array[j] = g_ToolCache[i].referencingFiles[j];
                                        }
                                        
                                        /* Populate context for batch mode */
                                        memset(&update_ctx, 0, sizeof(update_ctx));
                                        update_ctx.mode = UPDATE_MODE_BATCH;
                                        update_ctx.current_tool = g_ToolCache[i].toolName;
                                        update_ctx.icon_count = g_ToolCache[i].fileCount;
                                        update_ctx.icon_paths = icon_paths_array;
                                        
                                        log_info(LOG_GUI, "[TOOL_CACHE] Opening batch update window for %d icons\n",
                                                     update_ctx.icon_count);
                                        
                                        /* Initialize window data structure */
                                        memset(update_window, 0, sizeof(struct iTidy_DefaultToolUpdateWindow));
                                        
                                        /* Open the update window */
                                        if (iTidy_OpenDefaultToolUpdateWindow(update_window, &update_ctx))
                                        {
                                            /* Run the event loop */
                                            while (iTidy_HandleDefaultToolUpdateEvents(update_window))
                                            {
                                                /* Keep processing events */
                                            }
                                            
                                            /* Close the window */
                                            iTidy_CloseDefaultToolUpdateWindow(update_window);
                                            
                                            log_info(LOG_GUI, "[TOOL_CACHE] Batch update completed\n");
                                        }
                                        else
                                        {
                                            log_error(LOG_GUI, "[TOOL_CACHE] Failed to open batch update window\n");
                                        }
                                        
                                        /* Free the window structure */
                                        FreeVec(update_window);
                                        
                                        /* Free the icon paths array (but not the strings themselves) */
                                        if (icon_paths_array != NULL)
                                            FreeVec(icon_paths_array);
                                        
                                        break;
                                    }
                                }
                                
                                break;
                            }
                        }
                        break;
                    }
                    
                    case GID_TOOL_REPLACE_SINGLE:
                    {
                        struct ToolCacheDisplayEntry *entry;
                        struct Node *node;
                        LONG index = 0;
                        int i, file_index;
                        struct iTidy_DefaultToolUpdateWindow *update_window;
                        struct iTidy_DefaultToolUpdateContext update_ctx;
                        
                        log_info(LOG_GUI, "[TOOL_CACHE] Replace Tool (Single) button clicked\n");
                        
                        /* Check if a tool is selected */
                        if (tool_data->selected_index < 0)
                        {
                            log_info(LOG_GUI, "[TOOL_CACHE] No tool selected\n");
                            ShowEasyRequest(tool_data->window,
                                "No Tool Selected",
                                "Please select a tool from the upper list first.",
                                "OK");
                            break;
                        }
                        
                        /* Check if a specific file is selected in details */
                        if (tool_data->selected_details_index < 0)
                        {
                            log_info(LOG_GUI, "[TOOL_CACHE] No file selected in details\n");
                            ShowEasyRequest(tool_data->window,
                                "No File Selected",
                                "Please select a specific icon file from the lower list.",
                                "OK");
                            break;
                        }
                        
                        /* Allocate window structure on HEAP to avoid stack overflow */
                        update_window = (struct iTidy_DefaultToolUpdateWindow *)whd_malloc(sizeof(struct iTidy_DefaultToolUpdateWindow));
                        if (update_window == NULL)
                        {
                            log_error(LOG_GUI, "[TOOL_CACHE] Failed to allocate update window structure\n");
                            ShowEasyRequest(tool_data->window,
                                "Memory Error",
                                "Could not allocate memory for update window.",
                                "OK");
                            break;
                        }
                        if (tool_data->selected_details_index < 0)
                        {
                            log_info(LOG_GUI, "[TOOL_CACHE] No file selected in details\n");
                            ShowEasyRequest(tool_data->window,
                                "No File Selected",
                                "Please select a specific icon file from the lower list.",
                                "OK");
                            break;
                        }
                        
                        /* Find selected entry in filtered list */
                        for (node = tool_data->filtered_entries.lh_Head; 
                             node->ln_Succ != NULL; 
                             node = node->ln_Succ, index++)
                        {
                            if (index == tool_data->selected_index)
                            {
                                /* Calculate entry pointer from filter_node offset */
                                entry = (struct ToolCacheDisplayEntry *)((char *)node - sizeof(struct Node));
                                
                                /* Validate this is not a header row */
                                if (entry->tool_name == NULL)
                                {
                                    log_info(LOG_GUI, "[TOOL_CACHE] Header row selected, ignoring\n");
                                    ShowEasyRequest(tool_data->window,
                                        "Invalid Selection",
                                        "Please select a tool entry, not the header.",
                                        "OK");
                                    FreeVec(update_window);
                                    break;
                                }
                                
                                /* Find the corresponding cache entry */
                                for (i = 0; i < g_ToolCacheCount; i++)
                                {
                                    if (g_ToolCache[i].toolName && 
                                        strcmp(g_ToolCache[i].toolName, entry->tool_name) == 0)
                                    {
                                        /* Check if there is at least one file */
                                        if (g_ToolCache[i].fileCount < 1)
                                        {
                                            log_info(LOG_GUI, "[TOOL_CACHE] No files using this tool\n");
                                            ShowEasyRequest(tool_data->window,
                                                "No Files Found",
                                                "This tool has no associated icon files.",
                                                "OK");
                                            FreeVec(update_window);
                                            break;
                                        }
                                        
                                        /* Calculate file index from selected details index */
                                        /* Details list has: Tool name (line 0), Status/Version (line 1), Separator (line 2), Files (3+) */
                                        file_index = tool_data->selected_details_index - 3;
                                        
                                        /* Validate file index */
                                        if (file_index < 0 || file_index >= g_ToolCache[i].fileCount)
                                        {
                                            log_info(LOG_GUI, "[TOOL_CACHE] Invalid file selection (selected header/separator)\n");
                                            ShowEasyRequest(tool_data->window,
                                                "Invalid Selection",
                                                "Please select a file entry (not the header or separator).",
                                                "OK");
                                            FreeVec(update_window);
                                            break;
                                        }
                                        
                                        /* Populate context for single mode using selected file */
                                        memset(&update_ctx, 0, sizeof(update_ctx));
                                        update_ctx.mode = UPDATE_MODE_SINGLE;
                                        update_ctx.current_tool = g_ToolCache[i].toolName;
                                        update_ctx.single_info_path = g_ToolCache[i].referencingFiles[file_index];
                                        
                                        log_info(LOG_GUI, "[TOOL_CACHE] Opening single update window for file [%d]: %s\n",
                                                     file_index, update_ctx.single_info_path);
                                        
                                        /* Initialize window data structure */
                                        memset(update_window, 0, sizeof(struct iTidy_DefaultToolUpdateWindow));
                                        
                                        /* Open the update window */
                                        if (iTidy_OpenDefaultToolUpdateWindow(update_window, &update_ctx))
                                        {
                                            /* Run the event loop */
                                            while (iTidy_HandleDefaultToolUpdateEvents(update_window))
                                            {
                                                /* Keep processing events */
                                            }
                                            
                                            /* Close the window */
                                            iTidy_CloseDefaultToolUpdateWindow(update_window);
                                            
                                            log_info(LOG_GUI, "[TOOL_CACHE] Single update completed\n");
                                        }
                                        else
                                        {
                                            log_error(LOG_GUI, "[TOOL_CACHE] Failed to open single update window\n");
                                        }
                                        
                                        /* Free the window structure */
                                        FreeVec(update_window);
                                        
                                        break;
                                    }
                                }
                                
                                break;
                            }
                        }
                        break;
                    }
                        
                    case GID_TOOL_RESTORE_DEFAULT_TOOLS:
                        {
                            struct Window *restore_window;
                            struct IntuiMessage *restore_msg;
                            BOOL keep_running;
                            iTidy_ToolBackupManager temp_manager;
                            
                            log_info(LOG_GUI, "Restore Default Tools button clicked from Tool Cache window\n");
                            
                            /* Initialize temporary backup manager for restore operations */
                            if (!iTidy_InitToolBackupManager(&temp_manager, FALSE))
                            {
                                log_error(LOG_GUI, "Failed to initialize backup manager\n");
                                ShowEasyRequest(
                                    tool_data->window,
                                    "Error",
                                    "Failed to initialize backup system.",
                                    "OK");
                                break;
                            }
                            
                            /* Set busy pointer */
                            SetWindowPointer(tool_data->window, WA_BusyPointer, TRUE, TAG_END);
                            
                            /* Create restore window */
                            restore_window = iTidy_CreateToolRestoreWindow(
                                tool_data->window->WScreen,
                                &temp_manager);
                            
                            if (!restore_window)
                            {
                                /* Clear busy pointer */
                                SetWindowPointer(tool_data->window, WA_Pointer, NULL, TAG_END);
                                
                                iTidy_CleanupToolBackupManager(&temp_manager);
                                
                                log_error(LOG_GUI, "Failed to create restore window\n");
                                ShowEasyRequest(
                                    tool_data->window,
                                    "Window Error",
                                    "Failed to create restore window.",
                                    "OK");
                                break;
                            }
                            
                            /* Clear busy pointer - window is now open */
                            SetWindowPointer(tool_data->window, WA_Pointer, NULL, TAG_END);
                            
                            /* Disable tool cache window input while restore window is open */
                            ModifyIDCMP(tool_data->window, 0);
                            
                            /* Run restore window event loop */
                            keep_running = TRUE;
                            while (keep_running)
                            {
                                WaitPort(restore_window->UserPort);
                                
                                while ((restore_msg = GT_GetIMsg(restore_window->UserPort)))
                                {
                                    keep_running = iTidy_HandleToolRestoreWindowEvent(
                                        restore_window, restore_msg);
                                    
                                    GT_ReplyIMsg(restore_msg);
                                    
                                    if (!keep_running)
                                        break;
                                }
                            }
                            
                            /* Close restore window */
                            iTidy_CloseToolRestoreWindow(restore_window);
                            
                            /* Cleanup backup manager */
                            iTidy_CleanupToolBackupManager(&temp_manager);
                            
                            /* Re-enable tool cache window input */
                            ModifyIDCMP(tool_data->window, 
                                IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_REFRESHWINDOW);
                            
                            log_info(LOG_GUI, "Restore Default Tools window closed\n");
                        }
                        break;
                        
                    case GID_TOOL_CACHE_CLOSE:
                        return FALSE;  /* Close window */
                }
                break;
                
            case IDCMP_GADGETDOWN:
                /* Handle listview scroll buttons */
                if (gadget_id == GID_TOOL_LIST)
                {
                    GT_RefreshWindow(tool_data->window, NULL);
                }
                break;
                
            case IDCMP_MENUPICK:
                /* Handle menu selections */
                if (!handle_tool_cache_menu_selection(msg_code, tool_data))
                {
                    return FALSE;  /* Menu requested window close */
                }
                break;
        }
    }
    
    return TRUE;  /* Continue running */
}

