/*
 * exclude_paths_window.c - DefIcons Exclude Paths Management Window (ReAction)
 * 
 * GUI window for managing user-configurable exclude paths for DefIcons
 * icon creation scanning. Supports DEVICE: placeholder substitution.
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

/* Library base isolation - prevent linker conflicts with other windows */
#define WindowBase iTidy_ExcludePaths_WindowBase
#define LayoutBase iTidy_ExcludePaths_LayoutBase
#define ListBrowserBase iTidy_ExcludePaths_ListBrowserBase
#define ButtonBase iTidy_ExcludePaths_ButtonBase
#define GetFileBase iTidy_ExcludePaths_GetFileBase
#define RequesterBase iTidy_ExcludePaths_RequesterBase
#define SpaceBase iTidy_ExcludePaths_SpaceBase

#include "exclude_paths_window.h"
#include "../writeLog.h"
#include "../backups/backup_paths.h"
#include "../platform/platform.h"

#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/button.h>
#include <proto/getfile.h>
#include <proto/requester.h>
#include <proto/space.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/asl.h>

#include <classes/window.h>
#include <classes/requester.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <gadgets/button.h>
#include <gadgets/getfile.h>
#include <gadgets/space.h>

#include <string.h>
#include <stdio.h>

/* Local library bases */
struct Library *iTidy_ExcludePaths_WindowBase = NULL;
struct Library *iTidy_ExcludePaths_LayoutBase = NULL;
struct Library *iTidy_ExcludePaths_ListBrowserBase = NULL;
struct Library *iTidy_ExcludePaths_ButtonBase = NULL;
struct Library *iTidy_ExcludePaths_GetFileBase = NULL;
struct Library *iTidy_ExcludePaths_RequesterBase = NULL;
struct Library *iTidy_ExcludePaths_SpaceBase = NULL;

/* Gadget IDs */
enum {
    GID_PATH_LIST = 1,
    GID_ADD,
    GID_REMOVE,
    GID_MODIFY,
    GID_RESET_DEFAULTS,
    GID_OK,
    GID_CANCEL
};

/* Window state structure */
struct ExcludePathsWindow {
    Object *window_obj;
    Object *path_listbrowser;
    Object *add_button;
    Object *remove_button;
    Object *modify_button;
    Object *reset_button;
    Object *ok_button;
    Object *cancel_button;
    
    struct List *path_list;
    DefIconsExcludePaths *exclude_paths; /* Working copy of exclude paths */
    const char *folder_path;            /* Current scan folder (for ASL initial drawer) */
    BOOL user_cancelled;
};

/*========================================================================*/
/* Helper Functions                                                      */
/*========================================================================*/

/**
 * @brief Create ListBrowser column definition
 */
static struct ColumnInfo *create_column_info(void)
{
    static struct ColumnInfo column_info[] = {
        { 100, "Exclude Path", 0 },
        { -1, NULL, 0 }
    };
    
    return column_info;
}

/**
 * @brief Refresh ListBrowser display from preferences
 */
static BOOL refresh_path_list(struct ExcludePathsWindow *win, struct Window *window)
{
    struct Node *node;
    int i;
    
    if (!win || !win->exclude_paths)
        return FALSE;
    
    /* Detach and free existing nodes */
    SetGadgetAttrs((struct Gadget *)win->path_listbrowser, window, NULL,
        LISTBROWSER_Labels, ~0,
        TAG_DONE);
    
    /* Free old list nodes */
    while ((node = RemHead(win->path_list)))
    {
        FreeListBrowserNode(node);
    }
    
    /* Create new nodes from exclude paths */
    for (i = 0; i < (int)win->exclude_paths->count; i++)
    {
        node = AllocListBrowserNode(1,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, win->exclude_paths->paths[i],
            TAG_DONE);
        
        if (node)
        {
            AddTail(win->path_list, node);
        }
    }
    
    /* Reattach list to gadget */
    SetGadgetAttrs((struct Gadget *)win->path_listbrowser, window, NULL,
        LISTBROWSER_Labels, win->path_list,
        TAG_DONE);
    
    return TRUE;
}

/**
 * @brief Show ReAction requester for confirmation
 */
static BOOL show_reset_confirmation(struct Window *parent_window)
{
    Object *req_obj;
    struct orRequest req_msg;
    ULONG result = 0;
    
    req_obj = NewObject(REQUESTER_GetClass(), NULL,
        REQ_Type, REQTYPE_INFO,
        REQ_TitleText, "Reset to Defaults",
        REQ_BodyText, "Reset exclude list to defaults?\nCurrent list will be lost.",
        REQ_GadgetText, "_Reset|_Cancel",
        REQ_Image, REQIMAGE_WARNING,
        TAG_DONE);
    
    if (req_obj)
    {
        req_msg.MethodID = RM_OPENREQ;
        req_msg.or_Attrs = NULL;
        req_msg.or_Window = parent_window;
        req_msg.or_Screen = NULL;
        
        result = DoMethodA(req_obj, (Msg)&req_msg);
        DisposeObject(req_obj);
    }
    
    return (result == 1);  /* 1 = Reset button, 0 = Cancel */
}

/**
 * @brief Get the real device/volume name from a path (resolves assigns)
 * @param path Input path (can be assign like "SYS:" or real volume like "Workbench:")
 * @param device_name Output buffer for device name (should be at least 32 bytes)
 * @return TRUE if successful, FALSE otherwise
 */
static BOOL get_real_device_name(const char *path, char *device_name)
{
    BPTR lock;
    char full_path[256];
    BOOL success = FALSE;
    
    if (!path || !device_name)
    {
        if (device_name) device_name[0] = '\0';
        return FALSE;
    }
    
    /* Lock the path */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock)
    {
        /* Get full path with real device name */
        if (NameFromLock(lock, full_path, sizeof(full_path)))
        {
            /* Extract device name */
            if (GetDeviceName(full_path, device_name))
            {
                success = TRUE;
            }
        }
        UnLock(lock);
    }
    
    if (!success)
    {
        /* Fallback: just extract device from path as-is */
        GetDeviceName(path, device_name);
    }
    
    return success;
}

/**
 * @brief Ask user if path should be stored as DEVICE: pattern or absolute
 */
static BOOL ask_device_pattern_preference(struct Window *parent_window, 
                                          const char *selected_path,
                                          const char *scan_device)
{
    Object *req_obj;
    struct orRequest req_msg;
    ULONG result = 0;
    char body_text[512];
    
    /* Extract path after device for DEVICE: preview */
    const char *path_after_device = strchr(selected_path, ':');
    if (path_after_device)
    {
        path_after_device++; /* Skip the ':' */
    }
    else
    {
        path_after_device = selected_path; /* No device - use as-is */
    }
    
    snprintf(body_text, sizeof(body_text),
        "Store as portable DEVICE: pattern or absolute path?\n\n"
        "Selected: %s\n\n"
        "DEVICE: -> DEVICE:%s\n"
        "Absolute -> %s\n\n"
        "DEVICE: pattern allows exclude list to work across\n"
        "different volumes (e.g., SYS:Fonts, Work:Fonts, etc.)",
        selected_path,
        path_after_device,
        selected_path);
    
    req_obj = NewObject(REQUESTER_GetClass(), NULL,
        REQ_Type, REQTYPE_INFO,
        REQ_TitleText, "Store Path As",
        REQ_BodyText, body_text,
        REQ_GadgetText, "_DEVICE:|_Absolute|_Cancel",
        REQ_Image, REQIMAGE_QUESTION,
        TAG_DONE);
    
    if (req_obj)
    {
        req_msg.MethodID = RM_OPENREQ;
        req_msg.or_Attrs = NULL;
        req_msg.or_Window = parent_window;
        req_msg.or_Screen = NULL;
        
        result = DoMethodA(req_obj, (Msg)&req_msg);
        DisposeObject(req_obj);
    }
    
    return (result == 1);  /* 1 = DEVICE:, 2 = Absolute, 0 = Cancel */
}

/**
 * @brief Convert absolute path to DEVICE: pattern if it matches scan device
 */
static void convert_to_device_pattern(const char *absolute_path, 
                                      const char *scan_device,
                                      char *output_pattern,
                                      size_t output_size)
{
    const char *colon_pos;
    char path_device[32];
    
    if (!absolute_path || !output_pattern)
        return;
    
    /* Extract device from absolute path */
    colon_pos = strchr(absolute_path, ':');
    if (!colon_pos)
    {
        /* No device - copy as-is */
        strncpy(output_pattern, absolute_path, output_size - 1);
        output_pattern[output_size - 1] = '\0';
        return;
    }
    
    /* Extract device name */
    size_t device_len = colon_pos - absolute_path;
    if (device_len >= sizeof(path_device))
        device_len = sizeof(path_device) - 1;
    
    strncpy(path_device, absolute_path, device_len);
    path_device[device_len] = '\0';
    
    /* Check if it matches scan device */
    if (scan_device[0] != '\0' && Stricmp(path_device, scan_device) == 0)
    {
        /* Convert to DEVICE: pattern */
        snprintf(output_pattern, output_size, "DEVICE:%s", colon_pos + 1);
    }
    else
    {
        /* Different device - keep absolute */
        strncpy(output_pattern, absolute_path, output_size - 1);
        output_pattern[output_size - 1] = '\0';
    }
}

/**
 * @brief Handle Add button - open GetFile gadget
 */
static void handle_add_path(struct ExcludePathsWindow *win, struct Window *window)
{
    struct FileRequester *freq;
    char selected_path[256];
    char device[32];
    char final_path[256];
    BOOL use_device_pattern;
    
    if (!win)
        return;
    
    /* Get REAL scan device for DEVICE: substitution (resolves assigns) */
    if (!get_real_device_name(win->folder_path, device))
    {
        device[0] = '\0';
    }
    
    /* Open ASL file requester (drawers only) */
    freq = (struct FileRequester *)AllocAslRequest(ASL_FileRequest, NULL);
    if (freq)
    {
        if (AslRequestTags(freq,
            ASLFR_TitleText, "Select Directory to Exclude",
            ASLFR_DrawersOnly, TRUE,
            ASLFR_InitialDrawer, win->folder_path,
            TAG_DONE))
        {
            BPTR dir_lock;
            
            /* Build path from ASL result */
            snprintf(selected_path, sizeof(selected_path), "%s%s",
                freq->fr_Drawer, freq->fr_File);
            
            /* Get full absolute path using Lock/NameFromLock */
            dir_lock = Lock((STRPTR)selected_path, ACCESS_READ);
            if (dir_lock)
            {
                if (!NameFromLock(dir_lock, selected_path, sizeof(selected_path)))
                {
                    /* NameFromLock failed - use original */
                    snprintf(selected_path, sizeof(selected_path), "%s%s",
                        freq->fr_Drawer, freq->fr_File);
                }
                UnLock(dir_lock);
            }
            
            /* Check if selected path's device matches scan device */
            char selected_device[32];
            BOOL devices_match = FALSE;
            
            if (device[0] != '\0' && GetDeviceName(selected_path, selected_device))
            {
                devices_match = (Stricmp(selected_device, device) == 0);
            }
            
            /* Ask user: DEVICE: pattern or absolute? (only if devices match) */
            if (devices_match)
            {
                use_device_pattern = ask_device_pattern_preference(window, 
                    selected_path, device);
                
                if (use_device_pattern)
                {
                    /* Convert to DEVICE: pattern */
                    convert_to_device_pattern(selected_path, device, 
                        final_path, sizeof(final_path));
                }
                else
                {
                    /* Use absolute */
                    strncpy(final_path, selected_path, sizeof(final_path) - 1);
                    final_path[sizeof(final_path) - 1] = '\0';
                }
            }
            else
            {
                /* Different device - store as absolute */
                strncpy(final_path, selected_path, sizeof(final_path) - 1);
                final_path[sizeof(final_path) - 1] = '\0';
            }
            
            /* Add to preferences */
            if (add_deficons_exclude_path(win->exclude_paths, final_path))
            {
                refresh_path_list(win, window);
                log_info(LOG_GUI, "Added exclude path: %s\n", final_path);
            }
            else
            {
                log_warning(LOG_GUI, "Failed to add exclude path (duplicate or list full)\n");
            }
        }
        
        FreeAslRequest(freq);
    }
}

/**
 * @brief Handle Remove button - delete selected path
 */
static void handle_remove_path(struct ExcludePathsWindow *win, struct Window *window)
{
    LONG selected_index = -1;
    
    if (!win)
        return;
    
    /* Get selected index */
    GetAttr(LISTBROWSER_Selected, win->path_listbrowser, (ULONG *)&selected_index);
    
    if (selected_index >= 0)
    {
        if (remove_deficons_exclude_path(win->exclude_paths, selected_index))
        {
            refresh_path_list(win, window);
            log_info(LOG_GUI, "Removed exclude path at index %ld\n", selected_index);
        }
    }
}

/**
 * @brief Handle Modify button - edit selected path
 */
static void handle_modify_path(struct ExcludePathsWindow *win, struct Window *window)
{
    LONG selected_index = -1;
    struct FileRequester *freq;
    char selected_path[256];
    char device[32];
    char initial_path[256];
    char final_path[256];
    BOOL use_device_pattern;
    
    if (!win)
        return;
    
    /* Get selected index */
    GetAttr(LISTBROWSER_Selected, win->path_listbrowser, (ULONG *)&selected_index);
    
    if (selected_index < 0 || selected_index >= (LONG)win->exclude_paths->count)
        return;
    
    /* Get current path */
    strncpy(selected_path, win->exclude_paths->paths[selected_index], 
        sizeof(selected_path) - 1);
    selected_path[sizeof(selected_path) - 1] = '\0';
    
    log_debug(LOG_GUI, "[MODIFY] Step 1: Selected path from list: '%s'\n", selected_path);
    log_debug(LOG_GUI, "[MODIFY] Step 2: Scan folder path: '%s'\n", win->folder_path);
    
    /* Get REAL scan device (resolves assigns like SYS: -> Workbench:) */
    if (!get_real_device_name(win->folder_path, device))
    {
        device[0] = '\0';
    }
    
    log_debug(LOG_GUI, "[MODIFY] Step 3: Extracted REAL scan device: '%s'\n", 
        device[0] ? device : "(none)");
    
    /* Substitute DEVICE: for display */
    if (strncmp(selected_path, "DEVICE:", 7) == 0 && device[0] != '\0')
    {
        snprintf(initial_path, sizeof(initial_path), "%s:%s", 
            device, selected_path + 7);
        log_debug(LOG_GUI, "[MODIFY] Step 4: Substituted DEVICE: -> '%s'\n", initial_path);
    }
    else
    {
        strncpy(initial_path, selected_path, sizeof(initial_path) - 1);
        initial_path[sizeof(initial_path) - 1] = '\0';
        log_debug(LOG_GUI, "[MODIFY] Step 4: No DEVICE: substitution, using: '%s'\n", 
            initial_path);
    }
    
    log_debug(LOG_GUI, "[MODIFY] Step 5: Opening ASL at: '%s'\n", initial_path);
    
    /* Open ASL file requester */
    freq = (struct FileRequester *)AllocAslRequest(ASL_FileRequest, NULL);
    if (freq)
    {
        if (AslRequestTags(freq,
            ASLFR_TitleText, "Modify Exclude Path",
            ASLFR_DrawersOnly, TRUE,
            ASLFR_InitialDrawer, initial_path,
            TAG_DONE))
        {
            BPTR dir_lock;
            
            /* Build path from ASL result */
            snprintf(selected_path, sizeof(selected_path), "%s%s",
                freq->fr_Drawer, freq->fr_File);
            
            log_debug(LOG_GUI, "[MODIFY] Step 6: ASL returned: Drawer='%s' File='%s'\n",
                freq->fr_Drawer, freq->fr_File);
            log_debug(LOG_GUI, "[MODIFY] Step 7: Combined path: '%s'\n", selected_path);
            
            /* Get full absolute path using Lock/NameFromLock */
            dir_lock = Lock((STRPTR)selected_path, ACCESS_READ);
            if (dir_lock)
            {
                if (!NameFromLock(dir_lock, selected_path, sizeof(selected_path)))
                {
                    /* NameFromLock failed - use original */
                    snprintf(selected_path, sizeof(selected_path), "%s%s",
                        freq->fr_Drawer, freq->fr_File);
                    log_debug(LOG_GUI, "[MODIFY] Step 8: NameFromLock FAILED, using: '%s'\n",
                        selected_path);
                }
                else
                {
                    log_debug(LOG_GUI, "[MODIFY] Step 8: NameFromLock SUCCESS: '%s'\n",
                        selected_path);
                }
                UnLock(dir_lock);
            }
            else
            {
                log_debug(LOG_GUI, "[MODIFY] Step 8: Lock FAILED on '%s'\n", selected_path);
            }
            
            /* Check if selected path's device matches scan device */
            char selected_device[32];
            BOOL devices_match = FALSE;
            
            if (device[0] != '\0' && GetDeviceName(selected_path, selected_device))
            {
                devices_match = (Stricmp(selected_device, device) == 0);
                log_debug(LOG_GUI, "[MODIFY] Step 9: Selected device='%s', scan device='%s', match=%s\n",
                    selected_device, device, devices_match ? "YES" : "NO");
            }
            else
            {
                log_debug(LOG_GUI, "[MODIFY] Step 9: No device comparison (scan_device=%s)\n",
                    device[0] ? "empty" : "(none)");
            }
            
            /* Ask user: DEVICE: pattern or absolute? (only if devices match) */
            if (devices_match)
            {
                log_debug(LOG_GUI, "[MODIFY] Step 10: Devices match - asking user preference\n");
                use_device_pattern = ask_device_pattern_preference(window,
                    selected_path, device);
                
                if (use_device_pattern)
                {
                    convert_to_device_pattern(selected_path, device,
                        final_path, sizeof(final_path));
                    log_debug(LOG_GUI, "[MODIFY] Step 11: User chose DEVICE:, converted to: '%s'\n",
                        final_path);
                }
                else
                {
                    strncpy(final_path, selected_path, sizeof(final_path) - 1);
                    final_path[sizeof(final_path) - 1] = '\0';
                    log_debug(LOG_GUI, "[MODIFY] Step 11: User chose Absolute: '%s'\n", final_path);
                }
            }
            else
            {
                /* Different device - store as absolute */
                strncpy(final_path, selected_path, sizeof(final_path) - 1);
                final_path[sizeof(final_path) - 1] = '\0';
                log_debug(LOG_GUI, "[MODIFY] Step 10: Devices don't match - storing as absolute: '%s'\n",
                    final_path);
            }
            
            /* Update preferences */
            if (modify_deficons_exclude_path(win->exclude_paths, selected_index, final_path))
            {
                refresh_path_list(win, window);
                log_info(LOG_GUI, "Modified exclude path at index %ld to: '%s'\n", 
                    selected_index, final_path);
            }
        }
        
        FreeAslRequest(freq);
    }
}

/**
 * @brief Handle Reset to Defaults button
 */
static void handle_reset_defaults(struct ExcludePathsWindow *win, struct Window *window)
{
    if (!win)
        return;
    
    if (show_reset_confirmation(window))
    {
        reset_deficons_exclude_paths_to_defaults(win->exclude_paths);
        refresh_path_list(win, window);
        log_info(LOG_GUI, "Reset exclude paths to defaults\n");
    }
}

/*========================================================================*/
/* Event Loop                                                            */
/*========================================================================*/

static void handle_events(struct ExcludePathsWindow *win)
{
    struct Window *window = NULL;
    ULONG signal_mask, signals;
    ULONG result;
    UWORD code;
    BOOL done = FALSE;
    
    if (!win || !win->window_obj)
        return;
    
    /* Get window pointer and signal */
    GetAttr(WINDOW_Window, win->window_obj, (ULONG *)&window);
    GetAttr(WINDOW_SigMask, win->window_obj, &signal_mask);
    
    while (!done)
    {
        signals = Wait(signal_mask | SIGBREAKF_CTRL_C);
        
        if (signals & SIGBREAKF_CTRL_C)
        {
            done = TRUE;
            win->user_cancelled = TRUE;
            continue;
        }
        
        while ((result = RA_HandleInput(win->window_obj, &code)) != WMHI_LASTMSG)
        {
            switch (result & WMHI_CLASSMASK)
            {
                case WMHI_CLOSEWINDOW:
                    done = TRUE;
                    win->user_cancelled = TRUE;
                    break;
                    
                case WMHI_GADGETUP:
                    switch (result & WMHI_GADGETMASK)
                    {
                        case GID_ADD:
                            handle_add_path(win, window);
                            break;
                            
                        case GID_REMOVE:
                            handle_remove_path(win, window);
                            break;
                            
                        case GID_MODIFY:
                            handle_modify_path(win, window);
                            break;
                            
                        case GID_RESET_DEFAULTS:
                            handle_reset_defaults(win, window);
                            break;
                            
                        case GID_OK:
                            done = TRUE;
                            win->user_cancelled = FALSE;
                            break;
                            
                        case GID_CANCEL:
                            done = TRUE;
                            win->user_cancelled = TRUE;
                            break;
                    }
                    break;
            }
        }
    }
}

/*========================================================================*/
/* Public Functions                                                      */
/*========================================================================*/

BOOL open_exclude_paths_window(DefIconsExcludePaths *exclude_paths, const char *folder_path)
{
    struct ExcludePathsWindow win_data;
    Object *window_obj = NULL;
    Object *layout_obj = NULL;
    Object *button_layout = NULL;
    struct Window *window = NULL;
    BOOL success = FALSE;
    
    if (!exclude_paths)
    {
        log_error(LOG_GUI, "Invalid parameters to open_exclude_paths_window\n");
        return FALSE;
    }
    
    /* Initialize window state */
    memset(&win_data, 0, sizeof(win_data));
    win_data.exclude_paths = exclude_paths;
    win_data.folder_path = folder_path ? folder_path : "";
    win_data.user_cancelled = TRUE;
    
    /* Open ReAction classes */
    iTidy_ExcludePaths_WindowBase = OpenLibrary("window.class", 0);
    iTidy_ExcludePaths_LayoutBase = OpenLibrary("gadgets/layout.gadget", 0);
    iTidy_ExcludePaths_ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 0);
    iTidy_ExcludePaths_ButtonBase = OpenLibrary("gadgets/button.gadget", 0);
    iTidy_ExcludePaths_SpaceBase = OpenLibrary("gadgets/space.gadget", 0);
    iTidy_ExcludePaths_RequesterBase = OpenLibrary("requester.class", 0);
    
    if (!iTidy_ExcludePaths_WindowBase || !iTidy_ExcludePaths_LayoutBase ||
        !iTidy_ExcludePaths_ListBrowserBase || !iTidy_ExcludePaths_ButtonBase ||
        !iTidy_ExcludePaths_SpaceBase || !iTidy_ExcludePaths_RequesterBase)
    {
        log_error(LOG_GUI, "Failed to open ReAction classes for exclude paths window\n");
        goto cleanup;
    }
    
    /* Create ListBrowser node list */
    win_data.path_list = (struct List *)AllocVec(sizeof(struct List), MEMF_CLEAR);
    if (!win_data.path_list)
    {
        log_error(LOG_GUI, "Failed to allocate path list\n");
        goto cleanup;
    }
    NewList(win_data.path_list);
    
    /* Create ListBrowser gadget */
    win_data.path_listbrowser = NewObject(LISTBROWSER_GetClass(), NULL,
        GA_ID, GID_PATH_LIST,
        GA_RelVerify, TRUE,
        LISTBROWSER_ColumnInfo, create_column_info(),
        LISTBROWSER_ColumnTitles, TRUE,
        LISTBROWSER_Labels, win_data.path_list,
        LISTBROWSER_ShowSelected, TRUE,
        TAG_DONE);
    
    /* Create buttons */
    win_data.add_button = NewObject(BUTTON_GetClass(), NULL,
        GA_ID, GID_ADD,
        GA_Text, "_Add...",
        GA_RelVerify, TRUE,
        TAG_DONE);
    
    win_data.remove_button = NewObject(BUTTON_GetClass(), NULL,
        GA_ID, GID_REMOVE,
        GA_Text, "_Remove",
        GA_RelVerify, TRUE,
        TAG_DONE);
    
    win_data.modify_button = NewObject(BUTTON_GetClass(), NULL,
        GA_ID, GID_MODIFY,
        GA_Text, "_Modify...",
        GA_RelVerify, TRUE,
        TAG_DONE);
    
    win_data.reset_button = NewObject(BUTTON_GetClass(), NULL,
        GA_ID, GID_RESET_DEFAULTS,
        GA_Text, "Reset to _Defaults",
        GA_RelVerify, TRUE,
        TAG_DONE);
    
    win_data.ok_button = NewObject(BUTTON_GetClass(), NULL,
        GA_ID, GID_OK,
        GA_Text, "_OK",
        GA_RelVerify, TRUE,
        TAG_DONE);
    
    win_data.cancel_button = NewObject(BUTTON_GetClass(), NULL,
        GA_ID, GID_CANCEL,
        GA_Text, "_Cancel",
        GA_RelVerify, TRUE,
        TAG_DONE);
    
    /* Check if all buttons were created */
    if (!win_data.add_button || !win_data.remove_button || !win_data.modify_button ||
        !win_data.reset_button || !win_data.ok_button || !win_data.cancel_button)
    {
        log_error(LOG_GUI, "Failed to create one or more button gadgets\n");
        goto cleanup;
    }
    
    /* Create button layout */
    button_layout = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
        LAYOUT_AddChild, win_data.add_button,
        CHILD_WeightedWidth, 0,
        LAYOUT_AddChild, win_data.remove_button,
        CHILD_WeightedWidth, 0,
        LAYOUT_AddChild, win_data.modify_button,
        CHILD_WeightedWidth, 0,
        LAYOUT_AddChild, win_data.reset_button,
        CHILD_WeightedWidth, 0,
        LAYOUT_AddChild, NewObject(SPACE_GetClass(), NULL, TAG_DONE),
        CHILD_WeightedWidth, 100,
        LAYOUT_AddChild, win_data.ok_button,
        CHILD_WeightedWidth, 0,
        LAYOUT_AddChild, win_data.cancel_button,
        CHILD_WeightedWidth, 0,
        TAG_DONE);
    
    /* Create main layout */
    layout_obj = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_AddChild, win_data.path_listbrowser,
        CHILD_WeightedHeight, 100,
        LAYOUT_AddChild, button_layout,
        CHILD_WeightedHeight, 0,
        TAG_DONE);
    
    /* Create window */
    window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title, "DefIcons Exclude Paths",
        WA_Activate, TRUE,
        WA_DepthGadget, TRUE,
        WA_DragBar, TRUE,
        WA_CloseGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_InnerWidth, 600,
        WA_InnerHeight, 300,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_Layout, layout_obj,
        TAG_DONE);
    
    if (!window_obj)
    {
        log_error(LOG_GUI, "Failed to create exclude paths window object\n");
        goto cleanup;
    }
    
    win_data.window_obj = window_obj;
    
    /* Open window */
    window = (struct Window *)RA_OpenWindow(window_obj);
    if (!window)
    {
        log_error(LOG_GUI, "Failed to open exclude paths window\n");
        goto cleanup;
    }
    
    /* Populate list from preferences */
    refresh_path_list(&win_data, window);
    
    /* Run event loop */
    handle_events(&win_data);
    
    /* Check if user clicked OK */
    success = !win_data.user_cancelled;
    
cleanup:
    /* Close window */
    if (window_obj)
    {
        RA_CloseWindow(window_obj);
        DisposeObject(window_obj);
    }
    
    /* Free path list */
    if (win_data.path_list)
    {
        struct Node *node;
        while ((node = RemHead(win_data.path_list)))
        {
            FreeListBrowserNode(node);
        }
        FreeVec(win_data.path_list);
    }
    
    /* Close libraries */
    if (iTidy_ExcludePaths_RequesterBase)
        CloseLibrary(iTidy_ExcludePaths_RequesterBase);
    if (iTidy_ExcludePaths_SpaceBase)
        CloseLibrary(iTidy_ExcludePaths_SpaceBase);
    if (iTidy_ExcludePaths_ButtonBase)
        CloseLibrary(iTidy_ExcludePaths_ButtonBase);
    if (iTidy_ExcludePaths_ListBrowserBase)
        CloseLibrary(iTidy_ExcludePaths_ListBrowserBase);
    if (iTidy_ExcludePaths_LayoutBase)
        CloseLibrary(iTidy_ExcludePaths_LayoutBase);
    if (iTidy_ExcludePaths_WindowBase)
        CloseLibrary(iTidy_ExcludePaths_WindowBase);
    
    return success;
}
