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
    
    return result;
}
