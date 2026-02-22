/*
 * icon_image_scale.h - Internal image scaling and quantisation routines
 *
 * Area-average downscalers (chunky-palette and RGB24), 2x2 pre-filter,
 * and direct 6x6x6 colour-cube quantiser shared between
 * icon_iff_render.c and icon_datatype_render.c.
 *
 * NOT a public API -- include only from within src/icon_edit/Image/.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef ICON_IMAGE_SCALE_H
#define ICON_IMAGE_SCALE_H

#include <platform/platform.h>
#include <platform/amiga_headers.h>

#include "icon_iff_render.h"   /* iTidy_IFFRenderParams, struct ColorRegister */

/*========================================================================*/
/* Chunky-palette area-average downscaler                                 */
/*========================================================================*/

/**
 * @brief Area-average downscale source image into destination buffer.
 *
 * Source and destination pixels are palette indices.  Destination palette
 * may differ from the source palette (nearest-colour match is performed).
 */
void area_average_scale(const UBYTE *src_chunky,
                        UWORD src_w, UWORD src_h,
                        UBYTE *dest_buf, UWORD dest_w, UWORD dest_h,
                        UWORD dest_stride,
                        UWORD dest_offset_x, UWORD dest_offset_y,
                        const struct ColorRegister *src_palette,
                        ULONG src_palette_size,
                        const struct ColorRegister *dest_palette,
                        ULONG dest_palette_size,
                        iTidy_IFFRenderParams *params);

/*========================================================================*/
/* 2x2 averaging pre-filter                                               */
/*========================================================================*/

/**
 * @brief Pre-filter: average 2x2 pixel blocks to a half-size buffer.
 *
 * Used before area_average_scale when the reduction ratio exceeds
 * ITIDY_SCALE_PREFILTER_RATIO on large images.
 * out_buf must be whd_free()d by the caller on success.
 */
BOOL prefilter_2x2(const UBYTE *src, UWORD src_w, UWORD src_h,
                   UBYTE **out_buf, UWORD *out_w, UWORD *out_h,
                   const struct ColorRegister *palette,
                   ULONG palette_size);

/*========================================================================*/
/* RGB24 area-average downscaler                                          */
/*========================================================================*/

/**
 * @brief Area-average downscale operating on packed RGB24 (3 bytes/pixel).
 *
 * Used by the datatypes fallback path: downscales large RGB24 source
 * images to thumbnail size before quantisation, dramatically reducing
 * per-pixel work.
 */
void rgb24_area_average_scale(const UBYTE *src_rgb24,
                              UWORD src_w, UWORD src_h,
                              UBYTE *dest_rgb24,
                              UWORD dest_w, UWORD dest_h,
                              iTidy_IFFRenderParams *params);

/*========================================================================*/
/* 6x6x6 colour-cube quantiser                                            */
/*========================================================================*/

/**
 * @brief Quantize packed RGB24 pixels to the 6x6x6 colour cube.
 *
 * Writes one palette-index byte per pixel via direct index calculation
 * (no brute-force palette search).
 */
void quantize_rgb24_to_cube(const UBYTE *src_rgb24,
                            UBYTE *dest_chunky,
                            ULONG pixel_count,
                            ULONG palette_size);

#endif /* ICON_IMAGE_SCALE_H */
