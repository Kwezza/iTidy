/*------------------------------------------------------------------------*/
/*                                                                        *
 * test_window_template.c - Standalone test for amiga_window_template    *
 * Version compatible with vbcc +aos68k and Workbench 3.x                *
 *                                                                        *
 * This is a completely self-contained test program with no external     *
 * dependencies beyond standard Amiga OS 3.x libraries.                  *
 *                                                                        *
 * IMPORTANT: This program properly handles Workbench vs CLI launches:   *
 * - When launched from CLI: Uses existing console for printf() output   *
 * - When launched from Workbench: Opens its own console window          *
 *                                                                        */
/*------------------------------------------------------------------------*/

#include <exec/types.h>
#include <intuition/intuition.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#include "amiga_window_template.h"

/*------------------------------------------------------------------------*/
/* Library base pointers                                                  */
/*------------------------------------------------------------------------*/
struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *GadToolsBase = NULL;

/*------------------------------------------------------------------------*/
/* Console handling for Workbench launches                                */
/*------------------------------------------------------------------------*/
static BPTR console_handle = 0;

/*------------------------------------------------------------------------*/
/**
 * @brief Main test program for the window template
 * 
 * Handles both CLI and Workbench launches correctly:
 * - argc > 0: Launched from CLI (console already exists)
 * - argc == 0: Launched from Workbench (must open console for printf)
 */
/*------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    struct TemplateWindowData data;
    struct Screen *screen = NULL;
    BOOL running = TRUE;
    struct IntuiMessage *imsg;
    ULONG class;
    UWORD code;
    struct Gadget *gadget;

    /* IMPORTANT: When launched from Workbench, argc==0 and there is no console.
     * We must open a console window for printf() output to work.
     * See amiga_gui_research_3x.md lines 410-434 for details.
     */
    if (argc == 0)
    {
        /* Workbench launch - open a console for debug output */
        console_handle = Open("CON:10/30/620/180/Window Template Debug Output/AUTO/CLOSE/WAIT", MODE_NEWFILE);
        if (console_handle)
        {
            /* Redirect stdout to our console window */
            SelectOutput(console_handle);
        }
    }

    printf("=== Window Template Test Program ===\n");
    printf("Launch mode: %s\n", argc == 0 ? "Workbench" : "CLI");
    printf("\n");

    /* Open required libraries */
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37L);
    if (IntuitionBase == NULL)
    {
        printf("Failed to open intuition.library v37+\n");
        return 20;
    }

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37L);
    if (GfxBase == NULL)
    {
        printf("Failed to open graphics.library v37+\n");
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }

    GadToolsBase = OpenLibrary("gadtools.library", 37L);
    if (GadToolsBase == NULL)
    {
        printf("Failed to open gadtools.library v37+\n");
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }

    /* Lock the default public screen */
    screen = LockPubScreen(NULL);
    if (screen == NULL)
    {
        printf("Failed to lock public screen\n");
        CloseLibrary(GadToolsBase);
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }

    /* Initialize window data structure */
    memset(&data, 0, sizeof(struct TemplateWindowData));
    data.screen = screen;
    data.window_title = "Window Template Test";

    /* Create the template window */
    printf("Creating template window...\n");
    if (!create_template_window(&data))
    {
        printf("Failed to create template window\n");
        UnlockPubScreen(NULL, screen);
        CloseLibrary(GadToolsBase);
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }

    printf("Window created successfully - entering event loop\n");

    /* Main event loop */
    while (running)
    {
        WaitPort(data.window->UserPort);

        while ((imsg = GT_GetIMsg(data.window->UserPort)) != NULL)
        {
            class = imsg->Class;
            code = imsg->Code;
            gadget = (struct Gadget *)imsg->IAddress;

            GT_ReplyIMsg(imsg);

            switch (class)
            {
                case IDCMP_CLOSEWINDOW:
                    printf("Close window requested\n");
                    running = FALSE;
                    break;

                case IDCMP_GADGETUP:
                    if (!handle_template_gadget_event(&data, gadget->GadgetID))
                    {
                        printf("Gadget event requested window close\n");
                        running = FALSE;
                    }
                    break;

                case IDCMP_NEWSIZE:
                    printf("Window resize event\n");
                    handle_template_window_resize(&data);
                    break;

                default:
                    break;
            }
        }
    }

    printf("Closing window...\n");

    /* Clean up */
    close_template_window(&data);

    UnlockPubScreen(NULL, screen);

    /* Close libraries */
    CloseLibrary(GadToolsBase);
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)IntuitionBase);

    printf("\nTest completed successfully\n");
    
    /* Close console if we opened one (Workbench launch) */
    if (console_handle)
    {
        printf("Press RETURN to close this window...\n");
        /* Wait for user to read the output */
        getchar();
        Close(console_handle);
    }
    
    return 0;
}
