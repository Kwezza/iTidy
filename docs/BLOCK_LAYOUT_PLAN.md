# Block-Based Icon Layout by Workbench Type

## Feature Overview

A new layout mode that partitions icons into three visual blocks based on their Workbench icon type (`WBDRAWER`, `WBTOOL`, and everything else). Each block is laid out independently with its own optimal column count and centering, then stacked vertically with configurable gaps between blocks.

**Requested by:** User community  
**Target Version:** iTidy 2.1  
**Status:** Planning

---

## ⚠️ Implementation Flexibility Notice

**This document is a guide, not a rigid specification.**

The implementation details below represent the *intended approach* based on current understanding of the codebase. However, during actual implementation and testing, the agent should feel free to:

- **Adapt function signatures** if the existing API patterns suggest a better fit
- **Change data structure placement** if alignment or memory considerations require it
- **Modify the algorithm** if testing reveals edge cases or performance issues
- **Reorganise code across files** if it improves maintainability
- **Add helper functions** not listed here if they simplify the implementation
- **Adjust pixel values** (gaps, spacing) if visual testing shows better results
- **Rename identifiers** if clearer names emerge during development

**The goals are fixed; the methods are flexible:**

| Fixed (Must Achieve) | Flexible (Can Adapt) |
|---------------------|---------------------|
| Three blocks: Drawers → Tools → Other | Exact function names and signatures |
| Per-block optimal columns | Internal algorithm details |
| Narrower blocks centered in window | Helper function organisation |
| Configurable gap sizes (S/M/L) | Exact pixel values for gaps |
| Empty blocks collapse (no gap) | How collapsing is detected/handled |
| User enables via Advanced window | Exact gadget layout/positioning |
| "By: Type" renamed to avoid confusion | Final label wording |

**If something isn't working:** Don't spin wheels trying to match this document exactly. Adapt the approach, test it, and document what changed. The end result matters more than matching the plan.

---

## 📋 Logging Requirements (IMPORTANT)

**All new code must include logging statements** to aid debugging. The program runs on an Amiga emulator, and logs are the primary way to understand what the code is doing at runtime.

### Use the Existing Logging System

The project has an established logging system in `src/writeLog.c` and `src/writeLog.h`. **Do not create a new logging mechanism.**

```c
#include "writeLog.h"

/* Available log categories */
LOG_GENERAL    /* General program flow */
LOG_MEMORY     /* Memory operations */
LOG_GUI        /* GUI events */
LOG_ICONS      /* Icon processing - USE THIS FOR BLOCK LAYOUT */
LOG_BACKUP     /* Backup operations */

/* Log levels */
log_debug(LOG_ICONS, "Debug message: %s\n", value);
log_info(LOG_ICONS, "Info message: %d icons\n", count);
log_warning(LOG_ICONS, "Warning: %s\n", issue);
log_error(LOG_ICONS, "Error: %s\n", problem);
```

### What to Log in Block Layout Code

Add logging at these key points:

1. **Block partitioning:**
   ```c
   log_info(LOG_ICONS, "Partitioning %d icons by Workbench type\n", total);
   log_debug(LOG_ICONS, "  Drawers: %d, Tools: %d, Other: %d\n", 
             drawers->size, tools->size, other->size);
   ```

2. **Per-block layout calculation:**
   ```c
   log_debug(LOG_ICONS, "Block %d: %d icons, optimal columns=%d, width=%d\n",
             blockIndex, block->size, columns, width);
   ```

3. **Window width determination:**
   ```c
   log_info(LOG_ICONS, "Widest block width: %d, window content width: %d\n",
            widestBlockWidth, windowWidth);
   ```

4. **Block positioning:**
   ```c
   log_debug(LOG_ICONS, "Positioning block %d at Y=%d, centerOffset=%d\n",
             blockIndex, currentY, centerOffset);
   ```

5. **Gap insertion:**
   ```c
   log_debug(LOG_ICONS, "Adding %dpx gap after block %d\n", gapPixels, blockIndex);
   ```

6. **Empty block handling:**
   ```c
   log_debug(LOG_ICONS, "Skipping empty block %d (no gap added)\n", blockIndex);
   ```

7. **Final dimensions:**
   ```c
   log_info(LOG_ICONS, "Block layout complete: %dx%d content area\n", 
            totalWidth, totalHeight);
   ```

### Log File Location

Logs are written to `Bin\Amiga\logs\icons_YYYY-MM-DD_HH-MM-SS.log`. The AI agent can read these logs after running the program in the emulator to understand behaviour and debug issues.

### Why This Matters

- The Amiga emulator is the only place to test the actual code
- Printf/console output may not be visible from Workbench launches
- Logs persist and can be examined after crashes
- Detailed logs help identify exactly where things go wrong
- The AI agent can use log output to verify its implementation is working

---

## Visual Concept

```
┌─────────────────────────────────────────┐
│  Window Title                           │
├─────────────────────────────────────────┤
│                                         │
│   ┌───────┐  ┌───────┐  ┌───────┐      │  ← Block 1: Drawers (WBDRAWER)
│   │Folder1│  │Folder2│  │Folder3│      │    Centered within window width
│   └───────┘  └───────┘  └───────┘      │
│                                         │
│   ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   │  ← Gap (5/10/15 pixels)
│                                         │
│      ┌──────┐  ┌──────┐  ┌──────┐      │  ← Block 2: Tools (WBTOOL)
│      │Tool A│  │Tool B│  │Tool C│      │    Own optimal columns, centered
│      └──────┘  └──────┘  └──────┘      │
│      ┌──────┐  ┌──────┐                │
│      │Tool D│  │Tool E│                │
│      └──────┘  └──────┘                │
│                                         │
│   ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   │  ← Gap (5/10/15 pixels)
│                                         │
│    ┌────────┐  ┌────────┐              │  ← Block 3: Other (WBPROJECT, etc.)
│    │Project1│  │Project2│              │    Own optimal columns, centered
│    └────────┘  └────────┘              │
│                                         │
└─────────────────────────────────────────┘
```

---

## Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Block order** | Fixed: Drawers → Tools → Other | Matches typical Workbench organisation; keeps it simple |
| **Column calculation** | Per-block optimal columns | Each block looks polished; widest block sets window width |
| **Narrow block alignment** | Centered horizontally | Professional appearance; blocks don't hug left edge |
| **Empty blocks** | Collapse (no gap) | No wasted space when a folder has no Tools, for example |
| **Feature activation** | Explicit user selection | Not enabled by default; user must opt-in via Advanced window |
| **Gap sizes** | Small (5px), Medium (10px), Large (15px) | Three sensible presets; avoids complexity of custom values |

---

## Implementation Plan

### Phase 1: Data Structure Changes

#### 1.1 Add `workbench_type` to `FullIconDetails`

**File:** `src/itidy_types.h`

```c
typedef struct {
    /* ... existing fields ... */
    
    /* Workbench icon type (WBDRAWER, WBTOOL, WBPROJECT, etc.) */
    UBYTE workbench_type;
    
    /* Flags (BOOL = 2 bytes on Amiga) - keep at end */
    BOOL is_folder;
    BOOL is_write_protected;
} FullIconDetails;
```

**Note:** Place `workbench_type` (1 byte) before the BOOL fields to maintain proper 68k alignment.

#### 1.2 Populate `workbench_type` in folder scanner

**File:** `src/folder_scanner.c` - in `AddIconToArray()` function

After calling `GetIconDetailsFromDisk()`, copy the type:
```c
newIcon.workbench_type = iconDetails.workbenchType;
```

---

### Phase 2: Preference Changes

#### 2.1 Add new enumerations

**File:** `src/layout_preferences.h`

```c
/* Block grouping mode - how icons are visually grouped */
typedef enum {
    BLOCK_GROUP_NONE = 0,       /* Standard layout (no grouping) */
    BLOCK_GROUP_BY_TYPE = 1     /* Group by WBDRAWER/WBTOOL/other */
} BlockGroupMode;

/* Block gap size - vertical space between groups */
typedef enum {
    BLOCK_GAP_SMALL = 0,        /* 5 pixels */
    BLOCK_GAP_MEDIUM = 1,       /* 10 pixels */
    BLOCK_GAP_LARGE = 2         /* 15 pixels */
} BlockGapSize;

/* Pixel values for gap sizes */
#define BLOCK_GAP_SMALL_PX   5
#define BLOCK_GAP_MEDIUM_PX  10
#define BLOCK_GAP_LARGE_PX   15
```

#### 2.2 Add fields to `LayoutPreferences`

**File:** `src/layout_preferences.h`

```c
typedef struct {
    /* ... existing fields ... */
    
    /* Block grouping settings */
    BlockGroupMode blockGroupMode;    /* BLOCK_GROUP_NONE or BLOCK_GROUP_BY_TYPE */
    BlockGapSize blockGapSize;        /* SMALL, MEDIUM, or LARGE */
    
} LayoutPreferences;
```

#### 2.3 Add defaults and persistence

**File:** `src/layout_preferences.c`

- Set defaults: `blockGroupMode = BLOCK_GROUP_NONE`, `blockGapSize = BLOCK_GAP_MEDIUM`
- Add to preferences save/load functions

---

### Phase 3: Layout Engine Changes

#### 3.1 Create block partition function

**File:** `src/layout_processor.c`

```c
/**
 * Partition icons into three groups by Workbench type.
 * 
 * @param source      Input array with all icons
 * @param drawers     Output array for WBDRAWER icons (caller allocates)
 * @param tools       Output array for WBTOOL icons (caller allocates)
 * @param other       Output array for everything else (caller allocates)
 */
void PartitionIconsByWorkbenchType(
    const IconArray *source,
    IconArray *drawers,
    IconArray *tools,
    IconArray *other
);
```

Classification logic:
- `workbench_type == WBDRAWER` → drawers block
- `workbench_type == WBTOOL` → tools block
- Everything else (`WBPROJECT`, `WBDISK`, `WBGARBAGE`, etc.) → other block

#### 3.2 Create block layout function

**File:** `src/layout_processor.c`

```c
/**
 * Calculate layout positions for block-based grouping.
 * Each block gets its own optimal columns; widest block determines window width.
 * Narrower blocks are horizontally centered within the window.
 * 
 * ARCHITECTURE: Reuses existing positioning functions (CalculateLayoutPositions or
 * CalculateLayoutPositionsWithColumnCentering) by creating temporary IconArrays
 * for each block. This avoids code duplication and ensures consistent behavior.
 * 
 * @param iconArray   The full icon array (positions will be updated in place)
 * @param prefs       Layout preferences
 * @param outWidth    Output: calculated window content width
 * @param outHeight   Output: calculated window content height
 * @return TRUE on success
 */
static BOOL CalculateBlockLayout(
    IconArray *iconArray,
    const LayoutPreferences *prefs,
    int *outWidth,
    int *outHeight
);
```

**Algorithm (Reuses Existing Code):**

1. **Partition** icons into three index arrays (drawers, tools, other)
   - Store indices only, NOT full icon structs (avoids pointer corruption)

2. **For each non-empty block:**
   ```c
   // Create temporary IconArray with just this block's icons
   IconArray blockArray;
   blockArray.array = whd_malloc(blockSize * sizeof(FullIconDetails));
   blockArray.size = blockSize;
   
   // Copy icons from main array to block array (shallow copy of structs)
   for (j = 0; j < blockSize; j++) {
       blockArray.array[j] = iconArray->array[blockIndices[j]];
   }
   
   // Sort block using existing function
   SortIconArrayWithPreferences(&blockArray, prefs);
   
   // Calculate optimal columns for this block
   blockColumns = CalculateOptimalIconsPerRow(blockSize, avgWidth, avgHeight, ...);
   
   // Position icons using EXISTING battle-tested functions
   if (prefs->centerIconsInColumn) {
       CalculateLayoutPositionsWithColumnCentering(&blockArray, prefs, blockColumns);
   } else {
       CalculateLayoutPositions(&blockArray, prefs, blockColumns);
   }
   
   // Calculate block dimensions from positioned icons
   blockWidth = CalculateBlockWidth(&blockArray, prefs);
   blockHeight = CalculateBlockHeight(&blockArray, prefs);
   ```

3. **Determine window width** from widest block:
   ```c
   windowWidth = max(blockWidth[0], blockWidth[1], blockWidth[2]);
   ```

4. **Apply block-level transformations:**
   ```c
   currentY = prefs->iconSpacingY;
   
   for each block:
       // Calculate horizontal centering offset
       centerOffset = (windowWidth - blockWidth[i]) / 2;
       
       // Offset all icon positions in block by:
       // - Horizontal: centerOffset (centers narrow blocks)
       // - Vertical: currentY (stacks blocks)
       for each icon in blockArray:
           icon->icon_x += centerOffset;
           icon->icon_y += currentY;
       
       // Copy positioned icons back to main array at original indices
       for (j = 0; j < blockSize; j++) {
           iconArray->array[blockIndices[j]] = blockArray.array[j];
       }
       
       // Move Y down for next block
       currentY += blockHeight;
       if (more blocks follow)
           currentY += GetBlockGapPixels(prefs->blockGapSize);
   ```

5. **Return total dimensions** for window resize calculation

**Key Benefits:**
- **Zero code duplication** - uses existing positioning functions 100%
- **Consistent behavior** - column centering, text alignment, etc. work identically
- **Battle-tested** - reuses proven code from standard layouts
- **Maintainable** - bug fixes propagate to all layout modes
- **~180 lines** instead of ~440 lines

#### 3.3 Integrate into `ProcessSingleDirectory()`

**File:** `src/layout_processor.c`

In the layout calculation section, add a branch:

```c
if (prefs->blockGroupMode == BLOCK_GROUP_BY_TYPE)
{
    /* Use block-based layout */
    if (!CalculateBlockLayout(iconArray, prefs, &contentWidth, &contentHeight))
    {
        log_error(LOG_ICONS, "Block layout calculation failed\n");
        /* Fall back to standard layout */
        goto standard_layout;
    }
}
else
{
standard_layout:
    /* Existing standard layout code */
    SortIconArrayWithPreferences(iconArray, prefs);
    CalculateLayoutWithAspectRatio(iconArray, prefs, &finalColumns, &finalRows);
    /* ... etc ... */
}
```

---

### Phase 4: Window Resize Updates

**File:** `src/window_management.c`

Update `resizeFolderToContents()` to accept pre-calculated dimensions when block layout is used, rather than recalculating from icon positions.

---

### Phase 5: GUI Changes

#### 5.1 Advanced Window - Add Grouping Controls

**File:** `src/GUI/advanced_window.c`

Add a new row after "Align Icons Vertically":

```
[Grouping:]  [None | By Icon Type ▼]    [Gap:]  [Small | Medium | Large ▼]
```

**New gadget IDs:**
```c
#define GID_ADV_BLOCK_GROUPING   1021   /* Grouping mode cycle */
#define GID_ADV_BLOCK_GAP        1022   /* Gap size cycle */
```

**Label arrays:**
```c
static STRPTR block_grouping_labels[] = {
    "None",
    "By Icon Type",
    NULL
};

static STRPTR block_gap_labels[] = {
    "Small",
    "Medium",
    "Large",
    NULL
};
```

**Layout:** Use two-column pattern matching existing Advanced window style.

#### 5.2 Main Window - Rename "By: Type" to avoid confusion

**File:** `src/GUI/main_window.c`

Change the `sortby_labels[]` array entry from `"Type"` to `"Extension"`:

```c
static STRPTR sortby_labels[] = {
    "Name",
    "Extension",   /* Was "Type" - renamed to avoid confusion with block grouping */
    "Date",
    "Size",
    NULL
};
```

This clarifies that the existing "By" option sorts by file extension (e.g., `.iff`, `.txt`), not by Workbench icon type.

#### 5.3 Advanced Window Data Structure

**File:** `src/GUI/advanced_window.h`

```c
typedef struct {
    /* ... existing fields ... */
    
    /* Block grouping settings */
    UWORD block_grouping_selected;   /* 0=None, 1=By Icon Type */
    UWORD block_gap_selected;        /* 0=Small, 1=Medium, 2=Large */
    struct Gadget *block_grouping_cycle;
    struct Gadget *block_gap_cycle;
    
} AdvancedWindowData;
```

---

### Phase 6: Testing

#### Test Cases

1. **Empty folder** - Should handle gracefully (no crash)
2. **Only drawers** - Single block, no gaps
3. **Only tools** - Single block, no gaps
4. **Only projects** - Single block, no gaps
5. **Mixed types** - All three blocks with gaps
6. **One icon per block** - Each block gets 1 column
7. **Many icons per block** - Verify optimal column calculation
8. **Centering** - Verify narrower blocks center correctly
9. **Gap sizes** - Verify 5/10/15 pixel gaps render correctly
10. **Recursive processing** - Verify block layout works in subfolders
11. **Backup/restore** - Verify positions can be restored correctly

#### Performance Testing

- Test with 100+ icons to verify no performance regression
- Test on 68000 (A500) to verify acceptable speed

---

## Files to Modify

| File | Changes |
|------|---------|
| `src/itidy_types.h` | Add `workbench_type` field to `FullIconDetails` |
| `src/folder_scanner.c` | Populate `workbench_type` from disk object |
| `src/layout_preferences.h` | Add `BlockGroupMode`, `BlockGapSize` enums and fields |
| `src/layout_preferences.c` | Add defaults, save/load for new fields |
| `src/layout_processor.c` | Add `PartitionIconsByWorkbenchType()`, `CalculateBlockLayout()`, integrate into `ProcessSingleDirectory()` |
| `src/window_management.c` | Update `resizeFolderToContents()` for block dimensions |
| `src/GUI/advanced_window.h` | Add block grouping fields to window data struct |
| `src/GUI/advanced_window.c` | Add Grouping/Gap cycle gadgets |
| `src/GUI/main_window.c` | Rename "Type" to "Extension" in sortby labels |

---

## Future Enhancements (Out of Scope for v2.1)

- **Custom block order** - Let user drag blocks into preferred sequence
- **Per-block sorting** - Different sort criteria for each block
- **Custom gap sizes** - Pixel-level control instead of presets
- **Additional groupings** - By file extension, by date range, etc.
- **Block headers** - Optional text labels between blocks ("Folders", "Programs", etc.)

---

## References

- Workbench icon types defined in `<workbench/workbench.h>`:
  - `WBDISK` (1), `WBDRAWER` (2), `WBTOOL` (3), `WBPROJECT` (4), `WBGARBAGE` (5), `WBDEVICE` (6), `WBKICK` (7), `WBAPPICON` (8)
- Existing layout engine: `src/aspect_ratio_layout.c`, `src/layout_processor.c`
- Existing sorting: `SortIconArrayWithPreferences()` in `src/layout_processor.c`
- **Existing positioning functions (REUSE THESE):**
  - `CalculateLayoutPositions()` - Basic row-based positioning
  - `CalculateLayoutPositionsWithColumnCentering()` - Positioning with column-width centering
- Advanced window pattern: `src/GUI/advanced_window.c`

---

## Appendix: Implementation History & Lessons Learned

### Session 1: January 20, 2026 - Initial Implementation (ROLLED BACK)

**What Was Implemented:**
- Full block layout feature (Phases 1-5) with data structures, preferences, GUI, and layout engine
- Custom positioning logic within `CalculateBlockLayout()` function (~440 lines)
- Column centering logic duplicated from existing functions

**Issues Encountered:**

1. **Crash (Error 8100 0005)** - Pointer corruption
   - **Cause:** Attempted to copy `FullIconDetails` structs containing pointers into temporary arrays
   - **Fix:** Switched to index-based arrays instead of copying structs
   - **Lesson:** AmigaOS 68k requires careful pointer management

2. **Icon Positioning Bugs** - Multiple iterations of fixes
   - **Issue 1:** Icons overlapping - wrong X position formula
   - **Fix 1:** Changed from `col * width` to running accumulator
   - **Issue 2:** Wrong Y positions - using current icon height instead of row max height
   - **Fix 2:** Track `currentRowMaxHeight` across row
   - **Issue 3:** Column centering not working - missing centering logic
   - **Fix 3:** Added column width calculation and centering pass

3. **Code Duplication** - ~200 lines of duplicated positioning logic
   - Duplicated row-based positioning from `CalculateLayoutPositions()`
   - Duplicated column centering from `CalculateLayoutPositionsWithColumnCentering()`
   - Created maintenance burden - bug fixes wouldn't propagate

**Root Cause Analysis:**

The implementation tried to **reinvent positioning logic** instead of reusing existing battle-tested functions. This led to:
- Subtle bugs that had already been fixed in the original code
- Inconsistent behavior compared to standard layouts
- ~30-35% code duplication
- Risk of divergent behavior over time

**Why Refactor:**

The existing positioning code (`CalculateLayoutPositions` and `CalculateLayoutPositionsWithColumnCentering`) is **battle-hardened** through months of real-world use. It handles:
- Column width optimization
- Text alignment (TOP/MIDDLE/BOTTOM)
- Aspect ratio constraints
- Edge cases with various icon sizes
- Performance optimization for 1000+ icons

**Duplicating this logic was a mistake.** The correct approach is to **call the existing functions** on each block separately.

**Decision:**
- Roll back to last Git commit (before block layout implementation)
- Restart with new architecture that calls existing positioning functions
- Keep all the preparatory work (data structures, preferences, GUI) - those are fine
- Only change the `CalculateBlockLayout()` function to use existing code

**Estimated Code Reduction:**
- Before: ~440 lines in `CalculateBlockLayout()`
- After: ~180 lines (partition, per-block setup, call existing functions, offset positions)
- **Savings: ~260 lines of duplicated logic removed**

**Key Takeaway:**

> "Don't reinvent the wheel, especially when the wheel already handles edge cases you haven't discovered yet."

When working with legacy Amiga code, **reuse over reimplementation** is critical. The original code has survived real-world testing - duplicating it introduces regression risk.

---

## Appendix A: Testing Findings & Fixes

### The "Phantom Padding" Issue (January 2026)

During initial testing of the Block Layout implementation, a significant visual defect was identified where the window was excessively wide, leaving a large empty gap (~60px) on the right side.

#### Investigation
Debugging revealed that the issue stemmed from how the visual width of icons with text was calculated.
- Original code assumed: RightEdge = icon_x + icon_max_width
- This formula works for left-aligned text but fails for **centered text**.
- When text is wider than the icon, icon.library centers the text relative to the icon image (at icon_x).
- This means the text extends to both the left and right of the icon image.
- The original formula incorrectly assumed the text started at icon_x and extended fully to the right, effectively double-counting the left-side overhang as "phantom padding" on the right side.

#### The Fix
The logic was updated in two critical locations:
1. **CalculateBlockLayout usage in src/layout_processor.c** (for determining optimal block width)
2. **esizeFolderToContents in src/window_management.c** (for final window sizing)

**Corrected Formula:**
`c
if (text_width > icon_width) {
    // Text is centered on icon
    // Right Edge = Icon Center + Half Text Width
    RightEdge = icon_x + (icon_width + text_width) / 2;
} else {
    // Icon is wider (or equal)
    RightEdge = icon_x + icon_max_width;
}
`

This correction eliminated the empty space, resulting in a tight, professional layout that matches standard Workbench window sizing behavior.

## Appendix B: Block Centering Refinement (January 2026)

### Visual Balance Improvement

#### The Request
After fixing the "phantom padding" issue, the user requested that narrower blocks be horizontally centered relative to the widest block. In the initial implementation, all blocks were left-aligned at X=10, which looked unbalanced when one block was significantly wider than the others (e.g., a wide "Other" block of 3 columns vs a narrow "Drawers" block of 2 columns).

#### The Solution
Modified **Pass 3** of CalculateBlockLayout in src/layout_processor.c to include a horizontal centering offset.

**Algorithm:**
1. Determine the width of the widest block (maxBlockWidth).
2. For each block (lockIndex):
   - Calculate offset: centerOffset = (maxBlockWidth - blockWidths[blockIndex]) / 2
   - Apply this centerOffset to the icon_x coordinate of every icon in that block.

#### Result
Narrower blocks now sit perfectly centered above/below wider blocks, creating a symmetrical, professional appearance even when blocks have different column counts.
