# iTidy - GitHub Copilot Instructions

## Project Overview

iTidy is an **AmigaOS Workbench 3.0 GUI application** that tidies up icons and resizes folder windows. This is a **complete rewrite as a GUI-only application** (no CLI version). Written in **C89/C99** and targets **Workbench 3.0/3.1** on **AmigaOS 3.x** systems (68k architecture).

---

## ⚠️ CRITICAL: Before Writing ANY Window/GUI Code

**STOP AND READ THESE FIRST:**

1. **`src/templates/AI_AGENT_GETTING_STARTED.md`** - Entry point (10 min read)
2. **`src/templates/AI_AGENT_LAYOUT_GUIDE.md`** - Section 0 has CRITICAL patterns

**These documents contain hard-learned lessons from actual bugs.**  
Any section marked **CRITICAL** or **⚠️** is a **show-stopper issue** that:
- Has caused system crashes in production
- Is easy to overlook
- Must be implemented exactly as documented

**Do NOT skip sections marked CRITICAL.** They prevent Guru meditations, memory corruption, and non-functional UI elements.

**Examples of CRITICAL issues documented:**
- `IDCMP_GADGETDOWN` required for ListView scroll buttons (forgot this = buttons don't work)
- BorderTop calculation formula (wrong = gadgets overlap title bar)
- ListView cleanup order (wrong = system crash on window close)
- Checkbox data type for `GT_GetGadgetAttrs()` (wrong = features don't activate)

**If you encounter a bug, check if it's already documented in the CRITICAL sections before debugging.**

---

## Development Environment

### Host System (PC)
- **Development Platform**: Windows PC
- **Build System**: VBCC cross-compiler (`vbcc v0.9x`)
- **Target SDK**: Amiga SDK 3.2 (supports Workbench 3.0)
- **Shared Build Directory**: `build\amiga` is shared as a drive within WinUAE emulator

### Target System (Amiga)
- **Emulator**: WinUAE
- **OS Version**: Workbench 3.2 (targeting 3.0 compatibility)
- **Mounted Device**: Host's `build\amiga` directory mounted as a shared drive
- **Architecture**: 68000/68020 (no FPU, no MMU assumed)

### Build Process
1. Code is written on the PC host
2. VBCC cross-compiles to AmigaOS 68k binaries
3. Compiled binaries appear in `build\amiga` (shared folder)
4. Executables run directly in WinUAE from the shared drive
5. Output directory: `Bin\Amiga\`
6. Build command: `make` or `make clean && make`

### Testing on Host (Optional)
- **GCC is available** for testing algorithms, text manipulation, or logic locally on PC
- Test code goes in `src\tests\` directory
- **CRITICAL**: Test code must NOT pollute main codebase
- **NEVER** include host-specific code (GCC extensions, Windows APIs, etc.) in production files
- Tests are for validation only - final code must compile with VBCC for Amiga

## Language and Coding Standards

### Language: C89/C99 (VBCC with C99 support)
- **C99 features allowed**: `//` comments are allowed, inline variable declarations are allowed
- **Variable declarations**: Can be declared at point of use (C99 style)
- **Comments**: Both `/* */` and `//` style allowed
- **Format strings**: Be careful with `Printf()` vs `printf()` - use AmigaOS types correctly
- **Naming convention**: **snake_case** for all variables, functions, and identifiers

### Example of Correct C99 Style with snake_case:
```c
void process_icons(const char *path)
{
    // Lock the directory
    BPTR lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock == 0)
    {
        return;
    }
    
    // Allocate FIB
    struct FileInfoBlock *fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL)
    {
        UnLock(lock);
        return;
    }
    
    // Process the directory
    int result = Examine(lock, fib);
    
    // Cleanup
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
}
```

## GUI Framework: Native Workbench 3.0 Only

### What We Use:
- **GadTools**: For gadgets (buttons, listviews, string gadgets, checkboxes, cycles)
- **Intuition**: For windows and screens
- **Graphics**: For drawing and text
- **Icon.library**: For icon manipulation
- **DOS.library**: For file operations

### What We DON'T Use:
- ❌ **NO MUI** (Magic User Interface)
- ❌ **NO ReAction**
- ❌ **NO third-party GUI libraries**
- ❌ Only gadgets that shipped with **Workbench 3.0**

## Window Creation and GUI Development

### **CRITICAL REQUIREMENT**: Always Consult Template Documentation

**Before creating any new window or modifying existing windows, you MUST:**

1. **Read** `src/templates/AI_AGENT_GETTING_STARTED.md` - The authoritative entry point
2. **Follow** the GadTools coordinate system rules (BorderTop calculation)
3. **Use** the canonical templates in `src/templates/` as examples
4. **Verify** border calculations match actual window borders

### Fatal Mistakes to Avoid:

❌ **DON'T** position gadgets at `ng_TopEdge = 0` (that's inside the title bar!)  
❌ **DON'T** guess border heights (always calculate from screen metrics)  
❌ **DON'T** use `goto` statements unless absolutely necessary (document why if used)  
❌ **DON'T** ignore the template documentation - it contains hard-learned fixes

### Correct BorderTop Calculation (MANDATORY):

```c
struct Screen *screen = LockPubScreen("Workbench");

// This is the ONLY correct formula
WORD border_top = screen->WBorTop + screen->Font->ta_YSize + 1;
WORD border_left = screen->WBorLeft;
WORD border_right = screen->WBorRight;
WORD border_bottom = screen->WBorBottom;

UnlockPubScreen(NULL, screen);

// Position gadgets BELOW the title bar
WORD margin = 5;
WORD current_y = border_top + margin;  // Start below title bar
WORD current_x = border_left + margin;
```

### Naming Convention for AmigaOS SDK Collision Avoidance

**CRITICAL**: AmigaOS headers define many short common identifiers. To avoid collisions:

#### Prefix ALL Project Symbols:
- **Types**: `iTidy_` prefix → `iTidy_ProgressWindow`, `iTidy_IconArray`
- **Functions**: Use `snake_case` → `itidy_open_progress_window()`, `itidy_update_progress()`
- **Constants/Macros**: `ITIDY_` prefix (ALL CAPS) → `ITIDY_BAR_HEIGHT`, `ITIDY_DEFAULT_WIDTH`
- **Struct fields**: Use `snake_case` → `shine_pen`, `shadow_pen`, `fill_pen`

#### Parameter Naming:
- **Geometry**: Use `left, top, width, height` (NOT `x, y, w, h`)
- **Naming style**: **snake_case** for everything → `shine_pen` not `shinePen`

#### Avoid These SDK Identifiers:
- Pen macros: `SHINEPEN`, `SHADOWPEN`, `BACKGROUNDPEN`, `FILLPEN`, `TEXTPEN`
- Common tags and enums from GadTools/Intuition

#### Example:
```c
/* BAD - Risk of collision and wrong naming style */
typedef struct {
    ULONG shinePen;
    ULONG shadowPen;
} ProgressPens;

/* GOOD - Properly namespaced with snake_case */
typedef struct {
    ULONG shine_pen;
    ULONG shadow_pen;
    ULONG fill_pen;
} iTidy_ProgressPens;

void itidy_progress_draw_bevel_box(struct RastPort *rp,
                                    WORD left, WORD top, WORD width, WORD height,
                                    ULONG shine_pen, ULONG shadow_pen,
                                    BOOL recessed);
```

## Memory Management System

### Custom Memory Tracking
iTidy uses a **custom memory tracking system** with wrapper functions to detect leaks.

#### Enable Memory Tracking:
In `include\platform\platform.h`, line 27:
```c
#define DEBUG_MEMORY_TRACKING
```

#### Memory Allocation Functions:
```c
/* ALWAYS use these for iTidy modules (NOT malloc/free!) */
ptr = whd_malloc(size);      /* Tracked allocation */
whd_free(ptr);               /* Tracked deallocation */

/* Initialize tracking (call once at startup) */
whd_memory_init();

/* Generate report (call before exit) */
whd_memory_report();
```

**CRITICAL**: All iTidy production code MUST use `whd_malloc()`/`whd_free()` for general-purpose allocations. Only use standard `malloc()`/`free()` in standalone test programs in `src\tests\`.

#### AmigaOS-Specific Memory:
```c
/* For AmigaOS structures and exec-based allocations */
AllocVec(size, MEMF_CLEAR);
FreeVec(ptr);

AllocDosObject(DOS_FIB, NULL);
FreeDosObject(DOS_FIB, fib);

AllocMem(size, MEMF_PUBLIC | MEMF_CLEAR);
FreeMem(ptr, size);
```

#### Memory Tracking Output:
- Logs to: `Bin\Amiga\logs\memory_YYYY-MM-DD_HH-MM-SS.log`
- Leak details include file and line number
- All errors copied to: `errors_YYYY-MM-DD_HH-MM-SS.log`

### Memory Allocation Rules:
1. **Always check for NULL** after allocation
2. **Always free** what you allocate
3. **Match allocation/deallocation**: 
   - `AllocVec()` → `FreeVec()`
   - `AllocDosObject()` → `FreeDosObject()`
   - `whd_malloc()` → `whd_free()`
4. **Free in reverse order** of allocation when possible
5. **Unlock before returning** on error paths

## Logging System

### Multi-Category Logging:
```c
#include "writeLog.h"

/* Available categories */
LOG_GENERAL    /* General program flow */
LOG_MEMORY     /* Memory operations */
LOG_GUI        /* GUI events */
LOG_ICONS      /* Icon processing */
LOG_BACKUP     /* Backup operations */

/* Log levels */
log_debug(LOG_GENERAL, "Processing: %s\n", path);
log_info(LOG_GUI, "Window opened: %dx%d\n", w, h);
log_warning(LOG_ICONS, "Icon frame not found: %s\n", icon);
log_error(LOG_BACKUP, "Backup failed: %s\n", reason);
```

### Log File Locations:
- `Bin\Amiga\logs\general_YYYY-MM-DD_HH-MM-SS.log`
- `Bin\Amiga\logs\memory_YYYY-MM-DD_HH-MM-SS.log`
- `Bin\Amiga\logs\gui_YYYY-MM-DD_HH-MM-SS.log`
- `Bin\Amiga\logs\icons_YYYY-MM-DD_HH-MM-SS.log`
- `Bin\Amiga\logs\backup_YYYY-MM-DD_HH-MM-SS.log`
- `Bin\Amiga\logs\errors_YYYY-MM-DD_HH-MM-SS.log` (consolidated errors)

## AmigaOS-Specific Types

### Use These Types:
```c
/* Exec types */
BOOL           /* TRUE/FALSE (Amiga boolean) */
STRPTR         /* char * for strings */
BPTR           /* DOS file/lock pointer */
LONG           /* 32-bit signed */
ULONG          /* 32-bit unsigned */
WORD           /* 16-bit signed */
UWORD          /* 16-bit unsigned */
BYTE           /* 8-bit signed */
UBYTE          /* 8-bit unsigned */

/* Structures */
struct FileInfoBlock *fib;
struct DiskObject *dobj;
struct Window *window;
struct Screen *screen;
struct RastPort *rastPort;
struct TextFont *font;
```

### Boolean Values:
```c
// Use AmigaOS booleans
BOOL result = TRUE;   // NOT true
BOOL flag = FALSE;    // NOT false
```

### Structure Alignment (CRITICAL for 68k):
```c
// CORRECT: Group fields by size to minimize padding
typedef struct {
    // 4-byte fields first (int, pointers, ULONG)
    int icon_x, icon_y, icon_width, icon_height;
    char *icon_text;
    char *icon_full_path;
    ULONG file_size;
    
    // struct fields (DateStamp = 12 bytes)
    struct DateStamp file_date;
    
    // 2-byte fields last (BOOL on Amiga = 2 bytes)
    BOOL is_folder;
    BOOL is_write_protected;
} FullIconDetails;

// WRONG: Mixing sizes causes padding/corruption
typedef struct {
    char *ptr;      // 4 bytes
    BOOL flag;      // 2 bytes -> compiler adds 2 bytes padding!
    int value;      // 4 bytes
} BadAlignment;  // This will cause memory corruption on 68k!
```

## File Operations (AmigaOS DOS)

### File Locking:
```c
BPTR lock;
lock = Lock((STRPTR)path, ACCESS_READ);
if (lock == 0)
{
    /* Handle error */
    return;
}

/* Always unlock before returning */
UnLock(lock);
```

### File Information:
```c
struct FileInfoBlock *fib;

fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
if (fib == NULL)
{
    UnLock(lock);
    return;
}

if (Examine(lock, fib) == DOSFALSE)
{
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    return;
}

/* Use fib->fib_FileName, fib->fib_DirEntryType, etc. */

FreeDosObject(DOS_FIB, fib);
```

### Directory Scanning:
```c
while (ExNext(lock, fib))
{
    // Process fib->fib_FileName
    if (fib->fib_DirEntryType > 0)
    {
        // It's a directory
    }
    else
    {
        // It's a file
    }
}
```

### Pattern Matching with AnchorPath (CRITICAL):
```c
// CORRECT: Manual allocation for AmigaOS 3.x compatibility
// AllocDosObject(DOS_ANCHORPATH) with tags is NOT supported in WB 3.0/3.1!
AnchorPath *ap = (AnchorPath *)AllocVec(
    sizeof(AnchorPath) + 1024 - 1,  // -1 because AnchorPath includes 1 byte
    MEMF_CLEAR
);

if (ap) {
    ap->ap_Strlen = 1024;  // Manually set buffer size
    ap->ap_BreakBits = 0;
    ap->ap_Flags = 0;
    
    // Use with MatchFirst/MatchNext
    if (MatchFirst(pattern, ap) == 0) {
        do {
            // Process ap->ap_Info (FileInfoBlock)
        } while (MatchNext(ap) == 0);
        MatchEnd(ap);
    }
    
    // Cleanup with FreeVec (NOT FreeDosObject!)
    FreeVec(ap);
}

// WRONG: Using AllocDosObject with tags (AmigaOS 4 only)
AnchorPath *ap = AllocDosObject(DOS_ANCHORPATH, tags);  // Fails on WB 3.x!
```

## Icon Operations

### Loading Icons:
```c
#include <workbench/icon.h>
#include <proto/icon.h>

struct Library *IconBase;
struct DiskObject *dobj;

IconBase = OpenLibrary("icon.library", 0);
if (!IconBase)
{
    return;
}

dobj = GetDiskObject(iconPath);
if (dobj != NULL)
{
    /* Process icon */
    /* dobj->do_CurrentX, dobj->do_CurrentY for position */
    
    /* Update position */
    dobj->do_CurrentX = newX;
    dobj->do_CurrentY = newY;
    
    /* Save changes */
    PutDiskObject(iconPath, dobj);
    
    FreeDiskObject(dobj);
}

CloseLibrary(IconBase);
```

## Window and Gadget Cleanup Order

### CRITICAL: Proper Cleanup Sequence
```c
// CORRECT cleanup order (reverse of creation)
void cleanup_window(WindowData *wd)
{
    // 1. Detach lists from ListView gadgets FIRST
    if (wd->session_list) {
        GT_SetGadgetAttrs(wd->session_gadget, wd->window, NULL,
                         GTLV_Labels, ~0,  // Detach list
                         TAG_DONE);
    }
    
    // 2. Close window BEFORE freeing gadgets
    if (wd->window) {
        CloseWindow(wd->window);
        wd->window = NULL;
    }
    
    // 3. Free gadgets after window is closed
    if (wd->gadget_list) {
        FreeGadgets(wd->gadget_list);
        wd->gadget_list = NULL;
    }
    
    // 4. Free visual info
    if (wd->vi) {
        FreeVisualInfo(wd->vi);
        wd->vi = NULL;
    }
    
    // 5. Free lists and data last
    if (wd->session_list) {
        free_session_list(wd->session_list);
        wd->session_list = NULL;
    }
}

// WRONG: Closing window after freeing gadgets causes crashes!
void bad_cleanup(WindowData *wd)
{
    FreeGadgets(wd->gadget_list);  // WRONG: Frees gadgets first
    CloseWindow(wd->window);        // CRASH: Window still references freed gadgets!
}
```

## Project Structure

```
iTidy/
├── .github/
│   └── copilot-instructions.md  (this file)
├── Bin/Amiga/               # Compiled executables
│   └── logs/                # Runtime logs
├── build/amiga/             # Shared with WinUAE (build artifacts)
├── docs/                    # Documentation
├── include/                 # Header files
│   └── platform/
│       └── platform.h       # Memory tracking system
├── src/                     # Source code
│   ├── main_gui.c          # Main GUI entry point
│   ├── icon_management.c/h # Icon processing
│   ├── window_management.c/h # Window operations
│   ├── file_directory_handling.c/h # Directory traversal
│   ├── writeLog.c/h        # Logging system
│   ├── utilities.c/h       # Helper functions
│   ├── backup_*.c/h        # Backup system modules
│   ├── layout_*.c/h        # Layout system modules
│   ├── DOS/                # DOS-related utilities
│   ├── GUI/                # GUI window modules
│   ├── Settings/           # Preferences handling
   ├── platform/           # Platform abstraction
   ├── templates/          # AI-friendly window templates
   │   └── AI_AGENT_GETTING_STARTED.md  # **MUST READ for window creation**
   └── tests/              # Test code (GCC host testing only, NOT production)
└── Makefile                # VBCC build configuration
```

## Global Structures and Settings

### System Preferences (Loaded at Startup)

iTidy loads two critical system preference structures at startup that are used throughout the application:

#### 1. **IControlPrefs** - Interface Control Settings
- **Header**: `src/Settings/IControlPrefs.h`
- **Global Variable**: `prefsIControl` (type: `struct IControlPrefsDetails`)
- **Loaded By**: `fetchIControlSettings(&prefsIControl)` in `main_gui.c`
- **Purpose**: Contains system-wide UI settings from Workbench preferences
- **Key Fields**:
  - Font information: `systemFontName`, `systemFontSize`, `systemFontCharWidth`
  - Icon text font: `iconTextFontName`, `iconTextFontSize`, `iconTextFontCharWidth`
  - Screen metrics: `currentTitleBarHeight`, `currentWindowBarHeight`, `currentBarWidth`
  - UI flags: `coerceColors`, `menuSnap`, `correctRatio`, `offScrnWin`
- **Usage**: Access anywhere via `extern struct IControlPrefsDetails prefsIControl;`

#### 2. **WorkbenchPrefs** - Workbench Settings
- **Header**: `src/Settings/WorkbenchPrefs.h`
- **Global Variable**: `prefsWorkbench` (type: `struct WorkbenchSettings`)
- **Loaded By**: `fetchWorkbenchSettings(&prefsWorkbench)` in `main_gui.c`
- **Purpose**: Contains Workbench-specific settings and icon support flags
- **Key Fields**:
  - `borderless` - Borderless icon support
  - `newIconsSupport` - NewIcons format support
  - `colorIconSupport` - Color icon support (OS 3.5+)
  - `maxNameLength` - Maximum icon text length
  - `embossRectangleSize` - Icon emboss size
- **Usage**: Access anywhere via `extern struct WorkbenchSettings prefsWorkbench;`

**Important**: These structures are populated once at startup and remain read-only during execution. They provide system-level defaults that iTidy respects when processing icons.

### Application Preferences (User Settings)

#### **LayoutPreferences** - Main Configuration Structure
- **Header**: `src/layout_preferences.h`
- **Global Accessor**: `GetGlobalPreferences()` (returns const pointer)
- **Updater**: `UpdateGlobalPreferences(const LayoutPreferences *newPrefs)`
- **Purpose**: Master configuration for all iTidy operations (layout, sorting, backup, etc.)
- **Key Field Categories**:
  - **Folder Settings**: `folder_path`, `recursive_subdirs`, `skipHiddenFolders`
  - **Layout Settings**: `layoutMode`, `sortOrder`, `sortPriority`, `sortBy`, `reverseSort`
  - **Visual Settings**: `centerIconsInColumn`, `useColumnWidthOptimization`, `textAlignment`
  - **Window Settings**: `resizeWindows`, `aspectRatio`, `maxWindowWidthPct`, `overflowMode`
  - **Spacing**: `iconSpacingX`, `iconSpacingY`
  - **Features**: `enable_backup`, `enable_icon_upgrade`, `validate_default_tools`
  - **Beta Features**: `beta_openFoldersAfterProcessing`, `beta_FindWindowOnWorkbenchAndUpdate`
  - **Logging**: `logLevel`, `memoryLoggingEnabled`, `enable_performance_logging`
  - **Backup Settings**: `backupPrefs` (embedded `BackupPreferences` struct)

**Important**: This is the single source of truth for all user-configurable settings. Always use the global accessors rather than creating local copies.

## Main Code Paths

### Primary Execution Flow: User Clicks "Apply" Button

When the user clicks the **Apply** button in the main iTidy window, the following code path executes:

#### 1. **Event Handler** (`src/GUI/main_window.c`)
```
handle_itidy_window_events()
  └─ Case: IDCMP_GADGETUP, Gadget ID: win_data->apply_btn
      ├─ Get global preferences: prefs = GetGlobalPreferences()
      ├─ Apply selected preset: ApplyPreset(prefs, win_data->preset_selected)
      ├─ Map GUI values to prefs: MapGuiToPreferences(...)
      ├─ Copy folder path: strcpy(prefs->folder_path, win_data->folder_path_buffer)
      ├─ Set recursive mode: prefs->recursive_subdirs = win_data->recursive_subdirs
      ├─ Set backup/upgrade flags: prefs->enable_backup, prefs->enable_icon_upgrade
      ├─ Update global: UpdateGlobalPreferences(prefs)
      └─ Execute processing: ProcessDirectoryWithPreferences()
```

**File**: `src/GUI/main_window.c`, lines ~970-1030
**Key Function**: `handle_itidy_window_events()` - Main event loop for iTidy window

#### 2. **Processing Entry Point** (`src/layout_processor.c`)
```
ProcessDirectoryWithPreferences()
  ├─ Get global prefs: prefs = GetGlobalPreferences()
  ├─ Validate folder path: SanitizeFolderPath()
  ├─ Initialize backup context (if enabled): CreateBackupSession()
  ├─ Build window tracker: BuildFolderWindowList()
  ├─ Set global validation flag: g_ValidateDefaultTools = prefs->validate_default_tools
  ├─ Choose processing mode:
  │   ├─ If recursive: ProcessDirectoryRecursive(path, prefs, 0, &windowTracker)
  │   └─ Else: ProcessSingleDirectory(path, prefs, &windowTracker)
  ├─ Finalize backup (if enabled): FinalizeBackupSession()
  └─ Return success/failure
```

**File**: `src/layout_processor.c`, lines ~150-250
**Key Function**: `ProcessDirectoryWithPreferences()` - Main processing orchestrator

#### 3. **Single Directory Processing** (`src/layout_processor.c`)
```
ProcessSingleDirectory(path, prefs, windowTracker)
  ├─ Create backup (if enabled): BackupFolder(path, g_backupContext)
  ├─ Scan directory: ScanDirectoryForIcons(path) → returns IconArray
  ├─ Sort icons: SortIconArrayWithPreferences(iconArray, prefs)
  ├─ Calculate layout:
  │   ├─ If centerIconsInColumn: CalculateLayoutPositionsWithColumnCentering()
  │   └─ Else: CalculateLayoutPositions()
  ├─ Apply positions: SaveIconPositions(iconArray)
  ├─ Resize window (if enabled): ResizeDrawerWindow()
  ├─ Update Workbench window (if beta feature enabled): UpdateWorkbenchWindow()
  ├─ Free icon array: FreeIconArray(iconArray)
  └─ Filesystem delay (if enabled): Delay(FILESYSTEM_LOCK_DELAY_TICKS)
```

**File**: `src/layout_processor.c`, lines ~600-800
**Key Function**: `ProcessSingleDirectory()` - Processes one folder

#### 4. **Recursive Processing** (`src/layout_processor.c`)
```
ProcessDirectoryRecursive(path, prefs, level, windowTracker)
  ├─ Process current directory: ProcessSingleDirectory(path, prefs, windowTracker)
  ├─ Scan for subdirectories: Lock() → ExNext() loop
  ├─ For each subdirectory:
  │   ├─ Check if hidden (no .info): Skip if prefs->skipHiddenFolders
  │   ├─ Build subdir path: CombinePaths()
  │   └─ Recurse: ProcessDirectoryRecursive(subdir_path, prefs, level+1, windowTracker)
  └─ Unlock directory
```

**File**: `src/layout_processor.c`, lines ~900-1100
**Key Function**: `ProcessDirectoryRecursive()` - Handles recursive subdirectory processing

### Summary of Key Files in Apply Flow

1. **GUI Layer**: `src/GUI/main_window.c` - Captures user input, validates, calls processor
2. **Processing Layer**: `src/layout_processor.c` - Orchestrates backup, scanning, sorting, layout
3. **Icon Operations**: `src/icon_management.c` - Low-level icon loading/saving via icon.library
4. **Layout Engine**: `src/aspect_ratio_layout.c` - Calculates optimal icon positions and window size
5. **Backup System**: `src/backup_session.c`, `src/backup_catalog.c` - Creates LHA backups (if enabled)
6. **Window Tracking**: `src/window_enumerator.c` - Finds open Workbench windows (for beta features)

### Quick Reference: Where to Look

- **User clicks Apply button**: `src/GUI/main_window.c:974` (IDCMP_GADGETUP handler)
- **Main processing entry**: `src/layout_processor.c:ProcessDirectoryWithPreferences()`
- **Icon scanning**: `src/folder_scanner.c:ScanDirectoryForIcons()`
- **Icon sorting**: `src/layout_processor.c:SortIconArrayWithPreferences()`
- **Layout calculation**: `src/aspect_ratio_layout.c:CalculateOptimalLayout()`
- **Icon saving**: `src/icon_management.c:SaveIconPositions()`
- **Window resizing**: `src/window_management.c:ResizeDrawerWindow()`
- **Backup creation**: `src/backup_session.c:BackupFolder()`

## Common Patterns

### Error Handling Pattern (Avoid goto unless absolutely necessary):
```c
BOOL process_file(const char *path)
{
    BOOL success = FALSE;
    
    // Lock file
    BPTR lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock == 0)
    {
        return FALSE;
    }
    
    // Allocate FIB
    struct FileInfoBlock *fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL)
    {
        UnLock(lock);
        return FALSE;
    }
    
    // Process
    if (Examine(lock, fib) != DOSFALSE)
    {
        success = TRUE;
    }
    
    // Cleanup
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    return success;
}

// NOTE: Only use goto for cleanup if you have 3+ resources to manage
// and the cleanup logic is complex. Always document why goto is used.
```

### String Handling:
```c
// Always bounds-check and null-terminate
char buffer[256];
strncpy(buffer, source, sizeof(buffer) - 1);
buffer[sizeof(buffer) - 1] = '\0';

// Use STRPTR for AmigaOS string parameters, snake_case for function names
void process_path(STRPTR path)
{
    // ...
}
```

## Testing Workflow

### Production Code (Amiga Target)
1. **Edit code** on PC in VS Code
2. **Build**: Run `make` in PowerShell terminal
3. **Check output**: `build_output_latest.txt` for errors
4. **Run**: Execute in WinUAE from shared drive
5. **Check logs**: `Bin\Amiga\logs\` directory
6. **Debug**: Enable `#define DEBUG_MEMORY_TRACKING` for leak detection

### Local Testing (PC Host - Optional)
1. **Create test file** in `src\tests\` directory
2. **Compile with GCC**: `gcc -o test_name src\tests\test_name.c`
3. **Run test**: `.\test_name.exe`
4. **Validate logic** before porting to VBCC/Amiga
5. **NEVER** merge host-specific code into production files

## ListView and GadTools Patterns

### Fast Window Opening Pattern:
```c
// Open window with empty list FIRST (appears instantly)
window = OpenWindowTags(NULL, ...)

// Set busy pointer AFTER window is visible
SetWindowPointer(window, WA_BusyPointer, TRUE, TAG_DONE);

// Parse data and populate list (user sees progress)
data = parse_data();

// Update ListView with data
GT_SetGadgetAttrs(list_gadget, window, NULL,
                 GTLV_Labels, data,
                 TAG_DONE);

// Clear busy pointer
SetWindowPointer(window, WA_BusyPointer, FALSE, TAG_DONE);
```

### ListView Character Width Calculation:
```c
// CRITICAL: ListViews need CHARACTER width, not pixel width
struct TextFont *font = open_system_font();
WORD char_width = font_width_pixels / font->tf_XSize;  // Convert pixels to chars

ng.ng_Width = char_width;  // Use character count, NOT pixels!

// WRONG: Using pixel width causes blank ListView
ng.ng_Width = 584;  // This is pixels - ListView will show nothing!
```

## Important Notes for AI Agents

1. **C99 syntax allowed** - `//` comments, inline declarations are OK
2. **snake_case naming** - All functions, variables use snake_case
3. **Use AmigaOS types** - BOOL, STRPTR, BPTR, LONG, ULONG, etc.
4. **Prefix project symbols** - iTidy_/ITIDY_ for types/constants
5. **Use whd_malloc/whd_free** - ALWAYS use memory wrappers in iTidy modules (NOT malloc/free!)
6. **Check all allocations** - Every allocation must be checked for NULL
7. **Always cleanup** - Match every Lock with UnLock, every Alloc with Free
8. **Avoid goto** - Only use if absolutely necessary and document why
9. **Log appropriately** - Use category-specific logging functions
10. **Test on target** - Code must run on real Amiga/WinUAE, not just compile
11. **Workbench 3.0/3.1 compatibility** - Target WB 3.0 and 3.1 only
12. **Read template docs FIRST** - Before any window work, read `src/templates/AI_AGENT_GETTING_STARTED.md`
13. **Calculate BorderTop correctly** - Use the formula: `screen->WBorTop + screen->Font->ta_YSize + 1`
14. **Host testing allowed** - GCC can be used for testing in `src\tests\`, but keep host code separate from production
15. **Structure alignment matters** - Group fields by size (4-byte, then 2-byte) to avoid padding on 68k
16. **AnchorPath manual allocation** - Use `AllocVec()` with manual `ap_Strlen`, not `AllocDosObject()` with tags
17. **Window cleanup order** - Always: detach lists → close window → free gadgets → free visual info
18. **Fast window opening** - Open window first, populate lists after (deferred loading pattern)

## Additional Resources (Read in this order)

1. **`src/templates/AI_AGENT_GETTING_STARTED.md`** - **REQUIRED READING** for any window/GUI work
2. `docs/AI_AGENT_GUIDE_embedded.md` - Window template usage patterns
3. `docs/MEMORY_TRACKING_QUICKSTART.md` - Memory debugging guide
4. `docs/MEMORY_TRACKING_SYSTEM.md` - Detailed tracking system info
5. `src/templates/` - Canonical code templates (use as reference)
6. `Makefile` - Build configuration and flags

## Version Information

- **iTidy Version**: 2.0 (Complete GUI rewrite)
- **Target**: AmigaOS 3.x (Workbench 3.0 and 3.1 only)
- **Compiler**: VBCC v0.9x (with C99 support)
- **SDK**: Amiga SDK 3.2
- **Language**: C89/C99 hybrid
- **GUI**: GadTools (Workbench 3.0 native gadgets only)

---

## Critical Checklist When Suggesting Code Changes

**Before writing any window/GUI code:**
- [ ] ⚠️ **MANDATORY**: Have you read `src/templates/AI_AGENT_GETTING_STARTED.md` completely?
- [ ] ⚠️ **MANDATORY**: Have you read ALL sections marked CRITICAL in `src/templates/AI_AGENT_LAYOUT_GUIDE.md`?
- [ ] ⚠️ **CRITICAL**: Are you calculating BorderTop with the correct formula (`screen->WBorTop + screen->Font->ta_YSize + 1`)?
- [ ] ⚠️ **CRITICAL**: Are gadgets positioned with `border_top + margin`, not just `margin`?
- [ ] ⚠️ **CRITICAL**: If using ListView, have you added `IDCMP_GADGETDOWN` to IDCMP flags?
- [ ] ⚠️ **CRITICAL**: If using ListView, have you added `case IDCMP_GADGETDOWN:` event handler?
- [ ] Are you using snake_case for all identifiers?
- [ ] Are you avoiding goto unless absolutely necessary?
- [ ] Have you checked all allocations for NULL?
- [ ] Have you matched all Lock/UnLock, Alloc/Free pairs?
- [ ] Does the code target Workbench 3.0/3.1 only?
- [ ] Are you using only GadTools gadgets (no MUI/ReAction)?
- [ ] Will this compile with VBCC for 68k Amiga?

**When modifying existing windows:**
- [ ] Have you verified BorderTop calculations match actual window borders?
- [ ] Have you checked the template documentation for similar patterns?
- [ ] Are you following the same patterns as existing working code?

**When creating test code:**
- [ ] Is the test file in `src\tests\` directory?
- [ ] Does it use GCC/host-specific features only for testing?
- [ ] Are you keeping host-specific code completely separate from production code?
- [ ] Will the final algorithm port cleanly to VBCC/Amiga?

**When working with structures:**
- [ ] Are fields grouped by size (4-byte fields first, then 2-byte BOOL fields last)?
- [ ] Have you avoided mixing pointer/int fields with BOOL fields?
- [ ] Are you aware that BOOL is 2 bytes on Amiga (not 1 or 4)?

**When using pattern matching:**
- [ ] Are you using manual `AllocVec()` allocation for AnchorPath?
- [ ] Have you set `ap_Strlen` manually to the buffer size?
- [ ] Are you using `FreeVec()` (not `FreeDosObject()`) for cleanup?
- [ ] Is the buffer at least 512 bytes (preferably 1024) for deep paths?
