# EasyRequest Helper - Quick Reference

## Basic Usage

```c
#include "easy_request_helper.h"

// Simple confirmation
if (ShowEasyRequest(window, "Confirm", "Delete this file?", "Yes|No"))
{
    // User clicked "Yes"
}

// Information message (single button)
ShowEasyRequest(window, "Info", "Operation complete!", "OK");

// Error message with details
char error[256];
sprintf(error, "Failed to open: %s\nError: %ld", filename, IoErr());
ShowEasyRequest(window, "Error", error, "OK");
```

## Function Signature

```c
BOOL ShowEasyRequest(struct Window *parentWin,
                     CONST_STRPTR title,
                     CONST_STRPTR body,
                     CONST_STRPTR gadgets);
```

## Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `parentWin` | `struct Window *` | Parent window (requester opens on same screen) |
| `title` | `CONST_STRPTR` | Title bar text (can be NULL for default "iTidy") |
| `body` | `CONST_STRPTR` | Message text (supports `\n` for multiple lines) |
| `gadgets` | `CONST_STRPTR` | Gadget labels separated by `|` (e.g., "OK\|Cancel") |

## Return Value

- **TRUE**: User clicked first (leftmost) gadget
- **FALSE**: User clicked any other gadget, pressed ESC, or error occurred

## Common Patterns

### 1. Yes/No Question
```c
if (ShowEasyRequest(window, "Confirm", "Save changes?", "Yes|No"))
    save_document();
```

### 2. OK/Cancel Operation
```c
if (!ShowEasyRequest(window, "Warning", "This will delete everything!", "OK|Cancel"))
    return; // User cancelled
perform_delete();
```

### 3. Simple Alert (single button)
```c
ShowEasyRequest(window, "Notice", "Task completed successfully.", "OK");
```

### 4. Multi-line Message
```c
char msg[512];
sprintf(msg, "Restored %lu files\nTotal size: %lu KB\nErrors: %lu", 
        fileCount, totalSize, errorCount);
ShowEasyRequest(window, "Summary", msg, "OK");
```

### 5. Three Options
```c
// First gadget = leftmost
if (ShowEasyRequest(window, "Choose", "Select action:", "Save|Discard|Cancel"))
{
    // User clicked "Save" (first gadget)
    save_file();
}
// User clicked "Discard" or "Cancel" (any other gadget)
```

## Benefits vs. Direct EasyRequest()

### Before (Direct EasyRequest):
```c
struct EasyStruct es;
es.es_StructSize = sizeof(struct EasyStruct);
es.es_Flags = 0;
es.es_Title = "Confirm";
es.es_TextFormat = "Delete file?";
es.es_GadgetFormat = "Delete|Cancel";
if (EasyRequest(window, &es, NULL))
    delete_file();
```

### After (Using Helper):
```c
if (ShowEasyRequest(window, "Confirm", "Delete file?", "Delete|Cancel"))
    delete_file();
```

**Advantages:**
- ✅ 1 line instead of 7
- ✅ Guaranteed correct screen placement (RTG fix)
- ✅ Automatic null-checking
- ✅ Debug logging built-in
- ✅ Cleaner, more maintainable code

## Error Handling

The helper returns FALSE if:
- `parentWin` is NULL
- `parentWin->WScreen` is NULL
- `body` or `gadgets` is NULL
- User cancels or clicks non-first gadget

Always check for NULL window before calling:
```c
if (window != NULL)
{
    ShowEasyRequest(window, "Error", "Something went wrong", "OK");
}
```

## Compile-Time Options

### Standard Mode (Default)
```makefile
# No special flags needed
make
```
Uses standard `EasyRequest()` - works on WB 2.0+

### Experimental Centered Mode
```makefile
# Add to CFLAGS in Makefile
CFLAGS += -DBUILD_WITH_MOVEWINDOW
make
```
Uses `BuildEasyRequest()` + `MoveWindow()` - requires WB 3.0+, centers requester on screen

## Debug Logging

All calls are automatically logged when DEBUG is defined:
```
ShowEasyRequest: Opening requester
  Title: Confirm Delete
  Body: Delete backup run Run_0007?...
  Gadgets: Delete|Cancel
  Parent window: 0x080A4C80, screen: 0x080A3F00
ShowEasyRequest: Using standard EasyRequest()
ShowEasyRequest: Returning 1
```

Check `iTidy.log` for requester debugging information.

## Tips & Best Practices

### ✅ DO:
- Always pass a valid window pointer
- Keep messages concise and clear
- Use first gadget for primary action
- Use `\n` for multi-line messages
- Check return value for yes/no questions

### ❌ DON'T:
- Pass NULL window (will return FALSE)
- Make messages too long (use scrolling text gadget instead)
- Rely on specific gadget numbering (only first vs. others)
- Forget to include header file

## Integration Example

```c
#include <intuition/intuition.h>
#include "easy_request_helper.h"

void my_function(struct Window *window)
{
    char message[256];
    
    // Confirm action
    sprintf(message, "Process %ld files?", fileCount);
    if (!ShowEasyRequest(window, "Confirm", message, "Process|Cancel"))
        return;
    
    // Perform action
    process_files();
    
    // Show result
    sprintf(message, "Processed %ld files\n%ld succeeded\n%ld failed",
            fileCount, successCount, failCount);
    ShowEasyRequest(window, "Complete", message, "OK");
}
```

## See Also

- `docs/EASY_REQUEST_HELPER_MODULE.md` - Full implementation details
- `include/easy_request_helper.h` - API documentation
- `src/GUI/easy_request_helper.c` - Source code
- `src/GUI/restore_window.c` - Usage examples
