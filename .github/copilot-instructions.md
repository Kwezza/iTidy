# iTidy - GitHub Copilot Instructions (v2.0 - ReAction)

## Project Overview

iTidy is an **AmigaOS Workbench 3.2 GUI application** that automatically tidies icon layouts and resizes folder windows. Written in **C89/C99** (VBCC-compatible) targeting **Workbench 3.2 (V47+)** on **68020+** systems.

**Version History:**
- **v1.0**: GadTools-based GUI (Workbench 3.0/3.1 compatible) - **COMPLETED**
- **v2.0**: ReAction-based GUI (Workbench 3.2+ only) - **CURRENT DEVELOPMENT**

**Key Capabilities:**
- Intelligent icon grid layout with configurable sorting (name, type, date, size)
- Automated window resizing with aspect ratio control and overflow handling
- LHA-based backup system with restore capability
- Recursive directory processing with hidden folder filtering
- Default tool validation and upgrade system
- Performance-optimized for Amigas with 68020+ and Fast RAM

---

## ⚠️ CRITICAL: Before Writing ANY Window/GUI Code

**STOP AND READ THESE FIRST:**

1. **`src/templates/AI_AGENT_GETTING_STARTED.md`** - Entry point for ReAction patterns
2. **`src/templates/AI_AGENT_REACTION_GUIDE.md`** - ReAction-specific implementation guide

> **Note:** The archived GadTools documentation is available in `src/templates/GadTools_*.md` for reference but should NOT be used for new code.

**These documents contain hard-learned lessons from actual bugs.**  
Any section marked **CRITICAL** or **⚠️** is a **show-stopper issue**.

---

## GUI Framework: ReAction (Workbench 3.2+)

### What We Use (v2.0):
- **ReAction**: For all gadgets (buttons, listviews, string gadgets, checkboxes, choosers)
- **reaction.library**: Core ReAction support
- **window.class**: Window management with automatic layout
- **layout.gadget**: Automatic gadget arrangement
- **button.gadget**: Push buttons, toggle buttons
- **listbrowser.gadget**: Advanced list displays (replaces GadTools ListView)
- **string.gadget**: Text input fields
- **checkbox.gadget**: Boolean toggles
- **chooser.gadget**: Dropdown selections (replaces GadTools Cycle)
- **Intuition**: For screens and base window operations
- **Graphics**: For drawing and text
- **Icon.library**: For icon manipulation
- **DOS.library**: For file operations

### What We DON'T Use:
- ❌ **NO MUI** (Magic User Interface) - third-party, not bundled
- ❌ **NO GadTools** (deprecated in v2.0, see archived templates)
- ❌ **NO manual BorderTop calculations** (ReAction handles layout automatically)
- ❌ **NO hardcoded gadget coordinates** (use layout.gadget instead)

### Archived v1.0 Templates (GadTools):
The following files are preserved for reference but should NOT be used for v2.0 development:
- `.github/copilot-instructions-v1-GadTools.md`
- `src/templates/GadTools_AI_AGENT_*.md`
- `src/templates/GadTools_amiga_window_*.c`

---

## Development Environment

### Host System (PC)
- **Development Platform**: Windows PC
- **Build System**: VBCC cross-compiler (`vbcc v0.9x`)
- **Target SDK**: NDK 3.2 (includes ReAction classes)
- **Shared Build Directory**: `build\amiga` is shared as a drive within WinUAE emulator

### Target System (Amiga)
- **Emulator**: WinUAE
- **OS Version**: Workbench 3.2 (minimum required)
- **Mounted Device**: Host's `build\amiga` directory mounted as a shared drive
- **Architecture**: 68020+ (ReAction requires 68020 minimum)

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

### Debugging Build Errors
1. Check `build_output_latest.txt` for compiler output
2. VBCC warnings 51 (bitfield) and 61 (array size) from system headers are **expected** and cannot be suppressed
3. Memory tracking adds ~32 bytes overhead per allocation - disable for performance testing
4. Stack size is set to 80KB in `main_gui.c`: `long __stack = 80000L;`

---

## Language and Coding Standards

### Language: C89/C99 (VBCC with C99 support)
- **C99 features allowed**: `//` comments are allowed, inline variable declarations are allowed
- **Variable declarations**: Can be declared at point of use (C99 style)
- **Comments**: Both `/* */` and `//` style allowed
- **Format strings**: Be careful with `Printf()` vs `printf()` - use AmigaOS types correctly
- **Naming convention**: **snake_case** for all variables, functions, and identifiers
- **Text encoding**: **CRITICAL** - Use only **ASCII characters** (0x00-0x7F) in all strings, especially UI text. NO Unicode characters (→ ≠ ≤ ≥ • etc.) - AmigaOS uses the standard Topaz font which only supports ASCII. Use ASCII alternatives: `->` not `→`, `!=` not `≠`, `<=` not `≤`, `>=` not `≥`, `*` or `-` not `•`

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

---

## ReAction Window Creation (v2.0 Pattern)

### Key Differences from GadTools:
1. **No manual BorderTop calculation** - ReAction layout engine handles positioning
2. **No manual gadget coordinates** - Use `LAYOUT_ADDCHILD` with layout objects
3. **Object-oriented approach** - Create objects with `NewObject()`, dispose with `DisposeObject()`
4. **Automatic resizing** - Layout gadgets resize children automatically

### Basic ReAction Window Structure:

```c
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>

/* Library bases (must be opened) */
struct Library *WindowBase = NULL;
struct Library *LayoutBase = NULL;
struct Library *ButtonBase = NULL;

/* Class pointers */
Class *WindowClass = NULL;
Class *LayoutClass = NULL;
Class *ButtonClass = NULL;

/* Window and gadget objects */
Object *window_obj = NULL;
Object *layout_obj = NULL;
Object *button_obj = NULL;

BOOL open_reaction_window(void)
{
    /* Open required classes */
    WindowBase = OpenLibrary("window.class", 0);
    LayoutBase = OpenLibrary("gadgets/layout.gadget", 0);
    ButtonBase = OpenLibrary("gadgets/button.gadget", 0);
    
    if (!WindowBase || !LayoutBase || !ButtonBase)
    {
        return FALSE;
    }
    
    /* Get class pointers */
    WindowClass = WINDOW_GetClass();
    LayoutClass = LAYOUT_GetClass();
    ButtonClass = BUTTON_GetClass();
    
    /* Create button gadget */
    button_obj = NewObject(ButtonClass, NULL,
        GA_ID, GID_APPLY,
        GA_Text, "Apply",
        GA_RelVerify, TRUE,
        TAG_DONE);
    
    /* Create layout gadget (contains button) */
    layout_obj = NewObject(LayoutClass, NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_AddChild, button_obj,
        TAG_DONE);
    
    /* Create window object */
    window_obj = NewObject(WindowClass, NULL,
        WA_Title, "iTidy",
        WA_Activate, TRUE,
        WA_DepthGadget, TRUE,
        WA_DragBar, TRUE,
        WA_CloseGadget, TRUE,
        WA_SizeGadget, TRUE,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_Layout, layout_obj,
        TAG_DONE);
    
    if (window_obj == NULL)
    {
        return FALSE;
    }
    
    /* Open the window */
    struct Window *window = (struct Window *)RA_OpenWindow(window_obj);
    if (window == NULL)
    {
        DisposeObject(window_obj);
        window_obj = NULL;
        return FALSE;
    }
    
    return TRUE;
}

void close_reaction_window(void)
{
    if (window_obj)
    {
        DisposeObject(window_obj);  /* Disposes all child objects too */
        window_obj = NULL;
    }
    
    /* Close class libraries */
    if (ButtonBase) CloseLibrary(ButtonBase);
    if (LayoutBase) CloseLibrary(LayoutBase);
    if (WindowBase) CloseLibrary(WindowBase);
}
```

### ReAction Event Loop Pattern:

```c
void handle_reaction_events(Object *window_obj)
{
    struct Window *window = NULL;
    ULONG signal_mask, signals;
    ULONG result;
    UWORD code;
    BOOL done = FALSE;
    
    /* Get window pointer and signal */
    GetAttr(WINDOW_Window, window_obj, (ULONG *)&window);
    GetAttr(WINDOW_SigMask, window_obj, &signal_mask);
    
    while (!done)
    {
        signals = Wait(signal_mask | SIGBREAKF_CTRL_C);
        
        if (signals & SIGBREAKF_CTRL_C)
        {
            done = TRUE;
            continue;
        }
        
        while ((result = RA_HandleInput(window_obj, &code)) != WMHI_LASTMSG)
        {
            switch (result & WMHI_CLASSMASK)
            {
                case WMHI_CLOSEWINDOW:
                    done = TRUE;
                    break;
                    
                case WMHI_GADGETUP:
                    switch (result & WMHI_GADGETMASK)
                    {
                        case GID_APPLY:
                            /* Handle Apply button */
                            break;
                    }
                    break;
            }
        }
    }
}
```

### CRITICAL: ReAction Cleanup Order

```c
/* CORRECT cleanup order for ReAction */
void cleanup_reaction_window(Object *window_obj)
{
    /* 1. Close window first (if open) */
    RA_CloseWindow(window_obj);
    
    /* 2. Dispose top-level object (disposes children automatically) */
    DisposeObject(window_obj);
    
    /* 3. Close class libraries last */
    CloseLibrary(ButtonBase);
    CloseLibrary(LayoutBase);
    CloseLibrary(WindowBase);
}

/* WRONG: Do NOT manually dispose child objects! */
void bad_cleanup(Object *window_obj, Object *layout_obj, Object *button_obj)
{
    DisposeObject(button_obj);  /* WRONG: double-free! */
    DisposeObject(layout_obj);  /* WRONG: double-free! */
    DisposeObject(window_obj);  /* This already disposed children */
}
```

---

## ReAction Gadget Reference (TODO: Expand with patterns)

### Common Gadget Classes:

| Class | Purpose | Key Tags |
|-------|---------|----------|
| `button.gadget` | Push buttons | `GA_Text`, `GA_RelVerify` |
| `checkbox.gadget` | Boolean toggles | `GA_Selected`, `GA_Text` |
| `string.gadget` | Text input | `STRINGA_TextVal`, `STRINGA_MaxChars` |
| `integer.gadget` | Numeric input | `INTEGER_Number`, `INTEGER_Minimum`, `INTEGER_Maximum` |
| `chooser.gadget` | Dropdown selection | `CHOOSER_Labels`, `CHOOSER_Selected` |
| `listbrowser.gadget` | List display | `LISTBROWSER_Labels`, `LISTBROWSER_Selected` |
| `layout.gadget` | Automatic arrangement | `LAYOUT_Orientation`, `LAYOUT_AddChild` |
| `scroller.gadget` | Scrollbars | `SCROLLER_Total`, `SCROLLER_Visible`, `SCROLLER_Top` |
| `requester.class` | Modal dialogs | `REQ_Type`, `REQ_TitleText`, `REQ_BodyText`, `REQ_GadgetText` |

### Layout Gadget Orientations:
- `LAYOUT_ORIENT_HORIZ` - Arrange children horizontally
- `LAYOUT_ORIENT_VERT` - Arrange children vertically

---

## ReAction Requesters (Use Instead of EasyRequest)

**IMPORTANT**: For v2.0 ReAction applications, use `requester.class` instead of the old `EasyRequest()` function.

`requester.class` is a BOOPSI class introduced in **Workbench 3.2** that provides native, layout-driven requesters with:
- Consistent ReAction look and feel
- Support for embedded gadgets (string input, integer input, choosers)
- Rich text formatting (bold, italic, underline, colors, fonts)
- Built-in image types (info, question, warning, error, insert disk)
- Proper keyboard shortcuts with `_` underscores in button text

### Basic Requester Pattern:

```c
#include <classes/requester.h>
#include <proto/requester.h>

struct Library *RequesterBase = NULL;
Class *RequesterClass = NULL;

BOOL show_info_requester(struct Window *parent, const char *title, const char *message)
{
    Object *req_obj;
    ULONG result;
    
    RequesterBase = OpenLibrary("requester.class", 0);
    if (!RequesterBase)
        return FALSE;
    
    RequesterClass = REQUESTER_GetClass();
    
    req_obj = NewObject(RequesterClass, NULL,
        REQ_Type, REQTYPE_INFO,
        REQ_TitleText, title,
        REQ_BodyText, message,
        REQ_GadgetText, "_Ok",
        REQ_Image, REQIMAGE_INFO,
        TAG_DONE);
    
    if (req_obj)
    {
        struct orRequest req_msg;
        req_msg.MethodID = RM_OPENREQ;
        req_msg.or_Attrs = NULL;
        req_msg.or_Window = parent;
        req_msg.or_Screen = NULL;
        
        result = DoMethodA(req_obj, (Msg)&req_msg);
        DisposeObject(req_obj);
    }
    
    CloseLibrary(RequesterBase);
    return (result != 0);
}
```

### Requester Types:
| Type | Purpose |
|------|---------|  
| `REQTYPE_INFO` | Simple information/confirmation dialog |
| `REQTYPE_INTEGER` | Get numeric input from user |
| `REQTYPE_STRING` | Get text input from user |

### Built-in Images:
- `REQIMAGE_DEFAULT` - Auto-selects based on button count
- `REQIMAGE_INFO` - Information ("!" sign)
- `REQIMAGE_QUESTION` - Question ("?" sign)  
- `REQIMAGE_WARNING` - Warning triangle
- `REQIMAGE_ERROR` - Error sign
- `REQIMAGE_INSERTDISK` - Insert disk sign

### Body Text Formatting:
```c
/* Bold, italic, underline */
"\33bBold\33n \33iItalic\33n \33uUnderline\33n"

/* Centered text */
"\33cThis text is centered"

/* Custom font */
"\33f[topaz.font/8]This uses Topaz 8"
```

---

## Naming Convention for AmigaOS SDK Collision Avoidance

**CRITICAL**: AmigaOS headers define many short common identifiers. To avoid collisions:

### Prefix ALL Project Symbols:
- **Types**: `iTidy_` prefix → `iTidy_ProgressWindow`, `iTidy_IconArray`
- **Functions**: Use `snake_case` → `itidy_open_progress_window()`, `itidy_update_progress()`
- **Constants/Macros**: `ITIDY_` prefix (ALL CAPS) → `ITIDY_BAR_HEIGHT`, `ITIDY_DEFAULT_WIDTH`
- **Struct fields**: Use `snake_case` → `shine_pen`, `shadow_pen`, `fill_pen`

### Parameter Naming:
- **Geometry**: Use `left, top, width, height` (NOT `x, y, w, h`)
- **Naming style**: **snake_case** for everything → `shine_pen` not `shinePen`

---

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

**Fast RAM Optimization (CRITICAL for Performance)**:
`whd_malloc()` uses `AllocVec(size, MEMF_ANY | MEMF_CLEAR)` on Amiga, which prefers Fast RAM when available. This provides **15x performance improvement** on systems with Fast RAM.

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
   - `NewObject()` → `DisposeObject()` (ReAction)
4. **Free in reverse order** of allocation when possible
5. **Unlock before returning** on error paths

---

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

---

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

/* ReAction objects */
Object *window_obj;
Object *layout_obj;
Object *button_obj;
Class *WindowClass;
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
    // 4-byte fields first (int, pointers, ULONG, Object*)
    int icon_x, icon_y, icon_width, icon_height;
    char *icon_text;
    Object *window_obj;
    ULONG file_size;
    
    // struct fields (DateStamp = 12 bytes)
    struct DateStamp file_date;
    
    // 2-byte fields last (BOOL on Amiga = 2 bytes)
    BOOL is_folder;
    BOOL is_write_protected;
} FullIconDetails;
```

---

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

---

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

---

## Project Structure

```
iTidy/
├── .github/
│   ├── copilot-instructions.md        (this file - v2.0 ReAction)
│   └── copilot-instructions-v1-GadTools.md  (archived v1.0)
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
│   ├── GUI/                # GUI window modules (to be updated for ReAction)
│   ├── Settings/           # Preferences handling
│   ├── platform/           # Platform abstraction
│   ├── templates/          # AI-friendly window templates
│   │   ├── AI_AGENT_GETTING_STARTED.md     # ReAction entry point (TODO)
│   │   ├── AI_AGENT_REACTION_GUIDE.md      # ReAction patterns (TODO)
│   │   ├── GadTools_AI_AGENT_*.md          # Archived v1.0 docs
│   │   └── GadTools_amiga_window_*.c       # Archived v1.0 templates
│   └── tests/              # Test code (GCC host testing only)
└── Makefile                # VBCC build configuration
```

---

## Global Structures and Settings

### System Preferences (Loaded at Startup)

iTidy loads two critical system preference structures at startup:

#### 1. **IControlPrefs** - Interface Control Settings
- **Header**: `src/Settings/IControlPrefs.h`
- **Global Variable**: `prefsIControl` (type: `struct IControlPrefsDetails`)
- **Purpose**: Contains system-wide UI settings from Workbench preferences

#### 2. **WorkbenchPrefs** - Workbench Settings
- **Header**: `src/Settings/WorkbenchPrefs.h`
- **Global Variable**: `prefsWorkbench` (type: `struct WorkbenchSettings`)
- **Purpose**: Contains Workbench-specific settings and icon support flags

### Application Preferences (User Settings)

#### **LayoutPreferences** - Main Configuration Structure
- **Header**: `src/layout_preferences.h`
- **Global Accessor**: `GetGlobalPreferences()` (returns const pointer)
- **Updater**: `UpdateGlobalPreferences(const LayoutPreferences *newPrefs)`
- **Purpose**: Master configuration for all iTidy operations

---

## Common Patterns

### Error Handling Pattern:
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
```

### String Handling:
```c
// Always bounds-check and null-terminate
char buffer[256];
strncpy(buffer, source, sizeof(buffer) - 1);
buffer[sizeof(buffer) - 1] = '\0';

// Use STRPTR for AmigaOS string parameters
void process_path(STRPTR path)
{
    // ...
}
```

---

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

---

## Important Notes for AI Agents

1. **C99 syntax allowed** - `//` comments, inline declarations are OK
2. **snake_case naming** - All functions, variables use snake_case
3. **Use AmigaOS types** - BOOL, STRPTR, BPTR, LONG, ULONG, etc.
4. **Prefix project symbols** - iTidy_/ITIDY_ for types/constants
5. **Use whd_malloc/whd_free** - ALWAYS use memory wrappers in iTidy modules
6. **Check all allocations** - Every allocation must be checked for NULL
7. **Always cleanup** - Match every Lock with UnLock, every NewObject with DisposeObject
8. **Avoid goto** - Only use if absolutely necessary and document why
9. **Log appropriately** - Use category-specific logging functions
10. **Test on target** - Code must run on real Amiga/WinUAE, not just compile
11. **Workbench 3.2+ required** - Target WB 3.2 only for v2.0
12. **Use ReAction classes** - No GadTools for v2.0 development
13. **Layout automatically** - Let layout.gadget handle positioning
14. **DisposeObject disposes children** - Never manually dispose child objects

---

## Development Log and Change History

**CRITICAL RESOURCE**: The `docs/DEVELOPMENT_LOG.md` file contains the complete development history with:
- Detailed bug fix documentation with root cause analysis
- Performance optimization discoveries and benchmarks
- Architecture decisions and rationale
- Known issues and workarounds
- Recent changes (check latest entries first)

**Before implementing any change**, search the development log for related issues or prior work.

---

## Additional Resources

### Official AmigaOS AutoDocs Reference:
The `docs/AutoDocs/` folder contains the **official Commodore/Hyperion AutoDocs** for all AmigaOS libraries and ReAction classes. **Use these as the authoritative reference** when implementing ReAction code.

**Key ReAction AutoDocs for v2.0:**
| File | Description |
|------|-------------|
| `window_cl.doc` | window.class - Window management, WM_OPEN, WM_CLOSE, WM_HANDLEINPUT |
| `layout_gc.doc` | layout.gadget - Automatic GUI layout arrangement |
| `button_gc.doc` | button.gadget - Push buttons, toggle buttons |
| `listbrowser_gc.doc` | listbrowser.gadget - Advanced list display (replaces ListView) |
| `chooser_gc.doc` | chooser.gadget - Dropdown selection (replaces Cycle) |
| `checkbox_gc.doc` | checkbox.gadget - Boolean toggles |
| `string_gc.doc` | string.gadget - Text input fields |
| `integer_gc.doc` | integer.gadget - Numeric input |
| `getfile_gc.doc` | getfile.gadget - File requester |
| `getfont_gc.doc` | getfont.gadget - Font requester |
| `scroller_gc.doc` | scroller.gadget - Scrollbars |
| `slider_gc.doc` | slider.gadget - Slider controls |
| `label_ic.doc` | label.image - Text labels |
| `bevel_ic.doc` | bevel.image - Beveled borders |
| `requester_cl.doc` | requester.class - Modal dialogs (replaces EasyRequest) |

**Other useful AutoDocs:**
| File | Description |
|------|-------------|
| `intuition.doc` | Core Intuition functions |
| `dos.doc` | DOS library functions |
| `icon.doc` | Icon library functions |
| `exec.doc` | Exec library functions |
| `utility.doc` | Utility library (tag handling, hooks) |

**Usage:** When unsure about a ReAction tag, method, or attribute, search the relevant `.doc` file for the exact specification.

### v2.0 ReAction Documentation (TODO: Create these):
1. **`src/templates/AI_AGENT_GETTING_STARTED.md`** - ReAction entry point
2. **`src/templates/AI_AGENT_REACTION_GUIDE.md`** - ReAction patterns and rules

### Archived v1.0 GadTools Documentation:
3. `.github/copilot-instructions-v1-GadTools.md` - Original GadTools instructions
4. `src/templates/GadTools_AI_AGENT_GETTING_STARTED.md` - GadTools entry point
5. `src/templates/GadTools_AI_AGENT_LAYOUT_GUIDE.md` - GadTools layout patterns
6. `src/templates/GadTools_amiga_window_template.c` - GadTools window template

### General Documentation:
7. `docs/DEVELOPMENT_LOG.md` - Complete history of bugs, fixes, and decisions
8. `docs/MEMORY_TRACKING_QUICKSTART.md` - Memory debugging guide
9. `Makefile` - Build configuration and flags

---

## Version Information

- **iTidy Version**: 2.0 (ReAction GUI rewrite)
- **Target**: AmigaOS / Workbench 3.2+ (V47+)
- **Compiler**: VBCC v0.9x (with C99 support)
- **SDK**: NDK 3.2
- **Language**: C89/C99 hybrid
- **GUI**: ReAction (window.class, layout.gadget, etc.)
- **Minimum CPU**: 68020

---

## Critical Checklist When Suggesting Code Changes

**Before writing any window/GUI code (v2.0):**
- [ ] ⚠️ Are you using ReAction classes (NOT GadTools)?
- [ ] ⚠️ Are you using layout.gadget for automatic positioning?
- [ ] ⚠️ Are you using `NewObject()` / `DisposeObject()` correctly?
- [ ] ⚠️ Are you only disposing the top-level window object (not children)?
- [ ] Are you using snake_case for all identifiers?
- [ ] Have you checked all allocations for NULL?
- [ ] Have you matched all Lock/UnLock, NewObject/DisposeObject pairs?
- [ ] Does the code target Workbench 3.2+?
- [ ] Will this compile with VBCC for 68k Amiga?

**When working with ReAction:**
- [ ] Have you opened all required class libraries?
- [ ] Have you obtained class pointers with `*_GetClass()`?
- [ ] Are you using `RA_HandleInput()` for the event loop?
- [ ] Are you using `GetAttr()` / `SetAttrs()` for gadget values?
- [ ] Are you closing class libraries in reverse order?

**When working with structures:**
- [ ] Are fields grouped by size (4-byte fields first, then 2-byte BOOL fields last)?
- [ ] Are you aware that BOOL is 2 bytes on Amiga (not 1 or 4)?

**When updating the development log (docs\DEVELOPMENT_LOG.md):**
- [ ] Have you documented the change with date, author, and description?
- [ ] Is the update brief but informative?
- [ ] **CRITICAL**: Do NOT include code snippets in development log entries
- [ ] Focus on describing WHAT changed, WHY it changed, and the IMPACT

---

## TODO: ReAction Migration Tasks

The following documentation and code needs to be created for v2.0:

1. [ ] **`src/templates/AI_AGENT_GETTING_STARTED.md`** - Update for ReAction patterns
2. [ ] **`src/templates/AI_AGENT_REACTION_GUIDE.md`** - New ReAction-specific guide
3. [ ] **`src/templates/reaction_window_template.c`** - Canonical ReAction window template
4. [ ] **Update `src/GUI/*.c`** - Convert existing windows from GadTools to ReAction
5. [ ] **Update `Makefile`** - Add ReAction class library linking if needed
6. [ ] **Test all windows** - Verify ReAction migration works on WB 3.2

### Key ReAction Classes to Learn:
- `window.class` - Main window management
- `layout.gadget` - Automatic gadget arrangement
- `listbrowser.gadget` - Replaces GadTools ListView
- `chooser.gadget` - Replaces GadTools Cycle
- `getfile.gadget` / `getfont.gadget` - File/font requesters
