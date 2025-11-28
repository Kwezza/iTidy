/*
 * main_progress_window.h - iTidy Main Progress Window
 * Displays scrolling status history with Cancel control
 */

#ifndef ITIDY_MAIN_PROGRESS_WINDOW_H
#define ITIDY_MAIN_PROGRESS_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
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
#define ITIDY_MAIN_PROGRESS_WIDTH_CHARS     60
#define ITIDY_MAIN_PROGRESS_MIN_WIDTH      320
#define ITIDY_MAIN_PROGRESS_MAX_HISTORY     50

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
};

BOOL itidy_main_progress_window_open(struct iTidyMainProgressWindow *window_data);
void itidy_main_progress_window_close(struct iTidyMainProgressWindow *window_data);
BOOL itidy_main_progress_window_handle_events(struct iTidyMainProgressWindow *window_data);
BOOL itidy_main_progress_window_append_status(struct iTidyMainProgressWindow *window_data,
                                              const char *status_text);
void itidy_main_progress_window_clear_history(struct iTidyMainProgressWindow *window_data);
void itidy_main_progress_window_set_button_text(struct iTidyMainProgressWindow *window_data,
                                                const char *text);

#endif /* ITIDY_MAIN_PROGRESS_WINDOW_H */
