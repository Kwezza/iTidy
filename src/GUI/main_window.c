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
#include "version_info.h"
#include "advanced_window.h"
#include "RestoreBackups/restore_window.h"
#include "DefaultTools/tool_cache_window.h"
#include "DefaultTools/default_tool_backup.h"
#include "easy_request_helper.h"
#include "layout_preferences.h"
#include "icon_types.h"  /* For BuildPathSearchList() */
#include "layout_processor.h"
#include "folder_scanner.h"
#include "writeLog.h"
#include "window_enumerator.h"
#include "gui_groupbox.h"
#include "StatusWindows/main_progress_window.h"
#include "../icon_types.h"
#include "../path_utilities.h"
#include "GUI/gui_utilities.h"
#include "Settings/WorkbenchPrefs.h"
#include "../backup_lha.h"

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

/*------------------------------------------------------------------------*/
/* External ToolType Access (from main_gui.c)                            */
/*------------------------------------------------------------------------*/
extern UWORD get_tooltype_debug_level(void);
extern void iTidy_CloseToolRestoreWindow(struct Window *window);

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define ITIDY_WINDOW_STANDARD_PADDING 15
#define ITIDY_WINDOW_TITLE "iTidy v" ITIDY_VERSION " - Icon Cleanup Tool"
#define ITIDY_WINDOW_WIDTH 625
#define ITIDY_WINDOW_HEIGHT 350
#define ITIDY_WINDOW_LEFT 50
#define ITIDY_WINDOW_TOP 30
#define ITIDY_WINDOW_GAP_BETWEEN_GROUPS 30
#define ITIDY_WINDOW_LEFT_GROUP_GADETS 95
#define ITIDY_WINDOW_LEFT_GROUP_GADETS_LABEL 30
#define ITIDY_WINDOW_LEFT_GROUP_GADETS_COLUMN_2 356

/* Groupbox alignment constants (shared by gadgets and groupboxes) */
#define ITIDY_GROUPBOX_LEFT_EDGE 15    /* Left edge of all groupboxes */
#define ITIDY_GROUPBOX_RIGHT_EDGE 610  /* Right edge of all groupboxes (WINDOW_WIDTH - 15) */
#define ITIDY_GROUPBOX_USABLE_WIDTH 565 /* Inner width for button calculations (470 - 30 margins) */

/*------------------------------------------------------------------------*/
/* Menu Item IDs                                                          */
/*------------------------------------------------------------------------*/
#define MENU_PROJECT_NEW        1001
#define MENU_PROJECT_OPEN       1002
#define MENU_PROJECT_SAVE       1003
#define MENU_PROJECT_SAVE_AS    1004
#define MENU_PROJECT_CLOSE      1005
#define MENU_PROJECT_ABOUT      1006

/*------------------------------------------------------------------------*/
/* Menu System Global Variables                                          */
/*------------------------------------------------------------------------*/
static struct Screen *wb_screen_menu = NULL;     /* Workbench screen for menus */
static struct DrawInfo *draw_info_menu = NULL;   /* Drawing info for menus */
static APTR visual_info_menu = NULL;             /* Visual info for menus */
static struct Menu *main_menu_strip = NULL;      /* Menu strip */

/*------------------------------------------------------------------------*/
/* Menu Template                                                          */
/*------------------------------------------------------------------------*/
static struct NewMenu main_window_menu_template[] = 
{
	{ NM_TITLE, "Project",      NULL, 0, 0, NULL },
	{ NM_ITEM,  "New",          "N",  0, 0, (APTR)MENU_PROJECT_NEW },
	{ NM_ITEM,  "Open...",      "O",  0, 0, (APTR)MENU_PROJECT_OPEN },
	{ NM_ITEM,  NM_BARLABEL,    NULL, 0, 0, NULL },
	{ NM_ITEM,  "Save",         "S",  0, 0, (APTR)MENU_PROJECT_SAVE },
	{ NM_ITEM,  "Save as...",   "A",  0, 0, (APTR)MENU_PROJECT_SAVE_AS },
	{ NM_ITEM,  NM_BARLABEL,    NULL, 0, 0, NULL },
    { NM_ITEM,  "About...",     NULL, 0, 0, (APTR)MENU_PROJECT_ABOUT },
    { NM_ITEM,  NM_BARLABEL,    NULL, 0, 0, NULL },
	{ NM_ITEM,  "Close",        "C",  0, 0, (APTR)MENU_PROJECT_CLOSE },
	{ NM_END,   NULL,           NULL, 0, 0, NULL }
};

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
static BOOL setup_main_window_menus(void);
static void cleanup_main_window_menus(void);
static BOOL handle_main_window_menu_selection(ULONG menu_number, struct iTidyMainWindow *win_data);
static void handle_main_new_menu(struct iTidyMainWindow *win_data);
static void handle_main_open_menu(struct iTidyMainWindow *win_data);
static void handle_main_save_menu(struct iTidyMainWindow *win_data);
static void handle_main_save_as_menu(struct iTidyMainWindow *win_data);
static void handle_main_about_menu(struct iTidyMainWindow *win_data);
static BOOL save_preferences_to_file(const char *filepath, const LayoutPreferences *prefs);
static BOOL load_preferences_from_file(const char *filepath, LayoutPreferences *prefs);
static void sync_gui_from_preferences(struct iTidyMainWindow *win_data, const LayoutPreferences *prefs);
static void sync_gui_to_preferences(struct iTidyMainWindow *win_data, LayoutPreferences *prefs);

/*------------------------------------------------------------------------*/
/* Menu System Functions                                                  */
/*------------------------------------------------------------------------*/

/**
 * setup_main_window_menus - Initialize GadTools NewLook menu system
 * 
 * This function sets up GadTools menus with proper Workbench 3.x NewLook
 * appearance. The menus will automatically use system colors and modern
 * white background styling.
 *
 * Returns: TRUE if successful, FALSE on failure
 */
static BOOL setup_main_window_menus(void)
{
    /* Lock Workbench screen for menu system */
    wb_screen_menu = LockPubScreen("Workbench");
    if (!wb_screen_menu)
    {
        log_error(LOG_GUI, "Error: Could not lock Workbench screen for menus\n");
        return FALSE;
    }
    
    /* Get screen drawing information */
    draw_info_menu = GetScreenDrawInfo(wb_screen_menu);
    if (!draw_info_menu)
    {
        log_error(LOG_GUI, "Error: Could not get screen DrawInfo for menus\n");
        UnlockPubScreen(NULL, wb_screen_menu);
        wb_screen_menu = NULL;
        return FALSE;
    }
    
    /* Get visual information for GadTools */
    visual_info_menu = GetVisualInfo(wb_screen_menu, TAG_END);
    if (!visual_info_menu)
    {
        log_error(LOG_GUI, "Error: Could not get VisualInfo for menus\n");
        FreeScreenDrawInfo(wb_screen_menu, draw_info_menu);
        UnlockPubScreen(NULL, wb_screen_menu);
        draw_info_menu = NULL;
        wb_screen_menu = NULL;
        return FALSE;
    }
    
    /* Create menu strip from template */
    main_menu_strip = CreateMenus(main_window_menu_template, TAG_END);
    if (!main_menu_strip)
    {
        log_error(LOG_GUI, "Error: Could not create menu strip\n");
        FreeVisualInfo(visual_info_menu);
        FreeScreenDrawInfo(wb_screen_menu, draw_info_menu);
        UnlockPubScreen(NULL, wb_screen_menu);
        visual_info_menu = NULL;
        draw_info_menu = NULL;
        wb_screen_menu = NULL;
        return FALSE;
    }
    
    /* Layout menus - use NewLook appearance on WB 3.0+ only */
    /* On WB 2.x, GTMN_NewLookMenus is not available */
    BOOL layout_success;
    if (prefsWorkbench.workbenchVersion >= 39)
    {
        /* WB 3.0+ - Use NewLook menus */
        layout_success = LayoutMenus(main_menu_strip, visual_info_menu, GTMN_NewLookMenus, TRUE, TAG_END);
        log_info(LOG_GUI, "Using NewLook menus (WB 3.0+)\n");
    }
    else
    {
        /* WB 2.x - Use classic menu layout */
        layout_success = LayoutMenus(main_menu_strip, visual_info_menu, TAG_END);
        log_info(LOG_GUI, "Using classic menus (WB 2.x)\n");
    }
    
    if (!layout_success)
    {
        log_error(LOG_GUI, "Error: Could not layout menus\n");
        FreeMenus(main_menu_strip);
        FreeVisualInfo(visual_info_menu);
        FreeScreenDrawInfo(wb_screen_menu, draw_info_menu);
        UnlockPubScreen(NULL, wb_screen_menu);
        main_menu_strip = NULL;
        visual_info_menu = NULL;
        draw_info_menu = NULL;
        wb_screen_menu = NULL;
        return FALSE;
    }
    
    log_info(LOG_GUI, "Main window menus initialized successfully\n");
    return TRUE;
}

/**
 * cleanup_main_window_menus - Release all menu system resources
 * 
 * This function properly releases all resources allocated during
 * menu setup, following proper AmigaOS resource management.
 */
static void cleanup_main_window_menus(void)
{
    if (main_menu_strip)
    {
        FreeMenus(main_menu_strip);
        main_menu_strip = NULL;
    }
    
    if (visual_info_menu)
    {
        FreeVisualInfo(visual_info_menu);
        visual_info_menu = NULL;
    }
    
    if (draw_info_menu)
    {
        FreeScreenDrawInfo(wb_screen_menu, draw_info_menu);
        draw_info_menu = NULL;
    }
    
    if (wb_screen_menu)
    {
        UnlockPubScreen(NULL, wb_screen_menu);
        wb_screen_menu = NULL;
    }
    
    log_info(LOG_GUI, "Main window menus cleaned up\n");
}

/**
 * handle_main_window_menu_selection - Process menu item selections
 * 
 * @param menu_number: The menu selection number from IDCMP_MENUPICK
 * @param win_data: Pointer to main window data structure
 * @return: TRUE to continue running, FALSE to close window
 */
static BOOL handle_main_window_menu_selection(ULONG menu_number, struct iTidyMainWindow *win_data)
{
    struct MenuItem *menu_item = NULL;
    ULONG item_id = 0;
    BOOL continue_running = TRUE;
    
    while (menu_number != MENUNULL)
    {
        menu_item = ItemAddress(main_menu_strip, menu_number);
        if (menu_item)
        {
            item_id = (ULONG)GTMENUITEM_USERDATA(menu_item);
            
            /* Handle menu selections */
            switch (item_id)
            {
                case MENU_PROJECT_NEW:
                    handle_main_new_menu(win_data);
                    break;
                    
                case MENU_PROJECT_OPEN:
                    handle_main_open_menu(win_data);
                    break;
                    
                case MENU_PROJECT_SAVE:
                    handle_main_save_menu(win_data);
                    break;
                    
                case MENU_PROJECT_SAVE_AS:
                    handle_main_save_as_menu(win_data);
                    break;
                    
                case MENU_PROJECT_CLOSE:
                    continue_running = FALSE;  /* Close window */
                    break;
                    
                case MENU_PROJECT_ABOUT:
                    handle_main_about_menu(win_data);
                    break;

                default:
                    log_warning(LOG_GUI, "Unknown menu item ID: %ld\n", item_id);
                    break;
            }
        }
        
        menu_number = menu_item->NextSelect;
    }
    
    return continue_running;
}

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
    /* BROWSE BUTTON - Calculate position from right edge FIRST          */
    /*====================================================================*/
    {
        WORD browse_width = 90;
        WORD browse_left = ITIDY_GROUPBOX_RIGHT_EDGE - ITIDY_WINDOW_STANDARD_PADDING - browse_width;
        
        ng.ng_LeftEdge = browse_left;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = browse_width;
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
        /* Width dynamically calculated to fill space between label and      */
        /* Browse button: browse_left - gap - label_right                    */
        /*====================================================================*/
        win_data->folder_box_left = ITIDY_WINDOW_LEFT_GROUP_GADETS;
        win_data->folder_box_top = current_y;
        win_data->folder_box_width = browse_left - ITIDY_WINDOW_LEFT_GROUP_GADETS - ITIDY_WINDOW_STANDARD_PADDING;
        win_data->folder_box_height = font_height + 6;
        
        /* Note: Folder path will be drawn in refresh handler */
        /* No gadget created - this is a custom drawn display area */
        win_data->folder_path = gad;  /* Keep gadget chain intact */
    }
    
    current_y += font_height + ITIDY_WINDOW_GAP_BETWEEN_GROUPS;
    
    /* *******************************************************/
    /* Group Box for Tidy Options - Two Column Layout       */
    /*********************************************************/
    
    /*====================================================================*/
    /* Calculate two-column layout with measured label widths            */
    /*====================================================================*/
    {
        struct RastPort *rp = &win_data->screen->RastPort;
        WORD available_width;
        WORD column_gap;
        WORD column_width;
        WORD left_col_gadget_x;
        WORD right_col_gadget_x;
        WORD left_cycle_width;
        WORD right_cycle_width;
        
        /* Measure label widths using current font */
        WORD order_label_width = TextLength(rp, "Order:", 6);
        WORD by_label_width = TextLength(rp, "By:", 3);
        WORD position_label_width = TextLength(rp, "Position:", 9);
        
        /* Find widest label in each column */
        WORD left_label_width = (order_label_width > position_label_width) ? order_label_width : position_label_width;
        WORD right_label_width = by_label_width;
        
        /* Calculate midpoint of usable area using bit shift (avoids division) */
        available_width = ITIDY_GROUPBOX_RIGHT_EDGE - ITIDY_GROUPBOX_LEFT_EDGE - (2 * ITIDY_WINDOW_STANDARD_PADDING);
        WORD content_left = ITIDY_GROUPBOX_LEFT_EDGE + ITIDY_WINDOW_STANDARD_PADDING;
        WORD midpoint = content_left + (available_width >> 1);  /* Bit shift right = divide by 2 */
        
        /* Position left column gadgets (start after label + gap) */
        left_col_gadget_x = content_left + left_label_width + 8;
        
        /* Position right column gadgets at midpoint (with small adjustment + label + gap) */
        right_col_gadget_x = midpoint + 15 + right_label_width + 8;
        
        /* Calculate cycle widths to fill to edge/midpoint */
        left_cycle_width = midpoint - left_col_gadget_x - 15;
        right_cycle_width = ITIDY_GROUPBOX_RIGHT_EDGE - ITIDY_WINDOW_STANDARD_PADDING - right_col_gadget_x;
        
        /*====================================================================*/
        /* ORDER CYCLE GADGET - Top row left with built-in label             */
        /*====================================================================*/
        ng.ng_LeftEdge = left_col_gadget_x;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = left_cycle_width;
        ng.ng_Height = font_height + 6;
        ng.ng_GadgetText = "Order:";
        ng.ng_GadgetID = GID_ORDER;
        ng.ng_Flags = PLACETEXT_LEFT;  /* GadTools places label to left, right-justified */
        
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
        /* SORT BY CYCLE GADGET - Top row right with built-in label          */
        /*====================================================================*/
        ng.ng_LeftEdge = right_col_gadget_x;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = right_cycle_width;
        ng.ng_Height = font_height + 6;
        ng.ng_GadgetText = "By:";
        ng.ng_GadgetID = GID_SORTBY;
        ng.ng_Flags = PLACETEXT_LEFT;  /* GadTools places label to left, right-justified */
        
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
        ng.ng_LeftEdge = left_col_gadget_x;
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
        
        /*====================================================================*/
        /* BACKUP (LHA) CHECKBOX - Middle row right                          */
        /*====================================================================*/
        ng.ng_LeftEdge = right_col_gadget_x;
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
        /* WINDOW POSITION CYCLE GADGET - Bottom row left with built-in label*/
        /*====================================================================*/
        ng.ng_LeftEdge = left_col_gadget_x;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = left_cycle_width;
        ng.ng_Height = font_height + 6;
        ng.ng_GadgetText = "Position:";
        ng.ng_GadgetID = GID_WINDOW_POSITION;
        ng.ng_Flags = PLACETEXT_LEFT;  /* GadTools places label to left, right-justified */
        
        win_data->window_position_cycle = gad = CreateGadget(CYCLE_KIND, gad, &ng,
            GTCY_Labels, window_position_labels,
            GTCY_Active, 0,
            TAG_END);
        if (!gad)
        {
            CONSOLE_ERROR("Failed to create window position cycle\n");
            return FALSE;
        }
        
        /*====================================================================*/
        /* WINDOW POSITION HELP BUTTON - Bottom row right                    */
        /*====================================================================*/
        ng.ng_LeftEdge = right_col_gadget_x;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = 26;
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
    }
    
    current_y += font_height + ITIDY_WINDOW_GAP_BETWEEN_GROUPS;
    
    /*====================================================================*/
    /* ADVANCED BUTTON (Row 1, Position 1) - Equal width buttons         */
    /* Calculate button width dynamically:                               */
    /* Inner content width = RIGHT_EDGE - LEFT_EDGE - 2*PADDING (for box borders) */
    /* Space for 3 buttons with 2 gaps = (inner_width - 2*gap) / 3       */
    /*====================================================================*/
    {
        WORD inner_width = ITIDY_GROUPBOX_RIGHT_EDGE - ITIDY_GROUPBOX_LEFT_EDGE - (2 * ITIDY_WINDOW_STANDARD_PADDING);
        WORD button_width = (inner_width - (2 * ITIDY_WINDOW_STANDARD_PADDING)) / 3;
        
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
    /* These buttons exist outside group boxes, align with groupbox edges */
    /* Total width = RIGHT_EDGE - LEFT_EDGE (align with group box borders) */
    /* Space for 2 buttons with 1 gap = (total - gap) / 2                */
    /*====================================================================*/
    {
        WORD total_width = ITIDY_GROUPBOX_RIGHT_EDGE - ITIDY_GROUPBOX_LEFT_EDGE;
        WORD button_width = (total_width - ITIDY_WINDOW_STANDARD_PADDING) / 2;
        
        ng.ng_LeftEdge = ITIDY_GROUPBOX_LEFT_EDGE;
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
        
        log_debug(LOG_GUI, "Start button created: X=%d, Width=%d, RightEdge=%d\n", 
                 ng.ng_LeftEdge, ng.ng_Width, ng.ng_LeftEdge + ng.ng_Width);
        
        /*====================================================================*/
        /* CANCEL BUTTON (Row 2, Position 2) - Dynamic width calculation     */
        /* Make this button extend to GROUPBOX_RIGHT_EDGE exactly            */
        /* IMPORTANT: +3 pixels added to compensate for GadTools button      */
        /* internal bevel rendering. The button gadget structure uses full   */
        /* width, but the visible button face is drawn inset by ~3 pixels    */
        /* from the gadget boundary. Without this adjustment, the visual     */
        /* right edge appears 3 pixels short of the group box border.        */
        /*====================================================================*/
        ng.ng_LeftEdge = ITIDY_GROUPBOX_LEFT_EDGE + button_width + ITIDY_WINDOW_STANDARD_PADDING;
        ng.ng_TopEdge = current_y;
        ng.ng_Width = ITIDY_GROUPBOX_RIGHT_EDGE - ng.ng_LeftEdge + 3;  /* +3 compensates for button bevel inset */
        ng.ng_Height = font_height + 8;
        ng.ng_GadgetText = "Exit";
        ng.ng_GadgetID = GID_CANCEL;
        ng.ng_Flags = PLACETEXT_IN;
        
        log_debug(LOG_GUI, "Exit button calculation: total_width=%d, button_width=%d, LeftEdge=%d, Width=%d, Expected RightEdge=%d\n",
                 total_width, button_width, ng.ng_LeftEdge, ng.ng_Width, ng.ng_LeftEdge + ng.ng_Width);
        log_debug(LOG_GUI, "ITIDY_GROUPBOX_RIGHT_EDGE=%d, ITIDY_GROUPBOX_LEFT_EDGE=%d\n",
                 ITIDY_GROUPBOX_RIGHT_EDGE, ITIDY_GROUPBOX_LEFT_EDGE);
        
        win_data->cancel_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
        if (!gad)
        {
            CONSOLE_ERROR("Failed to create cancel button\n");
            return FALSE;
        }
        
        log_debug(LOG_GUI, "Exit button created: X=%d, Width=%d, RightEdge=%d\n",
                 ng.ng_LeftEdge, ng.ng_Width, ng.ng_LeftEdge + ng.ng_Width);
        log_debug(LOG_GUI, "Exit button ACTUAL gadget: LeftEdge=%d, Width=%d, RightEdge=%d\n",
                 gad->LeftEdge, gad->Width, gad->LeftEdge + gad->Width);
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
    
    /* Apply tooltype settings to preferences (if any were found during startup) */
    {
        LayoutPreferences temp_prefs;
        const LayoutPreferences *current_prefs = GetGlobalPreferences();
        UWORD tooltype_log_level;
        
        /* Copy current preferences */
        memcpy(&temp_prefs, current_prefs, sizeof(LayoutPreferences));
        
        /* Apply tooltype log level if it was set at startup */
        tooltype_log_level = get_tooltype_debug_level();
        temp_prefs.logLevel = tooltype_log_level;
        
        /* Update global preferences with tooltype-modified values */
        UpdateGlobalPreferences(&temp_prefs);
    }

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

    /* Setup menu system BEFORE opening window */
    if (!setup_main_window_menus())
    {
        CONSOLE_ERROR("ERROR: Could not setup menus\n");
        FreeGadgets(win_data->glist);
        FreeVisualInfo(win_data->visual_info);
        UnlockPubScreen(NULL, win_data->screen);
        return FALSE;
    }

    /* Open the window with calculated height based on font size */
    /* WA_NewLookMenus is only available on WB 3.0+ (Intuition v39+) */
    if (prefsWorkbench.workbenchVersion >= 39)
    {
        /* WB 3.0+ - Enable NewLook menus */
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
            WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK,
            WA_NewLookMenus, TRUE,  /* Enable NewLook menus (WB 3.0+) */
            TAG_END);
    }
    else
    {
        /* WB 2.x - Classic menus (no WA_NewLookMenus tag) */
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
            WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK,
            TAG_END);
    }

    if (win_data->window == NULL)
    {
        CONSOLE_ERROR("Could not open window\n");
        cleanup_main_window_menus();
        FreeGadgets(win_data->glist);
        FreeVisualInfo(win_data->visual_info);
        UnlockPubScreen(NULL, win_data->screen);
        return FALSE;
    }

    /* Attach menu strip to window */
    if (main_menu_strip)
    {
        SetMenuStrip(win_data->window, main_menu_strip);
        log_info(LOG_GUI, "Menu strip attached to main window\n");
    }

    /* Refresh gadgets */
    GT_RefreshWindow(win_data->window, NULL);
    
    /* Calculate group box rectangles for visual grouping */
    /* This must be done after window is open and gadgets have final geometry */
    CONSOLE_DEBUG("Calculating folder group box...\n");
    CalcGadgetGroupBoxRect(win_data->window,
                          win_data->browse_btn,
                          win_data->folder_label,  /* Gadgets are chained in creation order */
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

    /* Build PATH search list for default tool validation (deferred until after window opens) */
    /* Show busy pointer while parsing S:Startup-Sequence and S:User-Startup */
    CONSOLE_STATUS("Building system PATH list...\n");
    safe_set_window_pointer(win_data->window, TRUE);
    
    log_info(LOG_GENERAL, "Building system PATH search list (deferred after window open)...\n");
    if (!BuildPathSearchList())
    {
        log_warning(LOG_GENERAL, "Failed to build PATH search list - default tool validation may be limited\n");
    }
    else
    {
        log_info(LOG_GENERAL, "PATH search list built successfully\n");
    }
    
    safe_set_window_pointer(win_data->window, FALSE);
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
        /* Clear menu strip before closing window */
        if (main_menu_strip)
        {
            ClearMenuStrip(win_data->window);
            log_info(LOG_GUI, "Menu strip cleared from main window\n");
        }
        
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

    /* Clean up menu system */
    cleanup_main_window_menus();

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
                        
                        /* Check if LHA is available when backup is enabled */
                        if (prefs->enable_backup)
                        {
                            char lha_path[32];
                            if (!CheckLhaAvailable(lha_path))
                            {
                                /* LHA not found - ask user if they want to continue */
                                LONG result = ShowEasyRequest(win_data->window,
                                    "LHA Not Found",
                                    "LHA archiver not found in C:, SYS:C/, or SYS:Tools/.\n"
                                    "Backups cannot be created without LHA.\n\n"
                                    "Continue without backups?",
                                    "Continue|Cancel");
                                
                                if (result == 0)
                                {
                                    /* User chose Cancel - abort operation */
                                    CONSOLE_STATUS("Operation cancelled - LHA not available\n");
                                    break;
                                }
                                else
                                {
                                    /* User chose Continue - disable backup */
                                    CONSOLE_WARNING("Continuing without backups (LHA not found)\n");
                                    prefs->enable_backup = FALSE;
                                    prefs->backupPrefs.enableUndoBackup = FALSE;
                                    win_data->enable_backup = FALSE;
                                    
                                    /* Update checkbox to reflect disabled backup */
                                    if (win_data->backup_check)
                                    {
                                        GT_SetGadgetAttrs(win_data->backup_check, win_data->window, NULL,
                                                         GTCB_Checked, FALSE,
                                                         TAG_DONE);
                                    }
                                }
                            }
                        }
                        
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
                        safe_set_window_pointer(win_data->window, TRUE);
                        
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
                            IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK);
                        safe_set_window_pointer(win_data->window, FALSE);
                        
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
                            safe_set_window_pointer(win_data->window, TRUE);
                            
                            /* Open restore window (modal) */
                            if (open_restore_window(&restore_data))
                            {
                                /* Clear busy pointer - restore window is now open */
                                safe_set_window_pointer(win_data->window, FALSE);
                                
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
                                    IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK);
                                
                                CONSOLE_DEBUG("Restore window closed\n");
                            }
                            else
                            {
                                /* Clear busy pointer on error */
                                safe_set_window_pointer(win_data->window, FALSE);
                                
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
                                    IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK);
                                
                                /* If changes were accepted, update global preferences */
                                if (adv_data.changes_accepted)
                                {
                                    CONSOLE_STATUS("Advanced settings accepted - updating global preferences\n");
                                    CONSOLE_DEBUG("  Aspect Ratio: %lu\n", (unsigned long)temp_prefs.aspectRatio);
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

            case IDCMP_MENUPICK:
                /* Handle menu selections */
                if (!handle_main_window_menu_selection(msg_code, win_data))
                {
                    return FALSE;  /* Menu requested window close */
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
    text_y = win_data->folder_box_top + rp->TxBaseline + 3; /* Moved down by 1px for better vertical centering */
    
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

/*------------------------------------------------------------------------*/
/* Save/Load Preferences Functions                                        */
/*------------------------------------------------------------------------*/

/**
 * save_preferences_to_file - Save LayoutPreferences to binary file
 * 
 * Writes the LayoutPreferences structure to a binary file in a format
 * suitable for reloading. File format:
 *   - Header: "ITIDYPREFS" (10 bytes)
 *   - Version: ULONG (4 bytes) = 1
 *   - LayoutPreferences structure (binary)
 * 
 * @param filepath Full path to save file
 * @param prefs Pointer to LayoutPreferences to save
 * @return TRUE if successful, FALSE on error
 */
static BOOL save_preferences_to_file(const char *filepath, const LayoutPreferences *prefs)
{
    BPTR file;
    const char header[] = "ITIDYPREFS";
    ULONG version = 1;
    
    if (!filepath || !prefs)
    {
        log_error(LOG_GUI, "save_preferences_to_file: Invalid parameters\n");
        return FALSE;
    }
    
    /* Open file for writing */
    file = Open((STRPTR)filepath, MODE_NEWFILE);
    if (!file)
    {
        LONG error = IoErr();
        log_error(LOG_GUI, "save_preferences_to_file: Failed to create file: %s (IoErr: %ld)\n", filepath, error);
        return FALSE;
    }
    
    log_info(LOG_GUI, "Saving preferences to: %s\n", filepath);
    
    /* Write header */
    if (Write(file, (APTR)header, 10) != 10)
    {
        log_error(LOG_GUI, "Failed to write header\n");
        Close(file);
        return FALSE;
    }
    
    /* Write version */
    if (Write(file, (APTR)&version, sizeof(ULONG)) != sizeof(ULONG))
    {
        log_error(LOG_GUI, "Failed to write version\n");
        Close(file);
        return FALSE;
    }
    
    /* Write LayoutPreferences structure */
    if (Write(file, (APTR)prefs, sizeof(LayoutPreferences)) != sizeof(LayoutPreferences))
    {
        log_error(LOG_GUI, "Failed to write preferences structure\n");
        Close(file);
        return FALSE;
    }
    
    Close(file);
    log_info(LOG_GUI, "Successfully saved preferences to: %s\n", filepath);
    return TRUE;
}

/**
 * load_preferences_from_file - Load LayoutPreferences from binary file
 * 
 * Reads and validates a preferences file, then loads into provided structure.
 * 
 * @param filepath Full path to load file
 * @param prefs Pointer to LayoutPreferences to receive loaded data
 * @return TRUE if successful, FALSE on error
 */
static BOOL load_preferences_from_file(const char *filepath, LayoutPreferences *prefs)
{
    BPTR file;
    char header[11];
    ULONG version;
    
    if (!filepath || !prefs)
    {
        log_error(LOG_GUI, "load_preferences_from_file: Invalid parameters\n");
        return FALSE;
    }
    
    log_info(LOG_GUI, "Loading preferences from: %s\n", filepath);
    
    /* Open file for reading */
    file = Open((STRPTR)filepath, MODE_OLDFILE);
    if (!file)
    {
        log_error(LOG_GUI, "Failed to open file for reading: %s\n", filepath);
        return FALSE;
    }
    
    /* Read and validate header */
    memset(header, 0, sizeof(header));
    if (Read(file, (APTR)header, 10) != 10)
    {
        log_error(LOG_GUI, "Failed to read header\n");
        Close(file);
        return FALSE;
    }
    
    if (strncmp(header, "ITIDYPREFS", 10) != 0)
    {
        log_error(LOG_GUI, "Invalid file header: %s\n", header);
        Close(file);
        return FALSE;
    }
    
    /* Read version */
    if (Read(file, (APTR)&version, sizeof(ULONG)) != sizeof(ULONG))
    {
        log_error(LOG_GUI, "Failed to read version\n");
        Close(file);
        return FALSE;
    }
    
    if (version != 1)
    {
        log_error(LOG_GUI, "Unsupported file version: %lu\n", version);
        Close(file);
        return FALSE;
    }
    
    /* Read LayoutPreferences structure */
    if (Read(file, (APTR)prefs, sizeof(LayoutPreferences)) != sizeof(LayoutPreferences))
    {
        log_error(LOG_GUI, "Failed to read preferences structure\n");
        Close(file);
        return FALSE;
    }
    
    Close(file);
    log_info(LOG_GUI, "Successfully loaded preferences from: %s\n", filepath);
    return TRUE;
}

/**
 * sync_gui_from_preferences - Update all GUI gadgets from preferences
 * 
 * @param win_data Pointer to main window data
 * @param prefs Pointer to LayoutPreferences to sync from
 */
static void sync_gui_from_preferences(struct iTidyMainWindow *win_data, const LayoutPreferences *prefs)
{
    if (!win_data || !win_data->window || !prefs)
        return;
    
    /* Update window data from preferences */
    win_data->order_selected = prefs->sortPriority;
    win_data->sortby_selected = prefs->sortBy;
    win_data->recursive_subdirs = prefs->recursive_subdirs;
    win_data->enable_backup = prefs->enable_backup;
    win_data->window_position_selected = prefs->windowPositionMode;
    
    strncpy(win_data->folder_path_buffer, prefs->folder_path, sizeof(win_data->folder_path_buffer) - 1);
    win_data->folder_path_buffer[sizeof(win_data->folder_path_buffer) - 1] = '\0';
    
    /* Update gadgets */
    if (win_data->order_cycle)
    {
        GT_SetGadgetAttrs(win_data->order_cycle, win_data->window, NULL,
                         GTCY_Active, win_data->order_selected,
                         TAG_DONE);
    }
    
    if (win_data->sortby_cycle)
    {
        GT_SetGadgetAttrs(win_data->sortby_cycle, win_data->window, NULL,
                         GTCY_Active, win_data->sortby_selected,
                         TAG_DONE);
    }
    
    if (win_data->recursive_check)
    {
        GT_SetGadgetAttrs(win_data->recursive_check, win_data->window, NULL,
                         GTCB_Checked, win_data->recursive_subdirs,
                         TAG_DONE);
    }
    
    if (win_data->backup_check)
    {
        GT_SetGadgetAttrs(win_data->backup_check, win_data->window, NULL,
                         GTCB_Checked, win_data->enable_backup,
                         TAG_DONE);
    }
    
    if (win_data->window_position_cycle)
    {
        GT_SetGadgetAttrs(win_data->window_position_cycle, win_data->window, NULL,
                         GTCY_Active, win_data->window_position_selected,
                         TAG_DONE);
    }
    
    /* Redraw folder path */
    draw_folder_path_box(win_data);
    
    log_info(LOG_GUI, "GUI synchronized from loaded preferences\n");
}

/**
 * sync_gui_to_preferences - Update preferences from current GUI state
 * 
 * @param win_data Pointer to main window data
 * @param prefs Pointer to LayoutPreferences to update
 */
static void sync_gui_to_preferences(struct iTidyMainWindow *win_data, LayoutPreferences *prefs)
{
    if (!win_data || !prefs)
        return;
    
    /* Update preferences from current GUI state */
    prefs->sortPriority = win_data->order_selected;
    prefs->sortBy = win_data->sortby_selected;
    prefs->recursive_subdirs = win_data->recursive_subdirs;
    prefs->enable_backup = win_data->enable_backup;
    prefs->windowPositionMode = (WindowPositionMode)win_data->window_position_selected;
    
    /* Update folder path from GUI buffer */
    strncpy(prefs->folder_path, win_data->folder_path_buffer, sizeof(prefs->folder_path) - 1);
    prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
    
    /* Update backup preferences */
    prefs->backupPrefs.enableUndoBackup = win_data->enable_backup;
    
    log_debug(LOG_GUI, "Preferences updated from GUI state (folder: %s)\n", prefs->folder_path);
}

/*------------------------------------------------------------------------*/
/* Helper Functions for Menu Operations                                  */
/*------------------------------------------------------------------------*/

/**
 * ensure_directory_exists - Create directory if it doesn't exist
 * 
 * Due to WinUAE shared folder bugs, Lock() may succeed for non-existent paths.
 * Strategy: Always try CreateDir() first. If it fails with ERROR_OBJECT_EXISTS (203),
 * then verify it's actually a directory using Lock() and Examine().
 * Per AmigaDOS autodoc: CreateDir() fails with trailing '/' in path.
 * 
 * @param path Directory path to check/create
 * @return TRUE if directory exists or was created, FALSE on error
 */
static BOOL ensure_directory_exists(const char *path)
{
    BPTR lock;
    struct FileInfoBlock *fib = NULL;
    BOOL isDirectory = FALSE;
    char pathWithoutSlash[512];
    size_t len;
    LONG error;
    
    if (!path || path[0] == '\0')
        return FALSE;
    
    /* Strip trailing slash before CreateDir (per AmigaDOS autodoc) */
    strncpy(pathWithoutSlash, path, sizeof(pathWithoutSlash) - 1);
    pathWithoutSlash[sizeof(pathWithoutSlash) - 1] = '\0';
    len = strlen(pathWithoutSlash);
    if (len > 0 && pathWithoutSlash[len - 1] == '/')
    {
        pathWithoutSlash[len - 1] = '\0';
    }
    
    /* Try to create directory first (fast path for new directories) */
    log_debug(LOG_GUI, "Attempting to create directory: %s\n", pathWithoutSlash);
    lock = CreateDir((CONST_STRPTR)pathWithoutSlash);
    if (lock)
    {
        UnLock(lock);
        log_info(LOG_GUI, "Successfully created directory: %s\n", pathWithoutSlash);
        return TRUE;
    }
    
    /* CreateDir failed - check why */
    error = IoErr();
    log_debug(LOG_GUI, "CreateDir failed for: %s (IoErr: %ld)\n", pathWithoutSlash, error);
    
    /* ERROR_OBJECT_EXISTS (203) means something already exists at this path */
    if (error == 203)
    {
        /* Verify it's actually a directory */
        log_debug(LOG_GUI, "Object exists, verifying it's a directory: %s\n", path);
        lock = Lock((CONST_STRPTR)path, ACCESS_READ);
        if (lock)
        {
            fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
            if (fib)
            {
                if (Examine(lock, fib))
                {
                    isDirectory = (fib->fib_DirEntryType > 0);
                    log_debug(LOG_GUI, "Examine: fib_DirEntryType=%ld (isDir=%s)\n", 
                             fib->fib_DirEntryType, isDirectory ? "YES" : "NO");
                }
                FreeDosObject(DOS_FIB, fib);
            }
            UnLock(lock);
            
            if (isDirectory)
            {
                log_debug(LOG_GUI, "Directory already exists: %s\n", path);
                return TRUE;
            }
            else
            {
                log_error(LOG_GUI, "Path exists but is not a directory: %s\n", path);
                return FALSE;
            }
        }
    }
    
    log_error(LOG_GUI, "Failed to create or verify directory: %s (IoErr: %ld)\n", pathWithoutSlash, error);
    return FALSE;
}

/**
 * ensure_save_directories - Ensure userdata and Settings folders exist
 * 
 * Creates the userdata and userdata/Settings directory hierarchy if needed.
 * This is called before any save operation.
 * 
 * IMPORTANT NOTE - WinUAE Shared Folder Quirk:
 * WinUAE's shared folder filesystem caches directory state. If directories
 * are deleted from the Windows host while WinUAE is running, the cache becomes
 * stale and Lock()/Examine() may succeed for non-existent paths while Open()
 * fails with ERROR_OBJECT_NOT_FOUND. This only affects development/testing
 * when manually deleting directories from Windows. Normal users won't encounter
 * this. Restarting WinUAE clears the cache and resolves the issue.
 * 
 * We use PROGDIR: relative paths (not absolute paths) as this works more
 * reliably with WinUAE shared folders.
 * 
 * @return TRUE if directories exist or were created, FALSE on error
 */
static BOOL ensure_save_directories(void)
{
    BPTR lock;
    
    /* Create userdata directory using PROGDIR: relative path */
    lock = CreateDir((CONST_STRPTR)"PROGDIR:userdata");
    if (lock)
    {
        UnLock(lock);
        log_info(LOG_GUI, "Created PROGDIR:userdata\n");
    }
    else
    {
        LONG error = IoErr();
        if (error != 203)  /* 203 = ERROR_OBJECT_EXISTS (already exists, OK) */
        {
            log_error(LOG_GUI, "Failed to create userdata directory (IoErr: %ld)\n", error);
            return FALSE;
        }
    }
    
    /* Create Settings subdirectory using PROGDIR: relative path */
    lock = CreateDir((CONST_STRPTR)"PROGDIR:userdata/Settings");
    if (lock)
    {
        UnLock(lock);
        log_info(LOG_GUI, "Created PROGDIR:userdata/Settings\n");
    }
    else
    {
        LONG error = IoErr();
        if (error != 203)  /* 203 = ERROR_OBJECT_EXISTS (already exists, OK) */
        {
            log_error(LOG_GUI, "Failed to create Settings directory (IoErr: %ld)\n", error);
            return FALSE;
        }
    }
    
    log_debug(LOG_GUI, "Save directories verified/created successfully\n");
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Menu Handler Functions                                                */
/*------------------------------------------------------------------------*/

/**
 * handle_main_new_menu - Handle "New" menu selection
 * 
 * Resets all preferences to defaults and updates GUI.
 * 
 * @param win_data Pointer to main window data
 */
static void handle_main_new_menu(struct iTidyMainWindow *win_data)
{
    LayoutPreferences default_prefs;
    
    if (!win_data || !win_data->window)
        return;
    
    log_debug(LOG_GUI, "Menu: New clicked - resetting to defaults\n");
    
    /* Initialize default preferences */
    InitLayoutPreferences(&default_prefs);
    
    /* Update global preferences */
    UpdateGlobalPreferences(&default_prefs);
    
    /* Sync GUI from new defaults */
    sync_gui_from_preferences(win_data, &default_prefs);
    
    /* Clear last save path */
    win_data->last_save_path[0] = '\0';
    
    /* Refresh window */
    GT_RefreshWindow(win_data->window, NULL);
    
    log_info(LOG_GUI, "Preferences reset to defaults\n");
}

/**
 * handle_main_save_menu - Handle "Save" menu selection
 * 
 * Saves preferences to last saved file path.
 * If no previous save path exists, calls handle_main_save_as_menu instead.
 * 
 * @param win_data Pointer to main window data
 */
static void handle_main_save_menu(struct iTidyMainWindow *win_data)
{
    LayoutPreferences *prefs;
    
    if (!win_data || !win_data->window)
        return;
    
    /* Get mutable access to global preferences */
    prefs = (LayoutPreferences *)GetGlobalPreferences();
    if (!prefs)
    {
        ShowEasyRequest(win_data->window,
            "Error",
            "Failed to get preferences.",
            "OK");
        return;
    }
    
    /* If no last save path, call Save As instead */
    if (win_data->last_save_path[0] == '\0')
    {
        log_debug(LOG_GUI, "Menu: Save clicked but no previous save path - calling Save As\n");
        handle_main_save_as_menu(win_data);
        return;
    }
    
    /* Ensure save directories exist before saving */
    if (!ensure_save_directories())
    {
        log_error(LOG_GUI, "Failed to create save directories\n");
        ShowEasyRequest(win_data->window,
            "Save Failed",
            "Could not create required directories.\nCheck permissions and disk space.",
            "OK");
        return;
    }
    
    /* Update global preferences from current GUI state */
    sync_gui_to_preferences(win_data, prefs);
    UpdateGlobalPreferences(prefs);
    
    log_info(LOG_GUI, "Menu: Save clicked - saving to %s\n", win_data->last_save_path);
    
    /* Save the file */
    if (save_preferences_to_file(win_data->last_save_path, prefs))
    {
        log_info(LOG_GUI, "Preferences saved to: %s\n", win_data->last_save_path);
    }
    else
    {
        ShowEasyRequest(win_data->window,
            "Save Failed",
            "Failed to save preferences file.",
            "OK");
    }
}

/**
 * handle_main_save_as_menu - Handle "Save as..." menu selection
 * 
 * Opens ASL file requester, checks for overwrite, and saves preferences.
 * 
 * @param win_data Pointer to main window data
 */
static void handle_main_save_as_menu(struct iTidyMainWindow *win_data)
{
    struct FileRequester *freq;
    char full_path[512];
    char expanded_drawer[512];
    BPTR lock;
    LayoutPreferences *prefs;
    
    if (!win_data || !win_data->window)
        return;
    
    /* Get mutable access to global preferences */
    prefs = (LayoutPreferences *)GetGlobalPreferences();
    if (!prefs)
    {
        ShowEasyRequest(win_data->window,
            "Error",
            "Failed to get preferences.",
            "OK");
        return;
    }
    
    /* Ensure save directories exist before opening requester */
    if (!ensure_save_directories())
    {
        log_error(LOG_GUI, "Failed to create save directories\n");
        ShowEasyRequest(win_data->window,
            "Save Failed",
            "Could not create required directories.\nCheck permissions and disk space.",
            "OK");
        return;
    }
    
    /* Update global preferences from current GUI state */
    sync_gui_to_preferences(win_data, prefs);
    UpdateGlobalPreferences(prefs);
    
    /* Expand PROGDIR: to actual path for ASL requester */
    if (!ExpandProgDir("PROGDIR:userdata/Settings", expanded_drawer, sizeof(expanded_drawer)))
    {
        log_warning(LOG_GUI, "Could not expand PROGDIR:, using fallback path\n");
        strcpy(expanded_drawer, "RAM:");
    }
    
    log_debug(LOG_GUI, "Using initial drawer: %s\n", expanded_drawer);
    
    /* Allocate file requester */
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Save Preferences As...",
        ASLFR_InitialDrawer, expanded_drawer,
        ASLFR_InitialFile, "iTidy.prefs",
        ASLFR_DoSaveMode, TRUE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, win_data->window,
        TAG_END);
    
    if (!freq)
    {
        log_error(LOG_GUI, "Failed to allocate file requester\n");
        ShowEasyRequest(win_data->window,
            "Error",
            "Could not open file requester.",
            "OK");
        return;
    }
    
    /* Display requester */
    if (AslRequest(freq, NULL))
    {
        /* Build full path */
        strcpy(full_path, freq->fr_Drawer);
        if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            log_error(LOG_GUI, "Path too long: %s + %s\n", freq->fr_Drawer, freq->fr_File);
            FreeAslRequest(freq);
            ShowEasyRequest(win_data->window,
                "Error",
                "File path is too long.",
                "OK");
            return;
        }
        
        log_info(LOG_GUI, "User selected save path: %s\n", full_path);
        
        /* Check if file exists */
        lock = Lock((STRPTR)full_path, ACCESS_READ);
        if (lock)
        {
            UnLock(lock);
            
            /* File exists - ask for confirmation */
            if (!ShowEasyRequest(win_data->window,
                "File Exists",
                "File already exists.\\nDo you want to replace it?",
                "Replace|Cancel"))
            {
                log_info(LOG_GUI, "User cancelled overwrite\n");
                FreeAslRequest(freq);
                return;
            }
        }
        
        /* Save the file */
        if (save_preferences_to_file(full_path, prefs))
        {
            /* Store the save path for future Save operations */
            strncpy(win_data->last_save_path, full_path, sizeof(win_data->last_save_path) - 1);
            win_data->last_save_path[sizeof(win_data->last_save_path) - 1] = '\0';
            
            ShowEasyRequest(win_data->window,
                "Save Successful",
                "Preferences saved successfully.",
                "OK");
            log_info(LOG_GUI, "Preferences saved to: %s\n", full_path);
        }
        else
        {
            ShowEasyRequest(win_data->window,
                "Save Failed",
                "Failed to save preferences file.",
                "OK");
        }
    }
    else
    {
        log_info(LOG_GUI, "User cancelled save operation\n");
    }
    
    FreeAslRequest(freq);
}

/**
 * handle_main_open_menu - Handle "Open..." menu selection
 * 
 * Opens ASL file requester, loads preferences, updates GUI.
 * 
 * @param win_data Pointer to main window data
 */
static void handle_main_open_menu(struct iTidyMainWindow *win_data)
{
    struct FileRequester *freq;
    char full_path[512];
    char expanded_drawer[512];
    BPTR lock;
    LayoutPreferences loaded_prefs;
    
    if (!win_data || !win_data->window)
        return;
    
    log_debug(LOG_GUI, "Menu: Open... clicked\n");
    
    /* Expand PROGDIR: to actual path for ASL requester */
    if (!ExpandProgDir("PROGDIR:userdata/Settings", expanded_drawer, sizeof(expanded_drawer)))
    {
        log_warning(LOG_GUI, "Could not expand PROGDIR:, using fallback path\n");
        strcpy(expanded_drawer, "RAM:");
    }
    
    log_debug(LOG_GUI, "Using initial drawer: %s\n", expanded_drawer);
    
    /* Allocate file requester */
    freq = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, "Open Preferences File...",
        ASLFR_InitialDrawer, expanded_drawer,
        ASLFR_InitialFile, "iTidy.prefs",
        ASLFR_DoSaveMode, FALSE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_Window, win_data->window,
        TAG_END);
    
    if (!freq)
    {
        log_error(LOG_GUI, "Failed to allocate file requester\n");
        ShowEasyRequest(win_data->window,
            "Error",
            "Could not open file requester.",
            "OK");
        return;
    }
    
    /* Display requester */
    if (AslRequest(freq, NULL))
    {
        /* Build full path */
        strcpy(full_path, freq->fr_Drawer);
        if (!AddPart((STRPTR)full_path, (STRPTR)freq->fr_File, sizeof(full_path)))
        {
            log_error(LOG_GUI, "Path too long: %s + %s\n", freq->fr_Drawer, freq->fr_File);
            FreeAslRequest(freq);
            ShowEasyRequest(win_data->window,
                "Error",
                "File path is too long.",
                "OK");
            return;
        }
        
        log_info(LOG_GUI, "User selected file: %s\n", full_path);
        
        /* Check if file exists */
        lock = Lock((STRPTR)full_path, ACCESS_READ);
        if (!lock)
        {
            log_error(LOG_GUI, "File does not exist: %s\n", full_path);
            FreeAslRequest(freq);
            ShowEasyRequest(win_data->window,
                "File Not Found",
                "The selected file does not exist.",
                "OK");
            return;
        }
        UnLock(lock);
        
        /* Load the file */
        if (load_preferences_from_file(full_path, &loaded_prefs))
        {
            /* Update global preferences */
            UpdateGlobalPreferences(&loaded_prefs);
            
            /* Sync GUI from loaded preferences */
            sync_gui_from_preferences(win_data, &loaded_prefs);
            
            /* Store as last save path */
            strncpy(win_data->last_save_path, full_path, sizeof(win_data->last_save_path) - 1);
            win_data->last_save_path[sizeof(win_data->last_save_path) - 1] = '\0';
            
            ShowEasyRequest(win_data->window,
                "Load Successful",
                "Preferences loaded successfully.",
                "OK");
            log_info(LOG_GUI, "Preferences loaded from: %s\n", full_path);
        }
        else
        {
            ShowEasyRequest(win_data->window,
                "Load Failed",
                "Failed to load preferences file.\\nFile may be corrupted or invalid.",
                "OK");
        }
    }
    else
    {
        log_info(LOG_GUI, "User cancelled load operation\n");
    }
    
    FreeAslRequest(freq);
}

/**
 * handle_main_about_menu - Show About requester with version info
 *
 * Displays a simple EasyRequest box with the current version string.
 *
 * @param win_data Pointer to main window data
 */
static void handle_main_about_menu(struct iTidyMainWindow *win_data)
{
    char about_text[160];

    if (!win_data || !win_data->window)
        return;

    snprintf(about_text, sizeof(about_text), "iTidy %s\n\nEnhanced Workbench icon cleanup tool.\nCompiled on %s at %s\n\nBy Kerry Thompson.\n", ITIDY_VERSION, __DATE__, __TIME__);

    ShowEasyRequest(win_data->window,
        "About iTidy",
        about_text,
        "OK");
}

/* End of main_window.c */
