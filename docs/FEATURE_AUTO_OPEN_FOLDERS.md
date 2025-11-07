# Auto-Open Folders Feature

## Overview
The auto-open folders feature is an experimental developmental tool that automatically opens Workbench folder windows after iTidy processes them. This provides real-time visual feedback during iTidy runs, allowing developers and users to see the icon arrangement results immediately.

## Status
**Experimental** - Disabled by default

## Purpose
- **Development**: Visually verify iTidy's icon arrangements during testing
- **User Feedback**: Show progress in real-time during large recursive operations  
- **Debugging**: Quickly identify issues with icon positioning or window sizing

## Implementation

### Global Flag
```c
BOOL user_openFoldersWithIcons = FALSE;  /* Default: disabled */
```

**Declared in:** `src/itidy_types.h`  
**Defined in:** `src/main_gui.c`

### How It Works
1. User enables the flag before calling `ProcessDirectory()`
2. For each folder processed:
   - iTidy arranges icons via `FormatIconsAndWindow()`
   - If flag is enabled, calls `OpenWorkbenchObjectA(path, NULL)`
   - Workbench opens a window showing the tidied folder
3. Processing continues to next folder

### Code Location
**File:** `src/file_directory_handling.c`  
**Function:** `ProcessDirectory()`  
**Location:** After `FormatIconsAndWindow()` call, before subdirectory recursion

### Usage Example
```c
/* Enable auto-open before processing */
user_openFoldersWithIcons = TRUE;

/* Process directories - folders will auto-open */
ProcessDirectory("Work:Projects", TRUE, 0);

/* Disable after processing */
user_openFoldersWithIcons = FALSE;
```

## Technical Details

### Workbench Library
- Uses `OpenWorkbenchObjectA()` from workbench.library
- Library is **auto-opened** by `#include <proto/wb.h>`
- No manual `OpenLibrary()` or `CloseLibrary()` required

### Function Signature
```c
BOOL OpenWorkbenchObjectA(CONST_STRPTR name, CONST struct TagItem *tags)
```

**Parameters:**
- `name` - Full path to folder (e.g., "Work:Projects/MyFolder")
- `tags` - TagList for options (NULL for default behavior)

**Returns:**
- `TRUE` - Window opened successfully
- `FALSE` - Failed to open (non-fatal, processing continues)

### Error Handling
- Failures are logged to debug output
- Processing continues even if window open fails
- Non-fatal - won't halt recursive operations

## Use Cases

### 1. Development Testing
```c
/* Test icon layout on specific folder */
user_openFoldersWithIcons = TRUE;
ProcessDirectory("Test:Layouts/Sample", FALSE, 0);
user_openFoldersWithIcons = FALSE;
```

### 2. Recursive Verification
```c
/* Verify recursive operation visually */
user_openFoldersWithIcons = TRUE;
ProcessDirectory("Work:Projects", TRUE, 0);  /* Opens all processed folders */
user_openFoldersWithIcons = FALSE;
```

### 3. Demonstration Mode
```c
/* Show iTidy's capabilities to users */
user_openFoldersWithIcons = TRUE;
ProcessDirectory("Demo:Examples", TRUE, 0);
user_openFoldersWithIcons = FALSE;
```

## Cautions and Limitations

### Performance Impact
- **Memory**: Each open window consumes Workbench memory
- **Speed**: Opening windows adds overhead to processing time
- **Screen Clutter**: Many folders = many open windows

### Recommended Usage
- ✅ Single folder testing
- ✅ Small recursive operations (<10 folders)
- ✅ Development and debugging
- ❌ Large batch processing (100+ folders)
- ❌ Production automated scripts
- ❌ Low-memory systems (<4MB Chip RAM)

### System Requirements
- **Workbench**: 2.0+ (OpenWorkbenchObjectA available from WB 2.0)
- **Memory**: Adequate free Chip RAM for multiple windows
- **Screen Space**: Sufficient Workbench screen size to display windows

## Future Enhancements

### Planned Features
1. **GUI Checkbox**: "Open folders during processing"
2. **CLI Flag**: `-openFolders` command-line argument
3. **Delay Option**: Stagger window opening with configurable delay
4. **Max Windows**: Limit maximum simultaneously open windows
5. **Close After**: Auto-close windows after user acknowledgment

### Potential Improvements
- Add progress indicator showing X of Y folders opened
- Support for custom window positioning/sizing via tags
- Batch mode: Open only first and last folder
- Integration with progress window system (Phase 2/3)

## Examples

### Enable via GUI (Future)
```c
/* In GUI event handler for checkbox */
if (checkbox_state == CHECKED) {
    user_openFoldersWithIcons = TRUE;
} else {
    user_openFoldersWithIcons = FALSE;
}
```

### Enable via CLI (Future)
```bash
# Command-line usage (not yet implemented)
iTidy Work:Projects -subdirs -openFolders
```

## Files Modified

| File | Purpose | Changes |
|------|---------|---------|
| `src/itidy_types.h` | Global declarations | Added `extern BOOL user_openFoldersWithIcons` |
| `src/main_gui.c` | Variable definitions | Added `BOOL user_openFoldersWithIcons = FALSE` |
| `src/file_directory_handling.c` | Core functionality | Added `#include <proto/wb.h>` and `OpenWorkbenchObjectA()` call |

## Testing

### Test Case 1: Single Folder
```c
user_openFoldersWithIcons = TRUE;
ProcessDirectory("Test:SingleFolder", FALSE, 0);
/* Expected: One window opens showing tidied icons */
user_openFoldersWithIcons = FALSE;
```

### Test Case 2: Recursive (3 folders)
```c
user_openFoldersWithIcons = TRUE;
ProcessDirectory("Test:ThreeFolders", TRUE, 0);
/* Expected: Three windows open sequentially */
user_openFoldersWithIcons = FALSE;
```

### Test Case 3: Disabled (Default)
```c
/* user_openFoldersWithIcons remains FALSE */
ProcessDirectory("Test:AnyFolder", TRUE, 0);
/* Expected: No windows open, normal iTidy operation */
```

## Debugging

### Enable Debug Logging
```c
#define DEBUG  /* Uncomment in compilation */
```

**Log Output:**
```
Opening folder window via Workbench: Work:Projects/MyFolder
Warning: Failed to open folder window for: Work:BadPath
```

### Common Issues

**Issue**: No windows open  
**Solution**: Check that `user_openFoldersWithIcons = TRUE` is set

**Issue**: "Failed to open folder window"  
**Cause**: Invalid path or folder doesn't exist  
**Solution**: Verify folder path is valid and accessible

**Issue**: Too many windows, system slows down  
**Solution**: Disable feature or process fewer folders at once

## Conclusion

The auto-open folders feature is a powerful developmental tool for visual verification of iTidy's operations. While experimental, it provides immediate feedback that can accelerate testing and debugging workflows. Use selectively and consider system resources when enabling this feature.

**Default State**: Disabled  
**Recommended Use**: Development and testing only  
**Production Ready**: No (experimental status)
