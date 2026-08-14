/*
 * test_shared_decode.c - Host tests for NewIcons, classic planar, unified decode
 *
 * Compile/run via: make test-decode
 * Also writes a reusable corpus under tests/icons/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define mk_dir(p) _mkdir(p)
#else
#include <sys/stat.h>
#define mk_dir(p) mkdir((p), 0755)
#endif

#include <icon/icon_decode.h>

static int g_failures = 0;

static void fail(const char *name, const char *detail)
{
    printf("FAIL: %s: %s\n", name, detail);
    fflush(stdout);
    g_failures++;
}

static void pass(const char *name)
{
    printf("PASS: %s\n", name);
    fflush(stdout);
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
/* CRC32 (IEEE)                                                           */
/*========================================================================*/

static ULONG crc32_buf(const UBYTE *data, ULONG n)
{
    ULONG crc = 0xFFFFFFFFUL;
    ULONG i;
    int b;
    for (i = 0; i < n; i++)
    {
        crc ^= data[i];
        for (b = 0; b < 8; b++)
        {
            if (crc & 1UL)
                crc = (crc >> 1) ^ 0xEDB88320UL;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFFUL;
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

static void append_diskobject(Buf *b,
                              int has_render, int has_select, int has_tooltypes,
                              int gadget_w, int gadget_h)
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
    b->data[start + 0x11] = (UBYTE)(has_select ? 0x06 : 0x04);
    buf_patch_u32(b, start + 0x16, has_render ? 0x400C6718UL : 0UL);
    buf_patch_u32(b, start + 0x1A, has_select ? 0x400C6CA0UL : 0UL);
    b->data[start + 0x30] = ITIDY_ICON_WB_TOOL;
    buf_patch_u32(b, start + 0x36, has_tooltypes ? 1UL : 0UL);
    buf_patch_u32(b, start + 0x3A, 0x80000000UL);
    buf_patch_u32(b, start + 0x3E, 0x80000000UL);
    buf_patch_u32(b, start + 0x4A, 4096UL);
}

static void append_image_ex(Buf *b, int w, int h, int depth,
                            UBYTE pick, UBYTE onoff, const UBYTE *planar)
{
    ULONG sz = planar_size((unsigned)w, (unsigned)h, (unsigned)depth);
    buf_u16(b, 0);
    buf_u16(b, 0);
    buf_u16(b, (unsigned)w);
    buf_u16(b, (unsigned)h);
    buf_u16(b, (unsigned)depth);
    buf_u32(b, 0x4046DC28UL);
    buf_u8(b, pick);
    buf_u8(b, onoff);
    buf_u32(b, 0);
    if (planar != NULL)
        buf_append(b, planar, sz);
    else
        buf_zeros(b, sz);
}

static void append_text(Buf *b, const char *s)
{
    ULONG n = (ULONG)strlen(s) + 1UL;
    buf_u32(b, (unsigned)n);
    buf_append(b, s, n);
}

static void append_tooltypes(Buf *b, const char **strs, int n)
{
    int i;
    buf_u32(b, (unsigned)((n + 1) * 4));
    for (i = 0; i < n; i++)
        append_text(b, strs[i]);
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

static void planar_set(UBYTE *planes, int w, int h, int depth, int x, int y, int pen)
{
    int bpr = ((w + 15) / 16) * 2;
    int p;
    for (p = 0; p < depth; p++)
    {
        ULONG off = (ULONG)(p * h + y) * (ULONG)bpr + (ULONG)(x / 8);
        UBYTE bit = (UBYTE)(7 - (x & 7));
        if (pen & (1 << p))
            planes[off] = (UBYTE)(planes[off] | (UBYTE)(1U << bit));
        else
            planes[off] = (UBYTE)(planes[off] & (UBYTE)~(1U << bit));
    }
}

/*========================================================================*/
/* NewIcons encoder (matches decoder; optional short lines)               */
/*========================================================================*/

#define NI_MAX_LINES 64
#define NI_LINE_MAX  127

typedef struct NiEnc
{
    char lines[NI_MAX_LINES][NI_LINE_MAX + 8];
    int nlines;
    int max_payload;    /* after "IM1=" / "IM2=", default 123 */
    const char *prefix;
    int acc;
    int nbits;
} NiEnc;

static void ni_start(NiEnc *e, const char *prefix, int max_payload)
{
    memset(e, 0, sizeof(*e));
    e->prefix = prefix;
    e->max_payload = max_payload > 0 ? max_payload : 123;
    strcpy(e->lines[0], prefix);
    e->nlines = 1;
}

static char *ni_cur(NiEnc *e)
{
    return e->lines[e->nlines - 1];
}

static int ni_payload_len(NiEnc *e)
{
    return (int)strlen(ni_cur(e)) - (int)strlen(e->prefix);
}

static void ni_new_line(NiEnc *e)
{
    if (e->nlines >= NI_MAX_LINES)
    {
        fprintf(stderr, "NewIcons encoder: too many lines\n");
        exit(2);
    }
    e->nlines++;
    strcpy(e->lines[e->nlines - 1], e->prefix);
}

static void ni_emit7(NiEnc *e, unsigned seven)
{
    unsigned char c;
    int plen;

    if (seven <= 0x4FU)
        c = (unsigned char)(0x20U + seven);
    else
        c = (unsigned char)(0xA1U + (seven - 0x50U));

    plen = ni_payload_len(e);
    if (plen >= e->max_payload)
        ni_new_line(e);

    {
        char *cur = ni_cur(e);
        int n = (int)strlen(cur);
        cur[n] = (char)c;
        cur[n + 1] = '\0';
    }
}

static void ni_flush_bits(NiEnc *e)
{
    if (e->nbits == 0)
        return;
    e->acc <<= (7 - e->nbits);
    ni_emit7(e, (unsigned)e->acc);
    e->acc = 0;
    e->nbits = 0;
}

static void ni_put_bit(NiEnc *e, unsigned bit)
{
    e->acc = (e->acc << 1) | (int)(bit & 1U);
    e->nbits++;
    if (e->nbits == 7)
    {
        ni_emit7(e, (unsigned)e->acc);
        e->acc = 0;
        e->nbits = 0;
    }
}

static void ni_put_bits(NiEnc *e, unsigned val, int nbits)
{
    int i;
    for (i = nbits - 1; i >= 0; i--)
        ni_put_bit(e, (val >> i) & 1U);
}

static void ni_put_header(NiEnc *e, int trans, int w, int h, int ncolors)
{
    char *cur = ni_cur(e);
    int n = (int)strlen(cur);
    cur[n] = trans ? 'B' : 'C';
    cur[n + 1] = (char)(0x21 + w);
    cur[n + 2] = (char)(0x21 + h);
    cur[n + 3] = (char)(0x21 + ((ncolors >> 6) & 0x3F));
    cur[n + 4] = (char)(0x21 + (ncolors & 0x3F));
    cur[n + 5] = '\0';
}

static void ni_finish(NiEnc *e)
{
    ni_flush_bits(e);
}

static void ni_emit_rle_zeros(NiEnc *e, int groups)
{
    /* groups in 1..47, each 7 zero bits. Must be at a 7-bit boundary. */
    char *cur;
    int n;
    int plen;

    ni_flush_bits(e);
    if (groups < 1)
        groups = 1;
    if (groups > 47)
        groups = 47;
    plen = ni_payload_len(e);
    if (plen >= e->max_payload)
        ni_new_line(e);
    cur = ni_cur(e);
    n = (int)strlen(cur);
    cur[n] = (char)(0xD0 + groups);
    cur[n + 1] = '\0';
}

static int colour_bpp(int ncolors)
{
    int n = 1;
    int bpp = 0;
    while (n < ncolors && bpp < 8)
    {
        n <<= 1;
        bpp++;
    }
    return bpp;
}

static void ni_encode_image(NiEnc *e, int trans, int w, int h,
                            const iTidy_RGB8 *pal, int ncolors,
                            const UBYTE *pixels)
{
    int i;
    int bpp = colour_bpp(ncolors);
    int count = w * h;

    ni_put_header(e, trans, w, h, ncolors);
    for (i = 0; i < ncolors; i++)
    {
        ni_put_bits(e, pal[i].r, 8);
        ni_put_bits(e, pal[i].g, 8);
        ni_put_bits(e, pal[i].b, 8);
    }
    if (bpp == 0)
        return;
    for (i = 0; i < count; i++)
        ni_put_bits(e, pixels[i], bpp);
    ni_finish(e);
}

static void build_with_tooltypes(Buf *b, const char **tts, int ntt,
                                 int has_classic)
{
    buf_init(b);
    append_diskobject(b, has_classic, 0, 1, 16, 16);
    if (has_classic)
        append_image_ex(b, 16, 16, 1, 0x01, 0x00, NULL);
    append_tooltypes(b, tts, ntt);
}

/*========================================================================*/
/* ColorIcon mini-builder                                                 */
/*========================================================================*/

static void build_coloricon(Buf *b, int w, int h, int glow,
                            const UBYTE *pixels, const iTidy_RGB8 *pal,
                            int ncolors, int trans)
{
    Buf inner;
    Buf imag;
    UBYTE face[6];
    int i;
    ULONG pix_n = (ULONG)w * (ULONG)h;
    ULONG pal_n = (ULONG)ncolors * 3UL;

    buf_init(b);
    append_diskobject(b, 1, 0, 0, w, h);
    append_image_ex(b, 16, 16, 1, 0x01, 0x00, NULL);

    buf_init(&inner);
    buf_append(&inner, "ICON", 4);
    face[0] = (UBYTE)(w - 1);
    face[1] = (UBYTE)(h - 1);
    face[2] = 0;
    face[3] = 0x11;
    {
        unsigned maxp = glow ? 255U : (unsigned)(ncolors - 1);
        face[4] = (UBYTE)((maxp >> 8) & 0xFF);
        face[5] = (UBYTE)(maxp & 0xFF);
    }
    append_chunk(&inner, "FACE", face, 6);

    buf_init(&imag);
    buf_u8(&imag, trans >= 0 ? (unsigned)trans : 0);
    buf_u8(&imag, (unsigned)(ncolors - 1));
    buf_u8(&imag, trans >= 0 ? 0x03 : 0x02);
    buf_u8(&imag, 0);
    buf_u8(&imag, 0);
    buf_u8(&imag, 8);
    buf_u16(&imag, (unsigned)(pix_n - 1UL));
    buf_u16(&imag, (unsigned)(pal_n - 1UL));
    buf_append(&imag, pixels, pix_n);
    for (i = 0; i < ncolors; i++)
    {
        buf_u8(&imag, pal[i].r);
        buf_u8(&imag, pal[i].g);
        buf_u8(&imag, pal[i].b);
    }
    append_chunk(&inner, "IMAG", imag.data, imag.len);
    buf_free(&imag);

    buf_append(b, "FORM", 4);
    buf_u32(b, (unsigned)inner.len);
    buf_append(b, inner.data, inner.len);
    buf_free(&inner);
}

/*========================================================================*/
/* File / corpus helpers                                                  */
/*========================================================================*/

static void ensure_corpus_dirs(void)
{
    mk_dir("tests");
    mk_dir("tests/icons");
    mk_dir("tests/icons/classic");
    mk_dir("tests/icons/newicons");
    mk_dir("tests/icons/coloricons");
    mk_dir("tests/icons/glowicons");
    mk_dir("tests/icons/malformed");
}

static int write_file(const char *path, const UBYTE *data, ULONG n)
{
    FILE *fp = fopen(path, "wb");
    if (fp == NULL)
        return 0;
    if (fwrite(data, 1, n, fp) != n)
    {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}

static UBYTE *load_file(const char *path, ULONG *out_size)
{
    FILE *fp;
    long sz;
    UBYTE *buf;

    fp = fopen(path, "rb");
    if (fp == NULL)
        return NULL;
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return NULL;
    }
    sz = ftell(fp);
    if (sz < 0)
    {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    buf = (UBYTE *)malloc((size_t)sz);
    if (buf == NULL)
    {
        fclose(fp);
        return NULL;
    }
    if (fread(buf, 1, (size_t)sz, fp) != (size_t)sz)
    {
        free(buf);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    *out_size = (ULONG)sz;
    return buf;
}

static void write_expected(const char *path, const iTidy_DecodedIcon *d)
{
    FILE *fp = fopen(path, "w");
    ULONG pix_n;
    ULONG pal_n;
    if (fp == NULL)
        return;
    pix_n = (ULONG)d->normal.width * (ULONG)d->normal.height;
    pal_n = (ULONG)d->normal.palette_count * 3UL;
    fprintf(fp, "src=%lu\n", (unsigned long)d->source_format);
    fprintf(fp, "w=%u\n", d->normal.width);
    fprintf(fp, "h=%u\n", d->normal.height);
    fprintf(fp, "pal=%u\n", d->normal.palette_count);
    fprintf(fp, "trans=%ld\n", (long)d->normal.transparent_index);
    fprintf(fp, "selected=%d\n", d->has_selected ? 1 : 0);
    fprintf(fp, "pixels_crc=0x%08lX\n",
            (unsigned long)crc32_buf(d->normal.pixels, pix_n));
    if (d->normal.palette != NULL && pal_n > 0)
        fprintf(fp, "palette_crc=0x%08lX\n",
                (unsigned long)crc32_buf((const UBYTE *)d->normal.palette, pal_n));
    else
        fprintf(fp, "palette_crc=0x00000000\n");
    fclose(fp);
}

static void save_corpus(const char *info_path, const char *exp_path,
                        const Buf *b, const iTidy_DecodedIcon *d)
{
    write_file(info_path, b->data, b->len);
    write_expected(exp_path, d);
}

/*========================================================================*/
/* Tests                                                                  */
/*========================================================================*/

static void test_classic_simple(void)
{
    Buf b;
    UBYTE planes[64];
    iTidy_IconFile file;
    iTidy_DecodedIcon dec;
    UBYTE expect[16];
    int x;

    memset(planes, 0, sizeof(planes));
    /* 8x2x1: pixels (0,0)=1 (1,0)=0 (2,0)=1 ... */
    for (x = 0; x < 8; x++)
        planar_set(planes, 8, 2, 1, x, 0, x & 1);
    planar_set(planes, 8, 2, 1, 0, 1, 1);

    buf_init(&b);
    append_diskobject(&b, 1, 0, 0, 8, 2);
    append_image_ex(&b, 8, 2, 1, 0x01, 0x00, planes);

    expect_err("classic parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("classic decode", icon_classic_decode(&file, &dec), ITIDY_ICON_OK);
    expect_eq_u("classic src", (unsigned)dec.source_format, ITIDY_ICON_SRC_CLASSIC);
    expect_eq_u("classic w", dec.normal.width, 8);
    expect_eq_u("classic h", dec.normal.height, 2);
    expect_eq_u("classic pal count", dec.normal.palette_count, 0);
    expect_true("classic pal null", dec.normal.palette == NULL);
    expect_eq_i("classic trans", (int)dec.normal.transparent_index, -1);
    expect_false("classic no selected", dec.has_selected);

    memset(expect, 0, sizeof(expect));
    for (x = 0; x < 8; x++)
        expect[x] = (UBYTE)(x & 1);
    expect[8] = 1;
    expect_mem("classic pixels", dec.normal.pixels, expect, 16);

    save_corpus("tests/icons/classic/8x2x1.info",
                "tests/icons/classic/8x2x1.expected", &b, &dec);
    icon_decoded_free(&dec);
    buf_free(&b);
}

static void test_classic_selected_and_onoff(void)
{
    Buf b;
    UBYTE nplanes[32];
    UBYTE splanes[32];
    iTidy_IconFile file;
    iTidy_DecodedIcon dec;

    memset(nplanes, 0, sizeof(nplanes));
    memset(splanes, 0, sizeof(splanes));
    planar_set(nplanes, 8, 1, 1, 0, 0, 1);
    planar_set(splanes, 8, 1, 1, 1, 0, 1);

    buf_init(&b);
    append_diskobject(&b, 1, 1, 0, 8, 1);
    /* PlanePick bit0 from data, PlaneOnOff bit1 always on => pens 2 or 3 */
    append_image_ex(&b, 8, 1, 1, 0x01, 0x02, nplanes);
    append_image_ex(&b, 8, 1, 1, 0x01, 0x02, splanes);

    expect_err("classic sel parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("classic sel decode", icon_classic_decode(&file, &dec), ITIDY_ICON_OK);
    expect_true("classic has selected", dec.has_selected);
    expect_eq_u("classic onoff pen0", dec.normal.pixels[0], 3); /* data 1 | onoff 2 */
    expect_eq_u("classic onoff pen1", dec.normal.pixels[1], 2); /* data 0 | onoff 2 */
    expect_eq_u("classic sel pen1", dec.selected.pixels[1], 3);

    save_corpus("tests/icons/classic/8x1_selected.info",
                "tests/icons/classic/8x1_selected.expected", &b, &dec);
    icon_decoded_free(&dec);
    buf_free(&b);
}

static void test_classic_no_image(void)
{
    Buf b;
    iTidy_IconFile file;
    iTidy_DecodedIcon dec;

    buf_init(&b);
    append_diskobject(&b, 0, 0, 0, 16, 16);
    expect_err("classic empty parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("classic empty decode", icon_classic_decode(&file, &dec),
               ITIDY_ICON_ERR_NO_DATA);
    buf_free(&b);
}

static void test_newicons_basic(void)
{
    NiEnc e;
    Buf b;
    iTidy_RGB8 pal[4];
    UBYTE pix[4];
    const char *tts[8];
    int i;
    iTidy_IconFile file;
    iTidy_DecodedIcon dec;

    pal[0].r = 255; pal[0].g = 0; pal[0].b = 0;
    pal[1].r = 0; pal[1].g = 255; pal[1].b = 0;
    pal[2].r = 0; pal[2].g = 0; pal[2].b = 255;
    pal[3].r = 255; pal[3].g = 255; pal[3].b = 0;
    pix[0] = 0; pix[1] = 1; pix[2] = 2; pix[3] = 3;

    ni_start(&e, "IM1=", 123);
    ni_encode_image(&e, 1, 2, 2, pal, 4, pix);

    tts[0] = " ";
    tts[1] = "*** DON'T EDIT THE FOLLOWING LINES!! ***";
    for (i = 0; i < e.nlines; i++)
        tts[2 + i] = e.lines[i];

    build_with_tooltypes(&b, tts, 2 + e.nlines, 1);
    expect_err("ni parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("ni decode", icon_newicons_decode(&file, &dec), ITIDY_ICON_OK);
    expect_eq_u("ni src", (unsigned)dec.source_format, ITIDY_ICON_SRC_NEWICONS);
    expect_eq_u("ni w", dec.normal.width, 2);
    expect_eq_u("ni h", dec.normal.height, 2);
    expect_eq_u("ni pal", dec.normal.palette_count, 4);
    expect_eq_i("ni trans", (int)dec.normal.transparent_index, 0);
    expect_mem("ni pixels", dec.normal.pixels, pix, 4);
    expect_mem("ni pal rgb", dec.normal.palette, pal, sizeof(pal));
    expect_false("ni no sel", dec.has_selected);

    save_corpus("tests/icons/newicons/2x2_4col.info",
                "tests/icons/newicons/2x2_4col.expected", &b, &dec);
    icon_decoded_free(&dec);
    buf_free(&b);
}

static void test_newicons_selected_8col(void)
{
    NiEnc e1, e2;
    Buf b;
    iTidy_RGB8 pal[8];
    UBYTE pix1[16];
    UBYTE pix2[16];
    const char *tts[16];
    int i;
    iTidy_IconFile file;
    iTidy_DecodedIcon dec;

    for (i = 0; i < 8; i++)
    {
        pal[i].r = (UBYTE)(i * 32);
        pal[i].g = (UBYTE)(255 - i * 16);
        pal[i].b = (UBYTE)(i * 8);
    }
    for (i = 0; i < 16; i++)
    {
        pix1[i] = (UBYTE)(i & 7);
        pix2[i] = (UBYTE)((i + 3) & 7);
    }

    ni_start(&e1, "IM1=", 123);
    ni_encode_image(&e1, 0, 4, 4, pal, 8, pix1);
    ni_start(&e2, "IM2=", 123);
    ni_encode_image(&e2, 0, 4, 4, pal, 8, pix2);

    tts[0] = "*** DON'T EDIT THE FOLLOWING LINES!! ***";
    for (i = 0; i < e1.nlines; i++)
        tts[1 + i] = e1.lines[i];
    for (i = 0; i < e2.nlines; i++)
        tts[1 + e1.nlines + i] = e2.lines[i];

    build_with_tooltypes(&b, tts, 1 + e1.nlines + e2.nlines, 1);
    expect_err("ni8 parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("ni8 decode", icon_newicons_decode(&file, &dec), ITIDY_ICON_OK);
    expect_true("ni8 selected", dec.has_selected);
    expect_eq_i("ni8 opaque", (int)dec.normal.transparent_index, -1);
    expect_mem("ni8 pix1", dec.normal.pixels, pix1, 16);
    expect_mem("ni8 pix2", dec.selected.pixels, pix2, 16);

    save_corpus("tests/icons/newicons/4x4_8col_selected.info",
                "tests/icons/newicons/4x4_8col_selected.expected", &b, &dec);
    icon_decoded_free(&dec);
    buf_free(&b);
}

static void test_newicons_16col_and_boundary(void)
{
    NiEnc e;
    Buf b;
    iTidy_RGB8 pal[16];
    UBYTE pix[64];
    const char *tts[NI_MAX_LINES + 4];
    int i;
    iTidy_IconFile file;
    iTidy_DecodedIcon dec;

    for (i = 0; i < 16; i++)
    {
        pal[i].r = (UBYTE)(i * 17);
        pal[i].g = (UBYTE)(255 - i * 17);
        pal[i].b = 128;
    }
    for (i = 0; i < 64; i++)
        pix[i] = (UBYTE)(i & 15);

    /* Short lines wrap 7-bit groups; sample bits must continue across
     * ToolType boundaries (not a naive IM1= string concatenation). */
    ni_start(&e, "IM1=", 18);
    ni_encode_image(&e, 1, 8, 8, pal, 16, pix);
    expect_true("ni16 multi-line", e.nlines > 1);

    tts[0] = "*** DON'T EDIT THE FOLLOWING LINES!! ***";
    for (i = 0; i < e.nlines; i++)
        tts[1 + i] = e.lines[i];

    build_with_tooltypes(&b, tts, 1 + e.nlines, 1);
    expect_err("ni16 parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("ni16 decode", icon_newicons_decode(&file, &dec), ITIDY_ICON_OK);
    if (dec.normal.pixels != NULL && dec.normal.palette != NULL)
    {
        expect_eq_u("ni16 pal", dec.normal.palette_count, 16);
        expect_mem("ni16 pixels", dec.normal.pixels, pix, 64);
        expect_mem("ni16 palette", dec.normal.palette, pal, sizeof(pal));
        expect_eq_i("ni16 trans", (int)dec.normal.transparent_index, 0);
        save_corpus("tests/icons/newicons/8x8_16col_multiline.info",
                    "tests/icons/newicons/8x8_16col_multiline.expected", &b, &dec);
    }
    icon_decoded_free(&dec);
    buf_free(&b);
}

static void test_newicons_rle_zeros(void)
{
    NiEnc e;
    Buf b;
    iTidy_RGB8 pal[2];
    const char *tts[8];
    int i;
    iTidy_IconFile file;
    iTidy_DecodedIcon dec;
    UBYTE expect[8];

    pal[0].r = 0; pal[0].g = 0; pal[0].b = 0;
    pal[1].r = 255; pal[1].g = 255; pal[1].b = 255;

    ni_start(&e, "IM1=", 123);
    ni_put_header(&e, 0, 2, 1, 2);
    /* First encoded byte is RLE: 7 zero bits of the first red channel. */
    ni_emit_rle_zeros(&e, 1);
    ni_put_bits(&e, 0, 1);   /* last bit of red */
    ni_put_bits(&e, 0, 8);
    ni_put_bits(&e, 0, 8);
    ni_put_bits(&e, 255, 8);
    ni_put_bits(&e, 255, 8);
    ni_put_bits(&e, 255, 8);
    ni_put_bits(&e, 0, 1);
    ni_put_bits(&e, 1, 1);
    ni_finish(&e);

    tts[0] = "*** DON'T EDIT THE FOLLOWING LINES!! ***";
    for (i = 0; i < e.nlines; i++)
        tts[1 + i] = e.lines[i];
    build_with_tooltypes(&b, tts, 1 + e.nlines, 1);

    expect_err("ni rle parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("ni rle decode", icon_newicons_decode(&file, &dec), ITIDY_ICON_OK);
    expect[0] = 0;
    expect[1] = 1;
    expect_eq_u("ni rle w", dec.normal.width, 2);
    expect_mem("ni rle pixels", dec.normal.pixels, expect, 2);
    expect_mem("ni rle pal", dec.normal.palette, pal, sizeof(pal));

    save_corpus("tests/icons/newicons/4x2_rle_zeros.info",
                "tests/icons/newicons/4x2_rle_zeros.expected", &b, &dec);
    icon_decoded_free(&dec);
    buf_free(&b);
}

static void test_newicons_malformed(void)
{
    Buf b;
    iTidy_IconFile file;
    iTidy_DecodedIcon dec;
    const char *short_tt[] = { "IM1=B!!" }; /* header truncated */
    const char *bad_dim[] = { "IM1=B!!!!" }; /* width/height 0 */
    const char *no_im[] = {
        "*** DON'T EDIT THE FOLLOWING LINES!! ***"
    };

    build_with_tooltypes(&b, short_tt, 1, 1);
    expect_err("ni trunc parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("ni trunc decode", icon_newicons_decode(&file, &dec),
               ITIDY_ICON_ERR_TRUNCATED);
    write_file("tests/icons/malformed/newicons_trunc_header.info", b.data, b.len);
    buf_free(&b);

    build_with_tooltypes(&b, bad_dim, 1, 1);
    expect_err("ni dim parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("ni dim decode", icon_newicons_decode(&file, &dec),
               ITIDY_ICON_ERR_BAD_DIMENSION);
    write_file("tests/icons/malformed/newicons_zero_size.info", b.data, b.len);
    buf_free(&b);

    build_with_tooltypes(&b, no_im, 1, 1);
    expect_err("ni none parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("ni none decode", icon_newicons_decode(&file, &dec),
               ITIDY_ICON_ERR_NO_DATA);
    buf_free(&b);
}

static void test_unified_preference(void)
{
    Buf b;
    iTidy_RGB8 pal[2];
    UBYTE cpix[4];
    UBYTE npix[4];
    NiEnc e;
    const char *tts[8];
    int i;
    iTidy_IconFile file;
    iTidy_DecodedIcon dec;
    UBYTE orig0;

    pal[0].r = 10; pal[0].g = 20; pal[0].b = 30;
    pal[1].r = 40; pal[1].g = 50; pal[1].b = 60;
    cpix[0] = 0; cpix[1] = 1; cpix[2] = 1; cpix[3] = 0;
    npix[0] = 1; npix[1] = 0; npix[2] = 0; npix[3] = 1;

    /* ColorIcon + NewIcons + classic: BEST must pick ColorIcon. */
    ni_start(&e, "IM1=", 123);
    ni_encode_image(&e, 0, 2, 2, pal, 2, npix);
    buf_init(&b);
    append_diskobject(&b, 1, 0, 1, 2, 2);
    append_image_ex(&b, 16, 16, 1, 0x01, 0x00, NULL);
    tts[0] = "*** DON'T EDIT THE FOLLOWING LINES!! ***";
    for (i = 0; i < e.nlines; i++)
        tts[1 + i] = e.lines[i];
    append_tooltypes(&b, tts, 1 + e.nlines);
    {
        Buf inner, imag;
        UBYTE face[6];
        buf_init(&inner);
        buf_append(&inner, "ICON", 4);
        face[0] = 1; face[1] = 1; face[2] = 0; face[3] = 0x11;
        face[4] = 0; face[5] = 1;
        append_chunk(&inner, "FACE", face, 6);
        buf_init(&imag);
        buf_u8(&imag, 0);
        buf_u8(&imag, 1);
        buf_u8(&imag, 0x02);
        buf_u8(&imag, 0);
        buf_u8(&imag, 0);
        buf_u8(&imag, 8);
        buf_u16(&imag, 3);
        buf_u16(&imag, 5);
        buf_append(&imag, cpix, 4);
        buf_u8(&imag, pal[0].r); buf_u8(&imag, pal[0].g); buf_u8(&imag, pal[0].b);
        buf_u8(&imag, pal[1].r); buf_u8(&imag, pal[1].g); buf_u8(&imag, pal[1].b);
        append_chunk(&inner, "IMAG", imag.data, imag.len);
        buf_free(&imag);
        buf_append(&b, "FORM", 4);
        buf_u32(&b, (unsigned)inner.len);
        buf_append(&b, inner.data, inner.len);
        buf_free(&inner);
    }

    orig0 = b.data[0];
    expect_err("best parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("best decode", icon_decode(&file, ITIDY_ICON_REQ_BEST, &dec),
               ITIDY_ICON_OK);
    expect_eq_u("best is coloricon", (unsigned)dec.source_format,
                ITIDY_ICON_SRC_COLORICON);
    expect_mem("best color pixels", dec.normal.pixels, cpix, 4);
    icon_decoded_free(&dec);

    expect_err("req newicons", icon_decode(&file, ITIDY_ICON_REQ_NEWICONS, &dec),
               ITIDY_ICON_OK);
    expect_eq_u("explicit ni src", (unsigned)dec.source_format,
                ITIDY_ICON_SRC_NEWICONS);
    expect_mem("explicit ni pixels", dec.normal.pixels, npix, 4);
    icon_decoded_free(&dec);

    expect_err("req classic", icon_decode(&file, ITIDY_ICON_REQ_CLASSIC, &dec),
               ITIDY_ICON_OK);
    expect_eq_u("explicit classic src", (unsigned)dec.source_format,
                ITIDY_ICON_SRC_CLASSIC);
    icon_decoded_free(&dec);

    expect_eq_u("buffer not modified", b.data[0], orig0);
    buf_free(&b);
}

static void test_unified_newicons_over_classic(void)
{
    NiEnc e;
    Buf b;
    iTidy_RGB8 pal[2];
    UBYTE pix[4];
    const char *tts[8];
    int i;
    iTidy_DecodedIcon dec;

    pal[0].r = 1; pal[0].g = 2; pal[0].b = 3;
    pal[1].r = 4; pal[1].g = 5; pal[1].b = 6;
    pix[0] = 0; pix[1] = 1; pix[2] = 1; pix[3] = 0;
    ni_start(&e, "IM1=", 123);
    ni_encode_image(&e, 0, 2, 2, pal, 2, pix);
    tts[0] = "*** DON'T EDIT THE FOLLOWING LINES!! ***";
    for (i = 0; i < e.nlines; i++)
        tts[1 + i] = e.lines[i];
    build_with_tooltypes(&b, tts, 1 + e.nlines, 1);

    expect_err("ni-over-classic",
               icon_decode_buffer(b.data, b.len, ITIDY_ICON_REQ_BEST, &dec),
               ITIDY_ICON_OK);
    expect_eq_u("pref newicons", (unsigned)dec.source_format,
                ITIDY_ICON_SRC_NEWICONS);
    icon_decoded_free(&dec);
    buf_free(&b);
}

static void test_unified_unsupported(void)
{
    iTidy_DecodedIcon dec;
    static const UBYTE png[8] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
    };
    Buf b;
    Buf inner;

    expect_err("png only",
               icon_decode_buffer(png, 8, ITIDY_ICON_REQ_BEST, &dec),
               ITIDY_ICON_ERR_UNSUPPORTED);
    write_file("tests/icons/malformed/png_only.info", png, 8);

    buf_init(&b);
    append_diskobject(&b, 1, 0, 0, 16, 16);
    append_image_ex(&b, 16, 16, 1, 0x01, 0x00, NULL);
    buf_init(&inner);
    buf_append(&inner, "ARGB", 4);
    buf_append(&b, "FORM", 4);
    buf_u32(&b, (unsigned)inner.len);
    buf_append(&b, inner.data, inner.len);
    buf_free(&inner);

    /* Classic still present: BEST should use classic, not UNSUPPORTED. */
    expect_err("argb with classic",
               icon_decode_buffer(b.data, b.len, ITIDY_ICON_REQ_BEST, &dec),
               ITIDY_ICON_OK);
    expect_eq_u("argb fallback classic", (unsigned)dec.source_format,
                ITIDY_ICON_SRC_CLASSIC);
    icon_decoded_free(&dec);

    write_file("tests/icons/malformed/form_argb_with_classic.info", b.data, b.len);
    buf_free(&b);
}

static void test_coloricon_corpus(void)
{
    Buf b;
    iTidy_RGB8 pal[2];
    UBYTE pix[4];
    iTidy_DecodedIcon dec;

    pal[0].r = 255; pal[0].g = 0; pal[0].b = 0;
    pal[1].r = 0; pal[1].g = 0; pal[1].b = 255;
    pix[0] = 0; pix[1] = 1; pix[2] = 1; pix[3] = 0;

    build_coloricon(&b, 2, 2, 0, pix, pal, 2, 0);
    expect_err("ci corpus",
               icon_decode_buffer(b.data, b.len, ITIDY_ICON_REQ_BEST, &dec),
               ITIDY_ICON_OK);
    expect_eq_u("ci src", (unsigned)dec.source_format, ITIDY_ICON_SRC_COLORICON);
    expect_eq_i("ci trans", (int)dec.normal.transparent_index, 0);
    save_corpus("tests/icons/coloricons/2x2_raw.info",
                "tests/icons/coloricons/2x2_raw.expected", &b, &dec);
    icon_decoded_free(&dec);
    buf_free(&b);

    build_coloricon(&b, 2, 2, 1, pix, pal, 2, -1);
    expect_err("glow corpus",
               icon_decode_buffer(b.data, b.len, ITIDY_ICON_REQ_BEST, &dec),
               ITIDY_ICON_OK);
    expect_eq_u("glow src", (unsigned)dec.source_format, ITIDY_ICON_SRC_GLOWICON);
    save_corpus("tests/icons/glowicons/2x2_glow.info",
                "tests/icons/glowicons/2x2_glow.expected", &b, &dec);
    icon_decoded_free(&dec);
    buf_free(&b);
}

static void test_reload_corpus(void)
{
    static const char *paths[] = {
        "tests/icons/classic/8x2x1.info",
        "tests/icons/newicons/2x2_4col.info",
        "tests/icons/newicons/8x8_16col_multiline.info",
        "tests/icons/coloricons/2x2_raw.info",
        "tests/icons/glowicons/2x2_glow.info"
    };
    unsigned i;

    for (i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
    {
        ULONG sz;
        UBYTE *buf = load_file(paths[i], &sz);
        iTidy_DecodedIcon dec;
        char name[160];

        sprintf(name, "reload %s", paths[i]);
        if (buf == NULL)
        {
            fail(name, "missing file");
            continue;
        }
        expect_err(name, icon_decode_buffer(buf, sz, ITIDY_ICON_REQ_BEST, &dec),
                   ITIDY_ICON_OK);
        icon_decoded_free(&dec);
        free(buf);
    }
}

static void test_null_args(void)
{
    iTidy_DecodedIcon dec;
    expect_err("classic null", icon_classic_decode(NULL, &dec), ITIDY_ICON_ERR_NULL);
    expect_err("newicons null", icon_newicons_decode(NULL, &dec), ITIDY_ICON_ERR_NULL);
    expect_err("decode null out", icon_decode_buffer(NULL, 0, ITIDY_ICON_REQ_BEST, NULL),
               ITIDY_ICON_ERR_NULL);
}

int main(void)
{
    ensure_corpus_dirs();

    test_classic_simple();
    test_classic_selected_and_onoff();
    test_classic_no_image();
    test_newicons_basic();
    test_newicons_selected_8col();
    test_newicons_16col_and_boundary();
    test_newicons_rle_zeros();
    test_newicons_malformed();
    test_unified_preference();
    test_unified_newicons_over_classic();
    test_unified_unsupported();
    test_coloricon_corpus();
    test_reload_corpus();
    test_null_args();

    if (g_failures)
    {
        printf("\n%d test(s) failed\n", g_failures);
        return 1;
    }
    printf("\nAll shared decode tests passed.\n");
    return 0;
}
