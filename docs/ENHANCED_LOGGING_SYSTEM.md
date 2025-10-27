# Enhanced Multi-Category Logging System

## Overview

iTidy now features a powerful, flexible logging system with:

- ✅ **Multiple log categories** (general, memory, GUI, icons, backup, errors)
- ✅ **Log levels** (DEBUG, INFO, WARNING, ERROR)
- ✅ **Timestamped log files** (one set per program run)
- ✅ **Runtime enable/disable** per category
- ✅ **Automatic error duplication** to errors.log
- ✅ **Type-safe category enum** prevents typos
- ✅ **Backward compatible** with existing code

All logs are stored in `PROGDIR:logs/` with timestamps like:
```
PROGDIR:logs/
  ├── general_2025-10-27_14-23-45.log
  ├── memory_2025-10-27_14-23-45.log
  ├── gui_2025-10-27_14-23-45.log
  ├── icons_2025-10-27_14-23-45.log
  ├── backup_2025-10-27_14-23-45.log
  └── errors_2025-10-27_14-23-45.log
```

---

## Quick Start

### 1. Initialize the System

In your `main()` function:

```c
#include "writeLog.h"

int main(int argc, char **argv) {
    // Initialize logging (TRUE = clean old logs)
    initialize_log_system(TRUE);
    
    // Your program code...
    
    // Shutdown logging before exit
    shutdown_log_system();
    
    return 0;
}
```

### 2. Use Category-Specific Logging

```c
#include "writeLog.h"

// Log with level and category
log_debug(LOG_GENERAL, "Debugging info: %d\n", value);
log_info(LOG_GENERAL, "Program started\n");
log_warning(LOG_GUI, "Window size too small\n");
log_error(LOG_ICONS, "Failed to load: %s\n", path);

// Or use the generic function
log_message(LOG_BACKUP, LOG_LEVEL_INFO, "Backup complete\n");
```

---

## Log Categories

Defined in `writeLog.h` as type-safe enum:

```c
typedef enum {
    LOG_GENERAL,    /* General program flow */
    LOG_MEMORY,     /* Memory allocations/frees */
    LOG_GUI,        /* GUI events and interactions */
    LOG_ICONS,      /* Icon processing and management */
    LOG_BACKUP,     /* Backup system operations */
    LOG_ERRORS      /* Errors only (auto-populated) */
} LogCategory;
```

### Category Usage Guidelines

| Category | Use For | Examples |
|----------|---------|----------|
| `LOG_GENERAL` | Program flow, initialization, shutdown | "Program started", "Processing complete" |
| `LOG_MEMORY` | Memory allocations/frees (auto-used by tracking) | "Allocated 256 bytes", "Memory leak detected" |
| `LOG_GUI` | User interactions, window events | "Button clicked", "Window resized" |
| `LOG_ICONS` | Icon loading, processing, arrangement | "Found 10 icons", "Icon load failed" |
| `LOG_BACKUP` | Backup/restore operations | "Backup started", "Restore complete" |
| `LOG_ERRORS` | **Auto-populated** from ERROR level logs | Don't log directly, errors auto-copied here |

---

## Log Levels

```c
typedef enum {
    LOG_LEVEL_DEBUG,    /* Verbose debugging information */
    LOG_LEVEL_INFO,     /* Normal informational messages */
    LOG_LEVEL_WARNING,  /* Warning conditions */
    LOG_LEVEL_ERROR     /* Error conditions */
} LogLevel;
```

### Convenience Macros

```c
log_debug(category, format, ...);   // Verbose details
log_info(category, format, ...);    // Normal messages
log_warning(category, format, ...); // Warnings
log_error(category, format, ...);   // Errors (auto-copied to errors.log)
```

### When to Use Each Level

| Level | Use When | Example |
|-------|----------|---------|
| **DEBUG** | Verbose details, every operation | "Processing icon 5 of 20", "Entering function foo()" |
| **INFO** | Normal program flow | "Processing directory", "Backup complete" |
| **WARNING** | Something unusual but not fatal | "Icon format unusual", "Slow operation detected" |
| **ERROR** | Operation failed, data corrupt | "File not found", "Memory allocation failed" |

---

## Runtime Control

### Enable/Disable Categories

```c
// Disable memory logging after initialization
enable_log_category(LOG_MEMORY, FALSE);

// Re-enable later
enable_log_category(LOG_MEMORY, TRUE);

// Check if enabled
if (is_log_category_enabled(LOG_GUI)) {
    // Do expensive logging work only if enabled
}
```

### Set Minimum Log Level

Filter out low-priority messages:

```c
// Only log warnings and errors for icons
set_log_level(LOG_ICONS, LOG_LEVEL_WARNING);

// Now these are filtered:
log_debug(LOG_ICONS, "..."); // Not logged
log_info(LOG_ICONS, "...");  // Not logged

// But these are logged:
log_warning(LOG_ICONS, "..."); // Logged
log_error(LOG_ICONS, "...");   // Logged
```

---

## Log File Format

Each log entry has this format:

```
[HH:MM:SS][LEVEL] message
```

Example from `memory_2025-10-27_14-23-45.log`:

```
[14:23:45][INFO] Memory tracking initialized
[14:23:45][DEBUG] ALLOC: 256 bytes at 0x12345678 (icon_management.c:56) [Current: 256, Peak: 256]
[14:23:46][DEBUG] FREE: 256 bytes at 0x12345678 (allocated icon_management.c:56, freed icon_management.c:120) [Current: 0]
[14:23:50][ERROR] MEMORY LEAKS DETECTED: 1 block, 128 bytes
[14:23:50][ERROR]   - 128 bytes at 0x12345ABC (allocated at icon_types.c:221)
```

Example from `errors_2025-10-27_14-23-45.log`:

```
[14:23:50][ERROR][memory] MEMORY LEAKS DETECTED: 1 block, 128 bytes
[14:23:50][ERROR][memory]   - 128 bytes at 0x12345ABC (allocated at icon_types.c:221)
[14:24:12][ERROR][icons] Failed to load icon: DH0:Icons/MyIcon.info
[14:24:15][ERROR][backup] Backup catalog write failed
```

---

## Error Log Auto-Population

**All `log_error()` calls are automatically duplicated to `errors.log`**, regardless of category:

```c
// In icon_management.c
log_error(LOG_ICONS, "Failed to load: %s\n", iconPath);

// In backup_catalog.c  
log_error(LOG_BACKUP, "Catalog corrupt\n");

// In platform.c (memory tracking)
log_error(LOG_MEMORY, "Memory leak detected\n");
```

All these errors appear in:
1. Their category-specific log (`icons_*.log`, `backup_*.log`, `memory_*.log`)
2. **AND** in `errors_*.log` with category prefix `[ERROR][category]`

This gives you one place to see **all errors across all systems**.

---

## Practical Examples

### Example 1: Conditional Verbose Logging

```c
void process_icons(IconArray *icons) {
    log_info(LOG_ICONS, "Processing %d icons\n", icons->size);
    
    // Enable verbose logging for debugging
    set_log_level(LOG_ICONS, LOG_LEVEL_DEBUG);
    
    for (int i = 0; i < icons->size; i++) {
        log_debug(LOG_ICONS, "Icon %d: %s (%dx%d)\n", 
                 i, icons->array[i].icon_text,
                 icons->array[i].icon_width,
                 icons->array[i].icon_height);
    }
    
    // Restore normal logging
    set_log_level(LOG_ICONS, LOG_LEVEL_INFO);
    
    log_info(LOG_ICONS, "Processing complete\n");
}
```

### Example 2: Disabling Noisy Logs

```c
int main(int argc, char **argv) {
    initialize_log_system(TRUE);
    
    // Memory tracking is very verbose - only enable when needed
    enable_log_category(LOG_MEMORY, FALSE);
    
    // During leak investigation, enable it
    #ifdef DEBUG_MEMORY_TRACKING
        enable_log_category(LOG_MEMORY, TRUE);
    #endif
    
    // Your program...
    
    shutdown_log_system();
    return 0;
}
```

### Example 3: Error Handling with Logging

```c
BOOL load_icon(const char *path) {
    DiskObject *dobj;
    
    log_debug(LOG_ICONS, "Loading icon: %s\n", path);
    
    dobj = GetDiskObject(path);
    if (!dobj) {
        log_error(LOG_ICONS, "Failed to load icon: %s (IoErr: %ld)\n", 
                 path, IoErr());
        return FALSE;  // Error logged to both icons.log and errors.log
    }
    
    log_info(LOG_ICONS, "Successfully loaded: %s\n", path);
    FreeDiskObject(dobj);
    return TRUE;
}
```

---

## Migration from Old System

### Old Code (single log):

```c
initialize_logfile();
append_to_log("Program started\n");
append_to_log("Processing: %s\n", path);
delete_logfile();
```

### New Code (multi-category):

```c
initialize_log_system(TRUE);
log_info(LOG_GENERAL, "Program started\n");
log_info(LOG_GENERAL, "Processing: %s\n", path);
shutdown_log_system();
```

### Backward Compatibility

Old code still works! `append_to_log()` maps to `LOG_GENERAL` with `INFO` level:

```c
append_to_log("Still works\n"); // Goes to general_*.log
```

---

## Performance Considerations

### Memory Tracking (High Frequency)

- Memory logs write **immediately** (no buffering)
- This ensures logs survive crashes
- Each alloc/free = one disk write
- **Disable in production** for performance

### Other Categories (Normal Frequency)

- Files opened/closed for each log message
- Acceptable overhead for normal logging
- Consider disabling DEBUG level in production

### Best Practices

1. Use DEBUG level liberally during development
2. Use INFO for normal program flow
3. Use WARNING for unusual but handled cases
4. Use ERROR for actual failures
5. Disable DEBUG level or entire categories in release builds

---

## Advanced Usage

### Custom Category Control

```c
typedef struct {
    BOOL enableMemory;
    BOOL enableGUI;
    LogLevel iconLevel;
} LogConfig;

void configure_logging(LogConfig *config) {
    enable_log_category(LOG_MEMORY, config->enableMemory);
    enable_log_category(LOG_GUI, config->enableGUI);
    set_log_level(LOG_ICONS, config->iconLevel);
}

// Usage
LogConfig config = {
    .enableMemory = FALSE,  // Too verbose normally
    .enableGUI = TRUE,      // Track user interactions
    .iconLevel = LOG_LEVEL_WARNING  // Only problems
};

configure_logging(&config);
```

### Selective Logging Blocks

```c
void expensive_debug_operation(void) {
    if (!is_log_category_enabled(LOG_GUI)) {
        return;  // Skip expensive work if logging disabled
    }
    
    // Gather data (slow)
    char *debug_info = collect_debug_info();
    log_debug(LOG_GUI, "Debug info: %s\n", debug_info);
    free(debug_info);
}
```

---

## Troubleshooting

### Issue: No logs created

**Solution:** Check that `initialize_log_system()` is called before any logging.

### Issue: logs/ directory not created

**Solution:** Ensure write permissions in PROGDIR:. System falls back to PROGDIR: if directory creation fails.

### Issue: Memory log is empty

**Solution:** Check that `DEBUG_MEMORY_TRACKING` is defined and `whd_memory_init()` is called.

### Issue: Too much logging, logs are huge

**Solution:** 
- Disable DEBUG level: `set_log_level(category, LOG_LEVEL_INFO)`
- Disable entire categories: `enable_log_category(LOG_MEMORY, FALSE)`
- Only enable verbose logging when debugging specific issues

### Issue: Errors not appearing in errors.log

**Solution:** Make sure you're using `log_error()`, not `log_info()` or `log_debug()`.

---

## API Reference

### Initialization

```c
void initialize_log_system(BOOL cleanOldLogs);
```
- Creates `PROGDIR:logs/` directory
- Generates timestamp for this run's logs
- If `cleanOldLogs` is TRUE, deletes old log files
- Must be called before any logging

```c
void shutdown_log_system(void);
```
- Writes final statistics
- Closes all open log files
- Call before program exit

### Logging Functions

```c
void log_message(LogCategory category, LogLevel level, const char *format, ...);
```
- Main logging function
- Checks if category enabled and level sufficient
- Writes to category-specific log
- Auto-copies ERROR to errors.log

```c
void log_debug(LogCategory category, const char *format, ...);
void log_info(LogCategory category, const char *format, ...);
void log_warning(LogCategory category, const char *format, ...);
void log_error(LogCategory category, const char *format, ...);
```
- Convenience macros for common levels

### Category Control

```c
void enable_log_category(LogCategory category, BOOL enable);
BOOL is_log_category_enabled(LogCategory category);
void set_log_level(LogCategory category, LogLevel minLevel);
```

### Utility Functions

```c
const char* get_log_category_name(LogCategory category);
const char* get_log_level_name(LogLevel level);
```

### Legacy Compatibility

```c
void append_to_log(const char *format, ...);
void initialize_logfile(void);
void delete_logfile(void);
```
- Kept for backward compatibility
- Map to new system

---

## Summary

The enhanced logging system provides:

1. **Organization** - Separate logs by concern
2. **Clarity** - Timestamped runs, log levels
3. **Control** - Enable/disable at runtime
4. **Error focus** - All errors in one place
5. **Performance** - Disable verbose logging easily
6. **Safety** - Type-safe enums prevent typos
7. **History** - Timestamped logs for comparison
8. **Compatibility** - Works with existing code

Use it to make debugging easier and track down issues faster!
