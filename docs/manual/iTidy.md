# iTidy v1.0

**A Workbench Icon & Window Tidy Tool for AmigaOS 3.x**

iTidy is a small Workbench utility I wrote after getting fed up with the mess left behind by large archive extractions - especially when unpacking thousands of WHDLoad games and demos into drawer trees like `Games/AGA/A/`, `Games/AGA/B/`, and so on. The aim was simple: point it at a folder, let it recurse through everything underneath, and have it tidy icon layouts and drawer windows in a consistent way.

It works by updating Workbench `.info` files and drawer/window layout information only - it doesn’t touch the contents of your data files. If you enable backups, iTidy can create LhA restore points so you can roll back icon and layout changes later.


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

iTidy is a Workbench utility I wrote to deal with a few recurring annoyances I kept running into when using my own Amiga systems.

The point where I properly decided to write it was after extracting thousands of WHDLoad games and demos. Once everything was neatly filed away into folders like `Games/AGA/A/`, `Games/AGA/B/`, `Games/AGA/C/`, etc., Workbench would still leave the icons and drawer windows in a complete mess. Cleaning that up by hand across so many drawers was painfully slow, and I had the urge to try and build something that would do it for me: give it a folder path, let it dive through all subfolders, then arrange the icons and windows neatly. That experiment eventually turned into iTidy.

iTidy focuses on four areas that tend to cause friction once your disks and drawers start to grow:

- **Manual tidying gets tedious:** Workbench’s built-in “Clean Up” works one drawer at a time. iTidy can walk through entire directory trees in one go, which is useful for large WHDLoad collections, archive extractions, or project directories with lots of subfolders.
- **Layouts drift over time:** Changing screen modes, fonts, or resolutions often leaves icons overlapping or windows sized oddly. iTidy re-arranges icons into a simple grid, with adjustable spacing and proportions, so drawers stay readable and consistent with your current setup.
- **Default tools go missing:** Older software and archives often reference tools that aren’t present on a modern setup, resulting in “Unable to open your tool” errors. iTidy checks for missing or invalid default tools and helps you correct them, so icons continue to behave as expected.
- **No safety net for large changes:** Workbench doesn’t provide a way to undo icon layout or window changes. iTidy includes an optional backup and restore system, allowing you to roll back changes if the results aren’t what you wanted.

The program only works with Workbench `.info` files and drawer layout information. It never modifies the contents of your data files. If you enable automatic icon backups, any changes can later be reversed using the Restore Backups feature.


### What Types of Icons Does iTidy Support?

iTidy can work with the common icon formats you’ll typically run into on Workbench 3.x setups, including:

- Standard / Classic icons (the original format used across all AmigaOS versions)
- NewIcons (extended colour icons, widely used on OS3.x)
- OS3.5 / OS3.9 Colour Icons (the newer colour icon format introduced with OS3.5+)

Side note: GlowIcons are supported too - both the older NewIcons-style GlowIcons and the newer Colour Icons version.

iTidy detects icon types automatically, so you don’t need to pick a mode or convert anything first. You just point it at a folder and it will tidy the icons it finds in whatever supported format they’re already using.

### Safety & Disclaimer

iTidy is free hobby software that I work on in my spare time. It’s been tested on real Amiga systems, but it may still have bugs or edge cases I haven’t hit yet.

iTidy only touches `.info` files and Workbench drawer/window layout information. It also includes an optional backup feature to help roll back icon/layout changes, but you shouldn’t treat that as a replacement for your own regular backups (and it’s still being improved).

If you use iTidy, you do so at your own risk. I can’t accept responsibility for data loss, corruption, or any other problems caused by using (or misusing) the software.

Before running iTidy for the first time - especially on large or important partitions - make sure you have a current, verified backup. If you want to be extra cautious, try it on a small test folder or a copy of your Workbench setup first.

---

## Getting Started

To get started with iTidy:

1. Double-click the iTidy icon on your Workbench desktop to launch it.

2. In the main window, click **Browse...** and choose the folder (or whole partition) you want to tidy. The selected location will appear in the Folder Path display.

3. Pick how you want icons ordered using the **Order** cycle gadget (Folders First, Files First, or Mixed).

4. If you want iTidy to process everything underneath the chosen folder as well, enable **Cleanup subfolders**.

5. If you want more control over layout, window shape, and spacing, click **Advanced...** to open the Advanced Settings window.

6. When you’re ready, click **Start**. iTidy will process the selected folder (and any subfolders, if enabled), arrange the icons, and resize drawer windows using your chosen settings.

7. If backups are enabled, iTidy will create a restore point before making changes. You can roll back later using **Restore Backups...**.

**Tip:** For your first run, try a small test folder so you can see what the options do without touching a whole partition. If you’ve enabled backups, you can always restore afterwards if you want to undo the changes.

---

## Main Window

The main window is where you pick what to tidy and choose the basic options. It’s meant to make the common “tidy this folder” job quick, without getting in the way.

### Folder Section

- **Folder Path:** Shows the Amiga path of the folder that will be processed (for example `SYS:`, `Work:Projects`, or `DH0:Games`). It’s read-only and updates when you select a new folder.

- **Browse... Button:** Opens a file requester so you can choose the folder you want to tidy. The requester only shows drawers (folders), not files. If you prefer, you can also type a path directly. The last selected path is remembered for the current session.

### Tidy Options Section

- **Order:** Sets how icons are grouped and sorted in the window. The choices are Folders First, Files First, or Mixed.
  - *Folders First:* Drawer icons are listed before file icons.
  - *Files First:* File icons are listed before drawer icons.
  - *Mixed:* Folders and files are sorted together by name.

- **Cleanup subfolders:** If enabled, iTidy will recurse through all subfolders under the selected path. This is the option you’d use for tidying a whole partition or a large folder tree in one pass. On classic hardware, very large trees can take a while, so it’s normal to start it and leave it running.

- **Backup icons:** If enabled, iTidy creates an LhA archive of the relevant `.info` files before making changes. That gives you something to roll back to later using **Restore Backups...** from the main window. This requires LhA to be available in `C:`. (This backup is about icon/layout changes; Default Tool backups are handled separately when fixing default tools.)

- **Position:** Controls where drawer windows end up after iTidy resizes them.
  - *Center Screen:* Centres the window on the Workbench screen (default).
  - *Keep Position:* Keeps the window at its current location.
  - *Near Parent:* Places the window slightly down and right of the parent window (similar to how Workbench tends to cascade windows).
  - *No Change:* Resizes the window but doesn’t try to move it.

### Other Controls

- **Start Button:** Begins processing using the current settings.

- **Advanced...:** Opens the Advanced Settings window for more detailed control.

### Menu Options

The menu lets you save and load your settings:

- **Load Settings:** Loads previously saved preferences for layout, sorting, and backup options.
- **Save Settings:** Saves your current configuration so you can reuse it later.

This is handy if you want one setup for general Workbench use and another for things like WHDLoad folders or archive dumps.

### Tips

- The main window is meant for quick “point it at a folder and tidy it” runs. If you need more control over layout and window sizing, use **Advanced...**.
- All changes apply to the selected folder and (if enabled) everything underneath it.
- If you’re experimenting, enable backups and start with a small test folder first.


---

## Advanced Settings

You can open this window via **Advanced...** on the main window. These options are there if you want a bit more control over how icons are laid out and how drawer windows are sized.

### Layout Aspect Ratio

Sets the target width-to-height “shape” for drawer windows.

- *Tall (0.75):* Very tall, narrow windows
- *Square (1.0):* Roughly square windows
- *Compact (1.3):* Slightly wider than tall
- *Classic (1.6):* More traditional Workbench proportions
- *Wide (2.0):* Wider windows (default)
- *Ultrawide (2.4):* Very wide windows

### Overflow Strategy

Controls what iTidy does when a drawer has more icons than comfortably fit on screen.

- *Expand Horizontally:* Adds more columns (you’ll scroll left/right) (default)
- *Expand Vertically:* Adds more rows (you’ll scroll up/down)
- *Expand Both:* Tries to balance expansion in both directions

### Icon Spacing (X and Y)

Sets the horizontal and vertical gap between icons (0–20 pixels). The default is 8 pixels in both directions. Lower values pack icons tighter; higher values give things more breathing room.

### Icons per Row (Min/Max)

- **Min:** The minimum number of columns (default: 2). This stops drawers turning into a single long vertical list.
- **Max:** The maximum number of columns (only used when Auto Max is disabled).
- **Auto Max Icons:** When enabled (default), iTidy works out a sensible maximum column count based on your screen width.

### Max Window Width

Limits how wide drawer windows are allowed to become, as a percentage of the screen width (Auto, 30%, 50%, 70%, 90%, 100%). This only applies when **Auto Max Icons** is enabled.

### Align Icons Vertically

Sets how icons are vertically aligned when a row contains icons of different heights: *Top*, *Middle* (default), or *Bottom*.

### Reverse Sort (Z→A)

Reverses the current sort direction (for example Z→A, newest-first, or largest-first depending on the sort mode).

### Optimize Column Widths

When enabled (default), each column is sized based on the widest icon in that column rather than forcing every column to use the same width.

### Column Layout (centered)

When enabled (default), icons are centered within their column rather than being left-aligned.

### Skip Hidden Folders

When enabled (default), iTidy skips folders that don’t have `.info` files when doing recursive processing. This helps avoid pointless work when you’re running across large trees.

### Strip NewIcon Borders

Removes borders from NewIcons during processing. This requires icon.library v44+ (Workbench 3.5+). This change is permanent for the affected icons, so if you might want to go back to the original look later, enable icon backups first.

---

## Default Tool Analysis

You can open this window via **Fix Default Tools...** on the main window. It scans WBPROJECT icons and helps you find (and fix) default tools that no longer exist.

### What is a Default Tool?

On the Amiga, most data file icons have a “Default Tool” set - the program Workbench runs when you double-click that icon. For example, a `.txt` icon might have `SYS:Utilities/MultiView` as its default tool.

After copying icons between systems or extracting older archives, those tool paths often don’t match what’s actually installed. When that happens you’ll see the familiar “Unable to open your tool” error.

### Using the Scanner

1. **Select folder:** Click **Browse...** and choose a folder to scan. Scanning is always recursive (it will include subfolders).
2. **Scan:** Click **Scan** to examine icons and check whether their default tools can be found.
3. **Review:** The list shows each default tool that was found, how many icons reference it, and whether it exists.
4. **Filter:** Use the cycle gadget to show All, Existing only, or Missing only.
5. **Fix:** If a tool is missing, select it and use **Replace Tool (Batch)** to update all icons that use it. If you only want to change one file, select that file and use **Replace Tool (Single)**.

### Tool List Columns

- **Tool:** The default tool path (for example `SYS:Tools/TextEdit`)
- **Files:** How many icons reference that tool
- **Status:** EXISTS or MISSING

### Menu Options

- **Project menu:** New, Open, Save, Save As, Close. Saving scan results can be handy when you’re working with large drives that take a while to scan.
- **File menu:** Export the tool list or a detailed per-file list to text files.

### Default Tool Backups

Before iTidy changes anything, it automatically saves the original default tool values as small text backups. This backup system is separate from the icon/layout backups used for tidying.

To undo default tool changes, use **Restore Default Tools Backups...** from within this window. (This only restores default tool settings - it won’t restore icon positions or window layouts.)

### Technical Notes

When validating tools, iTidy checks absolute paths directly. For simple tool names like `MultiView`, it searches the Amiga PATH (parsed from `S:Startup-Sequence` and `S:User-Startup`) to see if the tool can be found there.

---

## Restoring Backups

You can open this window via **Restore Backups...** on the main window. It restores icon positions and drawer/window layout information from the LhA backup archives that iTidy created. This is only available if you previously ran iTidy with **Backup icons** enabled.

Note: this restores icon/layout backups only. If you’re trying to undo default tool changes, use **Restore Default Tools Backups...** in the Fix Default Tools window instead.

### Run List

The top list shows previous backup runs:

- **Run:** A simple run number (0001, 0002, etc.)
- **Date/Time:** When the backup was created
- **Folders:** How many folders were backed up
- **Size:** Total size of the archives
- **Status:** Complete or Incomplete

Click a run to select it and view its details.

### Details Panel

Shows information about the selected run, including the source directory, how many archives were created, total size, status, and where the backups are stored.

### Controls

- **Restore window positions:** When enabled (default), iTidy also restores drawer window geometry (size and position). Depending on your setup, you may need to restart Workbench before everything visibly updates.

### Buttons

- **Restore Run:** Extracts the archived `.info` files back to their original locations, restoring icon positions and (optionally) window geometry.
- **Delete Run:** Permanently deletes the selected backup run. This can’t be undone, so only delete runs you’re sure you no longer need.
- **View Folders...:** Shows the list of folders included in the selected run.
- **Cancel:** Closes the window without making changes.

### What Gets Restored

- The `.info` files that were backed up
- Icon positions (X/Y coordinates)
- Window geometry (if enabled)
- Folder view mode settings

### Requirements

- LhA must be installed in `C:`
- Backup archives are stored in `PROGDIR:Backups/`
- Enough free disk space to extract the archives

---

## Tips & Troubleshooting

### General Tips

- **Enable backups for big runs:** If you’re tidying a lot of folders (especially with recursion enabled), turning on backups gives you a way back if you don’t like the result.
- **Start small:** Try a single drawer first so you can see how the settings affect icon layout and window sizing before you point it at a whole partition.
- **Save your settings:** Once you’ve found a setup you like, use **Save Settings** from the menu so you can load it again later.
- **Live preview trick:** Keep a drawer window open while you run iTidy on that same drawer. You can quickly see how the layout looks and tweak settings before doing a long recursive run. While iTidy is working through a drawer, icons can briefly overlap until it has repositioned everything. For best results, let it finish the current drawer before cancelling. Once a drawer is complete, it’s generally safe to stop a long run if you’ve seen enough. Workbench may cache window sizing/position until you close and reopen the drawer, but icon placement can be checked straight away.
- **To see window changes clearly:** Close any drawers you’re tidying before you run iTidy. After it finishes, you may need to restart Workbench (or reboot) to see updated window sizes and positions everywhere.

### Icons Appear Misaligned

If some icons look misaligned after tidying while others look fine, it’s often down to icon.library support rather than iTidy itself.

On Workbench 3.0/3.1 with older icon.library versions, OS3.5+ colour icons may not render at their real size and can appear as tiny placeholder images. iTidy positions icons using the *actual* icon dimensions, but Workbench may draw them at the wrong size, which makes the layout look “off”.

**What to do:**
- Update icon.library to v44+ if you want to use OS3.5+ colour icons (typically via OS3.5/3.9/3.2, or an updated library from Aminet).
- Alternatively, convert colour icons back to NewIcons or classic icons if you’re keeping a more stock 3.0/3.1 setup.

### Window Positions Not Updating

If you restore a backup (or run iTidy) and window positions don’t appear to change right away, this is usually Workbench caching.

**What to do:**
- Close and reopen the affected drawer windows.
- If it still doesn’t look updated, restart Workbench or reboot.

### If Backup or Restore Doesn’t Work as Expected

A few quick things to check:

- LhA is installed in `C:`
- You have enough free disk space for archives (backups) or extraction (restores)
- The original folder paths still exist when restoring
- Check `PROGDIR:logs/` for any error details

### Slow Processing on Large Folder Trees

Recursing through thousands of folders takes time on real hardware, especially with mechanical drives.

A few tips:
- It’s fine to leave iTidy running unattended during large jobs.
- Avoid running other disk-heavy tasks at the same time.
- If you’re doing a huge collection, it can be quicker (and safer) to work in chunks (for example `WHDLoad:Games` first, then `WHDLoad:Demos`) rather than doing everything in one run.

### Icons aren't moved, and the progress log shows no icons found.

Check the folder in Workbench and see if the menu `Window->Show->All Files` is selected. This option shows the contents of the folder by giving each item a temporary icon, which won’t be visible to iTidy.

If you want iTidy to tidy this folder, you should create real icons first. Select the folder’s contents via `Window->Select Contents`, then choose `Icons->Snapshot`. Default icons will be created, and iTidy will be able to process them.

## iTidy is skipping some folders that contain valid icons

By default, iTidy will skip processing folders if the parent folder doesn’t have an icon.

To bypass this, go to Advanced Settings and untick "Skip hidden folders"

### Icons Don’t Open (“Unable to open your tool”)

This usually means an icon’s Default Tool points to something that isn’t installed on your system.

**What to do:**
- Use **Fix Default Tools...** to scan for missing tools and repair them.
- See the Default Tool Analysis section for how the scanner works.

---

### Safety & Disclaimer

iTidy is free hobby software that I work on in my spare time. It’s been tested on real Amiga systems, but it may still have bugs or edge cases I haven’t run into yet.

iTidy only touches `.info` files and Workbench drawer/window layout information. It also includes an optional backup system to help roll back changes, but you shouldn’t treat that as a replacement for your own regular system backups (and it’s still being improved).

If you use iTidy, you do so at your own risk. I can’t accept responsibility for data loss, corruption, or any other damage caused by using (or misusing) the software.

Before running iTidy for the first time - especially on large or important partitions - make sure you have a current, verified backup. If you want to be extra cautious, try it on a small test folder or a copy of your Workbench setup first.

---

## Credits & Version Info

**Author:** Kerry Thompson  
**Version:** 1.0
**Website:** https://github.com/Kwezza/iTidy
---
