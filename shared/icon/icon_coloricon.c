/*
 * icon_coloricon.c - Direct decoder for OS3.5 ColorIcon / GlowIcon
 *
 * Layout (IconFormats.txt + deep-research-report-colorIcons.md):
 *   FORM / size / ICON
 *   FACE: width-1, height-1, flags, aspect, max palette RGB bytes-1
 *   IMAG: header (10 bytes) then image bytes then optional RGB palette
 *   Odd chunk sizes are padded to even.
 *   Unknown chunks are skipped.
 *
 * Image format 0: one byte per pixel (even when Depth < 8).
 * Image format 1: ColorIcon variable-bit RLE — continuous bitstream of
 *   8-bit controls and Depth-bit samples (0x80 is a no-op; do not
 *   byte-align between fields). Palette RLE uses 8-bit entries.
 * Second IMAG may omit its palette and inherit the first.
 * GlowIcon is the same IMAG encoding; FACE max-palette-bytes is only a
 * labelling heuristic (see source_format).
 */

#include "icon_coloricon.h"

#include <platform/platform.h>
#include <string.h>

#define FACE_PAYLOAD_MIN    6UL
#define IMAG_HEADER_SIZE    10UL
#define IMAG_FLAG_TRANSPARENT  0x01U
#define IMAG_FLAG_PALETTE      0x02U
#define FACE_FLAG_FRAMELESS    0x01U

#define FMT_UNCOMPRESSED    0
#define FMT_RLE             1

typedef struct BitReader
{
    const UBYTE *data;
    ULONG bit_limit;
    ULONG bit_pos;
} BitReader;

typedef struct ImagHeader
{
    UBYTE transparent;
    UWORD num_colors;
    UBYTE flags;
    UBYTE image_format;
    UBYTE pal_format;
    UBYTE depth;
    ULONG image_bytes;
    ULONG pal_bytes;
    BOOL has_transparent;
    BOOL has_palette;
} ImagHeader;

static BOOL fourcc_eq(const UBYTE *p, const char *id)
{
    return p[0] == (UBYTE)id[0] && p[1] == (UBYTE)id[1] &&
           p[2] == (UBYTE)id[2] && p[3] == (UBYTE)id[3];
}

static void image_clear(iTidy_IndexedImage *img)
{
    if (img == NULL)
        return;
    if (img->pixels != NULL)
        whd_free(img->pixels);
    if (img->palette != NULL)
        whd_free(img->palette);
    memset(img, 0, sizeof(*img));
    img->transparent_index = -1;
}

void icon_decoded_free(iTidy_DecodedIcon *decoded)
{
    if (decoded == NULL)
        return;
    image_clear(&decoded->normal);
    image_clear(&decoded->selected);
    memset(decoded, 0, sizeof(*decoded));
}

static BOOL mul_ok(ULONG a, ULONG b, ULONG *out)
{
    if (a != 0 && b > (0xFFFFFFFFUL / a))
        return FALSE;
    *out = a * b;
    return TRUE;
}

static BOOL bitreader_init(BitReader *br, const UBYTE *data, ULONG size)
{
    if (size > (0xFFFFFFFFUL / 8UL))
        return FALSE;
    br->data = data;
    br->bit_limit = size * 8UL;
    br->bit_pos = 0;
    return TRUE;
}

static BOOL read_bits(BitReader *br, UBYTE nbits, UBYTE *out)
{
    UBYTE i;
    UBYTE val = 0;

    if (nbits == 0 || nbits > 8)
        return FALSE;
    if (br->bit_pos > br->bit_limit)
        return FALSE;
    if ((br->bit_limit - br->bit_pos) < (ULONG)nbits)
        return FALSE;

    for (i = 0; i < nbits; i++)
    {
        ULONG byte_idx = br->bit_pos >> 3;
        UBYTE bit_idx = (UBYTE)(7U - (UBYTE)(br->bit_pos & 7UL));

        val = (UBYTE)((val << 1) | ((br->data[byte_idx] >> bit_idx) & 1U));
        br->bit_pos++;
    }
    *out = val;
    return TRUE;
}

static iTidy_IconError unpack_rle(const UBYTE *src, ULONG src_len,
                                  UBYTE *dest, ULONG dest_count,
                                  UBYTE depth)
{
    BitReader br;
    ULONG written = 0;

    if (depth == 0 || depth > 8)
        return ITIDY_ICON_ERR_BAD_DIMENSION;
    if (!bitreader_init(&br, src, src_len))
        return ITIDY_ICON_ERR_OVERFLOW;

    while (written < dest_count)
    {
        UBYTE control;
        ULONG n;
        ULONG i;

        if (!read_bits(&br, 8, &control))
            return ITIDY_ICON_ERR_TRUNCATED;

        if (control == 0x80U)
            continue;

        if (control <= 0x7FU)
        {
            n = (ULONG)control + 1UL;
            if (n > (dest_count - written))
                return ITIDY_ICON_ERR_OVERFLOW;
            for (i = 0; i < n; i++)
            {
                UBYTE sample;
                if (!read_bits(&br, depth, &sample))
                    return ITIDY_ICON_ERR_TRUNCATED;
                dest[written++] = sample;
            }
        }
        else
        {
            UBYTE sample;

            n = 257UL - (ULONG)control;
            if (n > (dest_count - written))
                return ITIDY_ICON_ERR_OVERFLOW;
            if (!read_bits(&br, depth, &sample))
                return ITIDY_ICON_ERR_TRUNCATED;
            for (i = 0; i < n; i++)
                dest[written++] = sample;
        }
    }

    return ITIDY_ICON_OK;
}

static iTidy_IconError copy_palette(iTidy_IndexedImage *dst,
                                    const iTidy_IndexedImage *src)
{
    ULONG bytes;

    if (src->palette == NULL || src->palette_count == 0)
        return ITIDY_ICON_ERR_NO_DATA;
    if (!mul_ok((ULONG)src->palette_count, sizeof(iTidy_RGB8), &bytes))
        return ITIDY_ICON_ERR_OVERFLOW;
    dst->palette = (iTidy_RGB8 *)whd_malloc(bytes);
    if (dst->palette == NULL)
        return ITIDY_ICON_ERR_ALLOC;
    memcpy(dst->palette, src->palette, (size_t)bytes);
    dst->palette_count = src->palette_count;
    return ITIDY_ICON_OK;
}

static iTidy_IconError parse_imag_header(const UBYTE *data, ULONG size,
                                         ULONG payload, ULONG chunk_size,
                                         ImagHeader *hdr)
{
    UBYTE num_m1;
    UWORD image_m1;
    UWORD pal_m1;

    if (chunk_size < IMAG_HEADER_SIZE)
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_range_ok(size, payload, IMAG_HEADER_SIZE))
        return ITIDY_ICON_ERR_TRUNCATED;

    if (!icon_file_read_u8(data, size, payload, &hdr->transparent))
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_read_u8(data, size, payload + 1UL, &num_m1))
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_read_u8(data, size, payload + 2UL, &hdr->flags))
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_read_u8(data, size, payload + 3UL, &hdr->image_format))
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_read_u8(data, size, payload + 4UL, &hdr->pal_format))
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_read_u8(data, size, payload + 5UL, &hdr->depth))
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_read_u16(data, size, payload + 6UL, &image_m1))
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_read_u16(data, size, payload + 8UL, &pal_m1))
        return ITIDY_ICON_ERR_TRUNCATED;

    hdr->num_colors = (UWORD)num_m1 + 1U;
    hdr->image_bytes = (ULONG)image_m1 + 1UL;
    hdr->has_transparent = (hdr->flags & IMAG_FLAG_TRANSPARENT) ? TRUE : FALSE;
    hdr->has_palette = (hdr->flags & IMAG_FLAG_PALETTE) ? TRUE : FALSE;
    /* Palette size field is meaningful only when HASPALETTE is set.
     * A stored size of 0 must not become a 1-byte palette consume. */
    if (hdr->has_palette)
        hdr->pal_bytes = (ULONG)pal_m1 + 1UL;
    else
        hdr->pal_bytes = 0;

    if (hdr->depth == 0 || hdr->depth > ITIDY_ICON_MAX_DEPTH)
        return ITIDY_ICON_ERR_BAD_DIMENSION;
    if (hdr->image_format != FMT_UNCOMPRESSED && hdr->image_format != FMT_RLE)
        return ITIDY_ICON_ERR_UNSUPPORTED;
    if (hdr->has_palette &&
        hdr->pal_format != FMT_UNCOMPRESSED && hdr->pal_format != FMT_RLE)
    {
        return ITIDY_ICON_ERR_UNSUPPORTED;
    }
    if (hdr->num_colors == 0)
        return ITIDY_ICON_ERR_BAD_COUNT;
    /* Actual colour count is NumColors+1, not 2^Depth. When a palette is
     * present it must still fit in Depth bits. */
    if (hdr->has_palette &&
        (ULONG)hdr->num_colors > (1UL << hdr->depth))
    {
        return ITIDY_ICON_ERR_BAD_COUNT;
    }

    {
        ULONG need = IMAG_HEADER_SIZE + hdr->image_bytes;
        if (hdr->has_palette)
        {
            if (need > (0xFFFFFFFFUL - hdr->pal_bytes))
                return ITIDY_ICON_ERR_OVERFLOW;
            need += hdr->pal_bytes;
        }
        if (need > chunk_size)
            return ITIDY_ICON_ERR_TRUNCATED;
        if (!icon_file_range_ok(size, payload, need))
            return ITIDY_ICON_ERR_TRUNCATED;
    }

    return ITIDY_ICON_OK;
}

static iTidy_IconError decode_pixels(const UBYTE *src, ULONG src_len,
                                     UBYTE format, UBYTE depth,
                                     UBYTE *dest, ULONG dest_count)
{
    if (format == FMT_UNCOMPRESSED)
    {
        /* Raw ColorIcon imagery is one byte per pixel, not Depth-packed. */
        if (src_len != dest_count)
            return ITIDY_ICON_ERR_TRUNCATED;
        memcpy(dest, src, (size_t)dest_count);
        return ITIDY_ICON_OK;
    }
    return unpack_rle(src, src_len, dest, dest_count, depth);
}

static iTidy_IconError decode_palette(const UBYTE *src, ULONG src_len,
                                      UBYTE format, UWORD num_colors,
                                      iTidy_IndexedImage *img)
{
    ULONG rgb_count;
    ULONG pal_bytes;
    UBYTE *rgb = NULL;
    UWORD i;
    iTidy_IconError err;

    if (!mul_ok((ULONG)num_colors, 3UL, &rgb_count))
        return ITIDY_ICON_ERR_OVERFLOW;
    if (!mul_ok((ULONG)num_colors, sizeof(iTidy_RGB8), &pal_bytes))
        return ITIDY_ICON_ERR_OVERFLOW;

    rgb = (UBYTE *)whd_malloc(rgb_count);
    if (rgb == NULL)
        return ITIDY_ICON_ERR_ALLOC;

    if (format == FMT_UNCOMPRESSED)
    {
        /* Palette RGB byte count is num_colors * 3, not (1 << depth) * 3. */
        if (src_len != rgb_count)
        {
            whd_free(rgb);
            return ITIDY_ICON_ERR_TRUNCATED;
        }
        memcpy(rgb, src, (size_t)rgb_count);
        err = ITIDY_ICON_OK;
    }
    else
    {
        err = unpack_rle(src, src_len, rgb, rgb_count, 8);
    }

    if (err != ITIDY_ICON_OK)
    {
        whd_free(rgb);
        return err;
    }

    img->palette = (iTidy_RGB8 *)whd_malloc(pal_bytes);
    if (img->palette == NULL)
    {
        whd_free(rgb);
        return ITIDY_ICON_ERR_ALLOC;
    }

    for (i = 0; i < num_colors; i++)
    {
        img->palette[i].r = rgb[(ULONG)i * 3UL];
        img->palette[i].g = rgb[(ULONG)i * 3UL + 1UL];
        img->palette[i].b = rgb[(ULONG)i * 3UL + 2UL];
    }
    img->palette_count = num_colors;
    whd_free(rgb);
    return ITIDY_ICON_OK;
}

static iTidy_IconError validate_indices(const iTidy_IndexedImage *img)
{
    ULONG i;
    ULONG pixel_count;
    UWORD lim;

    if (img == NULL || img->pixels == NULL || img->palette == NULL ||
        img->palette_count == 0)
    {
        return ITIDY_ICON_ERR_NO_DATA;
    }
    if (!mul_ok((ULONG)img->width, (ULONG)img->height, &pixel_count))
        return ITIDY_ICON_ERR_OVERFLOW;

    lim = img->palette_count;
    for (i = 0; i < pixel_count; i++)
    {
        if ((UWORD)img->pixels[i] >= lim)
            return ITIDY_ICON_ERR_BAD_COUNT;
    }
    if (img->transparent_index >= 0 &&
        (ULONG)img->transparent_index >= (ULONG)lim)
    {
        return ITIDY_ICON_ERR_BAD_COUNT;
    }
    return ITIDY_ICON_OK;
}

static iTidy_IconError decode_imag(const UBYTE *data, ULONG size,
                                   ULONG payload, ULONG chunk_size,
                                   UWORD width, UWORD height,
                                   const iTidy_IndexedImage *inherit_pal,
                                   iTidy_IndexedImage *out)
{
    ImagHeader hdr;
    iTidy_IconError err;
    ULONG pixel_count;
    ULONG img_off;
    ULONG pal_off;

    memset(out, 0, sizeof(*out));
    out->transparent_index = -1;

    err = parse_imag_header(data, size, payload, chunk_size, &hdr);
    if (err != ITIDY_ICON_OK)
        return err;

    if (!mul_ok((ULONG)width, (ULONG)height, &pixel_count))
        return ITIDY_ICON_ERR_OVERFLOW;
    if (pixel_count == 0)
        return ITIDY_ICON_ERR_BAD_DIMENSION;

    out->pixels = (UBYTE *)whd_malloc(pixel_count);
    if (out->pixels == NULL)
        return ITIDY_ICON_ERR_ALLOC;

    img_off = payload + IMAG_HEADER_SIZE;
    err = decode_pixels(data + img_off, hdr.image_bytes,
                        hdr.image_format, hdr.depth,
                        out->pixels, pixel_count);
    if (err != ITIDY_ICON_OK)
    {
        image_clear(out);
        return err;
    }

    pal_off = img_off + hdr.image_bytes;
    if (hdr.has_palette)
    {
        err = decode_palette(data + pal_off, hdr.pal_bytes,
                             hdr.pal_format, hdr.num_colors, out);
        if (err != ITIDY_ICON_OK)
        {
            image_clear(out);
            return err;
        }
    }
    else if (inherit_pal != NULL)
    {
        err = copy_palette(out, inherit_pal);
        if (err != ITIDY_ICON_OK)
        {
            image_clear(out);
            return err;
        }
    }
    else
    {
        image_clear(out);
        return ITIDY_ICON_ERR_NO_DATA;
    }

    out->width = width;
    out->height = height;
    if (hdr.has_transparent)
        out->transparent_index = (LONG)hdr.transparent;
    else
        out->transparent_index = -1;

    err = validate_indices(out);
    if (err != ITIDY_ICON_OK)
    {
        image_clear(out);
        return err;
    }

    return ITIDY_ICON_OK;
}

static iTidy_IconError walk_next_chunk(const UBYTE *data, ULONG size,
                                       ULONG end, ULONG pos,
                                       ULONG *payload, ULONG *chunk_size,
                                       ULONG *next_pos)
{
    ULONG padded;

    if (pos >= end)
        return ITIDY_ICON_ERR_NO_DATA;
    if (!icon_file_range_ok(size, pos, 8) || (pos + 8UL) > end)
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_read_u32(data, size, pos + 4UL, chunk_size))
        return ITIDY_ICON_ERR_TRUNCATED;
    *payload = pos + 8UL;
    if (!icon_file_range_ok(size, *payload, *chunk_size) ||
        (*payload + *chunk_size) > end)
    {
        return ITIDY_ICON_ERR_TRUNCATED;
    }
    padded = *chunk_size + (*chunk_size & 1UL);
    if (*payload > end || padded > (end - *payload))
        return ITIDY_ICON_ERR_TRUNCATED;
    *next_pos = *payload + padded;
    return ITIDY_ICON_OK;
}

static iTidy_IconError decode_form_icon(const UBYTE *data, ULONG size,
                                        ULONG form_offset, ULONG form_size,
                                        iTidy_DecodedIcon *out)
{
    ULONG end;
    ULONG pos;
    ULONG payload;
    ULONG chunk_size;
    ULONG next_pos;
    UWORD width = 0;
    UWORD height = 0;
    UWORD max_pal_bytes_m1 = 0;
    BOOL saw_face = FALSE;
    BOOL saw_argb = FALSE;
    int imag_count = 0;
    iTidy_IconError err;

    if (!icon_file_range_ok(size, form_offset + 8UL, form_size))
        return ITIDY_ICON_ERR_TRUNCATED;
    if (form_size < 4UL)
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_range_ok(size, form_offset + 8UL, 4) ||
        !fourcc_eq(data + form_offset + 8UL, "ICON"))
    {
        return ITIDY_ICON_ERR_UNSUPPORTED;
    }

    end = form_offset + 8UL + form_size;

    /* Pass 1: FACE (IFF does not require FACE before IMAG). */
    pos = form_offset + 12UL;
    while ((err = walk_next_chunk(data, size, end, pos,
                                  &payload, &chunk_size, &next_pos)) == ITIDY_ICON_OK)
    {
        if (fourcc_eq(data + pos, "FACE"))
        {
            UBYTE w_m1, h_m1, flags;

            if (chunk_size < FACE_PAYLOAD_MIN)
                return ITIDY_ICON_ERR_TRUNCATED;
            if (!icon_file_read_u8(data, size, payload, &w_m1) ||
                !icon_file_read_u8(data, size, payload + 1UL, &h_m1) ||
                !icon_file_read_u8(data, size, payload + 2UL, &flags) ||
                !icon_file_read_u16(data, size, payload + 4UL, &max_pal_bytes_m1))
            {
                return ITIDY_ICON_ERR_TRUNCATED;
            }
            width = (UWORD)w_m1 + 1U;
            height = (UWORD)h_m1 + 1U;
            if (width > ITIDY_ICON_MAX_WIDTH || height > ITIDY_ICON_MAX_HEIGHT)
                return ITIDY_ICON_ERR_BAD_DIMENSION;
            out->frameless = (flags & FACE_FLAG_FRAMELESS) ? TRUE : FALSE;
            saw_face = TRUE;
        }
        else if (fourcc_eq(data + pos, "ARGB"))
        {
            saw_argb = TRUE;
        }
        pos = next_pos;
    }
    if (err != ITIDY_ICON_ERR_NO_DATA)
        return err;
    if (!saw_face)
        return ITIDY_ICON_ERR_NO_DATA;

    /* Pass 2: IMAG in file order (first = normal, second = selected). */
    pos = form_offset + 12UL;
    while ((err = walk_next_chunk(data, size, end, pos,
                                  &payload, &chunk_size, &next_pos)) == ITIDY_ICON_OK)
    {
        if (fourcc_eq(data + pos, "IMAG"))
        {
            iTidy_IndexedImage *slot;
            const iTidy_IndexedImage *inherit;

            if (imag_count >= 2)
            {
                pos = next_pos;
                continue;
            }

            slot = (imag_count == 0) ? &out->normal : &out->selected;
            inherit = (imag_count == 0) ? NULL : &out->normal;
            err = decode_imag(data, size, payload, chunk_size,
                              width, height, inherit, slot);
            if (err != ITIDY_ICON_OK)
                return err;
            imag_count++;
            if (imag_count == 2)
                out->has_selected = TRUE;
        }
        pos = next_pos;
    }
    if (err != ITIDY_ICON_ERR_NO_DATA)
        return err;

    if (imag_count < 1)
    {
        if (saw_argb)
            return ITIDY_ICON_ERR_UNSUPPORTED;
        return ITIDY_ICON_ERR_NO_DATA;
    }

    /* Labelling only: same IMAG decoder. FACE stores max RGB bytes - 1. */
    if (max_pal_bytes_m1 >= 255U)
        out->source_format = ITIDY_ICON_SRC_GLOWICON;
    else
        out->source_format = ITIDY_ICON_SRC_COLORICON;

    return ITIDY_ICON_OK;
}

iTidy_IconError icon_coloricon_decode(const iTidy_IconFile *file,
                                      iTidy_DecodedIcon *out)
{
    ULONG form_size;
    const UBYTE *data;
    ULONG size;
    ULONG offset;
    iTidy_IconError err;

    if (out == NULL)
        return ITIDY_ICON_ERR_NULL;
    memset(out, 0, sizeof(*out));

    if (file == NULL || file->data == NULL)
        return ITIDY_ICON_ERR_NULL;

    data = file->data;
    size = file->size;
    offset = file->extension_offset;

    if (file->extension_size == 0)
        return ITIDY_ICON_ERR_NO_DATA;
    if (file->extension_size < 12UL)
        return ITIDY_ICON_ERR_TRUNCATED;
    if (!icon_file_range_ok(size, offset, 12UL))
        return ITIDY_ICON_ERR_TRUNCATED;

    if (!fourcc_eq(data + offset, "FORM"))
        return ITIDY_ICON_ERR_UNSUPPORTED;

    if (!icon_file_read_u32(data, size, offset + 4UL, &form_size))
        return ITIDY_ICON_ERR_TRUNCATED;

    if (!fourcc_eq(data + offset + 8UL, "ICON"))
        return ITIDY_ICON_ERR_UNSUPPORTED;

    err = decode_form_icon(data, size, offset, form_size, out);
    if (err != ITIDY_ICON_OK)
        icon_decoded_free(out);
    return err;
}
