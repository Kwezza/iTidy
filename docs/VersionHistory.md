[1.0.0] - 2026-01-13
- First release

[1.1.0] - 2026-01-26
- Added block-based icon layout grouping by Workbench type (WBDRAWER/WBTOOL/Other)
  - Each block has independent column optimization for polished appearance
  - Narrower blocks automatically centered within window
  - Configurable gap sizes between blocks (Small/Medium/Large)
  - New "Grouping" and "Gap" controls in Advanced window
  - Works with all existing features (sorting, centering, aspect ratio, recursive processing)

[1.2.0] - 2026-02-16
- Implemented IFF Thumbnail palette reduction system (Plan sections 12b-12e)
  - Median Cut quantization algorithm for reducing palette to 4-256 colors
  - Bayer 4x4 ordered dithering and Floyd-Steinberg error diffusion
  - Grayscale, Workbench palette, and Hybrid mapping modes for 4-8 color targets
  - Ultra Quality mode with detail-preserving 256-color palette optimization
  - Auto dithering mode that selects method based on target color count
  - All arithmetic integer-only (no floats) for 68020 compatibility
- Added 3 new choosers to DefIcons Settings window:
  - Max Colors (4/8/16/32/64/128/256/Ultra) with dynamic ghosting
  - Dithering (None/Ordered/Floyd-Steinberg/Auto)
  - Low-Color Mapping (Grayscale/WB palette/Hybrid) - active when max colors <= 8
- Bumped preferences file format from v1 to v2 with backward compatibility
  - v1 files load successfully with palette settings defaulting to 256/Auto/Grayscale
  - v2 files include struct size for forward compatibility
  - Validation of loaded palette settings to prevent out-of-range values

