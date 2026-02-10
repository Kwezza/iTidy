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
| E: Polish & Future | 🔲 Not started | HAM support, screen palette mode implementation, datatype fallback |

### Key Deviations From Original Plan

1. **Template icon format problem** (Section 12 deviation): The `iff_template_medium.info` file is an old-format planar icon, NOT OS 3.5 palette-mapped. `itidy_icon_image_extract()` failed on it. **Fix**: Added a fallback path in `apply_iff_preview()` that creates blank pixel/palette buffers from the template's dimensions when extraction fails. Since PICTURE palette mode replaces the entire palette and overwrites all safe-area pixels, blank buffers work correctly.

2. **Safe area abandoned for IFF thumbnails** (Section 12 deviation): The original plan used template safe areas to constrain the thumbnail. In practice, source IFF images can be any size and aspect ratio, causing the thumbnail to float inside the icon frame with wasted space. **Fix**: The safe area is overridden to `(0, 0, img.width, img.height)` — the full template dimensions — and after rendering, the pixel buffer is cropped to the exact thumbnail size. This produces tight-fitting icons that match the content.

3. **Transparency disabled**: Palette index 0 (often black) was being treated as transparent by Workbench, causing holes in dark images like Space and Waterfall. **Fix**: `transparent_color_normal` and `transparent_color_selected` are set to -1 (no transparency) after cropping.

4. **Frameless icons**: Since the thumbnail IS the icon (edge-to-edge, no border artwork), `is_frameless` is set to TRUE. Workbench draws no border frame around the icon.

5. **Selected image discarded**: Edge-to-edge thumbnails have no safe area for the bg/text index swap to operate on, so the selected image building step is skipped. The icon relies on Workbench's standard complement highlighting for selection feedback.

6. **`itidy_find_closest_palette_color()`**: Promoted from static in `icon_text_render.c` to shared in `icon_image_access.c/.h` with `itidy_` prefix, as planned.

7. **EHB implemented in Phase A**: As planned, EHB support was included early via `expand_ehb_palette()` in the core parsing phase. Standard decode works after palette expansion.

8. **`iTidy_IFFRenderParams` extended**: Four output dimension fields were added post-plan: `output_width`, `output_height`, `output_offset_x`, `output_offset_y`. These are populated by `itidy_render_iff_thumbnail()` and used by the caller to crop the pixel buffer.

9. **`itidy_icon_image_create_blank()`**: Already existed in `icon_image_access.c/.h` — used as the fallback for non-palette-mapped templates.

10. **Hires aspect ratio correction missing** (Step 9 bug): The original implementation only corrected for interlace (halving `display_height`) but never corrected for hires (halving `display_width`). Amiga hires pixels are half the width of lowres pixels, so a 672-pixel-wide hires image should display at the same physical width as 336 lowres pixels. Without this correction, hires and hires+interlace images produced horizontally squished thumbnails (e.g. Zebra-Stripes-V2.IFF at 672×446 hires+lace was producing a 64×21 thumbnail instead of ~64×42). **Fix**: Added hires width correction in Step 9 — when `is_hires` is TRUE, `display_width = bmhd.w / 2`. This is independent of the interlace height correction, so hires+lace images get both halved.

11. **Missing HIRES CAMG flag in many IFF files** (Step 6b addition): Many old Amiga IFF tools (particularly those producing EHB images) wrote incomplete CAMG chunks — setting interlace and EHB flags but omitting the HIRES flag, even for 672-pixel-wide images that are clearly hires resolution. This caused the hires width correction from item 10 to not trigger. **Fix**: Added width-based hires inference as a fallback (Step 6b). After CAMG decoding, if `is_hires` is FALSE but `bmhd.w > ITIDY_HIRES_WIDTH_THRESHOLD` (400 pixels — lowres maxes out at ~376 with extreme overscan), `is_hires` is set to TRUE and the inference is logged. The threshold constant is defined in `icon_iff_render.h`.

12. **Combined hires+interlace+EHB** (test case validation): Images like Utopia-10.IFF and Utopia-13.IFF (672×442, 6 planes, EHB, interlace, missing HIRES flag) exercise all three fixes together: EHB palette expansion (item 7), hires inference from width (item 11), and dual aspect ratio correction (item 10). After all fixes, these produce correct ~64×42 thumbnails with proper proportions.

13. **IFF template icons removed** (template elimination refactor): The three `iff_template_small/medium/large.info` files turned out to be vestigial. They were old-format planar icons — `itidy_icon_image_extract()` always failed on them, falling back to blank pixel buffers. Their ToolTypes (safe area, bg/text color, palette mode) were all overridden immediately after reading because IFF thumbnails are edge-to-edge with no safe area margins. The templates only contributed icon dimensions, which are trivially derived from the size preference constant. **Refactor**: Removed all template loading, extraction, and ToolType parsing from `apply_iff_preview()`. Icon dimensions now come directly from `itidy_get_iff_icon_dimensions()` which maps the preference to pixel sizes (48/64/100). Palette mode now comes from a new `deficons_palette_mode` field in `LayoutPreferences` with a chooser gadget in the DefIcons settings window. Removed: `itidy_get_iff_template_path()`, `itidy_get_iff_render_params()`, `ITIDY_IFF_TEMPLATE_*` constants, `ITIDY_TT_PALETTE_MODE` constant. Added: `itidy_get_iff_icon_dimensions()`, `ITIDY_IFF_SIZE_*` pixel constants, `deficons_palette_mode` preference field, palette mode chooser gadget. The `iff_template_*.info` files in `Bin/Amiga/Icons/` can now be deleted.

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

#### Mode 2: Screen Palette (`ITIDY_PALETTE_MODE=SCREEN`) — Stubbed for Future

Quantize the IFF image's colors to match the current Workbench screen palette. This produces icons that display instantly without remapping.

**Status:** Stubbed. Will log "screen palette mode not yet implemented" and fall back to picture palette mode.

**Future implementation notes:**
- Read Workbench screen palette via `GetRGB32()` on the screen's `ViewPort.ColorMap`
- For each source pixel's RGB value, find the closest Workbench palette entry
- May need dithering (ordered or Floyd-Steinberg) to compensate for limited palette
- Consider using the existing `expand_palette_for_adaptive()` infrastructure

### ~~New ToolType~~ → GUI Preference (Deviation 13):

> **SUPERSEDED**: The palette mode was originally specified via a `ITIDY_PALETTE_MODE` ToolType on the template icon. Since templates have been eliminated (deviation 13), the palette mode is now stored as a `deficons_palette_mode` field in `LayoutPreferences` and controlled by a chooser gadget in the DefIcons settings window.

```c
/* In layout_preferences.h */
UWORD deficons_palette_mode;  /* 0=Picture (use image CMAP), 1=Screen (quantize to WB screen) */
#define DEFAULT_DEFICONS_PALETTE_MODE  0  /* Picture mode */

/* In deficons_settings_window.c */
static STRPTR palette_mode_labels[] = {
    "Picture (original colors)",
    "Screen (match Workbench)",
    NULL
};
```

~~Constant: `ITIDY_TT_PALETTE_MODE` = `"ITIDY_PALETTE_MODE"`~~ — Removed from `icon_iff_render.h`

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

- **Border**: None — icon is frameless (`is_frameless = TRUE`). Workbench complement highlighting used for selection
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

### HAM (Hold-And-Modify) — Stubbed for Future

HAM6 (6 bitplanes) and HAM8 (8 bitplanes) use a stateful per-scanline decoder where each pixel either sets a palette color or modifies one RGB component of the previous pixel. This produces near-true-color images from limited bitplane counts.

**Initial implementation**: Detect HAM via CAMG flags (`camg & 0x0800`). If HAM detected:
- Log: `"HAM image detected — thumbnail not yet supported: %s"`
- Return `ITIDY_PREVIEW_NOT_SUPPORTED`
- Stub function `decode_ham_to_rgb()` with TODO comment for future implementation

**Future HAM implementation notes:**
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

### Phase E: Polish and Future — 🔲 NOT STARTED

34. 🔲 HAM support (complex — per-scanline state machine + RGB→palette quantization)
35. 🔲 Screen palette mode implementation (read Workbench palette via `GetRGB32()`, quantize with optional dithering) — GUI chooser already exists
36. 🔲 Datatype-based fallback for non-ILBM picture types (Section 15)
37. 🔲 Low-res pixel aspect ratio correction (subtle)
38. 🔲 Consider: selected image for frameless thumbnails (complement highlight may suffice)

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
