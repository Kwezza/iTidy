# Window Layout & Aspect Ratio Feature Design

**Date:** October 19, 2025  
**Status:** Proposed Feature  
**Author:** Development Discussion

---

## Table of Contents
1. [Current Behavior Analysis](#current-behavior-analysis)
2. [The Problem](#the-problem)
3. [Proposed Solution](#proposed-solution)
4. [Implementation Details](#implementation-details)
5. [User Interface Design](#user-interface-design)

---

## Current Behavior Analysis

### How Icon Layout Currently Works

The current implementation in `src/icon_management.c` (function `ArrangeIcons()`) uses the following algorithm:

#### 1. Starting Parameters
- **Target Width:** 320 pixels (hard-coded in `window_management.c` line 50)
- **Screen Width:** Retrieved from Workbench screen (e.g., 640px for PAL, 1920px for RTG)
- **Icon Spacing:** 9px horizontal, 7px vertical (Workbench 3.1.4+)
- **Starting Margins:** 10px left (ICON_START_X), 10px top (ICON_START_Y)
- **Padding:** 4px width, 4px height

#### 2. Maximum Window Width Calculation
```c
maxWindowWidth = screenWidth - prefsIControl.currentBarWidth - 
                 prefsIControl.currentLeftBarWidth - (PADDING_WIDTH * 2);
```
This respects screen dimensions and window borders, leaving room for Workbench elements.

#### 3. Initial Icons Per Row Calculation
```c
iconsPerRow = MAX((newWidth - ICON_START_X) / (largestIconWidth + iconSpacingX), minIconsPerRow);
```
- Starts with target width (320px)
- Divides by icon width + spacing
- Enforces minimum of **3 icons per row**

#### 4. Dynamic Expansion (Current Aspect Ratio Logic)
```c
while (windowWidth < maxWindowWidth && (totalIcons / iconsPerRow) > (iconsPerRow / 2))
{
    iconsPerRow++;
    windowWidth = ICON_START_X + iconsPerRow * (largestIconWidth + iconSpacingX);
}
```

**What this does:**
- Expands window width by adding more icons per row
- **Key condition:** `(totalIcons / iconsPerRow) > (iconsPerRow / 2)`
  - This means: "Keep expanding if rows > columns × 2"
  - Creates a **2:1 aspect ratio preference** (tall windows)
  - Example: 12 icons → prefer 4 cols × 3 rows over 3 cols × 4 rows

#### 5. Safety Cap
```c
if (windowWidth > maxWindowWidth)
{
    windowWidth = maxWindowWidth;
    iconsPerRow = MAX((maxWindowWidth - ICON_START_X) / (largestIconWidth + iconSpacingX), minIconsPerRow);
}
```
Ensures window never exceeds available screen space.

### Current Target Shape
- **Preferred aspect ratio:** 2:1 (height:width) - Tall, narrow windows
- **Works well for:** Classic Amiga screens (640×512, 640×400)
- **Initial width target:** 320px (half of PAL width)

---

## The Problem

### Issue: Poor Behavior on RTG/High-Resolution Screens

The current algorithm has significant usability issues on modern RTG screens:

#### Scenario 1: Wide RTG Screen with Many Icons
**Setup:**
- Screen: 1920×1080 (RTG)
- Icons: 30 small icons
- Current behavior:
  - Target starts at 320px (way too small for screen)
  - Expansion loop allows up to ~1800px wide (almost full screen!)
  - Could result in: **1 row × 30 icons** = Extremely wide, single-row window

**Problems:**
- Window is absurdly wide relative to screen
- Single row of icons looks unbalanced
- User must scroll horizontally on some setups
- Poor use of vertical screen space

#### Scenario 2: Mixed Screen Sizes
**Setup:**
- User has both PAL (640×512) and RTG (1280×1024) systems
- Same icon collection on both
- Current behavior creates drastically different layouts

**Problems:**
- No consistent experience across systems
- 320px is too narrow for RTG, but reasonable for PAL
- Aspect ratio logic doesn't scale with screen resolution

### Root Cause Analysis

The current approach has **no upper limit on icons per row**, which causes:

1. **Percentage Problem:** Simple percentage of screen (e.g., 50%) would still allow:
   - 960px window on 1920px screen
   - Potentially 15-30 icons in a single row (depending on icon size)

2. **No Maximum Icons Per Row:** The algorithm only has a minimum (3), no maximum

3. **Fixed Starting Width:** 320px is not screen-resolution aware

4. **Aspect Ratio Not User-Configurable:** The 2:1 tall preference is hard-coded

---

## Proposed Solution

### Multi-Factor Smart Layout System

Instead of just aspect ratio OR width percentage, use a **combination of intelligent constraints** that work together.

### Key Design Principles

1. **Screen-Aware Starting Width**
   - Scale initial width based on screen resolution
   - Use percentage with sensible min/max bounds

2. **Maximum Icons Per Row Limit**
   - **This is the secret sauce** that prevents ultra-wide windows
   - Default: 8-12 icons per row (depending on layout preset)

3. **User-Configurable Aspect Ratio**
   - Replace hard-coded 2:1 with user preference
   - Support square, balanced, and tall layouts

4. **Preset System for Simplicity**
   - Encode good defaults into named presets
   - Advanced users can override individual parameters

---

## Implementation Details

### New Global Variables (in `main.c`)

```c
/* User layout preferences */
int user_maxIconsPerRow = 10;        // Prevent super-wide windows (default: balanced)
float user_aspectRatio = 0.5;        // Target ratio: rows/cols (0.5 = 2:1 tall, 1.0 = square)
int user_maxWidthPercent = 50;       // Percentage of screen width (default: 50%)
int user_maxWidthAbsolute = 0;       // Absolute pixel limit (0 = use percentage)
```

### Modified Algorithm (in `icon_management.c`)

#### Step 1: Calculate Smart Starting Width
```c
/* Calculate starting width based on screen resolution */
if (user_maxWidthAbsolute > 0)
{
    newWidth = MIN(user_maxWidthAbsolute, maxWindowWidth);
}
else
{
    newWidth = (screenWidth * user_maxWidthPercent) / 100;
    newWidth = MIN(newWidth, maxWindowWidth);
    newWidth = MAX(newWidth, 320);  // Minimum reasonable width
}
```

#### Step 2: Initial Icons Per Row (unchanged)
```c
iconsPerRow = MAX((newWidth - ICON_START_X) / (largestIconWidth + iconSpacingX), minIconsPerRow);
```

#### Step 3: Modified Expansion Loop
```c
/* Expand window width to fit more icons if necessary and possible */
while (windowWidth < maxWindowWidth && 
       iconsPerRow < user_maxIconsPerRow &&  // NEW: Prevent super-wide windows
       (totalIcons / iconsPerRow) > (iconsPerRow * user_aspectRatio))  // NEW: User-configurable
{
    iconsPerRow++;
    windowWidth = ICON_START_X + iconsPerRow * (largestIconWidth + iconSpacingX);
}
```

**Key Changes:**
- Added `iconsPerRow < user_maxIconsPerRow` condition
- Changed aspect ratio check to use `user_aspectRatio` variable
- Preserves existing safety caps

---

## User Interface Design

### Recommended Approach: Simple Presets + Advanced Options

### Simple Presets (Primary User Interface)

These encode sensible defaults for common use cases:

#### `-layoutCompact`
**Best for:** RTG screens, small folders, modern look
- Max icons per row: 8
- Aspect ratio: 1.0 (square, 1:1)
- Max width: 40% of screen
- Example: 20 icons → 8 cols × 3 rows

#### `-layoutBalanced` (Default)
**Best for:** General use, mixed screens
- Max icons per row: 10
- Aspect ratio: 0.67 (balanced, 3:2)
- Max width: 50% of screen
- Example: 20 icons → 7 cols × 3 rows

#### `-layoutClassic`
**Best for:** PAL screens, traditional Amiga look
- Max icons per row: 15
- Aspect ratio: 0.5 (tall, 2:1 - current behavior)
- Max width: 60% of screen
- Example: 20 icons → 5 cols × 4 rows

#### `-layoutWide`
**Best for:** Wide screens, many icons
- Max icons per row: 12
- Aspect ratio: 1.5 (wide, 3:2 landscape)
- Max width: 60% of screen
- Example: 20 icons → 10 cols × 2 rows

### Advanced Options (Power Users)

These override individual parameters:

```
-maxPerRow:8          → Maximum icons per row (prevents wide windows)
-maxWidth:600         → Absolute pixel width limit (overrides percentage)
-maxWidthPct:50       → Percentage of screen width (0-100)
-aspectRatio:1:1      → Preferred aspect ratio (width:height)
                        Examples: 1:1 (square), 3:2 (balanced), 
                                  16:9 (wide), 2:3 (tall)
```

### Example Usage

```bash
# Use compact preset (good for RTG)
iTidy DH0:MyFolder -subdirs -layoutCompact

# Classic tall layout on PAL screen
iTidy Work:Projects -layoutClassic

# Custom: Square layout, max 6 icons per row
iTidy PC:Downloads -aspectRatio:1:1 -maxPerRow:6

# Absolute width limit for consistent sizing
iTidy SYS:Utilities -maxWidth:500
```

---

## Example Scenarios with New System

### Scenario 1: RTG Screen (1920×1080), 30 icons
**Using `-layoutCompact`:**
- Screen width: 1920px
- Max width: 40% = 768px (or capped at configured max)
- Max icons per row: 8
- Aspect ratio: 1.0 (square)
- **Result:** 8 cols × 4 rows = Clean, compact window (~640px × 480px)

**Using `-layoutWide`:**
- Max width: 60% = 1152px
- Max icons per row: 12
- Aspect ratio: 1.5 (wide)
- **Result:** 12 cols × 3 rows = Wide but reasonable (~960px × 360px)

### Scenario 2: PAL Screen (640×512), 12 icons
**Using `-layoutBalanced`:**
- Screen width: 640px
- Max width: 50% = 320px
- Max icons per row: 10
- Aspect ratio: 0.67 (balanced)
- **Result:** 4 cols × 3 rows = Traditional compact window

**Using `-layoutClassic`:**
- Max width: 60% = 384px
- Max icons per row: 15
- Aspect ratio: 0.5 (tall - current behavior)
- **Result:** 3-4 cols × 3-4 rows = Classic Amiga tall window

---

## Implementation Checklist

### Phase 1: Core Infrastructure
- [ ] Add new global variables to `main.c`
- [ ] Add extern declarations to `main.h`
- [ ] Modify `FormatIconsAndWindow()` to calculate smart starting width
- [ ] Update expansion loop in `ArrangeIcons()` with new conditions

### Phase 2: Command-Line Interface
- [ ] Add preset parsing (`-layoutCompact`, etc.) to `main.c`
- [ ] Add advanced option parsing (`-maxPerRow:X`, `-aspectRatio:X:Y`)
- [ ] Add validation for numeric inputs
- [ ] Update help text with new options

### Phase 3: Testing
- [ ] Test on PAL resolution (640×512)
- [ ] Test on RTG resolution (1920×1080, 1280×1024)
- [ ] Test with various icon counts (3, 10, 30, 100 icons)
- [ ] Test with different icon sizes (standard, large NewIcons)
- [ ] Test preset combinations

### Phase 4: Documentation
- [ ] Update README.md with new options
- [ ] Add examples to documentation
- [ ] Document preset behavior differences

---

## Benefits of This Approach

### For Users
✅ **RTG-friendly:** No more ultra-wide single-row windows  
✅ **Simple presets:** Most users just pick a preset and go  
✅ **Consistent:** Same preset gives similar results across screen sizes  
✅ **Flexible:** Power users can fine-tune every parameter  
✅ **Backwards compatible:** Default behavior can match current (classic preset)

### For Code
✅ **Minimal changes:** Builds on existing algorithm  
✅ **Maintains safety:** All existing bounds checks still work  
✅ **Extensible:** Easy to add new presets or parameters later  
✅ **Testable:** Each parameter can be tested independently

---

## Alternative Approaches Considered

### Option 1: Fixed Percentage Only
**Rejected because:** Doesn't solve the "one row of 30 icons" problem on wide screens.

### Option 2: Absolute Width Only
**Rejected because:** Not screen-resolution aware; same width looks wrong on PAL vs RTG.

### Option 3: Automatic Only (No User Control)
**Rejected because:** Different users have different preferences; some want compact, others want classic.

### Option 4: Complex Formula
**Rejected because:** Too complex for users to understand; presets are more user-friendly.

---

## Future Enhancements

### Potential Future Features
1. **Save layout preference per folder** (in .info file tooltype?)
2. **Different presets for different folder types** (e.g., auto-detect pictures folder → use wide layout)
3. **Dynamic adjustment based on icon count ranges** (few icons = compact, many = wider)
4. **GUI preference editor** (for Workbench integration)
5. **Remember last-used preset** in ENV: variable

### Monitoring
Track usage metrics (if feasible) to see which presets are most popular and refine defaults accordingly.

---

## References

### Related Code Files
- `src/icon_management.c` - Main layout algorithm (ArrangeIcons function)
- `src/window_management.c` - Window sizing and positioning
- `src/main.c` - Command-line parsing and global variables
- `src/main.h` - Constants and global declarations

### Related Issues/Discussions
- Initial observation: 320px hard-coded width doesn't scale to RTG screens
- Key insight: "User may not want one window that's super wide on RTG screen with just one row of many icons"
- Solution: Combine max-per-row limit with aspect ratio preferences

---

## Conclusion

This feature will significantly improve iTidy's usability on modern Amiga systems with RTG graphics cards while maintaining excellent behavior on classic PAL/NTSC systems. The preset system provides simplicity for most users while advanced options give power users full control.

The key innovation is **adding a maximum icons-per-row limit** combined with **screen-aware width calculation** and **user-configurable aspect ratios**. This prevents the extreme cases (ultra-wide single-row windows) while providing flexibility for different preferences and screen configurations.
