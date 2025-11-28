# iTidy Main Progress Window

**Module**: `src/GUI/StatusWindows/main_progress_window.c`  
**Header**: `src/GUI/StatusWindows/main_progress_window.h`  
**Purpose**: Displays scrolling status history in a ListView with Cancel button for long-running operations  
**Target**: Workbench 3.0/3.1 (GadTools)  
**Date Created**: November 28, 2025

---

## Overview

The **iTidy Main Progress Window** is a general-purpose status display designed for long-running iTidy operations (recursive directory processing, backup generation, etc.). Unlike the older progress windows that combine a progress bar with a listview, this window focuses on **scrolling status messages** with a full-width **Cancel** button.

### Key Features

1. **Scrolling ListView History** (6 visible rows)
   - Displays status messages in chronological order
   - Auto-scrolls to most recent entry
   - Read-only (user cannot select items for editing)
   - Automatically trims old entries when history exceeds **50 messages** (configurable via `ITIDY_MAIN_PROGRESS_MAX_HISTORY`)

2. **Cancel Button**
   - Full-width button below the ListView
   - Sets `cancel_requested` flag when clicked
   - Caller is responsible for checking this flag and aborting operations

3. **Window Management**
   - Centered on Workbench screen
   - Title: **"iTidy - Progress"**
   - Drag bar, depth gadget, close gadget enabled
   - Modal-like behavior (event pump blocks until Cancel/Close)

4. **Memory Management**
   - Uses `whd_malloc()`/`whd_free()` for tracked allocations
   - Exec list for status entries (`struct Node` with `ln_Name` pointing to text)
   - Proper cleanup order: detach list → close window → free gadgets → free entries

---

## API Usage

### 1. Declare Window Data Structure

```c
#include "GUI/StatusWindows/main_progress_window.h"

struct iTidyMainProgressWindow progress_window;
```

### 2. Open the Window

```c
if (!itidy_main_progress_window_open(&progress_window))
{
    // Handle error - could not open window
    return;
}
```

**What happens during open:**
- Locks Workbench screen
- Calculates layout based on system font metrics
- Creates ListView gadget (initially seeded with "Status...")
- Creates Cancel button
- Opens window at center of screen
- Sets `window_open = TRUE`, `cancel_requested = FALSE`

### 3. Append Status Messages

```c
itidy_main_progress_window_append_status(&progress_window, "Processing DH0:Work");
itidy_main_progress_window_append_status(&progress_window, "Resizing 12 icons...");
itidy_main_progress_window_append_status(&progress_window, "Backup complete.");
```

**Features:**
- Automatically scrolls to the most recent entry (`GTLV_MakeVisible`)
- Trims oldest entries when count exceeds `ITIDY_MAIN_PROGRESS_MAX_HISTORY` (default: 50)
- Returns `TRUE` on success, `FALSE` if allocation fails

**Thread Safety:** **NOT thread-safe**. Call only from the main Intuition task.

### 4. Check for Cancel (Event Pump Pattern)

```c
BOOL keep_running = TRUE;

while (keep_running)
{
    // Do some work
    process_next_directory();
    
    itidy_main_progress_window_append_status(&progress_window, "Processed folder X");
    
    // Check for user cancel
    keep_running = itidy_main_progress_window_handle_events(&progress_window);
    
    if (progress_window.cancel_requested)
    {
        break;  // User clicked Cancel or close gadget
    }
}
```

**Event Handler Behavior:**
- Processes all pending Intuition messages
- Sets `cancel_requested = TRUE` on Cancel button click or close gadget
- Returns `FALSE` when Cancel/Close detected, `TRUE` otherwise
- Handles `IDCMP_REFRESHWINDOW` and `IDCMP_GADGETDOWN` (for ListView scroll)

### 5. Close the Window

```c
itidy_main_progress_window_close(&progress_window);
```

**Cleanup Order (CRITICAL):**
1. Detach ListView labels (`GTLV_Labels, ~0`)
2. Close window
3. Free gadgets (`FreeGadgets`)
4. Free visual info
5. Free all history entries and their text
6. Unlock Workbench screen

---

## Complete Example (Standalone Test)

This example shows how to open the window, populate it with sample status messages, wait for user interaction, and clean up properly.

```c
#include "GUI/StatusWindows/main_progress_window.h"
#include <proto/dos.h>

void test_main_progress_window(void)
{
    struct iTidyMainProgressWindow progress_window;
    BOOL keep_running;
    int i;
    
    // Open the window
    if (!itidy_main_progress_window_open(&progress_window))
    {
        Printf("ERROR: Failed to open main progress window\n");
        return;
    }
    
    // Seed some initial status messages
    itidy_main_progress_window_append_status(&progress_window, "Starting operation...");
    itidy_main_progress_window_append_status(&progress_window, "Scanning directories...");
    itidy_main_progress_window_append_status(&progress_window, "Processing icons...");
    
    // Event pump: wait for user to click Cancel or close gadget
    keep_running = TRUE;
    while (keep_running)
    {
        keep_running = itidy_main_progress_window_handle_events(&progress_window);
        
        if (progress_window.cancel_requested)
        {
            Printf("User cancelled operation\n");
            break;
        }
        
        // Optional: append more messages during event loop
        // (In real usage, this would be driven by your processing logic)
    }
    
    // Clean up
    itidy_main_progress_window_close(&progress_window);
}
```

---

## Example: Integration with Recursive Directory Processing

This is a **realistic pattern** showing how to integrate the progress window with iTidy's recursive directory processor.

```c
#include "GUI/StatusWindows/main_progress_window.h"
#include "layout_processor.h"
#include <proto/dos.h>

BOOL process_directory_with_progress(const char *root_path)
{
    struct iTidyMainProgressWindow progress_window;
    const LayoutPreferences *prefs;
    BOOL success = FALSE;
    BOOL keep_running;
    char status_buffer[256];
    
    prefs = GetGlobalPreferences();
    if (prefs == NULL)
    {
        return FALSE;
    }
    
    // Open progress window
    if (!itidy_main_progress_window_open(&progress_window))
    {
        return FALSE;
    }
    
    // Show initial status
    snprintf(status_buffer, sizeof(status_buffer), "Starting recursive scan: %s", root_path);
    itidy_main_progress_window_append_status(&progress_window, status_buffer);
    
    // Start processing (this is a simplified example - in reality,
    // you would need to integrate cancel checking into the processor)
    success = ProcessDirectoryWithPreferences();
    
    if (success)
    {
        itidy_main_progress_window_append_status(&progress_window, "Processing complete.");
    }
    else
    {
        itidy_main_progress_window_append_status(&progress_window, "Processing failed or cancelled.");
    }
    
    // Wait for user to review results or click Cancel
    keep_running = TRUE;
    while (keep_running)
    {
        keep_running = itidy_main_progress_window_handle_events(&progress_window);
    }
    
    // Clean up
    itidy_main_progress_window_close(&progress_window);
    
    return success && !progress_window.cancel_requested;
}
```

**Note**: For true cancel support during processing, you would need to periodically call `itidy_main_progress_window_handle_events()` from within your processing loop and check `cancel_requested`.

---

## Window Layout Constants

All layout characteristics are defined in `main_progress_window.h`:

| Constant | Value | Description |
|----------|-------|-------------|
| `ITIDY_MAIN_PROGRESS_LIST_ROWS` | 6 | Number of visible ListView rows |
| `ITIDY_MAIN_PROGRESS_MARGIN_X` | 15 | Left/right content margin |
| `ITIDY_MAIN_PROGRESS_MARGIN_TOP` | 12 | Top content margin |
| `ITIDY_MAIN_PROGRESS_MARGIN_BOTTOM` | 12 | Bottom content margin |
| `ITIDY_MAIN_PROGRESS_SPACE_Y` | 8 | Vertical spacing between gadgets |
| `ITIDY_MAIN_PROGRESS_WIDTH_CHARS` | 60 | Target ListView width in characters |
| `ITIDY_MAIN_PROGRESS_MIN_WIDTH` | 320 | Minimum ListView pixel width |
| `ITIDY_MAIN_PROGRESS_MAX_HISTORY` | 50 | Maximum number of status entries before trimming |

**To adjust history cap:** Change `ITIDY_MAIN_PROGRESS_MAX_HISTORY` in the header and rebuild.

---

## Gadget IDs

| ID | Constant | Purpose |
|----|----------|---------|
| 4201 | `ITIDY_MAIN_PROGRESS_GID_HISTORY` | ListView gadget |
| 4202 | `ITIDY_MAIN_PROGRESS_GID_CANCEL` | Cancel button |

**Important**: These IDs are unique to this window and will not conflict with other iTidy windows (which use different ID ranges).

---

## History Trimming Behavior

When the number of status entries exceeds `ITIDY_MAIN_PROGRESS_MAX_HISTORY`, the **oldest entries** are automatically removed from the head of the exec list. This prevents unbounded memory growth during long operations.

**How it works:**
1. New entry is added via `AddTail()` (appended to end of list)
2. `history_count` is incremented
3. `while (history_count > ITIDY_MAIN_PROGRESS_MAX_HISTORY)`:
   - Remove oldest node from list head (`Remove()`)
   - Free the text buffer (`whd_free(entry->text)`)
   - Free the entry structure (`whd_free(entry)`)
   - Decrement `history_count`
4. Refresh ListView with updated list (`GT_SetGadgetAttrs`, `GTLV_MakeVisible`)

**Performance**: Trimming is O(n) where n = number of entries to remove (typically 1). No impact on normal operation.

---

## Memory Tracking

All allocations use iTidy's custom memory tracking system (`whd_malloc()`/`whd_free()`). To debug memory leaks:

1. Enable tracking in `include/platform/platform.h`:
   ```c
   #define DEBUG_MEMORY_TRACKING
   ```

2. Rebuild:
   ```powershell
   make clean && make
   ```

3. Run iTidy and check logs:
   ```
   Bin/Amiga/logs/memory_YYYY-MM-DD_HH-MM-SS.log
   ```

**Expected Allocations (per status entry):**
- 1× `iTidyMainProgressEntry` struct (~16 bytes)
- 1× Text buffer (strlen + 1)

**Cleanup**: All allocations are freed in `free_history_entries()` and `itidy_main_progress_window_close()`.

---

## IDCMP Flags

The window listens for the following events:

| Flag | Purpose |
|------|---------|
| `IDCMP_CLOSEWINDOW` | User clicked close gadget |
| `IDCMP_GADGETUP` | User clicked Cancel button |
| `IDCMP_GADGETDOWN` | User clicked ListView scroll arrows (triggers refresh) |
| `IDCMP_REFRESHWINDOW` | Window needs redraw |

**Why `IDCMP_GADGETDOWN`?**  
ListView scroll buttons do not work without this flag. If you forget `IDCMP_GADGETDOWN`, the scroll arrows will appear but clicking them does nothing. This is a **CRITICAL** requirement for ListView gadgets. See `src/templates/AI_AGENT_LAYOUT_GUIDE.md` for details.

---

## Testing (Temporary Integration)

A **temporary test button** has been added to `src/GUI/main_window.c` for quick visual testing:

**Button Label**: "Show Progress Window (TEMP)"  
**Location**: Below the "Restore from Backup" button in the main iTidy window  
**Behavior**: Opens the progress window, seeds 4 sample status messages, waits for Cancel/Close

**To test:**
1. Build iTidy: `make`
2. Run `Bin/Amiga/iTidy` in WinUAE
3. Click the temporary button
4. Observe ListView population and scroll behavior
5. Click Cancel or close gadget to dismiss

**TODO**: Remove this temporary button once the progress window is integrated into the actual processing pipeline.

---

## Future Integration Points

The main progress window is designed to replace or supplement existing progress displays in these areas:

1. **Recursive Directory Processing** (`src/layout_processor.c`)
   - Show folder names as they are processed
   - Display "Processing DH0:Work (12 icons)"
   - Allow cancel during deep recursion

2. **Backup Operations** (`src/backup_session.c`)
   - "Creating backup: Workbench:Storage (session 42)"
   - "Compressing 48 icon files..."
   - "Backup complete: DH0:Backups/iTidy_20251128_143022.lha"

3. **Batch Icon Upgrades** (`src/GUI/default_tool_update_window.c`)
   - "Upgrading 127 icons to NewIcons format"
   - "Processed 50/127..."
   - Show problematic icons in scrolling history

4. **Window Enumeration** (`src/GUI/window_enumerator.c`)
   - "Scanning open Workbench windows..."
   - "Found 12 open drawers"
   - "Updating: DH0:Projects"

---

## Comparison with Other Progress Windows

iTidy has several progress-related windows. Here's how they differ:

| Window | File | Purpose |
|--------|------|---------|
| **Main Progress** | `main_progress_window.c` | General-purpose scrolling status history + Cancel |
| **Old Progress** | `progress_window.c` | Single operation progress bar + short status ListView |
| **Recursive Progress** | `recursive_progress.c` | Recursive scan depth tracker with progress bar |

**When to use Main Progress Window:**
- Long-running operations with many status updates
- Need cancel support from user
- Want to retain full history (up to 50 entries)
- No need for percentage-based progress bar

**When to use Old Progress Window:**
- Single task with known completion percentage
- Short status messages (< 10 entries)
- Want traditional progress bar

---

## Known Limitations

1. **Not Thread-Safe**: All API calls must be made from the main Intuition task.
2. **No Progress Bar**: This window is history-focused. Use `progress_window.c` if you need a percentage bar.
3. **Fixed Visible Rows**: Currently hardcoded to 6 rows. Can be adjusted via `ITIDY_MAIN_PROGRESS_LIST_ROWS`.
4. **No Item Selection**: ListView is read-only (`GTLV_ReadOnly, TRUE`). Users cannot select individual entries.
5. **Cancel is Cooperative**: The `cancel_requested` flag only works if the caller checks it regularly during processing.

---

## Troubleshooting

### Symptom: ListView scroll buttons don't work

**Cause**: Missing `IDCMP_GADGETDOWN` flag in window's IDCMP mask.

**Fix**: Verify `WA_IDCMP` includes `IDCMP_GADGETDOWN`:
```c
WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_REFRESHWINDOW,
```

### Symptom: System crash on window close

**Cause**: Closing window before detaching ListView labels or freeing gadgets after window closure.

**Fix**: Follow the **correct cleanup order** in `itidy_main_progress_window_close()`:
1. Detach labels (`GTLV_Labels, ~0`)
2. Close window
3. Free gadgets
4. Free visual info
5. Free history entries

### Symptom: Memory leaks reported

**Cause**: Not freeing all allocated text buffers or entry structures.

**Fix**: Ensure `free_history_entries()` is called during cleanup and that every `whd_malloc()` has a corresponding `whd_free()`.

### Symptom: Window appears but no status messages

**Cause**: Forgot to call `itidy_main_progress_window_append_status()` after opening.

**Fix**: The window opens with one placeholder entry ("Status..."). Replace or append to it using the append API.

---

## References

- **Template Guide**: `src/templates/AI_AGENT_GETTING_STARTED.md`
- **Layout Patterns**: `src/templates/AI_AGENT_LAYOUT_GUIDE.md` (Section 0: CRITICAL ListView requirements)
- **Memory System**: `docs/MEMORY_TRACKING_QUICKSTART.md`
- **GadTools Reference**: AmigaOS SDK 3.2 AutoDocs (gadtools.library)
- **Copilot Instructions**: `.github/copilot-instructions.md` (Window cleanup order, snake_case rules)

---

## Changelog

| Date | Change |
|------|--------|
| 2025-11-28 | Initial implementation with 50-entry history cap |
| 2025-11-28 | Documentation created |

