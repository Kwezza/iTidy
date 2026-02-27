# iTidy v2.0

**A Workbench Icon & Window Tidy Tool for AmigaOS 3.2**

iTidy is a Workbench utility I wrote after getting fed up with the mess left behind by large archive extractions - especially when unpacking thousands of WHDLoad games and demos into drawer trees like `Games/AGA/A/`, `Games/AGA/B/`, and so on. The aim was simple: point it at a folder, let it recurse through everything underneath, and have it tidy icon layouts and drawer windows in a consistent way.

Version 2.0 adds a Workbench 3.2 native ReAction interface, an icon creation system built on DefIcons (thumbnails for images and text files, configurable per file type), and expanded default tool management.

iTidy works by updating Workbench `.info` files and drawer layout information only - it doesn't touch the contents of your data files. If you enable backups, iTidy can create LhA restore points so you can roll back icon and layout changes later.


## Table of Contents

1. [Requirements](#requirements)
2. [Introduction](#introduction)
3. [What's New in v2.0](#whats-new-in-v20)
4. [Getting Started](#getting-started)
5. [Main Window](#main-window)
6. [Advanced Settings](#advanced-settings)
7. [Icon Creation Settings](#icon-creation-settings)
8. [DefIcons Categories](#deficons-categories)
9. [Text Templates](#text-templates)
10. [Exclude Paths](#exclude-paths)
11. [Default Tool Analysis](#default-tool-analysis)
12. [Replacing Default Tools](#replacing-default-tools)
13. [Restoring Default Tool Backups](#restoring-default-tool-backups)
14. [Restoring Layout Backups](#restoring-layout-backups)
15. [Folder View](#folder-view)
16. [ToolTypes](#tooltypes)
17. [Tips & Troubleshooting](#tips--troubleshooting)
18. [Credits & Version Info](#credits--version-info)

---

## Requirements

iTidy requires the following to run properly:

- AmigaOS Workbench 3.2 or newer
- 68000 CPU or better
- At least 1 megabyte (1 MB) of free memory
- At least 1 megabyte (1 MB) of free storage space in the installation location (more as backups accumulate)
- For backup and restore features: LhA must be installed in the `C:` path
  - Download LhA from: http://aminet.net/package/util/arc/lha

---

## Introduction

iTidy is a Workbench utility I wrote to deal with a few recurring annoyances I kept running into when using my own Amiga systems.

The point where I properly decided to write it was after extracting thousands of WHDLoad games and demos. Once everything was neatly filed away into folders like `Games/AGA/A/`, `Games/AGA/B/`, `Games/AGA/C/`, etc., Workbench would still leave the icons and drawer windows in a complete mess. Cleaning that up by hand across so many drawers was painfully slow, and I had the urge to try and build something that would do it for me: give it a folder path, let it dive through all subfolders, then arrange the icons and windows neatly. That experiment eventually turned into iTidy.

iTidy focuses on five areas that tend to cause friction once your disks and drawers start to grow:

- **Manual tidying gets tedious:** Workbench's built-in "Clean Up" works one drawer at a time. iTidy can walk through entire directory trees in one go, which is useful for large WHDLoad collections, archive extractions, or project directories with lots of subfolders.
- **Layouts drift over time:** Changing screen modes, fonts, or resolutions often leaves icons overlapping or windows sized oddly. iTidy re-arranges icons into a simple grid, with adjustable spacing and proportions, so drawers stay readable and consistent with your current setup.
- **Missing icons:** Files extracted from archives or copied from other systems often lack Workbench icons entirely. iTidy's DefIcons integration can create icons automatically - including thumbnail previews for images and text files - so everything is visible and clickable on your desktop.
- **Default tools go missing:** Older software and archives often reference tools that aren't present on a modern setup, resulting in "Unable to open your tool" errors. iTidy checks for missing or invalid default tools and helps you correct them, so icons continue to behave as expected.
- **No safety net for large changes:** Workbench doesn't provide a way to undo icon layout or window changes. iTidy includes an optional backup and restore system, allowing you to roll back changes if the results aren't what you wanted.

The program only works with Workbench `.info` files and drawer layout information. It never modifies the contents of your data files. If you enable automatic icon backups, any changes can later be reversed using the Restore Backups feature.


### What Types of Icons Does iTidy Support?

iTidy can work with the common icon formats you'll typically run into on Workbench setups, including:

- Standard / Classic icons (the original format used across all AmigaOS versions)
- NewIcons (extended colour icons, widely used on OS 3.x)
- OS 3.5 / OS 3.9 Colour Icons (the newer colour icon format introduced with OS 3.5+)

Side note: GlowIcons are supported too - both the older NewIcons-style GlowIcons and the newer Colour Icons version.

iTidy detects icon types automatically, so you don't need to pick a mode or convert anything first. You just point it at a folder and it will tidy the icons it finds in whatever supported format they're already using.

### Safety & Disclaimer

iTidy is free hobby software that I work on in my spare time. It's been tested on real Amiga systems, but it may still have bugs or edge cases I haven't hit yet.

iTidy only touches `.info` files and Workbench drawer/window layout information. It also includes an optional backup feature to help roll back icon/layout changes, but you shouldn't treat that as a replacement for your own regular backups (and it's still being improved).

If you use iTidy, you do so at your own risk. I can't accept responsibility for data loss, corruption, or any other problems caused by using (or misusing) the software.

Before running iTidy for the first time - especially on large or important partitions - make sure you have a current, verified backup. If you want to be extra cautious, try it on a small test folder or a copy of your Workbench setup first.

---

## What's New in v2.0

This section covers the main changes since v1.0. If you're new to iTidy, you can skip straight to [Getting Started](#getting-started).

### Fresh New Look

The entire interface has been rebuilt using ReAction, the native Workbench 3.2 GUI toolkit. The windows, buttons, and lists all follow the standard Workbench 3.2 style and behave consistently with the rest of your desktop. This version requires **Workbench 3.2 or later** as a result.

### Automatic Icon Creation

iTidy v2.0 can create icons automatically for files and folders that don't already have them. To do this it uses **DefIcons**, the file type detection system built into Workbench 3.2. DefIcons examines each file, identifies its type using its built-in file type database, and supplies the appropriate default icon. iTidy does not maintain its own type database or icon set -- it relies entirely on what DefIcons already knows about.

On top of the standard DefIcons icons, iTidy can optionally generate a **thumbnail preview** directly into the icon image for pictures, or a miniature rendered preview of the contents for text files. These are not separate files -- the preview is embedded in the .info icon itself. Whether to do this is configurable per file type.

For image files, ILBM/IFF is handled natively without needing a datatype. All other formats go through the Amiga datatype system -- if DefIcons identifies the file as an image type and a matching datatype is installed, iTidy can decode it and produce a thumbnail. A standard Workbench 3.2.3 installation includes datatypes for JPEG, BMP, GIF, and PNG, so those formats work out of the box. Any additional third-party datatypes you install are picked up automatically -- no changes to iTidy are needed.

**Note:** If DefIcons does not recognise a file type, iTidy will not create an icon for it. What gets recognised depends on your Workbench 3.2 installation and any additional DefIcons type entries you have added.

### Choose Which File Types Get Icons

A new settings window lists all the file types that DefIcons reports on your system. You can enable or disable icon creation for each type individually, and view or change the default tool assigned to open files of that type. Only types that DefIcons recognises will appear here.

### Exclude Folders From Icon Creation

You can tell iTidy to skip specific folders when creating icons. System directories like C:, Libs:, and Devs: are already excluded by default, and you can add your own.

---

## Getting Started

To get started with iTidy:

1. Double-click the iTidy icon on your Workbench desktop to launch it.

2. In the main window, click the **Folder to clean** gadget and choose the folder (or whole partition) you want to tidy. The selected path is shown in the field.

3. Pick how you want icons grouped using the **Grouping** chooser (Folders First, Files First, Mixed, or Grouped By Type).

4. Choose a sort order with the **Sort By** chooser (Name, Type, Date, or Size).

5. If you want iTidy to process everything underneath the chosen folder as well, enable **Include Subfolders**.

6. If you want iTidy to create icons for files that don't have them, enable **Create Icons During Tidy**. You can configure the details via **Icon Creation...**.

7. If you want more control over layout, window shape, and spacing, click **Advanced...** to open the Advanced Settings window.

8. When you're ready, click **Start**. iTidy will process the selected folder (and any subfolders, if enabled), arrange the icons, and resize drawer windows using your chosen settings.

9. If backups are enabled, iTidy will create a restore point before making changes. You can roll back later using **Restore backups...**.

**Tip:** For your first run, try a small test folder so you can see what the options do without touching a whole partition. If you've enabled backups, you can always restore afterwards if you want to undo the changes.

---

## Main Window

The main window is where you choose what to tidy and set the basic options. It's designed to make the common "tidy this folder" job quick, without getting in the way.

From here you can access all of iTidy's features: advanced layout settings, icon creation, default tool analysis, backup restoration, and the tidying process itself.

### Folder Selection

- **Folder to clean:** Click the folder button to open a requester that shows drawers only (not files). The selected path is shown in the field and is read-only - you must use the requester to change it. Default: `SYS:`.

**Tip:** You can select a whole partition (e.g. `Work:`) to process everything on that volume.

### Tidy Options

- **Grouping:** Sets how icons are grouped before sorting.
  - *Folders First:* Drawer icons are laid out before file icons. **(Default)**
  - *Files First:* File icons are laid out before drawer icons.
  - *Mixed:* Folders and files are sorted together with no grouping.
  - *Grouped By Type:* Icons are arranged in visual groups (e.g. Drawers, Tools, Projects, Other). Each group is laid out independently and separated by a configurable gap (see Advanced Settings).

  **Note:** When "Grouped By Type" is selected, the Sort By option is disabled because the group order is fixed. Icons within each group are still sorted by name.

- **Sort By:** Selects the sort key used within the chosen grouping mode (Name, Type, Date, or Size). Disabled when Grouping is set to "Grouped By Type".

- **Include Subfolders:** When enabled, iTidy also processes all subfolders under the selected folder. This is the option you'd use for tidying a whole partition or a large folder tree in one pass. On classic hardware, very large trees can take a while. Default: Off.

- **Create Icons During Tidy:** When enabled, iTidy will create new icons for files and folders that don't already have `.info` files, using the DefIcons system. Files that gain new icons are then included in the layout. You can configure which file types get icons, thumbnail settings, and other creation options via the **Icon Creation...** button. Default: Off.

- **Back Up Layout Before Changes:** When enabled, iTidy creates an LhA backup of each folder's `.info` files before making changes. This gives you something to roll back to later using the Restore backups feature. Requires LhA in `C:`. If LhA is not found when you click Start with backups enabled, iTidy will warn you and offer to continue without backups or cancel. Default: Off.

  This checkbox is kept in sync with the menu option at Settings > Backups > Back Up Layouts Before Changes. Changing one updates the other.

- **Window Position:** Controls where drawer windows are placed after iTidy resizes them.
  - *Center Screen:* Centres the window on the Workbench screen. **(Default)**
  - *Keep Position:* Keeps the window at its current location.
  - *Near Parent:* Places the window slightly down and right of the parent window (cascading style).
  - *No Change:* Resizes the window but doesn't try to move it.

### Tool Buttons

- **Advanced...:** Opens the Advanced Settings window for finer control over layout, density, limits, columns, grouping gaps, and filtering.
- **Fix default tools...:** Opens the Default Tool Analysis window, which scans icons for missing or invalid default tools. The scan targets the currently selected folder and respects the Include Subfolders setting.
- **Restore backups...:** Opens the Restore Backups window, showing previous backup runs created by iTidy. From there you can restore icon positions and window layouts to a previous state, view which folders were included, or delete old backups. **Note:** This restores icon/layout backups only. To undo default tool changes, use the Restore Default Tools feature within the Fix Default Tools window instead.
- **Icon Creation...:** Opens the Icon Creation Settings window, where you can configure how iTidy creates new icons for files that don't have them.

### Action Buttons

- **Start:** Begins processing using the current settings. A progress window opens showing the current status. When processing completes, a summary is shown and the Cancel button changes to Close so you can review the results.
- **Exit:** Closes iTidy.

### Menus

#### Presets

| Menu Item | Shortcut | Description |
|-----------|----------|-------------|
| Reset To Defaults | Amiga+N | Resets all settings to their default values. The current log level is preserved. |
| Open Preset... | Amiga+O | Loads a saved preferences file. Default location: `PROGDIR:userdata/Settings/`. |
| Save Preset | Amiga+S | Saves settings to the last-used file. If no file has been used yet, behaves like Save Preset As. |
| Save Preset As... | Amiga+A | Save settings to a chosen location. Default filename: `iTidy.prefs`. |
| Quit | Amiga+Q | Closes iTidy. |

#### Settings

| Menu Item | Description |
|-----------|-------------|
| Advanced Settings... | Opens the Advanced Settings window. |
| **DefIcons Categories** (submenu) | |
| -- DefIcons Categories... | Opens the DefIcons category selection window. |
| -- Preview Icons... | Opens the Text Templates management window. |
| -- DefIcons Excluded Folders... | Opens the Exclude Paths window. |
| **Backups** (submenu) | |
| -- Back Up Layouts Before Changes | Toggle. Enables or disables automatic LhA backups. Synced with the main window checkbox. |
| **Logging** (submenu) | |
| -- Disabled (Recommended) | Turns off all logging. **(Default)** |
| -- Debug | Most verbose - logs everything. |
| -- Info | Informational messages and above. |
| -- Warning | Warnings and errors only. |
| -- Error | Errors only. |
| -- Open Log Folder | Opens `PROGDIR:logs/` as a Workbench drawer window. |

#### Tools

| Menu Item | Description |
|-----------|-------------|
| **Restore** (submenu) | |
| -- Restore Layouts... | Opens the Restore Backups window. |
| -- Restore Default Tools... | Opens the Default Tool Restore window. |

#### Help

| Menu Item | Description |
|-----------|-------------|
| iTidy Guide... | Opens `PROGDIR:iTidy.guide` using AmigaGuide. |
| About | Shows version number, description, and copyright. |

### Tips

- The main window is intended for quick "point it at a folder and tidy it" runs. If you need more control over layout and window sizing, use **Advanced...**.
- All changes apply to the selected folder and (if enabled) everything underneath it.
- If you're experimenting, enable backups and start with a small test folder first.
- Save your preferred settings as a preset so you can quickly reload them later.
- You can set the LOADPREFS tooltype in the program icon to automatically load your favourite settings every time iTidy starts. See the [ToolTypes](#tooltypes) section for details.

---

## Advanced Settings

Open this window using **Advanced...** on the main window, or via the Settings menu. These options are split into five tabs so related settings stay together.

Changes made here are applied when you click OK, or discarded if you click Cancel (or close the window, or press Escape). To save settings permanently to disk, use Presets > Save on the main window.

### Layout Tab

**Layout Aspect** - Sets the target width-to-height shape for drawer windows.

- *Tall (0.75):* Tall, narrow windows
- *Square (1.0):* Roughly square windows
- *Compact (1.3):* Slightly wider than tall
- *Classic (1.0):* Traditional Workbench-like proportions
- *Wide (2.0):* Wider, landscape-oriented windows **(Default)**

**When Full** - Controls what iTidy does when a drawer has more icons than fit comfortably in the window at its target proportions.

- *Expand Horizontally:* Adds more columns. You will scroll left and right. **(Default)**
- *Expand Vertically:* Adds more rows. You will scroll up and down.
- *Expand Both:* Balances expansion in both directions.

**Align Vertically** - Sets how icons are aligned vertically when a row contains icons of different heights: *Top*, *Middle* **(Default)**, or *Bottom*.

### Density Tab

**Horizontal Spacing** - Sets the horizontal gap between icons, in pixels. Range: 0 to 20. Default: 8.

**Vertical Spacing** - Sets the vertical gap between icons, in pixels. Range: 0 to 20. Default: 8.

### Limits Tab

**Icons Per Row: Min** - Minimum columns. Prevents drawers from becoming one long vertical list when a window is narrow or icons are wide. Range: 0 to 30. Default: 2.

**Auto-Calc Max Icons** - When enabled, iTidy automatically calculates the maximum number of columns based on the window width and screen size. This is the recommended setting for most use cases. Default: On. When enabled, the Max field below is disabled.

**Max** - Sets the maximum number of columns manually. Only used when Auto-Calc Max Icons is turned off. Range: 2 to 40. Default: 10.

**Max Window Width** - Limits how wide drawer windows may become, as a percentage of screen width: *Auto*, *30%*, *50%*, *70%* **(Default)**, *90%*, *100%*.

### Columns & Groups Tab

**Column Layout** - When enabled, iTidy uses a column-based layout where icons are arranged in strict columns. When disabled, icons use a free-flow row-based layout. Default: Off.

**Gap Between Groups** - Controls spacing between icon groups when the main window's Grouping option is set to "Grouped By Type". Choices are *Small*, *Medium* **(Default)**, and *Large*. If a group contains no icons, that group is skipped and no gap is added.

### Filters & Misc Tab

**Auto-Fit Columns** - Each column is sized to fit its widest icon, rather than forcing all columns to the same width. Produces a more compact and visually balanced layout. Default: On.

**Reverse Sort Order** - Reverses the current sort direction (e.g. Z to A, newest first, or largest first depending on the sort mode). Default: Off.

**Strip NewIcons Borders (Permanent)** - Strips the black border from NewIcons during processing. Requires icon.library v44+. **Warning:** This permanently modifies the affected icons. If you might want to restore the original appearance later, enable the backup option on the main window first. Default: Off.

**Skip Hidden Folders** - When enabled, iTidy skips folders that do not have a corresponding `.info` file during recursive processing. Folders without `.info` files are considered "hidden" from Workbench and are typically not visible to the user, so processing them serves no purpose. Default: On.

### Buttons

- **Defaults:** Resets all settings in this window to their factory defaults. A confirmation requester is shown first.
- **Cancel:** Discards all changes and returns to the main window.
- **OK:** Accepts the current settings and closes the window.

---

## Icon Creation Settings

Open this window using **Icon Creation...** on the main window. It controls how iTidy creates new icons for files and folders that don't already have `.info` files, using the DefIcons system introduced in Workbench 3.2. Settings are organised into three tabs: Create, Rendering, and DefIcons.

Changes are applied when you click OK, or discarded if you click Cancel.

### Create Tab

**Folder Icons** - Controls whether iTidy creates drawer icons for folders that don't have them.

- *Smart:* Creates a drawer icon only if the folder contains files or icons that need processing. **(Default)**
- *Always Create:* A drawer icon is always created for folders without one.
- *Never Create:* Folder icons are never created.

**Skip icon creation inside WHDLoad folders** - When enabled, folders containing a WHDLoad slave file (`*.slave`) are skipped during icon creation. The drawer icon for the WHDLoad folder itself is still created, but no icons are generated for files inside it. Useful for WHDLoad game and demo collections where the internal files aren't meant to be launched individually from Workbench. Default: Off.

**Text File Previews** - When enabled, iTidy creates thumbnail-style icons for text files by rendering a preview of the file's contents onto the icon image. The appearance of text previews can be customised with templates (see the **Manage Templates...** button and the [Text Templates](#text-templates) section). Default: On.

**Picture File Previews** - When enabled, iTidy creates thumbnail icons for recognised picture files by generating a miniature version of the image. The specific formats to process are selected using the format checkboxes below. Default: On.

**Picture Formats** - Select which image file formats should have thumbnail icons generated:

| Format | Description | Default |
|--------|-------------|---------|
| ILBM (IFF) | Amiga IFF ILBM and PBM images. The most common Amiga image format. | On |
| PNG | PNG images. Supports transparency. | On |
| GIF | GIF images. Supports transparency. | On |
| JPEG (Slow) | JPEG images. Decoding is slow on 68k hardware. | Off |
| BMP | Windows BMP images. | On |
| ACBM | Amiga Continuous Bitmap format. | On |
| Other | Any other image formats supported by installed DataTypes and DefIcons. | On |

**Note:** JPEG is disabled by default because JPEG decoding is computationally expensive on 68k processors. Enabling it will significantly slow down icon creation on classic Amiga hardware.

**Replace Existing Image Thumbnails Created by iTidy** - When enabled, iTidy will delete and recreate any image thumbnail icons that it previously created. iTidy identifies its own icons using the `ITIDY_CREATED` tooltype. Icons placed by the user or by other programs are never affected. Useful after changing rendering settings if you want the new settings applied to previously created thumbnails. Default: Off.

**Replace Existing Text Previews Created by iTidy** - Same as above but for text preview icons. Default: Off.

### Rendering Tab

These settings control the visual appearance of generated thumbnail icons.

**Preview Size** - The canvas size for thumbnail icons:

- *Small (48x48)* - Compact thumbnails.
- *Medium (64x64)* - Balanced size and detail. **(Default)**
- *Large (100x100)* - Largest thumbnails with the most detail.

**Thumbnail Border** - The border style drawn around thumbnail icons:

- *None* - No border.
- *Workbench (Smart)* - Classic Workbench-style frame. Skipped for images with transparency.
- *Workbench (Always)* - Classic Workbench-style frame, always applied.
- *Bevel (Smart)* - Inner highlight bevel (bright top-left, dark bottom-right). Skipped for images with transparency. **(Default)**
- *Bevel (Always)* - Inner highlight bevel, always applied.

The "Smart" modes detect transparent images and skip the border, which avoids drawing borders on empty space around irregularly-shaped images.

**Upscale Small Images To Icon Size** - When enabled, images smaller than the chosen preview size are scaled up to fill the thumbnail area. When disabled, small images are centred at their original size. Default: Off.

**Max Colours** - The maximum number of colours used in generated thumbnail icons:

| Option | Description |
|--------|-------------|
| 4 colours | Very limited palette. |
| 8 colours | Basic colour range. |
| 16 colours | Moderate colour range. |
| GlowIcons palette (29 colours) | Standard GlowIcons palette for maximum compatibility. |
| 32 colours | Good colour range. |
| 64 colours | Rich colour range. |
| 128 colours | Near-photographic quality. |
| 256 colours (full) | Full 256-colour palette. **(Default)** |
| Ultra (256 + detail boost) | Best possible image quality. |

When Max Colours is set to 256 or Ultra, Dithering is disabled. When set above 8, or to GlowIcons palette or Ultra, Low-Colour Palette is disabled.

**Dithering** - The dithering method used when reducing colours:

- *None* - No dithering.
- *Ordered (Bayer 4x4)* - Systematic pattern. Fast and predictable.
- *Error Diffusion (Floyd-Steinberg)* - Smoother gradients but slower.
- *Auto* - Automatically selects the best method based on colour count. **(Default)**

**Low-Colour Palette** - The palette mapping used at very low colour counts (4 or 8 colours):

- *Greyscale* - Maps to a greyscale palette. **(Default)**
- *Workbench Palette* - Maps to the standard Workbench colour palette.
- *Hybrid (Grays + WB Accents)* - Mostly greyscale with a few Workbench accent colours.

Only enabled when Max Colours is set to 4 or 8.

### DefIcons Tab

This tab provides access to the DefIcons configuration sub-windows:

- **Icon Creation Setup...:** Opens the [DefIcons Categories](#deficons-categories) window, where you can select which file types should have icons created.
- **Exclude Paths...:** Opens the [Exclude Paths](#exclude-paths) window, where you can manage folders that should be skipped during icon creation.

### Buttons

- **OK:** Accepts the current settings and closes the window.
- **Cancel:** Discards all changes and closes the window.

---

## DefIcons Categories

Open this window from the Icon Creation Settings > DefIcons tab > **Icon Creation Setup...** button, or from the main window's Settings > DefIcons Categories menu.

This window lets you choose which file types iTidy should create icons for during DefIcons processing. It shows a tree view of all file types defined in the system's `ENV:deficons.prefs`, with checkboxes to enable or disable each type. You can also view and change the default tool assigned to each type.

### Type Tree

The main area shows a hierarchical tree of file types. Top-level entries are categories (e.g. music, picture, tool), and expanding a category reveals the individual file types within it.

- A **checked** type means iTidy will create icons for files of that type.
- An **unchecked** type means iTidy will skip files of that type.

Only file types at the second level have checkboxes. Root categories and deeper sub-types don't have individual checkboxes.

### Default Disabled Types

By default, the following categories are disabled because creating icons for them is rarely useful:

- **tool** - Executable programs (usually already have icons)
- **prefs** - Preferences files
- **iff** - Generic IFF files
- **key** - Keyfiles
- **kickstart** - Kickstart ROM images

All other types are enabled by default.

### Buttons

- **Select All:** Enables all file types. Use with care - this may create icons for system files.
- **Select None:** Disables all file types.
- **Show Default Tools:** Scans the default icon templates in `ENVARC:Sys/` and displays the default tool next to each type name (e.g. `music [MultiView]`). If a type doesn't have its own template icon, no tool is shown.
- **Change Default Tool...:** Opens a file requester to select a new default tool for the currently selected type. The change is written directly to the type's template icon file in `ENVARC:Sys/`. If the type doesn't have its own template icon yet, iTidy will create one by cloning the parent's template icon first.

  **Important:** Default tool changes are saved immediately to disk. They are NOT subject to the OK/Cancel buttons.

- **OK:** Accepts the current type selections and closes the window.
- **Cancel:** Closes the window without saving type selection changes. Default tool changes are not affected by Cancel, as noted above.

---

## Text Templates

Open this window from the Icon Creation Settings > Create tab > **Manage Templates...** button, or from the main window's Settings > DefIcons Categories > Preview Icons menu.

This window manages DefIcons text preview template icons. Each ASCII sub-type (C source, REXX, HTML, etc.) can have its own template icon that controls how iTidy renders text preview thumbnails.

### Master vs Custom Templates

- **Master template:** `def_ascii.info` in `PROGDIR:Icons/` - the fallback used for all ASCII sub-types that lack their own template. Always shown first in the list with status "Master".
- **Custom template:** `def_<type>.info` (e.g. `def_c.info`, `def_rexx.info`) - a type-specific override created by copying the master. Shown with status "Custom".
- If no custom template exists for a sub-type, it falls back to the master automatically. Shown with status "Using master".
- Types listed in the master's `EXCLUDETYPE` tooltype are excluded from icon creation entirely. Shown with status "Excluded".

### Template List

The list shows all DefIcons ASCII sub-types in three columns:

- **Type:** The sub-type name (the master "ascii" entry appears first).
- **Template:** The custom template filename if one exists, otherwise "-".
- **Status:** One of "Master", "Custom", "Using master", or "Excluded".

Use the **Show** filter at the top to display All types, Custom Only, or Missing Only.

### Action Buttons

- **Create from master / Overwrite from master:** Copies `def_ascii.info` to create (or replace) a custom `def_<type>.info` template for the selected type. Disabled when the master type itself is selected.
- **Edit tooltypes...:** Opens the effective template icon in the Workbench Information editor (via ARexx), where you can view and modify the rendering parameters stored as ToolTypes.
- **Validate tooltypes:** Checks the template's ToolTypes for errors (unknown keys, out-of-range values, malformed area definitions) and shows a summary of all rendering parameters.
- **Revert to master:** Deletes the custom template so the sub-type falls back to the master. Asks for confirmation first. Disabled when no custom template exists.
- **Close:** Closes the window.

### Template ToolTypes Reference

Template icons contain `ITIDY_*` prefixed ToolTypes that control text preview rendering:

| ToolType | Purpose |
|----------|---------|
| `ITIDY_TEXT_AREA` | Rectangle (x,y,w,h) defining where text is drawn on the icon |
| `ITIDY_EXCLUDE_AREA` | Rectangle (x,y,w,h) area to skip when drawing |
| `ITIDY_LINE_HEIGHT` | Pixel height of each text line |
| `ITIDY_LINE_GAP` | Pixel gap between text lines |
| `ITIDY_MAX_LINES` | Maximum number of lines to render |
| `ITIDY_CHAR_WIDTH` | Pixel width of each character |
| `ITIDY_READ_BYTES` | Number of bytes to read from the source file |
| `ITIDY_BG_COLOR` | Background colour index (0-255) |
| `ITIDY_TEXT_COLOR` | Text colour index (0-255) |
| `ITIDY_MID_COLOR` | Mid-tone colour index (0-255) |
| `ITIDY_DARKEN_PERCENT` | Darkening percentage (0-100) |
| `ITIDY_DARKEN_ALT_PERCENT` | Alternate darkening percentage (0-100) |
| `ITIDY_ADAPTIVE_TEXT` | Enable adaptive text colouring |
| `ITIDY_EXPAND_PALETTE` | Enable palette expansion |

To exclude a type from icon creation, add or edit the `EXCLUDETYPE` tooltype in `def_ascii.info` with a comma-separated list of type names.

---

## Exclude Paths

Open this window from the Icon Creation Settings > DefIcons tab > **Exclude Paths...** button, or from the main window's Settings > DefIcons Categories > DefIcons Excluded Folders menu.

This window manages the list of directory paths that should be skipped during DefIcons icon creation. When iTidy scans a volume, any folder matching an exclude path is skipped during icon generation.

### The DEVICE: Placeholder

Paths can be stored with a `DEVICE:` prefix instead of a real volume name. At scan time, `DEVICE:` is substituted with whatever volume is being scanned, making the exclude list portable across different drives. For example, `DEVICE:C` would match `Workbench:C`, `Work:C`, or any other volume's `C` directory.

When adding or modifying a path, if the selected directory is on the same volume as the current scan folder, iTidy asks whether to store it as `DEVICE:` or as an absolute path.

### Gadgets

- **Path List:** Shows all configured exclude paths. Single-select. Supports up to 32 entries.
- **Add...:** Opens a directory requester to add a new exclude path. If the path is on the same volume as the scan folder, you'll be asked whether to store it as `DEVICE:` or absolute.
- **Remove:** Removes the selected path from the list.
- **Modify...:** Opens a directory requester to change the selected path.
- **Reset to Defaults:** Clears all paths and repopulates with the 16 default system directories (after confirmation). The defaults cover common AmigaOS system directories that rarely need icons: `DEVICE:Fonts`, `DEVICE:Locale`, `DEVICE:Classes`, `DEVICE:Libs`, `DEVICE:C`, `DEVICE:Rexxc`, `DEVICE:T`, `DEVICE:L`, `DEVICE:Devs`, `DEVICE:Resources`, `DEVICE:System`, `DEVICE:Storage`, `DEVICE:Expansion`, `DEVICE:Kickstart`, `DEVICE:Env`, `DEVICE:Envarc`, and `DEVICE:Prefs/Env-Archive`.
- **OK:** Accepts the current list and closes the window.
- **Cancel:** Discards changes and closes the window.

---

## Default Tool Analysis

You can open this window via **Fix default tools...** on the main window. It scans a directory tree to find all default tools referenced by icons, then displays them in a sortable list so you can find and fix tools that are missing or incorrect.

### What is a Default Tool?

On the Amiga, most data file icons have a "Default Tool" set - the program Workbench runs when you double-click that icon. For example, a `.txt` icon might have `SYS:Utilities/MultiView` as its default tool.

After copying icons between systems or extracting older archives, those tool paths often don't match what's actually installed. When that happens you'll see the familiar "Unable to open your tool" error.

### Using the Scanner

1. **Select folder:** The Folder field shows the current scan folder from the main window. You can change it here if needed.
2. **Scan:** Click **Scan** to examine all icons and check whether their default tools can be found. Scanning is always recursive regardless of the main window's Include Subfolders setting. A progress window shows scan statistics while it works.
3. **Review:** The upper list shows each unique default tool found, how many icons reference it, and whether it exists.
4. **Filter:** Use the filter chooser to show All, Valid Only, or Missing Only.
5. **Inspect:** Click a tool in the upper list to see its details in the lower panel, including which icon files reference it.
6. **Fix:** Use **Replace Tool (Batch)** to update all icons that use the selected tool, or **Replace Tool (Single)** to change just one specific file. See [Replacing Default Tools](#replacing-default-tools) for details.

### Tool List Columns

- **Tool:** The default tool path (long paths are shortened with `/../` notation for display)
- **Files:** How many icons reference that tool
- **Status:** EXISTS or MISSING

Click a column header to sort by that column. Click again to reverse the sort order.

### Details Panel

When you select a tool in the upper list, the lower panel shows:

- The tool name
- Status (EXISTS/MISSING) and version string if available
- A list of all icon files that reference this tool (up to 200 shown)

### Menus

**Project menu:**

| Item | Shortcut | Description |
|------|----------|-------------|
| New | Amiga+N | Clears the tool cache and empties both lists. |
| Open... | Amiga+O | Loads a previously saved tool cache file. |
| Save | Amiga+S | Saves the cache to `PROGDIR:userdata/DTools/toolcache.dat`. |
| Save as... | Amiga+A | Saves the cache to a chosen location. |
| Close | Amiga+C | Closes the window. |

Saving and loading tool caches is handy when you're working with large drives that take a while to scan - you can save the results and reload them later without rescanning.

**File menu:**

| Item | Shortcut | Description |
|------|----------|-------------|
| Export list of tools | Amiga+T | Exports a text report of all tools found. |
| Export list of files and tools | Amiga+F | Exports a detailed report of all files and their tools. |

**View menu:**

| Item | Shortcut | Description |
|------|----------|-------------|
| System PATH... | Amiga+P | Shows the list of directories iTidy searches for default tools. |

### Buttons

- **Replace Tool (Batch)...:** Opens the Replace Default Tool window in batch mode. Only enabled when a tool is selected in the upper list.
- **Replace Tool (Single)...:** Opens the Replace Default Tool window for one specific icon file. Only enabled when a file is selected in the details panel.
- **Restore Default Tools Backups...:** Opens the [Restoring Default Tool Backups](#restoring-default-tool-backups) window. If any restores are performed, the tool cache is cleared on return and you'll be prompted to re-scan.
- **Close:** Closes the window.

### Technical Notes

When validating tools, iTidy checks absolute paths directly. For simple tool names like `MultiView`, it searches the Amiga PATH (parsed from `S:Startup-Sequence` and `S:User-Startup`) to see if the tool can be found there.

---

## Replacing Default Tools

This window opens when you click **Replace Tool (Batch)** or **Replace Tool (Single)** in the Default Tool Analysis window.

It operates in two modes:

- **Batch mode:** Updates ALL icons that reference the selected tool (e.g. change every icon using "OldTool" to use "MultiView").
- **Single mode:** Updates ONE specific icon file.

### Using the Window

The top section shows the current tool being replaced and the mode (batch or single). Use the **Change To** file browser to select the replacement tool program. The browser starts in `C:`.

Click **Update Default Tool** to start the replacement. A progress list shows the result for each icon processed:

- **SUCCESS:** The default tool was updated.
- **FAILED:** The icon could not be modified.
- **READ-ONLY:** The icon file is write-protected and was skipped.

After completion, a summary requester shows the success and failure counts. The Update button is then permanently disabled for this session - close and reopen the window to perform another update.

**Clearing a default tool:** If you leave the Change To field empty and click Update, iTidy will remove the default tool from the selected icon(s). A confirmation requester is shown first, since icons without a default tool won't launch a program when double-clicked.

### Default Tool Backups

Every change is automatically recorded in the backup system. A session folder is created under `PROGDIR:Backups/tools/` containing the date, mode, and a CSV file listing each icon's original and new tool. This data is used by the [Restoring Default Tool Backups](#restoring-default-tool-backups) feature to undo changes later.

---

## Restoring Default Tool Backups

Open this window from the Default Tool Analysis window > **Restore Default Tools Backups...** button, or from the main window's Tools > Restore > Restore Default Tools menu.

This window lets you view and restore backed-up default tool changes. Each time iTidy replaces default tools, a backup session is created. This is separate from the icon/layout backups used for tidying.

### Session List

The upper list shows all backup sessions found in `PROGDIR:Backups/tools/`, with four columns:

- **Date/Time:** When the backup was created.
- **Mode:** Batch or Single.
- **Path:** The folder that was being processed (truncated to 50 characters).
- **Changed:** Number of icons that were modified.

### Changes Panel

When you select a session, the lower panel shows the tool changes recorded in that session:

- **Old tool:** The original default tool (or "(none)" if it was empty).
- **New tool:** The replacement tool (or "(none)" if cleared).
- **Icons:** Number of icons affected by this specific change.

### Buttons

- **Restore:** Reverts all icons from the selected session to their original default tools. Shows a confirmation requester first, then a completion summary with success/fail counts. The backup session is kept after restoring - it's not automatically deleted.
- **Delete:** Permanently deletes the selected backup session and its files. This can't be undone.
- **Close:** Closes the window.

Both Restore and Delete are only enabled when a session is selected.

---

## Restoring Layout Backups

You can open this window via **Restore backups...** on the main window, or from the Tools > Restore > Restore Layouts menu. It restores icon positions and drawer/window layout information from the LhA backup archives that iTidy created during previous tidying runs.

**Note:** This restores icon/layout backups only. If you're trying to undo default tool changes, use [Restoring Default Tool Backups](#restoring-default-tool-backups) instead.

### Run List

The top list shows previous backup runs stored in `PROGDIR:Backups/`:

- **Run:** A run number (e.g. 0001, 0002). Numbers may have gaps if intermediate runs were deleted.
- **Date/Time:** When the backup was created.
- **Folders:** How many folders were backed up.
- **Size:** Total size of the archives (shown in B, KB, or MB).
- **Icons+:** Number of icons created by DefIcons during this run ("-" if none).
- **Status:** "Complete" (has catalog) or "NoCAT" (orphaned, no catalog file).

Click a run to select it and view its details. Double-click to open the folder view.

### Details Panel

Shows information about the selected run, including run number, date, source directory, archive count, total size, icons created, status, and storage location.

Status values include: "Complete (catalog present)", "Orphaned (no catalog)", "Incomplete (missing archives)", or "Corrupted (catalog error)".

### Buttons

- **Restore Run:** Restores the selected backup run. A requester offers three choices:
  - *With Windows:* Restores icons AND window positions/sizes.
  - *Icons Only:* Restores only icon files (positions, ToolTypes, default tools) without changing window geometry.
  - *Cancel:* Aborts the restore.

  If the run included DefIcons-created icons, those icons will be removed during restore. A progress window shows per-folder extraction progress.

- **Delete Run:** Permanently deletes the selected backup run and all its files. A confirmation requester lists exactly what will be removed.
- **View Folders...:** Opens the [Folder View](#folder-view) sub-window showing a tree of the archived folders. Only available for runs with a catalog file. Also triggered by double-clicking a run.
- **Cancel:** Closes the window.

### Requirements

- LhA must be installed in `C:` for the Restore function.
- Enough free disk space to extract the archives.

---

## Folder View

This window opens from the Restore Backups window via **View Folders...** or by double-clicking a backup run. It's a read-only tree viewer that shows the hierarchical folder structure of a backup run's archived contents.

The tree has two columns:

- **Folder:** The folder name, shown in a hierarchical tree with disclosure triangles for expanding and collapsing branches.
- **Size / Icons:** Combined info (e.g. "11 KB / 15 icons"). Shows the compressed archive size, not the original file sizes.

The tree starts fully collapsed - click the disclosure triangles to explore the folder structure. No actions can be performed from this window; it's purely informational.

---

## ToolTypes

iTidy reads the following ToolTypes from its program icon when launched from Workbench:

| ToolType | Values | Default | Description |
|----------|--------|---------|-------------|
| DEBUGLEVEL | 0-4 | 4 (Disabled) | Sets the log level. 0=Debug, 1=Info, 2=Warning, 3=Error, 4=Disabled. This overrides any log level stored in a loaded preferences file. |
| LOADPREFS | File path | (none) | Automatically loads a saved preferences file at startup. Paths without a device name are resolved relative to `PROGDIR:userdata/Settings/`. |
| PERFLOG | YES / NO | NO | Enables performance timing logs for benchmarking. |

To set ToolTypes, right-click the iTidy icon, select "Information..." from the Icons menu, and add or edit the ToolType entries.

For example, to always load your preferred settings on startup, add:
`LOADPREFS=MySettings.prefs`

---

## Tips & Troubleshooting

### General Tips

- **Enable backups for big runs:** If you're tidying a lot of folders (especially with recursion enabled), turning on backups gives you a way back if you don't like the result.
- **Start small:** Try a single drawer first so you can see how the settings affect icon layout and window sizing before you point it at a whole partition.
- **Save your settings:** Once you've found a setup you like, use **Save Preset** from the Presets menu so you can load it again later. Or set the LOADPREFS tooltype to load your settings automatically.
- **Live preview trick:** Keep a drawer window open while you run iTidy on that same drawer. You can quickly see how the layout looks and tweak settings before doing a long recursive run. While iTidy is working through a drawer, icons can briefly overlap until it has repositioned everything. For best results, let it finish the current drawer before cancelling. Once a drawer is complete, it's generally safe to stop a long run if you've seen enough. Workbench may cache window sizing/position until you close and reopen the drawer, but icon placement can be checked straight away.
- **To see window changes clearly:** Close any drawers you're tidying before you run iTidy. After it finishes, you may need to restart Workbench (or reboot) to see updated window sizes and positions everywhere.
- **Creating icons for a whole drive:** Enable "Create Icons During Tidy" and "Include Subfolders", then point iTidy at a partition root. Review the Exclude Paths list first to make sure system directories like `C:`, `Libs:`, and `Devs:` are excluded.
- **JPEG thumbnails are slow:** JPEG decoding is expensive on 68k hardware. If you have lots of JPEG files, consider leaving the JPEG format disabled unless you specifically want thumbnails for those files.

### Icons Appear Misaligned

If some icons look misaligned after tidying while others look fine, it's often down to icon.library support rather than iTidy itself.

On older Workbench versions with older icon.library, OS 3.5+ colour icons may not render at their real size and can appear as tiny placeholder images. iTidy positions icons using the *actual* icon dimensions, but Workbench may draw them at the wrong size, which makes the layout look "off".

**What to do:**
- Update icon.library to v44+ if you want to use OS 3.5+ colour icons.
- Alternatively, convert colour icons back to NewIcons or classic icons if you're keeping a more stock setup.

### Window Positions Not Updating

If you restore a backup (or run iTidy) and window positions don't appear to change right away, this is usually Workbench caching.

**What to do:**
- Close and reopen the affected drawer windows.
- If it still doesn't look updated, restart Workbench or reboot.

### If Backup or Restore Doesn't Work as Expected

A few quick things to check:

- LhA is installed in `C:`
- You have enough free disk space for archives (backups) or extraction (restores)
- The original folder paths still exist when restoring
- Check `PROGDIR:logs/` for any error details

### Slow Processing on Large Folder Trees

Recursing through thousands of folders takes time on real hardware, especially with mechanical drives.

A few tips:
- It's fine to leave iTidy running unattended during large jobs.
- Avoid running other disk-heavy tasks at the same time.

### Icons Aren't Moved, and the Progress Log Shows No Icons Found

Check the folder in Workbench and see if the menu `Window->Show->All Files` is selected. This option shows the contents of the folder by giving each item a temporary icon, which won't be visible to iTidy.

If you want iTidy to tidy this folder, you should create real icons first. You can either enable "Create Icons During Tidy" in iTidy, or manually snapshot them: select the folder's contents via `Window->Select Contents`, then choose `Icons->Snapshot`.

### iTidy Is Skipping Some Folders That Contain Valid Icons

By default, iTidy skips folders that don't have a parent folder icon. To bypass this, go to Advanced Settings and untick "Skip Hidden Folders".

### Icons Don't Open ("Unable to open your tool")

This usually means an icon's Default Tool points to something that isn't installed on your system.

**What to do:**
- Use **Fix default tools...** to scan for missing tools and repair them.
- See the [Default Tool Analysis](#default-tool-analysis) section for how the scanner works.

### DefIcons Categories Window Is Empty

If the DefIcons Categories window shows no file types, it means `ENV:deficons.prefs` was not found or could not be parsed. This file is managed by the "DefaultIcons" Preferences tool in the Workbench Prefs drawer. Make sure you have Workbench 3.2 installed and the DefIcons system is configured.  

---

### Safety & Disclaimer

iTidy is free hobby software that I work on in my spare time. It's been tested on real Amiga systems, but it may still have bugs or edge cases I haven't run into yet.

iTidy only touches `.info` files and Workbench drawer/window layout information. It also includes an optional backup system to help roll back changes, but you shouldn't treat that as a replacement for your own regular system backups (and it's still being improved).

If you use iTidy, you do so at your own risk. I can't accept responsibility for data loss, corruption, or any other damage caused by using (or misusing) the software.

Before running iTidy for the first time - especially on large or important partitions - make sure you have a current, verified backup. If you want to be extra cautious, try it on a small test folder or a copy of your Workbench setup first.

---

## Credits & Version Info

**Author:** Kerry Thompson
**Version:** 2.0
**Website:** https://github.com/Kwezza/iTidy
**Special thanks** Darren 'dmcoles' Cole for the excellent "ReBuild" which iTidy interface was rebuilt with.
---
