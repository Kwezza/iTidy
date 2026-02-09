/*
 * icon_image_access.c - Icon Image Access Module (Reusable Core)
 *
 * Implementation of the icon image extraction, modification, and save
 * pipeline. Uses IconControlA() for palette-mapped image access,
 * DupDiskObjectA() for safe cloning, and PutIconTagList() for OS3.5
 * format output.
 *
 * See: docs/Default icons/Customize icons/Icon_Image_Editing_Plan.md
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <utility/tagitem.h>
#include <datatypes/pictureclass.h>  /* struct ColorRegister */

#include <console_output.h>

#include "icon_image_access.h"
#include "../writeLog.h"
#include "../icon_types.h"

/*========================================================================*/
/* Internal Helper Prototypes                                             */
/*========================================================================*/

static ULONG calculate_luminance(UBYTE r, UBYTE g, UBYTE b);
static LONG parse_integer_tooltype(STRPTR *tool_types, const char *key, LONG default_val);
static BOOL parse_area_tooltype(STRPTR *tool_types, const char *key,
                                UWORD *out_x, UWORD *out_y,
                                UWORD *out_w, UWORD *out_h);

/*========================================================================*/
/* itidy_icon_image_extract                                               */
/*========================================================================*/

BOOL itidy_icon_image_extract(struct DiskObject *icon, iTidy_IconImageData *out)
{
    LONG is_palette_mapped = 0;
    ULONG width = 0;
    ULONG height = 0;
    UBYTE *src_pixels_1 = NULL;
    UBYTE *src_pixels_2 = NULL;
    struct ColorRegister *src_pal_1 = NULL;
    struct ColorRegister *src_pal_2 = NULL;
    ULONG pal_size_1 = 0;
    ULONG pal_size_2 = 0;
    LONG trans_color_1 = -1;
    LONG trans_color_2 = -1;
    LONG has_real_image_2 = 0;
    LONG is_frameless = 0;
    ULONG pixel_count;

    if (icon == NULL || out == NULL)
    {
        log_error(LOG_ICONS, "itidy_icon_image_extract: NULL parameter\n");
        return FALSE;
    }

    // Zero the output structure
    memset(out, 0, sizeof(iTidy_IconImageData));

    // Query whether the icon is palette-mapped
    {
        struct TagItem query_pm_tags[2];
        query_pm_tags[0].ti_Tag  = ICONCTRLA_IsPaletteMapped;
        query_pm_tags[0].ti_Data = (ULONG)&is_palette_mapped;
        query_pm_tags[1].ti_Tag  = TAG_DONE;
        query_pm_tags[1].ti_Data = 0;
        IconControlA(icon, query_pm_tags);
    }

    if (!is_palette_mapped)
    {
        // Attempt to convert the icon to palette-mapped via LayoutIconA
        // This makes icon.library rasterise old planar icons into chunky data
        log_info(LOG_ICONS, "Icon is not palette-mapped, attempting LayoutIconA conversion\n");

        {
            struct TagItem layout_tags[1];
            layout_tags[0].ti_Tag = TAG_DONE;
            layout_tags[0].ti_Data = 0;
            LayoutIconA(icon, NULL, layout_tags);
        }

        // Re-check palette-mapped status
        {
            struct TagItem recheck_tags[2];
            recheck_tags[0].ti_Tag  = ICONCTRLA_IsPaletteMapped;
            recheck_tags[0].ti_Data = (ULONG)&is_palette_mapped;
            recheck_tags[1].ti_Tag  = TAG_DONE;
            recheck_tags[1].ti_Data = 0;
            IconControlA(icon, recheck_tags);
        }

        if (!is_palette_mapped)
        {
            log_warning(LOG_ICONS, "Icon still not palette-mapped after LayoutIconA — cannot extract\n");
            return FALSE;
        }
    }

    out->is_palette_mapped = TRUE;

    // Extract dimensions, pixel data, palettes, transparency, and frameless setting
    {
        struct TagItem get_tags[15];
        int t = 0;

        get_tags[t].ti_Tag  = ICONCTRLA_GetWidth;
        get_tags[t].ti_Data = (ULONG)&width;
        t++;

        get_tags[t].ti_Tag  = ICONCTRLA_GetHeight;
        get_tags[t].ti_Data = (ULONG)&height;
        t++;

        get_tags[t].ti_Tag  = ICONCTRLA_GetImageData1;
        get_tags[t].ti_Data = (ULONG)&src_pixels_1;
        t++;

        get_tags[t].ti_Tag  = ICONCTRLA_GetImageData2;
        get_tags[t].ti_Data = (ULONG)&src_pixels_2;
        t++;

        get_tags[t].ti_Tag  = ICONCTRLA_GetPalette1;
        get_tags[t].ti_Data = (ULONG)&src_pal_1;
        t++;

        get_tags[t].ti_Tag  = ICONCTRLA_GetPalette2;
        get_tags[t].ti_Data = (ULONG)&src_pal_2;
        t++;

        get_tags[t].ti_Tag  = ICONCTRLA_GetPaletteSize1;
        get_tags[t].ti_Data = (ULONG)&pal_size_1;
        t++;

        get_tags[t].ti_Tag  = ICONCTRLA_GetPaletteSize2;
        get_tags[t].ti_Data = (ULONG)&pal_size_2;
        t++;

        get_tags[t].ti_Tag  = ICONCTRLA_GetTransparentColor1;
        get_tags[t].ti_Data = (ULONG)&trans_color_1;
        t++;

        get_tags[t].ti_Tag  = ICONCTRLA_GetTransparentColor2;
        get_tags[t].ti_Data = (ULONG)&trans_color_2;
        t++;

        get_tags[t].ti_Tag  = ICONCTRLA_HasRealImage2;
        get_tags[t].ti_Data = (ULONG)&has_real_image_2;
        t++;

        get_tags[t].ti_Tag  = ICONCTRLA_GetFrameless;
        get_tags[t].ti_Data = (ULONG)&is_frameless;
        t++;

        get_tags[t].ti_Tag  = TAG_DONE;
        get_tags[t].ti_Data = 0;

        IconControlA(icon, get_tags);
    }

    // Validate essential data
    if (width == 0 || height == 0 || src_pixels_1 == NULL)
    {
        log_error(LOG_ICONS, "itidy_icon_image_extract: missing essential data "
                  "(w=%lu h=%lu pix1=%p)\n",
                  width, height, (void *)src_pixels_1);
        return FALSE;
    }

    if (src_pal_1 == NULL || pal_size_1 == 0)
    {
        log_error(LOG_ICONS, "itidy_icon_image_extract: icon has pixel data "
                  "but no palette (w=%lu h=%lu pix1=%p pal1=%p palsize=%lu). "
                  "Ensure icon is loaded with GetIconTagList() not GetDiskObject() "
                  "so palette-mapped data is properly activated.\n",
                  width, height, (void *)src_pixels_1,
                  (void *)src_pal_1, pal_size_1);
        return FALSE;
    }

    out->width  = (UWORD)width;
    out->height = (UWORD)height;
    pixel_count = width * height;

    // Determine source format for diagnostics
    // (We already know it's palette-mapped at this point)
    out->source_format = ITIDY_ICON_FORMAT_OS35;

    // Copy normal image pixel data (IconControlA returns pointer into icon's
    // own data — we MUST copy before the icon is freed)
    out->pixel_data_normal = (UBYTE *)whd_malloc(pixel_count);
    if (out->pixel_data_normal == NULL)
    {
        log_error(LOG_ICONS, "itidy_icon_image_extract: failed to alloc normal pixel buffer (%lu bytes)\n",
                  pixel_count);
        itidy_icon_image_free(out);
        return FALSE;
    }
    memcpy(out->pixel_data_normal, src_pixels_1, pixel_count);

    // Copy normal palette
    out->palette_size_normal = pal_size_1;
    out->palette_normal = (struct ColorRegister *)whd_malloc(pal_size_1 * sizeof(struct ColorRegister));
    if (out->palette_normal == NULL)
    {
        log_error(LOG_ICONS, "itidy_icon_image_extract: failed to alloc normal palette (%lu entries)\n",
                  pal_size_1);
        itidy_icon_image_free(out);
        return FALSE;
    }
    memcpy(out->palette_normal, src_pal_1, pal_size_1 * sizeof(struct ColorRegister));

    // Copy selected image data if present
    if (src_pixels_2 != NULL && src_pal_2 != NULL && pal_size_2 > 0)
    {
        out->has_selected_image = TRUE;

        out->pixel_data_selected = (UBYTE *)whd_malloc(pixel_count);
        if (out->pixel_data_selected == NULL)
        {
            log_error(LOG_ICONS, "itidy_icon_image_extract: failed to alloc selected pixel buffer\n");
            itidy_icon_image_free(out);
            return FALSE;
        }
        memcpy(out->pixel_data_selected, src_pixels_2, pixel_count);

        out->palette_size_selected = pal_size_2;
        out->palette_selected = (struct ColorRegister *)whd_malloc(pal_size_2 * sizeof(struct ColorRegister));
        if (out->palette_selected == NULL)
        {
            log_error(LOG_ICONS, "itidy_icon_image_extract: failed to alloc selected palette\n");
            itidy_icon_image_free(out);
            return FALSE;
        }
        memcpy(out->palette_selected, src_pal_2, pal_size_2 * sizeof(struct ColorRegister));
    }
    else
    {
        out->has_selected_image = FALSE;
    }

    // Store transparent color indices
    out->transparent_color_normal = trans_color_1;
    out->transparent_color_selected = trans_color_2;

    // Store gadget flags (controls border/frame/highlight rendering)
    out->gadget_flags = icon->do_Gadget.Flags;

    // Store whether this icon has a "real" selected image or uses auto-dimming
    out->has_real_selected_image = (has_real_image_2 != 0);

    // Store frameless setting (non-zero = frameless)
    out->is_frameless = is_frameless;

    log_info(LOG_ICONS, "itidy_icon_image_extract: %ux%u, pal=%lu entries, selected=%s, real_sel=%s, trans=(%ld,%ld), gflags=0x%04x, frameless=%s\n",
             (unsigned)out->width, (unsigned)out->height,
             out->palette_size_normal,
             out->has_selected_image ? "yes" : "no",
             out->has_real_selected_image ? "yes" : "no",
             trans_color_1, trans_color_2,
             (unsigned)out->gadget_flags,
             is_frameless ? "yes" : "no");

    return TRUE;
}

/*========================================================================*/
/* itidy_icon_image_apply                                                 */
/*========================================================================*/

struct DiskObject *itidy_icon_image_apply(struct DiskObject *icon,
                                          const iTidy_IconImageData *data)
{
    struct DiskObject *clone = NULL;
    int t;

    if (icon == NULL || data == NULL)
    {
        log_error(LOG_ICONS, "itidy_icon_image_apply: NULL parameter\n");
        return NULL;
    }

    if (data->pixel_data_normal == NULL || data->palette_normal == NULL)
    {
        log_error(LOG_ICONS, "itidy_icon_image_apply: no normal image data to apply\n");
        return NULL;
    }

    // Clone the original icon — this preserves all metadata
    {
        struct TagItem dup_tags[1];
        dup_tags[0].ti_Tag  = TAG_DONE;
        dup_tags[0].ti_Data = 0;
        clone = DupDiskObjectA(icon, dup_tags);
    }

    if (clone == NULL)
    {
        log_error(LOG_ICONS, "itidy_icon_image_apply: DupDiskObjectA failed\n");
        return NULL;
    }

    // Apply gadget flags (controls border/frame/highlight rendering)
    clone->do_Gadget.Flags = data->gadget_flags;

    // Apply frameless setting using IconControlA (proper high-level API)
    {
        struct TagItem frameless_tags[2];
        frameless_tags[0].ti_Tag  = ICONCTRLA_SetFrameless;
        frameless_tags[0].ti_Data = (data->is_frameless != 0) ? TRUE : FALSE;
        frameless_tags[1].ti_Tag  = TAG_DONE;
        frameless_tags[1].ti_Data = 0;
        IconControlA(clone, frameless_tags);
    }

    // Apply the modified image data to the clone
    {
        struct TagItem set_tags[12];
        t = 0;

        set_tags[t].ti_Tag  = ICONCTRLA_SetWidth;
        set_tags[t].ti_Data = (ULONG)data->width;
        t++;

        set_tags[t].ti_Tag  = ICONCTRLA_SetHeight;
        set_tags[t].ti_Data = (ULONG)data->height;
        t++;

        set_tags[t].ti_Tag  = ICONCTRLA_SetPaletteSize1;
        set_tags[t].ti_Data = data->palette_size_normal;
        t++;

        set_tags[t].ti_Tag  = ICONCTRLA_SetPalette1;
        set_tags[t].ti_Data = (ULONG)data->palette_normal;
        t++;

        set_tags[t].ti_Tag  = ICONCTRLA_SetImageData1;
        set_tags[t].ti_Data = (ULONG)data->pixel_data_normal;
        t++;

        set_tags[t].ti_Tag  = ICONCTRLA_SetTransparentColor1;
        set_tags[t].ti_Data = (ULONG)data->transparent_color_normal;
        t++;

        // Apply selected image data ONLY if template had a real selected image
        // (not auto-dimmed). If template used auto-dimming, icon.library will
        // automatically generate a dimmed version for the new icon too.
        if (data->has_real_selected_image &&
            data->pixel_data_selected != NULL && data->palette_selected != NULL)
        {
            set_tags[t].ti_Tag  = ICONCTRLA_SetPaletteSize2;
            set_tags[t].ti_Data = data->palette_size_selected;
            t++;

            set_tags[t].ti_Tag  = ICONCTRLA_SetPalette2;
            set_tags[t].ti_Data = (ULONG)data->palette_selected;
            t++;

            set_tags[t].ti_Tag  = ICONCTRLA_SetImageData2;
            set_tags[t].ti_Data = (ULONG)data->pixel_data_selected;
            t++;

            set_tags[t].ti_Tag  = ICONCTRLA_SetTransparentColor2;
            set_tags[t].ti_Data = (ULONG)data->transparent_color_selected;
            t++;
        }

        set_tags[t].ti_Tag  = TAG_DONE;
        set_tags[t].ti_Data = 0;

        IconControlA(clone, set_tags);
    }

    // LayoutIconA is required after modifying image data to produce
    // a displayable icon
    {
        struct TagItem layout_tags[1];
        layout_tags[0].ti_Tag  = TAG_DONE;
        layout_tags[0].ti_Data = 0;
        LayoutIconA(clone, NULL, layout_tags);
    }

    log_info(LOG_ICONS, "itidy_icon_image_apply: applied %ux%u image to clone, gflags=0x%04x, frameless=%s, real_sel=%s\n",
             (unsigned)data->width, (unsigned)data->height,
             (unsigned)data->gadget_flags,
             data->is_frameless ? "yes" : "no",
             data->has_real_selected_image ? "yes" : "no (auto-dim)");

    return clone;
}

/*========================================================================*/
/* itidy_icon_image_save                                                  */
/*========================================================================*/

BOOL itidy_icon_image_save(const char *path, struct DiskObject *icon)
{
    BOOL result;
    struct TagItem put_tags[4];

    if (path == NULL || icon == NULL)
    {
        log_error(LOG_ICONS, "itidy_icon_image_save: NULL parameter\n");
        return FALSE;
    }

    // Save as OS3.5 format — drop planar and NewIcon data
    put_tags[0].ti_Tag  = ICONPUTA_NotifyWorkbench;
    put_tags[0].ti_Data = TRUE;

    put_tags[1].ti_Tag  = ICONPUTA_DropPlanarIconImage;
    put_tags[1].ti_Data = TRUE;

    put_tags[2].ti_Tag  = ICONPUTA_DropNewIconToolTypes;
    put_tags[2].ti_Data = TRUE;

    put_tags[3].ti_Tag  = TAG_DONE;
    put_tags[3].ti_Data = 0;

    result = PutIconTagList((STRPTR)path, icon, put_tags);

    if (result)
    {
        log_info(LOG_ICONS, "itidy_icon_image_save: saved icon to '%s' (OS3.5 format)\n", path);
    }
    else
    {
        log_error(LOG_ICONS, "itidy_icon_image_save: PutIconTagList failed for '%s'\n", path);
    }

    return result;
}

/*========================================================================*/
/* itidy_icon_image_free                                                  */
/*========================================================================*/

void itidy_icon_image_free(iTidy_IconImageData *data)
{
    if (data == NULL)
    {
        return;
    }

    if (data->pixel_data_normal != NULL)
    {
        whd_free(data->pixel_data_normal);
        data->pixel_data_normal = NULL;
    }

    if (data->pixel_data_selected != NULL)
    {
        whd_free(data->pixel_data_selected);
        data->pixel_data_selected = NULL;
    }

    if (data->palette_normal != NULL)
    {
        whd_free(data->palette_normal);
        data->palette_normal = NULL;
    }

    if (data->palette_selected != NULL)
    {
        whd_free(data->palette_selected);
        data->palette_selected = NULL;
    }

    data->palette_size_normal = 0;
    data->palette_size_selected = 0;
    data->width = 0;
    data->height = 0;
    data->has_selected_image = FALSE;
    data->is_palette_mapped = FALSE;
}

/*========================================================================*/
/* itidy_icon_image_create_blank                                          */
/*========================================================================*/

BOOL itidy_icon_image_create_blank(UWORD w, UWORD h, UBYTE bg_index,
                                   iTidy_IconImageData *out)
{
    ULONG pixel_count;

    if (out == NULL || w == 0 || h == 0)
    {
        return FALSE;
    }

    memset(out, 0, sizeof(iTidy_IconImageData));

    pixel_count = (ULONG)w * (ULONG)h;

    out->pixel_data_normal = (UBYTE *)whd_malloc(pixel_count);
    if (out->pixel_data_normal == NULL)
    {
        log_error(LOG_ICONS, "itidy_icon_image_create_blank: alloc failed (%lu bytes)\n", pixel_count);
        return FALSE;
    }

    memset(out->pixel_data_normal, bg_index, pixel_count);

    out->width  = w;
    out->height = h;
    out->source_format = ITIDY_ICON_FORMAT_OS35;
    out->is_palette_mapped = TRUE;
    out->has_selected_image = FALSE;

    log_debug(LOG_ICONS, "itidy_icon_image_create_blank: %ux%u filled with index %u\n",
              (unsigned)w, (unsigned)h, (unsigned)bg_index);

    return TRUE;
}

/*========================================================================*/
/* ToolType Helpers                                                       */
/*========================================================================*/

/**
 * @brief Parse an integer value from a ToolType key.
 *
 * Returns default_val if the key is not found or cannot be parsed.
 */
static LONG parse_integer_tooltype(STRPTR *tool_types, const char *key, LONG default_val)
{
    STRPTR value;

    if (tool_types == NULL || key == NULL)
    {
        return default_val;
    }

    value = (STRPTR)FindToolType(tool_types, (STRPTR)key);
    if (value != NULL)
    {
        LONG parsed = atoi((const char *)value);
        return parsed;
    }

    return default_val;
}

/**
 * @brief Parse a "x,y,w,h" area ToolType.
 *
 * Returns TRUE if found and parsed, FALSE otherwise.
 */
static BOOL parse_area_tooltype(STRPTR *tool_types, const char *key,
                                UWORD *out_x, UWORD *out_y,
                                UWORD *out_w, UWORD *out_h)
{
    STRPTR value;
    int x, y, w, h;

    if (tool_types == NULL || key == NULL)
    {
        return FALSE;
    }

    value = (STRPTR)FindToolType(tool_types, (STRPTR)key);
    if (value == NULL)
    {
        return FALSE;
    }

    if (sscanf((const char *)value, "%d,%d,%d,%d", &x, &y, &w, &h) == 4)
    {
        if (x >= 0 && y >= 0 && w > 0 && h > 0)
        {
            *out_x = (UWORD)x;
            *out_y = (UWORD)y;
            *out_w = (UWORD)w;
            *out_h = (UWORD)h;
            return TRUE;
        }
    }

    log_warning(LOG_ICONS, "itidy_parse_area_tooltype: malformed '%s' value: '%s'\n",
                key, (const char *)value);
    return FALSE;
}

/*------------------------------------------------------------------------*/

void itidy_parse_safe_area(struct DiskObject *icon,
                           UWORD icon_width, UWORD icon_height,
                           UWORD *safe_left, UWORD *safe_top,
                           UWORD *safe_width, UWORD *safe_height)
{
    STRPTR *tool_types;

    // Defaults: 4-pixel margin on each side
    *safe_left   = ITIDY_DEFAULT_MARGIN;
    *safe_top    = ITIDY_DEFAULT_MARGIN;
    *safe_width  = (icon_width > ITIDY_DEFAULT_MARGIN * 2) ? (icon_width - ITIDY_DEFAULT_MARGIN * 2) : 1;
    *safe_height = (icon_height > ITIDY_DEFAULT_MARGIN * 2) ? (icon_height - ITIDY_DEFAULT_MARGIN * 2) : 1;

    if (icon == NULL)
    {
        return;
    }

    tool_types = (STRPTR *)icon->do_ToolTypes;
    if (tool_types == NULL)
    {
        return;
    }

    // Try to parse ITIDY_TEXT_AREA=x,y,w,h
    {
        UWORD parsed_x, parsed_y, parsed_w, parsed_h;
        if (parse_area_tooltype(tool_types, ITIDY_TT_TEXT_AREA,
                                &parsed_x, &parsed_y, &parsed_w, &parsed_h))
        {
            // Validate against icon dimensions
            if (parsed_x + parsed_w <= icon_width &&
                parsed_y + parsed_h <= icon_height)
            {
                *safe_left   = parsed_x;
                *safe_top    = parsed_y;
                *safe_width  = parsed_w;
                *safe_height = parsed_h;

                log_debug(LOG_ICONS, "itidy_parse_safe_area: from ToolType: %u,%u,%u,%u\n",
                          (unsigned)parsed_x, (unsigned)parsed_y,
                          (unsigned)parsed_w, (unsigned)parsed_h);
            }
            else
            {
                log_warning(LOG_ICONS, "itidy_parse_safe_area: ToolType area exceeds icon bounds "
                            "(%u+%u > %u or %u+%u > %u), using defaults\n",
                            (unsigned)parsed_x, (unsigned)parsed_w, (unsigned)icon_width,
                            (unsigned)parsed_y, (unsigned)parsed_h, (unsigned)icon_height);
            }
        }
    }
}

/*------------------------------------------------------------------------*/

/**
 * @brief Calculate luminance using integer-only math.
 *
 * Approximation: lum = (299*R + 587*G + 114*B) / 1000
 * Returns a value 0..255.
 */
static ULONG calculate_luminance(UBYTE r, UBYTE g, UBYTE b)
{
    return (299UL * (ULONG)r + 587UL * (ULONG)g + 114UL * (ULONG)b) / 1000UL;
}

/*------------------------------------------------------------------------*/

void itidy_resolve_palette_indices(struct DiskObject *icon,
                                   const struct ColorRegister *palette,
                                   ULONG palette_size,
                                   UBYTE *bg_index,
                                   UBYTE *text_index)
{
    STRPTR *tool_types = NULL;
    LONG tt_bg = -1;
    LONG tt_text = -1;

    // Default fallbacks
    *bg_index   = 0;
    *text_index = 1;

    if (palette == NULL || palette_size == 0)
    {
        return;
    }

    // Check ToolTypes for explicit overrides
    if (icon != NULL)
    {
        tool_types = (STRPTR *)icon->do_ToolTypes;
    }

    if (tool_types != NULL)
    {
        tt_bg   = parse_integer_tooltype(tool_types, ITIDY_TT_BG_COLOR, -1);
        tt_text = parse_integer_tooltype(tool_types, ITIDY_TT_TEXT_COLOR, -1);
    }

    // Special case: -1 means "no background" (preserve template pixels)
    if (tt_bg == -1)
    {
        *bg_index = ITIDY_NO_BG_COLOR;
        log_debug(LOG_ICONS, "itidy_resolve_palette_indices: bg = NO_BG (255) from ToolType\n");
    }
    else if (tt_bg >= 0 && (ULONG)tt_bg < palette_size)
    {
        *bg_index = (UBYTE)tt_bg;
        log_debug(LOG_ICONS, "itidy_resolve_palette_indices: bg from ToolType = %u\n", (unsigned)*bg_index);
    }
    else
    {
        tt_bg = -2;  // Force auto-detect (use -2 to distinguish from explicit -1)
    }

    if (tt_text >= 0 && (ULONG)tt_text < palette_size)
    {
        *text_index = (UBYTE)tt_text;
        log_debug(LOG_ICONS, "itidy_resolve_palette_indices: text from ToolType = %u\n", (unsigned)*text_index);
    }
    else
    {
        tt_text = -1;  // Force auto-detect
    }

    // Auto-detect: scan palette for lightest (bg) and darkest (text)
    // Only auto-detect if not explicitly set (tt_bg == -2, not -1 which means "no bg")
    if (tt_bg == -2 || tt_text < 0)
    {
        ULONG i;
        ULONG lightest_lum = 0;
        ULONG darkest_lum  = 255;
        UBYTE lightest_idx  = 0;
        UBYTE darkest_idx   = 0;

        for (i = 0; i < palette_size; i++)
        {
            ULONG lum = calculate_luminance(palette[i].red, palette[i].green, palette[i].blue);

            if (lum >= lightest_lum)
            {
                lightest_lum = lum;
                lightest_idx = (UBYTE)i;
            }

            if (lum <= darkest_lum)
            {
                darkest_lum = lum;
                darkest_idx = (UBYTE)i;
            }
        }

        if (tt_bg == -2)
        {
            *bg_index = lightest_idx;
            log_debug(LOG_ICONS, "itidy_resolve_palette_indices: auto bg = %u (lum=%lu)\n",
                      (unsigned)lightest_idx, lightest_lum);
        }

        if (tt_text < 0)
        {
            *text_index = darkest_idx;
            log_debug(LOG_ICONS, "itidy_resolve_palette_indices: auto text = %u (lum=%lu)\n",
                      (unsigned)darkest_idx, darkest_lum);
        }

        // Safety: if bg and text ended up the same, try to pick alternates
        if (*bg_index == *text_index && palette_size > 1)
        {
            log_warning(LOG_ICONS, "itidy_resolve_palette_indices: bg == text (%u), adjusting\n",
                        (unsigned)*bg_index);
            // Pick the other extreme
            if (*bg_index == lightest_idx)
            {
                *text_index = darkest_idx;
            }
            else
            {
                *bg_index = lightest_idx;
            }

            // If still the same (degenerate palette), just pick index 0 and 1
            if (*bg_index == *text_index)
            {
                *bg_index   = 0;
                *text_index = (palette_size > 1) ? 1 : 0;
            }
        }
    }
}

/*------------------------------------------------------------------------*/

BOOL itidy_get_render_params(struct DiskObject *icon,
                             const iTidy_IconImageData *img,
                             iTidy_TextRenderParams *params)
{
    STRPTR *tool_types = NULL;

    if (img == NULL || params == NULL)
    {
        return FALSE;
    }

    memset(params, 0, sizeof(iTidy_TextRenderParams));

    // Parse safe area
    itidy_parse_safe_area(icon, img->width, img->height,
                          &params->base.safe_left,
                          &params->base.safe_top,
                          &params->base.safe_width,
                          &params->base.safe_height);

    // Parse exclusion area (for preserving template artwork like folded corners)
    if (icon != NULL && icon->do_ToolTypes != NULL)
    {
        UWORD parsed_x, parsed_y, parsed_w, parsed_h;
        if (parse_area_tooltype((STRPTR *)icon->do_ToolTypes, ITIDY_TT_EXCLUDE_AREA,
                                &parsed_x, &parsed_y, &parsed_w, &parsed_h))
        {
            // Validate against icon dimensions
            if (parsed_x + parsed_w <= img->width && parsed_y + parsed_h <= img->height)
            {
                params->base.exclude_left   = parsed_x;
                params->base.exclude_top    = parsed_y;
                params->base.exclude_width  = parsed_w;
                params->base.exclude_height = parsed_h;

                log_debug(LOG_ICONS, "itidy_get_render_params: exclude_area=%u,%u,%u,%u\n",
                          (unsigned)parsed_x, (unsigned)parsed_y,
                          (unsigned)parsed_w, (unsigned)parsed_h);
            }
            else
            {
                log_warning(LOG_ICONS, "itidy_get_render_params: EXCLUDE_AREA exceeds icon bounds, ignoring\n");
            }
        }
    }

    // Set buffer dimensions
    params->base.buffer_width  = img->width;
    params->base.buffer_height = img->height;
    params->base.pixel_buffer  = img->pixel_data_normal;

    // Resolve palette indices
    itidy_resolve_palette_indices(icon,
                                  img->palette_normal,
                                  img->palette_size_normal,
                                  &params->base.bg_color_index,
                                  &params->base.text_color_index);

    // Parse text-specific ToolTypes
    if (icon != NULL)
    {
        tool_types = (STRPTR *)icon->do_ToolTypes;
    }

    params->char_pixel_width  = (UWORD)parse_integer_tooltype(tool_types, ITIDY_TT_CHAR_WIDTH,
                                                               ITIDY_DEFAULT_CHAR_WIDTH);
    params->line_pixel_height = (UWORD)parse_integer_tooltype(tool_types, ITIDY_TT_LINE_HEIGHT,
                                                               ITIDY_DEFAULT_LINE_HEIGHT);
    params->line_gap          = (UWORD)parse_integer_tooltype(tool_types, ITIDY_TT_LINE_GAP,
                                                               ITIDY_DEFAULT_LINE_GAP);
    params->max_read_bytes    = (UWORD)parse_integer_tooltype(tool_types, ITIDY_TT_READ_BYTES,
                                                               ITIDY_DEFAULT_READ_BYTES);

    // Auto-select char width if set to 0
    if (params->char_pixel_width == 0)
    {
        if (params->base.safe_width >= ITIDY_CHAR_WIDTH_PROMOTE_THRESHOLD)
        {
            params->char_pixel_width = 2;
        }
        else
        {
            params->char_pixel_width = 1;
        }

        log_debug(LOG_ICONS, "itidy_get_render_params: auto char_width = %u (safe_width=%u)\n",
                  (unsigned)params->char_pixel_width, (unsigned)params->base.safe_width);
    }

    log_info(LOG_ICONS, "itidy_get_render_params: safe=%u,%u,%u,%u bg=%u text=%u "
             "char_w=%u line_h=%u gap=%u read=%u\n",
             (unsigned)params->base.safe_left, (unsigned)params->base.safe_top,
             (unsigned)params->base.safe_width, (unsigned)params->base.safe_height,
             (unsigned)params->base.bg_color_index, (unsigned)params->base.text_color_index,
             (unsigned)params->char_pixel_width, (unsigned)params->line_pixel_height,
             (unsigned)params->line_gap, (unsigned)params->max_read_bytes);

    return TRUE;
}

/*========================================================================*/
/* itidy_stamp_created_tooltypes                                          */
/*========================================================================*/

BOOL itidy_stamp_created_tooltypes(struct DiskObject *icon,
                                   const char *kind,
                                   const char *source_name,
                                   ULONG source_size,
                                   const struct DateStamp *source_date)
{
    STRPTR *old_tooltypes;
    STRPTR *new_tooltypes;
    int old_count = 0;
    int new_count;
    int i;
    int dest;
    char stamp_created[32];
    char stamp_kind[80];
    char stamp_src[256];
    char stamp_size[64];
    char stamp_date[64];

    if (icon == NULL || kind == NULL)
    {
        return FALSE;
    }

    // Build the new ToolType strings
    snprintf(stamp_created, sizeof(stamp_created), "%s=%s", ITIDY_TT_CREATED, "1");
    snprintf(stamp_kind, sizeof(stamp_kind), "%s=%s", ITIDY_TT_KIND, kind);

    if (source_name != NULL)
    {
        snprintf(stamp_src, sizeof(stamp_src), "%s=%s", ITIDY_TT_SRC, source_name);
    }
    else
    {
        stamp_src[0] = '\0';
    }

    snprintf(stamp_size, sizeof(stamp_size), "%s=%lu", ITIDY_TT_SRC_SIZE, source_size);

    if (source_date != NULL)
    {
        snprintf(stamp_date, sizeof(stamp_date), "%s=%ld,%ld,%ld",
                 ITIDY_TT_SRC_DATE,
                 source_date->ds_Days,
                 source_date->ds_Minute,
                 source_date->ds_Tick);
    }
    else
    {
        stamp_date[0] = '\0';
    }

    // Count existing ToolTypes (excluding any existing ITIDY_ entries)
    old_tooltypes = (STRPTR *)icon->do_ToolTypes;
    old_count = 0;
    if (old_tooltypes != NULL)
    {
        while (old_tooltypes[old_count] != NULL)
        {
            old_count++;
        }
    }

    // Allocate new array: old entries (filtered) + up to 5 new ITIDY_ entries + NULL
    new_count = old_count + 5 + 1;  // worst case
    new_tooltypes = (STRPTR *)whd_malloc(new_count * sizeof(STRPTR));
    if (new_tooltypes == NULL)
    {
        log_error(LOG_ICONS, "itidy_stamp_created_tooltypes: alloc failed\n");
        return FALSE;
    }

    // Copy existing non-ITIDY entries
    dest = 0;
    if (old_tooltypes != NULL)
    {
        for (i = 0; i < old_count; i++)
        {
            // Skip existing ITIDY_ entries (we'll re-add them)
            if (strncmp((const char *)old_tooltypes[i], "ITIDY_", 6) == 0)
            {
                continue;
            }
            new_tooltypes[dest] = whd_strdup((const char *)old_tooltypes[i]);
            if (new_tooltypes[dest] != NULL)
            {
                dest++;
            }
        }
    }

    // Add our stamp entries
    new_tooltypes[dest] = whd_strdup(stamp_created);
    if (new_tooltypes[dest] != NULL) dest++;

    new_tooltypes[dest] = whd_strdup(stamp_kind);
    if (new_tooltypes[dest] != NULL) dest++;

    if (stamp_src[0] != '\0')
    {
        new_tooltypes[dest] = whd_strdup(stamp_src);
        if (new_tooltypes[dest] != NULL) dest++;
    }

    new_tooltypes[dest] = whd_strdup(stamp_size);
    if (new_tooltypes[dest] != NULL) dest++;

    if (stamp_date[0] != '\0')
    {
        new_tooltypes[dest] = whd_strdup(stamp_date);
        if (new_tooltypes[dest] != NULL) dest++;
    }

    new_tooltypes[dest] = NULL;  // NULL-terminate the array

    // Replace the icon's ToolTypes pointer
    // NOTE: The original ToolTypes are owned by the DiskObject and will be
    // freed when the DiskObject is disposed. We're setting a new array that
    // we've allocated. The caller must ensure this is handled correctly
    // (typically the DiskObject is a clone that will be disposed after save).
    icon->do_ToolTypes = new_tooltypes;

    log_info(LOG_ICONS, "itidy_stamp_created_tooltypes: stamped %d entries (kind=%s)\n",
             dest, kind);

    return TRUE;
}

/*========================================================================*/
/* itidy_check_cache_valid                                                */
/*========================================================================*/

BOOL itidy_check_cache_valid(const char *target_path,
                             const char *expected_kind,
                             ULONG source_size,
                             const struct DateStamp *source_date)
{
    struct DiskObject *icon = NULL;
    STRPTR *tool_types;
    STRPTR value;
    BOOL is_valid = FALSE;

    if (target_path == NULL || expected_kind == NULL || source_date == NULL)
    {
        return FALSE;
    }

    // Try to load the existing target icon
    icon = GetDiskObject((STRPTR)target_path);
    if (icon == NULL)
    {
        return FALSE;  // No existing icon — not cached
    }

    tool_types = (STRPTR *)icon->do_ToolTypes;
    if (tool_types == NULL)
    {
        FreeDiskObject(icon);
        return FALSE;
    }

    // Check ITIDY_CREATED=1
    value = (STRPTR)FindToolType(tool_types, (STRPTR)ITIDY_TT_CREATED);
    if (value == NULL || strcmp((const char *)value, "1") != 0)
    {
        FreeDiskObject(icon);
        return FALSE;
    }

    // Check ITIDY_KIND matches expected
    value = (STRPTR)FindToolType(tool_types, (STRPTR)ITIDY_TT_KIND);
    if (value == NULL || strcmp((const char *)value, expected_kind) != 0)
    {
        FreeDiskObject(icon);
        return FALSE;
    }

    // Check ITIDY_SRC_SIZE matches current source size
    value = (STRPTR)FindToolType(tool_types, (STRPTR)ITIDY_TT_SRC_SIZE);
    if (value != NULL)
    {
        ULONG cached_size = (ULONG)atol((const char *)value);
        if (cached_size != source_size)
        {
            log_debug(LOG_ICONS, "itidy_check_cache_valid: size mismatch (cached=%lu, current=%lu)\n",
                      cached_size, source_size);
            FreeDiskObject(icon);
            return FALSE;
        }
    }
    else
    {
        FreeDiskObject(icon);
        return FALSE;
    }

    // Check ITIDY_SRC_DATE matches current source datestamp
    value = (STRPTR)FindToolType(tool_types, (STRPTR)ITIDY_TT_SRC_DATE);
    if (value != NULL)
    {
        LONG days = 0, minutes = 0, ticks = 0;
        if (sscanf((const char *)value, "%ld,%ld,%ld", &days, &minutes, &ticks) == 3)
        {
            if (days == source_date->ds_Days &&
                minutes == source_date->ds_Minute &&
                ticks == source_date->ds_Tick)
            {
                is_valid = TRUE;
                log_debug(LOG_ICONS, "itidy_check_cache_valid: cache hit for '%s'\n", target_path);
            }
            else
            {
                log_debug(LOG_ICONS, "itidy_check_cache_valid: date mismatch for '%s'\n", target_path);
            }
        }
    }

    FreeDiskObject(icon);
    return is_valid;
}

/*========================================================================*/
/* Debug: Pixel Buffer Dump                                               */
/*========================================================================*/

#ifdef DEBUG_ICON_DUMP

/**
 * @brief Echo the first few lines of a source file to the icon log.
 */
static void dump_source_text(const char *source_path, int max_lines)
{
    BPTR fh;
    char line_buf[128];
    int lines_read = 0;
    int buf_pos = 0;
    LONG bytes_read;
    char read_buf[512];
    int read_pos = 0;
    LONG total_read = 0;

    if (source_path == NULL)
    {
        return;
    }

    fh = Open((STRPTR)source_path, MODE_OLDFILE);
    if (fh == 0)
    {
        log_info(LOG_ICONS, "[ICON_DUMP] Could not open source: %s\n", source_path);
        return;
    }

    log_info(LOG_ICONS, "[ICON_DUMP] --- Source text (first %d lines) ---\n", max_lines);

    bytes_read = Read(fh, read_buf, sizeof(read_buf) - 1);
    if (bytes_read > 0)
    {
        read_buf[bytes_read] = '\0';
        total_read = bytes_read;
    }

    while (lines_read < max_lines && read_pos < total_read)
    {
        char ch = read_buf[read_pos++];

        if (ch == '\n' || ch == '\0')
        {
            line_buf[buf_pos] = '\0';
            log_info(LOG_ICONS, "[ICON_DUMP] %s\n", line_buf);
            buf_pos = 0;
            lines_read++;
        }
        else if (ch == '\r')
        {
            // Skip CR (handle CR+LF)
            continue;
        }
        else
        {
            if (buf_pos < (int)(sizeof(line_buf) - 1))
            {
                line_buf[buf_pos++] = ch;
            }
        }
    }

    // Flush any remaining partial line
    if (buf_pos > 0 && lines_read < max_lines)
    {
        line_buf[buf_pos] = '\0';
        log_info(LOG_ICONS, "[ICON_DUMP] %s\n", line_buf);
    }

    Close(fh);
}

void itidy_icon_image_dump(const iTidy_IconImageData *img,
                           const iTidy_RenderParams *params,
                           const char *source_path)
{
    UWORD row, col;
    BOOL use_two_hex;

    if (img == NULL || img->pixel_data_normal == NULL)
    {
        log_info(LOG_ICONS, "[ICON_DUMP] No image data to dump\n");
        return;
    }

    log_info(LOG_ICONS, "[ICON_DUMP] %ux%u pixels, palette: %lu entries\n",
              (unsigned)img->width, (unsigned)img->height,
              img->palette_size_normal);

    // Dump palette
    if (img->palette_normal != NULL)
    {
        char pal_buf[1024];
        int pos = 0;
        ULONG i;

        for (i = 0; i < img->palette_size_normal && pos < (int)(sizeof(pal_buf) - 16); i++)
        {
            pos += snprintf(pal_buf + pos, sizeof(pal_buf) - pos,
                            "%lu=%02X%02X%02X ",
                            i,
                            (unsigned)img->palette_normal[i].red,
                            (unsigned)img->palette_normal[i].green,
                            (unsigned)img->palette_normal[i].blue);
        }
        log_info(LOG_ICONS, "[ICON_DUMP] Palette: %s\n", pal_buf);
    }

    // Log safe area
    if (params != NULL)
    {
        log_info(LOG_ICONS, "[ICON_DUMP] Safe area: left=%u top=%u width=%u height=%u\n",
                  (unsigned)params->safe_left, (unsigned)params->safe_top,
                  (unsigned)params->safe_width, (unsigned)params->safe_height);
    }

    // Determine hex format
    use_two_hex = (img->palette_size_normal > 16) ? TRUE : FALSE;

    // Dump normal image pixel grid
    log_info(LOG_ICONS, "[ICON_DUMP] --- Normal image ---\n");

    for (row = 0; row < img->height; row++)
    {
        char row_buf[600];  // Enough for 256 pixels * 2 chars + spaces
        int rpos = 0;

        for (col = 0; col < img->width && rpos < (int)(sizeof(row_buf) - 8); col++)
        {
            UBYTE pixel = img->pixel_data_normal[row * img->width + col];

            if (use_two_hex)
            {
                rpos += snprintf(row_buf + rpos, sizeof(row_buf) - rpos,
                                 "%02X ", (unsigned)pixel);
            }
            else
            {
                row_buf[rpos++] = (pixel < 10) ? ('0' + pixel) : ('A' + pixel - 10);
            }
        }

        row_buf[rpos] = '\0';
        log_info(LOG_ICONS, "[ICON_DUMP] %s\n", row_buf);
    }

    // Dump source text excerpt
    dump_source_text(source_path, 5);
}

#endif /* DEBUG_ICON_DUMP */
