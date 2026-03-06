/*
 * backdrop_window.c - iTidy Workbench Screen Manager Window (ReAction)
 * ReAction GUI for Workbench 3.2+
 *
 * Scans all mounted volumes for .backdrop entries, validates them,
 * presents results in a ListBrowser, and provides orphan removal
 * and icon tidy-layout tools.
 *
 * Follows the library-base-isolation pattern used by restore_window.c.
 */

/* =========================================================================
 * LIBRARY BASE ISOLATION
 * Redefine library bases to local unique names BEFORE including proto headers.
 * This prevents linker collisions with bases defined in main_window.c
 * ========================================================================= */
#define WindowBase      iTidy_Backdrop_WindowBase
#define LayoutBase      iTidy_Backdrop_LayoutBase
#define ButtonBase      iTidy_Backdrop_ButtonBase
#define ListBrowserBase iTidy_Backdrop_ListBrowserBase
#define LabelBase       iTidy_Backdrop_LabelBase
#define RequesterBase   iTidy_Backdrop_RequesterBase

#include "platform/platform.h"
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <dos/dostags.h>
#include <proto/utility.h>
#include <string.h>
#include <stdio.h>

/* ReAction headers */
#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
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

/* Console output abstraction */
#include <console_output.h>

/* GadTools for menus - use global base auto-opened by -lauto */
#include <libraries/gadtools.h>
#include <proto/gadtools.h>

/* ASL file requester */
#include <libraries/asl.h>
#include <proto/asl.h>

#include "backdrop_window.h"
#include "utilities.h"
#include "DOS/device_scanner.h"
#include "backups/backdrop_parser.h"
#include "layout/workbench_layout.h"
#include "writeLog.h"
#include "GUI/gui_utilities.h"

#ifndef NewList
VOID NewList(struct List *list);
#endif

/*------------------------------------------------------------------------*/
/* Library Bases (prefixed to avoid collision with main_window.c)        */
/*------------------------------------------------------------------------*/
struct Library *iTidy_Backdrop_WindowBase      = NULL;
struct Library *iTidy_Backdrop_LayoutBase      = NULL;
struct Library *iTidy_Backdrop_ButtonBase      = NULL;
struct Library *iTidy_Backdrop_ListBrowserBase = NULL;
struct Library *iTidy_Backdrop_LabelBase       = NULL;
struct Library *iTidy_Backdrop_RequesterBase   = NULL;

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define BACKDROP_WINDOW_TITLE "iTidy - Workbench Screen Manager"

/*------------------------------------------------------------------------*/
/* Menu Item IDs                                                          */
/*------------------------------------------------------------------------*/
#define MENU_BD_PROJECT_SAVE_AS  1

/*------------------------------------------------------------------------*/
/* Menu Template                                                          */
/*------------------------------------------------------------------------*/
static struct NewMenu backdrop_menu_template[] =
{
    { NM_TITLE, "Project",         NULL, 0, 0, NULL },
    { NM_ITEM,  "Save list as...", NULL, 0, 0, (APTR)MENU_BD_PROJECT_SAVE_AS },
    { NM_END,   NULL,              NULL, 0, 0, NULL }
};

/*------------------------------------------------------------------------*/
/* Gadget IDs                                                             */
/*------------------------------------------------------------------------*/
enum {
    GID_BD_ROOT_LAYOUT = 1,
    GID_BD_LISTBROWSER,
    GID_BD_STATUS_LAYOUT,
    GID_BD_STATUS_LABEL,
    GID_BD_BUTTONS_LAYOUT,
    GID_BD_SCAN_BUTTON,
    GID_BD_REMOVE_BUTTON,
    GID_BD_TIDY_BUTTON,
    GID_BD_TEST_BUTTON,
    GID_BD_CLOSE_BUTTON
};

/*------------------------------------------------------------------------*/
/* ListBrowser Column Indices                                             */
/*------------------------------------------------------------------------*/
#define COL_NAME     0
#define COL_STATUS   1
#define COL_DEVICE   2
#define COL_PATH     3
#define COL_TYPE     4
#define COL_POS_X    5
#define COL_POS_Y    6
#define NUM_COLUMNS  7

/* NO_ICON_POSITION: value used by Workbench when icon has no saved position */
#define NO_ICON_POSITION ((LONG)0x80000000)

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static BOOL open_reaction_classes(void);
static void close_reaction_classes(void);
static BOOL init_backdrop_column_info(void);
static void free_listbrowser_list(struct List *list);
static void populate_listbrowser(struct iTidyBackdropWindow *bd_data);
static void update_status_label(struct iTidyBackdropWindow *bd_data,
                                 const char *text);
static void perform_scan(struct iTidyBackdropWindow *bd_data);
static void perform_remove_selected(struct iTidyBackdropWindow *bd_data);
static void perform_tidy_layout(struct iTidyBackdropWindow *bd_data);
static void perform_save_list_as(struct iTidyBackdropWindow *bd_data);
static ULONG show_requester(struct Window *parent_window,
                             CONST_STRPTR title,
                             CONST_STRPTR body,
                             CONST_STRPTR gadgets,
                             ULONG image_type);

/*------------------------------------------------------------------------*/
/* ReAction Class Management                                              */
/*------------------------------------------------------------------------*/

static BOOL open_reaction_classes(void)
{
    if (!WindowBase)      WindowBase      = OpenLibrary("window.class", 0);
    if (!LayoutBase)      LayoutBase      = OpenLibrary("gadgets/layout.gadget", 0);
    if (!ButtonBase)      ButtonBase      = OpenLibrary("gadgets/button.gadget", 0);
    if (!ListBrowserBase) ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 0);
    if (!LabelBase)       LabelBase       = OpenLibrary("images/label.image", 0);
    if (!RequesterBase)   RequesterBase   = OpenLibrary("requester.class", 0);

    if (!WindowBase || !LayoutBase || !ButtonBase ||
        !ListBrowserBase || !LabelBase || !RequesterBase)
    {
        log_error(LOG_GUI, "Failed to open ReAction classes for backdrop window\n");
        return FALSE;
    }

    return TRUE;
}

static void close_reaction_classes(void)
{
    if (RequesterBase)   { CloseLibrary(RequesterBase);   RequesterBase   = NULL; }
    if (LabelBase)       { CloseLibrary(LabelBase);       LabelBase       = NULL; }
    if (ListBrowserBase) { CloseLibrary(ListBrowserBase); ListBrowserBase = NULL; }
    if (ButtonBase)      { CloseLibrary(ButtonBase);      ButtonBase      = NULL; }
    if (LayoutBase)      { CloseLibrary(LayoutBase);      LayoutBase      = NULL; }
    if (WindowBase)      { CloseLibrary(WindowBase);      WindowBase      = NULL; }
}

/*------------------------------------------------------------------------*/
/* ListBrowser Helpers                                                    */
/*------------------------------------------------------------------------*/

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

/*
 * Column info for the backdrop ListBrowser - allocated dynamically via
 * AllocLBColumnInfo() to enable sortable column headers.
 * Static arrays cannot be used with SetLBColumnInfoAttrs() and produce
 * garbage values (see docs/Reaction_tips.md - ListBrowser Column Sorting).
 */
static struct ColumnInfo *backdrop_column_info = NULL;

/*------------------------------------------------------------------------*/
/* Allocate and Initialise Column Info                                   */
/*------------------------------------------------------------------------*/

static BOOL init_backdrop_column_info(void)
{
    log_debug(LOG_GUI, "Allocating backdrop column info\n");

    backdrop_column_info = AllocLBColumnInfo(NUM_COLUMNS,
        LBCIA_Column, COL_NAME,
            LBCIA_Title,     "Name",
            LBCIA_Weight,    15,
            LBCIA_Sortable,  TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags,     CIF_SORTABLE | CIF_DRAGGABLE,
        LBCIA_Column, COL_STATUS,
            LBCIA_Title,     "Status",
            LBCIA_Weight,    10,
            LBCIA_Sortable,  TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags,     CIF_SORTABLE | CIF_DRAGGABLE,
        LBCIA_Column, COL_DEVICE,
            LBCIA_Title,     "Device",
            LBCIA_Weight,     9,
            LBCIA_Sortable,  TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags,     CIF_SORTABLE | CIF_DRAGGABLE,
        LBCIA_Column, COL_PATH,
            LBCIA_Title,     "Path",
            LBCIA_Weight,    28,
            LBCIA_Sortable,  TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags,     CIF_SORTABLE | CIF_DRAGGABLE,
        LBCIA_Column, COL_TYPE,
            LBCIA_Title,     "Type",
            LBCIA_Weight,     9,
            LBCIA_Sortable,  TRUE,
            LBCIA_SortArrow, TRUE,
            LBCIA_Flags,     CIF_SORTABLE | CIF_DRAGGABLE,
        LBCIA_Column, COL_POS_X,
            LBCIA_Title,     "X",
            LBCIA_Weight,     6,
            LBCIA_Sortable,  FALSE,
            LBCIA_SortArrow, FALSE,
        LBCIA_Column, COL_POS_Y,
            LBCIA_Title,     "Y",
            LBCIA_Weight,     6,
            LBCIA_Sortable,  FALSE,
            LBCIA_SortArrow, FALSE,
        TAG_DONE);

    if (backdrop_column_info == NULL)
    {
        log_error(LOG_GUI, "Failed to allocate backdrop column info\n");
        return FALSE;
    }

    return TRUE;
}

/**
 * Build ListBrowser nodes from the current backdrop_list.
 */
static void populate_listbrowser(struct iTidyBackdropWindow *bd_data)
{
    struct List *new_list;
    int i;

    if (!bd_data || !bd_data->backdrop_list)
        return;

    /* Detach old list from gadget before freeing */
    if (bd_data->window && bd_data->listbrowser_obj)
    {
        SetGadgetAttrs((struct Gadget *)bd_data->listbrowser_obj,
                       bd_data->window, NULL,
                       LISTBROWSER_Labels, ~0,
                       TAG_DONE);
    }

    /* Free old list */
    if (bd_data->list_nodes)
    {
        free_listbrowser_list(bd_data->list_nodes);
        bd_data->list_nodes = NULL;
    }

    /* Allocate new list */
    new_list = (struct List *)AllocMem(sizeof(struct List), MEMF_CLEAR);
    if (!new_list)
        return;

    NewList(new_list);

    for (i = 0; i < bd_data->backdrop_list->count; i++)
    {
        const iTidy_BackdropEntry *entry = &bd_data->backdrop_list->entries[i];
        struct Node *node;
        const char *status_str;
        const char *type_str;
        char x_buf[16];
        char y_buf[16];

        /* Status text */
        switch (entry->status)
        {
            case ITIDY_ENTRY_VALID:         status_str = "OK";            break;
            case ITIDY_ENTRY_ORPHAN:        status_str = "ORPHAN";        break;
            case ITIDY_ENTRY_CANNOT_VERIFY: status_str = "No media";      break;
            case ITIDY_ENTRY_DEVICE_ICON:   status_str = "Device";        break;
            default:                        status_str = "?";             break;
        }

        /* Type text */
        type_str = itidy_icon_type_name(entry->icon_type);

        /* Position strings - show "Not set" for NO_ICON_POSITION */
        if (entry->icon_x == NO_ICON_POSITION)
            strcpy(x_buf, "Not set");
        else
            sprintf(x_buf, "%ld", (long)entry->icon_x);

        if (entry->icon_y == NO_ICON_POSITION)
            strcpy(y_buf, "Not set");
        else
            sprintf(y_buf, "%ld", (long)entry->icon_y);

        /* Create the node (LBNCA_CopyText MUST come before LBNCA_Text) */
        node = AllocListBrowserNode(NUM_COLUMNS,
            LBNA_UserData, (APTR)(LONG)i,
            LBNA_Column, COL_NAME,
                LBNCA_CopyText, TRUE,
                LBNCA_Text,     entry->display_name,
            LBNA_Column, COL_STATUS,
                LBNCA_CopyText, TRUE,
                LBNCA_Text,     status_str,
            LBNA_Column, COL_DEVICE,
                LBNCA_CopyText, TRUE,
                LBNCA_Text,     entry->device_name,
            LBNA_Column, COL_PATH,
                LBNCA_CopyText, TRUE,
                LBNCA_Text,     entry->entry_path,
            LBNA_Column, COL_TYPE,
                LBNCA_CopyText, TRUE,
                LBNCA_Text,     type_str,
            LBNA_Column, COL_POS_X,
                LBNCA_CopyText, TRUE,
                LBNCA_Text,     x_buf,
            LBNA_Column, COL_POS_Y,
                LBNCA_CopyText, TRUE,
                LBNCA_Text,     y_buf,
            TAG_DONE);

        if (node)
        {
            AddTail(new_list, node);
        }
    }

    bd_data->list_nodes = new_list;

    /* Attach new list to gadget */
    if (bd_data->window && bd_data->listbrowser_obj)
    {
        SetGadgetAttrs((struct Gadget *)bd_data->listbrowser_obj,
                       bd_data->window, NULL,
                       LISTBROWSER_Labels, new_list,
                       LISTBROWSER_AutoFit, TRUE,
                       TAG_DONE);
    }
}

/*------------------------------------------------------------------------*/
/* Status Label Update                                                    */
/*------------------------------------------------------------------------*/

static void update_status_label(struct iTidyBackdropWindow *bd_data,
                                 const char *text)
{
    if (!bd_data || !bd_data->status_label_obj || !bd_data->window)
        return;

    /* Dynamic text label: use Button with GA_ReadOnly + BUTTON_Transparent */
    SetGadgetAttrs((struct Gadget *)bd_data->status_label_obj,
                   bd_data->window, NULL,
                   GA_Text, text,
                   TAG_DONE);
}

/*------------------------------------------------------------------------*/
/* ReAction Requester (per-window copy)                                   */
/*------------------------------------------------------------------------*/

static ULONG show_requester(struct Window *parent_window,
                             CONST_STRPTR title,
                             CONST_STRPTR body,
                             CONST_STRPTR gadgets,
                             ULONG image_type)
{
    Object *req_obj;
    ULONG result = 0;

    req_obj = NewObject(REQUESTER_GetClass(), NULL,
        REQ_Type,       REQTYPE_INFO,
        REQ_TitleText,  title,
        REQ_BodyText,   body,
        REQ_GadgetText, gadgets,
        REQ_Image,      image_type,
        TAG_DONE);

    if (req_obj)
    {
        struct orRequest open_req;
        open_req.MethodID   = RM_OPENREQ;
        open_req.or_Attrs   = NULL;
        open_req.or_Window  = parent_window;
        open_req.or_Screen  = NULL;

        result = DoMethodA(req_obj, (Msg)&open_req);
        DisposeObject(req_obj);
    }

    return result;
}

/*------------------------------------------------------------------------*/
/* Scan Action                                                            */
/*------------------------------------------------------------------------*/

static void perform_scan(struct iTidyBackdropWindow *bd_data)
{
    int total, valid, orphan, device_ct, cannot_verify;
    iTidy_DeviceList *dev_list;
    iTidy_BackdropList *bd_list;
    int i;
    char status_buf[128];

    if (!bd_data)
        return;

    safe_set_window_pointer(bd_data->window, TRUE);
    update_status_label(bd_data, "Scanning devices...");

    /* Free old scan data */
    if (bd_data->backdrop_list)
    {
        itidy_free_backdrop_list(bd_data->backdrop_list);
        whd_free(bd_data->backdrop_list);
        bd_data->backdrop_list = NULL;
    }
    if (bd_data->device_list)
    {
        itidy_free_device_list(bd_data->device_list);
        whd_free(bd_data->device_list);
        bd_data->device_list = NULL;
    }

    /* Scan devices */
    dev_list = (iTidy_DeviceList *)whd_malloc(sizeof(iTidy_DeviceList));
    if (!dev_list)
    {
        update_status_label(bd_data, "Error: out of memory");
        safe_set_window_pointer(bd_data->window, FALSE);
        return;
    }
    itidy_init_device_list(dev_list);
    itidy_scan_devices(dev_list);

    bd_data->device_list = dev_list;

    /* Parse backdrops for each device that has media */
    bd_list = (iTidy_BackdropList *)whd_malloc(sizeof(iTidy_BackdropList));
    if (!bd_list)
    {
        update_status_label(bd_data, "Error: out of memory");
        safe_set_window_pointer(bd_data->window, FALSE);
        return;
    }
    memset(bd_list, 0, sizeof(iTidy_BackdropList));

    update_status_label(bd_data, "Parsing .backdrop files...");

    for (i = 0; i < dev_list->count; i++)
    {
        const iTidy_DeviceInfo *dev = &dev_list->devices[i];

        if (dev->status != ITIDY_DEV_OK)
            continue;
        if (!dev->is_filesystem)
            continue;

        /* Parse this device's .backdrop file */
        {
            iTidy_BackdropList *tmp;
            int j;

            tmp = (iTidy_BackdropList *)whd_malloc(sizeof(iTidy_BackdropList));
            if (!tmp)
                continue;

            memset(tmp, 0, sizeof(iTidy_BackdropList));

            if (itidy_parse_backdrop(dev->volume_name, tmp))
            {
                /* Add the device icon entry itself */
                itidy_add_device_icon_entry(tmp, dev->volume_name);

                /* Validate the entries */
                itidy_validate_backdrop_entries(tmp);

                /* Merge into master list */
                for (j = 0; j < tmp->count; j++)
                {
                    /* Ensure we have space */
                    if (bd_list->count >= bd_list->capacity)
                    {
                        int new_cap = bd_list->capacity > 0 ?
                                      bd_list->capacity * 2 : 32;
                        iTidy_BackdropEntry *new_entries =
                            (iTidy_BackdropEntry *)whd_malloc(
                                new_cap * sizeof(iTidy_BackdropEntry));
                        if (!new_entries)
                            break;

                        if (bd_list->entries && bd_list->count > 0)
                        {
                            memcpy(new_entries, bd_list->entries,
                                   bd_list->count * sizeof(iTidy_BackdropEntry));
                            whd_free(bd_list->entries);
                        }

                        bd_list->entries = new_entries;
                        bd_list->capacity = new_cap;
                    }

                    bd_list->entries[bd_list->count] = tmp->entries[j];
                    bd_list->count++;
                }
            }

            itidy_free_backdrop_list(tmp);
            whd_free(tmp);
        }
    }

    bd_data->backdrop_list = bd_list;
    bd_data->scan_performed = TRUE;

    /* Populate the ListBrowser */
    populate_listbrowser(bd_data);

    /* Count stats */
    total = bd_list->count;
    valid = 0;
    orphan = 0;
    device_ct = 0;
    cannot_verify = 0;

    for (i = 0; i < total; i++)
    {
        switch (bd_list->entries[i].status)
        {
            case ITIDY_ENTRY_VALID:         valid++;         break;
            case ITIDY_ENTRY_ORPHAN:        orphan++;        break;
            case ITIDY_ENTRY_DEVICE_ICON:   device_ct++;     break;
            case ITIDY_ENTRY_CANNOT_VERIFY: cannot_verify++; break;
        }
    }

    sprintf(status_buf, "%d entries: %d OK, %d devices, %d orphans, %d unverified",
            total, valid, device_ct, orphan, cannot_verify);

    update_status_label(bd_data, status_buf);

    /* Enable buttons if we have results */
    if (orphan > 0 && bd_data->remove_btn)
    {
        SetGadgetAttrs((struct Gadget *)bd_data->remove_btn,
                       bd_data->window, NULL,
                       GA_Disabled, FALSE,
                       TAG_DONE);
    }

    if ((valid > 0 || device_ct > 0) && bd_data->tidy_btn)
    {
        SetGadgetAttrs((struct Gadget *)bd_data->tidy_btn,
                       bd_data->window, NULL,
                       GA_Disabled, FALSE,
                       TAG_DONE);
    }

    safe_set_window_pointer(bd_data->window, FALSE);

    log_info(LOG_GUI, "Backdrop scan complete: %s\n", status_buf);
}

/*------------------------------------------------------------------------*/
/* Remove Selected Orphans                                                */
/*------------------------------------------------------------------------*/

static void perform_remove_selected(struct iTidyBackdropWindow *bd_data)
{
    int orphan_count;
    char msg_buf[256];
    int removed;
    int i;

    if (!bd_data || !bd_data->backdrop_list)
        return;

    /* Count orphans */
    orphan_count = itidy_count_orphans(bd_data->backdrop_list);

    if (orphan_count == 0)
    {
        show_requester(bd_data->window,
                       "No Orphans",
                       "No orphaned entries were found.",
                       "_OK",
                       REQIMAGE_INFO);
        return;
    }

    /* Mark all orphans as selected for removal */
    for (i = 0; i < bd_data->backdrop_list->count; i++)
    {
        if (bd_data->backdrop_list->entries[i].status == ITIDY_ENTRY_ORPHAN)
        {
            bd_data->backdrop_list->entries[i].selected = TRUE;
        }
    }

    sprintf(msg_buf,
            "Remove %d orphaned entries from .backdrop files?\n\n"
            "A backup (.backdrop.bak) will be created first.",
            orphan_count);

    /* Confirm with user */
    if (show_requester(bd_data->window,
                       "Confirm Orphan Removal",
                       msg_buf,
                       "_Remove|_Cancel",
                       REQIMAGE_QUESTION) != 1)
    {
        /* User cancelled - deselect all */
        for (i = 0; i < bd_data->backdrop_list->count; i++)
        {
            bd_data->backdrop_list->entries[i].selected = FALSE;
        }
        return;
    }

    safe_set_window_pointer(bd_data->window, TRUE);
    update_status_label(bd_data, "Removing orphans...");

    /* Remove orphans per device (itidy_remove_selected_orphans works per-device) */
    removed = 0;
    {
        /* Collect unique device names from orphan entries */
        char devices_done[16][ITIDY_BACKDROP_MAX_DEVICE];
        int num_devices_done = 0;

        for (i = 0; i < bd_data->backdrop_list->count; i++)
        {
            const iTidy_BackdropEntry *entry = &bd_data->backdrop_list->entries[i];
            int j;
            BOOL already_done = FALSE;

            if (entry->status != ITIDY_ENTRY_ORPHAN || !entry->selected)
                continue;

            /* Check if we already processed this device */
            for (j = 0; j < num_devices_done; j++)
            {
                if (strcmp(devices_done[j], entry->device_name) == 0)
                {
                    already_done = TRUE;
                    break;
                }
            }

            if (already_done)
                continue;

            /* Track this device */
            if (num_devices_done < 16)
            {
                strncpy(devices_done[num_devices_done], entry->device_name,
                        ITIDY_BACKDROP_MAX_DEVICE - 1);
                devices_done[num_devices_done][ITIDY_BACKDROP_MAX_DEVICE - 1] = '\0';
                num_devices_done++;
            }

            /* Remove orphans for this device */
            {
                int dev_removed = itidy_remove_selected_orphans(
                    entry->device_name, bd_data->backdrop_list);
                if (dev_removed > 0)
                    removed += dev_removed;
            }
        }
    }

    if (removed > 0)
    {
        char result_buf[128];

        bd_data->changes_made = TRUE;

        sprintf(result_buf, "Removed %d orphaned entries.", removed);
        update_status_label(bd_data, result_buf);

        /* Re-scan to refresh the display */
        safe_set_window_pointer(bd_data->window, FALSE);
        perform_scan(bd_data);
    }
    else
    {
        update_status_label(bd_data, "No entries were removed.");
        safe_set_window_pointer(bd_data->window, FALSE);
    }
}

/*------------------------------------------------------------------------*/
/* WB Snapshot via ARexx                                                 */
/*------------------------------------------------------------------------*/

/*
 * Write a tiny ARexx script to T: and run it synchronously via 'RX'.
 * This replicates the manual sequence:
 *   Workbench -> Update All
 *   Workbench -> Redraw All
 *   Workbench -> Snapshot -> All
 *
 * Running Snapshot All after iTidy has written new positions ensures that
 * WB's in-memory state (correct, because all devices are now mounted) is
 * flushed back to the .info files.  On the next reboot every icon loads
 * from those pre-correct coordinates and WB's boot-time collision-
 * avoidance logic never fires.
 *
 * Must be called after itidy_apply_wb_layout() has written positions.
 */
static void run_wb_snapshot_arexx(void)
{
    static const char *script_path = "T:iTidy_SnapWB.rexx";
    static const char *script_content =
        "/* iTidy - Workbench Snapshot */\n"
        "/* Update All */\n"
        "MENU INVOKE WORKBENCH.UPDATEALL\n"
        "/* Redraw All */\n"
        "MENU INVOKE WORKBENCH.REDRAWALL\n"
        "ADDRESS WORKBENCH\n"
        "WINDOW ROOT ACTIVATE\n"
        "MENU INVOKE WINDOW.SNAPSHOT.ALL\n";

    BPTR fh;

    /* Write script to T: */
    fh = Open((STRPTR)script_path, MODE_NEWFILE);
    if (!fh)
    {
        log_warning(LOG_GUI,
                    "run_wb_snapshot_arexx: cannot write %s\n",
                    script_path);
        return;
    }
    Write(fh, (APTR)script_content, (LONG)strlen(script_content));
    Close(fh);

    log_info(LOG_GUI, "Running WB Snapshot ARexx script...\n");

    /* Run synchronously - blocks until Snapshot All completes */
    /* Both handles must be valid - RX requires a real input stream */
    {
        BPTR in_fh  = Open((STRPTR)"NIL:", MODE_OLDFILE);
        BPTR out_fh = Open((STRPTR)"NIL:", MODE_NEWFILE);
        LONG rc;

        rc = SystemTags((STRPTR)"SYS:RexxC/RX T:iTidy_SnapWB.rexx",
                       SYS_Input,  in_fh,
                       SYS_Output, out_fh,
                       SYS_Asynch, FALSE,
                       TAG_DONE);

        if (in_fh)  Close(in_fh);
        if (out_fh) Close(out_fh);

        if (rc != 0)
            log_warning(LOG_GUI,
                        "WB Snapshot: RX returned error %ld\n", (long)rc);
    }

    /* Remove temp script */
    DeleteFile((STRPTR)script_path);

    log_info(LOG_GUI, "WB Snapshot ARexx script complete.\n");
}

/*
 * Update All + Redraw All only - no Snapshot.
 * Used by the TEST button to verify ARexx menu commands work on this system
 * without committing a snapshot.
 */
static void run_wb_update_redraw_arexx(void)
{
    static const char *script_path = "T:iTidy_UpdWB.rexx";
    static const char *script_content =
        "/* iTidy - WB Update+Redraw test */\n"
        "MENU INVOKE WORKBENCH.UPDATEALL\n"
        "MENU INVOKE WORKBENCH.REDRAWALL\n";

    BPTR fh;
    BPTR in_fh;
    BPTR out_fh;
    LONG rc;

    fh = Open((STRPTR)script_path, MODE_NEWFILE);
    if (!fh)
    {
        log_warning(LOG_GUI,
                    "run_wb_update_redraw_arexx: cannot write %s\n",
                    script_path);
        return;
    }
    Write(fh, (APTR)script_content, (LONG)strlen(script_content));
    Close(fh);

    log_info(LOG_GUI, "Running WB Update+Redraw ARexx script...\n");

    /* Both handles must be valid - RX requires a real input stream */
    in_fh  = Open((STRPTR)"NIL:", MODE_OLDFILE);
    out_fh = Open((STRPTR)"NIL:", MODE_NEWFILE);

    rc = SystemTags((STRPTR)"SYS:RexxC/RX T:iTidy_UpdWB.rexx",
                   SYS_Input,  in_fh,
                   SYS_Output, out_fh,
                   SYS_Asynch, FALSE,
                   TAG_DONE);

    if (in_fh)  Close(in_fh);
    if (out_fh) Close(out_fh);

    DeleteFile((STRPTR)script_path);

    if (rc != 0)
        log_warning(LOG_GUI,
                    "WB Update+Redraw: RX returned error %ld\n", (long)rc);
    else
        log_info(LOG_GUI, "WB Update+Redraw ARexx script complete.\n");
}

/*------------------------------------------------------------------------*/
/* Tidy Layout                                                            */
/*------------------------------------------------------------------------*/

static void perform_tidy_layout(struct iTidyBackdropWindow *bd_data)
{
    iTidy_WBLayoutParams params;
    iTidy_WBLayoutResult result;
    LONG screen_width, screen_height;
    int applied;
    char msg_buf[256];

    if (!bd_data || !bd_data->backdrop_list)
        return;

    /* Get screen dimensions */
    if (bd_data->screen)
    {
        screen_width  = bd_data->screen->Width;
        screen_height = bd_data->screen->Height;
    }
    else
    {
        screen_width  = 640;
        screen_height = 256;
    }

    /* Calculate layout */
    itidy_init_wb_layout_params(&params, screen_width, screen_height);
    itidy_init_wb_layout_result(&result);

    if (!itidy_calculate_wb_layout(&params, bd_data->backdrop_list, &result))
    {
        show_requester(bd_data->window,
                       "Layout Error",
                       "Failed to calculate icon layout.",
                       "_OK",
                       REQIMAGE_ERROR);
        itidy_free_wb_layout_result(&result);
        return;
    }

    if (result.count == 0)
    {
        show_requester(bd_data->window,
                       "No Icons",
                       "No valid icons found to layout.",
                       "_OK",
                       REQIMAGE_INFO);
        itidy_free_wb_layout_result(&result);
        return;
    }

    /* Confirm with user - always write all icons to ensure
     * Workbench refreshes backdrop icon positions via
     * ICONPUTA_NotifyWorkbench notification. */
    if (result.changed_count > 0)
    {
        sprintf(msg_buf,
                "Layout %d Workbench icons into a tidy grid?\n"
                "(%d icons need repositioning)\n\n"
                "Icon positions will be saved to disk.",
                result.count, result.changed_count);
    }
    else
    {
        sprintf(msg_buf,
                "All %d icons are already at target positions.\n\n"
                "Re-apply layout to refresh Workbench display?",
                result.count);
    }

    if (show_requester(bd_data->window,
                       "Confirm Layout",
                       msg_buf,
                       "_Tidy|_Cancel",
                       REQIMAGE_QUESTION) != 1)
    {
        itidy_free_wb_layout_result(&result);
        return;
    }

    safe_set_window_pointer(bd_data->window, TRUE);
    update_status_label(bd_data, "Repositioning icons...");

    /* Apply the layout */
    applied = itidy_apply_wb_layout(bd_data->backdrop_list, &result);

    /* Handle RAM: persistence */
    itidy_handle_ram_icon_persistence(bd_data->window,
                                      bd_data->backdrop_list,
                                      &result);

    itidy_free_wb_layout_result(&result);

    if (applied > 0)
    {
        char result_buf[196];

        bd_data->changes_made = TRUE;

        /* Snapshot All - flush correct positions to disk before WB can
         * clobber them with its in-memory state on the next Snapshot. */
        update_status_label(bd_data, "Snapshotting Workbench...");
        run_wb_snapshot_arexx();

        sprintf(result_buf,
                "Updated %d icons.\n"
                "Workbench has been snapshotted.\n"
                "Positions will survive a reboot.",
                applied);
        update_status_label(bd_data, result_buf);

        show_requester(bd_data->window,
                       "Layout Complete",
                       result_buf,
                       "_OK",
                       REQIMAGE_INFO);

        /* Re-scan to show updated positions */
        safe_set_window_pointer(bd_data->window, FALSE);
        perform_scan(bd_data);
    }
    else
    {
        update_status_label(bd_data, "No icons were repositioned.");
        safe_set_window_pointer(bd_data->window, FALSE);
    }
}

/*------------------------------------------------------------------------*/
/* Open Window                                                            */
/*------------------------------------------------------------------------*/

BOOL open_backdrop_window(struct iTidyBackdropWindow *bd_data)
{
    struct Screen *screen;

    log_info(LOG_GUI, "=== open_backdrop_window: Starting (ReAction) ===\n");

    if (bd_data == NULL)
    {
        log_error(LOG_GUI, "ERROR: bd_data is NULL\n");
        return FALSE;
    }

    /* Initialize structure */
    memset(bd_data, 0, sizeof(struct iTidyBackdropWindow));
    bd_data->selected_index = -1;
    bd_data->sort_column    = ~0UL;       /* ~0 = no column sorted yet */
    bd_data->sort_direction = LBMSORT_FORWARD;

    /* Open ReAction classes */
    if (!open_reaction_classes())
    {
        log_error(LOG_GUI, "Failed to open ReAction classes\n");
        return FALSE;
    }

    /* Allocate column info with sorting support */
    if (!init_backdrop_column_info())
    {
        log_error(LOG_GUI, "Failed to initialise backdrop column info\n");
        close_reaction_classes();
        return FALSE;
    }

    /* Get Workbench screen */
    screen = LockPubScreen(NULL);
    if (screen == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to lock Workbench screen\n");
        FreeLBColumnInfo(backdrop_column_info);
        backdrop_column_info = NULL;
        close_reaction_classes();
        return FALSE;
    }

    bd_data->screen = screen;

    /* Get visual info for menus */
    bd_data->visual_info = GetVisualInfo(screen, TAG_END);
    if (!bd_data->visual_info)
    {
        log_warning(LOG_GUI, "Backdrop window: Failed to get visual info - menus disabled\n");
    }

    /* Create menus */
    if (bd_data->visual_info)
    {
        bd_data->menu_strip = CreateMenus(backdrop_menu_template, TAG_END);
        if (bd_data->menu_strip)
        {
            LayoutMenus(bd_data->menu_strip, bd_data->visual_info,
                GTMN_NewLookMenus, TRUE,
                TAG_END);
        }
        else
        {
            log_warning(LOG_GUI, "Backdrop window: Failed to create menus\n");
        }
    }

    /* Create the ReAction window object */
    bd_data->window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title,        BACKDROP_WINDOW_TITLE,
        WA_ScreenTitle,  BACKDROP_WINDOW_TITLE,
        WA_PubScreen,    screen,
        WA_Left,         40,
        WA_Top,          25,
        WA_Width,        560,
        WA_Height,       320,
        WA_MinWidth,     450,
        WA_MinHeight,    200,
        WA_MaxWidth,     8192,
        WA_MaxHeight,    8192,
        WA_CloseGadget,  TRUE,
        WA_DepthGadget,  TRUE,
        WA_SizeGadget,   TRUE,
        WA_DragBar,      TRUE,
        WA_Activate,     TRUE,
        WA_NoCareRefresh, TRUE,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_MENUPICK,

        WINDOW_ParentGroup, NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_SpaceOuter,  TRUE,
            LAYOUT_DeferLayout, TRUE,

            /* Root vertical layout */
            LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                GA_ID, GID_BD_ROOT_LAYOUT,
                LAYOUT_Orientation, LAYOUT_ORIENT_VERT,

                /* ListBrowser */
                LAYOUT_AddChild, bd_data->listbrowser_obj =
                    NewObject(LISTBROWSER_GetClass(), NULL,
                    GA_ID,                    GID_BD_LISTBROWSER,
                    GA_RelVerify,             TRUE,
                    GA_TabCycle,              TRUE,
                    LISTBROWSER_ColumnInfo,   backdrop_column_info,
                    LISTBROWSER_ColumnTitles, TRUE,
                    LISTBROWSER_ShowSelected, TRUE,
                    LISTBROWSER_AutoFit,      TRUE,
                    LISTBROWSER_HorizontalProp, TRUE,
                TAG_END),
                CHILD_WeightedHeight, 80,

                /* Status bar (dynamic text via read-only transparent button) */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, GID_BD_STATUS_LAYOUT,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_TopSpacing,  2,

                    LAYOUT_AddChild, bd_data->status_label_obj =
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID,             GID_BD_STATUS_LABEL,
                        GA_ReadOnly,       TRUE,
                        GA_Text,           "Press 'Scan' to begin.",
                        BUTTON_BevelStyle, BVS_NONE,
                        BUTTON_Transparent, TRUE,
                    TAG_END),
                TAG_END),
                CHILD_WeightedHeight, 0,

                /* Button row */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, GID_BD_BUTTONS_LAYOUT,
                    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                    LAYOUT_TopSpacing,  4,
                    LAYOUT_EvenSize,    TRUE,

                    LAYOUT_AddChild, bd_data->scan_btn =
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID,        GID_BD_SCAN_BUTTON,
                        GA_Text,      "Scan",
                        GA_RelVerify, TRUE,
                        GA_TabCycle,  TRUE,
                    TAG_END),

                    LAYOUT_AddChild, bd_data->remove_btn =
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID,        GID_BD_REMOVE_BUTTON,
                        GA_Text,      "Remove Orphans",
                        GA_RelVerify, TRUE,
                        GA_TabCycle,  TRUE,
                        GA_Disabled,  TRUE,
                    TAG_END),

                    LAYOUT_AddChild, bd_data->tidy_btn =
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID,        GID_BD_TIDY_BUTTON,
                        GA_Text,      "Tidy Layout",
                        GA_RelVerify, TRUE,
                        GA_TabCycle,  TRUE,
                        GA_Disabled,  TRUE,
                    TAG_END),

                    LAYOUT_AddChild, bd_data->test_btn =
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID,        GID_BD_TEST_BUTTON,
                        GA_Text,      "TEST Upd/Redraw",
                        GA_RelVerify, TRUE,
                        GA_TabCycle,  TRUE,
                    TAG_END),

                    LAYOUT_AddChild, bd_data->close_btn =
                        NewObject(BUTTON_GetClass(), NULL,
                        GA_ID,        GID_BD_CLOSE_BUTTON,
                        GA_Text,      "Close",
                        GA_RelVerify, TRUE,
                        GA_TabCycle,  TRUE,
                    TAG_END),
                TAG_END),
                CHILD_WeightedHeight, 0,

            TAG_END), /* End root layout */
        TAG_END), /* End parent group */
    TAG_END);

    if (bd_data->window_obj == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to create backdrop window object\n");
        FreeLBColumnInfo(backdrop_column_info);
        backdrop_column_info = NULL;
        UnlockPubScreen(NULL, screen);
        close_reaction_classes();
        return FALSE;
    }

    /* Open the window */
    bd_data->window = (struct Window *)RA_OpenWindow(bd_data->window_obj);
    if (bd_data->window == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to open backdrop window\n");
        DisposeObject(bd_data->window_obj);
        bd_data->window_obj = NULL;
        FreeLBColumnInfo(backdrop_column_info);
        backdrop_column_info = NULL;
        UnlockPubScreen(NULL, screen);
        close_reaction_classes();
        return FALSE;
    }

    /* Attach menus */
    if (bd_data->menu_strip)
    {
        SetMenuStrip(bd_data->window, bd_data->menu_strip);
    }

    bd_data->window_open = TRUE;
    log_info(LOG_GUI, "Backdrop window opened successfully\n");

    /* Set LISTBROWSER_TitleClickable AFTER window is opened.
     * Setting it in NewObject() is silently ignored (see Reaction_tips.md,
     * "LISTBROWSER_TitleClickable Must Be Set AFTER Window Opens"). */
    if (bd_data->listbrowser_obj && bd_data->window)
    {
        log_debug(LOG_GUI, "Setting LISTBROWSER_TitleClickable to TRUE\n");
        SetGadgetAttrs((struct Gadget *)bd_data->listbrowser_obj,
                       bd_data->window, NULL,
                       LISTBROWSER_TitleClickable, TRUE,
                       TAG_DONE);
    }

    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Close Window                                                           */
/*------------------------------------------------------------------------*/

void close_backdrop_window(struct iTidyBackdropWindow *bd_data)
{
    if (bd_data == NULL)
        return;

    log_info(LOG_GUI, "close_backdrop_window: Starting cleanup\n");

    /* Remove menus before disposing window */
    if (bd_data->window && bd_data->menu_strip)
    {
        ClearMenuStrip(bd_data->window);
    }

    /* Detach ListBrowser list before disposing window */
    if (bd_data->window && bd_data->listbrowser_obj)
    {
        SetGadgetAttrs((struct Gadget *)bd_data->listbrowser_obj,
                       bd_data->window, NULL,
                       LISTBROWSER_Labels, ~0,
                       TAG_DONE);
    }

    /* Dispose window object (automatically frees all child gadgets) */
    if (bd_data->window_obj)
    {
        DisposeObject(bd_data->window_obj);
        bd_data->window_obj    = NULL;
        bd_data->window        = NULL;
        bd_data->listbrowser_obj = NULL;
        bd_data->scan_btn      = NULL;
        bd_data->remove_btn    = NULL;
        bd_data->tidy_btn      = NULL;
        bd_data->close_btn     = NULL;
        bd_data->status_label_obj = NULL;
    }

    /* Free ListBrowser node list */
    if (bd_data->list_nodes)
    {
        free_listbrowser_list(bd_data->list_nodes);
        bd_data->list_nodes = NULL;
    }

    /* Free scan data */
    if (bd_data->backdrop_list)
    {
        itidy_free_backdrop_list(bd_data->backdrop_list);
        whd_free(bd_data->backdrop_list);
        bd_data->backdrop_list = NULL;
    }

    if (bd_data->device_list)
    {
        itidy_free_device_list(bd_data->device_list);
        whd_free(bd_data->device_list);
        bd_data->device_list = NULL;
    }

    /* Free column info (must be done before close_reaction_classes) */
    if (backdrop_column_info != NULL)
    {
        FreeLBColumnInfo(backdrop_column_info);
        backdrop_column_info = NULL;
    }

    /* Free menu strip */
    if (bd_data->menu_strip)
    {
        FreeMenus(bd_data->menu_strip);
        bd_data->menu_strip = NULL;
    }

    /* Free visual info */
    if (bd_data->visual_info)
    {
        FreeVisualInfo(bd_data->visual_info);
        bd_data->visual_info = NULL;
    }

    /* Unlock screen */
    if (bd_data->screen)
    {
        UnlockPubScreen(NULL, bd_data->screen);
        bd_data->screen = NULL;
    }

    /* Close class libraries */
    close_reaction_classes();

    bd_data->window_open = FALSE;

    log_info(LOG_GUI, "Backdrop window closed successfully\n");
}

/*------------------------------------------------------------------------*/
/* Save List As                                                           */
/*------------------------------------------------------------------------*/

/**
 * perform_save_list_as - Dump the current ListBrowser contents to a text
 * file chosen via an ASL save requester.  Intended for AI agent validation.
 */
static void perform_save_list_as(struct iTidyBackdropWindow *bd_data)
{
    struct FileRequester *freq;
    char full_path[512];
    BPTR out_file;
    int i;
    char line_buf[512];

    if (!bd_data || !bd_data->window)
        return;

    if (!bd_data->scan_performed || !bd_data->backdrop_list)
    {
        show_requester(bd_data->window,
                       "No Data",
                       "No scan data to save.\nPlease click Scan first.",
                       "_OK",
                       REQIMAGE_INFO);
        return;
    }

    /* Expand PROGDIR: to the real directory path for the file requester */
    {
        char expanded_dir[512];
        if (!ExpandProgDir("PROGDIR:", expanded_dir, sizeof(expanded_dir)))
        {
            log_warning(LOG_GUI, "perform_save_list_as: ExpandProgDir failed, falling back to PROGDIR:\n");
            strcpy(expanded_dir, "PROGDIR:");
        }
        log_info(LOG_GUI, "Save list as: initial drawer: %s\n", expanded_dir);

        /* Open ASL save file requester */
        freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
            ASLFR_TitleText,     "Save List As...",
            ASLFR_InitialDrawer, expanded_dir,
            ASLFR_InitialFile,   "backdrop_list.txt",
            ASLFR_DoSaveMode,    TRUE,
            ASLFR_RejectIcons,   TRUE,
            ASLFR_Window,        bd_data->window,
            TAG_DONE);
    } /* end expanded_dir scope */

    if (!freq)
    {
        show_requester(bd_data->window,
                       "Error",
                       "Could not open file requester.",
                       "_OK",
                       REQIMAGE_ERROR);
        return;
    }

    if (!AslRequest(freq, NULL))
    {
        /* User cancelled */
        FreeAslRequest(freq);
        return;
    }

    /* Build full file path */
    strcpy(full_path, freq->fr_Drawer);
    if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
    {
        FreeAslRequest(freq);
        show_requester(bd_data->window,
                       "Error",
                       "File path is too long.",
                       "_OK",
                       REQIMAGE_ERROR);
        return;
    }

    FreeAslRequest(freq);

    /* Open file for writing */
    out_file = Open((STRPTR)full_path, MODE_NEWFILE);
    if (!out_file)
    {
        show_requester(bd_data->window,
                       "Error",
                       "Could not create output file.",
                       "_OK",
                       REQIMAGE_ERROR);
        return;
    }

    /* Write file header */
    sprintf(line_buf, "iTidy Backdrop Window - List Contents\n");
    Write(out_file, line_buf, strlen(line_buf));
    sprintf(line_buf, "Generated by: Save list as... (Project menu)\n");
    Write(out_file, line_buf, strlen(line_buf));
    sprintf(line_buf, "Total entries: %d\n", bd_data->backdrop_list->count);
    Write(out_file, line_buf, strlen(line_buf));
    sprintf(line_buf, "\n");
    Write(out_file, line_buf, strlen(line_buf));

    /* Column header */
    sprintf(line_buf, "%-32s  %-12s  %-10s  %-40s  %-12s  %-8s  %-8s\n",
            "Name", "Status", "Device", "Path", "Type", "X", "Y");
    Write(out_file, line_buf, strlen(line_buf));

    /* Separator */
    sprintf(line_buf,
            "------------------------------  ----------  --------  "
            "--------------------------------------  ----------  ------  ------\n");
    Write(out_file, line_buf, strlen(line_buf));

    /* Write each entry */
    for (i = 0; i < bd_data->backdrop_list->count; i++)
    {
        const iTidy_BackdropEntry *entry = &bd_data->backdrop_list->entries[i];
        const char *status_str;
        const char *type_str;
        char x_buf[16];
        char y_buf[16];

        switch (entry->status)
        {
            case ITIDY_ENTRY_VALID:         status_str = "OK";          break;
            case ITIDY_ENTRY_ORPHAN:        status_str = "ORPHAN";      break;
            case ITIDY_ENTRY_CANNOT_VERIFY: status_str = "No media";    break;
            case ITIDY_ENTRY_DEVICE_ICON:   status_str = "Device";      break;
            default:                        status_str = "?";           break;
        }

        type_str = itidy_icon_type_name(entry->icon_type);

        if (entry->icon_x == NO_ICON_POSITION)
            strcpy(x_buf, "Not set");
        else
            sprintf(x_buf, "%ld", (long)entry->icon_x);

        if (entry->icon_y == NO_ICON_POSITION)
            strcpy(y_buf, "Not set");
        else
            sprintf(y_buf, "%ld", (long)entry->icon_y);

        sprintf(line_buf, "%-32s  %-12s  %-10s  %-40s  %-12s  %-8s  %-8s\n",
                entry->display_name,
                status_str,
                entry->device_name,
                entry->entry_path,
                type_str,
                x_buf,
                y_buf);
        Write(out_file, line_buf, strlen(line_buf));
    }

    /* Write summary */
    {
        int valid = 0, orphan = 0, device_ct = 0, cannot_verify = 0;
        for (i = 0; i < bd_data->backdrop_list->count; i++)
        {
            switch (bd_data->backdrop_list->entries[i].status)
            {
                case ITIDY_ENTRY_VALID:         valid++;         break;
                case ITIDY_ENTRY_ORPHAN:        orphan++;        break;
                case ITIDY_ENTRY_DEVICE_ICON:   device_ct++;     break;
                case ITIDY_ENTRY_CANNOT_VERIFY: cannot_verify++; break;
            }
        }
        sprintf(line_buf, "\nSummary: %d total, %d OK, %d devices, %d orphans, %d unverified\n",
                bd_data->backdrop_list->count, valid, device_ct, orphan, cannot_verify);
        Write(out_file, line_buf, strlen(line_buf));
    }

    Close(out_file);

    /* Success message */
    {
        char msg[64];
        const char *filename = FilePart((STRPTR)full_path);
        sprintf(msg, "List saved to:\n%s", filename ? filename : full_path);
        show_requester(bd_data->window,
                       "Saved",
                       msg,
                       "_OK",
                       REQIMAGE_INFO);
    }

    log_info(LOG_GUI, "Backdrop list saved to: %s (%d entries)\n",
             full_path, bd_data->backdrop_list->count);
}

/*------------------------------------------------------------------------*/
/* Event Handler                                                          */
/*------------------------------------------------------------------------*/

BOOL handle_backdrop_window_events(struct iTidyBackdropWindow *bd_data)
{
    ULONG signals, signal_mask;
    ULONG result;
    UWORD code;
    BOOL continue_running = TRUE;

    if (!bd_data || !bd_data->window_obj)
        return FALSE;

    /* Get signal mask from ReAction window */
    GetAttr(WINDOW_SigMask, bd_data->window_obj, &signal_mask);

    /* Wait for events (single Wait - no double-Wait!) */
    signals = Wait(signal_mask | SIGBREAKF_CTRL_C);

    if (signals & SIGBREAKF_CTRL_C)
    {
        log_debug(LOG_GUI, "Ctrl+C detected, closing backdrop window.\n");
        return FALSE;
    }

    /* Process all pending messages */
    while ((result = RA_HandleInput(bd_data->window_obj, &code)) != WMHI_LASTMSG)
    {
        switch (result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                log_debug(LOG_GUI, "Backdrop window close gadget clicked\n");
                continue_running = FALSE;
                break;

            case WMHI_MENUPICK:
            {
                struct MenuItem *item;
                ULONG menu_number = (ULONG)code;

                while (menu_number != MENUNULL)
                {
                    item = ItemAddress(bd_data->menu_strip, menu_number);
                    if (!item)
                        break;

                    switch ((ULONG)GTMENUITEM_USERDATA(item))
                    {
                        case MENU_BD_PROJECT_SAVE_AS:
                            perform_save_list_as(bd_data);
                            break;
                    }

                    menu_number = item->NextSelect;
                }
                break;
            }

            case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                    case GID_BD_LISTBROWSER:
                    {
                        ULONG rel_event = LBRE_NORMAL;

                        GetAttr(LISTBROWSER_RelEvent,
                                bd_data->listbrowser_obj,
                                &rel_event);

                        if (rel_event == LBRE_TITLECLICK)
                        {
                            /* 'code' holds the clicked column number */
                            ULONG sort_column = (ULONG)code;
                            ULONG direction;

                            /* Toggle direction when clicking the same column */
                            if (sort_column == bd_data->sort_column)
                            {
                                direction = (bd_data->sort_direction == LBMSORT_FORWARD)
                                            ? LBMSORT_REVERSE : LBMSORT_FORWARD;
                            }
                            else
                            {
                                direction = LBMSORT_FORWARD;
                            }

                            bd_data->sort_column    = sort_column;
                            bd_data->sort_direction = direction;

                            log_debug(LOG_GUI, "Column sort: col=%lu dir=%s\n",
                                      sort_column,
                                      (direction == LBMSORT_FORWARD) ? "FWD" : "REV");

                            /* List must be ATTACHED when calling LBM_SORT */
                            if (bd_data->list_nodes)
                            {
                                LONG sort_result;

                                sort_result = DoGadgetMethod(
                                    (struct Gadget *)bd_data->listbrowser_obj,
                                    bd_data->window, NULL,
                                    LBM_SORT, bd_data->list_nodes,
                                    sort_column, direction, NULL);

                                log_debug(LOG_GUI, "LBM_SORT returned: %ld\n", sort_result);

                                /* Update sort arrow to show active column */
                                SetGadgetAttrs(
                                    (struct Gadget *)bd_data->listbrowser_obj,
                                    bd_data->window, NULL,
                                    LISTBROWSER_SortColumn, sort_column,
                                    TAG_DONE);

                                RefreshGadgets(
                                    (struct Gadget *)bd_data->listbrowser_obj,
                                    bd_data->window, NULL);
                            }
                        }
                        else if (rel_event == LBRE_COLUMNADJUST)
                        {
                            /* Column resize - handled automatically, no action needed */
                        }
                        else
                        {
                            ULONG selected = ~0;

                            GetAttr(LISTBROWSER_Selected,
                                    bd_data->listbrowser_obj,
                                    &selected);

                            if (selected != ~0 &&
                                bd_data->backdrop_list &&
                                (int)selected < bd_data->backdrop_list->count)
                            {
                                bd_data->selected_index = (LONG)selected;
                            }
                            else
                            {
                                bd_data->selected_index = -1;
                            }
                        }
                        break;
                    }

                    case GID_BD_SCAN_BUTTON:
                        perform_scan(bd_data);
                        break;

                    case GID_BD_REMOVE_BUTTON:
                        perform_remove_selected(bd_data);
                        break;

                    case GID_BD_TIDY_BUTTON:
                        perform_tidy_layout(bd_data);
                        break;

                    case GID_BD_TEST_BUTTON:
                        update_status_label(bd_data, "Running Update All + Redraw All...");
                        run_wb_update_redraw_arexx();
                        update_status_label(bd_data, "Update+Redraw done. Did icons move?");
                        break;

                    case GID_BD_CLOSE_BUTTON:
                        continue_running = FALSE;
                        break;
                }
                break;
        }
    }

    return continue_running;
}
