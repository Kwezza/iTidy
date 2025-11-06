/*
 * wb_classify.c - Workbench Window Classification Implementation
 * Part of iTidy Icon Cleanup Tool
 * 
 * Provides best-effort classification of windows to identify
 * Workbench drawers, utilities, and other window types.
 */

#include "wb_classify.h"

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/nodes.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <string.h>

/*------------------------------------------------------------------------*/
/* Internal Helper Functions                                              */
/*------------------------------------------------------------------------*/

/**
 * @brief Case-insensitive string comparison
 * 
 * @param a First string
 * @param b Second string
 * @return int 1 if equal, 0 if not equal
 */
static int str_ieq(const char *a, const char *b)
{
    if (!a || !b) return 0;
    
    /* Stricmp is an AmigaDOS/locale-aware compare; use if available */
#ifdef Stricmp
    return Stricmp(a, b) == 0;
#else
    /* Fallback: case-sensitive */
    return strcmp(a, b) == 0;
#endif
}

/**
 * @brief Get the name of the task that owns a window
 * 
 * @param w Pointer to Window structure
 * @return const char* Task name or NULL
 */
static const char *owner_task_name(const struct Window *w)
{
    const struct Task *t;
    
    if (!w || !w->UserPort)
    {
        return NULL;
    }
    
    t = w->UserPort->mp_SigTask;
    return t ? (const char *)t->tc_Node.ln_Name : NULL;
}

/**
 * @brief Check if window is on Workbench screen
 * 
 * @param w Pointer to Window structure
 * @return int 1 if on Workbench screen, 0 otherwise
 */
static int on_workbench_screen(const struct Window *w)
{
    int ok = 0;
    struct Screen *wb;
    
    if (!w)
    {
        return 0;
    }
    
    wb = LockPubScreen("Workbench");
    if (wb)
    {
        ok = (w->WScreen == wb);
        UnlockPubScreen(NULL, wb);
    }
    
    return ok;
}

/**
 * @brief Check if window title looks like a drawer name
 * 
 * One-token title or device root "DH0:" is a decent drawer-ish hint.
 * 
 * @param t Window title string
 * @return int 1 if looks like drawer, 0 otherwise
 */
static int title_looks_like_drawer(const char *t)
{
    size_t n;
    
    if (!t || !*t)
    {
        return 0;
    }
    
    n = strlen(t);
    
    /* Device root like "DH0:" */
    if (n && t[n-1] == ':')
    {
        return 1;
    }
    
    /* Single token like "Amiga" or "Programs" */
    return strchr(t, ' ') == NULL;
}

/**
 * @brief Check if window has normal drawer-style gadgets
 * 
 * Typical drawer windows have close, size, and dragbar gadgets.
 * 
 * @param w Pointer to Window structure
 * @return int 1 if has normal gadgets, 0 otherwise
 */
static int has_normal_gadgets(const struct Window *w)
{
    ULONG f = w->Flags;
    
    /* Typical drawer windows: close + size + dragbar (+ depth is common) */
    if (!(f & WFLG_CLOSEGADGET))
    {
        return 0;
    }
    
    if (!(f & WFLG_SIZEGADGET))
    {
        return 0;
    }
    
    if (!(f & WFLG_DRAGBAR))
    {
        return 0;
    }
    
    return 1;
}

/* From traces, WB drawers/utilities commonly show this shape of flags.
   We use masks so minor differences still match across systems. */
#define WB_WINFLAGS_MASK   (WFLG_SIZEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET| \
                            WFLG_CLOSEGADGET|WFLG_SIZEBRIGHT|WFLG_SIZEBBOTTOM| \
                            WFLG_REFRESHBITS|WFLG_SIMPLE_REFRESH|WFLG_ACTIVATE| \
                            WFLG_HASZOOM)

#define WB_WINFLAGS_VALUE  (WFLG_SIZEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET| \
                            WFLG_CLOSEGADGET|WFLG_SIZEBRIGHT|WFLG_SIZEBBOTTOM| \
                            WFLG_REFRESHBITS|WFLG_SIMPLE_REFRESH|WFLG_ACTIVATE| \
                            WFLG_HASZOOM)

/* IDCMP mask derived from common drawer/utility dumps 
   Note: IDCMP_MENUHELP (0x00080000) is commonly present in WB 3.x drawers */
#define WB_IDCMP_MASK     (IDCMP_NEWSIZE|IDCMP_REFRESHWINDOW|IDCMP_MOUSEBUTTONS| \
                           IDCMP_MOUSEMOVE|IDCMP_GADGETDOWN|IDCMP_GADGETUP|     \
                           IDCMP_MENUPICK|IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|       \
                           IDCMP_ACTIVEWINDOW|IDCMP_INACTIVEWINDOW)

/* Alternative mask including MENUHELP for systems that have it */
#define WB_IDCMP_MASK_WITH_HELP (WB_IDCMP_MASK | IDCMP_MENUHELP)

/**
 * @brief Check if window is Workbench family (drawer or utility)
 * 
 * Simplified approach: owned by "Workbench" task + has basic drawer gadgets.
 * This is much more reliable than complex flag matching.
 * 
 * @param w Pointer to Window structure
 * @return int 1 if Workbench family, 0 otherwise
 */
static int is_workbench_family(const struct Window *w)
{
    const char *owner;
    
    if (!w)
    {
        return 0;
    }
    
    /* Must be on Workbench screen */
    if (!on_workbench_screen(w))
    {
        return 0;
    }
    
    /* Must be owned by Workbench task */
    owner = owner_task_name(w);
    if (!owner || strcmp(owner, "Workbench") != 0)
    {
        return 0;
    }
    
    /* Backdrop windows handled separately */
    if (w->Flags & WFLG_BACKDROP)
    {
        return 0;
    }
    
    /* If owned by Workbench and has basic drawer gadgets, it's WB family */
    return has_normal_gadgets(w);
}

/*------------------------------------------------------------------------*/
/* Public API                                                             */
/*------------------------------------------------------------------------*/

/**
 * @brief Classify a window as Workbench drawer, utility, or other
 * 
 * Main classification function that examines window properties to
 * determine its likely type.
 * 
 * Simplified approach: All Workbench-owned windows with normal gadgets
 * are classified as drawers, as title-based heuristics are unreliable
 * (folders can have spaces, volume windows show statistics, etc.)
 */
WbKind ClassifyWB(const struct Window *w)
{
    if (!w)
    {
        return WBK_NONE;
    }

    /* Backdrop and tiny "ghost" helper windows */
    if (w->Flags & WFLG_BACKDROP)
    {
        if (w->Width <= 4 && w->Height <= 4)
        {
            return WBK_GHOST;
        }
        return WBK_BACKDROP;
    }

    /* Workbench family detected via owner + gadgets */
    if (is_workbench_family(w))
    {
        /* All Workbench-owned windows with normal gadgets are likely drawers.
           Title-based distinction (single-word vs multi-word) is unreliable:
           - Folders can have spaces: "Multi word folder test"
           - Volume roots show stats: "Workbench 17% full, 365.1MB free..."
           So we classify them all as drawers for consistency. */
        return WBK_DRAWER_LIKELY;
    }

    /* Not matching Workbench family signature */
    return WBK_NONE;
}

/**
 * @brief Convert WbKind enum to human-readable string
 * 
 * Convenience function for logging and display purposes.
 */
const char *WbKindToString(WbKind k)
{
    switch (k)
    {
        case WBK_NONE:
            return "None/Other";
        case WBK_DRAWER_LIKELY:
            return "Workbench Drawer (likely)";
        case WBK_UTILITY_LIKELY:
            return "Workbench Utility (likely)";
        case WBK_BACKDROP:
            return "Workbench Backdrop";
        case WBK_GHOST:
            return "Ghost/Helper";
        default:
            return "Unknown";
    }
}

/* End of wb_classify.c */
