# Progress Window Analysis for Tool Cache Rebuild

## Summary
**YES**, the `src/GUI/StatusWindows/main_progress_window.c` window **IS suitable** for showing progress for the Tool Cache "Rebuild Cache" operation with some modifications.

## Current State

### Main Window "Apply" Button Flow:
1. Opens `iTidyMainProgressWindow` 
2. Calls `ProcessDirectoryWithPreferencesAndProgress(&progress_window)`
3. This function sets a global pointer `g_progressWindow` 
4. All processing code uses `PROGRESS_STATUS(...)` macro which:
   - Writes to console
   - Appends to progress window ListView
   - Handles events (including cancel checking)
5. After completion, changes button text from "Cancel" to "Close"
6. Keeps window open for user review

### Tool Cache "Rebuild Cache" Button Current Flow:
1. Updates global preferences with folder path from textbox
2. Calls `ScanDirectoryForToolsOnly()` directly
3. **NO progress window integration** - only uses `log_info()`, `log_debug()`, etc.
4. Tool scanning happens "silently" from user perspective

## Key Code Elements

### Progress Window API (main_progress_window.h):
```c
BOOL itidy_main_progress_window_open(struct iTidyMainProgressWindow *window_data);
void itidy_main_progress_window_close(struct iTidyMainProgressWindow *window_data);
BOOL itidy_main_progress_window_handle_events(struct iTidyMainProgressWindow *window_data);
BOOL itidy_main_progress_window_append_status(struct iTidyMainProgressWindow *window_data,
                                              const char *status_text);
void itidy_main_progress_window_set_button_text(struct iTidyMainProgressWindow *window_data,
                                                const char *text);
```

### PROGRESS_STATUS Macro (layout_processor.c):
```c
/* Helper macro for dual output (console + progress window) */
#define PROGRESS_STATUS(...) \
    do { \
        char _buf[256]; \
        snprintf(_buf, sizeof(_buf), __VA_ARGS__); \
        CONSOLE_STATUS("%s\n", _buf); \
        if (g_progressWindow) { \
            itidy_main_progress_window_append_status(g_progressWindow, _buf); \
            itidy_main_progress_window_handle_events(g_progressWindow); \
        } \
    } while (0)

/* Helper macro to check for user cancellation */
#define CHECK_CANCEL() \
    do { \
        if (g_progressWindow && g_progressWindow->cancel_requested) { \
            CONSOLE_STATUS("\n*** User cancelled operation ***\n"); \
            return FALSE; \
        } \
    } while (0)
```

## Required Changes for Tool Cache Integration

### Option 1: Create Wrapper Function (RECOMMENDED)
Create a new function similar to `ProcessDirectoryWithPreferencesAndProgress()`:

```c
BOOL ScanDirectoryForToolsOnlyWithProgress(struct iTidyMainProgressWindow *progress_window)
{
    BOOL result;
    
    if (!progress_window)
    {
        log_error(LOG_GENERAL, "Error: NULL progress window pointer\n");
        return FALSE;
    }
    
    /* Set global progress window pointer */
    g_progressWindow = progress_window;
    
    /* Call the main scanning function */
    result = ScanDirectoryForToolsOnly();
    
    /* Clear global progress window pointer */
    g_progressWindow = NULL;
    
    return result;
}
```

### Option 2: Add Progress Updates Directly
Modify `ScanDirectoryForToolsOnly()`, `ScanSingleDirectoryForTools()`, and `ScanDirectoryRecursiveForTools()` to use `PROGRESS_STATUS()` instead of `log_info()` where appropriate.

**Changes needed:**
```c
// In ScanDirectoryForToolsOnly():
PROGRESS_STATUS("*** Building PATH search list for default tool scanning ***");
PROGRESS_STATUS("Scanning for default tools: %s", sanitizedPath);
PROGRESS_STATUS("Recursive: %s", prefs->recursive_subdirs ? "Yes" : "No");
PROGRESS_STATUS("Mode: SCAN TOOLS ONLY (no tidying)");

// In ScanSingleDirectoryForTools():
PROGRESS_STATUS("  Scanning: %s", path);
PROGRESS_STATUS("  Found %lu icons (tools validated)", (unsigned long)iconArray->size);

// In ScanDirectoryRecursiveForTools():
PROGRESS_STATUS("Entering: %s", subdir);
```

### Tool Cache Window Changes Required:

1. **Include the header:**
   ```c
   #include "StatusWindows/main_progress_window.h"
   ```

2. **Modify GID_TOOL_REBUILD_CACHE handler:**
   ```c
   case GID_TOOL_REBUILD_CACHE:
   {
       struct iTidyMainProgressWindow progress_window;
       LayoutPreferences *prefs;
       BOOL success;
       
       log_info(LOG_GUI, "[TOOL_CACHE] Rebuild Cache button clicked\n");
       
       /* Update global preferences with current folder path from textbox */
       prefs = (LayoutPreferences *)GetGlobalPreferences();
       if (prefs)
       {
           strncpy(prefs->folder_path, tool_data->folder_path_buffer, 
                  sizeof(prefs->folder_path) - 1);
           prefs->folder_path[sizeof(prefs->folder_path) - 1] = '\0';
           UpdateGlobalPreferences(prefs);
       }
       
       /* Open progress window */
       if (!itidy_main_progress_window_open(&progress_window))
       {
           log_error(LOG_GUI, "Failed to open progress window\n");
           ShowEasyRequest(tool_data->window,
               "Error",
               "Failed to open progress window",
               "OK");
           break;
       }
       
       /* Set busy pointer on tool cache window */
       SetWindowPointer(tool_data->window, WA_BusyPointer, TRUE, TAG_END);
       
       /* Rescan the directory to rebuild tool cache */
       success = ScanDirectoryForToolsOnlyWithProgress(&progress_window);
       
       /* Show result in progress window */
       itidy_main_progress_window_append_status(&progress_window, "");
       itidy_main_progress_window_append_status(&progress_window, 
           "===============================================");
       if (success)
       {
           itidy_main_progress_window_append_status(&progress_window, 
               "Tool cache rebuilt successfully!");
       }
       else
       {
           itidy_main_progress_window_append_status(&progress_window, 
               "Tool cache rebuild failed or was incomplete");
       }
       itidy_main_progress_window_append_status(&progress_window, 
           "===============================================");
       
       /* Change Cancel button to Close */
       itidy_main_progress_window_set_button_text(&progress_window, "Close");
       
       /* Clear busy pointer */
       SetWindowPointer(tool_data->window, WA_Pointer, NULL, TAG_END);
       
       /* Keep progress window open for review */
       while (itidy_main_progress_window_handle_events(&progress_window))
       {
           WaitPort(progress_window.window->UserPort);
       }
       
       /* Close progress window */
       itidy_main_progress_window_close(&progress_window);
       
       /* Rebuild display list from refreshed cache */
       if (success)
       {
           build_tool_cache_display_list(tool_data);
           apply_tool_filter(tool_data);
           populate_tool_list(tool_data);
           tool_data->selected_index = -1;
           update_tool_details(tool_data, -1);
           log_info(LOG_GUI, "Display updated with new cache data\n");
       }
       
       break;
   }
   ```

## Benefits of Using the Same Progress Window

1. **Consistent UI**: Same look and feel across the application
2. **Code Reuse**: Already tested and working
3. **Cancel Support**: Built-in cancellation with confirmation
4. **Scrollable History**: User can review all status messages
5. **Event Handling**: Proper event loop integration

## Recommendation

**Use Option 1 (Wrapper Function)** because:
- Minimal changes to existing code
- Clean separation of concerns
- Easy to implement
- Maintains backward compatibility (can still call `ScanDirectoryForToolsOnly()` without progress)

## Files to Modify

1. **src/layout_processor.h** - Add declaration for `ScanDirectoryForToolsOnlyWithProgress()`
2. **src/layout_processor.c** - Implement wrapper function + update scan functions to use `PROGRESS_STATUS()`
3. **src/GUI/tool_cache_window.c** - Update `GID_TOOL_REBUILD_CACHE` handler to use progress window
4. **Makefile** - Already includes all necessary files

## Estimated Implementation Time
- 30-45 minutes to implement and test
- Low risk - uses proven patterns from existing code
