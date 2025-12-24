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
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <string.h>
#include <stdio.h>

#include "main_window.h"
#include "version_info.h"

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define ITIDY_WINDOW_TITLE "iTidy v" ITIDY_VERSION " - Icon Cleanup Tool"
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
    
    current_y += font_height + 30;
    
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
        GA_Disabled, TRUE,  /* Disabled for Phase 2 */
        TAG_END);
    if (!gad)
    {
        printf("ERROR: Failed to create advanced button\n");
        return FALSE;
    }
    
    /*====================================================================*/
    /* APPLY BUTTON                                                       */
    /*====================================================================*/
    ng.ng_LeftEdge = 200;
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
    ng.ng_LeftEdge = 320;
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

    /* Initialize structure */
    memset(win_data, 0, sizeof(struct iTidyMainWindow));
    strcpy(win_data->folder_path_buffer, "DH0:");
    
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
                        /* TODO: Open ASL file requester */
                        break;

                    case GID_APPLY:
                        printf("Apply button clicked\n");
                        printf("  Folder: %s\n", win_data->folder_path_buffer);
                        printf("  Preset: %d\n", win_data->preset_selected);
                        printf("  Recursive: %s\n", win_data->recursive_subdirs ? "Yes" : "No");
                        /* TODO: Call ProcessDirectory() with settings */
                        break;

                    case GID_CANCEL:
                        printf("Cancel button clicked - closing window\n");
                        continue_running = FALSE;
                        break;

                    case GID_ADVANCED:
                        printf("Advanced button clicked\n");
                        /* TODO: Open advanced settings window */
                        break;

                    case GID_FOLDER_PATH:
                        printf("Folder path changed: %s\n", win_data->folder_path_buffer);
                        break;

                    case GID_PRESET:
                        GT_GetGadgetAttrs(gad, win_data->window, NULL,
                            GTCY_Active, &win_data->preset_selected,
                            TAG_END);
                        printf("Preset changed to: %s\n", preset_labels[win_data->preset_selected]);
                        break;

                    case GID_LAYOUT:
                        GT_GetGadgetAttrs(gad, win_data->window, NULL,
                            GTCY_Active, &win_data->layout_selected,
                            TAG_END);
                        printf("Layout changed to: %s\n", layout_labels[win_data->layout_selected]);
                        break;

                    case GID_SORT:
                        GT_GetGadgetAttrs(gad, win_data->window, NULL,
                            GTCY_Active, &win_data->sort_selected,
                            TAG_END);
                        printf("Sort changed to: %s\n", sort_labels[win_data->sort_selected]);
                        break;

                    case GID_ORDER:
                        GT_GetGadgetAttrs(gad, win_data->window, NULL,
                            GTCY_Active, &win_data->order_selected,
                            TAG_END);
                        printf("Order changed to: %s\n", order_labels[win_data->order_selected]);
                        break;

                    case GID_SORTBY:
                        GT_GetGadgetAttrs(gad, win_data->window, NULL,
                            GTCY_Active, &win_data->sortby_selected,
                            TAG_END);
                        printf("Sort by changed to: %s\n", sortby_labels[win_data->sortby_selected]);
                        break;

                    case GID_CENTER_ICONS:
                        GT_GetGadgetAttrs(gad, win_data->window, NULL,
                            GTCB_Checked, &win_data->center_icons,
                            TAG_END);
                        printf("Center icons: %s\n", win_data->center_icons ? "ON" : "OFF");
                        break;

                    case GID_OPTIMIZE_COLS:
                        GT_GetGadgetAttrs(gad, win_data->window, NULL,
                            GTCB_Checked, &win_data->optimize_columns,
                            TAG_END);
                        printf("Optimize columns: %s\n", win_data->optimize_columns ? "ON" : "OFF");
                        break;

                    case GID_RECURSIVE:
                        GT_GetGadgetAttrs(gad, win_data->window, NULL,
                            GTCB_Checked, &win_data->recursive_subdirs,
                            TAG_END);
                        printf("Recursive subfolders: %s\n", win_data->recursive_subdirs ? "ON" : "OFF");
                        break;

                    case GID_BACKUP:
                        GT_GetGadgetAttrs(gad, win_data->window, NULL,
                            GTCB_Checked, &win_data->enable_backup,
                            TAG_END);
                        printf("Backup: %s\n", win_data->enable_backup ? "ON" : "OFF");
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
