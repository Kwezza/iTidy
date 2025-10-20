# iTidy GUI Migration Plan

## Overview
Migrate iTidy from a CLI-only program to a Workbench GUI application while maintaining all existing functionality.

## Current CLI Arguments to GUI Controls Mapping

| CLI Argument | Current Type | GUI Control Type | Notes |
|--------------|--------------|------------------|-------|
| `<directory>` | Required positional | String gadget + Drawer button | Path to process |
| `-subdirs` | Flag | Checkbox | "Process subdirectories" |
| `-dontResize` | Flag | Checkbox | "Don't resize windows" |
| `-ViewShowAll` | Flag | Cycle gadget | View mode options |
| `-ViewDefault` | Flag | Cycle gadget | View mode options |
| `-ViewByName` | Flag | Cycle gadget | View mode options |
| `-ViewByType` | Flag | Cycle gadget | View mode options |
| `-resetIcons` | Flag | Checkbox | "Reset icon positions" |
| `-skipWHD` | Flag | Checkbox | "Skip WHDLoad cleanup" |
| `-forceStd` | Flag | Checkbox | "Force standard icons" |
| `-xPadding:N` | Integer (0-30) | Integer gadget | X spacing |
| `-yPadding:N` | Integer (0-30) | Integer gadget | Y spacing |
| `-fontName:` | String | String gadget | Font name |
| `-fontSize:N` | Integer (1-30) | Integer gadget | Font size |

## Global Variables to Preserve

These variables track the current settings and must be maintained:

```c
BOOL user_dontResize;              // Maps to GUI checkbox
BOOL user_cleanupWHDLoadFolders;   // Maps to GUI checkbox (inverted logic)
BOOL user_folderViewMode;          // Maps to GUI cycle gadget
BOOL user_folderFlags;             // Maps to GUI cycle gadget
BOOL user_stripIconPosition;       // Maps to GUI checkbox
BOOL user_forceStandardIcons;      // Maps to GUI checkbox
int ICON_SPACING_X;                // Maps to GUI integer gadget
int ICON_SPACING_Y;                // Maps to GUI integer gadget
struct FontPrefs *fontPrefs;       // Maps to GUI font selection
```

## Code Sections to Remove

1. **Help Text Arrays** (lines ~111-188)
   - `help_text2[]`
   - `help_text4[]`
   - `help_text[]`
   - No longer needed - GUI is self-documenting

2. **print_usage() Function** (lines ~280-312)
   - CLI help display
   - Replace with GUI About/Help window later

3. **Command-line Parsing Loop** (lines ~505-592)
   - The entire `for (i = 0; i < argc; i++)` loop
   - Will be replaced by GUI gadget handling

4. **display_help() Call** (line ~489)
   - CLI help invocation
   - Remove entirely

## Code Sections to Keep

1. **All initialization code** (lines ~380-450)
   - Timer setup
   - Font preferences allocation
   - Error tracker initialization
   - Workbench version checking

2. **All global variables and structures**
   - Keep all existing globals
   - They'll be set by GUI instead of CLI

3. **All processing functions**
   - `ProcessDirectory()`
   - `InitializeWindow()`
   - `CleanupWindow()`
   - All icon management functions
   - All file handling functions

4. **Summary/Statistics Display** (lines ~646-745)
   - Keep this logic but adapt for GUI display
   - Could show in a requester or status area

## New Code to Add

1. **GUI Includes**
```c
#include "GUI/main_window.h"
```

2. **GUI Initialization**
```c
struct iTidyMainWindow gui_window;
```

3. **GUI Event Loop**
```c
while (keep_running) {
    keep_running = handle_itidy_window_events(&gui_window);
}
```

## Proposed New main() Structure

```c
int main(int argc, char **argv)
{
    struct iTidyMainWindow gui_window;
    BOOL keep_running;
    
    // Keep: Banner and version display
    printf("iTidy V%s - Amiga Workbench Icon Tidier\n", VERSION);
    
    // Keep: All initialization
    if (setupTimer() != 0) { ... }
    fontPrefs = AllocVec(...);
    iconsErrorTracker initialization...
    
    // Keep: Workbench version check
    if (workbenchVersion < 36000) { ... }
    
    // Keep: Icon spacing adjustments for WB 2.x
    if (workbenchVersion < 44500) { ... }
    
    // Keep: Log initialization
    initialize_logfile();
    
    // NEW: Initialize default settings
    user_dontResize = FALSE;
    user_folderViewMode = DDVM_BYICON;
    user_folderFlags = DDFLAGS_SHOWICONS;
    user_cleanupWHDLoadFolders = TRUE;
    user_stripIconPosition = FALSE;
    user_forceStandardIcons = FALSE;
    
    // Keep: Load preferences
    fontPrefs = getIconFont();
    fetchWorkbenchSettings(&prefsWorkbench);
    fetchIControlSettings(&prefsIControl);
    
    // Keep: Initialize window system (for icon processing)
    InitializeWindow();
    
    // NEW: Open GUI window
    if (!open_itidy_main_window(&gui_window)) {
        printf("Failed to open GUI window\n");
        cleanup and return;
    }
    
    // NEW: GUI event loop
    keep_running = TRUE;
    while (keep_running) {
        keep_running = handle_itidy_window_events(&gui_window);
        // Future: Check for "Start" button click
        // Future: Call ProcessDirectory() when triggered
    }
    
    // NEW: Close GUI
    close_itidy_main_window(&gui_window);
    
    // Keep: All cleanup code
    CleanupWindow();
    disposeTimer();
    FreeIconErrorList(&iconsErrorTracker);
    if (fontPrefs) FreeVec(fontPrefs);
    
    // Keep: Exit with RemTask workaround
    RemTask(NULL);
    return RETURN_OK;
}
```

## Migration Steps

### Phase 1: Basic GUI Integration (Current)
- [x] Create simple GUI window (main_window.c/h)
- [x] Test window opening/closing
- [ ] Replace main() CLI code with GUI initialization
- [ ] Remove help text and print_usage()
- [ ] Test that window opens and closes properly

### Phase 2: Add Basic GUI Controls
- [ ] Add directory path string gadget
- [ ] Add "Start" button
- [ ] Add "Quit" button
- [ ] Wire up buttons to event handler
- [ ] Test button responses

### Phase 3: Add All Setting Controls
- [ ] Add all checkbox gadgets for flags
- [ ] Add cycle gadget for view modes
- [ ] Add integer gadgets for spacing
- [ ] Add font selection gadgets
- [ ] Wire all gadgets to settings variables

### Phase 4: Connect Processing
- [ ] Trigger ProcessDirectory() from "Start" button
- [ ] Show progress during processing
- [ ] Display summary in GUI requester
- [ ] Handle errors gracefully

### Phase 5: Polish
- [ ] Add menus
- [ ] Add keyboard shortcuts
- [ ] Add tooltips/help
- [ ] Test on different Workbench versions
- [ ] Update documentation

## Notes

- Keep all CLI argument parsing code in comments initially
- Each CLI flag maps directly to a GUI control
- Default values should match original CLI defaults
- Maintain all global variables - just set them differently
- Processing logic remains completely unchanged
- Only the input method (CLI vs GUI) changes

## Testing Checklist

- [ ] Window opens on Workbench 3.0
- [ ] Window opens on Workbench 2.0 (if available)
- [ ] All gadgets respond to input
- [ ] Settings are correctly applied
- [ ] ProcessDirectory() still works
- [ ] Summary displays correctly
- [ ] Memory cleanup is proper
- [ ] No crashes on exit
