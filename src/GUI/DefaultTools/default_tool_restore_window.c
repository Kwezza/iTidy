/**
 * @file default_tool_restore_window.c
 * @brief Restore window for reverting default tool changes from backups (ReAction)
 * 
 * Two-ListBrowser architecture with real columns:
 * 1. Sessions ListBrowser: Shows backup sessions from PROGDIR:Backups/tools/ (4 columns)
 * 2. Changes ListBrowser: Shows tool changes in label/value format (2 columns, 3 rows per change)
 *    - Row 1: "Old tool:" | <old_tool_path>
 *    - Row 2: "New tool:" | <new_tool_path>
 *    - Row 3: "Icons:" | <icon_count>
 * 
 * Migrated from GadTools to ReAction for Workbench 3.2+
 * Uses ListBrowser gadgets with real columns (replacing "fake columns" approach)
 */

/* =========================================================================
 * LIBRARY BASE ISOLATION
 * Redefine library bases to local unique names BEFORE including proto headers
 * ========================================================================= */
#define WindowBase      iTidy_ToolRestore_WindowBase
#define LayoutBase      iTidy_ToolRestore_LayoutBase
#define ButtonBase      iTidy_ToolRestore_ButtonBase
#define ListBrowserBase iTidy_ToolRestore_ListBrowserBase
#define LabelBase       iTidy_ToolRestore_LabelBase
#define RequesterBase   iTidy_ToolRestore_RequesterBase

#include "platform/platform.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <utility/hooks.h>

#include <stdio.h>
#include <string.h>

/* ReAction headers - MUST come before exec_list_compat.h */
#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

/* Project headers */
#include "default_tool_backup.h"
#include "../../helpers/exec_list_compat.h"
#include "writeLog.h"
#include "Settings/IControlPrefs.h"
#include "string_functions.h"
#include "GUI/gui_utilities.h"
#include "path_utilities.h"

/* ReAction class headers */
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/listbrowser.h>
#include <proto/label.h>
#include <proto/requester.h>
#include <classes/window.h>
#include <classes/requester.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/listbrowser.h>
#include <images/label.h>

/*------------------------------------------------------------------------*/
/* Library Bases (Prefixed to avoid collision)                           */
/*------------------------------------------------------------------------*/
struct Library *iTidy_ToolRestore_WindowBase = NULL;
struct Library *iTidy_ToolRestore_LayoutBase = NULL;
struct Library *iTidy_ToolRestore_ButtonBase = NULL;
struct Library *iTidy_ToolRestore_ListBrowserBase = NULL;
struct Library *iTidy_ToolRestore_LabelBase = NULL;
struct Library *iTidy_ToolRestore_RequesterBase = NULL;

/* Forward declarations for external interface */
struct Window *iTidy_CreateToolRestoreWindow(struct Screen *screen, APTR backup_manager);
BOOL iTidy_HandleToolRestoreWindowEvent(struct Window *window, struct IntuiMessage *msg);
void iTidy_CloseToolRestoreWindow(struct Window *window);
BOOL iTidy_WasRestorePerformed(void);

/* Forward declaration of data structure */
typedef struct iTidy_ToolRestoreData iTidy_ToolRestoreData;

/* Global window data pointer */
static iTidy_ToolRestoreData *g_restore_window_data = NULL;

/* ============================================
 * Constants and Gadget IDs
 * ============================================ */

#define WINDOW_TITLE "iTidy - Restore Default Tools"

/* Gadget IDs */
enum {
    GID_SESSION_LIST = 1,
    GID_CHANGES_LIST,
    GID_RESTORE_ALL,
    GID_DELETE_SESSION,
    GID_CLOSE,
    GID_VERT_182,
    GID_BACKUP_SESSION_LAYOUT,
    GID_RESTORE_LISTBROWSER_LABEL,
    GID_TOOL_CHANGES_SESSION,
    GID_TOOL_CHANGES_LABEL,
    GID_RESTORE_TOOLS_BUTTONS
};

/*------------------------------------------------------------------------*/
/* Column Configuration for Session ListBrowser (4 columns)              */
/*------------------------------------------------------------------------*/
static struct ColumnInfo *session_column_info = NULL;

/*------------------------------------------------------------------------*/
/* Column Configuration for Changes ListBrowser (3 columns)              */
/*------------------------------------------------------------------------*/
static struct ColumnInfo *changes_column_info = NULL;

/* ============================================
 * Data Structures
 * ============================================ */

/**
 * @brief Window state for restore UI (ReAction)
 */
struct iTidy_ToolRestoreData {
    /* ReAction objects */
    Object *window_obj;
    struct Window *window;
    Object *session_listbrowser_obj;
    Object *changes_listbrowser_obj;
    Object *restore_button_obj;
    Object *delete_button_obj;
    Object *close_button_obj;
    
    /* Session tracking */
    struct List session_list;           /* List of iTidy_ToolBackupSession */
    struct List changes_list;           /* List of iTidy_ToolChange */
    LONG selected_session_index;       /* Currently selected session (-1 = none) */
    char selected_session_id[32];      /* ID of selected session */
    
    /* Display lists for ListBrowsers */
    struct List *session_nodes;         /* ListBrowser nodes for sessions */
    struct List *changes_nodes;         /* ListBrowser nodes for changes */
    
    /* Restore tracking - for cache invalidation */
    BOOL restore_performed;             /* TRUE if any restore operation completed */
};

/* ============================================
 * Forward Declarations
 * ============================================ */

static BOOL open_reaction_classes(void);
static void close_reaction_classes(void);
static BOOL init_column_info(void);
static void free_column_info(void);
static void free_listbrowser_list(struct List *list);
static void populate_session_listbrowser(iTidy_ToolRestoreData *data);
static void populate_changes_listbrowser(iTidy_ToolRestoreData *data);
static void handle_restore_all(iTidy_ToolRestoreData *data);
static void handle_delete_session(iTidy_ToolRestoreData *data);
static void update_button_states(iTidy_ToolRestoreData *data);
static ULONG show_reaction_requester(struct Window *parent_window,
                                     CONST_STRPTR title,
                                     CONST_STRPTR body,
                                     CONST_STRPTR gadgets,
                                     ULONG image_type);

/* ============================================
 * ReAction Class Management
 * ============================================ */

/**
 * @brief Open ReAction class libraries
 */
static BOOL open_reaction_classes(void)
{
    if (!WindowBase)      WindowBase = OpenLibrary("window.class", 0);
    if (!LayoutBase)      LayoutBase = OpenLibrary("gadgets/layout.gadget", 0);
    if (!ButtonBase)      ButtonBase = OpenLibrary("gadgets/button.gadget", 0);
    if (!ListBrowserBase) ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 0);
    if (!LabelBase)       LabelBase = OpenLibrary("images/label.image", 0);
    if (!RequesterBase)   RequesterBase = OpenLibrary("requester.class", 0);
    
    if (!WindowBase || !LayoutBase || !ButtonBase || 
        !ListBrowserBase || !LabelBase || !RequesterBase)
    {
        log_error(LOG_GUI, "Failed to open ReAction classes for tool restore window\n");
        return FALSE;
    }
    
    return TRUE;
}

/**
 * @brief Close ReAction class libraries
 */
static void close_reaction_classes(void)
{
    if (RequesterBase)   { CloseLibrary(RequesterBase);   RequesterBase = NULL; }
    if (LabelBase)       { CloseLibrary(LabelBase);       LabelBase = NULL; }
    if (ListBrowserBase) { CloseLibrary(ListBrowserBase); ListBrowserBase = NULL; }
    if (ButtonBase)      { CloseLibrary(ButtonBase);      ButtonBase = NULL; }
    if (LayoutBase)      { CloseLibrary(LayoutBase);      LayoutBase = NULL; }
    if (WindowBase)      { CloseLibrary(WindowBase);      WindowBase = NULL; }
}

/* ============================================
 * Column Management
 * ============================================ */

/**
 * @brief Initialize column info structures for both ListBrowsers
 */
static BOOL init_column_info(void)
{
    log_debug(LOG_GUI, "Initializing column info for restore window\n");
    
    /* Session ListBrowser: Date/Time | Mode | Path | Changed */
    session_column_info = AllocLBColumnInfo(4,
        LBCIA_Column, 0,
            LBCIA_Title, "Date/Time",
            LBCIA_Weight, 25,
            LBCIA_Sortable, TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags, CIF_SORTABLE | CIF_DRAGGABLE,
        LBCIA_Column, 1,
            LBCIA_Title, "Mode",
            LBCIA_Weight, 15,
            LBCIA_Sortable, TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags, CIF_SORTABLE | CIF_DRAGGABLE,
        LBCIA_Column, 2,
            LBCIA_Title, "Path",
            LBCIA_Weight, 45,
            LBCIA_Sortable, TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags, CIF_SORTABLE | CIF_DRAGGABLE,
        LBCIA_Column, 3,
            LBCIA_Title, "Changed",
            LBCIA_Weight, 15,
            LBCIA_Sortable, TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags, CIF_SORTABLE | CIF_DRAGGABLE,
        TAG_DONE);
    
    if (session_column_info == NULL)
    {
        log_error(LOG_GUI, "Failed to allocate session column info\n");
        return FALSE;
    }
    
    /* Changes ListBrowser: Label | Value (3 rows per change) */
    changes_column_info = AllocLBColumnInfo(2,
        LBCIA_Column, 0,
            LBCIA_Title, "",
            LBCIA_Weight, 25,
            LBCIA_Sortable, FALSE,
        LBCIA_Column, 1,
            LBCIA_Title, "",
            LBCIA_Weight, 75,
            LBCIA_Sortable, FALSE,
        TAG_DONE);
    
    if (changes_column_info == NULL)
    {
        log_error(LOG_GUI, "Failed to allocate changes column info\n");
        FreeLBColumnInfo(session_column_info);
        session_column_info = NULL;
        return FALSE;
    }
    
    log_debug(LOG_GUI, "Column info initialized successfully\n");
    return TRUE;
}

/**
 * @brief Free column info structures
 */
static void free_column_info(void)
{
    if (session_column_info)
    {
        FreeLBColumnInfo(session_column_info);
        session_column_info = NULL;
    }
    
    if (changes_column_info)
    {
        FreeLBColumnInfo(changes_column_info);
        changes_column_info = NULL;
    }
}

/**
 * @brief Free a ListBrowser list and all its nodes
 */
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

/**
 * @brief Show a ReAction requester dialog
 */
static ULONG show_reaction_requester(struct Window *parent_window,
                                     CONST_STRPTR title,
                                     CONST_STRPTR body,
                                     CONST_STRPTR gadgets,
                                     ULONG image_type)
{
    Object *req_obj;
    struct orRequest req_msg;
    ULONG result = 0;
    
    if (!RequesterBase || !parent_window)
    {
        log_error(LOG_GUI, "RequesterBase or parent window is NULL\n");
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

/* ============================================
 * Session ListBrowser Population
 * ============================================ */

/**
 * @brief Populate the session ListBrowser with backup sessions (4 columns)
 */
static void populate_session_listbrowser(iTidy_ToolRestoreData *data)
{
    iTidy_ToolBackupSession *session;
    struct Node *session_node, *lb_node;
    char formatted_date[32];
    char changed_str[16];
    char truncated_path[64];
    
    log_debug(LOG_GUI, "populate_session_listbrowser: Starting...\n");
    
    /* Set busy pointer */
    if (data->window) {
        safe_set_window_pointer(data->window, TRUE);
    }
    
    /* Detach old list from gadget */
    if (data->session_listbrowser_obj && data->window)
    {
        SetGadgetAttrs((struct Gadget *)data->session_listbrowser_obj,
                       data->window, NULL,
                       LISTBROWSER_Labels, ~0,
                       TAG_DONE);
    }
    
    /* Free old nodes */
    if (data->session_nodes)
    {
        free_listbrowser_list(data->session_nodes);
        data->session_nodes = NULL;
    }
    
    /* Free old session data */
    iTidy_FreeSessionList(&data->session_list);
    NewList(&data->session_list);
    
    /* Load sessions from backup directory */
    log_debug(LOG_GUI, "Loading backup sessions from directory...\n");
    if (iTidy_ScanBackupSessions(&data->session_list) == 0)
    {
        log_warning(LOG_GUI, "No backup sessions found or failed to load\n");
    }
    
    /* Create new ListBrowser node list */
    data->session_nodes = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
    if (!data->session_nodes)
    {
        log_error(LOG_GUI, "Failed to allocate session_nodes\n");
        safe_set_window_pointer(data->window, FALSE);
        return;
    }
    NewList(data->session_nodes);
    
    /* Populate ListBrowser nodes with 4 columns */
    for (session_node = data->session_list.lh_Head; 
         session_node->ln_Succ != NULL; 
         session_node = session_node->ln_Succ)
    {
        session = (iTidy_ToolBackupSession *)session_node;
        
        /* Format date/time */
        if (!iTidy_FormatTimestamp(session->session_id, formatted_date, sizeof(formatted_date)))
        {
            strncpy(formatted_date, session->session_id, sizeof(formatted_date) - 1);
            formatted_date[sizeof(formatted_date) - 1] = '\0';
        }
        
        /* Format changed count */
        snprintf(changed_str, sizeof(changed_str), "%u", (unsigned int)session->icons_changed);
        
        /* Truncate path if needed */
        if (!iTidy_ShortenPathWithParentDir(session->scanned_path, truncated_path, 50))
        {
            strncpy(truncated_path, session->scanned_path, sizeof(truncated_path) - 1);
            truncated_path[sizeof(truncated_path) - 1] = '\0';
        }
        
        /* Create ListBrowser node: Date/Time | Mode | Path | Changed */
        lb_node = AllocListBrowserNode(4,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, formatted_date,
            LBNA_Column, 1,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, session->mode,
            LBNA_Column, 2,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, truncated_path,
            LBNA_Column, 3,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, changed_str,
                LBNCA_Justification, LCJ_RIGHT,
            TAG_DONE);
        
        if (lb_node)
        {
            /* Store session pointer in UserData for selection tracking */
            SetListBrowserNodeAttrs(lb_node,
                LBNA_UserData, (APTR)session,
                TAG_DONE);
            
            AddTail(data->session_nodes, lb_node);
        }
    }
    
    /* Reattach list to gadget */
    if (data->session_listbrowser_obj && data->window)
    {
        SetGadgetAttrs((struct Gadget *)data->session_listbrowser_obj,
                       data->window, NULL,
                       LISTBROWSER_Labels, data->session_nodes,
                       LISTBROWSER_AutoFit, TRUE,
                       TAG_DONE);
    }
    
    /* Clear busy pointer */
    if (data->window) {
        safe_set_window_pointer(data->window, FALSE);
    }
    
    log_debug(LOG_GUI, "populate_session_listbrowser: Complete\n");
}

/* ============================================
 * Changes ListBrowser Population
 * ============================================ */

/**
 * @brief Populate the changes ListBrowser with tool changes (3 columns)
 */
static void populate_changes_listbrowser(iTidy_ToolRestoreData *data)
{
    iTidy_ToolChange *change;
    struct Node *change_node, *lb_node;
    char icons_str[16];
    const char *old_tool_display;
    const char *new_tool_display;
    
    log_debug(LOG_GUI, "populate_changes_listbrowser: Starting...\n");
    
    /* Detach old list from gadget */
    if (data->changes_listbrowser_obj && data->window)
    {
        SetGadgetAttrs((struct Gadget *)data->changes_listbrowser_obj,
                       data->window, NULL,
                       LISTBROWSER_Labels, ~0,
                       TAG_DONE);
    }
    
    /* Free old nodes */
    if (data->changes_nodes)
    {
        free_listbrowser_list(data->changes_nodes);
        data->changes_nodes = NULL;
    }
    
    /* Free old change data */
    iTidy_FreeToolChangeList(&data->changes_list);
    NewList(&data->changes_list);
    
    /* If no session selected, show empty list */
    if (data->selected_session_index < 0 || data->selected_session_id[0] == '\0')
    {
        log_debug(LOG_GUI, "No session selected, showing empty list\n");
        
        /* Create empty list */
        data->changes_nodes = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
        if (data->changes_nodes)
        {
            NewList(data->changes_nodes);
            
            /* Reattach empty list */
            if (data->changes_listbrowser_obj && data->window)
            {
                SetGadgetAttrs((struct Gadget *)data->changes_listbrowser_obj,
                               data->window, NULL,
                               LISTBROWSER_Labels, data->changes_nodes,
                               TAG_DONE);
            }
        }
        return;
    }
    
    /* Load tool changes for selected session */
    log_debug(LOG_GUI, "Loading tool changes for session: %s\n", data->selected_session_id);
    if (!iTidy_LoadToolChanges(data->selected_session_id, &data->changes_list))
    {
        log_warning(LOG_GUI, "No tool changes found for session\n");
    }
    
    /* Create new ListBrowser node list */
    data->changes_nodes = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
    if (!data->changes_nodes)
    {
        log_error(LOG_GUI, "Failed to allocate changes_nodes\n");
        return;
    }
    NewList(data->changes_nodes);
    
    /* Populate ListBrowser nodes with 3 columns: Old Tool | New Tool | Icons */
    for (change_node = data->changes_list.lh_Head; 
         change_node->ln_Succ != NULL; 
         change_node = change_node->ln_Succ)
    {
        change = (iTidy_ToolChange *)change_node;
        
        /* Format icon count */
        snprintf(icons_str, sizeof(icons_str), "%u", (unsigned int)change->icon_count);
        
        /* Display strings (show "(none)" for empty tools) */
        old_tool_display = (change->old_tool[0] != '\0') ? change->old_tool : "(none)";
        new_tool_display = (change->new_tool[0] != '\0') ? change->new_tool : "(none)";
        
        /* Create 3 rows per change: Label | Value format */
        
        /* Row 1: Old tool */
        lb_node = AllocListBrowserNode(2,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, (STRPTR)"Old tool:",
                LBNCA_Justification, LCJ_RIGHT,
            LBNA_Column, 1,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, (STRPTR)old_tool_display,
            LBNA_UserData, (APTR)change,
            TAG_DONE);
        if (lb_node)
        {
            AddTail(data->changes_nodes, lb_node);
        }
        
        /* Row 2: New tool */
        lb_node = AllocListBrowserNode(2,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, (STRPTR)"New tool:",
                LBNCA_Justification, LCJ_RIGHT,
            LBNA_Column, 1,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, (STRPTR)new_tool_display,
            LBNA_UserData, (APTR)change,
            TAG_DONE);
        if (lb_node)
        {
            AddTail(data->changes_nodes, lb_node);
        }
        
        /* Row 3: Icons count */
        lb_node = AllocListBrowserNode(2,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, (STRPTR)"Icons:",
                LBNCA_Justification, LCJ_RIGHT,
            LBNA_Column, 1,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, icons_str,
            LBNA_UserData, (APTR)change,
            TAG_DONE);
        if (lb_node)
        {
            AddTail(data->changes_nodes, lb_node);
        }
    }
    
    /* Reattach list to gadget */
    if (data->changes_listbrowser_obj && data->window)
    {
        SetGadgetAttrs((struct Gadget *)data->changes_listbrowser_obj,
                       data->window, NULL,
                       LISTBROWSER_Labels, data->changes_nodes,
                       LISTBROWSER_AutoFit, TRUE,
                       TAG_DONE);
    }
    
    log_debug(LOG_GUI, "populate_changes_listbrowser: Complete\n");
}

/* ============================================
 * Button State Management
 * ============================================ */

/**
 * @brief Update button enable/disable states based on selection
 */
static void update_button_states(iTidy_ToolRestoreData *data)
{
    BOOL has_selection = (data->selected_session_index >= 0);
    
    if (data->restore_button_obj && data->window)
    {
        SetGadgetAttrs((struct Gadget *)data->restore_button_obj,
                       data->window, NULL,
                       GA_Disabled, !has_selection,
                       TAG_DONE);
    }
    
    if (data->delete_button_obj && data->window)
    {
        SetGadgetAttrs((struct Gadget *)data->delete_button_obj,
                       data->window, NULL,
                       GA_Disabled, !has_selection,
                       TAG_DONE);
    }
}

/* ============================================
 * Event Handlers
 * ============================================ */

/**
 * @brief Handle Restore button click
 */
static void handle_restore_all(iTidy_ToolRestoreData *data)
{
    UWORD success_count = 0;
    UWORD failed_count = 0;
    ULONG result;
    char message[256];
    iTidy_ToolBackupSession *session;
    struct Node *node;
    LONG index;
    char formatted_date[32];
    
    /* Validate selection */
    if (data->selected_session_index < 0 || data->selected_session_id[0] == '\0')
    {
        return;
    }
    
    /* Find session to get icon count for confirmation */
    index = 0;
    session = NULL;
    for (node = data->session_list.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ)
    {
        if (index == data->selected_session_index)
        {
            session = (iTidy_ToolBackupSession *)node;
            break;
        }
        index++;
    }
    
    if (!session)
    {
        return;
    }
    
    /* Format date for display */
    if (!iTidy_FormatTimestamp(data->selected_session_id, formatted_date, sizeof(formatted_date)))
    {
        strncpy(formatted_date, data->selected_session_id, sizeof(formatted_date) - 1);
        formatted_date[sizeof(formatted_date) - 1] = '\0';
    }
    
    /* Confirmation dialog */
    snprintf(message, sizeof(message),
             "Restore %u icon default tool%s\nfrom backup session %s?",
             session->icons_changed,
             session->icons_changed == 1 ? "" : "s",
             formatted_date);
    
    result = show_reaction_requester(data->window, "Restore Confirmation", 
                                     message, "_Restore|_Cancel", REQIMAGE_QUESTION);
    
    if (result != 1)
    {
        return;  /* User cancelled */
    }
    
    /* Perform restore */
    log_info(LOG_GUI, "[RESTORE_TRACKING] Starting restore operation for session: %s\n", data->selected_session_id);
    
    if (iTidy_RestoreAllIcons(data->selected_session_id, &success_count, &failed_count))
    {
        log_info(LOG_GUI, "[RESTORE_TRACKING] Restore completed: success=%u, failed=%u\n", success_count, failed_count);
        
        /* Mark that a restore was performed */
        if (success_count > 0)
        {
            data->restore_performed = TRUE;
            log_info(LOG_GUI, "[RESTORE_TRACKING] Setting restore_performed flag to TRUE\n");
        }
        
        /* Show result */
        snprintf(message, sizeof(message),
                 "Restore complete:\n%u icon%s restored successfully\n%u failed",
                 success_count,
                 success_count == 1 ? "" : "s",
                 failed_count);
        show_reaction_requester(data->window, "Restore Complete", message, "_Ok", REQIMAGE_INFO);
    }
    else
    {
        show_reaction_requester(data->window, "Restore Failed",
                                "Failed to restore icons.\nSee log for details.", "_Ok", REQIMAGE_ERROR);
    }
}

/**
 * @brief Handle Delete Session button click
 */
static void handle_delete_session(iTidy_ToolRestoreData *data)
{
    ULONG result;
    char message[512];
    iTidy_ToolBackupSession *session;
    struct Node *node;
    LONG index;
    char formatted_date[32];
    
    /* Validate selection */
    if (data->selected_session_index < 0 || data->selected_session_id[0] == '\0')
    {
        return;
    }
    
    /* Find session */
    index = 0;
    session = NULL;
    for (node = data->session_list.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ)
    {
        if (index == data->selected_session_index)
        {
            session = (iTidy_ToolBackupSession *)node;
            break;
        }
        index++;
    }
    
    if (!session)
    {
        return;
    }
    
    /* Format date for display */
    if (!iTidy_FormatTimestamp(data->selected_session_id, formatted_date, sizeof(formatted_date)))
    {
        strncpy(formatted_date, data->selected_session_id, sizeof(formatted_date) - 1);
        formatted_date[sizeof(formatted_date) - 1] = '\0';
    }
    
    /* Confirmation dialog */
    snprintf(message, sizeof(message),
             "Delete backup session %s?\n\nThis will permanently delete:\n- %u tool change record%s\n\nThis action cannot be undone!",
             formatted_date,
             session->icons_changed,
             session->icons_changed == 1 ? "" : "s");
    
    result = show_reaction_requester(data->window, "Confirm Delete", 
                                     message, "_Delete|_Cancel", REQIMAGE_WARNING);
    
    if (result != 1)
    {
        return;  /* User cancelled */
    }
    
    /* Perform delete */
    log_info(LOG_GUI, "Deleting backup session: %s\n", data->selected_session_id);
    
    if (iTidy_DeleteBackupSession(data->selected_session_id))
    {
        /* Clear selection state */
        data->selected_session_index = -1;
        data->selected_session_id[0] = '\0';
        
        /* Disable buttons */
        update_button_states(data);
        
        /* Clear changes list */
        populate_changes_listbrowser(data);
        
        /* Refresh session list */
        populate_session_listbrowser(data);
        
        show_reaction_requester(data->window, "Delete Complete",
                                "Backup session deleted successfully.", "_Ok", REQIMAGE_INFO);
    }
    else
    {
        show_reaction_requester(data->window, "Delete Failed",
                                "Failed to delete backup session.\nSee log for details.", "_Ok", REQIMAGE_ERROR);
    }
}

/* ============================================
 * Window Creation
 * ============================================ */

/**
 * @brief Create the restore window with ReAction ListBrowsers
 */
struct Window *iTidy_CreateToolRestoreWindow(struct Screen *screen, APTR backup_manager)
{
    iTidy_ToolRestoreData *data;
    Object *window_obj;
    struct Window *window;
    
    log_info(LOG_GUI, "Creating tool restore window (ReAction)...\n");
    
    /* Open ReAction classes */
    if (!open_reaction_classes())
    {
        log_error(LOG_GUI, "Failed to open ReAction classes\n");
        return NULL;
    }
    
    /* Initialize column info */
    if (!init_column_info())
    {
        log_error(LOG_GUI, "Failed to initialize column info\n");
        close_reaction_classes();
        return NULL;
    }
    
    /* Allocate window data */
    data = AllocVec(sizeof(iTidy_ToolRestoreData), MEMF_CLEAR);
    if (!data)
    {
        log_error(LOG_GUI, "Failed to allocate window data\n");
        free_column_info();
        close_reaction_classes();
        return NULL;
    }
    
    data->selected_session_index = -1;
    data->session_nodes = NULL;
    data->changes_nodes = NULL;
    data->restore_performed = FALSE;
    NewList(&data->session_list);
    NewList(&data->changes_list);
    
    /* Create window object following testcode.c layout */
    window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title, WINDOW_TITLE,
        WA_Left, 5,
        WA_Top, 20,
        WA_Width, 600,
        WA_Height, 400,
        WA_MinWidth, 400,
        WA_MinHeight, 300,
        WA_MaxWidth, 8192,
        WA_MaxHeight, 8192,
        WA_CloseGadget, TRUE,
        WA_DepthGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_DragBar, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, screen,
        WA_IDCMP, IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_ParentGroup, NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_DeferLayout, TRUE,
            
            /* Main vertical layout */
            LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                GA_ID, GID_VERT_182,
                LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                LAYOUT_LeftSpacing, 2,
                LAYOUT_RightSpacing, 2,
                LAYOUT_TopSpacing, 2,
                LAYOUT_BottomSpacing, 2,
                
                /* Session list layout */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, GID_BACKUP_SESSION_LAYOUT,
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                    LAYOUT_HorizAlignment, LALIGN_CENTER,
                    
                    /* Session label */
                    LAYOUT_AddImage, NewObject(LABEL_GetClass(), NULL,
                        GA_ID, GID_RESTORE_LISTBROWSER_LABEL,
                        LABEL_Text, "Backup Sessions:",
                        LABEL_Justification, LJ_CENTER,
                        TAG_DONE),
                    
                    /* Session ListBrowser */
                    LAYOUT_AddChild, data->session_listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
                        GA_ID, GID_SESSION_LIST,
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        LISTBROWSER_Position, 0,
                        LISTBROWSER_ColumnInfo, session_column_info,
                        LISTBROWSER_ColumnTitles, TRUE,
                        LISTBROWSER_AutoFit, TRUE,
                        TAG_DONE),
                    TAG_DONE),
                CHILD_WeightedHeight, 60,
                
                /* Changes list layout */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, GID_TOOL_CHANGES_SESSION,
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                    LAYOUT_HorizAlignment, LALIGN_CENTER,
                    
                    /* Changes label */
                    LAYOUT_AddImage, NewObject(LABEL_GetClass(), NULL,
                        GA_ID, GID_TOOL_CHANGES_LABEL,
                        LABEL_Text, "Tool Changes in Session",
                        TAG_DONE),
                    
                    /* Changes ListBrowser */
                    LAYOUT_AddChild, data->changes_listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
                        GA_ID, GID_CHANGES_LIST,
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        LISTBROWSER_Position, 0,
                        LISTBROWSER_ColumnInfo, changes_column_info,
                        LISTBROWSER_ColumnTitles, FALSE,
                        LISTBROWSER_AutoFit, TRUE,
                        TAG_DONE),
                    TAG_DONE),
                CHILD_WeightedHeight, 35,
                
                /* Button layout */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, GID_RESTORE_TOOLS_BUTTONS,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    
                    /* Restore button */
                    LAYOUT_AddChild, data->restore_button_obj = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_RESTORE_ALL,
                        GA_Text, "_Restore",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        GA_Disabled, TRUE,  /* Initially disabled */
                        BUTTON_TextPen, 1,
                        BUTTON_BackgroundPen, 0,
                        BUTTON_FillTextPen, 1,
                        BUTTON_FillPen, 3,
                        TAG_DONE),
                    
                    /* Delete button */
                    LAYOUT_AddChild, data->delete_button_obj = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_DELETE_SESSION,
                        GA_Text, "_Delete",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        GA_Disabled, TRUE,  /* Initially disabled */
                        BUTTON_TextPen, 1,
                        BUTTON_BackgroundPen, 0,
                        BUTTON_FillTextPen, 1,
                        BUTTON_FillPen, 3,
                        TAG_DONE),
                    
                    /* Close button */
                    LAYOUT_AddChild, data->close_button_obj = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID, GID_CLOSE,
                        GA_Text, "_Close",
                        GA_RelVerify, TRUE,
                        GA_TabCycle, TRUE,
                        BUTTON_TextPen, 1,
                        BUTTON_BackgroundPen, 0,
                        BUTTON_FillTextPen, 1,
                        BUTTON_FillPen, 3,
                        TAG_DONE),
                    TAG_DONE),
                CHILD_WeightedHeight, 5,
                TAG_DONE),
            TAG_DONE),
        TAG_DONE);
    
    if (!window_obj)
    {
        log_error(LOG_GUI, "Failed to create window object\n");
        iTidy_FreeSessionList(&data->session_list);
        iTidy_FreeToolChangeList(&data->changes_list);
        FreeVec(data);
        free_column_info();
        close_reaction_classes();
        return NULL;
    }
    
    /* Open the window */
    window = (struct Window *)RA_OpenWindow(window_obj);
    if (!window)
    {
        log_error(LOG_GUI, "Failed to open window\n");
        DisposeObject(window_obj);
        iTidy_FreeSessionList(&data->session_list);
        iTidy_FreeToolChangeList(&data->changes_list);
        FreeVec(data);
        free_column_info();
        close_reaction_classes();
        return NULL;
    }
    
    /* Store window data */
    data->window_obj = window_obj;
    data->window = window;
    g_restore_window_data = data;
    
    /* Populate session list */
    populate_session_listbrowser(data);
    
    log_info(LOG_GUI, "Tool restore window created successfully\n");
    
    return window;
}

/* ============================================
 * Event Handling
 * ============================================ */

/**
 * @brief Handle window events
 */
BOOL iTidy_HandleToolRestoreWindowEvent(struct Window *window, struct IntuiMessage *msg)
{
    iTidy_ToolRestoreData *data;
    ULONG result;
    UWORD code;
    BOOL keep_open = TRUE;
    ULONG gad_id;
    LONG selected_index;
    struct Node *selected_node;
    iTidy_ToolBackupSession *session;
    ULONG signal_mask, signals;
    
    if (!window)
    {
        return FALSE;
    }
    
    data = g_restore_window_data;
    if (!data || !data->window_obj)
    {
        return FALSE;
    }
    
    /* Get ReAction window's signal mask and wait for events */
    GetAttr(WINDOW_SigMask, data->window_obj, &signal_mask);
    signals = Wait(signal_mask | SIGBREAKF_CTRL_C);
    
    if (signals & SIGBREAKF_CTRL_C)
    {
        log_debug(LOG_GUI, "Ctrl+C detected, closing restore window\n");
        return FALSE;
    }
    
    /* Handle ReAction events */
    while ((result = RA_HandleInput(data->window_obj, &code)) != WMHI_LASTMSG)
    {
        switch (result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                keep_open = FALSE;
                break;
                
            case WMHI_GADGETUP:
                gad_id = result & WMHI_GADGETMASK;
                
                switch (gad_id)
                {
                    case GID_SESSION_LIST:
                        /* Session selected - update changes list */
                        GetAttr(LISTBROWSER_Selected, data->session_listbrowser_obj, (ULONG *)&selected_index);
                        
                        if (selected_index >= 0)
                        {
                            /* Get selected node */
                            GetAttr(LISTBROWSER_SelectedNode, data->session_listbrowser_obj, (ULONG *)&selected_node);
                            
                            if (selected_node)
                            {
                                /* Get session pointer from UserData */
                                GetListBrowserNodeAttrs(selected_node,
                                    LBNA_UserData, (ULONG *)&session,
                                    TAG_DONE);
                                
                                if (session)
                                {
                                    data->selected_session_index = selected_index;
                                    strncpy(data->selected_session_id, session->session_id, sizeof(data->selected_session_id) - 1);
                                    data->selected_session_id[sizeof(data->selected_session_id) - 1] = '\0';
                                    
                                    log_debug(LOG_GUI, "Session selected: %s (index=%ld)\n", 
                                             data->selected_session_id, selected_index);
                                    
                                    /* Update changes list */
                                    populate_changes_listbrowser(data);
                                    
                                    /* Enable buttons */
                                    update_button_states(data);
                                }
                            }
                        }
                        else
                        {
                            /* Selection cleared */
                            data->selected_session_index = -1;
                            data->selected_session_id[0] = '\0';
                            populate_changes_listbrowser(data);
                            update_button_states(data);
                        }
                        break;
                        
                    case GID_CHANGES_LIST:
                        /* Changes list clicked (no action needed for now) */
                        break;
                        
                    case GID_RESTORE_ALL:
                        handle_restore_all(data);
                        break;
                        
                    case GID_DELETE_SESSION:
                        handle_delete_session(data);
                        break;
                        
                    case GID_CLOSE:
                        keep_open = FALSE;
                        break;
                }
                break;
        }
    }
    
    return keep_open;
}

/* ============================================
 * Cleanup
 * ============================================ */

/**
 * @brief Check if any restore operations were performed
 */
BOOL iTidy_WasRestorePerformed(void)
{
    if (g_restore_window_data)
    {
        BOOL result = g_restore_window_data->restore_performed;
        log_info(LOG_GUI, "[RESTORE_TRACKING] iTidy_WasRestorePerformed() called: returning %s\n", 
                 result ? "TRUE" : "FALSE");
        return result;
    }
    log_info(LOG_GUI, "[RESTORE_TRACKING] iTidy_WasRestorePerformed() called but g_restore_window_data is NULL\n");
    return FALSE;
}

/**
 * @brief Close and cleanup restore window
 */
void iTidy_CloseToolRestoreWindow(struct Window *window)
{
    iTidy_ToolRestoreData *data;
    
    if (!window)
    {
        return;
    }
    
    data = g_restore_window_data;
    
    if (data)
    {
        log_info(LOG_GUI, "[RESTORE_TRACKING] iTidy_CloseToolRestoreWindow: restore_performed=%s\n",
                 data->restore_performed ? "TRUE" : "FALSE");
    }
    
    g_restore_window_data = NULL;  /* Clear global */
    
    if (data)
    {
        /* Detach lists from gadgets */
        if (data->session_listbrowser_obj && data->window)
        {
            SetGadgetAttrs((struct Gadget *)data->session_listbrowser_obj,
                           data->window, NULL,
                           LISTBROWSER_Labels, ~0,
                           TAG_DONE);
        }
        if (data->changes_listbrowser_obj && data->window)
        {
            SetGadgetAttrs((struct Gadget *)data->changes_listbrowser_obj,
                           data->window, NULL,
                           LISTBROWSER_Labels, ~0,
                           TAG_DONE);
        }
        
        /* Close window (also disposes all child objects) */
        if (data->window_obj)
        {
            DisposeObject(data->window_obj);  /* Closes window and disposes children */
            data->window_obj = NULL;
            data->window = NULL;
        }
        
        /* Free ListBrowser node lists */
        if (data->session_nodes)
        {
            free_listbrowser_list(data->session_nodes);
            data->session_nodes = NULL;
        }
        if (data->changes_nodes)
        {
            free_listbrowser_list(data->changes_nodes);
            data->changes_nodes = NULL;
        }
        
        /* Free session and change lists */
        iTidy_FreeSessionList(&data->session_list);
        iTidy_FreeToolChangeList(&data->changes_list);
        
        /* Free window data */
        FreeVec(data);
    }
    else
    {
        /* No data - should not happen, but handle gracefully */
        log_warning(LOG_GUI, "CloseToolRestoreWindow: No data structure found\n");
    }
    
    /* Free column info */
    free_column_info();
    
    /* Close ReAction classes */
    close_reaction_classes();
    
    log_info(LOG_GUI, "Tool restore window closed\n");
}