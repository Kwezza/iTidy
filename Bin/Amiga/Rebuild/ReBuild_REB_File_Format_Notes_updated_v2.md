# ReBuild `.reb` save file notes (plain-text format)

This document captures the **observed** structure of ReBuild GUI designer save files (`.reb`) as used by iTidy32.

> Scope: this is not official ReBuild documentation. It’s a practical reverse-engineered guide based on the current `iTidy32.reb` file.

---

## 1. Top-level layout

A `.reb` file is plain text and is broadly split into two parts:

1) **Header section** (global project settings)  
2) **Object blocks** (windows, gadgets, images, etc.)

### 1.1 Header section

The file starts with a marker line:

- `-REBUILD-`

This is followed by multiple **key=value** lines, for example:

- `VER=2`
- `NEXTID=407`
- `CODEWINLEFT=450`

Blank values are represented as `KEY=`.

The header ends at a single line containing:

- `#`

Everything after `#` is a sequence of object blocks.

---

## 2. Object blocks

Each object is stored as a block of lines, and **blocks are separated by a single line containing only:**

- `-`

So the general pattern is:

```
#               (end of header)
<block 1 lines>
-
<block 2 lines>
-
...
```

### 2.1 Block metadata (common fields)

Most blocks begin with a common set of **key: value** lines, for example:

- `TYPE: 0`
- `ID: 41`
- `PARENTID:`
- `NAME: main_OrderBy`
- `IDENT: ReactionList_41`
- `LABEL:`
- sizing/weighting fields such as:
  - `MINWIDTH: -1`
  - `WEIGHTEDWIDTH: 100`
  - `SCALEWIDTH: 0`

Empty values are represented as `KEY:` (no value after the colon).

**Important:** `ID` and `PARENTID` are how objects are related. Changing them will break the layout.

### 2.2 Payload marker `--`

Within a block, a single line containing:

- `--`

marks the start of the object’s **payload** (type-specific properties).

Example:

```
TYPE: 0
ID: 41
NAME: main_OrderBy
...
--
LISTITEM: Folders first
LISTITEM: Files first
...
-
```

Anything after `--` is interpreted as object-specific configuration.


### 2.3 How to find the right block quickly (NAME/IDENT/PARENTID)

When you want to edit a specific window or gadget (for example, `main_window` tooltips), the safest anchors are:

- `NAME:` human-readable object name used by ReBuild (more stable than numeric IDs)
- `IDENT:` generated identifier (often changes less than `ID`, but can still change)
- `PARENTID:` relationship link to the containing window/layout

**Rule of thumb:** match by `NAME:` first, then confirm you are in the expected window by walking `PARENTID:` up to the window block.

Practical patterns that work well:

- Window block: typically has a distinctive `NAME:` (e.g. `main_window`) and an empty `PARENTID:` (or a known root parent).
- Gadgets: usually share a naming prefix (`main_...`) and have `PARENTID:` pointing at a layout/window chain beneath `main_window`.

Avoid anchoring edits on:

- `ID:` (can be renumbered when ReBuild regenerates)
- block position/order (can change after GUI edits)


---

## 3. Common payload fields you’ll care about

### 3.1 ListBrowser / list-like gadgets

List entries are typically stored as repeated lines:

- `LISTITEM: <text>`

Example:

- `LISTITEM: Folders first`
- `LISTITEM: Files first`

These are safe to spellcheck/standardise as long as you keep them as individual `LISTITEM:` lines.

### 3.2 Hints / tooltips

Tooltips are stored as:

- `HINT: <text>`

These are ideal candidates for bulk editing, spelling fixes, shortening, and consistent terminology.


### 3.3 Text-bearing keys catalog (observed + recommended handling)

In practice, the fastest way to do safe bulk edits is to maintain a short allow-list of keys that are known to contain user-facing strings.

**Observed in iTidy32 work so far:**

- `HINT:` tooltip text (safe to edit)
- `LISTITEM:` list/cycle entries (safe to edit as long as each entry stays on its own line)

**Common in other `.reb` files (add here if/when you see them in your file):**

- `LABEL:` gadget label (usually safe to edit, but confirm it is in the block metadata vs payload)
- `TITLE:` window title
- `TEXT:` button/static text contents
- `HELP:` help text / short description fields

**Recommended policy:** only auto-edit keys in this allow-list. Leave everything else untouched.

### 3.4 Minimal block anatomy example (with HINT and LISTITEM)

This is the “shape” you should preserve when editing:

```
TYPE: 0
ID: 41
PARENTID: 10
NAME: main_OrderBy
IDENT: ReactionList_41
...
--
LISTITEM: Folders first
LISTITEM: Files first
HINT: Choose the sort field: name, kind, date, or size.
-
```

Things to preserve exactly:

- the header/payload separator line `--`
- the block terminator `-`
- one key/value per line (do not wrap unless you know ReBuild supports it)


---

### 3.5 Menus (menustrip) blocks

ReBuild represents menus as **Menu** object blocks (commonly named like `Menu_5`). The menu text you see in Workbench is stored in the payload as `MENUNAME:`, `ITEMNAME:`, and `SUBITEMNAME:` lines.

Key points observed from `iTidy32.reb`:

- A single Menu block can contain many entries.
- Text is stored in payload form: `KEY: value` (not `KEY=value`).
- Separators are typically represented by an `ITEMNAME:` like `bar`, `bar1`, `bar6`, etc.
- Menu shortcuts are stored as a single character in `COMMKEY:` (for example `N`, `O`, `S`).
- Ellipsis is literal three dots: `...` (avoid `..`).
- If you use `NAME:` to label menu items inside ReBuild, keep a consistent casing scheme (for example all separators `barX`).

Safe edit scope for menus:

- Safe to edit: the **value** part of `MENUNAME:`, `ITEMNAME:`, `SUBITEMNAME:`, and `COMMKEY:`.
- Do not edit: Menu structure lines, numeric IDs, or any key that appears to control linkage, ordering, or hierarchy.

Practical tip:

- When updating menu labels, scan for repeated labels that appear more than once in the payload. ReBuild may mirror some text fields in multiple places.


## 4. Safe editing rules (practical)

If you manually edit or auto-edit the file:

- **Do not change**: `TYPE`, `ID`, `PARENTID`, `IDENT` unless you’re intentionally restructuring.
- Preserve the block separators:
  - `--` must remain in-place (if present)
  - `-` must remain as the block terminator
- Keep the general key formatting intact:
  - header uses `KEY=value`
  - blocks use `KEY: value`
- Avoid adding/removing random whitespace before keys.
- If you need Amiga-friendly text, keep strings to characters you know are safe in your pipeline (you’ve mentioned ISO/IEC 8859-1 in the wider project).



### 4.1 Deterministic rewrite policy (recommended)

When updating text fields (especially `HINT:` and `LISTITEM:`), use rules that keep edits predictable:

- Only replace the **value** portion (text after `HINT:` / `LISTITEM:`), not the key.
- Keep formatting stable: `KEY:` then a single space then text.
- Do not introduce extra leading/trailing whitespace.
- Avoid inserting lines that look like keys (e.g. starting a hint with `SOMETHING:`).
- Prefer consistent terminology across the UI (e.g. “folder” vs “drawer”). Pick one and stick to it.
- Be conservative with punctuation. Use ellipses only when the UI label opens a window/requester.

If ReBuild truncates hints, add and enforce a max length here (example: 120 characters), then rewrite to fit.

### 4.2 Line endings and Git (practical)

Because these files are edited across Windows and Amiga tooling, line endings can bite.

- Treat `.reb` as an **Amiga text file**: use **LF only** line endings in the repo.
- Do not let Git convert `.reb` to CRLF. CR characters can confuse Amiga-side tools and some editors.
- If you round-trip a `.reb` through Amiga utilities, re-check that the file still contains only LF line endings.

Suggested `.gitattributes` approach:

- Force `.reb` to `eol=lf` and disable automatic conversions for that pattern.

Also consider setting your Git config so it does not auto-convert line endings for this repo, and rely on `.gitattributes` to be explicit.


## 5. Recommended workflow for “AI-assisted edits”

For repeated tasks like “update all HINTs in main_window”:


### 5.1 Quick recipe: update all HINTs for a given window (fast path)

Use this checklist to avoid re-learning the format each time:

1) Locate the window block by `NAME:` (e.g. `main_window`).
2) Identify the set of gadgets/layouts under it:
   - follow `PARENTID:` links downward (or build a parent→children map).
3) For each target gadget block, edit only the `HINT:` line (or add one after `--` if missing).
4) Do **not** modify: `TYPE`, `ID`, `PARENTID`, `IDENT`, ordering, `--`, or `-`.
5) Save and load the `.reb` in ReBuild to validate it parses cleanly.

### 5.2 Regex / parsing crib sheet (for scripts and careful bulk edits)

Block boundaries:

- Header ends at a line that is exactly: `#`
- Each block ends at a line that is exactly: `-`
- Payload begins after a line that is exactly: `--`

Key styles:

- Header: `KEY=value`
- Block metadata / payload: `KEY: value` (value may be empty)

Useful regex anchors:

- `^NAME:\s*(.*)$`
- `^IDENT:\s*(.*)$`
- `^ID:\s*(.*)$`
- `^PARENTID:\s*(.*)$`
- `^HINT:\s*(.*)$`
- `^LISTITEM:\s*(.*)$`

Policy reminder: only rewrite keys from your allow-list (see 3.3).




1) Parse all blocks into a structured list (TYPE/ID/NAME/PARENTID + payload fields).
2) Apply deterministic rewrite rules:
   - spelling fixes
   - terminology standardisation (“drawer”, “sub-folder”, etc.)
   - length constraints (if ReBuild truncates beyond a limit)
3) Re-emit the file with:
   - identical ordering
   - identical IDs
   - only the intended fields changed (e.g., `HINT:` and `LISTITEM:`)

---

Menu payload keys (edit values only):

- `^MENUNAME:\s*(.*)$`
- `^ITEMNAME:\s*(.*)$`
- `^SUBITEMNAME:\s*(.*)$`
- `^COMMKEY:\s*(.*)$`

Separator detection helpers:

- `^ITEMNAME:\s*bar\d*$`  (if you standardise to lower-case)
- `^ITEMNAME:\s*bar$`



## 6. Notes / unknowns

- `TYPE` values map to ReBuild internal object classes (window, layout, gadget types, etc.). We can build a mapping over time by observing patterns, but it’s not strictly required for hint/list edits.
- The text-bearing key allow-list now lives in section 3.3. Expand it only when you’ve confirmed a key is used in your file and is safe to edit.

