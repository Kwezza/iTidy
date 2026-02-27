/*
 * main_window.c - iTidy Main Window Implementation
 * ReAction GUI for Workbench 3.2+
 * 
 * This is the ReAction version (v2.0+) of the main window.
 * For the legacy GadTools version, see git tag v1.0-gadtools
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/asl.h>
#include <workbench/workbench.h>
#include <clib/wb_protos.h>

/* ReAction headers */
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/checkbox.h>
#include <proto/chooser.h>
#include <proto/getfile.h>
#include <proto/label.h>
#include <proto/requester.h>

#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
#include <classes/window.h>
#include <classes/requester.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/checkbox.h>
#include <gadgets/chooser.h>
#include <gadgets/getfile.h>
#include <images/label.h>

#include <string.h>
#include <stdio.h>

/* Console output abstraction */
#include <console_output.h>

#include "main_window.h"
#include "main_window_reaction.h"
#include "version_info.h"
#include "advanced_window.h"
#include "RestoreBackups/restore_window.h"
#include "DefaultTools/tool_cache_window.h"
#include "deficons/deficons_settings_window.h"
#include "deficons/deficons_creation_window.h"
#include "deficons/text_templates_window.h"
#include "DefaultTools/default_tool_backup.h"
#include "exclude_paths_window.h"
#include "layout_preferences.h"
#include "layout_processor.h"
#include "writeLog.h"
#include "gui_utilities.h"
#include "../utilities.h"
#include "StatusWindows/main_progress_window.h"
#include "../backups/backup_lha.h"
#include "../icon_types.h"  /* For ToolCacheEntry and g_ToolCache extern */

/*------------------------------------------------------------------------*/
/* External References                                                    */
/*------------------------------------------------------------------------*/

/* ToolType settings structure (defined in main_gui.c) */
typedef struct ToolTypeSettings_tag {
    BOOL tooltypes_loaded;
    UWORD debug_level;
    BOOL debug_level_set;
    char loadprefs_path[256];
    BOOL loadprefs_set;
} ToolTypeSettings;

extern ToolTypeSettings g_tooltypes;

/*------------------------------------------------------------------------*/
/* ReAction Library Bases                                                */
/*------------------------------------------------------------------------*/
struct Library *WindowBase = NULL;
struct Library *LayoutBase = NULL;
struct Library *ButtonBase = NULL;
struct Library *CheckBoxBase = NULL;
struct Library *ChooserBase = NULL;
struct Library *GetFileBase = NULL;
struct Library *LabelBase = NULL;
struct Library *RequesterBase = NULL;

/* Local GadToolsBase for menus - prefixed to avoid collision with system global */
static struct Library *iTidy_GadToolsBase = NULL;

/* Default Tool Restore window - old API (no clean open_ function exported from header) */
extern struct Window *iTidy_CreateToolRestoreWindow(struct Screen *screen, APTR backup_manager);
extern BOOL iTidy_HandleToolRestoreWindowEvent(struct Window *window, struct IntuiMessage *msg);
extern BOOL iTidy_WasRestorePerformed(void);
extern void iTidy_CloseToolRestoreWindow(struct Window *window);

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define ITIDY_WINDOW_TITLE "iTidy v" ITIDY_VERSION " - Icon Cleanup Tool"

/*------------------------------------------------------------------------*/
/* Chooser Label String Arrays                                           */
/*------------------------------------------------------------------------*/
static STRPTR order_labels_str[] = {
    "Folders First",
    "Files First",
    "Mixed",
    "Grouped By Type",
    NULL
};

static STRPTR sortby_labels_str[] = {
    "Name",
    "Type",
    "Date",
    "Size",
    NULL
};

static STRPTR position_labels_str[] = {
    "Center Screen",
    "Keep Position",
    "Near Parent",
    "No Change",
    NULL
};

/*------------------------------------------------------------------------*/
/* Menu Template                                                          */
/*------------------------------------------------------------------------*/
static struct NewMenu main_window_menu_template[] =
{
    /* Presets */
    { NM_TITLE, "Presets",                         NULL, 0, 0, NULL },
    { NM_ITEM,  "Reset To Defaults",               "N",  0, 0, (APTR)MENU_PRESETS_RESET },
    { NM_ITEM,  NM_BARLABEL,                       NULL, 0, 0, NULL },
    { NM_ITEM,  "Open Preset...",                  "O",  0, 0, (APTR)MENU_PRESETS_OPEN },
    { NM_ITEM,  NM_BARLABEL,                       NULL, 0, 0, NULL },
    { NM_ITEM,  "Save Preset",                     "S",  0, 0, (APTR)MENU_PRESETS_SAVE },
    { NM_ITEM,  "Save Preset As...",               "A",  0, 0, (APTR)MENU_PRESETS_SAVE_AS },
    { NM_ITEM,  NM_BARLABEL,                       NULL, 0, 0, NULL },
    { NM_ITEM,  "Quit",                            "Q",  0, 0, (APTR)MENU_PRESETS_QUIT },

    /* Settings */
    { NM_TITLE, "Settings",                        NULL, 0, 0, NULL },
    { NM_ITEM,  "Advanced Settings...",            NULL, 0, 0, (APTR)MENU_SETTINGS_ADVANCED },
    { NM_ITEM,  "DefIcons Categories...",          NULL, 0, 0, NULL },
    { NM_SUB,   "DefIcons Categories...",          NULL, 0, 0, (APTR)MENU_SETTINGS_DEFI_CATS },
    { NM_SUB,   "Preview Icons...",               NULL, 0, 0, (APTR)MENU_SETTINGS_DEFI_PREVIEW },
    { NM_SUB,   "DefIcons Excluded Folders...",   NULL, 0, 0, (APTR)MENU_SETTINGS_DEFI_EXCLUDE },
    { NM_ITEM,  "Backups...",                      NULL, 0, 0, NULL },
    { NM_SUB,   "Back Up Layouts Before Changes",  NULL, CHECKIT | MENUTOGGLE, 0, (APTR)MENU_SETTINGS_BACKUP_TOGGLE },
    { NM_ITEM,  "Logging",                         NULL, 0, 0, NULL },
    { NM_SUB,   "Disabled (Recommended)",          NULL, CHECKIT | CHECKED, 30, (APTR)MENU_SETTINGS_LOG_DISABLED },
    { NM_SUB,   "Debug",                           NULL, CHECKIT, 29, (APTR)MENU_SETTINGS_LOG_DEBUG },
    { NM_SUB,   "Info",                            NULL, CHECKIT, 27, (APTR)MENU_SETTINGS_LOG_INFO },
    { NM_SUB,   "Warning",                         NULL, CHECKIT, 23, (APTR)MENU_SETTINGS_LOG_WARNING },
    { NM_SUB,   "Error",                           NULL, CHECKIT, 15, (APTR)MENU_SETTINGS_LOG_ERROR },
    { NM_SUB,   NM_BARLABEL,                       NULL, 0, 0, NULL },
    { NM_SUB,   "Open Log Folder",                NULL, 0, 0, (APTR)MENU_SETTINGS_LOG_FOLDER },

    /* Tools */
    { NM_TITLE, "Tools",                           NULL, 0, 0, NULL },
    { NM_ITEM,  "Restore...",                      NULL, 0, 0, NULL },
    { NM_SUB,   "Restore Layouts...",              NULL, 0, 0, (APTR)MENU_TOOLS_RESTORE_LAYOUTS },
    { NM_SUB,   "Restore Default Tools...",        NULL, 0, 0, (APTR)MENU_TOOLS_RESTORE_DEFTOOLS },

    /* Help */
    { NM_TITLE, "Help",                            NULL, 0, 0, NULL },
    { NM_ITEM,  "iTidy Guide...",                  NULL, 0, 0, (APTR)MENU_HELP_GUIDE },
    { NM_ITEM,  NM_BARLABEL,                       NULL, 0, 0, NULL },
    { NM_ITEM,  "About",                           NULL, 0, 0, (APTR)MENU_HELP_ABOUT },

    { NM_END,   NULL,                              NULL, 0, 0, NULL }
};

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static struct List *create_chooser_labels(STRPTR *strings);
static void free_chooser_labels(struct List *list);
static BOOL handle_menu_selection(ULONG menu_number, struct iTidyMainWindow *win_data);
static BOOL handle_gadget_event(ULONG gadget_id, WORD code, struct iTidyMainWindow *win_data);

/* Save/Load Preferences */
static BOOL save_preferences_to_file(const char *filepath, const LayoutPreferences *prefs, const DefIconsExcludePaths *ep);
static BOOL load_preferences_from_file(const char *filepath, LayoutPreferences *prefs, DefIconsExcludePaths *ep);
static void sync_gui_from_preferences(struct iTidyMainWindow *win_data, const LayoutPreferences *prefs);
static void sync_gui_to_preferences(struct iTidyMainWindow *win_data, LayoutPreferences *prefs);

/* Menu Handlers */
static void handle_main_new_menu(struct iTidyMainWindow *win_data);
static void handle_main_open_menu(struct iTidyMainWindow *win_data);
static void handle_main_save_menu(struct iTidyMainWindow *win_data);
static void handle_main_save_as_menu(struct iTidyMainWindow *win_data);
static struct MenuItem *find_menu_item_by_id(struct Menu *menu_strip, ULONG target_id);
static void sync_backup_menu_check(struct iTidyMainWindow *win_data);
void handle_menu_open_log_folder(struct Window *parent);

/* ReAction Requester Helper */
ULONG ShowReActionRequester(struct Window *parent_window,
                                   CONST_STRPTR title,
                                   CONST_STRPTR body,
                                   CONST_STRPTR gadgets,
                                   ULONG image_type);

/*------------------------------------------------------------------------*/
/* ReAction Library Management                                           */
/*------------------------------------------------------------------------*/

/**
 * init_reaction_libs - Open all ReAction class libraries
 */
BOOL init_reaction_libs(void)
{
    /* GadTools is needed for menus */
    iTidy_GadToolsBase = OpenLibrary("gadtools.library", 39L);
    if (!iTidy_GadToolsBase)
    {
        log_error(LOG_GUI, "Failed to open gadtools.library v39\n");
        return FALSE;
    }
    
    /* ReAction window class */
    WindowBase = OpenLibrary("window.class", 0L);
    if (!WindowBase)
    {
        log_error(LOG_GUI, "Failed to open window.class\n");
        CloseLibrary(iTidy_GadToolsBase);
        iTidy_GadToolsBase = NULL;
        return FALSE;
    }
    
    /* Layout gadget */
    LayoutBase = OpenLibrary("gadgets/layout.gadget", 0L);
    if (!LayoutBase)
    {
        log_error(LOG_GUI, "Failed to open gadgets/layout.gadget\n");
        CloseLibrary(WindowBase);
        CloseLibrary(iTidy_GadToolsBase);
        WindowBase = NULL;
        iTidy_GadToolsBase = NULL;
        return FALSE;
    }
    
    /* Button gadget */
    ButtonBase = OpenLibrary("gadgets/button.gadget", 0L);
    if (!ButtonBase)
    {
        log_error(LOG_GUI, "Failed to open gadgets/button.gadget\n");
        CloseLibrary(LayoutBase);
        CloseLibrary(WindowBase);
        CloseLibrary(iTidy_GadToolsBase);
        LayoutBase = NULL;
        WindowBase = NULL;
        iTidy_GadToolsBase = NULL;
        return FALSE;
    }
    
    /* Checkbox gadget */
    CheckBoxBase = OpenLibrary("gadgets/checkbox.gadget", 0L);
    if (!CheckBoxBase)
    {
        log_error(LOG_GUI, "Failed to open gadgets/checkbox.gadget\n");
        CloseLibrary(ButtonBase);
        CloseLibrary(LayoutBase);
        CloseLibrary(WindowBase);
        CloseLibrary(iTidy_GadToolsBase);
        ButtonBase = NULL;
        LayoutBase = NULL;
        WindowBase = NULL;
        iTidy_GadToolsBase = NULL;
        return FALSE;
    }
    
    /* Chooser gadget */
    ChooserBase = OpenLibrary("gadgets/chooser.gadget", 0L);
    if (!ChooserBase)
    {
        log_error(LOG_GUI, "Failed to open gadgets/chooser.gadget\n");
        CloseLibrary(CheckBoxBase);
        CloseLibrary(ButtonBase);
        CloseLibrary(LayoutBase);
        CloseLibrary(WindowBase);
        CloseLibrary(iTidy_GadToolsBase);
        CheckBoxBase = NULL;
        ButtonBase = NULL;
        LayoutBase = NULL;
        WindowBase = NULL;
        iTidy_GadToolsBase = NULL;
        return FALSE;
    }
    
    /* GetFile gadget */
    GetFileBase = OpenLibrary("gadgets/getfile.gadget", 0L);
    if (!GetFileBase)
    {
        log_error(LOG_GUI, "Failed to open gadgets/getfile.gadget\n");
        CloseLibrary(ChooserBase);
        CloseLibrary(CheckBoxBase);
        CloseLibrary(ButtonBase);
        CloseLibrary(LayoutBase);
        CloseLibrary(WindowBase);
        CloseLibrary(iTidy_GadToolsBase);
        ChooserBase = NULL;
        CheckBoxBase = NULL;
        ButtonBase = NULL;
        LayoutBase = NULL;
        WindowBase = NULL;
        iTidy_GadToolsBase = NULL;
        return FALSE;
    }
    
    /* Label image */
    LabelBase = OpenLibrary("images/label.image", 0L);
    if (!LabelBase)
    {
        log_error(LOG_GUI, "Failed to open images/label.image\n");
        CloseLibrary(GetFileBase);
        CloseLibrary(ChooserBase);
        CloseLibrary(CheckBoxBase);
        CloseLibrary(ButtonBase);
        CloseLibrary(LayoutBase);
        CloseLibrary(WindowBase);
        CloseLibrary(iTidy_GadToolsBase);
        GetFileBase = NULL;
        ChooserBase = NULL;
        CheckBoxBase = NULL;
        ButtonBase = NULL;
        LayoutBase = NULL;
        WindowBase = NULL;
        iTidy_GadToolsBase = NULL;
        return FALSE;
    }
    
    /* Requester class */
    RequesterBase = OpenLibrary("requester.class", 0L);
    if (!RequesterBase)
    {
        log_error(LOG_GUI, "Failed to open requester.class\n");
        CloseLibrary(LabelBase);
        CloseLibrary(GetFileBase);
        CloseLibrary(ChooserBase);
        CloseLibrary(CheckBoxBase);
        CloseLibrary(ButtonBase);
        CloseLibrary(LayoutBase);
        CloseLibrary(WindowBase);
        CloseLibrary(iTidy_GadToolsBase);
        LabelBase = NULL;
        GetFileBase = NULL;
        ChooserBase = NULL;
        CheckBoxBase = NULL;
        ButtonBase = NULL;
        LayoutBase = NULL;
        WindowBase = NULL;
        iTidy_GadToolsBase = NULL;
        return FALSE;
    }
    
    log_info(LOG_GUI, "ReAction libraries initialized successfully\n");
    return TRUE;
}

/**
 * cleanup_reaction_libs - Close all ReAction class libraries
 */
void cleanup_reaction_libs(void)
{
    if (RequesterBase) { CloseLibrary(RequesterBase); RequesterBase = NULL; }
    if (LabelBase)     { CloseLibrary(LabelBase);     LabelBase = NULL; }
    if (GetFileBase)   { CloseLibrary(GetFileBase);   GetFileBase = NULL; }
    if (ChooserBase)   { CloseLibrary(ChooserBase);   ChooserBase = NULL; }
    if (CheckBoxBase)  { CloseLibrary(CheckBoxBase);  CheckBoxBase = NULL; }
    if (ButtonBase)    { CloseLibrary(ButtonBase);    ButtonBase = NULL; }
    if (LayoutBase)    { CloseLibrary(LayoutBase);    LayoutBase = NULL; }
    if (WindowBase)    { CloseLibrary(WindowBase);    WindowBase = NULL; }
    if (iTidy_GadToolsBase)  { CloseLibrary(iTidy_GadToolsBase);  iTidy_GadToolsBase = NULL; }
    
    log_info(LOG_GUI, "ReAction libraries cleaned up\n");
}

/*------------------------------------------------------------------------*/
/* Chooser Label Helper Functions                                        */
/*------------------------------------------------------------------------*/

/**
 * create_chooser_labels - Create a Chooser label list from string array
 */
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

/**
 * free_chooser_labels - Free a Chooser label list
 */
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
/* ReAction Requester Helper                                             */
/*------------------------------------------------------------------------*/

/**
 * ShowReActionRequester - Show a RequesterClass dialog on the parent window's screen
 *
 * Returns the button number selected (1 = first button, 0 = last/cancel button).
 */
ULONG ShowReActionRequester(struct Window *parent_window,
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
        log_error(LOG_GUI, "ShowReActionRequester: RequesterBase is NULL\n");
        return 0;
    }

    if (!parent_window)
    {
        log_error(LOG_GUI, "ShowReActionRequester: parent_window is NULL\n");
        return 0;
    }

    req_obj = NewObject(REQUESTER_GetClass(), NULL,
        REQ_Type,      REQTYPE_INFO,
        REQ_TitleText, title,
        REQ_BodyText,  body,
        REQ_GadgetText, gadgets,
        REQ_Image,     image_type,
        TAG_DONE);

    if (req_obj)
    {
        req_msg.MethodID  = RM_OPENREQ;
        req_msg.or_Attrs  = NULL;
        req_msg.or_Window = parent_window;
        req_msg.or_Screen = NULL;

        result = DoMethodA(req_obj, (Msg)&req_msg);
        DisposeObject(req_obj);
    }
    else
    {
        log_error(LOG_GUI, "ShowReActionRequester: Failed to create requester object\n");
    }

    return result;
}

/*------------------------------------------------------------------------*/
/* Window Open/Close Functions                                           */
/*------------------------------------------------------------------------*/

/**
 * open_itidy_main_window - Create and open the main iTidy window
 */
BOOL open_itidy_main_window(struct iTidyMainWindow *win_data)
{
    struct Screen *screen = NULL;
    struct DrawInfo *draw_info = NULL;
    
    /* Clear the structure */
    memset(win_data, 0, sizeof(struct iTidyMainWindow));
    
    /* Initialize default values */
    win_data->order_selected = 0;
    win_data->sortby_selected = 0;
    win_data->recursive_subdirs = FALSE;
    win_data->enable_backup = FALSE;
    win_data->enable_deficons_icon_creation = FALSE;
    win_data->window_position_selected = 0;
    strcpy(win_data->folder_path_buffer, "SYS:");
    
    /* Lock Workbench screen */
    screen = LockPubScreen("Workbench");
    if (!screen)
    {
        log_error(LOG_GUI, "Failed to lock Workbench screen\n");
        return FALSE;
    }
    win_data->screen = screen;
    
    /* Get visual info for menus */
    win_data->visual_info = GetVisualInfo(screen, TAG_END);
    if (!win_data->visual_info)
    {
        log_error(LOG_GUI, "Failed to get visual info\n");
        UnlockPubScreen(NULL, screen);
        win_data->screen = NULL;
        return FALSE;
    }
    
    /* Create application message port */
    win_data->app_port = CreateMsgPort();
    if (!win_data->app_port)
    {
        log_error(LOG_GUI, "Failed to create message port\n");
        FreeVisualInfo(win_data->visual_info);
        UnlockPubScreen(NULL, screen);
        win_data->visual_info = NULL;
        win_data->screen = NULL;
        return FALSE;
    }
    
    /* Create Chooser label lists */
    win_data->order_labels = create_chooser_labels(order_labels_str);
    win_data->sortby_labels = create_chooser_labels(sortby_labels_str);
    win_data->position_labels = create_chooser_labels(position_labels_str);
    
    if (!win_data->order_labels || !win_data->sortby_labels || !win_data->position_labels)
    {
        log_error(LOG_GUI, "Failed to create chooser labels\n");
        if (win_data->order_labels) free_chooser_labels(win_data->order_labels);
        if (win_data->sortby_labels) free_chooser_labels(win_data->sortby_labels);
        if (win_data->position_labels) free_chooser_labels(win_data->position_labels);
        DeleteMsgPort(win_data->app_port);
        FreeVisualInfo(win_data->visual_info);
        UnlockPubScreen(NULL, screen);
        win_data->order_labels = NULL;
        win_data->sortby_labels = NULL;
        win_data->position_labels = NULL;
        win_data->app_port = NULL;
        win_data->visual_info = NULL;
        win_data->screen = NULL;
        return FALSE;
    }
    
    /* Create menus */
    win_data->menu_strip = CreateMenus(main_window_menu_template, TAG_END);
    if (win_data->menu_strip)
    {
        LayoutMenus(win_data->menu_strip, win_data->visual_info,
            GTMN_NewLookMenus, TRUE,
            TAG_END);
    }
    
    /* Define hint info for gadget help (static so it persists) */
    static struct HintInfo hintInfo[] =
    {
        {ITIDY_GAID_MASTER_LAYOUT, -1, "", 0},
        {ITIDY_GAID_FOLDER_LAYOUT, -1, "", 0},
        {ITIDY_GAID_FOLDER_GETFILE, -1, "Select the folder you want to tidy. To include subfolders, enable \"Include Subfolders\" below.", 0},
        {ITIDY_GAID_OPTIONS_LAYOUT, -1, "", 0},
        {ITIDY_GAID_LEFT_COLUMN, -1, "", 0},
        {ITIDY_GAID_ORDER_CHOOSER, -1, "Sets how icons are grouped before sorting. Folders first, files first, mixed, or grouped by type.", 0},
        {ITIDY_GAID_RECURSIVE_CHECKBOX, -1, "When enabled, iTidy also processes all subfolders under the selected folder.", 0},
        {ITIDY_GAID_POSITION_CHOOSER, -1, "Controls where drawer windows are placed after resizing. (Only affects windows iTidy changes.)", 0},
        {ITIDY_GAID_RIGHT_COLUMN, -1, "", 0},
        {ITIDY_GAID_SORTBY_CHOOSER, -1, "Selects the field used for sorting: name, kind, date, or size.", 0},
        {ITIDY_GAID_BACKUP_CHECKBOX, -1, "Creates an LhA backup of the folder's .info files before changes. Requires LhA in C:", 0},
        {ITIDY_GAID_CREATE_NEW_ICONS, -1, "", 0},
        {ITIDY_GAID_TOOLS_LAYOUT, -1, "", 0},
        {ITIDY_GAID_ADVANCED_BUTTON, -1, "Opens Advanced Settings for finer control over layout and sizing.", 0},
        {ITIDY_GAID_DEFAULT_TOOLS_BUTTON, -1, "Scans icons for missing or invalid Default Tools. Lets you fix them or batch-replace one tool with another.", 0},
        {ITIDY_GAID_RESTORE_BUTTON, -1, "Restores icon positions and window snapshots from iTidy backups. Only available if you previously enabled backups.", 0},
        {ITIDY_GAID_ICON_CREATION_BUTTON, -1, "Opens the icon creation settings (thumbnails, text previews, folder icons)", 0},
        {ITIDY_GAID_BUTTONS_LAYOUT, -1, "", 0},
        {ITIDY_GAID_START_BUTTON, -1, "Starts tidying the selected folder using the current settings.", 0},
        {ITIDY_GAID_EXIT_BUTTON, -1, "Closes iTidy.", 0},
        {-1, -1, NULL, 0}
    };
    
    /* Create the ReAction window object */
    win_data->window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title, ITIDY_WINDOW_TITLE,
        WA_Left, 50,
        WA_Top, 30,
        WA_Width, 20,
        WA_Height, 20,
        WA_MinWidth, 350,
        WA_MinHeight, 200,
        WA_MaxWidth, 8192,
        WA_MaxHeight, 8192,
        WA_PubScreen, screen,
        WA_CloseGadget, TRUE,
        WA_DepthGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_DragBar, TRUE,
        WA_Activate, TRUE,
        WA_NoCareRefresh, TRUE,
        WINDOW_HintInfo, hintInfo,
        WINDOW_AppPort, win_data->app_port,
        WINDOW_GadgetHelp, TRUE,
        WINDOW_IconTitle, "iTidy",
        WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | 
                  IDCMP_MENUPICK | IDCMP_MENUHELP | IDCMP_NEWSIZE | IDCMP_IDCMPUPDATE,
        
        WINDOW_ParentGroup, NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_DeferLayout, TRUE,
            
            /* Master layout container */
            LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_MASTER_LAYOUT] = 
                NewObject(LAYOUT_GetClass(), NULL,
                GA_ID, ITIDY_GAID_MASTER_LAYOUT,
                LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                LAYOUT_LeftSpacing, 2,
                LAYOUT_RightSpacing, 2,
                LAYOUT_TopSpacing, 2,
                LAYOUT_BottomSpacing, 2,
                
                /* Folder selection group */
                LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_FOLDER_LAYOUT] = 
                    NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, ITIDY_GAID_FOLDER_LAYOUT,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_BevelStyle, BVS_THIN,
                    LAYOUT_LeftSpacing, 2,
                    LAYOUT_RightSpacing, 2,
                    LAYOUT_TopSpacing, 3,
                    LAYOUT_BottomSpacing, 4,
                    
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_FOLDER_GETFILE] = 
                        NewObject(GETFILE_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_FOLDER_GETFILE,
                        GA_RelVerify, TRUE,
                        GETFILE_TitleText, "Select folder to tidy",
                        GETFILE_Drawer, win_data->folder_path_buffer,
                        GETFILE_DoSaveMode, FALSE,
                        GETFILE_DrawersOnly, TRUE,
                        GETFILE_ReadOnly, TRUE,
                    TAG_END),
                    CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                        LABEL_Text, "Folder to clean:",
                    TAG_END),
                TAG_END),
                
                /* Options group */
                LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_OPTIONS_LAYOUT] = 
                    NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, ITIDY_GAID_OPTIONS_LAYOUT,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_BevelStyle, BVS_THIN,
                    LAYOUT_LeftSpacing, 2,
                    LAYOUT_RightSpacing, 2,
                    LAYOUT_TopSpacing, 2,
                    LAYOUT_BottomSpacing, 4,
                    
                    /* Left column */
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_LEFT_COLUMN] = 
                        NewObject(LAYOUT_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_LEFT_COLUMN,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_RightSpacing, 2,
                        
                        /* Order chooser */
                        LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_ORDER_CHOOSER] = 
                            NewObject(CHOOSER_GetClass(), NULL,
                            GA_ID, ITIDY_GAID_ORDER_CHOOSER,
                            GA_RelVerify, TRUE,
                            GA_TabCycle, TRUE,
                            CHOOSER_PopUp, TRUE,
                            CHOOSER_Selected, win_data->order_selected,
                            CHOOSER_Labels, win_data->order_labels,
                        TAG_END),
                        CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, "Grouping",
                        TAG_END),
                        
                        /* Sort by chooser */
                        LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_SORTBY_CHOOSER] = 
                            NewObject(CHOOSER_GetClass(), NULL,
                            GA_ID, ITIDY_GAID_SORTBY_CHOOSER,
                            GA_RelVerify, TRUE,
                            GA_TabCycle, TRUE,
                            CHOOSER_PopUp, TRUE,
                            CHOOSER_Selected, win_data->sortby_selected,
                            CHOOSER_Labels, win_data->sortby_labels,
                        TAG_END),
                        CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, "Sort By",
                        TAG_END),
                        
                        /* Position chooser */
                        LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_POSITION_CHOOSER] = 
                            NewObject(CHOOSER_GetClass(), NULL,
                            GA_ID, ITIDY_GAID_POSITION_CHOOSER,
                            GA_RelVerify, TRUE,
                            GA_TabCycle, TRUE,
                            CHOOSER_PopUp, TRUE,
                            CHOOSER_Selected, win_data->window_position_selected,
                            CHOOSER_Labels, win_data->position_labels,
                        TAG_END),
                        CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, "Window Position",
                        TAG_END),
                    TAG_END),
                    
                    /* Right column */
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_RIGHT_COLUMN] = 
                        NewObject(LAYOUT_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_RIGHT_COLUMN,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_LeftSpacing, 2,
                        
                        /* Include subfolders checkbox */
                        LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_RECURSIVE_CHECKBOX] = 
                            NewObject(CHECKBOX_GetClass(), NULL,
                            GA_ID, ITIDY_GAID_RECURSIVE_CHECKBOX,
                            GA_Text, "",
                            GA_RelVerify, TRUE,
                            GA_TabCycle, TRUE,
                            GA_Selected, win_data->recursive_subdirs,
                            CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                        TAG_END),
                        CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, "Include Subfolders",
                        TAG_END),
                        
                        /* Create icons checkbox */
                        LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_CREATE_NEW_ICONS] = 
                            NewObject(CHECKBOX_GetClass(), NULL,
                            GA_ID, ITIDY_GAID_CREATE_NEW_ICONS,
                            GA_Text, "",
                            GA_RelVerify, TRUE,
                            GA_TabCycle, TRUE,
                            GA_Selected, win_data->enable_deficons_icon_creation,
                            CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                        TAG_END),
                        CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, "Create Icons During Tidy",
                        TAG_END),
                        
                        /* Backup checkbox */
                        LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_BACKUP_CHECKBOX] = 
                            NewObject(CHECKBOX_GetClass(), NULL,
                            GA_ID, ITIDY_GAID_BACKUP_CHECKBOX,
                            GA_Text, "",
                            GA_RelVerify, TRUE,
                            GA_TabCycle, TRUE,
                            GA_Selected, win_data->enable_backup,
                            CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                        TAG_END),
                        CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, "Back Up Layout Before Changes",
                        TAG_END),
                    TAG_END),
                TAG_END),
                
                /* Tools button group */
                LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_TOOLS_LAYOUT] = 
                    NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, ITIDY_GAID_TOOLS_LAYOUT,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_BevelStyle, BVS_THIN,
                    LAYOUT_LeftSpacing, 2,
                    LAYOUT_RightSpacing, 2,
                    LAYOUT_TopSpacing, 2,
                    LAYOUT_BottomSpacing, 4,
                    
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_ADVANCED_BUTTON] = 
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_ADVANCED_BUTTON,
                        GA_Text, "Advanced...",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        BUTTON_TextPen, 1,
                        BUTTON_BackgroundPen, 0,
                        BUTTON_FillTextPen, 1,
                        BUTTON_FillPen, 3,
                    TAG_END),
                    
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_DEFAULT_TOOLS_BUTTON] = 
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_DEFAULT_TOOLS_BUTTON,
                        GA_Text, "Fix default tools...",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        BUTTON_TextPen, 1,
                        BUTTON_BackgroundPen, 0,
                        BUTTON_FillTextPen, 1,
                        BUTTON_FillPen, 3,
                    TAG_END),
                    
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_RESTORE_BUTTON] = 
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_RESTORE_BUTTON,
                        GA_Text, "Restore backups...",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        BUTTON_TextPen, 1,
                        BUTTON_BackgroundPen, 0,
                        BUTTON_FillTextPen, 1,
                        BUTTON_FillPen, 3,
                    TAG_END),
                    
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_ICON_CREATION_BUTTON] = 
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_ICON_CREATION_BUTTON,
                        GA_Text, "Icon Creation...",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        BUTTON_TextPen, 1,
                        BUTTON_BackgroundPen, 0,
                        BUTTON_FillTextPen, 1,
                        BUTTON_FillPen, 3,
                    TAG_END),
                TAG_END),
                
                /* Bottom buttons */
                LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_BUTTONS_LAYOUT] = 
                    NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, ITIDY_GAID_BUTTONS_LAYOUT,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_START_BUTTON] = 
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_START_BUTTON,
                        GA_Text, "Start",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        BUTTON_TextPen, 1,
                        BUTTON_BackgroundPen, 0,
                        BUTTON_FillTextPen, 1,
                        BUTTON_FillPen, 3,
                    TAG_END),
                    
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_EXIT_BUTTON] = 
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_EXIT_BUTTON,
                        GA_Text, "Exit",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        BUTTON_TextPen, 1,
                        BUTTON_BackgroundPen, 0,
                        BUTTON_FillTextPen, 1,
                        BUTTON_FillPen, 3,
                    TAG_END),
                TAG_END),
            TAG_END),
        TAG_END),
    TAG_END);
    
    if (!win_data->window_obj)
    {
        log_error(LOG_GUI, "Failed to create window object\n");
        if (win_data->menu_strip) FreeMenus(win_data->menu_strip);
        free_chooser_labels(win_data->order_labels);
        free_chooser_labels(win_data->sortby_labels);
        free_chooser_labels(win_data->position_labels);
        DeleteMsgPort(win_data->app_port);
        FreeVisualInfo(win_data->visual_info);
        UnlockPubScreen(NULL, screen);
        win_data->menu_strip = NULL;
        win_data->order_labels = NULL;
        win_data->sortby_labels = NULL;
        win_data->position_labels = NULL;
        win_data->app_port = NULL;
        win_data->visual_info = NULL;
        win_data->screen = NULL;
        return FALSE;
    }
    
    /* Open the window */
    win_data->window = (struct Window *)RA_OpenWindow(win_data->window_obj);
    if (!win_data->window)
    {
        log_error(LOG_GUI, "Failed to open window\n");
        DisposeObject(win_data->window_obj);
        if (win_data->menu_strip) FreeMenus(win_data->menu_strip);
        free_chooser_labels(win_data->order_labels);
        free_chooser_labels(win_data->sortby_labels);
        free_chooser_labels(win_data->position_labels);
        DeleteMsgPort(win_data->app_port);
        FreeVisualInfo(win_data->visual_info);
        UnlockPubScreen(NULL, screen);
        win_data->window_obj = NULL;
        win_data->menu_strip = NULL;
        win_data->order_labels = NULL;
        win_data->sortby_labels = NULL;
        win_data->position_labels = NULL;
        win_data->app_port = NULL;
        win_data->visual_info = NULL;
        win_data->screen = NULL;
        return FALSE;
    }
    
    /* Attach menus */
    if (win_data->menu_strip)
    {
        SetMenuStrip(win_data->window, win_data->menu_strip);
    }
    
    win_data->window_open = TRUE;
    log_info(LOG_GUI, "Main window opened successfully\n");
    
    return TRUE;
}

/**
 * close_itidy_main_window - Close the main window and free resources
 */
void close_itidy_main_window(struct iTidyMainWindow *win_data)
{
    if (!win_data)
        return;
    
    /* Remove menus */
    if (win_data->window && win_data->menu_strip)
    {
        ClearMenuStrip(win_data->window);
    }
    
    /* Dispose window object (this also closes the window) */
    if (win_data->window_obj)
    {
        DisposeObject(win_data->window_obj);
        win_data->window_obj = NULL;
        win_data->window = NULL;
    }
    
    /* Free menus */
    if (win_data->menu_strip)
    {
        FreeMenus(win_data->menu_strip);
        win_data->menu_strip = NULL;
    }
    
    /* Free chooser labels */
    if (win_data->order_labels)
    {
        free_chooser_labels(win_data->order_labels);
        win_data->order_labels = NULL;
    }
    if (win_data->sortby_labels)
    {
        free_chooser_labels(win_data->sortby_labels);
        win_data->sortby_labels = NULL;
    }
    if (win_data->position_labels)
    {
        free_chooser_labels(win_data->position_labels);
        win_data->position_labels = NULL;
    }
    
    /* Delete message port */
    if (win_data->app_port)
    {
        DeleteMsgPort(win_data->app_port);
        win_data->app_port = NULL;
    }
    
    /* Free visual info */
    if (win_data->visual_info)
    {
        FreeVisualInfo(win_data->visual_info);
        win_data->visual_info = NULL;
    }
    
    /* Unlock screen */
    if (win_data->screen)
    {
        UnlockPubScreen(NULL, win_data->screen);
        win_data->screen = NULL;
    }
    
    win_data->window_open = FALSE;
    log_info(LOG_GUI, "Main window closed\n");
}

/*------------------------------------------------------------------------*/
/* Event Handling                                                        */
/*------------------------------------------------------------------------*/

/**
 * handle_itidy_window_events - Main event loop
 * 
 * Returns TRUE to continue, FALSE to quit
 */
BOOL handle_itidy_window_events(struct iTidyMainWindow *win_data)
{
    ULONG signal = 0;
    ULONG wait_signals;
    ULONG result;
    WORD code;
    BOOL continue_running = TRUE;
    
    if (!win_data || !win_data->window_obj)
        return FALSE;
    
    /* Get the window signal */
    GetAttr(WINDOW_SigMask, win_data->window_obj, &signal);
    
    /* Wait for events */
    wait_signals = Wait(signal | SIGBREAKF_CTRL_C);
    
    /* Check for Ctrl-C */
    if (wait_signals & SIGBREAKF_CTRL_C)
    {
        return FALSE;
    }
    
    /* Handle window events */
    while ((result = RA_HandleInput(win_data->window_obj, &code)) != WMHI_LASTMSG)
    {
        switch (result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                continue_running = FALSE;
                break;
            
            case WMHI_MENUPICK:
                continue_running = handle_menu_selection(code, win_data);
                break;
            
            case WMHI_GADGETUP:
                continue_running = handle_gadget_event(result & WMHI_GADGETMASK, code, win_data);
                break;
            
            case WMHI_ICONIFY:
                if (RA_Iconify(win_data->window_obj))
                {
                    win_data->window = NULL;
                }
                break;
            
            case WMHI_UNICONIFY:
                win_data->window = (struct Window *)RA_OpenWindow(win_data->window_obj);
                if (win_data->window && win_data->menu_strip)
                {
                    SetMenuStrip(win_data->window, win_data->menu_strip);
                }
                break;
        }
    }
    
    return continue_running;
}

/*------------------------------------------------------------------------*/
/* Save/Load Preferences Functions                                        */
/*------------------------------------------------------------------------*/

static BOOL save_preferences_to_file(const char *filepath, const LayoutPreferences *prefs, const DefIconsExcludePaths *ep)
{
    BPTR file;
    const char header[] = "ITIDYPREFS";
    ULONG version = 3;
    ULONG struct_size = sizeof(LayoutPreferences);
    ULONG ep_size = sizeof(DefIconsExcludePaths);
    
    if (!filepath || !prefs || !ep)
    {
        log_error(LOG_GUI, "save_preferences_to_file: Invalid parameters\n");
        return FALSE;
    }
    
    /* Ensure the directory path exists before creating the file */
    if (!CreateDirectoryForFile(filepath))
    {
        log_error(LOG_GUI, "save_preferences_to_file: Failed to create directory path for: %s\n", filepath);
        return FALSE;
    }
    
    file = Open((STRPTR)filepath, MODE_NEWFILE);
    if (!file)
    {
        LONG error = IoErr();
        log_error(LOG_GUI, "save_preferences_to_file: Failed to create file: %s (IoErr: %ld)\n", filepath, error);
        return FALSE;
    }
    
    log_info(LOG_GUI, "Saving preferences to: %s (v%lu, size=%lu)\n", filepath, version, struct_size);
    
    if (Write(file, (APTR)header, 10) != 10)
    {
        log_error(LOG_GUI, "Failed to write header\n");
        Close(file);
        return FALSE;
    }
    
    if (Write(file, (APTR)&version, sizeof(ULONG)) != sizeof(ULONG))
    {
        log_error(LOG_GUI, "Failed to write version\n");
        Close(file);
        return FALSE;
    }
    
    /* v3: write struct size for forward compatibility */
    if (Write(file, (APTR)&struct_size, sizeof(ULONG)) != sizeof(ULONG))
    {
        log_error(LOG_GUI, "Failed to write struct size\n");
        Close(file);
        return FALSE;
    }
    
    if (Write(file, (APTR)prefs, sizeof(LayoutPreferences)) != sizeof(LayoutPreferences))
    {
        log_error(LOG_GUI, "Failed to write preferences structure\n");
        Close(file);
        return FALSE;
    }
    
    /* v3: write exclude paths block (size + data) */
    if (Write(file, (APTR)&ep_size, sizeof(ULONG)) != sizeof(ULONG))
    {
        log_error(LOG_GUI, "Failed to write exclude paths size\n");
        Close(file);
        return FALSE;
    }
    
    if (Write(file, (APTR)ep, sizeof(DefIconsExcludePaths)) != sizeof(DefIconsExcludePaths))
    {
        log_error(LOG_GUI, "Failed to write exclude paths structure\n");
        Close(file);
        return FALSE;
    }
    
    Close(file);
    log_info(LOG_GUI, "Successfully saved preferences to: %s\n", filepath);
    return TRUE;
}

static BOOL load_preferences_from_file(const char *filepath, LayoutPreferences *prefs, DefIconsExcludePaths *ep)
{
    BPTR file;
    char header[11];
    ULONG version;
    
    if (!filepath || !prefs || !ep)
    {
        log_error(LOG_GUI, "load_preferences_from_file: Invalid parameters\n");
        return FALSE;
    }
    
    log_info(LOG_GUI, "Loading preferences from: %s\n", filepath);
    
    /* Initialize with defaults first */
    InitLayoutPreferences(prefs);
    reset_deficons_exclude_paths_to_defaults(ep);
    
    file = Open((STRPTR)filepath, MODE_OLDFILE);
    if (!file)
    {
        log_error(LOG_GUI, "Failed to open file for reading: %s\n", filepath);
        return FALSE;
    }
    
    memset(header, 0, sizeof(header));
    if (Read(file, (APTR)header, 10) != 10)
    {
        log_error(LOG_GUI, "Failed to read header\n");
        Close(file);
        return FALSE;
    }
    
    if (strncmp(header, "ITIDYPREFS", 10) != 0)
    {
        log_error(LOG_GUI, "Invalid file header: %s\n", header);
        Close(file);
        return FALSE;
    }
    
    if (Read(file, (APTR)&version, sizeof(ULONG)) != sizeof(ULONG))
    {
        log_error(LOG_GUI, "Failed to read version\n");
        Close(file);
        return FALSE;
    }
    
    if (version == 1 || version == 2)
    {
        /* v1/v2 preference layout is incompatible with current struct.
         * The DefIcons exclude paths have been extracted from LayoutPreferences
         * (Option B refactor), changing field offsets for all fields that
         * followed the 8KB exclude-paths block.  Loading old files would
         * corrupt those fields, so we reject them and use defaults. */
        log_warning(LOG_GUI, "v%lu preferences file is no longer compatible "
                    "(struct layout changed) - please re-save your settings\n",
                    version);
        Close(file);
        return FALSE;
    }
    else if (version == 3)
    {
        /* v3 format: header(10) + version(4) + struct_size(4) + struct + ep_size(4) + ep */
        ULONG stored_size;
        ULONG read_size;
        ULONG stored_ep_size;
        ULONG read_ep_size;

        if (Read(file, (APTR)&stored_size, sizeof(ULONG)) != sizeof(ULONG))
        {
            log_error(LOG_GUI, "Failed to read v3 struct size\n");
            Close(file);
            return FALSE;
        }

        read_size = (stored_size < sizeof(LayoutPreferences))
                  ? stored_size
                  : sizeof(LayoutPreferences);

        if (Read(file, (APTR)prefs, read_size) != (LONG)read_size)
        {
            log_error(LOG_GUI, "Failed to read v3 preferences structure\n");
            Close(file);
            return FALSE;
        }

        /* Skip any extra bytes if stored struct was larger */
        if (stored_size > sizeof(LayoutPreferences))
        {
            Seek(file, stored_size - sizeof(LayoutPreferences), OFFSET_CURRENT);
        }

        /* Read exclude paths block size */
        if (Read(file, (APTR)&stored_ep_size, sizeof(ULONG)) != sizeof(ULONG))
        {
            log_warning(LOG_GUI, "v3 file: failed to read ep size, using defaults\n");
            Close(file);
            return TRUE;  /* prefs loaded OK; ep stays at defaults */
        }

        read_ep_size = (stored_ep_size < sizeof(DefIconsExcludePaths))
                     ? stored_ep_size
                     : sizeof(DefIconsExcludePaths);

        if (Read(file, (APTR)ep, read_ep_size) != (LONG)read_ep_size)
        {
            log_warning(LOG_GUI, "v3 file: failed to read ep data, using defaults\n");
            reset_deficons_exclude_paths_to_defaults(ep);
        }

        log_info(LOG_GUI, "Loaded v3 preferences: prefs_stored=%lu, ep_stored=%lu\n",
                 stored_size, stored_ep_size);
    }
    else
    {
        log_error(LOG_GUI, "Unsupported file version: %lu\n", version);
        Close(file);
        return FALSE;
    }
    
    Close(file);
    
    /* Validate palette reduction fields to prevent out-of-range crashes */
    {
        /* max_icon_colors must be one of: 4,8,16,32,64,128,256 */
        UWORD mc = prefs->deficons_max_icon_colors;
        if (mc != 4 && mc != 8 && mc != 16 && mc != 32 &&
            mc != 64 && mc != 128 && mc != 256)
        {
            log_warning(LOG_GUI, "Invalid max_icon_colors %u, resetting to 256\n", (unsigned)mc);
            prefs->deficons_max_icon_colors = DEFAULT_DEFICONS_MAX_ICON_COLORS;
        }

        /* dither_method: 0-3 */
        if (prefs->deficons_dither_method > 3)
        {
            log_warning(LOG_GUI, "Invalid dither_method %u, resetting to Auto\n",
                        (unsigned)prefs->deficons_dither_method);
            prefs->deficons_dither_method = DEFAULT_DEFICONS_DITHER_METHOD;
        }

        /* lowcolor_mapping: 0-2 */
        if (prefs->deficons_lowcolor_mapping > 2)
        {
            log_warning(LOG_GUI, "Invalid lowcolor_mapping %u, resetting to Grayscale\n",
                        (unsigned)prefs->deficons_lowcolor_mapping);
            prefs->deficons_lowcolor_mapping = DEFAULT_DEFICONS_LOWCOLOR_MAPPING;
        }
    }
    
    /* DefIcons icon creation setting loaded from file */
    if (prefs->enable_deficons_icon_creation)
    {
        log_info(LOG_GUI, "Post-load: DefIcons icon creation enabled\n");
    }
    
    log_info(LOG_GUI, "Successfully loaded preferences from: %s\n", filepath);
    return TRUE;
}

static void sync_gui_from_preferences(struct iTidyMainWindow *win_data, const LayoutPreferences *prefs)
{
    if (!win_data || !win_data->window || !prefs)
        return;
    
    if (prefs->blockGroupMode == BLOCK_GROUP_BY_TYPE)
    {
        win_data->order_selected = 3;
    }
    else
    {
        win_data->order_selected = prefs->sortPriority;
    }
    
    win_data->sortby_selected = prefs->sortBy;
    win_data->recursive_subdirs = prefs->recursive_subdirs;
    win_data->enable_backup = prefs->enable_backup;
    win_data->enable_deficons_icon_creation = prefs->enable_deficons_icon_creation;
    win_data->window_position_selected = prefs->windowPositionMode;
    
    strncpy(win_data->folder_path_buffer, prefs->folder_path, sizeof(win_data->folder_path_buffer) - 1);
    win_data->folder_path_buffer[sizeof(win_data->folder_path_buffer) - 1] = '\0';
    
    /* Update ReAction gadgets */
    if (win_data->gadgets[ITIDY_GAD_IDX_ORDER_CHOOSER])
    {
        SetGadgetAttrs((struct Gadget *)win_data->gadgets[ITIDY_GAD_IDX_ORDER_CHOOSER],
                      win_data->window, NULL,
                      CHOOSER_Selected, win_data->order_selected,
                      TAG_END);
    }
    
    if (win_data->gadgets[ITIDY_GAD_IDX_SORTBY_CHOOSER])
    {
        BOOL enable_sortby = (win_data->order_selected != 3);
        SetGadgetAttrs((struct Gadget *)win_data->gadgets[ITIDY_GAD_IDX_SORTBY_CHOOSER],
                      win_data->window, NULL,
                      CHOOSER_Selected, win_data->sortby_selected,
                      GA_Disabled, !enable_sortby,
                      TAG_END);
    }
    
    if (win_data->gadgets[ITIDY_GAD_IDX_RECURSIVE_CHECKBOX])
    {
        SetGadgetAttrs((struct Gadget *)win_data->gadgets[ITIDY_GAD_IDX_RECURSIVE_CHECKBOX],
                      win_data->window, NULL,
                      GA_Selected, win_data->recursive_subdirs,
                      TAG_END);
    }
    
    if (win_data->gadgets[ITIDY_GAD_IDX_BACKUP_CHECKBOX])
    {
        SetGadgetAttrs((struct Gadget *)win_data->gadgets[ITIDY_GAD_IDX_BACKUP_CHECKBOX],
                      win_data->window, NULL,
                      GA_Selected, win_data->enable_backup,
                      TAG_END);
    }
    
    if (win_data->gadgets[ITIDY_GAD_IDX_CREATE_NEW_ICONS])
    {
        SetGadgetAttrs((struct Gadget *)win_data->gadgets[ITIDY_GAD_IDX_CREATE_NEW_ICONS],
                      win_data->window, NULL,
                      GA_Selected, win_data->enable_deficons_icon_creation,
                      TAG_END);
    }
    
    if (win_data->gadgets[ITIDY_GAD_IDX_POSITION_CHOOSER])
    {
        SetGadgetAttrs((struct Gadget *)win_data->gadgets[ITIDY_GAD_IDX_POSITION_CHOOSER],
                      win_data->window, NULL,
                      CHOOSER_Selected, win_data->window_position_selected,
                      TAG_END);
    }
    
    /* Update folder path display */
    if (win_data->gadgets[ITIDY_GAD_IDX_FOLDER_GETFILE])
    {
        SetGadgetAttrs((struct Gadget *)win_data->gadgets[ITIDY_GAD_IDX_FOLDER_GETFILE],
                      win_data->window, NULL,
                      GETFILE_Drawer, win_data->folder_path_buffer,
                      TAG_END);
    }
    
    /* Force window refresh to update all gadget visuals */
    if (win_data->window)
    {
        RefreshGList((struct Gadget *)win_data->window->FirstGadget, win_data->window, NULL, -1);
    }
    
    /* Sync backup toggle menu item CHECKED state */
    sync_backup_menu_check(win_data);
    
    log_info(LOG_GUI, "GUI synchronized from loaded preferences\n");
}

static void sync_gui_to_preferences(struct iTidyMainWindow *win_data, LayoutPreferences *prefs)
{
    if (!win_data || !prefs)
        return;
    
    if (win_data->order_selected == 3)
    {
        prefs->blockGroupMode = BLOCK_GROUP_BY_TYPE;
        prefs->sortPriority = SORT_PRIORITY_MIXED;
    }
    else
    {
        prefs->blockGroupMode = BLOCK_GROUP_NONE;
        prefs->sortPriority = win_data->order_selected;
    }
    
    prefs->sortBy = win_data->sortby_selected;
    prefs->recursive_subdirs = win_data->recursive_subdirs;
    prefs->enable_backup = win_data->enable_backup;
    prefs->enable_deficons_icon_creation = win_data->enable_deficons_icon_creation;
    prefs->windowPositionMode = (WindowPositionMode)win_data->window_position_selected;
    
    strncpy(prefs->folder_path, win_data->folder_path_buffer, sizeof(prefs->folder_path) - 1);
    prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
    
    prefs->backupPrefs.enableUndoBackup = win_data->enable_backup;
    
    log_debug(LOG_GUI, "Preferences updated from GUI state (folder: %s)\n", prefs->folder_path);
}

/*------------------------------------------------------------------------*/
/* LOADPREFS Tooltype Handler                                            */
/*------------------------------------------------------------------------*/

/**
 * @brief Auto-load preferences file specified in LOADPREFS tooltype
 * 
 * This function is called after the main window opens successfully.
 * It checks if a LOADPREFS tooltype was specified, validates the file exists,
 * loads the preferences, and updates the GUI. Shows ReAction requesters for
 * any errors encountered.
 * 
 * @param win_data Pointer to main window structure
 */
void handle_tooltype_loadprefs(struct iTidyMainWindow *win_data)
{
    LayoutPreferences loaded_prefs;
    DefIconsExcludePaths loaded_ep;
    char expanded_path[512];
    char error_msg[600];
    BPTR lock;
    
    if (!win_data || !win_data->window)
        return;
    
    /* Check if LOADPREFS was set */
    if (!g_tooltypes.loadprefs_set || g_tooltypes.loadprefs_path[0] == '\0')
        return;
    
    log_info(LOG_GUI, "LOADPREFS tooltype found: %s\n", g_tooltypes.loadprefs_path);
    
    /* Check if path is relative (no colon = no device/assign specified)
     * If so, prepend the default settings directory */
    if (strchr(g_tooltypes.loadprefs_path, ':') == NULL)
    {
        /* Relative path - prepend PROGDIR:userdata/Settings/ */
        snprintf(expanded_path, sizeof(expanded_path), "PROGDIR:userdata/Settings/%s", 
                 g_tooltypes.loadprefs_path);
        log_debug(LOG_GUI, "LOADPREFS relative path, prepending default dir: %s\n", expanded_path);
        
        /* Now expand PROGDIR: */
        char temp_path[512];
        if (ExpandProgDir(expanded_path, temp_path, sizeof(temp_path)))
        {
            strncpy(expanded_path, temp_path, sizeof(expanded_path) - 1);
            expanded_path[sizeof(expanded_path) - 1] = '\0';
        }
    }
    else
    {
        /* Absolute path or assign - expand PROGDIR: if present */
        if (!ExpandProgDir(g_tooltypes.loadprefs_path, expanded_path, sizeof(expanded_path)))
        {
            /* If expansion fails, use original path */
            strncpy(expanded_path, g_tooltypes.loadprefs_path, sizeof(expanded_path) - 1);
            expanded_path[sizeof(expanded_path) - 1] = '\0';
        }
    }
    
    log_debug(LOG_GUI, "LOADPREFS final path: %s\n", expanded_path);
    
    /* Check if file exists */
    lock = Lock((STRPTR)expanded_path, ACCESS_READ);
    if (!lock)
    {
        log_warning(LOG_GUI, "LOADPREFS file not found: %s\n", expanded_path);
        
        /* Show warning requester */
        snprintf(error_msg, sizeof(error_msg),
                "Preferences file specified in LOADPREFS tooltype was not found.\n\n"
                "File: %s\n\n"
                "iTidy will continue with default settings.",
                expanded_path);
        
        ShowReActionRequester(win_data->window,
                             "LOADPREFS Warning",
                             error_msg,
                             "_Continue",
                             REQIMAGE_WARNING);
        return;
    }
    UnLock(lock);
    
    /* Load preferences from file */
    if (!load_preferences_from_file(expanded_path, &loaded_prefs, &loaded_ep))
    {
        log_error(LOG_GUI, "LOADPREFS failed to load preferences: %s\n", expanded_path);
        
        /* Show error requester */
        snprintf(error_msg, sizeof(error_msg),
                "Failed to load preferences file.\n\n"
                "File: %s\n\n"
                "The file may be corrupted or in an incompatible format.\n"
                "iTidy will continue with default settings.",
                expanded_path);
        
        ShowReActionRequester(win_data->window,
                             "LOADPREFS Error",
                             error_msg,
                             "_Continue",
                             REQIMAGE_ERROR);
        return;
    }
    
    /* CRITICAL: Preserve log level from DEBUGLEVEL tooltype if it was set
     * The tooltype should take precedence over the loaded preferences file */
    LogLevel current_log_level = get_global_log_level();
    BOOL preserve_log_level = g_tooltypes.debug_level_set;
    
    /* Update global preferences */
    UpdateGlobalPreferences(&loaded_prefs);
    UpdateGlobalExcludePaths(&loaded_ep);
    
    /* Restore log level if DEBUGLEVEL tooltype was set */
    if (preserve_log_level)
    {
        set_global_log_level(current_log_level);
        log_info(LOG_GUI, "LOADPREFS: Preserved DEBUGLEVEL tooltype setting\n");
    }
    
    /* Update GUI to reflect loaded preferences */
    sync_gui_from_preferences(win_data, &loaded_prefs);
    
    log_info(LOG_GUI, "LOADPREFS loaded successfully: %s\n", expanded_path);
    
    /* Show success message with filename */
    {
        char success_msg[700];
        const char *filename = FilePart((STRPTR)expanded_path);
        
        snprintf(success_msg, sizeof(success_msg),
                "Preferences loaded successfully from:\n\n%s",
                filename ? filename : expanded_path);
        
        ShowReActionRequester(win_data->window,
                             "LOADPREFS",
                             success_msg,
                             "_Ok",
                             REQIMAGE_INFO);
    }
}

/*------------------------------------------------------------------------*/
/* Menu Handler Functions                                                */
/*------------------------------------------------------------------------*/

static void handle_main_new_menu(struct iTidyMainWindow *win_data)
{
    LayoutPreferences default_prefs;
    
    if (!win_data || !win_data->window)
        return;
    
    log_debug(LOG_GUI, "Menu: New clicked - resetting to defaults\n");
    
    /* CRITICAL: Preserve current log level (from tooltype or menu selection)
     * User's log level choice should persist when creating new preferences */
    LogLevel current_log_level = get_global_log_level();
    
    InitLayoutPreferences(&default_prefs);
    
    /* Restore the log level before updating global preferences */
    default_prefs.logLevel = current_log_level;
    
    UpdateGlobalPreferences(&default_prefs);
    sync_gui_from_preferences(win_data, &default_prefs);
    
    win_data->last_save_path[0] = '\0';
    
    log_info(LOG_GUI, "Preferences reset to defaults\n");
}

static void handle_main_save_menu(struct iTidyMainWindow *win_data)
{
    LayoutPreferences *prefs;
    
    if (!win_data || !win_data->window)
        return;
    
    prefs = (LayoutPreferences *)GetGlobalPreferences();
    if (!prefs)
    {
        ShowReActionRequester(win_data->window,
            "Error",
            "Failed to get preferences.",
            "_OK",
            REQIMAGE_ERROR);
        return;
    }
    
    if (win_data->last_save_path[0] == '\0')
    {
        log_debug(LOG_GUI, "Menu: Save clicked but no previous save path - calling Save As\n");
        handle_main_save_as_menu(win_data);
        return;
    }
    
    sync_gui_to_preferences(win_data, prefs);
    UpdateGlobalPreferences(prefs);
    
    log_info(LOG_GUI, "Menu: Save clicked - saving to %s\n", win_data->last_save_path);
    
    if (save_preferences_to_file(win_data->last_save_path, prefs, GetGlobalExcludePaths()))
    {
        log_info(LOG_GUI, "Preferences saved to: %s\n", win_data->last_save_path);
    }
    else
    {
        ShowReActionRequester(win_data->window,
            "Save Failed",
            "Failed to save preferences file.",
            "_OK",
            REQIMAGE_ERROR);
    }
}

static void handle_main_save_as_menu(struct iTidyMainWindow *win_data)
{
    struct FileRequester *freq;
    char full_path[512];
    char initial_drawer[512];
    BPTR lock;
    LayoutPreferences *prefs;
    
    if (!win_data || !win_data->window)
        return;
    
    prefs = (LayoutPreferences *)GetGlobalPreferences();
    if (!prefs)
    {
        ShowReActionRequester(win_data->window,
            "Error",
            "Failed to get preferences.",
            "_OK",
            REQIMAGE_ERROR);
        return;
    }
    
    /* Expand PROGDIR: to actual path for file requester */
    if (!ExpandProgDir("PROGDIR:userdata/Settings", initial_drawer, sizeof(initial_drawer)))
    {
        log_warning(LOG_GUI, "Failed to expand PROGDIR, using literal path\n");
        strcpy(initial_drawer, "PROGDIR:userdata/Settings");
    }
    
    log_info(LOG_GUI, "Save As: Initial drawer path: '%s'\n", initial_drawer);
    
    /* Create the directory structure BEFORE opening the file requester */
    /* This prevents AmigaOS from showing its own "Create Drawer?" requester */
    log_info(LOG_GUI, "Save As: Pre-creating directory structure...\n");
    if (!CreateDirectoryPath(initial_drawer))
    {
        log_warning(LOG_GUI, "Save As: Failed to pre-create directory, continuing anyway...\n");
    }
    else
    {
        log_info(LOG_GUI, "Save As: Directory structure created successfully\n");
    }
    
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Save Preferences As...",
        ASLFR_InitialDrawer, initial_drawer,
        ASLFR_InitialFile, "iTidy.prefs",
        ASLFR_DoSaveMode, TRUE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, win_data->window,
        TAG_END);
    
    if (!freq)
    {
        ShowReActionRequester(win_data->window,
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
            FreeAslRequest(freq);
            ShowReActionRequester(win_data->window,
                "Error",
                "File path is too long.",
                "_OK",
                REQIMAGE_ERROR);
            return;
        }
        
        lock = Lock((STRPTR)full_path, ACCESS_READ);
        if (lock)
        {
            UnLock(lock);
            
            if (!ShowReActionRequester(win_data->window,
                "File Exists",
                "File already exists.\nDo you want to replace it?",
                "_Replace|_Cancel",
                REQIMAGE_QUESTION))
            {
                FreeAslRequest(freq);
                return;
            }
        }
        
        sync_gui_to_preferences(win_data, prefs);
        UpdateGlobalPreferences(prefs);
        
        if (save_preferences_to_file(full_path, prefs, GetGlobalExcludePaths()))
        {
            strncpy(win_data->last_save_path, full_path, sizeof(win_data->last_save_path) - 1);
            win_data->last_save_path[sizeof(win_data->last_save_path) - 1] = '\0';
            
            ShowReActionRequester(win_data->window,
                "Save Successful",
                "Preferences saved successfully.",
                "_OK",
                REQIMAGE_INFO);
            log_info(LOG_GUI, "Preferences saved to: %s\n", full_path);
        }
        else
        {
            ShowReActionRequester(win_data->window,
                "Save Failed",
                "Failed to save preferences file.",
                "_OK",
                REQIMAGE_ERROR);
        }
    }
    
    FreeAslRequest(freq);
}

static void handle_main_open_menu(struct iTidyMainWindow *win_data)
{
    struct FileRequester *freq;
    char full_path[512];
    char initial_drawer[512];
    BPTR lock;
    LayoutPreferences loaded_prefs;
    DefIconsExcludePaths loaded_ep;
    
    if (!win_data || !win_data->window)
        return;
    
    /* Expand PROGDIR: to actual path for file requester */
    if (!ExpandProgDir("PROGDIR:userdata/Settings", initial_drawer, sizeof(initial_drawer)))
    {
        log_warning(LOG_GUI, "Failed to expand PROGDIR, using literal path\n");
        strcpy(initial_drawer, "PROGDIR:userdata/Settings");
    }
    
    log_info(LOG_GUI, "Load: Initial drawer path: '%s'\n", initial_drawer);
    
    /* Create the directory structure BEFORE opening the file requester */
    log_info(LOG_GUI, "Load: Pre-creating directory structure...\n");
    if (!CreateDirectoryPath(initial_drawer))
    {
        log_warning(LOG_GUI, "Load: Failed to pre-create directory, continuing anyway...\n");
    }
    else
    {
        log_info(LOG_GUI, "Load: Directory structure created successfully\n");
    }
    
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Open Preferences File...",
        ASLFR_InitialDrawer, initial_drawer,
        ASLFR_InitialFile, "iTidy.prefs",
        ASLFR_DoSaveMode, FALSE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, win_data->window,
        TAG_END);
    
    if (!freq)
    {
        ShowReActionRequester(win_data->window,
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
            FreeAslRequest(freq);
            ShowReActionRequester(win_data->window,
                "Error",
                "File path is too long.",
                "_OK",
                REQIMAGE_ERROR);
            return;
        }
        
        lock = Lock((STRPTR)full_path, ACCESS_READ);
        if (!lock)
        {
            FreeAslRequest(freq);
            ShowReActionRequester(win_data->window,
                "File Not Found",
                "The selected file does not exist.",
                "_OK",
                REQIMAGE_ERROR);
            return;
        }
        UnLock(lock);
        
        if (load_preferences_from_file(full_path, &loaded_prefs, &loaded_ep))
        {
            /* CRITICAL: Preserve current log level (from tooltype or menu selection)
             * Old settings files may have different log level, but we want to keep
             * the user's current session log level choice */
            LogLevel current_log_level = get_global_log_level();
            
            UpdateGlobalPreferences(&loaded_prefs);
            UpdateGlobalExcludePaths(&loaded_ep);
            
            /* Restore the log level and update both global and preferences */
            set_global_log_level(current_log_level);
            {
                LayoutPreferences *prefs = (LayoutPreferences *)GetGlobalPreferences();
                ((LayoutPreferences *)prefs)->logLevel = current_log_level;
            }
            
            sync_gui_from_preferences(win_data, &loaded_prefs);
            
            strncpy(win_data->last_save_path, full_path, sizeof(win_data->last_save_path) - 1);
            win_data->last_save_path[sizeof(win_data->last_save_path) - 1] = '\0';
            
            ShowReActionRequester(win_data->window,
                "Load Successful",
                "Preferences loaded successfully.",
                "_OK",
                REQIMAGE_INFO);
            log_info(LOG_GUI, "Preferences loaded from: %s\n", full_path);
        }
        else
        {
            ShowReActionRequester(win_data->window,
                "Load Failed",
                "Failed to load preferences file.\nFile may be corrupted or invalid.",
                "_OK",
                REQIMAGE_ERROR);
        }
    }
    
    FreeAslRequest(freq);
}

/*------------------------------------------------------------------------*/
/* Menu Helper Functions                                                 */
/*------------------------------------------------------------------------*/

/* Find a menu item by its user-data ID, searching all menus, items, and subitems */
static struct MenuItem *find_menu_item_by_id(struct Menu *menu_strip, ULONG target_id)
{
    struct Menu *menu;
    struct MenuItem *item, *sub;
    
    if (!menu_strip || target_id == 0)
        return NULL;
    
    for (menu = menu_strip; menu; menu = menu->NextMenu)
    {
        for (item = menu->FirstItem; item; item = item->NextItem)
        {
            if ((ULONG)GTMENUITEM_USERDATA(item) == target_id)
                return item;
            for (sub = item->SubItem; sub; sub = sub->NextItem)
            {
                if ((ULONG)GTMENUITEM_USERDATA(sub) == target_id)
                    return sub;
            }
        }
    }
    return NULL;
}

/* Sync the Backups menu CHECKED state from win_data->enable_backup */
static void sync_backup_menu_check(struct iTidyMainWindow *win_data)
{
    struct MenuItem *mitem;
    
    if (!win_data || !win_data->menu_strip)
        return;
    
    mitem = find_menu_item_by_id(win_data->menu_strip, MENU_SETTINGS_BACKUP_TOGGLE);
    if (mitem)
    {
        if (win_data->enable_backup)
            mitem->Flags |= CHECKED;
        else
            mitem->Flags &= ~CHECKED;
    }
}

/*------------------------------------------------------------------------*/
/* Menu Handling                                                         */
/*------------------------------------------------------------------------*/

static BOOL handle_menu_selection(ULONG menu_number, struct iTidyMainWindow *win_data)
{
    struct MenuItem *item;
    ULONG item_id;
    BOOL continue_running = TRUE;
    
    while (menu_number != MENUNULL)
    {
        item = ItemAddress(win_data->menu_strip, menu_number);
        if (item)
        {
            item_id = (ULONG)GTMENUITEM_USERDATA(item);
            
            switch (item_id)
            {
                /* --- Presets menu --- */
                case MENU_PRESETS_RESET:
                    handle_main_new_menu(win_data);
                    break;
                
                case MENU_PRESETS_OPEN:
                    handle_main_open_menu(win_data);
                    break;
                
                case MENU_PRESETS_SAVE:
                    handle_main_save_menu(win_data);
                    break;
                
                case MENU_PRESETS_SAVE_AS:
                    handle_main_save_as_menu(win_data);
                    break;
                
                case MENU_PRESETS_QUIT:
                    continue_running = FALSE;
                    break;
                
                /* --- Settings > Advanced --- */
                case MENU_SETTINGS_ADVANCED:
                    {
                        struct iTidyAdvancedWindow adv_data;
                        LayoutPreferences *temp_prefs = (LayoutPreferences *)AllocVec(sizeof(LayoutPreferences), MEMF_ANY|MEMF_CLEAR);
                        if (!temp_prefs) break;
                        
                        memcpy(temp_prefs, GetGlobalPreferences(), sizeof(LayoutPreferences));
                        memset(&adv_data, 0, sizeof(adv_data));
                        
                        if (open_itidy_advanced_window(&adv_data, temp_prefs))
                        {
                            handle_itidy_advanced_window_events(&adv_data);
                            close_itidy_advanced_window(&adv_data);
                            
                            if (adv_data.changes_accepted)
                            {
                                UpdateGlobalPreferences(temp_prefs);
                                log_info(LOG_GUI, "Advanced settings updated via menu\n");
                            }
                        }
                        FreeVec(temp_prefs);
                    }
                    break;
                
                /* --- Settings > DefIcons Categories submenu --- */
                case MENU_SETTINGS_DEFI_CATS:
                    {
                        LayoutPreferences *prefs = (LayoutPreferences *)GetGlobalPreferences();
                        LayoutPreferences *working_copy = (LayoutPreferences *)AllocVec(sizeof(LayoutPreferences), MEMF_ANY|MEMF_CLEAR);
                        if (!working_copy) break;
                        memcpy(working_copy, prefs, sizeof(LayoutPreferences));
                        if (open_itidy_deficons_settings_window(working_copy))
                        {
                            UpdateGlobalPreferences(working_copy);
                            log_info(LOG_GUI, "DefIcons preferences updated\n");
                        }
                        FreeVec(working_copy);
                    }
                    break;
                
                case MENU_SETTINGS_DEFI_PREVIEW:
                    {
                        LayoutPreferences *prefs = (LayoutPreferences *)GetGlobalPreferences();
                        LayoutPreferences *working_copy = (LayoutPreferences *)AllocVec(sizeof(LayoutPreferences), MEMF_ANY|MEMF_CLEAR);
                        if (!working_copy) break;
                        memcpy(working_copy, prefs, sizeof(LayoutPreferences));
                        open_text_templates_window(working_copy);
                        FreeVec(working_copy);
                    }
                    break;
                
                case MENU_SETTINGS_DEFI_EXCLUDE:
                    {
                        /* Heap-allocate working copy (DefIconsExcludePaths ~8KB) to
                         * keep this case block off the 68000 stack frame */
                        DefIconsExcludePaths *ep_copy = (DefIconsExcludePaths *)AllocVec(
                            sizeof(DefIconsExcludePaths), MEMF_ANY|MEMF_CLEAR);
                        if (!ep_copy) break;
                        memcpy(ep_copy, GetGlobalExcludePaths(), sizeof(DefIconsExcludePaths));
                        if (open_exclude_paths_window(ep_copy, win_data->folder_path_buffer))
                        {
                            UpdateGlobalExcludePaths(ep_copy);
                            log_info(LOG_GUI, "Exclude paths updated\n");
                        }
                        FreeVec(ep_copy);
                    }
                    break;
                
                /* --- Settings > Backups submenu --- */
                case MENU_SETTINGS_BACKUP_TOGGLE:
                    /* GadTools toggles the CHECKED flag automatically on CHECKIT items.
                       We just need to read the new state and sync it to the gadget. */
                    {
                        struct MenuItem *mitem = find_menu_item_by_id(
                            win_data->menu_strip, MENU_SETTINGS_BACKUP_TOGGLE);
                        if (mitem)
                            win_data->enable_backup = (mitem->Flags & CHECKED) ? TRUE : FALSE;
                        
                        if (win_data->gadgets[ITIDY_GAD_IDX_BACKUP_CHECKBOX])
                        {
                            SetGadgetAttrs(
                                (struct Gadget *)win_data->gadgets[ITIDY_GAD_IDX_BACKUP_CHECKBOX],
                                win_data->window, NULL,
                                GA_Selected, win_data->enable_backup,
                                TAG_END);
                            /* RefreshGList forces the ReAction checkbox to redraw after
                               the attribute change, which SetGadgetAttrs alone doesn't do. */
                            RefreshGList(
                                (struct Gadget *)win_data->gadgets[ITIDY_GAD_IDX_BACKUP_CHECKBOX],
                                win_data->window, NULL, 1);
                        }
                        log_debug(LOG_GUI, "Backup toggle via menu: %s\n",
                            win_data->enable_backup ? "ON" : "OFF");
                    }
                    break;
                
                /* --- Settings > Logging submenu --- */
                case MENU_SETTINGS_LOG_DISABLED:
                    set_global_log_level(LOG_LEVEL_DISABLED);
                    ((LayoutPreferences *)GetGlobalPreferences())->logLevel = LOG_LEVEL_DISABLED;
                    log_info(LOG_GUI, "Log level: DISABLED\n");
                    break;
                
                case MENU_SETTINGS_LOG_DEBUG:
                    set_global_log_level(LOG_LEVEL_DEBUG);
                    ((LayoutPreferences *)GetGlobalPreferences())->logLevel = LOG_LEVEL_DEBUG;
                    log_info(LOG_GUI, "Log level: DEBUG\n");
                    break;
                
                case MENU_SETTINGS_LOG_INFO:
                    set_global_log_level(LOG_LEVEL_INFO);
                    ((LayoutPreferences *)GetGlobalPreferences())->logLevel = LOG_LEVEL_INFO;
                    log_info(LOG_GUI, "Log level: INFO\n");
                    break;
                
                case MENU_SETTINGS_LOG_WARNING:
                    set_global_log_level(LOG_LEVEL_WARNING);
                    ((LayoutPreferences *)GetGlobalPreferences())->logLevel = LOG_LEVEL_WARNING;
                    log_info(LOG_GUI, "Log level: WARNING\n");
                    break;
                
                case MENU_SETTINGS_LOG_ERROR:
                    set_global_log_level(LOG_LEVEL_ERROR);
                    ((LayoutPreferences *)GetGlobalPreferences())->logLevel = LOG_LEVEL_ERROR;
                    log_info(LOG_GUI, "Log level: ERROR\n");
                    break;
                
                case MENU_SETTINGS_LOG_FOLDER:
                    handle_menu_open_log_folder(win_data->window);
                    break;
                
                /* --- Tools > Restore submenu --- */
                case MENU_TOOLS_RESTORE_LAYOUTS:
                    {
                        struct iTidyRestoreWindow restore_data;
                        
                        safe_set_window_pointer(win_data->window, TRUE);
                        
                        if (open_restore_window(&restore_data))
                        {
                            safe_set_window_pointer(win_data->window, FALSE);
                            while (handle_restore_window_events(&restore_data))
                            {
                            }
                            close_restore_window(&restore_data);
                        }
                        else
                        {
                            safe_set_window_pointer(win_data->window, FALSE);
                            log_error(LOG_GUI, "Failed to open Restore Layouts window\n");
                        }
                    }
                    break;
                
                case MENU_TOOLS_RESTORE_DEFTOOLS:
                    {
                        iTidy_ToolBackupManager temp_manager;
                        struct Window *restore_win;
                        
                        if (!iTidy_InitToolBackupManager(&temp_manager, FALSE))
                        {
                            ShowReActionRequester(win_data->window,
                                "Error",
                                "Failed to initialize backup system.",
                                "_OK",
                                REQIMAGE_ERROR);
                            break;
                        }
                        
                        safe_set_window_pointer(win_data->window, TRUE);
                        restore_win = iTidy_CreateToolRestoreWindow(
                            win_data->window->WScreen, &temp_manager);
                        safe_set_window_pointer(win_data->window, FALSE);
                        
                        if (restore_win)
                        {
                            while (iTidy_HandleToolRestoreWindowEvent(restore_win, NULL))
                            {
                            }
                            iTidy_CloseToolRestoreWindow(restore_win);
                        }
                        else
                        {
                            ShowReActionRequester(win_data->window,
                                "Error",
                                "Failed to open Restore Default Tools window.",
                                "_OK",
                                REQIMAGE_ERROR);
                        }
                        
                        iTidy_CleanupToolBackupManager(&temp_manager);
                    }
                    break;
                
                /* --- Help menu --- */
                case MENU_HELP_GUIDE:
                    {
                        /* Resolve PROGDIR:iTidy.guide to an absolute path before
                         * passing to OpenWorkbenchObject() - it does not understand
                         * process-local assignments like PROGDIR:               */
                        BPTR guide_lock;
                        char guide_abs[512];
                        struct Library *WBBase2;

                        guide_lock = Lock("PROGDIR:iTidy.guide", ACCESS_READ);
                        if (!guide_lock)
                        {
                            ShowReActionRequester(win_data->window,
                                "iTidy Guide",
                                "Could not find iTidy.guide.\n\n"
                                "The guide file should be placed in\n"
                                "the iTidy program directory.",
                                "_OK",
                                REQIMAGE_WARNING);
                            break;
                        }

                        guide_abs[0] = '\0';
                        if (!NameFromLock(guide_lock, guide_abs, (LONG)sizeof(guide_abs)))
                        {
                            /* Fallback: unlikely but handle gracefully */
                            strncpy(guide_abs, "PROGDIR:iTidy.guide", sizeof(guide_abs) - 1);
                            guide_abs[sizeof(guide_abs) - 1] = '\0';
                        }
                        UnLock(guide_lock);

                        WBBase2 = OpenLibrary("workbench.library", 36L);
                        if (WBBase2)
                        {
                            if (!OpenWorkbenchObject(guide_abs, TAG_DONE))
                            {
                                ShowReActionRequester(win_data->window,
                                    "iTidy Guide",
                                    "Could not open iTidy.guide.\n"
                                    "AmigaGuide may not be installed.",
                                    "_OK",
                                    REQIMAGE_WARNING);
                            }
                            CloseLibrary(WBBase2);
                        }
                        else
                        {
                            ShowReActionRequester(win_data->window,
                                "iTidy Guide",
                                "Could not open workbench.library.",
                                "_OK",
                                REQIMAGE_ERROR);
                        }
                    }
                    break;
                
                case MENU_HELP_ABOUT:
                    ShowReActionRequester(win_data->window,
                        "About iTidy",
                        "iTidy v" ITIDY_VERSION "\n\n"
                        "Icon Cleanup Tool for AmigaOS\n"
                        "ReAction GUI Version (WB 3.2+)\n\n"
                        "Automatically arranges icon layouts\n"
                        "and resizes folder windows.\n\n"
                        "(c) 2025-2026",
                        "_Ok",
                        REQIMAGE_INFO);
                    break;
            }
            
            menu_number = item->NextSelect;
        }
        else
        {
            break;
        }
    }
    
    return continue_running;
}

/*------------------------------------------------------------------------*/
/* Gadget Event Handling                                                 */
/*------------------------------------------------------------------------*/

static BOOL handle_gadget_event(ULONG gadget_id, WORD code, struct iTidyMainWindow *win_data)
{
    switch (gadget_id)
    {
        case ITIDY_GAID_FOLDER_GETFILE:
            /* Invoke the file requester using GFILE_REQUEST method */
            if (DoMethod((Object *)win_data->gadgets[ITIDY_GAD_IDX_FOLDER_GETFILE],
                         GFILE_REQUEST, win_data->window))
            {
                /* Read the selected path */
                STRPTR drawer = NULL;
                
                GetAttr(GETFILE_Drawer, win_data->gadgets[ITIDY_GAD_IDX_FOLDER_GETFILE], 
                       (ULONG *)&drawer);
                       
                if (drawer && drawer[0])
                {
                    strncpy(win_data->folder_path_buffer, drawer, 
                           sizeof(win_data->folder_path_buffer) - 1);
                    win_data->folder_path_buffer[sizeof(win_data->folder_path_buffer) - 1] = '\0';
                    log_info(LOG_GUI, "Folder selected: %s\n", win_data->folder_path_buffer);
                }
            }
            break;
        
        case ITIDY_GAID_ORDER_CHOOSER:
            win_data->order_selected = code;
            CONSOLE_DEBUG("Order changed to: %s\n", order_labels_str[code]);
            /* Disable Sort By when "Grouped by type" is selected */
            if (win_data->gadgets[ITIDY_GAD_IDX_SORTBY_CHOOSER])
            {
                SetGadgetAttrs((struct Gadget *)win_data->gadgets[ITIDY_GAD_IDX_SORTBY_CHOOSER],
                              win_data->window, NULL,
                              GA_Disabled, (code == 3),
                              TAG_END);
            }
            break;
        
        case ITIDY_GAID_SORTBY_CHOOSER:
            win_data->sortby_selected = code;
            CONSOLE_DEBUG("Sort by changed to: %s\n", sortby_labels_str[code]);
            break;
        
        case ITIDY_GAID_RECURSIVE_CHECKBOX:
            {
                ULONG selected = 0;
                GetAttr(GA_Selected, win_data->gadgets[ITIDY_GAD_IDX_RECURSIVE_CHECKBOX], &selected);
                win_data->recursive_subdirs = (BOOL)selected;
                CONSOLE_DEBUG("Recursive: %s\n", win_data->recursive_subdirs ? "ON" : "OFF");
            }
            break;
        
        case ITIDY_GAID_BACKUP_CHECKBOX:
            {
                ULONG selected = 0;
                GetAttr(GA_Selected, win_data->gadgets[ITIDY_GAD_IDX_BACKUP_CHECKBOX], &selected);
                win_data->enable_backup = (BOOL)selected;
                sync_backup_menu_check(win_data);
                CONSOLE_DEBUG("Backup: %s\n", win_data->enable_backup ? "ON" : "OFF");
            }
            break;
        
        case ITIDY_GAID_POSITION_CHOOSER:
            win_data->window_position_selected = code;
            CONSOLE_DEBUG("Position changed to: %s\n", position_labels_str[code]);
            break;
        
        case ITIDY_GAID_CREATE_NEW_ICONS:
            {
                ULONG selected = 0;
                GetAttr(GA_Selected, win_data->gadgets[ITIDY_GAD_IDX_CREATE_NEW_ICONS], &selected);
                win_data->enable_deficons_icon_creation = (BOOL)selected;
                CONSOLE_DEBUG("Create new icons: %s\n", win_data->enable_deficons_icon_creation ? "ON" : "OFF");
            }
            break;
        
        case ITIDY_GAID_ADVANCED_BUTTON:
            {
                struct iTidyAdvancedWindow adv_data;
                LayoutPreferences *temp_prefs = (LayoutPreferences *)AllocVec(sizeof(LayoutPreferences), MEMF_ANY|MEMF_CLEAR);
                
                CONSOLE_STATUS("Advanced button clicked - opening Advanced Settings window\n");
                
                if (!temp_prefs)
                {
                    CONSOLE_ERROR("Failed to allocate temp_prefs\n");
                    break;
                }
                
                /* Get current global preferences (includes all current settings) */
                memcpy(temp_prefs, GetGlobalPreferences(), sizeof(LayoutPreferences));
                memset(&adv_data, 0, sizeof(adv_data));
                
                /* Open advanced window (modal) */
                if (open_itidy_advanced_window(&adv_data, temp_prefs))
                {
                    /* Disable main window input while advanced window is open */
                    /* ReAction: We can't use ModifyIDCMP on ReAction windows, so we just don't process events */
                    
                    /* Run advanced window event loop - blocks until done */
                    handle_itidy_advanced_window_events(&adv_data);
                    
                    /* Close advanced window */
                    close_itidy_advanced_window(&adv_data);
                    
                    /* If changes were accepted, update global preferences */
                    if (adv_data.changes_accepted)
                    {
                        CONSOLE_STATUS("Advanced settings accepted - updating global preferences\n");
                        
                        /* Update global preferences with advanced settings */
                        UpdateGlobalPreferences(temp_prefs);
                        
                        CONSOLE_DEBUG("Settings will be applied when you click Start button\n");
                    }
                    else
                    {
                        CONSOLE_DEBUG("Advanced settings cancelled\n");
                    }
                }
                else
                {
                    CONSOLE_ERROR("Failed to open Advanced Settings window\n");
                }
                FreeVec(temp_prefs);
            }
            break;
        
        case ITIDY_GAID_DEFAULT_TOOLS_BUTTON:
            {
                struct iTidyToolCacheWindow tool_window;
                LayoutPreferences *prefs;
                
                CONSOLE_STATUS("Fix default tools button clicked\n");
                log_info(LOG_GUI, "Opening tool cache window (cache has %d entries)\n", g_ToolCacheCount);
                
                /* Sync current GUI folder path and recursive mode to global preferences */
                /* This ensures Rebuild Cache works even if user hasn't clicked Start yet */
                prefs = (LayoutPreferences *)GetGlobalPreferences();
                if (prefs)
                {
                    strncpy(prefs->folder_path, win_data->folder_path_buffer, sizeof(prefs->folder_path) - 1);
                    prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
                    prefs->recursive_subdirs = win_data->recursive_subdirs;
                    log_info(LOG_GUI, "Synced folder path to global prefs: %s (recursive: %s)\n",
                             prefs->folder_path,
                             prefs->recursive_subdirs ? "Yes" : "No");
                }
                
                /* Open tool cache window */
                if (open_tool_cache_window(&tool_window))
                {
                    /* Event loop - handler does the Wait(), no wait here */
                    while (handle_tool_cache_window_events(&tool_window))
                    {
                        /* No wait here - handler does the waiting */
                    }
                    
                    /* Cleanup */
                    close_tool_cache_window(&tool_window);
                    log_info(LOG_GUI, "Tool cache window closed\n");
                }
                else
                {
                    log_error(LOG_GUI, "Failed to open tool cache window\n");
                    (void)ShowReActionRequester(
                        win_data->window,
                        "Error",
                        "Failed to open tool cache window.\n"
                        "Check the log for details.",
                        "_OK",
                        REQIMAGE_ERROR);
                }
            }
            break;
        
        case ITIDY_GAID_ICON_CREATION_BUTTON:
            {
                LayoutPreferences *prefs = (LayoutPreferences *)GetGlobalPreferences();
                LayoutPreferences *working_copy = (LayoutPreferences *)AllocVec(sizeof(LayoutPreferences), MEMF_ANY|MEMF_CLEAR);
                
                CONSOLE_STATUS("Icon Creation button clicked - opening DefIcons creation window\n");
                
                if (!working_copy) break;
                
                memcpy(working_copy, prefs, sizeof(LayoutPreferences));
                
                if (open_itidy_deficons_creation_window(working_copy))
                {
                    UpdateGlobalPreferences(working_copy);
                    log_info(LOG_GUI, "DefIcons creation preferences updated\n");
                }
                else
                {
                    log_debug(LOG_GUI, "DefIcons creation options cancelled\n");
                }
                FreeVec(working_copy);
            }
            break;
        
        case ITIDY_GAID_RESTORE_BUTTON:
            {
                struct iTidyRestoreWindow restore_data;
                
                CONSOLE_STATUS("Restore backups button clicked - opening Restore window\n");
                
                /* Set busy pointer on main window */
                safe_set_window_pointer(win_data->window, TRUE);
                
                /* Open restore window (modal) */
                if (open_restore_window(&restore_data))
                {
                    /* Clear busy pointer - restore window is now open */
                    safe_set_window_pointer(win_data->window, FALSE);
                    
                    /* Run restore window event loop (Wait is inside handler) */
                    while (handle_restore_window_events(&restore_data))
                    {
                        /* Event handler includes Wait() - no need to wait here */
                    }
                    
                    /* Close restore window */
                    close_restore_window(&restore_data);
                    
                    CONSOLE_DEBUG("Restore window closed\n");
                }
                else
                {
                    /* Clear busy pointer on error */
                    safe_set_window_pointer(win_data->window, FALSE);
                    
                    CONSOLE_ERROR("Failed to open Restore window\n");
                }
            }
            break;
        
        case ITIDY_GAID_START_BUTTON:
            CONSOLE_STATUS("Start button clicked - Processing icons...\n");
            {
                struct iTidyMainProgressWindow progress_window;
                LayoutPreferences *prefs = (LayoutPreferences *)GetGlobalPreferences();
                BOOL success;
                
                /* Update preferences from GUI state */
                if (win_data->order_selected == 3)
                {
                    prefs->blockGroupMode = BLOCK_GROUP_BY_TYPE;
                    prefs->sortPriority = SORT_PRIORITY_MIXED;
                }
                else
                {
                    prefs->blockGroupMode = BLOCK_GROUP_NONE;
                    prefs->sortPriority = win_data->order_selected;
                }
                
                prefs->sortBy = win_data->sortby_selected;
                prefs->recursive_subdirs = win_data->recursive_subdirs;
                prefs->enable_backup = win_data->enable_backup;
                prefs->enable_deficons_icon_creation = win_data->enable_deficons_icon_creation;
                prefs->windowPositionMode = (WindowPositionMode)win_data->window_position_selected;
                
                strncpy(prefs->folder_path, win_data->folder_path_buffer, 
                       sizeof(prefs->folder_path) - 1);
                prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
                
                prefs->backupPrefs.enableUndoBackup = win_data->enable_backup;
                
                /* Check LHA availability if backup enabled */
                if (prefs->enable_backup)
                {
                    char lha_path[32];
                    if (!CheckLhaAvailable(lha_path))
                    {
                        LONG result = (LONG)ShowReActionRequester(win_data->window,
                            "LHA Not Found",
                            "LHA archiver not found.\n"
                            "Backups cannot be created.\n\n"
                            "Continue without backups?",
                            "_Continue|_Cancel",
                            REQIMAGE_WARNING);
                        
                        if (result == 0)
                        {
                            break;
                        }
                        prefs->enable_backup = FALSE;
                        prefs->backupPrefs.enableUndoBackup = FALSE;
                    }
                }
                
                /* Open progress window */
                if (!itidy_main_progress_window_open(&progress_window))
                {
                    CONSOLE_ERROR("Failed to open progress window\n");
                    ShowReActionRequester(win_data->window,
                        "Error",
                        "Failed to open progress window",
                        "_OK",
                        REQIMAGE_ERROR);
                    break;
                }
                
                /* Set busy pointer on main window */
                safe_set_window_pointer(win_data->window, TRUE);
                
                /* Process the directory with progress window integration */
                CONSOLE_NEWLINE();
                CONSOLE_STATUS(">>> Starting icon processing...\n");
                CONSOLE_NEWLINE();
                success = ProcessDirectoryWithPreferencesAndProgress(&progress_window);
                
                /* Show result in progress window */
                itidy_main_progress_window_append_status(&progress_window, "");
                itidy_main_progress_window_append_status(&progress_window, 
                    "===============================================");
                if (success)
                {
                    CONSOLE_STATUS("Icon processing completed successfully!\n");
                    itidy_main_progress_window_append_status(&progress_window, 
                        "Icon processing completed successfully!");
                    itidy_main_progress_window_append_status(&progress_window, 
                        "Note you may need to restart your Amiga to see window updates.");
                }
                else
                {
                    CONSOLE_WARNING("Icon processing failed or was incomplete\n");
                    itidy_main_progress_window_append_status(&progress_window, 
                        "Icon processing failed or was incomplete");
                }
                itidy_main_progress_window_append_status(&progress_window, 
                    "===============================================");
                CONSOLE_SEPARATOR();
                CONSOLE_NEWLINE();
                
                /* Change Cancel button to Close now that processing is complete */
                itidy_main_progress_window_set_button_text(&progress_window, "Close");
                
                /* Clear busy pointer */
                safe_set_window_pointer(win_data->window, FALSE);
                
                /* Keep progress window open so user can review - wait for Cancel/Close */
                while (itidy_main_progress_window_handle_events(&progress_window))
                {
                    WaitPort(progress_window.window->UserPort);
                }
                
                /* Close progress window */
                itidy_main_progress_window_close(&progress_window);
            }
            break;
        
        case ITIDY_GAID_EXIT_BUTTON:
            /* Signal to close - will be handled in main loop */
            CONSOLE_STATUS("Exit button clicked\n");
            return FALSE;  /* Exit the program */
    }
    
    return TRUE;  /* Continue running */
}
