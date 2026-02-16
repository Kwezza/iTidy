/*
 * palette_grayscale.c - Grayscale & Workbench Palette Mapping
 *
 * See palette_grayscale.h for API documentation.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include "palette_grayscale.h"
#include "palette_mapping.h"
#include "palette_dithering.h"
#include "../../writeLog.h"

/*========================================================================*/
/* Pre-Defined Palettes                                                   */
/*========================================================================*/

/** 4-level evenly spaced grayscale palette */
static const struct ColorRegister gray_4_palette[4] = {
    {   0,   0,   0 },  /* Black */
    {  85,  85,  85 },  /* Dark gray */
    { 170, 170, 170 },  /* Light gray */
    { 255, 255, 255 }   /* White */
};

/** 8-level evenly spaced grayscale palette */
static const struct ColorRegister gray_8_palette[8] = {
    {   0,   0,   0 },  /* Black */
    {  36,  36,  36 },
    {  73,  73,  73 },
    { 109, 109, 109 },
    { 146, 146, 146 },
    { 182, 182, 182 },
    { 219, 219, 219 },
    { 255, 255, 255 }   /* White */
};

/** Standard Workbench 3.x system palette (8 colors) */
static const struct ColorRegister wb_palette_8[8] = {
    { 170, 170, 170 },  /* 0: Light gray (window background) */
    {   0,   0,   0 },  /* 1: Black (text, outlines) */
    { 255, 255, 255 },  /* 2: White (highlights) */
    { 102, 136, 187 },  /* 3: Blue/Cyan (selected items) */
    {  85,  85,  85 },  /* 4: Dark gray (shadows) */
    { 119, 119, 119 },  /* 5: Mid gray (window borders) */
    { 187, 119,  68 },  /* 6: Orange/Tan (drag bars) */
    { 170, 170, 221 }   /* 7: Light blue (gadgets) */
};

/** Hybrid palette: 5 grays + WB accent colors */
static const struct ColorRegister hybrid_8_palette[8] = {
    {   0,   0,   0 },  /* 0: Black */
    {  73,  73,  73 },  /* 1: Dark gray */
    { 146, 146, 146 },  /* 2: Mid gray */
    { 219, 219, 219 },  /* 3: Light gray */
    { 255, 255, 255 },  /* 4: White */
    { 102, 136, 187 },  /* 5: WB Blue (for UI elements) */
    { 187, 119,  68 },  /* 6: WB Orange (for warnings) */
    { 170, 170, 170 }   /* 7: WB Light gray (for borders) */
};

/*========================================================================*/
/* itidy_rgb_to_gray                                                      */
/*========================================================================*/

UBYTE itidy_rgb_to_gray(UBYTE r, UBYTE g, UBYTE b)
{
    /* ITU-R BT.601 luminance formula (integer-only):
     * Y = (306*R + 601*G + 117*B) >> 10
     * Coefficients sum to 1024 (2^10) for clean bit shift */
    return (UBYTE)((306L * (ULONG)r + 601L * (ULONG)g +
                    117L * (ULONG)b) >> 10);
}

/*========================================================================*/
/* itidy_grayscale_palette                                                */
/*========================================================================*/

void itidy_grayscale_palette(struct ColorRegister *out_palette,
                             UWORD num_colors)
{
    UWORD i;

    if (out_palette == NULL || num_colors == 0)
        return;

    if (num_colors <= 4)
    {
        for (i = 0; i < num_colors && i < 4; i++)
            out_palette[i] = gray_4_palette[i];
    }
    else
    {
        for (i = 0; i < num_colors && i < 8; i++)
            out_palette[i] = gray_8_palette[i];
    }
}

/*========================================================================*/
/* itidy_workbench_palette                                                */
/*========================================================================*/

void itidy_workbench_palette(struct ColorRegister *out_palette,
                             UWORD num_colors)
{
    UWORD i;

    if (out_palette == NULL || num_colors == 0)
        return;

    for (i = 0; i < num_colors && i < 8; i++)
        out_palette[i] = wb_palette_8[i];
}

/*========================================================================*/
/* itidy_hybrid_palette                                                   */
/*========================================================================*/

void itidy_hybrid_palette(struct ColorRegister *out_palette,
                          UWORD num_colors)
{
    if (out_palette == NULL || num_colors == 0)
        return;

    if (num_colors <= 4)
    {
        /* 4-color hybrid falls back to pure grayscale —
         * not enough slots for accent colors */
        itidy_grayscale_palette(out_palette, num_colors);
    }
    else
    {
        UWORD i;
        for (i = 0; i < num_colors && i < 8; i++)
            out_palette[i] = hybrid_8_palette[i];
    }
}

/*========================================================================*/
/* itidy_grayscale_remap                                                  */
/*========================================================================*/

void itidy_grayscale_remap(UBYTE *pixels, UWORD width, UWORD height,
                           const struct ColorRegister *old_palette,
                           ULONG old_pal_size,
                           const struct ColorRegister *gray_palette,
                           ULONG gray_pal_size,
                           UWORD dither_method)
{
    UWORD x, y;

    if (pixels == NULL || old_palette == NULL || gray_palette == NULL)
        return;

    if (old_pal_size == 0 || gray_pal_size == 0)
        return;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            ULONG offset = (ULONG)y * (ULONG)width + (ULONG)x;
            UBYTE old_idx = pixels[offset];
            UBYTE gray_val;

            /* Convert to grayscale via luminance */
            if ((ULONG)old_idx < old_pal_size)
            {
                gray_val = itidy_rgb_to_gray(
                    old_palette[old_idx].red,
                    old_palette[old_idx].green,
                    old_palette[old_idx].blue);
            }
            else
            {
                gray_val = 0;
            }

            /* Find closest match in gray palette (with optional dithering) */
            pixels[offset] = itidy_palette_find_nearest_dithered(
                gray_palette, gray_pal_size,
                gray_val, gray_val, gray_val,
                x, y,
                dither_method);
        }
    }
}
