# iTidy v2.0

**A Workbench icon and window tidy tool for AmigaOS 3.2**

iTidy tidies Workbench drawer icon layouts and resizes drawer windows so they fit their contents more consistently. It can also create missing icons using Workbench 3.2's DefIcons system (including optional thumbnail previews), scan and repair broken Default Tools, and (optionally) create LhA-based backups so you can roll back icon/layout changes later.

**Note:** iTidy works by reading and writing Workbench `.info` files and drawer window layout data. It does not modify the contents of your data files.


## Table of Contents

1. [Requirements](#requirements)
2. [Introduction](#introduction)
3. [Getting Started](#getting-started)
4. [Main Window](#main-window)
5. [Advanced Settings](#advanced-settings)
6. [Icon Creation Settings](#icon-creation-settings)
7. [DefIcons Categories](#deficons-categories)
8. [Text Templates](#text-templates)
9. [Exclude Paths](#exclude-paths)
10. [Default Tool Analysis](#default-tool-analysis)
11. [Replacing Default Tools](#replacing-default-tools)
12. [Restoring Default Tool Backups](#restoring-default-tool-backups)
13. [Restoring Layout Backups](#restoring-layout-backups)
14. [Folder View](#folder-view)
15. [ToolTypes](#tooltypes)
16. [Tips & Troubleshooting](#tips--troubleshooting)
17. [Credits & Version Info](#credits--version-info)

---

## Requirements

iTidy v2.0 requires:

- AmigaOS Workbench 3.2 or newer (ReAction GUI)
- 68000 CPU or better
- At least 1 MB of free memory
- At least 1 MB of free storage space in the installation location (more as backups accumulate)
- For backup and restore features: LhA installed in `C:`
  - Download LhA from: http://aminet.net/package/util/arc/lha

---

## Introduction

### Purpose

iTidy is a Workbench maintenance tool. It focuses on jobs that otherwise require lots of manual Workbench work:

- tidying icon positions into a consistent grid
- resizing drawer windows to match the resulting layout
- optionally creating missing icons so everything in a drawer can be laid out
- optionally repairing broken Default Tools so icons open cleanly

### What iTidy reads and writes

You should expect iTidy to touch these items on disk:

- **Drawer and file icons:** iTidy reads and writes Workbench `.info` files.
  - Icon positions, tooltypes, and default tools live in these `.info` files.
  - If icon creation is enabled, iTidy may create new `.info` files for items that previously had none.

- **Drawer window snapshots:** Workbench stores drawer window geometry in the drawer's `.info` file.
  - When iTidy resizes a drawer window, it writes the new window geometry back to the `.info` file.

- **Backups (optional):**
  - Layout backups are stored under `PROGDIR:Backups/Run_0001`, `Run_0002`, ... and contain one `.lha` archive per processed folder plus a `catalog.txt` (when present).
  - Default tool backups (when enabled in preferences) are stored under `PROGDIR:Backups/tools/YYYYMMDD_HHMMSS/` and contain `session.txt` and `changes.csv`.

- **DefIcons default tool changes (only if you change them):**
  - The DefIcons Categories window can write default tool changes directly into template icons under `ENVARC:Sys/`.

iTidy does **not** change the contents of your data files. For example, it does not rewrite documents, archives, or images. (Thumbnail previews and text previews are written into icon images inside `.info` files, not into the original files.)

### Safety and expectations

- iTidy can make lots of changes quickly, especially with recursion enabled. Test on a small drawer first.
- If you enable backups, you get a practical rollback path for `.info` changes. Backups are not a replacement for your normal backups.
- If you cancel an operation, changes already written to disk are **not** reverted.

---

## Getting Started

### Purpose

Run a simple, safe tidy pass so you can confirm the results match your Workbench setup.

### Typical Workflow

1. Launch iTidy.
2. In **Folder to tidy:** choose a small test drawer.
3. Leave **Grouping** at `Folders First` (default) and **Sort By** at `Name` (default).
4. Leave **Include Subfolders** off for the first run.
5. If you want a rollback option, enable **Back Up Layout Before Changes** (requires LhA in `C:`).
6. Click **Start**.
7. Watch the progress window and read the final summary before closing it.

### Settings That Matter

- **Back Up Layout Before Changes**
  - **Effect:** Creates a restore point before `.info` files are modified.
  - **When to use:** Any time you are tidying more than a small test drawer.
  - **Gotcha:** If LhA is missing, you will be prompted to continue without backups or cancel.
  - **Default:** Off.

- **Include Subfolders**
  - **Effect:** Turns one drawer tidy into a whole tree tidy.
  - **When to use:** Whole volumes, archive extractions, collections.
  - **Gotcha:** On classic hardware, very large trees can take a while.
  - **Default:** Off.

### Example

You unpack a large archive into `Work:Downloads/Unpacked`. The icons are scattered and the drawer window is the wrong size.

1. Set **Folder to tidy** to `Work:Downloads/Unpacked`.
2. Enable **Back Up Layout Before Changes**.
3. Click **Start**.
4. Re-open the drawer in Workbench: icons should appear in a neater grid and the window should be resized to suit.

---

## Main Window

Window title: **iTidy v2.0 - Icon Cleanup Tool**

### Purpose

This is the main task window. You pick a target drawer, choose how you want icons grouped and sorted, and start a tidy run. From here you can also open the windows for advanced layout, icon creation, default tool repair, and restores.

### Typical Workflow

1. Set **Folder to tidy**.
2. Choose **Grouping** and **Sort By**.
3. Decide whether to enable **Include Subfolders**.
4. Decide whether to enable **Create Icons During Tidy**.
5. (Optional) Enable **Back Up Layout Before Changes**.
6. Click **Start** and monitor the progress window.

### What happens when you click Start

During a tidy run you should expect:

- A progress window titled **iTidy - Progress**.
- Status messages showing which folder is being processed.
- Updated `.info` files written to disk as each folder completes.
- If icon creation is enabled, new `.info` files may appear for items that had none.

If you click Cancel in the progress window and confirm, iTidy stops as soon as practical. Any `.info` files already written remain changed.

### Settings That Matter

- **Folder to tidy**
  - **Effect:** Sets the root drawer iTidy will process.
  - **When to use:** Choose a drawer, or a whole volume like `Work:`.
  - **Gotcha:** The path field is read-only; use the requester button.
  - **Default:** `SYS:`.

- **Grouping**
  - **Effect:** Controls the high-level grouping order before sorting.
  - **When to use:**
    - `Folders First` for typical Workbench drawers.
    - `Grouped By Type` when you want visual blocks (drawers, tools, projects, etc.).
  - **Gotcha:** When `Grouped By Type` is selected, **Sort By** is disabled. Items within each group are still sorted by name.
  - **Default:** `Folders First`.

- **Sort By**
  - **Effect:** Chooses the sort key inside each grouping.
  - **When to use:**
    - `Name` for most drawers.
    - `Date` for "newest first" clean-ups (combine with Reverse Sort Order in Advanced Settings).
  - **Gotcha:** Disabled when `Grouping` is `Grouped By Type`.
  - **Default:** `Name`.

- **Include Subfolders**
  - **Effect:** Processes the entire drawer tree under the selected folder.
  - **When to use:** Whole collections or partitions.
  - **Gotcha:** Combine with **Skip Hidden Folders** (Advanced Settings) if you do not want to touch folders that Workbench does not show.
  - **Default:** Off.

- **Create Icons During Tidy**
  - **Effect:** Creates missing `.info` files using DefIcons while tidying, so those items can be included in the layout.
  - **When to use:** Drawers where you have lots of files without icons.
  - **Gotcha:** Icon creation can add time, especially with picture previews enabled (and especially JPEG on 68k).
  - **Default:** Off.

- **Back Up Layout Before Changes**
  - **Effect:** Creates LhA backups of `.info` files before modifying them.
  - **When to use:** Any non-trivial run.
  - **Gotcha:** Requires LhA in `C:`. If it is missing, you can continue without backups or cancel.
  - **Default:** Off.

- **Window Position**
  - **Effect:** Controls where resized drawer windows end up.
  - **When to use:**
    - `Keep Position` if you have carefully arranged windows.
    - `Near Parent` if you like cascading drawers.
  - **Gotcha:** Only affects windows that iTidy actually resizes.
  - **Default:** `Center Screen`.

- **Advanced... / Icon Creation... / Fix default tools... / Restore backups...**
  - **Effect:** Opens the related window for the task.
  - **When to use:** Open these when the defaults are not giving you the result you want.
  - **Gotcha:** Default Tool Analysis scanning inside that feature is always recursive, regardless of **Include Subfolders**.

- **Presets (open/save/reset)**
  - **Effect:** Lets you save your current settings to disk and load them later.
  - **When to use:** Keep different setups (for example, one preset for photo folders and one for source trees).
  - **Gotcha:** Advanced/Icon Creation changes apply immediately to the session, but are only made permanent when you save a preset.
  - **Defaults:** Default preset location is `PROGDIR:userdata/Settings/`.

- **Logging**
  - **Effect:** Controls how much iTidy writes into `PROGDIR:logs/`.
  - **When to use:** Set to Info/Debug when diagnosing issues.
  - **Gotcha:** Logging is disabled by default for performance and disk noise reasons.
  - **Default:** Disabled.

### Example

You have a drawer `Work:Projects` with a mix of sub-drawers, tools, and documents. You want drawers first, then files alphabetically.

1. Set **Folder to tidy** to `Work:Projects`.
2. Leave **Grouping** at `Folders First` and **Sort By** at `Name`.
3. Enable **Back Up Layout Before Changes**.
4. Click **Start**.

---

## Advanced Settings

Window title: **iTidy - Advanced Settings**

### Purpose

Fine-tune how iTidy lays out icons and how it resizes drawer windows. Use this when the main window options are not enough (for example, you want stricter column layout, different density, or different behaviour when a drawer has too many icons).

Settings are organised into tabs. Changes apply when you click **OK**. They are not written to disk until you save a preset from the main window.

### Typical Workflow

1. Open **Advanced...** from the main window.
2. Adjust one group of settings (for example, Density or Limits).
3. Click **OK**.
4. Run a tidy on a test drawer and check the result.
5. If you want to keep the settings, use Presets -> Save on the main window.

### Settings That Matter

- **Layout Aspect**
  - **Effect:** Changes the target width-to-height shape iTidy aims for when resizing drawer windows.
  - **When to use:**
    - Wider values for widescreen Workbench setups.
    - Square/tall values for narrow screens or when you prefer taller lists.
  - **Gotcha:** If a drawer has too many icons to fit, the When Full setting decides how iTidy breaks the target.
  - **Default:** `Wide (2.0)`.

- **When Full**
  - **Effect:** Decides whether iTidy adds columns, rows, or both when the target shape cannot fit all icons.
  - **When to use:**
    - `Expand Horizontally` if you prefer left/right scrolling.
    - `Expand Vertically` if you prefer up/down scrolling.
  - **Gotcha:** This affects how much scrolling you need after the tidy.
  - **Default:** `Expand Horizontally`.

- **Horizontal Spacing / Vertical Spacing**
  - **Effect:** Controls pixel gap between icons.
  - **When to use:** Increase spacing for readability, reduce it to pack more icons into a smaller window.
  - **Gotcha:** Very tight spacing can make dense drawers hard to scan visually.
  - **Default:** 8 / 8.

- **Icons Per Row: Min**
  - **Effect:** Prevents a drawer from becoming a single narrow column.
  - **When to use:** Raise it for drawers where you want a more grid-like look.
  - **Gotcha:** Setting this too high can force wider windows than you expect.
  - **Default:** 2.

- **Auto-Calc Max Icons / Max**
  - **Effect:** Controls the maximum number of columns.
  - **When to use:** Turn off Auto-Calc if you want a hard cap.
  - **Gotcha:** If you disable Auto-Calc and set Max too low, large drawers will grow vertically and require more scrolling.
  - **Default:** Auto-Calc On.

- **Max Window Width**
  - **Effect:** Caps how wide iTidy will make a drawer window.
  - **When to use:** Prevent windows spanning the whole Workbench screen.
  - **Gotcha:** Combined with high Min icons-per-row it may force taller windows instead.
  - **Default:** `70%`.

- **Column Layout / Auto-Fit Columns**
  - **Effect:** Switches to stricter column behaviour and optionally sizes each column to its widest icon.
  - **When to use:** When you want predictable vertical columns.
  - **Gotcha:** Column layout can look "rigid" compared to free-flow layout.
  - **Default:** Column Layout Off; Auto-Fit Columns On.

- **Gap Between Groups**
  - **Effect:** Controls the visual spacing between groups when Grouping is `Grouped By Type`.
  - **When to use:** When grouped layouts look too cramped or too loose.
  - **Gotcha:** Empty groups are skipped, so gaps only appear between groups that exist.
  - **Default:** Medium.

- **Reverse Sort Order**
  - **Effect:** Reverses name/date/size sort.
  - **When to use:** "Newest first" or "largest first" views.
  - **Gotcha:** Has no effect if the underlying sort does not apply (for example, Sort By disabled under Grouped By Type).
  - **Default:** Off.

- **Strip Newicons Borders**
  - **Effect:** Permanently removes the black border from NewIcons.
  - **When to use:** Only if you know you want this look.
  - **Gotcha:** This permanently modifies icons. Enable layout backups first. Requires `icon.library` v44 or later.
  - **Default:** Off.

- **Skip Hidden Folders**
  - **Effect:** During recursive tidies, skips folders that have no corresponding `.info` file (folders "hidden" from Workbench).
  - **When to use:** When you want iTidy to behave like Workbench (only touch folders that are visible).
  - **Gotcha:** If you expected recursion to include every directory on disk, this will skip some.
  - **Default:** On.

### Example

You have a drawer full of wide thumbnails and you do not want the window to become extremely wide.

1. Open **Advanced...**.
2. Set **Max Window Width** to `50%`.
3. Leave **Layout Aspect** at `Wide (2.0)`.
4. Click **OK** and run a tidy.

---

## Icon Creation Settings

Window title: **iTidy - Icon Creation Settings**

### Purpose

Configure how iTidy creates missing `.info` files during a tidy run, including optional thumbnail previews for pictures and text files.

This feature uses the DefIcons type system (Workbench 3.2) to decide what type each file is and what base icon it should use.

### Typical Workflow

1. In the main window, enable **Create Icons During Tidy**.
2. Open **Icon Creation...**.
3. Decide whether you want picture previews and/or text previews.
4. If you want picture previews, decide which formats to enable.
5. Click **OK**.
6. Run a tidy.

### Settings That Matter

- **Folder Icons**
  - **Effect:** Controls whether iTidy creates missing drawer icons.
  - **When to use:**
    - `Smart` for normal use.
    - `Always Create` if you want every folder to be Workbench-visible.
  - **Gotcha:** `Smart` only creates a folder icon when the folder actually has work to do.
  - **Default:** `Smart (When Folder Has Icons)`.

- **Skip Files In WHDLoad Folders**
  - **Effect:** When a folder contains a `*.slave`, iTidy does not create icons for files inside it.
  - **When to use:** WHDLoad collections, where internal files are not meant to be launched.
  - **Gotcha:** The folder icon itself can still be created.
  - **Default:** Off.

- **Text File Previews**
  - **Effect:** Generates a text preview image on the icon.
  - **When to use:** Source trees, documentation folders.
  - **Gotcha:** iTidy reads a limited number of bytes from each file (controlled by template ToolTypes) and renders that, not the whole file.
  - **Default:** On.

- **Picture File Previews**
  - **Effect:** Generates a thumbnail image on the icon.
  - **When to use:** Image collections.
  - **Gotcha:** Thumbnail generation can be CPU heavy. See JPEG note below.
  - **Default:** On.

- **Picture Formats (ILBM/PNG/GIF/JPEG/BMP/ACBM/Other)**
  - **Effect:** Limits which image types get thumbnails.
  - **When to use:** Disable formats you do not need to reduce processing time.
  - **Gotcha:** `JPEG (Slow)` is disabled by default for performance reasons on 68k.
  - **Defaults:** ILBM On, PNG On, GIF On, JPEG Off, BMP On, ACBM On, Other On.

- **Replace Existing Image Thumbnails Created by iTidy / Replace Existing Text Previews Created by iTidy**
  - **Effect:** Recreates thumbnails/text previews that iTidy previously created (identified by `ITIDY_CREATED=1` tooltype).
  - **When to use:** After you change preview size, borders, or templates and want existing generated icons updated.
  - **Gotcha:** This does not touch icons created by you or other tools.
  - **Default:** Off.

- **Preview Size**
  - **Effect:** Controls thumbnail canvas size.
  - **When to use:** Larger sizes for more detail, smaller for denser drawers.
  - **Gotcha:** Larger previews usually mean larger `.info` files and more memory/time during creation.
  - **Default:** `Medium (64x64)`.

- **Thumbnail Border**
  - **Effect:** Adds a frame/bevel around thumbnails.
  - **When to use:** To make thumbnails stand out against busy backgrounds.
  - **Gotcha:** Smart modes may skip borders on images with transparency.
  - **Default:** `Bevel (Smart)`.

- **Upscale Small Images**
  - **Effect:** Scales small images up to fill the preview.
  - **When to use:** When many source images are smaller than your chosen preview size.
  - **Gotcha:** Upscaling can make small images look soft or blocky.
  - **Default:** Off.

- **Max Colours / Dithering / Low-Colour Palette**
  - **Effect:** Controls how full-colour images are reduced to an icon-friendly palette.
  - **When to use:**
    - Lower colour counts for speed and smaller icon files.
    - Higher colour counts for quality.
  - **Gotcha:** At 256 colours (full) and Ultra, Dithering is disabled.
  - **Default:** Max Colours `256 colours (full)`; Dithering `Auto`.

### Performance note: JPEG on 68k

JPEG decoding is slow on classic 68k hardware. If you enable `JPEG (Slow)` and process a folder with lots of JPEG files, icon creation can dominate the run time.

### Example

You have a drawer full of PNG screenshots and you want thumbnails, but you do not want a slow run on a 68030.

1. Open **Icon Creation...**.
2. Leave `PNG` enabled. Leave `JPEG (Slow)` disabled.
3. Set **Preview Size** to `Small (48x48)`.
4. Click **OK**, then run a tidy with **Create Icons During Tidy** enabled.

---

## DefIcons Categories

Window title: **DefIcons: Icon Creation Setup**

### Purpose

Choose which DefIcons file types should have icons created during processing. This is the main way to stop iTidy creating icons for categories you do not care about.

This window reads the DefIcons tree from `ENV:deficons.prefs` (managed by the Workbench Preferences tool that configures DefaultIcons/DefIcons).

### Typical Workflow

1. Open **DefIcons: Icon Creation Setup** from Icon Creation Settings -> DefIcons tab, or from the main menu.
2. Expand categories and tick/untick file types.
3. (Optional) Click **Show Default Tools** to see what each type would launch.
4. Click **OK** to keep checkbox changes, or **Cancel** to discard them.

### Settings That Matter

- **Type checkboxes**
  - **Effect:** A checked type means iTidy will create icons for files of that type.
  - **When to use:** Untick types where icon creation is noisy or pointless.
  - **Gotcha:** Only second-level types have checkboxes. Root categories and deeper sub-types do not.
  - **Defaults:** Several categories are disabled by default: `tool`, `prefs`, `iff`, `key`, `kickstart`. Most others are enabled.

- **Show Default Tools**
  - **Effect:** Scans template icons under `ENVARC:Sys/` and shows the assigned default tool next to the type.
  - **When to use:** When you are diagnosing "Unable to open your tool" problems.
  - **Gotcha:** This only shows direct template icons. Inheritance is not shown.

- **Change Default Tool...**
  - **Effect:** Writes a new default tool into the selected type's template icon under `ENVARC:Sys/`.
  - **When to use:** When you want a type to open in a different viewer/editor.
  - **Gotcha:** This is written to disk immediately. It is not affected by OK/Cancel.

### Example

You want iTidy to create icons for pictures, but not for executable tools.

1. Open **DefIcons: Icon Creation Setup**.
2. Leave picture-related types enabled.
3. Ensure the `tool` category stays disabled.
4. Click **OK**.

---

## Text Templates

Window title: **iTidy - DefIcons Text Templates**

### Purpose

Control how text file previews are rendered onto icons. Templates are standard Workbench icons stored under `PROGDIR:Icons/`.

- Master template: `def_ascii.info` (fallback for all ASCII sub-types)
- Optional per-type templates: `def_<type>.info` (for example `def_c.info`, `def_rexx.info`)

### Typical Workflow

1. Open **Text Templates** (from Icon Creation Settings -> Manage Templates, or from the main menu).
2. Select a type (for example `c`).
3. Create a custom template from the master.
4. Edit ToolTypes in Workbench Information.
5. Validate ToolTypes in iTidy to catch typos/out-of-range values.

### Settings That Matter

- **Show filter (All / Custom Only / Missing Only)**
  - **Effect:** Filters the list so you can focus on just missing templates or just custom ones.
  - **When to use:** Use Missing Only to find types that are still using the master; use Custom Only when cleaning up overrides.
  - **Gotcha:** The master (`ascii`) row always appears regardless of filter.

- **Master vs custom templates**
  - **Effect:** If a type has `def_<type>.info`, iTidy uses it. Otherwise it uses `def_ascii.info`.
  - **When to use:** Create a custom template for types where you want different text size, colours, or layout.
  - **Gotcha:** If the master template is missing, many operations will fail.

- **Edit tooltypes...**
  - **Effect:** Opens the effective template icon in Workbench Information (via the WORKBENCH ARexx port).
  - **When to use:** To change how previews look.
  - **Gotcha:** Changes are saved in Workbench Information, not inside iTidy.

- **Validate tooltypes**
  - **Effect:** Checks `ITIDY_*` ToolTypes for typos, formatting, and out-of-range values and shows an effect summary.
  - **When to use:** After you edit a template.
  - **Gotcha:** Validation does not change anything; it only reports.

- **Excluded types (EXCLUDETYPE)**
  - **Effect:** Types listed in `EXCLUDETYPE` inside `def_ascii.info` are excluded from receiving icons.
  - **When to use:** When you never want previews/icons for certain sub-types.
  - **Gotcha:** You manage this by editing the master template's ToolTypes.

### Template ToolTypes

Template icons use `ITIDY_*` tooltypes. These control the rendering. iTidy understands (at minimum) the following keys:

| ToolType | Purpose |
|----------|---------|
| ITIDY_TEXT_AREA | Text draw rectangle (x,y,w,h) |
| ITIDY_EXCLUDE_AREA | Exclude rectangle (x,y,w,h) |
| ITIDY_LINE_HEIGHT | Pixel height of each text line |
| ITIDY_LINE_GAP | Pixel gap between lines |
| ITIDY_MAX_LINES | Maximum number of lines |
| ITIDY_CHAR_WIDTH | Pixel width per character |
| ITIDY_READ_BYTES | Bytes read from the file |
| ITIDY_BG_COLOR | Background colour index (0-255) |
| ITIDY_TEXT_COLOR | Text colour index (0-255) |
| ITIDY_MID_COLOR | Mid-tone colour index (0-255) |
| ITIDY_DARKEN_PERCENT | Darkening percentage (0-100) |
| ITIDY_DARKEN_ALT_PERCENT | Alternate darkening percentage (0-100) |
| ITIDY_ADAPTIVE_TEXT | Adaptive text colouring on/off |
| ITIDY_EXPAND_PALETTE | Palette expansion on/off |

### Example

You want C source previews to show more lines.

1. Open **Text Templates**.
2. Select type `c`.
3. Click **Create from master**.
4. Click **Edit tooltypes...** and increase `ITIDY_MAX_LINES`.
5. Back in iTidy, click **Validate tooltypes** to confirm the value is recognised.

---

## Exclude Paths

Window title: **DefIcons Exclude Paths**

### Purpose

Stop DefIcons icon creation in folders where it is not wanted or where it would waste time (for example system directories, caches, or large collections where you do not use Workbench icons).

### Typical Workflow

1. Open **Exclude Paths** from Icon Creation Settings or from the main menu.
2. Add one or more folders.
3. Choose whether to store each path as an absolute path or as a portable `DEVICE:` pattern.
4. Click **OK**.

### Settings That Matter

- **DEVICE: placeholder**
  - **Effect:** Makes an exclude path portable across volumes by substituting the current scanned volume name.
  - **When to use:** For standard directories (for example `C`, `Libs`, `Devs`) that exist on most bootable volumes.
  - **Gotcha:** `DEVICE:C` matches `Workbench:C` and `Work:C` depending on what you are scanning.

- **Default exclude list**
  - **Effect:** A set of 17 common system directories.
  - **When to use:** As a baseline; add your own as needed.
  - **Gotcha:** Excluded folders are skipped for icon creation, not for tidying existing icons.

- **Where changes apply**
  - **Effect:** The window edits a working copy and normally applies changes only when you click **OK**.
  - **When to use:** Always click **OK** after edits.
  - **Gotcha:** When opened from Icon Creation Settings, changes may apply immediately. (This inconsistency may change in future.)

### Example

You are tidying `Work:` with icon creation enabled, but you do not want thumbnails created in `Work:WHDLoad`.

1. Open **Exclude Paths**.
2. Add `Work:WHDLoad` and choose `DEVICE:WHDLoad` if you want it portable.
3. Click **OK**.

---

## Default Tool Analysis

Window title: **iTidy - Default Tool Analysis**

### Purpose

Find icons whose Default Tool is missing or wrong, then fix them in batch or one-by-one. This helps with the common Workbench error "Unable to open your tool".

This feature reads `.info` files, collects the tools they reference, checks whether those tools exist on disk, and lets you replace the tool path.

### Typical Workflow

1. Open **Fix default tools...** from the main window.
2. Set the **Folder** to scan.
3. Click **Scan**.
4. Use the filter to show only missing tools.
5. Replace a tool in batch (all icons using it) or in a single icon.
6. (Optional) Export a report for documentation.

### Settings That Matter

- **Scan is always recursive**
  - **Effect:** Scans the selected folder and all subfolders.
  - **When to use:** Plan for this; choose your scan root carefully.
  - **Gotcha:** This is independent of the main window's **Include Subfolders**.

- **Filter (Show All / Valid Only / Missing Only)**
  - **Effect:** Helps you focus on the tools that actually need attention.
  - **When to use:** Use Missing Only for repairs; use All for audits.
  - **Gotcha:** The filter only changes display; it does not change scan results.

- **Save/Open tool cache**
  - **Effect:** Lets you re-open scan results without re-scanning.
  - **When to use:** Slow volumes, repeated work.
  - **Gotcha:** A saved cache can become stale if you install/remove tools or change icons.

- **Export reports**
  - **Effect:** Writes text reports listing tools (and optionally the files that reference them).
  - **When to use:** Audits and clean-up planning.
  - **Gotcha:** The report reflects the current tool cache, not live disk changes since the scan.

- **System PATH...**
  - **Effect:** Shows where iTidy looked for tools.
  - **When to use:** When a tool is reported missing but you think it exists.
  - **Gotcha:** The PATH list only exists after a scan.

### Example

Many icons in `Work:Docs` fail to open because they point to an old viewer.

1. Open **Default Tool Analysis**.
2. Set Folder to `Work:Docs` and click **Scan**.
3. Filter to `Show Missing Only`.
4. Select the missing tool and click **Replace Tool (Batch)...**.
5. Choose your replacement viewer (for example `SYS:Utilities/MultiView`) and click **Update Default Tool**.

---

## Replacing Default Tools

Window title: **iTidy - Replace Default Tool**

### Purpose

Apply a Default Tool change to one icon file (single mode) or to every icon that references a particular tool (batch mode).

### Typical Workflow

1. Start from Default Tool Analysis.
2. Choose **Replace Tool (Batch)...** or **Replace Tool (Single)...**.
3. Use **Change To** to choose the replacement program.
4. Click **Update Default Tool**.
5. Review the per-file results list.

### Settings That Matter

- **Change To**
  - **Effect:** Selects the replacement program.
  - **When to use:** Choose the program Workbench should run for those icons.
  - **Gotcha:** `.info` files cannot be selected.

- **Leaving Change To empty**
  - **Effect:** Clears the Default Tool from the selected icon(s).
  - **When to use:** When you want an icon to have no forced tool association.
  - **Gotcha:** Workbench may then use other rules (project file tooltypes, filetype defaults, or prompts) when opening.

- **Write-protected icons**
  - **Effect:** iTidy skips them and reports `READ-ONLY`.
  - **When to use:** Remove write protection if you want them fixed.
  - **Gotcha:** Skips are not errors; they are expected.

- **Update button is single-use**
  - **Effect:** After an update, the button disables until you close and re-open the window.
  - **When to use:** Plan changes in one batch where possible.
  - **Gotcha:** This is deliberate.

- **Backups (if enabled)**
  - **Effect:** Records changes under `PROGDIR:Backups/tools/YYYYMMDD_HHMMSS/` so you can restore later.
  - **When to use:** Enable this if you want an undo trail for Default Tool changes.
  - **Gotcha:** Restoring uses the recorded session; it does not guess what your tools "should" be.

### Example

You want all `*.txt` icons that currently use a missing tool `OldViewer` to open in MultiView.

1. Scan the folder in Default Tool Analysis.
2. Select `OldViewer` and choose **Replace Tool (Batch)...**.
3. Set **Change To** to `SYS:Utilities/MultiView`.
4. Click **Update Default Tool**.

---

## Restoring Default Tool Backups

Window title: **iTidy - Restore Default Tools**

### Purpose

Undo previous Default Tool replacement operations recorded by iTidy.

Sessions are stored under `PROGDIR:Backups/tools/` and contain:

- `session.txt` (metadata)
- `changes.csv` (one line per icon changed)

### Typical Workflow

1. Open **Restore Default Tools** (from Default Tool Analysis or the Tools -> Restore menu).
2. Select a session.
3. Click **Restore**.
4. Re-scan in Default Tool Analysis if you want an updated view.

### Settings That Matter

- **Restore does not delete sessions**
  - **Effect:** You can restore multiple times.
  - **When to use:** Keep sessions until you are confident.
  - **Gotcha:** Disk usage grows until you delete sessions.

- **Delete is permanent**
  - **Effect:** Removes the session folder and its files.
  - **When to use:** After you no longer need that undo point.
  - **Gotcha:** There is no undo for deleting a session.

### Example

You batch-replaced a tool and later decide it was the wrong choice.

1. Open **Restore Default Tools**.
2. Select the session from the date/time you ran the batch.
3. Click **Restore**.
4. Go back to Default Tool Analysis and re-scan if you want to verify.

---

## Restoring Layout Backups

Window title: **iTidy - Restore Backups**

### Purpose

Undo a tidy run by restoring backed-up `.info` files from a previous run.

Backups are stored under `PROGDIR:Backups/Run_0001`, `Run_0002`, ... Each run typically contains:

- `catalog.txt` (metadata listing archived folders)
- one `.lha` archive per processed folder

### Typical Workflow

1. Open **Restore backups...** from the main window.
2. Select a run.
3. Click **Restore Run**.
4. Choose whether to restore **With Windows** or **Icons Only**.
5. Wait for the restore progress to complete.

### Settings That Matter

- **With Windows vs Icons Only**
  - **Effect:**
    - With Windows restores icon data and drawer window snapshots.
    - Icons Only restores icon data but does not change window geometry.
  - **When to use:** Use Icons Only if you like your current window sizes.
  - **Gotcha:** Both options overwrite current `.info` files.

- **Delete Run**
  - **Effect:** Permanently deletes the selected run directory and its archives.
  - **When to use:** When you are sure you no longer need that restore point.
  - **Gotcha:** This cannot be undone.

- **View Folders...**
  - **Effect:** Opens a read-only tree view of what the run contains.
  - **When to use:** Before restoring, to confirm which folders are included.
  - **Gotcha:** Only available when the run has a catalog (status "Complete").

- **Runs with "NoCAT" status**
  - **Effect:** Runs without a catalog cannot show the folder tree.
  - **When to use:** You can still restore or delete them.
  - **Gotcha:** View Folders is disabled.

- **DefIcons-created icons**
  - **Effect:** If the original run created new icons, restoring that run may remove those created icons.
  - **When to use:** Expect this if you enabled icon creation.
  - **Gotcha:** The restore is trying to return to the pre-run state.

- **LhA requirement**
  - **Effect:** Restore uses LhA to extract archives.
  - **When to use:** Install LhA in `C:`.
  - **Gotcha:** Restore will fail if LhA is missing.

### Example

You tidied `Work:Projects` yesterday and you want to go back to how it was.

1. Open **Restore Backups**.
2. Select yesterday's run.
3. Click **Restore Run** and choose **With Windows**.
4. Re-open the drawer in Workbench.

---

## Folder View

Window title: **Folder View - Run N (date)** (dynamic)

### Purpose

Show which folders a backup run contains, in a read-only tree view.

### Typical Workflow

1. In **Restore Backups**, select a run with a catalog (status "Complete").
2. Click **View Folders...** (or double-click the run).
3. Expand branches using the disclosure triangles.
4. Close the window.

### Settings That Matter

- There are no settings here. This window is informational.
- The Size/Icons column shows compressed archive size (from the catalog), not original file sizes.

### Example

You are not sure whether a run included `Work:Projects/Old`.

1. Open **View Folders...** for the run.
2. Expand `Work:Projects`.
3. Check whether `Old` appears in the tree.

---

## ToolTypes

### Purpose

Control iTidy behaviour at launch (logging and automation) when starting iTidy from Workbench.

### Typical Workflow

1. In Workbench, select the iTidy program icon.
2. Open Information (Workbench "Information...").
3. Add or edit the ToolTypes listed below.
4. Save the icon.
5. Launch iTidy from Workbench so the ToolTypes are applied.

**Gotcha:** ToolTypes are read from the program icon at launch. If you start iTidy in a way that bypasses Workbench icon ToolTypes, these settings may not apply.

iTidy reads these tooltypes from its program icon when launched from Workbench:

| ToolType | Values | Default | Effect / When / Gotcha |
|----------|--------|---------|-------------------------|
| DEBUGLEVEL | 0-4 | 4 (Disabled) | **Effect:** sets logging level (0=Debug, 1=Info, 2=Warning, 3=Error, 4=Disabled). **When:** use Debug for diagnostics. **Gotcha:** overrides any log level loaded from preferences. |
| LOADPREFS | File path | (none) | **Effect:** loads a preferences file at startup. **When:** use to pin iTidy to a known preset. **Gotcha:** paths without a device are resolved relative to `PROGDIR:userdata/Settings/`. |
| PERFLOG | YES / NO | NO | **Effect:** enables performance timing logs. **When:** benchmarking. **Gotcha:** produces extra log output. |

### Example

You want extra detail for diagnosing a problem run.

1. Set `DEBUGLEVEL` to `0` (Debug).
2. Run iTidy and reproduce the problem on a small drawer.
3. Use the main window logging menu to open `PROGDIR:logs/` and review the latest log files.

---

## Tips & Troubleshooting

### Purpose

Quick checks for common problems and the practical next step to take.

### Typical Workflow

1. Identify which feature you were using (tidy, icon creation, default tools, restore).
2. Check the relevant setting and the logs (if logging is enabled).
3. Re-run on a small folder to confirm the fix before scaling up.

### Settings That Matter

- **Logging level**
  - **Effect:** Controls how much iTidy writes into `PROGDIR:logs/`.
  - **When to use:** Enable Info/Debug when diagnosing.
  - **Gotcha:** Logging is disabled by default.

- **Backups**
  - **Effect:** Give you a rollback path.
  - **When to use:** Before large runs.
  - **Gotcha:** Restore overwrites current `.info` files.

### Common issues

#### LhA warnings

**Symptom:** iTidy warns that LhA is missing when you enable backups or try to restore.

**What to do:** Install LhA in `C:` and re-try the operation.

#### Icons do not open ("Unable to open your tool")

**Symptom:** Double-clicking icons fails.

**Likely cause:** An icon's Default Tool points to a program that is missing.

**What to do:** Use **Fix default tools...** -> Scan -> Replace Tool (Batch) for the missing tool.

#### DefIcons Categories list is empty

**Symptom:** The type tree is empty.

**Likely cause:** `ENV:deficons.prefs` is missing or could not be parsed.

**What to do:** Confirm you are on Workbench 3.2 and that the DefIcons system is installed/configured using the appropriate Preferences tool.

#### iTidy is skipping folders during recursion

**Symptom:** Some directories are not processed when Include Subfolders is enabled.

**Likely cause:** **Skip Hidden Folders** is enabled and the skipped folders have no `.info` file.

**What to do:** In Advanced Settings, disable **Skip Hidden Folders** if you really want all directories processed.

#### JPEG thumbnails are slow

**Symptom:** Icon creation runs very slowly on JPEG-heavy folders.

**Likely cause:** `JPEG (Slow)` was enabled.

**What to do:** Leave JPEG disabled unless you need it, or run icon creation in smaller batches.

### Example

You enable backups, run a tidy, then decide you preferred the previous layout.

1. Open **Restore backups...**.
2. Select the run you just created.
3. Click **Restore Run** and choose **Icons Only** if you want to keep your current window sizes.

---

## Credits & Version Info

**Author:** Kerry Thompson
**Version:** 2.0
**Website:** https://github.com/Kwezza/iTidy
**Special thanks:** Darren 'dmcoles' Cole for the excellent ReBuild which the iTidy interface was rebuilt with.
