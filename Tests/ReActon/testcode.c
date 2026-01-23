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
#include <proto/listbrowser.h>
#include <proto/label.h>

#include <libraries/gadtools.h>
#include <reaction/reaction.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <reaction/reaction_macros.h>
#include <classes/window.h>
#include <exec/memory.h>

void main_window( void );
void main_progress_window( void );

struct Screen	*gScreen = NULL;
struct DrawInfo	*gDrawInfo = NULL;
APTR gVisinfo = NULL;
struct MsgPort	*gAppPort = NULL;

struct Library *WindowBase = NULL,
               *ButtonBase = NULL,
               *CheckBoxBase = NULL,
               *ChooserBase = NULL,
               *GetFileBase = NULL,
               *ListBrowserBase = NULL,
               *LabelBase = NULL,
               *GadToolsBase = NULL,
               *LayoutBase = NULL,
               *IconBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;

//window ids
enum win { main_window_id = 4, main_progress_window_id = 53 };

//main_window gadgets
enum main_window_idx { master_layout, folder_layout, folder_name, itidy_options, left_column, 
  order_selection, cleanup_subfolders, position_selector, right_column, 
  order_by_selection, checkbox_39, button_40, tools_layout, advanced_button, 
  default_tools_button, restore_backups_button, main_buttons_layout, 
  start_button, exit_button };
//main_progress_window gadgets
enum main_progress_window_idx { status_container, progress_listbrowser, slow_progress_text, progress_cancel_button };


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
    {master_layout,-1,"",0},
    {folder_layout,-1,"",0},
    {folder_name,-1,"",0},
    {itidy_options,-1,"",0},
    {left_column,-1,"",0},
    {order_selection,-1,"",0},
    {cleanup_subfolders,-1,"",0},
    {position_selector,-1,"",0},
    {right_column,-1,"",0},
    {order_by_selection,-1,"",0},
    {checkbox_39,-1,"",0},
    {button_40,-1,"",0},
    {tools_layout,-1,"",0},
    {advanced_button,-1,"",0},
    {default_tools_button,-1,"",0},
    {restore_backups_button,-1,"",0},
    {main_buttons_layout,-1,"",0},
    {start_button,-1,"",0},
    {exit_button,-1,"",0},
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
    WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_MENUHELP | IDCMP_NEWSIZE,
    WINDOW_ParentGroup, NewObject( LAYOUT_GetClass(), NULL,
    LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
    LAYOUT_SpaceOuter, TRUE,
    LAYOUT_DeferLayout, TRUE,
      LAYOUT_AddChild, main_gadgets[master_layout] = NewObject( LAYOUT_GetClass(), NULL, 
        GA_ID, master_layout,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_LeftSpacing, 2,
        LAYOUT_RightSpacing, 2,
        LAYOUT_TopSpacing, 2,
        LAYOUT_BottomSpacing, 2,
        LAYOUT_AddChild, main_gadgets[folder_layout] = NewObject( LAYOUT_GetClass(), NULL, 
          GA_ID, folder_layout,
          LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
          LAYOUT_BevelStyle, BVS_THIN,
          LAYOUT_LeftSpacing, 2,
          LAYOUT_RightSpacing, 2,
          LAYOUT_TopSpacing, 3,
          LAYOUT_BottomSpacing, 4,
          LAYOUT_AddChild, main_gadgets[folder_name] = NewObject( GETFILE_GetClass(), NULL, 
            GA_ID, folder_name,
            GA_RelVerify, TRUE,
            GETFILE_TitleText, "Select a file",
            GETFILE_Drawer, "folder_name_string",
            GETFILE_FullFile, "Sys:",
            GETFILE_FilterDrawers, TRUE,
            GETFILE_ReadOnly, TRUE,
          TAG_END),
          CHILD_Label, NewObject( LABEL_GetClass(), NULL, 
            LABEL_Text, "Folder",
          TAG_END),
        TAG_END),
        LAYOUT_AddChild, main_gadgets[itidy_options] = NewObject( LAYOUT_GetClass(), NULL, 
          GA_ID, itidy_options,
          LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
          LAYOUT_BevelStyle, BVS_THIN,
          LAYOUT_LeftSpacing, 2,
          LAYOUT_RightSpacing, 2,
          LAYOUT_TopSpacing, 2,
          LAYOUT_BottomSpacing, 4,
          LAYOUT_AddChild, main_gadgets[left_column] = NewObject( LAYOUT_GetClass(), NULL, 
            GA_ID, left_column,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_RightSpacing, 2,
            LAYOUT_AddChild, main_gadgets[order_selection] = NewObject( CHOOSER_GetClass(), NULL, 
              GA_ID, order_selection,
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
              GA_ID, cleanup_subfolders,
              GA_Text, "",
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              CHECKBOX_TextPlace, PLACETEXT_RIGHT,
            TAG_END),
            CHILD_Label, NewObject( LABEL_GetClass(), NULL, 
              LABEL_Text, "Cleanup subfolders",
            TAG_END),
            LAYOUT_AddChild, main_gadgets[position_selector] = NewObject( CHOOSER_GetClass(), NULL, 
              GA_ID, position_selector,
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
            GA_ID, right_column,
            LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
            LAYOUT_LeftSpacing, 2,
            LAYOUT_AddChild, main_gadgets[order_by_selection] = NewObject( CHOOSER_GetClass(), NULL, 
              GA_ID, order_by_selection,
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
              GA_ID, checkbox_39,
              GA_Text, "",
              GA_RelVerify, TRUE,
              GA_TabCycle, TRUE,
              CHECKBOX_TextPlace, PLACETEXT_RIGHT,
            TAG_END),
            CHILD_Label, NewObject( LABEL_GetClass(), NULL, 
              LABEL_Text, "Backup icons",
            TAG_END),
            LAYOUT_AddChild, main_gadgets[button_40] = NewObject( BUTTON_GetClass(), NULL, 
              GA_ID, button_40,
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
          GA_ID, tools_layout,
          LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
          LAYOUT_BevelStyle, BVS_THIN,
          LAYOUT_LeftSpacing, 2,
          LAYOUT_RightSpacing, 2,
          LAYOUT_TopSpacing, 2,
          LAYOUT_BottomSpacing, 4,
          LAYOUT_AddChild, main_gadgets[advanced_button] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, advanced_button,
            GA_Text, "Advanced",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
          LAYOUT_AddChild, main_gadgets[default_tools_button] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, default_tools_button,
            GA_Text, "Fix default tools...",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
          LAYOUT_AddChild, main_gadgets[restore_backups_button] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, restore_backups_button,
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
          GA_ID, main_buttons_layout,
          LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
          LAYOUT_AddChild, main_gadgets[start_button] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, start_button,
            GA_Text, "Start",
            GA_RelVerify, TRUE,
            GA_TabCycle, TRUE,
            BUTTON_TextPen, 1,
            BUTTON_BackgroundPen, 0,
            BUTTON_FillTextPen, 1,
            BUTTON_FillPen, 3,
          TAG_END),
          LAYOUT_AddChild, main_gadgets[exit_button] = NewObject( BUTTON_GetClass(), NULL, 
            GA_ID, exit_button,
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
}

void main_progress_window( void )
{
  struct Gadget	*main_gadgets[ 5 ];
  Object *window_object = NULL;
  struct HintInfo hintInfo[] =
  {
    {status_container,-1,"",0},
    {progress_listbrowser,-1,"",0},
    {slow_progress_text,-1,"",0},
    {progress_cancel_button,-1,"Press this to cancel the current iTidy run.  Note: Any changes made to the system will not be undone.",0},
    {-1,-1,NULL,0}
  };
  struct List *labels56;
  UBYTE *labels56_str[] = { NULL };
  labels56 = BrowserNodesA( labels56_str, 1 );

  window_object = NewObject( WINDOW_GetClass(), NULL, 
    WA_Title, "Status",
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
      LAYOUT_AddChild, main_gadgets[status_container] = NewObject( LAYOUT_GetClass(), NULL, 
        GA_ID, status_container,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_LeftSpacing, 2,
        LAYOUT_RightSpacing, 2,
        LAYOUT_TopSpacing, 2,
        LAYOUT_BottomSpacing, 2,
        LAYOUT_AddChild, main_gadgets[progress_listbrowser] = NewObject( LISTBROWSER_GetClass(), NULL, 
          GA_ID, progress_listbrowser,
          GA_RelVerify, TRUE,
          GA_TabCycle, TRUE,
          LISTBROWSER_Position, 0,
        TAG_END),
        LAYOUT_AddImage, main_gadgets[slow_progress_text] = NewObject( LABEL_GetClass(), NULL, 
          GA_ID, slow_progress_text,
          LABEL_DrawInfo, gDrawInfo,
          LABEL_Text, "Starting, one moment please...",
        TAG_END),
        LAYOUT_AddChild, main_gadgets[progress_cancel_button] = NewObject( BUTTON_GetClass(), NULL, 
          GA_ID, progress_cancel_button,
          GA_Text, "Cancel",
          GA_RelVerify, TRUE,
          GA_TabCycle, TRUE,
          BUTTON_TextPen, 1,
          BUTTON_BackgroundPen, 0,
          BUTTON_FillTextPen, 1,
          BUTTON_FillPen, 3,
        TAG_END),
        CHILD_MinHeight, 16,
        CHILD_MaxHeight, 28,
      TAG_END),
    TAG_END),
  TAG_END);  
  main_gadgets[4] = 0;
}

