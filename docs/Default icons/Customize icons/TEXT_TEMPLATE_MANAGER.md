# ASCII Text Template Manager

## Overview

The **ASCII Text Template Manager** is a window inside iTidy that lets you manage the icon templates used when rendering text file content as thumbnail previews on Workbench icons.

It is opened from the **DefIcons Creation Settings** window via the **"Manage Text Templates..."** button.

---

## What Are Text Templates?

When iTidy creates a DefIcons-style preview icon for a text-based file type (C source, Rexx script, HTML, etc.), it uses a `.info` icon file as a template. The template carries ToolTypes that control exactly how the text preview is rendered — the text area position, colours, line height, and so on.

Templates live in:

```
PROGDIR:Icons/def_<type>.info
```

For example:
- `def_ascii.info` — the **master template** (all other types are copied from this)
- `def_c.info` — custom settings for C source files
- `def_rexx.info` — custom settings for Rexx scripts

If no custom template exists for a type, iTidy falls back to the master `def_ascii.info`.

---

## The Window

### List

The main list shows every known ASCII sub-type (c, rexx, html, python, etc.) with three columns:

| Column | Description |
|--------|-------------|
| **Type** | The DefIcons sub-type name |
| **File** | The expected template filename (e.g. `def_c.info`) |
| **Status** | `[Custom]` if a template file exists, blank if using the ascii fallback |

Select a row before using any of the action buttons.

---

## Action Buttons

Action buttons appear on the right side of the window. Select a row in the list to enable them.

### Create from master

Creates a copy of `def_ascii.info` as `def_<type>.info` for the selected type.

- If the destination already exists you will be asked to confirm overwrite.
- The confirmation requester shows the full absolute paths (e.g. `PC:Programming/iTidy/Bin/Amiga/Icons/...`) so you know exactly which files are involved.
- After copying, the list refreshes automatically and the status column updates to `[Custom]`.

This is the normal starting point for customising a type: copy the master, then open it in Workbench Information to edit its ToolTypes.

### Validate tooltypes

Reads the ToolTypes from the selected type's template icon and performs two things:

**1. Validation checks**
- Warns about any `ITIDY_*` key that is not in the known list (possible typo)
- Checks colour index keys (`ITIDY_BG_COLOR`, `ITIDY_TEXT_COLOR`, `ITIDY_MID_COLOR`) are in the range 0-255
- Checks percentage keys (`ITIDY_DARKEN_PERCENT`, `ITIDY_DARKEN_ALT_PERCENT`) are 0-100
- Checks area rectangle keys (`ITIDY_TEXT_AREA`, `ITIDY_EXCLUDE_AREA`) are in `x,y,w,h` format with positive width and height
- Checks positive-integer keys (`ITIDY_MAX_LINES`, `ITIDY_LINE_HEIGHT`, `ITIDY_LINE_GAP`, `ITIDY_CHAR_WIDTH`, `ITIDY_READ_BYTES`) are greater than zero

**2. Effect Summary**

After the validation results, a compact summary shows what the current ToolTypes will actually do:

```
--- Effect Summary ---
Text area:    3,3,40,52
Exclude area: (none)
Lines: max 8, height 8, gap 1
Char width: 6, Read: 512 bytes
Colours: bg=0, text=1, mid=(none)
Darken: 40%, alt 20%
Adaptive: YES, Expand palette: NO
Exclude types: (none)
```

Any key not present in the icon shows a default placeholder (`?`, `(none)`, `NO`, `0`) so you can see at a glance what is configured and what is absent.

### Edit tooltypes...

Opens the standard Workbench **Information** dialog for the selected template icon, exactly as if you had right-clicked the `.info` file on Workbench and chosen Information.

This lets you directly edit the ToolTypes from within the familiar Workbench interface without leaving iTidy.

> **Note:** Workbench requires a companion file alongside the `.info` file to display the Information dialog correctly, at least when launched via ARexx. iTidy creates a zero-byte companion file automatically the first time you use this button for a given type — this is harmless and is only needed for the dialog to open.

### Revert to master

Deletes the custom `def_<type>.info` template and reverts the type to use the master `def_ascii.info` instead.

- Only available for types that have a custom template.
- A confirmation dialog shows the full file path before deletion.
- After deletion, the list refreshes automatically and the status column updates (no longer shows `[Custom]`).

Use this if you want to discard customizations for a type and start fresh.

---

## ToolType Reference

### Core Rendering Parameters

The following ToolTypes are recognised in text template icons:

| ToolType | Type | Description |
|----------|------|-------------|
| `ITIDY_TEXT_AREA` | `x,y,w,h` | Rectangle within the icon image where text is drawn |
| `ITIDY_EXCLUDE_AREA` | `x,y,w,h` | Rectangle to leave blank (e.g. a logo area) |
| `ITIDY_MAX_LINES` | integer > 0 | Maximum number of lines to render |
| `ITIDY_LINE_HEIGHT` | integer > 0 | Pixel height per line |
| `ITIDY_LINE_GAP` | integer >= 0 | Extra gap in pixels between lines |
| `ITIDY_CHAR_WIDTH` | integer > 0 | Pixel width per character |
| `ITIDY_READ_BYTES` | integer > 0 | How many bytes to read from the file |

### Colour and Contrast Control

| ToolType | Type | Description | Notes |
|----------|------|-------------|-------|
| `ITIDY_BG_COLOR` | 0-255, or -1 | Palette index for the background colour | Use `-1` to preserve template pixels instead of filling background |
| `ITIDY_TEXT_COLOR` | 0-255 | Palette index for the text colour | If not specified, auto-detected from palette luminance |
| `ITIDY_MID_COLOR` | 0-255 | Palette index for a mid-tone colour | Used for anti-aliasing; defaults to text colour if unset |
| `ITIDY_ADAPTIVE_TEXT` | `YES`/`NO` | Enable automatic text colour selection for contrast | **Enables and requires** `ITIDY_DARKEN_PERCENT` and `ITIDY_EXPAND_PALETTE` |
| `ITIDY_DARKEN_PERCENT` | 0-100 | Darkening for even rows | Only active when `ITIDY_ADAPTIVE_TEXT=YES` |
| `ITIDY_DARKEN_ALT_PERCENT` | 0-100 | Darkening for odd rows (striping effect) | Only active when `ITIDY_ADAPTIVE_TEXT=YES` |
| `ITIDY_EXPAND_PALETTE` | `YES`/`NO` | Allow palette expansion for smooth shading | Only takes effect when `ITIDY_ADAPTIVE_TEXT=YES` and palette < 16 colors |

### Advanced Filtering

| ToolType | Type | Description |
|----------|------|-------------|
| `EXCLUDETYPE` | string | DefIcons type name to exclude from processing (can appear multiple times) |

### Metadata ToolTypes (Read-Only)

These ToolTypes are written by iTidy. Do not edit them manually.

| ToolType | Description |
|----------|-------------|
| `ITIDY_CREATED` | Date/time the template was created |
| `ITIDY_KIND` | Template kind identifier |
| `ITIDY_SRC` | Source template this was copied from |
| `ITIDY_SRC_SIZE` | Size of the source at copy time |
| `ITIDY_SRC_DATE` | Date of the source at copy time |

### ToolType Interaction Rules

**When `ITIDY_ADAPTIVE_TEXT=YES`:**
- The `ITIDY_DARKEN_PERCENT` and `ITIDY_DARKEN_ALT_PERCENT` values are activated to create alternating row darkening
- The `ITIDY_EXPAND_PALETTE` setting is checked:
  - If palette has 128 or more colours, expansion is skipped (palette considered sufficiently rich)
  - If palette has 2-127 colours and `ITIDY_EXPAND_PALETTE` is not explicitly set to `NO`, the palette is expanded with darkened colour variants
    - For each original colour: a 70% darkened version is added, plus a 35% darkened version for row striping
    - This preserves the original hue while adding darker shades, up to a maximum of 256 palette colours (full Amiga icon limit)
    - Duplicate or near-identical colours (within RGB tolerance) are intelligently reused to conserve palette slots rather than wasting them on duplicates

**When `ITIDY_ADAPTIVE_TEXT=NO` (default):**
- `ITIDY_DARKEN_PERCENT` and `ITIDY_DARKEN_ALT_PERCENT` have **no effect**
- `ITIDY_EXPAND_PALETTE` has **no effect**
- Text is rendered in the solid colour specified by (or auto-detected for) `ITIDY_TEXT_COLOR`

**When `ITIDY_BG_COLOR=-1` (special value):**
- The background fill step is skipped entirely
- Template artwork (like folded corners or embossed areas) is preserved
- Text is rendered over the unmodified template pixels
- Only `ITIDY_TEXT_AREA` defines where text is placed

**Colour Auto-Detection (when colours are not manually specified):**
- `ITIDY_BG_COLOR`: Lightest palette colour (closest to white)
- `ITIDY_TEXT_COLOR`: Darkest palette colour (closest to black)
- `ITIDY_MID_COLOR`: Colour closest to mid-luminance grey
- Manual ToolType values override auto-detection

---

## Typical Workflow

1. Open **DefIcons Creation Settings** -> **Manage Text Templates...**
2. Check the list to see which types already have custom templates (`[Custom]`).
3. Select a type that shows no custom template (blank status).
4. Click **Create from master** -> confirm the copy.
5. Click **Edit tooltypes...** to open the Workbench Information dialog.
6. Switch to the **Icon** tab in the Information dialog and edit the ToolTypes to suit the file type (adjust colours, text area, line settings, etc.).
7. Click **Save** in the Information dialog to close it.
8. Back in iTidy, the list updates automatically. Click **Validate tooltypes** to confirm the values are all valid and review the effect summary.
9. Repeat for other types as needed.
10. Click **Close** when finished. All template changes are already saved to disk.
