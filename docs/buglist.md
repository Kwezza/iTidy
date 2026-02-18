# iTidy Bug List

## Active Bugs

### Bug #1: DefIcons Randomly Returns 'project' Type Instead of 'ascii' for .txt Files

**Status:** FIXED - Extension cache was amplifying DefIcons bugs  
**Date Reported:** 2026-02-11  
**Date Fixed:** 2026-02-11  
**Severity:** Medium - Results in incorrect icon types being created

**Description:**
When running iTidy on a folder containing .txt files (after deleting all existing .info files with `delete #?.info`), DefIcons occasionally returns 'project' type instead of the expected 'ascii' type for .txt files.

**Evidence from `icons_2026-02-11_10-17-19.log`:**

**First run (10:17:26):**
- `sdw-life.txt` → DefIcons correctly returned `'ascii'` ✅
- File received hand-drawn text preview icon as expected

**Second run (10:19:32) - Same session:**
- `xz-aisli.txt` → DefIcons returned `'project'` ❌ (should be 'ascii')
- Multiple other .txt files received default project icons WITHOUT DefIcons being queried:
  - `wpx-rntn.txt`, `wpx-----.txt`, `tls-else.txt`, `tlo3o3o3.txt`
  - `skyline.txt`, `shd-back.txt`, `sdw-life.txt`, `shd-2oo3.txt`, `pa-jsa.txt`

**Log Evidence:**
```
Line 23: [10:17:26] DefIcons query: 'sdw-life.txt' → type 'ascii' ✅
Line 5142: [10:19:32] DefIcons query: 'xz-aisli.txt' → type 'project' ❌
Lines 5146-5153: Multiple .txt files got project icons without DefIcons queries
```

**Reproduction Context:**
- User deletes all .info files with shell command: `delete #?.info`
- Runs iTidy to regenerate icons
- Issue occurs randomly - not reproducible consistently

**Root Cause:**
1. **DefIcons bug**: DefIcons occasionally returns 'project' for .txt files with ASCII art content
2. **Extension cache amplification**: iTidy's extension cache stores the first DefIcons response per extension (`.txt` → `'project'`)
3. **Cascading failure**: All subsequent .txt files hit the cache and get 'project' WITHOUT querying DefIcons
4. **Files without extensions work**: Files like `ReadMeSE`, `Instructions`, `Manual` bypass cache and query DefIcons directly

**Proof:**
- User test: Renamed `KB-idiot.txt` → `KB-idiot` (removed extension)
- Result: File immediately received correct 'ascii' type and text preview icon
- This proves the cache was the issue, not DefIcons queries themselves

**Impact:**
- .txt files that should get hand-drawn text preview icons get generic project icons instead
- Only affects first run after cache is empty - subsequent runs reuse bad cache
- Files without .txt extension always work correctly

**Fix Implemented:**
1. **Response validation**: Added `validate_deficons_response()` function that detects `.txt` + `'project'` pattern
2. **Type override**: When DefIcons returns 'project' for .txt files, iTidy overrides it to 'ascii'
3. **Cache bypass**: Invalid responses from DefIcons are not cached (prevents amplification)
4. **Narrowly scoped**: Only corrects known bug case (.txt files), doesn't interfere with other file types
5. **Enhanced logging**: 
   - Warns: "Suspicious DefIcons response: .txt → 'project' (expected 'ascii')"
   - Confirms: "Corrected type to 'ascii' for: [filepath]"
   
**Code Location:** `src/deficons_identify.c` - `deficons_identify_file()` function

**Testing Results:**
- Multiple test runs confirm the fix works consistently
- When DefIcons returns 'project' for .txt files, iTidy detects and corrects it
- All .txt files now receive proper cyan text preview icons regardless of DefIcons response
- Other file types (.lha, .mod, etc.) unaffected - only .txt override implemented
- No false positives observed

---

### Bug #3: Magenta Key-Colour Sweep False Positive on JPEG Icons (Black Renders as Transparent)

**Status:** FIXED  
**Date Reported:** 2026-02-18  
**Date Fixed:** 2026-02-18  
**Severity:** High - All JPEG thumbnails had incorrect transparency applied

**Description:**
After the magenta key-colour sweep was added (Bug #2 fix), all JPEG thumbnails showed dark/black areas as transparent (grey Workbench background showing through). Palette index 0 was being incorrectly flagged as transparent for every JPEG processed.

**Root Cause:**
The 6x6x6 colour cube ALWAYS contains R=255, G=0, B=255 at index 185 regardless of the image content. The sweep scanned the palette for any entry matching the magenta threshold and always found it. When the palette was swapped (185 <-> 0), the old index 0 (black, #000000) moved to index 185, and all formerly-black pixels were remapped to index 0 (now transparent). Confirmed by log showing "magenta key colour found at palette index 185 (R=255 G=0 B=255)" for every JPEG.

**Fix:**
- Added `BOOL try_magenta_key` field to `iTidy_IFFRenderParams` (input flag).
- Added `itidy_format_supports_transparency()` helper in `icon_content_preview.c` -- returns FALSE for jpeg/bmp, TRUE for png/gif/ilbm/acbm/other.
- `apply_picture_preview()` sets `iff_params.try_magenta_key = itidy_format_supports_transparency(type_token)`, skipping the sweep for JPEG/BMP.
- `apply_iff_preview()` explicitly sets `iff_params.try_magenta_key = TRUE` (ILBM/ACBM can carry magenta key transparency).
- The sweep block in `itidy_render_via_datatype()` is gated on `params->try_magenta_key`.
- As a design improvement, Step 7 in `apply_picture_preview()` also forces borders ON for opaque formats (JPEG/BMP) regardless of the user's border preference -- without both transparency and borders, icons blend into the Workbench window background.

**Files Changed:**
- `src/icon_edit/icon_iff_render.h` -- added `try_magenta_key` to `iTidy_IFFRenderParams`
- `src/icon_edit/icon_iff_render.c` -- gated magenta sweep on `try_magenta_key`
- `src/icon_edit/icon_content_preview.c` -- added helper, set flag, format-aware Step 7

---

### Bug #4: Ultra Mode JPEG Thumbnails Have Muted/Posterised Colours (~30 Colours Instead of 256)

**Date:** 2026-02-18
**Severity:** High - Ultra mode produced worse colour quality than standard mode for JPEG/BMP

**Symptom:**
JPEG thumbnails generated with Ultra mode enabled had heavily posterised, muted colour palettes. Log showed only 30-37 unique colours from a 64x64 pixel thumbnail, all with RGB values that were exact multiples of 51 (the 6x6x6 grid step size).

**Root Cause:**
Ultra mode in `apply_picture_preview()` built its RGB24 input by expanding the already-indexed pixel buffer back through the 6x6x6 palette. By that point every channel had already been snapped to one of {0, 51, 102, 153, 204, 255}. The actual area-average downscaled RGB24 thumbnail with smooth intermediate values had been quantized and then freed inside `itidy_render_via_datatype()`. `itidy_ultra_generate_palette()` therefore saw only ~30 of the 216 possible 6x6x6 combinations, never the true photo colours.

Confirmed by log: `ultra_downsample: 30 unique colors from 4096 RGB24 pixels` with all palette entries being multiples of 0x33.

**Fix:**
Added `UBYTE **raw_rgb24_out` and `ULONG raw_rgb24_pixel_count` output fields to `iTidy_IFFRenderParams`. When `raw_rgb24_out` is non-NULL on entry, `itidy_render_via_datatype()` transfers ownership of its pre-quantization `thumb_rgb24` buffer to the caller (sets `*raw_rgb24_out`, nulls the local pointer to prevent cleanup from freeing it). `apply_picture_preview()` sets this field when Ultra mode is active and passes the received buffer directly to `itidy_ultra_generate_palette()`, bypassing the lossy indexed re-expansion entirely.

**Files Changed:**
- `src/icon_edit/icon_iff_render.h` -- added `raw_rgb24_out` and `raw_rgb24_pixel_count` to `iTidy_IFFRenderParams`
- `src/icon_edit/icon_iff_render.c` -- added transfer-or-free logic for `thumb_rgb24` in `itidy_render_via_datatype()`
- `src/icon_edit/icon_content_preview.c` -- wired `raw_rgb24_out`, new Ultra mode block uses pre-quantization buffer with indexed fallback

---

