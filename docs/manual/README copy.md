# iTidy v2.0

**A Workbench Icon & Window Tidy Tool for AmigaOS 3.x**

---

## Table of Contents

1. [Requirements](#requirements)
2. [Introduction](#introduction)
3. [Getting Started](#getting-started)
4. [Main Window](#main-window)
5. [Advanced Settings](#advanced-settings)
6. [Default Tool Analysis](#default-tool-analysis)
7. [Restoring Backups](#restoring-backups)
8. [Tips & Troubleshooting](#tips--troubleshooting)
9. [Credits & Version Info](#credits--version-info)

---

## Requirements

iTidy requires the following to run properly:

- AmigaOS Workbench 3.0 or newer
- At least 1 megabyte (1MB) of free memory
- At least 1 megabyte (1MB) of free storage space in the installation location
- More storage space is needed as backups are created
- For backup and restore features: LhA must be installed in the `SYS:C` path
  - Download LhA from: http://aminet.net/package/util/arc/lha

---

## Introduction

iTidy is a comprehensive Workbench utility designed to solve four major problems faced by Amiga users:

- **Tedious manual tidying:** Workbench's built-in "Clean Up" only works on one folder at a time. iTidy can process thousands of drawers and subfolders in a single run, making it ideal for large WHDLoad sets, archive extractions, and project trees.

- **Inconsistent layouts:** Changing fonts, screen modes, or resolutions often leaves icons overlapping or windows shaped awkwardly. iTidy automatically arranges icons in a sensible grid, with customizable aspect ratios and spacing, ensuring every drawer looks neat and matches your setup.

- **Broken or missing default tools:** Many archives and old projects use default tools that don't exist on your system, leading to "Unable to open your tool" errors. iTidy scans for invalid default tools and helps you repair them quickly, keeping your icons functional.

- **No easy undo for large changes:** Workbench offers no built-in "undo" for icon positions or window layouts. iTidy includes a backup and restore system, so you can safely revert changes if needed.

iTidy is designed for real-world Amiga use: it respects your existing layouts, offers both quick and advanced controls, and provides safety features for large-scale operations. Whether you're tidying a single drawer or an entire partition, iTidy helps keep your Workbench organized, consistent, and recoverable.

iTidy only works with Workbench `.info` files and window layout information – it never modifies the contents of your data files. For added safety, you can enable automatic icon backups so changes can be rolled back later using the Restore Backups feature.

### What Types of Icons Does iTidy Support?

iTidy can automatically tidy and process all major Amiga icon formats used on Workbench 3.x systems:

- Standard/Classic Icons (used on all AmigaOS versions)
- NewIcons (extended color icons, popular on OS3.x)
- OS3.5/OS3.9 Color Icons (modern color icons introduced in AmigaOS 3.5+)

*Side note: GlowIcons are fully supported—both in their traditional NewIcons format and the newer Color Icons format.*

iTidy detects and handles these formats automatically. You do not need to choose or convert icons—just run iTidy and it will tidy, sort, and update all supported icon types in your folders.

### Safety & Disclaimer

iTidy is free, hobbyist software developed in the author’s spare time. It is provided in good faith and has been tested on real Amiga systems, but it may still contain bugs or behave in ways that were not anticipated.

iTidy only works with `.info` files and Workbench window layouts, and it includes an optional backup system to help you roll back icon changes. However, this backup feature is still under active development and should not be treated as a substitute for your own regular system backups.

By using iTidy, you accept that you do so entirely at your own risk. The author cannot accept any responsibility or liability for data loss, corruption, or any other damage or loss arising from the use or misuse of this software.

**Before running iTidy for the first time—especially on large or important partitions—you must ensure you have a current, verified backup of your system.** For extra safety, consider testing iTidy on a copy of your Workbench setup or a small sample folder before using it on your main installation.

---

## Getting Started

To get started with iTidy:

1. Double-click the iTidy icon from your Workbench desktop to launch the program.

2. In the main window, use the **Browse...** button to select the folder or partition you want to tidy. The selected path will appear in the Folder Path display.

3. Choose your preferred sorting order for icons using the **Order** cycle gadget (Folders First, Files First, or Mixed).

4. If you want to tidy all subfolders as well, enable the **Cleanup subfolders** option.

5. For more control over icon layout, window shape, and spacing, click **Advanced...** to open the Advanced Settings window.

6. When ready, click **Start** to begin tidying. iTidy will process the selected folder (and subfolders, if enabled), arranging icons and resizing windows according to your chosen settings.

7. If you enabled backups, iTidy will create a restore point before making changes. You can revert to this backup later using **Restore Backups...**.

**Tip:** For your first run, try tidying a small folder to see how the options affect icon layout and window size. You can always restore from backup if you want to undo changes.

---

## Main Window

The iTidy main window is designed for fast, easy access to the most important icon tidying features. Here's a breakdown of each section and control:

### Folder Section

- **Folder Path:** Displays the Amiga path of the folder to be processed (e.g., `SYS:`, `Work:Projects`, `DH0:Games`). This is read-only and updates when you select a new folder.

- **Browse... Button:** Opens the AmigaDOS file requester, allowing you to navigate and select the folder you want to tidy. Only drawers (folders) are shown, not files. You can also type a path directly. The last selected path is remembered during your session.

### Tidy Options Section

- **Order:** Cycle gadget with options for Folders First, Files First, or Mixed. Controls how icons are grouped and sorted in the window.
  - *Folders First:* All drawer icons appear before file icons.
  - *Files First:* All file icons appear before drawer icons.
  - *Mixed:* Folders and files are sorted together alphabetically.

- **Cleanup subfolders:** When checked, iTidy processes all subdirectories within the selected folder tree. Useful for tidying entire partitions or large project trees in one pass. **Note:** On classic hardware this may take a while for very large folder trees, but it’s safe to let iTidy run unattended while it works.

- **Backup icons:** When checked, iTidy creates an LhA archive backup of all `.info` files before making changes. This backs up icon positions and window layouts—restored via **Restore Backups...** in the main window. Requires LhA in `C:` directory. Highly recommended for large operations. **Note:** This is separate from Default Tool backups, which are handled automatically when fixing default tools.

- **Position:** Controls where folder windows appear after resizing. Options:
  - *Center Screen:* Window centered on Workbench screen (default).
  - *Keep Position:* Window stays at current location.
  - *Near Parent:* Opens slightly down and right of parent window.
  - *No Change:* Window resized but not moved.

### Other Controls

- **Start Button:** Begins the tidying process using your current settings.

- **Advanced...:** Opens the Advanced Settings window for further customization.

### Menu Options

iTidy includes menu options to load and save your current settings:

- **Load Settings:** Restore previously saved preferences for icon layout, sorting, and backup options.
- **Save Settings:** Store your current configuration for future use, making it easy to repeat your preferred setup.

These options help you quickly switch between different layouts or restore your favorite configuration after experimenting with new settings.

### Tips

- The main window is optimized for quick, one-click tidying. For more control, use Advanced Settings.
- You can always return to the main window from other dialogs.
- All changes are applied to the selected folder and, if enabled, its subfolders.

---

## Advanced Settings

Accessed via **Advanced...** in the main window. These options give you fine-grained control over icon layout and window sizing.

### Layout Aspect Ratio

Controls the target width-to-height ratio for folder windows.

- *Tall (0.75):* Very tall, narrow windows
- *Square (1.0):* Square windows
- *Compact (1.3):* Slightly wider than tall
- *Classic (1.6):* Traditional Workbench proportions
- *Wide (2.0):* Wide, modern proportions (default)
- *Ultrawide (2.4):* Very wide windows

### Overflow Strategy

Determines how iTidy handles folders with too many icons to fit on screen.

- *Expand Horizontally:* Adds more columns, user scrolls horizontally (default)
- *Expand Vertically:* Adds more rows, user scrolls vertically
- *Expand Both:* Balances expansion in both directions

### Icon Spacing (X and Y)

Sets the horizontal and vertical gap between icons (0-20 pixels). Default is 8 pixels for both. Lower values create tighter layouts.

### Icons per Row (Min/Max)

- **Min:** Minimum columns (default: 2). Prevents single-column vertical stacks.
- **Max:** Maximum columns. Only used when Auto Max is disabled.
- **Auto Max Icons:** When checked (default), iTidy calculates max columns automatically based on screen width.

### Max Window Width

Limits folder window width as a percentage of screen width (Auto, 30%, 50%, 70%, 90%, 100%). Only applies when Auto Max Icons is enabled.

### Align Icons Vertically

Controls vertical alignment when icons in a row have different heights: *Top*, *Middle* (default), or *Bottom*.

### Reverse Sort (Z→A)

When checked, reverses the sort order (Z→A, newest-first, largest-first).

### Optimize Column Widths

When checked (default), each column width is based on its widest icon rather than a uniform width for all columns.

### Column Layout (centered)

When checked (default), icons are centered within their column rather than left-aligned.

### Skip Hidden Folders

When checked (default), skips folders without `.info` files during recursive processing.

### Strip NewIcon Borders

Removes borders from NewIcons during processing. Requires icon.library v44+ (Workbench 3.5+). **Important:** This change is permanent for the affected icons, so enable icon backups first if you might want to restore the original look later.

---

## Default Tool Analysis

Accessed via **Fix Default Tools...** in the main window. This feature scans WBPROJECT icons to find and repair missing or invalid default tools.

### What is a Default Tool?

Every Amiga data file icon specifies a "Default Tool"—the program that opens when you double-click it. For example, a `.txt` file might use `SYS:Utilities/MultiView`. When icons are copied between systems or extracted from archives, these tool paths often point to programs that don't exist, causing "Unable to open your tool" errors.

### Using the Scanner

1. **Select folder:** Use **Browse...** to choose a folder. Scanning is always recursive.
2. **Scan:** Click **Scan** to examine all icons and check if their default tools exist.
3. **Review:** The tool list shows each default tool, how many icons use it, and its status (EXISTS or MISSING).
4. **Filter:** Use the cycle gadget to show All, Existing only, or Missing only.
5. **Fix:** Select a missing tool and click **Replace Tool (Batch)** to fix all icons using it, or select a specific file and use **Replace Tool (Single)**.

### Tool List Columns

- **Tool:** The default tool path (e.g., `SYS:Tools/TextEdit`)
- **Files:** Number of icons using this tool
- **Status:** MISSING or EXISTS

### Menu Options

- **Project menu:** New, Open, Save, Save As, Close. Saving scans is useful for large drives that take time to scan.
- **File menu:** Export tool list or detailed file list to text files.

### Default Tool Backups

iTidy automatically backs up original default tool paths (as small text files) before making changes. This is a separate backup system from the icon layout backups created during tidying. Use **Restore Default Tools Backups...** within this window to revert default tool changes—this does not restore icon positions or window layouts.

### Technical Notes

iTidy searches the full Amiga PATH (parsed from `S:Startup-Sequence` and `S:User-Startup`) when validating simple tool names like `MultiView`. Absolute paths are checked directly.

---

## Restoring Backups

Accessed via **Restore Backups...** in the main window. This feature restores icon positions and window layouts from iTidy's LhA backup archives. Only available if you previously ran iTidy with **Backup icons** enabled.

**Note:** This restores icon layout backups only. To restore default tool changes, use **Restore Default Tools Backups...** in the Fix Default Tools window instead.

### Run List

The top list shows all previous backup runs:

- **Run:** Sequential identifier (0001, 0002, etc.)
- **Date/Time:** When the backup was created
- **Folders:** Number of folders backed up
- **Size:** Total archive size
- **Status:** Complete or Incomplete

Click a run to select it and view details below.

### Details Panel

Shows information about the selected run: source directory, total archives, size, status, and backup location.

### Controls

- **Restore window positions:** When checked (default), also restores window geometry (position and size). A Workbench restart may be required to see changes.

### Buttons

- **Restore Run:** Extracts all archived `.info` files back to their original locations, restoring icon positions and window geometry.
- **Delete Run:** Permanently deletes the selected backup. **Important:** This action cannot be undone, so only delete runs you’re sure you no longer need.
- **View Folders...:** Shows all folders included in the selected run.
- **Cancel:** Close without making changes.

### What Gets Restored

- All `.info` files in each folder
- Icon positions (X/Y coordinates)
- Window geometry (if checkbox enabled)
- Folder view mode settings

### Requirements

- LhA must be installed in `C:` directory
- Backup archives in `PROGDIR:Backups/`
- Sufficient disk space for extraction

---

## Tips & Troubleshooting

### General Tips

- **Always enable backups** for large or recursive operations. You can safely experiment knowing you can restore the original layout.
- **Start small:** Try tidying a single folder first to see how the settings affect icon layout and window size before processing an entire partition.
- **Save your settings:** Once you find a configuration you like, use **Save Settings** from the menu so you can quickly reload it later.
- **Live preview trick:** Keep a folder window open in Workbench while you run iTidy on that folder. This lets you fine-tune settings before committing to a long recursive run across many subfolders. During processing, some icons may temporarily overlap until all icons in that folder have been repositioned; for best results, allow iTidy to finish the folder it’s currently working on before cancelling. Once a folder is complete, you can safely stop a long recursive run if you decide you’ve seen enough. Window size and position might be cached by Workbench until you close and reopen the drawer, but you can immediately see how icons are arranged.
- **To see window changes:** Close all affected folders before running iTidy, then reboot after processing completes to see the new window sizes and positions.

### Icons Appear Misaligned (Color Icon / icon.library Issue)

If some icons appear misaligned after tidying, but others look fine, this is usually caused by an outdated icon.library that cannot display modern color icons correctly.

**What happens:** OS 3.5+ color icons require icon.library v44 or higher. On older systems (Workbench 3.0/3.1 with the original icon.library), these icons display as tiny placeholder images—often just a few pixels tall. iTidy reads the *actual* icon dimensions and positions icons correctly, but Workbench displays the wrong size, making icons appear misaligned.

**Solution:** Update your icon.library to v44 or newer. Updated libraries are available from Aminet or as part of OS 3.5/3.9/3.2 upgrades. Alternatively, convert color icons to NewIcons or classic format if you prefer to stay on an older Workbench.

**Note:** iTidy reads and positions icons according to their real dimensions. On older systems, the visual misalignment is caused by how classic icon.library versions render modern color icons, rather than by iTidy’s layout logic.

### Window Positions Not Updating

After restoring a backup or running iTidy, window positions may not appear to change immediately.

**Cause:** Workbench caches window geometry for open folders.

**Solution:** Close and reopen the affected folder windows. In some cases, you may need to restart Workbench or reboot for changes to take effect.

### If Backup or Restore Doesn't Work as Expected

**Check the following:**
- LhA must be installed in `C:` directory
- Sufficient free disk space for archives (backups) or extraction (restores)
- The original folder paths must still exist when restoring
- Check `PROGDIR:logs/` for detailed error messages

### Slow Processing on Large Folders

Processing thousands of folders recursively is naturally time-consuming, especially on real Amiga hardware with mechanical drives.

**Tips:**
- iTidy can be left running unattended for large operations
- Avoid running other disk-intensive programs simultaneously
- For very large collections, consider breaking the work into chunks (e.g., `WHDLoad:Games` then `WHDLoad:Demos`) rather than processing everything at once

### Icons Don't Open ("Unable to open your tool")

This error means the icon's default tool path points to a program that doesn't exist on your system.

**Solution:** Use **Fix Default Tools...** to scan for missing tools and repair them. See the Default Tool Analysis section for details.

---

### Safety & Disclaimer

iTidy is free, hobbyist software developed in the author’s spare time. It is provided in good faith and has been tested on real Amiga systems, but it may still contain bugs or behave in ways that were not anticipated.

iTidy only works with `.info` files and Workbench window layouts, and it includes an optional backup system to help you roll back icon changes. However, this backup feature is still under active development and should not be treated as a substitute for your own regular system backups.

By using iTidy, you accept that you do so entirely at your own risk. The author cannot accept any responsibility or liability for data loss, corruption, or any other damage or loss arising from the use or misuse of this software.

**Before running iTidy for the first time—especially on large or important partitions—you must ensure you have a current, verified backup of your system.** For extra safety, consider testing iTidy on a copy of your Workbench setup or a small sample folder before using it on your main installation.

---

## Credits & Version Info

**Author:** Kwezza  
**Version:** 2.0  
**Contact:** [Your Contact Info Here]

---

*For a full interactive manual, open `iTidy.guide` in AmigaGuide.*
