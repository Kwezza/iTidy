/*
 * progress_common.h - Shared drawing and refresh utilities for status windows
 * Workbench 3.0+ compatible (uses DrawInfo pens, TextLength measurements)
 */

#ifndef ITIDY_PROGRESS_COMMON_H
#define ITIDY_PROGRESS_COMMON_H

#include <exec/types.h>

/* Forward declarations to avoid heavy header dependencies here */
struct Screen;
struct RastPort;
struct Window;

/* C linkage only; this project builds as C on Amiga */

/* Pens bundle for consistent theming */
typedef struct iTidy_ProgressPens {
    ULONG shine_pen;     /* Bright edge */
    ULONG shadow_pen;    /* Dark edge */
    ULONG fill_pen;      /* Background inside bevel */
    ULONG bar_pen;       /* Progress fill */
    ULONG text_pen;      /* Text color */
} iTidy_ProgressPens;

/* Retrieve pens from the screen's DrawInfo (returns FALSE on failure). */
BOOL iTidy_Progress_GetPens(struct Screen *screen, iTidy_ProgressPens *pens);

/* Apply the Workbench screen font to the given RastPort (returns FALSE on failure). */
BOOL iTidy_Progress_ApplyScreenFont(struct Screen *screen, struct RastPort *rp);

/* Draw a 2px beveled box (recessed=TRUE for sunken progress container). */
void iTidy_Progress_DrawBevelBox(
    struct RastPort *rp,
    WORD left, WORD top, WORD width, WORD height,
    ULONG shine_pen, ULONG shadow_pen, ULONG fill_pen,
    BOOL recessed);

/* Fill the beveled bar interior to the given percentage (0..100). */
void iTidy_Progress_DrawBarFill(
    struct RastPort *rp,
    WORD left, WORD top, WORD width, WORD height,
    ULONG bar_pen, ULONG fill_pen,
    UWORD percent);

/* Clear a rectangular area (used before drawing updated text). */
void iTidy_Progress_ClearTextArea(
    struct RastPort *rp,
    WORD left, WORD top,
    UWORD rect_width, UWORD rect_height,
    ULONG bg_pen);

/* Draw simple text label at baseline-aligned position. */
void iTidy_Progress_DrawTextLabel(
    struct RastPort *rp,
    WORD left, WORD top,
    const char *text,
    ULONG text_pen);

/* Draw right-aligned percentage text with proper measurement. */
void iTidy_Progress_DrawPercentage(
    struct RastPort *rp,
    WORD right_x,
    WORD top,
    const char *percent_text,
    ULONG text_pen);

/* Draw text with smart truncation if it exceeds max_width.
 * is_path=TRUE: Middle truncation (e.g., "Work:Programs/.../Tools")
 * is_path=FALSE: End truncation (e.g., "Processing item...") */
void iTidy_Progress_DrawTruncatedText(
    struct RastPort *rp,
    WORD left, WORD top,
    const char *text,
    UWORD max_width,
    BOOL is_path,
    ULONG text_pen);

/* Redraw callback signature used by refresh handler */
typedef void (*iTidy_Progress_RedrawFunc)(APTR userData);

/* Handle pending IDCMP_REFRESHWINDOW events for SMART_REFRESH windows. */
void iTidy_Progress_HandleRefresh(struct Window *win, iTidy_Progress_RedrawFunc redraw, APTR userData);

/* end header */

#endif /* ITIDY_PROGRESS_COMMON_H */
