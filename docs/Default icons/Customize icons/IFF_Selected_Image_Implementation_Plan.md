# IFF Thumbnail Selected Image Support — Implementation Plan

**Date**: 2026-02-11  
**Status**: 🔲 Planning  
**Relates to**: `IFF_Thumbnail_Implementation_Plan.md`  
**Module**: `src/icon_edit/icon_iff_render.c/.h`

---

## Overview

Extend the IFF thumbnail renderer to create **selected state images** (the second image in an Amiga icon) by rendering the source IFF with modified palette colors. This enables visual feedback when the icon is selected in Workbench without relying on Workbench's standard complement highlighting.

### Current Behavior (v2.0)

The IFF thumbnail system (implemented in Phase A-D of `IFF_Thumbnail_Implementation_Plan.md`) currently:
- Renders **only the normal state** image (`pixel_data_normal`)
- **Explicitly discards** the selected state image (`pixel_data_selected` is freed and set to NULL)
- Rationale: "Edge-to-edge thumbnails have no safe area for the bg/text index swap to operate on"

See `icon_content_preview.c`, lines 598-605:
```c
// Discard selected image — edge-to-edge thumbnails have no safe
// area for the bg/text index swap to operate on
if (img.pixel_data_selected != NULL)
{
    whd_free(img.pixel_data_selected);
    img.pixel_data_selected = NULL;
}
img.has_selected_image = FALSE;
img.has_real_selected_image = FALSE;
```

### Problem Statement

The current "bg/text index swap" approach (used for text icons) doesn't work for IFF thumbnails because:
1. IFF thumbnails are **edge-to-edge** (no border artwork with safe area)
2. The palette is **entirely from the source IFF** (PICTURE mode)
3. There's no predefined "background" and "text" color to swap

However, we still want selected state images because:
- The template icon (`text_template.info`) has a dimmed selected image
- Users expect visual feedback when icons are selected
- Relying solely on Workbench complement may not provide sufficient contrast

---

## Proposed Solution: Render with Modified Palette

Instead of swapping palette indices, **render the image twice** with different palettes:

1. **Normal image**: Render with the original IFF CMAP palette
2. **Selected image**: Render with a **modified palette** (dimmed, desaturated, or complemented colors)

### Key Insight: Reuse `area_average_scale()`

The `area_average_scale()` function already supports rendering with different palettes:
```c
static void area_average_scale(const UBYTE *src_chunky,
                               UWORD src_w, UWORD src_h,
                               UBYTE *dest_buf, UWORD dest_w, UWORD dest_h,
                               UWORD dest_stride,
                               UWORD dest_offset_x, UWORD dest_offset_y,
                               const struct ColorRegister *src_palette,     // ← Input colors
                               ULONG src_palette_size,
                               const struct ColorRegister *dest_palette,    // ← Output colors
                               ULONG dest_palette_size);
```

**Current usage** (normal image):
- `src_palette` = IFF CMAP palette (original colors)
- `dest_palette` = same IFF CMAP palette (PICTURE mode)

**New usage** (selected image):
- `src_palette` = IFF CMAP palette (original colors for averaging)
- `dest_palette` = **dimmed** IFF CMAP palette (modified colors for output)

The function will:
1. Average source pixels using the original palette (correct color math)
2. Match the averaged RGB to the **closest entry in the dimmed palette**
3. Write the dimmed palette index to `pixel_data_selected`

---

## Implementation Steps

### Step 1: Add Palette Modification Functions

Create helper functions to generate modified palettes for selected images:

```c
/**
 * @brief Create a dimmed version of a palette.
 *
 * Each RGB component is multiplied by a factor (e.g., 0.6) to create
 * a darker version. This mimics the dimmed appearance of the
 * text_template.info selected image.
 *
 * @param src_palette     Source palette to dim
 * @param src_size        Number of entries in source palette
 * @param dim_factor_16   Dimming factor in fixed-point 16.16 (e.g., 0.6 = 0x9999)
 * @return Allocated dimmed palette (caller must whd_free), or NULL on error
 */
struct ColorRegister* itidy_palette_create_dimmed(
    const struct ColorRegister *src_palette,
    ULONG src_size,
    ULONG dim_factor_16);

/**
 * @brief Create a desaturated (grayscale) version of a palette.
 *
 * Each color is converted to grayscale using luminance weights:
 * gray = 0.299*R + 0.587*G + 0.114*B
 *
 * @param src_palette     Source palette to desaturate
 * @param src_size        Number of entries in source palette
 * @return Allocated desaturated palette (caller must whd_free), or NULL on error
 */
struct ColorRegister* itidy_palette_create_desaturated(
    const struct ColorRegister *src_palette,
    ULONG src_size);

/**
 * @brief Create a complemented version of a palette.
 *
 * Each RGB component is inverted: new = 255 - old
 *
 * @param src_palette     Source palette to complement
 * @param src_size        Number of entries in source palette
 * @return Allocated complemented palette (caller must whd_free), or NULL on error
 */
struct ColorRegister* itidy_palette_create_complemented(
    const struct ColorRegister *src_palette,
    ULONG src_size);
```

**Location**: `src/icon_edit/icon_iff_render.c` (static functions) or `icon_image_access.c` (if shared)

**Default mode**: Use **dimming** (60-70% brightness) to match `text_template.info` appearance.

---

### Step 2: Add Selected Image Mode Preference

Add a user preference to control selected image generation:

```c
/* In src/layout_preferences.h */

/** IFF thumbnail selected image mode */
typedef enum {
    ITIDY_SELECTED_NONE = 0,         /* No selected image (Workbench complement) */
    ITIDY_SELECTED_DIMMED = 1,       /* Dimmed palette (default) */
    ITIDY_SELECTED_DESATURATED = 2,  /* Grayscale palette */
    ITIDY_SELECTED_COMPLEMENTED = 3  /* Inverted colors */
} iTidy_IFFSelectedMode;

/* Add to LayoutPreferences struct */
UWORD deficons_iff_selected_mode;  /* Default: ITIDY_SELECTED_DIMMED */
```

**Default**: `DEFAULT_DEFICONS_IFF_SELECTED_MODE = ITIDY_SELECTED_DIMMED`

**UI**: Add a chooser gadget to `src/GUI/deficons_settings_window.c` with labels:
- "None (Workbench highlight)"
- "Dimmed"
- "Desaturated"
- "Complemented"

---

### Step 3: Modify `itidy_render_iff_thumbnail()` to Render Selected Image

**Location**: `src/icon_edit/icon_iff_render.c`, function `itidy_render_iff_thumbnail()`

**Current code** (Step 5, lines 1210-1283):
```c
/* Step 5: Scale source pixels into the safe area */
{
    // ... pre-filter logic ...
    
    area_average_scale(iff_params->src_chunky,
                       iff_params->src_width, iff_params->src_height,
                       img->pixel_data_normal,  // ← Only normal image rendered
                       output_width, output_height,
                       img->width,
                       offset_x, offset_y,
                       iff_params->src_palette,
                       iff_params->src_palette_size,
                       img->palette_normal,
                       img->palette_size_normal);
}
```

**New code** (add after Step 5):
```c
/*--------------------------------------------------------------------*/
/* Step 5b: Render selected image with modified palette (if enabled)  */
/*--------------------------------------------------------------------*/

if (iff_params->selected_mode != ITIDY_SELECTED_NONE && 
    img->pixel_data_selected != NULL)
{
    struct ColorRegister *selected_palette = NULL;
    
    // Generate modified palette based on mode
    switch (iff_params->selected_mode)
    {
        case ITIDY_SELECTED_DIMMED:
            selected_palette = itidy_palette_create_dimmed(
                iff_params->src_palette, 
                iff_params->src_palette_size,
                0x9999);  // 60% brightness (0.6 in 16.16 fixed-point)
            break;
            
        case ITIDY_SELECTED_DESATURATED:
            selected_palette = itidy_palette_create_desaturated(
                iff_params->src_palette,
                iff_params->src_palette_size);
            break;
            
        case ITIDY_SELECTED_COMPLEMENTED:
            selected_palette = itidy_palette_create_complemented(
                iff_params->src_palette,
                iff_params->src_palette_size);
            break;
            
        default:
            selected_palette = NULL;
            break;
    }
    
    if (selected_palette != NULL)
    {
        log_debug(LOG_ICONS, "itidy_render_iff_thumbnail: rendering selected image (mode=%u)\n",
                  (unsigned)iff_params->selected_mode);
        
        // Render selected image with modified palette
        // (reuse same scaling path as normal image)
        if (ratio > ITIDY_SCALE_PREFILTER_RATIO && src_area > ITIDY_SCALE_PREFILTER_MIN_AREA)
        {
            // Pre-filtered path
            UBYTE *half_buf = NULL;
            UWORD half_w = 0, half_h = 0;
            
            if (prefilter_2x2(iff_params->src_chunky,
                              iff_params->src_width, iff_params->src_height,
                              &half_buf, &half_w, &half_h,
                              iff_params->src_palette,  // ← Original palette for averaging
                              iff_params->src_palette_size))
            {
                area_average_scale(half_buf, half_w, half_h,
                                   img->pixel_data_selected,  // ← Selected buffer
                                   output_width, output_height,
                                   img->width,
                                   offset_x, offset_y,
                                   iff_params->src_palette,    // ← Original for averaging
                                   iff_params->src_palette_size,
                                   selected_palette,            // ← Modified for output
                                   iff_params->src_palette_size);
                whd_free(half_buf);
            }
            else
            {
                // Fallback to direct scale
                area_average_scale(iff_params->src_chunky,
                                   iff_params->src_width, iff_params->src_height,
                                   img->pixel_data_selected,
                                   output_width, output_height,
                                   img->width,
                                   offset_x, offset_y,
                                   iff_params->src_palette,
                                   iff_params->src_palette_size,
                                   selected_palette,
                                   iff_params->src_palette_size);
            }
        }
        else
        {
            // Direct area-average
            area_average_scale(iff_params->src_chunky,
                               iff_params->src_width, iff_params->src_height,
                               img->pixel_data_selected,
                               output_width, output_height,
                               img->width,
                               offset_x, offset_y,
                               iff_params->src_palette,
                               iff_params->src_palette_size,
                               selected_palette,
                               iff_params->src_palette_size);
        }
        
        // Copy modified palette to selected palette storage
        if (img->palette_selected != NULL)
        {
            whd_free(img->palette_selected);
        }
        img->palette_selected = selected_palette;  // Transfer ownership
        img->palette_size_selected = iff_params->src_palette_size;
        
        img->has_selected_image = TRUE;
        img->has_real_selected_image = TRUE;
    }
}
```

---

### Step 4: Modify `apply_iff_preview()` to Allocate Selected Buffers

**Location**: `src/icon_edit/icon_content_preview.c`, function `apply_iff_preview()`

**Current code** (Step 3, lines 442-489):
```c
if (!itidy_icon_image_create_blank(icon_dim, icon_dim, 0, &img))
{
    // ... error handling ...
}
```

**Change the third parameter** from `0` to `1` to request selected image allocation:
```c
if (!itidy_icon_image_create_blank(icon_dim, icon_dim, 1, &img))  // ← Allocate selected
{
    log_error(LOG_ICONS, "apply_iff_preview: "
              "blank image creation failed for %ux%u\n",
              (unsigned)icon_dim, (unsigned)icon_dim);
    FreeDiskObject(target_icon);
    return ITIDY_PREVIEW_FAILED;
}
```

**Current code** (Step 6b, lines 598-605):
```c
// Discard selected image — edge-to-edge thumbnails have no safe
// area for the bg/text index swap to operate on
if (img.pixel_data_selected != NULL)
{
    whd_free(img.pixel_data_selected);
    img.pixel_data_selected = NULL;
}
img.has_selected_image = FALSE;
img.has_real_selected_image = FALSE;
```

**Remove this code** — the selected image will be populated by the renderer.

**New code** (after cropping normal image, lines 620+):
```c
// Crop selected image to match normal dimensions (if enabled)
if (img.pixel_data_selected != NULL && 
    iff_params.selected_mode != ITIDY_SELECTED_NONE)
{
    UBYTE *tight_sel_buf = (UBYTE *)whd_malloc(tight_size);
    if (tight_sel_buf != NULL)
    {
        // Copy selected image rows
        for (row = 0; row < thumb_h; row++)
        {
            ULONG src_off = (ULONG)(src_oy + row) * (ULONG)old_width
                          + (ULONG)src_ox;
            ULONG dst_off = (ULONG)row * (ULONG)thumb_w;
            memcpy(tight_sel_buf + dst_off,
                   img.pixel_data_selected + src_off, thumb_w);
        }
        
        // Replace selected buffer with tight crop
        whd_free(img.pixel_data_selected);
        img.pixel_data_selected = tight_sel_buf;
    }
}
else if (img.pixel_data_selected != NULL)
{
    // Selected mode is NONE — discard the buffer
    whd_free(img.pixel_data_selected);
    img.pixel_data_selected = NULL;
    img.has_selected_image = FALSE;
    img.has_real_selected_image = FALSE;
}
```

---

### Step 5: Add `selected_mode` Field to `iTidy_IFFRenderParams`

**Location**: `src/icon_edit/icon_iff_render.h`

**Add to struct** (after `palette_mode` field, line ~184):
```c
typedef struct {
    iTidy_RenderParams base;
    
    // ... existing fields ...
    
    UWORD palette_mode;                 /* ITIDY_PAL_PICTURE or ITIDY_PAL_SCREEN */
    UWORD selected_mode;                /* Selected image mode (ITIDY_SELECTED_*) */  // ← NEW
    
    // ... output dimensions ...
} iTidy_IFFRenderParams;
```

**Set in `apply_iff_preview()`** (Step 4, after `palette_mode` assignment):
```c
iff_params.palette_mode = (prefs != NULL)
    ? prefs->deficons_palette_mode
    : ITIDY_PAL_PICTURE;

iff_params.selected_mode = (prefs != NULL)  // ← NEW
    ? prefs->deficons_iff_selected_mode
    : ITIDY_SELECTED_DIMMED;
```

---

## Implementation Checklist

| Step | Task | File | Status |
|------|------|------|--------|
| 1a | Add `itidy_palette_create_dimmed()` | `icon_iff_render.c` | 🔲 |
| 1b | Add `itidy_palette_create_desaturated()` | `icon_iff_render.c` | 🔲 |
| 1c | Add `itidy_palette_create_complemented()` | `icon_iff_render.c` | 🔲 |
| 2a | Add `iTidy_IFFSelectedMode` enum | `layout_preferences.h` | 🔲 |
| 2b | Add `deficons_iff_selected_mode` field | `layout_preferences.h` | 🔲 |
| 2c | Add default constant | `layout_preferences.h` | 🔲 |
| 2d | Initialize preference | `layout_preferences.c` | 🔲 |
| 2e | Add chooser gadget to settings | `deficons_settings_window.c` | 🔲 |
| 3 | Add `selected_mode` field | `icon_iff_render.h` | 🔲 |
| 4a | Populate `selected_mode` from prefs | `icon_content_preview.c` | 🔲 |
| 4b | Request selected buffer in `create_blank()` | `icon_content_preview.c` | 🔲 |
| 4c | Remove selected image discard code | `icon_content_preview.c` | 🔲 |
| 4d | Add selected image cropping code | `icon_content_preview.c` | 🔲 |
| 5 | Add Step 5b to `itidy_render_iff_thumbnail()` | `icon_iff_render.c` | 🔲 |
| 6 | Test with various IFF files | Manual testing | 🔲 |
| 7 | Update main plan status | `IFF_Thumbnail_Implementation_Plan.md` | 🔲 |

---

## Testing Strategy

1. **Visual inspection**: Load IFF thumbnails in Workbench, select icons, verify dimming
2. **Mode comparison**: Test all selected modes (none, dimmed, desaturated, complemented)
3. **Edge cases**: Test with 1-bit (2-color), 6-bit (64-color EHB), and 8-bit (256-color) IFFs
4. **Memory validation**: Enable `DEBUG_MEMORY_TRACKING` and verify no leaks

---

## Open Questions

### Q1: Should desaturated mode also dim?

**Option A**: Pure grayscale (0.299R + 0.587G + 0.114B)  
**Option B**: Grayscale + 60% dimming  

**Recommendation**: Option B (dimmed grayscale) for better visual distinction.

### Q2: What dimming factor to use?

**Options**:
- 50% (0x8000) — very dark
- 60% (0x9999) — moderate (recommended)
- 70% (0xB333) — subtle

**Recommendation**: 60% matches the `text_template.info` appearance.

### Q3: Should selected palette always match normal palette size?

**Current assumption**: Yes, both normal and selected palettes have the same number of entries (from IFF CMAP).

**Validation**: Verify this doesn't break with palette reduction in SCREEN mode (future feature).

---

## Performance Considerations

### Memory Overhead

- **Normal image**: Already allocated (no change)
- **Selected image**: Same size as normal image (~4-10 KB for 64×64 icon)
- **Selected palette**: 256 entries × 3 bytes = 768 bytes maximum

**Total overhead**: ~5-11 KB per icon (acceptable for 68020+ systems with Fast RAM)

### CPU Overhead

- **Palette modification**: O(n) where n = palette entries (256 max) — trivial
- **Second render pass**: Same cost as normal image render (~10-50ms on 68020)

**Total overhead**: ~10-50ms per icon (acceptable during DefIcons creation, not real-time)

---

## Future Enhancements

1. **Adaptive dimming**: Analyze image brightness and adjust dimming factor automatically
2. **Contrast boost**: Increase contrast in selected image for low-contrast sources
3. **Edge highlighting**: Add subtle edge detection glow in selected state
4. **Animation**: Support animated selected states (requires AFF format)

---

## References

- `docs/Default icons/Customize icons/IFF_Thumbnail_Implementation_Plan.md` — Original IFF implementation
- `docs/Default icons/Customize icons/Icon_Image_Editing_Plan.md` — Phase 1-3 architecture
- `src/icon_edit/icon_iff_render.c` — IFF rendering implementation
- `src/icon_edit/icon_content_preview.c` — Content preview orchestration
- `src/icon_edit/icon_image_access.c` — Image data structures and utilities
