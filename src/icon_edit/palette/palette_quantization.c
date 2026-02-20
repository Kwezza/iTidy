/*
 * palette_quantization.c - Median Cut Palette Quantization
 *
 * See palette_quantization.h for API documentation.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include "palette_quantization.h"
#include "../../writeLog.h"
#include "../../platform/platform.h"  /* whd_malloc, whd_free */

#include <string.h>  /* memset, memcpy */

/*========================================================================*/
/* Internal Helpers                                                       */
/*========================================================================*/

/**
 * @brief Compute bounding box and longest axis for a color box.
 */
static void compute_box_bounds(iTidy_ColorEntry *histogram,
                               iTidy_ColorBox *box)
{
    UWORD i;
    UBYTE r_range, g_range, b_range;

    box->r_min = 255; box->r_max = 0;
    box->g_min = 255; box->g_max = 0;
    box->b_min = 255; box->b_max = 0;
    box->pixel_count = 0;

    for (i = 0; i < box->count; i++)
    {
        UWORD idx = box->start + i;
        iTidy_ColorEntry *e = &histogram[idx];

        if (e->r < box->r_min) box->r_min = e->r;
        if (e->r > box->r_max) box->r_max = e->r;
        if (e->g < box->g_min) box->g_min = e->g;
        if (e->g > box->g_max) box->g_max = e->g;
        if (e->b < box->b_min) box->b_min = e->b;
        if (e->b > box->b_max) box->b_max = e->b;

        box->pixel_count += e->count;
    }

    r_range = box->r_max - box->r_min;
    g_range = box->g_max - box->g_min;
    b_range = box->b_max - box->b_min;

    /* Find longest axis */
    if (r_range >= g_range && r_range >= b_range)
        box->longest_axis = 0;  /* Red */
    else if (g_range >= r_range && g_range >= b_range)
        box->longest_axis = 1;  /* Green */
    else
        box->longest_axis = 2;  /* Blue */
}

/**
 * @brief Compare function for sorting histogram entries by red channel.
 */
static void sort_entries_by_axis(iTidy_ColorEntry *entries, UWORD count,
                                 UBYTE axis)
{
    /* Simple insertion sort — adequate for max 256 entries */
    UWORD i, j;

    for (i = 1; i < count; i++)
    {
        iTidy_ColorEntry temp = entries[i];
        UBYTE key;

        if (axis == 0) key = temp.r;
        else if (axis == 1) key = temp.g;
        else key = temp.b;

        j = i;
        while (j > 0)
        {
            UBYTE prev_key;
            if (axis == 0) prev_key = entries[j - 1].r;
            else if (axis == 1) prev_key = entries[j - 1].g;
            else prev_key = entries[j - 1].b;

            if (prev_key <= key)
                break;

            entries[j] = entries[j - 1];
            j--;
        }
        entries[j] = temp;
    }
}

/**
 * @brief Compute the average color of a box, weighted by pixel count.
 */
static void compute_box_average(const iTidy_ColorEntry *histogram,
                                const iTidy_ColorBox *box,
                                struct ColorRegister *out_color)
{
    ULONG r_sum = 0, g_sum = 0, b_sum = 0;
    ULONG total = 0;
    UWORD i;

    for (i = 0; i < box->count; i++)
    {
        UWORD idx = box->start + i;
        const iTidy_ColorEntry *e = &histogram[idx];

        r_sum += (ULONG)e->r * e->count;
        g_sum += (ULONG)e->g * e->count;
        b_sum += (ULONG)e->b * e->count;
        total += e->count;
    }

    if (total > 0)
    {
        out_color->red   = (UBYTE)(r_sum / total);
        out_color->green = (UBYTE)(g_sum / total);
        out_color->blue  = (UBYTE)(b_sum / total);
    }
    else
    {
        out_color->red = out_color->green = out_color->blue = 0;
    }
}

/*========================================================================*/
/* itidy_build_color_histogram                                            */
/*========================================================================*/

BOOL itidy_build_color_histogram(const UBYTE *pixels, ULONG pixel_count,
                                 const struct ColorRegister *palette,
                                 ULONG palette_size,
                                 iTidy_ColorEntry *out_histogram,
                                 UWORD *out_count)
{
    /* Use a frequency table indexed by palette index.
     * Since icons are 8-bit indexed, max 256 unique indices. */
    ULONG freq[256];
    UWORD unique = 0;
    ULONG i;

    if (pixels == NULL || palette == NULL || out_histogram == NULL || out_count == NULL)
        return FALSE;

    memset(freq, 0, sizeof(freq));

    /* Count frequency of each palette index */
    for (i = 0; i < pixel_count; i++)
    {
        if ((ULONG)pixels[i] < palette_size)
            freq[pixels[i]]++;
    }

    /* Build histogram entries for indices that are actually used */
    for (i = 0; i < 256 && i < palette_size; i++)
    {
        if (freq[i] > 0)
        {
            out_histogram[unique].r = palette[i].red;
            out_histogram[unique].g = palette[i].green;
            out_histogram[unique].b = palette[i].blue;
            out_histogram[unique]._pad = 0;
            out_histogram[unique].count = freq[i];
            unique++;
        }
    }

    *out_count = unique;

    log_debug(LOG_ICONS, "palette_quantization: histogram built, "
              "%u unique colors from %lu pixels\n",
              (unsigned)unique, (unsigned long)pixel_count);

    return TRUE;
}

/*========================================================================*/
/* itidy_median_cut                                                       */
/*========================================================================*/

BOOL itidy_median_cut(iTidy_ColorEntry *histogram, UWORD hist_count,
                      UWORD target_colors,
                      struct ColorRegister *out_palette,
                      UWORD *out_pal_size)
{
    iTidy_ColorBox boxes[ITIDY_MAX_COLOR_BOXES];
    UWORD num_boxes = 0;
    UWORD i;

    if (histogram == NULL || out_palette == NULL || out_pal_size == NULL)
        return FALSE;

    /* Clamp target to available colors */
    if (target_colors > hist_count)
        target_colors = hist_count;

    if (target_colors == 0)
        target_colors = 1;

    /* Initialize with a single box containing all entries */
    boxes[0].start = 0;
    boxes[0].count = hist_count;
    compute_box_bounds(histogram, &boxes[0]);
    num_boxes = 1;

    /* Split boxes until we have target_colors boxes */
    while (num_boxes < target_colors)
    {
        UWORD best_box = 0;
        UBYTE best_range = 0;
        UWORD split_point;
        ULONG running_count;
        ULONG half_count;

        /* Find the box with the longest axis range that can be split */
        for (i = 0; i < num_boxes; i++)
        {
            UBYTE range;

            if (boxes[i].count <= 1)
                continue;  /* Can't split a single-entry box */

            if (boxes[i].longest_axis == 0)
                range = boxes[i].r_max - boxes[i].r_min;
            else if (boxes[i].longest_axis == 1)
                range = boxes[i].g_max - boxes[i].g_min;
            else
                range = boxes[i].b_max - boxes[i].b_min;

            if (range > best_range)
            {
                best_range = range;
                best_box = i;
            }
        }

        /* If no box can be split, we're done */
        if (best_range == 0)
            break;

        /* Sort the entries in the best box along its longest axis */
        sort_entries_by_axis(&histogram[boxes[best_box].start],
                             boxes[best_box].count,
                             boxes[best_box].longest_axis);

        /* Find the median split point by pixel count */
        half_count = boxes[best_box].pixel_count / 2;
        running_count = 0;
        split_point = 0;

        for (i = 0; i < boxes[best_box].count - 1; i++)
        {
            running_count += histogram[boxes[best_box].start + i].count;
            if (running_count >= half_count)
            {
                split_point = i + 1;
                break;
            }
        }

        /* Ensure valid split (at least 1 entry per side) */
        if (split_point == 0)
            split_point = 1;
        if (split_point >= boxes[best_box].count)
            split_point = boxes[best_box].count - 1;

        /* Create new box from the upper half */
        boxes[num_boxes].start = boxes[best_box].start + split_point;
        boxes[num_boxes].count = boxes[best_box].count - split_point;
        compute_box_bounds(histogram, &boxes[num_boxes]);

        /* Shrink the original box to the lower half */
        boxes[best_box].count = split_point;
        compute_box_bounds(histogram, &boxes[best_box]);

        num_boxes++;
    }

    /* Compute average color for each box to produce final palette */
    for (i = 0; i < num_boxes; i++)
    {
        compute_box_average(histogram, &boxes[i], &out_palette[i]);
    }

    *out_pal_size = num_boxes;

    log_debug(LOG_ICONS, "palette_quantization: median cut produced "
              "%u colors from %u unique\n",
              (unsigned)num_boxes, (unsigned)hist_count);

    return TRUE;
}

/*========================================================================*/
/* itidy_quantize_palette                                                 */
/*========================================================================*/

BOOL itidy_quantize_palette(const UBYTE *pixels, ULONG pixel_count,
                            const struct ColorRegister *src_palette,
                            ULONG src_pal_size,
                            UWORD target_colors,
                            struct ColorRegister *out_palette,
                            UWORD *out_pal_size)
{
    iTidy_ColorEntry histogram[256];
    UWORD hist_count = 0;

    if (pixels == NULL || src_palette == NULL ||
        out_palette == NULL || out_pal_size == NULL)
        return FALSE;

    /* Step 1: Build color histogram */
    if (!itidy_build_color_histogram(pixels, pixel_count,
                                     src_palette, src_pal_size,
                                     histogram, &hist_count))
    {
        log_error(LOG_ICONS, "palette_quantization: "
                  "histogram build failed\n");
        return FALSE;
    }

    /* If already within limit, copy used colors directly */
    if (hist_count <= target_colors)
    {
        UWORD i;
        for (i = 0; i < hist_count; i++)
        {
            out_palette[i].red   = histogram[i].r;
            out_palette[i].green = histogram[i].g;
            out_palette[i].blue  = histogram[i].b;
        }
        *out_pal_size = hist_count;
        log_debug(LOG_ICONS, "palette_quantization: %u unique colors "
                 "<= target %u, no reduction needed\n",
                 (unsigned)hist_count, (unsigned)target_colors);
        return TRUE;
    }

    /* Step 2: Run Median Cut to reduce to target_colors */
    return itidy_median_cut(histogram, hist_count, target_colors,
                            out_palette, out_pal_size);
}
