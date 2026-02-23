/*
 * palette_harmonised.c - GlowIcons Harmonised Palette
 *
 * See palette_harmonised.h for API documentation.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include "palette_harmonised.h"
#include "palette_mapping.h"
#include "palette_dithering.h"
#include "../../writeLog.h"
#include "../../platform/platform.h"  /* whd_malloc, whd_free */

/*========================================================================*/
/* GlowIcons Harmonised Palette Data                                      */
/*========================================================================*/

/* AmigaOS icon harmonised palette (from UI Style Guide palette image).
 * 29 entries ordered: neutrals, blues, warm browns/oranges, yellows,
 * reds, greens, purples. */
static const struct ColorRegister harmonised_palette_data[ITIDY_HARMONISED_PALETTE_SIZE] =
{
    /* neutrals */
    {255, 255, 255},
    {127, 127, 127},
    {  0,   0,   0},

    /* blues */
    {160, 190, 250},
    { 80, 110, 170},
    {  0,  30,  90},

    /* warm browns / oranges */
    {250, 190, 130},
    {170, 110,  50},
    {110,  60,   0},
    {140,  60,  20},
    {230, 170, 110},
    {200, 150,  50},

    /* yellows */
    {250, 250, 150},
    {250, 250,  50},
    {250, 200,   0},
    {236, 186,   0},
    {200, 150,   0},

    /* reds */
    {250, 160, 160},
    {230, 140, 140},
    {170,  80,  80},
    {100,   0,   0},
    { 90,   0,   0},

    /* greens */
    {160, 250, 190},
    { 80, 170, 110},
    {  0,  90,  30},

    /* purples */
    {250, 190, 250},
    {230, 170, 230},
    {170, 110, 170},
    { 90,  30,  90}
};

/*========================================================================*/
/* itidy_harmonised_palette                                               */
/*========================================================================*/

void itidy_harmonised_palette(struct ColorRegister *out_palette)
{
    UWORD i;

    for (i = 0; i < ITIDY_HARMONISED_PALETTE_SIZE; i++)
    {
        out_palette[i] = harmonised_palette_data[i];
    }
}

/*========================================================================*/
/* itidy_apply_harmonised_palette                                         */
/*========================================================================*/

BOOL itidy_apply_harmonised_palette(iTidy_IconImageData *img,
                                    UWORD dither_method)
{
    struct ColorRegister harm_pal[ITIDY_HARMONISED_PALETTE_SIZE];
    UWORD actual_dither;
    UWORD i;

    if (img == NULL || img->pixel_data_normal == NULL ||
        img->palette_normal == NULL)
    {
        return FALSE;
    }

    /* Build the fixed harmonised palette */
    itidy_harmonised_palette(harm_pal);

    /* Resolve Auto dither to a concrete method.
     * 29 colours is in the 32-colour tier -> Ordered dithering by default. */
    if (dither_method == ITIDY_DITHER_AUTO)
        actual_dither = itidy_dither_auto_select(ITIDY_HARMONISED_PALETTE_SIZE);
    else
        actual_dither = dither_method;

    log_debug(LOG_ICONS, "harmonised_palette: applying GlowIcons 29-colour "
             "palette to %ux%u icon (dither=%u)\n",
             (unsigned)img->width, (unsigned)img->height,
             (unsigned)actual_dither);

    /* Remap every pixel to the nearest harmonised colour */
    if (actual_dither == ITIDY_DITHER_FLOYD)
    {
        if (!itidy_dither_floyd_steinberg(
                img->pixel_data_normal, img->width, img->height,
                img->palette_normal, img->palette_size_normal,
                harm_pal, (ULONG)ITIDY_HARMONISED_PALETTE_SIZE))
        {
            log_warning(LOG_ICONS, "harmonised_palette: "
                        "Floyd-Steinberg failed, falling back to nearest-colour\n");

            itidy_palette_remap(img->pixel_data_normal,
                                img->width, img->height,
                                img->palette_normal,
                                img->palette_size_normal,
                                harm_pal,
                                (ULONG)ITIDY_HARMONISED_PALETTE_SIZE,
                                ITIDY_DITHER_NONE);
        }
    }
    else
    {
        itidy_palette_remap(img->pixel_data_normal,
                            img->width, img->height,
                            img->palette_normal,
                            img->palette_size_normal,
                            harm_pal,
                            (ULONG)ITIDY_HARMONISED_PALETTE_SIZE,
                            actual_dither);
    }

    /* Replace the icon palette with the harmonised palette.
     * The current palette buffer may have been allocated with fewer entries
     * than ITIDY_HARMONISED_PALETTE_SIZE (e.g. only 16 entries after IFF
     * CMAP replacement).  Reallocate if necessary to avoid a heap overflow. */
    if (img->palette_size_normal < ITIDY_HARMONISED_PALETTE_SIZE)
    {
        struct ColorRegister *new_pal = (struct ColorRegister *)whd_malloc(
            ITIDY_HARMONISED_PALETTE_SIZE * sizeof(struct ColorRegister));

        if (new_pal == NULL)
        {
            log_error(LOG_ICONS, "harmonised_palette: "
                      "palette buffer realloc failed\n");
            return FALSE;
        }

        whd_free(img->palette_normal);
        img->palette_normal = new_pal;
    }

    for (i = 0; i < ITIDY_HARMONISED_PALETTE_SIZE; i++)
    {
        img->palette_normal[i] = harm_pal[i];
    }
    img->palette_size_normal = ITIDY_HARMONISED_PALETTE_SIZE;

    log_debug(LOG_ICONS, "harmonised_palette: done, palette set to "
             "%u GlowIcons colours\n",
             (unsigned)ITIDY_HARMONISED_PALETTE_SIZE);

    return TRUE;
}
