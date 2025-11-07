# Auto-Open Folders - Quick Reference

## Quick Enable/Disable

### Enable Before Processing
```c
user_openFoldersWithIcons = TRUE;
ProcessDirectory(path, TRUE, 0);
```

### Disable After Processing
```c
user_openFoldersWithIcons = FALSE;
```

## One-Liner Usage
```c
/* Enable, process, disable */
user_openFoldersWithIcons = TRUE;
ProcessDirectory("Work:Projects", TRUE, 0);
user_openFoldersWithIcons = FALSE;
```

## What It Does
- ✅ Opens Workbench folder windows automatically after iTidy tidies icons
- ✅ Provides real-time visual feedback during processing
- ✅ Non-fatal errors - continues processing even if window open fails

## Requirements
- Workbench 2.0+ 
- workbench.library (auto-opened by proto/wb.h)
- Adequate free memory for multiple windows

## Cautions
- ⚠️ Experimental feature - use for development/testing only
- ⚠️ May clutter screen with many open windows
- ⚠️ Adds processing overhead
- ⚠️ Not recommended for large batch operations (100+ folders)

## Files Involved
- `src/itidy_types.h` - Declaration
- `src/main_gui.c` - Definition (default: FALSE)
- `src/file_directory_handling.c` - Implementation

## Default State
**Disabled** - Must be explicitly enabled by setting flag to TRUE

## Debug Logging
```
Opening folder window via Workbench: Work:Projects/MyFolder
Warning: Failed to open folder window for: Work:BadPath
```

## See Also
- `docs/FEATURE_AUTO_OPEN_FOLDERS.md` - Complete documentation
- `docs/DEVELOPMENT_LOG.md` - Development history
