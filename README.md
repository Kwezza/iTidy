# Icon Cleanup

Icon Cleanup is an Amiga CLI tool that tidies up icons, then centers and resizes windows to fit the icons.

## Features

- Iterates through a directory and its subdirectories, cleaning up icons
- Can resize and center windows to fit icons relative to the screen resolution
- Supports Workbench 2.0 and higher
- Takes the current font settings into account when working out icon widths
- Considers any custom border, window, and title settings when calculating the window size
- Has the option to skip WHDLoad folders if the author's layout is preferred (Window will still be resized and centered)
- Can set folder view mode to View by icon, or view by name only

I wrote this program because I wasn't happy with the default placement of icons when copied or extracted from archives.
Also, I was not too fond of opening folders with a few icons and then having to scroll or resize the window to see them all.
Originally designed to be used with an update to WHDArchiveExtractor (for extracting WHDLoad archives en masse), I have since
found it useful for keeping my system tidy and decided to release it for others to, hopefully, find it useful. As it runs from
the CLI, it can be incorporated into larger scripts that preinstall other software and then used to tidy up the icons and
windows afterwards.

## Usage

The program is designed to be run from the CLI and takes the following arguments:
Usage: IconCleanup <directory> -iterateSubDIRs -dontResizeWindow -folderViewShowAll -folderViewDefault -folderViewByName
-folderViewByType -dontCleanupWHDFolders

Where:

- `<directory>` is the folder to start the cleanup from
- `-iterateSubDIRs` will walk through any subfolders from the parent folder
- `-dontResizeWindow` will not resize and center the Workbench window to fit the icons
- `-folderViewShowAll` will set the Workbench folder view to show all files, even those without icons
- `-folderViewDefault` will set the Workbench folder view to default (inherit parent's view mode)
- `-folderViewByName` will set the Workbench folder view as text, sorted by name
- `-folderViewByType` will set the Workbench folder view as text, sorted by type
- `-dontCleanupWHDFolders` will force the program to maintain the icon position in WHDLoad folders but will resize and center the window to fit the icons

I have been using this program on Workbench 3.2 and have only done limited testing on Workbench 2.
Please let me know if you have any issues via email, or better still, open a ticket on the GitHub page.
As always, although it works for me, this program is provided as is, and I take no responsibility for any issues it may cause.
Please back up your system before running this program.

## Changelog

**v1.0.0 - 2024-07-11** - First public release.
