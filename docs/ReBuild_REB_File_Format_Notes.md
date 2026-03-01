# ReBuild `.reb` save file notes (plain-text format)

This document describes the structure of ReBuild GUI designer save files (`.reb`), based on **iTidy32.reb** and verified directly against the ReBuild Amiga E source code (`rebuild.e`, `reactionObject.e`, `menuObject.e`, `reactionListObject.e`, and individual gadget object files).

> **Source authority:** Where the previous version of this document differed from the code, the source code takes precedence. Corrections are marked where they occur.
>
> **Current file format version:** `VER=2` (defined as `FILE_FORMAT_VER=2` in `rebuild.e`). ReBuild will refuse to load a file whose `VER` value is greater than 2.

---

## 1. Top-level layout

A `.reb` file is plain text, split into two parts:

1. **Header section** — global project settings, `KEY=value` format
2. **Object blocks** — windows, gadgets, images, lists, etc.

### 1.1 Header section

The file begins with the magic marker line:

```
-REBUILD-
```

This is followed by `KEY=value` lines (no spaces around `=`), then a single `#` line that ends the header:

```
-REBUILD-
VER=2
NEXTID=407
...
#
```

Blank values are written as `KEY=` (empty).

#### Complete header key reference (from `saveStream()` in `rebuild.e`)

| Key | Type | Meaning |
|-----|------|---------|
| `VER` | int | File format version. Must be `<= 2` or ReBuild refuses to load. |
| `NEXTID` | int | Next free object ID. Must be `>= 1`. |
| `SELECTEDID` | int | ID of the last-selected component (optional — only present when something was selected). |
| `VIEWTMP` | 0/1 | Whether the "buffer layout" view was active when saved. |
| `ADDSETT` | 0/1 | Show settings dialog when adding a gadget (`1`=yes). |
| `PREVIEWCODE` | 0/1 | Whether the code preview window was open (`1`=open). |
| `CODEWINLEFT` | int | Code preview window left position (pixels). |
| `CODEWINTOP` | int | Code preview window top position (pixels). |
| `CODEWINWIDTH` | int | Code preview window width (pixels). |
| `CODEWINHEIGHT` | int | Code preview window height (pixels). |
| `LANGID` | 0 or 1 | Output language: `0` = Amiga E, `1` = C. |
| `USEIDS` | 0/1 | Include gadget ID constants in generated code. |
| `FULLCODE` | 0/1 | Generate full standalone code (not just snippets). |
| `CODEFOLDER` | string | Path where generated code is saved (may be empty). |
| `USEMACROS` | 0/1 | Use Reaction macros in generated code. |

---

## 2. Object blocks

### 2.1 Block boundaries

After the `#` header terminator, the file is a series of object blocks. The **start** of each block is the line:

```
TYPE: <number>
```

A new `TYPE:` line directly after a block's content means the previous block has ended and a new one begins.

The **end of the payload section** of a block is a line containing only:

```
-
```

Blocks follow each other with no blank lines between them.

### 2.2 Serialisation order

Objects are written in this order by `saveStream()` in `rebuild.e`:

1. All **ReactionList** objects (`TYPE: 0`) — shared data lists used by chooser/cycle gadgets. They appear first because gadgets reference them by ID.
2. Root project objects in order: **Requester** (`TYPE: 49`), **Screen** (`TYPE: 1`).
3. For each window in the project: **Window** (`TYPE: 3`), its **Menu** (`TYPE: 4`), and its root **Layout** (`TYPE: 17`).
4. Child gadgets and layouts are written recursively by `serialiseChildren()`, immediately after their parent block.

> **Key insight for layout operations:** Although blocks are *written* depth-first (parent then children), the loader (`loadStream()` in `rebuild.e`) reads all blocks into a flat list and then rebuilds the tree entirely from `PARENTID:` values using `processObjects()`. This means **block order in the file is cosmetic only** — the tree structure is solely determined by `PARENTID:` values. An AI agent can safely move gadgets between layouts, or create new layout blocks anywhere in the file, as long as `PARENTID:` values are correct and all `ID:` values are unique. See section 10 for the full procedure.

### 2.3 Common metadata fields (all object types)

Every block begins with the same metadata lines, in exactly this order:

```
TYPE: <n>
ID: <n>
PARENTID: <n>          (empty value -> root/unparented object: "PARENTID:")
NAME: <string>
IDENT: <string>
LABEL: <string>
HINT: <text>           (zero or more lines — see section 3.2)
MINWIDTH: <n>
MINHEIGHT: <n>
MAXWIDTH: <n>
MAXHEIGHT: <n>
WEIGHTEDWIDTH: <n>
WEIGHTEDHEIGHT: <n>
SCALEWIDTH: <n>
SCALEHEIGHT: <n>
NOMINALSIZE: <0|1>
WEIGHTMINIMUM: <0|1>
CACHEDOMAIN: <0|255>
NODISPOSE: <0|1>
WEIGHTBAR: <0|1>
EXPANDED: <0|1>
```

| Field | Notes |
|-------|-------|
| `TYPE` | Numeric type code — see section 5 for the complete table. **Do not change.** |
| `ID` | Unique integer ID for this object, referenced by children via `PARENTID`. **Do not change.** |
| `PARENTID` | ID of the parent object, or empty for root objects. **Do not change.** |
| `NAME` | ReBuild internal object name (max 80 chars). Used for identification within ReBuild's UI. |
| `IDENT` | Generated code variable name (max 80 chars). This is what appears in generated E/C source code. |
| `LABEL` | Layout label shown next to the gadget in the generated UI (max 80 chars). May be empty. |
| `HINT` | Tooltip/hint text — one `HINT:` line per line of text. Zero or more lines. See section 3.2. |
| `MINWIDTH/MINHEIGHT` | Minimum size constraints (`-1` = no constraint). |
| `MAXWIDTH/MAXHEIGHT` | Maximum size constraints (`-1` = no constraint). |
| `WEIGHTEDWIDTH/HEIGHT` | Relative weight for layout sizing (default 100). |
| `SCALEWIDTH/HEIGHT` | Fixed pixel override (0 = use weight). |
| `NOMINALSIZE` | Boolean: use nominal size. |
| `WEIGHTMINIMUM` | Boolean: treat weighted size as minimum. |
| `CACHEDOMAIN` | Amiga boolean (255 = true, 0 = false): cache layout domain. |
| `NODISPOSE` | Boolean: do not dispose this object. |
| `WEIGHTBAR` | Boolean: this child acts as a layout weight bar. |
| `EXPANDED` | Boolean: node is expanded in ReBuild's component tree view. |

### 2.4 Payload separator `--` and terminator `-`

After the common metadata, **every block always writes:**

```
--
```

This marks the start of the type-specific payload.

**If the object has type-specific data**, the payload fields follow and the block ends with:

```
-
```

**Even if there is no meaningful payload** (e.g. a Space gadget), `--` and `-` are both still written:

```
TYPE: 25
ID: 251
...
EXPANDED: 0
--
-
```

In practice, every block in a real `.reb` file has both `--` and `-`.

---

## 3. Payload fields (type-specific, after `--`)

Payload keys use `KEY: value` style (note the **space after the colon**). All key names are written in **UPPERCASE** by the serialiser.

### 3.1 ReactionList (`TYPE: 0`) — list data

These are the named lists that feed chooser/cycle gadgets. One `LISTITEM:` line per entry:

```
--
LISTITEM: Folders first
LISTITEM: Files first
LISTITEM: Mixed
LISTITEM: Grouped by type
-
```

**Safe to edit:** the value of each `LISTITEM:` line.
Do not add or remove lines unless intentionally changing the list content.

### 3.2 HINT lines — position and format

> **Correction from the previous version of this document:**
> `HINT:` lines are in the **metadata section, before `--`**, not in the payload after `--`. They are written between `LABEL:` and `MINWIDTH:`.

Each `HINT:` line is one line of tooltip text (multi-line hints have one `HINT:` per line):

```
LABEL:
HINT: Select the folder you want to tidy. To include subfolders, enable 'Include subfolders' below.
MINWIDTH: -1
...
EXPANDED: 0
--
```

If there is no hint, no `HINT:` lines are emitted (zero lines).

- **Safe to edit:** the text value after `HINT: `.
- **Do not** place `HINT:` lines after `--` — the parser will not read them.
- **HARD LIMIT — 194 characters maximum per hint line.** ReBuild saves hints using `StringF(tempStr,'HINT: \s', text)` where `tempStr` is a fixed 200-character buffer (`DEF tempStr[200]:STRING` in `reactionObject.e`). The prefix `HINT: ` occupies 6 characters, leaving exactly **194 characters** for the text. Any hint text longer than 194 characters is **silently truncated** at save time with no warning — the GUI hint editor accepts unlimited input but the file will contain only the first 194 characters. AI agents **must** enforce this limit before writing.
- **Regex:** `^HINT:\s(.+)$`

### 3.3 Window (`TYPE: 3`) payload keys

```
TITLE: <string>           Window title bar text
SCREENTITLE: <string>     Screen title (may be empty)
ICONTITLE: <string>       Iconified window title
ICONFILE: <string>        Path to icon file (may be empty)
LEFTEDGE: <int>
TOPEDGE: <int>
WIDTH: <int>
HEIGHT: <int>
WINDOWPOS: <0-4>          0=manual, 1=centre screen, 2=centre mouse, 3=centre parent, 4=remember
LOCKWIDTH: <0|1>
LOCKHEIGHT: <0|1>
SHAREDPORT: <0|1>
ICONIFYGADGET: <0|1>
GADGETHELP: <0|1>         Enables gadget help (hint display)
REFRESHTYPE: <0|1>
FLAGS: <long>             WA_Flags bitmask
IDCMP: <long>             IDCMP flags bitmask
PREVIEWOPEN: <0|1>
PREVIEWLEFT: <int>
PREVIEWTOP: <int>
PREVIEWWIDTH: <int>
PREVIEWHEIGHT: <int>
```

**Safe to edit:** `TITLE:`, `SCREENTITLE:`, `ICONTITLE:`.
Do not edit `FLAGS:`, `IDCMP:`, or position fields unless you know what you are doing.

### 3.4 Menu (`TYPE: 4`) payload structure

> **Correction from the previous version:** The key is `ITEMNAME:`. The keys `MENUNAME:` and `SUBITEMNAME:` do **not** exist in the ReBuild codebase and were incorrect in the previous version.

The menu block uses a custom serialiser. After `--`, it writes a flat sequence of records — one group of 9 lines per menu entry — then `-`:

```
--
ITEMNAME: Presets
COMMKEY:
TYPE: 0
MENUBAR: 0
CHECK: 0
TOGGLE: 0
CHECKED: 0
DISABLED: 0
MUTUALGROUP: 0
ITEMNAME: Reset to Defaults
COMMKEY: N
TYPE: 1
MENUBAR: 0
CHECK: 0
TOGGLE: 0
CHECKED: 0
DISABLED: 0
MUTUALGROUP: 0
ITEMNAME: bar8
COMMKEY:
TYPE: 1
MENUBAR: 255
CHECK: 0
TOGGLE: 0
CHECKED: 0
DISABLED: 0
MUTUALGROUP: 0
-
```

> **Important:** Within the menu payload, `TYPE:` is the **menu entry type**, NOT the block object type. Values:
> - `0` = `MENU_TYPE_MENU` — top-level menu heading (e.g. "Presets")
> - `1` = `MENU_TYPE_MENUITEM` — item under a heading
> - `2` = `MENU_TYPE_MENUSUB` — sub-item under an item

Menu field meanings:

| Field | Meaning |
|-------|---------|
| `ITEMNAME` | Displayed menu text. Separator bars conventionally use names like `bar`, `bar1`, `bar7`. |
| `COMMKEY` | Keyboard shortcut (single character, or empty). |
| `TYPE` | Entry type: 0=menu heading, 1=item, 2=sub-item. |
| `MENUBAR` | `255` = this entry is a separator bar (ignore ITEMNAME). `0` = normal entry. |
| `CHECK` | `1` = entry has a checkmark gadget. |
| `TOGGLE` | `1` = checkmark toggles on/off. |
| `CHECKED` | `1` = item starts in checked state. |
| `DISABLED` | `1` = item starts disabled/greyed. |
| `MUTUALGROUP` | Mutual-exclusion group number (0 = none). |

**Safe to edit:** `ITEMNAME:` values, `COMMKEY:` values.
**Do not edit:** `TYPE:`, `MENUBAR:`, the ordering of entries.
**Separator detection:** `MENUBAR: 255` is the definitive flag. The name (e.g. `bar8`) is a ReBuild label only, not shown in the actual Amiga menu.

### 3.5 Layout (`TYPE: 17`) payload keys

```
ORIENTATION: <0|1>      0=horizontal, 1=vertical
HORIZALIGNMENT: <0-2>   0=left, 1=right, 2=centre
VERTALIGNMENT: <0-2>    0=top, 1=bottom, 2=centre
BEVELSTYLE: <0-10>
BEVELSTATE: <0-2>
LEFTSPACING: <int>
RIGHTSPACING: <int>
TOPSPACING: <int>
BOTTOMSPACING: <int>
LABELPLACE: <0-5>
FIXEDHORIZ: <0|1>
FIXEDVERT: <0|1>
SHRINKWRAP: <0|1>
EVENSIZE: <0|1>
SPACEOUTER: <0|1>
SPACEINNER: <0|1>
DEFERLAYOUT: <0|1>
```

### 3.6 Button (`TYPE: 5`) payload keys

```
TEXTPEN: <int>        Pen index (-1 = default)
BGPEN: <int>
FILLTEXTPEN: <int>
FILLPEN: <int>
AUTOBUTTON: <0-10>    0=none, or BAG_* constant index
BEVELSTYLE: <0-3>     0=none, 1=thin, 2=button, 3=group
JUSTIFY: <0-2>        0=left, 1=centre, 2=right
SELECTED: <0|1>
DISABLED: <0|1>
READONLY: <0|1>
PUSHBUTTON: <0|1>
TRANSPARENT: <0|1>
```

### 3.7 Chooser / Cycle gadget (`TYPE: 8`) payload keys

```
LISTOBJECTID: <int>   ID of the ReactionList object supplying labels (-1 if none)
MAXLABELS: <int>
ACTIVE: <int>         Initially selected index
WIDTH: <int>          Fixed width (-1 = auto)
READONLY: <0|1>
DISABLED: <0|1>
AUTOFIT: <0|1>
POPUP: <0|1>
DROPDOWN: <0|1>
```

The `LISTOBJECTID` links to a `TYPE: 0` ReactionList block by its `ID`. This is why ReactionLists are saved first in the file.

### 3.8 Text-bearing keys — safe-edit allow-list

Only edit the **values** of these keys. Confirmed directly from source code.

| Key | Where? | Notes |
|-----|--------|-------|
| `HINT:` | Metadata (before `--`) | One entry per tooltip line |
| `LISTITEM:` | ReactionList payload | One entry per list item |
| `ITEMNAME:` | Menu payload | Displayed menu text (skip entries where `MENUBAR: 255`) |
| `TITLE:` | Window payload | Window title bar |
| `SCREENTITLE:` | Window payload | Screen title bar |
| `ICONTITLE:` | Window payload | Iconified window title |
| `LABEL:` | Metadata (before `--`) | Gadget layout label |

**Read-only (do not edit values):**
`TYPE:`, `ID:`, `PARENTID:`, `IDENT:`, all numeric/flag fields.

> **Note on `TEXT:` and `HELP:`**: These keys do **not** appear in the ReBuild serialiser. The button label text is taken from `NAME:` at code-generation time, not stored separately. Do not invent `TEXT:` or `HELP:` keys.

---

## 4. How to find the right block

- Match by `NAME:` first (most stable human-readable identifier).
- Confirm context by walking `PARENTID:` up to the window block.
- Do not use `ID:` as a primary anchor — IDs can be renumbered when ReBuild edits the project.
- Do not use block position/order — this can change after GUI edits.

Window blocks have an empty `PARENTID:` and a distinctive `NAME:` (e.g. `main_window`).
Gadgets under a window share a naming prefix (e.g. `main_...`) and chain up through layouts to the window.

---

## 5. TYPE value table

Exact numeric values from `EXPORT ENUM` in `reactionObject.e`. The enum starts at 0 and increments by 1.

| Value | Constant | Description |
|------:|----------|-------------|
| 0 | `TYPE_REACTIONLIST` | Named list (data source for chooser/cycle gadgets) |
| 1 | `TYPE_SCREEN` | Screen object |
| 2 | `TYPE_REXX` | ARexx port settings |
| 3 | `TYPE_WINDOW` | Window |
| 4 | `TYPE_MENU` | Menu strip |
| 5 | `TYPE_BUTTON` | Button gadget |
| 6 | `TYPE_BITMAP` | Bitmap image |
| 7 | `TYPE_CHECKBOX` | Checkbox gadget |
| 8 | `TYPE_CHOOSER` | Chooser (cycle/popup) gadget |
| 9 | `TYPE_CLICKTAB` | ClickTab (tab strip) gadget |
| 10 | `TYPE_FUELGAUGE` | Fuel gauge / progress bar gadget |
| 11 | `TYPE_GETFILE` | File requester gadget |
| 12 | `TYPE_GETFONT` | Font requester gadget |
| 13 | `TYPE_GETSCREENMODE` | Screen mode requester gadget |
| 14 | `TYPE_INTEGER` | Integer input gadget |
| 15 | `TYPE_PALETTE` | Palette gadget |
| 16 | `TYPE_PENMAP` | Pen map image |
| 17 | `TYPE_LAYOUT` | Layout container |
| 18 | `TYPE_LISTBROWSER` | List browser gadget |
| 19 | `TYPE_RADIO` | Radio button group |
| 20 | `TYPE_SCROLLER` | Scroller gadget |
| 21 | `TYPE_SPEEDBAR` | Speed bar / toolbar gadget |
| 22 | `TYPE_SLIDER` | Slider gadget |
| 23 | `TYPE_STATUSBAR` | Status bar |
| 24 | `TYPE_STRING` | String (text input) gadget |
| 25 | `TYPE_SPACE` | Space (empty filler) gadget |
| 26 | `TYPE_TEXTFIELD` | Text field gadget |
| 27 | `TYPE_BEVEL` | Bevel box / decorative border |
| 28 | `TYPE_DRAWLIST` | Draw list gadget |
| 29 | `TYPE_GLYPH` | Glyph image |
| 30 | `TYPE_LABEL` | Label image |
| 31 | `TYPE_COLORWHEEL` | Colour wheel gadget |
| 32 | `TYPE_DATEBROWSER` | Date browser gadget |
| 33 | `TYPE_GETCOLOR` | Colour requester gadget |
| 34 | `TYPE_GRADSLIDER` | Gradient slider gadget |
| 35 | `TYPE_LISTVIEW` | List view gadget |
| 36 | `TYPE_PAGE` | Page / virtual page gadget |
| 37 | `TYPE_PROGRESS` | Progress indicator |
| 38 | `TYPE_SKETCH` | Sketchboard gadget |
| 39 | `TYPE_TAPEDECK` | Tape deck (media control) gadget |
| 40 | `TYPE_TEXTEDITOR` | Text editor gadget |
| 41 | `TYPE_TEXTENTRY` | Text entry gadget |
| 42 | `TYPE_VIRTUAL` | Virtual group gadget |
| 43 | `TYPE_BOINGBALL` | Boing ball animation |
| 44 | `TYPE_LED` | LED indicator gadget |
| 45 | `TYPE_PENMAP` (2nd) | Pen map second enum entry (reserved/extended) |
| 46 | `TYPE_SMARTBITMAP` | Smart bitmap |
| 47 | `TYPE_TITLEBAR` | Title bar |
| 48 | `TYPE_TABS` | Tabs gadget |
| 49 | `TYPE_REQUESTER` | Requester (dialog) container |
| 50 | `TYPE_REQUESTER_ITEM` | Item within a Requester |

`TYPE_MAX` = 51 (one past the last valid value).

---

## 6. Complete block anatomy examples

### 6.1 ReactionList (TYPE 0)

```
TYPE: 0
ID: 41
PARENTID:
NAME: main_OrderBy
IDENT: ReactionList_41
LABEL:
MINWIDTH: -1
MINHEIGHT: -1
MAXWIDTH: -1
MAXHEIGHT: -1
WEIGHTEDWIDTH: 100
WEIGHTEDHEIGHT: 100
SCALEWIDTH: 0
SCALEHEIGHT: 0
NOMINALSIZE: 0
WEIGHTMINIMUM: 0
CACHEDOMAIN: 255
NODISPOSE: 0
WEIGHTBAR: 0
EXPANDED: 0
--
LISTITEM: Folders first
LISTITEM: Files first
LISTITEM: Mixed
LISTITEM: Grouped by type
-
```

### 6.2 GetFile gadget (TYPE 11) — HINT in metadata (before `--`)

```
TYPE: 11
ID: 52
PARENTID: 28
NAME: Folder to tidy
IDENT: folder_name
LABEL:
HINT: Select the folder you want to tidy. To include subfolders, enable 'Include subfolders' below.
MINWIDTH: -1
MINHEIGHT: -1
MAXWIDTH: -1
MAXHEIGHT: -1
WEIGHTEDWIDTH: 100
WEIGHTEDHEIGHT: 100
SCALEWIDTH: 0
SCALEHEIGHT: 0
NOMINALSIZE: 0
WEIGHTMINIMUM: 0
CACHEDOMAIN: 255
NODISPOSE: 0
WEIGHTBAR: 0
EXPANDED: 0
--
FILEGADGETNAME:
-
```

### 6.3 Space gadget (TYPE 25) — no meaningful payload

```
TYPE: 25
ID: 251
PARENTID: 224
NAME: Space_251
IDENT: Space_251
LABEL:
MINWIDTH: -1
...
EXPANDED: 0
--
-
```

### 6.4 Menu (TYPE 4) — excerpt

```
TYPE: 4
ID: 5
PARENTID:
NAME: Menu_5
IDENT: Menu_5
LABEL:
MINWIDTH: -1
...
EXPANDED: 0
--
ITEMNAME: Presets
COMMKEY:
TYPE: 0
MENUBAR: 0
CHECK: 0
TOGGLE: 0
CHECKED: 0
DISABLED: 0
MUTUALGROUP: 0
ITEMNAME: Reset to Defaults
COMMKEY: N
TYPE: 1
MENUBAR: 0
CHECK: 0
TOGGLE: 0
CHECKED: 0
DISABLED: 0
MUTUALGROUP: 0
ITEMNAME: bar8
COMMKEY:
TYPE: 1
MENUBAR: 255
CHECK: 0
TOGGLE: 0
CHECKED: 0
DISABLED: 0
MUTUALGROUP: 0
-
```

---

## 7. Safe editing rules

### 7.1 What is safe to change

Values of the keys listed in section 3.8:
`HINT:`, `LISTITEM:`, `ITEMNAME:`, `TITLE:`, `SCREENTITLE:`, `ICONTITLE:`, `LABEL:`.
`COMMKEY:` if intentionally updating keyboard shortcuts (single character or empty).

### 7.2 What must not be changed (during text-only edits)

- `TYPE:` (block-level) — destroys object interpretation
- `ID:` — every object's unique numeric identity; **never reuse or change an existing ID**
- `PARENTID:` — must not be changed during text-only edits; **see section 10 for the controlled procedure** for layout restructuring operations where `PARENTID:` changes are the intended edit
- `IDENT:` — used as the variable name in generated source code
- Block structure lines: `-REBUILD-`, `#`, `--`, `-`
- Header key `VER=` — always keep as-is
- Header key `NEXTID=` — **must be incremented** when adding new objects (see section 10)
- Numeric / flag fields: `FLAGS:`, `IDCMP:`, `LANGID`, etc.

### 7.3 Key formatting rules

- Header: `KEY=value` (no spaces around `=`)
- All other lines: `KEY: value` (colon, then one space, then value)
- Empty values: `KEY: ` or `KEY:` (both accepted)
- All payload keys are in **UPPERCASE**
- One key/value per line — do not wrap

### 7.4 Deterministic rewrite policy

- Only replace the value portion (after `KEY: `), never the key name.
- Preserve one space between colon and value: `HINT: corrected text`.
- Do not add leading/trailing whitespace to values.
- Do not start a hint value with `SOMETHING:` — it could be misread as a key.
- Prefer consistent terminology (`folder` vs `drawer`); pick one term and use it throughout.
- Use ellipsis (`...`) only on labels that open a further requester/dialog.
- **Hint length hard limit: 194 characters maximum per `HINT:` line.** This limit comes from the fixed 200-char write buffer in `reactionObject.e` (`DEF tempStr[200]:STRING`) minus the 6-char `HINT: ` prefix. Longer text is silently truncated at save. Always trim hint text to 194 chars or fewer before writing.

### 7.5 Line endings

Treat `.reb` as an **Amiga text file**: **LF only** (no CRLF). Suggested `.gitattributes`:

```
*.reb  text  eol=lf
```

### 7.6 String field length limits

| Field | Max length |
|-------|-----------|
| `NAME:`, `IDENT:`, `LABEL:`, `ITEMNAME:` | 79 chars (stored in 80-char arrays) |
| `TITLE:`, `SCREENTITLE:`, `ICONTITLE:` | No ReBuild truncation |
| `HINT:` per line | **194 chars maximum** — hard limit. `HINT: ` prefix (6 chars) + text fills a fixed 200-char write buffer (`DEF tempStr[200]:STRING` in `reactionObject.e`). Text beyond 194 chars is silently truncated at save. |
| `COMMKEY:` | 1 char (stored in 2-char array) |

---

## 8. Parsing / regex crib sheet

### 8.1 Block boundaries

```
Header ends at:          ^#$
Block payload starts at: ^--$
Block payload ends at:   ^-$
New block starts when:   ^TYPE: \d+$
```

### 8.2 Metadata field anchors

```python
^TYPE:\s+(\d+)$
^ID:\s+(\d+)$
^PARENTID:\s*(\d*)$
^NAME:\s*(.*)$
^IDENT:\s*(.*)$
^LABEL:\s*(.*)$
^HINT:\s(.+)$            # note: one guaranteed space before value
^EXPANDED:\s+(\d+)$
```

### 8.3 Editable text field anchors

```python
^HINT:\s(.+)$
^LISTITEM:\s(.+)$
^ITEMNAME:\s(.+)$
^TITLE:\s(.+)$
^SCREENTITLE:\s*(.*)$
^ICONTITLE:\s*(.*)$
^LABEL:\s*(.*)$
```

### 8.4 Menu separator detection

```python
^MENUBAR:\s+255$         # definitive separator flag (255 = TRUE in Amiga boolean)
```

### 8.5 Menu entry TYPE within menu payload only

```
TYPE: 0   -> top-level menu heading
TYPE: 1   -> menu item
TYPE: 2   -> sub-item
```

---

## 9. Recommended AI-assisted edits workflow

### 9.1 Parsing strategy

1. Read line by line.
2. Collect the header (up to `#`) into `KEY=value` pairs.
3. After `#`, detect block starts at each `TYPE: <n>` line.
4. Within each block, accumulate metadata lines until `--` is reached.
5. After `--`, accumulate payload lines until `-` is reached.
6. Build a map: `ID -> {type, name, ident, parentId, hints[], label, payload{}}`.

### 9.2 Safe bulk edit algorithm

```
for each block:
  if block.type == 0:     -> ReactionList
      edit each LISTITEM: value
  if block.type == 4:     -> Menu
      edit each ITEMNAME: value where MENUBAR != 255
  edit each HINT: value (in metadata section, before --)
  if block.type == 3:     -> Window
      edit TITLE:, SCREENTITLE:, ICONTITLE: values
  if LABEL: is non-empty:
      edit LABEL: value
```

### 9.3 Re-emission rules

- Emit blocks in **identical order** to the input.
- All IDs unchanged.
- Only change values of keys in the allow-list (section 3.8).
- Preserve `--` and `-` placement exactly.
- Preserve LF-only line endings.

### 9.4 Validation

After editing:

1. Count `TYPE:` lines — must match original.
2. Count `-` block terminators — must match original.
3. Load the edited `.reb` in ReBuild and confirm it parses without error.

---

## 10. Layout tree operations (move, wrap, create)

This section documents how an AI agent can safely restructure the layout tree: moving gadgets between layouts, wrapping gadgets in a new layout, and creating new layout objects. These are structural edits — they require changing `PARENTID:` values and sometimes adding new blocks — but they are fully mechanical and documentable.

### 10.1 How the tree is represented

The layout hierarchy is stored purely through `PARENTID:` cross-references. There is no nesting of block text — all blocks are flat in the file. The loader ignores file order entirely and rebuilds the tree from `PARENTID:` after reading all blocks:

```
Root layout (TYPE: 17, PARENTID: empty)
  ├── Horiz layout (TYPE: 17, PARENTID: <root_id>)
  │     ├── Button (TYPE: 6, PARENTID: <horiz_id>)
  │     └── String (TYPE: 23, PARENTID: <horiz_id>)
  └── ListBrowser (TYPE: 18, PARENTID: <root_id>)
```

Each block stands alone. Moving a gadget means changing its `PARENTID:` value only.

### 10.2 Moving a gadget into a different layout

**Precondition:** Know the `ID:` of the target layout (`TYPE: 17` block).

**Procedure:**
1. Find the gadget block by its `NAME:` or `IDENT:` field.
2. Change its `PARENTID: <old>` line to `PARENTID: <target_layout_id>`.
3. That is the only change required. No block ordering change is needed.

**Example:** Move button `ID: 12` from layout `ID: 7` to layout `ID: 40`:
```
Before:  PARENTID: 7
After:   PARENTID: 40
```

### 10.3 Moving a gadget out to the root layout

Same procedure as 10.2. The root layout is the `TYPE: 17` block that has an empty `PARENTID:` and appears immediately after the window's Menu block (`TYPE: 4`) in the file. Change the gadget's `PARENTID:` to the root layout's `ID:`.

### 10.4 Creating a new layout

When wrapping gadgets in a new layout container or adding a new column/row, a new `TYPE: 17` block must be created.

**Procedure:**
1. Allocate a new ID: read `NEXTID=` from the header; the new ID is that value. Increment `NEXTID=` by 1 in the header.
2. Choose an orientation: `ORIENTATION: 0` = horizontal, `ORIENTATION: 1` = vertical.
3. Insert the new block anywhere in the file (location does not matter to the loader). Recommended: insert it immediately before the first gadget that will become its child, to keep the file readable.
4. Set the new layout's `PARENTID:` to the ID of the layout that will contain it.
5. Change the `PARENTID:` of the gadgets that should be inside it to the new layout's ID.

**Minimal new layout block template** (copy, then fill in `ID:`, `PARENTID:`, `NAME:`, and `ORIENTATION:`):

```
TYPE: 17
ID: <new_id>
PARENTID: <parent_layout_id>
NAME: <Horiz|Vert>_<new_id>
IDENT: <Horiz|Vert>_<new_id>
LABEL: 
MINWIDTH: -1
MINHEIGHT: -1
MAXWIDTH: -1
MAXHEIGHT: -1
WEIGHTEDWIDTH: 100
WEIGHTEDHEIGHT: 100
SCALEWIDTH: 0
SCALEHEIGHT: 0
NOMINALSIZE: 0
WEIGHTMINIMUM: 0
CACHEDOMAIN: 255
NODISPOSE: 0
WEIGHTBAR: 0
EXPANDED: 0
--
ORIENTATION: 1
HORIZALIGNMENT: 0
VERTALIGNMENT: 0
BEVELSTYLE: 0
BEVELSTATE: 0
LEFTSPACING: 0
RIGHTSPACING: 0
TOPSPACING: 0
BOTTOMSPACING: 0
LABEL: 
LABELPLACE: 0
FIXEDHORIZ: 255
FIXEDVERT: 255
SHRINKWRAP: 0
EVENSIZE: 0
SPACEOUTER: 0
SPACEINNER: 255
DEFERLAYOUT: 0
-
```

> The two `LABEL:` lines are distinct: the first is in the metadata section (before `--`) and sets the common reaction object label; the second is inside the layout payload (after `--`) and sets the Reaction `LAYOUT_Label` bevel label. Both can be empty.

### 10.5 Orientation values

| `ORIENTATION:` value | Meaning |
|---|---|
| `0` | Horizontal — children laid out left-to-right |
| `1` | Vertical — children laid out top-to-bottom |

Convention used by `create()` in `layoutObject.e`: default name is `Vert_<id>` for vertical, `Horiz_<id>` for horizontal. This is just a naming convention — it has no effect on the layout direction, which is controlled solely by `ORIENTATION:`.

### 10.6 NEXTID bookkeeping

The header field `NEXTID=` records the next free ID. **Every time a new object block is added, `NEXTID=` must be incremented by 1.** If adding multiple objects in one edit pass, increment by the total count added.

- `NEXTID=` must always be greater than the highest `ID:` value in the file.
- ReBuild validates `NEXTID >= 1` on load — setting it too low causes a load error.
- Safe approach: find the highest `ID:` in all blocks and set `NEXTID=` to `max_id + count_of_new_objects`.

### 10.7 Layout operation safety rules

- **Never remove a layout block while it still has children.** Reassign all children first, or delete them if they are being removed deliberately.
- **Never give two blocks the same `ID:`.** The tree rebuild processes each ID exactly once.
- **A gadget's type determines whether it can have children.** Only `TYPE: 17` (Layout) has `allowChildren()` returning non-zero. Do not set a non-layout gadget as the `PARENTID:` of another gadget — the loader will accept it but the UI will be broken.
- **The root layout (`TYPE: 17`, `PARENTID:` empty) must always exist** for each window. Never remove it. Other gadgets should all ultimately trace their `PARENTID:` chain up to this root layout.
- After any structural edit, validate: every `PARENTID:` value (except empty ones) must match an `ID:` that exists in the file.

### 10.8 Example: wrap two buttons in a new horizontal layout

**Before:** Two buttons are direct children of root layout `ID: 5`.
```
TYPE: 6        <- Button A
ID: 8
PARENTID: 5
...
TYPE: 6        <- Button B
ID: 9
PARENTID: 5
```

**Header before:** `NEXTID=42`

**Steps:**
1. New layout ID = 42. Change header to `NEXTID=43`.
2. Insert new horizontal layout block (from template 10.4) with `ID: 42`, `PARENTID: 5`, `ORIENTATION: 0`, `NAME: Horiz_42`, `IDENT: Horiz_42`.
3. Change both buttons: `PARENTID: 5` → `PARENTID: 42`.

**After:**
```
TYPE: 17       <- new Horiz layout
ID: 42
PARENTID: 5
NAME: Horiz_42
...
TYPE: 6        <- Button A
ID: 8
PARENTID: 42
...
TYPE: 6        <- Button B
ID: 9
PARENTID: 42
```

---

## 11. Renaming components (mockup → production cleanup)

During rapid mockup work, ReBuild auto-generates names like `Vert_354`, `Horiz_347`, `Button_348`. This section documents how an AI agent can systematically rename a window's components to meaningful identifiers — safely, in a single editing pass.

### 11.1 What needs renaming and why

There are exactly three categories of components that end up with generic names after mockup work:

| Category | `NAME:` example | `IDENT:` example | Cause |
|---|---|---|---|
| **Layout containers** (`TYPE: 17`) | `Vert_354` | `Vert_354` | Always auto-named; no display text available |
| **Buttons with unset IDENT** (`TYPE: 5/6`) | `Ok` | `Button_348` | Display text (`NAME:`) was set but IDENT was never updated |
| **Labels with unset IDENT** (`TYPE: 30`) | `Picture formats:` | `Label_392` | Same as above |
| **Space gadgets** (`TYPE: 25`) | `Space_364` | `Space_364` | Spacers have no meaningful content to name from |

Everything else — checkboxes, choosers, string gadgets, etc. — typically already has a meaningful `IDENT:` if the developer set the display text through the ReBuild UI, because ReBuild uses the first-set `NAME:` to seed `IDENT:` at creation time.

### 11.2 The roles of `NAME:` and `IDENT:`

These are **different fields** serving different purposes. Both must be updated consistently:

| Field | Used for | Max | Changes affect |
|---|---|---|---|
| `NAME:` | Label in ReBuild's component tree panel; button/label display text in the UI | 79 chars | Visual display only — safe to change any time |
| `IDENT:` | Variable name in generated Amiga E / C source code | 79 chars | Generated source code — safe to change in mockup phase before code generation |

For **buttons**: `NAME:` IS the button's displayed text (`GA_TEXT, self.name` in `buttonObject.e`). Do not change `NAME:` for buttons when the goal is identifier cleanup — only update `IDENT:`.

For **layouts**: `NAME:` has no visual effect. Update both `NAME:` and `IDENT:` to the same new value.

### 11.3 `IDENT:` sanitization rules

`IDENT:` is used directly as a source code variable/enum name. The value must conform to C and Amiga E identifier rules:

- Only lowercase letters, digits, and underscores
- Must start with a letter
- No spaces, no punctuation, no brackets
- Max 79 characters
- Must be **unique across the entire `.reb` file** (not just within a window — the .reb requires global uniqueness even though the generated C enums are scoped per window)

**Sanitization procedure for a display text like `"Picture formats:"`:**
1. Strip non-alphanumeric characters → `Picture formats`
2. Replace spaces/hyphens with `_` → `Picture_formats`
3. Lowercase → `picture_formats`
4. Add window prefix → `dco_picture_formats`
5. For layouts: add orientation suffix → `dco_picture_formats_row` (horiz) or `dco_picture_formats_col` (vert)

### 11.4 Naming convention

The convention used by this project is `windowprefix_descriptive_role`:

- Window abbreviation prefix (3-6 chars, derived from window `IDENT:` by taking initials or first word)
- Descriptive role based on the component's function or the text of its most significant child
- For layouts: append `_horiz` or `_vert` to disambiguate orientation

**Window abbreviation examples:**
| Window `IDENT:` | Abbreviation |
|---|---|
| `deficons_creation_options` | `dco` |
| `main_window` | `main` |
| `find_dialog` | `find` |
| `preferences` | `prefs` |

### 11.5 How to derive a layout's name from its children

Layouts have no display text. Derive their name by examining direct children and applying the `_row`/`_col` suffix for orientation (`_row` = horizontal `ORIENTATION: 0`, `_col` = vertical `ORIENTATION: 1`):

1. **Single-purpose layouts** (all children relate to one concept): name from that concept.
   - Horizontal layout containing a checkbox + button → `dco_text_preview_options_row`

2. **Grid grouping layouts** (children are other layouts forming a grid): name from what they group.
   - Horizontal layout containing four vertical column layouts of checkboxes → `dco_pic_formats_row`
   - Each of those vertical column layouts → `dco_pic_formats_col1`, `dco_pic_formats_col2`, etc. (suffix omitted — `col` in the name already implies vertical).

3. **Root layout** (direct child of window, `PARENTID:` empty or pointing to the window): name `{prefix}_root`. The orientation is usually obvious from context and a suffix is not needed.

4. **Button-bar layout** (all or most children are buttons, horizontal): name `{prefix}_buttons_row`.

5. **Tab page layout** (direct child of a ClickTab gadget, typically vertical): name `{prefix}_tab_{tabname}_col` or just `{prefix}_{tabname}_content`.

6. **Two-column option panel** (vertical layout forming the left or right half of a horizontal split): use `{prefix}_left_col`, `{prefix}_right_col`.

7. **Ambiguous/mixed layouts** where the purpose is not obvious: use `{prefix}_section{n}_row` / `{prefix}_section{n}_col`, or flag for manual review.

### 11.6 Algorithm for a complete window rename pass

```
Given: window IDENT (e.g. deficons_creation_options → prefix = "dco")

0. BEFORE STARTING: if hand-written C code already references gadgets in this window
   by their old enum names (e.g. button_348, vert_298), collect all those references
   now. After renaming and regenerating, every one of them must be updated. Do the
   .reb rename and the C code find-replace in the same pass.

1. Collect all blocks belonging to this window (PARENTID chain traces to this window's ID).

2. For each block in the window:

   a. If TYPE=17 (Layout):
      - Read ORIENTATION: from payload (0=horiz → _row suffix, 1=vert → _col suffix).
      - Examine direct children (by PARENTID) to derive the descriptive role name.
      - Apply rules from 11.5.
      - Update both NAME: and IDENT: to the new value.

   b. If TYPE=5 or TYPE=6 (Button) AND IDENT matches /^Button_\d+$/:
      - Take NAME: value (the display text — this IS the button label in the UI).
      - Sanitize per 11.3: strip punctuation, lowercase, underscores for spaces.
      - Prefix with window abbreviation.
      - Update IDENT: only. Leave NAME: strictly unchanged.

   c. If TYPE=30 (Label) AND IDENT matches /^Label_\d+$/:
      - Take NAME: value.
      - Sanitize per 11.3, prefix with window abbreviation + "lbl_".
      - Update IDENT: only.

   d. If TYPE=9 (ClickTab) AND IDENT matches /^ClickTab_\d+$/:
      - Rename to {prefix}_tabs.
      - Update both NAME: and IDENT:.

   e. If TYPE=4 (Menu) AND IDENT matches /^Menu_\d+$/:
      - Rename to {prefix}_menu.
      - Update both NAME: and IDENT:.
      - Note: Menu IDENT does NOT appear in the gadget enum in generated C —
        it is cosmetic only. Safe to rename any time with no C code impact.

   f. If TYPE=25 (Space):
      - Rename IDENT to {prefix}_sp{n} where n is a counter (sp1, sp2, ...).
      - Spaces appear in the _idx enum but are virtually never referenced in
        hand-written code. Low priority; rename for completeness.

   g. All other types where IDENT already looks meaningful (not matching
      /^[A-Za-z]+_\d+$/):
      - Leave unchanged.

3. Ensure uniqueness: after preparing all new names, check that no two blocks
   in the entire .reb file share an IDENT. If a collision occurs, append _2, _3 etc.

4. No changes to: TYPE:, ID:, PARENTID:, any payload keys, block structure.
```

### 11.7 Concrete example from iTidy32.reb (`deficons_creation_options` window, prefix `dco`)

The C source currently contains `enum deficons_creation_options_idx { vert_298, clicktab_353, vert_354, ..., horiz_347, button_348, button_349 }`. After this rename pass, hand-written code using any of those old enum values must be updated.

| ID | ORIENTATION | Old IDENT | Action | New IDENT | C enum change |
|---|---|---|---|---|---|
| 297 | Menu | `Menu_297` | Rename both | `dco_menu` | not in gadget enum |
| 298 | VERT (1) | `Vert_298` | Rename both | `dco_root` | `vert_298` → `dco_root` |
| 353 | ClickTab | `ClickTab_353` | Rename both | `dco_tabs` | `clicktab_353` → `dco_tabs` |
| 354 | VERT (1) | `Vert_354` | Rename both | `dco_tab_content` | `vert_354` → `dco_tab_content` |
| 358 | HORIZ (0) | `Horiz_358` | Rename both | `dco_text_preview_row` | `horiz_358` → `dco_text_preview_row` |
| 361 | HORIZ (0) | `Horiz_361` | Rename both | `dco_pic_preview_row` | `horiz_361` → `dco_pic_preview_row` |
| 365 | HORIZ (0) | `Horiz_365` | Rename both | `dco_pic_formats_row` | `horiz_365` → `dco_pic_formats_row` |
| 366 | VERT (1) | `Vert_366` | Rename both | `dco_pic_formats_col1` | `vert_366` → `dco_pic_formats_col1` |
| 367 | VERT (1) | `Vert_367` | Rename both | `dco_pic_formats_col2` | `vert_367` → `dco_pic_formats_col2` |
| 368 | VERT (1) | `Vert_368` | Rename both | `dco_pic_formats_col3` | `vert_368` → `dco_pic_formats_col3` |
| 369 | VERT (1) | `Vert_369` | Rename both | `dco_pic_formats_col4` | `vert_369` → `dco_pic_formats_col4` |
| 378 | VERT (1) | `Vert_378` | Rename both | `dco_icon_settings` | `vert_378` → `dco_icon_settings` |
| 379 | VERT (1) | `Vert_379` | Rename both | `dco_icon_size_options` | `vert_379` → `dco_icon_size_options` |
| 385 | VERT (1) | `Vert_385` | Rename both | `dco_color_settings` | `vert_385` → `dco_color_settings` |
| 347 | HORIZ (0) | `Horiz_347` | Rename both | `dco_buttons_row` | `horiz_347` → `dco_buttons_row` |
| 348 | Button | IDENT=`Button_348` | IDENT only | `dco_ok` | `button_348` → `dco_ok` |
| 349 | Button | IDENT=`Button_349` | IDENT only | `dco_cancel` | `button_349` → `dco_cancel` |
| 392 | Label | IDENT=`Label_392` | IDENT only | `dco_lbl_pic_formats` | `label_392` → `dco_lbl_pic_formats` |

Gadgets already left unchanged (IDENT already descriptive): `deficons_folder_icon_mode`, `deficons_enable_text_previews`, `deficons_manage_templates`, `deficons_enable_picture_previews`, `deficons_pic_ILBM`, `deficons_pic_JPEG`, `deficons_pic_PNG`, `deficons_pic_ACBM`, `deficons_pic_GIF`, `deficons_pic_OTHER`, `deficons_pic_BMP`.

### 11.8 Regeneration and C code impact

When you rename IDENTs in the `.reb` and regenerate C source, **every enum value in `{window}_idx` changes name**. Any hand-written C code that references the old names will fail to compile. This is expected and manageable:

**Impact map for `deficons_creation_options` after the rename in 11.7:**
```c
// OLD generated enum:
enum deficons_creation_options_idx { vert_298, clicktab_353, ..., button_348, button_349 };

// NEW generated enum:
enum deficons_creation_options_idx { dco_root, dco_tabs, ..., dco_ok, dco_cancel };
```

Any code like `main_gadgets[button_348]` or `GetAttr(..., dco_gadgets[button_349], ...)` stops compiling. Do a project-wide find/replace of each old name → new name **in the same session** as the `.reb` edit, before rebuilding.

**Recommended workflow:**
1. Complete the full rename pass on the `.reb` file (all windows, or one window).
2. Regenerate C source from ReBuild.
3. Diff the old and new `_idx` enum to get the exact old→new name mapping.
4. Perform find-replace in all `.c`/`.h` files for each changed name.
5. Build and confirm zero compile errors.

**Key benefit once done:** All gadget references in your C project are now prefixed by window (`mw_`, `dco_`, `adv_`, etc.). Grepping for `dco_` instantly shows all accesses to the deficons creation options window across the entire codebase. An AI agent can reliably identify which window any gadget belongs to from the identifier alone — without needing to trace PARENTID chains.

### 11.9 Validation checklist

1. Every `IDENT:` in the entire `.reb` file is unique — check for duplicates before writing.
2. No `IDENT:` contains spaces, punctuation, or starts with a digit.
3. All layout `NAME:` values match their `IDENT:` (kept in sync).
4. `TYPE:`, `ID:`, `PARENTID:`, all payload keys — unchanged.
5. Total block count unchanged.
6. If hand-written C code exists: the old `_idx` enum values have been found and replaced in all source files.
7. Project builds cleanly after regeneration.

---

## 12. Notes

- `CACHEDOMAIN: 255` means **true** (Amiga boolean: any non-zero value = true; 255 = 0xFF). `CACHEDOMAIN: 0` = false. The same applies to any field described as "boolean" in this document.
- `IDENT:` is the variable name in generated source. Never change it during text-only edits.
- `NAME:` is the human label in ReBuild's component list. Use it for identification but avoid changing it — it seeds the default `IDENT:` when an object is first created.
- Button label text is taken from `NAME:` at code-generation time. There is no separate `TEXT:` payload key for buttons. Do not invent one.
- For gadget types not fully documented here (e.g. `TYPE_TEXTEDITOR` (40), `TYPE_LISTBROWSER` (18), `TYPE_SPEEDBAR` (21)), their payload keys can be found in the corresponding `serialiseData()` method in the relevant `*Object.e` source file in the ReBuild project.