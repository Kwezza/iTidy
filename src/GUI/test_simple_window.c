/*
 * test_simple_window.c - Minimal test window to isolate MuForce errors
 * 
 * This is a bare-bones window implementation to test if the basic
 * Intuition window opening is causing the 7FFF0000 errors.
 */

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/dos.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <exec/memory.h>

#include "test_simple_window.h"
#include "../writeLog.h"

#include <string.h>
#include <stdio.h>

/* Gadget IDs */
#define GID_TEST_LISTVIEW   1
#define GID_TEST_CLOSE      2

/*------------------------------------------------------------------------*/
/**
 * @brief Open a simple test window
 */
/*------------------------------------------------------------------------*/
BOOL open_simple_test_window(struct SimpleTestWindow *test_data, UWORD run_number, const char *date_str)
{
    struct NewGadget ng;
    struct Gadget *gad;
    
    if (test_data == NULL || test_data->screen == NULL)
    {
        append_to_log("ERROR: Invalid test_data or screen\n");
        return FALSE;
    }
    
    append_to_log("=== SIMPLE TEST WINDOW - Opening ===\n");
    
    /* Set the title with run number - MUST be in the structure, not on stack! */
    sprintf(test_data->title, "Folder View - Run %u (%s)", 
            (unsigned int)run_number, 
            (date_str != NULL) ? date_str : "Unknown");
    
    append_to_log("Title set to: %s (address: %p)\n", test_data->title, test_data->title);
    append_to_log("Screen address: %p\n", test_data->screen);
    
    /* Initialize empty list for ListView */
    NewList(&test_data->folder_list);
    append_to_log("Initialized empty folder list\n");
    
    /* Add some test folder entries with tree formatting */
    {
        struct TestFolderEntry *entry;
        
        /* Root folder */
        entry = (struct TestFolderEntry *)AllocVec(sizeof(struct TestFolderEntry), MEMF_CLEAR);
        if (entry)
        {
            strcpy(entry->display_text, "Work:");
            entry->node.ln_Name = entry->display_text;
            AddTail(&test_data->folder_list, (struct Node *)entry);
        }
        
        /* Level 1 folders */
        entry = (struct TestFolderEntry *)AllocVec(sizeof(struct TestFolderEntry), MEMF_CLEAR);
        if (entry)
        {
            strcpy(entry->display_text, ":..Documents");
            entry->node.ln_Name = entry->display_text;
            AddTail(&test_data->folder_list, (struct Node *)entry);
        }
        
        entry = (struct TestFolderEntry *)AllocVec(sizeof(struct TestFolderEntry), MEMF_CLEAR);
        if (entry)
        {
            strcpy(entry->display_text, ":  :..Reports");
            entry->node.ln_Name = entry->display_text;
            AddTail(&test_data->folder_list, (struct Node *)entry);
        }
        
        entry = (struct TestFolderEntry *)AllocVec(sizeof(struct TestFolderEntry), MEMF_CLEAR);
        if (entry)
        {
            strcpy(entry->display_text, ":..Projects");
            entry->node.ln_Name = entry->display_text;
            AddTail(&test_data->folder_list, (struct Node *)entry);
        }
        
        entry = (struct TestFolderEntry *)AllocVec(sizeof(struct TestFolderEntry), MEMF_CLEAR);
        if (entry)
        {
            strcpy(entry->display_text, ":  :..ClientA");
            entry->node.ln_Name = entry->display_text;
            AddTail(&test_data->folder_list, (struct Node *)entry);
        }
        
        entry = (struct TestFolderEntry *)AllocVec(sizeof(struct TestFolderEntry), MEMF_CLEAR);
        if (entry)
        {
            strcpy(entry->display_text, ":  :..ClientB");
            entry->node.ln_Name = entry->display_text;
            AddTail(&test_data->folder_list, (struct Node *)entry);
        }
        
        append_to_log("Added test folder entries\n");
    }
    
    /* Get visual info for GadTools */
    test_data->visual_info = GetVisualInfo(test_data->screen, TAG_DONE);
    if (test_data->visual_info == NULL)
    {
        append_to_log("ERROR: Failed to get visual info\n");
        return FALSE;
    }
    append_to_log("Visual info: %p\n", test_data->visual_info);
    
    /* Create gadget context */
    gad = CreateContext(&test_data->glist);
    if (gad == NULL)
    {
        append_to_log("ERROR: Failed to create gadget context\n");
        FreeVisualInfo(test_data->visual_info);
        test_data->visual_info = NULL;
        return FALSE;
    }
    append_to_log("Gadget context created: %p\n", test_data->glist);
    
    /* Create ListView gadget (empty list for now) */
    ng.ng_LeftEdge = 10;
    ng.ng_TopEdge = 30;
    ng.ng_Width = 260;
    ng.ng_Height = 80;
    ng.ng_GadgetText = "Folders:";
    ng.ng_TextAttr = test_data->screen->Font;
    ng.ng_GadgetID = GID_TEST_LISTVIEW;
    ng.ng_Flags = PLACETEXT_ABOVE;
    ng.ng_VisualInfo = test_data->visual_info;
    
    gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, &test_data->folder_list,
        GTLV_ShowSelected, NULL,
        TAG_DONE);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Failed to create ListView\n");
        FreeGadgets(test_data->glist);
        test_data->glist = NULL;
        FreeVisualInfo(test_data->visual_info);
        test_data->visual_info = NULL;
        return FALSE;
    }
    test_data->listview_gad = gad;
    append_to_log("ListView created: %p\n", gad);
    
    /* Initialize NewGadget structure for Close button */
    ng.ng_LeftEdge = 10;
    ng.ng_TopEdge = 120;
    ng.ng_Width = 100;
    ng.ng_Height = 20;
    ng.ng_GadgetText = "Close";
    ng.ng_TextAttr = test_data->screen->Font;
    ng.ng_GadgetID = GID_TEST_CLOSE;
    ng.ng_Flags = 0;
    ng.ng_VisualInfo = test_data->visual_info;
    
    /* Create Close button */
    gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_DONE);
    if (gad == NULL)
    {
        append_to_log("ERROR: Failed to create Close button\n");
        FreeGadgets(test_data->glist);
        test_data->glist = NULL;
        FreeVisualInfo(test_data->visual_info);
        test_data->visual_info = NULL;
        return FALSE;
    }
    test_data->close_button = gad;
    append_to_log("Close button created: %p\n", gad);
    
    /* Open the window */
    test_data->window = OpenWindowTags(NULL,
        WA_Left, 100,
        WA_Top, 100,
        WA_Width, 300,
        WA_Height, 150,
        WA_Title, test_data->title,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, test_data->screen,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP,
        WA_Gadgets, test_data->glist,
        TAG_DONE);
    
    if (test_data->window == NULL)
    {
        append_to_log("ERROR: Failed to open test window\n");
        FreeGadgets(test_data->glist);
        test_data->glist = NULL;
        FreeVisualInfo(test_data->visual_info);
        test_data->visual_info = NULL;
        return FALSE;
    }
    
    append_to_log("Test window opened successfully at %p\n", test_data->window);
    append_to_log("Window UserPort: %p\n", test_data->window->UserPort);
    
    /* Refresh gadgets */
    GT_RefreshWindow(test_data->window, NULL);
    append_to_log("Gadgets refreshed\n");
    
    test_data->window_open = TRUE;
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Handle events for the test window
 */
/*------------------------------------------------------------------------*/
void handle_simple_test_events(struct SimpleTestWindow *test_data)
{
    struct IntuiMessage *msg;
    ULONG msg_class;
    UWORD gadget_id;
    BOOL done = FALSE;
    
    if (test_data == NULL || test_data->window == NULL)
    {
        return;
    }
    
    append_to_log("=== Entering event loop ===\n");
    
    while (!done)
    {
        /* Wait for messages */
        WaitPort(test_data->window->UserPort);
        
        while ((msg = GT_GetIMsg(test_data->window->UserPort)) != NULL)
        {
            msg_class = msg->Class;
            gadget_id = ((struct Gadget *)msg->IAddress)->GadgetID;
            
            append_to_log("Received message, class: 0x%08lx\n", msg_class);
            
            GT_ReplyIMsg(msg);
            
            switch (msg_class)
            {
                case IDCMP_CLOSEWINDOW:
                    append_to_log("Close window gadget clicked\n");
                    done = TRUE;
                    break;
                    
                case IDCMP_GADGETUP:
                    append_to_log("Gadget clicked, ID: %d\n", gadget_id);
                    if (gadget_id == GID_TEST_CLOSE)
                    {
                        append_to_log("Close button clicked\n");
                        done = TRUE;
                    }
                    break;
            }
        }
    }
    
    append_to_log("=== Exiting event loop ===\n");
}

/*------------------------------------------------------------------------*/
/**
 * @brief Close the test window
 */
/*------------------------------------------------------------------------*/
void close_simple_test_window(struct SimpleTestWindow *test_data)
{
    struct IntuiMessage *msg;
    
    if (test_data == NULL)
    {
        return;
    }
    
    append_to_log("=== SIMPLE TEST WINDOW - Closing ===\n");
    
    if (test_data->window != NULL)
    {
        append_to_log("Window pointer: %p\n", test_data->window);
        append_to_log("Window UserPort: %p\n", test_data->window->UserPort);
        
        /* Flush any remaining messages */
        if (test_data->window->UserPort != NULL)
        {
            append_to_log("Flushing messages...\n");
            while ((msg = GT_GetIMsg(test_data->window->UserPort)) != NULL)
            {
                GT_ReplyIMsg(msg);
            }
            append_to_log("Messages flushed\n");
        }
        
        append_to_log("About to call CloseWindow...\n");
        CloseWindow(test_data->window);
        append_to_log("CloseWindow returned\n");
        
        test_data->window = NULL;
        test_data->window_open = FALSE;
    }
    
    /* Free gadgets */
    if (test_data->glist != NULL)
    {
        append_to_log("Freeing gadgets...\n");
        FreeGadgets(test_data->glist);
        test_data->glist = NULL;
        test_data->close_button = NULL;
        test_data->listview_gad = NULL;
        append_to_log("Gadgets freed\n");
    }
    
    /* Free folder list entries */
    {
        struct TestFolderEntry *entry;
        struct TestFolderEntry *next_entry;
        
        append_to_log("Freeing folder list entries...\n");
        entry = (struct TestFolderEntry *)test_data->folder_list.lh_Head;
        while (entry->node.ln_Succ != NULL)
        {
            next_entry = (struct TestFolderEntry *)entry->node.ln_Succ;
            Remove((struct Node *)entry);
            FreeVec(entry);
            entry = next_entry;
        }
        append_to_log("Folder list entries freed\n");
    }
    
    /* Free visual info */
    if (test_data->visual_info != NULL)
    {
        append_to_log("Freeing visual info...\n");
        FreeVisualInfo(test_data->visual_info);
        test_data->visual_info = NULL;
        append_to_log("Visual info freed\n");
    }
    
    append_to_log("=== Test window closed ===\n");
}
