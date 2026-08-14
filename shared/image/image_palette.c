/*
 * image_palette.c - Nearest-colour matching, remapping, Median Cut
 *
 * Extracted from iTidy2 palette_mapping.c / palette_quantization.c /
 * icon_image_scale.c (cube quantiser) without algorithm changes.
 */

#include "image_palette.h"
#include "image_dither.h"
#include "image_types.h"

#include <string.h>

/*========================================================================*/
/* Distance and nearest-colour                                            */
/*========================================================================*/

UWORD image_palette_manhattan_distance(UBYTE r1, UBYTE g1, UBYTE b1,
                                       UBYTE r2, UBYTE g2, UBYTE b2)
{
    UWORD dist = 0;

    if (r1 > r2) dist += (UWORD)(r1 - r2); else dist += (UWORD)(r2 - r1);
    if (g1 > g2) dist += (UWORD)(g1 - g2); else dist += (UWORD)(g2 - g1);
    if (b1 > b2) dist += (UWORD)(b1 - b2); else dist += (UWORD)(b2 - b1);

    return dist;
}

UBYTE image_palette_find_nearest(const iTidy_RGB8 *palette,
                                 ULONG palette_size,
                                 UBYTE target_r, UBYTE target_g,
                                 UBYTE target_b)
{
    UBYTE best_idx = 0;
    UWORD best_dist = 0xFFFF;
    ULONG i;

    if (palette == NULL || palette_size == 0)
        return 0;

    for (i = 0; i < palette_size; i++)
    {
        UWORD dist = image_palette_manhattan_distance(
            target_r, target_g, target_b,
            palette[i].r, palette[i].g, palette[i].b);

        if (dist < best_dist)
        {
            best_dist = dist;
            best_idx = (UBYTE)i;
            if (dist == 0)
                break;
        }
    }

    return best_idx;
}

UBYTE image_palette_find_nearest_dithered(const iTidy_RGB8 *palette,
                                          ULONG palette_size,
                                          UBYTE target_r, UBYTE target_g,
                                          UBYTE target_b,
                                          UWORD pixel_x, UWORD pixel_y,
                                          UWORD dither_method)
{
    UBYTE adj_r = target_r;
    UBYTE adj_g = target_g;
    UBYTE adj_b = target_b;

    if (dither_method == ITIDY_DITHER_ORDERED)
    {
        WORD offset = image_dither_bayer_offset(pixel_x, pixel_y);

        if (offset > 0)
        {
            WORD temp;
            temp = (WORD)adj_r + offset;
            adj_r = (temp > 255) ? 255 : (UBYTE)temp;
            temp = (WORD)adj_g + offset;
            adj_g = (temp > 255) ? 255 : (UBYTE)temp;
            temp = (WORD)adj_b + offset;
            adj_b = (temp > 255) ? 255 : (UBYTE)temp;
        }
        else if (offset < 0)
        {
            WORD abs_off = -offset;
            WORD temp;
            temp = (WORD)adj_r - abs_off;
            adj_r = (temp < 0) ? 0 : (UBYTE)temp;
            temp = (WORD)adj_g - abs_off;
            adj_g = (temp < 0) ? 0 : (UBYTE)temp;
            temp = (WORD)adj_b - abs_off;
            adj_b = (temp < 0) ? 0 : (UBYTE)temp;
        }
    }

    return image_palette_find_nearest(palette, palette_size,
                                      adj_r, adj_g, adj_b);
}

UWORD image_palette_count_unique(const UBYTE *pixels, ULONG pixel_count)
{
    UBYTE seen[256];
    UWORD count = 0;
    ULONG i;

    if (pixels == NULL || pixel_count == 0)
        return 0;

    memset(seen, 0, sizeof(seen));

    for (i = 0; i < pixel_count; i++)
    {
        if (!seen[pixels[i]])
        {
            seen[pixels[i]] = 1;
            count++;
        }
    }

    return count;
}

void image_palette_remap(UBYTE *pixels, UWORD width, UWORD height,
                         const iTidy_RGB8 *old_palette,
                         ULONG old_pal_size,
                         const iTidy_RGB8 *new_palette,
                         ULONG new_pal_size,
                         UWORD dither_method)
{
    UWORD x, y;

    if (pixels == NULL || old_palette == NULL || new_palette == NULL)
        return;

    if (old_pal_size == 0 || new_pal_size == 0)
        return;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            ULONG offset = (ULONG)y * (ULONG)width + (ULONG)x;
            UBYTE old_idx = pixels[offset];
            UBYTE r, g, b;

            if ((ULONG)old_idx < old_pal_size)
            {
                r = old_palette[old_idx].r;
                g = old_palette[old_idx].g;
                b = old_palette[old_idx].b;
            }
            else
            {
                r = g = b = 0;
            }

            pixels[offset] = image_palette_find_nearest_dithered(
                new_palette, new_pal_size,
                r, g, b,
                x, y,
                dither_method);
        }
    }
}

/*========================================================================*/
/* Median Cut internals                                                   */
/*========================================================================*/

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

    if (r_range >= g_range && r_range >= b_range)
        box->longest_axis = 0;
    else if (g_range >= r_range && g_range >= b_range)
        box->longest_axis = 1;
    else
        box->longest_axis = 2;
}

static void sort_entries_by_axis(iTidy_ColorEntry *entries, UWORD count,
                                 UBYTE axis)
{
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

static void compute_box_average(const iTidy_ColorEntry *histogram,
                                const iTidy_ColorBox *box,
                                iTidy_RGB8 *out_color)
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
        out_color->r = (UBYTE)(r_sum / total);
        out_color->g = (UBYTE)(g_sum / total);
        out_color->b = (UBYTE)(b_sum / total);
    }
    else
    {
        out_color->r = out_color->g = out_color->b = 0;
    }
}

BOOL image_build_color_histogram(const UBYTE *pixels, ULONG pixel_count,
                                 const iTidy_RGB8 *palette,
                                 ULONG palette_size,
                                 iTidy_ColorEntry *out_histogram,
                                 UWORD *out_count)
{
    ULONG freq[256];
    UWORD unique = 0;
    ULONG i;

    if (pixels == NULL || palette == NULL || out_histogram == NULL || out_count == NULL)
        return FALSE;

    memset(freq, 0, sizeof(freq));

    for (i = 0; i < pixel_count; i++)
    {
        if ((ULONG)pixels[i] < palette_size)
            freq[pixels[i]]++;
    }

    for (i = 0; i < 256 && i < palette_size; i++)
    {
        if (freq[i] > 0)
        {
            out_histogram[unique].r = palette[i].r;
            out_histogram[unique].g = palette[i].g;
            out_histogram[unique].b = palette[i].b;
            out_histogram[unique]._pad = 0;
            out_histogram[unique].count = freq[i];
            unique++;
        }
    }

    *out_count = unique;

    image_log_debug("palette_quantization: histogram built, "
                    "%u unique colors from %lu pixels\n",
                    (unsigned)unique, (unsigned long)pixel_count);

    return TRUE;
}

BOOL image_median_cut(iTidy_ColorEntry *histogram, UWORD hist_count,
                      UWORD target_colors,
                      iTidy_RGB8 *out_palette,
                      UWORD *out_pal_size)
{
    iTidy_ColorBox boxes[ITIDY_MAX_COLOR_BOXES];
    UWORD num_boxes = 0;
    UWORD i;

    if (histogram == NULL || out_palette == NULL || out_pal_size == NULL)
        return FALSE;

    if (target_colors > hist_count)
        target_colors = hist_count;

    if (target_colors == 0)
        target_colors = 1;

    boxes[0].start = 0;
    boxes[0].count = hist_count;
    compute_box_bounds(histogram, &boxes[0]);
    num_boxes = 1;

    while (num_boxes < target_colors)
    {
        UWORD best_box = 0;
        UBYTE best_range = 0;
        UWORD split_point;
        ULONG running_count;
        ULONG half_count;

        for (i = 0; i < num_boxes; i++)
        {
            UBYTE range;

            if (boxes[i].count <= 1)
                continue;

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

        if (best_range == 0)
            break;

        sort_entries_by_axis(&histogram[boxes[best_box].start],
                             boxes[best_box].count,
                             boxes[best_box].longest_axis);

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

        if (split_point == 0)
            split_point = 1;
        if (split_point >= boxes[best_box].count)
            split_point = boxes[best_box].count - 1;

        boxes[num_boxes].start = boxes[best_box].start + split_point;
        boxes[num_boxes].count = boxes[best_box].count - split_point;
        compute_box_bounds(histogram, &boxes[num_boxes]);

        boxes[best_box].count = split_point;
        compute_box_bounds(histogram, &boxes[best_box]);

        num_boxes++;
    }

    for (i = 0; i < num_boxes; i++)
    {
        compute_box_average(histogram, &boxes[i], &out_palette[i]);
    }

    *out_pal_size = num_boxes;

    image_log_debug("palette_quantization: median cut produced "
                    "%u colors from %u unique\n",
                    (unsigned)num_boxes, (unsigned)hist_count);

    return TRUE;
}

BOOL image_quantize_palette(const UBYTE *pixels, ULONG pixel_count,
                            const iTidy_RGB8 *src_palette,
                            ULONG src_pal_size,
                            UWORD target_colors,
                            iTidy_RGB8 *out_palette,
                            UWORD *out_pal_size)
{
    iTidy_ColorEntry histogram[256];
    UWORD hist_count = 0;

    if (pixels == NULL || src_palette == NULL ||
        out_palette == NULL || out_pal_size == NULL)
        return FALSE;

    if (!image_build_color_histogram(pixels, pixel_count,
                                     src_palette, src_pal_size,
                                     histogram, &hist_count))
    {
        image_log_error("palette_quantization: "
                        "histogram build failed\n");
        return FALSE;
    }

    if (hist_count <= target_colors)
    {
        UWORD i;
        for (i = 0; i < hist_count; i++)
        {
            out_palette[i].r = histogram[i].r;
            out_palette[i].g = histogram[i].g;
            out_palette[i].b = histogram[i].b;
        }
        *out_pal_size = hist_count;
        image_log_debug("palette_quantization: %u unique colors "
                        "<= target %u, no reduction needed\n",
                        (unsigned)hist_count, (unsigned)target_colors);
        return TRUE;
    }

    return image_median_cut(histogram, hist_count, target_colors,
                            out_palette, out_pal_size);
}

void image_quantize_rgb24_to_cube(const UBYTE *src_rgb24,
                                  UBYTE *dest_chunky,
                                  ULONG pixel_count,
                                  ULONG palette_size)
{
    ULONG i;
    const UBYTE *rgb_ptr = src_rgb24;
    UBYTE *out_ptr = dest_chunky;

    for (i = 0; i < pixel_count; i++)
    {
        UBYTE r = *rgb_ptr++;
        UBYTE g = *rgb_ptr++;
        UBYTE b = *rgb_ptr++;

        UWORD r_idx = (UWORD)(r + 25) / 51;
        UWORD g_idx = (UWORD)(g + 25) / 51;
        UWORD b_idx = (UWORD)(b + 25) / 51;

        if (r_idx > 5) r_idx = 5;
        if (g_idx > 5) g_idx = 5;
        if (b_idx > 5) b_idx = 5;

        *out_ptr++ = (UBYTE)(r_idx * 36 + g_idx * 6 + b_idx);
    }

    (void)palette_size;
}
