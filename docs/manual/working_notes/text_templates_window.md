# Text Templates Window — Working Notes

**Source**: `src/GUI/deficons/text_templates_window.c` (2127 lines), `src/GUI/deficons/text_templates_window.h`
**Title bar**: "iTidy - DefIcons Text Templates"
**Opened from**: Main window -> Settings -> DefIcons Categories... -> Preview Icons...; also Icon Creation Settings -> Create tab -> Manage Templates...
**Modal**: No
**Blocking**: Yes (runs own event loop, returns when closed)
**Resizable**: Yes (min 420x300, initial 480x200)
**Keyboard shortcut**: C = Close

---

## Purpose

Manages DefIcons text preview template icons. Each ASCII sub-type (C source, REXX, HTML, etc.) can have its own template icon (`def_<type>.info`) stored in `PROGDIR:Icons/`. These template icons contain ToolTypes that control how iTidy renders text preview thumbnails onto file icons.

---

## Key Concepts

### Master vs Custom Templates

- **Master template**: `def_ascii.info` — the fallback template used for all ASCII sub-types that lack their own. Always appears first in the list with status "Master".
- **Custom template**: `def_<type>.info` (e.g. `def_c.info`, `def_rexx.info`) — a type-specific override, created by copying the master. Status shows "Custom".
- If no custom template exists for a sub-type, it falls back to the master. Status shows "Using master".

### Excluded Types

The master template's `EXCLUDETYPE` ToolType contains a comma-separated list of sub-type names that are excluded from receiving icons entirely. These appear in the list with status "Excluded". Clicking any action button for an excluded type shows an explanation requester directing the user to edit `def_ascii.info` to remove the type from the `EXCLUDETYPE` list.

### Template ToolTypes

Template icons contain `ITIDY_*` prefixed ToolTypes that control text preview rendering.

**Renderer context:** This is a micro-preview / density renderer, not a text layout engine. Each source character is treated as a single pixel-width column and each source line as a single pixel-tall band. The output shows the density and structure of the file's text as ruled-paper-like marks across the icon face. All size ToolTypes operate in output-pixel units, not font or point sizes. A default of 1 for `ITIDY_LINE_HEIGHT` is intentional and correct for small icons.

| ToolType | Default | Purpose |
|----------|---------|------|
| `ITIDY_TEXT_AREA` | 4-pixel margin | Rectangle (x,y,w,h) defining where text is drawn |
| `ITIDY_EXCLUDE_AREA` | (none) | Rectangle (x,y,w,h) area to skip when drawing |
| `ITIDY_LINE_HEIGHT` | 1 | Output height in pixels of each rendered line band. At 1, a 60px safe area holds 60 bands. Increase to 2 or 3 for thicker bands and fewer lines per icon. |
| `ITIDY_LINE_GAP` | 1 | Pixel gap between line bands *(parsed but not currently used by renderer -- see notes below)* |
| `ITIDY_MAX_LINES` | (auto) | Maximum number of rendered line bands to produce *(recognised by validator but not currently enforced by renderer -- see notes below)* |
| `ITIDY_CHAR_WIDTH` | 0 (auto) | Pixel width of each character; 0 = auto-select |
| `ITIDY_READ_BYTES` | 4096 | Number of bytes to read from the file |
| `ITIDY_BG_COLOR` | (none -- no fill) | Background colour index (0-254) to fill the safe area; absent or -1 = no fill (preserve template pixels). Index 255 is reserved internally and cannot be used. |
| `ITIDY_TEXT_COLOUR` | (auto) | Text colour index (0-255); **ignored** when `ITIDY_ADAPTIVE_TEXT=YES` |
| `ITIDY_MID_COLOUR` | (auto) | Mid-tone colour index (0-255); **ignored** when `ITIDY_ADAPTIVE_TEXT=YES` |
| `ITIDY_DARKEN_PERCENT` | 70 | Darkening percentage (1-100); **only active** when `ITIDY_ADAPTIVE_TEXT=YES` |
| `ITIDY_DARKEN_ALT_PERCENT` | 35 | Alternate-line darkening percentage (1-100); **only active** when `ITIDY_ADAPTIVE_TEXT=YES` |
| `ITIDY_ADAPTIVE_TEXT` | NO | Enable adaptive text colouring (YES/NO) |
| `ITIDY_EXPAND_PALETTE` | YES | Pre-adds darkened colour variants derived from the existing palette for smoother tones; only active when `ITIDY_ADAPTIVE_TEXT=YES` |

### ToolType Interactions and Precedence

The ToolTypes fall into two groups that behave quite differently depending on whether adaptive text colouring is on or off.

---

#### Adaptive Text Group (`ITIDY_ADAPTIVE_TEXT`)

`ITIDY_ADAPTIVE_TEXT` is the master switch for the entire adaptive rendering path. Setting it to `YES` changes how glyph pixels are coloured and activates several otherwise-ignored ToolTypes.

**When `ITIDY_ADAPTIVE_TEXT=NO` (default):**
- Each glyph pixel is painted with a fixed palette index drawn from `ITIDY_TEXT_COLOUR` (medium/high density) or `ITIDY_MID_COLOUR` (sparse/edge anti-alias pixels).
- `ITIDY_DARKEN_PERCENT`, `ITIDY_DARKEN_ALT_PERCENT`, and `ITIDY_EXPAND_PALETTE` are all **ignored entirely**.
- If `ITIDY_TEXT_COLOUR` or `ITIDY_MID_COLOUR` are absent, the renderer auto-detects the darkest and mid-luminance entries in the icon palette.

**When `ITIDY_ADAPTIVE_TEXT=YES`:**
- `ITIDY_TEXT_COLOUR` and `ITIDY_MID_COLOUR` are **ignored for glyph painting**. The renderer instead samples the existing background pixel at each position and darkens it using a pre-built lookup table. This means the text "inherits" the colour of whatever the template artwork has underneath it.
- `ITIDY_DARKEN_PERCENT` (default 70%) becomes active: it controls how strongly the background pixel is darkened for medium- and high-density output rows.
- `ITIDY_DARKEN_ALT_PERCENT` (default 35%) becomes active: it controls darkening for alternating/odd output rows, producing a lighter "ruled paper" stripe.
- `ITIDY_EXPAND_PALETTE` becomes active (see below).
- `ITIDY_BG_COLOR` is still respected when set, but when absent (the default), no fill happens -- the template artwork pixels are preserved, which is exactly what you want in adaptive mode. Setting it to a solid palette index defeats the purpose: every glyph pixel will darken the same background to the same result, no different from fixed colour mode. Leave `ITIDY_BG_COLOR` absent when using adaptive mode.

**Interaction summary — adaptive mode:**

| ToolType | Effect when ITIDY_ADAPTIVE_TEXT=NO | Effect when ITIDY_ADAPTIVE_TEXT=YES |
|----------|-------------------------------------|--------------------------------------|
| `ITIDY_TEXT_COLOUR` | Fixed colour for medium/high-density glyph pixels | **Ignored** — pixel darkened from background instead |
| `ITIDY_MID_COLOUR` | Fixed colour for sparse/edge glyph pixels | **Ignored** — pixel darkened from background instead |
| `ITIDY_DARKEN_PERCENT` | **Ignored** | Darkening strength for even output rows (default 70%) |
| `ITIDY_DARKEN_ALT_PERCENT` | **Ignored** | Darkening strength for odd output rows (default 35%) |
| `ITIDY_EXPAND_PALETTE` | **Ignored** | Adds darkened palette variants for smoother tone gradations |
| `ITIDY_BG_COLOR` | No fill (template pixels preserved) | Same default -- no fill. A solid fill defeats adaptive mode; leave absent or set to -1. |

---

#### `ITIDY_EXPAND_PALETTE` — depends on `ITIDY_ADAPTIVE_TEXT`

Palette expansion pre-adds darkened colour variants derived from the existing palette so the darken tables have more entries to map to, producing smoother tone gradations. It works in two phases:

1. Adds a 70%-darkened version of every existing palette colour (used for even-row text pixels).
2. Adds a 35%-darkened version of every existing palette colour (used for odd-row/alternate pixels).

Entries that are already close enough (within ~20 RGB units) to an existing entry are reused rather than duplicated. No neutral grey values are added — the expanded entries are all derived from the template's own colours.

It only triggers when **all** of the following are true:

1. `ITIDY_ADAPTIVE_TEXT=YES`
2. `ITIDY_EXPAND_PALETTE` is not explicitly set to `NO` (defaults to YES)
3. The palette has fewer than 128 colours and at least 2 colours

If `ITIDY_ADAPTIVE_TEXT=NO`, `ITIDY_EXPAND_PALETTE` has no effect regardless of its value.

---

#### `ITIDY_BG_COLOR` — absent means no fill

`ITIDY_BG_COLOR` is resolved with a default of `-1`, so when the ToolType is absent the behaviour is identical to setting it to `-1`: no background fill. The safe area is left as-is and glyph pixels are painted on top of whatever template artwork is already there.

This means adaptive mode works correctly with no `ITIDY_BG_COLOR` entry at all. You only need to set it if you want a solid fill, which you would not normally want with `ITIDY_ADAPTIVE_TEXT=YES` — once the safe area is a uniform colour, every glyph pixel darkens to the same result, no different from fixed colour mode.

Setting `ITIDY_BG_COLOR` to a valid palette index (0-254) forces a solid fill. Index 255 is reserved internally as the "no fill" sentinel and cannot be used as a colour. Any value outside the valid palette range (other than 255, which is always treated as no-fill) triggers auto-detection of the lightest palette entry as the background — also a solid fill.

---

#### Colour Auto-Detection (`ITIDY_TEXT_COLOR`, `ITIDY_MID_COLOR`)

`ITIDY_TEXT_COLOR` and `ITIDY_MID_COLOR` auto-detect when absent or out-of-range. `ITIDY_BG_COLOR` behaves differently (see above — absent = no fill, not auto-detect).

- **Text** (`ITIDY_TEXT_COLOR` absent or out-of-range): darkest palette entry
- **Mid-tone** (`ITIDY_MID_COLOR` absent or out-of-range): palette entry closest to the midpoint between darkest and lightest luminance, excluding the two extremes
- **Background** (`ITIDY_BG_COLOR` out-of-range only): lightest palette entry. Note: absent triggers no fill, not this path. Only an explicitly out-of-range value (e.g. `ITIDY_BG_COLOR=999` with a small palette) reaches auto-detect.

A safety check prevents background and text resolving to the same index.

---

#### `ITIDY_TEXT_AREA` and `ITIDY_EXCLUDE_AREA` — independent zones

These two ToolTypes define different things and are checked separately:

- `ITIDY_TEXT_AREA` sets the **safe rendering zone**: the renderer fills the background and draws all text within this rectangle. If absent, the zone defaults to the icon bounds minus a 4-pixel margin on all sides.
- `ITIDY_EXCLUDE_AREA` defines a **per-pixel skip zone** within the buffer: individual pixels that land inside this rectangle are not written, regardless of the text area. This preserves decorative elements of the template artwork (e.g. a folded corner graphic). It operates in both adaptive and non-adaptive modes.

The two zones are not mutually exclusive — the exclude area can overlap or lie entirely within the text area.

---

#### `ITIDY_CHAR_WIDTH` — auto-select mode

Setting `ITIDY_CHAR_WIDTH=0` (the default) enables auto-selection:

- If the safe area width is 64 pixels or greater: 2 pixels per character
- If the safe area width is less than 64 pixels: 1 pixel per character

Any non-zero value overrides auto-selection.

---

#### `ITIDY_MAX_LINES` and `ITIDY_LINE_GAP` — currently not enforced

These two ToolTypes are **recognised by the Validate ToolTypes function** and will not be reported as unknown keys, but the renderer does not currently read them:

- `ITIDY_MAX_LINES`: the renderer calculates the maximum source lines automatically from the safe area height divided by `ITIDY_LINE_HEIGHT`, scaled by an auto-computed vertical downscale factor. The ToolType value is never applied.
- `ITIDY_LINE_GAP`: the value is parsed into the render parameters struct but the rendering loop does not use it. Band spacing is controlled solely by `ITIDY_LINE_HEIGHT`.

These are reserved for potential future use. Setting them will not cause errors but will have no visible effect.

---

## Window Layout

The window is split into two main columns:

**Left column (65% width)** — "Icons" group:
- Show filter chooser at the top
- Template list below

**Right column (35% width)** — two groups:
- "Selected Type" info panel (read-only fields)
- "Template Action" buttons

**Bottom bar**: Close button (right-aligned)

---

## Gadgets

### Show Filter (Chooser)

Filters which template types appear in the list.

| Option | Behaviour |
|--------|-----------|
| All | Shows every sub-type (default) |
| Custom Only | Shows only types that have a custom `def_<type>.info` file |
| Missing Only | Shows only types that do NOT have a custom template |

The master row ("ascii") always appears regardless of filter. Changing the filter rebuilds the list and clears the selection.

**Hint:** "Filters which template types appear in the list. The master \"ascii\" row always appears regardless of filter."

### Template List (ListBrowser)

Displays all DefIcons ASCII sub-types with three columns:

| Column | Width | Content |
|--------|-------|---------|
| Type | 35% | Sub-type name. Master ("ascii") is unindented; children are indented |
| Template | 45% | Filename if custom exists (e.g. "def_c.info"), otherwise "-" |
| Status | 20% | "Master", "Custom", "Using master", or "Excluded" |

Single-select with alternating row shading. Selecting a row updates the info panel and enables/disables action buttons.
**Hint:** "Shows all DefIcons ASCII sub-types with their template status. Select a row to see details and enable the action buttons."
### Selected Type — Info Panel

Four read-only fields showing details of the currently selected type:

| Field | Shows |
|-------|-------|
| Type | The raw type name (e.g. "c", "rexx", "ascii") |
| Template file | Custom template filename if it exists, otherwise "-" |
| Effective | The template file that will actually be used at run time |
| Status | "Master", "Custom", "Using master", or "Excluded" |

All fields show "-" or "(none)" when nothing is selected.

### Action Buttons

Four buttons in the "Template Action" group. All start disabled and enable when a list row is selected.

#### Create from master / Overwrite from master

- Label dynamically changes: "Create from master" when no custom template exists, "Overwrite from master" when one already exists.
- Copies `def_ascii.info` to `def_<type>.info`.
- Shows confirmation before overwriting an existing custom template.
- Disabled when master type is selected (you can't copy the master onto itself).
- On success, rebuilds the list to reflect the new file.

**Hint:** "Copies the master template (def_ascii.info) to create or replace a custom template for the selected type."

#### Edit tooltypes...

- Opens the effective template icon in the Workbench Information editor via ARexx (`Info NAME "<path>"` sent to the WORKBENCH ARexx port).
- Creates a zero-byte companion file if needed (required by the Workbench Info command).
- If ARexx/Workbench port unavailable, shows an error requester.
- For the master type, opens `def_ascii.info`. For sub-types without a custom template, also opens `def_ascii.info`.

**Hint:** "Opens the selected template icon in the Workbench Information editor to view and edit its ITIDY_* rendering ToolTypes."

#### Validate tooltypes

- Reads all ToolTypes from the effective template and checks for:
  - Unknown `ITIDY_*` keys (possible typos)
  - Colour values outside 0-255 (or outside 0-254 for `ITIDY_BG_COLOUR`)
  - Percentage values outside 1-100 range
  - Area values not matching `x,y,w,h` format or with w,h not > 0
  - Positive integer values that are zero or negative
- Displays results in a requester with an "Effect Summary" showing all collected values:
  - Text area dimensions, line settings, character width, read bytes
  - Colour indices, darkening percentages
  - Adaptive text and palette expansion on/off status
  - Exclude types list

**Hint:** "Reads all ToolTypes from the effective template and checks for unknown keys, out-of-range values, and formatting errors."

#### Revert to master

- Deletes the custom `def_<type>.info` file so the sub-type falls back to the master.
- Shows a confirmation requester before deleting.
- Disabled when: nothing selected, master type selected, or no custom template exists.
- On success, rebuilds the list. If the type disappears from the current filtered view (e.g. "Custom Only" filter), the selection resets.

**Hint:** "Deletes the custom template for the selected type so it falls back to the master template. A confirmation requester is shown first."

### Close Button

Keyboard shortcut: C. Closes the window and returns to the calling window.

**Hint:** "Closes the Text Templates window and returns to the calling window."

---

## Button State Logic

| Condition | Create/Overwrite | Edit tooltypes | Validate | Revert |
|-----------|-----------------|----------------|----------|--------|
| No selection | Disabled | Disabled | Disabled | Disabled |
| Master ("ascii") selected | Disabled | Enabled | Enabled | Disabled |
| Sub-type, no custom template | Enabled ("Create from master") | Enabled | Enabled | Disabled |
| Sub-type, has custom template | Enabled ("Overwrite from master") | Enabled | Enabled | Enabled |
| Excluded type | Enabled (shows explanation) | Enabled (shows explanation) | Enabled (shows explanation) | Enabled (shows explanation) |

---

## Busy Pointer

A busy pointer is shown during initial window population while loading the EXCLUDETYPE list and checking the filesystem for existing custom templates.

---

## Notes for Manual

- The master template `def_ascii.info` must exist in `PROGDIR:Icons/` for most operations to work.
- Editing tooltypes opens the Workbench Information editor — changes are saved there, not within iTidy itself.
- The Validate function is useful for catching typos in tooltype names and out-of-range values.
- Excluded types must be managed by editing the EXCLUDETYPE tooltype in `def_ascii.info` directly (via Edit tooltypes on the master row).
- The window shows a busy pointer briefly on open while scanning for existing template files.
