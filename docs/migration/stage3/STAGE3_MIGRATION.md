# iTidy - Stage 3 VBCC Migration

## Overview

Stage 3 of the iTidy VBCC migration focuses on modernizing icon management, window management, and system preferences handling to use native AmigaOS 3.2 APIs with VBCC-compatible C99 features.

## Modules Migrated

### 1. **src/icon_types.c / icon_types.h** - Icon Type Detection & Sizing
   - ✅ Modernized icon.library usage with proto/icon.h
   - ✅ Replaced legacy GetDiskObject() usage with proper error handling
   - ✅ Updated all file handle types to BPTR
   - ✅ Normalized BOOL usage for API-facing functions
   - ✅ Added robust IoErr() logging for icon loading failures
   - ✅ Proper FreeDiskObject() cleanup on all code paths
   - ✅ Support for NewIcons, OS3.5 icons, and standard icon formats
   - ✅ C99 features: inline helpers, mixed declarations, // comments

### 2. **src/icon_misc.c / icon_misc.h** - Icon Miscellaneous Functions
   - ✅ Converted from void* to BPTR for file handles
   - ✅ Replaced platform-specific file I/O with AmigaDOS APIs
   - ✅ Updated FGets() usage for reading .backdrop files
   - ✅ Proper Open()/Close() with MODE_OLDFILE
   - ✅ Safe string handling with snprintf() throughout
   - ✅ Maintained left-out icon tracking functionality
   - ✅ Added IoErr() checks on file operations
   - ✅ Verified BPTR initialization to 0 instead of NULL

### 3. **src/icon_management.c / icon_management.h** - Icon Management Core
   - ✅ Complete icon.library modernization
   - ✅ GetDiskObject()/PutDiskObject()/FreeDiskObject() usage
   - ✅ LayoutIconA() for proper icon layout calculations
   - ✅ DrawIconState() for icon rendering (when needed)
   - ✅ GetIconTagList() for tag-based icon queries
   - ✅ WHDLoad folder icon handling preserved
   - ✅ Memory management: AllocVec(MEMF_CLEAR)/FreeVec()
   - ✅ Consistent error logging via append_to_log()
   - ✅ All BPTR locks properly paired with UnLock()

### 4. **src/window_management.c / window_management.h** - Window & Screen Management
   - ✅ Modernized intuition.library usage
   - ✅ GetDefaultPubScreen()/LockPubScreen()/UnlockPubScreen()
   - ✅ Proper struct Window, struct Screen, struct RastPort types
   - ✅ Replaced magic constants with symbolic IDCMP_ flags
   - ✅ Included proto/intuition.h and proto/graphics.h
   - ✅ Verified CloseWindow()/UnlockPubScreen() on all paths
   - ✅ OpenFont()/CloseFont() for font handling
   - ✅ Proper screen locking for font measurements
   - ✅ Resource cleanup in error conditions

### 5. **src/Settings/IControlPrefs.c / IControlPrefs.h** - IControl Preferences
   - ✅ Modernized IControl preferences reading
   - ✅ Proper struct IControlPrefs handling
   - ✅ Window border, title bar, and aspect ratio detection
   - ✅ Support for ICF_CORRECT_RATIO and square proportional modes
   - ✅ Version checking for IC_CURRENTVERSION
   - ✅ Default settings initialization
   - ✅ Safe flag extraction and bit masking
   - ✅ Title bar height calculations for different modes

### 6. **src/Settings/WorkbenchPrefs.c / WorkbenchPrefs.h** - Workbench Preferences
   - ✅ IFFParse-based preferences reading (WBNC chunk)
   - ✅ Proper BPTR file handle usage with Open()/Close()
   - ✅ AllocMem(MEMF_PUBLIC|MEMF_CLEAR)/FreeMem() for buffers
   - ✅ MAKE_ID() macro for chunk type identification
   - ✅ Support for WorkbenchPrefs and WorkbenchExtendedPrefs
   - ✅ Borderless window detection
   - ✅ NewIcons and ColorIcons support flags
   - ✅ IoErr() logging on file open failures
   - ✅ Proper chunk padding handling (even size alignment)

### 7. **src/Settings/get_fonts.c / get_fonts.h** - Font Preferences
   - ✅ Workbench icon font extraction from ENV:sys/font.prefs
   - ✅ Fallback to ENV:sys/wbfont.prefs for WB 2.x
   - ✅ BPTR file handle usage with Open()/Read()/Close()
   - ✅ Default topaz.font 8pt fallback
   - ✅ Support for both font.prefs (WB 3.x) and wbfont.prefs (WB 2.x)
   - ✅ Proper endianness handling (big-endian vs little-endian)
   - ✅ Fixed-point to integer conversion for font sizes
   - ✅ malloc()/free() for FontPref structures
   - ✅ Safe string copying with bounds checking

## Key Improvements

### Icon.library API Modernization
- **Auto-initialization**: Using -lauto linker flag for automatic library opening
- **Proper Types**: struct DiskObject*, BPTR for locks/files
- **Tag-based APIs**: GetIconTagList() for extended icon queries
- **Layout Functions**: LayoutIconA() for proper icon positioning
- **Resource Management**: FreeDiskObject() on all code paths
- **Error Handling**: IoErr() checks with append_to_log() reporting

### Intuition.library & Graphics.library Updates
- **Public Screens**: LockPubScreen("Workbench")/UnlockPubScreen()
- **Modern Structs**: struct Window*, struct Screen*, struct RastPort*
- **Symbolic Constants**: IDCMP_CLOSEWINDOW, WFLG_*, SCREENTYPE_*
- **Font Management**: OpenFont()/CloseFont() with proper TextFont handling
- **Window Sizing**: Proper calculation based on screen dimensions
- **Cleanup**: CloseWindow() and screen unlocking on all exit paths

### Preferences Reading (IFFParse & DOS)
- **IFF Chunks**: FORM/PREF/WBNC chunk parsing
- **Binary Reading**: Direct struct casting from chunk data
- **Fallback Defaults**: InitializeDefaultWorkbenchSettings()
- **Version Checks**: IC_CURRENTVERSION for IControl compatibility
- **Font Prefs**: FONT chunk extraction with endianness handling
- **Error Resilience**: Default values when preferences unavailable

### Memory & Safety
- **AllocVec/FreeVec**: Consistent memory allocation with MEMF_CLEAR
- **AllocDosObject**: For FileInfoBlock and other DOS structures
- **AllocMem/FreeMem**: For temporary IFF chunk buffers
- **snprintf**: All string formatting bounds-checked
- **BPTR Consistency**: 0 initialization, proper Lock/UnLock pairing
- **Resource Cleanup**: Comprehensive cleanup on all error paths

### Logging & Debugging
- **append_to_log()**: Consistent progress and error logging
- **Conditional Logs**: #ifdef DEBUG for verbose output
- **IoErr() Reporting**: All I/O failures logged with error codes
- **Library Failures**: icon.library, intuition.library, diskfont.library
- **Trace Points**: Lock acquisition, icon loading, window opening

## Build Configuration

### Compiler Settings
```makefile
CC = vc
CFLAGS = +aos68k -c99 -cpu=68020 -I$(INC_DIR) -Isrc -DPLATFORM_AMIGA=1 -D__AMIGA__
LDFLAGS = +aos68k -cpu=68020 -lamiga -lauto -lmieee
```

### Required Libraries
- **icon.library**: GetDiskObject, PutDiskObject, FreeDiskObject, LayoutIconA
- **intuition.library**: OpenWindow, CloseWindow, LockPubScreen, UnlockPubScreen
- **graphics.library**: OpenFont, CloseFont, Text, TextLength
- **diskfont.library**: Font enumeration and management
- **workbench.library**: Workbench integration
- **iffparse.library**: IFF chunk parsing (implied by prefs reading)

### Build Commands
```bash
# Clean build
make clean-all

# Build for Amiga
make amiga

# Output: Bin/Amiga/iTidy
```

## API Reference

### Icon.library Functions Used

#### Core Icon Functions
```c
struct DiskObject *GetDiskObject(CONST_STRPTR name);
BOOL PutDiskObject(CONST_STRPTR name, struct DiskObject *diskobj);
void FreeDiskObject(struct DiskObject *diskobj);
```

#### Icon Layout & Rendering
```c
BOOL LayoutIconA(struct DiskObject *icon, struct Screen *screen, struct TagItem *tags);
void DrawIconState(struct RastPort *rp, struct DiskObject *icon, 
                   CONST_STRPTR label, LONG x, LONG y, ULONG state, struct TagItem *tags);
struct DiskObject *GetIconTagList(CONST_STRPTR name, struct TagItem *tags);
```

#### Icon Structures
```c
struct DiskObject {
    UWORD do_Magic;
    UWORD do_Version;
    struct Gadget do_Gadget;
    UBYTE do_Type;
    char *do_DefaultTool;
    char **do_ToolTypes;
    LONG do_CurrentX;
    LONG do_CurrentY;
    struct DrawerData *do_DrawerData;
    char *do_ToolWindow;
    LONG do_StackSize;
};
```

### Intuition.library Functions Used

#### Window Management
```c
struct Window *OpenWindowTags(struct NewWindow *nw, struct TagItem *tags);
void CloseWindow(struct Window *window);
struct Screen *LockPubScreen(CONST_STRPTR name);
void UnlockPubScreen(CONST_STRPTR name, struct Screen *screen);
struct Screen *GetDefaultPubScreen(void);
```

#### Font Management
```c
struct TextFont *OpenFont(struct TextAttr *textAttr);
void CloseFont(struct TextFont *font);
LONG TextLength(struct RastPort *rp, CONST_STRPTR string, ULONG count);
```

### Graphics.library Functions Used

#### Text Rendering
```c
void Text(struct RastPort *rp, CONST_STRPTR string, ULONG count);
LONG TextExtent(struct RastPort *rp, CONST_STRPTR string, ULONG count, struct TextExtent *te);
```

### DOS.library Functions Used (Stage 3)

#### File I/O
```c
BPTR Open(CONST_STRPTR name, LONG mode);
LONG Read(BPTR file, APTR buffer, LONG length);
LONG Close(BPTR file);
LONG IoErr(void);
```

#### Preferences Files
- **ENV:sys/font.prefs**: Workbench 3.x font preferences
- **ENV:sys/wbfont.prefs**: Workbench 2.x font preferences
- **ENV:sys/Workbench.prefs**: Workbench display settings
- **ENV:sys/IControl.prefs**: Window aspect ratio and title bar settings

## C99 Features (VBCC-Compatible)

### Language Features Used
- ✅ **Inline Functions**: Small helpers marked with inline keyword
- ✅ **Mixed Declarations**: Variables declared where needed for clarity
- ✅ **Single-line Comments**: // comments used appropriately
- ✅ **Bool Type**: Internal logic uses bool, AmigaDOS APIs use BOOL
- ✅ **snprintf/vsnprintf**: Safe string formatting throughout
- ✅ **Explicit Types**: BPTR, BOOL, LONG, UBYTE, STRPTR for clarity

### Safety Improvements
- ✅ **Buffer Overflow Protection**: All string operations bounds-checked
- ✅ **Memory Leak Prevention**: Proper cleanup on all code paths
- ✅ **Resource Management**: Locks, windows, screens, fonts properly freed
- ✅ **Error Paths**: All error conditions properly handled with logging
- ✅ **Null Checks**: Pointer validation before dereferencing
- ✅ **Initialization**: Explicit initialization of BPTR to 0

## Testing & Validation

### Build Status
```
✅ icon_types.c              - Compiles successfully
✅ icon_misc.c               - Compiles successfully  
✅ icon_management.c         - Compiles successfully
✅ window_management.c       - Compiles successfully
✅ IControlPrefs.c           - Compiles successfully
✅ WorkbenchPrefs.c          - Compiles successfully
✅ get_fonts.c               - Compiles successfully
```

**All seven Stage 3 modules compile cleanly with VBCC!**

### Runtime Testing Checklist
- [ ] Icons load correctly from various formats (standard, NewIcons, OS3.5)
- [ ] Icon positions are preserved when reformatting
- [ ] WHDLoad folder icons maintain layout
- [ ] Windows open on Workbench screen with correct sizing
- [ ] Font preferences are read correctly from ENV: variables
- [ ] Workbench preferences (borderless, title bar) are applied
- [ ] IControl preferences affect window calculations
- [ ] Left-out icons (.backdrop) are respected
- [ ] All resources are freed (no memory leaks)
- [ ] Error conditions are handled gracefully

## Migration Notes

### Breaking Changes
- **BPTR Usage**: Changed from void* to BPTR throughout
- **BOOL Consistency**: API functions now return BOOL instead of bool
- **Library Auto-init**: Removed manual library base declarations
- **Proto Headers**: Using proto/icon.h instead of clib/icon_protos.h

### Backward Compatibility
- ✅ Supports AmigaOS 3.0, 3.1, 3.2
- ✅ Handles both Workbench 2.x and 3.x preference formats
- ✅ Detects and supports NewIcons and ColorIcons
- ✅ Fallback defaults when preferences unavailable
- ✅ Legacy icon formats still supported

### Known Limitations
- Requires icon.library v44+ for full functionality
- Some tag-based APIs may not be available on OS < 3.0
- LayoutIconA() requires graphics.library v40+
- IControl preferences require OS 3.5+ for extended features

## Documentation Files

### Created for Stage 3
- `docs/migration/stage3/STAGE3_MIGRATION.md` (this file)
- `docs/migration/stage3/migration_stage3_notes.txt`
- `docs/migration/stage3/VBCC_STAGE3_CHECKLIST.txt`
- `docs/migration/stage3/DIFFICULTIES_ENCOUNTERED_STAGE3.md`

### Updated
- `docs/migration/MIGRATION_PROGRESS.md` (global summary)

## Next Steps

Stage 3 completes the migration of all icon, window, and preferences management modules. Future stages may include:
- Additional Workbench integration features
- Advanced icon manipulation (color remapping, format conversion)
- Window gadget handling and user interaction
- Comprehensive error recovery and user feedback

---

**Stage 3 Migration Completed**: October 17, 2025
**VBCC Version**: v0.9x with +aos68k target
**Target OS**: AmigaOS 3.2 (Workbench 3.2 NDK)
**C Standard**: C99 subset (VBCC-compatible)
