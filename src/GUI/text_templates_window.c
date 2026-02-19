/*
 * text_templates_window.c - ASCII Text Template Manager Window (ReAction)
 *
 * Lists every ASCII DefIcons sub-type and, for each, indicates whether
 * a custom def_<type>.info template exists in PROGDIR:Icons/.
 * Provides Copy, Validate ToolTypes, and Open WB Info actions.
 *
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

/* Library base isolation - prevent linker conflicts with other windows */
#define WindowBase    iTidy_TextTemplates_WindowBase
#define LayoutBase    iTidy_TextTemplates_LayoutBase
#define ListBrowserBase iTidy_TextTemplates_ListBrowserBase
#define ButtonBase    iTidy_TextTemplates_ButtonBase
#define ChooserBase   iTidy_TextTemplates_ChooserBase
#define LabelBase     iTidy_TextTemplates_LabelBase
#define RequesterBase iTidy_TextTemplates_RequesterBase

#include "text_templates_window.h"
#include "../writeLog.h"
#include "../platform/platform.h"
#include "../deficons/deficons_templates.h"
#include "../icon_edit/icon_image_access.h"   /* ITIDY_TT_* defines */

#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/button.h>
#include <proto/chooser.h>
#include <proto/label.h>
#include <proto/requester.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/utility.h>

#include <classes/window.h>
#include <classes/requester.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <gadgets/button.h>
#include <gadgets/chooser.h>
#include <images/label.h>

/* ARexx for WB Info command */
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <proto/rexxsyslib.h>

#include <workbench/icon.h>

#include <string.h>
#include <stdio.h>

/*========================================================================*/
/* Local library base declarations                                       */
/*========================================================================*/

struct Library *iTidy_TextTemplates_WindowBase    = NULL;
struct Library *iTidy_TextTemplates_LayoutBase    = NULL;
struct Library *iTidy_TextTemplates_ListBrowserBase = NULL;
struct Library *iTidy_TextTemplates_ButtonBase    = NULL;
struct Library *iTidy_TextTemplates_ChooserBase   = NULL;
struct Library *iTidy_TextTemplates_LabelBase     = NULL;
struct Library *iTidy_TextTemplates_RequesterBase = NULL;

/*========================================================================*/
/* Constants                                                              */
/*========================================================================*/

/* Path to the template icons directory */
#define TEMPLATE_DIR  "PROGDIR:Icons/"

/* Column widths (sum must equal 100) */
#define COL_TYPE_PCT   35
#define COL_FILE_PCT   45
#define COL_STATUS_PCT 20

/* Gadget IDs - matching testcode.c reference design */
enum {
    GID_SHOW_CHOOSER         = 9,   /* "Show" filter chooser */
    GID_TEMPLATE_LB          = 13,  /* ListBrowser */
    GID_SELECTED_TYPE        = 16,  /* Read-only: type name */
    GID_SELECTED_TMPLFILE    = 17,  /* Read-only: template file */
    GID_SELECTED_EFFECTIVE   = 19,  /* Read-only: effective file */
    GID_SELECTED_STATUS      = 20,  /* Read-only: status */
    GID_BTN_CREATE_OVERWRITE = 24,  /* Create/Overwrite from master */
    GID_BTN_EDIT_TOOLTYPES   = 25,  /* Edit tooltypes (WB Info) */
    GID_BTN_VALIDATE         = 26,  /* Validate tooltypes */
    GID_BTN_REVERT           = 35,  /* Revert to master */
    GID_BTN_CLOSE            = 33   /* Close window */
};

/* Column indices */
#define COL_TYPE    0
#define COL_FILE    1
#define COL_STATUS  2

/*========================================================================*/
/* Window state structure                                                */
/*========================================================================*/

/* Filter for the "Show" chooser */
typedef enum {
    SHOW_ALL = 0,
    SHOW_CUSTOM_ONLY,
    SHOW_MISSING_ONLY
} ShowFilter;

typedef struct {
    /* ReAction objects */
    Object *window_obj;
    Object *show_chooser;
    Object *template_lb;
    Object *selected_type;
    Object *selected_tmplfile;
    Object *selected_effective;
    Object *selected_status;
    Object *btn_create_overwrite;
    Object *btn_edit_tooltypes;
    Object *btn_validate;
    Object *btn_revert;
    Object *btn_close;

    struct Window *window;
    struct List   *lb_list;      /* ListBrowser nodes */
    struct List   *chooser_list; /* Chooser label list */

    LayoutPreferences *prefs;

    ShowFilter show_filter;

    /* String buffers for the read-only display fields */
    char buf_type[64];
    char buf_tmplfile[256];
    char buf_effective[256];
    char buf_status[64];
    char buf_create_label[64];

    /* Per-window ColumnInfo buffer (must NOT be shared or static:
     * listbrowser writes back widths when LISTBROWSER_AutoFit is used) */
    struct ColumnInfo col_info[4];

    /* Cached subtype table */
    const char **subtypes;      /* from deficons_get_ascii_subtypes() */
    int          subtype_count;

    /* Row-to-subtype mapping (rebuilt by rebuild_list on every filter change).
     * subtype_map[visual_row] = actual index into win->subtypes[].
     * LISTBROWSER_Selected returns a visual row, NOT a subtypes[] index. */
    int subtype_map[512];
    int visible_count;          /* number of rows currently in the listbrowser */

    /* EXCLUDETYPE data loaded from PROGDIR:Icons/def_ascii.info on window open */
    char   exclude_types[512];  /* raw EXCLUDETYPE value: "amigaguide,html,install" */
    STRPTR exclude_list[64];    /* pointers into exclude_types[] after tokenising */
    int    exclude_count;       /* number of entries in exclude_list[] */
} TextTemplatesWindow;

/* Forward declarations */
static void update_selected_type_panel(TextTemplatesWindow *win, int selected_idx);
static void rebuild_list(TextTemplatesWindow *win);

/*========================================================================*/
/* Library open / close                                                  */
/*========================================================================*/

static BOOL open_libs(void)
{
    iTidy_TextTemplates_WindowBase     = OpenLibrary("window.class",            44);
    iTidy_TextTemplates_LayoutBase     = OpenLibrary("gadgets/layout.gadget",   44);
    iTidy_TextTemplates_ListBrowserBase= OpenLibrary("gadgets/listbrowser.gadget", 44);
    iTidy_TextTemplates_ButtonBase     = OpenLibrary("gadgets/button.gadget",   44);
    iTidy_TextTemplates_ChooserBase    = OpenLibrary("gadgets/chooser.gadget",  44);
    iTidy_TextTemplates_LabelBase      = OpenLibrary("images/label.image",      44);
    iTidy_TextTemplates_RequesterBase  = OpenLibrary("requester.class",         44);

    if (!iTidy_TextTemplates_WindowBase || !iTidy_TextTemplates_LayoutBase ||
        !iTidy_TextTemplates_ListBrowserBase || !iTidy_TextTemplates_ButtonBase ||
        !iTidy_TextTemplates_ChooserBase  || !iTidy_TextTemplates_LabelBase ||
        !iTidy_TextTemplates_RequesterBase)
    {
        log_error(LOG_GUI, "TextTemplates: failed to open ReAction libraries\n");
        return FALSE;
    }
    return TRUE;
}

static void close_libs(void)
{
    if (iTidy_TextTemplates_RequesterBase)
    {
        CloseLibrary(iTidy_TextTemplates_RequesterBase);
        iTidy_TextTemplates_RequesterBase = NULL;
    }
    if (iTidy_TextTemplates_LabelBase)
    {
        CloseLibrary(iTidy_TextTemplates_LabelBase);
        iTidy_TextTemplates_LabelBase = NULL;
    }
    if (iTidy_TextTemplates_ChooserBase)
    {
        CloseLibrary(iTidy_TextTemplates_ChooserBase);
        iTidy_TextTemplates_ChooserBase = NULL;
    }
    if (iTidy_TextTemplates_ButtonBase)
    {
        CloseLibrary(iTidy_TextTemplates_ButtonBase);
        iTidy_TextTemplates_ButtonBase = NULL;
    }
    if (iTidy_TextTemplates_ListBrowserBase)
    {
        CloseLibrary(iTidy_TextTemplates_ListBrowserBase);
        iTidy_TextTemplates_ListBrowserBase = NULL;
    }
    if (iTidy_TextTemplates_LayoutBase)
    {
        CloseLibrary(iTidy_TextTemplates_LayoutBase);
        iTidy_TextTemplates_LayoutBase = NULL;
    }
    if (iTidy_TextTemplates_WindowBase)
    {
        CloseLibrary(iTidy_TextTemplates_WindowBase);
        iTidy_TextTemplates_WindowBase = NULL;
    }
}

/*========================================================================*/
/* Column info initialiser                                               */
/*========================================================================*/

/** Initialise a per-window ColumnInfo[4] buffer (terminator at index 3). */
static void init_column_info(struct ColumnInfo *ci)
{
    ci[0].ci_Width = COL_TYPE_PCT;   ci[0].ci_Title = "Type";     ci[0].ci_Flags = 0;
    ci[1].ci_Width = COL_FILE_PCT;   ci[1].ci_Title = "Template"; ci[1].ci_Flags = 0;
    ci[2].ci_Width = COL_STATUS_PCT; ci[2].ci_Title = "Status";   ci[2].ci_Flags = 0;
    ci[3].ci_Width = -1;             ci[3].ci_Title = NULL;        ci[3].ci_Flags = 0;
}

/*========================================================================*/
/* Helpers                                                               */
/*========================================================================*/

/**
 * @brief Check whether PROGDIR:Icons/def_<type>.info exists
 */
static BOOL type_has_template(const char *type_token)
{
    char path[256];
    BPTR lock;

    snprintf(path, sizeof(path), "%sdef_%s.info", TEMPLATE_DIR, type_token);
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock)
    {
        UnLock(lock);
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Build the template filename string for display ("def_<type>.info" or "-")
 */
static void make_file_label(const char *type_token, char *buf, int buf_size)
{
    if (type_has_template(type_token))
        snprintf(buf, buf_size, "def_%s.info", type_token);
    else
        snprintf(buf, buf_size, "-");
}

/**
 * @brief Build a single ListBrowser node for one type
 *
 * The "ascii" row is the parent/fallback; child types are indented two
 * spaces in the Type column to give visual hierarchy.
 *
 * @param type_token  Type name (e.g., "c", "rexx", "ascii")
 * @param is_parent   TRUE for the "ascii" row itself
 * @param is_excluded TRUE if the type appears in EXCLUDETYPE of def_ascii.info
 */
static struct Node *make_type_node(const char *type_token, BOOL is_parent, BOOL is_excluded)
{
    char display_type[64];
    char file_label[64];
    const char *status;

    if (is_parent)
        snprintf(display_type, sizeof(display_type), "%s", type_token);
    else
        snprintf(display_type, sizeof(display_type), "  %s", type_token);

    make_file_label(type_token, file_label, sizeof(file_label));
    if (is_parent)
        status = "Master";
    else if (is_excluded)
        status = "Excluded";
    else if (type_has_template(type_token))
        status = "Custom";
    else
        status = "Using master";

    return AllocListBrowserNode(3,
        LBNA_Column, COL_TYPE,
            LBNCA_CopyText, TRUE,
            LBNCA_Text, display_type,
        LBNA_Column, COL_FILE,
            LBNCA_CopyText, TRUE,
            LBNCA_Text, file_label,
        LBNA_Column, COL_STATUS,
            LBNCA_CopyText, TRUE,
            LBNCA_Text, status,
        TAG_DONE);
}

/*========================================================================*/
/* EXCLUDETYPE helpers                                                   */
/*========================================================================*/

/**
 * @brief Tokenise a comma-separated EXCLUDETYPE value into win->exclude_list[].
 *
 * Operates in-place on win->exclude_types[], storing pointers to each
 * whitespace-trimmed token.  Safe to call multiple times (overwrites state).
 */
static void parse_exclude_types(TextTemplatesWindow *win, const char *value)
{
    char *p;
    char *start;

    win->exclude_count = 0;

    if (!value || value[0] == '\0')
    {
        win->exclude_types[0] = '\0';
        return;
    }

    strncpy(win->exclude_types, value, sizeof(win->exclude_types) - 1);
    win->exclude_types[sizeof(win->exclude_types) - 1] = '\0';

    p     = win->exclude_types;
    start = p;
    while (*p)
    {
        if (*p == ',')
        {
            *p = '\0';
            /* Trim leading whitespace in token */
            while (*start == ' ') start++;
            if (start < p && win->exclude_count < 64)
                win->exclude_list[win->exclude_count++] = (STRPTR)start;
            start = p + 1;
        }
        p++;
    }
    /* Final token */
    while (*start == ' ') start++;
    if (*start != '\0' && win->exclude_count < 64)
        win->exclude_list[win->exclude_count++] = (STRPTR)start;
}

/**
 * @brief Load the EXCLUDETYPE tooltype from PROGDIR:Icons/def_ascii.info.
 *
 * Populates win->exclude_list[] and win->exclude_count.  Safe to call
 * when def_ascii.info does not exist or has no EXCLUDETYPE tooltype.
 */
static void load_exclude_types(TextTemplatesWindow *win)
{
    char             icon_path[256];
    struct DiskObject *dobj;
    STRPTR            tt_value;

    win->exclude_count   = 0;
    win->exclude_types[0] = '\0';

    snprintf(icon_path, sizeof(icon_path), "%sdef_ascii", TEMPLATE_DIR);
    dobj = GetDiskObject((STRPTR)icon_path);
    if (!dobj)
    {
        log_debug(LOG_GUI, "TextTemplates: def_ascii.info not found - no EXCLUDETYPE\n");
        return;
    }

    tt_value = (STRPTR)FindToolType((STRPTR *)dobj->do_ToolTypes, (STRPTR)"EXCLUDETYPE");
    if (tt_value)
    {
        log_info(LOG_GUI, "TextTemplates: EXCLUDETYPE=%s\n", tt_value);
        parse_exclude_types(win, (const char *)tt_value);
        log_info(LOG_GUI, "TextTemplates: %d type(s) excluded\n", win->exclude_count);
    }
    else
    {
        log_debug(LOG_GUI, "TextTemplates: no EXCLUDETYPE tooltype in def_ascii.info\n");
    }

    FreeDiskObject(dobj);
}

/**
 * @brief Return TRUE if type_token appears in the EXCLUDETYPE list.
 */
static BOOL is_type_excluded(const TextTemplatesWindow *win, const char *type_token)
{
    int i;
    for (i = 0; i < win->exclude_count; i++)
    {
        if (Stricmp(win->exclude_list[i], (STRPTR)type_token) == 0)
            return TRUE;
    }
    return FALSE;
}

/*========================================================================*/
/* ListBrowser population                                                */
/*========================================================================*/

/**
 * @brief Rebuild the ListBrowser contents from the subtype array
 */
static void rebuild_list(TextTemplatesWindow *win)
{
    struct Node *node;
    int i;

    /* Detach from gadget */
    SetGadgetAttrs((struct Gadget *)win->template_lb, win->window, NULL,
        LISTBROWSER_Labels, ~0,
        TAG_DONE);

    /* Free old nodes */
    while ((node = RemHead(win->lb_list)))
        FreeListBrowserNode(node);

    /* Add one node per subtype; also rebuild subtype_map so that
     * subtype_map[visual_row] == actual index into win->subtypes[].          */
    win->visible_count = 0;
    for (i = 0; i < win->subtype_count; i++)
    {
        BOOL has_custom = type_has_template(win->subtypes[i]);

        /* Apply "Show" filter - always include the master row (index 0) */
        if (i > 0)
        {
            if (win->show_filter == SHOW_CUSTOM_ONLY  && !has_custom) continue;
            if (win->show_filter == SHOW_MISSING_ONLY &&  has_custom) continue;
        }

        {
            BOOL type_is_excl = is_type_excluded(win, win->subtypes[i]);
            node = make_type_node(win->subtypes[i], i == 0, type_is_excl);
        }
        if (node)
        {
            AddTail(win->lb_list, node);
            if (win->visible_count < 512)
                win->subtype_map[win->visible_count] = i;
            win->visible_count++;
        }
    }

    /* Reattach and tell AutoFit to recalculate column widths for the new content */
    SetGadgetAttrs((struct Gadget *)win->template_lb, win->window, NULL,
        LISTBROWSER_Labels, win->lb_list,
        TAG_DONE);
    SetGadgetAttrs((struct Gadget *)win->template_lb, win->window, NULL,
        LISTBROWSER_AutoFit, TRUE,
        TAG_DONE);
}

/**
 * @brief Get the type token of the currently selected ListBrowser row.
 * @return Pointer into win->subtypes[], or NULL if nothing selected.
 */
static const char *get_selected_type(TextTemplatesWindow *win)
{
    ULONG selected = 0;

    GetAttr(LISTBROWSER_Selected, win->template_lb, &selected);
    if ((LONG)selected < 0 || (int)selected >= win->visible_count)
        return NULL;

    return win->subtypes[win->subtype_map[(int)selected]];
}

/**
 * @brief Translate a ListBrowser visual row index to an actual subtypes[] index.
 *
 * LISTBROWSER_Selected returns the visual row (0, 1, 2...) within the
 * currently displayed (possibly filtered) list.  This can differ from the
 * subtypes[] index when a Show filter is active.
 *
 * @return Actual subtypes[] index, or -1 if visual_row is out of range.
 */
static int visual_to_actual(const TextTemplatesWindow *win, ULONG visual_row)
{
    if ((LONG)visual_row < 0 || (int)visual_row >= win->visible_count)
        return -1;
    return win->subtype_map[(int)visual_row];
}

/*========================================================================*/
/* Requester helpers                                                     */
/*========================================================================*/

static void show_info_req(struct Window *parent, const char *title,
                          const char *body, ULONG image)
{
    Object *req;
    struct orRequest msg;

    req = NewObject(REQUESTER_GetClass(), NULL,
        REQ_Type,      REQTYPE_INFO,
        REQ_TitleText, title,
        REQ_BodyText,  body,
        REQ_GadgetText,"_OK",
        REQ_Image,     image,
        TAG_DONE);

    if (req)
    {
        msg.MethodID   = RM_OPENREQ;
        msg.or_Attrs   = NULL;
        msg.or_Window  = parent;
        msg.or_Screen  = NULL;
        DoMethodA(req, (Msg)&msg);
        DisposeObject(req);
    }
}

/** Returns TRUE if user confirmed (first/only button). */
static BOOL show_confirm_req(struct Window *parent, const char *title,
                             const char *body, const char *buttons)
{
    Object *req;
    struct orRequest msg;
    ULONG result = 0;

    req = NewObject(REQUESTER_GetClass(), NULL,
        REQ_Type,      REQTYPE_INFO,
        REQ_TitleText, title,
        REQ_BodyText,  body,
        REQ_GadgetText, buttons,
        REQ_Image,     REQIMAGE_QUESTION,
        TAG_DONE);

    if (req)
    {
        msg.MethodID   = RM_OPENREQ;
        msg.or_Attrs   = NULL;
        msg.or_Window  = parent;
        msg.or_Screen  = NULL;
        result = DoMethodA(req, (Msg)&msg);
        DisposeObject(req);
    }

    return (result == 1);
}

/**
 * @brief Show a requester explaining that the type is excluded via EXCLUDETYPE.
 *
 * Directs the user to remove the type from the EXCLUDETYPE tooltype in
 * def_ascii.info and reopen the window to edit it.
 */
static void show_excluded_req(struct Window *parent, const char *type_token)
{
    char body[512];
    char def_ascii_path[256];
    BPTR lock;

    /* Expand PROGDIR: to an absolute path so the user sees the real location */
    strncpy(def_ascii_path, "PROGDIR:Icons/def_ascii.info", sizeof(def_ascii_path) - 1);
    def_ascii_path[sizeof(def_ascii_path) - 1] = '\0';
    lock = Lock((STRPTR)"PROGDIR:Icons/def_ascii.info", ACCESS_READ);
    if (lock)
    {
        NameFromLock(lock, (STRPTR)def_ascii_path, sizeof(def_ascii_path));
        UnLock(lock);
    }

    snprintf(body, sizeof(body),
             "The type '%s' is excluded from receiving an icon\n"
             "via the EXCLUDETYPE tooltype in:\n"
             "  %s\n\n"
             "To edit this type, remove '%s' from the\n"
             "EXCLUDETYPE tooltype in def_ascii.info, then\n"
             "close and reopen the Text Templates window.",
             type_token, def_ascii_path, type_token);
    show_info_req(parent, "Type Excluded", body, REQIMAGE_WARNING);
}

/*========================================================================*/
/* Action: Create / Overwrite from master                                */
/*========================================================================*/

/**
 * Copy PROGDIR:Icons/def_ascii.info to PROGDIR:Icons/def_<type>.info.
 * The dynamic button label (Create/Overwrite) already reflects intent.
 * Prompts the user for confirmation if the destination already exists.
 */
static void handle_create_overwrite(TextTemplatesWindow *win)
{
    const char *type_token;
    char src_path[256];
    char dst_path[256];
    char abs_src[256];
    char abs_dst[256];
    char body[768];
    BPTR src_lock;
    BOOL do_copy;
    char *last_sep;

    type_token = get_selected_type(win);
    if (!type_token)
    {
        show_info_req(win->window, "Copy Template",
                      "No type selected.\nPlease select a type from the list first.",
                      REQIMAGE_INFO);
        return;
    }

    /* Refuse action if type is excluded via EXCLUDETYPE in def_ascii.info */
    if (is_type_excluded(win, type_token))
    {
        show_excluded_req(win->window, type_token);
        return;
    }

    if (strcmp(type_token, "ascii") == 0)
    {
        show_info_req(win->window, "Create from master",
                      "The 'ascii' type is the master template itself.\n"
                      "Select a sub-type (e.g., c, rexx) to create a custom template.",
                      REQIMAGE_INFO);
        return;
    }

    snprintf(src_path, sizeof(src_path), "%sdef_ascii.info", TEMPLATE_DIR);
    snprintf(dst_path, sizeof(dst_path), "%sdef_%s.info", TEMPLATE_DIR, type_token);

    /* Check source exists */
    src_lock = Lock((STRPTR)src_path, ACCESS_READ);
    if (!src_lock)
    {
        snprintf(body, sizeof(body),
                 "Source template not found:\n%s\n\n"
                 "Please ensure def_ascii.info exists in PROGDIR:Icons/.",
                 src_path);
        show_info_req(win->window, "Copy Template", body, REQIMAGE_WARNING);
        return;
    }

    /* Expand PROGDIR: to absolute path for display in requesters.
     * NameFromLock() gives us the real volume path (e.g. PC:Programming/...).
     * We derive abs_dst from the same directory since dst may not exist yet. */
    strncpy(abs_src, src_path, sizeof(abs_src) - 1);
    abs_src[sizeof(abs_src) - 1] = '\0';
    if (NameFromLock(src_lock, (STRPTR)abs_src, sizeof(abs_src)))
    {
        /* Build abs_dst: replace the filename portion with def_<type>.info */
        strncpy(abs_dst, abs_src, sizeof(abs_dst) - 1);
        abs_dst[sizeof(abs_dst) - 1] = '\0';
        last_sep = NULL;
        {
            char *p = abs_dst;
            while (*p)
            {
                if (*p == '/' || *p == ':')
                    last_sep = p;
                p++;
            }
        }
        if (last_sep)
            snprintf(last_sep + 1, sizeof(abs_dst) - (int)(last_sep + 1 - abs_dst),
                     "def_%s.info", type_token);
        else
            strncpy(abs_dst, dst_path, sizeof(abs_dst) - 1);
    }
    else
    {
        /* NameFromLock failed - fall back to PROGDIR: paths */
        strncpy(abs_dst, dst_path, sizeof(abs_dst) - 1);
        abs_dst[sizeof(abs_dst) - 1] = '\0';
    }
    UnLock(src_lock);

    /* Warn if destination already exists */
    if (type_has_template(type_token))
    {
        snprintf(body, sizeof(body),
                 "def_%s.info already exists.\n\n"
                 "Overwrite with a copy of def_ascii.info?\n\n"
                 "Source:      %s\n"
                 "Destination: %s",
                 type_token, abs_src, abs_dst);
        do_copy = show_confirm_req(win->window, "Overwrite?", body, "_Overwrite|_Cancel");
    }
    else
    {
        snprintf(body, sizeof(body),
                 "Copy def_ascii.info to def_%s.info?\n\n"
                 "Source:      %s\n"
                 "Destination: %s",
                 type_token, abs_src, abs_dst);
        do_copy = show_confirm_req(win->window, "Copy Template", body, "_Copy|_Cancel");
    }

    if (!do_copy)
        return;

    /* Perform copy */
    if (deficons_copy_icon_file(src_path, dst_path))
    {
        snprintf(body, sizeof(body), "Template copied successfully.\n\ndef_%s.info created.", type_token);
        show_info_req(win->window, "Copy Template", body, REQIMAGE_INFO);
        log_info(LOG_GUI, "TextTemplates: copied def_ascii.info to def_%s.info\n", type_token);
        rebuild_list(win);
    }
    else
    {
        snprintf(body, sizeof(body),
                 "Failed to copy template.\n\n"
                 "Source:      %s\n"
                 "Destination: %s\n\n"
                 "Check that the destination directory is writable.",
                 abs_src, abs_dst);
        show_info_req(win->window, "Copy Failed", body, REQIMAGE_ERROR);
        log_error(LOG_GUI, "TextTemplates: copy failed for def_%s.info\n", type_token);
    }
}

/*========================================================================*/
/* Action: Validate ToolTypes                                            */
/*========================================================================*/

/* All known ITIDY_ ToolType keys for text template icons.
 * Keep sorted for readability. */
static const char *s_known_itidy_keys[] = {
    ITIDY_TT_ADAPTIVE_TEXT,
    ITIDY_TT_BG_COLOR,
    ITIDY_TT_CHAR_WIDTH,
    ITIDY_TT_DARKEN_ALT_PERCENT,
    ITIDY_TT_DARKEN_PERCENT,
    ITIDY_TT_EXCLUDE_AREA,
    ITIDY_TT_EXPAND_PALETTE,
    ITIDY_TT_LINE_GAP,
    ITIDY_TT_LINE_HEIGHT,
    ITIDY_TT_MAX_LINES,
    ITIDY_TT_MID_COLOR,
    ITIDY_TT_READ_BYTES,
    ITIDY_TT_TEXT_AREA,
    ITIDY_TT_TEXT_COLOR,
    /* Template icon metadata keys (read-only/informational) */
    "EXCLUDETYPE",
    NULL
};

/**
 * @brief Check whether 'key' starts with "ITIDY_" and is an unknown key.
 *
 * @param key ToolType key string (e.g., "ITIDY_BG_COLOR" or "TOOLTYPE")
 * @return TRUE if unknown ITIDY_ key (potential typo / unsupported key)
 */
static BOOL is_unknown_itidy_key(const char *key)
{
    int i;

    /* Only flag keys that start with ITIDY_ */
    if (Strnicmp(key, "ITIDY_", 6) != 0)
        return FALSE;

    for (i = 0; s_known_itidy_keys[i] != NULL; i++)
    {
        if (Stricmp(key, s_known_itidy_keys[i]) == 0)
            return FALSE;  /* Known */
    }
    return TRUE;  /* Unknown ITIDY_ key */
}

/**
 * @brief Parse "x,y,w,h" from a ToolType value.
 * @return TRUE if all four components are valid non-negative integers.
 */
static BOOL parse_area_valid(const char *val)
{
    int x, y, w, h;
    return (sscanf(val, "%d,%d,%d,%d", &x, &y, &w, &h) == 4 &&
            w > 0 && h > 0);
}

/**
 * @brief Validate the ToolTypes of a template icon.
 *
 * Checks:
 *  - All ITIDY_* keys are in the known list (warns on unknown)
 *  - Colour index keys (BG_COLOR, TEXT_COLOR, MID_COLOR) are 0-255
 *  - DARKEN_PERCENT / DARKEN_ALT_PERCENT are 0-100
 *  - TEXT_AREA / EXCLUDE_AREA match "x,y,w,h" format with w,h > 0
 *  - MAX_LINES / LINE_HEIGHT / LINE_GAP / CHAR_WIDTH / READ_BYTES are > 0
 */
static void handle_check_tooltypes(TextTemplatesWindow *win)
{
    const char *type_token;
    char icon_path[256];
    char report[2048];
    char line[256];
    struct DiskObject *dobj;
    STRPTR *tool_types;
    int i;
    int warning_count = 0;
    int checked_count = 0;

    type_token = get_selected_type(win);
    if (!type_token)
    {
        show_info_req(win->window, "Check ToolTypes",
                      "No type selected.\nSelect a type from the list first.",
                      REQIMAGE_INFO);
        return;
    }

    /* Refuse action if type is excluded via EXCLUDETYPE in def_ascii.info */
    if (is_type_excluded(win, type_token))
    {
        show_excluded_req(win->window, type_token);
        return;
    }

    /* Build path (without .info suffix - GetDiskObject adds it) */
    /* Resolve effective template: custom if it exists, else master (def_ascii.info) */
    {
        const char *eff_token = type_has_template(type_token) ? type_token : "ascii";
        snprintf(icon_path, sizeof(icon_path), "%sdef_%s", TEMPLATE_DIR, eff_token);
    }

    dobj = GetDiskObject((STRPTR)icon_path);
    if (!dobj)
    {
        snprintf(report, sizeof(report),
                 "No template icon found.\n\n"
                 "Ensure def_ascii.info exists in PROGDIR:Icons/.");
        show_info_req(win->window, "Validate ToolTypes", report, REQIMAGE_WARNING);
        return;
    }

    tool_types = (STRPTR *)dobj->do_ToolTypes;
    snprintf(report, sizeof(report), "ToolType check for def_%s.info:\n\n", type_token);

    if (tool_types)
    {
        for (i = 0; tool_types[i] != NULL; i++)
        {
            char key[128];
            char value[128];
            char *eq;

            strncpy(key, tool_types[i], sizeof(key) - 1);
            key[sizeof(key) - 1] = '\0';

            /* Strip leading '(' for disabled tooltypes */
            if (key[0] == '(')
                memmove(key, key + 1, strlen(key));

            eq = strchr(key, '=');
            if (eq)
            {
                *eq = '\0';
                strncpy(value, eq + 1, sizeof(value) - 1);
                value[sizeof(value) - 1] = '\0';
                /* Strip trailing ')' for disabled tooltypes */
                {
                    int vlen = strlen(value);
                    if (vlen > 0 && value[vlen - 1] == ')')
                        value[vlen - 1] = '\0';
                }
            }
            else
            {
                value[0] = '\0';
            }

            checked_count++;

            /* Check for unknown ITIDY_ key */
            if (is_unknown_itidy_key(key))
            {
                snprintf(line, sizeof(line),
                         "WARNING: Unknown key '%s'\n", key);
                strncat(report, line,
                        sizeof(report) - strlen(report) - 1);
                warning_count++;
                continue;
            }

            /* Validate colour index keys */
            if (Stricmp(key, ITIDY_TT_BG_COLOR)   == 0 ||
                Stricmp(key, ITIDY_TT_TEXT_COLOR)  == 0 ||
                Stricmp(key, ITIDY_TT_MID_COLOR)   == 0)
            {
                int v = 0;
                if (sscanf(value, "%d", &v) != 1 || v < 0 || v > 255)
                {
                    snprintf(line, sizeof(line),
                             "WARNING: %s value '%s' out of range 0..255\n",
                             key, value);
                    strncat(report, line,
                            sizeof(report) - strlen(report) - 1);
                    warning_count++;
                }
                continue;
            }

            /* Validate percentage keys */
            if (Stricmp(key, ITIDY_TT_DARKEN_PERCENT)     == 0 ||
                Stricmp(key, ITIDY_TT_DARKEN_ALT_PERCENT) == 0)
            {
                int v = 0;
                if (sscanf(value, "%d", &v) != 1 || v < 0 || v > 100)
                {
                    snprintf(line, sizeof(line),
                             "WARNING: %s value '%s' out of range 0..100\n",
                             key, value);
                    strncat(report, line,
                            sizeof(report) - strlen(report) - 1);
                    warning_count++;
                }
                continue;
            }

            /* Validate area rectangle keys */
            if (Stricmp(key, ITIDY_TT_TEXT_AREA)    == 0 ||
                Stricmp(key, ITIDY_TT_EXCLUDE_AREA) == 0)
            {
                if (!parse_area_valid(value))
                {
                    snprintf(line, sizeof(line),
                             "WARNING: %s value '%s' invalid (expected x,y,w,h)\n",
                             key, value);
                    strncat(report, line,
                            sizeof(report) - strlen(report) - 1);
                    warning_count++;
                }
                continue;
            }

            /* Validate positive integer keys */
            if (Stricmp(key, ITIDY_TT_MAX_LINES)   == 0 ||
                Stricmp(key, ITIDY_TT_LINE_HEIGHT)  == 0 ||
                Stricmp(key, ITIDY_TT_LINE_GAP)     == 0 ||
                Stricmp(key, ITIDY_TT_CHAR_WIDTH)   == 0 ||
                Stricmp(key, ITIDY_TT_READ_BYTES)   == 0)
            {
                int v = 0;
                if (sscanf(value, "%d", &v) != 1 || v <= 0)
                {
                    snprintf(line, sizeof(line),
                             "WARNING: %s value '%s' must be > 0\n",
                             key, value);
                    strncat(report, line,
                            sizeof(report) - strlen(report) - 1);
                    warning_count++;
                }
                continue;
            }
        }
    }

    /* --- Build human-readable effect summary from the same ToolTypes --- */
    {
        /* Collected values (defaults shown when key absent) */
        char v_text_area[32]    = "(not set)";
        char v_exclude_area[32] = "(none)";
        char v_max_lines[16]    = "?";
        char v_line_h[16]       = "?";
        char v_line_gap[16]     = "0";
        char v_char_w[16]       = "?";
        char v_bg[16]           = "?";
        char v_text_col[16]     = "?";
        char v_mid[16]          = "(none)";
        char v_darken[16]       = "0";
        char v_darken_alt[16]   = "0";
        char v_adaptive[16]     = "NO";
        char v_expand[16]       = "NO";
        char v_read[16]         = "?";
        char excludetypes[256]  = "(none)";
        int et_count = 0;
        char summary[1024];
        char sline[128];

        /* Re-scan ToolTypes to collect values for the summary */
        tool_types = (STRPTR *)dobj->do_ToolTypes;
        if (tool_types)
        {
            for (i = 0; tool_types[i] != NULL; i++)
            {
                char sk[128], sv[128], *seq;
                strncpy(sk, tool_types[i], sizeof(sk) - 1);
                sk[sizeof(sk) - 1] = '\0';
                if (sk[0] == '(') memmove(sk, sk + 1, strlen(sk));
                seq = strchr(sk, '=');
                if (seq) { *seq = '\0'; strncpy(sv, seq+1, sizeof(sv)-1); sv[sizeof(sv)-1] = '\0'; { int l=(int)strlen(sv); if (l>0 && sv[l-1]==')') sv[l-1]='\0'; } }
                else      { sv[0] = '\0'; }

                if (Stricmp(sk, ITIDY_TT_TEXT_AREA)        == 0) strncpy(v_text_area,    sv, sizeof(v_text_area)-1);
                else if (Stricmp(sk, ITIDY_TT_EXCLUDE_AREA)     == 0) strncpy(v_exclude_area, sv, sizeof(v_exclude_area)-1);
                else if (Stricmp(sk, ITIDY_TT_MAX_LINES)        == 0) strncpy(v_max_lines,    sv, sizeof(v_max_lines)-1);
                else if (Stricmp(sk, ITIDY_TT_LINE_HEIGHT)      == 0) strncpy(v_line_h,       sv, sizeof(v_line_h)-1);
                else if (Stricmp(sk, ITIDY_TT_LINE_GAP)         == 0) strncpy(v_line_gap,     sv, sizeof(v_line_gap)-1);
                else if (Stricmp(sk, ITIDY_TT_CHAR_WIDTH)       == 0) strncpy(v_char_w,       sv, sizeof(v_char_w)-1);
                else if (Stricmp(sk, ITIDY_TT_BG_COLOR)         == 0) strncpy(v_bg,           sv, sizeof(v_bg)-1);
                else if (Stricmp(sk, ITIDY_TT_TEXT_COLOR)       == 0) strncpy(v_text_col,     sv, sizeof(v_text_col)-1);
                else if (Stricmp(sk, ITIDY_TT_MID_COLOR)        == 0) strncpy(v_mid,          sv, sizeof(v_mid)-1);
                else if (Stricmp(sk, ITIDY_TT_DARKEN_PERCENT)   == 0) strncpy(v_darken,       sv, sizeof(v_darken)-1);
                else if (Stricmp(sk, ITIDY_TT_DARKEN_ALT_PERCENT)==0) strncpy(v_darken_alt,   sv, sizeof(v_darken_alt)-1);
                else if (Stricmp(sk, ITIDY_TT_ADAPTIVE_TEXT)    == 0) strncpy(v_adaptive,     sv, sizeof(v_adaptive)-1);
                else if (Stricmp(sk, ITIDY_TT_EXPAND_PALETTE)   == 0) strncpy(v_expand,       sv, sizeof(v_expand)-1);
                else if (Stricmp(sk, ITIDY_TT_READ_BYTES)       == 0) strncpy(v_read,         sv, sizeof(v_read)-1);
                else if (Stricmp(sk, "EXCLUDETYPE") == 0)
                {
                    if (et_count == 0)
                        strncpy(excludetypes, sv, sizeof(excludetypes)-1);
                    else
                    {
                        strncat(excludetypes, ", ", sizeof(excludetypes)-strlen(excludetypes)-1);
                        strncat(excludetypes, sv,   sizeof(excludetypes)-strlen(excludetypes)-1);
                    }
                    et_count++;
                }
            }
        }

        /* Compose summary block */
        snprintf(summary, sizeof(summary), "\n--- Effect Summary ---\n");

        snprintf(sline, sizeof(sline), "Text area:    %s\n", v_text_area);
        strncat(summary, sline, sizeof(summary)-strlen(summary)-1);

        snprintf(sline, sizeof(sline), "Exclude area: %s\n", v_exclude_area);
        strncat(summary, sline, sizeof(summary)-strlen(summary)-1);

        snprintf(sline, sizeof(sline), "Lines: max %s, height %s, gap %s\n",
                 v_max_lines, v_line_h, v_line_gap);
        strncat(summary, sline, sizeof(summary)-strlen(summary)-1);

        snprintf(sline, sizeof(sline), "Char width: %s, Read: %s bytes\n",
                 v_char_w, v_read);
        strncat(summary, sline, sizeof(summary)-strlen(summary)-1);

        snprintf(sline, sizeof(sline), "Colours: bg=%s, text=%s, mid=%s\n",
                 v_bg, v_text_col, v_mid);
        strncat(summary, sline, sizeof(summary)-strlen(summary)-1);

        snprintf(sline, sizeof(sline), "Darken: %s%%, alt %s%%\n",
                 v_darken, v_darken_alt);
        strncat(summary, sline, sizeof(summary)-strlen(summary)-1);

        snprintf(sline, sizeof(sline), "Adaptive: %s, Expand palette: %s\n",
                 v_adaptive, v_expand);
        strncat(summary, sline, sizeof(summary)-strlen(summary)-1);

        snprintf(sline, sizeof(sline), "Exclude types: %s\n", excludetypes);
        strncat(summary, sline, sizeof(summary)-strlen(summary)-1);

        strncat(report, summary, sizeof(report)-strlen(report)-1);
    }

    FreeDiskObject(dobj);

    if (checked_count == 0)
    {
        strncat(report, "No ToolTypes found in this icon.\n",
                sizeof(report) - strlen(report) - 1);
    }
    else if (warning_count == 0)
    {
        snprintf(line, sizeof(line),
                 "\nAll %d ToolType(s) valid.\n", checked_count);
        strncat(report, line, sizeof(report) - strlen(report) - 1);
    }
    else
    {
        snprintf(line, sizeof(line),
                 "\n%d warning(s) in %d ToolType(s).\n",
                 warning_count, checked_count);
        strncat(report, line, sizeof(report) - strlen(report) - 1);
    }

    show_info_req(win->window, "ToolType Validation", report,
                  warning_count > 0 ? REQIMAGE_WARNING : REQIMAGE_INFO);
}

/*========================================================================*/
/* Action: Open WB Information                                           */
/*========================================================================*/

/**
 * Send an ARexx "Info" command to the WORKBENCH port so Workbench opens
 * its standard Information dialog for the selected template icon.
 *
 * The Workbench ARexx port accepts:
 *   Info "<drawer>" "<file>"
 *
 * PROGDIR: is used as the drawer; the file is "Icons/def_<type>.info".
 * Workbench resolves PROGDIR: from the executable's home directory.
 *
 * This is fire-and-forget: we do not wait for a reply (the dialog is
 * independent of our task).
 */
static void handle_wbinfo(TextTemplatesWindow *win)
{
    const char *type_token;
    char command[512];
    char full_path[256];
    BPTR lock;
    struct RxsLib *RexxSysBase_local = NULL;
    struct MsgPort *wb_port;
    struct MsgPort *reply_port;
    struct RexxMsg *rxmsg;

    type_token = get_selected_type(win);
    if (!type_token)
    {
        show_info_req(win->window, "WB Information",
                      "No type selected.\nSelect a type from the list first.",
                      REQIMAGE_INFO);
        return;
    }

    /* Refuse action if type is excluded via EXCLUDETYPE in def_ascii.info */
    if (is_type_excluded(win, type_token))
    {
        show_excluded_req(win->window, type_token);
        return;
    }

    /* Resolve effective template: custom if it exists, else master (def_ascii.info) */
    {
        const char *eff_token = type_has_template(type_token) ? type_token : "ascii";
        snprintf(full_path, sizeof(full_path), "%sdef_%s.info", TEMPLATE_DIR, eff_token);
    }
    lock = Lock((STRPTR)full_path, ACCESS_READ);
    if (!lock)
    {
        char body[512];
        snprintf(body, sizeof(body),
                 "Template file not found.\n\n"
                 "Ensure def_ascii.info exists in PROGDIR:Icons/.");
        show_info_req(win->window, "WB Information", body, REQIMAGE_WARNING);
        return;
    }

    /* Expand PROGDIR: (and any other assigns) to an absolute path.
     * WORKBENCH runs ARexx commands in its own context and does not
     * inherit iTidy's assigns, so PROGDIR: would be unresolvable there. */
    {
        char abs_path[256];
        if (NameFromLock(lock, (STRPTR)abs_path, sizeof(abs_path)))
        {
            strncpy(full_path, abs_path, sizeof(full_path) - 1);
            full_path[sizeof(full_path) - 1] = '\0';
            log_debug(LOG_GUI, "TextTemplates: expanded to absolute path: %s\n", full_path);
        }
        else
        {
            log_warning(LOG_GUI, "TextTemplates: NameFromLock failed, using original path\n");
        }
    }
    UnLock(lock);

    /* Strip the .info suffix before passing to WB INFO command.
     * Workbench's INFO ARexx command takes the OBJECT name (e.g. "def_rexx"),
     * not the icon filename (e.g. "def_rexx.info").
     * Passing .info causes WB to open the .info file as a plain file,
     * showing a placeholder icon and no Tool Types.
     * The title bar in a manually-opened WB Info window confirms:
     *   correct path = "PC:.../def_rexx"  (no .info suffix) */
    {
        int flen = (int)strlen(full_path);
        if (flen > 5 && strcmp(full_path + flen - 5, ".info") == 0)
        {
            full_path[flen - 5] = '\0';
        }
    }

    /* Workbench's INFO ARexx command requires a real file to exist at the
     * given path - it will not open the icon info window for a standalone
     * .info file that has no corresponding file (no "def_rexx" alongside
     * "def_rexx.info" causes rc=10, lasterror=205 "object not found").
     *
     * NOTE: These DefIcons template files are standalone icons with no
     * matching data file by design. We therefore create a zero-byte
     * companion file if one does not already exist. This is purely a
     * display helper - the zero-byte file has no functional purpose
     * other than satisfying Workbench's object lookup so INFO works. */
    {
        BPTR companion_lock = Lock((STRPTR)full_path, ACCESS_READ);
        if (companion_lock == 0)
        {
            /* Companion file does not exist - create a zero-byte one */
            BPTR new_file = Open((STRPTR)full_path, MODE_NEWFILE);
            if (new_file != 0)
            {
                Close(new_file);
                log_debug(LOG_GUI, "TextTemplates: created zero-byte companion file: %s\n", full_path);
            }
            else
            {
                log_warning(LOG_GUI, "TextTemplates: failed to create companion file: %s\n", full_path);
            }
        }
        else
        {
            UnLock(companion_lock);
        }
    }

    /* Build the INFO NAME command.
     * Correct WB ARexx syntax:  INFO NAME "fullpath"
     * The path must be absolute (PROGDIR: already expanded above). */
    snprintf(command, sizeof(command), "Info NAME \"%s\"", full_path);

    log_info(LOG_GUI, "TextTemplates: sending WB ARexx command: %s\n", command);

    /* Open rexxsyslib.library locally */
    RexxSysBase_local = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 0);
    if (!RexxSysBase_local)
    {
        show_info_req(win->window, "WB Information",
                      "Failed to open rexxsyslib.library.\n"
                      "Cannot send Info command to Workbench.",
                      REQIMAGE_ERROR);
        return;
    }

    Forbid();
    wb_port = FindPort((STRPTR)"WORKBENCH");
    Permit();

    if (!wb_port)
    {
        CloseLibrary((struct Library *)RexxSysBase_local);
        show_info_req(win->window, "WB Information",
                      "Workbench ARexx port not found.\n\n"
                      "Workbench must be running with ARexx\n"
                      "support enabled.",
                      REQIMAGE_WARNING);
        return;
    }

    /* Create a reply port so we can send a proper RexxMsg */
    reply_port = CreateMsgPort();
    if (!reply_port)
    {
        CloseLibrary((struct Library *)RexxSysBase_local);
        show_info_req(win->window, "WB Information",
                      "Failed to create reply port.", REQIMAGE_ERROR);
        return;
    }

    rxmsg = CreateRexxMsg(reply_port, NULL, NULL);
    if (!rxmsg)
    {
        DeleteMsgPort(reply_port);
        CloseLibrary((struct Library *)RexxSysBase_local);
        show_info_req(win->window, "WB Information",
                      "Failed to create ARexx message.", REQIMAGE_ERROR);
        return;
    }

    rxmsg->rm_Args[0] = CreateArgstring((STRPTR)command, strlen(command));
    if (!rxmsg->rm_Args[0])
    {
        DeleteRexxMsg(rxmsg);
        DeleteMsgPort(reply_port);
        CloseLibrary((struct Library *)RexxSysBase_local);
        show_info_req(win->window, "WB Information",
                      "Failed to create ARexx arg string.", REQIMAGE_ERROR);
        return;
    }

    /* Request a result so we can log the return code and any error message */
    rxmsg->rm_Action = RXFF_RESULT;
    PutMsg(wb_port, (struct Message *)rxmsg);

    /* Wait for reply and log what Workbench tells us */
    {
        ULONG sigmask = 1L << reply_port->mp_SigBit;
        Wait(sigmask);
        GetMsg(reply_port);

        if (rxmsg->rm_Result1 == 0)
        {
            log_info(LOG_GUI, "TextTemplates: WB Info opened OK for def_%s (rc=0)\n",
                     type_token);
        }
        else
        {
            /* rm_Result2 is a numeric LASTERROR code when rc != 0,
             * NOT a string pointer - log it as a decimal number. */
            log_warning(LOG_GUI,
                "TextTemplates: WB Info failed for def_%s (rc=%ld, lasterror=%ld)\n",
                type_token, rxmsg->rm_Result1, (LONG)rxmsg->rm_Result2);
        }
    }

    if (rxmsg->rm_Args[0])
        DeleteArgstring(rxmsg->rm_Args[0]);
    DeleteRexxMsg(rxmsg);
    DeleteMsgPort(reply_port);
    CloseLibrary((struct Library *)RexxSysBase_local);

    log_info(LOG_GUI, "TextTemplates: WB Info command sent for def_%s\n", type_token);
}

/*========================================================================*/
/* Selected Type panel update                                            */
/*========================================================================*/

/**
 * @brief Update the "Selected Type" read-only info panel and button states.
 *
 * Called after list selection changes, after any file-level action, and
 * on window open (with selected_idx = -1 for no-selection state).
 *
 * @param win          Window state
 * @param selected_idx Index into win->subtypes[], or -1 for no selection
 */
static void update_selected_type_panel(TextTemplatesWindow *win, int selected_idx)
{
    BOOL has_selection;
    BOOL is_master;
    BOOL has_custom;

    has_selection = (selected_idx >= 0 && selected_idx < win->subtype_count);

    if (!has_selection)
    {
        /* No-selection state: blank all fields, disable all action buttons */
        strncpy(win->buf_type,      "(none)", sizeof(win->buf_type) - 1);
        strncpy(win->buf_tmplfile,  "-",      sizeof(win->buf_tmplfile) - 1);
        strncpy(win->buf_effective, "-",      sizeof(win->buf_effective) - 1);
        strncpy(win->buf_status,    "-",      sizeof(win->buf_status) - 1);

        SetGadgetAttrs((struct Gadget *)win->selected_type,      win->window, NULL, GA_Text, win->buf_type,      TAG_DONE);
        SetGadgetAttrs((struct Gadget *)win->selected_tmplfile,  win->window, NULL, GA_Text, win->buf_tmplfile,  TAG_DONE);
        SetGadgetAttrs((struct Gadget *)win->selected_effective, win->window, NULL, GA_Text, win->buf_effective, TAG_DONE);
        SetGadgetAttrs((struct Gadget *)win->selected_status,    win->window, NULL, GA_Text, win->buf_status,    TAG_DONE);

        SetGadgetAttrs((struct Gadget *)win->btn_create_overwrite, win->window, NULL, GA_Disabled, TRUE, TAG_DONE);
        SetGadgetAttrs((struct Gadget *)win->btn_edit_tooltypes,   win->window, NULL, GA_Disabled, TRUE, TAG_DONE);
        SetGadgetAttrs((struct Gadget *)win->btn_validate,         win->window, NULL, GA_Disabled, TRUE, TAG_DONE);
        SetGadgetAttrs((struct Gadget *)win->btn_revert,           win->window, NULL, GA_Disabled, TRUE, TAG_DONE);
        return;
    }

    /* Resolve type attributes */
    is_master  = (strcmp(win->subtypes[selected_idx], "ascii") == 0);
    has_custom = type_has_template(win->subtypes[selected_idx]);

    {
        BOOL is_excluded = is_type_excluded(win, win->subtypes[selected_idx]);

    /* Type name */
    strncpy(win->buf_type, win->subtypes[selected_idx], sizeof(win->buf_type) - 1);
    win->buf_type[sizeof(win->buf_type) - 1] = '\0';

    /* Template file: the actual file on disk for this type (or "-") */
    if (is_master)
        strncpy(win->buf_tmplfile, "def_ascii.info", sizeof(win->buf_tmplfile) - 1);
    else if (has_custom)
        snprintf(win->buf_tmplfile, sizeof(win->buf_tmplfile), "def_%s.info", win->subtypes[selected_idx]);
    else
        strncpy(win->buf_tmplfile, "-", sizeof(win->buf_tmplfile) - 1);
    win->buf_tmplfile[sizeof(win->buf_tmplfile) - 1] = '\0';

    /* Effective file: excluded types have no effective template from iTidy's
     * perspective, so show "-"; otherwise show the filename in use. */
    if (is_excluded)
        strncpy(win->buf_effective, "-", sizeof(win->buf_effective) - 1);
    else if (has_custom || is_master)
        snprintf(win->buf_effective, sizeof(win->buf_effective),
                 "def_%s.info", win->subtypes[selected_idx]);
    else
        strncpy(win->buf_effective, "def_ascii.info", sizeof(win->buf_effective) - 1);
    win->buf_effective[sizeof(win->buf_effective) - 1] = '\0';

    /* Status */
        if (is_master)
            strncpy(win->buf_status, "Master", sizeof(win->buf_status) - 1);
        else if (is_excluded)
            strncpy(win->buf_status, "Excluded", sizeof(win->buf_status) - 1);
        else if (has_custom)
            strncpy(win->buf_status, "Custom", sizeof(win->buf_status) - 1);
        else
            strncpy(win->buf_status, "Using master", sizeof(win->buf_status) - 1);
        win->buf_status[sizeof(win->buf_status) - 1] = '\0';

    /* Update the four display buttons */
    SetGadgetAttrs((struct Gadget *)win->selected_type,      win->window, NULL, GA_Text, win->buf_type,      TAG_DONE);
    SetGadgetAttrs((struct Gadget *)win->selected_tmplfile,  win->window, NULL, GA_Text, win->buf_tmplfile,  TAG_DONE);
    SetGadgetAttrs((struct Gadget *)win->selected_effective, win->window, NULL, GA_Text, win->buf_effective, TAG_DONE);
    SetGadgetAttrs((struct Gadget *)win->selected_status,    win->window, NULL, GA_Text, win->buf_status,    TAG_DONE);

    /* Dynamic Create/Overwrite label + button enabled/disabled state.
         * When a type is excluded all action buttons are kept enabled so the
         * user can click them and receive the "type excluded" explanation. */
        if (is_excluded)
        {
            strncpy(win->buf_create_label, "Create from master", sizeof(win->buf_create_label) - 1);
            win->buf_create_label[sizeof(win->buf_create_label) - 1] = '\0';

            SetGadgetAttrs((struct Gadget *)win->btn_create_overwrite, win->window, NULL,
                           GA_Text,     win->buf_create_label,
                           GA_Disabled, FALSE,
                           TAG_DONE);
            SetGadgetAttrs((struct Gadget *)win->btn_edit_tooltypes, win->window, NULL, GA_Disabled, FALSE, TAG_DONE);
            SetGadgetAttrs((struct Gadget *)win->btn_validate,       win->window, NULL, GA_Disabled, FALSE, TAG_DONE);
            SetGadgetAttrs((struct Gadget *)win->btn_revert,         win->window, NULL, GA_Disabled, FALSE, TAG_DONE);
        }
        else
        {
            /* Dynamic Create/Overwrite label */
            if (has_custom)
                strncpy(win->buf_create_label, "Overwrite from master", sizeof(win->buf_create_label) - 1);
            else
                strncpy(win->buf_create_label, "Create from master", sizeof(win->buf_create_label) - 1);
            win->buf_create_label[sizeof(win->buf_create_label) - 1] = '\0';

            /* Create/Overwrite: disabled for master type */
            SetGadgetAttrs((struct Gadget *)win->btn_create_overwrite, win->window, NULL,
                           GA_Text,     win->buf_create_label,
                           GA_Disabled, is_master,
                           TAG_DONE);

            /* Edit/Validate: always enabled when something is selected */
            SetGadgetAttrs((struct Gadget *)win->btn_edit_tooltypes, win->window, NULL, GA_Disabled, FALSE, TAG_DONE);
            SetGadgetAttrs((struct Gadget *)win->btn_validate,       win->window, NULL, GA_Disabled, FALSE, TAG_DONE);

            /* Revert: only for non-master types that actually have a custom template */
            SetGadgetAttrs((struct Gadget *)win->btn_revert, win->window, NULL,
                           GA_Disabled, (is_master || !has_custom),
                           TAG_DONE);
        }
    } /* end is_excluded scope */

    log_debug(LOG_GUI, "TextTemplates: panel updated for '%s' (master=%d custom=%d excl=%d)\n",
              win->subtypes[selected_idx], (int)is_master, (int)has_custom,
              (int)is_type_excluded(win, win->subtypes[selected_idx]));
}

/*========================================================================*/
/* Action: Revert to master                                              */
/*========================================================================*/

/**
 * Delete the custom template icon for the selected type, reverting it
 * to use the master (def_ascii.info) as its effective template.
 */
static void handle_revert(TextTemplatesWindow *win)
{
    const char *type_token;
    char        custom_path[256];
    char        confirm_body[512];
    ULONG       selected = 0;

    type_token = get_selected_type(win);
    if (!type_token)
        return;

    /* Refuse action if type is excluded via EXCLUDETYPE in def_ascii.info */
    if (is_type_excluded(win, type_token))
    {
        show_excluded_req(win->window, type_token);
        return;
    }

    if (strcmp(type_token, "ascii") == 0)
        return;  /* Button should already be disabled, but guard anyway */

    if (!type_has_template(type_token))
        return;  /* No custom template to revert */

    snprintf(custom_path, sizeof(custom_path), "%sdef_%s.info", TEMPLATE_DIR, type_token);

    snprintf(confirm_body, sizeof(confirm_body),
             "Delete the custom template for type '%s'\n"
             "and use the master template instead?\n\n"
             "File: %s",
             type_token, custom_path);

    if (!show_confirm_req(win->window, "Revert to master",
                          confirm_body, "_Revert|_Cancel"))
        return;

    if (!DeleteFile((STRPTR)custom_path))
    {
        LONG err = IoErr();
        /* If file was already gone, treat it as a successful revert */
        if (err != ERROR_OBJECT_NOT_FOUND)
        {
            char err_body[512];
            snprintf(err_body, sizeof(err_body),
                     "Failed to delete custom template.\n\n"
                     "File: %s\n"
                     "Error code: %ld\n\n"
                     "Check the file is not write-protected or locked.",
                     custom_path, (LONG)err);
            show_info_req(win->window, "Revert Failed", err_body, REQIMAGE_ERROR);
            log_error(LOG_GUI, "TextTemplates: revert failed for def_%s.info (IoErr=%ld)\n",
                      type_token, (LONG)err);
            return;
        }
    }

    log_info(LOG_GUI, "TextTemplates: reverted def_%s.info - now using master\n", type_token);

    /* Refresh list and update panel.
     * Map visual row -> subtypes[] index BEFORE rebuild (rebuild resets
     * subtype_map).  After rebuild, check whether the type is still visible
     * (e.g. it disappears from "Custom only" view once the file is deleted). */
    {
        int actual_idx, j;
        GetAttr(LISTBROWSER_Selected, win->template_lb, &selected);
        actual_idx = visual_to_actual(win, selected);
        rebuild_list(win);
        for (j = 0; j < win->visible_count; j++)
        {
            if (win->subtype_map[j] == actual_idx) break;
        }
        update_selected_type_panel(win,
            (actual_idx >= 0 && j < win->visible_count) ? actual_idx : -1);
    }
}

/*========================================================================*/
/* Chooser label helpers                                                 */
/*========================================================================*/

static struct List *create_chooser_labels(STRPTR *strings)
{
    struct List *list;
    struct Node *node;

    list = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
    if (!list)
        return NULL;

    NewList(list);
    while (*strings)
    {
        node = AllocChooserNode(CNA_Text, *strings, TAG_DONE);
        if (node)
            AddTail(list, node);
        strings++;
    }
    return list;
}

static void free_chooser_labels(struct List *list)
{
    struct Node *node;
    struct Node *next;

    if (!list)
        return;

    node = list->lh_Head;
    while ((next = node->ln_Succ))
    {
        FreeChooserNode(node);
        node = next;
    }
    FreeMem(list, sizeof(struct List));
}

/*========================================================================*/
/* Window creation                                                       */
/*========================================================================*/

static BOOL create_window(TextTemplatesWindow *win)
{
    UBYTE *chooser_strs[] = { "All", "Custom only", "Missing only", NULL };

    init_column_info(win->col_info);

    /* Allocate the ListBrowser node list */
    win->lb_list = (struct List *)whd_malloc(sizeof(struct List));
    if (!win->lb_list)
    {
        log_error(LOG_GUI, "TextTemplates: failed to allocate ListBrowser list\n");
        return FALSE;
    }
    NewList(win->lb_list);

    /* Allocate chooser labels */
    win->chooser_list = create_chooser_labels(chooser_strs);
    if (!win->chooser_list)
    {
        log_error(LOG_GUI, "TextTemplates: failed to allocate chooser labels\n");
        whd_free(win->lb_list);
        win->lb_list = NULL;
        return FALSE;
    }

    /* ------------------------------------------------------------------ */
    /* Create gadget objects (pre-allocated so we can store pointers)     */
    /* ------------------------------------------------------------------ */

    /* "Show" filter chooser */
    win->show_chooser = NewObject(CHOOSER_GetClass(), NULL,
        GA_ID,           GID_SHOW_CHOOSER,
        GA_RelVerify,    TRUE,
        GA_TabCycle,     TRUE,
        CHOOSER_PopUp,   TRUE,
        CHOOSER_Selected,0,
        CHOOSER_Labels,  win->chooser_list,
    TAG_DONE);

    /* ListBrowser */
    win->template_lb = (Object *)ListBrowserObject,
        GA_ID,                     GID_TEMPLATE_LB,
        GA_RelVerify,              TRUE,
        GA_TabCycle,               TRUE,
        LISTBROWSER_ColumnInfo,    win->col_info,
        LISTBROWSER_ColumnTitles,  TRUE,
        LISTBROWSER_Labels,        win->lb_list,
        LISTBROWSER_Separators,    FALSE,
        LISTBROWSER_Striping,      LBS_ROWS,
        LISTBROWSER_Selected,       -1,
        LISTBROWSER_MultiSelect,    FALSE,
        LISTBROWSER_ShowSelected,   TRUE,
        LISTBROWSER_HorizontalProp, TRUE,
        LISTBROWSER_AutoFit,        TRUE,
    ListBrowserEnd;

    /* Read-only "Selected Type" display fields (transparent buttons) */
    win->selected_type = (Object *)ButtonObject,
        GA_ID,                GID_SELECTED_TYPE,
        GA_Text,              "(none)",
        GA_Underscore,        0,          /* Prevent _ from being eaten as shortcut */
        GA_TabCycle,          TRUE,
        BUTTON_Transparent,   TRUE,
        BUTTON_TextPen,       1,
        BUTTON_BackgroundPen, 0,
        BUTTON_FillTextPen,   1,
        BUTTON_FillPen,       3,
        BUTTON_BevelStyle,    BVS_NONE,
        BUTTON_Justification, BCJ_LEFT,
    ButtonEnd;

    win->selected_tmplfile = (Object *)ButtonObject,
        GA_ID,                GID_SELECTED_TMPLFILE,
        GA_Text,              "-",
        GA_Underscore,        0,          /* Prevent _ from being eaten as shortcut */
        GA_TabCycle,          TRUE,
        BUTTON_Transparent,   TRUE,
        BUTTON_TextPen,       1,
        BUTTON_BackgroundPen, 0,
        BUTTON_FillTextPen,   1,
        BUTTON_FillPen,       3,
        BUTTON_BevelStyle,    BVS_NONE,
        BUTTON_Justification, BCJ_LEFT,
    ButtonEnd;

    win->selected_effective = (Object *)ButtonObject,
        GA_ID,                GID_SELECTED_EFFECTIVE,
        GA_Text,              "-",
        GA_Underscore,        0,          /* Prevent _ from being eaten as shortcut */
        GA_TabCycle,          TRUE,
        BUTTON_Transparent,   TRUE,
        BUTTON_TextPen,       1,
        BUTTON_BackgroundPen, 0,
        BUTTON_FillTextPen,   1,
        BUTTON_FillPen,       3,
        BUTTON_BevelStyle,    BVS_NONE,
        BUTTON_Justification, BCJ_LEFT,
    ButtonEnd;

    win->selected_status = (Object *)ButtonObject,
        GA_ID,                GID_SELECTED_STATUS,
        GA_Text,              "-",
        GA_Underscore,        0,          /* Prevent _ from being eaten as shortcut */
        GA_TabCycle,          TRUE,
        BUTTON_Transparent,   TRUE,
        BUTTON_TextPen,       1,
        BUTTON_BackgroundPen, 0,
        BUTTON_FillTextPen,   1,
        BUTTON_FillPen,       3,
        BUTTON_BevelStyle,    BVS_NONE,
        BUTTON_Justification, BCJ_LEFT,
    ButtonEnd;

    /* Template action buttons (disabled until a row is selected) */
    win->btn_create_overwrite = (Object *)ButtonObject,
        GA_ID,                GID_BTN_CREATE_OVERWRITE,
        GA_Text,              "Create from master",
        GA_RelVerify,         TRUE,
        GA_TabCycle,          TRUE,
        BUTTON_TextPen,       1,
        BUTTON_BackgroundPen, 0,
        BUTTON_FillTextPen,   1,
        BUTTON_FillPen,       3,
        GA_Disabled,          TRUE,
    ButtonEnd;

    win->btn_edit_tooltypes = (Object *)ButtonObject,
        GA_ID,                GID_BTN_EDIT_TOOLTYPES,
        GA_Text,              "Edit tooltypes...",
        GA_RelVerify,         TRUE,
        GA_TabCycle,          TRUE,
        BUTTON_TextPen,       1,
        BUTTON_BackgroundPen, 0,
        BUTTON_FillTextPen,   1,
        BUTTON_FillPen,       3,
        GA_Disabled,          TRUE,
    ButtonEnd;

    win->btn_validate = (Object *)ButtonObject,
        GA_ID,                GID_BTN_VALIDATE,
        GA_Text,              "Validate tooltypes",
        GA_RelVerify,         TRUE,
        GA_TabCycle,          TRUE,
        BUTTON_TextPen,       1,
        BUTTON_BackgroundPen, 0,
        BUTTON_FillTextPen,   1,
        BUTTON_FillPen,       3,
        GA_Disabled,          TRUE,
    ButtonEnd;

    win->btn_revert = (Object *)ButtonObject,
        GA_ID,                GID_BTN_REVERT,
        GA_Text,              "Revert to master",
        GA_RelVerify,         TRUE,
        GA_TabCycle,          TRUE,
        BUTTON_TextPen,       1,
        BUTTON_BackgroundPen, 0,
        BUTTON_FillTextPen,   1,
        BUTTON_FillPen,       3,
        GA_Disabled,          TRUE,
    ButtonEnd;

    /* Close button */
    win->btn_close = (Object *)ButtonObject,
        GA_ID,                GID_BTN_CLOSE,
        GA_Text,              "_Close",
        GA_RelVerify,         TRUE,
        GA_TabCycle,          TRUE,
        BUTTON_TextPen,       1,
        BUTTON_BackgroundPen, 0,
        BUTTON_FillTextPen,   1,
        BUTTON_FillPen,       3,
    ButtonEnd;

    /* ------------------------------------------------------------------ */
    /* Assemble window layout                                             */
    /* ------------------------------------------------------------------ */
    win->window_obj = (Object *)WindowObject,
        WA_Title,         "DefIcons: Text Templates",
        WA_Activate,      TRUE,
        WA_DepthGadget,   TRUE,
        WA_DragBar,       TRUE,
        WA_CloseGadget,   TRUE,
        WA_SizeGadget,    TRUE,
        WA_Width,         480,
        WA_Height,        200,
        WA_MinWidth,      420,
        WA_MinHeight,     300,
        WINDOW_Position,  WPOS_CENTERSCREEN,
        WINDOW_Layout, (Object *)VLayoutObject,
            LAYOUT_SpaceOuter,  TRUE,
            LAYOUT_DeferLayout, TRUE,

            /* ---- Top row: list (left) | info + actions (right) ---- */
            LAYOUT_AddChild, (Object *)HLayoutObject,

                /* Left column: Show chooser + listbrowser */
                LAYOUT_AddChild, (Object *)VLayoutObject,
                    LAYOUT_BevelStyle,    BVS_FIELD,
                    LAYOUT_LeftSpacing,   2,
                    LAYOUT_RightSpacing,  2,
                    LAYOUT_TopSpacing,    2,
                    LAYOUT_BottomSpacing, 4,
                    LAYOUT_Label,         "Icons",

                    LAYOUT_AddChild, win->show_chooser,
                    CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                        LABEL_Text, "Show",
                    TAG_DONE),

                    LAYOUT_AddChild, win->template_lb,
                LayoutEnd,
                CHILD_WeightedWidth, 65,

                /* Right column: three sub-groups */
                LAYOUT_AddChild, (Object *)VLayoutObject,
                    LAYOUT_LeftSpacing, 4,

                    /* "Selected Type" info group */
                    LAYOUT_AddChild, (Object *)VLayoutObject,
                        LAYOUT_BevelStyle,    BVS_FIELD,
                        LAYOUT_LeftSpacing,   2,
                        LAYOUT_RightSpacing,  2,
                        LAYOUT_TopSpacing,    2,
                        LAYOUT_BottomSpacing, 2,
                        LAYOUT_Label,         "Selected Type",

                        LAYOUT_AddChild, win->selected_type,
                        CHILD_WeightedHeight, 0,
                        CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, "Type:",
                        TAG_DONE),

                        LAYOUT_AddChild, win->selected_tmplfile,
                        CHILD_WeightedHeight, 0,
                        CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, "Template file:",
                        TAG_DONE),

                        LAYOUT_AddChild, win->selected_effective,
                        CHILD_WeightedHeight, 0,
                        CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, "Effective:",
                        TAG_DONE),

                        LAYOUT_AddChild, win->selected_status,
                        CHILD_WeightedHeight, 0,
                        CHILD_Label, NewObject(LABEL_GetClass(), NULL,
                            LABEL_Text, "Status:",
                        TAG_DONE),
                    LayoutEnd,

                    /* Fixed 5px spacer between info and actions */
                    LAYOUT_AddChild, (Object *)VLayoutObject,
                    LayoutEnd,
                    CHILD_MinHeight, 5,
                    CHILD_MaxHeight, 5,

                    /* "Template action" group */
                    LAYOUT_AddChild, (Object *)VLayoutObject,
                        LAYOUT_BevelStyle,    BVS_FIELD,
                        LAYOUT_LeftSpacing,   2,
                        LAYOUT_RightSpacing,  2,
                        LAYOUT_TopSpacing,    2,
                        LAYOUT_BottomSpacing, 4,
                        LAYOUT_Label,         "Template action",

                        LAYOUT_AddChild, win->btn_create_overwrite,
                        CHILD_WeightedHeight, 0,
                        CHILD_WeightedWidth,  100,

                        LAYOUT_AddChild, win->btn_edit_tooltypes,
                        CHILD_WeightedHeight, 0,
                        CHILD_WeightedWidth,  100,

                        LAYOUT_AddChild, win->btn_validate,
                        CHILD_WeightedHeight, 0,
                        CHILD_WeightedWidth,  100,

                        LAYOUT_AddChild, win->btn_revert,
                        CHILD_WeightedHeight, 0,
                        CHILD_WeightedWidth,  100,
                    LayoutEnd,

                LayoutEnd,
                CHILD_WeightedWidth, 35,

            LayoutEnd,  /* HLayout top row */
            CHILD_WeightedHeight, 95,

            /* ---- Bottom bar: spacer (left) + Close (right) ---- */
            LAYOUT_AddChild, (Object *)HLayoutObject,
                LAYOUT_TopSpacing, 10,

                /* Empty spacer, takes up the left half */
                LAYOUT_AddChild, (Object *)VLayoutObject,
                LayoutEnd,
                CHILD_WeightedWidth, 65,

                /* Close button aligned to the right half */
                LAYOUT_AddChild, (Object *)HLayoutObject,
                    LAYOUT_LeftSpacing, 4,
                    LAYOUT_AddChild, win->btn_close,
                LayoutEnd,
                CHILD_WeightedWidth, 35,

            LayoutEnd,  /* HLayout bottom bar */
            CHILD_WeightedHeight, 5,

        LayoutEnd,  /* VLayout root */
    WindowEnd;

    if (!win->window_obj)
    {
        log_error(LOG_GUI, "TextTemplates: failed to create window object\n");
        free_chooser_labels(win->chooser_list);
        win->chooser_list = NULL;
        return FALSE;
    }

    win->window = (struct Window *)RA_OpenWindow(win->window_obj);
    if (!win->window)
    {
        log_error(LOG_GUI, "TextTemplates: failed to open window\n");
        DisposeObject(win->window_obj);
        win->window_obj = NULL;
        free_chooser_labels(win->chooser_list);
        win->chooser_list = NULL;
        return FALSE;
    }

    log_info(LOG_GUI, "TextTemplates: window opened\n");
    return TRUE;
}


/*========================================================================*/
/* Event loop                                                            */
/*========================================================================*/

static void event_loop(TextTemplatesWindow *win)
{
    ULONG signal_mask, signals, result;
    ULONG selected;
    UWORD code;
    BOOL done = FALSE;

    GetAttr(WINDOW_SigMask, win->window_obj, &signal_mask);

    while (!done)
    {
        signals = Wait(signal_mask | SIGBREAKF_CTRL_C);

        if (signals & SIGBREAKF_CTRL_C)
        {
            done = TRUE;
            continue;
        }

        while ((result = RA_HandleInput(win->window_obj, &code)) != WMHI_LASTMSG)
        {
            switch (result & WMHI_CLASSMASK)
            {
                case WMHI_CLOSEWINDOW:
                    done = TRUE;
                    break;

                case WMHI_GADGETUP:
                {
                    ULONG gid = result & WMHI_GADGETMASK;
                    switch (gid)
                    {
                        case GID_BTN_CLOSE:
                            done = TRUE;
                            break;

                        case GID_TEMPLATE_LB:
                            /* Selection changed - translate visual row to subtypes[] index */
                            selected = 0;
                            GetAttr(LISTBROWSER_Selected, win->template_lb, &selected);
                            update_selected_type_panel(win, visual_to_actual(win, selected));
                            break;

                        case GID_SHOW_CHOOSER:
                        {
                            /* Filter changed - rebuild list, reset panel to no-selection */
                            ULONG filter_sel = 0;
                            GetAttr(CHOOSER_Selected, win->show_chooser, &filter_sel);
                            win->show_filter = (ShowFilter)filter_sel;
                            rebuild_list(win);
                            update_selected_type_panel(win, -1);
                            break;
                        }

                        case GID_BTN_CREATE_OVERWRITE:
                            handle_create_overwrite(win);
                            /* rebuild_list was called inside; subtype_map is now fresh */
                            selected = 0;
                            GetAttr(LISTBROWSER_Selected, win->template_lb, &selected);
                            update_selected_type_panel(win, visual_to_actual(win, selected));
                            break;

                        case GID_BTN_EDIT_TOOLTYPES:
                            handle_wbinfo(win);
                            break;

                        case GID_BTN_VALIDATE:
                            handle_check_tooltypes(win);
                            break;

                        case GID_BTN_REVERT:
                            handle_revert(win);
                            break;

                        default:
                            break;
                    }
                    break;
                }
            }
        }
    }
}

/*========================================================================*/
/* Cleanup                                                               */
/*========================================================================*/

static void cleanup_window(TextTemplatesWindow *win)
{
    struct Node *node;

    if (win->window_obj)
    {
        DisposeObject(win->window_obj);
        win->window_obj = NULL;
    }

    /* Free chooser labels */
    if (win->chooser_list)
    {
        free_chooser_labels(win->chooser_list);
        win->chooser_list = NULL;
    }

    /* Free ListBrowser nodes and list */
    if (win->lb_list)
    {
        while ((node = RemHead(win->lb_list)))
            FreeListBrowserNode(node);
        whd_free(win->lb_list);
        win->lb_list = NULL;
    }

    log_info(LOG_GUI, "TextTemplates: window closed\n");
}

/*========================================================================*/
/* Public API                                                            */
/*========================================================================*/

void open_text_templates_window(LayoutPreferences *prefs)
{
    TextTemplatesWindow win;

    if (!prefs)
    {
        log_error(LOG_GUI, "TextTemplates: NULL prefs\n");
        return;
    }

    memset(&win, 0, sizeof(TextTemplatesWindow));
    win.prefs       = prefs;
    win.show_filter = SHOW_ALL;

    /* Fetch subtype list (g_cached_deficons_tree must be initialised) */
    win.subtypes = deficons_get_ascii_subtypes(&win.subtype_count);
    if (win.subtype_count == 0)
    {
        log_warning(LOG_GUI,
            "TextTemplates: no ASCII subtypes found - "
            "deficons tree may not be initialised\n");
    }

    if (!open_libs())
        return;

    if (!create_window(&win))
    {
        close_libs();
        return;
    }

    /* Set busy pointer while loading exclusion data and populating the list.
     * Icon reading and filesystem checks can take a moment on slow Amigas. */
    SetWindowPointer(win.window, WA_BusyPointer, TRUE, TAG_DONE);

    /* Load EXCLUDETYPE tooltype from PROGDIR:Icons/def_ascii.info */
    load_exclude_types(&win);

    /* Populate list and set clean no-selection state */
    rebuild_list(&win);
    update_selected_type_panel(&win, -1);

    /* Dismiss busy pointer now the list is fully populated */
    SetWindowPointer(win.window, WA_BusyPointer, FALSE, TAG_DONE);

    event_loop(&win);

    cleanup_window(&win);
    close_libs();
}
