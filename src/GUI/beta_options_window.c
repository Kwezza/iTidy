/*
 * beta_options_window.c - iTidy Beta Options Window Implementation
 * GadTools-based GUI for Workbench 3.0+
 * Experimental Feature Configuration
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

#include "beta_options_window.h"
#include "layout_preferences.h"
#include "writeLog.h"
#include "../itidy_types.h"
#include "../Settings/IControlPrefs.h"

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define BETA_WINDOW_TITLE "iTidy - Beta Options"
#define BETA_WINDOW_REFERENCE_WIDTH 55  /* Characters */

/* Spacing constants */
#define BETA_MARGIN_LEFT   10
#define BETA_MARGIN_TOP    10
#define BETA_MARGIN_RIGHT  10
#define BETA_MARGIN_BOTTOM 10
#define BETA_SPACE_X       8
#define BETA_SPACE_Y       8
#define BETA_BUTTON_TEXT_PADDING 8

/*------------------------------------------------------------------------*/
/* Helper Functions                                                       */
/*------------------------------------------------------------------------*/

/**
 * @brief Create all gadgets for beta options window
 *
 * Creates checkboxes and buttons, calculates window dimensions based on
 * actual gadget sizes. Follows font-aware sizing patterns.
 *
 * @param beta_data Pointer to beta options window data structure
 * @param font Text font to use for gadgets
 * @return struct Gadget* Pointer to gadget list, or NULL on error
 */
static struct Gadget *create_beta_options_gadgets(struct iTidyBetaOptionsWindow *beta_data,
                                                   struct TextAttr *font)
{
    struct NewGadget ng;
    struct Gadget *gad = NULL;
    struct RastPort temp_rp;
    UWORD font_width, font_height;
    UWORD checkbox_height, button_height, cycle_height;
    UWORD current_y;
    UWORD reference_width_pixels;
    UWORD button_width;
    UWORD max_button_text_width;
    UWORD ok_text_width, cancel_text_width;
    
    /* Log level cycle gadget labels */
    static STRPTR log_level_labels[] = {
        "DEBUG (Verbose)",
        "INFO (Normal)",
        "WARNING (Warnings only)",
        "ERROR (Errors only)",
        NULL
    };
    
    /* Initialize temporary RastPort for text measurements */
    InitRastPort(&temp_rp);
    SetFont(&temp_rp, beta_data->screen->RastPort.Font);
    
    /* Calculate font dimensions */
    font_width = temp_rp.Font->tf_XSize;
    font_height = temp_rp.Font->tf_YSize;
    
    /* Calculate gadget heights */
    checkbox_height = font_height + 6;
    button_height = font_height + 6;
    cycle_height = font_height + 6;
    
    /* Calculate reference width in pixels */
    reference_width_pixels = BETA_WINDOW_REFERENCE_WIDTH * font_width;
    
    /* Calculate button widths */
    ok_text_width = TextLength(&temp_rp, "OK", 2) + BETA_BUTTON_TEXT_PADDING;
    cancel_text_width = TextLength(&temp_rp, "Cancel", 6) + BETA_BUTTON_TEXT_PADDING;
    max_button_text_width = (ok_text_width > cancel_text_width) ? ok_text_width : cancel_text_width;
    
    /* Equal button width for two buttons */
    button_width = (reference_width_pixels - BETA_SPACE_X) / 2;
    if (button_width < max_button_text_width)
        button_width = max_button_text_width;
    
    /* Start creating gadgets */
    gad = CreateContext(&beta_data->glist);
    if (!gad)
    {
        printf("ERROR: Failed to create gadget context for beta options window\n");
        return NULL;
    }
    
    current_y = BETA_MARGIN_TOP;
    
    /*--------------------------------------------------------------------*/
    /* Open Folders After Processing Checkbox                            */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = BETA_MARGIN_LEFT + prefsIControl.currentLeftBarWidth;
    ng.ng_TopEdge = current_y + prefsIControl.currentWindowBarHeight;
    ng.ng_Width = 26;
    ng.ng_Height = checkbox_height;
    ng.ng_GadgetText = "Auto-open folders after processing";
    ng.ng_TextAttr = font;
    ng.ng_GadgetID = GID_BETA_OPEN_FOLDERS;
    ng.ng_Flags = PLACETEXT_RIGHT;
    ng.ng_VisualInfo = beta_data->visual_info;
    
    beta_data->open_folders_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, beta_data->open_folders_enabled,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create open folders checkbox\n");
        return NULL;
    }
    
    current_y += checkbox_height + BETA_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* Update Window Geometry Checkbox                                   */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = BETA_MARGIN_LEFT + prefsIControl.currentLeftBarWidth;
    ng.ng_TopEdge = current_y + prefsIControl.currentWindowBarHeight;
    ng.ng_Width = 26;
    ng.ng_Height = checkbox_height;
    ng.ng_GadgetText = "Find and update open folder windows";
    ng.ng_GadgetID = GID_BETA_UPDATE_WINDOWS;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    beta_data->update_windows_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, beta_data->update_windows_enabled,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create update windows checkbox\n");
        return NULL;
    }
    
    current_y += checkbox_height + BETA_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* Log Level Cycle Gadget                                            */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = BETA_MARGIN_LEFT + prefsIControl.currentLeftBarWidth;
    ng.ng_TopEdge = current_y + prefsIControl.currentWindowBarHeight;
    ng.ng_Width = reference_width_pixels;
    ng.ng_Height = cycle_height;
    ng.ng_GadgetText = "Log Level:";
    ng.ng_TextAttr = font;
    ng.ng_GadgetID = GID_BETA_LOG_LEVEL;
    ng.ng_Flags = PLACETEXT_LEFT;
    ng.ng_VisualInfo = beta_data->visual_info;
    
    beta_data->log_level_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, log_level_labels,
        GTCY_Active, beta_data->log_level_selected,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create log level cycle gadget\n");
        return NULL;
    }
    
    current_y += cycle_height + BETA_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* Memory Logging Checkbox                                           */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = BETA_MARGIN_LEFT + prefsIControl.currentLeftBarWidth;
    ng.ng_TopEdge = current_y + prefsIControl.currentWindowBarHeight;
    ng.ng_Width = 26;
    ng.ng_Height = checkbox_height;
    ng.ng_GadgetText = "Enable memory allocation logging";
    ng.ng_GadgetID = GID_BETA_MEMORY_LOGGING;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    beta_data->memory_logging_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, beta_data->memory_logging_enabled,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create memory logging checkbox\n");
        return NULL;
    }
    
    current_y += checkbox_height + BETA_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* Performance Logging Checkbox                                      */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = BETA_MARGIN_LEFT + prefsIControl.currentLeftBarWidth;
    ng.ng_TopEdge = current_y + prefsIControl.currentWindowBarHeight;
    ng.ng_Width = 26;
    ng.ng_Height = checkbox_height;
    ng.ng_GadgetText = "Enable performance timing logs";
    ng.ng_GadgetID = GID_BETA_PERFORMANCE_LOG;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    beta_data->performance_logging_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, beta_data->performance_logging_enabled,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create performance logging checkbox\n");
        return NULL;
    }
    
    current_y += checkbox_height + BETA_SPACE_Y * 3;  /* Extra space before buttons */
    
    /*--------------------------------------------------------------------*/
    /* OK Button                                                          */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = BETA_MARGIN_LEFT + prefsIControl.currentLeftBarWidth;
    ng.ng_TopEdge = current_y + prefsIControl.currentWindowBarHeight;
    ng.ng_Width = button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "OK";
    ng.ng_GadgetID = GID_BETA_OK;
    ng.ng_Flags = PLACETEXT_IN;
    
    beta_data->ok_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create OK button\n");
        return NULL;
    }
    
    /*--------------------------------------------------------------------*/
    /* Cancel Button                                                      */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = BETA_MARGIN_LEFT + prefsIControl.currentLeftBarWidth + button_width + BETA_SPACE_X;
    ng.ng_TopEdge = current_y + prefsIControl.currentWindowBarHeight;
    ng.ng_Width = button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Cancel";
    ng.ng_GadgetID = GID_BETA_CANCEL;
    ng.ng_Flags = PLACETEXT_IN;
    
    beta_data->cancel_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        TAG_END);
    
    if (!gad)
    {
        printf("ERROR: Failed to create Cancel button\n");
        return NULL;
    }
    
    return gad;
}

/*------------------------------------------------------------------------*/
/* Public Functions                                                       */
/*------------------------------------------------------------------------*/

BOOL open_itidy_beta_options_window(struct iTidyBetaOptionsWindow *beta_data, 
                                     LayoutPreferences *prefs)
{
    struct TextAttr font_attr;
    struct Gadget *gad;
    UWORD window_width, window_height;
    UWORD font_width, font_height;
    struct RastPort temp_rp;
    
    if (!beta_data || !prefs)
        return FALSE;
    
    /* Initialize structure */
    memset(beta_data, 0, sizeof(struct iTidyBetaOptionsWindow));
    beta_data->prefs = prefs;
    
    /* Load current preference values into window structure */
    beta_data->open_folders_enabled = prefs->beta_openFoldersAfterProcessing;
    beta_data->update_windows_enabled = prefs->beta_FindWindowOnWorkbenchAndUpdate;
    beta_data->log_level_selected = prefs->logLevel;
    beta_data->memory_logging_enabled = prefs->memoryLoggingEnabled;
    beta_data->performance_logging_enabled = prefs->enable_performance_logging;
    
    /* Lock Workbench screen */
    beta_data->screen = LockPubScreen("Workbench");
    if (!beta_data->screen)
    {
        printf("ERROR: Failed to lock Workbench screen\n");
        return FALSE;
    }
    
    /* Get visual info */
    beta_data->visual_info = GetVisualInfo(beta_data->screen, TAG_END);
    if (!beta_data->visual_info)
    {
        printf("ERROR: Failed to get visual info\n");
        UnlockPubScreen(NULL, beta_data->screen);
        return FALSE;
    }
    
    /* Set up font */
    font_attr.ta_Name = beta_data->screen->Font->ta_Name;
    font_attr.ta_YSize = beta_data->screen->Font->ta_YSize;
    font_attr.ta_Style = FS_NORMAL;
    font_attr.ta_Flags = 0;
    
    /* Calculate font dimensions for window sizing */
    InitRastPort(&temp_rp);
    SetFont(&temp_rp, beta_data->screen->RastPort.Font);
    font_width = temp_rp.Font->tf_XSize;
    font_height = temp_rp.Font->tf_YSize;
    
    /* Create gadgets */
    gad = create_beta_options_gadgets(beta_data, &font_attr);
    if (!gad)
    {
        printf("ERROR: Failed to create beta options gadgets\n");
        close_itidy_beta_options_window(beta_data);
        return FALSE;
    }
    
    /* Calculate window dimensions based on content */
    window_width = BETA_MARGIN_LEFT + prefsIControl.currentLeftBarWidth +
                   (BETA_WINDOW_REFERENCE_WIDTH * font_width) +
                   BETA_MARGIN_RIGHT + prefsIControl.currentBarWidth;
    
    /* Height = margins + window chrome + 2 checkboxes + 1 cycle + 2 checkboxes + spacing + button row */
    window_height = BETA_MARGIN_TOP + prefsIControl.currentWindowBarHeight +
                    ((font_height + 6) * 2) +  /* 2 checkboxes */
                    (font_height + 6) +        /* 1 cycle gadget */
                    ((font_height + 6) * 2) +  /* 2 checkboxes (memory + performance logging) */
                    (BETA_SPACE_Y * 7) +       /* Spacing between elements */
                    (font_height + 6) +        /* Button row */
                    BETA_MARGIN_BOTTOM + prefsIControl.currentBarHeight;
    
    /* Open window */
    beta_data->window = OpenWindowTags(NULL,
        WA_Left,        (beta_data->screen->Width - window_width) / 2,
        WA_Top,         (beta_data->screen->Height - window_height) / 2,
        WA_Width,       window_width,
        WA_Height,      window_height,
        WA_Title,       BETA_WINDOW_TITLE,
        WA_Gadgets,     beta_data->glist,
        WA_CustomScreen, beta_data->screen,
        WA_DragBar,     TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate,    TRUE,
        WA_IDCMP,       IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | 
                        IDCMP_GADGETUP | BUTTONIDCMP | CHECKBOXIDCMP,
        TAG_END);
    
    if (!beta_data->window)
    {
        printf("ERROR: Failed to open beta options window\n");
        close_itidy_beta_options_window(beta_data);
        return FALSE;
    }
    
    /* Refresh gadgets */
    GT_RefreshWindow(beta_data->window, NULL);
    
    beta_data->window_open = TRUE;
    return TRUE;
}

void close_itidy_beta_options_window(struct iTidyBetaOptionsWindow *beta_data)
{
    if (!beta_data)
        return;
    
    if (beta_data->window)
    {
        CloseWindow(beta_data->window);
        beta_data->window = NULL;
    }
    
    if (beta_data->glist)
    {
        FreeGadgets(beta_data->glist);
        beta_data->glist = NULL;
    }
    
    if (beta_data->visual_info)
    {
        FreeVisualInfo(beta_data->visual_info);
        beta_data->visual_info = NULL;
    }
    
    if (beta_data->screen)
    {
        UnlockPubScreen(NULL, beta_data->screen);
        beta_data->screen = NULL;
    }
    
    beta_data->window_open = FALSE;
}

BOOL handle_beta_options_window_events(struct iTidyBetaOptionsWindow *beta_data)
{
    struct IntuiMessage *imsg;
    ULONG class;
    UWORD code;
    struct Gadget *gadget;
    BOOL keep_going = TRUE;
    
    if (!beta_data || !beta_data->window)
        return FALSE;
    
    while ((imsg = GT_GetIMsg(beta_data->window->UserPort)) != NULL)
    {
        class = imsg->Class;
        code = imsg->Code;
        gadget = (struct Gadget *)imsg->IAddress;
        
        GT_ReplyIMsg(imsg);
        
        switch (class)
        {
            case IDCMP_CLOSEWINDOW:
                /* User clicked close gadget - same as Cancel */
                beta_data->changes_accepted = FALSE;
                keep_going = FALSE;
                break;
            
            case IDCMP_REFRESHWINDOW:
                /* Handle window refresh */
                GT_BeginRefresh(beta_data->window);
                GT_EndRefresh(beta_data->window, TRUE);
                break;
            
            case IDCMP_GADGETUP:
                /* Handle gadget events */
                switch (gadget->GadgetID)
                {
                    case GID_BETA_OPEN_FOLDERS:
                        /* Update internal state from checkbox */
                        beta_data->open_folders_enabled = 
                            (code == 1) ? TRUE : FALSE;
                        break;
                    
                    case GID_BETA_UPDATE_WINDOWS:
                        /* Update internal state from checkbox */
                        beta_data->update_windows_enabled = 
                            (code == 1) ? TRUE : FALSE;
                        break;
                    
                    case GID_BETA_LOG_LEVEL:
                        /* Update log level selection from cycle gadget */
                        beta_data->log_level_selected = code;
                        break;
                    
                    case GID_BETA_MEMORY_LOGGING:
                        /* Update memory logging checkbox */
                        beta_data->memory_logging_enabled = 
                            (code == 1) ? TRUE : FALSE;
                        break;
                    
                    case GID_BETA_PERFORMANCE_LOG:
                        /* Update performance logging checkbox */
                        beta_data->performance_logging_enabled = 
                            (code == 1) ? TRUE : FALSE;
                        break;
                    
                    case GID_BETA_OK:
                        /* Save settings and close */
                        save_beta_options_window_to_preferences(beta_data);
                        beta_data->changes_accepted = TRUE;
                        keep_going = FALSE;
                        break;
                    
                    case GID_BETA_CANCEL:
                        /* Cancel without saving */
                        beta_data->changes_accepted = FALSE;
                        keep_going = FALSE;
                        break;
                }
                break;
        }
    }
    
    return keep_going;
}

void load_preferences_to_beta_options_window(struct iTidyBetaOptionsWindow *beta_data)
{
    if (!beta_data || !beta_data->prefs)
        return;
    
    /* Load values from preferences into window structure */
    beta_data->open_folders_enabled = beta_data->prefs->beta_openFoldersAfterProcessing;
    beta_data->update_windows_enabled = beta_data->prefs->beta_FindWindowOnWorkbenchAndUpdate;
    beta_data->log_level_selected = beta_data->prefs->logLevel;
    beta_data->memory_logging_enabled = beta_data->prefs->memoryLoggingEnabled;
    beta_data->performance_logging_enabled = beta_data->prefs->enable_performance_logging;
    
    /* Update gadgets if window is open */
    if (beta_data->window_open && beta_data->window)
    {
        GT_SetGadgetAttrs(beta_data->open_folders_check, beta_data->window, NULL,
            GTCB_Checked, beta_data->open_folders_enabled,
            TAG_END);
        
        GT_SetGadgetAttrs(beta_data->update_windows_check, beta_data->window, NULL,
            GTCB_Checked, beta_data->update_windows_enabled,
            TAG_END);
        
        GT_SetGadgetAttrs(beta_data->log_level_cycle, beta_data->window, NULL,
            GTCY_Active, beta_data->log_level_selected,
            TAG_END);
        
        GT_SetGadgetAttrs(beta_data->memory_logging_check, beta_data->window, NULL,
            GTCB_Checked, beta_data->memory_logging_enabled,
            TAG_END);
        
        GT_SetGadgetAttrs(beta_data->performance_logging_check, beta_data->window, NULL,
            GTCB_Checked, beta_data->performance_logging_enabled,
            TAG_END);
    }
}

void save_beta_options_window_to_preferences(struct iTidyBetaOptionsWindow *beta_data)
{
    LogLevel newLogLevel;
    
    if (!beta_data || !beta_data->prefs)
        return;
    
    /* Save values from window structure to preferences */
    beta_data->prefs->beta_openFoldersAfterProcessing = beta_data->open_folders_enabled;
    beta_data->prefs->beta_FindWindowOnWorkbenchAndUpdate = beta_data->update_windows_enabled;
    beta_data->prefs->logLevel = beta_data->log_level_selected;
    beta_data->prefs->memoryLoggingEnabled = beta_data->memory_logging_enabled;
    beta_data->prefs->enable_performance_logging = beta_data->performance_logging_enabled;
    
    /* Apply log level immediately */
    newLogLevel = (LogLevel)beta_data->log_level_selected;
    set_global_log_level(newLogLevel);
    
    /* Apply memory logging setting immediately */
    set_memory_logging_enabled(beta_data->memory_logging_enabled);
    
    /* Apply performance logging setting immediately */
    set_performance_logging_enabled(beta_data->performance_logging_enabled);
    
    append_to_log("Beta options updated:\n");
    append_to_log("  - Auto-open folders: %s\n", 
                  beta_data->open_folders_enabled ? "ENABLED" : "DISABLED");
    append_to_log("  - Update window geometry: %s\n", 
                  beta_data->update_windows_enabled ? "ENABLED" : "DISABLED");
    append_to_log("  - Log level: %s\n",
                  beta_data->log_level_selected == 0 ? "DEBUG" :
                  beta_data->log_level_selected == 1 ? "INFO" :
                  beta_data->log_level_selected == 2 ? "WARNING" : "ERROR");
    append_to_log("  - Memory logging: %s\n",
                  beta_data->memory_logging_enabled ? "ENABLED" : "DISABLED");
    append_to_log("  - Performance logging: %s\n",
                  beta_data->performance_logging_enabled ? "ENABLED" : "DISABLED");
}
