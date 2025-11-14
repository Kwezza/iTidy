/*------------------------------------------------------------------------
 *
 * amiga_window_resize_template.c - Workbench 3.0/3.1 GadTools resize demo
 *
 * Demonstrates a GimmeZeroZero window with manual gadget anchoring,
 * console logging, and robust resize handling for GadTools gadgets.
 *
 * Requirements covered:
 *  - C89 / vbcc compatible code (no C99-only features)
 *  - Targeted at AmigaOS / Workbench 3.0 and 3.1
 *  - Uses a GimmeZeroZero window so the client area starts at (0,0)
 *  - Handles IDCMP_SIZEVERIFY, IDCMP_NEWSIZE, and IDCMP_REFRESHWINDOW
 *  - Updates gadget geometry manually (no relative gadget flags)
 *  - Logs window/gadget geometry to a CON: window (Workbench safe)
 *  - Provides sample gadgets: ListView, String, Checkbox, OK/Cancel buttons
 *
 * This file is self-contained; build it with vbcc +aos68k using the
 * Makefile that sits in the same directory.
 *------------------------------------------------------------------------*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/*------------------------------------------------------------------------*/
/* Constants                                                              */
/*------------------------------------------------------------------------*/
#define ITIDY_WINDOW_TITLE          "Window Template Test"
#define ITIDY_CONSOLE_TITLE         "Resizable GadTools Debug"
#define ITIDY_CONSOLE_SPEC          "CON:0/22/640/200/" ITIDY_CONSOLE_TITLE "/AUTO/CLOSE/WAIT"
#define ITIDY_LIST_LABEL_TEXT       "List"
#define ITIDY_INPUT_LABEL_TEXT      "Input:"
#define ITIDY_CHECKBOX_LABEL_TEXT   "Enable Option"
#define ITIDY_BUTTON_OK_TEXT        "OK"
#define ITIDY_BUTTON_CANCEL_TEXT    "Cancel"

#define ITIDY_LIST_ITEMS            6
#define ITIDY_BASE_LIST_WIDTH       220
#define ITIDY_BASE_LIST_HEIGHT      120
#define ITIDY_RIGHT_COLUMN_WIDTH    170
#define ITIDY_LIST_LEFT_MARGIN      10
#define ITIDY_RIGHT_COLUMN_MARGIN   12
#define ITIDY_COLUMN_GAP            14
#define ITIDY_TOP_MARGIN            10
#define ITIDY_LABEL_GAP             4
#define ITIDY_LIST_BOTTOM_GAP       10
#define ITIDY_BUTTON_LEFT_MARGIN    10
#define ITIDY_BUTTON_SPACING        12
#define ITIDY_BUTTON_BOTTOM_MARGIN  12
#define ITIDY_CHECKBOX_GAP          10

/* Gadget IDs */
enum
{
    ITIDY_GAD_LISTVIEW = 1000,
    ITIDY_GAD_STRING,
    ITIDY_GAD_CHECKBOX,
    ITIDY_GAD_OK,
    ITIDY_GAD_CANCEL,
    ITIDY_GAD_LIST_LABEL,
    ITIDY_GAD_STRING_LABEL
};

/*------------------------------------------------------------------------*/
/* Data structures                                                        */
/*------------------------------------------------------------------------*/
typedef struct iTidy_ListItem
{
    struct Node node;
    STRPTR text;
} iTidy_ListItem;

typedef struct iTidy_FontMetrics
{
    UWORD font_width;
    UWORD font_height;
    UWORD button_height;
    UWORD string_height;
    UWORD checkbox_height;
} iTidy_FontMetrics;

typedef struct iTidy_LayoutMetrics
{
    UWORD list_label_height;
    UWORD list_top;
    UWORD list_bottom_margin;
    UWORD min_list_width;
    UWORD min_list_height;
    UWORD column_width;
    UWORD string_label_width;
    UWORD string_label_spacing;
    UWORD string_top;
    UWORD checkbox_top;
    UWORD initial_inner_width;
    UWORD initial_inner_height;
    UWORD button_width;
    UWORD button_height;
} iTidy_LayoutMetrics;

typedef struct iTidy_ResizeApp
{
    struct Screen    *screen;
    struct DrawInfo  *draw_info;
    struct VisualInfo *visual_info;
    struct Window    *window;
    struct Gadget    *glist;
    struct Gadget    *gad_list_label;
    struct Gadget    *gad_listview;
    struct Gadget    *gad_string_label;
    struct Gadget    *gad_string;
    struct Gadget    *gad_checkbox;
    struct Gadget    *gad_ok;
    struct Gadget    *gad_cancel;
    struct List       list_data;
    BOOL              gadgets_attached;
    iTidy_FontMetrics font;
    iTidy_LayoutMetrics layout;
    BPTR              console_handle;
    BPTR              original_stdout;
    BOOL              owns_console;
} iTidy_ResizeApp;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/
static BOOL iTidy_open_console(iTidy_ResizeApp *app, int argc);
static VOID iTidy_close_console(iTidy_ResizeApp *app);
static BOOL iTidy_init_screen(iTidy_ResizeApp *app);
static VOID iTidy_cleanup_screen(iTidy_ResizeApp *app);
static VOID iTidy_log(const char *fmt, ...);
static BOOL iTidy_init_layout(iTidy_ResizeApp *app);
static BOOL iTidy_populate_list(iTidy_ResizeApp *app);
static VOID iTidy_free_list(iTidy_ResizeApp *app);
static BOOL iTidy_create_gadgets(iTidy_ResizeApp *app);
static VOID iTidy_destroy_gadgets(iTidy_ResizeApp *app);
static BOOL iTidy_open_window(iTidy_ResizeApp *app);
static VOID iTidy_close_window(iTidy_ResizeApp *app);
static VOID iTidy_detach_gadgets(iTidy_ResizeApp *app);
static VOID iTidy_attach_gadgets(iTidy_ResizeApp *app);
static VOID iTidy_apply_layout(iTidy_ResizeApp *app);
static VOID iTidy_fill_background(iTidy_ResizeApp *app);
static VOID iTidy_set_gadget_bounds(iTidy_ResizeApp *app,
                                    struct Gadget *gadget,
                                    WORD left,
                                    WORD top,
                                    WORD width,
                                    WORD height,
                                    BOOL resize);
static VOID iTidy_log_geometry(iTidy_ResizeApp *app, const char *context);
static VOID iTidy_handle_resize(iTidy_ResizeApp *app);
static BOOL iTidy_handle_gadget_up(iTidy_ResizeApp *app, struct IntuiMessage *msg);
static VOID iTidy_handle_gadget_down(iTidy_ResizeApp *app, struct IntuiMessage *msg);
static STRPTR iTidy_get_list_text(struct List *list, UWORD index);
static UWORD iTidy_get_inner_width(struct Window *window);
static UWORD iTidy_get_inner_height(struct Window *window);

/*------------------------------------------------------------------------*/
/* Console helpers                                                        */
/*------------------------------------------------------------------------*/
static BOOL iTidy_open_console(iTidy_ResizeApp *app, int argc)
{
    app->console_handle = 0;
    app->original_stdout = Output();
    app->owns_console = FALSE;

    if (argc == 0)
    {
        app->console_handle = Open(ITIDY_CONSOLE_SPEC, MODE_OLDFILE);
        if (app->console_handle == 0)
        {
            return FALSE;
        }

        SelectOutput(app->console_handle);
        app->owns_console = TRUE;
    }

    return TRUE;
}

static VOID iTidy_close_console(iTidy_ResizeApp *app)
{
    if (app->owns_console)
    {
        SelectOutput(app->original_stdout);
        iTidy_log("Press RETURN to close the console window...\n");
        FGetC(app->console_handle);
        Close(app->console_handle);
        app->console_handle = 0;
        app->owns_console = FALSE;
    }
}

static VOID iTidy_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/*------------------------------------------------------------------------*/
/* Screen / layout setup                                                  */
/*------------------------------------------------------------------------*/
static BOOL iTidy_init_screen(iTidy_ResizeApp *app)
{
    app->screen = LockPubScreen("Workbench");
    if (app->screen == NULL)
    {
        return FALSE;
    }

    app->visual_info = GetVisualInfo(app->screen, TAG_END);
    if (app->visual_info == NULL)
    {
        return FALSE;
    }

    app->draw_info = GetScreenDrawInfo(app->screen);
    if (app->draw_info == NULL)
    {
        return FALSE;
    }

    app->font.font_width  = app->draw_info->dri_Font->tf_XSize;
    app->font.font_height = app->draw_info->dri_Font->tf_YSize;
    app->font.button_height = app->font.font_height + 6;
    app->font.string_height = app->font.font_height + 6;
    app->font.checkbox_height = app->font.font_height + 6;

    return TRUE;
}

static VOID iTidy_cleanup_screen(iTidy_ResizeApp *app)
{
    if (app->draw_info != NULL)
    {
        FreeScreenDrawInfo(app->screen, app->draw_info);
        app->draw_info = NULL;
    }

    if (app->visual_info != NULL)
    {
        FreeVisualInfo(app->visual_info);
        app->visual_info = NULL;
    }

    if (app->screen != NULL)
    {
        UnlockPubScreen(NULL, app->screen);
        app->screen = NULL;
    }
}

static BOOL iTidy_init_layout(iTidy_ResizeApp *app)
{
    struct RastPort temp_rp;
    struct TextFont *font;
    STRPTR label = ITIDY_INPUT_LABEL_TEXT;

    font = app->draw_info->dri_Font;
    InitRastPort(&temp_rp);
    SetFont(&temp_rp, font);

    app->layout.list_label_height = app->font.font_height;
    app->layout.list_top = ITIDY_TOP_MARGIN + app->layout.list_label_height + ITIDY_LABEL_GAP;
    app->layout.button_height = app->font.button_height;
    app->layout.button_width = app->font.font_width * 8 + 16;
    app->layout.list_bottom_margin = ITIDY_BUTTON_BOTTOM_MARGIN + app->layout.button_height + ITIDY_LIST_BOTTOM_GAP;
    app->layout.min_list_width = ITIDY_BASE_LIST_WIDTH;
    app->layout.min_list_height = ITIDY_BASE_LIST_HEIGHT;
    app->layout.column_width = ITIDY_RIGHT_COLUMN_WIDTH;
    app->layout.string_label_spacing = 6;
    app->layout.string_label_width = TextLength(&temp_rp, label, strlen(label));
    app->layout.string_top = ITIDY_TOP_MARGIN;
    app->layout.checkbox_top = app->layout.string_top + app->font.string_height + ITIDY_CHECKBOX_GAP;
    app->layout.initial_inner_width = ITIDY_LIST_LEFT_MARGIN + app->layout.min_list_width + ITIDY_COLUMN_GAP + app->layout.column_width + ITIDY_RIGHT_COLUMN_MARGIN;
    app->layout.initial_inner_height = app->layout.list_top + app->layout.min_list_height + app->layout.list_bottom_margin;

    if (app->layout.button_width < 90)
    {
        app->layout.button_width = 90;
    }

    return TRUE;
}

/*------------------------------------------------------------------------*/
/* List helpers                                                           */
/*------------------------------------------------------------------------*/
static BOOL iTidy_populate_list(iTidy_ResizeApp *app)
{
    static const char *items[ITIDY_LIST_ITEMS] =
    {
        "Item 01: Sample List",
        "Item 02: Font-Aware",
        "Item 03: Dynamic Window",
        "Item 04: Amiga Development",
        "Item 05: GadTools Layout",
        "Item 06: ANSI C Compatible"
    };

    int i;

    NewList(&app->list_data);

    for (i = 0; i < ITIDY_LIST_ITEMS; ++i)
    {
        iTidy_ListItem *item = (iTidy_ListItem *)AllocVec(sizeof(iTidy_ListItem), MEMF_CLEAR);
        if (item == NULL)
        {
            return FALSE;
        }

        item->text = (STRPTR)AllocVec(strlen(items[i]) + 1, MEMF_CLEAR);
        if (item->text == NULL)
        {
            FreeVec(item);
            return FALSE;
        }

        strcpy(item->text, items[i]);
        item->node.ln_Name = item->text;
        AddTail(&app->list_data, &item->node);
    }

    return TRUE;
}

static VOID iTidy_free_list(iTidy_ResizeApp *app)
{
    struct Node *node = app->list_data.lh_Head;

    while (node != NULL && node->ln_Succ != NULL)
    {
        struct Node *next = node->ln_Succ;
        iTidy_ListItem *item = (iTidy_ListItem *)node;

        Remove(node);
        if (item->text != NULL)
        {
            FreeVec(item->text);
        }
        FreeVec(item);
        node = next;
    }

    NewList(&app->list_data);
}

/*------------------------------------------------------------------------*/
/* Gadget creation                                                        */
/*------------------------------------------------------------------------*/
static BOOL iTidy_create_gadgets(iTidy_ResizeApp *app)
{
    struct NewGadget ng;
    struct Gadget *gad;
    UWORD list_width = app->layout.min_list_width;
    UWORD list_height = app->layout.min_list_height;
    UWORD list_left = ITIDY_LIST_LEFT_MARGIN;
    UWORD list_top = app->layout.list_top;
    UWORD column_left = list_left + list_width + ITIDY_COLUMN_GAP;
    UWORD button_top = app->layout.initial_inner_height - app->layout.button_height - ITIDY_BUTTON_BOTTOM_MARGIN;

    if (column_left + app->layout.column_width > app->layout.initial_inner_width)
    {
        column_left = app->layout.initial_inner_width - app->layout.column_width - ITIDY_RIGHT_COLUMN_MARGIN;
    }

    gad = CreateContext(&app->glist);
    if (gad == NULL)
    {
        return FALSE;
    }
    ng.ng_TextAttr = app->screen->Font;
    ng.ng_VisualInfo = app->visual_info;
    ng.ng_Flags = 0;
    ng.ng_LeftEdge = ITIDY_LIST_LEFT_MARGIN;
    ng.ng_TopEdge = ITIDY_TOP_MARGIN;
    ng.ng_Width = TextLength(&app->screen->RastPort, ITIDY_LIST_LABEL_TEXT, strlen(ITIDY_LIST_LABEL_TEXT));
    ng.ng_Height = app->layout.list_label_height;
    ng.ng_GadgetID = ITIDY_GAD_LIST_LABEL;
    ng.ng_GadgetText = NULL;

    app->gad_list_label = gad = CreateGadget(TEXT_KIND, gad, &ng,
                                             GTTX_Text, ITIDY_LIST_LABEL_TEXT,
                                             GTTX_Border, FALSE,
                                             TAG_END);
    if (gad == NULL)
    {
        return FALSE;
    }

    ng.ng_LeftEdge = list_left;
    ng.ng_TopEdge = list_top;
    ng.ng_Width = list_width;
    ng.ng_Height = list_height;
    ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = ITIDY_GAD_LISTVIEW;

    app->gad_listview = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
                                           GTLV_Labels, &app->list_data,
                                           GTLV_Selected, ~0UL,
                                           GTLV_ShowSelected, NULL,
                                           TAG_END);
    if (gad == NULL)
    {
        return FALSE;
    }

    ng.ng_LeftEdge = ITIDY_BUTTON_LEFT_MARGIN;
    ng.ng_TopEdge = button_top;
    ng.ng_Width = app->layout.button_width;
    ng.ng_Height = app->layout.button_height;
    ng.ng_GadgetText = ITIDY_BUTTON_OK_TEXT;
    ng.ng_GadgetID = ITIDY_GAD_OK;
    ng.ng_Flags = PLACETEXT_IN;

    app->gad_ok = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        return FALSE;
    }

    ng.ng_LeftEdge = ITIDY_BUTTON_LEFT_MARGIN + app->layout.button_width + ITIDY_BUTTON_SPACING;
    ng.ng_GadgetText = ITIDY_BUTTON_CANCEL_TEXT;
    ng.ng_GadgetID = ITIDY_GAD_CANCEL;

    app->gad_cancel = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        return FALSE;
    }

    ng.ng_LeftEdge = column_left;
    ng.ng_TopEdge = app->layout.string_top;
    ng.ng_Width = app->layout.string_label_width;
    ng.ng_Height = app->layout.list_label_height;
    ng.ng_GadgetID = ITIDY_GAD_STRING_LABEL;
    ng.ng_GadgetText = NULL;
    ng.ng_Flags = 0;

    app->gad_string_label = gad = CreateGadget(TEXT_KIND, gad, &ng,
                                               GTTX_Text, ITIDY_INPUT_LABEL_TEXT,
                                               GTTX_Border, FALSE,
                                               TAG_END);
    if (gad == NULL)
    {
        return FALSE;
    }

    ng.ng_LeftEdge = column_left + app->layout.string_label_width + app->layout.string_label_spacing;
    ng.ng_TopEdge = app->layout.string_top;
    ng.ng_Width = app->layout.column_width - app->layout.string_label_width - app->layout.string_label_spacing;
    ng.ng_Height = app->font.string_height;
    ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = ITIDY_GAD_STRING;
    ng.ng_Flags = 0;

    app->gad_string = gad = CreateGadget(STRING_KIND, gad, &ng,
                                         GTST_String, "",
                                         GTST_MaxChars, 80,
                                         TAG_END);
    if (gad == NULL)
    {
        return FALSE;
    }

    ng.ng_LeftEdge = column_left;
    ng.ng_TopEdge = app->layout.checkbox_top;
    ng.ng_Width = app->layout.column_width;
    ng.ng_Height = app->font.checkbox_height;
    ng.ng_GadgetText = ITIDY_CHECKBOX_LABEL_TEXT;
    ng.ng_GadgetID = ITIDY_GAD_CHECKBOX;
    ng.ng_Flags = PLACETEXT_RIGHT;

    app->gad_checkbox = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
                                           GTCB_Checked, FALSE,
                                           TAG_END);
    if (gad == NULL)
    {
        return FALSE;
    }

    return TRUE;
}

static VOID iTidy_destroy_gadgets(iTidy_ResizeApp *app)
{
    if (app->glist != NULL)
    {
        FreeGadgets(app->glist);
        app->glist = NULL;
    }
}

/*------------------------------------------------------------------------*/
/* Window helpers                                                         */
/*------------------------------------------------------------------------*/
static BOOL iTidy_open_window(iTidy_ResizeApp *app)
{
    ULONG inner_width = app->layout.initial_inner_width;
    ULONG inner_height = app->layout.initial_inner_height;

    app->window = OpenWindowTags(NULL,
                                 WA_Title, (ULONG)ITIDY_WINDOW_TITLE,
                                 WA_GimmeZeroZero, TRUE,
                                 WA_InnerWidth, inner_width,
                                 WA_InnerHeight, inner_height,
                                 WA_DragBar, TRUE,
                                 WA_DepthGadget, TRUE,
                                 WA_CloseGadget, TRUE,
                                 WA_SizeGadget, TRUE,
                                 WA_SizeBBottom, TRUE,
                                 WA_SizeBRight, TRUE,
                                 WA_Activate, TRUE,
                                 WA_PubScreen, (ULONG)app->screen,
                                 WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP |
                                           IDCMP_GADGETDOWN | IDCMP_REFRESHWINDOW |
                                           IDCMP_NEWSIZE | IDCMP_SIZEVERIFY,
                                 TAG_END);

    if (app->window == NULL)
    {
        return FALSE;
    }

    iTidy_apply_layout(app);
    AddGList(app->window, app->glist, (UWORD)~0, (UWORD)~0, NULL);
    app->gadgets_attached = TRUE;
    GT_RefreshWindow(app->window, NULL);
    WindowLimits(app->window,
                 app->window->Width,
                 app->window->Height,
                 0xFFFF,
                 0xFFFF);
    iTidy_log_geometry(app, "Initial layout");
    return TRUE;
}

static VOID iTidy_close_window(iTidy_ResizeApp *app)
{
    if (app->window != NULL)
    {
        if (app->gad_listview != NULL)
        {
            GT_SetGadgetAttrs(app->gad_listview, app->window, NULL,
                              GTLV_Labels, ~0UL,
                              TAG_END);
        }

        CloseWindow(app->window);
        app->window = NULL;
    }
}

static VOID iTidy_detach_gadgets(iTidy_ResizeApp *app)
{
    if (app->window != NULL && app->gadgets_attached)
    {
        RemoveGList(app->window, app->glist, (UWORD)~0);
        app->gadgets_attached = FALSE;
    }
}

static VOID iTidy_attach_gadgets(iTidy_ResizeApp *app)
{
    if (app->window != NULL && !app->gadgets_attached)
    {
        AddGList(app->window, app->glist, (UWORD)~0, (UWORD)~0, NULL);
        app->gadgets_attached = TRUE;
    }
}

static UWORD iTidy_get_inner_width(struct Window *window)
{
    if (window->Flags & WFLG_GIMMEZEROZERO)
    {
        return window->GZZWidth;
    }

    return window->Width - window->BorderLeft - window->BorderRight;
}

static UWORD iTidy_get_inner_height(struct Window *window)
{
    if (window->Flags & WFLG_GIMMEZEROZERO)
    {
        return window->GZZHeight;
    }

    return window->Height - window->BorderTop - window->BorderBottom;
}

static VOID iTidy_fill_background(iTidy_ResizeApp *app)
{
    struct Window *win;
    struct RastPort *rp;
    UWORD inner_w;
    UWORD inner_h;

    if (app == NULL)
    {
        return;
    }

    win = app->window;
    if (win == NULL)
    {
        return;
    }

    rp = win->RPort;
    if (rp == NULL)
    {
        return;
    }

    inner_w = iTidy_get_inner_width(win);
    inner_h = iTidy_get_inner_height(win);
    if (inner_w == 0 || inner_h == 0)
    {
        return;
    }

    SetAPen(rp, app->draw_info->dri_Pens[BACKGROUNDPEN]);
    SetDrMd(rp, JAM2);
    RectFill(rp, 0, 0, inner_w - 1, inner_h - 1);
}

static VOID iTidy_set_gadget_bounds(iTidy_ResizeApp *app,
                                    struct Gadget *gadget,
                                    WORD left,
                                    WORD top,
                                    WORD width,
                                    WORD height,
                                    BOOL resize)
{
    if (gadget == NULL)
    {
        return;
    }

    gadget->LeftEdge = left;
    gadget->TopEdge = top;

    if (resize)
    {
        gadget->Width = width;
        gadget->Height = height;
    }

    if (app->window != NULL)
    {
        if (resize)
        {
            GT_SetGadgetAttrs(gadget, app->window, NULL,
                              GA_Left, left,
                              GA_Top, top,
                              GA_Width, width,
                              GA_Height, height,
                              TAG_END);
        }
        else
        {
            GT_SetGadgetAttrs(gadget, app->window, NULL,
                              GA_Left, left,
                              GA_Top, top,
                              TAG_END);
        }
    }
}

static VOID iTidy_apply_layout(iTidy_ResizeApp *app)
{
    UWORD inner_width;
    UWORD inner_height;
    UWORD column_left;
    UWORD list_width;
    UWORD list_height;
    UWORD button_top;

    if (app->window == NULL)
    {
        return;
    }

    inner_width = iTidy_get_inner_width(app->window);
    inner_height = iTidy_get_inner_height(app->window);

    column_left = inner_width - ITIDY_RIGHT_COLUMN_MARGIN - app->layout.column_width;
    if (column_left < ITIDY_LIST_LEFT_MARGIN + ITIDY_COLUMN_GAP)
    {
        column_left = ITIDY_LIST_LEFT_MARGIN + ITIDY_COLUMN_GAP;
    }

    list_width = column_left - ITIDY_COLUMN_GAP - ITIDY_LIST_LEFT_MARGIN;
    if (list_width < app->layout.min_list_width)
    {
        list_width = app->layout.min_list_width;
    }

    list_height = inner_height - app->layout.list_top - app->layout.list_bottom_margin;
    if (list_height < app->layout.min_list_height)
    {
        list_height = app->layout.min_list_height;
    }

    button_top = inner_height - ITIDY_BUTTON_BOTTOM_MARGIN - app->layout.button_height;

    if (app->gad_list_label != NULL)
    {
        iTidy_set_gadget_bounds(app,
                                app->gad_list_label,
                                ITIDY_LIST_LEFT_MARGIN,
                                ITIDY_TOP_MARGIN,
                                app->gad_list_label->Width,
                                app->gad_list_label->Height,
                                FALSE);
    }

    if (app->gad_listview != NULL)
    {
        iTidy_set_gadget_bounds(app,
                                app->gad_listview,
                                ITIDY_LIST_LEFT_MARGIN,
                                app->layout.list_top,
                                list_width,
                                list_height,
                                TRUE);
    }

    if (app->gad_ok != NULL)
    {
        iTidy_set_gadget_bounds(app,
                                app->gad_ok,
                                ITIDY_BUTTON_LEFT_MARGIN,
                                button_top,
                                app->layout.button_width,
                                app->layout.button_height,
                                TRUE);
    }

    if (app->gad_cancel != NULL)
    {
        iTidy_set_gadget_bounds(app,
                                app->gad_cancel,
                                ITIDY_BUTTON_LEFT_MARGIN + app->layout.button_width + ITIDY_BUTTON_SPACING,
                                button_top,
                                app->layout.button_width,
                                app->layout.button_height,
                                TRUE);
    }

    if (app->gad_string_label != NULL)
    {
        iTidy_set_gadget_bounds(app,
                                app->gad_string_label,
                                column_left,
                                app->layout.string_top,
                                app->gad_string_label->Width,
                                app->gad_string_label->Height,
                                FALSE);
    }

    if (app->gad_string != NULL)
    {
        iTidy_set_gadget_bounds(app,
                                app->gad_string,
                                column_left + app->layout.string_label_width + app->layout.string_label_spacing,
                                app->layout.string_top,
                                app->layout.column_width - app->layout.string_label_width - app->layout.string_label_spacing,
                                app->font.string_height,
                                TRUE);
    }

    if (app->gad_checkbox != NULL)
    {
        iTidy_set_gadget_bounds(app,
                                app->gad_checkbox,
                                column_left,
                                app->layout.checkbox_top,
                                app->layout.column_width,
                                app->font.checkbox_height,
                                TRUE);
    }

    iTidy_fill_background(app);
}

static VOID iTidy_log_geometry(iTidy_ResizeApp *app, const char *context)
{
    if (app->window == NULL)
    {
        return;
    }

    iTidy_log("\n[%s] Window outer %ux%u, inner %ux%u\n",
              context,
              app->window->Width,
              app->window->Height,
              iTidy_get_inner_width(app->window),
              iTidy_get_inner_height(app->window));

    if (app->gad_listview != NULL)
    {
        iTidy_log("  ListView: (%u,%u) %ux%u\n",
                  app->gad_listview->LeftEdge,
                  app->gad_listview->TopEdge,
                  app->gad_listview->Width,
                  app->gad_listview->Height);
    }

    if (app->gad_ok != NULL)
    {
        iTidy_log("  OK button: (%u,%u) %ux%u\n",
                  app->gad_ok->LeftEdge,
                  app->gad_ok->TopEdge,
                  app->gad_ok->Width,
                  app->gad_ok->Height);
    }

    if (app->gad_cancel != NULL)
    {
        iTidy_log("  Cancel button: (%u,%u) %ux%u\n",
                  app->gad_cancel->LeftEdge,
                  app->gad_cancel->TopEdge,
                  app->gad_cancel->Width,
                  app->gad_cancel->Height);
    }

    if (app->gad_string != NULL)
    {
        iTidy_log("  Input string: (%u,%u) %ux%u\n",
                  app->gad_string->LeftEdge,
                  app->gad_string->TopEdge,
                  app->gad_string->Width,
                  app->gad_string->Height);
    }

    if (app->gad_checkbox != NULL)
    {
        iTidy_log("  Checkbox: (%u,%u) %ux%u\n",
                  app->gad_checkbox->LeftEdge,
                  app->gad_checkbox->TopEdge,
                  app->gad_checkbox->Width,
                  app->gad_checkbox->Height);
    }
}

static VOID iTidy_handle_resize(iTidy_ResizeApp *app)
{
    iTidy_apply_layout(app);
    iTidy_attach_gadgets(app);
    RefreshGList(app->glist, app->window, NULL, (UWORD)~0);
    GT_RefreshWindow(app->window, NULL);
    iTidy_log_geometry(app, "Resize");
}

/*------------------------------------------------------------------------*/
/* Gadget interaction                                                     */
/*------------------------------------------------------------------------*/
static STRPTR iTidy_get_list_text(struct List *list, UWORD index)
{
    struct Node *node;
    UWORD pos = 0;

    for (node = list->lh_Head; node->ln_Succ != NULL; node = node->ln_Succ)
    {
        if (pos == index)
        {
            return node->ln_Name;
        }
        pos++;
    }

    return NULL;
}

static BOOL iTidy_handle_gadget_up(iTidy_ResizeApp *app, struct IntuiMessage *msg)
{
    struct Gadget *gadget = (struct Gadget *)msg->IAddress;

    switch (gadget->GadgetID)
    {
        case ITIDY_GAD_LISTVIEW:
        {
            UWORD index = msg->Code;
            STRPTR text = iTidy_get_list_text(&app->list_data, index);
            if (text != NULL)
            {
                iTidy_log("List item %u selected: %s\n", index, text);
            }
            else
            {
                iTidy_log("List selection cleared\n");
            }
            break;
        }

        case ITIDY_GAD_STRING:
        {
            STRPTR value = NULL;
            GT_GetGadgetAttrs(app->gad_string, app->window, NULL,
                              GTST_String, &value,
                              TAG_END);
            iTidy_log("String gadget committed: %s\n", value ? value : "(null)");
            break;
        }

        case ITIDY_GAD_CHECKBOX:
        {
            ULONG checked = 0;
            GT_GetGadgetAttrs(app->gad_checkbox, app->window, NULL,
                              GTCB_Checked, &checked,
                              TAG_END);
            iTidy_log("Checkbox %s\n", checked ? "enabled" : "disabled");
            break;
        }

        case ITIDY_GAD_OK:
        {
            STRPTR value = NULL;
            ULONG checked = 0;
            GT_GetGadgetAttrs(app->gad_string, app->window, NULL,
                              GTST_String, &value,
                              TAG_END);
            GT_GetGadgetAttrs(app->gad_checkbox, app->window, NULL,
                              GTCB_Checked, &checked,
                              TAG_END);
            iTidy_log("OK pressed — input='%s', checkbox=%s\n",
                      value ? value : "",
                      checked ? "ON" : "OFF");
            break;
        }

        case ITIDY_GAD_CANCEL:
        {
            iTidy_log("Cancel pressed — exiting\n");
            return FALSE;
        }
    }

    return TRUE;
}

static VOID iTidy_handle_gadget_down(iTidy_ResizeApp *app, struct IntuiMessage *msg)
{
    struct Gadget *gadget = (struct Gadget *)msg->IAddress;

    if (gadget->GadgetID == ITIDY_GAD_LISTVIEW)
    {
        ULONG top = 0;
        GT_GetGadgetAttrs(app->gad_listview, app->window, NULL,
                          GTLV_Top, &top,
                          TAG_END);
        iTidy_log("ListView scroll button pressed (top index %lu)\n", top);
    }
}

/*------------------------------------------------------------------------*/
/* Main program                                                           */
/*------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    iTidy_ResizeApp app;
    BOOL running = TRUE;
    ULONG win_sig = 0;

    memset(&app, 0, sizeof(app));

    if (!iTidy_open_console(&app, argc))
    {
        return RETURN_FAIL;
    }

    iTidy_log("iTidy GadTools resize template\n");
    iTidy_log("Launch mode: %s\n\n", argc == 0 ? "Workbench" : "CLI");

    if (!iTidy_init_screen(&app))
    {
        iTidy_log("Failed to lock Workbench screen\n");
        iTidy_close_console(&app);
        return RETURN_FAIL;
    }

    if (!iTidy_init_layout(&app))
    {
        iTidy_log("Failed to initialize layout metrics\n");
        iTidy_cleanup_screen(&app);
        iTidy_close_console(&app);
        return RETURN_FAIL;
    }

    if (!iTidy_populate_list(&app))
    {
        iTidy_log("Failed to populate ListView data\n");
        iTidy_cleanup_screen(&app);
        iTidy_close_console(&app);
        return RETURN_FAIL;
    }

    if (!iTidy_create_gadgets(&app))
    {
        iTidy_log("Failed to create gadgets\n");
        iTidy_free_list(&app);
        iTidy_cleanup_screen(&app);
        iTidy_close_console(&app);
        return RETURN_FAIL;
    }

    if (!iTidy_open_window(&app))
    {
        iTidy_log("Failed to open window\n");
        iTidy_destroy_gadgets(&app);
        iTidy_free_list(&app);
        iTidy_cleanup_screen(&app);
        iTidy_close_console(&app);
        return RETURN_FAIL;
    }

    win_sig = 1UL << app.window->UserPort->mp_SigBit;

    while (running)
    {
        ULONG signals = Wait(win_sig | SIGBREAKF_CTRL_C);

        if (signals & SIGBREAKF_CTRL_C)
        {
            iTidy_log("Ctrl+C received\n");
            break;
        }

        if (signals & win_sig)
        {
            struct IntuiMessage *msg;

            while ((msg = GT_GetIMsg(app.window->UserPort)) != NULL)
            {
                ULONG msg_class = msg->Class;

                switch (msg_class)
                {
                    case IDCMP_CLOSEWINDOW:
                        running = FALSE;
                        break;

                    case IDCMP_GADGETUP:
                        if (!iTidy_handle_gadget_up(&app, msg))
                        {
                            running = FALSE;
                        }
                        break;

                    case IDCMP_GADGETDOWN:
                        iTidy_handle_gadget_down(&app, msg);
                        break;

                    case IDCMP_REFRESHWINDOW:
                        GT_BeginRefresh(app.window);
                        iTidy_fill_background(&app);
                        GT_EndRefresh(app.window, TRUE);
                        break;

                    case IDCMP_SIZEVERIFY:
                        iTidy_detach_gadgets(&app);
                        break;

                    case IDCMP_NEWSIZE:
                        iTidy_handle_resize(&app);
                        break;

                    default:
                        break;
                }

                GT_ReplyIMsg(msg);
            }
        }
    }

    iTidy_close_window(&app);
    iTidy_destroy_gadgets(&app);
    iTidy_free_list(&app);
    iTidy_cleanup_screen(&app);
    iTidy_close_console(&app);

    return RETURN_OK;
}
