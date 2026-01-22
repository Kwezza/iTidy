/*
    Project:    ReBuild Tutorial
    File:       mydemo.c
    Purpose:    enhancing basics
    Author:     M. Bergmann
*/
#include <clib/macros.h>
#include <clib/alib_protos.h>
#include <clib/compiler-specific.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/icon.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/requester.h>    // NEW
#include <classes/requester.h>  // NEW

#include <libraries/gadtools.h>
#include <reaction/reaction.h>
#include <intuition/gadgetclass.h>
#include <reaction/reaction_macros.h>
#include <classes/window.h>
#include <exec/memory.h>

/* "speaking" name IDs */
#define BTN_BUTTON_1		7	// for first Button ID's value, see ReBuild's main window
#define BTN_BUTTON_2		9	// for second Button ID's value, see ReBuild's main window

/* --- "speaking" Menu identifiers --- */
enum menus { // Menu Project              
		MN_ABOUT, MN_CHECKME,
		MN_TOGGLE_ONE, MN_TOGGLE_TWO, MN_TOGGLE_THREE,
		MN_QUIT,           
	};	
	
/* --- GadgetHelp --- */
struct HintInfo hintinfo[] =
{
	{ BTN_BUTTON_1, -1, "I am Button 1, here to \nbore you with informations...", 0L},
	{ BTN_BUTTON_2, -1, "I'm Button 1's younger brother. \nWhat he does is good.", 0L},
	{ -1, -1, NULL, 0L}   // terminate!
};

// Function prototypes:
void window_3( void );
int doInfoRequest(struct Screen *screen, const char *title, 
             const char *body, const char *buttons, ULONG image );  // NEW

// Globals variables:
struct Screen	*gScreen = NULL;
struct DrawInfo	*gDrawInfo = NULL;
APTR gVisinfo = NULL;
struct MsgPort	*gAppPort = NULL;

struct Library *WindowBase = NULL,
               *ButtonBase = NULL,
               *RequesterBase = NULL,   // NEW
               *GadToolsBase = NULL,
               *LayoutBase = NULL,
               *IconBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;

/* copy doInfoRequest() function HERE! 
 *
 *  int doInfoRequest()
 *  - creates a requester.class INFO type Requester
 *  - returns ID of pressed Buttons  
 */
int doInfoRequest(struct Screen *screen, const char *title, const char *body, const char *buttons, ULONG image )
{  
	Object *req = 0;		// the requester itself
	int button;			// the button that was clicked by the user

	// fill in the requester structure
	req = NewObject(REQUESTER_GetClass(), NULL, 
		REQ_Type,       REQTYPE_INFO,
		REQ_TitleText,  (ULONG)title,
		REQ_BodyText,   (ULONG)body,
		REQ_GadgetText, (ULONG) buttons ,
		REQ_Image,      image,
		TAG_DONE);
        
	if (req) 
	{ 
		struct orRequest reqmsg;

		reqmsg.MethodID  = RM_OPENREQ;
		reqmsg.or_Attrs  = NULL;
		reqmsg.or_Window = NULL;
		reqmsg.or_Screen = screen;

		button = DoMethodA(req, (Msg) &reqmsg);
		DisposeObject(req);

		return button;

	}  else printf("[request] Failed to allocate requester\n");

	return 0;
}

/*
 *  int setup(void)
 *  - open libraries, initialize screen and window-handling
 *  - create a message port for IDCMP events  
 */
int setup( void )
{
  if( !(IntuitionBase = (struct IntuitionBase*) OpenLibrary("intuition.library",0L)) ) return 0;
  if( !(GadToolsBase = (struct Library*) OpenLibrary("gadtools.library",0L) ) ) return 0;
  if( !(WindowBase = (struct Library*) OpenLibrary("window.class",0L) ) ) return 0;
  if( !(IconBase = (struct Library*) OpenLibrary("icon.library",0L) ) ) return 0;
  if( !(LayoutBase = (struct Library*) OpenLibrary("gadgets/layout.gadget",0L) ) ) return 0;
  if( !(ButtonBase = (struct Library*) OpenLibrary("gadgets/button.gadget",0L) ) ) return 0;
  if( !(RequesterBase = (struct Library*) OpenLibrary("requester.class",0L) ) ) return 0;   // NEW
  if( !(gScreen = LockPubScreen( 0 ) ) ) return 0;
  if( !(gVisinfo = GetVisualInfo( gScreen, TAG_DONE ) ) ) return 0;
  if( !(gDrawInfo = GetScreenDrawInfo ( gScreen ) ) ) return 0;
  if( !(gAppPort = CreateMsgPort() ) ) return 0;

  return -1;
}


/*
 *  void cleanup(void)
 *  - close libraries
 *  - free window resources
 */
void cleanup( void )
{
  if ( gDrawInfo ) FreeScreenDrawInfo( gScreen, gDrawInfo);
  if ( gVisinfo ) FreeVisualInfo( gVisinfo );
  if ( gAppPort ) DeleteMsgPort( gAppPort );
  if ( gScreen ) UnlockPubScreen( 0, gScreen );

  if (GadToolsBase) CloseLibrary( (struct Library *)GadToolsBase );
  if (IconBase) CloseLibrary( (struct Library *)IconBase );
  if (IntuitionBase) CloseLibrary( (struct Library *)IntuitionBase );
  if (RequesterBase) CloseLibrary( (struct Library *)RequesterBase );   // NEW
  if (ButtonBase) CloseLibrary( (struct Library *)ButtonBase );
  if (LayoutBase) CloseLibrary( (struct Library *)LayoutBase );
  if (WindowBase) CloseLibrary( (struct Library *)WindowBase );
}


/*
 *  void runWindow (parameters...)
 *  - run a certain window
 *  - provide an IDCMP message handling loop  
 */
void runWindow( Object *window_object, int window_id, struct Menu *menu_strip, 
                struct Gadget *win_gadgets[], int gadget_ids[] )
{
  struct Window	*main_window = NULL;
  // we need a MenuItem struct to keep
  // track on status informations:
  struct MenuItem *menuitem = NULL; // NEW: initialize MenuItem structure

  if ( window_object )
  {
    if ( main_window = (struct Window *) RA_OpenWindow( window_object ))
    {
      WORD Code;
      ULONG wait = 0, signal = 0, result = 0, done = FALSE;
      ULONG req_result;
      
      GetAttr( WINDOW_SigMask, window_object, &signal );
      if ( menu_strip)  
          SetMenuStrip( main_window, menu_strip );
          
      /* --- Event Loop --- */
      while ( !done)
      {
        wait = Wait( signal | SIGBREAKF_CTRL_C );

        if ( wait & SIGBREAKF_CTRL_C )
          done = TRUE;
        else
          while (( result = RA_HandleInput( window_object, &Code )) != WMHI_LASTMSG)
          {
            switch ( result & WMHI_CLASSMASK )  // What kind of event occured?
            {
              case WMHI_CLOSEWINDOW:
                SetAttrs(window_object, WA_BusyPointer, TRUE, TAG_DONE); // block window
       			req_result = doInfoRequest(NULL, "Quit myDemo", 
                                   			" Do you really want to end this disaster?" \
                                   			"\n Select 'Quit' to leave me alone!", 
                                   			"_Quit|_Cancel", REQIMAGE_QUESTION);
       			printf("result was: %ld\n", req_result);
       			if (req_result == 1)
           			done = TRUE;
       			SetAttrs(window_object, WA_BusyPointer, FALSE, TAG_DONE); // de-block window
                break;

              /* --- if user selected a menu entry... ---*/
			  case WMHI_MENUPICK:
				  puts("\nYou selected some menu:"); 
				    
				  menuitem = ItemAddress(menu_strip, result & WMHI_MENUMASK);   // NEW: update struct MenuItem *
				  printf("DEBUG: ItemAdress = %ld\n", menuitem);
				  
				  // Check for menuitem equals to Zero. If Zero, an enforcer hit occours,
				  // so we do not allow switching on menuitem == 0!
				  if(!(menuitem == 0))
				  {             
    				  /* --- ...get menu entries by using GadTools' GTMENUITEM_USERDATA field macro --- */
    				  switch ((long)GTMENUITEM_USERDATA(ItemAddress(menu_strip, result & WMHI_MENUMASK)))
    				  {    				   	
    					case MN_CHECKME:
        					printf("DEBUG: ItemAdress = %ld\n", menuitem);
    						puts("check me");
    						/* evaluate, if item has checkmark set... */
                             if(menuitem->Flags & CHECKED)
                             {                           
                                puts("MN_CHECKME is CHECKED!");
                             }
                             else
                                 puts("MN_CHECKME is not CHECKED!.");
    						break;
    					case MN_TOGGLE_ONE:
    						puts("One");
    						break;	
    					case MN_TOGGLE_TWO:
    						puts("Two");
    						break;
    					case MN_TOGGLE_THREE:
    						puts("Three");
    						break;	
    					case MN_QUIT:
    						SetAttrs(window_object, WA_BusyPointer, TRUE, TAG_DONE);  // block calling window
                   			
                   			// call Requester:
                   			SetAttrs(window_object, WA_BusyPointer, TRUE, TAG_DONE); // block window
                   			req_result = 
                   			doInfoRequest(NULL, "Quit myDemo", 
                                       			" Do you really want to end this disaster?" \
                                       			"\n Select 'Quit' to leave me alone!", 
                                       			"_Quit|_Cancel", REQIMAGE_QUESTION);
                   			printf("result was: %ld\n", req_result);
                   			if (req_result == 1)
                       			done = TRUE;
                   			SetAttrs(window_object, WA_BusyPointer, FALSE, TAG_DONE); // de-block window
    						break;	
						case MN_ABOUT:
        				   	printf("DEBUG: ItemAdress = %ld\n", menuitem);
    						puts("About");
    						SetAttrs(window_object, WA_BusyPointer, TRUE, TAG_DONE);
                           			doInfoRequest(NULL, "About myDemo", 
                                   			" myDemo v1.o\n\nsecond ReBuild tutorial Demo.", 
                                   			"Alright then...", REQIMAGE_INFO);
                           	SetAttrs(window_object, WA_BusyPointer, FALSE, TAG_DONE);
                           	done = FALSE;	// Workaround for ReAction stack corruption bug                           			
    						break;	
    						
    					default:
        					puts("Something strange happened - please debug me!");
        			}		  
				 } 
				break;  

              case WMHI_GADGETUP:
        		puts("gadget press");
        		switch (result & WMHI_GADGETMASK)	// WHICH gadget caused the event?
        		{
        			// Button 1:
        			case BTN_BUTTON_1:			// Button 1 ID
        			puts("Button 1 was clicked");
        			break;
        			// Button 2:
        			case BTN_BUTTON_2:			// Button 2 ID
        			puts("Button 2 was clicked");
        			break;
        		}
            	break;

              case WMHI_ICONIFY:
                if ( RA_Iconify( window_object ) )
                  main_window = NULL;
                break;

              case WMHI_UNICONIFY:
                main_window = RA_OpenWindow( window_object );
                if ( menu_strip)  SetMenuStrip( main_window, menu_strip );
              break;

            }
          }
      }
    }
  }
}


/*
 *  void window_xyz(void)
 *  - defines menues, window and layout
 *  - calls runWindow()
 *  - frees ressources before quitting the app
 */
void window_3( void )
{
  struct NewMenu menuData[] =
  {
    { NM_TITLE, "Project",0,0,0,NULL },
    { NM_ITEM, "About..","A",0,0,(APTR)MN_ABOUT },
    { NM_ITEM, NM_BARLABEL,0,0,0,NULL },
    { NM_ITEM, "Check me","E",CHECKIT|MENUTOGGLE,0,(APTR)MN_CHECKME },
    { NM_ITEM, "Toggle me",0,0,0,NULL },
    { NM_SUB, "One","1",CHECKIT|CHECKED,7 - 1,(APTR)MN_TOGGLE_ONE },
    { NM_SUB, "Two","2",CHECKIT,7 - 2,(APTR)MN_TOGGLE_TWO },
    { NM_SUB, "Three","3",CHECKIT,7 - 4,(APTR)MN_TOGGLE_THREE },
    { NM_ITEM, NM_BARLABEL,0,0,0,NULL },
    { NM_ITEM, "Quit","Q",0,0,(APTR)MN_QUIT },
    { NM_END, NULL, 0, 0, 0, (APTR)0 }
  };

  struct Menu	*menu_strip = NULL;
  struct Gadget	*main_gadgets[ 5 ];
  int gadget_ids[ 5 ];
  Object *window_object = NULL;
  menu_strip = CreateMenusA( menuData, TAG_END );
  LayoutMenus( menu_strip, gVisinfo,
    GTMN_NewLookMenus, TRUE,
    TAG_DONE );


  window_object = WindowObject,
    WA_Title, "myDemo v1.0",
    WA_ScreenTitle, "Just a demo...",
    WA_Left, 5,
    WA_Top, 20,
    WA_Width, 200,
    WA_Height, 25,
    WA_MinWidth, 200,
    WA_MinHeight, 25,
    WA_MaxWidth, 8192,
    WA_MaxHeight, 25,
    WINDOW_LockHeight, TRUE,
    WINDOW_IconifyGadget, TRUE, 
    WINDOW_HintInfo, &hintinfo, // NEW
    WINDOW_GadgetHelp, TRUE,    // NEW
    WINDOW_AppPort, gAppPort,
    WINDOW_IconifyGadget, TRUE,
    WA_CloseGadget, TRUE,
    WA_DepthGadget, TRUE,
    WA_SizeGadget, TRUE,
    WA_DragBar, TRUE,
    WA_Activate, TRUE,
    WA_SizeBBottom, TRUE,
    WINDOW_Position, WPOS_TOPLEFT,
    WINDOW_IconTitle, "myDemo",
    WINDOW_Icon,  GetDiskObject("myDemo"),
    WA_NoCareRefresh, TRUE,
    WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_MENUPICK,
    WINDOW_ParentGroup, VLayoutObject,
    LAYOUT_SpaceOuter, TRUE,
    LAYOUT_DeferLayout, TRUE,
      LAYOUT_AddChild, main_gadgets[0] = LayoutObject,
        GA_ID, gadget_ids[0] = 5,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_ShrinkWrap, TRUE,
        LAYOUT_DeferLayout, TRUE,
        LAYOUT_AddChild, main_gadgets[1] = LayoutObject,
          GA_ID, gadget_ids[1] = 6,
          LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
          LAYOUT_EvenSize, TRUE,
          LAYOUT_DeferLayout, TRUE,
          LAYOUT_AddChild, main_gadgets[2] = ButtonObject,
            GA_ID, gadget_ids[2] = 7, // identifies first button with ID "7"
            GA_Text, "B_utton 1",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          ButtonEnd,
          LAYOUT_AddChild, main_gadgets[3] = ButtonObject,
            GA_ID, gadget_ids[3] = 9,   // identifies second button with ID "9"
            GA_Text, "Butt_on 2",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          ButtonEnd,
        LayoutEnd,
      LayoutEnd,
    LayoutEnd,
  WindowEnd;  
  main_gadgets[4] = 0;
  gadget_ids[4] = 0;

  // 
  runWindow( window_object, 3, menu_strip, main_gadgets, gadget_ids );

  if ( window_object ) DisposeObject( window_object );
  if ( menu_strip ) FreeMenus( menu_strip );
}


/* --- main routine --- */
int main( int argc, char **argv )
{
  if ( setup() )
  {
    window_3();
  }
  cleanup();
}
