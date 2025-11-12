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

#include "main_window.h"
#include "advanced_window.h"
#include "restore_window.h"
#include "tool_cache_window.h"
#include "easy_request_helper.h"
#include "layout_preferences.h"
#include "layout_processor.h"
#include "writeLog.h"
#include "window_enumerator.h"
#include "../icon_types.h"

/*------------------------------------------------------------------------*/
/* External Tool Cache Variables                                         */
/*------------------------------------------------------------------------*/
extern ToolCacheEntry *g_ToolCache;
extern int g_ToolCacheCount;

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define ITIDY_WINDOW_TITLE "iTidy v1.2 - Icon Cleanup Tool"
#define ITIDY_WINDOW_WIDTH 500
#define ITIDY_WINDOW_HEIGHT 350
#define ITIDY_WINDOW_LEFT 50
#define ITIDY_WINDOW_TOP 30

/*------------------------------------------------------------------------*/
/* Cycle Gadget Labels                                                   */
/*------------------------------------------------------------------------*/
static STRPTR preset_labels[] = {
    "Classic",
    "Compact",
    "Modern",
    "WHDLoad",
    NULL
};

static STRPTR layout_labels[] = {
    "Row",
    "Column",
    NULL
};

static STRPTR sort_labels[] = {
    "Horizontal",
    "Vertical",
    NULL
};

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
        printf("ERROR: Failed to allocate ASL file requester\n");
        return FALSE;
    }
    
    printf("Opening directory requester...\n");
    
    /* Request a directory */
    if (AslRequestTags(freq,
        ASLFR_TitleText, "Select Folder to Process",
        ASLFR_DrawersOnly, TRUE,
        ASLFR_InitialDrawer, initial_path ? initial_path : "DH0:",
        ASLFR_DoPatterns, FALSE,
        TAG_END))
    {
        /* User selected a directory */
        printf("User selected: %s\n", freq->rf_Dir);
        
        /* Copy the directory path to temp buffer */
        if (freq->rf_Dir && freq->rf_Dir[0])
        {
            strncpy(temp_buffer, freq->rf_Dir, sizeof(temp_buffer) - 1);
            temp_buffer[sizeof(temp_buffer) - 1] = '\0';
            
            /* Copy to output buffer */
            strncpy(buffer, temp_buffer, buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
            
            printf("Selected folder: %s\n", buffer);
            result = TRUE;
        }
    }
    else
    {
        printf("User cancelled directory selection\n");
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
 * @return BOOL TRUE if successful, FALSE otherwise
 */
/*------------------------------------------------------------------------*/
static BOOL create_gadgets(struct iTidyMainWindow *win_data, WORD topborder)
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
        printf("ERROR: Failed to create gadget context\n");
        return FALSE;
    }
    
    /* Initialize NewGadget structure */
    ng.ng_TextAttr = win_data->screen->Font;
    ng.ng_VisualInfo = win_data->visual_info;
    ng.ng_Flags = 0;
    
    current_y = topborder + 10;
    
    /*====================================================================*/
    /* FOLDER PATH STRING GADGET                                          */
    /*====================================================================*/
    ng.ng_LeftEdge = 80;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 300;
    ng.ng_Height = font_height + 6;
    ng.ng_GadgetText = "Folder:";
    ng.ng_GadgetID = GID_FOLDER_PATH;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    win_data->folder_path = gad = CreateGadget(STRING_KIND, gad, &ng,
        GTST_String, win_data->folder_path_buffer,
        GTST_MaxChars, 255,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create folder path gadget\n");
        return FALSE;
    }
    
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
        printf("ERROR: Failed to create browse button\n");
        return FALSE;
    }
    
    current_y += font_height + 20;
    
    /*====================================================================*/
    /* PRESET CYCLE GADGET                                               */
    /*====================================================================*/
    ng.ng_LeftEdge = 80;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 200;
    ng.ng_Height = font_height + 6;
    ng.ng_GadgetText = "Preset:";
    ng.ng_GadgetID = GID_PRESET;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    win_data->preset_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, preset_labels,
        GTCY_Active, 0,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create preset cycle\n");
        return FALSE;
    }
    
    current_y += font_height + 20;
    
    /*====================================================================*/
    /* LAYOUT CYCLE GADGET                                               */
    /*====================================================================*/
    ng.ng_LeftEdge = 80;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 100;
    ng.ng_Height = font_height + 6;
    ng.ng_GadgetText = "Layout:";
    ng.ng_GadgetID = GID_LAYOUT;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    win_data->layout_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, layout_labels,
        GTCY_Active, 0,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create layout cycle\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* SORT CYCLE GADGET                                                 */
    /*====================================================================*/
    ng.ng_LeftEdge = 280;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 120;
    ng.ng_Height = font_height + 6;
    ng.ng_GadgetText = "Sort:";
    ng.ng_GadgetID = GID_SORT;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    win_data->sort_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, sort_labels,
        GTCY_Active, 0,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create sort cycle\n");
        return FALSE;
    }
    
    current_y += font_height + 16;
    
    /*====================================================================*/
    /* ORDER CYCLE GADGET                                                */
    /*====================================================================*/
    ng.ng_LeftEdge = 80;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 140;
    ng.ng_Height = font_height + 6;
    ng.ng_GadgetText = "Order:";
    ng.ng_GadgetID = GID_ORDER;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    win_data->order_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, order_labels,
        GTCY_Active, 0,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create order cycle\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* SORT BY CYCLE GADGET                                              */
    /*====================================================================*/
    ng.ng_LeftEdge = 310;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 90;
    ng.ng_Height = font_height + 6;
    ng.ng_GadgetText = "By:";
    ng.ng_GadgetID = GID_SORTBY;
    ng.ng_Flags = PLACETEXT_LEFT;
    
    win_data->sortby_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
        GTCY_Labels, sortby_labels,
        GTCY_Active, 0,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create sortby cycle\n");
        return FALSE;
    }
    
    current_y += font_height + 16;
    
    /*====================================================================*/
    /* CENTER ICONS CHECKBOX                                             */
    /*====================================================================*/
    ng.ng_LeftEdge = 30;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "Center icons";
    ng.ng_GadgetID = GID_CENTER_ICONS;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    win_data->center_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, FALSE,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create center icons checkbox\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* OPTIMIZE COLUMNS CHECKBOX                                         */
    /*====================================================================*/
    ng.ng_LeftEdge = 250;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "Optimize columns";
    ng.ng_GadgetID = GID_OPTIMIZE_COLS;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    win_data->optimize_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, TRUE,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create optimize checkbox\n");
        return FALSE;
    }
    
    current_y += font_height + 20;
    
    /*====================================================================*/
    /* RECURSIVE SUBFOLDERS CHECKBOX                                     */
    /*====================================================================*/
    ng.ng_LeftEdge = 30;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "Recursive Subfolders";
    ng.ng_GadgetID = GID_RECURSIVE;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    win_data->recursive_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, FALSE,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create recursive checkbox\n");
        return FALSE;
    }
    
    current_y += font_height + 12;
    
    /*====================================================================*/
    /* BACKUP (LHA) CHECKBOX                                             */
    /*====================================================================*/
    ng.ng_LeftEdge = 30;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "Backup (LHA)";
    ng.ng_GadgetID = GID_BACKUP;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    win_data->backup_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, FALSE,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create backup checkbox\n");
        return FALSE;
    }
    
    current_y += font_height + 12;
    
    /*====================================================================*/
    /* ICON UPGRADE CHECKBOX (disabled for now)                          */
    /*====================================================================*/
    ng.ng_LeftEdge = 30;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "Icon Upgrade (future)";
    ng.ng_GadgetID = GID_ICON_UPGRADE;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    win_data->iconupgrade_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, FALSE,
        GA_Disabled, TRUE,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create icon upgrade checkbox\n");
        return FALSE;
    }
    
    current_y += font_height + 10;
    
    /*====================================================================*/
    /* SKIP HIDDEN FOLDERS CHECKBOX                                      */
    /*====================================================================*/
    ng.ng_LeftEdge = 30;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 26;
    ng.ng_Height = font_height + 4;
    ng.ng_GadgetText = "Skip Hidden Folders";
    ng.ng_GadgetID = GID_SKIP_HIDDEN;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    win_data->skip_hidden_check = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, TRUE,  /* Default to enabled */
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create skip hidden folders checkbox\n");
        return FALSE;
    }
    
    current_y += font_height + 20;
    
    /*====================================================================*/
    /* ADVANCED BUTTON                                                    */
    /*====================================================================*/
    ng.ng_LeftEdge = 30;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 100;
    ng.ng_Height = font_height + 8;
    ng.ng_GadgetText = "Advanced...";
    ng.ng_GadgetID = GID_ADVANCED;
    ng.ng_Flags = PLACETEXT_IN;
    
    win_data->advanced_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        GA_Disabled, FALSE,  /* Enabled - Phase 5 complete */
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create advanced button\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* RESTORE BUTTON                                                     */
    /*====================================================================*/
    ng.ng_LeftEdge = 140;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 130;
    ng.ng_Height = font_height + 8;
    ng.ng_GadgetText = "Restore Backups...";
    ng.ng_GadgetID = GID_RESTORE;
    ng.ng_Flags = PLACETEXT_IN;
    
    win_data->restore_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        GA_Disabled, FALSE,
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create restore button\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* APPLY BUTTON                                                       */
    /*====================================================================*/
    ng.ng_LeftEdge = 280;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 100;
    ng.ng_Height = font_height + 8;
    ng.ng_GadgetText = "Apply";
    ng.ng_GadgetID = GID_APPLY;
    ng.ng_Flags = PLACETEXT_IN;
    
    win_data->apply_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create apply button\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* CANCEL BUTTON                                                      */
    /*====================================================================*/
    ng.ng_LeftEdge = 390;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 100;
    ng.ng_Height = font_height + 8;
    ng.ng_GadgetText = "Cancel";
    ng.ng_GadgetID = GID_CANCEL;
    ng.ng_Flags = PLACETEXT_IN;
    
    win_data->cancel_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create cancel button\n");
        return FALSE;
    }
    
    current_y += font_height + 16;
    
    /*====================================================================*/
    /* ENUMERATE WINDOWS BUTTON (Debug Tool)                             */
    /*====================================================================*/
    ng.ng_LeftEdge = 30;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = 180;
    ng.ng_Height = font_height + 8;
    ng.ng_GadgetText = "View Tool Cache";
    ng.ng_GadgetID = GID_VIEW_TOOL_CACHE;
    ng.ng_Flags = PLACETEXT_IN;
    
    win_data->view_tool_cache_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create view tool cache button\n");
        return FALSE;
    }
    
    printf("All gadgets created successfully\n");
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
        printf("ERROR: Invalid window data structure\n");
        return FALSE;
    }

    /* Initialize global preferences on first window open */
    InitializeGlobalPreferences();

    /* Initialize structure */
    memset(win_data, 0, sizeof(struct iTidyMainWindow));
    strcpy(win_data->folder_path_buffer, "PC:");
    
    /* Initialize default settings */
    win_data->preset_selected = 0;       /* Classic */
    win_data->layout_selected = 0;       /* Row */
    win_data->sort_selected = 0;         /* Horizontal */
    win_data->order_selected = 0;        /* Folders First */
    win_data->sortby_selected = 0;       /* Name */
    win_data->center_icons = FALSE;
    win_data->optimize_columns = TRUE;
    win_data->recursive_subdirs = FALSE;
    win_data->enable_backup = FALSE;
    win_data->enable_icon_upgrade = FALSE;
    win_data->skip_hidden_folders = TRUE;  /* Default: skip hidden folders */
    
    /* Initialize advanced settings flag */
    win_data->has_advanced_settings = FALSE;

    /* Lock the Workbench screen */
    win_data->screen = LockPubScreen(NULL);
    if (win_data->screen == NULL)
    {
        printf("ERROR: Could not lock Workbench screen\n");
        return FALSE;
    }

    /* Get visual info for GadTools */
    win_data->visual_info = GetVisualInfo(win_data->screen, TAG_END);
    if (win_data->visual_info == NULL)
    {
        printf("ERROR: Could not get visual info\n");
        UnlockPubScreen(NULL, win_data->screen);
        return FALSE;
    }

    printf("Opening iTidy main window with GadTools gadgets...\n");

    /* Calculate top border for gadget positioning */
    topborder = win_data->screen->WBorTop + win_data->screen->RastPort.TxHeight + 1;

    /* Create all gadgets */
    if (!create_gadgets(win_data, topborder))
    {
        printf("ERROR: Failed to create gadgets\n");
        FreeVisualInfo(win_data->visual_info);
        UnlockPubScreen(NULL, win_data->screen);
        return FALSE;
    }

    /* Open the window */
    win_data->window = OpenWindowTags(NULL,
        WA_Left, ITIDY_WINDOW_LEFT,
        WA_Top, ITIDY_WINDOW_TOP,
        WA_Width, ITIDY_WINDOW_WIDTH,
        WA_Height, ITIDY_WINDOW_HEIGHT,
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
        printf("ERROR: Could not open window\n");
        FreeGadgets(win_data->glist);
        FreeVisualInfo(win_data->visual_info);
        UnlockPubScreen(NULL, win_data->screen);
        return FALSE;
    }

    /* Refresh gadgets */
    GT_RefreshWindow(win_data->window, NULL);

    /* Mark window as successfully opened */
    win_data->window_open = TRUE;

    printf("iTidy main window opened successfully\n");
    printf("Window dimensions: %dx%d at position (%d,%d)\n",
           ITIDY_WINDOW_WIDTH, ITIDY_WINDOW_HEIGHT,
           ITIDY_WINDOW_LEFT, ITIDY_WINDOW_TOP);
    printf("Ready for user interaction.\n");

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

    printf("Closing iTidy main window...\n");

    /* Close window if it's open */
    if (win_data->window != NULL)
    {
        CloseWindow(win_data->window);
        win_data->window = NULL;
        win_data->window_open = FALSE;
        printf("Window closed\n");
    }

    /* Free gadgets */
    if (win_data->glist != NULL)
    {
        FreeGadgets(win_data->glist);
        win_data->glist = NULL;
        printf("Gadgets freed\n");
    }

    /* Free visual info */
    if (win_data->visual_info != NULL)
    {
        FreeVisualInfo(win_data->visual_info);
        win_data->visual_info = NULL;
        printf("Visual info freed\n");
    }

    /* Unlock the Workbench screen */
    if (win_data->screen != NULL)
    {
        UnlockPubScreen(NULL, win_data->screen);
        win_data->screen = NULL;
        printf("Screen unlocked\n");
    }

    printf("iTidy main window cleanup complete\n");
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
                printf("Close gadget clicked - shutting down\n");
                continue_running = FALSE;
                break;

            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(win_data->window);
                GT_EndRefresh(win_data->window, TRUE);
                break;

            case IDCMP_GADGETUP:
                /* Handle gadget events */
                switch (gad->GadgetID)
                {
                    case GID_BROWSE:
                        printf("Browse button clicked\n");
                        /* Open ASL directory requester */
                        if (request_directory(win_data->folder_path_buffer, 
                                            sizeof(win_data->folder_path_buffer),
                                            win_data->folder_path_buffer))
                        {
                            /* User selected a new directory - update the string gadget */
                            GT_SetGadgetAttrs(win_data->folder_path, win_data->window, NULL,
                                GTST_String, win_data->folder_path_buffer,
                                TAG_END);
                            printf("Folder path updated to: %s\n", win_data->folder_path_buffer);
                        }
                        break;

                    case GID_APPLY:
                    {
                        LayoutPreferences *prefs;
                        BOOL success;
                        char *current_path;
                        ULONG recursive_checked = 0;
                        
                        printf("\n===============================================\n");
                        printf("Apply button clicked - Processing icons...\n");
                        printf("===============================================\n\n");
                        
                        /* Read current folder path from string gadget (in case user typed it) */
                        GT_GetGadgetAttrs(win_data->folder_path, win_data->window, NULL,
                            GTST_String, &current_path,
                            TAG_END);
                        
                        if (current_path != NULL)
                        {
                            strncpy(win_data->folder_path_buffer, current_path, sizeof(win_data->folder_path_buffer) - 1);
                            win_data->folder_path_buffer[sizeof(win_data->folder_path_buffer) - 1] = '\0';
                        }
                        
                        /* Read current recursive checkbox state */
                        GT_GetGadgetAttrs(win_data->recursive_check, win_data->window, NULL,
                            GTCB_Checked, &recursive_checked,
                            TAG_END);
                        win_data->recursive_subdirs = (BOOL)recursive_checked;
                        
                        /* Get mutable access to global preferences */
                        prefs = (LayoutPreferences *)GetGlobalPreferences();
                        
                        /* Apply preset if one was selected */
                        append_to_log("Applying preset: %s\n", preset_labels[win_data->preset_selected]);
                        ApplyPreset(prefs, win_data->preset_selected);
                        
                        /* Override with user's GUI selections */
                        MapGuiToPreferences(prefs,
                            win_data->layout_selected,
                            win_data->sort_selected,
                            win_data->order_selected,
                            win_data->sortby_selected,
                            win_data->center_icons,
                            win_data->optimize_columns);
                        
                        /* Set folder path and recursive mode from GUI */
                        strncpy(prefs->folder_path, win_data->folder_path_buffer, sizeof(prefs->folder_path) - 1);
                        prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
                        prefs->recursive_subdirs = win_data->recursive_subdirs;
                        
                        /* Set skip hidden folders and backup preferences from GUI */
                        prefs->skipHiddenFolders = win_data->skip_hidden_folders;
                        prefs->enable_backup = win_data->enable_backup;
                        prefs->enable_icon_upgrade = win_data->enable_icon_upgrade;
                        prefs->backupPrefs.enableUndoBackup = win_data->enable_backup;
                        
                        /* Advanced settings are already in global prefs (set by Advanced window) */
                        /* Beta settings are already in global prefs (set by Beta Options window) */
                        
                        /* Print preferences for debugging */
                        print_layout_preferences(prefs, 
                                               prefs->folder_path,
                                               prefs->recursive_subdirs);
                        
                        /* Process the directory with global preferences */
                        printf("\n>>> Starting icon processing...\n\n");
                        success = ProcessDirectoryWithPreferences();
                        
                        /* Show result */
                        printf("\n===============================================\n");
                        if (success)
                        {
                            printf("✓ Icon processing completed successfully!\n");
                        }
                        else
                        {
                            printf("✗ Icon processing failed or was incomplete\n");
                        }
                        printf("===============================================\n\n");
                        
                        break;
                    }

                    case GID_CANCEL:
                        printf("Cancel button clicked - closing window\n");
                        continue_running = FALSE;
                        break;

                    case GID_RESTORE:
                        {
                            struct iTidyRestoreWindow restore_data;
                            
                            printf("Restore Backups button clicked - opening Restore window\n");
                            
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
                                
                                printf("Restore window closed\n");
                            }
                            else
                            {
                                /* Clear busy pointer on error */
                                SetWindowPointer(win_data->window,
                                               WA_Pointer, NULL,
                                               TAG_END);
                                
                                printf("ERROR: Failed to open Restore window\n");
                            }
                        }
                        break;

                    case GID_ADVANCED:
                        {
                            struct iTidyAdvancedWindow adv_data;
                            LayoutPreferences temp_prefs;
                            
                            printf("Advanced button clicked - opening Advanced Settings window\n");
                            
                            /* Get current global preferences */
                            memcpy(&temp_prefs, GetGlobalPreferences(), sizeof(LayoutPreferences));
                            
                            /* Apply current preset to get baseline settings */
                            ApplyPreset(&temp_prefs, win_data->preset_selected);
                            
                            /* Also apply GUI selections to get current state */
                            MapGuiToPreferences(&temp_prefs,
                                win_data->layout_selected,
                                win_data->sort_selected,
                                win_data->order_selected,
                                win_data->sortby_selected,
                                win_data->center_icons,
                                win_data->optimize_columns);
                            
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
                                    printf("Advanced settings accepted - updating global preferences\n");
                                    printf("  Aspect Ratio: %.2f\n", temp_prefs.aspectRatio);
                                    printf("  Overflow Mode: %d\n", temp_prefs.overflowMode);
                                    printf("  Spacing: %hux%hu\n", 
                                           temp_prefs.iconSpacingX,
                                           temp_prefs.iconSpacingY);
                                    
                                    /* Update global preferences with advanced settings */
                                    UpdateGlobalPreferences(&temp_prefs);
                                    win_data->has_advanced_settings = TRUE;
                                    
                                    printf("  (Settings will be applied when you click Apply button)\n");
                                }
                                else
                                {
                                    printf("Advanced settings cancelled\n");
                                }
                            }
                            else
                            {
                                printf("ERROR: Failed to open Advanced Settings window\n");
                            }
                        }
                        break;

                    case GID_FOLDER_PATH:
                        printf("Folder path changed: %s\n", win_data->folder_path_buffer);
                        break;

                    case GID_PRESET:
                        /* Use msg_code which contains the new cycle gadget selection */
                        win_data->preset_selected = msg_code;
                        printf("Preset changed to: %s\n", preset_labels[win_data->preset_selected]);
                        
                        /* Clear advanced settings when preset is changed */
                        win_data->has_advanced_settings = FALSE;
                        printf("  (Advanced settings cleared - will use preset defaults)\n");
                        
                        /* Update all cycle gadgets to match preset */
                        switch (win_data->preset_selected)
                        {
                            case 0: /* Classic */
                                GT_SetGadgetAttrs(win_data->layout_cycle, win_data->window, NULL,
                                    GTCY_Active, 0, TAG_END);  /* Row */
                                GT_SetGadgetAttrs(win_data->sort_cycle, win_data->window, NULL,
                                    GTCY_Active, 0, TAG_END);  /* Horizontal */
                                GT_SetGadgetAttrs(win_data->order_cycle, win_data->window, NULL,
                                    GTCY_Active, 0, TAG_END);  /* Folders First */
                                GT_SetGadgetAttrs(win_data->sortby_cycle, win_data->window, NULL,
                                    GTCY_Active, 0, TAG_END);  /* Name */
                                win_data->layout_selected = 0;
                                win_data->sort_selected = 0;
                                win_data->order_selected = 0;
                                win_data->sortby_selected = 0;
                                break;
                                
                            case 1: /* Compact */
                                GT_SetGadgetAttrs(win_data->layout_cycle, win_data->window, NULL,
                                    GTCY_Active, 1, TAG_END);  /* Column */
                                GT_SetGadgetAttrs(win_data->sort_cycle, win_data->window, NULL,
                                    GTCY_Active, 1, TAG_END);  /* Vertical */
                                GT_SetGadgetAttrs(win_data->order_cycle, win_data->window, NULL,
                                    GTCY_Active, 0, TAG_END);  /* Folders First */
                                GT_SetGadgetAttrs(win_data->sortby_cycle, win_data->window, NULL,
                                    GTCY_Active, 0, TAG_END);  /* Name */
                                win_data->layout_selected = 1;
                                win_data->sort_selected = 1;
                                win_data->order_selected = 0;
                                win_data->sortby_selected = 0;
                                break;
                                
                            case 2: /* Modern */
                                GT_SetGadgetAttrs(win_data->layout_cycle, win_data->window, NULL,
                                    GTCY_Active, 0, TAG_END);  /* Row */
                                GT_SetGadgetAttrs(win_data->sort_cycle, win_data->window, NULL,
                                    GTCY_Active, 0, TAG_END);  /* Horizontal */
                                GT_SetGadgetAttrs(win_data->order_cycle, win_data->window, NULL,
                                    GTCY_Active, 2, TAG_END);  /* Mixed */
                                GT_SetGadgetAttrs(win_data->sortby_cycle, win_data->window, NULL,
                                    GTCY_Active, 2, TAG_END);  /* Date */
                                win_data->layout_selected = 0;
                                win_data->sort_selected = 0;
                                win_data->order_selected = 2;
                                win_data->sortby_selected = 2;
                                break;
                                
                            case 3: /* WHDLoad */
                                GT_SetGadgetAttrs(win_data->layout_cycle, win_data->window, NULL,
                                    GTCY_Active, 1, TAG_END);  /* Column */
                                GT_SetGadgetAttrs(win_data->sort_cycle, win_data->window, NULL,
                                    GTCY_Active, 1, TAG_END);  /* Vertical */
                                GT_SetGadgetAttrs(win_data->order_cycle, win_data->window, NULL,
                                    GTCY_Active, 0, TAG_END);  /* Folders First */
                                GT_SetGadgetAttrs(win_data->sortby_cycle, win_data->window, NULL,
                                    GTCY_Active, 0, TAG_END);  /* Name */
                                win_data->layout_selected = 1;
                                win_data->sort_selected = 1;
                                win_data->order_selected = 0;
                                win_data->sortby_selected = 0;
                                break;
                        }
                        break;

                    case GID_LAYOUT:
                        /* Use msg_code which contains the new cycle gadget selection */
                        win_data->layout_selected = msg_code;
                        printf("Layout changed to: %s\n", layout_labels[win_data->layout_selected]);
                        break;

                    case GID_SORT:
                        /* Use msg_code which contains the new cycle gadget selection */
                        win_data->sort_selected = msg_code;
                        printf("Sort changed to: %s\n", sort_labels[win_data->sort_selected]);
                        break;

                    case GID_ORDER:
                        /* Use msg_code which contains the new cycle gadget selection */
                        win_data->order_selected = msg_code;
                        printf("Order changed to: %s\n", order_labels[win_data->order_selected]);
                        append_to_log("Order cycle changed to: %d (%s)\n", 
                                    win_data->order_selected, 
                                    order_labels[win_data->order_selected]);
                        break;

                    case GID_SORTBY:
                        /* Use msg_code which contains the new cycle gadget selection */
                        win_data->sortby_selected = msg_code;
                        printf("Sort by changed to: %s\n", sortby_labels[win_data->sortby_selected]);
                        break;

                    case GID_CENTER_ICONS:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, win_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            win_data->center_icons = (BOOL)checked;
                            printf("Center icons: %s\n", win_data->center_icons ? "ON" : "OFF");
                        }
                        break;

                    case GID_OPTIMIZE_COLS:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, win_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            win_data->optimize_columns = (BOOL)checked;
                            printf("Optimize columns: %s\n", win_data->optimize_columns ? "ON" : "OFF");
                        }
                        break;

                    case GID_RECURSIVE:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, win_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            win_data->recursive_subdirs = (BOOL)checked;
                            printf("Recursive subfolders: %s\n", win_data->recursive_subdirs ? "ON" : "OFF");
                        }
                        break;

                    case GID_BACKUP:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, win_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            win_data->enable_backup = (BOOL)checked;
                            printf("Backup: %s\n", win_data->enable_backup ? "ON" : "OFF");
                        }
                        break;

                    case GID_SKIP_HIDDEN:
                        {
                            ULONG checked = 0;
                            GT_GetGadgetAttrs(gad, win_data->window, NULL,
                                GTCB_Checked, &checked,
                                TAG_END);
                            win_data->skip_hidden_folders = (BOOL)checked;
                            printf("Skip hidden folders: %s\n", win_data->skip_hidden_folders ? "ON" : "OFF");
                        }
                        break;

                    case GID_VIEW_TOOL_CACHE:
                        {
                            struct iTidyToolCacheWindow tool_window;
                            LayoutPreferences *prefs;
                            char *current_path;
                            ULONG recursive_checked = 0;
                            
                            log_info(LOG_GUI, "View Tool Cache button clicked\n");
                            log_info(LOG_GUI, "Opening tool cache window (cache has %d entries)\n", g_ToolCacheCount);
                            
                            /* Read current folder path from string gadget (in case user typed it) */
                            GT_GetGadgetAttrs(win_data->folder_path, win_data->window, NULL,
                                GTST_String, &current_path,
                                TAG_END);
                            
                            log_info(LOG_GUI, "String gadget value: '%s'\n", current_path ? current_path : "(null)");
                            log_info(LOG_GUI, "Buffer before update: '%s'\n", win_data->folder_path_buffer);
                            
                            if (current_path != NULL)
                            {
                                strncpy(win_data->folder_path_buffer, current_path, sizeof(win_data->folder_path_buffer) - 1);
                                win_data->folder_path_buffer[sizeof(win_data->folder_path_buffer) - 1] = '\0';
                            }
                            
                            log_info(LOG_GUI, "Buffer after update: '%s'\n", win_data->folder_path_buffer);
                            
                            /* Read current recursive checkbox state */
                            GT_GetGadgetAttrs(win_data->recursive_check, win_data->window, NULL,
                                GTCB_Checked, &recursive_checked,
                                TAG_END);
                            win_data->recursive_subdirs = (BOOL)recursive_checked;
                            
                            log_info(LOG_GUI, "Recursive checkbox state: %s\n", win_data->recursive_subdirs ? "YES" : "NO");
                            
                            /* Update global preferences with current GUI values for Rebuild Cache button */
                            prefs = (LayoutPreferences *)GetGlobalPreferences();
                            strncpy(prefs->folder_path, win_data->folder_path_buffer, sizeof(prefs->folder_path) - 1);
                            prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
                            prefs->recursive_subdirs = win_data->recursive_subdirs;
                            
                            log_info(LOG_GUI, "Updated global prefs: path='%s', recursive=%s\n",
                                     prefs->folder_path, prefs->recursive_subdirs ? "YES" : "NO");
                            
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

/* End of main_window.c */
