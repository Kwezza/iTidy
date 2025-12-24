/*
 * easy_request_helper.h - Global EasyRequest Helper for iTidy
 * 
 * Provides a reusable wrapper around AmigaOS Intuition's EasyRequest()
 * that ensures requesters always open on the same screen as the parent
 * window. This fixes RTG "top-left requester" placement issues.
 * 
 * Features:
 * - Always attaches to parent window's screen
 * - Optional compile-time support for centered placement
 * - Clean error handling and resource cleanup
 * - Debug logging support
 * 
 * Usage:
 *   BOOL result = ShowEasyRequest(myWindow, 
 *                                 "Confirm Action",
 *                                 "Do you want to proceed?",
 *                                 "Yes|No");
 * 
 * Compile-time options:
 *   BUILD_WITH_MOVEWINDOW - Enable experimental centering using
 *                           BuildEasyRequest() + MoveWindow()
 */

#ifndef EASY_REQUEST_HELPER_H
#define EASY_REQUEST_HELPER_H

#include <exec/types.h>
#include <intuition/intuition.h>

/*------------------------------------------------------------------------*/
/**
 * @brief Show an EasyRequest dialog on the parent window's screen
 * 
 * Opens a simple requester dialog that is guaranteed to appear on the
 * same screen as the parent window. This ensures proper behavior on
 * RTG screens and multi-monitor setups.
 * 
 * @param parentWin Parent window (must not be NULL)
 * @param title Title bar text for the requester
 * @param body Main message text (may contain newlines with \n)
 * @param gadgets Gadget labels separated by | (e.g., "OK|Cancel")
 * 
 * @return TRUE if user clicked first gadget (leftmost), FALSE otherwise
 * 
 * Example:
 *   if (ShowEasyRequest(window, "Confirm", "Delete file?", "Delete|Cancel"))
 *   {
 *       // User clicked "Delete"
 *   }
 * 
 * Notes:
 * - Returns FALSE if parentWin is NULL or any other error occurs
 * - If BUILD_WITH_MOVEWINDOW is defined, attempts to center requester
 * - All memory and resources are cleaned up before returning
 */
/*------------------------------------------------------------------------*/
BOOL ShowEasyRequest(struct Window *parentWin,
                     CONST_STRPTR title,
                     CONST_STRPTR body,
                     CONST_STRPTR gadgets);

#endif /* EASY_REQUEST_HELPER_H */
