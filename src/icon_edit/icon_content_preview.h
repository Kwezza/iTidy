/*
 * icon_content_preview.h - Content Preview Orchestration
 *
 * Phase 3 integration module — orchestrates the full pipeline of
 * loading an icon, rendering a content preview into its pixel buffer,
 * and saving the result. Called from the DefIcons creation pipeline
 * after a template .info has been copied to the target location.
 *
 * Currently supports ASCII text preview. Future renderers (IFF
 * thumbnails, font previews) are added here as new dispatch entries.
 *
 * See: docs/Default icons/Customize icons/Icon_Image_Editing_Plan.md
 *      Section 5 — Phase 3
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef ICON_CONTENT_PREVIEW_H
#define ICON_CONTENT_PREVIEW_H

#include <exec/types.h>
#include <dos/dos.h>
#include "../layout_preferences.h"

/*========================================================================*/
/* Result Codes                                                           */
/*========================================================================*/

/** Type does not support content preview — no action taken */
#define ITIDY_PREVIEW_NOT_APPLICABLE    0

/** Preview was rendered and saved successfully */
#define ITIDY_PREVIEW_APPLIED           1

/** Preview rendering failed — icon retains unmodified template image */
#define ITIDY_PREVIEW_FAILED            2

/** Existing icon is already current — skipped (cache hit) */
#define ITIDY_PREVIEW_SKIPPED_CACHED    3

/*========================================================================*/
/* Image Template Paths                                                   */
/*========================================================================*/

/**
 * Path to the generic text preview image template (ultimate fallback).
 *
 * This is iTidy's own palette-mapped color icon that provides the pixel
 * data for text preview rendering when no type-specific template exists.
 * It lives in the program's Icons drawer, separate from the Workbench
 * default icons in ENVARC:Sys/.
 *
 * The DefIcons pipeline copies the Workbench default icon (for metadata:
 * default tool, tooltypes, icon type), then an image template supplies
 * the color pixel data that gets the text preview rendered into it.
 *
 * Template resolution order:
 *   1. PROGDIR:Icons/def_<type>     (e.g., def_c, def_rexx)
 *   2. PROGDIR:Icons/def_ascii      (generic ASCII fallback)
 *   3. PROGDIR:Icons/text_template  (ultimate fallback, always exists)
 *
 * Host path:  Bin\Amiga\Icons\text_template.info
 * Amiga path: PROGDIR:Icons/text_template
 */
#define ITIDY_TEXT_TEMPLATE_FALLBACK    "PROGDIR:Icons/text_template"

/*========================================================================*/
/* IFF Thumbnail Icon Sizes                                               */
/*========================================================================*/

/**
 * IFF thumbnail pixel dimensions for each size tier.
 *
 * IFF thumbnails no longer use separate template .info files.
 * Dimensions are derived directly from the deficons_icon_size_mode
 * preference via itidy_get_iff_icon_dimensions().
 */

/** Icon size tiers (indices into the size preference chooser) */
#define ITIDY_ICON_SIZE_SMALL   0
#define ITIDY_ICON_SIZE_MEDIUM  1
#define ITIDY_ICON_SIZE_LARGE   2

/** Pixel dimensions for each size tier (square icons) */
#define ITIDY_IFF_SIZE_SMALL    48
#define ITIDY_IFF_SIZE_MEDIUM   64
#define ITIDY_IFF_SIZE_LARGE    100

/*========================================================================*/
/* Type Detection                                                         */
/*========================================================================*/

/**
 * @brief Check whether a DefIcons type token supports text preview.
 *
 * Walks the DefIcons type tree to determine if the type_token is
 * "ascii" or has "ascii" as its root category (generation 2 ancestor).
 * All children of the "ascii" category (e.g., "c", "h", "rexx",
 * "html", "python", etc.) are considered text-previewable.
 *
 * @param type_token    DefIcons type token (e.g., "ascii", "c", "mod")
 * @return TRUE if the type is text-based and supports preview
 */
BOOL itidy_is_text_preview_type(const char *type_token);

/**
 * @brief Check whether a DefIcons type token supports IFF thumbnail.
 *
 * Currently matches only "ilbm" — the standard IFF ILBM picture format.
 * Future expansion may add other picture types (acbm, etc.) or use
 * the broader "picture" category for datatype-based decoding.
 *
 * @param type_token    DefIcons type token (e.g., "ilbm", "c", "mod")
 * @return TRUE if the type supports IFF thumbnail generation
 */
BOOL itidy_is_iff_preview_type(const char *type_token);

/**
 * @brief Check whether a DefIcons type token is a non-ILBM picture
 *        format that can be decoded via a picture datatype.
 *
 * Matches: png, gif, jpeg, bmp, acbm, tiff, targa, pcx, ico,
 *          sunraster, reko, brush, PicView (and any other "picture"
 *          category child not handled by the native IFF parser).
 *
 * These formats bypass the iffparse.library native path and go
 * directly through itidy_render_via_datatype(). If the required
 * datatype is not installed, NewDTObject() returns NULL and the
 * pipeline logs a warning and returns ITIDY_PREVIEW_NOT_APPLICABLE.
 *
 * @param type_token    DefIcons type token (e.g., "png", "jpeg", "gif")
 * @return TRUE if the type should be attempted via picture.datatype
 */
BOOL itidy_is_picture_preview_type(const char *type_token);

/**
 * @brief Get IFF thumbnail pixel dimensions from size preference.
 *
 * Converts the deficons_icon_size_mode preference into actual pixel
 * dimensions:
 *   - ITIDY_ICON_SIZE_SMALL  → 48x48
 *   - ITIDY_ICON_SIZE_MEDIUM → 64x64 (default)
 *   - ITIDY_ICON_SIZE_LARGE  → 100x100
 *
 * @param prefs  Layout preferences (NULL-safe, defaults to medium)
 * @return Square icon dimension in pixels
 */
UWORD itidy_get_iff_icon_dimensions(const LayoutPreferences *prefs);

/*========================================================================*/
/* Cache Validation                                                       */
/*========================================================================*/

/**
 * @brief Check if an existing target icon's content preview is current.
 *
 * Should be called BEFORE the template copy step to avoid redundant
 * work. Checks:
 *   - Does the target .info already exist?
 *   - Does it have ITIDY_CREATED=1?
 *   - Does its ITIDY_KIND match the expected kind for this type?
 *   - Does ITIDY_SRC_SIZE match the current source file size?
 *   - Does ITIDY_SRC_DATE match the current source file datestamp?
 *
 * If all match, the icon is up-to-date and both the template copy
 * and rendering can be skipped entirely.
 *
 * @param target_path       Base path (without .info) to check
 * @param type_token        DefIcons type token (to determine expected kind)
 * @param source_size       Current source file size in bytes
 * @param source_date       Current source file datestamp
 * @return TRUE if existing icon is current (skip), FALSE otherwise
 */
BOOL itidy_content_preview_cache_valid(const char *target_path,
                                       const char *type_token,
                                       ULONG source_size,
                                       const struct DateStamp *source_date);

/*========================================================================*/
/* Content Preview Application                                            */
/*========================================================================*/

/**
 * @brief Apply a content preview to a newly-copied icon.
 *
 * This is the main Phase 3 orchestration function. It performs the
 * complete pipeline:
 *
 *   1. Check if the type supports content preview
 *   2. Load the target icon via GetIconTagList()
 *   3. Extract image data (pixels + palette)
 *   4. Build render parameters (safe area, palette indices)
 *   5. Call the appropriate renderer (currently: ASCII text)
 *   6. If template had a selected image, build inverted selected image
 *   7. Debug dump pixel buffer (if DEBUG_ICON_DUMP enabled)
 *   8. Apply modified image data back to a DiskObject clone
 *   9. Stamp ITIDY_CREATED / ITIDY_KIND / ITIDY_SRC_* ToolTypes
 *  10. Save the icon to disk (OS3.5 format)
 *  11. Free all allocated resources
 *
 * @param source_path   Full Amiga path to the source file (also the
 *                      base path for the .info icon — without .info)
 * @param type_token    DefIcons type token (e.g., "ascii")
 * @param source_size   Source file size in bytes (from FileInfoBlock)
 * @param source_date   Source file datestamp (from FileInfoBlock)
 * @param progress_callback Optional progress callback for long operations (may be NULL)
 * @param progress_user_data Opaque data passed to callback (may be NULL)
 * @return One of the ITIDY_PREVIEW_* result codes
 */
int itidy_apply_content_preview(const char *source_path,
                                const char *type_token,
                                ULONG source_size,
                                const struct DateStamp *source_date,
                                void (*progress_callback)(void *, const char *, ULONG, ULONG),
                                void *progress_user_data);

#endif /* ICON_CONTENT_PREVIEW_H */
