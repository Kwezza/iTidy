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
 * @brief Calculate darkened RGB color (reduce by percentage).
 *
 * @param r       Red component (0-255)
 * @param g       Green component (0-255)
 * @param b       Blue component (0-255)
 * @param percent Darken percentage (1-100, where 100 = black, 0 = no change)
 * @param out_r   Output red
 * @param out_g   Output green
 * @param out_b   Output blue
 */
static void darken_color(UBYTE r, UBYTE g, UBYTE b, UBYTE percent,
                        UBYTE *out_r, UBYTE *out_g, UBYTE *out_b)
{
    UWORD factor;
    
    // Convert percent to reduction factor (0-100 -> 100-0)
    // 70% darker means keep 30% of original brightness
    factor = 100 - percent;
    if (factor > 100)
    {
        factor = 100;
    }
    
    *out_r = (UBYTE)((r * factor) / 100);
    *out_g = (UBYTE)((g * factor) / 100);
    *out_b = (UBYTE)((b * factor) / 100);
}

/**
 * @brief Find the closest palette entry to a target RGB color.
 *
 * Uses Euclidean distance in RGB space.
 *
 * @param palette      Palette array
 * @param palette_size Number of palette entries
 * @param target_r     Target red
 * @param target_g     Target green
 * @param target_b     Target blue
 * @return Palette index of closest color
 */
static UBYTE find_closest_palette_color(const struct ColorRegister *palette,
                                       ULONG palette_size,
                                       UBYTE target_r, UBYTE target_g, UBYTE target_b)
{
    ULONG best_dist = 999999UL;
    UBYTE best_idx = 0;
    ULONG i;
    
    for (i = 0; i < palette_size; i++)
    {
        LONG dr = (LONG)palette[i].red   - (LONG)target_r;
        LONG dg = (LONG)palette[i].green - (LONG)target_g;
        LONG db = (LONG)palette[i].blue  - (LONG)target_b;
        ULONG dist = (ULONG)(dr*dr + dg*dg + db*db);
        
        if (dist < best_dist)
        {
            best_dist = dist;
            best_idx = (UBYTE)i;
        }
    }
    
    return best_idx;
}

/**
 * @brief Build adaptive darken lookup table.
 *
 * Pre-computes darkened palette indices for fast per-pixel lookup.
 * For each palette index N, finds the palette index that's closest
 * to N darkened by darken_percent.
 *
 * @param palette       Palette array
 * @param palette_size  Number of palette entries
 * @param darken_percent Darken percentage (1-100)
 * @param out_table     Output lookup table [256]
 */
static void build_darken_table(const struct ColorRegister *palette,
                               ULONG palette_size,
                               UBYTE darken_percent,
                               UBYTE *out_table)
{
    ULONG i;
    
    for (i = 0; i < palette_size; i++)
    {
        UBYTE dark_r, dark_g, dark_b;
        
        // Calculate darkened RGB
        darken_color(palette[i].red, palette[i].green, palette[i].blue,
                    darken_percent, &dark_r, &dark_g, &dark_b);
        
        // Find closest palette match
        out_table[i] = find_closest_palette_color(palette, palette_size,
                                                   dark_r, dark_g, dark_b);
    }
    
    // Fill unused entries (palette_size to 255) with index 0
    for (i = palette_size; i < 256; i++)
    {
        out_table[i] = 0;
    }
    
    log_debug(LOG_ICONS, "build_darken_table: built table with %lu entries, darken=%u%%\n",
              palette_size, (unsigned)darken_percent);
}

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
 *
 * If bg_color_index is ITIDY_NO_BG_COLOR (255), skips filling entirely,
 * preserving the template's existing pixel data.
 */
static void fill_safe_area(const iTidy_RenderParams *p)
{
    UWORD row;
    UWORD left  = p->safe_left;
    UWORD top   = p->safe_top;
    UWORD width = p->safe_width;
    UWORD height = p->safe_height;

    // Skip filling if "no background" mode is enabled
    if (p->bg_color_index == ITIDY_NO_BG_COLOR)
    {
        return;
    }

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
 * Supports two modes:
 * 1. Fixed color mode: paint with the specified color_index
 * 2. Adaptive mode: sample background pixel, darken it via lookup table
 *
 * @param p             Render parameters (buffer, dimensions)
 * @param safe_x        X position relative to safe area left edge
 * @param safe_y        Y position relative to safe area top edge
 * @param count         Number of pixels to paint
 * @param color_index   Palette index to paint with (ignored in adaptive mode)
 * @param params        Text render params (for adaptive mode, can be NULL for fixed mode)
 */
static void paint_pixels(const iTidy_RenderParams *p,
                         UWORD safe_x, UWORD safe_y,
                         UWORD count, UBYTE color_index,
                         const iTidy_TextRenderParams *params)
{
    UWORD abs_x = p->safe_left + safe_x;
    UWORD abs_y = p->safe_top + safe_y;
    UWORD i;
    UWORD exclude_right;
    UWORD exclude_bottom;
    BOOL adaptive_mode;

    if (abs_y >= p->buffer_height)
    {
        return;
    }

    // Check if adaptive mode is enabled
    adaptive_mode = (params != NULL && params->enable_adaptive_text && 
                     params->darken_table != NULL);

    // Pre-compute exclusion area bounds (0 width/height = no exclusion)
    exclude_right  = p->exclude_left + p->exclude_width;
    exclude_bottom = p->exclude_top + p->exclude_height;

    for (i = 0; i < count; i++)
    {
        UWORD px = abs_x + i;
        ULONG pixel_offset;
        UBYTE bg_pixel;
        UBYTE paint_color;
        
        if (px >= p->safe_left + p->safe_width || px >= p->buffer_width)
        {
            break;
        }

        // Skip pixel if it falls within the exclusion area (preserves template artwork)
        if (p->exclude_width > 0 && p->exclude_height > 0)
        {
            if (px >= p->exclude_left && px < exclude_right &&
                abs_y >= p->exclude_top && abs_y < exclude_bottom)
            {
                continue;  // Skip this pixel, preserve template
            }
        }

        pixel_offset = abs_y * p->buffer_width + px;
        
        if (adaptive_mode)
        {
            // Adaptive mode: sample background, darken via lookup table
            bg_pixel = p->pixel_buffer[pixel_offset];
            paint_color = params->darken_table[bg_pixel];
        }
        else
        {
            // Fixed color mode: use specified color
            paint_color = color_index;
        }
        
        p->pixel_buffer[pixel_offset] = paint_color;
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

    /* Per-line horizontal state map for count-based anti-aliasing.
     * Each element counts printable characters seen in a downscale block.
     * 0 = no chars (background), 1 = sparse (mid-tone grey),
     * 2+ = dense (full text color). Spaces decrement the count,
     * preserving word gaps. */
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
    /* Calculate layout limits with automatic scaling                     */
    /*--------------------------------------------------------------------*/

    line_step = params->line_pixel_height + params->line_gap;
    if (line_step == 0)
    {
        line_step = 1;
    }

    out_pixels_per_line = params->base.safe_width / char_w;
    if (out_pixels_per_line == 0)
    {
        out_pixels_per_line = 1;
    }

    /* Auto-calculate horizontal scale to fit target columns into safe width.
     * Examples: 34px wide → 2:1 scale (68 cols), 20px → 4:1 scale (80 cols),
     *           10px → 7:1 scale (~70 cols). Minimum scale = 1 (no downscaling). */
    h_scale = (ITIDY_TARGET_COLUMNS + out_pixels_per_line - 1) / out_pixels_per_line;
    if (h_scale < 1)
    {
        h_scale = 1;
    }

    /* Auto-calculate vertical scale to fit target lines into safe height */
    {
        UWORD out_rows = params->base.safe_height / line_step;
        if (out_rows == 0)
        {
            out_rows = 1;
        }
        v_scale = (ITIDY_TARGET_LINES + out_rows - 1) / out_rows;
        if (v_scale < 1)
        {
            v_scale = 1;
        }
    }

    // Source chars/lines = output pixel slots * scale factor
    max_source_chars = out_pixels_per_line * h_scale;
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
    /* Build adaptive darken table (if enabled)                           */
    /*--------------------------------------------------------------------*/

    if (params->enable_adaptive_text && params->darken_table != NULL &&
        params->palette != NULL && params->palette_size > 0)
    {
        UBYTE darken_pct = params->darken_percent;
        if (darken_pct == 0)
        {
            darken_pct = ITIDY_DEFAULT_DARKEN_PERCENT;
        }
        if (darken_pct > 100)
        {
            darken_pct = 100;
        }
        
        build_darken_table(params->palette, params->palette_size,
                          darken_pct, params->darken_table);
        
        log_info(LOG_ICONS, "itidy_render_ascii_preview: adaptive text enabled, darken=%u%%\n",
                 (unsigned)darken_pct);
    }

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
    /* Render text: line-buffered count-based anti-aliasing               */
    /*                                                                    */
    /* Each source character increments a per-column counter. Spaces      */
    /* decrement the counter, preserving word gaps. At line end, the      */
    /* count threshold determines pixel color:                            */
    /*   - Count 0: background (white, no output)                         */
    /*   - Count 1: mid-tone (grey, anti-alias edge)                      */
    /*   - Count 2+: full text color (black, dense text)                  */
    /*                                                                    */
    /* When v_scale > 1, multiple source lines share the same output      */
    /* row — any line with text in a column contributes to the counter.   */
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
            // Flush: paint output pixels based on character count threshold
            UWORD out_y = (current_line / v_scale) * line_step;
            for (col = 0; col < out_pixels_per_line; col++)
            {
                UBYTE count = line_map[col];
                UBYTE color;
                
                if (count == 0)
                {
                    continue;  // Leave as background
                }
                else if (count == 1)
                {
                    // Sparse/edge — use mid-tone (grey) for anti-aliasing
                    color = params->base.mid_color_index;
                }
                else
                {
                    // Dense — use full text color (black)
                    color = params->base.text_color_index;
                }
                
                paint_pixels(&params->base, col * char_w, out_y,
                             char_w, color, params);
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
        // Handle tab — treat as whitespace (decrements count)
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
                if (out_x < out_pixels_per_line && line_map[out_x] > 0)
                {
                    line_map[out_x]--;  // Whitespace reduces count
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
        // Count-based anti-aliasing: accumulate printable chars, decrement
        // for spaces to preserve word gaps.  Threshold determines final color:
        // 0 = background, 1 = grey (mid-tone), 2+ = black (text).
        //
        {
            UWORD out_x = char_pos / h_scale;
            if (out_x < out_pixels_per_line)
            {
                if (is_printable_char(ch))
                {
                    // Increment count (cap at 255 to prevent overflow)
                    if (line_map[out_x] < 255)
                    {
                        line_map[out_x]++;
                    }
                }
                else if (ch == 0x20)
                {
                    // Space decrements count (preserves word gaps)
                    if (line_map[out_x] > 0)
                    {
                        line_map[out_x]--;
                    }
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
            UBYTE count = line_map[col];
            UBYTE color;
            
            if (count == 0)
            {
                continue;  // Leave as background
            }
            else if (count == 1)
            {
                // Sparse — use mid-tone (grey)
                color = params->base.mid_color_index;
            }
            else
            {
                // Dense — use full text color (black)
                color = params->base.text_color_index;
            }
            
            paint_pixels(&params->base, col * char_w, out_y,
                         char_w, color, params);
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
