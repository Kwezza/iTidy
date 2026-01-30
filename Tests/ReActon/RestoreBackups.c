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
#include <proto/listbrowser.h>
#include <proto/label.h>

#include <libraries/gadtools.h>
#include <reaction/reaction.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <reaction/reaction_macros.h>
#include <classes/window.h>
#include <exec/memory.h>

void restore_backups_window( void );

struct Screen	*gScreen = NULL;
struct DrawInfo	*gDrawInfo = NULL;
APTR gVisinfo = NULL;
struct MsgPort	*gAppPort = NULL;

struct List *BrowserNodesA(STRPTR *nameList, int colCount)
{
  struct List *newList;
  newList = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC );

  if (newList && nameList)
  {
    NewList( newList );
    while(*nameList)
    {
      AddTail(newList, AllocListBrowserNode(colCount, LBNCA_Text, *nameList, TAG_END));
      nameList++;
    }
  }
  return newList;
}

void FreeBrowserNodes(struct List *list)
{
  
    if (list)
    {
      FreeListBrowserList(list);
      FreeMem(list, sizeof (struct List));
    }
}

struct Library *WindowBase = NULL,
               *ButtonBase = NULL,
               *ListBrowserBase = NULL,
               *LabelBase = NULL,
               *GadToolsBase = NULL,
               *LayoutBase = NULL,
               *IconBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;

//window ids
enum win { restore_backups_id = 103 };

//restore_backups gadgets
enum restore_backups_idx { vert_105, backup_list_layout, backup_list, backup_details_layout, 
  backup_details, backup_buttons_row1_layout, backup_delete_run, 
  backup_restore_run, backup_view_folders, button_cancel };

enum restore_backups_gid { vert_105_id = 105, backup_list_layout_id = 106, backup_list_id = 107, 
  backup_details_layout_id = 108, backup_details_id = 109, backup_buttons_row1_layout_id = 110, 
  backup_delete_run_id = 111, backup_restore_run_id = 112, backup_view_folders_id = 113, 
  button_cancel_id = 114 };

int setup( void )
{
  if( !(IntuitionBase = (struct IntuitionBase*) OpenLibrary("intuition.library",0L)) ) return 0;
  if( !(GadToolsBase = (struct Library*) OpenLibrary("gadtools.library",0L) ) ) return 0;
  if( !(WindowBase = (struct Library*) OpenLibrary("window.class",0L) ) ) return 0;
  if( !(IconBase = (struct Library*) OpenLibrary("icon.library",0L) ) ) return 0;
  if( !(LayoutBase = (struct Library*) OpenLibrary("gadgets/layout.gadget",0L) ) ) return 0;
  if( !(ButtonBase = (struct Library*) OpenLibrary("gadgets/button.gadget",0L) ) ) return 0;
  if( !(ListBrowserBase = (struct Library*) OpenLibrary("gadgets/listbrowser.gadget",0L) ) ) return 0;
  if( !(LabelBase = (struct Library*) OpenLibrary("images/label.image",0L) ) ) return 0;
  if( !(gScreen = LockPubScreen( 0 ) ) ) return 0;
  if( !(gVisinfo = GetVisualInfo( gScreen, TAG_DONE ) ) ) return 0;
  if( !(gDrawInfo = GetScreenDrawInfo ( gScreen ) ) ) return 0;
  if( !(gAppPort = CreateMsgPort() ) ) return 0;

  return -1;
}

void cleanup( void )
{
  if ( gDrawInfo ) FreeScreenDrawInfo( gScreen, gDrawInfo);
  if ( gVisinfo ) FreeVisualInfo( gVisinfo );
  if ( gAppPort ) DeleteMsgPort( gAppPort );
  if ( gScreen ) UnlockPubScreen( 0, gScreen );

  if (GadToolsBase) CloseLibrary( (struct Library *)GadToolsBase );
  if (IconBase) CloseLibrary( (struct Library *)IconBase );
  if (IntuitionBase) CloseLibrary( (struct Library *)IntuitionBase );
  if (ButtonBase) CloseLibrary( (struct Library *)ButtonBase );
  if (ListBrowserBase) CloseLibrary( (struct Library *)ListBrowserBase );
  if (LabelBase) CloseLibrary( (struct Library *)LabelBase );
  if (LayoutBase) CloseLibrary( (struct Library *)LayoutBase );
  if (WindowBase) CloseLibrary( (struct Library *)WindowBase );
}

void runWindow( Object *window_object, int window_id, struct Menu *menu_strip, struct Gadget *win_gadgets[] )
{
  struct Window	*main_window = NULL;

  if ( window_object )
  {
    if ( main_window = (struct Window *) RA_OpenWindow( window_object ))
    {
      WORD Code;
      ULONG wait = 0, signal = 0, result = 0, done = FALSE;
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
                done = TRUE;
                break;

              case WMHI_MENUPICK:
                puts("menu pick");
                break;

              case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                  case backup_delete_run_id:
                    puts("Delete run button pressed");
                    break;
                  
                  case backup_restore_run_id:
                    puts("Restore run button pressed");
                    break;
                  
                  case backup_view_folders_id:
                    puts("View folders button pressed");
                    break;
                  
                  case button_cancel_id:
                    puts("Cancel button pressed");
                    done = TRUE;
                    break;
                  
                  default:
                    puts("gadget press");
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

void restore_backups_window( void )
{
  struct Gadget	*main_gadgets[ 11 ];
  Object *window_object = NULL;
  struct HintInfo hintInfo[] =
  {
    {vert_105_id,-1,"",0},
    {backup_list_layout_id,-1,"",0},
    {backup_list_id,-1,"",0},
    {backup_details_layout_id,-1,"",0},
    {backup_details_id,-1,"",0},
    {backup_buttons_row1_layout_id,-1,"",0},
    {backup_delete_run_id,-1,"",0},
    {backup_restore_run_id,-1,"",0},
    {backup_view_folders_id,-1,"",0},
    {button_cancel_id,-1,"",0},
    {-1,-1,NULL,0}
  };
  struct List *labels107;
  struct List *labels109;
  struct ColumnInfo ListBrowser107_ci[] =
  {
    { 25, "Date", 0 },
    { 15, "Time", 0 },
    { 35, "Folders", 0 },
    { 25, "Size", 0 },
    { -1, (STRPTR)~0, -1 }
  };
  struct ColumnInfo ListBrowser109_ci[] =
  {
    { 30, "Path", 0 },
    { 15, "Icons", 0 },
    { 20, "Status", 0 },
    { 35, "Notes", 0 },
    { -1, (STRPTR)~0, -1 }
  };

  /* Create backup list with 4 columns */
  labels107 = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC);
  if (labels107)
  {
    struct Node *node;
    NewList(labels107);
    
    node = AllocListBrowserNode(4,
      LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, "28-Jan-2026",
      LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, "14:23:15",
      LBNA_Column, 2, LBNCA_CopyText, TRUE, LBNCA_Text, "12",
      LBNA_Column, 3, LBNCA_CopyText, TRUE, LBNCA_Text, "2.4 MB",
      TAG_END);
    if (node) AddTail(labels107, node);
    
    node = AllocListBrowserNode(4,
      LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, "27-Jan-2026",
      LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, "09:15:42",
      LBNA_Column, 2, LBNCA_CopyText, TRUE, LBNCA_Text, "8",
      LBNA_Column, 3, LBNCA_CopyText, TRUE, LBNCA_Text, "1.8 MB",
      TAG_END);
    if (node) AddTail(labels107, node);
    
    node = AllocListBrowserNode(4,
      LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, "26-Jan-2026",
      LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, "16:48:03",
      LBNA_Column, 2, LBNCA_CopyText, TRUE, LBNCA_Text, "5",
      LBNA_Column, 3, LBNCA_CopyText, TRUE, LBNCA_Text, "956 KB",
      TAG_END);
    if (node) AddTail(labels107, node);
  }
  
  /* Create details list with 4 columns */
  labels109 = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC);
  if (labels109)
  {
    struct Node *node;
    NewList(labels109);
    
    node = AllocListBrowserNode(4,
      LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, "Work:Projects/iTidy",
      LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, "42",
      LBNA_Column, 2, LBNCA_CopyText, TRUE, LBNCA_Text, "Backed up",
      LBNA_Column, 3, LBNCA_CopyText, TRUE, LBNCA_Text, "Complete",
      TAG_END);
    if (node) AddTail(labels109, node);
    
    node = AllocListBrowserNode(4,
      LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, "Work:Documents",
      LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, "28",
      LBNA_Column, 2, LBNCA_CopyText, TRUE, LBNCA_Text, "Backed up",
      LBNA_Column, 3, LBNCA_CopyText, TRUE, LBNCA_Text, "Complete",
      TAG_END);
    if (node) AddTail(labels109, node);
    
    node = AllocListBrowserNode(4,
      LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, "Work:Utils",
      LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, "15",
      LBNA_Column, 2, LBNCA_CopyText, TRUE, LBNCA_Text, "Backed up",
      LBNA_Column, 3, LBNCA_CopyText, TRUE, LBNCA_Text, "Complete",
      TAG_END);
    if (node) AddTail(labels109, node);
  }

  window_object = NewObject( WINDOW_GetClass(), NULL, 
    WA_Title, "iTidy - Restore backups",
    WA_Left, 5,
    WA_Top, 20,
    WA_Width, 500,
    WA_Height, 400,
    WA_MinWidth, 350,
    WA_MinHeight, 300,
    WA_MaxWidth, 8192,
    WA_MaxHeight, 8192,
    WINDOW_HintInfo, hintInfo,
    WINDOW_GadgetHelp, TRUE,
    WINDOW_AppPort, gAppPort,
    WA_CloseGadget, TRUE,
    WA_DepthGadget, TRUE,
    WA_SizeGadget, TRUE,
    WA_DragBar, TRUE,
    WA_Activate, TRUE,
    WINDOW_IconTitle, "RestoreBackups",
    WA_NoCareRefresh, TRUE,
    WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE,
    WINDOW_ParentGroup, NewObject( LAYOUT_GetClass(), NULL,
    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
    LAYOUT_SpaceOuter, TRUE,
    LAYOUT_DeferLayout, TRUE,
      LAYOUT_AddChild, main_gadgets[vert_105] = NewObject( LAYOUT_GetClass(), NULL, 
        GA_ID, vert_105_id,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_AddChild, main_gadgets[backup_list_layout] = NewObject( LAYOUT_GetClass(), NULL, 
          GA_ID, backup_list_layout_id,
          LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
          LAYOUT_LeftSpacing, 2,
          LAYOUT_RightSpacing, 2,
          LAYOUT_TopSpacing, 2,
          LAYOUT_BottomSpacing, 2,
          LAYOUT_AddChild, main_gadgets[backup_list] = NewObject( LISTBROWSER_GetClass(), NULL, 
            GA_ID, backup_list_id,
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            LISTBROWSER_Position, 0,
            LISTBROWSER_Labels, labels107,
            LISTBROWSER_ColumnInfo, &ListBrowser107_ci,
            LISTBROWSER_ColumnTitles, TRUE,
          TAG_END),
        TAG_END),
        CHILD_WeightedHeight, 45,
        LAYOUT_AddChild, main_gadgets[backup_details_layout] = NewObject( LAYOUT_GetClass(), NULL, 
          GA_ID, backup_details_layout_id,
          LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
          LAYOUT_LeftSpacing, 2,
          LAYOUT_RightSpacing, 2,
          LAYOUT_TopSpacing, 2,
          LAYOUT_BottomSpacing, 2,
          LAYOUT_AddChild, main_gadgets[backup_details] = NewObject( LISTBROWSER_GetClass(), NULL, 
            GA_ID, backup_details_id,
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            LISTBROWSER_Position, 0,
            LISTBROWSER_Labels, labels109,
            LISTBROWSER_ColumnInfo, &ListBrowser109_ci,
            LISTBROWSER_ColumnTitles, TRUE,
          TAG_END),
        TAG_END),
        CHILD_WeightedHeight, 45,
        LAYOUT_AddChild, main_gadgets[backup_buttons_row1_layout] = NewObject( LAYOUT_GetClass(), NULL, 
          GA_ID, backup_buttons_row1_layout_id,
          LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
          LAYOUT_AddChild, main_gadgets[backup_delete_run] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, backup_delete_run_id,
            GA_Text, "Delete run",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
          LAYOUT_AddChild, main_gadgets[backup_restore_run] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, backup_restore_run_id,
            GA_Text, "Restore run",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
          LAYOUT_AddChild, main_gadgets[backup_view_folders] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, backup_view_folders_id,
            GA_Text, "View folders",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
          LAYOUT_AddChild, main_gadgets[button_cancel] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, button_cancel_id,
            GA_Text, "Cancel",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
        TAG_END),
        CHILD_WeightedHeight, 10,
      TAG_END),
    TAG_END),
  TAG_END);  
  main_gadgets[10] = 0;

  runWindow( window_object, restore_backups_id, 0, main_gadgets );

  if ( window_object ) DisposeObject( window_object );
  if ( labels107 ) FreeBrowserNodes( labels107 );
  if ( labels109 ) FreeBrowserNodes( labels109 );
}

int main( int argc, char **argv )
{
  if ( setup() )
  {
    restore_backups_window();
  }
  cleanup();
  return 0;
}
