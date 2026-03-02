# ReBuild UI Text Scan and Polish Rules (iTidy) - v2.2

This document defines the **text lint rules** used when reviewing a ReBuild `.reb` file for professional UI polish.
It is designed for repeatable use by AI agents and humans.

v2.2 incorporates evidence-backed Workbench/ReAction conventions (Workbench 3.2, with support from 3.5/3.9) and keeps ReBuild-specific heuristics clearly separated.

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

## 2. Character Set Safety (Classic Amiga Safe)

All UI text must be safe on classic Workbench systems that are not Unicode-first.

Rules:
- Prefer plain ASCII punctuation for maximum compatibility.
- Avoid smart quotes, typographic dashes, and the single ellipsis glyph.
- Use `...` for ellipsis.
- Use `-` for dashes.
- Use straight quotes: `"` and `'`.

When you must use extended Latin characters, keep them consistent with the active locale and catalog codeset.

---

## 3. Text Length and Layout Risk

### 3.1 Evidence-backed label brevity rule (most important)
Visible UI labels should be **terse**:
- Prefer **1 to 3 words** for most labels and menu items.
- Prefer **1 to 2 words** for tab captions and column titles.

Rationale:
- Workbench UI guidance recommends terse labels.
- ReAction can clip button text and ClickTab can hide tabs when captions do not fit.

### 3.2 Window title sizing baseline
Window titles must remain short enough to support classic layouts:
- Use **640x200 with Topaz 8** as a conservative baseline when deciding title length.
- Avoid sentence-style titles that force wide minimum widths.

### 3.3 ReBuild-specific heuristics (use as conservative lint, not OS guarantees)
ReBuild can truncate strings on save in some situations, and narrow gadgets can truncate visually.

Heuristic limits (adjust if you observe better limits in your setup):
- `HINT:` keep each line <= **194 characters**.
- For narrow gadgets (tabs, small buttons), treat **~79 characters** as a hard warning threshold even if the control might technically allow more.

If a string appears truncated after reload, rewrite it shorter and re-check in ReBuild.

---

## 4. Spelling and Typo Corrections

- Correct obvious spelling mistakes (default to UK English).
- Do not "correct" Amiga terms into something else.

Common terms to preserve exactly:
- `WHDLoad`, `DefIcons`, `Workbench`, `Amiga`
- `ReAction` (project spelling)
- `OK`, `CPU`, `RTG`, `JPEG`, `PNG`, `IFF`, `ILBM`, `LhA`
- `Tool Type(s)`, `Default Tool(s)`

Whitespace rules:
- Remove doubled spaces.
- Remove leading and trailing spaces.
- Fix repeated punctuation such as `,,` or `..` (except `...`).

---

## 5. Capitalisation Rules

### 5.1 Titles, labels, menu items, list items
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
  - `WHDLoad`, `DefIcons`, `Workbench`, `OK`, `LhA`

### 5.2 Hints
Hints use **sentence case** (see section 12).

---

## 6. Button Text Rules (Action-first, non-technical)

Buttons should be easy to scan and feel Workbench-native:

- Use **imperative, verb-first** phrasing.
- Prefer **1 to 3 words**.
- Use user-friendly wording:
  - Prefer `Stop` over `Abort`.
- Use `Cancel` only when the action truly backs out leaving prior state unchanged.

Ellipsis:
- Add `...` only if the button opens another window/requester or needs further choices.

---

## 7. Checkbox, Toggle, and Option Text Rules

### 7.1 Checkboxes
Checkbox text should describe a **state**:

Preferred patterns:
- `Enable X`
- `Show X`
- `Include X`
- `Skip X`
- `Back Up X` (verb form)

Avoid:
- Double negatives
- Checkboxes that read like commands

### 7.2 Chooser and list entries (`LISTITEM:`)
- Keep entries terse and parallel.
- Prefer 1 to 3 words per entry.
- Avoid mixing grammatical shapes in the same list.

---

## 8. Menus (ITEMNAME) and Standard Patterns

### 8.1 Menu item phrasing
- Menu items that perform actions should be labeled as actions (verb-first).
- Prefer 1 to 3 words.
- Avoid repeating the menu title in each menu item unless clarity would suffer.

### 8.2 Ellipsis in menus
Use `...` only when the action opens a window/requester or requires further user input.

### 8.3 Toggle menu items
For toggles presented as checkmark/indent items:
- Consider a trailing `?` where it improves clarity (example: `Create Icons?`).
- Do not implement toggles as `On/Off` submenus.

---

## 9. Keyboard Equivalents and Underlines

If your UI uses underlined shortcut letters:
- Underline a logical letter from the label, preferably the first letter.
- Avoid awkward late-label underlines that are hard to spot.

Requester keyboard behaviour:
- Avoid keyboard equivalents in asynchronous requesters.
- Do not map Return to `OK` by default in requesters.

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

## 11. Terminology Dictionary (Workbench Canon)

These terms are canonical for Workbench-facing UI:

- `Drawer` (preferred for Workbench UI; corresponds to an AmigaDOS directory)
- `Requester` (preferred over "dialog" in UI text)
- `Snapshot` (save window and icon positions)
- `Default Tool` / `Default Tools` (Workbench icon concept)
- `Tool Type` / `Tool Types` (Workbench icon concept)

Rules:
- Do not mix `folder` and `drawer` within the same window.
- Use `directory` only when explicitly discussing Shell/AmigaDOS concepts.

Backup wording:
- Verb: `Back Up`
- Noun/adjective: `Backup`, `Backups`

---

## 12. Hints and Tooltips (`HINT:`)

### 12.1 Workbench 3.2 reality
On Workbench 3.2, tooltips map to window.class contextual help and per-gadget help strings.
Treat `HINT:` text as a **single help bubble string**.

### 12.2 Style
- Sentence case.
- Prefer short, factual, context-specific text.
- Write so it remains clear **as a single line** (do not rely on line breaks).
- End each hint line with a period.

### 12.3 Content
- Start with what it does.
- Add conditions only if they affect behaviour.
- Avoid paragraphs and long lists in hints.
- When referencing UI labels inside the hint, use double quotes.

### 12.4 Terminology
- Preserve product and acronym casing exactly.
- Use canonical Workbench terminology (drawer, requester, Default Tool, Tool Type, snapshot).

---

## 13. Catalog and Localization Readiness (Recommended)

Even if the UI is English-only today, write strings as if they may be cataloged later:

- Avoid building sentences by concatenating fragments.
- Prefer full, translatable format strings with placeholders.
- Do not hard-code assumptions about word order.
- Treat keyboard shortcuts and underlines as locale-sensitive.
- Prefer impersonal message style for status and error text (avoid "I" and "you" where practical).

---

## 14. Recommended Workflow for Future Passes

1. Export or copy the latest `.reb`.
2. Scan and normalise, in this order:
   - `TITLE:`
   - Visible gadget labels (`LABEL:` and gadget text)
   - `ITEMNAME:`
   - `LISTITEM:`
   - `HINT:`
3. Apply rules:
   - Character set safety
   - Spelling and whitespace cleanup
   - Standard Title Case (except hints)
   - Label brevity (1 to 3 words)
   - Ellipsis policy
   - Consistency against terminology dictionary
4. Re-open the `.reb` in ReBuild and visually verify key windows (tabs and narrow buttons).
5. Save as a new revision and keep the previous `.reb` for rollback.
