/*
 * test_simple_window.h - Minimal test window to isolate MuForce errors
 */

#ifndef TEST_SIMPLE_WINDOW_H
#define TEST_SIMPLE_WINDOW_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>

/* Folder Entry Structure (simplified) */
struct TestFolderEntry
{
    struct Node node;               /* For linking in list */
    char display_text[128];         /* Display text for ListView */
};

/* Simple window data structure */
struct SimpleTestWindow
{
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* The window itself */
    BOOL window_open;                   /* TRUE if window is open */
    char title[80];                     /* Window title (MUST persist!) */
    APTR visual_info;                   /* GadTools visual info */
    struct Gadget *glist;               /* Gadget list */
    struct Gadget *listview_gad;        /* ListView gadget */
    struct Gadget *close_button;        /* Close button gadget */
    struct List folder_list;            /* List for ListView (empty for now) */
};

/* Function prototypes */
BOOL open_simple_test_window(struct SimpleTestWindow *test_data, UWORD run_number, const char *date_str);
void close_simple_test_window(struct SimpleTestWindow *test_data);
void handle_simple_test_events(struct SimpleTestWindow *test_data);

#endif /* TEST_SIMPLE_WINDOW_H */
