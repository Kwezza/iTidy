# Beta Options Window - Technical Documentation

## Overview

The Beta Options window provides GUI controls for experimental features in iTidy. These features are controlled by boolean flags in the `LayoutPreferences` structure and can be toggled on/off through a dedicated settings window.

**Created**: November 7, 2025  
**Status**: Complete and tested  
**Location**: `src/GUI/beta_options_window.c/h`

## Architecture

### Window Structure

```c
struct iTidyBetaOptionsWindow {
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Modal window */
    APTR visual_info;                   /* GadTools visual info */
    struct Gadget *glist;               /* Gadget list */
    BOOL window_open;                   /* Window state */
    
    /* Gadget pointers */
    struct Gadget *open_folders_check;
    struct Gadget *update_windows_check;
    struct Gadget *ok_btn;
    struct Gadget *cancel_btn;
    
    /* Current settings */
    BOOL open_folders_enabled;          /* beta_openFoldersAfterProcessing */
    BOOL update_windows_enabled;        /* beta_FindWindowOnWorkbenchAndUpdate */
    
    /* Pointer to preferences to update */
    LayoutPreferences *prefs;
    
    /* Result flag */
    BOOL changes_accepted;              /* TRUE if OK clicked */
};
```

### Gadget IDs

```c
#define GID_BETA_OPEN_FOLDERS       2001  /* Auto-open folders checkbox */
#define GID_BETA_UPDATE_WINDOWS     2002  /* Update window geometry checkbox */
#define GID_BETA_OK                 2003  /* OK button */
#define GID_BETA_CANCEL             2004  /* Cancel button */
```

## Controlled Features

### 1. Auto-Open Folders After Processing
**Preference**: `prefs->beta_openFoldersAfterProcessing`  
**Default**: TRUE (enabled for testing)

**Behavior**:
- When enabled, calls `OpenWorkbenchObjectA()` after tidying each folder
- Allows users to see icon arrangement results in real-time
- Particularly useful during recursive operations
- Non-fatal if window open fails (continues processing)

**Use Cases**:
- Debugging: Visual verification of icon arrangements
- User feedback: Real-time progress display
- Testing: Quick validation without manually opening folders

**Cautions**:
- May impact performance on low-memory systems
- Can clutter Workbench screen with many open folders
- Recommended for selective use, not production batch processing

### 2. Find and Update Open Folder Windows
**Preference**: `prefs->beta_FindWindowOnWorkbenchAndUpdate`  
**Default**: TRUE (enabled for testing)

**Behavior**:
- Builds tracker of open Workbench folder windows
- After iTidy resizes windows, finds matching windows by title
- Moves and resizes open windows to match saved `.info` geometry
- Uses `MoveWindow()` and `SizeWindow()` Intuition APIs

**Technical Details**:
- Gets fresh window lookups instead of caching pointers
- Properly excludes backdrop desktop windows
- Includes bounds checking (respects min/max sizes, screen boundaries)
- Uses disk-event IDCMP flags to detect volume root windows

**Limitations**:
- Window geometry cached by Workbench until reboot
- Icons move immediately, but window size/position cached
- Requires reboot to see window geometry changes for previously-opened folders

## API Reference

### Opening the Window

```c
BOOL open_itidy_beta_options_window(
    struct iTidyBetaOptionsWindow *beta_data,
    LayoutPreferences *prefs
);
```

**Parameters**:
- `beta_data` - Pointer to window structure (uninitialized)
- `prefs` - Pointer to LayoutPreferences to modify

**Returns**: TRUE on success, FALSE on error

**Behavior**:
- Initializes window structure
- Loads current preference values from `prefs`
- Locks Workbench screen
- Creates gadgets with proper IControl border calculations
- Opens centered modal window
- Window sized to 55 characters wide (reference width)

### Event Loop

```c
BOOL handle_beta_options_window_events(
    struct iTidyBetaOptionsWindow *beta_data
);
```

**Returns**: TRUE to continue, FALSE to close window

**Behavior**:
- Processes IDCMP messages using `GT_GetIMsg()`
- Updates internal state when checkboxes clicked
- Saves to preferences on OK button
- Discards changes on Cancel or close gadget
- Handles window refresh events

**Typical Usage**:
```c
struct iTidyBetaOptionsWindow beta_window;

if (open_itidy_beta_options_window(&beta_window, prefs)) {
    /* Run event loop */
    while (handle_beta_options_window_events(&beta_window)) {
        WaitPort(beta_window.window->UserPort);
    }
    
    /* Close and cleanup */
    close_itidy_beta_options_window(&beta_window);
    
    /* Check result */
    if (beta_window.changes_accepted) {
        /* Settings were saved */
    }
}
```

### Closing the Window

```c
void close_itidy_beta_options_window(
    struct iTidyBetaOptionsWindow *beta_data
);
```

**Behavior**:
- Closes window
- Frees gadgets using `FreeGadgets()`
- Frees visual info
- Unlocks Workbench screen
- Cleans up all resources

### Preference Management

```c
void load_preferences_to_beta_options_window(
    struct iTidyBetaOptionsWindow *beta_data
);
```

Loads current values from `prefs` into window gadgets. Called automatically during window opening.

```c
void save_beta_options_window_to_preferences(
    struct iTidyBetaOptionsWindow *beta_data
);
```

Saves gadget values to `prefs` structure. Called automatically when OK clicked. Logs changes to debug output.

## Integration

### From Advanced Window

The Beta Options window is accessed via a button in the Advanced Settings window:

```c
case GID_ADV_BETA_OPTIONS:
    /* Open Beta Options window */
    {
        struct iTidyBetaOptionsWindow beta_window;
        
        if (open_itidy_beta_options_window(&beta_window, adv_data->prefs)) {
            /* Run event loop */
            while (handle_beta_options_window_events(&beta_window)) {
                WaitPort(beta_window.window->UserPort);
            }
            
            /* Cleanup */
            close_itidy_beta_options_window(&beta_window);
        }
    }
    break;
```

## Layout Details

### Window Dimensions

**Reference Width**: 55 characters  
**Calculation**:
```c
window_width = BETA_MARGIN_LEFT + prefsIControl.currentLeftBarWidth +
               (BETA_WINDOW_REFERENCE_WIDTH * font_width) +
               BETA_MARGIN_RIGHT + prefsIControl.currentBarWidth;

window_height = BETA_MARGIN_TOP + prefsIControl.currentWindowBarHeight +
                ((font_height + 6) * 2) +  /* 2 checkboxes */
                (BETA_SPACE_Y * 4) +       /* Spacing */
                (font_height + 6) +        /* Button row */
                BETA_MARGIN_BOTTOM + prefsIControl.currentBarHeight;
```

### Spacing Constants

```c
#define BETA_MARGIN_LEFT   10
#define BETA_MARGIN_TOP    10
#define BETA_MARGIN_RIGHT  10
#define BETA_MARGIN_BOTTOM 10
#define BETA_SPACE_X       8
#define BETA_SPACE_Y       8
#define BETA_BUTTON_TEXT_PADDING 8
```

### Gadget Layout

1. **Checkbox 1**: Auto-open folders (PLACETEXT_RIGHT)
2. **Checkbox 2**: Update window geometry (PLACETEXT_RIGHT)
3. **Buttons**: OK and Cancel (equal width, PLACETEXT_IN)

All gadgets positioned relative to IControl borders using:
- `prefsIControl.currentLeftBarWidth`
- `prefsIControl.currentWindowBarHeight`

## Design Patterns Used

### Font-Aware Sizing
- Uses screen font for all measurements
- Calculates gadget heights as `font_height + 6`
- Calculates button widths using `TextLength()`
- Window size adapts to font size

### IControl Border Compliance
- All gadget positions include border offsets
- Window size includes full border chrome
- Matches patterns from other iTidy windows

### Equal-Width Buttons
- Calculates button width to fit 2 buttons in reference width
- Ensures buttons are at least as wide as longest text + padding
- Consistent with project button layout standards

### Modal Dialog Pattern
- Locks Workbench screen while open
- Prevents interaction with parent window
- Clean separation of concerns
- Proper cleanup on all exit paths

## Testing Notes

**Build Status**: ✅ Compiles and links successfully  
**Platform**: Amiga OS 3.x with GadTools  
**Tested With**: VBCC compiler

**Known SDK Warnings** (from Amiga headers, can be ignored):
- `prefs/icontrol.h`: Bitfield type non-portable (lines 49-50)
- `prefs/icontrol.h`: Array of size <=0 (line 94)

## Future Enhancements

1. **Descriptive Text**: Add IntuiText explaining what each beta feature does
2. **Restore Defaults**: Button to reset to default values
3. **More Beta Features**: Expand as new experimental features developed
4. **Feature Status**: Visual indicator showing which features are stable/experimental
5. **Help Context**: Integration with help system for feature explanations

## Related Documentation

- `docs/DEVELOPMENT_LOG.md` - Implementation timeline
- `docs/BUGFIX_WINDOW_MOVEMENT_LOCKUP.md` - Window update feature background
- `src/templates/AI_AGENT_GUIDE.md` - Window creation patterns
- `src/templates/AI_AGENT_LAYOUT_GUIDE.md` - Layout calculation guide
