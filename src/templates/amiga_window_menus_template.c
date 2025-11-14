/*------------------------------------------------------------------------*/
/*                                                                        *
 *  $Id: simplemenu.c,v 2.0 2025/07/01 15:00:00 user Exp $
 *                                                                        *
 *  SimpleMenu - Workbench 3.x NewLook Menu Template                      *
 *  Template for creating modern Ami
 *  IMPORTANT LIMITATION (for AI agents):                            *
 *  - This template intentionally does NOT use IDCMP_MENUVERIFY or     *
 *    MENUVERIFY.                                                      *
 *  - Do NOT add IDCMP_MENUVERIFY to the IDCMP mask.                   *
 *  - All menu behaviour must be implemented via IDCMP_MENUPICK only,  *
 *    using ItemAddress() + GTMENUITEM_USERDATA() as shown below.      *
 *                                                                      *
ga menus with proper appearance       *
 *                                                                        *
 *  TEMPLATE USAGE GUIDE FOR AI AGENTS:                                   *
 *  ====================================                                  *
 *                                                                        *
 *  1. CUSTOMIZE WINDOW SETTINGS (lines 25-30):                          *
 *     - Change WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT as needed       *
 *     - Adjust WINDOW_LEFT, WINDOW_TOP for positioning                   *
 *                                                                        *
 *  2. ADD MENU ITEM IDs (lines 32-37):                                   *
 *     - Add #define statements for your menu item constants              *
 *     - Use sequential numbers starting from 1                           *
 *                                                                        *
 *  3. MODIFY MENU TEMPLATE (lines 44-52):                                *
 *     - Add/remove NM_TITLE entries for menu titles                      *
 *     - Add/remove NM_ITEM entries for menu items                        *
 *     - Use NM_BARLABEL for separator bars                               *
 *     - End with NM_END entry                                            *
 *                                                                        *
 *  4. UPDATE MENU HANDLER (handle_menu_selection function):              *
 *     - Add case statements for your menu item IDs                       *
 *     - Implement the actual functionality for each menu item            *
 *                                                                        *
 *  5. CORE FUNCTIONS (DO NOT MODIFY):                                    *
 *     - setup_newlook_menus() - Sets up the menu system                  *
 *     - cleanup_newlook_menus() - Cleans up resources                    *
 *     - main() - Main program loop and window handling                   *
 *                                                                        *
 *  This template provides automatic NewLook menu support with proper     *
 *  Workbench 3.x white backgrounds and system color adaptation.          *
 *                                                                        */
/*------------------------------------------------------------------------*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>

/* Function prototypes - avoiding proto includes to prevent conflicts */
struct Library *OpenLibrary(STRPTR, ULONG);
void CloseLibrary(struct Library *);
struct Screen *LockPubScreen(STRPTR);
void UnlockPubScreen(STRPTR, struct Screen *);
struct DrawInfo *GetScreenDrawInfo(struct Screen *);
void FreeScreenDrawInfo(struct Screen *, struct DrawInfo *);
APTR GetVisualInfo(struct Screen *, ULONG, ...);
void FreeVisualInfo(APTR);
struct Menu *CreateMenus(struct NewMenu *, ULONG, ...);
void FreeMenus(struct Menu *);
BOOL LayoutMenus(struct Menu *, APTR, ULONG, ...);
struct Window *OpenWindowTags(struct Window *, ULONG, ...);
void CloseWindow(struct Window *);
void SetMenuStrip(struct Window *, struct Menu *);
void ClearMenuStrip(struct Window *);
ULONG Wait(ULONG);
struct IntuiMessage *GT_GetIMsg(struct MsgPort *);
void GT_ReplyIMsg(struct IntuiMessage *);
struct MenuItem *ItemAddress(struct Menu *, ULONG);
LONG Printf(STRPTR, ...);

/* Library bases for standalone program */
#pragma amiga-align
struct Library *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;

/* Template Configuration - Modify these for your application */
#define WINDOW_TITLE    "GadTools NewLook Menu Template"
#define WINDOW_WIDTH    300
#define WINDOW_HEIGHT   200
#define WINDOW_LEFT     20
#define WINDOW_TOP      20

/* Menu item IDs - Add your own menu item constants here */
#define MENU_OPEN       1
#define MENU_SAVE       2  
#define MENU_PRINT      3
#define MENU_QUIT       4

/* Global variables for menu system */
struct Screen *wb_screen = NULL;        /* Workbench screen pointer */
struct DrawInfo *draw_info = NULL;      /* Drawing info for visual info */
APTR visual_info = NULL;                /* Visual info for GadTools menus */
struct Menu *menu_strip = NULL;         /* GadTools menu strip */

/* Menu Template - Customize this for your application menus */
struct NewMenu menu_template[] = 
{
	{ NM_TITLE, "File",        NULL, 0, 0, NULL },
	{ NM_ITEM,  "Open",        "O",  0, 0, (APTR)MENU_OPEN },
	{ NM_ITEM,  "Save",        "S",  0, 0, (APTR)MENU_SAVE },
	{ NM_ITEM,  "Print",       "P",  0, 0, (APTR)MENU_PRINT },
	{ NM_ITEM,  NM_BARLABEL,   NULL, 0, 0, NULL },
	{ NM_ITEM,  "Quit",        "Q",  0, 0, (APTR)MENU_QUIT },
	{ NM_END,   NULL,          NULL, 0, 0, NULL }
};

/*------------------------------------------------------------------------*/

/**
 * setup_newlook_menus - Initialize GadTools NewLook menu system
 * 
 * This function sets up GadTools menus with proper Workbench 3.x NewLook
 * appearance. The menus will automatically use system colors and modern
 * white background styling.
 *
 * Returns: TRUE if successful, FALSE on failure
 */
static BOOL setup_newlook_menus(void)
{
	/* Open required libraries for standalone program */
	IntuitionBase = (struct Library *)OpenLibrary("intuition.library", 37L);
	if (!IntuitionBase)
	{
		Printf("Error: Could not open intuition.library v37+\n");
		return FALSE;
	} /* if */
	
	GadToolsBase = OpenLibrary("gadtools.library", 37L);
	if (!GadToolsBase)
	{
		Printf("Error: Could not open gadtools.library v37+\n");
		CloseLibrary(IntuitionBase);
		IntuitionBase = NULL;
		return FALSE;
	} /* if */
	
	/* Lock Workbench screen for menu system */
	wb_screen = LockPubScreen("Workbench");
	if (!wb_screen)
	{
		Printf("Error: Could not lock Workbench screen\n");
		return FALSE;
	} /* if */
	
	/* Get screen drawing information */
	draw_info = GetScreenDrawInfo(wb_screen);
	if (!draw_info)
	{
		Printf("Error: Could not get screen DrawInfo\n");
		UnlockPubScreen(NULL, wb_screen);
		wb_screen = NULL;
		return FALSE;
	} /* if */
	
	/* Get visual information for GadTools */
	visual_info = GetVisualInfo(wb_screen, TAG_END);
	if (!visual_info)
	{
		Printf("Error: Could not get VisualInfo\n");
		FreeScreenDrawInfo(wb_screen, draw_info);
		UnlockPubScreen(NULL, wb_screen);
		CloseLibrary(GadToolsBase);
		CloseLibrary(IntuitionBase);
		draw_info = NULL;
		wb_screen = NULL;
		GadToolsBase = NULL;
		IntuitionBase = NULL;
		return FALSE;
	} /* if */
	
	/* Create menu strip from template */
	menu_strip = CreateMenus(menu_template, TAG_END);
	if (!menu_strip)
	{
		Printf("Error: Could not create menu strip\n");
		FreeVisualInfo(visual_info);
		FreeScreenDrawInfo(wb_screen, draw_info);
		UnlockPubScreen(NULL, wb_screen);
		CloseLibrary(GadToolsBase);
		CloseLibrary(IntuitionBase);
		visual_info = NULL;
		draw_info = NULL;
		wb_screen = NULL;
		GadToolsBase = NULL;
		IntuitionBase = NULL;
		return FALSE;
	} /* if */
	
	/* Layout menus with NewLook appearance */
	if (!LayoutMenus(menu_strip, visual_info, GTMN_NewLookMenus, TRUE, TAG_END))
	{
		Printf("Error: Could not layout NewLook menus\n");
		FreeMenus(menu_strip);
		FreeVisualInfo(visual_info);
		FreeScreenDrawInfo(wb_screen, draw_info);
		UnlockPubScreen(NULL, wb_screen);
		CloseLibrary(GadToolsBase);
		CloseLibrary(IntuitionBase);
		menu_strip = NULL;
		visual_info = NULL;
		draw_info = NULL;
		wb_screen = NULL;
		GadToolsBase = NULL;
		IntuitionBase = NULL;
		return FALSE;
	} /* if */
	
	return TRUE;
} /* setup_newlook_menus */

/*------------------------------------------------------------------------*/

/**
 * cleanup_newlook_menus - Release all menu system resources
 * 
 * This function properly releases all resources allocated during
 * menu setup, following proper AmigaOS resource management.
 */
static void cleanup_newlook_menus(void)
{
	if (menu_strip)
	{
		FreeMenus(menu_strip);
		menu_strip = NULL;
	} /* if */
	
	if (visual_info)
	{
		FreeVisualInfo(visual_info);
		visual_info = NULL;
	} /* if */
	
	if (draw_info)
	{
		FreeScreenDrawInfo(wb_screen, draw_info);
		draw_info = NULL;
	} /* if */
	
	if (wb_screen)
	{
		UnlockPubScreen(NULL, wb_screen);
		wb_screen = NULL;
	} /* if */
	
	/* Close libraries opened during setup */
	if (GadToolsBase)
	{
		CloseLibrary(GadToolsBase);
		GadToolsBase = NULL;
	} /* if */
	
	if (IntuitionBase)
	{
		CloseLibrary(IntuitionBase);
		IntuitionBase = NULL;
	} /* if */
} /* cleanup_newlook_menus */

/*------------------------------------------------------------------------*/

/**
 * handle_menu_selection - Process menu item selections
 * 
 * This function handles menu item selections by examining the menu
 * item ID and performing the appropriate action. Customize the switch
 * statement for your application's menu items.
 *
 * @param menu_number: The menu selection number from IDCMP_MENUPICK
 * @return: TRUE to continue running, FALSE to quit application
 */
static BOOL handle_menu_selection(ULONG menu_number)
{
	struct MenuItem *menu_item = NULL;	/* Selected menu item pointer */
	ULONG item_id = 0;			/* Menu item ID */
	BOOL continue_running = TRUE;		/* Application continue flag */
	
	while (menu_number != MENUNULL)
	{
		menu_item = ItemAddress(menu_strip, menu_number);
		if (menu_item)
		{
			item_id = (ULONG)GTMENUITEM_USERDATA(menu_item);
			
			/* Handle menu selections - customize for your application */
			switch (item_id)
			{
				case MENU_OPEN:
					Printf("Open menu item selected\n");
					/* Add your Open handling code here */
					break;
					
				case MENU_SAVE:
					Printf("Save menu item selected\n");
					/* Add your Save handling code here */
					break;
					
				case MENU_PRINT:
					Printf("Print menu item selected\n");
					/* Add your Print handling code here */
					break;
					
				case MENU_QUIT:
					Printf("Quit selected - exiting application\n");
					continue_running = FALSE;
					break;
					
				default:
					Printf("Unknown menu item: %ld\n", item_id);
					break;
			} /* switch */
		} /* if */
		
		menu_number = menu_item->NextSelect;
	} /* while */
	
	return continue_running;
} /* handle_menu_selection */

/*------------------------------------------------------------------------*/

/**
 * main - Main program entry point
 * 
 * Creates a window with NewLook menus that automatically use the proper
 * Workbench 3.x appearance with white backgrounds and system colors.
 *
 * Returns: 0 on success, non-zero on error
 */
int main(void)
{
	struct Window *my_window = NULL;	/* Main window pointer */
	BOOL continue_running = TRUE;		/* Main loop flag */
	struct IntuiMessage *msg = NULL;	/* Intuition message pointer */
	ULONG msg_class = 0;			/* Message class storage */
	ULONG menu_number = 0;			/* Menu selection number */
	
	/* Setup NewLook menu system */
	if (!setup_newlook_menus())
	{
		Printf("Error: Could not setup NewLook menus\n");
		return 10;
	} /* if */
	
	/* Open window with NewLook menu support */
	my_window = OpenWindowTags(NULL,
		WA_Left, WINDOW_LEFT,
		WA_Top, WINDOW_TOP,
		WA_Width, WINDOW_WIDTH,
		WA_Height, WINDOW_HEIGHT,
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_REFRESHWINDOW | IDCMP_NEWSIZE,
		WA_Flags, WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_ACTIVATE,
		WA_Title, WINDOW_TITLE,
		WA_PubScreenName, "Workbench",
		WA_NewLookMenus, TRUE,  /* Enable NewLook menus for window */
		TAG_DONE);
	
	if (!my_window)
	{
		Printf("Error: Could not open window\n");
		cleanup_newlook_menus();
		return 10;
	} /* if */
	
	/* Attach menu strip to window */
	SetMenuStrip(my_window, menu_strip);
	
	Printf("NewLook menu template ready\n");
	
	/* Main event loop */
	{
	ULONG win_sig = 0;
	ULONG sigs = 0;

	win_sig = 1L << my_window->UserPort->mp_SigBit;

	while (continue_running)
	{
		sigs = Wait(win_sig);

		if (sigs & win_sig)
		{
			while ((msg = GT_GetIMsg(my_window->UserPort)) != NULL)
			{
				msg_class   = msg->Class;
				menu_number = msg->Code;
				GT_ReplyIMsg(msg);
				
				switch (msg_class)
				{
					case IDCMP_CLOSEWINDOW:
						continue_running = FALSE;
						break;
						
					case IDCMP_MENUPICK:
						continue_running = handle_menu_selection(menu_number);
						break;
						
					case IDCMP_REFRESHWINDOW:
						GT_BeginRefresh(my_window);
						GT_EndRefresh(my_window, TRUE);
						break;
						
					case IDCMP_NEWSIZE:
						/* Simple template: nothing special to do yet.
						 * More advanced apps may adjust layout here.
						 */
						break;
						
					default:
						break;
				} /* switch */
			} /* while messages */
		} /* if win_sig */
	} /* while */
}	
	/* Cleanup and exit */
	ClearMenuStrip(my_window);
	CloseWindow(my_window);
	cleanup_newlook_menus();
	
	return 0;
} /* main */

/*------------------------------------------------------------------------*/

#ifdef TEST
/**
 * Test function for NewLook menu functionality
 * Compile with: make test-compile
 */
int test_newlook_menus(void)
{
	Printf("Testing NewLook menu setup...\n");
	
	if (setup_newlook_menus())
	{
		Printf("NewLook menu setup successful\n");
		cleanup_newlook_menus();
		return 0;
	} /* if */
	else
	{
		Printf("NewLook menu setup failed\n");
		return 1;
	} /* else */
} /* test_newlook_menus */
#endif /* TEST */

/*------------------------------------------------------------------------*/
/* End of Text */