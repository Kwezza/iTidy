#ifndef ICON_TYPES_READER_H
#define ICON_TYPES_READER_H

/**
 * @file reader.h
 * @brief Optimized icon metadata reader
 * 
 * This module provides high-performance icon reading operations that extract
 * all required icon metadata in a SINGLE disk operation. This is critical
 * for performance, especially on floppy-based systems.
 */

#include <exec/types.h>
#include "../itidy_types.h"

/*========================================================================*/
/* IconDetailsFromDisk Structure                                          */
/*========================================================================*/

/**
 * @brief Comprehensive icon details from a single disk read
 * 
 * Contains all icon metadata extracted in one GetDiskObject() call:
 * - Position, size, type, frame status
 * - Calculated display sizes with emboss/text
 * - Default tool path
 */
typedef struct {
    IconPosition position;      /* Icon X,Y coordinates */
    IconSize size;              /* Icon width and height (base bitmap only) */
    int iconType;               /* Icon format: standard, NewIcon, or OS3.5 */
    UBYTE workbenchType;        /* Workbench icon type: WBTOOL, WBPROJECT, WBDRAWER, etc. */
    BOOL hasFrame;              /* Whether icon has a border/frame */
    char *defaultTool;          /* Default tool path (caller must free) */
    BOOL isNewIcon;             /* TRUE if NewIcon format detected */
    BOOL isOS35Icon;            /* TRUE if OS3.5 format detected */
    
    /* Calculated size fields (require emboss settings and optional text) */
    int borderWidth;            /* Actual border width (0 if frameless, embossSize otherwise) */
    IconSize iconWithEmboss;    /* Bitmap + one-side emboss (e.g., 38+3=41, 11+3=14) */
    IconSize iconVisualSize;    /* Full visual footprint with borders both sides (e.g., 44x17) */
    IconSize textSize;          /* Text label dimensions (0x0 if text not provided) */
    IconSize totalDisplaySize;  /* Icon + text + gap (complete display rectangle) */
} IconDetailsFromDisk;

/*========================================================================*/
/* Optimized Icon Reader                                                  */
/*========================================================================*/

/**
 * @brief Read ALL icon details in a single disk operation
 * 
 * CRITICAL PERFORMANCE FUNCTION
 * 
 * Performs ONE GetDiskObject() call and extracts:
 * - Icon position (X, Y coordinates)
 * - Icon size (width, height) - adjusted for NewIcon/OS3.5
 * - Icon type detection (Standard, NewIcon, OS3.5)
 * - Frame status (border presence) via IconControl
 * - Default tool path (allocated, caller must free)
 * - Calculated display sizes with emboss rectangles
 * - Text size (if iconTextForFont provided)
 * 
 * This replaces multiple separate disk reads and dramatically improves
 * performance, especially on floppy-based systems.
 * 
 * @param filePath Path to icon file (with or without .info extension)
 * @param details Pointer to IconDetailsFromDisk structure to fill
 * @param iconTextForFont Icon text label for font size calculation (or NULL)
 * @return TRUE if successful, FALSE on error
 * 
 * NOTE: Caller MUST free details->defaultTool if not NULL
 */
BOOL GetIconDetailsFromDisk(const char *filePath, IconDetailsFromDisk *details, const char *iconTextForFont);

#endif /* ICON_TYPES_READER_H */
