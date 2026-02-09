/*
 * icon_text_render.c - ASCII Text-to-Pixel Renderer
 *
 * Reads an ASCII/Latin-1 text file and renders a miniature "page"
 * preview into a pixel buffer. Each printable character becomes 1–2
 * pixels of foreground colour; whitespace stays as background colour.
 * The result is a recognisable "zoomed-out document" thumbnail.
 *
 * This module is format-agnostic — it paints into a plain pixel
 * buffer using palette indices. The caller (Phase 3 integration)
 * handles all icon I/O via the Phase 1 Icon Image Access module.
 *
 * See: docs/Default icons/Customize icons/Icon_Image_Editing_Plan.md
 *      Section 4 — Phase 2
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stdio.h>
#include <string.h>

#include <console_output.h>

#include "icon_text_render.h"
#include "../writeLog.h"

/*========================================================================*/
/* Internal: Character Classification                                     */
/*========================================================================*/

/**
 * @brief Check if a byte is an acceptable text character.
 *
 * Acceptable bytes for text content:
 *   - 0x09 (TAB)
 *   - 0x0A (LF)
 *   - 0x0D (CR)
 *   - 0x20–0x7E (printable ASCII)
 *   - 0xA0–0xFF (printable ISO-8859-1 / Latin-1)
 */
static BOOL is_text_byte(UBYTE ch)
{
    if (ch == 0x09 || ch == 0x0A || ch == 0x0D)
    {
        return TRUE;
    }
    if (ch >= 0x20 && ch <= 0x7E)
    {
        return TRUE;
    }
    if (ch >= 0xA0)  /* 0xA0–0xFF: Latin-1 printable */
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Check if a byte is a printable character that should render
 *        as foreground (text colour) pixels.
 *
 * Printable = visible glyph. Excludes space (0x20), tab, CR, LF,
 * and control characters — those render as background.
 */
static BOOL is_printable_char(UBYTE ch)
{
    if (ch >= 0x21 && ch <= 0x7E)
    {
        return TRUE;
    }
    if (ch >= 0xA0)  /* 0xA0–0xFF: Latin-1 printable */
    {
        return TRUE;
    }
    return FALSE;
}

/*========================================================================*/
/* itidy_is_text_content — Binary Detection                               */
/*========================================================================*/

BOOL itidy_is_text_content(const UBYTE *buffer, ULONG buffer_size)
{
    ULONG i;
    ULONG non_text_count = 0;
    ULONG threshold;

    if (buffer == NULL || buffer_size == 0)
    {
        return FALSE;
    }

    for (i = 0; i < buffer_size; i++)
    {
        if (!is_text_byte(buffer[i]))
        {
            non_text_count++;
        }
    }

    // Threshold: more than 10% non-text = binary
    threshold = (buffer_size * ITIDY_BINARY_THRESHOLD_PERCENT) / 100;
    if (threshold == 0)
    {
        threshold = 1;  // At least 1 byte tolerance for very small files
    }

    if (non_text_count > threshold)
    {
        log_info(LOG_ICONS, "itidy_is_text_content: binary detected "
                 "(%lu/%lu non-text bytes, threshold=%lu)\n",
                 non_text_count, buffer_size, threshold);
        return FALSE;
    }

    return TRUE;
}

/*========================================================================*/
/* Internal: Safe Area Fill                                               */
/*========================================================================*/

/**
 * @brief Fill the safe area rectangle with the background colour index.
 *
 * Only fills within the safe area — pixels outside are untouched
 * (preserving template border artwork).
 */
static void fill_safe_area(const iTidy_RenderParams *p)
{
    UWORD row;
    UWORD left  = p->safe_left;
    UWORD top   = p->safe_top;
    UWORD width = p->safe_width;
    UWORD height = p->safe_height;

    for (row = 0; row < height; row++)
    {
        UWORD y = top + row;
        if (y >= p->buffer_height)
        {
            break;
        }

        // Calculate start of this row within the safe area
        UBYTE *row_ptr = p->pixel_buffer + (y * p->buffer_width) + left;

        // Clamp width to buffer bounds
        UWORD fill_width = width;
        if (left + fill_width > p->buffer_width)
        {
            fill_width = p->buffer_width - left;
        }

        memset(row_ptr, p->bg_color_index, fill_width);
    }
}

/*========================================================================*/
/* Internal: Paint Pixels                                                 */
/*========================================================================*/

/**
 * @brief Paint a horizontal run of pixels at a position within the safe area.
 *
 * @param p             Render parameters (buffer, dimensions)
 * @param safe_x        X position relative to safe area left edge
 * @param safe_y        Y position relative to safe area top edge
 * @param count         Number of pixels to paint
 * @param color_index   Palette index to paint with
 */
static void paint_pixels(const iTidy_RenderParams *p,
                         UWORD safe_x, UWORD safe_y,
                         UWORD count, UBYTE color_index)
{
    UWORD abs_x = p->safe_left + safe_x;
    UWORD abs_y = p->safe_top + safe_y;
    UWORD i;

    if (abs_y >= p->buffer_height)
    {
        return;
    }

    for (i = 0; i < count; i++)
    {
        UWORD px = abs_x + i;
        if (px >= p->safe_left + p->safe_width || px >= p->buffer_width)
        {
            break;
        }

        p->pixel_buffer[abs_y * p->buffer_width + px] = color_index;
    }
}

/*========================================================================*/
/* itidy_render_ascii_preview — Main Renderer                             */
/*========================================================================*/

BOOL itidy_render_ascii_preview(const char *file_path,
                                iTidy_TextRenderParams *params)
{
    BPTR fh = 0;
    UBYTE *read_buffer = NULL;
    LONG bytes_read;
    UWORD max_source_chars;     /* Max source characters per line (after scaling) */
    UWORD max_source_lines;     /* Max source lines to process (after scaling) */
    UWORD line_step;
    UWORD current_line = 0;
    UWORD char_pos = 0;         /* Source character position on current line */
    UWORD h_scale;              /* Horizontal downscale factor */
    UWORD v_scale;              /* Vertical downscale factor */
    ULONG read_pos;
    ULONG read_size;
    UWORD char_w;
    UWORD scan_size;
    UWORD out_pixels_per_line;  /* Output pixel columns in safe area */
    UWORD col;                  /* Flush loop iterator */

    /* Per-line horizontal state map for AND word-gap semantics.
     * 0 = no char seen, 1 = printable (paint), 2 = has space (gap forced).
     * Any space in a 2-char downscale block kills the output pixel,
     * preserving visible word gaps in zoomed-out text. */
    UBYTE line_map[128];

    /*--------------------------------------------------------------------*/
    /* Validate parameters                                                */
    /*--------------------------------------------------------------------*/

    if (file_path == NULL || params == NULL)
    {
        log_error(LOG_ICONS, "itidy_render_ascii_preview: NULL parameter\n");
        return FALSE;
    }

    if (params->base.pixel_buffer == NULL)
    {
        log_error(LOG_ICONS, "itidy_render_ascii_preview: NULL pixel buffer\n");
        return FALSE;
    }

    if (params->base.safe_width == 0 || params->base.safe_height == 0)
    {
        log_error(LOG_ICONS, "itidy_render_ascii_preview: zero-size safe area\n");
        return FALSE;
    }

    char_w = params->char_pixel_width;
    if (char_w == 0)
    {
        char_w = 1;  // Fallback — should have been resolved by itidy_get_render_params
    }

    /*--------------------------------------------------------------------*/
    /* Calculate layout limits                                            */
    /*--------------------------------------------------------------------*/

    h_scale = ITIDY_TEXT_H_SCALE;
    v_scale = ITIDY_TEXT_V_SCALE;

    line_step = params->line_pixel_height + params->line_gap;
    if (line_step == 0)
    {
        line_step = 1;
    }

    // Source chars/lines = output pixel slots * scale factor
    max_source_chars = (params->base.safe_width / char_w) * h_scale;
    if (max_source_chars == 0)
    {
        max_source_chars = 1;
    }

    max_source_lines = (params->base.safe_height / line_step) * v_scale;
    if (max_source_lines == 0)
    {
        max_source_lines = 1;
    }

    out_pixels_per_line = params->base.safe_width / char_w;
    if (out_pixels_per_line > 128)
    {
        out_pixels_per_line = 128;
    }

    // Determine read size
    read_size = params->max_read_bytes;
    if (read_size == 0)
    {
        read_size = ITIDY_DEFAULT_READ_BYTES;
    }

    log_info(LOG_ICONS, "itidy_render_ascii_preview: file='%s' safe=%ux%u "
             "char_w=%u scale=%ux%u src_chars=%u src_lines=%u read=%lu\n",
             file_path,
             (unsigned)params->base.safe_width,
             (unsigned)params->base.safe_height,
             (unsigned)char_w,
             (unsigned)h_scale, (unsigned)v_scale,
             (unsigned)max_source_chars,
             (unsigned)max_source_lines,
             (unsigned long)read_size);

    /*--------------------------------------------------------------------*/
    /* Read file into buffer                                              */
    /*--------------------------------------------------------------------*/

    read_buffer = (UBYTE *)whd_malloc(read_size);
    if (read_buffer == NULL)
    {
        log_error(LOG_ICONS, "itidy_render_ascii_preview: alloc failed (%lu bytes)\n",
                  (unsigned long)read_size);
        return FALSE;
    }

    fh = Open((STRPTR)file_path, MODE_OLDFILE);
    if (fh == 0)
    {
        log_error(LOG_ICONS, "itidy_render_ascii_preview: cannot open '%s'\n", file_path);
        whd_free(read_buffer);
        return FALSE;
    }

    bytes_read = Read(fh, read_buffer, read_size);
    Close(fh);
    fh = 0;

    if (bytes_read <= 0)
    {
        log_warning(LOG_ICONS, "itidy_render_ascii_preview: empty or unreadable file '%s'\n",
                    file_path);
        whd_free(read_buffer);
        return FALSE;
    }

    /*--------------------------------------------------------------------*/
    /* Binary detection: scan first N bytes                               */
    /*--------------------------------------------------------------------*/

    scan_size = (UWORD)((bytes_read < ITIDY_BINARY_SCAN_BYTES) ?
                         bytes_read : ITIDY_BINARY_SCAN_BYTES);

    if (!itidy_is_text_content(read_buffer, (ULONG)scan_size))
    {
        log_info(LOG_ICONS, "itidy_render_ascii_preview: binary file detected, skipping '%s'\n",
                 file_path);
        whd_free(read_buffer);
        return FALSE;
    }

    /*--------------------------------------------------------------------*/
    /* Fill safe area with background colour                              */
    /*--------------------------------------------------------------------*/

    fill_safe_area(&params->base);

    /*--------------------------------------------------------------------*/
    /* Render text: line-buffered AND semantics with vertical OR          */
    /*                                                                    */
    /* Horizontal AND: for each h_scale-char block that maps to one       */
    /* output pixel, ALL source chars must be printable.  If ANY char     */
    /* in the block is a space, the output pixel stays as background.     */
    /* This preserves visible word gaps in zoomed-out text.               */
    /*                                                                    */
    /* Vertical OR: each source line paints independently into its        */
    /* output row.  When v_scale > 1, multiple source lines share the     */
    /* same output row — any line with text in a column lights it up.     */
    /*--------------------------------------------------------------------*/

    memset(line_map, 0, out_pixels_per_line);
    char_pos = 0;

    for (read_pos = 0; read_pos < (ULONG)bytes_read; read_pos++)
    {
        UBYTE ch = read_buffer[read_pos];

        // Check if we've exhausted the vertical space (in source lines)
        if (current_line >= max_source_lines)
        {
            break;
        }

        //
        // Handle newline (LF) — flush line_map, advance to next line
        //
        if (ch == 0x0A)
        {
            // Flush: paint output pixels that are PRINTABLE (state 1)
            UWORD out_y = (current_line / v_scale) * line_step;
            for (col = 0; col < out_pixels_per_line; col++)
            {
                if (line_map[col] == 1)
                {
                    paint_pixels(&params->base, col * char_w, out_y,
                                 char_w, params->base.text_color_index);
                }
            }

            // Reset map for the next source line
            memset(line_map, 0, out_pixels_per_line);
            current_line++;
            char_pos = 0;
            continue;
        }

        //
        // Handle carriage return (CR) — skip it
        // Handles both bare CR and CR+LF sequences gracefully
        //
        if (ch == 0x0D)
        {
            continue;
        }

        //
        // Handle tab — mark whitespace positions in line_map
        //
        if (ch == 0x09)
        {
            UWORD next_tab = ((char_pos / ITIDY_TAB_WIDTH) + 1) * ITIDY_TAB_WIDTH;
            if (next_tab > max_source_chars)
            {
                next_tab = max_source_chars;
            }
            while (char_pos < next_tab)
            {
                UWORD out_x = char_pos / h_scale;
                if (out_x < out_pixels_per_line)
                {
                    line_map[out_x] = 2;  // Whitespace kills pixel
                }
                char_pos++;
            }
            continue;
        }

        //
        // Beyond source line width — silently truncate (no wrapping)
        //
        if (char_pos >= max_source_chars)
        {
            continue;  // Skip remaining chars until newline
        }

        //
        // Map source character to output column and update line_map.
        // AND semantics: a space anywhere in a scale-block forces the
        // output pixel to stay as background (gap).  Printable chars
        // only light a pixel if no space has been seen in that block.
        //
        {
            UWORD out_x = char_pos / h_scale;
            if (out_x < out_pixels_per_line)
            {
                if (is_printable_char(ch))
                {
                    // Set printable only if not already killed by a space
                    if (line_map[out_x] == 0)
                    {
                        line_map[out_x] = 1;
                    }
                    // State 1 stays 1 (still printable)
                    // State 2 stays 2 (space already killed it)
                }
                else if (ch == 0x20)
                {
                    // Space forces gap — overrides printable
                    line_map[out_x] = 2;
                }
            }
        }

        char_pos++;
    }

    // Flush the final line (file may not end with a trailing newline)
    if (current_line < max_source_lines)
    {
        UWORD out_y = (current_line / v_scale) * line_step;
        for (col = 0; col < out_pixels_per_line; col++)
        {
            if (line_map[col] == 1)
            {
                paint_pixels(&params->base, col * char_w, out_y,
                             char_w, params->base.text_color_index);
            }
        }
    }

    /*--------------------------------------------------------------------*/
    /* Cleanup and return                                                 */
    /*--------------------------------------------------------------------*/

    whd_free(read_buffer);

    log_info(LOG_ICONS, "itidy_render_ascii_preview: rendered %u source lines "
             "(%ux%u scale) from '%s'\n",
             (unsigned)(current_line + 1),
             (unsigned)h_scale, (unsigned)v_scale,
             file_path);

    return TRUE;
}
