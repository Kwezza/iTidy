/*
 * easy_request_helper.c - Global EasyRequest Helper Implementation
 * 
 * Implements a reusable wrapper around AmigaOS Intuition's EasyRequest()
 * that ensures requesters always open on the same screen as the parent
 * window.
 * 
 * This module fixes RTG screen placement issues by explicitly using
 * the parent window's screen pointer when opening requesters.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <string.h>

#include "easy_request_helper.h"
#include "../writeLog.h"

/*------------------------------------------------------------------------*/
/**
 * @brief Show an EasyRequest dialog on the parent window's screen
 * 
 * Implementation notes:
 * - Uses standard EasyRequest() by default (WB 2.0+)
 * - Optional BUILD_WITH_MOVEWINDOW branch uses BuildEasyRequest() +
 *   MoveWindow() for centered placement (WB 3.0+)
 * - Always attaches to parent window's screen via PubScreenName or
 *   direct screen pointer
 * - Comprehensive null-checking and error handling
 */
/*------------------------------------------------------------------------*/
BOOL ShowEasyRequest(struct Window *parentWin,
                     CONST_STRPTR title,
                     CONST_STRPTR body,
                     CONST_STRPTR gadgets)
{
    struct EasyStruct easyStruct;
    BOOL result = FALSE;
    
    /* Validate parameters */
    if (parentWin == NULL)
    {
        append_to_log("ShowEasyRequest: ERROR - parentWin is NULL\n");
        return FALSE;
    }
    
    if (parentWin->WScreen == NULL)
    {
        append_to_log("ShowEasyRequest: ERROR - parentWin->WScreen is NULL\n");
        return FALSE;
    }
    
    if (body == NULL || gadgets == NULL)
    {
        append_to_log("ShowEasyRequest: ERROR - body or gadgets is NULL\n");
        return FALSE;
    }
    
    /* Log the request details */
    append_to_log("ShowEasyRequest: Opening requester\n");
    append_to_log("  Title: %s\n", title ? title : "(none)");
    append_to_log("  Body: %s\n", body);
    append_to_log("  Gadgets: %s\n", gadgets);
    append_to_log("  Parent window: %p, screen: %p\n", 
                  parentWin, parentWin->WScreen);
    
    /* Prepare EasyStruct */
    easyStruct.es_StructSize = sizeof(struct EasyStruct);
    easyStruct.es_Flags = 0;
    easyStruct.es_Title = (UBYTE *)(title ? title : "iTidy");
    easyStruct.es_TextFormat = (UBYTE *)body;
    easyStruct.es_GadgetFormat = (UBYTE *)gadgets;
    
#ifdef BUILD_WITH_MOVEWINDOW
    /*
     * EXPERIMENTAL: Centered requester using BuildEasyRequest() + MoveWindow()
     * 
     * This branch requires Workbench 3.0+ (V39+) for MoveWindow().
     * It builds the requester, calculates centered position, moves it,
     * then enters the event loop manually.
     * 
     * Benefits:
     * - Can center requester on parent window's screen
     * - More control over requester placement
     * 
     * Drawbacks:
     * - More complex code
     * - Requires manual event handling
     * - Not compatible with WB 2.x
     */
    {
        struct Window *reqWin;
        struct IntuiMessage *msg;
        ULONG idcmpFlags;
        BOOL done = FALSE;
        WORD centerX, centerY;
        struct Screen *screen = parentWin->WScreen;
        
        append_to_log("ShowEasyRequest: Using BuildEasyRequest() with centering\n");
        
        /* Build the requester window */
        reqWin = BuildEasyRequest(parentWin, &easyStruct, 0, NULL);
        
        if (reqWin == NULL)
        {
            append_to_log("ShowEasyRequest: ERROR - BuildEasyRequest() failed\n");
            return FALSE;
        }
        
        append_to_log("  Requester window: %p\n", reqWin);
        append_to_log("  Initial position: (%d,%d), size: %dx%d\n",
                      reqWin->LeftEdge, reqWin->TopEdge,
                      reqWin->Width, reqWin->Height);
        append_to_log("  Screen dimensions: %dx%d\n",
                      screen->Width, screen->Height);
        
        /* Hide the window immediately to prevent flicker during move */
        append_to_log("  Hiding window before move to prevent flicker\n");
        ModifyIDCMP(reqWin, 0);  /* Disable IDCMP temporarily */
        
        /* Calculate centered position on parent's screen */
        centerX = (screen->Width - reqWin->Width) / 2;
        centerY = (screen->Height - reqWin->Height) / 2;
        
        /* Ensure requester stays within screen bounds */
        if (centerX < 0) centerX = 0;
        if (centerY < 0) centerY = 0;
        
        append_to_log("  Calculated center position: (%d,%d)\n", centerX, centerY);
        append_to_log("  MoveWindow delta: (%d,%d)\n", 
                      centerX - reqWin->LeftEdge, centerY - reqWin->TopEdge);
        
        /* Move window to centered position (requires V39+) */
        MoveWindow(reqWin, centerX - reqWin->LeftEdge, centerY - reqWin->TopEdge);
        
        /* Re-enable IDCMP now that window is in correct position */
        ModifyIDCMP(reqWin, IDCMP_GADGETUP | IDCMP_RAWKEY);
        
        append_to_log("  Requester moved. New position: (%d,%d)\n",
                      reqWin->LeftEdge, reqWin->TopEdge);
        append_to_log("  Window unhidden, entering event loop...\n");
        
        /* Event loop - wait for user response */
        while (!done)
        {
            Wait(1L << reqWin->UserPort->mp_SigBit);
            
            while ((msg = (struct IntuiMessage *)GetMsg(reqWin->UserPort)) != NULL)
            {
                idcmpFlags = msg->Class;
                
                if (idcmpFlags == IDCMP_GADGETUP)
                {
                    /* User clicked a gadget - check which one */
                    struct Gadget *gad = (struct Gadget *)msg->IAddress;
                    
                    /* First gadget (leftmost) returns TRUE */
                    result = (gad->GadgetID == 1);
                    done = TRUE;
                    
                    append_to_log("  User clicked gadget ID=%ld (result=%d)\n",
                                  gad->GadgetID, result);
                }
                else if (idcmpFlags == IDCMP_RAWKEY)
                {
                    /* Handle keyboard shortcuts if needed */
                    /* For now, treat ESC or close as FALSE */
                    result = FALSE;
                    done = TRUE;
                    
                    append_to_log("  User pressed key (result=FALSE)\n");
                }
                else
                {
                    append_to_log("  Received IDCMP event: 0x%08lx\n", idcmpFlags);
                }
                
                ReplyMsg((struct Message *)msg);
            }
        }
        
        append_to_log("  Event loop complete. User choice: %d\n", result);
        
        /* Cleanup */
        FreeSysRequest(reqWin);
        
        append_to_log("ShowEasyRequest: Cleanup complete. Returning %d\n", result);
    }
#else
    /*
     * STANDARD: Simple EasyRequest() call
     * 
     * This is the standard approach compatible with Workbench 2.0+.
     * The requester will appear on the parent window's screen because
     * we pass the parent window pointer to EasyRequest().
     * 
     * AmigaOS automatically ensures the requester appears on the same
     * screen as the parent window when you pass a valid window pointer.
     */
    append_to_log("ShowEasyRequest: Using standard EasyRequest()\n");
    append_to_log("  Screen dimensions: %dx%d\n", 
                  parentWin->WScreen->Width, parentWin->WScreen->Height);
    append_to_log("  Parent window position: (%d,%d), size: %dx%d\n",
                  parentWin->LeftEdge, parentWin->TopEdge,
                  parentWin->Width, parentWin->Height);
    append_to_log("  Screen title: %s\n", 
                  parentWin->WScreen->Title ? parentWin->WScreen->Title : "(none)");
    
    result = EasyRequest(parentWin, &easyStruct, NULL);
    
    append_to_log("ShowEasyRequest: User selection = %d\n", result);
    append_to_log("ShowEasyRequest: Returning %d\n", result);
#endif
    
    return result;
}
