/*
 * tool_cache_window.c - iTidy Tool Cache Window Implementation (ReAction)
 * Displays default tool validation cache with filtering and details
 * ReAction-based window for Workbench 3.2+
 * 
 * Migrated from GadTools to ReAction following REACTION_MIGRATION_GUIDE.md
 * Uses ListBrowser gadgets with real columns (replacing "fake columns" approach).
 */

/* =========================================================================
 * LIBRARY BASE ISOLATION
 * Redefine library bases to local unique names BEFORE including proto headers.
 * This prevents linker collisions with bases defined in main_window.c
 * ========================================================================= */
#define WindowBase      iTidy_ToolCache_WindowBase
#define LayoutBase      iTidy_ToolCache_LayoutBase
#define ButtonBase      iTidy_ToolCache_ButtonBase
#define ListBrowserBase iTidy_ToolCache_ListBrowserBase
#define LabelBase       iTidy_ToolCache_LabelBase
#define ChooserBase     iTidy_ToolCache_ChooserBase
#define GetFileBase     iTidy_ToolCache_GetFileBase
#define RequesterBase   iTidy_ToolCache_RequesterBase

#include "platform/platform.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <libraries/asl.h>
#include <libraries/dos.h>
#include <libraries/gadtools.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/asl.h>
#include <proto/gadtools.h>
#include <string.h>
#include <stdio.h>

/* ReAction headers - MUST come before exec_list_compat.h to avoid macro conflict */
#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

/* Project headers (include after system headers) */
#include "tool_cache_window.h"
#include "tool_cache_reports.h"
#include "default_tool_update_window.h"
#include "GUI/RestoreBackups/restore_window.h"
#include "default_tool_backup.h"
#include "../easy_request_helper.h"
#include "GUI/StatusWindows/main_progress_window.h"
#include "GUI/gui_utilities.h"
#include "../../helpers/exec_list_compat.h"
#include "icon_types.h"
#include "itidy_types.h"
#include "Settings/IControlPrefs.h"
#include "writeLog.h"
#include "layout_processor.h"
#include "path_utilities.h"
#include "utilities.h"
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/listbrowser.h>
#include <proto/label.h>
#include <proto/chooser.h>
#include <proto/getfile.h>
#include <proto/requester.h>
#include <classes/window.h>
#include <classes/requester.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/listbrowser.h>
#include <gadgets/chooser.h>
#include <gadgets/getfile.h>
#include <images/label.h>

/*------------------------------------------------------------------------*/
/* Library Bases (Prefixed to avoid collision with main_window.c)        */
/*------------------------------------------------------------------------*/
struct Library *iTidy_ToolCache_WindowBase = NULL;
struct Library *iTidy_ToolCache_LayoutBase = NULL;
struct Library *iTidy_ToolCache_ButtonBase = NULL;
struct Library *iTidy_ToolCache_ListBrowserBase = NULL;
struct Library *iTidy_ToolCache_LabelBase = NULL;
struct Library *iTidy_ToolCache_ChooserBase = NULL;
struct Library *iTidy_ToolCache_GetFileBase = NULL;
struct Library *iTidy_ToolCache_RequesterBase = NULL;

/*------------------------------------------------------------------------*/
/* Menu IDs                                                               */
/*------------------------------------------------------------------------*/
#define MENU_PROJECT_NEW        5001
#define MENU_PROJECT_OPEN       5002
#define MENU_PROJECT_SAVE       5003
#define MENU_PROJECT_SAVE_AS    5004
#define MENU_PROJECT_CLOSE      5005

#define MENU_FILE_EXPORT_TOOLS      5010
#define MENU_FILE_EXPORT_FILES      5011

#define MENU_VIEW_SYSTEM_PATH       5020

/*------------------------------------------------------------------------*/
/* Menu System Global Variables                                          */
/*------------------------------------------------------------------------*/
static struct Screen *wb_screen_menu = NULL;
static struct DrawInfo *draw_info_menu = NULL;
static APTR visual_info_menu = NULL;
static struct Menu *menu_strip = NULL;

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
	
	{ NM_TITLE, "File",         NULL, 0, 0, NULL },
	{ NM_ITEM,  "Export list of tools",           "T",  0, 0, (APTR)MENU_FILE_EXPORT_TOOLS },
	{ NM_ITEM,  "Export list of files and tools", "F",  0, 0, (APTR)MENU_FILE_EXPORT_FILES },
	
	{ NM_TITLE, "View",         NULL, 0, 0, NULL },
	{ NM_ITEM,  "System PATH...", "P",  0, 0, (APTR)MENU_VIEW_SYSTEM_PATH },
	
	{ NM_END,   NULL,           NULL, 0, 0, NULL }
};

/*------------------------------------------------------------------------*/
/* Forward declarations for menu functions                               */
/*------------------------------------------------------------------------*/
static BOOL setup_tool_cache_menus(void);
static void cleanup_tool_cache_menus(void);
static BOOL handle_tool_cache_menu_selection(ULONG menu_number, struct iTidyToolCacheWindow *tool_data);
static void handle_new_menu(struct iTidyToolCacheWindow *tool_data);
static void handle_open_menu(struct iTidyToolCacheWindow *tool_data);
static void handle_save_menu(struct iTidyToolCacheWindow *tool_data);
static void handle_save_as_menu(struct iTidyToolCacheWindow *tool_data);
static void handle_export_tools_menu(struct iTidyToolCacheWindow *tool_data);
static void handle_export_files_and_tools_menu(struct iTidyToolCacheWindow *tool_data);
static void handle_view_system_path_menu(struct iTidyToolCacheWindow *tool_data);
static BOOL load_tool_cache_from_file(const char *filepath, char *folder_path_out);

/*------------------------------------------------------------------------*/
/* External Default Tool Restore Functions                               */
/*------------------------------------------------------------------------*/
extern struct Window *iTidy_CreateToolRestoreWindow(struct Screen *screen, APTR backup_manager);
extern BOOL iTidy_HandleToolRestoreWindowEvent(struct Window *window, struct IntuiMessage *msg);
extern BOOL iTidy_WasRestorePerformed(void);
extern void iTidy_CloseToolRestoreWindow(struct Window *window);

/*------------------------------------------------------------------------*/
/* Column Configuration for Tool ListBrowser                             */
/* Replaces the "fake columns" approach with real ListBrowser columns    */
/*------------------------------------------------------------------------*/
static struct ColumnInfo *tool_list_column_info = NULL;  /* Allocated dynamically */

static struct ColumnInfo details_column_info[] = {
    { 100, "File Path", 0 },  /* Full path to referencing file */
    { -1, (STRPTR)~0, -1 }
};

/*------------------------------------------------------------------------*/
/* Initialize Column Info with Sorting and Resizing                      */
/*------------------------------------------------------------------------*/
static BOOL init_column_info(void)
{
    log_debug(LOG_GUI, "*** Allocating and initializing column info ***\n");
    
    /* Allocate column info with 3 columns */
    tool_list_column_info = AllocLBColumnInfo(3,
        LBCIA_Column, 0,
            LBCIA_Title, "Tool",
            LBCIA_Weight, 55,
            LBCIA_Sortable, TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags, CIF_SORTABLE | CIF_DRAGGABLE,
        LBCIA_Column, 1,
            LBCIA_Title, "Files",
            LBCIA_Weight, 12,
            LBCIA_Sortable, TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags, CIF_SORTABLE | CIF_DRAGGABLE,
        LBCIA_Column, 2,
            LBCIA_Title, "Status",
            LBCIA_Weight, 15,
            LBCIA_Sortable, TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags, CIF_SORTABLE | CIF_DRAGGABLE,
        TAG_DONE);
    
    if (tool_list_column_info == NULL)
    {
        log_error(LOG_GUI, "*** ERROR: Failed to allocate column info! ***\n");
        return FALSE;
    }
    
    log_debug(LOG_GUI, "*** Column info allocated successfully ***\n");
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static BOOL open_reaction_classes(void);
static void close_reaction_classes(void);
static void free_listbrowser_list(struct List *list);
static void free_chooser_labels(struct List *list);
static struct List *create_chooser_labels(STRPTR *strings);
static void populate_tool_listbrowser(struct iTidyToolCacheWindow *tool_data);
static void populate_details_listbrowser(struct iTidyToolCacheWindow *tool_data);
static void update_button_states(struct iTidyToolCacheWindow *tool_data);
static BOOL save_tool_cache_to_file(const char *filepath, const char *folder_path);
static BOOL load_tool_cache_from_file(const char *filepath, char *folder_path_out);
static void handle_scan_button(struct iTidyToolCacheWindow *tool_data);
static void handle_replace_batch_button(struct iTidyToolCacheWindow *tool_data);
static void handle_replace_single_button(struct iTidyToolCacheWindow *tool_data);
static void handle_restore_tools_button(struct iTidyToolCacheWindow *tool_data);
static ULONG ShowReActionRequester(struct Window *parent_window,
                                   CONST_STRPTR title,
                                   CONST_STRPTR body,
                                   CONST_STRPTR gadgets,
                                   ULONG image_type);

/*------------------------------------------------------------------------*/
/**
 * @brief Open ReAction class libraries
 */
/*------------------------------------------------------------------------*/
static BOOL open_reaction_classes(void)
{
    if (!WindowBase)      WindowBase = OpenLibrary("window.class", 0);
    if (!LayoutBase)      LayoutBase = OpenLibrary("gadgets/layout.gadget", 0);
    if (!ButtonBase)      ButtonBase = OpenLibrary("gadgets/button.gadget", 0);
    if (!ListBrowserBase) ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 0);
    if (!LabelBase)       LabelBase = OpenLibrary("images/label.image", 0);
    if (!ChooserBase)     ChooserBase = OpenLibrary("gadgets/chooser.gadget", 0);
    if (!GetFileBase)     GetFileBase = OpenLibrary("gadgets/getfile.gadget", 0);
    if (!RequesterBase)   RequesterBase = OpenLibrary("requester.class", 0);
    
    if (!WindowBase || !LayoutBase || !ButtonBase || 
        !ListBrowserBase || !LabelBase || !ChooserBase ||
        !GetFileBase || !RequesterBase)
    {
        log_error(LOG_GUI, "Failed to open ReAction classes for tool cache window\n");
        return FALSE;
    }
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Close ReAction class libraries
 */
/*------------------------------------------------------------------------*/
static void close_reaction_classes(void)
{
    if (RequesterBase)   { CloseLibrary(RequesterBase);   RequesterBase = NULL; }
    if (GetFileBase)     { CloseLibrary(GetFileBase);     GetFileBase = NULL; }
    if (ChooserBase)     { CloseLibrary(ChooserBase);     ChooserBase = NULL; }
    if (LabelBase)       { CloseLibrary(LabelBase);       LabelBase = NULL; }
    if (ListBrowserBase) { CloseLibrary(ListBrowserBase); ListBrowserBase = NULL; }
    if (ButtonBase)      { CloseLibrary(ButtonBase);      ButtonBase = NULL; }
    if (LayoutBase)      { CloseLibrary(LayoutBase);      LayoutBase = NULL; }
    if (WindowBase)      { CloseLibrary(WindowBase);      WindowBase = NULL; }
}

/*------------------------------------------------------------------------*/
/**
 * @brief Free a ListBrowser list and all its nodes
 */
/*------------------------------------------------------------------------*/
static void free_listbrowser_list(struct List *list)
{
    struct Node *node;
    struct Node *next;
    
    if (!list)
        return;
    
    node = list->lh_Head;
    while ((next = node->ln_Succ))
    {
        FreeListBrowserNode(node);
        node = next;
    }
    
    FreeMem(list, sizeof(struct List));
}

/*------------------------------------------------------------------------*/
/**
 * @brief Free a Chooser labels list
 */
/*------------------------------------------------------------------------*/
static void free_chooser_labels(struct List *list)
{
    struct Node *node;
    struct Node *next;
    
    if (!list)
        return;
    
    node = list->lh_Head;
    while ((next = node->ln_Succ))
    {
        FreeChooserNode(node);
        node = next;
    }
    
    FreeMem(list, sizeof(struct List));
}

/*------------------------------------------------------------------------*/
/**
 * @brief Create a Chooser labels list from NULL-terminated string array
 */
/*------------------------------------------------------------------------*/
static struct List *create_chooser_labels(STRPTR *strings)
{
    struct List *list;
    struct Node *node;
    
    list = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
    if (!list)
        return NULL;
    
    NewList(list);
    
    while (*strings)
    {
        node = AllocChooserNode(CNA_Text, *strings, TAG_END);
        if (node)
        {
            AddTail(list, node);
        }
        strings++;
    }
    
    return list;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Show a ReAction requester dialog
 */
/*------------------------------------------------------------------------*/
static ULONG ShowReActionRequester(struct Window *parent_window,
                                   CONST_STRPTR title,
                                   CONST_STRPTR body,
                                   CONST_STRPTR gadgets,
                                   ULONG image_type)
{
    Object *req_obj;
    struct orRequest req_msg;
    ULONG result = 0;
    
    if (!RequesterBase)
    {
        log_error(LOG_GUI, "RequesterBase is NULL, cannot show requester\n");
        return 0;
    }
    
    if (!parent_window)
    {
        log_error(LOG_GUI, "Parent window is NULL, cannot show requester\n");
        return 0;
    }
    
    req_obj = NewObject(REQUESTER_GetClass(), NULL,
        REQ_Type, REQTYPE_INFO,
        REQ_TitleText, title,
        REQ_BodyText, body,
        REQ_GadgetText, gadgets,
        REQ_Image, image_type,
        TAG_DONE);
    
    if (req_obj)
    {
        req_msg.MethodID = RM_OPENREQ;
        req_msg.or_Attrs = NULL;
        req_msg.or_Window = parent_window;
        req_msg.or_Screen = NULL;
        
        result = DoMethodA(req_obj, (Msg)&req_msg);
        
        DisposeObject(req_obj);
    }
    else
    {
        log_error(LOG_GUI, "Failed to create requester object\n");
    }
    
    return result;
}

/*========================================================================*/
/* MENU SYSTEM FUNCTIONS                                                  */
/*========================================================================*/

/**
 * setup_tool_cache_menus - Initialize GadTools NewLook menu system
 */
static BOOL setup_tool_cache_menus(void)
{
    log_debug(LOG_GUI, "Setting up tool cache menus...\n");
    
    wb_screen_menu = LockPubScreen("Workbench");
    if (!wb_screen_menu)
    {
        log_error(LOG_GUI, "Error: Could not lock Workbench screen for menus\n");
        return FALSE;
    }
    
    draw_info_menu = GetScreenDrawInfo(wb_screen_menu);
    if (!draw_info_menu)
    {
        log_error(LOG_GUI, "Error: Could not get DrawInfo for menus\n");
        UnlockPubScreen(NULL, wb_screen_menu);
        wb_screen_menu = NULL;
        return FALSE;
    }
    
    visual_info_menu = GetVisualInfo(wb_screen_menu, TAG_END);
    if (!visual_info_menu)
    {
        log_error(LOG_GUI, "Error: Could not get VisualInfo for menus\n");
        FreeScreenDrawInfo(wb_screen_menu, draw_info_menu);
        UnlockPubScreen(NULL, wb_screen_menu);
        draw_info_menu = NULL;
        wb_screen_menu = NULL;
        return FALSE;
    }
    
    menu_strip = CreateMenus(tool_cache_menu_template, TAG_END);
    if (!menu_strip)
    {
        log_error(LOG_GUI, "Error: Could not create menu strip\n");
        FreeVisualInfo(visual_info_menu);
        FreeScreenDrawInfo(wb_screen_menu, draw_info_menu);
        UnlockPubScreen(NULL, wb_screen_menu);
        visual_info_menu = NULL;
        draw_info_menu = NULL;
        wb_screen_menu = NULL;
        return FALSE;
    }
    
    if (!LayoutMenus(menu_strip, visual_info_menu, GTMN_NewLookMenus, TRUE, TAG_END))
    {
        log_error(LOG_GUI, "Error: Could not layout NewLook menus\n");
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
    
    log_info(LOG_GUI, "Tool cache menus initialized successfully\n");
    return TRUE;
}

/**
 * cleanup_tool_cache_menus - Release all menu system resources
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
    
    log_info(LOG_GUI, "Tool cache menus cleaned up\n");
}

/**
 * handle_tool_cache_menu_selection - Process menu selection events
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
            
            switch (item_id)
            {
                case MENU_PROJECT_NEW:
                    handle_new_menu(tool_data);
                    break;
                    
                case MENU_PROJECT_OPEN:
                    handle_open_menu(tool_data);
                    break;
                    
                case MENU_PROJECT_SAVE:
                    handle_save_menu(tool_data);
                    break;
                    
                case MENU_PROJECT_SAVE_AS:
                    handle_save_as_menu(tool_data);
                    break;
                    
                case MENU_PROJECT_CLOSE:
                    continue_running = FALSE;
                    break;
                    
                case MENU_FILE_EXPORT_TOOLS:
                    handle_export_tools_menu(tool_data);
                    break;
                    
                case MENU_FILE_EXPORT_FILES:
                    handle_export_files_and_tools_menu(tool_data);
                    break;
                    
                case MENU_VIEW_SYSTEM_PATH:
                    handle_view_system_path_menu(tool_data);
                    break;
                    
                default:
                    log_warning(LOG_GUI, "Unknown menu item ID: %ld\n", item_id);
                    break;
            }
        }
        
        menu_number = menu_item->NextSelect;
    }
    
    return continue_running;
}

/**
 * handle_new_menu - Handle "New" menu selection
 */
static void handle_new_menu(struct iTidyToolCacheWindow *tool_data)
{
    if (!tool_data || !tool_data->window)
        return;
    
    log_debug(LOG_GUI, "Menu: New clicked - resetting window\n");
    
    if (tool_data->tool_listbrowser_obj)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj,
                       tool_data->window, NULL,
                       LISTBROWSER_Labels, ~0,
                       TAG_DONE);
    }
    
    if (tool_data->details_listbrowser_obj)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->details_listbrowser_obj,
                       tool_data->window, NULL,
                       LISTBROWSER_Labels, ~0,
                       TAG_DONE);
    }
    
    strcpy(tool_data->folder_path_buffer, "SYS:");
    
    if (tool_data->folder_getfile_obj)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->folder_getfile_obj,
                       tool_data->window, NULL,
                       GETFILE_Drawer, tool_data->folder_path_buffer,
                       TAG_DONE);
    }
    
    FreeToolCache();
    
    NewList(&tool_data->filtered_entries);
    
    free_tool_cache_entries(tool_data);
    
    tool_data->total_count = 0;
    tool_data->valid_count = 0;
    tool_data->missing_count = 0;
    tool_data->selected_index = -1;
    tool_data->selected_details_index = -1;
    
    if (tool_data->tool_listbrowser_obj)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj,
                       tool_data->window, NULL,
                       LISTBROWSER_Labels, &tool_data->filtered_entries,
                       LISTBROWSER_Selected, ~0,
                       TAG_DONE);
    }
    
    if (tool_data->details_listbrowser_obj)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->details_listbrowser_obj,
                       tool_data->window, NULL,
                       LISTBROWSER_Labels, &tool_data->details_list,
                       LISTBROWSER_Selected, ~0,
                       TAG_DONE);
    }
    
    tool_data->last_save_path[0] = '\0';
    
    update_button_states(tool_data);
    
    RefreshGadgets((struct Gadget *)tool_data->tool_listbrowser_obj,
                  tool_data->window, NULL);
    
    log_info(LOG_GUI, "Tool cache window reset to defaults\n");
}

/**
 * handle_save_menu - Handle "Save" menu selection
 */
static void handle_save_menu(struct iTidyToolCacheWindow *tool_data)
{
    extern ToolCacheEntry *g_ToolCache;
    extern int g_ToolCacheCount;
    
    if (!tool_data || !tool_data->window)
        return;
    
    if (!g_ToolCache || g_ToolCacheCount == 0)
    {
        ShowReActionRequester(tool_data->window,
            "No Data to Save",
            "The tool cache is empty.\nPlease scan a directory first.",
            "_OK",
            REQIMAGE_INFO);
        return;
    }
    
    if (tool_data->last_save_path[0] == '\0')
    {
        log_debug(LOG_GUI, "Menu: Save clicked but no previous save path - calling Save As\n");
        handle_save_as_menu(tool_data);
        return;
    }
    
    log_info(LOG_GUI, "Menu: Save clicked - saving to %s\n", tool_data->last_save_path);
    
    if (save_tool_cache_to_file(tool_data->last_save_path, tool_data->folder_path_buffer))
    {
        log_info(LOG_GUI, "Tool cache saved to: %s (folder: %s)\n", 
                 tool_data->last_save_path, tool_data->folder_path_buffer);
    }
    else
    {
        ShowReActionRequester(tool_data->window,
            "Save Failed",
            "Failed to save tool cache file.",
            "_OK",
            REQIMAGE_ERROR);
    }
}

/**
 * handle_save_as_menu - Handle "Save as..." menu selection
 */
static void handle_save_as_menu(struct iTidyToolCacheWindow *tool_data)
{
    extern ToolCacheEntry *g_ToolCache;
    extern int g_ToolCacheCount;
    struct FileRequester *freq;
    char full_path[512];
    char expanded_drawer[512];
    BPTR lock;
    
    if (!tool_data || !tool_data->window)
        return;
    
    if (!g_ToolCache || g_ToolCacheCount == 0)
    {
        ShowReActionRequester(tool_data->window,
            "No Data to Save",
            "The tool cache is empty.\nPlease scan a directory first.",
            "_OK",
            REQIMAGE_INFO);
        return;
    }
    
    if (!ExpandProgDir("PROGDIR:userdata/DTools", expanded_drawer, sizeof(expanded_drawer)))
    {
        log_warning(LOG_GUI, "Could not expand PROGDIR:, using fallback path\n");
        strcpy(expanded_drawer, "RAM:");
    }
    
    log_debug(LOG_GUI, "Using initial drawer: %s\n", expanded_drawer);
    
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
        log_error(LOG_GUI, "Failed to allocate file requester\n");
        ShowReActionRequester(tool_data->window,
            "Error",
            "Could not open file requester.",
            "_OK",
            REQIMAGE_ERROR);
        return;
    }
    
    if (AslRequest(freq, NULL))
    {
        strcpy(full_path, freq->fr_Drawer);
        if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            log_error(LOG_GUI, "Path too long: %s + %s\n", freq->fr_Drawer, freq->fr_File);
            FreeAslRequest(freq);
            ShowReActionRequester(tool_data->window,
                "Error",
                "File path is too long.",
                "_OK",
                REQIMAGE_ERROR);
            return;
        }
        
        log_info(LOG_GUI, "User selected save path: %s\n", full_path);
        
        lock = Lock((STRPTR)full_path, ACCESS_READ);
        if (lock)
        {
            UnLock(lock);
            
            if (!ShowReActionRequester(tool_data->window,
                "File Exists",
                "File already exists.\nDo you want to replace it?",
                "_Replace|_Cancel",
                REQIMAGE_QUESTION))
            {
                log_info(LOG_GUI, "User cancelled overwrite\n");
                FreeAslRequest(freq);
                return;
            }
        }
        
        if (save_tool_cache_to_file(full_path, tool_data->folder_path_buffer))
        {
            strncpy(tool_data->last_save_path, full_path, sizeof(tool_data->last_save_path) - 1);
            tool_data->last_save_path[sizeof(tool_data->last_save_path) - 1] = '\0';
            
            ShowReActionRequester(tool_data->window,
                "Save Successful",
                "Tool cache saved successfully.",
                "_OK",
                REQIMAGE_INFO);
            log_info(LOG_GUI, "Tool cache saved to: %s (folder: %s)\n", full_path, tool_data->folder_path_buffer);
        }
        else
        {
            ShowReActionRequester(tool_data->window,
                "Save Failed",
                "Failed to save tool cache file.",
                "_OK",
                REQIMAGE_ERROR);
        }
    }
    else
    {
        log_info(LOG_GUI, "User cancelled save operation\n");
    }
    
    FreeAslRequest(freq);
}

/**
 * handle_export_tools_menu - Handle "Export list of tools" menu selection
 */
static void handle_export_tools_menu(struct iTidyToolCacheWindow *tool_data)
{
    if (!tool_data || !tool_data->window)
        return;
    
    export_tool_list(tool_data->window, tool_data->folder_path_buffer);
}

/**
 * handle_export_files_and_tools_menu - Handle "Export list of files and tools" menu selection
 */
static void handle_export_files_and_tools_menu(struct iTidyToolCacheWindow *tool_data)
{
    if (!tool_data || !tool_data->window)
        return;
    
    export_files_and_tools_list(tool_data->window, tool_data->folder_path_buffer);
}

/**
 * handle_view_system_path_menu - Handle "View System PATH..." menu selection
 */
static void handle_view_system_path_menu(struct iTidyToolCacheWindow *tool_data)
{
    extern char **g_PathSearchList;
    extern int g_PathSearchCount;
    char *message_buffer;
    int buffer_size = 2048;
    int i;
    int offset = 0;
    
    if (!tool_data || !tool_data->window)
        return;
    
    log_debug(LOG_GUI, "Menu: View System PATH... clicked\n");
    
    message_buffer = (char *)whd_malloc(buffer_size);
    if (!message_buffer)
    {
        ShowReActionRequester(tool_data->window, 
                        "Memory Error",
                        "Failed to allocate memory for PATH display.",
                        "_OK",
                        REQIMAGE_ERROR);
        return;
    }
    
    memset(message_buffer, 0, buffer_size);
    
    if (g_PathSearchList && g_PathSearchCount > 0)
    {
        offset += snprintf(message_buffer + offset, buffer_size - offset,
                          "iTidy searches these directories for default tools:\n\n");
        
        for (i = 0; i < g_PathSearchCount && offset < buffer_size - 100; i++)
        {
            offset += snprintf(message_buffer + offset, buffer_size - offset,
                              "  %d. %s\n", i + 1, g_PathSearchList[i]);
        }
        
        if (i < g_PathSearchCount)
        {
            offset += snprintf(message_buffer + offset, buffer_size - offset,
                              "\n  ... and %d more directories", g_PathSearchCount - i);
        }
    }
    else
    {
        snprintf(message_buffer, buffer_size,
                "No PATH search list is currently loaded.\n\n"
                "The PATH is built when you click the Scan button.");
    }
    
    ShowReActionRequester(tool_data->window, "System PATH", message_buffer, "_OK", REQIMAGE_INFO);
    
    whd_free(message_buffer);
    
    log_debug(LOG_GUI, "PATH viewer closed\n");
}

/**
 * handle_open_menu - Handle "Open..." menu selection
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
    
    if (!ExpandProgDir("PROGDIR:userdata/DTools", expanded_drawer, sizeof(expanded_drawer)))
    {
        log_warning(LOG_GUI, "Could not expand PROGDIR:, using fallback path\n");
        strcpy(expanded_drawer, "RAM:");
    }
    
    log_debug(LOG_GUI, "Using initial drawer: %s\n", expanded_drawer);
    
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Open Tool Cache File...",
        ASLFR_InitialDrawer, expanded_drawer,
        ASLFR_InitialFile, "toolcache.dat",
        ASLFR_DoSaveMode, FALSE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, tool_data->window,
        TAG_END);
    
    if (!freq)
    {
        log_error(LOG_GUI, "Failed to allocate file requester\n");
        ShowReActionRequester(tool_data->window,
            "Error",
            "Could not open file requester.",
            "_OK",
            REQIMAGE_ERROR);
        return;
    }
    
    if (AslRequest(freq, NULL))
    {
        strcpy(full_path, freq->fr_Drawer);
        if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            log_error(LOG_GUI, "Path too long: %s + %s\n", freq->fr_Drawer, freq->fr_File);
            FreeAslRequest(freq);
            ShowReActionRequester(tool_data->window,
                "Error",
                "File path is too long.",
                "_OK",
                REQIMAGE_ERROR);
            return;
        }
        
        log_info(LOG_GUI, "User selected file: %s\n", full_path);
        
        lock = Lock((STRPTR)full_path, ACCESS_READ);
        if (!lock)
        {
            log_error(LOG_GUI, "File does not exist: %s\n", full_path);
            FreeAslRequest(freq);
            ShowReActionRequester(tool_data->window,
                "File Not Found",
                "The selected file does not exist.",
                "_OK",
                REQIMAGE_ERROR);
            return;
        }
        UnLock(lock);
        
        if (load_tool_cache_from_file(full_path, tool_data->folder_path_buffer))
        {
            if (tool_data->folder_getfile_obj)
            {
                SetGadgetAttrs((struct Gadget *)tool_data->folder_getfile_obj,
                               tool_data->window, NULL,
                               GETFILE_Drawer, tool_data->folder_path_buffer,
                               TAG_DONE);
            }
            
            {
                LayoutPreferences *prefs = (LayoutPreferences *)GetGlobalPreferences();
                if (prefs)
                {
                    strncpy(prefs->folder_path, tool_data->folder_path_buffer, sizeof(prefs->folder_path) - 1);
                    prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
                    UpdateGlobalPreferences(prefs);
                    log_debug(LOG_GUI, "Updated global preferences with loaded folder path: %s\n", prefs->folder_path);
                }
            }
            
            if (build_tool_cache_display_list(tool_data))
            {
                apply_tool_filter(tool_data);
                
                populate_tool_listbrowser(tool_data);
                
                ShowReActionRequester(tool_data->window,
                    "Load Successful",
                    "Tool cache loaded successfully.",
                    "_OK",
                    REQIMAGE_INFO);
                log_info(LOG_GUI, "Tool cache loaded from: %s (folder: %s)\n", full_path, tool_data->folder_path_buffer);
            }
            else
            {
                ShowReActionRequester(tool_data->window,
                    "Display Error",
                    "Cache loaded but failed to update display.",
                    "_OK",
                    REQIMAGE_ERROR);
            }
        }
        else
        {
            ShowReActionRequester(tool_data->window,
                "Load Failed",
                "Failed to load tool cache file.\nFile may be corrupted or invalid.",
                "_OK",
                REQIMAGE_ERROR);
        }
    }
    else
    {
        log_info(LOG_GUI, "User cancelled load operation\n");
    }
    
    FreeAslRequest(freq);
}

/*------------------------------------------------------------------------*/
/* Build Tool Cache Display List                                         */
/*------------------------------------------------------------------------*/
BOOL build_tool_cache_display_list(struct iTidyToolCacheWindow *tool_data)
{
    int i;
    struct ToolCacheDisplayEntry *entry;
    
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
        log_debug(LOG_GUI, "build_tool_cache_display_list: No tools in cache\n");
        return TRUE;  /* Empty list is valid */
    }
    
    log_debug(LOG_GUI, "build_tool_cache_display_list: Processing %d cached tools\n", g_ToolCacheCount);
    
    /* Build display entries from global cache */
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        /* Allocate entry */
        entry = (struct ToolCacheDisplayEntry *)whd_malloc(sizeof(struct ToolCacheDisplayEntry));
        if (entry == NULL)
        {
            log_error(LOG_GUI, "ERROR: Failed to allocate display entry\n");
            free_tool_cache_entries(tool_data);
            return FALSE;
        }
        memset(entry, 0, sizeof(struct ToolCacheDisplayEntry));
        
        /* Copy tool data - point to cache data, don't duplicate */
        entry->tool_name = g_ToolCache[i].toolName;
        entry->exists = g_ToolCache[i].exists;
        entry->hit_count = g_ToolCache[i].hitCount;
        entry->file_count = g_ToolCache[i].fileCount;
        entry->full_path = g_ToolCache[i].fullPath;
        entry->version = g_ToolCache[i].versionString;
        entry->cache_index = i;
        
        /* Set node name for internal tracking */
        entry->node.ln_Name = entry->tool_name;
        
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
    sprintf(tool_data->summary_text, "Total: %lu  |  Valid: %lu  |  Missing: %lu",
        tool_data->total_count, tool_data->valid_count, tool_data->missing_count);
    
    log_debug(LOG_GUI, "build_tool_cache_display_list: Created %lu entries\n", tool_data->total_count);
    
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
    NewList(&tool_data->filtered_entries);
    
    /* Build filtered list based on current filter */
    for (node = tool_data->tool_entries.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ)
    {
        entry = (struct ToolCacheDisplayEntry *)node;
        
        /* Apply filter */
        switch (tool_data->current_filter)
        {
            case TOOL_FILTER_ALL:
                entry->filter_node.ln_Name = entry->tool_name;
                AddTail(&tool_data->filtered_entries, &entry->filter_node);
                count++;
                break;
                
            case TOOL_FILTER_VALID:
                if (entry->exists)
                {
                    entry->filter_node.ln_Name = entry->tool_name;
                    AddTail(&tool_data->filtered_entries, &entry->filter_node);
                    count++;
                }
                break;
                
            case TOOL_FILTER_MISSING:
                if (!entry->exists)
                {
                    entry->filter_node.ln_Name = entry->tool_name;
                    AddTail(&tool_data->filtered_entries, &entry->filter_node);
                    count++;
                }
                break;
        }
    }
    
    log_debug(LOG_GUI, "apply_tool_filter: Filter=%d, Result count=%lu\n",
        tool_data->current_filter, count);
}

/*------------------------------------------------------------------------*/
/* Populate Tool ListBrowser                                              */
/* This replaces the old populate_tool_list function with real columns   */
/*------------------------------------------------------------------------*/
static void populate_tool_listbrowser(struct iTidyToolCacheWindow *tool_data)
{
    struct ToolCacheDisplayEntry *entry;
    struct Node *node;
    struct Node *lb_node;
    char truncated_name[64];
    char file_count_str[16];
    const char *status_str;
    
    if (tool_data == NULL || tool_data->tool_listbrowser_obj == NULL)
        return;
    
    /* Detach old list from gadget */
    SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj, 
                   tool_data->window, NULL,
                   LISTBROWSER_Labels, ~0,
                   TAG_DONE);
    
    /* Free existing list */
    if (tool_data->tool_list_nodes != NULL)
    {
        free_listbrowser_list(tool_data->tool_list_nodes);
        tool_data->tool_list_nodes = NULL;
    }
    
    /* Create new list */
    tool_data->tool_list_nodes = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
    if (tool_data->tool_list_nodes == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to allocate tool_list_nodes\n");
        return;
    }
    NewList(tool_data->tool_list_nodes);
    
    /* Add rows from filtered list */
    for (node = tool_data->filtered_entries.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ)
    {
        /* Get entry from filter_node offset */
        entry = (struct ToolCacheDisplayEntry *)((char *)node - sizeof(struct Node));
        
        /* Truncate tool name with /../ notation if needed */
        if (!iTidy_ShortenPathWithParentDir(
            entry->tool_name ? entry->tool_name : "(unknown)",
            truncated_name,
            50))
        {
            strncpy(truncated_name, entry->tool_name ? entry->tool_name : "(unknown)", 50);
            truncated_name[50] = '\0';
        }
        
        /* Format file count */
        sprintf(file_count_str, "%d", entry->file_count);
        
        /* Status string */
        status_str = entry->exists ? "EXISTS" : "MISSING";
        
        /* Create ListBrowser node with 3 columns */
        lb_node = AllocListBrowserNode(3,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, truncated_name,
            LBNA_Column, 1,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, file_count_str,
                LBNCA_Justification, LCJ_RIGHT,
            LBNA_Column, 2,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, (STRPTR)status_str,
            LBNA_UserData, (APTR)(ULONG)entry->cache_index,  /* Store cache index for lookup */
            TAG_DONE);
        
        if (lb_node)
        {
            AddTail(tool_data->tool_list_nodes, lb_node);
        }
    }
    
    /* Reattach list to gadget */
    SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj, 
                   tool_data->window, NULL,
                   LISTBROWSER_Labels, tool_data->tool_list_nodes,
                   LISTBROWSER_AutoFit, TRUE,
                   TAG_DONE);
    
    /* Clear selection */
    tool_data->selected_index = -1;
    
    /* Update button states */
    update_button_states(tool_data);
}

/*------------------------------------------------------------------------*/
/* Update Tool Details (for selected tool)                               */
/*------------------------------------------------------------------------*/
void update_tool_details(struct iTidyToolCacheWindow *tool_data, LONG selected_index)
{
    struct Node *lb_node;
    char buffer[512];
    int i, j;
    int cache_index = -1;
    
    if (tool_data == NULL)
        return;
    
    log_debug(LOG_GUI, "update_tool_details: selected_index=%ld\n", selected_index);
    
    /* Store selected index */
    tool_data->selected_index = selected_index;
    tool_data->selected_details_index = -1;
    
    /* Detach old list from gadget */
    if (tool_data->details_listbrowser_obj != NULL && tool_data->window != NULL)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->details_listbrowser_obj, 
                       tool_data->window, NULL,
                       LISTBROWSER_Labels, ~0,
                       TAG_DONE);
    }
    
    /* Free existing details list */
    if (tool_data->details_list_nodes != NULL)
    {
        free_listbrowser_list(tool_data->details_list_nodes);
        tool_data->details_list_nodes = NULL;
    }
    
    /* Create new list */
    tool_data->details_list_nodes = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
    if (tool_data->details_list_nodes == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to allocate details_list_nodes\n");
        return;
    }
    NewList(tool_data->details_list_nodes);
    
    /* If nothing selected, show placeholder */
    if (selected_index < 0)
    {
        lb_node = AllocListBrowserNode(1,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, "(Select a tool to view details)",
            TAG_DONE);
        if (lb_node) AddTail(tool_data->details_list_nodes, lb_node);
        goto attach_list;
    }
    
    /* Find selected entry by walking filtered list */
    {
        struct Node *node;
        LONG index = 0;
        
        for (node = tool_data->filtered_entries.lh_Head; 
             node->ln_Succ != NULL; 
             node = node->ln_Succ, index++)
        {
            if (index == selected_index)
            {
                struct ToolCacheDisplayEntry *entry;
                entry = (struct ToolCacheDisplayEntry *)((char *)node - sizeof(struct Node));
                cache_index = entry->cache_index;
                break;
            }
        }
    }
    
    if (cache_index < 0 || cache_index >= g_ToolCacheCount)
    {
        lb_node = AllocListBrowserNode(1,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, "(Invalid selection)",
            TAG_DONE);
        if (lb_node) AddTail(tool_data->details_list_nodes, lb_node);
        goto attach_list;
    }
    
    /* Add header info row: Tool name */
    sprintf(buffer, "Tool: %s", 
        g_ToolCache[cache_index].toolName ? g_ToolCache[cache_index].toolName : "(unknown)");
    lb_node = AllocListBrowserNode(1,
        LBNA_Column, 0,
            LBNCA_CopyText, TRUE,
            LBNCA_Text, buffer,
        LBNA_Flags, LBFLG_HASCHILDREN,  /* Visual indicator - not expandable */
        TAG_DONE);
    if (lb_node) AddTail(tool_data->details_list_nodes, lb_node);
    
    /* Add status row */
    if (g_ToolCache[cache_index].exists)
    {
        sprintf(buffer, "Status: EXISTS  |  Version: %s",
            g_ToolCache[cache_index].versionString ? g_ToolCache[cache_index].versionString : "(no version)");
    }
    else
    {
        sprintf(buffer, "Status: MISSING");
    }
    lb_node = AllocListBrowserNode(1,
        LBNA_Column, 0,
            LBNCA_CopyText, TRUE,
            LBNCA_Text, buffer,
        TAG_DONE);
    if (lb_node) AddTail(tool_data->details_list_nodes, lb_node);
    
    /* Add separator */
    lb_node = AllocListBrowserNode(1,
        LBNA_Column, 0,
            LBNCA_CopyText, TRUE,
            LBNCA_Text, "----------------------------------------",
        TAG_DONE);
    if (lb_node) AddTail(tool_data->details_list_nodes, lb_node);
    
    /* Add file references */
    if (g_ToolCache[cache_index].fileCount > 0 && g_ToolCache[cache_index].referencingFiles)
    {
        for (j = 0; j < g_ToolCache[cache_index].fileCount; j++)
        {
            if (g_ToolCache[cache_index].referencingFiles[j])
            {
                char truncated_path[80];
                
                /* Abbreviate long paths */
                if (!iTidy_ShortenPathWithParentDir(g_ToolCache[cache_index].referencingFiles[j], 
                                                     truncated_path, 70))
                {
                    strncpy(truncated_path, g_ToolCache[cache_index].referencingFiles[j], 70);
                    truncated_path[70] = '\0';
                }
                
                lb_node = AllocListBrowserNode(1,
                    LBNA_Column, 0,
                        LBNCA_CopyText, TRUE,
                        LBNCA_Text, truncated_path,
                    LBNA_UserData, (APTR)(ULONG)j,  /* Store file index for single replace */
                    TAG_DONE);
                if (lb_node) AddTail(tool_data->details_list_nodes, lb_node);
            }
        }
        
        /* Add footer if truncated */
        if (g_ToolCache[cache_index].fileCount >= TOOL_CACHE_MAX_FILES_PER_TOOL)
        {
            sprintf(buffer, "(showing first %d files, more may exist)", TOOL_CACHE_MAX_FILES_PER_TOOL);
            lb_node = AllocListBrowserNode(1,
                LBNA_Column, 0,
                    LBNCA_CopyText, TRUE,
                    LBNCA_Text, buffer,
                TAG_DONE);
            if (lb_node) AddTail(tool_data->details_list_nodes, lb_node);
        }
    }
    else
    {
        lb_node = AllocListBrowserNode(1,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, "(no files using this tool)",
            TAG_DONE);
        if (lb_node) AddTail(tool_data->details_list_nodes, lb_node);
    }
    
attach_list:
    /* Reattach list to gadget */
    if (tool_data->details_listbrowser_obj != NULL && tool_data->window != NULL)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->details_listbrowser_obj, 
                       tool_data->window, NULL,
                       LISTBROWSER_Labels, tool_data->details_list_nodes,
                       TAG_DONE);
    }
    
    /* Update button states */
    update_button_states(tool_data);
}

/*------------------------------------------------------------------------*/
/* Populate Details ListBrowser (called internally)                      */
/*------------------------------------------------------------------------*/
static void populate_details_listbrowser(struct iTidyToolCacheWindow *tool_data)
{
    /* Just calls update_tool_details with current selection */
    update_tool_details(tool_data, tool_data->selected_index);
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
            whd_free(entry->display_text);
        /* Note: Don't free tool_name, full_path, version - they point to g_ToolCache */
        whd_free(entry);
    }
    
    /* Clear filtered entries list (doesn't own entries) */
    NewList(&tool_data->filtered_entries);
    
    /* Free details list nodes (legacy) */
    while ((node = RemHead(&tool_data->details_list)) != NULL)
    {
        if (node->ln_Name != NULL)
            whd_free(node->ln_Name);
        whd_free(node);
    }
}

/*------------------------------------------------------------------------*/
/* Update Button States                                                   */
/*------------------------------------------------------------------------*/
static void update_button_states(struct iTidyToolCacheWindow *tool_data)
{
    BOOL has_tool_data;
    BOOL has_valid_tool_selection;
    BOOL has_valid_details_selection;
    
    if (tool_data == NULL || tool_data->window == NULL)
        return;
    
    /* Check if we have any tool data */
    has_tool_data = (tool_data->total_count > 0);
    
    /* Check if a tool is selected */
    has_valid_tool_selection = (tool_data->selected_index >= 0);
    
    /* Check if a file is selected in details (skip header rows: 0=tool, 1=status, 2=separator) */
    has_valid_details_selection = (tool_data->selected_details_index >= 3);
    
    /* Filter chooser: enabled only if there's tool data */
    if (tool_data->filter_chooser_obj)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->filter_chooser_obj,
                       tool_data->window, NULL,
                       GA_Disabled, !has_tool_data,
                       TAG_DONE);
    }
    
    /* Replace Tool (Batch) button: enabled only if valid tool is selected */
    if (tool_data->replace_batch_btn_obj)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->replace_batch_btn_obj,
                       tool_data->window, NULL,
                       GA_Disabled, !has_valid_tool_selection,
                       TAG_DONE);
    }
    
    /* Replace Tool (Single) button: enabled only if valid file selected in details */
    if (tool_data->replace_single_btn_obj)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->replace_single_btn_obj,
                       tool_data->window, NULL,
                       GA_Disabled, !has_valid_details_selection,
                       TAG_DONE);
    }
}

/*------------------------------------------------------------------------*/
/* Tool Cache Save/Load Functions                                        */
/*------------------------------------------------------------------------*/
static BOOL save_tool_cache_to_file(const char *filepath, const char *folder_path)
{
    BPTR file;
    LONG i, j;
    LONG len;
    LONG exists_flag;
    const char header[] = "ITIDYTOOLCACHE";
    ULONG version = 2;
    
    if (!filepath || !g_ToolCache)
    {
        log_error(LOG_GUI, "save_tool_cache_to_file: Invalid parameters\n");
        return FALSE;
    }
    
    file = Open((STRPTR)filepath, MODE_NEWFILE);
    if (!file)
    {
        log_error(LOG_GUI, "save_tool_cache_to_file: Failed to create file: %s\n", filepath);
        return FALSE;
    }
    
    log_info(LOG_GUI, "Saving tool cache to: %s\n", filepath);
    
    /* Write header */
    if (Write(file, (APTR)header, 14) != 14)
    {
        Close(file);
        return FALSE;
    }
    
    /* Write version */
    if (Write(file, (APTR)&version, sizeof(ULONG)) != sizeof(ULONG))
    {
        Close(file);
        return FALSE;
    }

    /* Write folder path */
    if (folder_path)
    {
        len = strlen(folder_path);
        Write(file, (APTR)&len, sizeof(LONG));
        Write(file, (APTR)folder_path, len);
    }
    else
    {
        len = 0;
        Write(file, (APTR)&len, sizeof(LONG));
    }

    /* Write tool count */
    Write(file, (APTR)&g_ToolCacheCount, sizeof(LONG));

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
    log_info(LOG_GUI, "Successfully saved %d tools to cache file\n", g_ToolCacheCount);
    return TRUE;
}

static BOOL load_tool_cache_from_file(const char *filepath, char *folder_path_out)
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
        log_error(LOG_GUI, "load_tool_cache_from_file: Invalid filepath\n");
        return FALSE;
    }
    
    log_info(LOG_GUI, "Loading tool cache from: %s\n", filepath);
    
    file = Open((STRPTR)filepath, MODE_OLDFILE);
    if (!file)
    {
        log_error(LOG_GUI, "Failed to open file for reading: %s\n", filepath);
        return FALSE;
    }
    
    /* Read and validate header */
    memset(header, 0, sizeof(header));
    if (Read(file, (APTR)header, 14) != 14 || strncmp(header, "ITIDYTOOLCACHE", 14) != 0)
    {
        log_error(LOG_GUI, "Invalid file format\n");
        Close(file);
        return FALSE;
    }
    
    /* Read version */
    if (Read(file, (APTR)&version, sizeof(ULONG)) != sizeof(ULONG) || version != 2)
    {
        log_error(LOG_GUI, "Unsupported file version\n");
        Close(file);
        return FALSE;
    }

    /* Read folder path */
    if (Read(file, (APTR)&len, sizeof(LONG)) != sizeof(LONG))
    {
        Close(file);
        return FALSE;
    }

    if (len > 0 && len < 512)
    {
        if (folder_path_out)
        {
            Read(file, (APTR)folder_path_out, len);
            folder_path_out[len] = '\0';
        }
        else
        {
            Seek(file, len, OFFSET_CURRENT);
        }
    }
    else if (folder_path_out)
    {
        folder_path_out[0] = '\0';
    }

    /* Read tool count */
    if (Read(file, (APTR)&tool_count, sizeof(LONG)) != sizeof(LONG))
    {
        Close(file);
        return FALSE;
    }

    if (tool_count < 0 || tool_count > 10000)
    {
        log_error(LOG_GUI, "Invalid tool count: %ld\n", tool_count);
        Close(file);
        return FALSE;
    }
    
    /* Allocate temporary cache array */
    if (tool_count > 0)
    {
        temp_cache = (ToolCacheEntry *)whd_malloc(tool_count * sizeof(ToolCacheEntry));
        if (!temp_cache)
        {
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
            goto load_error;
        
        if (len > 0 && len < 512)
        {
            temp_cache[i].toolName = (char *)whd_malloc(len + 1);
            if (!temp_cache[i].toolName)
                goto load_error;
            Read(file, (APTR)temp_cache[i].toolName, len);
            temp_cache[i].toolName[len] = '\0';
        }
        
        /* Read exists flag */
        if (Read(file, (APTR)&exists_flag, sizeof(LONG)) != sizeof(LONG))
            goto load_error;
        temp_cache[i].exists = (exists_flag != 0);
        
        /* Read full path */
        if (Read(file, (APTR)&len, sizeof(LONG)) != sizeof(LONG))
            goto load_error;
        
        if (len > 0 && len < 1024)
        {
            temp_cache[i].fullPath = (char *)whd_malloc(len + 1);
            if (!temp_cache[i].fullPath)
                goto load_error;
            Read(file, (APTR)temp_cache[i].fullPath, len);
            temp_cache[i].fullPath[len] = '\0';
        }
        
        /* Read version string */
        if (Read(file, (APTR)&len, sizeof(LONG)) != sizeof(LONG))
            goto load_error;
        
        if (len > 0 && len < 256)
        {
            temp_cache[i].versionString = (char *)whd_malloc(len + 1);
            if (!temp_cache[i].versionString)
                goto load_error;
            Read(file, (APTR)temp_cache[i].versionString, len);
            temp_cache[i].versionString[len] = '\0';
        }
        
        /* Read hit count */
        Read(file, (APTR)&temp_cache[i].hitCount, sizeof(LONG));
        
        /* Read file count */
        Read(file, (APTR)&temp_cache[i].fileCount, sizeof(LONG));
        
        if (temp_cache[i].fileCount > 0 && temp_cache[i].fileCount <= 200)
        {
            temp_cache[i].referencingFiles = (char **)whd_malloc(temp_cache[i].fileCount * sizeof(char *));
            if (!temp_cache[i].referencingFiles)
                goto load_error;
            memset(temp_cache[i].referencingFiles, 0, temp_cache[i].fileCount * sizeof(char *));
            
            for (j = 0; j < temp_cache[i].fileCount; j++)
            {
                if (Read(file, (APTR)&len, sizeof(LONG)) != sizeof(LONG))
                    goto load_error;
                
                if (len > 0 && len < 1024)
                {
                    temp_cache[i].referencingFiles[j] = (char *)whd_malloc(len + 1);
                    if (!temp_cache[i].referencingFiles[j])
                        goto load_error;
                    Read(file, (APTR)temp_cache[i].referencingFiles[j], len);
                    temp_cache[i].referencingFiles[j][len] = '\0';
                }
            }
        }
    }
    
    Close(file);
    
    /* Success - replace global cache */
    FreeToolCache();
    
    g_ToolCache = temp_cache;
    g_ToolCacheCount = tool_count;
    g_ToolCacheCapacity = tool_count;
    
    log_info(LOG_GUI, "Successfully loaded %ld tools from cache file\n", tool_count);
    return TRUE;
    
load_error:
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

/*------------------------------------------------------------------------*/
/* Button Handlers                                                        */
/*------------------------------------------------------------------------*/
static void handle_scan_button(struct iTidyToolCacheWindow *tool_data)
{
    struct iTidyMainProgressWindow progress_window;
    LayoutPreferences *prefs;
    BOOL success;
    BOOL original_recursive_mode;
    char *folder_path = NULL;
    
    log_info(LOG_GUI, "Scan button clicked\n");
    
    /* Get folder path from GetFile gadget */
    if (tool_data->folder_getfile_obj)
    {
        GetAttr(GETFILE_Drawer, tool_data->folder_getfile_obj, (ULONG *)&folder_path);
        if (folder_path && folder_path[0])
        {
            strncpy(tool_data->folder_path_buffer, folder_path, sizeof(tool_data->folder_path_buffer) - 1);
            tool_data->folder_path_buffer[sizeof(tool_data->folder_path_buffer) - 1] = '\0';
        }
    }
    
    /* Detach lists and clear display */
    if (tool_data->tool_listbrowser_obj && tool_data->window)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj,
                       tool_data->window, NULL,
                       LISTBROWSER_Labels, ~0,
                       TAG_DONE);
    }
    
    if (tool_data->details_listbrowser_obj && tool_data->window)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->details_listbrowser_obj,
                       tool_data->window, NULL,
                       LISTBROWSER_Labels, ~0,
                       TAG_DONE);
    }
    
    /* Free old display entries */
    free_tool_cache_entries(tool_data);
    
    /* Re-initialize empty lists */
    NewList(&tool_data->tool_entries);
    NewList(&tool_data->filtered_entries);
    NewList(&tool_data->details_list);
    
    /* Update global preferences with current folder path */
    prefs = (LayoutPreferences *)GetGlobalPreferences();
    if (prefs)
    {
        strncpy(prefs->folder_path, tool_data->folder_path_buffer, 
               sizeof(prefs->folder_path) - 1);
        prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
        
        /* Save original recursive mode and force it to TRUE */
        original_recursive_mode = prefs->recursive_subdirs;
        prefs->recursive_subdirs = TRUE;
        
        UpdateGlobalPreferences(prefs);
        log_info(LOG_GUI, "Using folder path: %s (recursive: FORCED ON)\n", prefs->folder_path);
    }
    
    /* Open progress window */
    if (!itidy_main_progress_window_open(&progress_window))
    {
        log_error(LOG_GUI, "Failed to open progress window\n");
        
        if (prefs)
        {
            prefs->recursive_subdirs = original_recursive_mode;
            UpdateGlobalPreferences(prefs);
        }
        
        ShowReActionRequester(tool_data->window,
            "Error",
            "Failed to open progress window",
            "_OK",
            REQIMAGE_ERROR);
        return;
    }
    
    /* Set busy pointer */
    safe_set_window_pointer(tool_data->window, TRUE);
    
    /* Rescan the directory */
    success = ScanDirectoryForToolsOnlyWithProgress(&progress_window);
    
    /* Restore original recursive mode */
    if (prefs)
    {
        prefs->recursive_subdirs = original_recursive_mode;
        UpdateGlobalPreferences(prefs);
    }
    
    /* Show result in progress window */
    itidy_main_progress_window_append_status(&progress_window, "");
    itidy_main_progress_window_append_status(&progress_window, 
        "===============================================");
    
    if (success)
    {
        char stats_buffer[256];
        int valid_tools = 0;
        int invalid_tools = 0;
        int valid_icon_count = 0;
        int invalid_icon_count = 0;
        int i;
        
        log_info(LOG_GUI, "Tool cache rebuilt successfully\n");
        itidy_main_progress_window_append_status(&progress_window, 
            "Tool cache rebuilt successfully!");
        
        /* Calculate statistics */
        for (i = 0; i < g_ToolCacheCount; i++)
        {
            if (g_ToolCache[i].exists)
            {
                valid_tools++;
                valid_icon_count += g_ToolCache[i].fileCount;
            }
            else
            {
                invalid_tools++;
                invalid_icon_count += g_ToolCache[i].fileCount;
            }
        }
        
        itidy_main_progress_window_append_status(&progress_window, "");
        
        if (valid_tools > 0)
        {
            sprintf(stats_buffer, "Valid: %d tool%s found (%d icon%s)",
                   valid_tools, valid_tools == 1 ? "" : "s",
                   valid_icon_count, valid_icon_count == 1 ? "" : "s");
            itidy_main_progress_window_append_status(&progress_window, stats_buffer);
        }
        
        if (invalid_tools > 0)
        {
            sprintf(stats_buffer, "Invalid: %d tool%s not found (%d icon%s)",
                   invalid_tools, invalid_tools == 1 ? "" : "s",
                   invalid_icon_count, invalid_icon_count == 1 ? "" : "s");
            itidy_main_progress_window_append_status(&progress_window, stats_buffer);
        }
    }
    else
    {
        log_error(LOG_GUI, "Tool cache rebuild failed or was cancelled\n");
        itidy_main_progress_window_append_status(&progress_window, 
            "Tool cache rebuild failed or was cancelled");
    }
    
    itidy_main_progress_window_append_status(&progress_window, 
        "===============================================");
    
    /* Change Cancel button to Close */
    itidy_main_progress_window_set_button_text(&progress_window, "Close");
    
    /* Clear busy pointer */
    safe_set_window_pointer(tool_data->window, FALSE);
    
    /* Wait for user to close progress window */
    while (itidy_main_progress_window_handle_events(&progress_window))
    {
        WaitPort(progress_window.window->UserPort);
    }
    
    itidy_main_progress_window_close(&progress_window);
    
    /* Rebuild display if successful */
    if (success)
    {
        build_tool_cache_display_list(tool_data);
        apply_tool_filter(tool_data);
        populate_tool_listbrowser(tool_data);
        
        tool_data->selected_index = -1;
        update_tool_details(tool_data, -1);
        
        log_info(LOG_GUI, "Display updated with new cache data\n");
    }
}

static void handle_replace_batch_button(struct iTidyToolCacheWindow *tool_data)
{
    struct ToolCacheDisplayEntry *entry;
    struct Node *node;
    LONG index = 0;
    int i, j;
    struct iTidy_DefaultToolUpdateWindow *update_window;
    struct iTidy_DefaultToolUpdateContext update_ctx;
    char **icon_paths_array = NULL;
    int cache_index = -1;
    
    log_info(LOG_GUI, "Replace Tool (Batch) button clicked\n");
    
    if (tool_data->selected_index < 0)
    {
        ShowReActionRequester(tool_data->window,
            "No Tool Selected",
            "Please select a tool from the list first.",
            "_OK",
            REQIMAGE_INFO);
        return;
    }
    
    /* Allocate window structure on heap */
    update_window = (struct iTidy_DefaultToolUpdateWindow *)whd_malloc(sizeof(struct iTidy_DefaultToolUpdateWindow));
    if (update_window == NULL)
    {
        ShowReActionRequester(tool_data->window,
            "Memory Error",
            "Could not allocate memory for update window.",
            "_OK",
            REQIMAGE_ERROR);
        return;
    }
    
    /* Find selected entry */
    for (node = tool_data->filtered_entries.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ, index++)
    {
        if (index == tool_data->selected_index)
        {
            entry = (struct ToolCacheDisplayEntry *)((char *)node - sizeof(struct Node));
            cache_index = entry->cache_index;
            break;
        }
    }
    
    if (cache_index < 0 || cache_index >= g_ToolCacheCount)
    {
        whd_free(update_window);
        return;
    }
    
    /* Allocate and copy icon paths (deep copy required) */
    icon_paths_array = (char **)whd_malloc(g_ToolCache[cache_index].fileCount * sizeof(char *));
    if (icon_paths_array == NULL)
    {
        whd_free(update_window);
        return;
    }
    
    for (j = 0; j < g_ToolCache[cache_index].fileCount; j++)
    {
        int path_len = strlen(g_ToolCache[cache_index].referencingFiles[j]);
        icon_paths_array[j] = (char *)whd_malloc(path_len + 1);
        if (icon_paths_array[j] != NULL)
        {
            strcpy(icon_paths_array[j], g_ToolCache[cache_index].referencingFiles[j]);
        }
    }
    
    /* Populate context for batch mode */
    memset(&update_ctx, 0, sizeof(update_ctx));
    update_ctx.mode = UPDATE_MODE_BATCH;
    update_ctx.current_tool = g_ToolCache[cache_index].toolName;
    update_ctx.icon_count = g_ToolCache[cache_index].fileCount;
    update_ctx.icon_paths = icon_paths_array;
    update_ctx.parent_window = (void *)tool_data;
    
    log_info(LOG_GUI, "Opening batch update window for %d icons\n", update_ctx.icon_count);
    
    memset(update_window, 0, sizeof(struct iTidy_DefaultToolUpdateWindow));
    
    if (iTidy_OpenDefaultToolUpdateWindow(update_window, &update_ctx))
    {
        while (iTidy_HandleDefaultToolUpdateEvents(update_window))
        {
            /* Keep processing events */
        }
        iTidy_CloseDefaultToolUpdateWindow(update_window);
    }
    
    whd_free(update_window);
    
    /* Free icon paths array */
    if (icon_paths_array != NULL)
    {
        for (j = 0; j < g_ToolCache[cache_index].fileCount; j++)
        {
            if (icon_paths_array[j] != NULL)
                whd_free(icon_paths_array[j]);
        }
        whd_free(icon_paths_array);
    }
}

static void handle_replace_single_button(struct iTidyToolCacheWindow *tool_data)
{
    struct ToolCacheDisplayEntry *entry;
    struct Node *node;
    LONG index = 0;
    int cache_index = -1;
    int file_index;
    struct iTidy_DefaultToolUpdateWindow *update_window;
    struct iTidy_DefaultToolUpdateContext update_ctx;
    
    log_info(LOG_GUI, "Replace Tool (Single) button clicked\n");
    
    if (tool_data->selected_index < 0)
    {
        ShowReActionRequester(tool_data->window,
            "No Tool Selected",
            "Please select a tool from the upper list first.",
            "_OK",
            REQIMAGE_INFO);
        return;
    }
    
    if (tool_data->selected_details_index < 3)
    {
        ShowReActionRequester(tool_data->window,
            "No File Selected",
            "Please select a specific icon file from the lower list.",
            "_OK",
            REQIMAGE_INFO);
        return;
    }
    
    /* Find selected tool entry */
    for (node = tool_data->filtered_entries.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ, index++)
    {
        if (index == tool_data->selected_index)
        {
            entry = (struct ToolCacheDisplayEntry *)((char *)node - sizeof(struct Node));
            cache_index = entry->cache_index;
            break;
        }
    }
    
    if (cache_index < 0 || cache_index >= g_ToolCacheCount)
        return;
    
    /* Calculate file index (details list has 3 header rows) */
    file_index = tool_data->selected_details_index - 3;
    
    if (file_index < 0 || file_index >= g_ToolCache[cache_index].fileCount)
    {
        ShowReActionRequester(tool_data->window,
            "Invalid Selection",
            "Please select a file entry (not the header).",
            "_OK",
            REQIMAGE_INFO);
        return;
    }
    
    /* Allocate window structure */
    update_window = (struct iTidy_DefaultToolUpdateWindow *)whd_malloc(sizeof(struct iTidy_DefaultToolUpdateWindow));
    if (update_window == NULL)
    {
        ShowReActionRequester(tool_data->window,
            "Memory Error",
            "Could not allocate memory for update window.",
            "_OK",
            REQIMAGE_ERROR);
        return;
    }
    
    /* Populate context for single mode */
    memset(&update_ctx, 0, sizeof(update_ctx));
    update_ctx.mode = UPDATE_MODE_SINGLE;
    update_ctx.current_tool = g_ToolCache[cache_index].toolName;
    update_ctx.single_info_path = g_ToolCache[cache_index].referencingFiles[file_index];
    update_ctx.parent_window = (void *)tool_data;
    
    log_info(LOG_GUI, "Opening single update window for file [%d]: %s\n",
                 file_index, update_ctx.single_info_path);
    
    memset(update_window, 0, sizeof(struct iTidy_DefaultToolUpdateWindow));
    
    if (iTidy_OpenDefaultToolUpdateWindow(update_window, &update_ctx))
    {
        while (iTidy_HandleDefaultToolUpdateEvents(update_window))
        {
            /* Keep processing events */
        }
        iTidy_CloseDefaultToolUpdateWindow(update_window);
    }
    
    whd_free(update_window);
}

static void handle_restore_tools_button(struct iTidyToolCacheWindow *tool_data)
{
    struct Window *restore_window;
    struct IntuiMessage *restore_msg;
    BOOL keep_running;
    iTidy_ToolBackupManager temp_manager;
    
    log_info(LOG_GUI, "Restore Default Tools button clicked\n");
    
    /* Initialize temporary backup manager */
    if (!iTidy_InitToolBackupManager(&temp_manager, FALSE))
    {
        log_error(LOG_GUI, "Failed to initialize backup manager\n");
        ShowReActionRequester(tool_data->window,
            "Error",
            "Failed to initialize backup system.",
            "_OK",
            REQIMAGE_ERROR);
        return;
    }
    
    /* Set busy pointer */
    safe_set_window_pointer(tool_data->window, TRUE);
    
    /* Create restore window */
    restore_window = iTidy_CreateToolRestoreWindow(tool_data->window->WScreen, &temp_manager);
    
    if (!restore_window)
    {
        safe_set_window_pointer(tool_data->window, FALSE);
        iTidy_CleanupToolBackupManager(&temp_manager);
        
        ShowReActionRequester(tool_data->window,
            "Window Error",
            "Failed to create restore window.",
            "_OK",
            REQIMAGE_ERROR);
        return;
    }
    
    safe_set_window_pointer(tool_data->window, FALSE);
    
    /* Run restore window event loop */
    keep_running = TRUE;
    while (keep_running)
    {
        WaitPort(restore_window->UserPort);
        
        while ((restore_msg = GT_GetIMsg(restore_window->UserPort)))
        {
            keep_running = iTidy_HandleToolRestoreWindowEvent(restore_window, restore_msg);
            GT_ReplyIMsg(restore_msg);
            
            if (!keep_running)
                break;
        }
    }
    
    /* Check if any restore operations were performed */
    BOOL restore_performed = iTidy_WasRestorePerformed();
    
    /* Close restore window */
    iTidy_CloseToolRestoreWindow(restore_window);
    iTidy_CleanupToolBackupManager(&temp_manager);
    
    /* If restores were performed, invalidate cache */
    if (restore_performed)
    {
        log_info(LOG_GUI, "Restores were performed - clearing tool cache\n");
        
        /* Detach lists */
        if (tool_data->tool_listbrowser_obj && tool_data->window)
        {
            SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj,
                           tool_data->window, NULL,
                           LISTBROWSER_Labels, ~0,
                           TAG_DONE);
        }
        
        if (tool_data->details_listbrowser_obj && tool_data->window)
        {
            SetGadgetAttrs((struct Gadget *)tool_data->details_listbrowser_obj,
                           tool_data->window, NULL,
                           LISTBROWSER_Labels, ~0,
                           TAG_DONE);
        }
        
        /* Free display entries and global cache */
        free_tool_cache_entries(tool_data);
        FreeToolCache();
        
        /* Re-initialize empty lists */
        NewList(&tool_data->tool_entries);
        NewList(&tool_data->filtered_entries);
        NewList(&tool_data->details_list);
        
        /* Clear counts and selection */
        tool_data->total_count = 0;
        tool_data->valid_count = 0;
        tool_data->missing_count = 0;
        tool_data->selected_index = -1;
        tool_data->selected_details_index = -1;
        
        /* Free and clear listbrowser node lists */
        if (tool_data->tool_list_nodes)
        {
            free_listbrowser_list(tool_data->tool_list_nodes);
            tool_data->tool_list_nodes = NULL;
        }
        if (tool_data->details_list_nodes)
        {
            free_listbrowser_list(tool_data->details_list_nodes);
            tool_data->details_list_nodes = NULL;
        }
        
        update_button_states(tool_data);
        
        ShowReActionRequester(tool_data->window,
            "Cache Cleared",
            "Tool cache cleared due to restore operation.\nPlease scan a folder to refresh the cache.",
            "_OK",
            REQIMAGE_INFO);
    }
}

/*------------------------------------------------------------------------*/
/* Open Tool Cache Window                                                 */
/*------------------------------------------------------------------------*/
BOOL open_tool_cache_window(struct iTidyToolCacheWindow *tool_data)
{
    struct Screen *screen;
    static STRPTR filter_labels_str[] = {
        "Show All",
        "Show Valid Only",
        "Show Missing Only",
        NULL
    };
    
    log_info(LOG_GUI, "=== open_tool_cache_window: Starting (ReAction) ===\n");
    
    if (tool_data == NULL)
    {
        log_error(LOG_GUI, "ERROR: tool_data is NULL\n");
        return FALSE;
    }
    
    /* Initialize structure */
    memset(tool_data, 0, sizeof(struct iTidyToolCacheWindow));
    NewList(&tool_data->tool_entries);
    NewList(&tool_data->filtered_entries);
    NewList(&tool_data->details_list);
    tool_data->current_filter = TOOL_FILTER_ALL;
    tool_data->selected_index = -1;
    tool_data->selected_details_index = -1;
    tool_data->last_sort_column = 0;
    tool_data->sort_direction = LBMSORT_FORWARD;
    
    /* Initialize folder path from global preferences */
    {
        const LayoutPreferences *prefs = GetGlobalPreferences();
        if (prefs && prefs->folder_path[0])
        {
            strncpy(tool_data->folder_path_buffer, prefs->folder_path, sizeof(tool_data->folder_path_buffer) - 1);
            tool_data->folder_path_buffer[sizeof(tool_data->folder_path_buffer) - 1] = '\0';
        }
        else
        {
            strcpy(tool_data->folder_path_buffer, "SYS:");
        }
    }
    
    /* Open ReAction classes */
    if (!open_reaction_classes())
    {
        log_error(LOG_GUI, "Failed to open ReAction classes\n");
        return FALSE;
    }
    
    /* Initialize column info with sorting attributes */
    if (!init_column_info())
    {
        log_error(LOG_GUI, "Failed to initialize column info\n");
        close_reaction_classes();
        return FALSE;
    }
    
    /* Get Workbench screen */
    screen = LockPubScreen(NULL);
    if (screen == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to lock Workbench screen\n");
        close_reaction_classes();
        return FALSE;
    }
    
    tool_data->screen = screen;
    
    /* Setup menu system BEFORE creating window object */
    if (!setup_tool_cache_menus())
    {
        log_error(LOG_GUI, "Failed to setup menus\n");
        free_chooser_labels(tool_data->filter_labels);
        UnlockPubScreen(NULL, screen);
        close_reaction_classes();
        return FALSE;
    }
    
    /* Create filter chooser labels */
    tool_data->filter_labels = create_chooser_labels(filter_labels_str);
    if (tool_data->filter_labels == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to create chooser labels\n");
        cleanup_tool_cache_menus();
        UnlockPubScreen(NULL, screen);
        close_reaction_classes();
        return FALSE;
    }
    
    /* Create the ReAction window object */
    tool_data->window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title, TOOL_WINDOW_TITLE,
        WA_ScreenTitle, TOOL_WINDOW_TITLE,
        WA_PubScreen, screen,
        WA_Left, 50,
        WA_Top, 30,
        WA_Width, 550,
        WA_Height, 400,
        WA_MinWidth, 450,
        WA_MinHeight, 300,
        WA_MaxWidth, 8192,
        WA_MaxHeight, 8192,
        WA_CloseGadget, TRUE,
        WA_DepthGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_DragBar, TRUE,
        WA_Activate, TRUE,
        WA_NoCareRefresh, TRUE,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_MENUPICK,
        
        WINDOW_ParentGroup, NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_DeferLayout, TRUE,
            
            /* Root vertical layout */
            LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                GA_ID, GID_TOOL_ROOT_LAYOUT,
                LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                LAYOUT_LeftSpacing, 2,
                LAYOUT_RightSpacing, 2,
                LAYOUT_TopSpacing, 2,
                LAYOUT_BottomSpacing, 2,
                
                /* Folder selection row */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, GID_TOOL_FOLDER_LAYOUT,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    
                    LAYOUT_AddChild, tool_data->folder_getfile_obj = NewObject(GETFILE_GetClass(), NULL,
                        GA_ID, GID_TOOL_FOLDER_GETFILE,
                        GA_RelVerify, TRUE,
                        GETFILE_TitleText, "Select Folder to Scan",
                        GETFILE_Drawer, tool_data->folder_path_buffer,
                        GETFILE_DrawersOnly, TRUE,
                        GETFILE_ReadOnly, FALSE,
                    TAG_END),
                    CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                        LABEL_Text, "Folder",
                    TAG_END),
                    CHILD_WeightedWidth, 75,
                    
                    LAYOUT_AddChild, tool_data->scan_btn_obj = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_TOOL_SCAN_BUTTON,
                        GA_Text, "Scan",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                    TAG_END),
                    CHILD_WeightedWidth, 25,
                TAG_END),
                CHILD_WeightedHeight, 0,
                
                /* Filter chooser */
                LAYOUT_AddChild, tool_data->filter_chooser_obj = NewObject(CHOOSER_GetClass(), NULL,
                    GA_ID, GID_TOOL_FILTER_CHOOSER,
                    GA_RelVerify, TRUE,
                    GA_TabCycle, TRUE,
                    GA_Disabled, TRUE,  /* Disabled until data loaded */
                    CHOOSER_PopUp, TRUE,
                    CHOOSER_Selected, 0,
                    CHOOSER_Labels, tool_data->filter_labels,
                TAG_END),
                CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                    LABEL_Text, "Filter",
                TAG_END),
                CHILD_WeightedHeight, 0,
                
                /* Tool list ListBrowser (with real columns!) */
                LAYOUT_AddChild, tool_data->tool_listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
                    GA_ID, GID_TOOL_LIST_LISTBROWSER,
                    GA_RelVerify, TRUE,
                    GA_TabCycle, TRUE,
                    LISTBROWSER_ColumnInfo, tool_list_column_info,
                    LISTBROWSER_ColumnTitles, TRUE,
                    LISTBROWSER_ShowSelected, TRUE,
                    LISTBROWSER_AutoFit, TRUE,
                    LISTBROWSER_TitleClickable, TRUE,
                    LISTBROWSER_SortColumn, 0,
                TAG_END),
                CHILD_WeightedHeight, 45,
                
                /* Details ListBrowser */
                LAYOUT_AddChild, tool_data->details_listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
                    GA_ID, GID_TOOL_DETAILS_LISTBROWSER,
                    GA_RelVerify, TRUE,
                    GA_TabCycle, TRUE,
                    LISTBROWSER_ColumnInfo, details_column_info,
                    LISTBROWSER_ColumnTitles, FALSE,
                    LISTBROWSER_ShowSelected, TRUE,
                    LISTBROWSER_AutoFit, TRUE,
                TAG_END),
                CHILD_WeightedHeight, 35,
                
                /* Replace buttons row */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_TopSpacing, 4,
                    LAYOUT_EvenSize, TRUE,
                    
                    LAYOUT_AddChild, tool_data->replace_batch_btn_obj = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_TOOL_REPLACE_BATCH_BUTTON,
                        GA_Text, "Replace Tool (Batch)",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        GA_Disabled, TRUE,
                    TAG_END),
                    
                    LAYOUT_AddChild, tool_data->replace_single_btn_obj = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_TOOL_REPLACE_SINGLE_BUTTON,
                        GA_Text, "Replace Tool (Single)",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        GA_Disabled, TRUE,
                    TAG_END),
                TAG_END),
                CHILD_WeightedHeight, 0,
                
                /* Bottom buttons row */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_TopSpacing, 4,
                    
                    LAYOUT_AddChild, tool_data->restore_tools_btn_obj = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_TOOL_RESTORE_TOOLS_BUTTON,
                        GA_Text, "Restore Default Tools Backups...",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                    TAG_END),
                    CHILD_WeightedWidth, 70,
                    
                    LAYOUT_AddChild, tool_data->close_btn_obj = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_TOOL_CLOSE_BUTTON,
                        GA_Text, "Close",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                    TAG_END),
                    CHILD_WeightedWidth, 30,
                TAG_END),
                CHILD_WeightedHeight, 0,
                
            TAG_END), /* End root layout */
        TAG_END), /* End parent group */
    TAG_END);
    
    if (tool_data->window_obj == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to create tool cache window object\n");
        free_chooser_labels(tool_data->filter_labels);
        tool_data->filter_labels = NULL;
        UnlockPubScreen(NULL, screen);
        close_reaction_classes();
        return FALSE;
    }
    
    /* Open the window */
    tool_data->window = (struct Window *)RA_OpenWindow(tool_data->window_obj);
    if (tool_data->window == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to open tool cache window\n");
        DisposeObject(tool_data->window_obj);
        tool_data->window_obj = NULL;
        free_chooser_labels(tool_data->filter_labels);
        tool_data->filter_labels = NULL;
        UnlockPubScreen(NULL, screen);
        close_reaction_classes();
        return FALSE;
    }
    
    tool_data->window_open = TRUE;
    log_info(LOG_GUI, "Tool cache window opened successfully\n");
    
    /* Attach menu strip to window */
    if (menu_strip)
    {
        SetMenuStrip(tool_data->window, menu_strip);
        log_info(LOG_GUI, "Menu strip attached to tool cache window\n");
    }
    
    /* Set LISTBROWSER_TitleClickable after window is opened */
    if (tool_data->tool_listbrowser_obj && tool_data->window)
    {
        log_debug(LOG_GUI, "*** Setting LISTBROWSER_TitleClickable to TRUE ***\n");
        SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj,
                       tool_data->window, NULL,
                       LISTBROWSER_TitleClickable, TRUE,
                       TAG_DONE);
    }
    
    /* Debug: Verify listbrowser TitleClickable attribute */
    if (tool_data->tool_listbrowser_obj)
    {
        ULONG clickable = 0;
        ULONG sort_col = 0;
        GetAttr(LISTBROWSER_TitleClickable, tool_data->tool_listbrowser_obj, &clickable);
        GetAttr(LISTBROWSER_SortColumn, tool_data->tool_listbrowser_obj, &sort_col);
        log_debug(LOG_GUI, "*** LISTBROWSER: TitleClickable=%lu, SortColumn=%lu ***\n", clickable, sort_col);
    }
    
    /* Build display list from global cache (if any) */
    if (build_tool_cache_display_list(tool_data))
    {
        apply_tool_filter(tool_data);
        populate_tool_listbrowser(tool_data);
    }
    
    /* Initialize details panel */
    update_tool_details(tool_data, -1);
    
    /* Update button states */
    update_button_states(tool_data);
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Close Tool Cache Window                                                */
/*------------------------------------------------------------------------*/
void close_tool_cache_window(struct iTidyToolCacheWindow *tool_data)
{
    if (tool_data == NULL)
        return;
    
    log_info(LOG_GUI, "close_tool_cache_window: Starting cleanup\n");
    
    /* Clear menu strip before closing window */
    if (menu_strip && tool_data->window)
    {
        ClearMenuStrip(tool_data->window);
        log_info(LOG_GUI, "Menu strip cleared from tool cache window\n");
    }
    
    /* Detach lists before disposing */
    if (tool_data->window != NULL)
    {
        if (tool_data->tool_listbrowser_obj != NULL)
        {
            SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj, 
                           tool_data->window, NULL,
                           LISTBROWSER_Labels, ~0,
                           TAG_DONE);
        }
        
        if (tool_data->details_listbrowser_obj != NULL)
        {
            SetGadgetAttrs((struct Gadget *)tool_data->details_listbrowser_obj, 
                           tool_data->window, NULL,
                           LISTBROWSER_Labels, ~0,
                           TAG_DONE);
        }
        
        if (tool_data->filter_chooser_obj != NULL)
        {
            SetGadgetAttrs((struct Gadget *)tool_data->filter_chooser_obj,
                           tool_data->window, NULL,
                           CHOOSER_Labels, ~0,
                           TAG_DONE);
        }
    }
    
    /* Dispose window object (automatically frees child gadgets) */
    if (tool_data->window_obj != NULL)
    {
        DisposeObject(tool_data->window_obj);
        tool_data->window_obj = NULL;
        tool_data->window = NULL;
        tool_data->folder_getfile_obj = NULL;
        tool_data->scan_btn_obj = NULL;
        tool_data->filter_chooser_obj = NULL;
        tool_data->tool_listbrowser_obj = NULL;
        tool_data->details_listbrowser_obj = NULL;
        tool_data->replace_batch_btn_obj = NULL;
        tool_data->replace_single_btn_obj = NULL;
        tool_data->restore_tools_btn_obj = NULL;
        tool_data->close_btn_obj = NULL;
    }
    
    /* Free ListBrowser lists */
    if (tool_data->tool_list_nodes != NULL)
    {
        free_listbrowser_list(tool_data->tool_list_nodes);
        tool_data->tool_list_nodes = NULL;
    }
    
    if (tool_data->details_list_nodes != NULL)
    {
        free_listbrowser_list(tool_data->details_list_nodes);
        tool_data->details_list_nodes = NULL;
    }
    
    /* Free chooser labels */
    if (tool_data->filter_labels != NULL)
    {
        free_chooser_labels(tool_data->filter_labels);
        tool_data->filter_labels = NULL;
    }
    
    /* Free tool entries */
    free_tool_cache_entries(tool_data);
    
    /* Free column info */
    if (tool_list_column_info != NULL)
    {
        FreeLBColumnInfo(tool_list_column_info);
        tool_list_column_info = NULL;
    }
    
    /* Unlock screen */
    if (tool_data->screen != NULL)
    {
        UnlockPubScreen(NULL, tool_data->screen);
        tool_data->screen = NULL;
    }
    
    /* Close ReAction classes */
    close_reaction_classes();
    
    /* Cleanup menu system */
    cleanup_tool_cache_menus();
    
    tool_data->window_open = FALSE;
    
    log_info(LOG_GUI, "Tool cache window closed successfully\n");
}

/*------------------------------------------------------------------------*/
/* Handle Tool Cache Window Events                                        */
/*------------------------------------------------------------------------*/
BOOL handle_tool_cache_window_events(struct iTidyToolCacheWindow *tool_data)
{
    ULONG signals, signal_mask;
    ULONG result;
    UWORD code;
    BOOL continue_running = TRUE;
    
    if (tool_data == NULL || tool_data->window_obj == NULL)
        return FALSE;

    /* Get the ReAction window's signal mask */
    GetAttr(WINDOW_SigMask, tool_data->window_obj, &signal_mask);

    /* Wait for events */
    signals = Wait(signal_mask | SIGBREAKF_CTRL_C);

    if (signals & SIGBREAKF_CTRL_C)
    {
        log_debug(LOG_GUI, "Ctrl+C detected, closing window.\n");
        return FALSE;
    }

    /* Handle window events */
    while ((result = RA_HandleInput(tool_data->window_obj, &code)) != WMHI_LASTMSG)
    {
        /* Log ALL events received */
        log_debug(LOG_GUI, "==> RAW EVENT: result=0x%08lx, CLASSMASK=0x%08lx, GADGETMASK=0x%08lx, code=%u\n",
                  result, (result & WMHI_CLASSMASK), (result & WMHI_GADGETMASK), code);
        log_debug(LOG_GUI, "    WMHI_CLOSEWINDOW=0x%08lx, WMHI_GADGETUP=0x%08lx, WMHI_GADGETDOWN=0x%08lx\n",
                  WMHI_CLOSEWINDOW, WMHI_GADGETUP, WMHI_GADGETDOWN);
        
        switch (result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                log_debug(LOG_GUI, "Close gadget clicked\n");
                continue_running = FALSE;
                break;
            
            case WMHI_MENUPICK:
                if (!handle_tool_cache_menu_selection(code, tool_data))
                {
                    log_debug(LOG_GUI, "Menu requested window close\n");
                    continue_running = FALSE;
                }
                break;
            
            case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                    case GID_TOOL_FOLDER_GETFILE:
                        {
                            /* Manually invoke the file requester - getfile.gadget does NOT auto-open */
                            if (DoMethod((Object *)tool_data->folder_getfile_obj,
                                         GFILE_REQUEST, tool_data->window))
                            {
                                /* Requester closed with selection - get the path */
                                char *folder_path = NULL;
                                GetAttr(GETFILE_Drawer, tool_data->folder_getfile_obj, (ULONG *)&folder_path);
                                if (folder_path && folder_path[0])
                                {
                                    strncpy(tool_data->folder_path_buffer, folder_path, 
                                           sizeof(tool_data->folder_path_buffer) - 1);
                                    tool_data->folder_path_buffer[sizeof(tool_data->folder_path_buffer) - 1] = '\0';
                                    log_debug(LOG_GUI, "Folder changed to: %s\n", tool_data->folder_path_buffer);
                                }
                            }
                        }
                        break;
                    
                    case GID_TOOL_SCAN_BUTTON:
                        handle_scan_button(tool_data);
                        break;
                    
                    case GID_TOOL_FILTER_CHOOSER:
                        {
                            ULONG selected = 0;
                            GetAttr(CHOOSER_Selected, tool_data->filter_chooser_obj, &selected);
                            
                            tool_data->current_filter = (ToolFilterType)selected;
                            apply_tool_filter(tool_data);
                            populate_tool_listbrowser(tool_data);
                            tool_data->selected_index = -1;
                            update_tool_details(tool_data, -1);
                            
                            log_debug(LOG_GUI, "Filter changed to: %lu\n", selected);
                        }
                        break;
                    
                    case GID_TOOL_LIST_LISTBROWSER:
                        {
                            ULONG selected = ~0;
                            ULONG rel_event = LBRE_NORMAL;
                            
                            GetAttr(LISTBROWSER_Selected, tool_data->tool_listbrowser_obj, &selected);
                            GetAttr(LISTBROWSER_RelEvent, tool_data->tool_listbrowser_obj, &rel_event);
                            
                            /* Handle column title click for sorting */
                            if (rel_event == LBRE_TITLECLICK)
                            {
                                ULONG sort_column = code;
                                ULONG direction;
                                
                                /* Toggle direction if same column, otherwise use forward */
                                if (sort_column == tool_data->last_sort_column)
                                {
                                    direction = (tool_data->sort_direction == LBMSORT_FORWARD) 
                                                ? LBMSORT_REVERSE : LBMSORT_FORWARD;
                                }
                                else
                                {
                                    direction = LBMSORT_FORWARD;
                                }
                                
                                /* Store sort state */
                                tool_data->last_sort_column = sort_column;
                                tool_data->sort_direction = direction;
                                
                                /* Invoke sort method */
                                DoGadgetMethod((struct Gadget *)tool_data->tool_listbrowser_obj,
                                              tool_data->window, NULL,
                                              LBM_SORT, tool_data->tool_list_nodes, sort_column, direction, NULL);
                                
                                /* Update display to show sort arrow */
                                SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj,
                                               tool_data->window, NULL,
                                               LISTBROWSER_SortColumn, sort_column,
                                               TAG_DONE);
                                
                                RefreshGadgets((struct Gadget *)tool_data->tool_listbrowser_obj,
                                              tool_data->window, NULL);
                            }
                            else if (rel_event == LBRE_COLUMNADJUST)
                            {
                                /* Column resizing - handled automatically by gadget, no action needed */
                            }
                            else if (selected != ~0)
                            {
                                log_debug(LOG_GUI, "Tool selected: %lu (event: %lu)\n", selected, rel_event);
                                tool_data->selected_index = (LONG)selected;
                                update_tool_details(tool_data, (LONG)selected);
                            }
                            else
                            {
                                log_debug(LOG_GUI, "Unhandled event type: %lu\n", rel_event);
                            }
                        }
                        break;
                    
                    case GID_TOOL_DETAILS_LISTBROWSER:
                        {
                            ULONG selected = ~0;
                            GetAttr(LISTBROWSER_Selected, tool_data->details_listbrowser_obj, &selected);
                            
                            if (selected != ~0)
                            {
                                tool_data->selected_details_index = (LONG)selected;
                                update_button_states(tool_data);
                                log_debug(LOG_GUI, "Details item selected: %lu\n", selected);
                            }
                        }
                        break;
                    
                    case GID_TOOL_REPLACE_BATCH_BUTTON:
                        handle_replace_batch_button(tool_data);
                        break;
                    
                    case GID_TOOL_REPLACE_SINGLE_BUTTON:
                        handle_replace_single_button(tool_data);
                        break;
                    
                    case GID_TOOL_RESTORE_TOOLS_BUTTON:
                        handle_restore_tools_button(tool_data);
                        break;
                    
                    case GID_TOOL_CLOSE_BUTTON:
                        log_debug(LOG_GUI, "Close button clicked\n");
                        continue_running = FALSE;
                        break;
                }
                break;
        }
    }
    
    return continue_running;
}

/*------------------------------------------------------------------------*/
/* Refresh Tool Cache Window Display                                      */
/*------------------------------------------------------------------------*/
void refresh_tool_cache_window(struct iTidyToolCacheWindow *tool_data)
{
    if (!tool_data || !tool_data->window)
        return;
    
    log_info(LOG_GUI, "refresh_tool_cache_window: Refreshing display from updated cache\n");
    
    /* Detach lists */
    if (tool_data->tool_listbrowser_obj)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj,
                       tool_data->window, NULL,
                       LISTBROWSER_Labels, ~0,
                       TAG_DONE);
    }
    
    if (tool_data->details_listbrowser_obj)
    {
        SetGadgetAttrs((struct Gadget *)tool_data->details_listbrowser_obj,
                       tool_data->window, NULL,
                       LISTBROWSER_Labels, ~0,
                       TAG_DONE);
    }
    
    /* Free existing entries */
    free_tool_cache_entries(tool_data);
    
    /* Free existing listbrowser nodes */
    if (tool_data->tool_list_nodes)
    {
        free_listbrowser_list(tool_data->tool_list_nodes);
        tool_data->tool_list_nodes = NULL;
    }
    
    if (tool_data->details_list_nodes)
    {
        free_listbrowser_list(tool_data->details_list_nodes);
        tool_data->details_list_nodes = NULL;
    }
    
    /* Rebuild from current cache */
    if (build_tool_cache_display_list(tool_data))
    {
        apply_tool_filter(tool_data);
        populate_tool_listbrowser(tool_data);
        
        /* Auto-select last item if list is not empty */
        if (tool_data->total_count > 0)
        {
            ULONG last_index = 0;
            struct Node *node;
            
            /* Count filtered entries */
            for (node = tool_data->filtered_entries.lh_Head; 
                 node->ln_Succ != NULL; 
                 node = node->ln_Succ)
            {
                last_index++;
            }
            
            if (last_index > 0)
            {
                last_index--;
                
                SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj,
                               tool_data->window, NULL,
                               LISTBROWSER_Selected, last_index,
                               LISTBROWSER_MakeVisible, last_index,
                               TAG_DONE);
                
                tool_data->selected_index = (LONG)last_index;
                update_tool_details(tool_data, (LONG)last_index);
            }
        }
        else
        {
            tool_data->selected_index = -1;
            tool_data->selected_details_index = -1;
            update_tool_details(tool_data, -1);
        }
        
        update_button_states(tool_data);
        
        log_info(LOG_GUI, "refresh_tool_cache_window: Display refreshed successfully\n");
    }
    else
    {
        log_error(LOG_GUI, "refresh_tool_cache_window: Failed to rebuild display list\n");
    }
}

