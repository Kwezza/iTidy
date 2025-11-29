# icon.library Usage in iTidy

This document catalogs all uses of AmigaOS icon.library functions throughout the iTidy project.

---

## Overview

iTidy uses icon.library for:
1. Reading icon metadata (position, size, default tool)
2. Detecting icon types (Standard, NewIcon, OS 3.5)
3. Detecting frameless/borderless icons
4. Writing updated icon data (default tools, positions)

**Minimum Required Version**: icon.library v33+ (Workbench 1.3+)  
**Advanced Features Require**: icon.library v44+ (OS 3.5+) for frameless detection

---

## Version Detection

### File: `src/Settings/WorkbenchPrefs.c`

**Location**: Lines 114-119  
**Function**: `fetchWorkbenchSettings()`

```c
/* Get icon.library version */
IconBase = OpenLibrary("icon.library", 0);
if (IconBase) {
    settings->iconLibraryVersion = IconBase->lib_Version;
    CloseLibrary(IconBase);
}
```

**Purpose**: Detects icon.library version at startup and stores in global `prefsWorkbench.iconLibraryVersion`  
**Minimum Version**: 0 (any version)  
**Used For**: Version checking before using v44+ features

---

## Icon Data Reading Functions

### 1. GetDiskObject()

Used to load icon metadata from disk (.info files).

#### File: `src/icon_types.c`

**Function**: `GetNewIconSizePath()` (Lines 26-120)
- **Purpose**: Extract NewIcon bitmap dimensions from IM1= tooltype
- **Opens library**: No (uses global icon.library functions)
- **Version required**: v33+ (standard)
- **Critical code**:
  ```c
  diskObject = GetDiskObject(filePathCopy);
  toolTypes = diskObject->do_ToolTypes;
  /* Parse IM1= tooltype for width/height */
  FreeDiskObject(diskObject);
  ```

**Function**: `GetStandardIconSize()` (Lines 154-205)
- **Purpose**: Read icon dimensions from standard DiskObject
- **Opens library**: No
- **Version required**: v33+ (standard)
- **Critical code**:
  ```c
  diskObject = GetDiskObject(filePathCopy);
  iconSize->width = diskObject->do_Gadget.Width;
  iconSize->height = diskObject->do_Gadget.Height;
  FreeDiskObject(diskObject);
  ```

**Function**: `IsNewIconPath()` (Lines 272-359)
- **Purpose**: Detect if icon is NewIcon format by checking tooltypes
- **Opens library**: No
- **Version required**: v33+ (standard)
- **Critical code**:
  ```c
  diskObject = GetDiskObject(adjustedFilePath);
  toolTypes = diskObject->do_ToolTypes;
  /* Check for "*** DON'T EDIT THE FOLLOWING LINES!! ***" */
  FreeDiskObject(diskObject);
  ```

**Function**: `GetIconDetailsFromDisk()` (Lines 697-885)
- **Purpose**: **PRIMARY FUNCTION** - Single optimized read of all icon metadata
- **Opens library**: No (uses proto/icon.h implicit library)
- **Version required**: v33+ (standard), v44+ for frameless detection
- **Returns**: Position, size, type, frame status, default tool
- **Critical code**:
  ```c
  diskObj = GetDiskObject(pathCopy);
  details->position.x = diskObj->do_CurrentX;
  details->position.y = diskObj->do_CurrentY;
  details->size.width = diskObj->do_Gadget.Width;
  details->size.height = diskObj->do_Gadget.Height;
  /* Extract default tool if present */
  if (diskObj->do_DefaultTool != NULL) {
      details->defaultTool = whd_malloc(strlen(diskObj->do_DefaultTool) + 1);
      strcpy(details->defaultTool, diskObj->do_DefaultTool);
  }
  FreeDiskObject(diskObj);
  ```

**Function**: `SetIconDefaultTool()` (Lines 896-985)
- **Purpose**: Update icon's default tool field and write to disk
- **Opens library**: No
- **Version required**: v33+ (standard)
- **Critical code**:
  ```c
  diskObj = GetDiskObject(pathCopy);
  /* Update do_DefaultTool */
  diskObj->do_DefaultTool = newTool;
  PutDiskObject(pathCopy, diskObj);  /* Write changes */
  FreeDiskObject(diskObj);
  ```

---

## Frameless/Borderless Detection

### File: `src/icon_types.c`

**Function**: `GetIconDetailsFromDisk()` (Lines 801-824)

```c
/* Determine frame status (only on Amiga with icon.library v44+) */
#if PLATFORM_AMIGA
    /* Only attempt frameless detection if icon.library v44+ is available */
    if (prefsWorkbench.iconLibraryVersion >= 44)
    {
        struct Library *IconBase = OpenLibrary("icon.library", 44);
        if (IconBase != NULL)
        {
            int32_t frameStatus = 0;
            int32_t errorCode = 0;
            
            if (IconControl(diskObj,
                           ICONCTRLA_GetFrameless, &frameStatus,
                           ICONA_ErrorCode, &errorCode,
                           TAG_END) == 1)
            {
                /* frameStatus == 0 means it HAS a frame */
                details->hasFrame = (frameStatus == 0) ? TRUE : FALSE;
            }
            
            CloseLibrary(IconBase);
        }
    }
#endif
```

**Purpose**: Query icon for frameless/borderless status  
**Version required**: v44+ (OS 3.5+)  
**Fallback**: If library < v44, assumes `hasFrame = TRUE`  
**Critical**: Version check prevents crash on WB 2.x systems

---

### File: `src/icon_management.c`

**Function**: `DoesIconHaveBorder()` (Lines 860-959)

```c
/* Open the icon.library */
IconBase = OpenLibrary("icon.library", 0);
if (!IconBase) {
    return TRUE; /* Assume frame if library unavailable */
}

if (IconBase->lib_Version < 44) {
    CloseLibrary(IconBase);
    return TRUE; /* Assume frame if version too low */
}

icon = GetIconTags(newIconName, TAG_END);
if (IconControl(icon,
                ICONCTRLA_GetFrameless, &frameStatus,
                ICONA_ErrorCode, &errorCode,
                TAG_END) == 1)
{
    /* frameStatus == 0 means it HAS a frame */
    hasFrame = (frameStatus == 0) ? TRUE : FALSE;
}

FreeDiskObject(icon);
CloseLibrary(IconBase);
```

**Purpose**: Alternative frameless detection (appears to be legacy code)  
**Version required**: v44+ (with runtime check)  
**Uses**: `GetIconTags()` and `IconControl()` with `ICONCTRLA_GetFrameless`  
**Note**: This function duplicates functionality in `GetIconDetailsFromDisk()`

---

## Icon Writing Functions

### 1. PutDiskObject()

Used to write icon changes back to disk.

#### File: `src/icon_types.c`

**Function**: `SetIconDefaultTool()` (Line 964)

```c
if (PutDiskObject(pathCopy, diskObj))
{
    success = TRUE;
}
```

**Purpose**: Save updated default tool to .info file  
**Version required**: v33+ (standard)  
**Critical**: Must call `FreeDiskObject()` after `PutDiskObject()`

---

## Memory Management

### Critical Rules:

1. **Always pair operations**:
   - `GetDiskObject()` → `FreeDiskObject()`
   - `OpenLibrary()` → `CloseLibrary()`

2. **do_DefaultTool handling**:
   ```c
   /* Note: do_DefaultTool memory is managed by icon.library, don't free it manually */
   ```
   - When reading: icon.library allocates the string
   - When writing: Must allocate new string with `whd_malloc()` before assigning
   - Never call `whd_free()` on `do_DefaultTool` from a loaded DiskObject

3. **Library handle scope**:
   - `GetIconDetailsFromDisk()`: Opens library v44 locally, closes immediately
   - `DoesIconHaveBorder()`: Opens library v0, checks version, closes after use
   - `fetchWorkbenchSettings()`: Opens library v0 to get version, closes immediately

---

## Version Requirements Summary

| Function | Minimum Version | Notes |
|----------|----------------|-------|
| `GetDiskObject()` | v33 (WB 1.3+) | Standard icon reading |
| `FreeDiskObject()` | v33 (WB 1.3+) | Standard icon cleanup |
| `PutDiskObject()` | v33 (WB 1.3+) | Standard icon writing |
| `GetIconTags()` | v44 (OS 3.5+) | Advanced icon loading |
| `IconControl()` with `ICONCTRLA_GetFrameless` | v44 (OS 3.5+) | Frameless detection |

---

## Platform Compatibility

### Workbench 1.3 - 2.1 (icon.library v33-39)
- ✅ Icon reading/writing works
- ✅ Position/size detection works
- ❌ Frameless detection **NOT AVAILABLE**
- **Behavior**: All icons assumed to have frames (`hasFrame = TRUE`)

### Workbench 3.0 - 3.1 (icon.library v39-40)
- ✅ Icon reading/writing works
- ✅ NewIcon detection works
- ❌ Frameless detection **NOT AVAILABLE**
- **Behavior**: All icons assumed to have frames

### OS 3.5+ (icon.library v44+)
- ✅ Icon reading/writing works
- ✅ NewIcon detection works
- ✅ Frameless detection works
- ✅ All features available

---

## Known Issues and Fixes

### Issue: Crash on Workbench 2.x (November 29, 2025)

**Problem**: Attempting to open icon.library v44 on systems with only v37-39 caused crashes

**Fix**: Added version check before opening library v44:
```c
if (prefsWorkbench.iconLibraryVersion >= 44)
{
    struct Library *IconBase = OpenLibrary("icon.library", 44);
    /* Safe to use v44 features */
}
```

**Location**: `src/icon_types.c:803-824`

---

## Future Improvements

1. **Consolidate frameless detection**: `DoesIconHaveBorder()` in `icon_management.c` appears to duplicate functionality in `GetIconDetailsFromDisk()`. Consider removing duplicate.

2. **Cache library handle**: Currently opens/closes icon.library for each icon. Could cache handle for batch operations.

3. **Error reporting**: Add structured error returns instead of boolean TRUE/FALSE for better debugging.

4. **OS 3.9/4.x support**: Consider adding support for ColorIcon and GlowIcon formats if icon.library v45+ is available.

---

## References

- AmigaOS icon.library documentation: [autodocs/icon.doc](http://amigadev.elowar.com/read/ADCD_2.1/Libraries_Manual_guide/node02AF.html)
- NewIcons specification: [NewIcons format](http://www.aminet.net/package/util/wb/newicons46)
- icon.library version history:
  - v33: Workbench 1.3
  - v37: Workbench 2.04
  - v39: Workbench 3.0/3.1
  - v44: OS 3.5
  - v45: OS 3.9
  - v46+: OS 4.x

---

**Document Version**: 1.0  
**Last Updated**: November 29, 2025  
**Author**: iTidy Development Team
