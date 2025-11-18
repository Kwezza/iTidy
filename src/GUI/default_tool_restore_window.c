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
#include "listview_formatter.h"
#include "easy_request_helper.h"
#include "../writeLog.h"
#include "../Settings/IControlPrefs.h"  /* For prefsIControl global */
#include "../string_functions.h"

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
    
    /* Display lists for ListViews (managed by formatter) */
    struct List *session_display_list;  /* Formatted session list (from formatter) */
    struct List changes_display_list;   /* Formatted strings for changes ListView */
    
    /* Layout dimensions */
    UWORD listview_width;               /* Width of ListViews for formatter */
    
};

/* ============================================
 * Forward Declarations
 * ============================================ */

static void populate_session_list(iTidy_ToolRestoreData *data);
static void populate_changes_list(iTidy_ToolRestoreData *data);
static void handle_session_selection(iTidy_ToolRestoreData *data, LONG selected_index);
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
    
    /* Free session display list (managed by formatter) */
    if (data->session_display_list) {
        iTidy_FreeFormattedList(data->session_display_list);
        data->session_display_list = NULL;
    }
    
    /* Free changes display list (ln_Name belongs to change) */
    node = data->changes_display_list.lh_Head;
    while (node->ln_Succ) {
        next_node = node->ln_Succ;
        Remove(node);
        FreeVec(node);
        node = next_node;
    }
}

/* ============================================
 * Session List Population
 * ============================================ */

/**
 * @brief Populate the session ListView with backup sessions using formatter
 */
static void populate_session_list(iTidy_ToolRestoreData *data)
{
    iTidy_ToolBackupSession *session;
    struct Node *session_node;
    UWORD count;
    iTidy_ColumnConfig columns[4];
    const char ***data_rows = NULL;
    const char **row_data = NULL;
    char **changed_strings = NULL;
    char **formatted_dates = NULL;
    int row_index;
    
    log_debug(LOG_GUI, "populate_session_list: Starting...\n");
    
    /* Free old formatted list */
    if (data->session_display_list) {
        log_debug(LOG_GUI, "populate_session_list: Freeing old display list\n");
        iTidy_FreeFormattedList(data->session_display_list);
        data->session_display_list = NULL;
    }
    
    /* Free old session data */
    iTidy_FreeSessionList(&data->session_list);
    NewList(&data->session_list);
    
    /* Scan for backup sessions */
    log_debug(LOG_GUI, "populate_session_list: Scanning for backup sessions...\n");
    count = iTidy_ScanBackupSessions(&data->session_list);
    log_info(LOG_GUI, "populate_session_list: Found %u backup sessions\n", count);
    
    if (count == 0) {
        log_warning(LOG_GUI, "populate_session_list: No sessions found - creating empty list\n");
        /* No sessions - create empty formatted list with just headers */
        columns[0].title = "Date/Time";
        columns[0].min_width = 10;
        columns[0].max_width = 20;
        columns[0].align = ITIDY_ALIGN_LEFT;
        columns[0].flexible = FALSE;
        columns[0].is_path = FALSE;
        
        columns[1].title = "Mode";
        columns[1].min_width = 6;
        columns[1].max_width = 8;
        columns[1].align = ITIDY_ALIGN_LEFT;
        columns[1].flexible = FALSE;
        columns[1].is_path = FALSE;
        
        columns[2].title = "Path";
        columns[2].min_width = 30;
        columns[2].max_width = 0;
        columns[2].align = ITIDY_ALIGN_LEFT;
        columns[2].flexible = TRUE;
        columns[2].is_path = TRUE;
        
        columns[3].title = "Changed";
        columns[3].min_width = 7;
        columns[3].max_width = 12;
        columns[3].align = ITIDY_ALIGN_RIGHT;
        columns[3].flexible = FALSE;
        columns[3].is_path = FALSE;
        
        data->session_display_list = iTidy_FormatListViewColumns(columns, 4, NULL, 0, 0);
        
        if (data->session_listview && data->window) {
            GT_SetGadgetAttrs(data->session_listview, data->window, NULL,
                              GTLV_Labels, data->session_display_list,
                              TAG_DONE);
        }
        return;
    }
    
    /* Define columns */
    columns[0].title = "Date/Time";
    columns[0].min_width = 0;
    columns[0].max_width = 20;
    columns[0].align = ITIDY_ALIGN_LEFT;
    columns[0].flexible = FALSE;
    columns[0].is_path = FALSE;
    
    columns[1].title = "Mode";
    columns[1].min_width = 0;
    columns[1].max_width = 8;
    columns[1].align = ITIDY_ALIGN_LEFT;
    columns[1].flexible = FALSE;
    columns[1].is_path = FALSE;
    
    columns[2].title = "Path";
    columns[2].min_width = 30;
    columns[2].max_width = 0;  /* Unlimited */
    columns[2].align = ITIDY_ALIGN_LEFT;
    columns[2].flexible = TRUE;  /* This column absorbs extra space */
    columns[2].is_path = TRUE;   /* Enable path abbreviation with /../ notation */
    
    columns[3].title = "Changed";
    columns[3].min_width = 0;
    columns[3].max_width = 12;
    columns[3].align = ITIDY_ALIGN_RIGHT;
    columns[3].flexible = FALSE;
    columns[3].is_path = FALSE;
    
    /* Allocate data rows array */
    log_debug(LOG_GUI, "populate_session_list: Allocating arrays for %u rows\n", count);
    data_rows = (const char ***)whd_malloc(sizeof(const char **) * count);
    changed_strings = (char **)whd_malloc(sizeof(char *) * count);
    formatted_dates = (char **)whd_malloc(sizeof(char *) * count);
    
    if (!data_rows || !changed_strings || !formatted_dates) {
        log_error(LOG_GUI, "Failed to allocate data arrays\n");
        if (data_rows) whd_free(data_rows);
        if (changed_strings) whd_free(changed_strings);
        if (formatted_dates) whd_free(formatted_dates);
        return;
    }
    
    /* Build data rows from session list */
    log_debug(LOG_GUI, "populate_session_list: Building data rows from session list\n");
    row_index = 0;
    for (session_node = data->session_list.lh_Head; 
         session_node->ln_Succ != NULL; 
         session_node = session_node->ln_Succ)
    {
        session = (iTidy_ToolBackupSession *)session_node;
        
        log_debug(LOG_GUI, "populate_session_list: Processing session %d: %s | %s | %s | %d\n",
                  row_index, session->date_string, session->mode, 
                  session->scanned_path, session->icons_changed);
        
        /* Allocate row */
        row_data = (const char **)whd_malloc(sizeof(const char *) * 4);
        if (!row_data) {
            log_error(LOG_GUI, "Failed to allocate row data\n");
            break;
        }
        
        /* Allocate buffer for "changed" string */
        changed_strings[row_index] = (char *)whd_malloc(16);
        if (!changed_strings[row_index]) {
            log_error(LOG_GUI, "Failed to allocate changed string\n");
            whd_free((void *)row_data);
            break;
        }
        sprintf(changed_strings[row_index], "%d", session->icons_changed);
        
        /* Allocate and format date string */
        formatted_dates[row_index] = (char *)whd_malloc(32);
        if (!formatted_dates[row_index]) {
            log_error(LOG_GUI, "Failed to allocate formatted date string\n");
            whd_free(changed_strings[row_index]);
            whd_free((void *)row_data);
            break;
        }
        
        /* Format the timestamp from session_id (YYYYMMDD_HHMMSS) to human-readable */
        if (!iTidy_FormatTimestamp(session->session_id, formatted_dates[row_index], 32)) {
            /* If formatting fails, fall back to original date_string */
            strncpy(formatted_dates[row_index], session->date_string, 31);
            formatted_dates[row_index][31] = '\0';
        }
        
        /* Point to session data */
        row_data[0] = formatted_dates[row_index];
        row_data[1] = session->mode;
        row_data[2] = session->scanned_path;
        row_data[3] = changed_strings[row_index];
        
        data_rows[row_index] = row_data;
        row_index++;
    }
    
    /* Format the list - use stored ListView width for flexible column expansion */
    log_debug(LOG_GUI, "populate_session_list: Calling formatter with %u rows, width=%u\n", 
              count, data->listview_width);
    data->session_display_list = iTidy_FormatListViewColumns(columns, 4, data_rows, count, 
                                                              data->listview_width);
    
    if (!data->session_display_list) {
        log_error(LOG_GUI, "populate_session_list: Formatter returned NULL!\n");
    } else {
        log_debug(LOG_GUI, "populate_session_list: Formatter succeeded\n");
    }
    
    /* Free temporary data */
    log_debug(LOG_GUI, "populate_session_list: Freeing temporary data\n");
    for (row_index = 0; row_index < (int)count; row_index++) {
        if (data_rows[row_index]) {
            whd_free((void *)data_rows[row_index]);
        }
        if (changed_strings[row_index]) {
            whd_free(changed_strings[row_index]);
        }
        if (formatted_dates[row_index]) {
            whd_free(formatted_dates[row_index]);
        }
    }
    whd_free(data_rows);
    whd_free(changed_strings);
    whd_free(formatted_dates);
    
    /* Refresh ListView */
    log_debug(LOG_GUI, "populate_session_list: Refreshing ListView gadget\n");
    if (data->session_listview && data->window) {
        GT_SetGadgetAttrs(data->session_listview, data->window, NULL,
                          GTLV_Labels, data->session_display_list,
                          TAG_DONE);
        log_info(LOG_GUI, "populate_session_list: ListView updated with %u sessions\n", count);
    } else {
        log_warning(LOG_GUI, "populate_session_list: ListView gadget or window not available!\n");
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
    UWORD count;
    
    /* Clear existing changes display list */
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
        /* Create display nodes pointing to change display_text */
        for (change_node = data->changes_list.lh_Head; 
             change_node->ln_Succ != NULL; 
             change_node = change_node->ln_Succ)
        {
            change = (iTidy_ToolChange *)change_node;
            
            log_message(LOG_GUI, LOG_LEVEL_DEBUG,
                        "DISPLAY CHANGE: display_text='%s' ln_Name='%s'",
                        change->display_text ? change->display_text : "(null)",
                        change->node.ln_Name ? change->node.ln_Name : "(null)");
            
            display_node = AllocVec(sizeof(struct Node), MEMF_CLEAR);
            if (display_node) {
                display_node->ln_Name = change->display_text;
                
                log_message(LOG_GUI, LOG_LEVEL_DEBUG,
                            "DISPLAY NODE: ln_Name='%s'",
                            display_node->ln_Name ? display_node->ln_Name : "(null)");
                
                AddTail(&data->changes_display_list, display_node);
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
 * @brief Handle session ListView selection
 */
static void handle_session_selection(iTidy_ToolRestoreData *data, LONG selected_index)
{
    iTidy_ToolBackupSession *session;
    struct Node *node;
    LONG index;
    LONG session_index;
    
    /* Skip header rows (indices 0 and 1) */
    if (selected_index < 2) {
        log_info(LOG_GUI, "[RESTORE] Header row selected, ignoring\n");
        data->selected_session_index = -1;
        data->selected_session_id[0] = '\0';
        
        /* Disable restore button */
        if (data->restore_button && data->window) {
            GT_SetGadgetAttrs(data->restore_button, data->window, NULL,
                              GA_Disabled, TRUE,
                              TAG_DONE);
        }
        return;
    }
    
    data->selected_session_index = selected_index;
    data->selected_session_id[0] = '\0';
    
    /* Find the session at this index (accounting for 2 header rows) */
    session_index = selected_index - 2;
    index = 0;
    for (node = data->session_list.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ)
    {
        if (index == session_index) {
            session = (iTidy_ToolBackupSession *)node;
            strncpy(data->selected_session_id, session->session_id, 31);
            data->selected_session_id[31] = '\0';
            break;
        }
        index++;
    }
    
    /* Populate changes list */
    populate_changes_list(data);
    
    /* Enable/disable restore button based on selection */
    if (data->restore_button && data->window) {
        GT_SetGadgetAttrs(data->restore_button, data->window, NULL,
                          GA_Disabled, (selected_index < 2),
                          TAG_DONE);
    }
}

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
    if (iTidy_RestoreAllIcons(data->selected_session_id, &success_count, &failed_count)) {
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
    UWORD list_width, list_height;
    struct TextAttr listview_font;
    UWORD font_char_width;
    
    (void)backup_manager;  /* Not used - we call iTidy_ScanBackupSessions directly */
    
    /* Use system font from global prefsIControl (already loaded at startup) */
    listview_font.ta_Name = prefsIControl.systemFontName;
    listview_font.ta_YSize = prefsIControl.systemFontSize;
    listview_font.ta_Style = FS_NORMAL;
    listview_font.ta_Flags = 0;
    font_char_width = prefsIControl.systemFontCharWidth;
    
    log_info(LOG_GUI, "Using system font for ListView: %s size=%u, char_width=%u\n", 
             listview_font.ta_Name, listview_font.ta_YSize, font_char_width);
    log_info(LOG_GUI, "Screen font: %s size=%u\n", 
             screen->Font->ta_Name, screen->Font->ta_YSize);
    
    /* Allocate window data */
    data = AllocVec(sizeof(iTidy_ToolRestoreData), MEMF_CLEAR);
    if (!data) {
        return NULL;
    }
    
    data->selected_session_index = -1;
    data->session_display_list = NULL;  /* Will be created by formatter */
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
    list_height = (WINDOW_HEIGHT - top_edge - 80) / 2;  /* Split vertically */
    
    /* Store ListView width in CHARACTERS for formatter (not pixels!) */
    /* Account for: scrollbar (~20px), left border (~4px), right border (~4px), padding (~8px) = ~36px total */
    data->listview_width = (list_width - 36) / font_char_width;
    
    log_info(LOG_GUI, "ListView: pixel_width=%u, border_adjust=%u, font_char_width=%u, char_width=%u\n",
             list_width, list_width - 36, font_char_width, data->listview_width);
    
    /* Session ListView (top) */
    ng.ng_LeftEdge = left_edge;
    ng.ng_TopEdge = top_edge + 14;
    ng.ng_Width = list_width;
    ng.ng_Height = list_height;
    ng.ng_GadgetText = "Backup Sessions:";
    ng.ng_TextAttr = &listview_font;  /* Use icon font from preferences */
    ng.ng_GadgetID = GID_SESSION_LIST;
    ng.ng_Flags = PLACETEXT_ABOVE;
    ng.ng_VisualInfo = data->visual_info;
    
    gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
                       GTLV_Labels, &data->session_display_list,
                       GTLV_ShowSelected, NULL,
                       TAG_DONE);
    data->session_listview = gad;
    
    /* Changes ListView (bottom) */
    ng.ng_TopEdge = top_edge + 14 + list_height + 16;
    ng.ng_GadgetText = "Tool Changes in Session:";
    ng.ng_GadgetID = GID_CHANGES_LIST;
    
    gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
                       GTLV_Labels, &data->changes_display_list,
                       GTLV_ReadOnly, TRUE,
                       TAG_DONE);
    data->changes_listview = gad;
    
    /* Restore All button */
    ng.ng_LeftEdge = left_edge;
    ng.ng_TopEdge = top_edge + 14 + (list_height * 2) + 30;
    ng.ng_Width = 120;
    ng.ng_Height = 20;
    ng.ng_GadgetText = "Restore All";
    ng.ng_GadgetID = GID_RESTORE_ALL;
    ng.ng_Flags = PLACETEXT_IN;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng,
                       GA_Disabled, TRUE,  /* Disabled until selection */
                       TAG_DONE);
    data->restore_button = gad;
    
    /* Close button */
    ng.ng_LeftEdge = WINDOW_WIDTH - 120 - 8;
    ng.ng_GadgetText = "Close";
    ng.ng_GadgetID = GID_CLOSE;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    data->close_button = gad;
    
    /* Open window */
    data->window = OpenWindowTags(NULL,
        WA_Left, (screen->Width - WINDOW_WIDTH) / 2,
        WA_Top, (screen->Height - WINDOW_HEIGHT) / 2,
        WA_Width, WINDOW_WIDTH,
        WA_Height, WINDOW_HEIGHT,
        WA_Title, (ULONG)WINDOW_TITLE,
        WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_ACTIVATE,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_GADGETUP | LISTVIEWIDCMP | BUTTONIDCMP,
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
                    handle_session_selection(data, code);
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
        
        /* Free session display list (managed by formatter) */
        if (data->session_display_list) {
            iTidy_FreeFormattedList(data->session_display_list);
            data->session_display_list = NULL;
        }
        
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
