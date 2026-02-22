/*
 * icon_iff_decode.c - IFF ILBM chunk validation and bitplane decoder
 *
 * Handles: BMHD/CMAP/CAMG chunk validation, EHB palette expansion,
 * ByteRun1 decompression, bitplane-to-chunky conversion, and the top-level
 * itidy_iff_parse() entry point.
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

#include <console_output.h>

#include "icon_iff_render.h"
#include "../icon_image_access.h"
#include "../../writeLog.h"

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
static UWORD calculate_row_bytes(UWORD width);
static int  decompress_byterun1(const UBYTE *src, ULONG src_len,
                                UBYTE *dest, ULONG dest_len,
                                ULONG *consumed_out);
static void planar_to_chunky(const UBYTE *planar_row, UWORD width,
                             UWORD num_planes, UBYTE *chunky_row);

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
    params->camg_flags   = camg_raw;
    params->is_interlace = (camg_raw & ITIDY_CAMG_LACE) ? TRUE : FALSE;
    params->is_hires     = (camg_raw & ITIDY_CAMG_HIRES) ? TRUE : FALSE;
    params->is_ham       = (camg_raw & ITIDY_CAMG_HAM)   ? TRUE : FALSE;
    params->is_ehb       = (camg_raw & ITIDY_CAMG_EHB)   ? TRUE : FALSE;

    log_debug(LOG_ICONS, "decode_camg_flags: raw=0x%08lX lace=%s hires=%s ham=%s ehb=%s\n",
             camg_raw,
             params->is_interlace ? "yes" : "no",
             params->is_hires     ? "yes" : "no",
             params->is_ham       ? "yes" : "no",
             params->is_ehb       ? "yes" : "no");
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
 * @param src           Compressed source data
 * @param src_len       Length of compressed data in bytes
 * @param dest          Destination buffer for decompressed data
 * @param dest_len      Size of destination buffer
 * @param consumed_out  Output: number of source bytes consumed (may be NULL)
 * @return Number of bytes written to dest, or -1 on error
 */
static int decompress_byterun1(const UBYTE *src, ULONG src_len,
                               UBYTE *dest, ULONG dest_len,
                               ULONG *consumed_out)
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

            if (src_pos + count > src_len || dest_pos + count > dest_len)
            {
                log_warning(LOG_ICONS,
                            "decompress_byterun1: literal overrun at src=%lu dest=%lu count=%lu\n",
                            src_pos, dest_pos, count);
                break;
            }

            memcpy(dest + dest_pos, src + src_pos, count);
            src_pos  += count;
            dest_pos += count;
        }
        else if (n != -128)
        {
            // Repeat run: replicate next byte (-n+1) times
            ULONG count = (ULONG)(-(int)n) + 1;
            UBYTE value;

            if (src_pos >= src_len || dest_pos + count > dest_len)
            {
                log_warning(LOG_ICONS,
                            "decompress_byterun1: repeat overrun at src=%lu dest=%lu count=%lu\n",
                            src_pos, dest_pos, count);
                break;
            }

            value = src[src_pos++];
            memset(dest + dest_pos, value, count);
            dest_pos += count;
        }
        // n == -128: no-op, continue
    }

    if (consumed_out != NULL)
    {
        *consumed_out = src_pos;
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
 * Optimized: processes 8 pixels per iteration by reading one byte from
 * each bitplane and extracting all 8 pixel values with unrolled bit
 * tests.  This reduces outer-loop iterations 8x and eliminates per-pixel
 * byte_idx / bit_mask calculations.  Pre-computed plane offsets avoid
 * repeated multiply-by-row_bytes in the inner loop (saves ~70 cycles
 * per MULS on 68000).
 *
 * @param planar_row    Pointer to interleaved plane data for one row
 * @param width         Image width in pixels
 * @param num_planes    Number of bitplanes (1-8)
 * @param chunky_row    Output: one byte per pixel (palette indices)
 */
static void planar_to_chunky(const UBYTE *planar_row, UWORD width,
                             UWORD num_planes, UBYTE *chunky_row)
{
    UWORD row_bytes  = calculate_row_bytes(width);
    UWORD full_bytes = width >> 3;   // Complete groups of 8 pixels
    UWORD remainder  = width & 7;    // Leftover pixels (0-7)
    UWORD x_byte;
    UWORD plane;

    // Pre-compute plane offsets and bit values (avoids multiply in inner loop)
    UWORD plane_offsets[8];
    UBYTE plane_bits[8];

    for (plane = 0; plane < num_planes && plane < 8; plane++)
    {
        plane_offsets[plane] = plane * row_bytes;
        plane_bits[plane]    = (UBYTE)(1 << plane);
    }

    // Process 8 pixels at a time — reads one byte per plane, extracts all 8
    for (x_byte = 0; x_byte < full_bytes; x_byte++)
    {
        UBYTE *out = chunky_row + (UWORD)(x_byte << 3);

        // Clear 8 output pixels
        out[0] = 0; out[1] = 0; out[2] = 0; out[3] = 0;
        out[4] = 0; out[5] = 0; out[6] = 0; out[7] = 0;

        // Merge each plane's contribution into the 8 output pixels
        for (plane = 0; plane < num_planes; plane++)
        {
            UBYTE b  = planar_row[plane_offsets[plane] + x_byte];
            UBYTE pb = plane_bits[plane];

            if (b & 0x80) out[0] |= pb;
            if (b & 0x40) out[1] |= pb;
            if (b & 0x20) out[2] |= pb;
            if (b & 0x10) out[3] |= pb;
            if (b & 0x08) out[4] |= pb;
            if (b & 0x04) out[5] |= pb;
            if (b & 0x02) out[6] |= pb;
            if (b & 0x01) out[7] |= pb;
        }
    }

    // Handle remaining pixels (< 8) at end of row
    if (remainder > 0)
    {
        UWORD base_x = (UWORD)(full_bytes << 3);
        UWORD x;

        for (x = 0; x < remainder; x++)
        {
            UBYTE bit_mask = (UBYTE)(0x80 >> x);
            UBYTE pixel    = 0;

            for (plane = 0; plane < num_planes; plane++)
            {
                if (planar_row[plane_offsets[plane] + full_bytes] & bit_mask)
                {
                    pixel |= plane_bits[plane];
                }
            }

            chunky_row[base_x + x] = pixel;
        }
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
    UBYTE *body_buf   = NULL;
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

    log_debug(LOG_ICONS, "itidy_iff_parse: BMHD %ux%u depth=%u comp=%u mask=%u '%s'\n",
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
        log_debug(LOG_ICONS,
                 "itidy_iff_parse: CAMG bytes=%02X %02X %02X %02X (raw=0x%08lX) '%s'\n",
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
        log_debug(LOG_ICONS,
                 "itidy_iff_parse: inferred hires from width %u "
                 "(CAMG lacked HIRES flag) '%s'\n",
                 (unsigned)bmhd.w, source_path);
    }

    // Check for HAM — not supported
    if (iff_params->is_ham)
    {
        log_info(LOG_ICONS, "itidy_iff_parse: HAM image detected -- "
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
            iff_params->src_palette      = ehb_palette;
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

        body_row_size    = (ULONG)row_bytes * (ULONG)planes_per_row;
        total_planar_size = body_row_size * (ULONG)bmhd.h;
    }

    // Suppress unused-variable warning for total_planar_size (kept for documentation)
    (void)total_planar_size;

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
                log_warning(LOG_ICONS,
                            "itidy_iff_parse: short BODY read at row %u "
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
                    log_info(LOG_ICONS,
                             "itidy_iff_parse: cancelled by user at row %u\n",
                             (unsigned)y);
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
            log_error(LOG_ICONS,
                      "itidy_iff_parse: BODY buffer alloc failed (%lu bytes)\n",
                      body_size);
            goto cleanup;
        }

        body_read = (ULONG)ReadChunkBytes(iff, body_buf, body_size);
        if (body_read == 0)
        {
            log_error(LOG_ICONS, "itidy_iff_parse: BODY read returned 0 bytes\n");
            goto cleanup;
        }

        log_debug(LOG_ICONS, "itidy_iff_parse: BODY %lu bytes read (ByteRun1)\n",
                  body_read);

        // Allocate buffer for one decompressed row (all planes)
        planar_buf = (UBYTE *)whd_malloc(body_row_size);
        if (planar_buf == NULL)
        {
            log_error(LOG_ICONS,
                      "itidy_iff_parse: planar decompressed buffer alloc failed\n");
            goto cleanup;
        }

        // Decompress row by row
        {
            ULONG src_pos = 0;

            for (y = 0; y < bmhd.h; y++)
            {
                int   decomp_bytes;
                ULONG consumed = 0;

                if (src_pos >= body_read)
                {
                    log_warning(LOG_ICONS,
                                "itidy_iff_parse: compressed data exhausted at row %u\n",
                                (unsigned)y);
                    memset(chunky_buf + (ULONG)y * (ULONG)bmhd.w, 0,
                           (ULONG)(bmhd.h - y) * (ULONG)bmhd.w);
                    break;
                }

                decomp_bytes = decompress_byterun1(
                    body_buf + src_pos, body_read - src_pos,
                    planar_buf, body_row_size, &consumed);

                if (decomp_bytes <= 0)
                {
                    log_warning(LOG_ICONS,
                                "itidy_iff_parse: ByteRun1 failed at row %u\n",
                                (unsigned)y);
                    memset(chunky_buf + (ULONG)y * (ULONG)bmhd.w, 0,
                           (ULONG)(bmhd.h - y) * (ULONG)bmhd.w);
                    break;
                }

                src_pos += consumed;

                planar_to_chunky(planar_buf, bmhd.w, bmhd.nPlanes,
                                 chunky_buf + (ULONG)y * (ULONG)bmhd.w);

                /* Report progress every 20 rows (throttled to ~1 second) */
                if (y % 20 == 0 || y == bmhd.h - 1)
                {
                    if (!itidy_report_progress_throttled(iff_params, "Decoding pixels",
                                                         y + 1, bmhd.h, TICKS_PER_SECOND))
                    {
                        log_info(LOG_ICONS,
                                 "itidy_iff_parse: cancelled by user at row %u\n",
                                 (unsigned)y);
                        result = -1;  /* Cancelled */
                        goto cleanup;
                    }
                }
            }
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 9: Calculate display dimensions (aspect ratio correction)     */
    /*                                                                    */
    /* Amiga display modes have non-square pixels:                        */
    /*   Lowres (320px wide): ~1:1 pixel aspect on a standard monitor     */
    /*   Hires (640px wide):  pixels are half as wide -> halve width      */
    /*   Interlace (2x lines): pixels are half as tall -> halve height    */
    /*   Hires+Lace (e.g. 672x446): halve BOTH width and height          */
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

    log_debug(LOG_ICONS,
             "itidy_iff_parse: decoded %ux%u (%u planes) -> "
             "display %ux%u, palette %lu colors '%s'\n",
             (unsigned)bmhd.w, (unsigned)bmhd.h, (unsigned)bmhd.nPlanes,
             (unsigned)iff_params->display_width,
             (unsigned)iff_params->display_height,
             iff_params->src_palette_size, source_path);

cleanup:
    // Free working buffers
    if (planar_buf != NULL) whd_free(planar_buf);
    if (body_buf   != NULL) whd_free(body_buf);
    if (chunky_buf != NULL) whd_free(chunky_buf);  // Only if we didn't transfer ownership

    // Close IFF (CRITICAL ORDER: CloseIFF -> Close file -> FreeIFF -> CloseLibrary)
    CloseIFF(iff);
    if (file != 0) Close(file);
    FreeIFF(iff);
    CloseLibrary(IFFParseBase);

    // On error, free any allocated palette
    if (result != 0 && iff_params->src_palette != NULL)
    {
        whd_free(iff_params->src_palette);
        iff_params->src_palette      = NULL;
        iff_params->src_palette_size = 0;
    }

    return result;
}
