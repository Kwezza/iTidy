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
#include "ASCII/icon_text_render.h"
#include "Image/icon_iff_render.h"
#include "Image/icon_image_bevel.h"
#include "palette/palette_reduction.h"
#include "palette/palette_harmonised.h"
#include "palette/ultra_downsample.h"
#include "../deficons/deficons_templates.h"
#include "../writeLog.h"
#include "../layout_preferences.h"
#include "../GUI/StatusWindows/main_progress_window.h"  /* For cancel flag */

/*========================================================================*/
/* Helper: Check if type is excluded via EXCLUDETYPE tooltype            */
/*========================================================================*/

/**
 * @brief Check if a file type should be excluded from text preview
 * 
 * Reads the EXCLUDETYPE tooltype from Icons/def_ascii.info to check
 * if the given type token should bypass text preview rendering.
 * 
 * Format: EXCLUDETYPE=amigaguide,html,install
 * 
 * @param type_token File type to check (e.g., "amigaguide", "c", "rexx")
 * @return TRUE if type should be excluded, FALSE otherwise
 * 
 * @note Case-insensitive matching
 * @note Handles comma-separated lists with optional whitespace
 */
static BOOL is_excluded_from_text_preview(const char *type_token)
{
    struct DiskObject *ascii_template = NULL;
    char **tooltypes = NULL;
    char *exclude_list = NULL;
    BOOL excluded = FALSE;
    char buffer[256];
    char *token;
    char *token_start;
    
    if (type_token == NULL || type_token[0] == '\0')
    {
        return FALSE;
    }
    
    /* Load the base ASCII template to read its tooltypes */
    ascii_template = GetDiskObject((STRPTR)"Icons/def_ascii");
    if (ascii_template == NULL)
    {
        log_debug(LOG_ICONS, "Could not load Icons/def_ascii.info for EXCLUDETYPE check\n");
        return FALSE;
    }
    
    /* Look for EXCLUDETYPE tooltype */
    tooltypes = (char **)ascii_template->do_ToolTypes;
    if (tooltypes != NULL)
    {
        exclude_list = (char *)FindToolType(tooltypes, "EXCLUDETYPE");
        if (exclude_list != NULL)
        {
            log_debug(LOG_ICONS, "Found EXCLUDETYPE tooltype: %s\n", exclude_list);
            
            /* Parse comma-separated list (case-insensitive) */
            strncpy(buffer, exclude_list, sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
            
            token = strtok(buffer, ",");
            while (token != NULL)
            {
                /* Trim leading whitespace */
                token_start = token;
                while (*token_start == ' ' || *token_start == '\t')
                {
                    token_start++;
                }
                
                /* Trim trailing whitespace */
                {
                    char *token_end = token_start + strlen(token_start) - 1;
                    while (token_end > token_start && 
                           (*token_end == ' ' || *token_end == '\t'))
                    {
                        *token_end = '\0';
                        token_end--;
                    }
                }
                
                /* Case-insensitive compare */
                if (platform_stricmp(token_start, type_token) == 0)
                {
                    log_info(LOG_ICONS, "Type '%s' excluded from text preview via EXCLUDETYPE tooltype\n",
                             type_token);
                    excluded = TRUE;
                    break;
                }
                
                token = strtok(NULL, ",");
            }
        }
    }
    
    FreeDiskObject(ascii_template);
    return excluded;
}

/*========================================================================*/
/* Template Resolution                                                    */
/*========================================================================*/

/**
 * @brief Resolve the preview template path for a given DefIcons type.
 *
 * Implements two-level fallback chain:
 *   1. PROGDIR:Icons/def_<type_token>  (type-specific template)
 *   2. PROGDIR:Icons/def_ascii         (generic ASCII category template)
 *
 * Each level is checked for existence via Lock() before proceeding to
 * the next fallback level. If neither template exists, returns FALSE so
 * the caller can skip rendering and let the normal DefIcons system handle
 * the icon — no text preview is applied.
 *
 * @param type_token    DefIcons type token (e.g., "c", "rexx", "html")
 * @param path_buf      Output buffer for resolved template path
 * @param buf_size      Size of path_buf in bytes
 * @return TRUE if a usable template was found, FALSE if none exist
 *
 * @note The returned path does NOT include the .info extension
 * @note All logging to LOG_ICONS at info level for template selection tracking
 */
static BOOL itidy_resolve_preview_template(const char *type_token,
                                            char *path_buf,
                                            int buf_size)
{
    BPTR lock = 0;
    char test_path[256];
    
    if (type_token == NULL || type_token[0] == '\0' || 
        path_buf == NULL || buf_size < 32)
    {
        log_error(LOG_ICONS, "itidy_resolve_preview_template: invalid parameters\n");
        return FALSE;
    }
    
    /*--------------------------------------------------------------------*/
    /* Level 1: Try type-specific template (PROGDIR:Icons/def_<type>)     */
    /*--------------------------------------------------------------------*/
    
    snprintf(test_path, sizeof(test_path), "PROGDIR:Icons/def_%s.info", type_token);
    test_path[sizeof(test_path) - 1] = '\0';
    
    log_debug(LOG_ICONS, "Trying type-specific template: %s\n", test_path);
    
    lock = Lock((STRPTR)test_path, ACCESS_READ);
    if (lock != 0)
    {
        UnLock(lock);
        snprintf(path_buf, buf_size, "PROGDIR:Icons/def_%s", type_token);
        path_buf[buf_size - 1] = '\0';
        log_debug(LOG_ICONS, "Using type-specific preview template: %s for type=%s\n",
                 path_buf, type_token);
        return TRUE;
    }
    
    /*--------------------------------------------------------------------*/
    /* Level 2: Try generic ASCII category template (def_ascii)           */
    /*--------------------------------------------------------------------*/
    
    snprintf(test_path, sizeof(test_path), "PROGDIR:Icons/def_ascii.info");
    test_path[sizeof(test_path) - 1] = '\0';
    
    log_debug(LOG_ICONS, "Type-specific template not found, trying generic: %s\n",
              test_path);
    
    lock = Lock((STRPTR)test_path, ACCESS_READ);
    if (lock != 0)
    {
        UnLock(lock);
        snprintf(path_buf, buf_size, "PROGDIR:Icons/def_ascii");
        path_buf[buf_size - 1] = '\0';
        log_debug(LOG_ICONS, "Using generic ASCII preview template: %s for type=%s\n",
                 path_buf, type_token);
        return TRUE;
    }
    
    /*--------------------------------------------------------------------*/
    /* No template found — caller should skip rendering and use the       */
    /* normal DefIcons system icon for this file type instead.            */
    /*--------------------------------------------------------------------*/
    
    log_info(LOG_ICONS, "No preview template found for type=%s "
             "(neither def_%s.info nor def_ascii.info exists) — "
             "skipping text preview\n", type_token, type_token);
    
    return FALSE;
}

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

    // Strictly ILBM only — the native iffparse.library path handles this.
    // All other picture formats are routed through itidy_is_picture_preview_type().
    if (strcmp(type_token, "ilbm") == 0)
    {
        return TRUE;
    }

    return FALSE;
}

/*========================================================================*/
/* itidy_is_picture_preview_type                                          */
/*========================================================================*/

BOOL itidy_is_picture_preview_type(const char *type_token)
{
    if (type_token == NULL || type_token[0] == '\0')
    {
        return FALSE;
    }

    // Non-ILBM picture formats handled via picture.datatype.
    // "ilbm" is deliberately excluded here (handled by itidy_is_iff_preview_type).
    if (strcmp(type_token, "png")       == 0) return TRUE;
    if (strcmp(type_token, "gif")       == 0) return TRUE;
    if (strcmp(type_token, "jpeg")      == 0) return TRUE;
    if (strcmp(type_token, "bmp")       == 0) return TRUE;
    if (strcmp(type_token, "acbm")      == 0) return TRUE;
    if (strcmp(type_token, "tiff")      == 0) return TRUE;
    if (strcmp(type_token, "targa")     == 0) return TRUE;
    if (strcmp(type_token, "pcx")       == 0) return TRUE;
    if (strcmp(type_token, "ico")       == 0) return TRUE;
    if (strcmp(type_token, "sunraster") == 0) return TRUE;
    if (strcmp(type_token, "reko")      == 0) return TRUE;
    if (strcmp(type_token, "brush")     == 0) return TRUE;
    if (strcmp(type_token, "PicView")   == 0) return TRUE;

    return FALSE;
}

/*========================================================================*/
/* type_token_to_format_bit — map type token to ITIDY_PICTFMT_* bit       */
/*========================================================================*/

/**
 * @brief Return the ITIDY_PICTFMT_* bitmask bit for a type token.
 *
 * Used to check whether a format is enabled in
 * prefs->deficons_picture_formats_enabled before attempting thumbnail
 * generation. Returns ITIDY_PICTFMT_OTHER for any format not explicitly
 * listed, so unknown/third-party formats inherit the OTHER toggle.
 *
 * Note: "ilbm" returns ITIDY_PICTFMT_ILBM and is checked in
 * apply_iff_preview() rather than apply_picture_preview().
 */
static ULONG type_token_to_format_bit(const char *type_token)
{
    if (type_token == NULL) return ITIDY_PICTFMT_OTHER;

    if (strcmp(type_token, "ilbm")  == 0) return ITIDY_PICTFMT_ILBM;
    if (strcmp(type_token, "acbm")  == 0) return ITIDY_PICTFMT_ACBM;
    if (strcmp(type_token, "png")   == 0) return ITIDY_PICTFMT_PNG;
    if (strcmp(type_token, "gif")   == 0) return ITIDY_PICTFMT_GIF;
    if (strcmp(type_token, "jpeg")  == 0) return ITIDY_PICTFMT_JPEG;
    if (strcmp(type_token, "bmp")   == 0) return ITIDY_PICTFMT_BMP;

    return ITIDY_PICTFMT_OTHER;
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
 * IMPORTANT: If the template has a REAL selected image (not auto-dimmed),
 * the selected buffer already has the text rendered on it (done during
 * the text rendering phase). In this case, we skip the index swap entirely
 * and just copy the palette.
 *
 * If bg_color_index is ITIDY_NO_BG_COLOUR (255), skips the swap entirely
 * since there's no meaningful background colour to swap.
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
    if (params != NULL && params->bg_color_index == ITIDY_NO_BG_COLOUR)
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

    // If template has a REAL selected image (not auto-dimmed), the text
    // has already been rendered onto it during the rendering phase.
    // Just copy the palette and return.
    if (img->has_real_selected_image)
    {
        // Copy normal palette to selected palette if both exist
        if (img->palette_normal != NULL && img->palette_selected != NULL
            && img->palette_size_normal > 0)
        {
            ULONG pal_bytes = img->palette_size_normal * sizeof(struct ColorRegister);
            memcpy(img->palette_selected, img->palette_normal, pal_bytes);
            img->palette_size_selected = img->palette_size_normal;
        }

        log_debug(LOG_ICONS, "build_selected_image: template has real selected image, "
                  "skipping index swap (text already rendered)\n");
        return TRUE;
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
 * deficons_icon_size_mode preference. No separate template .info files needed.
 */
static int apply_iff_preview(const char *source_path,
                             const char *type_token,
                             ULONG source_size,
                             const struct DateStamp *source_date,
                             void (*progress_callback)(void *, const char *, ULONG, ULONG),
                             void *progress_user_data)
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
    BOOL want_bevel = FALSE;  /* Set by border section; controls bevel application */

    memset(&img, 0, sizeof(img));
    memset(&iff_params, 0, sizeof(iff_params));

    /* Debug: Log incoming parameters */
    log_debug(LOG_ICONS, "apply_iff_preview: progress_callback=%p, progress_user_data=%p\n",
              (void*)progress_callback, progress_user_data);

    log_debug(LOG_ICONS, "apply_iff_preview: applying IFF thumbnail "
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

    log_debug(LOG_ICONS, "apply_iff_preview: icon size=%ux%u (mode=%u)\n",
             (unsigned)icon_dim, (unsigned)icon_dim,
             (unsigned)(prefs ? prefs->deficons_icon_size_mode : 1));

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

    /* Initialize progress callback (AFTER memset!) */
    iff_params.progress_callback = (iTidy_ProgressCallback)progress_callback;
    iff_params.progress_user_data = progress_user_data;
    iff_params.last_progress_ticks = 0;
    
    /* Initialize cancel flag - points to progress window's cancel_requested if available */
    if (progress_user_data != NULL)
    {
        struct iTidyMainProgressWindow *pw = (struct iTidyMainProgressWindow *)progress_user_data;
        iff_params.cancel_flag = &pw->cancel_requested;
    }
    else
    {
        iff_params.cancel_flag = NULL;
    }

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
    iff_params.base.bg_color_index = ITIDY_NO_BG_COLOUR;
    iff_params.allow_upscale    = prefs ? prefs->deficons_upscale_thumbnails : FALSE;
    iff_params.try_magenta_key  = TRUE;  /* ILBM/ACBM can use magenta key transparency */

    log_debug(LOG_ICONS, "apply_iff_preview: params ready — "
              "buffer=%ux%u\n",
              (unsigned)img.width, (unsigned)img.height);

    /*--------------------------------------------------------------------*/
    /* Step 6: Render IFF thumbnail (parse, decode, scale)                */
    /*--------------------------------------------------------------------*/

    render_result = itidy_render_iff_thumbnail(source_path, &iff_params, &img);
    if (render_result != 0)
    {
        if (render_result == ITIDY_PREVIEW_NOT_SUPPORTED)
        {
            log_info(LOG_ICONS, "apply_iff_preview: "
                     "native parser cannot handle format (HAM?) - trying datatype fallback for '%s'\n",
                     source_path);
            
            /* Try datatype-based fallback (handles HAM, HAM8, and other formats) */
            render_result = itidy_render_via_datatype(source_path, &iff_params, &img,
                                                      TRUE /* ilbm_heuristics */);
            if (render_result != 0)
            {
                log_warning(LOG_ICONS, "apply_iff_preview: "
                            "datatype fallback also failed for '%s' (error=%d)\n",
                            source_path, render_result);
                result = (render_result == ITIDY_PREVIEW_NOT_SUPPORTED) 
                         ? ITIDY_PREVIEW_NOT_SUPPORTED 
                         : ITIDY_PREVIEW_FAILED;
                itidy_iff_params_free(&iff_params);
                itidy_icon_image_free(&img);
                FreeDiskObject(target_icon);
                return result;
            }
            
            log_info(LOG_ICONS, "apply_iff_preview: "
                     "datatype fallback succeeded for '%s'\n", source_path);
        }
        else
        {
            log_warning(LOG_ICONS, "apply_iff_preview: "
                        "render failed for '%s' (error=%d)\n",
                        source_path, render_result);
            result = ITIDY_PREVIEW_FAILED;
            itidy_iff_params_free(&iff_params);
            itidy_icon_image_free(&img);
            FreeDiskObject(target_icon);
            return result;
        }
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

        // Frameless/bevel setting for IFF thumbnails.
        // IFF images processed here are always opaque (mask=0, no alpha channel),
        // so BEVEL_AUTO and BEVEL_ALWAYS behave identically.
        // NONE         -> frameless, no bevel
        // WB_AUTO      -> Workbench frame (IFF is always opaque)
        // WB_ALWAYS    -> Workbench frame
        // BEVEL_AUTO   -> frameless, bevel applied (IFF is always opaque)
        // BEVEL_ALWAYS -> frameless, bevel applied
        {
            UWORD bmode = prefs ? prefs->deficons_thumbnail_border_mode : ITIDY_THUMB_BORDER_WB_AUTO;
            img.is_frameless = (bmode == ITIDY_THUMB_BORDER_NONE
                             || bmode == ITIDY_THUMB_BORDER_BEVEL_AUTO
                             || bmode == ITIDY_THUMB_BORDER_BEVEL_ALWAYS);
            want_bevel = (bmode == ITIDY_THUMB_BORDER_BEVEL_AUTO
                       || bmode == ITIDY_THUMB_BORDER_BEVEL_ALWAYS);
        }

        log_debug(LOG_ICONS, "apply_iff_preview: cropped to tight %ux%u, "
                 "transparency disabled, frameless=%s\n",
                 (unsigned)thumb_w, (unsigned)thumb_h,
                 img.is_frameless ? "yes" : "no");
    }

    /*--------------------------------------------------------------------*/
    /* Step 6c: Palette reduction (max colors / dithering)                 */
    /*          Reduces the colour count if user has configured a limit    */
    /*          below the current unique-colour count.  Dithering and      */
    /*          low-colour mapping are applied as configured.              */
    /*          Skipped when Ultra or Harmonised mode is active.           */
    /*--------------------------------------------------------------------*/

    if (prefs != NULL && prefs->deficons_max_icon_colors < 256
        && !prefs->deficons_ultra_mode
        && !prefs->deficons_harmonised_palette)
    {
        log_debug(LOG_ICONS, "apply_iff_preview: reducing palette to max %u colors "
                 "(dither=%u, lowcolor=%u)\n",
                 (unsigned)prefs->deficons_max_icon_colors,
                 (unsigned)prefs->deficons_dither_method,
                 (unsigned)prefs->deficons_lowcolor_mapping);

        if (!itidy_reduce_palette(&img,
                                  prefs->deficons_max_icon_colors,
                                  prefs->deficons_dither_method,
                                  prefs->deficons_lowcolor_mapping))
        {
            log_warning(LOG_ICONS, "apply_iff_preview: "
                        "palette reduction failed (non-fatal)\n");
        }
    }
    else if (prefs != NULL && prefs->deficons_harmonised_palette)
    {
        /*--------------------------------------------------------------*/
        /* Step 6c (Harmonised): Force fixed 29-colour GlowIcons palette */
        /*--------------------------------------------------------------*/
        log_debug(LOG_ICONS, "apply_iff_preview: applying GlowIcons "
                 "harmonised palette (dither=%u)\n",
                 (unsigned)prefs->deficons_dither_method);

        if (!itidy_apply_harmonised_palette(&img, prefs->deficons_dither_method))
        {
            log_warning(LOG_ICONS, "apply_iff_preview: "
                        "harmonised palette failed (non-fatal)\n");
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 6d: Ultra Quality mode (detail-preserving palette)             */
    /*          Converts indexed pixels to RGB24, generates an optimal     */
    /*          256-color palette via Median Cut, then remaps back to      */
    /*          indexed.  If max_colors < 256, a subsequent palette        */
    /*          reduction + dithering pass is applied on top.              */
    /*--------------------------------------------------------------------*/

    if (prefs != NULL && prefs->deficons_ultra_mode)
    {
        ULONG pixel_count = (ULONG)img.width * (ULONG)img.height;
        UBYTE *rgb24_buf = (UBYTE *)whd_malloc(pixel_count * 3);

        log_debug(LOG_ICONS, "apply_iff_preview: Ultra mode - optimizing "
                 "256-color palette for %ux%u thumbnail\n",
                 (unsigned)img.width, (unsigned)img.height);

        if (rgb24_buf != NULL)
        {
            struct ColorRegister *ultra_pal;
            ULONG i;

            // Convert indexed pixels to RGB24 using current palette
            for (i = 0; i < pixel_count; i++)
            {
                UBYTE idx = img.pixel_data_normal[i];
                if (idx < img.palette_size_normal)
                {
                    rgb24_buf[i * 3 + 0] = img.palette_normal[idx].red;
                    rgb24_buf[i * 3 + 1] = img.palette_normal[idx].green;
                    rgb24_buf[i * 3 + 2] = img.palette_normal[idx].blue;
                }
                else
                {
                    rgb24_buf[i * 3 + 0] = 0;
                    rgb24_buf[i * 3 + 1] = 0;
                    rgb24_buf[i * 3 + 2] = 0;
                }
            }

            // Generate optimal 256-color palette from actual pixel content
            ultra_pal = (struct ColorRegister *)whd_malloc(256 * sizeof(struct ColorRegister));
            if (ultra_pal != NULL)
            {
                UWORD ultra_pal_size = 0;

                if (itidy_ultra_generate_palette(rgb24_buf, pixel_count,
                                                  256U - BEVEL_PALETTE_RESERVED,
                                                  ultra_pal, &ultra_pal_size))
                {
                    // Remap pixels to the new optimal palette
                    itidy_ultra_remap_to_indexed(rgb24_buf, pixel_count,
                                                 ultra_pal, ultra_pal_size,
                                                 img.pixel_data_normal);

                    // Replace palette in img
                    whd_free(img.palette_normal);
                    img.palette_normal = ultra_pal;
                    img.palette_size_normal = ultra_pal_size;
                    ultra_pal = NULL;  // Ownership transferred

                    log_debug(LOG_ICONS, "apply_iff_preview: Ultra mode "
                             "produced %u-color optimized palette\n",
                             (unsigned)ultra_pal_size);
                }
                else
                {
                    log_warning(LOG_ICONS, "apply_iff_preview: "
                                "Ultra palette generation failed (non-fatal)\n");
                }

                if (ultra_pal != NULL)
                {
                    whd_free(ultra_pal);
                }
            }

            whd_free(rgb24_buf);
        }
        else
        {
            log_warning(LOG_ICONS, "apply_iff_preview: "
                        "Ultra RGB24 alloc failed (non-fatal)\n");
        }

        /* Ultra + Dithering: if user also wants fewer than 256 colors,
         * run palette reduction on top of the Ultra-optimized image.
         * This combines detail-preserving palette with dithering. */
        if (prefs->deficons_max_icon_colors < 256)
        {
            log_debug(LOG_ICONS, "apply_iff_preview: Ultra + reduction to "
                     "%u colors (dither=%u, lowcolor=%u)\n",
                     (unsigned)prefs->deficons_max_icon_colors,
                     (unsigned)prefs->deficons_dither_method,
                     (unsigned)prefs->deficons_lowcolor_mapping);

            if (!itidy_reduce_palette(&img,
                                      prefs->deficons_max_icon_colors,
                                      prefs->deficons_dither_method,
                                      prefs->deficons_lowcolor_mapping))
            {
                log_warning(LOG_ICONS, "apply_iff_preview: "
                            "Ultra + reduction failed (non-fatal)\n");
            }
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 6e: Thumbnail bevel (top-left lit, bottom-right shadowed)     */
    /*          Applied here — after all palette operations — so there    */
    /*          are always free palette slots for the blend variants.      */
    /*          Only applied when want_bevel is TRUE (set in border        */
    /*          section to TRUE only for BEVEL_AUTO and BEVEL_ALWAYS).    */
    /*--------------------------------------------------------------------*/
    if (want_bevel)
    {
        iTidy_RenderParams bevel_rp;
        bevel_rp.safe_left   = 0;
        bevel_rp.safe_top    = 0;
        bevel_rp.safe_width  = img.width;
        bevel_rp.safe_height = img.height;
        itidy_apply_thumbnail_bevel(&img, &bevel_rp);
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
        log_debug(LOG_ICONS, "apply_iff_preview: "
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
/* itidy_format_supports_transparency — per-format transparency check     */
/*========================================================================*/

/**
 * @brief Returns TRUE if the image format can carry transparency information.
 *
 * Used to:
 *  - Gate the magenta key-colour sweep (JPEG/BMP never have transparency, so
 *    the 6x6x6 palette entry #FF00FF at index 185 must NOT be treated as key).
 *  - Force border frames on opaque-format thumbnails (without transparency a
 *    borderless icon blends into the Workbench window background).
 *
 * Formats that CAN carry transparency (via PDTA_AlphaChannel or magenta key):
 *   png, gif, ilbm, acbm, other (unknown).
 * Formats that CANNOT: jpeg, bmp.
 */
static BOOL itidy_format_can_be_transparent(const char *type_token)
{
    if (type_token == NULL)
        return FALSE;
    if (Stricmp((STRPTR)type_token, (STRPTR)"jpeg") == 0) return FALSE;
    if (Stricmp((STRPTR)type_token, (STRPTR)"jpg")  == 0) return FALSE;
    if (Stricmp((STRPTR)type_token, (STRPTR)"bmp")  == 0) return FALSE;
    return TRUE;  /* png, gif, ilbm, acbm, other -- format CAN be transparent */
}

/**
 * @brief Returns TRUE if this format uses Amiga magenta (#FF00FF) as a
 *        transparency key colour, so the palette sweep should be attempted
 *        when picture.datatype does NOT report PDTA_AlphaChannel=TRUE.
 *
 * GIF transparency is reported by the GIF89a Graphic Control Extension, which
 * a modern GIF datatype surfaces via PDTA_AlphaChannel (the RGBA path).
 * If PDTA_AlphaChannel=FALSE for a GIF it means the datatype has confirmed
 * the image carries NO transparency -- running the magenta sweep would be a
 * false positive because the 6x6x6 cube always contains #FF00FF at index 185.
 *
 * PNG files exported from old Amiga tools sometimes use magenta as a key
 * rather than a proper alpha channel, so PNG keeps the sweep enabled.
 *
 * Formats that USE magenta key: png, ilbm, acbm, other (unknown).
 * Formats that do NOT:          jpeg, bmp, gif.
 */
static BOOL itidy_format_uses_magenta_key(const char *type_token)
{
    if (type_token == NULL)
        return FALSE;
    /* Formats that are always opaque -- sweep would always false-positive */
    if (Stricmp((STRPTR)type_token, (STRPTR)"jpeg") == 0) return FALSE;
    if (Stricmp((STRPTR)type_token, (STRPTR)"jpg")  == 0) return FALSE;
    if (Stricmp((STRPTR)type_token, (STRPTR)"bmp")  == 0) return FALSE;
    /* GIF: transparency only via PDTA_AlphaChannel (already handled by RGBA
     * path).  If PDTA_AlphaChannel=FALSE the datatype confirmed no transparency
     * exists -- the 6x6x6 cube's guaranteed #FF00FF would be a false positive. */
    if (Stricmp((STRPTR)type_token, (STRPTR)"gif")  == 0) return FALSE;
    return TRUE;  /* png, ilbm, acbm, other -- may use magenta key */
}

/*========================================================================*/
/* apply_picture_preview — datatype-only thumbnail pipeline               */
/*========================================================================*/

/**
 * @brief Render a picture thumbnail via picture.datatype for non-ILBM formats.
 *
 * Pipeline:
 *   1. Check the per-format enable bitmask (graceful skip if disabled)
 *   2. Load the target icon (Workbench metadata)
 *   3. Create blank pixel buffer at preferred size
 *   4. Build IFF render params (full-canvas safe area, no aspect heuristics)
 *   5. Call itidy_render_via_datatype() — DT opens file, reads RGB(A), scales
 *      If DT is not installed, NewDTObject() returns NULL → PREVIEW_NOT_APPLICABLE
 *   6. Crop pixel buffer to exact thumbnail dimensions
 *   7. Re-enable transparency if DT reported alpha (PNG, GIF etc.)
 *   8. Palette reduction / Ultra mode (shared with IFF pipeline)
 *   9. Apply, stamp ToolTypes, save
 *
 * Graceful datatype-missing handling: itidy_render_via_datatype() calls
 * NewDTObject() which returns NULL when no matching datatype is installed.
 * The function logs a warning and returns ITIDY_PREVIEW_NOT_SUPPORTED,
 * which this function translates to ITIDY_PREVIEW_NOT_APPLICABLE so the
 * file gets a standard (non-thumbnail) icon instead of an error.
 */
static int apply_picture_preview(const char *source_path,
                                 const char *type_token,
                                 ULONG source_size,
                                 const struct DateStamp *source_date,
                                 void (*progress_callback)(void *, const char *, ULONG, ULONG),
                                 void *progress_user_data)
{
    struct DiskObject *target_icon = NULL;
    struct DiskObject *clone = NULL;
    iTidy_IconImageData img;
    iTidy_IFFRenderParams iff_params;
    const LayoutPreferences *prefs;
    const char *source_name;
    UWORD icon_dim;
    ULONG format_bit;
    int render_result;
    int result = ITIDY_PREVIEW_FAILED;
    UBYTE *raw_rgb24 = NULL;      /* Pre-quantization RGB24 buffer for Ultra mode */
    BOOL want_bevel = FALSE;      /* Set by border section; controls bevel application */

    memset(&img, 0, sizeof(img));
    memset(&iff_params, 0, sizeof(iff_params));

    log_debug(LOG_ICONS, "apply_picture_preview: '%s' (type=%s)\n",
             source_path, type_token);

    prefs = GetGlobalPreferences();

    /*--------------------------------------------------------------------*/
    /* Step 1: Per-format enable check                                     */
    /*--------------------------------------------------------------------*/

    format_bit = type_token_to_format_bit(type_token);
    if (prefs != NULL &&
        (prefs->deficons_picture_formats_enabled & format_bit) == 0)
    {
        log_info(LOG_ICONS, "apply_picture_preview: format '%s' disabled "
                 "by user preference — skipping\n", type_token);
        return ITIDY_PREVIEW_NOT_APPLICABLE;
    }

    /*--------------------------------------------------------------------*/
    /* Step 2: Load target icon (Workbench metadata)                       */
    /*--------------------------------------------------------------------*/

    target_icon = GetDiskObject((STRPTR)source_path);
    if (target_icon == NULL)
    {
        log_error(LOG_ICONS, "apply_picture_preview: "
                  "cannot load target icon for '%s'\n", source_path);
        return ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 3: Create blank pixel buffer at preferred icon size            */
    /*--------------------------------------------------------------------*/

    icon_dim = itidy_get_iff_icon_dimensions(prefs);

    log_debug(LOG_ICONS, "apply_picture_preview: icon size=%ux%u\n",
             (unsigned)icon_dim, (unsigned)icon_dim);

    if (!itidy_icon_image_create_blank(icon_dim, icon_dim, 0, &img))
    {
        log_error(LOG_ICONS, "apply_picture_preview: blank image creation failed\n");
        FreeDiskObject(target_icon);
        return ITIDY_PREVIEW_FAILED;
    }

    /* Placeholder palette — 256 entries; DT path overwrites with 6x6x6 cube */
    {
        ULONG pal_entries = 256;
        img.palette_size_normal = pal_entries;
        img.palette_normal = (struct ColorRegister *)whd_malloc(
            pal_entries * sizeof(struct ColorRegister));
        if (img.palette_normal == NULL)
        {
            log_error(LOG_ICONS, "apply_picture_preview: palette alloc failed\n");
            itidy_icon_image_free(&img);
            FreeDiskObject(target_icon);
            return ITIDY_PREVIEW_FAILED;
        }
        memset(img.palette_normal, 0, pal_entries * sizeof(struct ColorRegister));
        img.palette_normal[0].red   = 0xAA;
        img.palette_normal[0].green = 0xAA;
        img.palette_normal[0].blue  = 0xAA;
    }

    /*--------------------------------------------------------------------*/
    /* Step 4: Build render parameters                                     */
    /*--------------------------------------------------------------------*/

    iff_params.progress_callback  = (iTidy_ProgressCallback)progress_callback;
    iff_params.progress_user_data = progress_user_data;
    iff_params.last_progress_ticks = 0;

    if (progress_user_data != NULL)
    {
        struct iTidyMainProgressWindow *pw =
            (struct iTidyMainProgressWindow *)progress_user_data;
        iff_params.cancel_flag = &pw->cancel_requested;
    }
    else
    {
        iff_params.cancel_flag = NULL;
    }

    iff_params.base.pixel_buffer  = img.pixel_data_normal;
    iff_params.base.buffer_width  = img.width;
    iff_params.base.buffer_height = img.height;
    iff_params.base.safe_left   = 0;
    iff_params.base.safe_top    = 0;
    iff_params.base.safe_width  = img.width;
    iff_params.base.safe_height = img.height;
    iff_params.base.bg_color_index = ITIDY_NO_BG_COLOUR;
    iff_params.allow_upscale   = prefs ? prefs->deficons_upscale_thumbnails : FALSE;
    iff_params.try_magenta_key = itidy_format_uses_magenta_key(type_token);

    /* Request raw pre-quantization RGB24 when Ultra mode is active.       */
    /* The render function will transfer ownership of its thumb_rgb24      */
    /* buffer here rather than freeing it, giving Ultra mode the true      */
    /* pixel values before 6x6x6 snapping degrades them to ~30 colours.   */
    if (prefs != NULL && prefs->deficons_ultra_mode)
    {
        iff_params.raw_rgb24_out         = &raw_rgb24;
        iff_params.raw_rgb24_pixel_count = 0;
    }

    /*--------------------------------------------------------------------*/
    /* Step 5: Render via picture.datatype                                 */
    /*         ilbm_heuristics=FALSE: suppress Amiga interlace/hires       */
    /*         aspect corrections — irrelevant for PNG/GIF/JPEG/BMP.      */
    /*                                                                     */
    /* Graceful missing-datatype handling:                                 */
    /*   NewDTObject() returns NULL when no matching datatype is installed. */
    /*   itidy_render_via_datatype() returns ITIDY_PREVIEW_NOT_SUPPORTED.  */
    /*   We map that to NOT_APPLICABLE so the file gets a standard icon.  */
    /*--------------------------------------------------------------------*/

    render_result = itidy_render_via_datatype(source_path, &iff_params, &img,
                                              FALSE /* ilbm_heuristics */);
    if (render_result != 0)
    {
        if (render_result == ITIDY_PREVIEW_NOT_SUPPORTED)
        {
            log_warning(LOG_ICONS, "apply_picture_preview: "
                        "datatype not available or format unrecognised for '%s' "
                        "(type=%s) -- no thumbnail will be generated\n",
                        source_path, type_token);
            if (raw_rgb24) { whd_free(raw_rgb24); raw_rgb24 = NULL; }
            itidy_icon_image_free(&img);
            FreeDiskObject(target_icon);
            return ITIDY_PREVIEW_NOT_APPLICABLE;
        }
        log_warning(LOG_ICONS, "apply_picture_preview: "
                    "render failed for '%s' (error=%d)\n", source_path, render_result);
        if (raw_rgb24) { whd_free(raw_rgb24); raw_rgb24 = NULL; }
        itidy_icon_image_free(&img);
        FreeDiskObject(target_icon);
        return ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 6: Crop pixel buffer to exact thumbnail dimensions             */
    /*--------------------------------------------------------------------*/

    {
        UWORD thumb_w  = iff_params.output_width;
        UWORD thumb_h  = iff_params.output_height;
        UWORD src_ox   = iff_params.output_offset_x;
        UWORD src_oy   = iff_params.output_offset_y;
        ULONG tight_sz = (ULONG)thumb_w * (ULONG)thumb_h;
        UBYTE *tight_buf;
        UWORD row;

        tight_buf = (UBYTE *)whd_malloc(tight_sz);
        if (tight_buf == NULL)
        {
            log_error(LOG_ICONS, "apply_picture_preview: crop alloc failed\n");
            if (raw_rgb24) { whd_free(raw_rgb24); raw_rgb24 = NULL; }
            itidy_icon_image_free(&img);
            FreeDiskObject(target_icon);
            return ITIDY_PREVIEW_FAILED;
        }

        for (row = 0; row < thumb_h; row++)
        {
            ULONG src_off = (ULONG)(src_oy + row) * (ULONG)img.width + (ULONG)src_ox;
            ULONG dst_off = (ULONG)row * (ULONG)thumb_w;
            memcpy(tight_buf + dst_off, img.pixel_data_normal + src_off, thumb_w);
        }

        whd_free(img.pixel_data_normal);
        img.pixel_data_normal = tight_buf;
        img.width  = thumb_w;
        img.height = thumb_h;

        if (img.pixel_data_selected != NULL)
        {
            whd_free(img.pixel_data_selected);
            img.pixel_data_selected = NULL;
        }
        img.has_selected_image      = FALSE;
        img.has_real_selected_image = FALSE;

        /*----------------------------------------------------------------*/
        /* Step 7: Transparency + Border decisions                         */
        /*                                                                  */
        /* Transparency:                                                    */
        /*   Only enable if BOTH the datatype reported alpha/key colour    */
        /*   AND the format is capable of carrying transparency.           */
        /*   JPEG and BMP never have transparency -- their 6x6x6 palette   */
        /*   always contains #FF00FF at index 185 regardless of image      */
        /*   content, so src_has_alpha must be ignored for those formats.  */
        /*                                                                  */
        /* Borders:                                                         */
        /*   For opaque formats (JPEG, BMP) always show borders regardless  */
        /*   of the user's 'Enable borders' preference.  Without borders    */
        /*   and without transparency the icon blends into the Workbench    */
        /*   window background.                                             */
        /*----------------------------------------------------------------*/
        {
            BOOL fmt_can_be_transparent = itidy_format_can_be_transparent(type_token);

            if (iff_params.src_has_alpha && fmt_can_be_transparent)
            {
                img.transparent_color_normal   = 0;
                img.transparent_color_selected = 0;
                log_debug(LOG_ICONS,
                         "apply_picture_preview: "
                         "alpha/key transparency enabled -- transparent_color_normal=0\n");
            }
            else
            {
                img.transparent_color_normal   = -1;
                img.transparent_color_selected = -1;
            }

            {
                UWORD bmode = prefs ? prefs->deficons_thumbnail_border_mode : ITIDY_THUMB_BORDER_WB_AUTO;
                BOOL  is_bevel_mode = (bmode == ITIDY_THUMB_BORDER_BEVEL_AUTO
                                    || bmode == ITIDY_THUMB_BORDER_BEVEL_ALWAYS);

                if (!fmt_can_be_transparent)
                {
                    /* Opaque format (JPEG/BMP): no transparency to worry about.
                     * WB modes: Workbench frame.  NONE: frameless.
                     * Bevel modes: frameless (bevel IS the border). */
                    img.is_frameless = (bmode == ITIDY_THUMB_BORDER_NONE || is_bevel_mode);
                    want_bevel = is_bevel_mode;
                    log_debug(LOG_ICONS,
                             "apply_picture_preview: "
                             "opaque format -- frameless=%s, bevel=%s\n",
                             img.is_frameless ? "yes" : "no",
                             want_bevel       ? "yes" : "no");
                }
                else
                {
                    /* Format can be transparent (PNG/GIF/IFF with mask): apply style. */
                    switch (bmode)
                    {
                        case ITIDY_THUMB_BORDER_WB_ALWAYS:
                            /* WB frame always, even for transparent images */
                            img.is_frameless = FALSE;
                            break;
                        case ITIDY_THUMB_BORDER_BEVEL_AUTO:
                            /* Bevel only for opaque images — on transparent images the
                             * bevel edge pixels would merge with the desktop background */
                            img.is_frameless = TRUE;
                            want_bevel = !iff_params.src_has_alpha;
                            break;
                        case ITIDY_THUMB_BORDER_BEVEL_ALWAYS:
                            img.is_frameless = TRUE;
                            want_bevel = TRUE;
                            break;
                        case ITIDY_THUMB_BORDER_NONE:
                            img.is_frameless = TRUE;
                            break;
                        case ITIDY_THUMB_BORDER_WB_AUTO:
                        default:
                            /* Smart: frameless only when image has transparency */
                            img.is_frameless = iff_params.src_has_alpha ? TRUE : FALSE;
                            break;
                    }
                }
            }

            log_debug(LOG_ICONS, "apply_picture_preview: cropped to %ux%u, "
                     "alpha=%s, fmt_transparent=%s, frameless=%s, bevel=%s\n",
                     (unsigned)thumb_w, (unsigned)thumb_h,
                     iff_params.src_has_alpha ? "yes" : "no",
                     fmt_can_be_transparent   ? "yes" : "no",
                     img.is_frameless         ? "yes" : "no",
                     want_bevel               ? "yes" : "no");
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 8a: Palette reduction                                          */
    /*          Skipped when Ultra or Harmonised mode is active.           */
    /*--------------------------------------------------------------------*/

    if (prefs != NULL && prefs->deficons_max_icon_colors < 256
        && !prefs->deficons_ultra_mode
        && !prefs->deficons_harmonised_palette)
    {
        if (!itidy_reduce_palette(&img,
                                  prefs->deficons_max_icon_colors,
                                  prefs->deficons_dither_method,
                                  prefs->deficons_lowcolor_mapping))
        {
            log_warning(LOG_ICONS, "apply_picture_preview: "
                        "palette reduction failed (non-fatal)\n");
        }
    }
    else if (prefs != NULL && prefs->deficons_harmonised_palette)
    {
        /*--------------------------------------------------------------*/
        /* Step 8a (Harmonised): Force fixed 29-colour GlowIcons palette */
        /*--------------------------------------------------------------*/
        log_debug(LOG_ICONS, "apply_picture_preview: applying GlowIcons "
                 "harmonised palette (dither=%u)\n",
                 (unsigned)prefs->deficons_dither_method);

        if (!itidy_apply_harmonised_palette(&img, prefs->deficons_dither_method))
        {
            log_warning(LOG_ICONS, "apply_picture_preview: "
                        "harmonised palette failed (non-fatal)\n");
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 8b: Ultra Quality mode                                         */
    /*--------------------------------------------------------------------*/

    if (prefs != NULL && prefs->deficons_ultra_mode)
    {
        ULONG pixel_count = (ULONG)img.width * (ULONG)img.height;
        UBYTE *rgb24_buf  = NULL;
        BOOL  rgb24_owned = FALSE;  /* TRUE = we allocated rgb24_buf, must free it */

        log_debug(LOG_ICONS, "apply_picture_preview: Ultra mode -- %ux%u\n",
                 (unsigned)img.width, (unsigned)img.height);

        /* Prefer the pre-quantization RGB24 buffer handed off by the     */
        /* render function (params->raw_rgb24_out).  This contains the    */
        /* true area-average-downscaled pixel values BEFORE they were     */
        /* snapped to the coarse 6x6x6 grid, so Ultra mode sees hundreds  */
        /* of distinct colours instead of the ~30 grid-snapped tones.     */
        if (raw_rgb24 != NULL && iff_params.raw_rgb24_pixel_count == pixel_count)
        {
            rgb24_buf  = raw_rgb24;
            raw_rgb24  = NULL;   /* owned by rgb24_buf from here */
            rgb24_owned = TRUE;
            log_debug(LOG_ICONS, "apply_picture_preview: "
                     "using pre-quantization RGB24 for Ultra palette generation\n");
        }
        else
        {
            /* Fallback: expand from the indexed (6x6x6) palette.        */
            /* This path is kept for safety but should rarely be taken.  */
            if (raw_rgb24 != NULL)
            {
                log_warning(LOG_ICONS, "apply_picture_preview: "
                            "raw RGB24 pixel count mismatch "
                            "(%lu vs %lu) -- falling back to indexed expand\n",
                            (unsigned long)iff_params.raw_rgb24_pixel_count,
                            (unsigned long)pixel_count);
                whd_free(raw_rgb24);
                raw_rgb24 = NULL;
            }
            rgb24_buf = (UBYTE *)whd_malloc(pixel_count * 3);
            if (rgb24_buf != NULL)
            {
                ULONG i;
                rgb24_owned = TRUE;
                for (i = 0; i < pixel_count; i++)
                {
                    UBYTE idx = img.pixel_data_normal[i];
                    if (idx < img.palette_size_normal)
                    {
                        rgb24_buf[i * 3 + 0] = img.palette_normal[idx].red;
                        rgb24_buf[i * 3 + 1] = img.palette_normal[idx].green;
                        rgb24_buf[i * 3 + 2] = img.palette_normal[idx].blue;
                    }
                }
            }
        }

        if (rgb24_buf != NULL)
        {
            struct ColorRegister *ultra_pal;

            /* Generate optimal 256-color palette from actual pixel content */
            ultra_pal = (struct ColorRegister *)whd_malloc(
                256 * sizeof(struct ColorRegister));
            if (ultra_pal != NULL)
            {
                UWORD ultra_pal_size = 0;

                if (itidy_ultra_generate_palette(rgb24_buf, pixel_count,
                                                  256U - BEVEL_PALETTE_RESERVED,
                                                  ultra_pal, &ultra_pal_size))
                {
                    /* Remap pixels to the new optimal palette */
                    itidy_ultra_remap_to_indexed(rgb24_buf, pixel_count,
                                                 ultra_pal, ultra_pal_size,
                                                 img.pixel_data_normal);

                    /* Transfer palette ownership */
                    whd_free(img.palette_normal);
                    img.palette_normal      = ultra_pal;
                    img.palette_size_normal = ultra_pal_size;
                    ultra_pal = NULL;

                    /* Re-evaluate transparency after palette rebuild */
                    if (iff_params.src_has_alpha)
                        img.transparent_color_normal = 0;

                    log_debug(LOG_ICONS, "apply_picture_preview: "
                             "Ultra mode produced %u-color palette\n",
                             (unsigned)ultra_pal_size);
                }
                else
                {
                    log_warning(LOG_ICONS, "apply_picture_preview: "
                                "Ultra palette generation failed (non-fatal)\n");
                }

                if (ultra_pal != NULL)
                    whd_free(ultra_pal);
            }

            if (rgb24_owned)
                whd_free(rgb24_buf);
            rgb24_buf = NULL;
        }
        else
        {
            log_warning(LOG_ICONS, "apply_picture_preview: "
                        "Ultra RGB24 alloc failed (non-fatal)\n");
        }

        /* Ultra + Dithering: reduce further if user wants < 256 colors */
        if (prefs->deficons_max_icon_colors < 256)
        {
            if (!itidy_reduce_palette(&img,
                                      prefs->deficons_max_icon_colors,
                                      prefs->deficons_dither_method,
                                      prefs->deficons_lowcolor_mapping))
            {
                log_warning(LOG_ICONS, "apply_picture_preview: "
                            "Ultra + reduction failed (non-fatal)\n");
            }
        }
    }

    /*--------------------------------------------------------------------*/
    /* Step 8c: Thumbnail bevel                                            */
    /*          Applied after all palette operations so there are always   */
    /*          free palette slots for the blend variants.                 */
    /*          Only applied when want_bevel is TRUE (set in border        */
    /*          section to TRUE only for BEVEL_AUTO and BEVEL_ALWAYS).    */
    /*--------------------------------------------------------------------*/
    if (want_bevel)
    {
        iTidy_RenderParams bevel_rp;
        bevel_rp.safe_left   = 0;
        bevel_rp.safe_top    = 0;
        bevel_rp.safe_width  = img.width;
        bevel_rp.safe_height = img.height;
        itidy_apply_thumbnail_bevel(&img, &bevel_rp);
    }

    /*--------------------------------------------------------------------*/
    /* Step 9: Apply to DiskObject clone                                   */
    /*--------------------------------------------------------------------*/

    itidy_icon_image_dump(&img, &iff_params.base, source_path);

    clone = itidy_icon_image_apply(target_icon, &img);
    if (clone == NULL)
    {
        log_error(LOG_ICONS, "apply_picture_preview: image apply failed\n");
        if (raw_rgb24) { whd_free(raw_rgb24); raw_rgb24 = NULL; }
        itidy_iff_params_free(&iff_params);
        itidy_icon_image_free(&img);
        FreeDiskObject(target_icon);
        return ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 10: Stamp ToolTypes                                            */
    /*--------------------------------------------------------------------*/

    source_name = (const char *)FilePart((STRPTR)source_path);

    if (!itidy_stamp_created_tooltypes(clone, ITIDY_KIND_IFF_THUMBNAIL,
                                       source_name, source_size, source_date))
    {
        log_warning(LOG_ICONS, "apply_picture_preview: "
                    "ToolType stamp failed (non-fatal)\n");
    }

    /*--------------------------------------------------------------------*/
    /* Step 11: Save                                                       */
    /*--------------------------------------------------------------------*/

    if (itidy_icon_image_save(source_path, clone))
    {
        log_debug(LOG_ICONS, "apply_picture_preview: saved thumbnail for '%s'\n",
                 source_path);
        result = ITIDY_PREVIEW_APPLIED;
    }
    else
    {
        log_error(LOG_ICONS, "apply_picture_preview: save failed for '%s'\n",
                  source_path);
        result = ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 12: Cleanup                                                    */
    /*--------------------------------------------------------------------*/

    if (raw_rgb24) { whd_free(raw_rgb24); raw_rgb24 = NULL; }
    itidy_iff_params_free(&iff_params);
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
                                const struct DateStamp *source_date,
                                void (*progress_callback)(void *, const char *, ULONG, ULONG),
                                void *progress_user_data)
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

    /* Get global preferences to check if preview features are enabled */
    const LayoutPreferences *prefs = GetGlobalPreferences();

    if (itidy_is_text_preview_type(type_token))
    {
        /* Check if text preview feature is disabled by user */
        if (prefs && !prefs->deficons_enable_text_previews)
        {
            log_debug(LOG_ICONS, "Skipping text preview for '%s' (type=%s) - "
                      "text preview feature disabled by user\n",
                      source_path, type_token);
            return ITIDY_PREVIEW_NOT_APPLICABLE;
        }
        
        /* Check if this type is excluded via EXCLUDETYPE tooltype */
        if (is_excluded_from_text_preview(type_token))
        {
            log_info(LOG_ICONS, "Skipping text preview for '%s' (type=%s) - "
                     "excluded via EXCLUDETYPE tooltype in Icons/def_ascii.info\n",
                     source_path, type_token);
            return ITIDY_PREVIEW_NOT_APPLICABLE;
        }
        
        // Fall through to text preview pipeline below
    }
    else if (itidy_is_iff_preview_type(type_token))
    {
        /* Check if picture preview feature is disabled by user */
        if (prefs && !prefs->deficons_enable_picture_previews)
        {
            log_debug(LOG_ICONS, "Skipping picture preview for '%s' (type=%s) - "
                      "picture preview feature disabled by user\n",
                      source_path, type_token);
            return ITIDY_PREVIEW_NOT_APPLICABLE;
        }

        /* Check per-format ILBM bitmask */
        if (prefs && !(prefs->deficons_picture_formats_enabled & ITIDY_PICTFMT_ILBM))
        {
            log_debug(LOG_ICONS, "Skipping IFF picture preview for '%s' "
                      "(ILBM format disabled by user)\n", source_path);
            return ITIDY_PREVIEW_NOT_APPLICABLE;
        }

        return apply_iff_preview(source_path, type_token,
                                 source_size, source_date,
                                 progress_callback, progress_user_data);
    }
    else if (itidy_is_picture_preview_type(type_token))
    {
        /* Check if picture preview feature is disabled by user */
        if (prefs && !prefs->deficons_enable_picture_previews)
        {
            log_debug(LOG_ICONS, "Skipping picture preview for '%s' (type=%s) - "
                      "picture preview feature disabled by user\n",
                      source_path, type_token);
            return ITIDY_PREVIEW_NOT_APPLICABLE;
        }

        return apply_picture_preview(source_path, type_token,
                                     source_size, source_date,
                                     progress_callback, progress_user_data);
    }
    else
    {
        return ITIDY_PREVIEW_NOT_APPLICABLE;
    }

    log_debug(LOG_ICONS, "itidy_apply_content_preview: applying text preview "
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
    /* Step 2: Resolve and load the IMAGE TEMPLATE (separate from target  */
    /*         icon). Template selection follows a two-level fallback:    */
    /*           1. PROGDIR:Icons/def_<type>  (type-specific)             */
    /*           2. PROGDIR:Icons/def_ascii   (generic ASCII fallback)    */
    /*         If neither exists, rendering is skipped and the normal     */
    /*         DefIcons icon copy is used unchanged.                      */
    /*                                                                     */
    /*         This is separate from the Workbench default — it is        */
    /*         iTidy's own palette-mapped icon designed for content       */
    /*         preview rendering.                                         */
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
        char template_path[256];
        struct TagItem get_template_tags[3];
        
        /* Resolve template path based on type_token */
        if (!itidy_resolve_preview_template(type_token, template_path,
                                            sizeof(template_path)))
        {
            /* Neither def_<type>.info nor def_ascii.info found.
             * Skip text preview — the normal DefIcons icon copy (already done
             * by the caller) will be used unchanged. This is not an error. */
            log_info(LOG_ICONS, "itidy_apply_content_preview: "
                     "no preview template available for type=%s — "
                     "using default DefIcons icon\n", type_token);
            FreeDiskObject(target_icon);
            return ITIDY_PREVIEW_NOT_APPLICABLE;
        }
        
        get_template_tags[0].ti_Tag  = ICONGETA_RemapIcon;
        get_template_tags[0].ti_Data = FALSE;
        get_template_tags[1].ti_Tag  = ICONGETA_GenerateImageMasks;
        get_template_tags[1].ti_Data = FALSE;
        get_template_tags[2].ti_Tag  = TAG_DONE;
        get_template_tags[2].ti_Data = 0;

        image_icon = GetIconTagList((STRPTR)template_path, get_template_tags);
        
        if (image_icon == NULL)
        {
            log_error(LOG_ICONS, "itidy_apply_content_preview: "
                      "cannot load image template '%s' (resolved for type=%s) — "
                      "ensure the Icons drawer exists in the program directory\n",
                      template_path, type_token);
            FreeDiskObject(target_icon);
            return ITIDY_PREVIEW_FAILED;
        }
    }

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
                  "image extract failed from resolved template (type=%s)\n",
                  type_token);
        FreeDiskObject(image_icon);
        FreeDiskObject(target_icon);
        return ITIDY_PREVIEW_FAILED;
    }

    /*--------------------------------------------------------------------*/
    /* Step 4: Build render parameters from the IMAGE TEMPLATE.           */
    /*         The template may contain ITIDY_TEXT_AREA, ITIDY_BG_COLOUR, */
    /*         ITIDY_TEXT_COLOUR and other ToolTypes for rendering.        */
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
    /* Step 4b: Render text on selected image (if template has real one)  */
    /*--------------------------------------------------------------------*/

    if (img.has_selected_image && img.has_real_selected_image &&
        img.pixel_data_selected != NULL)
    {
        iTidy_TextRenderParams selected_params;
        UBYTE *selected_darken_table = NULL;

        log_debug(LOG_ICONS, "itidy_apply_content_preview: template has real selected image, "
                 "rendering text on selected buffer for '%s'\n", source_path);

        // Copy the text_params structure
        memcpy(&selected_params, &text_params, sizeof(iTidy_TextRenderParams));

        // Point to the selected image buffer instead
        selected_params.base.pixel_buffer = img.pixel_data_selected;

        // CRITICAL: Point to the SELECTED palette, not the normal palette.
        // The selected template artwork has a different (dimmed) palette,
        // so the darken table must be built from the selected palette entries.
        // Without this, the renderer will calculate pixel indices that don't
        // exist in the selected palette, causing bright white artifacts.
        if (img.palette_selected != NULL)
        {
            selected_params.palette = img.palette_selected;
            selected_params.palette_size = img.palette_size_selected;
        }

        // CRITICAL: Allocate a NEW darken table for the selected image.
        // The selected template artwork has different pixel indices than
        // the normal artwork, so we need to rebuild the darken lookup table.
        // itidy_render_ascii_preview() will populate this table from the palette.
        if (selected_params.enable_adaptive_text && selected_params.darken_table != NULL)
        {
            selected_darken_table = (UBYTE *)whd_malloc(256);
            if (selected_darken_table == NULL)
            {
                log_warning(LOG_ICONS, "itidy_apply_content_preview: "
                            "failed to allocate darken table for selected image\n");
                // Fall back to non-adaptive rendering
                selected_params.enable_adaptive_text = FALSE;
                selected_params.darken_table = NULL;
            }
            else
            {
                selected_params.darken_table = selected_darken_table;
            }
        }

        // Render text on the selected image buffer
        // (This will paint text on top of the template's selected artwork)
        if (!itidy_render_ascii_preview(source_path, &selected_params))
        {
            log_warning(LOG_ICONS, "itidy_apply_content_preview: "
                        "selected image text render failed for '%s' (non-fatal)\n",
                        source_path);
            // Non-fatal — continue with normal image only
        }
        else
        {
            log_debug(LOG_ICONS, "itidy_apply_content_preview: "
                      "successfully rendered text on selected image for '%s'\n",
                      source_path);
        }

        // Free the selected image's darken table
        if (selected_darken_table != NULL)
        {
            whd_free(selected_darken_table);
            selected_darken_table = NULL;
        }
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
        log_debug(LOG_ICONS, "itidy_apply_content_preview: "
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
