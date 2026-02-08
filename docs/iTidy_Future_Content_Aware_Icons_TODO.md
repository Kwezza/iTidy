# iTidy – Future Content-Aware Icon Plans (To‑Do)

This document tracks **future / optional enhancements** related to DefIcons and content‑aware icon generation.
All items here are **non‑destructive** by design and only apply when **no existing .info file is present**, unless stated otherwise.

---

## 1. Core Safeguards (Foundation)

- [ ] Enforce global rule: *Never overwrite existing user icons*
- [ ] Treat only **missing .info files** as eligible for generated icons
- [ ] Stamp all generated icons with identifying ToolTypes:
  - `ITIDY_CREATED=1`
  - `ITIDY_KIND=<type>`
  - `ITIDY_SRC=<filename>`
  - `ITIDY_SRC_SIZE=<bytes>`
  - `ITIDY_SRC_DATE=<datestamp>`
- [ ] Ensure restore/rollback removes all generated icons cleanly
- [ ] Include generated icon counts in run summary

---

## 2. IFF Image Thumbnail Icons

- [ ] Generate thumbnail icons for IFF picture files (via DefIcons detection)
- [ ] Only create when no .info exists
- [ ] Implement screen‑depth aware rendering:
  - RTG / 15–24 bit: full‑colour thumbnail
  - 8‑bit: reduced palette + ordered dithering
  - 4‑bit or lower: heavy quantisation or fallback icon
- [ ] Detect and optionally support HAM images:
  - [ ] Fast sampling mode
  - [ ] Quality mode (optional, slower)
  - [ ] Bail‑out rules for HAM hires / interlace / large images
- [ ] Cache thumbnails and skip regeneration when source unchanged

---

## 3. Font Preview Icons

- [ ] Detect font files (bitmap and/or outline where supported)
- [ ] Render sample text (e.g. “Aa” or “Ag”) into icon image
- [ ] Fixed layout and background for legibility
- [ ] Depth‑aware rendering (mono → colour)
- [ ] Cache by font datestamp + settings
- [ ] Optional user‑configurable sample string

---

## 4. ASCII / Text Document Preview Icons

- [ ] Detect plain‑text files (printable ASCII / ISO‑8859‑1)
- [ ] Read only first N lines or first M bytes (e.g. 20 lines / 4–8 KB)
- [ ] Skip binary or oversized files
- [ ] Render preview into a “paper‑style” icon:
  - Fixed mono font
  - Black text on light background
  - Folded corner motif
- [ ] Ensure icon remains readable when selected/inverted
- [ ] Cache preview icons using source size + datestamp

---

## 5. Template Icon System (Advanced / Power Users)

- [ ] Support user‑supplied **template icons** for previews
- [ ] Define required ToolTypes for templates, e.g.:
  - `ITIDY_TEXT_AREA=x,y,w,h`
  - `ITIDY_LINE_HEIGHT=n`
  - `ITIDY_MAX_LINES=n`
- [ ] Draw generated content only inside defined safe area
- [ ] Provide built‑in default templates if none supplied
- [ ] Document template format for advanced users

---

## 6. Selected Image Support

- [ ] Generate both **Normal** and **Selected** icon images
- [ ] Default behaviour:
  - Same content
  - Selected image uses darker or inverted styling
- [ ] Ensure both images are pre‑rendered (no runtime logic)
- [ ] Optional / experimental:
  - Selected image shows alternate preview (e.g. “page 2”)
  - Clearly mark as novelty feature

---

## 7. Performance & Safety Controls

- [ ] Global limits:
  - Max files per run for preview generation
  - Auto‑disable previews on very large recursive runs
- [ ] Skip generation when free memory is low
- [ ] Skip softlinks or use generic link icon
- [ ] Deterministic output: same input = same icon every run

---

## 8. UI & Documentation

- [ ] Add clear UI wording:
  - “Only generates icons when no existing icon is present”
- [ ] Add per‑feature enable/disable toggles
- [ ] Add hints explaining one‑time generation cost
- [ ] Extend documentation:
  - DefIcons preview behaviour
  - Template icon system
  - Restore/rollback implications
- [ ] Add run summary breakdown:
  - Thumbnails created
  - Font previews created
  - Text previews created
  - Skipped (cached / too large / unsupported)

---

## 9. Experimental / Nice‑to‑Have

- [ ] Badge overlays (e.g. small ‘TXT’, ‘IMG’, ‘FONT’ markers)
- [ ] Per‑drawer override via tooltypes or config file
- [ ] Optional regeneration command for iTidy‑created icons only
- [ ] Themed template sets (paper, terminal, blueprint, etc.)

---

**Status:** Ideas backlog  
**Priority:** Low–Medium (non‑core, quality‑of‑life features)  
**Philosophy:** Pre‑calculated, deterministic, Workbench‑native, never invasive
---

## 10. Iconify Mode (Workbench AppIcon Drop Target)

Goal: Allow iTidy to be **iconified** to an **AppIcon** on the Workbench and process folders that are **dropped onto it**, using the **current GUI settings** (including DefIcons creation options).

- [ ] Add **Iconify** action (button/menu) in iTidy GUI
- [ ] When iconified, create a Workbench **AppIcon** (runtime icon)
- [ ] Handle Workbench **AppMessage** drop events:
  - [ ] Accept dropped **drawers** (primary use)
  - [ ] Decide policy for dropped **files** (ignore or process parent drawer)
  - [ ] Support multiple dropped items (queue + process sequentially)
- [ ] On drop, run a **single-folder tidy** using current settings:
  - [ ] Respect recurse option if enabled
  - [ ] Respect “create missing icons” and preview icon features if enabled
- [ ] Progress + UX:
  - [ ] Show progress window while processing drops
  - [ ] Option: remain iconified while running (recommended default)
  - [ ] Option: auto re-iconify when finished
- [ ] Safety/guards:
  - [ ] Warn/skip read-only volumes
  - [ ] Optional confirmation for risky targets (root/system drawers)
- [ ] Reporting:
  - [ ] Log “drop runs” distinctly in run summary/history
  - [ ] Include created-icon counts for drop runs the same as normal runs


