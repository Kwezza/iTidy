/*
 * gui_groupbox.h - Workbench 2/3 Group Box Helper
 * 
 * Provides reusable functions for drawing Workbench-style group frames
 * around GadTools gadgets using DrawBevelBox() with centered captions.
 * 
 * Font-aware layout ensures proper rendering on RTG/high-res screens.
 */

#ifndef GUI_GROUPBOX_H
#define GUI_GROUPBOX_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>

/*------------------------------------------------------------------------*/
/**
 * @brief Calculate a group box rectangle for a range of gadgets
 * 
 * Inspects all gadgets from first to last (following NextGadget chain)
 * and computes a bounding rectangle that includes font-relative margins
 * and a title band at the top for the caption.
 * 
 * @param win Window containing the gadgets
 * @param first First gadget in the group
 * @param last Last gadget in the group
 * @param box Output rectangle (caller-allocated)
 * 
 * Notes:
 * - Gadgets must be fully created with valid geometry before calling
 * - Uses screen font metrics for proportional spacing
 * - Title band height is calculated to fit caption text
 */
/*------------------------------------------------------------------------*/
VOID CalcGadgetGroupBoxRect(struct Window    *win,
                            struct Gadget    *first,
                            struct Gadget    *last,
                            struct Rectangle *box);

/*------------------------------------------------------------------------*/
/**
 * @brief Draw a Workbench-style group box with caption
 * 
 * Renders a recessed bevel frame using DrawBevelBox() and draws a
 * centered caption in the top border band with shadow/highlight text
 * for a classic Workbench appearance.
 * 
 * @param win Window to draw into
 * @param vi VisualInfo from GetVisualInfo() (for DrawBevelBox)
 * @param box Rectangle computed by CalcGadgetGroupBoxRect()
 * @param label Caption text (e.g., "Folder", "Sort Options")
 * 
 * Notes:
 * - Must be called during window refresh (inside GT_BeginRefresh/EndRefresh)
 * - Uses current screen font for text rendering
 * - Caption is automatically centered horizontally
 */
/*------------------------------------------------------------------------*/
VOID DrawGroupBoxWithLabel(struct Window    *win,
                           APTR              vi,
                           struct Rectangle *box,
                           CONST_STRPTR      label);

#endif /* GUI_GROUPBOX_H */
