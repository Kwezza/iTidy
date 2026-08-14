/*
 * icon_probe.c - Read-only identification of representations inside a .info
 *
 * Looks at classic image presence, NewIcons ToolTypes, and appended
 * extension payloads (FORM ICON, PNG, ARGB). Does not decode pixels.
 */

#include "icon_probe.h"

#include <string.h>

static const char k_newicons_marker[] =
    "*** DON'T EDIT THE FOLLOWING LINES!! ***";

static BOOL is_png_signature(const UBYTE *data, ULONG size, ULONG offset)
{
    static const UBYTE png[8] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
    };
    if (!icon_file_range_ok(size, offset, 8))
        return FALSE;
    return memcmp(data + offset, png, 8) == 0;
}

static BOOL fourcc_eq(const UBYTE *p, const char *id)
{
    return p[0] == (UBYTE)id[0] && p[1] == (UBYTE)id[1] &&
           p[2] == (UBYTE)id[2] && p[3] == (UBYTE)id[3];
}

static BOOL tooltype_starts_with(const UBYTE *text, ULONG len,
                                 const char *prefix)
{
    ULONG i;
    for (i = 0; prefix[i] != '\0'; i++)
    {
        if (i >= len)
            return FALSE;
        if (text[i] != (UBYTE)prefix[i])
            return FALSE;
    }
    return TRUE;
}

static BOOL tooltype_equals(const UBYTE *text, ULONG len, const char *want)
{
    ULONG i;
    for (i = 0; want[i] != '\0'; i++)
    {
        if (i >= len)
            return FALSE;
        if (text[i] != (UBYTE)want[i])
            return FALSE;
    }
    return i == len;
}

static void scan_tooltypes(const iTidy_IconFile *file, iTidy_IconProbe *out)
{
    ULONG i;
    BOOL saw_im1 = FALSE;
    BOOL saw_im2 = FALSE;

    for (i = 0; i < file->tooltypes_count; i++)
    {
        const UBYTE *text;
        ULONG len;

        if (!icon_file_tooltype(file, i, &text, &len))
            continue;

        if (tooltype_equals(text, len, k_newicons_marker) ||
            tooltype_starts_with(text, len, "IM1=") ||
            tooltype_starts_with(text, len, "IM2="))
        {
            out->has_newicons = TRUE;
        }
        if (tooltype_starts_with(text, len, "IM1="))
            saw_im1 = TRUE;
        if (tooltype_starts_with(text, len, "IM2="))
            saw_im2 = TRUE;
    }

    if (saw_im1)
        out->has_normal = TRUE;
    if (saw_im2)
        out->has_selected = TRUE;
}

static void walk_iff_icon(const UBYTE *data, ULONG size,
                          ULONG form_offset, ULONG form_size,
                          iTidy_IconProbe *out)
{
    ULONG pos;
    ULONG end;
    ULONG imag_count = 0;
    UWORD max_pal = 0;

    /* form_size excludes the FORM+size header (8 bytes) and includes "ICON". */
    if (!icon_file_range_ok(size, form_offset + 8UL, form_size))
        return;

    end = form_offset + 8UL + form_size;
    pos = form_offset + 12UL; /* skip FORM, size, ICON */

    while (icon_file_range_ok(size, pos, 8) && (pos + 8UL) <= end)
    {
        ULONG chunk_size;
        ULONG payload;
        ULONG padded;

        if (!icon_file_read_u32(data, size, pos + 4UL, &chunk_size))
            break;
        payload = pos + 8UL;
        if (!icon_file_range_ok(size, payload, chunk_size) ||
            (payload + chunk_size) > end)
        {
            break;
        }

        if (fourcc_eq(data + pos, "FACE") && chunk_size >= 6UL)
        {
            if (!icon_file_read_u16(data, size, payload + 4UL, &max_pal))
                max_pal = 0;
        }
        else if (fourcc_eq(data + pos, "IMAG"))
        {
            imag_count++;
        }
        else if (fourcc_eq(data + pos, "ARGB"))
        {
            out->has_os4_argb = TRUE;
            out->has_unsupported = TRUE;
        }

        padded = chunk_size + (chunk_size & 1UL);
        if (payload > end || padded > (end - payload))
            break;
        pos = payload + padded;
    }

    /* OS3.5 ColorIcon requires at least one IMAG; FACE+ARGB alone is not. */
    if (imag_count > 0UL)
        out->has_coloricon = TRUE;

    /*
     * GlowIcon is the same FORM ICON/IMAG encoding. Historical label when
     * FACE MaxPaletteBytes (max RGB palette byte count - 1) >= 255.
     */
    if (out->has_coloricon && max_pal >= 255U)
        out->has_glowicon = TRUE;

    if (imag_count >= 1UL)
        out->has_normal = TRUE;
    if (imag_count >= 2UL)
        out->has_selected = TRUE;
}

static void scan_extension(const iTidy_IconFile *file, iTidy_IconProbe *out)
{
    const UBYTE *data = file->data;
    ULONG size = file->size;
    ULONG offset = file->extension_offset;
    ULONG remaining = file->extension_size;
    ULONG form_size;
    ULONG type_off;

    if (remaining == 0 || data == NULL)
        return;

    if (is_png_signature(data, size, offset))
    {
        out->has_png = TRUE;
        out->has_unsupported = TRUE;
        return;
    }

    if (remaining < 12UL)
        return;

    if (!fourcc_eq(data + offset, "FORM"))
    {
        /* Cheap scan for a PNG or FORM that is not at the first byte. */
        ULONG i;
        ULONG limit = remaining;
        if (limit > 64UL)
            limit = 64UL;
        for (i = 1; i + 8UL <= limit; i++)
        {
            if (is_png_signature(data, size, offset + i))
            {
                out->has_png = TRUE;
                out->has_unsupported = TRUE;
                return;
            }
        }
        return;
    }

    if (!icon_file_read_u32(data, size, offset + 4UL, &form_size))
        return;
    if (form_size < 4UL)
        return;

    type_off = offset + 8UL;
    if (!icon_file_range_ok(size, type_off, 4))
        return;

    if (fourcc_eq(data + type_off, "ICON"))
    {
        /* has_coloricon set only if walk finds IMAG (not ARGB-only). */
        walk_iff_icon(data, size, offset, form_size, out);
        return;
    }

    if (fourcc_eq(data + type_off, "ARGB"))
    {
        out->has_os4_argb = TRUE;
        out->has_unsupported = TRUE;
        return;
    }

    out->has_unsupported = TRUE;
}

iTidy_IconError icon_probe_file(const iTidy_IconFile *file,
                                iTidy_IconProbe *out)
{
    if (out == NULL)
        return ITIDY_ICON_ERR_NULL;
    memset(out, 0, sizeof(*out));
    if (file == NULL || file->data == NULL)
        return ITIDY_ICON_ERR_NULL;

    if (file->normal.present)
    {
        out->has_classic = TRUE;
        out->has_normal = TRUE;
    }
    if (file->selected.present)
        out->has_selected = TRUE;

    scan_tooltypes(file, out);
    scan_extension(file, out);
    return ITIDY_ICON_OK;
}

iTidy_IconError icon_probe_buffer(const UBYTE *data, ULONG size,
                                  iTidy_IconProbe *out)
{
    iTidy_IconFile file;
    iTidy_IconError err;

    if (out == NULL)
        return ITIDY_ICON_ERR_NULL;
    memset(out, 0, sizeof(*out));

    if (data != NULL && is_png_signature(data, size, 0))
    {
        out->has_png = TRUE;
        out->has_unsupported = TRUE;
        return ITIDY_ICON_OK;
    }

    err = icon_file_parse(data, size, &file);
    if (err != ITIDY_ICON_OK)
        return err;
    return icon_probe_file(&file, out);
}
