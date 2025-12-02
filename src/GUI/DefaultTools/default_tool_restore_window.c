/**
 * @file default_tool_restore_window.c
 * @brief Restore window for reverting default tool changes from backups
 * 
 * Two-ListView architecture using Exec lists:
 * 1. Sessions ListView: Shows backup sessions from PROGDIR:Backups/tools/
 * 2. Changes ListView: Shows tool changes in selected session
 * 
 * Uses list-based API from default_tool_backup.h
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <clib/exec_protos.h>  /* For NewList() */

#include <stdio.h>
#include <string.h>

#include "platform/platform.h"
#include "default_tool_backup.h"
#include "helpers/listview_simple_columns.h"
#include <easy_request_helper.h>
#include "writeLog.h"
#include "Settings/IControlPrefs.h"  /* For prefsIControl global */
#include "string_functions.h"

/* NewList macro if not available */
#ifndef NewList
#define NewList(l) \
    do { \
        struct List *_list = (l); \
        _list->lh_Head = (struct Node *)&_list->lh_Tail; \
        _list->lh_Tail = NULL; \
        _list->lh_TailPred = (struct Node *)&_list->lh_Head; \
    } while(0)
#endif

/* Forward declarations for external interface */
struct Window *iTidy_CreateToolRestoreWindow(struct Screen *screen, APTR backup_manager);
BOOL iTidy_HandleToolRestoreWindowEvent(struct Window *window, struct IntuiMessage *msg);
void iTidy_CloseToolRestoreWindow(struct Window *window);

/* Forward declaration of data structure */
typedef struct iTidy_ToolRestoreData iTidy_ToolRestoreData;

/* Global window data pointer (safer than UserPort hack) */
static iTidy_ToolRestoreData *g_restore_window_data = NULL;

/* ============================================
 * Constants and Gadget IDs
 * ============================================ */

#define WINDOW_TITLE "Restore Default Tools"
#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 320

/*------------------------------------------------------------------------*/
/* Column Configuration for Session List (Simple API)                     */
/*------------------------------------------------------------------------*/
#define NUM_SESSION_LIST_COLUMNS 4

static const iTidy_SimpleColumn session_list_columns[] = {
    /* title         width  align               smart_path */
    {"Date/Time",    20,   ITIDY_ALIGN_LEFT,   FALSE},
    {"Mode",         10,   ITIDY_ALIGN_LEFT,   FALSE},
    {"Path",         30,   ITIDY_ALIGN_LEFT,   TRUE},  /* Enable smart path truncation */
    {"Changed",      7,    ITIDY_ALIGN_RIGHT,  FALSE}
};

/* Gadget IDs */
enum {
    GID_SESSION_LIST = 1,
    GID_CHANGES_LIST,
    GID_RESTORE_ALL,
    GID_CLOSE
};

/* ============================================
 * Data Structures
 * ============================================ */

/**
 * @brief Window state for restore UI
 */
struct iTidy_ToolRestoreData {
    struct Window *window;          /* Intuition window */
    struct Gadget *gadget_list;     /* First gadget in list */
    void *visual_info;              /* GadTools visual info */
    
    /* Gadgets */
    struct Gadget *session_listview;
    struct Gadget *changes_listview;
    struct Gadget *restore_button;
    struct Gadget *close_button;
    
    /* Session tracking */
    struct List session_list;           /* List of iTidy_ToolBackupSession */
    struct List changes_list;           /* List of iTidy_ToolChange */
    LONG selected_session_index;       /* Currently selected session (-1 = none) */
    char selected_session_id[32];      /* ID of selected session */
    
    /* Display lists for ListViews */
    struct List *session_display_list;  /* Formatted session list (simple API) */
    struct List changes_display_list;   /* Formatted strings for changes ListView */
    
    /* Layout dimensions */
    UWORD listview_width;               /* Width of ListViews for formatter */
    
    /* Restore tracking - for cache invalidation */
    BOOL restore_performed;             /* TRUE if any restore operation completed */
};

/* ============================================
 * Forward Declarations
 * ============================================ */

static void populate_session_list(iTidy_ToolRestoreData *data);
static void populate_changes_list(iTidy_ToolRestoreData *data);
static void handle_restore_all(iTidy_ToolRestoreData *data);
static void cleanup_display_lists(iTidy_ToolRestoreData *data);

/* ============================================
 * Display List Management
 * ============================================ */

/**
 * @brief Free display list nodes and strings
 */
static void cleanup_display_lists(iTidy_ToolRestoreData *data)
{
    struct Node *node, *next_node;
    
    /* Detach lists from gadgets first */
    if (data->session_listview && data->window) {
        GT_SetGadgetAttrs(data->session_listview, data->window, NULL,
                          GTLV_Labels, ~0,
                          TAG_DONE);
    }
    if (data->changes_listview && data->window) {
        GT_SetGadgetAttrs(data->changes_listview, data->window, NULL,
                          GTLV_Labels, ~0,
                          TAG_DONE);
    }
    
    /* Free session ListView resources (simple API) */
    if (data->session_display_list) {
        while ((node = RemHead(data->session_display_list)) != NULL) {
            if (node->ln_Name) whd_free(node->ln_Name);
            FreeVec(node);
        }
        FreeVec(data->session_display_list);
        data->session_display_list = NULL;
    }
    
    /* Free changes display list (both tool lines and count lines are allocated) */
    node = data->changes_display_list.lh_Head;
    while (node->ln_Succ) {
        next_node = node->ln_Succ;
        Remove(node);
        /* Free allocated strings (tool lines and count lines) */
        /* Check if it's an allocated string (not the placeholder) */
        if (node->ln_Name && strcmp(node->ln_Name, "(No tool changes in session)") != 0) {
            whd_free(node->ln_Name);
        }
        FreeVec(node);
        node = next_node;
    }
}

/* ============================================
 * Session List Population
 * ============================================ */

/**
 * @brief Sort session array by specified column
 * @param sessions Array of sessions to sort (will be modified in-place)
 * @param count Number of sessions
 * @param column Column index (0=date, 1=mode, 2=path, 3=changed count)
 */
static void iTidy_SortSessionsByColumn(iTidy_ToolBackupSession *sessions, int count, int column)
{
    int i, j;
    iTidy_ToolBackupSession temp;
    int swap_needed;
    
    /* Simple bubble sort - sufficient for small session lists */
    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - i - 1; j++) {
            swap_needed = 0;
            
            switch (column) {
                case 0:  /* Date/Time - compare session_id (timestamp string) */
                    swap_needed = (strcmp(sessions[j].session_id, sessions[j+1].session_id) > 0);
                    break;
                case 1:  /* Mode */
                    swap_needed = (strcmp(sessions[j].mode, sessions[j+1].mode) > 0);
                    break;
                case 2:  /* Path */
                    swap_needed = (strcmp(sessions[j].scanned_path, sessions[j+1].scanned_path) > 0);
                    break;
                case 3:  /* Changed count */
                    swap_needed = (sessions[j].icons_changed > sessions[j+1].icons_changed);
                    break;
            }
            
            if (swap_needed) {
                temp = sessions[j];
                sessions[j] = sessions[j+1];
                sessions[j+1] = temp;
            }
        }
    }
}

/**
 * @brief Populate the session ListView using simple columns API
 */
static void populate_session_list(iTidy_ToolRestoreData *data)
{
    iTidy_ToolBackupSession *session;
    struct Node *session_node, *node;
    UWORD count;
    char formatted_date[32];
    char changed_str[16];
    const char *cell_values[NUM_SESSION_LIST_COLUMNS];
    
    log_debug(LOG_GUI, "populate_session_list: Starting...\n");
    
    /* Set busy pointer during loading */
    if (data->window) {
        SetWindowPointer(data->window, WA_BusyPointer, TRUE, TAG_DONE);
    }
    
    /* Free old ListView resources */
    log_debug(LOG_GUI, "populate_session_list: Freeing old ListView data\n");
    if (data->session_display_list) {
        while ((node = RemHead(data->session_display_list)) != NULL) {
            if (node->ln_Name) whd_free(node->ln_Name);
            FreeVec(node);
        }
        FreeVec(data->session_display_list);
        data->session_display_list = NULL;
    }
    
    /* Free old session data */
    iTidy_FreeSessionList(&data->session_list);
    NewList(&data->session_list);
    
    /* Scan for backup sessions */
    log_debug(LOG_GUI, "populate_session_list: Scanning for backup sessions...\n");
    count = iTidy_ScanBackupSessions(&data->session_list);
    log_info(LOG_GUI, "populate_session_list: Found %u backup sessions\n", count);
    
    /* Create new display list */
    data->session_display_list = (struct List *)AllocVec(sizeof(struct List), MEMF_CLEAR);
    if (!data->session_display_list) {
        log_error(LOG_GUI, "populate_session_list: Failed to allocate display list\n");
        if (data->window) {
            SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
        }
        return;
    }
    NewList(data->session_display_list);
    
    /* Add header row */
    node = AllocVec(sizeof(struct Node), MEMF_CLEAR);
    if (node) {
        node->ln_Name = iTidy_FormatHeader(session_list_columns, NUM_SESSION_LIST_COLUMNS);
        if (node->ln_Name) {
            AddTail(data->session_display_list, node);
        } else {
            FreeVec(node);
        }
    }
    
    /* Add separator row */
    node = AllocVec(sizeof(struct Node), MEMF_CLEAR);
    if (node) {
        node->ln_Name = iTidy_FormatSeparator(session_list_columns, NUM_SESSION_LIST_COLUMNS);
        if (node->ln_Name) {
            AddTail(data->session_display_list, node);
        } else {
            FreeVec(node);
        }
    }
    
    /* Add data rows */
    if (count > 0) {
        log_debug(LOG_GUI, "populate_session_list: Building %u data rows\n", count);
        for (session_node = data->session_list.lh_Head; 
             session_node->ln_Succ != NULL; 
             session_node = session_node->ln_Succ)
        {
            session = (iTidy_ToolBackupSession *)session_node;
            
            /* Format date using iTidy_FormatTimestamp() */
            if (!iTidy_FormatTimestamp(session->session_id, formatted_date, sizeof(formatted_date))) {
                strncpy(formatted_date, session->date_string, 31);
                formatted_date[31] = '\0';
            }
            
            /* Format changed count */
            sprintf(changed_str, "%d", session->icons_changed);
            
            /* Build cell values array */
            cell_values[0] = formatted_date;
            cell_values[1] = session->mode;
            cell_values[2] = session->scanned_path;
            cell_values[3] = changed_str;
            
            /* Format row */
            node = AllocVec(sizeof(struct Node), MEMF_CLEAR);
            if (node) {
                node->ln_Name = iTidy_FormatRow(session_list_columns, NUM_SESSION_LIST_COLUMNS, cell_values);
                if (node->ln_Name) {
                    AddTail(data->session_display_list, node);
                } else {
                    FreeVec(node);
                }
            }
        }
    }
    
    /* Update the ListView gadget */
    log_debug(LOG_GUI, "populate_session_list: Updating ListView gadget\n");
    if (data->session_listview && data->window) {
        LONG select_row = (count > 0) ? 2 : ~0;  /* Row 2 = first data row */
        GT_SetGadgetAttrs(data->session_listview, data->window, NULL,
                          GTLV_Labels, data->session_display_list,
                          GTLV_Selected, select_row,
                          TAG_DONE);
        log_debug(LOG_GUI, "populate_session_list: ListView updated\n");
    }
    
    /* Clear busy pointer */
    if (data->window) {
        SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
    }
    
    log_debug(LOG_GUI, "populate_session_list: Complete\n");
}

/* ============================================
 * Changes List Population
 * ============================================ */

/**
 * @brief Populate changes ListView for selected session
 */
static void populate_changes_list(iTidy_ToolRestoreData *data)
{
    iTidy_ToolChange *change;
    struct Node *change_node, *display_node;
    char *tool_line;
    char *count_line;
    UWORD count;
    
    /* Clear existing changes display list */
    {
        struct Node *node, *next_node;
        node = data->changes_display_list.lh_Head;
        while (node->ln_Succ) {
            next_node = node->ln_Succ;
            Remove(node);
            /* Free allocated string if it's not pointing to change->display_text */
            if (node->ln_Name && strncmp(node->ln_Name, "Total icons updated:", 20) == 0) {
                whd_free(node->ln_Name);
            }
            FreeVec(node);
            node = next_node;
        }
    }
    NewList(&data->changes_display_list);
    
    /* Free old changes data */
    iTidy_FreeToolChangeList(&data->changes_list);
    NewList(&data->changes_list);
    
    /* Validate selection */
    if (data->selected_session_index < 0 || data->selected_session_id[0] == '\0') {
        if (data->changes_listview && data->window) {
            GT_SetGadgetAttrs(data->changes_listview, data->window, NULL,
                              GTLV_Labels, &data->changes_display_list,
                              TAG_DONE);
        }
        return;
    }
    
    /* Load tool changes for selected session */
    count = iTidy_LoadToolChanges(data->selected_session_id, &data->changes_list);
    
    if (count == 0) {
        /* No changes - add placeholder */
        display_node = AllocVec(sizeof(struct Node), MEMF_CLEAR);
        if (display_node) {
            display_node->ln_Name = "(No tool changes in session)";
            AddTail(&data->changes_display_list, display_node);
        }
    } else {
        /* Create TWO display nodes for each change: tool conversion + icon count */
        for (change_node = data->changes_list.lh_Head; 
             change_node->ln_Succ != NULL; 
             change_node = change_node->ln_Succ)
        {
            change = (iTidy_ToolChange *)change_node;
            
            /* First row: Tool conversion (old -> new) */
            tool_line = (char *)whd_malloc(256);
            if (tool_line) {
                sprintf(tool_line, "%s -> %s",
                        change->old_tool[0] ? change->old_tool : "(none)",
                        change->new_tool[0] ? change->new_tool : "(none)");
                
                display_node = AllocVec(sizeof(struct Node), MEMF_CLEAR);
                if (display_node) {
                    display_node->ln_Name = tool_line;
                    AddTail(&data->changes_display_list, display_node);
                    
                    log_message(LOG_GUI, LOG_LEVEL_DEBUG,
                                "TOOL LINE: '%s'", tool_line);
                } else {
                    whd_free(tool_line);
                }
            }
            
            /* Second row: Icon count */
            count_line = (char *)whd_malloc(128);
            if (count_line) {
                sprintf(count_line, "Total icons updated: %d", change->icon_count);
                
                display_node = AllocVec(sizeof(struct Node), MEMF_CLEAR);
                if (display_node) {
                    display_node->ln_Name = count_line;
                    AddTail(&data->changes_display_list, display_node);
                    
                    log_message(LOG_GUI, LOG_LEVEL_DEBUG,
                                "COUNT LINE: '%s'", count_line);
                } else {
                    whd_free(count_line);
                }
            }
        }
    }
    
    /* Refresh ListView */
    if (data->changes_listview && data->window) {
        GT_SetGadgetAttrs(data->changes_listview, data->window, NULL,
                          GTLV_Labels, &data->changes_display_list,
                          TAG_DONE);
    }
}

/* ============================================
 * Event Handlers
 * ============================================ */

/**
 * @brief Handle Restore All button click
 */
static void handle_restore_all(iTidy_ToolRestoreData *data)
{
    UWORD success_count = 0;
    UWORD failed_count = 0;
    LONG result;
    char message[256];
    iTidy_ToolBackupSession *session;
    struct Node *node;
    LONG index;
    
    /* Validate selection */
    if (data->selected_session_index < 0 || data->selected_session_id[0] == '\0') {
        return;
    }
    
    /* Find session to get icon count for confirmation */
    index = 0;
    session = NULL;
    for (node = data->session_list.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ)
    {
        if (index == data->selected_session_index) {
            session = (iTidy_ToolBackupSession *)node;
            break;
        }
        index++;
    }
    
    if (!session) {
        return;
    }
    
    /* Confirmation dialog */
    snprintf(message, sizeof(message),
             "Restore %u icon default tool%s\nfrom backup session %s?",
             session->icons_changed,
             session->icons_changed == 1 ? "" : "s",
             data->selected_session_id);
    
    result = ShowEasyRequest(data->window, "Restore Confirmation", 
                             message, "Restore|Cancel");
    
    if (result != 1) {
        return;  /* User cancelled */
    }
    
    /* Perform restore */
    log_info(LOG_GUI, "[RESTORE_TRACKING] Starting restore operation for session: %s\n", data->selected_session_id);
    
    if (iTidy_RestoreAllIcons(data->selected_session_id, &success_count, &failed_count)) {
        log_info(LOG_GUI, "[RESTORE_TRACKING] Restore completed: success=%u, failed=%u\n", success_count, failed_count);
        
        /* Mark that a restore was performed (even if some failed) */
        if (success_count > 0) {
            data->restore_performed = TRUE;
            log_info(LOG_GUI, "[RESTORE_TRACKING] Setting restore_performed flag to TRUE\n");
        } else {
            log_info(LOG_GUI, "[RESTORE_TRACKING] No icons restored successfully - flag remains FALSE\n");
        }
        
        /* Show result */
        snprintf(message, sizeof(message),
                 "Restore complete:\n%u icon%s restored successfully\n%u failed",
                 success_count,
                 success_count == 1 ? "" : "s",
                 failed_count);
        ShowEasyRequest(data->window, "Restore Complete", message, "OK");
    } else {
        ShowEasyRequest(data->window, "Restore Failed",
                        "Failed to restore icons.\nSee log for details.", "OK");
    }
}

/* ============================================
 * Window Creation
 * ============================================ */

/**
 * @brief Create the restore window with dual ListView layout
 */
struct Window *iTidy_CreateToolRestoreWindow(struct Screen *screen, APTR backup_manager)
{
    iTidy_ToolRestoreData *data;
    struct NewGadget ng;
    struct Gadget *gad;
    UWORD top_edge, left_edge;
    UWORD list_width, session_list_height, changes_list_height;
    UWORD font_height, listview_line_height;
    UWORD button_height, window_height;
    struct TextAttr listview_font;
    UWORD font_char_width;
    
    (void)backup_manager;  /* Not used - we call iTidy_ScanBackupSessions directly */
    
    /* Use system font from global prefsIControl (already loaded at startup) */
    listview_font.ta_Name = prefsIControl.systemFontName;
    listview_font.ta_YSize = prefsIControl.systemFontSize;
    listview_font.ta_Style = FS_NORMAL;
    listview_font.ta_Flags = 0;
    font_char_width = prefsIControl.systemFontCharWidth;
    font_height = prefsIControl.systemFontSize;
    
    log_info(LOG_GUI, "Using system font for ListView: %s size=%u, char_width=%u\n", 
             listview_font.ta_Name, listview_font.ta_YSize, font_char_width);
    log_info(LOG_GUI, "Screen font: %s size=%u\n", 
             screen->Font->ta_Name, screen->Font->ta_YSize);
    
    /* Calculate ListView line height */
    listview_line_height = font_height + 2;
    
    /* Calculate button height */
    button_height = 20;  /* Standard button height */
    
    /* Allocate window data */
    data = AllocVec(sizeof(iTidy_ToolRestoreData), MEMF_CLEAR);
    if (!data) {
        return NULL;
    }
    
    data->selected_session_index = -1;
    data->session_display_list = NULL;  /* Will be created by simple API */
    data->restore_performed = FALSE;     /* Track if any restores happen */
    NewList(&data->session_list);
    NewList(&data->changes_list);
    NewList(&data->changes_display_list);
    
    /* Get visual info */
    data->visual_info = GetVisualInfo(screen, TAG_DONE);
    if (!data->visual_info) {
        FreeVec(data);
        return NULL;
    }
    
    /* Create context gadget */
    gad = CreateContext(&data->gadget_list);
    if (!gad) {
        FreeVisualInfo(data->visual_info);
        FreeVec(data);
        return NULL;
    }
    
    /* Layout parameters */
    top_edge = screen->WBorTop + screen->Font->ta_YSize + 2;
    left_edge = 8;
    list_width = WINDOW_WIDTH - 16;  /* Full width */
    
    /* Calculate ListView heights based on font */
    session_list_height = listview_line_height * 9;  /* 9 rows for sessions */
    changes_list_height = listview_line_height * 3;  /* 3 rows for changes */
    
    /* Store ListView width in CHARACTERS for formatter (not pixels!) */
    /* Account for: scrollbar (~20px), left border (~4px), right border (~4px), padding (~8px) = ~36px total */
    data->listview_width = (list_width - 36) / font_char_width;
    
    log_info(LOG_GUI, "ListView: pixel_width=%u, border_adjust=%u, font_char_width=%u, char_width=%u\n",
             list_width, list_width - 36, font_char_width, data->listview_width);
    
    /* Session ListView (top) */
    ng.ng_LeftEdge = left_edge;
    ng.ng_TopEdge = top_edge + 14;
    ng.ng_Width = list_width;
    ng.ng_Height = session_list_height;
    ng.ng_GadgetText = "Backup Sessions:";
    ng.ng_TextAttr = &listview_font;  /* Use icon font from preferences */
    ng.ng_GadgetID = GID_SESSION_LIST;
    ng.ng_Flags = PLACETEXT_ABOVE;
    ng.ng_VisualInfo = data->visual_info;
    
    gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
                       GTLV_Labels, data->session_display_list,  /* Pass list pointer, NOT &pointer! */
                       GTLV_ShowSelected, NULL,
                       TAG_DONE);
    data->session_listview = gad;
    
    /* Changes ListView (bottom) */
    ng.ng_TopEdge = top_edge + 14 + session_list_height + 16;
    ng.ng_Height = changes_list_height;
    ng.ng_GadgetText = "Tool Changes in Session:";
    ng.ng_GadgetID = GID_CHANGES_LIST;
    
    gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
                       GTLV_Labels, data->changes_display_list,  /* Pass list pointer, NOT &pointer! */
                       GTLV_ReadOnly, TRUE,
                       TAG_DONE);
    data->changes_listview = gad;
    
    /* Calculate equal button widths */
    UWORD button_width = (list_width - 8) / 2;  /* Two buttons with gap in middle */
    
    /* Restore All button */
    ng.ng_LeftEdge = left_edge;
    ng.ng_TopEdge = top_edge + 14 + session_list_height + 16 + changes_list_height + 16;
    ng.ng_Width = button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Restore selected";
    ng.ng_GadgetID = GID_RESTORE_ALL;
    ng.ng_Flags = 0;  /* Default centered text in button */
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng,
                       GA_Disabled, TRUE,  /* Disabled until selection */
                       TAG_DONE);
    data->restore_button = gad;
    
    /* Close button - equal width with gap */
    ng.ng_LeftEdge = left_edge + button_width + 8;  /* After first button + gap */
    ng.ng_Width = button_width;
    ng.ng_GadgetText = "Close";
    ng.ng_GadgetID = GID_CLOSE;
    ng.ng_Flags = 0;  /* Default centered text in button */
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    data->close_button = gad;
    
    /* Calculate window height: top_edge + label space + session list + spacing + label space + changes list + spacing + buttons + bottom margin */
    window_height = ng.ng_TopEdge + button_height + 8;  /* Button top + button height + bottom padding */
    
    /* Open window */
    data->window = OpenWindowTags(NULL,
        WA_Left, (screen->Width - WINDOW_WIDTH) / 2,
        WA_Top, (screen->Height - window_height) / 2,
        WA_Width, WINDOW_WIDTH,
        WA_Height, window_height,
        WA_Title, (ULONG)WINDOW_TITLE,
        WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_ACTIVATE,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_GADGETUP | IDCMP_MOUSEBUTTONS | LISTVIEWIDCMP | BUTTONIDCMP,
        WA_Gadgets, data->gadget_list,
        WA_PubScreen, screen,
        TAG_DONE);
    
    if (!data->window) {
        FreeGadgets(data->gadget_list);
        FreeVisualInfo(data->visual_info);
        FreeVec(data);
        return NULL;
    }
    
    /* Store data pointer globally (safer than hacking window structures) */
    g_restore_window_data = data;
    
    GT_RefreshWindow(data->window, NULL);
    
    /* Populate session list */
    populate_session_list(data);
    
    return data->window;
}

/* ============================================
 * Event Loop Handler
 * ============================================ */

/**
 * @brief Handle window events (call from main event loop)
 * 
 * @return TRUE to keep window open, FALSE to close
 * 
 * NOTE: To check if any restores were performed after window closes,
 *       call iTidy_WasRestorePerformed() before calling cleanup.
 */
BOOL iTidy_HandleToolRestoreWindowEvent(struct Window *window, struct IntuiMessage *msg)
{
    iTidy_ToolRestoreData *data;
    struct Gadget *gad;
    ULONG class;
    UWORD code;
    BOOL keep_open = TRUE;
    
    /* Safety checks */
    if (!window || !msg) {
        return FALSE;
    }
    
    /* Retrieve data from global pointer */
    data = g_restore_window_data;
    if (!data) {
        return FALSE;  /* Safety check */
    }
    
    if (data->window != window) {
        return FALSE;  /* Window mismatch */
    }
    
    class = msg->Class;
    code = msg->Code;
    gad = (struct Gadget *)msg->IAddress;
    
    switch (class) {
        case IDCMP_CLOSEWINDOW:
            keep_open = FALSE;
            break;
            
        case IDCMP_REFRESHWINDOW:
            GT_BeginRefresh(window);
            GT_EndRefresh(window, TRUE);
            break;
            
        case IDCMP_GADGETUP:
            switch (gad->GadgetID) {
                case GID_SESSION_LIST:
                    {
                        /* Get selected row from ListView */
                        LONG selected_row = ~0;
                        GT_GetGadgetAttrs(data->session_listview, window, NULL,
                                         GTLV_Selected, &selected_row,
                                         TAG_DONE);
                        
                        /* Check if click was on header row for column sorting */
                        if (selected_row == 0) {
                            int clicked_column = iTidy_DetectClickedColumn(
                                session_list_columns,
                                NUM_SESSION_LIST_COLUMNS,
                                msg->MouseX,
                                data->session_listview->LeftEdge,
                                prefsIControl.systemFontCharWidth
                            );
                            
                            if (clicked_column >= 0) {
                                iTidy_ToolBackupSession *session_array, *session;
                                struct Node *node;
                                int i, count = 0;
                                
                                log_info(LOG_GUI, "Column %d header clicked, sorting...\n", clicked_column);
                                
                                /* Count sessions */
                                for (node = data->session_list.lh_Head; node->ln_Succ; node = node->ln_Succ) {
                                    count++;
                                }
                                
                                if (count > 0) {
                                    /* Convert list to array for sorting */
                                    session_array = (iTidy_ToolBackupSession *)whd_malloc(count * sizeof(iTidy_ToolBackupSession));
                                    if (session_array) {
                                        i = 0;
                                        for (node = data->session_list.lh_Head; node->ln_Succ; node = node->ln_Succ) {
                                            session = (iTidy_ToolBackupSession *)node;
                                            session_array[i++] = *session;
                                        }
                                        
                                        /* Sort array by clicked column */
                                        iTidy_SortSessionsByColumn(session_array, count, clicked_column);
                                        
                                        /* Rebuild list from sorted array */
                                        NewList(&data->session_list);
                                        for (i = 0; i < count; i++) {
                                            session = (iTidy_ToolBackupSession *)whd_malloc(sizeof(iTidy_ToolBackupSession));
                                            if (session) {
                                                *session = session_array[i];
                                                AddTail(&data->session_list, (struct Node *)session);
                                            }
                                        }
                                        
                                        whd_free(session_array);
                                        
                                        /* Refresh display */
                                        populate_session_list(data);
                                    }
                                }
                            }
                        }
                        /* Check if data row selected (skip header row 0, separator row 1) */
                        else if (selected_row >= 2) {
                            iTidy_ToolBackupSession *session;
                            struct Node *node;
                            LONG session_index = 0;
                            LONG data_index = selected_row - 2;  /* Subtract header+separator rows */
                            
                            /* Find session at this data index */
                            for (node = data->session_list.lh_Head; 
                                 node->ln_Succ != NULL && session_index < data_index; 
                                 node = node->ln_Succ, session_index++)
                            {
                                /* Skip to target index */
                            }
                            
                            if (node && node->ln_Succ) {
                                session = (iTidy_ToolBackupSession *)node;
                                
                                /* Store selected session */
                                data->selected_session_index = data_index;
                                strncpy(data->selected_session_id, session->session_id, 
                                       sizeof(data->selected_session_id) - 1);
                                data->selected_session_id[sizeof(data->selected_session_id) - 1] = '\0';
                                
                                log_info(LOG_GUI, "Selected session %ld: %s (ID=%s, %d changes)\n",
                                        data_index, session->date_string, 
                                        session->session_id, session->icons_changed);
                                
                                /* Enable Restore All button */
                                if (data->restore_button && window) {
                                    GT_SetGadgetAttrs(data->restore_button, window, NULL,
                                                     GA_Disabled, FALSE,
                                                     TAG_DONE);
                                }
                                
                                /* Populate changes list for this session */
                                populate_changes_list(data);
                            }
                        }
                    }
                    break;
                    
                case GID_RESTORE_ALL:
                    handle_restore_all(data);
                    break;
                    
                case GID_CLOSE:
                    keep_open = FALSE;
                    break;
            }
            break;
    }
    
    return keep_open;
}

/* ============================================
 * Cleanup
 * ============================================ */

/**
 * @brief Check if any restore operations were performed
 * 
 * Call this AFTER the window event loop ends but BEFORE calling
 * iTidy_CloseToolRestoreWindow() to determine if tool cache should
 * be invalidated.
 * 
 * @return TRUE if at least one restore operation completed successfully
 */
BOOL iTidy_WasRestorePerformed(void)
{
    if (g_restore_window_data) {
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
    
    if (!window) {
        return;
    }
    
    /* Retrieve data from global pointer */
    data = g_restore_window_data;
    
    if (data) {
        log_info(LOG_GUI, "[RESTORE_TRACKING] iTidy_CloseToolRestoreWindow: restore_performed=%s\n",
                 data->restore_performed ? "TRUE" : "FALSE");
    }
    
    g_restore_window_data = NULL;  /* Clear global */
    
    if (data) {
        /* CRITICAL: Detach lists from gadgets BEFORE closing window */
        if (data->session_listview && data->window) {
            GT_SetGadgetAttrs(data->session_listview, data->window, NULL,
                              GTLV_Labels, ~0,
                              TAG_DONE);
        }
        if (data->changes_listview && data->window) {
            GT_SetGadgetAttrs(data->changes_listview, data->window, NULL,
                              GTLV_Labels, ~0,
                              TAG_DONE);
        }
        
        /* Close window BEFORE freeing gadgets */
        if (data->window) {
            CloseWindow(data->window);
            data->window = NULL;
        }
        
        /* Now safe to free gadgets */
        if (data->gadget_list) {
            FreeGadgets(data->gadget_list);
            data->gadget_list = NULL;
        }
        
        /* Free visual info */
        if (data->visual_info) {
            FreeVisualInfo(data->visual_info);
            data->visual_info = NULL;
        }
        
        /* Free session ListView resources (simple API - already done in cleanup_display_lists) */
        
        /* Free changes display list (just the wrapper nodes) */
        {
            struct Node *node, *next_node;
            
            node = data->changes_display_list.lh_Head;
            while (node->ln_Succ) {
                next_node = node->ln_Succ;
                Remove(node);
                FreeVec(node);
                node = next_node;
            }
        }
        
        /* Free session and change lists */
        iTidy_FreeSessionList(&data->session_list);
        iTidy_FreeToolChangeList(&data->changes_list);
        
        /* Finally free the data structure */
        FreeVec(data);
    } else {
        /* No data - just close window */
        CloseWindow(window);
    }
}
