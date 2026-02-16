/*
 * icon_iff_render.h - IFF ILBM Thumbnail Renderer
 *
 * Parses IFF ILBM image files via iffparse.library, decodes interleaved
 * bitplane data to chunky pixels, corrects aspect ratio for interlace
 * images, and area-average downscales to fit an icon template's safe area.
 *
 * Supports:
 *   - Standard palette ILBM (1-8 bitplanes)
 *   - ByteRun1 compression (type 1) and uncompressed (type 0)
 *   - CAMG viewport mode detection (interlace, hires)
 *   - EHB (Extra Half-Brite) via palette expansion
 *   - Picture palette mode (embed IFF's own CMAP into icon)
 *
 * Does NOT support (stubs with graceful skip):
 *   - HAM (Hold-And-Modify) — returns ITIDY_PREVIEW_NOT_SUPPORTED
 *   - Screen palette mode — falls back to picture palette
 *
 * Uses the shared iTidy_RenderParams infrastructure and two-icon-merge
 * architecture from icon_image_access.h.
 *
 * See: docs/Default icons/Customize icons/IFF_Thumbnail_Implementation_Plan.md
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef ICON_IFF_RENDER_H
#define ICON_IFF_RENDER_H

#include <exec/types.h>
#include <datatypes/pictureclass.h>  /* struct ColorRegister */
#include "icon_image_access.h"       /* iTidy_RenderParams, iTidy_IconImageData */

/*========================================================================*/
/* IFF Chunk IDs                                                          */
/*========================================================================*/

/** Standard ILBM chunk identifiers (4-byte ASCII packed into LONG) */
#ifndef ID_ILBM
#define ID_ILBM  MAKE_ID('I','L','B','M')
#endif
#ifndef ID_BMHD
#define ID_BMHD  MAKE_ID('B','M','H','D')
#endif
#ifndef ID_CMAP
#define ID_CMAP  MAKE_ID('C','M','A','P')
#endif
#ifndef ID_CAMG
#define ID_CAMG  MAKE_ID('C','A','M','G')
#endif
#ifndef ID_BODY
#define ID_BODY  MAKE_ID('B','O','D','Y')
#endif

/*========================================================================*/
/* CAMG Viewport Mode Flags                                               */
/*========================================================================*/

/** Interlace mode — display height is doubled vs visual height */
#define ITIDY_CAMG_LACE           0x0004UL

/** Hi-res mode (640px horizontal) */
#define ITIDY_CAMG_HIRES          0x8000UL

/** Hold-And-Modify (HAM) — NOT SUPPORTED, detect and skip */
#define ITIDY_CAMG_HAM            0x0800UL

/** Extra Half-Brite (EHB) — 64 colors via palette expansion */
#define ITIDY_CAMG_EHB            0x0080UL

/** Width threshold for inferring hires when CAMG flag is missing.
 *  Lowres max with extreme overscan is ~376; anything above 400 is hires. */
#define ITIDY_HIRES_WIDTH_THRESHOLD  400

/*========================================================================*/
/* Compression Modes                                                      */
/*========================================================================*/

/** No compression — BODY is raw interleaved bitplane data */
#define ITIDY_IFF_COMPRESS_NONE     0

/** ByteRun1 compression — standard ILBM run-length encoding */
#define ITIDY_IFF_COMPRESS_BYTERUN1 1

/*========================================================================*/
/* Scaling Thresholds                                                     */
/*========================================================================*/

/** Reduction ratio above which the 2x2 pre-filter is used */
#define ITIDY_SCALE_PREFILTER_RATIO     8

/** Minimum source pixel count to trigger pre-filter (below this, direct) */
#define ITIDY_SCALE_PREFILTER_MIN_AREA  65536UL

/*========================================================================*/
/* Return Codes (supplement icon_content_preview.h codes)                 */
/*========================================================================*/

/** IFF type detected but format not supported (HAM etc.) */
#define ITIDY_PREVIEW_NOT_SUPPORTED  4

/*========================================================================*/
/* iTidy_BMHD — BitMapHeader (IFF ILBM BMHD Chunk)                       */
/*========================================================================*/

/**
 * @brief IFF ILBM BitMapHeader — describes the image dimensions and format.
 *
 * Defined locally (with iTidy_ prefix) to avoid dependency on any
 * specific IFF header. The BMHD chunk data is read directly into
 * this structure.
 */
typedef struct {
    UWORD w;                    /* Raster width in pixels */
    UWORD h;                    /* Raster height in pixels */
    WORD  x;                    /* X position (usually 0) */
    WORD  y;                    /* Y position (usually 0) */
    UBYTE nPlanes;              /* Number of bitplanes (1-8 typically) */
    UBYTE masking;              /* 0=none, 1=hasMask, 2=hasTransparentColor, 3=lasso */
    UBYTE compression;          /* 0=none, 1=ByteRun1 */
    UBYTE pad1;                 /* Unused, for alignment */
    UWORD transparentColor;     /* Transparent color index */
    UBYTE xAspect;              /* Pixel aspect ratio X */
    UBYTE yAspect;              /* Pixel aspect ratio Y */
    WORD  pageWidth;            /* Source page width */
    WORD  pageHeight;           /* Source page height */
} iTidy_BMHD;

/*========================================================================*/
/* Progress Callback                                                      */
/*========================================================================*/

/**
 * @brief Progress callback function for long-running operations.
 *
 * Called periodically during IFF parsing, quantization, and scaling to
 * update the user interface with current progress. Implementations should
 * be fast and non-blocking.
 *
 * @param user_data   Opaque pointer passed through from caller
 * @param phase       Human-readable description of current phase
 * @param current     Current progress value (e.g., scanline number, pixel count)
 * @param total       Total expected value for this phase
 */
typedef void (*iTidy_ProgressCallback)(void *user_data,
                                        const char *phase,
                                        ULONG current,
                                        ULONG total);

/*========================================================================*/
/* iTidy_IFFRenderParams — IFF-Specific Render Parameters                 */
/*========================================================================*/

/**
 * @brief IFF-specific extension of the base render params.
 *
 * Wraps iTidy_RenderParams with fields for source image properties,
 * CAMG viewport mode, decoded pixel/palette data, and palette mode.
 */
typedef struct {
    iTidy_RenderParams base;            /* Embedded base (safe area, buffer, colors) */

    /* Source image properties (from BMHD chunk) */
    UWORD src_width;                    /* BMHD width */
    UWORD src_height;                   /* BMHD height */
    UWORD src_depth;                    /* BMHD nPlanes */
    UBYTE src_compression;              /* BMHD compression (0=none, 1=ByteRun1) */
    UBYTE src_masking;                  /* BMHD masking */

    /* CAMG viewport mode flags */
    ULONG camg_flags;                   /* Raw CAMG data (0 if no CAMG chunk) */
    BOOL is_interlace;                  /* ITIDY_CAMG_LACE detected */
    BOOL is_hires;                      /* ITIDY_CAMG_HIRES detected */
    BOOL is_ham;                        /* ITIDY_CAMG_HAM detected */
    BOOL is_ehb;                        /* ITIDY_CAMG_EHB detected */

    /* Corrected display dimensions (after aspect ratio adjustment) */
    UWORD display_width;                /* = src_width */
    UWORD display_height;              /* = src_height / 2 if interlace, else src_height */

    /* Source pixel and palette data (decoded from IFF) */
    UBYTE *src_chunky;                  /* Decoded chunky pixel buffer (one byte per pixel) */
    struct ColorRegister *src_palette;  /* CMAP palette (allocated copy) */
    ULONG src_palette_size;             /* Number of CMAP entries */

    /* Output dimensions (populated by itidy_render_iff_thumbnail) */
    UWORD output_width;                 /* Actual thumbnail width after scaling */
    UWORD output_height;                /* Actual thumbnail height after scaling */
    UWORD output_offset_x;             /* X pixel offset of thumbnail in buffer */
    UWORD output_offset_y;             /* Y pixel offset of thumbnail in buffer */

    /* Progress reporting */
    iTidy_ProgressCallback progress_callback;  /* Optional progress callback */
    void *progress_user_data;           /* Opaque data for callback */
    ULONG last_progress_ticks;          /* Timer for throttling updates (DOS ticks) */
    BOOL *cancel_flag;                  /* Optional pointer to cancel flag (may be NULL) */
} iTidy_IFFRenderParams;

/*========================================================================*/
/* Public API — IFF Parsing and Rendering                                 */
/*========================================================================*/

/**
 * @brief Parse an IFF ILBM file and decode to chunky pixels.
 *
 * Opens the file via iffparse.library, reads BMHD/CMAP/CAMG/BODY chunks,
 * decompresses ByteRun1 if needed, and converts interleaved bitplanes
 * to a chunky pixel buffer. Handles EHB palette expansion. Detects and
 * rejects HAM images.
 *
 * On success, populates iff_params with:
 *   - src_width, src_height, src_depth, src_compression, src_masking
 *   - camg_flags and boolean mode flags
 *   - display_width, display_height (after interlace correction)
 *   - src_chunky (allocated chunky pixel buffer — caller must whd_free)
 *   - src_palette, src_palette_size (allocated palette — caller must whd_free)
 *
 * @param source_path   Full Amiga path to the IFF ILBM file
 * @param iff_params    Output: populated IFF render params
 * @return 0 on success, negative on error, ITIDY_PREVIEW_NOT_SUPPORTED for HAM
 */
int itidy_iff_parse(const char *source_path, iTidy_IFFRenderParams *iff_params);

/**
 * @brief Render an IFF thumbnail into the icon pixel buffer.
 *
 * Complete rendering pipeline:
 *   1. Parse the IFF file (calls itidy_iff_parse)
 *   2. Calculate scaling to fit safe area with aspect ratio preservation
 *   3. Fill safe area with bg_color_index
 *   4. Area-average downscale source pixels into safe area
 *   5. In PICTURE mode: replace icon palette with source CMAP
 *   6. Free decoded source pixel buffer
 *
 * @param source_path   Full Amiga path to the IFF ILBM file
 * @param iff_params    Render params (base filled from template ToolTypes)
 * @param img           Icon image data (pixel buffer and palette to modify)
 * @return 0 on success, negative on error, ITIDY_PREVIEW_NOT_SUPPORTED for HAM
 */
int itidy_render_iff_thumbnail(const char *source_path,
                               iTidy_IFFRenderParams *iff_params,
                               iTidy_IconImageData *img);

/**
 * @brief Render IFF thumbnail via datatypes.library fallback.
 *
 * Used when the native parser cannot handle a format (e.g., HAM images).
 * Opens the image via picture.datatype, reads decoded RGB24 pixel data,
 * quantizes to the destination palette, and scales to fit.
 *
 * @param source_path   Full Amiga path to the IFF image file
 * @param params        Render params (safe area, palette mode, etc.)
 * @param dest_img      Destination image buffer to render into
 * @return 0 on success, ITIDY_PREVIEW_* error code on failure
 */
int itidy_render_via_datatype(const char *source_path,
                               iTidy_IFFRenderParams *params,
                               iTidy_IconImageData *dest_img);

/**
 * @brief Free IFF-specific allocated buffers.
 *
 * Frees src_chunky and src_palette in the IFF render params.
 * Safe to call on already-freed or zero-initialized params.
 *
 * @param params    IFF render params to clean up
 */
void itidy_iff_params_free(iTidy_IFFRenderParams *params);

/**
 * @brief Report progress if callback is set and throttle time has elapsed.
 *
 * Helper to invoke the progress callback only if:
 *  1. A callback is registered
 *  2. At least 'min_ticks' have elapsed since last update (for throttling)
 *  3. This is a forced update (current == total) OR throttle time passed
 *
 * Also checks the cancel flag if set and returns FALSE if operation was cancelled.
 *
 * @param params      IFF render params containing callback and timing state
 * @param phase       Human-readable phase description
 * @param current     Current progress value
 * @param total       Total expected value
 * @param min_ticks   Minimum ticks between updates (use TICKS_PER_SECOND for 1s)
 * @return TRUE to continue, FALSE if cancelled
 */
BOOL itidy_report_progress_throttled(iTidy_IFFRenderParams *params,
                                     const char *phase,
                                     ULONG current,
                                     ULONG total,
                                     ULONG min_ticks);

#endif /* ICON_IFF_RENDER_H */
