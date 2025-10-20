/*
 * main_window.c - iTidy Main Window Implementation
 * Simple Intuition-based GUI for Workbench 3.0+
 * No MUI - Pure Intuition only
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <stdio.h>

#include "main_window.h"

/*------------------------------------------------------------------------*/
/* Window Constants                                                       */
/*------------------------------------------------------------------------*/
#define ITIDY_WINDOW_TITLE "iTidy - Icon Cleanup Tool"
#define ITIDY_WINDOW_WIDTH 400
#define ITIDY_WINDOW_HEIGHT 200
#define ITIDY_WINDOW_LEFT 100
#define ITIDY_WINDOW_TOP 50

/*------------------------------------------------------------------------*/
/**
 * @brief Open the iTidy main window on Workbench screen
 *
 * Creates a simple window with close gadget on the Workbench screen.
 * The window is centered and has standard Intuition gadgets.
 *
 * @param win_data Pointer to window data structure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
/*------------------------------------------------------------------------*/
BOOL open_itidy_main_window(struct iTidyMainWindow *win_data)
{
    /* Validate input */
    if (win_data == NULL)
    {
        printf("ERROR: Invalid window data structure\n");
        return FALSE;
    }

    /* Initialize structure */
    win_data->screen = NULL;
    win_data->window = NULL;
    win_data->window_open = FALSE;

    /* Lock the Workbench screen */
    win_data->screen = LockPubScreen(NULL);
    if (win_data->screen == NULL)
    {
        printf("ERROR: Could not lock Workbench screen\n");
        return FALSE;
    }

    printf("Opening iTidy main window...\n");

    /* Open the window on Workbench screen */
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
        WA_IDCMP, IDCMP_CLOSEWINDOW,
        TAG_END);

    if (win_data->window == NULL)
    {
        printf("ERROR: Could not open window\n");
        UnlockPubScreen(NULL, win_data->screen);
        win_data->screen = NULL;
        return FALSE;
    }

    /* Mark window as successfully opened */
    win_data->window_open = TRUE;

    printf("iTidy main window opened successfully\n");
    printf("Window dimensions: %dx%d at position (%d,%d)\n",
           ITIDY_WINDOW_WIDTH, ITIDY_WINDOW_HEIGHT,
           ITIDY_WINDOW_LEFT, ITIDY_WINDOW_TOP);

    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Close the iTidy main window and cleanup resources
 *
 * Properly closes the window and unlocks the Workbench screen.
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
 * Processes Intuition messages. Currently only handles IDCMP_CLOSEWINDOW.
 * This function should be called in a loop until it returns FALSE.
 *
 * @param win_data Pointer to window data structure
 * @return BOOL TRUE to continue, FALSE to quit
 */
/*------------------------------------------------------------------------*/
BOOL handle_itidy_window_events(struct iTidyMainWindow *win_data)
{
    struct IntuiMessage *msg;
    ULONG msg_class;
    BOOL continue_running = TRUE;

    /* Validate input */
    if (win_data == NULL || win_data->window == NULL)
    {
        return FALSE;
    }

    /* Wait for a message */
    WaitPort(win_data->window->UserPort);

    /* Process all pending messages */
    while ((msg = (struct IntuiMessage *)GetMsg(win_data->window->UserPort)))
    {
        /* Get message class before replying */
        msg_class = msg->Class;

        /* Reply to the message */
        ReplyMsg((struct Message *)msg);

        /* Handle the message */
        switch (msg_class)
        {
            case IDCMP_CLOSEWINDOW:
                printf("Close gadget clicked - shutting down\n");
                continue_running = FALSE;
                break;

            default:
                /* Unknown message - ignore */
                break;
        }
    }

    return continue_running;
}

/* End of main_window.c */
