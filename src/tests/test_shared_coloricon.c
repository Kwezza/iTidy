/*
 * test_shared_coloricon.c - Host tests for the ColorIcon / GlowIcon decoder
 *
 * Compile/run via: make test-coloricon
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <icon/icon_file.h>
#include <icon/icon_coloricon.h>

static int g_failures = 0;

static void fail(const char *name, const char *detail)
{
    printf("FAIL: %s: %s\n", name, detail);
    g_failures++;
}

static void pass(const char *name)
{
    printf("PASS: %s\n", name);
}

static void expect_eq_i(const char *name, int got, int want)
{
    char buf[128];
    if (got != want)
    {
        sprintf(buf, "got %d want %d", got, want);
        fail(name, buf);
    }
    else
        pass(name);
}

static void expect_eq_u(const char *name, unsigned got, unsigned want)
{
    char buf[128];
    if (got != want)
    {
        sprintf(buf, "got %u want %u", got, want);
        fail(name, buf);
    }
    else
        pass(name);
}

static void expect_true(const char *name, int cond)
{
    if (!cond)
        fail(name, "expected TRUE");
    else
        pass(name);
}

static void expect_false(const char *name, int cond)
{
    if (cond)
        fail(name, "expected FALSE");
    else
        pass(name);
}

static void expect_err(const char *name, iTidy_IconError got, iTidy_IconError want)
{
    char buf[160];
    if (got != want)
    {
        sprintf(buf, "got %d (%s) want %d (%s)",
                got, icon_error_string(got), want, icon_error_string(want));
        fail(name, buf);
    }
    else
        pass(name);
}

static void expect_mem(const char *name, const void *got, const void *want, unsigned n)
{
    if (memcmp(got, want, n) != 0)
        fail(name, "buffer mismatch");
    else
        pass(name);
}

/*========================================================================*/
/* Buffer helpers                                                         */
/*========================================================================*/

typedef struct Buf
{
    UBYTE *data;
    ULONG len;
    ULONG cap;
} Buf;

static void buf_init(Buf *b)
{
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void buf_free(Buf *b)
{
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void buf_need(Buf *b, ULONG extra)
{
    ULONG need = b->len + extra;
    ULONG cap;
    UBYTE *p;

    if (need <= b->cap)
        return;
    cap = b->cap ? b->cap : 64UL;
    while (cap < need)
        cap *= 2UL;
    p = (UBYTE *)realloc(b->data, cap);
    if (p == NULL)
    {
        fprintf(stderr, "realloc failed\n");
        exit(2);
    }
    b->data = p;
    b->cap = cap;
}

static void buf_append(Buf *b, const void *src, ULONG n)
{
    buf_need(b, n);
    memcpy(b->data + b->len, src, n);
    b->len += n;
}

static void buf_u8(Buf *b, unsigned v)
{
    UBYTE x = (UBYTE)v;
    buf_append(b, &x, 1);
}

static void buf_u16(Buf *b, unsigned v)
{
    UBYTE x[2];
    x[0] = (UBYTE)((v >> 8) & 0xFF);
    x[1] = (UBYTE)(v & 0xFF);
    buf_append(b, x, 2);
}

static void buf_u32(Buf *b, unsigned v)
{
    UBYTE x[4];
    x[0] = (UBYTE)((v >> 24) & 0xFF);
    x[1] = (UBYTE)((v >> 16) & 0xFF);
    x[2] = (UBYTE)((v >> 8) & 0xFF);
    x[3] = (UBYTE)(v & 0xFF);
    buf_append(b, x, 4);
}

static void buf_zeros(Buf *b, ULONG n)
{
    buf_need(b, n);
    memset(b->data + b->len, 0, n);
    b->len += n;
}

static void buf_patch_u32(Buf *b, ULONG offset, unsigned v)
{
    if (offset + 4 > b->len)
        return;
    b->data[offset]     = (UBYTE)((v >> 24) & 0xFF);
    b->data[offset + 1] = (UBYTE)((v >> 16) & 0xFF);
    b->data[offset + 2] = (UBYTE)((v >> 8) & 0xFF);
    b->data[offset + 3] = (UBYTE)(v & 0xFF);
}

static ULONG planar_size(unsigned w, unsigned h, unsigned depth)
{
    unsigned bpr = ((w + 15) / 16) * 2;
    return (ULONG)bpr * (ULONG)h * (ULONG)depth;
}

static void append_diskobject(Buf *b, int gadget_w, int gadget_h)
{
    ULONG start = b->len;
    buf_zeros(b, ITIDY_ICON_DISKOBJECT_SIZE);
    b->data[start + 0] = 0xE3;
    b->data[start + 1] = 0x10;
    b->data[start + 2] = 0x00;
    b->data[start + 3] = 0x01;
    b->data[start + 0x0C] = (UBYTE)((gadget_w >> 8) & 0xFF);
    b->data[start + 0x0D] = (UBYTE)(gadget_w & 0xFF);
    b->data[start + 0x0E] = (UBYTE)((gadget_h >> 8) & 0xFF);
    b->data[start + 0x0F] = (UBYTE)(gadget_h & 0xFF);
    b->data[start + 0x10] = 0x00;
    b->data[start + 0x11] = 0x04;
    buf_patch_u32(b, start + 0x16, 0x400C6718UL);
    b->data[start + 0x30] = ITIDY_ICON_WB_TOOL;
    buf_patch_u32(b, start + 0x3A, 0x80000000UL);
    buf_patch_u32(b, start + 0x3E, 0x80000000UL);
    buf_patch_u32(b, start + 0x4A, 4096UL);
}

static void append_image(Buf *b, int w, int h, int depth)
{
    ULONG sz = planar_size((unsigned)w, (unsigned)h, (unsigned)depth);
    buf_u16(b, 0);
    buf_u16(b, 0);
    buf_u16(b, (unsigned)w);
    buf_u16(b, (unsigned)h);
    buf_u16(b, (unsigned)depth);
    buf_u32(b, 0x4046DC28UL);
    buf_u8(b, (unsigned)((1 << depth) - 1));
    buf_u8(b, 0);
    buf_u32(b, 0);
    buf_zeros(b, sz);
}

static void append_chunk(Buf *b, const char *id, const UBYTE *payload, ULONG n)
{
    buf_append(b, id, 4);
    buf_u32(b, (unsigned)n);
    if (n && payload)
        buf_append(b, payload, n);
    if (n & 1UL)
        buf_u8(b, 0);
}

/*========================================================================*/
/* Bitstream RLE encoder (same rules as the decoder)                      */
/*========================================================================*/

typedef struct BitWriter
{
    Buf *buf;
    UBYTE cur;
    int filled;
} BitWriter;

static void bw_init(BitWriter *bw, Buf *buf)
{
    bw->buf = buf;
    bw->cur = 0;
    bw->filled = 0;
}

static void bw_bit(BitWriter *bw, unsigned bit)
{
    bw->cur = (UBYTE)((bw->cur << 1) | (bit & 1U));
    bw->filled++;
    if (bw->filled == 8)
    {
        buf_u8(bw->buf, bw->cur);
        bw->cur = 0;
        bw->filled = 0;
    }
}

static void bw_bits(BitWriter *bw, unsigned val, int nbits)
{
    int i;
    for (i = nbits - 1; i >= 0; i--)
        bw_bit(bw, (val >> i) & 1U);
}

static void bw_flush(BitWriter *bw)
{
    if (bw->filled == 0)
        return;
    bw->cur = (UBYTE)(bw->cur << (8 - bw->filled));
    buf_u8(bw->buf, bw->cur);
    bw->cur = 0;
    bw->filled = 0;
}

static void rle_literals(BitWriter *bw, const UBYTE *samples, unsigned n,
                         int depth)
{
    unsigned i;
    while (n > 0)
    {
        unsigned chunk = n > 128U ? 128U : n;
        bw_bits(bw, chunk - 1U, 8);
        for (i = 0; i < chunk; i++)
            bw_bits(bw, samples[i], depth);
        samples += chunk;
        n -= chunk;
    }
}

static void rle_run(BitWriter *bw, UBYTE sample, unsigned n, int depth)
{
    while (n > 0)
    {
        unsigned chunk = n > 128U ? 128U : n;
        bw_bits(bw, 257U - chunk, 8);
        bw_bits(bw, sample, depth);
        n -= chunk;
    }
}

static void rle_nop(BitWriter *bw)
{
    bw_bits(bw, 0x80, 8);
}

/*========================================================================*/
/* FORM ICON builders                                                     */
/*========================================================================*/

typedef struct ImagSpec
{
    UBYTE transparent;
    UBYTE flags;          /* bit0 trans, bit1 palette */
    UBYTE image_format;   /* 0 raw, 1 RLE */
    UBYTE pal_format;
    UBYTE depth;
    const UBYTE *pixels;
    ULONG pixel_count;
    const UBYTE *pixel_stored; /* if non-NULL, use instead of pixels/RLE */
    ULONG pixel_stored_len;
    const iTidy_RGB8 *palette;
    UWORD pal_count;
    const UBYTE *pal_stored;
    ULONG pal_stored_len;
} ImagSpec;

static void build_imag_payload(Buf *payload, const ImagSpec *spec)
{
    Buf img_data;
    Buf pal_data;
    ULONG img_len;
    ULONG pal_len;

    buf_init(&img_data);
    buf_init(&pal_data);

    if (spec->pixel_stored != NULL)
    {
        buf_append(&img_data, spec->pixel_stored, spec->pixel_stored_len);
    }
    else if (spec->image_format == 0)
    {
        buf_append(&img_data, spec->pixels, spec->pixel_count);
    }
    else
    {
        BitWriter bw;
        bw_init(&bw, &img_data);
        rle_literals(&bw, spec->pixels, (unsigned)spec->pixel_count, spec->depth);
        bw_flush(&bw);
    }

    if (spec->flags & 0x02)
    {
        if (spec->pal_stored != NULL)
        {
            buf_append(&pal_data, spec->pal_stored, spec->pal_stored_len);
        }
        else if (spec->pal_format == 0)
        {
            UWORD i;
            for (i = 0; i < spec->pal_count; i++)
            {
                buf_u8(&pal_data, spec->palette[i].r);
                buf_u8(&pal_data, spec->palette[i].g);
                buf_u8(&pal_data, spec->palette[i].b);
            }
        }
        else
        {
            BitWriter bw;
            UBYTE rgb[256 * 3];
            UWORD i;
            ULONG n = (ULONG)spec->pal_count * 3UL;
            for (i = 0; i < spec->pal_count; i++)
            {
                rgb[i * 3U]     = spec->palette[i].r;
                rgb[i * 3U + 1] = spec->palette[i].g;
                rgb[i * 3U + 2] = spec->palette[i].b;
            }
            bw_init(&bw, &pal_data);
            rle_literals(&bw, rgb, (unsigned)n, 8);
            bw_flush(&bw);
        }
    }

    img_len = img_data.len ? img_data.len : 1UL;
    pal_len = (spec->flags & 0x02) ? (pal_data.len ? pal_data.len : 1UL) : 1UL;

    buf_u8(payload, spec->transparent);
    buf_u8(payload, (unsigned)(spec->pal_count ? spec->pal_count - 1U : 0U));
    buf_u8(payload, spec->flags);
    buf_u8(payload, spec->image_format);
    buf_u8(payload, spec->pal_format);
    buf_u8(payload, spec->depth);
    buf_u16(payload, (unsigned)(img_len - 1UL));
    buf_u16(payload, (unsigned)(pal_len - 1UL));
    if (img_data.len)
        buf_append(payload, img_data.data, img_data.len);
    else
        buf_u8(payload, 0);
    if (spec->flags & 0x02)
    {
        if (pal_data.len)
            buf_append(payload, pal_data.data, pal_data.len);
        else
            buf_u8(payload, 0);
    }

    buf_free(&img_data);
    buf_free(&pal_data);
}

static void append_form_from_inner(Buf *b, Buf *inner)
{
    buf_append(b, "FORM", 4);
    buf_u32(b, (unsigned)inner->len);
    buf_append(b, inner->data, inner->len);
}

static void append_face(Buf *inner, unsigned w, unsigned h, unsigned flags,
                        unsigned max_pal_m1)
{
    UBYTE face[6];
    face[0] = (UBYTE)((w - 1U) & 0xFF);
    face[1] = (UBYTE)((h - 1U) & 0xFF);
    face[2] = (UBYTE)flags;
    face[3] = 0x11;
    face[4] = (UBYTE)((max_pal_m1 >> 8) & 0xFF);
    face[5] = (UBYTE)(max_pal_m1 & 0xFF);
    append_chunk(inner, "FACE", face, 6);
}

static void wrap_icon(Buf *out, unsigned w, unsigned h, int has_classic)
{
    buf_init(out);
    append_diskobject(out, (int)w, (int)h);
    if (has_classic)
        append_image(out, (int)w, (int)h, 1);
}

/*========================================================================*/
/* Tests                                                                  */
/*========================================================================*/

static iTidy_RGB8 pal4[4] = {
    { 0, 0, 0 },
    { 255, 0, 0 },
    { 0, 255, 0 },
    { 0, 0, 255 }
};

static void test_raw_image_and_palette(void)
{
    Buf file, inner, imag;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE pixels[4] = { 0, 1, 2, 3 };
    ImagSpec spec;
    UBYTE *copy;

    memset(&spec, 0, sizeof(spec));
    spec.transparent = 0;
    spec.flags = 0x03;
    spec.image_format = 0;
    spec.pal_format = 0;
    spec.depth = 8;
    spec.pixels = pixels;
    spec.pixel_count = 4;
    spec.palette = pal4;
    spec.pal_count = 4;

    buf_init(&imag);
    build_imag_payload(&imag, &spec);

    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_face(&inner, 2, 2, 0, 3);
    append_chunk(&inner, "IMAG", imag.data, imag.len);

    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);

    copy = (UBYTE *)malloc(file.len);
    memcpy(copy, file.data, file.len);

    expect_err("raw parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("raw decode", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_OK);
    expect_eq_u("raw src", (unsigned)dec.source_format, (unsigned)ITIDY_ICON_SRC_COLORICON);
    expect_eq_u("raw w", dec.normal.width, 2);
    expect_eq_u("raw h", dec.normal.height, 2);
    expect_eq_u("raw pal count", dec.normal.palette_count, 4);
    expect_eq_i("raw trans", (int)dec.normal.transparent_index, 0);
    expect_false("raw no selected", dec.has_selected);
    expect_mem("raw pixels", dec.normal.pixels, pixels, 4);
    expect_eq_u("raw pal1 r", dec.normal.palette[1].r, 255);
    expect_eq_u("raw pal2 g", dec.normal.palette[2].g, 255);
    expect_true("raw buffer unchanged", memcmp(copy, file.data, file.len) == 0);
    expect_false("raw not frameless", dec.frameless);

    icon_decoded_free(&dec);
    free(copy);
    buf_free(&imag);
    buf_free(&inner);
    buf_free(&file);
}

static void test_rle_image(void)
{
    Buf file, inner, imag;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE pixels[4] = { 2, 2, 2, 2 };
    ImagSpec spec;
    Buf stored;
    BitWriter bw;

    buf_init(&stored);
    bw_init(&bw, &stored);
    rle_nop(&bw);
    rle_run(&bw, 2, 4, 8);
    bw_flush(&bw);

    memset(&spec, 0, sizeof(spec));
    spec.flags = 0x02;
    spec.image_format = 1;
    spec.pal_format = 0;
    spec.depth = 8;
    spec.pixel_stored = stored.data;
    spec.pixel_stored_len = stored.len;
    spec.palette = pal4;
    spec.pal_count = 4;

    buf_init(&imag);
    build_imag_payload(&imag, &spec);
    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_face(&inner, 2, 2, 0, 3);
    append_chunk(&inner, "IMAG", imag.data, imag.len);
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);

    expect_err("rle img parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("rle img decode", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_OK);
    expect_mem("rle img pixels", dec.normal.pixels, pixels, 4);
    expect_eq_i("rle img no trans", (int)dec.normal.transparent_index, -1);

    icon_decoded_free(&dec);
    buf_free(&stored);
    buf_free(&imag);
    buf_free(&inner);
    buf_free(&file);
}

static void test_rle_palette(void)
{
    Buf file, inner, imag;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE pixels[4] = { 0, 1, 2, 3 };
    ImagSpec spec;

    memset(&spec, 0, sizeof(spec));
    spec.flags = 0x02;
    spec.image_format = 0;
    spec.pal_format = 1;
    spec.depth = 8;
    spec.pixels = pixels;
    spec.pixel_count = 4;
    spec.palette = pal4;
    spec.pal_count = 4;

    buf_init(&imag);
    build_imag_payload(&imag, &spec);
    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_face(&inner, 2, 2, 0, 3);
    append_chunk(&inner, "IMAG", imag.data, imag.len);
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);

    expect_err("rle pal parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("rle pal decode", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_OK);
    expect_eq_u("rle pal count", dec.normal.palette_count, 4);
    expect_eq_u("rle pal blue", dec.normal.palette[3].b, 255);
    expect_mem("rle pal pixels", dec.normal.pixels, pixels, 4);

    icon_decoded_free(&dec);
    buf_free(&imag);
    buf_free(&inner);
    buf_free(&file);
}

static void test_both_compressed(void)
{
    Buf file, inner, imag;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE pixels[4] = { 1, 1, 0, 0 };
    ImagSpec spec;

    memset(&spec, 0, sizeof(spec));
    spec.flags = 0x02;
    spec.image_format = 1;
    spec.pal_format = 1;
    spec.depth = 2; /* packed 2-bit samples in the image RLE */
    spec.pixels = pixels;
    spec.pixel_count = 4;
    spec.palette = pal4;
    spec.pal_count = 4;

    buf_init(&imag);
    build_imag_payload(&imag, &spec);
    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_face(&inner, 2, 2, 0, 3);
    append_chunk(&inner, "IMAG", imag.data, imag.len);
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);

    expect_err("both parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("both decode", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_OK);
    expect_mem("both pixels", dec.normal.pixels, pixels, 4);
    expect_eq_u("both pal red", dec.normal.palette[1].r, 255);

    icon_decoded_free(&dec);
    buf_free(&imag);
    buf_free(&inner);
    buf_free(&file);
}

static void test_selected_and_inherited_palette(void)
{
    Buf file, inner, imag1, imag2;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE pix_n[4] = { 0, 1, 2, 3 };
    UBYTE pix_s[4] = { 3, 2, 1, 0 };
    ImagSpec spec1, spec2;

    memset(&spec1, 0, sizeof(spec1));
    spec1.transparent = 0;
    spec1.flags = 0x03;
    spec1.image_format = 0;
    spec1.pal_format = 0;
    spec1.depth = 8;
    spec1.pixels = pix_n;
    spec1.pixel_count = 4;
    spec1.palette = pal4;
    spec1.pal_count = 4;

    memset(&spec2, 0, sizeof(spec2));
    spec2.flags = 0x00; /* no palette, no transparency */
    spec2.image_format = 0;
    spec2.pal_format = 0;
    spec2.depth = 8;
    spec2.pixels = pix_s;
    spec2.pixel_count = 4;
    spec2.pal_count = 4; /* stored in header only */

    buf_init(&imag1);
    build_imag_payload(&imag1, &spec1);
    buf_init(&imag2);
    build_imag_payload(&imag2, &spec2);

    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_face(&inner, 2, 2, 1, 3); /* frameless */
    append_chunk(&inner, "IMAG", imag1.data, imag1.len);
    append_chunk(&inner, "IMAG", imag2.data, imag2.len);
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);

    expect_err("sel parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("sel decode", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_OK);
    expect_true("has selected", dec.has_selected);
    expect_true("frameless", dec.frameless);
    expect_mem("sel pixels", dec.selected.pixels, pix_s, 4);
    expect_eq_u("inherited pal count", dec.selected.palette_count, 4);
    expect_eq_u("inherited pal g", dec.selected.palette[2].g, 255);
    expect_eq_i("sel no trans", (int)dec.selected.transparent_index, -1);
    expect_eq_i("norm trans", (int)dec.normal.transparent_index, 0);

    icon_decoded_free(&dec);
    buf_free(&imag1);
    buf_free(&imag2);
    buf_free(&inner);
    buf_free(&file);
}

static void test_unknown_chunk_and_reorder(void)
{
    Buf file, inner, imag;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE pixels[4] = { 0, 1, 2, 3 };
    ImagSpec spec;
    UBYTE junk[1] = { 0xAB };

    memset(&spec, 0, sizeof(spec));
    spec.flags = 0x02;
    spec.image_format = 0;
    spec.pal_format = 0;
    spec.depth = 8;
    spec.pixels = pixels;
    spec.pixel_count = 4;
    spec.palette = pal4;
    spec.pal_count = 4;

    buf_init(&imag);
    build_imag_payload(&imag, &spec);

    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_chunk(&inner, "JUNK", junk, 1); /* odd size + pad */
    append_chunk(&inner, "IMAG", imag.data, imag.len);
    append_face(&inner, 2, 2, 0, 3); /* FACE after IMAG */
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);

    expect_err("reorder parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("reorder decode", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_OK);
    expect_mem("reorder pixels", dec.normal.pixels, pixels, 4);

    icon_decoded_free(&dec);
    buf_free(&imag);
    buf_free(&inner);
    buf_free(&file);
}

static void test_glow_heuristic(void)
{
    Buf file, inner, imag;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE pixels[4] = { 0, 1, 0, 1 };
    ImagSpec spec;

    memset(&spec, 0, sizeof(spec));
    spec.flags = 0x02;
    spec.image_format = 0;
    spec.pal_format = 0;
    spec.depth = 8;
    spec.pixels = pixels;
    spec.pixel_count = 4;
    spec.palette = pal4;
    spec.pal_count = 4;

    buf_init(&imag);
    build_imag_payload(&imag, &spec);
    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_face(&inner, 2, 2, 0, 255);
    append_chunk(&inner, "IMAG", imag.data, imag.len);
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);

    expect_err("glow parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("glow decode", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_OK);
    expect_eq_u("glow src", (unsigned)dec.source_format, (unsigned)ITIDY_ICON_SRC_GLOWICON);

    icon_decoded_free(&dec);
    buf_free(&imag);
    buf_free(&inner);
    buf_free(&file);
}

static void test_malformed(void)
{
    Buf file, inner;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE face[6] = { 1, 1, 0, 0x11, 0, 3 };
    UBYTE truncated_imag[8];

    wrap_icon(&file, 16, 16, 1);
    expect_err("classic parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("no extension", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_ERR_NO_DATA);
    buf_free(&file);

    wrap_icon(&file, 16, 16, 1);
    buf_append(&file, "FORM", 4);
    buf_u32(&file, 4);
    buf_append(&file, "ARGB", 4);
    expect_err("argb parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("argb unsupported", icon_coloricon_decode(&parsed, &dec),
               ITIDY_ICON_ERR_UNSUPPORTED);
    buf_free(&file);

    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_chunk(&inner, "FACE", face, 6);
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);
    expect_err("face only parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("face only no imag", icon_coloricon_decode(&parsed, &dec),
               ITIDY_ICON_ERR_NO_DATA);
    buf_free(&inner);
    buf_free(&file);

    memset(truncated_imag, 0, sizeof(truncated_imag));
    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_chunk(&inner, "FACE", face, 6);
    append_chunk(&inner, "IMAG", truncated_imag, 8);
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);
    expect_err("short imag parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("short imag", icon_coloricon_decode(&parsed, &dec),
               ITIDY_ICON_ERR_TRUNCATED);
    buf_free(&inner);
    buf_free(&file);

    {
        Buf stored;
        BitWriter bw;
        ImagSpec spec;
        Buf imag;
        UBYTE pixels_unused[1] = { 0 };

        buf_init(&stored);
        bw_init(&bw, &stored);
        rle_run(&bw, 1, 8, 8); /* 8 samples into a 4-pixel image */
        bw_flush(&bw);

        memset(&spec, 0, sizeof(spec));
        spec.flags = 0x02;
        spec.image_format = 1;
        spec.pal_format = 0;
        spec.depth = 8;
        spec.pixel_stored = stored.data;
        spec.pixel_stored_len = stored.len;
        spec.palette = pal4;
        spec.pal_count = 4;
        spec.pixels = pixels_unused;
        spec.pixel_count = 1;

        buf_init(&imag);
        build_imag_payload(&imag, &spec);
        buf_init(&inner);
        buf_append(&inner, "ICON", 4);
        append_face(&inner, 2, 2, 0, 3);
        append_chunk(&inner, "IMAG", imag.data, imag.len);
        wrap_icon(&file, 2, 2, 1);
        append_form_from_inner(&file, &inner);

        expect_err("rle overrun parse",
                   icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
        expect_err("rle overrun", icon_coloricon_decode(&parsed, &dec),
                   ITIDY_ICON_ERR_OVERFLOW);

        buf_free(&stored);
        buf_free(&imag);
        buf_free(&inner);
        buf_free(&file);
    }

    expect_err("null out", icon_coloricon_decode(NULL, NULL), ITIDY_ICON_ERR_NULL);
}

/* Research golden vector: depth-2 literal 03 1B -> 0,1,2,3 */
static void test_rle_depth2_literal_boundary(void)
{
    Buf file, inner, imag;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE stored[2] = { 0x03, 0x1B };
    UBYTE want[4] = { 0, 1, 2, 3 };
    ImagSpec spec;

    memset(&spec, 0, sizeof(spec));
    spec.flags = 0x02;
    spec.image_format = 1;
    spec.pal_format = 0;
    spec.depth = 2;
    spec.pixel_stored = stored;
    spec.pixel_stored_len = 2;
    spec.palette = pal4;
    spec.pal_count = 4;

    buf_init(&imag);
    build_imag_payload(&imag, &spec);
    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_face(&inner, 4, 1, 0, 11); /* max RGB bytes-1 for 4 colours */
    append_chunk(&inner, "IMAG", imag.data, imag.len);
    wrap_icon(&file, 4, 1, 1);
    append_form_from_inner(&file, &inner);

    expect_err("d2 lit parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("d2 lit decode", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_OK);
    expect_eq_u("d2 lit w", dec.normal.width, 4);
    expect_eq_u("d2 lit h", dec.normal.height, 1);
    expect_mem("d2 lit pixels", dec.normal.pixels, want, 4);

    icon_decoded_free(&dec);
    buf_free(&imag);
    buf_free(&inner);
    buf_free(&file);
}

/* Research: FD 00 at depth 1 repeats value 0 four times */
static void test_rle_depth1_repeat_boundary(void)
{
    Buf file, inner, imag;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE stored[2] = { 0xFD, 0x00 };
    UBYTE want[4] = { 0, 0, 0, 0 };
    iTidy_RGB8 pal2[2] = { { 0, 0, 0 }, { 255, 255, 255 } };
    ImagSpec spec;

    memset(&spec, 0, sizeof(spec));
    spec.flags = 0x02;
    spec.image_format = 1;
    spec.pal_format = 0;
    spec.depth = 1;
    spec.pixel_stored = stored;
    spec.pixel_stored_len = 2;
    spec.palette = pal2;
    spec.pal_count = 2;

    buf_init(&imag);
    build_imag_payload(&imag, &spec);
    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_face(&inner, 4, 1, 0, 5);
    append_chunk(&inner, "IMAG", imag.data, imag.len);
    wrap_icon(&file, 4, 1, 1);
    append_form_from_inner(&file, &inner);

    expect_err("d1 rep parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("d1 rep decode", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_OK);
    expect_mem("d1 rep pixels", dec.normal.pixels, want, 4);

    icon_decoded_free(&dec);
    buf_free(&imag);
    buf_free(&inner);
    buf_free(&file);
}

/* Uncompressed depth < 8 is still one byte per pixel */
static void test_raw_sub8_depth(void)
{
    Buf file, inner, imag;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE pixels[4] = { 0, 1, 1, 0 };
    iTidy_RGB8 pal2[2] = { { 0, 0, 0 }, { 255, 255, 255 } };
    ImagSpec spec;

    memset(&spec, 0, sizeof(spec));
    spec.flags = 0x02;
    spec.image_format = 0;
    spec.pal_format = 0;
    spec.depth = 1;
    spec.pixels = pixels;
    spec.pixel_count = 4;
    spec.palette = pal2;
    spec.pal_count = 2;

    buf_init(&imag);
    build_imag_payload(&imag, &spec);
    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_face(&inner, 2, 2, 0, 5);
    append_chunk(&inner, "IMAG", imag.data, imag.len);
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);

    expect_err("raw d1 parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("raw d1 decode", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_OK);
    expect_mem("raw d1 pixels", dec.normal.pixels, pixels, 4);
    expect_eq_u("raw d1 pal", dec.normal.palette_count, 2);

    icon_decoded_free(&dec);
    buf_free(&imag);
    buf_free(&inner);
    buf_free(&file);
}

/*
 * Critical regression: palette size is num_colors*3, not (1<<depth)*3.
 * depth=5 with 17 colours => 51 RGB bytes.
 */
static void test_palette_17_at_depth5(void)
{
    Buf file, inner, imag;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    iTidy_RGB8 pal17[17];
    UBYTE pixels[4];
    ImagSpec spec;
    unsigned i;

    for (i = 0; i < 17; i++)
    {
        pal17[i].r = (UBYTE)i;
        pal17[i].g = (UBYTE)(255 - i);
        pal17[i].b = (UBYTE)(i * 3);
    }
    pixels[0] = 0;
    pixels[1] = 16;
    pixels[2] = 8;
    pixels[3] = 1;

    memset(&spec, 0, sizeof(spec));
    spec.transparent = 16;
    spec.flags = 0x03;
    spec.image_format = 0;
    spec.pal_format = 0;
    spec.depth = 5;
    spec.pixels = pixels;
    spec.pixel_count = 4;
    spec.palette = pal17;
    spec.pal_count = 17;

    buf_init(&imag);
    build_imag_payload(&imag, &spec);
    /* Raw palette must be exactly 51 bytes for 17 colours. */
    expect_eq_u("17c imag payload has 51 rgb",
                (unsigned)(imag.len - 10UL - 4UL), 51U);

    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_face(&inner, 2, 2, 0, 50); /* 17*3 - 1 */
    append_chunk(&inner, "IMAG", imag.data, imag.len);
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);

    expect_err("17c parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("17c decode", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_OK);
    expect_eq_u("17c pal count", dec.normal.palette_count, 17);
    expect_eq_u("17c pal16 r", dec.normal.palette[16].r, 16);
    expect_eq_i("17c trans non-zero", (int)dec.normal.transparent_index, 16);
    expect_mem("17c pixels", dec.normal.pixels, pixels, 4);

    icon_decoded_free(&dec);
    buf_free(&imag);
    buf_free(&inner);
    buf_free(&file);
}

static void test_reject_bad_index_and_formats(void)
{
    Buf file, inner, imag;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE bad_pix[4] = { 0, 1, 2, 4 }; /* index 4 with only 4 colours */
    ImagSpec spec;

    memset(&spec, 0, sizeof(spec));
    spec.flags = 0x02;
    spec.image_format = 0;
    spec.pal_format = 0;
    spec.depth = 8;
    spec.pixels = bad_pix;
    spec.pixel_count = 4;
    spec.palette = pal4;
    spec.pal_count = 4;

    buf_init(&imag);
    build_imag_payload(&imag, &spec);
    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_face(&inner, 2, 2, 0, 11);
    append_chunk(&inner, "IMAG", imag.data, imag.len);
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);

    expect_err("bad idx parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("bad idx", icon_coloricon_decode(&parsed, &dec), ITIDY_ICON_ERR_BAD_COUNT);
    buf_free(&imag);
    buf_free(&inner);
    buf_free(&file);

    {
        Buf payload;
        buf_init(&payload);
        buf_u8(&payload, 0);
        buf_u8(&payload, 1);
        buf_u8(&payload, 0x02);
        buf_u8(&payload, 2); /* unsupported image format */
        buf_u8(&payload, 0);
        buf_u8(&payload, 8);
        buf_u16(&payload, 3);
        buf_u16(&payload, 5);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);

        buf_init(&inner);
        buf_append(&inner, "ICON", 4);
        append_face(&inner, 2, 2, 0, 5);
        append_chunk(&inner, "IMAG", payload.data, payload.len);
        wrap_icon(&file, 2, 2, 1);
        append_form_from_inner(&file, &inner);

        expect_err("bad imgfmt parse",
                   icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
        expect_err("bad imgfmt", icon_coloricon_decode(&parsed, &dec),
                   ITIDY_ICON_ERR_UNSUPPORTED);
        buf_free(&payload);
        buf_free(&inner);
        buf_free(&file);
    }

    {
        Buf payload;
        buf_init(&payload);
        buf_u8(&payload, 0);
        buf_u8(&payload, 1);
        buf_u8(&payload, 0x02);
        buf_u8(&payload, 0);
        buf_u8(&payload, 2); /* unsupported palette format */
        buf_u8(&payload, 8);
        buf_u16(&payload, 3);
        buf_u16(&payload, 5);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);

        buf_init(&inner);
        buf_append(&inner, "ICON", 4);
        append_face(&inner, 2, 2, 0, 5);
        append_chunk(&inner, "IMAG", payload.data, payload.len);
        wrap_icon(&file, 2, 2, 1);
        append_form_from_inner(&file, &inner);

        expect_err("bad palfmt parse",
                   icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
        expect_err("bad palfmt", icon_coloricon_decode(&parsed, &dec),
                   ITIDY_ICON_ERR_UNSUPPORTED);
        buf_free(&payload);
        buf_free(&inner);
        buf_free(&file);
    }

    {
        Buf stored;
        BitWriter bw;
        ImagSpec trunc;
        UBYTE unused[1] = { 0 };

        buf_init(&stored);
        bw_init(&bw, &stored);
        bw_bits(&bw, 0x03, 8); /* ask for 4 literals */
        bw_bits(&bw, 1, 8);    /* only one sample, then EOF */
        bw_flush(&bw);

        memset(&trunc, 0, sizeof(trunc));
        trunc.flags = 0x02;
        trunc.image_format = 1;
        trunc.pal_format = 0;
        trunc.depth = 8;
        trunc.pixel_stored = stored.data;
        trunc.pixel_stored_len = stored.len;
        trunc.palette = pal4;
        trunc.pal_count = 4;
        trunc.pixels = unused;
        trunc.pixel_count = 1;

        buf_init(&imag);
        build_imag_payload(&imag, &trunc);
        buf_init(&inner);
        buf_append(&inner, "ICON", 4);
        append_face(&inner, 2, 2, 0, 11);
        append_chunk(&inner, "IMAG", imag.data, imag.len);
        wrap_icon(&file, 2, 2, 1);
        append_form_from_inner(&file, &inner);

        expect_err("trunc rle parse",
                   icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
        expect_err("trunc rle", icon_coloricon_decode(&parsed, &dec),
                   ITIDY_ICON_ERR_TRUNCATED);
        buf_free(&stored);
        buf_free(&imag);
        buf_free(&inner);
        buf_free(&file);
    }

    {
        /* num_colors > 1<<depth */
        Buf payload;
        buf_init(&payload);
        buf_u8(&payload, 0);
        buf_u8(&payload, 16); /* 17 colours */
        buf_u8(&payload, 0x02);
        buf_u8(&payload, 0);
        buf_u8(&payload, 0);
        buf_u8(&payload, 4); /* depth 4 => max 16 colours */
        buf_u16(&payload, 3);
        buf_u16(&payload, 50);
        buf_zeros(&payload, 4 + 51);

        buf_init(&inner);
        buf_append(&inner, "ICON", 4);
        append_face(&inner, 2, 2, 0, 50);
        append_chunk(&inner, "IMAG", payload.data, payload.len);
        wrap_icon(&file, 2, 2, 1);
        append_form_from_inner(&file, &inner);

        expect_err("ncol>depth parse",
                   icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
        expect_err("ncol>depth", icon_coloricon_decode(&parsed, &dec),
                   ITIDY_ICON_ERR_BAD_COUNT);
        buf_free(&payload);
        buf_free(&inner);
        buf_free(&file);
    }
}

static void test_form_icon_argb_only_not_coloricon(void)
{
    Buf file, inner;
    iTidy_IconFile parsed;
    iTidy_DecodedIcon dec;
    UBYTE face[6] = { 1, 1, 0, 0x11, 0, 3 };
    UBYTE argb[4] = { 0, 0, 0, 0 };

    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    append_chunk(&inner, "FACE", face, 6);
    append_chunk(&inner, "ARGB", argb, 4);
    wrap_icon(&file, 2, 2, 1);
    append_form_from_inner(&file, &inner);

    expect_err("argb-only parse", icon_file_parse(file.data, file.len, &parsed), ITIDY_ICON_OK);
    expect_err("argb-only decode", icon_coloricon_decode(&parsed, &dec),
               ITIDY_ICON_ERR_UNSUPPORTED);

    buf_free(&inner);
    buf_free(&file);
}

int main(void)
{
    test_raw_image_and_palette();
    test_rle_image();
    test_rle_palette();
    test_both_compressed();
    test_selected_and_inherited_palette();
    test_unknown_chunk_and_reorder();
    test_glow_heuristic();
    test_malformed();
    test_rle_depth2_literal_boundary();
    test_rle_depth1_repeat_boundary();
    test_raw_sub8_depth();
    test_palette_17_at_depth5();
    test_reject_bad_index_and_formats();
    test_form_icon_argb_only_not_coloricon();

    if (g_failures)
    {
        printf("\n%d test(s) failed\n", g_failures);
        return 1;
    }
    printf("\nAll shared ColorIcon tests passed.\n");
    return 0;
}
