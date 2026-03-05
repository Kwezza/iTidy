# iTidy

**A Workbench icon and window tidy tool for AmigaOS 3.2+**

![AmigaOS](https://img.shields.io/badge/AmigaOS-3.2%2B-blue.svg)
![License](https://img.shields.io/badge/license-See%20LICENSE-green.svg)
![Version](https://img.shields.io/badge/version-2.0-orange.svg)

Workbench drawers have a way of drifting into chaos over time, especially after large Aminet extractions, cover disks, or years of deferred tidying. iTidy is a small utility that brings things back into shape. Point it at a folder or an entire volume and it will tidy icon layouts and drawer windows in a consistent, repeatable way, with optional recursion through subfolders.

Version 2.0 adds a Workbench 3.2 ReAction GUI plus DefIcons-based icon creation, so files without icons can be handled automatically. Where supported, it can also generate thumbnail-style icons for images and text-preview icons for source files. A backup and restore system using LhA lets you roll back any run if you change your mind.

## Versions

- **v2.x (Current)**: ReAction GUI for Workbench 3.2+. Modern BOOPSI-based interface with icon creation, thumbnail generation, and DefIcons integration.
- **v1.0-gadtools**: GadTools GUI for Workbench 3.0/3.1. Legacy stable version ([access here](https://github.com/Kwezza/iTidy/releases/tag/v1.0-gadtools))

## Requirements

- AmigaOS Workbench 3.2 or newer (v2 requires ReAction from WB 3.2)
- 68000 CPU or better
- At least 1 MB free RAM (more recommended for large folders, recursion, and icon creation)
- At least 1 MB free storage space in the installation location (more as backups accumulate)
- For backup and restore features: LhA must be installed in `C:`

## Features

### Icon Layout

- Automatic icon grid layout with configurable spacing and sorting
- Sort by name, type, date, or size; reverse sort supported
- Grouping modes: Folders First, Files First, Mixed, or Grouped By Type (with configurable gap between groups)
- Configurable icon spacing (horizontal and vertical)
- Multiple aspect ratio presets (Tall, Square, Compact, Classic, Wide)
- Overflow strategies: expand horizontally, vertically, or balanced
- Column width auto-fitting
- Icon vertical alignment (top, middle, bottom)
- Min/max icons per row with auto-calculation
- Max window width limit as a percentage of screen width
- Window positioning (Centre Screen, Keep Position, Near Parent, No Change)
- Strip NewIcons borders (requires icon.library v44+)
- Skip drawers without icons (no `.info` file)

### Icon Creation (DefIcons Integration)

- Creates new icons for files and folders that do not already have them, using the DefIcons system
- Thumbnail icons for image files (ILBM/IFF, PNG, GIF, BMP, ACBM; JPEG optional)
- Text preview icons for source files, scripts, and other ASCII content
- Configurable preview size (48x48, 64x64, 100x100)
- Configurable colour depth (4 to 256 colours), dithering, palette, and border style
- Per file-type enable/disable via the DefIcons Categories window
- Exclude paths list to skip directories during icon creation
- Folder icon creation (Never, Always, or Smart mode)
- WHDLoad folder protection (skip icon creation inside WHDLoad game directories)
- Replace previously generated thumbnails or text previews on subsequent runs

### Default Tool Analysis

- Scan directories for icons with missing or invalid default tool paths
- Batch replace a tool across all icons that reference it
- Single-icon replacement mode
- PATH-based tool resolution
- Automatic backup before changes
- Restore previous default tool settings via a session-based backup system
- Save and load the tool cache between sessions
- Export tool and file reports as text

### Backup and Restore

- LhA-based backups of `.info` files before making layout changes
- Session-based restore: restore icons only, or icons and window positions together
- Folder View window shows the hierarchical contents of any backup run
- Separate backup system for default tool changes
- DefIcons-created icons tracked and removed during restore

### Other

- Recursive directory processing in one pass
- Progress window with per-folder status during processing
- Preset save/load system for storing different configurations
- LOADPREFS ToolType to auto-load a preset at startup
- Logging system with configurable levels (Debug, Info, Warning, Error, Disabled)
- Performance logging option for benchmarking

## Supported Icon Formats

- Classic/Standard icons (all AmigaOS versions)
- NewIcons (extended colour icons)
- OS3.5/OS3.9 colour icons
- GlowIcons (both NewIcons-style and colour icon versions)

## Installation

1. Extract the iTidy archive to your desired location (e.g. `SYS:Utilities/iTidy`)
2. Ensure LhA is installed in `C:` if you want to use backup and restore features
3. Double-click the iTidy icon to launch

## Quick Start

1. Launch iTidy by double-clicking the icon on Workbench.
2. Click **Folder to tidy** and choose the drawer or volume you want to process.
3. Choose how icons are grouped using the **Grouping** chooser (Folders First is the default).
4. Enable **Include Subfolders** if you want to process the entire folder tree.
5. Enable **Back Up Layout Before Changes** if you want a restore point (requires LhA in `C:`).
6. Click **Start**. A progress window shows what iTidy is doing while it works.
7. Use **Restore Backups...** to undo changes if needed.

For your first run, try a small test folder. If you enable backups, you can always restore afterwards.

## Main Window Options

- **Grouping** -- Folders First, Files First, Mixed, or Grouped By Type
- **Sort By** -- Name, Type, Date, or Size (disabled when Grouped By Type is selected)
- **Include Subfolders** -- Recurse through the entire folder tree
- **Create Icons During Tidy** -- Generate icons for files without them using DefIcons
- **Back Up Layout Before Changes** -- Create an LhA restore point before each run
- **Window Position** -- Centre Screen, Keep Position, Near Parent, or No Change
- **Advanced...** -- Opens the Advanced Settings window (layout, density, limits, columns, filters)
- **Icon Creation...** -- Configure thumbnail generation, text previews, and DefIcons categories
- **Fix Default Tools...** -- Scan and repair missing default tool paths
- **Restore Backups...** -- Restore a previous layout backup run

## ToolTypes

These ToolTypes are read from the iTidy program icon when launched from Workbench:

| ToolType | Values | Default | Description |
|---|---|---|---|
| `DEBUGLEVEL` | 0-4 | 4 | Log level: 0=Debug, 1=Info, 2=Warning, 3=Error, 4=Disabled |
| `LOADPREFS` | file path | none | Auto-load a saved preferences file at startup |
| `PERFLOG` | YES / NO | NO | Enable performance timing logs |

## Documentation

Full documentation is in [docs/manual/iTidy.md](docs/manual/iTidy.md), covering all windows, settings, ToolTypes, backup systems, and troubleshooting.

Manual topics:
- Main window controls
- Advanced settings guide
- Default tool analysis and fixing
- Backup and restore procedures
- Tips and troubleshooting
- Known issues and workarounds

## Development

iTidy is written in C for AmigaOS 3.2+ using:
- **Compiler**: VBCC cross-compiler (v0.9x)
- **SDK**: AmigaOS 3.2 SDK
- **Target**: 68000
- **GUI**: Native ReAction gadgets (Workbench 3.2)

### Building from Source

```bash
# Clean build
make clean && make

# Incremental build
make

# Build with console output (debugging)
make CONSOLE=1
```

Build output appears in `build/amiga/` and final binaries in `Bin/Amiga/`.

## Project Structure

```
iTidy/
├── Bin/Amiga/           # Compiled executables and runtime logs
├── docs/                # Documentation
│   └── manual/          # User manual
├── include/             # Header files
├── src/                 # Source code
│   ├── GUI/             # GUI window modules
│   ├── Settings/        # Preferences handling
│   └── templates/       # Code templates for development
├── ProjectDocs/         # Development documentation
└── Makefile             # Build configuration
```

## Safety & Disclaimer

**Important**: iTidy is hobby software provided as-is. While tested on real Amiga systems, bugs may exist.

- iTidy only modifies `.info` files and Workbench drawer/window layout information
- Your data files are never touched
- The backup system helps roll back changes, but is not a replacement for regular backups
- Always maintain proper system backups before processing important directories
- Try on test folders first, especially for large operations

**Use at your own risk.** The author accepts no responsibility for data loss, corruption, or other issues.

## Credits

**Author:** Kerry Thompson  
**Website:** https://github.com/Kwezza/iTidy  
**Special thanks:** Darren "dmcoles" Cole for ReBuild, an excellent GUI builder that made the updated interface possible.

## License

See [LICENSE](LICENSE) for details.
