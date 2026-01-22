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
#include <proto/chooser.h>
#include <proto/getfile.h>
#include <proto/label.h>

#include <libraries/gadtools.h>
#include <reaction/reaction.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <reaction/reaction_macros.h>
#include <classes/window.h>
#include <exec/memory.h>

void main_window( void );

struct Screen	*gScreen = NULL;
struct DrawInfo	*gDrawInfo = NULL;
APTR gVisinfo = NULL;
struct MsgPort	*gAppPort = NULL;

struct List *ChooserLabelsA(STRPTR *nameList)
{
  struct List *newList;
  newList = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC );

  if (newList && nameList)
  {
    NewList( newList );
    while(*nameList)
    {
      AddTail(newList, AllocChooserNode(CNA_Text, *nameList, TAG_END));
      nameList++;
    }
  }
  return newList;
}

void FreeChooserLabels(struct List *list)
{
  struct Node *node;
  struct Node *nextnode;
  
    if (list)
    {
      node = list->lh_Head;

      while(nextnode = node->ln_Succ)
      {
        FreeChooserNode(node);
        node = nextnode;
      }
      FreeMem(list, sizeof (struct List));
    }
}

struct Library *WindowBase = NULL,
               *ButtonBase = NULL,
               *CheckBoxBase = NULL,
               *ChooserBase = NULL,
               *GetFileBase = NULL,
               *LabelBase = NULL,
               *GadToolsBase = NULL,
               *LayoutBase = NULL,
               *IconBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;

//window ids
enum win { main_window_id = 4 };

//main_window gadgets
enum main_window_idx { master_layout, folder_layout, folder_name, itidy_options, left_column, 
  order_selection, cleanup_subfolders, position_selector, right_column, 
  order_by_selection, checkbox_39, button_40, tools_layout, advanced_button, 
  default_tools_button, restore_backups_button, main_buttons_layout, 
  start_button, exit_button };
enum main_window_id { master_layout_id = 6, folder_layout_id = 28, folder_name_id = 52, 
  itidy_options_id = 26, left_column_id = 33, order_selection_id = 35, 
  cleanup_subfolders_id = 36, position_selector_id = 37, right_column_id = 34, 
  order_by_selection_id = 38, checkbox_39_id = 39, button_40_id = 40, 
  tools_layout_id = 45, advanced_button_id = 46, default_tools_button_id = 47, 
  restore_backups_button_id = 48, main_buttons_layout_id = 49, 
  start_button_id = 50, exit_button_id = 51 };

int setup( void )
{
  if( !(IntuitionBase = (struct IntuitionBase*) OpenLibrary("intuition.library",0L)) ) return 0;
  if( !(GadToolsBase = (struct Library*) OpenLibrary("gadtools.library",0L) ) ) return 0;
  if( !(WindowBase = (struct Library*) OpenLibrary("window.class",0L) ) ) return 0;
  if( !(IconBase = (struct Library*) OpenLibrary("icon.library",0L) ) ) return 0;
  if( !(LayoutBase = (struct Library*) OpenLibrary("gadgets/layout.gadget",0L) ) ) return 0;
  if( !(ButtonBase = (struct Library*) OpenLibrary("gadgets/button.gadget",0L) ) ) return 0;
  if( !(CheckBoxBase = (struct Library*) OpenLibrary("gadgets/checkbox.gadget",0L) ) ) return 0;
  if( !(ChooserBase = (struct Library*) OpenLibrary("gadgets/chooser.gadget",0L) ) ) return 0;
  if( !(GetFileBase = (struct Library*) OpenLibrary("gadgets/getfile.gadget",0L) ) ) return 0;
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
  if (CheckBoxBase) CloseLibrary( (struct Library *)CheckBoxBase );
  if (ChooserBase) CloseLibrary( (struct Library *)ChooserBase );
  if (GetFileBase) CloseLibrary( (struct Library *)GetFileBase );
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
                puts("gadget press");
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

void main_window( void )
{
  struct NewMenu menuData[] =
  {
    { NM_TITLE, "Project", 0, 0, 0, NULL },
    { NM_ITEM, "New", "N", 0, 510, NULL },
    { NM_ITEM, "Open...", "O", 0, 509, NULL },
    { NM_ITEM, NM_BARLABEL, 0, 0, 507, NULL },
    { NM_ITEM, "Save...", "S", 0, 503, NULL },
    { NM_ITEM, "Save as", "A", 0, 495, NULL },
    { NM_ITEM, NM_BARLABEL, 0, 0, 479, NULL },
    { NM_ITEM, "About", 0, 0, 447, NULL },
    { NM_ITEM, NM_BARLABEL, 0, 0, 383, NULL },
    { NM_ITEM, "Quit", 0, 0, 255, NULL },
    { NM_END, NULL, 0, 0, 0, (APTR)0 }
  };

  struct Menu	*menu_strip = NULL;
  struct Gadget	*main_gadgets[ 20 ];
  Object *window_object = NULL;
  struct HintInfo hintInfo[] =
  {
    {master_layout_id,-1,"",0},
    {folder_layout_id,-1,"",0},
    {folder_name_id,-1,"",0},
    {itidy_options_id,-1,"",0},
    {left_column_id,-1,"",0},
    {order_selection_id,-1,"Choose the order for tidy-up",0},
    {cleanup_subfolders_id,-1,"",0},
    {position_selector_id,-1,"",0},
    {right_column_id,-1,"",0},
    {order_by_selection_id,-1,"",0},
    {checkbox_39_id,-1,"",0},
    {button_40_id,-1,"",0},
    {tools_layout_id,-1,"",0},
    {advanced_button_id,-1,"",0},
    {default_tools_button_id,-1,"",0},
    {restore_backups_button_id,-1,"",0},
    {main_buttons_layout_id,-1,"",0},
    {start_button_id,-1,"",0},
    {exit_button_id,-1,"",0},
    {-1,-1,NULL,0}
  };
  struct List *labels35;
  UBYTE *labels35_str[] = { "Folders first", "Files first", "Mixed", "Grouped by type",  NULL };
  struct List *labels37;
  UBYTE *labels37_str[] = { "Center screen", "Keep position", "Near parent", "No change",  NULL };
  struct List *labels38;
  UBYTE *labels38_str[] = { "Name", "Type", "Date", "Size",  NULL };
  menu_strip = CreateMenusA( menuData, TAG_END );
  LayoutMenus( menu_strip, gVisinfo,
    GTMN_NewLookMenus, TRUE,
    TAG_DONE );

  labels35 = ChooserLabelsA( labels35_str );
  labels37 = ChooserLabelsA( labels37_str );
  labels38 = ChooserLabelsA( labels38_str );

  window_object = NewObject( WINDOW_GetClass(), NULL, 
    WA_Title, "iTidy - Icon cleanup tool",
    WA_Left, 5,
    WA_Top, 20,
    WA_Width, 150,
    WA_Height, 80,
    WA_MinWidth, 150,
    WA_MinHeight, 80,
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
    WINDOW_IconTitle, "MyApp",
    WA_NoCareRefresh, TRUE,
    WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_NEWSIZE,
    WINDOW_ParentGroup, NewObject( LAYOUT_GetClass(), NULL,
    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
    LAYOUT_SpaceOuter, TRUE,
    LAYOUT_DeferLayout, TRUE,
      LAYOUT_AddChild, main_gadgets[master_layout] = NewObject( LAYOUT_GetClass(), NULL, 
        GA_ID, master_layout_id,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_LeftSpacing, 2,
        LAYOUT_RightSpacing, 2,
        LAYOUT_TopSpacing, 2,
        LAYOUT_BottomSpacing, 2,
        LAYOUT_AddChild, main_gadgets[folder_layout] = NewObject( LAYOUT_GetClass(), NULL, 
          GA_ID, folder_layout_id,
          LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
          LAYOUT_BevelStyle, BVS_THIN,
          LAYOUT_LeftSpacing, 2,
          LAYOUT_RightSpacing, 2,
          LAYOUT_TopSpacing, 3,
          LAYOUT_BottomSpacing, 4,
          LAYOUT_AddChild, main_gadgets[folder_name] = NewObject( GETFILE_GetClass(), NULL, 
            GA_ID, folder_name_id,
            GA_RelVerify, TRUE,
            GETFILE_TitleText, "Select a file",
            GETFILE_Drawer, "folder_name_string",
            GETFILE_FullFile, "S",
            GETFILE_FilterDrawers, TRUE,
            GETFILE_ReadOnly, TRUE,
          TAG_END),
          CHILD_Label, NewObject( LABEL_GetClass(), NULL, 
            LABEL_Text, "Folder",
          TAG_END),
        TAG_END),
        LAYOUT_AddChild, main_gadgets[itidy_options] = NewObject( LAYOUT_GetClass(), NULL, 
          GA_ID, itidy_options_id,
          LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
          LAYOUT_BevelStyle, BVS_THIN,
          LAYOUT_LeftSpacing, 2,
          LAYOUT_RightSpacing, 2,
          LAYOUT_TopSpacing, 2,
          LAYOUT_BottomSpacing, 4,
          LAYOUT_AddChild, main_gadgets[left_column] = NewObject( LAYOUT_GetClass(), NULL, 
            GA_ID, left_column_id,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_RightSpacing, 2,
            LAYOUT_AddChild, main_gadgets[order_selection] = NewObject( CHOOSER_GetClass(), NULL, 
              GA_ID, order_selection_id,
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              CHOOSER_PopUp, TRUE,
              CHOOSER_Selected, 0,
              CHOOSER_Labels, labels35,
            TAG_END),
            CHILD_Label, NewObject( LABEL_GetClass(), NULL, 
              LABEL_Text, "Order",
            TAG_END),
            LAYOUT_AddChild, main_gadgets[cleanup_subfolders] = NewObject( CHECKBOX_GetClass(), NULL, 
              GA_ID, cleanup_subfolders_id,
              GA_Text, "",
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              CHECKBOX_TextPlace, PLACETEXT_RIGHT,
            TAG_END),
            CHILD_Label, NewObject( LABEL_GetClass(), NULL, 
              LABEL_Text, "Cleanup subfolders",
            TAG_END),
            LAYOUT_AddChild, main_gadgets[position_selector] = NewObject( CHOOSER_GetClass(), NULL, 
              GA_ID, position_selector_id,
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              CHOOSER_PopUp, TRUE,
              CHOOSER_Selected, 0,
              CHOOSER_Labels, labels37,
            TAG_END),
            CHILD_Label, NewObject( LABEL_GetClass(), NULL, 
              LABEL_Text, "Position",
            TAG_END),
          TAG_END),
          LAYOUT_AddChild, main_gadgets[right_column] = NewObject( LAYOUT_GetClass(), NULL, 
            GA_ID, right_column_id,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_LeftSpacing, 2,
            LAYOUT_AddChild, main_gadgets[order_by_selection] = NewObject( CHOOSER_GetClass(), NULL, 
              GA_ID, order_by_selection_id,
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              CHOOSER_PopUp, TRUE,
              CHOOSER_Selected, 0,
              CHOOSER_Labels, labels38,
            TAG_END),
            CHILD_Label, NewObject( LABEL_GetClass(), NULL, 
              LABEL_Text, "By",
            TAG_END),
            LAYOUT_AddChild, main_gadgets[checkbox_39] = NewObject( CHECKBOX_GetClass(), NULL, 
              GA_ID, checkbox_39_id,
              GA_Text, "",
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              CHECKBOX_TextPlace, PLACETEXT_RIGHT,
            TAG_END),
            CHILD_Label, NewObject( LABEL_GetClass(), NULL, 
              LABEL_Text, "Backup icons",
            TAG_END),
            LAYOUT_AddChild, main_gadgets[button_40] = NewObject( BUTTON_GetClass(), NULL, 
              GA_ID, button_40_id,
              GA_Text, "?",
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              BUTTON_TextPen, 1,
              BUTTON_BackgroundPen, 0,
              BUTTON_FillTextPen, 1,
              BUTTON_FillPen, 3,
            TAG_END),
            CHILD_Label, NewObject( LABEL_GetClass(), NULL, 
              LABEL_Text, "Help",
            TAG_END),
          TAG_END),
        TAG_END),
        LAYOUT_AddChild, main_gadgets[tools_layout] = NewObject( LAYOUT_GetClass(), NULL, 
          GA_ID, tools_layout_id,
          LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
          LAYOUT_BevelStyle, BVS_THIN,
          LAYOUT_LeftSpacing, 2,
          LAYOUT_RightSpacing, 2,
          LAYOUT_TopSpacing, 2,
          LAYOUT_BottomSpacing, 4,
          LAYOUT_AddChild, main_gadgets[advanced_button] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, advanced_button_id,
            GA_Text, "Advanced",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
          LAYOUT_AddChild, main_gadgets[default_tools_button] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, default_tools_button_id,
            GA_Text, "Fix default tools...",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
          LAYOUT_AddChild, main_gadgets[restore_backups_button] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, restore_backups_button_id,
            GA_Text, "Restore backups",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
        TAG_END),
        LAYOUT_AddChild, main_gadgets[main_buttons_layout] = NewObject( LAYOUT_GetClass(), NULL, 
          GA_ID, main_buttons_layout_id,
          LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
          LAYOUT_AddChild, main_gadgets[start_button] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, start_button_id,
            GA_Text, "Start",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
          LAYOUT_AddChild, main_gadgets[exit_button] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, exit_button_id,
            GA_Text, "Exit",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
        TAG_END),
      TAG_END),
    TAG_END),
  TAG_END);  
  main_gadgets[19] = 0;

  runWindow( window_object, main_window_id, menu_strip, main_gadgets );

  if ( window_object ) DisposeObject( window_object );
  if ( menu_strip ) FreeMenus( menu_strip );
  if ( labels35 ) FreeChooserLabels( labels35 );
  if ( labels37 ) FreeChooserLabels( labels37 );
  if ( labels38 ) FreeChooserLabels( labels38 );
}

int main( int argc, char **argv )
{
  if ( setup() )
  {
    main_window();
  }
  cleanup();
}
