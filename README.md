# ITidy

ITidy is an Amiga CLI tool that tidies up icons, then centres and resizes windows to fit the icons.

## Features

- Iterates through a directory and its subdirectories, cleaning up icons
- Can resize and centre windows to fit icons relative to the screen resolution
- Supports Workbench 2.0 and higher
- Takes the current font settings into account when working out icon widths
- Considers any custom border, window, and title settings when calculating the window size
- Has the option to skip WHDLoad folders if the author's layout is preferred (The window will still be resized and centred)
- Can set folder view mode to View by icon, or view by name only

I wrote this program because I wasn't happy with the default placement of icons when copied or extracted from archives. Also, I was not too fond of opening folder after folder with lots of icons and then having to scroll or resize the window to see them all. Although Workbench has built-in tools to tidy up folders, I needed something that could organize multiple folders at once. Originally designed to be used with an update to WHDArchiveExtractor (for extracting WHDLoad archives en masse), I have since found it useful for keeping my system tidy and decided to release it for others to, hopefully, find it useful. As it runs from the CLI, it can be incorporated into larger scripts that preinstall other software and then used to tidy up the icons and windows afterwards.

## Usage

The program is designed to be run from the CLI and takes the following arguments:
```
`Usage: ITidy <directory> -iterateSubDIRs -dontResizeWindow -folderViewShowAll -folderViewDefault -folderViewByName -folderViewByType -dontCleanupWHDFolders
```
Where:

- `<directory>` is the folder to start the cleanup from
- `-subdirs` Walk through all folders in the parent folder and recursively apply the operations.
- `-dontResize` Do not resize and centre the folder window to fit after arranging icons.
- `-ViewShowAll` Set the Workbench folder view to show all files, including those without icons.
- `-ViewDefault` Set the Workbench folder view to the default mode, inheriting the parent folder's view mode.
- `-ViewByName` Set the Workbench folder view to text-only mode, sorted by name (no icons).
- `-ViewByType` Set the Workbench folder view to text-only mode, sorted by type (no icons). Useful for sorting directories first.
- `-resetIcons` Remove each icon's x and y positions, instructing Workbench to place the icon automatically when opening the folder.
- `-skipWHD` By default, the program skips rearranging icons in WHDLoad folders to preserve the original author's layout. This option forces the program to rearrange the icons even if a .slave file is in the folder. Without this option, the folder's window will only be resized and centred.

**Note:** If the directory contains spaces in the name, it will need to be enclosed in two speech marks "DH0:My DIR"

## Example 
```
ITidy PC:Extracted -iterateSubDIRs
```

In this example, the folder "PC:Extracted" will be tidied up, its window will be resized and centred, and all subfolders will be tidied.  If any folders are found to be WHDLoad game folders, the original icon layout by the author is preserved and the folder window will be resized and centred.

## Limitations

I have been using this program on Workbench 3.2 and have only done limited testing on Workbench 2.
Please let me know if you have any issues via email, or better still, open a ticket on the project's GitHub page.
As always, and although it works for me, this program is provided as is, and I take no responsibility for any issues it may cause.
Please back up your system before running this program.

## Changelog

**v1.0.0 - 2024-07-11** - First public release.
