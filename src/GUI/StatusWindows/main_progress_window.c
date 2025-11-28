/*
 * main_progress_window.c - iTidy Main Progress Window implementation
 * Provides a scrolling ListView history with a full-width Cancel button.
 */

#include "platform/platform.h"

#include "main_progress_window.h"
#include "writeLog.h"
#include "easy_request_helper.h"

#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <graphics/gfx.h>
#include <graphics/text.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>

#include <string.h>

#define ITIDY_MAIN_PROGRESS_TITLE "iTidy - Progress"

#ifndef NewList
VOID NewList(struct List *list);
#endif

#ifndef AddTail
VOID AddTail(struct List *list, struct Node *node);
#endif

#ifndef Remove
VOID Remove(struct Node *node);
#endif

typedef struct iTidyMainProgressEntry
{
    struct Node node;
    char *text;
} iTidyMainProgressEntry;

static BOOL add_history_entry_internal(struct iTidyMainProgressWindow *window_data,
                                       const char *status_text,
                                       BOOL refresh_list);
static void free_history_entries(struct iTidyMainProgressWindow *window_data);

BOOL itidy_main_progress_window_open(struct iTidyMainProgressWindow *window_data)
{
    struct Screen *screen = NULL;
    struct DrawInfo *draw_info = NULL;
    struct Gadget *gad;
    struct NewGadget ng;
    UWORD font_width, font_height;
    UWORD listview_width, listview_height;
    UWORD button_height;
    WORD border_left, border_right, border_top, border_bottom;
    WORD window_left, window_top;
    UWORD content_right, content_bottom;
    UWORD window_width, window_height;
    BOOL success = FALSE;

    if (window_data == NULL)
    {
        return FALSE;
    }

    memset(window_data, 0, sizeof(*window_data));
    NewList(&window_data->history_entries);

    screen = LockPubScreen(NULL);
    if (screen == NULL)
    {
        append_to_log("ERROR: Failed to lock Workbench screen for main progress window\n");
        return FALSE;
    }
    window_data->screen = screen;

    draw_info = GetScreenDrawInfo(screen);
    if (draw_info == NULL)
    {
        append_to_log("ERROR: GetScreenDrawInfo failed for main progress window\n");
        goto cleanup;
    }

    font_width = draw_info->dri_Font->tf_XSize;
    font_height = draw_info->dri_Font->tf_YSize;

    listview_width = font_width * ITIDY_MAIN_PROGRESS_WIDTH_CHARS;
    if (listview_width < ITIDY_MAIN_PROGRESS_MIN_WIDTH)
    {
        listview_width = ITIDY_MAIN_PROGRESS_MIN_WIDTH;
    }
    listview_height = (font_height + 2) * ITIDY_MAIN_PROGRESS_LIST_ROWS;
    button_height = font_height + 6;

    border_left = screen->WBorLeft;
    border_right = screen->WBorRight;
    border_bottom = screen->WBorBottom;
    border_top = screen->WBorTop + screen->Font->ta_YSize + 1;

    window_data->visual_info = GetVisualInfo(screen, TAG_DONE);
    if (window_data->visual_info == NULL)
    {
        append_to_log("ERROR: GetVisualInfo failed for main progress window\n");
        goto cleanup;
    }

    gad = CreateContext(&window_data->glist);
    if (gad == NULL)
    {
        append_to_log("ERROR: CreateContext failed for main progress window\n");
        goto cleanup;
    }

    if (!add_history_entry_internal(window_data, "Status...", FALSE))
    {
        append_to_log("ERROR: Failed to seed main progress history list\n");
        goto cleanup;
    }

    memset(&ng, 0, sizeof(ng));
    ng.ng_VisualInfo = window_data->visual_info;
    ng.ng_LeftEdge = border_left + ITIDY_MAIN_PROGRESS_MARGIN_X;
    ng.ng_TopEdge = border_top + ITIDY_MAIN_PROGRESS_MARGIN_TOP;
    ng.ng_Width = listview_width;
    ng.ng_Height = listview_height;
    ng.ng_GadgetText = (UBYTE *)"";
    ng.ng_GadgetID = ITIDY_MAIN_PROGRESS_GID_HISTORY;
    ng.ng_Flags = 0;

    window_data->history_listview = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, &window_data->history_entries,
        GTLV_ReadOnly, TRUE,
        GTLV_ShowSelected, NULL,
        TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Failed to create main progress ListView\n");
        goto cleanup;
    }

    memset(&ng, 0, sizeof(ng));
    ng.ng_VisualInfo = window_data->visual_info;
    ng.ng_LeftEdge = window_data->history_listview->LeftEdge;
    ng.ng_TopEdge = window_data->history_listview->TopEdge +
                    window_data->history_listview->Height + ITIDY_MAIN_PROGRESS_SPACE_Y;
    ng.ng_Width = window_data->history_listview->Width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = (UBYTE *)"Cancel";
    ng.ng_GadgetID = ITIDY_MAIN_PROGRESS_GID_CANCEL;
    ng.ng_Flags = PLACETEXT_IN;

    window_data->cancel_button = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Failed to create main progress Cancel button\n");
        goto cleanup;
    }

    content_right = window_data->history_listview->LeftEdge +
                    window_data->history_listview->Width + ITIDY_MAIN_PROGRESS_MARGIN_X;
    content_bottom = window_data->cancel_button->TopEdge +
                     window_data->cancel_button->Height + ITIDY_MAIN_PROGRESS_MARGIN_BOTTOM;

    window_width = content_right + border_right;
    window_height = content_bottom + border_bottom;

    window_left = screen->LeftEdge + (screen->Width - window_width) / 2;
    if (window_left < 0)
    {
        window_left = 0;
    }
    window_top = screen->TopEdge + (screen->Height - window_height) / 2;
    if (window_top < 0)
    {
        window_top = 0;
    }

    window_data->window = OpenWindowTags(NULL,
        WA_Left, window_left,
        WA_Top, window_top,
        WA_Width, window_width,
        WA_Height, window_height,
        WA_Title, (ULONG)ITIDY_MAIN_PROGRESS_TITLE,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, screen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_REFRESHWINDOW,
        WA_Flags, WFLG_SMART_REFRESH | WFLG_RMBTRAP,
        WA_Gadgets, window_data->glist,
        TAG_END);

    if (window_data->window == NULL)
    {
        append_to_log("ERROR: Failed to open main progress window\n");
        goto cleanup;
    }

    window_data->window_open = TRUE;
    window_data->cancel_requested = FALSE;
    success = TRUE;

cleanup:
    if (draw_info != NULL)
    {
        FreeScreenDrawInfo(screen, draw_info);
    }

    if (!success)
    {
        itidy_main_progress_window_close(window_data);
    }

    return success;
}

void itidy_main_progress_window_close(struct iTidyMainProgressWindow *window_data)
{
    if (window_data == NULL)
    {
        return;
    }

    if (window_data->history_listview != NULL && window_data->window != NULL)
    {
        GT_SetGadgetAttrs(window_data->history_listview, window_data->window, NULL,
                          GTLV_Labels, ~0UL,
                          TAG_DONE);
    }

    if (window_data->window != NULL)
    {
        CloseWindow(window_data->window);
        window_data->window = NULL;
    }

    if (window_data->glist != NULL)
    {
        FreeGadgets(window_data->glist);
        window_data->glist = NULL;
    }

    if (window_data->visual_info != NULL)
    {
        FreeVisualInfo(window_data->visual_info);
        window_data->visual_info = NULL;
    }

    free_history_entries(window_data);

    if (window_data->screen != NULL)
    {
        UnlockPubScreen(NULL, window_data->screen);
        window_data->screen = NULL;
    }

    window_data->window_open = FALSE;
    window_data->cancel_requested = FALSE;
    window_data->history_listview = NULL;
    window_data->cancel_button = NULL;
}

BOOL itidy_main_progress_window_handle_events(struct iTidyMainProgressWindow *window_data)
{
    struct IntuiMessage *msg;
    BOOL keep_running = TRUE;

    if (window_data == NULL || window_data->window == NULL)
    {
        return FALSE;
    }

    while (keep_running && (msg = GT_GetIMsg(window_data->window->UserPort)) != NULL)
    {
        ULONG msg_class = msg->Class;
        struct Gadget *gadget = (struct Gadget *)msg->IAddress;
        UWORD gadget_id = gadget ? gadget->GadgetID : 0;

        GT_ReplyIMsg(msg);

        switch (msg_class)
        {
            case IDCMP_CLOSEWINDOW:
                window_data->cancel_requested = TRUE;
                keep_running = FALSE;
                break;

            case IDCMP_GADGETUP:
                if (gadget_id == ITIDY_MAIN_PROGRESS_GID_CANCEL)
                {
                    /* Confirm cancellation with user */
                    BOOL confirmed = ShowEasyRequest(
                        window_data->window,
                        "Confirm Cancel",
                        "Are you sure you want to cancel?\n"
                        "\n"
                        "Note: Changes already made to icons\n"
                        "will NOT be reverted.",
                        "Yes, Cancel|No, Continue"
                    );
                    
                    if (confirmed)
                    {
                        window_data->cancel_requested = TRUE;
                        keep_running = FALSE;
                    }
                    /* If not confirmed, just continue - keep_running stays TRUE */
                }
                break;

            case IDCMP_GADGETDOWN:
                if (gadget_id == ITIDY_MAIN_PROGRESS_GID_HISTORY)
                {
                    GT_RefreshWindow(window_data->window, NULL);
                }
                break;

            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(window_data->window);
                GT_EndRefresh(window_data->window, TRUE);
                break;

            default:
                break;
        }
    }

    return keep_running;
}

BOOL itidy_main_progress_window_append_status(struct iTidyMainProgressWindow *window_data,
                                              const char *status_text)
{
    if (window_data == NULL || status_text == NULL || status_text[0] == '\0')
    {
        return FALSE;
    }

    return add_history_entry_internal(window_data, status_text, TRUE);
}

void itidy_main_progress_window_clear_history(struct iTidyMainProgressWindow *window_data)
{
    if (window_data == NULL)
    {
        return;
    }

    if (window_data->history_listview != NULL && window_data->window != NULL)
    {
        GT_SetGadgetAttrs(window_data->history_listview, window_data->window, NULL,
                          GTLV_Labels, ~0UL,
                          TAG_DONE);
    }

    free_history_entries(window_data);

    if (!add_history_entry_internal(window_data, "Status...", FALSE))
    {
        append_to_log("WARNING: Failed to reset main progress history list\n");
        return;
    }

    if (window_data->history_listview != NULL && window_data->window != NULL)
    {
        GT_SetGadgetAttrs(window_data->history_listview, window_data->window, NULL,
                          GTLV_Labels, &window_data->history_entries,
                          GTLV_Selected, ~0U,
                          TAG_DONE);
    }
}

static BOOL add_history_entry_internal(struct iTidyMainProgressWindow *window_data,
                                       const char *status_text,
                                       BOOL refresh_list)
{
    iTidyMainProgressEntry *entry;
    size_t text_length;

    if (window_data == NULL || status_text == NULL)
    {
        return FALSE;
    }

    entry = (iTidyMainProgressEntry *)whd_malloc(sizeof(iTidyMainProgressEntry));
    if (entry == NULL)
    {
        append_to_log("ERROR: Out of memory allocating progress entry\n");
        return FALSE;
    }
    memset(entry, 0, sizeof(*entry));

    text_length = strlen(status_text);
    entry->text = (char *)whd_malloc(text_length + 1);
    if (entry->text == NULL)
    {
        whd_free(entry);
        append_to_log("ERROR: Out of memory allocating progress text\n");
        return FALSE;
    }

    memcpy(entry->text, status_text, text_length);
    entry->text[text_length] = '\0';
    entry->node.ln_Name = entry->text;
    entry->node.ln_Type = NT_USER;

    AddTail(&window_data->history_entries, (struct Node *)entry);
    window_data->history_count++;

    while (window_data->history_count > ITIDY_MAIN_PROGRESS_MAX_HISTORY)
    {
        struct Node *old_node = window_data->history_entries.lh_Head;
        iTidyMainProgressEntry *old_entry;

        if (old_node == NULL || old_node->ln_Succ == NULL)
        {
            break;
        }

        Remove(old_node);
        old_entry = (iTidyMainProgressEntry *)old_node;

        if (old_entry->text != NULL)
        {
            whd_free(old_entry->text);
        }
        whd_free(old_entry);

        if (window_data->history_count > 0)
        {
            window_data->history_count--;
        }
    }

    if (refresh_list && window_data->history_listview != NULL && window_data->window != NULL)
    {
        GT_SetGadgetAttrs(window_data->history_listview, window_data->window, NULL,
                          GTLV_Labels, &window_data->history_entries,
                          GTLV_MakeVisible, window_data->history_count ? (ULONG)(window_data->history_count - 1) : 0,
                          TAG_DONE);
    }

    return TRUE;
}

static void free_history_entries(struct iTidyMainProgressWindow *window_data)
{
    struct Node *node;
    struct Node *next;

    if (window_data == NULL)
    {
        return;
    }

    node = window_data->history_entries.lh_Head;
    while (node != NULL && node->ln_Succ != NULL)
    {
        next = node->ln_Succ;
        Remove(node);

        if (((iTidyMainProgressEntry *)node)->text != NULL)
        {
            whd_free(((iTidyMainProgressEntry *)node)->text);
        }
        whd_free(node);

        node = next;
    }

    NewList(&window_data->history_entries);
    window_data->history_count = 0;
}

void itidy_main_progress_window_set_button_text(struct iTidyMainProgressWindow *window_data,
                                                const char *text)
{
    /* CRITICAL FIX: Don't dynamically change button text - causes gadget list corruption
     * and Workbench lockups. Use GT_SetGadgetAttrs which is safer but may not always
     * display the change visually on all Workbench versions.
     * 
     * The proper solution would be to create the button with "Close" text initially
     * and just disable it during processing, or use two separate windows.
     */
    
    if (window_data == NULL || window_data->cancel_button == NULL || window_data->window == NULL)
    {
        return;
    }

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    /* Try to change the text using GT_SetGadgetAttrs (safest approach) */
    GT_SetGadgetAttrs(window_data->cancel_button, window_data->window, NULL,
                      GA_Text, (ULONG)text,
                      TAG_DONE);
    
    /* Refresh the window - may or may not show the text change depending on WB version */
    RefreshGList(window_data->cancel_button, window_data->window, NULL, 1);
    GT_RefreshWindow(window_data->window, NULL);
}
