/*
 * advanced_window.c - iTidy Advanced Settings Window Implementation
 * GadTools-based GUI for Workbench 3.0+
 * Aspect Ratio and Window Overflow Configuration
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <string.h>
#include <stdio.h>

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

#include "advanced_window.h"
#include "beta_options_window.h"
#include "layout_preferences.h"
#include "writeLog.h"
#include "../Settings/IControlPrefs.h"
#include "../Settings/WorkbenchPrefs.h"

/* External global for Workbench settings */
extern struct WorkbenchSettings prefsWorkbench;

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define ADV_WINDOW_TITLE "iTidy - Advanced Settings"
#define ADV_WINDOW_WIDTH 530
#define ADV_WINDOW_HEIGHT 600
#define ADV_WINDOW_LEFT 100
#define ADV_WINDOW_TOP 50

#define ADV_WINDOW_STANDARD_PADDING 15
#define ADV_WINDOW_Column_1_LEFT 140
#define ADV_WINDOW_Column_2_LEFT 400
#define ADV_WINDOW_TOP_START 10
#define ADV_WINDOW_GAP_BETWEEN_GADGETS_VERTICAL 10
#define ADV_WINDOW_GAP_BETWEEN_CHECKBOXES_VERTICAL 10

/*------------------------------------------------------------------------*/
/* Cycle Gadget Labels                                                   */
/*------------------------------------------------------------------------*/
static STRPTR aspect_ratio_labels[] = {
    "Tall (0.75)",
    "Square (1.0)",
    "Compact (1.3)",
    "Classic (1.6)",
    "Wide (2.0)",
    "Ultrawide (2.4)",
    NULL
};

static STRPTR overflow_mode_labels[] = {
    "Expand Horizontally",
    "Expand Vertically",
    "Expand Both",
    NULL
};

static STRPTR max_width_pct_labels[] = {
    "Auto",
    "30%",
    "50%",
    "70%",
    "90%",
    "100%",
    NULL
};

static STRPTR vertical_align_labels[] = {
    "Top",
    "Middle",
    "Bottom",
    NULL
};

/*------------------------------------------------------------------------*/
/* Aspect Ratio Preset Values (Fixed-Point: Scaled by 1000)             */
/*------------------------------------------------------------------------*/
static const int aspect_ratio_presets[] = {
    750,   /* Tall (0.75 * 1000) */
    1000,  /* Square (1.0 * 1000) */
    1300,  /* Compact (1.3 * 1000) */
    1600,  /* Classic (1.6 * 1000) */
    2000,  /* Wide (2.0 * 1000) */
    2400   /* Ultrawide (2.4 * 1000) */
};

/*------------------------------------------------------------------------*/
/* Helper Functions                                                       */
/*------------------------------------------------------------------------*/

/**
 * @brief Determine which preset matches the current aspect ratio
 *
 * @param prefs Pointer to LayoutPreferences
 * @return WORD Preset index (0-6), or ASPECT_PRESET_CUSTOM if custom
 */
static WORD get_aspect_ratio_preset_index(const LayoutPreferences *prefs)
{
    int i;
    
    /* Find matching preset (allow small differences due to rounding) */
    for (i = 0; i < 6; i++)
    {
        int diff = prefs->aspectRatio - aspect_ratio_presets[i];
        if (diff < 0) diff = -diff;  /* abs() */
        
        if (diff < 10)  /* Within tolerance (0.01 * 1000) */
        {
            return i;
        }
    }
    
    /* Default to Wide if no match */
    return ASPECT_PRESET_WIDE;
}

/**
 * @brief Get max window width percentage index from actual percentage value
 *
 * @param pct Percentage value (0-100)
 * @return WORD Index (0=Auto, 1=30%, 2=50%, 3=70%, 4=90%, 5=100%)
 */
static WORD get_max_width_pct_index(UWORD pct)
{
    /* Map common preset values to indices */
    switch (pct)
    {
        case 30: return MAX_WIDTH_30;
        case 50: return MAX_WIDTH_50;
        case 70: return MAX_WIDTH_70;
        case 90: return MAX_WIDTH_90;
        case 100: return MAX_WIDTH_100;
        default: return MAX_WIDTH_AUTO;  /* Anything else defaults to Auto */
    }
}

/**
 * @brief Get actual percentage value from max window width index
 *
 * @param index Index (0-5)
 * @param prefs Current preferences (for Auto default)
 * @return UWORD Percentage value (0-100), 0 means use preset default
 */
static UWORD get_max_width_pct_value(WORD index, const LayoutPreferences *prefs)
{
    switch (index)
    {
        case MAX_WIDTH_30: return 30;
        case MAX_WIDTH_50: return 50;
        case MAX_WIDTH_70: return 70;
        case MAX_WIDTH_90: return 90;
        case MAX_WIDTH_100: return 100;
        case MAX_WIDTH_AUTO:
        default:
            return 0;  /* 0 = Auto, means use preset default */
    }
}

/**
 * @brief Create all gadgets for the advanced settings window
 *
 * @param adv_data Pointer to advanced window data structure
 * @return struct Gadget* Pointer to gadget list, or NULL on failure
 */
static struct Gadget *create_advanced_gadgets(struct iTidyAdvancedWindow *adv_data,
                                              UWORD *calculated_height)
{
    struct Gadget *gad = NULL;
    struct NewGadget ng;
    UWORD current_y;
    UWORD label_width = 140;
    UWORD gadget_x = label_width + 10;
    UWORD gadget_width = 120;
    UWORD small_gadget_width = 50;
    
    /* Get screen's default font height for gadget sizing */
    struct TextAttr *font = adv_data->screen->Font;
    UWORD font_height = font->ta_YSize;
    UWORD button_height = font_height + 6;
    UWORD string_height = font_height + 4;
    UWORD slider_height = font_height + 6;
    
    /* Create a context gadget (required by GadTools) */
    gad = CreateContext(&adv_data->glist);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create gadget context\n");
        return NULL;
    }
    
    current_y = ADV_WINDOW_TOP_START + prefsIControl.currentTitleBarHeight;
    
    /*--------------------------------------------------------------------*/
    /* Aspect Ratio Cycle Gadget                                         */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_Column_1_LEFT;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = gadget_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Layout Aspect Ratio:";
    ng.ng_TextAttr = font;
    ng.ng_GadgetID = GID_ADV_ASPECT_RATIO;
    ng.ng_Flags = PLACETEXT_LEFT;
    ng.ng_VisualInfo = adv_data->visual_info;
    
    adv_data->aspect_ratio_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, aspect_ratio_labels,
        GTCY_Active, adv_data->aspect_preset_selected,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create aspect ratio cycle gadget\n");
        return NULL;
    }
    
    

    /*--------------------------------------------------------------------*/
    /* Window Overflow Mode Cycle Gadget                                 */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = 370;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 135;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Overflow Strategy:";
    ng.ng_GadgetID = GID_ADV_OVERFLOW_MODE;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->overflow_mode_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, overflow_mode_labels,
        GTCY_Active, adv_data->overflow_mode_selected,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create overflow mode cycle gadget\n");
        return NULL;
    }
    
    current_y += button_height + 12;
    
    /*--------------------------------------------------------------------*/
    /* Horizontal Spacing Slider                                         */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_Column_1_LEFT;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 50;
    ng.ng_Height = slider_height;
    ng.ng_GadgetText = "Icon Spacing:  X:";
    ng.ng_GadgetID = GID_ADV_SPACING_X;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->spacing_x_slider = gad = CreateGadget(SLIDER_KIND, gad, &ng,
        GTSL_Min, 0,
        GTSL_Max, 20,
        GTSL_Level, adv_data->spacing_x_value,
        GTSL_MaxLevelLen, 3,
        GTSL_LevelFormat, "%2ld",
        GTSL_LevelPlace, PLACETEXT_RIGHT,
        GA_RelVerify, TRUE,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create horizontal spacing slider\n");
        return NULL;
    }
    

    
    /*--------------------------------------------------------------------*/
    /* Vertical Spacing Slider                                           */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_Column_1_LEFT + ADV_WINDOW_STANDARD_PADDING +50+ 38; // extra 38 for the previous slider output number
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 50;
    ng.ng_Height = slider_height;
    ng.ng_GadgetText = "Y:";
    ng.ng_GadgetID = GID_ADV_SPACING_Y;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->spacing_y_slider = gad = CreateGadget(SLIDER_KIND, gad, &ng,
        GTSL_Min, 0,
        GTSL_Max, 20,
        GTSL_Level, adv_data->spacing_y_value,
        GTSL_MaxLevelLen, 3,
        GTSL_LevelFormat, "%2ld",
        GTSL_LevelPlace, PLACETEXT_RIGHT,
        GA_RelVerify, TRUE,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create vertical spacing slider\n");
        return NULL;
    }
    
    current_y += slider_height  + ADV_WINDOW_GAP_BETWEEN_GADGETS_VERTICAL;
    

    /*--------------------------------------------------------------------*/
    /* Min Icons Per Row Integer Gadget                                  */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_Column_1_LEFT;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 30;
    ng.ng_Height = string_height;
    ng.ng_GadgetText = "Icons per Row:  Min:";
    ng.ng_GadgetID = GID_ADV_MIN_ICONS_ROW;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->min_icons_row_int = gad = CreateGadget(INTEGER_KIND, gad, &ng,
        GTIN_Number, adv_data->min_icons_per_row,
        GTIN_MaxChars, 2,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create min icons/row integer gadget\n");
        return NULL;
    }
    
    //current_y += string_height + 4;
    
    /*--------------------------------------------------------------------*/
    /* Auto Max Icons/Row Checkbox                                       */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = 300;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 30;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Auto Max Icons:";
    ng.ng_GadgetID = GID_ADV_MAX_AUTO_CHECKBOX;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->max_auto_checkbox = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, adv_data->max_auto_enabled,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create auto max icons checkbox\n");
        return NULL;
    }
    
    //current_y += button_height + 4;
    
    /*--------------------------------------------------------------------*/
    /* Max Icons Per Row Integer Gadget                                  */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = 385;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 30;
    ng.ng_Height = string_height;
    ng.ng_GadgetText = "Max:";
    ng.ng_GadgetID = GID_ADV_MAX_ICONS_ROW;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->max_icons_row_int = gad = CreateGadget(INTEGER_KIND, gad, &ng,
        GTIN_Number, adv_data->max_icons_per_row > 0 ? adv_data->max_icons_per_row : 5,
        GTIN_MaxChars, 2,
        GA_Disabled, adv_data->max_auto_enabled,  /* Disabled if Auto checked */
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create max icons/row integer gadget\n");
        return NULL;
    }
    
    current_y += string_height + ADV_WINDOW_GAP_BETWEEN_GADGETS_VERTICAL;
    
    /*--------------------------------------------------------------------*/
    /* Max Window Width Percentage Cycle Gadget                          */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_Column_1_LEFT;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = gadget_width - 40;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Max Window Width:";
    ng.ng_GadgetID = GID_ADV_MAX_WIDTH_PCT;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->max_width_pct_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, max_width_pct_labels,
        GTCY_Active, adv_data->max_width_pct_selected,
        GA_Disabled, !adv_data->max_auto_enabled,  /* Disabled if manual mode */
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create max window width percentage cycle gadget\n");
        return NULL;
    }
    
    current_y += button_height + ADV_WINDOW_GAP_BETWEEN_GADGETS_VERTICAL;
    
    /*--------------------------------------------------------------------*/
    /* Vertical Alignment Cycle Gadget                                   */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_Column_1_LEFT;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = gadget_width - 40;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Align Icons Vertically:";
    ng.ng_GadgetID = GID_ADV_VERTICAL_ALIGN;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->vertical_align_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, vertical_align_labels,
        GTCY_Active, adv_data->vertical_align_selected,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create vertical alignment cycle gadget\n");
        return NULL;
    }
    
    current_y += ADV_WINDOW_GAP_BETWEEN_CHECKBOXES_VERTICAL + ADV_WINDOW_GAP_BETWEEN_GADGETS_VERTICAL;
    
    /*--------------------------------------------------------------------*/
    /* Reverse Sort Checkbox                                             */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_Column_1_LEFT;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = button_height - 4;
    ng.ng_GadgetText = "Reverse Sort (Z->A)";
    ng.ng_GadgetID = GID_ADV_REVERSE_SORT;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    adv_data->reverse_sort_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, adv_data->reverse_sort_enabled,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create reverse sort checkbox\n");
        return NULL;
    }
    
    current_y += ADV_WINDOW_GAP_BETWEEN_CHECKBOXES_VERTICAL + ADV_WINDOW_GAP_BETWEEN_GADGETS_VERTICAL;
    
    /*--------------------------------------------------------------------*/
    /* Optimize Column Widths Checkbox                                   */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_Column_1_LEFT;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = button_height - 4;
    ng.ng_GadgetText = "Optimize Column Widths";
    ng.ng_GadgetID = GID_ADV_OPTIMIZE_COLS;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    adv_data->optimize_cols_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, adv_data->optimize_cols_enabled,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create optimize columns checkbox\n");
        return NULL;
    }
        current_y += ADV_WINDOW_GAP_BETWEEN_CHECKBOXES_VERTICAL + ADV_WINDOW_GAP_BETWEEN_GADGETS_VERTICAL;
    
    /*--------------------------------------------------------------------*/
    /* Column Layout Checkbox                                            */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_Column_1_LEFT;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = button_height - 4;
    ng.ng_GadgetText = "Column Layout (centered columns) ";
    ng.ng_GadgetID = GID_ADV_COLUMN_LAYOUT;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    adv_data->column_layout_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, adv_data->column_layout_enabled,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create column layout checkbox\n");
        return NULL;
    }
    current_y += ADV_WINDOW_GAP_BETWEEN_CHECKBOXES_VERTICAL + ADV_WINDOW_GAP_BETWEEN_GADGETS_VERTICAL;
    
    /*--------------------------------------------------------------------*/
    /* Skip Hidden Folders Checkbox                                      */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_Column_1_LEFT;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = button_height - 4;
    ng.ng_GadgetText = "Skip Hidden Folders";
    ng.ng_GadgetID = GID_ADV_SKIP_HIDDEN;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    adv_data->skip_hidden_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, adv_data->skip_hidden_enabled,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create skip hidden folders checkbox\n");
        return NULL;
    }
    current_y += ADV_WINDOW_GAP_BETWEEN_CHECKBOXES_VERTICAL + ADV_WINDOW_GAP_BETWEEN_GADGETS_VERTICAL;
    
    /*--------------------------------------------------------------------*/
    /* Strip NewIcon Borders Checkbox                                    */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_Column_1_LEFT;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = button_height - 4;
    ng.ng_GadgetText = "Strip NewIcon Borders";
    ng.ng_GadgetID = GID_ADV_STRIP_NEWICON_BORDERS;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    /* Only enable if icon.library v44+ is available */
    if (prefsWorkbench.iconLibraryVersion >= 44)
    {
        adv_data->strip_newicon_borders_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
            GTCB_Checked, adv_data->strip_newicon_borders_enabled,
            TAG_END);
    }
    else
    {
        /* Disabled checkbox for icon.library < v44 */
        adv_data->strip_newicon_borders_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
            GTCB_Checked, FALSE,
            GA_Disabled, TRUE,
            TAG_END);
    }
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create strip NewIcon borders checkbox\n");
        return NULL;
    }
    

    

    
    /*--------------------------------------------------------------------*/
    /* Beta Options Button                                               */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = 5;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 120;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Beta Options...";
    ng.ng_GadgetID = GID_ADV_BETA_OPTIONS;
    ng.ng_Flags = PLACETEXT_IN;
    
    adv_data->beta_options_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create Beta Options button\n");
        return NULL;
    }
    
    
    /*--------------------------------------------------------------------*/
    /* OK and Cancel Buttons                                             */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = 330;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 80;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "OK";
    ng.ng_GadgetID = GID_ADV_OK;
    ng.ng_Flags = PLACETEXT_IN;
    
    adv_data->ok_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create OK button\n");
        return NULL;
    }
    
    ng.ng_LeftEdge = 330+80+ADV_WINDOW_STANDARD_PADDING;
    ng.ng_GadgetText = "Cancel";
    ng.ng_GadgetID = GID_ADV_CANCEL;
    
    adv_data->cancel_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create Cancel button\n");
        return NULL;
    }
    
    current_y += button_height  + 5+ADV_WINDOW_GAP_BETWEEN_GADGETS_VERTICAL;

    if (calculated_height)
    {
        *calculated_height = current_y ;
    }

    return adv_data->glist;
}

/*------------------------------------------------------------------------*/
/* Public Functions                                                       */
/*------------------------------------------------------------------------*/

BOOL open_itidy_advanced_window(struct iTidyAdvancedWindow *adv_data, 
                                 LayoutPreferences *prefs)
{
    struct Gadget *glist;
    UWORD calculated_window_height = ADV_WINDOW_HEIGHT;
    
    if (!adv_data || !prefs)
    {
        CONSOLE_ERROR("NULL pointer passed to open_itidy_advanced_window\n");
        return FALSE;
    }
    
    /* Initialize structure */
    memset(adv_data, 0, sizeof(struct iTidyAdvancedWindow));
    adv_data->prefs = prefs;
    adv_data->changes_accepted = FALSE;
    
    /* Load current preferences into local variables */
    adv_data->aspect_preset_selected = get_aspect_ratio_preset_index(prefs);
    adv_data->custom_aspect_width = prefs->customAspectWidth;
    adv_data->custom_aspect_height = prefs->customAspectHeight;
    adv_data->overflow_mode_selected = prefs->overflowMode;
    adv_data->spacing_x_value = prefs->iconSpacingX;
    adv_data->spacing_y_value = prefs->iconSpacingY;
    adv_data->min_icons_per_row = prefs->minIconsPerRow;
    adv_data->max_icons_per_row = prefs->maxIconsPerRow;
    adv_data->max_auto_enabled = (prefs->maxIconsPerRow == 0);  /* Auto if 0 */
    adv_data->max_width_pct_selected = get_max_width_pct_index(prefs->maxWindowWidthPct);
    adv_data->vertical_align_selected = prefs->textAlignment;  /* 0=Top, 1=Middle, 2=Bottom */
    adv_data->reverse_sort_enabled = prefs->reverseSort;        /* Load reverse sort setting */
    adv_data->optimize_cols_enabled = prefs->useColumnWidthOptimization;  /* Load optimize columns setting */
    adv_data->skip_hidden_enabled = prefs->skipHiddenFolders;  /* Load skip hidden folders setting */
    adv_data->column_layout_enabled = prefs->centerIconsInColumn;  /* Load column layout setting */
    adv_data->strip_newicon_borders_enabled = prefs->stripNewIconBorders;  /* Load strip NewIcon borders setting */
    
    log_debug(LOG_GUI, "Loading prefs into adv_data on window open:\n");
    log_debug(LOG_GUI, "  prefs->iconSpacingX = %hu\n", prefs->iconSpacingX);
    log_debug(LOG_GUI, "  prefs->iconSpacingY = %hu\n", prefs->iconSpacingY);
    log_debug(LOG_GUI, "  prefs->maxWindowWidthPct = %ld\n", (long)prefs->maxWindowWidthPct);
    log_debug(LOG_GUI, "  prefs->textAlignment = %ld\n", (long)prefs->textAlignment);
    log_debug(LOG_GUI, "  adv_data->max_width_pct_selected = %ld\n", (long)adv_data->max_width_pct_selected);
    log_debug(LOG_GUI, "  adv_data->vertical_align_selected = %ld\n", (long)adv_data->vertical_align_selected);
    log_debug(LOG_GUI, "  prefs->overflowMode = %ld\n", (long)prefs->overflowMode);
    log_debug(LOG_GUI, "  prefs->aspectRatio = %d\n", prefs->aspectRatio);
    log_debug(LOG_GUI, "  prefs->reverseSort = %s\n", prefs->reverseSort ? "YES" : "NO");
    log_debug(LOG_GUI, "  prefs->stripNewIconBorders = %s\n", prefs->stripNewIconBorders ? "YES" : "NO");
    log_debug(LOG_GUI, "  adv_data->strip_newicon_borders_enabled = %s\n", adv_data->strip_newicon_borders_enabled ? "YES" : "NO");
    
    /* Get Workbench screen */
    adv_data->screen = LockPubScreen(NULL);
    if (!adv_data->screen)
    {
        CONSOLE_ERROR("Failed to lock Workbench screen\n");
        return FALSE;
    }
    
    /* Get visual info for GadTools */
    adv_data->visual_info = GetVisualInfo(adv_data->screen, TAG_END);
    if (!adv_data->visual_info)
    {
        CONSOLE_ERROR("Failed to get visual info\n");
        UnlockPubScreen(NULL, adv_data->screen);
        return FALSE;
    }
    
    /* Create gadgets */
    glist = create_advanced_gadgets(adv_data, &calculated_window_height);
    if (!glist)
    {
        CONSOLE_ERROR("Failed to create gadgets\n");
        FreeVisualInfo(adv_data->visual_info);
        UnlockPubScreen(NULL, adv_data->screen);
        return FALSE;
    }
    
    /* Open the window */
    adv_data->window = OpenWindowTags(NULL,
        WA_Title, ADV_WINDOW_TITLE,
        WA_Left, ADV_WINDOW_LEFT,
        WA_Top, ADV_WINDOW_TOP,
        WA_Width, ADV_WINDOW_WIDTH,
        WA_Height, calculated_window_height,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, adv_data->screen,
        WA_Gadgets, glist,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW,
        TAG_END);
    
    if (!adv_data->window)
    {
        CONSOLE_ERROR("Failed to open advanced settings window\n");
        FreeGadgets(adv_data->glist);
        FreeVisualInfo(adv_data->visual_info);
        UnlockPubScreen(NULL, adv_data->screen);
        return FALSE;
    }
    
    adv_data->window_open = TRUE;
    
    /* Load current preferences into gadgets (in case they were changed since last open) */
    load_preferences_to_advanced_window(adv_data);
    
    /* Refresh gadgets */
    GT_RefreshWindow(adv_data->window, NULL);
    
    CONSOLE_STATUS("Advanced Settings window opened successfully\n");
    return TRUE;
}

void close_itidy_advanced_window(struct iTidyAdvancedWindow *adv_data)
{
    if (!adv_data)
    {
        return;
    }
    
    /* Close window */
    if (adv_data->window)
    {
        CloseWindow(adv_data->window);
        adv_data->window = NULL;
    }
    
    /* Free gadgets */
    if (adv_data->glist)
    {
        FreeGadgets(adv_data->glist);
        adv_data->glist = NULL;
    }
    
    /* Free visual info */
    if (adv_data->visual_info)
    {
        FreeVisualInfo(adv_data->visual_info);
        adv_data->visual_info = NULL;
    }
    
    /* Unlock screen */
    if (adv_data->screen)
    {
        UnlockPubScreen(NULL, adv_data->screen);
        adv_data->screen = NULL;
    }
    
    adv_data->window_open = FALSE;
    
    CONSOLE_STATUS("Advanced Settings window closed\n");
}

void set_max_icons_gadget_state(struct iTidyAdvancedWindow *adv_data,
                                BOOL auto_enabled)
{
    if (!adv_data || !adv_data->window)
    {
        return;
    }
    
    /* Disable max icons/row gadget when Auto is enabled */
    if (adv_data->max_icons_row_int)
    {
        GT_SetGadgetAttrs(adv_data->max_icons_row_int, adv_data->window, NULL,
            GA_Disabled, auto_enabled,
            TAG_END);
    }
}

void update_max_width_pct_gadget_state(struct iTidyAdvancedWindow *adv_data)
{
    BOOL manual_mode;
    
    if (!adv_data || !adv_data->window)
    {
        return;
    }
    
    /* Disable max width pct gadget when in manual mode (not Auto) */
    manual_mode = !adv_data->max_auto_enabled;
    
    if (adv_data->max_width_pct_cycle)
    {
        GT_SetGadgetAttrs(adv_data->max_width_pct_cycle, adv_data->window, NULL,
            GA_Disabled, manual_mode,
            TAG_END);
    }
}

void save_advanced_window_to_preferences(struct iTidyAdvancedWindow *adv_data)
{
    if (!adv_data || !adv_data->prefs)
    {
        CONSOLE_ERROR("NULL pointer in save_advanced_window_to_preferences\n");
        return;
    }
    
    /* Save aspect ratio settings (always use preset) */
    adv_data->prefs->useCustomAspectRatio = FALSE;
    adv_data->prefs->aspectRatio = 
        aspect_ratio_presets[adv_data->aspect_preset_selected];
    
    /* Save overflow mode */
    adv_data->prefs->overflowMode = adv_data->overflow_mode_selected;
    
    /* Save spacing values */
    adv_data->prefs->iconSpacingX = adv_data->spacing_x_value;
    adv_data->prefs->iconSpacingY = adv_data->spacing_y_value;
    
    log_debug(LOG_GUI, "  Saved prefs->iconSpacingX = %hu\n", adv_data->prefs->iconSpacingX);
    log_debug(LOG_GUI, "  Saved prefs->iconSpacingY = %hu\n", adv_data->prefs->iconSpacingY);
    
    /* Save column limits */
    adv_data->prefs->minIconsPerRow = adv_data->min_icons_per_row;
    
    /* Save max icons per row: 0 if Auto mode, otherwise the manual value */
    if (adv_data->max_auto_enabled)
    {
        adv_data->prefs->maxIconsPerRow = 0;  /* Auto mode */
    }
    else
    {
        adv_data->prefs->maxIconsPerRow = adv_data->max_icons_per_row;
    }
    
    /* Save max window width percentage */
    adv_data->prefs->maxWindowWidthPct = get_max_width_pct_value(
        adv_data->max_width_pct_selected, adv_data->prefs);
    
    /* Save vertical alignment */
    adv_data->prefs->textAlignment = adv_data->vertical_align_selected;
    
    /* Save reverse sort setting */
    adv_data->prefs->reverseSort = adv_data->reverse_sort_enabled;
    
    /* Save optimize columns setting */
    adv_data->prefs->useColumnWidthOptimization = adv_data->optimize_cols_enabled;
    
    /* Save skip hidden folders setting */
    adv_data->prefs->skipHiddenFolders = adv_data->skip_hidden_enabled;
    
    /* Save column layout setting */
    adv_data->prefs->centerIconsInColumn = adv_data->column_layout_enabled;
    
    /* Save strip NewIcon borders setting */
    adv_data->prefs->stripNewIconBorders = adv_data->strip_newicon_borders_enabled;
    
    log_debug(LOG_GUI, "save_advanced_window_to_preferences() called:\n");
    log_debug(LOG_GUI, "  adv_data->max_width_pct_selected = %ld\n", (long)adv_data->max_width_pct_selected);
    log_debug(LOG_GUI, "  adv_data->vertical_align_selected = %ld\n", (long)adv_data->vertical_align_selected);
    log_debug(LOG_GUI, "  adv_data->overflow_mode_selected = %ld\n", (long)adv_data->overflow_mode_selected);
    log_debug(LOG_GUI, "  adv_data->aspect_preset_selected = %ld\n", (long)adv_data->aspect_preset_selected);
    log_debug(LOG_GUI, "  adv_data->reverse_sort_enabled = %s\n", adv_data->reverse_sort_enabled ? "YES" : "NO");
    log_debug(LOG_GUI, "  Saved prefs->maxWindowWidthPct = %ld\n", (long)adv_data->prefs->maxWindowWidthPct);
    log_debug(LOG_GUI, "  Saved prefs->textAlignment = %ld\n", (long)adv_data->prefs->textAlignment);
    log_debug(LOG_GUI, "  Saved prefs->overflowMode = %ld\n", (long)adv_data->prefs->overflowMode);
    log_debug(LOG_GUI, "  Saved prefs->reverseSort = %s\n", adv_data->prefs->reverseSort ? "YES" : "NO");
    
    CONSOLE_DEBUG("Advanced settings saved to preferences:\n");
    CONSOLE_DEBUG("  Aspect Ratio: %.2f (Custom: %s)\n", 
           adv_data->prefs->aspectRatio,
           adv_data->prefs->useCustomAspectRatio ? "YES" : "NO");
    CONSOLE_DEBUG("  Overflow Mode: %d\n", adv_data->prefs->overflowMode);
    CONSOLE_DEBUG("  Spacing: %hu x %hu\n", 
           adv_data->prefs->iconSpacingX, 
           adv_data->prefs->iconSpacingY);
    CONSOLE_DEBUG("  Min Icons/Row: %hu\n", adv_data->prefs->minIconsPerRow);
    CONSOLE_DEBUG("  Max Icons/Row: %hu (%s)\n", 
           adv_data->prefs->maxIconsPerRow,
           adv_data->prefs->maxIconsPerRow == 0 ? "AUTO" : "MANUAL");
    CONSOLE_DEBUG("  Max Window Width: %hu%% (%s)\n",
           adv_data->prefs->maxWindowWidthPct,
           adv_data->prefs->maxWindowWidthPct == 0 ? "AUTO" : "MANUAL");
    CONSOLE_DEBUG("  Vertical Alignment: %s\n",
           vertical_align_labels[adv_data->prefs->textAlignment]);
    CONSOLE_DEBUG("  Reverse Sort: %s\n",
           adv_data->prefs->reverseSort ? "YES" : "NO");
}

BOOL handle_advanced_window_events(struct iTidyAdvancedWindow *adv_data)
{
    struct IntuiMessage *msg;
    ULONG msg_class;
    UWORD msg_code;
    struct Gadget *gad;
    BOOL continue_running = TRUE;
    
    if (!adv_data || !adv_data->window)
    {
        return FALSE;
    }
    
    /* Wait for messages */
    while ((msg = GT_GetIMsg(adv_data->window->UserPort)))
    {
        msg_class = msg->Class;
        msg_code = msg->Code;
        gad = (struct Gadget *)msg->IAddress;
        
        /* Reply to message */
        GT_ReplyIMsg(msg);
        
        /* Handle message */
        switch (msg_class)
        {
            case IDCMP_CLOSEWINDOW:
                CONSOLE_DEBUG("Advanced window close requested\n");
                adv_data->changes_accepted = FALSE;
                continue_running = FALSE;
                break;
                
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(adv_data->window);
                GT_EndRefresh(adv_data->window, TRUE);
                break;
                
            case IDCMP_GADGETUP:
                switch (gad->GadgetID)
                {
                    case GID_ADV_OK:
                        CONSOLE_DEBUG("OK button clicked\n");
                        
                        /* Read all gadget values into local variables */
                        {
                            LONG temp_number;
                            
                            /* Min icons per row */
                            GT_GetGadgetAttrs(adv_data->min_icons_row_int, 
                                            adv_data->window, NULL,
                                            GTIN_Number, &temp_number, TAG_END);
                            adv_data->min_icons_per_row = (UWORD)temp_number;
                            
                            /* Max icons per row */
                            GT_GetGadgetAttrs(adv_data->max_icons_row_int, 
                                            adv_data->window, NULL,
                                            GTIN_Number, &temp_number, TAG_END);
                            adv_data->max_icons_per_row = (UWORD)temp_number;
                            
                            /* Auto checkbox */
                            GT_GetGadgetAttrs(adv_data->max_auto_checkbox,
                                            adv_data->window, NULL,
                                            GTCB_Checked, &temp_number, TAG_END);
                            adv_data->max_auto_enabled = (BOOL)temp_number;
                            
                            /* Spacing X */
                            GT_GetGadgetAttrs(adv_data->spacing_x_slider, 
                                            adv_data->window, NULL,
                                            GTSL_Level, &temp_number, TAG_END);
                            adv_data->spacing_x_value = (UWORD)temp_number;
                            
                            /* Spacing Y */
                            GT_GetGadgetAttrs(adv_data->spacing_y_slider, 
                                            adv_data->window, NULL,
                                            GTSL_Level, &temp_number, TAG_END);
                            adv_data->spacing_y_value = (UWORD)temp_number;
                            
                            log_debug(LOG_GUI, "Read gadget values on OK click:\n");
                            log_debug(LOG_GUI, "  spacing_x_value = %hu\n", adv_data->spacing_x_value);
                            log_debug(LOG_GUI, "  spacing_y_value = %hu\n", adv_data->spacing_y_value);
                            log_debug(LOG_GUI, "  max_width_pct_selected = %ld\n", (long)adv_data->max_width_pct_selected);
                            GT_GetGadgetAttrs(adv_data->max_width_pct_cycle,
                                            adv_data->window, NULL,
                                            GTCY_Active, &temp_number, TAG_END);
                            adv_data->max_width_pct_selected = (WORD)temp_number;
                            
                            /* Aspect ratio cycle gadget */
                            GT_GetGadgetAttrs(adv_data->aspect_ratio_cycle,
                                            adv_data->window, NULL,
                                            GTCY_Active, &temp_number, TAG_END);
                            adv_data->aspect_preset_selected = (WORD)temp_number;
                            
                            /* Overflow mode cycle gadget */
                            GT_GetGadgetAttrs(adv_data->overflow_mode_cycle,
                                            adv_data->window, NULL,
                                            GTCY_Active, &temp_number, TAG_END);
                            adv_data->overflow_mode_selected = (WORD)temp_number;
                            
                            /* Vertical alignment cycle gadget */
                            GT_GetGadgetAttrs(adv_data->vertical_align_cycle,
                                            adv_data->window, NULL,
                                            GTCY_Active, &temp_number, TAG_END);
                            adv_data->vertical_align_selected = (WORD)temp_number;
                            
                            log_debug(LOG_GUI, "Read gadget values on OK click:\n");
                            log_debug(LOG_GUI, "  max_width_pct_selected = %ld\n", (long)adv_data->max_width_pct_selected);
                            log_debug(LOG_GUI, "  vertical_align_selected = %ld\n", (long)adv_data->vertical_align_selected);
                            log_debug(LOG_GUI, "  overflow_mode_selected = %ld\n", (long)adv_data->overflow_mode_selected);
                            log_debug(LOG_GUI, "  aspect_preset_selected = %ld\n", (long)adv_data->aspect_preset_selected);
                        }
                        
                        /* Save to preferences */
                        save_advanced_window_to_preferences(adv_data);
                        adv_data->changes_accepted = TRUE;
                        continue_running = FALSE;
                        break;
                        
                    case GID_ADV_CANCEL:
                        CONSOLE_DEBUG("Cancel button clicked\n");
                        adv_data->changes_accepted = FALSE;
                        continue_running = FALSE;
                        break;
                        
                    case GID_ADV_ASPECT_RATIO:
                        /* CRITICAL: Use msg_code for cycle gadgets (not GT_GetGadgetAttrs) */
                        adv_data->aspect_preset_selected = msg_code;
                        CONSOLE_DEBUG("Aspect ratio changed to: %s\n", 
                               aspect_ratio_labels[adv_data->aspect_preset_selected]);
                        break;
                        
                    case GID_ADV_OVERFLOW_MODE:
                        /* CRITICAL: Use msg_code for cycle gadgets (not GT_GetGadgetAttrs) */
                        adv_data->overflow_mode_selected = msg_code;
                        CONSOLE_DEBUG("Overflow mode changed to: %s\n", 
                               overflow_mode_labels[adv_data->overflow_mode_selected]);
                        break;
                        
                    case GID_ADV_MAX_AUTO_CHECKBOX:
                        /* Auto checkbox toggled */
                        {
                            LONG checked;
                            GT_GetGadgetAttrs(adv_data->max_auto_checkbox,
                                            adv_data->window, NULL,
                                            GTCB_Checked, &checked, TAG_END);
                            adv_data->max_auto_enabled = (BOOL)checked;
                            
                            CONSOLE_DEBUG("Auto Max Icons/Row: %s\n",
                                   adv_data->max_auto_enabled ? "ENABLED" : "DISABLED");
                            
                            /* Enable/disable the max icons/row integer gadget */
                            set_max_icons_gadget_state(adv_data, adv_data->max_auto_enabled);
                            
                            /* Enable/disable the max window width pct gadget */
                            update_max_width_pct_gadget_state(adv_data);
                        }
                        break;
                        
                    case GID_ADV_MAX_WIDTH_PCT:
                        /* Max window width percentage changed */
                        adv_data->max_width_pct_selected = msg_code;
                        CONSOLE_DEBUG("Max window width changed to: %s\n", 
                               max_width_pct_labels[adv_data->max_width_pct_selected]);
                        break;
                    
                    case GID_ADV_VERTICAL_ALIGN:
                        /* Vertical alignment changed */
                        adv_data->vertical_align_selected = msg_code;
                        CONSOLE_DEBUG("Vertical alignment changed to: %s\n",
                               vertical_align_labels[adv_data->vertical_align_selected]);
                        break;
                    
                    case GID_ADV_REVERSE_SORT:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, adv_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            adv_data->reverse_sort_enabled = (BOOL)checked;
                            CONSOLE_DEBUG("Reverse Sort: %s\n", 
                                   adv_data->reverse_sort_enabled ? "ENABLED" : "DISABLED");
                        }
                        break;
                    
                    case GID_ADV_OPTIMIZE_COLS:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, adv_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            adv_data->optimize_cols_enabled = (BOOL)checked;
                            CONSOLE_DEBUG("Optimize Column Widths: %s\n", 
                                   adv_data->optimize_cols_enabled ? "ENABLED" : "DISABLED");
                        }
                        break;
                    
                    case GID_ADV_SKIP_HIDDEN:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, adv_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            adv_data->skip_hidden_enabled = (BOOL)checked;
                            CONSOLE_DEBUG("Skip Hidden Folders: %s\n", 
                                   adv_data->skip_hidden_enabled ? "ENABLED" : "DISABLED");
                        }
                        break;
                    
                    case GID_ADV_STRIP_NEWICON_BORDERS:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, adv_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            adv_data->strip_newicon_borders_enabled = (BOOL)checked;
                            CONSOLE_DEBUG("Strip NewIcon Borders: %s\n", 
                                   adv_data->strip_newicon_borders_enabled ? "ENABLED" : "DISABLED");
                        }
                        break;
                    
                    case GID_ADV_COLUMN_LAYOUT:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, adv_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            adv_data->column_layout_enabled = (BOOL)checked;
                            CONSOLE_DEBUG("Column Layout: %s\n", 
                                   adv_data->column_layout_enabled ? "ENABLED" : "DISABLED");
                        }
                        break;
                    
                    case GID_ADV_BETA_OPTIONS:
                        /* Open Beta Options window */
                        {
                            struct iTidyBetaOptionsWindow beta_window;
                            
                            CONSOLE_DEBUG("Opening Beta Options window...\n");
                            
                            if (open_itidy_beta_options_window(&beta_window, adv_data->prefs))
                            {
                                /* Run the beta options event loop */
                                while (handle_beta_options_window_events(&beta_window))
                                {
                                    WaitPort(beta_window.window->UserPort);
                                }
                                
                                /* Close the beta options window */
                                close_itidy_beta_options_window(&beta_window);
                                
                                /* Log result */
                                if (beta_window.changes_accepted)
                                {
                                    CONSOLE_DEBUG("Beta options saved\n");
                                }
                                else
                                {
                                    CONSOLE_DEBUG("Beta options cancelled\n");
                                }
                            }
                            else
                            {
                                CONSOLE_ERROR("Failed to open Beta Options window\n");
                            }
                        }
                        break;
                        
                    case GID_ADV_SPACING_X:
                    case GID_ADV_SPACING_Y:
                    case GID_ADV_MIN_ICONS_ROW:
                    case GID_ADV_MAX_ICONS_ROW:
                        /* These are updated when OK is clicked */
                        break;
                        
                    default:
                        CONSOLE_DEBUG("Unknown gadget ID: %lu\n", (unsigned long)gad->GadgetID);
                        break;
                }
                break;
        }
    }
    
    return continue_running;
}

void load_preferences_to_advanced_window(struct iTidyAdvancedWindow *adv_data)
{
    if (!adv_data || !adv_data->window || !adv_data->prefs)
    {
        CONSOLE_ERROR("NULL pointer in load_preferences_to_advanced_window\n");
        return;
    }
    
    log_debug(LOG_GUI, "load_preferences_to_advanced_window() called:\n");
    log_debug(LOG_GUI, "  Setting max_width_pct_cycle to: %ld\n", (long)adv_data->max_width_pct_selected);
    log_debug(LOG_GUI, "  Setting vertical_align_cycle to: %ld\n", (long)adv_data->vertical_align_selected);
    log_debug(LOG_GUI, "  Setting overflow_mode_cycle to: %ld\n", (long)adv_data->overflow_mode_selected);
    log_debug(LOG_GUI, "  Setting aspect_ratio_cycle to: %ld\n", (long)adv_data->aspect_preset_selected);
    
    /* Aspect ratio preset */
    GT_SetGadgetAttrs(adv_data->aspect_ratio_cycle, adv_data->window, NULL,
        GTCY_Active, adv_data->aspect_preset_selected,
        TAG_END);
    
    /* Overflow mode */
    GT_SetGadgetAttrs(adv_data->overflow_mode_cycle, adv_data->window, NULL,
        GTCY_Active, adv_data->overflow_mode_selected,
        TAG_END);
    
    /* Spacing sliders */
    GT_SetGadgetAttrs(adv_data->spacing_x_slider, adv_data->window, NULL,
        GTSL_Level, adv_data->spacing_x_value,
        TAG_END);
    
    GT_SetGadgetAttrs(adv_data->spacing_y_slider, adv_data->window, NULL,
        GTSL_Level, adv_data->spacing_y_value,
        TAG_END);
    
    /* Column limits */
    GT_SetGadgetAttrs(adv_data->min_icons_row_int, adv_data->window, NULL,
        GTIN_Number, adv_data->min_icons_per_row,
        TAG_END);
    
    /* Auto checkbox */
    GT_SetGadgetAttrs(adv_data->max_auto_checkbox, adv_data->window, NULL,
        GTCB_Checked, adv_data->max_auto_enabled,
        TAG_END);
    
    /* Max icons/row (only if not Auto) */
    GT_SetGadgetAttrs(adv_data->max_icons_row_int, adv_data->window, NULL,
        GTIN_Number, adv_data->max_icons_per_row > 0 ? adv_data->max_icons_per_row : 5,
        GA_Disabled, adv_data->max_auto_enabled,
        TAG_END);
    
    /* Max window width percentage */
    GT_SetGadgetAttrs(adv_data->max_width_pct_cycle, adv_data->window, NULL,
        GTCY_Active, adv_data->max_width_pct_selected,
        GA_Disabled, !adv_data->max_auto_enabled,
        TAG_END);
    
    /* Vertical alignment */
    GT_SetGadgetAttrs(adv_data->vertical_align_cycle, adv_data->window, NULL,
        GTCY_Active, adv_data->vertical_align_selected,
        TAG_END);
    
    /* Reverse sort checkbox */
    GT_SetGadgetAttrs(adv_data->reverse_sort_check, adv_data->window, NULL,
        GTCB_Checked, adv_data->reverse_sort_enabled,
        TAG_END);
    
    /* Optimize columns checkbox */
    GT_SetGadgetAttrs(adv_data->optimize_cols_check, adv_data->window, NULL,
        GTCB_Checked, adv_data->optimize_cols_enabled,
        TAG_END);
    
    /* Column layout checkbox */
    GT_SetGadgetAttrs(adv_data->column_layout_check, adv_data->window, NULL,
        GTCB_Checked, adv_data->column_layout_enabled,
        TAG_END);
    
    /* Skip hidden folders checkbox */
    GT_SetGadgetAttrs(adv_data->skip_hidden_check, adv_data->window, NULL,
        GTCB_Checked, adv_data->skip_hidden_enabled,
        TAG_END);
    
    /* Strip NewIcon borders checkbox */
    GT_SetGadgetAttrs(adv_data->strip_newicon_borders_check, adv_data->window, NULL,
        GTCB_Checked, adv_data->strip_newicon_borders_enabled,
        TAG_END);
    
    /* Enable/disable max icons gadget based on Auto checkbox */
    set_max_icons_gadget_state(adv_data, adv_data->max_auto_enabled);
    
    /* Enable/disable max width pct gadget based on Auto checkbox */
    update_max_width_pct_gadget_state(adv_data);
    
    CONSOLE_DEBUG("Preferences loaded into advanced window\n");
}
