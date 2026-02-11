/*
 * main_progress_window.c - iTidy Main Progress Window implementation (ReAction)
 * Provides a scrolling ListBrowser history with a full-width Cancel button.
 */

#include "platform/platform.h"
#include "main_progress_window.h"
#include "writeLog.h"
#include "../easy_request_helper.h"

#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/button.h>
#include <proto/label.h>
#include <proto/graphics.h>
#include <proto/dos.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <gadgets/button.h>
#include <images/label.h>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define ITIDY_MAIN_PROGRESS_TITLE "iTidy - Progress"
#define ITIDY_MAIN_PROGRESS_MAX_HISTORY 50

struct Library *ListBrowserBase = NULL;
extern struct Library *WindowBase;
extern struct Library *LayoutBase;
extern struct Library *ButtonBase;
/* LabelBase is not used anymore */

void itidy_main_progress_clear_heartbeat(struct iTidyMainProgressWindow *window_data)
{
    if (!window_data || !window_data->status_label_obj) return;
    
    SetGadgetAttrs((struct Gadget *)window_data->status_label_obj, window_data->window, NULL,
        GA_Text, "",
        TAG_DONE);
}

BOOL itidy_main_progress_window_open(struct iTidyMainProgressWindow *window_data)
{
    struct Screen *screen;
    struct DrawInfo *draw_info = NULL;
    BOOL success = FALSE;

    /* Open ReAction classes - WindowBase/LayoutBase/ButtonBase expected from main_window.c */
    /* Only open if not already open (though risky if main closes it while we run, but single threaded) */
    if (!ListBrowserBase) ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 44);
    
    /* Ensure other bases are available */
    if (!WindowBase || !LayoutBase || !ButtonBase)
    {
        /* Try to open them if missing (fallback) */
        if (!WindowBase) WindowBase = OpenLibrary("window.class", 44);
        if (!LayoutBase) LayoutBase = OpenLibrary("gadgets/layout.gadget", 44);
        if (!ButtonBase) ButtonBase = OpenLibrary("gadgets/button.gadget", 44);
    }

    if (!WindowBase || !LayoutBase || !ListBrowserBase || !ButtonBase)
    {
        append_to_log("ERROR: Failed to open ReAction classes\n");
        return FALSE;
    }

    if (window_data == NULL)
    {
        return FALSE;
    }

    memset(window_data, 0, sizeof(*window_data));
    
    /* Default cancel text */
    strncpy(window_data->cancel_button_text, "Cancel", sizeof(window_data->cancel_button_text)-1);

    /* Allocate basic List for ListBrowser (nodes will be added later) */
    window_data->history_list = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
    if (!window_data->history_list)
    {
        append_to_log("ERROR: Failed to allocate history list\n");
        return FALSE;
    }
    NewList(window_data->history_list);

    screen = LockPubScreen(NULL);
    if (screen == NULL)
    {
        append_to_log("ERROR: Failed to lock Workbench screen\n");
        goto cleanup;
    }
    window_data->screen = screen;

    draw_info = GetScreenDrawInfo(screen);
    if (draw_info == NULL)
    {
        append_to_log("ERROR: GetScreenDrawInfo failed\n");
        goto cleanup;
    }

    window_data->window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title,          ITIDY_MAIN_PROGRESS_TITLE,
        WA_Left,           5,
        WA_Top,            20,
        WA_Width,          400,
        WA_Height,         300,
        WA_MinWidth,       300,
        WA_MinHeight,      200,
        WA_MaxWidth,       8192,
        WA_MaxHeight,      8192,
        WA_PubScreen,      screen,
        WINDOW_Position,   WPOS_CENTERMOUSE,
        WA_CloseGadget,    TRUE,
        WA_DepthGadget,    TRUE,
        WA_SizeGadget,     TRUE,
        WA_DragBar,        TRUE,
        WA_Activate,       TRUE,
        WA_NoCareRefresh,  TRUE,
        WA_IDCMP,          IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE,
        
        WINDOW_ParentGroup, window_data->main_layout = NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_SpaceOuter,  TRUE,
            LAYOUT_DeferLayout, TRUE,
            
            LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                GA_ID, 1, /* status_container */
                LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                LAYOUT_LeftSpacing, 2,
                LAYOUT_RightSpacing,2,
                LAYOUT_TopSpacing,  2,
                LAYOUT_BottomSpacing,2,
                
                /* ListBrowser Container */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, 2, /* status_list_container */
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                    LAYOUT_AddChild, window_data->listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
                        GA_ID,                ITIDY_MAIN_PROGRESS_GID_HISTORY,
                        GA_RelVerify,         TRUE,
                        GA_TabCycle,          TRUE,
                        LISTBROWSER_Labels,   window_data->history_list, /* Attach initial empty list */
                        LISTBROWSER_ShowSelected, TRUE,
                        LISTBROWSER_AutoFit,  TRUE,
                        LISTBROWSER_Position, 0,
                    TAG_END),
                TAG_END),
                CHILD_WeightedHeight, 90,
                
                /* Status Text Container */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, 4, /* status_sub_text */
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                    LAYOUT_AddChild, window_data->status_label_obj = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID,                0, /* slow_progress_text */
                        GA_Text,              "Starting, one moment please...",
                        GA_RelVerify,         TRUE,
                        GA_TabCycle,          TRUE,
                        BUTTON_Transparent,   TRUE,
                        BUTTON_TextPen,       1,
                        BUTTON_BackgroundPen, 0,
                        BUTTON_FillTextPen,   1,
                        BUTTON_FillPen,       3,
                        BUTTON_BevelStyle,    BVS_NONE,
                        BUTTON_Justification, BCJ_LEFT,
                    TAG_END),
                TAG_END),
                CHILD_WeightedHeight, 5,
                
                /* Cancel Button Container */
                LAYOUT_AddChild, NewObject(LAYOUT_GetClass(), NULL,
                    GA_ID, 6, /* status_buttons_layout */
                    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                    LAYOUT_AddChild, window_data->cancel_button_obj = NewObject(BUTTON_GetClass(), NULL,
                        GA_ID,                ITIDY_MAIN_PROGRESS_GID_CANCEL,
                        GA_Text,              window_data->cancel_button_text,
                        GA_RelVerify,         TRUE,
                        GA_TabCycle,          TRUE,
                        BUTTON_TextPen,       1,
                        BUTTON_BackgroundPen, 0,
                        BUTTON_FillTextPen,   1,
                        BUTTON_FillPen,       3,
                    TAG_END),
                    CHILD_MinHeight,      16,
                    CHILD_MaxHeight,      28,
                TAG_END),
                CHILD_WeightedHeight, 5,
            TAG_END),
        TAG_END),
    TAG_END);

    if (window_data->window_obj == NULL)
    {
        append_to_log("ERROR: Failed to create main progress window object\n");
        goto cleanup;
    }

    window_data->window = (struct Window *)RA_OpenWindow(window_data->window_obj);
    if (window_data->window == NULL)
    {
        append_to_log("ERROR: Failed to RA_OpenWindow main progress window\n");
        goto cleanup;
    }

    window_data->window_open = TRUE;
    window_data->cancel_requested = FALSE;
    success = TRUE;

    itidy_main_progress_window_append_status(window_data, "Progress window opened.");

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

    if (window_data->window_obj != NULL)
    {
        /* Close and dispose all gadgets */
        DisposeObject(window_data->window_obj);
        window_data->window_obj = NULL;
        window_data->window = NULL;
        window_data->listbrowser_obj = NULL;
        window_data->status_label_obj = NULL;
        window_data->cancel_button_obj = NULL;
    }

    if (window_data->history_list != NULL)
    {
        FreeListBrowserList(window_data->history_list);
        FreeMem(window_data->history_list, sizeof(struct List));
        window_data->history_list = NULL;
    }
    
    if (window_data->screen != NULL)
    {
        UnlockPubScreen(NULL, window_data->screen);
        window_data->screen = NULL;
    }
    
    if (ListBrowserBase) 
    {
        CloseLibrary(ListBrowserBase); 
        ListBrowserBase = NULL;
    }
    
    window_data->window_open = FALSE;
}

BOOL itidy_main_progress_window_handle_events(struct iTidyMainProgressWindow *window_data)
{
    ULONG result, code;
    BOOL keep_running = TRUE;

    if (!window_data || !window_data->window_obj) return FALSE;

    while ((result = RA_HandleInput(window_data->window_obj, &code)) != WMHI_LASTMSG)
    {
        switch (result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                window_data->cancel_requested = TRUE;
                keep_running = FALSE;
                break;

            case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                    case ITIDY_MAIN_PROGRESS_GID_CANCEL:
                        /* Check button text */
                        if (strcmp(window_data->cancel_button_text, "Cancel") == 0)
                        {
                            BOOL confirmed = ShowEasyRequest(
                                window_data->window,
                                "Confirm Cancel",
                                "Are you sure you want to cancel?\n"
                                "Changes will not be reverted.",
                                "Yes, Cancel|No, Continue"
                            );
                            
                            if (confirmed)
                            {
                                window_data->cancel_requested = TRUE;
                                keep_running = FALSE;
                                itidy_main_progress_window_append_status(window_data, "Cancellation requested...");
                            }
                        }
                        else
                        {
                            /* "Close" or similar */
                            window_data->cancel_requested = TRUE;
                            keep_running = FALSE;
                        }
                        break;
                }
                break;
        }
    }
    return keep_running;
}

BOOL itidy_main_progress_window_append_status(struct iTidyMainProgressWindow *window_data, const char *fmt, ...)
{
    va_list args;
    char buffer[256];
    struct Node *node;
    struct Node *old_node;

    if (!window_data || !window_data->history_list || !window_data->listbrowser_obj)
    {
        return FALSE;
    }
    
    /* Handle optional string without formatting if needed, but vsnprintf is safe */
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    node = AllocListBrowserNode(1,
        LBNA_Column, 0,
            LBNCA_CopyText, TRUE,
            LBNCA_Text, buffer,
        TAG_DONE);

    if (!node) return FALSE;

    /* Detach */
    SetGadgetAttrs((struct Gadget *)window_data->listbrowser_obj, window_data->window, NULL,
        LISTBROWSER_Labels, ~0,
        TAG_DONE);

    AddTail(window_data->history_list, node);
    window_data->history_count++;

    while (window_data->history_count > ITIDY_MAIN_PROGRESS_MAX_HISTORY)
    {
        old_node = RemHead(window_data->history_list);
        if (old_node)
        {
            FreeListBrowserNode(old_node);
            window_data->history_count--;
        }
        else break;
    }

    /* Re-attach */
    SetGadgetAttrs((struct Gadget *)window_data->listbrowser_obj, window_data->window, NULL,
        LISTBROWSER_Labels, window_data->history_list,
        LISTBROWSER_MakeVisible, (window_data->history_count > 0 ? window_data->history_count - 1 : 0),
        TAG_DONE);
    
    return TRUE;
}

void itidy_main_progress_window_clear_history(struct iTidyMainProgressWindow *window_data)
{
    if (!window_data || !window_data->history_list) return;
    
    SetGadgetAttrs((struct Gadget *)window_data->listbrowser_obj, window_data->window, NULL,
        LISTBROWSER_Labels, ~0,
        TAG_DONE);
        
    FreeListBrowserList(window_data->history_list);
    NewList(window_data->history_list); /* Re-init empty list */
    window_data->history_count = 0;
    
    SetGadgetAttrs((struct Gadget *)window_data->listbrowser_obj, window_data->window, NULL,
        LISTBROWSER_Labels, window_data->history_list,
        TAG_DONE);
}

void itidy_main_progress_update_heartbeat(struct iTidyMainProgressWindow *window_data,
                                          const char *phase,
                                          LONG current,
                                          LONG total)
{
    char buffer[64];
    
    if (!window_data || !window_data->status_label_obj) return;
    
    if (total > 0)
    {
        snprintf(buffer, sizeof(buffer), "%s: %ld / %ld", phase ? phase : "Processing", current, total);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "%s: %ld", phase ? phase : "Processing", current);
    }
    
    /* Using BUTTON_GetClass for status allows simple SetGadgetAttrs update */
    SetGadgetAttrs((struct Gadget *)window_data->status_label_obj, window_data->window, NULL,
        GA_Text, buffer,
        TAG_DONE);
    
    /* CRITICAL: Pump events so cancel button works during long operations */
    itidy_main_progress_window_handle_events(window_data);
}

void itidy_main_progress_window_set_button_text(struct iTidyMainProgressWindow *window_data, const char *text)
{
    if (!window_data || !window_data->cancel_button_obj || !text) return;
    
    strncpy(window_data->cancel_button_text, text, sizeof(window_data->cancel_button_text)-1);
    
    SetGadgetAttrs((struct Gadget *)window_data->cancel_button_obj, window_data->window, NULL,
        GA_Text, window_data->cancel_button_text,
        TAG_DONE);
}
