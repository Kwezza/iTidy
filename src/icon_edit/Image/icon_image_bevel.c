/*
 * icon_image_bevel.c - Thumbnail bevel highlight post-processor
 *
 * Applies a 2-pixel graduated bevel to the thumbnail safe-area boundary,
 * simulating illumination from the top-left corner:
 *
 *   Outer ring (1 px from edge):  50% blended toward white (top/left)
 *                                  50% blended toward black (bottom/right)
 *   Inner ring (2 px from edge):  25% blended toward white (top/left)
 *                                  25% blended toward black (bottom/right)
 *   Ambiguous corners (top-right, bottom-left): no bevel applied
 *
 * Because Amiga icons use indexed colour with no real alpha channel, the
 * semi-transparent look is created by:
 *   1. Expanding palette_normal with pre-blended colour variants (same
 *      approach as expand_palette_for_adaptive in icon_image_access.c).
 *   2. Building four 256-entry index-remap lookup tables (lighten-outer,
 *      lighten-inner, darken-outer, darken-inner).
 *   3. Walking the 2-pixel border band and remapping each pixel's palette
 *      index through the appropriate table.
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <string.h>

#include <datatypes/pictureclass.h>  /* struct ColorRegister */

#include <console_output.h>

#include "../icon_image_access.h"
#include "icon_image_bevel.h"
#include "../../writeLog.h"

/*========================================================================*/
/* Constants                                                              */
/*========================================================================*/

/* Hard limit on palette size (Amiga OS 3.5 icon maximum) */
#define BEVEL_MAX_PALETTE    256U

/* Euclidean RGB² tolerance used for palette deduplication (~20 per channel) */
#define BEVEL_TOLERANCE_SQ   (20 * 20)

/*
 * Minimum number of free palette slots required before attempting expansion.
 * With 4 blend phases and deduplication this is a conservative lower bound;
 * we just need room for at least one new variant per phase.
 */
#define BEVEL_MIN_FREE_SLOTS  BEVEL_PALETTE_RESERVED

/* Blend strengths (percent toward white / toward black) */
#define BEVEL_LIGHTEN_OUTER  50
#define BEVEL_LIGHTEN_INNER  25
#define BEVEL_DARKEN_OUTER   50
#define BEVEL_DARKEN_INNER   25

/*========================================================================*/
/* Internal RGB helpers                                                   */
/*========================================================================*/

/**
 * @brief Blend an RGB colour toward white by 'percent'% (0=unchanged, 100=white).
 *
 * Formula: out = in + (255 - in) * percent / 100
 */
static void bevel_lighten_rgb(UBYTE r, UBYTE g, UBYTE b, UBYTE percent,
                              UBYTE *out_r, UBYTE *out_g, UBYTE *out_b)
{
    UWORD p = (percent > 100) ? 100 : (UWORD)percent;
    *out_r = (UBYTE)((ULONG)r + ((ULONG)(255 - r) * (ULONG)p) / 100UL);
    *out_g = (UBYTE)((ULONG)g + ((ULONG)(255 - g) * (ULONG)p) / 100UL);
    *out_b = (UBYTE)((ULONG)b + ((ULONG)(255 - b) * (ULONG)p) / 100UL);
}

/**
 * @brief Blend an RGB colour toward black by 'percent'% (0=unchanged, 100=black).
 *
 * Formula: out = in * (100 - percent) / 100
 */
static void bevel_darken_rgb(UBYTE r, UBYTE g, UBYTE b, UBYTE percent,
                             UBYTE *out_r, UBYTE *out_g, UBYTE *out_b)
{
    UWORD keep = (percent >= 100) ? 0 : (UWORD)(100 - percent);
    *out_r = (UBYTE)((ULONG)r * (ULONG)keep / 100UL);
    *out_g = (UBYTE)((ULONG)g * (ULONG)keep / 100UL);
    *out_b = (UBYTE)((ULONG)b * (ULONG)keep / 100UL);
}

/**
 * @brief Search palette for an entry within Euclidean tolerance.
 *
 * @return Palette index on match, -1 if no entry is within tolerance.
 */
static LONG bevel_find_in_tolerance(const struct ColorRegister *pal,
                                    ULONG pal_size,
                                    UBYTE tr, UBYTE tg, UBYTE tb,
                                    ULONG tol_sq)
{
    ULONG i;
    for (i = 0; i < pal_size; i++)
    {
        LONG dr = (LONG)pal[i].red   - (LONG)tr;
        LONG dg = (LONG)pal[i].green - (LONG)tg;
        LONG db = (LONG)pal[i].blue  - (LONG)tb;
        ULONG dist = (ULONG)(dr * dr + dg * dg + db * db);
        if (dist <= tol_sq)
            return (LONG)i;
    }
    return -1;
}

/*========================================================================*/
/* Palette expansion                                                      */
/*========================================================================*/

/**
 * @brief Expand palette with four blend-variant phases for bevel rendering.
 *
 * Four new variant sets are appended (each deduplicated, capped at 256 total):
 *   LO: 50% lighten  (outer highlight)
 *   LI: 25% lighten  (inner highlight)
 *   DO: 50% darken   (outer shadow)
 *   DI: 25% darken   (inner shadow)
 *
 * When all variants already exist within tolerance, the function returns TRUE
 * without modifying the palette (the remap tables will still find good matches).
 *
 * @return TRUE on success, FALSE if palette is too full or allocation failed.
 */
static BOOL bevel_expand_palette(struct ColorRegister **pal_ptr,
                                 ULONG               *pal_size_ptr)
{
    struct ColorRegister *old_pal;
    struct ColorRegister *new_pal;
    ULONG old_size;
    ULONG cur;
    ULONG i;
    ULONG added = 0;
    UBYTE nr, ng, nb;
    LONG  match;

    if (pal_ptr == NULL || *pal_ptr == NULL || pal_size_ptr == NULL)
        return FALSE;

    old_pal  = *pal_ptr;
    old_size = *pal_size_ptr;

    if (old_size == 0)
        return FALSE;

    /* Bail out early if there is no room */
    if (old_size >= BEVEL_MAX_PALETTE ||
        (BEVEL_MAX_PALETTE - old_size) < BEVEL_MIN_FREE_SLOTS)
    {
        log_debug(LOG_ICONS,
                  "bevel_expand_palette: palette too full (%lu entries) -- bevel skipped\n",
                  old_size);
        return FALSE;
    }

    new_pal = (struct ColorRegister *)whd_malloc(
        BEVEL_MAX_PALETTE * sizeof(struct ColorRegister));
    if (new_pal == NULL)
    {
        log_error(LOG_ICONS, "bevel_expand_palette: alloc failed\n");
        return FALSE;
    }

    memcpy(new_pal, old_pal, old_size * sizeof(struct ColorRegister));
    cur = old_size;

    /*--------------------------------------------------------------------*/
    /* Phase LO: 50% lighten (outer highlight pixels)                     */
    /*--------------------------------------------------------------------*/
    for (i = 0; i < old_size && cur < BEVEL_MAX_PALETTE; i++)
    {
        bevel_lighten_rgb(old_pal[i].red, old_pal[i].green, old_pal[i].blue,
                          BEVEL_LIGHTEN_OUTER, &nr, &ng, &nb);
        match = bevel_find_in_tolerance(new_pal, cur, nr, ng, nb, BEVEL_TOLERANCE_SQ);
        if (match < 0)
        {
            new_pal[cur].red   = nr;
            new_pal[cur].green = ng;
            new_pal[cur].blue  = nb;
            cur++;
            added++;
        }
    }

    /*--------------------------------------------------------------------*/
    /* Phase LI: 25% lighten (inner highlight pixels)                     */
    /*--------------------------------------------------------------------*/
    for (i = 0; i < old_size && cur < BEVEL_MAX_PALETTE; i++)
    {
        bevel_lighten_rgb(old_pal[i].red, old_pal[i].green, old_pal[i].blue,
                          BEVEL_LIGHTEN_INNER, &nr, &ng, &nb);
        match = bevel_find_in_tolerance(new_pal, cur, nr, ng, nb, BEVEL_TOLERANCE_SQ);
        if (match < 0)
        {
            new_pal[cur].red   = nr;
            new_pal[cur].green = ng;
            new_pal[cur].blue  = nb;
            cur++;
            added++;
        }
    }

    /*--------------------------------------------------------------------*/
    /* Phase DO: 50% darken (outer shadow pixels)                         */
    /*--------------------------------------------------------------------*/
    for (i = 0; i < old_size && cur < BEVEL_MAX_PALETTE; i++)
    {
        bevel_darken_rgb(old_pal[i].red, old_pal[i].green, old_pal[i].blue,
                         BEVEL_DARKEN_OUTER, &nr, &ng, &nb);
        match = bevel_find_in_tolerance(new_pal, cur, nr, ng, nb, BEVEL_TOLERANCE_SQ);
        if (match < 0)
        {
            new_pal[cur].red   = nr;
            new_pal[cur].green = ng;
            new_pal[cur].blue  = nb;
            cur++;
            added++;
        }
    }

    /*--------------------------------------------------------------------*/
    /* Phase DI: 25% darken (inner shadow pixels)                         */
    /*--------------------------------------------------------------------*/
    for (i = 0; i < old_size && cur < BEVEL_MAX_PALETTE; i++)
    {
        bevel_darken_rgb(old_pal[i].red, old_pal[i].green, old_pal[i].blue,
                         BEVEL_DARKEN_INNER, &nr, &ng, &nb);
        match = bevel_find_in_tolerance(new_pal, cur, nr, ng, nb, BEVEL_TOLERANCE_SQ);
        if (match < 0)
        {
            new_pal[cur].red   = nr;
            new_pal[cur].green = ng;
            new_pal[cur].blue  = nb;
            cur++;
            added++;
        }
    }

    if (added == 0)
    {
        /*
         * Every variant was already in the palette within tolerance — free
         * our temporary copy and keep the original.  The lookup tables will
         * still find adequate matches via nearest-colour search.
         */
        whd_free(new_pal);
        log_debug(LOG_ICONS,
                  "bevel_expand_palette: all variants already in palette "
                  "(%lu entries unchanged)\n", old_size);
        return TRUE;
    }

    /* Replace palette with the expanded version */
    whd_free(old_pal);
    *pal_ptr      = new_pal;
    *pal_size_ptr = cur;

    log_debug(LOG_ICONS,
              "bevel_expand_palette: %lu -> %lu entries (+%lu bevel variants)\n",
              old_size, cur, added);
    return TRUE;
}

/*========================================================================*/
/* Lookup table builders                                                  */
/*========================================================================*/

/**
 * @brief Build an index-remap table that lighens each palette colour.
 *
 * out_table[src_idx] = index of the palette entry closest to
 *                      (src_colour blended 'percent'% toward white).
 *
 * Uses itidy_find_closest_palette_color() (public, in icon_image_access.h).
 */
static void bevel_build_lighten_table(const struct ColorRegister *pal,
                                      ULONG pal_size,
                                      UBYTE percent,
                                      UBYTE *out_table)
{
    ULONG i;
    UBYTE nr, ng, nb;

    for (i = 0; i < pal_size; i++)
    {
        bevel_lighten_rgb(pal[i].red, pal[i].green, pal[i].blue,
                          percent, &nr, &ng, &nb);
        out_table[i] = itidy_find_closest_palette_color(pal, pal_size, nr, ng, nb);
    }
    for (i = pal_size; i < 256; i++)
        out_table[i] = 0;
}

/**
 * @brief Build an index-remap table that darkens each palette colour.
 *
 * out_table[src_idx] = index of the palette entry closest to
 *                      (src_colour blended 'percent'% toward black).
 */
static void bevel_build_darken_table(const struct ColorRegister *pal,
                                     ULONG pal_size,
                                     UBYTE percent,
                                     UBYTE *out_table)
{
    ULONG i;
    UBYTE nr, ng, nb;

    for (i = 0; i < pal_size; i++)
    {
        bevel_darken_rgb(pal[i].red, pal[i].green, pal[i].blue,
                         percent, &nr, &ng, &nb);
        out_table[i] = itidy_find_closest_palette_color(pal, pal_size, nr, ng, nb);
    }
    for (i = pal_size; i < 256; i++)
        out_table[i] = 0;
}

/*========================================================================*/
/* Public API                                                             */
/*========================================================================*/

void itidy_apply_thumbnail_bevel(iTidy_IconImageData      *img,
                                 const iTidy_RenderParams *rp)
{
    UBYTE lighten_outer[256];   /* 50% toward white */
    UBYTE lighten_inner[256];   /* 25% toward white */
    UBYTE darken_outer[256];    /* 50% toward black */
    UBYTE darken_inner[256];    /* 25% toward black */

    UWORD sl, st, sw, sh;       /* safe left/top/width/height */
    UWORD right1, right2;       /* outermost and inner right column indices */
    UWORD bot1, bot2;           /* outermost and inner bottom row indices    */
    ULONG stride;
    UBYTE *buf;
    ULONG row_off;
    UWORD x, y, y_start, y_end;

    if (img == NULL || rp == NULL)
        return;
    if (img->pixel_data_normal == NULL || img->palette_normal == NULL)
        return;

    sl = rp->safe_left;
    st = rp->safe_top;
    sw = rp->safe_width;
    sh = rp->safe_height;

    /*
     * Need at least 5 pixels in each dimension:
     *   2 highlight + 1 content pixel + 2 shadow = 5
     */
    if (sw < 5 || sh < 5)
    {
        log_debug(LOG_ICONS,
                  "itidy_apply_thumbnail_bevel: safe area too small (%ux%u) -- skipped\n",
                  (unsigned)sw, (unsigned)sh);
        return;
    }

    /* Clamp safe area to canvas bounds */
    if ((ULONG)sl + (ULONG)sw > (ULONG)img->width)
        return;
    if ((ULONG)st + (ULONG)sh > (ULONG)img->height)
        return;

    /*--------------------------------------------------------------------*/
    /* Expand palette with blended variants                               */
    /*--------------------------------------------------------------------*/
    if (!bevel_expand_palette(&img->palette_normal, &img->palette_size_normal))
        return;  /* Too full or alloc failure -- silently skip */

    /*--------------------------------------------------------------------*/
    /* Build four remap tables against the now-expanded palette           */
    /*--------------------------------------------------------------------*/
    bevel_build_lighten_table(img->palette_normal, img->palette_size_normal,
                              BEVEL_LIGHTEN_OUTER, lighten_outer);
    bevel_build_lighten_table(img->palette_normal, img->palette_size_normal,
                              BEVEL_LIGHTEN_INNER, lighten_inner);
    bevel_build_darken_table (img->palette_normal, img->palette_size_normal,
                              BEVEL_DARKEN_OUTER,  darken_outer);
    bevel_build_darken_table (img->palette_normal, img->palette_size_normal,
                              BEVEL_DARKEN_INNER,  darken_inner);

    stride = (ULONG)img->width;
    buf    = img->pixel_data_normal;

    /* Precompute the four border line indices */
    right1 = sl + sw - 1;   /* outermost right column */
    right2 = sl + sw - 2;   /* inner right column      */
    bot1   = st + sh - 1;   /* outermost bottom row    */
    bot2   = st + sh - 2;   /* inner bottom row        */

    /*
     * The bevel is drawn as two nested rectangles.  Each of the 8 segments
     * below processes its pixels exactly once — no pixel is written twice.
     *
     * Outer ring (50% blend):
     *   top row    : full width  sl .. sl+sw-1
     *   bottom row : full width  sl .. sl+sw-1
     *   left col   : inner rows  st+1 .. st+sh-2  (corners owned by top/bottom rows)
     *   right col  : inner rows  st+1 .. st+sh-2
     *
     * Inner ring (25% blend), inset one pixel from the outer ring:
     *   top row    : sl+1 .. sl+sw-2
     *   bottom row : sl+1 .. sl+sw-2
     *   left col   : st+2 .. st+sh-3
     *   right col  : st+2 .. st+sh-3
     *
     * Corner ownership:
     *   (sl,      st     ) → outer top row → highlight
     *   (sl+sw-1, st     ) → outer top row → highlight  (top-right: top wins)
     *   (sl,      st+sh-1) → outer bottom row → shadow  (bottom-left: bottom wins)
     *   (sl+sw-1, st+sh-1) → outer bottom row → shadow
     *
     * This eliminates the "notch" gap that occurred when the previous code
     * left the top-right and bottom-left 2×2 corner zones completely blank.
     */

    /*====================================================================*/
    /* Segment 1: Outer top row — 50% lighten, full width                 */
    /*====================================================================*/
    row_off = (ULONG)st * stride;
    for (x = sl; x <= right1; x++)
        buf[row_off + x] = lighten_outer[buf[row_off + x]];

    /*====================================================================*/
    /* Segment 2: Inner top row — 25% lighten, inset 1 px each side      */
    /*====================================================================*/
    row_off = (ULONG)(st + 1) * stride;
    for (x = (UWORD)(sl + 1); x <= right2; x++)
        buf[row_off + x] = lighten_inner[buf[row_off + x]];

    /*====================================================================*/
    /* Segment 3: Outer bottom row — 50% darken, full width               */
    /*====================================================================*/
    row_off = (ULONG)bot1 * stride;
    for (x = sl; x <= right1; x++)
        buf[row_off + x] = darken_outer[buf[row_off + x]];

    /*====================================================================*/
    /* Segment 4: Inner bottom row — 25% darken, inset 1 px each side    */
    /*====================================================================*/
    row_off = (ULONG)bot2 * stride;
    for (x = (UWORD)(sl + 1); x <= right2; x++)
        buf[row_off + x] = darken_inner[buf[row_off + x]];

    /*====================================================================*/
    /* Segment 5: Outer left column — 50% lighten, rows st+1 .. bot1-1   */
    /*====================================================================*/
    y_start = (UWORD)(st + 1);
    y_end   = (UWORD)(st + sh - 2);   /* = bot1 - 1 */
    for (y = y_start; y <= y_end; y++)
        buf[(ULONG)y * stride + sl] = lighten_outer[buf[(ULONG)y * stride + sl]];

    /*====================================================================*/
    /* Segment 6: Inner left column — 25% lighten, rows st+2 .. bot2-1   */
    /*====================================================================*/
    y_start = (UWORD)(st + 2);
    y_end   = (UWORD)(st + sh - 3);   /* = bot2 - 1 */
    for (y = y_start; y <= y_end; y++)
        buf[(ULONG)y * stride + sl + 1] = lighten_inner[buf[(ULONG)y * stride + sl + 1]];

    /*====================================================================*/
    /* Segment 7: Outer right column — 50% darken, rows st+1 .. bot1-1   */
    /*====================================================================*/
    y_start = (UWORD)(st + 1);
    y_end   = (UWORD)(st + sh - 2);
    for (y = y_start; y <= y_end; y++)
        buf[(ULONG)y * stride + right1] = darken_outer[buf[(ULONG)y * stride + right1]];

    /*====================================================================*/
    /* Segment 8: Inner right column — 25% darken, rows st+2 .. bot2-1   */
    /*====================================================================*/
    y_start = (UWORD)(st + 2);
    y_end   = (UWORD)(st + sh - 3);
    for (y = y_start; y <= y_end; y++)
        buf[(ULONG)y * stride + right2] = darken_inner[buf[(ULONG)y * stride + right2]];

    log_debug(LOG_ICONS,
              "itidy_apply_thumbnail_bevel: bevel applied "
              "(safe %ux%u at %u,%u, palette now %lu entries)\n",
              (unsigned)sw, (unsigned)sh,
              (unsigned)sl, (unsigned)st,
              img->palette_size_normal);
}
