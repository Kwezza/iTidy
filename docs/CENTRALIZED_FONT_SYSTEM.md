# Centralized Font System Implementation

**Status:** ✅ Complete  
**Date:** 2025-01-18  
**Build:** Successful

## Overview

Centralized font information storage to optimize ListView performance by reading font metrics once at startup instead of opening the font for every ListView window created.

## Problem

Original implementation in `default_tool_restore_window.c`:
- Opened font with `OpenDiskFont()` for every ListView window
- Read `tf_XSize` (character width) each time
- Performance overhead: Multiple font opens for same font
- Duplicated code across potential multiple ListView windows

## Solution

### 1. Added Font Fields to IControlPrefsDetails Structure

**File:** `src/Settings/IControlPrefs.h`

```c
struct IControlPrefsDetails {
    /* ... existing fields ... */
    
    /* System font information (for ListViews, etc.) */
    char systemFontName[256];           /* Font name from fontPrefs */
    UWORD systemFontSize;               /* Font size (ta_YSize) */
    UWORD systemFontCharWidth;          /* Character width (tf_XSize) from opened font */
};
```

### 2. Populated Font Info at Startup

**File:** `src/Settings/IControlPrefs.c`

Modified `fetchIControlSettings()`:
```c
/* Read font from global fontPrefs (icon font from ENV:sys/font.prefs) */
if (fontPrefs && fontPrefs->name[0]) {
    strncpy(details->systemFontName, fontPrefs->name, 255);
    details->systemFontName[255] = '\0';
    details->systemFontSize = fontPrefs->size;
    
    /* Open font to get actual character width */
    listview_font.ta_Name = fontPrefs->name;
    listview_font.ta_YSize = fontPrefs->size;
    listview_font.ta_Style = FS_NORMAL;
    listview_font.ta_Flags = 0;
    
    opened_font = OpenDiskFont(&listview_font);
    if (opened_font) {
        details->systemFontCharWidth = opened_font->tf_XSize;
        CloseFont(opened_font);
    } else {
        details->systemFontCharWidth = fontPrefs->size;  /* Estimate fallback */
    }
} else {
    /* Fallback to Topaz 8 */
    strcpy(details->systemFontName, "topaz.font");
    details->systemFontSize = 8;
    details->systemFontCharWidth = 8;  /* Topaz 8 is 8x8 */
}
```

**Fallback Chain:**
1. Try `fontPrefs` (icon font from prefs)
2. Try `OpenDiskFont()` to get `tf_XSize`
3. Estimate as `ta_YSize` (font size)
4. Ultimate fallback: Topaz 8 (8x8)

### 3. Updated Debug Logging

Added to `dumpIControlPrefs()`:
```c
log_info(LOG_SETTINGS, "  System Font:\n");
log_info(LOG_SETTINGS, "    Name:       %s\n", prefs->systemFontName);
log_info(LOG_SETTINGS, "    Size:       %u\n", prefs->systemFontSize);
log_info(LOG_SETTINGS, "    Char Width: %u\n", prefs->systemFontCharWidth);
```

### 4. Updated ListView Windows

**File:** `src/GUI/default_tool_restore_window.c`

**Before:**
```c
#include "../Settings/get_fonts.h"

/* Set up ListView font from global fontPrefs */
if (fontPrefs && fontPrefs->name[0]) {
    listview_font.ta_Name = fontPrefs->name;
    listview_font.ta_YSize = fontPrefs->size;
    /* ... */
    opened_font = OpenDiskFont(&listview_font);
    if (opened_font) {
        font_char_width = opened_font->tf_XSize;
        CloseFont(opened_font);
    }
}
```

**After:**
```c
#include "../Settings/IControlPrefs.h"

/* Use system font from global prefsIControl (already loaded at startup) */
listview_font.ta_Name = prefsIControl.systemFontName;
listview_font.ta_YSize = prefsIControl.systemFontSize;
listview_font.ta_Style = FS_NORMAL;
listview_font.ta_Flags = 0;
font_char_width = prefsIControl.systemFontCharWidth;

log_info(LOG_GUI, "Using system font for ListView: %s size=%u, char_width=%u\n", 
         listview_font.ta_Name, listview_font.ta_YSize, font_char_width);
```

**Changes:**
- Removed `#include <proto/diskfont.h>` (no longer opening fonts)
- Removed `OpenDiskFont()` / `CloseFont()` calls
- Direct access to cached font info from `prefsIControl`
- Simpler, faster, less code

## Benefits

1. **Performance:** Font opened once at startup instead of per-window
2. **Code Simplification:** Removed duplicate font-opening logic
3. **Consistency:** All ListViews use same font source
4. **Maintainability:** Font info in one central location
5. **Reusability:** Other windows can easily access font info

## Global Variable

**Declaration:** `src/Settings/IControlPrefs.h`
```c
extern struct IControlPrefsDetails prefsIControl;
```

**Definition:** `src/main_gui.c`
```c
struct IControlPrefsDetails prefsIControl;
```

**Population:** Called early in `main()` via `fetchIControlSettings(&prefsIControl)`

## Usage Pattern

Any window needing font information:
```c
#include "Settings/IControlPrefs.h"

/* Access font info */
struct TextAttr font;
font.ta_Name = prefsIControl.systemFontName;
font.ta_YSize = prefsIControl.systemFontSize;
font.ta_Style = FS_NORMAL;
font.ta_Flags = 0;

UWORD char_width = prefsIControl.systemFontCharWidth;
```

## Testing

**Build:** ✅ Success (Stage 1)  
**Warnings:** Only expected system header warnings  
**Next:** Test on Amiga to verify:
- Font info appears in debug log
- ListView columns still format correctly
- No crashes from global variable access

## Files Modified

1. `src/Settings/IControlPrefs.h` - Added font fields to structure
2. `src/Settings/IControlPrefs.c` - Populate font info at startup
3. `src/GUI/default_tool_restore_window.c` - Use cached font info

## Related

- **ListView Formatter:** `listview_formatter.c` (uses character width)
- **Font Source:** `fontPrefs` from `ENV:sys/font.prefs` (icon font)
- **Character Width Calculation:** `(pixel_width - 36) / systemFontCharWidth`
- **Border Spacing:** 36 pixels (scrollbar 20px + borders 16px)

## Future Work

- Apply to other ListView windows when created
- Consider adding to host stub for better testing
- Potential expansion for different font contexts (buttons, etc.)
