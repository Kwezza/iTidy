/*
 * gui_groupbox.c - Workbench 2/3 Group Box Helper Implementation
 * 
 * Provides reusable pattern for drawing Workbench-style group frames
 * around GadTools gadgets with font-aware layout.
 */

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/text.h>
#include <libraries/gadtools.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>

#include "gui_groupbox.h"
#include "writeLog.h"

#include <stdio.h>

/*------------------------------------------------------------------------*/
/* Configurable padding values for group box appearance                  */
/*------------------------------------------------------------------------*/
#define GROUPBOX_PADDING_LEFT    0   /* Left edge padding (pixels) */
#define GROUPBOX_PADDING_RIGHT   4   /* Right edge padding (pixels) */
#define GROUPBOX_PADDING_TOP     11   /* Top edge padding (pixels) */
#define GROUPBOX_PADDING_BOTTOM  10   /* Bottom edge padding (pixels) */

#define GROUPBOX_TITLE_BAND_EXTRA 2 /* Extra height for title band */

/*------------------------------------------------------------------------*/
/**
 * @brief Calculate a font-aware group box rectangle
 * 
 * Computes the bounding rectangle for a group of gadgets from first to
 * last in the NextGadget chain, with margins and title band space
 * proportional to the current screen font metrics.
 */
/*------------------------------------------------------------------------*/
VOID CalcGadgetGroupBoxRect(struct Window    *win,
                            struct Gadget    *first,
                            struct Gadget    *last,
                            struct Rectangle *box)
{
    struct RastPort *rp  = win->RPort;
    struct Screen   *scr = win->WScreen;
    struct Gadget   *g;
    WORD minX, minY, maxX, maxY;
    WORD fh;          /* font height      */
    WORD hMargin;     /* horizontal pad   */
    WORD innerPad;    /* inside top/bot   */
    WORD titleBandH;  /* vertical height reserved for title band */

    if (!win || !first || !last || !box)
    {
        log_error(LOG_GUI, "CalcGadgetGroupBoxRect: NULL parameter\n");
        return;
    }

    log_debug(LOG_GUI, "CalcGadgetGroupBoxRect: Calculating box for gadgets %p to %p\n", first, last);

    /* Union of gadget rects */
    minX =  32767; minY =  32767;
    maxX = -32768; maxY = -32768;

    for (g = first; g; g = g->NextGadget)
    {
        WORD l = g->LeftEdge;
        WORD t = g->TopEdge;
        WORD r = g->LeftEdge + g->Width  - 1;
        WORD b = g->TopEdge  + g->Height - 1;

        log_debug(LOG_GUI, "  Gadget at (%d,%d) size %dx%d (right=%d, bottom=%d)\n",
                 l, t, g->Width, g->Height, r, b);

        if (l < minX) minX = l;
        if (t < minY) minY = t;
        if (r > maxX) maxX = r;
        if (b > maxY) maxY = b;

        if (g == last)
            break;
    }

    log_debug(LOG_GUI, "CalcGadgetGroupBoxRect: Gadget union = (%d,%d) to (%d,%d)\n",
             minX, minY, maxX, maxY);

    /* Use existing RastPort font - it's already set to screen font */
    fh        = rp->TxHeight;
    hMargin   = fh / 2;
    innerPad  = fh / 4;  /* Reduced from fh/3 for tighter fit */
    titleBandH = fh + 4;   /* strip where the caption sits */

    /* Calculate box bounds:
     *  - Use equal padding top and bottom for centered appearance
     *  - Add configurable padding values
     */
    box->MinX = minX - hMargin - GROUPBOX_PADDING_LEFT;
    box->MaxX = maxX + hMargin + GROUPBOX_PADDING_RIGHT;
    box->MinY = minY - innerPad - GROUPBOX_PADDING_TOP;  /* Equal padding above gadgets */
    box->MaxY = maxY + innerPad + GROUPBOX_PADDING_BOTTOM;  
    
    log_debug(LOG_GUI, "CalcGadgetGroupBoxRect: innerPad=%d, GROUPBOX_PADDING_TOP=%d\n",
             innerPad, GROUPBOX_PADDING_TOP);
    log_debug(LOG_GUI, "CalcGadgetGroupBoxRect: Top spacing = minY(%d) - innerPad(%d) - PADDING_TOP(%d) = %d\n",
             minY, innerPad, GROUPBOX_PADDING_TOP, box->MinY);
    log_debug(LOG_GUI, "CalcGadgetGroupBoxRect: Bottom spacing = maxY(%d) + innerPad(%d) + PADDING_TOP(%d) = %d\n",
             maxY, innerPad, GROUPBOX_PADDING_TOP, box->MaxY);
    log_debug(LOG_GUI, "CalcGadgetGroupBoxRect: Box = (%d,%d) to (%d,%d), fh=%d\n",
             box->MinX, box->MinY, box->MaxX, box->MaxY, fh);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Draw a Workbench-style group box with centered label
 * 
 * Renders a recessed bevel frame and centered caption using current
 * screen font and DrawInfo pens for proper Workbench appearance.
 */
/*------------------------------------------------------------------------*/
VOID DrawGroupBoxWithLabel(struct Window    *win,
                           APTR              vi,
                           struct Rectangle *box,
                           CONST_STRPTR      label)
{
    struct RastPort *rp  = win->RPort;
    struct Screen   *scr = win->WScreen;
    struct DrawInfo *dri;
    struct IntuiText itext;
    WORD fh, fb;
    WORD boxW, boxH;
    WORD textW, textX, textY;
    WORD titleBandH;

    if (!win || !box || !label)
    {
        log_error(LOG_GUI, "DrawGroupBoxWithLabel: NULL parameter\n");
        return;
    }

    log_debug(LOG_GUI, "DrawGroupBoxWithLabel: Drawing '%s' at (%d,%d) to (%d,%d)\n",
             label, box->MinX, box->MinY, box->MaxX, box->MaxY);

    /* Get DrawInfo for screen pens */
    if ((dri = GetScreenDrawInfo(scr)) == NULL)
    {
        log_error(LOG_GUI, "DrawGroupBoxWithLabel: Failed to get DrawInfo\n");
        return;
    }

    /* Use existing RastPort font - it's already set to screen font */
    fh        = rp->TxHeight;
    fb        = rp->TxBaseline;
    titleBandH = fh + 4;

    boxW = box->MaxX - box->MinX + 1;
    boxH = box->MaxY - box->MinY + 1;

    /* Draw the double-line rectangle frame (Workbench 3.2 style) */
    log_debug(LOG_GUI, "DrawGroupBoxWithLabel: Drawing double-line rectangle %dx%d at (%d,%d)\n",
             boxW, boxH, box->MinX, box->MinY);
    
    /* Draw white rectangle first, offset by 1 pixel down and right (SHINEPEN) */
    SetAPen(rp, dri->dri_Pens[SHINEPEN]);
    Move(rp, box->MinX + 1, box->MinY + 1);
    Draw(rp, box->MaxX + 1, box->MinY + 1); /* Top */
    Draw(rp, box->MaxX + 1, box->MaxY + 1); /* Right */
    Draw(rp, box->MinX + 1, box->MaxY + 1); /* Bottom */
    Draw(rp, box->MinX + 1, box->MinY + 1); /* Left */
    
    /* Draw black rectangle over it at base coordinates (SHADOWPEN) */
    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
    Move(rp, box->MinX, box->MinY);
    Draw(rp, box->MaxX, box->MinY);         /* Top */
    Draw(rp, box->MaxX, box->MaxY);         /* Right */
    Draw(rp, box->MinX, box->MaxY);         /* Bottom */
    Draw(rp, box->MinX, box->MinY);         /* Left */
    
    log_debug(LOG_GUI, "DrawGroupBoxWithLabel: Double-line rectangle drawn\n");

    /* Set up IntuiText for label */
    itext.FrontPen  = dri->dri_Pens[TEXTPEN];
    itext.BackPen   = dri->dri_Pens[BACKGROUNDPEN];
    itext.DrawMode  = JAM2;
    itext.LeftEdge  = 0;
    itext.TopEdge   = 0;
    itext.ITextFont = NULL;
    itext.IText     = (UBYTE *)label;
    itext.NextText  = NULL;

    textW = IntuiTextLength(&itext);

        /* Centre text horizontally on the box */
        textX = box->MinX + (boxW - textW) / 2;
        
        /* Position text baseline so the visual center of the text aligns with box->MinY:
         * If we want the center of text height at box->MinY, and baseline is 'fb' from top,
         * then: textY (baseline) = box->MinY - fb + (fh / 2) - GROUPBOX_TITLE_BAND_EXTRA
         * Subtract 1 extra pixel to move text slightly higher for better visual alignment
         */
        textY = box->MinY - fb + (fh / 2) - GROUPBOX_TITLE_BAND_EXTRA;
        
        log_debug(LOG_GUI, "DrawGroupBoxWithLabel: Font metrics: fh=%d, fb=%d\n", fh, fb);
        log_debug(LOG_GUI, "DrawGroupBoxWithLabel: Text positioning: box->MinY=%d, fh/2=%d, textY=%d\n",
                 box->MinY, fh / 2, textY);
        log_debug(LOG_GUI, "DrawGroupBoxWithLabel: Calculation: %d - %d + %d - GROUPBOX_TITLE_BAND_EXTRA = %d\n",
                 box->MinY, fb, fh / 2, textY);
        
        /* Clear background behind text (create gap in top border for text) */
        /* The clear area must be centered on the top line of the box */
        SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
        RectFill(rp,
                 textX - 2,                    /* Start just before text */
                 box->MinY - (fh / 2) - 1,     /* Half font height above top line */
                 textX + textW + 1,            /* End just after text */
                 box->MinY + (fh / 2) + 1);    /* Half font height below top line */
        
        log_debug(LOG_GUI, "DrawGroupBoxWithLabel: RectFill from Y=%d to Y=%d\n",
                 box->MinY - (fh / 2) - 1, box->MinY + (fh / 2) + 1);

        /* Optional: shadow then highlight text, for WB look */
        //itext.FrontPen = dri->dri_Pens[SHADOWPEN];
        //PrintIText(rp, &itext, textX + 1, textY + 1);

        itext.FrontPen = dri->dri_Pens[HIGHLIGHTTEXTPEN];
        PrintIText(rp, &itext, textX, textY);
        
        log_debug(LOG_GUI, "DrawGroupBoxWithLabel: Text '%s' drawn at (%d,%d)\n",
                 label, textX, textY);

        FreeScreenDrawInfo(scr, dri);
    
    log_debug(LOG_GUI, "DrawGroupBoxWithLabel: Complete\n");
}
