/*
 * test_shared_image.c - Host regression tests for shared image kernels
 *
 * Compile/run via: make test-image
 * Does not link Amiga libraries. Uses src/tests/host_stubs for exec types.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <image/image_types.h>
#include <image/image_scale.h>
#include <image/image_palette.h>
#include <image/image_dither.h>

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

static void expect_mem(const char *name, const void *got, const void *want, unsigned n)
{
    if (memcmp(got, want, n) != 0)
        fail(name, "buffer mismatch");
    else
        pass(name);
}

static void test_manhattan_and_nearest(void)
{
    iTidy_RGB8 pal[3];

    pal[0].r = 0;   pal[0].g = 0;   pal[0].b = 0;
    pal[1].r = 255; pal[1].g = 0;   pal[1].b = 0;
    pal[2].r = 0;   pal[2].g = 255; pal[2].b = 0;

    expect_eq_u("manhattan black-white",
                image_palette_manhattan_distance(0, 0, 0, 255, 255, 255), 765);
    expect_eq_u("manhattan identical",
                image_palette_manhattan_distance(10, 20, 30, 10, 20, 30), 0);
    expect_eq_u("nearest (128,0,0) -> red",
                image_palette_find_nearest(pal, 3, 128, 0, 0), 1);

    pal[1].r = 0; pal[1].g = 0; pal[1].b = 0; /* duplicate of pal[0] */
    expect_eq_u("nearest tie broken by first",
                image_palette_find_nearest(pal, 2, 0, 0, 0), 0);
}

static void test_rgb24_scale(void)
{
    UBYTE src[2 * 2 * 3] = {
        10, 20, 30,  40, 50, 60,
        70, 80, 90, 100, 110, 120
    };
    UBYTE dest[3];
    UBYTE want[3] = { 55, 65, 75 };

    image_scale_rgb24(src, 2, 2, dest, 1, 1, NULL);
    expect_mem("rgb24 2x2 -> 1x1 average", dest, want, 3);
}

static void test_rgb24_nonsquare_scale(void)
{
    /* 4x2 -> 2x1: independent X/Y scale factors */
    UBYTE src[4 * 2 * 3] = {
        0, 0, 0,  20, 0, 0,  40, 0, 0,  60, 0, 0,
        0, 10, 0, 20, 10, 0, 40, 10, 0, 60, 10, 0
    };
    UBYTE dest[2 * 1 * 3];
    UBYTE want[6] = { 10, 5, 0,  50, 5, 0 };

    image_scale_rgb24(src, 4, 2, dest, 2, 1, NULL);
    expect_mem("rgb24 4x2 -> 2x1 nonsquare", dest, want, 6);
}

static void test_indexed_scale(void)
{
    iTidy_RGB8 pal[4];
    UBYTE src_same[4] = { 3, 3, 3, 3 };
    UBYTE dest_same[1];
    UBYTE src_mix[4] = { 0, 0, 0, 1 };
    UBYTE dest_mix[1];

    pal[0].r = 0;   pal[0].g = 0;   pal[0].b = 0;
    pal[1].r = 255; pal[1].g = 255; pal[1].b = 255;
    pal[2].r = 128; pal[2].g = 0;   pal[2].b = 0;
    pal[3].r = 0;   pal[3].g = 0;   pal[3].b = 128;

    image_scale_indexed(src_same, 2, 2, dest_same, 1, 1, 1, 0, 0,
                        pal, 4, pal, 4, NULL);
    expect_eq_u("indexed all-same fast path", dest_same[0], 3);

    image_scale_indexed(src_mix, 2, 2, dest_mix, 1, 1, 1, 0, 0,
                        pal, 4, pal, 4, NULL);
    expect_eq_u("indexed mixed average -> black", dest_mix[0], 0);
}

static void test_prefilter(void)
{
    iTidy_RGB8 pal[1];
    UBYTE src[4] = { 0, 0, 0, 0 };
    UBYTE *out = NULL;
    UWORD w = 0, h = 0;

    pal[0].r = 10; pal[0].g = 20; pal[0].b = 30;

    if (!image_prefilter_2x2(src, 2, 2, &out, &w, &h, pal, 1))
    {
        fail("prefilter_2x2", "returned FALSE");
        return;
    }
    expect_eq_u("prefilter size w", w, 1);
    expect_eq_u("prefilter size h", h, 1);
    expect_eq_u("prefilter pixel", out[0], 0);
    free(out);
}

static void test_4colour_mapping(void)
{
    iTidy_RGB8 old_pal[4];
    iTidy_RGB8 new_pal[4];
    UBYTE pixels[4] = { 0, 1, 2, 3 };

    old_pal[0].r = 40;  old_pal[0].g = 40;  old_pal[0].b = 40;
    old_pal[1].r = 100; old_pal[1].g = 100; old_pal[1].b = 100;
    old_pal[2].r = 200; old_pal[2].g = 200; old_pal[2].b = 200;
    old_pal[3].r = 250; old_pal[3].g = 250; old_pal[3].b = 250;

    new_pal[0].r = 0;   new_pal[0].g = 0;   new_pal[0].b = 0;
    new_pal[1].r = 85;  new_pal[1].g = 85;  new_pal[1].b = 85;
    new_pal[2].r = 170; new_pal[2].g = 170; new_pal[2].b = 170;
    new_pal[3].r = 255; new_pal[3].g = 255; new_pal[3].b = 255;

    image_palette_remap(pixels, 2, 2, old_pal, 4, new_pal, 4, ITIDY_DITHER_NONE);
    expect_eq_u("4-colour map px0", pixels[0], 0);
    expect_eq_u("4-colour map px1", pixels[1], 1);
    expect_eq_u("4-colour map px2", pixels[2], 2);
    expect_eq_u("4-colour map px3", pixels[3], 3);
}

static void test_bayer_dither(void)
{
    iTidy_RGB8 old_pal[1];
    iTidy_RGB8 new_pal[2];
    UBYTE pixels[2];

    expect_eq_u("bayer (0,0)", (unsigned)(WORD)image_dither_bayer_offset(0, 0),
                (unsigned)(WORD)(-7));
    expect_eq_u("bayer (1,0)", (unsigned)image_dither_bayer_offset(1, 0), 1);
    expect_eq_u("bayer (0,1)", (unsigned)image_dither_bayer_offset(0, 1), 5);
    expect_eq_u("bayer (1,1)", (unsigned)(WORD)image_dither_bayer_offset(1, 1),
                (unsigned)(WORD)(-3));

    old_pal[0].r = 128; old_pal[0].g = 128; old_pal[0].b = 128;
    new_pal[0].r = 0;   new_pal[0].g = 0;   new_pal[0].b = 0;
    new_pal[1].r = 255; new_pal[1].g = 255; new_pal[1].b = 255;

    pixels[0] = 0;
    pixels[1] = 0;
    image_palette_remap(pixels, 2, 1, old_pal, 1, new_pal, 2, ITIDY_DITHER_ORDERED);
    expect_eq_u("ordered dither px(0,0)", pixels[0], 0);
    expect_eq_u("ordered dither px(1,0)", pixels[1], 1);
}

static void test_floyd_steinberg(void)
{
    iTidy_RGB8 old_pal[1];
    iTidy_RGB8 new_pal[2];
    UBYTE one[1] = { 0 };
    UBYTE a[4];
    UBYTE b[4];
    UBYTE nearest[4];
    unsigned i;
    BOOL ok;

    old_pal[0].r = 100; old_pal[0].g = 0; old_pal[0].b = 0;
    new_pal[0].r = 0;   new_pal[0].g = 0; new_pal[0].b = 0;
    new_pal[1].r = 255; new_pal[1].g = 0; new_pal[1].b = 0;

    ok = image_dither_floyd_steinberg(one, 1, 1, old_pal, 1, new_pal, 2);
    if (!ok)
        fail("FS 1x1", "returned FALSE");
    else
        expect_eq_u("FS 1x1 equals nearest", one[0], 0);

    /* Mid-grey field mapped to black/red: nearest is uniform; FS should mix. */
    old_pal[0].r = 128; old_pal[0].g = 128; old_pal[0].b = 128;
    a[0] = a[1] = a[2] = a[3] = 0;
    b[0] = b[1] = b[2] = b[3] = 0;
    nearest[0] = nearest[1] = nearest[2] = nearest[3] = 0;
    image_palette_remap(nearest, 2, 2, old_pal, 1, new_pal, 2, ITIDY_DITHER_NONE);

    ok = image_dither_floyd_steinberg(a, 2, 2, old_pal, 1, new_pal, 2);
    if (!ok)
    {
        fail("FS 2x2", "returned FALSE");
        return;
    }

    ok = image_dither_floyd_steinberg(b, 2, 2, old_pal, 1, new_pal, 2);
    if (!ok || memcmp(a, b, 4) != 0)
        fail("FS 2x2 deterministic", "repeat mismatch");
    else
        pass("FS 2x2 deterministic");

    for (i = 0; i < 4; i++)
    {
        if (a[i] > 1)
        {
            fail("FS 2x2 index range", "index out of palette");
            return;
        }
    }
    pass("FS 2x2 index range");

    /* Error diffusion should not always match pure nearest-colour. */
    if (memcmp(a, nearest, 4) == 0)
        fail("FS 2x2 differs from nearest", "identical to no-dither remap");
    else
        pass("FS 2x2 differs from nearest");
}

static void test_higher_colour_median_cut(void)
{
    iTidy_RGB8 src_pal[16];
    iTidy_RGB8 out_pal[8];
    UBYTE pixels[16];
    UWORD out_n = 0;
    UWORD i;
    BOOL ok;

    for (i = 0; i < 16; i++)
    {
        src_pal[i].r = (UBYTE)(i * 17);
        src_pal[i].g = (UBYTE)(255 - i * 17);
        src_pal[i].b = (UBYTE)(i * 8);
        pixels[i] = (UBYTE)i;
    }

    ok = image_quantize_palette(pixels, 16, src_pal, 16, 8, out_pal, &out_n);
    if (!ok)
        fail("median cut 16->8", "returned FALSE");
    else
        expect_eq_u("median cut 16->8 count", out_n, 8);

    expect_eq_u("unique count 16", image_palette_count_unique(pixels, 16), 16);
}

static void test_cube_quant(void)
{
    UBYTE rgb[9] = { 0, 0, 0,  255, 255, 255,  51, 0, 0 };
    UBYTE out[3];

    image_quantize_rgb24_to_cube(rgb, out, 3, 216);
    expect_eq_u("cube black", out[0], 0);
    expect_eq_u("cube white", out[1], 215);
    expect_eq_u("cube (51,0,0)", out[2], 36);
}

static void test_auto_select(void)
{
    expect_eq_u("auto 4 -> floyd", image_dither_auto_select(4), ITIDY_DITHER_FLOYD);
    expect_eq_u("auto 16 -> ordered", image_dither_auto_select(16), ITIDY_DITHER_ORDERED);
    expect_eq_u("auto 29 -> floyd", image_dither_auto_select(29), ITIDY_DITHER_FLOYD);
    expect_eq_u("auto 64 -> none", image_dither_auto_select(64), ITIDY_DITHER_NONE);
}

int main(void)
{
    test_manhattan_and_nearest();
    test_rgb24_scale();
    test_rgb24_nonsquare_scale();
    test_indexed_scale();
    test_prefilter();
    test_4colour_mapping();
    test_bayer_dither();
    test_floyd_steinberg();
    test_higher_colour_median_cut();
    test_cube_quant();
    test_auto_select();

    if (g_failures)
    {
        printf("\n%d test(s) failed\n", g_failures);
        return 1;
    }

    printf("\nAll shared image tests passed\n");
    return 0;
}
