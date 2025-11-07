# Beta Options Preferences Persistence Fix

## Issue Description

**Date**: November 7, 2025  
**Reporter**: User  
**Status**: Fixed

### Problem

The Beta Options window was not maintaining preference state across sessions:

1. ✅ **Works**: Open Advanced → Open Beta → Change settings → OK → Open Beta again
   - Settings persist correctly within the same Advanced window session

2. ❌ **Broken**: Open Advanced → Open Beta → Change settings → OK → Close Advanced → Open Advanced → Open Beta
   - Settings reset to defaults (both ticked)
   - User changes were lost

### Root Cause

The main window (`struct iTidyMainWindow`) was creating a **temporary** `LayoutPreferences` structure each time the Advanced Settings button was clicked:

```c
case GID_ADVANCED:
{
    LayoutPreferences temp_prefs;  // ← Created fresh each time!
    
    InitLayoutPreferences(&temp_prefs);  // ← Resets to defaults
    // ... apply GUI selections ...
    
    if (win_data->has_advanced_settings)
    {
        // Apply advanced settings...
        // BUT: Beta settings were NOT being saved/restored!
    }
}
```

**The Problem**: 
- Advanced window settings (aspect ratio, spacing, etc.) were being saved to `win_data->advanced_*` fields
- Beta settings were saved to `temp_prefs`, but when changes were accepted, **only the advanced fields were copied back** to `win_data`
- Beta settings in `temp_prefs` were discarded
- Next time Advanced window opened, beta settings reset to defaults via `InitLayoutPreferences()`

## Solution

Added beta preference fields to `iTidyMainWindow` structure and implemented proper save/restore:

### 1. Added Beta Fields to Main Window Structure

**File**: `src/GUI/main_window.h`

```c
struct iTidyMainWindow {
    // ... existing fields ...
    
    /* Advanced settings */
    BOOL has_advanced_settings;
    float advanced_aspect_ratio;
    // ... other advanced fields ...
    
    /* Beta/Experimental settings */
    BOOL beta_open_folders;             /* Auto-open folders after processing */
    BOOL beta_update_windows;           /* Find and update open folder windows */
};
```

### 2. Initialize Beta Settings on Window Open

**File**: `src/GUI/main_window.c` - `open_itidy_main_window()`

```c
/* Initialize beta settings to defaults */
win_data->beta_open_folders = DEFAULT_BETA_OPEN_FOLDERS_AFTER_PROCESSING;
win_data->beta_update_windows = DEFAULT_BETA_FIND_WINDOW_ON_WORKBENCH_AND_UPDATE;
```

### 3. Restore Beta Settings When Opening Advanced Window

**File**: `src/GUI/main_window.c` - `GID_ADVANCED` handler

```c
/* If we already have advanced settings, apply those too */
if (win_data->has_advanced_settings)
{
    // ... restore advanced settings ...
    
    /* Apply beta settings */
    temp_prefs.beta_openFoldersAfterProcessing = win_data->beta_open_folders;
    temp_prefs.beta_FindWindowOnWorkbenchAndUpdate = win_data->beta_update_windows;
}
```

### 4. Save Beta Settings When Advanced Window Closes

**File**: `src/GUI/main_window.c` - `GID_ADVANCED` handler (after event loop)

```c
if (adv_data.changes_accepted)
{
    // ... save advanced settings ...
    
    /* Save beta settings to main window data */
    win_data->beta_open_folders = temp_prefs.beta_openFoldersAfterProcessing;
    win_data->beta_update_windows = temp_prefs.beta_FindWindowOnWorkbenchAndUpdate;
}
```

### 5. Apply Beta Settings in Apply Button Handler

**File**: `src/GUI/main_window.c` - `GID_APPLY` handler

```c
/* Apply advanced settings if user configured them */
if (win_data->has_advanced_settings)
{
    // ... apply advanced settings ...
    
    /* Apply beta settings */
    prefs.beta_openFoldersAfterProcessing = win_data->beta_open_folders;
    prefs.beta_FindWindowOnWorkbenchAndUpdate = win_data->beta_update_windows;
}
```

## Data Flow After Fix

```
┌─────────────────────────────────────────────────────────────────┐
│ Main Window (win_data)                                          │
│                                                                  │
│  Beta Settings Persistent Storage:                              │
│  • beta_open_folders          (initialized to TRUE on startup) │
│  • beta_update_windows        (initialized to TRUE on startup) │
└───────────────┬─────────────────────────────────────────────────┘
                │
                │ User clicks "Advanced Settings"
                ↓
┌─────────────────────────────────────────────────────────────────┐
│ Advanced Window Opens                                            │
│                                                                  │
│  temp_prefs ← InitLayoutPreferences()  (creates defaults)      │
│  temp_prefs ← win_data->beta_* fields  (restores user values)  │
└───────────────┬─────────────────────────────────────────────────┘
                │
                │ User clicks "Beta Options..."
                ↓
┌─────────────────────────────────────────────────────────────────┐
│ Beta Options Window Opens                                        │
│                                                                  │
│  Loads checkboxes from temp_prefs (now has correct values!)    │
│  User toggles settings                                           │
│  On OK: Saves to temp_prefs                                     │
└───────────────┬─────────────────────────────────────────────────┘
                │
                │ Beta Options window closes
                ↓
┌─────────────────────────────────────────────────────────────────┐
│ Back to Advanced Window                                          │
│                                                                  │
│  temp_prefs now contains updated beta settings                  │
└───────────────┬─────────────────────────────────────────────────┘
                │
                │ User clicks OK in Advanced Window
                ↓
┌─────────────────────────────────────────────────────────────────┐
│ Advanced Window Closes                                           │
│                                                                  │
│  win_data->beta_* ← temp_prefs.beta_* (SAVED!)                 │
└───────────────┬─────────────────────────────────────────────────┘
                │
                │ Settings now persisted in main window
                ↓
┌─────────────────────────────────────────────────────────────────┐
│ Next time "Advanced Settings" is clicked                        │
│                                                                  │
│  temp_prefs gets win_data->beta_* (user's previous choices!)   │
└─────────────────────────────────────────────────────────────────┘
```

## Testing Verification

### Test Case 1: Within Single Advanced Window Session
✅ **Expected**: Settings persist  
✅ **Result**: PASS (already worked)

Steps:
1. Open Advanced Settings
2. Open Beta Options → Change setting → OK
3. Open Beta Options again
4. Verify: Changes are preserved ✓

### Test Case 2: Across Advanced Window Sessions
❌ **Before Fix**: Settings reset to defaults  
✅ **After Fix**: Settings persist

Steps:
1. Open Advanced Settings
2. Open Beta Options → Uncheck "Auto-open folders" → OK
3. Close Advanced Settings
4. Open Advanced Settings again
5. Open Beta Options
6. **Verify**: "Auto-open folders" remains unchecked ✓

### Test Case 3: Apply Button Uses Beta Settings
✅ **Expected**: Beta settings applied during processing  
✅ **Result**: PASS

Steps:
1. Open Advanced Settings
2. Open Beta Options → Configure settings → OK
3. Close Advanced Settings (OK)
4. Click Apply button
5. **Verify**: `prefs.beta_*` fields contain user choices ✓

## Files Modified

1. **src/GUI/main_window.h**
   - Added `beta_open_folders` field
   - Added `beta_update_windows` field

2. **src/GUI/main_window.c**
   - `open_itidy_main_window()`: Initialize beta fields to defaults
   - `GID_ADVANCED` handler: Restore beta settings when opening Advanced window
   - `GID_ADVANCED` handler: Save beta settings when Advanced window closes
   - `GID_APPLY` handler: Apply beta settings to preferences

## Build Status

✅ **Compiles**: Successfully with VBCC  
✅ **Links**: No errors  
✅ **Warnings**: Only standard Amiga SDK warnings (can be ignored)

## Related Documentation

- `docs/BETA_OPTIONS_WINDOW.md` - Beta Options window technical details
- `docs/DEVELOPMENT_LOG.md` - Implementation timeline
- `src/layout_preferences.h` - LayoutPreferences structure definition

## Lessons Learned

**Problem Pattern**: 
Creating temporary preference structures without proper persistence layer causes settings loss across window sessions.

**Solution Pattern**:
1. Store all user-configurable settings in the main window structure
2. Initialize settings to defaults on window open
3. **Restore** settings when creating temporary preference structures
4. **Save** settings back to main window when user confirms changes
5. **Apply** settings from main window when processing

**Key Principle**:
The main window structure should be the **single source of truth** for all user preferences until Apply is clicked.
