/*
 * deficons_creation_window.c - DefIcons Icon Creation Settings Window (ReAction)
 * 
 * GUI window for configuring DefIcons icon creation settings such as:
 * - Folder icon mode (Smart/Always/Never)
 * - Icon size (Small/Medium/Large)
 * - Thumbnail options (borders, text preview, picture preview)
 * - Color depth and dithering settings
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

/* Library base isolation - prevent linker conflicts with other windows */
#define WindowBase iTidy_DefIconsCreation_WindowBase
#define LayoutBase iTidy_DefIconsCreation_LayoutBase
#define ChooserBase iTidy_DefIconsCreation_ChooserBase
#define ButtonBase iTidy_DefIconsCreation_ButtonBase
#define LabelBase iTidy_DefIconsCreation_LabelBase
#define CheckBoxBase iTidy_DefIconsCreation_CheckBoxBase
#define ClickTabBase iTidy_DefIconsCreation_ClickTabBase
#define SpaceBase iTidy_DefIconsCreation_SpaceBase

#include "deficons_creation_window.h"
#include "text_templates_window.h"
#include "deficons_settings_window.h"
#include "../exclude_paths_window.h"
#include "../../layout_preferences.h"
#include "../../writeLog.h"
#include "../../icon_edit/palette/palette_reduction.h"
#include "../../platform/platform.h"

#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/chooser.h>
#include <proto/button.h>
#include <proto/checkbox.h>
#include <proto/label.h>
#include <proto/utility.h>
#include <proto/clicktab.h>
#include <proto/space.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/chooser.h>
#include <gadgets/button.h>
#include <gadgets/checkbox.h>
#include <gadgets/clicktab.h>
#include <gadgets/space.h>
#include <images/label.h>

#include <string.h>
#include <stdio.h>

/* Local library bases */
struct Library *iTidy_DefIconsCreation_WindowBase = NULL;
struct Library *iTidy_DefIconsCreation_LayoutBase = NULL;
struct Library *iTidy_DefIconsCreation_ChooserBase = NULL;
struct Library *iTidy_DefIconsCreation_ButtonBase = NULL;
struct Library *iTidy_DefIconsCreation_LabelBase = NULL;
struct Library *iTidy_DefIconsCreation_CheckBoxBase = NULL;
struct Library *iTidy_DefIconsCreation_ClickTabBase = NULL;
struct Library *iTidy_DefIconsCreation_SpaceBase = NULL;

/* Gadget IDs */
enum {
    GID_CLICKTAB = 1,
    GID_FOLDER_MODE_CHOOSER,
    GID_SKIP_WHDLOAD_CB,
    GID_ICON_SIZE_CHOOSER,
    GID_THUMBNAIL_BORDERS_CHOOSER,
    GID_TEXT_PREVIEW_CHECKBOX,
    GID_MANAGE_TEXT_TEMPLATES,
    GID_PICTURE_PREVIEW_CHECKBOX,
    GID_UPSCALE_THUMBNAILS_CB,
    /* Per-format enable checkboxes */
    GID_FMT_ILBM_CB,
    GID_FMT_PNG_CB,
    GID_FMT_GIF_CB,
    GID_FMT_JPEG_CB,
    GID_FMT_BMP_CB,
    GID_FMT_ACBM_CB,
    GID_FMT_OTHER_CB,
    GID_MAX_COLORS_CHOOSER,
    GID_DITHER_METHOD_CHOOSER,
    GID_LOWCOLOR_MAPPING_CHOOSER,
    GID_REPLACE_THUMBNAILS_CB,
    GID_REPLACE_TEXT_PREVIEWS_CB,
    GID_DEFICONS_CREATION_SETUP,
    GID_DEFICONS_EXCLUDE_PATHS,
    GID_OK,
    GID_CANCEL
};

/*
 * Internal window structure
 */
typedef struct {
    /* ReAction objects */
    Object *window_obj;
    Object *main_layout;
    Object *clicktab_obj;
    struct List *tab_labels;
    Object *folder_mode_chooser_obj;
    Object *whdload_skip_cb;
    Object *replace_thumbnails_cb;
    Object *replace_text_previews_cb;
    Object *icon_size_chooser_obj;
    Object *thumbnail_borders_chooser_obj;
    Object *text_preview_checkbox;
    Object *manage_text_templates_btn;
    Object *picture_preview_checkbox;
    Object *upscale_thumbnails_cb;
    /* Per-format enable checkboxes */
    Object *fmt_ilbm_cb;
    Object *fmt_png_cb;
    Object *fmt_gif_cb;
    Object *fmt_jpeg_cb;
    Object *fmt_bmp_cb;
    Object *fmt_acbm_cb;
    Object *fmt_other_cb;
    Object *max_colors_chooser_obj;
    Object *dither_method_chooser_obj;
    Object *lowcolor_mapping_chooser_obj;
    Object *deficons_creation_setup_btn;
    Object *deficons_exclude_paths_btn;
    Object *ok_btn;
    Object *cancel_btn;
    
    struct Window *window;
    
    /* Chooser lists (must be allocated with AllocChooserNode) */
    struct List *folder_mode_list;
    struct List *icon_size_list;
    struct List *thumbnail_borders_list;
    struct List *max_colors_list;
    struct List *dither_method_list;
    struct List *lowcolor_mapping_list;
    
    /* Working copy of preferences */
    LayoutPreferences *prefs;
    
    /* State */
    BOOL user_accepted;
    
} DefIconsCreationWindow;

/* Folder icon mode chooser labels (matches ITIDY_FOLDER_ICON_MODE_* constants) */
static STRPTR folder_mode_labels[] = {
    "Smart (When Folder Has Icons)",
    "Always Create",
    "Never Create",
    NULL
};

/* ClickTab labels */
static STRPTR tab_label_strings[] = {
    "Create",
    "Rendering",
    "DefIcons",
    NULL
};

/* Icon size chooser labels (matches ITIDY_ICON_SIZE_* constants) */
static STRPTR icon_size_labels[] = {
    "Small (48x48)",
    "Medium (64x64)",
    "Large (100x100)",
    NULL
};

/* Thumbnail border/bevel style chooser labels (matches ITIDY_THUMB_BORDER_* constants 0-4) */
static STRPTR thumbnail_border_labels[] = {
    "None",
    "Workbench (Smart)",
    "Workbench (Always)",
    "Bevel (Smart)",
    "Bevel (Always)",
    NULL
};

/* Max icon colours chooser labels (indices 0-8) */
static STRPTR max_colors_labels[] = {
    "4 colours",
    "8 colours",
    "16 colours",
    "GlowIcons palette (29 colours)",    /* ITIDY_MAX_COLORS_HARMONISED_INDEX */
    "32 colours",
    "64 colours",
    "128 colours",
    "256 colours (full)",
    "Ultra (256 + detail boost)",         /* ITIDY_MAX_COLORS_ULTRA_INDEX */
    NULL
};

/* Dithering method chooser labels */
static STRPTR dither_method_labels[] = {
    "None",
    "Ordered (Bayer 4x4)",
    "Error Diffusion (Floyd-Steinberg)",
    "Auto (Based On Colour Count)",
    NULL
};

/* Low-color mapping chooser labels (used when max colors <= 8) */
static STRPTR lowcolor_mapping_labels[] = {
    "Greyscale",
    "Workbench Palette",
    "Hybrid (Grays + WB Accents)",
    NULL
};

/*
 * Open ReAction libraries
 */
static BOOL open_reaction_libs(void)
{
    iTidy_DefIconsCreation_WindowBase = OpenLibrary("window.class", 44);
    iTidy_DefIconsCreation_LayoutBase = OpenLibrary("gadgets/layout.gadget", 44);
    iTidy_DefIconsCreation_ChooserBase = OpenLibrary("gadgets/chooser.gadget", 44);
    iTidy_DefIconsCreation_ButtonBase = OpenLibrary("gadgets/button.gadget", 44);
    iTidy_DefIconsCreation_CheckBoxBase = OpenLibrary("gadgets/checkbox.gadget", 44);
    iTidy_DefIconsCreation_LabelBase = OpenLibrary("images/label.image", 44);
    iTidy_DefIconsCreation_ClickTabBase = OpenLibrary("gadgets/clicktab.gadget", 44);
    iTidy_DefIconsCreation_SpaceBase = OpenLibrary("gadgets/space.gadget", 44);
    
    if (!iTidy_DefIconsCreation_WindowBase || 
        !iTidy_DefIconsCreation_LayoutBase ||
        !iTidy_DefIconsCreation_ChooserBase ||
        !iTidy_DefIconsCreation_ButtonBase ||
        !iTidy_DefIconsCreation_CheckBoxBase ||
        !iTidy_DefIconsCreation_LabelBase ||
        !iTidy_DefIconsCreation_ClickTabBase ||
        !iTidy_DefIconsCreation_SpaceBase)
    {
        log_error(LOG_GUI, "Failed to open ReAction libraries for DefIcons creation window\n");
        return FALSE;
    }
    
    return TRUE;
}

/*
 * Close ReAction libraries
 */
static void close_reaction_libs(void)
{
    if (iTidy_DefIconsCreation_SpaceBase)
    {
        CloseLibrary(iTidy_DefIconsCreation_SpaceBase);
        iTidy_DefIconsCreation_SpaceBase = NULL;
    }
    if (iTidy_DefIconsCreation_ClickTabBase)
    {
        CloseLibrary(iTidy_DefIconsCreation_ClickTabBase);
        iTidy_DefIconsCreation_ClickTabBase = NULL;
    }
    if (iTidy_DefIconsCreation_CheckBoxBase)
    {
        CloseLibrary(iTidy_DefIconsCreation_CheckBoxBase);
        iTidy_DefIconsCreation_CheckBoxBase = NULL;
    }
    if (iTidy_DefIconsCreation_ButtonBase)
    {
        CloseLibrary(iTidy_DefIconsCreation_ButtonBase);
        iTidy_DefIconsCreation_ButtonBase = NULL;
    }
    if (iTidy_DefIconsCreation_LabelBase)
    {
        CloseLibrary(iTidy_DefIconsCreation_LabelBase);
        iTidy_DefIconsCreation_LabelBase = NULL;
    }
    if (iTidy_DefIconsCreation_ChooserBase)
    {
        CloseLibrary(iTidy_DefIconsCreation_ChooserBase);
        iTidy_DefIconsCreation_ChooserBase = NULL;
    }
    if (iTidy_DefIconsCreation_LayoutBase)
    {
        CloseLibrary(iTidy_DefIconsCreation_LayoutBase);
        iTidy_DefIconsCreation_LayoutBase = NULL;
    }
    if (iTidy_DefIconsCreation_WindowBase)
    {
        CloseLibrary(iTidy_DefIconsCreation_WindowBase);
        iTidy_DefIconsCreation_WindowBase = NULL;
    }
}

/*
 * Helper function to create a ClickTab labels list
 */
static struct List *make_clicktab_list(STRPTR *labels)
{
    struct List *list = (struct List *)whd_malloc(sizeof(struct List));
    if (list)
    {
        WORD tab_number = 0;
        NewList(list);
        while (*labels)
        {
            struct Node *node = AllocClickTabNode(TNA_Text, *labels, TNA_Number, tab_number, TAG_END);
            if (node)
            {
                AddTail(list, node);
            }
            labels++;
            tab_number++;
        }
    }
    return list;
}

/*
 * Helper function to free a ClickTab labels list
 */
static void free_clicktab_list(struct List *list)
{
    if (list)
    {
        struct Node *node, *next;
        node = list->lh_Head;
        while ((next = node->ln_Succ))
        {
            FreeClickTabNode(node);
            node = next;
        }
        whd_free(list);
    }
}

/*
 * Helper function to create a chooser list from a string array
 */
static struct List* make_chooser_list(STRPTR *labels)
{
    struct List *list = (struct List *)whd_malloc(sizeof(struct List));
    if (list)
    {
        NewList(list);
        while (*labels)
        {
            struct Node *node = AllocChooserNode(CNA_Text, *labels, TAG_END);
            if (node)
            {
                AddTail(list, node);
            }
            labels++;
        }
    }
    return list;
}

/*
 * Helper function to free a chooser list
 */
static void free_chooser_list(struct List *list)
{
    if (list)
    {
        struct Node *node, *next;
        node = list->lh_Head;
        while ((next = node->ln_Succ))
        {
            FreeChooserNode(node);
            node = next;
        }
        whd_free(list);
    }
}

/*
 * Handle OK button - save changes
 */
static void handle_ok(DefIconsCreationWindow *win)
{
    ULONG selected_value = 0;
    
    /* Get folder icon mode from chooser */
    GetAttr(CHOOSER_Selected, win->folder_mode_chooser_obj, &selected_value);
    win->prefs->deficons_folder_icon_mode = (UWORD)selected_value;

    /* Get WHDLoad skip checkbox state */
    selected_value = 0;
    GetAttr(GA_Selected, win->whdload_skip_cb, &selected_value);
    win->prefs->deficons_skip_whdload_folders = (BOOL)selected_value;

    /* Get icon size mode from chooser */
    selected_value = 0;
    GetAttr(CHOOSER_Selected, win->icon_size_chooser_obj, &selected_value);
    win->prefs->deficons_icon_size_mode = (UWORD)selected_value;
    
    /* Get thumbnail border mode from chooser */
    selected_value = 0;
    GetAttr(CHOOSER_Selected, win->thumbnail_borders_chooser_obj, &selected_value);
    win->prefs->deficons_thumbnail_border_mode = (UWORD)selected_value;
    
    /* Get text preview checkbox state */
    selected_value = 0;
    GetAttr(GA_Selected, win->text_preview_checkbox, &selected_value);
    win->prefs->deficons_enable_text_previews = (BOOL)selected_value;
    log_info(LOG_GUI, "Text preview thumbnails toggled: %s\n",
             selected_value ? "enabled" : "disabled");
    
    /* Get picture preview checkbox state */
    selected_value = 0;
    GetAttr(GA_Selected, win->picture_preview_checkbox, &selected_value);
    win->prefs->deficons_enable_picture_previews = (BOOL)selected_value;
    log_info(LOG_GUI, "Picture preview thumbnails toggled: %s\n",
             selected_value ? "enabled" : "disabled");

    /* Get upscale thumbnails checkbox state */
    selected_value = 0;
    GetAttr(GA_Selected, win->upscale_thumbnails_cb, &selected_value);
    win->prefs->deficons_upscale_thumbnails = (BOOL)selected_value;
    log_info(LOG_GUI, "Upscale thumbnails: %s\n",
             selected_value ? "enabled" : "disabled");

    /* Get replace image thumbnails checkbox state */
    selected_value = 0;
    GetAttr(GA_Selected, win->replace_thumbnails_cb, &selected_value);
    win->prefs->deficons_replace_itidy_thumbnails = (BOOL)selected_value;
    log_info(LOG_GUI, "Replace iTidy image thumbnails: %s\n",
             selected_value ? "enabled" : "disabled");

    /* Get replace text previews checkbox state */
    selected_value = 0;
    GetAttr(GA_Selected, win->replace_text_previews_cb, &selected_value);
    win->prefs->deficons_replace_itidy_text_previews = (BOOL)selected_value;
    log_info(LOG_GUI, "Replace iTidy text previews: %s\n",
             selected_value ? "enabled" : "disabled");

    /* Build per-format bitmask from the seven format checkboxes */
    {
        ULONG fmt_bits = 0;
        ULONG v = 0;

        GetAttr(GA_Selected, win->fmt_ilbm_cb,  &v); if (v) fmt_bits |= ITIDY_PICTFMT_ILBM;
        v = 0;
        GetAttr(GA_Selected, win->fmt_png_cb,   &v); if (v) fmt_bits |= ITIDY_PICTFMT_PNG;
        v = 0;
        GetAttr(GA_Selected, win->fmt_gif_cb,   &v); if (v) fmt_bits |= ITIDY_PICTFMT_GIF;
        v = 0;
        GetAttr(GA_Selected, win->fmt_jpeg_cb,  &v); if (v) fmt_bits |= ITIDY_PICTFMT_JPEG;
        v = 0;
        GetAttr(GA_Selected, win->fmt_bmp_cb,   &v); if (v) fmt_bits |= ITIDY_PICTFMT_BMP;
        v = 0;
        GetAttr(GA_Selected, win->fmt_acbm_cb,  &v); if (v) fmt_bits |= ITIDY_PICTFMT_ACBM;
        v = 0;
        GetAttr(GA_Selected, win->fmt_other_cb, &v); if (v) fmt_bits |= ITIDY_PICTFMT_OTHER;

        win->prefs->deficons_picture_formats_enabled = fmt_bits;
        log_info(LOG_GUI, "Picture format bitmask: 0x%08lX\n", fmt_bits);
    }
    
    /* Get max colors chooser state */
    selected_value = 0;
    GetAttr(CHOOSER_Selected, win->max_colors_chooser_obj, &selected_value);
    if (selected_value == ITIDY_MAX_COLORS_ULTRA_INDEX)
    {
        win->prefs->deficons_ultra_mode        = TRUE;
        win->prefs->deficons_harmonised_palette = FALSE;
        win->prefs->deficons_max_icon_colors   = 256;
    }
    else if (selected_value == ITIDY_MAX_COLORS_HARMONISED_INDEX)
    {
        win->prefs->deficons_harmonised_palette = TRUE;
        win->prefs->deficons_ultra_mode        = FALSE;
        win->prefs->deficons_max_icon_colors   = 29;  /* informational only */
    }
    else
    {
        win->prefs->deficons_ultra_mode        = FALSE;
        win->prefs->deficons_harmonised_palette = FALSE;
        win->prefs->deficons_max_icon_colors   = itidy_max_colors_from_index((UWORD)selected_value);
    }
    
    /* Get dither method chooser state */
    selected_value = 0;
    GetAttr(CHOOSER_Selected, win->dither_method_chooser_obj, &selected_value);
    win->prefs->deficons_dither_method = (UWORD)selected_value;
    
    /* Get low-color mapping chooser state */
    selected_value = 0;
    GetAttr(CHOOSER_Selected, win->lowcolor_mapping_chooser_obj, &selected_value);
    win->prefs->deficons_lowcolor_mapping = (UWORD)selected_value;
    
    log_info(LOG_GUI, "DefIcons creation settings saved: folder_mode=%d, icon_size=%d, "
             "border_mode=%d, text_preview=%d, picture_preview=%d, max_colors=%d, "
             "ultra=%d, harmonised=%d, dither=%d, lowcolor=%d\n",
             win->prefs->deficons_folder_icon_mode,
             win->prefs->deficons_icon_size_mode,
             win->prefs->deficons_thumbnail_border_mode,
             win->prefs->deficons_enable_text_previews,
             win->prefs->deficons_enable_picture_previews,
             win->prefs->deficons_max_icon_colors,
             win->prefs->deficons_ultra_mode,
             win->prefs->deficons_harmonised_palette,
             win->prefs->deficons_dither_method,
             win->prefs->deficons_lowcolor_mapping);
    
    win->user_accepted = TRUE;
}

/*
 * Handle gadget up events
 */
static void handle_gadget_up(DefIconsCreationWindow *win, ULONG gadget_id)
{
    switch (gadget_id)
    {
        case GID_MAX_COLORS_CHOOSER:
        {
            /* Update ghost state of dither/lowcolor choosers */
            ULONG sel = 0;
            BOOL ghost_dither;
            BOOL ghost_lowcolor;
            UWORD max_col;

            GetAttr(CHOOSER_Selected, win->max_colors_chooser_obj, &sel);
            max_col = itidy_max_colors_from_index((UWORD)sel);

            /* Ultra (index 8) or 256 -> no reduction, ghost dither */
            ghost_dither = (sel == ITIDY_MAX_COLORS_ULTRA_INDEX) || (max_col >= 256);
            /* Lowcolor mapping only relevant for <= 8 colors;
             * also ghosted for Harmonised (fixed palette, mapping N/A) */
            ghost_lowcolor = (sel == ITIDY_MAX_COLORS_ULTRA_INDEX)
                          || (sel == ITIDY_MAX_COLORS_HARMONISED_INDEX)
                          || (max_col > 8);

            SetGadgetAttrs((struct Gadget *)win->dither_method_chooser_obj,
                win->window, NULL,
                GA_Disabled, ghost_dither, TAG_DONE);
            SetGadgetAttrs((struct Gadget *)win->lowcolor_mapping_chooser_obj,
                win->window, NULL,
                GA_Disabled, ghost_lowcolor, TAG_DONE);
            break;
        }
        
        default:
            /* Other gadgets require no special handling */
            break;
    }
}

/*
 * Create window and gadgets
 */
static BOOL create_window(DefIconsCreationWindow *win)
{
    UWORD max_colors_index;
    BOOL is_ultra;
    BOOL ghost_dither;
    BOOL ghost_lowcolor;
    ULONG fmt_bits;

    /* Create tab labels list */
    win->tab_labels = make_clicktab_list(tab_label_strings);
    if (!win->tab_labels)
    {
        log_error(LOG_GUI, "Failed to create ClickTab labels for DefIcons creation window\n");
        return FALSE;
    }

    /* Create chooser lists (must be done before creating chooser objects) */
    win->folder_mode_list = make_chooser_list(folder_mode_labels);
    win->icon_size_list = make_chooser_list(icon_size_labels);
    win->thumbnail_borders_list = make_chooser_list(thumbnail_border_labels);
    win->max_colors_list = make_chooser_list(max_colors_labels);
    win->dither_method_list = make_chooser_list(dither_method_labels);
    win->lowcolor_mapping_list = make_chooser_list(lowcolor_mapping_labels);

    if (!win->folder_mode_list || !win->icon_size_list ||
        !win->thumbnail_borders_list ||
        !win->max_colors_list || !win->dither_method_list || !win->lowcolor_mapping_list)
    {
        log_error(LOG_GUI, "Failed to create chooser lists\n");
        free_chooser_list(win->folder_mode_list);
        free_chooser_list(win->icon_size_list);
        free_chooser_list(win->thumbnail_borders_list);
        free_chooser_list(win->max_colors_list);
        free_chooser_list(win->dither_method_list);
        free_chooser_list(win->lowcolor_mapping_list);
        free_clicktab_list(win->tab_labels);
        win->tab_labels = NULL;
        return FALSE;
    }

    /* Determine initial chooser indices and ghosting from preferences */
    is_ultra = win->prefs->deficons_ultra_mode;
    if (is_ultra)
        max_colors_index = ITIDY_MAX_COLORS_ULTRA_INDEX;
    else if (win->prefs->deficons_harmonised_palette)
        max_colors_index = ITIDY_MAX_COLORS_HARMONISED_INDEX;
    else
        max_colors_index = itidy_max_colors_to_index(win->prefs->deficons_max_icon_colors);

    /* Ghost dither when 256 colors or Ultra (no reduction needed) */
    ghost_dither   = (win->prefs->deficons_max_icon_colors >= 256) || is_ultra;
    /* Ghost lowcolor when max colors > 8 */
    ghost_lowcolor = (win->prefs->deficons_max_icon_colors > 8) || is_ultra;

    fmt_bits = win->prefs->deficons_picture_formats_enabled;

    /* ---------------------------------------------------------------
     * Build the full window object with ClickTab-based tabbed layout:
     *
     *  [Tab: Create   | Tab: Rendering]
     *  +--[ Create page ]-------------------------------------------+
     *  |  Folder icons: [chooser         ]                          |
     *  |  [x] Text file previews   [Manage templates...]            |
     *  |  [x] Picture file previews                                 |
     *  |  Picture formats:                                          |
     *  |   ILBM  PNG  GIF  BMP                                      |
     *  |   JPEG  ACBM Other                                         |
     *  +------------------------------------------------------------+
     *  +--[ Rendering page ]----------------------------------------+
     *  |  Preview size:    [chooser  ]                              |
     *  |  Thumbnail frame: [chooser  ]                              |
     *  |  [x] Upscale small images to icon size                     |
     *  |  -- Colour reduction --                                    |
     *  |  Max colours:    [chooser  ]                               |
     *  |  Dithering:      [chooser  ]                               |
     *  |  Low-col palette:[chooser  ]                               |
     *  +------------------------------------------------------------+
     *  [ OK ]  [ Cancel ]
     * --------------------------------------------------------------- */
    /* Hint/tooltip info for all interactive gadgets in this window */
    static struct HintInfo hintInfo[] =
    {
        {GID_CLICKTAB,                -1, "", 0},
        {GID_FOLDER_MODE_CHOOSER,     -1, "Controls whether iTidy creates drawer icons for folders that do not already have them.", 0},
        {GID_SKIP_WHDLOAD_CB,         -1, "When enabled, files inside WHDLoad game folders are skipped during icon creation. The WHDLoad folder icon itself is still created.", 0},
        {GID_TEXT_PREVIEW_CHECKBOX,   -1, "When enabled, iTidy renders a preview of a text file's contents onto the icon image.", 0},
        {GID_MANAGE_TEXT_TEMPLATES,   -1, "Opens the Text Templates window to view and edit how different text file types are rendered as icon previews.", 0},
        {GID_PICTURE_PREVIEW_CHECKBOX,-1, "When enabled, iTidy creates thumbnail icons for recognised image files by generating a miniature version of the image.", 0},
        {GID_FMT_ILBM_CB,             -1, "Enable thumbnail creation for Amiga IFF ILBM and PBM images.", 0},
        {GID_FMT_JPEG_CB,             -1, "Enable thumbnail creation for JPEG images. Disabled by default because JPEG decoding is slow on 68k hardware.", 0},
        {GID_FMT_PNG_CB,              -1, "Enable thumbnail creation for PNG images. Supports transparency.", 0},
        {GID_FMT_ACBM_CB,             -1, "Enable thumbnail creation for Amiga Continuous Bitmap images, a rare format used in some older demos and game assets.", 0},
        {GID_FMT_GIF_CB,              -1, "Enable thumbnail creation for GIF images. Supports transparency.", 0},
        {GID_FMT_OTHER_CB,            -1, "Enable thumbnail creation for other image formats supported by installed DataTypes and DefIcons.", 0},
        {GID_FMT_BMP_CB,              -1, "Enable thumbnail creation for Windows BMP images.", 0},
        {GID_REPLACE_THUMBNAILS_CB,   -1, "When enabled, image thumbnail icons previously created by iTidy are deleted and regenerated. User-placed icons are not affected.", 0},
        {GID_REPLACE_TEXT_PREVIEWS_CB,-1, "When enabled, text preview icons previously created by iTidy are deleted and regenerated. User-placed icons are not affected.", 0},
        {GID_ICON_SIZE_CHOOSER,       -1, "Sets the canvas size used for thumbnail icons. Larger sizes show more detail but take up more screen space.", 0},
        {GID_THUMBNAIL_BORDERS_CHOOSER,-1, "Controls the border style drawn around thumbnail icons. The \"Smart\" modes skip the border on images with transparency.", 0},
        {GID_UPSCALE_THUMBNAILS_CB,   -1, "When enabled, images smaller than the preview size are scaled up to fill the thumbnail area.", 0},
        {GID_MAX_COLORS_CHOOSER,      -1, "Sets the maximum number of colours used in generated thumbnail icons. Higher counts look better but produce larger icon files.", 0},
        {GID_DITHER_METHOD_CHOOSER,   -1, "Selects the dithering method used when reducing colours. Disabled when \"Max Colours\" is set to 256 or Ultra.", 0},
        {GID_LOWCOLOR_MAPPING_CHOOSER,-1, "Controls the colour palette used at very low colour counts (4 or 8). Only enabled when \"Max Colours\" is set to 4 or 8.", 0},
        {GID_DEFICONS_CREATION_SETUP,  -1, "Opens the DefIcons Categories window to select which file types should receive icons during processing.", 0},
        {GID_DEFICONS_EXCLUDE_PATHS,   -1, "Opens the Exclude Paths window to manage folders that should be skipped during icon creation.", 0},
        {GID_OK,                      -1, "Accepts the current settings and closes the window. Changes are applied to the active session.", 0},
        {GID_CANCEL,                  -1, "Discards all changes and closes the window. The original settings are preserved.", 0},
        {-1, -1, NULL, 0}
    };

    win->window_obj = (Object *)WindowObject,
        WA_Title,              "iTidy - Icon Creation Settings",
        WA_Activate,           TRUE,
        WA_DepthGadget,        TRUE,
        WA_DragBar,            TRUE,
        WA_CloseGadget,        TRUE,
        WA_SizeGadget,         TRUE,
        WA_MinWidth,           300,
        WA_MinHeight,          200,
        WA_MaxWidth,           8192,
        WA_MaxHeight,          8192,
        WINDOW_Position,       WPOS_CENTERSCREEN,
        WINDOW_HintInfo,       hintInfo,
        WINDOW_GadgetHelp,     TRUE,
        WINDOW_ParentGroup, (Object *)VLayoutObject,
            LAYOUT_SpaceOuter,  TRUE,
            LAYOUT_DeferLayout, TRUE,

            /* ---- ClickTab row ---- */
            LAYOUT_AddChild, win->clicktab_obj = (Object *)ClickTabObject,
                GA_ID,          GID_CLICKTAB,
                GA_RelVerify,   TRUE,
                GA_TabCycle,    TRUE,
                CLICKTAB_Labels, win->tab_labels,
                CLICKTAB_PageGroup, (Object *)PageObject,
                    LAYOUT_DeferLayout, TRUE,

                    /* ============ Page 0: Create ============ */
                    PAGE_Add, (Object *)VLayoutObject,
                        LAYOUT_LeftSpacing,   4,
                        LAYOUT_RightSpacing,  4,
                        LAYOUT_TopSpacing,    4,
                        LAYOUT_BottomSpacing, 4,

                        /* Folder icons chooser */
                        LAYOUT_AddChild, win->folder_mode_chooser_obj = (Object *)ChooserObject,
                            GA_ID,             GID_FOLDER_MODE_CHOOSER,
                            GA_RelVerify,      TRUE,
                            GA_TabCycle,       TRUE,
                            CHOOSER_PopUp,     TRUE,
                            CHOOSER_Labels,    win->folder_mode_list,
                            CHOOSER_Selected,  win->prefs->deficons_folder_icon_mode,
                        ChooserEnd,
                        CHILD_Label, (Object *)LabelObject,
                            LABEL_Text, "Folder Icons:",
                        LabelEnd,
                        CHILD_WeightedHeight, 0,

                        /* WHDLoad skip checkbox */
                        LAYOUT_AddChild, win->whdload_skip_cb = (Object *)CheckBoxObject,
                            GA_ID,              GID_SKIP_WHDLOAD_CB,
                            GA_Text,            "Skip Files In WHDLoad Folders",
                            GA_Selected,        win->prefs->deficons_skip_whdload_folders,
                            GA_RelVerify,       TRUE,
                            GA_TabCycle,        TRUE,
                            CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                        CheckBoxEnd,
                        CHILD_WeightedHeight, 0,

                        /* Spacer */
                        LAYOUT_AddChild, (Object *)SpaceObject,
                        SpaceEnd,
                        CHILD_WeightedHeight, 0,

                        /* Text preview row: checkbox + Manage Templates button */
                        LAYOUT_AddChild, (Object *)HLayoutObject,
                            LAYOUT_AddChild, win->text_preview_checkbox = (Object *)CheckBoxObject,
                                GA_ID,                 GID_TEXT_PREVIEW_CHECKBOX,
                                GA_Text,               "Text File Previews",
                                GA_Selected,           win->prefs->deficons_enable_text_previews,
                                GA_RelVerify,          TRUE,
                                GA_TabCycle,           TRUE,
                                CHECKBOX_TextPlace,    PLACETEXT_RIGHT,
                            CheckBoxEnd,
                            LAYOUT_AddChild, win->manage_text_templates_btn = (Object *)ButtonObject,
                                GA_ID,        GID_MANAGE_TEXT_TEMPLATES,
                                GA_Text,      "Manage Templates...",
                                GA_RelVerify, TRUE,
                                GA_TabCycle,  TRUE,
                            ButtonEnd,
                        LayoutEnd,
                        CHILD_WeightedHeight, 0,

                        /* Picture preview checkbox row */
                        LAYOUT_AddChild, (Object *)HLayoutObject,
                            LAYOUT_AddChild, win->picture_preview_checkbox = (Object *)CheckBoxObject,
                                GA_ID,              GID_PICTURE_PREVIEW_CHECKBOX,
                                GA_Text,            "Picture File Previews",
                                GA_Selected,        win->prefs->deficons_enable_picture_previews,
                                GA_RelVerify,       TRUE,
                                GA_TabCycle,        TRUE,
                                CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                            CheckBoxEnd,
                        LayoutEnd,
                        CHILD_WeightedHeight, 0,

                        /* Spacer */
                        LAYOUT_AddChild, (Object *)SpaceObject,
                        SpaceEnd,
                        CHILD_WeightedHeight, 0,

                        /* Picture formats label */
                        LAYOUT_AddImage, (Object *)LabelObject,
                            LABEL_Text, "Picture Formats:",
                        LabelEnd,

                        /* Picture format checkboxes: 4 columns */
                        LAYOUT_AddChild, (Object *)HLayoutObject,
                            /* Column 1: ILBM, JPEG */
                            LAYOUT_AddChild, (Object *)VLayoutObject,
                                LAYOUT_AddChild, win->fmt_ilbm_cb = (Object *)CheckBoxObject,
                                    GA_ID,              GID_FMT_ILBM_CB,
                                    GA_Text,            "ILBM (IFF)",
                                    GA_Selected,        (fmt_bits & ITIDY_PICTFMT_ILBM) ? TRUE : FALSE,
                                    GA_RelVerify,       TRUE,
                                    GA_TabCycle,        TRUE,
                                    CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                                CheckBoxEnd,
                                CHILD_WeightedHeight, 0,
                                LAYOUT_AddChild, win->fmt_jpeg_cb = (Object *)CheckBoxObject,
                                    GA_ID,              GID_FMT_JPEG_CB,
                                    GA_Text,            "JPEG (Slow)",
                                    GA_Selected,        (fmt_bits & ITIDY_PICTFMT_JPEG) ? TRUE : FALSE,
                                    GA_RelVerify,       TRUE,
                                    GA_TabCycle,        TRUE,
                                    CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                                CheckBoxEnd,
                                CHILD_WeightedHeight, 0,
                            LayoutEnd,
                            /* Column 2: PNG, ACBM */
                            LAYOUT_AddChild, (Object *)VLayoutObject,
                                LAYOUT_AddChild, win->fmt_png_cb = (Object *)CheckBoxObject,
                                    GA_ID,              GID_FMT_PNG_CB,
                                    GA_Text,            "PNG",
                                    GA_Selected,        (fmt_bits & ITIDY_PICTFMT_PNG) ? TRUE : FALSE,
                                    GA_RelVerify,       TRUE,
                                    GA_TabCycle,        TRUE,
                                    CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                                CheckBoxEnd,
                                CHILD_WeightedHeight, 0,
                                LAYOUT_AddChild, win->fmt_acbm_cb = (Object *)CheckBoxObject,
                                    GA_ID,              GID_FMT_ACBM_CB,
                                    GA_Text,            "ACBM",
                                    GA_Selected,        (fmt_bits & ITIDY_PICTFMT_ACBM) ? TRUE : FALSE,
                                    GA_RelVerify,       TRUE,
                                    GA_TabCycle,        TRUE,
                                    CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                                CheckBoxEnd,
                                CHILD_WeightedHeight, 0,
                            LayoutEnd,
                            /* Column 3: GIF, Other */
                            LAYOUT_AddChild, (Object *)VLayoutObject,
                                LAYOUT_AddChild, win->fmt_gif_cb = (Object *)CheckBoxObject,
                                    GA_ID,              GID_FMT_GIF_CB,
                                    GA_Text,            "GIF",
                                    GA_Selected,        (fmt_bits & ITIDY_PICTFMT_GIF) ? TRUE : FALSE,
                                    GA_RelVerify,       TRUE,
                                    GA_TabCycle,        TRUE,
                                    CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                                CheckBoxEnd,
                                CHILD_WeightedHeight, 0,
                                LAYOUT_AddChild, win->fmt_other_cb = (Object *)CheckBoxObject,
                                    GA_ID,              GID_FMT_OTHER_CB,
                                    GA_Text,            "Other",
                                    GA_Selected,        (fmt_bits & ITIDY_PICTFMT_OTHER) ? TRUE : FALSE,
                                    GA_RelVerify,       TRUE,
                                    GA_TabCycle,        TRUE,
                                    CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                                CheckBoxEnd,
                                CHILD_WeightedHeight, 0,
                            LayoutEnd,
                            /* Column 4: BMP */
                            LAYOUT_AddChild, (Object *)VLayoutObject,
                                LAYOUT_AddChild, win->fmt_bmp_cb = (Object *)CheckBoxObject,
                                    GA_ID,              GID_FMT_BMP_CB,
                                    GA_Text,            "BMP",
                                    GA_Selected,        (fmt_bits & ITIDY_PICTFMT_BMP) ? TRUE : FALSE,
                                    GA_RelVerify,       TRUE,
                                    GA_TabCycle,        TRUE,
                                    CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                                CheckBoxEnd,
                                CHILD_WeightedHeight, 0,
                            LayoutEnd,
                        LayoutEnd,
                        CHILD_WeightedHeight, 0,

                        /* Spacer before replace options */
                        LAYOUT_AddChild, (Object *)SpaceObject,
                        SpaceEnd,
                        CHILD_WeightedHeight, 0,

                        /* Replace section label */
                        LAYOUT_AddImage, (Object *)LabelObject,
                            LABEL_Text, "Re-Run and Refresh Options:",
                        LabelEnd,

                        /* Replace image thumbnails checkbox */
                        LAYOUT_AddChild, win->replace_thumbnails_cb = (Object *)CheckBoxObject,
                            GA_ID,              GID_REPLACE_THUMBNAILS_CB,
                            GA_Text,            "Replace Existing Image Thumbnails Created by iTidy",
                            GA_Selected,        win->prefs->deficons_replace_itidy_thumbnails,
                            GA_RelVerify,       TRUE,
                            GA_TabCycle,        TRUE,
                            CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                        CheckBoxEnd,
                        CHILD_WeightedHeight, 0,

                        /* Replace text previews checkbox */
                        LAYOUT_AddChild, win->replace_text_previews_cb = (Object *)CheckBoxObject,
                            GA_ID,              GID_REPLACE_TEXT_PREVIEWS_CB,
                            GA_Text,            "Replace Existing Text Previews Created by iTidy",
                            GA_Selected,        win->prefs->deficons_replace_itidy_text_previews,
                            GA_RelVerify,       TRUE,
                            GA_TabCycle,        TRUE,
                            CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                        CheckBoxEnd,
                        CHILD_WeightedHeight, 0,

                        /* Bottom spacer */
                        LAYOUT_AddChild, (Object *)SpaceObject,
                        SpaceEnd,
                    LayoutEnd, /* end Create page */

                    /* ============ Page 1: Rendering ============ */
                    PAGE_Add, (Object *)VLayoutObject,

                        /* Size / border / upscale group */
                        LAYOUT_AddChild, (Object *)VLayoutObject,
                            LAYOUT_LeftSpacing,   4,
                            LAYOUT_RightSpacing,  4,
                            LAYOUT_TopSpacing,    4,
                            LAYOUT_BottomSpacing, 4,

                            LAYOUT_AddChild, win->icon_size_chooser_obj = (Object *)ChooserObject,
                                GA_ID,            GID_ICON_SIZE_CHOOSER,
                                GA_RelVerify,     TRUE,
                                GA_TabCycle,      TRUE,
                                CHOOSER_PopUp,    TRUE,
                                CHOOSER_Labels,   win->icon_size_list,
                                CHOOSER_Selected, win->prefs->deficons_icon_size_mode,
                            ChooserEnd,
                            CHILD_Label, (Object *)LabelObject,
                                LABEL_Text, "Preview Size:",
                            LabelEnd,
                            CHILD_WeightedHeight, 0,

                            LAYOUT_AddChild, win->thumbnail_borders_chooser_obj = (Object *)ChooserObject,
                                GA_ID,            GID_THUMBNAIL_BORDERS_CHOOSER,
                                GA_RelVerify,     TRUE,
                                GA_TabCycle,      TRUE,
                                CHOOSER_PopUp,    TRUE,
                                CHOOSER_Labels,   win->thumbnail_borders_list,
                                CHOOSER_Selected, (ULONG)win->prefs->deficons_thumbnail_border_mode,
                            ChooserEnd,
                            CHILD_Label, (Object *)LabelObject,
                                LABEL_Text, "Thumbnail Border:",
                            LabelEnd,
                            CHILD_WeightedHeight, 0,

                            LAYOUT_AddChild, win->upscale_thumbnails_cb = (Object *)CheckBoxObject,
                                GA_ID,              GID_UPSCALE_THUMBNAILS_CB,
                                GA_Text,            "Upscale Small Images",
                                GA_Selected,        win->prefs->deficons_upscale_thumbnails,
                                GA_RelVerify,       TRUE,
                                GA_TabCycle,        TRUE,
                                CHECKBOX_TextPlace, PLACETEXT_LEFT,
                            CheckBoxEnd,
                            CHILD_WeightedHeight, 0,
                        LayoutEnd,
                        CHILD_WeightedHeight, 0,

                        /* Spacer between sections */
                        LAYOUT_AddChild, (Object *)SpaceObject,
                        SpaceEnd,
                        CHILD_WeightedHeight, 0,

                        /* Colour reduction group */
                        LAYOUT_AddChild, (Object *)VLayoutObject,
                            LAYOUT_LeftSpacing,   4,
                            LAYOUT_RightSpacing,  4,
                            LAYOUT_TopSpacing,    4,
                            LAYOUT_BottomSpacing, 4,
                            LAYOUT_LabelPlace,    BVJ_TOP_LEFT,

                            LAYOUT_AddImage, (Object *)LabelObject,
                                LABEL_Text, "Colour Reduction:",
                            LabelEnd,

                            LAYOUT_AddChild, win->max_colors_chooser_obj = (Object *)ChooserObject,
                                GA_ID,            GID_MAX_COLORS_CHOOSER,
                                GA_RelVerify,     TRUE,
                                GA_TabCycle,      TRUE,
                                GA_Disabled,      FALSE,
                                CHOOSER_PopUp,    TRUE,
                                CHOOSER_Labels,   win->max_colors_list,
                                CHOOSER_Selected, (ULONG)max_colors_index,
                            ChooserEnd,
                            CHILD_Label, (Object *)LabelObject,
                                LABEL_Text, "Max Colours:",
                            LabelEnd,
                            CHILD_WeightedHeight, 0,

                            LAYOUT_AddChild, win->dither_method_chooser_obj = (Object *)ChooserObject,
                                GA_ID,            GID_DITHER_METHOD_CHOOSER,
                                GA_RelVerify,     TRUE,
                                GA_TabCycle,      TRUE,
                                GA_Disabled,      ghost_dither,
                                CHOOSER_PopUp,    TRUE,
                                CHOOSER_Labels,   win->dither_method_list,
                                CHOOSER_Selected, win->prefs->deficons_dither_method,
                            ChooserEnd,
                            CHILD_Label, (Object *)LabelObject,
                                LABEL_Text, "Dithering:",
                            LabelEnd,
                            CHILD_WeightedHeight, 0,

                            LAYOUT_AddChild, win->lowcolor_mapping_chooser_obj = (Object *)ChooserObject,
                                GA_ID,            GID_LOWCOLOR_MAPPING_CHOOSER,
                                GA_RelVerify,     TRUE,
                                GA_TabCycle,      TRUE,
                                GA_Disabled,      ghost_lowcolor,
                                CHOOSER_PopUp,    TRUE,
                                CHOOSER_Labels,   win->lowcolor_mapping_list,
                                CHOOSER_Selected, win->prefs->deficons_lowcolor_mapping,
                            ChooserEnd,
                            CHILD_Label, (Object *)LabelObject,
                                LABEL_Text, "Low-Colour Palette:",
                            LabelEnd,
                            CHILD_WeightedHeight, 0,
                        LayoutEnd,
                        CHILD_WeightedHeight, 0,

                    LayoutEnd, /* end Rendering page */

                    /* ============ Page 2: DefIcons ============ */
                    PAGE_Add, (Object *)VLayoutObject,

                        /* Top spacers */
                        LAYOUT_AddChild, (Object *)SpaceObject,
                        SpaceEnd,

                        LAYOUT_AddChild, (Object *)SpaceObject,
                        SpaceEnd,

                        /* Icon Creation Setup button */
                        LAYOUT_AddChild, win->deficons_creation_setup_btn = (Object *)ButtonObject,
                            GA_ID,        GID_DEFICONS_CREATION_SETUP,
                            GA_Text,      "Icon Creation Setup...",
                            GA_RelVerify, TRUE,
                            GA_TabCycle,  TRUE,
                        ButtonEnd,
                        CHILD_WeightedHeight, 0,

                        /* Exclude Paths button */
                        LAYOUT_AddChild, win->deficons_exclude_paths_btn = (Object *)ButtonObject,
                            GA_ID,        GID_DEFICONS_EXCLUDE_PATHS,
                            GA_Text,      "Exclude Paths...",
                            GA_RelVerify, TRUE,
                            GA_TabCycle,  TRUE,
                        ButtonEnd,
                        CHILD_WeightedHeight, 0,

                        /* Bottom spacers */
                        LAYOUT_AddChild, (Object *)SpaceObject,
                        SpaceEnd,

                        LAYOUT_AddChild, (Object *)SpaceObject,
                        SpaceEnd,

                    LayoutEnd, /* end DefIcons page */

                PageEnd, /* end PageObject */
            ClickTabEnd, /* end ClickTab */

            /* ---- OK / Cancel button row (outside tabs) ---- */
            LAYOUT_AddChild, (Object *)HLayoutObject,
                LAYOUT_AddChild, win->ok_btn = (Object *)ButtonObject,
                    GA_ID,        GID_OK,
                    GA_Text,      "_OK",
                    GA_RelVerify, TRUE,
                    GA_TabCycle,  TRUE,
                ButtonEnd,
                LAYOUT_AddChild, win->cancel_btn = (Object *)ButtonObject,
                    GA_ID,        GID_CANCEL,
                    GA_Text,      "_Cancel",
                    GA_RelVerify, TRUE,
                    GA_TabCycle,  TRUE,
                ButtonEnd,
            LayoutEnd,
            CHILD_WeightedHeight, 0,

        LayoutEnd, /* end WINDOW_ParentGroup */
    WindowEnd;

    if (win->window_obj == NULL)
    {
        log_error(LOG_GUI, "Failed to create DefIcons creation window object\n");
        return FALSE;
    }

    /* Open the window */
    win->window = (struct Window *)RA_OpenWindow(win->window_obj);
    if (win->window == NULL)
    {
        log_error(LOG_GUI, "Failed to open DefIcons creation window\n");
        DisposeObject(win->window_obj);
        win->window_obj = NULL;
        return FALSE;
    }

    log_info(LOG_GUI, "DefIcons creation settings window opened\n");
    return TRUE;
}

/*
 * Event loop
 */
static void event_loop(DefIconsCreationWindow *win)
{
    ULONG signal_mask, signals;
    ULONG result;
    UWORD code;
    BOOL done = FALSE;
    
    /* Get signal mask */
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
                    ULONG gadget_id = result & WMHI_GADGETMASK;
                    
                    switch (gadget_id)
                    {
                        case GID_OK:
                            handle_ok(win);
                            done = TRUE;
                            break;

                        case GID_CANCEL:
                            done = TRUE;
                            break;

                        case GID_MANAGE_TEXT_TEMPLATES:
                            open_text_templates_window(win->prefs);
                            break;

                        case GID_DEFICONS_CREATION_SETUP:
                            open_itidy_deficons_settings_window(win->prefs);
                            break;

                        case GID_DEFICONS_EXCLUDE_PATHS:
                            open_exclude_paths_window(GetGlobalExcludePaths(), NULL);
                            break;

                        default:
                            handle_gadget_up(win, gadget_id);
                            break;
                    }
                    break;
                }
            }
        }
    }
}

/*
 * Cleanup window
 */
static void cleanup_window(DefIconsCreationWindow *win)
{
    if (win->window_obj)
    {
        DisposeObject(win->window_obj);
        win->window_obj = NULL;
    }

    /* Free ClickTab labels */
    if (win->tab_labels)
    {
        free_clicktab_list(win->tab_labels);
        win->tab_labels = NULL;
    }

    /* Free chooser lists */
    free_chooser_list(win->folder_mode_list);
    free_chooser_list(win->icon_size_list);
    free_chooser_list(win->thumbnail_borders_list);
    free_chooser_list(win->max_colors_list);
    free_chooser_list(win->dither_method_list);
    free_chooser_list(win->lowcolor_mapping_list);
    
    log_info(LOG_GUI, "DefIcons creation settings window closed\n");
}

/*
 * Public API function
 */
BOOL open_itidy_deficons_creation_window(LayoutPreferences *prefs)
{
    DefIconsCreationWindow win;
    BOOL result;
    
    if (prefs == NULL)
    {
        log_error(LOG_GUI, "NULL prefs passed to DefIcons creation window\n");
        return FALSE;
    }
    
    /* Initialize window structure */
    memset(&win, 0, sizeof(DefIconsCreationWindow));
    win.prefs = prefs;
    win.user_accepted = FALSE;
    
    /* Open libraries */
    if (!open_reaction_libs())
    {
        return FALSE;
    }
    
    /* Create window */
    if (!create_window(&win))
    {
        close_reaction_libs();
        return FALSE;
    }
    
    /* Run event loop */
    event_loop(&win);
    
    /* Get result before cleanup */
    result = win.user_accepted;
    
    /* Cleanup */
    cleanup_window(&win);
    close_reaction_libs();
    
    return result;
}
