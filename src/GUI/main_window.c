/*
 * main_window.c - iTidy Main Window Implementation
 * GadTools-based GUI for Workbench 3.0+
 * Based on iTidy_GUI_Feature_Design.md specification
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/asl.h>
#include <string.h>
#include <stdio.h>

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

#include "main_window.h"
#include "advanced_window.h"
#include "restore_window.h"
#include "tool_cache_window.h"
#include "default_tool_backup.h"
#include "easy_request_helper.h"
#include "layout_preferences.h"
#include "layout_processor.h"
#include "folder_scanner.h"
#include "writeLog.h"
#include "window_enumerator.h"
#include "gui_groupbox.h"
#include "StatusWindows/main_progress_window.h"
#include "../icon_types.h"

/*------------------------------------------------------------------------*/
/* External Tool Cache Variables                                         */
/*------------------------------------------------------------------------*/
extern ToolCacheEntry *g_ToolCache;
extern int g_ToolCacheCount;

/*------------------------------------------------------------------------*/
/* External Default Tool Restore Functions                               */
/*------------------------------------------------------------------------*/
extern struct Window *iTidy_CreateToolRestoreWindow(struct Screen *screen, APTR backup_manager);
extern BOOL iTidy_HandleToolRestoreWindowEvent(struct Window *window, struct IntuiMessage *msg);
extern void iTidy_CloseToolRestoreWindow(struct Window *window);

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define ITIDY_WINDOW_STANDARD_PADDING 15
#define ITIDY_WINDOW_TITLE "iTidy v1.2 - Icon Cleanup Tool"
#define ITIDY_WINDOW_WIDTH 500
#define ITIDY_WINDOW_HEIGHT 350
#define ITIDY_WINDOW_LEFT 50
#define ITIDY_WINDOW_TOP 30
#define ITIDY_WINDOW_GAP_BETWEEN_GROUPS 40
#define ITIDY_WINDOW_LEFT_GROUP_GADETS 95
#define ITIDY_WINDOW_LEFT_GROUP_GADETS_LABEL 30
#define ITIDY_WINDOW_LEFT_GROUP_GADETS_COLUMN_2 295

/* Groupbox alignment constants (shared by gadgets and groupboxes) */
#define ITIDY_GROUPBOX_LEFT_EDGE 15    /* Left edge of all groupboxes */
#define ITIDY_GROUPBOX_RIGHT_EDGE 485  /* Right edge of all groupboxes (WINDOW_WIDTH - 15) */
#define ITIDY_GROUPBOX_USABLE_WIDTH 440 /* Inner width for button calculations (470 - 30 margins) */

/*------------------------------------------------------------------------*/
/* Cycle Gadget Labels                                                   */
/*------------------------------------------------------------------------*/
static STRPTR order_labels[] = {
    "Folders First",
    "Files First",
    "Mixed",
    NULL
};

static STRPTR sortby_labels[] = {
    "Name",
    "Type",
    "Date",
    "Size",
    NULL
};

static STRPTR window_position_labels[] = {
    "Center Screen",
    "Keep Position",
    "Near Parent",
    "No Change",
    NULL
};

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static void draw_folder_path_box(struct iTidyMainWindow *win_data);

/*------------------------------------------------------------------------*/
/**
 * @brief Print LayoutPreferences structure to console
 * 
 * Outputs all fields of the LayoutPreferences structure in a
 * human-readable format for debugging and verification.
 * 
 * @param prefs Pointer to LayoutPreferences structure to print
 * @param folder Folder path being processed
 * @param recursive Recursive processing flag
 */
/*------------------------------------------------------------------------*/
static void print_layout_preferences(const LayoutPreferences *prefs, 
                                     const char *folder, 
                                     BOOL recursive)
{
    if (prefs == NULL)
    {
        append_to_log("ERROR: NULL preferences pointer\n");
        return;
    }
    
    append_to_log("\n");
    append_to_log("==================================================\n");
    append_to_log("      iTidy Layout Preferences - Debug Output    \n");
    append_to_log("==================================================\n");
    append_to_log("\n");
    
    /* Folder Settings */
    append_to_log("FOLDER SETTINGS:\n");
    append_to_log("  Target Folder:    %s\n", folder ? folder : "(none)");
    append_to_log("  Recursive:        %s\n", recursive ? "Yes" : "No");
    append_to_log("\n");
    
    /* Layout Settings */
    append_to_log("LAYOUT SETTINGS:\n");
    append_to_log("  Layout Mode:      %s\n", 
           prefs->layoutMode == LAYOUT_MODE_ROW ? "Row-major (Horizontal first)" : "Column-major (Vertical first)");
    append_to_log("  Sort Order:       %s\n",
           prefs->sortOrder == SORT_ORDER_HORIZONTAL ? "Horizontal (Left-to-right)" : "Vertical (Top-to-bottom)");
    append_to_log("  Sort Priority:    ");
    switch (prefs->sortPriority)
    {
        case SORT_PRIORITY_FOLDERS_FIRST:
            append_to_log("Folders First\n");
            break;
        case SORT_PRIORITY_FILES_FIRST:
            append_to_log("Files First\n");
            break;
        case SORT_PRIORITY_MIXED:
            append_to_log("Mixed (No priority)\n");
            break;
        default:
            append_to_log("Unknown (%d)\n", prefs->sortPriority);
            break;
    }
    append_to_log("  Sort By:          ");
    switch (prefs->sortBy)
    {
        case SORT_BY_NAME:
            append_to_log("Name\n");
            break;
        case SORT_BY_TYPE:
            append_to_log("Type\n");
            break;
        case SORT_BY_DATE:
            append_to_log("Date\n");
            break;
        case SORT_BY_SIZE:
            append_to_log("Size\n");
            break;
        default:
            append_to_log("Unknown (%d)\n", prefs->sortBy);
            break;
    }
    append_to_log("  Reverse Sort:     %s\n", prefs->reverseSort ? "Yes" : "No");
    append_to_log("\n");
    
    /* Visual Settings */
    append_to_log("VISUAL SETTINGS:\n");
    append_to_log("  Center Icons:     %s\n", prefs->centerIconsInColumn ? "Yes" : "No");
    append_to_log("  Optimize Columns: %s\n", prefs->useColumnWidthOptimization ? "Yes" : "No");
    append_to_log("\n");
    
    /* Window Management */
    append_to_log("WINDOW MANAGEMENT:\n");
    append_to_log("  Resize Windows:   %s\n", prefs->resizeWindows ? "Yes" : "No");
    append_to_log("  Max Icons/Row:    %u\n", prefs->maxIconsPerRow);
    append_to_log("  Max Width:        %u%% of screen\n", prefs->maxWindowWidthPct);
    append_to_log("  Aspect Ratio:     %.2f\n", prefs->aspectRatio);
    append_to_log("\n");
    
    /* Advanced Settings */
    append_to_log("ADVANCED SETTINGS:\n");
    append_to_log("  Skip Hidden:      %s\n", prefs->skipHiddenFolders ? "Yes" : "No");
    append_to_log("\n");
    
    /* Backup Settings */
    append_to_log("BACKUP SETTINGS:\n");
    append_to_log("  Enable Backup:    %s\n", prefs->backupPrefs.enableUndoBackup ? "Yes" : "No");
    append_to_log("  Use LhA:          %s\n", prefs->backupPrefs.useLha ? "Yes" : "No");
    append_to_log("  Backup Path:      %s\n", prefs->backupPrefs.backupRootPath);
    append_to_log("  Max Backups:      %u per folder\n", prefs->backupPrefs.maxBackupsPerFolder);
    append_to_log("\n");
    append_to_log("==================================================\n");
    append_to_log("\n");
}

/*------------------------------------------------------------------------*/
/**
 * @brief Request a directory using ASL file requester
 *
 * Opens a standard Amiga ASL file requester configured to select
 * directories only. If the user selects a directory, it is copied
 * to the provided buffer.
 *
 * @param buffer Buffer to store the selected path
 * @param buffer_size Size of the buffer
 * @param initial_path Initial directory to display (can be NULL)
 * @return BOOL TRUE if user selected a directory, FALSE if cancelled
 */
/*------------------------------------------------------------------------*/
static BOOL request_directory(char *buffer, ULONG buffer_size, const char *initial_path)
{
    struct FileRequester *freq;
    BOOL result = FALSE;
    char temp_buffer[256];
    
    /* Allocate file requester */
    freq = (struct FileRequester *)AllocAslRequest(ASL_FileRequest, NULL);
    if (freq == NULL)
    {
        CONSOLE_ERROR("Failed to allocate ASL file requester\n");
        return FALSE;
    }
    
    CONSOLE_STATUS("Opening directory requester...\n");
    
    /* Request a directory */
    if (AslRequestTags(freq,
        ASLFR_TitleText, "Select Folder to Process",
        ASLFR_DrawersOnly, TRUE,
        ASLFR_InitialDrawer, initial_path ? initial_path : "DH0:",
        ASLFR_DoPatterns, FALSE,
        TAG_END))
    {
        /* User selected a directory */
        CONSOLE_STATUS("User selected: %s\n", freq->rf_Dir);
        
        /* Copy the directory path to temp buffer */
        if (freq->rf_Dir && freq->rf_Dir[0])
        {
            strncpy(temp_buffer, freq->rf_Dir, sizeof(temp_buffer) - 1);
            temp_buffer[sizeof(temp_buffer) - 1] = '\0';
            
            /* Copy to output buffer */
            strncpy(buffer, temp_buffer, buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
            
            CONSOLE_STATUS("Selected folder: %s\n", buffer);
            result = TRUE;
        }
    }
    else
    {
        CONSOLE_STATUS("User cancelled directory selection\n");
    }
    
    /* Free the requester */
    FreeAslRequest(freq);
    
    return result;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Create all gadgets for the main window
 *
 * Creates the complete iTidy GUI using GadTools gadgets as specified
 * in the iTidy_GUI_Feature_Design.md document.
 *
 * @param win_data Pointer to window data structure
 * @param topborder Top border height for gadget positioning
 * @param out_window_height Pointer to store calculated window height
 * @return BOOL TRUE if successful, FALSE otherwise
 */
/*------------------------------------------------------------------------*/
static BOOL create_gadgets(struct iTidyMainWindow *win_data, WORD topborder, WORD *out_window_height)
{
    struct NewGadget ng;
    struct Gadget *gad;
    WORD current_y;
    WORD font_height;
    
    /* Get font height for spacing */
    font_height = win_data->screen->RastPort.TxHeight;
    
    /* Create gadget context */
    gad = CreateContext(&win_data->glist);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create gadget context\n");
        return FALSE;
    }
    
    /* Initialize NewGadget structure */
    ng.ng_TextAttr = win_data->screen->Font;
    ng.ng_VisualInfo = win_data->visual_info;
    ng.ng_Flags = 0;
    
    /* Start below the title bar with moderate spacing */
    current_y = topborder + 25;  /* 15px extra spacing for groupbox */
    

    /* *******************************************************/
    /* Group Box for Folder Selection                        */
    /*********************************************************/

    /*====================================================================*/
    /* FOLDER LABEL (TEXT GADGET)                                        */
    /*====================================================================*/
    ng.ng_LeftEdge = ITIDY_WINDOW_LEFT_GROUP_GADETS_LABEL;
    ng.ng_TopEdge = current_y;
ng.ng_Width = ITIDY_WINDOW_LEFT_GROUP_GADETS-ITIDY_WINDOW_LEFT_GROUP_GADETS_LABEL-2;
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "Folder:";
    ng.ng_GadgetID = 0;  /* No ID needed for static text */
    ng.ng_Flags = PLACETEXT_IN;
    
    win_data->folder_label = gad = CreateGadget(TEXT_KIND, gad, &ng,
        GTTX_Text, "",
        GTTX_Border, FALSE,
        TAG_END);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create folder label\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* FOLDER PATH DISPLAY BOX (Custom drawn recessed bevel)             */
    /*====================================================================*/
    /* Store coordinates for custom drawing */
    win_data->folder_box_left = ITIDY_WINDOW_LEFT_GROUP_GADETS;
    win_data->folder_box_top = current_y;
    win_data->folder_box_width = 290;
    win_data->folder_box_height = font_height + 6;
    
    /* Note: Folder path will be drawn in refresh handler */
    /* No gadget created - this is a custom drawn display area */
    win_data->folder_path = gad;  /* Keep gadget chain intact */
    
    /*====================================================================*/
    /* BROWSE BUTTON                                                      */
    /*====================================================================*/
    ng.ng_LeftEdge = 390;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 90;
    ng.ng_Height = font_height + 6;
    ng.ng_GadgetText = "Browse...";
    ng.ng_GadgetID = GID_BROWSE;
    ng.ng_Flags = PLACETEXT_IN;
    
    win_data->browse_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create browse button\n");
        return FALSE;
    }
    
    current_y += font_height + ITIDY_WINDOW_GAP_BETWEEN_GROUPS;
    




    /*====================================================================*/
    /* ORDER LABEL (TEXT GADGET) - Top row left                          */
    /*====================================================================*/
    ng.ng_LeftEdge = ITIDY_WINDOW_LEFT_GROUP_GADETS_LABEL;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = ITIDY_WINDOW_LEFT_GROUP_GADETS-ITIDY_WINDOW_LEFT_GROUP_GADETS_LABEL-2;
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "Order:";
    ng.ng_GadgetID = 0;
    ng.ng_Flags = PLACETEXT_IN;
    
    gad = CreateGadget(TEXT_KIND, gad, &ng,
        GTTX_Text, "",
        GTTX_Border, FALSE,
        TAG_END);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create order label\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* ORDER CYCLE GADGET - Top row left                                 */
    /*====================================================================*/
    ng.ng_LeftEdge = ITIDY_WINDOW_LEFT_GROUP_GADETS;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 140;
    ng.ng_Height = font_height + 6;
    ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = GID_ORDER;
    ng.ng_Flags = 0;
    
    win_data->order_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, order_labels,
        GTCY_Active, 0,
        TAG_END);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create order cycle\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* BY LABEL (TEXT GADGET) - Top row right                            */
    /*====================================================================*/
    ng.ng_LeftEdge = 260;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 30;
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "By:";
    ng.ng_GadgetID = 0;
    ng.ng_Flags = PLACETEXT_IN;
    
    gad = CreateGadget(TEXT_KIND, gad, &ng,
        GTTX_Text, "",
        GTTX_Border, FALSE,
        TAG_END);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create by label\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* SORT BY CYCLE GADGET - Top row right                              */
    /*====================================================================*/
    ng.ng_LeftEdge = ITIDY_WINDOW_LEFT_GROUP_GADETS_COLUMN_2;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 90;
    ng.ng_Height = font_height + 6;
    ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = GID_SORTBY;
    ng.ng_Flags = 0;
    
    win_data->sortby_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, sortby_labels,
        GTCY_Active, 0,
        TAG_END);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create sortby cycle\n");
        return FALSE;
    }
    
    current_y += font_height + 10;
    
    /*====================================================================*/
    /* RECURSIVE SUBFOLDERS CHECKBOX - Middle row left                   */
    /*====================================================================*/
    ng.ng_LeftEdge = ITIDY_WINDOW_LEFT_GROUP_GADETS;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "Cleanup subfolders";
    ng.ng_GadgetID = GID_RECURSIVE;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    win_data->recursive_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, FALSE,
        TAG_END);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create recursive checkbox\n");
        return FALSE;
    }
    
    //current_y += font_height + 8;
    
    /*====================================================================*/
    /* BACKUP (LHA) CHECKBOX - Middle row left (stacked below recursive) */
    /*====================================================================*/
    ng.ng_LeftEdge = ITIDY_WINDOW_LEFT_GROUP_GADETS_COLUMN_2;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "Backup icons using LhA";
    ng.ng_GadgetID = GID_BACKUP;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    win_data->backup_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, FALSE,
        TAG_END);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create backup checkbox\n");
        return FALSE;
    }
    
    current_y += font_height + 10;
    
    /*====================================================================*/
    /* WINDOW POSITION LABEL (TEXT GADGET) - Bottom row                  */
    /*====================================================================*/
    ng.ng_LeftEdge = ITIDY_WINDOW_LEFT_GROUP_GADETS_LABEL;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = ITIDY_WINDOW_LEFT_GROUP_GADETS-ITIDY_WINDOW_LEFT_GROUP_GADETS_LABEL-2;  /* Width for "Position:" text */
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "Position:";
    ng.ng_GadgetID = 0;  /* No ID needed for static text */
    ng.ng_Flags = PLACETEXT_IN;
    
    win_data->window_position_label = gad = CreateGadget(TEXT_KIND, gad, &ng,
        GTTX_Text, "",
        GTTX_Border, FALSE,
        TAG_END);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create window position label\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* WINDOW POSITION CYCLE GADGET - Bottom row center                  */
    /*====================================================================*/
    ng.ng_LeftEdge = ITIDY_WINDOW_LEFT_GROUP_GADETS;  /* After "Position:" label */
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 150;
    ng.ng_Height = font_height + 6;
    ng.ng_GadgetText = NULL;  /* No label - we have separate label gadget */
    ng.ng_GadgetID = GID_WINDOW_POSITION;
    ng.ng_Flags = 0;  /* No PLACETEXT flag */
    
    win_data->window_position_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, window_position_labels,
        GTCY_Active, 0,  /* Default to "Center Screen" */
        TAG_END);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create window position cycle\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* WINDOW POSITION HELP BUTTON - Bottom row right                    */
    /*====================================================================*/
    ng.ng_LeftEdge = ITIDY_WINDOW_LEFT_GROUP_GADETS_COLUMN_2;  /* After cycle + gap */
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 30;
    ng.ng_Height = font_height + 6;
    ng.ng_GadgetText = "?";
    ng.ng_GadgetID = GID_WINDOW_POSITION_HELP;
    ng.ng_Flags = PLACETEXT_IN;
    
    win_data->window_position_help_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        TAG_END);
    if (!gad)
    {
        CONSOLE_ERROR("Failed to create window position help button\n");
        return FALSE;
    }
    
    current_y += font_height + ITIDY_WINDOW_GAP_BETWEEN_GROUPS;
    
    /*====================================================================*/
    /* ADVANCED BUTTON (Row 1, Position 1) - Equal width buttons         */
    /* Calculate button width dynamically:                               */
    /* Total available width = RIGHT_EDGE - LEFT_EDGE - 2*PADDING        */
    /* Space for 3 buttons with 2 gaps = (available - 2*gap) / 3         */
    /*====================================================================*/
    {
        WORD button_width = ((ITIDY_GROUPBOX_RIGHT_EDGE - ITIDY_GROUPBOX_LEFT_EDGE - 2 * ITIDY_WINDOW_STANDARD_PADDING) - (2 * ITIDY_WINDOW_STANDARD_PADDING)) / 3;
        
        ng.ng_LeftEdge = ITIDY_GROUPBOX_LEFT_EDGE + ITIDY_WINDOW_STANDARD_PADDING;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = button_width;
        ng.ng_Height = font_height + 8;
        ng.ng_GadgetText = "Advanced...";
        ng.ng_GadgetID = GID_ADVANCED;
        ng.ng_Flags = PLACETEXT_IN;
    
        win_data->advanced_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
            GA_Disabled, FALSE,  /* Enabled - Phase 5 complete */
            TAG_END);
        if (!gad)
        {
            CONSOLE_ERROR("Failed to create advanced button\n");
            return FALSE;
        }
        
        /*====================================================================*/
        /* VIEW TOOL CACHE BUTTON (Row 1, Position 2) - Equal width          */
        /*====================================================================*/
        ng.ng_LeftEdge = ITIDY_GROUPBOX_LEFT_EDGE + ITIDY_WINDOW_STANDARD_PADDING + button_width + ITIDY_WINDOW_STANDARD_PADDING;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = button_width;
        ng.ng_Height = font_height + 8;
        ng.ng_GadgetText = "Fix Default Tools...";
        ng.ng_GadgetID = GID_VIEW_TOOL_CACHE;
        ng.ng_Flags = PLACETEXT_IN;
    
        win_data->view_tool_cache_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
        if (!gad)
        {
            CONSOLE_ERROR("Failed to create view tool cache button\n");
            return FALSE;
        }
        
        /*====================================================================*/
        /* RESTORE BUTTON (Row 1, Position 3) - Equal width                  */
        /*====================================================================*/
        ng.ng_LeftEdge = ITIDY_GROUPBOX_LEFT_EDGE + ITIDY_WINDOW_STANDARD_PADDING + (button_width + ITIDY_WINDOW_STANDARD_PADDING) * 2;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = button_width;
        ng.ng_Height = font_height + 8;
        ng.ng_GadgetText = "Restore Backups...";
        ng.ng_GadgetID = GID_RESTORE;
        ng.ng_Flags = PLACETEXT_IN;
        
        win_data->restore_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
            GA_Disabled, FALSE,
            TAG_END);
        if (!gad)
        {
            CONSOLE_ERROR("Failed to create restore button\n");
            return FALSE;
        }
    }
    
    current_y += font_height + 27;
    
    /*====================================================================*/
    /* APPLY BUTTON (Row 2, Position 1) - Dynamic width calculation      */
    /* Calculate button width dynamically:                               */
    /* Total available width = WINDOW_WIDTH - 2*PADDING                  */
    /* Space for 2 buttons with 1 gap = (available - gap) / 2            */
    /*====================================================================*/
    {
        WORD total_available = ITIDY_WINDOW_WIDTH - (2 * ITIDY_WINDOW_STANDARD_PADDING);
        WORD button_width = (total_available - ITIDY_WINDOW_STANDARD_PADDING) / 2;
        
        ng.ng_LeftEdge = ITIDY_WINDOW_STANDARD_PADDING;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = button_width;
        ng.ng_Height = font_height + 8;
        ng.ng_GadgetText = "Start";
        ng.ng_GadgetID = GID_APPLY;
        ng.ng_Flags = PLACETEXT_IN;
        
        win_data->apply_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
        if (!gad)
        {
            CONSOLE_ERROR("Failed to create apply button\n");
            return FALSE;
        }
        
        /*====================================================================*/
        /* CANCEL BUTTON (Row 2, Position 2) - Dynamic width calculation     */
        /*====================================================================*/
        ng.ng_LeftEdge = ITIDY_WINDOW_STANDARD_PADDING + button_width + ITIDY_WINDOW_STANDARD_PADDING;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = button_width;
        ng.ng_Height = font_height + 8;
        ng.ng_GadgetText = "Cancel";
        ng.ng_GadgetID = GID_CANCEL;
        ng.ng_Flags = PLACETEXT_IN;
        
        win_data->cancel_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
        if (!gad)
        {
            CONSOLE_ERROR("Failed to create cancel button\n");
            return FALSE;
        }
    }

    current_y += font_height + 10;
    
    /* Store calculated window height */
    if (out_window_height != NULL)
    {
        *out_window_height = current_y + ITIDY_WINDOW_STANDARD_PADDING + 1;
    }
    
    CONSOLE_STATUS("All gadgets created successfully\n");
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Open the iTidy main window with GadTools gadgets
 *
 * Creates a complete GUI interface with all controls specified in
 * iTidy_GUI_Feature_Design.md
 *
 * @param win_data Pointer to window data structure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
/*------------------------------------------------------------------------*/
BOOL open_itidy_main_window(struct iTidyMainWindow *win_data)
{
    WORD topborder;
    
    /* Validate input */
    if (win_data == NULL)
    {
        CONSOLE_ERROR("Invalid window data structure\n");
        return FALSE;
    }

    /* Initialize global preferences on first window open */
    InitializeGlobalPreferences();

    /* Initialize structure */
    memset(win_data, 0, sizeof(struct iTidyMainWindow));
    
    /* Load current global preferences into window controls */
    {
        const LayoutPreferences *prefs = GetGlobalPreferences();
        
        win_data->order_selected = prefs->sortPriority;
        win_data->sortby_selected = prefs->sortBy;
        win_data->recursive_subdirs = prefs->recursive_subdirs;
        win_data->enable_backup = prefs->enable_backup;
        win_data->window_position_selected = prefs->windowPositionMode;
        
        /* Copy folder path from prefs, or use default if empty */
        if (prefs->folder_path[0] != '\0')
        {
            strncpy(win_data->folder_path_buffer, prefs->folder_path, sizeof(win_data->folder_path_buffer) - 1);
            win_data->folder_path_buffer[sizeof(win_data->folder_path_buffer) - 1] = '\0';
        }
        else
        {
            strcpy(win_data->folder_path_buffer, "SYS:");
        }
    }

    /* Lock the Workbench screen */
    win_data->screen = LockPubScreen(NULL);
    if (win_data->screen == NULL)
    {
        CONSOLE_ERROR("Could not lock Workbench screen\n");
        return FALSE;
    }

    /* Get visual info for GadTools */
    win_data->visual_info = GetVisualInfo(win_data->screen, TAG_END);
    if (win_data->visual_info == NULL)
    {
        CONSOLE_ERROR("Could not get visual info\n");
        UnlockPubScreen(NULL, win_data->screen);
        return FALSE;
    }

    CONSOLE_STATUS("Opening iTidy main window with GadTools gadgets...\n");

    /* Calculate top border for gadget positioning */
    topborder = win_data->screen->WBorTop + win_data->screen->RastPort.TxHeight + 1;

    /* Create all gadgets and get calculated window height */
    WORD calculated_height = ITIDY_WINDOW_HEIGHT;  /* Default fallback */
    if (!create_gadgets(win_data, topborder, &calculated_height))
    {
        CONSOLE_ERROR("Failed to create gadgets\n");
        FreeVisualInfo(win_data->visual_info);
        UnlockPubScreen(NULL, win_data->screen);
        return FALSE;
    }

    /* Open the window with calculated height based on font size */
    win_data->window = OpenWindowTags(NULL,
        WA_Left, ITIDY_WINDOW_LEFT,
        WA_Top, ITIDY_WINDOW_TOP,
        WA_Width, ITIDY_WINDOW_WIDTH,
        WA_Height, calculated_height,
        WA_Title, ITIDY_WINDOW_TITLE,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, win_data->screen,
        WA_Gadgets, win_data->glist,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW,
        TAG_END);

    if (win_data->window == NULL)
    {
        CONSOLE_ERROR("Could not open window\n");
        FreeGadgets(win_data->glist);
        FreeVisualInfo(win_data->visual_info);
        UnlockPubScreen(NULL, win_data->screen);
        return FALSE;
    }

    /* Refresh gadgets */
    GT_RefreshWindow(win_data->window, NULL);
    
    /* Calculate group box rectangles for visual grouping */
    /* This must be done after window is open and gadgets have final geometry */
    CONSOLE_DEBUG("Calculating folder group box...\n");
    CalcGadgetGroupBoxRect(win_data->window,
                          win_data->folder_label,  /* Start from label, not string gadget */
                          win_data->browse_btn,
                          &win_data->folder_group_box);
    
    /* Override MinX and MaxX to enforce alignment constants */
    win_data->folder_group_box.MinX = ITIDY_GROUPBOX_LEFT_EDGE;
    win_data->folder_group_box.MaxX = ITIDY_GROUPBOX_RIGHT_EDGE;
    
    CONSOLE_DEBUG("Folder group box calculated: (%d,%d) to (%d,%d)\n",
           win_data->folder_group_box.MinX, win_data->folder_group_box.MinY,
           win_data->folder_group_box.MaxX, win_data->folder_group_box.MaxY);
    
    CONSOLE_DEBUG("Calculating tidy options group box...\n");
    CalcGadgetGroupBoxRect(win_data->window,
                          win_data->order_cycle,
                          win_data->backup_check,  /* Use backup check as end - Window Position controls added manually */
                          &win_data->tidy_options_group_box);
    
    /* Check if calculation failed (MinY would be very small or negative) */
    if (win_data->tidy_options_group_box.MinY < 50) {
        CONSOLE_WARNING("Tidy options groupbox calculation may have failed. MinY=%d\n", 
               win_data->tidy_options_group_box.MinY);
    }
    
    /* Extend the groupbox to include Window Position controls below */
    /* Window Position controls are at current_y which is about 16px below backup checkbox */
    /* Add approximately 30px to MaxY to encompass the Window Position row */
    win_data->tidy_options_group_box.MaxY += 25;
    
    /* Override MinX and MaxX to enforce alignment constants */
    win_data->tidy_options_group_box.MinX = ITIDY_GROUPBOX_LEFT_EDGE;
    win_data->tidy_options_group_box.MaxX = ITIDY_GROUPBOX_RIGHT_EDGE;
    
    CONSOLE_DEBUG("Tidy options group box calculated: (%d,%d) to (%d,%d)\n",
           win_data->tidy_options_group_box.MinX, win_data->tidy_options_group_box.MinY,
           win_data->tidy_options_group_box.MaxX, win_data->tidy_options_group_box.MaxY);
    
    CONSOLE_DEBUG("Calculating tools group box...\n");
    CalcGadgetGroupBoxRect(win_data->window,
                          win_data->advanced_btn,
                          win_data->restore_btn,
                          &win_data->tools_group_box);
    
    /* Override MinX and MaxX to enforce alignment constants */
    win_data->tools_group_box.MinX = ITIDY_GROUPBOX_LEFT_EDGE;
    win_data->tools_group_box.MaxX = ITIDY_GROUPBOX_RIGHT_EDGE;
    
    CONSOLE_DEBUG("Tools group box calculated: (%d,%d) to (%d,%d)\n",
           win_data->tools_group_box.MinX, win_data->tools_group_box.MinY,
           win_data->tools_group_box.MaxX, win_data->tools_group_box.MaxY);
    
    /* Draw the group boxes immediately after calculating */
    CONSOLE_DEBUG("Drawing initial folder group box...\n");
    DrawGroupBoxWithLabel(win_data->window,
                         win_data->visual_info,
                         &win_data->folder_group_box,
                         "Folder");
    CONSOLE_DEBUG("Initial folder group box drawing complete\n");
    
    CONSOLE_DEBUG("Drawing initial tidy options group box...\n");
    DrawGroupBoxWithLabel(win_data->window,
                         win_data->visual_info,
                         &win_data->tidy_options_group_box,
                         "Tidy options");
    CONSOLE_DEBUG("Initial tidy options group box drawing complete\n");
    
    CONSOLE_DEBUG("Drawing initial tools group box...\n");
    DrawGroupBoxWithLabel(win_data->window,
                         win_data->visual_info,
                         &win_data->tools_group_box,
                         "Tools");
    CONSOLE_DEBUG("Initial tools group box drawing complete\n");
    
    /* Draw the initial folder path display */
    draw_folder_path_box(win_data);

    /* Mark window as successfully opened */
    win_data->window_open = TRUE;

    CONSOLE_STATUS("iTidy main window opened successfully\n");
    CONSOLE_DEBUG("Window dimensions: %dx%d at position (%d,%d)\n",
           ITIDY_WINDOW_WIDTH, ITIDY_WINDOW_HEIGHT,
           ITIDY_WINDOW_LEFT, ITIDY_WINDOW_TOP);
    CONSOLE_STATUS("Ready for user interaction.\n");

    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Close the iTidy main window and cleanup resources
 *
 * Properly closes the window and releases all GadTools resources.
 * Safe to call even if window was never opened.
 *
 * @param win_data Pointer to window data structure
 */
/*------------------------------------------------------------------------*/
void close_itidy_main_window(struct iTidyMainWindow *win_data)
{
    if (win_data == NULL)
    {
        return;
    }

    CONSOLE_STATUS("Closing iTidy main window...\n");

    /* Close window if it's open */
    if (win_data->window != NULL)
    {
        CloseWindow(win_data->window);
        win_data->window = NULL;
        win_data->window_open = FALSE;
        CONSOLE_DEBUG("Window closed\n");
    }

    /* Free gadgets */
    if (win_data->glist != NULL)
    {
        FreeGadgets(win_data->glist);
        win_data->glist = NULL;
        CONSOLE_DEBUG("Gadgets freed\n");
    }

    /* Free visual info */
    if (win_data->visual_info != NULL)
    {
        FreeVisualInfo(win_data->visual_info);
        win_data->visual_info = NULL;
        CONSOLE_DEBUG("Visual info freed\n");
    }

    /* Unlock the Workbench screen */
    if (win_data->screen != NULL)
    {
        UnlockPubScreen(NULL, win_data->screen);
        win_data->screen = NULL;
        CONSOLE_DEBUG("Screen unlocked\n");
    }

    CONSOLE_STATUS("iTidy main window cleanup complete\n");
}

/*------------------------------------------------------------------------*/
/**
 * @brief Handle window events (main event loop)
 *
 * Processes Intuition and GadTools messages. Handles gadget events,
 * window refresh, and close requests.
 *
 * @param win_data Pointer to window data structure
 * @return BOOL TRUE to continue, FALSE to quit
 */
/*------------------------------------------------------------------------*/
BOOL handle_itidy_window_events(struct iTidyMainWindow *win_data)
{
    struct IntuiMessage *msg;
    ULONG msg_class;
    UWORD msg_code;
    struct Gadget *gad;
    BOOL continue_running = TRUE;

    /* Validate input */
    if (win_data == NULL || win_data->window == NULL)
    {
        return FALSE;
    }

    /* Wait for a message */
    WaitPort(win_data->window->UserPort);

    /* Process all pending messages */
    while ((msg = GT_GetIMsg(win_data->window->UserPort)))
    {
        /* Get message details before replying */
        msg_class = msg->Class;
        msg_code = msg->Code;
        gad = (struct Gadget *)msg->IAddress;

        /* Reply to the message */
        GT_ReplyIMsg(msg);

        /* Handle the message */
        switch (msg_class)
        {
            case IDCMP_CLOSEWINDOW:
                CONSOLE_STATUS("Close gadget clicked - shutting down\n");
                continue_running = FALSE;
                break;

            case IDCMP_REFRESHWINDOW:
                CONSOLE_DEBUG("REFRESHWINDOW event - drawing groupboxes\n");
                GT_BeginRefresh(win_data->window);
                
                /* Draw group box around folder path and browse button */
                CONSOLE_DEBUG("Drawing folder group box...\n");
                DrawGroupBoxWithLabel(win_data->window,
                                     win_data->visual_info,
                                     &win_data->folder_group_box,
                                     "Folder");
                CONSOLE_DEBUG("Folder group box drawn\n");
                
                /* Draw custom folder path display box */
                draw_folder_path_box(win_data);
                
                /* Draw group box around tidy options */
                CONSOLE_DEBUG("Drawing tidy options group box...\n");
                DrawGroupBoxWithLabel(win_data->window,
                                     win_data->visual_info,
                                     &win_data->tidy_options_group_box,
                                     "Tidy options");
                CONSOLE_DEBUG("Tidy options group box drawn\n");
                
                /* Draw group box around tool buttons */
                CONSOLE_DEBUG("Drawing tools group box...\n");
                DrawGroupBoxWithLabel(win_data->window,
                                     win_data->visual_info,
                                     &win_data->tools_group_box,
                                     "Tools");
                CONSOLE_DEBUG("Tools group box drawn\n");
                
                GT_EndRefresh(win_data->window, TRUE);
                break;

            case IDCMP_GADGETUP:
                /* Handle gadget events */
                switch (gad->GadgetID)
                {
                    case GID_BROWSE:
                        CONSOLE_STATUS("Browse button clicked\n");
                        /* Open ASL directory requester */
                        if (request_directory(win_data->folder_path_buffer, 
                                            sizeof(win_data->folder_path_buffer),
                                            win_data->folder_path_buffer))
                        {
                            /* User selected a new directory - redraw the folder path display */
                            draw_folder_path_box(win_data);
                            CONSOLE_STATUS("Folder path updated to: %s\n", win_data->folder_path_buffer);
                        }
                        break;

                    case GID_APPLY:
                    {
                        struct iTidyMainProgressWindow progress_window;
                        LayoutPreferences *prefs;
                        BOOL success;
                        
                        CONSOLE_NEWLINE();
                        CONSOLE_SEPARATOR();
                        CONSOLE_STATUS("Apply button clicked - Processing icons...\n");
                        CONSOLE_SEPARATOR();
                        CONSOLE_NEWLINE();
                        
                        /* Get mutable access to global preferences */
                        prefs = (LayoutPreferences *)GetGlobalPreferences();
                        
                        /* Update only the fields controlled by main window gadgets */
                        /* All other settings (aspect ratio, spacing, etc.) remain unchanged */
                        prefs->sortPriority = win_data->order_selected;
                        prefs->sortBy = win_data->sortby_selected;
                        prefs->recursive_subdirs = win_data->recursive_subdirs;
                        prefs->enable_backup = win_data->enable_backup;
                        prefs->windowPositionMode = (WindowPositionMode)win_data->window_position_selected;
                        
                        /* Update folder path from GUI */
                        strncpy(prefs->folder_path, win_data->folder_path_buffer, sizeof(prefs->folder_path) - 1);
                        prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
                        
                        /* Update backup preferences */
                        prefs->backupPrefs.enableUndoBackup = win_data->enable_backup;
                        
                        /* Advanced settings (aspect ratio, spacing, etc.) are preserved */
                        /* Beta settings are preserved */
                        
                        /* Print preferences for debugging */
                        print_layout_preferences(prefs, 
                                               prefs->folder_path,
                                               prefs->recursive_subdirs);
                        
                        /* Open progress window */
                        if (!itidy_main_progress_window_open(&progress_window))
                        {
                            CONSOLE_ERROR("Failed to open progress window\n");
                            ShowEasyRequest(win_data->window,
                                "Error",
                                "Failed to open progress window",
                                "OK");
                            break;
                        }
                        
                        /* Set busy pointer on main window */
                        SetWindowPointer(win_data->window, WA_BusyPointer, TRUE, TAG_END);
                        
                        /* Disable main window input while processing */
                        ModifyIDCMP(win_data->window, 0);
                        
                        /* Process the directory with progress window integration */
                        CONSOLE_NEWLINE();
                        CONSOLE_STATUS(">>> Starting icon processing...\n");
                        CONSOLE_NEWLINE();
                        success = ProcessDirectoryWithPreferencesAndProgress(&progress_window);
                        
                        /* Show result in progress window */
                        itidy_main_progress_window_append_status(&progress_window, "");
                        itidy_main_progress_window_append_status(&progress_window, 
                            "===============================================");
                        if (success)
                        {
                            CONSOLE_STATUS("Icon processing completed successfully!\n");
                            itidy_main_progress_window_append_status(&progress_window, 
                                "Icon processing completed successfully!");
                            itidy_main_progress_window_append_status(&progress_window, 
                                "Note you may need to restart your Amiga to see window updates.");
                        }
                        else
                        {
                            CONSOLE_WARNING("Icon processing failed or was incomplete\n");
                            itidy_main_progress_window_append_status(&progress_window, 
                                "Icon processing failed or was incomplete");
                        }
                        itidy_main_progress_window_append_status(&progress_window, 
                            "===============================================");
                        CONSOLE_SEPARATOR();
                        CONSOLE_NEWLINE();
                        
                        /* Change Cancel button to Close now that processing is complete */
                        itidy_main_progress_window_set_button_text(&progress_window, "Close");
                        
                        /* Re-enable main window */
                        ModifyIDCMP(win_data->window, 
                            IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_REFRESHWINDOW);
                        SetWindowPointer(win_data->window, WA_Pointer, NULL, TAG_END);
                        
                        /* Keep progress window open so user can review - wait for Cancel/Close */
                        while (itidy_main_progress_window_handle_events(&progress_window))
                        {
                            WaitPort(progress_window.window->UserPort);
                        }
                        
                        /* Close progress window */
                        itidy_main_progress_window_close(&progress_window);
                        
                        break;
                    }

                    case GID_CANCEL:
                        CONSOLE_STATUS("Cancel button clicked - closing window\n");
                        continue_running = FALSE;
                        break;

                    case GID_RESTORE:
                        {
                            struct iTidyRestoreWindow restore_data;
                            
                            CONSOLE_STATUS("Restore Backups button clicked - opening Restore window\n");
                            
                            /* Set busy pointer on main window */
                            SetWindowPointer(win_data->window,
                                           WA_BusyPointer, TRUE,
                                           TAG_END);
                            
                            /* Open restore window (modal) */
                            if (open_restore_window(&restore_data))
                            {
                                /* Clear busy pointer - restore window is now open */
                                SetWindowPointer(win_data->window,
                                               WA_Pointer, NULL,
                                               TAG_END);
                                
                                /* Disable main window input while restore window is open */
                                ModifyIDCMP(win_data->window, 0);
                                
                                /* Run restore window event loop */
                                while (handle_restore_window_events(&restore_data))
                                {
                                    /* Wait for restore window events */
                                    WaitPort(restore_data.window->UserPort);
                                }
                                
                                /* Close restore window */
                                close_restore_window(&restore_data);
                                
                                /* Re-enable main window input */
                                ModifyIDCMP(win_data->window, 
                                    IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW);
                                
                                CONSOLE_DEBUG("Restore window closed\n");
                            }
                            else
                            {
                                /* Clear busy pointer on error */
                                SetWindowPointer(win_data->window,
                                               WA_Pointer, NULL,
                                               TAG_END);
                                
                                CONSOLE_ERROR("Failed to open Restore window\n");
                            }
                        }
                        break;

                    case GID_ADVANCED:
                        {
                            struct iTidyAdvancedWindow adv_data;
                            LayoutPreferences temp_prefs;
                            
                            CONSOLE_STATUS("Advanced button clicked - opening Advanced Settings window\n");
                            
                            /* Get current global preferences (includes all current settings) */
                            memcpy(&temp_prefs, GetGlobalPreferences(), sizeof(LayoutPreferences));
                            
                            /* Open advanced window (modal) */
                            if (open_itidy_advanced_window(&adv_data, &temp_prefs))
                            {
                                /* Disable main window input while advanced window is open */
                                ModifyIDCMP(win_data->window, 0);
                                
                                /* Run advanced window event loop */
                                while (handle_advanced_window_events(&adv_data))
                                {
                                    /* Wait for advanced window events */
                                    WaitPort(adv_data.window->UserPort);
                                }
                                
                                /* Close advanced window */
                                close_itidy_advanced_window(&adv_data);
                                
                                /* Re-enable main window input */
                                ModifyIDCMP(win_data->window, 
                                    IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW);
                                
                                /* If changes were accepted, update global preferences */
                                if (adv_data.changes_accepted)
                                {
                                    CONSOLE_STATUS("Advanced settings accepted - updating global preferences\n");
                                    CONSOLE_DEBUG("  Aspect Ratio: %.2f\n", temp_prefs.aspectRatio / 1000.0);
                                    CONSOLE_DEBUG("  Overflow Mode: %d\n", temp_prefs.overflowMode);
                                    CONSOLE_DEBUG("  Spacing: %hux%hu\n", 
                                           temp_prefs.iconSpacingX,
                                           temp_prefs.iconSpacingY);
                                    
                                    /* Update global preferences with advanced settings */
                                    UpdateGlobalPreferences(&temp_prefs);
                                    
                                    CONSOLE_DEBUG("  (Settings will be applied when you click Apply button)\n");
                                }
                                else
                                {
                                    CONSOLE_DEBUG("Advanced settings cancelled\n");
                                }
                            }
                            else
                            {
                                CONSOLE_ERROR("Failed to open Advanced Settings window\n");
                            }
                        }
                        break;

                    case GID_ORDER:
                        /* Use msg_code which contains the new cycle gadget selection */
                        win_data->order_selected = msg_code;
                        CONSOLE_DEBUG("Order changed to: %s\n", order_labels[win_data->order_selected]);
                        append_to_log("Order cycle changed to: %d (%s)\n", 
                                    win_data->order_selected, 
                                    order_labels[win_data->order_selected]);
                        break;

                    case GID_SORTBY:
                        /* Use msg_code which contains the new cycle gadget selection */
                        win_data->sortby_selected = msg_code;
                        CONSOLE_DEBUG("Sort by changed to: %s\n", sortby_labels[win_data->sortby_selected]);
                        break;

                    case GID_WINDOW_POSITION:
                        /* Use msg_code which contains the new cycle gadget selection */
                        win_data->window_position_selected = msg_code;
                        CONSOLE_DEBUG("Window position changed to: %s\n", window_position_labels[win_data->window_position_selected]);
                        break;

                    case GID_WINDOW_POSITION_HELP:
                        {
                            /* Display help text using EasyRequest */
                            ShowEasyRequest(
                                win_data->window,
                                "Window Placement Options",
                                "Center Screen - iTidy resizes the drawer window to fit\n"
                                "the icons and centers it on the Workbench screen.\n"
                                "\n"
                                "Keep Position - iTidy resizes the drawer window to fit\n"
                                "the icons but keeps its current position. If any part\n"
                                "would go off-screen, the window is pulled fully back\n"
                                "on-screen.\n"
                                "\n"
                                "Near Parent - iTidy resizes the drawer window to fit\n"
                                "the icons and tries to place it just below and to the\n"
                                "right of its parent drawer window. If there isn't\n"
                                "enough room, it behaves like 'Keep Position'.\n"
                                "\n"
                                "No Change - iTidy does not move or resize the drawer\n"
                                "window. Only the icons are rearranged.",
                                "OK"
                            );
                        }
                        break;

                    case GID_RECURSIVE:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, win_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            win_data->recursive_subdirs = (BOOL)checked;
                            CONSOLE_DEBUG("Recursive subfolders: %s\n", win_data->recursive_subdirs ? "ON" : "OFF");
                        }
                        break;

                    case GID_BACKUP:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, win_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            win_data->enable_backup = (BOOL)checked;
                            CONSOLE_DEBUG("Backup: %s\n", win_data->enable_backup ? "ON" : "OFF");
                        }
                        break;

                    case GID_VIEW_TOOL_CACHE:
                        {
                            struct iTidyToolCacheWindow tool_window;
                            LayoutPreferences *prefs;
                            
                            log_info(LOG_GUI, "View Tool Cache button clicked\n");
                            log_info(LOG_GUI, "Opening tool cache window (cache has %d entries)\n", g_ToolCacheCount);
                            
                            /* Sync current GUI folder path and recursive mode to global preferences */
                            /* This ensures Rebuild Cache works even if user hasn't clicked Apply yet */
                            prefs = (LayoutPreferences *)GetGlobalPreferences();
                            if (prefs)
                            {
                                strncpy(prefs->folder_path, win_data->folder_path_buffer, sizeof(prefs->folder_path) - 1);
                                prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
                                prefs->recursive_subdirs = win_data->recursive_subdirs;
                                log_info(LOG_GUI, "Synced folder path to global prefs: %s (recursive: %s)\n",
                                         prefs->folder_path,
                                         prefs->recursive_subdirs ? "Yes" : "No");
                            }
                            
                            /* Open tool cache window */
                            if (open_tool_cache_window(&tool_window))
                            {
                                /* Event loop */
                                while (handle_tool_cache_window_events(&tool_window))
                                {
                                    WaitPort(tool_window.window->UserPort);
                                }
                                
                                /* Cleanup */
                                close_tool_cache_window(&tool_window);
                                log_info(LOG_GUI, "Tool cache window closed\n");
                            }
                            else
                            {
                                log_error(LOG_GUI, "Failed to open tool cache window\n");
                                (void)ShowEasyRequest(
                                    win_data->window,
                                    "Error",
                                    "Failed to open tool cache window.\n"
                                    "Check the log for details.",
                                    "OK");
                            }
                        }
                        break;

                    default:
                        break;
                }
                break;

            default:
                /* Unknown message - ignore */
                break;
        }
    }

    return continue_running;
}

/*------------------------------------------------------------------------*/
/* Draw Folder Path Display Box                                          */
/*------------------------------------------------------------------------*/
static void draw_folder_path_box(struct iTidyMainWindow *win_data)
{
    struct RastPort *rp;
    struct DrawInfo *dri;
    WORD text_x, text_y;
    
    if (!win_data || !win_data->window)
        return;
    
    rp = win_data->window->RPort;
    dri = GetScreenDrawInfo(win_data->window->WScreen);
    if (!dri)
        return;
    
    /* Draw recessed bevel box */
    DrawBevelBox(rp,
                 win_data->folder_box_left,
                 win_data->folder_box_top,
                 win_data->folder_box_width,
                 win_data->folder_box_height,
                 GT_VisualInfo, win_data->visual_info,
                 GTBB_Recessed, TRUE,
                 TAG_END);
    
    /* Draw text inside box */
    SetAPen(rp, dri->dri_Pens[TEXTPEN]);
    SetBPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
    SetDrMd(rp, JAM2);
    
    /* Position text with padding inside box */
    text_x = win_data->folder_box_left + 4;
    text_y = win_data->folder_box_top + rp->TxBaseline + 2;
    
    /* Clear background first */
    SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp,
             win_data->folder_box_left + 2,
             win_data->folder_box_top + 2,
             win_data->folder_box_left + win_data->folder_box_width - 3,
             win_data->folder_box_top + win_data->folder_box_height - 3);
    
    /* Draw text with truncation if needed */
    SetAPen(rp, dri->dri_Pens[TEXTPEN]);
    Move(rp, text_x, text_y);
    
    {
        char display_buffer[512];
        const char *text_to_display = win_data->folder_path_buffer;
        int text_len = strlen(text_to_display);
        int available_width = win_data->folder_box_width - 8; /* Account for padding */
        int text_width = TextLength(rp, text_to_display, text_len);
        
        /* Check if text fits */
        if (text_width > available_width)
        {
            /* Text is too long - truncate with ellipsis */
            const char *ellipsis = "...";
            int ellipsis_width = TextLength(rp, ellipsis, 3);
            int target_width = available_width - ellipsis_width;
            
            /* Binary search for maximum fitting length */
            int low = 0, high = text_len;
            int best_len = 0;
            
            while (low <= high)
            {
                int mid = (low + high) / 2;
                int mid_width = TextLength(rp, text_to_display, mid);
                
                if (mid_width <= target_width)
                {
                    best_len = mid;
                    low = mid + 1;
                }
                else
                {
                    high = mid - 1;
                }
            }
            
            /* Build truncated string with ellipsis */
            if (best_len > 0 && best_len < sizeof(display_buffer) - 4)
            {
                strncpy(display_buffer, text_to_display, best_len);
                strcpy(display_buffer + best_len, ellipsis);
                text_to_display = display_buffer;
                text_len = best_len + 3;
            }
        }
        
        Text(rp, text_to_display, text_len);
    }
    
    FreeScreenDrawInfo(win_data->window->WScreen, dri);
}

/* End of main_window.c */
