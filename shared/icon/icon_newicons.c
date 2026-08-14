/*
 * icon_newicons.c - Decode NewIcons IM1=/IM2= ToolType imagery
 *
 * Layout from support files/IconFormats.txt and the AROS/Deark-backed
 * NewIcons research report:
 *   First payload bytes of each image set (not 7-bit encoded):
 *     [0] 'B' transparent / 'C' opaque
 *     [1] width  + 0x21
 *     [2] height + 0x21
 *     [3..4] colour count: ((b3-0x21)<<6) + (b4-0x21)
 *   Remaining bytes of that string, and every later IM1=/IM2= string,
 *   are a 7-bit bitstream:
 *     0x20..0x6F -> 0x00..0x4F
 *     0xA1..0xD0 -> 0x50..0x7F
 *     0xD1..0xFF -> (c-0xD0) seven-bit groups of zero
 *   Ordinary groups and RLE zero groups enter one ordered accumulator.
 *   Every physical ToolType is a framing boundary: residual bits are
 *   discarded. Palette completion also ends that physical line; pixels
 *   begin on the next same-image ToolType.
 *   bpp = max(1, ceil(log2(num_colors))).
 *
 * Decoding never writes the ToolType table.
 */

#include "icon_newicons.h"

#include <platform/platform.h>
#include <string.h>

#define NI_MAX_WIDTH     93U
#define NI_MAX_HEIGHT    93U
#define NI_MAX_COLORS    257U   /* 1..256 canonical; 257 is compatibility */
#define NI_ASCII_BASE    0x21U
#define NI_LINE_PREFIX   4UL    /* "IM1=" / "IM2=" */

static const char k_ni_marker[] =
    "*** DON'T EDIT THE FOLLOWING LINES!! ***";

typedef struct NiBits
{
    const UBYTE *p;
    ULONG left;
    ULONG acc;
    int nbits;
    ULONG zero_groups;  /* pending zero 7-bit groups, AFTER acc, source order */
    BOOL malformed;
} NiBits;

static BOOL mul_ok(ULONG a, ULONG b, ULONG *out)
{
    if (a != 0 && b > (0xFFFFFFFFUL / a))
        return FALSE;
    *out = a * b;
    return TRUE;
}

static void ni_reset_bits(NiBits *b)
{
    b->acc = 0;
    b->nbits = 0;
    b->zero_groups = 0;
}

static void ni_attach_line(NiBits *b, const UBYTE *p, ULONG n)
{
    /* Physical ToolType boundary: discard residual pad bits and RLE state. */
    b->p = p;
    b->left = n;
    ni_reset_bits(b);
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
        /* N zero-valued 7-bit groups, fed later through the same acc. */
        b->zero_groups += (ULONG)(c - 0xD0U);
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
    b->malformed = TRUE;
    return FALSE;
}

static BOOL ni_ensure(NiBits *b, int need)
{
    while (b->nbits < need)
    {
        if (b->zero_groups > 0)
        {
            /* RLE zeros follow bits already in acc (source order). */
            b->acc <<= 7;
            b->nbits += 7;
            b->zero_groups--;
            continue;
        }
        if (!ni_load_group(b))
            return FALSE;
    }
    return TRUE;
}

static BOOL ni_get_bits(NiBits *b, int nbits, UWORD *out)
{
    ULONG mask;
    ULONG val;

    if (nbits <= 0 || nbits > 9)
        return FALSE;
    if (!ni_ensure(b, nbits))
        return FALSE;

    b->nbits -= nbits;
    mask = (1UL << nbits) - 1UL;
    val = (b->acc >> b->nbits) & mask;
    if (b->nbits <= 0)
        b->acc = 0;
    else
        b->acc &= (1UL << b->nbits) - 1UL;

    *out = (UWORD)val;
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

/* bpp = max(1, ceil(log2(ncolors))); 257 colours => 9. */
static UBYTE colour_bpp(UWORD ncolors)
{
    UWORD n = 1;
    UBYTE bpp = 0;

    while (n < ncolors && bpp < 9)
    {
        n = (UWORD)(n << 1);
        bpp++;
    }
    if (bpp < 1)
        bpp = 1;
    return bpp;
}

typedef struct NiLine
{
    const UBYTE *payload;   /* bytes after IM1=/IM2= */
    ULONG length;
} NiLine;

static iTidy_IconError collect_prefix_run(const iTidy_IconFile *file,
                                          const char *prefix,
                                          ULONG from,
                                          NiLine *lines, ULONG max_lines,
                                          ULONG *out_count,
                                          ULONG *out_next)
{
    ULONG i;
    ULONG n = 0;
    BOOL started = FALSE;

    for (i = from; i < file->tooltypes_count; i++)
    {
        const UBYTE *text;
        ULONG len;

        if (!icon_file_tooltype(file, i, &text, &len))
            return ITIDY_ICON_ERR_BAD_TEXT;
        if (!starts_with(text, len, prefix))
        {
            if (started)
                break;
            continue;
        }
        if (len < NI_LINE_PREFIX)
            return ITIDY_ICON_ERR_TRUNCATED;
        if (n >= max_lines)
            return ITIDY_ICON_ERR_OVERFLOW;
        lines[n].payload = text + NI_LINE_PREFIX;
        lines[n].length = len - NI_LINE_PREFIX;
        n++;
        started = TRUE;
    }
    *out_count = n;
    *out_next = i;
    return ITIDY_ICON_OK;
}

/*
 * Marker-aware collection. The ToolType array is never modified.
 * If the canonical NewIcons marker is present, only IM1=/IM2= records
 * after it are used. Continuation runs must be physically contiguous;
 * a foreign ToolType ends the current image run. IM2 may start after
 * scanning forward from the end of the IM1 run.
 * If the marker is absent, collection starts at the first IM1=/IM2=.
 */
static iTidy_IconError collect_lines(const iTidy_IconFile *file,
                                     NiLine *im1, NiLine *im2,
                                     ULONG cap,
                                     ULONG *out_n1, ULONG *out_n2)
{
    ULONG i;
    ULONG start = 0;
    ULONG next = 0;
    iTidy_IconError err;

    for (i = 0; i < file->tooltypes_count; i++)
    {
        const UBYTE *text;
        ULONG len;

        if (!icon_file_tooltype(file, i, &text, &len))
            return ITIDY_ICON_ERR_BAD_TEXT;
        if (tooltype_equals(text, len, k_ni_marker))
        {
            start = i + 1UL;
            break;
        }
    }

    err = collect_prefix_run(file, "IM1=", start, im1, cap, out_n1, &next);
    if (err != ITIDY_ICON_OK)
        return err;
    return collect_prefix_run(file, "IM2=", next, im2, cap, out_n2, &next);
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
    BOOL phase_pixels;
    iTidy_IconError fail = ITIDY_ICON_ERR_TRUNCATED;

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
    phase_pixels = FALSE;
    memset(&bits, 0, sizeof(bits));

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

        if (!phase_pixels)
        {
            while (pal_got < pal_bytes)
            {
                UWORD v;
                ULONG ci;
                ULONG ch;

                if (!ni_get_bits(&bits, 8, &v))
                    break;
                ci = pal_got / 3UL;
                ch = pal_got % 3UL;
                if (ch == 0)
                    palette[ci].r = (UBYTE)v;
                else if (ch == 1)
                    palette[ci].g = (UBYTE)v;
                else
                    palette[ci].b = (UBYTE)v;
                pal_got++;
            }
            if (bits.malformed)
            {
                fail = ITIDY_ICON_ERR_BAD_TEXT;
                break;
            }
            if (pal_got < pal_bytes)
                continue;

            /* Palette finished on this physical ToolType. Remaining
             * characters/bits are padding. Pixels start on the next
             * same-image ToolType with a clean accumulator. */
            phase_pixels = TRUE;
            continue;
        }

        while (pix_got < pix_count)
        {
            UWORD v;

            if (!ni_get_bits(&bits, (int)bpp, &v))
                break;
            if (v >= ncolors)
            {
                fail = ITIDY_ICON_ERR_BAD_COUNT;
                pal_got = pal_bytes;
                pix_got = 0;
                line_i = nlines;
                break;
            }
            if (v > 255U)
            {
                /* 257-entry palette referenced index 256. */
                fail = ITIDY_ICON_ERR_UNSUPPORTED;
                pal_got = pal_bytes;
                pix_got = 0;
                line_i = nlines;
                break;
            }
            pixels[pix_got++] = (UBYTE)v;
        }
        if (bits.malformed)
        {
            fail = ITIDY_ICON_ERR_BAD_TEXT;
            break;
        }
        if (pix_got >= pix_count)
            break;
    }

    if (pal_got < pal_bytes || pix_got < pix_count)
    {
        whd_free(pixels);
        whd_free(palette);
        return fail;
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

    err = collect_lines(file, im1, im2, cap, &n1, &n2);

    if (err == ITIDY_ICON_OK && n1 == 0)
        err = ITIDY_ICON_ERR_NO_DATA;

    if (err == ITIDY_ICON_OK)
        err = decode_image_set(im1, n1, &out->normal);

    if (err == ITIDY_ICON_OK && n2 > 0)
    {
        err = decode_image_set(im2, n2, &out->selected);
        if (err == ITIDY_ICON_OK)
        {
            if (out->selected.width != out->normal.width ||
                out->selected.height != out->normal.height)
            {
                err = ITIDY_ICON_ERR_BAD_DIMENSION;
            }
            else
                out->has_selected = TRUE;
        }
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
