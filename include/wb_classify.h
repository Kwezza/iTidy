/*
 * wb_classify.h - Workbench Window Classification
 * Part of iTidy Icon Cleanup Tool
 * 
 * Provides best-effort classification of windows on Workbench screen
 * to distinguish between drawers, utilities, and other window types.
 * 
 * Compatible with Workbench 3.0-3.9, uses only public Intuition fields.
 */

#ifndef WB_CLASSIFY_H
#define WB_CLASSIFY_H

#include <exec/types.h>
#include <intuition/intuition.h>

/*------------------------------------------------------------------------*/
/* Window Classification Types                                           */
/*------------------------------------------------------------------------*/

/* Returned as an integer type so you can switch() easily */
typedef enum WbKind {
    WBK_NONE = 0,          /* Not Workbench family (or unknown) */
    WBK_DRAWER_LIKELY,     /* Likely a Workbench drawer (folder) */
    WBK_UTILITY_LIKELY,    /* Workbench-owned non-drawer (Disk Info, main WB, prefs, etc.) */
    WBK_BACKDROP,          /* Workbench backdrop window */
    WBK_GHOST              /* Tiny helper/ghost window (often 1x1, backdrop) */
} WbKind;

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Classify a window as Workbench drawer, utility, or other
 * 
 * Uses window flags, IDCMP flags, owner task, and title heuristics
 * to determine the likely type of a window. Best-effort classification
 * compatible with WB 3.0-3.9.
 * 
 * @param w Pointer to Window structure to classify
 * @return WbKind Classification enum value
 */
WbKind ClassifyWB(const struct Window *w);

/**
 * @brief Convert WbKind enum to human-readable string
 * 
 * Convenience function for logging and display purposes.
 * 
 * @param k WbKind enum value
 * @return const char* Human-readable string describing the kind
 */
const char *WbKindToString(WbKind k);

#endif /* WB_CLASSIFY_H */
