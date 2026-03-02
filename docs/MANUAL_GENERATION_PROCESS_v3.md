# iTidy Manual Generation Process

This document describes the repeatable process used to create `docs/manual/iTidy.md` (the iTidy v2 user manual), so it can be regenerated after GUI changes (labels, hints, menus, new windows, etc.).

---

## Overview

The manual is built by analysing every GUI window's source code, writing per-window working notes, then distilling those notes into a single user-facing manual.

---

## Step 1: Analyse Each Window's Source Code

Each GUI window source file is read in full and analysed for:

- Window title, how it's opened, and whether it's modal
- Every gadget (buttons, choosers, checkboxes, listbrowsers, string fields, etc.)
- All menu items (including keyboard shortcuts)
- Default values for every setting
- Button enable/disable logic (when are buttons greyed out)
- All requesters shown to the user (confirmations, warnings, errors)
- File paths used (backup locations, prefs files, cache files)
- Gadget dependencies (e.g. one chooser disabling another)
- ToolTypes read at startup

### Windows analysed (13 source files)

| # | Working Note | Source File(s) |
|---|-------------|----------------|
| 1 | `main_window.md` | `src/GUI/main_window.c`, `src/GUI/main_window.h` |
| 2 | `advanced_window.md` | `src/GUI/advanced_window.c` |
| 3 | `deficons_creation_window.md` | `src/GUI/deficons/deficons_creation_window.c` |
| 4 | `deficons_settings_window.md` | `src/GUI/deficons/deficons_settings_window.c` |
| 5 | `text_templates_window.md` | `src/GUI/deficons/text_templates_window.c` |
| 6 | `exclude_paths_window.md` | `src/GUI/exclude_paths_window.c` |
| 7 | `tool_cache_window.md` | `src/GUI/DefaultTools/tool_cache_window.c` |
| 8 | `default_tool_update_window.md` | `src/GUI/DefaultTools/default_tool_update_window_reaction.c` |
| 9 | `default_tool_restore_window.md` | `src/GUI/DefaultTools/default_tool_restore_window.c` |
| 10 | `restore_window.md` | `src/GUI/RestoreBackups/restore_window.c` |
| 11 | `folder_view_window.md` | `src/GUI/RestoreBackups/folder_view_window.c` |
| 12 | `backdrop_window.md` | `src/GUI/backdrop_window.c` (temporarily removed from build) |
| 13 | `progress_windows.md` | `src/GUI/progress_window.c`, `src/GUI/recursive_progress_window.c`, `src/GUI/simple_progress_window.c` |

---

## Step 2: Write Per-Window Working Notes

Each analysis is written up as a markdown file in `docs/manual/working_notes/`. These notes are developer-facing and can contain implementation details not shown to users (button state matrices, requester text, internal file formats, etc.).

The working notes are kept as a reference and are NOT deleted after the manual is generated.

---

## Step 3: Read the Existing Manual for Style

Before rewriting the manual, read the existing `docs/manual/iTidy.md` to preserve:

- Structure (TOC with anchor links, `---` separators, bullet-point gadget descriptions)
- Formatting conventions (bold gadget names, italics for option values, defaults marked clearly)
- What to include vs omit (user-visible behaviour, not implementation detail)

### Introduction guidance (keep it short and non-persuasive)

The intro can include a brief "why this exists" note, but keep it to a single short paragraph. Avoid sales language and avoid extended justification.

---

## Step 4: Distill Working Notes into the Final Manual

All working notes are synthesised into a single `docs/manual/iTidy.md` using the rules below.

### Writing Style (Core Rules)

The manual is written in a consistent tone and layout. Keep these rules across regenerations:

- **First-person, conversational tone.** It reads like one Amiga user explaining the tool to another.
- **Informal but technically precise.** Casual phrasing is fine, but keep details exact (ranges, defaults, paths).
- **UK English spelling throughout.** Use "colour" not "color", "organise" not "organize", "centre" not "center", etc.
- **Plain ASCII only.** No Unicode characters anywhere. The manual is converted to AmigaGuide which only supports ASCII.
  - Use `->` not `→`, `--` not `—`, `*` not `•`.
- **Bold for gadget/setting names** when first introduced in a section (e.g. **Include Subfolders**).
- **Italics for option values** within lists (e.g. *Folders First*, *Wide (2.0)*).
- **"(Default)"** marked in bold after the default option in every list.
- **Defaults stated explicitly** for every setting, either inline or in a summary table.
- **Short paragraphs.** Most descriptions are 1 to 3 sentences. Longer explanations should be bullets.
- **"Note:" and "Warning:" prefixes** in bold for important caveats (no callout boxes).
- **"Tip:" prefix** in bold for helpful suggestions.
- **Markdown tables** for menu items (Shortcut and Description columns), and for option lists where comparison is useful.
- **Bullet lists** for gadget descriptions within a window section.
- **Troubleshooting format.** Use a bold problem header, then a "What to do:" list of actions.
- **No jargon without context.** Explain AmigaOS terms (Default Tool, drawer, ToolTypes) on first use where needed.
- **Section separators.** Use `---` between major top-level sections, not between subsections.
- **TOC uses anchor links** matching the markdown header IDs.

### Human-Like Manual Writing (Avoid AI Tells)

The manual should read like it was written by a real person who has used the software. Avoid common "AI writing" patterns:

- **Be concrete, not generic.** Prefer exact menu paths, option names, defaults, file names, and expected outcomes over broad statements.
- **Avoid filler signposting.** Do not use blog-style transitions like "In summary", "Let's dive in", "Key takeaways", or "In conclusion" unless genuinely needed.
- **Do not restate the same point.** If something must be repeated, do it intentionally (for example, a short recap or checklist), otherwise cut.
- **Vary sentence rhythm.** Mix short direct sentences with longer explanatory ones. Avoid the same sentence shape repeated line after line.
- **Use cautious language only when warranted.** Avoid excessive hedging. Use "may/can/depends" when behaviour varies by Workbench setup or configuration, otherwise state the behaviour directly.
- **Prefer plain verbs.** Avoid over-polished phrasing. Keep instructions actionable: "Click", "Select", "Set", "Run", "Check".
- **Keep tone matter-of-fact.** No hype, no "sales" phrasing. Explain behaviour and trade-offs.

### Tone Guardrails (Anti-Marketing Rules)

iTidy is a hobby tool, not a commercial product. The manual should be practical and neutral, not persuasive.

- **Describe behaviour, not hype.** Explain what iTidy does and how to use it, without selling it.
- **Avoid pressure wording.** Do not tell the reader what they "need" or "must" do unless it is required for correctness or safety.
  - Avoid: "must-have", "you need this", "game changer", "best", "perfect", "ultimate", "supercharge", "next level".
  - Prefer: "If you want to...", "This option is useful when...", "You can...", "This may help if...".
- **No benefit-claim fluff.** Do not claim time savings or outcomes unless you can state the exact observable behaviour.
  - Good: "Creates backups of .info files before changes (if enabled)."
  - Avoid: "Saves you loads of time" or "makes everything effortless".
- **Be cautious around variability.** Use "may", "can", and "depends" when behaviour varies by Workbench setup, icon set, or DefIcons config.
- **Keep first-person origin story minimal.** One short paragraph is fine. The rest should be straightforward instructions and reference.

### Quick Tone Lint (Final Pass)

Do a last pass over the regenerated manual and search for these phrases/words. Replace with neutral wording or remove:

- `must-have`
- `you need`
- `game changer`
- `ultimate`
- `perfect`
- `best`
- `incredible`
- `powerful` (often used as hype; keep only if you mean "has these capabilities" and can justify it)
- `supercharge`
- `next level`
- `effortless`
- `save(s) you time` (unless you can quantify or describe it precisely)

---

## What to Include

- Every user-visible setting with its options and default value
- Menu items with keyboard shortcuts
- How to access each window (navigation path)
- Warnings and important notes (e.g. permanent changes, disk writes)
- Tips and troubleshooting entries
- ToolTypes reference
- Requirements (OS version, CPU, RAM, LhA)

---

## What to Omit

- Internal button state logic tables (simplify to prose)
- Exact requester body text (describe in general terms)
- Window pixel dimensions and coordinates
- Source file names and line numbers
- Implementation details (binary formats, ARexx commands, internal caching)
- Windows not currently in the build (Backdrop Cleaner, etc.) - keep in working notes only
- Progress windows (internal UI, not user-configured)
- Simple Progress Window (unused/legacy)

---

## Structure Used (Suggested Chapter Order)

1. Requirements
2. Introduction (scope + safety disclaimer, kept short)
3. Getting Started (numbered quick-start steps)
4. Main Window (folder, options, buttons, menus, tips)
5. Advanced Settings (tabs, all gadgets with defaults)
6. Icon Creation Settings (tabs, formats, rendering options)
7. DefIcons Categories (types, default tools, important caveats)
8. Text Templates (master/custom system, ToolTypes reference table)
9. Exclude Paths (DEVICE: placeholder, default list, gadgets)
10. Default Tool Analysis (scanner workflow, menus, columns)
11. Replacing Default Tools (batch/single, clearing, backups)
12. Restoring Default Tool Backups (sessions, changes panel)
13. Restoring Layout Backups (run list, restore options, requirements)
14. Folder View (tree display, read-only)
15. ToolTypes (DEBUGLEVEL, LOADPREFS, PERFLOG)
16. Tips and Troubleshooting
17. Credits and Version Info

---

## Regenerating the Manual

To regenerate after code changes:

1. Identify which windows have changed (check diffs in `src/GUI/`).
2. Re-analyse the changed source files and update the corresponding working notes in `docs/manual/working_notes/`.
3. Re-read the working notes and rewrite the affected sections in `docs/manual/iTidy.md`.
4. Check `src/version_info.h` for the current version number and update the manual header.
5. Verify the TOC anchor links still match the section headers.
6. Run the Quick Tone Lint pass (see above).

For a full regeneration from scratch, repeat all steps above for every window.

---

## File Locations

| File | Purpose |
|------|---------|
| `docs/manual/iTidy.md` | The final user manual (this is what gets converted to AmigaGuide) |
| `docs/manual/working_notes/*.md` | Per-window developer analysis notes |
| `docs/MANUAL_GENERATION_PROCESS.md` | This file (process and style rules) |
| `src/version_info.h` | Version number source of truth |
