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

#include "backdrop_window.h"
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
static void free_listbrowser_list(struct List *list);
static void populate_listbrowser(struct iTidyBackdropWindow *bd_data);
static void update_status_label(struct iTidyBackdropWindow *bd_data,
                                 const char *text);
static void perform_scan(struct iTidyBackdropWindow *bd_data);
static void perform_remove_selected(struct iTidyBackdropWindow *bd_data);
static void perform_tidy_layout(struct iTidyBackdropWindow *bd_data);
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

/**
 * Column info for the backdrop ListBrowser.
 * NOT static: AutoFit writes back into the array, so each window
 * needs its own mutable copy.
 */
static struct ColumnInfo backdrop_column_info[] = {
    { 15, "Name",   0 },
    { 10, "Status", 0 },
    {  9, "Device", 0 },
    { 28, "Path",   0 },
    {  9, "Type",   0 },
    {  6, "X",      0 },
    {  6, "Y",      0 },
    { -1, (STRPTR)~0, -1 }
};

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

        sprintf(result_buf,
                "Updated %d icons.\n"
                "Use Workbench -> Update All\n"
                "if icons don't move immediately.",
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

    /* Open ReAction classes */
    if (!open_reaction_classes())
    {
        log_error(LOG_GUI, "Failed to open ReAction classes\n");
        return FALSE;
    }

    /* Get Workbench screen */
    screen = LockPubScreen(NULL);
    if (screen == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to lock Workbench screen\n");
        close_reaction_classes();
        return FALSE;
    }

    bd_data->screen = screen;

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
        WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE,

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
        UnlockPubScreen(NULL, screen);
        close_reaction_classes();
        return FALSE;
    }

    bd_data->window_open = TRUE;
    log_info(LOG_GUI, "Backdrop window opened successfully\n");

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

            case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                    case GID_BD_LISTBROWSER:
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

                    case GID_BD_CLOSE_BUTTON:
                        continue_running = FALSE;
                        break;
                }
                break;
        }
    }

    return continue_running;
}
