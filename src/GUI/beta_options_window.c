/*
 * beta_options_window.c - iTidy Beta Options Window (ReAction)
 * ReAction-based GUI for Workbench 3.2+
 * Replacement for legacy GadTools implementation
 */

/* Redefine library bases to local unique names BEFORE including proto headers */
#define WindowBase iTidy_Beta_WindowBase
#define LayoutBase iTidy_Beta_LayoutBase
#define ButtonBase iTidy_Beta_ButtonBase
#define CheckBoxBase iTidy_Beta_CheckBoxBase
#define ChooserBase iTidy_Beta_ChooserBase
#define LabelBase iTidy_Beta_LabelBase

#include "beta_options_window.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>
#include <reaction/reaction_macros.h>

#include "platform/platform.h"
#include "writeLog.h"

/* ReAction Classes */
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/checkbox.h>
#include <proto/chooser.h>
#include <proto/label.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/checkbox.h>
#include <gadgets/chooser.h>
#include <images/label.h>

/* Local Library Bases */
struct Library *iTidy_Beta_WindowBase = NULL;
struct Library *iTidy_Beta_LayoutBase = NULL;
struct Library *iTidy_Beta_ButtonBase = NULL;
struct Library *iTidy_Beta_CheckBoxBase = NULL;
struct Library *iTidy_Beta_ChooserBase = NULL;
struct Library *iTidy_Beta_LabelBase = NULL;

/* Gadget IDs */
enum {
    GID_BETA_ROOT = 0,
    GID_BETA_OPEN_FOLDERS,
    GID_BETA_UPDATE_WINDOWS,
    GID_BETA_LOG_LEVEL,
    GID_BETA_MEMORY_LOG,
    GID_BETA_PERFORMANCE_LOG,
    GID_BETA_OK,
    GID_BETA_CANCEL
};

/* Internal Window Data */
struct BetaWindowData {
    struct Window *window;
    Object *root_layout;
    Object *gadgets[GID_BETA_CANCEL + 1];
    struct List *log_level_list;
    LayoutPreferences *prefs;
};

/*------------------------------------------------------------------------*/
/* Library Handling                                                       */
/*------------------------------------------------------------------------*/

static BOOL open_reaction_classes(void)
{
    iTidy_Beta_WindowBase = OpenLibrary("window.class", 47);
    iTidy_Beta_LayoutBase = OpenLibrary("gadgets/layout.gadget", 47);
    iTidy_Beta_ButtonBase = OpenLibrary("gadgets/button.gadget", 47);
    iTidy_Beta_CheckBoxBase = OpenLibrary("gadgets/checkbox.gadget", 47);
    iTidy_Beta_ChooserBase = OpenLibrary("gadgets/chooser.gadget", 47);
    iTidy_Beta_LabelBase = OpenLibrary("images/label.image", 47);

    return (iTidy_Beta_WindowBase && iTidy_Beta_LayoutBase &&
            iTidy_Beta_ButtonBase && iTidy_Beta_CheckBoxBase &&
            iTidy_Beta_ChooserBase && iTidy_Beta_LabelBase);
}

static void close_reaction_classes(void)
{
    if (iTidy_Beta_LabelBase) CloseLibrary(iTidy_Beta_LabelBase);
    if (iTidy_Beta_ChooserBase) CloseLibrary(iTidy_Beta_ChooserBase);
    if (iTidy_Beta_CheckBoxBase) CloseLibrary(iTidy_Beta_CheckBoxBase);
    if (iTidy_Beta_ButtonBase) CloseLibrary(iTidy_Beta_ButtonBase);
    if (iTidy_Beta_LayoutBase) CloseLibrary(iTidy_Beta_LayoutBase);
    if (iTidy_Beta_WindowBase) CloseLibrary(iTidy_Beta_WindowBase);
    
    iTidy_Beta_LabelBase = NULL;
    iTidy_Beta_ChooserBase = NULL;
    iTidy_Beta_CheckBoxBase = NULL;
    iTidy_Beta_ButtonBase = NULL;
    iTidy_Beta_LayoutBase = NULL;
    iTidy_Beta_WindowBase = NULL;
}

/*------------------------------------------------------------------------*/
/* Helper Functions                                                       */
/*------------------------------------------------------------------------*/

static struct List *make_chooser_list(STRPTR *labels)
{
    struct List *list = (struct List *)whd_malloc(sizeof(struct List));
    if (list)
    {
        NewList(list);
        while (*labels)
        {
            struct Node *node = AllocChooserNode(CNA_Text, *labels, TAG_END);
            if (node)
            {
                AddTail(list, node);
            }
            labels++;
        }
    }
    return list;
}

static void free_chooser_list(struct List *list)
{
    if (list)
    {
        struct Node *node, *next;
        node = list->lh_Head;
        while ((next = node->ln_Succ))
        {
            FreeChooserNode(node);
            node = next;
        }
        whd_free(list);
    }
}

/*------------------------------------------------------------------------*/
/* Window Logic                                                           */
/*------------------------------------------------------------------------*/

static void apply_prefs_to_gadgets(struct BetaWindowData *wd)
{
    if (!wd || !wd->prefs) return;

    SetGadgetAttrs((struct Gadget *)wd->gadgets[GID_BETA_OPEN_FOLDERS], wd->window, NULL,
        GA_Selected, wd->prefs->beta_openFoldersAfterProcessing, TAG_DONE);

    SetGadgetAttrs((struct Gadget *)wd->gadgets[GID_BETA_UPDATE_WINDOWS], wd->window, NULL,
        GA_Selected, wd->prefs->beta_FindWindowOnWorkbenchAndUpdate, TAG_DONE);

    SetGadgetAttrs((struct Gadget *)wd->gadgets[GID_BETA_LOG_LEVEL], wd->window, NULL,
        CHOOSER_Selected, wd->prefs->logLevel, TAG_DONE);

    SetGadgetAttrs((struct Gadget *)wd->gadgets[GID_BETA_MEMORY_LOG], wd->window, NULL,
        GA_Selected, wd->prefs->memoryLoggingEnabled, TAG_DONE);

    SetGadgetAttrs((struct Gadget *)wd->gadgets[GID_BETA_PERFORMANCE_LOG], wd->window, NULL,
        GA_Selected, wd->prefs->enable_performance_logging, TAG_DONE);
}

static void save_gadgets_to_prefs(struct BetaWindowData *wd)
{
    if (!wd || !wd->prefs) return;

    ULONG val = 0;

    GetAttr(GA_Selected, wd->gadgets[GID_BETA_OPEN_FOLDERS], &val);
    wd->prefs->beta_openFoldersAfterProcessing = (BOOL)val;

    GetAttr(GA_Selected, wd->gadgets[GID_BETA_UPDATE_WINDOWS], &val);
    wd->prefs->beta_FindWindowOnWorkbenchAndUpdate = (BOOL)val;

    GetAttr(CHOOSER_Selected, wd->gadgets[GID_BETA_LOG_LEVEL], &val);
    wd->prefs->logLevel = (LogLevel)val;

    GetAttr(GA_Selected, wd->gadgets[GID_BETA_MEMORY_LOG], &val);
    wd->prefs->memoryLoggingEnabled = (BOOL)val;

    GetAttr(GA_Selected, wd->gadgets[GID_BETA_PERFORMANCE_LOG], &val);
    wd->prefs->enable_performance_logging = (BOOL)val;
}

BOOL open_itidy_beta_options_window(LayoutPreferences *prefs)
{
    struct BetaWindowData wd = {0};
    struct Screen *screen;
    BOOL result = FALSE;
    BOOL loop = TRUE;
    
    static STRPTR log_levels[] = {
        "DEBUG (Verbose)",
        "INFO (Normal)",
        "WARNING (Warnings)",
        "ERROR (Errors)",
        NULL
    };

    if (!prefs) return FALSE;
    wd.prefs = prefs;

    if (!open_reaction_classes())
    {
        close_reaction_classes();
        return FALSE;
    }

    /* Lock Screen for Visual Info */
    screen = LockPubScreen("Workbench");
    if (!screen)
    {
        close_reaction_classes();
        return FALSE;
    }

    /* Create Lists */
    wd.log_level_list = make_chooser_list(log_levels);

    /* Create Window Object */
    /* Reference: Layout plan
       VLayout
         Check: Open Folders
         Check: Update WB
         HLayout: Label "Log Level", Chooser
         Check: Memory Log
         Check: Perf Log
         Space
         HLayout: OK, Cancel
    */

    wd.root_layout = WindowObject,
        WA_Title, "iTidy Beta Options",
        WA_DragBar, TRUE,
        WA_CloseGadget, TRUE,
        WA_DepthGadget, TRUE,
        WA_SizeGadget, FALSE, /* Fixed size */
        WA_Activate, TRUE,
        WA_PubScreen, screen,
        WINDOW_Position, WPOS_CENTERMOUSE,
        
        WINDOW_ParentGroup, VLayoutObject,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_BevelStyle, BVS_GROUP,
            LAYOUT_Label, "Experimental Features",

            LAYOUT_AddChild, wd.gadgets[GID_BETA_OPEN_FOLDERS] = CheckBoxObject,
                GA_ID, GID_BETA_OPEN_FOLDERS,
                GA_RelVerify, TRUE,
                GA_Text, "Open Folders After Processing",
                CHECKBOX_TextPlace, PLACETEXT_RIGHT,
            End,

            LAYOUT_AddChild, wd.gadgets[GID_BETA_UPDATE_WINDOWS] = CheckBoxObject,
                GA_ID, GID_BETA_UPDATE_WINDOWS,
                GA_RelVerify, TRUE,
                GA_Text, "Update Workbench Windows",
                CHECKBOX_TextPlace, PLACETEXT_RIGHT,
            End,

            LAYOUT_AddChild, LayoutObject,
                LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                LAYOUT_AddChild, LabelObject,
                    GA_Text, "Log Level:",
                End,
                LAYOUT_AddChild, wd.gadgets[GID_BETA_LOG_LEVEL] = ChooserObject,
                    GA_ID, GID_BETA_LOG_LEVEL,
                    GA_RelVerify, TRUE,
                    CHOOSER_Labels, wd.log_level_list,
                End,
            End,

            LAYOUT_AddChild, wd.gadgets[GID_BETA_MEMORY_LOG] = CheckBoxObject,
                GA_ID, GID_BETA_MEMORY_LOG,
                GA_RelVerify, TRUE,
                GA_Text, "Enable Memory Logging",
                CHECKBOX_TextPlace, PLACETEXT_RIGHT,
            End,

            LAYOUT_AddChild, wd.gadgets[GID_BETA_PERFORMANCE_LOG] = CheckBoxObject,
                GA_ID, GID_BETA_PERFORMANCE_LOG,
                GA_RelVerify, TRUE,
                GA_Text, "Enable Performance Logging",
                CHECKBOX_TextPlace, PLACETEXT_RIGHT,
            End,
            
            LAYOUT_AddChild, LayoutObject, End,
            CHILD_MinHeight, 5,
            CHILD_WeightedHeight, 0,

            LAYOUT_AddChild, LayoutObject,
                LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                LAYOUT_EvenSize, TRUE, /* Equal width buttons */
                
                LAYOUT_AddChild, wd.gadgets[GID_BETA_OK] = ButtonObject,
                    GA_ID, GID_BETA_OK,
                    GA_RelVerify, TRUE,
                    GA_Text, "OK",
                End,
                
                LAYOUT_AddChild, wd.gadgets[GID_BETA_CANCEL] = ButtonObject,
                    GA_ID, GID_BETA_CANCEL,
                    GA_RelVerify, TRUE,
                    GA_Text, "Cancel",
                End,
            End,
        End,
    End;

    if (wd.root_layout)
    {
        wd.window = (struct Window *)RA_OpenWindow(wd.root_layout);
        if (wd.window)
        {
            /* Apply current prefs to gadgets */
            apply_prefs_to_gadgets(&wd);
            
            /* Event Loop */
            while (loop)
            {
                ULONG result_mask = RA_HandleInput(wd.root_layout, &result_mask);
                ULONG method = result_mask & WMHI_CLASSMASK;
                
                switch (method)
                {
                    case WMHI_CLOSEWINDOW:
                        loop = FALSE;
                        break;
                        
                    case WMHI_GADGETUP:
                        switch (result_mask & WMHI_GADGETMASK)
                        {
                            case GID_BETA_OK:
                                save_gadgets_to_prefs(&wd);
                                result = TRUE;
                                loop = FALSE;
                                break;
                                
                            case GID_BETA_CANCEL:
                                loop = FALSE;
                                break;
                        }
                        break;
                }
            }
        }
        DisposeObject(wd.root_layout);
    }

    if (wd.log_level_list) free_chooser_list(wd.log_level_list);
    UnlockPubScreen(NULL, screen);
    close_reaction_classes();

    return result;
}
