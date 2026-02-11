/*
 * icon_iff_render.c - IFF ILBM Thumbnail Renderer
 *
 * Implementation of IFF ILBM parsing (via iffparse.library), ByteRun1
 * decompression, bitplane-to-chunky conversion, CAMG viewport mode
 * detection, EHB palette expansion, and area-average downscaling.
 *
 * See: docs/Default icons/Customize icons/IFF_Thumbnail_Implementation_Plan.md
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stdio.h>
#include <string.h>

#include <libraries/iffparse.h>
#include <proto/iffparse.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <proto/datatypes.h>

#include <console_output.h>

#include "icon_iff_render.h"
#include "icon_image_access.h"
#include "../writeLog.h"

/*========================================================================*/
/* Progress Reporting Helper                                              */
/*========================================================================*/

/**
 * @brief Report progress with time-based throttling.
 *
 * Invokes the progress callback only if a callback is set AND either:
 *  - This is the final update (current == total), OR
 *  - At least min_ticks have elapsed since the last update
 *
 * Uses DOS ticks (TICKS_PER_SECOND = 50 on PAL, 60 on NTSC).
 */
BOOL itidy_report_progress_throttled(iTidy_IFFRenderParams *params,
                                     const char *phase,
                                     ULONG current,
                                     ULONG total,
                                     ULONG min_ticks)
{
    struct DateStamp ds;
    ULONG current_ticks;
    
    /* Check for cancellation first */
    if (params != NULL && params->cancel_flag != NULL && *params->cancel_flag)
    {
        return FALSE;  /* Operation cancelled */
    }
    
    if (params == NULL || params->progress_callback == NULL)
    {
        return TRUE;  /* No callback, continue */
    }
    
    /* Always report final progress */
    if (current >= total)
    {
        params->progress_callback(params->progress_user_data, phase, current, total);
        params->last_progress_ticks = 0;  /* Reset for next phase */
        
        /* Log final progress update */
        log_debug(LOG_ICONS, "Progress: %s: %lu / %lu (complete)\n",
                  phase ? phase : "Processing", current, total);
        return TRUE;
    }
    
    /* Check if enough time has elapsed since last update */
    /* DateStamp has ticks (1/50th or 1/60th second), convert to simple tick count */
    DateStamp(&ds);
    current_ticks = (ds.ds_Days * 24 * 60 * TICKS_PER_SECOND) +
                    (ds.ds_Minute * TICKS_PER_SECOND) +
                    ds.ds_Tick;
    
    if (current_ticks - params->last_progress_ticks >= min_ticks)
    {
        params->progress_callback(params->progress_user_data, phase, current, total);
        params->last_progress_ticks = current_ticks;
        
        /* Log progress update for post-analysis */
        log_debug(LOG_ICONS, "Progress: %s: %lu / %lu\n", 
                  phase ? phase : "Processing", current, total);
    }
    
    return TRUE;  /* Continue processing */
}

/*========================================================================*/
/* Internal Prototypes                                                    */
/*========================================================================*/

static BOOL validate_bmhd(const iTidy_BMHD *bmhd);
static BOOL validate_cmap(const UBYTE *cmap_data, ULONG cmap_size,
                          ULONG *out_palette_size);
static void decode_camg_flags(ULONG camg_raw, iTidy_IFFRenderParams *params);
static BOOL expand_ehb_palette(const struct ColorRegister *palette_32,
                               struct ColorRegister **out_palette_64,
                               ULONG *out_size);
static int decompress_byterun1(const UBYTE *src, ULONG src_len,
                               UBYTE *dest, ULONG dest_len);
static void planar_to_chunky(const UBYTE *planar_row, UWORD width,
                             UWORD num_planes, UBYTE *chunky_row);
static UWORD calculate_row_bytes(UWORD width);

/* Scaling — area-average downscaler with pre-filter fast path */
static void area_average_scale(const UBYTE *src_chunky,
                               UWORD src_w, UWORD src_h,
                               UBYTE *dest_buf, UWORD dest_w, UWORD dest_h,
                               UWORD dest_stride,
                               UWORD dest_offset_x, UWORD dest_offset_y,
                               const struct ColorRegister *src_palette,
                               ULONG src_palette_size,
                               const struct ColorRegister *dest_palette,
                               ULONG dest_palette_size,
                               iTidy_IFFRenderParams *params);
static BOOL prefilter_2x2(const UBYTE *src, UWORD src_w, UWORD src_h,
                          UBYTE **out_buf, UWORD *out_w, UWORD *out_h,
                          const struct ColorRegister *palette,
                          ULONG palette_size);

/*========================================================================*/
/* validate_bmhd — Check BMHD chunk for sane values                      */
/*========================================================================*/

static BOOL validate_bmhd(const iTidy_BMHD *bmhd)
{
    if (bmhd == NULL)
    {
        return FALSE;
    }

    // Width and height must be non-zero and reasonable
    if (bmhd->w == 0 || bmhd->h == 0)
    {
        log_error(LOG_ICONS, "validate_bmhd: zero dimensions (%ux%u)\n",
                  (unsigned)bmhd->w, (unsigned)bmhd->h);
        return FALSE;
    }

    // Clamp to reasonable maximum (4096x4096 is very generous for Amiga)
    if (bmhd->w > 4096 || bmhd->h > 4096)
    {
        log_error(LOG_ICONS, "validate_bmhd: dimensions too large (%ux%u)\n",
                  (unsigned)bmhd->w, (unsigned)bmhd->h);
        return FALSE;
    }

    // Depth must be 1-8 for standard palette images
    if (bmhd->nPlanes == 0 || bmhd->nPlanes > 8)
    {
        log_error(LOG_ICONS, "validate_bmhd: unsupported depth (%u planes)\n",
                  (unsigned)bmhd->nPlanes);
        return FALSE;
    }

    // Compression must be 0 (none) or 1 (ByteRun1)
    if (bmhd->compression > 1)
    {
        log_error(LOG_ICONS, "validate_bmhd: unsupported compression (%u)\n",
                  (unsigned)bmhd->compression);
        return FALSE;
    }

    return TRUE;
}

/*========================================================================*/
/* validate_cmap — Check CMAP chunk for usable palette data               */
/*========================================================================*/

static BOOL validate_cmap(const UBYTE *cmap_data, ULONG cmap_size,
                          ULONG *out_palette_size)
{
    ULONG entries;

    if (cmap_data == NULL || cmap_size < 6)
    {
        // Missing or tiny CMAP — need at least 2 RGB triplets (6 bytes)
        return FALSE;
    }

    entries = cmap_size / 3;  // Truncate incomplete triplets

    if (entries > 256)
    {
        entries = 256;  // Clamp to OS 3.5 icon limit
    }

    if (entries < 2)
    {
        return FALSE;  // Unusable — need at least 2 colors
    }

    *out_palette_size = entries;
    return TRUE;
}

/*========================================================================*/
/* decode_camg_flags — Extract viewport mode information                  */
/*========================================================================*/

static void decode_camg_flags(ULONG camg_raw, iTidy_IFFRenderParams *params)
{
    params->camg_flags = camg_raw;
    params->is_interlace = (camg_raw & ITIDY_CAMG_LACE) ? TRUE : FALSE;
    params->is_hires     = (camg_raw & ITIDY_CAMG_HIRES) ? TRUE : FALSE;
    params->is_ham       = (camg_raw & ITIDY_CAMG_HAM) ? TRUE : FALSE;
    params->is_ehb       = (camg_raw & ITIDY_CAMG_EHB) ? TRUE : FALSE;

    log_info(LOG_ICONS, "decode_camg_flags: raw=0x%08lX lace=%s hires=%s ham=%s ehb=%s\n",
             camg_raw,
             params->is_interlace ? "yes" : "no",
             params->is_hires ? "yes" : "no",
             params->is_ham ? "yes" : "no",
             params->is_ehb ? "yes" : "no");
}

/*========================================================================*/
/* expand_ehb_palette — Generate 64-color EHB palette from 32 colors      */
/*========================================================================*/

static BOOL expand_ehb_palette(const struct ColorRegister *palette_32,
                               struct ColorRegister **out_palette_64,
                               ULONG *out_size)
{
    struct ColorRegister *pal64;
    int i;

    pal64 = (struct ColorRegister *)whd_malloc(64 * sizeof(struct ColorRegister));
    if (pal64 == NULL)
    {
        log_error(LOG_ICONS, "expand_ehb_palette: alloc failed\n");
        return FALSE;
    }

    // Copy original 32 colors
    memcpy(pal64, palette_32, 32 * sizeof(struct ColorRegister));

    // Generate half-bright copies for indices 32-63
    for (i = 0; i < 32; i++)
    {
        pal64[32 + i].red   = palette_32[i].red   >> 1;
        pal64[32 + i].green = palette_32[i].green >> 1;
        pal64[32 + i].blue  = palette_32[i].blue  >> 1;
    }

    *out_palette_64 = pal64;
    *out_size = 64;

    log_debug(LOG_ICONS, "expand_ehb_palette: expanded 32 -> 64 colors\n");
    return TRUE;
}

/*========================================================================*/
/* calculate_row_bytes — Word-aligned row stride per bitplane             */
/*========================================================================*/

/**
 * @brief Calculate the byte width of one bitplane row (word-aligned).
 *
 * Each bitplane row is padded to an even number of bytes (word boundary).
 * Formula: ((width + 15) / 16) * 2
 */
static UWORD calculate_row_bytes(UWORD width)
{
    return ((width + 15) / 16) * 2;
}

/*========================================================================*/
/* decompress_byterun1 — ByteRun1 decompression                          */
/*========================================================================*/

/**
 * @brief Decompress ByteRun1 encoded data.
 *
 * ByteRun1 algorithm:
 *   n >= 0 and n <= 127: copy next (n+1) bytes literally
 *   n >= -127 and n <= -1: replicate next byte (-n+1) times
 *   n == -128: no-op (skip)
 *
 * @param src       Compressed source data
 * @param src_len   Length of compressed data in bytes
 * @param dest      Destination buffer for decompressed data
 * @param dest_len  Size of destination buffer
 * @return Number of bytes written to dest, or -1 on error
 */
static int decompress_byterun1(const UBYTE *src, ULONG src_len,
                               UBYTE *dest, ULONG dest_len)
{
    ULONG src_pos = 0;
    ULONG dest_pos = 0;

    while (src_pos < src_len && dest_pos < dest_len)
    {
        BYTE n = (BYTE)src[src_pos++];

        if (n >= 0)
        {
            // Literal run: copy next (n+1) bytes
            ULONG count = (ULONG)n + 1;
            ULONG i;

            if (src_pos + count > src_len || dest_pos + count > dest_len)
            {
                log_warning(LOG_ICONS, "decompress_byterun1: literal overrun at src=%lu dest=%lu count=%lu\n",
                            src_pos, dest_pos, count);
                break;
            }

            for (i = 0; i < count; i++)
            {
                dest[dest_pos++] = src[src_pos++];
            }
        }
        else if (n != -128)
        {
            // Repeat run: replicate next byte (-n+1) times
            ULONG count = (ULONG)(-(int)n) + 1;
            UBYTE value;
            ULONG i;

            if (src_pos >= src_len || dest_pos + count > dest_len)
            {
                log_warning(LOG_ICONS, "decompress_byterun1: repeat overrun at src=%lu dest=%lu count=%lu\n",
                            src_pos, dest_pos, count);
                break;
            }

            value = src[src_pos++];
            for (i = 0; i < count; i++)
            {
                dest[dest_pos++] = value;
            }
        }
        // n == -128: no-op, continue
    }

    return (int)dest_pos;
}

/*========================================================================*/
/* planar_to_chunky — Convert one row of interleaved bitplane data        */
/*========================================================================*/

/**
 * @brief Convert one row of bitplane data to chunky pixels.
 *
 * Expects planar_row to contain all bitplane rows for this scanline
 * contiguously: plane0_data, plane1_data, ..., planeN_data.
 * Each plane's data is calculate_row_bytes(width) bytes wide.
 *
 * @param planar_row    Pointer to interleaved plane data for one row
 * @param width         Image width in pixels
 * @param num_planes    Number of bitplanes (1-8)
 * @param chunky_row    Output: one byte per pixel (palette indices)
 */
static void planar_to_chunky(const UBYTE *planar_row, UWORD width,
                             UWORD num_planes, UBYTE *chunky_row)
{
    UWORD row_bytes = calculate_row_bytes(width);
    UWORD x;

    for (x = 0; x < width; x++)
    {
        UWORD byte_idx = x >> 3;            // x / 8
        UBYTE bit_mask = 0x80 >> (x & 7);   // MSB first: 7 - (x % 8)
        UBYTE pixel = 0;
        UWORD plane;

        for (plane = 0; plane < num_planes; plane++)
        {
            if (planar_row[plane * row_bytes + byte_idx] & bit_mask)
            {
                pixel |= (1 << plane);
            }
        }

        chunky_row[x] = pixel;
    }
}

/*========================================================================*/
/* itidy_iff_parse — Parse IFF ILBM and decode to chunky pixels           */
/*========================================================================*/

int itidy_iff_parse(const char *source_path, iTidy_IFFRenderParams *iff_params)
{
    struct Library *IFFParseBase = NULL;
    struct IFFHandle *iff = NULL;
    BPTR file = 0;
    struct StoredProperty *bmhd_sp = NULL;
    struct StoredProperty *cmap_sp = NULL;
    struct StoredProperty *camg_sp = NULL;
    iTidy_BMHD bmhd;
    ULONG cmap_entries = 0;
    UWORD row_bytes;
    ULONG body_row_size;
    ULONG total_planar_size;
    UBYTE *body_buf = NULL;
    UBYTE *planar_buf = NULL;
    UBYTE *chunky_buf = NULL;
    ULONG body_read;
    LONG parse_result;
    UWORD y;
    int result = -1;

    if (source_path == NULL || iff_params == NULL)
    {
        log_error(LOG_ICONS, "itidy_iff_parse: NULL parameter\n");
        return -1;
    }

    /*--------------------------------------------------------------------*/
    /* Step 1: Open iffparse.library                                      */
    /*--------------------------------------------------------------------*/

    IFFParseBase = OpenLibrary("iffparse.library", 39);
    if (IFFParseBase == NULL)
    {
        log_error(LOG_ICONS, "itidy_iff_parse: cannot open iffparse.library V39\n");
        return -1;
    }

    /*--------------------------------------------------------------------*/
    /* Step 2: Allocate IFF handle and open file                          */
    /*--------------------------------------------------------------------*/

    iff = AllocIFF();
    if (iff == NULL)
    {
        log_error(LOG_ICONS, "itidy_iff_parse: AllocIFF failed\n");
        CloseLibrary(IFFParseBase);
        return -1;
    }

    file = Open((STRPTR)source_path, MODE_OLDFILE);
    if (file == 0)
    {
        log_error(LOG_ICONS, "itidy_iff_parse: cannot open '%s'\n", source_path);
        FreeIFF(iff);
        CloseLibrary(IFFParseBase);
        return -1;
    }

    iff->iff_Stream = (ULONG)file;
    InitIFFasDOS(iff);

    if (OpenIFF(iff, IFFF_READ) != 0)
    {
        log_error(LOG_ICONS, "itidy_iff_parse: OpenIFF failed for '%s'\n", source_path);
        Close(file);
        FreeIFF(iff);
        CloseLibrary(IFFParseBase);
        return -1;
    }

    /*--------------------------------------------------------------------*/
    /* Step 3: Register property and stop chunks                          */
    /*--------------------------------------------------------------------*/

    PropChunk(iff, ID_ILBM, ID_BMHD);
    PropChunk(iff, ID_ILBM, ID_CMAP);
    PropChunk(iff, ID_ILBM, ID_CAMG);
    StopChunk(iff, ID_ILBM, ID_BODY);

    /*--------------------------------------------------------------------*/
    /* Step 4: Parse to BODY chunk                                        */
    /*--------------------------------------------------------------------*/

    parse_result = ParseIFF(iff, IFFPARSE_SCAN);
    if (parse_result != 0)
    {
        log_error(LOG_ICONS, "itidy_iff_parse: ParseIFF failed (%ld) for '%s'\n",
                  parse_result, source_path);
        goto cleanup;
    }

    /*--------------------------------------------------------------------*/
    /* Step 5: Extract BMHD                                               */
    /*--------------------------------------------------------------------*/

    bmhd_sp = FindProp(iff, ID_ILBM, ID_BMHD);
    if (bmhd_sp == NULL || bmhd_sp->sp_Size < (LONG)sizeof(iTidy_BMHD))
    {
        log_error(LOG_ICONS, "itidy_iff_parse: missing or short BMHD in '%s'\n",
                  source_path);
        goto cleanup;
    }

    memcpy(&bmhd, bmhd_sp->sp_Data, sizeof(iTidy_BMHD));

    if (!validate_bmhd(&bmhd))
    {
        log_error(LOG_ICONS, "itidy_iff_parse: invalid BMHD in '%s'\n", source_path);
        goto cleanup;
    }

    iff_params->src_width       = bmhd.w;
    iff_params->src_height      = bmhd.h;
    iff_params->src_depth       = bmhd.nPlanes;
    iff_params->src_compression = bmhd.compression;
    iff_params->src_masking     = bmhd.masking;

    log_info(LOG_ICONS, "itidy_iff_parse: BMHD %ux%u depth=%u comp=%u mask=%u '%s'\n",
             (unsigned)bmhd.w, (unsigned)bmhd.h,
             (unsigned)bmhd.nPlanes, (unsigned)bmhd.compression,
             (unsigned)bmhd.masking, source_path);

    /*--------------------------------------------------------------------*/
    /* Step 6: Extract CAMG (optional)                                    */
    /*--------------------------------------------------------------------*/

    camg_sp = FindProp(iff, ID_ILBM, ID_CAMG);
    if (camg_sp != NULL && camg_sp->sp_Size >= 4)
    {
        ULONG camg_raw;
        UBYTE *camg_bytes = (UBYTE *)camg_sp->sp_Data;
        memcpy(&camg_raw, camg_sp->sp_Data, 4);
        log_info(LOG_ICONS, "itidy_iff_parse: CAMG bytes=%02X %02X %02X %02X (raw=0x%08lX) '%s'\n",
                 camg_bytes[0], camg_bytes[1], camg_bytes[2], camg_bytes[3],
                 camg_raw, source_path);
        decode_camg_flags(camg_raw, iff_params);
    }
    else
    {
        // No CAMG chunk — assume non-interlace, no special modes
        log_debug(LOG_ICONS, "itidy_iff_parse: no CAMG chunk, assuming standard mode\n");
        decode_camg_flags(0, iff_params);
    }

    /*--------------------------------------------------------------------*/
    /* Step 6b: Infer hires from width if CAMG flag is missing             */
    /*                                                                    */
    /* Many old Amiga IFF tools wrote incorrect CAMG chunks, omitting the  */
    /* HIRES flag even for 640+ pixel wide images. Fall back to width.     */
    /*--------------------------------------------------------------------*/

    if (!iff_params->is_hires && bmhd.w > ITIDY_HIRES_WIDTH_THRESHOLD)
    {
        iff_params->is_hires = TRUE;
        log_info(LOG_ICONS, "itidy_iff_parse: inferred hires from width %u "
                 "(CAMG lacked HIRES flag) '%s'\n",
                 (unsigned)bmhd.w, source_path);
    }

    // Check for HAM — not supported
    if (iff_params->is_ham)
    {
        log_info(LOG_ICONS, "itidy_iff_parse: HAM image detected — "
                 "thumbnail not yet supported: '%s'\n", source_path);
        result = ITIDY_PREVIEW_NOT_SUPPORTED;
        goto cleanup;
    }

    /*--------------------------------------------------------------------*/
    /* Step 7: Extract CMAP                                               */
    /*--------------------------------------------------------------------*/

    cmap_sp = FindProp(iff, ID_ILBM, ID_CMAP);
    if (!validate_cmap(cmap_sp ? (const UBYTE *)cmap_sp->sp_Data : NULL,
                       cmap_sp ? cmap_sp->sp_Size : 0,
                       &cmap_entries))
    {
        log_error(LOG_ICONS, "itidy_iff_parse: missing or invalid CMAP in '%s'\n",
                  source_path);
        goto cleanup;
    }

    // Allocate and copy palette
    iff_params->src_palette = (struct ColorRegister *)whd_malloc(
        cmap_entries * sizeof(struct ColorRegister));
    if (iff_params->src_palette == NULL)
    {
        log_error(LOG_ICONS, "itidy_iff_parse: palette alloc failed\n");
        goto cleanup;
    }

    memcpy(iff_params->src_palette, cmap_sp->sp_Data,
           cmap_entries * sizeof(struct ColorRegister));
    iff_params->src_palette_size = cmap_entries;

    log_debug(LOG_ICONS, "itidy_iff_parse: CMAP %lu entries\n", cmap_entries);

    // EHB: expand 32-color palette to 64 colors
    if (iff_params->is_ehb && cmap_entries <= 32)
    {
        struct ColorRegister *ehb_palette = NULL;
        ULONG ehb_size = 0;

        if (expand_ehb_palette(iff_params->src_palette, &ehb_palette, &ehb_size))
        {
            whd_free(iff_params->src_palette);
            iff_params->src_palette = ehb_palette;
            iff_params->src_palette_size = ehb_size;
        }
        else
        {
            log_warning(LOG_ICONS, "itidy_iff_parse: EHB palette expansion failed, "
                        "using original 32 colors\n");
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 8: Decode BODY — interleaved bitplanes to chunky pixels       */
    /*--------------------------------------------------------------------*/

    row_bytes = calculate_row_bytes(bmhd.w);

    // Number of planes to read per row (include mask plane if present)
    {
        UWORD planes_per_row = bmhd.nPlanes;
        if (bmhd.masking == 1)
        {
            planes_per_row++;  // Extra mask plane per row
        }

        body_row_size = (ULONG)row_bytes * (ULONG)planes_per_row;
        total_planar_size = body_row_size * (ULONG)bmhd.h;
    }

    // Allocate chunky output buffer
    chunky_buf = (UBYTE *)whd_malloc((ULONG)bmhd.w * (ULONG)bmhd.h);
    if (chunky_buf == NULL)
    {
        log_error(LOG_ICONS, "itidy_iff_parse: chunky buffer alloc failed (%lux%lu)\n",
                  (ULONG)bmhd.w, (ULONG)bmhd.h);
        goto cleanup;
    }

    if (bmhd.compression == ITIDY_IFF_COMPRESS_NONE)
    {
        /*----------------------------------------------------------------*/
        /* Uncompressed: read each row and convert                        */
        /*----------------------------------------------------------------*/

        planar_buf = (UBYTE *)whd_malloc(body_row_size);
        if (planar_buf == NULL)
        {
            log_error(LOG_ICONS, "itidy_iff_parse: planar row buffer alloc failed\n");
            goto cleanup;
        }

        for (y = 0; y < bmhd.h; y++)
        {
            LONG bytes_read = ReadChunkBytes(iff, planar_buf, body_row_size);
            if (bytes_read < (LONG)body_row_size)
            {
                log_warning(LOG_ICONS, "itidy_iff_parse: short BODY read at row %u "
                            "(got %ld, expected %lu)\n",
                            (unsigned)y, bytes_read, body_row_size);
                // Fill remaining rows with 0 and break
                memset(chunky_buf + (ULONG)y * (ULONG)bmhd.w, 0,
                       (ULONG)(bmhd.h - y) * (ULONG)bmhd.w);
                break;
            }

            planar_to_chunky(planar_buf, bmhd.w, bmhd.nPlanes,
                             chunky_buf + (ULONG)y * (ULONG)bmhd.w);

            /* Report progress every 20 rows (throttled to ~1 second) */
            if (y % 20 == 0 || y == bmhd.h - 1)
            {
                if (!itidy_report_progress_throttled(iff_params, "Decoding pixels",
                                                     y + 1, bmhd.h, TICKS_PER_SECOND))
                {
                    log_info(LOG_ICONS, "itidy_iff_parse: cancelled by user at row %u\n", (unsigned)y);
                    result = -1;  /* Cancelled */
                    goto cleanup;
                }
            }
        }
    }
    else if (bmhd.compression == ITIDY_IFF_COMPRESS_BYTERUN1)
    {
        /*----------------------------------------------------------------*/
        /* ByteRun1: read entire BODY, decompress row-by-row              */
        /*----------------------------------------------------------------*/

        // Read entire BODY into a temporary buffer
        // Estimate compressed size: use the current chunk context
        struct ContextNode *body_cn = CurrentChunk(iff);
        ULONG body_size;

        if (body_cn == NULL)
        {
            log_error(LOG_ICONS, "itidy_iff_parse: cannot get BODY chunk size\n");
            goto cleanup;
        }

        body_size = body_cn->cn_Size;
        body_buf = (UBYTE *)whd_malloc(body_size);
        if (body_buf == NULL)
        {
            log_error(LOG_ICONS, "itidy_iff_parse: BODY buffer alloc failed (%lu bytes)\n",
                      body_size);
            goto cleanup;
        }

        body_read = (ULONG)ReadChunkBytes(iff, body_buf, body_size);
        if (body_read == 0)
        {
            log_error(LOG_ICONS, "itidy_iff_parse: BODY read returned 0 bytes\n");
            goto cleanup;
        }

        log_debug(LOG_ICONS, "itidy_iff_parse: BODY %lu bytes read (ByteRun1)\n", body_read);

        // Allocate buffer for one decompressed row (all planes)
        planar_buf = (UBYTE *)whd_malloc(body_row_size);
        if (planar_buf == NULL)
        {
            log_error(LOG_ICONS, "itidy_iff_parse: planar decompressed buffer alloc failed\n");
            goto cleanup;
        }

        // Decompress row by row
        {
            ULONG src_pos = 0;

            for (y = 0; y < bmhd.h; y++)
            {
                int decomp_bytes;

                if (src_pos >= body_read)
                {
                    log_warning(LOG_ICONS, "itidy_iff_parse: compressed data exhausted at row %u\n",
                                (unsigned)y);
                    memset(chunky_buf + (ULONG)y * (ULONG)bmhd.w, 0,
                           (ULONG)(bmhd.h - y) * (ULONG)bmhd.w);
                    break;
                }

                decomp_bytes = decompress_byterun1(
                    body_buf + src_pos, body_read - src_pos,
                    planar_buf, body_row_size);

                if (decomp_bytes <= 0)
                {
                    log_warning(LOG_ICONS, "itidy_iff_parse: ByteRun1 failed at row %u\n",
                                (unsigned)y);
                    memset(chunky_buf + (ULONG)y * (ULONG)bmhd.w, 0,
                           (ULONG)(bmhd.h - y) * (ULONG)bmhd.w);
                    break;
                }

                planar_to_chunky(planar_buf, bmhd.w, bmhd.nPlanes,
                                 chunky_buf + (ULONG)y * (ULONG)bmhd.w);

                /* Report progress every 20 rows (throttled to ~1 second) */
                if (y % 20 == 0 || y == bmhd.h - 1)
                {
                    if (!itidy_report_progress_throttled(iff_params, "Decoding pixels",
                                                         y + 1, bmhd.h, TICKS_PER_SECOND))
                    {
                        log_info(LOG_ICONS, "itidy_iff_parse: cancelled by user at row %u\n", (unsigned)y);
                        result = -1;  /* Cancelled */
                        goto cleanup;
                    }
                }

                // Advance source position past the compressed data consumed.
                // ByteRun1 decompresses exactly body_row_size bytes of output.
                // We need to track how many compressed bytes were consumed.
                // Unfortunately our decompress function returns output bytes.
                // We need a version that also reports input consumption.
                // For now, re-scan to count consumed bytes.
                {
                    ULONG consumed = 0;
                    ULONG out_count = 0;
                    const UBYTE *sp = body_buf + src_pos;
                    ULONG sp_remain = body_read - src_pos;

                    while (consumed < sp_remain && out_count < body_row_size)
                    {
                        BYTE n = (BYTE)sp[consumed++];

                        if (n >= 0)
                        {
                            ULONG count = (ULONG)n + 1;
                            consumed += count;
                            out_count += count;
                        }
                        else if (n != -128)
                        {
                            ULONG count = (ULONG)(-(int)n) + 1;
                            consumed++;
                            out_count += count;
                        }
                    }

                    src_pos += consumed;
                }
            }
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 9: Calculate display dimensions (aspect ratio correction)     */
    /*                                                                    */
    /* Amiga display modes have non-square pixels:                        */
    /*   Lowres (320px wide): ~1:1 pixel aspect on a standard monitor     */
    /*   Hires (640px wide):  pixels are half as wide → halve width       */
    /*   Interlace (2× lines): pixels are half as tall → halve height     */
    /*   Hires+Lace (e.g. 672×446): halve BOTH width and height          */
    /*                                                                    */
    /* The source chunky buffer retains the full BMHD dimensions; the     */
    /* area-average scaler will naturally average over the denser source   */
    /* samples when display_width < src_width or display_height < src_h.  */
    /*--------------------------------------------------------------------*/

    if (iff_params->is_hires)
    {
        iff_params->display_width = bmhd.w / 2;
        if (iff_params->display_width == 0)
        {
            iff_params->display_width = 1;
        }
        log_debug(LOG_ICONS, "itidy_iff_parse: hires correction: %u -> %u width\n",
                  (unsigned)bmhd.w, (unsigned)iff_params->display_width);
    }
    else
    {
        iff_params->display_width = bmhd.w;
    }

    if (iff_params->is_interlace)
    {
        iff_params->display_height = bmhd.h / 2;
        if (iff_params->display_height == 0)
        {
            iff_params->display_height = 1;
        }
        log_debug(LOG_ICONS, "itidy_iff_parse: interlace correction: %u -> %u height\n",
                  (unsigned)bmhd.h, (unsigned)iff_params->display_height);
    }
    else
    {
        iff_params->display_height = bmhd.h;
    }

    // Store chunky buffer in params
    iff_params->src_chunky = chunky_buf;
    chunky_buf = NULL;  // Transfer ownership — don't free in cleanup

    result = 0;  // Success

    log_info(LOG_ICONS, "itidy_iff_parse: decoded %ux%u (%u planes) -> "
             "display %ux%u, palette %lu colors '%s'\n",
             (unsigned)bmhd.w, (unsigned)bmhd.h, (unsigned)bmhd.nPlanes,
             (unsigned)iff_params->display_width,
             (unsigned)iff_params->display_height,
             iff_params->src_palette_size, source_path);

cleanup:
    // Free working buffers
    if (planar_buf != NULL)
    {
        whd_free(planar_buf);
    }
    if (body_buf != NULL)
    {
        whd_free(body_buf);
    }
    if (chunky_buf != NULL)
    {
        whd_free(chunky_buf);  // Only if we didn't transfer ownership
    }

    // Close IFF (CRITICAL ORDER: CloseIFF -> Close file -> FreeIFF -> CloseLibrary)
    CloseIFF(iff);
    if (file != 0)
    {
        Close(file);
    }
    FreeIFF(iff);
    CloseLibrary(IFFParseBase);

    // On error, free any allocated palette
    if (result != 0 && iff_params->src_palette != NULL)
    {
        whd_free(iff_params->src_palette);
        iff_params->src_palette = NULL;
        iff_params->src_palette_size = 0;
    }

    return result;
}

/*========================================================================*/
/* Scaling — Area-Average Downscaler (Fixed-Point 16.16)                  */
/*========================================================================*/

/**
 * @brief Area-average downscale source image into destination buffer.
 *
 * Uses fixed-point (16.16) arithmetic only — no floating point.
 * Each output pixel maps to a rectangle in the source image. All source
 * pixels within that rectangle have their RGB values looked up via
 * src_palette, summed, and averaged. The resulting average RGB is then
 * matched to the closest entry in dest_palette.
 *
 * In PICTURE palette mode (src_palette == dest_palette), the source pixel
 * indices are used directly for palette lookup and the dest_palette match
 * finds the same or nearest index — this is the common fast path.
 *
 * @param src_chunky       Source chunky pixel buffer (palette indices)
 * @param src_w            Source image width
 * @param src_h            Source image height
 * @param dest_buf         Destination pixel buffer (full icon buffer)
 * @param dest_w           Output thumbnail width (within safe area)
 * @param dest_h           Output thumbnail height (within safe area)
 * @param dest_stride      Destination buffer row stride (= icon width)
 * @param dest_offset_x    X offset in dest buffer for thumbnail placement
 * @param dest_offset_y    Y offset in dest buffer for thumbnail placement
 * @param src_palette      Source image palette (for RGB lookup)
 * @param src_palette_size Number of entries in source palette
 * @param dest_palette     Destination icon palette (for color matching)
 * @param dest_palette_size Number of entries in destination palette
 * @param params           IFF render params (for progress reporting, may be NULL)
 */
static void area_average_scale(const UBYTE *src_chunky,
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
    UWORD out_x, out_y;
    ULONG scale_x_fp;  // Fixed 16.16: src pixels per output pixel (horizontal)
    ULONG scale_y_fp;  // Fixed 16.16: src pixels per output pixel (vertical)

    if (dest_w == 0 || dest_h == 0 || src_w == 0 || src_h == 0)
    {
        return;
    }

    if (src_palette == NULL || dest_palette == NULL)
    {
        log_warning(LOG_ICONS, "area_average_scale: NULL palette\n");
        return;
    }

    // Fixed-point 16.16 scale factors: how many source pixels per output pixel
    scale_x_fp = ((ULONG)src_w << 16) / (ULONG)dest_w;
    scale_y_fp = ((ULONG)src_h << 16) / (ULONG)dest_h;

    for (out_y = 0; out_y < dest_h; out_y++)
    {
        // Source Y range for this output row (fixed-point -> integer bounds)
        ULONG src_y_start_fp = (ULONG)out_y * scale_y_fp;
        ULONG src_y_end_fp   = src_y_start_fp + scale_y_fp;
        UWORD sy_start = (UWORD)(src_y_start_fp >> 16);
        UWORD sy_end   = (UWORD)(src_y_end_fp >> 16);

        // Clamp to source bounds
        if (sy_end > src_h) sy_end = src_h;
        if (sy_start >= src_h) sy_start = src_h - 1;
        // Ensure at least one row is sampled
        if (sy_end <= sy_start) sy_end = sy_start + 1;

        for (out_x = 0; out_x < dest_w; out_x++)
        {
            // Source X range for this output column
            ULONG src_x_start_fp = (ULONG)out_x * scale_x_fp;
            ULONG src_x_end_fp   = src_x_start_fp + scale_x_fp;
            UWORD sx_start = (UWORD)(src_x_start_fp >> 16);
            UWORD sx_end   = (UWORD)(src_x_end_fp >> 16);

            ULONG sum_r = 0, sum_g = 0, sum_b = 0;
            ULONG pixel_count = 0;
            UWORD sy, sx;
            UBYTE avg_r, avg_g, avg_b;
            UBYTE best_idx;
            ULONG dest_offset;

            // Clamp to source bounds
            if (sx_end > src_w) sx_end = src_w;
            if (sx_start >= src_w) sx_start = src_w - 1;
            if (sx_end <= sx_start) sx_end = sx_start + 1;

            // Sum RGB values of all source pixels in this rectangle
            for (sy = sy_start; sy < sy_end; sy++)
            {
                const UBYTE *src_row = src_chunky + (ULONG)sy * (ULONG)src_w;

                for (sx = sx_start; sx < sx_end; sx++)
                {
                    UBYTE idx = src_row[sx];

                    // Clamp index to palette size
                    if ((ULONG)idx >= src_palette_size)
                    {
                        idx = 0;
                    }

                    sum_r += src_palette[idx].red;
                    sum_g += src_palette[idx].green;
                    sum_b += src_palette[idx].blue;
                    pixel_count++;
                }
            }

            // Average the accumulated RGB
            if (pixel_count > 0)
            {
                avg_r = (UBYTE)(sum_r / pixel_count);
                avg_g = (UBYTE)(sum_g / pixel_count);
                avg_b = (UBYTE)(sum_b / pixel_count);
            }
            else
            {
                avg_r = 0;
                avg_g = 0;
                avg_b = 0;
            }

            // Find closest match in the destination palette
            best_idx = itidy_find_closest_palette_color(
                dest_palette, dest_palette_size, avg_r, avg_g, avg_b);

            // Write to destination buffer at the correct offset
            dest_offset = (ULONG)(dest_offset_y + out_y) * (ULONG)dest_stride
                        + (ULONG)(dest_offset_x + out_x);
            dest_buf[dest_offset] = best_idx;
        }

        /* Report progress every 5 rows (throttled to ~1 second) */
        if (params != NULL && (out_y % 5 == 0 || out_y == dest_h - 1))
        {
            if (!itidy_report_progress_throttled(params, "Scaling image",
                                                out_y + 1, dest_h, TICKS_PER_SECOND))
            {
                log_info(LOG_ICONS, "area_average_scale: cancelled by user at row %u\n", (unsigned)out_y);
                return;  /* Abort scaling */
            }
        }
    }

    log_debug(LOG_ICONS, "area_average_scale: %ux%u -> %ux%u (scale_x=0x%lX scale_y=0x%lX)\n",
              (unsigned)src_w, (unsigned)src_h,
              (unsigned)dest_w, (unsigned)dest_h,
              scale_x_fp, scale_y_fp);
}

/*========================================================================*/
/* prefilter_2x2 — Half-size pre-filter for extreme reduction ratios      */
/*========================================================================*/

/**
 * @brief Pre-filter: average 2×2 blocks of source pixels to half-size buffer.
 *
 * For extreme reduction ratios (>8:1 on large images), this pre-filter
 * halves the source dimensions first. The area-average scaler then works
 * on the smaller buffer, capping per-pixel work and keeping quality high.
 *
 * Each 2×2 block of source pixels is converted to RGB via palette lookup,
 * averaged, and the closest palette index is stored in the output buffer.
 *
 * @param src           Source chunky pixel buffer (palette indices)
 * @param src_w         Source width
 * @param src_h         Source height
 * @param out_buf       Output: allocated half-size buffer (caller must whd_free)
 * @param out_w         Output: half width
 * @param out_h         Output: half height
 * @param palette       Palette for RGB lookup
 * @param palette_size  Number of palette entries
 * @return TRUE on success, FALSE on allocation failure
 */
static BOOL prefilter_2x2(const UBYTE *src, UWORD src_w, UWORD src_h,
                          UBYTE **out_buf, UWORD *out_w, UWORD *out_h,
                          const struct ColorRegister *palette,
                          ULONG palette_size)
{
    UWORD half_w, half_h;
    UBYTE *half_buf;
    UWORD hx, hy;

    *out_buf = NULL;
    *out_w = 0;
    *out_h = 0;

    if (src == NULL || palette == NULL || src_w < 2 || src_h < 2)
    {
        return FALSE;
    }

    half_w = src_w / 2;
    half_h = src_h / 2;

    half_buf = (UBYTE *)whd_malloc((ULONG)half_w * (ULONG)half_h);
    if (half_buf == NULL)
    {
        log_error(LOG_ICONS, "prefilter_2x2: alloc failed (%ux%u)\n",
                  (unsigned)half_w, (unsigned)half_h);
        return FALSE;
    }

    for (hy = 0; hy < half_h; hy++)
    {
        UWORD src_y = hy * 2;
        const UBYTE *row0 = src + (ULONG)src_y * (ULONG)src_w;
        const UBYTE *row1 = src + (ULONG)(src_y + 1) * (ULONG)src_w;

        for (hx = 0; hx < half_w; hx++)
        {
            UWORD src_x = hx * 2;
            UBYTE i0 = row0[src_x];
            UBYTE i1 = row0[src_x + 1];
            UBYTE i2 = row1[src_x];
            UBYTE i3 = row1[src_x + 1];
            ULONG avg_r, avg_g, avg_b;

            // Clamp indices to palette size
            if ((ULONG)i0 >= palette_size) i0 = 0;
            if ((ULONG)i1 >= palette_size) i1 = 0;
            if ((ULONG)i2 >= palette_size) i2 = 0;
            if ((ULONG)i3 >= palette_size) i3 = 0;

            // Average the 4 pixels' RGB values
            avg_r = ((ULONG)palette[i0].red   + palette[i1].red
                   + palette[i2].red   + palette[i3].red) / 4;
            avg_g = ((ULONG)palette[i0].green + palette[i1].green
                   + palette[i2].green + palette[i3].green) / 4;
            avg_b = ((ULONG)palette[i0].blue  + palette[i1].blue
                   + palette[i2].blue  + palette[i3].blue) / 4;

            // Find closest palette match for the averaged color
            half_buf[(ULONG)hy * (ULONG)half_w + hx] =
                itidy_find_closest_palette_color(palette, palette_size,
                                                (UBYTE)avg_r, (UBYTE)avg_g,
                                                (UBYTE)avg_b);
        }
    }

    *out_buf = half_buf;
    *out_w = half_w;
    *out_h = half_h;

    log_debug(LOG_ICONS, "prefilter_2x2: %ux%u -> %ux%u\n",
              (unsigned)src_w, (unsigned)src_h,
              (unsigned)half_w, (unsigned)half_h);

    return TRUE;
}

/*========================================================================*/
/* itidy_render_iff_thumbnail — Complete IFF rendering pipeline            */
/*========================================================================*/

int itidy_render_iff_thumbnail(const char *source_path,
                               iTidy_IFFRenderParams *iff_params,
                               iTidy_IconImageData *img)
{
    int parse_result;
    UWORD output_width, output_height;
    UWORD offset_x, offset_y;
    ULONG x_scale_fp, y_scale_fp, scale_fp;

    if (source_path == NULL || iff_params == NULL || img == NULL)
    {
        log_error(LOG_ICONS, "itidy_render_iff_thumbnail: NULL parameter\n");
        return -1;
    }

    /*--------------------------------------------------------------------*/
    /* Step 1: Parse the IFF file                                         */
    /*--------------------------------------------------------------------*/

    parse_result = itidy_iff_parse(source_path, iff_params);
    if (parse_result != 0)
    {
        return parse_result;  // Error or NOT_SUPPORTED
    }

    /*--------------------------------------------------------------------*/
    /* Step 2: Calculate scaling to fit safe area with aspect ratio        */
    /*--------------------------------------------------------------------*/

    if (iff_params->base.safe_width == 0 || iff_params->base.safe_height == 0)
    {
        log_error(LOG_ICONS, "itidy_render_iff_thumbnail: zero safe area\n");
        itidy_iff_params_free(iff_params);
        return -1;
    }

    // Fixed-point 16.16: scale = source / dest
    x_scale_fp = ((ULONG)iff_params->display_width << 16) / iff_params->base.safe_width;
    y_scale_fp = ((ULONG)iff_params->display_height << 16) / iff_params->base.safe_height;

    // Use the larger scale factor to fit entirely within safe area
    scale_fp = (x_scale_fp > y_scale_fp) ? x_scale_fp : y_scale_fp;

    if (scale_fp == 0)
    {
        scale_fp = 1 << 16;  // 1:1 minimum
    }

    // Output dimensions at this scale
    output_width  = (UWORD)(((ULONG)iff_params->display_width << 16) / scale_fp);
    output_height = (UWORD)(((ULONG)iff_params->display_height << 16) / scale_fp);

    // Clamp to safe area
    if (output_width > iff_params->base.safe_width)
    {
        output_width = iff_params->base.safe_width;
    }
    if (output_height > iff_params->base.safe_height)
    {
        output_height = iff_params->base.safe_height;
    }

    // Ensure minimum 1x1
    if (output_width == 0) output_width = 1;
    if (output_height == 0) output_height = 1;

    // Center within safe area (letterbox/pillarbox)
    offset_x = iff_params->base.safe_left + (iff_params->base.safe_width - output_width) / 2;
    offset_y = iff_params->base.safe_top + (iff_params->base.safe_height - output_height) / 2;

    // Store output dimensions for caller (enables post-render cropping)
    iff_params->output_width = output_width;
    iff_params->output_height = output_height;
    iff_params->output_offset_x = offset_x;
    iff_params->output_offset_y = offset_y;

    log_info(LOG_ICONS, "itidy_render_iff_thumbnail: scaling %ux%u -> %ux%u "
             "(safe %ux%u, offset %u,%u)\n",
             (unsigned)iff_params->display_width,
             (unsigned)iff_params->display_height,
             (unsigned)output_width, (unsigned)output_height,
             (unsigned)iff_params->base.safe_width,
             (unsigned)iff_params->base.safe_height,
             (unsigned)offset_x, (unsigned)offset_y);

    /*--------------------------------------------------------------------*/
    /* Step 3: Fill safe area with background color                       */
    /*--------------------------------------------------------------------*/

    if (iff_params->base.bg_color_index != ITIDY_NO_BG_COLOR)
    {
        UWORD row;
        for (row = 0; row < iff_params->base.safe_height; row++)
        {
            UWORD y = iff_params->base.safe_top + row;
            if (y < img->height)
            {
                ULONG row_offset = (ULONG)y * (ULONG)img->width + iff_params->base.safe_left;
                UWORD fill_w = iff_params->base.safe_width;
                if (iff_params->base.safe_left + fill_w > img->width)
                {
                    fill_w = img->width - iff_params->base.safe_left;
                }
                memset(img->pixel_data_normal + row_offset,
                       iff_params->base.bg_color_index, fill_w);
            }
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 4: Replace icon palette with source CMAP (PICTURE mode)       */
    /*--------------------------------------------------------------------*/

    if (iff_params->palette_mode == ITIDY_PAL_SCREEN)
    {
        log_warning(LOG_ICONS, "itidy_render_iff_thumbnail: SCREEN palette mode "
                    "not yet implemented, falling back to PICTURE mode\n");
        iff_params->palette_mode = ITIDY_PAL_PICTURE;
    }

    if (iff_params->palette_mode == ITIDY_PAL_PICTURE)
    {
        // Replace the icon's palette with the source image's palette
        if (img->palette_normal != NULL)
        {
            whd_free(img->palette_normal);
        }

        img->palette_normal = (struct ColorRegister *)whd_malloc(
            iff_params->src_palette_size * sizeof(struct ColorRegister));

        if (img->palette_normal == NULL)
        {
            log_error(LOG_ICONS, "itidy_render_iff_thumbnail: palette replacement alloc failed\n");
            itidy_iff_params_free(iff_params);
            return -1;
        }

        memcpy(img->palette_normal, iff_params->src_palette,
               iff_params->src_palette_size * sizeof(struct ColorRegister));
        img->palette_size_normal = iff_params->src_palette_size;

        log_debug(LOG_ICONS, "itidy_render_iff_thumbnail: replaced icon palette "
                  "with source CMAP (%lu entries)\n", iff_params->src_palette_size);
    }

    /*--------------------------------------------------------------------*/
    /* Step 5: Scale source pixels into the safe area                     */
    /*         Uses 2x2 pre-filter for extreme reduction ratios to cap    */
    /*         per-pixel work on 68020 while maintaining quality.         */
    /*--------------------------------------------------------------------*/

    {
        ULONG ratio_x = (output_width > 0) ?
            (ULONG)iff_params->display_width / (ULONG)output_width : 1;
        ULONG ratio_y = (output_height > 0) ?
            (ULONG)iff_params->display_height / (ULONG)output_height : 1;
        ULONG ratio = (ratio_x > ratio_y) ? ratio_x : ratio_y;
        ULONG src_area = (ULONG)iff_params->src_width * (ULONG)iff_params->src_height;

        if (ratio > ITIDY_SCALE_PREFILTER_RATIO && src_area > ITIDY_SCALE_PREFILTER_MIN_AREA)
        {
            // Pre-filter: halve the source image first, then area-average
            UBYTE *half_buf = NULL;
            UWORD half_w = 0, half_h = 0;

            log_debug(LOG_ICONS, "itidy_render_iff_thumbnail: ratio=%lu area=%lu — using 2x2 pre-filter\n",
                      ratio, src_area);

            if (prefilter_2x2(iff_params->src_chunky,
                              iff_params->src_width, iff_params->src_height,
                              &half_buf, &half_w, &half_h,
                              iff_params->src_palette,
                              iff_params->src_palette_size))
            {
                area_average_scale(half_buf, half_w, half_h,
                                   img->pixel_data_normal,
                                   output_width, output_height,
                                   img->width,
                                   offset_x, offset_y,
                                   iff_params->src_palette,
                                   iff_params->src_palette_size,
                                   img->palette_normal,
                                   img->palette_size_normal,
                                   iff_params);
                whd_free(half_buf);
            }
            else
            {
                // Pre-filter failed — fall back to direct scale
                log_warning(LOG_ICONS, "itidy_render_iff_thumbnail: prefilter failed, using direct scale\n");
                area_average_scale(iff_params->src_chunky,
                                   iff_params->src_width, iff_params->src_height,
                                   img->pixel_data_normal,
                                   output_width, output_height,
                                   img->width,
                                   offset_x, offset_y,
                                   iff_params->src_palette,
                                   iff_params->src_palette_size,
                                   img->palette_normal,
                                   img->palette_size_normal,
                                   iff_params);
            }
        }
        else
        {
            // Direct area-average — small enough to handle directly
            area_average_scale(iff_params->src_chunky,
                               iff_params->src_width, iff_params->src_height,
                               img->pixel_data_normal,
                               output_width, output_height,
                               img->width,
                               offset_x, offset_y,
                               iff_params->src_palette,
                               iff_params->src_palette_size,
                               img->palette_normal,
                               img->palette_size_normal,
                               iff_params);
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 6: Cleanup parsed source data                                 */
    /*--------------------------------------------------------------------*/

    itidy_iff_params_free(iff_params);

    log_info(LOG_ICONS, "itidy_render_iff_thumbnail: complete for '%s'\n", source_path);
    return 0;
}

/*========================================================================*/
/* itidy_iff_params_free — Free IFF render params buffers                 */
/*========================================================================*/

void itidy_iff_params_free(iTidy_IFFRenderParams *params)
{
    if (params == NULL)
    {
        return;
    }

    if (params->src_chunky != NULL)
    {
        whd_free(params->src_chunky);
        params->src_chunky = NULL;
    }

    if (params->src_palette != NULL)
    {
        whd_free(params->src_palette);
        params->src_palette = NULL;
        params->src_palette_size = 0;
    }
}

/*========================================================================*/
/* itidy_render_via_datatype — Datatype-based fallback renderer          */
/*========================================================================*/

int itidy_render_via_datatype(const char *source_path,
                               iTidy_IFFRenderParams *params,
                               iTidy_IconImageData *dest_img)
{
    Object *dt_obj = NULL;
    struct Library *DataTypesBase = NULL;
    struct BitMapHeader *bmhd = NULL;
    UBYTE *rgb24_buffer = NULL;
    UBYTE *chunky_buffer = NULL;
    ULONG src_width = 0, src_height = 0;
    ULONG display_width, display_height;
    ULONG rgb24_size;
    int result = ITIDY_PREVIEW_NOT_SUPPORTED;
    ULONG modeid;
    BOOL is_hires = FALSE;
    BOOL is_lace = FALSE;

    log_info(LOG_ICONS, "itidy_render_via_datatype: attempting datatype fallback for '%s'\n",
             source_path);

    /* Open datatypes.library */
    DataTypesBase = OpenLibrary("datatypes.library", 39);
    if (DataTypesBase == NULL)
    {
        log_error(LOG_ICONS, "itidy_render_via_datatype: failed to open datatypes.library\n");
        return ITIDY_PREVIEW_NOT_SUPPORTED;
    }

    /* Open the image via datatypes */
    dt_obj = NewDTObject((APTR)source_path,
                         DTA_SourceType, DTST_FILE,
                         DTA_GroupID, GID_PICTURE,
                         PDTA_Remap, FALSE,  /* Don't remap - we want original colors */
                         TAG_DONE);

    if (dt_obj == NULL)
    {
        log_error(LOG_ICONS, "itidy_render_via_datatype: NewDTObject failed for '%s'\n",
                  source_path);
        CloseLibrary(DataTypesBase);
        return ITIDY_PREVIEW_NOT_SUPPORTED;
    }

    /* Get bitmap header for dimensions */
    if (GetDTAttrs(dt_obj, PDTA_BitMapHeader, &bmhd, TAG_DONE) != 1 || bmhd == NULL)
    {
        log_error(LOG_ICONS, "itidy_render_via_datatype: failed to get BitMapHeader\n");
        goto cleanup;
    }

    src_width = bmhd->bmh_Width;
    src_height = bmhd->bmh_Height;

    log_info(LOG_ICONS, "itidy_render_via_datatype: image %lux%lu depth=%u\n",
             src_width, src_height, (unsigned)bmhd->bmh_Depth);

    if (src_width == 0 || src_height == 0 || src_width > 4096 || src_height > 4096)
    {
        log_error(LOG_ICONS, "itidy_render_via_datatype: invalid dimensions\n");
        goto cleanup;
    }

    /* Get mode ID for aspect ratio correction */
    if (GetDTAttrs(dt_obj, PDTA_ModeID, &modeid, TAG_DONE) == 1)
    {
        is_hires = (modeid & 0x8000) ? TRUE : FALSE;
        is_lace = (modeid & 0x0004) ? TRUE : FALSE;
        log_info(LOG_ICONS, "itidy_render_via_datatype: ModeID=0x%08lX hires=%s lace=%s\n",
                 modeid, is_hires ? "yes" : "no", is_lace ? "yes" : "no");
    }

    /* Infer hires from width if ModeID didn't indicate it (like normal IFF code) */
    if (!is_hires && src_width >= ITIDY_HIRES_WIDTH_THRESHOLD)
    {
        is_hires = TRUE;
        log_info(LOG_ICONS, "itidy_render_via_datatype: inferred hires from width %lu "
                 "(ModeID lacked HIRES flag)\n", src_width);
    }

    /* Infer interlace from height if ModeID didn't indicate it */
    if (!is_lace && src_height >= 400)
    {
        is_lace = TRUE;
        log_info(LOG_ICONS, "itidy_render_via_datatype: inferred interlace from height %lu "
                 "(ModeID lacked LACE flag)\n", src_height);
    }

    /* Apply aspect ratio corrections */
    display_width = src_width;
    display_height = src_height;

    if (is_hires && src_width >= ITIDY_HIRES_WIDTH_THRESHOLD)
    {
        display_width = src_width / 2;
        log_debug(LOG_ICONS, "itidy_render_via_datatype: hires correction: %lu -> %lu width\n",
                  src_width, display_width);
    }

    if (is_lace)
    {
        display_height = src_height / 2;
        log_debug(LOG_ICONS, "itidy_render_via_datatype: interlace correction: %lu -> %lu height\n",
                  src_height, display_height);
    }

    /* Allocate RGB24 buffer (3 bytes per pixel) */
    rgb24_size = src_width * src_height * 3;
    rgb24_buffer = (UBYTE *)whd_malloc(rgb24_size);
    if (rgb24_buffer == NULL)
    {
        log_error(LOG_ICONS, "itidy_render_via_datatype: failed to allocate RGB24 buffer (%lu bytes)\n",
                  rgb24_size);
        goto cleanup;
    }

    /* Read pixel data as RGB24 via PDTM_READPIXELARRAY */
    if (DoMethod(dt_obj, PDTM_READPIXELARRAY,
                 rgb24_buffer,            /* PixelArray (APTR) */
                 PBPAFMT_RGB,             /* PixelFormat (24-bit RGB) */
                 src_width * 3,           /* PixelArrayMod (bytes per row) */
                 0, 0,                    /* Left, Top */
                 src_width, src_height)   /* Width, Height */
        == 0)
    {
        log_error(LOG_ICONS, "itidy_render_via_datatype: PDTM_READPIXELARRAY failed\n");
        goto cleanup;
    }

    log_info(LOG_ICONS, "itidy_render_via_datatype: read %lux%lu RGB24 pixels\n",
             src_width, src_height);

    /* Build a better palette from the RGB data (6x6x6 color cube + grays) */
    {
        ULONG pal_idx = 0;
        ULONG r_step, g_step, b_step;
        
        /* Generate 6x6x6 RGB color cube (216 colors) */
        for (r_step = 0; r_step < 6; r_step++)
        {
            for (g_step = 0; g_step < 6; g_step++)
            {
                for (b_step = 0; b_step < 6; b_step++)
                {
                    if (pal_idx < dest_img->palette_size_normal)
                    {
                        dest_img->palette_normal[pal_idx].red   = (r_step * 255) / 5;
                        dest_img->palette_normal[pal_idx].green = (g_step * 255) / 5;
                        dest_img->palette_normal[pal_idx].blue  = (b_step * 255) / 5;
                        pal_idx++;
                    }
                }
            }
        }
        
        /* Fill remaining with grayscale ramp */
        while (pal_idx < dest_img->palette_size_normal)
        {
            UBYTE gray = (UBYTE)((pal_idx - 216) * 255 / (dest_img->palette_size_normal - 216));
            dest_img->palette_normal[pal_idx].red   = gray;
            dest_img->palette_normal[pal_idx].green = gray;
            dest_img->palette_normal[pal_idx].blue  = gray;
            pal_idx++;
        }
        
        log_info(LOG_ICONS, "itidy_render_via_datatype: built %u-color palette (6x6x6 cube + grays)\n",
                 (unsigned)dest_img->palette_size_normal);
    }

    /* Allocate chunky buffer for quantized pixels */
    chunky_buffer = (UBYTE *)whd_malloc(src_width * src_height);
    if (chunky_buffer == NULL)
    {
        log_error(LOG_ICONS, "itidy_render_via_datatype: failed to allocate chunky buffer\n");
        goto cleanup;
    }

    /* Quantize RGB24 to new palette */
    {
        ULONG i;
        UBYTE *rgb_ptr = rgb24_buffer;
        UBYTE *chunky_ptr = chunky_buffer;
        ULONG total_pixels = src_width * src_height;

        for (i = 0; i < total_pixels; i++)
        {
            UBYTE r = *rgb_ptr++;
            UBYTE g = *rgb_ptr++;
            UBYTE b = *rgb_ptr++;

            /* Find closest color in destination palette */
            *chunky_ptr++ = itidy_find_closest_palette_color(
                dest_img->palette_normal,
                dest_img->palette_size_normal,
                r, g, b);

            /* Report progress every 5000 pixels (throttled to 0.5 seconds) */
            /* This is critical for HAM images which take ~24 minutes on 68000 */
            if (i % 5000 == 0 || i == total_pixels - 1)
            {
                if (!itidy_report_progress_throttled(params, "Quantizing colors",
                                                     i + 1, total_pixels, TICKS_PER_SECOND / 2))
                {
                    log_info(LOG_ICONS, "itidy_render_via_datatype: cancelled by user at pixel %lu\n", i);
                    result = -1;  /* Cancelled */
                    goto cleanup;
                }
            }
        }
    }

    log_info(LOG_ICONS, "itidy_render_via_datatype: quantized to %u-color palette\n",
             (unsigned)dest_img->palette_size_normal);

    /* Calculate destination size based on display aspect ratio (like normal IFF code) */
    {
        ULONG x_scale_fp, y_scale_fp, scale_fp;
        UWORD output_width, output_height;
        UWORD offset_x, offset_y;

        /* Fixed-point 16.16: scale = source / dest */
        x_scale_fp = ((ULONG)display_width << 16) / params->base.safe_width;
        y_scale_fp = ((ULONG)display_height << 16) / params->base.safe_height;

        /* Use larger scale to fit entirely within safe area */
        scale_fp = (x_scale_fp > y_scale_fp) ? x_scale_fp : y_scale_fp;
        if (scale_fp == 0)
        {
            scale_fp = 1 << 16;  /* 1:1 minimum */
        }

        /* Output dimensions at this scale */
        output_width  = (UWORD)(((ULONG)display_width << 16) / scale_fp);
        output_height = (UWORD)(((ULONG)display_height << 16) / scale_fp);

        /* Clamp to safe area */
        if (output_width > params->base.safe_width)
            output_width = params->base.safe_width;
        if (output_height > params->base.safe_height)
            output_height = params->base.safe_height;

        /* Ensure minimum 1x1 */
        if (output_width == 0) output_width = 1;
        if (output_height == 0) output_height = 1;

        /* Center within safe area */
        offset_x = params->base.safe_left + (params->base.safe_width - output_width) / 2;
        offset_y = params->base.safe_top + (params->base.safe_height - output_height) / 2;

        log_info(LOG_ICONS, "itidy_render_via_datatype: scaling %lux%lu -> %ux%u "
                 "(display %lux%lu, safe %ux%u, offset %u,%u)\n",
                 src_width, src_height, (unsigned)output_width, (unsigned)output_height,
                 display_width, display_height,
                 (unsigned)params->base.safe_width, (unsigned)params->base.safe_height,
                 (unsigned)offset_x, (unsigned)offset_y);

        area_average_scale(chunky_buffer,
                           (UWORD)src_width, (UWORD)src_height,
                           dest_img->pixel_data_normal, output_width, output_height,
                           dest_img->width,  /* stride */
                           offset_x, offset_y,
                           dest_img->palette_normal, dest_img->palette_size_normal,
                           dest_img->palette_normal, dest_img->palette_size_normal,
                           params);

        /* Store output dimensions for cropping */
        params->output_width = output_width;
        params->output_height = output_height;
        params->output_offset_x = offset_x;
        params->output_offset_y = offset_y;
    }

    log_info(LOG_ICONS, "itidy_render_via_datatype: complete for '%s'\n", source_path);
    result = 0;

cleanup:
    if (chunky_buffer) whd_free(chunky_buffer);
    if (rgb24_buffer) whd_free(rgb24_buffer);
    if (dt_obj) DisposeDTObject(dt_obj);
    if (DataTypesBase) CloseLibrary(DataTypesBase);
    return result;
}
