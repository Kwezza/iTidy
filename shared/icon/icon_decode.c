/*
 * icon_decode.c - Unified probe/decode dispatch
 *
 * Preference for BEST: ColorIcon/GlowIcon, then NewIcons, then classic.
 * PNG and OS4 ARGB are unsupported, not treated as corrupt DiskObjects.
 */

#include "icon_decode.h"

#include <string.h>

static BOOL is_png_signature(const UBYTE *data, ULONG size)
{
    static const UBYTE png[8] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
    };
    if (data == NULL || size < 8UL)
        return FALSE;
    return memcmp(data, png, 8) == 0;
}

static iTidy_IconError decode_best(const iTidy_IconFile *file,
                                   const iTidy_IconProbe *probe,
                                   iTidy_DecodedIcon *out)
{
    iTidy_IconError err;

    if (probe->has_coloricon || probe->has_glowicon)
    {
        err = icon_coloricon_decode(file, out);
        if (err == ITIDY_ICON_OK)
            return ITIDY_ICON_OK;
        if (err != ITIDY_ICON_ERR_NO_DATA)
            return err;
    }

    if (probe->has_newicons)
    {
        err = icon_newicons_decode(file, out);
        if (err == ITIDY_ICON_OK)
            return ITIDY_ICON_OK;
        if (err != ITIDY_ICON_ERR_NO_DATA)
            return err;
    }

    if (probe->has_classic)
        return icon_classic_decode(file, out);

    if (probe->has_png || probe->has_os4_argb || probe->has_unsupported)
        return ITIDY_ICON_ERR_UNSUPPORTED;

    return ITIDY_ICON_ERR_NO_DATA;
}

iTidy_IconError icon_decode(const iTidy_IconFile *file, ULONG request,
                            iTidy_DecodedIcon *out)
{
    iTidy_IconProbe probe;
    iTidy_IconError err;

    if (out == NULL)
        return ITIDY_ICON_ERR_NULL;
    memset(out, 0, sizeof(*out));

    if (file == NULL || file->data == NULL)
        return ITIDY_ICON_ERR_NULL;

    err = icon_probe_file(file, &probe);
    if (err != ITIDY_ICON_OK)
        return err;

    switch (request)
    {
        case ITIDY_ICON_REQ_BEST:
            return decode_best(file, &probe, out);

        case ITIDY_ICON_REQ_COLORICON:
            return icon_coloricon_decode(file, out);

        case ITIDY_ICON_REQ_NEWICONS:
            return icon_newicons_decode(file, out);

        case ITIDY_ICON_REQ_CLASSIC:
            return icon_classic_decode(file, out);

        default:
            return ITIDY_ICON_ERR_UNSUPPORTED;
    }
}

iTidy_IconError icon_decode_buffer(const UBYTE *data, ULONG size,
                                   ULONG request, iTidy_DecodedIcon *out)
{
    iTidy_IconFile file;
    iTidy_IconError err;

    if (out == NULL)
        return ITIDY_ICON_ERR_NULL;
    memset(out, 0, sizeof(*out));

    if (is_png_signature(data, size))
        return ITIDY_ICON_ERR_UNSUPPORTED;

    err = icon_file_parse(data, size, &file);
    if (err != ITIDY_ICON_OK)
        return err;
    return icon_decode(&file, request, out);
}
