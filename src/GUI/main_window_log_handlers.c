/*
 * main_window_log_handlers.c - Log-related menu handlers for iTidy main window
 *
 * Contains the "Open Log Folder" menu handler implementation.
 * Extracted from main_window.c to keep that file within 68000 branch range.
 *
 * Target: AmigaOS / Workbench 3.2+ (VBCC, 68000+)
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <dos/dos.h>
#include <workbench/workbench.h>
#include <clib/wb_protos.h>
#include <classes/requester.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/requester.h>
#include <clib/alib_protos.h>

#include <string.h>
#include <stdio.h>

#include "writeLog.h"

/*------------------------------------------------------------------------*/
/* External References                                                   */
/*------------------------------------------------------------------------*/

/* RequesterBase is opened by main_window.c and kept open for app lifetime */
extern struct Library *RequesterBase;

/* ShowReActionRequester is defined in main_window.c (non-static) */
extern ULONG ShowReActionRequester(struct Window *parent_window,
                                   CONST_STRPTR title,
                                   CONST_STRPTR body,
                                   CONST_STRPTR gadgets,
                                   ULONG image_type);

/*------------------------------------------------------------------------*/
/* Log Folder Handlers                                                   */
/*------------------------------------------------------------------------*/

/**
 * Open PROGDIR:logs/ as a Workbench drawer window.
 * Uses workbench.library OpenWorkbenchObject() (WB 3.0+).
 */
void handle_menu_open_log_folder(struct Window *parent)
{
    struct Library *WorkbenchBase;
    BPTR lock;
    char abs_path[512];

    /* Resolve PROGDIR:logs/ to an absolute path.
     * OpenWorkbenchObject() does not understand process-local assignments
     * like PROGDIR:, so we must use NameFromLock() to get the real path. */
    lock = Lock("PROGDIR:logs/", ACCESS_READ);
    if (!lock)
    {
        /* Folder may not exist yet - try to create it so future log writes work */
        BPTR created = CreateDir("PROGDIR:logs");
        if (created)
        {
            UnLock(created);
            lock = Lock("PROGDIR:logs/", ACCESS_READ);
        }
    }

    if (!lock)
    {
        ShowReActionRequester(parent,
            "Open Log Folder",
            "Could not find or create the logs folder.\n"
            "Logs are stored in PROGDIR:logs/",
            "_OK",
            REQIMAGE_WARNING);
        return;
    }

    abs_path[0] = '\0';
    if (!NameFromLock(lock, abs_path, (LONG)sizeof(abs_path)))
    {
        /* NameFromLock failed - fall back to the PROGDIR: form */
        strncpy(abs_path, "PROGDIR:logs/", sizeof(abs_path) - 1);
        abs_path[sizeof(abs_path) - 1] = '\0';
    }
    UnLock(lock);

    WorkbenchBase = OpenLibrary("workbench.library", 36L);
    if (WorkbenchBase)
    {
        if (!OpenWorkbenchObject(abs_path, TAG_DONE))
        {
            char msg[600];
            snprintf(msg, sizeof(msg),
                "Could not open the logs folder.\nPath: %s", abs_path);
            ShowReActionRequester(parent,
                "Open Log Folder",
                msg,
                "_OK",
                REQIMAGE_WARNING);
        }
        CloseLibrary(WorkbenchBase);
    }
    else
    {
        ShowReActionRequester(parent,
            "Open Log Folder",
            "Could not open workbench.library.\n"
            "Logs are stored in PROGDIR:logs/",
            "_OK",
            REQIMAGE_INFO);
    }
}


