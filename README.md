# iTidy

**An automated Workbench icon layout and window tidy tool for AmigaOS 3.x**

![AmigaOS](https://img.shields.io/badge/AmigaOS-3.x-blue.svg)
![License](https://img.shields.io/badge/license-See%20LICENSE-green.svg)
![Version](https://img.shields.io/badge/version-2.0-orange.svg)

iTidy is a Workbench utility that automatically arranges icon layouts and resizes folder windows across entire directory trees. Built specifically for AmigaOS 3.0/3.1 (Workbench 3.x), it solves the tedious problem of manually cleaning up hundreds or thousands of icons after large archive extractions, especially useful for WHDLoad collections, game libraries, and project directories.

## Features

### Core Functionality
- 🎯 **Automatic Icon Grid Layout** - Intelligent icon positioning with configurable spacing and sorting
- 📐 **Smart Window Resizing** - Aspect ratio control with overflow handling for large directories
- 🔄 **Recursive Processing** - Process entire directory trees in one operation
- 💾 **LHA Backup System** - Create restore points before making changes
- 🛠️ **Default Tool Validation** - Find and fix missing or invalid default tool paths
- 📊 **Multiple Sort Options** - Folders First, Files First, Mixed, with reverse sorting

### Advanced Options
- Configurable icon spacing (X/Y)
- Multiple aspect ratios (Tall, Square, Compact, Classic, Wide, Ultrawide)
- Overflow strategies (horizontal, vertical, or balanced expansion)
- Column width optimization
- Icon vertical alignment control (top, middle, bottom)
- Min/max icons per row with auto-calculation
- Max window width limits (% of screen)
- Hidden folder filtering
- NewIcons border stripping (requires icon.library v44+)

### Supported Icon Formats
- ✅ Classic/Standard icons (all AmigaOS versions)
- ✅ NewIcons (extended color icons)
- ✅ OS3.5/OS3.9 Color Icons
- ✅ GlowIcons (both NewIcons-style and Color Icons versions)

## Requirements

- **OS**: AmigaOS Workbench 3.0 or newer
- **Memory**: At least 1MB free RAM
- **Storage**: At least 1MB free disk space (more for backups)
- **Optional**: LhA (required for backup/restore features)
  - Must be installed in `SYS:C` path
  - Download from [Aminet](http://aminet.net/package/util/arc/lha)

## Installation

1. Extract the iTidy archive to your desired location (e.g., `SYS:Utilities/iTidy`)
2. Ensure LhA is installed if you want to use backup features
3. Double-click the iTidy icon to launch

## Quick Start

1. **Launch iTidy** - Double-click the iTidy icon on Workbench
2. **Select Folder** - Click "Browse..." and choose the folder to tidy
3. **Choose Options**:
   - Set icon order (Folders First, Files First, or Mixed)
   - Enable "Cleanup subfolders" for recursive processing
   - Enable "Backup icons" to create restore points
4. **Click Start** - iTidy processes the folder(s) and arranges icons
5. **Restore if Needed** - Use "Restore Backups..." to undo changes

💡 **Tip**: Try iTidy on a small test folder first to see how the options work!

## Main Features Explained

### Recursive Processing
Process entire directory trees in one operation - perfect for tidying large WHDLoad collections, archive extractions, or project folders with many subdirectories.

### Backup & Restore System
- Creates LHA archives of `.info` files before making changes
- Restore previous layouts with one click
- View and manage backup sessions
- Separate backup system for default tool changes

### Default Tool Analysis
Scan directories for icons with missing or invalid default tools and fix them:
- Identify broken default tool paths
- Batch replace tools across multiple icons
- PATH-based tool resolution
- Automatic backup before changes

### Smart Window Sizing
- Multiple aspect ratio presets
- Configurable overflow handling
- Screen width limits
- Automatic column calculation
- Window positioning options (center, keep, near parent, no change)

## Documentation

For detailed instructions, see [docs/manual/README.md](docs/manual/README.md)

Topics covered:
- Main window controls
- Advanced settings detailed guide
- Default tool analysis and fixing
- Backup and restore procedures
- Tips and troubleshooting
- Known issues and workarounds

## Development

iTidy is written in C89/C99 for AmigaOS 3.x using:
- **Compiler**: VBCC cross-compiler (v0.9x)
- **SDK**: Amiga SDK 3.2 (targeting Workbench 3.0 compatibility)
- **Target**: 68000/68020 (no FPU/MMU required)
- **GUI**: Native GadTools (Workbench 3.0 gadgets only)

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

For development details, see `.github/copilot-instructions.md` and `docs/DEVELOPMENT_LOG.md`.

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

⚠️ **Important**: iTidy is hobby software provided as-is. While tested on real Amiga systems, bugs may exist.

- iTidy only modifies `.info` files and Workbench drawer/window layout information
- Your data files are never touched
- The backup system helps roll back changes, but is not a replacement for regular backups
- **Always maintain proper system backups before processing important directories**
- Try on test folders first, especially for large operations

**Use at your own risk.** The author accepts no responsibility for data loss, corruption, or other issues.

## Tips & Troubleshooting

### Icons Appear Misaligned
Update icon.library to v44+ for proper OS3.5+ color icon support, or convert icons to NewIcons/classic format.

### Window Positions Not Updating
Close and reopen drawer windows, or restart Workbench to see position changes.

### Slow Processing on Large Trees
Processing thousands of folders takes time on real hardware - this is normal. Process in chunks for very large collections.

### "Unable to open your tool" Errors
Use the "Fix Default Tools..." feature to scan for and repair missing default tool paths.

For more troubleshooting help, see the [user manual](docs/manual/README.md#tips--troubleshooting).

## Version History

**v2.0** - Complete GUI rewrite
- Full Workbench GUI interface (no CLI version)
- Advanced layout options with aspect ratio control
- LHA-based backup and restore system
- Default tool validation and repair
- Recursive directory processing
- Performance optimized for low-memory systems (A500/A600/A1200)

## Credits

**Author**: Kerry Thompson  
**GitHub**: https://github.com/Kwezza/iTidy

## License

See [LICENSE](LICENSE) file for details.

---

Made with ❤️ for the Amiga community
