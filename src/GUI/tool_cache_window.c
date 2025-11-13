/*
 * tool_cache_window.c - iTidy Tool Cache Window Implementation
 * Displays default tool validation cache with filtering and details
 * GadTools-based window for Workbench 2.0+
 */

#include "platform/platform.h"
#include "tool_cache_window.h"
#include "../icon_types.h"
#include "../itidy_types.h"
#include "../Settings/IControlPrefs.h"
#include "../writeLog.h"
#include "../layout_processor.h"

#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <graphics/text.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <string.h>
#include <stdio.h>

/* External graphics base for default font */
extern struct GfxBase *GfxBase;

/*------------------------------------------------------------------------*/
/* Layout Configuration                                                   */
/*------------------------------------------------------------------------*/
#define TOOL_NAME_COLUMN_WIDTH  40  /* Maximum characters for tool name column */

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static void populate_tool_list(struct iTidyToolCacheWindow *tool_data);
static void populate_details_panel(struct iTidyToolCacheWindow *tool_data);
static void truncate_tool_name_middle(const char *tool_name, char *output, int max_length);

/*------------------------------------------------------------------------*/
/* Truncate Tool Name in Middle                                          */
/*------------------------------------------------------------------------*/
/**
 * @brief Truncate a tool name in the middle with "..." if too long
 * 
 * For Amiga paths like "Workbench:Programs/Wordworth7/Wordworth", this will
 * truncate to show the device/start and the program name, e.g.:
 * "Workbench:...Wordworth" (22 chars max)
 * 
 * @param tool_name Original tool name/path
 * @param output Buffer to store truncated result (must be at least max_length+1)
 * @param max_length Maximum length for the output string
 */
static void truncate_tool_name_middle(const char *tool_name, char *output, int max_length)
{
    int len;
    int left_part_len;
    int right_part_len;
    int i;
    const char *last_slash;
    const char *last_colon;
    
    if (tool_name == NULL || output == NULL || max_length < 10)
    {
        if (output != NULL && max_length > 0)
            output[0] = '\0';
        return;
    }
    
    len = strlen(tool_name);
    
    /* If it fits, just copy it */
    if (len <= max_length)
    {
        strcpy(output, tool_name);
        return;
    }
    
    /* Find the last slash or colon to identify the program name */
    last_slash = strrchr(tool_name, '/');
    last_colon = strrchr(tool_name, ':');
    
    /* Determine which separator is last */
    if (last_slash == NULL && last_colon == NULL)
    {
        /* No path separators - just truncate in middle */
        left_part_len = (max_length - 3) / 2;
        right_part_len = max_length - 3 - left_part_len;
        
        strncpy(output, tool_name, left_part_len);
        output[left_part_len] = '\0';
        strcat(output, "...");
        strncat(output, tool_name + len - right_part_len, right_part_len);
    }
    else
    {
        /* We have path separators - try to show device and program name */
        const char *separator = (last_slash > last_colon) ? last_slash : last_colon;
        int program_name_len = strlen(separator); /* Includes the separator */
        
        /* Calculate how much space we have for the left part */
        /* Format: "device:...program" or "device:.../program" */
        left_part_len = max_length - 3 - program_name_len;
        
        if (left_part_len < 5)
        {
            /* Not enough space to show device - just show end with ellipsis */
            strcpy(output, "...");
            strncat(output, separator, max_length - 3);
            output[max_length] = '\0';
        }
        else
        {
            /* Show device/start + ... + program name */
            strncpy(output, tool_name, left_part_len);
            output[left_part_len] = '\0';
            strcat(output, "...");
            strcat(output, separator);
        }
    }
    
    /* Ensure null termination */
    output[max_length] = '\0';
}

/*------------------------------------------------------------------------*/
/* Build Tool Cache Display List                                         */
/*------------------------------------------------------------------------*/
BOOL build_tool_cache_display_list(struct iTidyToolCacheWindow *tool_data)
{
    int i;
    struct ToolCacheDisplayEntry *entry;
    char buffer[256];
    
    if (tool_data == NULL)
        return FALSE;
    
    /* Clear any existing entries */
    free_tool_cache_entries(tool_data);
    
    /* Initialize counts */
    tool_data->total_count = 0;
    tool_data->valid_count = 0;
    tool_data->missing_count = 0;
    
    /* Check if cache exists */
    if (g_ToolCache == NULL || g_ToolCacheCount == 0)
    {
        append_to_log("build_tool_cache_display_list: No tools in cache\n");
        return TRUE;  /* Empty list is valid */
    }
    
    append_to_log("build_tool_cache_display_list: Processing %d cached tools\n", g_ToolCacheCount);
    
    /* Build display entries from global cache */
    for (i = 0; i < g_ToolCacheCount; i++)
    {
        char truncated_name[TOOL_NAME_COLUMN_WIDTH + 1];  /* Buffer for truncated tool name */
        
        /* Allocate entry */
        entry = (struct ToolCacheDisplayEntry *)whd_malloc(sizeof(struct ToolCacheDisplayEntry));
        if (entry == NULL)
        {
            append_to_log("ERROR: Failed to allocate display entry\n");
            free_tool_cache_entries(tool_data);
            return FALSE;
        }
        memset(entry, 0, sizeof(struct ToolCacheDisplayEntry));
        
        /* Copy tool data */
        entry->tool_name = g_ToolCache[i].toolName;  /* Point to cache data - don't duplicate */
        entry->exists = g_ToolCache[i].exists;
        entry->hit_count = g_ToolCache[i].hitCount;
        entry->full_path = g_ToolCache[i].fullPath;  /* Point to cache data */
        entry->version = g_ToolCache[i].versionString;  /* Point to cache data */
        
        /* Truncate tool name if needed (max TOOL_NAME_COLUMN_WIDTH chars to fit in column) */
        truncate_tool_name_middle(
            entry->tool_name ? entry->tool_name : "(unknown)",
            truncated_name,
            TOOL_NAME_COLUMN_WIDTH
        );
        
        /* Log if truncation occurred */
        if (entry->tool_name && strlen(entry->tool_name) > TOOL_NAME_COLUMN_WIDTH)
        {
            append_to_log("  Truncated: '%s' -> '%s'\n", entry->tool_name, truncated_name);
        }
        
        /* Format display text: "Tool Name             | Hits | Status  " */
        /* Use fixed-width format for column alignment */
        /* Add 1 to hit_count for display (user-friendly numbering starting from 1) */
        sprintf(buffer, "%-*s| %4d | %s",
            TOOL_NAME_COLUMN_WIDTH,
            truncated_name,
            entry->hit_count + 1,
            entry->exists ? "EXISTS " : "MISSING");
        
        /* Allocate and copy display text */
        entry->display_text = (char *)whd_malloc(strlen(buffer) + 1);
        if (entry->display_text == NULL)
        {
            FreeVec(entry);
            free_tool_cache_entries(tool_data);
            return FALSE;
        }
        memset(entry->display_text, 0, strlen(buffer) + 1);
        strcpy(entry->display_text, buffer);
        
        /* Set node name for GadTools listview */
        entry->node.ln_Name = entry->display_text;
        
        /* Add to list */
        AddTail(&tool_data->tool_entries, (struct Node *)entry);
        
        /* Update counts */
        tool_data->total_count++;
        if (entry->exists)
            tool_data->valid_count++;
        else
            tool_data->missing_count++;
    }
    
    /* Build summary text */
    sprintf(tool_data->summary_text, "Total Tools: %lu  |  Valid: %lu  |  Missing: %lu",
        tool_data->total_count, tool_data->valid_count, tool_data->missing_count);
    
    append_to_log("build_tool_cache_display_list: Created %lu entries\n", tool_data->total_count);
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Apply Filter                                                           */
/*------------------------------------------------------------------------*/
void apply_tool_filter(struct iTidyToolCacheWindow *tool_data)
{
    struct ToolCacheDisplayEntry *entry;
    struct Node *node;
    ULONG count = 0;
    
    if (tool_data == NULL)
        return;
    
    /* Clear filtered list by rebuilding it */
    /* NOTE: The filtered_entries list uses the filter_node member of each entry.
       This allows the same entry to appear in both tool_entries (via node) 
       and filtered_entries (via filter_node) simultaneously. */
    NewList(&tool_data->filtered_entries);
    
    /* Build filtered list based on current filter */
    for (node = tool_data->tool_entries.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ)
    {
        entry = (struct ToolCacheDisplayEntry *)node;
        
        /* Apply filter and add to filtered list using filter_node */
        switch (tool_data->current_filter)
        {
            case TOOL_FILTER_ALL:
                /* Add all entries using filter_node */
                entry->filter_node.ln_Name = entry->display_text;  /* Point to same display text */
                AddTail(&tool_data->filtered_entries, &entry->filter_node);
                count++;
                break;
                
            case TOOL_FILTER_VALID:
                /* Only add tools that exist */
                if (entry->exists)
                {
                    entry->filter_node.ln_Name = entry->display_text;
                    AddTail(&tool_data->filtered_entries, &entry->filter_node);
                    count++;
                }
                break;
                
            case TOOL_FILTER_MISSING:
                /* Only add missing tools */
                if (!entry->exists)
                {
                    entry->filter_node.ln_Name = entry->display_text;
                    AddTail(&tool_data->filtered_entries, &entry->filter_node);
                    count++;
                }
                break;
        }
    }
    
    append_to_log("apply_tool_filter: Filter=%d, Result count=%lu\n",
        tool_data->current_filter, count);
}

/*------------------------------------------------------------------------*/
/* Update Details Panel                                                   */
/*------------------------------------------------------------------------*/
void update_tool_details(struct iTidyToolCacheWindow *tool_data, LONG selected_index)
{
    struct ToolCacheDisplayEntry *entry;
    struct Node *node;
    struct Node *detail_node;
    LONG index = 0;
    char buffer[512];
    int i;
    
    if (tool_data == NULL)
        return;
    
    log_info(LOG_GUI, "[TOOL_CACHE] update_tool_details() called with selected_index=%ld\n", selected_index);
    
    /* Clear details list */
    while ((detail_node = RemHead(&tool_data->details_list)) != NULL)
    {
        if (detail_node->ln_Name != NULL)
            FreeVec(detail_node->ln_Name);
        FreeVec(detail_node);
    }
    
    /* Store selected index */
    tool_data->selected_index = selected_index;
    
    /* If nothing selected, show empty details */
    if (selected_index < 0)
    {
        log_info(LOG_GUI, "[TOOL_CACHE] No tool selected (index < 0), clearing details\n");
        populate_details_panel(tool_data);
        return;
    }
    
    /* Find selected entry in filtered list */
    for (node = tool_data->filtered_entries.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ, index++)
    {
        if (index == selected_index)
        {
            /* Calculate entry pointer from filter_node offset */
            /* filter_node is the second member of ToolCacheDisplayEntry, after node */
            entry = (struct ToolCacheDisplayEntry *)((char *)node - sizeof(struct Node));
            
            log_info(LOG_GUI, "[TOOL_CACHE] Found selected entry: tool_name='%s'\n", 
                         entry->tool_name ? entry->tool_name : "(null)");
            
            /* Find the corresponding cache entry to get file references */
            for (i = 0; i < g_ToolCacheCount; i++)
            {
                if (g_ToolCache[i].toolName && entry->tool_name && 
                    strcmp(g_ToolCache[i].toolName, entry->tool_name) == 0)
                {
                    int j;
                    
                    log_info(LOG_GUI, "[TOOL_CACHE] Matched cache entry %d: fileCount=%d\n", 
                                 i, g_ToolCache[i].fileCount);
                    
                    /* Add tool name on first line */
                    sprintf(buffer, "Tool:   %s",
                        entry->tool_name ? entry->tool_name : "(unknown)");
                    
                    detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                    if (detail_node)
                    {
                        memset(detail_node, 0, sizeof(struct Node));
                        detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                        if (detail_node->ln_Name)
                        {
                            memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                            strcpy(detail_node->ln_Name, buffer);
                            AddTail(&tool_data->details_list, detail_node);
                        }
                        else
                        {
                            FreeVec(detail_node);
                        }
                    }
                    
                    /* Add status and version on second line */
                    sprintf(buffer, "Status: %s  |  Version: %s",
                        entry->exists ? "EXISTS" : "MISSING",
                        entry->version ? entry->version : "(no version)");
                    
                    detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                    if (detail_node)
                    {
                        memset(detail_node, 0, sizeof(struct Node));
                        detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                        if (detail_node->ln_Name)
                        {
                            memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                            strcpy(detail_node->ln_Name, buffer);
                            AddTail(&tool_data->details_list, detail_node);
                        }
                        else
                        {
                            FreeVec(detail_node);
                        }
                    }
                    
                    /* Add separator using simple dashes (Amiga doesn't support Unicode box chars) */
                    sprintf(buffer, "--------------------------------------------------------------------");
                    detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                    if (detail_node)
                    {
                        memset(detail_node, 0, sizeof(struct Node));
                        detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                        if (detail_node->ln_Name)
                        {
                            memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                            strcpy(detail_node->ln_Name, buffer);
                            AddTail(&tool_data->details_list, detail_node);
                        }
                        else
                        {
                            FreeVec(detail_node);
                        }
                    }
                    
                    /* Add file references */
                    if (g_ToolCache[i].fileCount > 0 && g_ToolCache[i].referencingFiles)
                    {
                        for (j = 0; j < g_ToolCache[i].fileCount; j++)
                        {
                            if (g_ToolCache[i].referencingFiles[j])
                            {
                                /* Truncate long paths if needed */
                                char truncated_path[80];
                                truncate_tool_name_middle(g_ToolCache[i].referencingFiles[j], 
                                                         truncated_path, 70);
                                
                                sprintf(buffer, "%s", truncated_path);
                                
                                detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                                if (detail_node)
                                {
                                    memset(detail_node, 0, sizeof(struct Node));
                                    detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                                    if (detail_node->ln_Name)
                                    {
                                        memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                                        strcpy(detail_node->ln_Name, buffer);
                                        AddTail(&tool_data->details_list, detail_node);
                                    }
                                    else
                                    {
                                        FreeVec(detail_node);
                                    }
                                }
                            }
                        }
                        
                        /* Add footer if truncated */
                        if (g_ToolCache[i].fileCount >= TOOL_CACHE_MAX_FILES_PER_TOOL)
                        {
                            sprintf(buffer, "(showing first %d files, more may exist)", 
                                   TOOL_CACHE_MAX_FILES_PER_TOOL);
                            detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                            if (detail_node)
                            {
                                memset(detail_node, 0, sizeof(struct Node));
                                detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                                if (detail_node->ln_Name)
                                {
                                    memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                                    strcpy(detail_node->ln_Name, buffer);
                                    AddTail(&tool_data->details_list, detail_node);
                                }
                                else
                                {
                                    FreeVec(detail_node);
                                }
                            }
                        }
                    }
                    else
                    {
                        /* No files found */
                        sprintf(buffer, "(no files using this tool)");
                        detail_node = (struct Node *)whd_malloc(sizeof(struct Node));
                        if (detail_node)
                        {
                            memset(detail_node, 0, sizeof(struct Node));
                            detail_node->ln_Name = (char *)whd_malloc(strlen(buffer) + 1);
                            if (detail_node->ln_Name)
                            {
                                memset(detail_node->ln_Name, 0, strlen(buffer) + 1);
                                strcpy(detail_node->ln_Name, buffer);
                                AddTail(&tool_data->details_list, detail_node);
                            }
                            else
                            {
                                FreeVec(detail_node);
                            }
                        }
                    }
                    
                    break;  /* Found the tool, exit loop */
                }
            }
            
            break;  /* Found the selected entry, exit loop */
        }
    }
    
    /* Count details list items for logging */
    {
        int detail_count = 0;
        struct Node *count_node;
        for (count_node = tool_data->details_list.lh_Head; 
             count_node->ln_Succ != NULL; 
             count_node = count_node->ln_Succ)
        {
            detail_count++;
        }
        log_info(LOG_GUI, "[TOOL_CACHE] Calling populate_details_panel() with %d items in details_list\n", 
                     detail_count);
    }
    
    /* Update details listview */
    populate_details_panel(tool_data);
}

/*------------------------------------------------------------------------*/
/* Free Tool Cache Entries                                               */
/*------------------------------------------------------------------------*/
void free_tool_cache_entries(struct iTidyToolCacheWindow *tool_data)
{
    struct ToolCacheDisplayEntry *entry;
    struct Node *node;
    
    if (tool_data == NULL)
        return;
    
    /* Free tool entries */
    while ((node = RemHead(&tool_data->tool_entries)) != NULL)
    {
        entry = (struct ToolCacheDisplayEntry *)node;
        if (entry->display_text != NULL)
            FreeVec(entry->display_text);
        /* Note: Don't free tool_name, full_path, version - they point to g_ToolCache */
        FreeVec(entry);
    }
    
    /* Free details list */
    while ((node = RemHead(&tool_data->details_list)) != NULL)
    {
        if (node->ln_Name != NULL)
            FreeVec(node->ln_Name);
        FreeVec(node);
    }
}

/*------------------------------------------------------------------------*/
/* Populate Tool List                                                     */
/*------------------------------------------------------------------------*/
static void populate_tool_list(struct iTidyToolCacheWindow *tool_data)
{
    if (tool_data == NULL || tool_data->window == NULL)
        return;
    
    /* Detach list from gadget (mandatory for safe update) */
    GT_SetGadgetAttrs(tool_data->tool_list, tool_data->window, NULL,
        GTLV_Labels, ~0,
        TAG_END);
    
    /* Attach filtered list */
    GT_SetGadgetAttrs(tool_data->tool_list, tool_data->window, NULL,
        GTLV_Labels, &tool_data->filtered_entries,
        GTLV_Selected, ~0,  /* Clear selection */
        TAG_END);
    
    /* Refresh window */
    GT_RefreshWindow(tool_data->window, NULL);
}

/*------------------------------------------------------------------------*/
/* Populate Details Panel                                                 */
/*------------------------------------------------------------------------*/
static void populate_details_panel(struct iTidyToolCacheWindow *tool_data)
{
    if (tool_data == NULL || tool_data->window == NULL || tool_data->details_listview == NULL)
    {
        log_info(LOG_GUI, "[TOOL_CACHE] populate_details_panel() FAILED: NULL pointer (tool_data=%p, window=%p, details_listview=%p)\n",
                     (void*)tool_data, 
                     (void*)(tool_data ? tool_data->window : NULL),
                     (void*)(tool_data ? tool_data->details_listview : NULL));
        return;
    }
    
    log_info(LOG_GUI, "[TOOL_CACHE] populate_details_panel() called - detaching list\n");
    
    /* Detach list from gadget (mandatory for safe update) */
    GT_SetGadgetAttrs(tool_data->details_listview, tool_data->window, NULL,
        GTLV_Labels, ~0,
        TAG_END);
    
    log_info(LOG_GUI, "[TOOL_CACHE] populate_details_panel() - attaching new list\n");
    
    /* Attach details list */
    GT_SetGadgetAttrs(tool_data->details_listview, tool_data->window, NULL,
        GTLV_Labels, &tool_data->details_list,
        GTLV_Top, 0,  /* Scroll to top */
        TAG_END);
    
    log_info(LOG_GUI, "[TOOL_CACHE] populate_details_panel() - refreshing window\n");
    
    /* Refresh just the details listview gadget, not the entire window */
    RefreshGList(tool_data->details_listview, tool_data->window, NULL, 1);
    
    log_info(LOG_GUI, "[TOOL_CACHE] populate_details_panel() COMPLETE\n");
}

/*------------------------------------------------------------------------*/
/* Open Tool Cache Window                                                 */
/*------------------------------------------------------------------------*/
BOOL open_tool_cache_window(struct iTidyToolCacheWindow *tool_data)
{
    struct NewGadget ng;
    struct Gadget *gad;
    struct DrawInfo *draw_info = NULL;
    struct TextFont *font;
    struct RastPort temp_rp;
    UWORD font_width, font_height;
    UWORD button_height, listview_line_height;
    UWORD current_x, current_y;
    UWORD window_width, window_height;
    UWORD listview_width, listview_height, actual_listview_height;
    UWORD details_listview_height;
    UWORD button_width, equal_button_width;
    UWORD reference_width, precalc_max_right;
    UWORD max_btn_text_width, temp_width;
    BOOL using_system_font = FALSE;
    
    if (tool_data == NULL)
        return FALSE;
    
    /* Initialize structure */
    memset(tool_data, 0, sizeof(struct iTidyToolCacheWindow));
    NewList(&tool_data->tool_entries);
    NewList(&tool_data->filtered_entries);
    NewList(&tool_data->details_list);
    tool_data->current_filter = TOOL_FILTER_ALL;
    tool_data->selected_index = -1;
    
    /* Lock Workbench screen */
    tool_data->screen = LockPubScreen(NULL);
    if (tool_data->screen == NULL)
    {
        append_to_log("ERROR: Could not lock Workbench screen\n");
        return FALSE;
    }
    
    /* Get DrawInfo for fonts and pens */
    draw_info = GetScreenDrawInfo(tool_data->screen);
    if (draw_info == NULL)
    {
        append_to_log("ERROR: Could not get DrawInfo\n");
        UnlockPubScreen(NULL, tool_data->screen);
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
        tool_data->system_font_attr.ta_Name = (STRPTR)GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name;
        tool_data->system_font_attr.ta_YSize = GfxBase->DefaultFont->tf_YSize;
        tool_data->system_font_attr.ta_Style = FS_NORMAL;
        tool_data->system_font_attr.ta_Flags = 0;
        
        tool_data->system_font = OpenFont(&tool_data->system_font_attr);
        if (tool_data->system_font != NULL)
        {
            font = tool_data->system_font;
            font_width = font->tf_XSize;
            font_height = font->tf_YSize;
            using_system_font = TRUE;
            append_to_log("Using system font: %s %d\n", 
                tool_data->system_font_attr.ta_Name,
                tool_data->system_font_attr.ta_YSize);
        }
        else
        {
            append_to_log("WARNING: Could not open system font, using screen font\n");
        }
    }
    
    /* Initialize temp RastPort for TextLength measurements */
    InitRastPort(&temp_rp);
    SetFont(&temp_rp, font);
    
    /* Calculate gadget heights */
    button_height = font_height + 6;
    listview_line_height = font_height + 2;
    
    /*--------------------------------------------------------------------*/
    /* PRE-CALCULATE LAYOUT DIMENSIONS                                   */
    /*--------------------------------------------------------------------*/
    reference_width = font_width * TOOL_WINDOW_WIDTH_CHARS;
    
    current_x = TOOL_WINDOW_MARGIN_LEFT + prefsIControl.currentLeftBarWidth;
    current_y = TOOL_WINDOW_MARGIN_TOP + prefsIControl.currentWindowBarHeight;
    
    precalc_max_right = current_x + reference_width;
    
    /* Calculate listview dimensions */
    listview_width = reference_width;
    listview_height = listview_line_height * 12;  /* Show 12 tools */
    
    /* Calculate details panel height */
    details_listview_height = listview_line_height * 5;  /* 5 detail lines */
    
    /* Pre-calculate equal-width buttons (3 filter buttons + rebuild + close = 5 buttons) */
    UWORD button_count = 5;
    
    /* Find maximum button text width */
    max_btn_text_width = TextLength(&temp_rp, "Show Missing Only", 17);
    temp_width = TextLength(&temp_rp, "Show Valid Only", 15);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    temp_width = TextLength(&temp_rp, "Rebuild Cache", 13);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    temp_width = TextLength(&temp_rp, "Show All", 8);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    temp_width = TextLength(&temp_rp, "Close", 5);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    
    /* Calculate equal button width */
    equal_button_width = (reference_width - ((button_count - 1) * TOOL_WINDOW_SPACE_X)) / button_count;
    if (equal_button_width < max_btn_text_width + TOOL_WINDOW_BUTTON_PADDING)
        equal_button_width = max_btn_text_width + TOOL_WINDOW_BUTTON_PADDING;
    
    append_to_log("=== PRE-CALCULATED LAYOUT ===\n");
    append_to_log("Reference width: %d, max_right: %d\n", reference_width, precalc_max_right);
    append_to_log("Equal button width: %d\n", equal_button_width);
    
    /* Get visual info */
    tool_data->visual_info = GetVisualInfo(tool_data->screen, TAG_END);
    if (tool_data->visual_info == NULL)
    {
        append_to_log("ERROR: Could not get visual info\n");
        if (using_system_font && tool_data->system_font != NULL)
            CloseFont(tool_data->system_font);
        FreeScreenDrawInfo(tool_data->screen, draw_info);
        UnlockPubScreen(NULL, tool_data->screen);
        return FALSE;
    }
    
    /* Create gadgets */
    gad = CreateContext(&tool_data->glist);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create gadget context\n");
        if (using_system_font && tool_data->system_font != NULL)
            CloseFont(tool_data->system_font);
        FreeVisualInfo(tool_data->visual_info);
        FreeScreenDrawInfo(tool_data->screen, draw_info);
        UnlockPubScreen(NULL, tool_data->screen);
        return FALSE;
    }
    
    /* Set text attr for all gadgets if using system font */
    ng.ng_TextAttr = using_system_font ? &tool_data->system_font_attr : tool_data->screen->Font;
    ng.ng_VisualInfo = tool_data->visual_info;
    
    /*--------------------------------------------------------------------*/
    /* MAIN TOOL LISTVIEW                                                */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = listview_width;
    ng.ng_Height = listview_height;
    ng.ng_GadgetText = "";  /* Empty label - we'll draw summary separately */
    ng.ng_GadgetID = GID_TOOL_LIST;
    ng.ng_Flags = PLACETEXT_ABOVE;
    
    tool_data->tool_list = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, NULL,  /* Will populate after building list */
        GTLV_ShowSelected, NULL,
        TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create tool listview\n");
        goto cleanup_error;
    }
    
    /* Get actual listview height */
    actual_listview_height = gad->Height;
    current_y = gad->TopEdge + actual_listview_height + TOOL_WINDOW_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* DETAILS PANEL LISTVIEW                                            */
    /*--------------------------------------------------------------------*/
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = listview_width;
    ng.ng_Height = details_listview_height;
    ng.ng_GadgetText = "";
    ng.ng_GadgetID = GID_TOOL_LIST + 100;  /* Non-interactive, just for display */
    ng.ng_Flags = PLACETEXT_ABOVE;
    
    tool_data->details_listview = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, &tool_data->details_list,
        GTLV_ReadOnly, TRUE,
        TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create details listview\n");
        goto cleanup_error;
    }
    
    current_y = gad->TopEdge + gad->Height + TOOL_WINDOW_SPACE_Y;
    
    /*--------------------------------------------------------------------*/
    /* FILTER AND CLOSE BUTTONS (4 equal-width buttons)                  */
    /*--------------------------------------------------------------------*/
    
    /* Show All button */
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Show All";
    ng.ng_GadgetID = GID_TOOL_FILTER_ALL;
    ng.ng_Flags = PLACETEXT_IN;
    
    tool_data->filter_all_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Show All button\n");
        goto cleanup_error;
    }
    
    /* Show Valid Only button */
    ng.ng_LeftEdge = current_x + equal_button_width + TOOL_WINDOW_SPACE_X;
    ng.ng_Width = equal_button_width;
    ng.ng_GadgetText = "Show Valid Only";
    ng.ng_GadgetID = GID_TOOL_FILTER_VALID;
    
    tool_data->filter_valid_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Show Valid button\n");
        goto cleanup_error;
    }
    
    /* Show Missing Only button */
    ng.ng_LeftEdge = current_x + (2 * equal_button_width) + (2 * TOOL_WINDOW_SPACE_X);
    ng.ng_Width = equal_button_width;
    ng.ng_GadgetText = "Show Missing Only";
    ng.ng_GadgetID = GID_TOOL_FILTER_MISSING;
    
    tool_data->filter_missing_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Show Missing button\n");
        goto cleanup_error;
    }
    
    /* Rebuild Cache button */
    ng.ng_LeftEdge = current_x + (3 * equal_button_width) + (3 * TOOL_WINDOW_SPACE_X);
    ng.ng_Width = equal_button_width;
    ng.ng_GadgetText = "Rebuild Cache";
    ng.ng_GadgetID = GID_TOOL_REBUILD_CACHE;
    
    tool_data->rebuild_cache_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Rebuild Cache button\n");
        goto cleanup_error;
    }
    
    /* Close button */
    ng.ng_LeftEdge = current_x + (4 * equal_button_width) + (4 * TOOL_WINDOW_SPACE_X);
    ng.ng_Width = equal_button_width;
    ng.ng_GadgetText = "Close";
    ng.ng_GadgetID = GID_TOOL_CLOSE_BTN;
    
    tool_data->close_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (gad == NULL)
    {
        append_to_log("ERROR: Could not create Close button\n");
        goto cleanup_error;
    }
    
    /* Calculate final window size */
    window_width = precalc_max_right + TOOL_WINDOW_MARGIN_RIGHT;
    window_height = current_y + button_height + TOOL_WINDOW_MARGIN_BOTTOM;
    
    /* Set window title */
    strcpy(tool_data->window_title, "iTidy - Default Tool Analysis");
    
    /* Open window */
    tool_data->window = OpenWindowTags(NULL,
        WA_Left, (tool_data->screen->Width - window_width) / 2,
        WA_Top, (tool_data->screen->Height - window_height) / 2,
        WA_Width, window_width,
        WA_Height, window_height,
        WA_Title, tool_data->window_title,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, tool_data->screen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN,
        WA_Gadgets, tool_data->glist,
        TAG_END);
    
    if (tool_data->window == NULL)
    {
        append_to_log("ERROR: Could not open tool cache window\n");
        goto cleanup_error;
    }
    
    /* Free DrawInfo (no longer needed) */
    FreeScreenDrawInfo(tool_data->screen, draw_info);
    draw_info = NULL;
    
    /* Build display list from global cache */
    if (!build_tool_cache_display_list(tool_data))
    {
        append_to_log("ERROR: Failed to build tool cache display list\n");
        close_tool_cache_window(tool_data);
        return FALSE;
    }
    
    /* Apply initial filter (show all) */
    apply_tool_filter(tool_data);
    
    /* Populate listview */
    populate_tool_list(tool_data);
    
    tool_data->window_open = TRUE;
    append_to_log("Tool cache window opened successfully\n");
    
    return TRUE;
    
cleanup_error:
    if (draw_info != NULL)
        FreeScreenDrawInfo(tool_data->screen, draw_info);
    if (tool_data->glist != NULL)
        FreeGadgets(tool_data->glist);
    if (tool_data->visual_info != NULL)
        FreeVisualInfo(tool_data->visual_info);
    if (using_system_font && tool_data->system_font != NULL)
        CloseFont(tool_data->system_font);
    if (tool_data->screen != NULL)
        UnlockPubScreen(NULL, tool_data->screen);
    
    return FALSE;
}

/*------------------------------------------------------------------------*/
/* Close Tool Cache Window                                                */
/*------------------------------------------------------------------------*/
void close_tool_cache_window(struct iTidyToolCacheWindow *tool_data)
{
    if (tool_data == NULL)
        return;
    
    /* Detach lists from listviews FIRST (critical for safe cleanup) */
    if (tool_data->window != NULL)
    {
        if (tool_data->tool_list != NULL)
        {
            GT_SetGadgetAttrs(tool_data->tool_list, tool_data->window, NULL,
                GTLV_Labels, ~0,
                TAG_END);
        }
    }
    
    /* Free lists */
    free_tool_cache_entries(tool_data);
    
    /* Close window */
    if (tool_data->window != NULL)
    {
        CloseWindow(tool_data->window);
        tool_data->window = NULL;
    }
    
    /* Free gadgets */
    if (tool_data->glist != NULL)
    {
        FreeGadgets(tool_data->glist);
        tool_data->glist = NULL;
    }
    
    /* Free visual info */
    if (tool_data->visual_info != NULL)
    {
        FreeVisualInfo(tool_data->visual_info);
        tool_data->visual_info = NULL;
    }
    
    /* Close system font */
    if (tool_data->system_font != NULL)
    {
        CloseFont(tool_data->system_font);
        tool_data->system_font = NULL;
    }
    
    /* Unlock screen */
    if (tool_data->screen != NULL)
    {
        UnlockPubScreen(NULL, tool_data->screen);
        tool_data->screen = NULL;
    }
    
    tool_data->window_open = FALSE;
    append_to_log("Tool cache window closed\n");
}

/*------------------------------------------------------------------------*/
/* Handle Tool Cache Window Events                                        */
/*------------------------------------------------------------------------*/
BOOL handle_tool_cache_window_events(struct iTidyToolCacheWindow *tool_data)
{
    struct IntuiMessage *msg;
    ULONG msg_class;
    UWORD msg_code;
    UWORD gadget_id;
    struct Gadget *gadget;
    
    if (tool_data == NULL || tool_data->window == NULL)
        return FALSE;
    
    while ((msg = GT_GetIMsg(tool_data->window->UserPort)) != NULL)
    {
        msg_class = msg->Class;
        msg_code = msg->Code;
        gadget = (struct Gadget *)msg->IAddress;
        gadget_id = (gadget != NULL) ? gadget->GadgetID : 0;
        
        GT_ReplyIMsg(msg);
        
        switch (msg_class)
        {
            case IDCMP_CLOSEWINDOW:
                return FALSE;  /* Close window */
                
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(tool_data->window);
                GT_EndRefresh(tool_data->window, TRUE);
                break;
                
            case IDCMP_GADGETUP:
                switch (gadget_id)
                {
                    case GID_TOOL_LIST:
                    {
                        /* Tool selected - update details */
                        LONG selected = ~0;
                        GT_GetGadgetAttrs(tool_data->tool_list, tool_data->window, NULL,
                            GTLV_Selected, &selected,
                            TAG_END);
                        log_info(LOG_GUI, "[TOOL_CACHE] Tool selected: index=%ld\n", selected);
                        update_tool_details(tool_data, selected);
                        break;
                    }
                    
                    case GID_TOOL_FILTER_ALL:
                        tool_data->current_filter = TOOL_FILTER_ALL;
                        apply_tool_filter(tool_data);
                        populate_tool_list(tool_data);
                        tool_data->selected_index = -1;
                        update_tool_details(tool_data, -1);
                        break;
                        
                    case GID_TOOL_FILTER_VALID:
                        tool_data->current_filter = TOOL_FILTER_VALID;
                        apply_tool_filter(tool_data);
                        populate_tool_list(tool_data);
                        tool_data->selected_index = -1;
                        update_tool_details(tool_data, -1);
                        break;
                        
                    case GID_TOOL_FILTER_MISSING:
                        tool_data->current_filter = TOOL_FILTER_MISSING;
                        apply_tool_filter(tool_data);
                        populate_tool_list(tool_data);
                        tool_data->selected_index = -1;
                        update_tool_details(tool_data, -1);
                        break;
                    
                    case GID_TOOL_REBUILD_CACHE:
                        log_info(LOG_GUI, "[TOOL_CACHE] Rebuild Cache button clicked\n");
                        
                        /* Use global preferences for scan path and recursive mode */
                        log_info(LOG_GUI, "Rescanning using global preferences\n");
                        
                        /* Rescan the directory to rebuild tool cache */
                        if (ScanDirectoryForToolsOnly())
                        {
                            log_info(LOG_GUI, "Tool cache rebuilt successfully\n");
                            
                            /* Rebuild display list from refreshed cache */
                            build_tool_cache_display_list(tool_data);
                            apply_tool_filter(tool_data);
                            populate_tool_list(tool_data);
                            
                            /* Clear selection and details */
                            tool_data->selected_index = -1;
                            update_tool_details(tool_data, -1);
                            
                            log_info(LOG_GUI, "Display updated with new cache data\n");
                        }
                        else
                        {
                            log_error(LOG_GUI, "Failed to rebuild tool cache\n");
                            /* Could show an error requester here */
                        }
                        break;
                        
                    case GID_TOOL_CLOSE_BTN:
                        return FALSE;  /* Close window */
                }
                break;
                
            case IDCMP_GADGETDOWN:
                /* Handle listview scroll buttons */
                if (gadget_id == GID_TOOL_LIST)
                {
                    GT_RefreshWindow(tool_data->window, NULL);
                }
                break;
        }
    }
    
    return TRUE;  /* Continue running */
}

