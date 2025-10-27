# Memory Tracking Quick Start Guide

## Overview

The iTidy logging system now supports **multiple log categories** with **log levels** and **timestamped files**. Memory tracking writes to a dedicated `memory_YYYY-MM-DD_HH-MM-SS.log` file, and all errors are automatically copied to `errors_YYYY-MM-DD_HH-MM-SS.log`.

## Step 1: Enable Memory Tracking

Open `include\platform\platform.h` and uncomment line 27:

```c
/* Enable memory debugging by uncommenting this line */
#define DEBUG_MEMORY_TRACKING
```

## Step 2: Update main_gui.c

Replace the old `initialize_logfile()` call with the new system:

```c
#include <platform/platform.h>

int main(int argc, char **argv) {
    // Initialize logging system FIRST (TRUE = clean old logs)
    initialize_log_system(TRUE);
    
    // Initialize memory tracking
    whd_memory_init();
    
    // ... rest of your program initialization ...
    
    // ... your program code ...
    
    // Before cleanup, report memory status
    whd_memory_report();
    
    // ... existing cleanup code ...
    FreeIconErrorList(&iconsErrorTracker);
    
    // Shutdown logging (closes all log files)
    shutdown_log_system();
    
    return returnCode;
}
```

## Step 3: Build

```powershell
make clean
make
```

## Step 4: Run and Check Results

1. Run your program: `.\Bin\Amiga\iTidy`
2. Check the logs directory: `.\Bin\Amiga\logs\`
3. You'll find timestamped logs:
   - `general_2025-10-27_14-23-45.log` - General program flow
   - `memory_2025-10-27_14-23-45.log` - All memory operations
   - `errors_2025-10-27_14-23-45.log` - All errors from all categories
   - `gui_2025-10-27_14-23-45.log` - GUI events (if you log them)
   - `icons_2025-10-27_14-23-45.log` - Icon processing (if you log them)
   - `backup_2025-10-27_14-23-45.log` - Backup operations (if you log them)

## What You'll See

### In memory_YYYY-MM-DD_HH-MM-SS.log:

```
[14:23:45][INFO] Memory tracking initialized
[14:23:45][DEBUG] ALLOC: 256 bytes at 0x12345678 (icon_management.c:56) [Current: 256, Peak: 256]
[14:23:46][DEBUG] FREE: 256 bytes at 0x12345678 (allocated icon_management.c:56, freed icon_management.c:120) [Current: 0]
[14:23:50][INFO] 
========== MEMORY TRACKING REPORT ==========
Total allocations: 45 (12,584 bytes)
Total frees: 45 (12,584 bytes)
Peak memory usage: 5,432 bytes (5 KB)
Current memory: 0 bytes

*** NO MEMORY LEAKS DETECTED - ALL ALLOCATIONS FREED ***
============================================
```

### If Leaks Detected:

Memory leaks are logged as **errors**, so they appear in BOTH logs:
- `memory_YYYY-MM-DD_HH-MM-SS.log` (with full details)
- `errors_YYYY-MM-DD_HH-MM-SS.log` (automatically copied)

```
[14:23:50][ERROR] 
*** MEMORY LEAKS DETECTED: 2 blocks, 256 bytes ***

Leak details:
  - 128 bytes at 0x12345678 (allocated at icon_management.c:515)
  - 128 bytes at 0x12345ABC (allocated at icon_types.c:36)

*** CHECK error.log FOR COMPLETE LEAK REPORT ***
```

## Step 5: Using Other Log Categories

Throughout your code, you can now use category-specific logging:

```c
// In your source files
#include "writeLog.h"

// General program flow
log_info(LOG_GENERAL, "Processing directory: %s\n", path);

// GUI events
log_debug(LOG_GUI, "Button clicked: %s\n", buttonName);
log_warning(LOG_GUI, "Window resize too small: %dx%d\n", w, h);

// Icon processing
log_info(LOG_ICONS, "Found %d icons in %s\n", count, folder);
log_error(LOG_ICONS, "Failed to load icon: %s\n", iconPath);

// Backup operations
log_info(LOG_BACKUP, "Starting backup of %s\n", path);
log_error(LOG_BACKUP, "Backup failed: %s\n", reason);

// Direct error logging (goes to errors.log too)
log_error(LOG_GENERAL, "Critical failure: %s\n", message);
```

## Step 6: Runtime Control

You can enable/disable categories at runtime:

```c
// Disable verbose memory logging after initialization
enable_log_category(LOG_MEMORY, FALSE);

// Only log warnings and errors for icons
set_log_level(LOG_ICONS, LOG_LEVEL_WARNING);

// Check if category is enabled
if (is_log_category_enabled(LOG_GUI)) {
    // Do expensive logging preparation
    log_debug(LOG_GUI, "Complex data: %s\n", prepare_data());
}
```

## Step 7: Disable for Release

When done debugging, comment out the define:

```c
/* Enable memory debugging by uncommenting this line */
/* #define DEBUG_MEMORY_TRACKING */
```

This removes all tracking overhead. The logging system still works, but memory tracking is disabled.

## Log Levels

- **DEBUG**: Verbose details (every alloc/free)
- **INFO**: Normal informational messages
- **WARNING**: Something unusual but not fatal
- **ERROR**: Error conditions (auto-copied to errors.log)

## Benefits

✅ **Separate logs** - Easy to find what you're looking for
✅ **Timestamped** - Compare runs, track history
✅ **Error consolidation** - All errors in one place
✅ **Runtime control** - Enable/disable categories on the fly
✅ **Type safety** - Enum prevents typos in category names
✅ **Immediate writes** - Memory logs survive crashes
✅ **Backward compatible** - Old `append_to_log()` still works

## Notes

- The system creates `PROGDIR:logs/` directory automatically
- Old logs are deleted on startup (if you pass `TRUE` to `initialize_log_system()`)
- Each program run gets a new set of timestamped logs
- Memory tracking writes immediately (no buffering) to survive crashes
- The Makefile is already configured to build platform.c when tracking is enabled

