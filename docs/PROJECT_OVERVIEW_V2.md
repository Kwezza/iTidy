# iTidy Project Overview

## Project Summary

**iTidy** is a native AmigaOS Workbench 3.2+ GUI application that automatically organizes icon layouts and resizes folder windows. It is designed to solve the tedious problem of manually arranging hundreds or thousands of icons, particularly useful for WHDLoad collections, game libraries, archive extractions, and large project directories.

**Version**: 2.0 (ReAction GUI with DefIcons integration)  
**Target Platform**: AmigaOS 3.2+ (Workbench 3.2 or newer)  
**Architecture**: 68000+ (all Amiga models supported)  
**Language**: C89/C99 (VBCC-compatible)  
**License**: See LICENSE file
**Available from***: https://aminet.net/package/util/wb/iTidy

---

## Key Capabilities

### Core Features
- 🎯 **Automatic Icon Grid Layout** - Intelligent icon positioning with configurable spacing and sorting
- 📐 **Smart Window Resizing** - Aspect ratio control with overflow handling for large directories
- 🔄 **Recursive Processing** - Process entire directory trees in one operation
- 💾 **LHA Backup System** - Create restore points before making changes with rollback capability
- 🛠️ **Default Tool Validation** - Find and fix missing or invalid default tool paths
- 📊 **Multiple Sort Options** - Folders First, Files First, Mixed, Grouped By Type, with reverse sorting
- **Multiple Icon Format Support** - Classic icons, NewIcons, OS3.5+ Color Icons, GlowIcons
- 🎨 **DefIcons Icon Creation** - Automatically create icons for iconless files using DefIcons templates
- 🖼️ **Image Thumbnail Icons** - Generate scaled preview icons for images (ILBM, PNG, GIF, JPEG, BMP, ACBM)
- 📄 **Text Preview Icons** - Render a micro-preview of file content onto text file icons

### Advanced Configuration Options
- **Configurable icon spacing** (X/Y axes)
- **Multiple aspect ratios** - Tall (0.75), Square (1.0), Compact (1.3), Classic (1.0), Wide (2.0)
- **Overflow strategies** - Horizontal, vertical, or balanced expansion
- **Column width optimization** with vertical alignment control (top, middle, bottom)
- **Min/max icons per row** with auto-calculation
- **Max window width limits** (percentage of screen)
- **Window positioning strategies** - Center, keep, near parent, no change
- **Hidden folder filtering** - Skip folders without `.info` files
- **NewIcons border stripping** (requires icon.library v44+)
- **DefIcons integration** - Automatic icon creation with type filtering and template-based assignment
- **Grouped By Type mode** - Visual icon groups (Drawers, Tools, Projects, Other) with configurable gap between groups
- **Thumbnail rendering** - Configurable preview size (48x48, 64x64, 100x100), border style, colour depth (4-256+), dithering method
- **Text preview rendering** - Render file content micro-preview onto icon; adaptive and fixed colour modes with ToolType templates
- **Skip WHDLoad slave folders** - Exclude WHDLoad game folder contents from icon creation
- **Replace existing created icons** - Refresh `ITIDY_CREATED` thumbnail and text preview icons
- **Exclude paths list** - Skip specific paths during icon creation, with `DEVICE:` placeholder for portable path matching

### Main Features Explained

#### Recursive Processing
Process entire directory trees in one operation - perfect for tidying large WHDLoad collections, archive extractions, or project folders with many subdirectories.

#### Backup & Restore System
- Creates LHA archives of `.info` files before making changes
- Restore previous layouts with one click
- View and manage backup sessions
- Separate backup system for default tool changes

#### Default Tool Analysis
Scan directories for icons with missing or invalid default tools and fix them:
- Identify broken default tool paths
- Batch replace tools across multiple icons
- PATH-based tool resolution
- Automatic backup before changes

#### Smart Window Sizing
- Multiple aspect ratio presets
- Configurable overflow handling
- Screen width limits
- Automatic column calculation
- Window positioning options (center, keep, near parent, no change)

#### DefIcons Icon Creation (v2.0)
Automatically create `.info` files for iconless entries using DefIcons templates:
- **ARexx Integration** - Communicates with DefIcons.library for file type identification
- **Template Resolution** - Walks DefIcons type hierarchy to find appropriate templates
- **Smart Filtering** - Category-based filtering, configurable exclude paths list with `DEVICE:` placeholder
- **Image Thumbnail Generation** - Renders scaled image previews onto icon images (ILBM, PNG, GIF, JPEG, BMP, ACBM)
- **Text Preview Rendering** - Renders micro-previews of file content onto icon images; configurable via ToolType templates
- **Statistics Tracking** - Reports icons created by category
- **Requirements**: DefIcons.library must be running; templates installed in `ENVARC:Sys/def_*.info`

**Folder Icon Modes:**
- **Never Create** (default): Do not create folder icons
- **Smart Mode**: Create folder icon only when the folder will have visible contents after the run
- **Always Mode**: Create a folder icon for every folder that is missing one
- **Category Filtering**: Disable specific file type categories (e.g., skip music, images)
- **System Path Exclusion**: Exclude paths defined in Exclude Paths list, with `DEVICE:` placeholder support for portable path matching
- **WHDLoad Folder Skipping**: Skip icon creation inside WHDLoad slave folders (the folder icon itself is still created)

---

## Technical Architecture

### Development Environment

#### Host System (PC)
- **Platform**: Windows PC with VS Code
- **Cross-Compiler**: VBCC v0.9x (C89/C99 support)
- **Target SDK**: Amiga SDK 3.2 (includes ReAction classes)
- **Build System**: Make-based with VBCC toolchain
- **Shared Build Directory**: `build\amiga` mounted in WinUAE

#### Target System (Amiga)
- **Emulator**: WinUAE
- **OS**: Workbench 3.2 or newer (ReAction classes required)
- **Minimum RAM**: 2MB recommended (ReAction + icon processing)
- **Mounted Device**: Host's `build\amiga` as shared drive
- **Stack Size**: 80KB (configured in `main_gui.c`)

### GUI Framework

**Native Workbench 3.2 ReAction** - No third-party dependencies:
- **ReAction Classes**: Modern BOOPSI-based GUI system (built into Workbench 3.2)
  - `window.class`: Window management
  - `layout.gadget`: Automatic gadget layout
  - `button.gadget`: Push buttons, toggle buttons
  - `listbrowser.gadget`: Advanced list displays
  - `string.gadget`: Text input fields
  - `checkbox.gadget`: Boolean toggles
  - `chooser.gadget`: Dropdown selections
  - `integer.gadget`: Numeric input
- **Intuition**: Window and screen management
- **Graphics Library**: Drawing and text rendering
- **Icon.library**: Icon manipulation (v44+ recommended for color icon support)
- **DOS.library**: File operations and directory scanning

**Explicitly NOT Used**: MUI or any third-party GUI libraries (ReAction is built into OS 3.2)

### DefIcons Integration Architecture

**Icon Creation System** (v2.0):
- **deficons_identify.c/.h**: ARexx communication with DefIcons.library for file type identification
  - Finds and connects to `DEFICONS` ARexx port
  - Sends `Identify` commands and receives type tokens
  - Maintains extension-to-token cache for performance
  - Graceful degradation if DefIcons not available
  
- **deficons_templates.c/.h**: Template resolution and icon copying
  - Scans `ENVARC:Sys/` and `ENV:Sys/` for `def_*.info` templates
  - Walks DefIcons type hierarchy to find appropriate templates
  - Maintains template cache with resolved paths
  - Implements fallback logic (executable → `def_tool`, others → `def_project`)
  - Byte-accurate icon file copying
  
- **deficons_filters.c/.h**: User preference filtering
  - Category-based type filtering
  - System path exclusion (SYS:, C:, DEVS:, LIBS:, etc.)
  - Folder icon creation modes (Smart/Always/Never)
  - Smart mode detection based on visible contents
  
- **deficons_parser.c/.h**: DefIcons type hierarchy parsing (shared with GUI)
  - Parse DefIcons type definition files
  - Build parent-child type relationships
  - Used for both GUI display and runtime resolution

**Processing Flow:**
1. Initialize ARexx connection to DefIcons port
2. Scan templates at startup (ENVARC: prioritized over ENV:)
3. For each iconless file:
   - Identify type via ARexx (`deficons_identify_file()`)
   - Apply user filters (`deficons_should_create_icon()`)
   - Resolve template via hierarchy walk (`deficons_resolve_template()`)
   - Copy template to create `.info` file (`deficons_copy_icon_file()`)
   - Track statistics by category
4. Display creation statistics and log created icons

### Memory Management

**Custom Memory Tracking System**:
- Wrapper functions: `whd_malloc()` / `whd_free()`
- Leak detection with file/line tracking
- Configurable via `#define DEBUG_MEMORY_TRACKING` in `include/platform/platform.h`
- Memory logs: `Bin/Amiga/logs/memory_YYYY-MM-DD_HH-MM-SS.log`

**Fast RAM Optimization**:
- Uses `AllocVec(size, MEMF_ANY | MEMF_CLEAR)` to prefer Fast RAM
- **15x performance improvement** on systems with Fast RAM expansion
- Benchmark: Fast RAM ~0.017s vs Chip RAM ~0.250s per operation

**AmigaOS-Specific Allocators**:
- `AllocVec()` / `FreeVec()` for general Amiga structures
- `AllocDosObject()` / `FreeDosObject()` for DOS objects
- `AllocMem()` / `FreeMem()` for exec-based allocations

### Logging System

**Multi-Category Logging** (`src/writeLog.c`):
- `LOG_GENERAL` - Program flow
- `LOG_MEMORY` - Memory operations
- `LOG_GUI` - GUI events
- `LOG_ICONS` - Icon processing
- `LOG_BACKUP` - Backup operations

**Log Levels**: `log_debug()`, `log_info()`, `log_warning()`, `log_error()`

**Output Locations**: `Bin/Amiga/logs/` with category-specific and consolidated error logs

---

## Code Structure

### Primary Modules

| Module | Purpose | Key Files |
|--------|---------|-----------|
| **GUI** | Window management and user interface | `src/GUI/main_window.c`, `advanced_window.c`, `beta_options_window.c` |
| **Layout Engine** | Icon positioning and sorting logic | `src/layout_processor.c`, `aspect_ratio_layout.c` |
| **Icon Management** | Icon loading/saving via icon.library | `src/icon_management.c`, `icon_misc.c`, `icon_types.c` |
| **Directory Scanning** | Recursive folder traversal | `src/folder_scanner.c`, `file_directory_handling.c` |
| **Window Operations** | Drawer window resizing | `src/window_management.c`, `GUI/window_enumerator.c` |
| **Backup System** | LHA-based backup/restore | `src/backup_session.c`, `backup_catalog.c`, `backup_lha.c` |
| **Settings Management** | Preferences and configuration | `src/layout_preferences.c`, `Settings/IControlPrefs.h` |
| **DefIcons Integration** | Automatic icon creation | `src/deficons_identify.c`, `deficons_templates.c`, `deficons_filters.c` |
| **Utilities** | Helper functions and string handling | `src/utilities.c`, `string_functions.c`, `path_utilities.c` |

### Directory Structure

```
iTidy/
├── Bin/Amiga/              # Compiled executables and logs
├── build/amiga/            # Shared build artifacts (WinUAE mount)
├── docs/                   # Documentation
│   ├── manual/            # User manual
│   └── DEVELOPMENT_LOG.md # Complete development history
├── include/               # Header files
│   └── platform/          # Platform abstraction layer
├── src/                   # Source code
│   ├── GUI/              # Window modules
│   ├── DOS/              # DOS utilities
│   ├── helpers/          # Helper modules
│   ├── platform/         # Platform-specific code
│   ├── Settings/         # Preferences handling
│   ├── templates/        # AI-friendly code templates
│   └── tests/            # Test code (GCC host testing only)
└── Makefile              # VBCC build configuration
```

---

## Execution Flow

### Main Processing Path: User Clicks "Start"

1. **Event Handler** (`src/GUI/main_window.c`)
   - Captures `WMHI_GADGETUP` event from Start button (via `RA_HandleInput()`)
   - Maps GUI values to `LayoutPreferences` structure
   - Validates folder path and options
   - Calls `ProcessDirectoryWithPreferences()`

2. **Processing Entry Point** (`src/layout_processor.c`)
   - Retrieves global preferences via `GetGlobalPreferences()`
   - Initializes backup context (if enabled)
   - Builds window tracker for Workbench window updates
   - **DefIcons Pre-Pass** (if enabled): Creates missing `.info` files before tidying
   - Routes to recursive or single-directory processing

3. **Single Directory Processing**
   - Creates backup with `BackupFolder()` (if enabled)
   - **Icon Creation Pre-Pass**: `CreateMissingIconsInDirectory()` (if DefIcons enabled)
   - Scans directory with `ScanDirectoryForIcons()`
   - Sorts icons with `SortIconArrayWithPreferences()`
   - Calculates layout with `CalculateOptimalLayout()`
   - Applies positions with `SaveIconPositions()`
   - Resizes window with `ResizeDrawerWindow()` (if enabled)
   - Updates Workbench window (beta feature)

4. **Recursive Processing**
   - Processes current directory via `ProcessSingleDirectory()`
   - Scans for subdirectories using `Lock()` and `ExNext()`
   - Filters hidden folders (no `.info` file)
   - Recursively processes each subdirectory

---

## Coding Standards

### Language: C89/C99 (VBCC with C99 support)
- **C99 features allowed**: `//` comments, inline variable declarations
- **Naming convention**: `snake_case` for all identifiers (functions, variables, types)
- **AmigaOS types**: `BOOL`, `STRPTR`, `BPTR`, `LONG`, `ULONG`, `WORD`, `UWORD`
- **Boolean values**: `TRUE` / `FALSE` (not `true` / `false`)

### Namespace Rules (Collision Avoidance)
- **Types**: `iTidy_` prefix → `iTidy_ProgressWindow`, `iTidy_IconArray`
- **Functions**: `itidy_` prefix with snake_case → `itidy_open_progress_window()`
- **Constants/Macros**: `ITIDY_` prefix (ALL CAPS) → `ITIDY_BAR_HEIGHT`
- **Struct fields**: snake_case → `shine_pen`, `shadow_pen`
- **Parameters**: Use `left, top, width, height` (not `x, y, w, h`)

### Memory Allocation Rules
1. **Always check for NULL** after allocation
2. **Always free** what you allocate
3. **Match allocation/deallocation pairs**:
   - `AllocVec()` → `FreeVec()`
   - `AllocDosObject()` → `FreeDosObject()`
   - `whd_malloc()` → `whd_free()`
4. **Free in reverse order** of allocation when possible
5. **Unlock before returning** on error paths

### Structure Alignment (CRITICAL for 68k)
Group fields by size to minimize padding:
```c
typedef struct {
    // 4-byte fields first (int, pointers, ULONG)
    int icon_x, icon_y, icon_width, icon_height;
    char *icon_text;
    ULONG file_size;
    
    // struct fields (DateStamp = 12 bytes)
    struct DateStamp file_date;
    
    // 2-byte fields last (BOOL on Amiga = 2 bytes)
    BOOL is_folder;
    BOOL is_write_protected;
} FullIconDetails;
```

### Window Creation Guidelines
**CRITICAL**: Before creating any new window or modifying existing windows:
1. **Read** `src/templates/AI_AGENT_GETTING_STARTED.md` (mandatory)
2. **Follow** the ReAction layout.gadget patterns
3. **Use** canonical ReAction templates in `src/templates/`
4. **Let layout.gadget handle positioning**: No manual BorderTop calculations needed
5. **Use BOOPSI objects**: `NewObject()` for gadgets, `DisposeObject()` for cleanup

---

## Build System

### Build Commands
```powershell
# Clean build (recommended after significant changes)
make clean && make

# Incremental build (faster for small changes)
make

# Build with console output enabled (debugging Workbench launch issues)
make CONSOLE=1

# Host build for algorithm testing (GCC on PC)
make TARGET=host
```

### Build Output
- **Executables**: `Bin/Amiga/`
- **Build logs**: `build_output_latest.txt`
- **Expected warnings**: VBCC warnings 51 (bitfield) and 61 (array size) from system headers

### Testing Workflow

**Production Code (Amiga Target)**:
1. Edit code on PC in VS Code
2. Build with `make`
3. Check output in `build_output_latest.txt`
4. Run in WinUAE from shared drive
5. Check logs in `Bin/Amiga/logs/`

**Local Testing (PC Host - Optional)**:
1. Create test file in `src/tests/`
2. Compile with GCC: `gcc -o test_name src/tests/test_name.c`
3. Run test: `.\test_name.exe`
4. Requirements

### System Requirements
- **OS**: AmigaOS Workbench 3.2 or newer (required for ReAction classes)
- **CPU**: 68000 or better (all Amiga models supported)
- **Memory**: At least 1MB free RAM (2MB+ recommended for large directory trees and icon creation)
- **Storage**: At least 1MB free disk space (more for backups)

### Optional Dependencies
- **LhA** - Required for backup/restore features
  - Must be installed in `SYS:C` path or accessible via PATH
  - Download from [Aminet](http://aminet.net/package/util/arc/lha)
  - Without LhA, backup features will be disabled
- **DefIcons.library** - Required for automatic icon creation feature
  - DefIcons must be running in background for file type identification
  - Templates must be installed in `ENVARC:Sys/def_*.info` (permanent) or `ENV:Sys/def_*.info` (RAM disk)
  - Download from [Aminet](http://aminet.net/package/util/wb/DefIcons)
  - Without DefIcons, icon creation feature will be disabled

## Supported Icon Formats

| Format | Workbench Version | Detection Method | Support Level |
|--------|------------------|------------------|---------------|
| **Classic/Standard** | All versions | No extended data | ✅ Full |
| **NewIcons** | 3.0+ with NewIcons | `NEWICON` tooltype | ✅ Full |
| **OS3.5/OS3.9 Color Icons** | 3.5+ | Icon.library v44+ | ✅ Full |
| **GlowIcons** | 3.5+ (both variants) | ARGB data chunks | ✅ Full |

**Note**: For proper OS3.5+ color icon support, update icon.library to v44+ or convert icons to NewIcons/classic format.
**IControlPrefs** (`src/Settings/IControlPrefs.h`):
- Global variable: `prefsIControl` (type: `struct IControlPrefsDetails`)
- Contains: Font settings, screen metrics, title bar heights, UI flags
- Loaded by: `fetchIControlSettings(&prefsIControl)` in `main_gui.c`

**WorkbenchPrefs** (`src/Settings/WorkbenchPrefs.h`):
- Global variable: `prefsWorkbench` (type: `struct WorkbenchSettings`)
- Contains: Icon format support flags, max name length, emboss size
- Loaded by: `fetchWorkbenchSettings(&prefsWorkbench)` in `main_gui.c`

### Application Preferences

**LayoutPreferences** (`src/layout_preferences.h`):
- Accessor: `GetGlobalPreferences()` (returns const pointer)
- Updater: `UpdateGlobalPreferences(const LayoutPreferences *newPrefs)`
- **Master configuration** for all iTidy operations
- Categories: Folder settings, layout settings, visual settings, window settings, spacing, features, beta features, logging, backup settings

---

## Supported Icon Formats

| Format | Workbench Version | Detection Method | Support Level |
|--------|------------------|------------------|---------------|
| **Classic/Standard** | All versions | No extended data | Full |
| **NewIcons** | 3.0+ with NewIcons | `NEWICON` tooltype | Full |
| **OS3.5 Color Icons** | 3.5+ | Icon.library v44+ | Full |
| **GlowIcons** | 3.5+ | ARGB data chunks | Full |

---

## Known Issues and Workarounds

### RAM Disk Crash on Fast Emulators
- **Issue**: Filesystem lock timing on fast systems causes crashes
- **Fix**: Filesystem delay added after operations (`FILESYSTEM_LOCK_DELAY_TICKS`)

### PATH Search from Startup Scripts
- **Issue**: Default tool validation requires reading startup-script PATH
- **Fix**: Parse `S:User-Startup` and `S:Startup-Sequence` for PATH assignments

### Fast RAM Allocation
- **Discovery**: Standard `malloc()` only uses Chip RAM
- **Fix**: `whd_malloc()` uses `AllocVec(MEMF_ANY)` for Fast RAM preference
- **Impact**: 15x performance improvement with Fast RAM expansion

### AnchorPath Allocation (CRITICAL)
- **Issue**: `AllocDosObject(DOS_ANCHORPATH)` with tags is AmigaOS 4 only
- **Fix**: Manual allocation with `AllocVec()` and `ap_Strlen` assignment
- **Cleanup**: Use `FreeVec()` (NOT `FreeDosObject()`)

---

## Development Resources

### Essential Documentation (Read in Order)
1. **`src/templates/AI_AGENT_GETTING_STARTED.md`** - **REQUIRED** for any window/GUI work
2. **`docs/DEVELOPMENT_LOG.md`** - Complete history of bugs, fixes, architectural decisions
3. **`docs/AI_AGENT_GUIDE_embedded.md`** - Window template usage patterns
4. **`docs/MEMORY_TRACKING_QUICKSTART.md`** - Memory debugging guide
5. **`.github/copilot-instructions.md`** - Complete coding standards and conventions
6. **`docs/DEFICONS_ICON_CREATION_INTEGRATION_PLAN.md`** - DefIcons feature implementation plan and status
7. **`docs/iTidy_DefIcons_Icon_Creation_Flow.md`** - DefIcons feature design and workflow

### Quick Reference: Where to Look

| Task | File Location |
|------|---------------|
| User clicks Start button | `src/GUI/main_window.c` (WMHI_GADGETUP via RA_HandleInput) |
| Main processing entry | `src/layout_processor.c:ProcessDirectoryWithPreferences()` |
| DefIcons icon creation | `src/layout_processor.c:CreateMissingIconsInDirectory()` |
| Icon scanning | `src/folder_scanner.c:ScanDirectoryForIcons()` |
| Icon sorting | `src/layout_processor.c:SortIconArrayWithPreferences()` |
| Layout calculation | `src/aspect_ratio_layout.c:CalculateOptimalLayout()` |
| Icon saving | `src/icon_management.c:SaveIconPositions()` |
| Window resizing | `src/window_management.c:ResizeDrawerWindow()` |
| Backup creation | `src/backup_session.c:BackupFolder()` |
| DefIcons type identification | `src/deficons_identify.c:deficons_identify_file()` |
| DefIcons template resolution | `src/deficons_templates.c:deficons_resolve_template()` |
| DefIcons filtering | `src/deficons_filters.c:deficons_should_create_icon()` |

---

## Performance Characteristics

### Memory Usage
- **Base overhead**: ~32 bytes per tracked allocation
- **Stack size**: 80KB (configurable)
- **Fast RAM preference**: Automatic with `whd_malloc()`

### Benchmarks (A1200 with 8MB Fast RAM)
- **Chip RAM operations**: ~0.250 seconds per operation
- **Fast RAM operations**: ~0.017 seconds per operation (15x faster)
- **Recursive processing**: ~100-500 icons/second (varies by directory depth)

---

## Future Enhancements

### Beta Features (Currently Experimental)
- `beta_openFoldersAfterProcessing` - Auto-open processed folders
- `beta_FindWindowOnWorkbenchAndUpdate` - Live Workbench window updates

### Potential Additions
- Custom icon grid patterns
- Icon label color customization
- Multi-threaded icon processing (requires OS4+)
- AppIcon support for drag-and-drop operation

---

## Contribution Guidelines

### Code Checklist

**Before Writing Any Window/GUI Code**:
- [ ] Read `src/templates/AI_AGENT_GETTING_STARTED.md` completely
- [ ] Read ALL sections marked CRITICAL in `src/templates/AI_AGENT_REACTION_GUIDE.md`
- [ ] Use ReAction `layout.gadget` for automatic positioning (no manual BorderTop)
- [ ] Use `NewObject()` to create gadgets, `DisposeObject()` for cleanup
- [ ] Let ReAction handle layout - don't manually calculate coordinates

**General Code Standards**:
- [ ] Use `snake_case` for all identifiers
- [ ] Avoid `goto` unless absolutely necessary (document why if used)
- [ ] Check all allocations for NULL
- [ ] Match all Lock/UnLock, Alloc/Free, NewObject/DisposeObject pairs
- [ ] Target Workbench 3.2+ only (ReAction required)
- [ ] Use ReAction classes (not GadTools)

## Safety & Disclaimer

⚠️ **Important**: iTidy is hobby software provided as-is. While tested on real Amiga systems, bugs may exist.

### What iTidy Modifies
- iTidy **only modifies** `.info` files and Workbench drawer/window layout information
- Your **data files are never touched**
- The backup system helps roll back changes, but is not a replacement for regular backups

### Best Practices
- **Always maintain proper system backups** before processing important directories
- **Try on test folders first**, especially for large operations
- Close and reopen drawer windows, or restart Workbench to see position changes
- Processing thousands of folders takes time on real hardware - this is normal

## Documentation

For detailed instructions and guides:
- **User Manual**: [docs/manual/README.md](../manual/README.md)
- **Development Log**: [docs/DEVELOPMENT_LOG.md](DEVELOPMENT_LOG.md)
- **Coding Standards**: [.github/copilot-instructions.md](../.github/copilot-instructions.md)
- **Memory Tracking Guide**: [docs/MEMORY_TRACKING_QUICKSTART.md](MEMORY_TRACKING_QUICKSTART.md)

User manual topics covered:
- Main window controls and presets
- Advanced settings detailed guide
- Default tool analysis and fixing
- Backup and restore procedures
- Tips and troubleshooting
- Known issues and workarounds

## Credits

**Author**: Kerry Thompson  
**GitHub**: https://github.com/Kwezza/iTidy  
**Special thanks**: Darren "dmcoles" Cole for ReBuild, an excellent GUI builder that made iTidy's updated interface possible.

Made with ❤️ for the Amiga community

**Use at your own risk.** The author accepts no responsibility for data loss, corruption, or other issues.

## Quick Start Guide

1. **Launch iTidy** - Double-click the iTidy icon on Workbench
2. **Select Folder** - Click the **Folder to tidy** gadget to open a directory requester
3. **Choose Options**:
   - Set **Grouping** (Folders First, Files First, Mixed, or Grouped By Type)
   - Enable **Include Subfolders** for recursive processing
   - Enable **Back Up Layout Before Changes** to create restore points (requires `LhA` in `C:`)
   - (Optional) Enable **Create Icons During Tidy**; configure via **Icon Creation...**
4. **Click Start** - iTidy processes the folder(s) and arranges icons
5. **Restore if Needed** - Use "Restore Backups..." to undo changes

💡 **Tip**: Try iTidy on a small test folder first to see how the options work!

## Installation

1. Extract the iTidy archive to your desired location (e.g., `SYS:Utilities/iTidy`)
2. Ensure LhA is installed if you want to use backup features (copy to `SYS:C` or add to PATH)
3. (Optional) Install DefIcons.library for automatic icon creation feature
   - Download and install DefIcons from [Aminet](http://aminet.net/package/util/wb/DefIcons)
   - Install icon templates to `ENVARC:Sys/def_*.info` (permanent) or `ENV:Sys/def_*.info` (RAM disk)
   - Ensure DefIcons is running in background
4. Double-click the iTidy icon to launch

## ToolTypes

These ToolTypes are read from the iTidy program icon when launched from Workbench.

**DEBUGLEVEL** (Values: 0-4, Default: 4/Disabled)  
  Log level. 0=Debug, 1=Info, 2=Warning, 3=Error, 4=Disabled. Overrides any log level stored in a loaded preferences file.

**LOADPREFS** (Values: File path, Default: none)  
  Automatically loads a saved preferences file at startup. Paths without a device name are resolved relative to `PROGDIR:userdata/Settings/`.

**PERFLOG** (Values: YES / NO, Default: NO)  
  Enables performance timing logs for benchmarking.

## Tips & Troubleshooting

### Common Issues and Solutions

**Icons Appear Misaligned**
- Update icon.library to v44+ for proper OS3.5+ color icon support
- Or convert icons to NewIcons/classic format

**Window Positions Not Updating**
- Close and reopen drawer windows to see changes
- Restart Workbench if windows still don't reflect updates

**Slow Processing on Large Trees**
- Processing thousands of folders takes time on real hardware - this is normal
- Process in chunks for very large collections (break into smaller subdirectories)

**"Unable to open your tool" Errors**
- Use the "Fix Default Tools..." feature to scan for and repair missing default tool paths
- iTidy will search your PATH and suggest valid replacements

**Memory Issues on Low-RAM Systems**
- iTidy v2 requires at least 1MB free RAM; 2MB+ recommended for large directory trees and icon creation
- Try processing smaller directory trees
- Disable recursive processing for very large collections

**DefIcons Not Creating Icons**
- Ensure DefIcons.library is running in background
- Check that templates are installed in `ENVARC:Sys/def_*.info` or `ENV:Sys/def_*.info`
- Verify "Create new icons" checkbox is enabled in iTidy
- Check category filters - some types may be disabled in preferences
- Review creation log in `PROGDIR:logs/IconsCreated/created_*.txt`

**DefIcons Creating Wrong Icon Types**
- DefIcons uses file content analysis, not just extensions
- Check DefIcons configuration and type definitions
- Some files may need manual icon assignment

For more troubleshooting help, see the user manual in `docs/manual/README.md`.

## Version History

**v2.0** - Current Release (ReAction GUI + DefIcons + Thumbnail Icons)
- **ReAction GUI** - Complete rewrite using Workbench 3.2 built-in ReAction classes
  - Modern BOOPSI-based interface with automatic layout (no external dependencies)
  - Multi-tab Advanced Settings (Layout, Density, Limits, Columns & Groups, Filters & Misc)
  - Preset management: save/load/reset settings (with `LOADPREFS` ToolType for auto-load)
  - Logging with configurable level (Debug/Info/Warning/Error/Disabled)
- **Grouped By Type** sorting mode - icon groups (Drawers, Tools, Projects, Other) with configurable gap
- **DefIcons integration** for automatic icon creation
  - ARexx communication with DefIcons.library for file type identification
  - Template-based icon assignment with hierarchy walking
  - Category filtering via DefIcons Categories window
  - Configurable Exclude Paths list with `DEVICE:` placeholder for portable path matching
  - Never/Smart/Always folder icon modes (Never is default)
  - WHDLoad slave folder detection and skipping
- **Image thumbnail icon generation** - ILBM, PNG, GIF, JPEG, BMP, ACBM
  - Configurable preview size (Small 48x48, Medium 64x64, Large 100x100)
  - Border styles: None, Workbench (Smart/Always), Bevel (Smart/Always)
  - Colour depth: 4 to 256 colours, GlowIcons palette (29), Ultra mode
  - Dithering: None, Ordered (Bayer 4x4), Error Diffusion (Floyd-Steinberg), Auto
- **Text preview icon generation** with ToolType-configurable rendering templates
  - Adaptive and fixed colour rendering modes
  - Per-type custom templates plus master fallback with EXCLUDETYPE support
  - ToolType Validate function for template debugging
- **Default Tool Analysis** - Cache save/load, export reports, System PATH view
- **Folder View** - Hierarchical tree view of backup run contents
- All v1.x features retained and enhanced
- **Requires**: Workbench 3.2+, 68000+, 1MB RAM

**v1.x** - Legacy Release (GadTools GUI - Workbench 3.0/3.1)
- GadTools-based GUI interface (compatible with WB 3.0/3.1)
- Optimized for low-end systems (68000, 1MB RAM, A500/A600/A1200)
- Advanced layout options with aspect ratio control
- LHA-based backup and restore system
- Default tool validation and repair
- Recursive directory processing
- Multiple sort options (Folders First, Files First, Mixed)
- Fast RAM allocation for 15x performance improvement
- Column width optimization and vertical alignment control
- Hidden folder filtering
- NewIcons border stripping support
- **Note**: For Workbench 3.0/3.1 or <2MB RAM systems, use v1.x

**v0.x** - Original CLI version (deprecated)
- Command-line interface only

---

## Contact and Support

- **Repository**: GitHub (owner: Kwezza, repo: iTidy)
- **Current Branch**: Dev
- **Default Branch**: main
- **License**: See LICENSE file
- **Documentation**: See `docs/manual/` for user guide

---

## License

See the LICENSE file in the project root for licensing information.

---

*This document was last updated: March 6, 2026*
