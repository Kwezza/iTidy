/*------------------------------------------------------------------------*/
/*                                                                        *
 * amiga_window_template.c - Standalone Dynamic Amiga Window Template    *
 * Version compatible with vbcc +aos68k and Workbench 3.x                *
 *                                                                        *
 * This is a completely self-contained example showing how to create     *
 * dynamic Amiga windows with proper font-aware layout calculations.     *
 * No external dependencies - just standard Amiga OS 3.x APIs.           *
 *                                                                        */
/*------------------------------------------------------------------------*/

/* STANDALONE AMIGA WINDOW TEMPLATE
 *
 * TEMPLATE USAGE:
 * 1. Copy this template for new window creation
 * 2. Replace TEMPLATE_* constants with actual values
 * 3. Modify gadget creation sections for your needs
 * 4. Update data structure and gadget IDs
 * 5. Implement window event handling
 * 6. Test thoroughly with different font sizes and screen resolutions
 *
 * IMPORTANT LAYOUT RULES (see AI_AGENT_LAYOUT_GUIDE.md):
 * - When using PLACETEXT_ABOVE with a label, you MUST add space BEFORE
 *   positioning the gadget: current_y += font_height + 4
 * - The label space is NOT included in ng.ng_Height
 * - Account for this spacing in window height calculations
 * - Alternative: Use empty label ("") and create separate TEXT gadget
 *
 * IMPORTANT CONSOLE OUTPUT (see amiga_gui_research_3x.md):
 * - When launched from Workbench (argc==0), there is NO console
 * - Calling printf() without a console can crash the program
 * - SOLUTION: Check argc in main(), if 0 then Open("CON:...") first
 * - See test_window_template.c for the correct pattern
 * - NOTE: This template .c file contains debug printf() calls that
 *   assume the calling program has handled console setup properly
 *
 * TESTING CHECKLIST:
 * - Run with Topaz 8 font (default)
 * - Run with Topaz 9 font (common alternative)
 * - Test with larger fonts if available
 * - Check gadget positioning and spacing
 * - Verify window size accommodates all content
 * - Ensure no label overlap or truncation
 * - Test window resizing functionality
 * - Test launch from CLI (with console)
 * - Test launch from Workbench (verify console opens)
 *
 * REQUIREMENTS:
 * - AmigaOS 3.x (Workbench 3.0+)
 * - intuition.library v37+
 * - graphics.library v37+
 * - gadtools.library v37+
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <string.h>
#include <stdio.h>

#include "amiga_window_template.h"

/*------------------------------------------------------------------------*/
/* TEMPLATE CONSTANTS - Replace with actual values for your window       */
/*------------------------------------------------------------------------*/
#define TEMPLATE_WINDOW_TITLE "Dynamic Window Template"
#define TEMPLATE_MIN_BUTTON_WIDTH 80        /* Minimum button width in pixels */
#define TEMPLATE_BUTTON_TEXT_CHARS 10       /* Characters for button width calculation */
#define TEMPLATE_LISTVIEW_LINES 6           /* Number of visible lines in ListViews */
#define TEMPLATE_SPACE_X 5                  /* Horizontal spacing between gadgets */
#define TEMPLATE_SPACE_Y 5                  /* Vertical spacing between gadgets */
#define TEMPLATE_GAP_BETWEEN_COLUMNS 20     /* Gap between gadget columns */

/*------------------------------------------------------------------------*/
/* RELATIVE POSITIONING CONSTANTS FOR GADTOOLS                           */
/*------------------------------------------------------------------------*/
#define REL_LISTVIEW_WIDTH 60               /* ListView width as percentage of window */
#define REL_BUTTON_WIDTH 25                 /* Button width as percentage of window */
#define REL_STRING_WIDTH 35                 /* String gadget width as percentage */
#define REL_MARGIN_LEFT 5                   /* Left margin as percentage */
#define REL_MARGIN_TOP 10                   /* Top margin as percentage */
#define REL_MARGIN_RIGHT 5                  /* Right margin as percentage */
#define REL_MARGIN_BOTTOM 10                /* Bottom margin as percentage */

/*------------------------------------------------------------------------*/
/* TEMPLATE GADGET IDs - Replace with your actual gadget identifiers     */
/*------------------------------------------------------------------------*/
enum template_gadget_ids
{
    TEMPLATE_GADGET_FIRST = 1000,
    TEMPLATE_BUTTON_OK,
    TEMPLATE_BUTTON_CANCEL,
    TEMPLATE_LISTVIEW_MAIN,
    TEMPLATE_STRING_INPUT,
    TEMPLATE_CHECKBOX_OPTION,
    TEMPLATE_GADGET_LAST
};

/*------------------------------------------------------------------------*/
/* LAYOUT HOOK FOR CUSTOM GADGETS (OPTIONAL)                             */
/*------------------------------------------------------------------------*/

/**
 * @brief Layout hook function for custom gadgets
 *
 * This hook is called during GM_LAYOUT method processing for custom
 * gadget classes. Only needed if template includes custom gadgets.
 * For standard GadTools gadgets, this is not used.
 *
 * @param hook Pointer to the hook structure
 * @param obj Pointer to the gadget object
 * @param msg Pointer to the layout message
 * @return ULONG Always returns 0 for standard template
 */
/*------------------------------------------------------------------------*/
static ULONG LayoutHookFunc(struct Hook *hook, Object *obj, struct gpLayout *msg)
{
    /* No custom drawing here - only size mathematics if custom gadget created */
    /* For standard GadTools template, this hook is not used */
    return 0;
} /* LayoutHookFunc */

/*------------------------------------------------------------------------*/
/**
 * @brief Updates maximum window dimensions based on gadget position and size
 *
 * This function tracks the rightmost and bottommost edges of gadgets to
 * determine the minimum window size needed to contain all gadgets with
 * proper spacing and borders.
 *
 * @param current_max_width Current maximum width tracked so far
 * @param current_max_height Current maximum height tracked so far
 * @param gadget_right_edge Right edge of current gadget (left + width)
 * @param gadget_bottom_edge Bottom edge of current gadget (top + height)
 * @param new_max_width Pointer to store updated maximum width
 * @param new_max_height Pointer to store updated maximum height
 */
/*------------------------------------------------------------------------*/
static void update_window_max_dimensions(UWORD current_max_width, UWORD current_max_height, UWORD gadget_right_edge, UWORD gadget_bottom_edge, UWORD *new_max_width, UWORD *new_max_height)
{
    /* Update maximum width if gadget extends further right */
    if (current_max_width < gadget_right_edge)
    {
        *new_max_width = gadget_right_edge;
    } /* if */
    else
    {
        *new_max_width = current_max_width;
    } /* else */

    /* Update maximum height if gadget extends further down */
    if (current_max_height < gadget_bottom_edge)
    {
        *new_max_height = gadget_bottom_edge;
    } /* if */
    else
    {
        *new_max_height = current_max_height;
    } /* else */
} /* update_window_max_dimensions */

/*------------------------------------------------------------------------*/
/**
 * @brief Calculate font-based dimensions for gadgets
 *
 * This function calculates appropriate gadget sizes based on the active
 * Workbench font. This ensures consistent appearance across different
 * font sizes and provides proper scaling.
 *
 * @param screen Pointer to the public screen
 * @param font_dims Pointer to structure to store calculated dimensions
 */
/*------------------------------------------------------------------------*/
static void calculate_font_dimensions(struct Screen *screen, struct FontDimensions *font_dims)
{
    /* Get font metrics from screen's RastPort */
    font_dims->font_width = screen->RastPort.TxWidth;
    font_dims->font_height = screen->RastPort.TxHeight;

    /* Calculate standard gadget dimensions based on font */
    font_dims->button_height = font_dims->font_height + 4;
    font_dims->button_width = font_dims->font_width * TEMPLATE_BUTTON_TEXT_CHARS + 8;
    font_dims->listview_height = font_dims->font_height * TEMPLATE_LISTVIEW_LINES + 4;
    font_dims->string_height = font_dims->font_height + 6;

    /* Ensure minimum button width */
    if (font_dims->button_width < TEMPLATE_MIN_BUTTON_WIDTH)
    {
        font_dims->button_width = TEMPLATE_MIN_BUTTON_WIDTH;
    } /* if */

    /* Calculate window title bar and border offsets */
    /* CRITICAL: Based on expert advice and RKM documentation:
     * - GadTools gadgets (NewGadget) are positioned relative to the WINDOW's
     *   top-left corner, where (0,0) is the outer corner INCLUDING the title bar.
     * - To position gadgets BELOW the title bar, we MUST offset by BorderTop.
     * - We can calculate BorderTop BEFORE opening the window using the formula:
     *   BorderTop = screen->WBorTop + screen->Font->ta_YSize + 1
     * 
     * This is the standard Amiga pattern documented in the RKMs and used throughout
     * all classic Amiga software including iTidy's own windows.
     */
    font_dims->title_bar_height = screen->WBorTop + screen->Font->ta_YSize + 1;
    font_dims->window_top_edge = font_dims->title_bar_height + TEMPLATE_SPACE_Y;
    font_dims->window_left_edge = screen->WBorLeft + TEMPLATE_SPACE_X;
    font_dims->window_bottom_edge = screen->WBorBottom + TEMPLATE_SPACE_Y;
    
    printf("DEBUG: === SCREEN CHROME VALUES ===\n");
    printf("DEBUG: Screen WBorTop=%d, WBorBottom=%d, WBorLeft=%d, WBorRight=%d\n",
           screen->WBorTop, screen->WBorBottom, screen->WBorLeft, screen->WBorRight);
    printf("DEBUG: Screen Font->ta_YSize=%d (font height)\n", (int)screen->Font->ta_YSize);
    printf("DEBUG: Screen BarHeight=%d (Workbench screen title bar)\n", screen->BarHeight);
    printf("DEBUG: Calculated BorderTop (WBorTop + ta_YSize + 1) = %d + %d + 1 = %u\n",
           (int)screen->WBorTop, (int)screen->Font->ta_YSize, (unsigned int)font_dims->title_bar_height);
    printf("DEBUG: Font dimensions - font_height=%u, font_width=%u\n",
           (unsigned int)font_dims->font_height, (unsigned int)font_dims->font_width);
    printf("DEBUG: Gadget positioning (window-relative) - top_offset=%u, left_offset=%u\n",
           (unsigned int)font_dims->window_top_edge, (unsigned int)font_dims->window_left_edge);
    printf("DEBUG: === END CHROME VALUES ===\n");
} /* calculate_font_dimensions */

/*------------------------------------------------------------------------*/
/**
 * @brief Populate ListView with test data items
 *
 * This function creates and adds 20 test items to the ListView for testing
 * purposes. Each item contains sample text and is properly linked into the list.
 * Memory is allocated using AllocVec() and must be freed with FreeVec().
 *
 * @param data Pointer to window data structure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
/*------------------------------------------------------------------------*/
BOOL populate_listview_test_data(struct TemplateWindowData *data)
{
    struct ListViewItem *item;      /* Current item being created */
    UWORD i;                        /* Loop counter */
    static STRPTR test_items[] = {  /* Array of test item text */
        "Item 01: Sample ListView Entry",
        "Item 02: Font-Aware Display Test",
        "Item 03: Dynamic Window Template",
        "Item 04: Amiga Development Tool",
        "Item 05: GadTools ListView Demo",
        "Item 06: ANSI C Compatible Code",
        "Item 07: SAS/C 6 Compilation",
        "Item 08: Workbench 3.x Support",
        "Item 09: NDK 3.2 API Usage",
        "Item 10: Memory Management Test",
        "Item 11: Selectable List Items",
        "Item 12: Proper Event Handling",
        "Item 13: Font Dimension Scaling",
        "Item 14: Layout Debug Verification",
        "Item 15: Window Resize Support",
        "Item 16: Gadget Positioning Test",
        "Item 17: User Interface Element",
        "Item 18: Interactive Selection",
        "Item 19: Template Customization",
        "Item 20: Final Test Entry"
    };

    /* Validate input parameters */
    if (data == NULL)
    {
        return FALSE;
    } /* if */

    printf("DEBUG: Populating ListView with 20 test items...\n");

    /* Create and add each test item to the list */
    for (i = 0; i < 20; i++)
    {
        /* Allocate memory for the ListView item using AllocVec */
        item = (struct ListViewItem *)AllocVec(sizeof(struct ListViewItem), MEMF_CLEAR);
        if (item == NULL)
        {
            return FALSE;
        } /* if */

        /* Allocate memory for the text string using AllocVec */
        item->text = (STRPTR)AllocVec(strlen(test_items[i]) + 1, MEMF_CLEAR);
        if (item->text == NULL)
        {
            FreeVec(item);
            return FALSE;
        } /* if */

        /* Copy the text string */
        strcpy(item->text, test_items[i]);

        /* Set the node name to point to our text (required for GadTools ListView) */
        item->node.ln_Name = item->text;
        item->node.ln_Type = NT_USER;
        item->node.ln_Pri = 0;

        /* Store item index as user data */
        item->user_data = i;

        /* Add the item to the end of the list */
        AddTail(&data->main_list, (struct Node *)item);

        printf("DEBUG: Added item %u: '%s'\n", (unsigned int)(i + 1), item->text);
    } /* for */

    printf("DEBUG: Successfully populated ListView with %u test items\n", (unsigned int)i);

    return TRUE;
} /* populate_listview_test_data */

/*------------------------------------------------------------------------*/
/**
 * @brief Free all ListView items and clear the list
 *
 * This function properly frees all ListView items and clears the list
 * using FreeVec(). Call this before closing the window to prevent memory leaks.
 *
 * @param data Pointer to window data structure
 */
/*------------------------------------------------------------------------*/
void free_listview_items(struct TemplateWindowData *data)
{
    struct ListViewItem *item;      /* Current item being freed */
    struct ListViewItem *next_item; /* Next item in the list */

    /* Validate input parameters */
    if (data == NULL)
    {
        return;
    } /* if */

    printf("DEBUG: Freeing ListView items...\n");

    /* Walk through the list and free each item */
    item = (struct ListViewItem *)data->main_list.lh_Head;
    while (item->node.ln_Succ != NULL)
    {
        next_item = (struct ListViewItem *)item->node.ln_Succ;

        /* Remove the item from the list */
        Remove((struct Node *)item);

        /* Free the text string using FreeVec */
        if (item->text != NULL)
        {
            FreeVec(item->text);
        } /* if */

        /* Free the item structure using FreeVec */
        FreeVec(item);

        item = next_item;
    } /* while */

    /* Reinitialize the list */
    data->main_list.lh_Head = (struct Node *)&data->main_list.lh_Tail;
    data->main_list.lh_Tail = NULL;
    data->main_list.lh_TailPred = (struct Node *)&data->main_list.lh_Head;

    printf("DEBUG: ListView items freed successfully\n");
} /* free_listview_items */

/*------------------------------------------------------------------------*/
/**
 * @brief Print debug information about actual gadget positions and sizes
 *
 * This function queries each gadget's actual position and size using
 * direct gadget structure access and prints the results for debugging
 * layout issues. This is especially important for ListView gadgets which
 * "snap" their height to show complete rows.
 *
 * @param data Pointer to window data structure
 */
/*------------------------------------------------------------------------*/
static void debug_print_gadget_positions(struct TemplateWindowData *data)
{
    if (data == NULL || data->window == NULL)
    {
        printf("DEBUG: Cannot print gadget positions - invalid data or window\n");
        return;
    } /* if */

    printf("DEBUG: === ACTUAL GADGET POSITIONS AND SIZES ===\n");
    printf("DEBUG: Window size: %d x %d\n", data->window->Width, data->window->Height);
    printf("DEBUG: Window borders: Left=%d, Right=%d, Top=%d, Bottom=%d\n",
           data->window->BorderLeft, data->window->BorderRight,
           data->window->BorderTop, data->window->BorderBottom);
    printf("DEBUG: Window inner size: %d x %d\n",
           data->window->Width - data->window->BorderLeft - data->window->BorderRight,
           data->window->Height - data->window->BorderTop - data->window->BorderBottom);

    /* Print ListView gadget info - access Gadget structure directly */
    if (data->main_listview != NULL)
    {
        printf("DEBUG: ListView at (%d, %d) size %d x %d\n",
               data->main_listview->LeftEdge, data->main_listview->TopEdge,
               data->main_listview->Width, data->main_listview->Height);
    } /* if */

    /* Print String gadget info - access Gadget structure directly */
    if (data->string_input != NULL)
    {
        printf("DEBUG: String gadget at (%d, %d) size %d x %d\n",
               data->string_input->LeftEdge, data->string_input->TopEdge,
               data->string_input->Width, data->string_input->Height);
    } /* if */

    /* Print Checkbox gadget info - access Gadget structure directly */
    if (data->checkbox_option != NULL)
    {
        printf("DEBUG: Checkbox at (%d, %d) size %d x %d\n",
               data->checkbox_option->LeftEdge, data->checkbox_option->TopEdge,
               data->checkbox_option->Width, data->checkbox_option->Height);
    } /* if */

    /* Print OK Button info - access Gadget structure directly */
    if (data->ok_button != NULL)
    {
        printf("DEBUG: OK Button at (%d, %d) size %d x %d\n",
               data->ok_button->LeftEdge, data->ok_button->TopEdge,
               data->ok_button->Width, data->ok_button->Height);
    } /* if */

    /* Print Cancel Button info - access Gadget structure directly */
    if (data->cancel_button != NULL)
    {
        printf("DEBUG: Cancel Button at (%d, %d) size %d x %d\n",
               data->cancel_button->LeftEdge, data->cancel_button->TopEdge,
               data->cancel_button->Width, data->cancel_button->Height);
    } /* if */

    printf("DEBUG: === END GADGET POSITIONS ===\n");
} /* debug_print_gadget_positions */

/*------------------------------------------------------------------------*/
/**
 * @brief Create gadgets for the template window using proper positioning
 *
 * This function creates all gadgets for the window using a combination of
 * absolute positioning and relative sizing. The ListView is created first
 * and its actual height is discovered for positioning subsequent gadgets.
 *
 * @param data Pointer to window data structure
 * @param font_dims Pointer to calculated font dimensions
 * @return BOOL TRUE if successful, FALSE otherwise
 */
/*------------------------------------------------------------------------*/
static BOOL create_template_gadgets(struct TemplateWindowData *data, struct FontDimensions *font_dims)
{
    struct NewGadget ng;        /* NewGadget structure for gadget creation */
    struct Gadget *gad;         /* Current gadget being created */
    UWORD listview_actual_height = 0;  /* Actual ListView height after creation */
    UWORD current_x;            /* Current X position for gadget placement */
    UWORD current_y;            /* Current Y position for gadget placement */

    /* Initialize gadget creation context */
    gad = CreateContext(&data->glist);
    if (gad == NULL)
    {
        return FALSE;
    } /* if */

    printf("DEBUG: Gadget context created successfully\n");

    /* Initialize common NewGadget fields */
    ng.ng_TextAttr = NULL;
    ng.ng_VisualInfo = data->visual_info;

    /* Set initial positions based on font dimensions */
    current_x = font_dims->window_left_edge;
    current_y = font_dims->window_top_edge;

    /*====================================================================*/
    /* CREATE LISTVIEW FIRST - MUST BE FIRST FOR HEIGHT DISCOVERY        */
    /*====================================================================*/

    /* IMPORTANT: Following AI_AGENT_LAYOUT_GUIDE.md recommendations:
     * When using PLACETEXT_ABOVE with a label, you MUST add space for the label
     * before positioning the ListView, otherwise the label appears cramped against
     * the window border. The label space is NOT included in ng.ng_Height.
     * 
     * Space needed = font_height + small_gap (typically 4 pixels)
     */
    
    /* Add space for the "List" label that will appear above the ListView */
    current_y += font_dims->font_height + 4;  /* Label height + gap */

    /* Main ListView - positioned at left side of window */
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = font_dims->button_width * 2;  /* Make ListView wider */
    ng.ng_Height = font_dims->listview_height;   /* This is just a REQUEST */
    ng.ng_GadgetText = "List";
    ng.ng_GadgetID = TEMPLATE_LISTVIEW_MAIN;
    ng.ng_Flags = PLACETEXT_ABOVE;

    data->main_listview = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
                                           GTLV_Labels, &data->main_list,
                                           GTLV_ShowSelected, NULL,
                                           TAG_END);
    if (gad == NULL)
    {
        return FALSE;
    } /* if */

    /* CRITICAL: Get the actual ListView height after creation */
    listview_actual_height = data->main_listview->Height;

    printf("DEBUG: ListView requested height: %u, actual height: %u\n",
           (unsigned int)ng.ng_Height, (unsigned int)listview_actual_height);

    /* Calculate position for gadgets in the right column */
    current_x = font_dims->window_left_edge + ng.ng_Width + TEMPLATE_GAP_BETWEEN_COLUMNS;
    current_y = font_dims->window_top_edge;

    /*====================================================================*/
    /* CREATE STRING GADGETS USING PROPER POSITIONING                    */
    /*====================================================================*/

    /* String Input Gadget - positioned to the right of ListView */
    /* Calculate label width and adjust position to prevent overlap */
    {
        STRPTR string_label = "Input:";
        UWORD label_width = strlen(string_label) * font_dims->font_width;  /* Calculate actual label width */
        UWORD label_spacing = 4;  /* Small gap between label and gadget */

        /* CRITICAL: Position gadget AFTER the label, not at the base position */
        ng.ng_LeftEdge = current_x + label_width + label_spacing;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = font_dims->button_width;
        ng.ng_Height = font_dims->string_height;
        ng.ng_GadgetText = string_label;
        ng.ng_GadgetID = TEMPLATE_STRING_INPUT;
        ng.ng_Flags = PLACETEXT_LEFT;  /* Label will be drawn to the LEFT of ng.ng_LeftEdge */

        data->string_input = gad = CreateGadget(STRING_KIND, gad, &ng,
                                              GTST_MaxChars, 64,
                                              GTST_String, "",
                                              TAG_END);
        if (gad == NULL)
        {
            return FALSE;
        } /* if */
    } /* block */

    /* Move to next row for checkbox */
    current_y += ng.ng_Height + TEMPLATE_SPACE_Y;

    /*====================================================================*/
    /* CREATE CHECKBOX GADGETS USING PROPER POSITIONING                  */
    /*====================================================================*/

    /* Checkbox Option - positioned below string input */
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = font_dims->button_height;     /* Square checkbox */
    ng.ng_Height = font_dims->button_height;
    ng.ng_GadgetText = "Enable Option";
    ng.ng_GadgetID = TEMPLATE_CHECKBOX_OPTION;
    ng.ng_Flags = PLACETEXT_RIGHT;

    data->checkbox_option = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
                                             GTCB_Checked, FALSE,
                                             TAG_END);
    if (gad == NULL)
    {
        return FALSE;
    } /* if */

    /*====================================================================*/
    /* CREATE BUTTON GADGETS AT BOTTOM OF WINDOW                         */
    /*====================================================================*/

    /* Calculate button row position - ensure buttons are below ListView */
    /* Use the actual ListView gadget dimensions after it's created */
    {
        UWORD listview_start_y = data->main_listview->TopEdge;               /* Actual position */
        UWORD listview_actual_height_from_gadget = data->main_listview->Height;  /* Actual height */
        UWORD listview_bottom = listview_start_y + listview_actual_height_from_gadget;
        current_y = listview_bottom + TEMPLATE_SPACE_Y * 2;
        current_x = font_dims->window_left_edge;

        printf("DEBUG: ListView at Y=%u, height=%u, bottom=%u, buttons at Y=%u\n",
               (unsigned int)listview_start_y, (unsigned int)listview_actual_height_from_gadget,
               (unsigned int)listview_bottom, (unsigned int)current_y);
    } /* block */

    /* OK Button - positioned at bottom left */
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = font_dims->button_width;
    ng.ng_Height = font_dims->button_height;
    ng.ng_GadgetText = "OK";
    ng.ng_GadgetID = TEMPLATE_BUTTON_OK;
    ng.ng_Flags = PLACETEXT_IN;

    data->ok_button = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        return FALSE;
    } /* if */

    /* Cancel Button - positioned next to OK button */
    current_x += ng.ng_Width + TEMPLATE_SPACE_X;

    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = font_dims->button_width;
    ng.ng_Height = font_dims->button_height;
    ng.ng_GadgetText = "Cancel";
    ng.ng_GadgetID = TEMPLATE_BUTTON_CANCEL;
    ng.ng_Flags = PLACETEXT_IN;

    data->cancel_button = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        return FALSE;
    } /* if */

    printf("DEBUG: All gadgets created successfully with proper positioning\n");

    return TRUE;
} /* create_template_gadgets */

/*------------------------------------------------------------------------*/
/**
 * @brief Creates a dynamically-sized Amiga window using font-based calculations
 *
 * This is the main template function that demonstrates the complete process
 * of creating an Amiga window that automatically adjusts its size based on
 * the active Workbench font. The window will be properly sized to contain
 * all gadgets with appropriate spacing.
 *
 * @param data Pointer to window data structure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
/*------------------------------------------------------------------------*/
BOOL create_template_window(struct TemplateWindowData *data)
{
    struct FontDimensions font_dims;    /* Calculated font-based dimensions */
    UWORD window_width;                 /* Calculated window width */
    UWORD window_height;                /* Calculated window height */

    /* Validate input parameters */
    if (data == NULL || data->screen == NULL)
    {
        return FALSE;
    } /* if */

    /* Calculate font-based dimensions */
    calculate_font_dimensions(data->screen, &font_dims);

    /* Get visual info from screen BEFORE creating gadgets */
    data->visual_info = GetVisualInfo(data->screen, TAG_END);
    if (data->visual_info == NULL)
    {
        return FALSE;
    } /* if */

    /* Initialize data lists for gadgets */
    printf("DEBUG: Initializing main_list...\n");
    /* Manual NewList implementation for compatibility */
    data->main_list.lh_Head = (struct Node *)&data->main_list.lh_Tail;
    data->main_list.lh_Tail = NULL;
    data->main_list.lh_TailPred = (struct Node *)&data->main_list.lh_Head;
    printf("DEBUG: main_list initialized\n");

    /* Populate ListView with test data */
    if (!populate_listview_test_data(data))
    {
        return FALSE;
    } /* if */

    /* Create all gadgets using proper positioning */
    if (!create_template_gadgets(data, &font_dims))
    {
        return FALSE;
    } /* if */

    /* Calculate proper window dimensions based on actual gadget layout */

    /* Get the actual ListView height from the gadget structure */
    {
        UWORD actual_listview_height = 0;
        if (data->main_listview != NULL)
        {
            actual_listview_height = data->main_listview->Height;  /* ACTUAL height, not requested */
        } /* if */

        /* Calculate width: ListView width + gap + right column + margins + label */
        /* CRITICAL: Include label width for PLACETEXT_LEFT gadgets to prevent truncation */
        {
            UWORD string_label_width = strlen("Input:") * font_dims.font_width + 4; /* Label + spacing */
            window_width = (font_dims.button_width * 2) +           /* ListView width */
                           TEMPLATE_GAP_BETWEEN_COLUMNS +           /* Gap between columns */
                           string_label_width +                     /* String label width (ESSENTIAL!) */
                           font_dims.button_width +                 /* String gadget width */
                           (font_dims.window_left_edge * 2) +       /* Left and right margins */
                           20;                                       /* Extra padding */
        } /* block */

        /* Calculate height: Button position + button height + margins + borders */
        /* CRITICAL: Now we can calculate the EXACT window height using the pre-calculated
         * BorderTop value (screen->WBorTop + screen->Font->ta_YSize + 1) and BorderBottom.
         * No more guessing! */
        {
            UWORD client_content_height = data->ok_button->TopEdge + font_dims.button_height + TEMPLATE_SPACE_Y;
            UWORD exact_border_top = font_dims.title_bar_height;
            UWORD exact_border_bottom = data->screen->WBorBottom;
            window_height = client_content_height + exact_border_top + exact_border_bottom;
            
            printf("DEBUG: Window height calculation: client_content=%u + BorderTop=%u + BorderBottom=%u = total=%u\n",
                   (unsigned int)client_content_height, (unsigned int)exact_border_top, 
                   (unsigned int)exact_border_bottom, (unsigned int)window_height);
        }

        printf("DEBUG: Calculated window size: %ux%u (ListView actual height: %u)\n",
               (unsigned int)window_width, (unsigned int)window_height, (unsigned int)actual_listview_height);
    } /* block */

    /* Create menu strip if needed - TEMPLATE: Replace with your menu */
    data->menu = NULL;  /* Replace with actual menu creation */

    /* Open the window with calculated dimensions */
    /* Set min/max to same values to prevent resizing */
    data->window = OpenWindowTags(NULL,
                                  WA_Left, 50,
                                  WA_Top, 50,
                                  WA_Width, window_width,
                                  WA_Height, window_height,
                                  WA_MinWidth, window_width,
                                  WA_MinHeight, window_height,
                                  WA_MaxWidth, window_width,
                                  WA_MaxHeight, window_height,
                                  WA_Title, data->window_title ? data->window_title : TEMPLATE_WINDOW_TITLE,
                                  WA_DragBar, TRUE,
                                  WA_DepthGadget, TRUE,
                                  WA_CloseGadget, TRUE,
                                  WA_SizeGadget, FALSE,
                                  WA_Activate, TRUE,
                                  WA_PubScreen, data->screen,
                                  WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_MENUPICK,
                                  TAG_END);

    if (data->window == NULL)
    {
        return FALSE;
    } /* if */

    /* IMPORTANT: Now we have the ACTUAL window borders from Intuition! */
    printf("DEBUG: *** ACTUAL WINDOW BORDERS (from Intuition) ***\n");
    printf("DEBUG: window->BorderTop = %d (actual title bar height)\n", data->window->BorderTop);
    printf("DEBUG: window->BorderBottom = %d\n", data->window->BorderBottom);
    printf("DEBUG: window->BorderLeft = %d\n", data->window->BorderLeft);
    printf("DEBUG: window->BorderRight = %d\n", data->window->BorderRight);
    printf("DEBUG: Our estimate was %u, actual is %d (difference: %d)\n",
           (unsigned int)font_dims.title_bar_height, data->window->BorderTop,
           (int)data->window->BorderTop - (int)font_dims.title_bar_height);
    printf("DEBUG: *** END ACTUAL BORDERS ***\n");

    /* Set the window's menu strip if available */
    if (data->menu != NULL)
    {
        SetMenuStrip(data->window, data->menu);
    } /* if */

    /* Add gadgets to window */
    AddGList(data->window, data->glist, (UWORD)~0, (UWORD)~0, NULL);

    /* Refresh gadgets to make them visible */
    RefreshGList(data->glist, data->window, NULL, (UWORD)~0);

    /* Final window refresh to ensure everything is drawn */
    GT_RefreshWindow(data->window, NULL);

    /* Print debug information about actual gadget positions */
    debug_print_gadget_positions(data);

    /* Mark window as successfully opened */
    data->window_open = TRUE;

    return TRUE;
} /* create_template_window */

/*------------------------------------------------------------------------*/
/**
 * @brief Clean up and close the template window
 *
 * This function properly cleans up all resources associated with the window
 * including gadgets, menus, visual info, and the window itself.
 *
 * @param data Pointer to window data structure
 */
/*------------------------------------------------------------------------*/
void close_template_window(struct TemplateWindowData *data)
{
    if (data == NULL)
    {
        return;
    } /* if */

    /* Detach ListView labels BEFORE closing window or freeing list data */
    if (data->window != NULL && data->main_listview != NULL)
    {
        GT_SetGadgetAttrs(data->main_listview, data->window, NULL,
                          GTLV_Labels, ~0UL,
                          TAG_END);
    }

    /* Close window if it's open */
    if (data->window != NULL)
    {
        /* Clear menu strip before closing */
        if (data->menu != NULL)
        {
            ClearMenuStrip(data->window);
        } /* if */

        /* Close the window */
        CloseWindow(data->window);
        data->window = NULL;
        data->window_open = FALSE;
    } /* if */

    /* Free ListView items */
    free_listview_items(data);

    /* Free gadget list */
    if (data->glist != NULL)
    {
        FreeGadgets(data->glist);
        data->glist = NULL;
    } /* if */

    /* Free menu strip */
    if (data->menu != NULL)
    {
        FreeMenus(data->menu);
        data->menu = NULL;
    } /* if */

    /* Free visual info */
    if (data->visual_info != NULL)
    {
        FreeVisualInfo(data->visual_info);
        data->visual_info = NULL;
    } /* if */
} /* close_template_window */

/*------------------------------------------------------------------------*/
/* TEMPLATE EVENT HANDLING - Replace with your actual event processing   */
/*------------------------------------------------------------------------*/

/**
 * @brief Handle gadget events for the template window
 *
 * This function processes gadget clicks and other events. Replace the
 * switch statement cases with your actual gadget IDs and actions.
 *
 * @param data Pointer to window data structure
 * @param gadget_id ID of the activated gadget
 * @return BOOL TRUE to continue, FALSE to close window
 */
BOOL handle_template_gadget_event(struct TemplateWindowData *data, UWORD gadget_id)
{
    switch (gadget_id)
    {
        case TEMPLATE_BUTTON_OK:
            /* Add your OK button action here */
            return FALSE;  /* Close window */

        case TEMPLATE_BUTTON_CANCEL:
            /* Add your Cancel button action here */
            return FALSE;  /* Close window */

        case TEMPLATE_LISTVIEW_MAIN:
            /* Enhanced ListView selection handling with test data */
            {
                ULONG selected_index = ~0;  /* Selected item index */
                struct ListViewItem *selected_item = NULL;  /* Selected item pointer */
                struct Node *node;          /* Node for list traversal */
                ULONG count = 0;           /* Counter for finding selected item */

                /* Get the selected item index */
                GT_GetGadgetAttrs(data->main_listview, data->window, NULL,
                                  GTLV_Selected, &selected_index,
                                  TAG_END);

                if (selected_index != ~0)
                {
                    printf("DEBUG: ListView item %lu selected\n", (unsigned long)selected_index);

                    /* Find the selected item in our list */
                    for (node = data->main_list.lh_Head;
                         node->ln_Succ != NULL;
                         node = node->ln_Succ)
                    {
                        if (count == selected_index)
                        {
                            selected_item = (struct ListViewItem *)node;
                            break;
                        } /* if */
                        count++;
                    } /* for */

                    if (selected_item != NULL)
                    {
                        printf("DEBUG: Selected item text: '%s'\n", selected_item->text);
                        printf("DEBUG: Selected item user_data: %lu\n", (unsigned long)selected_item->user_data);

                        /* Update string gadget with selected item text */
                        GT_SetGadgetAttrs(data->string_input, data->window, NULL,
                                          GTST_String, selected_item->text,
                                          TAG_END);

                        /* Refresh the string gadget to show the new text */
                        RefreshGList(data->string_input, data->window, NULL, 1);
                    } /* if */
                    else
                    {
                        printf("DEBUG: Could not find selected item in list\n");
                    } /* else */
                } /* if */
                else
                {
                    printf("DEBUG: No ListView item selected\n");
                } /* else */
            } /* block */
            break;

        case TEMPLATE_STRING_INPUT:
            /* Add your string input handling here */
            break;

        case TEMPLATE_CHECKBOX_OPTION:
            /* Add your checkbox handling here */
            break;

        default:
            printf("DEBUG: Unknown gadget ID: %u\n", (unsigned int)gadget_id);
            break;
    } /* switch */

    return TRUE;  /* Continue processing */
} /* handle_template_gadget_event */

/*------------------------------------------------------------------------*/
/**
 * @brief Handle window resize events (IDCMP_NEWSIZE)
 *
 * This function handles window resize events by refreshing the gadget layout.
 * With relative positioning using GFLG_REL* flags, most gadgets will
 * automatically adjust their positions during window resize.
 *
 * @param data Pointer to window data structure
 * @return BOOL TRUE if resize handled successfully, FALSE otherwise
 */
/*------------------------------------------------------------------------*/
BOOL handle_template_window_resize(struct TemplateWindowData *data)
{
    UWORD listview_actual_height = 0;   /* Current actual ListView height */

    /* Validate parameters */
    if (data == NULL || data->window == NULL || data->main_listview == NULL)
    {
        return FALSE;
    } /* if */

    printf("DEBUG: Handling window resize event\n");

    /* Get the ListView's current actual height after window resize */
    GT_GetGadgetAttrs(data->main_listview, data->window, NULL,
                      GA_Height, &listview_actual_height,
                      TAG_DONE);

    printf("DEBUG: ListView actual height after resize: %u\n", (unsigned int)listview_actual_height);

    /* Simple refresh approach - just refresh all gadgets */
    RefreshGList(data->glist, data->window, NULL, (UWORD)~0);
    GT_RefreshWindow(data->window, NULL);

    printf("DEBUG: Window resize handling completed successfully\n");

    return TRUE;
} /* handle_template_window_resize */

/*------------------------------------------------------------------------*/
/* MAIN - Simple test program to demonstrate template usage              */
/*------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    struct TemplateWindowData window_data;
    struct IntuiMessage *imsg;
    ULONG signals;
    BOOL running = TRUE;

    /* Initialize window data structure */
    memset(&window_data, 0, sizeof(struct TemplateWindowData));

    /* Lock the Workbench screen */
    window_data.screen = LockPubScreen("Workbench");
    if (window_data.screen == NULL)
    {
        printf("ERROR: Cannot lock Workbench screen\n");
        return 20;
    }

    /* Create the template window */
    if (!create_template_window(&window_data))
    {
        printf("Failed to create template window\n");
        UnlockPubScreen(NULL, window_data.screen);
        return 20;
    }

    printf("Template window created successfully. Close window to exit.\n");

    /* Main event loop */
    while (running)
    {
        signals = Wait((1L << window_data.window->UserPort->mp_SigBit) | SIGBREAKF_CTRL_C);

        if (signals & SIGBREAKF_CTRL_C)
        {
            running = FALSE;
            continue;
        }

        while ((imsg = GT_GetIMsg(window_data.window->UserPort)))
        {
            ULONG class = imsg->Class;
            UWORD code = imsg->Code;
            struct Gadget *gad = (struct Gadget *)imsg->IAddress;

            GT_ReplyIMsg(imsg);

            switch (class)
            {
                case IDCMP_CLOSEWINDOW:
                    running = FALSE;
                    break;

                case IDCMP_GADGETUP:
                    if (gad)
                    {
                        handle_template_gadget_event(&window_data, gad->GadgetID);
                    }
                    break;

                case IDCMP_NEWSIZE:
                    handle_template_window_resize(&window_data);
                    break;

                case IDCMP_REFRESHWINDOW:
                    GT_BeginRefresh(window_data.window);
                    GT_EndRefresh(window_data.window, TRUE);
                    break;
            }
        }
    }

    /* Cleanup */
    close_template_window(&window_data);
    
    /* Unlock the screen */
    if (window_data.screen)
    {
        UnlockPubScreen(NULL, window_data.screen);
    }
    
    printf("Template window closed.\n");

    return 0;
} /* main */

/* End of Text */
