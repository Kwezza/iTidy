# iTidy Manual Generation Process

This document describes the process used to create `docs/manual/iTidy.md` (the v2.0 user manual), so it can be repeated if the manual needs to be regenerated after significant changes.

---

## Overview

The manual was built by analysing every GUI window's source code, writing per-window working notes, then distilling those notes into a single user-facing manual.

---

## Step 1: Analyse Each Window's Source Code

Each GUI window source file was read in full and analysed for:

- Window title, how it's opened, and whether it's modal
- Every gadget (buttons, choosers, checkboxes, listbrowsers, string fields, etc.)
- All menu items with keyboard shortcuts
- Default values for every setting
- Button enable/disable logic (when are buttons greyed out)
- All requesters shown to the user (confirmations, warnings, errors)
- File paths used (backup locations, prefs files, cache files)
- Gadget dependencies (e.g. one chooser disabling another)
- ToolTypes read at startup

### Windows analysed (13 source files):

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

Each analysis was written up as a markdown file in `docs/manual/working_notes/`. These notes are developer-facing and contain implementation details not shown to users (button state matrices, requester text, internal file formats, etc.).

The working notes are kept as a reference and are NOT deleted after the manual is generated.

---

## Step 3: Read the Existing Manual for Style

The previous manual (`docs/manual/iTidy.md` v1.0, ~400 lines) was read to capture:

- Writing tone (first-person, conversational, informal but technical)
- Structure (TOC with anchor links, `---` separators, bullet-point gadget descriptions)
- What to include vs omit (user-facing behaviour, not implementation details)
- Tip formatting (bold lead text)

---

## Step 4: Distill Working Notes into Final Manual

All 13 working notes were synthesised into a single `docs/manual/iTidy.md` with these guidelines:

### Writing Style

The manual is written in a specific tone and style that should be maintained across regenerations:

- **First-person, conversational tone.** The author speaks directly ("I wrote after getting fed up with..."). Not corporate or formal — it reads like one Amiga user explaining their tool to another.
- **Informal but technically precise.** Casual phrasing ("it's fine to leave iTidy running unattended") paired with exact details (ranges, defaults, paths).
- **British English spelling.** Use "colour" not "color", "organised" not "organized", "centres" not "centers", etc.
- **Plain ASCII only.** No Unicode characters anywhere — the manual is converted to AmigaGuide which only supports ASCII. Use `->` not `→`, `--` not `—`, `*` not `•`.
- **Bold for gadget/setting names** when first introduced in a section (e.g. **Include Subfolders**).
- **Italics for option values** within lists (e.g. *Folders First*, *Wide (2.0)*).
- **"(Default)"** marked in bold after the default option in every list.
- **Defaults stated explicitly** for every setting, either inline or in a summary table.
- **Short paragraphs.** Most descriptions are 1-3 sentences. Longer explanations are broken into bullets.
- **"Note:" and "Warning:" prefixes** in bold for important caveats, not boxed or styled differently.
- **"Tip:" prefix** in bold for helpful suggestions.
- **Markdown tables** for menu items (with Shortcut and Description columns) and for option lists where comparison is useful.
- **Bullet lists** for gadget descriptions within a window section.
- **"What to do:" pattern** in troubleshooting — bold header stating the problem, then a "What to do:" list of solutions.
- **No jargon without context.** AmigaOS terms like "Default Tool", "drawer", and "BPTR" are explained on first use where a user might not know them. Developer-only terms are kept out entirely.
- **Section separators** (`---`) between major top-level sections, not between subsections.
- **TOC uses anchor links** matching the markdown header IDs.

### What to include:
- Every user-visible setting with its options and default value
- Menu items with keyboard shortcuts
- How to access each window (navigation path)
- Warnings and important notes (e.g. permanent changes, disk writes)
- Tips and troubleshooting entries
- ToolTypes reference
- Requirements (OS version, CPU, RAM, LhA)

### What to omit:
- Internal button state logic tables (simplified to prose)
- Exact requester body text (described in general terms)
- Window pixel dimensions and coordinates
- Source file names and line numbers
- Implementation details (binary formats, ARexx commands, internal caching)
- Windows not currently in the build (Backdrop Cleaner — noted only in working notes)
- Progress windows (internal UI, not user-configured)
- Simple Progress Window (unused/legacy)

### Structure used:
1. Requirements
2. Introduction (what iTidy does, icon format support, safety disclaimer)
3. Getting Started (numbered quick-start steps)
4. Main Window (folder, options, buttons, menus, tips)
5. Advanced Settings (5 tabs, all gadgets with defaults)
6. Icon Creation Settings (3 tabs, format checkboxes, rendering options)
7. DefIcons Categories (type tree, default tools, important caveats)
8. Text Templates (master/custom system, ToolTypes reference table)
9. Exclude Paths (DEVICE: placeholder, default list, gadgets)
10. Default Tool Analysis (scanner workflow, menus, columns, technical notes)
11. Replacing Default Tools (batch/single, clearing, backups)
12. Restoring Default Tool Backups (sessions, changes panel)
13. Restoring Layout Backups (run list, 3-option restore, requirements)
14. Folder View (tree display, read-only)
15. ToolTypes (DEBUGLEVEL, LOADPREFS, PERFLOG)
16. Tips & Troubleshooting (general tips + specific issues)
17. Credits & Version Info

---

## Regenerating the Manual

To regenerate after code changes:

1. Identify which windows have changed (check `src/GUI/` diffs).
2. Re-analyse the changed source files and update the corresponding working notes in `docs/manual/working_notes/`.
3. Re-read all working notes and rewrite the affected sections in `docs/manual/iTidy.md`.
4. Check `src/version_info.h` for the current version number and update the manual header.
5. Verify the TOC anchor links still match the section headers.

For a full regeneration from scratch, repeat all steps above for every window.

---

## File Locations

| File | Purpose |
|------|---------|
| `docs/manual/iTidy.md` | The final user manual (this is what gets converted to AmigaGuide) |
| `docs/manual/working_notes/*.md` | Per-window developer analysis notes (13 files) |
| `docs/MANUAL_GENERATION_PROCESS.md` | This file |
| `src/version_info.h` | Version number source of truth |