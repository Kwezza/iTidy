# Bug Fix: Window Movement Causing Lockup and Display Corruption

## Issue Description

When the experimental "move open windows" feature was enabled, windows that were moved to match saved geometry would:
1. Become locked in position and unable to be moved by the user
2. Not refresh their contents when other windows moved over them (display corruption)
3. Occasionally trigger Amiga recoverable alerts (black screen with yellow box)
4. **UPDATE**: After fixing the lockup, windows were not moving at all - getting invalid window pointers

## Root Causes

### Issue #1: LockIBase() Blocking Intuition

The `ApplyWindowGeometry()` function in `src/GUI/window_enumerator.c` was using `LockIBase(0)` to lock Intuition while calling `MoveWindow()` and `SizeWindow()`.

**Why This Was Wrong:**
1. **`LockIBase()` blocks ALL Intuition operations** - This includes user input, window dragging, refresh operations, and layer management
2. **`MoveWindow()` and `SizeWindow()` are already safe** - These functions handle their own internal synchronization
3. **No refresh after unlock** - Even after unlocking, the code didn't explicitly refresh the window

### Issue #2: Stale Window Pointers

After fixing the lockup issue, windows still weren't moving because the code was using **cached window pointers** from the beginning of the run:

1. Window tracker was built at the **start** of processing (in `layout_processor.c`)
2. Workbench could close/reopen windows during icon processing (several seconds/minutes)
3. By the time we tried to move windows, the pointers were stale/invalid
4. Window structures showed garbage values like `(16384, -17440) size 0x0`

**Why Cached Pointers Failed:**
- Windows can be closed and reopened by Workbench during icon operations
- Window structures can be reallocated at different memory addresses
- Pointers captured at start of run become invalid after window refresh
- No way to detect if a pointer is still valid without crashing

## The Fix

### Part 1: Remove LockIBase (Fixed Window Lockup)

1. **Removed `ULONG lockState;` variable** - No longer needed

2. **Removed `LockIBase()` and `UnlockIBase()` calls** - These were causing the lockup

3. **Added explicit window refresh** - After moving/resizing, the code now:
   - Calls `RefreshWindowFrame(win)` to redraw the window frame and contents
   - Adds a small delay (`Delay(1)`) to allow Intuition to complete the refresh

### Code Changes

**Before:**
```c
/* Lock Intuition before manipulating window */
lockState = LockIBase(0);

/* Move window if position changed */
if (deltaX != 0 || deltaY != 0)
{
    MoveWindow(win, deltaX, deltaY);
}

/* Resize window if dimensions changed */
if (deltaWidth != 0 || deltaHeight != 0)
{
    SizeWindow(win, deltaWidth, deltaHeight);
}

/* Unlock Intuition */
UnlockIBase(lockState);
```

**After:**
```c
/* 
 * NOTE: MoveWindow() and SizeWindow() are designed to be called WITHOUT locking Intuition.
 * LockIBase() was causing windows to become unresponsive and preventing proper refresh.
 * These functions handle their own internal synchronization safely.
 */

/* Move window if position changed */
if (deltaX != 0 || deltaY != 0)
{
    MoveWindow(win, deltaX, deltaY);
    log_debug(LOG_GENERAL, "  Moved window by (%d, %d)\n", deltaX, deltaY);
}

/* Resize window if dimensions changed */
if (deltaWidth != 0 || deltaHeight != 0)
{
    SizeWindow(win, deltaWidth, deltaHeight);
    log_debug(LOG_GENERAL, "  Resized window by (%d, %d)\n", deltaWidth, deltaHeight);
}

/* 
 * After moving/resizing, refresh the window to ensure proper display.
 * This prevents display corruption when other windows overlap.
 */
if (deltaX != 0 || deltaY != 0 || deltaWidth != 0 || deltaHeight != 0)
{
    /* Request a complete window refresh including frame and contents */
    RefreshWindowFrame(win);
    
    /* Small delay to allow Intuition to complete the refresh */
    Delay(1);  /* 1/50th second delay */
}
```

### Part 2: Use Fresh Window Lookups (Fixed Window Movement)

After Part 1, the system was stable but windows weren't actually moving. Investigation showed **window pointers were stale**.

#### Problem: Cached Pointers Become Invalid

**Original Approach (window_management.c):**
```c
/* Try to find matching window in cached tracker */
for (int i = 0; i < windowTracker->windowCount && matchedCount < MAX_WINDOW_MATCHES; i++) {
    if (windowTracker->windows[i].window != NULL) {
        if (strcmp(windowTracker->windows[i].title, drawer->name) == 0) {
            /* Use cached window pointer from start of run */
            struct Window *win = windowTracker->windows[i].window;
            /* ... move window ... */
        }
    }
}
```

**Why This Failed:**
- `windowTracker` was built at the **start** of processing (in `layout_processor.c`)
- Several seconds/minutes could pass while processing icons
- Workbench could close/reopen windows during that time
- By the time we tried to move windows, pointers pointed to invalid memory
- Log showed garbage values: `(16384, -17440) size 0x0`

#### Solution: Fresh Window Lookup

**New Helper Function (window_enumerator.c):**
```c
struct Window *FindWindowByTitle(const char *title) {
    FolderWindowTracker tracker = {0};
    struct Window *result = NULL;
    
    /* Build fresh list of currently open folder windows */
    if (BuildFolderWindowList(&tracker)) {
        for (int i = 0; i < tracker.windowCount; i++) {
            if (strcmp(tracker.windows[i].title, title) == 0) {
                result = tracker.windows[i].window;
                break;
            }
        }
        FreeFolderWindowList(&tracker);  /* Clean up */
    }
    
    return result;
}
```

**Updated window_management.c:**
```c
/* Get fresh window pointer by title - don't use cached tracker */
struct Window *win = FindWindowByTitle(drawer->name);
if (win != NULL) {
    /* Window exists NOW, pointer is valid */
    ApplyWindowGeometry(win, targetX, targetY, targetWidth, targetHeight);
} else {
    log_debug(LOG_LAYOUT, "Window '%s' not currently open\n", drawer->name);
}
```

**Why This Works:**
1. **Fresh snapshot** - Calls `BuildFolderWindowList()` right when we need the window
2. **Current state** - Gets windows that are actually open NOW, not minutes ago
3. **Proper cleanup** - Calls `FreeFolderWindowList()` to avoid memory leaks
4. **Reuses safe code** - `BuildFolderWindowList()` already filters folder windows correctly

**Performance Note:** Adds small delay (rebuilding window list for each drawer), but ensures correctness. For testing phase, correctness > speed.

## Testing

After both fixes:
- ✅ System remains stable (no lockups or alerts)
- ✅ Windows are actually moved to saved positions
- ✅ User can interact with windows immediately after move
- ✅ Display corruption eliminated
- ✅ Window pointers are always current and valid

## Related Files

- `src/GUI/window_enumerator.c` - Contains fixed `ApplyWindowGeometry()` and new `FindWindowByTitle()` functions
- `src/window_management.c` - Updated to use `FindWindowByTitle()` instead of cached tracker
- `include/window_enumerator.h` - Added `FindWindowByTitle()` function prototype

## Lessons Learned

1. **Don't lock Intuition for window operations** - `LockIBase()` should ONLY be used for reading Intuition's data structures, never for operations
2. **Window pointers can become stale** - In long-running operations, always get fresh window pointers rather than caching them
3. **Workbench is dynamic** - Windows can be closed/reopened at any time by the system during icon operations
4. **Use existing safe functions** - `BuildFolderWindowList()` already handles window enumeration correctly with proper filtering

## References

- Amiga ROM Kernel Reference Manual: Libraries (Intuition chapter)
- `MoveWindow()` - Safe to call without locking
- `SizeWindow()` - Safe to call without locking  
- `LockIBase()` - Should only be used for reading internal structures
- `RefreshWindowFrame()` - Forces window redraw after geometric changes

## Date

November 7, 2025

## Author

AI Assistant (GitHub Copilot)
