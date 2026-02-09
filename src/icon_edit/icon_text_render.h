/*
 * icon_text_render.h - ASCII Text-to-Pixel Renderer
 *
 * Reads an ASCII/Latin-1 text file and renders a miniature "page"
 * preview into a caller-supplied pixel buffer. Each printable character
 * maps to 1–2 horizontal pixels; each line is separated by a configurable
 * gap. The result is a recognisable "zoomed-out document" thumbnail.
 *
 * This module knows nothing about icon formats — it operates purely on
 * a rectangular pixel buffer with palette indices. The Phase 1 Icon
 * Image Access module handles all icon I/O.
 *
 * See: docs/Default icons/Customize icons/Icon_Image_Editing_Plan.md
 *      Section 4 — Phase 2
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef ICON_TEXT_RENDER_H
#define ICON_TEXT_RENDER_H

#include <exec/types.h>
#include "icon_image_access.h"   /* iTidy_TextRenderParams, iTidy_RenderParams */

/*========================================================================*/
/* Binary Detection Constants                                             */
/*========================================================================*/

/**
 * Number of bytes to scan at the start of a file for binary detection.
 * If more than ITIDY_BINARY_THRESHOLD_PERCENT of these bytes are
 * non-text, the file is treated as binary and rendering is skipped.
 */
#define ITIDY_BINARY_SCAN_BYTES     256

/**
 * Percentage threshold for binary detection.
 * If more than 10% of scanned bytes are outside the acceptable text
 * character set, the file is classified as binary.
 */
#define ITIDY_BINARY_THRESHOLD_PERCENT  10

/** Default tab stop width (characters).
 *  4 is better than the traditional 8 for tiny icon previews where
 *  the safe area may only be 34 characters wide. */
#define ITIDY_TAB_WIDTH             4

/** Horizontal downscale factor for text preview.
 *  A value of 2 means every 2 source characters map to 1 output pixel
 *  column, effectively fitting ~68 columns of an 80-column document
 *  into a 34-pixel-wide safe area.  Set to 1 to disable. */
#define ITIDY_TEXT_H_SCALE          2

/** Vertical downscale factor for text preview.
 *  A value of 2 means every 2 source lines map to 1 output pixel row,
 *  effectively fitting ~38 lines into 19 display rows.
 *  Set to 1 to disable. */
#define ITIDY_TEXT_V_SCALE          2

/*========================================================================*/
/* ASCII Text Renderer Function                                           */
/*========================================================================*/

/**
 * @brief Render a miniature ASCII text preview into a pixel buffer.
 *
 * Reads the specified file and paints a "mini-page" representation
 * into the safe area of the pixel buffer described by params.
 *
 * The function:
 *   1. Opens the source file and reads up to max_read_bytes
 *   2. Scans the first 256 bytes for binary content detection
 *   3. If binary (>10% non-text bytes), returns FALSE (skip preview)
 *   4. Fills the safe area with the background colour index
 *   5. Renders each printable character as char_pixel_width pixels
 *      of text colour; spaces/tabs as background colour
 *   6. Handles tabs (expand to next multiple of 8 positions)
 *   7. Truncates lines beyond the safe area width (no wrapping)
 *   8. Stops after max_lines or end of safe area height
 *
 * The pixel buffer, safe area rectangle, and palette indices are
 * all taken from params->base (iTidy_RenderParams).
 *
 * @param file_path     Full path to the source text file (Amiga path)
 * @param params        Rendering parameters (buffer, safe area, colours,
 *                      text-specific options like char_pixel_width)
 * @return TRUE if rendering succeeded,
 *         FALSE if file is binary, unreadable, or params are invalid
 */
BOOL itidy_render_ascii_preview(const char *file_path,
                                iTidy_TextRenderParams *params);

/**
 * @brief Check whether a file appears to be ASCII/Latin-1 text.
 *
 * Reads the first ITIDY_BINARY_SCAN_BYTES bytes and classifies each
 * byte. Acceptable bytes are:
 *   - 0x09 (TAB), 0x0A (LF), 0x0D (CR)
 *   - 0x20–0x7E (printable ASCII)
 *   - 0xA0–0xFF (printable ISO-8859-1 / Latin-1)
 *
 * @param buffer        Pointer to the file data to scan
 * @param buffer_size   Number of bytes in the buffer
 * @return TRUE if the data looks like text, FALSE if binary
 */
BOOL itidy_is_text_content(const UBYTE *buffer, ULONG buffer_size);

#endif /* ICON_TEXT_RENDER_H */
