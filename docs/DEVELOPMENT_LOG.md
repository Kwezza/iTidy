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

### Phase 7: Pattern Matching Optimization (Performance Enhancement)

#### Problem: Slow Directory Scanning on Large Folders
- **Issue**: Scanning WHDLoad folder with 600+ subdirectories and 0 .info files took ~4 seconds with extensive disk grinding
- **Root Cause**: `Examine()`/`ExNext()` scans ALL directory entries (directories + files), then filters .info files in userspace
- **Impact**: Unnecessary filesystem overhead examining entries that will never match

#### Solution: AmigaDOS Pattern Matching (MatchFirst/MatchNext)
- **Approach**: Use filesystem-level pattern matching to filter .info files before userspace processing
- **Pattern**: `"path/#?.info"` - Only returns files ending in `.info`
- **API**: AmigaDOS `MatchFirst()`/`MatchNext()`/`MatchEnd()` from `dos/dosasl.h`
- **Requirement**: Kickstart 2.0+ (available since 1990)

#### Implementation Details:

**Key Changes:**
1. **Replaced FileInfoBlock with AnchorPath**
   - `struct AnchorPath` contains embedded `FileInfoBlock` at `ap_Info`
   - More efficient memory allocation (single structure instead of separate FIB)
   - `ap_Buf` contains full path to matched file

2. **Pattern-Based Filtering**
   ```c
   // Build pattern: "path/#?.info"
   snprintf(pattern, sizeof(pattern), "%s/#?.info", dirPath);
   
   // Initialize pattern matching
   matchResult = MatchFirst(pattern, anchorPath);
   
   // Process only matched files
   while (matchResult == 0) {
       fib = &anchorPath->ap_Info;  // Get embedded FileInfoBlock
       // Process icon...
       matchResult = MatchNext(anchorPath);
   }
   
   // Clean up (CRITICAL - prevents memory leaks)
   MatchEnd(anchorPath);
   FreeDosObject(DOS_ANCHORPATH, anchorPath);
   ```

3. **Removed Directory Entry Type Checks**
   - Old code: `if (fib->fib_DirEntryType < 0 || fib->fib_DirEntryType > 0)`
   - Pattern matching only returns FILES, not directories
   - Directories detected later via `isDirectory(fullPathAndFileNoInfo)`

4. **Added Extensive Documentation**
   - ~150 lines of inline comments explaining optimization
   - Pattern syntax examples
   - API usage and return codes
   - Memory management requirements

#### Performance Results:

| Test Scenario | Old Method (Examine/ExNext) | New Method (Pattern Matching) | Improvement |
|---------------|----------------------------|------------------------------|-------------|
| **WHDLoad folder (600+ dirs, 0 icons)** | ~4 seconds (disk grinding) | **Instant** (<0.1 sec) | **~40x faster** |
| **Small folder (2 icons)** | Works correctly | **Works correctly** | Same functionality |

#### Log Output Comparison:

**Before Optimization:**
```
[timestamp] Examining all 600+ entries...
[timestamp] Filtering .info files in userspace...
[4 seconds later] No icons found
```

**After Optimization:**
```
[21:09:02] Pattern: 'Work:WHDLoad/GamesOCS/#?.info'
[21:09:02] MatchFirst returned 232 (no .info files found or error)
[21:09:02] No icons in the folder that can be arranged
```
*Return code 232 = ERROR_NO_MORE_ENTRIES (correct response)*

#### Technical Benefits:

1. **Filesystem-Level Filtering**
   - AmigaDOS filters entries in the filesystem layer
   - Only matching files cross filesystem/userspace boundary
   - Drastically reduces I/O operations

2. **Reduced Memory Overhead**
   - Single `AnchorPath` structure instead of separate FIB
   - No need to examine non-matching entries
   - Immediate rejection of non-.info files

3. **Better Resource Utilization**
   - Less CPU time spent in userspace filtering
   - Less disk I/O (no need to stat non-matching entries)
   - More responsive on slow media (floppy disks, network drives)

4. **Backward Compatible**
   - Pattern matching API available since Kickstart 2.0
   - All existing functionality preserved
   - Icon type detection (NewIcons, OS3.5, Standard) unchanged

#### Files Modified:
- `include/platform/amiga_headers.h` - Added `dos/dosasl.h` include and `DOS_ANCHORPATH` definition
- `src/icon_management.c` - Complete rewrite of `CreateIconArrayFromPath()` function (lines 102-577)
  - Replaced `Examine()`/`ExNext()` loop with `MatchFirst()`/`MatchNext()`
  - Updated all error handling paths to call `MatchEnd()` (prevents memory leaks)
  - Fixed indentation issues from nested conditional blocks
  - Added comprehensive comments explaining optimization

#### Testing Results (October 20, 2025):
- ✅ WHDLoad/GamesOCS folder (600+ subdirectories): **Instant response**
- ✅ 1000ccTurbo subfolder (2 icons): **Correctly detected and processed**
- ✅ Standard icons: **Working**
- ✅ NewIcons: **Working** (IM1= format detected correctly)
- ✅ Icon positioning and saving: **All working**

#### Error Handling:
- Error code 232 (ERROR_NO_MORE_ENTRIES): Normal "no matches" condition
- `MatchEnd()` called on all code paths (success, error, early return)
- Proper cleanup prevents AmigaDOS memory leaks

#### Documentation Created:
- `docs/PATTERN_MATCHING_OPTIMIZATION.md` - Design rationale and API documentation
- `docs/PATTERN_MATCHING_INSTRUCTIONS.md` - Implementation guide
- `docs/PATTERN_MATCHING_IMPLEMENTATION.c` - Complete reference implementation

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
10. **Pattern matching optimization for directory scanning (40x faster)**

### 🔧 Technical Achievements
- Zero memory leaks in icon array management
- Proper AmigaDOS file I/O with BPTR handles
- GadTools GUI with synchronized gadget states
- Robust error handling with detailed logging
- Clean separation between layout calculation and icon saving
- **Filesystem-level pattern matching (MatchFirst/MatchNext API)**
- **Optimized for large directories with minimal .info files**
- **Proper resource cleanup with MatchEnd() preventing memory leaks**

### 📊 Testing Results
- **Test Directory 1**: PC: (26 icons)
  - Sort Mode: Name, Mixed priority
  - Layout Mode: Classic preset (55% width)
  - Screen Resolution: 800x600 (PAL)
  - Result: All icons positioned correctly, no overlapping, proper centering
  
- **Test Directory 2**: Work:WHDLoad/GamesOCS (600+ subdirectories, 0 icons)
  - Performance: **Instant response** (previously ~4 seconds)
  - Pattern Matching: ERROR_NO_MORE_ENTRIES returned immediately
  - Result: **40x performance improvement**
  
- **Test Directory 3**: Work:WHDLoad/GamesOCS/1000ccTurbo (2 icons)
  - Icons Detected: ReadMe.info (Standard), 1000ccTurbo.info (NewIcon)
  - Pattern Matching: Both icons found and processed correctly
  - Result: All functionality preserved with optimization

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

---

### Phase 7: Pattern Matching Bug Fixes (Volume Root & Empty Names)

#### Problem 1: Pattern Matching Failed on Volume Roots
- **Issue**: When scanning root directories like `workbench:`, pattern matching returned ERROR_OBJECT_NOT_FOUND (205), finding 0 icons despite .info files existing
- **Root Cause**: Pattern building always added "/" separator: `workbench:/#?.info`
  - AmigaDOS volume roots use syntax `volume:` (no trailing slash)
  - Adding "/" creates invalid pattern trying to match subdirectory named "/"
  - Correct root pattern: `workbench:#?.info` (no slash between : and #)
  - Correct subdir pattern: `workbench:Prefs/#?.info` (slash separates path and pattern)

#### Solution: Dynamic Pattern Building
**Implementation:**
```c
size_t pathLen = strlen(dirPath);
if (pathLen > 0 && dirPath[pathLen - 1] == ':')
{
    /* Root directory (volume) - no slash needed */
    snprintf(pattern, sizeof(pattern), "%s#?.info", dirPath);
}
else
{
    /* Subdirectory - needs slash separator */
    snprintf(pattern, sizeof(pattern), "%s/#?.info", dirPath);
}
```

**Pattern Examples:**
- `workbench:#?.info` - ✅ Matches icons in Workbench: root
- `workbench:/#?.info` - ❌ Tries to match subdirectory "/"
- `Work:WHDLoad/#?.info` - ✅ Matches icons in Work:WHDLoad/
- `DH0:Utilities/#?.info` - ✅ Matches icons in DH0:Utilities/

#### Problem 2: Blank Icon Displayed in Workbench Root
- **Issue**: After fixing volume root pattern, a blank icon (no name/text) appeared in the window
- **Investigation**: Debug logs revealed file `.info` (literally just dot + .info extension)
  - Full path: `Workbench:.info`
  - Text dimensions: width=0, height=0
  - Result: Invisible/blank icon in window that couldn't be seen to delete
- **Root Cause**: Pattern `#?.info` matches ANY filename ending in .info, including `.info`
  - This is a hidden/corrupted icon file
  - After removing .info extension, filename becomes just "." or empty string
  - Icon displays with zero text dimensions = blank icon

#### Solution: Empty/Dot-Only Name Filter
**Implementation:**
```c
removeInfoExtension(fib->fib_FileName, fileNameNoInfo);

/* Skip icons with empty names or names that are just a dot */
if (fileNameNoInfo[0] != '\0' &&  /* Not empty */
    !(fileNameNoInfo[0] == '.' && fileNameNoInfo[1] == '\0'))  /* Not just "." */
{
    /* Process icon normally */
}
else
{
    append_to_log("DEBUG: Skipping icon with empty or dot-only name: '%s'\n", 
                  fib->fib_FileName);
}
```

**Filtered Cases:**
- `.info` → Empty string after removing .info → Skipped
- Files that become just "." after extension removal → Skipped
- Normal icons like `Devs.info` → "Devs" → Processed

#### Testing Results
**Before Fix:**
```
Pattern: 'workbench:/#?.info'
MatchFirst returned 205 (ERROR_OBJECT_NOT_FOUND)
Icon count: 0
```

**After Volume Root Fix:**
```
Pattern: 'Workbench:#?.info'
MatchFirst successful
Found 17 icons including blank '.info' file
```

**After Empty Name Filter:**
```
Pattern: 'Workbench:#?.info'
MatchFirst successful
DEBUG: Skipping icon with empty or dot-only name: '.info'
Found 16 valid icons (blank icon filtered out)
```

#### Exclusion Filter Summary
The pattern matching now has 4 exclusion filters:
1. **Disk.info** - Volume icons (`isIconTypeDisk()` returns 1)
2. **Left Out Icons** - Workbench backdrop icons (`isIconLeftOut()` checks WBConfig.prefs)
3. **Empty/Dot-Only Names** - Hidden/corrupted files (new filter)
4. **Corrupted Icons** - Invalid .info files (`IsValidIcon()` fails)

#### Files Modified:
- `src/icon_management.c` (CreateIconArrayFromPath):
  - Lines 206-220: Added volume root detection and conditional pattern building
  - Lines 330-348: Added empty/dot-only name filter with debug logging
- `docs/DEVELOPMENT_LOG.md` - This section

#### Performance Impact:
- No performance degradation
- Volume roots now work with same 40x speed improvement as subdirectories
- Additional filter is simple string check (minimal overhead)

---

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

### 6. Performance Optimization Strategies
- **Always consider filesystem-level filtering before userspace filtering**
- AmigaDOS pattern matching (MatchFirst/MatchNext) available since Kickstart 2.0
- Pattern `#?.info` filters at filesystem level (40x faster than Examine/ExNext)
- Must call `MatchEnd()` on ALL code paths to prevent memory leaks
- `AnchorPath` more efficient than separate FileInfoBlock allocation
- Indentation and code structure critical - compiler errors can be misleading

### 7. Directory Scanning Best Practices
- Use pattern matching when you know the file extension/pattern
- Avoid examining ALL entries when only specific files needed
- Filesystem-level filtering reduces I/O dramatically
- Especially important on slow media (floppy disks, network drives)
- Pattern matching returns files only - directories require separate detection

---

## Version History

### v0.1 (Current Development Build)
- Initial implementation of dynamic layout system
- GUI with cycle gadgets for sort options
- Comprehensive debug logging
- Icon centering for wide text labels
- Log file management
- Default folder configuration
- **Pattern matching optimization (40x performance improvement)**
- Support for NewIcons, OS3.5 ColorIcons, and Standard Workbench icons

---

## Contributors
- Development: Kwezza
- AI Assistance: GitHub Copilot

---

*Last Updated: October 20, 2025*
