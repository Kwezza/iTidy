/*
 * default_tool_update_window_reaction.c - Default Tool Replacement Window Implementation (ReAction)
 * Provides UI for batch or single icon default tool replacement
 * ReAction-based window for Workbench 3.2+
 */

/* CRITICAL: Declare external global ListBrowserBase (shared across modules) */
/* ListBrowserBase is globally defined in main_progress_window.c */
#include <exec/types.h>
extern struct Library *ListBrowserBase;

/*------------------------------------------------------------------------*/
/* Library Base Isolation - Prevents linker collisions                   */
/*------------------------------------------------------------------------*/
#define WindowBase iTidy_DefaultToolUpdate_WindowBase
#define LayoutBase iTidy_DefaultToolUpdate_LayoutBase
#define ButtonBase iTidy_DefaultToolUpdate_ButtonBase
#define GetFileBase iTidy_DefaultToolUpdate_GetFileBase
#define LabelBase iTidy_DefaultToolUpdate_LabelBase
#define RequesterBase iTidy_DefaultToolUpdate_RequesterBase
/* Note: ListBrowserBase is NOT isolated - it's used globally for amiga.lib stubs */

#include "platform/platform.h"
#include "default_tool_update_window_reaction.h"
#include "tool_cache_window.h"
#include "default_tool_backup.h"
#include "icon_types.h"
#include "itidy_types.h"
#include "path_utilities.h"
#include "layout_preferences.h"
#include "writeLog.h"
#include "../../helpers/exec_list_compat.h"
#include "../gui_utilities.h"

#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <classes/window.h>
#include <classes/requester.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/getfile.h>
#include <images/label.h>
#include <gadgets/listbrowser.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/requester.h>

/* ReAction proto headers */
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/getfile.h>
#include <proto/label.h>
#include <proto/listbrowser.h>
#include <proto/requester.h>
#include <classes/requester.h>

#include <reaction/reaction_macros.h>

#include <string.h>
#include <stdio.h>

/* Library bases (isolated to prevent linker collisions, except ListBrowserBase) */
struct Library *iTidy_DefaultToolUpdate_WindowBase = NULL;
struct Library *iTidy_DefaultToolUpdate_LayoutBase = NULL;
struct Library *iTidy_DefaultToolUpdate_ButtonBase = NULL;
struct Library *iTidy_DefaultToolUpdate_GetFileBase = NULL;
struct Library *iTidy_DefaultToolUpdate_LabelBase = NULL;
struct Library *iTidy_DefaultToolUpdate_RequesterBase = NULL;
/* ListBrowserBase declared globally above (before #defines) for amiga.lib stubs */

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static void populate_status_list(struct iTidy_DefaultToolUpdateWindow_ReAction *data);
static BOOL perform_tool_update(struct iTidy_DefaultToolUpdateWindow_ReAction *data);
static void add_status_entry(struct iTidy_DefaultToolUpdateWindow_ReAction *data, 
                             const char *icon_path, const char *status_text);
static BOOL init_reaction_libs(void);
static void close_reaction_libs(void);
static ULONG ShowReActionRequester(struct Window *parent_window, CONST_STRPTR title, CONST_STRPTR body, CONST_STRPTR gadgets, ULONG image_type);

/*------------------------------------------------------------------------*/
/* Initialize ReAction Libraries                                          */
/*------------------------------------------------------------------------*/
static BOOL init_reaction_libs(void)
{
    WindowBase = OpenLibrary("window.class", 0);
    LayoutBase = OpenLibrary("gadgets/layout.gadget", 0);
    ButtonBase = OpenLibrary("gadgets/button.gadget", 0);
    GetFileBase = OpenLibrary("gadgets/getfile.gadget", 0);
    LabelBase = OpenLibrary("images/label.image", 0);
    ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 0);
    RequesterBase = OpenLibrary("requester.class", 0);
    
    if (!WindowBase || !LayoutBase || !ButtonBase || !GetFileBase || !LabelBase || !ListBrowserBase || !RequesterBase)
    {
        log_error(LOG_GUI, "Failed to open ReAction libraries for Default Tool Update window");
        close_reaction_libs();
        return FALSE;
    }
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Close ReAction Libraries                                               */
/*------------------------------------------------------------------------*/
static void close_reaction_libs(void)
{
    if (RequesterBase) CloseLibrary(RequesterBase);
    if (ListBrowserBase) CloseLibrary(ListBrowserBase);
    if (LabelBase) CloseLibrary(LabelBase);
    if (GetFileBase) CloseLibrary(GetFileBase);
    if (ButtonBase) CloseLibrary(ButtonBase);
    if (LayoutBase) CloseLibrary(LayoutBase);
    if (WindowBase) CloseLibrary(WindowBase);
    
    RequesterBase = NULL;
    ListBrowserBase = NULL;
    LabelBase = NULL;
    GetFileBase = NULL;
    ButtonBase = NULL;
    LayoutBase = NULL;
    WindowBase = NULL;
}

/*------------------------------------------------------------------------*/
/* ReAction Requester Helper                                             */
/*------------------------------------------------------------------------*/

static ULONG ShowReActionRequester(struct Window *parent_window,
                                   CONST_STRPTR title,
                                   CONST_STRPTR body,
                                   CONST_STRPTR gadgets,
                                   ULONG image_type)
{
    Object *req_obj;
    struct orRequest req_msg;
    ULONG result = 0;

    if (!RequesterBase || !parent_window) return 0;

    req_obj = NewObject(REQUESTER_GetClass(), NULL,
        REQ_Type,       REQTYPE_INFO,
        REQ_TitleText,  title,
        REQ_BodyText,   body,
        REQ_GadgetText, gadgets,
        REQ_Image,      image_type,
        TAG_DONE);

    if (req_obj)
    {
        req_msg.MethodID  = RM_OPENREQ;
        req_msg.or_Attrs  = NULL;
        req_msg.or_Window = parent_window;
        req_msg.or_Screen = NULL;
        result = DoMethodA(req_obj, (Msg)&req_msg);
        DisposeObject(req_obj);
    }
    return result;
}

/*------------------------------------------------------------------------*/
/* Add Status Entry                                                       */
/*------------------------------------------------------------------------*/
static void add_status_entry(struct iTidy_DefaultToolUpdateWindow_ReAction *data,
                             const char *icon_path, const char *status_text)
{
    struct Node *node;
    
    if (data == NULL || icon_path == NULL || status_text == NULL)
        return;
    
    /* Create ListBrowser node with 2 columns: Status, Filepath */
    node = AllocListBrowserNode(2,
        LBNA_Column, LBCOL_STATUS,
            LBNCA_Text, status_text,
        LBNA_Column, LBCOL_FILEPATH,
            LBNCA_Text, icon_path,
        TAG_END);
    
    if (node == NULL)
    {
        log_error(LOG_GUI, "Failed to allocate ListBrowser node\n");
        return;
    }
    
    AddTail(&data->status_list, node);
}

/*------------------------------------------------------------------------*/
/* Free Status Entries (static - internal use only)                      */
/*------------------------------------------------------------------------*/
static void iTidy_FreeStatusEntries(struct iTidy_DefaultToolUpdateWindow_ReAction *data)
{
    struct Node *node;
    
    if (data == NULL)
        return;
    
    /* Detach list from gadget first */
    if (data->window && data->listbrowser_progress_obj)
    {
        SetGadgetAttrs((struct Gadget *)data->listbrowser_progress_obj, data->window, NULL,
            LISTBROWSER_Labels, ~0,
            TAG_END);
    }
    
    /* Free all ListBrowser nodes */
    while ((node = RemHead(&data->status_list)) != NULL)
    {
        FreeListBrowserNode(node);
    }
    
    /* Re-attach empty list and refresh display */
    if (data->window && data->listbrowser_progress_obj)
    {
        SetGadgetAttrs((struct Gadget *)data->listbrowser_progress_obj, data->window, NULL,
            LISTBROWSER_Labels, &data->status_list,
            TAG_END);
        
        /* Force complete window refresh */
        RefreshWindowFrame(data->window);
    }
}

/*------------------------------------------------------------------------*/
/* Populate Status List                                                   */
/*------------------------------------------------------------------------*/
static void populate_status_list(struct iTidy_DefaultToolUpdateWindow_ReAction *data)
{
    if (data == NULL || data->window == NULL || data->listbrowser_progress_obj == NULL)
        return;
    
    /* Detach list from gadget */
    SetGadgetAttrs((struct Gadget *)data->listbrowser_progress_obj, data->window, NULL,
        LISTBROWSER_Labels, ~0,
        TAG_END);
    
    /* Attach status list */
    SetGadgetAttrs((struct Gadget *)data->listbrowser_progress_obj, data->window, NULL,
        LISTBROWSER_Labels, &data->status_list,
        TAG_END);
}

/*------------------------------------------------------------------------*/
/* Perform Tool Update                                                    */
/*------------------------------------------------------------------------*/
static BOOL perform_tool_update(struct iTidy_DefaultToolUpdateWindow_ReAction *data)
{
    int i;
    int success_count = 0;
    int failed_count = 0;
    BOOL result;
    BPTR lock;
    struct FileInfoBlock *fib = NULL;
    char *info_path;
    const LayoutPreferences *prefs;
    BOOL backup_enabled;
    IconDetailsFromDisk icon_details;
    char *old_tool = NULL;
    
    if (data == NULL || data->update_in_progress)
        return FALSE;
    
    prefs = GetGlobalPreferences();
    backup_enabled = prefs->enable_default_tool_backup;
    
    /* Check if new tool path is empty */
    if (data->new_tool_path[0] == '\0')
    {
        if (!ShowReActionRequester(data->window,
                            "Clear Default Tool",
                            "This will remove the default tool from the selected icon(s).\n"
                            "The icon(s) will no longer launch a specific program.\n\n"
                            "Are you sure you want to continue?",
                            "_Yes, Clear Tool|_Cancel",
                            REQIMAGE_QUESTION))
        {
            return FALSE;
        }
    }
    
    data->update_in_progress = TRUE;
    
    safe_set_window_pointer(data->window, TRUE);
    
    /* Disable update button */
    SetGadgetAttrs((struct Gadget *)data->update_button_obj, data->window, NULL,
        GA_Disabled, TRUE,
        TAG_END);
    
    iTidy_FreeStatusEntries(data);
    
    if (backup_enabled)
    {
        iTidy_InitToolBackupManager(&data->backup_manager, TRUE);
        
        if (data->context.mode == UPDATE_MODE_BATCH)
        {
            iTidy_StartBackupSession(&data->backup_manager, "Batch",
                                    data->context.icon_paths[0]);
        }
        else
        {
            iTidy_StartBackupSession(&data->backup_manager, "Single",
                                    data->context.single_info_path);
        }
    }
    
    log_info(LOG_GUI, "=== BATCH UPDATE STARTING ===");
    log_info(LOG_GUI, "Old tool: '%s'", data->context.current_tool ? data->context.current_tool : "(none)");
    log_info(LOG_GUI, "New tool: '%s'", data->new_tool_path[0] ? data->new_tool_path : "(none)");
    log_info(LOG_GUI, "Icon count: %d", data->context.icon_count);
    
    if (data->context.mode == UPDATE_MODE_BATCH)
    {
        for (i = 0; i < data->context.icon_count; i++)
        {
            info_path = data->context.icon_paths[i];
            
            lock = Lock((CONST_STRPTR)info_path, ACCESS_READ);
            if (lock)
            {
                fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
                if (fib && Examine(lock, fib))
                {
                    if (fib->fib_Protection & FIBF_WRITE)
                    {
                        add_status_entry(data, info_path, "READ-ONLY");
                        UnLock(lock);
                        if (fib) FreeDosObject(DOS_FIB, fib);
                        
                        if (backup_enabled && data->backup_manager.session_active)
                            data->backup_manager.icons_skipped++;
                        
                        continue;
                    }
                }
                if (fib) FreeDosObject(DOS_FIB, fib);
                UnLock(lock);
            }
            
            memset(&icon_details, 0, sizeof(IconDetailsFromDisk));
            if (GetIconDetailsFromDisk(info_path, &icon_details, NULL))
            {
                old_tool = icon_details.defaultTool;
            }
            
            result = SetIconDefaultTool(info_path, 
                                       data->new_tool_path[0] ? data->new_tool_path : NULL);
            
            if (result)
            {
                if (backup_enabled && data->backup_manager.session_active)
                {
                    iTidy_RecordToolChange(&data->backup_manager, info_path,
                                          old_tool ? old_tool : "",
                                          data->new_tool_path[0] ? data->new_tool_path : "");
                }
                
                UpdateToolCacheForFileChange(info_path,
                                            old_tool ? old_tool : NULL,
                                            data->new_tool_path[0] ? data->new_tool_path : NULL);
                
                add_status_entry(data, info_path, "SUCCESS");
                success_count++;
            }
            else
            {
                add_status_entry(data, info_path, "FAILED");
                failed_count++;
                
                if (backup_enabled && data->backup_manager.session_active)
                    data->backup_manager.icons_skipped++;
            }
            
            if (old_tool)
            {
                FreeVec(old_tool);
                old_tool = NULL;
            }
            
            if ((i % 5) == 0)
                populate_status_list(data);
        }
    }
    else
    {
        info_path = data->context.single_info_path;
        
        lock = Lock((CONST_STRPTR)info_path, ACCESS_READ);
        if (lock)
        {
            fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
            if (fib && Examine(lock, fib))
            {
                if (fib->fib_Protection & FIBF_WRITE)
                {
                    add_status_entry(data, info_path, "READ-ONLY");
                    UnLock(lock);
                    FreeDosObject(DOS_FIB, fib);
                    
                    populate_status_list(data);
                    
                    if (backup_enabled && data->backup_manager.session_active)
                    {
                        data->backup_manager.icons_skipped++;
                        iTidy_EndBackupSession(&data->backup_manager);
                    }
                    
                    safe_set_window_pointer(data->window, FALSE);
                    
                    SetGadgetAttrs((struct Gadget *)data->update_button_obj, data->window, NULL,
                        GA_Disabled, FALSE,
                        TAG_END);
                    
                    data->update_in_progress = FALSE;
                    return FALSE;
                }
            }
            if (fib) FreeDosObject(DOS_FIB, fib);
            UnLock(lock);
        }
        
        memset(&icon_details, 0, sizeof(IconDetailsFromDisk));
        if (GetIconDetailsFromDisk(info_path, &icon_details, NULL))
        {
            old_tool = icon_details.defaultTool;
        }
        
        result = SetIconDefaultTool(info_path,
                                   data->new_tool_path[0] ? data->new_tool_path : NULL);
        
        if (result)
        {
            if (backup_enabled && data->backup_manager.session_active)
            {
                iTidy_RecordToolChange(&data->backup_manager, info_path,
                                      old_tool ? old_tool : "",
                                      data->new_tool_path[0] ? data->new_tool_path : "");
            }
            
            UpdateToolCacheForFileChange(info_path,
                                        old_tool ? old_tool : NULL,
                                        data->new_tool_path[0] ? data->new_tool_path : NULL);
            
            add_status_entry(data, info_path, "SUCCESS");
            success_count = 1;
        }
        else
        {
            add_status_entry(data, info_path, "FAILED");
            failed_count = 1;
            
            if (backup_enabled && data->backup_manager.session_active)
                data->backup_manager.icons_skipped++;
        }
        
        if (old_tool)
        {
            FreeVec(old_tool);
            old_tool = NULL;
        }
    }
    
    populate_status_list(data);
    
    if (backup_enabled && data->backup_manager.session_active)
    {
        iTidy_EndBackupSession(&data->backup_manager);
    }
    
    safe_set_window_pointer(data->window, FALSE);
    
    /* Keep update button disabled after completion */
    SetGadgetAttrs((struct Gadget *)data->update_button_obj, data->window, NULL,
        GA_Disabled, TRUE,
        TAG_END);
    
    data->update_in_progress = FALSE;
    
    log_info(LOG_GUI, "=== BATCH UPDATE COMPLETE ===");
    log_info(LOG_GUI, "Success: %d, Failed: %d", success_count, failed_count);
    
    {
        char msg_buffer[256];
        
        if (failed_count > 0)
        {
            sprintf(msg_buffer, 
                    "Update Complete\n\n"
                    "Successfully updated: %d icon%s\n"
                    "Failed: %d icon%s\n\n"
                    "Close this window to see updated tool cache.",
                    success_count, (success_count == 1) ? "" : "s",
                    failed_count, (failed_count == 1) ? "" : "s");
        }
        else
        {
            sprintf(msg_buffer,
                    "Update Complete\n\n"
                    "Successfully updated: %d icon%s\n\n"
                    "Close this window to see updated tool cache.",
                    success_count, (success_count == 1) ? "" : "s");
        }
        
        /* Show completion message using ReAction requester */
        ShowReActionRequester(data->window,
            "Default Tool Update",
            msg_buffer,
            "_Ok",
            REQIMAGE_INFO);

        /* Refresh window after requester closes */
        if (data->window)
        {
            RefreshWindowFrame(data->window);
        }
    }
    
    return (success_count > 0);
}

/*------------------------------------------------------------------------*/
/* Open Default Tool Update Window (ReAction)                             */
/*------------------------------------------------------------------------*/
BOOL iTidy_OpenDefaultToolUpdateWindow_ReAction(struct iTidy_DefaultToolUpdateWindow_ReAction *data,
                                                 struct iTidy_DefaultToolUpdateContext *context)
{
    struct DrawInfo *draw_info = NULL;
    
    if (data == NULL || context == NULL)
        return FALSE;
    
    log_info(LOG_GUI, "Opening Default Tool Update window (ReAction)...");
    
    /* Initialize structure */
    memset(data, 0, sizeof(struct iTidy_DefaultToolUpdateWindow_ReAction));
    
    /* Initialize Exec list using NewList() */
    NewList(&data->status_list);
    
    /* Copy context */
    memcpy(&data->context, context, sizeof(struct iTidy_DefaultToolUpdateContext));
    
    data->new_tool_path[0] = '\0';
    data->update_in_progress = FALSE;
    
    /* Build formatted label strings */
    snprintf(data->current_tool_label, sizeof(data->current_tool_label), 
             "Current tool: %s",
             data->context.current_tool ? data->context.current_tool : "(unknown)");
    
    if (data->context.mode == UPDATE_MODE_BATCH)
    {
        snprintf(data->mode_label, sizeof(data->mode_label),
                 "Batch Mode: %d icon(s) will be updated", data->context.icon_count);
    }
    else
    {
        char shortened_path[128];
        
        if (data->context.single_info_path && data->context.single_info_path[0] != '\0')
        {
            iTidy_ShortenPathWithParentDir(data->context.single_info_path, shortened_path, 40);
            snprintf(data->mode_label, sizeof(data->mode_label), 
                     "Single mode: %s", shortened_path);
        }
        else
        {
            snprintf(data->mode_label, sizeof(data->mode_label), 
                     "Single mode: Updating one icon");
        }
    }
    
    /* Lock Workbench screen */
    data->screen = LockPubScreen(NULL);
    if (data->screen == NULL)
    {
        log_error(LOG_GUI, "Could not lock Workbench screen");
        return FALSE;
    }
    
    /* Get DrawInfo for label images */
    draw_info = GetScreenDrawInfo(data->screen);
    if (draw_info == NULL)
    {
        log_error(LOG_GUI, "Could not get DrawInfo");
        UnlockPubScreen(NULL, data->screen);
        return FALSE;
    }
    
    /* Initialize ReAction libraries */
    if (!init_reaction_libs())
    {
        FreeScreenDrawInfo(data->screen, draw_info);
        UnlockPubScreen(NULL, data->screen);
        return FALSE;
    }
    
    /* Create ReAction window object tree */
    data->window_obj = WindowObject,
        WA_Title, "iTidy - Replace Default Tool",
        WA_Activate, TRUE,
        WA_DepthGadget, TRUE,
        WA_DragBar, TRUE,
        WA_CloseGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_PubScreen, data->screen,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WA_InnerWidth, 500,
        WA_InnerHeight, 350,
        WINDOW_ParentGroup, data->main_layout_obj = VLayoutObject,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_DeferLayout, TRUE,
                            LAYOUT_LeftSpacing, 2,
                LAYOUT_RightSpacing, 2,
                LAYOUT_TopSpacing, 2,

            
            /* Horizontal two-column layout with bevel for Current Tool / Mode / Change To */
            LAYOUT_AddChild, data->horizontal_layout_obj = HLayoutObject,
                GA_ID, GID_TOOL_UPDATE_CURRENT_TOOL_LAYOUT,
                LAYOUT_BevelStyle, BVS_STANDARD,
                LAYOUT_LeftSpacing, 2,
                LAYOUT_RightSpacing, 2,
                LAYOUT_TopSpacing, 4,
                LAYOUT_BottomSpacing, 4,
                
                /* Left column: labels (right-aligned) */
                LAYOUT_AddChild, data->left_column_obj = VLayoutObject,
                    LAYOUT_HorizAlignment, LALIGN_RIGHT,
                    LAYOUT_AddChild, data->label_current_tool_obj = ButtonObject,
                        GA_ID, GID_TOOL_UPDATE_LABEL_CURRENT_TOOL,
                        GA_Text, "Current tool:",
                        GA_ReadOnly, TRUE,
                        BUTTON_Transparent, TRUE,
                        BUTTON_BevelStyle, BVS_NONE,
                        BUTTON_Justification, BCJ_RIGHT,
                    ButtonEnd,
                    LAYOUT_AddChild, data->label_mode_obj = ButtonObject,
                        GA_ID, GID_TOOL_UPDATE_MODE_TEXT,
                        GA_Text, "Mode:",
                        GA_ReadOnly, TRUE,
                        BUTTON_Transparent, TRUE,
                        BUTTON_BevelStyle, BVS_NONE,
                        BUTTON_Justification, BCJ_RIGHT,
                    ButtonEnd,
                    LAYOUT_AddChild, data->label_change_to_obj = ButtonObject,
                        GA_Text, "Change to:",
                        GA_ReadOnly, TRUE,
                        BUTTON_Transparent, TRUE,
                        BUTTON_BevelStyle, BVS_NONE,
                        BUTTON_Justification, BCJ_RIGHT,
                    ButtonEnd,
                LayoutEnd,
                CHILD_WeightedWidth, 20,
                
                /* Right column: values (left-aligned) */
                LAYOUT_AddChild, data->right_column_obj = VLayoutObject,
                    LAYOUT_AddChild, data->current_tool_text_obj = ButtonObject,
                        GA_Text, data->current_tool_label,
                        GA_ReadOnly, TRUE,
                        BUTTON_Transparent, TRUE,
                        BUTTON_BevelStyle, BVS_NONE,
                        BUTTON_Justification, BCJ_LEFT,
                    ButtonEnd,
                    LAYOUT_AddChild, data->mode_text_obj = ButtonObject,
                        GA_Text, data->mode_label,
                        GA_ReadOnly, TRUE,
                        BUTTON_Transparent, TRUE,
                        BUTTON_BevelStyle, BVS_NONE,
                        BUTTON_Justification, BCJ_LEFT,
                    ButtonEnd,
                    LAYOUT_AddChild, data->new_tool_getfile_obj = GetFileObject,
                        GA_ID, GID_TOOL_UPDATE_NEW_TOOL_GETFILE,
                        GA_RelVerify, TRUE,
                        GETFILE_TitleText, "Select Default Tool Program",
                        GETFILE_Drawer, "C:",
                        GETFILE_ReadOnly, TRUE,
                        GETFILE_RejectIcons, TRUE,
                        GETFILE_DoPatterns, FALSE,
                    GetFileEnd,
                LayoutEnd,
                CHILD_WeightedWidth, 80,
            LayoutEnd,
            CHILD_WeightedHeight, 5,
            
            /* Update progress listview */
            LAYOUT_AddChild, data->update_progress_layout_obj = VLayoutObject,
                GA_ID, GID_TOOL_UPDATE_PROGRESS_LAYOUT,
                LAYOUT_HorizAlignment, LALIGN_CENTER,
                LAYOUT_TopSpacing, 2,
                LAYOUT_BottomSpacing, 2,
                LAYOUT_AddImage, data->update_progress_label_obj = LabelObject,
                    GA_ID, GID_TOOL_UPDATE_PROGRESS_LABEL,
                    LABEL_DrawInfo, draw_info,
                    LABEL_Text, "Update progress",
                    LABEL_Justification, LJ_CENTER,
                LabelEnd,
                CHILD_WeightedHeight, 5,
                LAYOUT_AddChild, data->listbrowser_progress_obj = ListBrowserObject,
                    GA_ID, GID_TOOL_UPDATE_LISTVIEW_PROGRESS,
                    GA_RelVerify, TRUE,
                    GA_TabCycle, TRUE,
                    LISTBROWSER_Labels, &data->status_list,
                    LISTBROWSER_ColumnInfo, NULL,  /* Will set after creating column info */
                    LISTBROWSER_ColumnTitles, TRUE,
                End,
            LayoutEnd,
            CHILD_WeightedHeight, 80,
            
            /* Buttons */
            LAYOUT_AddChild, data->buttons_layout_obj = HLayoutObject,
                GA_ID, GID_TOOL_UPDATE_BUTTONS_LAYOUT,
                LAYOUT_LeftSpacing, 0,
                LAYOUT_RightSpacing, 0,
                LAYOUT_BottomSpacing, 2,
                LAYOUT_AddChild, data->update_button_obj = ButtonObject,
                    GA_ID, GID_TOOL_UPDATE_UPDATE_BUTTON,
                    GA_Text, "Update Default Tool",
                    GA_RelVerify, TRUE,
                    GA_TabCycle, TRUE,
                ButtonEnd,
                LAYOUT_AddChild, data->close_button_obj = ButtonObject,
                    GA_ID, GID_TOOL_UPDATE_CLOSE_BUTTON,
                    GA_Text, "Close",
                    GA_RelVerify, TRUE,
                    GA_TabCycle, TRUE,
                ButtonEnd,
            LayoutEnd,
            CHILD_WeightedHeight, 5,
            
        LayoutEnd,
    WindowEnd;
    
    /* Free DrawInfo (no longer needed after window creation) */
    FreeScreenDrawInfo(data->screen, draw_info);
    draw_info = NULL;
    
    if (data->window_obj == NULL)
    {
        log_error(LOG_GUI, "Failed to create ReAction window object");
        close_reaction_libs();
        UnlockPubScreen(NULL, data->screen);
        return FALSE;
    }
    
    /* Open the window */
    data->window = (struct Window *)RA_OpenWindow(data->window_obj);
    if (data->window == NULL)
    {
        log_error(LOG_GUI, "Failed to open ReAction window");
        DisposeObject(data->window_obj);
        data->window_obj = NULL;
        close_reaction_libs();
        UnlockPubScreen(NULL, data->screen);
        return FALSE;
    }
    
    /* Configure ListBrowser columns */
    {
        struct ColumnInfo *column_info = AllocLBColumnInfo(2,
            LBCIA_Column, LBCOL_STATUS,
                LBCIA_Title, "Status",
                LBCIA_Weight, 15,
                LBCIA_Sortable, FALSE,
            LBCIA_Column, LBCOL_FILEPATH,
                LBCIA_Title, "File Path",
                LBCIA_Weight, 85,
                LBCIA_Sortable, FALSE,
            TAG_END);
        
        if (column_info)
        {
            SetGadgetAttrs((struct Gadget *)data->listbrowser_progress_obj, data->window, NULL,
                LISTBROWSER_ColumnInfo, column_info,
                TAG_END);
            
            /* Note: Don't free column_info - ListBrowser makes a copy */
        }
    }
    
    data->window_open = TRUE;
    log_info(LOG_GUI, "Default Tool Update window opened successfully (ReAction)");
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Close Default Tool Update Window (ReAction)                            */
/*------------------------------------------------------------------------*/
void iTidy_CloseDefaultToolUpdateWindow_ReAction(struct iTidy_DefaultToolUpdateWindow_ReAction *data)
{
    if (data == NULL)
        return;
    
    log_info(LOG_GUI, "Closing Default Tool Update window (ReAction)...");
    
    /* Detach list from ListBrowser gadget */
    if (data->window != NULL && data->listbrowser_progress_obj != NULL)
    {
        SetGadgetAttrs((struct Gadget *)data->listbrowser_progress_obj, data->window, NULL,
            LISTBROWSER_Labels, ~0,
            TAG_END);
    }
    
    /* Free status entries */
    iTidy_FreeStatusEntries(data);
    
    /* Cleanup backup manager */
    iTidy_CleanupToolBackupManager(&data->backup_manager);
    
    /* Refresh parent tool cache window */
    if (data->context.parent_window != NULL)
    {
        struct iTidyToolCacheWindow *parent = (struct iTidyToolCacheWindow *)data->context.parent_window;
        log_info(LOG_GUI, "Refreshing parent Tool Cache window after updates");
        refresh_tool_cache_window(parent);
    }
    
    /* Dispose window object (auto-disposes all child objects) */
    if (data->window_obj != NULL)
    {
        DisposeObject(data->window_obj);
        data->window_obj = NULL;
        data->window = NULL;
    }
    
    /* Close ReAction libraries */
    close_reaction_libs();
    
    /* Unlock screen */
    if (data->screen != NULL)
    {
        UnlockPubScreen(NULL, data->screen);
        data->screen = NULL;
    }
    
    data->window_open = FALSE;
    log_info(LOG_GUI, "Default Tool Update window closed (ReAction)");
}

/*------------------------------------------------------------------------*/
/* Handle Default Tool Update Window Events (ReAction)                    */
/*------------------------------------------------------------------------*/
BOOL iTidy_HandleDefaultToolUpdateEvents_ReAction(struct iTidy_DefaultToolUpdateWindow_ReAction *data)
{
    ULONG result;
    UWORD code;
    ULONG signal_mask;
    ULONG signals;
    
    if (data == NULL || data->window_obj == NULL)
        return FALSE;
    
    /* Get signal mask for the window */
    GetAttr(WINDOW_SigMask, data->window_obj, &signal_mask);
    
    /* Wait for signals */
    signals = Wait(signal_mask | SIGBREAKF_CTRL_C);
    
    /* Handle Ctrl-C break */
    if (signals & SIGBREAKF_CTRL_C)
    {
        return FALSE;
    }
    
    /* Process all pending events */
    while ((result = RA_HandleInput(data->window_obj, &code)) != WMHI_LASTMSG)
    {
        switch (result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                return FALSE;
                
            case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                    case GID_TOOL_UPDATE_NEW_TOOL_GETFILE:
                    {
                        /* GetFile gadget browse button clicked */
                        ULONG res = DoMethod(data->new_tool_getfile_obj, GFILE_REQUEST, data->window);
                        if (res)
                        {
                            STRPTR new_path = NULL;
                            GetAttr(GETFILE_FullFile, data->new_tool_getfile_obj, (ULONG *)&new_path);
                            if (new_path != NULL)
                            {
                                strncpy(data->new_tool_path, new_path, sizeof(data->new_tool_path) - 1);
                                data->new_tool_path[sizeof(data->new_tool_path) - 1] = '\0';
                                log_info(LOG_GUI, "Selected tool: %s", data->new_tool_path);
                            }
                        }
                        break;
                    }
                    
                    case GID_TOOL_UPDATE_UPDATE_BUTTON:
                        perform_tool_update(data);
                        break;
                        
                    case GID_TOOL_UPDATE_CLOSE_BUTTON:
                        return FALSE;
                }
                break;
        }
    }
    
    return TRUE;
}
