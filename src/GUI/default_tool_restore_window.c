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
#include "helpers/listview_columns_api.h"
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

/*------------------------------------------------------------------------*/
/* Column Configuration for Session List                                  */
/*------------------------------------------------------------------------*/
#define NUM_SESSION_LIST_COLUMNS 4

static iTidy_ColumnConfig session_list_columns[] = {
    /* Date/Time: Auto-sized, left-aligned, default sort descending (newest first) */
    {"Date/Time", 0, 0, ITIDY_ALIGN_LEFT, FALSE, FALSE,
     ITIDY_SORT_DESCENDING, ITIDY_COLTYPE_DATE},
    
    /* Mode: Auto-sized, left-aligned, sortable by text */
    {"Mode", 0, 0, ITIDY_ALIGN_LEFT, FALSE, FALSE,
     ITIDY_SORT_NONE, ITIDY_COLTYPE_TEXT},
    
    /* Path: Flexible column, left-aligned, sortable by text, path abbreviation enabled */
    {"Path", 0, 0, ITIDY_ALIGN_LEFT, TRUE, TRUE,
     ITIDY_SORT_NONE, ITIDY_COLTYPE_TEXT},
    
    /* Changed: Auto-sized, right-aligned, sortable by number */
    {"Changed", 0, 0, ITIDY_ALIGN_RIGHT, FALSE, FALSE,
     ITIDY_SORT_NONE, ITIDY_COLTYPE_NUMBER}
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
    
    /* Display lists for ListViews (managed by formatter) */
    struct List *session_display_list;  /* Formatted session list (from formatter) */
    struct List changes_display_list;   /* Formatted strings for changes ListView */
    
    /* Sorting support for session ListView */
    struct List session_entry_list;     /* List of iTidy_ListViewEntry for sorting */
    iTidy_ListViewState *session_lv_state; /* State for column sorting (legacy, now via session) */
    iTidy_ListViewSession *session_lv_session; /* Session wrapper for ListView */
    int last_sort_column;               /* Track which column was last sorted (-1 = none) */
    
    /* Layout dimensions */
    UWORD listview_width;               /* Width of ListViews for formatter */
    
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
    
    /* Free session ListView resources (entry list, display list, and state) */
    if (data->session_lv_session) {
        iTidy_ListViewSessionDestroy(data->session_lv_session);
        data->session_lv_session = NULL;
        data->session_display_list = NULL;
        data->session_lv_state = NULL;
    } else {
        itidy_free_listview_entries(&data->session_entry_list,
                                    data->session_display_list,
                                    data->session_lv_state);
        data->session_display_list = NULL;
        data->session_lv_state = NULL;
    }
    if (data->session_entry_list.lh_TailPred != NULL) {
        itidy_free_listview_entries(&data->session_entry_list, NULL, NULL);
        NewList(&data->session_entry_list);
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
 * @brief Populate the session ListView with sortable backup sessions
 */
static void populate_session_list(iTidy_ToolRestoreData *data)
{
    iTidy_ToolBackupSession *session;
    struct Node *session_node;
    iTidy_ListViewEntry *entry;
    UWORD count;
    
    log_debug(LOG_GUI, "populate_session_list: Starting...\n");
    
    /* Set busy pointer during loading */
    if (data->window) {
        SetWindowPointer(data->window, WA_BusyPointer, TRUE, TAG_DONE);
    }
    
    /* Free old ListView resources */
    log_debug(LOG_GUI, "populate_session_list: Freeing old ListView data\n");
    if (data->session_lv_session) {
        iTidy_ListViewSessionDestroy(data->session_lv_session);
        data->session_lv_session = NULL;
        data->session_display_list = NULL;
        data->session_lv_state = NULL;
    } else {
        itidy_free_listview_entries(&data->session_entry_list,
                                    data->session_display_list,
                                    data->session_lv_state);
        data->session_display_list = NULL;
        data->session_lv_state = NULL;
    }
    if (data->session_entry_list.lh_TailPred != NULL) {
        itidy_free_listview_entries(&data->session_entry_list, NULL, NULL);
    }
    NewList(&data->session_entry_list);
    
    /* Free old session data */
    iTidy_FreeSessionList(&data->session_list);
    NewList(&data->session_list);
    
    /* Scan for backup sessions */
    log_debug(LOG_GUI, "populate_session_list: Scanning for backup sessions...\n");
    count = iTidy_ScanBackupSessions(&data->session_list);
    log_info(LOG_GUI, "populate_session_list: Found %u backup sessions\n", count);
    
    if (count == 0) {
        log_info(LOG_GUI, "populate_session_list: No backups found, showing empty list\n");
        iTidy_ListViewOptions options;
        iTidy_InitListViewOptions(&options);
        options.columns = session_list_columns;
        options.num_columns = NUM_SESSION_LIST_COLUMNS;
        options.entries = &data->session_entry_list;
        options.total_char_width = data->listview_width;
        options.out_state = &data->session_lv_state;
        options.mode = ITIDY_MODE_FULL;
        options.page_size = 0;
        options.current_page = 1;
        options.out_total_pages = NULL;
        options.nav_direction = 0;
        data->session_lv_session = iTidy_ListViewSessionCreate(&options);
        if (data->session_lv_session) {
            data->session_display_list = iTidy_ListViewSessionFormat(data->session_lv_session);
        } else {
            data->session_display_list = NULL;
        }
        if (data->session_listview && data->window) {
            GT_SetGadgetAttrs(data->session_listview, data->window, NULL,
                              GTLV_Labels, data->session_display_list,
                              TAG_DONE);
        }
        return;
    }
    
    /* Build entry list from session data */
    log_debug(LOG_GUI, "populate_session_list: Building %u entry nodes\n", count);
    for (session_node = data->session_list.lh_Head; 
         session_node->ln_Succ != NULL; 
         session_node = session_node->ln_Succ)
    {
        char *changed_str, *formatted_date;
        int i;
        
        session = (iTidy_ToolBackupSession *)session_node;
        
        log_debug(LOG_GUI, "populate_session_list: Processing session: %s | %s | %s | %d\n",
                  session->date_string, session->mode, 
                  session->scanned_path, session->icons_changed);
        
        /* Allocate entry node */
        entry = (iTidy_ListViewEntry *)whd_malloc(sizeof(iTidy_ListViewEntry));
        if (!entry) {
            log_error(LOG_GUI, "Failed to allocate entry node\n");
            break;
        }

        memset(entry, 0, sizeof(iTidy_ListViewEntry));
        entry->node.ln_Type = NT_USER;
        entry->source_entry = entry;
        entry->num_columns = 4;
        entry->row_type = ITIDY_ROW_DATA;  /* Normal data row */
        
        /* Allocate column arrays */
        entry->display_data = (const char **)whd_malloc(sizeof(const char *) * 4);
        entry->sort_keys = (const char **)whd_malloc(sizeof(const char *) * 4);
        
        if (!entry->display_data || !entry->sort_keys) {
            log_error(LOG_GUI, "Failed to allocate entry arrays\n");
            if (entry->display_data) whd_free((void *)entry->display_data);
            if (entry->sort_keys) whd_free((void *)entry->sort_keys);
            whd_free(entry);
            break;
        }
        
        /* Initialize arrays to NULL for safe cleanup */
        for (i = 0; i < 4; i++) {
            entry->display_data[i] = NULL;
            entry->sort_keys[i] = NULL;
        }
        
        /* Column 0: Date/Time - display formatted, sort by session_id */
        formatted_date = (char *)whd_malloc(32);
        if (!formatted_date) {
            log_error(LOG_GUI, "Failed to allocate formatted date\n");
            goto cleanup_entry;
        }
        
        if (!iTidy_FormatTimestamp(session->session_id, formatted_date, 32)) {
            strncpy(formatted_date, session->date_string, 31);
            formatted_date[31] = '\0';
        }
        
        entry->display_data[0] = formatted_date;
        
        /* Sort key: duplicate session_id string */
        entry->sort_keys[0] = (char *)whd_malloc(strlen(session->session_id) + 1);
        if (!entry->sort_keys[0]) {
            log_error(LOG_GUI, "Failed to allocate session_id sort key\n");
            goto cleanup_entry;
        }
        strcpy((char *)entry->sort_keys[0], session->session_id);
        
        /* Column 1: Mode - same for display and sort */
        entry->display_data[1] = (char *)whd_malloc(strlen(session->mode) + 1);
        if (!entry->display_data[1]) {
            log_error(LOG_GUI, "Failed to allocate mode display string\n");
            goto cleanup_entry;
        }
        strcpy((char *)entry->display_data[1], session->mode);
        
        entry->sort_keys[1] = (char *)whd_malloc(strlen(session->mode) + 1);
        if (!entry->sort_keys[1]) {
            log_error(LOG_GUI, "Failed to allocate mode sort key\n");
            goto cleanup_entry;
        }
        strcpy((char *)entry->sort_keys[1], session->mode);
        
        /* Column 2: Path - same for display and sort */
        entry->display_data[2] = (char *)whd_malloc(strlen(session->scanned_path) + 1);
        if (!entry->display_data[2]) {
            log_error(LOG_GUI, "Failed to allocate path display string\n");
            goto cleanup_entry;
        }
        strcpy((char *)entry->display_data[2], session->scanned_path);
        
        entry->sort_keys[2] = (char *)whd_malloc(strlen(session->scanned_path) + 1);
        if (!entry->sort_keys[2]) {
            log_error(LOG_GUI, "Failed to allocate path sort key\n");
            goto cleanup_entry;
        }
        strcpy((char *)entry->sort_keys[2], session->scanned_path);
        
        /* Column 3: Changed count - display as string, sort as padded number */
        changed_str = (char *)whd_malloc(16);
        if (!changed_str) {
            log_error(LOG_GUI, "Failed to allocate changed string\n");
            goto cleanup_entry;
        }
        sprintf(changed_str, "%d", session->icons_changed);
        entry->display_data[3] = changed_str;
        
        /* Sort key: zero-padded for numeric sorting */
        entry->sort_keys[3] = (char *)whd_malloc(16);
        if (!entry->sort_keys[3]) {
            log_error(LOG_GUI, "Failed to allocate sort key\n");
            goto cleanup_entry;
        }
        sprintf((char *)entry->sort_keys[3], "%08d", session->icons_changed);
        
        /* Verify all allocations succeeded */
        if (!entry->display_data[0] || !entry->display_data[1] || 
            !entry->display_data[2] || !entry->display_data[3] ||
            !entry->sort_keys[0] || !entry->sort_keys[1] || 
            !entry->sort_keys[2] || !entry->sort_keys[3]) {
            log_error(LOG_GUI, "Failed to allocate entry strings\n");
            goto cleanup_entry;
        }
        
        /* Add to entry list */
        AddTail(&data->session_entry_list, (struct Node *)entry);
        continue;
        
cleanup_entry:
        /* Clean up on allocation failure */
        for (i = 0; i < 4; i++) {
            if (entry->display_data && entry->display_data[i]) {
                whd_free((void *)entry->display_data[i]);
            }
            if (entry->sort_keys && entry->sort_keys[i]) {
                whd_free((void *)entry->sort_keys[i]);
            }
        }
        if (entry->display_data) whd_free((void *)entry->display_data);
        if (entry->sort_keys) whd_free((void *)entry->sort_keys);
        whd_free(entry);
        break;
    }
    
    /* Format the list with sorting enabled */
    log_debug(LOG_GUI, "populate_session_list: Calling formatter with %u entries, width=%u\n", 
              count, data->listview_width);
    {
        iTidy_ListViewOptions options;
        iTidy_InitListViewOptions(&options);
        options.columns = session_list_columns;
        options.num_columns = NUM_SESSION_LIST_COLUMNS;
        options.entries = &data->session_entry_list;
        options.total_char_width = data->listview_width;
        options.out_state = &data->session_lv_state;
        options.mode = ITIDY_MODE_FULL;
        options.page_size = 0;
        options.current_page = 1;
        options.out_total_pages = NULL;
        options.nav_direction = 0;
        data->session_lv_session = iTidy_ListViewSessionCreate(&options);
        if (data->session_lv_session) {
            data->session_display_list = iTidy_ListViewSessionFormat(data->session_lv_session);
        } else {
            data->session_display_list = NULL;
        }
        if (!data->session_display_list) {
            log_error(LOG_GUI, "populate_session_list: Formatter returned NULL!\n");
        } else {
            log_debug(LOG_GUI, "populate_session_list: Formatter succeeded\n");
        }
    }
    
    /* Update the ListView gadget */
    log_debug(LOG_GUI, "populate_session_list: Updating ListView gadget\n");
    if (data->session_listview && data->window) {
        GT_SetGadgetAttrs(data->session_listview, data->window, NULL,
                          GTLV_Labels, data->session_display_list,
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
    data->session_lv_state = NULL;  /* Will be created by formatter */
    NewList(&data->session_list);
    NewList(&data->session_entry_list);  /* Initialize entry list for sorting */
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
                       GTLV_Labels, data->session_display_list,  /* Pass list pointer, NOT &pointer! */
                       GTLV_ShowSelected, NULL,
                       TAG_DONE);
    data->session_listview = gad;
    
    /* Changes ListView (bottom) */
    ng.ng_TopEdge = top_edge + 14 + list_height + 16;
    ng.ng_GadgetText = "Tool Changes in Session:";
    ng.ng_GadgetID = GID_CHANGES_LIST;
    
    gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
                       GTLV_Labels, data->changes_display_list,  /* Pass list pointer, NOT &pointer! */
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
                        /* Use unified handler for ALL ListView events */
                        iTidy_ListViewEvent event;
                        
                        if (iTidy_HandleListViewGadgetUp(
                                window,
                                data->session_listview,
                                msg->MouseX, msg->MouseY,
                                &data->session_entry_list,
                                data->session_display_list,
                                data->session_lv_state,
                                prefsIControl.systemFontSize,
                                prefsIControl.systemFontCharWidth,
                                session_list_columns,
                                NUM_SESSION_LIST_COLUMNS,
                                &event))
                        {
                            /* Refresh gadget if list was sorted */
                            if (event.did_sort)
                            {
                                GT_SetGadgetAttrs(data->session_listview,
                                                 window, NULL,
                                                 GTLV_Labels, ~0,
                                                 TAG_DONE);
                                GT_SetGadgetAttrs(data->session_listview,
                                                 window, NULL,
                                                 GTLV_Labels, data->session_display_list,
                                                 TAG_DONE);
                            }
                            
                            /* Handle event by type */
                            switch (event.type)
                            {
                                case ITIDY_LV_EVENT_HEADER_SORTED:
                                    log_info(LOG_GUI, "Session list sorted by column %d, order %d\n",
                                            event.sorted_column, event.sort_order);
                                    break;
                                
                                case ITIDY_LV_EVENT_ROW_CLICK:
                                    /* Process row selection - session selected */
                                    if (event.entry != NULL)
                                    {
                                        iTidy_ToolBackupSession *session;
                                        struct Node *node;
                                        LONG session_index = 0;
                                        const char *selected_date_time = event.entry->display_data[0];
                                        
                                        /* Find session matching the selected date/time (column 0) */
                                        for (node = data->session_list.lh_Head; 
                                             node->ln_Succ != NULL; 
                                             node = node->ln_Succ, session_index++)
                                        {
                                            session = (iTidy_ToolBackupSession *)node;
                                            
                                            if (strcmp(session->date_string, selected_date_time) == 0)
                                            {
                                                /* Found matching session */
                                                data->selected_session_index = session_index;
                                                strncpy(data->selected_session_id, session->session_id, 
                                                       sizeof(data->selected_session_id) - 1);
                                                data->selected_session_id[sizeof(data->selected_session_id) - 1] = '\0';
                                                
                                                log_info(LOG_GUI, "Selected session %ld: %s (ID=%s, %d changes)\n",
                                                        session_index, session->date_string, 
                                                        session->session_id, session->icons_changed);
                                                
                                                /* Enable Restore All button */
                                                if (data->restore_button && window) {
                                                    GT_SetGadgetAttrs(data->restore_button, window, NULL,
                                                                     GA_Disabled, FALSE,
                                                                     TAG_DONE);
                                                }
                                                
                                                /* Populate changes list for this session */
                                                populate_changes_list(data);
                                                
                                                break;
                                            }
                                        }
                                    }
                                    break;
                                
                                case ITIDY_LV_EVENT_NONE:
                                    /* Click outside valid area - ignore */
                                    break;
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
        
        /* Free session ListView resources (entry list, display list, and state) */
        if (data->session_lv_session) {
            iTidy_ListViewSessionDestroy(data->session_lv_session);
            data->session_lv_session = NULL;
            data->session_display_list = NULL;
            data->session_lv_state = NULL;
        } else {
            itidy_free_listview_entries(&data->session_entry_list,
                                        data->session_display_list,
                                        data->session_lv_state);
            data->session_display_list = NULL;
            data->session_lv_state = NULL;
        }
        if (data->session_entry_list.lh_TailPred != NULL) {
            itidy_free_listview_entries(&data->session_entry_list, NULL, NULL);
            NewList(&data->session_entry_list);
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
