# Window Positioning Offsets - Near Parent Mode

## Overview

This document tracks the empirically-determined offset values required for accurate "Near Parent" window positioning in iTidy. These offsets account for internal margins in Workbench drawer windows that affect icon positioning.

---

## Root Directory Windows (e.g., Work:, DH0:)

**Detection**: Path ends with `:` - use `IsRootFolder()` function

**Offset Applied**: Volume gauge width only

| Setting | Value | Notes |
|---------|-------|-------|
| Volume Gauge Width | `currentCGaugeWidth` (typically 19 pixels) | From IControl preferences |
| Left Content Margin | 0 | No additional margin beyond gauge |
| Top Content Margin | 0 | No additional margin |

**Formula**:
```c
contentAreaLeftOffset = prefsIControl.currentCGaugeWidth;  // 19 pixels
contentAreaTopOffset = 0;
```

---

## Non-Root Drawer Windows (e.g., Work:MyFolder)

**Detection**: Path does NOT end with `:` - subdirectory

**Offset Applied**: Margins based on emboss rectangle size

### Tested Configurations

**✅ TESTING COMPLETE - Formulas validated and implemented**

| Emboss Size | Left Margin | Top Margin | Status | Workbench Version | Notes |
|-------------|-------------|------------|--------|-------------------|-------|
| **Borderless** | **1 pixel** | **1 pixel** | ✅ Perfect | WB 3.2 | 2025-11-26: Fixed margins when borderless=TRUE |
| **1** (1-pixel) | **2 pixels** | **0 pixels** | ✅ Perfect | WB 3.2 | 2025-11-26: Formula confirmed pixel-perfect |
| **2** (2-pixel) | **4 pixels** | **2 pixels** | ✅ Perfect | WB 3.2 | 2025-11-26: Formula confirmed pixel-perfect |
| **3** (Normal) | **6 pixels** | **4 pixels** | ✅ Perfect | WB 3.2 | 2025-11-26: Formula confirmed pixel-perfect |
| **4** (Large) | **8 pixels** | **6 pixels** | ⚠️ Extrapolated | - | Formula predicts (8,6) - not tested |

**Validation Summary:**
- **Platform**: Workbench 3.2 (47.100)
- **Folder Type**: Non-root drawer (Work:My Files/Progs)
- **Test Results**: All tested configurations pixel-perfect
- **Formula Status**: Production-ready and implemented

**Test Details - Emboss Size 0 (Borderless):**
- **CRITICAL**: Borderless mode actually uses emboss=1 internally, NOT emboss=0!
- Borderless: Yes, Emboss reported: 1 (not 0!)
- Icon border width in .info: 1 pixel
- Parent Window: left=263, top=235, width=274, height=130
- Child Icon Position: x=168, y=18 (base size: 38×11)
- Window Chrome: leftBorder=4, titleBar=16
- Final Window Position: left=473, top=280
- Visual Offset: +1 horizontal (gap, too far right), -1 vertical (overlap)
- **Formula gives (2,0) for emboss=1, but borderless mode needs (1,1) instead!**
- **Hypothesis**: Borderless mode has different content area margins than regular emboss

**Test Details - Emboss Size 1:**
- Parent Window: left=263, top=235, width=274, height=130
- Child Icon Position: x=168, y=18 (base size: 38×11)
- Window Chrome: leftBorder=4, titleBar=16
- Final Window Position: left=473, top=280
- Visual Offset: -2 horizontal (overlapping), 0 vertical (perfect)
- **Required Correction**: +2 horizontal, +0 vertical
- **Formula Check**: emboss×2 = 1×2 = **2** ✅, (emboss-1)×2 = 0×2 = **0** ✅

**Test Details - Emboss Size 2:**
- Parent Window: left=263, top=235, width=274, height=130
- Child Icon Position: x=168, y=18 (base size: 38×11)
- Window Chrome: leftBorder=4, titleBar=16
- Final Window Position: left=473, top=280
- Visual Offset: -4 horizontal (overlapping), -2 vertical (overlapping)
- **Required Correction**: +4 horizontal, +2 vertical
- **Formula Check**: emboss×2 = 2×2 = **4** ✅, (emboss-1)×2 = 1×2 = **2** ✅

**Test Details - Emboss Size 3:**
- Parent Window: left=263, top=235, width=274, height=130
- Child Icon Position: x=168, y=18 (base size: 38×11)
- Window Chrome: leftBorder=4, titleBar=16
- Final Window Position: left=473, top=280
- Visual Offset: -6 horizontal (overlapping), -4 vertical (overlapping)
- **Required Correction**: +6 horizontal, +4 vertical
- **Formula Check**: emboss×2 = 3×2 = **6** ✅, (emboss-1)×2 = 2×2 = **4** ✅

### Formula

```c
// Check if borderless mode is enabled
if (prefsWorkbench.borderless) {
    // Borderless mode has fixed margins (empirically measured)
    contentAreaLeftOffset = 1;
    contentAreaTopOffset = 1;
} else {
    // Normal embossed mode - use formula based on emboss size
    // Horizontal margin = emboss × 2
    contentAreaLeftOffset = embossRectangleSize * 2;
    
    // Vertical margin = (emboss - 1) × 2
    contentAreaTopOffset = (embossRectangleSize - 1) * 2;
}
```

**Code Location**: `src/window_management.c`, lines 27-30 (macros)

**Note**: Borderless mode uses a fixed margin regardless of the stored emboss size value. When borderless is enabled, Workbench appears to ignore the emboss setting for rendering, resulting in different content area margins.

---

## Complete Position Calculation

### Near Parent Window Positioning Algorithm

When the user selects "Near Parent" positioning mode, iTidy calculates the child window position to appear at the **bottom-right corner** of the parent folder's icon.

#### Step-by-Step Calculation:

1. **Get Parent Window Geometry**: Read parent folder's `.info` file for window position/size
2. **Get Child Icon Details**: Read child folder's `.info` file from parent directory for icon position
3. **Detect Root vs Non-Root**: Check if parent path ends with `:` (e.g., `Work:`, `DH0:`)
4. **Calculate Offsets**: Apply appropriate content area margins based on root/borderless/emboss
5. **Compute Final Position**: Add all offset components together

---

### For Root Directories (e.g., Work:, DH0:)

**Detection**: Path ends with `:` using `IsRootFolder()` function

```c
// Root directories have volume gauge but no content area margins
volumeGaugeOffset = prefsIControl.currentCGaugeWidth;  // Typically 19 pixels
contentAreaLeftOffset = 0;
contentAreaTopOffset = 0;

// Calculate window position
windowLeft = parentWindow.left +                    // Parent window left edge
             currentLeftBarWidth +                   // Window chrome (4 px)
             volumeGaugeOffset +                     // Volume gauge (19 px)
             iconX +                                 // Icon X position in window
             iconWidth;                              // Base icon width (no emboss)

windowTop = parentWindow.top +                      // Parent window top edge
            currentTitleBarHeight +                  // Title bar (16 px)
            iconY +                                  // Icon Y position in window
            iconHeight;                              // Base icon height (no emboss)
```

**Example Calculation** (Work: root directory):
```
Parent Window: left=100, top=50, width=640, height=300
Child Icon: x=50, y=30, base_w=80, base_h=60
Window Chrome: leftBorder=4, titleBar=16
Volume Gauge: 19

windowLeft = 100 + 4 + 19 + 50 + 80 = 253
windowTop  = 50 + 16 + 30 + 60 = 156
```

---

### For Non-Root Drawer Windows (e.g., Work:MyFolder)

**Detection**: Path does NOT end with `:` (subdirectory)

```c
// Non-root drawers have content area margins based on emboss/borderless
volumeGaugeOffset = 0;  // No volume gauge in subdirectories

// Calculate content area margins
if (prefsWorkbench.borderless) {
    contentAreaLeftOffset = 1;  // Borderless mode: fixed margins
    contentAreaTopOffset = 1;
} else {
    contentAreaLeftOffset = embossRectangleSize * 2;        // Normal mode
    contentAreaTopOffset = (embossRectangleSize - 1) * 2;
}

// Calculate window position
windowLeft = parentWindow.left +                    // Parent window left edge
             currentLeftBarWidth +                   // Window chrome (4 px)
             contentAreaLeftOffset +                 // Drawer content margin
             iconX +                                 // Icon X position in window
             iconWidth;                              // Base icon width (no emboss)

windowTop = parentWindow.top +                      // Parent window top edge
            currentTitleBarHeight +                  // Title bar (16 px)
            contentAreaTopOffset +                   // Drawer content margin
            iconY +                                  // Icon Y position in window
            iconHeight;                              // Base icon height (no emboss)
```

**Example Calculation** (Work:Programs with emboss=3):
```
Parent Window: left=263, top=235, width=274, height=130
Child Icon: x=168, y=18, base_w=38, base_h=11
Window Chrome: leftBorder=4, titleBar=16
Emboss: 3 (normal), Borderless: No
Content Margins: left=6 (3×2), top=4 ((3-1)×2)

windowLeft = 263 + 4 + 6 + 168 + 38 = 479
windowTop  = 235 + 16 + 4 + 18 + 11 = 284
```

**Example Calculation** (Work:Programs with borderless enabled):
```
Parent Window: left=263, top=235, width=274, height=130
Child Icon: x=168, y=18, base_w=38, base_h=11
Window Chrome: leftBorder=4, titleBar=16
Borderless: Yes
Content Margins: left=1, top=1

windowLeft = 263 + 4 + 1 + 168 + 38 = 474
windowTop  = 235 + 16 + 1 + 18 + 11 = 281
```

---

### Key Variables Reference

| Variable | Source | Typical Value | Notes |
|----------|--------|---------------|-------|
| `parentWindow.left/top` | Parent `.info` file | Varies | Window position on screen |
| `iconX, iconY` | Child `.info` file in parent dir | Varies | Icon position WITHIN parent window |
| `iconWidth, iconHeight` | Child `.info` file | Varies | **Base size** (size.width/height) NOT iconWithEmboss |
| `currentLeftBarWidth` | IControl prefs | 4 | Window chrome left border |
| `currentTitleBarHeight` | IControl prefs | 16 | Window title bar height |
| `currentCGaugeWidth` | IControl prefs | 19 | Volume gauge width (root only) |
| `embossRectangleSize` | Workbench prefs | 1-4 | Icon emboss border size |
| `borderless` | Workbench prefs | TRUE/FALSE | Borderless icon mode |

---

### Critical Implementation Notes

1. **Use Base Icon Size**: MUST use `size.width` and `size.height`, NOT `iconWithEmboss` or `iconVisualSize`
2. **Borderless Detection**: Check `borderless` flag BEFORE using emboss size
3. **Root Detection**: Use `IsRootFolder(path)` - don't assume based on depth
4. **Margin Order**: Apply margins in the correct order: chrome → gauge/margin → icon → size
5. **Boundary Clamping**: If calculated position exceeds screen bounds, clamp to screen edges (don't fall back to center)

---

## Testing Notes

### Test Procedure

1. Set Workbench emboss size in Prefs/IControl
2. Snapshot parent icon to known position (use Icon Information)
3. Run iTidy on child folder with "Near Parent" mode
4. Measure pixel offset between expected and actual position
5. Adjust formulas and retest

### Known Issues

- Emboss size of 0 is untested - margin formula may produce negative values
- Large emboss sizes (>3) are untested
- Pattern may not hold for all Workbench versions

---

## System Values Reference

### IControl Preferences (from `prefsIControl`)

| Field | Typical Value | Purpose |
|-------|---------------|---------|
| `currentLeftBarWidth` | 4 | Window chrome left border |
| `currentBarWidth` | 18 | Window chrome right border |
| `currentTitleBarHeight` | 16 | Window title bar height |
| `currentBarHeight` | 16 | Window chrome bottom border |
| `currentCGaugeWidth` | 19 | Volume gauge width (root windows) |

### Workbench Preferences (from `prefsWorkbench`)

| Field | Typical Value | Purpose |
|-------|---------------|---------|
| `embossRectangleSize` | 3 | Icon emboss border size |

---

## Future Investigation

### Workbench Version Compatibility

**Tested:**
- ✅ **Workbench 3.2 (47.100)**: All formulas confirmed pixel-perfect
  - Tested emboss sizes: borderless, 1, 2, 3
  - Tested folder types: root directories, non-root drawers
  - Platform: WinUAE emulation

**Pending Testing:**
- 🔬 **Workbench 3.0**: Testing in progress (emboss settings may differ)
- ❓ **Workbench 3.1**: Not yet tested
- ❓ **Workbench 3.5**: Not yet tested (color icon support)
- ❓ **Workbench 3.9**: Not yet tested (NewIcons support)

### Questions to Answer

1. **Why the asymmetry?** 
   - Left margin = `emboss × 2`
   - Top margin = `(emboss - 1) × 2`
   - Why the `-1` for vertical but not horizontal?
   - **Hypothesis**: Top alignment compensation or text label spacing

2. **Workbench version differences?**
   - Do margins vary between WB 3.0, 3.1, 3.5, 3.9?
   - Are there differences in icon.library versions?
   - Does IControl prefs structure change across versions?

3. **Icon type variations?**
   - Do NewIcons (WB 3.5+) affect margins?
   - Do OS3.5 color icons affect margins?
   - Does frameless icon mode change anything?

4. **Large emboss sizes?**
   - Does emboss=4 follow the pattern? (predicted: 8, 6)
   - What's the maximum emboss size supported?
   - Do margins scale linearly beyond emboss=3?

---

## Version History

| Date | Author | Change |
|------|--------|--------|
| 2025-11-26 | Kerry Thompson | Initial documentation with emboss=3 and emboss=1 values |
| 2025-11-26 | Kerry Thompson | Complete empirical testing: emboss 1, 2, 3, borderless mode |
| 2025-11-26 | Kerry Thompson | Formula validation on Workbench 3.2 - all tests pixel-perfect ✅ |
| 2025-11-26 | Kerry Thompson | Production implementation with borderless detection |

---

## Related Files

- **Code**: `src/window_management.c` - Window positioning implementation
- **Macros**: `DRAWER_CONTENT_LEFT_MARGIN()`, `DRAWER_CONTENT_TOP_MARGIN()`
- **Function**: `IsRootFolder()` in `src/backup_paths.c`
- **Preferences**: `src/Settings/IControlPrefs.h`, `src/Settings/WorkbenchPrefs.h`
