# Icons and Workbench Settings - Dimension Calculation Guide

## Overview

This document explains how Workbench icon dimensions are calculated and how iTidy must account for emboss borders and visual rendering to achieve pixel-perfect spacing.

---

## The Three Stages of Icon Dimensions

### 1. Raw Icon (from IconEdit)
- **Size**: 44×25 pixels (example icon used in testing)
- **Source**: The actual bitmap stored in the `.info` file
- **Location**: Read directly from the DiskObject image data

### 2. After Emboss Applied (icon_width/icon_height)
The icon.library adds the **embossRectangleSize** from Workbench Preferences to the raw dimensions when loading the icon:

```c
icon_width = raw_width + embossRectangleSize
icon_height = raw_height + embossRectangleSize
```

**Example calculations:**

| Emboss Setting | embossRectangleSize | Raw Icon | icon_width × icon_height |
|----------------|---------------------|----------|--------------------------|
| Small          | 1                   | 44×25    | 45×26                    |
| Medium         | 2                   | 44×25    | 46×27                    |
| Large          | 3                   | 44×25    | 47×28                    |

These are the values stored in `newIcon.icon_width` and `newIcon.icon_height` in iTidy's code.

### 3. Visual Rendering (icon_max_width/icon_max_height)
Workbench adds **additional border pixels** when rendering the icon on screen. This visual border equals the **embossRectangleSize** again:

```c
visual_width = icon_width + embossRectangleSize
visual_height = icon_height + embossRectangleSize
```

**Complete calculation chain:**

| Emboss Setting | Raw Icon | + emboss (load) | + visual border | Final Visual Size | Measured in WB |
|----------------|----------|-----------------|-----------------|-------------------|----------------|
| Large (3)      | 44×25    | 47×28           | 50×31           | **50×31**         | ✓ 50×31        |
| Medium (2)     | 44×25    | 46×27           | 48×29           | **48×29**         | ✓ 48×29        |
| Small (1)      | 44×25    | 45×26           | 46×27           | **46×27**         | ✓ 46×27        |

---

## Critical Discovery: The Formula

The visual border addition is **NOT a constant +3 pixels**. It is **dynamic** and equals the `embossRectangleSize` (which is stored in iTidy as `border_width`).

### Old iTidy Formula (FIXED - November 25, 2025)
```c
// This only worked correctly for Large borders (embossRectangleSize=3)
newIcon.icon_max_width = MAX(newIcon.icon_width + 3, textExtent.te_Width);
newIcon.icon_max_height = newIcon.icon_height + 3 + GAP_BETWEEN_ICON_AND_TEXT + textExtent.te_Height;
```

### Current Formula (✅ IMPLEMENTED - Works for All Border Sizes)
```c
// Use the dynamic border_width value (equals embossRectangleSize or 0 for frameless icons)
newIcon.icon_max_width = MAX(newIcon.icon_width + newIcon.border_width, textExtent.te_Width);
newIcon.icon_max_height = newIcon.icon_height + newIcon.border_width + GAP_BETWEEN_ICON_AND_TEXT + textExtent.te_Height;
```

Where:
- `border_width` = `embossRectangleSize` from Workbench Preferences (1, 2, or 3) for framed icons
- `border_width` = `0` for per-icon frameless icons (detected via `ICONCTRLA_GetFrameless`)
- `icon_width`/`icon_height` = raw icon dimensions + embossRectangleSize (already applied by icon.library)
- `GAP_BETWEEN_ICON_AND_TEXT` = 1 pixel (measured actual gap between icon image and text)

---

## Testing Data

### Test Setup
- **Test icons**: 8 icons in `PC:testfolder`
- **Raw icon size** (from IconEdit): 44×25 pixels
- **Target spacing**: 0 pixels horizontal and vertical
- **Font**: XEN.font size 8
- **Text height**: 8 pixels
- **Gap between icon and text**: 1 pixel (measured)

### Test Results

#### Large Border (embossRectangleSize=3)
- **Log shows**: Width=47, Height=28, Border=3, MaxW=50, MaxH=40
- **Visual measurement**: 50×31 pixels
- **Formula result**: 47+3=50 ✓ (CORRECT with hardcoded +3)
- **Icon positions**: X=0,50,100 and Y=0,40,80 (perfect edge-to-edge)

#### Small Border (embossRectangleSize=1)
- **Log shows**: Width=45, Height=26, Border=1, MaxW=48, MaxH=38
- **Visual measurement**: 46×27 pixels
- **Formula result**: 45+3=48 ✗ (WRONG - adds 2 extra pixels)
- **Correct result should be**: 45+1=46 ✓

#### Medium Border (embossRectangleSize=2)
- **Log shows**: Width=46, Height=27, Border=2, MaxW=49, MaxH=39
- **Visual measurement**: 48×29 pixels
- **Formula result**: 46+3=49 ✗ (WRONG - adds 1 extra pixel)
- **Correct result should be**: 46+2=48 ✓

---

## Per-Icon Frameless Detection

**Implementation**: iTidy detects individual frameless icons using `ICONCTRLA_GetFrameless` from icon.library (requires v44+).

**How it works**:
```c
// In src/icon_management.c, lines 563-566
if (iconDetails.hasFrame || newIcon.icon_type == icon_type_standard)
{
    newIcon.border_width = prefsWorkbench.embossRectangleSize;  // 1, 2, or 3
}
else
{
    newIcon.border_width = 0;  // Frameless icon - no extra border
}
```

**Result**: Folders can contain **mixed icons** (some with frames, some frameless), and each icon will be spaced correctly according to its individual `border_width` setting.

**Example**:
- Icon A: Has frame, emboss=2 → `border_width=2` → MaxW = icon_width + 2
- Icon B: Frameless → `border_width=0` → MaxW = icon_width + 0
- Both positioned correctly with 0-pixel spacing

**ColorIcon/NewIcon support**: Modern colorful icons can override the global border setting and specify frameless rendering on a per-icon basis. iTidy correctly detects and handles this.

---

iTidy reads the `embossRectangleSize` from Workbench Preferences at startup:

```c
// From src/Settings/WorkbenchPrefs.h
struct WorkbenchSettings {
    BOOL borderless;              // "No borders" setting
    int embossRectangleSize;      // 1=Small, 2=Medium, 3=Large
    int maxNameLength;
    BOOL newIconsSupport;
    BOOL colorIconSupport;
    // ... other fields
};

// Global variable (loaded at startup)
extern struct WorkbenchSettings prefsWorkbench;
```

The `border_width` field in `FullIconDetails` is populated from `prefsWorkbench.embossRectangleSize` during icon loading.

---

## Icon Dimension Fields in iTidy

### FullIconDetails Structure
```c
typedef struct {
    // Raw dimensions after emboss applied (from icon.library)
    int icon_width;       // raw_width + embossRectangleSize
    int icon_height;      // raw_height + embossRectangleSize
    
    // Visual dimensions (for spacing calculations)
    int icon_max_width;   // icon_width + border_width
    int icon_max_height;  // icon_height + border_width + gap + text_height
    
    // Text dimensions (from TextExtent)
    int text_width;
    int text_height;
    
    // Border setting from Workbench Prefs
    int border_width;     // equals embossRectangleSize (1, 2, or 3)
    
    // ... other fields
} FullIconDetails;
```

---

## Borderless Icons (Known Limitations)

**Testing completed**: When `Borderless=Yes` in Workbench Preferences, icons are rendered without the 3D embossed frame, revealing transparent edges in the icon bitmap.

**Configuration tested**:
- **Borderless**: Yes
- **Emboss Rectangle Size**: 1
- **Log shows**: Width=45, Height=26, Border=1, MaxW=48, MaxH=38
- **Visual measurement**: 38×22 pixels (visible opaque pixels only)

**The transparency issue**:

Standard Amiga icons support transparency via a **mask plane**. Some icons (particularly from Workbench 3.2 community set) have transparent borders/corners:
- Full bitmap size: 44×25 pixels
- Visible opaque pixels: ~38×22 pixels
- Transparent padding: ~3 pixels horizontal edges, ~1-2 pixels vertical edges

When borders are **enabled**, the 3D embossed frame **hides** these transparent areas. When `Borderless=Yes`, the transparent areas are revealed, making the visual icon size **smaller** than the bitmap dimensions.

**Current behavior**:
iTidy calculates spacing based on full bitmap dimensions (44×25 + emboss), which includes the transparent areas. This results in visible gaps between icons in borderless mode because the formula doesn't account for transparency.

**Example with current formula**:
- MaxW = 48 (icon_width 45 + hardcoded 3)
- Icons positioned at X=0, 48, 96
- Visible icon width = 38 pixels
- **Gap between visible areas** = 48 - 38 = 10 pixels

**Inconsistent icon implementations**:
Testing revealed that the Workbench 3.2 community icon set is **inconsistent** - some icons have transparent corners applied, others don't. This suggests the icon set is still a work-in-progress.

**Potential solution (NOT IMPLEMENTED)**:
To achieve perfect edge-to-edge spacing with borderless transparent icons, iTidy would need to:
1. Parse the icon mask plane to detect actual opaque pixel bounds
2. Calculate MaxW/MaxH based on visible pixels, not full bitmap size

**Decision**: This complexity is **not worth implementing** for v2.0 because:
- Borderless mode is rarely used
- Icon transparency is inconsistently implemented in community icon sets
- Current behavior still looks "tidy enough" with slight gaps
- Perfect spacing works correctly for all bordered modes (Small/Medium/Large)

**Recommended formula for borderless** (simpler, approximate):
```c
if (prefsWorkbench.borderless) {
    // Borderless: Use icon dimensions as-is (no extra visual border)
    icon_max_width = MAX(icon_width, text_width);
    icon_max_height = icon_height + GAP_BETWEEN_ICON_AND_TEXT + text_height;
} else {
    // Normal borders: Add visual border equal to emboss size
    icon_max_width = MAX(icon_width + border_width, text_width);
    icon_max_height = icon_height + border_width + GAP_BETWEEN_ICON_AND_TEXT + text_height;
}
```

This would reduce gaps from ~10px to ~7px, but still not perfect. True edge-to-edge requires mask parsing.

**Status**: Documented for future research. Not a priority for current release.

---

## Implementation Location

The dimension calculation is in `src/icon_management.c`, function `GetIconDetailsFromDisk()`, lines 581-585:

```c
// ✅ IMPLEMENTED (November 25, 2025)
newIcon.text_width = textExtent.te_Width;
newIcon.text_height = textExtent.te_Height;
/* Workbench adds visual border equal to embossRectangleSize (or 0 for frameless icons) */
newIcon.icon_max_width = MAX(newIcon.icon_width + newIcon.border_width, textExtent.te_Width);
newIcon.icon_max_height = newIcon.icon_height + newIcon.border_width + GAP_BETWEEN_ICON_AND_TEXT + textExtent.te_Height;
```

The `border_width` field is set earlier in the same function (lines 563-566) based on per-icon frameless detection.

---

## Summary of Key Learnings

1. **Icon dimensions are calculated in two stages**:
   - Stage 1: icon.library adds `embossRectangleSize` when loading the icon
   - Stage 2: Workbench adds `embossRectangleSize` again when rendering visually

2. **The visual border is NOT constant** - it varies with the emboss setting:
   - Small border = +1 pixel
   - Medium border = +2 pixels
   - Large border = +3 pixels
   - Frameless = +0 pixels (per-icon override)

3. **The hardcoded `+3` only worked for Large borders** - it added too many pixels for Small and Medium settings, causing gaps instead of edge-to-edge spacing.

4. **The `border_width` field contains the correct dynamic value** - iTidy now uses it instead of the hardcoded `+3`.

5. **Text gap is 1 pixel**, not 2 - this was measured and confirmed in testing.

6. **Perfect 0-pixel spacing is achievable** with the corrected formula for all bordered modes (Small/Medium/Large).

7. **Per-icon frameless detection works** - iTidy uses `ICONCTRLA_GetFrameless` to detect individual frameless icons and sets `border_width=0` for them.

8. **Mixed icon folders are supported** - Folders can contain both framed and frameless icons, each spaced correctly.

---

## Implementation Status

1. ✅ **COMPLETED**: Documented findings from testing Small, Medium, and Large border settings
2. ✅ **COMPLETED**: Updated `src/icon_management.c` to use dynamic `border_width` instead of hardcoded `+3`
3. ✅ **COMPLETED**: Tested with Borderless=Yes and documented transparency limitations
4. ✅ **COMPLETED**: Verified per-icon frameless detection via `ICONCTRLA_GetFrameless`
5. ✅ **COMPLETED**: Formula now works correctly for all border configurations (Small/Medium/Large/Per-icon-frameless)
6. ⚠️ **LIMITATION**: Global borderless mode with transparent icons has ~7px gaps (mask parsing not implemented)

---

## Log References

- **Large border test**: `general_2025-11-25_14-02-02.log` (actually shows embossRectangleSize=1, was mislabeled)
- **Small border test**: `general_2025-11-25_14-02-02.log` (embossRectangleSize=1)
- **Medium border test**: `general_2025-11-25_14-07-11.log` (embossRectangleSize=2)

---

**Document created**: November 25, 2025  
**Author**: Kerry Thompson  
**Based on**: Empirical testing with precise pixel measurements and log analysis
