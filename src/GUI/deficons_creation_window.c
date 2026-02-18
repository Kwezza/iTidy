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

#include "deficons_creation_window.h"
#include "writeLog.h"
#include "icon_edit/palette/palette_reduction.h"
#include "platform/platform.h"

#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/chooser.h>
#include <proto/button.h>
#include <proto/checkbox.h>
#include <proto/label.h>
#include <proto/utility.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/chooser.h>
#include <gadgets/button.h>
#include <gadgets/checkbox.h>
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

/* Gadget IDs */
enum {
    GID_FOLDER_MODE_CHOOSER = 1,
    GID_ICON_SIZE_CHOOSER,
    GID_THUMBNAIL_BORDERS_CHECKBOX,
    GID_TEXT_PREVIEW_CHECKBOX,
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
    Object *folder_mode_chooser_obj;
    Object *icon_size_chooser_obj;
    Object *thumbnail_borders_checkbox;
    Object *text_preview_checkbox;
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
    Object *ok_btn;
    Object *cancel_btn;
    
    struct Window *window;
    
    /* Chooser lists (must be allocated with AllocChooserNode) */
    struct List *folder_mode_list;
    struct List *icon_size_list;
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
    "Smart (create if visible)",
    "Always create",
    "Never create",
    NULL
};

/* Icon size chooser labels (matches ITIDY_ICON_SIZE_* constants) */
static STRPTR icon_size_labels[] = {
    "Small (48x48)",
    "Medium (64x64)",
    "Large (100x100)",
    NULL
};

/* Max icon colors chooser labels (indices 0-7) */
static STRPTR max_colors_labels[] = {
    "4 colors",
    "8 colors",
    "16 colors",
    "32 colors",
    "64 colors",
    "128 colors",
    "256 colors (full)",
    "Ultra (256 + detail boost)",
    NULL
};

/* Dithering method chooser labels */
static STRPTR dither_method_labels[] = {
    "None",
    "Ordered (Bayer 4x4)",
    "Error diffusion (Floyd-Steinberg)",
    "Auto (based on color count)",
    NULL
};

/* Low-color mapping chooser labels (used when max colors <= 8) */
static STRPTR lowcolor_mapping_labels[] = {
    "Grayscale",
    "Workbench palette",
    "Hybrid (grays + WB accents)",
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
    iTidy_DefIconsCreation_LabelBase = OpenLibrary("gadgets/label.image", 44);
    
    if (!iTidy_DefIconsCreation_WindowBase || 
        !iTidy_DefIconsCreation_LayoutBase ||
        !iTidy_DefIconsCreation_ChooserBase ||
        !iTidy_DefIconsCreation_ButtonBase ||
        !iTidy_DefIconsCreation_CheckBoxBase ||
        !iTidy_DefIconsCreation_LabelBase)
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
    
    /* Get icon size mode from chooser */
    selected_value = 0;
    GetAttr(CHOOSER_Selected, win->icon_size_chooser_obj, &selected_value);
    win->prefs->deficons_icon_size_mode = (UWORD)selected_value;
    
    /* Get thumbnail borders checkbox state */
    selected_value = 0;
    GetAttr(GA_Selected, win->thumbnail_borders_checkbox, &selected_value);
    win->prefs->deficons_enable_thumbnail_borders = (BOOL)selected_value;
    
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
        win->prefs->deficons_ultra_mode = TRUE;
        win->prefs->deficons_max_icon_colors = 256;
    }
    else
    {
        win->prefs->deficons_ultra_mode = FALSE;
        win->prefs->deficons_max_icon_colors = itidy_max_colors_from_index((UWORD)selected_value);
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
             "borders=%d, text_preview=%d, picture_preview=%d, max_colors=%d, "
             "ultra=%d, dither=%d, lowcolor=%d\n",
             win->prefs->deficons_folder_icon_mode,
             win->prefs->deficons_icon_size_mode,
             win->prefs->deficons_enable_thumbnail_borders,
             win->prefs->deficons_enable_text_previews,
             win->prefs->deficons_enable_picture_previews,
             win->prefs->deficons_max_icon_colors,
             win->prefs->deficons_ultra_mode,
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

            /* Ultra (index 7) or 256 -> no reduction, ghost dither */
            ghost_dither = (sel == ITIDY_MAX_COLORS_ULTRA_INDEX) || (max_col >= 256);
            /* Lowcolor mapping only relevant for <= 8 colors */
            ghost_lowcolor = (sel == ITIDY_MAX_COLORS_ULTRA_INDEX) || (max_col > 8);

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
    /* Create chooser lists (must be done before creating chooser objects) */
    win->folder_mode_list = make_chooser_list(folder_mode_labels);
    win->icon_size_list = make_chooser_list(icon_size_labels);
    win->max_colors_list = make_chooser_list(max_colors_labels);
    win->dither_method_list = make_chooser_list(dither_method_labels);
    win->lowcolor_mapping_list = make_chooser_list(lowcolor_mapping_labels);
    
    if (!win->folder_mode_list || !win->icon_size_list ||
        !win->max_colors_list || !win->dither_method_list || !win->lowcolor_mapping_list)
    {
        log_error(LOG_GUI, "Failed to create chooser lists\n");
        /* Free any lists that were created before the failure */
        free_chooser_list(win->folder_mode_list);
        free_chooser_list(win->icon_size_list);
        free_chooser_list(win->max_colors_list);
        free_chooser_list(win->dither_method_list);
        free_chooser_list(win->lowcolor_mapping_list);
        return FALSE;
    }
    
    /* Create gadget objects */
    win->folder_mode_chooser_obj = (Object *)ChooserObject,
        GA_ID, GID_FOLDER_MODE_CHOOSER,
        GA_RelVerify, TRUE,
        CHOOSER_Labels, win->folder_mode_list,
        CHOOSER_Selected, win->prefs->deficons_folder_icon_mode,
    ChooserEnd;
    
    win->icon_size_chooser_obj = (Object *)ChooserObject,
        GA_ID, GID_ICON_SIZE_CHOOSER,
        GA_RelVerify, TRUE,
        CHOOSER_Labels, win->icon_size_list,
        CHOOSER_Selected, win->prefs->deficons_icon_size_mode,
    ChooserEnd;
    
    win->thumbnail_borders_checkbox = (Object *)CheckBoxObject,
        GA_ID, GID_THUMBNAIL_BORDERS_CHECKBOX,
        GA_Text, "Enable _borders on image thumbnails",
        GA_Selected, win->prefs->deficons_enable_thumbnail_borders,
        GA_RelVerify, TRUE,
    CheckBoxEnd;
    
    win->text_preview_checkbox = (Object *)CheckBoxObject,
        GA_ID, GID_TEXT_PREVIEW_CHECKBOX,
        GA_Text, "Enable _text file preview thumbnails",
        GA_Selected, win->prefs->deficons_enable_text_previews,
        GA_RelVerify, TRUE,
    CheckBoxEnd;
    
    win->picture_preview_checkbox = (Object *)CheckBoxObject,
        GA_ID, GID_PICTURE_PREVIEW_CHECKBOX,
        GA_Text, "Enable _picture file preview thumbnails",
        GA_Selected, win->prefs->deficons_enable_picture_previews,
        GA_RelVerify, TRUE,
    CheckBoxEnd;

    win->upscale_thumbnails_cb = (Object *)CheckBoxObject,
        GA_ID, GID_UPSCALE_THUMBNAILS_CB,
        GA_Text, "_Upscale small images to icon size",
        GA_Selected, win->prefs->deficons_upscale_thumbnails,
        GA_RelVerify, TRUE,
    CheckBoxEnd;

    /* Per-format enable checkboxes */
    {
        ULONG fmt_bits = win->prefs->deficons_picture_formats_enabled;

        win->fmt_ilbm_cb = (Object *)CheckBoxObject,
            GA_ID, GID_FMT_ILBM_CB,
            GA_Text, "_ILBM",
            GA_Selected, (fmt_bits & ITIDY_PICTFMT_ILBM) ? TRUE : FALSE,
            GA_RelVerify, TRUE,
        CheckBoxEnd;

        win->fmt_png_cb = (Object *)CheckBoxObject,
            GA_ID, GID_FMT_PNG_CB,
            GA_Text, "_PNG",
            GA_Selected, (fmt_bits & ITIDY_PICTFMT_PNG) ? TRUE : FALSE,
            GA_RelVerify, TRUE,
        CheckBoxEnd;

        win->fmt_gif_cb = (Object *)CheckBoxObject,
            GA_ID, GID_FMT_GIF_CB,
            GA_Text, "_GIF",
            GA_Selected, (fmt_bits & ITIDY_PICTFMT_GIF) ? TRUE : FALSE,
            GA_RelVerify, TRUE,
        CheckBoxEnd;

        win->fmt_jpeg_cb = (Object *)CheckBoxObject,
            GA_ID, GID_FMT_JPEG_CB,
            GA_Text, "_JPEG (slow)",
            GA_Selected, (fmt_bits & ITIDY_PICTFMT_JPEG) ? TRUE : FALSE,
            GA_RelVerify, TRUE,
        CheckBoxEnd;

        win->fmt_bmp_cb = (Object *)CheckBoxObject,
            GA_ID, GID_FMT_BMP_CB,
            GA_Text, "_BMP",
            GA_Selected, (fmt_bits & ITIDY_PICTFMT_BMP) ? TRUE : FALSE,
            GA_RelVerify, TRUE,
        CheckBoxEnd;

        win->fmt_acbm_cb = (Object *)CheckBoxObject,
            GA_ID, GID_FMT_ACBM_CB,
            GA_Text, "_ACBM",
            GA_Selected, (fmt_bits & ITIDY_PICTFMT_ACBM) ? TRUE : FALSE,
            GA_RelVerify, TRUE,
        CheckBoxEnd;

        win->fmt_other_cb = (Object *)CheckBoxObject,
            GA_ID, GID_FMT_OTHER_CB,
            GA_Text, "_Other",
            GA_Selected, (fmt_bits & ITIDY_PICTFMT_OTHER) ? TRUE : FALSE,
            GA_RelVerify, TRUE,
        CheckBoxEnd;
    }
    
    /* Determine initial chooser indices and ghosting from preferences */
    {
        UWORD max_colors_index;
        BOOL is_ultra = win->prefs->deficons_ultra_mode;
        BOOL ghost_dither;
        BOOL ghost_lowcolor;

        if (is_ultra)
        {
            max_colors_index = ITIDY_MAX_COLORS_ULTRA_INDEX;  /* 7 = Ultra */
        }
        else
        {
            max_colors_index = itidy_max_colors_to_index(win->prefs->deficons_max_icon_colors);
        }

        /* Ghost dither when 256 colors or Ultra (no reduction needed) */
        ghost_dither = (win->prefs->deficons_max_icon_colors >= 256) || is_ultra;
        /* Ghost lowcolor when max colors > 8 */
        ghost_lowcolor = (win->prefs->deficons_max_icon_colors > 8) || is_ultra;

        win->max_colors_chooser_obj = (Object *)ChooserObject,
            GA_ID, GID_MAX_COLORS_CHOOSER,
            GA_RelVerify, TRUE,
            GA_Disabled, FALSE,
            CHOOSER_Labels, win->max_colors_list,
            CHOOSER_Selected, max_colors_index,
        ChooserEnd;

        win->dither_method_chooser_obj = (Object *)ChooserObject,
            GA_ID, GID_DITHER_METHOD_CHOOSER,
            GA_RelVerify, TRUE,
            GA_Disabled, ghost_dither,
            CHOOSER_Labels, win->dither_method_list,
            CHOOSER_Selected, win->prefs->deficons_dither_method,
        ChooserEnd;

        win->lowcolor_mapping_chooser_obj = (Object *)ChooserObject,
            GA_ID, GID_LOWCOLOR_MAPPING_CHOOSER,
            GA_RelVerify, TRUE,
            GA_Disabled, ghost_lowcolor,
            CHOOSER_Labels, win->lowcolor_mapping_list,
            CHOOSER_Selected, win->prefs->deficons_lowcolor_mapping,
        ChooserEnd;
    }
    
    win->ok_btn = (Object *)ButtonObject,
        GA_ID, GID_OK,
        GA_Text, "_OK",
        GA_RelVerify, TRUE,
    ButtonEnd;
    
    win->cancel_btn = (Object *)ButtonObject,
        GA_ID, GID_CANCEL,
        GA_Text, "_Cancel",
        GA_RelVerify, TRUE,
    ButtonEnd;
    
    /* Build layout */
    win->main_layout = (Object *)VLayoutObject,
        LAYOUT_AddChild, (Object *)VLayoutObject,
            LAYOUT_BevelStyle, BVS_GROUP,
            LAYOUT_Label, "Icon Creation Options",
            
            LAYOUT_AddChild, (Object *)HLayoutObject,
                LAYOUT_AddChild, (Object *)LabelObject,
                    LABEL_Text, "Folder Icons:",
                LabelEnd,
                CHILD_WeightedWidth, 0,
                
                LAYOUT_AddChild, win->folder_mode_chooser_obj,
                CHILD_WeightedWidth, 100,
            LayoutEnd,
            CHILD_WeightedHeight, 0,
            
            LAYOUT_AddChild, (Object *)HLayoutObject,
                LAYOUT_AddChild, (Object *)LabelObject,
                    LABEL_Text, "Icon Size:",
                LabelEnd,
                CHILD_WeightedWidth, 0,
                
                LAYOUT_AddChild, win->icon_size_chooser_obj,
                CHILD_WeightedWidth, 100,
            LayoutEnd,
            CHILD_WeightedHeight, 0,
            
            LAYOUT_AddChild, win->thumbnail_borders_checkbox,
            CHILD_WeightedHeight, 0,
            
            LAYOUT_AddChild, win->text_preview_checkbox,
            CHILD_WeightedHeight, 0,
            
            LAYOUT_AddChild, win->picture_preview_checkbox,
            CHILD_WeightedHeight, 0,
            
            LAYOUT_AddChild, win->upscale_thumbnails_cb,
            CHILD_WeightedHeight, 0,
        LayoutEnd,
        CHILD_WeightedHeight, 0,

        LAYOUT_AddChild, (Object *)VLayoutObject,
            LAYOUT_BevelStyle, BVS_GROUP,
            LAYOUT_Label, "Enabled Picture Formats",

            LAYOUT_AddChild, (Object *)HLayoutObject,
                /* Left column: ILBM, PNG, GIF, BMP */
                LAYOUT_AddChild, (Object *)VLayoutObject,
                    LAYOUT_AddChild, win->fmt_ilbm_cb,
                    CHILD_WeightedHeight, 0,
                    LAYOUT_AddChild, win->fmt_png_cb,
                    CHILD_WeightedHeight, 0,
                    LAYOUT_AddChild, win->fmt_gif_cb,
                    CHILD_WeightedHeight, 0,
                    LAYOUT_AddChild, win->fmt_bmp_cb,
                    CHILD_WeightedHeight, 0,
                LayoutEnd,
                /* Right column: JPEG (slow), ACBM, Other */
                LAYOUT_AddChild, (Object *)VLayoutObject,
                    LAYOUT_AddChild, win->fmt_jpeg_cb,
                    CHILD_WeightedHeight, 0,
                    LAYOUT_AddChild, win->fmt_acbm_cb,
                    CHILD_WeightedHeight, 0,
                    LAYOUT_AddChild, win->fmt_other_cb,
                    CHILD_WeightedHeight, 0,
                LayoutEnd,
            LayoutEnd,
            CHILD_WeightedHeight, 0,
        LayoutEnd,
        CHILD_WeightedHeight, 0,

        LAYOUT_AddChild, (Object *)VLayoutObject,
            LAYOUT_BevelStyle, BVS_GROUP,
            LAYOUT_Label, "Color Reduction",

            LAYOUT_AddChild, (Object *)HLayoutObject,
                LAYOUT_AddChild, (Object *)LabelObject,
                    LABEL_Text, "Max Colors:",
                LabelEnd,
                CHILD_WeightedWidth, 0,

                LAYOUT_AddChild, win->max_colors_chooser_obj,
                CHILD_WeightedWidth, 100,
            LayoutEnd,
            CHILD_WeightedHeight, 0,

            LAYOUT_AddChild, (Object *)HLayoutObject,
                LAYOUT_AddChild, (Object *)LabelObject,
                    LABEL_Text, "Dithering:",
                LabelEnd,
                CHILD_WeightedWidth, 0,

                LAYOUT_AddChild, win->dither_method_chooser_obj,
                CHILD_WeightedWidth, 100,
            LayoutEnd,
            CHILD_WeightedHeight, 0,

            LAYOUT_AddChild, (Object *)HLayoutObject,
                LAYOUT_AddChild, (Object *)LabelObject,
                    LABEL_Text, "Low-Color Map:",
                LabelEnd,
                CHILD_WeightedWidth, 0,

                LAYOUT_AddChild, win->lowcolor_mapping_chooser_obj,
                CHILD_WeightedWidth, 100,
            LayoutEnd,
            CHILD_WeightedHeight, 0,
        LayoutEnd,
        CHILD_WeightedHeight, 0,

        LAYOUT_AddChild, (Object *)HLayoutObject,
            LAYOUT_AddChild, win->ok_btn,
            LAYOUT_AddChild, win->cancel_btn,
        LayoutEnd,
        CHILD_WeightedHeight, 0,
    LayoutEnd;

    /* Create window object */
    win->window_obj = (Object *)WindowObject,
        WA_Title, "DefIcons: Icon Creation Settings",
        WA_Activate, TRUE,
        WA_DepthGadget, TRUE,
        WA_DragBar, TRUE,
        WA_CloseGadget, TRUE,
        WA_SizeGadget, FALSE,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_Layout, win->main_layout,
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
    
    /* Free chooser lists */
    free_chooser_list(win->folder_mode_list);
    free_chooser_list(win->icon_size_list);
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
