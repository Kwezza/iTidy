# IFF ILBM Thumbnail Renderer — Implementation Plan

**Date**: 2026-02-10  
**Status**: Planning  
**Module**: `src/icon_edit/icon_iff_render.c/.h`  
**Depends on**: Completed ASCII text renderer (Phase 1–3 of Icon_Image_Editing_Plan.md)

---

## 1. Overview

Add an IFF ILBM image thumbnail renderer to the existing icon_edit pipeline, reusing the proven two-icon-merge architecture and `iTidy_RenderParams` infrastructure. The renderer will:

1. Parse IFF ILBM files via `iffparse.library`
2. Decode interleaved bitplanes to a chunky pixel buffer
3. Correct aspect ratio for interlace images using CAMG viewport flags
4. Area-average downscale to fit the icon template's safe area
5. Handle palette via two user-selectable modes (picture palette / screen palette)
6. Produce OS 3.5 format icons via the existing apply→stamp→save pipeline

Three icon size tiers are supported (48×48, 64×64, 100×100) via separate template icons, with a user preference chooser in the DefIcons settings GUI.

---

## 2. Architecture — How It Fits Into Existing Code

### Existing Pipeline (from ASCII text renderer):

```
Type detection → Load target icon (metadata) + Load image template (pixels)
→ Extract image data → Build render params → Call renderer → Build selected image
→ Debug dump → Apply to clone → Stamp ToolTypes → Save OS 3.5 → Cleanup
```

### IFF Thumbnail Addition:

The IFF renderer slots in as a **parallel renderer** alongside the text renderer. The dispatch in `icon_content_preview.c` gains a new branch:

```
if (itidy_is_text_preview_type(type_token))
    → text renderer pipeline (existing)
else if (itidy_is_iff_preview_type(type_token))
    → IFF thumbnail pipeline (NEW)
else
    → ITIDY_PREVIEW_NOT_APPLICABLE
```

Everything after the renderer call (selected image, apply, stamp, save) is **shared unchanged**.

### Key Reused Components:

| Component | Location | Reuse |
|-----------|----------|-------|
| `iTidy_IconImageData` | `icon_image_access.h` | Holds template + output pixel/palette data |
| `iTidy_RenderParams` | `icon_image_access.h` | Base struct for safe area, buffer, colors |
| `itidy_icon_image_extract()` | `icon_image_access.c` | Extract pixels from image template |
| `itidy_icon_image_apply()` | `icon_image_access.c` | Apply rendered pixels to output icon |
| `itidy_icon_image_save()` | `icon_image_access.c` | Save as OS 3.5 format |
| `itidy_icon_image_free()` | `icon_image_access.c` | Free pixel/palette memory |
| `itidy_resolve_palette_indices()` | `icon_image_access.c` | Auto-detect bg/text/mid colors |
| `itidy_stamp_created_tooltypes()` | `icon_image_access.c` | Stamp ITIDY_CREATED/KIND/SRC |
| `itidy_check_cache_valid()` | `icon_image_access.c` | Skip unchanged files |
| `itidy_icon_image_dump()` | `icon_image_access.c` | Debug hex grid output |
| `build_selected_image()` | `icon_content_preview.c` | Safe-area index swap for selected state |
| `find_closest_palette_color()` | Promote to `icon_image_access.c` (currently static in `icon_text_render.c`) | Euclidean RGB palette matching |
| `ITIDY_KIND_IFF_THUMBNAIL` | `icon_image_access.h` | Already defined as `"iff_thumbnail"` |

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

| File | Changes |
|------|---------|
| `src/icon_edit/icon_image_access.h` | Add `find_closest_palette_color()` prototype, new IFF ToolType constants |
| `src/icon_edit/icon_image_access.c` | Promote `find_closest_palette_color()` from `icon_text_render.c` |
| `src/icon_edit/icon_text_render.c` | Remove static `find_closest_palette_color()`, call shared version |
| `src/icon_edit/icon_content_preview.h` | Add IFF template path constants |
| `src/icon_edit/icon_content_preview.c` | Add IFF type detection, dispatch branch, template path selection |
| `src/layout_preferences.h` | Add `deficons_icon_size_mode` preference field |
| `src/layout_preferences.c` | Load/save icon size preference |
| `src/GUI/deficons_settings_window.c` | Add icon size chooser gadget |
| `Makefile` | Add `icon_iff_render.c` to `ICON_EDIT_SRCS` and dependency rule |

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
} iTidy_IFFRenderParams;
```

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

### New ToolType:

```
ITIDY_PALETTE_MODE=PICTURE    (default — use IFF's own CMAP)
ITIDY_PALETTE_MODE=SCREEN     (future — quantize to Workbench palette)
```

Constant: `ITIDY_TT_PALETTE_MODE` = `"ITIDY_PALETTE_MODE"`

---

## 12. IFF Template Contract

**⚠️ HARD RULES** for IFF thumbnail template icons. These are non-negotiable constraints that keep the rendering pipeline correct and predictable.

### Rule 1: Safe Area Must Be Defined

Every IFF template icon **must** include the `ITIDY_TEXT_AREA=x,y,w,h` ToolType defining the rectangular area where the thumbnail will be rendered. Without this, the renderer falls back to the 4px-margin default, which may not match the template's visual design.

### Rule 2: No Custom Pixel-Art Borders

IFF templates **must not** rely on custom pixel-art borders or decorative frame artwork stored in the pixel data. In `ITIDY_PALETTE_MODE=PICTURE`, the template's entire palette is replaced with the source image's CMAP. Any pixel-art drawn at specific palette indices will display with wrong colors after palette replacement.

**Instead**: Templates should be plain safe-area fill (single `ITIDY_BG_COLOR` fill covering the entire pixel area). The icon's visible border/frame is handled by Workbench's standard icon border rendering (`GFLG_GADGHCOMP` or similar), which is drawn by Intuition at display time and is independent of the icon's pixel palette.

### Rule 3: Background Index Must Be Defined

Every template **must** include `ITIDY_BG_COLOR=N` specifying the palette index used for:
- Filling the safe area before rendering
- Letterbox/pillarbox bars when the image aspect ratio doesn't fill the safe area

This index will be remapped to the closest match in the source image's palette during rendering.

### Rule 4: Palette Mode Must Be Specified

Every template **must** include `ITIDY_PALETTE_MODE=PICTURE` (or `SCREEN` when supported). This makes the rendering mode explicit and self-documenting. The default if missing is `PICTURE`.

### Rule 5: Templates Are Size-Specific

Each template file is designed for one specific icon dimension. The output icon will be exactly the template's pixel dimensions. There is no runtime scaling of the template itself.

### Minimum Valid IFF Template ToolTypes:

```
ITIDY_TEXT_AREA=4,4,40,40
ITIDY_BG_COLOR=0
ITIDY_PALETTE_MODE=PICTURE
```

### Why These Rules Matter:

The two-icon-merge architecture means the template provides **all pixel data** (dimensions, palette, pixel buffer) while the target Workbench icon provides only **metadata** (DefaultTool, ToolTypes, icon type). When the renderer replaces the palette (PICTURE mode), any assumption about palette index → color mapping in the template's pixels is invalidated. Keeping templates as plain fills with Workbench-drawn borders eliminates this class of bug entirely.

---

## 13. Icon Size Tiers

### Three Size Options:

| Size | Template File | Dimensions | Safe Area (approx) | Use Case |
|------|--------------|------------|--------------------|----|
| Small | `iff_template_small.info` | 48×48 | ~40×40 | Low-res screens, many icons |
| Medium | `iff_template_medium.info` | 64×64 | ~56×56 | Default, good balance |
| Large | `iff_template_large.info` | 100×100 | ~92×92 | Hi-res screens, detail |

### Template Icon Design:

- **Border**: Handled by Workbench's default icon border rendering (NOT stored in pixel data). Template icon has standard `GFLG_GADGHCOMP` (complement highlighting) or similar. No custom frame artwork to remap
- **Safe area**: The entire pixel area minus a small margin, specified via `ITIDY_TEXT_AREA` ToolType
- **Background**: Flat fill color specified by `ITIDY_BG_COLOR` (used for letterbox/pillarbox fill)
- **Palette mode**: `ITIDY_PALETTE_MODE=PICTURE` (default)
- **Each template is a separately crafted `.info` file** with its own dimensions, safe area, and ToolTypes

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

A chooser gadget will be added to the DefIcons settings GUI (`src/GUI/deficons_settings_window.c`).

### Template Path Selection:

```c
/* In icon_content_preview.h */
#define ITIDY_IFF_TEMPLATE_SMALL   "PROGDIR:Icons/iff_template_small"
#define ITIDY_IFF_TEMPLATE_MEDIUM  "PROGDIR:Icons/iff_template_medium"
#define ITIDY_IFF_TEMPLATE_LARGE   "PROGDIR:Icons/iff_template_large"

/* In icon_content_preview.c — select template based on preference */
const char *get_iff_template_path(void)
{
    const LayoutPreferences *prefs = GetGlobalPreferences();
    switch (prefs->deficons_icon_size_mode)
    {
        case ITIDY_ICON_SIZE_SMALL:  return ITIDY_IFF_TEMPLATE_SMALL;
        case ITIDY_ICON_SIZE_LARGE:  return ITIDY_IFF_TEMPLATE_LARGE;
        default:                     return ITIDY_IFF_TEMPLATE_MEDIUM;
    }
}
```

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
- **Advanced toggle** (future): `ITIDY_TT_USE_DATATYPES=YES` on the template icon
  - Opens the source file via `picture.datatype` (`NewDTObjectA()`)
  - Obtains decoded bitmap/pixel data via `PDTA_BitMap` or `PDTA_DestBitMap`
  - Uses `PDTA_ModeID` for CAMG-equivalent mode detection
  - Feeds decoded pixels into the same scaling/rendering pipeline
  - Would match any type under the `"picture"` category

This approach would give iTidy thumbnail support for **any format with an installed datatype**, without needing format-specific decoders. However, it introduces a dependency on the datatype being present and working correctly. Keep this as a clearly separate code path activated by an opt-in ToolType, not a silent upgrade.

---

## 16. Complete Rendering Pipeline

### IFF Thumbnail Pipeline (step-by-step):

```
 1. itidy_is_iff_preview_type(type_token)       → Verify type is "ilbm"
 2. itidy_check_cache_valid(path, ITIDY_KIND_IFF_THUMBNAIL, size, date)
                                                  → Skip if icon already up-to-date
 3. GetDiskObject(source_path)                    → Load TARGET icon (Workbench metadata)
 4. get_iff_template_path()                       → Select template by size preference
 5. GetIconTagList(template_path,
       ICONGETA_RemapIcon=FALSE,
       ICONGETA_GenerateImageMasks=FALSE)          → Load IMAGE TEMPLATE (pixel data)
 6. itidy_icon_image_extract(image_icon, &img)    → Copy pixels+palette from template
 7. itidy_get_iff_render_params(image_icon, &img, &iff_params)
                                                   → Build render params from ToolTypes
 8. FreeDiskObject(image_icon)                     → Done with template DiskObject
 9. itidy_render_iff_thumbnail(source_path, &iff_params, &img)
                                                   → Parse IFF, decode, scale, render
10. build_selected_image(&img, &params.base)       → Safe-area index swap for selected
10b. validate_selected_contrast(&img, &params.base) → Ensure selection is visually distinct
11. itidy_icon_image_dump(&img, &params.base, path) → Debug hex grid (if enabled)
12. itidy_icon_image_apply(target_icon, &img)      → Clone target, apply rendered pixels
13. itidy_stamp_created_tooltypes(clone, ITIDY_KIND_IFF_THUMBNAIL, ...)
                                                   → Stamp ITIDY_CREATED/KIND/SRC
14. itidy_icon_image_save(source_path, clone)      → Save as OS 3.5 format
15. Cleanup: FreeDiskObject(clone) → itidy_icon_image_free(&img) → FreeDiskObject(target)
```

### Inside `itidy_render_iff_thumbnail()` (step 9 expanded):

```
 9a. Open iffparse.library
 9b. AllocIFF → InitIFFasDOS → Open file → OpenIFF
 9c. PropChunk(BMHD, CMAP, CAMG) + StopChunk(BODY)
 9d. ParseIFF(IFFPARSE_SCAN)
 9e. FindProp(BMHD) → validate, extract dimensions/depth/compression
 9f. FindProp(CAMG) → decode viewport flags (HAM/EHB/interlace/hires)
 9g. If HAM or EHB → cleanup, return ITIDY_PREVIEW_NOT_SUPPORTED
 9h. FindProp(CMAP) → copy palette to iff_params.src_palette
 9i. Allocate src_chunky buffer (src_width * src_height bytes)
 9j. ReadChunkBytes(BODY) into compressed buffer
 9k. Decompress ByteRun1 (if compression=1) row by row
 9l. Convert interleaved bitplanes to chunky pixels
 9m. CloseIFF → Close file → FreeIFF → CloseLibrary
 9n. Calculate display dimensions (halve height if interlace)
 9o. Calculate scaling to fit safe area, preserving aspect ratio
 9p. Fill safe area with bg_color_index
 9q. If palette_mode == PICTURE:
       - Replace img->palette_normal with src_palette
       - Area-average scale src_chunky → pixel_buffer (direct index write)
 9r. If palette_mode == SCREEN:
       - Stub: log warning, fall back to PICTURE mode
 9s. Free src_chunky buffer
 9t. Return ITIDY_PREVIEW_OK
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

## 18. New ToolType Constants

### Added to `icon_image_access.h`:

| Constant | String | Purpose |
|----------|--------|---------|
| `ITIDY_TT_PALETTE_MODE` | `"ITIDY_PALETTE_MODE"` | Palette mode selection (PICTURE/SCREEN) |

### Template Icon ToolType Examples:

**iff_template_medium.info ToolTypes:**
```
ITIDY_TEXT_AREA=4,4,56,56
ITIDY_BG_COLOR=0
ITIDY_PALETTE_MODE=PICTURE
```

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
8. **Palette replacement**: Verify template border handled by Workbench, safe area shows correct colors
9. **Cache validation**: Render once, re-run — verify skip on unchanged source
10. **Selected image**: Verify selected state shows safe-area index swap
11. **Debug dump**: Verify hex grid output shows expected pixel indices

### Testing Workflow:

1. Copy test images to a test folder on the Amiga
2. Configure DefIcons to recognize `.iff` / `ilbm` type files
3. Run iTidy on the test folder
4. Inspect generated `.info` files visually on Workbench
5. Check `Bin/Amiga/logs/` for errors and debug dumps
6. Compare thumbnail appearance across all three size tiers

---

## 21. Implementation Order

### Phase A: Core IFF Parsing (get data out of the file)

1. Create `icon_iff_render.h` with all structs and prototypes
2. Create `icon_iff_render.c` with IFF parsing (iffparse.library)
3. Implement ByteRun1 decompression
4. Implement bitplane→chunky conversion
5. Implement CAMG detection (interlace, HAM, EHB)
6. Add HAM stub (detect and skip with log message)
7. **Implement EHB support** — palette expansion via `expand_ehb_palette()`, then standard decode (low-hanging fruit, completes before Phase B scaling)
8. Implement CMAP validation (Section 11 edge cases)
9. Test standalone parsing with debug output

### Phase B: Scaling and Rendering (get pixels into the icon)

10. Implement interlace height correction
11. Implement area-average downscaling with aspect ratio preservation
12. Add scaling fast-path with 2×2 pre-filter for extreme ratios (Section 10)
13. Implement picture palette mode (replace palette, write indices)
14. Stub screen palette mode
15. Promote `find_closest_palette_color()` to shared
16. Test rendering into a template pixel buffer

### Phase C: Integration (wire into the pipeline)

17. Add `itidy_is_iff_preview_type()` to dispatch
18. Add IFF branch to `icon_content_preview.c`
19. Define template path constants
20. Update Makefile
21. Create initial template icons (plain safe-area fill per Template Contract)
22. Add selected image contrast validation (step 10b in pipeline)
23. Test end-to-end with test images

### Phase D: Size Selection UI

24. Add `deficons_icon_size_mode` to `LayoutPreferences`
25. Add size chooser to DefIcons settings GUI
26. Create all three template icon sizes
27. Test size switching

### Phase E: Polish and Future

28. HAM support (complex — per-scanline state machine + RGB→palette quantization)
29. Screen palette mode (read Workbench palette, quantize with optional dithering)
30. Datatype-based fallback for non-ILBM picture types (Section 15)
31. Low-res pixel aspect ratio correction (subtle)

---

## 22. Open Questions / Decisions Log

| # | Question | Decision | Date |
|---|----------|----------|------|
| 1 | Use iffparse.library or hand-rolled parser? | iffparse.library — proven, handles chunk nesting | 2026-02-10 |
| 2 | HAM/EHB support? | HAM: detect and stub. EHB: implement early (palette expansion is trivial) | 2026-02-10 |
| 3 | Palette strategy? | Two modes via ToolType: PICTURE (initial) / SCREEN (future) | 2026-02-10 |
| 4 | Template border artwork remapping? | Not needed — icon uses Workbench default border drawing. Formalised as Template Contract rule | 2026-02-10 |
| 5 | Icon size selection mechanism? | Preference in LayoutPreferences + chooser in DefIcons GUI | 2026-02-10 |
| 6 | Interlace aspect correction? | Yes — halve display height when V_LACE detected | 2026-02-10 |
| 7 | Low-res pixel aspect correction? | Deferred — subtle effect, skip for initial implementation | 2026-02-10 |
| 8 | CAMG chunk parsing? | Yes — PropChunk alongside BMHD/CMAP for HAM/EHB/interlace | 2026-02-10 |
| 9 | Initial testing palette mode? | PICTURE mode (embed IFF's own CMAP) | 2026-02-10 |
| 10 | Hardcode size for initial testing? | Medium (64×64) as default, all three templates created | 2026-02-10 |
| 11 | CMAP validation edge cases? | Validate missing/tiny/odd/oversized CMAP. Fall back to normal DefIcons if unusable | 2026-02-10 |
| 12 | Template pixel-art borders? | Forbidden in PICTURE mode — templates must be plain safe-area fill (Template Contract) | 2026-02-10 |
| 13 | Scaling performance on 68020? | Area-average default + 2×2 pre-filter bail-out for extreme ratios (>8:1 on large images) | 2026-02-10 |
| 14 | EHB implementation timing? | Phase A (with parsing) — low-hanging fruit, palette expansion + standard decode | 2026-02-10 |
| 15 | Future picture type expansion? | Datatype-based fallback path, opt-in via ITIDY_TT_USE_DATATYPES ToolType | 2026-02-10 |
| 16 | Selected image contrast? | Add minimum contrast validation to ensure selection is visually distinct | 2026-02-10 |
| 17 | Integer vs float arithmetic? | Fixed-point (16.16) only — no float code paths on 68020 | 2026-02-10 |
