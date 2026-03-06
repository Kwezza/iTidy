/*
 * main_progress_window.h - iTidy Main Progress Window
 * Displays scrolling status history with Cancel control (ReAction Version)
 */

#ifndef ITIDY_MAIN_PROGRESS_WINDOW_H
#define ITIDY_MAIN_PROGRESS_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>

/* Gadget identifiers */
#define ITIDY_MAIN_PROGRESS_GID_HISTORY  4201
#define ITIDY_MAIN_PROGRESS_GID_CANCEL   4202

struct iTidyMainProgressWindow
{
    struct Screen *screen;
    struct Window *window; /* Intuition Window (via ReAction) */
    
    /* ReAction Objects */
    Object *window_obj;
    Object *main_layout;
    Object *listbrowser_obj;
    Object *status_label_obj; /* Image object for status text */
    Object *cancel_button_obj;
    
    struct List *history_list; /* ReAction ListBrowser list */
    ULONG history_count;
    
    /* State */
    BOOL window_open;
    BOOL cancel_requested;
    char cancel_button_text[32]; /* Track button text state */
};

/* API */
BOOL itidy_main_progress_window_open(struct iTidyMainProgressWindow *window_data);
void itidy_main_progress_window_close(struct iTidyMainProgressWindow *window_data);
BOOL itidy_main_progress_window_handle_events(struct iTidyMainProgressWindow *window_data);

/* Status Updates */
BOOL itidy_main_progress_window_append_status(struct iTidyMainProgressWindow *window_data, const char *fmt, ...);
void itidy_main_progress_window_clear_history(struct iTidyMainProgressWindow *window_data);

/* Button Control */
void itidy_main_progress_window_set_button_text(struct iTidyMainProgressWindow *window_data, const char *text);

/* Heartbeat / Spinner */
void itidy_main_progress_update_heartbeat(struct iTidyMainProgressWindow *window_data,
                                          const char *phase,
                                          LONG current,
                                          LONG total);
                                          
/* Clear heartbeat text */
void itidy_main_progress_clear_heartbeat(struct iTidyMainProgressWindow *window_data);

/* Internal helper alias */
#define itidy_add_main_history_entry itidy_main_progress_window_append_status

#endif /* ITIDY_MAIN_PROGRESS_WINDOW_H */
