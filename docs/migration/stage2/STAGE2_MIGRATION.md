# iTidy - Stage 2 VBCC Migration

## Overview

Stage 2 of the iTidy VBCC migration focuses on modernizing file I/O, disk utilities, and core utility functions to use native AmigaDOS APIs and VBCC-compatible C99 features.

## Modules Migrated

### 1. **src/writeLog.c / writeLog.h** - Logging System
   - ✅ Migrated from standard C FILE* to AmigaDOS BPTR
   - ✅ Replaced fopen/fprintf/fclose with Open/Write/Close
   - ✅ Added fallback from `Bin/Amiga/logs/iTidy.log` to `T:iTidy.log`
   - ✅ Improved error handling with IoErr()
   - ✅ Maintained timestamp functionality

### 2. **src/DOS/getDiskDetails.c / getDiskDetails.h** - Disk Information
   - ✅ Removed outdated system() command approach
   - ✅ Implemented native Info() API for disk queries
   - ✅ Replaced custom bool typedef with AmigaDOS BOOL
   - ✅ Eliminated temporary file creation
   - ✅ Direct access to InfoData structure
   - ✅ Proper error handling with IoErr()

### 3. **src/utilities.c / utilities.h** - Core Utilities
   - ✅ Fixed `does_file_or_folder_exist()` to use BPTR instead of void*
   - ✅ Changed return type from bool to BOOL for consistency
   - ✅ Added C99 improvements (inline, //, mixed declarations)
   - ✅ Improved string safety with bounds checking
   - ✅ Maintained platform abstraction layer

### 4. **src/file_directory_handling.c / file_directory_handling.h** - File Operations
   - ✅ Replaced AllocMem/FreeMem with AllocVec/FreeVec
   - ✅ Upgraded to AllocDosObject/FreeDosObject for FileInfoBlock
   - ✅ Replaced sprintf with snprintf for safety
   - ✅ Replaced strcpy/strcat with safe variants
   - ✅ Added proper error handling with IoErr()
   - ✅ Verified all Lock/UnLock pairs

## Key Improvements

### AmigaDOS API Modernization
- **Correct Types**: All code now uses proper AmigaDOS types (BPTR, LONG, UBYTE, BOOL)
- **Native APIs**: Direct use of Lock(), Examine(), ExNext(), Info(), Open(), Write(), Close()
- **Error Handling**: Proper use of IoErr() throughout
- **Memory Management**: Consistent use of AllocVec/FreeVec and AllocDosObject/FreeDosObject

### C99 Features (VBCC-Compatible)
- **Inline Functions**: Small helpers marked with inline keyword
- **Mixed Declarations**: Variables declared where needed for clarity
- **Modern Comments**: Single-line // comments used appropriately
- **Bool Type**: Internal logic can use bool, AmigaDOS APIs use BOOL
- **Safe String Functions**: snprintf, vsnprintf, strncpy, strncat used throughout

### Safety Improvements
- **Buffer Overflow Protection**: All string operations bounds-checked
- **Memory Leak Prevention**: Proper cleanup on all code paths
- **Lock Management**: Verified Lock/UnLock pairing
- **Error Paths**: All error conditions properly handled

## Build Configuration

### Compiler Settings
```makefile
CC = vc
CFLAGS = +aos68k -c99 -cpu=68020 -I$(INC_DIR) -Isrc -DPLATFORM_AMIGA=1 -D__AMIGA__
LDFLAGS = +aos68k -cpu=68020 -lamiga -lauto -lmieee
```

### Build Commands
```bash
# Clean build
make clean-all

# Build for Amiga
make amiga

# Output: Bin/Amiga/iTidy
```

## API Reference

### AmigaDOS Functions Used

#### File I/O
```c
BPTR Open(CONST_STRPTR name, LONG mode);
LONG Close(BPTR file);
LONG Read(BPTR file, APTR buffer, LONG length);
LONG Write(BPTR file, CONST APTR buffer, LONG length);
```

#### File System
```c
BPTR Lock(CONST_STRPTR name, LONG mode);
void UnLock(BPTR lock);
BOOL Examine(BPTR lock, struct FileInfoBlock *fib);
BOOL ExNext(BPTR lock, struct FileInfoBlock *fib);
BOOL Info(BPTR lock, struct InfoData *infoData);
```

#### Memory
```c
APTR AllocVec(ULONG size, ULONG flags);
void FreeVec(APTR memoryBlock);
APTR AllocDosObject(ULONG type, CONST struct TagItem *tags);
void FreeDosObject(ULONG type, APTR ptr);
```

#### Error Handling
```c
LONG IoErr(void);  // Returns last DOS error code
```

### Important Constants

#### Lock Modes
- `ACCESS_READ` (-2): Shared read lock
- `SHARED_LOCK` (-2): Same as ACCESS_READ
- `ACCESS_WRITE` (-1): Exclusive write lock (rarely used)

#### File Modes
- `MODE_OLDFILE` (1005): Open existing file for read
- `MODE_NEWFILE` (1006): Create new file or truncate existing
- `MODE_READWRITE` (1004): Open for read/write

#### Protection Bits
- `FIBF_DELETE` (0x01): Delete protection
- `FIBF_WRITE` (0x02): Write protection
- `FIBF_READ` (0x04): Read protection
- `FIBF_EXECUTE` (0x08): Execute protection

#### Disk States
- `ID_WRITE_PROTECTED` (80): Disk is write-protected
- `ID_VALIDATED` (82): Disk is validated
- `ID_VALIDATING` (83): Disk is being validated

#### DOS Object Types
- `DOS_FIB`: FileInfoBlock structure

#### Memory Flags
- `MEMF_CLEAR`: Clear memory to zero after allocation
- `MEMF_PUBLIC`: Accessible by all tasks

## Migration Patterns

### Pattern 1: File I/O Migration

**Before (Standard C):**
```c
FILE *file = fopen("logfile.txt", "a");
if (file) {
    fprintf(file, "Message\n");
    fclose(file);
}
```

**After (AmigaDOS):**
```c
BPTR file = Open("Bin/Amiga/logs/iTidy.log", MODE_NEWFILE);
if (!file) {
    file = Open("T:iTidy.log", MODE_NEWFILE);
}
if (file) {
    Write(file, "Message\n", 8);
    Close(file);
}
```

### Pattern 2: Disk Information

**Before (system() command):**
```c
system("info DH0: > RAM:info_output.txt");
FILE *file = fopen("RAM:info_output.txt", "r");
// Parse text output...
```

**After (Native AmigaDOS):**
```c
BPTR lock = Lock("DH0:", SHARED_LOCK);
struct InfoData *info = AllocVec(sizeof(struct InfoData), MEMF_CLEAR);
if (lock && info) {
    if (Info(lock, info)) {
        LONG totalBytes = info->id_NumBlocks * info->id_BytesPerBlock;
        // Use data directly...
    }
    UnLock(lock);
    FreeVec(info);
}
```

### Pattern 3: File Existence Check

**Before:**
```c
void *lock = Lock(filename, ACCESS_READ);
if (lock != NULL) {
    UnLock(lock);
    return true;
}
```

**After:**
```c
BPTR lock = Lock(filename, ACCESS_READ);
if (lock) {
    UnLock(lock);
    return TRUE;
}
```

### Pattern 4: Memory Allocation

**Before:**
```c
struct FileInfoBlock *fib = 
    AllocMem(sizeof(struct FileInfoBlock), MEMF_PUBLIC | MEMF_CLEAR);
// ...
FreeMem(fib, sizeof(struct FileInfoBlock));
```

**After (Better):**
```c
struct FileInfoBlock *fib = 
    AllocVec(sizeof(struct FileInfoBlock), MEMF_CLEAR);
// ...
FreeVec(fib);
```

**After (Best for FileInfoBlock):**
```c
struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
// ...
FreeDosObject(DOS_FIB, fib);
```

### Pattern 5: Safe String Operations

**Before:**
```c
sprintf(buffer, "%s/%s", path, filename);
strcpy(dest, src);
strcat(dest, extension);
```

**After:**
```c
snprintf(buffer, sizeof(buffer), "%s/%s", path, filename);
strncpy(dest, src, destSize - 1);
dest[destSize - 1] = '\0';
strncat(dest, extension, destSize - strlen(dest) - 1);
```

## Testing

### Test Checklist

#### Logging System
- [x] Log writes to `Bin/Amiga/logs/iTidy.log` when directory exists
- [x] Fallback to `T:iTidy.log` works when primary path unavailable
- [x] Timestamps are formatted correctly
- [x] Long messages don't cause buffer overflow
- [x] Error handling works when disk is full/write-protected

#### Disk Information
- [x] Works with hard disk devices (DH0:, Work:)
- [x] Works with floppy drives (DF0:, DF1:)
- [x] Works with RAM: disk
- [x] Correctly detects write-protected media
- [x] Handles invalid/non-existent devices gracefully
- [x] Size calculations are accurate
- [x] Percentage calculations are correct

#### File Operations
- [x] `does_file_or_folder_exist()` works with various paths
- [x] Works with both files and directories
- [x] Handles non-existent paths correctly
- [x] `ProcessDirectory()` handles recursive subdirectories
- [x] WHDLoad folder detection works
- [x] Path sanitization removes double slashes
- [x] Protection bit functions work correctly
- [x] No memory leaks detected

## Common Issues and Solutions

### Issue 1: BPTR vs void*
**Problem:** Using `void*` for lock variables causes type warnings.
**Solution:** Always use `BPTR` for locks and file handles.

### Issue 2: NULL vs 0 for BPTR
**Problem:** Comparing BPTR to NULL can cause warnings.
**Solution:** Use `if (!lock)` or `if (lock == 0)` instead of `if (lock == NULL)`.

### Issue 3: Buffer Overflows
**Problem:** Using sprintf/strcpy without bounds checking.
**Solution:** Always use snprintf/strncpy and check buffer sizes.

### Issue 4: Memory Leaks
**Problem:** Forgetting to UnLock() or FreeVec() on error paths.
**Solution:** Use consistent error handling pattern with cleanup before every return.

### Issue 5: Wrong AllocDosObject Type
**Problem:** Using wrong type parameter for AllocDosObject.
**Solution:** Use `DOS_FIB` for FileInfoBlock, check documentation for other types.

## Compilation Results

### Build Output
```
vc +aos68k -c99 -cpu=68020 -c src/writeLog.c -o build/amiga/writeLog.o
vc +aos68k -c99 -cpu=68020 -c src/DOS/getDiskDetails.c -o build/amiga/DOS/getDiskDetails.o
vc +aos68k -c99 -cpu=68020 -c src/utilities.c -o build/amiga/utilities.o
vc +aos68k -c99 -cpu=68020 -c src/file_directory_handling.c -o build/amiga/file_directory_handling.o
```

### Status
✅ **All modules compile cleanly with no warnings**
✅ **Linker resolves all symbols correctly**
✅ **Binary created successfully in `Bin/Amiga/iTidy`**

## Next Steps (Stage 3)

Stage 3 will focus on:
- Icon management modules (icon_types.c, icon_misc.c, icon_management.c)
- Window management (window_management.c)
- Settings modules (IControlPrefs.c, WorkbenchPrefs.c, get_fonts.c)
- Final integration testing

## Documentation

- **Detailed Notes**: `docs/migration/stage2/migration_stage2_notes.txt`
- **Checklist**: `docs/migration/stage2/VBCC_STAGE2_CHECKLIST.txt`
- **This Summary**: `docs/migration/stage2/STAGE2_MIGRATION.md`
- **Progress**: `docs/migration/MIGRATION_PROGRESS.md`

## Credits

Migration performed using VBCC v0.9x targeting AmigaOS 3.2 (Workbench 3.2 NDK).

---

**Stage 2 Status: ✅ COMPLETE**

All four modules successfully migrated to use native AmigaDOS APIs with VBCC-compatible C99 features.
