/*
 * icon_file.c - Bounds-checked raw Amiga .info envelope parser
 *
 * Serialized DiskObject layout (big-endian), from IconFormats.txt:
 *   DiskObject (78 bytes)
 *   DrawerData (56)           if do_DrawerData != 0
 *   Image + planar data       if ga_GadgetRender != 0
 *   Image + planar data       if ga_SelectRender != 0
 *   DefaultTool text          if do_DefaultTool != 0
 *   ToolTypes table           if do_ToolTypes != 0
 *   ToolWindow text           if do_ToolWindow != 0
 *   DrawerData2 (6)           if DrawerData present and ga_UserData lo-byte == 1
 *   extension bytes           ColorIcon FORM ICON, PNG, etc.
 *
 * On-disk APTR fields are leftover memory addresses. Non-zero means present.
 */

#include "icon_file.h"

#include <string.h>

/*========================================================================*/
/* On-disk field offsets                                                  */
/*========================================================================*/

#define OFF_MAGIC           0x00UL
#define OFF_VERSION         0x02UL
#define OFF_GADGET_WIDTH    0x0CUL
#define OFF_GADGET_HEIGHT   0x0EUL
#define OFF_GADGET_FLAGS    0x10UL
#define OFF_GADGET_RENDER   0x16UL
#define OFF_SELECT_RENDER   0x1AUL
#define OFF_USERDATA        0x2CUL
#define OFF_TYPE            0x30UL
#define OFF_DEFAULT_TOOL    0x32UL
#define OFF_TOOLTYPES       0x36UL
#define OFF_CURRENT_X       0x3AUL
#define OFF_CURRENT_Y       0x3EUL
#define OFF_DRAWER_DATA     0x42UL
#define OFF_TOOL_WINDOW     0x46UL
#define OFF_STACK_SIZE      0x4AUL

#define IMG_OFF_WIDTH       4
#define IMG_OFF_HEIGHT      6
#define IMG_OFF_DEPTH       8
#define IMG_OFF_PLANE_PICK  14
#define IMG_OFF_PLANE_ONOFF 15

/*========================================================================*/
/* Error strings                                                          */
/*========================================================================*/

const char *icon_error_string(iTidy_IconError err)
{
    switch (err)
    {
        case ITIDY_ICON_OK:                return "ok";
        case ITIDY_ICON_ERR_NULL:          return "null argument";
        case ITIDY_ICON_ERR_TOO_SMALL:     return "buffer too small";
        case ITIDY_ICON_ERR_BAD_MAGIC:     return "not a DiskObject (bad magic)";
        case ITIDY_ICON_ERR_BAD_VERSION:   return "unsupported DiskObject version";
        case ITIDY_ICON_ERR_TRUNCATED:     return "truncated icon data";
        case ITIDY_ICON_ERR_OVERFLOW:      return "size overflow";
        case ITIDY_ICON_ERR_BAD_DIMENSION: return "impossible image dimension";
        case ITIDY_ICON_ERR_BAD_COUNT:     return "impossible count";
        case ITIDY_ICON_ERR_BAD_TEXT:      return "malformed text blob";
        case ITIDY_ICON_ERR_UNSUPPORTED:   return "unsupported icon extension";
        case ITIDY_ICON_ERR_NO_DATA:       return "no ColorIcon image data";
        case ITIDY_ICON_ERR_ALLOC:         return "out of memory";
        default:                           return "unknown error";
    }
}

/*========================================================================*/
/* Safe readers                                                           */
/*========================================================================*/

BOOL icon_file_range_ok(ULONG size, ULONG offset, ULONG length)
{
    if (length == 0)
        return offset <= size;
    if (offset >= size)
        return FALSE;
    return length <= (size - offset);
}

BOOL icon_file_read_u8(const UBYTE *data, ULONG size, ULONG offset, UBYTE *out)
{
    if (data == NULL || out == NULL)
        return FALSE;
    if (!icon_file_range_ok(size, offset, 1))
        return FALSE;
    *out = data[offset];
    return TRUE;
}

BOOL icon_file_read_u16(const UBYTE *data, ULONG size, ULONG offset, UWORD *out)
{
    if (data == NULL || out == NULL)
        return FALSE;
    if (!icon_file_range_ok(size, offset, 2))
        return FALSE;
    *out = ((UWORD)data[offset] << 8) | (UWORD)data[offset + 1];
    return TRUE;
}

BOOL icon_file_read_s16(const UBYTE *data, ULONG size, ULONG offset, WORD *out)
{
    UWORD u;
    if (!icon_file_read_u16(data, size, offset, &u))
        return FALSE;
    *out = (WORD)u;
    return TRUE;
}

BOOL icon_file_read_u32(const UBYTE *data, ULONG size, ULONG offset, ULONG *out)
{
    if (data == NULL || out == NULL)
        return FALSE;
    if (!icon_file_range_ok(size, offset, 4))
        return FALSE;
    *out = ((ULONG)data[offset] << 24) |
           ((ULONG)data[offset + 1] << 16) |
           ((ULONG)data[offset + 2] << 8) |
           (ULONG)data[offset + 3];
    return TRUE;
}

BOOL icon_file_read_s32(const UBYTE *data, ULONG size, ULONG offset, LONG *out)
{
    ULONG u;
    if (!icon_file_read_u32(data, size, offset, &u))
        return FALSE;
    *out = (LONG)u;
    return TRUE;
}

BOOL icon_file_planar_data_size(UWORD width, UWORD height, UWORD depth,
                                ULONG *out_size)
{
    ULONG bpr;
    ULONG plane;

    if (out_size == NULL)
        return FALSE;

    if (width == 0 || height == 0 || depth == 0 || depth > ITIDY_ICON_MAX_DEPTH)
        return FALSE;
    if (width > ITIDY_ICON_MAX_WIDTH || height > ITIDY_ICON_MAX_HEIGHT)
        return FALSE;

    bpr = (((ULONG)width + 15UL) / 16UL) * 2UL;
    if (bpr == 0 || (ULONG)height > (~0UL / bpr))
        return FALSE;
    plane = bpr * (ULONG)height;
    if ((ULONG)depth > (~0UL / plane))
        return FALSE;

    *out_size = plane * (ULONG)depth;
    return TRUE;
}

/*========================================================================*/
/* Internal parse helpers                                                 */
/*========================================================================*/

static BOOL pointer_present(ULONG value)
{
    return value != 0;
}

static iTidy_IconError parse_image(const UBYTE *data, ULONG size,
                                   ULONG offset, iTidy_IconImageLoc *loc,
                                   ULONG *next_offset)
{
    WORD width;
    WORD height;
    WORD depth;
    ULONG data_size;
    UBYTE pick;
    UBYTE onoff;

    if (!icon_file_range_ok(size, offset, ITIDY_ICON_IMAGE_HEADER_SIZE))
        return ITIDY_ICON_ERR_TRUNCATED;

    if (!icon_file_read_s16(data, size, offset + IMG_OFF_WIDTH, &width) ||
        !icon_file_read_s16(data, size, offset + IMG_OFF_HEIGHT, &height) ||
        !icon_file_read_s16(data, size, offset + IMG_OFF_DEPTH, &depth) ||
        !icon_file_read_u8(data, size, offset + IMG_OFF_PLANE_PICK, &pick) ||
        !icon_file_read_u8(data, size, offset + IMG_OFF_PLANE_ONOFF, &onoff))
    {
        return ITIDY_ICON_ERR_TRUNCATED;
    }

    if (width <= 0 || height <= 0 || depth <= 0 ||
        (UWORD)width > ITIDY_ICON_MAX_WIDTH ||
        (UWORD)height > ITIDY_ICON_MAX_HEIGHT ||
        (UWORD)depth > ITIDY_ICON_MAX_DEPTH)
    {
        return ITIDY_ICON_ERR_BAD_DIMENSION;
    }

    if (!icon_file_planar_data_size((UWORD)width, (UWORD)height,
                                    (UWORD)depth, &data_size))
    {
        return ITIDY_ICON_ERR_OVERFLOW;
    }

    if (!icon_file_range_ok(size, offset + ITIDY_ICON_IMAGE_HEADER_SIZE,
                            data_size))
    {
        return ITIDY_ICON_ERR_TRUNCATED;
    }

    loc->present = TRUE;
    loc->header_offset = offset;
    loc->data_offset = offset + ITIDY_ICON_IMAGE_HEADER_SIZE;
    loc->data_size = data_size;
    loc->width = width;
    loc->height = height;
    loc->depth = depth;
    loc->plane_pick = pick;
    loc->plane_onoff = onoff;
    *next_offset = loc->data_offset + data_size;
    return ITIDY_ICON_OK;
}

static iTidy_IconError parse_text(const UBYTE *data, ULONG size,
                                  ULONG offset, iTidy_IconTextLoc *loc,
                                  ULONG *next_offset)
{
    ULONG text_size;

    if (!icon_file_read_u32(data, size, offset, &text_size))
        return ITIDY_ICON_ERR_TRUNCATED;

    if (text_size < 1UL || text_size > ITIDY_ICON_MAX_TEXT_SIZE)
        return ITIDY_ICON_ERR_BAD_TEXT;

    if (!icon_file_range_ok(size, offset + 4UL, text_size))
        return ITIDY_ICON_ERR_TRUNCATED;

    if (data[offset + 4UL + text_size - 1UL] != 0)
        return ITIDY_ICON_ERR_BAD_TEXT;

    loc->present = TRUE;
    loc->size_offset = offset;
    loc->text_offset = offset + 4UL;
    loc->text_size = text_size;
    *next_offset = offset + 4UL + text_size;
    return ITIDY_ICON_OK;
}

static iTidy_IconError parse_tooltypes(const UBYTE *data, ULONG size,
                                       ULONG offset, iTidy_IconFile *out,
                                       ULONG *next_offset)
{
    ULONG encoded;
    ULONG count;
    ULONG i;
    ULONG pos;

    if (!icon_file_read_u32(data, size, offset, &encoded))
        return ITIDY_ICON_ERR_TRUNCATED;

    /* Encoded as (entry_count + 1) * 4 */
    if ((encoded % 4UL) != 0UL || encoded < 4UL)
        return ITIDY_ICON_ERR_BAD_COUNT;

    count = (encoded / 4UL) - 1UL;
    if (count > ITIDY_ICON_MAX_TOOLTYPES)
        return ITIDY_ICON_ERR_BAD_COUNT;

    pos = offset + 4UL;
    for (i = 0; i < count; i++)
    {
        iTidy_IconTextLoc tmp;
        iTidy_IconError err;
        ULONG next_pos;

        memset(&tmp, 0, sizeof(tmp));
        err = parse_text(data, size, pos, &tmp, &next_pos);
        if (err != ITIDY_ICON_OK)
            return err;
        pos = next_pos;
    }

    out->has_tooltypes = TRUE;
    out->tooltypes_offset = offset;
    out->tooltypes_count = count;
    *next_offset = pos;
    return ITIDY_ICON_OK;
}

static BOOL looks_like_extension(const UBYTE *data, ULONG size, ULONG offset)
{
    if (icon_file_range_ok(size, offset, 4) &&
        data[offset] == 'F' && data[offset + 1] == 'O' &&
        data[offset + 2] == 'R' && data[offset + 3] == 'M')
    {
        return TRUE;
    }
    if (icon_file_range_ok(size, offset, 8) &&
        data[offset] == 0x89 && data[offset + 1] == 'P' &&
        data[offset + 2] == 'N' && data[offset + 3] == 'G')
    {
        return TRUE;
    }
    return FALSE;
}

/*========================================================================*/
/* Public parse                                                           */
/*========================================================================*/

iTidy_IconError icon_file_parse(const UBYTE *data, ULONG size,
                                iTidy_IconFile *out)
{
    ULONG gadget_render;
    ULONG select_render;
    ULONG default_tool;
    ULONG tooltypes;
    ULONG drawer_data;
    ULONG tool_window;
    ULONG offset;
    iTidy_IconError err;

    if (out == NULL)
        return ITIDY_ICON_ERR_NULL;

    memset(out, 0, sizeof(*out));
    out->data = data;
    out->size = size;

    if (data == NULL)
        return ITIDY_ICON_ERR_NULL;

    if (size < ITIDY_ICON_DISKOBJECT_SIZE)
        return ITIDY_ICON_ERR_TOO_SMALL;

    if (!icon_file_read_u16(data, size, OFF_MAGIC, &out->magic) ||
        !icon_file_read_u16(data, size, OFF_VERSION, &out->version))
    {
        return ITIDY_ICON_ERR_TRUNCATED;
    }

    if (out->magic != ITIDY_ICON_MAGIC)
        return ITIDY_ICON_ERR_BAD_MAGIC;
    if (out->version != ITIDY_ICON_VERSION)
        return ITIDY_ICON_ERR_BAD_VERSION;

    if (!icon_file_read_s16(data, size, OFF_GADGET_WIDTH, &out->gadget_width) ||
        !icon_file_read_s16(data, size, OFF_GADGET_HEIGHT, &out->gadget_height) ||
        !icon_file_read_u16(data, size, OFF_GADGET_FLAGS, &out->gadget_flags) ||
        !icon_file_read_u32(data, size, OFF_GADGET_RENDER, &gadget_render) ||
        !icon_file_read_u32(data, size, OFF_SELECT_RENDER, &select_render) ||
        !icon_file_read_u32(data, size, OFF_USERDATA, &out->gadget_userdata) ||
        !icon_file_read_u8(data, size, OFF_TYPE, &out->type) ||
        !icon_file_read_u32(data, size, OFF_DEFAULT_TOOL, &default_tool) ||
        !icon_file_read_u32(data, size, OFF_TOOLTYPES, &tooltypes) ||
        !icon_file_read_s32(data, size, OFF_CURRENT_X, &out->current_x) ||
        !icon_file_read_s32(data, size, OFF_CURRENT_Y, &out->current_y) ||
        !icon_file_read_u32(data, size, OFF_DRAWER_DATA, &drawer_data) ||
        !icon_file_read_u32(data, size, OFF_TOOL_WINDOW, &tool_window) ||
        !icon_file_read_s32(data, size, OFF_STACK_SIZE, &out->stack_size))
    {
        return ITIDY_ICON_ERR_TRUNCATED;
    }

    out->has_gadget_render = pointer_present(gadget_render);
    out->has_select_render = pointer_present(select_render);

    offset = ITIDY_ICON_DISKOBJECT_SIZE;

    if (pointer_present(drawer_data))
    {
        if (!icon_file_range_ok(size, offset, ITIDY_ICON_DRAWERDATA_SIZE))
            return ITIDY_ICON_ERR_TRUNCATED;
        out->has_drawer_data = TRUE;
        out->drawer_data_offset = offset;
        offset += ITIDY_ICON_DRAWERDATA_SIZE;
    }

    if (out->has_gadget_render)
    {
        err = parse_image(data, size, offset, &out->normal, &offset);
        if (err != ITIDY_ICON_OK)
            return err;
    }

    if (out->has_select_render)
    {
        err = parse_image(data, size, offset, &out->selected, &offset);
        if (err != ITIDY_ICON_OK)
            return err;
    }

    if (pointer_present(default_tool))
    {
        err = parse_text(data, size, offset, &out->default_tool, &offset);
        if (err != ITIDY_ICON_OK)
            return err;
    }

    if (pointer_present(tooltypes))
    {
        err = parse_tooltypes(data, size, offset, out, &offset);
        if (err != ITIDY_ICON_OK)
            return err;
    }

    if (pointer_present(tool_window))
    {
        err = parse_text(data, size, offset, &out->tool_window, &offset);
        if (err != ITIDY_ICON_OK)
            return err;
    }

    if (out->has_drawer_data && ((out->gadget_userdata & 0xFFUL) == 1UL))
    {
        if (icon_file_range_ok(size, offset, ITIDY_ICON_DRAWERDATA2_SIZE) &&
            !looks_like_extension(data, size, offset))
        {
            out->has_drawer_data2 = TRUE;
            out->drawer_data2_offset = offset;
            offset += ITIDY_ICON_DRAWERDATA2_SIZE;
        }
        else if (offset < size && looks_like_extension(data, size, offset))
        {
            /* UserData claims OS2 DrawerData2, but the remainder is an
             * enhanced payload. Leave DrawerData2 absent. */
        }
        else if (offset != size)
        {
            return ITIDY_ICON_ERR_TRUNCATED;
        }
    }

    if (offset > size)
        return ITIDY_ICON_ERR_OVERFLOW;

    out->classic_end = offset;
    out->extension_offset = offset;
    out->extension_size = size - offset;
    return ITIDY_ICON_OK;
}

BOOL icon_file_tooltype(const iTidy_IconFile *file, ULONG index,
                        const UBYTE **text, ULONG *text_len)
{
    ULONG pos;
    ULONG i;
    ULONG text_size;

    if (file == NULL || file->data == NULL || !file->has_tooltypes)
        return FALSE;
    if (index >= file->tooltypes_count)
        return FALSE;

    pos = file->tooltypes_offset + 4UL;
    for (i = 0; i <= index; i++)
    {
        if (!icon_file_read_u32(file->data, file->size, pos, &text_size))
            return FALSE;
        if (text_size < 1UL ||
            !icon_file_range_ok(file->size, pos + 4UL, text_size))
        {
            return FALSE;
        }
        if (i == index)
        {
            if (text != NULL)
                *text = file->data + pos + 4UL;
            if (text_len != NULL)
                *text_len = text_size - 1UL;
            return TRUE;
        }
        pos += 4UL + text_size;
    }
    return FALSE;
}

BOOL icon_file_default_tool(const iTidy_IconFile *file,
                            const UBYTE **text, ULONG *text_len)
{
    if (file == NULL || file->data == NULL || !file->default_tool.present)
        return FALSE;
    if (file->default_tool.text_size < 1UL)
        return FALSE;
    if (text != NULL)
        *text = file->data + file->default_tool.text_offset;
    if (text_len != NULL)
        *text_len = file->default_tool.text_size - 1UL;
    return TRUE;
}
