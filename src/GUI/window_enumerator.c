/*
 * window_enumerator.c - Workbench Window Enumeration Implementation
 * Part of iTidy Icon Cleanup Tool
 * 
 * Provides debugging functionality to list all open Workbench windows
 * with their titles and filesystem paths.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

#include "window_enumerator.h"
#include "writeLog.h"
#include "wb_classify.h"

/* External library bases */
extern struct IntuitionBase *IntuitionBase;

/*------------------------------------------------------------------------*/
/* Flag Decoding Tables and Helpers                                      */
/*------------------------------------------------------------------------*/

/* Generic flag descriptor structure */
struct FlagDesc 
{ 
    ULONG bit; 
    const char *name; 
};

/* Window->Flags table (WB 3.0 era) */
static const struct FlagDesc winFlags[] = {
#ifdef WFLG_SIZEGADGET
    { WFLG_SIZEGADGET,      "WFLG_SIZEGADGET"      },
#endif
#ifdef WFLG_DRAGBAR
    { WFLG_DRAGBAR,         "WFLG_DRAGBAR"         },
#endif
#ifdef WFLG_DEPTHGADGET
    { WFLG_DEPTHGADGET,     "WFLG_DEPTHGADGET"     },
#endif
#ifdef WFLG_CLOSEGADGET
    { WFLG_CLOSEGADGET,     "WFLG_CLOSEGADGET"     },
#endif
#ifdef WFLG_SIZEBRIGHT
    { WFLG_SIZEBRIGHT,      "WFLG_SIZEBRIGHT"      },
#endif
#ifdef WFLG_SIZEBBOTTOM
    { WFLG_SIZEBBOTTOM,     "WFLG_SIZEBBOTTOM"     },
#endif
#ifdef WFLG_REFRESHBITS
    { WFLG_REFRESHBITS,     "WFLG_REFRESHBITS"     },
#endif
#ifdef WFLG_SMART_REFRESH
    { WFLG_SMART_REFRESH,   "WFLG_SMART_REFRESH"   },
#endif
#ifdef WFLG_SIMPLE_REFRESH
    { WFLG_SIMPLE_REFRESH,  "WFLG_SIMPLE_REFRESH"  },
#endif
#ifdef WFLG_SUPER_BITMAP
    { WFLG_SUPER_BITMAP,    "WFLG_SUPER_BITMAP"    },
#endif
#ifdef WFLG_GIMMEZEROZERO
    { WFLG_GIMMEZEROZERO,   "WFLG_GIMMEZEROZERO"   },
#endif
#ifdef WFLG_BORDERLESS
    { WFLG_BORDERLESS,      "WFLG_BORDERLESS"      },
#endif
#ifdef WFLG_ACTIVATE
    { WFLG_ACTIVATE,        "WFLG_ACTIVATE"        },
#endif
#ifdef WFLG_RMBTRAP
    { WFLG_RMBTRAP,         "WFLG_RMBTRAP"         },
#endif
#ifdef WFLG_BACKDROP
    { WFLG_BACKDROP,        "WFLG_BACKDROP"        },
#endif
#ifdef WFLG_REPORTMOUSE
    { WFLG_REPORTMOUSE,     "WFLG_REPORTMOUSE"     },
#endif
#ifdef WFLG_WINDOWACTIVE
    { WFLG_WINDOWACTIVE,    "WFLG_WINDOWACTIVE"    },
#endif
#ifdef WFLG_INREQUEST
    { WFLG_INREQUEST,       "WFLG_INREQUEST"       },
#endif
#ifdef WFLG_MENUSTATE
    { WFLG_MENUSTATE,       "WFLG_MENUSTATE"       },
#endif
#ifdef WFLG_ZOOMGADGET
    { WFLG_ZOOMGADGET,      "WFLG_ZOOMGADGET"      },
#endif
#ifdef WFLG_HASZOOM
    { WFLG_HASZOOM,         "WFLG_HASZOOM"         },
#endif
#ifdef WFLG_NOCAREREFRESH
    { WFLG_NOCAREREFRESH,   "WFLG_NOCAREREFRESH"   },
#endif
#ifdef WFLG_NW_EXTENDED
    { WFLG_NW_EXTENDED,     "WFLG_NW_EXTENDED"     },
#endif
};

/* IDCMP flags table */
static const struct FlagDesc idcmpFlags[] = {
#ifdef IDCMP_SIZEVERIFY
    { IDCMP_SIZEVERIFY,     "IDCMP_SIZEVERIFY"     },
#endif
#ifdef IDCMP_NEWSIZE
    { IDCMP_NEWSIZE,        "IDCMP_NEWSIZE"        },
#endif
#ifdef IDCMP_REFRESHWINDOW
    { IDCMP_REFRESHWINDOW,  "IDCMP_REFRESHWINDOW"  },
#endif
#ifdef IDCMP_MOUSEBUTTONS
    { IDCMP_MOUSEBUTTONS,   "IDCMP_MOUSEBUTTONS"   },
#endif
#ifdef IDCMP_MOUSEMOVE
    { IDCMP_MOUSEMOVE,      "IDCMP_MOUSEMOVE"      },
#endif
#ifdef IDCMP_GADGETDOWN
    { IDCMP_GADGETDOWN,     "IDCMP_GADGETDOWN"     },
#endif
#ifdef IDCMP_GADGETUP
    { IDCMP_GADGETUP,       "IDCMP_GADGETUP"       },
#endif
#ifdef IDCMP_REQVERIFY
    { IDCMP_REQVERIFY,      "IDCMP_REQVERIFY"      },
#endif
#ifdef IDCMP_REQSET
    { IDCMP_REQSET,         "IDCMP_REQSET"         },
#endif
#ifdef IDCMP_REQCLEAR
    { IDCMP_REQCLEAR,       "IDCMP_REQCLEAR"       },
#endif
#ifdef IDCMP_MENUVERIFY
    { IDCMP_MENUVERIFY,     "IDCMP_MENUVERIFY"     },
#endif
#ifdef IDCMP_INTUITICKS
    { IDCMP_INTUITICKS,     "IDCMP_INTUITICKS"     },
#endif
#ifdef IDCMP_MENUPICK
    { IDCMP_MENUPICK,       "IDCMP_MENUPICK"       },
#endif
#ifdef IDCMP_CLOSEWINDOW
    { IDCMP_CLOSEWINDOW,    "IDCMP_CLOSEWINDOW"    },
#endif
#ifdef IDCMP_RAWKEY
    { IDCMP_RAWKEY,         "IDCMP_RAWKEY"         },
#endif
#ifdef IDCMP_VANILLAKEY
    { IDCMP_VANILLAKEY,     "IDCMP_VANILLAKEY"     },
#endif
#ifdef IDCMP_ACTIVEWINDOW
    { IDCMP_ACTIVEWINDOW,   "IDCMP_ACTIVEWINDOW"   },
#endif
#ifdef IDCMP_INACTIVEWINDOW
    { IDCMP_INACTIVEWINDOW, "IDCMP_INACTIVEWINDOW" },
#endif
#ifdef IDCMP_DELTAMOVE
    { IDCMP_DELTAMOVE,      "IDCMP_DELTAMOVE"      },
#endif
#ifdef IDCMP_NEWPREFS
    { IDCMP_NEWPREFS,       "IDCMP_NEWPREFS"       },
#endif
#ifdef IDCMP_IDCMPUPDATE
    { IDCMP_IDCMPUPDATE,    "IDCMP_IDCMPUPDATE"    },
#endif
#ifdef IDCMP_MENUHELP
    { IDCMP_MENUHELP,       "IDCMP_MENUHELP"       },
#endif
#ifdef IDCMP_GADGETHELP
    { IDCMP_GADGETHELP,     "IDCMP_GADGETHELP"     },
#endif
#ifdef IDCMP_WBENCHMESSAGE
    { IDCMP_WBENCHMESSAGE,  "IDCMP_WBENCHMESSAGE"  },
#endif
};

/*------------------------------------------------------------------------*/
/**
 * @brief Print a list of flag bits with symbolic names
 * 
 * Generic bit-decoder helper that prints the hex value and then lists
 * all symbolic names for bits that are set.
 * 
 * @param label Label to print (e.g., "Flags", "IDCMPFlags")
 * @param value The flag value to decode
 * @param tab Array of FlagDesc structures
 * @param n Number of entries in the table
 */
/*------------------------------------------------------------------------*/
static void print_flag_list(const char *label, ULONG value,
                            const struct FlagDesc *tab, size_t n)
{
    size_t i;
    
    log_debug(LOG_GUI, "  %s: 0x%08lx\n", label, value);
    
    for (i = 0; i < n; ++i)
    {
        if (tab[i].bit && (value & tab[i].bit))
        {
            log_debug(LOG_GUI, "    - %s\n", tab[i].name);
        }
    }
}

/*------------------------------------------------------------------------*/
/**
 * @brief Dump decoded flags for a window
 * 
 * Prints both Window->Flags and Window->IDCMPFlags with symbolic names.
 * Useful for identifying window types and their event handling.
 * 
 * @param w Pointer to Window structure
 */
/*------------------------------------------------------------------------*/
static void Debug_DumpFlags(const struct Window *w)
{
    if (w == NULL)
    {
        return;
    }
    
    print_flag_list("Flags", w->Flags, 
                    winFlags, sizeof(winFlags)/sizeof(winFlags[0]));
    print_flag_list("IDCMPFlags", w->IDCMPFlags, 
                    idcmpFlags, sizeof(idcmpFlags)/sizeof(idcmpFlags[0]));
}

/*------------------------------------------------------------------------*/
/**
 * @brief Get the name of the task that owns a window
 * 
 * Retrieves the task name from the window's UserPort message port.
 * This helps identify whether a window belongs to Workbench or another
 * application.
 * 
 * @param w Pointer to Window structure
 * @return const char* Task name string, or NULL if unavailable
 */
/*------------------------------------------------------------------------*/
static const char *OwnerTaskName(const struct Window *w)
{
    if (!w || !w->UserPort || !w->UserPort->mp_SigTask)
    {
        return NULL;
    }
    
    const struct Task *t = w->UserPort->mp_SigTask;
    return t->tc_Node.ln_Name;  /* May be NULL */
}

/*------------------------------------------------------------------------*/
/**
 * @brief Attempt to resolve a drawer path from window title
 * 
 * For Workbench drawer windows, the title often contains the path.
 * This function attempts to get a lock on the path and resolve it
 * to a full filesystem path.
 * 
 * @param title Window title (may be a drawer path)
 * @param pathBuffer Buffer to receive resolved path
 * @param bufferSize Size of path buffer
 * @return BOOL TRUE if path resolved successfully, FALSE otherwise
 */
/*------------------------------------------------------------------------*/
static BOOL TryResolveDrawerPath(const char *title, char *pathBuffer, ULONG bufferSize)
{
    BPTR lock;
    BOOL success = FALSE;
    
    if (title == NULL || pathBuffer == NULL || bufferSize == 0)
    {
        return FALSE;
    }
    
    /* Clear buffer */
    pathBuffer[0] = '\0';
    
    /* Try to get a lock on the title as a path */
    lock = Lock(title, ACCESS_READ);
    if (lock != 0)
    {
        /* Got a lock - resolve to full path */
        if (NameFromLock(lock, pathBuffer, bufferSize))
        {
            success = TRUE;
        }
        UnLock(lock);
    }
    
    return success;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Check if a window appears to be a Workbench drawer
 * 
 * Uses heuristics to determine if a window is likely a Workbench drawer
 * based on its properties and title.
 * 
 * @param win Pointer to Window structure
 * @return BOOL TRUE if window appears to be a drawer, FALSE otherwise
 */
/*------------------------------------------------------------------------*/
static BOOL IsLikelyDrawerWindow(struct Window *win)
{
    if (win == NULL || win->Title == NULL)
    {
        return FALSE;
    }
    
    /* Check if title contains a colon (indicating a device/path) */
    if (strchr(win->Title, ':') != NULL)
    {
        return TRUE;
    }
    
    /* Could add more heuristics here */
    return FALSE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Enumerate and print all open Workbench windows
 * 
 * Main implementation of the window enumeration functionality.
 * Safely iterates through all windows on the Workbench screen and
 * prints diagnostic information about each one.
 */
/*------------------------------------------------------------------------*/
void Debug_ListWorkbenchWindows(void)
{
    struct Screen *wbScreen = NULL;
    struct Window *win = NULL;
    ULONG lockState;
    ULONG windowCount = 0;
    char pathBuffer[512];
    
    log_debug(LOG_GUI, "\n");
    log_debug(LOG_GUI, "========================================\n");
    log_debug(LOG_GUI, "  Workbench Window Enumerator - Debug\n");
    log_debug(LOG_GUI, "========================================\n");
    log_debug(LOG_GUI, "\n");
    
    /* Lock the Workbench screen */
    wbScreen = LockPubScreen("Workbench");
    if (wbScreen == NULL)
    {
        log_error(LOG_GUI, "ERROR: Failed to lock Workbench screen\n");
        log_error(LOG_GUI, "       (Workbench may not be running)\n");
        log_debug(LOG_GUI, "\n");
        return;
    }
    
    log_debug(LOG_GUI, "Workbench screen locked successfully\n");
    log_debug(LOG_GUI, "Screen dimensions: %dx%d\n", wbScreen->Width, wbScreen->Height);
    log_debug(LOG_GUI, "\n");
    
    /* Lock IntuitionBase to safely traverse window list */
    lockState = LockIBase(0);
    
    /* Iterate through all windows on the Workbench screen */
    win = wbScreen->FirstWindow;
    
    if (win == NULL)
    {
        log_debug(LOG_GUI, "No windows found on Workbench screen\n");
    }
    else
    {
        log_debug(LOG_GUI, "Enumerating windows...\n");
        log_debug(LOG_GUI, "\n");
        
        while (win != NULL)
        {
            windowCount++;
            
            /* Print window title */
            if (win->Title != NULL && win->Title[0] != '\0')
            {
                log_debug(LOG_GUI, "Window #%lu: \"%s\"\n", windowCount, win->Title);
            }
            else
            {
                log_debug(LOG_GUI, "Window #%lu: (no title)\n", windowCount);
            }
            
            /* Print window dimensions and position */
            log_debug(LOG_GUI, "  Position: (%d, %d)\n", win->LeftEdge, win->TopEdge);
            log_debug(LOG_GUI, "  Size: %dx%d\n", win->Width, win->Height);
            
            /* Print decoded window flags and IDCMP flags */
            Debug_DumpFlags(win);
            
            /* Print screen information */
            if (win->WScreen && win->WScreen->Title)
            {
                log_debug(LOG_GUI, "  Screen: %s", win->WScreen->Title);
                if (win->WScreen->Flags & PUBLICSCREEN)
                {
                    log_debug(LOG_GUI, " (Public screen)");
                }
                log_debug(LOG_GUI, "\n");
            }
            
            /* Print owner task name */
            {
                const char *owner = OwnerTaskName(win);
                log_debug(LOG_GUI, "  Owner task: %s\n", owner ? owner : "<unknown>");
            }
            
            /* Classify the window using wb_classify */
            {
                WbKind kind = ClassifyWB(win);
                log_debug(LOG_GUI, "  Classification: %s\n", WbKindToString(kind));
            }
            
            log_debug(LOG_GUI, "\n");
            
            /* Move to next window */
            win = win->NextWindow;
        }
    }
    
    /* Unlock IntuitionBase */
    UnlockIBase(lockState);
    
    /* Unlock the Workbench screen */
    UnlockPubScreen(NULL, wbScreen);
    
    log_debug(LOG_GUI, "========================================\n");
    log_debug(LOG_GUI, "Total windows enumerated: %lu\n", windowCount);
    log_debug(LOG_GUI, "========================================\n");
    log_debug(LOG_GUI, "\n");
}

/*------------------------------------------------------------------------*/
/**
 * @brief Build array of potential folder windows for iTidy tracking
 * 
 * Scans all open Workbench windows and creates a tracking array of those
 * that are likely folder/drawer windows.
 */
/*------------------------------------------------------------------------*/
BOOL BuildFolderWindowList(FolderWindowTracker *tracker)
{
    struct Screen *wbScreen;
    struct Window *win;
    ULONG lockState;
    ULONG capacity = 16; /* Initial capacity */
    
    if (!tracker)
    {
        log_error(LOG_GUI, "BuildFolderWindowList: NULL tracker pointer\n");
        return FALSE;
    }
    
    /* Initialize tracker */
    tracker->windows = NULL;
    tracker->count = 0;
    tracker->capacity = 0;
    
    /* Lock the Workbench screen */
    wbScreen = LockPubScreen("Workbench");
    if (!wbScreen)
    {
        log_error(LOG_GUI, "BuildFolderWindowList: Failed to lock Workbench screen\n");
        return FALSE;
    }
    
    /* Allocate initial array */
    tracker->windows = AllocVec(capacity * sizeof(FolderWindowInfo), MEMF_CLEAR);
    if (!tracker->windows)
    {
        log_error(LOG_GUI, "BuildFolderWindowList: Failed to allocate memory\n");
        UnlockPubScreen(NULL, wbScreen);
        return FALSE;
    }
    tracker->capacity = capacity;
    
    /* Lock IntuitionBase for window list traversal */
    lockState = LockIBase(0);
    
    /* Enumerate windows on Workbench screen */
    win = wbScreen->FirstWindow;
    while (win)
    {
        /* Classify the window */
        WbKind kind = ClassifyWB(win);
        
        /* Only track windows classified as likely drawers */
        if (kind == WBK_DRAWER_LIKELY)
        {
            /* Skip volume/disk root windows by checking for disk-event IDCMP flags.
               Root volume windows (like "Workbench:", "Work:", etc.) listen for
               disk insertion/removal events, while regular folder windows don't.
               
               IDCMP_DISKINSERTED  = 0x00008000
               IDCMP_DISKREMOVED   = 0x00010000
               
               This is a reliable indicator that works regardless of window position,
               size, or title. */
            if (win->IDCMPFlags & 0x00018000)  /* Has disk event flags */
            {
                /* Skip this volume root window */
                win = win->NextWindow;
                continue;
            }
            
            /* Expand array if needed */
            if (tracker->count >= tracker->capacity)
            {
                ULONG newCapacity = tracker->capacity * 2;
                FolderWindowInfo *newArray = AllocVec(newCapacity * sizeof(FolderWindowInfo), MEMF_CLEAR);
                
                if (!newArray)
                {
                    log_error(LOG_GUI, "BuildFolderWindowList: Failed to expand array\n");
                    break;
                }
                
                /* Copy old data */
                CopyMem(tracker->windows, newArray, tracker->count * sizeof(FolderWindowInfo));
                
                /* Free old array and use new one */
                FreeVec(tracker->windows);
                tracker->windows = newArray;
                tracker->capacity = newCapacity;
            }
            
            /* Add window to tracker */
            FolderWindowInfo *info = &tracker->windows[tracker->count];
            
            info->window = win;
            
            /* Copy title (safely handle NULL or long titles) */
            if (win->Title)
            {
                strncpy(info->title, win->Title, sizeof(info->title) - 1);
                info->title[sizeof(info->title) - 1] = '\0';
            }
            else
            {
                strcpy(info->title, "(no title)");
            }
            
            /* Store current geometry */
            info->left = win->LeftEdge;
            info->top = win->TopEdge;
            info->width = win->Width;
            info->height = win->Height;
            
            tracker->count++;
        }
        
        win = win->NextWindow;
    }
    
    /* Unlock IntuitionBase */
    UnlockIBase(lockState);
    
    /* Unlock the Workbench screen */
    UnlockPubScreen(NULL, wbScreen);
    
    log_info(LOG_GUI, "BuildFolderWindowList: Tracked %lu folder window(s)\n", tracker->count);
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Free the folder window tracker and its resources
 */
/*------------------------------------------------------------------------*/
void FreeFolderWindowList(FolderWindowTracker *tracker)
{
    if (!tracker)
    {
        return;
    }
    
    if (tracker->windows)
    {
        FreeVec(tracker->windows);
        tracker->windows = NULL;
    }
    
    tracker->count = 0;
    tracker->capacity = 0;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Debug print the folder window list to log
 */
/*------------------------------------------------------------------------*/
void Debug_PrintFolderWindowList(const FolderWindowTracker *tracker)
{
    ULONG i;
    
    if (!tracker)
    {
        log_error(LOG_GUI, "Debug_PrintFolderWindowList: NULL tracker\n");
        return;
    }
    
    log_debug(LOG_GUI, "\n");
    log_debug(LOG_GUI, "========================================\n");
    log_debug(LOG_GUI, "  Folder Window Tracker - Debug\n");
    log_debug(LOG_GUI, "========================================\n");
    log_debug(LOG_GUI, "\n");
    log_debug(LOG_GUI, "Total tracked folders: %lu\n", tracker->count);
    log_debug(LOG_GUI, "Array capacity: %lu\n", tracker->capacity);
    log_debug(LOG_GUI, "\n");
    
    if (tracker->count == 0)
    {
        log_debug(LOG_GUI, "No folder windows tracked.\n");
        log_debug(LOG_GUI, "\n");
        return;
    }
    
    for (i = 0; i < tracker->count; i++)
    {
        const FolderWindowInfo *info = &tracker->windows[i];
        
        log_debug(LOG_GUI, "Folder #%lu: \"%s\"\n", i + 1, info->title);
        log_debug(LOG_GUI, "  Window pointer: 0x%08lx\n", (ULONG)info->window);
        log_debug(LOG_GUI, "  Position: (%d, %d)\n", info->left, info->top);
        log_debug(LOG_GUI, "  Size: %dx%d\n", info->width, info->height);
        log_debug(LOG_GUI, "\n");
    }
    
    log_debug(LOG_GUI, "========================================\n");
    log_debug(LOG_GUI, "\n");
}

/* End of window_enumerator.c */
