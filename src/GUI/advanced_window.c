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

#include "advanced_window.h"
#include "layout_preferences.h"
#include "writeLog.h"

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define ADV_WINDOW_TITLE "iTidy - Advanced Settings"
#define ADV_WINDOW_WIDTH 420
#define ADV_WINDOW_HEIGHT 280
#define ADV_WINDOW_LEFT 100
#define ADV_WINDOW_TOP 50

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
    "Custom",
    NULL
};

static STRPTR overflow_mode_labels[] = {
    "Expand Horizontally",
    "Expand Vertically",
    "Expand Both",
    NULL
};

/*------------------------------------------------------------------------*/
/* Aspect Ratio Preset Values                                            */
/*------------------------------------------------------------------------*/
static const float aspect_ratio_presets[] = {
    0.75f,  /* Tall */
    1.0f,   /* Square */
    1.3f,   /* Compact */
    1.6f,   /* Classic */
    2.0f,   /* Wide */
    2.4f,   /* Ultrawide */
    0.0f    /* Custom (calculated from width:height) */
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
    
    /* Check if using custom aspect ratio */
    if (prefs->useCustomAspectRatio)
    {
        return ASPECT_PRESET_CUSTOM;
    }
    
    /* Find matching preset (allow small floating point differences) */
    for (i = 0; i < 6; i++)
    {
        float diff = prefs->aspectRatio - aspect_ratio_presets[i];
        if (diff < 0.0f) diff = -diff;  /* abs() */
        
        if (diff < 0.01f)  /* Within tolerance */
        {
            return i;
        }
    }
    
    /* Default to Classic if no match */
    return ASPECT_PRESET_CLASSIC;
}

/**
 * @brief Create all gadgets for the advanced settings window
 *
 * @param adv_data Pointer to advanced window data structure
 * @return struct Gadget* Pointer to gadget list, or NULL on failure
 */
static struct Gadget *create_advanced_gadgets(struct iTidyAdvancedWindow *adv_data)
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
        printf("ERROR: Failed to create gadget context\n");
        return NULL;
    }
    
    current_y = 10;
    
    /*--------------------------------------------------------------------*/
    /* Aspect Ratio Cycle Gadget                                         */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = gadget_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = gadget_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Aspect Ratio:";
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
        printf("ERROR: Failed to create aspect ratio cycle gadget\n");
        return NULL;
    }
    
    current_y += button_height + 8;
    
    /*--------------------------------------------------------------------*/
    /* Custom Aspect Ratio Width Integer Gadget                         */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = gadget_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = small_gadget_width;
    ng.ng_Height = string_height;
    ng.ng_GadgetText = "Custom Width:";
    ng.ng_GadgetID = GID_ADV_CUSTOM_WIDTH;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->custom_width_int = gad = CreateGadget(INTEGER_KIND, gad, &ng,
        GTIN_Number, adv_data->custom_aspect_width,
        GTIN_MaxChars, 3,
        GA_Disabled, (adv_data->aspect_preset_selected != ASPECT_PRESET_CUSTOM),
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create custom width integer gadget\n");
        return NULL;
    }
    
    current_y += string_height + 4;
    
    /*--------------------------------------------------------------------*/
    /* Custom Aspect Ratio Height Integer Gadget                        */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = gadget_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = small_gadget_width;
    ng.ng_Height = string_height;
    ng.ng_GadgetText = "Custom Height:";
    ng.ng_GadgetID = GID_ADV_CUSTOM_HEIGHT;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->custom_height_int = gad = CreateGadget(INTEGER_KIND, gad, &ng,
        GTIN_Number, adv_data->custom_aspect_height,
        GTIN_MaxChars, 3,
        GA_Disabled, (adv_data->aspect_preset_selected != ASPECT_PRESET_CUSTOM),
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create custom height integer gadget\n");
        return NULL;
    }
    
    current_y += string_height + 12;
    
    /*--------------------------------------------------------------------*/
    /* Window Overflow Mode Cycle Gadget                                 */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = gadget_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = gadget_width + 60;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Window Overflow:";
    ng.ng_GadgetID = GID_ADV_OVERFLOW_MODE;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->overflow_mode_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, overflow_mode_labels,
        GTCY_Active, adv_data->overflow_mode_selected,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create overflow mode cycle gadget\n");
        return NULL;
    }
    
    current_y += button_height + 12;
    
    /*--------------------------------------------------------------------*/
    /* Horizontal Spacing Slider                                         */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = gadget_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = gadget_width + 40;
    ng.ng_Height = slider_height;
    ng.ng_GadgetText = "Horizontal Spacing:";
    ng.ng_GadgetID = GID_ADV_SPACING_X;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->spacing_x_slider = gad = CreateGadget(SLIDER_KIND, gad, &ng,
        GTSL_Min, 4,
        GTSL_Max, 20,
        GTSL_Level, adv_data->spacing_x_value,
        GTSL_MaxLevelLen, 3,
        GTSL_LevelFormat, "%2ld",
        GTSL_LevelPlace, PLACETEXT_RIGHT,
        GA_RelVerify, TRUE,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create horizontal spacing slider\n");
        return NULL;
    }
    
    current_y += slider_height + 8;
    
    /*--------------------------------------------------------------------*/
    /* Vertical Spacing Slider                                           */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = gadget_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = gadget_width + 40;
    ng.ng_Height = slider_height;
    ng.ng_GadgetText = "Vertical Spacing:";
    ng.ng_GadgetID = GID_ADV_SPACING_Y;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->spacing_y_slider = gad = CreateGadget(SLIDER_KIND, gad, &ng,
        GTSL_Min, 4,
        GTSL_Max, 20,
        GTSL_Level, adv_data->spacing_y_value,
        GTSL_MaxLevelLen, 3,
        GTSL_LevelFormat, "%2ld",
        GTSL_LevelPlace, PLACETEXT_RIGHT,
        GA_RelVerify, TRUE,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create vertical spacing slider\n");
        return NULL;
    }
    
    current_y += slider_height + 12;
    
    /*--------------------------------------------------------------------*/
    /* Min Icons Per Row Integer Gadget                                  */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = gadget_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = small_gadget_width;
    ng.ng_Height = string_height;
    ng.ng_GadgetText = "Min Icons/Row:";
    ng.ng_GadgetID = GID_ADV_MIN_ICONS_ROW;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->min_icons_row_int = gad = CreateGadget(INTEGER_KIND, gad, &ng,
        GTIN_Number, adv_data->min_icons_per_row,
        GTIN_MaxChars, 2,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create min icons/row integer gadget\n");
        return NULL;
    }
    
    current_y += string_height + 4;
    
    /*--------------------------------------------------------------------*/
    /* Auto Max Icons/Row Checkbox                                       */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = gadget_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Auto Max Icons/Row:";
    ng.ng_GadgetID = GID_ADV_MAX_AUTO_CHECKBOX;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->max_auto_checkbox = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, adv_data->max_auto_enabled,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create auto max icons checkbox\n");
        return NULL;
    }
    
    current_y += button_height + 4;
    
    /*--------------------------------------------------------------------*/
    /* Max Icons Per Row Integer Gadget                                  */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = gadget_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = small_gadget_width;
    ng.ng_Height = string_height;
    ng.ng_GadgetText = "Max Icons/Row:";
    ng.ng_GadgetID = GID_ADV_MAX_ICONS_ROW;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    adv_data->max_icons_row_int = gad = CreateGadget(INTEGER_KIND, gad, &ng,
        GTIN_Number, adv_data->max_icons_per_row > 0 ? adv_data->max_icons_per_row : 5,
        GTIN_MaxChars, 2,
        GA_Disabled, adv_data->max_auto_enabled,  /* Disabled if Auto checked */
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create max icons/row integer gadget\n");
        return NULL;
    }
    
    current_y += string_height + 16;
    
    /*--------------------------------------------------------------------*/
    /* OK and Cancel Buttons                                             */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = ADV_WINDOW_WIDTH - 200;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 80;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "OK";
    ng.ng_GadgetID = GID_ADV_OK;
    ng.ng_Flags = PLACETEXT_IN;
    
    adv_data->ok_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create OK button\n");
        return NULL;
    }
    
    ng.ng_LeftEdge = ADV_WINDOW_WIDTH - 110;
    ng.ng_GadgetText = "Cancel";
    ng.ng_GadgetID = GID_ADV_CANCEL;
    
    adv_data->cancel_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create Cancel button\n");
        return NULL;
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
    
    if (!adv_data || !prefs)
    {
        printf("ERROR: NULL pointer passed to open_itidy_advanced_window\n");
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
    
    /* Get Workbench screen */
    adv_data->screen = LockPubScreen(NULL);
    if (!adv_data->screen)
    {
        printf("ERROR: Failed to lock Workbench screen\n");
        return FALSE;
    }
    
    /* Get visual info for GadTools */
    adv_data->visual_info = GetVisualInfo(adv_data->screen, TAG_END);
    if (!adv_data->visual_info)
    {
        printf("ERROR: Failed to get visual info\n");
        UnlockPubScreen(NULL, adv_data->screen);
        return FALSE;
    }
    
    /* Create gadgets */
    glist = create_advanced_gadgets(adv_data);
    if (!glist)
    {
        printf("ERROR: Failed to create gadgets\n");
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
        WA_Height, ADV_WINDOW_HEIGHT,
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
        printf("ERROR: Failed to open advanced settings window\n");
        FreeGadgets(adv_data->glist);
        FreeVisualInfo(adv_data->visual_info);
        UnlockPubScreen(NULL, adv_data->screen);
        return FALSE;
    }
    
    adv_data->window_open = TRUE;
    
    /* Refresh gadgets */
    GT_RefreshWindow(adv_data->window, NULL);
    
    printf("Advanced Settings window opened successfully\n");
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
    
    printf("Advanced Settings window closed\n");
}

void set_custom_ratio_gadgets_state(struct iTidyAdvancedWindow *adv_data, 
                                    BOOL enable)
{
    if (!adv_data || !adv_data->window)
    {
        return;
    }
    
    /* Enable/disable custom width gadget */
    if (adv_data->custom_width_int)
    {
        GT_SetGadgetAttrs(adv_data->custom_width_int, adv_data->window, NULL,
            GA_Disabled, !enable,
            TAG_END);
    }
    
    /* Enable/disable custom height gadget */
    if (adv_data->custom_height_int)
    {
        GT_SetGadgetAttrs(adv_data->custom_height_int, adv_data->window, NULL,
            GA_Disabled, !enable,
            TAG_END);
    }
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

void save_advanced_window_to_preferences(struct iTidyAdvancedWindow *adv_data)
{
    if (!adv_data || !adv_data->prefs)
    {
        printf("ERROR: NULL pointer in save_advanced_window_to_preferences\n");
        return;
    }
    
    /* Save aspect ratio settings */
    if (adv_data->aspect_preset_selected == ASPECT_PRESET_CUSTOM)
    {
        /* Using custom aspect ratio */
        adv_data->prefs->useCustomAspectRatio = TRUE;
        adv_data->prefs->customAspectWidth = adv_data->custom_aspect_width;
        adv_data->prefs->customAspectHeight = adv_data->custom_aspect_height;
        
        /* Calculate aspect ratio from custom values */
        if (adv_data->custom_aspect_height > 0)
        {
            adv_data->prefs->aspectRatio = 
                (float)adv_data->custom_aspect_width / 
                (float)adv_data->custom_aspect_height;
        }
        else
        {
            /* Invalid height, default to Classic */
            adv_data->prefs->aspectRatio = 1.6f;
            printf("WARNING: Invalid custom aspect height, using default 1.6\n");
        }
    }
    else
    {
        /* Using preset aspect ratio */
        adv_data->prefs->useCustomAspectRatio = FALSE;
        adv_data->prefs->aspectRatio = 
            aspect_ratio_presets[adv_data->aspect_preset_selected];
    }
    
    /* Save overflow mode */
    adv_data->prefs->overflowMode = adv_data->overflow_mode_selected;
    
    /* Save spacing values */
    adv_data->prefs->iconSpacingX = adv_data->spacing_x_value;
    adv_data->prefs->iconSpacingY = adv_data->spacing_y_value;
    
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
    
    printf("Advanced settings saved to preferences:\n");
    printf("  Aspect Ratio: %.2f (Custom: %s)\n", 
           adv_data->prefs->aspectRatio,
           adv_data->prefs->useCustomAspectRatio ? "YES" : "NO");
    printf("  Overflow Mode: %d\n", adv_data->prefs->overflowMode);
    printf("  Spacing: %d x %d\n", 
           adv_data->prefs->iconSpacingX, 
           adv_data->prefs->iconSpacingY);
    printf("  Min Icons/Row: %d\n", adv_data->prefs->minIconsPerRow);
    printf("  Max Icons/Row: %d (%s)\n", 
           adv_data->prefs->maxIconsPerRow,
           adv_data->prefs->maxIconsPerRow == 0 ? "AUTO" : "MANUAL");
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
                printf("Advanced window close requested\n");
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
                        printf("OK button clicked\n");
                        
                        /* Read all gadget values into local variables */
                        {
                            LONG temp_number;
                            
                            /* Custom width */
                            GT_GetGadgetAttrs(adv_data->custom_width_int, 
                                            adv_data->window, NULL,
                                            GTIN_Number, &temp_number, TAG_END);
                            adv_data->custom_aspect_width = (UWORD)temp_number;
                            
                            /* Custom height */
                            GT_GetGadgetAttrs(adv_data->custom_height_int, 
                                            adv_data->window, NULL,
                                            GTIN_Number, &temp_number, TAG_END);
                            adv_data->custom_aspect_height = (UWORD)temp_number;
                            
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
                        }
                        
                        /* Save to preferences */
                        save_advanced_window_to_preferences(adv_data);
                        adv_data->changes_accepted = TRUE;
                        continue_running = FALSE;
                        break;
                        
                    case GID_ADV_CANCEL:
                        printf("Cancel button clicked\n");
                        adv_data->changes_accepted = FALSE;
                        continue_running = FALSE;
                        break;
                        
                    case GID_ADV_ASPECT_RATIO:
                        /* CRITICAL: Use msg_code for cycle gadgets (not GT_GetGadgetAttrs) */
                        adv_data->aspect_preset_selected = msg_code;
                        printf("Aspect ratio changed to: %s\n", 
                               aspect_ratio_labels[adv_data->aspect_preset_selected]);
                        
                        /* Enable/disable custom ratio gadgets */
                        set_custom_ratio_gadgets_state(adv_data, 
                            adv_data->aspect_preset_selected == ASPECT_PRESET_CUSTOM);
                        break;
                        
                    case GID_ADV_OVERFLOW_MODE:
                        /* CRITICAL: Use msg_code for cycle gadgets (not GT_GetGadgetAttrs) */
                        adv_data->overflow_mode_selected = msg_code;
                        printf("Overflow mode changed to: %s\n", 
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
                            
                            printf("Auto Max Icons/Row: %s\n",
                                   adv_data->max_auto_enabled ? "ENABLED" : "DISABLED");
                            
                            /* Enable/disable the max icons/row integer gadget */
                            set_max_icons_gadget_state(adv_data, adv_data->max_auto_enabled);
                        }
                        break;
                        
                    case GID_ADV_CUSTOM_WIDTH:
                    case GID_ADV_CUSTOM_HEIGHT:
                    case GID_ADV_SPACING_X:
                    case GID_ADV_SPACING_Y:
                    case GID_ADV_MIN_ICONS_ROW:
                    case GID_ADV_MAX_ICONS_ROW:
                        /* These are updated when OK is clicked */
                        break;
                        
                    default:
                        printf("Unknown gadget ID: %d\n", gad->GadgetID);
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
        printf("ERROR: NULL pointer in load_preferences_to_advanced_window\n");
        return;
    }
    
    /* Aspect ratio preset */
    GT_SetGadgetAttrs(adv_data->aspect_ratio_cycle, adv_data->window, NULL,
        GTCY_Active, adv_data->aspect_preset_selected,
        TAG_END);
    
    /* Custom aspect ratio values */
    GT_SetGadgetAttrs(adv_data->custom_width_int, adv_data->window, NULL,
        GTIN_Number, adv_data->custom_aspect_width,
        TAG_END);
    
    GT_SetGadgetAttrs(adv_data->custom_height_int, adv_data->window, NULL,
        GTIN_Number, adv_data->custom_aspect_height,
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
    
    /* Enable/disable custom ratio gadgets based on selection */
    set_custom_ratio_gadgets_state(adv_data, 
        adv_data->aspect_preset_selected == ASPECT_PRESET_CUSTOM);
    
    /* Enable/disable max icons gadget based on Auto checkbox */
    set_max_icons_gadget_state(adv_data, adv_data->max_auto_enabled);
    
    printf("Preferences loaded into advanced window\n");
}
