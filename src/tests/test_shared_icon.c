/*
 * test_shared_icon.c - Host regression tests for shared .info reader/probe
 *
 * Compile/run via: make test-icon
 * Does not link Amiga libraries. Uses src/tests/host_stubs for exec types.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <icon/icon_file.h>
#include <icon/icon_probe.h>

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
    if (got != want)
    {
        char buf[128];
        sprintf(buf, "got %d want %d", got, want);
        fail(name, buf);
    }
    else
    {
        pass(name);
    }
}

static void expect_eq_u(const char *name, unsigned got, unsigned want)
{
    if (got != want)
    {
        char buf[128];
        sprintf(buf, "got %u want %u", got, want);
        fail(name, buf);
    }
    else
    {
        pass(name);
    }
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
    if (got != want)
    {
        char buf[160];
        sprintf(buf, "got %d (%s) want %d (%s)",
                got, icon_error_string(got), want, icon_error_string(want));
        fail(name, buf);
    }
    else
    {
        pass(name);
    }
}

/*========================================================================*/
/* Big-endian buffer builder                                              */
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
                              UBYTE type,
                              int has_render,
                              int has_select,
                              int has_deftool,
                              int has_tooltypes,
                              int has_drawer,
                              int has_toolwin,
                              unsigned userdata,
                              int gadget_w,
                              int gadget_h)
{
    ULONG start = b->len;
    buf_zeros(b, ITIDY_ICON_DISKOBJECT_SIZE);
    buf_patch_u32(b, start + 0x00, 0); /* filled below as words */
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
    buf_patch_u32(b, start + 0x2C, userdata);
    b->data[start + 0x30] = type;
    buf_patch_u32(b, start + 0x32, has_deftool ? 1UL : 0UL);
    buf_patch_u32(b, start + 0x36, has_tooltypes ? 1UL : 0UL);
    buf_patch_u32(b, start + 0x3A, 0x80000000UL);
    buf_patch_u32(b, start + 0x3E, 0x80000000UL);
    buf_patch_u32(b, start + 0x42, has_drawer ? 0x4031AF28UL : 0UL);
    buf_patch_u32(b, start + 0x46, has_toolwin ? 1UL : 0UL);
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
    buf_u32(b, 0x4046DC28UL); /* leftover ImageData pointer, not an offset */
    buf_u8(b, (unsigned)((1 << depth) - 1));
    buf_u8(b, 0);
    buf_u32(b, 0);
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

static void append_form_icon(Buf *b, unsigned width, unsigned height,
                             unsigned max_pal_minus_1, int imag_count)
{
    Buf inner;
    UBYTE face[6];
    int i;

    buf_init(&inner);
    buf_append(&inner, "ICON", 4);

    face[0] = (UBYTE)((width - 1U) & 0xFF);
    face[1] = (UBYTE)((height - 1U) & 0xFF);
    face[2] = 0;      /* flags */
    face[3] = 0x11;   /* 1:1 aspect */
    face[4] = (UBYTE)((max_pal_minus_1 >> 8) & 0xFF);
    face[5] = (UBYTE)(max_pal_minus_1 & 0xFF);
    append_chunk(&inner, "FACE", face, 6);

    for (i = 0; i < imag_count; i++)
    {
        UBYTE imag[12];
        memset(imag, 0, sizeof(imag));
        imag[0] = 0;    /* transparent */
        imag[1] = 1;    /* 2 colours - 1 */
        imag[2] = 0x03; /* has transparent + has palette */
        imag[3] = 0;    /* uncompressed image */
        imag[4] = 0;    /* uncompressed pal */
        imag[5] = 1;    /* depth */
        imag[6] = 0x00; /* image size-1 high */
        imag[7] = 0x00; /* image size-1 = 0 => 1 byte */
        imag[8] = 0x00;
        imag[9] = 0x05; /* pal size-1 = 5 => 6 bytes RGB */
        imag[10] = 0;   /* one dummy pixel */
        imag[11] = 0;
        append_chunk(&inner, "IMAG", imag, 12);
    }

    buf_append(b, "FORM", 4);
    buf_u32(b, (unsigned)inner.len);
    buf_append(b, inner.data, inner.len);
    buf_free(&inner);
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

/*========================================================================*/
/* Tests                                                                  */
/*========================================================================*/

static void test_malformed(void)
{
    iTidy_IconFile file;
    iTidy_IconProbe probe;
    UBYTE tiny[4] = { 1, 2, 3, 4 };
    UBYTE small[40];
    Buf b;

    expect_err("parse NULL data",
               icon_file_parse(NULL, 10, &file), ITIDY_ICON_ERR_NULL);
    expect_err("parse NULL out",
               icon_file_parse(tiny, 4, NULL), ITIDY_ICON_ERR_NULL);

    expect_err("too small",
               icon_file_parse(tiny, 4, &file), ITIDY_ICON_ERR_TOO_SMALL);

    memset(small, 0, sizeof(small));
    small[0] = 0xE3;
    small[1] = 0x10;
    expect_err("header truncated",
               icon_file_parse(small, sizeof(small), &file),
               ITIDY_ICON_ERR_TOO_SMALL);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 0, 0, 0, 0, 16, 16);
    b.data[0] = 0x00;
    b.data[1] = 0x00;
    expect_err("bad magic",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_ERR_BAD_MAGIC);
    buf_free(&b);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 0, 0, 0, 0, 16, 16);
    b.data[3] = 2;
    expect_err("bad version",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_ERR_BAD_VERSION);
    buf_free(&b);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 0, 0, 0, 0, 16, 16);
    expect_err("truncated before image",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_ERR_TRUNCATED);
    buf_free(&b);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 0, 0, 0, 0, 16, 16);
    append_image(&b, 16, 16, 1);
    b.len = ITIDY_ICON_DISKOBJECT_SIZE + 10; /* mid image header */
    expect_err("truncated image header",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_ERR_TRUNCATED);
    buf_free(&b);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 0, 0, 0, 0, 16, 16);
    append_image(&b, 0, 16, 1);
    expect_err("zero width",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_ERR_BAD_DIMENSION);
    buf_free(&b);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 0, 0, 0, 0, 16, 16);
    append_image(&b, 16, 16, 9);
    expect_err("depth 9",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_ERR_BAD_DIMENSION);
    buf_free(&b);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 0, 0, 0, 1, 0, 0, 0, 16, 16);
    buf_u32(&b, 3); /* not a multiple of 4 */
    expect_err("bad tooltype count",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_ERR_BAD_COUNT);
    buf_free(&b);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 0, 0, 1, 0, 0, 0, 0, 16, 16);
    buf_u32(&b, 0); /* tx_Size 0 */
    expect_err("empty text size",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_ERR_BAD_TEXT);
    buf_free(&b);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 0, 0, 1, 0, 0, 0, 0, 16, 16);
    buf_u32(&b, 4);
    buf_append(&b, "ABCX", 4); /* no NUL */
    expect_err("text missing NUL",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_ERR_BAD_TEXT);
    buf_free(&b);

    expect_err("probe NULL out",
               icon_probe_buffer(tiny, 4, NULL), ITIDY_ICON_ERR_NULL);
    expect_err("probe truncated DiskObject",
               icon_probe_buffer(tiny, 4, &probe), ITIDY_ICON_ERR_TOO_SMALL);
}

static void test_classic_normal_only(void)
{
    Buf b;
    iTidy_IconFile file;
    iTidy_IconProbe probe;
    UBYTE *copy;
    ULONG want_data;

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 0, 0, 0, 0, 16, 16);
    append_image(&b, 16, 16, 1);

    copy = (UBYTE *)malloc(b.len);
    memcpy(copy, b.data, b.len);

    expect_err("classic parse",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_eq_u("classic type", file.type, ITIDY_ICON_WB_TOOL);
    expect_eq_i("classic gadget w", file.gadget_width, 16);
    expect_eq_i("classic gadget h", file.gadget_height, 16);
    expect_true("classic normal present", file.normal.present);
    expect_false("classic selected absent", file.selected.present);
    expect_eq_i("classic image w", file.normal.width, 16);
    expect_eq_i("classic image h", file.normal.height, 16);
    expect_eq_i("classic image depth", file.normal.depth, 1);
    want_data = planar_size(16, 16, 1);
    expect_eq_u("classic planar size", file.normal.data_size, want_data);
    expect_eq_u("classic image at 0x4E", file.normal.header_offset, 78);
    expect_eq_u("classic_end is EOF", file.classic_end, b.len);
    expect_eq_u("no extension", file.extension_size, 0);
    expect_true("pointers not used as offsets",
                file.normal.header_offset != 0x400C6718UL);

    expect_true("buffer not modified", memcmp(copy, b.data, b.len) == 0);

    expect_err("classic probe",
               icon_probe_file(&file, &probe), ITIDY_ICON_OK);
    expect_true("probe classic", probe.has_classic);
    expect_true("probe normal", probe.has_normal);
    expect_false("probe selected", probe.has_selected);
    expect_false("probe newicons", probe.has_newicons);
    expect_false("probe coloricon", probe.has_coloricon);
    expect_false("probe glow", probe.has_glowicon);
    expect_false("probe png", probe.has_png);
    expect_false("probe unsupported", probe.has_unsupported);

    free(copy);
    buf_free(&b);
}

static void test_classic_selected_and_drawer(void)
{
    Buf b;
    iTidy_IconFile file;
    iTidy_IconProbe probe;

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_DRAWER, 1, 1, 0, 0, 1, 0, 1, 57, 14);
    buf_zeros(&b, ITIDY_ICON_DRAWERDATA_SIZE);
    append_image(&b, 57, 14, 2);
    append_image(&b, 57, 14, 2);
    buf_u32(&b, 1); /* DrawerData2 flags */
    buf_u16(&b, 0); /* viewmodes */

    expect_err("drawer parse",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_true("drawer data", file.has_drawer_data);
    expect_eq_u("drawerdata at 0x4E", file.drawer_data_offset, 78);
    expect_true("drawerdata2", file.has_drawer_data2);
    expect_true("selected image", file.selected.present);
    expect_eq_i("selected w", file.selected.width, 57);
    expect_eq_i("selected depth", file.selected.depth, 2);
    expect_eq_u("drawer classic_end EOF", file.classic_end, b.len);

    expect_err("drawer probe", icon_probe_file(&file, &probe), ITIDY_ICON_OK);
    expect_true("drawer has classic", probe.has_classic);
    expect_true("drawer has selected", probe.has_selected);
    expect_true("drawer has normal", probe.has_normal);

    buf_free(&b);
}

static void test_default_tool_and_tooltypes(void)
{
    Buf b;
    iTidy_IconFile file;
    const char *tts[2];
    const UBYTE *text;
    ULONG len;

    tts[0] = "FILETYPE=project";
    tts[1] = "STACK=8192";

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_PROJECT, 1, 0, 1, 1, 0, 0, 0, 16, 16);
    append_image(&b, 16, 16, 1);
    append_text(&b, "SYS:Utilities/MultiView");
    append_tooltypes(&b, tts, 2);

    expect_err("meta parse",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_true("default tool present", file.default_tool.present);
    expect_true("tooltypes present", file.has_tooltypes);
    expect_eq_u("tooltype count", file.tooltypes_count, 2);

    expect_true("read default tool",
                icon_file_default_tool(&file, &text, &len));
    expect_eq_u("default tool len", len, (unsigned)strlen("SYS:Utilities/MultiView"));
    expect_true("default tool text",
                memcmp(text, "SYS:Utilities/MultiView", (size_t)len) == 0);

    expect_true("tt0", icon_file_tooltype(&file, 0, &text, &len));
    expect_true("tt0 text", memcmp(text, "FILETYPE=project", (size_t)len) == 0);
    expect_true("tt1", icon_file_tooltype(&file, 1, &text, &len));
    expect_true("tt1 text", memcmp(text, "STACK=8192", (size_t)len) == 0);
    expect_false("tt2 missing", icon_file_tooltype(&file, 2, &text, &len));

    buf_free(&b);
}

static void test_newicons_probe(void)
{
    Buf b;
    iTidy_IconFile file;
    iTidy_IconProbe probe;
    const char *tts[3];

    tts[0] = " ";
    tts[1] = "*** DON'T EDIT THE FOLLOWING LINES!! ***";
    tts[2] = "IM1=B!!'";

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 1, 0, 0, 0, 16, 16);
    append_image(&b, 16, 16, 1);
    append_tooltypes(&b, tts, 3);

    expect_err("newicons parse",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("newicons probe",
               icon_probe_file(&file, &probe), ITIDY_ICON_OK);
    expect_true("newicons marker", probe.has_newicons);
    expect_true("newicons still has classic", probe.has_classic);
    expect_true("newicons IM1 => normal", probe.has_normal);
    expect_false("newicons no IM2", probe.has_selected);
    expect_false("newicons not coloricon", probe.has_coloricon);

    buf_free(&b);

    tts[0] = "IM2=C!!'";
    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 1, 0, 0, 0, 16, 16);
    append_image(&b, 16, 16, 1);
    append_tooltypes(&b, tts, 1);
    expect_err("im2 parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("im2 probe", icon_probe_file(&file, &probe), ITIDY_ICON_OK);
    expect_true("im2 is newicons", probe.has_newicons);
    expect_true("im2 selected", probe.has_selected);
    buf_free(&b);
}

static void test_coloricon_and_glow(void)
{
    Buf b;
    iTidy_IconFile file;
    iTidy_IconProbe probe;

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 0, 0, 0, 0, 16, 16);
    append_image(&b, 16, 16, 1);
    append_form_icon(&b, 16, 16, 7, 1); /* 8-colour ColorIcon, one IMAG */

    expect_err("coloricon parse",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_true("coloricon has extension", file.extension_size > 0);
    expect_true("extension starts at classic_end",
                file.extension_offset == file.classic_end);
    expect_err("coloricon probe",
               icon_probe_file(&file, &probe), ITIDY_ICON_OK);
    expect_true("has coloricon", probe.has_coloricon);
    expect_false("8-colour is not glow", probe.has_glowicon);
    expect_true("coloricon normal (IMAG)", probe.has_normal);
    expect_false("one IMAG => no selected", probe.has_selected);
    expect_true("classic fallback still flagged", probe.has_classic);
    buf_free(&b);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 1, 0, 0, 0, 0, 0, 16, 16);
    append_image(&b, 16, 16, 1);
    append_image(&b, 16, 16, 1);
    append_form_icon(&b, 32, 32, 255, 2); /* 256-colour, two IMAG */

    expect_err("glow parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("glow probe", icon_probe_file(&file, &probe), ITIDY_ICON_OK);
    expect_true("glow has coloricon", probe.has_coloricon);
    expect_true("256-colour FACE => glow", probe.has_glowicon);
    expect_true("two IMAG => selected", probe.has_selected);
    buf_free(&b);
}

static void test_png_and_unsupported(void)
{
    Buf b;
    iTidy_IconFile file;
    iTidy_IconProbe probe;
    static const UBYTE png_sig[8] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
    };

    expect_err("png-only probe",
               icon_probe_buffer(png_sig, 8, &probe), ITIDY_ICON_OK);
    expect_true("png-only has_png", probe.has_png);
    expect_true("png-only unsupported", probe.has_unsupported);
    expect_false("png-only not classic", probe.has_classic);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 0, 0, 0, 0, 16, 16);
    append_image(&b, 16, 16, 1);
    buf_append(&b, png_sig, 8);
    expect_err("dual png parse",
               icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("dual png probe",
               icon_probe_file(&file, &probe), ITIDY_ICON_OK);
    expect_true("dual has classic", probe.has_classic);
    expect_true("dual has png", probe.has_png);
    expect_true("dual unsupported", probe.has_unsupported);
    buf_free(&b);

    buf_init(&b);
    append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 0, 0, 0, 0, 16, 16);
    append_image(&b, 16, 16, 1);
    buf_append(&b, "FORM", 4);
    buf_u32(&b, 4);
    buf_append(&b, "ARGB", 4);
    expect_err("argb parse", icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
    expect_err("argb probe", icon_probe_file(&file, &probe), ITIDY_ICON_OK);
    expect_true("argb flag", probe.has_os4_argb);
    expect_true("argb unsupported", probe.has_unsupported);
    expect_false("argb not coloricon", probe.has_coloricon);
    buf_free(&b);

    /* FORM ICON containing only FACE+ARGB must not look like ColorIcon. */
    {
        Buf inner;
        UBYTE face[6] = { 15, 15, 0, 0x11, 0, 3 };
        UBYTE argb[4] = { 0, 0, 0, 1 };

        buf_init(&inner);
        buf_append(&inner, "ICON", 4);
        append_chunk(&inner, "FACE", face, 6);
        append_chunk(&inner, "ARGB", argb, 4);

        buf_init(&b);
        append_diskobject(&b, ITIDY_ICON_WB_TOOL, 1, 0, 0, 0, 0, 0, 0, 16, 16);
        append_image(&b, 16, 16, 1);
        buf_append(&b, "FORM", 4);
        buf_u32(&b, (unsigned)inner.len);
        buf_append(&b, inner.data, inner.len);

        expect_err("icon+argb parse",
                   icon_file_parse(b.data, b.len, &file), ITIDY_ICON_OK);
        expect_err("icon+argb probe",
                   icon_probe_file(&file, &probe), ITIDY_ICON_OK);
        expect_true("icon+argb has os4", probe.has_os4_argb);
        expect_false("icon+argb not coloricon", probe.has_coloricon);
        expect_true("icon+argb still classic", probe.has_classic);

        buf_free(&inner);
        buf_free(&b);
    }
}

static void test_real_icons(void)
{
    const char *paths[] = {
        "Bin/Amiga.info",
        "Bin/Amiga/iTidy2.info"
    };
    unsigned i;

    for (i = 0; i < 2; i++)
    {
        ULONG size = 0;
        UBYTE *data = load_file(paths[i], &size);
        iTidy_IconFile file;
        iTidy_IconProbe probe;
        char name[128];

        if (data == NULL)
        {
            printf("SKIP: %s not present\n", paths[i]);
            continue;
        }

        sprintf(name, "%s parse", paths[i]);
        expect_err(name, icon_file_parse(data, size, &file), ITIDY_ICON_OK);

        sprintf(name, "%s magic", paths[i]);
        expect_eq_u(name, file.magic, ITIDY_ICON_MAGIC);

        sprintf(name, "%s classic image", paths[i]);
        expect_true(name, file.normal.present);

        sprintf(name, "%s selected image", paths[i]);
        expect_true(name, file.selected.present);

        sprintf(name, "%s classic_end == size", paths[i]);
        expect_eq_u(name, file.classic_end, size);

        sprintf(name, "%s probe", paths[i]);
        expect_err(name, icon_probe_file(&file, &probe), ITIDY_ICON_OK);
        sprintf(name, "%s probe classic", paths[i]);
        expect_true(name, probe.has_classic);
        sprintf(name, "%s probe selected", paths[i]);
        expect_true(name, probe.has_selected);
        sprintf(name, "%s not newicons", paths[i]);
        expect_false(name, probe.has_newicons);
        sprintf(name, "%s not coloricon", paths[i]);
        expect_false(name, probe.has_coloricon);

        /* leftover on-disk pointers must not be treated as file offsets */
        sprintf(name, "%s pointer not offset", paths[i]);
        expect_true(name, file.normal.header_offset < size);

        free(data);
    }
}

static void test_readers(void)
{
    UBYTE buf[4] = { 0x12, 0x34, 0x56, 0x78 };
    UWORD u16;
    ULONG u32;
    ULONG planar;

    expect_true("u16", icon_file_read_u16(buf, 4, 0, &u16) && u16 == 0x1234);
    expect_true("u32", icon_file_read_u32(buf, 4, 0, &u32) && u32 == 0x12345678UL);
    expect_false("u16 oob", icon_file_read_u16(buf, 4, 3, &u16));
    expect_true("planar 16x16x1",
                icon_file_planar_data_size(16, 16, 1, &planar) && planar == 32);
    expect_true("planar 57x21x3",
                icon_file_planar_data_size(57, 21, 3, &planar) && planar == 504);
    expect_false("planar overflow dim",
                 icon_file_planar_data_size(2000, 16, 1, &planar));
}

int main(void)
{
    test_readers();
    test_malformed();
    test_classic_normal_only();
    test_classic_selected_and_drawer();
    test_default_tool_and_tooltypes();
    test_newicons_probe();
    test_coloricon_and_glow();
    test_png_and_unsupported();
    test_real_icons();

    if (g_failures)
    {
        printf("\n%d test(s) failed\n", g_failures);
        return 1;
    }
    printf("\nAll shared icon tests passed.\n");
    return 0;
}
