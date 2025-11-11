# Filesystem Lock Timing Fix - Fast Emulated Systems

**Date:** November 10, 2025  
**Issue:** Recursive directory processing crash (Amiga error #80000003)  
**Status:** Fixed with configurable delay  

## Problem Description

iTidy would crash with error #80000003 (software exception) during deep recursive directory processing on emulated Amiga systems running at maximum speed. The crash occurred at unpredictable depths (sometimes level 2, sometimes level 5+).

## Root Cause

The crash was caused by filesystem lock resource exhaustion, not memory problems.

### Technical Analysis

1. **Nested Lock Pattern:**
   - `ProcessDirectoryRecursive()` calls `ProcessSingleDirectory(path)` which locks the directory
   - Immediately after, it calls `Lock(path)` again for subdirectory enumeration
   - On fast emulated systems, these Lock/UnLock operations occur so rapidly that AmigaOS filesystem doesn't have time to release internal lock table entries

2. **Why Original Hardware Worked:**
   - Real Amiga hardware (7MHz 68000) has natural I/O delays
   - Disk access, memory timing, and system bus speed provided ~20-50ms between operations
   - This was sufficient for DOS to clean up internal filesystem structures

3. **Why Emulators Fail:**
   - Modern emulators running at maximum speed eliminate these natural delays
   - Lock/UnLock operations occur microseconds apart instead of milliseconds
   - DOS lock tables get exhausted, causing system exception

### Evidence

- Memory profiling showed peak usage of only 4.3KB (trivial)
- Increasing stack from 20KB to 80KB made crashes **worse** (counterintuitive if memory was the problem)
- Enabling memory logging (which adds I/O overhead) **completely prevented crashes**
- This is a classic "Heisenbug" - observation changes the behavior

## Solution

Added a configurable delay after each directory is processed to give the filesystem time to release internal resources.

### Configuration

Located in `src/layout_processor.c` (lines 18-77):

```c
/* Enable filesystem lock release delay (disable for debugging/original hardware) */
#define ENABLE_FILESYSTEM_LOCK_DELAY 1

/* Delay in DOS ticks (1 tick = 1/50 second PAL, 1/60 second NTSC) */
#define FILESYSTEM_LOCK_DELAY_TICKS 1
```

### Default Settings

- **Enabled:** Yes (`ENABLE_FILESYSTEM_LOCK_DELAY 1`)
- **Delay:** 1 tick (~20ms PAL, ~17ms NTSC)

### When to Disable

You may want to disable this delay if:
- Running on original Amiga hardware (natural delays are sufficient)
- Debugging lock-related issues
- Testing performance on very slow systems

To disable, change:
```c
#define ENABLE_FILESYSTEM_LOCK_DELAY 0
```

### When to Increase Delay

If crashes still occur on extremely fast emulators, increase the delay:
```c
#define FILESYSTEM_LOCK_DELAY_TICKS 2  /* 40ms PAL, 33ms NTSC */
```

## Performance Impact

- **User Impact:** None - 20ms per directory is imperceptible
- **Processing Speed:** Minimal - only adds delay between directories, not between individual icons
- **Example:** Processing 1000 directories adds ~20 seconds total (still completes in under a minute)

## Testing Results

**Before Fix:**
- Crashed at varying depths (level 2-5+) on WinUAE at maximum speed
- Peak memory usage: 4.3KB
- No memory leaks detected

**After Fix:**
- Processed hundreds of directories without crash
- All memory properly released (0 bytes at end)
- No performance degradation noticeable to users

## Related Files

- `src/layout_processor.c` - Implementation with comprehensive comments
- `docs/CRASH_FIXES_LOG.md` - General crash investigation log
- Memory log example: `Bin/Amiga/logs/memory_2025-11-10_16-03-54.log`

## For Future Developers

This fix addresses a fundamental race condition between user code and AmigaOS filesystem internals. If you encounter similar issues with other AmigaOS resources (memory pools, file handles, etc.), the same pattern applies:

1. **Profile first** - Don't assume it's memory
2. **Look for timing issues** - If enabling logging "fixes" it, it's a Heisenbug
3. **Add delays strategically** - Target resource release points, not random locations
4. **Make it configurable** - Allow users to tune for their specific systems

The AmigaOS filesystem was designed for 7MHz hardware. Modern emulators running at GHz speeds expose timing assumptions that were valid in 1985 but break on fast systems.
