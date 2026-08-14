/*
 * icon_classic.c - Classic planar Image -> chunky Workbench-pen indexes
 *
 * Image data is stored as sequential bitplanes, each row padded to a
 * 16-bit word. Bit 7 of each byte is the leftmost pixel in that byte.
 * PlanePick / PlaneOnOff follow Intuition struct Image colouring so the
 * resulting indexes are the pens Workbench would use.
 *
 * No RGB palette is embedded in classic icons.
 */

#include "icon_classic.h"

#include <platform/platform.h>
#include <string.h>

static BOOL mul_ok(ULONG a, ULONG b, ULONG *out)
{
    if (a != 0 && b > (0xFFFFFFFFUL / a))
        return FALSE;
    *out = a * b;
    return TRUE;
}

static iTidy_IconError decode_planar(const iTidy_IconFile *file,
                                     const iTidy_IconImageLoc *loc,
                                     iTidy_IndexedImage *img)
{
    ULONG bpr;
    ULONG pix_count;
    ULONG plane_bytes;
    UBYTE *pixels;
    UWORD x;
    UWORD y;
    UWORD p;
    UWORD depth;
    UBYTE pick;
    UBYTE onoff;

    if (!loc->present)
        return ITIDY_ICON_ERR_NO_DATA;

    if (loc->width <= 0 || loc->height <= 0 || loc->depth <= 0)
        return ITIDY_ICON_ERR_BAD_DIMENSION;

    depth = (UWORD)loc->depth;
    pick = loc->plane_pick;
    onoff = loc->plane_onoff;

    if (!icon_file_planar_data_size((UWORD)loc->width, (UWORD)loc->height,
                                    depth, &plane_bytes))
    {
        return ITIDY_ICON_ERR_OVERFLOW;
    }
    if (plane_bytes != loc->data_size)
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_range_ok(file->size, loc->data_offset, loc->data_size))
        return ITIDY_ICON_ERR_TRUNCATED;

    bpr = (((ULONG)loc->width + 15UL) / 16UL) * 2UL;
    if (!mul_ok((ULONG)loc->width, (ULONG)loc->height, &pix_count))
        return ITIDY_ICON_ERR_OVERFLOW;

    pixels = (UBYTE *)whd_malloc(pix_count);
    if (pixels == NULL)
        return ITIDY_ICON_ERR_ALLOC;
    memset(pixels, 0, pix_count);

    for (y = 0; y < (UWORD)loc->height; y++)
    {
        for (x = 0; x < (UWORD)loc->width; x++)
        {
            UBYTE pen = onoff;
            ULONG byte_in_row = (ULONG)x / 8UL;
            UBYTE bit = (UBYTE)(7U - (UBYTE)(x & 7U));

            for (p = 0; p < depth; p++)
            {
                ULONG off;
                UBYTE sample;

                if ((pick & (UBYTE)(1U << p)) == 0)
                    continue;

                off = loc->data_offset +
                      ((ULONG)p * (ULONG)loc->height + (ULONG)y) * bpr +
                      byte_in_row;
                sample = file->data[off];
                if (sample & (UBYTE)(1U << bit))
                    pen = (UBYTE)(pen | (UBYTE)(1U << p));
                else
                    pen = (UBYTE)(pen & (UBYTE)~(1U << p));
            }

            pixels[(ULONG)y * (ULONG)loc->width + (ULONG)x] = pen;
        }
    }

    img->width = (UWORD)loc->width;
    img->height = (UWORD)loc->height;
    img->pixels = pixels;
    img->palette = NULL;
    img->palette_count = 0;
    img->transparent_index = -1;
    return ITIDY_ICON_OK;
}

iTidy_IconError icon_classic_decode(const iTidy_IconFile *file,
                                    iTidy_DecodedIcon *out)
{
    iTidy_IconError err;

    if (out == NULL)
        return ITIDY_ICON_ERR_NULL;
    memset(out, 0, sizeof(*out));

    if (file == NULL || file->data == NULL)
        return ITIDY_ICON_ERR_NULL;

    if (!file->normal.present)
        return ITIDY_ICON_ERR_NO_DATA;

    err = decode_planar(file, &file->normal, &out->normal);
    if (err != ITIDY_ICON_OK)
    {
        icon_decoded_free(out);
        return err;
    }

    if (file->selected.present)
    {
        err = decode_planar(file, &file->selected, &out->selected);
        if (err != ITIDY_ICON_OK)
        {
            icon_decoded_free(out);
            return err;
        }
        out->has_selected = TRUE;
    }

    out->source_format = ITIDY_ICON_SRC_CLASSIC;
    out->frameless = FALSE;
    return ITIDY_ICON_OK;
}
