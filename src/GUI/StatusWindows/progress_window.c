/*
 * progress_window.c - Simple single-bar progress window implementation
 * Workbench 3.0+ compatible, follows Fast Window Opening pattern
 */

#include "progress_window.h"
#include "progress_common.h"
#include "Settings/IControlPrefs.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <string.h>
#include <stdio.h>

/* External IControl preferences */
extern struct IControlPrefsDetails prefsIControl;

/* Gadget IDs */
#define GID_PROGRESS_CLOSE 1

/* Window dimensions - INNER content area only */
#define PROGRESS_WINDOW_WIDTH  400
#define PROGRESS_WINDOW_HEIGHT 120

/* Layout constants - margins WITHIN the window content area */
#define MARGIN_LEFT   8
#define MARGIN_TOP    8
#define MARGIN_RIGHT  8
#define MARGIN_BOTTOM 8
#define BAR_HEIGHT    18
#define TEXT_SPACING  4

/*
 * Redraw callback for refresh handler
 */
static void RedrawProgressWindow(APTR userData)
{
    iTidy_ProgressWindow *pw = (iTidy_ProgressWindow *)userData;
    iTidy_ProgressPens pens;
    char percent_buf[8];
    UWORD percent;
    
    if (!pw || !pw->window)
        return;
    
    /* Get theme pens */
    if (!iTidy_Progress_GetPens(pw->screen, &pens))
        return;
    
    /* Apply screen font */
    iTidy_Progress_ApplyScreenFont(pw->screen, pw->window->RPort);
    
    /* Draw task label */
    iTidy_Progress_DrawTextLabel(pw->window->RPort, pw->label_x, pw->label_y,
                                  pw->task_label, pens.text_pen);
    
    /* Draw percentage */
    percent = (pw->total_items > 0) ? (pw->current_item * 100) / pw->total_items : 0;
    if (percent > 100) percent = 100;
    sprintf(percent_buf, "%u%%", percent);
    iTidy_Progress_DrawPercentage(pw->window->RPort, pw->percent_x, pw->percent_y,
                                   percent_buf, pens.text_pen);
    
    /* Draw progress bar with bevel */
    iTidy_Progress_DrawBevelBox(pw->window->RPort, pw->bar_x, pw->bar_y,
                                 pw->bar_w, pw->bar_h,
                                 pens.shine_pen, pens.shadow_pen, pens.fill_pen, TRUE);
    
    /* Fill progress bar */
    iTidy_Progress_DrawBarFill(pw->window->RPort, pw->bar_x, pw->bar_y,
                                pw->bar_w, pw->bar_h,
                                pens.bar_pen, pens.fill_pen, percent);
    
    /* Draw helper text if present */
    if (pw->last_helper_text[0] != '\0') {
        iTidy_Progress_DrawTruncatedText(pw->window->RPort, pw->helper_x, pw->helper_y,
                                          pw->last_helper_text, pw->helper_max_width,
                                          TRUE, pens.text_pen);  /* TRUE = path truncation */
    }
}

struct iTidy_ProgressWindow* iTidy_OpenProgressWindow(
    struct Screen *screen,
    const char *task_label,
    UWORD total_items)
{
    iTidy_ProgressWindow *pw;
    struct DrawInfo *dri;
    struct TextFont *font;
    struct RastPort temp_rp;
    UWORD window_width, window_height;
    WORD window_left, window_top;
    
    /* Validate parameters */
    if (!screen || !task_label)
        return NULL;
    
    /* Allocate structure */
    pw = (iTidy_ProgressWindow *)AllocMem(sizeof(iTidy_ProgressWindow), MEMF_CLEAR);
    if (!pw)
        return NULL;
    
    /* Store parameters */
    pw->screen = screen;
    strncpy(pw->task_label, task_label, sizeof(pw->task_label) - 1);
    pw->task_label[sizeof(pw->task_label) - 1] = '\0';
    pw->total_items = total_items;
    pw->current_item = 0;
    pw->completed = FALSE;
    pw->close_button = NULL;
    pw->userCancelled = FALSE;
    
    /* Initialize cached state */
    pw->last_fill_width = 0;
    pw->last_percent = 0;
    pw->last_helper_text[0] = '\0';
    
    /* Get screen's DrawInfo for font measurements */
    dri = GetScreenDrawInfo(screen);
    if (!dri) {
        FreeMem(pw, sizeof(iTidy_ProgressWindow));
        return NULL;
    }
    
    font = dri->dri_Font;
    pw->font_width = font->tf_XSize;
    pw->font_height = font->tf_YSize;
    
    /* Initialize temp RastPort for text measurements */
    InitRastPort(&temp_rp);
    SetFont(&temp_rp, font);
    
    /* Pre-calculate layout positions using IControl preferences for borders */
    /* This follows the same pattern as restore_window.c */
    UWORD content_width = PROGRESS_WINDOW_WIDTH;
    UWORD content_height = PROGRESS_WINDOW_HEIGHT;
    
    /* Task label - top left (border + margin) */
    pw->label_x = prefsIControl.currentLeftBarWidth + MARGIN_LEFT;
    pw->label_y = prefsIControl.currentWindowBarHeight + MARGIN_TOP;
    
    /* Percentage - top right (reserve space for "100%") */
    {
        char temp_percent[] = "100%";
        UWORD percent_width = (UWORD)TextLength(&temp_rp, temp_percent, 4);
        pw->percent_x = prefsIControl.currentLeftBarWidth + content_width - MARGIN_RIGHT;
        pw->percent_y = prefsIControl.currentWindowBarHeight + MARGIN_TOP;
    }
    
    /* Progress bar - centered below labels */
    pw->bar_x = prefsIControl.currentLeftBarWidth + MARGIN_LEFT;
    pw->bar_y = prefsIControl.currentWindowBarHeight + MARGIN_TOP + pw->font_height + TEXT_SPACING * 2;
    pw->bar_w = content_width - MARGIN_LEFT - MARGIN_RIGHT;
    pw->bar_h = BAR_HEIGHT;
    
    /* Helper text - below progress bar */
    pw->helper_x = prefsIControl.currentLeftBarWidth + MARGIN_LEFT;
    pw->helper_y = pw->bar_y + pw->bar_h + TEXT_SPACING * 2;
    pw->helper_max_width = content_width - MARGIN_LEFT - MARGIN_RIGHT;
    
    FreeScreenDrawInfo(screen, dri);
    
    /* Calculate total window size including borders */
    window_width = prefsIControl.currentLeftBarWidth + content_width + prefsIControl.currentLeftBarWidth;
    window_height = prefsIControl.currentWindowBarHeight + content_height;
    
    /* Calculate centered window position */
    window_left = (screen->Width - window_width) / 2;
    window_top = (screen->Height - window_height) / 2;
    
    /* Open window IMMEDIATELY (no slow operations before this!) */
    /* Use WA_Width/Height to specify total window size including borders */
    pw->window = OpenWindowTags(NULL,
        WA_Left, window_left,
        WA_Top, window_top,
        WA_Width, window_width,
        WA_Height, window_height,
        WA_Title, task_label,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, screen,
        WA_IDCMP, IDCMP_REFRESHWINDOW | IDCMP_INTUITICKS,
        WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_SMART_REFRESH | WFLG_ACTIVATE | WFLG_RMBTRAP,
        TAG_END);
    
    if (!pw->window) {
        FreeMem(pw, sizeof(iTidy_ProgressWindow));
        return NULL;
    }
    
    /* Set busy pointer IMMEDIATELY after window opens */
    SetWindowPointer(pw->window,
                     WA_BusyPointer, TRUE,
                     TAG_END);
    
    /* Draw initial empty state */
    RedrawProgressWindow(pw);
    
    return pw;
}

void iTidy_UpdateProgress(
    struct iTidy_ProgressWindow *pw,
    UWORD current_item,
    const char *helper_text)
{
    iTidy_ProgressPens pens;
    UWORD new_percent, new_fill_width;
    char percent_buf[8];
    BOOL bar_changed = FALSE;
    BOOL percent_changed = FALSE;
    BOOL helper_changed = FALSE;
    
    if (!pw || !pw->window)
        return;
    
    /* Update current item */
    pw->current_item = current_item;
    
    /* Handle pending refresh events first */
    iTidy_Progress_HandleRefresh(pw->window, RedrawProgressWindow, pw);
    
    /* Get theme pens */
    if (!iTidy_Progress_GetPens(pw->screen, &pens))
        return;
    
    /* Apply screen font */
    iTidy_Progress_ApplyScreenFont(pw->screen, pw->window->RPort);
    
    /* Calculate new percentage */
    new_percent = (pw->total_items > 0) ? (current_item * 100) / pw->total_items : 0;
    if (new_percent > 100)
        new_percent = 100;
    
    /* Calculate new fill width */
    {
        WORD interior_w = pw->bar_w - 4;  /* Inside 2px bevel */
        new_fill_width = (interior_w * new_percent) / 100;
    }
    
    /* Check what changed */
    bar_changed = (new_fill_width != pw->last_fill_width);
    percent_changed = (new_percent != pw->last_percent);
    
    if (helper_text) {
        helper_changed = (strcmp(helper_text, pw->last_helper_text) != 0);
    }
    
    /* Update progress bar if fill width changed */
    if (bar_changed) {
        iTidy_Progress_DrawBarFill(pw->window->RPort, pw->bar_x, pw->bar_y,
                                    pw->bar_w, pw->bar_h,
                                    pens.bar_pen, pens.fill_pen, new_percent);
        pw->last_fill_width = new_fill_width;
    }
    
    /* Update percentage text if value changed */
    if (percent_changed) {
        UWORD old_text_width, new_text_width;
        char old_percent_buf[8];
        
        /* Calculate old text width for clearing */
        sprintf(old_percent_buf, "%u%%", pw->last_percent);
        old_text_width = (UWORD)TextLength(pw->window->RPort, old_percent_buf,
                                           (LONG)strlen(old_percent_buf));
        
        /* Clear old percentage text */
        iTidy_Progress_ClearTextArea(pw->window->RPort,
                                      pw->percent_x - old_text_width,
                                      pw->percent_y,
                                      old_text_width, pw->font_height,
                                      pens.fill_pen);
        
        /* Draw new percentage */
        sprintf(percent_buf, "%u%%", new_percent);
        iTidy_Progress_DrawPercentage(pw->window->RPort, pw->percent_x, pw->percent_y,
                                       percent_buf, pens.text_pen);
        
        pw->last_percent = new_percent;
    }
    
    /* Update helper text if changed */
    if (helper_changed && helper_text) {
        /* Clear old helper text */
        iTidy_Progress_ClearTextArea(pw->window->RPort,
                                      pw->helper_x, pw->helper_y,
                                      pw->helper_max_width, pw->font_height,
                                      pens.fill_pen);
        
        /* Draw new helper text with truncation if needed */
        iTidy_Progress_DrawTruncatedText(pw->window->RPort, pw->helper_x, pw->helper_y,
                                          helper_text, pw->helper_max_width,
                                          TRUE, pens.text_pen);  /* TRUE = path truncation */
        
        /* Cache for next comparison */
        strncpy(pw->last_helper_text, helper_text, sizeof(pw->last_helper_text) - 1);
        pw->last_helper_text[sizeof(pw->last_helper_text) - 1] = '\0';
    }
}

void iTidy_ShowCompletionState(
    struct iTidy_ProgressWindow *pw,
    BOOL success)
{
    iTidy_ProgressPens pens;
    const char *status_text;
    struct Gadget *gad;
    UWORD button_width, button_height;
    WORD button_x, button_y;
    
    if (!pw || !pw->window)
        return;
    
    /* Clear busy pointer - operation is complete */
    SetWindowPointer(pw->window,
                     WA_Pointer, NULL,
                     TAG_END);
    
    /* Get theme pens */
    if (!iTidy_Progress_GetPens(pw->screen, &pens))
        return;
    
    /* Apply screen font */
    iTidy_Progress_ApplyScreenFont(pw->screen, pw->window->RPort);
    
    /* Update status text */
    status_text = success ? "Complete!" : "Failed";
    
    /* Clear old helper text */
    iTidy_Progress_ClearTextArea(pw->window->RPort,
                                  pw->helper_x, pw->helper_y,
                                  pw->helper_max_width, pw->font_height,
                                  pens.fill_pen);
    
    /* Draw completion status (use FALSE for non-path text) */
    iTidy_Progress_DrawTruncatedText(pw->window->RPort, pw->helper_x, pw->helper_y,
                                      status_text, pw->helper_max_width,
                                      FALSE, pens.text_pen);  /* FALSE = end truncation */
    
    /* Update cached helper text */
    strncpy(pw->last_helper_text, status_text, sizeof(pw->last_helper_text) - 1);
    pw->last_helper_text[sizeof(pw->last_helper_text) - 1] = '\0';
    
    /* Calculate Close button dimensions */
    button_width = pw->bar_w - 16;  /* Slightly narrower than bar */
    button_height = pw->font_height + 6;
    button_x = (pw->window->Width - button_width) / 2;  /* Centered */
    button_y = pw->bar_y;
    
    /* Clear progress bar area */
    iTidy_Progress_ClearTextArea(pw->window->RPort,
                                  pw->bar_x, pw->bar_y,
                                  pw->bar_w, pw->bar_h,
                                  pens.fill_pen);
    
    /* Create visual context for gadgets */
    gad = CreateContext(&pw->close_button);
    if (!gad)
        return;
    
    /* Create Close button */
    {
        struct NewGadget ng;
        
        ng.ng_LeftEdge = button_x;
        ng.ng_TopEdge = button_y;
        ng.ng_Width = button_width;
        ng.ng_Height = button_height;
        ng.ng_GadgetText = (UBYTE *)"Close";
        ng.ng_TextAttr = pw->screen->Font;
        ng.ng_GadgetID = GID_PROGRESS_CLOSE;
        ng.ng_Flags = PLACETEXT_IN;
        ng.ng_VisualInfo = GetVisualInfo(pw->screen, TAG_END);
        
        gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
        
        if (ng.ng_VisualInfo)
            FreeVisualInfo(ng.ng_VisualInfo);
    }
    
    if (!pw->close_button)
        return;
    
    /* Add gadgets to window and enable GADGETUP events */
    AddGList(pw->window, pw->close_button, (UWORD)-1, (UWORD)-1, NULL);
    RefreshGList(pw->close_button, pw->window, NULL, (UWORD)-1);
    GT_RefreshWindow(pw->window, NULL);
    
    /* Modify IDCMP to receive gadget events */
    ModifyIDCMP(pw->window, IDCMP_REFRESHWINDOW | IDCMP_GADGETUP);
    
    pw->completed = TRUE;
}

BOOL iTidy_HandleProgressWindowEvents(
    struct iTidy_ProgressWindow *pw)
{
    struct IntuiMessage *msg;
    BOOL keep_open = TRUE;
    
    if (!pw || !pw->window)
        return FALSE;
    
    /* Process all pending messages */
    while ((msg = GT_GetIMsg(pw->window->UserPort)) != NULL) {
        ULONG msg_class = msg->Class;
        UWORD gadget_id = 0;
        
        if (msg_class == IDCMP_GADGETUP) {
            gadget_id = ((struct Gadget *)msg->IAddress)->GadgetID;
        }
        
        GT_ReplyIMsg(msg);
        
        /* Handle events */
        if (msg_class == IDCMP_REFRESHWINDOW) {
            /* Redraw window contents */
            GT_BeginRefresh(pw->window);
            RedrawProgressWindow(pw);
            GT_EndRefresh(pw->window, TRUE);
        }
        else if (msg_class == IDCMP_GADGETUP && gadget_id == GID_PROGRESS_CLOSE) {
            keep_open = FALSE;  /* User clicked Close */
        }
    }
    
    return keep_open;
}

void iTidy_CloseProgressWindow(
    struct iTidy_ProgressWindow *pw)
{
    if (!pw)
        return;
    
    if (pw->window) {
        /* Remove gadgets if present */
        if (pw->close_button) {
            RemoveGList(pw->window, pw->close_button, (UWORD)-1);
            FreeGadgets(pw->close_button);
            pw->close_button = NULL;
        }
        
        /* Clear busy pointer if still set */
        SetWindowPointer(pw->window,
                         WA_Pointer, NULL,
                         TAG_END);
        
        CloseWindow(pw->window);
        pw->window = NULL;
    }
    
    FreeMem(pw, sizeof(iTidy_ProgressWindow));
}
