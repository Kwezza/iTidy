# Main Progress Window API Documentation

**Module:** `src/GUI/StatusWindows/main_progress_window.c` / `main_progress_window.h`  
**Purpose:** ReAction-based progress window with scrolling history, status text, and cancel button  
**Target:** Workbench 3.2+ (ReAction required)

---

## Overview

The Main Progress Window provides a consistent way to display long-running operations to the user with:

- **Scrolling ListBrowser** - Shows up to 50 historical status messages
- **Status Text Button** - Displays real-time "heartbeat" updates (e.g., "Creating: 35/100").  As some stages like reading all icons into memory can take a few seconds or more, this heartbeat section s used for things like displaying how may icons its currently scanned, so the user doesnt start to think the program has hung.
- **Cancel Button** - Allows user to interrupt operations
- **Auto-scrolling** - Always shows most recent messages

**Visual Layout:**
```
┌─────────────────────────────────────┐
│ iTidy - Progress              [×]   │
├─────────────────────────────────────┤
│ ┌─────────────────────────────────┐ │
│ │ Progress window opened.         │ │
│ │ Backup session started...       │ │
│ │ Created 35 icon(s) recursively  │ │
│ │ Processing: Work:Music          │ │  <- Scrolling history
│ │ ...                             │ │
│ └─────────────────────────────────┘ │
│ ┌─────────────────────────────────┐ │
│ │ Creating: 35                    │ │  <- Status/heartbeat
│ └─────────────────────────────────┘ │
│ ┌─────────────────────────────────┐ │
│ │          Cancel                 │ │  <- Action button
│ └─────────────────────────────────┘ │
└─────────────────────────────────────┘
```

---

## Data Structure

### `struct iTidyMainProgressWindow`

```c
struct iTidyMainProgressWindow
{
    struct Screen *screen;          // Workbench screen
    struct Window *window;          // Intuition Window
    
    /* ReAction Objects */
    Object *window_obj;             // window.class object
    Object *main_layout;            // layout.gadget object
    Object *listbrowser_obj;        // listbrowser.gadget for history
    Object *status_label_obj;       // button.gadget for status text
    Object *cancel_button_obj;      // button.gadget for cancel
    
    struct List *history_list;      // ListBrowser list nodes
    ULONG history_count;            // Number of entries
    
    /* State */
    BOOL window_open;               // Window is visible
    BOOL cancel_requested;          // User clicked Cancel
    char cancel_button_text[32];   // Current button text
};
```

**Memory Management:**
- Allocate the structure yourself (stack or heap)
- Call `itidy_main_progress_window_open()` to initialize
- Call `itidy_main_progress_window_close()` to cleanup
- ReAction objects are auto-disposed by window disposal

---

## Core Functions

### 1. Opening the Window

```c
BOOL itidy_main_progress_window_open(struct iTidyMainProgressWindow *window_data);
```

**Purpose:** Creates and opens the progress window on the Workbench screen.

**Parameters:**
- `window_data` - Pointer to structure (will be initialized)

**Returns:**
- `TRUE` - Window opened successfully
- `FALSE` - Failed to open (check logs)

**Side Effects:**
- Opens required ReAction class libraries
- Locks Workbench screen
- Creates ReAction window with gadgets
- Sets `window_open = TRUE`
- Sets `cancel_requested = FALSE`
- Initializes cancel button text to "Cancel"

**Example:**
```c
struct iTidyMainProgressWindow progress_win;

if (itidy_main_progress_window_open(&progress_win))
{
    itidy_main_progress_window_append_status(&progress_win, "Operation started");
    
    // Do work...
    
    itidy_main_progress_window_close(&progress_win);
}
else
{
    log_error(LOG_GENERAL, "Failed to open progress window\n");
}
```

---

### 2. Closing the Window

```c
void itidy_main_progress_window_close(struct iTidyMainProgressWindow *window_data);
```

**Purpose:** Closes window and frees all resources.

**Parameters:**
- `window_data` - Pointer to structure

**Side Effects:**
- Disposes ReAction window object (auto-disposes children)
- Frees ListBrowser list nodes
- Frees list structure memory
- Unlocks Workbench screen
- Closes class libraries
- Sets `window_open = FALSE`

**Note:** Safe to call multiple times or on unopened window.

**Example:**
```c
itidy_main_progress_window_close(&progress_win);
// progress_win is now safely reset
```

---

### 3. Handling Events

```c
BOOL itidy_main_progress_window_handle_events(struct iTidyMainProgressWindow *window_data);
```

**Purpose:** Processes window events (clicks, close gadget, etc.).

**Parameters:**
- `window_data` - Pointer to structure

**Returns:**
- `TRUE` - Continue processing
- `FALSE` - User requested cancel

**Side Effects:**
- Sets `cancel_requested = TRUE` if user clicks Cancel or close gadget
- Processes button gadget clicks
- Handles "Close and Cancel" vs "Close" button logic

**Important:** Call this periodically during long operations.

**Example:**
```c
for (int i = 0; i < 1000; i++)
{
    // Check for cancel every iteration
    if (!itidy_main_progress_window_handle_events(&progress_win))
    {
        log_info(LOG_GENERAL, "User cancelled operation\n");
        break;
    }
    
    // Do work...
}
```

**Alternative Pattern (macro-based):**
```c
// Define a CHECK_CANCEL macro (used in iTidy)
#define CHECK_CANCEL() \
    if (g_progressWindow && !itidy_main_progress_window_handle_events(g_progressWindow)) \
    { \
        log_info(LOG_GENERAL, "User cancelled\n"); \
        return FALSE; \
    }

// Use in loops
while (ExNext(lock, fib))
{
    CHECK_CANCEL();
    // Process file...
}
```

---

## Status Functions

### 4. Adding Status Messages

```c
BOOL itidy_main_progress_window_append_status(struct iTidyMainProgressWindow *window_data, 
                                              const char *fmt, ...);
```

**Purpose:** Adds a message to the scrolling history ListBrowser.

**Parameters:**
- `window_data` - Pointer to structure
- `fmt` - printf-style format string
- `...` - Variable arguments

**Returns:**
- `TRUE` - Message added successfully
- `FALSE` - Failed to allocate node

**Behavior:**
- Auto-scrolls to show newest message
- Maintains max 50 messages (oldest are removed)
- Uses single-column ListBrowser
- Messages are timestamped in logs, but not in display

**Example:**
```c
itidy_main_progress_window_append_status(&progress_win, "Processing folder: %s", path);
itidy_main_progress_window_append_status(&progress_win, "Created %lu icon(s)", count);
```

**Macro Alias:**
```c
#define itidy_add_main_history_entry itidy_main_progress_window_append_status
// Can use either name
```

---

### 5. Clearing History

```c
void itidy_main_progress_window_clear_history(struct iTidyMainProgressWindow *window_data);
```

**Purpose:** Removes all messages from the history list.

**Parameters:**
- `window_data` - Pointer to structure

**Side Effects:**
- Frees all ListBrowser nodes
- Resets `history_count` to 0
- Clears ListBrowser display

**Use Case:** Starting a new phase of operation.

**Example:**
```c
// Phase 1: Icon creation
for (int i = 0; i < 100; i++)
{
    itidy_main_progress_window_append_status(&progress_win, "Created icon %d", i);
}

// Clear for Phase 2
itidy_main_progress_window_clear_history(&progress_win);

// Phase 2: Layout calculation
itidy_main_progress_window_append_status(&progress_win, "Calculating layout...");
```

---

## Heartbeat / Status Text Functions

### 6. Updating Heartbeat Text

```c
void itidy_main_progress_update_heartbeat(struct iTidyMainProgressWindow *window_data,
                                          const char *phase,
                                          LONG current,
                                          LONG total);
```

**Purpose:** Updates the status button with a real-time counter.

**Parameters:**
- `window_data` - Pointer to structure
- `phase` - Operation name (e.g., "Creating", "Processing", "Loading")
- `current` - Current progress value
- `total` - Total items (0 = unknown/unbounded)

**Display Format:**
- If `total > 0`: `"Phase: current / total"` → `"Creating: 35 / 100"`
- If `total = 0`: `"Phase: current"` → `"Creating: 35"`

**Use Case:** Real-time feedback without spamming the history list.

**Example:**
```c
// During icon creation
for (int i = 1; i <= icon_count; i++)
{
    CreateIcon(icons[i]);
    
    // Update heartbeat every iteration
    itidy_main_progress_update_heartbeat(&progress_win, "Creating", i, icon_count);
}

// After completion, clear it
itidy_main_progress_clear_heartbeat(&progress_win);
```

**Performance Note:** This is lightweight - updates only the button text, not the ListBrowser.

---

### 7. Clearing Heartbeat Text

```c
void itidy_main_progress_clear_heartbeat(struct iTidyMainProgressWindow *window_data);
```

**Purpose:** Clears the status button text (sets to empty string).

**Parameters:**
- `window_data` - Pointer to structure

**Use Case:** After operation completes, to remove stale status text.

**Example:**
```c
// Icon creation loop with heartbeat
for (int i = 1; i <= 35; i++)
{
    create_icon(icons[i]);
    itidy_main_progress_update_heartbeat(&progress_win, "Creating", i, 0);
}

// Clear the "Creating: 35" text when done
itidy_main_progress_clear_heartbeat(&progress_win);

// Add final summary to history
itidy_main_progress_window_append_status(&progress_win, "Created 35 icon(s) recursively");
```

---

## Button Control

### 8. Changing Button Text

```c
void itidy_main_progress_window_set_button_text(struct iTidyMainProgressWindow *window_data, 
                                                const char *text);
```

**Purpose:** Changes the cancel button text (e.g., "Cancel" → "Close").

**Parameters:**
- `window_data` - Pointer to structure
- `text` - New button text (max 31 characters)

**Use Case:** Change button from "Cancel" to "Close" after operation completes.

**Example:**
```c
// During operation
itidy_main_progress_window_append_status(&progress_win, "Processing...");

// User can cancel
while (processing)
{
    if (!itidy_main_progress_window_handle_events(&progress_win))
    {
        // Cancelled
        break;
    }
    do_work();
}

// Operation complete - change button to "Close"
itidy_main_progress_window_set_button_text(&progress_win, "Close");

// Now user can only close, not cancel
```

**Note:** `handle_events()` interprets "Close" differently than "Cancel":
- "Cancel" → Sets `cancel_requested = TRUE`, closes window
- "Close" → Just closes window
- Other text → Treated like "Cancel"

---

## Complete Usage Example

```c
#include "GUI/StatusWindows/main_progress_window.h"

BOOL ProcessFoldersWithProgress(const char *root_path)
{
    struct iTidyMainProgressWindow progress_win;
    ULONG files_processed = 0;
    ULONG total_files = 1000; // Known count
    
    // Open progress window
    if (!itidy_main_progress_window_open(&progress_win))
    {
        log_error(LOG_GENERAL, "Failed to open progress window\n");
        return FALSE;
    }
    
    // Initial status
    itidy_main_progress_window_append_status(&progress_win, "Starting operation...");
    
    // Phase 1: Create icons
    itidy_main_progress_window_append_status(&progress_win, "Creating missing icons...");
    
    for (int i = 0; i < total_files; i++)
    {
        // Check for cancel
        if (!itidy_main_progress_window_handle_events(&progress_win))
        {
            log_info(LOG_GENERAL, "User cancelled icon creation\n");
            itidy_main_progress_window_close(&progress_win);
            return FALSE;
        }
        
        // Create icon
        create_icon(files[i]);
        files_processed++;
        
        // Update heartbeat (lightweight)
        itidy_main_progress_update_heartbeat(&progress_win, "Creating", 
                                            files_processed, total_files);
    }
    
    // Clear heartbeat after icon creation
    itidy_main_progress_clear_heartbeat(&progress_win);
    
    // Add summary to history
    itidy_main_progress_window_append_status(&progress_win, 
                                            "Created %lu icon(s)", files_processed);
    
    // Phase 2: Layout calculation
    itidy_main_progress_window_append_status(&progress_win, "Calculating layout...");
    
    // Do layout work...
    calculate_layout();
    
    // Final status
    itidy_main_progress_window_append_status(&progress_win, "Operation complete!");
    
    // Change button to "Close" (no longer cancellable)
    itidy_main_progress_window_set_button_text(&progress_win, "Close");
    
    // Wait for user to close window
    BOOL keep_open = TRUE;
    while (keep_open)
    {
        WaitPort(progress_win.window->UserPort);
        keep_open = itidy_main_progress_window_handle_events(&progress_win);
    }
    
    // Cleanup
    itidy_main_progress_window_close(&progress_win);
    
    return TRUE;
}
```

---

## Best Practices

### ✅ DO:

1. **Check for cancel frequently** - Call `handle_events()` inside loops
2. **Use heartbeat for high-frequency updates** - Don't spam the history list
3. **Clear heartbeat after operations** - Prevent stale status text
4. **Add summaries to history** - Important milestones go in the list
5. **Change button text when done** - "Cancel" → "Close" for completed operations
6. **Initialize structure before open** - Pass valid pointer to `open()`
7. **Always call close** - Even on errors, to free resources

### ❌ DON'T:

1. **Don't add status for every file** - Use heartbeat instead
2. **Don't forget to handle events** - User can't cancel otherwise
3. **Don't leave heartbeat text showing** - Clear it when operation ends
4. **Don't modify structure directly** - Use provided functions
5. **Don't call handle_events() after cancel** - Check return value
6. **Don't skip close on error paths** - Always cleanup

---

## Common Patterns

### Pattern 1: Simple Progress with Cancel

```c
struct iTidyMainProgressWindow progress_win;

if (!itidy_main_progress_window_open(&progress_win))
    return FALSE;

itidy_main_progress_window_append_status(&progress_win, "Processing %s", path);

for (int i = 0; i < count; i++)
{
    if (!itidy_main_progress_window_handle_events(&progress_win))
        break; // Cancelled
    
    process_item(i);
}

itidy_main_progress_window_close(&progress_win);
```

### Pattern 2: Progress with Heartbeat

```c
struct iTidyMainProgressWindow progress_win;

itidy_main_progress_window_open(&progress_win);
itidy_main_progress_window_append_status(&progress_win, "Creating icons...");

for (int i = 1; i <= total; i++)
{
    if (!itidy_main_progress_window_handle_events(&progress_win))
        break;
    
    create_icon(i);
    itidy_main_progress_update_heartbeat(&progress_win, "Creating", i, total);
}

itidy_main_progress_clear_heartbeat(&progress_win);
itidy_main_progress_window_append_status(&progress_win, "Created %d icons", total);
itidy_main_progress_window_close(&progress_win);
```

### Pattern 3: Multi-Phase Operation

```c
struct iTidyMainProgressWindow progress_win;

itidy_main_progress_window_open(&progress_win);

// Phase 1
itidy_main_progress_window_append_status(&progress_win, "Phase 1: Scanning...");
scan_files(&progress_win);

// Phase 2
itidy_main_progress_window_append_status(&progress_win, "Phase 2: Processing...");
process_files(&progress_win);

// Phase 3
itidy_main_progress_window_append_status(&progress_win, "Phase 3: Finalizing...");
finalize(&progress_win);

// Done
itidy_main_progress_window_append_status(&progress_win, "Complete!");
itidy_main_progress_window_set_button_text(&progress_win, "Close");

// Wait for close
while (itidy_main_progress_window_handle_events(&progress_win))
    Delay(10);

itidy_main_progress_window_close(&progress_win);
```

### Pattern 4: Global Progress Window (iTidy Style)

```c
// Global pointer (in main module)
struct iTidyMainProgressWindow *g_progressWindow = NULL;

// Macro for easy cancel checking
#define CHECK_CANCEL() \
    if (g_progressWindow && !itidy_main_progress_window_handle_events(g_progressWindow)) \
        return FALSE;

// In functions
BOOL MyFunction(void)
{
    CHECK_CANCEL(); // Quick cancel check
    
    // Do work...
    
    if (g_progressWindow)
    {
        itidy_main_progress_window_append_status(g_progressWindow, 
                                                "Step completed");
    }
    
    return TRUE;
}
```

---

## Troubleshooting

### Window doesn't open
- **Check:** Are ReAction classes available? (Workbench 3.2+ required)
- **Check:** Is `WindowBase`, `LayoutBase`, `ButtonBase` open?
- **Check:** Is Workbench screen available?

### Events not processing
- **Check:** Are you calling `handle_events()` regularly?
- **Check:** Is window still open? (don't call after cancel)

### Heartbeat not updating
- **Check:** Is `status_label_obj` valid?
- **Check:** Are you passing correct parameters?
- **Check:** Did you forget to clear it after operation?

### Button text not changing
- **Check:** Is string too long? (max 31 characters)
- **Check:** Is `cancel_button_obj` valid?

### Memory leak
- **Check:** Did you call `close()`?
- **Check:** Did you call `close()` on all error paths?

---

## API Summary Table

| Function | Purpose | When to Use |
|----------|---------|-------------|
| `itidy_main_progress_window_open()` | Open window | Start of operation |
| `itidy_main_progress_window_close()` | Close window | End of operation (always) |
| `itidy_main_progress_window_handle_events()` | Check for cancel | Inside loops |
| `itidy_main_progress_window_append_status()` | Add message to history | Important milestones |
| `itidy_main_progress_window_clear_history()` | Clear all messages | Start new phase |
| `itidy_main_progress_update_heartbeat()` | Update status text | Real-time counters |
| `itidy_main_progress_clear_heartbeat()` | Clear status text | After operation completes |
| `itidy_main_progress_window_set_button_text()` | Change button label | Operation complete |

---

## Related Documentation

- ReAction AutoDocs: `docs/AutoDocs/window_cl.doc`, `listbrowser_gc.doc`, `button_gc.doc`
- Layout Guide: `.github/copilot-instructions.md` (ReAction section)
- Easy Request Helper: `src/GUI/easy_request_helper.c`

---

**Last Updated:** 2026-02-09  
**Author:** iTidy Development Team  
**Version:** 2.0 (ReAction)
