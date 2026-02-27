# ReBuild UI Text Scan and Polish Rules (iTidy) - v2

This document defines the **text lint rules** used when reviewing a ReBuild `.reb` file for professional UI polish.
It is designed for repeatable use by AI agents and humans.

---

## 1. Scope: What Text Gets Scanned

Scan **user-facing strings** in these ReBuild fields:

- **Window titles**: `TITLE:`
- **Visible gadget text** (buttons, checkboxes, tabs, frames, static labels):
  - Typically `LABEL:` (and sometimes gadget-specific text fields, depending on the object)
- **Menus**:
  - Menu titles and items: `ITEMNAME:`
- **Chooser / list entries**:
  - Entries displayed to the user: `LISTITEM:`
- **Hints / tooltips**:
  - Tooltip strings: `HINT:`

Do not modify:
- Internal IDs and identifiers (for example `GID_*`, `GA_ID`, object IDs, layout names, `deficons_text_*`)
- Separators (often `bar*`)
- Paths, command strings, ARexx commands, tooltypes content
- Protocol or file-format strings where casing is significant, unless explicitly requested

---

## 2. Character Set Safety (Amiga ASCII Only)

All UI text must be representable in **Amiga-friendly ASCII (ISO-8859-1 / Latin-1)**.

Reject or replace any characters that are likely to break on Amiga tools, fonts, or editors, including:
- Emoji and emoticons outside Latin-1
- Typographic punctuation: curly quotes “ ” ‘ ’, bullet •, en/em dashes, ellipsis character …
- Arrows and symbols: → ← ⇢ ↔, checkmarks ✓, crossmarks ✗, triangles, etc.

Allowed replacements:
- Use straight quotes: `"` and `'`
- Use three dots: `...` (not the single ellipsis glyph)
- Use hyphen-minus: `-` (not en dash or em dash)

If in doubt, keep to plain ASCII characters 32 to 126, plus standard Latin-1 letters when needed.

---

## 3. Field Length Limits (Prevent Truncation)

ReBuild can truncate strings on save. Enforce these limits proactively:

- `HINT:` maximum **194 characters per line**
- Most other text fields (including `LABEL:`, `ITEMNAME:`, `LISTITEM:`) treat **79 characters** as a safe maximum

Rules:
- If a hint needs more than 194 characters, split into multiple `HINT:` lines with sensible breaks.
- Avoid labels that approach the limit if the gadget is narrow (tabs and buttons are the usual offenders).
- If a string appears truncated (missing ending or first character), rewrite it shorter and re-check.

---

## 4. Spelling and Typo Corrections

### 4.1 Spellcheck baseline
- Correct obvious spelling mistakes (default to UK English).
- Do not "correct" Amiga terms into something else.

Common terms to preserve exactly:
- `WHDLoad`, `DefIcons`, `Workbench`, `Amiga`
- `ReAction` vs `Reaction`: pick one project spelling and enforce it everywhere
- File formats and acronyms: `OK`, `CPU`, `RTG`, `JPEG`, `PNG`, `IFF`, `ILBM`

### 4.2 Whitespace and duplication
- Remove doubled spaces.
- Remove leading and trailing spaces.
- Fix repeated punctuation such as `,,` or `..` (see ellipsis rules below).

---

## 5. Capitalisation Rules

### 5.1 Labels, menu items, list items, and window titles
Use **Standard Title Case** for:
- `TITLE:`
- `LABEL:`
- `ITEMNAME:`
- `LISTITEM:`

Standard Title Case rules:
- Capitalise the first letter of each significant word.
- Keep short connector words lowercase unless they are the first or last word:
  - `and`, `or`, `to`, `for`, `of`, `in`, `on`, `with`, `by`, `as`, `at`, `from`, `into`, `per`, `if`
- Preserve established casing for acronyms and product names:
  - `WHDLoad`, `DefIcons`, `Workbench`, `JPEG`, `PNG`, `IFF`, `ILBM`, `OK`

Examples:
- `Smart (Create Folder if It Has Icons Inside)`
- `Export List of Tools`
- `Sort By`
- `Columns & Groups`

### 5.2 Hyphenation and compound terms
- Keep correct casing around hyphens: `Floyd-Steinberg`, `Auto-Calc`.
- If the project prefers no hyphen for a term, standardise it consistently.

### 5.3 Parentheses
- Apply the same casing rules inside parentheses as outside:
  - Prefer `JPEG (Slow)` over `JPEG (slow)` when using Title Case.
- Keep parentheses content short where possible.

---

## 6. Button Text Rules (Professional UI Feel)

Buttons should be easy to scan and feel "native":

- Use **imperative, verb-first** phrasing:
  - Prefer `Restore Backups...` over `Backups Restore...`
  - Prefer `View Folders` over `Folders View`
- Avoid filler words: do not use `Now`, `Here`, `Please`.
- Avoid ambiguous verbs:
  - Prefer `Delete Backup Set...` over `Delete...` if multiple delete concepts exist.
- Keep standard dialog buttons as-is:
  - `OK`, `Cancel`, `Help`, `Close`
- Do not mix `Close` and `Cancel` unless they do different things.

---

## 7. Checkbox and Option Text Rules

### 7.1 Checkboxes
Checkbox text should describe a **state** and remain unambiguous:

Preferred patterns:
- `Enable X`
- `Show X`
- `Include X`
- `Skip X`
- `Back Up X`

Avoid:
- Double negatives (`Do Not Skip...`)
- Checkboxes that read like commands (`Do X`)

### 7.2 Chooser and list items (`LISTITEM:`)
All items within a chooser/list should share a **parallel grammatical shape**.

Good:
- `Folders First`, `Files First`, `Mixed`, `Grouped by Type`

Avoid mixing:
- Verb phrases with noun phrases in the same list.

---

## 8. Ellipsis Rules (`...`)

Ellipsis indicates **more interaction is coming**.

Add `...` only when the control:
- Opens another window or sub-panel
- Opens a requester (file or directory)
- Opens a settings dialog
- Requires additional user choices before the action completes

Examples:
- `Save As...`
- `Restore Backups...`
- `DefIcons Categories...`

Do not add `...` for actions that run immediately:
- `Refresh` (if it truly just refreshes)
- `Reset`
- `Defaults` (if it applies instantly)
- `OK`, `Cancel`

Do not use the single ellipsis glyph `…`. Always use `...`.

---

## 9. Punctuation and Symbols

### 9.1 Colons
- Keep colons where they express "label: value".
- No space before the colon:
  - `Folder:` not `Folder :`

### 9.2 Ampersands
- Use `&` consistently where space is tight (tabs/groups).
- Do not mix `and` and `&` randomly in the same set of related labels.

### 9.3 Quoting UI labels
- When referencing UI labels inside hints, use double quotes:
  - Example: enable `"Include Subfolders"`.

---

## 10. Cross-Surface Consistency Rules

If a phrase appears in multiple places (button, menu item, window title, hint), it must match.

Example pattern:
- Window title: `iTidy - Restore Backups`
- Menu item: `Restore Backups...`
- Button: `Restore Backups...`

Allow differences only for:
- Ellipsis (only where applicable)
- Optional leading product prefix in titles (`iTidy - ...`)

---

## 11. Terminology Dictionary (Project Canon)

Maintain and enforce a short canonical list of preferred spellings and casing, for example:

- `Workbench`
- `ReAction` (or `Reaction`, choose one)
- `Default Tool`
- `Back Up` (verb) vs `Backup` (noun), choose a rule and enforce it
- `Subfolder` (or `Sub-folder`, choose one)
- `WHDLoad`
- `DefIcons`

Rule:
- Once a canonical term exists, do not allow variants anywhere in `TITLE:`, `LABEL:`, `ITEMNAME:`, `LISTITEM:`, or `HINT:`.

---

## 12. Hints and Tooltips (`HINT:`)

Hints are treated differently from labels and menu items.

### 12.1 Style
- Use **sentence case** (not Title Case).
- End each hint line with a period.
- Prefer one behaviour per sentence.
- Start with what it does, then conditions, then side effects.

### 12.2 Length and wrapping
- Keep each `HINT:` line <= 194 characters.
- Split long hints across multiple `HINT:` lines.
- Avoid unnecessary detail that does not affect the user.

### 12.3 Terminology and quoting
- Preserve product/feature names and acronyms exactly: `iTidy`, `WHDLoad`, `DefIcons`, `JPEG`, etc.
- Use consistent terminology (drawer vs folder, whichever the project standard is).
- Use double quotes for UI labels mentioned in the hint.

---

## 13. Recommended Workflow for Future Passes

1. Export or copy the latest `.reb`.
2. Scan and normalise, in this order:
   - `TITLE:`
   - Visible gadget labels (`LABEL:` and gadget text)
   - `ITEMNAME:`
   - `LISTITEM:`
   - `HINT:`
3. Apply rules:
   - Character set safety (Amiga ASCII)
   - Spelling and whitespace cleanup
   - Standard Title Case (except hints)
   - Ellipsis policy
   - Consistency against terminology dictionary
   - Field length limits
4. Re-open the `.reb` in ReBuild and visually verify key windows (tabs and narrow buttons).
5. Save as a new revision and keep the previous `.reb` for rollback.

---

## 14. Quick Decision Table (Ellipsis)

| UI Text Type | Example | Add `...`? | Reason |
|---|---|:---:|---|
| Opens file requester | `Save As` | Yes | More choices needed |
| Opens settings window | `Icon Creation` | Yes | More UI follows |
| Runs immediately | `Defaults` | No | Action completes now |
| Confirms or aborts | `OK` / `Cancel` | No | Standard buttons |

