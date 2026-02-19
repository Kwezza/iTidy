# DefIcons Icon Creation Settings Window

**Source:** `src/GUI/deficons_creation_window.c`  
**Header:** `src/GUI/deficons_creation_window.h`  
**Public API:** `open_itidy_deficons_creation_window(LayoutPreferences *prefs)`  

---

## Overview

This window lets you configure how iTidy generates DefIcons-style icons for files and
folders it encounters during a scan.  It is a modal ReAction window (Workbench 3.2+)
that operates on a live `LayoutPreferences` structure.  Changes are written back to the
preferences only when the user clicks **OK**; clicking **Cancel** or the close gadget
discards all changes.

The window is divided into three labeled groups:

1. **Icon Creation Options** - folder icon policy, physical icon size, thumbnail frame,
   text/picture preview toggles, and upscaling.
2. **Enabled Picture Formats** - per-format checkboxes that control which image types
   get thumbnail rendering.
3. **Color Reduction** - palette depth, dithering algorithm, and low-color palette
   mapping.

---

## Group 1: Icon Creation Options

### Folder Icons (chooser)

| Label | Value | Description |
|-------|-------|-------------|
| Smart (create folder if it has icons inside) | 0 | Scan the containing folder for icons - creating icons in the folder if needed. This is the default. |
| Always create | 1 | Always generate a folder icon, for folders with no icons. |
| Never create | 2 | Skip folder icon creation entirely. |

**Preference field:** `deficons_folder_icon_mode` (`UWORD`)  
**Default:** `0` (Smart)

---

### Icon Size (chooser)

Sets the pixel dimensions used when generating thumbnail images inside icons.

| Label | Value | Pixel size |
|-------|-------|-----------|
| Small (48x48) | 0 | 48 x 48 |
| Medium (64x64) | 1 | 64 x 64 (default) |
| Large (100x100) | 2 | 100 x 100 |

**Preference field:** `deficons_icon_size_mode` (`UWORD`)  
**Default:** `1` (Medium)

---

### Thumbnail Borders (chooser)

Controls whether a Workbench-style border frame is drawn around a thumbnail image.

| Label | Value | Constant | Behaviour |
|-------|-------|----------|-----------|
| Never | 0 | `ITIDY_THUMB_BORDER_NEVER` | Edge-to-edge thumbnail, no frame. |
| Auto (smart) | 1 | `ITIDY_THUMB_BORDER_AUTO` | Frame is drawn only when the source image is fully opaque (has no transparency/alpha channel). Transparent images are rendered frameless so the icon shape shows through. This is the default. |
| Always | 2 | `ITIDY_THUMB_BORDER_ALWAYS` | Always draw the Workbench border frame, regardless of transparency. |

**Preference field:** `deficons_thumbnail_border_mode` (`UWORD`)  
**Default:** `ITIDY_THUMB_BORDER_AUTO` (1 - Auto)

---

### Enable Text File Preview Thumbnails (checkbox)

When ticked, iTidy renders the first few lines of text files (README, source code, etc.)
directly onto the icon image as a small preview.

- **Ticked** -> preview enabled.
- **Unticked** -> plain type icon used for text files.

**Preference field:** `deficons_enable_text_previews` (`BOOL`)

#### Manage Text Templates... (button)

Clicking this button opens the **Text Templates** window
(`open_text_templates_window()`).  That window lets you customise the font, colours, and
layout rules used when rendering text-file thumbnails.  The button is always available
regardless of whether text preview is currently enabled.

---

### Enable Picture File Preview Thumbnails (checkbox)

When ticked, iTidy renders a scaled-down version of a recognised image file directly
onto the icon it creates.

- **Ticked** -> thumbnail rendering enabled for the formats selected in the
  **Enabled Picture Formats** group below.
- **Unticked** -> plain type icon used for picture files.

**Preference field:** `deficons_enable_picture_previews` (`BOOL`)

---

### Upscale Small Images to Icon Size (checkbox)

When ticked, images that are smaller than the selected icon size are scaled *up* to fill
the icon canvas.  When unticked (the default) small images are displayed at their
natural size, centred within the icon.

**Preference field:** `deficons_upscale_thumbnails` (`BOOL`)  
**Default:** `FALSE` (unticked - keep natural size)

---

## Group 2: Enabled Picture Formats

A two-column grid of checkboxes.  Each checkbox enables or disables thumbnail generation
for one image file format.  The combined state is stored as a bitmask.

| Checkbox | Bit constant | Bitmask value | Note |
|----------|-------------|---------------|------|
| ILBM | `ITIDY_PICTFMT_ILBM` | `0x0001` | Amiga IFF ILBM/PBM images |
| PNG | `ITIDY_PICTFMT_PNG` | `0x0002` | Portable Network Graphics |
| GIF | `ITIDY_PICTFMT_GIF` | `0x0004` | Graphics Interchange Format |
| JPEG (slow) | `ITIDY_PICTFMT_JPEG` | `0x0008` | JPEG — labelled "slow" because JPEG decoding is slower on 68k hardware |
| BMP | `ITIDY_PICTFMT_BMP` | `0x0010` | Windows BMP |
| ACBM | `ITIDY_PICTFMT_ACBM` | `0x0020` | Amiga Contiguous Bitmap format |
| Other | `ITIDY_PICTFMT_OTHER` | `0x0040` | Any recognised image format not covered by the six formats above |

**Preference field:** `deficons_picture_formats_enabled` (`ULONG`, bitmask of `ITIDY_PICTFMT_*`)  
**Default:** all formats enabled (`ITIDY_PICTFMT_ALL` minus JPEG)

---

## Group 3: Color Reduction

### Max Colors (chooser)

Sets the maximum number of palette entries used in the generated icon.  Lower values
produce smaller, lower-fidelity icons; higher values produce more accurate thumbnails.

| Label | Index | Colors | Notes |
|-------|-------|--------|-------|
| 4 colors | 0 | 4 | Minimum; very coarse |
| 8 colors | 1 | 8 | |
| 16 colors | 2 | 16 | |
| 32 colors | 3 | 32 | |
| 64 colors | 4 | 64 | |
| 128 colors | 5 | 128 | |
| 256 colors (full) | 6 | 256 | Full AmigaOS 8-bit palette |
| Ultra (256 + detail boost) | 7 | 256 | 256-color detail-preserving downsample mode. Sets `deficons_ultra_mode = TRUE`. Dithering and low-color mapping choosers are greyed out when this is selected. |

When **256 colors (full)** or **Ultra** is selected, dithering is not needed (no
reduction is performed), so the **Dithering** chooser is automatically disabled.

When more than **8 colors** is selected, the **Low-Color Map** chooser is automatically
disabled (it is only meaningful at 4 or 8 colors).

**Preference fields:**
- `deficons_max_icon_colors` (`UWORD`) - the actual color count (4, 8, …, 256)
- `deficons_ultra_mode` (`BOOL`) - `TRUE` when Ultra is selected

---

### Dithering (chooser)

Controls the algorithm used to reduce color banding when the palette is smaller than the
source image requires.

| Label | Value | Description |
|-------|-------|-------------|
| None | 0 | No dithering. Fast, but can produce visible color banding. |
| Ordered (Bayer 4x4) | 1 | Classic Bayer ordered dithering using a 4x4 matrix. Predictable pattern, fast, no error accumulation. |
| Error diffusion (Floyd-Steinberg) | 2 | Diffuses quantisation error to neighbouring pixels. Produces smoother gradients but is slower. |
| Auto (based on color count) | 3 | iTidy picks the algorithm automatically: Bayer for >= 16 colors, Floyd-Steinberg for fewer. |

**Grayed out** when Max Colors is 256 or Ultra.

**Preference field:** `deficons_dither_method` (`UWORD`)

---

### Low-Color Map (chooser)

When palette depth is 8 colors or fewer this chooser selects the palette template that
the color-reduction step maps to.  At higher color counts the palette is derived from
the image itself, so this setting has no effect and the chooser is greyed out.

| Label | Value | Description |
|-------|-------|-------------|
| Grayscale | 0 | Map all colors to a grayscale ramp. Suitable for icons displayed on non-color Workbench screens. |
| Workbench palette | 1 | Snap each color to the nearest entry in the standard Workbench 8-color palette. Gives icons a consistent look alongside system icons. |
| Hybrid (grays + WB accents) | 2 | Use a grayscale ramp as the base but substitute Workbench accent colors (red, green, blue, yellow) where they produce a significantly better match. |

**Grayed out** when Max Colors is > 8 or Ultra.

**Preference field:** `deficons_lowcolor_mapping` (`UWORD`)

---

## Buttons

| Button | Action |
|--------|--------|
| **OK** | Saves all current gadget values back into the `LayoutPreferences` structure and closes the window. The calling code receives `TRUE`. |
| **Cancel** | Closes the window without modifying preferences. The calling code receives `FALSE`. |
| Close gadget (window title bar) | Same as Cancel. |

---

## Dynamic Gadget Interactions

The window adjusts gadget states automatically as choices change:

- Choosing **256 colors (full)** or **Ultra** immediately disables (ghosts) both the  
  **Dithering** and **Low-Color Map** choosers.
- Choosing any value with more than **8 colors** disables only the **Low-Color Map**
  chooser.
- Re-selecting a value of 4 or 8 colors re-enables both dependent choosers.

This logic runs in `handle_gadget_up()` whenever `GID_MAX_COLORS_CHOOSER` fires a
`WMHI_GADGETUP` event, using `SetGadgetAttrs()` with `GA_Disabled`.

---

## Preference Fields Summary

| Field | Type | Gadget |
|-------|------|--------|
| `deficons_folder_icon_mode` | `UWORD` | Folder Icons chooser |
| `deficons_icon_size_mode` | `UWORD` | Icon Size chooser |
| `deficons_thumbnail_border_mode` | `UWORD` | Thumbnail Borders chooser |
| `deficons_enable_text_previews` | `BOOL` | Text preview checkbox |
| `deficons_enable_picture_previews` | `BOOL` | Picture preview checkbox |
| `deficons_upscale_thumbnails` | `BOOL` | Upscale small images checkbox |
| `deficons_picture_formats_enabled` | `ULONG` | Per-format checkboxes (bitmask) |
| `deficons_max_icon_colors` | `UWORD` | Max Colors chooser |
| `deficons_ultra_mode` | `BOOL` | Max Colors chooser (Ultra index) |
| `deficons_dither_method` | `UWORD` | Dithering chooser |
| `deficons_lowcolor_mapping` | `UWORD` | Low-Color Map chooser |

All fields are members of `LayoutPreferences` defined in `src/layout_preferences.h`.
