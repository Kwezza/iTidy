# iTidy — Icon Image Editing & Content-Aware Preview Plan

**Version:** 2.0  
**Created:** 2026-02-09  
**Last updated:** 2026-02-18 (border mode chooser added)  
**Status:** Implemented (Phases 1–4 complete)  
**Relates to:** `docs/iTidy_Future_Content_Aware_Icons_TODO.md`

---

## Table of Contents

1. [Overview](#1-overview)
2. [Architecture Summary](#2-architecture-summary)
3. [Phase 1 — Icon Image Access Module (Reusable Core)](#3-phase-1--icon-image-access-module-reusable-core)
4. [Phase 2 — ASCII Text-to-Pixel Renderer](#4-phase-2--ascii-text-to-pixel-renderer)
5. [Phase 3 — Integration with DefIcons Pipeline](#5-phase-3--integration-with-deficons-pipeline)
   - [5.4 Type-Specific Template Resolution Chain](#54-type-specific-template-resolution-chain)
6. [Template Icon ToolType Conventions](#6-template-icon-tooltype-conventions)
7. [Output Format Decision](#7-output-format-decision)
8. [Selected Image Handling](#8-selected-image-handling)
9. [Palette Strategy](#9-palette-strategy)
10. [Safe Area Defaults](#10-safe-area-defaults)
11. [Character-to-Pixel Mapping](#11-character-to-pixel-mapping)
12. [Performance & Safety](#12-performance--safety)
13. [Debug & Development Aids](#13-debug--development-aids)
14. [Future Reuse — Other Icon Types](#14-future-reuse--other-icon-types)
15. [API Surface Summary](#15-api-surface-summary)
16. [Metadata Preservation — Test Checklist](#16-metadata-preservation--test-checklist)
17. [Open Questions / Decisions Log](#17-open-questions--decisions-log)
18. [Implementation Status & Lessons Learned](#18-implementation-status--lessons-learned)
19. [Text Downscaling — Fitting 80-Column Documents into Tiny Icons](#19-text-downscaling--fitting-80-column-documents-into-tiny-icons)
20. [Template Icon ToolType Reference (For Icon Designers)](#20-template-icon-tooltype-reference-for-icon-designers)
21. [Phase 4 — Picture Datatype Pipeline (PNG, GIF, JPEG, BMP)](#21-phase-4--picture-datatype-pipeline-png-gif-jpeg-bmp)
22. [Thumbnail Border Mode — Never / Auto / Always](#22-thumbnail-border-mode--never--auto--always)

---

## 1. Overview

This document describes the plan for rendering content previews onto Amiga
icons. The first implementation targets **ASCII text documents** — each
character in the source file maps to roughly 1–2 horizontal pixels, and each
line of text is separated by 1 vertical pixel, producing a miniature "page"
thumbnail effect directly on the icon.

The architecture is deliberately split into a **reusable icon image editing
core** (Phase 1) and a **content-specific renderer** (Phase 2 onward). The
core module will be shared by all future content-aware icon features described
in `docs/iTidy_Future_Content_Aware_Icons_TODO.md`, including:

- IFF image thumbnails (Section 2)
- Font preview icons (Section 3)
- Badge overlays (Section 9)

### High-Level Flow

```
Template .info ──copy──> Target .info ──load──> DiskObject
                                                    │
                                        Extract image data
                                        (palette + pixels)
                                                    │
                                        Render content into
                                        safe area of pixel buf
                                                    │
                                        Write back image data
                                        (always OS3.5 format)
                                                    │
                                        Save icon, preserving
                                        all non-image metadata
```

---

## 2. Architecture Summary

All new icon-editing code lives in a dedicated subfolder to keep it
separate from the existing codebase:

```
src/
  icon_edit/              ← NEW: all icon image editing code lives here
    icon_image_access.c   ← Reusable core: load/extract/apply/save
    icon_image_access.h
    icon_text_render.c    ← ASCII text-to-pixel renderer
    icon_text_render.h
    (future) icon_iff_render.c/.h
    (future) icon_font_render.c/.h
    (future) icon_badge_render.c/.h
```

The existing `src/deficons_creation.c` is modified to call into the new
modules, but the bulk of the new code stays inside `src/icon_edit/`.

| Module | Files | Purpose |
|--------|-------|---------|
| **Icon Image Access** | `src/icon_edit/icon_image_access.c/.h` | Load, inspect, modify, and save icon image data. Reusable by all icon editing features. |
| **ASCII Text Renderer** | `src/icon_edit/icon_text_render.c/.h` | Read an ASCII file and paint a miniature text preview into a pixel buffer. |
| **DefIcons Integration** | Modified `src/deficons_creation.c` | Orchestrates: copy template → load → render → save. Calls into `src/icon_edit/`. |

### Dependency Graph

```
deficons_creation.c  (existing, modified)
    │
    ├── icon_edit/icon_image_access.c   (load/save icon pixels)
    │       │
    │       └── icon_types.c  (existing: format detection)
    │
    └── icon_edit/icon_text_render.c    (ASCII → pixel buffer)
```

Future renderers (IFF thumbnail, font preview) are added as new `.c/.h`
files inside `src/icon_edit/`. They receive a pixel buffer and safe area
rectangle, and paint into it. They never need to know about icon formats.

### Template Icon Location

Text preview templates live in `PROGDIR:Icons/` (host: `Bin\Amiga\Icons\`).
There is no single hardcoded template — the system resolves which template
to use at runtime via a two-level fallback chain (see Section 5.4):

1. `PROGDIR:Icons/def_<type>.info` — type-specific (e.g. `def_c.info`, `def_rexx.info`)
2. `PROGDIR:Icons/def_ascii.info` — generic ASCII fallback

If neither template exists, text preview rendering is skipped entirely and
the file keeps the plain DefIcons system icon. Future template icons for
other preview types (IFF, font, etc.) will also live in `PROGDIR:Icons/`.

---

## 3. Phase 1 — Icon Image Access Module (Reusable Core)

**New files:** `src/icon_edit/icon_image_access.c`, `src/icon_edit/icon_image_access.h`

This module provides a clean API to:

1. **Extract** image data (pixels + palette) from any loaded `DiskObject`
2. **Modify** the pixel buffer in-place
3. **Write back** the modified image and save the icon to disk

### 3.1 Data Structure

```c
typedef struct {
    UWORD width;                    /* Icon image width in pixels */
    UWORD height;                   /* Icon image height in pixels */

    UBYTE *pixel_data_normal;       /* Chunky pixel buffer — normal state */
    UBYTE *pixel_data_selected;     /* Chunky pixel buffer — selected state (may be NULL) */

    struct ColorRegister *palette_normal;    /* RGB palette — normal */
    struct ColorRegister *palette_selected;  /* RGB palette — selected (may be NULL) */
    ULONG palette_size_normal;      /* Number of palette entries — normal */
    ULONG palette_size_selected;    /* Number of palette entries — selected */

    /* Metadata (read-only, for diagnostics) */
    int source_format;              /* icon_type_standard / icon_type_newIcon / icon_type_os35 */
    BOOL is_palette_mapped;         /* TRUE if icon.library reports palette-mapped */
    BOOL has_selected_image;        /* TRUE if a second image state exists */
} iTidy_IconImageData;
```

All pixel and palette buffers are **owned copies** allocated with
`whd_malloc()`. The caller is free to modify them and must free them via a
dedicated cleanup function.

### 3.2 Core Functions

| Function | Purpose |
|----------|---------|
| `itidy_icon_image_extract(DiskObject *icon, iTidy_IconImageData *out)` | Populate `out` from a loaded icon. Copies pixel data and palette so the caller owns them. |
| `itidy_icon_image_apply(DiskObject *icon, const iTidy_IconImageData *data)` | Write modified pixel/palette data back onto the icon via `IconControlA()`. |
| `itidy_icon_image_save(const char *path, DiskObject *icon)` | Save the icon to disk via `PutIconTagList()` in OS3.5 format. Preserves DefaultTool, ToolTypes, position, protection, stack size, etc. |
| `itidy_icon_image_free(iTidy_IconImageData *data)` | Free all allocated buffers inside the struct. |
| `itidy_icon_image_create_blank(UWORD w, UWORD h, UBYTE bg_index, iTidy_IconImageData *out)` | Allocate a blank pixel buffer filled with `bg_index`. Useful when the template has no usable image. |

### 3.3 Implementation Details — Extract

```
1. Load icon with GetIconTagList(path, TAG_DONE)
2. Query format:
     IconControlA(icon, ICONCTRLA_IsPaletteMapped, &is_pm, TAG_END)
3. If palette-mapped:
     IconControlA(icon,
         ICONCTRLA_GetWidth,         &width,
         ICONCTRLA_GetHeight,        &height,
         ICONCTRLA_GetImageData1,    &src_pixels_1,
         ICONCTRLA_GetImageData2,    &src_pixels_2,
         ICONCTRLA_GetPalette1,      &src_pal_1,
         ICONCTRLA_GetPalette2,      &src_pal_2,
         ICONCTRLA_GetPaletteSize1,  &pal_size_1,
         ICONCTRLA_GetPaletteSize2,  &pal_size_2,
         TAG_END)
4. Copy pixel data: whd_malloc(width * height) per image
5. Copy palette: whd_malloc(pal_size * sizeof(struct ColorRegister)) per image
6. If NOT palette-mapped (old planar icon):
     - Use LayoutIconA() to make icon.library convert to chunky
     - Then extract as above
     - Alternatively: create blank buffer at template dimensions
```

**Critical note from AutoDocs:** `IconControlA` returns **pointers into the
icon's own data** — they are NOT copies. We MUST copy them immediately, because
the original icon will be disposed before we save.

### 3.4 Implementation Details — Apply & Save

```
1. DupDiskObjectA(original_icon, TAG_END)  →  clone
2. IconControlA(clone,
       ICONCTRLA_SetWidth,         data->width,
       ICONCTRLA_SetHeight,        data->height,
       ICONCTRLA_SetImageData1,    data->pixel_data_normal,
       ICONCTRLA_SetPalette1,      data->palette_normal,
       ICONCTRLA_SetPaletteSize1,  data->palette_size_normal,
       ICONCTRLA_SetImageData2,    data->pixel_data_selected,    (if present)
       ICONCTRLA_SetPalette2,      data->palette_selected,       (if present)
       ICONCTRLA_SetPaletteSize2,  data->palette_size_selected,  (if present)
       TAG_END)
3. LayoutIconA(clone, NULL, TAG_END)       ← required after modifying image
4. PutIconTagList(path, clone,
       ICONPUTA_NotifyWorkbench,        TRUE,
       ICONPUTA_DropPlanarIconImage,    TRUE,
       ICONPUTA_DropNewIconToolTypes,   TRUE,
       TAG_END)
5. DisposeObject(clone)
6. NOW safe to free pixel_data / palette buffers
```

**Important lifetime rule:** The pixel and palette buffers passed via
`ICONCTRLA_Set*` are **not copied** by `IconControlA()`. They must remain
valid from the `Set` call through `PutIconTagList()` and until after
`DisposeObject()`. Only then may `itidy_icon_image_free()` be called.

---

## 4. Phase 2 — ASCII Text-to-Pixel Renderer

**New files:** `src/icon_edit/icon_text_render.c`, `src/icon_edit/icon_text_render.h`

This module reads an ASCII file and renders a miniature text preview into a
caller-supplied pixel buffer. It knows nothing about icon formats — it
operates purely on a rectangular pixel buffer with palette indices.

### 4.1 Generic Render Parameters (Shared by All Renderers)

All renderers share a common parameter struct. This is defined once in
`icon_image_access.h` and used by every renderer — no refactoring needed
when adding IFF thumbnails or font previews later.

```c
typedef struct {
    UBYTE *pixel_buffer;      /* Target buffer (width × height bytes) */
    UWORD buffer_width;       /* Full buffer width (icon width) */
    UWORD buffer_height;      /* Full buffer height (icon height) */

    /* Safe area within the buffer where rendering is permitted */
    UWORD safe_left;
    UWORD safe_top;
    UWORD safe_width;
    UWORD safe_height;

    /* Palette indices to use */
    UBYTE bg_color_index;     /* Background / whitespace */
    UBYTE text_color_index;   /* Foreground / content pixels */
} iTidy_RenderParams;
```

### 4.1.1 Text-Specific Extensions

The ASCII renderer accepts additional parameters beyond the common set.
These are passed as extra fields or a wrapper struct:

```c
typedef struct {
    iTidy_RenderParams base;  /* Common render parameters */

    /* Text-specific options */
    UWORD char_pixel_width;   /* 1 or 2 pixels per character (0 = auto) */
    UWORD line_pixel_height;  /* Height of one text line in pixels (typically 1) */
    UWORD line_gap;           /* Vertical gap between lines in pixels (typically 1) */
    UWORD max_read_bytes;     /* Max bytes to read from source file (default 4096) */
} iTidy_TextRenderParams;
```

Future renderers (IFF thumbnails, font previews) will define their own
extension structs wrapping `iTidy_RenderParams` if they need extra fields.

### 4.2 Renderer Function

```c
BOOL itidy_render_ascii_preview(
    const char *file_path,
    iTidy_TextRenderParams *params
);
```

All renderers follow the same calling convention:
`BOOL itidy_render_<type>(const char *source_path, iTidy_<Type>RenderParams *params)`

### 4.3 Rendering Algorithm

```
1. Open the source file (read-only, first 4 KB or 20 lines max)
2. Determine char_pixel_width if auto-select (0):
     Default: char_pixel_width = 1  (the "mini-page" aesthetic)
     Only use 2 if safe_width >= 64  (large icons where bolder text helps)
     In practice, 1px is almost always selected for typical 32–80px icons.
3. Calculate max_chars_per_line = safe_width / char_pixel_width
4. Calculate max_lines = safe_height / (line_pixel_height + line_gap)
5. Fill the safe area with bg_color_index
6. For each line of text (up to max_lines):
     For each character in the line (up to max_chars_per_line):
       If printable (0x20–0x7E, or 0xA0–0xFF for ISO-8859-1):
         Paint char_pixel_width pixels at current position with text_color_index
       Else (space, tab, control):
         Leave as bg_color_index (already filled)
       Advance horizontal position by char_pixel_width
     Advance vertical position by (line_pixel_height + line_gap)
7. Close file, return TRUE
```

**Tab handling:** Expand tabs to the next multiple of 8 character positions
(standard terminal convention). Each "space" within the tab expansion renders
as background colour.

**Line length:** Lines longer than `max_chars_per_line` are silently
truncated (no wrapping — mimics the "zoomed out document" look).

### 4.4 Visual Result

At 1 pixel per character on a typical 48×48 icon with a safe area of
roughly 38×30 pixels:

- ~38 characters visible per line
- ~15 lines visible (with 1px line + 1px gap)
- Produces a recognisable "miniature page of text" effect

At 2 pixels per character:

- ~19 characters visible per line
- ~15 lines visible
- Bolder / more visible on high-res screens

---

## 5. Phase 3 — Integration with DefIcons Pipeline

### 5.1 Modified Creation Flow

Currently `deficons_copy_icon_file()` does a raw byte-for-byte copy of the
template `.info` file. The new flow adds an optional post-copy editing step:

```
1. Identify file type via DefIcons (existing)
2. Resolve template icon (existing)
3. ** NEW: Cache-skip check (before any file copy or rendering) **
   If target .info already exists:
     a. Load it with GetIconTagList()
     b. Check ToolTypes for ITIDY_CREATED=1 and ITIDY_KIND=<expected>
     c. Compare ITIDY_SRC_SIZE and ITIDY_SRC_DATE against current source file
     d. If all match → source unchanged, icon already current → SKIP entirely
     e. FreeDiskObject() and continue to step 4 if stale or missing stamps
4. Copy template .info to target location (existing)
5. ** NEW: If content preview is enabled for this type: **
   a. Load the newly-copied icon: GetIconTagList(target_path, TAG_END)
   b. Extract image data: itidy_icon_image_extract(icon, &img)
   c. Read safe area from icon ToolTypes (or use defaults)
   d. Resolve palette indices:
        - Use ITIDY_BG_COLOR / ITIDY_TEXT_COLOR from ToolTypes if present
        - Otherwise, scan palette for lightest entry (→ bg) and
          darkest entry (→ text) as fallback heuristic
   e. Call the appropriate renderer:
        - ASCII/text → itidy_render_ascii_preview(source_file, &params)
        - (future) IFF image → itidy_render_iff_thumbnail(source_file, &params)
        - (future) Font → itidy_render_font_preview(source_file, &params)
   f. If template had a selected image: build selected image (see Section 8)
      If template had NO selected image: leave ImageData2 as NULL
      (Workbench will auto-dim the normal image on selection)
   g. Apply modified image: itidy_icon_image_apply(icon, &img)
   h. Stamp ToolTypes: ITIDY_CREATED=1, ITIDY_KIND=text_preview, etc.
   i. Save: itidy_icon_image_save(target_path, icon)
   j. Cleanup: itidy_icon_image_free(&img), FreeDiskObject(icon)
6. Log result (existing statistics tracking)
```

### 5.1.1 Cache-Skip Benefits

- Avoids re-rendering icons whose source file has not changed
- Prevents "regenerate every run" behaviour on large recursive operations
- The check is cheap: one `GetIconTagList()` + a few `FindToolType()` calls
- If the source file has changed (different size or datestamp), the old icon
  is overwritten with a fresh render

### 5.2 Renderer Dispatch

A simple type-to-renderer mapping:

```c
/* Content renderer dispatch — extend as new renderers are added */
if (type is ascii/text)
    itidy_render_ascii_preview(source_path, &params);
else if (type is iff_picture)       /* future */
    itidy_render_iff_thumbnail(source_path, &params);
else if (type is font)              /* future */
    itidy_render_font_preview(source_path, &params);
else
    /* No preview — icon keeps the unmodified template image */
```

### 5.3 File Exclusion Filters

Before processing files, iTidy applies several pre-filters to skip files that
should never receive icons, regardless of user preferences or DefIcons
identification results.

#### 5.3.1 System Files (Hardcoded Exclusions)

These files are critical Workbench system files and are **always** skipped:

| File Pattern | Purpose | Reason for Exclusion |
|--------------|---------|---------------------|
| `*.info` | Icon metadata files | These ARE icons themselves — creating icons for icons would cause recursion |
| `.info` | Edge case: file named exactly ".info" | Same as above — treated as icon metadata |
| `.backdrop` | Workbench left-out icon tracking | Critical system file that tracks which icons should remain visible on Workbench root. Must never be modified or given an icon |

**Implementation:** Function `is_system_file_never_icon()` in
`src/deficons_creation.c` checks filenames against these patterns before any
DefIcons identification occurs. These exclusions apply **universally** across
all volumes and directories.

**Historical note:** Early testing revealed that without these filters, iTidy
would create icons for `Workbench:.info` (the device icon itself) and
`Workbench:.backdrop` (the left-out icon tracker), which could corrupt the
Workbench display. The `.info` check also had a subtle bug where files named
exactly ".info" (5 characters) were missed due to a minimum length check of 6
characters — this was corrected to catch all `.info` files including this edge
case.

#### 5.3.2 Empty Files (0 Bytes)

Files with zero bytes (completely empty) are skipped before DefIcons
identification:

```c
/* In deficons_creation.c, before calling deficons_identify_file() */
if (fib->fib_Size == 0)
{
    log_debug(LOG_ICONS, "Skipping empty file (0 bytes): %s\n", fib->fib_FileName);
    continue;
}
```

**Rationale:**
- Empty files contain no data to identify or preview
- DefIcons cannot meaningfully determine the type of an empty file
- Text preview rendering requires actual content to display
- Such files are typically placeholders or remnants (e.g., `MuForce.log`
  created at 0 bytes on system startup)

**Minimum size:** 1 byte. Any file with actual content (≥1 byte) will be
processed normally. This allows legitimate small files (e.g., a 64-byte text
file) to receive icons while excluding truly empty placeholders.

#### 5.3.3 User-Configurable Exclude Paths

In addition to the above hardcoded filters, users can configure directory paths
to exclude via the "Exclude Paths" window (see `src/GUI/exclude_paths_window.c`).
This allows project-specific or temporary directories to be skipped during
recursive icon creation. See `docs/layout_preferences.h` for the preference
storage details.

### 5.4 Type-Specific Template Resolution Chain

When rendering a text preview, iTidy resolves the image template through
a **two-level fallback chain**, checking each level with `Lock()` before
trying the next. If no template is found at either level, rendering is
skipped and the file keeps the plain DefIcons system icon — no text
preview is applied.

**Purpose:** This lets you create custom-styled icons for specific file types
(e.g. a blue-tinted `def_c.info` for C source, a green-tinted `def_rexx.info`
for Rexx scripts) while falling back gracefully to the generic ASCII template
when a type-specific one has not been created.

#### Fallback Levels

| Level | Path | When used |
|-------|------|-----------|
| 1 | `PROGDIR:Icons/def_<type>.info` | Custom template exists for this exact DefIcons type token |
| 2 | `PROGDIR:Icons/def_ascii.info` | No type-specific template; generic ASCII template present |
| — | *(none)* | Neither found — text preview skipped, default DefIcons icon used |

**Examples:**
- `.c` file → tries `def_c.info` first, falls back to `def_ascii.info`
- `.rexx` file → tries `def_rexx.info` first, falls back to `def_ascii.info`
- `.readme` file → tries `def_readme.info` (likely missing), uses `def_ascii.info`
- `.c` file on a fresh install with no Icons drawer → preview skipped, plain DefIcons icon used

**Minimum requirement:** `def_ascii.info` must exist in `PROGDIR:Icons/` for
any text preview to render. Without it, only files that have their own
type-specific template will get a preview.

#### How to Add a Type-Specific Template

1. Copy an existing template (e.g. `Bin/Amiga/Icons/def_ascii.info`) to a new
   file named after the DefIcons type token, e.g. `def_c.info` or `def_rexx.info`.
2. Open it in an icon editor and adjust the ToolTypes to customise the safe area,
   colours, character width etc. (see Section 6 for all available ToolTypes).
3. Place the finished `.info` file in `PROGDIR:Icons/` (i.e. `Bin/Amiga/Icons/`
   on the host machine, which is shared into WinUAE).
4. Run iTidy — logs will confirm which template was selected for each file type:

```
[LOG_ICONS] Using type-specific preview template: PROGDIR:Icons/def_c for type=c
[LOG_ICONS] Using generic ASCII preview template: PROGDIR:Icons/def_ascii for type=readme
[LOG_ICONS] No preview template found for type=unknown (...) -- skipping text preview
```

#### EXCLUDETYPE — Opting Specific Types Out of Text Preview

The `def_ascii.info` template icon may carry an `EXCLUDETYPE` ToolType listing
DefIcons type tokens that should **skip** text preview rendering entirely and
keep the plain template image instead.

```
EXCLUDETYPE=amigaguide,html,install
```

- The list is **comma-separated** and **case-insensitive**.
- Leading and trailing whitespace around each token is trimmed.
- iTidy reads this ToolType from `PROGDIR:Icons/def_ascii.info` before
  deciding whether to render a preview for a given file.
- If the file's type token appears in the list, the file receives the plain
  copied template icon with no preview painted onto it.
- This is useful for types that are technically classified as `ascii` by
  DefIcons but have their own dedicated icon artwork that should not be
  overwritten with a text preview (e.g. AmigaGuide documents, HTML pages,
  installer scripts).

See also Section 6.4 for the full ToolType specification.

#### Implementation Reference

The resolution logic lives in `itidy_resolve_preview_template()` in
`src/icon_edit/icon_content_preview.c`. The EXCLUDETYPE check is performed by
`is_excluded_from_text_preview()` in the same file, called before any
template loading begins.

For the complete implementation notes and testing checklist see
`docs/Type_Specific_Text_Preview_Templates.md`.

---

## 6. Template Icon ToolType Conventions

Template icons (e.g. `PROGDIR:Icons/def_ascii.info`) may contain ToolTypes
that control the preview rendering area. These are **optional** — sensible
defaults apply if they are absent.

### 6.1 Defined ToolTypes

| ToolType | Format | Description | Default |
|----------|--------|-------------|---------|
| `ITIDY_TEXT_AREA` | `x,y,w,h` | Safe area rectangle in pixels (origin = top-left of icon image) | See Section 10 |
| `ITIDY_LINE_HEIGHT` | integer | Height of one rendered text line in pixels | `1` |
| `ITIDY_LINE_GAP` | integer | Vertical gap between lines in pixels | `1` |
| `ITIDY_MAX_LINES` | integer | Maximum number of lines to render (overrides calculated value) | calculated from safe area |
| `ITIDY_CHAR_WIDTH` | `1` or `2` | Pixels per character (`0` = auto-select) | `0` (auto) |
| `ITIDY_BG_COLOR` | palette index or `-1` | Background colour index in icon palette. `-1` = no background (preserve template pixels) | auto: lightest palette entry |
| `ITIDY_TEXT_COLOR` | palette index | Text colour index in icon palette | auto: darkest palette entry |
| `ITIDY_READ_BYTES` | integer | Maximum bytes to read from source file | `4096` |

### 6.2 Parsing Example

```
ToolType string:  ITIDY_TEXT_AREA=6,4,36,28
Parsed as:        safe_left=6, safe_top=4, safe_width=36, safe_height=28
```

### 6.3 Stamped ToolTypes on Generated Icons

Icons created by iTidy will have these ToolTypes added (per the safeguards
in `docs/iTidy_Future_Content_Aware_Icons_TODO.md`):

| ToolType | Value | Purpose |
|----------|-------|---------|
| `ITIDY_CREATED` | `1` | Marks icon as iTidy-generated |
| `ITIDY_KIND` | `text_preview` / `iff_thumbnail` / `font_preview` | Type of preview |
| `ITIDY_SRC` | filename | Source file that was previewed |
| `ITIDY_SRC_SIZE` | bytes | Source file size at generation time |
| `ITIDY_SRC_DATE` | datestamp | Source file datestamp at generation time |

These allow iTidy to detect its own generated icons and decide whether
regeneration is needed (e.g. source file changed) or whether to skip
(source unchanged, icon already current).

### 6.4 EXCLUDETYPE ToolType (in `def_ascii.info`)

The `EXCLUDETYPE` ToolType is placed in `PROGDIR:Icons/def_ascii.info` and
contains a comma-separated list of DefIcons type tokens that should **not**
receive a text preview, even though they are classified as ASCII by DefIcons.

| Detail | Value |
|--------|-------|
| **ToolType name** | `EXCLUDETYPE` |
| **Format** | Comma-separated type tokens, e.g. `amigaguide,html,install` |
| **Case sensitivity** | Case-insensitive matching |
| **Whitespace** | Leading/trailing spaces around each token are trimmed |
| **Location** | ToolTypes of `PROGDIR:Icons/def_ascii.info` |
| **Read by** | `is_excluded_from_text_preview()` in `src/icon_edit/icon_content_preview.c` |

**Example ToolType value:**
```
EXCLUDETYPE=amigaguide,html,install
```

**Behaviour when a type is excluded:**
- The file still receives an icon (the plain template is copied as normal).
- The text preview rendering step is skipped entirely.
- The icon is saved without any `ITIDY_KIND` or `ITIDY_SRC_*` stamp ToolTypes
  (because no rendering was done).
- The log records: `Type '<token>' excluded from text preview via EXCLUDETYPE tooltype`

**Typical use cases:**
- `amigaguide` — AmigaGuide documents have their own distinctive system icon and
  the guide markup syntax would look messy as a raw text preview.
- `html` — HTML source contains angle-bracket tags; the raw markup preview is
  not very informative and HTML files often have dedicated artwork.
- `install` — Installer scripts are binary-ish and their preview adds no value.

**Note:** `EXCLUDETYPE` is read from `def_ascii.info` regardless of which
template level is ultimately selected for the file. It acts as a global
exclusion list for the entire text preview system.

---

## 7. Output Format Decision

**Decision: Always save as OS3.5 palette-mapped format.**

Rationale:

- `IconControlA()` with `ICONCTRLA_Set*` tags works natively with chunky
  palette-mapped data — no manual bit-plane conversion needed
- OS3.5 format is the modern standard for Workbench 3.2+
- Simplifies the write path to a single code path
- Template icons on WB 3.2 systems are almost always OS3.5 already
- Old planar/NewIcon data in the template is stripped on save

### Save Tags Used

```c
PutIconTagList(path, icon,
    ICONPUTA_NotifyWorkbench,      TRUE,    /* Tell WB to refresh */
    ICONPUTA_DropPlanarIconImage,  TRUE,    /* Drop old planar data */
    ICONPUTA_DropNewIconToolTypes, TRUE,    /* Drop NewIcon IM1=/IM2= */
    TAG_END);
```

This ensures the saved icon contains only the clean OS3.5 IFF `FORM ICON`
image data, regardless of what format the template originally used.

### User-Visible Note

**Preview icons are stored in OS3.5+ icon format.** If the template icon
was a classic planar or NewIcon-format icon, the saved preview icon will be
converted to OS3.5 palette-mapped format. On Workbench 3.2 (the minimum
target) this is fully supported and generally desirable.

Some users may prefer a consistent classic look across all icons. A future
"preserve template format" advanced option could be added, but is out of
scope for v1 of this feature.

---

## 8. Selected Image Handling

### Core Rule: Honour the Template

The code **respects what the template provides**:

- If the template icon **has** a selected image → render into both images
- If the template icon **does not** have a selected image → render into
  the normal image only; leave `ImageData2` as NULL

When no selected image is present, **Workbench automatically dims the
normal image** when the icon is selected. This is the standard Workbench
behaviour and looks correct without any extra work.

The current built-in template (`PROGDIR:Icons/text_template.info`) does
**not** have a selected image, so the initial implementation will only
render into ImageData1.

### Strategy When Template HAS a Selected Image

If a future or user-supplied template provides both image states:

1. Render the content preview into the **normal** pixel buffer (ImageData1)
2. **Copy** the entire normal pixel buffer to create the **selected** buffer
   (ImageData2), preserving any template border artwork outside the safe area
3. **Swap text and background palette indices within the safe area only** in
   the selected buffer:
   - Every pixel in the safe area that is `bg_color_index` becomes
     `text_color_index`, and vice versa
   - Pixels outside the safe area are untouched — template border art
     (folded corner, decorative frame, etc.) is preserved
4. Both images show identical text content — only the safe area is inverted
5. The selected palette is a **copy** of the normal palette (no modification)
   — the visual difference comes from the swapped indices, not from changed
   RGB values

This approach avoids the contrast/readability problems that palette darkening
can cause, and it respects the template's border artwork.

### Strategy When Template Has NO Selected Image

Do nothing. Leave `pixel_data_selected` as NULL in the `iTidy_IconImageData`
struct. The `itidy_icon_image_apply()` function skips `ICONCTRLA_SetImageData2`
and related tags when the selected buffer is NULL. Workbench handles the
rest.

### Why Not Always Generate a Selected Image?

- The Workbench auto-dim behaviour is well-understood and expected by users
- Fabricating a selected image for icons that don't have one adds complexity
  for no visual benefit
- It keeps the saved icon file smaller
- It matches the template's intent — if the template author chose not to
  include a selected image, we respect that choice

---

## 9. Palette Strategy

### Template Palette Preservation

The template icon's palette is preserved as-is. We only need two specific
colours from it for the text rendering:

- **Background colour** — overridable via `ITIDY_BG_COLOR` ToolType.
  Set to `-1` to skip background fill entirely (preserves template pixels).
- **Text colour** — overridable via `ITIDY_TEXT_COLOR` ToolType

### Default Colour Selection (When ToolTypes Are Absent)

If the template icon does not provide `ITIDY_BG_COLOR` / `ITIDY_TEXT_COLOR`,
the module scans the palette and picks:

- **Background** — the palette entry with the **highest luminance**
  (lightest colour). Luminance approximation:
  `lum = 0.299 * R + 0.587 * G + 0.114 * B`
- **Text** — the palette entry with the **lowest luminance** (darkest colour)

This heuristic is cheap (one pass over ≤256 entries), avoids hardcoded
assumptions about palette ordering, and produces sensible results on any
colour scheme — standard Workbench palettes, MagicWB, custom themes, etc.

### Why Palette Indices Rather Than RGB Values

Using palette indices means:

- Zero palette modification needed — we paint with colours already in the
  template icon's palette
- The icon's visual style (colour scheme, depth) is entirely controlled by
  the template the user chooses
- Works correctly regardless of screen depth or palette remapping

### Palette for Selected Image

The selected image uses an **unmodified copy** of the normal palette. The
visual difference in the selected state comes from swapping the text and
background palette *indices* within the safe area (see Section 8), not
from modifying RGB values.

---

## 10. Safe Area Defaults

If `ITIDY_TEXT_AREA` is not present in the template icon's ToolTypes, the
module calculates a default safe area based on the icon dimensions:

```
Margin = 4 pixels on each side (top, bottom, left, right)

safe_left   = 4
safe_top    = 4
safe_width  = icon_width  - 8
safe_height = icon_height - 8
```

This leaves a border around the edge of the icon that preserves any
decorative frame/border the template may have (e.g. a "page" outline with
a folded corner).

### Example Defaults by Common Icon Sizes

| Icon Size | Safe Area | Max Lines (1px+1px gap) | Max Chars (1px wide) |
|-----------|-----------|------------------------|---------------------|
| 32 × 32 | 4,4,24,24 | 12 | 24 |
| 48 × 48 | 4,4,40,40 | 20 | 40 |
| 64 × 64 | 4,4,56,56 | 28 | 56 |
| 80 × 80 | 4,4,72,72 | 36 | 72 |

---

## 11. Character-to-Pixel Mapping

### Auto-Select Logic

When `ITIDY_CHAR_WIDTH` is `0` (default) or absent:

```
Default: char_pixel_width = 1

Only promote to 2 if safe_width >= 64
(i.e. the safe area alone is at least 64 pixels wide)
```

For typical Workbench icons (32–80px wide), the safe area after margins
will be 24–72px. This means:

- **32px icon** (24px safe) → 1px per char (24 chars/line) — dense mini-page
- **48px icon** (40px safe) → 1px per char (40 chars/line) — good balance
- **64px icon** (56px safe) → 1px per char (56 chars/line) — still 1px
- **80px icon** (72px safe) → 2px per char (36 chars/line) — bolder text

**1 pixel per character is the intended default aesthetic** — the "miniature
zoomed-out page of text" look. The 2px mode exists only for unusually large
icons where individual character strokes are beneficial, and can always be
forced via the `ITIDY_CHAR_WIDTH=2` ToolType.

The user can also force `ITIDY_CHAR_WIDTH=1` on any template to override
the auto-selection and keep the dense look regardless of icon size.

### What Gets Rendered

| Byte Value | Rendering |
|------------|-----------|
| 0x20 (space) | Background colour (gap) |
| 0x09 (tab) | Background colour, advance to next tab stop (every 8 chars) |
| 0x21–0x7E (printable ASCII) | Text colour, `char_pixel_width` pixels wide |
| 0xA0–0xFF (Latin-1 printable) | Text colour, `char_pixel_width` pixels wide |
| 0x0A (newline) | End of line, advance to next row |
| 0x0D (CR) | Ignored (handle CR+LF gracefully) |
| All other control chars | Ignored / treated as space |

### Text Rendering Modes

iTidy supports three rendering modes for text icons, providing progressively
more sophisticated visual quality:

#### 1. Binary Mode (Classic Two-Color)

The original rendering mode. Each pixel is either:
- **Text colour** (darkest palette entry) for printable characters
- **Background colour** (lightest palette entry) for spaces/whitespace

This produces a stark, high-contrast "bitmap" look. Works well for icons
with pure black text on white backgrounds.

#### 2. Three-Color Threshold Mode (Anti-Aliased)

Introduces a **mid-tone grey** colour for anti-aliasing, producing smoother
text rendering. Uses a count-based accumulator:

- **Count 0** (no characters) → Background colour
- **Count 1** (single character) → Mid-tone grey (anti-aliasing)
- **Count 2+** (multiple characters) → Text colour (solid black)

This mode requires the palette to include:
- Lightest entry (background/white)
- Darkest entry (text/black)
- Mid-luminance entry (grey anti-aliasing)

The mid-tone colour is selected automatically by finding the palette entry
closest to the luminance midpoint between the darkest and lightest colours.
It can be overridden via the `ITIDY_MID_COLOR` ToolType.

**Result:** Smoother character edges, particularly visible on ASCII art and
small text, without requiring additional palette entries beyond what most
templates already provide.

#### 3. Adaptive Background-Aware Mode (Content-Sensitive)

**Added:** 2026-02-09

The most sophisticated rendering mode. Instead of using fixed text and
background colours, the renderer **samples the background pixel** it's about
to overwrite and chooses a text colour that is **darkened by a configurable
percentage** (default 70% darker, keeping 30% of the original brightness).

This allows text to "adapt" to gradient backgrounds or template artwork:

- **Dark background areas** → Text is slightly darker (subtle contrast)
- **Light background areas** → Text is much darker (high contrast)
- **Gradient backgrounds** → Text smoothly transitions from dark to darker

**Performance:** Uses a pre-computed lookup table (`darken_table[256]`)
that maps each palette index to its darkened equivalent. The table is built
once before rendering (~400ms on 68000) and then each pixel lookup is instant.

**Visual Result:** Text that feels "printed on" the background rather than
simply overlaid. Particularly effective for templates with left-to-right or
top-to-bottom colour gradients. **30% darkening (70% of original brightness
remaining) produces excellent results on grey-scale palettes.**

**Enabling Adaptive Mode:**

```
ITIDY_ADAPTIVE_TEXT=YES
ITIDY_DARKEN_PERCENT=70
```

The darken percentage can range from 1–100, where:
- **100** = completely black (original brightness removed)
- **70** = keep 30% of original brightness **(recommended)**
- **50** = keep 50% of original brightness (light darkening)
- **30** = keep 70% of original brightness (very subtle)

**Compatibility:** Requires Workbench 3.2+ and works best with icons that
have smooth gradient backgrounds. Falls back to 3-color threshold mode if
disabled.

---

## 12. Performance & Safety

### File Reading Limits

- Read at most **4096 bytes** from the source file (overridable via
  `ITIDY_READ_BYTES`)
- Read at most **20 lines** (overridable via `ITIDY_MAX_LINES` or
  calculated from safe area)
- Whichever limit is hit first terminates reading

### Binary File Detection

Before rendering, scan the first 256 bytes of the source file and classify
each byte. A byte is considered **acceptable** if it falls into any of
these ranges:

- `0x09` (TAB)
- `0x0A` (LF)
- `0x0D` (CR)
- `0x20–0x7E` (printable ASCII)
- `0xA0–0xFF` (printable ISO-8859-1 / Latin-1)

If more than **10%** of the scanned bytes fall **outside** the acceptable
set, treat the file as binary and **skip preview generation** — the icon
keeps the unmodified template image.

This aligns the binary detection exactly with the character set the renderer
actually handles, avoiding false positives on Latin-1 text files.

### Memory Budget

Per icon generation:

- Pixel buffer: `width × height` bytes × 2 (normal + selected) — typically
  ~4–12 KB for a 48–80px icon
- Palette: ~768 bytes × 2 (256 entries × 3 bytes RGB × 2 states)
- File read buffer: 4 KB
- **Total: ~15–20 KB peak per icon**, freed immediately after save

All allocations via `whd_malloc()` for tracking. Check every allocation for
NULL and bail cleanly on failure.

### Workbench Notification

`ICONPUTA_NotifyWorkbench = TRUE` ensures the Workbench refreshes the icon
display after we save. This avoids stale cached icon images.

---

## 13. Debug & Development Aids

### Pixel Buffer Dump to Log

During development, the pixel buffer is dumped to the icon log (`LOG_ICONS`)
immediately **after the renderer finishes** and **before**
`itidy_icon_image_apply()` — so the dump captures exactly what the renderer
produced, regardless of whether the icon save step works or corrupts data.

Guarded by a compile-time define in `src/icon_edit/icon_image_access.h`:

```c
#define DEBUG_ICON_DUMP
```

When disabled, the dump function compiles to nothing (zero overhead).

### Dump Format

**Palette (one line):**
```
[ICON_DUMP] 40x38 pixels, palette: 8 entries
[ICON_DUMP] Palette: 0=959595 1=000000 2=FFFFFF 3=3B67A2 4=7B7B7B 5=A0A0A0 6=D4D4D4 7=505050
```

**Safe area (one line):**
```
[ICON_DUMP] Safe area: left=4 top=4 width=32 height=30
```

**Pixel grid — one hex digit per pixel (palettes ≤ 16 colours):**
```
[ICON_DUMP] --- Normal image ---
[ICON_DUMP] 3333333333333333333333333333333333333333
[ICON_DUMP] 3222222222222222222222222222222222222223
[ICON_DUMP] 3211111222112221122211121122222222222223
[ICON_DUMP] 3222222222222222222222222222222222222223
[ICON_DUMP] 3211221112211122211222111222222222222223
[ICON_DUMP] ...
```

Each row is one line of pixels. Runs of `1` (text index) on a `2`
(background index) background represent rendered characters. The `3`
border is the template's decorative frame. Corruption, off-by-one
errors, and safe area overruns are immediately visible.

**Fallback for palettes > 16 colours:** Two hex digits per pixel,
space-separated:
```
[ICON_DUMP] 03 02 02 02 02 02 02 02 02 02 03
```

### Source Text Echo

Alongside the pixel dump, log the first few lines of the raw source text
that was fed to the renderer. This lets you cross-check that the pixel
pattern matches the expected character layout:

```
[ICON_DUMP] --- Source text (first 5 lines) ---
[ICON_DUMP] ; Example config file
[ICON_DUMP] ToolTypes=YES
[ICON_DUMP] DefaultTool=SYS:Utilities/More
[ICON_DUMP]
[ICON_DUMP] # End of header
```

### Dump Function

```c
void itidy_icon_image_dump(
    const iTidy_IconImageData *img,
    const iTidy_RenderParams *params,   /* for safe area annotation */
    const char *source_path             /* echo first N lines of source file */
);
```

Called from the Phase 3 orchestration code, between the renderer call and
the apply call:

```
   e. Call renderer
   → itidy_icon_image_dump(&img, &params, source_path)   ← DEBUG ONLY
   f. If template had selected image: build selected image
   g. Apply modified image
```

### When to Remove

This is a development/debugging aid. It should be:
- **Enabled** during all Phase 1 and Phase 2 development and testing
- **Disabled** (comment out `#define DEBUG_ICON_DUMP`) before any release
  build or performance benchmarking
- **Kept in the source** permanently — it costs nothing when disabled and
  will be valuable again whenever a new renderer is developed

---

## 14. Future Reuse — Other Icon Types

The **Icon Image Access module** (Phase 1) is deliberately format-agnostic
and renderer-agnostic. Future content-aware icon features plug in as new
renderers that operate on the same `iTidy_IconImageData` pixel buffer:

### IFF Image Thumbnails

- Renderer: `itidy_render_iff_thumbnail(path, params)`
- Reads an IFF ILBM file, decodes to chunky pixels, scales down to fit the
  safe area
- May need to quantise colours to fit the icon's palette, or build a new
  palette from the image
- Uses the shared `iTidy_RenderParams` base struct (already defined), with
  an IFF-specific extension struct if extra fields are needed

### Font Preview Icons

- Renderer: `itidy_render_font_preview(path, params)`
- Opens a font (bitmap or outline), renders sample glyphs ("Aa" / "Ag")
  into the safe area
- Uses text and background palette indices like the ASCII renderer

### Badge Overlays

- Renderer: `itidy_render_badge(icon_image, badge_type)`
- Paints a small overlay (e.g. "TXT", "IMG", "LHA") in a corner of the
  icon image
- Operates on the pixel buffer after the main content renderer has finished
- Could be stacked on top of any preview type

### Automatic Palette Expansion (Planned Enhancement)

**Goal:** Allow users to provide simple 8-16 color template icons and have
iTidy automatically generate smooth grey ramps for high-quality adaptive
text rendering.

**Current limitation:** Adaptive text rendering quality depends entirely
on the palette richness of the template icon. Users must manually create
32-64 color palettes in an icon editor to achieve smooth gradient adaptation.

**Proposed solution:** Add a palette expansion function that:

1. **Extracts existing template palette** (e.g., 10 colors with border artwork)
2. **Identifies key colors:**
   - Lightest color (background)
   - Darkest color (text)
   - Border/decorative colors (preserve these)
3. **Generates smooth grey ramp:**
   ```c
   for (i = 0; i < 32; i++)
   {
       UBYTE grey = 255 - (i * 8);  // 255, 247, 239, ..., 8, 0
       new_palette[i] = RGB(grey, grey, grey);
   }
   ```
4. **Relocates border colors** to higher palette indices (32-41)
5. **Updates pixel buffer** to use new indices for border pixels
6. **Expands palette size** to 32-48 entries

**Algorithm: Smart Palette Reorganization**

```
Input:  10-color template with border artwork
Output: 48-color expanded palette

Step 1: Analyze existing palette
  - Scan all pixels to determine which colors are used
  - Classify as "text area" (inside safe rect) vs "border" (outside)
  - Identify lightest/darkest based on luminance

Step 2: Build new palette layout
  Entries 0-31:  Smooth grey ramp (white → black)
  Entries 32-41: Original border colors (preserved RGB values)
  Entries 42-47: Reserved for future accent colors

Step 3: Build index translation table
  old_to_new[original_bg_index]   = 0    (white end of ramp)
  old_to_new[original_text_index] = 31   (black end of ramp)
  old_to_new[border_color_0]      = 32
  old_to_new[border_color_1]      = 33
  ... and so on

Step 4: Remap pixel buffer
  for each pixel in icon image:
    pixel = old_to_new[pixel]

Step 5: Update render params
  bg_color_index   = 0   (white)
  text_color_index = 31  (black)
  mid_color_index  = 15  (mid-grey)
```

**Benefits:**

- **User-friendly:** Users provide simple templates, iTidy handles complexity
- **Consistent quality:** Every icon gets optimal palette for adaptive rendering
- **Backward compatible:** Works with existing 10-color templates (no re-design needed)
- **Performance:** One-time palette expansion cost (~50ms on 68030)
- **File size:** Minimal increase (~1KB for 38 extra palette entries)

**Implementation location:**

- New function: `itidy_expand_palette_for_adaptive()` in `src/icon_edit/icon_image_access.c`
- Called after `itidy_icon_image_extract()`, before rendering
- Modifiable by ToolType: `ITIDY_EXPAND_PALETTE=YES` (default ON for adaptive mode)

**Edge cases to handle:**

1. **Template already has 32+ colors:** Skip expansion, use as-is
2. **Non-grey border colors:** Preserve exact RGB values, don't convert to grey
3. **Gradient backgrounds:** Detect and preserve gradient, only expand grey range
4. **Selected image:** Apply same index remapping to both normal and selected buffers

**Performance estimate:**

| Step | Time (68000 @ 7.14 MHz) | Time (68030 @ 25 MHz) |
|------|-------------------------|------------------------|
| Palette analysis | ~10ms | ~3ms |
| Grey ramp generation | ~15ms | ~4ms |
| Pixel remapping (42×46) | ~25ms | ~7ms |
| **Total** | **~50ms** | **~14ms** |

**Future enhancement:** Smart color interpolation between existing colors
instead of pure grey ramp, preserving the template's color "theme" while
adding intermediate shades.

### Adding a New Renderer — Checklist

1. Create `src/icon_edit/icon_<type>_render.c/.h`
2. Implement a function matching the pattern:
   `BOOL itidy_render_<type>(const char *source_path, iTidy_RenderParams *params)`
3. Add the new type to the renderer dispatch in the DefIcons integration
4. Define any type-specific ToolTypes (following the `ITIDY_` prefix convention)
5. Add the new `ITIDY_KIND` value for stamping generated icons
6. Update this document

---

## 15. API Surface Summary

### Icon Image Access (`src/icon_edit/icon_image_access.h`)

```
itidy_icon_image_extract()       — Extract pixels + palette from DiskObject
itidy_icon_image_apply()         — Write pixels + palette back to DiskObject
                                   (skips ImageData2 tags if selected buffer is NULL)
itidy_icon_image_save()          — Save icon to disk as OS3.5 format
itidy_icon_image_free()          — Free pixel/palette buffers
itidy_icon_image_create_blank()  — Create empty pixel buffer at given size
itidy_icon_image_dump()          — Dump pixel buffer + palette to LOG_ICONS (DEBUG_ICON_DUMP only)
```

### Generic Render Parameters (`src/icon_edit/icon_image_access.h`)

```
iTidy_RenderParams               — Base struct: pixel buffer, safe area, colour indices
iTidy_TextRenderParams           — Extends base with char_pixel_width, line_gap, etc.
(future) iTidy_IFFRenderParams   — Extends base with IFF-specific fields
(future) iTidy_FontRenderParams  — Extends base with font sample text, size, etc.
```

### ASCII Text Renderer (`src/icon_edit/icon_text_render.h`)

```
itidy_render_ascii_preview()     — Read file, paint text into pixel buffer
```

### ToolType Helpers (`src/icon_edit/icon_image_access.h`)

```
itidy_parse_safe_area()          — Parse ITIDY_TEXT_AREA from ToolTypes
itidy_get_render_params()        — Build render params from icon ToolTypes + defaults
itidy_resolve_palette_indices()  — Find bg/text indices (ToolType or lightest/darkest)
itidy_stamp_created_tooltypes()  — Add ITIDY_CREATED/KIND/SRC ToolTypes to icon
itidy_check_cache_valid()        — Check if existing icon matches source (cache-skip)
```

---

## 16. Metadata Preservation — Test Checklist

The icon editing pipeline must **never alter non-image metadata**. The
following properties must survive a load → modify image → save cycle
unchanged:

| Property | How to Verify |
|----------|---------------|
| **DefaultTool** | Compare `do_DefaultTool` before and after |
| **ToolTypes** (user-defined) | All original ToolTypes present and unmodified (new `ITIDY_*` entries are *added*, not replacing) |
| **Stack size** | Compare `do_StackSize` |
| **Position** (`do_CurrentX`, `do_CurrentY`) | Compare before/after (note: `NO_ICON_POSITION` = 0x80000000 must be preserved if set) |
| **Protection bits** | File protection flags on the `.info` file itself |
| **Type** (`do_Type`) | `WBPROJECT`, `WBTOOL`, `WBDRAWER`, etc. — must not change |
| **DrawerData** | If icon is a drawer, `do_DrawerData` (window position, size, flags) must be preserved |
| **Workbench type byte** | The `wbType` field used by icon.library |

### Test Strategy

1. Create a test icon with known values for every field above
2. Run it through the full pipeline: extract → render → apply → save
3. Reload the saved icon and compare every field
4. Automate as a host-build test in `src/tests/` if possible, or manual
   test on WinUAE

This should be validated **early in Phase 1** — before any renderer work
begins — to catch metadata corruption issues at the source.

---

## 17. Open Questions / Decisions Log

| # | Question | Decision | Date |
|---|----------|----------|------|
| 1 | Output format: preserve original or always OS3.5? | **Always OS3.5 palette-mapped.** Document as: "Preview icons are stored in OS3.5+ icon format." Future option for format preservation is out of scope for v1. | 2026-02-09 |
| 2 | Safe area: hardcoded or ToolType-driven? | **ToolType-driven with sensible defaults** (4px margin) | 2026-02-09 |
| 3 | Character pixel width: fixed or auto? | **Default 1px always.** Auto-promote to 2px only if safe_width ≥ 64. ToolType override available. | 2026-02-09 |
| 4 | Selected image palette treatment? | **Safe-area-only index swap** (swap bg ↔ text indices within safe rect). No palette RGB modification. Border art untouched. | 2026-02-09 |
| 5 | Should binary files get any preview? | No — skip preview, keep template image unchanged | 2026-02-09 |
| 6 | Generic `iTidy_RenderParams` vs per-renderer structs? | **Generic base struct from day one** (`iTidy_RenderParams`). Per-renderer structs embed it. No refactor needed later. | 2026-02-09 |
| 7 | ToolType parsing: icon_image_access or separate module? | **In `icon_image_access.h`** — keeps all icon-related helpers in one place. | 2026-02-09 |
| 8 | Default palette indices when ToolTypes absent? | **Scan palette for lightest (→ bg) and darkest (→ text) entry** using luminance formula. No hardcoded index assumptions. | 2026-02-09 |
| 9 | Binary detection: what counts as non-binary? | **TAB, LF, CR, 0x20–0x7E, 0xA0–0xFF.** >10% outside = binary. Aligned with ISO-8859-1 render set. | 2026-02-09 |
| 10 | Cache-skip: where does the check live? | **Phase 3 (DefIcons pipeline), before file copy.** Check ITIDY_CREATED + SRC_SIZE + SRC_DATE on existing target icon. | 2026-02-09 |
| 11 | Integration module: does `deficons_creation.c` exist? | **Yes** — confirmed. No rename needed. | 2026-02-09 |
| 12 | When to validate metadata preservation? | **Early in Phase 1**, before any renderer work. See Section 16 checklist. | 2026-02-09 |
| 13 | Where does new code live? | **`src/icon_edit/`** subfolder. Keeps plugin code separate from existing codebase. | 2026-02-09 |
| 14 | Where is the built-in text template? | **`Bin/Amiga/Icons/text_template.info`** (= `PROGDIR:Icons/text_template.info` on Amiga). | 2026-02-09 |
| 15 | Selected image: generate if template lacks one? | **No.** Honour the template — if no selected image exists, leave ImageData2 NULL. Workbench auto-dims. | 2026-02-09 |
| 16 | Debug pixel dump to log? | **Yes.** Dump pixel buffer as hex grid to `LOG_ICONS` after render, before apply. Guarded by `#define DEBUG_ICON_DUMP`. See Section 13. | 2026-02-09 |
| 17 | Two-icon merge: why not load from the copied target? | **Old planar icons cannot provide palette-mapped data.** `ENVARC:Sys/def_ascii.info` on many systems is an old-format planar icon. We load a separate image template for pixel data and use the Workbench default for metadata only. See Section 18.2. | 2026-02-09 |
| 18 | `GetDiskObject()` vs `GetIconTagList()` for image template? | **`GetIconTagList()` with `ICONGETA_RemapIcon=FALSE`.** `GetDiskObject()` does NOT activate palette-mapped image data — palette pointers come back NULL. See Section 18.3. | 2026-02-09 |
| 19 | Text scaling: 1:1 or downsampled? | **2:1 horizontal and vertical downsampling.** Fits ~68 columns × 38 lines into a 34×19 pixel output. See Section 19. | 2026-02-09 |
| 20 | Downscale merge rule: OR or AND semantics? | **Horizontal AND, vertical OR.** AND preserves word gaps in text. OR ensures multi-line merges don't lose content. See Section 19.2. | 2026-02-09 |
| 21 | Tab width for icon preview? | **4 characters** (not the traditional 8). Reduced because the icon safe area is tiny and 8-char tabs waste too much space. | 2026-02-09 |
| 22 | Debug dump log level? | **`log_info()`** (not `log_debug()`). The dump is already compile-time guarded by `DEBUG_ICON_DUMP`, so using INFO ensures it actually appears in the icons log at default log levels. | 2026-02-09 |
| 23 | How to preserve template artwork in safe area? | **`ITIDY_BG_COLOR=-1` mode.** Uses sentinel value 255 (`ITIDY_NO_BG_COLOR`) to skip background fill entirely. Selected image index swap also skipped in this mode. Allows template icons with pre-rendered artwork or special effects. | 2026-02-09 |
| 24 | Should iTidy auto-generate grey ramps for better adaptive rendering? | **Yes, enabled by default** (`ITIDY_EXPAND_PALETTE=YES`). Automatically expands palettes < 16 colors to 32 colors when adaptive mode is enabled. Preserves original colors at indices 0-N, adds grey ramp at N-31. Zero pixel remapping needed. Provides smooth adaptive text on gradients with minimal overhead (~34ms on 68000, +200 bytes per icon). Implemented 2026-02-10. See `Palette_Expansion_Implementation_Plan.md`. | 2026-02-10 |

---

## 18. Implementation Status & Lessons Learned

**Status as of 2026-02-09:** All three phases are **implemented, compiled,
and tested on real hardware (WinUAE).** Icons render correctly as OS3.5
palette-mapped colour icons with visible text content previews.

### 18.1 Implementation Summary

| Phase | Files | Status | Lines (approx) |
|-------|-------|--------|-----------------|
| **Phase 1** — Icon Image Access | `src/icon_edit/icon_image_access.h` (412), `icon_image_access.c` (~1174) | ✅ Complete | ~1586 |
| **Phase 2** — ASCII Text Renderer | `src/icon_edit/icon_text_render.h` (110), `icon_text_render.c` (~480) | ✅ Complete | ~590 |
| **Phase 3** — Orchestration | `src/icon_edit/icon_content_preview.h` (142), `icon_content_preview.c` (~440) | ✅ Complete | ~582 |
| **Integration** | Modified `src/deficons_creation.c` (~825 total) | ✅ Complete | ~10 lines added |
| **Build** | `Makefile` — `ICON_EDIT_SRCS` variable | ✅ Complete | 3 lines added |

**Total new code:** ~2758 lines across 6 new files plus minor integration
edits.

### 18.2 Critical Design Change: Two-Icon Merge Architecture

The original plan (Section 5.1) assumed the pipeline would:
1. Copy the Workbench template `.info` to the target
2. Load the copied icon
3. Extract pixel data from it
4. Render into its pixel buffer
5. Save

**This failed in practice.** The Workbench default icon for ASCII files
(`ENVARC:Sys/def_ascii.info`) on many systems is an **old planar format**
icon. Old planar icons cannot provide palette-mapped pixel data via
`IconControlA()` — even after calling `LayoutIconA()` to attempt conversion,
the palette pointers came back NULL:

```
"missing essential data (w=42 h=46 pix1=0x4102f292 pal1=0 palsize=0)"
```

**Solution: Two-icon merge.** The implementation uses two separate icons:

1. **Target icon** (`ENVARC:Sys/def_ascii.info` or whichever Workbench
   default the DefIcons system resolves) — provides **metadata only**:
   DefaultTool, ToolTypes, icon type (`WBPROJECT`), stack size, etc.
   Loaded with the standard `GetDiskObject()`.

2. **Image template** (`PROGDIR:Icons/text_template`) — provides **pixel
   data only**: a custom OS3.5 palette-mapped colour icon designed for
   content preview rendering. Loaded with `GetIconTagList()`.

The pipeline extracts pixels from the image template, renders the text
preview into them, then applies the modified pixels onto a `DupDiskObjectA()`
clone of the target icon. The result has the Workbench metadata from icon #1
and the colour image from icon #2.

```
Target icon (ENVARC:Sys/def_ascii.info)
  └── Provides: DefaultTool, ToolTypes, icon type, stack, position
       └── DupDiskObjectA() clone receives the rendered pixel data

Image template (PROGDIR:Icons/text_template)
  └── Provides: Palette-mapped pixel data, palette, safe area
       └── GetIconTagList() with ICONGETA_RemapIcon=FALSE
       └── Pixel data extracted, rendered into, then applied to clone
```

### 18.3 Critical API Discovery: `GetIconTagList()` vs `GetDiskObject()`

**`GetDiskObject()` does NOT fully activate palette-mapped image data.**

This was discovered during testing. When loading the image template with
`GetDiskObject()`, the icon loaded successfully but `IconControlA()` with
`ICONCTRLA_GetPalette1` returned NULL — the palette-mapped data was never
"put to use" by icon.library.

The AmigaOS AutoDocs for `icon.library` confirm this:

> "palette mapped image data that was never put to use (this happens if
> it is retrieved with GetDiskObject() rather than the new
> GetIconTagList() call)"

**Fix:** Load the image template using `GetIconTagList()` with these tags:

```c
ICONGETA_RemapIcon         = FALSE  /* Get raw palette indices */
ICONGETA_GenerateImageMasks = FALSE /* Skip mask generation */
```

`ICONGETA_RemapIcon=FALSE` is critical — without it, icon.library would
remap the pixel data to screen colours, making the palette indices unusable
for modification and save.

### 18.4 Image Template Details

The built-in text template (`Bin/Amiga/Icons/text_template.info`) has:

| Property | Value |
|----------|-------|
| Dimensions | 42 × 46 pixels |
| Format | OS3.5 palette-mapped |
| Palette | 10 entries |
| Selected image | Yes (both image states) |
| Border art | Grey gradient frame around a white interior |

**Palette entries (from actual icon):**

| Index | RGB | Description |
|-------|-----|-------------|
| 0 | `000000` | Black (border outer) |
| 1 | `AAAAAA` | Light grey |
| 2 | `FFFFFF` | **White — background (lightest)** |
| 3 | `E3E3E3` | Very light grey |
| 4 | `C6C6C6` | Light-medium grey |
| 5 | `8A8A8A` | Medium grey |
| 6 | `6A6A6A` | Medium-dark grey |
| 7 | `292929` | Dark grey |
| 8 | `494949` | Dark grey |
| 9 | `000000` | **Black — text (darkest)** |

The palette auto-detection correctly identifies index 2 (white) as
background and index 9 (black) as text colour via the luminance heuristic.

**Safe area** (default, no ToolType override): `left=4, top=4, width=34,
height=38` — calculated as `(42-8) × (46-8)` with the standard 4px margin.

### 18.5 Actual Rendering Pipeline Flow (As Implemented)

This is the actual flow in `itidy_apply_content_preview()`, reflecting the
two-icon merge and the order of operations:

```
 1. Check type supports preview (itidy_is_text_preview_type)
 2. Load target_icon = GetDiskObject(source_path)       [Workbench metadata]
 3. Load image_icon = GetIconTagList(PROGDIR:Icons/text_template,
                        ICONGETA_RemapIcon=FALSE,
                        ICONGETA_GenerateImageMasks=FALSE) [pixel data]
 4. Extract pixel data from image_icon (itidy_icon_image_extract)
 5. Get render params from image_icon (itidy_get_render_params)
 6. FreeDiskObject(image_icon)                          [done with template]
 7. Render ASCII text into extracted pixel buffer
 8. Build selected image (safe-area index swap, if template had one)
 9. Debug dump pixel buffer to log (if DEBUG_ICON_DUMP)
10. Clone target_icon, apply modified pixels to clone (itidy_icon_image_apply)
11. Stamp ITIDY_CREATED/KIND/SRC ToolTypes on clone
12. Save clone to disk (itidy_icon_image_save) as OS3.5 format
13. DisposeObject(clone), FreeDiskObject(target_icon)
14. Free pixel/palette buffers (itidy_icon_image_free)
```

### 18.6 DefIcons Integration Point

The content preview is called from `deficons_creation.c` after a successful
`deficons_copy_icon_file()` for files (not drawers). The call site is:

```c
/* After deficons_copy_icon_file() succeeds, for files only: */
itidy_apply_content_preview(fullpath, type_token, fib->fib_Size, &fib->fib_Date);
```

The function returns one of:
- `ITIDY_PREVIEW_NOT_APPLICABLE` (0) — type doesn't support preview
- `ITIDY_PREVIEW_APPLIED` (1) — preview rendered and saved
- `ITIDY_PREVIEW_FAILED` (2) — rendering failed, icon keeps template image
- `ITIDY_PREVIEW_SKIPPED_CACHED` (3) — existing icon is already current

---

## 19. Text Downscaling — Fitting 80-Column Documents into Tiny Icons

### 19.1 The Problem

The original 1:1 renderer mapped 1 source character to 1 output pixel.
With a 34-pixel-wide safe area, this showed only the first 34 characters
of each line — roughly half of a typical 80-column Amiga text document.
ASCII art banners, formatted tables, and paragraph text were all cropped
to the left-hand side.

### 19.2 Solution: Automatic Downsampling with AND/OR Semantics

The renderer now **automatically calculates** the optimal downscaling ratio
based on the safe area dimensions. Two compile-time target constants in
`src/icon_edit/icon_text_render.h` define the desired text coverage:

```c
#define ITIDY_TARGET_COLUMNS  68   /* Aim to show 68 columns of source text */
#define ITIDY_TARGET_LINES    38   /* Aim to show 38 lines of source text */
```

The renderer calculates the necessary scale factors at runtime:

```c
h_scale = ceiling(ITIDY_TARGET_COLUMNS / output_pixels)
v_scale = ceiling(ITIDY_TARGET_LINES / output_rows)
```

**This means smaller icons automatically increase downscaling** to fit more
text into less space, while larger icons use less aggressive downscaling
to maintain readability.

**Examples of automatic scaling:**

| Safe Width | Output Pixels | H-Scale | Columns Shown | Description |
|------------|---------------|---------|---------------|-------------|
| 34px | 34 | 2:1 | 68 | Standard icon (default template) |
| 20px | 20 | 4:1 | 80 | Smaller icon, more aggressive scaling |
| 10px | 10 | 7:1 | 70 | Tiny icon, maximum downscaling |
| 68px | 68 | 1:1 | 68 | Large icon, no downscaling needed |

With the default 42×46 icon template (safe area 34×38, line_step=2):

| Parameter | Without Scaling | With Auto-Scaling (2:1) |
|-----------|-----------------|-------------------------|
| Source columns visible | 34 | 68 (~85% of 80-col doc) |
| Source lines visible | 19 | 38 |
| Output pixel columns | 34 | 34 |
| Output pixel rows | 19 | 19 |

**Horizontal merge rule: AND semantics.**

For each 2-character source block that maps to one output pixel, the output
is foreground (text) ONLY if **all** source characters in the block are
printable. If **any** character is a space, the output stays as background.

This preserves visible word gaps: a source block like `[e, ]` (end of word
+ space) produces a background pixel, maintaining the visual rhythm of
words separated by whitespace. Without this, every text line becomes a
solid bar from left to right.

**Vertical merge rule: OR semantics.**

When multiple source lines map to the same output row (v_scale > 1), each
source line is processed independently. If ANY source line has printable
text in a column, the output pixel lights up. This is correct because
different lines should contribute their content, not cancel each other out.

### 19.3 Implementation: Line-Buffered State Map

The renderer uses a per-line state map (`UBYTE line_map[128]`) with three
states per output column:

| State | Meaning |
|-------|---------|
| 0 | No character seen yet in this scale-block |
| 1 | Printable character seen, no space — will paint foreground |
| 2 | Space seen — forces background (kills the pixel) |

At each newline, the map is flushed: columns in state 1 are painted as
foreground pixels, then the map is cleared for the next source line. The
vertical OR happens naturally because `paint_pixels()` only sets foreground
and never erases existing foreground from a previous source line.

### 19.4 Tab Handling

Tabs are expanded to `ITIDY_TAB_WIDTH` (4 characters, reduced from the
traditional 8 because the effective source width is only 68 characters).
Each position in the tab expansion is treated as whitespace and marks
its corresponding output column with state 2 (space / gap forced).

### 19.5 Vertical Scaling and Character Aspect Ratio

**Problem discovered 2026-02-11:** The initial implementation of vertical
scaling set `v_scale = h_scale`, meaning both horizontal and vertical
dimensions were compressed at the same ratio (e.g. 3:1 horizontal, 3:1
vertical). This caused two critical issues:

1. **Loss of fine detail** — ASCII art with circular designs or intricate
   patterns lost definition because 3×3 source pixels were averaged into
   each output pixel, blurring out the details
2. **Incorrect aspect ratio assumption** — The code assumed text characters
   have 1:1 aspect ratio (square pixels), but **Amiga text characters are
   approximately 2:1 tall-to-wide** (e.g. Topaz 8 font is 8 pixels tall ×
   4 pixels wide)

**Original problem investigation:** Looking at the log output for `sdw-life.txt`:

```
[INFO] file='...sdw-life.txt' safe=24x33 char_w=1 scale=3x1 src_chars=72 src_lines=33
```

This revealed:
- Safe area: 24 pixels wide × 33 pixels tall
- Horizontal scaling: **3:1** (72 source characters → 24 pixels)
- Vertical scaling: **1:1** (33 source lines → 33 pixels) ← **HARDCODED PROBLEM**

The code originally hardcoded `v_scale = 1` at `icon_text_render.c:484`,
while it auto-calculated horizontal scale to fit `ITIDY_TARGET_COLUMNS` (68)
into the safe area. This created an asymmetric compression ratio.

**Why it looked stretched:** The original ASCII art had:
- ~68-80 characters wide → compressed 3:1 horizontally (correct)
- ~38+ lines tall → NOT compressed at all (1:1 mapping) ← created stretch

The first attempted fix was to apply the same auto-scaling to vertical:
```c
v_scale = (ITIDY_TARGET_LINES + max_lines - 1) / max_lines;
```

However, this caused even worse problems — with `ITIDY_TARGET_LINES = 38`
and `safe_height = 33`, this produced `v_scale = 2`, leading to 3×3 block
compression that destroyed fine ASCII art details.

**Root cause (discovered after first fix failed):** Setting `v_scale = h_scale`
treated characters as if they were square, when they are actually rectangular.
This caused excessive vertical compression, losing the fine structure of ASCII
art like circles made from special characters.

**Solution implemented:** Change vertical scale calculation to account for
the 2:1 character aspect ratio:

```c
/* Calculate horizontal scale to fit target columns */
h_scale = (ITIDY_TARGET_COLUMNS + out_pixels_per_line - 1) / out_pixels_per_line;
if (h_scale < 1) h_scale = 1;

/* Calculate vertical scale at half the rate of horizontal scale
 * to account for 2:1 tall-to-wide character aspect ratio.
 * Example: h_scale=3 → v_scale=1 (3:1 horizontal, 1:1 vertical) */
v_scale = h_scale / 2;
if (v_scale < 1) v_scale = 1;
```

**Actual scaling examples:**

| h_scale | Old v_scale | New v_scale | Compression | Result |
|---------|-------------|-------------|-------------|--------|
| 2 | 2 (2×2) | 1 (2×1) | Reduced vertical blur | ✓ Preserves detail |
| 3 | 3 (3×3) | 1 (3×1) | **Reduced vertical blur** | ✓ Circle details visible |
| 4 | 4 (4×4) | 2 (4×2) | Reduced vertical blur | ✓ Good balance |
| 6 | 6 (6×6) | 3 (6×3) | Reduced vertical blur | ✓ Maintains proportion |

**Visual benefit:** With the corrected scaling (e.g. 3×1 instead of 3×3),
ASCII art circles and intricate designs maintain their definition because
vertical detail is preserved while still achieving the necessary horizontal
compression to fit 68-80 columns of text.

**Character aspect ratio explained:** Amiga's Topaz 8 font and most other
system fonts have roughly 2:1 height-to-width proportions. When text is
compressed 3:1 horizontally but only 1:1 vertically (via `v_scale = h_scale / 2`),
the visual result maintains correct aspect ratio because:
- 3 characters wide × 1 character tall (source)
- 1 pixel wide × 1 pixel tall (output)
- Character proportions: 3×(2:1) horizontally vs 1×(2:1) vertically = balanced

### 19.6 Visual Comparison

**Before (1:1, 34 chars × 19 lines — cropped):**
```
             ########  ########  ##
            ##       ###       ###
   ## #### #####     #   ########
  ######### ####    #   #     #####
       #######     ####      #####
        ###########################
        ############  ########  ###
        ###
         #             ########## #
         #
         #
         #                     ####
         #
         #     #
         # ####
         # ###        ####### ####
         #########  ###############
                  ##
```

**After (2:1 AND/OR, 68 chars × 38 lines — full document width):**
```
       #### #### #### ## #### ####
  # # ##   # ####    #  #    #    #
  ######  ##   ###  #####   ##   ##
     ##############################
     #      #####  ### ###       #
                                 #
                ### ##           #
      ##                       # #
     ####  ################# #####
                            #
 ### ### ## ###
 ##############
 ## ### # # # ### # # ## ## # ## #
 ### ########## # # ###  # # ### ##
 ### ####  ## ### # ####  # #### ##
 ############ ##### #### # ## ###
 ## ## # # # # # ### # # ### # ####
 ##### # ### ################ #####
 # ##### ########## ### # #### # #
```

The ASCII art header retains its recognisable shape, and body text
paragraphs show visible word spacing instead of solid bars.

### 19.6 Tuning the Target Coverage

The target constants are compile-time defines and easy to experiment with:

- `ITIDY_TARGET_COLUMNS=34` — Show 34 columns (1:1 scaling on standard icon, less on smaller)
- `ITIDY_TARGET_COLUMNS=68` — **Current default**, show ~68 columns (good for 80-col files)
- `ITIDY_TARGET_COLUMNS=80` — Show full 80-column width (more aggressive scaling)
- `ITIDY_TARGET_LINES=38` — **Current default**, good document coverage

**How it adapts to different icon sizes:**
- 34×38 safe area → 2:1 scaling (68 cols × 38 lines)
- 20×20 safe area → 4:1 h-scale, 2:1 v-scale (80 cols × 40 lines)
- 10×10 safe area → 7:1 h-scale, 4:1 v-scale (70 cols × 40 lines)
- 68×76 safe area → 1:1 scaling (68 cols × 38 lines, no downscaling)

For most Amiga text files (80-column formatted), `ITIDY_TARGET_COLUMNS=68`
provides the best balance of coverage and readability across different
icon sizes.

---

## 20. Template Icon ToolType Reference (For Icon Designers)

Template icons can be customised by adding ToolTypes that control how iTidy
renders content previews into them. All ToolTypes use the `ITIDY_` prefix
and are **optional** — sensible defaults are computed automatically.

### 20.1 ToolTypes Read from the Image Template

These ToolTypes are read from the image template icon
(`PROGDIR:Icons/text_template.info`) and control the rendering area and
style. They are parsed by `itidy_get_render_params()` in
`src/icon_edit/icon_image_access.c`.

| ToolType | Format | Default | Description |
|----------|--------|---------|-------------|
| `ITIDY_TEXT_AREA` | `x,y,w,h` | `4,4,(width-8),(height-8)` | The safe area rectangle where text is rendered. `x,y` is the top-left corner in pixels from the icon's top-left. `w,h` is the width and height in pixels. Everything outside this area is preserved (template border art, folded corner, decorative frame, etc.). |
| `ITIDY_EXCLUDE_AREA` | `x,y,w,h` | (none) | Optional exclusion zone **within** the safe area where rendering is skipped, preserving template artwork like folded page corners or decorative elements. If a pixel falls within this rectangle during rendering, it is left unchanged from the template. `x,y` is absolute position from icon's top-left, not relative to safe area. |
| `ITIDY_CHAR_WIDTH` | `0`, `1`, or `2` | `0` (auto) | Pixels per character. `0` auto-selects: 1px for safe widths < 64, 2px for ≥ 64. For typical 42-pixel icons this is always 1. |
| `ITIDY_LINE_HEIGHT` | integer | `1` | Height of one rendered text line in pixels. |
| `ITIDY_LINE_GAP` | integer | `1` | Vertical gap between rendered text lines in pixels. With `LINE_HEIGHT=1` and `LINE_GAP=1`, each line occupies 2 pixel rows (1 text + 1 blank), giving the "ruled paper" look. |
| `ITIDY_BG_COLOR` | palette index or `-1` | auto (lightest) | Which palette entry to use for the background (whitespace) colour. If not set, the palette is scanned and the entry with the highest luminance is chosen. **Special value `-1` skips background fill entirely**, preserving the template's existing pixel data in the safe area. |
| `ITIDY_TEXT_COLOR` | palette index | auto (darkest) | Which palette entry to use for text (foreground) pixels. If not set, the darkest palette entry is chosen via luminance. |
| `ITIDY_MID_COLOR` | palette index | auto (mid-luminance) | Grey/mid-tone colour for 3-color anti-aliasing mode. If not set, the palette entry with luminance closest to the midpoint between darkest and lightest is chosen. Used for smoother character edges. |
| `ITIDY_ADAPTIVE_TEXT` | `YES`, `TRUE`, `ON`, `1` | (off) | Enables adaptive background-aware text rendering. When enabled, text colour is dynamically chosen by darkening the background pixel by `ITIDY_DARKEN_PERCENT`. Produces text that adapts to gradient backgrounds. |
| `ITIDY_DARKEN_PERCENT` | `1`–`100` | `70` | Percentage to darken background pixels when `ITIDY_ADAPTIVE_TEXT` is enabled. `70` means "keep 30% of original brightness" (recommended). Higher values = darker text. Only used when adaptive mode is active. This controls the primary text darkening. |
| `ITIDY_DARKEN_ALT_PERCENT` | `1`–`100` | `35` | Percentage to darken background pixels for alternating lines, creating a "ruled paper" effect for better readability. Default is half of `ITIDY_DARKEN_PERCENT`. Set to the **same value** as `ITIDY_DARKEN_PERCENT` to disable the ruled effect. Lower values = lighter alternating lines = stronger ruled effect. Only used when adaptive mode is active. |
| `ITIDY_EXPAND_PALETTE` | `YES`, `NO` | `YES` | When adaptive text is enabled and the palette has fewer than 16 colors, automatically expand it to 32 colors by adding a smooth grey ramp. This provides better darkening matches for gradient backgrounds. Set to `NO` to preserve the original palette exactly as-is. Expanded palettes preserve original colors at indices 0-N, adding grey ramp at N-31, so existing border artwork is unaffected. |
| `ITIDY_READ_BYTES` | integer | `4096` | Maximum number of bytes to read from the source file. |

**⚠️ Important: When `ITIDY_ADAPTIVE_TEXT=YES` is enabled, the following
ToolTypes are IGNORED:**

- `ITIDY_BG_COLOR` — Background is sampled from the template's existing pixels
- `ITIDY_TEXT_COLOR` — Text color is dynamically calculated by darkening the background
- `ITIDY_MID_COLOR` — Graduated darkening via the lookup table provides smooth transitions

In adaptive mode, the renderer works by sampling the background pixel color
at each position and darkening it according to `ITIDY_DARKEN_PERCENT` and
`ITIDY_DARKEN_ALT_PERCENT`. This allows text to adapt to gradient backgrounds,
textured artwork, or any existing pixel data in the safe area. The explicit
color settings are only used in **non-adaptive mode** (fixed color rendering).

**Example — customising the safe area for a differently-shaped template:**

If your template icon is 48×48 with a 3-pixel border and a 5-pixel
decorative header, you might set:

```
ITIDY_TEXT_AREA=3,8,42,37
```

This tells the renderer to start at pixel (3,8) and use a 42×37 area,
preserving the top 8 rows for the header artwork.

**Example — preserving a folded page corner:**

If your 42×46 template has a decorative "turned over page" corner in the
bottom-left (e.g., a 10×10 pixel folded corner starting at pixel position
4,36), you would add:

```
ITIDY_TEXT_AREA=4,4,34,38
ITIDY_EXCLUDE_AREA=4,36,10,10
```

The text renderer will fill the safe area but skip any pixels that fall
within the 10×10 exclusion zone, creating a "hole" where the folded corner
artwork shows through. Text rendering continues around the excluded region
— it doesn't stop, it just skips those pixels.

### 20.2 ToolTypes Stamped onto Generated Icons

These ToolTypes are **added** to every icon that iTidy generates with a
content preview. They are written by `itidy_stamp_created_tooltypes()`.
Existing user ToolTypes (like `FILETYPE` from DefIcons) are preserved —
the `ITIDY_*` entries are appended.

| ToolType | Example Value | Purpose |
|----------|---------------|---------|
| `ITIDY_CREATED` | `1` | Marks this icon as iTidy-generated. |
| `ITIDY_KIND` | `text_preview` | Identifies the type of content preview. Future values: `iff_thumbnail`, `font_preview`. |
| `ITIDY_SRC` | `Instructions` | Filename (without path) of the source file that was previewed. |
| `ITIDY_SRC_SIZE` | `12345` | Source file size in bytes at the time the icon was generated. |
| `ITIDY_SRC_DATE` | `5765,42300,0` | Source file AmigaOS datestamp (`ds_Days,ds_Minute,ds_Tick`) at generation time. |

**Cache-skip logic:** Before re-rendering, iTidy checks `ITIDY_CREATED`,
`ITIDY_SRC_SIZE`, and `ITIDY_SRC_DATE` on an existing target icon. If all
match the current source file, the icon is already up-to-date and both the
template copy and rendering are skipped entirely. This prevents redundant
work during recursive directory processing.

### 20.3 How to Create a Custom Image Template

To create your own text preview template icon:

1. **Create a 42×46 (or any size) palette-mapped OS3.5 icon** using an icon
   editor (e.g., IconEdit on Workbench 3.2).
2. **Design the border/frame artwork** around the edges. The 4-pixel margin
   is the default safe area, so leave pixels 0–3 and (width-4)–(width-1)
   for your border design.
3. **Fill the interior** (the safe area) with a solid background colour —
   this is the "paper" area that text will be rendered into.
4. **Add ToolTypes** to fine-tune the rendering area if your border is
   wider/narrower than the default 4px margin:
   ```
   ITIDY_TEXT_AREA=6,6,30,34
   ITIDY_BG_COLOR=2
   ITIDY_TEXT_COLOR=9
   ```
5. **Save as `PROGDIR:Icons/text_template.info`** (or replace the existing
   file at `Bin/Amiga/Icons/text_template.info` on the host).
6. **Ensure the icon has a selected image** if you want Workbench selection
   highlighting to use safe-area index swap instead of auto-dim.

The icon's palette is preserved exactly as-is. The renderer only paints
using the two palette indices (background and text) — all other palette
entries in the border artwork are never touched.

### 20.4 Palette Design for Adaptive Rendering

When using `ITIDY_ADAPTIVE_TEXT=YES`, the quality of the darkening effect
is directly determined by the richness of the template icon's palette.
The more palette entries available, the smoother the text adaptation to
gradient backgrounds.

#### How Color Selection Works

The `build_darken_table()` function (in `src/icon_edit/icon_text_render.c`)
pre-computes a lookup table that maps each palette index to its darkened
equivalent:

1. **For each palette entry** (0 to palette_size−1):
   - Calculate darkened RGB: `new_r = r × (100 - darken_percent) / 100`
   - **Search the entire palette** for the closest match using Euclidean
     distance in RGB color space:
     ```
     distance = sqrt((r1−r2)² + (g1−g2)² + (b1−b2)²)
     ```
   - Store the best match: `darken_table[source_index] = closest_index`

2. **At render time** (millions of pixel operations), each darkening is an
   instant array lookup — no distance calculations during rendering.

**Key insight:** The algorithm automatically finds the best available match
from whatever palette you provide. More colors = smoother results.

#### Visual Quality by Palette Size

| Palette Size | Gradient Smoothness | Example Use Case |
|--------------|---------------------|------------------|
| **2–4 colors** | Binary/harsh | Simple black-on-white documents, no gradients |
| **8–10 colors** | Basic shading | Current default — good for solid backgrounds |
| **16–32 colors** | Smooth gradients | **Recommended for adaptive mode** — professional quality |
| **48–64 colors** | Film-quality | Premium templates with complex artwork |
| **128–256 colors** | Overkill | Diminishing returns — file bloat for minimal gain |

#### Recommended Palette Structure for Text Icons

**For a 48-color palette optimized for adaptive text rendering:**

| Range | Colors | Purpose |
|-------|--------|---------|
| **0–31** (32 entries) | Smooth grey ramp from pure white to pure black | Text rendering — provides 32 darkening steps |
| **32–39** (8 entries) | Border/artwork colors (beige, tan, brown, shadow) | Decorative frame, folded corners |
| **40–47** (8 entries) | Accent colors (red, blue, green, yellow) | Optional: future badge overlays, file type indicators |

**Example 32-entry grey ramp formula:**

```c
for (i = 0; i < 32; i++)
{
    UBYTE grey_value = 255 - (i * 8);  // 255, 247, 239, ..., 16, 8, 0
    palette[i].red   = grey_value;
    palette[i].green = grey_value;
    palette[i].blue  = grey_value;
}
```

This provides perfectly even steps across the entire brightness range.
When darkening by 70%, the system can always find a grey tone within ~3%
of the mathematically correct value.

#### Why Grey Ramps Work Best

With a gradient background (e.g., white → light grey → medium grey), the
adaptive renderer samples each background pixel and darkens it. If the
palette contains a smooth grey ramp, the algorithm finds **near-perfect
matches** at every brightness level:

- White background (255) → darkened to ~77 → finds grey tone ~76
- Light grey (200) → darkened to ~60 → finds grey tone ~60
- Medium grey (128) → darkened to ~38 → finds grey tone ~40
- Dark grey (40) → darkened to ~12 → finds grey tone ~8

The text appears to "follow" the background gradient naturally, creating
the illusion that the text is printed **onto** the gradient rather than
simply overlaid.

#### Comparison to Halfbright Mode

Classic Amiga EHB (Extra-HalfBright) screen mode provided 64 colors: 32
base colors plus 32 hardware-generated half-brightness variants. This was
brilliant for games needing instant shadows with no palette management.

Icons don't benefit from EHB because:
- Icons have **independent palettes** (not tied to screen modes)
- Workbench typically runs in non-EHB modes (OCS/AGA standard 8-color,
  or RTG TrueColor)
- Icon.library works with **stored RGB palettes**, not bitplane tricks

However, you can **manually replicate the EHB aesthetic** in an icon palette
by designing 32 base colors (entries 0–31) and 32 half-brightness variants
(entries 32–63), where `palette[32+i]` = `palette[i] / 2` for all i.

This gives the adaptive renderer the same smooth darkening capability that
EHB provided for games, but works on **any** Workbench setup (OCS, EGA,
AGA, RTG, CyberGraphX, Picasso96).

#### Performance Considerations

| Palette Size | Table Build Time (68000 @ 7.14 MHz) | Table Build Time (68030 @ 25 MHz) | File Size Increase |
|--------------|--------------------------------------|-------------------------------------|-------------------|
| 10 colors | ~15 ms | <5 ms | +0 KB (baseline) |
| 32 colors | ~48 ms | ~12 ms | +0.2 KB |
| 64 colors | ~96 ms | ~25 ms | +0.4 KB |
| 256 colors | ~384 ms | ~95 ms | +1.5 KB |

**Rendering performance is identical** regardless of palette size — the
darken table lookup is always `O(1)`. Only the one-time table building
stage scales with palette size.

For modern Amigas with RTG cards and 68030+ CPUs, even 64-color palettes
build in under 30ms — imperceptible to users.

#### Practical Example: Upgrading the Default Template

The current `text_template.info` uses a **10-color palette** (indices 0–9).
To upgrade it for smoother adaptive rendering:

1. **Expand palette to 32 entries** in your icon editor
2. **Fill entries 0–31** with a smooth grey ramp (formula above)
3. **Optional:** Add decorative border colors in entries 32–39 if needed
4. **Update ToolTypes:**
   ```
   ITIDY_ADAPTIVE_TEXT=YES
   ITIDY_DARKEN_PERCENT=30
   ITIDY_DARKEN_ALT_PERCENT=15
   ITIDY_EXPAND_PALETTE=YES
   ```
   (Note: `ITIDY_BG_COLOR`, `ITIDY_TEXT_COLOR`, and `ITIDY_MID_COLOR` are
   not needed in adaptive mode — they are ignored when adaptive rendering
   is active.)
5. **Test with gradient backgrounds** — the text should adapt smoothly
   across the entire gradient without visible banding

**Working Example with Standard 42x46 Template:**
```
ITIDY_TEXT_AREA=10,7,24,33
ITIDY_EXCLUDE_AREA=26,33,8,8
ITIDY_ADAPTIVE_TEXT=YES
ITIDY_DARKEN_PERCENT=40
ITIDY_DARKEN_ALT_PERCENT=15
```
This configuration produces excellent results with the default text template,
creating readable text with a subtle ruled paper effect for improved
legibility. The lower `ITIDY_DARKEN_ALT_PERCENT` creates lighter alternating
lines without excessive contrast.

The file size increase is negligible (~200 bytes for 22 extra palette
entries), and the visual improvement is dramatic on icons with gradient
backgrounds or textured artwork.

### 20.5 Automatic Palette Expansion (As of 2026-02-10)

**Problem:** Many template icons use small palettes (8–10 colors) which
provide insufficient grey tones for smooth adaptive text darkening on
gradient backgrounds. Manual palette expansion requires icon editing skills
and is tedious for users.

**Solution:** iTidy now **automatically expands small palettes** at render
time when adaptive text mode is enabled. This happens transparently —
no icon editing required.

#### How It Works

1. **Trigger conditions** (all must be true):
   - `ITIDY_ADAPTIVE_TEXT=YES` is enabled
   - Template palette has **fewer than 16 colors**
   - `ITIDY_EXPAND_PALETTE` is not explicitly set to `NO`

2. **Expansion process** (in `expand_palette_for_adaptive()`):
   - Original N colors are preserved at indices 0 to N-1 (unchanged)
   - New grey ramp is added at indices N to 31 (total = 32 colors)
   - Grey ramp spans from **brightest** to **darkest** original color
   - Uses integer-only math for 68000 compatibility:
     ```c
     grey = lightest - (i * (lightest - darkest)) / (num_new - 1)
     ```

3. **Result:**
   - Original border/frame artwork uses indices 0–9 (unchanged)
   - Text rendering uses expanded grey ramp at indices 10–31
   - Zero pixel remapping needed — existing pixels stay valid
   - Adaptive darkening has 22 additional shades to choose from

#### Performance & File Size

| Metric | Value |
|--------|-------|
| Build time (68000 @ 7MHz) | ~34 ms per icon |
| Build time (68030 @ 25MHz) | ~9 ms per icon |
| File size increase | +200 bytes per icon |
| Visual improvement | Dramatic on gradients |

**Example log output:**
```
expand_palette_for_adaptive: expanded 10 colors -> 32 (added 22 grey ramp entries from lum 255 to 0)
```

#### Disabling Automatic Expansion

To preserve the original palette exactly as-is, add to the template icon:

```
ITIDY_EXPAND_PALETTE=NO
```

This is useful for:
- Templates already using 32+ colors
- Artistic palettes where grey tones would clash
- Testing/comparison purposes

#### Visual Comparison

**Before expansion (10 colors):**
- Adaptive text shows visible banding on gradients
- Limited darkening options cause "stair-stepping"
- Text appears harsh against smooth backgrounds

**After expansion (32 colors):**
- Smooth text adaptation across entire gradient
- Near-photographic quality darkening transitions
- Text appears naturally "printed onto" the background

The expanded palette provides the same quality improvement described in
Section 20.4 ("Recommended Palette Structure"), but achieved automatically
without manual icon editing.

### 20.6 Adaptive Rendering with Alternating Line Intensity (As of 2026-02-10)

**Enhancement:** The adaptive text renderer now supports **dual-level
darkening** for alternating lines, creating a "ruled paper" visual effect
that improves readability while maximizing content density.

#### The Problem

With vertical scaling disabled (v_scale=1) to show maximum line detail,
text lines appear tightly packed with no visual separation. This makes
it difficult to track individual lines, especially in dense ASCII art or
formatted tables.

#### The Solution: Dual Darken Tables

Instead of using a single darken percentage for all lines, the renderer
now builds **two darken tables** with different intensities:

1. **Even lines (darker):** Use full darken percentage (e.g., 30%)
2. **Odd lines (lighter):** Use **half** the darken percentage (e.g., 15%)

This creates alternating bands of darker/lighter text that guide the eye
along each line, similar to ruled notebook paper.

#### Implementation Details

**In `itidy_render_ascii_preview()`:**

```c
UBYTE darken_table_light[256];  // Second table for alternating lines

// Build primary darken table (30% darkening)
build_darken_table(palette, palette_size, 30, darken_table);

// Build lighter darken table (15% darkening = 30% / 2)
build_darken_table(palette, palette_size, 15, darken_table_light);
```

**At render time (per line):**

```c
BOOL is_alternate_line = (out_y % 2) == 1;  // Odd lines are lighter
const UBYTE *active_table = is_alternate_line ? darken_table_light : darken_table;

paint_pixels(..., active_table);  // Use line-specific darken table
```

**Log output shows dual percentages:**
```
itidy_render_ascii_preview: adaptive text enabled, darken=30% (alt=15%)
```

#### Visual Effect

**Before (single darken level):**
- All lines rendered at same intensity
- Dense text appears as solid block
- Difficult to follow individual lines

**After (dual darken levels):**
- Even lines: 30% darkening (darker, more prominent)
- Odd lines: 15% darkening (lighter, more subtle)
- Creates horizontal "ruled paper" bands
- Eye naturally tracks along lighter/darker transitions
- Maintains full content visibility (no lines skipped)

#### When This Activates

This feature is **always active** when:
- `ITIDY_ADAPTIVE_TEXT=YES` is enabled
- Vertical scaling is set (v_scale determines line mapping)

The alternating intensity follows the **output pixel row** number, so it
adapts automatically to whatever line spacing is configured via
`ITIDY_LINE_HEIGHT` and `ITIDY_LINE_GAP`.

#### Performance Impact

Minimal — the dual darken tables are built once at the start of rendering
(~48ms total for both tables on 68000). All rendering operations remain
`O(1)` array lookups regardless of table count.

#### Customization

The alternating line intensity is now **user-configurable** via the
`ITIDY_DARKEN_ALT_PERCENT` tooltype (default is 35%, which is half of the
default 70% darken). To adjust the ruled paper effect:

- **Set both to same value** → disable ruled effect entirely
  - `ITIDY_DARKEN_PERCENT=70` + `ITIDY_DARKEN_ALT_PERCENT=70` → all lines same darkness
- **Large difference** → strong ruled effect
  - `ITIDY_DARKEN_PERCENT=70` + `ITIDY_DARKEN_ALT_PERCENT=20` → very visible alternating lines
- **Small difference** → subtle ruled effect
  - `ITIDY_DARKEN_PERCENT=70` + `ITIDY_DARKEN_ALT_PERCENT=60` → gentle variation
- **Recommended for readability:**
  - `ITIDY_DARKEN_PERCENT=40` + `ITIDY_DARKEN_ALT_PERCENT=15` → excellent balance

Recommended range: **DARKEN=30–40%** with **ALT=10–20%** provides good
balance between text legibility and ruled paper visibility.

---

*This document was last updated on 2026-02-18 after implementing the
picture.datatype thumbnail pipeline (Phase 4: PNG, GIF, JPEG, BMP support),
Ultra mode pre-quantization RGB24 transfer, 5-bit histogram palette sampling,
format-aware transparency handling, and the thumbnail border mode chooser
(Never/Auto/Always). It will continue to be updated as new renderers are
added and issues are discovered.*

---

## 21. Phase 4 — Picture Datatype Pipeline (PNG, GIF, JPEG, BMP)

### 21.1 Overview and Evolution

Sections 14 and 15 of this document described `iTidy_IFFRenderParams` and
`itidy_render_iff_thumbnail()` as **future** work — the planned next renderer
after the ASCII text pipeline. Phase 4 implements that future work, and
extends it well beyond IFF files to cover any format that has a
`picture.datatype` subclass installed on the Amiga.

The new renderer lives in two files:

| File | Purpose |
|------|---------|
| `src/icon_edit/icon_iff_render.c` | Opens files via `picture.datatype`, reads RGB24/RGBA, scales and quantizes |
| `src/icon_edit/icon_content_preview.c` | Orchestrates the full pipeline; dispatches to text or picture renderer |

The entry point called by the DefIcons integration remains the same
`apply_picture_preview()` function in `icon_content_preview.c`. The function
now handles both text files (via the ASCII text renderer from Phase 3) and
picture files (via the new datatype renderer).

### 21.2 Supported Formats

Any format for which the Amiga has a `picture.datatype` subclass is
supported. The following are the primary formats targeted in iTidy's
preference flags:

| Format | Flag | Notes |
|--------|------|-------|
| PNG | `ITIDY_PICTFMT_PNG` | Full alpha channel support via `PDTA_AlphaChannel` |
| GIF | `ITIDY_PICTFMT_GIF` | Transparency via `PDTA_AlphaChannel` only (see Section 21.4) |
| JPEG | `ITIDY_PICTFMT_JPEG` | Always opaque — no alpha, no magenta key |
| BMP | `ITIDY_PICTFMT_BMP` | Always opaque — no alpha, no magenta key |
| IFF ILBM / ACBM | (always enabled) | Legacy Amiga formats; magenta key supported |

Because the renderer uses the standard `NewDTObject()` API, **any other
format with a picture.datatype subclass** (e.g. PCX, TIFF, JPEG 2000 if
a third-party datatype is installed) will also work via the same code path.

### 21.3 How the Pipeline Works

```
 1. Detect source file extension -> determine type token (e.g. "png", "gif")
 2. Check format flag in preferences (ITIDY_PICTFMT_* enable bitmask)
 3. Open file via picture.datatype:
       obj = NewDTObject(path, DTA_GroupID, GID_PICTURE, ...)
 4. Query alpha: GetDTAttrs(obj, PDTA_AlphaChannel, &has_alpha, ...)
 5. Query display mode: GetDTAttrs(obj, PDTA_ModeID, &mode_id, ...)
 6. Render to bitmap:
       if (has_alpha) -> RGBA path (4 bytes per pixel)
       else           -> RGB24 path (3 bytes per pixel)
    using DoMethod(obj, DTM_PROCLAYOUT, NULL, 1) + direct bitmap read
 7. Scale to icon dimensions:
       rgb24_area_average_scale(src, src_w, src_h, dst, dst_w, dst_h)
 8. Quantize to 6x6x6 colour cube (216 entries):
       quantize_rgb24_to_cube(rgb24, pixels, indexed_out)
       -- OR (if Ultra mode active) -- transfer raw RGB24 to caller (see 21.5)
 9. DisposeDTObject(obj)
10. Continue with normal apply pipeline:
       itidy_icon_image_apply() -> itidy_icon_image_save()
```

The 6x6x6 cube palette uses fixed RGB values: each channel can be
{0, 51, 102, 153, 204, 255}. Every pixel is snapped to the nearest
cube entry. The cube is compact (216 entries) and always contains a
full range of colours, making it a reliable target for arbitrary images.

**Note on palette index 185:** The 6x6x6 cube systematically places
`#FF00FF` (magenta) at index 185. This is relevant to the transparency
handling described in Section 21.4.

### 21.4 Transparency Handling Per Format

This was the most nuanced design area in Phase 4 and evolved through
several iterations. The core issue: the 6x6x6 cube always contains
`#FF00FF` at index 185. If the foreground image never uses that colour,
a naive magenta sweep would find it and set it as the transparent index —
incorrectly making parts of the image transparent.

Two separate questions must be answered independently for each format:

1. **CAN this format produce a transparent result?**
   (If no, force borders on — the icon background will always show through)
2. **Should the magenta key sweep run for this format?**
   (If no, only the `PDTA_AlphaChannel` RGBA path is used)

These are answered by two helper functions in `icon_content_preview.c`:

#### `itidy_format_can_be_transparent(type_token)`

Returns TRUE if the format is capable of producing a transparent icon
(i.e., borders are NOT forced on by default):

| Format | Returns | Reason |
|--------|---------|--------|
| `jpeg`, `jpg` | FALSE | JPEG has no alpha channel in the standard |
| `bmp` | FALSE | BMP in its common uncompressed form has no alpha |
| All other formats | TRUE | GIF, PNG, ILBM, ACBM, unknown types may be transparent |

When FALSE is returned, the pipeline forces `ITIDY_BORDER=YES` —
the icon will always paint over a background rather than appearing floating.

#### `itidy_format_uses_magenta_key(type_token)`

Returns TRUE if the magenta sweep should be run after quantization:

| Format | Returns | Reason |
|--------|---------|--------|
| `jpeg`, `jpg` | FALSE | No transparency possible; sweep would cause false positives |
| `bmp` | FALSE | No transparency possible |
| `gif` | **FALSE** | GIF transparency is **exclusively** via `PDTA_AlphaChannel`. If the datatype reports no alpha channel, it has *definitively* answered "no transparency". Running the magenta sweep on the 6x6x6 cube result would find palette index 185 (#FF00FF) and incorrectly mark it transparent. |
| All other formats | TRUE | PNG, ILBM, ACBM: transparency may be present via both `PDTA_AlphaChannel` AND magenta convention |

**Why GIF is special:** GIF's transparency is stored as a specific palette
index inside the GIF file. The picture.datatype reads this and reports it
via `PDTA_AlphaChannel=TRUE` when transparency is present. If it reports
`PDTA_AlphaChannel=FALSE`, that means the GIF has no transparency — full
stop. After quantization to the 6x6x6 cube, palette index 185 is present
for algebraic reasons (it fills a slot), not because any pixel is magenta.
Running the sweep would swap black to transparent — a false positive.

**Bug discovered:** `fire.gif` — a GIF with no transparency — was getting
transparency incorrectly applied. The log showed:
`no alpha channel -- using RGB mode` but the magenta sweep still ran and
found index 185 in the cube. The fix (splitting the single
`itidy_format_supports_transparency()` into the two functions above)
resolved this completely.

#### Format Comparison Summary

| Format | Can be transparent | Uses magenta key | How transparency detected |
|--------|--------------------|-----------------|--------------------------|
| JPEG / JPG | No | No | N/A — always opaque |
| BMP | No | No | N/A — always opaque |
| GIF | Yes | **No** | `PDTA_AlphaChannel` only |
| PNG | Yes | Yes | `PDTA_AlphaChannel` OR magenta sweep |
| IFF ILBM | Yes | Yes | `PDTA_AlphaChannel` OR magenta sweep |
| IFF ACBM | Yes | Yes | `PDTA_AlphaChannel` OR magenta sweep |
| Unknown | Yes | Yes | `PDTA_AlphaChannel` OR magenta sweep |

### 21.5 Ultra Mode — Pre-Quantization RGB24 Transfer

Ultra mode (`deficons_ultra_mode` preference) computes a custom per-image
palette using median-cut analysis rather than the fixed 6x6x6 cube. It
requires raw RGB24 input.

**Problem (first discovered):** The original code path quantized to the
6x6x6 cube first, then Ultra mode expanded the indexed pixels back to RGB24
by table lookup. Because the 6x6x6 cube channels snap to multiples of 51,
the expanded RGB24 contained only ~30 distinct colours — all muted
multiples-of-51. The histogram and median-cut would compute a palette from
those 30 values, not from the original continuous-tone image data.

**Solution — `raw_rgb24_out` mechanism:**

A new field was added to `iTidy_IFFRenderParams`:

```c
UBYTE **raw_rgb24_out;        /* if non-NULL: transfer thumb_rgb24 before free */
ULONG  raw_rgb24_pixel_count; /* pixel count of transferred buffer */
```

At the point in `itidy_render_via_datatype()` where `thumb_rgb24` would
normally be freed, the function now checks:

```c
if (params->raw_rgb24_out != NULL)
{
    *params->raw_rgb24_out = thumb_rgb24;           /* transfer ownership */
    params->raw_rgb24_pixel_count = thumb_pixel_count;
    thumb_rgb24 = NULL;  /* prevent cleanup free */
}
else
{
    whd_free(thumb_rgb24);
    thumb_rgb24 = NULL;
}
```

The caller (`apply_picture_preview()`) sets `iff_params.raw_rgb24_out =
&raw_rgb24` when Ultra mode is active, receives the pre-quantization buffer,
passes it to `itidy_ultra_generate_palette()`, and frees it afterwards.

Log output confirms the transfer: `transferred raw RGB24 (%lu pixels) to caller`
and then: `using pre-quantization RGB24 for Ultra palette generation`

### 21.6 Ultra Mode — 5-Bit Histogram Sampling

**Problem (second discovered):** Even with pre-quantization RGB24 data,
the histogram was capped at 256 exact-colour entries (one entry per unique
exact RGB triplet). A continuous-tone JPEG thumbnail at 64x64 pixels (4096
samples) may have thousands of unique exact triplets. The first 256 colours
seen filled the table; on a typical photograph these come from the top
portion of the image (e.g., dark background, table surface), leaving the
main subject's colours (orange fur, gold tones, bright highlights)
completely unrepresented.

**Solution — 5-bit bucket keys:**

`itidy_ultra_generate_palette()` was rewritten to use per-channel 5-bit
approximation as the histogram key:

```c
UBYTE r_key = r & 0xF8;   /* top 5 bits -> 32 possible values per channel */
UBYTE g_key = g & 0xF8;
UBYTE b_key = b & 0xF8;
```

This collapses 32x32x32 = 32768 possible bucket identifiers to at most 256
entries, but the key insight is that with 5-bit resolution, typical 64x64
photo thumbnails produce 80-180 distinct buckets — providing far better
coverage of the image's colour space.

Each bucket accumulates the **exact** R, G, B values using ULONG
accumulators. After scanning all pixels, each bucket's stored colour is
replaced by the per-bucket average:

```c
histogram[j].r = (UBYTE)(r_sum[j] / counts[j]);
histogram[j].g = (UBYTE)(g_sum[j] / counts[j]);
histogram[j].b = (UBYTE)(b_sum[j] / counts[j]);
```

**Result:** The histogram now fairly represents the entire image rather
than being dominated by colours from the first portion scanned. The
palette returned by median-cut reflects the image's actual dominant colours.

Log output: `ultra_downsample: %u unique colors (5-bit buckets) from %lu RGB24 pixels`

### 21.7 No-Upscale Preference

Small images (shorter dimension < icon thumbnail size) can optionally be
left at their natural size and centred within the icon canvas rather than
being stretched to fill it.

This is controlled by the `deficons_upscale_thumbnails` preference flag,
which maps to `allow_upscale` in `iTidy_IFFRenderParams`. When FALSE:

- Images smaller than the thumbnail dimensions are **not scaled up**
- They are centred within the icon canvas
- The surrounding area uses the template background colour

This prevents blocky pixel-doubled thumbnails for very small source images
(e.g., 16x16 icons used as their own thumbnails).

### 21.8 API Extensions for Phase 4

The `iTidy_IFFRenderParams` struct now has the following fields (additions
highlighted as NEW):

```c
typedef struct {
    /* --- inherited from iTidy_RenderParams base --- */
    UBYTE          *pixel_buffer;
    ULONG           pixel_buffer_size;
    iTidy_SafeArea  safe_area;
    UBYTE           bg_color_index;
    UBYTE           text_color_index;

    /* --- IFF / picture render specifics --- */
    BOOL            try_magenta_key;    /* NEW: run magenta sweep? */
    BOOL            allow_upscale;      /* NEW: allow enlargement of small images */
    BOOL            src_has_alpha;      /* NEW: output — did DT report alpha? */
    UBYTE         **raw_rgb24_out;      /* NEW: if non-NULL, transfer thumb_rgb24 here */
    ULONG           raw_rgb24_pixel_count; /* NEW: pixel count of transferred buffer */
} iTidy_IFFRenderParams;
```

### 21.9 Open Questions Added by Phase 4

| # | Question | Decision | Date |
|---|----------|----------|------|
| 25 | How to handle GIF transparency — magenta key or datatype alpha? | **Datatype alpha (`PDTA_AlphaChannel`) only for GIF.** Magenta sweep disabled. 6x6x6 cube always places #FF00FF at index 185 regardless of image content; sweep on GIF with no alpha causes false positives. | 2026-02-18 |
| 26 | Ultra mode: expand indexed back to RGB24, or transfer pre-quantization buffer? | **Transfer pre-quantization buffer.** `raw_rgb24_out` mechanism in `iTidy_IFFRenderParams`. Post-quantization expansion yields only ~30 muted colours; pre-quantization buffer gives true continuous-tone RGB24. | 2026-02-18 |
| 27 | Ultra histogram: exact-colour matching or approximate-colour buckets? | **5-bit bucket keys** (`r & 0xF8`). Exact-colour matching causes first-256-seen bias on continuous-tone photographs. 5-bit buckets with per-bucket averaging give fair full-image coverage. | 2026-02-18 |
| 28 | Should small images be upscaled to fill the thumbnail? | **User-configurable** via `deficons_upscale_thumbnails` preference + GUI checkbox. Default ON. When OFF, images are centred at natural size. | 2026-02-18 |
| 29 | The old boolean "enable borders" was too coarse — opaque images look odd without a frame, but forcing borders on transparent PNGs is also wrong. How to reconcile? | **Three-way chooser: Never / Auto (smart) / Always.** `ITIDY_THUMB_BORDER_NEVER`=0, `ITIDY_THUMB_BORDER_AUTO`=1, `ITIDY_THUMB_BORDER_ALWAYS`=2. Stored as `UWORD deficons_thumbnail_border_mode`. Auto (default) consults `src_has_alpha`: opaque images get a border, transparent ones are frameless. JPEG/BMP always get borders regardless of user setting. See Section 21.10. | 2026-02-18 |

---

## 22. Thumbnail Border Mode — Never / Auto / Always

### 22.1 Why the Boolean Was Not Enough

The original border preference was a single checkbox: "Enable borders on
image thumbnails" (`BOOL deficons_enable_thumbnail_borders`). This was a
reasonable starting point, but produced unsatisfactory results in practice:

- **Checked (borders on):** Transparent PNG thumbnails got a Workbench
  border frame drawn around them. Because the icon has no background fill,
  the frame appears to float in space — or worse, sits against the wrong
  Workbench background colour. The edge-to-edge transparency is the whole
  point of the thumbnail for such images.

- **Unchecked (borders off):** Opaque images like IFF ILBM waterfall
  photographs and JPEG thumbnails were rendered frameless. These images
  fill every pixel of the icon and have no transparency, so they look like
  a raw pixels rectangle with no visual anchoring on the Workbench. A
  border frame is exactly what makes them look like proper icons.

The boolean could never satisfy both cases simultaneously — the right
answer is different depending on whether the image actually has
transparency.

### 22.2 The Three Modes

The preference was replaced with a `UWORD` field and a Chooser gadget
offering three options:

| Value | Constant | Label | Behaviour |
|-------|----------|-------|-----------|
| 0 | `ITIDY_THUMB_BORDER_NEVER` | Never | Always frameless. Every thumbnail is edge-to-edge with no Workbench border frame, regardless of content. |
| 1 | `ITIDY_THUMB_BORDER_AUTO` | Auto (smart) | **Default.** Workbench border drawn only when the image is opaque. Transparent images (those that have a transparent palette index) remain frameless so the transparency is visible. |
| 2 | `ITIDY_THUMB_BORDER_ALWAYS` | Always | Border drawn on every thumbnail unconditionally, even for transparent images. |

**Implementation:** `UWORD deficons_thumbnail_border_mode` in
`LayoutPreferences` (was `BOOL deficons_enable_thumbnail_borders`).
Default: `ITIDY_THUMB_BORDER_AUTO`.

### 22.3 How Auto Mode Decides

Auto mode consults the `src_has_alpha` field that the picture.datatype
renderer fills in `iTidy_IFFRenderParams`. The decision is made at the
`is_frameless` assignment in `apply_picture_preview()`:

```
if format is always opaque (JPEG, BMP):
    is_frameless = FALSE   <- borders forced on regardless of user setting

else (format can potentially be transparent):
    if AUTO:
        is_frameless = src_has_alpha    <- transparent -> frameless; opaque -> framed
    if ALWAYS:
        is_frameless = FALSE            <- always framed
    if NEVER:
        is_frameless = TRUE             <- always frameless
```

`src_has_alpha` is set to TRUE when either:
- `GetDTAttrs(PDTA_AlphaChannel)` reported TRUE (real alpha channel) — the
  RGBA render path was used, and the quantizer found pixels with A < 255
- The magenta key sweep found magenta pixels and set transparent index 0

**Format-specific overrides always take priority over the user setting:**
JPEG and BMP are unconditionally opaque (border forced on even in Never
mode). This mirrors the existing `itidy_format_can_be_transparent()` logic.

### 22.4 IFF ILBM Path

The IFF renderer (`apply_iff_preview`) uses its own internal path that
does not go through `picture.datatype`. It has no `src_has_alpha` field.
IFF ILBM files processed here always have `mask=0` in the BMHD chunk —
they are definitively opaque.

The border logic for this path is therefore simpler:

```
if NEVER  -> frameless
if AUTO   -> framed   (opaque by definition; same result as JPEG/BMP)
if ALWAYS -> framed
```

Only `NEVER` produces a frameless result on the IFF path.

### 22.5 Code Locations

| Symbol | File | Notes |
|--------|------|-------|
| `ITIDY_THUMB_BORDER_NEVER` / `_AUTO` / `_ALWAYS` | `src/layout_preferences.h` | Constants 0, 1, 2 |
| `deficons_thumbnail_border_mode` | `src/layout_preferences.h` | `UWORD` field in `LayoutPreferences` |
| `DEFAULT_DEFICONS_THUMBNAIL_BORDER_MODE` | `src/layout_preferences.h` | = `ITIDY_THUMB_BORDER_AUTO` |
| `init_default_preferences()` | `src/layout_preferences.c` | Sets field to default |
| `is_frameless` assignment (picture path) | `src/icon_edit/icon_content_preview.c` | Three-way switch in `apply_picture_preview()` |
| `is_frameless` assignment (IFF path) | `src/icon_edit/icon_content_preview.c` | Simpler `NEVER`-only check in `apply_iff_preview()` |
| `GID_THUMBNAIL_BORDERS_CHOOSER` | `src/GUI/deficons_creation_window.c` | Replaces old `GID_THUMBNAIL_BORDERS_CHECKBOX` |
| `thumbnail_borders_chooser_obj` | `src/GUI/deficons_creation_window.c` | Replaces old `thumbnail_borders_checkbox` Object pointer |
| `thumbnail_borders_list` | `src/GUI/deficons_creation_window.c` | Chooser node list for the three labels |
