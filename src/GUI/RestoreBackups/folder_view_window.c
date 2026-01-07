/*
 * folder_view_window.c - iTidy Folder View Window Implementation
 * 
 * Based on proven working test_simple_window.c - avoids MuForce errors
 * by using simple font handling and avoiding complex DrawInfo operations.
 * 
 * CRITICAL: Uses screen->Font directly instead of OpenFont() to avoid
 * the 7FFF0000 MuForce errors that occurred with complex font handling.
 */

#include "platform/platform.h"
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/dos.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include "../../helpers/exec_list_compat.h"

#include "folder_view_window.h"
#include "writeLog.h"
#include "GUI/gui_utilities.h"

#include <string.h>
#include <stdio.h>

/*------------------------------------------------------------------------*/
/* Forward Declarations for Helper Functions                             */
/*------------------------------------------------------------------------*/
static UWORD calculate_folder_depth(const char *path);
static const char *get_folder_name(const char *path);
static void format_folder_display(const char *path, UWORD depth, char *buffer, UWORD buffer_size);
static void format_folder_display_with_size(const char *path, UWORD depth, const char *size_str, char *buffer, UWORD buffer_size);
static BOOL parse_catalog_callback(const char *line, struct iTidyFolderViewWindow *folder_data);

/*------------------------------------------------------------------------*/
/**
 * @brief Open the Folder View Window for a backup run
 */
/*------------------------------------------------------------------------*/
BOOL open_folder_view_window(struct iTidyFolderViewWindow *folder_data,
                             const char *catalog_path,
                             UWORD run_number,
                             const char *date_str,
                             ULONG archive_count)
{
    struct NewGadget ng;
    struct Gadget *gad;
    
    if (folder_data == NULL || folder_data->screen == NULL)
    {
        append_to_log("ERROR: Invalid folder_data or screen\n");
        return FALSE;
    }
    
    append_to_log("=== FOLDER VIEW WINDOW - Opening ===\n");
    append_to_log("Catalog path: %s\n", catalog_path ? catalog_path : "NULL");
    append_to_log("Run number: %u\n", (unsigned int)run_number);
    
    /* Initialize data */
    folder_data->run_number = run_number;
    folder_data->archive_count = archive_count;
    
    /* Set the window title - MUST be in the structure, not on stack! */
    if (date_str != NULL)
    {
        sprintf(folder_data->window_title, "Folder View - Run %u (%s)", 
                (unsigned int)run_number, date_str);
        strncpy(folder_data->date_str, date_str, sizeof(folder_data->date_str) - 1);
        folder_data->date_str[sizeof(folder_data->date_str) - 1] = '\0';
    }
    else
    {
        sprintf(folder_data->window_title, "Folder View - Run %u", 
                (unsigned int)run_number);
        strcpy(folder_data->date_str, "Unknown");
    }
    
    sprintf(folder_data->run_name, "Run_%04u", (unsigned int)run_number);
    
    append_to_log("Title set to: %s (address: %p)\n", 
                  folder_data->window_title, folder_data->window_title);
    
    /* Initialize folder list - start empty for fast window opening */
    NewList(&folder_data->folder_entries);
    append_to_log("Initialized folder entries list (empty for now)\n");
    
    /* Store catalog path for later parsing - AFTER window opens */
    const char *stored_catalog_path = catalog_path;
    
    /* Get visual info for GadTools - CRITICAL: use simple approach */
    folder_data->visual_info = GetVisualInfo(folder_data->screen, TAG_DONE);
    if (folder_data->visual_info == NULL)
    {
        append_to_log("ERROR: Failed to get visual info\n");
        free_folder_entries(folder_data);
        return FALSE;
    }
    append_to_log("Visual info: %p\n", folder_data->visual_info);
    
    /* Create gadget context */
    gad = CreateContext(&folder_data->glist);
    if (gad == NULL)
    {
        append_to_log("ERROR: Failed to create gadget context\n");
        FreeVisualInfo(folder_data->visual_info);
        folder_data->visual_info = NULL;
        free_folder_entries(folder_data);
        return FALSE;
    }
    append_to_log("Gadget context created: %p\n", folder_data->glist);
    
    /* Create ListView gadget - CRITICAL: use screen->Font directly */
    ng.ng_LeftEdge = FOLDER_VIEW_MARGIN_LEFT;
    ng.ng_TopEdge = FOLDER_VIEW_MARGIN_TOP + 20;
    ng.ng_Width = 380;  /* Adjust for 400px window width */
    ng.ng_Height = 120;  /* Reduced to make room for Close button */
    ng.ng_GadgetText = "Folders:";
    ng.ng_TextAttr = folder_data->screen->Font;  /* SIMPLE approach - no custom fonts! */
    ng.ng_GadgetID = GID_FOLDER_LIST;
    ng.ng_Flags = PLACETEXT_ABOVE;
    ng.ng_VisualInfo = folder_data->visual_info;
    
    gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, &folder_data->folder_entries,
        GTLV_ShowSelected, NULL,
        TAG_DONE);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Failed to create ListView\n");
        FreeGadgets(folder_data->glist);
        folder_data->glist = NULL;
        FreeVisualInfo(folder_data->visual_info);
        folder_data->visual_info = NULL;
        free_folder_entries(folder_data);
        return FALSE;
    }
    folder_data->folder_list = gad;
    append_to_log("ListView created: %p\n", gad);
    
    /* Create Close button - CRITICAL: use screen->Font directly */
    ng.ng_LeftEdge = FOLDER_VIEW_MARGIN_LEFT;
    ng.ng_TopEdge = ng.ng_TopEdge + ng.ng_Height + 10;
    ng.ng_Width = 380;  /* Same width as ListView */
    ng.ng_Height = 20;
    ng.ng_GadgetText = "Close";
    ng.ng_TextAttr = folder_data->screen->Font;  /* SIMPLE approach - no custom fonts! */
    ng.ng_GadgetID = GID_FOLDER_CLOSE_BTN;
    ng.ng_Flags = 0;
    ng.ng_VisualInfo = folder_data->visual_info;
    
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    if (gad == NULL)
    {
        append_to_log("ERROR: Failed to create Close button\n");
        FreeGadgets(folder_data->glist);
        folder_data->glist = NULL;
        FreeVisualInfo(folder_data->visual_info);
        folder_data->visual_info = NULL;
        free_folder_entries(folder_data);
        return FALSE;
    }
    folder_data->close_btn = gad;
    append_to_log("Close button created: %p\n", gad);
    
    /* Open the window - CRITICAL: use permanent title string */
    folder_data->window = OpenWindowTags(NULL,
        WA_Left, 100,
        WA_Top, 50,
        WA_Width, 400,   /* Requested width */
        WA_Height, 200,  /* Requested height */
        WA_Title, folder_data->window_title,  /* PERMANENT string in structure */
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, folder_data->screen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_REFRESHWINDOW,
        WA_Gadgets, folder_data->glist,
        TAG_DONE);
    
    if (folder_data->window == NULL)
    {
        append_to_log("ERROR: Failed to open folder view window\n");
        FreeGadgets(folder_data->glist);
        folder_data->glist = NULL;
        FreeVisualInfo(folder_data->visual_info);
        folder_data->visual_info = NULL;
        free_folder_entries(folder_data);
        return FALSE;
    }
    
    append_to_log("Folder view window opened successfully at %p\n", folder_data->window);
    append_to_log("Window UserPort: %p\n", folder_data->window->UserPort);
    
    /* Set busy pointer while populating listview */
    safe_set_window_pointer(folder_data->window, TRUE);
    
    /* NOW parse catalog and build folder tree - AFTER window is open */
    if (stored_catalog_path != NULL)
    {
        append_to_log("Now parsing catalog and populating listview...\n");
        if (!parse_catalog_and_build_tree(stored_catalog_path, folder_data))
        {
            append_to_log("ERROR: Failed to parse catalog\n");
            
            /* Clear busy pointer even on error */
            safe_set_window_pointer(folder_data->window, FALSE);
            
            /* Close window and cleanup */
            close_folder_view_window(folder_data);
            return FALSE;
        }
        append_to_log("Catalog parsed successfully, entries: %lu\n", 
                     (ULONG)folder_data->folder_entries.lh_Head);
        
        /* Update the ListView with the newly populated list */
        GT_SetGadgetAttrs(folder_data->folder_list, folder_data->window, NULL,
                          GTLV_Labels, &folder_data->folder_entries,
                          TAG_DONE);
    }
    else
    {
        /* Add some test data if no catalog provided */
        append_to_log("No catalog provided, adding test data\n");
        struct FolderEntry *entry;
        
        /* Root folder */
        entry = (struct FolderEntry *)whd_malloc(sizeof(struct FolderEntry));
        if (entry)
        {
            memset(entry, 0, sizeof(struct FolderEntry));
            entry->path = (char *)whd_malloc(strlen("Work:") + 1);
            entry->display_text = (char *)whd_malloc(64);
            if (entry->path && entry->display_text)
            {
                memset(entry->path, 0, strlen("Work:") + 1);
                memset(entry->display_text, 0, 64);
                strcpy(entry->path, "Work:");
                strcpy(entry->display_text, "Work:");
                entry->depth = 0;
                entry->node.ln_Name = entry->display_text;
                AddTail(&folder_data->folder_entries, (struct Node *)entry);
            }
            else
            {
                if (entry->path) FreeVec(entry->path);
                if (entry->display_text) FreeVec(entry->display_text);
                FreeVec(entry);
            }
        }
        
        /* Update the ListView with test data */
        GT_SetGadgetAttrs(folder_data->folder_list, folder_data->window, NULL,
                          GTLV_Labels, &folder_data->folder_entries,
                          TAG_DONE);
    }
    
    /* Refresh gadgets to show the populated list */
    GT_RefreshWindow(folder_data->window, NULL);
    append_to_log("Gadgets refreshed with populated list\n");
    
    /* Clear busy pointer - listview is populated and window is ready */
    safe_set_window_pointer(folder_data->window, FALSE);
    
    folder_data->window_open = TRUE;
    folder_data->system_font = NULL;  /* We're not using custom fonts */
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Handle folder view window events (main event loop)
 */
/*------------------------------------------------------------------------*/
BOOL handle_folder_view_window_events(struct iTidyFolderViewWindow *folder_data)
{
    struct IntuiMessage *msg;
    ULONG msg_class;
    UWORD gadget_id;
    
    if (folder_data == NULL || folder_data->window == NULL || !folder_data->window_open)
    {
        return FALSE;
    }
    
    /* Check for messages without blocking */
    while ((msg = GT_GetIMsg(folder_data->window->UserPort)) != NULL)
    {
        msg_class = msg->Class;
        
        switch (msg_class)
        {
            case IDCMP_GADGETUP:
                gadget_id = ((struct Gadget *)msg->IAddress)->GadgetID;
                GT_ReplyIMsg(msg);
                
                append_to_log("Gadget clicked, ID: %d\n", gadget_id);
                if (gadget_id == GID_FOLDER_CLOSE_BTN)
                {
                    append_to_log("Close button clicked\n");
                    return FALSE;  /* Signal to close window */
                }
                break;
                
            case IDCMP_GADGETDOWN:
                /* Handle ListView scroll arrow buttons */
                gadget_id = ((struct Gadget *)msg->IAddress)->GadgetID;
                GT_ReplyIMsg(msg);
                
                if (gadget_id == GID_FOLDER_LIST)
                {
                    /* Get current top position */
                    LONG current_top = 0;
                    GT_GetGadgetAttrs(folder_data->folder_list, folder_data->window, NULL,
                                      GTLV_Top, &current_top,
                                      TAG_END);
                    
                    append_to_log("ListView scroll button pressed, current top: %ld\n", current_top);
                    
                    /* The scroll direction is determined by which arrow was clicked,
                     * but we need to let GadTools handle the actual scrolling.
                     * We refresh the window to ensure proper display. */
                    GT_RefreshWindow(folder_data->window, NULL);
                }
                break;
                
            case IDCMP_CLOSEWINDOW:
                GT_ReplyIMsg(msg);
                append_to_log("Close window gadget clicked\n");
                return FALSE;  /* Signal to close window */
                
            case IDCMP_REFRESHWINDOW:
                GT_ReplyIMsg(msg);
                GT_BeginRefresh(folder_data->window);
                GT_EndRefresh(folder_data->window, TRUE);
                break;
                
            default:
                GT_ReplyIMsg(msg);
                break;
        }
    }
    
    return TRUE;  /* Continue running */
}

/*------------------------------------------------------------------------*/
/**
 * @brief Close the Folder View Window and cleanup resources
 */
/*------------------------------------------------------------------------*/
void close_folder_view_window(struct iTidyFolderViewWindow *folder_data)
{
    struct IntuiMessage *msg;
    
    if (folder_data == NULL)
    {
        return;
    }
    
    append_to_log("=== FOLDER VIEW WINDOW - Closing ===\n");
    
    if (folder_data->window != NULL)
    {
        append_to_log("Window pointer: %p\n", folder_data->window);
        
        /* Flush any remaining messages */
        if (folder_data->window->UserPort != NULL)
        {
            append_to_log("Flushing messages...\n");
            while ((msg = GT_GetIMsg(folder_data->window->UserPort)) != NULL)
            {
                GT_ReplyIMsg(msg);
            }
            append_to_log("Messages flushed\n");
        }
        
        append_to_log("About to call CloseWindow...\n");
        CloseWindow(folder_data->window);
        append_to_log("CloseWindow returned\n");
        
        folder_data->window = NULL;
        folder_data->window_open = FALSE;
    }
    
    /* Free gadgets */
    if (folder_data->glist != NULL)
    {
        append_to_log("Freeing gadgets...\n");
        FreeGadgets(folder_data->glist);
        folder_data->glist = NULL;
        folder_data->folder_list = NULL;
        folder_data->close_btn = NULL;
        append_to_log("Gadgets freed\n");
    }
    
    /* Free folder entries */
    free_folder_entries(folder_data);
    
    /* Free visual info */
    if (folder_data->visual_info != NULL)
    {
        append_to_log("Freeing visual info...\n");
        FreeVisualInfo(folder_data->visual_info);
        folder_data->visual_info = NULL;
        append_to_log("Visual info freed\n");
    }
    
    /* Note: We don't close system_font because we're using screen->Font directly */
    folder_data->system_font = NULL;
    
    append_to_log("=== Folder view window closed ===\n");
}

/*------------------------------------------------------------------------*/
/**
 * @brief Parse catalog and build folder hierarchy with tree structure
 */
/*------------------------------------------------------------------------*/
BOOL parse_catalog_and_build_tree(const char *catalog_path,
                                  struct iTidyFolderViewWindow *folder_data)
{
    BPTR file_handle;
    char line_buffer[512];
    BOOL success = TRUE;
    
    if (catalog_path == NULL || folder_data == NULL)
    {
        append_to_log("ERROR: Invalid parameters for parse_catalog_and_build_tree\n");
        return FALSE;
    }
    
    append_to_log("Opening catalog file: %s\n", catalog_path);
    
    file_handle = Open(catalog_path, MODE_OLDFILE);
    if (file_handle == 0)
    {
        append_to_log("ERROR: Cannot open catalog file: %s\n", catalog_path);
        return FALSE;
    }
    
    /* Read file line by line */
    while (FGets(file_handle, line_buffer, sizeof(line_buffer) - 1))
    {
        /* Remove trailing newline */
        UWORD len = strlen(line_buffer);
        if (len > 0 && line_buffer[len - 1] == '\n')
        {
            line_buffer[len - 1] = '\0';
        }
        
        /* Skip empty lines */
        if (strlen(line_buffer) == 0)
        {
            continue;
        }
        
        /* Process the line */
        if (!parse_catalog_callback(line_buffer, folder_data))
        {
            append_to_log("ERROR: Failed to process catalog line: %s\n", line_buffer);
            success = FALSE;
            break;
        }
    }
    
    Close(file_handle);
    append_to_log("Catalog file closed, success: %d\n", success);
    
    return success;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Free all folder entries in the list
 */
/*------------------------------------------------------------------------*/
void free_folder_entries(struct iTidyFolderViewWindow *folder_data)
{
    struct FolderEntry *entry;
    struct FolderEntry *next_entry;
    
    if (folder_data == NULL)
    {
        return;
    }
    
    append_to_log("Freeing folder entries...\n");
    
    entry = (struct FolderEntry *)folder_data->folder_entries.lh_Head;
    while (entry->node.ln_Succ != NULL)
    {
        next_entry = (struct FolderEntry *)entry->node.ln_Succ;
        Remove((struct Node *)entry);
        
        /* Free allocated strings */
        if (entry->path != NULL)
        {
            FreeVec(entry->path);
        }
        if (entry->display_text != NULL)
        {
            FreeVec(entry->display_text);
        }
        
        FreeVec(entry);
        entry = next_entry;
    }
    
    append_to_log("Folder entries freed\n");
}

/*------------------------------------------------------------------------*/
/* HELPER FUNCTIONS - Based on working test_simple_window.c approach     */
/*------------------------------------------------------------------------*/

/**
 * @brief Calculate the depth of a folder path by counting directory levels
 */
static UWORD calculate_folder_depth(const char *path)
{
    UWORD depth = 0;
    const char *p = path;
    BOOL after_colon = FALSE;
    
    if (path == NULL)
    {
        return 0;
    }
    
    /* Skip leading ".." references - they don't add to visual depth */
    while (strncmp(p, "../", 3) == 0)
    {
        p += 3;
    }
    
    /* Count directory separators */
    while (*p != '\0')
    {
        if (*p == ':')
        {
            after_colon = TRUE;
        }
        else if (*p == '/')
        {
            /* For Amiga paths, only count after colon */
            if (after_colon || strchr(path, ':') == NULL)
            {
                depth++;
            }
        }
        p++;
    }
    
    return depth;
}

/**
 * @brief Get the folder name from a full path
 */
static const char *get_folder_name(const char *path)
{
    const char *last_slash;
    const char *colon;
    
    if (path == NULL)
    {
        return "";
    }
    
    last_slash = strrchr(path, '/');
    colon = strchr(path, ':');
    
    if (last_slash != NULL)
    {
        /* Don't return empty string for trailing slash */
        if (*(last_slash + 1) != '\0')
        {
            return last_slash + 1;  /* Return part after last '/' */
        }
        else
        {
            /* Handle trailing slash - find previous component */
            const char *prev_slash = last_slash - 1;
            while (prev_slash >= path && *prev_slash != '/')
            {
                prev_slash--;
            }
            if (prev_slash >= path)
            {
                return prev_slash + 1;  /* Return component before trailing slash */
            }
        }
    }
    
    if (colon != NULL)
    {
        return path;            /* Return full path for root folders */
    }
    
    return path;
}

/**
 * @brief Format a folder entry with ASCII tree indentation
 */
static void format_folder_display(const char *path, UWORD depth, char *buffer, UWORD buffer_size)
{
    format_folder_display_with_size(path, depth, "", buffer, buffer_size);
}

/**
 * @brief Format a folder entry with ASCII tree indentation and size
 */
static void format_folder_display_with_size(const char *path, UWORD depth, const char *size_str, char *buffer, UWORD buffer_size)
{
    const char *folder_name = get_folder_name(path);
    char *p = buffer;
    UWORD remaining = buffer_size;
    UWORD i;
    
    if (buffer == NULL || buffer_size == 0)
    {
        return;
    }
    
    /* Add indentation - 3 characters per depth level */
    for (i = 0; i < depth && remaining > 3; i++)
    {
        *p++ = ':';     /* Vertical line */
        *p++ = ' ';     /* Space */
        *p++ = ' ';     /* Space */
        remaining -= 3;
    }
    
    /* Add branch indicator for non-root folders */
    if (depth > 0 && remaining > 3)
    {
        *p++ = ':';     /* Branch start */
        *p++ = '.';     /* Connector */
        *p++ = '.';     /* Connector */
        remaining -= 3;
    }
    
    /* Add folder name */
    UWORD name_len = strlen(folder_name);
    if (remaining > name_len + 1)
    {
        strcpy(p, folder_name);
        p += name_len;
        remaining -= name_len;
        
        /* Add size if provided */
        if (size_str != NULL && strlen(size_str) > 0 && remaining > strlen(size_str) + 4)
        {
            strcpy(p, " (");
            p += 2;
            strcpy(p, size_str);
            p += strlen(size_str);
            strcpy(p, ")");
            remaining -= (strlen(size_str) + 3);
        }
    }
    else
    {
        strncpy(p, folder_name, remaining - 1);
        buffer[buffer_size - 1] = '\0';
    }
}

/**
 * @brief Process a single line from the catalog file
 */
static BOOL parse_catalog_callback(const char *line, struct iTidyFolderViewWindow *folder_data)
{
    struct FolderEntry *entry;
    char *path_start;
    char *size_start;
    char *icons_start;
    char *path_copy;
    char size_str[32];
    char icons_str[16];
    char combined_str[64];
    UWORD path_len;
    UWORD depth;
    
    if (line == NULL || folder_data == NULL)
    {
        return FALSE;
    }
    
    /* Skip empty lines and header lines */
    path_len = strlen(line);
    if (path_len == 0 || strstr(line, "====") != NULL || 
        strstr(line, "iTidy") != NULL || strstr(line, "Run Number") != NULL ||
        strstr(line, "Session") != NULL || strstr(line, "LhA Path") != NULL ||
        strstr(line, "# Index") != NULL || strstr(line, "-----") != NULL ||
        strstr(line, "Total") != NULL)
    {
        return TRUE;  /* Skip header/separator lines */
    }
    
    /* Look for catalog entries with "Original Path" column */
    /* Format: "00001.lha  | 000/      | 11 KB   | 15    | PC:Workbench | 709x374+45+113 | 1" */
    /* Original Path is in the 5th column (after 4th pipe) */
    
    /* Find the 4th pipe to get to the Original Path column */
    path_start = strchr(line, '|');  /* 1st pipe */
    if (path_start != NULL)
    {
        path_start = strchr(path_start + 1, '|');  /* 2nd pipe */
    }
    if (path_start != NULL)
    {
        path_start = strchr(path_start + 1, '|');  /* 3rd pipe */
    }
    if (path_start != NULL)
    {
        path_start = strchr(path_start + 1, '|');  /* 4th pipe - now we're at the start of Original Path column */
    }
    
    if (path_start != NULL)
    {
        /* Skip past the '|' and any spaces */
        path_start++;
        while (*path_start == ' ' || *path_start == '\t')
        {
            path_start++;
        }
        
        /* Find the end of the Original Path column (next pipe or end of line) */
        char *path_end = strchr(path_start, '|');
        UWORD path_column_len;
        if (path_end != NULL)
        {
            path_column_len = path_end - path_start;
        }
        else
        {
            path_column_len = strlen(path_start);
        }
        
        /* Check if we have a valid path (contains '/' or ':') within the column */
        if ((strchr(path_start, '/') != NULL || strchr(path_start, ':') != NULL) &&
            path_column_len > 0)
        {
            /* Extract size from the third column and icons from the fourth column */
            char *pipe1, *pipe2, *pipe3, *pipe4;
            pipe1 = strchr(line, '|');
            if (pipe1 != NULL)
            {
                pipe2 = strchr(pipe1 + 1, '|');
                if (pipe2 != NULL)
                {
                    pipe3 = strchr(pipe2 + 1, '|');
                    if (pipe3 != NULL)
                    {
                        pipe4 = strchr(pipe3 + 1, '|');
                        
                        /* Extract size between pipe2 and pipe3 */
                        size_start = pipe2 + 1;
                        while (*size_start == ' ' || *size_start == '\t')
                        {
                            size_start++;
                        }
                        
                        /* Copy size string */
                        UWORD i = 0;
                        while (size_start < pipe3 && i < sizeof(size_str) - 1 &&
                               *size_start != ' ' && *size_start != '\t')
                        {
                            size_str[i++] = *size_start++;
                        }
                        
                        /* Add unit if present */
                        while (size_start < pipe3 && (*size_start == ' ' || *size_start == '\t'))
                        {
                            size_start++;
                        }
                        if (size_start < pipe3 && i < sizeof(size_str) - 3)
                        {
                            size_str[i++] = ' ';
                            while (size_start < pipe3 && i < sizeof(size_str) - 1 &&
                                   *size_start != ' ' && *size_start != '\t')
                            {
                                size_str[i++] = *size_start++;
                            }
                        }
                        size_str[i] = '\0';
                        
                        /* Extract icons count between pipe3 and pipe4 (if pipe4 exists) */
                        icons_str[0] = '\0';
                        if (pipe4 != NULL)
                        {
                            icons_start = pipe3 + 1;
                            while (*icons_start == ' ' || *icons_start == '\t')
                            {
                                icons_start++;
                            }
                            
                            /* Copy icons string */
                            i = 0;
                            while (icons_start < pipe4 && i < sizeof(icons_str) - 1 &&
                                   *icons_start != ' ' && *icons_start != '\t')
                            {
                                icons_str[i++] = *icons_start++;
                            }
                            icons_str[i] = '\0';
                        }
                        
                        /* Combine size and icons into format "size / icons icons" */
                        if (icons_str[0] != '\0')
                        {
                            sprintf(combined_str, "%s / %s icons", size_str, icons_str);
                        }
                        else
                        {
                            strcpy(combined_str, size_str);
                        }
                    }
                }
            }
            
            /* Allocate folder entry */
            entry = (struct FolderEntry *)whd_malloc(sizeof(struct FolderEntry));
            if (entry == NULL)
            {
                append_to_log("ERROR: Failed to allocate folder entry\n");
                return FALSE;
            }
            memset(entry, 0, sizeof(struct FolderEntry));
            
            /* Copy and clean up the path (only the Original Path column content) */
            /* Create a temporary buffer for the path column only */
            char temp_path[256];
            UWORD copy_len = path_column_len;
            if (copy_len >= sizeof(temp_path))
            {
                copy_len = sizeof(temp_path) - 1;
            }
            
            strncpy(temp_path, path_start, copy_len);
            temp_path[copy_len] = '\0';
            
            /* Remove trailing whitespace from the column */
            while (copy_len > 0 && (temp_path[copy_len - 1] == ' ' || 
                                   temp_path[copy_len - 1] == '\t' ||
                                   temp_path[copy_len - 1] == '\n' ||
                                   temp_path[copy_len - 1] == '\r'))
            {
                temp_path[copy_len - 1] = '\0';
                copy_len--;
            }
            
            /* Allocate and copy the cleaned path */
            entry->path = (char *)whd_malloc(copy_len + 1);
            if (entry->path == NULL)
            {
                FreeVec(entry);
                return FALSE;
            }
            memset(entry->path, 0, copy_len + 1);
            
            strcpy(entry->path, temp_path);
            path_len = strlen(entry->path);
            
            /* Calculate depth based on directory separators */
            depth = calculate_folder_depth(entry->path);
            entry->depth = depth;
            
            /* Format display text with tree lines and size */
            entry->display_text = (char *)whd_malloc(300);  /* Larger buffer for size info */
            if (entry->display_text == NULL)
            {
                whd_free(entry->path);
                whd_free(entry);
                return FALSE;
            }
            memset(entry->display_text, 0, 300);
            
            format_folder_display_with_size(entry->path, depth, combined_str, entry->display_text, 300);
            
            /* Set up node for ListView */
            entry->node.ln_Name = entry->display_text;
            
            /* Add to list */
            AddTail(&folder_data->folder_entries, (struct Node *)entry);
            
            append_to_log("Added folder: %s (depth %u, original: %s, size/icons: %s)\n", 
                         entry->display_text, depth, entry->path, combined_str);
        }
    }
    
    return TRUE;
}