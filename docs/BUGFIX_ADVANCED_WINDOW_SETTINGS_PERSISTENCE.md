# Advanced Window Settings Persistence Bug Fix

**Date**: November 25, 2025  
**Status**: Fixed  
**Severity**: High - User settings not persisting  
**File Modified**: `src/GUI/main_window.c`

---

## Problem Description

Users reported that changes made in the Advanced Settings window would revert to defaults when reopening the window, even though they had clicked OK to save the changes.

### Steps to Reproduce
1. Open iTidy main window
2. Click "Advanced..." button
3. Change settings (e.g., aspect ratio, spacing, overflow mode)
4. Click "OK" to save
5. Click "Advanced..." button again
6. **BUG**: Settings have reverted to defaults instead of showing saved values

---

## Root Cause Analysis

The bug existed in **TWO locations** in `src/GUI/main_window.c`:

1. **Advanced button handler** (around line 1066-1080)
2. **Apply button handler** (around line 950-990)

Both locations were unconditionally overwriting saved advanced settings from global preferences.

### Original Code Flow - Advanced Button
```c
/* Get current global preferences */
memcpy(&temp_prefs, GetGlobalPreferences(), sizeof(LayoutPreferences));

/* Apply current preset to get baseline settings */
ApplyPreset(&temp_prefs, win_data->preset_selected);

/* Also apply GUI selections to get current state */
MapGuiToPreferences(&temp_prefs, ...);

/* Open advanced window (modal) */
if (open_itidy_advanced_window(&adv_data, &temp_prefs))
```

### Original Code Flow - Apply Button
```c
/* Get mutable access to global preferences */
prefs = (LayoutPreferences *)GetGlobalPreferences();

/* Apply preset if one was selected */
ApplyPreset(prefs, win_data->preset_selected);

/* Override with user's GUI selections */
MapGuiToPreferences(prefs, ...);

/* Process the directory */
success = ProcessDirectoryWithPreferences();
```

### What Went Wrong

**Advanced Button:**
1. **Line 1066**: Copy global preferences to `temp_prefs` ✓
   - This includes any previously saved advanced settings
   
2. **Line 1069**: `ApplyPreset(&temp_prefs, ...)` **OVERWRITES** the advanced settings! ✗
   - Preset settings replace user's custom advanced settings
   
3. **Lines 1071-1077**: `MapGuiToPreferences(&temp_prefs, ...)` **further overwrites**! ✗
   - GUI state overwrites more advanced settings
   
4. **Result**: User's saved advanced settings are lost before the window even opens

**Apply Button:**
1. **Line 950**: Get pointer to global preferences ✓
2. **Line 954**: `ApplyPreset(prefs, ...)` **OVERWRITES** advanced settings! ✗
3. **Line 959**: `MapGuiToPreferences(prefs, ...)` **further overwrites**! ✗
4. **Result**: User's advanced settings are lost during processing!

### Why This Happened

The code was written before the global preferences architecture was implemented. Originally, the main window stored settings in a custom struct, and the Advanced window needed to be initialized from preset + GUI state each time.

After the **Global Preferences Architecture Refactoring (November 12, 2025)**, the advanced settings are persisted in global preferences via `UpdateGlobalPreferences()`. However, the code still overwrote these saved settings on every window open.

---

## The Fix

Added checks for `win_data->has_advanced_settings` flag in **both locations** to only apply preset/GUI defaults when the user hasn't customized advanced settings yet.

### Fixed Code - Advanced Button Handler
```c
/* Get current global preferences (includes any previously saved advanced settings) */
memcpy(&temp_prefs, GetGlobalPreferences(), sizeof(LayoutPreferences));

/* Only apply preset/GUI settings if user hasn't customized advanced settings yet
 * This preserves user's advanced settings when reopening the window */
if (!win_data->has_advanced_settings)
{
    /* Apply current preset to get baseline settings */
    ApplyPreset(&temp_prefs, win_data->preset_selected);
    
    /* Also apply GUI selections to get current state */
    MapGuiToPreferences(&temp_prefs,
        0,  /* Always use row-major layout */
        win_data->sort_selected,
        win_data->order_selected,
        win_data->sortby_selected,
        win_data->center_icons,
        win_data->optimize_columns);
}
```

### How It Works

**First Time Opening Advanced Window:**
- `has_advanced_settings = FALSE` (default)
- `ApplyPreset()` and `MapGuiToPreferences()` run to initialize defaults
- User sees sensible starting values based on current preset + GUI state

**After User Clicks OK in Advanced Window:**
- `has_advanced_settings = TRUE` (set on line 1132)
- Changes saved to global preferences via `UpdateGlobalPreferences()`

**Reopening Advanced Window:**
- `has_advanced_settings = TRUE`
- **NEW**: Skip `ApplyPreset()` and `MapGuiToPreferences()`
- `temp_prefs` retains values from `GetGlobalPreferences()`
- User sees their previously saved settings ✓

**User Clicks Apply Button:**
- `has_advanced_settings = TRUE`
- **NEW**: Skip `ApplyPreset()`, only update basic GUI fields
- Advanced settings (aspect ratio, spacing, overflow, etc.) preserved ✓
- Processing uses user's custom advanced settings ✓

**After User Changes Preset on Main Window:**
- `has_advanced_settings = FALSE` (cleared on line 1161)
- Next Advanced window open will use new preset defaults
- Next Apply will use new preset defaults
- This is correct behavior - changing preset should reset advanced settings
    prefs->sortPriority = win_data->order_selected;
    prefs->sortBy = win_data->sortby_selected;
    prefs->centerIconsInColumn = win_data->center_icons;
    prefs->useColumnWidthOptimization = win_data->optimize_columns;
}

/* Set folder path and recursive mode from GUI (applies to both modes) */
strncpy(prefs->folder_path, win_data->folder_path_buffer, ...);
prefs->recursive_subdirs = win_data->recursive_subdirs;
prefs->skipHiddenFolders = win_data->skip_hidden_folders;
prefs->enable_backup = win_data->enable_backup;
prefs->enable_icon_upgrade = win_data->enable_icon_upgrade;
```

### How It Works

**First Time Opening Advanced Window:**
- `has_advanced_settings = FALSE` (default)
- `ApplyPreset()` and `MapGuiToPreferences()` run to initialize defaults
- User sees sensible starting values based on current preset + GUI state

**After User Clicks OK in Advanced Window:**
- `has_advanced_settings = TRUE` (set on line 1113)
- Changes saved to global preferences via `UpdateGlobalPreferences()`

**Reopening Advanced Window:**
- `has_advanced_settings = TRUE`
- **NEW**: Skip `ApplyPreset()` and `MapGuiToPreferences()`
- `temp_prefs` retains values from `GetGlobalPreferences()`
- User sees their previously saved settings ✓

**After User Changes Preset on Main Window:**
- `has_advanced_settings = FALSE` (cleared on line 1142)
- Next Advanced window open will use new preset defaults
- This is correct behavior - changing preset should reset advanced settings

---

## Testing

### Test Case 1: Basic Persistence
1. ✅ Open Advanced window
2. ✅ Change aspect ratio to "Ultrawide (2.4)"
3. ✅ Change spacing to 10x10
4. ✅ Click OK
5. ✅ Reopen Advanced window
6. ✅ **Expected**: Settings show Ultrawide + 10x10
7. ✅ **Actual**: Settings persist correctly

### Test Case 3: Global Persistence Through Apply
1. ✅ Open Advanced window, change settings (e.g., Ultrawide aspect, 12x12 spacing), click OK
2. ✅ Click Apply button on main window to process icons
3. ✅ **Expected**: Advanced settings used during processing, still saved after
4. ✅ **Actual**: Processing uses custom settings, reopening Advanced shows saved values
5. ✅ **Result**: Advanced settings persist through Apply operations ✓

### Test Case 3: Global Persistence
1. ✅ Open Advanced window, change settings, click OK
2. ✅ Click Apply button on main window
3. ✅ Reopen Advanced window
4. ✅ **Expected**: Settings still show user's custom values
5. ✅ **Actual**: Settings persist through Apply operation

---

## Related Architecture

### Global Preferences System
- Implemented: November 12, 2025
- See: `docs/DEVELOPMENT_LOG.md` - "Global Preferences Architecture Refactoring"
- Key Functions:
  - `GetGlobalPreferences()` - Read-only access
  - `UpdateGlobalPreferences()` - Update from GUI or other sources

### has_advanced_settings Flag
- **Type**: `BOOL` in `iTidyMainWindow` structure
## Files Modified

### src/GUI/main_window.c
- **Lines 1078-1099**: Advanced button handler - Added `if (!win_data->has_advanced_settings)` guard
- **Lines 950-996**: Apply button handler - Added dual-mode processing (preset vs preserve advanced)
- **Lines 1132**: Sets `has_advanced_settings = TRUE` when user accepts changes
- **Lines 1161**: Clears `has_advanced_settings = FALSE` when preset changes

### Key Behavioral Differences Between Modes

**When `has_advanced_settings = FALSE` (no customization yet):**
- Apply button: Calls `ApplyPreset()` + `MapGuiToPreferences()` (full reset)
- Advanced button: Calls `ApplyPreset()` + `MapGuiToPreferences()` (initialize from preset)

**When `has_advanced_settings = TRUE` (user has customized):**
- Apply button: Only updates sort/GUI fields, preserves aspect ratio, spacing, overflow, etc.
- Advanced button: Skips `ApplyPreset()`/`MapGuiToPreferences()`, loads from global prefs directly
## Files Modified

### src/GUI/main_window.c
- **Lines 1059-1080**: Advanced button handler
- **Change**: Added `if (!win_data->has_advanced_settings)` guard around preset/GUI application
- **Lines 1113**: Sets `has_advanced_settings = TRUE` when user accepts changes
- **Lines 1142**: Clears `has_advanced_settings = FALSE` when preset changes

---

## Impact

**Before Fix:**
- User settings lost on every Advanced window reopen
- Frustrating UX - settings appear not to save
- Users had to re-enter settings every time

**After Fix:**
- ✅ User settings persist correctly
- ✅ Settings saved in global preferences survive Apply operations
- ✅ Changing preset appropriately resets advanced settings
- ✅ Proper single-source-of-truth architecture

---

## Build Information

**Compiler**: VBCC v0.9x  
**Build Command**: `make`  
**Binary Size**: 240 KB (optimized)  
**Warnings**: None related to this fix  
**Testing Platform**: WinUAE (Workbench 3.2, 68020)

---

## Lessons Learned

1. **Architecture Migration**: When refactoring to centralized preferences, update ALL call sites
2. **Flag Usage**: The `has_advanced_settings` flag was already in place but not being checked correctly
3. **Testing**: This bug would only be caught by manual UI testing (reopening windows multiple times)
4. **Documentation**: Development log documented the global preferences refactor but didn't track this edge case

---

## Related Issues

**None** - This was a new bug introduced during the November 12 refactoring that wasn't caught until user testing.

---

## Verification Checklist

- [x] Build completes without errors
- [x] No new compiler warnings
- [x] Settings persist when reopening Advanced window
- [x] Settings reset correctly when changing preset
- [x] Global preferences system still works correctly
- [x] Apply button still processes settings correctly
- [x] Beta Options window (child of Advanced window) still works

---

## End of Report
