#ifndef ITIDY_PATH_UTILITIES_H
#define ITIDY_PATH_UTILITIES_H

/*============================================================================*/
/* Path Utilities Module - Global path manipulation functions                */
/*============================================================================*/
/**
 * @file path_utilities.h
 * @brief Reusable path truncation and abbreviation utilities for iTidy
 * 
 * This module provides consistent path handling across all GUI windows,
 * supporting both fixed-width and proportional fonts.
 * 
 * Created: 2025-11-13
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include <exec/types.h>
#include <graphics/rastport.h>

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Truncate a path in the middle preserving device and filename
 * 
 * For Amiga paths like "Workbench:Programs/Wordworth7/Wordworth", this will
 * truncate to show the device/volume and the filename while shortening the
 * middle directories, e.g.: "Workbench:...Wordworth"
 * 
 * Best used with fixed-width fonts in ListViews for columnar data display.
 * Uses character counts rather than pixel measurements.
 * 
 * @param path Original path to truncate (must not be NULL)
 * @param output Buffer to store result (must be at least max_chars+1 bytes)
 * @param max_chars Maximum character length for output (excluding null terminator)
 * 
 * Example:
 *   char shortened[41];
 *   iTidy_TruncatePathMiddle("Workbench:Programs/Wordworth7/Wordworth", 
 *                             shortened, 40);
 *   // Result: "Workbench:...Wordworth"
 * 
 * @note Output buffer is always null-terminated, even if path is truncated
 * @note If path is NULL, output will be set to empty string
 * @note For paths without separators, performs simple middle truncation
 */
void iTidy_TruncatePathMiddle(const char *path, char *output, int max_chars);

/**
 * @brief Truncate a path in the middle using pixel measurements
 * 
 * Similar to iTidy_TruncatePathMiddle() but uses TextLength() to measure
 * pixel width instead of character counts. This makes it suitable for
 * proportional fonts where characters have varying widths.
 * 
 * Performs balanced middle truncation: "Work:Prog.../Tool"
 * 
 * @param rp RastPort with font already set via SetFont() (must not be NULL)
 * @param path Original path to truncate (must not be NULL)
 * @param output Buffer to store result (recommended at least 256 bytes)
 * @param max_width Maximum pixel width for output text
 * 
 * Example:
 *   struct RastPort *rp;  // Already initialized with SetFont()
 *   char shortened[256];
 *   iTidy_TruncatePathMiddlePixels(rp, "Work:Projects/Programming/Amiga/Tool",
 *                                   shortened, 150);
 *   // Result depends on font metrics: "Work:Proj.../Tool"
 * 
 * @note Requires RastPort with valid font set before calling
 * @note If path fits within max_width, output will be unchanged
 * @note If RastPort is NULL, output will be set to empty string (defensive)
 * @note Buffer overflow protection: output is always null-terminated
 */
void iTidy_TruncatePathMiddlePixels(struct RastPort *rp,
                                     const char *path,
                                     char *output,
                                     UWORD max_width);

/**
 * @brief Intelligent Amiga-style path abbreviation with /../
 * 
 * Shortens paths using the Amiga convention of collapsing middle directories
 * with "/../" notation while preserving context from device, first directory,
 * and filename.
 * 
 * Strategy:
 * - Always preserve device/volume name (before first :)
 * - Always preserve filename (last component)
 * - Preserve first directory level for context
 * - Collapse middle directories with /../
 * 
 * @param path Original path to abbreviate (must not be NULL)
 * @param output Buffer to store result (must be at least max_chars+1 bytes)
 * @param max_chars Maximum character length for output (excluding null terminator)
 * @return TRUE if path was shortened, FALSE if path already fits
 * 
 * Examples:
 *   char abbrev[51];
 *   
 *   // Long path gets abbreviated:
 *   iTidy_ShortenPathWithParentDir("Work:Projects/Programming/Amiga/iTidy/src/GUI/windows/tool.c",
 *                                   abbrev, 50);
 *   // Result: "Work:Projects/../GUI/windows/tool.c"
 *   // Returns: TRUE
 *   
 *   // Very long path gets more aggressive:
 *   iTidy_ShortenPathWithParentDir("Work:Projects/Programming/Amiga/iTidy/src/GUI/windows/tool.c",
 *                                   abbrev, 40);
 *   // Result: "Work:Projects/../tool.c"
 *   // Returns: TRUE
 *   
 *   // Short path unchanged:
 *   iTidy_ShortenPathWithParentDir("SYS:Utilities/More", abbrev, 40);
 *   // Result: "SYS:Utilities/More"
 *   // Returns: FALSE
 * 
 * @note Uses character counts, not pixel measurements
 * @note If path is NULL, output will be set to empty string and returns FALSE
 * @note Output is always null-terminated
 */
BOOL iTidy_ShortenPathWithParentDir(const char *path,
                                     char *output,
                                     int max_chars);

#endif /* ITIDY_PATH_UTILITIES_H */
