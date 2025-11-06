/*
 * progress_common.c - Shared drawing and refresh utilities for status windows
 * Workbench 3.0+ compatible implementation
 */

#include "progress_common.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <string.h>

BOOL iTidy_Progress_GetPens(struct Screen *screen, iTidy_ProgressPens *pens)
{
    struct DrawInfo *dri;

    if (!screen || !pens)
        return FALSE;

    dri = GetScreenDrawInfo(screen);
    if (!dri)
        return FALSE;

    /* Map from Workbench DrawInfo pens */
    pens->shine_pen  = (ULONG)dri->dri_Pens[SHINEPEN];
    pens->shadow_pen = (ULONG)dri->dri_Pens[SHADOWPEN];
    pens->fill_pen   = (ULONG)dri->dri_Pens[BACKGROUNDPEN];
    pens->bar_pen    = (ULONG)dri->dri_Pens[FILLPEN];
    pens->text_pen   = (ULONG)dri->dri_Pens[TEXTPEN];

    FreeScreenDrawInfo(screen, dri);
    return TRUE;
}

BOOL iTidy_Progress_ApplyScreenFont(struct Screen *screen, struct RastPort *rp)
{
    struct DrawInfo *dri;

    if (!screen || !rp)
        return FALSE;

    dri = GetScreenDrawInfo(screen);
    if (!dri)
        return FALSE;

    /* Use the screen's font for accurate measurements and drawing */
    SetFont(rp, dri->dri_Font);

    FreeScreenDrawInfo(screen, dri);
    return TRUE;
}

void iTidy_Progress_DrawBevelBox(
    struct RastPort *rp,
    WORD left, WORD top, WORD width, WORD height,
    ULONG shine_pen, ULONG shadow_pen, ULONG fill_pen,
    BOOL recessed)
{
    if (!rp || width <= 0 || height <= 0)
        return;

    /* Outer bevel - 2 pixels wide */
    SetAPen(rp, recessed ? shadow_pen : shine_pen);
    Move(rp, left, top + height - 1);
    Draw(rp, left, top);
    Draw(rp, left + width - 1, top);

    SetAPen(rp, recessed ? shine_pen : shadow_pen);
    Draw(rp, left + width - 1, top + height - 1);
    Draw(rp, left, top + height - 1);

    /* Inner bevel - 1 pixel inside */
    SetAPen(rp, recessed ? shadow_pen : shine_pen);
    Move(rp, left + 1, top + height - 2);
    Draw(rp, left + 1, top + 1);
    Draw(rp, left + width - 2, top + 1);

    SetAPen(rp, recessed ? shine_pen : shadow_pen);
    Draw(rp, left + width - 2, top + height - 2);
    Draw(rp, left + 1, top + height - 2);

    /* Fill interior */
    SetAPen(rp, fill_pen);
    RectFill(rp, left + 2, top + 2, left + width - 3, top + height - 3);
}

void iTidy_Progress_DrawBarFill(
    struct RastPort *rp,
    WORD left, WORD top, WORD width, WORD height,
    ULONG bar_pen, ULONG fill_pen,
    UWORD percent)
{
    WORD interior_w, fill_w;

    if (!rp || width <= 4 || height <= 4)
        return;

    if (percent > 100)
        percent = 100;

    /* Inside 2px bevel */
    interior_w = width - 4;
    fill_w = (interior_w * (WORD)percent) / 100;

    if (fill_w > 0)
    {
        SetAPen(rp, bar_pen);
        RectFill(rp, left + 2, top + 2, left + 2 + fill_w - 1, top + height - 3);
    }

    if (fill_w < interior_w)
    {
        SetAPen(rp, fill_pen);
        RectFill(rp, left + 2 + fill_w, top + 2, left + width - 3, top + height - 3);
    }
}

void iTidy_Progress_ClearTextArea(
    struct RastPort *rp,
    WORD left, WORD top,
    UWORD rect_width, UWORD rect_height,
    ULONG bg_pen)
{
    if (!rp || rect_width == 0 || rect_height == 0)
        return;

    SetAPen(rp, bg_pen);
    RectFill(rp, left, top, left + rect_width - 1, top + rect_height - 1);
}

void iTidy_Progress_DrawTextLabel(
    struct RastPort *rp,
    WORD left, WORD top,
    const char *text,
    ULONG text_pen)
{
    if (!rp || !text)
        return;

    SetAPen(rp, text_pen);
    Move(rp, left, top + rp->Font->tf_Baseline);
    Text(rp, (STRPTR)text, (LONG)strlen(text));
}

void iTidy_Progress_DrawPercentage(
    struct RastPort *rp,
    WORD right_x,
    WORD top,
    const char *percent_text,
    ULONG text_pen)
{
    UWORD text_width;

    if (!rp || !percent_text)
        return;

    text_width = (UWORD)TextLength(rp, (STRPTR)percent_text, (LONG)strlen(percent_text));

    SetAPen(rp, text_pen);
    Move(rp, right_x - (WORD)text_width, top + rp->Font->tf_Baseline);
    Text(rp, (STRPTR)percent_text, (LONG)strlen(percent_text));
}

void iTidy_Progress_DrawTruncatedText(
    struct RastPort *rp,
    WORD left, WORD top,
    const char *text,
    UWORD max_width,
    BOOL is_path,
    ULONG text_pen)
{
    char buffer[256];
    UWORD text_width;
    ULONG text_len;
    const char *ellipsis = "...";
    UWORD ellipsis_width;
    
    if (!rp || !text)
        return;
    
    text_len = strlen(text);
    
    /* Check if text fits as-is */
    text_width = (UWORD)TextLength(rp, (STRPTR)text, (LONG)text_len);
    if (text_width <= max_width)
    {
        /* Fits fine - draw as normal */
        SetAPen(rp, text_pen);
        Move(rp, left, top + rp->Font->tf_Baseline);
        Text(rp, (STRPTR)text, (LONG)text_len);
        return;
    }
    
    /* Text needs truncation */
    ellipsis_width = (UWORD)TextLength(rp, (STRPTR)ellipsis, 3);
    
    if (is_path)
    {
        /* Middle truncation for paths: "Work:Programs/.../Tools" */
        ULONG start_chars, end_chars;
        UWORD target_width = max_width - ellipsis_width;
        ULONG half_target = target_width / 2;
        
        /* Find how many characters fit in first half */
        for (start_chars = 0; start_chars < text_len; start_chars++)
        {
            UWORD w = (UWORD)TextLength(rp, (STRPTR)text, (LONG)start_chars);
            if (w >= half_target)
                break;
        }
        
        /* Find how many characters fit in second half (measure from end) */
        for (end_chars = 0; end_chars < text_len; end_chars++)
        {
            const char *end_start = text + text_len - end_chars;
            UWORD w = (UWORD)TextLength(rp, (STRPTR)end_start, (LONG)end_chars);
            if (w >= half_target)
                break;
        }
        
        /* Safety check - ensure we don't overflow buffer */
        if (start_chars + 3 + end_chars >= sizeof(buffer))
        {
            start_chars = 50;  /* Fallback to safe values */
            end_chars = 50;
        }
        
        /* Build truncated string: "start...end" */
        if (start_chars > 0)
            strncpy(buffer, text, start_chars);
        strcpy(buffer + start_chars, ellipsis);
        if (end_chars > 0)
            strcpy(buffer + start_chars + 3, text + text_len - end_chars);
        
        buffer[sizeof(buffer) - 1] = '\0';
    }
    else
    {
        /* End truncation for normal text: "Processing item..." */
        ULONG fit_chars;
        UWORD target_width = max_width - ellipsis_width;
        
        /* Find how many characters fit before ellipsis */
        for (fit_chars = 0; fit_chars < text_len; fit_chars++)
        {
            UWORD w = (UWORD)TextLength(rp, (STRPTR)text, (LONG)fit_chars);
            if (w >= target_width)
                break;
        }
        
        /* Safety check */
        if (fit_chars + 3 >= sizeof(buffer))
            fit_chars = sizeof(buffer) - 4;
        
        /* Build truncated string: "text..." */
        if (fit_chars > 0)
            strncpy(buffer, text, fit_chars);
        strcpy(buffer + fit_chars, ellipsis);
        buffer[sizeof(buffer) - 1] = '\0';
    }
    
    /* Draw the truncated text */
    SetAPen(rp, text_pen);
    Move(rp, left, top + rp->Font->tf_Baseline);
    Text(rp, (STRPTR)buffer, (LONG)strlen(buffer));
}

void iTidy_Progress_HandleRefresh(struct Window *win, iTidy_Progress_RedrawFunc redraw, APTR userData)
{
    struct IntuiMessage *msg;

    if (!win)
        return;

    /* Non-blocking drain of refresh messages */
    while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)) != NULL)
    {
        ULONG cls = msg->Class;
        ReplyMsg((struct Message *)msg);

        if (cls == IDCMP_REFRESHWINDOW)
        {
            GT_BeginRefresh(win);
            if (redraw)
            {
                redraw(userData);
            }
            GT_EndRefresh(win, TRUE);
        }
    }
}
