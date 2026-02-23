/*
 * workbench_layout.h - iTidy Workbench Screen Icon Layout Engine
 * Calculates and applies icon positions for the Workbench screen
 * (device icons and left-out icons)
 */

#ifndef ITIDY_WORKBENCH_LAYOUT_H
#define ITIDY_WORKBENCH_LAYOUT_H

#include <exec/types.h>
#include "backups/backdrop_parser.h"

/*------------------------------------------------------------------------*/
/* Constants                                                              */
/*------------------------------------------------------------------------*/

/* Default grid spacing (pixels) */
#define ITIDY_WB_GRID_X         80   /* Horizontal spacing between icons */
#define ITIDY_WB_GRID_Y         50   /* Vertical spacing between rows */
#define ITIDY_WB_MARGIN_LEFT    10   /* Left edge margin */
#define ITIDY_WB_MARGIN_TOP     20   /* Top margin (below screen title bar) */
#define ITIDY_WB_MARGIN_RIGHT   10   /* Right edge margin */

/*------------------------------------------------------------------------*/
/* Layout Parameters                                                      */
/*------------------------------------------------------------------------*/
typedef struct {
    LONG screen_width;          /* Workbench screen width in pixels */
    LONG screen_height;         /* Workbench screen height in pixels */
    LONG grid_x;                /* Horizontal grid spacing */
    LONG grid_y;                /* Vertical grid spacing */
    LONG margin_left;           /* Left margin */
    LONG margin_top;            /* Top margin */
    LONG margin_right;          /* Right margin */
} iTidy_WBLayoutParams;

/*------------------------------------------------------------------------*/
/* Layout Result Entry                                                    */
/*------------------------------------------------------------------------*/
typedef struct {
    int entry_index;            /* Index into the backdrop list */
    LONG new_x;                 /* Calculated X position */
    LONG new_y;                 /* Calculated Y position */
    LONG old_x;                 /* Original X position */
    LONG old_y;                 /* Original Y position */
    BOOL changed;               /* TRUE if position was modified */
} iTidy_WBLayoutEntry;

/*------------------------------------------------------------------------*/
/* Layout Result                                                          */
/*------------------------------------------------------------------------*/
typedef struct {
    iTidy_WBLayoutEntry *entries;   /* Dynamic array (whd_malloc) */
    int count;                       /* Number of entries */
    int capacity;                    /* Allocated capacity */
    int changed_count;               /* How many icons actually moved */
} iTidy_WBLayoutResult;

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Initialize layout parameters with sensible defaults
 *
 * @param params Parameters struct to initialize
 * @param screen_width Workbench screen width
 * @param screen_height Workbench screen height
 */
void itidy_init_wb_layout_params(iTidy_WBLayoutParams *params,
                                  LONG screen_width,
                                  LONG screen_height);

/**
 * @brief Calculate new icon positions for Workbench screen
 *
 * Pure calculation function. Arranges icons in horizontal rows:
 * - Device icons first, sorted alphabetically
 * - Left-out icons below, sorted by type then alphabetically
 *
 * @param params Layout parameters (screen size, spacing, margins)
 * @param list Backdrop list with validated entries
 * @param result Layout result to populate (caller provides)
 * @return BOOL TRUE if calculation succeeded
 */
BOOL itidy_calculate_wb_layout(const iTidy_WBLayoutParams *params,
                                const iTidy_BackdropList *list,
                                iTidy_WBLayoutResult *result);

/**
 * @brief Apply calculated positions to icon files
 *
 * Writes new positions via GetDiskObject/PutDiskObject for each
 * changed icon. Uses PutIconTagList with ICONPUTA_NotifyWorkbench
 * (V44+) for live updates.
 *
 * @param list Backdrop list containing full paths
 * @param result Layout result containing new positions
 * @return int Number of icons successfully repositioned, -1 on error
 */
int itidy_apply_wb_layout(const iTidy_BackdropList *list,
                           const iTidy_WBLayoutResult *result);

/**
 * @brief Check if RAM: icon was repositioned and offer ENVARC persistence
 *
 * If RAM: was in the layout result and its position changed,
 * prompts user to copy RAM:Disk.info to ENVARC:Sys/def_RAM.info.
 *
 * @param parent_window Parent window for requester (can be NULL)
 * @param list Backdrop list
 * @param result Layout result
 * @return BOOL TRUE if persistence was performed or not needed
 */
BOOL itidy_handle_ram_icon_persistence(struct Window *parent_window,
                                        const iTidy_BackdropList *list,
                                        const iTidy_WBLayoutResult *result);

/**
 * @brief Initialize an empty layout result
 *
 * @param result Layout result to initialize
 */
void itidy_init_wb_layout_result(iTidy_WBLayoutResult *result);

/**
 * @brief Free layout result resources
 *
 * @param result Layout result to free
 */
void itidy_free_wb_layout_result(iTidy_WBLayoutResult *result);

#endif /* ITIDY_WORKBENCH_LAYOUT_H */
