/*
 * icon_newicons.c - Decode NewIcons IM1=/IM2= ToolType imagery
 *
 * Layout from support files/IconFormats.txt:
 *   First payload bytes of each image set (not 7-bit encoded):
 *     [0] 'B' transparent / 'C' opaque
 *     [1] width  + 0x21
 *     [2] height + 0x21
 *     [3..4] colour count: ((b3-0x21)<<6) + (b4-0x21)
 *   Remaining bytes of that string, and every later IM1=/IM2= string,
 *   are a 7-bit bitstream:
 *     0x20..0x6F -> 0x00..0x4F
 *     0xA1..0xD0 -> 0x50..0x7F
 *     0xD1..0xFF -> (c-0xD0)*7 zero bits
 *   Each ToolType is a new ASCII segment (do not concatenate "IM1="
 *   prefixes). Already-unpacked sample bits are kept across lines so a
 *   palette/pixel field may finish after a 7-bit character wrap. Pad
 *   bits are added only when an incomplete 7-bit group is flushed
 *   (typically end of image).
 *   Palette is 8-bit RGB triples, then chunky pixels using
 *   ceil(log2(num_colors)) bits (0 bits if a single colour).
 */

#include "icon_newicons.h"

#include <platform/platform.h>
#include <string.h>

#define NI_MAX_WIDTH     93U
#define NI_MAX_HEIGHT    93U
#define NI_MAX_COLORS    256U
#define NI_ASCII_BASE    0x21U
#define NI_LINE_PREFIX   4UL    /* "IM1=" / "IM2=" */

typedef struct NiBits
{
    const UBYTE *p;
    ULONG left;
    ULONG acc;
    int nbits;
    ULONG zero_queued;
} NiBits;

static BOOL mul_ok(ULONG a, ULONG b, ULONG *out)
{
    if (a != 0 && b > (0xFFFFFFFFUL / a))
        return FALSE;
    *out = a * b;
    return TRUE;
}

static void ni_reset(NiBits *b)
{
    b->p = NULL;
    b->left = 0;
    b->acc = 0;
    b->nbits = 0;
    b->zero_queued = 0;
}

static void ni_attach_line(NiBits *b, const UBYTE *p, ULONG n)
{
    /* New ASCII segment. Keep already-unpacked sample bits so a
     * palette byte or pixel that straddles a ToolType boundary can
     * finish. Do not concatenate "IM1="/"IM2=" prefixes. */
    b->p = p;
    b->left = n;
}

static BOOL ni_load_group(NiBits *b)
{
    UBYTE c;

    if (b->left == 0)
        return FALSE;

    c = *b->p++;
    b->left--;

    if (c >= 0xD1U)
    {
        b->zero_queued += (ULONG)(c - 0xD0U) * 7UL;
        return TRUE;
    }
    if (c >= 0x20U && c <= 0x6FU)
    {
        b->acc = (b->acc << 7) | (ULONG)(c - 0x20U);
        b->nbits += 7;
        return TRUE;
    }
    if (c >= 0xA1U && c <= 0xD0U)
    {
        b->acc = (b->acc << 7) | (ULONG)((c - 0xA1U) + 0x50U);
        b->nbits += 7;
        return TRUE;
    }
    return FALSE;
}

static ULONG ni_available(const NiBits *b)
{
    return b->zero_queued + (ULONG)b->nbits;
}

static BOOL ni_ensure(NiBits *b, int nbits)
{
    while (ni_available(b) < (ULONG)nbits)
    {
        if (!ni_load_group(b))
            return FALSE;
    }
    return TRUE;
}

static BOOL ni_get_bit(NiBits *b, UBYTE *out)
{
    if (b->zero_queued > 0)
    {
        b->zero_queued--;
        *out = 0;
        return TRUE;
    }

    if (b->nbits <= 0)
        return FALSE;

    b->nbits--;
    *out = (UBYTE)((b->acc >> b->nbits) & 1UL);
    return TRUE;
}

static BOOL ni_get_bits(NiBits *b, int nbits, UBYTE *out)
{
    UBYTE val = 0;
    int i;

    if (nbits <= 0 || nbits > 8)
        return FALSE;
    if (!ni_ensure(b, nbits))
        return FALSE;

    for (i = 0; i < nbits; i++)
    {
        UBYTE bit;
        if (!ni_get_bit(b, &bit))
            return FALSE;
        val = (UBYTE)((val << 1) | bit);
    }
    *out = val;
    return TRUE;
}

static BOOL starts_with(const UBYTE *text, ULONG len, const char *prefix)
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

static UBYTE colour_bpp(UWORD ncolors)
{
    UWORD n = 1;
    UBYTE bpp = 0;

    while (n < ncolors && bpp < 8)
    {
        n = (UWORD)(n << 1);
        bpp++;
    }
    return bpp;
}

typedef struct NiLine
{
    const UBYTE *payload;   /* bytes after IM1=/IM2= */
    ULONG length;
} NiLine;

static iTidy_IconError collect_lines(const iTidy_IconFile *file,
                                     const char *prefix,
                                     NiLine *lines, ULONG max_lines,
                                     ULONG *out_count)
{
    ULONG i;
    ULONG n = 0;

    for (i = 0; i < file->tooltypes_count; i++)
    {
        const UBYTE *text;
        ULONG len;

        if (!icon_file_tooltype(file, i, &text, &len))
            return ITIDY_ICON_ERR_BAD_TEXT;
        if (!starts_with(text, len, prefix))
            continue;
        if (len < NI_LINE_PREFIX)
            return ITIDY_ICON_ERR_TRUNCATED;
        if (n >= max_lines)
            return ITIDY_ICON_ERR_OVERFLOW;
        lines[n].payload = text + NI_LINE_PREFIX;
        lines[n].length = len - NI_LINE_PREFIX;
        n++;
    }
    *out_count = n;
    return ITIDY_ICON_OK;
}

static iTidy_IconError decode_image_set(const NiLine *lines, ULONG nlines,
                                        iTidy_IndexedImage *img)
{
    const UBYTE *hdr;
    ULONG hdr_len;
    UWORD width;
    UWORD height;
    UWORD ncolors;
    UBYTE bpp;
    ULONG pix_count;
    ULONG pal_bytes;
    ULONG pal_got;
    ULONG pix_got;
    ULONG line_i;
    NiBits bits;
    UBYTE *pixels;
    iTidy_RGB8 *palette;
    BOOL trans;

    if (nlines == 0)
        return ITIDY_ICON_ERR_NO_DATA;

    hdr = lines[0].payload;
    hdr_len = lines[0].length;
    if (hdr_len < 5UL)
        return ITIDY_ICON_ERR_TRUNCATED;

    if (hdr[1] < NI_ASCII_BASE || hdr[2] < NI_ASCII_BASE ||
        hdr[3] < NI_ASCII_BASE || hdr[4] < NI_ASCII_BASE)
    {
        return ITIDY_ICON_ERR_BAD_DIMENSION;
    }

    width = (UWORD)(hdr[1] - NI_ASCII_BASE);
    height = (UWORD)(hdr[2] - NI_ASCII_BASE);
    ncolors = (UWORD)(((UWORD)(hdr[3] - NI_ASCII_BASE) << 6) +
                      (UWORD)(hdr[4] - NI_ASCII_BASE));
    trans = (hdr[0] == (UBYTE)'B');

    if (width < 1U || height < 1U ||
        width > NI_MAX_WIDTH || height > NI_MAX_HEIGHT)
    {
        return ITIDY_ICON_ERR_BAD_DIMENSION;
    }
    if (ncolors < 1U || ncolors > NI_MAX_COLORS)
        return ITIDY_ICON_ERR_BAD_COUNT;

    bpp = colour_bpp(ncolors);
    if ((UWORD)(1U << bpp) < ncolors && ncolors > 1U)
        return ITIDY_ICON_ERR_BAD_COUNT;

    if (!mul_ok((ULONG)width, (ULONG)height, &pix_count))
        return ITIDY_ICON_ERR_OVERFLOW;
    pal_bytes = (ULONG)ncolors * 3UL;

    pixels = (UBYTE *)whd_malloc(pix_count);
    if (pixels == NULL)
        return ITIDY_ICON_ERR_ALLOC;
    memset(pixels, 0, pix_count);

    palette = (iTidy_RGB8 *)whd_malloc(sizeof(iTidy_RGB8) * (ULONG)ncolors);
    if (palette == NULL)
    {
        whd_free(pixels);
        return ITIDY_ICON_ERR_ALLOC;
    }
    memset(palette, 0, sizeof(iTidy_RGB8) * (ULONG)ncolors);

    pal_got = 0;
    pix_got = 0;
    ni_reset(&bits);

    for (line_i = 0; line_i < nlines; line_i++)
    {
        const UBYTE *pay = lines[line_i].payload;
        ULONG pay_len = lines[line_i].length;

        if (line_i == 0)
        {
            pay += 5UL;
            pay_len -= 5UL;
        }

        ni_attach_line(&bits, pay, pay_len);

        while (pal_got < pal_bytes)
        {
            UBYTE v;
            ULONG ci;
            ULONG ch;

            if (!ni_get_bits(&bits, 8, &v))
                break;
            ci = pal_got / 3UL;
            ch = pal_got % 3UL;
            if (ch == 0)
                palette[ci].r = v;
            else if (ch == 1)
                palette[ci].g = v;
            else
                palette[ci].b = v;
            pal_got++;
        }

        if (pal_got < pal_bytes)
            continue;

        if (bpp == 0)
        {
            pix_got = pix_count;
            break;
        }

        while (pix_got < pix_count)
        {
            UBYTE v;
            if (!ni_get_bits(&bits, (int)bpp, &v))
                break;
            pixels[pix_got++] = v;
        }

        if (pix_got >= pix_count)
            break;
    }

    if (pal_got < pal_bytes || pix_got < pix_count)
    {
        whd_free(pixels);
        whd_free(palette);
        return ITIDY_ICON_ERR_TRUNCATED;
    }

    img->width = width;
    img->height = height;
    img->pixels = pixels;
    img->palette = palette;
    img->palette_count = ncolors;
    img->transparent_index = trans ? 0L : -1L;
    return ITIDY_ICON_OK;
}

iTidy_IconError icon_newicons_decode(const iTidy_IconFile *file,
                                     iTidy_DecodedIcon *out)
{
    NiLine *im1 = NULL;
    NiLine *im2 = NULL;
    ULONG n1 = 0;
    ULONG n2 = 0;
    ULONG cap;
    iTidy_IconError err;

    if (out == NULL)
        return ITIDY_ICON_ERR_NULL;
    memset(out, 0, sizeof(*out));

    if (file == NULL || file->data == NULL)
        return ITIDY_ICON_ERR_NULL;

    if (!file->has_tooltypes || file->tooltypes_count == 0)
        return ITIDY_ICON_ERR_NO_DATA;

    cap = file->tooltypes_count;
    im1 = (NiLine *)whd_malloc(sizeof(NiLine) * cap);
    im2 = (NiLine *)whd_malloc(sizeof(NiLine) * cap);
    if (im1 == NULL || im2 == NULL)
    {
        if (im1 != NULL)
            whd_free(im1);
        if (im2 != NULL)
            whd_free(im2);
        return ITIDY_ICON_ERR_ALLOC;
    }

    err = collect_lines(file, "IM1=", im1, cap, &n1);
    if (err == ITIDY_ICON_OK)
        err = collect_lines(file, "IM2=", im2, cap, &n2);

    if (err == ITIDY_ICON_OK && n1 == 0)
        err = ITIDY_ICON_ERR_NO_DATA;

    if (err == ITIDY_ICON_OK)
        err = decode_image_set(im1, n1, &out->normal);

    if (err == ITIDY_ICON_OK && n2 > 0)
    {
        err = decode_image_set(im2, n2, &out->selected);
        if (err == ITIDY_ICON_OK)
            out->has_selected = TRUE;
    }

    whd_free(im1);
    whd_free(im2);

    if (err != ITIDY_ICON_OK)
    {
        icon_decoded_free(out);
        return err;
    }

    out->source_format = ITIDY_ICON_SRC_NEWICONS;
    out->frameless = FALSE;
    return ITIDY_ICON_OK;
}
