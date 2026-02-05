/*
 * deficons_settings_window.c - DefIcons Type Selection Window (ReAction)
 * 
 * GUI window for selecting which DefIcons file types should have automatic
 * icon creation enabled. Uses hierarchical ListBrowser to display type tree.
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

/* Library base isolation - prevent linker conflicts with other windows */
#define WindowBase iTidy_DefIcons_WindowBase
#define LayoutBase iTidy_DefIcons_LayoutBase
#define ListBrowserBase iTidy_DefIcons_ListBrowserBase
#define ChooserBase iTidy_DefIcons_ChooserBase
#define ButtonBase iTidy_DefIcons_ButtonBase
#define LabelBase iTidy_DefIcons_LabelBase

#include "deficons_settings_window.h"
#include "writeLog.h"
#include "deficons_parser.h"
#include "platform/platform.h"

#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/chooser.h>
#include <proto/button.h>
#include <proto/label.h>
#include <proto/utility.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <gadgets/chooser.h>
#include <gadgets/button.h>
#include <images/label.h>
#include <libraries/gadtools.h>

#include <string.h>
#include <stdio.h>

#ifndef ITIDY_LBRE_CHECKED
#define ITIDY_LBRE_CHECKED LBRE_CHECKED
#endif

#ifndef ITIDY_LBRE_UNCHECKED
#ifdef LBRE_UNCHECKED_
#define ITIDY_LBRE_UNCHECKED LBRE_UNCHECKED_
#else
#define ITIDY_LBRE_UNCHECKED LBRE_UNCHECKED
#endif
#endif

/* Local library bases */
struct Library *iTidy_DefIcons_WindowBase = NULL;
struct Library *iTidy_DefIcons_LayoutBase = NULL;
struct Library *iTidy_DefIcons_ListBrowserBase = NULL;
struct Library *iTidy_DefIcons_ChooserBase = NULL;
struct Library *iTidy_DefIcons_ButtonBase = NULL;
struct Library *iTidy_DefIcons_LabelBase = NULL;

/* Gadget IDs */
enum {
    GID_TYPE_TREE = 1,
    GID_SELECT_ALL,
    GID_SELECT_NONE,
    GID_FOLDER_MODE_CHOOSER,
    GID_OK,
    GID_CANCEL
};

/* External reference to global DefIcons cache from main_gui.c */
extern DeficonTypeTreeNode *g_cached_deficons_tree;
extern int g_cached_deficons_count;

/*
 * Internal window structure
 */
typedef struct {
    /* ReAction objects */
    Object *window_obj;
    Object *main_layout;
    Object *tree_listbrowser;
    Object *chooser_obj;
    Object *select_all_btn;
    Object *select_none_btn;
    Object *ok_btn;
    Object *cancel_btn;
    
    struct Window *window;
    struct List *tree_list;
    struct ColumnInfo *column_info;
    
    /* Working copy of preferences */
    LayoutPreferences *prefs;
    
    /* Result */
    BOOL user_accepted;
    
} DefIconsSettingsWindow;

/*
 * Open ReAction libraries
 */
static BOOL open_reaction_libs(void)
{
    iTidy_DefIcons_WindowBase = OpenLibrary("window.class", 44);
    iTidy_DefIcons_LayoutBase = OpenLibrary("gadgets/layout.gadget", 44);
    iTidy_DefIcons_ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 44);
    iTidy_DefIcons_ChooserBase = OpenLibrary("gadgets/chooser.gadget", 44);
    iTidy_DefIcons_ButtonBase = OpenLibrary("gadgets/button.gadget", 44);
    iTidy_DefIcons_LabelBase = OpenLibrary("images/label.image", 44);
    
    if (!iTidy_DefIcons_WindowBase || !iTidy_DefIcons_LayoutBase ||
        !iTidy_DefIcons_ListBrowserBase || !iTidy_DefIcons_ChooserBase ||
        !iTidy_DefIcons_ButtonBase || !iTidy_DefIcons_LabelBase)
    {
        return FALSE;
    }
    
    return TRUE;
}

/*
 * Close ReAction libraries
 */
static void close_reaction_libs(void)
{
    if (iTidy_DefIcons_LabelBase)
    {
        CloseLibrary(iTidy_DefIcons_LabelBase);
        iTidy_DefIcons_LabelBase = NULL;
    }
    if (iTidy_DefIcons_ButtonBase)
    {
        CloseLibrary(iTidy_DefIcons_ButtonBase);
        iTidy_DefIcons_ButtonBase = NULL;
    }
    if (iTidy_DefIcons_ChooserBase)
    {
        CloseLibrary(iTidy_DefIcons_ChooserBase);
        iTidy_DefIcons_ChooserBase = NULL;
    }
    if (iTidy_DefIcons_ListBrowserBase)
    {
        CloseLibrary(iTidy_DefIcons_ListBrowserBase);
        iTidy_DefIcons_ListBrowserBase = NULL;
    }
    if (iTidy_DefIcons_LayoutBase)
    {
        CloseLibrary(iTidy_DefIcons_LayoutBase);
        iTidy_DefIcons_LayoutBase = NULL;
    }
    if (iTidy_DefIcons_WindowBase)
    {
        CloseLibrary(iTidy_DefIcons_WindowBase);
        iTidy_DefIcons_WindowBase = NULL;
    }
}

/*
 * CRITICAL: Hide all child nodes before creating ListBrowser
 * ListBrowser crashes if hierarchical nodes aren't properly hidden at creation
 */
static void hide_all_listbrowser_children(struct List *list)
{
    if (list == NULL)
    {
        return;
    }
    
    /* Use ListBrowser API to hide children (avoids flag value ambiguity) */
    HideAllListBrowserChildren(list);
}

/*
 * Build hierarchical ListBrowser node list from DefIcons cache
 */
static BOOL is_deficon_type_checked_for_ui(const LayoutPreferences *prefs, const char *type_name)
{
    char *ptr;
    char temp_buffer[256];
    char search_token[MAX_DEFICONS_TYPE_NAME + 3];  /* ",type_name," */
    
    if (prefs == NULL || type_name == NULL || type_name[0] == '\0')
        return FALSE;
    
    /* If no disabled types, all are checked */
    if (prefs->deficons_disabled_types[0] == '\0')
        return TRUE;
    
    /* Build search token: ",type_name," to avoid partial matches */
    sprintf(search_token, ",%s,", type_name);
    sprintf(temp_buffer, ",%s,", prefs->deficons_disabled_types);
    
    ptr = strstr(temp_buffer, search_token);
    return (ptr == NULL);  /* TRUE if NOT found in disabled list */
}

static struct List* build_tree_list(LayoutPreferences *prefs)
{
    struct List *list;
    struct Node *node;
    int i;
    char display_text[128];
    
    if (g_cached_deficons_tree == NULL || g_cached_deficons_count == 0)
    {
        log_error(LOG_GUI, "DefIcons cache not available\n");
        return NULL;
    }
    
    /* Create empty list */
    list = (struct List *)whd_malloc(sizeof(struct List));
    if (list)
    {
        NewList(list);
    }
    
    if (list == NULL)
    {
        log_error(LOG_GUI, "Failed to allocate ListBrowser list\n");
        return NULL;
    }
    
    /* Build nodes from DefIcons tree */
    for (i = 0; i < g_cached_deficons_count; i++)
    {
        DeficonTypeTreeNode *tree_node = &g_cached_deficons_tree[i];
        BOOL is_enabled = is_deficon_type_checked_for_ui(prefs, tree_node->type_name);
        ULONG node_flags = 0;
        
        /* Format display text based on generation (generations start at 1) */
        if (tree_node->generation == 1)
        {
            /* Root nodes: Show type name with checkbox */
            sprintf(display_text, "%s", tree_node->type_name);
        }
        else
        {
            /* Child nodes: Show as sub-items (informational only) */
            sprintf(display_text, "  %s", tree_node->type_name);
        }
        
        /* Set flags */
        if (tree_node->has_children)
        {
            node_flags |= LBFLG_HASCHILDREN;
        }
        
        /* Only second-level nodes get checkboxes */
        if (tree_node->generation == 2)
        {
            /* Note: Selection state will be set via LBNA_Selected attribute below */
        }
        else
        {
            /* Child nodes are hidden initially (handled via hide function) */
        }
        
        /* Create node */
        node = AllocListBrowserNode(1,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, display_text,
                LBNCA_Editable, (tree_node->generation == 1) ? TRUE : FALSE,
            LBNA_Generation, tree_node->generation,
            LBNA_Flags, node_flags,
            LBNA_CheckBox, (tree_node->generation == 2) ? TRUE : FALSE,
            LBNA_Checked, (tree_node->generation == 2 && is_enabled) ? TRUE : FALSE,
            LBNA_UserData, (APTR)tree_node,
            TAG_DONE);
        
        if (node == NULL)
        {
            log_error(LOG_GUI, "Failed to allocate ListBrowser node\n");
            FreeListBrowserList(list);
            return NULL;
        }
        
        AddTail(list, node);
    }
    
    log_debug(LOG_GUI, "Built ListBrowser tree with %d nodes\n", g_cached_deficons_count);
    
    /* CRITICAL: Hide all children before returning */
    hide_all_listbrowser_children(list);
    
    /* Debug: Log list contents */
    log_debug(LOG_GUI, "ListBrowser list after build:\n");
    for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        ULONG gen = 0;
        ULONG flags = 0;
        BOOL selected = FALSE;
        STRPTR text = NULL;
        
        GetListBrowserNodeAttrs(node,
            LBNA_Generation, &gen,
            LBNA_Flags, &flags,
            LBNA_Selected, &selected,
            LBNA_Column, 0,
                LBNCA_Text, &text,
            TAG_DONE);
        
        log_debug(LOG_GUI, "  Node: gen=%lu, flags=0x%08lx, sel=%d, text=%s\n",
                  gen, flags, selected, text ? text : "(null)");
    }
    
    return list;
}

/*
 * Free ListBrowser node list
 */
static void free_tree_list(struct List *list)
{
    struct Node *node;
    struct Node *next;
    
    if (list == NULL)
        return;
    
    /* Free all nodes */
    for (node = list->lh_Head; node->ln_Succ; node = next)
    {
        next = node->ln_Succ;
        FreeListBrowserNode(node);
    }
    
    /* Free list structure */
    whd_free(list);
}

/* Folder mode chooser labels (static array - no allocation needed) */
static STRPTR folder_mode_labels[] = {
    "Smart (create if visible)",
    "Always create",
    "Never create",
    NULL
};

/*
 * Create the window
 */
static BOOL create_window(DefIconsSettingsWindow *win)
{
    /* Build tree list */
    win->tree_list = build_tree_list(win->prefs);
    if (win->tree_list == NULL)
    {
        log_error(LOG_GUI, "Failed to build tree list\n");
        return FALSE;
    }
    
    /* Allocate column info (single column with terminator) */
    win->column_info = (struct ColumnInfo *)AllocMem(sizeof(struct ColumnInfo) * 2, MEMF_PUBLIC | MEMF_CLEAR);
    if (win->column_info == NULL)
    {
        log_error(LOG_GUI, "Failed to allocate column info\n");
        FreeListBrowserList(win->tree_list);
        win->tree_list = NULL;
        return FALSE;
    }
    
    /* Configure single column for type names */
    win->column_info[0].ci_Width = 100;  /* Weighted width */
    win->column_info[0].ci_Title = "DefIcon Type";
    win->column_info[0].ci_Flags = CIF_WEIGHTED;
    
    /* Terminator entry (CRITICAL!) */
    win->column_info[1].ci_Width = -1;
    win->column_info[1].ci_Title = (STRPTR)~0;
    win->column_info[1].ci_Flags = -1;
    
    /* Create gadgets */
    win->tree_listbrowser = (Object *)ListBrowserObject,
        GA_ID, GID_TYPE_TREE,
        GA_RelVerify, TRUE,
        GA_TabCycle, TRUE,
        LISTBROWSER_Labels, win->tree_list,
        LISTBROWSER_Hierarchical, TRUE,
        LISTBROWSER_ShowSelected, TRUE,
        LISTBROWSER_AutoFit, TRUE,
        LISTBROWSER_ColumnInfo, win->column_info,
        LISTBROWSER_ColumnTitles, TRUE,
        LISTBROWSER_TitleClickable, FALSE,
    ListBrowserEnd;
    
    win->chooser_obj = (Object *)ChooserObject,
        GA_ID, GID_FOLDER_MODE_CHOOSER,
        GA_RelVerify, TRUE,
        CHOOSER_LabelArray, folder_mode_labels,
        CHOOSER_Selected, win->prefs->deficons_folder_icon_mode,
    ChooserEnd;
    
    win->select_all_btn = (Object *)ButtonObject,
        GA_ID, GID_SELECT_ALL,
        GA_Text, "Select _All",
        GA_RelVerify, TRUE,
    ButtonEnd;
    
    win->select_none_btn = (Object *)ButtonObject,
        GA_ID, GID_SELECT_NONE,
        GA_Text, "Select _None",
        GA_RelVerify, TRUE,
    ButtonEnd;
    
    win->ok_btn = (Object *)ButtonObject,
        GA_ID, GID_OK,
        GA_Text, "_OK",
        GA_RelVerify, TRUE,
    ButtonEnd;
    
    win->cancel_btn = (Object *)ButtonObject,
        GA_ID, GID_CANCEL,
        GA_Text, "_Cancel",
        GA_RelVerify, TRUE,
    ButtonEnd;
    
    /* Create layout */
    win->main_layout = (Object *)VLayoutObject,
        LAYOUT_AddChild, (Object *)LabelObject,
            LABEL_Text, "Select file types for automatic icon creation:",
        LabelEnd,
        CHILD_WeightedHeight, 0,
        
        LAYOUT_AddChild, win->tree_listbrowser,
        CHILD_WeightedHeight, 100,
        
        LAYOUT_AddChild, (Object *)HLayoutObject,
            LAYOUT_AddChild, (Object *)LabelObject,
                LABEL_Text, "Folder Icons:",
            LabelEnd,
            CHILD_WeightedWidth, 0,
            
            LAYOUT_AddChild, win->chooser_obj,
            CHILD_WeightedWidth, 100,
        LayoutEnd,
        CHILD_WeightedHeight, 0,
        
        LAYOUT_AddChild, (Object *)HLayoutObject,
            LAYOUT_AddChild, win->select_all_btn,
            LAYOUT_AddChild, win->select_none_btn,
        LayoutEnd,
        CHILD_WeightedHeight, 0,
        
        LAYOUT_AddChild, (Object *)HLayoutObject,
            LAYOUT_AddChild, win->ok_btn,
            LAYOUT_AddChild, win->cancel_btn,
        LayoutEnd,
        CHILD_WeightedHeight, 0,
    LayoutEnd;
    
    /* Create window */
    win->window_obj = (Object *)WindowObject,
        WA_Title, "DefIcons: Icon Type Selection",
        WA_Activate, TRUE,
        WA_DepthGadget, TRUE,
        WA_DragBar, TRUE,
        WA_CloseGadget, TRUE,
        WA_SizeGadget, FALSE,
        WA_Width, 400,
        WA_Height, 400,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_Layout, win->main_layout,
    WindowEnd;
    
    if (win->window_obj == NULL)
    {
        log_error(LOG_GUI, "Failed to create window object\n");
        if (win->column_info)
        {
            FreeMem(win->column_info, sizeof(struct ColumnInfo) * 2);
        }
        free_tree_list(win->tree_list);
        return FALSE;
    }
    
    /* Open window */
    win->window = (struct Window *)RA_OpenWindow(win->window_obj);
    if (win->window == NULL)
    {
        log_error(LOG_GUI, "Failed to open window\n");
        DisposeObject(win->window_obj);
        win->window_obj = NULL;
        if (win->column_info)
        {
            FreeMem(win->column_info, sizeof(struct ColumnInfo) * 2);
        }
        free_tree_list(win->tree_list);
        return FALSE;
    }
    
    /* Ensure list is attached and log listbrowser stats */
    SetGadgetAttrs((struct Gadget *)win->tree_listbrowser, win->window, NULL,
        LISTBROWSER_Labels, win->tree_list,
        LISTBROWSER_ColumnInfo, win->column_info,
        TAG_DONE);
    
    {
        ULONG total_nodes = 0;
        ULONG visible_nodes = 0;
        ULONG gadget_width = 0;
        ULONG gadget_height = 0;
        
        GetAttr(LISTBROWSER_TotalNodes, win->tree_listbrowser, &total_nodes);
        GetAttr(LISTBROWSER_TotalVisibleNodes, win->tree_listbrowser, &visible_nodes);
        GetAttr(GA_Width, win->tree_listbrowser, &gadget_width);
        GetAttr(GA_Height, win->tree_listbrowser, &gadget_height);
        
        log_debug(LOG_GUI, "ListBrowser stats: total=%lu, visible=%lu, size=%lux%lu\n",
                  total_nodes, visible_nodes, gadget_width, gadget_height);
    }
    
    log_debug(LOG_GUI, "DefIcons settings window opened\n");
    return TRUE;
}

/*
 * Handle Select All button
 */
static void handle_select_all(DefIconsSettingsWindow *win)
{
    struct Node *node;
    int i;
    
    /* Enable all root types in preferences */
    clear_disabled_deficon_types(win->prefs);
    
    /* Update UI - check all root nodes */
    for (node = win->tree_list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        ULONG generation = 0;
        
        GetListBrowserNodeAttrs(node,
            LBNA_Generation, &generation,
            TAG_DONE);
        
        if (generation == 2)
        {
            SetListBrowserNodeAttrs(node,
                LBNA_Checked, TRUE,
                TAG_DONE);
        }
    }
    
    /* Refresh display */
    RefreshGadgets((struct Gadget *)win->tree_listbrowser, win->window, NULL);
    
    log_debug(LOG_GUI, "Selected all DefIcons types\n");
}

/*
 * Handle Select None button
 */
static void handle_select_none(DefIconsSettingsWindow *win)
{
    struct Node *node;
    int i;
    
    /* Disable all root types in preferences */
    for (i = 0; i < g_cached_deficons_count; i++)
    {
        DeficonTypeTreeNode *tree_node = &g_cached_deficons_tree[i];
        if (tree_node->generation == 2)
        {
            add_disabled_deficon_type(win->prefs, tree_node->type_name);
        }
    }
    
    /* Update UI - uncheck all root nodes */
    for (node = win->tree_list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        ULONG generation = 0;
        
        GetListBrowserNodeAttrs(node,
            LBNA_Generation, &generation,
            TAG_DONE);
        
        if (generation == 2)
        {
            SetListBrowserNodeAttrs(node,
                LBNA_Checked, FALSE,
                TAG_DONE);
        }
    }
    
    /* Refresh display */
    RefreshGadgets((struct Gadget *)win->tree_listbrowser, win->window, NULL);
    
    log_debug(LOG_GUI, "Deselected all DefIcons types\n");
}

/*
 * Handle ListBrowser click (toggle checkbox on root nodes)
 */
static void handle_tree_click(DefIconsSettingsWindow *win)
{
    struct Node *cursor_node = NULL;
    DeficonTypeTreeNode *tree_node;
    ULONG generation = 0;
    BOOL is_checked = FALSE;
    ULONG rel_event = 0;
    
    /* Get release event and cursor node */
    GetAttr(LISTBROWSER_RelEvent, win->tree_listbrowser, &rel_event);
    GetAttr(LISTBROWSER_CursorNode, win->tree_listbrowser, (ULONG *)&cursor_node);
    if (cursor_node == NULL)
        return;
    
    /* Get node data */
    GetListBrowserNodeAttrs(cursor_node,
        LBNA_Generation, &generation,
        LBNA_Checked, &is_checked,
        LBNA_UserData, &tree_node,
        TAG_DONE);
    
    /* Only allow toggling second-level nodes */
    if (generation != 2 || tree_node == NULL)
        return;
    
    /* Update preferences based on checkbox event */
    if (rel_event == ITIDY_LBRE_CHECKED)
    {
        /* Checkbox is now checked - enable this type */
        remove_disabled_deficon_type(win->prefs, tree_node->type_name);
        log_debug(LOG_GUI, "Enabled type: %s\n", tree_node->type_name);
    }
    else if (rel_event == ITIDY_LBRE_UNCHECKED)
    {
        /* Checkbox is now unchecked - disable this type */
        add_disabled_deficon_type(win->prefs, tree_node->type_name);
        log_debug(LOG_GUI, "Disabled type: %s\n", tree_node->type_name);
    }
    else
    {
        /* Ignore non-checkbox events */
        return;
    }
    
    RefreshGadgets((struct Gadget *)win->tree_listbrowser, win->window, NULL);
}

/*
 * Handle OK button - save changes
 */
static void handle_ok(DefIconsSettingsWindow *win)
{
    ULONG selected_mode = 0;
    struct Node *node;
    
    /* Get folder icon mode from chooser */
    GetAttr(CHOOSER_Selected, win->chooser_obj, &selected_mode);
    win->prefs->deficons_folder_icon_mode = (UWORD)selected_mode;
    
    /* Rebuild disabled types from current checkbox state (source of truth) */
    clear_disabled_deficon_types(win->prefs);
    for (node = win->tree_list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        ULONG generation = 0;
        BOOL is_checked = FALSE;
        DeficonTypeTreeNode *tree_node = NULL;
        
        GetListBrowserNodeAttrs(node,
            LBNA_Generation, &generation,
            LBNA_Checked, &is_checked,
            LBNA_UserData, &tree_node,
            TAG_DONE);
        
        if (generation == 2 && tree_node != NULL && !is_checked)
        {
            add_disabled_deficon_type(win->prefs, tree_node->type_name);
        }
    }

    /* Debug: dump checkbox states */
    log_debug(LOG_GUI, "DefIcons checkbox states on OK:\n");
    for (node = win->tree_list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        ULONG generation = 0;
        BOOL is_checked = FALSE;
        DeficonTypeTreeNode *tree_node = NULL;
        
        GetListBrowserNodeAttrs(node,
            LBNA_Generation, &generation,
            LBNA_Checked, &is_checked,
            LBNA_UserData, &tree_node,
            TAG_DONE);
        
        if (generation == 2 && tree_node != NULL)
        {
            log_debug(LOG_GUI, "  [%c] %s\n", is_checked ? 'X' : ' ', tree_node->type_name);
        }
    }
    
    log_info(LOG_GUI, "DefIcons settings saved: folder_mode=%d, disabled_types='%s'\n",
             win->prefs->deficons_folder_icon_mode,
             win->prefs->deficons_disabled_types);
    
    win->user_accepted = TRUE;
}

/*
 * Event loop
 */
static void run_event_loop(DefIconsSettingsWindow *win)
{
    BOOL done = FALSE;
    ULONG signal_mask = 0;
    ULONG signals;
    ULONG result;
    UWORD code;
    
    GetAttr(WINDOW_SigMask, win->window_obj, &signal_mask);
    
    while (!done)
    {
        signals = Wait(signal_mask | SIGBREAKF_CTRL_C);
        
        if (signals & SIGBREAKF_CTRL_C)
        {
            done = TRUE;
            continue;
        }
        
        while ((result = RA_HandleInput(win->window_obj, &code)) != WMHI_LASTMSG)
        {
            switch (result & WMHI_CLASSMASK)
            {
                case WMHI_CLOSEWINDOW:
                    done = TRUE;
                    break;
                
                case WMHI_GADGETUP:
                    switch (result & WMHI_GADGETMASK)
                    {
                        case GID_TYPE_TREE:
                            handle_tree_click(win);
                            break;
                        
                        case GID_SELECT_ALL:
                            handle_select_all(win);
                            break;
                        
                        case GID_SELECT_NONE:
                            handle_select_none(win);
                            break;
                        
                        case GID_OK:
                            handle_ok(win);
                            done = TRUE;
                            break;
                        
                        case GID_CANCEL:
                            done = TRUE;
                            break;
                    }
                    break;
            }
        }
    }
}

/*
 * Public API: Open DefIcons settings window
 */
BOOL open_itidy_deficons_settings_window(LayoutPreferences *prefs)
{
    DefIconsSettingsWindow win;
    BOOL result;
    
    if (prefs == NULL)
    {
        log_error(LOG_GUI, "NULL preferences pointer\n");
        return FALSE;
    }
    
    /* Check if DefIcons cache is available */
    if (g_cached_deficons_tree == NULL || g_cached_deficons_count == 0)
    {
        log_warning(LOG_GUI, "DefIcons cache not available\n");
        return FALSE;
    }
    
    /* Initialize structure */
    memset(&win, 0, sizeof(DefIconsSettingsWindow));
    win.prefs = prefs;
    win.user_accepted = FALSE;
    
    /* Open ReAction libraries */
    if (!open_reaction_libs())
    {
        log_error(LOG_GUI, "Failed to open ReAction libraries\n");
        return FALSE;
    }
    
    /* Create window */
    if (!create_window(&win))
    {
        close_reaction_libs();
        return FALSE;
    }
    
    /* Run event loop */
    run_event_loop(&win);
    
    /* Get result before cleanup */
    result = win.user_accepted;
    
    /* Cleanup */
    if (win.window_obj)
    {
        DisposeObject(win.window_obj);
    }
    
    /* Free column info */
    if (win.column_info)
    {
        FreeMem(win.column_info, sizeof(struct ColumnInfo) * 2);
        win.column_info = NULL;
    }
    
    free_tree_list(win.tree_list);
    close_reaction_libs();
    
    return result;
}
