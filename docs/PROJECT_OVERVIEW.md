# iTidy - Project Overview

## Project Summary

**iTidy** is a command-line utility for the Amiga operating system that automatically organizes and tidies up icon layouts in Workbench folders. It's designed to solve the common frustration of messy icon placement when files are copied from archives or other sources.

**Author**: Kerry Thompson  
**Version**: 1.0.0  
**Release Date**: July 11, 2024  
**License**: See LICENSE file  
**Language**: C  
**Platform**: Amiga (Workbench 2.0+)

---

## Main Purpose

iTidy addresses a specific pain point in Amiga Workbench usage: when you extract files from archives or copy folders, icons often appear in random, messy positions, and folder windows may be too small to display all icons without scrolling. iTidy automates the process of organizing these icons and resizing windows to fit them properly.

The tool was originally created to work with WHDArchiveExtractor (for mass-extracting WHDLoad game archives), but has proven useful for general system maintenance and can be integrated into larger installation scripts.

---

## Interface Type

**iTidy is a CLI-only application** - it runs exclusively from the Amiga Command Line Interface and does **not** have a Workbench GUI.

### CLI-Only Characteristics:

- Uses standard `main(int argc, char **argv)` with command-line argument parsing
- No Workbench startup message handling
- Text-based output using `printf()` statements
- Designed for shell execution and scripting integration
- No graphical user interface or window controls

### Workbench Integration:

While iTidy itself runs from CLI, it **modifies Workbench elements**:
- Updates `.info` files (icon metadata)
- Adjusts folder window positions and sizes
- Changes view modes and display settings
- Users see the results when opening folders in Workbench GUI

**Usage Pattern**: Run iTidy from Shell/CLI → Changes are applied to filesystem → Users see organized folders when browsing in Workbench

---

## Key Features

### 1. **Icon Organization**
- Automatically arranges icons in a neat grid layout
- Calculates optimal icon spacing based on icon sizes and text labels
- Supports multiple icon formats (standard, NewIcons, OS3.5)

### 2. **Intelligent Window Resizing**
Calculates optimal window dimensions considering:
- Number and dimensions of icons in the folder
- Current screen resolution (reads Workbench screen settings)
- User's font preferences from Workbench settings
- Custom window borders and title bar heights from IControl preferences
- Scrollbar dimensions
- Screen estate limitations

### 3. **Recursive Directory Processing**
- Can process entire directory trees with `-subdirs` flag
- Walks through nested folder hierarchies
- Processes each folder independently

### 4. **WHDLoad-Aware Behavior**
- Detects WHDLoad game folders (by presence of `.slave` files)
- Can preserve original author's icon layout in WHDLoad folders (default)
- Still resizes and centers windows even when skipping icon rearrangement
- Option to override and rearrange WHDLoad folders if desired

### 5. **Multiple Icon Format Support**
- **Standard Amiga icons**: Classic icon format
- **NewIcons**: Enhanced color icons
- **OS3.5 style icons**: Color icon format from OS3.5+
- Tracks and reports which icon types were processed

### 6. **Flexible View Mode Control**
Set folder display modes:
- View by icon (default)
- List by name (text-only, sorted alphabetically)
- List by type (text-only, directories first)
- Show all files (including those without icons)
- Default view (inherit from parent folder)

### 7. **Icon Position Management**
- Can reset icon positions to default (removes saved coordinates)
- Workbench will auto-place icons when folder is opened
- Useful for completely reorganizing cluttered folders

### 8. **Error Tracking and Reporting**
- Identifies corrupted or unreadable icon files
- Maintains list of problematic files
- Provides detailed error report at completion
- Suggests remediation actions

### 9. **Device Protection**
- Checks if target device is write-protected before processing
- Prevents accidental attempts to modify read-only media
- Clean error messaging for protected devices

### 10. **Performance Monitoring**
- Tracks processing time
- Displays completion statistics
- Reports icon counts by type
- Shows any errors encountered

---

## System Requirements

- **Minimum**: Amiga with Workbench 2.0 (Kickstart 37+)
- **Recommended**: Workbench 3.0 or higher
- **Note**: Some features have limited support on Workbench 2.x
  - Icon spacing is increased on WB 2.x (15x10 vs 9x7)
  - Some icon.library functions are restricted

---

## Command-Line Usage

### Basic Syntax
```
iTidy <directory> [options]
```

### Options

| Option | Description |
|--------|-------------|
| `-subdirs` | Recursively process all subfolders |
| `-dontResize` | Skip window resizing and centering (only tidy icons) |
| `-viewShowAll` | Show all files, including those without icons |
| `-viewDefault` | Use default view settings (inherit from parent) |
| `-viewByName` | Display as text list sorted by name |
| `-viewByType` | Display as text list sorted by type (directories first) |
| `-resetIcons` | Remove saved icon positions (force auto-placement) |
| `-skipWHD` | Force icon rearrangement even in WHDLoad folders |
| `-forceStd` | Use classic icon sizes only (for WB2 compatibility) |
| `-xPadding:N` | Override horizontal icon spacing (0-30 pixels) |
| `-yPadding:N` | Override vertical icon spacing (0-30 pixels) |
| `-fontName:name.font` | Override icon font name |
| `-fontSize:N` | Override icon font size (1-30) |

### Examples

```bash
# Tidy a single folder
iTidy Work:Projects

# Recursively tidy all subfolders
iTidy Work:Projects -subdirs

# Tidy and change view mode to text list
iTidy DH0:Downloads -viewByName -viewShowAll

# Reset all icon positions and reorganize
iTidy Games: -subdirs -resetIcons

# Process WHDLoad folders with custom spacing
iTidy WHDLoad:Games -subdirs -skipWHD -xPadding:12 -yPadding:8
```

---

## Technical Architecture

### Programming Language
- **C** (ANSI C with Amiga-specific extensions)
- Compiled with SAS/C or similar Amiga C compiler

### Key Amiga Libraries Used
- `icon.library` - Icon file manipulation
- `intuition.library` - Screen and window management
- `workbench.library` - Workbench integration
- `graphics.library` - Screen dimensions and graphics info
- `dos.library` - File system operations
- `diskfont.library` - Font handling
- `iffparse.library` - IFF file parsing (for preferences)

### Core Components

#### Source Files Structure
```
src/
├── main.c                    # Entry point and main logic
├── cli_utilities.c          # Command-line argument parsing
├── file_directory_handling.c # Directory traversal
├── icon_management.c        # Icon file operations
├── icon_misc.c              # Icon utility functions
├── icon_types.c             # Icon format detection
├── window_management.c      # Window sizing calculations
├── utilities.c              # General utilities
├── spinner.c                # Progress indicator
├── writeLog.c               # Debug logging
├── Settings/
│   ├── IControlPrefs.c      # Window border preferences
│   ├── WorkbenchPrefs.c     # Workbench settings
│   └── get_fonts.c          # Font preference handling
└── DOS/
    └── getDiskDetails.c     # Device information
```

### Key Algorithms

#### 1. Icon Grid Layout Algorithm
- Calculates icon width (max of icon image width and text width)
- Accounts for icon borders and spacing
- Arranges icons left-to-right, top-to-bottom
- Creates rows based on available window width

#### 2. Window Size Calculation
```
Window Width = (number of columns × icon width) + padding + borders + scrollbar
Window Height = (number of rows × icon height) + padding + title bar + borders
```

#### 3. Screen Resolution Awareness
- Reads Workbench screen dimensions
- Ensures windows don't exceed screen boundaries
- Centers windows on screen

#### 4. Preference File Parsing
- Reads IFF-formatted preference files
- Extracts font settings, border widths, title bar heights
- Falls back to defaults if preferences unavailable

### Memory Management
- Uses Amiga `AllocVec()` and `FreeVec()` for memory allocation
- Dynamic arrays with expansion capability
- Proper cleanup in error paths
- Tracks icon errors in expandable list

### Platform Abstraction
```
include/platform/
├── amiga_headers.h      # Amiga-specific includes
├── platform.h           # Platform interface
└── platform_io.h        # I/O abstraction
```

This structure suggests potential portability to other platforms in the future.

---

## Build System

### Makefile
- Root-level `Makefile` for building the project
- Separate makefiles in subdirectories
- Supports both Amiga native builds and cross-compilation
- Build outputs to `build/amiga/` directory

### Build Directories
```
build/
├── amiga/           # Amiga executable
│   ├── iTidy       # Final binary
│   ├── DOS/
│   ├── platform/
│   └── Settings/
└── host/           # Host-side development builds
    ├── DOS/
    ├── platform/
    └── Settings/
```

---

## Testing Infrastructure

### Test Directories
```
Tests/
├── AmosPro/         # AMOS Pro application folder with multiple icons
├── DPaintV/         # Deluxe Paint V with various file types
└── test-icons/      # Collection of different icon formats
    ├── alpha/
    ├── ColorIcons/
    ├── MUI/
    ├── Newicons/
    ├── OS1.3/
    ├── OS3/
    └── OS4/
```

These test directories provide a comprehensive set of real-world scenarios with different icon types and folder structures.

---

## Use Cases

### 1. WHDLoad Archive Management
**Scenario**: Extracting hundreds of WHDLoad game archives  
**Solution**: Run iTidy after extraction to organize all game folders
```bash
iTidy WHDLoad:Games -subdirs
```

### 2. System Cleanup After Installation
**Scenario**: Software installation leaves messy icon placement  
**Solution**: Integrate iTidy into installation scripts
```bash
Installer ; Install the software
iTidy SYS:NewApp -subdirs -viewShowAll
```

### 3. Archive Extraction Cleanup
**Scenario**: Extracted LHA/LZX archives have random icon positions  
**Solution**: Tidy extracted folders automatically
```bash
UnLZX archive.lzx Work:Temp/
iTidy Work:Temp -subdirs -resetIcons
```

### 4. Disk Organization
**Scenario**: Organizing an entire disk or partition  
**Solution**: Recursive processing with custom view settings
```bash
iTidy DH1: -subdirs -viewByType
```

### 5. Workbench Maintenance
**Scenario**: Regular system tidying as maintenance task  
**Solution**: Create a script for regular cleanup
```bash
; Tidy common work areas
iTidy Work: -subdirs
iTidy SYS:Utilities -subdirs
iTidy Downloads: -subdirs
```

---

## Limitations and Caveats

### Known Limitations
1. **Workbench 2.x**: Limited icon library support, some features disabled
2. **Large Folders**: Performance may degrade with hundreds of icons
3. **Read-Only Media**: Cannot process CD-ROMs or write-protected disks
4. **Custom Icons**: Very large custom icons may not fit optimally
5. **Multiple Screens**: Assumes standard Workbench screen

### Important Notes
- **Backup recommended**: Always backup your system before running
- **No undo**: Changes to `.info` files are immediate
- **WHDLoad default**: Preserves WHDLoad layouts by default (use `-skipWHD` to override)
- **Path format**: Use full Amiga paths with device names (e.g., `DH0:MyDir` not just `MyDir`)

### User Responsibility
As stated in the README:
> "As always, and although it works for me, this program is provided as is, and I take no responsibility for any issues it may cause. Please back up your system before running this program."

---

## Output and Reporting

### Runtime Output
iTidy provides informative console output during execution:
- Program header with version and compile date
- Target directory being processed
- Progress indicators (via spinner)
- Device write-protection checks
- Completion statistics

### Summary Report
Upon completion, iTidy displays:
```
==================================================
Icons Tidied Summary:
   Standard icons:    150
   NewIcon icons:     45
   OS3.5 style icons: 23
==================================================

Total corrupted icons found: 2
  - Games:Puzzle/BadIcon.info
  - Work:Old/Corrupt.info

Possible issues:
  - Files may be unreadable or corrupted.

Suggested actions:
  1. Replace the icon files.
  2. Restore from a backup if available.
==================================================

iTidy completed successfully in 2 minutes and 15 seconds
```

### Debug Logging
When compiled with `DEBUG` flag:
- Detailed log file created (`src/logfile.txt`)
- Logs all operations and decisions
- Useful for troubleshooting and development

---

## Development Status

### Current Version: 1.0.0
- First public release (July 11, 2024)
- Stable and feature-complete for primary use cases
- Limited testing on Workbench 2.x

### Migration Status
The project includes `MIGRATION_STATUS.md`, suggesting ongoing development or refactoring efforts.

### Future Enhancements (Potential)
Based on code structure:
- Platform portability (abstraction layer exists)
- Additional icon format support
- Performance optimizations for large directories
- Enhanced Workbench 2.x compatibility

---

## Support and Contribution

### Getting Help
- Open issues on GitHub repository: `Kwezza/iTidy`
- Contact author via email
- Check README.md for latest information

### Reporting Issues
When reporting problems, include:
- Workbench version
- Command used
- Directory structure being processed
- Any error messages displayed
- Debug log if available

### Testing
Users are encouraged to:
- Test on different Workbench versions
- Report compatibility issues
- Suggest feature improvements
- Provide feedback on usability

---

## Project Philosophy

iTidy embodies several design principles:

1. **CLI-First**: Designed for automation and scripting
2. **User Settings Respect**: Honors Workbench preferences
3. **Safe Defaults**: Conservative behavior (e.g., preserving WHDLoad layouts)
4. **Clear Communication**: Informative output and error messages
5. **Amiga Native**: Uses system libraries and follows AmigaOS conventions
6. **Practical Focus**: Solves real-world organizational problems

---

## Conclusion

iTidy is a well-crafted, specialized tool that fills a specific niche in the Amiga ecosystem. It automates the tedious task of organizing icons and resizing windows, making file management on Amiga systems more pleasant and efficient. Its CLI-only design makes it perfect for scripting and automation, while its respect for user preferences ensures it integrates seamlessly with existing Workbench configurations.

Whether you're managing WHDLoad archives, cleaning up after software installations, or simply maintaining a tidy Amiga system, iTidy provides a powerful and flexible solution.

---

*Documentation generated: October 17, 2025*  
*Project Version: 1.0.0*  
*Repository: github.com/Kwezza/iTidy*
