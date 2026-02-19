/*
 * PROFESSIONAL_WINDOW_LAYOUT_TEMPLATE.c
 * 
 * AmigaOS 3.0+ GadTools Window Layout Template
 * Demonstrates pre-calculated, font-sensitive layout strategy
 * 
 * KEY FEATURES:
 * - Font-sensitive using DrawInfo and TextLength()
 * - Pre-calculated gadget dimensions for perfect alignment
 * - Equal-width button rows spanning window width
 * - Buttons extending to right edge of reference gadget
 * - Respects IControl preferences for window borders
 * - Works with any font size (proportional or monospaced)
 * 
 * PORTABILITY: This pattern is fully portable and can be adapted
 * to any Amiga window requiring professional layout with proper
 * alignment and font sensitivity.
 */

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <string.h>

/* Include your IControl preferences header */
#include "../Settings/IControlPrefs.h"
extern struct IControlPrefsDetails prefsIControl;

/*------------------------------------------------------------------------*/
/* Window Layout Constants (iTidy Standard)                              */
/*------------------------------------------------------------------------*/
#define WINDOW_MARGIN_LEFT    10
#define WINDOW_MARGIN_TOP     10
#define WINDOW_MARGIN_RIGHT   10
#define WINDOW_MARGIN_BOTTOM  10
#define WINDOW_SPACE_X        8   /* Horizontal spacing between gadgets */
#define WINDOW_SPACE_Y        8   /* Vertical spacing between gadgets */
#define BUTTON_TEXT_PADDING   8   /* Standard padding around button text */

/* Standard Window Sizes (in characters) */
#define WINDOW_WIDTH_SMALL    40  /* Simple dialogs, 2 buttons */
#define WINDOW_WIDTH_MEDIUM   60  /* Main windows, 3 buttons */
#define WINDOW_WIDTH_LARGE    80  /* Complex layouts, 4 buttons */

/* Standard Input Field Widths (in characters) */
#define INPUT_WIDTH_SMALL     20  /* Dates, numbers, short text */
#define INPUT_WIDTH_MEDIUM    35  /* Filenames, paths */
#define INPUT_WIDTH_LARGE     50  /* Long paths, descriptions */

/*------------------------------------------------------------------------*/
/* Gadget IDs                                                            */
/*------------------------------------------------------------------------*/
#define GID_LABEL_TEXT        1000
#define GID_STRING_GADGET     1001
#define GID_BUTTON_RIGHT      1002
#define GID_LISTVIEW_MAIN     1003
#define GID_BUTTON_ACTION1    1004
#define GID_BUTTON_ACTION2    1005
#define GID_BUTTON_CANCEL     1006

/*------------------------------------------------------------------------*/
/**
 * @brief Open a professional, font-sensitive Amiga window
 * 
 * This function demonstrates the complete pattern for creating
 * a GadTools window with:
 * - Pre-calculated layout dimensions
 * - Font-sensitive text measurements
 * - Equal-width button rows
 * - Buttons extending to reference gadget edges
 * - Proper margin and spacing
 */
/*------------------------------------------------------------------------*/
BOOL open_professional_window(struct Window **window_out)
{
    struct Screen *screen;
    struct DrawInfo *draw_info;
    struct TextFont *font;
    struct RastPort temp_rp;
    struct NewGadget ng;
    struct Gadget *glist = NULL;
    struct Gadget *gad;
    void *visual_info;
    
    UWORD font_width, font_height;
    UWORD button_height, string_height;
    UWORD current_x, current_y;
    UWORD window_max_width, window_max_height;
    
    /* Gadget pointers */
    struct Gadget *label_gadget = NULL;
    struct Gadget *string_gadget = NULL;
    struct Gadget *button_right = NULL;
    struct Gadget *listview_main = NULL;
    struct Gadget *button_action1 = NULL;
    struct Gadget *button_action2 = NULL;
    struct Gadget *button_cancel = NULL;
    
    /*--------------------------------------------------------------------*/
    /* STEP 1: Get Screen and Font Information (AmigaOS 3.0 Style)      */
    /*--------------------------------------------------------------------*/
    screen = LockPubScreen(NULL);
    if (screen == NULL)
        return FALSE;
    
    /* Get DrawInfo for proper font access */
    draw_info = GetScreenDrawInfo(screen);
    if (draw_info == NULL)
    {
        UnlockPubScreen(NULL, screen);
        return FALSE;
    }
    
    font = draw_info->dri_Font;
    font_width = font->tf_XSize;
    font_height = font->tf_YSize;
    
    /* Initialize RastPort for TextLength() measurements */
    InitRastPort(&temp_rp);
    SetFont(&temp_rp, font);
    
    /*--------------------------------------------------------------------*/
    /* STEP 2: Calculate Gadget Dimensions                              */
    /*--------------------------------------------------------------------*/
    button_height = font_height + 6;
    string_height = font_height + 6;
    
    /* Initialize position tracking */
    current_x = prefsIControl.currentLeftBarWidth + WINDOW_MARGIN_LEFT;
    current_y = prefsIControl.currentWindowBarHeight + WINDOW_MARGIN_TOP;
    window_max_width = 0;
    window_max_height = 0;
    
    /*--------------------------------------------------------------------*/
    /* STEP 3: Create GadTools Context                                  */
    /*--------------------------------------------------------------------*/
    visual_info = GetVisualInfo(screen, TAG_END);
    if (visual_info == NULL)
    {
        FreeScreenDrawInfo(screen, draw_info);
        UnlockPubScreen(NULL, screen);
        return FALSE;
    }
    
    gad = CreateContext(&glist);
    if (gad == NULL)
    {
        FreeVisualInfo(visual_info);
        FreeScreenDrawInfo(screen, draw_info);
        UnlockPubScreen(NULL, screen);
        return FALSE;
    }
    
    /* Initialize NewGadget structure */
    ng.ng_TextAttr = screen->Font;
    ng.ng_VisualInfo = visual_info;
    ng.ng_Flags = 0;
    
    /*--------------------------------------------------------------------*/
    /* STEP 4: PRE-CALCULATE LAYOUT DIMENSIONS (iTidy Standard Pattern) */
    /*--------------------------------------------------------------------*/
    /* Establish the reference width (iTidy uses MEDIUM = 60 chars) */
    UWORD reference_width = font_width * WINDOW_WIDTH_MEDIUM;
    UWORD string_gadget_width = font_width * INPUT_WIDTH_MEDIUM;
    
    /* Calculate the maximum right edge */
    UWORD precalc_max_right = current_x + reference_width;
    
    /* Pre-calculate label dimensions using TextLength() */
    STRPTR label = "Input Field:";
    UWORD label_width = TextLength(&temp_rp, label, strlen(label));
    UWORD label_spacing = 4;
    
    /* Pre-calculate button dimensions to extend to right edge (PATTERN_INPUT_ROW) */
    UWORD string_left = current_x + label_width + label_spacing;
    UWORD string_right = string_left + string_gadget_width;
    UWORD button_right_left = string_right + WINDOW_SPACE_X;
    UWORD button_right_width = precalc_max_right - button_right_left;
    
    /*--------------------------------------------------------------------*/
    /* STEP 5: CREATE TOP ROW GADGETS (Label + String + Button)         */
    /*--------------------------------------------------------------------*/
    
    /* Label (TEXT gadget) */
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = label_width;
    ng.ng_Height = string_height;
    ng.ng_GadgetText = label;
    ng.ng_GadgetID = GID_LABEL_TEXT;
    ng.ng_Flags = PLACETEXT_IN;
    
    label_gadget = gad = CreateGadget(TEXT_KIND, gad, &ng,
        GTTX_Text, label,
        GTTX_Border, FALSE,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    /* String Gadget */
    ng.ng_LeftEdge = string_left;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = string_gadget_width;
    ng.ng_Height = string_height;
    ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = GID_STRING_GADGET;
    ng.ng_Flags = 0;
    
    string_gadget = gad = CreateGadget(STRING_KIND, gad, &ng,
        GTST_String, "",
        GTST_MaxChars, 255,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    /* Button (extends to right edge) - PRE-CALCULATED WIDTH */
    ng.ng_LeftEdge = button_right_left;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = button_right_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Browse...";
    ng.ng_GadgetID = GID_BUTTON_RIGHT;
    ng.ng_Flags = PLACETEXT_IN;
    
    button_right = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    /*--------------------------------------------------------------------*/
    /* STEP 6: CREATE LISTVIEW (PATTERN_REFERENCE_CONTENT)              */
    /*--------------------------------------------------------------------*/
    current_y += string_height + WINDOW_SPACE_Y;
    
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = reference_width;  /* Use standard width */
    ng.ng_Height = (font_height + 2) * 8;  /* 8 visible lines */
    ng.ng_GadgetText = "Main List:";
    ng.ng_GadgetID = GID_LISTVIEW_MAIN;
    ng.ng_Flags = PLACETEXT_ABOVE;
    
    listview_main = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, NULL,
        GTLV_Selected, ~0,
        GTLV_ShowSelected, NULL,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    /* Get actual height after creation (ListView may adjust) */
    UWORD actual_listview_height = listview_main->Height;
    
    /*--------------------------------------------------------------------*/
    /* STEP 7: CREATE BUTTON ROW (PATTERN_EQUAL_BUTTON_ROW)             */
    /*         Medium window = 3 buttons                                 */
    /*--------------------------------------------------------------------*/
    current_y = listview_main->TopEdge + actual_listview_height + WINDOW_SPACE_Y;
    
    /* Pre-calculate equal button widths (iTidy Standard) */
    UWORD available_width = reference_width;
    UWORD button_count = 3;  /* Standard for MEDIUM windows */
    
    /* Find maximum button text width using TextLength() */
    UWORD max_btn_text_width = TextLength(&temp_rp, "Action 1", 8);
    UWORD temp_width = TextLength(&temp_rp, "Action 2", 8);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    temp_width = TextLength(&temp_rp, "Cancel", 6);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    
    /* Calculate equal button width using standard formula */
    UWORD equal_button_width = (available_width - ((button_count - 1) * WINDOW_SPACE_X)) / button_count;
    
    /* Ensure buttons are wide enough for text + standard padding */
    if (equal_button_width < max_btn_text_width + BUTTON_TEXT_PADDING)
        equal_button_width = max_btn_text_width + BUTTON_TEXT_PADDING;
    
    /* Button 1 */
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Action 1";
    ng.ng_GadgetID = GID_BUTTON_ACTION1;
    ng.ng_Flags = PLACETEXT_IN;
    
    button_action1 = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    /* Button 2 */
    ng.ng_LeftEdge = current_x + equal_button_width + WINDOW_SPACE_X;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Action 2";
    ng.ng_GadgetID = GID_BUTTON_ACTION2;
    ng.ng_Flags = PLACETEXT_IN;
    
    button_action2 = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    /* Button 3 (Cancel) */
    ng.ng_LeftEdge = current_x + (2 * equal_button_width) + (2 * WINDOW_SPACE_X);
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Cancel";
    ng.ng_GadgetID = GID_BUTTON_CANCEL;
    ng.ng_Flags = PLACETEXT_IN;
    
    button_cancel = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    /*--------------------------------------------------------------------*/
    /* STEP 8: Calculate Final Window Size                              */
    /*--------------------------------------------------------------------*/
    UWORD final_window_width = precalc_max_right + 
                               prefsIControl.currentLeftBarWidth + 
                               WINDOW_MARGIN_RIGHT;
    UWORD final_window_height = current_y + button_height + WINDOW_MARGIN_BOTTOM;
    
    /*--------------------------------------------------------------------*/
    /* STEP 9: Open Window                                              */
    /*--------------------------------------------------------------------*/
    *window_out = OpenWindowTags(NULL,
        WA_Left, (screen->Width - final_window_width) / 2,
        WA_Top, (screen->Height - final_window_height) / 2,
        WA_Width, final_window_width,
        WA_Height, final_window_height,
        WA_Title, "Professional Layout Example",
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, screen,
        WA_Gadgets, glist,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_GADGETUP,
        TAG_END);
    
    if (*window_out == NULL)
        goto cleanup_error;
    
    /* Refresh window */
    GT_RefreshWindow(*window_out, NULL);
    
    /* Clean up DrawInfo - no longer needed */
    FreeScreenDrawInfo(screen, draw_info);
    
    return TRUE;

cleanup_error:
    if (glist != NULL)
        FreeGadgets(glist);
    if (visual_info != NULL)
        FreeVisualInfo(visual_info);
    if (draw_info != NULL)
        FreeScreenDrawInfo(screen, draw_info);
    if (screen != NULL)
        UnlockPubScreen(NULL, screen);
    return FALSE;
}

/*------------------------------------------------------------------------*/
/* ADAPTATION GUIDE FOR iTidy STANDARD LAYOUT PATTERNS                  */
/*------------------------------------------------------------------------*/
/*
 * TO ADAPT THIS TEMPLATE FOR YOUR WINDOW:
 * 
 * 1. CHOOSE WINDOW SIZE (iTidy Standard):
 *    - Small (40 chars):  Simple dialogs, 2 buttons
 *    - Medium (60 chars): Main windows, 3 buttons (this template)
 *    - Large (80 chars):  Complex layouts, 4 buttons
 *    
 *    reference_width = font_width * WINDOW_WIDTH_[SIZE];
 * 
 * 2. DEFINE CONSTANTS:
 *    Use iTidy standard constants:
 *    - WINDOW_MARGIN_* (10 pixels)
 *    - WINDOW_SPACE_X/Y (8 pixels)
 *    - BUTTON_TEXT_PADDING (8 pixels) - ALWAYS use this for buttons
 *    - INPUT_WIDTH_* (20/35/50 chars)
 * 
 * 3. PRE-CALCULATE SECTION (Step 4):
 *    - Set reference_width based on window size
 *    - Calculate precalc_max_right = current_x + reference_width
 *    - Pre-calculate PATTERN_INPUT_ROW dimensions (if using)
 *    - Pre-calculate PATTERN_EQUAL_BUTTON_ROW dimensions
 *    - Use TextLength() for ALL text measurements
 * 
 * 4. GADGET CREATION (Steps 5-7):
 *    Follow iTidy standard patterns:
 *    - PATTERN_INPUT_ROW: Label + Input + Action button extending right
 *    - PATTERN_REFERENCE_CONTENT: Main content (usually ListView)
 *    - PATTERN_EQUAL_BUTTON_ROW: Equal-width buttons spanning full width
 * 
 * 5. BUTTON COUNT:
 *    Match button count to window size:
 *    - Small window:  2 buttons
 *    - Medium window: 3 buttons (this template)
 *    - Large window:  4 buttons
 * 
 * 6. WINDOW SIZE (Step 8):
 *    - Use precalc_max_right as width base
 *    - Add prefsIControl.currentLeftBarWidth + WINDOW_MARGIN_RIGHT
 *    - Use actual gadget heights, not requested heights
 * 
 * KEY PRINCIPLES (iTidy Standard):
 * - ALWAYS use GetScreenDrawInfo() and TextLength() for font measurements
 * - ALWAYS pre-calculate ALL dimensions before creating gadgets
 * - ALWAYS use standard constants for spacing and padding
 * - ALWAYS use actual ListView height after creation for positioning
 * - Test with different fonts to verify portability
 * 
 * PATTERN EXAMPLES:
 * 
 * PATTERN_INPUT_ROW (top of window):
 *   [Label:]  [Input Field────]  [Action Button────────]
 *   └─TEXT─┘  └─STRING/NUMBER─┘  └─Extends to ref width┘
 * 
 * PATTERN_REFERENCE_CONTENT (main content):
 *   [ListView or Main Content ──────────────────────]
 *   └─ Sets precalc_max_right for entire window ────┘
 * 
 * PATTERN_EQUAL_BUTTON_ROW (bottom):
 *   [Button 1────]  [Button 2────]  [Button 3────]
 *   └────── All equal width, span reference width ──┘
 * 
 * PORTABILITY NOTES:
 * - Works with Workbench 2.0+ (AmigaOS 2.04+)
 * - Fully font-sensitive (proportional and monospaced)
 * - Respects IControl preferences for window borders
 * - Automatically scales with different screen resolutions
 * - Follows AmigaOS 3.0 Style Guide best practices
 * 
 * For complete documentation, see:
 * - src/templates/AI_AGENT_LAYOUT_GUIDE.md (critical patterns)
 * - docs/LAYOUT_SYSTEM_OVERVIEW.md (overview and benefits)
 */
