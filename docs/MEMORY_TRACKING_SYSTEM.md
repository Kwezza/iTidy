# Memory Tracking System

## Overview

The iTidy project now includes a comprehensive memory tracking system to detect memory leaks and monitor allocation patterns. All memory allocations go through `whd_malloc()` and `whd_free()` wrappers defined in `include/platform/platform.h`.

## How It Works

When enabled, the system:
- Tracks every `whd_malloc()` call with file/line information
- Tracks every `whd_free()` call and matches it to allocations
- Logs all operations to `PROGDIR:iTidy.log` via `writeLog.c`
- Maintains statistics (total allocated, peak usage, current usage)
- Detects memory leaks and double-frees

## Enabling Memory Tracking

### Step 1: Enable the Feature

Edit `include/platform/platform.h` and uncomment this line:

```c
#define DEBUG_MEMORY_TRACKING
```

### Step 2: Update Your Makefile

Add `platform.c` to your build:

**For the root Makefile:**
```makefile
PLATFORM_OBJ = build/amiga/platform.o

# Add to your object list
OBJECTS = ... $(PLATFORM_OBJ) ...

# Add build rule
$(PLATFORM_OBJ): include/platform/platform.c
	$(CC) $(CFLAGS) -o $@ include/platform/platform.c
```

**For src/makefile (SAS/C):**
```makefile
main: main.o platform.o
    $(SC) link to iTidy $(CFLAGS) ... platform.o

platform.o: ../include/platform/platform.c
    $(SC) $(CFLAGS) ../include/platform/platform.c
```

### Step 3: Initialize and Report

Add to your `main.c`:

```c
#include <platform/platform.h>

int main(int argc, char **argv) {
    /* Initialize memory tracking at program start */
    whd_memory_init();
    
    /* ... your program code ... */
    
    /* Report memory status before exit */
    whd_memory_report();
    
    return 0;
}
```

## Output Examples

### Successful Allocation
```
[14:23:45] MALLOC: 256 bytes at 0x12345678 (icon_management.c:56) [Current: 256 bytes, Peak: 256 bytes]
```

### Successful Free
```
[14:23:46] FREE: 256 bytes at 0x12345678 (allocated at icon_management.c:56, freed at icon_management.c:120) [Current: 0 bytes]
```

### Memory Leak Detection
```
========== MEMORY TRACKING REPORT ==========
Total allocations: 45 (12,584 bytes)
Total frees: 43 (12,328 bytes)
Peak memory usage: 5,432 bytes
Current memory: 256 bytes

*** MEMORY LEAKS DETECTED: 2 blocks, 256 bytes ***

Leak details:
  - 128 bytes at 0x12345678 (allocated at icon_management.c:515)
  - 128 bytes at 0x12345ABC (allocated at icon_types.c:36)
============================================
```

### No Leaks
```
========== MEMORY TRACKING REPORT ==========
Total allocations: 45 (12,584 bytes)
Total frees: 45 (12,584 bytes)
Peak memory usage: 5,432 bytes
Current memory: 0 bytes

*** NO MEMORY LEAKS DETECTED - ALL ALLOCATIONS FREED ***
============================================
```

## Error Detection

The system detects and logs:

1. **Failed Allocations**: When malloc() returns NULL
2. **Double Free**: Attempting to free memory not in tracking list
3. **Null Pointer Free**: Freeing NULL (legal but logged)
4. **Untracked Free**: Freeing memory that wasn't allocated through whd_malloc()

## Performance Considerations

When `DEBUG_MEMORY_TRACKING` is enabled:
- Each allocation/free creates a log entry (I/O overhead)
- Tracking structures add memory overhead (~32 bytes per allocation)
- Good for development and testing
- **Disable for release builds** by commenting out the #define

## Integration with Existing Code

The system works seamlessly with existing code because:
- All `whd_malloc()`/`whd_free()` calls are already in place
- When disabled, they compile to direct `malloc()`/`free()` calls (zero overhead)
- No source code changes needed to existing allocation sites

## Checking for Leaks

1. Enable tracking
2. Run your program through test scenarios
3. Check `PROGDIR:iTidy.log` for the memory report
4. Fix any reported leaks
5. Retest until report shows 0 leaks

## Notes on Direct Allocations

Some parts of iTidy use direct `AllocVec()`/`malloc()` calls:
- `main.c` - some Amiga-specific allocations
- `Workbench/whd/` directory - WHDLoad-specific code

These won't be tracked. To add tracking, replace:
```c
ptr = AllocVec(size, flags);
```
with:
```c
ptr = whd_malloc(size);
```

## Tips

1. **Run at program start and exit**: Initialize tracking early, report late
2. **Check log regularly**: Don't wait until the end of development
3. **Test different code paths**: Each feature may have different allocation patterns
4. **Use with valgrind**: On host builds, combine with valgrind for extra validation
5. **Disable for performance testing**: Logging overhead affects benchmarks

## Troubleshooting

**Issue**: Compilation errors about writeLog.h
**Solution**: Verify `platform.c` path to `../../src/writeLog.h` is correct

**Issue**: No output in log
**Solution**: Call `whd_memory_init()` at program start

**Issue**: Huge log files
**Solution**: This is normal with many allocations. Consider testing smaller scenarios or periodically deleting the log.

**Issue**: "Untracked pointer" errors
**Solution**: Some memory was allocated with direct malloc()/AllocVec(). Convert to whd_malloc().
