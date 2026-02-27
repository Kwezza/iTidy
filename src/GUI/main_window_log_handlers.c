/*
 * main_window_log_handlers.c - Log-related menu handlers for iTidy main window
 *
 * Contains "Open Log Folder" and "Archive Logs To RAM" implementations.
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

    WorkbenchBase = OpenLibrary("workbench.library", 36L);
    if (WorkbenchBase)
    {
        if (!OpenWorkbenchObject("PROGDIR:logs/", TAG_DONE))
        {
            ShowReActionRequester(parent,
                "Open Log Folder",
                "Could not open the logs folder.\n"
                "Logs are stored in PROGDIR:logs/",
                "_OK",
                REQIMAGE_WARNING);
        }
        CloseLibrary(WorkbenchBase);
    }
    else
    {
        ShowReActionRequester(parent,
            "Open Log Folder",
            "Logs are stored in PROGDIR:logs/",
            "_OK",
            REQIMAGE_INFO);
    }
}

/**
 * Copy all files from PROGDIR:logs/ to RAM:iTidy_Logs/.
 * Creates the destination directory if it does not exist.
 * Shows a completion requester when done.
 */
void handle_menu_archive_logs(struct Window *parent)
{
    BPTR src_lock;
    struct FileInfoBlock *fib;
    BOOL any_copied = FALSE;
    int files_copied = 0;
    char src_path[256];
    char dst_path[256];
    char msg[256];

    /* Ensure destination directory exists (CreateDir fails if it already does) */
    {
        BPTR tmp = CreateDir("RAM:iTidy_Logs");
        if (tmp) UnLock(tmp);
    }

    src_lock = Lock("PROGDIR:logs/", ACCESS_READ);
    if (!src_lock)
    {
        ShowReActionRequester(parent,
            "Archive Logs",
            "No logs folder found (PROGDIR:logs/)",
            "_OK",
            REQIMAGE_WARNING);
        return;
    }

    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        UnLock(src_lock);
        return;
    }

    if (Examine(src_lock, fib))
    {
        while (ExNext(src_lock, fib))
        {
            if (fib->fib_DirEntryType < 0)  /* file, not directory */
            {
                BPTR src_fh, dst_fh;
                LONG len;
                UBYTE copy_buf[512];

                sprintf(src_path, "PROGDIR:logs/%s", fib->fib_FileName);
                sprintf(dst_path, "RAM:iTidy_Logs/%s", fib->fib_FileName);

                src_fh = Open((STRPTR)src_path, MODE_OLDFILE);
                if (src_fh)
                {
                    dst_fh = Open((STRPTR)dst_path, MODE_NEWFILE);
                    if (dst_fh)
                    {
                        while ((len = Read(src_fh, copy_buf, sizeof(copy_buf))) > 0)
                            Write(dst_fh, copy_buf, len);
                        Close(dst_fh);
                        files_copied++;
                        any_copied = TRUE;
                    }
                    Close(src_fh);
                }
            }
        }
    }

    FreeDosObject(DOS_FIB, fib);
    UnLock(src_lock);

    if (any_copied)
    {
        sprintf(msg, "Archived %d log file%s to RAM:iTidy_Logs/",
            files_copied, files_copied == 1 ? "" : "s");
        ShowReActionRequester(parent, "Archive Logs", msg, "_OK", REQIMAGE_INFO);
    }
    else
    {
        ShowReActionRequester(parent,
            "Archive Logs",
            "No log files found in PROGDIR:logs/",
            "_OK",
            REQIMAGE_INFO);
    }
}
