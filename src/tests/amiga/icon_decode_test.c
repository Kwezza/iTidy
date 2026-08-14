/*
 * icon_decode_test.c - Amiga Shell CLI for shared icon-decoder validation
 *
 * Recursively scans a directory of real .info files, probes and decodes
 * each with the shared APIs, and prints a pasteable report plus summary.
 *
 * Build:  make test-icon-amiga
 * Binary: Bin/Amiga/iTidy2/iTidyIconTest
 *
 * Usage:
 *   iTidyIconTest
 *   iTidyIconTest <path>
 *   iTidyIconTest COMPARE
 *   iTidyIconTest <path> COMPARE
 *
 * Default scan root: TestsIcons/test-icons  (relative to the current
 * directory; not a machine-specific drive). Pass an AmigaDOS path to
 * override.
 *
 * COMPARE is optional. It uses GetIconTagList() + IconControlA() as an
 * independent icon.library v44+ oracle. Index-level MATCH still requires
 * identical chunky indexes and palettes. RGB-composite COMPARE maps each
 * side through its own palette so remapped indexes can still match
 * visually. It does not call the iTidy2 extract path (that layer remaps,
 * expands palettes, and may LayoutIconA classic icons). Shared modules
 * are not linked to icon.library.
 *
 * Target: 68000, no FPU, no GUI, no ReAction.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <datatypes/pictureclass.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include <platform/platform.h>
#include <icon/icon_decode.h>

#define DEFAULT_ROOT        "TestsIcons/test-icons"
#define ITIDY_TEST_PATH_MAX 512
#define MAX_FILE_BYTES      (2UL * 1024UL * 1024UL)

/*========================================================================*/
/* CRC32 (IEEE 802.3) — same algorithm as src/tests/test_shared_decode.c  */
/* Regression comparison only; not a cryptographic hash.                  */
/*========================================================================*/

static ULONG crc32_buf(const UBYTE *data, ULONG n)
{
    ULONG crc = 0xFFFFFFFFUL;
    ULONG i;
    int b;

    if (data == NULL)
        return 0;

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
/* String helpers                                                         */
/*========================================================================*/

static const char *yn(BOOL v)
{
    return v ? "yes" : "no";
}

static BOOL ends_with_info(const char *name)
{
    ULONG n;

    if (name == NULL)
        return FALSE;
    n = (ULONG)strlen(name);
    if (n < 5UL)
        return FALSE;
    {
        const char *e = name + (n - 5UL);
        return (e[0] == '.' &&
                (e[1] == 'i' || e[1] == 'I') &&
                (e[2] == 'n' || e[2] == 'N') &&
                (e[3] == 'f' || e[3] == 'F') &&
                (e[4] == 'o' || e[4] == 'O'));
    }
}

static int icmp(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;

    if (a == NULL || b == NULL)
        return (a == b) ? 0 : (a ? 1 : -1);
    do
    {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if (ca >= (unsigned char)'a' && ca <= (unsigned char)'z')
            ca = (unsigned char)(ca - 32U);
        if (cb >= (unsigned char)'a' && cb <= (unsigned char)'z')
            cb = (unsigned char)(cb - 32U);
    } while (ca != 0 && ca == cb);
    return (int)ca - (int)cb;
}

static BOOL is_compare_token(const char *s)
{
    if (s == NULL)
        return FALSE;
    return (icmp(s, "COMPARE") == 0 || icmp(s, "ORACLE") == 0);
}

static BOOL is_help_token(const char *s)
{
    if (s == NULL)
        return FALSE;
    return (s[0] == '?' ||
            icmp(s, "HELP") == 0 ||
            icmp(s, "-H") == 0 ||
            icmp(s, "--HELP") == 0);
}

static const char *src_name(ULONG src)
{
    switch (src)
    {
        case ITIDY_ICON_SRC_COLORICON: return "COLORICON";
        case ITIDY_ICON_SRC_GLOWICON:  return "GLOWICON";
        case ITIDY_ICON_SRC_NEWICONS:  return "NEWICONS";
        case ITIDY_ICON_SRC_CLASSIC:   return "CLASSIC";
        case ITIDY_ICON_SRC_UNKNOWN:   return "UNKNOWN";
        default:                       return "UNKNOWN";
    }
}

static const char *err_token(iTidy_IconError err)
{
    switch (err)
    {
        case ITIDY_ICON_OK:                return "OK";
        case ITIDY_ICON_ERR_NULL:          return "NULL";
        case ITIDY_ICON_ERR_TOO_SMALL:     return "TOO_SMALL";
        case ITIDY_ICON_ERR_BAD_MAGIC:     return "BAD_MAGIC";
        case ITIDY_ICON_ERR_BAD_VERSION:   return "BAD_VERSION";
        case ITIDY_ICON_ERR_TRUNCATED:     return "TRUNCATED";
        case ITIDY_ICON_ERR_OVERFLOW:      return "OVERFLOW";
        case ITIDY_ICON_ERR_BAD_DIMENSION: return "BAD_DIMENSION";
        case ITIDY_ICON_ERR_BAD_COUNT:     return "BAD_COUNT";
        case ITIDY_ICON_ERR_BAD_TEXT:      return "BAD_TEXT";
        case ITIDY_ICON_ERR_UNSUPPORTED:   return "UNSUPPORTED";
        case ITIDY_ICON_ERR_NO_DATA:       return "NO_DATA";
        case ITIDY_ICON_ERR_ALLOC:         return "ALLOC";
        default:                           return "UNKNOWN";
    }
}

static BOOL join_path(char *out, ULONG out_size, const char *dir, const char *name)
{
    ULONG dir_len;
    ULONG name_len;
    ULONG need;
    char sep;

    if (out == NULL || dir == NULL || name == NULL || out_size < 2UL)
        return FALSE;

    dir_len = (ULONG)strlen(dir);
    name_len = (ULONG)strlen(name);
    if (dir_len == 0UL)
    {
        if (name_len + 1UL > out_size)
            return FALSE;
        strcpy(out, name);
        return TRUE;
    }

    sep = 0;
    if (dir[dir_len - 1UL] != ':' && dir[dir_len - 1UL] != '/')
        sep = '/';

    need = dir_len + (sep ? 1UL : 0UL) + name_len + 1UL;
    if (need > out_size)
        return FALSE;

    strcpy(out, dir);
    if (sep)
        strcat(out, "/");
    strcat(out, name);
    return TRUE;
}

/* Strip a trailing .info so GetIconTagList() does not look for .info.info */
static void strip_info_suffix(char *path)
{
    ULONG n;

    if (path == NULL)
        return;
    n = (ULONG)strlen(path);
    if (n >= 5UL && ends_with_info(path))
        path[n - 5UL] = '\0';
}

/*========================================================================*/
/* Counters                                                               */
/*========================================================================*/

typedef struct Stats
{
    ULONG scanned;
    ULONG decoded_ok;
    ULONG expected_unsupported;
    ULONG failures;
    ULONG fmt_classic;
    ULONG fmt_newicons;
    ULONG fmt_coloricon;
    ULONG fmt_glowicon;
    ULONG fmt_unknown;
    BOOL compare;
    ULONG oracle_attempts;
    ULONG oracle_matches;
    ULONG oracle_mismatches;
    ULONG oracle_skipped;
    ULONG oracle_rgb_match;
    ULONG oracle_rgb_mismatch;
    ULONG oracle_rgb_skipped;
    ULONG oracle_rgb_bad_index;
} Stats;

static void stats_note_format(Stats *st, ULONG src)
{
    switch (src)
    {
        case ITIDY_ICON_SRC_CLASSIC:   st->fmt_classic++;   break;
        case ITIDY_ICON_SRC_NEWICONS:  st->fmt_newicons++;  break;
        case ITIDY_ICON_SRC_COLORICON: st->fmt_coloricon++; break;
        case ITIDY_ICON_SRC_GLOWICON:  st->fmt_glowicon++;  break;
        default:                       st->fmt_unknown++;   break;
    }
}

/*========================================================================*/
/* File load                                                              */
/*========================================================================*/

static UBYTE *load_file(const char *path, ULONG *out_size, char *errbuf, ULONG errbuf_size)
{
    BPTR fh;
    LONG size;
    UBYTE *buf;
    LONG got;

    *out_size = 0;
    fh = Open((STRPTR)path, MODE_OLDFILE);
    if (!fh)
    {
        snprintf(errbuf, errbuf_size, "cannot open file (IoErr=%ld)", (long)IoErr());
        return NULL;
    }

    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);
    if (size < 0)
    {
        Close(fh);
        snprintf(errbuf, errbuf_size, "cannot seek file");
        return NULL;
    }
    if (size == 0)
    {
        Close(fh);
        snprintf(errbuf, errbuf_size, "file is empty");
        return NULL;
    }
    if ((ULONG)size > MAX_FILE_BYTES)
    {
        Close(fh);
        snprintf(errbuf, errbuf_size,
                 "file too large (%ld bytes, max %lu)",
                 (long)size, (unsigned long)MAX_FILE_BYTES);
        return NULL;
    }

    buf = (UBYTE *)whd_malloc((ULONG)size);
    if (buf == NULL)
    {
        Close(fh);
        snprintf(errbuf, errbuf_size, "out of memory (%ld bytes)", (long)size);
        return NULL;
    }

    got = Read(fh, buf, size);
    Close(fh);
    if (got != size)
    {
        whd_free(buf);
        snprintf(errbuf, errbuf_size,
                 "short read (%ld of %ld bytes)", (long)got, (long)size);
        return NULL;
    }

    *out_size = (ULONG)size;
    return buf;
}

/*========================================================================*/
/* Image / decode printers                                                */
/*========================================================================*/

static void print_image_block(const char *title, const iTidy_IndexedImage *img,
                              BOOL present, ULONG source_format)
{
    ULONG pix_n;
    ULONG pal_n;

    printf("%s:\n", title);
    printf("  present:        %s\n", yn(present));
    if (!present || img == NULL)
        return;

    printf("  size:           %u x %u\n",
           (unsigned)img->width, (unsigned)img->height);

    if (source_format == ITIDY_ICON_SRC_CLASSIC ||
        img->palette == NULL || img->palette_count == 0)
    {
        if (source_format == ITIDY_ICON_SRC_CLASSIC)
        {
            printf("  palette:        WORKBENCH_PENS\n");
            printf("  palette_crc32:  N/A\n");
        }
        else
        {
            printf("  palette:        0\n");
            printf("  palette_crc32:  N/A\n");
        }
    }
    else
    {
        pal_n = (ULONG)img->palette_count * 3UL;
        printf("  palette:        %u\n", (unsigned)img->palette_count);
        printf("  palette_crc32:  %08lX\n",
               (unsigned long)crc32_buf((const UBYTE *)img->palette, pal_n));
    }

    if (img->transparent_index < 0)
        printf("  transparent:    none\n");
    else
        printf("  transparent:    %ld\n", (long)img->transparent_index);

    pix_n = (ULONG)img->width * (ULONG)img->height;
    if (img->pixels != NULL && pix_n > 0UL)
        printf("  pixel_crc32:    %08lX\n",
               (unsigned long)crc32_buf(img->pixels, pix_n));
    else
        printf("  pixel_crc32:    N/A\n");
}

/*========================================================================*/
/* Optional icon.library oracle                                           */
/*========================================================================*/

static BOOL oracle_library_ok(void)
{
    if (IconBase == NULL)
        return FALSE;
    return (IconBase->lib_Version >= 44);
}

static void print_oracle_skipped(const char *why)
{
    printf("ORACLE:\n");
    printf("  available:      no\n");
    printf("  reason:         %s\n", why);
    printf("ORACLE RESULT: SKIPPED\n");
}

static BOOL rgb_equal(const iTidy_RGB8 *a, const struct ColorRegister *b, ULONG count)
{
    ULONG i;

    if (a == NULL || b == NULL)
        return FALSE;
    for (i = 0; i < count; i++)
    {
        if (a[i].r != b[i].red || a[i].g != b[i].green || a[i].b != b[i].blue)
            return FALSE;
    }
    return TRUE;
}

#define RGBCMP_SKIP         0
#define RGBCMP_MATCH        1
#define RGBCMP_DIFF         2
#define RGBCMP_BAD_INDEX    3

typedef struct RgbCmp
{
    int status;
    const char *detail;
    ULONG pixel;
    ULONG x;
    ULONG y;
    ULONG shared_idx;
    ULONG oracle_idx;
    UBYTE sr;
    UBYTE sg;
    UBYTE sb;
    UBYTE orr;
    UBYTE og;
    UBYTE ob;
    BOOL shared_trans;
    BOOL oracle_trans;
} RgbCmp;

static BOOL mul_ok_rgb(ULONG a, ULONG b, ULONG *out)
{
    if (a != 0 && b > (0xFFFFFFFFUL / a))
        return FALSE;
    *out = a * b;
    return TRUE;
}

static void rgb_cmp_skip(RgbCmp *out, const char *why)
{
    memset(out, 0, sizeof(*out));
    out->status = RGBCMP_SKIP;
    out->detail = why;
}

static const char *rgb_cmp_yn(int status)
{
    if (status == RGBCMP_MATCH)
        return "yes";
    if (status == RGBCMP_SKIP)
        return "n/a";
    return "no";
}

static void print_rgb_mismatch(const char *prefix, const RgbCmp *c)
{
    if (c->status == RGBCMP_SKIP)
    {
        printf("  %s_reason:           %s\n", prefix, c->detail ? c->detail : "n/a");
        return;
    }

    if (c->status == RGBCMP_BAD_INDEX)
    {
        printf("  %s_error:            %s\n", prefix, c->detail);
        printf("  %s_first_bad:        pixel %lu at %lu,%lu\n",
               prefix,
               (unsigned long)c->pixel,
               (unsigned long)c->x,
               (unsigned long)c->y);
        printf("  shared_index:        %lu\n", (unsigned long)c->shared_idx);
        printf("  oracle_index:        %lu\n", (unsigned long)c->oracle_idx);
        return;
    }

    if (c->status != RGBCMP_DIFF)
        return;

    printf("  %s_first_mismatch:   pixel %lu at %lu,%lu\n",
           prefix,
           (unsigned long)c->pixel,
           (unsigned long)c->x,
           (unsigned long)c->y);
    printf("  shared_index:        %lu\n", (unsigned long)c->shared_idx);
    printf("  oracle_index:        %lu\n", (unsigned long)c->oracle_idx);
    if (c->shared_trans || c->oracle_trans)
    {
        printf("  shared_transparent:  %s\n", yn(c->shared_trans));
        printf("  oracle_transparent:  %s\n", yn(c->oracle_trans));
    }
    if (c->detail != NULL && strcmp(c->detail, "transparency mismatch") == 0)
        return;
    printf("  shared_rgb:          %u,%u,%u\n",
           (unsigned)c->sr, (unsigned)c->sg, (unsigned)c->sb);
    printf("  oracle_rgb:          %u,%u,%u\n",
           (unsigned)c->orr, (unsigned)c->og, (unsigned)c->ob);
}

static void rgb_compare_image(const UBYTE *spix, const iTidy_RGB8 *spal,
                              ULONG spal_n, LONG strans,
                              const UBYTE *opix, const struct ColorRegister *opal,
                              ULONG opal_n, LONG otrans,
                              ULONG w, ULONG h, RgbCmp *out)
{
    ULONG n;
    ULONG p;
    ULONG si;
    ULONG oi;
    BOOL s_trans;
    BOOL o_trans;

    memset(out, 0, sizeof(*out));
    if (spix == NULL || spal == NULL || opix == NULL || opal == NULL)
    {
        rgb_cmp_skip(out, "missing pixels or palette");
        return;
    }
    if (w == 0UL || h == 0UL)
    {
        rgb_cmp_skip(out, "zero size");
        return;
    }
    if (!mul_ok_rgb(w, h, &n))
    {
        rgb_cmp_skip(out, "size overflow");
        return;
    }

    for (p = 0; p < n; p++)
    {
        si = (ULONG)spix[p];
        oi = (ULONG)opix[p];
        s_trans = (strans >= 0 && si == (ULONG)strans);
        o_trans = (otrans >= 0 && oi == (ULONG)otrans);

        if (s_trans && o_trans)
            continue;

        if (s_trans != o_trans)
        {
            out->status = RGBCMP_DIFF;
            out->detail = "transparency mismatch";
            out->pixel = p;
            out->x = p % w;
            out->y = p / w;
            out->shared_idx = si;
            out->oracle_idx = oi;
            out->shared_trans = s_trans;
            out->oracle_trans = o_trans;
            return;
        }

        if (si >= spal_n)
        {
            out->status = RGBCMP_BAD_INDEX;
            out->detail = "shared pixel index out of palette range";
            out->pixel = p;
            out->x = p % w;
            out->y = p / w;
            out->shared_idx = si;
            out->oracle_idx = oi;
            return;
        }
        if (oi >= opal_n)
        {
            out->status = RGBCMP_BAD_INDEX;
            out->detail = "oracle pixel index out of palette range";
            out->pixel = p;
            out->x = p % w;
            out->y = p / w;
            out->shared_idx = si;
            out->oracle_idx = oi;
            return;
        }

        if (spal[si].r != opal[oi].red ||
            spal[si].g != opal[oi].green ||
            spal[si].b != opal[oi].blue)
        {
            out->status = RGBCMP_DIFF;
            out->detail = "RGB mismatch";
            out->pixel = p;
            out->x = p % w;
            out->y = p / w;
            out->shared_idx = si;
            out->oracle_idx = oi;
            out->sr = spal[si].r;
            out->sg = spal[si].g;
            out->sb = spal[si].b;
            out->orr = opal[oi].red;
            out->og = opal[oi].green;
            out->ob = opal[oi].blue;
            return;
        }
    }

    out->status = RGBCMP_MATCH;
    out->detail = "ok";
}

static void run_oracle(const char *info_path, const iTidy_DecodedIcon *decoded,
                       iTidy_IconError dec_err, Stats *st)
{
    char load_path[ITIDY_TEST_PATH_MAX];
    struct DiskObject *obj;
    struct TagItem get_tags[3];
    LONG is_palette_mapped = 0;
    ULONG width = 0;
    ULONG height = 0;
    UBYTE *pix1 = NULL;
    UBYTE *pix2 = NULL;
    struct ColorRegister *pal1 = NULL;
    struct ColorRegister *pal2 = NULL;
    ULONG pal_size_1 = 0;
    ULONG pal_size_2 = 0;
    LONG trans1 = -1;
    LONG trans2 = -1;
    LONG has_real_2 = 0;
    BOOL size_match;
    BOOL palette_match;
    BOOL pixels_match;
    BOOL selected_match;
    BOOL trans_match;
    BOOL both_selected;
    BOOL selected_size_ok;
    BOOL selected_index_match;
    BOOL selected_palette_match;
    const char *sel_index_s;
    const char *sel_pal_s;
    const struct ColorRegister *opal_sel;
    ULONG opal_sel_n;
    LONG otrans_sel;
    RgbCmp rgb_n;
    RgbCmp rgb_s;
    ULONG pix_n;

    st->oracle_attempts++;

    if (dec_err != ITIDY_ICON_OK)
    {
        print_oracle_skipped("shared decode did not succeed");
        st->oracle_skipped++;
        return;
    }

    if (!oracle_library_ok())
    {
        print_oracle_skipped("icon.library v44+ not available");
        st->oracle_skipped++;
        return;
    }

    /* Classic output is Workbench pens; icon.library palette-mapped
     * extraction (and LayoutIconA) is a different representation. */
    if (decoded->source_format == ITIDY_ICON_SRC_CLASSIC)
    {
        print_oracle_skipped("CLASSIC uses Workbench pens; not compared");
        st->oracle_skipped++;
        return;
    }

    if (strlen(info_path) + 1UL > ITIDY_TEST_PATH_MAX)
    {
        print_oracle_skipped("path too long");
        st->oracle_skipped++;
        return;
    }
    strcpy(load_path, info_path);
    strip_info_suffix(load_path);

    get_tags[0].ti_Tag  = ICONGETA_RemapIcon;
    get_tags[0].ti_Data = FALSE;
    get_tags[1].ti_Tag  = ICONGETA_GenerateImageMasks;
    get_tags[1].ti_Data = FALSE;
    get_tags[2].ti_Tag  = TAG_DONE;
    get_tags[2].ti_Data = 0;

    obj = GetIconTagList((STRPTR)load_path, get_tags);
    if (obj == NULL)
    {
        print_oracle_skipped("GetIconTagList failed");
        st->oracle_skipped++;
        return;
    }

    {
        struct TagItem pm_tags[2];
        pm_tags[0].ti_Tag  = ICONCTRLA_IsPaletteMapped;
        pm_tags[0].ti_Data = (ULONG)&is_palette_mapped;
        pm_tags[1].ti_Tag  = TAG_DONE;
        pm_tags[1].ti_Data = 0;
        IconControlA(obj, pm_tags);
    }

    if (!is_palette_mapped)
    {
        FreeDiskObject(obj);
        print_oracle_skipped("icon.library reports not palette-mapped");
        st->oracle_skipped++;
        return;
    }

    {
        struct TagItem q[16];
        int t = 0;

        q[t].ti_Tag  = ICONCTRLA_GetWidth;
        q[t].ti_Data = (ULONG)&width;
        t++;
        q[t].ti_Tag  = ICONCTRLA_GetHeight;
        q[t].ti_Data = (ULONG)&height;
        t++;
        q[t].ti_Tag  = ICONCTRLA_GetImageData1;
        q[t].ti_Data = (ULONG)&pix1;
        t++;
        q[t].ti_Tag  = ICONCTRLA_GetImageData2;
        q[t].ti_Data = (ULONG)&pix2;
        t++;
        q[t].ti_Tag  = ICONCTRLA_GetPalette1;
        q[t].ti_Data = (ULONG)&pal1;
        t++;
        q[t].ti_Tag  = ICONCTRLA_GetPalette2;
        q[t].ti_Data = (ULONG)&pal2;
        t++;
        q[t].ti_Tag  = ICONCTRLA_GetPaletteSize1;
        q[t].ti_Data = (ULONG)&pal_size_1;
        t++;
        q[t].ti_Tag  = ICONCTRLA_GetPaletteSize2;
        q[t].ti_Data = (ULONG)&pal_size_2;
        t++;
        q[t].ti_Tag  = ICONCTRLA_GetTransparentColor1;
        q[t].ti_Data = (ULONG)&trans1;
        t++;
        q[t].ti_Tag  = ICONCTRLA_GetTransparentColor2;
        q[t].ti_Data = (ULONG)&trans2;
        t++;
        q[t].ti_Tag  = ICONCTRLA_HasRealImage2;
        q[t].ti_Data = (ULONG)&has_real_2;
        t++;
        q[t].ti_Tag  = TAG_DONE;
        q[t].ti_Data = 0;
        IconControlA(obj, q);
    }

    pix_n = (ULONG)decoded->normal.width * (ULONG)decoded->normal.height;
    size_match = (width == (ULONG)decoded->normal.width &&
                  height == (ULONG)decoded->normal.height);

    palette_match = FALSE;
    if (decoded->normal.palette != NULL && pal1 != NULL &&
        pal_size_1 == (ULONG)decoded->normal.palette_count)
    {
        palette_match = rgb_equal(decoded->normal.palette, pal1, pal_size_1);
    }
    else if (decoded->normal.palette == NULL && pal_size_1 == 0)
        palette_match = TRUE;

    pixels_match = FALSE;
    if (size_match && pix1 != NULL && decoded->normal.pixels != NULL && pix_n > 0UL)
        pixels_match = (memcmp(pix1, decoded->normal.pixels, (size_t)pix_n) == 0);

    trans_match = (trans1 == decoded->normal.transparent_index);

    both_selected = ((has_real_2 != 0) && decoded->has_selected);
    selected_size_ok = FALSE;
    selected_index_match = FALSE;
    selected_palette_match = FALSE;
    sel_index_s = "n/a";
    sel_pal_s = "n/a";
    opal_sel = pal1;
    opal_sel_n = pal_size_1;
    otrans_sel = trans1;
    if (pal2 != NULL && pal_size_2 > 0UL)
    {
        opal_sel = pal2;
        opal_sel_n = pal_size_2;
        otrans_sel = trans2;
    }

    selected_match = TRUE;
    if ((has_real_2 != 0) != (decoded->has_selected ? 1 : 0))
    {
        selected_match = FALSE;
        sel_index_s = "no";
    }
    else if (decoded->has_selected)
    {
        ULONG sel_n;

        sel_n = (ULONG)decoded->selected.width * (ULONG)decoded->selected.height;
        selected_size_ok = ((ULONG)decoded->selected.width == width &&
                            (ULONG)decoded->selected.height == height);
        if (decoded->selected.width != decoded->normal.width ||
            decoded->selected.height != decoded->normal.height)
        {
            /* icon.library exposes one width/height; selected size differs */
            selected_match = FALSE;
            selected_size_ok = FALSE;
        }
        else if (pix2 == NULL || decoded->selected.pixels == NULL)
            selected_match = FALSE;
        else if (memcmp(pix2, decoded->selected.pixels, (size_t)sel_n) != 0)
            selected_match = FALSE;
        else
            selected_index_match = TRUE;

        if (decoded->selected.palette != NULL && opal_sel != NULL &&
            opal_sel_n == (ULONG)decoded->selected.palette_count &&
            rgb_equal(decoded->selected.palette, opal_sel, opal_sel_n))
            selected_palette_match = TRUE;
        else if (decoded->selected.palette == NULL && opal_sel_n == 0)
            selected_palette_match = TRUE;

        if (decoded->selected.palette != NULL && pal2 != NULL)
        {
            if (pal_size_2 != (ULONG)decoded->selected.palette_count ||
                !rgb_equal(decoded->selected.palette, pal2, pal_size_2))
                selected_match = FALSE;
        }
        if (trans2 != decoded->selected.transparent_index)
            selected_match = FALSE;

        sel_index_s = yn(selected_index_match);
        sel_pal_s = yn(selected_palette_match);
    }

    rgb_cmp_skip(&rgb_n, "not compared");
    rgb_cmp_skip(&rgb_s, "no selected image");
    if (!size_match)
        rgb_cmp_skip(&rgb_n, "size mismatch");
    else
        rgb_compare_image(decoded->normal.pixels, decoded->normal.palette,
                          (ULONG)decoded->normal.palette_count,
                          decoded->normal.transparent_index,
                          pix1, pal1, pal_size_1, trans1,
                          width, height, &rgb_n);

    if (both_selected)
    {
        if (!selected_size_ok)
            rgb_cmp_skip(&rgb_s, "selected size mismatch");
        else
            rgb_compare_image(decoded->selected.pixels, decoded->selected.palette,
                              (ULONG)decoded->selected.palette_count,
                              decoded->selected.transparent_index,
                              pix2, opal_sel, opal_sel_n, otrans_sel,
                              width, height, &rgb_s);
    }
    else if ((has_real_2 != 0) || decoded->has_selected)
        rgb_cmp_skip(&rgb_s, "selected image not present on both sides");

    printf("ORACLE:\n");
    printf("  available:               yes\n");
    printf("  size_match:              %s  (lib %lu x %lu)\n",
           yn(size_match), (unsigned long)width, (unsigned long)height);
    printf("  index_pixels_match:      %s\n", yn(pixels_match));
    printf("  palette_match:           %s  (lib %lu colours)\n",
           yn(palette_match), (unsigned long)pal_size_1);
    printf("  trans_match:             %s  (lib %ld)\n", yn(trans_match), (long)trans1);
    printf("  rgb_composite_match:     %s\n", rgb_cmp_yn(rgb_n.status));
    if (rgb_n.status != RGBCMP_MATCH)
    {
        print_rgb_mismatch("rgb", &rgb_n);
        if (decoded->source_format == ITIDY_ICON_SRC_NEWICONS &&
            rgb_n.status == RGBCMP_DIFF)
        {
            printf("  newicons_image:      IM1\n");
            printf("  newicons_phase:      pixels (RGB composite)\n");
            printf("  newicons_sample:     %lu\n", (unsigned long)rgb_n.pixel);
        }
    }
    printf("  selected_match:          %s  (lib has_image2=%s)\n",
           yn(selected_match), yn(has_real_2 != 0));
    printf("  selected_index_match:    %s\n", sel_index_s);
    printf("  selected_palette_match:  %s\n", sel_pal_s);
    printf("  selected_rgb_match:      %s\n", rgb_cmp_yn(rgb_s.status));
    if (rgb_s.status != RGBCMP_MATCH &&
        (both_selected || has_real_2 != 0 || decoded->has_selected))
    {
        print_rgb_mismatch("selected_rgb", &rgb_s);
        if (decoded->source_format == ITIDY_ICON_SRC_NEWICONS &&
            rgb_s.status == RGBCMP_DIFF)
        {
            printf("  newicons_image:      IM2\n");
            printf("  newicons_phase:      pixels (RGB composite)\n");
            printf("  newicons_sample:     %lu\n", (unsigned long)rgb_s.pixel);
        }
    }

    if (size_match && palette_match && pixels_match && trans_match && selected_match)
    {
        printf("ORACLE RESULT: MATCH\n");
        st->oracle_matches++;
    }
    else
    {
        printf("ORACLE RESULT: MISMATCH\n");
        st->oracle_mismatches++;
    }

    if (rgb_n.status == RGBCMP_SKIP)
    {
        printf("ORACLE RGB: N/A\n");
        st->oracle_rgb_skipped++;
    }
    else if (rgb_n.status == RGBCMP_BAD_INDEX || rgb_s.status == RGBCMP_BAD_INDEX)
    {
        printf("ORACLE RGB: FAIL (invalid palette index)\n");
        st->oracle_rgb_bad_index++;
        st->oracle_rgb_mismatch++;
    }
    else if (rgb_n.status == RGBCMP_DIFF || rgb_s.status == RGBCMP_DIFF)
    {
        printf("ORACLE RGB: MISMATCH\n");
        st->oracle_rgb_mismatch++;
    }
    else if (rgb_n.status == RGBCMP_MATCH &&
             (rgb_s.status == RGBCMP_MATCH || !both_selected))
    {
        printf("ORACLE RGB: MATCH\n");
        st->oracle_rgb_match++;
    }
    else
    {
        printf("ORACLE RGB: N/A\n");
        st->oracle_rgb_skipped++;
    }

    FreeDiskObject(obj);
}

/*========================================================================*/
/* Per-file processing                                                    */
/*========================================================================*/

static void process_info(const char *path, Stats *st)
{
    UBYTE *data;
    ULONG size;
    char errbuf[128];
    iTidy_IconProbe probe;
    iTidy_IconError probe_err;
    iTidy_DecodedIcon decoded;
    iTidy_IconError dec_err;
    const char *status;

    st->scanned++;

    printf("------------------------------------------------------------\n");
    printf("FILE: %s\n", path);
    printf("------------------------------------------------------------\n");

    data = load_file(path, &size, errbuf, (ULONG)sizeof(errbuf));
    if (data == NULL)
    {
        printf("PROBE:\n");
        printf("  result:         FAIL (read error)\n");
        printf("DECODE:\n");
        printf("  result:         FAIL\n");
        printf("STATUS: FAIL\n");
        printf("ERROR: %s\n\n", errbuf);
        st->failures++;
        return;
    }

    memset(&probe, 0, sizeof(probe));
    probe_err = icon_probe_buffer(data, size, &probe);

    printf("PROBE:\n");
    if (probe_err != ITIDY_ICON_OK)
    {
        printf("  result:         %s (%d) %s\n",
               err_token(probe_err), (int)probe_err, icon_error_string(probe_err));
    }
    else
    {
        printf("  result:         OK\n");
    }
    printf("  classic:        %s\n", yn(probe.has_classic));
    printf("  newicons:       %s\n", yn(probe.has_newicons));
    printf("  coloricon:      %s\n", yn(probe.has_coloricon));
    printf("  glowicon:       %s\n", yn(probe.has_glowicon));
    printf("  png:            %s\n", yn(probe.has_png));
    printf("  argb:           %s\n", yn(probe.has_os4_argb));
    printf("  unsupported:    %s\n", yn(probe.has_unsupported));
    printf("  normal:         %s\n", yn(probe.has_normal));
    printf("  selected:       %s\n", yn(probe.has_selected));

    memset(&decoded, 0, sizeof(decoded));
    dec_err = icon_decode_buffer(data, size, ITIDY_ICON_REQ_BEST, &decoded);

    printf("DECODE:\n");
    printf("  result:         %s (%d) %s\n",
           err_token(dec_err), (int)dec_err, icon_error_string(dec_err));

    if (dec_err == ITIDY_ICON_OK)
    {
        printf("  source:         %s (%lu)\n",
               src_name(decoded.source_format),
               (unsigned long)decoded.source_format);
        printf("  frameless:      %s\n", yn(decoded.frameless));

        print_image_block("NORMAL", &decoded.normal, TRUE, decoded.source_format);
        print_image_block("SELECTED", &decoded.selected, decoded.has_selected,
                          decoded.source_format);

        status = "PASS";
        st->decoded_ok++;
        stats_note_format(st, decoded.source_format);
    }
    else if (dec_err == ITIDY_ICON_ERR_UNSUPPORTED)
    {
        printf("  source:         (none)\n");
        status = "EXPECTED UNSUPPORTED";
        st->expected_unsupported++;
    }
    else
    {
        printf("  source:         (none)\n");
        status = "FAIL";
        st->failures++;
    }

    if (st->compare)
        run_oracle(path, &decoded, dec_err, st);

    printf("STATUS: %s\n", status);
    if (dec_err != ITIDY_ICON_OK && dec_err != ITIDY_ICON_ERR_UNSUPPORTED)
    {
        printf("ERROR: %s (%d) %s\n",
               err_token(dec_err), (int)dec_err, icon_error_string(dec_err));
    }
    printf("\n");
    fflush(stdout);

    icon_decoded_free(&decoded);
    whd_free(data);
}

/*========================================================================*/
/* Recursive scan (Examine / ExNext, WB 2.x+)                             */
/*========================================================================*/

static BOOL scan_path(const char *path, Stats *st);

static BOOL scan_directory(const char *path, BPTR lock, struct FileInfoBlock *fib,
                           Stats *st)
{
    LONG result;

    while ((result = ExNext(lock, fib)) != 0)
    {
        char child[ITIDY_TEST_PATH_MAX];

        if (fib->fib_DirEntryType > 0)
        {
            if (fib->fib_FileName[0] == '.')
                continue;
            if (!join_path(child, ITIDY_TEST_PATH_MAX, path, fib->fib_FileName))
            {
                printf("------------------------------------------------------------\n");
                printf("FILE: %s/%s\n", path, fib->fib_FileName);
                printf("------------------------------------------------------------\n");
                printf("STATUS: FAIL\n");
                printf("ERROR: path too long\n\n");
                st->scanned++;
                st->failures++;
                continue;
            }
            scan_path(child, st);
        }
        else if (fib->fib_DirEntryType < 0)
        {
            if (!ends_with_info(fib->fib_FileName))
                continue;
            if (!join_path(child, ITIDY_TEST_PATH_MAX, path, fib->fib_FileName))
            {
                printf("------------------------------------------------------------\n");
                printf("FILE: %s/%s\n", path, fib->fib_FileName);
                printf("------------------------------------------------------------\n");
                printf("STATUS: FAIL\n");
                printf("ERROR: path too long\n\n");
                st->scanned++;
                st->failures++;
                continue;
            }
            process_info(child, st);
        }
    }

    return TRUE;
}

static BOOL scan_path(const char *path, Stats *st)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    BOOL ok = TRUE;

    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        printf("ERROR: cannot lock '%s' (IoErr=%ld)\n", path, (long)IoErr());
        return FALSE;
    }

    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL)
    {
        UnLock(lock);
        printf("ERROR: AllocDosObject(DOS_FIB) failed\n");
        return FALSE;
    }

    if (!Examine(lock, fib))
    {
        printf("ERROR: Examine failed for '%s' (IoErr=%ld)\n",
               path, (long)IoErr());
        ok = FALSE;
    }
    else if (fib->fib_DirEntryType < 0)
    {
        if (ends_with_info(fib->fib_FileName) || ends_with_info(path))
            process_info(path, st);
        else
        {
            printf("ERROR: '%s' is not a .info file\n", path);
            ok = FALSE;
        }
    }
    else
    {
        ok = scan_directory(path, lock, fib, st);
    }

    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    return ok;
}

/*========================================================================*/
/* Summary / usage                                                        */
/*========================================================================*/

static void print_usage(void)
{
    printf("iTidyIconTest — shared icon decoder validation (Amiga CLI)\n");
    printf("\n");
    printf("Usage:\n");
    printf("  iTidyIconTest\n");
    printf("  iTidyIconTest <path>\n");
    printf("  iTidyIconTest COMPARE\n");
    printf("  iTidyIconTest <path> COMPARE\n");
    printf("\n");
    printf("Default path: %s\n", DEFAULT_ROOT);
    printf("<path> may be a directory (scanned recursively) or a single .info file.\n");
    printf("COMPARE: optional icon.library v44+ oracle (GetIconTagList + IconControlA).\n");
    printf("         Index MATCH requires identical chunky indexes and palettes.\n");
    printf("         RGB-composite COMPARE maps each side through its own palette.\n");
    printf("         Not used for CLASSIC (Workbench pens). Shared code is not linked\n");
    printf("         to icon.library.\n");
}

static void print_summary(const Stats *st)
{
    printf("============================================================\n");
    printf("iTidy Shared Icon Decoder Validation Summary\n");
    printf("============================================================\n");
    printf("Files scanned:              %lu\n", (unsigned long)st->scanned);
    printf("Decoded successfully:       %lu\n", (unsigned long)st->decoded_ok);
    printf("Expected unsupported:       %lu\n", (unsigned long)st->expected_unsupported);
    printf("Failures:                   %lu\n", (unsigned long)st->failures);
    printf("\n");
    printf("Formats decoded:\n");
    printf("  Classic:                  %lu\n", (unsigned long)st->fmt_classic);
    printf("  NewIcons:                 %lu\n", (unsigned long)st->fmt_newicons);
    printf("  ColorIcon:                %lu\n", (unsigned long)st->fmt_coloricon);
    printf("  GlowIcon:                 %lu\n", (unsigned long)st->fmt_glowicon);
    if (st->fmt_unknown)
        printf("  Unknown:                  %lu\n", (unsigned long)st->fmt_unknown);
    printf("\n");
    printf("Unsupported:\n");
    printf("  PNG/ARGB/other:           %lu\n", (unsigned long)st->expected_unsupported);

    if (st->compare)
    {
        printf("\n");
        printf("Oracle (icon.library v44+):\n");
        printf("  Attempts:                 %lu\n", (unsigned long)st->oracle_attempts);
        printf("  Matches:                  %lu\n", (unsigned long)st->oracle_matches);
        printf("  Mismatches:               %lu\n", (unsigned long)st->oracle_mismatches);
        printf("  Skipped:                  %lu\n", (unsigned long)st->oracle_skipped);
        printf("  RGB composite matches:    %lu\n", (unsigned long)st->oracle_rgb_match);
        printf("  RGB composite mismatches: %lu\n", (unsigned long)st->oracle_rgb_mismatch);
        printf("  RGB composite skipped:    %lu\n", (unsigned long)st->oracle_rgb_skipped);
        printf("  RGB invalid palette idx:  %lu\n", (unsigned long)st->oracle_rgb_bad_index);
    }

    printf("\n");
    if (st->failures == 0)
        printf("RESULT: PASS\n");
    else
        printf("RESULT: FAIL\n");
    printf("============================================================\n");
}

/*========================================================================*/
/* main                                                                   */
/*========================================================================*/

int main(int argc, char **argv)
{
    const char *root = DEFAULT_ROOT;
    Stats st;
    int i;
    BOOL scanned_ok;

    memset(&st, 0, sizeof(st));

    for (i = 1; i < argc; i++)
    {
        if (is_help_token(argv[i]))
        {
            print_usage();
            return RETURN_OK;
        }
        if (is_compare_token(argv[i]))
        {
            st.compare = TRUE;
            continue;
        }
        root = argv[i];
    }

    printf("iTidyIconTest — shared icon decoder validation\n");
    printf("Scan root: %s\n", root);
    printf("Request:   BEST (ColorIcon/GlowIcon, then NewIcons, then classic)\n");
    printf("Oracle:    %s\n", st.compare ? "COMPARE (icon.library v44+ + RGB composite)" : "off");
    printf("\n");
    fflush(stdout);

    scanned_ok = scan_path(root, &st);
    if (!scanned_ok && st.scanned == 0)
    {
        printf("\nNo icons processed. Pass an AmigaDOS path to the test root, e.g.:\n");
        printf("  iTidyIconTest TestsIcons:test-icons\n");
        printf("  iTidyIconTest TestsIcons/test-icons\n");
        print_summary(&st);
        return RETURN_FAIL;
    }

    print_summary(&st);
    return (st.failures == 0) ? RETURN_OK : RETURN_FAIL;
}
