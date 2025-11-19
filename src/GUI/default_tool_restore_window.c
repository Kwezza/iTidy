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
    
    /* Sorting support for session ListView */
    struct List session_entry_list;     /* List of iTidy_ListViewEntry for sorting */
    iTidy_ListViewState *session_lv_state; /* State for column sorting */
    int last_sort_column;               /* Track which column was last sorted (-1 = none) */
    
    /* Layout dimensions */
    UWORD listview_width;               /* Width of ListViews for formatter */
    
};

/* ============================================
 * Forward Declarations
 * ============================================ */

static void populate_session_list(iTidy_ToolRestoreData *data);
static void populate_changes_list(iTidy_ToolRestoreData *data);
static void handle_session_selection(iTidy_ToolRestoreData *data, LONG selected_index, struct IntuiMessage *msg);
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
    itidy_free_listview_entries(&data->session_entry_list,
                                data->session_display_list,
                                data->session_lv_state);
    data->session_display_list = NULL;
    data->session_lv_state = NULL;
    
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
    iTidy_ColumnConfig columns[4];
    
    log_debug(LOG_GUI, "populate_session_list: Starting...\n");
    
    /* Set busy pointer during loading */
    if (data->window) {
        SetWindowPointer(data->window, WA_BusyPointer, TRUE, TAG_DONE);
    }
    
    /* Free old ListView resources */
    log_debug(LOG_GUI, "populate_session_list: Freeing old ListView data\n");
    itidy_free_listview_entries(&data->session_entry_list,
                                data->session_display_list,
                                data->session_lv_state);
    data->session_display_list = NULL;
    data->session_lv_state = NULL;
    NewList(&data->session_entry_list);
    
    /* Free old session data */
    iTidy_FreeSessionList(&data->session_list);
    NewList(&data->session_list);
    
    /* Scan for backup sessions */
    log_debug(LOG_GUI, "populate_session_list: Scanning for backup sessions...\n");
    count = iTidy_ScanBackupSessions(&data->session_list);
    log_info(LOG_GUI, "populate_session_list: Found %u backup sessions\n", count);
    
    /* Define columns with sorting enabled */
    columns[0].title = "Date/Time";
    columns[0].min_width = 0;
    columns[0].max_width = 20;
    columns[0].align = ITIDY_ALIGN_LEFT;
    columns[0].flexible = FALSE;
    columns[0].is_path = FALSE;
    columns[0].default_sort = ITIDY_SORT_DESCENDING;  /* Most recent first */
    columns[0].sort_type = ITIDY_COLTYPE_DATE;
    
    columns[1].title = "Mode";
    columns[1].min_width = 0;
    columns[1].max_width = 8;
    columns[1].align = ITIDY_ALIGN_LEFT;
    columns[1].flexible = FALSE;
    columns[1].is_path = FALSE;
    columns[1].default_sort = ITIDY_SORT_NONE;
    columns[1].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[2].title = "Path";
    columns[2].min_width = 30;
    columns[2].max_width = 0;  /* Unlimited */
    columns[2].align = ITIDY_ALIGN_LEFT;
    columns[2].flexible = TRUE;  /* This column absorbs extra space */
    columns[2].is_path = TRUE;   /* Enable path abbreviation with /../ notation */
    columns[2].default_sort = ITIDY_SORT_NONE;
    columns[2].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[3].title = "Changed";
    columns[3].min_width = 0;
    columns[3].max_width = 12;
    columns[3].align = ITIDY_ALIGN_RIGHT;
    columns[3].flexible = FALSE;
    columns[3].is_path = FALSE;
    columns[3].default_sort = ITIDY_SORT_NONE;
    columns[3].sort_type = ITIDY_COLTYPE_NUMBER;
    
    if (count == 0) {
        log_info(LOG_GUI, "populate_session_list: No backups found, showing empty list\n");
        
        /* Format empty list with headers only */
        data->session_display_list = iTidy_FormatListViewColumns(
            columns, 4, &data->session_entry_list, data->listview_width, 
            &data->session_lv_state);
        
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
        
        entry->num_columns = 4;
        entry->node.ln_Name = NULL;  /* Formatter will set this */
        entry->display_data = NULL;
        entry->sort_keys = NULL;
        
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
    data->session_display_list = iTidy_FormatListViewColumns(
        columns, 4, &data->session_entry_list, data->listview_width,
        &data->session_lv_state);
    
    if (!data->session_display_list) {
        log_error(LOG_GUI, "populate_session_list: Formatter returned NULL!\n");
    } else {
        log_debug(LOG_GUI, "populate_session_list: Formatter succeeded\n");
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
 * @brief Handle session ListView selection
 */
static void handle_session_selection(iTidy_ToolRestoreData *data, LONG selected_index, struct IntuiMessage *msg)
{
    iTidy_ToolBackupSession *session;
    struct Node *node;
    LONG index;
    LONG session_index;
    
    /* Check if header row clicked (indices 0 or 1) */
    if (selected_index < 2) {
        log_info(LOG_GUI, "[RESTORE] Header row selected (index=%ld), attempting column sort\n", selected_index);
        
        /* Header row 0 contains the column headers - this is where we want to sort */
        if (selected_index == 0 && data->session_lv_state && data->session_display_list && msg) {
            int col_to_sort = -1;
            iTidy_SortOrder new_order;
            
            /* Calculate which column was clicked based on mouse X position */
            WORD mouse_x = msg->MouseX;
            WORD gadget_left = data->session_listview->LeftEdge;
            WORD relative_x = mouse_x - gadget_left;
            
            /* Convert pixel position to character position */
            int char_pos = relative_x / prefsIControl.systemFontCharWidth;
            
            /* Find which column contains this character position */
            for (int i = 0; i < data->session_lv_state->num_columns; i++) {
                if (char_pos >= data->session_lv_state->columns[i].char_start &&
                    char_pos < data->session_lv_state->columns[i].char_end) {
                    col_to_sort = i;
                    break;
                }
            }
            
            /* If no column found (shouldn't happen), default to column 0 */
            if (col_to_sort < 0) {
                log_warning(LOG_GUI, "Could not determine column from mouse position, defaulting to 0\n");
                col_to_sort = 0;
            }
            
            /* Find the currently sorted column */
            int current_sorted = -1;
            iTidy_SortOrder current_order = ITIDY_SORT_NONE;
            
            for (int i = 0; i < data->session_lv_state->num_columns; i++) {
                if (data->session_lv_state->columns[i].sort_state != ITIDY_SORT_NONE) {
                    current_sorted = i;
                    current_order = data->session_lv_state->columns[i].sort_state;
                    break;
                }
            }
            
            /* Decide sort order: toggle if same column, else use default for column */
            if (current_sorted == col_to_sort) {
                /* Clicking same column - toggle direction */
                new_order = (current_order == ITIDY_SORT_ASCENDING) ? ITIDY_SORT_DESCENDING : ITIDY_SORT_ASCENDING;
            } else {
                /* Different column clicked - use default sort for that column */
                if (col_to_sort == 0) {
                    /* Date column - default descending (newest first) */
                    new_order = ITIDY_SORT_DESCENDING;
                } else {
                    /* Other columns - default ascending */
                    new_order = ITIDY_SORT_ASCENDING;
                }
            }
            
            /* Re-sort and rebuild display */
            iTidy_ColumnConfig columns[4];
            
            /* Set all default_sort to NONE since we're manually sorting */
            columns[0].title = "Date/Time";
            columns[0].min_width = 0;
            columns[0].max_width = 20;
            columns[0].align = ITIDY_ALIGN_LEFT;
            columns[0].flexible = FALSE;
            columns[0].is_path = FALSE;
            columns[0].default_sort = ITIDY_SORT_NONE;  /* Don't auto-sort - we already did it */
            columns[0].sort_type = ITIDY_COLTYPE_DATE;
            
            columns[1].title = "Mode";
            columns[1].min_width = 0;
            columns[1].max_width = 8;
            columns[1].align = ITIDY_ALIGN_LEFT;
            columns[1].flexible = FALSE;
            columns[1].is_path = FALSE;
            columns[1].default_sort = ITIDY_SORT_NONE;
            columns[1].sort_type = ITIDY_COLTYPE_TEXT;
            
            columns[2].title = "Path";
            columns[2].min_width = 30;
            columns[2].max_width = 0;
            columns[2].align = ITIDY_ALIGN_LEFT;
            columns[2].flexible = TRUE;
            columns[2].is_path = TRUE;
            columns[2].default_sort = ITIDY_SORT_NONE;
            columns[2].sort_type = ITIDY_COLTYPE_TEXT;
            
            columns[3].title = "Changed";
            columns[3].min_width = 0;
            columns[3].max_width = 12;
            columns[3].align = ITIDY_ALIGN_RIGHT;
            columns[3].flexible = FALSE;
            columns[3].is_path = FALSE;
            columns[3].default_sort = ITIDY_SORT_NONE;
            columns[3].sort_type = ITIDY_COLTYPE_NUMBER;
            
            /* Sort the entry list using the public sort function */
            {
                iTidy_ColumnType col_type = columns[col_to_sort].sort_type;
                BOOL ascending = (new_order == ITIDY_SORT_ASCENDING);
                
                iTidy_SortListViewEntries(&data->session_entry_list, col_to_sort, col_type, ascending);
            }
            
            /* Rebuild the formatted display list */
            if (data->session_display_list) {
                iTidy_FreeFormattedList(data->session_display_list);
            }
            
            /* Free old state before creating new one */
            if (data->session_lv_state) {
                iTidy_FreeListViewState(data->session_lv_state);
                data->session_lv_state = NULL;
            }
            
            /* Clear all column sort indicators, then set the sorted column */
            for (int i = 0; i < 4; i++) {
                columns[i].default_sort = ITIDY_SORT_NONE;
            }
            columns[col_to_sort].default_sort = new_order;
            
            data->session_display_list = iTidy_FormatListViewColumns(
                columns, 4, &data->session_entry_list, data->listview_width,
                &data->session_lv_state);
            
            /* Refresh ListView */
            if (data->session_listview && data->window) {
                GT_SetGadgetAttrs(data->session_listview, data->window, NULL,
                                  GTLV_Labels, ~0,
                                  TAG_DONE);
                GT_SetGadgetAttrs(data->session_listview, data->window, NULL,
                                  GTLV_Labels, data->session_display_list,
                                  GTLV_Top, 0,  /* Reset scroll to top */
                                  GTLV_MakeVisible, 0,  /* Force recalculation */
                                  TAG_DONE);
            }
        }
        
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
            
        case IDCMP_MOUSEBUTTONS:
            /* Handle column header clicks for sorting */
            if (code == SELECTDOWN && data->session_listview && data->session_lv_state && data->session_display_list) {
                /* Define columns (must match those in populate_session_list) */
                iTidy_ColumnConfig columns[4];
                
                columns[0].title = "Date/Time";
                columns[0].min_width = 0;
                columns[0].max_width = 20;
                columns[0].align = ITIDY_ALIGN_LEFT;
                columns[0].flexible = FALSE;
                columns[0].is_path = FALSE;
                columns[0].default_sort = ITIDY_SORT_DESCENDING;
                columns[0].sort_type = ITIDY_COLTYPE_DATE;
                
                columns[1].title = "Mode";
                columns[1].min_width = 0;
                columns[1].max_width = 8;
                columns[1].align = ITIDY_ALIGN_LEFT;
                columns[1].flexible = FALSE;
                columns[1].is_path = FALSE;
                columns[1].default_sort = ITIDY_SORT_NONE;
                columns[1].sort_type = ITIDY_COLTYPE_TEXT;
                
                columns[2].title = "Path";
                columns[2].min_width = 30;
                columns[2].max_width = 0;
                columns[2].align = ITIDY_ALIGN_LEFT;
                columns[2].flexible = TRUE;
                columns[2].is_path = TRUE;
                columns[2].default_sort = ITIDY_SORT_NONE;
                columns[2].sort_type = ITIDY_COLTYPE_TEXT;
                
                columns[3].title = "Changed";
                columns[3].min_width = 0;
                columns[3].max_width = 12;
                columns[3].align = ITIDY_ALIGN_RIGHT;
                columns[3].flexible = FALSE;
                columns[3].is_path = FALSE;
                columns[3].default_sort = ITIDY_SORT_NONE;
                columns[3].sort_type = ITIDY_COLTYPE_NUMBER;
                
                /* High-level sort handler - does everything! */
                iTidy_HandleListViewSort(
                    window,
                    data->session_listview,
                    data->session_display_list,
                    &data->session_entry_list,
                    data->session_lv_state,
                    msg->MouseX, msg->MouseY,
                    prefsIControl.systemFontSize,
                    prefsIControl.systemFontCharWidth,
                    columns, 4
                );
            }
            break;
            
        case IDCMP_GADGETUP:
            switch (gad->GadgetID) {
                case GID_SESSION_LIST:
                    handle_session_selection(data, code, msg);
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
        itidy_free_listview_entries(&data->session_entry_list,
                                    data->session_display_list,
                                    data->session_lv_state);
        data->session_display_list = NULL;
        data->session_lv_state = NULL;
        
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
