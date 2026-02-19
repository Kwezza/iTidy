/*------------------------------------------------------------------------*/
/*                                                                        *
 * amiga_window_resize_template.c - Resizable GadTools Window Template   *
 * Version compatible with vbcc +aos68k and Workbench 3.0/3.1            *
 *                                                                        *
 * CRITICAL REFERENCES (must read before modifying):                     *
 * - Resizable GadTools Forms on Amiga Workbench 3.0.txt                 *
 *   (Ultimate source of truth for layout and resizing)                  *
 * - Clearing Window Contents Before Gadget Re-Add in GadTools.txt       *
 *   (Ultimate source of truth for clean redraws, no ghost gadgets)      *
 *                                                                        *
 * This template demonstrates:                                           *
 * - Correct resizable GadTools window on WB 3.0/3.1 (no RELWIDTH flags) *
 * - Manual gadget anchoring (ListView stretches, buttons anchor bottom) *
 * - Flicker-free resize using IDCMP_SIZEVERIFY/NEWSIZE                  *
 * - Proper clearing and refresh (no gadget trails or artifacts)         *
 * - Console logging for geometry and events                             *
 * - Standard Workbench screen window (no GimmeZeroZero)                 *
 *                                                                        */
/*------------------------------------------------------------------------*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>

/* Macro for NewList if not defined */
#ifndef NewList
#define NewList(l) \
    do { \
        struct List *_list = (l); \
        _list->lh_Head = (struct Node *)&_list->lh_Tail; \
        _list->lh_Tail = NULL; \
        _list->lh_TailPred = (struct Node *)&_list->lh_Head; \
    } while(0)
#endif

/*------------------------------------------------------------------------*/
/* Constants                                                              */
/*------------------------------------------------------------------------*/
#define ITIDY_RESIZE_WINDOW_TITLE "Resizable GadTools Template"

/* Margins and spacing (pixels) */
#define ITIDY_RESIZE_MARGIN_LEFT     10
#define ITIDY_RESIZE_MARGIN_TOP      10
#define ITIDY_RESIZE_MARGIN_RIGHT    10
#define ITIDY_RESIZE_MARGIN_BOTTOM   10
#define ITIDY_RESIZE_SPACE_X         8
#define ITIDY_RESIZE_SPACE_Y         8

/* Initial window dimensions (inner content area) */
#define ITIDY_RESIZE_INITIAL_INNER_WIDTH   500
#define ITIDY_RESIZE_INITIAL_INNER_HEIGHT  300

/* Minimum window dimensions (inner content area) */
#define ITIDY_RESIZE_MIN_INNER_WIDTH   175
#define ITIDY_RESIZE_MIN_INNER_HEIGHT  200

/* Button row height (including spacing) */
#define ITIDY_RESIZE_BUTTON_ROW_HEIGHT   30


/*------------------------------------------------------------------------*/
/* Gadget IDs                                                             */
/*------------------------------------------------------------------------*/
enum iTidy_ResizeGadgetIDs
{
    ITIDY_RESIZE_GID_LISTVIEW = 1,
    ITIDY_RESIZE_GID_OK,
    ITIDY_RESIZE_GID_CANCEL
};

/*------------------------------------------------------------------------*/
/* ListView Item Structure                                                */
/*------------------------------------------------------------------------*/
struct iTidy_ResizeListViewItem
{
    struct Node node;   /* Standard Exec node for list linkage */
    STRPTR text;        /* Item text (dynamically allocated) */
    ULONG user_data;    /* User-defined data */
};

/*------------------------------------------------------------------------*/
/* Gadget Anchor Information                                              */
/* Used to recompute gadget positions/sizes on resize                    */
/*------------------------------------------------------------------------*/
struct iTidy_ResizeGadgetAnchor
{
    UWORD left_margin;    /* Distance from left edge of inner area */
    UWORD top_margin;     /* Distance from top edge of inner area */
    UWORD right_margin;   /* Distance from right edge of inner area */
    UWORD bottom_margin;  /* Distance from bottom edge of inner area */
    BOOL  stretch_h;      /* TRUE if gadget stretches horizontally */
    BOOL  stretch_v;      /* TRUE if gadget stretches vertically */
    BOOL  anchor_right;   /* TRUE if anchored to right edge */
    BOOL  anchor_bottom;  /* TRUE if anchored to bottom edge */
};

/*------------------------------------------------------------------------*/
/* Window Data Structure                                                  */
/*------------------------------------------------------------------------*/
struct iTidy_ResizeWindowData
{
    /* Intuition/GadTools resources */
    struct Screen *screen;
    struct Window *window;
    APTR visual_info;
    struct Gadget *glist;
    
    /* Individual gadgets */
    struct Gadget *listview_gad;
    struct Gadget *ok_gad;
    struct Gadget *cancel_gad;
    
    /* ListView data */
    struct List listview_list;
    
    /* Font metrics */
    UWORD font_width;
    UWORD font_height;
    
    /* Border sizes (read after window opens) */
    UWORD border_left;
    UWORD border_top;
    UWORD border_right;
    UWORD border_bottom;
    
    /* Current inner dimensions */
    UWORD inner_width;
    UWORD inner_height;
    
    /* Gadget anchor information */
    struct iTidy_ResizeGadgetAnchor listview_anchor;
    struct iTidy_ResizeGadgetAnchor ok_anchor;
    struct iTidy_ResizeGadgetAnchor cancel_anchor;
    
    /* Console output file handle (for Workbench launch) */
    BPTR console_fh;
};

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static BOOL iTidy_Resize_OpenConsole(struct iTidy_ResizeWindowData *data);
static void iTidy_Resize_CloseConsole(struct iTidy_ResizeWindowData *data);
static BOOL iTidy_Resize_PopulateListView(struct iTidy_ResizeWindowData *data);
static void iTidy_Resize_FreeListViewItems(struct iTidy_ResizeWindowData *data);
static BOOL iTidy_Resize_InitWindowData(struct iTidy_ResizeWindowData *data);
static void iTidy_Resize_CleanupWindowData(struct iTidy_ResizeWindowData *data);
static BOOL iTidy_Resize_CreateGadgets(struct iTidy_ResizeWindowData *data);
static void iTidy_Resize_DestroyGadgets(struct iTidy_ResizeWindowData *data);
static BOOL iTidy_Resize_OpenWindow(struct iTidy_ResizeWindowData *data);
static void iTidy_Resize_CloseWindow(struct iTidy_ResizeWindowData *data);
static void iTidy_Resize_ComputeGadgetPositions(struct iTidy_ResizeWindowData *data);
static void iTidy_Resize_HandleResize(struct iTidy_ResizeWindowData *data);
static void iTidy_Resize_ClearWindowInterior(struct iTidy_ResizeWindowData *data);
static void iTidy_Resize_LogWindowGeometry(struct iTidy_ResizeWindowData *data, const char *prefix);
static void iTidy_Resize_LogGadgetGeometry(struct iTidy_ResizeWindowData *data);
static void iTidy_Resize_HandleGadgetEvent(struct iTidy_ResizeWindowData *data, struct IntuiMessage *imsg);
static void iTidy_Resize_EventLoop(struct iTidy_ResizeWindowData *data);

/*------------------------------------------------------------------------*/
/**
 * @brief Open console for printf() output when launched from Workbench
 */
/*------------------------------------------------------------------------*/
static BOOL iTidy_Resize_OpenConsole(struct iTidy_ResizeWindowData *data)
{
    /* Open a console window for output */
    data->console_fh = Open("CON:10/50/600/200/Resize Template Console/AUTO/CLOSE/WAIT", MODE_NEWFILE);
    
    if (data->console_fh == 0)
    {
        return FALSE;
    }
    
    /* Redirect stdout to the console */
    SelectOutput(data->console_fh);
    
    printf("=== Resizable GadTools Window Template ===\n");
    printf("Console opened successfully.\n\n");
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Close console window
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_CloseConsole(struct iTidy_ResizeWindowData *data)
{
    if (data->console_fh != 0)
    {
        printf("\nClosing console...\n");
        Close(data->console_fh);
        data->console_fh = 0;
    }
}

/*------------------------------------------------------------------------*/
/**
 * @brief Initialize window data structure and open screen
 */
/*------------------------------------------------------------------------*/
static BOOL iTidy_Resize_InitWindowData(struct iTidy_ResizeWindowData *data)
{
    struct TextAttr *font_attr;
    
    /* Clear the structure */
    memset(data, 0, sizeof(struct iTidy_ResizeWindowData));
    
    /* Initialize ListView list */
    NewList(&data->listview_list);
    
    /* Lock the Workbench screen */
    data->screen = LockPubScreen("Workbench");
    if (data->screen == NULL)
    {
        printf("ERROR: Cannot lock Workbench screen\n");
        return FALSE;
    }
    
    /* Get font metrics from screen */
    font_attr = data->screen->Font;
    data->font_width = font_attr->ta_YSize / 2;  /* Approximate for proportional fonts */
    data->font_height = font_attr->ta_YSize;
    
    printf("Screen font: %s/%d\n", font_attr->ta_Name, font_attr->ta_YSize);
    printf("Font metrics: width=%d, height=%d\n", data->font_width, data->font_height);
    
    /* Get visual info for GadTools */
    data->visual_info = GetVisualInfo(data->screen, TAG_DONE);
    if (data->visual_info == NULL)
    {
        printf("ERROR: Cannot get visual info\n");
        UnlockPubScreen(NULL, data->screen);
        data->screen = NULL;
        return FALSE;
    }
    
    /* Initialize ListView list */
    NewList(&data->listview_list);
    
    /* Populate ListView with Amiga-themed data */
    if (!iTidy_Resize_PopulateListView(data))
    {
        printf("ERROR: Failed to populate ListView\n");
        iTidy_Resize_CleanupWindowData(data);
        return FALSE;
    }
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Populate ListView with test data
 */
/*------------------------------------------------------------------------*/
static BOOL iTidy_Resize_PopulateListView(struct iTidy_ResizeWindowData *data)
{
    static const STRPTR test_items[] = {
        "The Amiga 500 was the best-selling model, bringing 16-bit computing to homes",
        "Shadow of the Beast showcased the Amiga's graphics with 12 layers of parallax",
        "Defender of the Crown was an early hit demonstrating the Amiga's capabilities",
        "Lemmings became a phenomenon with its addictive puzzle gameplay and music",
        "The Amiga's custom chipset included Paula for 4-channel stereo sound",
        "Workbench was the graphical interface that made the Amiga user-friendly",
        "Deluxe Paint revolutionized digital art creation on personal computers",
        "The Motorola 68000 CPU ran at 7.16 MHz in NTSC Amiga models",
        "Monkey Island brought LucasArts adventures to life with SCUMM engine magic",
        "Sensible Soccer offered fast-paced action with tiny players and huge fun",
        "The Amiga could display 4096 colors with HAM mode graphics capabilities",
        "Speedball 2: Brutal Deluxe combined sports and violence in futuristic arenas",
        "Agnus was the custom chip handling blitter operations and copper control",
        "Turrican showed off smooth scrolling and detailed sprite-based graphics",
        "The Amiga 1200 brought AGA chipset with 256 colors from a 16.8M palette",
        "Cannon Fodder mixed strategy and action with dark humor about warfare",
        "Denise handled video display and sprite collision detection efficiently",
        "Kick Off series delivered fast arcade football with innovative controls",
        "The Amiga used AutoConfig for automatic hardware configuration and setup",
        "Flashback featured rotoscoped animation and cinematic platform gameplay",
        "Chip RAM was shared between CPU and custom chips for maximum efficiency",
        "Worms brought turn-based artillery chaos with exploding sheep and holy grenades",
        "The blitter could move data blocks without CPU intervention for smooth graphics",
        "Another World amazed players with polygon graphics and cinematic storytelling",
        "Zool was marketed as a mascot to rival Sonic on the Mega Drive console",
        "The copper coprocessor could change registers mid-screen for stunning effects",
        "Stunt Car Racer delivered thrilling 3D racing on roller coaster-like tracks",
        "AmigaDOS provided a powerful command-line interface for advanced users",
        "Lotus Turbo Challenge featured split-screen racing through weather conditions",
        "The Amiga supported multitasking when PCs were still running DOS exclusively",
        "Wings brought World War I dogfighting to life with detailed missions",
        "Guru Meditation errors became infamous as the Amiga's equivalent of blue screens",
        "The Secret of Monkey Island featured insult sword fighting and ghost pirates",
        "Fast RAM could be added via expansions for better CPU performance and speed",
        "Frontier: Elite II offered a whole galaxy to explore with Newtonian physics",
        "The Amiga 2000 provided expansion slots for professional video and audio cards",
        "Xenon 2 Megablast combined shooting action with a legendary Bomb the Bass tune",
        "Kickstart ROM contained the essential OS code that booted the Amiga system",
        "Beneath a Steel Sky offered cyberpunk adventure with comic book style graphics",
        "The Amiga 4000 was the high-end model with faster CPUs and expansion options",
        "Hired Guns delivered first-person action with four simultaneous characters",
        "IFF file format became a standard for storing images, sounds, and animations",
        "The Chaos Engine combined steampunk themes with run-and-gun cooperative play",
        "Video Toaster turned Amigas into professional broadcast video editing systems",
        "Pinball Dreams and Fantasies brought realistic table physics to home computers",
        "The Amiga CD32 was the first 32-bit CD-ROM gaming console ever released",
        "Superfrog featured a slimy hero and colorful platforming across many worlds",
        "Paula chip could play samples at different frequencies for music and effects",
        "Simon the Sorcerer brought point-and-click adventures with British humor",
        "The Amiga forever changed home computing with multimedia power and elegance"
    };
    
    struct iTidy_ResizeListViewItem *item;
    ULONG i;
    
    for (i = 0; i < 50; i++)
    {
        /* Allocate memory for the ListView item */
        item = (struct iTidy_ResizeListViewItem *)AllocVec(sizeof(struct iTidy_ResizeListViewItem), MEMF_CLEAR);
        if (item == NULL)
        {
            return FALSE;
        }
        
        /* Allocate memory for the text string */
        item->text = (STRPTR)AllocVec(strlen(test_items[i]) + 1, MEMF_CLEAR);
        if (item->text == NULL)
        {
            FreeVec(item);
            return FALSE;
        }
        
        /* Copy the text string */
        strcpy(item->text, test_items[i]);
        
        /* Set the node name to point to our text (required for GadTools ListView) */
        item->node.ln_Name = item->text;
        item->node.ln_Type = NT_USER;
        item->node.ln_Pri = 0;
        
        /* Store item index as user data */
        item->user_data = i;
        
        /* Add the item to the end of the list */
        AddTail(&data->listview_list, (struct Node *)item);
    }
    
    printf("Successfully populated ListView with %d items\n", (int)i);
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Free all ListView items and clear the list
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_FreeListViewItems(struct iTidy_ResizeWindowData *data)
{
    struct iTidy_ResizeListViewItem *item;
    struct iTidy_ResizeListViewItem *next_item;
    
    item = (struct iTidy_ResizeListViewItem *)data->listview_list.lh_Head;
    
    while (item->node.ln_Succ != NULL)
    {
        next_item = (struct iTidy_ResizeListViewItem *)item->node.ln_Succ;
        
        /* Free the text string */
        if (item->text != NULL)
        {
            FreeVec(item->text);
        }
        
        /* Free the item structure */
        FreeVec(item);
        
        item = next_item;
    }
    
    /* Reinitialize the list */
    NewList(&data->listview_list);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Cleanup window data and unlock screen
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_CleanupWindowData(struct iTidy_ResizeWindowData *data)
{
    iTidy_Resize_FreeListViewItems(data);
    
    if (data->visual_info != NULL)
    {
        FreeVisualInfo(data->visual_info);
        data->visual_info = NULL;
    }
    
    if (data->screen != NULL)
    {
        UnlockPubScreen(NULL, data->screen);
        data->screen = NULL;
    }
}

/*------------------------------------------------------------------------*/
/**
 * @brief Create all GadTools gadgets with current window dimensions
 * 
 * CRITICAL: Follows patterns from "Resizable GadTools Forms" document.
 * Computes gadget positions based on current inner_width/inner_height.
 * Stores anchor information for later resize calculations.
 * Uses equal-width button layout aligned with ListView width.
 */
/*------------------------------------------------------------------------*/
static BOOL iTidy_Resize_CreateGadgets(struct iTidy_ResizeWindowData *data)
{
    struct NewGadget ng;
    struct Gadget *gad;
    UWORD button_height;
    UWORD listview_height;
    
    /* Compute standard gadget heights based on font */
    button_height = data->font_height + 6;
    
    /* Create gadget context */
    gad = CreateContext(&data->glist);
    if (gad == NULL)
    {
        printf("ERROR: Cannot create gadget context\n");
        return FALSE;
    }
    
    /* Compute gadget positions based on current inner dimensions */
    iTidy_Resize_ComputeGadgetPositions(data);
    
    /*--------------------------------------------------------------------*/
    /* LISTVIEW - Anchored top-left, stretches both horizontally and      */
    /* vertically to fill space above buttons                             */
    /* CRITICAL: Use anchor margins consistently (already include label)  */
    /* Window-relative coordinates: add border offsets to content margins */
    /*--------------------------------------------------------------------*/
    listview_height = data->inner_height - data->listview_anchor.top_margin - data->listview_anchor.bottom_margin;
    
    /* CRITICAL: Add border offsets - GadTools uses window-relative coords */
    ng.ng_LeftEdge = data->border_left + data->listview_anchor.left_margin;
    ng.ng_TopEdge = data->border_top + data->listview_anchor.top_margin;
    ng.ng_Width = data->inner_width - data->listview_anchor.left_margin - data->listview_anchor.right_margin;
    ng.ng_Height = listview_height;
    ng.ng_GadgetText = "List";
    ng.ng_TextAttr = data->screen->Font;
    ng.ng_GadgetID = ITIDY_RESIZE_GID_LISTVIEW;
    ng.ng_Flags = PLACETEXT_ABOVE;  /* Label above ListView */
    ng.ng_VisualInfo = data->visual_info;
    
    data->listview_gad = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, &data->listview_list,
        TAG_DONE);
    
    if (gad == NULL)
    {
        printf("ERROR: Cannot create ListView gadget\n");
        FreeGadgets(data->glist);
        data->glist = NULL;
        return FALSE;
    }
    
    /*--------------------------------------------------------------------*/
    /* OK BUTTON - Aligned with ListView left, takes up left half         */
    /* CANCEL BUTTON - Takes up right half, aligned with ListView right   */
    /* Both buttons stretch horizontally to maintain equal spacing        */
    /* Uses font-measured widths for proper text display                  */
    /*--------------------------------------------------------------------*/
    {
        UWORD listview_bottom = data->listview_gad->TopEdge + data->listview_gad->Height;
        UWORD button_y = listview_bottom + (ITIDY_RESIZE_SPACE_Y * 2);
        UWORD listview_left = data->listview_gad->LeftEdge;
        UWORD listview_width = data->listview_gad->Width;
        UWORD button_spacing = ITIDY_RESIZE_SPACE_X;
        UWORD total_button_width = listview_width - button_spacing;
        UWORD equal_button_width = total_button_width / 2;
        
        /* OK button: aligned with ListView left edge */
        ng.ng_LeftEdge = listview_left;
        ng.ng_TopEdge = button_y;
        ng.ng_Width = equal_button_width;
        ng.ng_Height = button_height;
        ng.ng_GadgetText = "OK";
        ng.ng_GadgetID = ITIDY_RESIZE_GID_OK;
        ng.ng_Flags = PLACETEXT_IN;
    
        data->ok_gad = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    
        if (gad == NULL)
        {
            printf("ERROR: Cannot create OK button\n");
            FreeGadgets(data->glist);
            data->glist = NULL;
            return FALSE;
        }
    
        /* Cancel button: positioned after OK button with spacing */
        ng.ng_LeftEdge = listview_left + equal_button_width + button_spacing;
        ng.ng_TopEdge = button_y;
        ng.ng_Width = equal_button_width;
        ng.ng_Height = button_height;
        ng.ng_GadgetText = "Cancel";
        ng.ng_GadgetID = ITIDY_RESIZE_GID_CANCEL;
        ng.ng_Flags = PLACETEXT_IN;
    
        data->cancel_gad = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    
        if (gad == NULL)
        {
            printf("ERROR: Cannot create Cancel button\n");
            FreeGadgets(data->glist);
            data->glist = NULL;
            return FALSE;
        }
    }  /* End button positioning block */
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Destroy all GadTools gadgets
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_DestroyGadgets(struct iTidy_ResizeWindowData *data)
{
    if (data->glist != NULL)
    {
        FreeGadgets(data->glist);
        data->glist = NULL;
    }
    
    /* Clear individual gadget pointers */
    data->listview_gad = NULL;
    data->ok_gad = NULL;
    data->cancel_gad = NULL;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Compute and store gadget anchor positions
 * 
 * CRITICAL: This function implements the anchoring strategy from
 * "Resizable GadTools Forms on Amiga Workbench 3.0.txt".
 * 
 * Each gadget's anchor information stores its distance from window edges,
 * allowing us to recompute positions/sizes on resize.
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_ComputeGadgetPositions(struct iTidy_ResizeWindowData *data)
{
    /*--------------------------------------------------------------------*/
    /* LISTVIEW: Anchored top-left, stretches to fill available space    */
    /* Note: top_margin includes space for "List" label above            */
    /*--------------------------------------------------------------------*/
    data->listview_anchor.left_margin = ITIDY_RESIZE_MARGIN_LEFT;
    data->listview_anchor.top_margin = ITIDY_RESIZE_MARGIN_TOP + (data->font_height + 4);
    data->listview_anchor.right_margin = ITIDY_RESIZE_MARGIN_RIGHT;
    data->listview_anchor.bottom_margin = ITIDY_RESIZE_MARGIN_BOTTOM + ITIDY_RESIZE_BUTTON_ROW_HEIGHT + ITIDY_RESIZE_SPACE_Y;
    data->listview_anchor.stretch_h = TRUE;
    data->listview_anchor.stretch_v = TRUE;
    data->listview_anchor.anchor_right = FALSE;
    data->listview_anchor.anchor_bottom = FALSE;
    
    /*--------------------------------------------------------------------*/
    /* OK BUTTON: Anchored bottom-left                                    */
    /*--------------------------------------------------------------------*/
    data->ok_anchor.left_margin = ITIDY_RESIZE_MARGIN_LEFT;
    data->ok_anchor.top_margin = 0;  /* Not used for bottom-anchored */
    data->ok_anchor.right_margin = 0;  /* Not used */
    data->ok_anchor.bottom_margin = ITIDY_RESIZE_MARGIN_BOTTOM;
    data->ok_anchor.stretch_h = FALSE;
    data->ok_anchor.stretch_v = FALSE;
    data->ok_anchor.anchor_right = FALSE;
    data->ok_anchor.anchor_bottom = TRUE;
    
    /*--------------------------------------------------------------------*/
    /* CANCEL BUTTON: Anchored bottom-left, positioned after OK button   */
    /*--------------------------------------------------------------------*/
    data->cancel_anchor.left_margin = ITIDY_RESIZE_MARGIN_LEFT + 80 + ITIDY_RESIZE_SPACE_X;  /* After OK button */
    data->cancel_anchor.top_margin = 0;  /* Not used for bottom-anchored */
    data->cancel_anchor.right_margin = 0;  /* Not used */
    data->cancel_anchor.bottom_margin = ITIDY_RESIZE_MARGIN_BOTTOM;
    data->cancel_anchor.stretch_h = FALSE;
    data->cancel_anchor.stretch_v = FALSE;
    data->cancel_anchor.anchor_right = FALSE;
    data->cancel_anchor.anchor_bottom = TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Open the resizable window
 * 
 * CRITICAL: Uses standard Workbench window (no GimmeZeroZero).
 * Border sizes are calculated using RKM formula, then verified after open.
 * Implements BorderTop verification per AI_AGENT_GETTING_STARTED.md
 */
/*------------------------------------------------------------------------*/
static BOOL iTidy_Resize_OpenWindow(struct iTidy_ResizeWindowData *data)
{
    UWORD outer_width;
    UWORD outer_height;
    UWORD min_outer_width;
    UWORD min_outer_height;
    
    /* CRITICAL: Calculate border sizes using RKM formula */
    /* BorderTop = WBorTop + font height + 1 (title bar) */
    /* This is the ONLY correct formula per AI_AGENT_GETTING_STARTED.md */
    UWORD est_border_top = data->screen->WBorTop + data->screen->Font->ta_YSize + 1;
    UWORD est_border_bottom = data->screen->WBorBottom + 2;  /* Size gadget */
    UWORD est_border_left = data->screen->WBorLeft + 2;
    UWORD est_border_right = data->screen->WBorRight + 13;  /* Size gadget */
    
    /* Compute initial outer dimensions */
    outer_width = ITIDY_RESIZE_INITIAL_INNER_WIDTH + est_border_left + est_border_right;
    outer_height = ITIDY_RESIZE_INITIAL_INNER_HEIGHT + est_border_top + est_border_bottom;
    
    /* Compute minimum outer dimensions */
    min_outer_width = ITIDY_RESIZE_MIN_INNER_WIDTH + est_border_left + est_border_right;
    min_outer_height = ITIDY_RESIZE_MIN_INNER_HEIGHT + est_border_top + est_border_bottom;
    
    /* Set initial inner dimensions for gadget creation */
    data->inner_width = ITIDY_RESIZE_INITIAL_INNER_WIDTH;
    data->inner_height = ITIDY_RESIZE_INITIAL_INNER_HEIGHT;
    
    /* CRITICAL: Set estimated border values for initial gadget positioning */
    /* These will be verified against actual values after window opens */
    data->border_left = est_border_left;
    data->border_top = est_border_top;
    data->border_right = est_border_right;
    data->border_bottom = est_border_bottom;
    
    /* Create initial gadgets before opening window */
    if (!iTidy_Resize_CreateGadgets(data))
    {
        return FALSE;
    }
    
    /* Open the window */
    data->window = OpenWindowTags(NULL,
        WA_Left, (data->screen->Width - outer_width) / 2,
        WA_Top, (data->screen->Height - outer_height) / 2,
        WA_Width, outer_width,
        WA_Height, outer_height,
        WA_MinWidth, min_outer_width,
        WA_MinHeight, min_outer_height,
        WA_MaxWidth, ~0,
        WA_MaxHeight, ~0,
        WA_Title, ITIDY_RESIZE_WINDOW_TITLE,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_SizeBRight, TRUE,
        WA_SizeBBottom, TRUE,
        WA_Activate, TRUE,
        WA_SimpleRefresh, TRUE,  /* Simple Refresh as per clearing guide */
        WA_NoCareRefresh, FALSE,  /* MUST handle refresh for GadTools */
        WA_PubScreen, data->screen,
        WA_Gadgets, data->glist,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN |
                  IDCMP_REFRESHWINDOW | IDCMP_SIZEVERIFY | IDCMP_NEWSIZE,
        TAG_DONE);
    
    if (data->window == NULL)
    {
        printf("ERROR: Cannot open window\n");
        iTidy_Resize_DestroyGadgets(data);
        return FALSE;
    }
    
    /* Read actual border sizes from opened window */
    data->border_left = data->window->BorderLeft;
    data->border_top = data->window->BorderTop;
    data->border_right = data->window->BorderRight;
    data->border_bottom = data->window->BorderBottom;
    
    /* CRITICAL: Verify BorderTop calculation (AI_AGENT_GETTING_STARTED.md) */
    if (data->border_top != est_border_top)
    {
        printf("WARNING: BorderTop mismatch! Calculated=%d, Actual=%d, Diff=%d\n",
               est_border_top, data->border_top, data->border_top - est_border_top);
    }
    else
    {
        printf("BorderTop verification: PASS (Calculated=%d matches Actual=%d)\n",
               est_border_top, data->border_top);
    }
    
    /* Compute actual inner dimensions */
    data->inner_width = data->window->Width - data->border_left - data->border_right;
    data->inner_height = data->window->Height - data->border_top - data->border_bottom;
    
    /* Refresh gadgets to ensure they're drawn */
    GT_RefreshWindow(data->window, NULL);
    
    /* Log initial window geometry */
    iTidy_Resize_LogWindowGeometry(data, "Initial window opened");
    iTidy_Resize_LogGadgetGeometry(data);
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Close the window
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_CloseWindow(struct iTidy_ResizeWindowData *data)
{
    if (data->window != NULL)
    {
        CloseWindow(data->window);
        data->window = NULL;
    }
    
    iTidy_Resize_DestroyGadgets(data);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Clear window interior to background color
 * 
 * CRITICAL: Implements the clearing strategy from "Clearing Window
 * Contents Before Gadget Re-Add in GadTools.txt" to prevent ghost gadgets.
 * 
 * Uses EraseRect to clear the entire inner region with background pen.
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_ClearWindowInterior(struct iTidy_ResizeWindowData *data)
{
    struct RastPort *rp = data->window->RPort;
    
    /* Set background pen (typically pen 0 for Workbench) */
    SetAPen(rp, 0);
    
    /* Clear entire inner region */
    /* Coordinates are relative to window's RastPort (includes borders) */
    RectFill(rp,
        data->border_left,
        data->border_top,
        data->window->Width - data->border_right - 1,
        data->window->Height - data->border_bottom - 1);
    
    printf("  Cleared inner region: (%d,%d) to (%d,%d)\n",
        data->border_left, data->border_top,
        data->window->Width - data->border_right - 1,
        data->window->Height - data->border_bottom - 1);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Handle window resize event (IDCMP_NEWSIZE)
 * 
 * CRITICAL: Implements the resize strategy from both reference documents:
 * 1. Recompute inner client area from new window dimensions
 * 2. Clear entire inner region to remove old gadget imagery
 * 3. Destroy and recreate gadgets with new positions/sizes
 * 4. Refresh window to draw new gadgets
 * 
 * This is the "full rebuild" approach which is simpler and ensures
 * clean redraws with no artifacts.
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_HandleResize(struct iTidy_ResizeWindowData *data)
{
    /* Compute new inner dimensions based on actual window size */
    data->inner_width = data->window->Width - data->border_left - data->border_right;
    data->inner_height = data->window->Height - data->border_top - data->border_bottom;
    
    printf("\n=== RESIZE EVENT ===\n");
    printf("New window size: %d x %d (outer)\n", data->window->Width, data->window->Height);
    printf("New inner size: %d x %d\n", data->inner_width, data->inner_height);
    
    /* Clear the entire window interior to remove old gadget imagery */
    /* This prevents "ghost" gadgets from remaining visible */
    iTidy_Resize_ClearWindowInterior(data);
    
    /* Destroy old gadgets completely */
    iTidy_Resize_DestroyGadgets(data);
    
    /* Create new gadgets with updated positions/sizes */
    if (!iTidy_Resize_CreateGadgets(data))
    {
        printf("ERROR: Failed to recreate gadgets after resize\n");
        return;
    }
    
    /* Add new gadgets to window */
    AddGList(data->window, data->glist, ~0, -1, NULL);
    
    /* Refresh gadgets - CRITICAL: Must refresh list AND call GT_RefreshWindow */
    RefreshGList(data->glist, data->window, NULL, -1);
    GT_RefreshWindow(data->window, NULL);
    
    /* Log updated geometry */
    iTidy_Resize_LogGadgetGeometry(data);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Log window geometry to console
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_LogWindowGeometry(struct iTidy_ResizeWindowData *data, const char *prefix)
{
    printf("\n=== %s ===\n", prefix);
    printf("Window outer size: %d x %d\n", data->window->Width, data->window->Height);
    printf("Window position: (%d, %d)\n", data->window->LeftEdge, data->window->TopEdge);
    printf("Borders: Left=%d, Top=%d, Right=%d, Bottom=%d\n",
        data->border_left, data->border_top, data->border_right, data->border_bottom);
    printf("Inner client area: %d x %d\n", data->inner_width, data->inner_height);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Log all gadget positions and sizes to console
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_LogGadgetGeometry(struct iTidy_ResizeWindowData *data)
{
    printf("\n--- Gadget Geometry (window-relative coordinates) ---\n");
    
    if (data->listview_gad != NULL)
    {
        printf("ListView: LeftEdge=%d, TopEdge=%d, Width=%d, Height=%d\n",
            data->listview_gad->LeftEdge, data->listview_gad->TopEdge,
            data->listview_gad->Width, data->listview_gad->Height);
    }
    
    if (data->ok_gad != NULL)
    {
        printf("OK Button: LeftEdge=%d, TopEdge=%d, Width=%d, Height=%d\n",
            data->ok_gad->LeftEdge, data->ok_gad->TopEdge,
            data->ok_gad->Width, data->ok_gad->Height);
    }
    
    if (data->cancel_gad != NULL)
    {
        printf("Cancel Button: LeftEdge=%d, TopEdge=%d, Width=%d, Height=%d\n",
            data->cancel_gad->LeftEdge, data->cancel_gad->TopEdge,
            data->cancel_gad->Width, data->cancel_gad->Height);
    }
    
    printf("--- End Gadget Geometry ---\n\n");
}

/*------------------------------------------------------------------------*/
/**
 * @brief Handle gadget events and log them to console
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_HandleGadgetEvent(struct iTidy_ResizeWindowData *data, struct IntuiMessage *imsg)
{
    struct Gadget *gad = (struct Gadget *)imsg->IAddress;
    UWORD code = imsg->Code;
    
    switch (gad->GadgetID)
    {
        case ITIDY_RESIZE_GID_LISTVIEW:
        {
            struct Node *node;
            UWORD index = 0;
            
            printf("ListView: Selected index %d\n", code);
            
            /* Walk the list to find the selected node */
            for (node = data->listview_list.lh_Head; node->ln_Succ != NULL; node = node->ln_Succ)
            {
                if (index == code)
                {
                    printf("  Text: %s\n", node->ln_Name);
                    break;
                }
                index++;
            }
            break;
        }
        
        case ITIDY_RESIZE_GID_OK:
            printf("OK button pressed\n");
            break;
        
        case ITIDY_RESIZE_GID_CANCEL:
            printf("Cancel button pressed - closing window\n");
            /* Signal to exit event loop by setting window to NULL */
            /* (will be caught in event_loop) */
            break;
    }
}

/*------------------------------------------------------------------------*/
/**
 * @brief Main event loop
 * 
 * CRITICAL: Handles all IDCMP events including:
 * - IDCMP_SIZEVERIFY: Remove gadgets before resize (reduce flicker)
 * - IDCMP_NEWSIZE: Rebuild UI after resize
 * - IDCMP_REFRESHWINDOW: Satisfy Intuition's refresh protocol
 * - IDCMP_CLOSEWINDOW: Clean exit
 * - IDCMP_GADGETUP: User interactions
 */
/*------------------------------------------------------------------------*/
static void iTidy_Resize_EventLoop(struct iTidy_ResizeWindowData *data)
{
    struct IntuiMessage *imsg;
    ULONG imsg_class;
    BOOL done = FALSE;
    
    printf("\nEntering event loop. Try resizing the window!\n");
    printf("Press Cancel button or close gadget to exit.\n\n");
    
    while (!done && data->window != NULL)
    {
        /* Wait for messages */
        WaitPort(data->window->UserPort);
        
        /* Process all pending messages */
        while ((imsg = GT_GetIMsg(data->window->UserPort)) != NULL)
        {
            imsg_class = imsg->Class;
            
            /* Process message based on class */
            switch (imsg_class)
            {
                case IDCMP_CLOSEWINDOW:
                    printf("Close gadget clicked - exiting\n");
                    done = TRUE;
                    break;
                
                case IDCMP_SIZEVERIFY:
                    /*--------------------------------------------------------*/
                    /* CRITICAL: Remove gadgets BEFORE window is resized     */
                    /* This prevents flicker and artifacts during drag       */
                    /* As per "Resizable GadTools Forms" document            */
                    /*--------------------------------------------------------*/
                    printf("SIZEVERIFY: Removing gadgets before resize...\n");
                    
                    if (data->glist != NULL)
                    {
                        RemoveGList(data->window, data->glist, -1);
                    }
                    
                    /* Reply immediately - Intuition waits for this */
                    GT_ReplyIMsg(imsg);
                    imsg = NULL;  /* Don't reply again */
                    break;
                
                case IDCMP_NEWSIZE:
                    /*--------------------------------------------------------*/
                    /* CRITICAL: User finished resizing - rebuild UI         */
                    /* Follows pattern from both reference documents         */
                    /*--------------------------------------------------------*/
                    iTidy_Resize_HandleResize(data);
                    break;
                
                case IDCMP_REFRESHWINDOW:
                    /*--------------------------------------------------------*/
                    /* CRITICAL: GadTools requires proper refresh handling   */
                    /* Must call GT_BeginRefresh/GT_EndRefresh even if we    */
                    /* have no custom drawing, as per clearing guide         */
                    /*--------------------------------------------------------*/
                    GT_BeginRefresh(data->window);
                    /* No custom graphics to redraw - GadTools handles gadgets */
                    GT_EndRefresh(data->window, TRUE);
                    break;
                
                case IDCMP_GADGETUP:
                    iTidy_Resize_HandleGadgetEvent(data, imsg);
                    
                    /* Check if Cancel was pressed */
                    if (((struct Gadget *)imsg->IAddress)->GadgetID == ITIDY_RESIZE_GID_CANCEL)
                    {
                        done = TRUE;
                    }
                    break;
            }
            
            /* Reply to message if not already done */
            if (imsg != NULL)
            {
                GT_ReplyIMsg(imsg);
            }
        }
    }
    
    printf("\nExiting event loop.\n");
}

/*------------------------------------------------------------------------*/
/**
 * @brief Main program entry point
 */
/*------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    struct iTidy_ResizeWindowData data;
    BOOL success = TRUE;
    
    /* Initialize data structure */
    memset(&data, 0, sizeof(struct iTidy_ResizeWindowData));
    
    /* If launched from Workbench (argc==0), open console */
    if (argc == 0)
    {
        if (!iTidy_Resize_OpenConsole(&data))
        {
            return 20;  /* FAIL */
        }
    }
    
    printf("=== Resizable GadTools Window Template ===\n");
    printf("Workbench 3.0/3.1 compatible\n");
    printf("Demonstrates manual resize with no RELWIDTH/RELHEIGHT flags\n\n");
    
    /* Initialize window data and lock screen */
    if (!iTidy_Resize_InitWindowData(&data))
    {
        success = FALSE;
    }
    
    /* Open window with initial gadgets */
    if (success && !iTidy_Resize_OpenWindow(&data))
    {
        success = FALSE;
    }
    
    /* Run event loop */
    if (success)
    {
        iTidy_Resize_EventLoop(&data);
    }
    
    /* Cleanup */
    iTidy_Resize_CloseWindow(&data);
    iTidy_Resize_CleanupWindowData(&data);
    
    if (argc == 0)
    {
        iTidy_Resize_CloseConsole(&data);
    }
    
    printf("Template exited %s.\n", success ? "successfully" : "with errors");
    
    return success ? 0 : 20;
}
