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
#include "deficons_templates.h"
#include "icon_types.h"
#include "platform/platform.h"
#include "easy_request_helper.h"

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
#include <proto/asl.h>
#include <proto/dos.h>

#include <classes/window.h>
#include <libraries/asl.h>
#include <dos/dos.h>
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
    GID_SHOW_TOOLS,
    GID_CHANGE_DEFAULT_TOOL,
    GID_FOLDER_MODE_CHOOSER,
    GID_ICON_SIZE_CHOOSER,
    GID_PALETTE_MODE_CHOOSER,
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
    Object *icon_size_chooser_obj;
    Object *palette_mode_chooser_obj;
    Object *select_all_btn;
    Object *select_none_btn;
    Object *show_tools_btn;
    Object *change_default_tool_btn;
    Object *ok_btn;
    Object *cancel_btn;
    
    struct Window *window;
    struct List *tree_list;
    struct ColumnInfo *column_info;
    
    /* Working copy of preferences */
    LayoutPreferences *prefs;
    
    /* State */
    BOOL user_accepted;
    BOOL is_showing_tools;
    
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
 * Expand generation 1 nodes to show generation 2 (the checkbox level)
 * This makes the checkboxes visible on window open.
 */
static void expand_root_nodes(struct List *list)
{
    struct Node *node;
    
    if (list == NULL)
    {
        return;
    }
    
    /* Find all generation 1 nodes (root level) and show their children */
    for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        ULONG generation = 0;
        ULONG flags = 0;
        
        GetListBrowserNodeAttrs(node,
            LBNA_Generation, &generation,
            LBNA_Flags, &flags,
            TAG_DONE);
        
        /* If this is a generation 1 node with children, show them */
        if (generation == 1 && (flags & LBFLG_HASCHILDREN))
        {
            /* Show only next generation (depth = 1) */
            ShowListBrowserNodeChildren(node, 1);
            log_debug(LOG_GUI, "Expanded root node to show generation 2 checkboxes\n");
        }
    }
}

/*
 * Look up default tool for a deficon type by locating its template icon
 * ONLY returns a tool if the DIRECT def_<type>.info file exists
 * Does NOT fall back to parent (to show inheritance clearly in UI)
 * Returns allocated string (caller must free) or NULL if no default tool
 */
static char* lookup_deficon_default_tool(const char *type_name)
{
    char template_path[512];
    char *default_tool = NULL;
    IconDetailsFromDisk details;
    BPTR lock;
    
    if (type_name == NULL || type_name[0] == '\0')
    {
        log_debug(LOG_GUI, "lookup_deficon_default_tool: NULL or empty type_name\n");
        return NULL;
    }
    
    log_debug(LOG_GUI, "lookup_deficon_default_tool: Looking up type '%s'\n", type_name);
    
    /* Build direct path to template: ENVARC:Sys/def_<type>.info */
    snprintf(template_path, sizeof(template_path), "ENVARC:Sys/def_%s.info", type_name);
    
    log_debug(LOG_GUI, "  Checking for direct template: %s\n", template_path);
    
    /* Check if the direct template file exists (don't fall back to parent!) */
    lock = Lock((STRPTR)template_path, ACCESS_READ);
    if (!lock)
    {
        log_debug(LOG_GUI, "  Direct template does not exist: %s\n", template_path);
        log_debug(LOG_GUI, "  (Not checking parent - want to show inheritance clearly)\n");
        return NULL;
    }
    UnLock(lock);
    
    log_info(LOG_GUI, "  >>> Direct template exists: %s\n", template_path);
    
    /* Load icon and extract default tool */
    log_info(LOG_GUI, "  Loading icon from: %s.info\n", template_path);
    if (GetIconDetailsFromDisk(template_path, &details, NULL))
    {
        if (details.defaultTool != NULL && details.defaultTool[0] != '\0')
        {
            log_info(LOG_GUI, "  >>> Found default tool for '%s': %s\n", type_name, details.defaultTool);
            
            /* Make a copy since details.defaultTool will be freed */
            size_t len = strlen(details.defaultTool);
            default_tool = (char *)whd_malloc(len + 1);
            if (default_tool != NULL)
            {
                strcpy(default_tool, details.defaultTool);
            }
            
            /* Free the default tool from GetIconDetailsFromDisk */
            whd_free(details.defaultTool);
        }
        else
        {
            log_debug(LOG_GUI, "  No default tool set in icon: %s\n", template_path);
        }
    }
    else
    {
        log_warning(LOG_GUI, "  Failed to load icon details from: %s\n", template_path);
    }
    
    return default_tool;
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

static struct List* build_tree_list(LayoutPreferences *prefs, BOOL show_tools)
{
    struct List *list;
    struct Node *node;
    int i;
    char display_text[256];
    char *default_tool = NULL;
    
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
    log_debug(LOG_GUI, "build_tree_list: Building %d nodes, show_tools=%d\n", g_cached_deficons_count, show_tools);
    
    for (i = 0; i < g_cached_deficons_count; i++)
    {
        DeficonTypeTreeNode *tree_node = &g_cached_deficons_tree[i];
        BOOL is_enabled = is_deficon_type_checked_for_ui(prefs, tree_node->type_name);
        ULONG node_flags = 0;
        
        /* Format display text based on generation (generations start at 1) */
        if (tree_node->generation == 1)
        {
            /* Root nodes: Show type name, optionally with default tool */
            if (show_tools)
            {
                log_debug(LOG_GUI, "build_tree_list: Processing generation 1 node '%s'\n", tree_node->type_name);
                default_tool = lookup_deficon_default_tool(tree_node->type_name);
                if (default_tool != NULL)
                {
                    sprintf(display_text, "%s  [%s]", tree_node->type_name, default_tool);
                    log_info(LOG_GUI, "  Display text: '%s'\n", display_text);
                    whd_free(default_tool);
                    default_tool = NULL;
                }
                else
                {
                    sprintf(display_text, "%s", tree_node->type_name);
                    log_debug(LOG_GUI, "  No tool found, display text: '%s'\n", display_text);
                }
            }
            else
            {
                sprintf(display_text, "%s", tree_node->type_name);
            }
        }
        else if (tree_node->generation == 2)
        {
            /* Child nodes: Show as sub-items, optionally with default tool */
            if (show_tools)
            {
                log_debug(LOG_GUI, "build_tree_list: Processing generation 2 node '%s'\n", tree_node->type_name);
                default_tool = lookup_deficon_default_tool(tree_node->type_name);
                if (default_tool != NULL)
                {
                    sprintf(display_text, "  %s  [%s]", tree_node->type_name, default_tool);
                    log_info(LOG_GUI, "  Display text: '%s'\n", display_text);
                    whd_free(default_tool);
                    default_tool = NULL;
                }
                else
                {
                    sprintf(display_text, "  %s", tree_node->type_name);
                    log_debug(LOG_GUI, "  No tool found, display text: '%s'\n", display_text);
                }
            }
            else
            {
                sprintf(display_text, "  %s", tree_node->type_name);
            }
        }
        else
        {
            /* Deeper levels (generation 3+) */
            if (show_tools)
            {
                log_debug(LOG_GUI, "build_tree_list: Processing generation %d node '%s'\n", 
                         tree_node->generation, tree_node->type_name);
                default_tool = lookup_deficon_default_tool(tree_node->type_name);
                if (default_tool != NULL)
                {
                    sprintf(display_text, "  %s  [%s]", tree_node->type_name, default_tool);
                    log_info(LOG_GUI, "  Display text: '%s'\n", display_text);
                    whd_free(default_tool);
                    default_tool = NULL;
                }
                else
                {
                    sprintf(display_text, "  %s", tree_node->type_name);
                    log_debug(LOG_GUI, "  No tool found, display text: '%s'\n", display_text);
                }
            }
            else
            {
                sprintf(display_text, "  %s", tree_node->type_name);
            }
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

/* Icon size chooser labels (matches ITIDY_ICON_SIZE_* constants) */
static STRPTR icon_size_labels[] = {
    "Small (48x48)",
    "Medium (64x64)",
    "Large (100x100)",
    NULL
};

/* Palette mode chooser labels (matches ITIDY_PAL_* constants) */
static STRPTR palette_mode_labels[] = {
    "Picture (original colors)",
    "Screen (match Workbench)",
    NULL
};

/*
 * Create the window
 */
static BOOL create_window(DefIconsSettingsWindow *win)
{
    /* Build tree list (initially without tools) */
    win->tree_list = build_tree_list(win->prefs, FALSE);
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
    
    win->icon_size_chooser_obj = (Object *)ChooserObject,
        GA_ID, GID_ICON_SIZE_CHOOSER,
        GA_RelVerify, TRUE,
        CHOOSER_LabelArray, icon_size_labels,
        CHOOSER_Selected, win->prefs->deficons_icon_size_mode,
    ChooserEnd;
    
    win->palette_mode_chooser_obj = (Object *)ChooserObject,
        GA_ID, GID_PALETTE_MODE_CHOOSER,
        GA_RelVerify, TRUE,
        CHOOSER_LabelArray, palette_mode_labels,
        CHOOSER_Selected, win->prefs->deficons_palette_mode,
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
    
    win->show_tools_btn = (Object *)ButtonObject,
        GA_ID, GID_SHOW_TOOLS,
        GA_Text, "Show default _tools",
        GA_RelVerify, TRUE,
    ButtonEnd;
    
    win->change_default_tool_btn = (Object *)ButtonObject,
        GA_ID, GID_CHANGE_DEFAULT_TOOL,
        GA_Text, "Change default tool",
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
            LAYOUT_AddChild, (Object *)LabelObject,
                LABEL_Text, "Icon Size:",
            LabelEnd,
            CHILD_WeightedWidth, 0,
            
            LAYOUT_AddChild, win->icon_size_chooser_obj,
            CHILD_WeightedWidth, 100,
        LayoutEnd,
        CHILD_WeightedHeight, 0,
        
        LAYOUT_AddChild, (Object *)HLayoutObject,
            LAYOUT_AddChild, (Object *)LabelObject,
                LABEL_Text, "Palette Mode:",
            LabelEnd,
            CHILD_WeightedWidth, 0,
            
            LAYOUT_AddChild, win->palette_mode_chooser_obj,
            CHILD_WeightedWidth, 100,
        LayoutEnd,
        CHILD_WeightedHeight, 0,
        
        LAYOUT_AddChild, (Object *)HLayoutObject,
            LAYOUT_AddChild, win->select_all_btn,
            LAYOUT_AddChild, win->select_none_btn,
        LayoutEnd,
        CHILD_WeightedHeight, 0,
        
        LAYOUT_AddChild, (Object *)HLayoutObject,
            LAYOUT_AddChild, win->show_tools_btn,
            LAYOUT_AddChild, win->change_default_tool_btn,
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
    
    /* Expand root nodes to show generation 2 checkboxes on open */
    expand_root_nodes(win->tree_list);
    
    /* Refresh the gadget to show the expanded tree */
    SetGadgetAttrs((struct Gadget *)win->tree_listbrowser, win->window, NULL,
        LISTBROWSER_Labels, ~0,  /* Detach */
        TAG_DONE);
    SetGadgetAttrs((struct Gadget *)win->tree_listbrowser, win->window, NULL,
        LISTBROWSER_Labels, win->tree_list,  /* Reattach */
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
 * Handle Show Tools button - rebuild list with default tool info
 */
static void handle_show_tools(DefIconsSettingsWindow *win)
{
    struct List *new_list;
    
    log_info(LOG_GUI, "\n=== Scanning DefIcons for default tools ===\n");
    
    /* Set busy pointer while scanning */
    SetWindowPointer(win->window, WA_BusyPointer, TRUE, TAG_DONE);
    
    /* Check if template system is initialized */
    {
        int template_count = 0;
        const TemplateCacheEntry *cache = deficons_get_template_cache_stats(&template_count);
        if (cache == NULL || template_count == 0)
        {
            log_warning(LOG_GUI, "DefIcons template system not initialized, initializing now...\n");
            if (deficons_initialize_templates())
            {
                cache = deficons_get_template_cache_stats(&template_count);
                log_info(LOG_GUI, "Template system initialized: %d templates cached\n", template_count);
            }
            else
            {
                log_error(LOG_GUI, "Failed to initialize template system!\n");
                /* Restore normal pointer on error */
                SetWindowPointer(win->window, WA_BusyPointer, FALSE, TAG_DONE);
                return;
            }
        }
        else
        {
            log_debug(LOG_GUI, "Template system already initialized: %d templates cached\n", template_count);
        }
    }
    
    log_info(LOG_GUI, "Detaching current listbrowser...\n");
    
    /* Detach current list */
    SetGadgetAttrs((struct Gadget *)win->tree_listbrowser, win->window, NULL,
        LISTBROWSER_Labels, ~0,
        TAG_DONE);
    
    /* Free old list */
    if (win->tree_list != NULL)
    {
        log_debug(LOG_GUI, "Freeing old tree list...\n");
        free_tree_list(win->tree_list);
        win->tree_list = NULL;
    }
    
    /* Build new list with tools displayed */
    log_info(LOG_GUI, "Building new tree list with default tools...\n");
    new_list = build_tree_list(win->prefs, TRUE);
    if (new_list == NULL)
    {
        log_error(LOG_GUI, "Failed to rebuild tree list with tools\n");
        /* Try to rebuild without tools as fallback */
        log_warning(LOG_GUI, "Attempting fallback rebuild without tools...\n");
        new_list = build_tree_list(win->prefs, FALSE);
        win->is_showing_tools = FALSE;
    }
    else
    {
        win->is_showing_tools = TRUE;
    }
    
    win->tree_list = new_list;
    
    /* Expand root nodes to show generation 2 */
    if (win->tree_list != NULL)
    {
        log_debug(LOG_GUI, "Expanding root nodes...\n");
        expand_root_nodes(win->tree_list);
    }
    else
    {
        log_error(LOG_GUI, "ERROR: tree_list is NULL after rebuild!\n");
    }
    
    /* Reattach list */
    log_debug(LOG_GUI, "Reattaching listbrowser with new list...\n");
    SetGadgetAttrs((struct Gadget *)win->tree_listbrowser, win->window, NULL,
        LISTBROWSER_Labels, win->tree_list,
        TAG_DONE);
    
    /* Refresh display */
    log_debug(LOG_GUI, "Refreshing display...\n");
    RefreshGadgets((struct Gadget *)win->tree_listbrowser, win->window, NULL);
    RefreshWindowFrame(win->window);
    
    /* Restore normal pointer */
    SetWindowPointer(win->window, WA_BusyPointer, FALSE, TAG_DONE);
    
    log_info(LOG_GUI, "=== Default tools display updated ===\n\n");
}

/*
 * Handle Change Default Tool button
 */
static void handle_change_default_tool(DefIconsSettingsWindow *win)
{
    struct Node *selected_node = NULL;
    DeficonTypeTreeNode *tree_node = NULL;
    ULONG generation = 0;
    char *current_tool = NULL;
    char template_path[512];
    char drawer[256];
    char filename[128];
    struct FileRequester *freq = NULL;
    char full_path[512];
    BOOL found = FALSE;
    
    log_info(LOG_GUI, "Change default tool button clicked\n");
    
    /* Get selected node from listbrowser */
    GetAttr(LISTBROWSER_SelectedNode, win->tree_listbrowser, (ULONG *)&selected_node);
    if (selected_node == NULL)
    {
        ShowEasyRequest(win->window,
            "No Selection",
            "Please select a DefIcon type first.",
            "OK");
        return;
    }
    
    /* Get node data */
    GetListBrowserNodeAttrs(selected_node,
        LBNA_Generation, &generation,
        LBNA_UserData, &tree_node,
        TAG_DONE);
    
    /* Validate node */
    if (tree_node == NULL || generation < 1 || generation > 3)
    {
        ShowEasyRequest(win->window,
            "Invalid Selection",
            "Please select a valid DefIcon type.",
            "OK");
        return;
    }
    
    log_info(LOG_GUI, "Selected type: '%s' (generation %lu)\n", tree_node->type_name, generation);
    
    /* Look up current default tool */
    current_tool = lookup_deficon_default_tool(tree_node->type_name);
    
    /* Parse current tool into drawer and filename */
    if (current_tool != NULL && current_tool[0] != '\0')
    {
        char *last_slash = strrchr(current_tool, '/');
        char *last_colon = strrchr(current_tool, ':');
        char *separator = NULL;
        
        /* Find the last path separator */
        if (last_slash && last_colon)
        {
            separator = (last_slash > last_colon) ? last_slash : last_colon;
        }
        else if (last_slash)
        {
            separator = last_slash;
        }
        else if (last_colon)
        {
            separator = last_colon;
        }
        
        if (separator != NULL)
        {
            size_t drawer_len = separator - current_tool + 1;
            if (drawer_len < sizeof(drawer))
            {
                strncpy(drawer, current_tool, drawer_len);
                drawer[drawer_len] = '\0';
                strncpy(filename, separator + 1, sizeof(filename) - 1);
                filename[sizeof(filename) - 1] = '\0';
            }
            else
            {
                strcpy(drawer, "SYS:");
                strcpy(filename, "");
            }
        }
        else
        {
            /* No path separator - just a filename */
            strcpy(drawer, "SYS:");
            strncpy(filename, current_tool, sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0';
        }
        
        log_debug(LOG_GUI, "Parsed current tool: drawer='%s', file='%s'\n", drawer, filename);
    }
    else
    {
        /* No current tool - default to SYS: */
        strcpy(drawer, "SYS:");
        strcpy(filename, "");
        log_debug(LOG_GUI, "No current tool, defaulting to SYS:\n");
    }
    
    /* Free current tool string */
    if (current_tool != NULL)
    {
        whd_free(current_tool);
        current_tool = NULL;
    }
    
    /* Open file requester */
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Select Default Tool",
        ASLFR_InitialDrawer, drawer,
        ASLFR_InitialFile, filename,
        ASLFR_DoSaveMode, FALSE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, win->window,
        TAG_END);
    
    if (!freq)
    {
        ShowEasyRequest(win->window,
            "Error",
            "Could not open file requester.",
            "OK");
        return;
    }
    
    if (AslRequest(freq, NULL))
    {
        /* Build full path to selected tool */
        strcpy(full_path, freq->fr_Drawer);
        if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            FreeAslRequest(freq);
            ShowEasyRequest(win->window,
                "Error",
                "File path is too long.",
                "OK");
            return;
        }
        
        log_info(LOG_GUI, "Selected tool: %s\n", full_path);
        
        /* For generation 3 (file types), check if child template exists first */
        /* If not, clone from parent before updating */
        if (generation == 3)
        {
            char child_template_path[512];
            BPTR lock;
            BOOL child_exists = FALSE;
            
            /* Build direct path to child template: ENVARC:Sys/def_<childtype>.info */
            snprintf(child_template_path, sizeof(child_template_path), "ENVARC:Sys/def_%s.info", tree_node->type_name);
            
            log_info(LOG_GUI, "*** Generation 3 node '%s' - checking if child template exists: %s\n", 
                    tree_node->type_name, child_template_path);
            
            /* Check if child template exists */
            lock = Lock((STRPTR)child_template_path, ACCESS_READ);
            if (lock)
            {
                UnLock(lock);
                child_exists = TRUE;
                log_info(LOG_GUI, "*** Child template EXISTS: %s\n", child_template_path);
            }
            else
            {
                log_info(LOG_GUI, "*** Child template DOES NOT EXIST: %s\n", child_template_path);
                log_info(LOG_GUI, "*** Need to clone from parent...\n");
            }
            
            /* If child doesn't exist, clone from parent */
            if (!child_exists)
            {
                const char *parent_type = NULL;
                char parent_template_path[512];
                BOOL parent_found = FALSE;
                
                /* Get parent type name */
                parent_type = get_parent_type_name(g_cached_deficons_tree, g_cached_deficons_count, tree_node->type_name);
                if (parent_type != NULL)
                {
                    log_info(LOG_GUI, "*** Parent type: '%s'\n", parent_type);
                    
                    /* Find parent template (will walk up hierarchy if needed) */
                    parent_found = deficons_resolve_template(parent_type, parent_template_path, sizeof(parent_template_path));
                    if (parent_found)
                    {
                        char parent_path_no_info[512];
                        char *info_ext;
                        
                        log_info(LOG_GUI, "*** Parent template found: %s\n", parent_template_path);
                        
                        /* GetDiskObject() expects path WITHOUT .info extension */
                        /* deficons_resolve_template() returns path WITH .info extension */
                        /* So we need to strip it before calling GetDiskObject() */
                        strncpy(parent_path_no_info, parent_template_path, sizeof(parent_path_no_info) - 1);
                        parent_path_no_info[sizeof(parent_path_no_info) - 1] = '\0';
                        
                        info_ext = strstr(parent_path_no_info, ".info");
                        if (info_ext)
                        {
                            *info_ext = '\0';
                            log_debug(LOG_GUI, "*** Stripped .info extension: %s\n", parent_path_no_info);
                        }
                        
                        /* Clone the parent icon to child */
                        log_info(LOG_GUI, "*** Calling GetDiskObject(%s)\n", parent_path_no_info);
                        struct DiskObject *parent_dobj = GetDiskObject(parent_path_no_info);
                        if (parent_dobj)
                        {
                            char child_path_no_info[512];
                            char *child_info_ext;
                            
                            log_info(LOG_GUI, "*** Successfully loaded parent icon\n");
                            log_info(LOG_GUI, "*** Parent path (with .info):  %s\n", parent_template_path);
                            log_info(LOG_GUI, "*** Child path (with .info):   %s\n", child_template_path);
                            
                            /* PutDiskObject() also expects path WITHOUT .info extension */
                            strncpy(child_path_no_info, child_template_path, sizeof(child_path_no_info) - 1);
                            child_path_no_info[sizeof(child_path_no_info) - 1] = '\0';
                            
                            child_info_ext = strstr(child_path_no_info, ".info");
                            if (child_info_ext)
                            {
                                *child_info_ext = '\0';
                                log_debug(LOG_GUI, "*** Child path (without .info): %s\n", child_path_no_info);
                            }
                            
                            log_info(LOG_GUI, "*** Calling PutDiskObject(%s, ...)\n", child_path_no_info);
                            if (PutDiskObject(child_path_no_info, parent_dobj))
                            {
                                log_info(LOG_GUI, "*** Successfully cloned parent icon to child\n");
                                strcpy(template_path, child_template_path);
                                found = TRUE;
                            }
                            else
                            {
                                log_error(LOG_GUI, "*** FAILED to save cloned icon to: %s\n", child_path_no_info);
                            }
                            
                            FreeDiskObject(parent_dobj);
                        }
                        else
                        {
                            log_error(LOG_GUI, "*** FAILED to load parent icon: %s\n", parent_template_path);
                        }
                    }
                    else
                    {
                        log_error(LOG_GUI, "*** Parent template not found for: %s\n", parent_type);
                    }
                }
                else
                {
                    log_error(LOG_GUI, "*** Could not determine parent type for: %s\n", tree_node->type_name);
                }
            }
            else
            {
                /* Child template exists - use it */
                log_info(LOG_GUI, "*** Using existing child template: %s\n", child_template_path);
                strcpy(template_path, child_template_path);
                found = TRUE;
            }
        }
        else
        {
            /* Generation 1 or 2 - use normal template resolution */
            log_info(LOG_GUI, "Generation %lu node '%s' - using deficons_resolve_template\n", 
                    generation, tree_node->type_name);
            found = deficons_resolve_template(tree_node->type_name, template_path, sizeof(template_path));
        }
        
        /* Final check - did we find or create a template? */
        if (!found)
        {
            FreeAslRequest(freq);
            ShowEasyRequest(win->window,
                "Error",
                "Could not find or create\ntemplate icon for this type.",
                "OK");
            log_error(LOG_GUI, "Template not found for type: %s\n", tree_node->type_name);
            return;
        }
        
        log_info(LOG_GUI, "Template path: %s\n", template_path);
        
        /* Update the icon's default tool */
        if (SetIconDefaultTool(template_path, full_path))
        {
            log_info(LOG_GUI, "Default tool updated successfully\n");
            
            /* Refresh the listbrowser if showing tools */
            if (win->is_showing_tools)
            {
                log_debug(LOG_GUI, "Refreshing listbrowser to show updated tool...\n");
                handle_show_tools(win);
            }
            
            ShowEasyRequest(win->window,
                "Success",
                "Default tool updated successfully.",
                "OK");
        }
        else
        {
            ShowEasyRequest(win->window,
                "Error",
                "Failed to update default tool.",
                "OK");
            log_error(LOG_GUI, "Failed to update default tool for: %s\n", template_path);
        }
    }
    
    FreeAslRequest(freq);
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
    
    /* Get icon size mode from chooser */
    {
        ULONG selected_size = 0;
        GetAttr(CHOOSER_Selected, win->icon_size_chooser_obj, &selected_size);
        win->prefs->deficons_icon_size_mode = (UWORD)selected_size;
    }
    
    /* Get palette mode from chooser */
    {
        ULONG selected_palette = 0;
        GetAttr(CHOOSER_Selected, win->palette_mode_chooser_obj, &selected_palette);
        win->prefs->deficons_palette_mode = (UWORD)selected_palette;
    }
    
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
    
    log_info(LOG_GUI, "DefIcons settings saved: folder_mode=%d, icon_size=%d, "
             "palette_mode=%d, disabled_types='%s'\n",
             win->prefs->deficons_folder_icon_mode,
             win->prefs->deficons_icon_size_mode,
             win->prefs->deficons_palette_mode,
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
                        
                        case GID_SHOW_TOOLS:
                            handle_show_tools(win);
                            break;
                        
                        case GID_CHANGE_DEFAULT_TOOL:
                            handle_change_default_tool(win);
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
    win.is_showing_tools = FALSE;
    
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
