/*
    Project:    ReBuild Tutorial
    File:       cbdemo.c
    Purpose:    explaination basis, first program
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
#include <proto/checkbox.h>
#include <proto/string.h>
#include <proto/label.h>
#include <proto/requester.h>

#include <classes/requester.h>

#include <libraries/gadtools.h>
#include <reaction/reaction.h>
#include <intuition/gadgetclass.h>
#include <reaction/reaction_macros.h>
#include <classes/window.h>
#include <exec/memory.h>

/* "speaking" main_gadgets[] index constants */
#define GADS_CB_ENE 	3
#define GADS_CB_MENE	4
#define GADS_CB_MUH		5
#define GADS_STR_ENE 	7
#define GADS_STR_MENE	8
#define GADS_STR_MUH	9
/* "speaking" gadget ID constants */
#define CB_ENE      29
#define CB_MENE     30
#define CB_MUH      31
#define STR_ENE     33
#define STR_MENE    34
#define STR_MUH     35
#define BTN_STATUS  37  

/* Menu identifiers */
enum menus {              
             // Menu "Project"
             MN_ENE, 
             MN_MENE,           
             MN_MUH,   
             MN_QUIT, 
             // Menu "Help"
             MN_ABOUT,                              
           };

/* GadgetHelp */
struct HintInfo hintinfo[] =
{
	{ CB_ENE, -1, "I am  CheckBox #1, setting status text in String #1", 0L},
	{ CB_MENE, -1, "I am  CheckBox #2, setting status text in String #2", 0L},
	{ CB_MUH, -1, "I am  CheckBox #3, setting status text in String #3", 0L},
	{ BTN_STATUS, -1, "I have no real effect, since setting status" \ 
                      "\ntexts is done by the CheckBoxes already!", 0L},
	{ -1, -1, NULL, 0L}   // terminate!
};



/* Function Prototypes */
void window_cb( void );

void getCheckBoxStatus(struct Window *win, 
                        struct Gadget *chkgad, 
                        struct Gadget *txtgad);
                        
int doInfoRequest(struct Screen *screen, const char *title, 
        const char *body, const char *buttons, 
        ULONG image );


/* other global assignments */
struct Screen	*gScreen = NULL;
struct DrawInfo	*gDrawInfo = NULL;
APTR gVisinfo = NULL;
struct MsgPort	*gAppPort = NULL;

struct Library *WindowBase = NULL,
               *ButtonBase = NULL,
               *CheckBoxBase = NULL,
               *StringBase = NULL,
               *LabelBase = NULL,
               *RequesterBase,
               *GadToolsBase = NULL,
               *LayoutBase = NULL,
               *IconBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;


/*
 * Function: void GetCheckBoxStatus( ... )
 * Purpose:  get CheckBox status, set String text
*/
void getCheckBoxStatus(struct Window *win, struct Gadget *chkgad, struct Gadget *txtgad)
{
    ULONG status = 0;
    ULONG success = 0;
    STRPTR isChecked = "empty";
    
    GetAttr(CHECKBOX_Checked, (APTR)chkgad, &status);    
    switch (status)
    {
        case 0:
            isChecked = "unchecked";
            break;
        case 1:
            isChecked = "checked";
            break;        
    }
    success = SetGadgetAttrs((APTR)txtgad, win, 0, STRINGA_TextVal, isChecked, TAG_END );     
    
    printf("Status: %ld\n\n", status);
    printf("success: %ld\n\n", success);    
}

/*
 * Function: int doInfoRequest( ... )
 * Purpose:  display Requester, return button value
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

	}  else 
    	printf("[doInfoRequest] Failed to allocate requester\n");

	return 0;
}


/*
 * Function: int setup()
 * Purpose:  open libs, prepare app
*/
int setup( void )
{
  if( !(IntuitionBase = (struct IntuitionBase*) OpenLibrary("intuition.library",0L)) ) return 0;
  if( !(GadToolsBase = (struct Library*) OpenLibrary("gadtools.library",0L) ) ) return 0;
  if( !(WindowBase = (struct Library*) OpenLibrary("window.class",0L) ) ) return 0;
  if( !(IconBase = (struct Library*) OpenLibrary("icon.library",0L) ) ) return 0;
  if( !(LayoutBase = (struct Library*) OpenLibrary("gadgets/layout.gadget",0L) ) ) return 0;
  if( !(ButtonBase = (struct Library*) OpenLibrary("gadgets/button.gadget",0L) ) ) return 0;
  if( !(CheckBoxBase = (struct Library*) OpenLibrary("gadgets/checkbox.gadget",0L) ) ) return 0;
  if( !(StringBase = (struct Library*) OpenLibrary("gadgets/string.gadget",0L) ) ) return 0;
  if( !(LabelBase = (struct Library*) OpenLibrary("images/label.image",0L) ) ) return 0;
  if( !(RequesterBase = (struct Library*) OpenLibrary("requester.class",0L) ) ) return 0;
  if( !(gScreen = LockPubScreen( 0 ) ) ) return 0;
  if( !(gVisinfo = GetVisualInfo( gScreen, TAG_DONE ) ) ) return 0;
  if( !(gDrawInfo = GetScreenDrawInfo ( gScreen ) ) ) return 0;
  if( !(gAppPort = CreateMsgPort() ) ) return 0;

  return -1;
}

/*
 * Function: void cleanup()
 * Purpose:  close libs, clean up things
*/
void cleanup( void )
{
  if ( gDrawInfo ) FreeScreenDrawInfo( gScreen, gDrawInfo);
  if ( gVisinfo ) FreeVisualInfo( gVisinfo );
  if ( gAppPort ) DeleteMsgPort( gAppPort );
  if ( gScreen ) UnlockPubScreen( 0, gScreen );

  if (GadToolsBase) CloseLibrary( (struct Library *)GadToolsBase );
  if (RequesterBase) CloseLibrary( (struct Library *)RequesterBase );
  if (IconBase) CloseLibrary( (struct Library *)IconBase );
  if (IntuitionBase) CloseLibrary( (struct Library *)IntuitionBase );
  if (ButtonBase) CloseLibrary( (struct Library *)ButtonBase );
  if (CheckBoxBase) CloseLibrary( (struct Library *)CheckBoxBase );
  if (StringBase) CloseLibrary( (struct Library *)StringBase );
  if (LabelBase) CloseLibrary( (struct Library *)LabelBase );
  if (LayoutBase) CloseLibrary( (struct Library *)LayoutBase );
  if (WindowBase) CloseLibrary( (struct Library *)WindowBase );
}


/*
 * Function: void runWindow( ... )
 * Purpose:  open window, handle event loop
*/
void runWindow( Object *window_object, int window_id, 
                struct Menu *menu_strip, struct Gadget *win_gadgets[] )
{
  struct Window	*main_window = NULL;
  // we need a MenuItem struct to keep
  // track on status informations:
  struct MenuItem *menuitem = NULL;

  if ( window_object )
  {
    if ( main_window = (struct Window *) RA_OpenWindow( window_object ))
    {
      WORD Code;
      ULONG wait = 0, signal = 0, result = 0, done = FALSE;
      ULONG req_result = NULL;

      GetAttr( WINDOW_SigMask, window_object, &signal );
      if ( menu_strip)  SetMenuStrip( main_window, menu_strip );
      while ( !done)
      {
        wait = Wait( signal | SIGBREAKF_CTRL_C );

        if ( wait & SIGBREAKF_CTRL_C )
          done = TRUE;
        else
          while (( result = RA_HandleInput( window_object, &Code )) != WMHI_LASTMSG)
          {
            switch ( result & WMHI_CLASSMASK )
            {
              case WMHI_CLOSEWINDOW:
                SetAttrs(window_object, WA_BusyPointer, TRUE, TAG_DONE); // block window
       			req_result = doInfoRequest(NULL, "Quit cbDemo", 
                                   			" Do you really want to end this disaster?" \
                                   			"\n Select 'Quit' to leave me alone!", 
                                   			"_Quit|_Cancel", REQIMAGE_QUESTION);
       			printf("result was: %ld\n", req_result);
       			if (req_result == 1)
           			done = TRUE;
       			SetAttrs(window_object, WA_BusyPointer, FALSE, TAG_DONE); // de-block window
                break;

              case WMHI_MENUPICK:
                puts("menu pick");
                menuitem = ItemAddress(menu_strip, result & WMHI_MENUMASK);
                if(!(menuitem == 0)) // check against NULL, otherwise enforcer hit!
        			  {
                    switch ((long)GTMENUITEM_USERDATA(menuitem))
                    {
                        case MN_QUIT:
                            SetAttrs(window_object, WA_BusyPointer, TRUE, TAG_DONE); // block window
                   			req_result = 
                   			doInfoRequest(NULL, "Quit cbDemo", 
                                       			" Do you really want to end this disaster?" \
                                       			"\n Select 'Quit' to leave me alone!", 
                                       			"_Quit|_Cancel", REQIMAGE_QUESTION);
                   			printf("result was: %ld\n", req_result);
                   			if (req_result == 1)
                       			done = TRUE;
                   			SetAttrs(window_object, WA_BusyPointer, FALSE, TAG_DONE); // de-block window
                            break; 
                        case MN_ENE:
                            puts("Project Menu: Ene selected");
                            /* evaluate, if item has checkmark set... */
                             if(menuitem->Flags & CHECKED)
                             {                           
                                puts("MenuItem is CHECKED.");
                             }
                             else
                                 puts("MenuItem is NOT CHECKED.");
                            break;
                            
                        case MN_MENE:
                            puts("Project Menu: Mene selected");
                            if(menuitem->Flags & CHECKED)
                             {                           
                                puts("MenuItem is CHECKED.");
                             }
                             else
                                 puts("MenuItem is NOT CHECKED.");
                            break;
                            
                        case MN_MUH:
                            puts("Project Menu: Muh selected");
                            if(menuitem->Flags & CHECKED)
                             {                           
                                puts("MenuItem is CHECKED.");
                             }
                             else
                                 puts("MenuItem is NOT CHECKED.");
                            break;
                            
                        case MN_ABOUT:
                            puts("Help Menu: About selected");
                            SetAttrs(window_object, WA_BusyPointer, TRUE, TAG_DONE);
                   			doInfoRequest(NULL, "About cbDemo", 
                                   			" cbDemo v1.o\n\nfirst ReBuild tutorial Demo.", 
                                   			"Alright then...", REQIMAGE_INFO);
                   			SetAttrs(window_object, WA_BusyPointer, FALSE, TAG_DONE);
                            break;
                            
                        default:
                					puts("Something strange happened - please debug me!");
                    }
                }
                break;

              case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)	// WHICH gadget caused the event?
        		{
        			// CheckBox 1:
        			case CB_ENE:			      
            			puts("CheckBox 1 was clicked");
            			// example call with "speaking" index constants:
            			getCheckBoxStatus(main_window, win_gadgets[GADS_CB_ENE], win_gadgets[GADS_STR_ENE]);
            			break;
        			// CheckBox 2:
        			case CB_MENE:			      
            			puts("CheckBox 2 was clicked");
            			// example call with "speaking" index constants:
            			getCheckBoxStatus(main_window, win_gadgets[GADS_CB_MENE], win_gadgets[GADS_STR_MENE]);
            			break;
            		// CheckBox 3:
        			case CB_MUH:			      
            			puts("CheckBox 3 was clicked");
            			// example call with "speaking" index constants:
            			getCheckBoxStatus(main_window, win_gadgets[GADS_CB_MUH], win_gadgets[GADS_STR_MUH]);
            			break;
            		// Button:
        			case BTN_STATUS:			      
            			puts("Status Button was clicked - you won't notice anything!");
            			// example calls, using index numbers
            			getCheckBoxStatus(main_window, win_gadgets[3], win_gadgets[7]);
               			getCheckBoxStatus(main_window, win_gadgets[4], win_gadgets[8]);
               			getCheckBoxStatus(main_window, win_gadgets[5], win_gadgets[9]);
            			break;
        		}
                break;
            }
          }
      }
    }
  }
}


/*
 * Function: void window_cb
 * Purpose:  define window, menus, layout and gadgets
*/
void window_cb( void )
{
  struct NewMenu menuData[] =
  {
    { NM_TITLE, "Action",0,0,0,NULL },
    { NM_ITEM, "Ene","1",CHECKIT|MENUTOGGLE,0,(APTR)MN_ENE },
    { NM_ITEM, "Mene","2",CHECKIT|MENUTOGGLE,0,(APTR)MN_MENE },
    { NM_ITEM, "Muh","3",CHECKIT|MENUTOGGLE,0,(APTR)MN_MUH },
    { NM_ITEM, NM_BARLABEL,0,0,0,NULL },
    { NM_ITEM, "Quit","Q",0,0,(APTR)MN_QUIT },
    { NM_TITLE, "Help",0,0,0,NULL },
    { NM_ITEM, "About...","?",0,0,(APTR)MN_ABOUT },
    { NM_END, NULL, 0, 0, 0, (APTR)0 }
  };

  struct Menu	*menu_strip = NULL;
  struct Gadget	*main_gadgets[ 13 ];
  int gadget_ids[ 13 ];
  Object *window_object = NULL;
  menu_strip = CreateMenusA( menuData, TAG_END );
  LayoutMenus( menu_strip, gVisinfo,
    GTMN_NewLookMenus, TRUE,
    TAG_DONE );


  window_object = WindowObject,
    WA_Title, "CheckBox Demo",
    WA_ScreenTitle, "CheckBox Demo v1.0",
    WA_Left, 5,
    WA_Top, 20,
    WA_Width, 120,
    WA_Height, 20,
    WA_MinWidth, 120,
    WA_MinHeight, 20,
    WA_MaxWidth, 8192,
    WA_MaxHeight, 20,
    WINDOW_LockHeight, TRUE,
    WINDOW_HintInfo, &hintinfo, 
    WINDOW_GadgetHelp, TRUE,    
    WINDOW_AppPort, gAppPort,
    WA_CloseGadget, TRUE,
    WA_DepthGadget, TRUE,
    WA_SizeGadget, TRUE,
    WA_DragBar, TRUE,
    WA_Activate, TRUE,
    WA_SizeBBottom, TRUE,
    WINDOW_Position, WPOS_TOPLEFT,
    WINDOW_IconTitle, "cbDemo",
    WA_NoCareRefresh, TRUE,
    WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_MENUPICK,
    WINDOW_ParentGroup, VLayoutObject,
    LAYOUT_SpaceOuter, TRUE,
    LAYOUT_DeferLayout, TRUE,
      LAYOUT_AddChild, main_gadgets[0] = LayoutObject,
        GA_ID, gadget_ids[0] = 5,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_LeftSpacing, 4,
        LAYOUT_RightSpacing, 4,
        LAYOUT_TopSpacing, 4,
        LAYOUT_BottomSpacing, 4,
        LAYOUT_ShrinkWrap, TRUE,
        LAYOUT_DeferLayout, TRUE,
        LAYOUT_AddChild, main_gadgets[1] = LayoutObject,
          GA_ID, gadget_ids[1] = 27,
          LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
          LAYOUT_AddChild, main_gadgets[2] = LayoutObject,
            GA_ID, gadget_ids[2] = 28,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_BevelStyle, BVS_GROUP,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_LeftSpacing, 2,
            LAYOUT_RightSpacing, 2,
            LAYOUT_TopSpacing, 2,
            LAYOUT_BottomSpacing, 2,
            LAYOUT_Label, "CheckBoxes",
            LAYOUT_DeferLayout, TRUE,
            LAYOUT_AddChild, main_gadgets[3] = CheckBoxObject,
              GA_ID, gadget_ids[3] = 29,
              GA_Text, "_Ene",
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              GA_Selected, TRUE,
              CHECKBOX_TextPlace, PLACETEXT_RIGHT,
            CheckBoxEnd,
            LAYOUT_AddChild, main_gadgets[4] = CheckBoxObject,
              GA_ID, gadget_ids[4] = 30,
              GA_Text, "_Mene",
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              CHECKBOX_TextPlace, PLACETEXT_RIGHT,
            CheckBoxEnd,
            LAYOUT_AddChild, main_gadgets[5] = CheckBoxObject,
              GA_ID, gadget_ids[5] = 31,
              GA_Text, "M_uh",
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              CHECKBOX_TextPlace, PLACETEXT_RIGHT,
            CheckBoxEnd,
          LayoutEnd,
          LAYOUT_AddChild, main_gadgets[6] = LayoutObject,
            GA_ID, gadget_ids[6] = 32,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_BevelStyle, BVS_GROUP,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_LeftSpacing, 2,
            LAYOUT_RightSpacing, 2,
            LAYOUT_TopSpacing, 2,
            LAYOUT_BottomSpacing, 2,
            LAYOUT_Label, "Status",
            LAYOUT_DeferLayout, TRUE,
            LAYOUT_AddChild, main_gadgets[7] = StringObject,
              GA_ID, gadget_ids[7] = 33,
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              GA_ReadOnly, TRUE,
              STRINGA_TextVal, "checked",
              STRINGA_MaxChars, 80,
              STRINGA_MinVisible, 9,
            StringEnd,
            CHILD_Label, LabelObject,
              LABEL_Text, "Ene:",
            LabelEnd,
            LAYOUT_AddChild, main_gadgets[8] = StringObject,
              GA_ID, gadget_ids[8] = 34,
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              GA_ReadOnly, TRUE,
              STRINGA_TextVal, "unchecked",
              STRINGA_MaxChars, 80,
              STRINGA_MinVisible, 9,
            StringEnd,
            CHILD_Label, LabelObject,
              LABEL_Text, "Mene:",
            LabelEnd,
            LAYOUT_AddChild, main_gadgets[9] = StringObject,
              GA_ID, gadget_ids[9] = 35,
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              GA_ReadOnly, TRUE,
              STRINGA_TextVal, "unchecked",
              STRINGA_MaxChars, 80,
              STRINGA_MinVisible, 9,
            StringEnd,
            CHILD_Label, LabelObject,
              LABEL_Text, "Muh:",
            LabelEnd,
          LayoutEnd,
        LayoutEnd,
        LAYOUT_AddChild, main_gadgets[10] = LayoutObject,
          GA_ID, gadget_ids[10] = 36,
          LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
          LAYOUT_SpaceOuter, TRUE,
          LAYOUT_LeftSpacing, 6,
          LAYOUT_RightSpacing, 6,
          LAYOUT_TopSpacing, 6,
          LAYOUT_BottomSpacing, 1,
          LAYOUT_AddChild, main_gadgets[11] = ButtonObject,
            GA_ID, gadget_ids[11] = 37,
            GA_Text, "_Get Status",
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
  main_gadgets[12] = 0;
  gadget_ids[12] = 0;

  //runWindow( window_object, 3, menu_strip, main_gadgets, gadget_ids );
  runWindow( window_object, 3, menu_strip, main_gadgets );

  if ( window_object ) DisposeObject( window_object );
  if ( menu_strip ) FreeMenus( menu_strip );
}


/*
 * Function: int main( ... )
 * Purpose:  manage flow
*/
int main( int argc, char **argv )
{
  if ( setup() )
  {
    window_cb();
  }
  cleanup();
}
