# Complete Font System Implementation

**Status:** ✅ Complete  
**Date:** 2025-11-18  
**Build:** Successful

## Overview

Comprehensive font system that reads all three Amiga font types from `font.prefs` and caches them globally at startup for optimal performance and future expansion.

## Font Types Supported

From `prefs/font.h`:

1. **FP_WBFONT (0)** - Workbench Icon Text Font
   - Used by Workbench for icon labels
   - Stored in: `iconTextFontName`, `iconTextFontSize`, `iconTextFontCharWidth`

2. **FP_SYSFONT (1)** - System Font
   - Used by system GUI elements (currently ListViews)
   - Stored in: `systemFontName`, `systemFontSize`, `systemFontCharWidth`

3. **FP_SCREENFONT (2)** - Screen Text Font
   - Used for screen title bars and other screen text
   - Stored in: `screenTextFontName`, `screenTextFontSize`, `screenTextFontCharWidth`

## Architecture

### Data Structure

**File:** `src/Settings/IControlPrefs.h`

```c
struct IControlPrefsDetails {
    /* ... existing fields ... */
    
    /* System font information (for ListViews, etc.) */
    char systemFontName[256];           /* Font name from fontPrefs (icon font) */
    UWORD systemFontSize;               /* Font size (ta_YSize) */
    UWORD systemFontCharWidth;          /* Character width (tf_XSize) from opened font */
    
    /* Workbench icon text font (from workbench.prefs) */
    char iconTextFontName[256];         /* Icon text font name */
    UWORD iconTextFontSize;             /* Icon text font size (ta_YSize) */
    UWORD iconTextFontCharWidth;        /* Icon text character width (tf_XSize) */
    
    /* Screen text font (from workbench.prefs) */
    char screenTextFontName[256];       /* Screen text font name */
    UWORD screenTextFontSize;           /* Screen text font size (ta_YSize) */
    UWORD screenTextFontCharWidth;      /* Screen text character width (tf_XSize) */
};
```

### Font Reader

**File:** `src/Settings/IControlPrefs.c`

**New Function:** `read_font_prefs()`

```c
/**
 * @brief Read specific font type from font.prefs
 * @param font_type FP_WBFONT (0), FP_SYSFONT (1), or FP_SCREENFONT (2)
 * @param name_out Buffer to receive font name (256 bytes)
 * @param size_out Pointer to receive font size
 * @param char_width_out Pointer to receive character width
 * @return TRUE if font was read successfully, FALSE otherwise
 */
static BOOL read_font_prefs(UWORD font_type, char *name_out, 
                           UWORD *size_out, UWORD *char_width_out)
```

**Features:**
- Reads from `ENV:sys/font.prefs` or `ENVARC:sys/font.prefs`
- Parses IFF FORM structure
- Searches for `ID_FONT` chunks matching requested type
- Opens font with `OpenDiskFont()` to get actual `tf_XSize`
- Returns font metrics in provided buffers

**Algorithm:**
1. Open `ENV:sys/font.prefs` (or `ENVARC:` fallback)
2. Skip 12-byte FORM header
3. Read 8-byte chunk headers (ID + size)
4. When `ID_FONT` found:
   - Read `struct FontPrefs`
   - Check if `fp_Type` matches requested type
   - Extract `fp_Name` and `fp_TextAttr.ta_YSize`
   - Open font to get `tf_XSize`
   - Return metrics
5. Continue until font found or EOF

### Initialization

**Function:** `fetchIControlSettings()`

Called at startup from `main()`, reads all three font types:

```c
/* 1. System font (FP_SYSFONT) - used for ListViews */
if (!read_font_prefs(FP_SYSFONT, details->systemFontName, 
                    &details->systemFontSize, &details->systemFontCharWidth)) {
    /* Fallback to fontPrefs global or Topaz 8 */
}

/* 2. Workbench icon text font (FP_WBFONT) */
if (!read_font_prefs(FP_WBFONT, details->iconTextFontName,
                    &details->iconTextFontSize, &details->iconTextFontCharWidth)) {
    /* Fallback to system font */
}

/* 3. Screen text font (FP_SCREENFONT) */
if (!read_font_prefs(FP_SCREENFONT, details->screenTextFontName,
                    &details->screenTextFontSize, &details->screenTextFontCharWidth)) {
    /* Fallback to system font */
}
```

**Fallback Chain:**

For System Font:
1. `read_font_prefs(FP_SYSFONT)` from font.prefs
2. Global `fontPrefs` from `get_fonts.c`
3. Topaz 8 default

For Icon/Screen Fonts:
1. `read_font_prefs(FP_WBFONT/FP_SCREENFONT)` from font.prefs
2. Copy from System Font
3. Topaz 8 (via System Font fallback)

## Debug Logging

Enhanced `dumpIControlPrefs()` output:

```
Font Settings:
  System Font (FP_SYSFONT):
    Name: topaz.font, Size: 8, Char Width: 8
  Icon Text Font (FP_WBFONT):
    Name: helvetica.font, Size: 11, Char Width: 9
  Screen Text Font (FP_SCREENFONT):
    Name: courier.font, Size: 13, Char Width: 8
```

## Usage Examples

### ListView Fonts (Current Use)

**File:** `src/GUI/default_tool_restore_window.c`

```c
#include "../Settings/IControlPrefs.h"

/* Use cached system font */
listview_font.ta_Name = prefsIControl.systemFontName;
listview_font.ta_YSize = prefsIControl.systemFontSize;
font_char_width = prefsIControl.systemFontCharWidth;
```

### Icon Text Rendering (Future)

```c
/* Use cached icon text font */
struct TextAttr icon_font;
icon_font.ta_Name = prefsIControl.iconTextFontName;
icon_font.ta_YSize = prefsIControl.iconTextFontSize;
icon_font.ta_Style = FS_NORMAL;
icon_font.ta_Flags = 0;

UWORD char_width = prefsIControl.iconTextFontCharWidth;
```

### Screen Title Bars (Future)

```c
/* Use cached screen text font */
struct TextAttr screen_font;
screen_font.ta_Name = prefsIControl.screenTextFontName;
screen_font.ta_YSize = prefsIControl.screenTextFontSize;
screen_font.ta_Style = FS_NORMAL;
screen_font.ta_Flags = 0;

UWORD char_width = prefsIControl.screenTextFontCharWidth;
```

## Performance Benefits

**Before:** Each ListView window:
- Opened font.prefs
- Parsed IFF structure
- Opened font with `OpenDiskFont()`
- Read metrics
- Closed font
- **Cost:** File I/O + parsing + font opening per window

**After:** Once at startup:
- Read all three fonts from font.prefs
- Open each font to get metrics
- Store in global structure
- **Cost:** File I/O + parsing + font opening × 3 (one time only)

**Savings:** Eliminates redundant I/O and font operations for every ListView window.

## Font.prefs File Format

IFF FORM structure:
```
FORM nnnn PREF
    FONT ssss
        struct FontPrefs {
            fp_Type = 0 (WBFONT) | 1 (SYSFONT) | 2 (SCREENFONT)
            fp_TextAttr.ta_YSize = font size
            fp_Name = font name string
        }
    FONT ssss
        ...
```

## Files Modified

1. **`src/Settings/IControlPrefs.h`**
   - Added 9 new font fields (3 fonts × 3 fields each)
   - Comments indicate usage context

2. **`src/Settings/IControlPrefs.c`**
   - Added `#include <prefs/font.h>`
   - Added `read_font_prefs()` helper function
   - Updated `fetchIControlSettings()` to read all three fonts
   - Updated host stub to initialize all three fonts
   - Enhanced `dumpIControlPrefs()` debug output

3. **`src/GUI/default_tool_restore_window.c`**
   - Changed from `fontPrefs` to `prefsIControl.systemFont*`
   - Removed `OpenDiskFont()` / `CloseFont()` calls
   - Removed `#include <proto/diskfont.h>`

## Testing

**Build Status:** ✅ Success  
**Executable:** `Bin/Amiga/iTidy`

**Next Steps:**
1. Test on Amiga
2. Verify debug log shows all three fonts
3. Confirm ListView still formats correctly
4. Check font.prefs with different font configurations

## Future Expansion Opportunities

1. **Custom Icon Rendering:**
   - Use `iconTextFontName` for custom icon labels
   - Respect user's Workbench preferences

2. **Screen Title Customization:**
   - Use `screenTextFontName` for screen titles
   - Match Workbench appearance

3. **Font-Aware Layout:**
   - Calculate button widths based on font metrics
   - Adjust spacing for different font sizes
   - Dynamic layout based on `*CharWidth`

4. **Font Preferences UI:**
   - Allow overriding fonts per-feature
   - Font preview in settings window
   - Live font switching

5. **Font Fallback Chain:**
   - Try multiple fonts if preferred not available
   - Graceful degradation for missing fonts

6. **Font Caching:**
   - Keep opened fonts in memory
   - Reuse TextFont pointers across windows
   - Close on application exit

## Technical Notes

### Character Width Importance

The `*CharWidth` fields are crucial for:
- **ListView Columns:** Calculate how many characters fit in pixel width
- **Text Truncation:** Know when to add "..." ellipsis
- **Button Sizing:** Ensure text fits within gadget
- **Alignment:** Center/right-align text accurately

Formula: `character_width = (pixel_width - padding) / font_char_width`

Example:
- ListView width: 584 pixels
- Border padding: 36 pixels
- Font char width: 8 pixels
- Result: (584 - 36) / 8 = 68.5 → 68 characters

### Font Opening Cost

`OpenDiskFont()` is expensive:
- Searches font directories
- Loads font data from disk
- Allocates memory structures
- Parses font metrics

**Optimization:** Open once, cache metrics, close immediately.

### IFF Chunk Alignment

IFF chunks must be word-aligned:
```c
chunk_size = (chunk_size + 1) & ~1;  /* Ensure even size */
```

Odd-sized chunks have padding byte that must be skipped.

## Global Variable

**Declaration:** `src/Settings/IControlPrefs.h`
```c
extern struct IControlPrefsDetails prefsIControl;
```

**Definition:** `src/main_gui.c`
```c
struct IControlPrefsDetails prefsIControl;
```

**Initialization:** `main()` → `fetchIControlSettings(&prefsIControl)`

## Compatibility

- **Workbench 2.x:** Falls back to old `wbfont.prefs` via `fontPrefs`
- **Workbench 3.x:** Reads `font.prefs` with all three types
- **Workbench 3.9+:** Full support for extended font types
- **Missing Fonts:** Graceful fallback to Topaz 8

## Summary

Complete font system that:
- ✅ Reads all three Amiga font types
- ✅ Caches fonts globally at startup
- ✅ Provides character width metrics
- ✅ Eliminates redundant font operations
- ✅ Supports future expansion
- ✅ Maintains compatibility
- ✅ Includes comprehensive fallbacks
- ✅ Enhanced debug logging

Ready for Amiga testing and future feature development!
