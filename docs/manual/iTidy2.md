# iTidy v2.0

**A Workbench Icon and Window Tidy Tool for AmigaOS 3.2+**

Workbench drawers have a way of drifting into chaos over time, especially after big Aminet extractions, cover disks, or years of “I will sort it later” installs.

iTidy is a small utility I wrote to bring things back into shape. Point it at a folder or an entire volume and it will tidy icon layouts and drawer windows in a consistent, repeatable way, with optional recursion through subfolders.

Version 2.0 adds a Workbench 3.2 ReAction GUI plus DefIcons-based icon creation, so files without icons can be handled automatically. Where supported, it can also generate thumbnail-style icons for images. Backups are available so you can roll back a run if you change your mind.


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

- AmigaOS Workbench 3.2 or newer (ReAction GUI requires WB 3.2)
- 68000 CPU or better
- At least 1 MB free RAM (more recommended for large folders, recursion, and icon creation).
- At least 1 MB of free storage space in the installation location (more as backups accumulate)
- For backup and restore features: LhA must be installed in `C:`

---

## Introduction

iTidy focuses on a few areas that tend to cause friction once your disks and drawers start to grow.

**Manual tidying gets tedious.** Workbench's built-in "Clean Up" works one drawer at a time. iTidy can walk through entire directory trees (or a whole volume) in one go.

**Layouts drift over time.** Changing screen modes, fonts, or resolutions can leave icons overlapping or windows sized oddly. iTidy re-arranges icons into a grid, with adjustable spacing and proportions.

**Missing and inconsistent icons.** Large archive extractions often leave files sitting in drawers with no icons at all. Version 2.0 can create icons automatically using the DefIcons system, including thumbnail-style icons for images and preview icons for text files. If you enable it, iTidy can also refresh icons it previously generated.

**Default tools go missing.** Older archives often reference tools that are not present on a modern setup, resulting in "Unable to open your tool" errors. iTidy scans for missing tools and lets you fix them.

**No safety net for large changes.** iTidy includes an optional backup and restore system using LhA, so you can roll back icon and layout changes later if needed.

iTidy only works with Workbench metadata: `.info` files and drawer/window layout information. It does not modify the contents of your data files.

---

## Getting Started

1. Double-click the iTidy icon to launch it. The main window opens.

2. Click the **Folder to tidy** gadget and choose the drawer (or whole partition) you want to process. The selected path appears in the field.

3. Choose how you want icons grouped using **Grouping** (Folders First is the default).

4. If you want iTidy to process everything underneath the chosen folder, enable **Include Subfolders**.

5. If you want backups before any changes are made, enable **Back Up Layout Before Changes**. LhA must be installed in `C:` for this to work.

6. If you want iTidy to create new icons for files and folders that don't already have them, enable **Create Icons During Tidy**. You can configure which file types get icons, thumbnail settings, and other creation options via **Icon Creation...**.

7. For more control over layout and window sizing, click **Advanced...**.

8. Click **Start**. A progress window shows what iTidy is doing. When it finishes, the Cancel button changes to Close.

**Tip:** For your first run, try a small test folder. If you enable backups, you can always restore afterwards if you want to undo the changes.

---

## Main Window

The main window is where you choose what to tidy and set the basic options. All other windows are opened from here.

### Folder Selection

- **Folder to tidy** -- Click this to open a directory requester. The selected drawer path appears in the field afterwards. It defaults to `SYS:` when iTidy first launches.

### Tidy Options

- **Grouping** -- Sets how icons are grouped before sorting.
  - *Folders First* -- Drawer icons are laid out before file icons. **(Default)**
  - *Files First* -- File icons are laid out before drawer icons.
  - *Mixed* -- Folders and files are sorted together with no grouping.
  - *Grouped By Type* -- Icons are arranged in visual groups (Drawers, Tools, Projects, Other). Each group is laid out separately, with a configurable gap between them (see Advanced Settings).

  **Note:** When *Grouped By Type* is selected, the Sort By option is disabled because the group order is fixed. Icons within each group are still sorted by name.

- **Sort By** -- Selects the sort key within the chosen grouping mode.
  - *Name* -- Sort alphabetically. **(Default)**
  - *Type* -- Sort by file type.
  - *Date* -- Sort by modification date.
  - *Size* -- Sort by file size.

  This option is disabled when Grouping is set to *Grouped By Type*.

- **Include Subfolders** -- When enabled, iTidy also processes all subfolders under the selected folder. Useful for tidying a whole partition or a large folder tree in one pass. Default: Off.

- **Create Icons During Tidy** -- When enabled, iTidy creates new `.info` files for files and folders that don't already have them, using the DefIcons system. Files that gain new icons are then included in the layout. Configure what gets icons using the **Icon Creation...** button, including thumbnail icons.   Default: Off.

- **Back Up Layout Before Changes** -- When enabled, iTidy creates an LhA backup of each folder's `.info` files before making any changes. Requires LhA in `C:`. If LhA is not found when you click Start with this enabled, iTidy will warn you and offer to continue without backups or cancel. Default: Off.

  This checkbox is kept in sync with the menu item at Settings -> Backups -> Back Up Layout Before Changes.

- **Window Position** -- Controls where drawer windows are placed after iTidy resizes them.
  - *Center Screen* -- Centres the window on the Workbench screen. **(Default)**
  - *Keep Position* -- Keeps the window at its current location.
  - *Near Parent* -- Places the window slightly down and right of its parent (cascading style).
  - *No Change* -- Resizes the window but does not move it.

### Buttons

- **Advanced...** -- Opens the Advanced Settings window for finer control over layout, density, limits, columns, grouping gaps, and filtering.

- **Fix Default Tools...** -- Opens the Default Tool Analysis window, which scans icons for missing or invalid default tools. The scan targets the selected folder and respects the Include Subfolders setting.

- **Restore Backups...** -- Opens the Restore Backups window. This restores icon/layout backups only. To undo default tool changes, use the Restore Default Tools feature inside the Fix Default Tools window.

- **Icon Creation...** -- Opens the Icon Creation Settings window to configure thumbnail generation, text previews, folder icons, and DefIcons categories.

- **Start** -- Begins processing using the current settings. All current settings are saved to the active preferences before processing starts. A progress window opens showing status during the run.

- **Exit** -- Closes iTidy.

### Menus

#### Presets

**Reset To Defaults** (Amiga+N)
  Resets all settings to their default values. The current log level is preserved.

**Open Preset...** (Amiga+O)
  Loads a previously saved preferences file. The default location is `PROGDIR:userdata/Settings/`.

**Save Preset** (Amiga+S)
  Saves the current settings to the last-used file path. Behaves like Save Preset As if no file has been saved yet.

**Save Preset As...** (Amiga+A)
  Opens a requester to choose where to save. Default filename is `iTidy.prefs` in `PROGDIR:userdata/Settings/`.

**Quit** (Amiga+Q)
  Closes iTidy.

#### Settings

**Advanced Settings...**
  Opens the Advanced Settings window.

**DefIcons Categories...**
  Opens the DefIcons category selection window.

**Preview Icons...**
  Opens the Text Templates window.

**DefIcons Excluded Folders...**
  Opens the Exclude Paths window.

**Back Up Layout Before Changes?**
  Toggle checkmark. Kept in sync with the checkbox on the main window.

**Logging -- Disabled (Recommended)**
  Turns off all logging. **(Default)**

**Logging -- Debug**
  Most verbose level, logs everything.

**Logging -- Info**
  Logs informational messages and above.

**Logging -- Warning**
  Logs warnings and errors only.

**Logging -- Error**
  Logs errors only.

**Logging -- Open Log Folder**
  Opens `PROGDIR:logs/` as a Workbench drawer.

The five logging level items are mutually exclusive.

#### Tools

**Restore Layouts...**
  Opens the Restore Backups window (same as the Restore Backups button).

**Restore Default Tools...**
  Opens the Restore Default Tools window for reverting default tool changes.

#### Help

**iTidy Guide...**
  Opens `PROGDIR:iTidy2.guide` using AmigaGuide.

**About**
  Shows a requester with the version number, description, and copyright notice.

### Tips

- Save your preferred settings as a preset. Use the LOADPREFS ToolType to load them automatically every time iTidy starts.
- If you are experimenting, enable backups and start with a small test folder first.
- You can point iTidy at a whole partition (e.g. `Work:`) to process everything on that volume in one pass.

---

## Advanced Settings

Open this window with **Advanced...** on the main window, or via the Settings menu. Settings are organised into five tabs. Changes are applied when you click **OK**, or discarded if you click **Cancel** or press Escape. To save settings permanently, use Presets -> Save on the main window.

### Layout Tab

- **Layout Aspect** -- Sets the target width-to-height shape for drawer windows.
  - *Tall (0.75)* -- Tall, narrow windows.
  - *Square (1.0)* -- Roughly square windows.
  - *Compact (1.3)* -- Slightly wider than tall.
  - *Classic (1.0)* -- Traditional Workbench-like proportions.
  - *Wide (2.0)* -- Wider, landscape-oriented windows. **(Default)**

- **When Full** -- Controls what iTidy does when a drawer has more icons than fit at the target proportions.
  - *Expand Horizontally* -- Adds more columns. You will scroll left and right. **(Default)**
  - *Expand Vertically* -- Adds more rows. You will scroll up and down.
  - *Expand Both* -- Balances expansion in both directions.

- **Align Vertically** -- Sets how icons are aligned vertically when a row contains icons of different heights.
  - *Top* -- Aligns icons to the top of the row.
  - *Middle* -- Aligns icons to the middle. **(Default)**
  - *Bottom* -- Aligns icons to the bottom of the row.

### Density Tab

- **Horizontal Spacing** -- Horizontal gap between icons, in pixels. Range: 0 to 20. Default: 8.
- **Vertical Spacing** -- Vertical gap between icons, in pixels. Range: 0 to 20. Default: 8.

### Limits Tab

- **Icons Per Row: Min** -- Minimum number of columns in a drawer window. Prevents drawers from collapsing to a single-column list. Range: 0 to 30. Default: 2.

- **Auto-Calc Max Icons** -- When enabled, iTidy calculates the maximum columns automatically based on window width and screen size. Default: On. When this is on, the Max field below is disabled.

- **Max** -- Maximum number of columns. Only used when Auto-Calc Max Icons is off. Range: 2 to 40. Default: 10.

- **Max Window Width** -- Limits how wide drawer windows may become, as a percentage of screen width.
  - *Auto* -- No fixed limit.
  - *30%* -- Up to 30% of screen width.
  - *50%* -- Up to 50% of screen width.
  - *70%* -- Up to 70% of screen width. **(Default)**
  - *90%* -- Up to 90% of screen width.
  - *100%* -- Full screen width.

### Columns & Groups Tab

- **Column Layout** -- When enabled, icons are arranged in strict vertical columns. When disabled, a free-flow row-based layout is used. Default: Off.

- **Gap Between Groups** -- Controls spacing between icon groups when Grouping is set to *Grouped By Type*.
  - *Small* -- Small gap.
  - *Medium* -- Medium gap. **(Default)**
  - *Large* -- Large gap.

  **Note:** Groups with no icons are skipped and no gap is added for them.

### Filters & Misc Tab

- **Auto-Fit Columns** -- When enabled, each column is sized to fit its widest icon rather than forcing all columns to the same width. Produces a more compact layout. Default: On.

- **Reverse Sort Order** -- Reverses the sort direction (Z to A, newest first, or largest first, depending on the sort mode). Default: Off.

- **Strip Newicons Borders** -- Removes the icon border from NewIcons. Requires icon.library v44 or later.  Default: Off.

- **Skip Hidden Folders** -- When enabled, iTidy skips folders that do not have a corresponding `.info` file during recursive processing. Folders without `.info` files are not visible in Workbench. Default: On.

### Buttons

- **Defaults** -- Resets all settings in this window to factory defaults. A confirmation requester is shown first. Does not save anything to disk.
- **Cancel** -- Discards all changes and returns to the main window.
- **OK** -- Accepts the current settings and closes the window. Changes apply to the session but are not saved to disk.

### Default Values Summary

- **Layout Aspect** -- Wide (2.0)
- **When Full** -- Expand Horizontally
- **Align Vertically** -- Middle
- **Horizontal Spacing** -- 8
- **Vertical Spacing** -- 8
- **Icons Per Row: Min** -- 2
- **Auto-Calc Max Icons** -- On
- **Max** -- 10
- **Max Window Width** -- 70%
- **Column Layout** -- Off
- **Gap Between Groups** -- Medium
- **Auto-Fit Columns** -- On
- **Reverse Sort Order** -- Off
- **Strip NewIcon Borders** -- Off
- **Skip Hidden Folders** -- On

---

## Icon Creation Settings

Open this window with **Icon Creation...** on the main window. It controls how iTidy creates new icons for files and folders that do not already have `.info` files, using the DefIcons system. Settings are organised into three tabs: Create, Rendering, and DefIcons.

Changes are applied when you click **OK**, or discarded if you click **Cancel**.

### Create Tab

- **Folder Icons** -- Controls whether iTidy creates drawer icons for folders that don't have them.
  - *Smart (When Folder Has Icons)* -- Only creates a folder icon if the folder will end up showing something on Workbench after the run.
  - *Always Create* -- Creates a folder icon for every folder that is missing one.
  - *Never Create* -- Never creates folder icons. **(Default)**

  **How iTidy processes folders when "Include Subfolders" is on**

  When **Include Subfolders** is ticked on the main window, iTidy uses a depth-first strategy. Rather than processing the folder you pointed it at first and then stepping into its subfolders, it dives straight down to the deepest subfolder it can find before touching anything. Only once it has reached the bottom does it begin creating icons and tidying, then it gradually works its way back up through each parent folder until it finally reaches the top.

  This means that when you start a recursive run, iTidy may appear to do nothing visible for a while as it descends through the tree. It can end up very deep inside folder structures you might not even know are there. Once it starts working back out, you will see folders being processed in roughly bottom-up order -- the deepest nested folders first, then their parents, and so on.

  This approach is intentional. A folder can only be given a drawer icon once iTidy knows what is inside it, and on a recursive run it does not know that until the contents have been processed. By going deep first, each folder's icon decision is made with complete information as the recursion unwinds.

  **How Smart works**
  Smart mode is designed to avoid creating folder icons for folders that would still look “empty” on Workbench.

  - **If you are not processing subfolders (non-recursive):**  
    iTidy looks inside each subfolder (just the files in that folder, not deeper folders).  
    1) It first checks for any existing icons (`.info` files). If it finds one, it creates the folder icon straight away.  
    2) If there are no icons, it then checks the files in the folder to see if DefIcons would normally give any of them an icon. As soon as it finds one that would get an icon, it stops and creates the folder icon.

  - **If you are processing subfolders (recursive):**  
    iTidy does not do a separate “look ahead” scan. Instead, it processes the deepest folders first, creating icons as needed, then works back up.  
    When it returns to a folder, it creates that folder’s icon only if something inside ended up getting an icon (or was already visible).  
    This makes recursive Smart mode faster, because iTidy learns what’s inside while it’s doing the real work.

>   **Note:** Smart mode is quickest when iTidy can decide based on existing icons. If a folder has no icons at all, iTidy may need to ask DefIcons about many files to work out what would get an icon. On a large collection, that can mean a DefIcons ARexx call for every file in every subfolder, which can be slow on Classic hardware.

>   **Performance warning:** When Smart mode is combined with "Include Subfolders" on the main window, iTidy will recurse into every subfolder, sub-subfolder, and so on -- querying DefIcons for files at every level and then backtracking up through the entire folder tree to determine which folders need drawer icons. On deeply nested volumes or large collections this can take a very long time on Classic Amiga hardware. If processing time is a concern, use "Never Create" for folder icons, or limit the run to a single folder without recursion.

- **Skip Files In WHDLoad Folders** -- When enabled, folders containing a WHDLoad slave file (`*.slave`) are skipped during icon creation. The drawer icon for the WHDLoad folder itself is still created, but no icons are generated for files inside it. Default: Off.

- **Text File Previews** -- When enabled, iTidy renders a preview of a text file's contents onto the icon image. Default: On.

- **Manage Templates...** -- Opens the Text Templates window to view and edit the rendering templates used for text preview icons.

- **Picture File Previews** -- When enabled, iTidy generates thumbnail icons for recognised image files. Default: On.

  Individual image formats can be enabled or disabled separately:

  - **ILBM (IFF)** -- On
  - **PNG** -- On
  - **GIF** -- On
  - **JPEG (Slow)** -- Off
  - **BMP** -- On
  - **ACBM** -- On
  - **Other** -- On

  **Note:** JPEG is disabled by default because JPEG decoding is computationally expensive on 68k hardware. Enabling it will significantly slow down icon creation on classic Amiga hardware.

- **Replace Existing Image Thumbnails** -- When enabled, iTidy deletes and recreates image thumbnail icons that it previously created (identified by the `ITIDY_CREATED` ToolType). User-placed icons are never affected. Default: Off.

- **Replace Existing Text Previews** -- When enabled, iTidy deletes and recreates text preview icons that it previously created. Only icons with the `ITIDY_CREATED` ToolType are affected. Default: Off.

### Rendering Tab

These settings control the visual appearance of generated thumbnail icons.

- **Preview Size** -- Sets the canvas size for thumbnail icons.
  - *Small (48x48)* -- Compact thumbnails.
  - *Medium (64x64)* -- Balanced size and detail. **(Default)**
  - *Large (100x100)* -- Largest thumbnails with the most detail.

- **Thumbnail Border** -- Controls the border style drawn around thumbnails.
  - *None* -- No border.
  - *Workbench (Smart)* -- Classic Workbench frame, skipped for images with transparency.
  - *Workbench (Always)* -- Classic Workbench frame, always applied.
  - *Bevel (Smart)* -- Inner highlight bevel, skipped for images with transparency. **(Default)**
  - *Bevel (Always)* -- Inner highlight bevel, always applied.

  The "Smart" modes detect transparent images and skip the border, avoiding borders around irregularly shaped images.

- **Upscale Small Images** -- When enabled, images smaller than the chosen preview size are scaled up to fill the thumbnail area. When disabled, small images are centred at their original size. Default: Off.

- **Max Colours** -- Sets the maximum number of colours in generated icons.
  - *4 colours* -- Very limited palette.
  - *8 colours* -- Basic colour range.
  - *16 colours* -- Moderate colour range.
  - *GlowIcons palette (29 colours)* -- Standard GlowIcons palette for maximum compatibility.
  - *32 colours* -- Good colour range.
  - *64 colours* -- Rich colour range.
  - *128 colours* -- Near-photographic quality.
  - *256 colours (full)* -- Full 256-colour palette. **(Default)**
  - *Ultra (256 + detail boost)* -- 256 colours with enhanced detail processing.

  When Max Colours is set to 256 or Ultra, the Dithering option is disabled. When Max Colours is above 8 (or set to GlowIcons palette or Ultra), the Low-Colour Palette option is disabled.

- **Dithering** -- Selects the dithering method used when reducing colours.
  - *None* -- Colours are mapped directly.
  - *Ordered (Bayer 4x4)* -- Systematic dithering pattern, fast and predictable.
  - *Error Diffusion (Floyd-Steinberg)* -- Diffuses quantisation error to neighbouring pixels. Smoother gradients but can be slower.
  - *Auto (Based On Colour Count)* -- Automatically selects the best method. **(Default)**

  Disabled when Max Colours is 256 or Ultra.

- **Low-Colour Palette** -- Controls the palette used at very low colour counts (4 or 8 colours).
  - *Greyscale* -- Maps to a greyscale palette. **(Default)**
  - *Workbench Palette* -- Maps to the standard Workbench palette.
  - *Hybrid (Grays + WB Accents)* -- Mostly greyscale with a few Workbench accent colours.

  Only enabled when Max Colours is set to 4 or 8.

### DefIcons Tab

- **Icon Creation Setup...** -- Opens the DefIcons Categories window to select which file types should receive icons during processing.
- **Exclude Paths...** -- Opens the Exclude Paths window to manage folders that should be skipped during icon creation.

### Default Values Summary

- **Folder Icons** -- Never Create
- **Skip WHDLoad Folders** -- Off
- **Text File Previews** -- On
- **Picture File Previews** -- On
- **ILBM (IFF)** -- On
- **PNG** -- On
- **GIF** -- On
- **JPEG (Slow)** -- Off
- **BMP** -- On
- **ACBM** -- On
- **Other** -- On
- **Replace Image Thumbnails** -- Off
- **Replace Text Previews** -- Off
- **Preview Size** -- Medium (64x64)
- **Thumbnail Border** -- Bevel (Smart)
- **Upscale Small Images** -- Off
- **Max Colours** -- 256 colours (full)
- **Dithering** -- Auto
- **Low-Colour Palette** -- Greyscale

---

## DefIcons Categories

Open this window from the main window via Settings -> DefIcons Categories... -> DefIcons Categories..., or from Icon Creation Settings -> DefIcons tab -> Icon Creation Setup....

This window lets you choose which file types iTidy should create icons for. It shows a tree view of all file types defined in the system's DefIcons preferences, with checkboxes to enable or disable each type.

The type tree is read from `ENV:deficons.prefs`, managed by the "DefaultIcons" Preferences tool in the Workbench Prefs drawer. If the DefIcons system is not available, this window cannot be used.

### Type Tree

The tree shows a hierarchical list of file types. Top-level entries are categories (music, picture, tool, etc.). Expanding a category reveals the individual file types within it. Only second-level file types have checkboxes -- root categories and deeper sub-types do not.

- A checked type means iTidy will create icons for files of that type.
- An unchecked type means iTidy will skip that type during icon creation.

The following categories are disabled by default because creating icons for them is rarely useful or could affect system files:

- **tool** -- Executable programs (most already have icons)
- **prefs** -- Preferences files
- **iff** -- Generic IFF files
- **key** -- Keyfiles
- **kickstart** -- Kickstart ROM images

All other types are enabled by default.

### Buttons

- **Select All** -- Enables all file types. Use with care -- this may create icons for system files that don't ordinarily have them.
- **Select None** -- Disables all file types. No icons will be created for any type.
- **Show Default Tools** -- Scans `ENVARC:Sys/` and displays the default tool assigned to each type next to its name (e.g. `mod  [EaglePlayer]`). This is a one-shot action that updates automatically after you change a default tool.
- **Change Default Tool...** -- Opens a file requester to assign a new default tool to the selected type. The change is written directly to the type's template icon in `ENVARC:Sys/`. If the selected type does not yet have its own template icon, iTidy creates one by cloning the parent's template first.

  **Warning:** Default tool changes are applied immediately and saved to disk. They are not subject to the OK/Cancel buttons.

- **OK** -- Accepts the current checkbox selections and closes the window.
- **Cancel** -- Closes the window without saving checkbox changes. Default tool changes already made via "Change Default Tool..." are not affected by Cancel.

---

## Text Templates

Open this window from the main window via Settings -> DefIcons Categories... -> Preview Icons..., or from Icon Creation Settings -> Create tab -> Manage Templates....

This window manages the DefIcons text preview templates. Each ASCII sub-type (C source, REXX, HTML, etc.) can have its own template icon (`def_<type>.info`) stored in `PROGDIR:Icons/`. These templates contain ToolTypes that control how iTidy renders text previews onto file icons.

### Master and Custom Templates

- The **master template** is `def_ascii.info`. It is the fallback used for all ASCII sub-types that don't have their own template. It always appears first in the list with status "Master".
- A **custom template** is a type-specific file such as `def_c.info` or `def_rexx.info`. Status shows "Custom".
- If no custom template exists for a sub-type, it falls back to the master. Status shows "Using master".

### Excluded Types

The master template (def_ascii.info) supports an EXCLUDETYPE ToolType that lists file sub-types that iTidy will never create icons for. These types are shown with the status Excluded.

Example:

EXCLUDETYPE=amigaguide,html,install

To re-enable a type, remove it from the EXCLUDETYPE list by using Edit ToolTypes on the master template row. This opens the Workbench Information editor for def_ascii.info, where the change is saved.

### Template ToolTypes

Custom templates can contain the following `ITIDY_*` ToolTypes to control rendering. Defaults are used for any ToolType that is absent.

**Note:** The text renderer is a micro-preview renderer, not a text layout engine. It fits an entire file's worth of text into an icon a few dozen pixels across by treating each character as a single pixel-width column and each line as a single pixel-tall band. The result looks like ruled-paper marks that suggest text density and structure, not readable characters. All the size and spacing ToolTypes operate in this output-pixel coordinate space, not in font or point sizes.

**`ITIDY_TEXT_AREA`** (Default: 4-pixel margin)
  Rectangle (x,y,w,h) defining the safe zone where text is drawn.

**`ITIDY_EXCLUDE_AREA`** (Default: none)
  Rectangle (x,y,w,h) area to skip pixel-by-pixel when drawing.

**`ITIDY_LINE_HEIGHT`** (Default: 1)
  Output height in pixels of each rendered line band. At the default of 1, a 60px-tall safe area holds 60 bands. Increase to 2 or 3 for thicker bands and fewer lines per icon.

**`ITIDY_LINE_GAP`** (Default: 1)
  Gap between line bands in pixels *(reserved -- not currently used by the renderer)*.

**`ITIDY_MAX_LINES`** (Default: auto)
  Maximum number of rendered line bands to produce *(reserved -- not currently enforced by the renderer)*.

**`ITIDY_CHAR_WIDTH`** (Default: 0/auto)
  Horizontal pixel step per rendered character (1 = narrow, 2 = normal); 0 = auto-select based on safe area width.

**`ITIDY_READ_BYTES`** (Default: 4096)
  Number of bytes to read from the source file.

**`ITIDY_BG_COLOUR`** (Default: none -- no fill)
  Background colour index (0-254) to fill the safe area; absent or -1 = no fill (preserve template pixels). Index 255 is reserved internally and cannot be used.

**`ITIDY_TEXT_COLOUR`** (Default: auto)
  Text colour index (0-255); ignored when `ITIDY_ADAPTIVE_TEXT=YES`.

**`ITIDY_MID_COLOUR`** (Default: auto)
  Mid-tone colour index (0-255); ignored when `ITIDY_ADAPTIVE_TEXT=YES`.

**`ITIDY_DARKEN_PERCENT`** (Default: 70)
  Darkening strength for even output rows (1-100); only active when `ITIDY_ADAPTIVE_TEXT=YES`.

**`ITIDY_DARKEN_ALT_PERCENT`** (Default: 35)
  Darkening strength for odd output rows (1-100); only active when `ITIDY_ADAPTIVE_TEXT=YES`.

**`ITIDY_ADAPTIVE_TEXT`** (Default: NO)
  Master switch for the adaptive rendering path (YES/NO).

**`ITIDY_EXPAND_PALETTE`** (Default: YES)
  Pre-adds darkened colour variants derived from the existing palette for smoother tones; only active when `ITIDY_ADAPTIVE_TEXT=YES`.

Editing ToolTypes opens the Workbench Information editor via ARexx. Changes are saved there, not within iTidy itself.

#### The two rendering modes: fixed colour vs adaptive

**`ITIDY_ADAPTIVE_TEXT`** is the master switch that divides the ToolTypes into two groups. Getting this right is probably the most important thing to understand about the template system.

**When `ITIDY_ADAPTIVE_TEXT=NO` (the default):**
- Glyph pixels are painted with fixed palette indices: `ITIDY_TEXT_COLOUR` for medium- and high-density pixels, `ITIDY_MID_COLOUR` for sparse and edge pixels.
- `ITIDY_DARKEN_PERCENT`, `ITIDY_DARKEN_ALT_PERCENT`, and `ITIDY_EXPAND_PALETTE` are all ignored entirely.
- If `ITIDY_TEXT_COLOUR` or `ITIDY_MID_COLOUR` are absent, the renderer auto-detects appropriate palette entries (see below).

**When `ITIDY_ADAPTIVE_TEXT=YES`:**
- `ITIDY_TEXT_COLOUR` and `ITIDY_MID_COLOUR` are ignored for glyph painting. Instead, the renderer samples the background pixel at each position and darkens it. The text inherits the colour of whatever the template artwork has underneath it, which works particularly well with detailed or colourful template art.
- `ITIDY_DARKEN_PERCENT` (default 70%) controls how strongly even output rows are darkened.
- `ITIDY_DARKEN_ALT_PERCENT` (default 35%) controls darkening for odd output rows, producing a lighter "ruled paper" stripe effect.
- `ITIDY_EXPAND_PALETTE` becomes active (see below).
- `ITIDY_BG_COLOUR` still controls whether the safe area is filled before rendering. When absent (the default), no fill happens -- the template artwork pixels are preserved, which is exactly what you need. Pairing a solid fill with adaptive mode defeats the purpose: once every pixel is the same colour, the text just darkens to one result, no different from fixed colour mode.

At a glance:

**`ITIDY_TEXT_COLOUR`**
  ITIDY_ADAPTIVE_TEXT=NO: Fixed colour for medium/high-density glyph pixels
  ITIDY_ADAPTIVE_TEXT=YES: **Ignored**

**`ITIDY_MID_COLOUR`**
  ITIDY_ADAPTIVE_TEXT=NO: Fixed colour for sparse/edge glyph pixels
  ITIDY_ADAPTIVE_TEXT=YES: **Ignored**

**`ITIDY_DARKEN_PERCENT`**
  ITIDY_ADAPTIVE_TEXT=NO: **Ignored**
  ITIDY_ADAPTIVE_TEXT=YES: Darkening strength for even rows (default 70%)

**`ITIDY_DARKEN_ALT_PERCENT`**
  ITIDY_ADAPTIVE_TEXT=NO: **Ignored**
  ITIDY_ADAPTIVE_TEXT=YES: Darkening strength for odd rows (default 35%)

**`ITIDY_EXPAND_PALETTE`**
  ITIDY_ADAPTIVE_TEXT=NO: **Ignored**
  ITIDY_ADAPTIVE_TEXT=YES: Adds darkened palette variants for smoother tone gradations

**`ITIDY_BG_COLOUR`**
  ITIDY_ADAPTIVE_TEXT=NO: No fill (template pixels preserved)
  ITIDY_ADAPTIVE_TEXT=YES: Same default -- no fill. A solid fill defeats adaptive mode; leave absent or set to -1.

#### `ITIDY_EXPAND_PALETTE` -- depends on `ITIDY_ADAPTIVE_TEXT`

When palette expansion is active (`ITIDY_ADAPTIVE_TEXT=YES` and `ITIDY_EXPAND_PALETTE` not set to `NO`), darkened colour variants are derived from the existing palette and added to it, giving the darken tables more entries to map to and producing smoother tone gradations. It works in two phases: first a 70%-darkened copy of every palette colour is added (used for even output rows), then a 35%-darkened copy (used for odd/alternate rows). Entries that are already close enough to an existing colour are reused rather than duplicated, so no colour slots are wasted. No neutral grey values are inserted -- all new entries come from the template's own colours.

Expansion only fires when the palette has fewer than 128 colours and at least 2. If `ITIDY_ADAPTIVE_TEXT=NO`, this ToolType has no effect regardless of its value.

#### `ITIDY_BG_COLOUR` -- absent means no fill

When `ITIDY_BG_COLOUR` is absent (or set to `-1`), the safe area is left exactly as it is and glyph pixels are painted on top of whatever template artwork is already there. This is the default behaviour.

This means adaptive mode works correctly without setting `ITIDY_BG_COLOUR` at all -- just leave it out. You only need to set it if you want a solid fill, and you would not normally want that with `ITIDY_ADAPTIVE_TEXT=YES`, because once the safe area is filled with one colour, every glyph pixel darkens to the same result, which is no different from fixed colour mode.

Setting `ITIDY_BG_COLOUR` to a valid palette index (0-254) forces a solid fill. Index 255 is reserved internally as the "no fill" sentinel and cannot be used as a colour. Setting it to any value outside the valid palette range (other than 255, which is always treated as no-fill) triggers auto-detection of the lightest palette entry, which is also a solid fill.

#### Colour auto-detection

`ITIDY_TEXT_COLOUR` and `ITIDY_MID_COLOUR` (fixed colour mode only) auto-detect when absent or out-of-range. `ITIDY_BG_COLOUR` behaves differently -- see below.

- **Text** (`ITIDY_TEXT_COLOUR` absent): darkest palette entry
- **Mid-tone** (`ITIDY_MID_COLOUR` absent): palette entry closest to the midpoint between the darkest and lightest entries, excluding those two extremes
- **Background** (`ITIDY_BG_COLOUR` absent): no fill -- template pixels are preserved. This differs from the other two: absent does not trigger a palette scan, it disables the fill entirely. Auto-detection of the lightest entry only happens if `ITIDY_BG_COLOUR` is set to an out-of-range value.

A safety check ensures the text and mid-tone colours never resolve to the same palette index.

#### `ITIDY_TEXT_AREA` and `ITIDY_EXCLUDE_AREA` -- independent zones

These two ToolTypes are checked separately and serve different purposes:

- **`ITIDY_TEXT_AREA`** sets the safe rendering zone. The renderer fills the background and draws all text within this rectangle. If absent, the zone defaults to the icon bounds minus a 4-pixel margin on all sides.
- **`ITIDY_EXCLUDE_AREA`** defines a per-pixel skip zone. Any individual pixel that falls inside this rectangle is not written, regardless of the text area. This is useful for preserving decorative template artwork such as a folded-corner graphic. It works in both adaptive and non-adaptive modes.

The two zones are independent -- the exclude area can overlap the text area, lie entirely within it, or be placed outside it entirely.

#### `ITIDY_CHAR_WIDTH` -- auto-select mode

Setting `ITIDY_CHAR_WIDTH=0` (the default) enables automatic selection: 2 pixels per character column if the safe area is 64 pixels wide or greater; 1 pixel per character column if it is narrower. Any non-zero value overrides this.

#### `ITIDY_MAX_LINES` and `ITIDY_LINE_GAP` -- reserved for future use

These ToolTypes are recognised by the Validate function and will not be flagged as unknown keys, but the renderer does not currently use them. `ITIDY_MAX_LINES` is calculated automatically from the safe area height divided by `ITIDY_LINE_HEIGHT`. `ITIDY_LINE_GAP` is parsed but the rendering loop does not read it -- band spacing is controlled solely by `ITIDY_LINE_HEIGHT`. Setting either will not cause errors but will have no visible effect.

### Gadgets

- **Show** -- Filters which types appear in the list.
  - *All* -- Shows every sub-type. **(Default)**
  - *Custom Only* -- Shows only types with a custom template.
  - *Missing Only* -- Shows only types without a custom template.

  The master row ("ascii") always appears regardless of the filter.

- **Template list** -- Three-column list showing Type, Template filename, and Status for each sub-type. Select a row to see details and enable the action buttons.

- **Selected Type info panel** -- Read-only fields showing: Type, Template file, Effective file (which template will actually be used at runtime), and Status.

### Action Buttons

All action buttons start disabled and enable when a row is selected.

- **Create from master / Overwrite from master** -- Copies `def_ascii.info` to create a new custom template for the selected type. The label changes to "Overwrite from master" if a custom template already exists. A confirmation is shown before overwriting. Disabled when the master is selected (cannot copy the master onto itself).

- **Edit tooltypes...** -- Opens the effective template icon in the Workbench Information editor. For sub-types without a custom template, this opens the master `def_ascii.info`.

- **Validate tooltypes** -- Reads all ToolTypes from the effective template and checks for unknown `ITIDY_*` keys, colour values outside 0-255 (or outside 0-254 for `ITIDY_BG_COLOUR`), percentage values outside 1-100, and formatting errors. Results are shown in a requester with an "Effect Summary" of all collected values.

- **Revert to master** -- Deletes the custom `def_<type>.info` so the sub-type falls back to the master. A confirmation is shown first. Disabled when the master is selected or no custom template exists.

- **Close** -- Closes the window (keyboard shortcut: C).

---

## Exclude Paths

Open this window from the main window via Settings -> DefIcons Categories... -> DefIcons Excluded Folders..., or from the DefIcons tab in Icon Creation Settings.

This window manages the list of folder paths skipped during DefIcons icon creation. Any folder matching an exclude path is skipped entirely -- no icons are generated inside it.

### DEVICE: Placeholder

Paths can use DEVICE: as a placeholder for the volume currently being scanned. At scan time, iTidy replaces DEVICE: with the scanned volume name, so the exclude list can be reused across different drives.

For example, if you are scanning Workbench:Games/, then DEVICE:C means Workbench:C. If you are scanning Work:WHDLoad/, the same entry means Work:C.

When you add or modify a path, if the selected folder is on the same volume as the current scan folder, iTidy asks whether to store it as DEVICE: (portable) or as an absolute path (fixed to that specific volume).


### Limits

- Maximum 32 exclude paths
- Each path up to 255 characters long

### Gadgets

- **Path list** -- Shows all configured exclude paths. Click a row to select it for Remove or Modify.

- **Add...** -- Opens a directory requester. After selecting a folder, iTidy asks whether to store it as a portable `DEVICE:` pattern or absolute path. Duplicate paths are silently rejected.

- **Remove** -- Removes the selected path. Does nothing if no row is selected.

- **Modify...** -- Opens a directory requester pre-set to the selected path. Same `DEVICE:`/absolute logic as Add. Does nothing if no row is selected.

- **Reset to Defaults** -- After a confirmation, replaces the current list with the following 17 default AmigaOS system directory paths:

  `DEVICE:Fonts`, `DEVICE:Locale`, `DEVICE:Classes`, `DEVICE:Libs`, `DEVICE:C`, `DEVICE:Rexxc`, `DEVICE:T`, `DEVICE:L`, `DEVICE:Devs`, `DEVICE:Resources`, `DEVICE:System`, `DEVICE:Storage`, `DEVICE:Expansion`, `DEVICE:Kickstart`, `DEVICE:Env`, `DEVICE:Envarc`, `DEVICE:Prefs/Env-Archive`

- **OK** -- Accepts the current list and closes the window.

- **Cancel** -- Discards changes and closes the window.

---

## Default Tool Analysis

Open this window with **Fix Default Tools...** on the main window.

### What is a Default Tool?

Most data file icons on the Amiga have a "Default Tool" -- the program Workbench runs when you double-click that icon. After copying icons between systems or extracting older archives, those tool paths often break, resulting in "Unable to open your tool" errors.

### Using the Scanner

1. The **Folder** field initialises from the current scan folder in your preferences. Change it if needed.

2. Click **Scan** to start a recursive scan. A progress window shows scan statistics. After scanning, close the progress window to return to the tool list.

3. Use the **Filter** chooser to narrow the list: *Show All*, *Show Valid Only*, or *Show Missing Only*.

4. Click a tool in the upper list to see which icons reference it in the details panel.

5. To fix a tool, select it and use **Replace Tool (Batch)...** to update all icons that reference it, or select a specific icon file in the details panel and use **Replace Tool (Single)...** to update just that one.

### Tool List Columns

**Tool**
  Tool name or path.

**Files**
  Number of icons that reference this tool.

**Status**
  EXISTS or MISSING.

Click a column header to sort. Click again to reverse the sort order.

### Details Panel

Shows information about the selected tool: tool name, status, version string if available, and the list of icon files that reference it. If a tool has more than 200 referencing files, only the first 200 are shown.

### Buttons

- **Scan** -- Starts the recursive directory scan. Always scans recursively regardless of the Include Subfolders setting on the main window.
- **Replace Tool (Batch)...** -- Opens the Replace Default Tool window to update all icons referencing the selected tool. Only enabled when a tool is selected in the upper list.
- **Replace Tool (Single)...** -- Opens the Replace Default Tool window for one specific icon. Only enabled when an icon file row is selected in the details panel.
- **Restore Default Tools Backups...** -- Opens the Restore Default Tools window. If any restores are performed, the cache is cleared and you will be prompted to re-scan.
- **Close** -- Closes the window.

### Menus

#### Project Menu

**New** (Amiga+N)
  Clears the tool cache and resets the display.

**Open...** (Amiga+O)
  Loads a previously saved tool cache from a `.dat` file.

**Save** (Amiga+S)
  Saves the cache to `PROGDIR:userdata/DTools/toolcache.dat`.

**Save as...** (Amiga+A)
  Saves the cache to a user-chosen location.

**Close** (Amiga+C)
  Closes the window.

#### File Menu

**Export list of tools** (Amiga+T)
  Exports a text report of all tools.

**Export list of files and tools** (Amiga+F)
  Exports a detailed report of all files and their tools.

#### View Menu

**System PATH...** (Amiga+P)
  Shows the list of directories iTidy searches for default tools.

### Notes

- Saving and loading the tool cache lets you preserve scan results between sessions without re-scanning large volumes.
- The Export menus create text reports useful for documenting which tools are used across a volume.
- After using Restore Default Tools Backups, the tool cache is cleared and must be rebuilt with a new scan.

---

## Replacing Default Tools

This window opens from **Replace Tool (Batch)...** or **Replace Tool (Single)...** in the Default Tool Analysis window.

### Batch vs Single Mode

In **batch mode** (opened via Replace Tool (Batch)), all icons that reference the selected tool are updated at once. The Mode field shows "Batch Mode: N icon(s) will be updated".

In **single mode** (opened via Replace Tool (Single)), only the one icon selected in the details panel is updated. The Mode field shows the icon file path.

### Using the Window

1. The **Current Tool** field shows the tool being replaced.
2. Click the browse button next to **Change To** to select the replacement tool program. The initial location is `C:`. `.info` files cannot be selected.
3. Click **Update Default Tool** to start the replacement.
4. Results appear in the progress list as each icon is processed: SUCCESS, FAILED, or READ-ONLY.
5. After completion, a summary shows success and failure counts. The Update button is disabled after one use -- close and reopen to do another update.
6. Click **Close** to return to the Default Tool Analysis window, which refreshes automatically.

**Tip:** Leave the Change To field empty and click Update to remove the default tool from the icon(s) entirely. A confirmation requester will be shown first.

**Note:** Write-protected icons are automatically skipped with a "READ-ONLY" status and are not counted as failures.

### Backup System

If default tool backup is enabled, every change is recorded in `PROGDIR:Backups/tools/YYYYMMDD_HHMMSS/`. The session folder contains `session.txt` (metadata) and `changes.csv` (one row per icon: icon path, old tool, new tool). Use the Restore Default Tools window to revert changes.

---

## Restoring Default Tool Backups

Open this window from the Default Tool Analysis window via **Restore Default Tools Backups...**, or from the main window via Tools -> Restore Default Tools....

This window lists all backup sessions created when iTidy replaced default tools. Each session represents one Replace operation.

### Session List

**Date/Time**
  When the backup was created.

**Mode**
  "Batch" or "Single".

**Path**
  The folder path that was being processed.

**Changed**
  Number of icons that were modified.

Click a session to see its tool changes in the panel below.

### Changes Panel

Shows the tool changes recorded in the selected session -- the original tool, the replacement, and how many icons were affected by each specific old->new pair.

### Buttons

- **Restore** -- Restores all icons in the selected session to their original default tools. A confirmation requester is shown with the icon count. The backup session is kept after restoring. Only enabled when a session is selected.

- **Delete** -- Permanently deletes the selected backup session and all its files. A confirmation is shown first. Deleted sessions cannot be recovered. Only enabled when a session is selected.

- **Close** -- Closes the window.

**Note:** After restoring, the tool cache in the Default Tool Analysis window is invalidated and must be rebuilt with a new scan.

---

## Restoring Layout Backups

Open this window with **Restore Backups...** on the main window, or via Tools -> Restore -> Restore Layouts....

This window lists all iTidy backup runs created when tidying with **Back Up Layout Before Changes** enabled. Each run is an LhA-based snapshot of the `.info` files and window positions for the processed folders.

**Note:** This restores icon/layout backups only. To undo default tool changes, use the Restore Default Tools window instead.

### Run List

**Run**
  Run number (e.g. 0007).

**Date/Time**
  When the backup was created.

**Folders**
  Number of folder archives in the run.

**Size**
  Total size of all archives.

**Icons+**
  Number of icons created by DefIcons during this run ("-" if none).

**Status**
  "Complete" (has catalog) or "NoCAT" (no catalog file).

Click a run to select it and update the details panel. Double-click a run to open the Folder View window (requires a catalog file). The first run is automatically selected when the window opens.

### Details Panel

Shows: Run Number, Date Created, Source Directory, Total Archives, Total Size, Icons Created, Status, and the on-disk location.

### Buttons

- **Delete Run** -- Permanently deletes the selected run and all its files. A confirmation is shown first. Cannot be undone.

- **Restore Run** -- Restores the selected run using LhA. A requester gives three choices:
  - *With Windows* -- Restores icons AND window positions/sizes.
  - *Icons Only* -- Restores only icon files without changing window geometry.
  - *Cancel* -- Aborts.

  If the run included icons created by DefIcons, the requester notes that those icons will also be removed during restoration. A progress window shows per-folder extraction progress.

  **Requires LhA:** If LhA is not found in `C:`, an error requester is shown.

- **View Folders...** -- Opens the Folder View window for the selected run. Only enabled when the run has a catalog file ("Complete" status). Also opened by double-clicking the run.

- **Close** -- Closes the window.

### Notes

- Restoring overwrites current `.info` files. Any changes made after the original backup will be lost.
- "NoCAT" (orphaned) runs can be deleted but the View Folders button will be disabled for them.
- Run numbers may have gaps (e.g. Run_0001, Run_0003) if intermediate runs were deleted.
- Backups are stored in `PROGDIR:Backups/`.

---

## Folder View

This read-only window opens from the Restore Backups window via **View Folders...** or by double-clicking a run.

It shows the hierarchical folder structure of a backup run -- which folders were backed up, their compressed archive sizes, and how many icons each contained.

The tree starts **fully collapsed**. Click the disclosure triangles to expand branches. Two columns are shown: the folder name (with indentation showing depth), and a combined Size/Icons value (e.g. "11 KB / 15 icons").

Selecting a tree node has no effect -- this window is informational only. Click **Close** (or press C) to return to the Restore Backups window.

---

## ToolTypes

These ToolTypes are read from the iTidy program icon when launched from Workbench.

**DEBUGLEVEL** (Values: 0-4, Default: 4/Disabled)
  Log level. 0=Debug, 1=Info, 2=Warning, 3=Error, 4=Disabled. Overrides any log level stored in a loaded preferences file.

**LOADPREFS** (Values: File path, Default: none)
  Automatically loads a saved preferences file at startup. Paths without a device name are resolved relative to `PROGDIR:userdata/Settings/`.

**PERFLOG** (Values: YES / NO, Default: NO)
  Enables performance timing logs for benchmarking.

---

## Tips & Troubleshooting

### General Tips

- **Enable backups for big runs.** If you are tidying lots of drawers with recursion enabled, turning on backups gives you a way back if you do not like the result. (Requires `LhA` in `C:` or on your command path.)
- **Start small.** Try a single drawer first so you can see how the settings affect layout before pointing iTidy at a whole partition.
- **Save your settings.** Once you have a setup you like, use Presets -> Save As so you can load it again later. Use the `LOADPREFS` ToolType to load it automatically at startup.
- **Use a test drawer.** Tidy one small drawer repeatedly while you adjust settings. Close and reopen it after each run to see snapshot changes.
- **To see window changes clearly,** close any drawers you are tidying before you run iTidy. After it finishes, you may need to close and reopen the drawers (or restart Workbench) to see updated window sizes and positions.

If something does not look right after a run, check the sections below.

### Icons Appear Misaligned

If some icons look misaligned while others look fine, it is often an `icon.library` issue. On setups with an older `icon.library`, OS3.5+ colour icons may not render at their real size and can appear as tiny placeholder images. iTidy positions icons using the icon's real dimensions, but Workbench may draw them at the wrong size, so the layout looks off.

**What to do:**
- Update `icon.library` to v44+ if you want to use OS3.5+ colour icons.
- Alternatively, convert colour icons back to NewIcons or classic icons if you are keeping a stock WB 3.0/3.1 setup.

### Window Positions Not Updating

If window positions do not change after a run or restore, this is usually Workbench caching.

**What to do:**
- Close and reopen the affected drawers.
- If that does not help, restart Workbench or reboot.

### Backup or Restore Not Working

A few things to check:

- `LhA` is installed in `C:` (or otherwise available on your command path).
- You have enough free disk space for archives (when backing up) or extraction (when restoring).
- The original drawer paths still exist when restoring.
- Check `PROGDIR:logs/` for error details.

### Slow Processing on Large Drawer Trees

Recursing through thousands of drawers takes time on real hardware, especially with mechanical drives.

- It is fine to leave iTidy running unattended during large jobs.
- Avoid running other disk heavy tasks at the same time.
- For large collections, it can be quicker to work in chunks (for example `WHDLoad:Games` first, then `WHDLoad:Demos`) rather than the entire volume at once.

### Icons Are Not Moved, and the Progress Log Shows No Icons Found

Check the drawer in Workbench and see if Window -> Show -> All Files is selected. This option shows contents using temporary icons that are not backed by `.info` files, so iTidy has nothing to move.

If you want iTidy to process this drawer, create real icons first: select the contents via Window -> Select Contents, then choose Icons -> Snapshot. Real icons will be created and iTidy will be able to process them.

### iTidy Skips Some Drawers That Contain Valid Icons

By default, iTidy skips drawers without an `.info` file during recursive processing, because those drawers are not visible in Workbench.

To bypass this, open Advanced Settings and untick **Skip Hidden Folders**.

### Icons Do Not Open ("Unable to open your tool")

This means an icon's Default Tool points to something that is not installed.

**What to do:**
- Use **Fix Default Tools...** to scan for missing tools and repair them.
- See the Default Tool Analysis section for details on the scanner.

### DefIcons Icon Creation Produces No Icons

Check the following:

- **Create Icons During Tidy** is enabled on the main window.
- The target file type is not disabled in the DefIcons Categories window.
- The target drawer is not in the Exclude Paths list.
- `ENV:deficons.prefs` is present and valid (managed by the DefaultIcons Preferences tool in the Workbench Prefs drawer).

### Text Preview Icons Look Wrong or Are Missing

- Check that **Text File Previews** is enabled in Icon Creation Settings -> Create tab.
- Make sure the master template `PROGDIR:Icons/def_ascii.info` exists.
- Use the Text Templates window to validate the ToolTypes in the relevant template.
- If a sub-type appears as "Excluded" in the templates list, edit the `EXCLUDETYPE` ToolType in `def_ascii.info` to remove it (select the master row, then use **Edit ToolTypes**).
---

## Credits & Version Info

**Author:** Kerry Thompson
**Version:** 2.0
**Website:** https://github.com/Kwezza/iTidy
**Special thanks:** Darren "dmcoles" Cole for ReBuild, an excellent GUI builder that made iTidy's updated interface possible.
---
