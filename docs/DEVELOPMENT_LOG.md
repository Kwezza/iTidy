# iTidy Development Log

## Project Overview
iTidy is an Amiga icon management utility that allows users to sort and arrange icons in directories. The application features a GadTools-based GUI and supports multiple sorting modes with configurable layout presets.

## Development Timeline

### Phase 1: Initial Bug Fixes (Console & Positioning Issues)

#### Problem: Console Corruption and Icons Not Moving
- **Issue**: Console output was garbled with Unicode characters, icons weren't being repositioned after sorting
- **Root Cause**: 
  - Unicode checkmark character (✓) causing display issues on Amiga console
  - Layout system was recreating the icon array, losing the sorted order
- **Solution**:
  - Removed Unicode characters, replaced with "Done!"
  - Fixed printf format strings (%d → %lu with proper casts)
  - Created new `CalculateLayoutPositions()` function that works on pre-sorted array

#### Files Modified:
- `src/file_directory_handling.c` - Removed Unicode, fixed format strings
- `src/layout_processor.c` - Created new layout calculation function

---

### Phase 2: Enhanced Debugging & Logging System

#### Implementation: Comprehensive Section-Tagged Logging
- **Purpose**: Track icon processing through all stages for debugging
- **Features**:
  - Section headers: LOADING, SORTING, CALCULATING, SAVING
  - Detailed icon tables showing properties at each stage
  - Added columns: IsDir, Size, Date, Type
  - File-based logging to `PROGDIR:iTidy.log`

#### Files Modified:
- `src/writeLog.c` - Enhanced logging functions
- `src/file_directory_handling.c` - Added detailed icon table output
- `src/layout_processor.c` - Added layout calculation logging

---

### Phase 3: GUI Bug Fix (Cycle Gadget Race Condition)

#### Problem: GUI Selection Mismatch
- **Issue**: GUI showed "Mixed" but code received sortPriority=0, causing incorrect sorting behavior
- **Root Cause**: `GT_GetGadgetAttrs()` was returning stale data before gadget updated
- **Solution**: Switched to using `IntuiMessage->Code` directly from gadget events
- **Result**: Perfect synchronization between GUI display and internal state

#### Files Modified:
- `src/GUI/main_window.c` - Changed from GT_GetGadgetAttrs() to msg->Code
- Added preset synchronization for all cycle gadgets

---

### Phase 4: Save Loop Bug Fix

#### Problem: Only 2 Icons Saved Out of 26
- **Issue**: Layout calculated positions for all 26 icons, but only 2 were written to disk
- **Root Cause**: Missing brace placement caused `FreeDiskObject()` to execute outside the conditional block
- **Solution**: Fixed brace structure in save loop, added extensive logging
- **Result**: All 26 icons now save successfully

#### Files Modified:
- `src/file_directory_handling.c` - Fixed saveIconsPositionsToDisk() brace structure
- Added "Processing icon X:" and "Saved OK:" logging for each icon

---

### Phase 5: Dynamic Layout System Overhaul

#### Problem: Fixed Icons-Per-Row Too Rigid
- **Issue**: `maxIconsPerRow=10` caused icons to go off-screen on 640px displays
- **Root Cause**: Fixed icon count doesn't account for variable text widths
- **Solution**: Complete rewrite of layout algorithm with width-based wrapping

#### New Layout Algorithm Features:
1. **Dynamic Width-Based Wrapping**
   - Tracks cumulative X position (`currentX`)
   - Calculates next position: `nextX = currentX + icon_max_width + spacing`
   - Wraps when `nextX > effectiveMaxWidth - rightMargin`
   - No longer relies on fixed icon count per row

2. **Percentage-Based Width Limiting**
   - Respects `maxWindowWidthPct` from preferences
   - Calculation: `effectiveMaxWidth = (screenWidth * maxWindowWidthPct) / 100`
   - Classic preset: 55% of screen width (440px on 800px screen)

3. **Dynamic Row Heights**
   - Tracks `maxHeightInRow` for tallest icon in each row
   - Advances Y position: `currentY = rowStartY + maxHeightInRow + 8`
   - Prevents icon overlapping when mixing different icon heights

4. **Icon Centering for Wide Text**
   - Calculates offset when text is wider than icon image
   - Formula: `iconOffset = (text_width - icon_width) / 2`
   - Centers icon horizontally within allocated text width space
   - Applies offset to icon_x while currentX tracks spacing width

#### Algorithm Logic:
```c
// Calculate effective width from percentage
if (prefs->maxWindowWidthPct > 0 && prefs->maxWindowWidthPct < 100)
{
    effectiveMaxWidth = (maxWidth * prefs->maxWindowWidthPct) / 100;
}

// For each icon in sorted array
for (i = 0; i < iconArray->count; i++)
{
    icon = &iconArray->icons[i];
    
    // Check if we need to wrap to next row
    nextX = currentX + icon->icon_max_width + iconSpacing;
    if (currentX > 8 && nextX > (effectiveMaxWidth - rightMargin))
    {
        currentX = 8;
        currentY = rowStartY + maxHeightInRow + 8; // Dynamic height
        rowStartY = currentY;
        maxHeightInRow = 0;
    }
    
    // Track tallest icon in this row
    if (icon->icon_max_height > maxHeightInRow)
    {
        maxHeightInRow = icon->icon_max_height;
    }
    
    // Center icon if text is wider
    int iconOffset = 0;
    if (icon->text_width > icon->icon_width)
    {
        iconOffset = (icon->text_width - icon->icon_width) / 2;
    }
    
    // Set position
    icon->icon_x = currentX + iconOffset;
    icon->icon_y = currentY;
    
    // Advance for next icon
    currentX += icon->icon_max_width + iconSpacing;
}
```

#### Files Modified:
- `src/layout_processor.c` - Complete rewrite of CalculateLayoutPositions() (lines 76-165)
- `src/layout_preferences.c` - Changed Classic preset maxIconsPerRow from 10 to 5

---

### Phase 6: Quality of Life Improvements

#### Feature: Log File Management
- **Purpose**: Fresh log file on each program run for cleaner debugging
- **Implementation**: Added `delete_logfile()` function
- **Deletes**: Both `PROGDIR:iTidy.log` and fallback `iTidy.log`

#### Feature: Default Folder Change
- **Purpose**: Start in user's test directory instead of system directory
- **Change**: Default folder from "SYS:" to "PC:"
- **Benefit**: Convenience for testing workflow

#### Files Modified:
- `src/writeLog.h` - Added delete_logfile() declaration
- `src/writeLog.c` - Implemented delete_logfile() function
- `src/main_gui.c` - Call delete_logfile() before initialize_logfile()
- `src/GUI/main_window.c` - Changed default folder to "PC:"

---

## Technical Specifications

### Build Environment
- **Compiler**: VBCC v0.9x
- **Target**: Amiga OS 68020+
- **Flags**: `+aos68k -c99 -cpu=68020 -DDEBUG`
- **Output**: `Bin/Amiga/iTidy` (77,768 bytes)

### Key Data Structures

#### FullIconDetails
```c
struct FullIconDetails {
    int icon_x;              // Screen X position
    int icon_y;              // Screen Y position
    int icon_width;          // Icon image width
    int icon_height;         // Icon image height
    int text_width;          // Text label width
    int text_height;         // Text label height
    int icon_max_width;      // max(icon_width, text_width)
    int icon_max_height;     // Total height (icon + text + spacing)
    char icon_text[256];     // Icon label text
    char icon_full_path[512]; // Full filesystem path
    BOOL is_folder;          // TRUE if directory
    ULONG file_size;         // File size in bytes
    ULONG file_date;         // File modification date
};
```

#### LayoutPreferences
```c
struct LayoutPreferences {
    int layoutMode;          // Layout mode (grid, list, etc.)
    int sortOrder;           // Ascending/descending
    int sortPriority;        // Folder/file separation priority
    int sortBy;              // Name, date, size, type
    int maxWindowWidthPct;   // Percentage of screen width (1-100)
    int maxIconsPerRow;      // Fallback for fixed layouts
};
```

### Layout Algorithm Constants
- **Classic Preset**:
  - `maxWindowWidthPct`: 55%
  - `maxIconsPerRow`: 5 (fallback only)
- **Spacing**:
  - `iconSpacing`: 8px between icons
  - `leftMargin`: 8px
  - `rightMargin`: 8px
  - Row vertical spacing: 8px between rows

### Sorting System
- **Sort By**: Name (0), Date, Size, Type
- **Sort Priority**: Mixed (2) = no folder/file separation
- **Implementation**: `qsort()` with `CompareIconsWrapper()`
- **Stable Sort**: Maintains relative order for equal elements

---

## Current Status

### ✅ Completed Features
1. Dynamic width-based icon wrapping
2. Percentage-based window width limiting (55% for Classic preset)
3. Dynamic row heights (prevents overlapping)
4. Icon centering when text is wider than icon image
5. Comprehensive debug logging system
6. Log file cleanup on startup
7. Default folder set to "PC:"
8. All 26 test icons sort and save successfully
9. GUI cycle gadget synchronization working perfectly

### 🔧 Technical Achievements
- Zero memory leaks in icon array management
- Proper AmigaDOS file I/O with BPTR handles
- GadTools GUI with synchronized gadget states
- Robust error handling with detailed logging
- Clean separation between layout calculation and icon saving

### 📊 Testing Results
- **Test Directory**: PC: (26 icons)
- **Sort Mode**: Name, Mixed priority
- **Layout Mode**: Classic preset (55% width)
- **Screen Resolution**: 800x600 (PAL)
- **Result**: All icons positioned correctly, no overlapping, proper centering

---

## Debug Output Format

### Section: LOADING ICONS
```
==== SECTION: LOADING ICONS ====
Loaded 26 icons from directory
ID | Icon                    | IsDir | Width | Height | Size     | Date
--------------------------------------------------------------------------------
01 | AmigaExplorer.hdf       | No    | 80    | 56     | 512000   | 12345678
...
```

### Section: CALCULATING LAYOUT
```
==== SECTION: CALCULATING LAYOUT ====
Screen dimensions: 800 x 600
Max window width: 55% = 440px
ID | Icon                    | NewX  | NewY  | Width | Height | Offset
--------------------------------------------------------------------------------
01 | AmigaExplorer.hdf       | 8     | 8     | 80    | 56     | 0
02 | ClassicWB3.9-MWB.lha    | 96    | 8     | 120   | 56     | 5
...
```

### Section: SAVING ICONS
```
==== SECTION: SAVING ICONS ====
Processing icon 1: AmigaExplorer.hdf
  Position: (8, 8)
  Saved OK: AmigaExplorer.hdf
...
Saved 26 icons successfully
```

---

## Known Limitations & Future Enhancements

### Current Limitations
- Fixed `maxIconsPerRow` still exists as fallback (though rarely used)
- Layout algorithm assumes single window/screen context
- No support for custom icon spacing per preset

### Potential Future Enhancements
- Configurable spacing per preset
- Vertical layout mode (top-to-bottom)
- Grid snap-to alignment
- Multi-column text wrapping for very long filenames
- Custom sort order persistence
- Undo/redo for layout changes

---

## File Structure

### Core Source Files
- `src/main_gui.c` - Program entry point, initialization
- `src/GUI/main_window.c` - GadTools GUI implementation
- `src/layout_processor.c` - Dynamic layout algorithm
- `src/layout_preferences.c` - Preset configurations
- `src/file_directory_handling.c` - Icon loading and saving
- `src/icon_management.c` - Icon data structure management
- `src/icon_types.c` - Icon type detection and sorting
- `src/writeLog.c` - Logging and debug output

### Documentation
- `docs/PROJECT_OVERVIEW.md` - High-level project description
- `docs/iTidy_GUI_Feature_Design.md` - GUI design specifications
- `docs/DEVELOPMENT_LOG.md` - This file (development history)
- `docs/migration/MIGRATION_PROGRESS.md` - Platform migration tracking

---

## Build Instructions

### Compile
```bash
make
```

### Output
- Executable: `Bin/Amiga/iTidy`
- Log file: `PROGDIR:iTidy.log` or `Bin/Amiga/iTidy.log`

### Test
1. Run iTidy on Amiga
2. Select folder (defaults to PC:)
3. Choose sort options (Name, Mixed priority)
4. Apply layout
5. Check log file for debug output

---

## Lessons Learned

### 1. GUI Framework Quirks
- AmigaDOS GadTools gadgets don't update immediately
- Must use `IntuiMessage->Code` for current state, not `GT_GetGadgetAttrs()`
- Race conditions possible between GUI updates and code reads

### 2. Layout Algorithm Design
- Fixed icon counts don't work with variable-width text
- Must calculate cumulative width, not assume uniform spacing
- Dynamic row heights essential for mixed icon sizes
- Text width often exceeds icon width - must center icon image

### 3. Debugging Best Practices
- Section-tagged logging invaluable for multi-stage processing
- Tabular output easier to verify than verbose descriptions
- Log file cleanup prevents confusion with stale data
- Include both input and output states in logs

### 4. AmigaDOS File I/O
- BPTR handles require proper Open/Close pairing
- `DeleteFile()` works with both absolute and PROGDIR: paths
- Always check error codes, but avoid unused variable warnings

### 5. VBCC Compiler Considerations
- Strict about bitfield types (non-portable warnings)
- Requires explicit casts for printf format strings
- C99 mode necessary for modern C features
- Warning 357 (unterminated //) is spurious on Windows-edited files

---

## Version History

### v0.1 (Current Development Build)
- Initial implementation of dynamic layout system
- GUI with cycle gadgets for sort options
- Comprehensive debug logging
- Icon centering for wide text labels
- Log file management
- Default folder configuration

---

## Contributors
- Development: Kwezza
- AI Assistance: GitHub Copilot

---

*Last Updated: October 20, 2025*
