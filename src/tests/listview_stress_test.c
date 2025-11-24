/**
 * listview_stress_test.c - ListView Column API Performance Benchmark
 * 
 * Purpose: Test the listview_columns_api.c performance on 7MHz Amiga 68000
 * 
 * Features:
 * - Single window with 5-column ListView (auto-sized)
 * - Two buttons: "Add 50 Rows" and "Remove 50 Rows"
 * - Initial data: 50 rows of random Amiga hardware/game facts
 * - Column types: Date, Number, Text (with at least one of each)
 * - Performance timing to measure sort/render speed
 * - Click-to-sort enabled on all columns
 * 
 * Usage: Run from Workbench or CLI, observe timing output in console
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "platform/platform.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <graphics/text.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/dos.h>

#include "../helpers/listview_columns_api.h"
#include "../Settings/IControlPrefs.h"
#include "../writeLog.h"

/* Gadget IDs */
#define GID_LISTVIEW       1
#define GID_ADD_BTN        2
#define GID_REMOVE_BTN     3
#define GID_BASELINE_BTN   4
#define GID_SIMPLE_BTN     5
#define GID_PAGINATED_BTN  6
#define GID_AUTORUN_BTN    7
#define GID_RUNALL_BTN     8

/* Global structures needed by the API and utilities */
struct IControlPrefsDetails prefsIControl;  /* Define it here */
struct RastPort *rastPort = NULL;  /* Needed by utilities.c */
struct Device* TimerBase = NULL;

/* External reference to SysBase (provided by VBCC runtime) */
extern struct ExecBase *SysBase;

/* Timer device for performance measurements */
static struct MsgPort *timer_port = NULL;
static struct timerequest *timer_req = NULL;

/* Window data structure */
typedef struct {
    struct Window *window;
    struct Gadget *gadget_list;
    void *vi;
    
    struct Gadget *listview_gad;
    struct Gadget *add_btn;
    struct Gadget *remove_btn;
    struct Gadget *baseline_btn;
    struct Gadget *simple_btn;
    struct Gadget *paginated_btn;
    struct Gadget *autorun_btn;
    struct Gadget *runall_btn;
    
    struct List entry_list;
    struct List plain_list;          /* For baseline test (raw nodes) */
    struct List *display_list;
    iTidy_ListViewState *lv_state;
    
    int font_width;
    int font_height;
    int listview_width_chars;
    
    int total_rows;
} TestWindowData;

/* Sample Amiga facts for populating ListView */
static const char *amiga_facts[] = {
    "Amiga 500", "Commodore 64", "Amiga 1200", "Amiga 4000", "Amiga 2000",
    "Amiga 3000", "CDTV", "CD32", "Amiga 1000", "Amiga 600",
    "Shadow of the Beast", "Speedball 2", "Cannon Fodder", "Lotus Turbo",
    "Sensible Soccer", "Another World", "Lemmings", "Turrican II",
    "Pinball Dreams", "The Settlers", "Stunt Car Racer", "IK+", "Zool",
    "Worms", "Superfrog", "Project X", "Xenon 2", "Wings", "SWIV",
    "Agony", "Walker", "Banshee", "Flashback", "Rise of the Robots",
    "Desert Strike", "Hired Guns", "Body Blows", "UFO Enemy Unknown",
    "Syndicate", "Populous", "Dungeon Master", "Monkey Island",
    "SimCity", "Civilization", "Elite", "F/A-18 Interceptor",
    "Frontier Elite II", "Street Fighter II", "Mortal Kombat", "Pushover"
};

#define NUM_FACTS (sizeof(amiga_facts) / sizeof(amiga_facts[0]))

/* Forward declarations */
static void free_entry(iTidy_ListViewEntry *entry);
static void free_plain_node(struct Node *node);
static void teardown_and_time(TestWindowData *data, const char *test_name, BOOL has_plain_list);
static void run_baseline_test(TestWindowData *data);
static void run_simple_test(TestWindowData *data);
static void run_paginated_test(TestWindowData *data);
static void run_all_tests(TestWindowData *data);
static void log_memory_status(int entry_count);

/**
 * @brief Open timer.device for performance measurements
 */
static BOOL open_timer_device(void)
{
    timer_port = CreateMsgPort();
    if (!timer_port) {
        return FALSE;
    }
    
    timer_req = (struct timerequest *)CreateIORequest(timer_port, sizeof(struct timerequest));
    if (!timer_req) {
        DeleteMsgPort(timer_port);
        timer_port = NULL;
        return FALSE;
    }
    
    if (OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)timer_req, 0) != 0) {
        DeleteIORequest((struct IORequest *)timer_req);
        DeleteMsgPort(timer_port);
        timer_req = NULL;
        timer_port = NULL;
        return FALSE;
    }
    
    TimerBase = (struct Device *)timer_req->tr_node.io_Device;
    return TRUE;
}

/**
 * @brief Close timer.device
 */
static void close_timer_device(void)
{
    if (timer_req) {
        CloseDevice((struct IORequest *)timer_req);
        DeleteIORequest((struct IORequest *)timer_req);
        timer_req = NULL;
    }
    if (timer_port) {
        DeleteMsgPort(timer_port);
        timer_port = NULL;
    }
    TimerBase = NULL;
}

/**
 * @brief Create a formatted date string (display) and sort key (YYYYMMDD_HHMMSS)
 */
static void generate_random_date(char *display_buf, char *sortkey_buf, int seed)
{
    int year = 1985 + (seed % 25);  /* 1985-2009 */
    int month = 1 + (seed % 12);
    int day = 1 + (seed % 28);
    int hour = seed % 24;
    int minute = seed % 60;
    int second = (seed * 17) % 60;
    
    /* Display: "24-Nov-2025 15:19" */
    static const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    sprintf(display_buf, "%02d-%s-%04d %02d:%02d", 
            day, months[month - 1], year, hour, minute);
    
    /* Sort key: "20251124_151900" */
    sprintf(sortkey_buf, "%04d%02d%02d_%02d%02d%02d",
            year, month, day, hour, minute, second);
}

/**
 * @brief Create a new ListView entry with random Amiga fact data
 */
static iTidy_ListViewEntry *create_test_entry(int index)
{
    iTidy_ListViewEntry *entry;
    char display_buf[64];
    char sortkey_buf[64];
    int fact_index;
    
    entry = (iTidy_ListViewEntry *)whd_malloc(sizeof(iTidy_ListViewEntry));
    if (!entry) {
        return NULL;
    }
    
    memset(entry, 0, sizeof(iTidy_ListViewEntry));
    entry->node.ln_Type = NT_USER;
    entry->node.ln_Name = NULL;  /* CRITICAL: Initialize to NULL (never used but freed in cleanup) */
    entry->source_entry = entry;
    entry->num_columns = 5;
    entry->row_type = ITIDY_ROW_DATA;  /* Normal data row */
    
    /* printf("[DEBUG] Creating entry %d at address 0x%08lx\n", index, (ULONG)entry); */
    
    /* Allocate arrays */
    entry->display_data = (const char **)whd_malloc(sizeof(char *) * 5);
    entry->sort_keys = (const char **)whd_malloc(sizeof(char *) * 5);
    
    /* printf("[DEBUG] Entry %d: display_data=0x%08lx, sort_keys=0x%08lx\n",
           index, (ULONG)entry->display_data, (ULONG)entry->sort_keys); */
    
    if (!entry->display_data || !entry->sort_keys) {
        printf("[ERROR] Failed to allocate arrays for entry %d\n", index);
        if (entry->display_data) whd_free((void *)entry->display_data);
        if (entry->sort_keys) whd_free((void *)entry->sort_keys);
        whd_free(entry);
        return NULL;
    }
    
    /* Initialize arrays to NULL for safety */
    memset((void *)entry->display_data, 0, sizeof(char *) * 5);
    memset((void *)entry->sort_keys, 0, sizeof(char *) * 5);
    
    /* Column 0: Date/Time (DATE type) */
    generate_random_date(display_buf, sortkey_buf, index);
    entry->display_data[0] = whd_strdup(display_buf);
    entry->sort_keys[0] = whd_strdup(sortkey_buf);
    
    /* CRITICAL: Check for allocation failure (NULL) on low-memory systems */
    if (!entry->display_data[0] || !entry->sort_keys[0]) {
        printf("[ERROR] Entry %d: Failed to allocate date strings (out of memory!)\n", index);
        free_entry(entry);
        return NULL;
    }
    
    /* Column 1: Item number (NUMBER type) */
    sprintf(display_buf, "%d", index + 1);
    sprintf(sortkey_buf, "%010d", index + 1);  /* Zero-padded for sorting */
    entry->display_data[1] = whd_strdup(display_buf);
    entry->sort_keys[1] = whd_strdup(sortkey_buf);
    
    if (!entry->display_data[1] || !entry->sort_keys[1]) {
        printf("[ERROR] Entry %d: Failed to allocate number strings (out of memory!)\n", index);
        free_entry(entry);
        return NULL;
    }
    
    /* Column 2: Category (TEXT type) */
    if (index % 3 == 0) {
        entry->display_data[2] = whd_strdup("Hardware");
        entry->sort_keys[2] = whd_strdup("Hardware");
    } else if (index % 3 == 1) {
        entry->display_data[2] = whd_strdup("Game");
        entry->sort_keys[2] = whd_strdup("Game");
    } else {
        entry->display_data[2] = whd_strdup("Software");
        entry->sort_keys[2] = whd_strdup("Software");
    }
    
    if (!entry->display_data[2] || !entry->sort_keys[2]) {
        printf("[ERROR] Entry %d: Failed to allocate category strings (out of memory!)\n", index);
        free_entry(entry);
        return NULL;
    }
    
    /* Column 3: Name (TEXT type - random Amiga fact) */
    fact_index = index % NUM_FACTS;
    entry->display_data[3] = whd_strdup(amiga_facts[fact_index]);
    entry->sort_keys[3] = whd_strdup(amiga_facts[fact_index]);
    
    if (!entry->display_data[3] || !entry->sort_keys[3]) {
        printf("[ERROR] Entry %d: Failed to allocate name strings (out of memory!)\n", index);
        free_entry(entry);
        return NULL;
    }
    
    /* Column 4: Rating (NUMBER type) */
    sprintf(display_buf, "%d/10", 5 + (index % 6));
    sprintf(sortkey_buf, "%010d", 5 + (index % 6));
    entry->display_data[4] = whd_strdup(display_buf);
    entry->sort_keys[4] = whd_strdup(sortkey_buf);
    
    if (!entry->display_data[4] || !entry->sort_keys[4]) {
        printf("[ERROR] Entry %d: Failed to allocate rating strings (out of memory!)\n", index);
        free_entry(entry);
        return NULL;
    }
    
    return entry;
}

/**
 * @brief Free a plain node (baseline test)
 */
static void free_plain_node(struct Node *node)
{
    if (!node) {
        return;
    }
    
    if (node->ln_Name) {
        whd_free(node->ln_Name);
        node->ln_Name = NULL;
    }
    
    whd_free(node);
}

/**
 * @brief Free a single ListView entry
 */
static void free_entry(iTidy_ListViewEntry *entry)
{
    int i;
    
    if (!entry) {
        return;
    }
    
    /* ln_Name should always be NULL (formatter owns this memory) */
    if (entry->node.ln_Name) {
        whd_free(entry->node.ln_Name);
        entry->node.ln_Name = NULL;
    }
    
    /* Validate num_columns before loop */
    if (entry->num_columns > 10) {
        entry->num_columns = 5;  /* Assume 5 columns for safety */
    }
    
    for (i = 0; i < entry->num_columns; i++) {
        if (entry->display_data && entry->display_data[i]) {
            whd_free((void *)entry->display_data[i]);
            entry->display_data[i] = NULL;
        }
        if (entry->sort_keys && entry->sort_keys[i]) {
            whd_free((void *)entry->sort_keys[i]);
            entry->sort_keys[i] = NULL;
        }
    }
    
    if (entry->display_data) {
        whd_free((void *)entry->display_data);
        entry->display_data = NULL;
    }
    if (entry->sort_keys) {
        whd_free((void *)entry->sort_keys);
        entry->sort_keys = NULL;
    }
    
    whd_free(entry);
}

/**
 * @brief Teardown and time cleanup between tests
 */
static void teardown_and_time(TestWindowData *data, const char *test_name, BOOL has_plain_list)
{
    struct timeval start_time, end_time, step_start;
    ULONG elapsed_micros;
    float elapsed_seconds;
    struct Node *node;
    int entry_count = 0;
    
    printf("\n=== TEARDOWN TIMING (%s) ===\n", test_name);
    GetSysTime(&start_time);
    step_start = start_time;
    
    /* Detach from gadget */
    if (data->listview_gad && data->window) {
        GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                         GTLV_Labels, ~0,
                         TAG_DONE);
    }
    
    /* Free display list */
    if (data->display_list) {
        GetSysTime(&step_start);
        iTidy_FreeFormattedList(data->display_list);
        data->display_list = NULL;
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - step_start.tv_secs) * 1000000) +
                         (end_time.tv_micro - step_start.tv_micro);
        printf("Free display_list: %.6f seconds\n", (float)elapsed_micros / 1000000.0f);
        
        /* CRITICAL: NULL out ln_Name in all entries after display list is freed
         * The formatter sets ln_Name in FULL mode, but owns that memory.
         * After freeing display_list, those pointers are invalid. */
        if (!has_plain_list) {
            iTidy_ListViewEntry *entry;
            for (node = data->entry_list.lh_Head; node->ln_Succ; node = node->ln_Succ) {
                entry = (iTidy_ListViewEntry *)node;
                entry->node.ln_Name = NULL;
            }
        }
    }
    
    /* Free state */
    if (data->lv_state) {
        GetSysTime(&step_start);
        iTidy_FreeListViewState(data->lv_state);
        data->lv_state = NULL;
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - step_start.tv_secs) * 1000000) +
                         (end_time.tv_micro - step_start.tv_micro);
        printf("Free state: %.6f seconds\n", (float)elapsed_micros / 1000000.0f);
    }
    
    /* Free entry list or plain list */
    GetSysTime(&step_start);
    whd_memory_suspend_logging();
    
    if (has_plain_list) {
        /* Free plain nodes (baseline test) */
        while ((node = RemHead(&data->plain_list)) != NULL) {
            free_plain_node(node);
            entry_count++;
        }
        printf("Free %d plain nodes: ", entry_count);
    } else {
        /* Free API entries */
        while ((node = RemHead(&data->entry_list)) != NULL) {
            free_entry((iTidy_ListViewEntry *)node);
            entry_count++;
        }
        printf("Free %d API entries: ", entry_count);
    }
    
    whd_memory_resume_logging();
    GetSysTime(&end_time);
    elapsed_micros = ((end_time.tv_secs - step_start.tv_secs) * 1000000) +
                     (end_time.tv_micro - step_start.tv_micro);
    printf("%.6f seconds\n", (float)elapsed_micros / 1000000.0f);
    if (entry_count > 0) {
        printf("  Time per entry: %.6f seconds\n", 
               (float)elapsed_micros / (float)entry_count / 1000000.0f);
    }
    
    /* Total teardown time */
    elapsed_micros = ((end_time.tv_secs - start_time.tv_secs) * 1000000) +
                     (end_time.tv_micro - start_time.tv_micro);
    elapsed_seconds = (float)elapsed_micros / 1000000.0f;
    printf("TOTAL TEARDOWN TIME: %.6f seconds\n", elapsed_seconds);
    printf("================================\n\n");
    
    data->total_rows = 0;
}

/**
 * @brief Add 50 new rows to the ListView
 */
static void add_50_rows(TestWindowData *data)
{
    int i;
    iTidy_ListViewEntry *entry;
    struct timeval start_time, end_time;
    ULONG elapsed_micros, elapsed_millis;
    float elapsed_seconds;
    iTidy_ColumnConfig columns[5];
    
    printf("\n=== ADDING 50 ROWS ===\n");
    printf("Current row count: %d\n", data->total_rows);
    printf("[DEBUG] entry_list head=0x%08lx\n", (ULONG)data->entry_list.lh_Head);
    printf("[DEBUG] add_50_rows: listview_width_chars=%d\n", data->listview_width_chars);
    
    /* Set busy pointer */
    SetWindowPointer(data->window, WA_BusyPointer, TRUE, TAG_DONE);
    
    GetSysTime(&start_time);
    
    /* Detach list from gadget */
    GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                     GTLV_Labels, ~0,
                     TAG_DONE);
    
    printf("[DEBUG] Creating 50 new entries starting at index %d\n", data->total_rows);
    
    /* Add 50 new entries */
    for (i = 0; i < 50; i++) {
        entry = create_test_entry(data->total_rows + i);
        if (entry) {
            /* printf("[DEBUG] Adding entry %d to list\n", data->total_rows + i); */
            AddTail(&data->entry_list, (struct Node *)entry);
        } else {
            printf("[ERROR] Failed to create entry %d!\n", data->total_rows + i);
            break;  /* Stop on allocation failure */
        }
    }
    
    data->total_rows += i;  /* Use actual count (might be less than 50 if allocation failed) */
    
    printf("[DEBUG] Successfully added %d entries, total now %d\n", i, data->total_rows);
    
    /* Free old display list and state */
    if (data->display_list) {
        printf("[DEBUG] Freeing old display list at 0x%08lx\n", (ULONG)data->display_list);
        iTidy_FreeFormattedList(data->display_list);
        data->display_list = NULL;
    }
    if (data->lv_state) {
        iTidy_FreeListViewState(data->lv_state);
        data->lv_state = NULL;
    }
    
    /* Reformat with new data */
    columns[0].title = "Date/Time";
    columns[0].min_width = 17;
    columns[0].max_width = 17;
    columns[0].align = ITIDY_ALIGN_LEFT;
    columns[0].flexible = FALSE;
    columns[0].is_path = FALSE;
    columns[0].default_sort = ITIDY_SORT_NONE;
    columns[0].sort_type = ITIDY_COLTYPE_DATE;
    
    columns[1].title = "#";
    columns[1].min_width = 4;
    columns[1].max_width = 6;
    columns[1].align = ITIDY_ALIGN_RIGHT;
    columns[1].flexible = FALSE;
    columns[1].is_path = FALSE;
    columns[1].default_sort = ITIDY_SORT_NONE;
    columns[1].sort_type = ITIDY_COLTYPE_NUMBER;
    
    columns[2].title = "Type";
    columns[2].min_width = 8;
    columns[2].max_width = 8;
    columns[2].align = ITIDY_ALIGN_LEFT;
    columns[2].flexible = FALSE;
    columns[2].is_path = FALSE;
    columns[2].default_sort = ITIDY_SORT_NONE;
    columns[2].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[3].title = "Name";
    columns[3].min_width = 15;
    columns[3].max_width = 200;
    columns[3].align = ITIDY_ALIGN_LEFT;
    columns[3].flexible = TRUE;
    columns[3].is_path = FALSE;
    columns[3].default_sort = ITIDY_SORT_NONE;
    columns[3].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[4].title = "Rating";
    columns[4].min_width = 6;
    columns[4].max_width = 6;
    columns[4].align = ITIDY_ALIGN_CENTER;
    columns[4].flexible = FALSE;
    columns[4].is_path = FALSE;
    columns[4].default_sort = ITIDY_SORT_NONE;
    columns[4].sort_type = ITIDY_COLTYPE_NUMBER;
    
    printf("[DEBUG] add_50_rows: About to format %d total entries\n", data->total_rows);
    
    data->display_list = iTidy_FormatListViewColumns(
        columns, 5, &data->entry_list,
        data->listview_width_chars, &data->lv_state,
        ITIDY_MODE_FULL, 0, 1, NULL, 0
    );
    
    if (!data->display_list) {
        printf("[ERROR] add_50_rows: iTidy_FormatListViewColumns returned NULL!\n");
        SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
        return;
    }
    printf("[DEBUG] add_50_rows: Format complete, list at 0x%08lx\n", (ULONG)data->display_list);
    
    /* Reattach to gadget */
    GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                     GTLV_Labels, data->display_list,
                     GTLV_Top, 0,
                     TAG_DONE);
    
    printf("[DEBUG] add_50_rows: Reattached to gadget\n");
    
    GetSysTime(&end_time);
    elapsed_micros = ((end_time.tv_secs - start_time.tv_secs) * 1000000) +
                     (end_time.tv_micro - start_time.tv_micro);
    elapsed_millis = elapsed_micros / 1000;
    elapsed_seconds = (float)elapsed_micros / 1000000.0f;
    
    /* Clear busy pointer */
    SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
    
    printf("Added 50 rows in %.6f seconds\n", elapsed_seconds);
    printf("New total: %d rows\n", data->total_rows);
    printf("Time per row: %.6f seconds\n", (float)elapsed_micros / 50.0f / 1000000.0f);
}

/**
 * @brief Remove 50 rows from the bottom of the ListView
 */
static void remove_50_rows(TestWindowData *data)
{
    int i;
    struct Node *node;
    struct timeval start_time, end_time;
    ULONG elapsed_micros, elapsed_millis;
    float elapsed_seconds;
    iTidy_ColumnConfig columns[5];
    
    if (data->total_rows < 50) {
        printf("\n=== CANNOT REMOVE: Only %d rows remaining ===\n", data->total_rows);
        return;
    }
    
    printf("\n=== REMOVING 50 ROWS ===\n");
    printf("Current row count: %d\n", data->total_rows);
    
    /* Set busy pointer */
    SetWindowPointer(data->window, WA_BusyPointer, TRUE, TAG_DONE);
    
    GetSysTime(&start_time);
    
    /* Detach list from gadget */
    GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                     GTLV_Labels, ~0,
                     TAG_DONE);
    
    /* Remove last 50 entries */
    for (i = 0; i < 50; i++) {
        node = data->entry_list.lh_TailPred;
        if (node && node->ln_Pred) {
            Remove(node);
            free_entry((iTidy_ListViewEntry *)node);
        }
    }
    
    data->total_rows -= 50;
    
    /* Free old display list and state */
    if (data->display_list) {
        iTidy_FreeFormattedList(data->display_list);
        data->display_list = NULL;
    }
    if (data->lv_state) {
        iTidy_FreeListViewState(data->lv_state);
        data->lv_state = NULL;
    }
    
    /* Reformat with remaining data */
    columns[0].title = "Date/Time";
    columns[0].min_width = 17;
    columns[0].max_width = 17;
    columns[0].align = ITIDY_ALIGN_LEFT;
    columns[0].flexible = FALSE;
    columns[0].is_path = FALSE;
    columns[0].default_sort = ITIDY_SORT_NONE;
    columns[0].sort_type = ITIDY_COLTYPE_DATE;
    
    columns[1].title = "#";
    columns[1].min_width = 4;
    columns[1].max_width = 6;
    columns[1].align = ITIDY_ALIGN_RIGHT;
    columns[1].flexible = FALSE;
    columns[1].is_path = FALSE;
    columns[1].default_sort = ITIDY_SORT_NONE;
    columns[1].sort_type = ITIDY_COLTYPE_NUMBER;
    
    columns[2].title = "Type";
    columns[2].min_width = 8;
    columns[2].max_width = 8;
    columns[2].align = ITIDY_ALIGN_LEFT;
    columns[2].flexible = FALSE;
    columns[2].is_path = FALSE;
    columns[2].default_sort = ITIDY_SORT_NONE;
    columns[2].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[3].title = "Name";
    columns[3].min_width = 15;
    columns[3].max_width = 200;
    columns[3].align = ITIDY_ALIGN_LEFT;
    columns[3].flexible = TRUE;
    columns[3].is_path = FALSE;
    columns[3].default_sort = ITIDY_SORT_NONE;
    columns[3].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[4].title = "Rating";
    columns[4].min_width = 6;
    columns[4].max_width = 6;
    columns[4].align = ITIDY_ALIGN_CENTER;
    columns[4].flexible = FALSE;
    columns[4].is_path = FALSE;
    columns[4].default_sort = ITIDY_SORT_NONE;
    columns[4].sort_type = ITIDY_COLTYPE_NUMBER;
    
    printf("[DEBUG] remove_50_rows: About to format %d total entries\n", data->total_rows);
    
    data->display_list = iTidy_FormatListViewColumns(
        columns, 5, &data->entry_list,
        data->listview_width_chars, &data->lv_state,
        ITIDY_MODE_FULL, 0, 1, NULL, 0
    );
    
    if (!data->display_list) {
        printf("[ERROR] remove_50_rows: iTidy_FormatListViewColumns returned NULL!\n");
        SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
        return;
    }
    printf("[DEBUG] remove_50_rows: Format complete, list at 0x%08lx\n", (ULONG)data->display_list);
    
    /* Reattach to gadget */
    GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                     GTLV_Labels, data->display_list,
                     GTLV_Top, 0,
                     TAG_DONE);
    
    printf("[DEBUG] remove_50_rows: Reattached to gadget\n");
    
    GetSysTime(&end_time);
    elapsed_micros = ((end_time.tv_secs - start_time.tv_secs) * 1000000) +
                     (end_time.tv_micro - start_time.tv_micro);
    elapsed_millis = elapsed_micros / 1000;
    elapsed_seconds = (float)elapsed_micros / 1000000.0f;
    
    /* Clear busy pointer */
    SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
    
    printf("Removed 50 rows in %.6f seconds\n", elapsed_seconds);
    printf("New total: %d rows\n", data->total_rows);
    printf("Time per row: %.6f seconds\n", (float)elapsed_micros / 50.0f / 1000000.0f);
}

/**
 * @brief Create and open the test window
 */
static TestWindowData *create_test_window(void)
{
    TestWindowData *data;
    struct Screen *screen;
    struct NewGadget ng;
    struct Gadget *gad;
    WORD border_top, border_left, border_right, border_bottom;
    WORD current_y, current_x;
    WORD win_width, win_height;
    WORD listview_height;
    WORD button_width, button_spacing;
    int i;
    iTidy_ListViewEntry *entry;
    iTidy_ColumnConfig columns[5];
    struct TextAttr topaz8 = { "topaz.font", 8, FS_NORMAL, FPF_ROMFONT };
    
    data = (TestWindowData *)whd_malloc(sizeof(TestWindowData));
    if (!data) {
        return NULL;
    }
    memset(data, 0, sizeof(TestWindowData));
    
    NewList(&data->entry_list);
    NewList(&data->plain_list);
    
    /* Get screen and calculate borders */
    screen = LockPubScreen("Workbench");
    if (!screen) {
        whd_free(data);
        return NULL;
    }
    
    border_top = screen->WBorTop + screen->Font->ta_YSize + 1;
    border_left = screen->WBorLeft;
    border_right = screen->WBorRight;
    border_bottom = screen->WBorBottom;
    
    data->font_width = prefsIControl.systemFontCharWidth;
    data->font_height = prefsIControl.systemFontSize;
    
    UnlockPubScreen(NULL, screen);
    
    /* Window dimensions - fit PAL Hires (640x256) with 10px margins */
    win_width = 610;
    win_height = 230;
    
    /* Create visual info */
    screen = LockPubScreen("Workbench");
    data->vi = GetVisualInfo(screen, TAG_END);
    UnlockPubScreen(NULL, screen);
    
    if (!data->vi) {
        whd_free(data);
        return NULL;
    }
    
    /* Create gadgets */
    gad = CreateContext(&data->gadget_list);
    
    /* ListView gadget */
    current_x = border_left + 10;
    current_y = border_top + 10;
    
    /* Reduced ListView height to fit PAL Hires screen */
    listview_height = 110;
    
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = win_width - border_left - border_right - 20;
    ng.ng_Height = listview_height;
    ng.ng_GadgetText = NULL;
    ng.ng_TextAttr = &topaz8;  /* Force Topaz 8 fixed-width font */
    ng.ng_GadgetID = GID_LISTVIEW;
    ng.ng_Flags = 0;
    ng.ng_VisualInfo = data->vi;
    
    /* Calculate ListView width in characters */
    data->listview_width_chars = (ng.ng_Width - 36) / data->font_width;
    
    gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
                      GTLV_Labels, NULL,
                      GTLV_ShowSelected, NULL,
                      TAG_DONE);
    data->listview_gad = gad;
    
    /* Button rows - 2 rows of buttons below ListView */
    current_y += listview_height + 8;
    
    button_width = 90;
    button_spacing = 6;
    
    /* Row 1: Add | Remove | Baseline | Simple */
    current_x = border_left + 10;
    
    /* Add button */
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = button_width;
    ng.ng_Height = data->font_height + 6;
    ng.ng_GadgetText = "Add 50";
    ng.ng_GadgetID = GID_ADD_BTN;
    ng.ng_Flags = PLACETEXT_IN;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    data->add_btn = gad;
    
    /* Remove button */
    ng.ng_LeftEdge = current_x + button_width + button_spacing;
    ng.ng_GadgetText = "Remove 50";
    ng.ng_GadgetID = GID_REMOVE_BTN;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    data->remove_btn = gad;
    
    /* Baseline Test button */
    ng.ng_LeftEdge = current_x + (button_width + button_spacing) * 2;
    ng.ng_GadgetText = "Baseline";
    ng.ng_GadgetID = GID_BASELINE_BTN;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    data->baseline_btn = gad;
    
    /* Simple Test button */
    ng.ng_LeftEdge = current_x + (button_width + button_spacing) * 3;
    ng.ng_GadgetText = "Simple";
    ng.ng_GadgetID = GID_SIMPLE_BTN;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    data->simple_btn = gad;
    
    /* Row 2: Paginated | Auto Run | Run All */
    current_y += data->font_height + 6 + 4;
    current_x = border_left + 10;
    
    /* Paginated Test button */
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_GadgetText = "Paginated";
    ng.ng_GadgetID = GID_PAGINATED_BTN;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    data->paginated_btn = gad;
    
    /* Auto Run button */
    ng.ng_LeftEdge = current_x + button_width + button_spacing;
    ng.ng_GadgetText = "Auto Run";
    ng.ng_GadgetID = GID_AUTORUN_BTN;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    data->autorun_btn = gad;
    
    /* Run All Tests button */
    ng.ng_LeftEdge = current_x + (button_width + button_spacing) * 2;
    ng.ng_Width = (button_width * 2) + button_spacing;  /* Wider button */
    ng.ng_GadgetText = "Run All Tests";
    ng.ng_GadgetID = GID_RUNALL_BTN;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    data->runall_btn = gad;
    
    /* Open window with Topaz 8 font */
    data->window = OpenWindowTags(NULL,
        WA_Left, 50,
        WA_Top, 50,
        WA_Width, win_width,
        WA_Height, win_height,
        WA_Title, (ULONG)"ListView Stress Test - iTidy Performance Benchmark",
        WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_ACTIVATE,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN,
        WA_Gadgets, data->gadget_list,
        WA_NewLookMenus, TRUE,
        WA_PubScreen, NULL,
        TAG_DONE);
    
    if (!data->window) {
        FreeGadgets(data->gadget_list);
        FreeVisualInfo(data->vi);
        whd_free(data);
        return NULL;
    }
    
    /* Open Topaz 8 font and set it on the window */
    {
        struct TextFont *font;
        font = OpenFont(&topaz8);
        if (font) {
            SetFont(data->window->RPort, font);
            /* Note: Don't close the font - window will use it */
        }
    }
    
    /* Create initial 50 rows */
    printf("\n=== INITIALIZING LISTVIEW ===\n");
    printf("Creating 50 initial rows...\n");
    
    for (i = 0; i < 50; i++) {
        entry = create_test_entry(i);
        if (entry) {
            AddTail(&data->entry_list, (struct Node *)entry);
        }
    }
    data->total_rows = 50;
    
    /* Configure columns */
    columns[0].title = "Date/Time";
    columns[0].min_width = 17;
    columns[0].max_width = 17;
    columns[0].align = ITIDY_ALIGN_LEFT;
    columns[0].flexible = FALSE;
    columns[0].is_path = FALSE;
    columns[0].default_sort = ITIDY_SORT_DESCENDING;  /* Default sort by date */
    columns[0].sort_type = ITIDY_COLTYPE_DATE;
    
    columns[1].title = "#";
    columns[1].min_width = 4;
    columns[1].max_width = 6;
    columns[1].align = ITIDY_ALIGN_RIGHT;
    columns[1].flexible = FALSE;
    columns[1].is_path = FALSE;
    columns[1].default_sort = ITIDY_SORT_NONE;
    columns[1].sort_type = ITIDY_COLTYPE_NUMBER;
    
    columns[2].title = "Type";
    columns[2].min_width = 8;
    columns[2].max_width = 8;
    columns[2].align = ITIDY_ALIGN_LEFT;
    columns[2].flexible = FALSE;
    columns[2].is_path = FALSE;
    columns[2].default_sort = ITIDY_SORT_NONE;
    columns[2].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[3].title = "Name";
    columns[3].min_width = 15;
    columns[3].max_width = 200;
    columns[3].align = ITIDY_ALIGN_LEFT;
    columns[3].flexible = TRUE;  /* This column gets remaining space */
    columns[3].is_path = FALSE;
    columns[3].default_sort = ITIDY_SORT_NONE;
    columns[3].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[4].title = "Rating";
    columns[4].min_width = 6;
    columns[4].max_width = 6;
    columns[4].align = ITIDY_ALIGN_CENTER;
    columns[4].flexible = FALSE;
    columns[4].is_path = FALSE;
    columns[4].default_sort = ITIDY_SORT_NONE;
    columns[4].sort_type = ITIDY_COLTYPE_NUMBER;
    
    /* Format and display */
    printf("Formatting ListView with %d character width...\n", data->listview_width_chars);
    printf("[DEBUG] create_test_window: About to format %d total entries\n", data->total_rows);
    
    data->display_list = iTidy_FormatListViewColumns(
        columns, 5, &data->entry_list,
        data->listview_width_chars, &data->lv_state,
        ITIDY_MODE_FULL, 0, 1, NULL, 0
    );
    
    if (!data->display_list) {
        printf("[ERROR] create_test_window: iTidy_FormatListViewColumns returned NULL!\n");
        FreeGadgets(data->gadget_list);
        FreeVisualInfo(data->vi);
        whd_free(data);
        return NULL;
    }
    printf("[DEBUG] create_test_window: Format complete, list at 0x%08lx\n", (ULONG)data->display_list);
    
    if (data->display_list) {
        GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                         GTLV_Labels, data->display_list,
                         TAG_DONE);
        printf("[DEBUG] create_test_window: Reattached to gadget\n");
        printf("ListView initialized with %d rows\n", data->total_rows);
        printf("Click column headers to sort!\n");
    }
    
    return data;
}

/**
 * @brief Handle ListView column header click for sorting
 */
static void handle_listview_click(TestWindowData *data, WORD mouse_x, WORD mouse_y)
{
    LONG selected = -1;
    iTidy_ColumnConfig columns[5];
    
    GT_GetGadgetAttrs(data->listview_gad, data->window, NULL,
                     GTLV_Selected, &selected,
                     TAG_END);
    
    if (selected < 0) return;
    
    /* Configure columns (same as creation) */
    columns[0].title = "Date/Time";
    columns[0].min_width = 17;
    columns[0].max_width = 17;
    columns[0].align = ITIDY_ALIGN_LEFT;
    columns[0].flexible = FALSE;
    columns[0].is_path = FALSE;
    columns[0].default_sort = ITIDY_SORT_NONE;
    columns[0].sort_type = ITIDY_COLTYPE_DATE;
    
    columns[1].title = "#";
    columns[1].min_width = 4;
    columns[1].max_width = 6;
    columns[1].align = ITIDY_ALIGN_RIGHT;
    columns[1].flexible = FALSE;
    columns[1].is_path = FALSE;
    columns[1].default_sort = ITIDY_SORT_NONE;
    columns[1].sort_type = ITIDY_COLTYPE_NUMBER;
    
    columns[2].title = "Type";
    columns[2].min_width = 8;
    columns[2].max_width = 8;
    columns[2].align = ITIDY_ALIGN_LEFT;
    columns[2].flexible = FALSE;
    columns[2].is_path = FALSE;
    columns[2].default_sort = ITIDY_SORT_NONE;
    columns[2].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[3].title = "Name";
    columns[3].min_width = 15;
    columns[3].max_width = 200;
    columns[3].align = ITIDY_ALIGN_LEFT;
    columns[3].flexible = TRUE;
    columns[3].is_path = FALSE;
    columns[3].default_sort = ITIDY_SORT_NONE;
    columns[3].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[4].title = "Rating";
    columns[4].min_width = 6;
    columns[4].max_width = 6;
    columns[4].align = ITIDY_ALIGN_CENTER;
    columns[4].flexible = FALSE;
    columns[4].is_path = FALSE;
    columns[4].default_sort = ITIDY_SORT_NONE;
    columns[4].sort_type = ITIDY_COLTYPE_NUMBER;
    
    /* Check if header was clicked and sort */
    if (selected == 0) {  /* Header row */
        int header_top = data->listview_gad->TopEdge;
        int header_height = data->font_height;
        struct timeval start_time, end_time;
        ULONG elapsed_micros;
        float elapsed_seconds;
        
        printf("\n=== SORTING LISTVIEW ===\n");
        printf("Current row count: %d\n", data->total_rows);
        
        /* Set busy pointer */
        SetWindowPointer(data->window, WA_BusyPointer, TRUE, TAG_DONE);
        
        GetSysTime(&start_time);
        
        if (iTidy_ResortListViewByClick(
                data->display_list,
                &data->entry_list,
                data->lv_state,
                mouse_x, mouse_y,
                header_top, header_height,
                data->listview_gad->LeftEdge,
                data->font_width,
                columns)) {
            
            GetSysTime(&end_time);
            elapsed_micros = ((end_time.tv_secs - start_time.tv_secs) * 1000000) +
                             (end_time.tv_micro - start_time.tv_micro);
            elapsed_seconds = (float)elapsed_micros / 1000000.0f;
            
            /* Refresh ListView */
            GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                             GTLV_Labels, ~0,
                             TAG_DONE);
            GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                             GTLV_Labels, data->display_list,
                             GTLV_Top, 0,
                             TAG_DONE);
            
            printf("Sorted %d rows in %.6f seconds\n", data->total_rows, elapsed_seconds);
        }
        
        /* Clear busy pointer */
        SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
    }
}

/**
 * @brief Run baseline test - raw GadTools nodes without API
 */
static void run_baseline_test(TestWindowData *data)
{
    int target_rows;
    int i;
    struct timeval start_time, end_time;
    ULONG elapsed_micros;
    float elapsed_seconds;
    struct Node *node;
    
    printf("\n");
    printf("================================================\n");
    printf("  TEST 0: BASELINE (Raw GadTools - No API)\n");
    printf("  Target: 1000 rows in 50-row increments\n");
    printf("  Mode: Plain struct Node + ln_Name only\n");
    printf("================================================\n\n");
    
    SetWindowPointer(data->window, WA_BusyPointer, TRUE, TAG_DONE);
    
    /* Clear any existing plain nodes from previous tests */
    {
        struct Node *node;
        while ((node = RemHead(&data->plain_list)) != NULL) {
            free_plain_node(node);
        }
    }
    
    /* Start with 50 plain nodes */
    data->total_rows = 0;
    for (i = 0; i < 50; i++) {
        node = (struct Node *)whd_malloc(sizeof(struct Node));
        if (node) {
            memset(node, 0, sizeof(struct Node));
            node->ln_Name = (char *)whd_malloc(32);
            if (node->ln_Name) {
                sprintf(node->ln_Name, "Row %d", i + 1);
                AddTail(&data->plain_list, node);
                data->total_rows++;
            } else {
                whd_free(node);
            }
        }
    }
    
    GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                     GTLV_Labels, &data->plain_list,
                     TAG_DONE);
    
    printf("Initial 50 rows created\n\n");
    
    /* Add in 50-row increments up to 1000 */
    for (target_rows = 100; target_rows <= 1000; target_rows += 50) {
        printf("--- Milestone: %d rows ---\n", target_rows);
        printf("Adding 50 plain nodes...\n");
        
        GetSysTime(&start_time);
        
        /* Detach list */
        GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                         GTLV_Labels, ~0,
                         TAG_DONE);
        
        /* Add 50 plain nodes */
        for (i = 0; i < 50; i++) {
            node = (struct Node *)whd_malloc(sizeof(struct Node));
            if (node) {
                memset(node, 0, sizeof(struct Node));
                node->ln_Name = (char *)whd_malloc(32);
                if (node->ln_Name) {
                    sprintf(node->ln_Name, "Row %d", data->total_rows + i + 1);
                    AddTail(&data->plain_list, node);
                } else {
                    whd_free(node);
                    break;
                }
            } else {
                break;
            }
        }
        
        data->total_rows += 50;
        
        /* Reattach list */
        GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                         GTLV_Labels, &data->plain_list,
                         GTLV_Top, 0,
                         TAG_DONE);
        
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - start_time.tv_secs) * 1000000) +
                         (end_time.tv_micro - start_time.tv_micro);
        elapsed_seconds = (float)elapsed_micros / 1000000.0f;
        
        log_memory_status(data->total_rows);
        
        printf("  [TIMING] Add 50 rows: %.6f seconds (%d total)\n", 
               elapsed_seconds, data->total_rows);
        printf("\n");
    }
    
    SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
    
    printf("================================================\n");
    printf("  TEST 0 COMPLETED - BASELINE\n");
    printf("  Final row count: %d\n", data->total_rows);
    printf("================================================\n");
    
    /* Full teardown with timing */
    teardown_and_time(data, "Baseline Test", TRUE);
}

/**
 * @brief Run simple mode test - no sorting, no pagination
 */
static void run_simple_test(TestWindowData *data)
{
    int target_rows;
    int i;
    struct timeval start_time, end_time;
    ULONG elapsed_micros;
    float elapsed_seconds;
    iTidy_ListViewEntry *entry;
    iTidy_ColumnConfig columns[5];
    
    printf("\n");
    printf("================================================\n");
    printf("  TEST 1: SIMPLE MODE (No Sorting/Pagination)\n");
    printf("  Target: 1000 rows in 50-row increments\n");
    printf("  Mode: ITIDY_MODE_SIMPLE\n");
    printf("================================================\n\n");
    
    SetWindowPointer(data->window, WA_BusyPointer, TRUE, TAG_DONE);
    
    /* Clear any existing entries from previous tests */
    {
        struct Node *node;
        iTidy_ListViewEntry *temp_entry;
        
        /* First pass: NULL out all ln_Name pointers to prevent double-free */
        for (node = data->entry_list.lh_Head; node->ln_Succ; node = node->ln_Succ) {
            temp_entry = (iTidy_ListViewEntry *)node;
            temp_entry->node.ln_Name = NULL;  /* Formatter owns this memory */
        }
        
        /* Second pass: Free entries */
        while ((node = RemHead(&data->entry_list)) != NULL) {
            free_entry((iTidy_ListViewEntry *)node);
        }
    }
    
    /* Configure columns */
    columns[0].title = "Date/Time";
    columns[0].min_width = 17;
    columns[0].max_width = 17;
    columns[0].align = ITIDY_ALIGN_LEFT;
    columns[0].flexible = FALSE;
    columns[0].is_path = FALSE;
    columns[0].default_sort = ITIDY_SORT_NONE;
    columns[0].sort_type = ITIDY_COLTYPE_DATE;
    
    columns[1].title = "#";
    columns[1].min_width = 4;
    columns[1].max_width = 6;
    columns[1].align = ITIDY_ALIGN_RIGHT;
    columns[1].flexible = FALSE;
    columns[1].is_path = FALSE;
    columns[1].default_sort = ITIDY_SORT_NONE;
    columns[1].sort_type = ITIDY_COLTYPE_NUMBER;
    
    columns[2].title = "Type";
    columns[2].min_width = 8;
    columns[2].max_width = 8;
    columns[2].align = ITIDY_ALIGN_LEFT;
    columns[2].flexible = FALSE;
    columns[2].is_path = FALSE;
    columns[2].default_sort = ITIDY_SORT_NONE;
    columns[2].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[3].title = "Name";
    columns[3].min_width = 15;
    columns[3].max_width = 200;
    columns[3].align = ITIDY_ALIGN_LEFT;
    columns[3].flexible = TRUE;
    columns[3].is_path = FALSE;
    columns[3].default_sort = ITIDY_SORT_NONE;
    columns[3].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[4].title = "Rating";
    columns[4].min_width = 6;
    columns[4].max_width = 6;
    columns[4].align = ITIDY_ALIGN_CENTER;
    columns[4].flexible = FALSE;
    columns[4].is_path = FALSE;
    columns[4].default_sort = ITIDY_SORT_NONE;
    columns[4].sort_type = ITIDY_COLTYPE_NUMBER;
    
    /* Start with 50 entries */
    data->total_rows = 0;
    for (i = 0; i < 50; i++) {
        entry = create_test_entry(i);
        if (entry) {
            AddTail(&data->entry_list, (struct Node *)entry);
            data->total_rows++;
        }
    }
    
    data->display_list = iTidy_FormatListViewColumns(
        columns, 5, &data->entry_list,
        data->listview_width_chars, NULL,
        ITIDY_MODE_SIMPLE, 0, 1, NULL, 0
    );
    
    GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                     GTLV_Labels, data->display_list,
                     TAG_DONE);
    
    printf("Initial 50 rows created\n\n");
    
    /* Add in 50-row increments up to 1000 */
    for (target_rows = 100; target_rows <= 1000; target_rows += 50) {
        printf("--- Milestone: %d rows ---\n", target_rows);
        printf("Adding 50 API entries...\n");
        
        GetSysTime(&start_time);
        
        /* Detach list */
        GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                         GTLV_Labels, ~0,
                         TAG_DONE);
        
        /* Add 50 entries */
        for (i = 0; i < 50; i++) {
            entry = create_test_entry(data->total_rows + i);
            if (entry) {
                AddTail(&data->entry_list, (struct Node *)entry);
            } else {
                break;
            }
        }
        
        data->total_rows += 50;
        
        /* Free old display list */
        if (data->display_list) {
            iTidy_FreeFormattedList(data->display_list);
            data->display_list = NULL;
        }
        
        /* Reformat with SIMPLE mode */
        data->display_list = iTidy_FormatListViewColumns(
            columns, 5, &data->entry_list,
            data->listview_width_chars, NULL,
            ITIDY_MODE_SIMPLE, 0, 1, NULL, 0
        );
        
        /* Reattach */
        GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                         GTLV_Labels, data->display_list,
                         GTLV_Top, 0,
                         TAG_DONE);
        
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - start_time.tv_secs) * 1000000) +
                         (end_time.tv_micro - start_time.tv_micro);
        elapsed_seconds = (float)elapsed_micros / 1000000.0f;
        
        log_memory_status(data->total_rows);
        
        printf("  [TIMING] Add + Format: %.6f seconds (%d total)\n", 
               elapsed_seconds, data->total_rows);
        printf("\n");
    }
    
    SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
    
    printf("================================================\n");
    printf("  TEST 1 COMPLETED - SIMPLE MODE\n");
    printf("  Final row count: %d\n", data->total_rows);
    printf("================================================\n");
    
    /* Full teardown with timing */
    teardown_and_time(data, "Simple Mode Test", FALSE);
}

/**
 * @brief Run paginated mode test - with page navigation
 */
static void run_paginated_test(TestWindowData *data)
{
    int target_rows;
    int i;
    struct timeval start_time, end_time, nav_start, nav_end;
    ULONG elapsed_micros;
    float elapsed_seconds;
    iTidy_ListViewEntry *entry;
    iTidy_ColumnConfig columns[5];
    int current_page = 1;
    int total_pages = 1;
    
    printf("\n");
    printf("================================================\n");
    printf("  TEST 2: PAGINATED MODE\n");
    printf("  Target: 1000 rows in 50-row increments\n");
    printf("  Mode: ITIDY_MODE_SIMPLE_PAGINATED (page_size=100)\n");
    printf("================================================\n\n");
    
    SetWindowPointer(data->window, WA_BusyPointer, TRUE, TAG_DONE);
    
    /* Clear any existing entries from previous tests */
    {
        struct Node *node;
        iTidy_ListViewEntry *temp_entry;
        
        /* First pass: NULL out all ln_Name pointers to prevent double-free */
        for (node = data->entry_list.lh_Head; node->ln_Succ; node = node->ln_Succ) {
            temp_entry = (iTidy_ListViewEntry *)node;
            temp_entry->node.ln_Name = NULL;  /* Formatter owns this memory */
        }
        
        /* Second pass: Free entries */
        while ((node = RemHead(&data->entry_list)) != NULL) {
            free_entry((iTidy_ListViewEntry *)node);
        }
    }
    
    /* Configure columns (same as simple test) */
    columns[0].title = "Date/Time";
    columns[0].min_width = 17;
    columns[0].max_width = 17;
    columns[0].align = ITIDY_ALIGN_LEFT;
    columns[0].flexible = FALSE;
    columns[0].is_path = FALSE;
    columns[0].default_sort = ITIDY_SORT_NONE;
    columns[0].sort_type = ITIDY_COLTYPE_DATE;
    
    columns[1].title = "#";
    columns[1].min_width = 4;
    columns[1].max_width = 6;
    columns[1].align = ITIDY_ALIGN_RIGHT;
    columns[1].flexible = FALSE;
    columns[1].is_path = FALSE;
    columns[1].default_sort = ITIDY_SORT_NONE;
    columns[1].sort_type = ITIDY_COLTYPE_NUMBER;
    
    columns[2].title = "Type";
    columns[2].min_width = 8;
    columns[2].max_width = 8;
    columns[2].align = ITIDY_ALIGN_LEFT;
    columns[2].flexible = FALSE;
    columns[2].is_path = FALSE;
    columns[2].default_sort = ITIDY_SORT_NONE;
    columns[2].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[3].title = "Name";
    columns[3].min_width = 15;
    columns[3].max_width = 200;
    columns[3].align = ITIDY_ALIGN_LEFT;
    columns[3].flexible = TRUE;
    columns[3].is_path = FALSE;
    columns[3].default_sort = ITIDY_SORT_NONE;
    columns[3].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[4].title = "Rating";
    columns[4].min_width = 6;
    columns[4].max_width = 6;
    columns[4].align = ITIDY_ALIGN_CENTER;
    columns[4].flexible = FALSE;
    columns[4].is_path = FALSE;
    columns[4].default_sort = ITIDY_SORT_NONE;
    columns[4].sort_type = ITIDY_COLTYPE_NUMBER;
    
    /* Start with 50 entries */
    data->total_rows = 0;
    for (i = 0; i < 50; i++) {
        entry = create_test_entry(i);
        if (entry) {
            AddTail(&data->entry_list, (struct Node *)entry);
            data->total_rows++;
        }
    }
    
    data->display_list = iTidy_FormatListViewColumns(
        columns, 5, &data->entry_list,
        data->listview_width_chars, &data->lv_state,
        ITIDY_MODE_SIMPLE_PAGINATED, 100, 1, &total_pages, 0
    );
    
    GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                     GTLV_Labels, data->display_list,
                     TAG_DONE);
    
    printf("Initial 50 rows created (page 1 of %d)\n\n", total_pages);
    
    /* Add in 50-row increments up to 1000 */
    for (target_rows = 100; target_rows <= 1000; target_rows += 50) {
        printf("--- Milestone: %d rows ---\n", target_rows);
        printf("Adding 50 API entries...\n");
        
        GetSysTime(&start_time);
        
        /* Detach list */
        GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                         GTLV_Labels, ~0,
                         TAG_DONE);
        
        /* Add 50 entries */
        for (i = 0; i < 50; i++) {
            entry = create_test_entry(data->total_rows + i);
            if (entry) {
                AddTail(&data->entry_list, (struct Node *)entry);
            } else {
                break;
            }
        }
        
        data->total_rows += 50;
        
        /* Free old display list and state */
        if (data->display_list) {
            iTidy_FreeFormattedList(data->display_list);
            data->display_list = NULL;
        }
        if (data->lv_state) {
            iTidy_FreeListViewState(data->lv_state);
            data->lv_state = NULL;
        }
        
        /* Reformat with SIMPLE_PAGINATED mode */
        data->display_list = iTidy_FormatListViewColumns(
            columns, 5, &data->entry_list,
            data->listview_width_chars, &data->lv_state,
            ITIDY_MODE_SIMPLE_PAGINATED, 100, current_page, &total_pages, 0
        );
        
        /* Reattach */
        GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                         GTLV_Labels, data->display_list,
                         GTLV_Top, 0,
                         TAG_DONE);
        
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - start_time.tv_secs) * 1000000) +
                         (end_time.tv_micro - start_time.tv_micro);
        elapsed_seconds = (float)elapsed_micros / 1000000.0f;
        
        printf("  [TIMING] Add + Format: %.6f seconds (page %d of %d)\n", 
               elapsed_seconds, current_page, total_pages);
        
        /* Test page navigation if more pages available */
        if (data->lv_state && current_page < total_pages) {
            printf("Testing page navigation (page %d -> %d)...\n", current_page, current_page + 1);
            
            GetSysTime(&nav_start);
            
            /* Detach */
            GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                             GTLV_Labels, ~0,
                             TAG_DONE);
            
            /* Free old display */
            if (data->display_list) {
                iTidy_FreeFormattedList(data->display_list);
                data->display_list = NULL;
            }
            if (data->lv_state) {
                iTidy_FreeListViewState(data->lv_state);
                data->lv_state = NULL;
            }
            
            /* Navigate to next page */
            current_page++;
            data->display_list = iTidy_FormatListViewColumns(
                columns, 5, &data->entry_list,
                data->listview_width_chars, &data->lv_state,
                ITIDY_MODE_SIMPLE_PAGINATED, 100, current_page, &total_pages, 1
            );
            
            /* Reattach */
            GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                             GTLV_Labels, data->display_list,
                             TAG_DONE);
            
            GetSysTime(&nav_end);
            elapsed_micros = ((nav_end.tv_secs - nav_start.tv_secs) * 1000000) +
                             (nav_end.tv_micro - nav_start.tv_micro);
            
            printf("  [TIMING] Page navigation: %.6f seconds\n", 
                   (float)elapsed_micros / 1000000.0f);
        }
        
        log_memory_status(data->total_rows);
        printf("\n");
    }
    
    SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
    
    printf("================================================\n");
    printf("  TEST 2 COMPLETED - PAGINATED MODE\n");
    printf("  Final row count: %d\n", data->total_rows);
    printf("  Total pages: %d\n", total_pages);
    printf("================================================\n");
    
    /* Full teardown with timing */
    teardown_and_time(data, "Paginated Mode Test", FALSE);
}

/**
 * @brief Run automated benchmark sequence
 * Adds rows in 50-row increments up to 1000, sorting column 0 after each addition
 */
static void run_automated_benchmark(TestWindowData *data)
{
    int target_rows;
    int current_count;
    struct timeval start_time, end_time;
    ULONG elapsed_micros;
    float elapsed_seconds;
    iTidy_ColumnConfig columns[5];
    
    printf("\n");
    printf("================================================\n");
    printf("  AUTOMATED BENCHMARK SEQUENCE STARTED\n");
    printf("  Target: 1000 rows in 50-row increments\n");
    printf("  Sorting column 0 (Date) after each addition\n");
    printf("================================================\n\n");
    
    /* Set busy pointer for entire sequence */
    SetWindowPointer(data->window, WA_BusyPointer, TRUE, TAG_DONE);
    
    /* Clear any existing entries from previous tests */
    {
        struct Node *node;
        iTidy_ListViewEntry *temp_entry;
        
        /* First pass: NULL out all ln_Name pointers to prevent double-free */
        for (node = data->entry_list.lh_Head; node->ln_Succ; node = node->ln_Succ) {
            temp_entry = (iTidy_ListViewEntry *)node;
            temp_entry->node.ln_Name = NULL;  /* Formatter owns this memory */
        }
        
        /* Second pass: Free entries */
        while ((node = RemHead(&data->entry_list)) != NULL) {
            free_entry((iTidy_ListViewEntry *)node);
        }
    }
    
    /* Configure columns for sorting (same as manual click handler) */
    columns[0].title = "Date/Time";
    columns[0].min_width = 17;
    columns[0].max_width = 17;
    columns[0].align = ITIDY_ALIGN_LEFT;
    columns[0].flexible = FALSE;
    columns[0].is_path = FALSE;
    columns[0].default_sort = ITIDY_SORT_DESCENDING;
    columns[0].sort_type = ITIDY_COLTYPE_DATE;
    
    columns[1].title = "#";
    columns[1].min_width = 4;
    columns[1].max_width = 6;
    columns[1].align = ITIDY_ALIGN_RIGHT;
    columns[1].flexible = FALSE;
    columns[1].is_path = FALSE;
    columns[1].default_sort = ITIDY_SORT_ASCENDING;
    columns[1].sort_type = ITIDY_COLTYPE_NUMBER;
    
    columns[2].title = "Type";
    columns[2].min_width = 8;
    columns[2].max_width = 8;
    columns[2].align = ITIDY_ALIGN_LEFT;
    columns[2].flexible = FALSE;
    columns[2].is_path = FALSE;
    columns[2].default_sort = ITIDY_SORT_ASCENDING;
    columns[2].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[3].title = "Name";
    columns[3].min_width = 15;
    columns[3].max_width = 200;
    columns[3].align = ITIDY_ALIGN_LEFT;
    columns[3].flexible = TRUE;
    columns[3].is_path = FALSE;
    columns[3].default_sort = ITIDY_SORT_NONE;
    columns[3].sort_type = ITIDY_COLTYPE_TEXT;
    
    columns[4].title = "Rating";
    columns[4].min_width = 6;
    columns[4].max_width = 6;
    columns[4].align = ITIDY_ALIGN_CENTER;
    columns[4].flexible = FALSE;
    columns[4].is_path = FALSE;
    columns[4].default_sort = ITIDY_SORT_NONE;
    columns[4].sort_type = ITIDY_COLTYPE_NUMBER;
    
    /* Run benchmark from current count to 1000 rows */
    for (target_rows = 100; target_rows <= 1000; target_rows += 50) {
        current_count = data->total_rows;
        
        /* Skip if already at or past this milestone */
        if (current_count >= target_rows) {
            continue;
        }
        
        printf("--- Milestone: %d rows ---\n", target_rows);
        
        /* Add 50 rows */
        printf("Adding 50 rows (current: %d)...\n", current_count);
        GetSysTime(&start_time);
        
        add_50_rows(data);  /* This already has timing and updates display */
        
        /* Sort by column 0 (Date) */
        printf("Sorting %d rows by Date/Time (column 0)...\n", data->total_rows);
        GetSysTime(&start_time);
        
        /* Sort using the API function - column 0, DATE type, descending */
        iTidy_SortListViewEntries(&data->entry_list, 0, ITIDY_COLTYPE_DATE, FALSE);
        
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - start_time.tv_secs) * 1000000) +
                         (end_time.tv_micro - start_time.tv_micro);
        elapsed_seconds = (float)elapsed_micros / 1000000.0f;
        
        /* Rebuild formatted list after sorting */
        if (data->display_list) {
            iTidy_FreeFormattedList(data->display_list);
            data->display_list = NULL;
        }
        if (data->lv_state) {
            iTidy_FreeListViewState(data->lv_state);
            data->lv_state = NULL;
        }
        
        printf("[DEBUG] run_automated_benchmark: About to format %d total entries\n", data->total_rows);
        
        data->display_list = iTidy_FormatListViewColumns(
            columns, 5,
            &data->entry_list,
            data->listview_width_chars,
            &data->lv_state,
            ITIDY_MODE_FULL, 0, 1, NULL, 0);
        
        if (!data->display_list) {
            printf("[ERROR] run_automated_benchmark: iTidy_FormatListViewColumns returned NULL!\n");
            SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
            return;
        }
        printf("[DEBUG] run_automated_benchmark: Format complete, list at 0x%08lx\n", (ULONG)data->display_list);
        
        /* Log memory status after this iteration */
        log_memory_status(data->total_rows);
        
        /* Refresh ListView to show sorted data */
        GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                         GTLV_Labels, ~0,
                         TAG_DONE);
        GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                         GTLV_Labels, data->display_list,
                         GTLV_Top, 0,
                         TAG_DONE);
        
        printf("[DEBUG] run_automated_benchmark: Reattached to gadget\n");
        
        printf("  [TIMING] Sort completed in %.6f seconds (%d rows)\n", 
               elapsed_seconds, data->total_rows);
        printf("\n");
    }
    
    /* Clear busy pointer */
    SetWindowPointer(data->window, WA_BusyPointer, FALSE, TAG_DONE);
    
    printf("================================================\n");
    printf("  TEST 3 COMPLETED - FULL MODE (Auto Run)\n");
    printf("  Final row count: %d\n", data->total_rows);
    printf("  Check logs for detailed timing breakdown\n");
    printf("================================================\n");
    
    /* Full teardown with timing */
    teardown_and_time(data, "Full Mode Test (Auto Run)", FALSE);
}

/**
 * @brief Run all tests sequentially
 */
static void run_all_tests(TestWindowData *data)
{
    printf("\n");
    printf("====================================================\n");
    printf("  COMPREHENSIVE BENCHMARK SUITE\n");
    printf("  Running all 4 tests sequentially\n");
    printf("  Each test: 50 -> 1000 rows (50-row increments)\n");
    printf("====================================================\n");
    
    run_baseline_test(data);
    run_simple_test(data);
    run_paginated_test(data);
    run_automated_benchmark(data);  /* Existing Auto Run test */
    
    printf("\n");
    printf("====================================================\n");
    printf("  ALL TESTS COMPLETED\n");
    printf("  Check logs for detailed performance breakdown\n");
    printf("====================================================\n\n");
}

/**
 * @brief Main event loop
 */
static void run_event_loop(TestWindowData *data)
{
    struct IntuiMessage *msg;
    BOOL done = FALSE;
    ULONG class;
    UWORD code;
    struct Gadget *gadget;
    WORD mouse_x, mouse_y;
    
    printf("\n=== EVENT LOOP STARTED ===\n");
    printf("Instructions:\n");
    printf("- Click 'Add 50' / 'Remove 50' to manually add/remove data\n");
    printf("- Click 'Baseline' to run Test 0 (raw GadTools, no API)\n");
    printf("- Click 'Simple' to run Test 1 (SIMPLE mode, no sorting)\n");
    printf("- Click 'Paginated' to run Test 2 (SIMPLE_PAGINATED mode)\n");
    printf("- Click 'Auto Run' to run Test 3 (FULL mode with sorting)\n");
    printf("- Click 'Run All Tests' to run all 4 tests sequentially\n");
    printf("- Click column headers to sort\n");
    printf("- Watch console for timing statistics\n");
    printf("- Close window to exit\n\n");
    
    while (!done) {
        WaitPort(data->window->UserPort);
        
        while ((msg = GT_GetIMsg(data->window->UserPort))) {
            class = msg->Class;
            code = msg->Code;
            gadget = (struct Gadget *)msg->IAddress;
            mouse_x = msg->MouseX;
            mouse_y = msg->MouseY;
            
            GT_ReplyIMsg(msg);
            
            switch (class) {
                case IDCMP_CLOSEWINDOW:
                    done = TRUE;
                    break;
                
                case IDCMP_GADGETUP:
                    if (gadget->GadgetID == GID_ADD_BTN) {
                        add_50_rows(data);
                    } else if (gadget->GadgetID == GID_REMOVE_BTN) {
                        remove_50_rows(data);
                    } else if (gadget->GadgetID == GID_BASELINE_BTN) {
                        run_baseline_test(data);
                    } else if (gadget->GadgetID == GID_SIMPLE_BTN) {
                        run_simple_test(data);
                    } else if (gadget->GadgetID == GID_PAGINATED_BTN) {
                        run_paginated_test(data);
                    } else if (gadget->GadgetID == GID_AUTORUN_BTN) {
                        run_automated_benchmark(data);
                    } else if (gadget->GadgetID == GID_RUNALL_BTN) {
                        run_all_tests(data);
                    } else if (gadget->GadgetID == GID_LISTVIEW) {
                        handle_listview_click(data, mouse_x, mouse_y);
                    }
                    break;
                
                case IDCMP_GADGETDOWN:
                    /* Required for ListView scroll buttons */
                    break;
            }
        }
    }
    
    printf("\n=== EXITING ===\n");
}

/**
 * @brief Cleanup and close window
 */
static void cleanup_test_window(TestWindowData *data)
{
    struct Node *node;
    struct timeval start_time, end_time, step_start;
    ULONG elapsed_micros;
    float elapsed_seconds;
    int entry_count = 0;
    
    if (!data) return;
    
    printf("\n=== CLEANUP TIMING ===\n");
    GetSysTime(&start_time);
    step_start = start_time;
    
    /* Detach list from gadget */
    if (data->listview_gad && data->window) {
        GT_SetGadgetAttrs(data->listview_gad, data->window, NULL,
                         GTLV_Labels, ~0,
                         TAG_DONE);
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - step_start.tv_secs) * 1000000) +
                         (end_time.tv_micro - step_start.tv_micro);
        printf("Detach ListView: %.6f seconds\n", (float)elapsed_micros / 1000000.0f);
        step_start = end_time;
    }
    
    /* Close window */
    if (data->window) {
        CloseWindow(data->window);
        data->window = NULL;
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - step_start.tv_secs) * 1000000) +
                         (end_time.tv_micro - step_start.tv_micro);
        printf("Close window: %.6f seconds\n", (float)elapsed_micros / 1000000.0f);
        step_start = end_time;
    }
    
    /* Free gadgets */
    if (data->gadget_list) {
        FreeGadgets(data->gadget_list);
        data->gadget_list = NULL;
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - step_start.tv_secs) * 1000000) +
                         (end_time.tv_micro - step_start.tv_micro);
        printf("Free gadgets: %.6f seconds\n", (float)elapsed_micros / 1000000.0f);
        step_start = end_time;
    }
    
    /* Free visual info */
    if (data->vi) {
        FreeVisualInfo(data->vi);
        data->vi = NULL;
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - step_start.tv_secs) * 1000000) +
                         (end_time.tv_micro - step_start.tv_micro);
        printf("Free visual info: %.6f seconds\n", (float)elapsed_micros / 1000000.0f);
        step_start = end_time;
    }
    
    /* Free display list and state */
    if (data->display_list) {
        iTidy_FreeFormattedList(data->display_list);
        data->display_list = NULL;
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - step_start.tv_secs) * 1000000) +
                         (end_time.tv_micro - step_start.tv_micro);
        printf("Free formatted list: %.6f seconds\n", (float)elapsed_micros / 1000000.0f);
        step_start = end_time;
    }
    if (data->lv_state) {
        iTidy_FreeListViewState(data->lv_state);
        data->lv_state = NULL;
        GetSysTime(&end_time);
        elapsed_micros = ((end_time.tv_secs - step_start.tv_secs) * 1000000) +
                         (end_time.tv_micro - step_start.tv_micro);
        printf("Free ListView state: %.6f seconds\n", (float)elapsed_micros / 1000000.0f);
        step_start = end_time;
    }
    
    /* Free entry list */
    if (data->total_rows > 0) {
        printf("Freeing %d entries (logging suspended for speed)...\n", data->total_rows);
        whd_memory_suspend_logging();  /* Disable logging during bulk free */
        
        while ((node = RemHead(&data->entry_list)) != NULL) {
            free_entry((iTidy_ListViewEntry *)node);
            entry_count++;
        }
        
        whd_memory_resume_logging();  /* Re-enable logging */
    }
    
    /* Free plain list (baseline test) */
    while ((node = RemHead(&data->plain_list)) != NULL) {
        free_plain_node(node);
    }
    
    GetSysTime(&end_time);
    elapsed_micros = ((end_time.tv_secs - step_start.tv_secs) * 1000000) +
                     (end_time.tv_micro - step_start.tv_micro);
    printf("Free %d entries: %.6f seconds\n", entry_count, (float)elapsed_micros / 1000000.0f);
    if (entry_count > 0) {
        printf("  Time per entry: %.6f seconds\n", (float)elapsed_micros / (float)entry_count / 1000000.0f);
    }
    step_start = end_time;
    
    whd_free(data);
    GetSysTime(&end_time);
    elapsed_micros = ((end_time.tv_secs - step_start.tv_secs) * 1000000) +
                     (end_time.tv_micro - step_start.tv_micro);
    printf("Free window data: %.6f seconds\n", (float)elapsed_micros / 1000000.0f);
    
    /* Total time */
    elapsed_micros = ((end_time.tv_secs - start_time.tv_secs) * 1000000) +
                     (end_time.tv_micro - start_time.tv_micro);
    elapsed_seconds = (float)elapsed_micros / 1000000.0f;
    printf("TOTAL CLEANUP TIME: %.6f seconds\n", elapsed_seconds);
}

/**
 * @brief Get CPU name from SysBase->AttnFlags
 * @return Constant string with CPU name
 */
static const char *get_cpu_name(void)
{
    UWORD flags = SysBase->AttnFlags;
    
    if (flags & AFF_68060) return "MC68060";
    if (flags & AFF_68040) return "MC68040";
    if (flags & AFF_68030) return "MC68030";
    if (flags & AFF_68020) return "MC68020";
    if (flags & AFF_68010) return "MC68010";
    return "MC68000";
}

/**
 * @brief Log current memory status (lightweight version for benchmarks)
 */
static void log_memory_status(int entry_count)
{
    ULONG chip_total, chip_largest;
    ULONG fast_total;
    
    chip_total = AvailMem(MEMF_CHIP);
    chip_largest = AvailMem(MEMF_CHIP | MEMF_LARGEST);
    fast_total = AvailMem(MEMF_FAST);
    
    log_info(LOG_GUI, "[MEMORY] After %d entries: Chip=%lu KB free (largest=%lu KB), Fast=%lu KB",
             entry_count,
             chip_total / 1024,
             chip_largest / 1024,
             fast_total / 1024);
}

/**
 * @brief Log CPU and memory information at startup
 */
static void log_system_info(void)
{
    ULONG chip_total, chip_largest;
    ULONG fast_total, fast_largest;
    const char *cpu_name;
    
    /* Get CPU type */
    cpu_name = get_cpu_name();
    
    /* Get memory info */
    chip_total = AvailMem(MEMF_CHIP);
    chip_largest = AvailMem(MEMF_CHIP | MEMF_LARGEST);
    fast_total = AvailMem(MEMF_FAST);
    fast_largest = AvailMem(MEMF_FAST | MEMF_LARGEST);
    
    /* Log system information */
    log_info(LOG_GUI, "=== System Information (Stress Test) ===");
    log_info(LOG_GUI, "CPU: %s", cpu_name);
    log_info(LOG_GUI, "Chip RAM: %lu KB free (%lu bytes total, %lu bytes largest block)",
             chip_total / 1024, chip_total, chip_largest);
    
    if (fast_total > 0)
    {
        log_info(LOG_GUI, "Fast RAM: %lu KB free (%lu bytes total, %lu bytes largest block)",
                 fast_total / 1024, fast_total, fast_largest);
    }
    else
    {
        log_info(LOG_GUI, "Fast RAM: None detected");
    }
    
    log_info(LOG_GUI, "========================================");
}

/**
 * @brief Initialize font metrics with safe defaults
 */
static void init_font_defaults(void)
{
    struct Screen *screen;
    struct TextFont *font;
    
    /* Initialize entire structure to zero first */
    memset(&prefsIControl, 0, sizeof(struct IControlPrefsDetails));
    
    /* Set safe defaults first (Topaz 8) */
    prefsIControl.systemFontCharWidth = 8;
    prefsIControl.systemFontSize = 8;
    strcpy(prefsIControl.systemFontName, "topaz.font");
    strcpy(prefsIControl.iconTextFontName, "topaz.font");
    prefsIControl.iconTextFontSize = 8;
    prefsIControl.iconTextFontCharWidth = 8;
    
    /* Set window border defaults */
    prefsIControl.currentBarWidth = 18;
    prefsIControl.currentBarHeight = 10;
    prefsIControl.currentTitleBarHeight = 16;
    prefsIControl.currentWindowBarHeight = 16;
    prefsIControl.currentLeftBarWidth = 4;
    
    /* Try to get actual Workbench screen font */
    screen = LockPubScreen("Workbench");
    if (screen) {
        font = screen->Font;
        if (font && font->tf_XSize > 0 && font->tf_YSize > 0) {
            /* Only use screen font if values are sane */
            if (font->tf_XSize >= 4 && font->tf_XSize <= 16 &&
                font->tf_YSize >= 6 && font->tf_YSize <= 24) {
                prefsIControl.systemFontCharWidth = font->tf_XSize;
                prefsIControl.systemFontSize = font->tf_YSize;
                
                /* Try to get font name safely */
                if (font->tf_Message.mn_Node.ln_Name) {
                    strncpy(prefsIControl.systemFontName, 
                           font->tf_Message.mn_Node.ln_Name, 
                           sizeof(prefsIControl.systemFontName) - 1);
                    prefsIControl.systemFontName[sizeof(prefsIControl.systemFontName) - 1] = '\0';
                }
            }
        }
        UnlockPubScreen(NULL, screen);
    }
    
    /* Final sanity check - if anything is still zero or insane, reset to Topaz 8 */
    if (prefsIControl.systemFontCharWidth < 4 || prefsIControl.systemFontCharWidth > 16) {
        prefsIControl.systemFontCharWidth = 8;
    }
    if (prefsIControl.systemFontSize < 6 || prefsIControl.systemFontSize > 24) {
        prefsIControl.systemFontSize = 8;
    }
    if (prefsIControl.systemFontName[0] == '\0') {
        strcpy(prefsIControl.systemFontName, "topaz.font");
    }
}

/**
 * @brief Main entry point
 */
int main(void)
{
    TestWindowData *data;
    
    printf("\n");
    printf("========================================\n");
    printf("  ListView Stress Test - iTidy v2.0\n");
    printf("  Performance Benchmark for 7MHz Amiga\n");
    printf("========================================\n");
    
    /* Initialize logging system */
    printf("Initializing logging system...\n");
    initialize_log_system(FALSE);  /* Don't clean old logs */
    
    /* Initialize memory tracking */
    whd_memory_init();
    
    /* Disable memory logging to avoid I/O overhead during operations */
    /* (Tracking still works - allocations are counted, just not logged to file) */
    set_memory_logging_enabled(FALSE);
    printf("Memory logging: DISABLED (tracking only)\n");
    
    /* Enable performance logging for ListView API timing */
    set_performance_logging_enabled(TRUE);
    printf("Performance logging: ENABLED (check PROGDIR:logs/gui_*.log)\n");
    
    /* Log system information (CPU and memory) */
    log_system_info();
    printf("\n");
    
    /* Initialize font metrics with safe defaults from Workbench screen */
    init_font_defaults();
    
    printf("Font metrics: %d x %d pixels (%s)\n",
           prefsIControl.systemFontCharWidth,
           prefsIControl.systemFontSize,
           prefsIControl.systemFontName);
    
    /* Open timer device */
    if (!open_timer_device()) {
        printf("WARNING: Could not open timer.device - no timing available\n");
    }
    
    /* Create window */
    data = create_test_window();
    if (!data) {
        printf("ERROR: Failed to create test window\n");
        close_timer_device();
        whd_memory_report();
        return 1;
    }
    
    /* Run event loop */
    run_event_loop(data);
    
    /* Cleanup */
    printf("Cleaning up...\n");
    cleanup_test_window(data);
    
    close_timer_device();
    
    /* Shutdown logging system */
    printf("Shutting down logging system...\n");
    shutdown_log_system();
    
    /* Memory report */
    printf("\n=== MEMORY REPORT ===\n");
    whd_memory_report();
    
    printf("\nTest completed successfully.\n");
    return 0;
}
