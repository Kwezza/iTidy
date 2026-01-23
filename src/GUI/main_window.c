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

/* ReAction headers */
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/checkbox.h>
#include <proto/chooser.h>
#include <proto/getfile.h>
#include <proto/label.h>

#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
#include <classes/window.h>
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
#include "easy_request_helper.h"
#include "layout_preferences.h"
#include "layout_processor.h"
#include "writeLog.h"
#include "gui_utilities.h"
#include "StatusWindows/main_progress_window.h"
#include "../backup_lha.h"
#include "../icon_types.h"  /* For ToolCacheEntry and g_ToolCache extern */

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

/* Local GadToolsBase for menus - prefixed to avoid collision with system global */
static struct Library *iTidy_GadToolsBase = NULL;

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define ITIDY_WINDOW_TITLE "iTidy v" ITIDY_VERSION " - Icon Cleanup Tool"

/*------------------------------------------------------------------------*/
/* Chooser Label String Arrays                                           */
/*------------------------------------------------------------------------*/
static STRPTR order_labels_str[] = {
    "Folders first",
    "Files first",
    "Mixed",
    "Grouped by type",
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
    "Center screen",
    "Keep position",
    "Near parent",
    "No change",
    NULL
};

/*------------------------------------------------------------------------*/
/* Menu Template                                                          */
/*------------------------------------------------------------------------*/
static struct NewMenu main_window_menu_template[] = 
{
    { NM_TITLE, "Project",      NULL, 0, 0, NULL },
    { NM_ITEM,  "New",          "N",  0, 0, (APTR)MENU_PROJECT_NEW },
    { NM_ITEM,  "Open...",      "O",  0, 0, (APTR)MENU_PROJECT_OPEN },
    { NM_ITEM,  NM_BARLABEL,    NULL, 0, 0, NULL },
    { NM_ITEM,  "Save",         "S",  0, 0, (APTR)MENU_PROJECT_SAVE },
    { NM_ITEM,  "Save as...",   "A",  0, 0, (APTR)MENU_PROJECT_SAVE_AS },
    { NM_ITEM,  NM_BARLABEL,    NULL, 0, 0, NULL },
    { NM_ITEM,  "About...",     NULL, 0, 0, (APTR)MENU_PROJECT_ABOUT },
    { NM_ITEM,  NM_BARLABEL,    NULL, 0, 0, NULL },
    { NM_ITEM,  "Quit",         "Q",  0, 0, (APTR)MENU_PROJECT_CLOSE },
    { NM_END,   NULL,           NULL, 0, 0, NULL }
};

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static struct List *create_chooser_labels(STRPTR *strings);
static void free_chooser_labels(struct List *list);
static BOOL handle_menu_selection(ULONG menu_number, struct iTidyMainWindow *win_data);
static void handle_gadget_event(ULONG gadget_id, WORD code, struct iTidyMainWindow *win_data);

/* Save/Load Preferences */
static BOOL save_preferences_to_file(const char *filepath, const LayoutPreferences *prefs);
static BOOL load_preferences_from_file(const char *filepath, LayoutPreferences *prefs);
static void sync_gui_from_preferences(struct iTidyMainWindow *win_data, const LayoutPreferences *prefs);
static void sync_gui_to_preferences(struct iTidyMainWindow *win_data, LayoutPreferences *prefs);

/* Menu Handlers */
static void handle_main_new_menu(struct iTidyMainWindow *win_data);
static void handle_main_open_menu(struct iTidyMainWindow *win_data);
static void handle_main_save_menu(struct iTidyMainWindow *win_data);
static void handle_main_save_as_menu(struct iTidyMainWindow *win_data);

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
    
    log_info(LOG_GUI, "ReAction libraries initialized successfully\n");
    return TRUE;
}

/**
 * cleanup_reaction_libs - Close all ReAction class libraries
 */
void cleanup_reaction_libs(void)
{
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
    
    /* Create the ReAction window object */
    win_data->window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title, ITIDY_WINDOW_TITLE,
        WA_Left, 50,
        WA_Top, 30,
        WA_Width, 400,
        WA_Height, 250,
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
        WINDOW_AppPort, win_data->app_port,
        WINDOW_GadgetHelp, TRUE,
        WINDOW_IconTitle, "iTidy",
        WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | 
                  IDCMP_MENUPICK | IDCMP_NEWSIZE | IDCMP_IDCMPUPDATE,
        
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
                        LABEL_Text, "Folder",
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
                            LABEL_Text, "Order",
                        TAG_END),
                        
                        /* Recursive checkbox */
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
                            LABEL_Text, "Cleanup subfolders",
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
                            LABEL_Text, "Position",
                        TAG_END),
                    TAG_END),
                    
                    /* Right column */
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_RIGHT_COLUMN] = 
                        NewObject(LAYOUT_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_RIGHT_COLUMN,
                        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                        LAYOUT_LeftSpacing, 2,
                        
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
                            LABEL_Text, "By",
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
                            LABEL_Text, "Backup icons",
                        TAG_END),
                        
                        /* Help button */
                        LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_HELP_BUTTON] = 
                            NewObject(BUTTON_GetClass(), NULL,
                            GA_ID, ITIDY_GAID_HELP_BUTTON,
                            GA_Text, "?",
                            GA_RelVerify, TRUE,
                            GA_TabCycle, TRUE,
                        TAG_END),
                        CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, "Help",
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
                        GA_Text, "Advanced",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                    TAG_END),
                    
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_DEFAULT_TOOLS_BUTTON] = 
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_DEFAULT_TOOLS_BUTTON,
                        GA_Text, "Fix default tools...",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                    TAG_END),
                    
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_RESTORE_BUTTON] = 
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_RESTORE_BUTTON,
                        GA_Text, "Restore backups",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
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
                    TAG_END),
                    
                    LAYOUT_AddChild, win_data->gadgets[ITIDY_GAD_IDX_EXIT_BUTTON] = 
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, ITIDY_GAID_EXIT_BUTTON,
                        GA_Text, "Exit",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
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
                handle_gadget_event(result & WMHI_GADGETMASK, code, win_data);
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

static BOOL save_preferences_to_file(const char *filepath, const LayoutPreferences *prefs)
{
    BPTR file;
    const char header[] = "ITIDYPREFS";
    ULONG version = 1;
    
    if (!filepath || !prefs)
    {
        log_error(LOG_GUI, "save_preferences_to_file: Invalid parameters\n");
        return FALSE;
    }
    
    file = Open((STRPTR)filepath, MODE_NEWFILE);
    if (!file)
    {
        LONG error = IoErr();
        log_error(LOG_GUI, "save_preferences_to_file: Failed to create file: %s (IoErr: %ld)\n", filepath, error);
        return FALSE;
    }
    
    log_info(LOG_GUI, "Saving preferences to: %s\n", filepath);
    
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
    
    if (Write(file, (APTR)prefs, sizeof(LayoutPreferences)) != sizeof(LayoutPreferences))
    {
        log_error(LOG_GUI, "Failed to write preferences structure\n");
        Close(file);
        return FALSE;
    }
    
    Close(file);
    log_info(LOG_GUI, "Successfully saved preferences to: %s\n", filepath);
    return TRUE;
}

static BOOL load_preferences_from_file(const char *filepath, LayoutPreferences *prefs)
{
    BPTR file;
    char header[11];
    ULONG version;
    
    if (!filepath || !prefs)
    {
        log_error(LOG_GUI, "load_preferences_from_file: Invalid parameters\n");
        return FALSE;
    }
    
    log_info(LOG_GUI, "Loading preferences from: %s\n", filepath);
    
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
    
    if (version != 1)
    {
        log_error(LOG_GUI, "Unsupported file version: %lu\n", version);
        Close(file);
        return FALSE;
    }
    
    if (Read(file, (APTR)prefs, sizeof(LayoutPreferences)) != sizeof(LayoutPreferences))
    {
        log_error(LOG_GUI, "Failed to read preferences structure\n");
        Close(file);
        return FALSE;
    }
    
    Close(file);
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
    prefs->windowPositionMode = (WindowPositionMode)win_data->window_position_selected;
    
    strncpy(prefs->folder_path, win_data->folder_path_buffer, sizeof(prefs->folder_path) - 1);
    prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
    
    prefs->backupPrefs.enableUndoBackup = win_data->enable_backup;
    
    log_debug(LOG_GUI, "Preferences updated from GUI state (folder: %s)\n", prefs->folder_path);
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
    
    InitLayoutPreferences(&default_prefs);
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
        ShowEasyRequest(win_data->window,
            "Error",
            "Failed to get preferences.",
            "OK");
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
    
    if (save_preferences_to_file(win_data->last_save_path, prefs))
    {
        log_info(LOG_GUI, "Preferences saved to: %s\n", win_data->last_save_path);
    }
    else
    {
        ShowEasyRequest(win_data->window,
            "Save Failed",
            "Failed to save preferences file.",
            "OK");
    }
}

static void handle_main_save_as_menu(struct iTidyMainWindow *win_data)
{
    struct FileRequester *freq;
    char full_path[512];
    BPTR lock;
    LayoutPreferences *prefs;
    
    if (!win_data || !win_data->window)
        return;
    
    prefs = (LayoutPreferences *)GetGlobalPreferences();
    if (!prefs)
    {
        ShowEasyRequest(win_data->window,
            "Error",
            "Failed to get preferences.",
            "OK");
        return;
    }
    
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Save Preferences As...",
        ASLFR_InitialDrawer, "PROGDIR:userdata/Settings",
        ASLFR_InitialFile, "iTidy.prefs",
        ASLFR_DoSaveMode, TRUE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, win_data->window,
        TAG_END);
    
    if (!freq)
    {
        ShowEasyRequest(win_data->window,
            "Error",
            "Could not open file requester.",
            "OK");
        return;
    }
    
    if (AslRequest(freq, NULL))
    {
        strcpy(full_path, freq->fr_Drawer);
        if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            FreeAslRequest(freq);
            ShowEasyRequest(win_data->window,
                "Error",
                "File path is too long.",
                "OK");
            return;
        }
        
        lock = Lock((STRPTR)full_path, ACCESS_READ);
        if (lock)
        {
            UnLock(lock);
            
            if (!ShowEasyRequest(win_data->window,
                "File Exists",
                "File already exists.\nDo you want to replace it?",
                "Replace|Cancel"))
            {
                FreeAslRequest(freq);
                return;
            }
        }
        
        sync_gui_to_preferences(win_data, prefs);
        UpdateGlobalPreferences(prefs);
        
        if (save_preferences_to_file(full_path, prefs))
        {
            strncpy(win_data->last_save_path, full_path, sizeof(win_data->last_save_path) - 1);
            win_data->last_save_path[sizeof(win_data->last_save_path) - 1] = '\0';
            
            ShowEasyRequest(win_data->window,
                "Save Successful",
                "Preferences saved successfully.",
                "OK");
            log_info(LOG_GUI, "Preferences saved to: %s\n", full_path);
        }
        else
        {
            ShowEasyRequest(win_data->window,
                "Save Failed",
                "Failed to save preferences file.",
                "OK");
        }
    }
    
    FreeAslRequest(freq);
}

static void handle_main_open_menu(struct iTidyMainWindow *win_data)
{
    struct FileRequester *freq;
    char full_path[512];
    BPTR lock;
    LayoutPreferences loaded_prefs;
    
    if (!win_data || !win_data->window)
        return;
    
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Open Preferences File...",
        ASLFR_InitialDrawer, "PROGDIR:userdata/Settings",
        ASLFR_InitialFile, "iTidy.prefs",
        ASLFR_DoSaveMode, FALSE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, win_data->window,
        TAG_END);
    
    if (!freq)
    {
        ShowEasyRequest(win_data->window,
            "Error",
            "Could not open file requester.",
            "OK");
        return;
    }
    
    if (AslRequest(freq, NULL))
    {
        strcpy(full_path, freq->fr_Drawer);
        if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            FreeAslRequest(freq);
            ShowEasyRequest(win_data->window,
                "Error",
                "File path is too long.",
                "OK");
            return;
        }
        
        lock = Lock((STRPTR)full_path, ACCESS_READ);
        if (!lock)
        {
            FreeAslRequest(freq);
            ShowEasyRequest(win_data->window,
                "File Not Found",
                "The selected file does not exist.",
                "OK");
            return;
        }
        UnLock(lock);
        
        if (load_preferences_from_file(full_path, &loaded_prefs))
        {
            UpdateGlobalPreferences(&loaded_prefs);
            sync_gui_from_preferences(win_data, &loaded_prefs);
            
            strncpy(win_data->last_save_path, full_path, sizeof(win_data->last_save_path) - 1);
            win_data->last_save_path[sizeof(win_data->last_save_path) - 1] = '\0';
            
            ShowEasyRequest(win_data->window,
                "Load Successful",
                "Preferences loaded successfully.",
                "OK");
            log_info(LOG_GUI, "Preferences loaded from: %s\n", full_path);
        }
        else
        {
            ShowEasyRequest(win_data->window,
                "Load Failed",
                "Failed to load preferences file.\nFile may be corrupted or invalid.",
                "OK");
        }
    }
    
    FreeAslRequest(freq);
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
                case MENU_PROJECT_NEW:
                    handle_main_new_menu(win_data);
                    break;
                
                case MENU_PROJECT_OPEN:
                    handle_main_open_menu(win_data);
                    break;
                
                case MENU_PROJECT_SAVE:
                    handle_main_save_menu(win_data);
                    break;
                
                case MENU_PROJECT_SAVE_AS:
                    handle_main_save_as_menu(win_data);
                    break;
                
                case MENU_PROJECT_ABOUT:
                    ShowEasyRequest(win_data->window,
                        "About iTidy",
                        "iTidy v" ITIDY_VERSION "\n\n"
                        "Icon Cleanup Tool for AmigaOS\n"
                        "ReAction GUI Version (WB 3.2+)\n\n"
                        "Automatically arranges icon layouts\n"
                        "and resizes folder windows.\n\n"
                        "(c) 2025-2026",
                        "OK");
                    break;
                
                case MENU_PROJECT_CLOSE:
                    continue_running = FALSE;
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

static void handle_gadget_event(ULONG gadget_id, WORD code, struct iTidyMainWindow *win_data)
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
                CONSOLE_DEBUG("Backup: %s\n", win_data->enable_backup ? "ON" : "OFF");
            }
            break;
        
        case ITIDY_GAID_POSITION_CHOOSER:
            win_data->window_position_selected = code;
            CONSOLE_DEBUG("Position changed to: %s\n", position_labels_str[code]);
            break;
        
        case ITIDY_GAID_HELP_BUTTON:
            ShowEasyRequest(win_data->window,
                "Window Placement Options",
                "Center screen - Resizes window and centers it.\n\n"
                "Keep position - Resizes but keeps current position.\n\n"
                "Near parent - Places window near its parent drawer.\n\n"
                "No change - Only rearranges icons, no window change.",
                "OK");
            break;
        
        case ITIDY_GAID_ADVANCED_BUTTON:
            {
                struct iTidyAdvancedWindow adv_data;
                LayoutPreferences temp_prefs;
                
                CONSOLE_STATUS("Advanced button clicked - opening Advanced Settings window\n");
                
                /* Get current global preferences (includes all current settings) */
                memcpy(&temp_prefs, GetGlobalPreferences(), sizeof(LayoutPreferences));
                
                /* Open advanced window (modal) */
                if (open_itidy_advanced_window(&adv_data, &temp_prefs))
                {
                    /* Disable main window input while advanced window is open */
                    /* ReAction: We can't use ModifyIDCMP on ReAction windows, so we just don't process events */
                    
                    /* Run advanced window event loop */
                    while (handle_advanced_window_events(&adv_data))
                    {
                        /* Wait for advanced window events */
                        WaitPort(adv_data.window->UserPort);
                    }
                    
                    /* Close advanced window */
                    close_itidy_advanced_window(&adv_data);
                    
                    /* If changes were accepted, update global preferences */
                    if (adv_data.changes_accepted)
                    {
                        CONSOLE_STATUS("Advanced settings accepted - updating global preferences\n");
                        
                        /* Update global preferences with advanced settings */
                        UpdateGlobalPreferences(&temp_prefs);
                        
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
                    /* Event loop */
                    while (handle_tool_cache_window_events(&tool_window))
                    {
                        WaitPort(tool_window.window->UserPort);
                    }
                    
                    /* Cleanup */
                    close_tool_cache_window(&tool_window);
                    log_info(LOG_GUI, "Tool cache window closed\n");
                }
                else
                {
                    log_error(LOG_GUI, "Failed to open tool cache window\n");
                    (void)ShowEasyRequest(
                        win_data->window,
                        "Error",
                        "Failed to open tool cache window.\n"
                        "Check the log for details.",
                        "OK");
                }
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
                    
                    /* Run restore window event loop */
                    while (handle_restore_window_events(&restore_data))
                    {
                        /* Wait for restore window events */
                        WaitPort(restore_data.window->UserPort);
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
                        LONG result = ShowEasyRequest(win_data->window,
                            "LHA Not Found",
                            "LHA archiver not found.\n"
                            "Backups cannot be created.\n\n"
                            "Continue without backups?",
                            "Continue|Cancel");
                        
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
                    ShowEasyRequest(win_data->window,
                        "Error",
                        "Failed to open progress window",
                        "OK");
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
            /* Post a close window message */
            CONSOLE_STATUS("Exit button clicked\n");
            /* Use RA_HandleInput return to signal exit */
            break;
    }
}
