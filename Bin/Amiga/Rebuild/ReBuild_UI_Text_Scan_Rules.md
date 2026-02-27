# ReBuild UI Text Scan and Polish Rules (iTidy)

This document describes the **formatting checks and corrections** applied when reviewing a ReBuild `.reb` file for UI text polish.  
Use it as a repeatable checklist for future passes.

---

## 1. Scope: What Text Gets Scanned

The scan targets **user-facing strings** found in common ReBuild fields, including:

- **Window titles** (e.g. `TITLE:`)
- **Gadget labels / captions** (fields that render visible text on buttons, checkboxes, tabs, frames, text labels, etc.)
  - Commonly: `LABEL:` and/or `NAME:` depending on gadget type
- **Menus**
  - Menu items and titles: `ITEMNAME:`
- **Chooser / list entries**
  - Entries shown to users in list/chooser gadgets: `LISTITEM:`
- **Hints / tooltips**
  - Tooltip strings for gadgets: `HINT:`

Notes:
- Internal IDs (e.g. `GID_*`, `GA_ID`, object IDs, layout names) are **not** modified.
- Separator items (often named `bar*`) are **not** modified.

---

## 2. Spelling and Typo Corrections

### 2.1 Spellcheck
- Correct obvious spelling mistakes (UK English unless your project explicitly prefers US English).
- Common Amiga/UI words to treat carefully (do not “fix” into something else):
  - `WHDLoad`, `DefIcons`, `Workbench`, `Amiga`, `Reaction`/`ReAction` (match your project’s chosen spelling)

### 2.2 Truncation artifacts
- Fix strings that were truncated or accidentally missing the first character(s) (common if edited in tools with length limits).

### 2.3 Accidental repeats / punctuation slips
- Fix `..` to `...` where appropriate.
- Remove doubled spaces.
- Remove stray leading/trailing spaces.

---

## 3. Capitalisation Rules (Standard Title Case)

For this polish pass, **Standard Title Case** is enforced for user-facing text (similar to most apps).

### 3.1 Standard Title Case baseline
- Capitalise the first letter of each significant word.
- Keep short connector words lowercase when they are not the first or last word (e.g. `if`, `and`, `or`, `to`, `for`, `of`).
- Example: `Smart (Create Folder if It Has Icons Inside)`.

### 3.2 Words typically left lowercase (unless first/last)
Common style choices (apply consistently):
- `and`, `or`, `to`, `for`, `of`, `in`, `on`, `with`, `by`, `as`, `at`, `from`, `into`, `per`, `if`
- Example: `Columns & Groups`, `Sort By`, `Export List of Tools`

### 3.3 Acronyms and product names
- Preserve established casing:
  - `OK`, `CPU`, `RTG`, `JPEG`, `PNG`, `IFF`, `ILBM`, `WHDLoad`, `DefIcons`
- Do not “Title Case” these into `Jpeg` etc.

### 3.4 Hyphenation and compound terms
- Keep correct casing around hyphens:
  - `Floyd-Steinberg`, `Auto-Calc`
- If the project prefers no hyphen, standardise, but do so consistently.

---

## 4. Ellipsis Rules (`...`) and When to Add It

Ellipsis indicates **more interaction is coming** (dialog, requester, wizard, or another window).

Add `...` when the control typically:
- Opens a **new window** / **sub-panel**
- Opens a **file requester** / **directory chooser**
- Opens a **settings dialog**
- Triggers an action that requires **additional user choices** before it completes

Examples:
- `Save As...`
- `Restore Backups...`
- `Show Default Tools...`
- `DefIcons Categories...`

Do **not** add `...` for immediate actions that complete immediately and do not require further input, such as:
- `Refresh` (if it truly just refreshes)
- `Reset`
- `Defaults` (if it applies defaults instantly)
- `OK`, `Cancel`

---

## 5. Punctuation and Symbols

### 5.1 Colons
- Keep colons where they communicate “label: value”.
- If a label ends with a colon, keep spacing consistent:
  - `Folder:` not `Folder :`

### 5.2 Ampersands
- Use `&` consistently in tab/group names if already used in the UI:
  - `Columns & Groups`
- Don’t mix `and` and `&` randomly in the same window/tab set.

### 5.3 Parentheses
- Standardise capitalisation inside parentheses:
  - `JPEG (Slow)` vs `JPEG (slow)`  
  Choose one; with Standard Title Case, `JPEG (Slow)` is typically preferred.

---

## 6. Consistency Rules (Terminology)

Standardise recurring terms across the whole UI:

- `Default Tool` (consistent)
- `Backup` / `Backups` (plural when it leads to a list/manager)
- `Folder View` (if it is a named feature/window)
- `Icon Creation`, `Advanced Settings`, etc.

If a phrase appears in multiple places (button + menu + title), they should match exactly.

---

## 7. What Was Not Changed

The scan intentionally avoids editing:

- Gadget IDs, internal names, or constants (`GID_*`, `deficons_text_*`, etc.)
- File paths, command strings, ARexx commands, tooltypes content
- Any strings that are part of technical protocols where casing is significant, unless you explicitly want them normalised

---

## 8. Recommended Workflow for Future Passes

1. **Export** or copy the latest `.reb`.
2. Run the scan over:
   - Window titles
   - Visible gadget labels
   - `ITEMNAME:` entries
   - `LISTITEM:` entries
3. Apply the rules above:
   - Fix spelling/typos
   - Enforce Standard Title Case (lowercase short connector words like `if` when not first/last)
   - Apply `...` only where it opens another dialog/window/requester
4. Re-open the `.reb` in ReBuild and visually verify key windows (tabs especially).
5. Spot-check for length limits (ReBuild sometimes truncates long labels).
6. Save as a new revision (keep the previous `.reb` for rollback).

---

## 9. Quick “Decision Table” (Ellipsis)

| UI Text Type | Example | Add `...`? | Reason |
|---|---:|:---:|---|
| Opens file requester | `Save As` | Yes | More choices needed |
| Opens settings window | `Icon Creation` | Yes | More UI follows |
| Runs immediately | `Defaults` | No | Action completes now |
| Confirms/aborts | `OK` / `Cancel` | No | Standard buttons |

---

## 10. Optional Enhancements (If You Want Even More Polish)

- Enforce maximum label widths per gadget type (avoid truncation).
- Create a project “terms dictionary” (preferred spellings for Amiga-specific words).
- Add automated linting via a small script that extracts strings and runs checks.


---

## 11. Hints / Tooltips (HINT:)

Hints are treated differently from labels and menu items. They should read naturally and explain behaviour clearly.

### 11.1 Capitalisation
- Use **sentence case** for hints (not Title Case).
- Preserve product/feature names and acronyms as-is (e.g. `iTidy`, `WHDLoad`, `DefIcons`, `JPEG`).

### 11.2 Punctuation and readability
- End hints with a period.
- Remove double spaces and stray whitespace.
- Prefer double quotes for UI labels referenced inside hints:
  - Example: enable `"Include Subfolders"`.

### 11.3 Clarity and consistency
- Fix typos and incorrect terms (e.g. `drawer icon` vs `draw icon`).
- Prefer consistent terminology (`subfolder` vs `sub folder`).
- Avoid overly long hints where ReBuild truncates text.

