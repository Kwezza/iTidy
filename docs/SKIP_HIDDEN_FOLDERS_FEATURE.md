# Skip Hidden Folders Feature

## Overview
Added a new advanced preference option to skip folders without `.info` files (hidden folders) during recursive processing.

## Implementation Date
October 20, 2025

## Description
On Amiga systems, folders without matching `.info` files are considered "hidden" - they don't appear in Workbench windows by default. This feature allows iTidy to respect this convention and skip processing such folders when enabled.

## Changes Made

### 1. Layout Preferences Structure (`layout_preferences.h`)
- Added new boolean field: `skipHiddenFolders`
- Added default constant: `DEFAULT_SKIP_HIDDEN_FOLDERS` (TRUE)
- Located in the "Advanced Settings" section of the preferences structure

### 2. Layout Preferences Implementation (`layout_preferences.c`)
- Updated `InitLayoutPreferences()` to initialize `skipHiddenFolders` to TRUE

### 3. Recursive Processing Logic (`layout_processor.c`)
- Modified `ProcessDirectoryRecursive()` to check for `.info` files before entering subdirectories
- When `skipHiddenFolders` is enabled:
  - Attempts to lock `<foldername>.info`
  - If lock fails (no .info file exists), the folder is skipped
  - Logs the skipped folder for debugging

### 4. GUI Components (`main_window.h` and `main_window.c`)
- Added gadget ID: `GID_SKIP_HIDDEN` (13)
- Added checkbox gadget: "Skip Hidden Folders"
- Added structure field: `skip_hidden_folders`
- Checkbox defaults to checked (enabled)
- Event handler updates the preference when checkbox is toggled
- Preference is passed to the layout processor when Apply is clicked

### 5. Debug Output (`main_window.c`)
- Added "ADVANCED SETTINGS" section to log output
- Shows current state of "Skip Hidden" preference

## Code Example

### Before (processed all folders):
```c
while (ExNext(lock, fib))
{
    if (fib->fib_DirEntryType > 0)
    {
        snprintf(subdir, sizeof(subdir), "%s/%s", path, fib->fib_FileName);
        ProcessDirectoryRecursive(subdir, prefs, recursion_level + 1);
    }
}
```

### After (checks for .info file):
```c
while (ExNext(lock, fib))
{
    if (fib->fib_DirEntryType > 0)
    {
        snprintf(subdir, sizeof(subdir), "%s/%s", path, fib->fib_FileName);
        
        if (prefs->skipHiddenFolders)
        {
            char iconPath[520];
            BPTR iconLock;
            
            snprintf(iconPath, sizeof(iconPath), "%s.info", subdir);
            iconLock = Lock((STRPTR)iconPath, ACCESS_READ);
            
            if (!iconLock)
            {
                printf("Skipping hidden folder: %s\n", fib->fib_FileName);
                continue;
            }
            UnLock(iconLock);
        }
        
        ProcessDirectoryRecursive(subdir, prefs, recursion_level + 1);
    }
}
```

## User Interface

The checkbox appears below the "Icon Upgrade (future)" option:
```
☑ Recursive Subfolders
☐ Backup (LHA) (future)
☐ Icon Upgrade (future)
☑ Skip Hidden Folders      <-- NEW
```

## Default Behavior
- **Enabled by default**: Hidden folders are skipped
- This matches standard Workbench behavior
- Users can uncheck the box to process all folders including hidden ones

## Use Cases

### When to Keep Enabled (Default)
- Processing user-visible folders only
- Respecting Workbench conventions
- Avoiding system/cache folders that are intentionally hidden

### When to Disable
- Need to organize all folders regardless of visibility
- System maintenance tasks
- Recovering icon positions in folders where .info was lost

## Related Bug Fixes
This feature was added in the same session as fixing the NewIcon size detection bug (negative width/height values when parsing IM1 tooltype).

## Files Modified
1. `src/layout_preferences.h` - Added field and default
2. `src/layout_preferences.c` - Initialize new field
3. `src/layout_processor.c` - Implement skip logic
4. `src/GUI/main_window.h` - GUI structure changes
5. `src/GUI/main_window.c` - GUI implementation and event handling

## Testing
To test this feature:
1. Create a folder with a `.info` file (visible in Workbench)
2. Create a folder without a `.info` file (hidden in Workbench)
3. Run iTidy with recursive processing
4. With checkbox enabled: Hidden folder should be skipped
5. With checkbox disabled: Both folders should be processed

## Future Enhancements
- Could add option to auto-create .info files for hidden folders
- Could provide statistics on how many folders were skipped
- Could add a list view showing which folders were skipped
