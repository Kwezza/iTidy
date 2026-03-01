# Text Templates Window — Working Notes

**Source**: `src/GUI/deficons/text_templates_window.c` (2127 lines), `src/GUI/deficons/text_templates_window.h`
**Title bar**: "iTidy - DefIcons Text Templates"
**Opened from**: Main window -> Settings -> DefIcons -> Text Templates (also accessible from DefIcons Categories window)
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

Template icons contain `ITIDY_*` prefixed ToolTypes that control text preview rendering:

| ToolType | Purpose |
|----------|---------|
| `ITIDY_TEXT_AREA` | Rectangle (x,y,w,h) defining where text is drawn |
| `ITIDY_EXCLUDE_AREA` | Rectangle (x,y,w,h) area to skip when drawing |
| `ITIDY_LINE_HEIGHT` | Pixel height of each text line |
| `ITIDY_LINE_GAP` | Pixel gap between text lines |
| `ITIDY_MAX_LINES` | Maximum number of lines to render |
| `ITIDY_CHAR_WIDTH` | Pixel width of each character |
| `ITIDY_READ_BYTES` | Number of bytes to read from the file |
| `ITIDY_BG_COLOR` | Background colour index (0-255) |
| `ITIDY_TEXT_COLOR` | Text colour index (0-255) |
| `ITIDY_MID_COLOR` | Mid-tone colour index (0-255) |
| `ITIDY_DARKEN_PERCENT` | Darkening percentage (0-100) |
| `ITIDY_DARKEN_ALT_PERCENT` | Alternate darkening percentage (0-100) |
| `ITIDY_ADAPTIVE_TEXT` | Enable adaptive text colouring |
| `ITIDY_EXPAND_PALETTE` | Enable palette expansion |

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
  - Colour values outside 0-255 range
  - Percentage values outside 0-100 range
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
