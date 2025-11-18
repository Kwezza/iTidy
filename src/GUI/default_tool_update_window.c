/*
 * default_tool_update_window.c - Default Tool Replacement Window Implementation
 * Provides UI for batch or single icon default tool replacement
 * GadTools-based window for Workbench 2.0+
 */

#include "platform/platform.h"
#include "default_tool_update_window.h"
#include "default_tool_backup.h"
#include "../icon_types.h"
#include "../itidy_types.h"
#include "../path_utilities.h"
#include "../Settings/IControlPrefs.h"
#include "../layout_preferences.h"
#include "../writeLog.h"
#include "easy_request_helper.h"

#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <graphics/text.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>

/* External graphics base for default font */
extern struct GfxBase *GfxBase;

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static void truncate_path_for_status(const char *path, char *output, int max_length);
static void populate_status_list(struct iTidy_DefaultToolUpdateWindow *data);
static BOOL perform_tool_update(struct iTidy_DefaultToolUpdateWindow *data);
static void add_status_entry(struct iTidy_DefaultToolUpdateWindow *data, 
                             const char *icon_path, const char *status_text);

/*------------------------------------------------------------------------*/
/* Truncate Path for Status Display                                      */
/*------------------------------------------------------------------------*/
/**
 * @brief Truncate path for status listview display
 * 
 * Uses middle truncation to preserve device and filename while
 * shortening the middle directories. This is temporary until
 * the global path_utilities module is implemented.
 * 
 * @param path Original path
 * @param output Buffer for truncated path (must be at least max_length+1)
 * @param max_length Maximum character length
 */
static void truncate_path_for_status(const char *path, char *output, int max_length)
{
    int len;
    int left_part_len;
    const char *last_slash;
    const char *last_colon;
    
    if (path == NULL || output == NULL || max_length < 10)
    {
        if (output != NULL && max_length > 0)
            output[0] = '\0';
        return;
    }
    
    len = strlen(path);
    
    /* If it fits, just copy it */
    if (len <= max_length)
    {
        strcpy(output, path);
        return;
    }
    
    /* Find the last slash or colon to identify the filename */
    last_slash = strrchr(path, '/');
    last_colon = strrchr(path, ':');
    
    /* Determine which separator is last */
    if (last_slash == NULL && last_colon == NULL)
    {
        /* No path separators - just truncate in middle */
        left_part_len = (max_length - 3) / 2;
        
        strncpy(output, path, left_part_len);
        output[left_part_len] = '\0';
        strcat(output, "...");
        strncat(output, path + len - (max_length - 3 - left_part_len), 
                max_length - 3 - left_part_len);
    }
    else
    {
        /* We have path separators - try to show device and filename */
        const char *separator = (last_slash > last_colon) ? last_slash : last_colon;
        int filename_len = strlen(separator);
        
        /* Calculate how much space we have for the left part */
        left_part_len = max_length - 3 - filename_len;
        
        if (left_part_len < 5)
        {
            /* Not enough space - just show end with ellipsis */
            strcpy(output, "...");
            strncat(output, separator, max_length - 3);
            output[max_length] = '\0';
        }
        else
        {
            /* Show device/start + ... + filename */
            strncpy(output, path, left_part_len);
            output[left_part_len] = '\0';
            strcat(output, "...");
            strcat(output, separator);
        }
    }
    
    /* Ensure null termination */
    output[max_length] = '\0';
}

/*------------------------------------------------------------------------*/
/* Add Status Entry                                                       */
/*------------------------------------------------------------------------*/
static void add_status_entry(struct iTidy_DefaultToolUpdateWindow *data,
                             const char *icon_path, const char *status_text)
{
    struct iTidy_StatusEntry *entry;
    char truncated_path[STATUS_ICON_PATH_WIDTH + 1];
    char buffer[256];
    
    if (data == NULL || icon_path == NULL || status_text == NULL)
        return;
    
    /* Allocate entry */
    entry = (struct iTidy_StatusEntry *)whd_malloc(sizeof(struct iTidy_StatusEntry));
    if (entry == NULL)
        return;
    
    memset(entry, 0, sizeof(struct iTidy_StatusEntry));
    
    /* Truncate path for display */
    truncate_path_for_status(icon_path, truncated_path, STATUS_ICON_PATH_WIDTH);
    
    /* Allocate and copy icon path */
    entry->icon_path = (char *)whd_malloc(strlen(truncated_path) + 1);
    if (entry->icon_path == NULL)
    {
        FreeVec(entry);
        return;
    }
    strcpy(entry->icon_path, truncated_path);
    
    /* Allocate and copy status text */
    entry->status_text = (char *)whd_malloc(strlen(status_text) + 1);
    if (entry->status_text == NULL)
    {
        FreeVec(entry->icon_path);
        FreeVec(entry);
        return;
    }
    strcpy(entry->status_text, status_text);
    
    /* Format display text: "path                                      | status    " */
    sprintf(buffer, "%-*s | %-*s", 
            STATUS_ICON_PATH_WIDTH, truncated_path,
            STATUS_TEXT_WIDTH, status_text);
    
    /* Allocate and copy display text */
    entry->display_text = (char *)whd_malloc(strlen(buffer) + 1);
    if (entry->display_text == NULL)
    {
        FreeVec(entry->status_text);
        FreeVec(entry->icon_path);
        FreeVec(entry);
        return;
    }
    strcpy(entry->display_text, buffer);
    
    /* Set node name for GadTools */
    entry->node.ln_Name = entry->display_text;
    
    /* Add to list (at tail to maintain order) */
    AddTail(&data->status_list, (struct Node *)entry);
}

/*------------------------------------------------------------------------*/
/* Free Status Entries                                                    */
/*------------------------------------------------------------------------*/
void iTidy_FreeStatusEntries(struct iTidy_DefaultToolUpdateWindow *data)
{
    struct iTidy_StatusEntry *entry;
    struct Node *node;
    
    if (data == NULL)
        return;
    
    /* Free all entries */
    while ((node = RemHead(&data->status_list)) != NULL)
    {
        entry = (struct iTidy_StatusEntry *)node;
        
        if (entry->icon_path != NULL)
            FreeVec(entry->icon_path);
        if (entry->status_text != NULL)
            FreeVec(entry->status_text);
        if (entry->display_text != NULL)
            FreeVec(entry->display_text);
        
        FreeVec(entry);
    }
}

/*------------------------------------------------------------------------*/
/* Populate Status List                                                   */
/*------------------------------------------------------------------------*/
static void populate_status_list(struct iTidy_DefaultToolUpdateWindow *data)
{
    if (data == NULL || data->window == NULL || data->status_listview == NULL)
        return;
    
    /* Detach list from gadget */
    GT_SetGadgetAttrs(data->status_listview, data->window, NULL,
        GTLV_Labels, ~0,
        TAG_END);
    
    /* Attach status list */
    GT_SetGadgetAttrs(data->status_listview, data->window, NULL,
        GTLV_Labels, &data->status_list,
        GTLV_Top, 0,  /* Scroll to top */
        TAG_END);
    
    /* Refresh gadget */
    RefreshGList(data->status_listview, data->window, NULL, 1);
}

/*------------------------------------------------------------------------*/
/* Perform Tool Update                                                    */
/*------------------------------------------------------------------------*/
static BOOL perform_tool_update(struct iTidy_DefaultToolUpdateWindow *data)
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
    
    /* Get global preferences for backup setting */
    prefs = GetGlobalPreferences();
    backup_enabled = prefs->enable_default_tool_backup;
    
    /* Check if new tool path is empty */
    if (data->new_tool_path[0] == '\0')
    {
        /* Ask user for confirmation */
        if (!ShowEasyRequest(data->window,
                            "Clear Default Tool",
                            "This will remove the default tool from the selected icon(s).\n"
                            "The icon(s) will no longer launch a specific program.\n\n"
                            "Are you sure you want to continue?",
                            "Yes, Clear Tool|Cancel"))
        {
            /* User cancelled */
            return FALSE;
        }
    }
    
    /* Mark update as in progress */
    data->update_in_progress = TRUE;
    
    /* Set busy pointer */
    SetWindowPointer(data->window,
        WA_BusyPointer, TRUE,
        TAG_END);
    
    /* Disable update button during operation */
    GT_SetGadgetAttrs(data->update_btn, data->window, NULL,
        GA_Disabled, TRUE,
        TAG_END);
    
    /* Clear previous status entries */
    iTidy_FreeStatusEntries(data);
    
    /* Initialize backup manager and start session if enabled */
    if (backup_enabled)
    {
        iTidy_InitToolBackupManager(&data->backup_manager, TRUE);
        
        if (data->context.mode == UPDATE_MODE_BATCH)
        {
            /* Start batch backup session */
            iTidy_StartBackupSession(&data->backup_manager, "Batch",
                                    data->context.icon_paths[0]);  /* Use first path as reference */
        }
        else
        {
            /* Start single backup session */
            iTidy_StartBackupSession(&data->backup_manager, "Single",
                                    data->context.single_info_path);
        }
    }
    
    /* Perform update based on mode */
    if (data->context.mode == UPDATE_MODE_BATCH)
    {
        /* Batch mode - update all icons */
        append_to_log("Batch update: %d icons to process\n", data->context.icon_count);
        
        for (i = 0; i < data->context.icon_count; i++)
        {
            info_path = data->context.icon_paths[i];
            
            /* Check if file is write-protected */
            lock = Lock((CONST_STRPTR)info_path, ACCESS_READ);
            if (lock)
            {
                fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
                if (fib && Examine(lock, fib))
                {
                    if (fib->fib_Protection & FIBF_WRITE)
                    {
                        /* File is write-protected */
                        add_status_entry(data, info_path, "READ-ONLY");
                        append_to_log("  %s: READ-ONLY (skipped)\n", info_path);
                        UnLock(lock);
                        if (fib) FreeDosObject(DOS_FIB, fib);
                        
                        /* Track skipped icon in backup */
                        if (backup_enabled && data->backup_manager.session_active)
                            data->backup_manager.icons_skipped++;
                        
                        continue;
                    }
                }
                if (fib) FreeDosObject(DOS_FIB, fib);
                UnLock(lock);
            }
            
            /* Get old default tool before changing (for backup) */
            memset(&icon_details, 0, sizeof(IconDetailsFromDisk));
            if (GetIconDetailsFromDisk(info_path, &icon_details))
            {
                old_tool = icon_details.defaultTool;  /* Will be freed later */
            }
            
            /* Attempt to update icon */
            result = SetIconDefaultTool(info_path, 
                                       data->new_tool_path[0] ? data->new_tool_path : NULL);
            
            if (result)
            {
                /* Record change to backup before showing success */
                if (backup_enabled && data->backup_manager.session_active)
                {
                    iTidy_RecordToolChange(&data->backup_manager, info_path,
                                          old_tool ? old_tool : "",
                                          data->new_tool_path[0] ? data->new_tool_path : "");
                }
                
                add_status_entry(data, info_path, "SUCCESS");
                append_to_log("  %s: SUCCESS\n", info_path);
                success_count++;
            }
            else
            {
                add_status_entry(data, info_path, "FAILED");
                append_to_log("  %s: FAILED\n", info_path);
                failed_count++;
                
                /* Track failed icon in backup */
                if (backup_enabled && data->backup_manager.session_active)
                    data->backup_manager.icons_skipped++;
            }
            
            /* Free old tool string (allocated by GetIconDetailsFromDisk) */
            if (old_tool)
            {
                FreeVec(old_tool);
                old_tool = NULL;
            }
            
            /* Update display periodically */
            if ((i % 5) == 0)
                populate_status_list(data);
        }
    }
    else
    {
        /* Single mode - update one icon */
        info_path = data->context.single_info_path;
        append_to_log("Single update: %s\n", info_path);
        
        /* Check if file is write-protected */
        lock = Lock((CONST_STRPTR)info_path, ACCESS_READ);
        if (lock)
        {
            fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
            if (fib && Examine(lock, fib))
            {
                if (fib->fib_Protection & FIBF_WRITE)
                {
                    /* File is write-protected */
                    add_status_entry(data, info_path, "READ-ONLY");
                    append_to_log("  READ-ONLY (skipped)\n");
                    UnLock(lock);
                    FreeDosObject(DOS_FIB, fib);
                    
                    /* Update display */
                    populate_status_list(data);
                    
                    /* End backup session if active */
                    if (backup_enabled && data->backup_manager.session_active)
                    {
                        data->backup_manager.icons_skipped++;
                        iTidy_EndBackupSession(&data->backup_manager);
                    }
                    
                    /* Restore normal pointer */
                    SetWindowPointer(data->window,
                        WA_BusyPointer, FALSE,
                        TAG_END);
                    
                    /* Re-enable update button */
                    GT_SetGadgetAttrs(data->update_btn, data->window, NULL,
                        GA_Disabled, FALSE,
                        TAG_END);
                    
                    data->update_in_progress = FALSE;
                    return FALSE;
                }
            }
            if (fib) FreeDosObject(DOS_FIB, fib);
            UnLock(lock);
        }
        
        /* Get old default tool before changing (for backup) */
        memset(&icon_details, 0, sizeof(IconDetailsFromDisk));
        if (GetIconDetailsFromDisk(info_path, &icon_details))
        {
            old_tool = icon_details.defaultTool;  /* Will be freed later */
        }
        
        /* Attempt to update icon */
        result = SetIconDefaultTool(info_path,
                                   data->new_tool_path[0] ? data->new_tool_path : NULL);
        
        if (result)
        {
            /* Record change to backup before showing success */
            if (backup_enabled && data->backup_manager.session_active)
            {
                iTidy_RecordToolChange(&data->backup_manager, info_path,
                                      old_tool ? old_tool : "",
                                      data->new_tool_path[0] ? data->new_tool_path : "");
            }
            
            add_status_entry(data, info_path, "SUCCESS");
            append_to_log("  SUCCESS\n");
            success_count = 1;
        }
        else
        {
            add_status_entry(data, info_path, "FAILED");
            append_to_log("  FAILED\n");
            failed_count = 1;
            
            /* Track failed icon in backup */
            if (backup_enabled && data->backup_manager.session_active)
                data->backup_manager.icons_skipped++;
        }
        
        /* Free old tool string (allocated by GetIconDetailsFromDisk) */
        if (old_tool)
        {
            FreeVec(old_tool);
            old_tool = NULL;
        }
    }
    
    /* Final display update */
    populate_status_list(data);
    
    /* End backup session if active */
    if (backup_enabled && data->backup_manager.session_active)
    {
        iTidy_EndBackupSession(&data->backup_manager);
    }
    
    /* Restore normal pointer */
    SetWindowPointer(data->window,
        WA_BusyPointer, FALSE,
        TAG_END);
    
    /* Re-enable update button */
    GT_SetGadgetAttrs(data->update_btn, data->window, NULL,
        GA_Disabled, FALSE,
        TAG_END);
    
    /* Clear progress flag */
    data->update_in_progress = FALSE;
    
    append_to_log("Update complete: %d succeeded, %d failed\n", 
                 success_count, failed_count);
    
    return (success_count > 0);
}

/*------------------------------------------------------------------------*/
/* Open ASL File Requester                                                */
/*------------------------------------------------------------------------*/
static void open_file_requester(struct iTidy_DefaultToolUpdateWindow *data)
{
    struct FileRequester *freq;
    
    if (data == NULL || data->window == NULL)
        return;
    
    /* Allocate file requester */
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Select Default Tool Program",
        ASLFR_InitialDrawer, "C:",
        ASLFR_DoPatterns, FALSE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, data->window,
        TAG_END);
    
    if (freq == NULL)
    {
        append_to_log("ERROR: Could not allocate file requester\n");
        return;
    }
    
    /* Display requester */
    if (AslRequest(freq, NULL))
    {
        /* User selected a file */
        char full_path[256];
        
        /* Build full path */
        strcpy(full_path, freq->fr_Drawer);
        if (AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            /* Update string gadget */
            strcpy(data->new_tool_path, full_path);
            
            GT_SetGadgetAttrs(data->new_path_string, data->window, NULL,
                GTST_String, data->new_tool_path,
                TAG_END);
            
            RefreshGList(data->new_path_string, data->window, NULL, 1);
            
            append_to_log("Selected tool: %s\n", full_path);
        }
    }
    
    /* Free requester */
    FreeAslRequest(freq);
}

/*------------------------------------------------------------------------*/
/* Open Default Tool Update Window                                        */
/*------------------------------------------------------------------------*/
BOOL iTidy_OpenDefaultToolUpdateWindow(struct iTidy_DefaultToolUpdateWindow *data,
                                        struct iTidy_DefaultToolUpdateContext *context)
{
    struct NewGadget ng;
    struct Gadget *gad;
    struct DrawInfo *draw_info = NULL;
    struct TextFont *font;
    struct RastPort temp_rp;
    UWORD font_width, font_height;
    UWORD button_height, string_height, listview_line_height;
    UWORD current_x, current_y;
    UWORD window_width, window_height;
    UWORD reference_width, precalc_max_right;
    UWORD equal_button_width, max_btn_text_width, temp_width;
    UWORD label_width, input_width, browse_btn_width;
    UWORD status_listview_height;
    BOOL using_system_font = FALSE;
    
    if (data == NULL || context == NULL)
        return FALSE;
    
    /* Initialize structure */
    memset(data, 0, sizeof(struct iTidy_DefaultToolUpdateWindow));
    NewList(&data->status_list);
    
    /* Copy context */
    memcpy(&data->context, context, sizeof(struct iTidy_DefaultToolUpdateContext));
    
    /* Debug: Log what we received */
    append_to_log("iTidy_OpenDefaultToolUpdateWindow: current_tool = '%s'\n",
        data->context.current_tool ? data->context.current_tool : "(NULL)");
    append_to_log("  mode = %d, icon_count = %d\n",
        data->context.mode, data->context.icon_count);
    
    /* Initialize new tool path to empty */
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
        /* Single mode - show path using iTidy_ShortenPathWithParentDir */
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
        append_to_log("ERROR: Could not lock Workbench screen\n");
        return FALSE;
    }
    
    /* Get DrawInfo */
    draw_info = GetScreenDrawInfo(data->screen);
    if (draw_info == NULL)
    {
        append_to_log("ERROR: Could not get DrawInfo\n");
        UnlockPubScreen(NULL, data->screen);
        return FALSE;
    }
    
    font = draw_info->dri_Font;
    font_width = font->tf_XSize;
    font_height = font->tf_YSize;
    
    /* Check if screen font is proportional - use system default if so */
    if (font->tf_Flags & FPF_PROPORTIONAL)
    {
        append_to_log("WARNING: Screen uses proportional font - using system default for columns\n");
        
        /* Open system default text font */
        data->system_font_attr.ta_Name = (STRPTR)GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name;
        data->system_font_attr.ta_YSize = GfxBase->DefaultFont->tf_YSize;
        data->system_font_attr.ta_Style = FS_NORMAL;
        data->system_font_attr.ta_Flags = 0;
        
        data->system_font = OpenFont(&data->system_font_attr);
        if (data->system_font != NULL)
        {
            font = data->system_font;
            font_width = font->tf_XSize;
            font_height = font->tf_YSize;
            using_system_font = TRUE;
            append_to_log("Using system font: %s %d\n",
                data->system_font_attr.ta_Name,
                data->system_font_attr.ta_YSize);
        }
    }
    
    /* Initialize temp RastPort for TextLength measurements */
    InitRastPort(&temp_rp);
    SetFont(&temp_rp, font);
    
    /* Calculate gadget heights */
    button_height = font_height + 6;
    string_height = font_height + 6;
    listview_line_height = font_height + 2;
    
    /*--------------------------------------------------------------------*/
    /* PRE-CALCULATE LAYOUT DIMENSIONS                                   */
    /*--------------------------------------------------------------------*/
    reference_width = font_width * TOOL_UPDATE_WINDOW_WIDTH_CHARS;
    
    current_x = TOOL_UPDATE_WINDOW_MARGIN_LEFT + prefsIControl.currentLeftBarWidth;
    current_y = TOOL_UPDATE_WINDOW_MARGIN_TOP + prefsIControl.currentWindowBarHeight;
    
    precalc_max_right = current_x + reference_width;
    
    /* Calculate input row dimensions */
    label_width = TextLength(&temp_rp, "Change to:", 10);
    input_width = font_width * TOOL_UPDATE_PATH_INPUT_CHARS;
    browse_btn_width = precalc_max_right - (current_x + label_width + 4 + input_width + TOOL_UPDATE_WINDOW_SPACE_X);
    
    /* Calculate status listview height */
    status_listview_height = listview_line_height * TOOL_UPDATE_STATUS_LINES;
    
    /* Calculate equal-width buttons (Update + Close = 2 buttons) */
    max_btn_text_width = TextLength(&temp_rp, "Update Default Tool", 19);
    temp_width = TextLength(&temp_rp, "Close", 5);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    
    equal_button_width = reference_width;  /* Full width buttons */
    if (equal_button_width < max_btn_text_width + TOOL_UPDATE_BUTTON_PADDING)
        equal_button_width = max_btn_text_width + TOOL_UPDATE_BUTTON_PADDING;
    
    append_to_log("=== PRE-CALCULATED LAYOUT ===\n");
    append_to_log("Reference width: %d, max_right: %d\n", reference_width, precalc_max_right);
    append_to_log("Browse button width: %d\n", browse_btn_width);
    
    /* Get visual info */
    data->visual_info = GetVisualInfo(data->screen, TAG_END);
    if (data->visual_info == NULL)
    {
        append_to_log("ERROR: Could not get visual info\n");
        if (using_system_font && data->system_font != NULL)
            CloseFont(data->system_font);
        FreeScreenDrawInfo(data->screen, draw_info);
        UnlockPubScreen(NULL, data->screen);
        return FALSE;
    }
    
    /* Create gadgets */
    gad = CreateContext(&data->glist);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create gadget context\n");
        if (using_system_font && data->system_font != NULL)
            CloseFont(data->system_font);
        FreeVisualInfo(data->visual_info);
        FreeScreenDrawInfo(data->screen, draw_info);
        UnlockPubScreen(NULL, data->screen);
        return FALSE;
    }
    
    /* Set text attr for all gadgets if using system font */
    ng.ng_TextAttr = using_system_font ? &data->system_font_attr : data->screen->Font;
    ng.ng_VisualInfo = data->visual_info;
    
    /*--------------------------------------------------------------------*/
    /* CURRENT TOOL TEXT (with bevel box for visual separation)          */
    /*--------------------------------------------------------------------*/
    /*--------------------------------------------------------------------*/
    /* CURRENT TOOL LABEL (formatted as "Current tool: xxx")            */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = reference_width;
    ng.ng_Height = font_height;  /* Use font_height for consistent text label spacing */
    ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = 0;  /* Non-interactive */
    ng.ng_Flags = 0;
    
    data->current_tool_text = gad = CreateGadget(TEXT_KIND, gad, &ng,
        GTTX_Text, data->current_tool_label,
        GTTX_Border, FALSE,
        TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create current tool label\n");
        goto cleanup_error;
    }
    
    current_y += gad->Height + TOOL_UPDATE_WINDOW_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* MODE LABEL (shows batch or single mode with details)             */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = reference_width;
    ng.ng_Height = font_height;
    ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = 0;
    ng.ng_Flags = 0;
    
    data->mode_text = gad = CreateGadget(TEXT_KIND, gad, &ng,
        GTTX_Text, data->mode_label,
        GTTX_Border, FALSE,
        TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create mode label\n");
        goto cleanup_error;
    }

    
    current_y += gad->Height + TOOL_UPDATE_WINDOW_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* NEW TOOL PATH INPUT ROW                                           */
    /*--------------------------------------------------------------------*/
    /* VERTICAL ALIGNMENT STRATEGY:
     * When aligning a label (TEXT gadget) with a taller input gadget:
     * 1. The taller gadget (string/button) defines the row baseline at current_y
     * 2. The label is vertically centered using: label_top = baseline + (tall_height - label_height) / 2
     * 3. Gadget heights are available immediately after CreateGadget() via gad->Height
     * 4. Pre-calculate offset using known heights (string_height vs font_height)
     * This creates consistent vertical centering for all label+input pairs.
     */
    
    /* "Change to:" label (vertically centered with string gadget) */
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y + (string_height - font_height) / 2;  /* Vertically centered */
    ng.ng_Width = label_width;
    ng.ng_Height = font_height;
    ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = 0;  /* Non-interactive */
    ng.ng_Flags = 0;
    
    data->change_to_label = gad = CreateGadget(TEXT_KIND, gad, &ng,
        GTTX_Text, "Change to:",
        GTTX_Border, FALSE,
        TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create 'Change to:' label\n");
        goto cleanup_error;
    }
    
    /* String gadget to the right of label (at row baseline) */
    ng.ng_LeftEdge = current_x + label_width + 4;
    ng.ng_TopEdge = current_y;  /* Row baseline */
    ng.ng_Width = input_width;
    ng.ng_Height = string_height;
    ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = GID_TOOL_NEW_PATH_STRING;
    ng.ng_Flags = 0;
    
    data->new_path_string = gad = CreateGadget(STRING_KIND, gad, &ng,
        GTST_String, data->new_tool_path,
        GTST_MaxChars, 255,
        GA_ReadOnly, TRUE,  /* Read-only - user must use Browse button */
        TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create new path string\n");
        goto cleanup_error;
    }
    
    /* Browse button to the right */
    ng.ng_LeftEdge = current_x + label_width + 4 + input_width + TOOL_UPDATE_WINDOW_SPACE_X;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = browse_btn_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Browse...";
    ng.ng_GadgetID = GID_TOOL_BROWSE_BTN;
    ng.ng_Flags = PLACETEXT_IN;
    
    data->browse_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create browse button\n");
        goto cleanup_error;
    }
    
    current_y += button_height + TOOL_UPDATE_WINDOW_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* BACKUP CHECKBOX (disabled for now)                                */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = font_height;
    ng.ng_Height = font_height + 4;  /* Add padding for label text */
    ng.ng_GadgetText = "Backup icon(s) before update (not yet implemented)";
    ng.ng_GadgetID = GID_TOOL_BACKUP_CHECKBOX;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    data->backup_checkbox = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, FALSE,
        GA_Disabled, TRUE,  /* Disabled for now */
        TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create backup checkbox\n");
        goto cleanup_error;
    }
    
    current_y += gad->Height + TOOL_UPDATE_WINDOW_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* UPDATE BUTTON (full width)                                        */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Update Default Tool";
    ng.ng_GadgetID = GID_TOOL_UPDATE_BTN;
    ng.ng_Flags = PLACETEXT_IN;
    
    data->update_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create update button\n");
        goto cleanup_error;
    }
    
    current_y += button_height + TOOL_UPDATE_WINDOW_SPACE_Y * 2;
    
    /*--------------------------------------------------------------------*/
    /* STATUS LISTVIEW (shows update progress)                           */
    /*--------------------------------------------------------------------*/
    /* CRITICAL: Reserve space for PLACETEXT_ABOVE label to prevent overlap */
    /* The label "Update Progress:" is placed ABOVE ng.ng_TopEdge */
    current_y += font_height + 4;  /* Reserve space for label above ListView */
    
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = reference_width;
    ng.ng_Height = status_listview_height;
    ng.ng_GadgetText = "Update Progress:";
    ng.ng_GadgetID = GID_TOOL_STATUS_LISTVIEW;
    ng.ng_Flags = PLACETEXT_ABOVE;
    
    data->status_listview = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, &data->status_list,
        GTLV_ReadOnly, TRUE,
        TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create status listview\n");
        goto cleanup_error;
    }
    
    current_y = gad->TopEdge + gad->Height + TOOL_UPDATE_WINDOW_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* CLOSE BUTTON (full width)                                         */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Close";
    ng.ng_GadgetID = GID_TOOL_CLOSE_BTN;
    ng.ng_Flags = PLACETEXT_IN;
    
    data->close_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create close button\n");
        goto cleanup_error;
    }
    
    /* Calculate final window size */
    window_width = precalc_max_right + TOOL_UPDATE_WINDOW_MARGIN_RIGHT;
    window_height = current_y + button_height + TOOL_UPDATE_WINDOW_MARGIN_BOTTOM;
    
    /* Set window title */
    strcpy(data->window_title, "iTidy - Replace Default Tool");
    
    /* Open window */
    data->window = OpenWindowTags(NULL,
        WA_Left, (data->screen->Width - window_width) / 2,
        WA_Top, (data->screen->Height - window_height) / 2,
        WA_Width, window_width,
        WA_Height, window_height,
        WA_Title, data->window_title,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, data->screen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_GADGETUP,
        WA_Gadgets, data->glist,
        TAG_END);
    
    if (data->window == NULL)
    {
        append_to_log("ERROR: Could not open default tool update window\n");
        goto cleanup_error;
    }
    
    /* Free DrawInfo (no longer needed) */
    FreeScreenDrawInfo(data->screen, draw_info);
    draw_info = NULL;
    
    /* Refresh TEXT gadgets to display their content */
    /* Note: ALL TEXT_KIND gadgets require explicit refresh, even borderless ones! */
    GT_RefreshWindow(data->window, NULL);
    RefreshGList(data->current_tool_text, data->window, NULL, 1);
    RefreshGList(data->mode_text, data->window, NULL, 1);
    RefreshGList(data->change_to_label, data->window, NULL, 1);
    
    data->window_open = TRUE;
    append_to_log("Default tool update window opened successfully\n");
    
    return TRUE;
    
cleanup_error:
    if (draw_info != NULL)
        FreeScreenDrawInfo(data->screen, draw_info);
    if (data->glist != NULL)
        FreeGadgets(data->glist);
    if (data->visual_info != NULL)
        FreeVisualInfo(data->visual_info);
    if (using_system_font && data->system_font != NULL)
        CloseFont(data->system_font);
    if (data->screen != NULL)
        UnlockPubScreen(NULL, data->screen);
    
    return FALSE;
}

/*------------------------------------------------------------------------*/
/* Close Default Tool Update Window                                       */
/*------------------------------------------------------------------------*/
void iTidy_CloseDefaultToolUpdateWindow(struct iTidy_DefaultToolUpdateWindow *data)
{
    if (data == NULL)
        return;
    
    /* Detach lists from gadgets FIRST */
    if (data->window != NULL && data->status_listview != NULL)
    {
        GT_SetGadgetAttrs(data->status_listview, data->window, NULL,
            GTLV_Labels, ~0,
            TAG_END);
    }
    
    /* Free status entries */
    iTidy_FreeStatusEntries(data);
    
    /* Cleanup backup manager (will end session if still active) */
    iTidy_CleanupToolBackupManager(&data->backup_manager);
    
    /* Close window */
    if (data->window != NULL)
    {
        CloseWindow(data->window);
        data->window = NULL;
    }
    
    /* Free gadgets */
    if (data->glist != NULL)
    {
        FreeGadgets(data->glist);
        data->glist = NULL;
    }
    
    /* Free visual info */
    if (data->visual_info != NULL)
    {
        FreeVisualInfo(data->visual_info);
        data->visual_info = NULL;
    }
    
    /* Close system font */
    if (data->system_font != NULL)
    {
        CloseFont(data->system_font);
        data->system_font = NULL;
    }
    
    /* Unlock screen */
    if (data->screen != NULL)
    {
        UnlockPubScreen(NULL, data->screen);
        data->screen = NULL;
    }
    
    data->window_open = FALSE;
    append_to_log("Default tool update window closed\n");
}

/*------------------------------------------------------------------------*/
/* Handle Default Tool Update Window Events                               */
/*------------------------------------------------------------------------*/
BOOL iTidy_HandleDefaultToolUpdateEvents(struct iTidy_DefaultToolUpdateWindow *data)
{
    struct IntuiMessage *msg;
    ULONG msg_class;
    UWORD gadget_id;
    struct Gadget *gadget;
    
    if (data == NULL || data->window == NULL)
        return FALSE;
    
    while ((msg = GT_GetIMsg(data->window->UserPort)) != NULL)
    {
        msg_class = msg->Class;
        gadget = (struct Gadget *)msg->IAddress;
        gadget_id = (gadget != NULL) ? gadget->GadgetID : 0;
        
        GT_ReplyIMsg(msg);
        
        switch (msg_class)
        {
            case IDCMP_CLOSEWINDOW:
                return FALSE;  /* Close window */
                
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(data->window);
                GT_EndRefresh(data->window, TRUE);
                break;
                
            case IDCMP_GADGETUP:
                switch (gadget_id)
                {
                    case GID_TOOL_BROWSE_BTN:
                        open_file_requester(data);
                        break;
                        
                    case GID_TOOL_UPDATE_BTN:
                        perform_tool_update(data);
                        break;
                        
                    case GID_TOOL_CLOSE_BTN:
                        return FALSE;  /* Close window */
                }
                break;
        }
    }
    
    return TRUE;  /* Continue running */
}
