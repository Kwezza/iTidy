/*
 * main_progress_window.h - iTidy Main Progress Window
 * Displays scrolling status history with Cancel control
 */

#ifndef ITIDY_MAIN_PROGRESS_WINDOW_H
#define ITIDY_MAIN_PROGRESS_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>

/* Gadget identifiers */
#define ITIDY_MAIN_PROGRESS_GID_HISTORY  4201
#define ITIDY_MAIN_PROGRESS_GID_CANCEL   4202

/* Layout characteristics */
#define ITIDY_MAIN_PROGRESS_LIST_ROWS        8
#define ITIDY_MAIN_PROGRESS_MARGIN_X        15
#define ITIDY_MAIN_PROGRESS_MARGIN_TOP      12
#define ITIDY_MAIN_PROGRESS_MARGIN_BOTTOM   12
#define ITIDY_MAIN_PROGRESS_SPACE_Y          8
#define ITIDY_MAIN_PROGRESS_HEARTBEAT_PAD   4
#define ITIDY_MAIN_PROGRESS_WIDTH_CHARS     60
#define ITIDY_MAIN_PROGRESS_MIN_WIDTH      320
#define ITIDY_MAIN_PROGRESS_MAX_HISTORY     50

/* Spinner animation characters */
#define ITIDY_SPINNER_CHARS "|/-\\"
#define ITIDY_SPINNER_COUNT 4

/* Heartbeat delay threshold (ticks) - 50 ticks/sec PAL, 60 ticks/sec NTSC */
/* 100 ticks = 2 seconds on PAL */
#define ITIDY_HEARTBEAT_DELAY_TICKS 100

struct iTidyMainProgressWindow
{
    struct Screen *screen;
    struct Window *window;
    APTR visual_info;
    struct Gadget *glist;
    struct Gadget *history_listview;
    struct Gadget *cancel_button;
    struct List history_entries;
    ULONG history_count;
    BOOL window_open;
    BOOL cancel_requested;
    
    /* Heartbeat status area (raw text rendering) */
    WORD heartbeat_left;       /* Left edge of heartbeat text area */
    WORD heartbeat_top;        /* Baseline Y for Move() - where text sits */
    WORD heartbeat_area_top;   /* Top of erase area for RectFill */
    WORD heartbeat_width;      /* Width available for text */
    WORD heartbeat_height;     /* Height of erase area (font_height + descenders) */
    UBYTE spinner_state;       /* Current spinner frame (0-3) */
    UBYTE text_pen;            /* DrawInfo TEXTPEN for text */
    UBYTE bg_pen;              /* DrawInfo BACKGROUNDPEN for erase */
    
    /* Delayed heartbeat timing */
    struct DateStamp heartbeat_start;  /* When first update was called */
    BOOL heartbeat_timing_active;      /* TRUE if we're tracking time */
    BOOL heartbeat_visible;            /* TRUE if heartbeat has been displayed */
};

BOOL itidy_main_progress_window_open(struct iTidyMainProgressWindow *window_data);
void itidy_main_progress_window_close(struct iTidyMainProgressWindow *window_data);
BOOL itidy_main_progress_window_handle_events(struct iTidyMainProgressWindow *window_data);
BOOL itidy_main_progress_window_append_status(struct iTidyMainProgressWindow *window_data,
                                              const char *status_text);
void itidy_main_progress_window_clear_history(struct iTidyMainProgressWindow *window_data);
void itidy_main_progress_window_set_button_text(struct iTidyMainProgressWindow *window_data,
                                                const char *text);

/* Heartbeat status update - lightweight raw text rendering for progress feedback
 * phase: "Scanning" or "Saving"
 * current: Number of icons found/saved so far
 * total: Total icons (0 = unknown, used for "X/Y" format in save phase)
 */
void itidy_main_progress_update_heartbeat(struct iTidyMainProgressWindow *window_data,
                                          const char *phase,
                                          LONG current,
                                          LONG total);

/* Clear the heartbeat area (call before starting a new operation) */
void itidy_main_progress_clear_heartbeat(struct iTidyMainProgressWindow *window_data);

#endif /* ITIDY_MAIN_PROGRESS_WINDOW_H */
