# IFF ILBM Thumbnail Renderer — Implementation Plan

**Date**: 2026-02-10  
**Status**: ✅ Implemented (Phases A–D complete)  
**Implemented**: 2026-02-10  
**Module**: `src/icon_edit/icon_iff_render.c/.h`  
**Depends on**: Completed ASCII text renderer (Phase 1–3 of Icon_Image_Editing_Plan.md)

---

## Implementation Status Summary

| Phase | Status | Notes |
|-------|--------|-------|
| A: Core IFF Parsing | ✅ Complete | All parsing, ByteRun1, bitplane→chunky, CAMG, EHB, HAM stub, hires inference |
| B: Scaling & Rendering | ✅ Complete | Area-average + 2×2 pre-filter, fixed-point 16.16, picture palette, hires+lace aspect |
| C: Integration | ✅ Complete | Template-free pipeline, direct dimension lookup, cache validation |
| D: Size & Palette UI | ✅ Complete | Size chooser + palette mode chooser in DefIcons settings |
| E: Polish & Future | 🔄 Partial | HAM via datatype fallback ✅, screen palette mode 🔲, other datatypes 🔲, max color count 🔲, dithering (3 methods + Auto) 🔲, grayscale/WB palette mapping 🔲, Ultra mode (256-color detail-preserving) 🔲 |

### Key Deviations From Original Plan

1. **Template icon format problem** (Section 12 deviation): The `iff_template_medium.info` file is an old-format planar icon, NOT OS 3.5 palette-mapped. `itidy_icon_image_extract()` failed on it. **Fix**: Added a fallback path in `apply_iff_preview()` that creates blank pixel/palette buffers from the template's dimensions when extraction fails. Since PICTURE palette mode replaces the entire palette and overwrites all safe-area pixels, blank buffers work correctly.

2. **Safe area abandoned for IFF thumbnails** (Section 12 deviation): The original plan used template safe areas to constrain the thumbnail. In practice, source IFF images can be any size and aspect ratio, causing the thumbnail to float inside the icon frame with wasted space. **Fix**: The safe area is overridden to `(0, 0, img.width, img.height)` — the full template dimensions — and after rendering, the pixel buffer is cropped to the exact thumbnail size. This produces tight-fitting icons that match the content.

3. **Transparency disabled**: Palette index 0 (often black) was being treated as transparent by Workbench, causing holes in dark images like Space and Waterfall. **Fix**: `transparent_color_normal` and `transparent_color_selected` are set to -1 (no transparency) after cropping.

4. **Border style — frameless -> WB checkbox -> 5-mode chooser with bevel (see Deviation 14)**: Since the thumbnail IS the icon (edge-to-edge, no border artwork), the initial behavior was to set `is_frameless` to TRUE so Workbench draws no border frame. This was then made configurable via a BOOL checkbox (`deficons_enable_thumbnail_borders`, default: TRUE) — when enabled, `is_frameless = FALSE` (Workbench draws its standard frame); when disabled, `is_frameless = TRUE` (edge-to-edge, no frame). **This checkbox approach was later superseded** by a 5-option border style chooser `deficons_thumbnail_border_mode` that adds a bevel painting mode as a third option alongside WB frame or none. The old BOOL field and its constants (`ITIDY_THUMB_BORDER_NEVER`, `ITIDY_THUMB_BORDER_AUTO`, `ITIDY_THUMB_BORDER_ALWAYS`) are retained as backward-compatible aliases to the new constants. See **Deviation 14** for the full current implementation.

5. **Selected image discarded**: Edge-to-edge thumbnails have no safe area for the bg/text index swap to operate on, so the selected image building step is skipped. The icon relies on Workbench's standard complement highlighting for selection feedback.

6. **`itidy_find_closest_palette_color()`**: Promoted from static in `icon_text_render.c` to shared in `icon_image_access.c/.h` with `itidy_` prefix, as planned.

7. **EHB implemented in Phase A**: As planned, EHB support was included early via `expand_ehb_palette()` in the core parsing phase. Standard decode works after palette expansion.

8. **`iTidy_IFFRenderParams` extended**: Four output dimension fields were added post-plan: `output_width`, `output_height`, `output_offset_x`, `output_offset_y`. These are populated by `itidy_render_iff_thumbnail()` and used by the caller to crop the pixel buffer.

9. **`itidy_icon_image_create_blank()`**: Already existed in `icon_image_access.c/.h` — used as the fallback for non-palette-mapped templates.

10. **Hires aspect ratio correction missing** (Step 9 bug): The original implementation only corrected for interlace (halving `display_height`) but never corrected for hires (halving `display_width`). Amiga hires pixels are half the width of lowres pixels, so a 672-pixel-wide hires image should display at the same physical width as 336 lowres pixels. Without this correction, hires and hires+interlace images produced horizontally squished thumbnails (e.g. Zebra-Stripes-V2.IFF at 672×446 hires+lace was producing a 64×21 thumbnail instead of ~64×42). **Fix**: Added hires width correction in Step 9 — when `is_hires` is TRUE, `display_width = bmhd.w / 2`. This is independent of the interlace height correction, so hires+lace images get both halved.

11. **Missing HIRES CAMG flag in many IFF files** (Step 6b addition): Many old Amiga IFF tools (particularly those producing EHB images) wrote incomplete CAMG chunks — setting interlace and EHB flags but omitting the HIRES flag, even for 672-pixel-wide images that are clearly hires resolution. This caused the hires width correction from item 10 to not trigger. **Fix**: Added width-based hires inference as a fallback (Step 6b). After CAMG decoding, if `is_hires` is FALSE but `bmhd.w > ITIDY_HIRES_WIDTH_THRESHOLD` (400 pixels — lowres maxes out at ~376 with extreme overscan), `is_hires` is set to TRUE and the inference is logged. The threshold constant is defined in `icon_iff_render.h`.

12. **Combined hires+interlace+EHB** (test case validation): Images like Utopia-10.IFF and Utopia-13.IFF (672×442, 6 planes, EHB, interlace, missing HIRES flag) exercise all three fixes together: EHB palette expansion (item 7), hires inference from width (item 11), and dual aspect ratio correction (item 10). After all fixes, these produce correct ~64×42 thumbnails with proper proportions.

13. **IFF template icons removed** (template elimination refactor): The three `iff_template_small/medium/large.info` files turned out to be vestigial. They were old-format planar icons — `itidy_icon_image_extract()` always failed on them, falling back to blank pixel buffers. Their ToolTypes (safe area, bg/text color, palette mode) were all overridden immediately after reading because IFF thumbnails are edge-to-edge with no safe area margins. The templates only contributed icon dimensions, which are trivially derived from the size preference constant. **Refactor**: Removed all template loading, extraction, and ToolType parsing from `apply_iff_preview()`. Icon dimensions now come directly from `itidy_get_iff_icon_dimensions()` which maps the preference to pixel sizes (48/64/100). Palette mode now comes from a new `deficons_palette_mode` field in `LayoutPreferences` with a chooser gadget in the DefIcons settings window. Removed: `itidy_get_iff_template_path()`, `itidy_get_iff_render_params()`, `ITIDY_IFF_TEMPLATE_*` constants, `ITIDY_TT_PALETTE_MODE` constant. Added: `itidy_get_iff_icon_dimensions()`, `ITIDY_IFF_SIZE_*` pixel constants, `deficons_palette_mode` preference field, palette mode chooser gadget. The `iff_template_*.info` files in `Bin/Amiga/Icons/` can now be deleted.

14. **5-mode border style chooser with bevel painting** (replaces BOOL checkbox from Deviation 4): The `deficons_enable_thumbnail_borders` BOOL field was replaced by `deficons_thumbnail_border_mode` (UWORD) with five distinct modes. A new bevel module (`src/icon_edit/Image/icon_image_bevel.c/.h`) paints a 2-pixel-wide double-ring highlight/shadow directly into the indexed pixel buffer, simulating a raised-edge effect using only 8 extra palette entries. **Constants** (in `layout_preferences.h`): `ITIDY_THUMB_BORDER_NONE = 0` (no border, frameless), `ITIDY_THUMB_BORDER_WB_AUTO = 1` (Workbench frame if opaque), `ITIDY_THUMB_BORDER_WB_ALWAYS = 2` (Workbench frame always), `ITIDY_THUMB_BORDER_BEVEL_AUTO = 3` (bevel if opaque, frameless if transparent), `ITIDY_THUMB_BORDER_BEVEL_ALWAYS = 4` (bevel always, frameless). Backward-compatible aliases kept for old code: `ITIDY_THUMB_BORDER_NEVER = 0`, `ITIDY_THUMB_BORDER_AUTO = 1`, `ITIDY_THUMB_BORDER_ALWAYS = 2`. **Default**: `DEFAULT_DEFICONS_THUMBNAIL_BORDER_MODE = ITIDY_THUMB_BORDER_BEVEL_AUTO` (Bevel Smart). **GUI**: 5-label chooser in `src/GUI/deficons/deficons_creation_window.c` with labels "None" / "Workbench (Smart)" / "Workbench (Always)" / "Bevel (Smart)" / "Bevel (Always)". **Pipeline position**: bevel fires in `icon_content_preview.c` — after Ultra palette reduction in both `apply_iff_preview` (Step 6e) and `apply_picture_preview` (Step 8c). Bevel calls were removed from `icon_iff_render.c` and `icon_datatype_render.c` because the palette was still full (256-entry cube) at that point, leaving no room for bevel entries. **Palette reservation**: Ultra mode palette target is capped at `256 - BEVEL_PALETTE_RESERVED` (= 248) so bevel always has room. `BEVEL_PALETTE_RESERVED = 8U` is exported from `icon_image_bevel.h`. **Bevel geometry**: 8-segment nested rectangle — outer+inner horizontal rows (own their corners, full width / inset-1) + outer+inner vertical columns (side spans only, rows st+1..bot1-1 / st+2..bot2-1). Top-right corner gets highlight; bottom-left gets shadow. No gaps, no double-writes at corners. **`want_bevel` flag**: a per-function boolean, set only for modes 3 or 4, replaced the earlier `!is_frameless` guard which would incorrectly activate bevel when a WB frame was also selected. For transparent-capable formats (PNG, GIF, etc.) in mode 3 (BEVEL_AUTO), bevel is suppressed when `src_has_alpha` is true (same as WB Smart suppresses the WB frame).

---

## Important Note for Implementers

**⚠️ Code Examples Are Guidance, Not Gospel**

All implementation code shown in this document (algorithms, function signatures, logic flows) is provided as **guidance** to illustrate the intended approach and goals. It is NOT rigid specification.

**If production testing reveals:**
- A better algorithm or approach to achieve the same goal
- Performance issues requiring optimization
- Edge cases not handled by the documented code
- Incorrect logic or calculations

**Then the programmer should:**
- ✅ **Adjust the code** to fix issues or improve the implementation
- ✅ **Find a better way** to reach the goals if the documented approach proves problematic
- ❌ **Do NOT try to force the documented code to work** if testing shows it's fundamentally flawed

**The goals and requirements matter** (e.g. "preserve hair strand details during downsampling", "reduce palette to N colors with optional dithering"). **The specific implementation path is flexible** — use the documented code as a starting point, then refine based on real-world testing.

**Documentation update**: When significant deviations are made, add an entry to the "Key Deviations" section above explaining what was changed and why.

---

## 1. Overview

Add an IFF ILBM image thumbnail renderer to the existing icon_edit pipeline, reusing the proven two-icon-merge architecture and `iTidy_RenderParams` infrastructure. The renderer will:

1. Parse IFF ILBM files via `iffparse.library`
2. Decode interleaved bitplanes to a chunky pixel buffer
3. Correct aspect ratio for hires and interlace images using CAMG viewport flags (with width-based hires inference fallback)
4. Area-average downscale to fit the icon template's safe area
5. Handle palette via two user-selectable modes (picture palette / screen palette)
6. Produce OS 3.5 format icons via the existing apply→stamp→save pipeline

Three icon size tiers are supported (48×48, 64×64, 100×100) controlled by a user preference chooser in the DefIcons settings GUI. Dimensions are derived directly from the preference — no separate template `.info` files are needed.

---

## 2. Architecture — How It Fits Into Existing Code

### Existing Pipeline (from ASCII text renderer):

```
Type detection → Load target icon (metadata) + Load image template (pixels)
→ Extract image data → Build render params → Call renderer → Build selected image
→ Debug dump → Apply to clone → Stamp ToolTypes → Save OS 3.5 → Cleanup
```

### IFF Thumbnail Pipeline (Template-Free):

The IFF renderer slots in as a **parallel renderer** alongside the text renderer, but with a significantly simplified setup phase — no template icons are loaded. The dispatch in `icon_content_preview.c`:

```
if (itidy_is_text_preview_type(type_token))
    → text renderer pipeline (existing)
else if (itidy_is_iff_preview_type(type_token))
    → IFF thumbnail pipeline (NEW)
else
    → ITIDY_PREVIEW_NOT_APPLICABLE
```

Everything after the renderer call (selected image, apply, stamp, save) is **shared unchanged**.

> **⚠️ IMPLEMENTATION NOTE**: The IFF pipeline is **template-free** (deviation 13). Unlike the text renderer which loads a template `.info` file for pixel data and ToolType-based rendering parameters, the IFF pipeline creates blank pixel buffers directly from dimension constants and derives all parameters from `LayoutPreferences`. The IFF pipeline also diverges after rendering: it crops the pixel buffer to the exact thumbnail size, disables transparency, sets the icon to frameless mode, and skips the selected image building step.

### Key Reused Components:

| Component | Location | Reuse |
|-----------|----------|-------|
| `iTidy_IconImageData` | `icon_image_access.h` | Holds pixel/palette data (created blank for IFF, not extracted from template) |
| `iTidy_RenderParams` | `icon_image_access.h` | Base struct for safe area, buffer, colors |
| `itidy_icon_image_create_blank()` | `icon_image_access.c` | Create blank pixel buffer at specified dimensions |
| `itidy_icon_image_apply()` | `icon_image_access.c` | Apply rendered pixels to output icon |
| `itidy_icon_image_save()` | `icon_image_access.c` | Save as OS 3.5 format |
| `itidy_icon_image_free()` | `icon_image_access.c` | Free pixel/palette memory |
| `itidy_resolve_palette_indices()` | `icon_image_access.c` | Auto-detect bg/text/mid colors |
| `itidy_stamp_created_tooltypes()` | `icon_image_access.c` | Stamp ITIDY_CREATED/KIND/SRC |
| `itidy_check_cache_valid()` | `icon_image_access.c` | Skip unchanged files |
| `itidy_icon_image_dump()` | `icon_image_access.c` | Debug hex grid output |
| `itidy_find_closest_palette_color()` | `icon_image_access.c` | Euclidean RGB palette matching (promoted from static in `icon_text_render.c`) |
| `ITIDY_KIND_IFF_THUMBNAIL` | `icon_image_access.h` | Already defined as `"iff_thumbnail"` |
| ~~`itidy_icon_image_extract()`~~ | ~~`icon_image_access.c`~~ | ~~Extract pixels from image template~~ — Not used by IFF pipeline (deviation 13) |
| ~~`build_selected_image()`~~ | ~~`icon_content_preview.c`~~ | ~~Safe-area index swap~~ — Skipped for IFF (edge-to-edge thumbnails) |

---

## 3. New Files

| File | Purpose |
|------|---------|
| `src/icon_edit/icon_iff_render.h` | Header: structs, constants, prototypes |
| `src/icon_edit/icon_iff_render.c` | Implementation: IFF parsing, decoding, scaling, rendering |
| `Bin/Amiga/Icons/iff_template_small.info` | 48×48 template icon |
| `Bin/Amiga/Icons/iff_template_medium.info` | 64×64 template icon |
| `Bin/Amiga/Icons/iff_template_large.info` | 100×100 template icon |

---

## 4. Modified Files

| File | Changes | Status |
|------|---------|--------|
| `src/icon_edit/icon_image_access.h` | Add `itidy_find_closest_palette_color()` prototype, IFF ToolType constants | ✅ Done |
| `src/icon_edit/icon_image_access.c` | Promote `find_closest_palette_color()` from `icon_text_render.c` with `itidy_` prefix | ✅ Done |
| `src/icon_edit/icon_text_render.c` | Remove static `find_closest_palette_color()`, call shared version | ✅ Done |
| `src/icon_edit/icon_content_preview.h` | ~~Add IFF template path constants~~ → Replaced with `ITIDY_IFF_SIZE_*` pixel dimension constants + `itidy_get_iff_icon_dimensions()` prototype. Removed `ITIDY_IFF_TEMPLATE_*` and `itidy_get_iff_template_path()` | ✅ Updated |
| `src/icon_edit/icon_content_preview.c` | ~~Template dispatch~~ → Template-free dispatch. `apply_iff_preview()` refactored: removed template load/extract/ToolType parse (~100 lines). `itidy_get_iff_template_path()` replaced by `itidy_get_iff_icon_dimensions()` | ✅ Updated |
| `src/icon_edit/icon_iff_render.h` | Removed `ITIDY_TT_PALETTE_MODE` constant and `itidy_get_iff_render_params()` prototype | ✅ Updated |
| `src/icon_edit/icon_iff_render.c` | Removed `itidy_get_iff_render_params()` function (~80 lines) — params now built directly in `apply_iff_preview()` | ✅ Updated |
| `src/layout_preferences.h` | Add `deficons_icon_size_mode` + `deficons_palette_mode` preference fields and defaults | ✅ Done |
| `src/layout_preferences.c` | Initialize both icon size and palette mode preferences | ✅ Done |
| `src/GUI/deficons_settings_window.c` | Add icon size chooser gadget + palette mode chooser gadget | ✅ Done |
| `Makefile` | Add `icon_iff_render.c` to `ICON_EDIT_SRCS` and dependency rule | ✅ Done |

---

## 5. Data Structures

### `iTidy_IFFRenderParams` (new — wraps `iTidy_RenderParams`)

```c
/* Palette mode for IFF thumbnail rendering */
typedef enum {
    ITIDY_PAL_PICTURE = 0,   /* Use IFF image's own CMAP palette */
    ITIDY_PAL_SCREEN  = 1    /* Quantize to Workbench screen palette (future) */
} iTidy_PaletteMode;

/* IFF-specific extension of the base render params */
typedef struct {
    iTidy_RenderParams base;             /* Embedded base (safe area, buffer, colors) */

    /* Source image properties (after IFF decode) */
    UWORD src_width;                     /* BMHD width */
    UWORD src_height;                    /* BMHD height */
    UWORD src_depth;                     /* BMHD nPlanes */
    UBYTE src_compression;               /* BMHD compression (0=none, 1=ByteRun1) */
    UBYTE src_masking;                   /* BMHD masking */

    /* CAMG viewport mode flags */
    ULONG camg_flags;                    /* Raw CAMG data (0 if no CAMG chunk) */
    BOOL is_interlace;                   /* V_LACE (0x0004) detected */
    BOOL is_hires;                       /* V_HIRES (0x8000) detected */
    BOOL is_ham;                         /* HAM (0x0800) detected */
    BOOL is_ehb;                         /* EXTRA_HALFBRITE (0x0080) detected */

    /* Corrected display dimensions (after aspect ratio adjustment) */
    UWORD display_width;                 /* = src_width */
    UWORD display_height;               /* = src_height / 2 if interlace, else src_height */

    /* Source pixel and palette data (decoded from IFF) */
    UBYTE *src_chunky;                   /* Decoded chunky pixel buffer (one byte per pixel) */
    struct ColorRegister *src_palette;   /* CMAP palette (allocated copy) */
    ULONG src_palette_size;              /* Number of CMAP entries */

    /* Rendering mode */
    iTidy_PaletteMode palette_mode;      /* PICTURE or SCREEN */

    /* Output dimensions (populated by itidy_render_iff_thumbnail) */
    /* Added during implementation — not in original plan */
    UWORD output_width;                  /* Actual thumbnail width after scaling */
    UWORD output_height;                 /* Actual thumbnail height after scaling */
    UWORD output_offset_x;              /* X pixel offset of thumbnail in buffer */
    UWORD output_offset_y;              /* Y pixel offset of thumbnail in buffer */
} iTidy_IFFRenderParams;
```

> **Implementation note**: The `output_*` fields were added to support post-render cropping. The renderer calculates the scaled thumbnail dimensions and position, stores them here, and the caller uses them to extract a tight pixel buffer.

### BMHD Structure (standard IFF ILBM — from IFF spec)

```c
/* BitMapHeader — contents of the BMHD chunk */
typedef struct {
    UWORD w;                /* Raster width in pixels */
    UWORD h;                /* Raster height in pixels */
    WORD  x;                /* X position (usually 0) */
    WORD  y;                /* Y position (usually 0) */
    UBYTE nPlanes;          /* Number of bitplanes (1-8 typically) */
    UBYTE masking;          /* 0=none, 1=hasMask, 2=hasTransparentColor, 3=lasso */
    UBYTE compression;      /* 0=none, 1=ByteRun1 */
    UBYTE pad1;             /* Unused, for alignment */
    UWORD transparentColor; /* Transparent color index */
    UBYTE xAspect;          /* Pixel aspect ratio X */
    UBYTE yAspect;          /* Pixel aspect ratio Y */
    WORD  pageWidth;        /* Source page width */
    WORD  pageHeight;       /* Source page height */
} iTidy_BMHD;
```

Note: We define our own BMHD struct (prefixed `iTidy_`) to avoid dependency on any specific IFF header. The BMHD chunk data is read directly into this structure.

---

## 6. IFF Parsing Strategy — Using iffparse.library

### Why iffparse.library (not hand-rolled parser):
- Handles chunk nesting, byte-padding, and IFF structure validation
- Proven reliable across all AmigaOS versions since V36
- `PropChunk` + `StopChunk` + `ParseIFF(IFFPARSE_SCAN)` model is simple and robust
- Only bitplane→chunky conversion and ByteRun1 decompression need custom code
- Opened at runtime via `OpenLibrary("iffparse.library", 39)` — no linker changes

### Chunk Reading Sequence:

```c
/* 1. Declare property chunks (auto-stored by iffparse) */
PropChunk(iff, ID_ILBM, ID_BMHD);     /* BitMapHeader — dimensions, depth, compression */
PropChunk(iff, ID_ILBM, ID_CMAP);     /* ColorMap — palette RGB triplets */
PropChunk(iff, ID_ILBM, ID_CAMG);     /* Commodore AMiGa — viewport mode flags */

/* 2. Stop on body data */
StopChunk(iff, ID_ILBM, ID_BODY);     /* Image body — interleaved bitplane data */

/* 3. Scan to BODY (auto-stores BMHD, CMAP, CAMG along the way) */
ParseIFF(iff, IFFPARSE_SCAN);

/* 4. Retrieve stored properties */
struct StoredProperty *bmhd_sp = FindProp(iff, ID_ILBM, ID_BMHD);
struct StoredProperty *cmap_sp = FindProp(iff, ID_ILBM, ID_CMAP);
struct StoredProperty *camg_sp = FindProp(iff, ID_ILBM, ID_CAMG);

/* 5. Read BODY data */
ReadChunkBytes(iff, body_buffer, body_size);
```

### IFF Stream Setup and Cleanup:

```c
/* Setup */
struct Library *IFFParseBase = OpenLibrary("iffparse.library", 39);
struct IFFHandle *iff = AllocIFF();
BPTR file = Open(path, MODE_OLDFILE);
iff->iff_Stream = (ULONG)file;
InitIFFasDOS(iff);
OpenIFF(iff, IFFF_READ);

/* ... parsing ... */

/* Cleanup (CRITICAL ORDER) */
CloseIFF(iff);       /* 1. Close IFF context */
Close(file);         /* 2. Close DOS file handle */
FreeIFF(iff);        /* 3. Free IFF handle */
CloseLibrary(IFFParseBase);  /* 4. Close library */
```

### IFF Chunk IDs:

```c
/* Standard ILBM chunk IDs (4-byte ASCII packed into LONG) */
#define ID_ILBM  MAKE_ID('I','L','B','M')
#define ID_BMHD  MAKE_ID('B','M','H','D')
#define ID_CMAP  MAKE_ID('C','M','A','P')
#define ID_CAMG  MAKE_ID('C','A','M','G')
#define ID_BODY  MAKE_ID('B','O','D','Y')
```

These are typically available from `<datatypes/pictureclass.h>` or can be defined locally.

---

## 7. CAMG Viewport Mode Detection

### CAMG Chunk Format:
- Chunk size: 4 bytes (always)
- Data: One ULONG containing Amiga ViewPort mode flags

### Key Viewport Mode Flags:

| Flag | Value | Meaning | Action |
|------|-------|---------|--------|
| `V_LACE` | `0x0004` | Interlace mode | Halve display height for scaling |
| `V_HIRES` | `0x8000` | Hi-res mode (640px) | Note for future aspect correction |
| `HAM` | `0x0800` | Hold-And-Modify | Skip (return NOT_SUPPORTED) |
| `EXTRA_HALFBRITE` | `0x0080` | Extra Half-Brite (64 colors) | Skip (return NOT_SUPPORTED) |

### Interlace Aspect Ratio Correction:

Interlace images have double the vertical pixels compared to their visual height. When displayed on a non-interlace screen, they appear "stretched" vertically (double height). The fix is simple:

```
display_height = is_interlace ? (src_height / 2) : src_height;
display_width  = src_width;  /* No horizontal correction for initial implementation */
```

This corrected `display_width × display_height` is used for the scaling calculation, ensuring the thumbnail has the correct visual aspect ratio.

### Low-Res Pixel Aspect Ratio (deferred):

PAL low-res (320×256) has non-square pixels (~1:1.2 ratio). This is a subtle effect and will NOT be corrected in the initial implementation. Could be addressed later using BMHD `xAspect`/`yAspect` fields if needed.

### Missing CAMG Chunk:

Some older IFF files may lack a CAMG chunk. In this case:
- `camg_flags = 0` (no mode flags)
- `is_interlace = FALSE` (assume non-interlace)
- Infer depth from BMHD `nPlanes` only
- No HAM/EHB detection possible — treat as standard palette image

---

## 8. ByteRun1 Decompression

ILBM BODY data is stored as interleaved bitplane rows. When BMHD `compression = 1`, each row is ByteRun1 compressed.

### ByteRun1 Algorithm:

```
For each byte n read from compressed data:
  if n >= 0 and n <= 127:
    Copy the next (n + 1) bytes literally
  if n >= -127 and n <= -1:
    Replicate the next byte (-n + 1) times
  if n == -128:
    No-op (skip)
```

### Decompression is per-row:

Each bitplane row is independently compressed. Row stride = `((bmhd_width + 15) / 16) * 2` bytes per plane (word-aligned). The decompressor must track bytes consumed per row to avoid buffer overruns.

### Compression Modes Supported:

| Value | Mode | Support |
|-------|------|---------|
| 0 | Uncompressed | ✅ Direct copy |
| 1 | ByteRun1 | ✅ Decompress per row |
| 2+ | Unknown/future | ❌ Return error, log warning |

---

## 9. Bitplane→Chunky Conversion

### ILBM BODY Layout (interleaved):

```
Row 0, Plane 0: [word-aligned bitplane data]
Row 0, Plane 1: [word-aligned bitplane data]
...
Row 0, Plane N-1: [word-aligned bitplane data]
Row 1, Plane 0: [word-aligned bitplane data]
...
```

### Conversion Algorithm:

For each row, after decompressing all bitplane data for that row:

```
For each pixel x in 0..width-1:
    byte_index = x / 8
    bit_index  = 7 - (x % 8)   /* MSB first */
    
    pixel_value = 0
    For each plane p in 0..nPlanes-1:
        if (plane_row[p][byte_index] & (1 << bit_index)):
            pixel_value |= (1 << p)
    
    chunky_buffer[y * width + x] = pixel_value
```

### Masking Plane:

If BMHD `masking = 1` (hasMask), there is an extra bitplane row after the image planes for each scanline. This mask plane must be read/skipped but is NOT included in the pixel value. The mask could be used for transparency detection in future.

---

## 10. Area-Average Downscaling

### Why Area-Average (not nearest-neighbor or bilinear):

- **Nearest-neighbor**: Fast but produces aliased, blocky results — terrible for small thumbnails
- **Bilinear**: Better but still loses detail at high reduction ratios
- **Area-average**: Each output pixel averages ALL source pixels that map to it — produces the best quality at high reduction ratios (e.g., 640→40 = 16:1 reduction), which is exactly our use case

### Algorithm:

```
scale_x = display_width  / safe_width   (floating-point or fixed-point ratio)
scale_y = display_height / safe_height

For each output pixel (out_x, out_y) in the safe area:
    src_x_start = out_x * scale_x
    src_x_end   = (out_x + 1) * scale_x
    src_y_start = out_y * scale_y
    src_y_end   = (out_y + 1) * scale_y
    
    Sum all source pixel RGB values in the rectangle [src_x_start..src_x_end, src_y_start..src_y_end]
    Divide by pixel count to get average RGB
    
    Find closest palette color index via find_closest_palette_color()
    Write index to pixel_buffer[safe_top + out_y][safe_left + out_x]
```

### Aspect Ratio Preservation:

The image should maintain its (corrected) aspect ratio within the safe area:

```
x_scale = (float)display_width  / safe_width;
y_scale = (float)display_height / safe_height;
scale   = max(x_scale, y_scale);   /* Fit entirely within safe area */

output_width  = display_width  / scale;
output_height = display_height / scale;

/* Center within safe area */
offset_x = (safe_width  - output_width)  / 2;
offset_y = (safe_height - output_height) / 2;
```

Remaining pixels around the image (letterbox/pillarbox bars) are filled with `bg_color_index`.

### Integer-Only Arithmetic:

Since this targets 68020, prefer fixed-point arithmetic (16.16 or similar) over floating-point:

```c
/* Fixed-point 16.16 scaling */
ULONG scale_x_fp = (display_width << 16) / output_width;
ULONG scale_y_fp = (display_height << 16) / output_height;
```

**⚠️ CRITICAL**: Use fixed-point from day one. Do not ship any float code path — VBCC soft-float on 68020 is expensive and easy to miss in testing.

### Performance: Fast Path + Quality Path

Area-average gives the best thumbnails at brutal reductions, but on a 68020 the worst-case cost can be significant (e.g., a 640×512 image scaled to 40×40 = each output pixel averages 16×13 = 208 source pixels, with RGB lookup per source pixel).

**Strategy — two internal code paths:**

| Reduction Ratio | Method | Why |
|----------------|--------|-----|
| ≤ 8:1 | **Area-average** (full quality) | Manageable pixel counts per output pixel |
| > 8:1 | **Point-sample with 2×2 pre-filter** | Pre-average into a half-size buffer first, then area-average the smaller buffer. Effectively a two-pass scale that keeps quality high while capping per-pixel work |

The threshold (8:1) is a conservative default. If the source image is small enough that the total work is trivial regardless of ratio, always use full area-average.

```c
/* Decide scaling strategy */
ULONG ratio = max(display_width / output_width, display_height / output_height);
if (ratio > 8 && (display_width * display_height) > 65536)
{
    /* Pre-filter: average 2×2 blocks into half-size buffer, then area-average */
    prefilter_2x2(src_chunky, src_width, src_height, &half_buf, &half_w, &half_h);
    area_average_scale(half_buf, half_w, half_h, output_buf, out_w, out_h, palette);
    whd_free(half_buf);
}
else
{
    /* Direct area-average — small enough to handle directly */
    area_average_scale(src_chunky, display_width, display_height, output_buf, out_w, out_h, palette);
}
```

This keeps the code simple — the area-average function is always used, but worst-case input sizes are bounded by the pre-filter step. The pre-filter itself is trivially fast (one pass, 4 pixels → 1).

---

## 11. Palette Handling

### Two Modes (User-Selectable via Template ToolType):

#### Mode 1: Picture Palette (`ITIDY_PALETTE_MODE=PICTURE`) — Initial Focus

Use the IFF image's own CMAP palette as the icon's palette. This produces the most accurate thumbnail colors but means the icon's palette may differ from Workbench's screen palette, requiring Workbench to remap when displaying.

**Process:**
1. Extract CMAP from IFF (typically 2–256 RGB triplets)
2. **Validate CMAP** (see edge cases below)
3. Replace `iTidy_IconImageData.palette_normal` with a copy of the CMAP
4. Update `palette_size_normal` to match CMAP entry count
5. Template border artwork: **Not remapped** — the icon uses default Workbench border drawing, so the template's pixel data is a plain safe-area fill only. The icon frame is drawn by Intuition/Workbench, not stored in the pixel data (see Section 12: Template Contract)
6. Scaled image pixels are written as direct CMAP palette indices
7. Selected image: Safe-area index swap as per existing `build_selected_image()`

**CMAP Validation and Edge Cases:**

The CMAP chunk can have surprises. The following must be validated before use:

| Condition | Action |
|-----------|--------|
| **Missing CMAP** (no CMAP chunk at all) | Fall back to normal DefIcons template (no thumbnail). Log warning. Return `ITIDY_PREVIEW_FAILED`. |
| **Tiny CMAP** (< 2 entries) | Same as missing — unusable for thumbnail. Fall back. |
| **CMAP size ≠ 2^depth** | Accept the CMAP as-is. Clamp palette size to `min(cmap_entries, 256)`. Pixel indices exceeding palette size map to index 0. |
| **CMAP has odd byte count** | CMAP is always RGB triplets (3 bytes each). If `sp_Size % 3 != 0`, truncate to the last complete triplet. |
| **Very large CMAP** (> 256 entries) | Clamp to 256. OS 3.5 icons support up to 256-color palettes. |
| **Depth > 8 without HAM** | Unusual but possible. Treat as 8-bit palette image, clamp to 256 colors. |

```c
/* Validate CMAP — returns FALSE if unusable */
static BOOL validate_cmap(const struct StoredProperty *cmap_sp, ULONG *out_palette_size)
{
    if (cmap_sp == NULL || cmap_sp->sp_Data == NULL || cmap_sp->sp_Size < 6)
    {
        /* Missing or tiny CMAP — need at least 2 RGB triplets */
        return FALSE;
    }
    ULONG entries = cmap_sp->sp_Size / 3;  /* Truncate incomplete triplets */
    if (entries > 256) entries = 256;       /* Clamp to OS 3.5 icon limit */
    *out_palette_size = entries;
    return TRUE;
}
```

**Advantages:**
- Best color fidelity in the thumbnail
- Simple implementation — no quantization needed
- Works correctly for all standard palette depths (2–256 colors)

**Disadvantages:**
- Workbench may need to remap/dither when displaying if screen palette differs
- Slight display performance cost vs screen-matched palette

#### Mode 2: Screen Palette (`ITIDY_PALETTE_MODE=SCREEN`) — Low Priority

Quantize the IFF image's colors to match the current Workbench screen palette. This produces icons that display instantly without remapping.

**Status:** Stubbed and marked as **low priority**. Will log "screen palette mode not yet implemented" and fall back to picture palette mode.

**Rationale for low priority:** Testing has shown that Workbench handles color reduction and remapping surprisingly well, even when the screen palette is reduced all the way down to just 4 colors. The current picture palette mode implementation is fast enough that the performance benefit of pre-quantizing to the screen palette is minimal. The complexity of implementing screen palette matching, dithering, and dynamic palette tracking does not justify the marginal improvement.

**Future implementation notes (if revisited):**
- Read Workbench screen palette via `GetRGB32()` on the screen's `ViewPort.ColorMap`
- For each source pixel's RGB value, find the closest Workbench palette entry
- May need dithering (ordered or Floyd-Steinberg) to compensate for limited palette
- Consider using the existing `expand_palette_for_adaptive()` infrastructure
- Track screen depth changes and palette modifications

#### Mode 3: Generic Palette (Fixed 6×6×6 Cube) — RECOMMENDED for Low-Depth Screens

Use a **fixed, pre-defined palette** with wide color distribution that is shared across ALL generated icons. This solves the pen scavenging problem on low-depth screens.

**Status:** 🔲 Not yet implemented (high priority for 8-bit users)

**Advantages:**

| Benefit | Impact |
|---------|--------|
| **No pen scavenging** | All icons share the same palette → Workbench allocates pens once |
| **Consistent appearance** | All icons use the same color set → visual harmony across desktop |
| **Fast icon loading** | No per-icon palette allocation or remapping overhead |
| **Predictable quality** | Known palette → users can tune for their screen depth |
| **Already implemented** | The 6×6×6 RGB cube (216 colors) + grayscale ramp is used in datatype path |

**Disadvantages:**

- Slightly lower color fidelity vs per-image palettes (but with dithering, the difference is negligible)
- Fixed palette may not suit all images (e.g., monochrome images get forced into color cube)

**Implementation:**

```c
/* In layout_preferences.h */
UWORD deficons_palette_mode;  /* 0=Picture, 1=Screen (stub), 2=Generic */
#define DEFAULT_DEFICONS_PALETTE_MODE  0  /* Picture mode (backward compatible) */

/* In deficons_settings_window.c */
static STRPTR palette_mode_labels[] = {
    "Picture (original colours)",
    "Screen (match Workbench)",     /* Stubbed */
    "Generic (fixed 216-colour cube)",
    NULL
};
```

**The Generic Palette:**

- **6×6×6 RGB color cube**: 216 colors covering the full RGB space uniformly
  - R, G, B each quantized to 6 levels: 0, 51, 102, 153, 204, 255
  - Index = `r_idx * 36 + g_idx * 6 + b_idx` (where r/g/b_idx ∈ [0..5])
  - Indices 0-215
- **Grayscale ramp**: 40 additional gray levels for smooth monochrome gradients
  - Indices 216-255
  - Already defined in `build_6x6x6_cube_palette()` (see `icon_iff_render.c` line ~1850)

**When to use Generic Palette:**

- **Low-depth screens**: 32-128 colors (pen pressure is real)
- **Icon-heavy workflows**: Folders with 50+ thumbnails
- **Consistency preference**: Users who want uniform icon appearance

**Pipeline changes:**

1. At icon creation time, replace normal palette generation with the fixed 6×6×6 cube palette
2. During area-average scaling or quantization, map colors to the cube (same as datatype path)
3. Apply dithering (if enabled) during the color mapping step
4. All icons in the folder will share identical palette → Workbench allocates pens once

**Comparison to existing modes:**

| Mode | Palette Source | Pen Allocation | Best For |
|------|----------------|----------------|----------|
| Picture (Mode 0) | Image's own CMAP | Per-icon (can exhaust pens) | High-depth screens (256 colors), quality priority |
| Screen (Mode 1) | Workbench screen palette | None (uses existing pens) | **Stubbed** — complexity not justified |
| Generic (Mode 2) | Fixed 6×6×6 cube | Once for all icons | **Low-depth screens (32-128 colors), icon-heavy folders** |

### ~~New ToolType~~ → GUI Preference (Deviation 13):

> **SUPERSEDED**: The palette mode was originally specified via a `ITIDY_PALETTE_MODE` ToolType on the template icon. Since templates have been eliminated (deviation 13), the palette mode is now stored as a `deficons_palette_mode` field in `LayoutPreferences` and controlled by a chooser gadget in the DefIcons settings window.

```c
/* In layout_preferences.h */
UWORD deficons_palette_mode;  /* 0=Picture (use image CMAP), 1=Screen (stub), 2=Generic (6x6x6 cube) */
#define DEFAULT_DEFICONS_PALETTE_MODE  0  /* Picture mode (backward compatible) */

/* In deficons_settings_window.c */
static STRPTR palette_mode_labels[] = {
    "Picture (original colours)",
    "Screen (match Workbench)",
    "Generic (fixed 216-colour cube)",
    NULL
};
```

~~Constant: `ITIDY_TT_PALETTE_MODE` = `"ITIDY_PALETTE_MODE"`~~ — Removed from `icon_iff_render.h`

---

## 12b. Maximum Icon Color Count (Palette Reduction)

**Status:** ✅ Implemented (2026-02-16)

> **Implementation Notes (2026-02-16):**
> - All 6 modules created in `src/icon_edit/palette/` exactly as proposed: `palette_quantization.c/h`, `palette_dithering.c/h`, `palette_mapping.c/h`, `palette_reduction.c/h`, `palette_grayscale.c/h`, `ultra_downsample.c/h`
> - Main entry point: `itidy_reduce_palette()` in `palette_reduction.c` — called from `icon_content_preview.c` Step 6c
> - Median Cut quantization implemented in `palette_quantization.c` with `iTidy_ColorEntry` histogram and `iTidy_ColorBox` splitting
> - Manhattan (L1) color distance used throughout (multiply-free for 68020 performance) rather than squared Euclidean
> - All arithmetic is integer-only as required (no floats)
> - Preference fields added to `LayoutPreferences`: `deficons_max_icon_colors` (UWORD), `deficons_dither_method` (UWORD), `deficons_lowcolor_mapping` (UWORD), `deficons_ultra_mode` (BOOL) — all at END of struct for binary compatibility
> - Three chooser gadgets added to DefIcons settings window: Max Colors, Dithering, Low-Color Mapping
> - Ghosting behavior: Dither chooser ghosted when max colors >= 256 or Ultra mode active; Low-color mapping chooser ghosted when max colors > 8 or Ultra mode active
> - Ultra mode integrated as last entry in Max Colors chooser ("Ultra (256 + detail boost)") per Option 1 recommendation in section 12e, rather than a separate checkbox
> - **Deviation from plan**: Chooser labels use slightly different wording than planned (e.g., "256 colors (full)" instead of "No limit (256 colours)", "Ultra (256 + detail boost)" instead of "Ultra (256 + detail-preserving)") — cosmetic only, no functional difference

### Problem

When a high-color source image (e.g. a HAM6/HAM8 picture converted to a 256-color icon via the datatype fallback path) is displayed on an 8-bit (256-color) Workbench screen, Workbench must find or allocate matching palette entries for every unique color in the icon. If the icon contains close to 256 distinct colors, this color mapping process can cause significant slowdown when loading and displaying the icon — the system spends time searching for closest-match colors across the full screen palette for each icon pixel.

This is especially noticeable on 68020-class hardware where the palette matching is CPU-intensive. A HAM image can contain thousands of unique colors which, after quantization to 256 via the 6x6x6 color cube, still produces up to 216 distinct palette entries that all need mapping against the screen palette.

### Proposed Solution

Add a user-configurable **maximum color count** option that reduces the icon's palette before saving. This caps the number of unique colors in the generated icon to a user-selected limit:

| Option | Colors | Typical Use Case |
|--------|--------|------------------|
| 4 | 4 | Extreme reduction, very low-color screens |
| 8 | 8 | Classic 8-color Amiga look, dithering essential |
| 16 | 16 | Fastest display, 4-bit screens, minimal palette pressure |
| 32 | 32 | Good compromise for low-color screens |
| 64 | 64 | Moderate reduction, decent quality |
| 128 | 128 | Light reduction, preserves most detail |
| No limit (256) | 256 | No reduction (current default behavior) |

### Implementation Notes

**⚠️ Implementation Flexibility Reminder**: All code examples in this section (quantization algorithms, dithering logic, thresholds, formulas) are guidance to illustrate the goals. If testing shows a better approach or reveals issues, adjust the implementation rather than forcing the documented code to work. The goals matter ("reduce palette", "preserve quality with dithering") — the exact code path is flexible. Document significant changes in "Key Deviations" section.

- **File Organization** (CRITICAL for maintainability):
  - **DO NOT create one monolithic source file** for all dithering/palette functions
  - **Create subfolder**: `src/icon_edit/palette/` to organize related code modules
  - **Proposed file structure**:
    - `palette/palette_quantization.c/h` — Median Cut, popularity-based quantization, color space conversions
    - `palette/palette_dithering.c/h` — Ordered (Bayer), Floyd-Steinberg error diffusion, dithering utilities
    - `palette/palette_mapping.c/h` — Color distance calculations, nearest-color matching, RGB-to-palette remapping
    - `palette/palette_reduction.c/h` — High-level pipeline coordinator (count colors, select algorithm, apply dithering)
    - `palette/palette_grayscale.c/h` — Grayscale conversion, Workbench palette (0-7) mapping, hybrid modes (see 12d)
    - `palette/ultra_downsample.c/h` — Detail-preserving downsampling algorithm (see 12e)
  - **Rationale**: Each file has a single, clear responsibility. Easier to test, debug, and extend individual algorithms without affecting others.
  - **Makefile integration**: Add `palette/*.c` to compilation rules
  - **Header includes**: Main renderer (`icon_iff_render.c`) includes only the high-level coordinator header (`palette_reduction.h`)

- **GUI**: Three primary controls in DefIcons settings window:
  1. **Chooser gadget**: "Max icon colors" with labels: `"4 colours"`, `"8 colours"`, `"16 colours"`, `"32 colours"`, `"64 colours"`, `"128 colours"`, `"No limit (256 colours)"`, optionally `"Ultra (256 + detail-preserving)"` (see section 12e)
  2. **Chooser gadget**: "Dithering" with labels: `"None"`, `"Ordered (Fast)"`, `"Floyd-Steinberg (Best)"`, `"Auto (recommended)"` (see section 12c)
  3. **Chooser gadget** (for 4-8 colors only): "Color mapping" with labels: `"Grayscale"`, `"Workbench palette (0-7)"`, `"Hybrid (grays + accents)"` (see section 12d)
- **Preference fields**: 
  - `UWORD deficons_max_icon_colors` in `LayoutPreferences` with `DEFAULT_DEFICONS_MAX_ICON_COLORS = 256`
  - `UWORD deficons_dither_method` with `DEFAULT_DEFICONS_DITHER_METHOD = 3` (Auto)
  - `UWORD deficons_lowcolor_mapping` with `DEFAULT_DEFICONS_LOWCOLOR_MAPPING = 0` (Grayscale)
  - `BOOL deficons_ultra_mode` with `DEFAULT_DEFICONS_ULTRA_MODE = FALSE` (see section 12e)
- **Order of operations** (CRITICAL):
  1. Render image to full palette (via area-average scaling)
  2. Count unique colors actually used in the pixel buffer
  3. **If count exceeds limit**: Calculate reduced palette (median-cut or popularity-based quantization)
  4. **Remap pixels to reduced palette**: WITH dithering if enabled (see 12c)
  5. Save icon with reduced palette
- **Why this order**: Dithering must know the target palette to select neighboring colors effectively. You cannot dither before knowing what colors are available in the final palette.
- **Applies to both pipelines**: Native IFF (PICTURE palette mode) and datatype fallback (6×6×6 cube)
- **Performance**: The reduction itself adds minimal overhead (single-pass pixel scan + palette rebuild). The benefit comes at display time when Workbench has far fewer colors to map
- **Fixed-point arithmetic**: All color distance calculations must use integer math only (no floats on 68020)
- **Dithering integration**: When remapping to reduced palette, pass pixel coordinates (x, y) to `find_closest_color_with_dither()` if dithering is enabled

### Rationale

On an 8-bit Workbench screen, loading a folder full of 256-color HAM-derived icons can noticeably slow the desktop as Workbench remaps each icon's palette against the screen palette. Reducing to 64 or even 32 colors typically produces visually acceptable thumbnails at a fraction of the display-time cost. The user can choose the trade-off between quality and performance based on their hardware and screen depth.

### Additional Consideration: Pen Scavenging Problem

On low-depth Workbench screens (32-128 colors), each icon with a unique palette forces Workbench to allocate shared pens. When loading a folder with many icons, Workbench quickly runs out of free pens and begins remapping/dithering icons to available colors, causing:
- Slow icon loading (pen allocation overhead)
- Visual inconsistency (earlier icons get better colors, later icons get remapped)
- Desktop slowdown (constant palette juggling)

This is particularly noticeable on 32-color screens where icons can exhaust the palette.

**Potential solutions**: 
- See Mode 3 (Generic Palette) in section 11 — use a fixed, shared palette across all icons
- See section 12d (Grayscale/Workbench Palette Mapping) — for 4-8 color icons, pre-map to stable colors

### Open Questions

- Should the option auto-adjust based on screen depth? (e.g. auto-cap at 16 on 4-bit screens)
- Should there be an "Auto" setting that picks a sensible limit based on detected screen depth?
- Quantization algorithm: median-cut (better quality) vs popularity (simpler, faster)?
- ~~Should low color counts (4, 8) auto-enable dithering by default?~~ **RESOLVED**: Auto dithering option handles this (see 12c)

---

## 12c. Dithering for Palette Reduction

**Status:** ✅ Implemented (2026-02-16)

> **Implementation Notes (2026-02-16):**
> - Bayer 4x4 ordered dithering implemented in `palette_dithering.c` via `itidy_dither_bayer_offset()` — returns signed offset (-7 to +8) exactly as planned
> - Floyd-Steinberg error diffusion implemented in `palette_dithering.c` via `itidy_floyd_steinberg_dither()` — processes entire pixel buffer in-place
> - Auto mode selection logic implemented in `palette_reduction.c`: 64+ colors -> None, 32 -> Ordered, 16 -> Ordered, 8 -> Floyd-Steinberg, 4 -> Floyd-Steinberg — matches the planned selection table
> - Nearest-color with dithering adjustment implemented in `palette_mapping.c` via `itidy_palette_find_nearest_dithered()`
> - **Deviation from plan**: Floyd-Steinberg uses `WORD` (16-bit signed) error buffers rather than full 16.16 fixed-point as documented. The error values at icon dimensions (max 100px wide) fit comfortably in 16-bit range, so the simpler approach was used. Error distribution fractions (7/16, 5/16, 3/16, 1/16) are computed via integer multiply-and-shift
> - **Deviation from plan**: Chooser labels differ slightly from plan (e.g., "Ordered (Bayer 4x4)" instead of "Ordered (Fast - 4x4 pattern)", "Error diffusion (Floyd-Steinberg)" instead of "Floyd-Steinberg (Best quality)", "Auto (based on color count)" instead of "Auto (recommended)") — cosmetic only
> - Dithering is applied during the palette reduction remapping pass (primary use case), not during initial rendering — as documented in the plan's "Integration Points" section
> - Error buffers allocated via `whd_malloc()` (tracked memory) and freed before function return

### Problem

When reducing icon color count (especially to 8-16 colors via the max color count feature in 12b), smooth gradients and subtle color transitions produce visible color banding artifacts. The quantization process maps similar colors to the same palette entry, creating "posterization" where the image looks flat and blocky instead of smooth.

This is most noticeable in:
- Sky gradients (light blue → dark blue)
- Skin tones (beige → tan → brown)
- Shadows and highlights (smooth brightness transitions)
- Natural images reduced to 16 colors or less

### Proposed Solution: User-Selectable Dithering Methods

Offer **three dithering methods** plus an **Auto** option that picks the best method based on color count. This gives users control over the quality vs speed trade-off.

**Key Insight: One-Time Cost vs Repeated Display Cost**

Icon creation is a **one-time operation**, but icon display happens **every time Workbench opens a folder**. Spending an extra 0.1-0.2 seconds during creation to produce a better-dithered icon that displays instantly (no runtime remapping) is a smart trade-off.

**Workbench 3.2 Quality Setting Benefit:**

Workbench's Quality setting controls how much effort it spends remapping/dithering icons at display time:
- **Low quality**: Fast, uses nearest-color (blocky if palette doesn't match)
- **High quality**: Slow, applies its own dithering/remapping

If the icon is **already dithered to the correct palette**, Workbench can use **Low Quality mode** (fast display) and still look good. You're pre-computing what Workbench would do at runtime.

### Dithering Method Comparison

| Method | Speed | Quality | Memory | When to Use |
|--------|-------|---------|--------|-------------|
| **None** | Instant | Poor at <16 colors | 0 bytes | 64+ colors, or smooth gradients only |
| **Ordered (Bayer)** | Very fast (~0.02s) | Good at 16-32, acceptable at 8 | 16 bytes | General purpose, 8-32 colors |
| **Floyd-Steinberg** | Moderate (~0.12s) | Excellent at all counts | ~192 bytes/row | 4-16 colors, quality priority |
| **Auto** | Varies | Best for color count | Varies | Recommended default (smart selection) |

### Method 1: None (No Dithering)

**Description:** Pure nearest-color matching with no dithering.

**Speed:** Instant (no overhead)

**Quality:**
- Excellent at 64+ colors
- Acceptable at 32 colors for images with smooth gradients
- Poor at 16 colors (visible banding)
- Unusable at 8 or 4 colors

**When to use:**
- High color counts (64+) where dithering adds no value
- Images with already-smooth color transitions
- Testing/comparison purposes

### Method 2: Ordered (Bayer 4×4 Matrix) — Fast

**Description:** Spatial dithering using a 4×4 threshold matrix. Adds position-dependent noise to break up color bands.

**Advantages:**

| Advantage | Benefit |
|-----------|----------|
| **Very fast** | One array lookup + simple math per pixel (~40 cycles) |
| **Fixed-point only** | No floating-point math — 68020-friendly |
| **Stateless** | Each pixel processed independently (no error buffers) |
| **Predictable pattern** | Repeating 4×4 grid pattern is visually stable |
| **Memory-efficient** | Only needs 16-byte lookup table |

**Disadvantages:**
- Visible grid pattern at very low color counts (4-8)
- Less effective than error diffusion for complex images

**Performance:**
- Per-pixel cost: ~40 cycles
- 64×64 icon: ~0.02 seconds on 7 MHz 68020
- Negligible overhead

**When to use:**
- 16-32 colors (sweet spot)
- 8 colors if Floyd-Steinberg is too slow
- General-purpose dithering

### Method 3: Floyd-Steinberg Error Diffusion — High Quality

**Description:** Error diffusion dithering. The quantization error at each pixel is propagated to neighboring unprocessed pixels, producing organic-looking dither patterns with no visible grid.

**Advantages:**

| Advantage | Benefit |
|-----------|----------|
| **Excellent quality** | Best results at all color counts, especially 4-16 |
| **No grid pattern** | Organic, natural-looking dither |
| **Preserves detail** | Better edge preservation than ordered dithering |
| **Industry standard** | Proven algorithm used in print/display for decades |

**Disadvantages:**
- Slower (~200 cycles per pixel vs ~40 for Bayer)
- Requires error buffers (2 rows × width × 3 channels × 2 bytes = ~384 bytes for 64px width)
- Sequential processing (must process left-to-right, top-to-bottom)
- More complex implementation

**Performance:**
- Per-pixel cost: ~200 cycles
- 64×64 icon: ~0.12 seconds on 7 MHz 68020
- Still acceptable for one-time creation

**When to use:**
- 4-8 colors (critical quality improvement)
- 16 colors (noticeable improvement)
- Quality priority, user willing to wait ~0.1s longer

**Error Diffusion Pattern:**

```
        X   7/16  
  3/16  5/16  1/16
```

Where `X` is the current pixel. Error is distributed to 4 neighbors.

### Method 4: Auto (Recommended Default)

**Description:** Automatically selects the best dithering method based on the max color count.

**Selection Logic:**

| Max Colors | Auto Selects | Rationale |
|------------|--------------|----------|
| 64+ | **None** | Enough colors for smooth gradients |
| 32 | **Ordered** | Good quality, fast |
| 16 | **Ordered** | Acceptable quality, negligible overhead |
| 8 | **Floyd-Steinberg** | Quality critical, worth the 0.1s cost |
| 4 | **Floyd-Steinberg** | Essential for usability |

**User Experience:**
- Smart default for most users
- Balances quality and speed automatically
- Can be overridden by manual selection

### Implementation Details

#### 4×4 Bayer Threshold Matrix

```c
/* 4x4 Bayer ordered dithering matrix (range 0-15) */
static const UBYTE bayer_4x4[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
};
```

#### Modified Color Matching Function

```c
/**
 * Find closest palette color with optional Bayer dithering.
 * 
 * When dithering is enabled, applies a position-dependent threshold
 * from a 4x4 Bayer matrix to break up color banding. The threshold
 * is centered around 0 (range -7 to +8) and added to the RGB values
 * before palette matching.
 */
static UBYTE find_closest_color_with_dither(
    const struct ColorRegister *palette,
    ULONG palette_size,
    UBYTE target_r, UBYTE target_g, UBYTE target_b,
    UWORD pixel_x, UWORD pixel_y,  /* For dither matrix lookup */
    BOOL enable_dither)
{
    UBYTE adj_r = target_r;
    UBYTE adj_g = target_g;
    UBYTE adj_b = target_b;
    
    if (enable_dither)
    {
        /* Get threshold from Bayer matrix (0-15) based on pixel position */
        UBYTE threshold = bayer_4x4[(pixel_y & 3) * 4 + (pixel_x & 3)];
        
        /* Center range: -7 to +8 */
        WORD dither_offset = (WORD)threshold - 7;
        
        /* Apply dither offset with clamping to 0-255 */
        if (dither_offset > 0)
        {
            WORD temp;
            temp = (WORD)adj_r + dither_offset;
            adj_r = (temp > 255) ? 255 : (UBYTE)temp;
            temp = (WORD)adj_g + dither_offset;
            adj_g = (temp > 255) ? 255 : (UBYTE)temp;
            temp = (WORD)adj_b + dither_offset;
            adj_b = (temp > 255) ? 255 : (UBYTE)temp;
        }
        else if (dither_offset < 0)
        {
            WORD abs_offset = -dither_offset;
            WORD temp;
            temp = (WORD)adj_r - abs_offset;
            adj_r = (temp < 0) ? 0 : (UBYTE)temp;
            temp = (WORD)adj_g - abs_offset;
            adj_g = (temp < 0) ? 0 : (UBYTE)temp;
            temp = (WORD)adj_b - abs_offset;
            adj_b = (temp < 0) ? 0 : (UBYTE)temp;
        }
        /* dither_offset == 0: no adjustment */
    }
    
    /* Standard Manhattan distance search with adjusted RGB values */
    /* ... existing find_closest_color_fast() logic ... */
}
```

#### Integration Points

**Order of Operations (CRITICAL):**

1. Render image → area-average scale → full palette (e.g., 256 colors)
2. If max color count < current palette size → calculate reduced palette (median-cut/popularity)
3. **Remap all pixels to reduced palette WITH dithering** (if enabled)
4. Save icon with reduced palette

**Primary Use Case: Palette Reduction Remapping (from 12b)**

When remapping pixels from a full palette to a reduced palette (e.g., 256 → 16 colors):

```c
/* For each pixel in the icon */
for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
        UBYTE old_idx = pixel_buffer[y * width + x];
        
        /* Look up RGB from original palette */
        UBYTE r = old_palette[old_idx].red;
        UBYTE g = old_palette[old_idx].green;
        UBYTE b = old_palette[old_idx].blue;
        
        /* Find closest match in reduced palette WITH dithering */
        UBYTE new_idx = find_closest_color_with_dither(
            reduced_palette, reduced_palette_size,
            r, g, b,
            x, y,  /* Pixel position for Bayer matrix */
            prefs->deficons_enable_dithering);
        
        pixel_buffer[y * width + x] = new_idx;
    }
}
```

This is where dithering has the most visual impact — when reducing from many colors to few.

**Secondary Use Case: Direct Rendering (Optional)**

Dithering can also be applied during the initial area-average scaling:

- **Location**: `area_average_scale()` function
- **When**: During the final `find_closest_color_fast()` call (line ~1268)
- **How**: Pass `out_x` and `out_y` for Bayer matrix lookup
- **Note**: Only needed when rendering directly to a low-color palette (e.g., Generic Palette mode with 4-8 colors)

**Tertiary Use Case: Datatype Path (Optional)**

For datatype fallback path (HAM, JPEG, etc.):

- **Location**: `quantize_rgb24_to_cube()` function
- **When**: During 6×6×6 cube quantization
- **Note**: The 216-color cube is dense enough that dithering provides minimal benefit unless further reduced

### Implementation Details

#### Ordered (Bayer) Implementation

**4×4 Bayer Threshold Matrix:**

```c
/* 4x4 Bayer ordered dithering matrix (range 0-15) */
static const UBYTE bayer_4x4[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
};

/* Get dither threshold for pixel position */
UBYTE threshold = bayer_4x4[(pixel_y & 3) * 4 + (pixel_x & 3)];
WORD dither_offset = (WORD)threshold - 7;  /* Center range: -7 to +8 */

/* Apply to RGB values (with clamping to 0-255) */
adjusted_r = CLAMP(target_r + dither_offset, 0, 255);
/* ... then do palette matching ... */
```

#### Floyd-Steinberg Implementation

**Error Buffers:**

```c
/* Allocate error buffers for RGB channels (2 rows × width) */
WORD *error_buf_r = whd_malloc((ULONG)width * 2 * sizeof(WORD));
WORD *error_buf_g = whd_malloc((ULONG)width * 2 * sizeof(WORD));
WORD *error_buf_b = whd_malloc((ULONG)width * 2 * sizeof(WORD));

/* Current and next row pointers */
WORD *curr_row_r = error_buf_r;
WORD *next_row_r = error_buf_r + width;
/* Swap rows at end of each scanline */
```

**Error Diffusion:**

```c
for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
        /* Add accumulated error to pixel RGB */
        WORD adj_r = CLAMP(pixel_r + curr_row_r[x], 0, 255);
        /* ... match to palette ... */
        UBYTE chosen_idx = find_closest_color(palette, adj_r, adj_g, adj_b);
        
        /* Calculate quantization error (16.16 fixed-point) */
        LONG err_r = ((LONG)adj_r - (LONG)palette[chosen_idx].red) << 16;
        
        /* Distribute error to neighbors */
        if (x + 1 < width)
            curr_row_r[x + 1] += (err_r * 7) >> 20;  /* 7/16 */
        if (x > 0)
            next_row_r[x - 1] += (err_r * 3) >> 20;  /* 3/16 */
        next_row_r[x]     += (err_r * 5) >> 20;      /* 5/16 */
        if (x + 1 < width)
            next_row_r[x + 1] += (err_r * 1) >> 20;  /* 1/16 */
    }
    /* Swap current and next row buffers */
}
```

**Fixed-Point Error Propagation:**
- Error scaled to 16.16 fixed-point to avoid rounding errors
- Fractions (7/16, 5/16, 3/16, 1/16) implemented as bit shifts
- No floating-point math — 68020-friendly

#### GUI & Preferences

**Preferences in `LayoutPreferences`:**

```c
/* In layout_preferences.h */
UWORD deficons_max_icon_colors;     /* 4, 8, 16, 32, 64, 128, 256 */
UWORD deficons_dither_method;       /* 0=None, 1=Ordered, 2=Floyd-Steinberg, 3=Auto */

#define DEFAULT_DEFICONS_MAX_ICON_COLORS 256
#define DEFAULT_DEFICONS_DITHER_METHOD   3  /* Auto */

/* Dithering method constants */
#define ITIDY_DITHER_NONE          0
#define ITIDY_DITHER_ORDERED       1
#define ITIDY_DITHER_FLOYD         2
#define ITIDY_DITHER_AUTO          3
```

**GUI Layout:**

```
DefIcons Settings Window
┌──────────────────────────────────────────────┐
│ Palette mode:   [Generic (fixed 216▼]      │
│                                              │
│ Max icon colors: [16 colours          ▼]    │
│ Dithering:      [Auto (recommended)   ▼]    │
│                  - None                       │
│                  - Ordered (Fast)             │
│                  - Floyd-Steinberg (Best)     │
│                  - Auto (recommended)         │
│ ...                                          │
└──────────────────────────────────────────────┘
```

**Chooser Labels:**

```c
/* In deficons_settings_window.c */
static STRPTR dither_method_labels[] = {
    "None (smooth gradients only)",
    "Ordered (Fast - 4x4 pattern)",
    "Floyd-Steinberg (Best quality)",
    "Auto (recommended)",
    NULL
};
```

**Behavior:**
- Dithering chooser is **ghosted/disabled** when max colors = 256 (no reduction active)
- Auto option shows tooltip explaining its selection logic
- Manual options allow user to override Auto for specific needs

**Recommended Workflow for Low-Depth Screens:**
1. Palette mode: "Generic (fixed 216-colour cube)" → no pen scavenging
2. Max icon colors: 16 or 8
3. Dithering: "Auto" (or "Floyd-Steinberg" for best quality)
4. Result: Fast-loading, good-looking icons with no Workbench overhead

### Performance Analysis

**Detailed Per-Method Breakdown (64×64 icon = 4,096 pixels):**

| Method | Cycles/Pixel | Total Cycles | 7 MHz 68020 | 14 MHz 68030 | Overhead |
|--------|--------------|--------------|-------------|--------------|----------|
| None | 0 | 0 | 0.000s | 0.000s | Baseline |
| Ordered | ~40 | 164K | 0.023s | 0.012s | Negligible |
| Floyd-Steinberg | ~200 | 819K | 0.117s | 0.058s | Noticeable but acceptable |

**Real-World Impact:**

- **None**: Instant, but poor quality at <16 colors
- **Ordered**: Adds ~0.02s per icon (imperceptible)
- **Floyd-Steinberg**: Adds ~0.1s per icon (one-time cost, worth it for 4-8 colors)

**Batch Creation:**
- 100 icons with Floyd-Steinberg: +10 seconds total
- **Still acceptable** given the one-time nature and quality improvement

**Memory Overhead:**

| Method | Memory Required |
|--------|----------------|
| None | 0 bytes |
| Ordered | 16 bytes (lookup table) |
| Floyd-Steinberg | ~384 bytes (2 rows × 64px × 3 channels × 2 bytes) |

Even Floyd-Steinberg's memory cost is trivial on systems with 1+ MB RAM.

### Visual Quality Improvement

Dithering effectiveness increases as color count decreases:

| Max Colors | Dithering Benefit |
|------------|-------------------|
| 256 | None (enough colors for smooth gradients) |
| 128 | Minimal (slight improvement in very smooth areas) |
| 64 | Moderate (helps with sky gradients, subtle shadows) |
| 32 | High (clearly reduces banding in most images) |
| 16 | **Very High** (essential — makes 16-color images look acceptable) |
| 8 | **Critical** (unusable without dithering, acceptable with it) |
| 4 | **Extreme** (barely recognizable without dithering, may need 8×8 matrix) |

### Open Questions

- ~~Should dithering auto-enable when max colors ≤ 16?~~ **RESOLVED**: Use Auto option (smart default)
- Should there be a 4th manual option for 8×8 Bayer matrix? (finer pattern, 64 bytes)
- ~~Apply dithering during initial rendering or only during palette reduction?~~ **RESOLVED**: Primary use is during palette reduction remapping (see Integration Points above)
- Add visual indicator in icon to show dithering method used? (e.g., tiny "O" for Ordered, "F" for Floyd)
- For Floyd-Steinberg: serpentine scanning (alternate row directions) for better quality?

### Future Enhancements

**8×8 Bayer Matrix Option:**

```c
/* 8x8 Bayer matrix (range 0-63) — finer dither pattern */
static const UBYTE bayer_8x8[64] = { ... };
```

Would provide smoother dither patterns with less visible grid. Memory cost is trivial (64 bytes). Could be added as:
- "Ordered 4×4 (Fast)"
- "Ordered 8×8 (Better pattern)"
- "Floyd-Steinberg (Best)"

**Serpentine Scanning:**

Alternate row scan direction (left-to-right, then right-to-left) to reduce directional bias in Floyd-Steinberg. Common refinement in production implementations.

---

## 12d. Grayscale and Workbench Palette Mapping (4-8 Colors)

**Status:** ✅ Implemented (2026-02-16)

> **Implementation Notes (2026-02-16):**
> - All three strategies implemented in `palette_grayscale.c`: Grayscale, Workbench palette (0-7), Hybrid (grays + WB accents)
> - `itidy_rgb_to_gray()` uses ITU-R BT.601 formula `(306*R + 601*G + 117*B) >> 10` — integer-only as planned
> - `itidy_grayscale_palette()` generates evenly-spaced gray levels for 4 or 8 color modes
> - `itidy_workbench_palette()` provides standard Workbench 3.x system palette (indices 0-7)
> - `itidy_hybrid_palette()` combines grayscale base with WB accent colors (blue, orange)
> - Low-color mapping is invoked by `palette_reduction.c` when `max_colors <= 8` and replaces the Median Cut quantization step
> - Chooser gadget ghosted/disabled when max colors > 8 (not applicable) — as planned
> - Preference field `deficons_lowcolor_mapping` (UWORD, default 0 = Grayscale) stored in `LayoutPreferences`
> - Chooser labels: "Grayscale", "Workbench palette", "Hybrid (grays + WB accents)" — matches plan closely

### Problem

On classic 4-color or 8-color Workbench screens, **colors 0-7 are stable system colors** that rarely change. These are the "locked" Workbench palette that most applications rely on. When creating icons with 4 or 8 colors, two strategies emerge:

1. **Grayscale conversion**: Convert image to grayscale, use 4 or 8 gray levels
2. **Workbench palette mapping**: Map to the known Workbench system colors (indices 0-7)

Both eliminate runtime color remapping by Workbench, speeding up display significantly.

### Standard Workbench 0-7 Palette

Typical Workbench 3.x system palette (may vary slightly):

| Index | Color | RGB (approx) | Usage |
|-------|-------|--------------|-------|
| 0 | Light gray | (170, 170, 170) | Window background |
| 1 | Black | (0, 0, 0) | Text, outlines |
| 2 | White | (255, 255, 255) | Highlights |
| 3 | Blue/Cyan | (102, 136, 187) | Selected items |
| 4 | Dark gray | (85, 85, 85) | Shadows |
| 5 | Mid gray | (119, 119, 119) | Window borders |
| 6 | Orange/Tan | (187, 119, 68) | Drag bars (often) |
| 7 | Light blue | (170, 170, 221) | Gadgets (often) |

**Note:** Colors 4-7 vary more across Workbench themes, but 0-3 are highly stable.

### Strategy 1: Grayscale Conversion

**When to use:**
- Monochrome or near-monochrome source images
- User prefers consistent appearance across all images
- Maximum compatibility (grayscale works everywhere)

**Implementation:**

```c
/* Convert RGB to grayscale using standard luminance formula */
UBYTE gray = (UBYTE)((306L * r + 601L * g + 117L * b) >> 10);  /* ITU-R BT.601 */

/* For 4 colors: quantize to 0, 85, 170, 255 */
UBYTE gray_4color = (gray + 42) / 85;  /* 0-3 index */

/* For 8 colors: quantize to 0, 36, 73, 109, 146, 182, 219, 255 */
UBYTE gray_8color = (gray + 18) / 36;  /* 0-7 index */
```

**Palette setup (4 grays):**

```c
static const struct ColorRegister gray_4_palette[4] = {
    {   0,   0,   0 },  /* Black */
    {  85,  85,  85 },  /* Dark gray */
    { 170, 170, 170 },  /* Light gray */
    { 255, 255, 255 }   /* White */
};
```

**Palette setup (8 grays):**

```c
static const struct ColorRegister gray_8_palette[8] = {
    {   0,   0,   0 },  /* Black */
    {  36,  36,  36 },
    {  73,  73,  73 },
    { 109, 109, 109 },
    { 146, 146, 146 },
    { 182, 182, 182 },
    { 219, 219, 219 },
    { 255, 255, 255 }   /* White */
};
```

**Advantages:**
- Simple, deterministic
- Works on any Workbench (no theme dependency)
- Clean, professional look
- Excellent with dithering (Floyd-Steinberg produces photo-quality grays)

**Disadvantages:**
- Loses color information (blue sky becomes gray)
- May look dull compared to color images

### Strategy 2: Workbench Palette Mapping

**When to use:**
- User wants to preserve some color (blues, tans)
- Targeting a specific Workbench theme
- Folder contains mixed color and monochrome images

**Implementation:**

```c
/* Pre-defined Workbench 0-7 palette (user-configurable or detected) */
static const struct ColorRegister wb_palette[8] = {
    { 170, 170, 170 },  /* 0: Light gray */
    {   0,   0,   0 },  /* 1: Black */
    { 255, 255, 255 },  /* 2: White */
    { 102, 136, 187 },  /* 3: Blue */
    {  85,  85,  85 },  /* 4: Dark gray */
    { 119, 119, 119 },  /* 5: Mid gray */
    { 187, 119,  68 },  /* 6: Orange/Tan */
    { 170, 170, 221 }   /* 7: Light blue */
};

/* For each pixel RGB, find closest match in wb_palette[0..7] */
UBYTE best_idx = find_closest_color_fast(wb_palette, 8, r, g, b);
```

**Palette Setup:**

The icon's palette is set to **exactly match** the Workbench 0-7 colors. Workbench then uses direct pen allocation (no remapping).

**Advantages:**
- Preserves some color information (blues, tans, etc.)
- Matches Workbench theme colors (visual integration)
- Fast display (no runtime remapping needed)
- Works with Workbench Quality = Low (instant display)

**Disadvantages:**
- Dependent on Workbench theme (if user changes theme, icons may look wrong)
- Only 8 colors to work with (4 for 4-color mode using indices 0-3 only)
- May produce odd color shifts (green grass → gray or tan)

### Strategy 3: Hybrid (Grayscale + WB Accent Colors)

**Best of both worlds:**

Use grayscale for most of the palette, but reserve 1-2 colors for Workbench accent colors (blue for highlights, orange for warnings).

**Example 8-color hybrid palette:**

```c
static const struct ColorRegister hybrid_8_palette[8] = {
    {   0,   0,   0 },  /* 0: Black */
    {  73,  73,  73 },  /* 1: Dark gray */
    { 146, 146, 146 },  /* 2: Mid gray */
    { 219, 219, 219 },  /* 3: Light gray */
    { 255, 255, 255 },  /* 4: White */
    { 102, 136, 187 },  /* 5: WB Blue (for UI elements) */
    { 187, 119,  68 },  /* 6: WB Orange (for warnings) */
    { 170, 170, 170 }   /* 7: WB Light gray (for borders) */
};
```

This provides good grayscale range (5 grays) plus Workbench theme integration (blues/oranges).

### GUI Option

**Add to DefIcons Settings Window:**

```
For 4-8 color icons:
  Color mapping: [Grayscale               ▼]
                 [Workbench palette (0-7)]
                 [Hybrid (grays + accents)]
```

**Preference field:**

```c
/* In layout_preferences.h */
UWORD deficons_lowcolor_mapping;  /* 0=Grayscale, 1=WB palette, 2=Hybrid */
#define DEFAULT_DEFICONS_LOWCOLOR_MAPPING 0  /* Grayscale (safe default) */
```

**Behavior:**
- Only active when max icon colors ≤ 8
- Ghosted/disabled for 16+ colors (not applicable)
- Tooltip: "Grayscale provides consistent quality. Workbench palette matches your desktop theme but may change appearance if theme changes."

### Performance Benefit

**Display-time speedup:**

By pre-mapping to known stable colors (grayscale or WB 0-7), Workbench can use **direct pen allocation** instead of:
1. Searching for closest available colors
2. Allocating new shared pens
3. Remapping/dithering at display time

**Result:** Icon display is **near-instant** even on 68000 systems with Workbench Quality = Low.

### Open Questions

- Should grayscale be forced for 4 colors? (WB palette 0-3 only gives limited options)
- Auto-detect current Workbench palette and save it to preferences?
- Allow user to customize the "Workbench palette" definition via color pickers?
- Hybrid option: which WB colors to include? User-selectable or hardcoded?

---

## 12e. Ultra Quality Mode (Detail-Preserving + Full 256-Color Palette)

**Status:** ✅ Implemented (2026-02-16)

> **Implementation Notes (2026-02-16):**
> - Detail-preserving downsampling implemented in `ultra_downsample.c` via `itidy_ultra_downsample()` — algorithm follows the plan: area-average + brightness detection + 30% boost for isolated bright pixels
> - Tuning constants match plan defaults: `ITIDY_ULTRA_DETAIL_THRESHOLD = 3` (3x brighter triggers boost), `ITIDY_ULTRA_BOOST_NUMERATOR/DENOMINATOR = 3/10` (30% blend)
> - Palette generation from RGB24 buffer: `itidy_ultra_generate_palette()` uses Median Cut quantization to produce optimal 256-color palette
> - Pixel remapping: `itidy_ultra_remap_to_indexed()` converts RGB24 to palette indices
> - **GUI: Option 1 chosen** (as recommended in plan): Ultra added as last entry in Max Colors chooser ("Ultra (256 + detail boost)") rather than a separate checkbox. This keeps the UI simpler.
> - Ultra mode pipeline in `icon_content_preview.c` Step 6d: converts indexed pixels to RGB24 via palette lookup -> Ultra downsample -> generate 256-color palette -> remap to indexed
> - **Ultra + Dithering combined workflow implemented**: When Ultra mode is active AND `max_icon_colors < 256` (user manually overrides), the pipeline runs Ultra downsample first (preserving details), then applies standard palette reduction + dithering. This matches the "Ultra + Dithering Workflow" section's recommendation.
> - **Deviation from plan**: When Ultra is selected via the chooser, `deficons_ultra_mode` is set to TRUE and `deficons_max_icon_colors` is forced to 256. The dither and lowcolor choosers are ghosted. The Ultra+Dither workflow is only triggered if the user has Ultra mode enabled in preferences AND a low color count — this scenario arises from direct preference editing rather than the GUI, since the GUI ghosts the color count chooser when Ultra is selected.
> - Item 46 (Auto mode logic — auto-detect high-frequency details) deferred as nice-to-have. The `itidy_should_use_ultra_mode()` function from the plan is not yet implemented.
> - All arithmetic is integer-only (fixed-point luminance calculation) as required

### The Problem: Lost High-Frequency Details

Traditional area-average downsampling loses **small bright/dark details** like stars, sparkles, hair strands, and fine textures. When scaling 320×256 → 64×64, each output pixel averages a 5×4 source region (20 pixels). A single-pixel bright detail surrounded by darker pixels essentially vanishes:

```
/* Example: white star (255) surrounded by 19 black pixels (0) */
Output = (255 + 0×19) / 20 = 13  /* Nearly black — star vanishes! */

/* Example: dark hair strand (50) surrounded by 19 skin-tone pixels (180) */
Output = (50 + 180×19) / 20 = 173  /* Blurred into skin tone — hair vanishes! */
```

**Real-world examples from 32-color OCS Amiga artwork:**

1. **Hair texture** (monkey face, Venus) — Individual hair strands blur into solid brown/orange blocks. Fine texture disappears.
2. **Night sky** (space scene) — Stars vanish because they're averaged with surrounding black space.
3. **Fabric texture** — Weave patterns and folds lose definition.
4. **Facial details** — Wrinkles, eyelashes, nostril edges blur away.

**This is especially problematic for classic OCS artwork** where the original 32-color palette was carefully crafted to show these fine details at native resolution. When downsampling to icon size, those details are lost entirely.

### AmigaOS Icon Color Limit: 256 Colors Maximum

**CRITICAL:** AmigaOS icons (including OS3.5+ GlowIcons/ColorIcons) are **palette-based with a maximum of 256 colors**. Even on RTG screens with 16-bit or 24-bit color depth, icons are stored as:
- An 8-bit palette (CMAP) with up to 256 RGB entries
- Pixels as 8-bit indices into that palette

**Ultra mode cannot bypass this limit.** There is no way to store true 16-bit or 24-bit RGB data in an AmigaOS icon file.

### The Solution: Detail-Preserving Downsampling + Full 256-Color Palette

**What Ultra mode does:**

1. **Uses detail-preserving downsampling** to prevent fine detail loss (see algorithm below)
2. **Uses the FULL 256-color palette** (ignores the "Max icon colors" setting — no reduction to 16/32/64)
3. **Generates optimal palette** from the downsampled RGB data (Median Cut or similar)
4. **No dithering needed** (256 colors are enough to minimize banding)

**Why 256 colors helps even when source is 32 colors:**

When downsampling a 320×256 OCS image with 32 source colors to 64×64:
- **Standard mode (32 colors):** Hair strands blend into ~3-4 averaged colors (solid blocks)
- **Ultra mode (256 colors):** Hair strands get ~20-30 intermediate colors representing subtle texture variations
- The detail-preserving algorithm **boosts strand visibility**, and the larger palette **captures the gradations** that area-averaging creates

**Example:** Hair texture in a 32-color monkey portrait:
- **Source (32 colors):** Individual brown hair strands (RGB 120, 80, 40) against orange skin (RGB 200, 140, 80)
- **Standard downsampling (32 colors):** Strands average to mid-tone (RGB 160, 110, 60) — looks like solid brown fill
- **Ultra downsampling (256 colors):** Strand boost creates variations (RGB 130-180, 85-135, 45-75) — texture visible!

**This is particularly valuable for:**
- Classic OCS/ECS artwork (32-64 colors) with fine details
- Scanned artwork and digitized photos
- Hand-painted artwork with brush strokes
- Any image where texture matters

**RTG benefit:**
- RTG screens display all 256 colors **accurately** without remapping/dithering
- On Chipset 8-bit screens, Workbench must remap the 256 colors to available screen colors (slower but still better than losing detail entirely)
- **Recommended primarily for RTG users**, but works on any system

### Detail-Preserving Downsampling Algorithm

**Core idea:** Boost the contribution of bright isolated pixels (stars) so they don't vanish in the average.

**Implementation:**

```c
/* Detail-preserving downsampling for Ultra mode */
void ultra_quality_downsample(UBYTE *src_rgb, UBYTE *dest_rgb,
                               int src_w, int src_h, int dest_w, int dest_h)
{
    int scale_x = src_w / dest_w;  /* e.g., 4 */
    int scale_y = src_h / dest_h;  /* e.g., 4 */
    int sample_count = scale_x * scale_y;  /* e.g., 16 */
    
    for (int dest_y = 0; dest_y < dest_h; dest_y++)
    {
        for (int dest_x = 0; dest_x < dest_w; dest_x++)
        {
            int src_x_start = dest_x * scale_x;
            int src_y_start = dest_y * scale_y;
            int src_x_end = src_x_start + scale_x;
            int src_y_end = src_y_start + scale_y;
            
            /* Accumulate area-average sums */
            ULONG r_sum = 0, g_sum = 0, b_sum = 0;
            UBYTE max_brightness = 0;
            UBYTE max_r = 0, max_g = 0, max_b = 0;
            
            /* Scan source region */
            for (int sy = src_y_start; sy < src_y_end; sy++)
            {
                for (int sx = src_x_start; sx < src_x_end; sx++)
                {
                    int src_offset = (sy * src_w + sx) * 3;
                    UBYTE r = src_rgb[src_offset + 0];
                    UBYTE g = src_rgb[src_offset + 1];
                    UBYTE b = src_rgb[src_offset + 2];
                    
                    /* Standard perceptual luminance (ITU-R BT.601) */
                    /* Y = 0.299*R + 0.587*G + 0.114*B */
                    /* Fixed-point: (306*R + 601*G + 117*B) >> 10 */
                    UBYTE brightness = (UBYTE)((306L * r + 601L * g + 117L * b) >> 10);
                    
                    /* Track brightest pixel (for detail preservation) */
                    if (brightness > max_brightness)
                    {
                        max_brightness = brightness;
                        max_r = r;
                        max_g = g;
                        max_b = b;
                    }
                    
                    /* Accumulate for average */
                    r_sum += r;
                    g_sum += g;
                    b_sum += b;
                }
            }
            
            /* Compute area-average color */
            UBYTE avg_r = (UBYTE)(r_sum / sample_count);
            UBYTE avg_g = (UBYTE)(g_sum / sample_count);
            UBYTE avg_b = (UBYTE)(b_sum / sample_count);
            
            /* Compute average brightness */
            UBYTE avg_brightness = (UBYTE)((306L * avg_r + 601L * avg_g + 117L * avg_b) >> 10);
            
            /* Detail preservation: if brightest pixel is 3× brighter than average,
             * boost output to preserve the detail (e.g., stars in night sky) */
            if (max_brightness > avg_brightness * 3)
            {
                /* Blend: 70% average + 30% boosted brightness */
                /* This preserves the star while maintaining color accuracy */
                int boost_r = max_r - avg_r;
                int boost_g = max_g - avg_g;
                int boost_b = max_b - avg_b;
                
                avg_r = (UBYTE)MIN(255, avg_r + boost_r * 3 / 10);
                avg_g = (UBYTE)MIN(255, avg_g + boost_g * 3 / 10);
                avg_b = (UBYTE)MIN(255, avg_b + boost_b * 3 / 10);
            }
            
            /* Write output pixel (as 24-bit RGB — will be quantized to 256-color palette later) */
            int dest_offset = (dest_y * dest_w + dest_x) * 3;
            dest_rgb[dest_offset + 0] = avg_r;
            dest_rgb[dest_offset + 1] = avg_g;
            dest_rgb[dest_offset + 2] = avg_b;
        }
    }
}
```

**Key Parameters:**

```c
/* Brightness detection threshold */
#define ULTRA_DETAIL_THRESHOLD 3  /* Max must be 3× brighter than avg to trigger */

/* Brightness boost blend ratio */
#define ULTRA_BOOST_NUMERATOR   3  /* 3/10 = 30% boost */
#define ULTRA_BOOST_DENOMINATOR 10
```

### Palette Generation (256 Colors Maximum)

After detail-preserving downsampling produces a 24-bit RGB buffer, it must be quantized to a 256-color palette:

**Method: Median Cut or Popularity-Based**

```c
/* Generate optimal 256-color palette from RGB buffer */
ColorRegister palette[256];
int palette_size = generate_optimal_palette(dest_rgb, dest_w, dest_h, palette, 256);

/* Remap RGB pixels to palette indices */
UBYTE *indexed_pixels = remap_to_palette(dest_rgb, dest_w, dest_h, palette, palette_size);

/* Save as standard 8-bit AmigaOS icon with 256-color CMAP */
save_icon_with_palette(indexed_pixels, dest_w, dest_h, palette, palette_size);
```

**No dithering needed:** With 256 colors available, banding is minimal for most images. Dithering would only add noise.

### File Size Comparison

| Mode | Palette Size | Icon Size (64×64) | Compression | Approx. File Size |
|------|--------------|-------------------|-------------|-------------------|
| Reduced (16 colors) | 16 entries | 4,096 bytes (8-bit indices) | RLE | 2-3 KB |
| Reduced (64 colors) | 64 entries | 4,096 bytes (8-bit indices) | RLE | 2-4 KB |
| Ultra (256 colors) | 256 entries | 4,096 bytes (8-bit indices) | RLE | 3-5 KB |

**Note:** All modes use the same pixel buffer size (64×64 = 4,096 bytes). The only difference is palette size (16×3 = 48 bytes vs 256×3 = 768 bytes). File size is similar regardless of color count.

### Performance Analysis

| Phase | Standard Mode (64 colors) | Ultra Mode (256 colors) | Overhead |
|-------|---------------------------|-------------------------|----------|
| **Load source IFF** | 0.05s | 0.05s | — |
| **Downsampling** | 0.05s (area-average) | 0.06-0.07s (detail-preserving) | +20-40% |
| **Palette quantization** | 0.02s (Median Cut, 64 colors) | 0.03s (Median Cut, 256 colors) | +50% |
| **Dithering** | 0.05s (Floyd-Steinberg) | — (not needed with 256 colors) | -0.05s |
| **Save output IFF** | 0.03s | 0.03s | — |
| **Total** | ~0.20s | ~0.17-0.19s | **5-15% faster** |

**Result:** Ultra mode with 256 colors is slightly **faster or equal** to standard mode with 64 colors + dithering because it skips the dithering step.

### Visual Quality Comparison

| Detail Type | Standard (64 colors + dither) | Ultra (256 colors detail-preserving) |
|-------------|-------------------------------|-------------------------------------|
| **Hair texture (OCS artwork)** | Blurs into solid blocks, strands lost | Individual strands visible, texture preserved |
| **Facial details (wrinkles, eyelashes)** | Smoothed away, generic appearance | Sharp features, character retained |
| **Stars in night sky** | Most single-pixel stars lost | Stars preserved (30% brightness boost) |
| **Fabric texture (weave, folds)** | Flat appearance, detail lost | Texture and depth visible |
| **Fine text** | Readable but slightly fuzzy | Sharper with edge preservation |
| **Smooth gradients** | Minor banding with dithering | Smooth (256 colors) |
| **Color accuracy** | Good with dithering | Excellent (4× more colors) |
| **Overall quality** | Good | Excellent |

**Bottom line for OCS artwork:** Ultra mode is the difference between "recognizable but blurry" and "looks like a proper miniature of the original."

### RTG vs Chipset Display

**RTG screens (16-bit or higher):**
- Display all 256 colors **accurately** without remapping
- Fast icon display (no pen scavenging or palette juggling)
- Icons look exactly as intended

**Chipset 8-bit screens (256 colors total):**
- If Workbench palette matches icon palette → perfect display
- If Workbench palette differs → remapping/dithering required (slow)
- Multiple 256-color icons cause pen scavenging problems (see section 11, Mode 3)

**Ultra mode is a trade-off:**
- **Best for:** RTG users displaying classic OCS/ECS artwork who want maximum detail preservation
- **Acceptable for:** Chipset 8-bit users willing to accept palette remapping overhead for better quality
- **Avoid for:** Chipset users on very low-depth screens (32-color screens) — stick to 16-32 color modes

### Limitations and Warnings

**1. Icon format constraint:**
   - Icons are **palette-based** with maximum 256 colors (AmigaOS limitation)
   - Cannot bypass this limit, even on RTG screens
   - This is NOT true 16-bit or 24-bit color (just more palette entries)

**2. Chipset display performance:**
   - On Chipset 8-bit screens, 256-color icons may cause palette remapping overhead
   - Multiple 256-color icons exhaust shared pens (pen scavenging problem)
   - Consider using "Generic Palette" mode (section 11, Mode 3) alongside Ultra for Chipset compatibility

**3. File size:**
   - Slightly larger palette (256 entries vs 64 entries = +640 bytes)
   - RLE compression minimizes impact (~3-5 KB vs 2-4 KB)
   - Not a significant concern for hard drive users

**4. When NOT to use Ultra:**
   - Simple graphics, diagrams, or flat-color artwork (no fine details to preserve)
   - Landscapes or portraits without texture (standard 64-color + dither is sufficient)
   - Users on 32-64 color Chipset screens (pen scavenging will make display slow)
   - Auto mode should detect when Ultra is unnecessary

### GUI Integration

**Option 1: Extend max colors chooser (recommended)**

```
Max icon colors: [4 colours                  ▼]
                 [8 colours                  ]
                 [16 colours                 ]
                 [32 colours                 ]
                 [64 colours                 ]
                 [128 colours                ]
                 [No limit (256 colours)     ]
                 [Ultra (256 + detail-preserving)]
```

**Option 2: Separate checkbox**

```
Max icon colors: [64 colours                 ▼]

☑ Ultra quality (detail-preserving)
  Preserves fine details like hair texture and stars.
  Uses full 256-color palette. Best for RTG screens.
```

**Recommended: Option 1** — simpler UI, clearly shows Ultra is a "max colors" variant.

### Preference Fields

```c
/* In layout_preferences.h */
BOOL deficons_ultra_mode;  /* TRUE = 256 colors + detail-preserving, FALSE = standard */
#define DEFAULT_DEFICONS_ULTRA_MODE FALSE  /* Standard mode by default */
```

**Interaction with other settings:**

When `deficons_ultra_mode == TRUE`:
- **Override** `deficons_max_icon_colors` to 256 (full palette)
- **Disable** `deficons_dither_method` (no dithering needed with 256 colors)
- **Ignore** `deficons_lowcolor_mapping` (not applicable above 8 colors)
- **Still use** `deficons_palette_mode` (PICTURE vs SCREEN)

**IMPORTANT EXCEPTION:** Ultra mode CAN be combined with palette reduction + dithering for "Ultra + Dithering" workflow (see below).

### Ultra + Dithering Workflow (Best of Both Worlds)

**Scenario:** You want detail preservation from Ultra mode BUT need low color counts for Chipset compatibility (avoid pen scavenging).

**Pipeline:**

1. **Step 1:** Use Ultra detail-preserving downsample → 64×64 RGB buffer with preserved hair/stars/texture
2. **Step 2:** Reduce to target color count (16/32 colors) using palette quantization
3. **Step 3:** Apply Floyd-Steinberg dithering during remapping
4. **Result:** 16-color icon with dithered hair texture patterns

**Why this works better than standard pipeline:**

| Pipeline | Hair Strands in Source | After Downsampling | After Dither to 16 Colors | Result |
|----------|------------------------|-------------------|---------------------------|--------|
| **Standard** | Present | **Blurred away** by area-average | Dithering can't restore lost detail | Solid brown blur |
| **Ultra + Dither** | Present | **Preserved** by detail-boost | Dithering represents preserved texture | Dithered hair patterns visible! |

**Key insight:** Dithering can only dither what's in the image. If details are already averaged away, dithering just patterns the blur. But if Ultra preserves details first, dithering can represent them at low color counts.

**Example: 32-color OCS monkey portrait → 16-color icon**

- **Standard (area-average + 16-color dither):**
  - Hair strands: Averaged to mid-tone during downsampling → dithered as solid pattern (no texture visible)
  
- **Ultra (detail-preserve + 16-color dither):**
  - Hair strands: Preserved during downsampling (boosted by 30%) → dithered as textured pattern (strands visible!)

**When to use Ultra + Dithering:**

- ✅ Chipset users on 32-128 color screens (avoid pen scavenging but want quality)
- ✅ Classic OCS artwork with fine texture details
- ✅ File size concerns (16-32 colors = smaller than 256 colors)
- ✅ Want detail preservation without palette remapping overhead

**How to enable in GUI:**

```
Max icon colors: [16 colours                 ▼]
Dithering:       [Floyd-Steinberg (Best)     ▼]
☑ Ultra quality (detail-preserving)
```

**Implementation note:** When Ultra mode is enabled AND max icon colors < 256, the pipeline becomes:
1. Ultra detail-preserving downsample to RGB buffer
2. Generate optimal N-color palette (N = user's max icon colors setting)
3. Remap with dithering if enabled

This overrides the "Ultra disables dithering" rule. Ultra + low color count = automatic dithering recommendation.

### Implementation Notes

**⚠️ Implementation Flexibility Reminder**: All code examples in section 12e (detail-preserving algorithm, brightness thresholds, boost percentages, Ultra + Dithering pipeline) are guidance to illustrate the goals. If production testing shows different thresholds work better, or a different approach preserves details more effectively, adjust the implementation. The goal is "preserve hair strands/stars during downsampling" — the exact algorithm is flexible. Document significant changes in "Key Deviations" section.

**Phase E additions:**

**Item 43:** Add "Ultra quality mode" GUI control:
   - Add to max icon colors chooser: `"Ultra (256 + detail-preserving)"`
   - Or add checkbox: `"☑ Ultra quality (detail-preserving)"`
   - Info tooltip: "Best for images with stars, sparkles, or fine details. Uses full 256-color palette."
   
**Item 44:** Implement `ultra_quality_downsample()`:
   - Detail-preserving downsampling algorithm (as shown above)
   - Brightness detection and boost logic
   - Integration with existing `area_average_scale()` — use Ultra algorithm if mode enabled
   
**Item 45:** Integrate with palette quantization:
   - After Ultra downsampling, generate optimal 256-color palette (Median Cut)
   - Remap RGB buffer to palette indices
   - Save as standard 8-bit icon with 256-entry CMAP
   
**Item 46:** Add Auto mode logic (optional):
   - Detect high-frequency details in source image
   - Auto-enable Ultra mode if >1% of pixels are bright isolated details
   - Display notification: "Ultra mode auto-enabled (stars detected)"

### Tuning Parameters

**Brightness threshold** (how bright must a detail be to preserve it?):

```c
/* Conservative (preserve only very bright details like stars) */
#define ULTRA_DETAIL_THRESHOLD 4  /* Max must be 4× brighter */

/* Moderate (recommended — good balance) */
#define ULTRA_DETAIL_THRESHOLD 3  /* Max must be 3× brighter */

/* Aggressive (preserve even subtle highlights) */
#define ULTRA_DETAIL_THRESHOLD 2  /* Max must be 2× brighter */
```

**Boost strength** (how much to brighten the detail):

```c
/* Subtle boost (more natural, may still lose faint stars) */
#define ULTRA_BOOST_NUMERATOR   2  /* 2/10 = 20% */
#define ULTRA_BOOST_DENOMINATOR 10

/* Standard boost (recommended) */
#define ULTRA_BOOST_NUMERATOR   3  /* 3/10 = 30% */
#define ULTRA_BOOST_DENOMINATOR 10

/* Strong boost (maximum star visibility, may look artificial) */
#define ULTRA_BOOST_NUMERATOR   5  /* 5/10 = 50% */
#define ULTRA_BOOST_DENOMINATOR 10
```

### Auto Mode Integration

**When to auto-enable Ultra mode:**

```c
/* Auto mode decision logic */
BOOL should_use_ultra_mode(UBYTE *source_rgb, int src_width, int src_height)
{
    /* Check image characteristics first (fast test) */
    int high_freq_pixel_count = count_high_frequency_details(source_rgb, src_width, src_height);
    int total_pixels = src_width * src_height;
    
    /* If >1% of pixels are bright isolated details, use Ultra */
    if (high_freq_pixel_count * 100 / total_pixels > 1)
        return TRUE;  /* Image has stars/sparkles — Ultra benefits it */
    
    /* Otherwise standard 64-color mode is fine */
    return FALSE;
}
```

**High-frequency detail detection:**

```c
int count_high_frequency_details(UBYTE *rgb, int width, int height)
{
    int detail_count = 0;
    
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int offset = (y * width + x) * 3;
            UBYTE center_brightness = (UBYTE)((306L * rgb[offset+0] + 
                                               601L * rgb[offset+1] + 
                                               117L * rgb[offset+2]) >> 10);
            
            /* Sample 4 neighbors (N, S, E, W) */
            int n_off = ((y-1) * width + x) * 3;
            int s_off = ((y+1) * width + x) * 3;
            int e_off = (y * width + (x+1)) * 3;
            int w_off = (y * width + (x-1)) * 3;
            
            UBYTE north_brightness = (UBYTE)((306L * rgb[n_off+0] + 601L * rgb[n_off+1] + 117L * rgb[n_off+2]) >> 10);
            UBYTE south_brightness = (UBYTE)((306L * rgb[s_off+0] + 601L * rgb[s_off+1] + 117L * rgb[s_off+2]) >> 10);
            UBYTE east_brightness  = (UBYTE)((306L * rgb[e_off+0] + 601L * rgb[e_off+1] + 117L * rgb[e_off+2]) >> 10);
            UBYTE west_brightness  = (UBYTE)((306L * rgb[w_off+0] + 601L * rgb[w_off+1] + 117L * rgb[w_off+2]) >> 10);
            
            UBYTE avg_neighbor = (north_brightness + south_brightness + 
                                  east_brightness + west_brightness) / 4;
            
            /* If center is 3× brighter than neighbors, it's a detail */
            if (center_brightness > avg_neighbor * 3)
                detail_count++;
        }
    }
    
    return detail_count;
}
```

**Note:** This detection scan takes ~0.01s for a 256×256 image on 68020. It's fast enough to run on every image if Auto mode is enabled.

### Summary: Why Ultra Mode Matters for OCS Artwork

**Target use case:** Classic Amiga artwork in 32-64 colors (OCS/ECS) with fine texture details

**Problem solved:** When reducing 320×256 → 64×64 icons:
- Hair strands blur into solid blocks
- Facial features (wrinkles, eyelashes) disappear
- Fabric texture and brush strokes flatten
- Stars and sparkles vanish

**Ultra mode solution:**
1. **Detail-preserving algorithm:** Boosts contribution of fine details by 30% so they don't vanish in averaging
2. **256-color palette:** Provides enough colors to represent the gradations that downsampling creates (even from 32-color sources)
3. **No dithering needed:** 256 colors eliminate banding without adding noise
4. **Result:** Icons look like proper miniatures of the original artwork, not blurry thumbnails

**Performance:** Similar or faster than 64-color mode with dithering (~0.17-0.19s vs ~0.20s per icon)

**Best for:** 
- **Primary:** RTG users displaying classic OCS artwork, digitized artwork, hand-painted images, or any content where texture/detail preservation is critical
- **Also valuable:** Chipset users who combine Ultra with palette reduction + dithering (see "Ultra + Dithering Workflow" above) for best quality at low color counts

### Open Questions

- **Threshold tuning:** Is 3× the right brightness threshold, or should it be user-adjustable?
- **Boost strength:** 30% boost seems reasonable from initial testing, but needs validation with real images
- **Auto mode default:** Should Auto mode enable Ultra for high-frequency images by default, or require explicit user opt-in?
- **Performance on 68000:** Is detail-preserving scan acceptable on 68000 systems (~0.05-0.1s)?
- **UI placement:** Extend max colors chooser (Option 1) or separate checkbox (Option 2)?
- **Ultra + Dithering workflow:** Should GUI automatically enable dithering when Ultra mode + low color count (<64) is selected? Or leave it manual?
- **Ultra + Dithering recommendation:** Should Auto mode suggest "Ultra + 32 colors + dither" for Chipset users with detailed images?
- **Comparison baseline:** Should "standard mode" default to 64 colors or 128 colors for comparison?

---

## 12. IFF Template Contract

### ⚠️ FULLY SUPERSEDED — Templates Eliminated (Deviation 13)

> **Templates have been completely removed from the IFF thumbnail pipeline.** The original plan used `.info` template icons to provide dimensions, palette data, safe area coordinates, and rendering parameters via ToolTypes. Through a series of implementation discoveries, every role the templates played was found to be either unnecessary or better served by other mechanisms:
>
> | Original Template Role | Why It Was Unnecessary | Replacement |
> |----------------------|----------------------|-------------|
> | **Pixel data** (palette, pixel buffer) | PICTURE mode replaces the entire palette and overwrites all pixels | Blank pixel buffer created from dimensions |
> | **Safe area** (`ITIDY_TEXT_AREA` ToolType) | Overridden to full image dimensions for edge-to-edge thumbnails | Hardcoded `(0, 0, width, height)` |
> | **Background color** (`ITIDY_BG_COLOR` ToolType) | Disabled — no background fill needed for edge-to-edge thumbnails | `ITIDY_NO_BG_COLOR` constant |
> | **Palette mode** (`ITIDY_PALETTE_MODE` ToolType) | Better as a user-facing GUI setting than a hidden ToolType | `deficons_palette_mode` preference field + chooser gadget |
> | **Icon dimensions** (width × height) | Only remaining role — trivially replaced by a constant lookup | `itidy_get_iff_icon_dimensions()` maps preference to 48/64/100 |
> | **Icon metadata** (DiskObject structure) | `itidy_icon_image_apply()` clones the *target* icon, not the template | Not needed |
>
> **The three `iff_template_*.info` files in `Bin/Amiga/Icons/` are no longer referenced by any code and should be deleted.**
>
> See deviation 13 in the Key Deviations section for the full change summary.

### Original Rules (preserved for historical reference only):

### Rule 1: Safe Area Must Be Defined *(now overridden)*

Every IFF template icon **must** include the `ITIDY_TEXT_AREA=x,y,w,h` ToolType defining the rectangular area where the thumbnail will be rendered. Without this, the renderer falls back to the 4px-margin default, which may not match the template's visual design.

### Rule 2: No Custom Pixel-Art Borders *(still true — icon is frameless)*

IFF templates **must not** rely on custom pixel-art borders or decorative frame artwork stored in the pixel data. In `ITIDY_PALETTE_MODE=PICTURE`, the template's entire palette is replaced with the source image's CMAP. Any pixel-art drawn at specific palette indices will display with wrong colors after palette replacement.

**Instead**: Templates should be plain safe-area fill (single `ITIDY_BG_COLOR` fill covering the entire pixel area). The icon's visible border/frame is handled by Workbench's standard icon border rendering (`GFLG_GADGHCOMP` or similar), which is drawn by Intuition at display time and is independent of the icon's pixel palette.

### Rule 3: Background Index Must Be Defined *(now overridden)*

Every template **must** include `ITIDY_BG_COLOR=N` specifying the palette index used for:
- Filling the safe area before rendering
- Letterbox/pillarbox bars when the image aspect ratio doesn't fill the safe area

This index will be remapped to the closest match in the source image's palette during rendering.

### Rule 4: Palette Mode Must Be Specified *(still applies)*

Every template **must** include `ITIDY_PALETTE_MODE=PICTURE` (or `SCREEN` when supported). This makes the rendering mode explicit and self-documenting. The default if missing is `PICTURE`.

### Rule 5: Templates Are Size-Specific *(still true but output is cropped)*

Each template file is designed for one specific icon dimension. ~~The output icon will be exactly the template's pixel dimensions.~~ The template's dimensions set the **maximum** thumbnail size. The output icon is cropped to the actual scaled thumbnail dimensions, which may be smaller if the image's aspect ratio doesn't fill the full template area.

### Minimum Valid IFF Template ToolTypes:

```
ITIDY_PALETTE_MODE=PICTURE
```

> Note: `ITIDY_TEXT_AREA` and `ITIDY_BG_COLOR` are no longer required for IFF thumbnails but are still used by the text preview renderer.

### Why These Rules Mattered (Historical Context):

The two-icon-merge architecture means the template provides **all pixel data** (dimensions, palette, pixel buffer) while the target Workbench icon provides only **metadata** (DefaultTool, ToolTypes, icon type). When the renderer replaces the palette (PICTURE mode), any assumption about palette index → color mapping in the template's pixels is invalidated.

**Post-implementation update**: ~~In practice, the template's pixel data is entirely irrelevant for IFF thumbnails~~ → **Templates have been eliminated entirely (deviation 13).** The IFF pipeline no longer loads any template `.info` files. Dimensions come from `itidy_get_iff_icon_dimensions()` (preference-to-pixel constant lookup), palette mode from `deficons_palette_mode` GUI preference, and blank pixel buffers are created directly. The template icon's two remaining roles (dimensions + palette mode ToolType) are now served by preference constants and a GUI chooser respectively.

---

## 13. Icon Size Tiers

### Three Size Options:

| Size | Preference Value | Dimensions | Constant | Use Case |
|------|-----------------|------------|----------|----------|
| Small | `ITIDY_ICON_SIZE_SMALL` (0) | 48×48 | `ITIDY_IFF_SIZE_SMALL` | Low-res screens, many icons |
| Medium | `ITIDY_ICON_SIZE_MEDIUM` (1) | 64×64 | `ITIDY_IFF_SIZE_MEDIUM` | Default, good balance |
| Large | `ITIDY_ICON_SIZE_LARGE` (2) | 100×100 | `ITIDY_IFF_SIZE_LARGE` | Hi-res screens, detail |

### Icon Properties (Template-Free):

- **Border**: Controlled by `deficons_thumbnail_border_mode` preference (0=None, 1=WB Smart, 2=WB Always, 3=Bevel Smart, 4=Bevel Always; default: Bevel Smart). WB modes set `is_frameless = FALSE` (Workbench draws its standard frame). Bevel and None modes set `is_frameless = TRUE`. Bevel is painted into the pixel buffer by `icon_image_bevel.c` after Ultra palette reduction. Workbench complement highlighting used for selection.
- **Safe area**: Full image dimensions `(0, 0, width, height)` — edge-to-edge thumbnails, no margin
- **Background**: Disabled (`ITIDY_NO_BG_COLOR`) — thumbnail pixels fill the entire icon
- **Palette mode**: Controlled by `deficons_palette_mode` preference (default: PICTURE)
- **Transparency**: Disabled (`transparent_color = -1`) — no transparent pixels
- **Dimensions**: Derived from `itidy_get_iff_icon_dimensions()` based on preference constant

### Size Selection Mechanism:

**Option A (chosen)**: Add a `deficons_icon_size_mode` preference to `LayoutPreferences`:

```c
/* In layout_preferences.h */
#define ITIDY_ICON_SIZE_SMALL   0
#define ITIDY_ICON_SIZE_MEDIUM  1
#define ITIDY_ICON_SIZE_LARGE   2

/* Added to LayoutPreferences struct */
UWORD deficons_icon_size_mode;   /* 0=Small(48), 1=Medium(64), 2=Large(100) */
```

A chooser gadget ~~will be~~ has been added to the DefIcons settings GUI (`src/GUI/deficons_settings_window.c`), along with a palette mode chooser.

### ~~Template Path Selection~~ → Dimension Lookup (Deviation 13):

```c
/* In icon_content_preview.h — pixel dimension constants */
#define ITIDY_IFF_SIZE_SMALL    48
#define ITIDY_IFF_SIZE_MEDIUM   64
#define ITIDY_IFF_SIZE_LARGE   100

/* In icon_content_preview.c — map preference to pixel dimensions */
UWORD itidy_get_iff_icon_dimensions(const LayoutPreferences *prefs)
{
    switch (prefs->deficons_icon_size_mode)
    {
        case ITIDY_ICON_SIZE_SMALL:  return ITIDY_IFF_SIZE_SMALL;
        case ITIDY_ICON_SIZE_LARGE:  return ITIDY_IFF_SIZE_LARGE;
        default:                     return ITIDY_IFF_SIZE_MEDIUM;
    }
}
```

> **Replaces**: `itidy_get_iff_template_path()` which returned `ITIDY_IFF_TEMPLATE_SMALL/MEDIUM/LARGE` path strings. Template `.info` files are no longer loaded.

---

## 14. HAM and EHB Support

### EHB (Extra Half-Brite) — Low-Hanging Fruit, Implement Early

Uses 6 bitplanes where colors 32–63 are half-brightness versions of colors 0–31. This is essentially **palette expansion followed by standard decode** — making it far simpler than HAM and a good early win for broader image support.

**Implementation plan** (Phase B or early Phase C):

Detect EHB via CAMG flags (`camg & 0x0080`). If EHB detected:
1. Read the 32-color CMAP normally
2. Call `expand_ehb_palette()` to generate a 64-color palette:
   - Copy entries 0–31 as-is
   - For entries 32–63: `R = original_R / 2`, `G = original_G / 2`, `B = original_B / 2`
3. Decode bitplanes as a standard 6-plane image (pixels index 0–63)
4. Proceed with normal scaling and rendering — the 64-color palette is the icon palette

```c
static BOOL expand_ehb_palette(struct ColorRegister *palette_32,
                                struct ColorRegister **out_palette_64,
                                ULONG *out_size)
{
    struct ColorRegister *pal64 = (struct ColorRegister *)whd_malloc(64 * sizeof(struct ColorRegister));
    if (!pal64) return FALSE;
    
    /* Copy original 32 colors */
    memcpy(pal64, palette_32, 32 * sizeof(struct ColorRegister));
    
    /* Generate half-bright copies for indices 32-63 */
    for (int i = 0; i < 32; i++)
    {
        pal64[32 + i].red   = palette_32[i].red   >> 1;
        pal64[32 + i].green = palette_32[i].green >> 1;
        pal64[32 + i].blue  = palette_32[i].blue  >> 1;
    }
    
    *out_palette_64 = pal64;
    *out_size = 64;
    return TRUE;
}
```

**Why implement EHB early:** Many classic Amiga images use EHB (it was the standard way to get 64 colors on OCS/ECS). Skipping EHB would leave a significant gap in coverage. The implementation is minimal — a single palette expansion function and then everything else works unchanged.

### HAM (Hold-And-Modify) — ✅ IMPLEMENTED via Datatype Fallback

HAM6 (6 bitplanes) and HAM8 (8 bitplanes) use a stateful per-scanline decoder where each pixel either sets a palette color or modifies one RGB component of the previous pixel. This produces near-true-color images from limited bitplane counts.

**Initial implementation**: Detect HAM via CAMG flags (`camg & 0x0800`). If HAM detected:
- Log: `"HAM image detected — thumbnail not yet supported: %s"`
- Return `ITIDY_PREVIEW_NOT_SUPPORTED`
- Stub function `decode_ham_to_rgb()` with TODO comment for future implementation

**✅ IMPLEMENTED — Datatype Fallback (2026-02-10)**:

Rather than implementing the complex HAM stateful decoder, HAM images now fall back to **Amiga's built-in `picture.datatype`** which has full HAM6/HAM8 support. This approach is:
- **Simpler to maintain** — OS does the decoding
- **Future-proof** — works for other exotic formats (HAM8, JPEG, PNG, etc.) if datatypes installed
- **Robust** — uses proven OS decoder, not custom implementation

#### Implementation: `itidy_render_via_datatype()`

Location: `src/icon_edit/icon_iff_render.c` (lines ~1320-1570)

**Flow**:
1. **Open datatypes.library V39+**
2. **Open image via `NewDTObject()`** with:
   - `DTA_SourceType = DTST_FILE`
   - `DTA_GroupID = GID_PICTURE`
   - `PDTA_Remap = FALSE` (we'll handle palette ourselves)
3. **Get BitMapHeader** via `GetDTAttrs(PDTA_BitMapHeader)` for dimensions and depth
4. **Get ModeID** via `GetDTAttrs(PDTA_ModeID)` for aspect ratio flags (hires/lace)
5. **Infer missing flags** (datatypes often strip CAMG details):
   - **Hires inference**: If `!is_hires && src_width >= ITIDY_HIRES_WIDTH_THRESHOLD` (400px) → set `is_hires = TRUE`
   - **Lace inference**: If `!is_lace && src_height >= 400` → set `is_lace = TRUE`
6. **Allocate RGB24 buffer** (3 bytes per pixel × width × height)
7. **Read decoded pixels** via `DoMethod(PDTM_READPIXELARRAY)`:
   - Format: `PBPAFMT_RGB` (24-bit RGB)
   - Returns the **full decoded RGB image** (HAM artifacts resolved by datatype)
8. **Build 6×6×6 RGB color cube palette** (216 colors) + grayscale ramp:
   - Generates uniform color distribution for quantization
   - Fills remaining palette slots (up to 256) with grayscale
9. **Quantize RGB24 → palette** using `itidy_find_closest_palette_color()`:
   - For each pixel: find closest match in destination palette
   - Creates chunky indexed buffer
10. **Calculate aspect-corrected destination size** (same as normal IFF code):
    - Uses `display_width × display_height` (aspect-corrected) for ratio calculation
    - Fits into safe area maintaining aspect ratio
    - Centers result if doesn't fill safe area
11. **Scale via `area_average_scale()`** from **full source buffer** (src_width × src_height) to calculated destination
12. **Store output dimensions** for cropping

#### Key Learnings from Testing:

**Issue #1: Black and white output**  
- **Problem**: Initially quantized to the destination icon's existing palette (from `def_picture.info`), which was mostly black (palette entries 1-255 were `000000`)
- **Fix**: Generate a proper 6×6×6 RGB color cube (216 colors) before quantization. This gives even color distribution and good color matching quality

**Issue #2: Only top half of image visible**  
- **Problem**: Read RGB24 at full size (336×442) but told scaler to use `display_width × display_height` (336×221 after lace correction). Scaler only read top 221 rows of 442-row buffer
- **Fix**: Pass **actual buffer dimensions** (`src_width × src_height`) to scaler, not aspect-corrected dimensions. The aspect correction happens through the destination size calculation

**Issue #3: Image rendered at double height**  
- **Problem**: Scaled from 336×442 to 48×48 (square), ignoring interlace aspect ratio. Should have scaled to ~48×31
- **Fix**: Calculate destination size using aspect ratio of `display_width × display_height` (same algorithm as normal IFF code). Use fixed-point 16.16 math to fit aspect-corrected dimensions into safe area

**Issue #4: ModeID incomplete**  
- **Problem**: `PDTA_ModeID` returned `0x00000800` (only HAM bit), not full CAMG `0x00011804` (HAM+LACE+other flags). Lace correction not applied
- **Fix**: Added dimension-based flag inference (hires from width ≥400px, lace from height ≥400px), matching the robustness of the normal IFF parser

#### Fallback Integration:

In `apply_iff_preview()` (src/icon_edit/icon_content_preview.c):

```c
/* Try native IFF parser first */
result = itidy_render_iff_thumbnail(source_path, &iff_params, &img);

if (result == ITIDY_PREVIEW_NOT_SUPPORTED)
{
    /* Fall back to datatype for HAM/exotic formats */
    log_info(LOG_ICONS, "apply_iff_preview: native parser cannot handle format "
             "(HAM?) - trying datatype fallback for '%s'\n", source_path);
    
    result = itidy_render_via_datatype(source_path, &iff_params, &img);
    
    if (result == 0)
    {
        log_info(LOG_ICONS, "apply_iff_preview: datatype fallback succeeded for '%s'\n",
                 source_path);
    }
    else
    {
        log_warning(LOG_ICONS, "apply_iff_preview: datatype fallback also failed "
                    "for '%s' (error=%d)\n", source_path, result);
    }
}
```

**Advantages of Hybrid Approach**:
- **Native parser for 90% of images** (faster, no library dependencies)
- **Datatype fallback for HAM/JPEG/PNG** (handles exotic formats automatically)
- **Full control over common cases** (CAMG quirks, width-based hires inference)
- **OS handles edge cases** (HAM state machine, JPEG decompression, etc.)

**Future HAM implementation notes (if native decoder desired):**
- Decode to a 24-bit RGB buffer (3 bytes per pixel) first
- Then quantize to the icon palette using `find_closest_palette_color()`
- HAM6: top 2 bits select mode (set/modify-R/modify-G/modify-B), bottom 4 bits are value
- HAM8: top 2 bits select mode, bottom 6 bits are value
- Requires per-scanline state reset (each row starts with palette color 0)
- HAM images also benefit from the scaling fast-path pre-filter (Section 10) since they tend to be large

---

## 15. Type Detection — DefIcons Type Tree Mapping

### IFF-ILBM in the Type Tree:

```
ROOT
  project
    picture              ← Category (generation 2)
      ilbm               ← IFF-ILBM images (generation 3)
      acbm               ← IFF-ACBM images (generation 3)
      bmp, gif, jpeg...  ← Other picture formats (future)
    iff                  ← NON-picture IFF types (catalogs, datatypes, etc.)
```

**Key**: IFF-ILBM is under `"picture"`, NOT under `"iff"`. The `"iff"` category holds non-image IFF types.

### Type Detection Function:

```c
BOOL itidy_is_iff_preview_type(const char *type_token)
{
    /* Direct match for known IFF image types */
    if (strcmp(type_token, "ilbm") == 0) return TRUE;
    /* acbm could be added later */
    
    /* Could also match by category: */
    /* category = deficons_get_resolved_category(type_token); */
    /* if (category && strcmp(category, "picture") == 0) return TRUE; */
    
    return FALSE;
}
```

Initially match only `"ilbm"` (the standard IFF picture format). Broader `"picture"` category matching would also pick up JPEG, PNG, etc., which require different decoders.

### Future Expansion Path — Datatype-Based Decoding

Once the ILBM-specific decoder is proven, a future enhancement could add a **datatype-based fallback path** for other picture types (JPEG, PNG, GIF, TIFF, etc.):

- **Default behaviour** (current): Only `"ilbm"` matched — pure iffparse.library decoding
- **Advanced toggle** (future): A preference or command-line flag to enable datatype decoding
  - Opens the source file via `picture.datatype` (`NewDTObjectA()`)
  - Obtains decoded bitmap/pixel data via `PDTA_BitMap` or `PDTA_DestBitMap`
  - Uses `PDTA_ModeID` for CAMG-equivalent mode detection
  - Feeds decoded pixels into the same scaling/rendering pipeline
  - Would match any type under the `"picture"` category

This approach would give iTidy thumbnail support for **any format with an installed datatype**, without needing format-specific decoders. However, it introduces a dependency on the datatype being present and working correctly. Keep this as a clearly separate code path activated by an opt-in preference, not a silent upgrade.

---

## 16. Complete Rendering Pipeline

### IFF Thumbnail Pipeline (step-by-step) — AS IMPLEMENTED (post-deviation 13):

```
 1. itidy_is_iff_preview_type(type_token)       → Verify type is "ilbm"
 2. itidy_check_cache_valid(path, ITIDY_KIND_IFF_THUMBNAIL, size, date)
                                                  → Skip if icon already up-to-date
 3. GetDiskObject(source_path)                    → Load TARGET icon (Workbench metadata)
 4. prefs = GetGlobalPreferences()                → Get user preferences
    icon_dim = itidy_get_iff_icon_dimensions(prefs)  → Map size pref to pixels (48/64/100)
 5. itidy_icon_image_create_blank(icon_dim, icon_dim, 0, &img)
                                                  → Create blank pixel buffer at icon size
    → Create 256-entry placeholder palette (overwritten by PICTURE mode)
 6. Build iTidy_IFFRenderParams directly:
    → safe area = (0, 0, icon_dim, icon_dim) — full image, edge-to-edge
    → bg_color_index = ITIDY_NO_BG_COLOR (no background fill)
    → palette_mode = prefs->deficons_palette_mode
    → All IFF-specific fields zeroed (populated by renderer)
10. itidy_render_iff_thumbnail(source_path, &iff_params, &img)
                                                   → Parse IFF, decode, scale, render
    → Stores output_width/height/offset_x/offset_y in iff_params
 7. itidy_render_iff_thumbnail(source_path, &iff_params, &img)
                                                   → Parse IFF, decode, scale, render
    → Stores output_width/height/offset_x/offset_y in iff_params
 8. CROP: Allocate tight buffer (output_width × output_height)
    → Copy only thumbnail rows from rendered buffer
    → Replace img pixel buffer with tight crop
    → Discard selected image (no safe area for index swap)
    → Set transparent_color_normal = -1 (disable transparency)
    → Set is_frameless = TRUE (thumbnail IS the icon)
 9. itidy_icon_image_dump(&img, &params.base, path) → Debug hex grid (if enabled)
10. itidy_icon_image_apply(target_icon, &img)      → Clone target, apply rendered pixels
11. itidy_stamp_created_tooltypes(clone, ITIDY_KIND_IFF_THUMBNAIL, ...)
                                                   → Stamp ITIDY_CREATED/KIND/SRC
12. itidy_icon_image_save(source_path, clone)      → Save as OS 3.5 format
13. Cleanup: iff_params_free → FreeDiskObject(clone) → icon_image_free → FreeDiskObject(target)
```

### Original Pipeline (preserved for reference):

```
(Original plan used template icons — steps 4-6 were different:)
 4. itidy_get_iff_template_path()              → Select template .info by size preference
 5. GetIconTagList(template_path, ...)         → Load IMAGE TEMPLATE from .info file
 6. itidy_icon_image_extract(image_icon, &img) → Extract pixels+palette from template
    └─ FALLBACK: Create blank buffers if template not palette-mapped
 7. itidy_get_iff_render_params(image_icon, &img, &iff_params)
                                               → Build render params from template ToolTypes
 8. FreeDiskObject(image_icon)                 → Done with template DiskObject
 9. Override safe area to full image (0, 0, w, h) — bg_color = ITIDY_NO_BG_COLOR

All of steps 4-9 have been replaced by steps 4-6 in the current pipeline (deviation 13).
```

### Inside `itidy_render_iff_thumbnail()` (step 7 expanded):

```
 7a. Open iffparse.library
 7b. AllocIFF → InitIFFasDOS → Open file → OpenIFF
 7c. PropChunk(BMHD, CMAP, CAMG) + StopChunk(BODY)
 7d. ParseIFF(IFFPARSE_SCAN)
 7e. FindProp(BMHD) → validate, extract dimensions/depth/compression
 7f. FindProp(CAMG) → decode viewport flags (HAM/EHB/interlace/hires)
 7g. If HAM or EHB → cleanup, return ITIDY_PREVIEW_NOT_SUPPORTED
 7h. FindProp(CMAP) → copy palette to iff_params.src_palette
 7i. Allocate src_chunky buffer (src_width * src_height bytes)
 7j. ReadChunkBytes(BODY) into compressed buffer
 7k. Decompress ByteRun1 (if compression=1) row by row
 7l. Convert interleaved bitplanes to chunky pixels
 7m. CloseIFF → Close file → FreeIFF → CloseLibrary
 7n. Calculate display dimensions (halve height if interlace, halve width if hires)
 7o. Calculate scaling to fit safe area, preserving aspect ratio
 7p. Fill safe area with bg_color_index
 7q. If palette_mode == PICTURE:
       - Replace img->palette_normal with src_palette
       - Area-average scale src_chunky → pixel_buffer (direct index write)
 7r. If palette_mode == SCREEN:
       - Stub: log warning, fall back to PICTURE mode
 7s. Free src_chunky buffer
 7t. Return ITIDY_PREVIEW_OK
```
### Selected Image Contrast Validation (step 10b)

The existing `build_selected_image()` swaps palette indices within the safe area to produce a visually distinct selected state. However, IFF thumbnails can have mid-tone heavy palettes where the index swap produces near-identical colors, making selection invisible.

**Minimum contrast rule:**

After building the selected image, validate that the average luminance delta between normal and selected safe-area pixels exceeds a threshold:

```c
/* Ensure selected state is visually distinct */
static BOOL validate_selected_contrast(const iTidy_IconImageData *img,
                                        const iTidy_RenderParams *params)
{
    /* Sample a grid of pixels from the safe area */
    /* Compare normal vs selected pixel luminance */
    /* If average delta < ITIDY_MIN_SELECT_CONTRAST (e.g., 20/255), */
    /* apply a fallback: invert safe area, or apply a uniform darken/lighten */
    return TRUE;  /* TRUE if contrast is sufficient */
}
```

If contrast is insufficient, the fallback applies a uniform brightness shift (darken by 30% or lighten by 30%, whichever moves further from the normal average) to the selected image's safe-area pixels. This ensures the user can always see which icon is selected, regardless of the source image's tonal distribution.

---

## 17. Shared Utility Promotion

### `find_closest_palette_color()`

Currently `static` in `icon_text_render.c`. Needed by both text renderer and IFF renderer.

**Move to**: `icon_image_access.c` (non-static)  
**Declare in**: `icon_image_access.h`

```c
/* Euclidean RGB distance palette matching */
UBYTE itidy_find_closest_palette_color(const struct ColorRegister *palette,
                                       ULONG palette_size,
                                       UBYTE target_r, UBYTE target_g, UBYTE target_b);
```

Rename with `itidy_` prefix for namespace safety. Update all call sites in `icon_text_render.c`.

---

## 18. ~~New ToolType Constants~~ → Removed (Deviation 13)

### ~~Added to `icon_image_access.h`~~ — Removed from `icon_iff_render.h`:

| Constant | String | Original Purpose | Status |
|----------|--------|-----------------|--------|
| ~~`ITIDY_TT_PALETTE_MODE`~~ | ~~`"ITIDY_PALETTE_MODE"`~~ | ~~Palette mode selection (PICTURE/SCREEN)~~ | ❌ Removed — palette mode now stored in `LayoutPreferences.deficons_palette_mode` and controlled by GUI chooser |

### ~~Template Icon ToolType Examples~~ — Templates Eliminated:

> Templates no longer exist. The palette mode, icon dimensions, and all rendering parameters that were previously read from template ToolTypes are now derived from `LayoutPreferences` fields set via the DefIcons settings GUI.

---

## 19. Makefile Changes

### Add to `ICON_EDIT_SRCS`:

```makefile
ICON_EDIT_SRCS = \
    $(SRC_DIR)/icon_edit/icon_image_access.c \
    $(SRC_DIR)/icon_edit/icon_text_render.c \
    $(SRC_DIR)/icon_edit/icon_iff_render.c \
    $(SRC_DIR)/icon_edit/icon_content_preview.c
```

### Add dependency rule:

```makefile
$(OUT_DIR)/icon_edit/icon_iff_render.o: $(SRC_DIR)/icon_edit/icon_iff_render.c \
    $(SRC_DIR)/icon_edit/icon_iff_render.h \
    $(SRC_DIR)/icon_edit/icon_image_access.h
```

No linker changes needed — `iffparse.library` is a standard AmigaOS library opened at runtime via `OpenLibrary()`. The `-lamiga` linker library already contains its stubs.

---

## 20. Test Plan

### Test Images Available:

Six classic Amiga IFF ILBM images in `Bin/Amiga/Tests/images/`:

| File | Expected Content |
|------|-----------------|
| `Gorilla` | Classic Amiga gorilla picture |
| `KingTut` | King Tutankhamun image |
| `Space` | Space scene |
| `Venus` | Botticelli's Venus |
| `Waterfall` | Waterfall photograph |
| `Yacht` | Yacht/sailing image |

### Test Scenarios:

1. **Basic decode**: Parse each test image, verify BMHD extraction, CMAP palette, BODY decompression
2. **Bitplane→chunky**: Verify pixel values match expected palette indices
3. **ByteRun1**: Test both compressed and uncompressed images
4. **Scaling**: Verify aspect ratio preservation at all three size tiers
5. **Interlace detection**: Test with interlace IFF (if available) — verify height halving
6. **HAM/EHB rejection**: Test with HAM images — verify graceful skip with log message
7. **Missing CAMG**: Test with old IFF files lacking CAMG chunk — verify fallback
8. **Palette replacement**: Verify icon uses source image's CMAP palette, edge-to-edge thumbnail fills entire icon
9. **Cache validation**: Render once, re-run — verify skip on unchanged source
10. **Selected image**: Verify Workbench complement highlighting provides selection feedback (no custom selected image)
11. **Debug dump**: Verify hex grid output shows expected pixel indices
12. **Size switching**: Change icon size preference (Small/Medium/Large) in DefIcons settings, verify thumbnails regenerate at correct dimensions
13. **Palette mode**: Verify Picture mode (default) uses source CMAP. Screen mode should fall back to Picture with log message

### Testing Workflow:

1. Copy test images to a test folder on the Amiga
2. Configure DefIcons to recognize `.iff` / `ilbm` type files
3. Run iTidy on the test folder
4. Inspect generated `.info` files visually on Workbench
5. Check `Bin/Amiga/logs/` for errors and debug dumps
6. Compare thumbnail appearance across all three size tiers

---

## 21. Implementation Order

### Phase A: Core IFF Parsing (get data out of the file) — ✅ COMPLETE

1. ✅ Create `icon_iff_render.h` with all structs and prototypes
2. ✅ Create `icon_iff_render.c` with IFF parsing (iffparse.library)
3. ✅ Implement ByteRun1 decompression
4. ✅ Implement bitplane→chunky conversion
5. ✅ Implement CAMG detection (interlace, HAM, EHB)
6. ✅ Add HAM stub (detect and skip with log message)
7. ✅ **Implement EHB support** — palette expansion via `expand_ehb_palette()`, then standard decode
8. ✅ Implement CMAP validation (Section 11 edge cases)
9. ✅ Test standalone parsing with debug output

> **Phase A implementation note**: All IFF parsing, ByteRun1 decompression, bitplane→chunky conversion, CAMG detection, EHB palette expansion, and HAM stub were implemented in a single phase. `icon_iff_render.c` is ~1200 lines. `find_closest_palette_color()` was promoted to shared as `itidy_find_closest_palette_color()` in `icon_image_access.c/.h`. Compiled cleanly on first build.

### Phase B: Scaling and Rendering (get pixels into the icon) — ✅ COMPLETE

10. ✅ Implement interlace height correction
11. ✅ Implement area-average downscaling with aspect ratio preservation
12. ✅ Add scaling fast-path with 2×2 pre-filter for extreme ratios (Section 10)
13. ✅ Implement picture palette mode (replace palette, write indices)
14. ✅ Stub screen palette mode
15. ✅ Promote `find_closest_palette_color()` to shared (done in Phase A)
16. ✅ Test rendering into a template pixel buffer

> **Phase B implementation note**: All scaling uses fixed-point 16.16 arithmetic exclusively — no floating point. The `area_average_scale()` function handles arbitrary reduction ratios. The `prefilter_2x2()` function creates a half-size intermediate buffer for extreme reductions (>8:1 ratio on images >65K pixels). Compiled cleanly.

### Phase C: Integration (wire into the pipeline) — ✅ COMPLETE

17. ✅ Add `itidy_is_iff_preview_type()` to dispatch
18. ✅ Add IFF branch to `icon_content_preview.c`
19. ✅ ~~Define template path constants~~ → Replaced with `ITIDY_IFF_SIZE_*` pixel dimension constants (deviation 13)
20. ✅ Update Makefile
21. ✅ ~~Create initial template icons~~ → Templates eliminated; blank buffers created from dimension constants (deviation 13)
22. ✅ Add selected image contrast validation (step 10b in pipeline)
23. ✅ Test end-to-end with test images

> **Phase C implementation notes**:
> - First compile failed because `deficons_icon_size_mode` didn't exist yet (Phase D dependency). Fixed by stubbing `itidy_get_iff_template_path()` to always return medium template.
> - **Runtime bug #1**: Template `iff_template_medium.info` is old-format planar, not OS 3.5 palette-mapped. `itidy_icon_image_extract()` failed on it. Fixed by adding fallback path: query template dimensions via `ICONCTRLA_GetWidth/GetHeight`, create blank pixel/palette buffers via `itidy_icon_image_create_blank()`. PICTURE mode replaces everything anyway.
> - **Runtime bug #2**: Thumbnail appeared floating inside the icon frame with wasted space. Fixed by overriding safe area to `(0, 0, img.width, img.height)` and cropping the pixel buffer to exact thumbnail dimensions after rendering.
> - **Runtime bug #3**: Black (palette index 0) was transparent in Workbench, causing holes in dark images (Space, Waterfall). Fixed by setting `transparent_color_normal = -1` to disable transparency.
> - Set `is_frameless = TRUE` since the thumbnail IS the icon (no border artwork).
> - Discarded selected image building — edge-to-edge thumbnails have no safe area for the bg/text index swap.
> - Added `output_width/height/offset_x/offset_y` fields to `iTidy_IFFRenderParams` for post-render cropping.
> - **Deviation 13 (later refactor)**: All template loading, extraction, ToolType parsing, and `itidy_get_iff_render_params()` were subsequently removed. The pipeline now creates blank pixel buffers directly from dimension constants and builds render params inline. See Phase D notes.

### Phase D: Size & Palette UI — ✅ COMPLETE

24. ✅ Add `deficons_icon_size_mode` to `LayoutPreferences`
25. ✅ Add size chooser to DefIcons settings GUI
26. ✅ ~~Create all three template icon sizes~~ → N/A — templates eliminated (deviation 13)
27. ✅ Test size switching
28. ✅ Add `deficons_palette_mode` to `LayoutPreferences` (deviation 13)
29. ✅ Add palette mode chooser to DefIcons settings GUI (deviation 13)
30. ✅ Remove `itidy_get_iff_template_path()` → replaced by `itidy_get_iff_icon_dimensions()` (deviation 13)
31. ✅ Remove `itidy_get_iff_render_params()` → params built directly in `apply_iff_preview()` (deviation 13)
32. ✅ Remove `ITIDY_IFF_TEMPLATE_*` path constants and `ITIDY_TT_PALETTE_MODE` ToolType constant (deviation 13)
33. ✅ Refactor `apply_iff_preview()` to template-free pipeline (~100 lines removed) (deviation 13)

> **Phase D implementation note**: Added `UWORD deficons_icon_size_mode` to `LayoutPreferences` struct with `DEFAULT_DEFICONS_ICON_SIZE_MODE = 1` (medium). Added `GID_ICON_SIZE_CHOOSER` enum value, `icon_size_chooser_obj`, `icon_size_labels[]` array (`"Small (48x48)"`, `"Medium (64x64)"`, `"Large (100x100)"`), gadget creation, layout placement, and value read on OK in `deficons_settings_window.c`.
>
> **Template elimination (deviation 13)**: Subsequently added `UWORD deficons_palette_mode` with `DEFAULT_DEFICONS_PALETTE_MODE = 0` (Picture). Added `GID_PALETTE_MODE_CHOOSER`, `palette_mode_chooser_obj`, `palette_mode_labels[]` array (`"Picture (original colors)"`, `"Screen (match Workbench)"`). Removed all template-related code: `itidy_get_iff_template_path()`, `itidy_get_iff_render_params()`, `ITIDY_IFF_TEMPLATE_*` constants, `ITIDY_TT_PALETTE_MODE` constant. The `apply_iff_preview()` function was refactored to derive dimensions from `itidy_get_iff_icon_dimensions()` (preference-to-pixel lookup) and build render params directly. The three `iff_template_*.info` files in `Bin/Amiga/Icons/` are no longer needed.

### Phase E: Polish and Future — ✅ MOSTLY COMPLETE (items 39-45 done, 46 deferred)

**File Organization Note**: When implementing items 39-46 (palette reduction, dithering, Ultra mode), organize code into `src/icon_edit/palette/` subfolder with separate focused modules rather than one monolithic file. See section 12b Implementation Notes for proposed file structure (`palette_quantization.c`, `palette_dithering.c`, `palette_mapping.c`, `palette_reduction.c`, `palette_grayscale.c`, `ultra_downsample.c`).

34. ✅ HAM support via datatype fallback (implemented 2026-02-10) — `itidy_render_via_datatype()` handles HAM6/HAM8 via `picture.datatype`, RGB24 pixel extraction via `PDTM_READPIXELARRAY`, 6×6×6 color cube quantization, aspect-ratio-aware scaling
35. 🔲 Screen palette mode implementation (read Workbench palette via `GetRGB32()`, quantize with optional dithering) — GUI chooser already exists
36. 🔲 Datatype-based rendering for non-ILBM picture types (JPEG, PNG, GIF) — infrastructure exists (`itidy_render_via_datatype()`), needs type detection expansion
37. 🔲 Low-res pixel aspect ratio correction (subtle)
38. 🔲 Consider: selected image for frameless thumbnails (complement highlight may suffice)
39. ✅ Maximum icon color count option (reduce palette to 4/8/16/32/64/128/256 colors) — see section 12b below — **File**: `palette/palette_reduction.c` (implemented 2026-02-16)
40. ✅ Dithering methods: None/Ordered/Floyd-Steinberg/Auto — see section 12c below — **Files**: `palette/palette_dithering.c`, `palette/palette_mapping.c` (implemented 2026-02-16)
41. ✅ Floyd-Steinberg error diffusion dithering (high quality, ~0.1s overhead) — worth it for 4-16 colors — see section 12c — **File**: `palette/palette_dithering.c` (implemented 2026-02-16)
42. ✅ Grayscale and Workbench palette mapping for 4-8 color icons — see section 12d below — **File**: `palette/palette_grayscale.c` (implemented 2026-02-16)
43. ✅ Ultra quality mode GUI control (add to max colors chooser or separate checkbox) — see section 12e — **GUI integration only** (implemented 2026-02-16: added as last entry in Max Colors chooser)
44. ✅ Detail-preserving downsampling algorithm (preserve stars, sparkles, high-frequency details) — see section 12e — **File**: `palette/ultra_downsample.c` (implemented 2026-02-16)
45. ✅ Integrate Ultra with 256-color palette quantization (Median Cut, no dithering needed) — see section 12e — **Files**: `palette/palette_quantization.c`, `palette/ultra_downsample.c` (implemented 2026-02-16)
46. 🔲 Optional Auto mode logic (detect high-frequency details and auto-enable Ultra) — see section 12e — **File**: `palette/ultra_downsample.c` — deferred (nice-to-have)

---

## 22. Open Questions / Decisions Log

| # | Question | Decision | Date |
|---|----------|----------|------|
| 1 | Use iffparse.library or hand-rolled parser? | iffparse.library — proven, handles chunk nesting | 2026-02-10 |
| 2 | HAM/EHB support? | HAM: detect and stub. EHB: implement early (palette expansion is trivial) | 2026-02-10 |
| 3 | Palette strategy? | Two modes: PICTURE (initial) / SCREEN (future). ~~Originally via template ToolType~~ → Now via `deficons_palette_mode` GUI preference (deviation 13) | 2026-02-10 |
| 4 | Template border artwork remapping? | Not needed — icon uses Workbench default border drawing. Formalised as Template Contract rule | 2026-02-10 |
| 5 | Icon size selection mechanism? | Preference in LayoutPreferences + chooser in DefIcons GUI | 2026-02-10 |
| 6 | Interlace aspect correction? | Yes — halve display height when V_LACE detected | 2026-02-10 |
| 7 | Low-res pixel aspect correction? | Deferred — subtle effect, skip for initial implementation | 2026-02-10 |
| 8 | CAMG chunk parsing? | Yes — PropChunk alongside BMHD/CMAP for HAM/EHB/interlace | 2026-02-10 |
| 9 | Initial testing palette mode? | PICTURE mode (embed IFF's own CMAP) | 2026-02-10 |
| 10 | Hardcode size for initial testing? | Medium (64×64) as default. ~~All three templates created~~ → Templates eliminated, dimensions from constants (deviation 13) | 2026-02-10 |
| 11 | CMAP validation edge cases? | Validate missing/tiny/odd/oversized CMAP. Fall back to normal DefIcons if unusable | 2026-02-10 |
| 12 | Template pixel-art borders? | Forbidden in PICTURE mode — templates must be plain safe-area fill (Template Contract) | 2026-02-10 |
| 13 | Scaling performance on 68020? | Area-average default + 2×2 pre-filter bail-out for extreme ratios (>8:1 on large images) | 2026-02-10 |
| 14 | EHB implementation timing? | Phase A (with parsing) — low-hanging fruit, palette expansion + standard decode | 2026-02-10 |
| 15 | Future picture type expansion? | Datatype-based fallback path, opt-in via preference or flag (~~originally ITIDY_TT_USE_DATATYPES ToolType~~ — templates eliminated) | 2026-02-10 |
| 16 | Selected image contrast? | Add minimum contrast validation to ensure selection is visually distinct | 2026-02-10 |
| 17 | Integer vs float arithmetic? | Fixed-point (16.16) only — no float code paths on 68020 | 2026-02-10 |
| 18 | Template not palette-mapped? | ~~Fallback: create blank pixel/palette buffers from template dimensions~~ → Templates eliminated entirely. Blank buffers created from dimension constants (deviation 13) | 2026-02-10 |
| 19 | Safe area for IFF thumbnails? | Override to full image dimensions. Crop buffer to exact thumbnail size after rendering. Template safe area ToolType not required | 2026-02-10 |
| 20 | Transparency for IFF thumbnails? | Disabled (set to -1). Index 0 is often black and must be visible, not transparent | 2026-02-10 |
| 21 | Icon frame for IFF thumbnails? | Frameless (is_frameless=TRUE). Thumbnail IS the icon, no border needed. Workbench complement highlighting used for selection | 2026-02-10 |
| 22 | Selected image for IFF thumbnails? | Discarded. Edge-to-edge thumbnails have no safe area for bg/text index swap. Complement highlighting provides selection feedback | 2026-02-10 |
| 23 | Template pixel data needed? | No. ~~Template only needs valid dimensions~~ → Templates eliminated entirely. Dimensions from preference constants, palette mode from GUI chooser (deviation 13) | 2026-02-10 |
| 24 | Eliminate template .info files entirely? | Yes. Templates were vestigial — extraction always failed (old planar format), all ToolTypes were overridden, only dimensions were used. Replaced by `itidy_get_iff_icon_dimensions()` + `deficons_palette_mode` GUI preference. Three files (`iff_template_small/medium/large.info`) can be deleted from `Bin/Amiga/Icons/` | 2026-02-10 |
| 25 | HAM decoder: native or datatype? | Datatype fallback. Simpler, future-proof, handles HAM6/HAM8/JPEG/PNG automatically. Native parser tries first (fast path), datatype fallback on `ITIDY_PREVIEW_NOT_SUPPORTED`. | 2026-02-10 |
| 26 | Datatype RGB24 quantization palette? | 6×6×6 RGB color cube (216 colors) + grayscale ramp. Uniform color distribution gives better matching than source palette. Avoids black/white output from minimal default palettes. | 2026-02-10 |
| 27 | Datatype aspect ratio handling? | Same algorithm as native IFF: calculate destination size from `display_width × display_height` (aspect-corrected), scale from actual buffer size (`src_width × src_height`). Infer hires from width ≥400px, lace from height ≥400px (ModeID often incomplete). | 2026-02-10 |
| 28 | Hybrid native+datatype or datatype-only? | Hybrid. Native parser for 90% (faster, no dependencies), datatype fallback for HAM/exotic formats. Best of both: speed for common cases, completeness for edge cases. | 2026-02-10 |
| 29 | Max icon color count to reduce palette pressure? | Add user-configurable chooser (16/32/64/128/256). Post-render quantization step before save. Primarily benefits 8-bit screens loading HAM-derived icons. See section 12b. | 2026-02-13 |
| 30 | Dithering algorithm for low-color icons? | Three methods offered: None, Ordered (Bayer 4×4), Floyd-Steinberg, plus Auto option. Floyd-Steinberg reconsidered: one-time creation cost (~0.1s) is acceptable for quality improvement at 4-16 colors. Ordered remains fast default (16-32 colors). Auto selects best method based on color count. See section 12c. | 2026-02-15 |
| 31 | Order of operations: palette reduction vs dithering? | Palette reduction FIRST, then dither during pixel remapping. Dithering requires knowing the target palette to select neighboring colors effectively. See sections 12b and 12c. | 2026-02-15 |
| 32 | Color count options for max icon colors chooser? | 4, 8, 16, 32, 64, 128, "No limit (256 colours)". More granular than original spec to support extreme reduction for very low-depth screens. Dithering becomes critical at 16 and below. | 2026-02-15 |
| 33 | Generic palette to solve pen scavenging? | Add Mode 3: Generic Palette using fixed 6×6×6 cube (216 colors) + grayscale ramp. All icons share the same palette → Workbench allocates pens once → no scavenging on 32-128 color screens. Already implemented for datatype path. See section 11, Mode 3. | 2026-02-15 |
| 34 | Floyd-Steinberg worth the performance cost? | YES for one-time icon creation. Adds ~0.1s per icon but produces dramatically better quality at 4-8 colors. Workbench displays pre-dithered icons instantly (no runtime remapping). Benefits Workbench Quality setting. See section 12c performance analysis. | 2026-02-15 |
| 35 | Grayscale vs Workbench palette for 4-8 colors? | Offer both via chooser: Grayscale (consistent, theme-independent), Workbench palette 0-7 (color preservation, theme integration), Hybrid (grays + WB accent colors). Pre-mapping to stable colors eliminates Workbench display-time remapping (instant display). See section 12d. | 2026-02-15 |
| 36 | Ultra quality mode for detail preservation? | Add Ultra mode (256 colors + detail-preserving downsampling). Preserves small bright details (stars in night sky) lost in traditional area-averaging. Uses full 256-color palette (not reduced). Similar file size to standard mode (~3-5 KB for 64×64). Best displayed on RTG screens for accurate color rendering. See section 12e. | 2026-02-15 |
| 37 | Detail-preserving algorithm: how to detect stars/sparkles? | Scan each downsampled region for maximum brightness. If max brightness > 3× average brightness, boost output by 30% to preserve the detail. Uses perceptual luminance (ITU-R BT.601). Threshold and boost % are tunable constants. See section 12e implementation. | 2026-02-15 |
| 38 | Ultra mode palette generation? | Generate optimal 256-color palette from detail-preserved RGB buffer using Median Cut. No dithering needed with 256 colors. File size similar to standard mode (palette is 256×3 = 768 bytes vs 64×3 = 192 bytes, negligible difference with RLE). Still 8-bit palette-based icon (AmigaOS constraint). | 2026-02-15 |
| 39 | Ultra mode interaction with other settings? | When Ultra enabled: override max icon colors to 256 (full palette), disable dithering (not needed with 256 colors), ignore lowcolor mapping (N/A above 8 colors). Palette mode (PICTURE vs SCREEN) still applies. Ultra = 256 colors + detail-preserving algorithm. | 2026-02-15 |
| 40 | Auto mode: when to select Ultra automatically? | Check image characteristics: if >1% of pixels are bright isolated details (stars/sparkles), enable Ultra. Fast pre-scan (~0.01s) detects high-frequency content. No RTG/screen depth checks needed (Ultra works on any screen, just displays better on RTG). Otherwise use standard 64-color mode. See section 12e `should_use_ultra_mode()` logic. | 2026-02-15 |
| 41 | Should Ultra mode warn about Chipset display? | Optional: display info tooltip "Best for RTG screens" but don't block usage. 256-color icons work on Chipset but may cause palette remapping overhead (pen scavenging). User can choose between Ultra (quality) or reduced colors (Chipset compatibility). No mandatory RTG detection needed. | 2026-02-15 |
| 42 | Ultra mode performance vs standard 64-color? | Similar or slightly faster! Detail-preserving scan adds ~0.01-0.02s. Palette generation to 256 colors takes ~0.03s (vs 0.02s for 64 colors). No dithering needed (saves 0.05s). Net: ~0.17-0.19s vs ~0.20s for 64-color + dither. Comparable speed with better quality. See section 12e performance table. | 2026-02-15 || 43 | Ultra + Dithering workflow: Should GUI auto-enable dithering? | When Ultra mode + low color count (<64) selected, should dithering be automatically enabled (override "Ultra disables dithering" rule)? Or leave it manual? Auto-enable is user-friendly (preserves details THEN dithers them), but may confuse users expecting Ultra = no dither. Recommend: Auto-enable with tooltip explaining the pipeline. See section 12e "Ultra + Dithering Workflow". | 2026-02-15 |
| 44 | Ultra + Dithering: Order-of-operations critical? | YES. Must preserve details FIRST (Ultra downsample), THEN reduce colors + dither. Standard pipeline averages away hair strands/stars, then dithering just patterns the blur. Ultra + Dither pipeline keeps details during downsampling, then dithers them into 16-32 color patterns (strands visible!). Key insight: Dithering can only dither what's in the image. If details averaged away, dithering can't restore them. See section 12e comparison table. | 2026-02-15 |
| 45 | Ultra + Dithering performance impact? | Slightly slower than Ultra alone: detail-scan ~0.01-0.02s, palette generation to 16-32 colors ~0.01s, dithering ~0.05s. Total ~0.23-0.26s vs ~0.17-0.19s for Ultra alone. But produces Chipset-compatible 16-32 color icons with preserved texture (hair strands, fabric weave) that standard 16-color + dither cannot achieve. Worth the extra ~0.06s for OCS artwork use case. See section 12e. | 2026-02-15 |