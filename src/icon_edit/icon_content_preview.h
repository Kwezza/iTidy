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
 * Path to the built-in text preview image template.
 *
 * This is iTidy's own palette-mapped color icon that provides the pixel
 * data for text preview rendering. It lives in the program's Icons
 * drawer, separate from the Workbench default icons in ENVARC:Sys/.
 *
 * The DefIcons pipeline copies the Workbench default icon (for metadata:
 * default tool, tooltypes, icon type), then this image template supplies
 * the color pixel data that gets the text preview rendered into it.
 *
 * Host path:  Bin\Amiga\Icons\text_template.info
 * Amiga path: PROGDIR:Icons/text_template
 */
#define ITIDY_TEXT_TEMPLATE_PATH    "PROGDIR:Icons/text_template"

/**
 * IFF thumbnail template paths — one per size tier.
 *
 * Each template is a plain safe-area fill icon with ToolTypes defining
 * the render parameters (ITIDY_TEXT_AREA, ITIDY_BG_COLOR,
 * ITIDY_PALETTE_MODE). Per the Template Contract (Section 12 of the
 * IFF Thumbnail Implementation Plan), these have no custom pixel-art
 * borders — Workbench draws the icon frame.
 */
#define ITIDY_IFF_TEMPLATE_SMALL   "PROGDIR:Icons/iff_template_small"
#define ITIDY_IFF_TEMPLATE_MEDIUM  "PROGDIR:Icons/iff_template_medium"
#define ITIDY_IFF_TEMPLATE_LARGE   "PROGDIR:Icons/iff_template_large"

/*========================================================================*/
/* Icon Size Tiers                                                        */
/*========================================================================*/

/** Small template: 48x48 pixels */
#define ITIDY_ICON_SIZE_SMALL   0

/** Medium template: 64x64 pixels (default) */
#define ITIDY_ICON_SIZE_MEDIUM  1

/** Large template: 100x100 pixels */
#define ITIDY_ICON_SIZE_LARGE   2

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
 * @brief Select the IFF template path based on icon size preference.
 *
 * Returns the appropriate IFF template path constant based on the
 * current global deficons_icon_size_mode preference:
 *   - ITIDY_ICON_SIZE_SMALL  → ITIDY_IFF_TEMPLATE_SMALL  (48x48)
 *   - ITIDY_ICON_SIZE_MEDIUM → ITIDY_IFF_TEMPLATE_MEDIUM (64x64)
 *   - ITIDY_ICON_SIZE_LARGE  → ITIDY_IFF_TEMPLATE_LARGE  (100x100)
 *
 * Defaults to medium if preference is out of range.
 *
 * @return Static string pointer to the template path (do NOT free)
 */
const char *itidy_get_iff_template_path(void);

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
 * @return One of the ITIDY_PREVIEW_* result codes
 */
int itidy_apply_content_preview(const char *source_path,
                                const char *type_token,
                                ULONG source_size,
                                const struct DateStamp *source_date);

#endif /* ICON_CONTENT_PREVIEW_H */
