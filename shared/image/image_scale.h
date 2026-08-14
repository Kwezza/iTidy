/*
 * image_scale.h - Indexed and RGB24 area-average scaling
 *
 * Target: 68000+, Workbench 2.x+, C89/C99 subset
 */

#ifndef ITIDY_IMAGE_SCALE_H
#define ITIDY_IMAGE_SCALE_H

#include "image_types.h"

void image_scale_indexed(const UBYTE *src_chunky,
                         UWORD src_w, UWORD src_h,
                         UBYTE *dest_buf, UWORD dest_w, UWORD dest_h,
                         UWORD dest_stride,
                         UWORD dest_offset_x, UWORD dest_offset_y,
                         const iTidy_RGB8 *src_palette,
                         ULONG src_palette_size,
                         const iTidy_RGB8 *dest_palette,
                         ULONG dest_palette_size,
                         const iTidy_ImageProgress *progress);

BOOL image_prefilter_2x2(const UBYTE *src, UWORD src_w, UWORD src_h,
                         UBYTE **out_buf, UWORD *out_w, UWORD *out_h,
                         const iTidy_RGB8 *palette,
                         ULONG palette_size);

void image_scale_rgb24(const UBYTE *src_rgb24,
                       UWORD src_w, UWORD src_h,
                       UBYTE *dest_rgb24,
                       UWORD dest_w, UWORD dest_h,
                       const iTidy_ImageProgress *progress);

#endif /* ITIDY_IMAGE_SCALE_H */
