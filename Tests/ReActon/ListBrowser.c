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
#include <time.h>

#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/listbrowser.h>

#include <libraries/gadtools.h>
#include <reaction/reaction.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <reaction/reaction_macros.h>
#include <classes/window.h>
#include <exec/memory.h>

void window_4( void );

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
               *GadToolsBase = NULL,
               *LayoutBase = NULL,
               *IconBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;

//window ids
enum win { window_4_id = 4 };

//Window_4 gadgets
enum window_4_idx { vert_6, listbrowser_7, button_9 };
enum window_4_id { vert_6_id = 6, listbrowser_7_id = 7, button_9_id = 9 };

int setup( void )
{
  if( !(IntuitionBase = (struct IntuitionBase*) OpenLibrary("intuition.library",0L)) ) return 0;
  if( !(GadToolsBase = (struct Library*) OpenLibrary("gadtools.library",0L) ) ) return 0;
  if( !(WindowBase = (struct Library*) OpenLibrary("window.class",0L) ) ) return 0;
  if( !(IconBase = (struct Library*) OpenLibrary("icon.library",0L) ) ) return 0;
  if( !(LayoutBase = (struct Library*) OpenLibrary("gadgets/layout.gadget",0L) ) ) return 0;
  if( !(ButtonBase = (struct Library*) OpenLibrary("gadgets/button.gadget",0L) ) ) return 0;
  if( !(ListBrowserBase = (struct Library*) OpenLibrary("gadgets/listbrowser.gadget",0L) ) ) return 0;
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
                if ((result & WMHI_GADGETMASK) == button_9_id)
                {
                  struct List *list = NULL;
                  struct Node *node;
                  struct Gadget *lb = win_gadgets[listbrowser_7];
                  int count = 0;

                  /* 
                   * AI LEARNING POINT:
                   * Before modifying a generic list attached to a ListBrowser, ALWAYS detach it first.
                   * Modifying a live list can cause race conditions or crashes in the GUI system.
                   * We detach by setting LISTBROWSER_Labels to ~0 (or NULL in some docs, but ~0 is safer).
                   */
                  GetAttr( LISTBROWSER_Labels, (Object *)lb, (ULONG *)&list );

                  SetGadgetAttrs( lb, main_window, NULL,
                    LISTBROWSER_Labels, ~0,
                    TAG_DONE );

                  if ( list )
                  {
                    char buffer[32];
                    char buffer2[32];
                    struct Node *chk_node;

                    /* Generate random text for both columns */
                    sprintf(buffer, "item: %02d", rand() % 100);
                    sprintf(buffer2, "Action %d", rand() % 1000);

                    /* 
                     * AI LEARNING POINT:
                     * Allocate node with 2 columns.
                     * We specify content for Column 0 and Column 1 separately.
                     */
                    node = AllocListBrowserNode( 2,
                      LBNA_Column, 0,
                        LBNCA_CopyText, TRUE,
                        LBNCA_Text, buffer,
                      LBNA_Column, 1,
                        LBNCA_CopyText, TRUE,
                        LBNCA_Text, buffer2,
                      TAG_DONE );
                    if ( node ) AddTail( list, node );

                    /* 
                     * Count nodes in the list.
                     * We manually walk the list because we don't track the count externally in this simple test.
                     */
                    for(chk_node = list->lh_Head; chk_node->ln_Succ; chk_node = chk_node->ln_Succ)
                    {
                        count++;
                    }

                    /* 
                     * AI LEARNING POINT:
                     * Limit list size to separate old data.
                     * RemHead() removes from the start of the list.
                     * IMPORTANT: When using AllocListBrowserNode, you MUST match it with FreeListBrowserNode,
                     * not just FreeVec or FreeMem.
                     */
                    if (count > 7)
                    {
                        struct Node *rem_node = RemHead(list);
                        if (rem_node) FreeListBrowserNode(rem_node);
                        count--;
                    }
                  }

                  /* 
                   * AI LEARNING POINT:
                   * 1. Re-attach the modified list to update the GUI.
                   * 2. Use LISTBROWSER_MakeVisible to auto-scroll to the new item.
                   *    'count - 1' is the index of the last item (0-based).
                   */
                  SetGadgetAttrs( lb, main_window, NULL,
                    LISTBROWSER_Labels, list,
                    LISTBROWSER_MakeVisible, (count > 0 ? count - 1 : 0),
                    TAG_DONE );
                }
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

void window_4( void )
{
  struct Gadget	*main_gadgets[ 4 ];
  Object *window_object = NULL;
  struct HintInfo hintInfo[] =
  {
    {vert_6_id,-1,"",0},
    {listbrowser_7_id,-1,"",0},
    {button_9_id,-1,"",0},
    {-1,-1,NULL,0}
  };
  struct List *labels7;
  UBYTE *labels7_str[] = { "item1", "item2", "item3",  NULL };
  struct ColumnInfo ListBrowser7_ci[] =
  {
    { 40, "Status", 0 },
    { 60, "Action", 0 },
    { -1, (STRPTR)~0, -1 }
  };

  /* AI LEARNING POINT: We now have 2 columns, so we tell the allocator */
  labels7 = BrowserNodesA( labels7_str, 2 );

  window_object = NewObject( WINDOW_GetClass(), NULL, 
    WA_Title, "Application Window",
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
    WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE,
    WINDOW_ParentGroup, NewObject( LAYOUT_GetClass(), NULL,
    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
    LAYOUT_SpaceOuter, TRUE,
    LAYOUT_DeferLayout, TRUE,
      LAYOUT_AddChild, main_gadgets[vert_6] = NewObject( LAYOUT_GetClass(), NULL, 
        GA_ID, vert_6_id,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_AddChild, main_gadgets[listbrowser_7] = NewObject( LISTBROWSER_GetClass(), NULL, 
          GA_ID, listbrowser_7_id,
          GA_RelVerify, TRUE,
          GA_TabCycle, TRUE,
          LISTBROWSER_Position, 0,
          LISTBROWSER_Labels, labels7,
          LISTBROWSER_ColumnInfo, &ListBrowser7_ci,
          LISTBROWSER_ColumnTitles, TRUE,
        TAG_END),
        LAYOUT_AddChild, main_gadgets[button_9] = NewObject( BUTTON_GetClass(), NULL, 
          GA_ID, button_9_id,
          GA_Text, "add",
          GA_RelVerify, TRUE,
          GA_TabCycle, TRUE,
          BUTTON_TextPen, 1,
          BUTTON_BackgroundPen, 0,
          BUTTON_FillTextPen, 1,
          BUTTON_FillPen, 3,
        TAG_END),
      TAG_END),
    TAG_END),
  TAG_END);  
  main_gadgets[3] = 0;

  runWindow( window_object, window_4_id, 0, main_gadgets );

  if ( window_object ) DisposeObject( window_object );
  if ( labels7 ) FreeBrowserNodes( labels7 );
}

int main( int argc, char **argv )
{
  srand(time(NULL));
  if ( setup() )
  {
    window_4();
  }
  cleanup();
}
