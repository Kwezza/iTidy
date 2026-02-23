/*
 * palette_reduction.c - High-Level Palette Reduction Pipeline
 *
 * See palette_reduction.h for API documentation.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include "palette_reduction.h"
#include "palette_mapping.h"
#include "palette_quantization.h"
#include "palette_dithering.h"
#include "palette_grayscale.h"
#include "../../writeLog.h"
#include "../../platform/platform.h"  /* whd_malloc, whd_free */

#include <string.h>  /* memcpy */

/*========================================================================*/
/* Chooser Index Mapping                                                  */
/*========================================================================*/

/** Chooser index to color count lookup table.
 *  Index 3 is the GlowIcons harmonised palette (29 colours).
 *  ITIDY_MAX_COLORS_CHOOSER_COUNT must equal the number of entries here. */
static const UWORD color_count_table[] = {
    4, 8, 16, 29, 32, 64, 128, 256
};

UWORD itidy_max_colors_from_index(UWORD chooser_index)
{
    if (chooser_index >= ITIDY_MAX_COLORS_CHOOSER_COUNT)
        return ITIDY_MAX_COLORS_256;  /* Ultra and beyond -> 256 */

    return color_count_table[chooser_index];
}

UWORD itidy_max_colors_to_index(UWORD max_colors)
{
    UWORD i;

    for (i = 0; i < ITIDY_MAX_COLORS_CHOOSER_COUNT; i++)
    {
        if (color_count_table[i] >= max_colors)
            return i;
    }

    return ITIDY_MAX_COLORS_CHOOSER_COUNT - 1;  /* Default: No limit (256) */
}

/*========================================================================*/
/* itidy_reduce_palette                                                   */
/*========================================================================*/

BOOL itidy_reduce_palette(iTidy_IconImageData *img,
                          UWORD max_colors,
                          UWORD dither_method,
                          UWORD lowcolor_mapping)
{
    ULONG pixel_count;
    UWORD unique_count;
    struct ColorRegister reduced_palette[256];
    UWORD reduced_pal_size = 0;
    UWORD actual_dither;
    UWORD i;

    if (img == NULL || img->pixel_data_normal == NULL ||
        img->palette_normal == NULL)
    {
        return FALSE;
    }

    pixel_count = (ULONG)img->width * (ULONG)img->height;

    /* Step 1: Count unique colors actually used */
    unique_count = itidy_palette_count_unique(img->pixel_data_normal,
                                              pixel_count);

    log_debug(LOG_ICONS, "palette_reduction: %u unique colors in %ux%u icon, "
             "max=%u\n",
             (unsigned)unique_count, (unsigned)img->width,
             (unsigned)img->height, (unsigned)max_colors);

    /* Step 2: Check if reduction is needed */
    if (unique_count <= max_colors)
    {
        log_debug(LOG_ICONS, "palette_reduction: no reduction needed "
                 "(%u <= %u)\n",
                 (unsigned)unique_count, (unsigned)max_colors);
        return TRUE;  /* No-op */
    }

    /* Step 3: Resolve dithering method (Auto -> concrete method) */
    if (dither_method == ITIDY_DITHER_AUTO)
        actual_dither = itidy_dither_auto_select(max_colors);
    else
        actual_dither = dither_method;

    /* Step 4: Generate the reduced palette */
    if (max_colors <= 8 && lowcolor_mapping != ITIDY_LOWCOLOR_GRAYSCALE)
    {
        /* Low-color mode: use pre-defined palettes */
        if (lowcolor_mapping == ITIDY_LOWCOLOR_WORKBENCH)
        {
            itidy_workbench_palette(reduced_palette, max_colors);
            reduced_pal_size = max_colors;
        }
        else if (lowcolor_mapping == ITIDY_LOWCOLOR_HYBRID)
        {
            itidy_hybrid_palette(reduced_palette, max_colors);
            reduced_pal_size = max_colors;
        }
        else
        {
            /* Fallback to grayscale */
            itidy_grayscale_palette(reduced_palette, max_colors);
            reduced_pal_size = max_colors;
        }

        log_debug(LOG_ICONS, "palette_reduction: using %s palette (%u colors)\n",
                 (lowcolor_mapping == ITIDY_LOWCOLOR_WORKBENCH) ? "Workbench" :
                 (lowcolor_mapping == ITIDY_LOWCOLOR_HYBRID) ? "Hybrid" :
                 "Grayscale",
                 (unsigned)reduced_pal_size);
    }
    else if (max_colors <= 8)
    {
        /* Grayscale mode for 4-8 colors */
        itidy_grayscale_palette(reduced_palette, max_colors);
        reduced_pal_size = max_colors;

        log_debug(LOG_ICONS, "palette_reduction: using Grayscale palette "
                 "(%u colors)\n", (unsigned)reduced_pal_size);
    }
    else
    {
        /* Standard Median Cut quantization */
        if (!itidy_quantize_palette(img->pixel_data_normal, pixel_count,
                                     img->palette_normal,
                                     img->palette_size_normal,
                                     max_colors,
                                     reduced_palette, &reduced_pal_size))
        {
            log_error(LOG_ICONS, "palette_reduction: "
                      "quantization failed\n");
            return FALSE;
        }
    }

    /* Step 5: Remap pixels to reduced palette (with dithering) */
    if (actual_dither == ITIDY_DITHER_FLOYD)
    {
        /* Floyd-Steinberg: full-pass error diffusion */
        if (!itidy_dither_floyd_steinberg(
                img->pixel_data_normal, img->width, img->height,
                img->palette_normal, img->palette_size_normal,
                reduced_palette, (ULONG)reduced_pal_size))
        {
            log_warning(LOG_ICONS, "palette_reduction: F-S dithering failed, "
                        "falling back to nearest-color\n");
            /* Fall through to nearest-color remap */
            itidy_palette_remap(img->pixel_data_normal,
                                img->width, img->height,
                                img->palette_normal,
                                img->palette_size_normal,
                                reduced_palette,
                                (ULONG)reduced_pal_size,
                                ITIDY_DITHER_NONE);
        }
    }
    else
    {
        /* None or Ordered: per-pixel remapping */
        itidy_palette_remap(img->pixel_data_normal,
                            img->width, img->height,
                            img->palette_normal,
                            img->palette_size_normal,
                            reduced_palette,
                            (ULONG)reduced_pal_size,
                            actual_dither);
    }

    /* Step 6: Replace the icon's palette with the reduced palette */
    for (i = 0; i < reduced_pal_size; i++)
    {
        img->palette_normal[i] = reduced_palette[i];
    }
    img->palette_size_normal = (ULONG)reduced_pal_size;

    log_debug(LOG_ICONS, "palette_reduction: reduced %u -> %u colors, "
             "dither=%s\n",
             (unsigned)unique_count, (unsigned)reduced_pal_size,
             (actual_dither == ITIDY_DITHER_NONE)    ? "None" :
             (actual_dither == ITIDY_DITHER_ORDERED)  ? "Ordered" :
             (actual_dither == ITIDY_DITHER_FLOYD)    ? "Floyd-Steinberg" :
             "Unknown");

    return TRUE;
}
