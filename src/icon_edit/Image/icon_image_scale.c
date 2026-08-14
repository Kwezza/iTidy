/*
 * icon_image_scale.c - iTidy2 adapters for shared image scaling
 *
 * Progress reporting stays in iTidy2 (throttled DOS ticks + cancel flag).
 * Algorithms live in shared/image/.
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include "icon_iff_render.h"
#include "icon_image_scale.h"

#include <image/image_scale.h>
#include <image/image_palette.h>

static BOOL image_scale_progress_adapter(void *user_data,
                                         const char *phase,
                                         ULONG current,
                                         ULONG total)
{
    return itidy_report_progress_throttled((iTidy_IFFRenderParams *)user_data,
                                           phase, current, total,
                                           TICKS_PER_SECOND);
}

static const iTidy_ImageProgress *make_progress(iTidy_IFFRenderParams *params,
                                                iTidy_ImageProgress *storage)
{
    if (params == NULL)
        return NULL;

    storage->fn = image_scale_progress_adapter;
    storage->user_data = params;
    return storage;
}

void area_average_scale(const UBYTE *src_chunky,
                        UWORD src_w, UWORD src_h,
                        UBYTE *dest_buf, UWORD dest_w, UWORD dest_h,
                        UWORD dest_stride,
                        UWORD dest_offset_x, UWORD dest_offset_y,
                        const struct ColorRegister *src_palette,
                        ULONG src_palette_size,
                        const struct ColorRegister *dest_palette,
                        ULONG dest_palette_size,
                        iTidy_IFFRenderParams *params)
{
    iTidy_ImageProgress prog;

    image_scale_indexed(src_chunky, src_w, src_h,
                        dest_buf, dest_w, dest_h,
                        dest_stride, dest_offset_x, dest_offset_y,
                        (const iTidy_RGB8 *)src_palette, src_palette_size,
                        (const iTidy_RGB8 *)dest_palette, dest_palette_size,
                        make_progress(params, &prog));
}

BOOL prefilter_2x2(const UBYTE *src, UWORD src_w, UWORD src_h,
                   UBYTE **out_buf, UWORD *out_w, UWORD *out_h,
                   const struct ColorRegister *palette,
                   ULONG palette_size)
{
    return image_prefilter_2x2(src, src_w, src_h, out_buf, out_w, out_h,
                               (const iTidy_RGB8 *)palette, palette_size);
}

void rgb24_area_average_scale(const UBYTE *src_rgb24,
                              UWORD src_w, UWORD src_h,
                              UBYTE *dest_rgb24,
                              UWORD dest_w, UWORD dest_h,
                              iTidy_IFFRenderParams *params)
{
    iTidy_ImageProgress prog;

    image_scale_rgb24(src_rgb24, src_w, src_h,
                      dest_rgb24, dest_w, dest_h,
                      make_progress(params, &prog));
}

void quantize_rgb24_to_cube(const UBYTE *src_rgb24,
                            UBYTE *dest_chunky,
                            ULONG pixel_count,
                            ULONG palette_size)
{
    image_quantize_rgb24_to_cube(src_rgb24, dest_chunky,
                                 pixel_count, palette_size);
}
