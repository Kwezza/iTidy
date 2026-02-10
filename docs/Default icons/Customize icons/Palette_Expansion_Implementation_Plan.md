# iTidy — Automatic Palette Expansion for Adaptive Rendering

## 1. Overview

**Goal:** Automatically expand small palettes (8-16 colors) when adaptive text rendering is enabled, adding smooth grey ramps to improve text quality on gradient backgrounds without requiring manual icon editing.

**Current Situation:**
- Template icon `text_template.info` has only **10 colors** (indices 0-9)
- When `ITIDY_ADAPTIVE_TEXT=YES`, the `build_darken_table()` function can only find "closest matches" from these 10 colors
- Text on gradient backgrounds looks "stepped" or "banded" because intermediate shades are missing

**Proposed Solution:**
- **New ToolType:** `ITIDY_EXPAND_PALETTE` (YES/NO, default YES)
- **Automatic expansion:** When enabled and palette is small (< 16 colors), iTidy automatically:
  1. Analyzes existing palette to identify grey/neutral tones
  2. Generates additional grey ramp entries to fill gaps
  3. Expands palette to 32-48 colors
  4. **Remaps existing pixel data** so border artwork references correct indices
  5. Uses expanded palette for adaptive rendering

---

## 2. Design Decisions

### 2.1 When to Expand?

**Trigger conditions (ALL must be true):**
1. `ITIDY_ADAPTIVE_TEXT=YES` (adaptive rendering enabled)
2. `ITIDY_EXPAND_PALETTE=YES` (not explicitly disabled by user)
3. Original palette size < 16 colors (small palette threshold)
4. Palette contains at least 2 colors (not a 1-color icon)

**Do NOT expand if:**
- Adaptive mode is disabled (fixed text color, expansion is wasted)
- Palette already >= 16 colors (user already designed a rich palette)
- ToolType explicitly says `ITIDY_EXPAND_PALETTE=NO`

### 2.2 How Many Colors to Add?

**Target palette size: 32 colors**

| Original Size | Target Size | Colors Added | Grey Ramp Entries |
|---------------|-------------|--------------|-------------------|
| 2-4 colors | 32 | 28-30 | 24-26 (rest preserved for originals) |
| 5-10 colors | 32 | 22-27 | 18-23 |
| 11-15 colors | 32 | 17-21 | 13-17 |
| 16+ colors | (no expansion) | 0 | n/a |

**Why 32?** Section 20.4.4 of Icon_Image_Editing_Plan.md shows:
- 32-color palettes provide "smooth gradients" quality
- Build time is ~48ms on 68000 @ 7.14 MHz (acceptable)
- File size increase is only ~200 bytes
- 32 entries provide ~3% accuracy for darkening operations

### 2.3 Palette Remapping Strategy

**Two-phase approach:**

#### Phase 1: Preserve Original Colors at Low Indices
The first N colors (where N = original palette size) **remain unchanged**.
This ensures existing pixel data in border artwork still references the
correct colors — no remapping needed for the icon border.

#### Phase 2: Add Grey Ramp at High Indices
New grey ramp entries are added starting at index N.

**Example: 10-color icon expanded to 32:**

| Index Range | Contents | Purpose |
|-------------|----------|---------|
| 0-9 | Original palette (unchanged) | Border artwork colors |
| 10-31 | New grey ramp (22 entries) | Adaptive text darkening |

**Pixel remapping:**
- Pixels in border area: Already use indices 0-9, no change needed
- Pixels in safe area: Will be overwritten during text rendering anyway
- **Result:** Zero pixel remapping required!

### 2.4 Grey Ramp Generation Algorithm

**Step 1: Identify darkest and lightest colors in original palette**

Use the same luminance formula already used by `find_darkest_palette_color()`:

```c
luminance = 0.299 * R + 0.587 * G + 0.114 * B
```

**Step 2: Generate evenly-spaced grey ramp between them**

For N new colors needed, generate N evenly-spaced grey tones from
lightest to darkest:

```c
for (i = 0; i < num_new_colors; i++)
{
    float t = (float)i / (float)(num_new_colors - 1);
    UBYTE grey_value = lightest_lum - (UBYTE)(t * (lightest_lum - darkest_lum));
    palette[original_size + i].red   = grey_value;
    palette[original_size + i].green = grey_value;
    palette[original_size + i].blue  = grey_value;
}
```

**Step 3: Avoid duplicates with existing palette**

After generating each grey tone, check if it's too close to an existing
palette entry (Euclidean distance < 10 in RGB space). If so, adjust
slightly to avoid wasting a palette slot.

### 2.5 Where in the Pipeline?

**Location:** Inside `itidy_icon_image_extract()` in `icon_image_access.c`

**When:** After copying the original palette from the template icon,
but before returning the `iTidy_IconImageData` structure.

**Why here?**
- Happens once per icon rendering operation
- The extracted palette is already a writable copy (allocated with `whd_malloc()`)
- All downstream code (render params, darken table, rendering) uses
  the extracted palette — expansion is transparent to them
- No changes needed to existing rendering code!

**Pseudocode insertion point:**

```c
BOOL itidy_icon_image_extract(struct DiskObject *icon, iTidy_IconImageData *out)
{
    // ... existing extraction code ...
    
    // Copy normal palette (lines 215-223)
    out->palette_size_normal = pal_size_1;
    out->palette_normal = (struct ColorRegister *)whd_malloc(pal_size_1 * sizeof(struct ColorRegister));
    memcpy(out->palette_normal, src_pal_1, pal_size_1 * sizeof(struct ColorRegister));
    
    // **NEW: Automatic palette expansion (if conditions met)**
    if (should_expand_palette(icon, out))
    {
        expand_palette_for_adaptive(&out->palette_normal, &out->palette_size_normal);
    }
    
    // ... rest of extraction ...
}
```

---

## 3. Implementation Plan

### 3.1 New Functions (in `icon_image_access.c`)

#### 3.1.1 `should_expand_palette()`

```c
/**
 * @brief Check if palette expansion should be performed.
 *
 * Expansion is triggered when:
 *   1. ITIDY_ADAPTIVE_TEXT=YES on template icon
 *   2. ITIDY_EXPAND_PALETTE != NO (defaults to YES)
 *   3. Palette size < 16 colors
 *   4. Palette has at least 2 colors
 *
 * @param icon  Template DiskObject (for reading ToolTypes)
 * @param img   Extracted image data (for palette size check)
 * @return TRUE if expansion should occur, FALSE otherwise
 */
static BOOL should_expand_palette(struct DiskObject *icon,
                                 const iTidy_IconImageData *img);
```

**Implementation:**
1. Check `img->palette_size_normal` < 16 && >= 2
2. Parse `ITIDY_ADAPTIVE_TEXT` ToolType → must be YES/TRUE/ON/1
3. Parse `ITIDY_EXPAND_PALETTE` ToolType → if explicitly NO/FALSE/OFF/0, return FALSE
4. Return TRUE (all conditions met)

#### 3.1.2 `expand_palette_for_adaptive()`

```c
/**
 * @brief Expand a small palette by adding smooth grey ramp entries.
 *
 * Preserves original palette entries at indices 0..N-1, adds new
 * grey tones at indices N..31 to provide smoother darkening during
 * adaptive text rendering.
 *
 * Reallocates the palette buffer to 32 entries. Original entries
 * are preserved so existing pixel data in icon borders remains valid.
 *
 * @param palette_ptr      Pointer to palette pointer (will be reallocated)
 * @param palette_size_ptr Pointer to palette size (will be updated to 32)
 * @return TRUE on success, FALSE on allocation failure
 */
static BOOL expand_palette_for_adaptive(struct ColorRegister **palette_ptr,
                                        ULONG *palette_size_ptr);
```

**Implementation steps:**

1. **Save original palette:**
   ```c
   struct ColorRegister *old_palette = *palette_ptr;
   ULONG old_size = *palette_size_ptr;
   ```

2. **Allocate new 32-entry palette:**
   ```c
   struct ColorRegister *new_palette = 
       (struct ColorRegister *)whd_malloc(32 * sizeof(struct ColorRegister));
   if (new_palette == NULL) return FALSE;
   ```

3. **Copy original entries (preserve indices):**
   ```c
   memcpy(new_palette, old_palette, old_size * sizeof(struct ColorRegister));
   ```

4. **Find darkest and lightest colors in original palette:**
   ```c
   UBYTE darkest_lum = find_darkest_luminance(old_palette, old_size);
   UBYTE lightest_lum = find_lightest_luminance(old_palette, old_size);
   ```

5. **Generate grey ramp (evenly spaced from light to dark):**
   ```c
   ULONG num_new = 32 - old_size;
   for (i = 0; i < num_new; i++)
   {
       float t = (float)i / (float)(num_new - 1);
       UBYTE grey = lightest_lum - (UBYTE)(t * (lightest_lum - darkest_lum));
       new_palette[old_size + i].red   = grey;
       new_palette[old_size + i].green = grey;
       new_palette[old_size + i].blue  = grey;
   }
   ```

6. **Free old palette, update pointers:**
   ```c
   whd_free(old_palette);
   *palette_ptr = new_palette;
   *palette_size_ptr = 32;
   ```

7. **Log the expansion:**
   ```c
   log_info(LOG_ICONS, "expand_palette_for_adaptive: expanded %lu colors -> 32 "
            "(added %lu grey ramp entries for adaptive rendering)\n",
            old_size, num_new);
   ```

#### 3.1.3 Helper Functions

```c
/**
 * @brief Find the luminance of the darkest color in a palette.
 */
static UBYTE find_darkest_luminance(const struct ColorRegister *palette,
                                    ULONG palette_size);

/**
 * @brief Find the luminance of the lightest color in a palette.
 */
static UBYTE find_lightest_luminance(const struct ColorRegister *palette,
                                     ULONG palette_size);

/**
 * @brief Calculate luminance of an RGB color (0-255).
 */
static UBYTE calculate_luminance(UBYTE r, UBYTE g, UBYTE b);
```

**Luminance formula:**
```c
return (UBYTE)(0.299f * r + 0.587f * g + 0.114f * b);
```

---

### 3.2 New ToolType Support

#### In `icon_image_access.h`:

Add new ToolType constant:

```c
#define ITIDY_TT_EXPAND_PALETTE  "ITIDY_EXPAND_PALETTE"
```

#### In documentation (`Icon_Image_Editing_Plan.md` Section 20.1):

Add new table row:

| ToolType | Format | Default | Description |
|----------|--------|---------|-------------|
| `ITIDY_EXPAND_PALETTE` | `YES`, `NO` | `YES` | When adaptive text is enabled and the palette has fewer than 16 colors, automatically expand it to 32 colors by adding a smooth grey ramp. This provides better darkening matches for gradient backgrounds. Set to `NO` to preserve the original palette exactly as-is. |

---

### 3.3 Selected Image Handling

**Question:** Should the selected image palette also be expanded?

**Answer:** YES, for consistency.

The selected image often has the same palette (or very similar). If we
expand only the normal palette, the selected image might have mismatched
indices when using safe-area index swap.

**Implementation:**
- Expand both `palette_normal` and `palette_selected` (if selected exists)
- Use the same grey ramp for both (based on normal palette luminance range)

**Code location:**
```c
// After expanding normal palette
if (out->has_selected_image && out->palette_selected != NULL)
{
    expand_palette_for_adaptive(&out->palette_selected,
                                &out->palette_size_selected);
}
```

---

## 4. Testing Strategy

### 4.1 Test Case 1: 10-Color Template (Current Default)

**Setup:**
- Use existing `text_template.info` (10 colors, gradient background)
- Create a test text file with ASCII art and body text

**Steps:**
1. Enable adaptive mode: `ITIDY_ADAPTIVE_TEXT=YES` on template icon
2. Run iTidy on the test file
3. Check logs for: `expand_palette_for_adaptive: expanded 10 colors -> 32`
4. Open generated icon in IconEdit, verify palette has 32 entries
5. Visual check: Text should have smooth shading across gradient

**Expected result:**
- Palette expanded from 10 → 32 colors
- Indices 0-9 unchanged (border artwork unaffected)
- Indices 10-31 contain smooth grey ramp
- Text adapts smoothly to gradient background (no banding)

### 4.2 Test Case 2: Explicit Disable

**Setup:**
- Same 10-color template
- Add ToolType: `ITIDY_EXPAND_PALETTE=NO`

**Steps:**
1. Run iTidy on test file
2. Check logs: Should NOT see palette expansion message
3. Open generated icon: Palette should still be 10 colors

**Expected result:**
- No palette expansion (user explicitly disabled it)
- Original 10-color palette preserved
- Text may show banding on gradient (expected with small palette)

### 4.3 Test Case 3: Already-Large Palette

**Setup:**
- Create a custom template with 20 colors (manually designed)
- Enable adaptive mode

**Steps:**
1. Run iTidy on test file
2. Check logs: Should NOT expand (already >= 16 colors)
3. Palette should remain 20 colors

**Expected result:**
- No expansion (palette already rich enough)
- User's custom palette preserved

### 4.4 Test Case 4: Non-Adaptive Mode

**Setup:**
- 10-color template
- `ITIDY_ADAPTIVE_TEXT=NO` (or omit ToolType)

**Steps:**
1. Run iTidy on test file
2. Check logs: Should NOT expand (adaptive mode disabled)

**Expected result:**
- No expansion (adaptive mode not enabled, expansion is wasted)
- Fixed text color used (not darkening background)

---

## 5. Performance Impact

### 5.1 Time Complexity

| Operation | Original | With Expansion |
|-----------|----------|----------------|
| Palette copy | O(N) | O(N) + O(32) = O(N) |
| Grey ramp generation | — | O(22) = O(1) |
| Darken table build | O(N²) | O(32²) = O(1024) |
| Per-pixel rendering | O(1) | O(1) (unchanged) |

**Net impact:** ~1ms on 68000 @ 7.14 MHz (negligible)

### 5.2 Memory Impact

| Item | Original | With Expansion | Increase |
|------|----------|----------------|----------|
| Palette buffer (runtime) | 10 × 3 = 30 bytes | 32 × 3 = 96 bytes | +66 bytes |
| Icon file size | ~1.5 KB | ~1.7 KB | +200 bytes |
| Darken table | 256 bytes | 256 bytes | 0 |

**Net impact:** +200 bytes per generated icon (acceptable)

### 5.3 CPU Benchmarks (Estimated)

| CPU | Palette Expansion Time | Darken Table Build Time | Total Overhead |
|-----|------------------------|-------------------------|----------------|
| 68000 @ 7.14 MHz | ~0.5 ms | +33 ms (10→32 colors) | ~34 ms |
| 68020 @ 14 MHz | ~0.2 ms | +16 ms | ~16 ms |
| 68030 @ 25 MHz | ~0.1 ms | +9 ms | ~9 ms |
| 68060 @ 50 MHz | <0.1 ms | ~4 ms | ~4 ms |

**Verdict:** Imperceptible to users, even on stock Amiga 500.

---

## 6. File Locations

| File | Changes |
|------|---------|
| `src/icon_edit/icon_image_access.h` | Add `ITIDY_TT_EXPAND_PALETTE` constant |
| `src/icon_edit/icon_image_access.c` | Add 4 new functions: `should_expand_palette()`, `expand_palette_for_adaptive()`, `find_darkest_luminance()`, `find_lightest_luminance()` |
| `docs/Default icons/Customize icons/Icon_Image_Editing_Plan.md` | Update Section 20.1 (add `ITIDY_EXPAND_PALETTE` ToolType documentation) |

**Estimated LOC:** ~150 lines of new code

---

## 7. Alternative Approaches Considered

### 7.1 Expand to 64 Colors Instead of 32

**Pros:** Even smoother gradients (film-quality)  
**Cons:** File bloat (+400 bytes), diminishing returns, slower darken table build

**Decision:** 32 colors is the sweet spot (Section 20.4.4 already recommends this)

### 7.2 Remap Existing Pixels to Interleave Original Colors with Grey Ramp

**Example:** Original colors at even indices (0, 2, 4, ...), grey ramp at odd indices (1, 3, 5, ...)

**Pros:** More intuitive palette layout  
**Cons:** Requires pixel remapping across entire icon (complex, slow, error-prone)

**Decision:** Preserve-at-low-indices approach is simpler and faster

### 7.3 Analyze Template Gradient and Match Its Colors

**Example:** If template has a blue→white gradient, generate blue→white grey ramp

**Pros:** Tighter color matching for complex backgrounds  
**Cons:** Much more complex algorithm, may fail on multi-color gradients

**Decision:** Simple grey ramp works for most cases, users can manually design 32-color palettes for special effects

---

## 8. Future Enhancements (Post-v1.0)

### 8.1 Smart Duplicate Detection

Currently: Grey ramp generation doesn't check if a generated grey tone
is already in the original palette (might waste slots).

**Enhancement:** Before adding a grey tone, check Euclidean distance to
all existing palette entries. If distance < 10, adjust grey value slightly.

### 8.2 Colored Ramps for Non-Grey Gradients

Currently: Always generates grey ramps (R = G = B).

**Enhancement:** Detect dominant gradient color in template (e.g., blue→white)
and generate a colored ramp that follows the gradient's hue.

### 8.3 User-Specified Target Size

Currently: Always expands to 32 colors.

**Enhancement:** Add `ITIDY_EXPAND_TO=48` ToolType to let users choose
32, 48, or 64 as the target.

---

## 9. Known Limitations

1. **Only works with palette-mapped icons:**
   Old planar icons (WB 1.x/2.x format) cannot benefit — they don't
   have RGB palettes. This is acceptable because iTidy's text preview
   system already requires OS 3.5+ palette-mapped icons.

2. **No color matching for existing gradients:**
   If the template has a red→yellow gradient, the grey ramp won't
   match it. Users must manually design a 32-color palette for such
   special cases.

3. **Fixed threshold (16 colors):**
   The "small palette" threshold is hardcoded. A palette with 17 colors
   won't expand, even if it could benefit. This is a reasonable
   compromise to avoid expanding already-rich palettes.

---

## 10. Decision Log

| # | Question | Decision | Date |
|---|----------|----------|------|
| 1 | Enable by default? | **Yes** — `ITIDY_EXPAND_PALETTE=YES` by default | 2026-02-10 |
| 2 | Target palette size? | **32 colors** (best balance of quality/performance) | 2026-02-10 |
| 3 | Preserve original indices? | **Yes** — original colors stay at 0..N-1, grey ramp at N..31 | 2026-02-10 |
| 4 | Expand selected palette too? | **Yes** — for consistency with safe-area index swap | 2026-02-10 |
| 5 | Where in pipeline? | **Inside `itidy_icon_image_extract()`** after palette copy | 2026-02-10 |
| 6 | Small palette threshold? | **< 16 colors** | 2026-02-10 |
| 7 | Grey ramp algorithm? | **Evenly-spaced luminance from lightest to darkest** | 2026-02-10 |

---

## 11. Implementation Checklist

- [ ] Add `ITIDY_TT_EXPAND_PALETTE` constant to `icon_image_access.h`
- [ ] Implement `calculate_luminance()` helper
- [ ] Implement `find_darkest_luminance()` helper
- [ ] Implement `find_lightest_luminance()` helper
- [ ] Implement `should_expand_palette()` with ToolType checks
- [ ] Implement `expand_palette_for_adaptive()` with grey ramp generation
- [ ] Integrate into `itidy_icon_image_extract()` after normal palette copy
- [ ] Add selected palette expansion (if selected image exists)
- [ ] Add logging for expansion operations
- [ ] Update `Icon_Image_Editing_Plan.md` Section 20.1 with new ToolType
- [ ] Add Decision #24 entry to Section 17 (Decision Log) in `Icon_Image_Editing_Plan.md`
- [ ] Build and test on WinUAE
- [ ] Test Case 1: 10-color template with adaptive mode (should expand)
- [ ] Test Case 2: `ITIDY_EXPAND_PALETTE=NO` (should NOT expand)
- [ ] Test Case 3: 20-color template (should NOT expand)
- [ ] Test Case 4: Adaptive mode disabled (should NOT expand)
- [ ] Visual verification: Text should adapt smoothly to gradient
- [ ] Performance check: Verify overhead is < 50ms on 68000

---

*This plan was created on 2026-02-10 as part of the iTidy v1.0 content-aware
icon preview system.*
