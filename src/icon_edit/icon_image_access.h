/*
 * icon_image_access.h - Icon Image Access Module (Reusable Core)
 *
 * Provides a clean API to extract, modify, and save icon image data
 * (pixels + palettes) from AmigaOS DiskObject icons. This module is
 * the foundation for all content-aware icon rendering features:
 *   - ASCII text previews (Phase 2)
 *   - IFF image thumbnails (future)
 *   - Font preview icons (future)
 *   - Badge overlays (future)
 *
 * All pixel/palette buffers are owned copies allocated with whd_malloc().
 * The caller is free to modify them and must free them via
 * itidy_icon_image_free().
 *
 * See: docs/Default icons/Customize icons/Icon_Image_Editing_Plan.md
 *
 * Target: AmigaOS / Workbench 3.2+ (V47+)
 * Language: C89/C99 (VBCC)
 */

#ifndef ICON_IMAGE_ACCESS_H
#define ICON_IMAGE_ACCESS_H

#include <exec/types.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <datatypes/pictureclass.h>  /* struct ColorRegister */

/*========================================================================*/
/* Compile-Time Debug Toggle                                              */
/*========================================================================*/

/**
 * Enable DEBUG_ICON_DUMP to log pixel buffer contents to LOG_ICONS after
 * rendering. Produces a hex-grid dump of the pixel buffer + palette info.
 * Guarded so it compiles to nothing when disabled (zero overhead).
 *
 * Enable during Phase 1/2 development. Disable for release builds.
 */
#define DEBUG_ICON_DUMP

/*========================================================================*/
/* Icon Source Format Constants                                           */
/*========================================================================*/

/** Standard planar Amiga icon (Workbench 1.x-2.x format) */
#define ITIDY_ICON_FORMAT_STANDARD  0

/** NewIcon format (ToolType-encoded image data) */
#define ITIDY_ICON_FORMAT_NEWICON   1

/** OS 3.5+ palette-mapped icon (IFF FORM ICON) */
#define ITIDY_ICON_FORMAT_OS35      2

/*========================================================================*/
/* iTidy_IconImageData — Extracted Icon Image Data                        */
/*========================================================================*/

/**
 * @brief Holds a complete copy of an icon's image data.
 *
 * All pixel and palette buffers are owned copies allocated with
 * whd_malloc(). The caller may freely modify them. Call
 * itidy_icon_image_free() to release all buffers.
 *
 * Pixel data is chunky (one byte per pixel = palette index).
 * Palette data is an array of ColorRegister (3 bytes: R, G, B each).
 */
typedef struct {
    /* Image dimensions */
    UWORD width;                        /* Icon image width in pixels */
    UWORD height;                       /* Icon image height in pixels */

    /* Chunky pixel buffers (one byte per pixel = palette index) */
    UBYTE *pixel_data_normal;           /* Normal state pixel buffer */
    UBYTE *pixel_data_selected;         /* Selected state pixel buffer (may be NULL) */

    /* Palette data (array of ColorRegister: R, G, B bytes) */
    struct ColorRegister *palette_normal;    /* RGB palette — normal */
    struct ColorRegister *palette_selected;  /* RGB palette — selected (may be NULL) */
    ULONG palette_size_normal;              /* Number of palette entries — normal */
    ULONG palette_size_selected;            /* Number of palette entries — selected */

    /* Transparency (palette index to treat as transparent, or -1 for opaque) */
    LONG transparent_color_normal;          /* Transparent color for normal image */
    LONG transparent_color_selected;        /* Transparent color for selected image */

    /* Gadget rendering flags (controls border/frame/highlight appearance) */
    UWORD gadget_flags;                     /* do_Gadget.Flags from DiskObject */
    LONG is_frameless;                      /* Frameless setting from ICONCTRLA_GetFrameless */

    /* Metadata (read-only, for diagnostics) */
    int source_format;                  /* ITIDY_ICON_FORMAT_STANDARD/NEWICON/OS35 */
    BOOL is_palette_mapped;             /* TRUE if icon.library reports palette-mapped */
    BOOL has_selected_image;            /* TRUE if a second image state exists */
    BOOL has_real_selected_image;       /* TRUE if selected image is real (not auto-dimmed) */
} iTidy_IconImageData;

/*========================================================================*/
/* iTidy_RenderParams — Generic Render Parameters (All Renderers)         */
/*========================================================================*/

/**
 * @brief Common parameters shared by all content renderers.
 *
 * Defines the target pixel buffer and the safe area rectangle within
 * which rendering is permitted. Also specifies the palette indices
 * for background and foreground colours.
 *
 * Each renderer type (text, IFF, font, badge) uses this as a base.
 */
typedef struct {
    UBYTE *pixel_buffer;        /* Target buffer (width x height bytes, chunky) */
    UWORD buffer_width;         /* Full buffer width (= icon width) */
    UWORD buffer_height;        /* Full buffer height (= icon height) */

    /* Safe area within the buffer where rendering is permitted */
    UWORD safe_left;            /* Left edge of safe area (pixels from left) */
    UWORD safe_top;             /* Top edge of safe area (pixels from top) */
    UWORD safe_width;           /* Width of safe area in pixels */
    UWORD safe_height;          /* Height of safe area in pixels */

    /* Exclusion area within safe area (preserves template artwork like folded corners) */
    UWORD exclude_left;         /* Left edge of exclusion area (pixels from buffer left) */
    UWORD exclude_top;          /* Top edge of exclusion area (pixels from buffer top) */
    UWORD exclude_width;        /* Width of exclusion area (0 = no exclusion) */
    UWORD exclude_height;       /* Height of exclusion area (0 = no exclusion) */

    /* Palette indices to use for rendering */
    UBYTE bg_color_index;       /* Background / whitespace palette index */
    UBYTE text_color_index;     /* Foreground / content pixels palette index */
    UBYTE mid_color_index;      /* Mid-tone / anti-alias palette index (grey) */
} iTidy_RenderParams;

/*========================================================================*/
/* iTidy_TextRenderParams — ASCII Text Renderer Extension                 */
/*========================================================================*/

/**
 * @brief Text-specific rendering parameters.
 *
 * Wraps the generic iTidy_RenderParams and adds fields specific to
 * the ASCII text-to-pixel renderer. Future renderers (IFF thumbnails,
 * font previews) will define their own extension structs wrapping
 * iTidy_RenderParams if they need extra fields.
 */
typedef struct {
    iTidy_RenderParams base;    /* Common render parameters */

    /* Text-specific options */
    UWORD char_pixel_width;     /* 1 or 2 pixels per character (0 = auto) */
    UWORD line_pixel_height;    /* Height of one text line in pixels (typically 1) */
    UWORD line_gap;             /* Vertical gap between lines in pixels (typically 1) */
    UWORD max_read_bytes;       /* Max bytes to read from source file (default 4096) */
    
    /* Adaptive text coloring */
    BOOL enable_adaptive_text;  /* Enable adaptive text color (samples background) */
    UBYTE darken_percent;       /* Percentage to darken background color (1-100, default 70) */
    UBYTE *darken_table;        /* Pre-computed lookup table [256]: bg_index -> darkened_index */
    const struct ColorRegister *palette;  /* Palette reference for adaptive mode */
    ULONG palette_size;         /* Number of palette entries */
} iTidy_TextRenderParams;

/*========================================================================*/
/* Default Values                                                         */
/*========================================================================*/

/** Default margin for safe area (pixels on each side) */
#define ITIDY_DEFAULT_MARGIN        4

/** Default character pixel width (1 = mini-page aesthetic) */
#define ITIDY_DEFAULT_CHAR_WIDTH    0   /* 0 = auto-select */

/** Default line pixel height */
#define ITIDY_DEFAULT_LINE_HEIGHT   1

/** Default line gap (vertical gap between text lines) */
#define ITIDY_DEFAULT_LINE_GAP      1

/** Default maximum bytes to read from source file */
#define ITIDY_DEFAULT_READ_BYTES    4096

/** Default darken percentage for adaptive text rendering (1-100) */
#define ITIDY_DEFAULT_DARKEN_PERCENT  70

/** Threshold for auto-promoting char width from 1 to 2 */
#define ITIDY_CHAR_WIDTH_PROMOTE_THRESHOLD  64

/** Sentinel value for "no background" — skip background fill entirely.
 *  Set ITIDY_BG_COLOR=-1 in icon ToolType to preserve template pixels. */
#define ITIDY_NO_BG_COLOR                   255

/*========================================================================*/
/* ToolType String Constants                                              */
/*========================================================================*/

#define ITIDY_TT_TEXT_AREA      "ITIDY_TEXT_AREA"
#define ITIDY_TT_EXCLUDE_AREA   "ITIDY_EXCLUDE_AREA"
#define ITIDY_TT_LINE_HEIGHT    "ITIDY_LINE_HEIGHT"
#define ITIDY_TT_LINE_GAP       "ITIDY_LINE_GAP"
#define ITIDY_TT_MAX_LINES      "ITIDY_MAX_LINES"
#define ITIDY_TT_CHAR_WIDTH     "ITIDY_CHAR_WIDTH"
#define ITIDY_TT_BG_COLOR       "ITIDY_BG_COLOR"
#define ITIDY_TT_TEXT_COLOR      "ITIDY_TEXT_COLOR"
#define ITIDY_TT_MID_COLOR       "ITIDY_MID_COLOR"
#define ITIDY_TT_ADAPTIVE_TEXT   "ITIDY_ADAPTIVE_TEXT"
#define ITIDY_TT_DARKEN_PERCENT  "ITIDY_DARKEN_PERCENT"
#define ITIDY_TT_READ_BYTES     "ITIDY_READ_BYTES"

/* Stamped ToolTypes on generated icons */
#define ITIDY_TT_CREATED        "ITIDY_CREATED"
#define ITIDY_TT_KIND           "ITIDY_KIND"
#define ITIDY_TT_SRC            "ITIDY_SRC"
#define ITIDY_TT_SRC_SIZE       "ITIDY_SRC_SIZE"
#define ITIDY_TT_SRC_DATE       "ITIDY_SRC_DATE"

/* ITIDY_KIND values */
#define ITIDY_KIND_TEXT_PREVIEW  "text_preview"
#define ITIDY_KIND_IFF_THUMBNAIL "iff_thumbnail"
#define ITIDY_KIND_FONT_PREVIEW  "font_preview"

/*========================================================================*/
/* Core Icon Image Functions                                              */
/*========================================================================*/

/**
 * @brief Extract image data (pixels + palette) from a loaded DiskObject.
 *
 * Copies pixel data and palette from the icon so the caller owns them.
 * The DiskObject may be freed after this call — extracted data is
 * independent.
 *
 * Uses IconControlA() to query palette-mapped image data. If the icon
 * is not palette-mapped, attempts LayoutIconA() conversion first.
 *
 * @param icon    Loaded DiskObject (from GetDiskObject/GetIconTagList)
 * @param out     Pointer to iTidy_IconImageData to populate
 * @return TRUE on success, FALSE on failure (out is zeroed on failure)
 */
BOOL itidy_icon_image_extract(struct DiskObject *icon, iTidy_IconImageData *out);

/**
 * @brief Write modified pixel/palette data back onto a DiskObject clone.
 *
 * Creates a clone of the original icon via DupDiskObjectA(), then
 * applies the modified pixel and palette data via IconControlA().
 * The clone is returned and must be saved with itidy_icon_image_save()
 * then freed with FreeDiskObject().
 *
 * IMPORTANT: The pixel and palette buffers in 'data' must remain valid
 * from this call through PutIconTagList() and until after FreeDiskObject()
 * on the returned clone. Only then may itidy_icon_image_free() be called.
 *
 * If data->pixel_data_selected is NULL, selected image tags are skipped.
 *
 * @param icon    Original DiskObject to clone
 * @param data    Modified image data to apply
 * @return Cloned DiskObject with new image data, or NULL on failure
 */
struct DiskObject *itidy_icon_image_apply(struct DiskObject *icon,
                                          const iTidy_IconImageData *data);

/**
 * @brief Save an icon to disk in OS3.5 palette-mapped format.
 *
 * Uses PutIconTagList() with tags to:
 *   - Drop old planar image data
 *   - Drop NewIcon ToolType image data (IM1=/IM2=)
 *   - Notify Workbench to refresh the icon display
 *
 * Preserves all non-image metadata: DefaultTool, ToolTypes, position,
 * stack size, protection bits, DrawerData, etc.
 *
 * @param path    Target file path (without .info extension)
 * @param icon    DiskObject to save (typically from itidy_icon_image_apply)
 * @return TRUE on success, FALSE on failure
 */
BOOL itidy_icon_image_save(const char *path, struct DiskObject *icon);

/**
 * @brief Free all allocated buffers inside an iTidy_IconImageData struct.
 *
 * Frees pixel_data_normal, pixel_data_selected, palette_normal, and
 * palette_selected. Zeros all pointers and sizes afterward.
 *
 * Safe to call on an already-freed or zero-initialized struct.
 *
 * @param data    Pointer to iTidy_IconImageData to clean up
 */
void itidy_icon_image_free(iTidy_IconImageData *data);

/**
 * @brief Create a blank pixel buffer filled with a background colour.
 *
 * Allocates pixel_data_normal of size w*h and fills every byte with
 * bg_index. Useful as a fallback when the template icon has no usable
 * image data.
 *
 * Does NOT allocate a selected image buffer or palette.
 *
 * @param w         Width in pixels
 * @param h         Height in pixels
 * @param bg_index  Palette index to fill with
 * @param out       Pointer to iTidy_IconImageData to populate
 * @return TRUE on success, FALSE on allocation failure
 */
BOOL itidy_icon_image_create_blank(UWORD w, UWORD h, UBYTE bg_index,
                                   iTidy_IconImageData *out);

/*========================================================================*/
/* ToolType Helper Functions                                              */
/*========================================================================*/

/**
 * @brief Parse the ITIDY_TEXT_AREA ToolType from an icon.
 *
 * Looks for "ITIDY_TEXT_AREA=x,y,w,h" in the icon's ToolTypes.
 * If not found, calculates defaults based on icon dimensions
 * (4-pixel margin on each side).
 *
 * @param icon          Loaded DiskObject with ToolTypes
 * @param icon_width    Icon image width (for default calculation)
 * @param icon_height   Icon image height (for default calculation)
 * @param safe_left     Output: left edge of safe area
 * @param safe_top      Output: top edge of safe area
 * @param safe_width    Output: width of safe area
 * @param safe_height   Output: height of safe area
 */
void itidy_parse_safe_area(struct DiskObject *icon,
                           UWORD icon_width, UWORD icon_height,
                           UWORD *safe_left, UWORD *safe_top,
                           UWORD *safe_width, UWORD *safe_height);

/**
 * @brief Build complete render parameters from icon ToolTypes + defaults.
 *
 * Combines safe area parsing, palette index resolution, and text-specific
 * ToolType reading into a single call. Fills a iTidy_TextRenderParams
 * struct ready for passing to the ASCII text renderer.
 *
 * @param icon      Loaded DiskObject with ToolTypes
 * @param img       Extracted image data (for dimensions and palette)
 * @param params    Output: fully populated text render parameters
 * @return TRUE on success, FALSE on failure
 */
BOOL itidy_get_render_params(struct DiskObject *icon,
                             const iTidy_IconImageData *img,
                             iTidy_TextRenderParams *params);

/**
 * @brief Find background and text palette indices.
 *
 * First checks the icon ToolTypes for ITIDY_BG_COLOR and ITIDY_TEXT_COLOR.
 * If not present, scans the palette for the lightest entry (→ bg) and
 * darkest entry (→ text) using luminance approximation:
 *   lum = 0.299 * R + 0.587 * G + 0.114 * B
 *
 * Uses integer-only math (fixed-point) for 68k performance.
 *
 * @param icon          Loaded DiskObject (for ToolType check)
 * @param palette       Palette array to scan
 * @param palette_size  Number of entries in palette
 * @param bg_index      Output: background colour palette index
 * @param text_index    Output: text colour palette index
 */
void itidy_resolve_palette_indices(struct DiskObject *icon,
                                   const struct ColorRegister *palette,
                                   ULONG palette_size,
                                   UBYTE *bg_index,
                                   UBYTE *text_index,
                                   UBYTE *mid_index);

/**
 * @brief Add iTidy stamp ToolTypes to a DiskObject.
 *
 * Adds/updates: ITIDY_CREATED=1, ITIDY_KIND=<kind>,
 * ITIDY_SRC=<filename>, ITIDY_SRC_SIZE=<bytes>,
 * ITIDY_SRC_DATE=<datestamp>.
 *
 * Existing user ToolTypes are preserved. New ITIDY_* entries are
 * appended or updated if already present.
 *
 * @param icon          DiskObject to stamp (modified in place via new ToolTypes array)
 * @param kind          ITIDY_KIND value (e.g. ITIDY_KIND_TEXT_PREVIEW)
 * @param source_name   Source filename (basename only)
 * @param source_size   Source file size in bytes
 * @param source_date   Source file datestamp
 * @return TRUE on success, FALSE on failure
 */
BOOL itidy_stamp_created_tooltypes(struct DiskObject *icon,
                                   const char *kind,
                                   const char *source_name,
                                   ULONG source_size,
                                   const struct DateStamp *source_date);

/**
 * @brief Check if an existing target icon is still current (cache-skip).
 *
 * Loads the target icon and checks for ITIDY_CREATED=1 and matching
 * ITIDY_KIND, ITIDY_SRC_SIZE, and ITIDY_SRC_DATE values. If all
 * match the current source file, the icon is up-to-date and can be
 * skipped.
 *
 * @param target_path   Path to existing target icon (without .info)
 * @param expected_kind Expected ITIDY_KIND value
 * @param source_size   Current source file size
 * @param source_date   Current source file datestamp
 * @return TRUE if icon is current (skip regeneration), FALSE otherwise
 */
BOOL itidy_check_cache_valid(const char *target_path,
                             const char *expected_kind,
                             ULONG source_size,
                             const struct DateStamp *source_date);

/*========================================================================*/
/* Debug Functions                                                        */
/*========================================================================*/

/**
 * @brief Dump pixel buffer and palette to LOG_ICONS for debugging.
 *
 * Only active when DEBUG_ICON_DUMP is defined. Compiles to nothing
 * otherwise (zero overhead in release builds).
 *
 * Outputs:
 *   - Image dimensions and palette entry count
 *   - Full palette as hex RGB values
 *   - Safe area rectangle
 *   - Pixel grid as hex digits (one digit per pixel for ≤16 colours,
 *     two hex digits space-separated for >16 colours)
 *   - First few lines of the source file text for cross-checking
 *
 * @param img           Extracted image data to dump
 * @param params        Render parameters (for safe area annotation)
 * @param source_path   Source file path (echo first N lines)
 */
#ifdef DEBUG_ICON_DUMP
void itidy_icon_image_dump(const iTidy_IconImageData *img,
                           const iTidy_RenderParams *params,
                           const char *source_path);
#else
#define itidy_icon_image_dump(img, params, source_path) ((void)0)
#endif

#endif /* ICON_IMAGE_ACCESS_H */
