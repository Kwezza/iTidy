/*------------------------------------------------------------------------*/
/*                                                                        *
 * amiga_window_template.h - Standalone Dynamic Window Template Header   *
 * Version compatible with vbcc +aos68k and Workbench 3.x                *
 *                                                                        *
 * This is a completely self-contained template header with no           *
 * external dependencies beyond standard Amiga OS 3.x includes.          *
 *                                                                        */
/*------------------------------------------------------------------------*/

#ifndef AMIGA_WINDOW_TEMPLATE_H
#define AMIGA_WINDOW_TEMPLATE_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

/*------------------------------------------------------------------------*/
/* Font dimension structure for layout calculations                       */
/*------------------------------------------------------------------------*/
struct FontDimensions
{
    UWORD font_width;           /* Width of one character in current font */
    UWORD font_height;          /* Height of one character in current font */
    UWORD button_width;         /* Calculated button width based on font */
    UWORD button_height;        /* Calculated button height based on font */
    UWORD listview_height;      /* Calculated ListView height based on font */
    UWORD string_height;        /* Calculated string gadget height based on font */
    UWORD title_bar_height;     /* Height of window title bar */
    UWORD window_top_edge;      /* Top edge offset including title and borders */
    UWORD window_left_edge;     /* Left edge offset including borders */
    UWORD window_bottom_edge;   /* Bottom edge offset including borders */
};

/*------------------------------------------------------------------------*/
/* ListView item structure                                                */
/*------------------------------------------------------------------------*/
struct ListViewItem
{
    struct Node node;           /* Standard Exec node for list linkage */
    STRPTR text;                /* Item text (dynamically allocated) */
    ULONG user_data;            /* User-defined data associated with item */
};

/*------------------------------------------------------------------------*/
/* Template window data structure                                         */
/*------------------------------------------------------------------------*/
struct TemplateWindowData
{
    /* Screen and window */
    struct Screen *screen;              /* Pointer to public screen */
    struct Window *window;              /* Pointer to window */
    BOOL window_open;                   /* TRUE if window is open */
    STRPTR window_title;                /* Window title string */

    /* Gadgets */
    struct Gadget *glist;               /* Gadget list */
    struct Gadget *ok_button;           /* OK button gadget */
    struct Gadget *cancel_button;       /* Cancel button gadget */
    struct Gadget *main_listview;       /* Main ListView gadget */
    struct Gadget *string_input;        /* String input gadget */
    struct Gadget *checkbox_option;     /* Checkbox gadget */

    /* Menus */
    struct Menu *menu;                  /* Menu strip */

    /* GadTools visual info */
    APTR visual_info;                   /* Visual info for GadTools */

    /* Lists */
    struct List main_list;              /* List for ListView items */
};

/*------------------------------------------------------------------------*/
/* Function prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Creates a dynamically-sized Amiga window using font-based calculations
 * @param data Pointer to window data structure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
BOOL create_template_window(struct TemplateWindowData *data);

/**
 * @brief Clean up and close the template window
 * @param data Pointer to window data structure
 */
void close_template_window(struct TemplateWindowData *data);

/**
 * @brief Handle gadget events for the template window
 * @param data Pointer to window data structure
 * @param gadget_id ID of the activated gadget
 * @return BOOL TRUE to continue, FALSE to close window
 */
BOOL handle_template_gadget_event(struct TemplateWindowData *data, UWORD gadget_id);

/**
 * @brief Handle window resize events
 * @param data Pointer to window data structure
 * @return BOOL TRUE if resize handled successfully, FALSE otherwise
 */
BOOL handle_template_window_resize(struct TemplateWindowData *data);

/**
 * @brief Populate ListView with test data items
 * @param data Pointer to window data structure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
BOOL populate_listview_test_data(struct TemplateWindowData *data);

/**
 * @brief Free all ListView items and clear the list
 * @param data Pointer to window data structure
 */
void free_listview_items(struct TemplateWindowData *data);

#endif /* AMIGA_WINDOW_TEMPLATE_H */
