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
#define CheckBoxBase  iTidy_TextTemplates_CheckBoxBase
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
#include <proto/checkbox.h>
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
#include <gadgets/checkbox.h>
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
struct Library *iTidy_TextTemplates_CheckBoxBase  = NULL;
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

/* Gadget IDs */
enum {
    GID_TEMPLATE_LB = 1,    /* ListBrowser */
    GID_ENABLE_CB,           /* Enable text previews checkbox */
    GID_COPY_BTN,            /* Copy def_ascii.info -> def_<type>.info */
    GID_CHECK_BTN,           /* Validate ToolTypes */
    GID_WBINFO_BTN,          /* Open WB Information dialog */
    GID_REFRESH_BTN,         /* Refresh list */
    GID_OK,
    GID_CANCEL
};

/* Column indices */
#define COL_TYPE    0
#define COL_FILE    1
#define COL_STATUS  2

/*========================================================================*/
/* Window state structure                                                */
/*========================================================================*/

typedef struct {
    /* ReAction objects */
    Object *window_obj;
    Object *template_lb;
    Object *enable_cb;
    Object *copy_btn;
    Object *check_btn;
    Object *wbinfo_btn;
    Object *refresh_btn;
    Object *ok_btn;
    Object *cancel_btn;

    struct Window *window;
    struct List   *lb_list;     /* ListBrowser nodes */

    LayoutPreferences *prefs;

    BOOL user_accepted;

    /* Cached subtype table */
    const char **subtypes;      /* from deficons_get_ascii_subtypes() */
    int          subtype_count;
} TextTemplatesWindow;

/*========================================================================*/
/* Library open / close                                                  */
/*========================================================================*/

static BOOL open_libs(void)
{
    iTidy_TextTemplates_WindowBase     = OpenLibrary("window.class",            44);
    iTidy_TextTemplates_LayoutBase     = OpenLibrary("gadgets/layout.gadget",   44);
    iTidy_TextTemplates_ListBrowserBase= OpenLibrary("gadgets/listbrowser.gadget", 44);
    iTidy_TextTemplates_ButtonBase     = OpenLibrary("gadgets/button.gadget",   44);
    iTidy_TextTemplates_CheckBoxBase   = OpenLibrary("gadgets/checkbox.gadget", 44);
    iTidy_TextTemplates_LabelBase      = OpenLibrary("images/label.image",      44);
    iTidy_TextTemplates_RequesterBase  = OpenLibrary("requester.class",         44);

    if (!iTidy_TextTemplates_WindowBase || !iTidy_TextTemplates_LayoutBase ||
        !iTidy_TextTemplates_ListBrowserBase || !iTidy_TextTemplates_ButtonBase ||
        !iTidy_TextTemplates_CheckBoxBase || !iTidy_TextTemplates_LabelBase ||
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
    if (iTidy_TextTemplates_CheckBoxBase)
    {
        CloseLibrary(iTidy_TextTemplates_CheckBoxBase);
        iTidy_TextTemplates_CheckBoxBase = NULL;
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
/* Column info (static - 3 columns)                                      */
/*========================================================================*/

static struct ColumnInfo *make_column_info(void)
{
    static struct ColumnInfo ci[] = {
        { COL_TYPE_PCT,   "Type",     0 },
        { COL_FILE_PCT,   "Template", 0 },
        { COL_STATUS_PCT, "Status",   0 },
        { -1, NULL, 0 }
    };
    return ci;
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
 */
static struct Node *make_type_node(const char *type_token, BOOL is_parent)
{
    char display_type[64];
    char file_label[64];
    const char *status;

    if (is_parent)
        snprintf(display_type, sizeof(display_type), "%s", type_token);
    else
        snprintf(display_type, sizeof(display_type), "  %s", type_token);

    make_file_label(type_token, file_label, sizeof(file_label));
    status = type_has_template(type_token) ? "Custom" : "None";

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

    /* Add one node per subtype; index 0 is "ascii" itself (parent) */
    for (i = 0; i < win->subtype_count; i++)
    {
        node = make_type_node(win->subtypes[i], i == 0);
        if (node)
            AddTail(win->lb_list, node);
    }

    /* Reattach */
    SetGadgetAttrs((struct Gadget *)win->template_lb, win->window, NULL,
        LISTBROWSER_Labels, win->lb_list,
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
    if ((LONG)selected < 0 || (int)selected >= win->subtype_count)
        return NULL;

    return win->subtypes[(int)selected];
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

/*========================================================================*/
/* Action: Copy                                                          */
/*========================================================================*/

/**
 * Copy PROGDIR:Icons/def_ascii.info to PROGDIR:Icons/def_<type>.info.
 * Prompts the user if the destination already exists.
 */
static void handle_copy(TextTemplatesWindow *win)
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

    if (strcmp(type_token, "ascii") == 0)
    {
        show_info_req(win->window, "Copy Template",
                      "The 'ascii' type is the master template itself.\n"
                      "Select a sub-type (e.g., c, rexx) to copy it to.",
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

    /* Build path (without .info suffix - GetDiskObject adds it) */
    snprintf(icon_path, sizeof(icon_path), "%sdef_%s", TEMPLATE_DIR, type_token);

    dobj = GetDiskObject((STRPTR)icon_path);
    if (!dobj)
    {
        snprintf(report, sizeof(report),
                 "No icon found for def_%s.info\n\n"
                 "Use 'Copy from ascii' to create one first.", type_token);
        show_info_req(win->window, "Check ToolTypes", report, REQIMAGE_WARNING);
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

    /* Build the full icon path and check it exists */
    snprintf(full_path, sizeof(full_path), "%sdef_%s.info", TEMPLATE_DIR, type_token);
    lock = Lock((STRPTR)full_path, ACCESS_READ);
    if (!lock)
    {
        char body[512];
        snprintf(body, sizeof(body),
                 "def_%s.info does not exist.\n\n"
                 "Use 'Copy from ascii' to create it first.", type_token);
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
/* Window creation                                                       */
/*========================================================================*/

static BOOL create_window(TextTemplatesWindow *win)
{
    struct ColumnInfo *ci = make_column_info();

    /* Allocate an empty list for the ListBrowser */
    win->lb_list = (struct List *)whd_malloc(sizeof(struct List));
    if (!win->lb_list)
    {
        log_error(LOG_GUI, "TextTemplates: failed to allocate ListBrowser list\n");
        return FALSE;
    }
    NewList(win->lb_list);

    /* --- Create gadget objects --- */

    win->template_lb = (Object *)ListBrowserObject,
        GA_ID,                     GID_TEMPLATE_LB,
        GA_RelVerify,              TRUE,
        LISTBROWSER_ColumnInfo,    ci,
        LISTBROWSER_ColumnTitles,  TRUE,
        LISTBROWSER_Labels,        win->lb_list,
        LISTBROWSER_Separators,    FALSE,
        LISTBROWSER_Striping,      LBS_ROWS,
        LISTBROWSER_Selected,      -1,
        LISTBROWSER_MultiSelect,   FALSE,
        LISTBROWSER_ShowSelected,  TRUE,
    ListBrowserEnd;

    win->enable_cb = (Object *)CheckBoxObject,
        GA_ID,       GID_ENABLE_CB,
        GA_Text,     "Enable _text file preview thumbnails",
        GA_Selected, win->prefs->deficons_enable_text_previews,
        GA_RelVerify,TRUE,
    CheckBoxEnd;

    win->copy_btn = (Object *)ButtonObject,
        GA_ID,        GID_COPY_BTN,
        GA_Text,      "_Copy from ascii template",
        GA_RelVerify, TRUE,
    ButtonEnd;

    win->check_btn = (Object *)ButtonObject,
        GA_ID,        GID_CHECK_BTN,
        GA_Text,      "_Validate ToolTypes",
        GA_RelVerify, TRUE,
    ButtonEnd;

    win->wbinfo_btn = (Object *)ButtonObject,
        GA_ID,        GID_WBINFO_BTN,
        GA_Text,      "WB _Information...",
        GA_RelVerify, TRUE,
    ButtonEnd;

    win->refresh_btn = (Object *)ButtonObject,
        GA_ID,        GID_REFRESH_BTN,
        GA_Text,      "_Refresh List",
        GA_RelVerify, TRUE,
    ButtonEnd;

    win->ok_btn = (Object *)ButtonObject,
        GA_ID,        GID_OK,
        GA_Text,      "_OK",
        GA_RelVerify, TRUE,
    ButtonEnd;

    win->cancel_btn = (Object *)ButtonObject,
        GA_ID,        GID_CANCEL,
        GA_Text,      "_Cancel",
        GA_RelVerify, TRUE,
    ButtonEnd;

    /* --- Layout --- */
    win->window_obj = (Object *)WindowObject,
        WA_Title,         "DefIcons: Manage ASCII Text Templates",
        WA_Activate,      TRUE,
        WA_DepthGadget,   TRUE,
        WA_DragBar,       TRUE,
        WA_CloseGadget,   TRUE,
        WA_SizeGadget,    TRUE,
        WA_Width,         520,
        WA_Height,        380,
        WINDOW_Position,  WPOS_CENTERSCREEN,
        WINDOW_Layout, (Object *)VLayoutObject,

            /* Top group: list + action buttons side-by-side */
            LAYOUT_AddChild, (Object *)HLayoutObject,

                /* Left: list + enable checkbox */
                LAYOUT_AddChild, (Object *)VLayoutObject,

                    LAYOUT_AddChild, (Object *)VLayoutObject,
                        LAYOUT_BevelStyle, BVS_GROUP,
                        LAYOUT_Label,      "ASCII Type Templates",
                        LAYOUT_AddChild, win->template_lb,
                    LayoutEnd,

                    LAYOUT_AddChild, win->enable_cb,
                    CHILD_WeightedHeight, 0,
                LayoutEnd,

                /* Right: action buttons */
                LAYOUT_AddChild, (Object *)VLayoutObject,
                    CHILD_WeightedWidth, 0,

                    LAYOUT_BevelStyle, BVS_GROUP,
                    LAYOUT_Label,      "Actions",

                    LAYOUT_AddChild, win->copy_btn,
                    CHILD_WeightedHeight, 0,
                    CHILD_WeightedWidth,  100,

                    LAYOUT_AddChild, win->check_btn,
                    CHILD_WeightedHeight, 0,
                    CHILD_WeightedWidth,  100,

                    LAYOUT_AddChild, win->wbinfo_btn,
                    CHILD_WeightedHeight, 0,
                    CHILD_WeightedWidth,  100,

                    LAYOUT_AddChild, win->refresh_btn,
                    CHILD_WeightedHeight, 0,
                    CHILD_WeightedWidth,  100,
                LayoutEnd,

            LayoutEnd,  /* HLayout */

            /* Bottom: OK / Cancel */
            LAYOUT_AddChild, (Object *)HLayoutObject,
                CHILD_WeightedHeight, 0,
                LAYOUT_AddChild, win->ok_btn,
                LAYOUT_AddChild, win->cancel_btn,
            LayoutEnd,
            CHILD_WeightedHeight, 0,

        LayoutEnd,  /* VLayout */
    WindowEnd;

    if (!win->window_obj)
    {
        log_error(LOG_GUI, "TextTemplates: failed to create window object\n");
        return FALSE;
    }

    win->window = (struct Window *)RA_OpenWindow(win->window_obj);
    if (!win->window)
    {
        log_error(LOG_GUI, "TextTemplates: failed to open window\n");
        DisposeObject(win->window_obj);
        win->window_obj = NULL;
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
                        case GID_OK:
                        {
                            ULONG sel = 0;
                            GetAttr(GA_Selected, win->enable_cb, &sel);
                            win->prefs->deficons_enable_text_previews = (BOOL)sel;
                            win->user_accepted = TRUE;
                            done = TRUE;
                            break;
                        }

                        case GID_CANCEL:
                            done = TRUE;
                            break;

                        case GID_COPY_BTN:
                            handle_copy(win);
                            break;

                        case GID_CHECK_BTN:
                            handle_check_tooltypes(win);
                            break;

                        case GID_WBINFO_BTN:
                            handle_wbinfo(win);
                            break;

                        case GID_REFRESH_BTN:
                            rebuild_list(win);
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

BOOL open_text_templates_window(LayoutPreferences *prefs)
{
    TextTemplatesWindow win;

    if (!prefs)
    {
        log_error(LOG_GUI, "TextTemplates: NULL prefs\n");
        return FALSE;
    }

    memset(&win, 0, sizeof(TextTemplatesWindow));
    win.prefs         = prefs;
    win.user_accepted = FALSE;

    /* Fetch subtype list (g_cached_deficons_tree must be initialised) */
    win.subtypes = deficons_get_ascii_subtypes(&win.subtype_count);
    if (win.subtype_count == 0)
    {
        log_warning(LOG_GUI,
            "TextTemplates: no ASCII subtypes found - "
            "deficons tree may not be initialised\n");
    }

    if (!open_libs())
        return FALSE;

    if (!create_window(&win))
    {
        close_libs();
        return FALSE;
    }

    /* Populate list after window is open so SetGadgetAttrs works */
    rebuild_list(&win);

    event_loop(&win);

    {
        BOOL result = win.user_accepted;
        cleanup_window(&win);
        close_libs();
        return result;
    }
}
