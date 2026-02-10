/*
 * icon_content_preview.c - Content Preview Orchestration
 *
 * Phase 3 integration module — orchestrates the full pipeline of
 * loading an icon, rendering a content preview into its pixel buffer,
 * and saving the result. Called from the DefIcons creation pipeline
 * after a template .info has been copied to the target location.
 *
 * See: docs/Default icons/Customize icons/Icon_Image_Editing_Plan.md
 *      Section 5 — Phase 3
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stdio.h>
#include <string.h>

#include <console_output.h>

#include "icon_content_preview.h"
#include "icon_image_access.h"
#include "icon_text_render.h"
#include "icon_iff_render.h"
#include "../deficons_templates.h"
#include "../writeLog.h"
#include "../layout_preferences.h"

/*========================================================================*/
/* itidy_is_text_preview_type                                             */
/*========================================================================*/

BOOL itidy_is_text_preview_type(const char *type_token)
{
    const char *category;

    if (type_token == NULL || type_token[0] == '\0')
    {
        return FALSE;
    }

    // Direct match — "ascii" itself is the base text type
    if (strcmp(type_token, "ascii") == 0)
    {
        return TRUE;
    }

    // Check root category — all children of "ascii" are text types
    // (e.g., "c", "h", "rexx", "html", "python", "make", etc.)
    category = deficons_get_resolved_category(type_token);
    if (category != NULL && strcmp(category, "ascii") == 0)
    {
        return TRUE;
    }

    return FALSE;
}

/*========================================================================*/
/* itidy_is_iff_preview_type                                              */
/*========================================================================*/

BOOL itidy_is_iff_preview_type(const char *type_token)
{
    if (type_token == NULL || type_token[0] == '\0')
    {
        return FALSE;
    }

    // Direct match — only "ilbm" (standard IFF ILBM picture format)
    // Future: could also match "acbm" or broader "picture" category
    if (strcmp(type_token, "ilbm") == 0)
    {
        return TRUE;
    }

    return FALSE;
}

/*========================================================================*/
/* itidy_get_iff_icon_dimensions                                          */
/*========================================================================*/

UWORD itidy_get_iff_icon_dimensions(const LayoutPreferences *prefs)
{
    if (prefs != NULL)
    {
        switch (prefs->deficons_icon_size_mode)
        {
            case ITIDY_ICON_SIZE_SMALL:
                return ITIDY_IFF_SIZE_SMALL;
            case ITIDY_ICON_SIZE_LARGE:
                return ITIDY_IFF_SIZE_LARGE;
            default:
                break;
        }
    }

    return ITIDY_IFF_SIZE_MEDIUM;
}

/*========================================================================*/
/* itidy_content_preview_cache_valid                                      */
/*========================================================================*/

BOOL itidy_content_preview_cache_valid(const char *target_path,
                                       const char *type_token,
                                       ULONG source_size,
                                       const struct DateStamp *source_date)
{
    const char *expected_kind = NULL;

    if (target_path == NULL || type_token == NULL || source_date == NULL)
    {
        return FALSE;
    }

    // Determine expected kind for this type
    if (itidy_is_text_preview_type(type_token))
    {
        expected_kind = ITIDY_KIND_TEXT_PREVIEW;
    }
    else if (itidy_is_iff_preview_type(type_token))
    {
        expected_kind = ITIDY_KIND_IFF_THUMBNAIL;
    }
    else
    {
        // No preview type matched — no cache to validate
        return FALSE;
    }

    // Delegate to the Phase 1 cache check
    return itidy_check_cache_valid(target_path, expected_kind,
                                   source_size, source_date);
}

/*========================================================================*/
/* Internal: Build Selected Image (Safe-Area Index Swap)                  */
/*========================================================================*/

/**
 * @brief Build the selected image by swapping bg/text indices in safe area.
 *
 * If the template icon had a selected image, this function creates
 * a copy of the normal pixel buffer and then swaps the background
 * and text colour palette indices within the safe area only.
 * Pixels outside the safe area (template border artwork) are preserved.
 *
 * If bg_color_index is ITIDY_NO_BG_COLOR (255), skips the swap entirely
 * since there's no meaningful background color to swap.
 *
 * See plan Section 8: Selected Image Handling.
 */
static BOOL build_selected_image(iTidy_IconImageData *img,
                                 const iTidy_RenderParams *params)
{
    UBYTE *selected;
    UWORD row;
    ULONG buffer_size;

    // Skip selected image generation if "no background" mode
    if (params != NULL && params->bg_color_index == ITIDY_NO_BG_COLOR)
    {
        log_debug(LOG_ICONS, "build_selected_image: skipping (no background mode)\n");
        return TRUE;
    }

    if (img == NULL || params == NULL)
    {
        return FALSE;
    }

    // Only build selected if template had one
    if (!img->has_selected_image || img->pixel_data_selected == NULL)
    {
        return TRUE;  // Nothing to do — this is fine
    }

    buffer_size = (ULONG)img->width * (ULONG)img->height;
    if (buffer_size == 0)
    {
        return FALSE;
    }

    // Copy the normal buffer into the selected buffer
    // (pixel_data_selected was already allocated by extract)
    memcpy(img->pixel_data_selected, img->pixel_data_normal, buffer_size);

    // Swap bg and text indices within the safe area only
    for (row = 0; row < params->safe_height; row++)
    {
        UWORD y = params->safe_top + row;
        UWORD col;

        if (y >= img->height)
        {
            break;
        }

        for (col = 0; col < params->safe_width; col++)
        {
            UWORD x = params->safe_left + col;
            ULONG offset;
            UBYTE pixel;

            if (x >= img->width)
            {
                break;
            }

            offset = (ULONG)y * (ULONG)img->width + (ULONG)x;
            pixel = img->pixel_data_selected[offset];

            if (pixel == params->bg_color_index)
            {
                img->pixel_data_selected[offset] = params->text_color_index;
            }
            else if (pixel == params->text_color_index)
            {
                img->pixel_data_selected[offset] = params->bg_color_index;
            }
            // Other palette indices (border art, etc.) are untouched
        }
    }

    // Copy normal palette to selected palette if both exist
    if (img->palette_normal != NULL && img->palette_selected != NULL
        && img->palette_size_normal > 0)
    {
        ULONG pal_bytes = img->palette_size_normal * sizeof(struct ColorRegister);
        memcpy(img->palette_selected, img->palette_normal, pal_bytes);
        img->palette_size_selected = img->palette_size_normal;
    }

    log_debug(LOG_ICONS, "build_selected_image: safe area index swap complete "
              "(bg=%u <-> text=%u)\n",
              (unsigned)params->bg_color_index,
              (unsigned)params->text_color_index);

    return TRUE;
}

/*========================================================================*/
/* Internal: Selected Image Contrast Validation                           */
/*========================================================================*/

/**
 * @brief Ensure the selected image is visually distinct from normal.
 *
 * IFF thumbnails can have mid-tone palettes where the bg/text index
 * swap produces near-identical colors. This samples the safe area
 * and checks that the average luminance delta between normal and
 * selected pixels exceeds a threshold.
 *
 * If contrast is insufficient, applies a uniform brightness shift
 * to the selected image's safe-area pixels as a fallback.
 *
 * @param img       Icon image data with both normal and selected buffers
 * @param params    Render params with safe area coordinates
 * @return TRUE if contrast is sufficient (or was corrected)
 */
static BOOL validate_selected_contrast(const iTidy_IconImageData *img,
                                       const iTidy_RenderParams *params)
{
    ULONG sample_count = 0;
    ULONG delta_sum = 0;
    UWORD row, col;
    UWORD step;
    ULONG avg_delta;

    if (img == NULL || params == NULL)
    {
        return TRUE;
    }

    if (!img->has_selected_image || img->pixel_data_selected == NULL)
    {
        return TRUE;  // No selected image to validate
    }

    if (img->palette_normal == NULL || img->palette_size_normal == 0)
    {
        return TRUE;
    }

    // Sample a grid of pixels (every 4th pixel to keep it fast)
    step = 4;
    for (row = 0; row < params->safe_height; row += step)
    {
        UWORD y = params->safe_top + row;
        if (y >= img->height)
        {
            break;
        }

        for (col = 0; col < params->safe_width; col += step)
        {
            UWORD x = params->safe_left + col;
            ULONG offset;
            UBYTE idx_normal, idx_selected;
            LONG lum_normal, lum_selected, delta;

            if (x >= img->width)
            {
                break;
            }

            offset = (ULONG)y * (ULONG)img->width + (ULONG)x;
            idx_normal = img->pixel_data_normal[offset];
            idx_selected = img->pixel_data_selected[offset];

            // Calculate luminance (approximate: (R*2 + G*5 + B) / 8)
            if (idx_normal < img->palette_size_normal)
            {
                lum_normal = ((LONG)img->palette_normal[idx_normal].red * 2
                            + (LONG)img->palette_normal[idx_normal].green * 5
                            + (LONG)img->palette_normal[idx_normal].blue) / 8;
            }
            else
            {
                lum_normal = 0;
            }

            if (idx_selected < img->palette_size_normal)
            {
                lum_selected = ((LONG)img->palette_normal[idx_selected].red * 2
                              + (LONG)img->palette_normal[idx_selected].green * 5
                              + (LONG)img->palette_normal[idx_selected].blue) / 8;
            }
            else
            {
                lum_selected = 0;
            }

            delta = lum_normal - lum_selected;
            if (delta < 0) delta = -delta;
            delta_sum += (ULONG)delta;
            sample_count++;
        }
    }

    if (sample_count == 0)
    {
        return TRUE;
    }

    // Average luminance delta across sampled pixels
    avg_delta = delta_sum / sample_count;

    // Minimum contrast threshold: 20 out of 255 luminance range
    if (avg_delta < 20)
    {
        log_warning(LOG_ICONS, "validate_selected_contrast: "
                    "low contrast (avg delta=%lu) — selected state may be hard to see\n",
                    avg_delta);
        // TODO: Future enhancement — apply brightness shift fallback
        // For now, just warn. The icon will still work, just less visible.
        return FALSE;
    }

    log_debug(LOG_ICONS, "validate_selected_contrast: OK (avg delta=%lu)\n",
              avg_delta);
    return TRUE;
}

/*========================================================================*/
/* Internal: Apply IFF Thumbnail Preview                                  */
/*========================================================================*/

/**
 * @brief Complete IFF thumbnail rendering pipeline.
 *
 * Streamlined pipeline (template-free):
 * load target icon → create blank image from size preference →
 * build render params from preferences → render thumbnail →
 * crop to content → apply to target clone → stamp → save → cleanup.
 *
 * The target icon (Workbench deficon) provides all metadata (default
 * tool, tooltypes, icon type). Image dimensions come from the
 * deficons_icon_size_mode preference; palette mode comes from
 * deficons_palette_mode. No separate template .info files needed.
 */
static int apply_iff_preview(const char *source_path,
                             const char *type_token,
                             ULONG source_size,
                             const struct DateStamp *source_date)
{
    struct DiskObject *target_icon = NULL;
    struct DiskObject *clone = NULL;
    iTidy_IconImageData img;
    iTidy_IFFRenderParams iff_params;
    const LayoutPreferences *prefs;
    const char *source_name;
    UWORD icon_dim;
    int render_result;
    int result = ITIDY_PREVIEW_FAILED;

    log_info(LOG_ICONS, "apply_iff_preview: applying IFF thumbnail "
             "for '%s' (type=%s)\n", source_path, type_token);

    /*--------------------------------------------------------------------*/
    /* Step 1: Load the target icon (Workbench metadata template)         */
    /*--------------------------------------------------------------------*/

    target_icon = GetDiskObject((STRPTR)source_path);
    if (target_icon == NULL)
    {
        log_error(LOG_ICONS, "apply_iff_preview: "
                  "cannot load target icon for '%s'\n", source_path);
        return ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 2: Determine icon dimensions from size preference              */
    /*         No template file needed — size comes from preferences,      */
    /*         PICTURE palette mode replaces all pixels and palette.       */
    /*--------------------------------------------------------------------*/

    prefs = GetGlobalPreferences();
    icon_dim = itidy_get_iff_icon_dimensions(prefs);

    log_info(LOG_ICONS, "apply_iff_preview: icon size=%ux%u (mode=%u), "
             "palette_mode=%u\n",
             (unsigned)icon_dim, (unsigned)icon_dim,
             (unsigned)(prefs ? prefs->deficons_icon_size_mode : 1),
             (unsigned)(prefs ? prefs->deficons_palette_mode : 0));

    /*--------------------------------------------------------------------*/
    /* Step 3: Create blank pixel buffer and placeholder palette           */
    /*         PICTURE mode replaces the entire palette and all pixels     */
    /*         with the IFF source image's CMAP/BODY data, so we only     */
    /*         need valid allocations for the pipeline to operate on.      */
    /*--------------------------------------------------------------------*/

    memset(&img, 0, sizeof(img));

    if (!itidy_icon_image_create_blank(icon_dim, icon_dim, 0, &img))
    {
        log_error(LOG_ICONS, "apply_iff_preview: "
                  "blank image creation failed for %ux%u\n",
                  (unsigned)icon_dim, (unsigned)icon_dim);
        FreeDiskObject(target_icon);
        return ITIDY_PREVIEW_FAILED;
    }

    // Create a placeholder palette (256 entries).
    // The IFF renderer in PICTURE mode will replace this entirely
    // with the source image's CMAP, so actual colours don't matter.
    {
        ULONG pal_entries = 256;
        img.palette_size_normal = pal_entries;
        img.palette_normal = (struct ColorRegister *)whd_malloc(
            pal_entries * sizeof(struct ColorRegister));
        if (img.palette_normal == NULL)
        {
            log_error(LOG_ICONS, "apply_iff_preview: "
                      "placeholder palette alloc failed\n");
            itidy_icon_image_free(&img);
            FreeDiskObject(target_icon);
            return ITIDY_PREVIEW_FAILED;
        }
        memset(img.palette_normal, 0,
               pal_entries * sizeof(struct ColorRegister));

        // Entry 0 = light grey (typical background)
        img.palette_normal[0].red   = 0xAA;
        img.palette_normal[0].green = 0xAA;
        img.palette_normal[0].blue  = 0xAA;
        // Entry 1 = black (typical foreground) — already zero
    }

    /*--------------------------------------------------------------------*/
    /* Step 4: Build IFF render parameters directly from preferences       */
    /*         No template ToolTypes needed — all parameters are either    */
    /*         derived from preferences or use fixed IFF-path defaults.    */
    /*--------------------------------------------------------------------*/

    memset(&iff_params, 0, sizeof(iff_params));

    // Palette mode from preferences (default: PICTURE)
    iff_params.palette_mode = (prefs != NULL)
        ? prefs->deficons_palette_mode
        : ITIDY_PAL_PICTURE;

    // Point the render params at the blank pixel buffer
    iff_params.base.pixel_buffer = img.pixel_data_normal;
    iff_params.base.buffer_width = img.width;
    iff_params.base.buffer_height = img.height;

    // IFF thumbnails use the full icon dimensions for scaling (no safe
    // area margins). After rendering, the buffer is cropped to the exact
    // thumbnail dimensions so the icon size matches the content.
    iff_params.base.safe_left = 0;
    iff_params.base.safe_top = 0;
    iff_params.base.safe_width = img.width;
    iff_params.base.safe_height = img.height;
    iff_params.base.bg_color_index = ITIDY_NO_BG_COLOR;

    log_debug(LOG_ICONS, "apply_iff_preview: params ready — "
              "buffer=%ux%u, palette_mode=%u\n",
              (unsigned)img.width, (unsigned)img.height,
              (unsigned)iff_params.palette_mode);

    /*--------------------------------------------------------------------*/
    /* Step 6: Render IFF thumbnail (parse, decode, scale)                */
    /*--------------------------------------------------------------------*/

    render_result = itidy_render_iff_thumbnail(source_path, &iff_params, &img);
    if (render_result != 0)
    {
        if (render_result == ITIDY_PREVIEW_NOT_SUPPORTED)
        {
            log_warning(LOG_ICONS, "apply_iff_preview: "
                        "unsupported IFF format (HAM?) for '%s'\n", source_path);
            result = ITIDY_PREVIEW_NOT_SUPPORTED;
        }
        else
        {
            log_warning(LOG_ICONS, "apply_iff_preview: "
                        "render failed for '%s' (error=%d)\n",
                        source_path, render_result);
            result = ITIDY_PREVIEW_FAILED;
        }
        itidy_iff_params_free(&iff_params);
        itidy_icon_image_free(&img);
        FreeDiskObject(target_icon);
        return result;
    }

    /*--------------------------------------------------------------------*/
    /* Step 6b: Crop pixel buffer to exact thumbnail dimensions            */
    /*          Remove letterbox/pillarbox areas so the icon size matches  */
    /*          the thumbnail content. Disable transparency so palette     */
    /*          index 0 (often black) is visible, not transparent.         */
    /*--------------------------------------------------------------------*/

    {
        UWORD thumb_w = iff_params.output_width;
        UWORD thumb_h = iff_params.output_height;
        UWORD src_ox = iff_params.output_offset_x;
        UWORD src_oy = iff_params.output_offset_y;
        ULONG tight_size = (ULONG)thumb_w * (ULONG)thumb_h;
        UBYTE *tight_buf;
        UWORD row;

        tight_buf = (UBYTE *)whd_malloc(tight_size);
        if (tight_buf == NULL)
        {
            log_error(LOG_ICONS, "apply_iff_preview: "
                      "crop alloc failed (%ux%u)\n",
                      (unsigned)thumb_w, (unsigned)thumb_h);
            itidy_iff_params_free(&iff_params);
            itidy_icon_image_free(&img);
            FreeDiskObject(target_icon);
            return ITIDY_PREVIEW_FAILED;
        }

        // Copy just the thumbnail rows from the rendered buffer
        for (row = 0; row < thumb_h; row++)
        {
            ULONG src_off = (ULONG)(src_oy + row) * (ULONG)img.width
                          + (ULONG)src_ox;
            ULONG dst_off = (ULONG)row * (ULONG)thumb_w;
            memcpy(tight_buf + dst_off,
                   img.pixel_data_normal + src_off, thumb_w);
        }

        // Replace pixel buffer with the tight crop
        whd_free(img.pixel_data_normal);
        img.pixel_data_normal = tight_buf;
        img.width = thumb_w;
        img.height = thumb_h;

        // Discard selected image — edge-to-edge thumbnails have no safe
        // area for the bg/text index swap to operate on
        if (img.pixel_data_selected != NULL)
        {
            whd_free(img.pixel_data_selected);
            img.pixel_data_selected = NULL;
        }
        img.has_selected_image = FALSE;
        img.has_real_selected_image = FALSE;

        // Disable transparency — palette index 0 is often black and
        // must be visible in the thumbnail, not treated as transparent
        img.transparent_color_normal = -1;
        img.transparent_color_selected = -1;

        // Frameless — the thumbnail IS the icon, no border/frame needed
        img.is_frameless = TRUE;

        log_info(LOG_ICONS, "apply_iff_preview: cropped to tight %ux%u, "
                 "transparency disabled, frameless\n",
                 (unsigned)thumb_w, (unsigned)thumb_h);
    }

    /*--------------------------------------------------------------------*/
    /* Step 7: Debug dump (guarded by DEBUG_ICON_DUMP)                     */
    /*--------------------------------------------------------------------*/

    itidy_icon_image_dump(&img, &iff_params.base, source_path);

    /*--------------------------------------------------------------------*/
    /* Step 9: Apply modified image to a DiskObject clone                  */
    /*--------------------------------------------------------------------*/

    clone = itidy_icon_image_apply(target_icon, &img);
    if (clone == NULL)
    {
        log_error(LOG_ICONS, "apply_iff_preview: "
                  "image apply failed for '%s'\n", source_path);
        itidy_iff_params_free(&iff_params);
        itidy_icon_image_free(&img);
        FreeDiskObject(target_icon);
        return ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 10: Stamp ToolTypes on the clone                               */
    /*--------------------------------------------------------------------*/

    source_name = (const char *)FilePart((STRPTR)source_path);

    if (!itidy_stamp_created_tooltypes(clone, ITIDY_KIND_IFF_THUMBNAIL,
                                       source_name, source_size, source_date))
    {
        log_warning(LOG_ICONS, "apply_iff_preview: "
                    "ToolType stamp failed for '%s' (non-fatal)\n", source_path);
    }

    /*--------------------------------------------------------------------*/
    /* Step 11: Save the modified icon to disk                             */
    /*--------------------------------------------------------------------*/

    if (itidy_icon_image_save(source_path, clone))
    {
        log_info(LOG_ICONS, "apply_iff_preview: "
                 "saved IFF thumbnail icon for '%s'\n", source_path);
        result = ITIDY_PREVIEW_APPLIED;
    }
    else
    {
        log_error(LOG_ICONS, "apply_iff_preview: "
                  "save failed for '%s'\n", source_path);
        result = ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 12: Cleanup                                                    */
    /*--------------------------------------------------------------------*/

    // Free IFF-specific buffers (src_chunky, src_palette)
    itidy_iff_params_free(&iff_params);

    // CRITICAL: Free clone BEFORE image data (clone references pixel buffers)
    FreeDiskObject(clone);
    clone = NULL;

    itidy_icon_image_free(&img);
    FreeDiskObject(target_icon);
    target_icon = NULL;

    return result;
}

/*========================================================================*/
/* itidy_apply_content_preview                                            */
/*========================================================================*/

int itidy_apply_content_preview(const char *source_path,
                                const char *type_token,
                                ULONG source_size,
                                const struct DateStamp *source_date)
{
    struct DiskObject *target_icon = NULL;   /* Workbench template (metadata) */
    struct DiskObject *image_icon = NULL;    /* Image template (color pixels) */
    struct DiskObject *clone = NULL;
    iTidy_IconImageData img;
    iTidy_TextRenderParams text_params;
    const char *source_name;
    int result = ITIDY_PREVIEW_FAILED;

    /*--------------------------------------------------------------------*/
    /* Validate parameters                                                */
    /*--------------------------------------------------------------------*/

    if (source_path == NULL || type_token == NULL || source_date == NULL)
    {
        log_error(LOG_ICONS, "itidy_apply_content_preview: NULL parameter\n");
        return ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Check if this type supports content preview                        */
    /*--------------------------------------------------------------------*/

    if (itidy_is_text_preview_type(type_token))
    {
        // Fall through to text preview pipeline below
    }
    else if (itidy_is_iff_preview_type(type_token))
    {
        return apply_iff_preview(source_path, type_token,
                                 source_size, source_date);
    }
    else
    {
        return ITIDY_PREVIEW_NOT_APPLICABLE;
    }

    log_info(LOG_ICONS, "itidy_apply_content_preview: applying text preview "
             "for '%s' (type=%s)\n", source_path, type_token);

    /*--------------------------------------------------------------------*/
    /* Step 1: Load the target icon (the already-copied Workbench         */
    /*         template). This has the correct default tool, tooltypes,   */
    /*         icon type, and other metadata we want to preserve.         */
    /*--------------------------------------------------------------------*/

    target_icon = GetDiskObject((STRPTR)source_path);
    if (target_icon == NULL)
    {
        log_error(LOG_ICONS, "itidy_apply_content_preview: "
                  "cannot load target icon for '%s'\n", source_path);
        return ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 2: Load the image template (custom color icon with pixel      */
    /*         data). This is separate from the Workbench default —       */
    /*         it is iTidy's own palette-mapped icon designed for          */
    /*         content preview rendering.                                 */
    /*                                                                    */
    /*         CRITICAL: We use GetIconTagList() instead of the older     */
    /*         GetDiskObject() because GetDiskObject() does NOT fully     */
    /*         activate palette-mapped image data. The AutoDocs note:     */
    /*           "palette mapped image data that was never put to use     */
    /*            (this happens if it is retrieved with GetDiskObject()   */
    /*            rather than the new GetIconTagList() call)"             */
    /*                                                                    */
    /*         We also set ICONGETA_RemapIcon=FALSE so we get the raw     */
    /*         original palette indices, not screen-remapped values.      */
    /*         This lets us modify and save the pixel data cleanly.       */
    /*--------------------------------------------------------------------*/

    {
        struct TagItem get_template_tags[3];
        get_template_tags[0].ti_Tag  = ICONGETA_RemapIcon;
        get_template_tags[0].ti_Data = FALSE;
        get_template_tags[1].ti_Tag  = ICONGETA_GenerateImageMasks;
        get_template_tags[1].ti_Data = FALSE;
        get_template_tags[2].ti_Tag  = TAG_DONE;
        get_template_tags[2].ti_Data = 0;

        image_icon = GetIconTagList((STRPTR)ITIDY_TEXT_TEMPLATE_PATH,
                                    get_template_tags);
    }
    if (image_icon == NULL)
    {
        log_error(LOG_ICONS, "itidy_apply_content_preview: "
                  "cannot load image template '%s' — ensure the "
                  "Icons drawer exists in the program directory\n",
                  ITIDY_TEXT_TEMPLATE_PATH);
        FreeDiskObject(target_icon);
        return ITIDY_PREVIEW_FAILED;
    }

    log_info(LOG_ICONS, "itidy_apply_content_preview: loaded image template "
             "from '%s'\n", ITIDY_TEXT_TEMPLATE_PATH);

    /*--------------------------------------------------------------------*/
    /* Step 3: Extract image data from the IMAGE TEMPLATE (not the        */
    /*         target). The image template is a palette-mapped color      */
    /*         icon, so this succeeds where the old planar Workbench      */
    /*         icon would fail.                                           */
    /*--------------------------------------------------------------------*/

    memset(&img, 0, sizeof(img));

    if (!itidy_icon_image_extract(image_icon, &img))
    {
        log_error(LOG_ICONS, "itidy_apply_content_preview: "
                  "image extract failed from template '%s'\n",
                  ITIDY_TEXT_TEMPLATE_PATH);
        FreeDiskObject(image_icon);
        FreeDiskObject(target_icon);
        return ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 4: Build render parameters from the IMAGE TEMPLATE.           */
    /*         The template may contain ITIDY_TEXT_AREA, ITIDY_BG_COLOR,  */
    /*         ITIDY_TEXT_COLOR and other ToolTypes for rendering.         */
    /*--------------------------------------------------------------------*/

    memset(&text_params, 0, sizeof(text_params));

    if (!itidy_get_render_params(image_icon, &img, &text_params))
    {
        log_error(LOG_ICONS, "itidy_apply_content_preview: "
                  "get_render_params failed for image template\n");
        itidy_icon_image_free(&img);
        FreeDiskObject(image_icon);
        FreeDiskObject(target_icon);
        return ITIDY_PREVIEW_FAILED;
    }

    // Done with image template DiskObject — extracted data is our own copy
    FreeDiskObject(image_icon);
    image_icon = NULL;

    // Point the render params at the normal pixel buffer
    text_params.base.pixel_buffer = img.pixel_data_normal;
    text_params.base.buffer_width = img.width;
    text_params.base.buffer_height = img.height;

    /*--------------------------------------------------------------------*/
    /* Step 4: Call the ASCII text renderer                                */
    /*--------------------------------------------------------------------*/

    if (!itidy_render_ascii_preview(source_path, &text_params))
    {
        log_warning(LOG_ICONS, "itidy_apply_content_preview: "
                    "text render returned FALSE for '%s' (binary or empty)\n",
                    source_path);
        // Not a hard failure — icon keeps the unmodified template image
        if (text_params.darken_table != NULL)
        {
            whd_free(text_params.darken_table);
        }
        itidy_icon_image_free(&img);
        FreeDiskObject(target_icon);
        return ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 5: Build selected image (if template had one)                  */
    /*--------------------------------------------------------------------*/

    if (img.has_selected_image && img.pixel_data_selected != NULL)
    {
        if (!build_selected_image(&img, &text_params.base))
        {
            log_warning(LOG_ICONS, "itidy_apply_content_preview: "
                        "selected image build failed for '%s'\n", source_path);
            // Non-fatal — continue with normal image only
            // Null out the selected buffer so apply() skips it
            if (img.pixel_data_selected)
            {
                whd_free(img.pixel_data_selected);
                img.pixel_data_selected = NULL;
            }
            img.has_selected_image = FALSE;
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 6: Debug dump (guarded by DEBUG_ICON_DUMP)                     */
    /*--------------------------------------------------------------------*/

    itidy_icon_image_dump(&img, &text_params.base, source_path);

    /*--------------------------------------------------------------------*/
    /* Step 7: Apply modified image to a DiskObject clone                  */
    /*--------------------------------------------------------------------*/

    // Clone the TARGET icon (preserves Workbench metadata: default tool,
    // tooltypes, icon type, drawer data, stack, etc.) and apply the
    // modified pixel/palette data from the image template.
    clone = itidy_icon_image_apply(target_icon, &img);
    if (clone == NULL)
    {
        log_error(LOG_ICONS, "itidy_apply_content_preview: "
                  "image apply failed for '%s'\n", source_path);
        if (text_params.darken_table != NULL)
        {
            whd_free(text_params.darken_table);
        }
        itidy_icon_image_free(&img);
        FreeDiskObject(target_icon);
        return ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 8: Stamp ToolTypes on the clone                                */
    /*--------------------------------------------------------------------*/

    source_name = (const char *)FilePart((STRPTR)source_path);

    if (!itidy_stamp_created_tooltypes(clone, ITIDY_KIND_TEXT_PREVIEW,
                                       source_name, source_size, source_date))
    {
        log_warning(LOG_ICONS, "itidy_apply_content_preview: "
                    "ToolType stamp failed for '%s' (non-fatal)\n", source_path);
        // Non-fatal — the icon will still be saved without stamps
    }

    /*--------------------------------------------------------------------*/
    /* Step 9: Save the modified icon to disk                              */
    /*--------------------------------------------------------------------*/

    if (itidy_icon_image_save(source_path, clone))
    {
        log_info(LOG_ICONS, "itidy_apply_content_preview: "
                 "saved text preview icon for '%s'\n", source_path);
        result = ITIDY_PREVIEW_APPLIED;
    }
    else
    {
        log_error(LOG_ICONS, "itidy_apply_content_preview: "
                  "save failed for '%s'\n", source_path);
        result = ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 10: Cleanup                                                    */
    /*--------------------------------------------------------------------*/

    // Free adaptive text lookup table if allocated
    if (text_params.darken_table != NULL)
    {
        whd_free(text_params.darken_table);
        text_params.darken_table = NULL;
    }

    // CRITICAL: Free the clone BEFORE freeing the image data, because
    // the clone references the pixel/palette buffers we set via
    // IconControlA (they are NOT copied by icon.library).
    FreeDiskObject(clone);
    clone = NULL;

    // Now safe to free the image data
    itidy_icon_image_free(&img);

    // Free the target icon (Workbench template we cloned metadata from)
    FreeDiskObject(target_icon);
    target_icon = NULL;

    return result;
}
