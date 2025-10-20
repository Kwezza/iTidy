/*
 * test_main_window.c - Simple test program for iTidy main window
 * 
 * This program demonstrates opening the iTidy main window and handling
 * the close gadget. It's a minimal example showing how to integrate
 * the GUI into the main iTidy program.
 *
 * Compile with VBCC:
 *   vc +aos68k -c main_window.c
 *   vc +aos68k -c test_main_window.c
 *   vc +aos68k -o test_main_window test_main_window.o main_window.o -lauto
 */

#include <exec/types.h>
#include <stdio.h>

#include "main_window.h"

/* VBCC: Set stack size to 20KB at compile time */
#ifdef __AMIGA__
long __stack = 20000L;
#endif

/*------------------------------------------------------------------------*/
/**
 * @brief Main entry point for window test
 *
 * Opens the iTidy window, runs the event loop until the user clicks
 * the close gadget, then cleans up and exits.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return int 0 for success, 1 for failure
 */
/*------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    struct iTidyMainWindow win_data;
    BOOL keep_running;

    printf("\n");
    printf("==================================================\n");
    printf("   iTidy GUI Test - Simple Window Demo\n");
    printf("==================================================\n\n");

    /* Open the main window */
    if (!open_itidy_main_window(&win_data))
    {
        printf("Failed to open iTidy main window\n");
        return 1;
    }

    printf("\nWindow is now open. Click the close gadget to exit.\n\n");

    /* Main event loop - runs until user clicks close gadget */
    keep_running = TRUE;
    while (keep_running)
    {
        keep_running = handle_itidy_window_events(&win_data);
    }

    /* Cleanup and exit */
    close_itidy_main_window(&win_data);

    printf("\n");
    printf("==================================================\n");
    printf("   iTidy GUI Test Complete\n");
    printf("==================================================\n\n");

    return 0;
}

/* End of test_main_window.c */
