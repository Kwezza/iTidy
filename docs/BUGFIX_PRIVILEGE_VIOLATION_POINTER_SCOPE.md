# Privilege Violation Fix - Invalid Pointer Access

**Date:** November 10, 2025  
**Issue:** Crash with error #80000008 (privilege violation) during icon processing  
**Status:** Fixed - pointer scope corrected  

## Problem Description

iTidy crashed with Amiga error #80000008 (privilege violation) when processing icons. The crash occurred during icon type detection, specifically when accessing FileInfoBlock data.

## Root Cause

**Critical scope/indentation bug in `src/icon_management.c`:**

The code was accessing the `fib` (FileInfoBlock) pointer OUTSIDE the scope where it was valid. The indentation made it appear that lines 529-629 were inside the `if (GetIconDetailsFromDisk(...))` success block, but they were actually OUTSIDE.

### Original Broken Structure

```c
if (GetIconDetailsFromDisk(fullPathAndFile, &iconDetails))
{
    /* Extract position, size, default tool */
    newIcon.icon_x = iconDetails.position.x;
    ...
}  /* ← Block ENDED here at line 521 */

/* CODE BELOW WAS OUTSIDE THE IF-BLOCK (wrong indentation misled reading) */
newIcon.is_write_protected = (fib->fib_Protection & FIBF_WRITE); /* ← Line 529: CRASH! */
newIcon.file_size = fib->fib_Size;  /* ← Also accessing invalid pointer */
newIcon.file_date.ds_Days = fib->fib_Date.ds_Days;  /* ← Also invalid */
... (100+ more lines accessing fib)
```

### Why This Caused Privilege Violation

When `GetIconDetailsFromDisk()` failed (e.g., corrupted icon), the `fib` pointer was never initialized. The code then tried to dereference this invalid pointer, causing AmigaOS to throw a privilege violation (#80000008).

The indentation was DECEPTIVE - the code LOOKED like it was inside the if-block with double-tab indentation, but the closing brace at line 521 actually ended the block.

## The Fix

**Moved ALL fib-dependent code INSIDE the GetIconDetailsFromDisk success block:**

### Corrected Structure

```c
if (GetIconDetailsFromDisk(fullPathAndFile, &iconDetails))
{
    /* Extract position, size, default tool */
    newIcon.icon_x = iconDetails.position.x;
    ...
    
    /* NOW INSIDE THE SUCCESS BLOCK - fib is valid here */
    newIcon.is_write_protected = (fib->fib_Protection & FIBF_WRITE);
    newIcon.file_size = fib->fib_Size;
    newIcon.file_date.ds_Days = fib->fib_Date.ds_Days;
    ...
    /* Allocate memory, add to array, etc. */
    fileCount++;
}  /* ← Success block closes here */
else
{
    /* Failure path - skip this icon */
    append_to_log("Warning: Failed to read icon details for %s\n", fullPathAndFile);
    continue;
}
```

### Brace Structure Fixed

The fix also required correcting the nested brace structure:

1. **Added missing closing brace** for the `if (GetIconDetailsFromDisk(...))` block after the else clause
2. **Corrected block nesting:**
   - `if (fileNameNoInfo[0] != '\0' ...)` - outer block
     - `if (IsValidIcon(...))` - middle block  
       - `if (GetIconDetailsFromDisk(...)) { ... } else { ... }` - inner block
     - Close middle block
   - Close outer block

The original code was missing the closing brace for the middle block (`IsValidIcon`), causing all subsequent function definitions to appear as "function definition in inner block" errors.

## Impact

**Before Fix:**
- Crashed with privilege violation when processing icons with missing/invalid default tools
- Specifically failed on "Programming" type icons in user's test folder
- Stack trace showed crash at `icon_types.c:714` during memory allocation (misleading - actual cause was earlier invalid pointer access)

**After Fix:**
- Icons with corrupted data are safely skipped
- Only valid icons with readable FileInfoBlock data are processed
- Proper error logging for failed icon reads
- No crashes during recursive processing

## Technical Details

### Amiga Error Code #80000008

This is a **privilege violation** - attempting to access memory that the program doesn't have rights to access. Common causes:
- Dereferencing NULL pointers
- Accessing freed memory
- Using pointers outside their valid scope (← our issue)

### FileInfoBlock (`fib`) Pointer

The `fib` pointer is provided by AmigaDOS during pattern matching with `MatchFirst/MatchNext`. It's only valid when:
1. Pattern matching succeeds (MatchNext returns 0)
2. The file exists and is accessible
3. Within the scope of the match result

Our code was accessing `fib->fib_Protection`, `fib->fib_Size`, and `fib->fib_Date` even when `GetIconDetailsFromDisk()` failed, meaning the icon data was corrupted or inaccessible.

## Files Modified

- `src/icon_management.c` - Lines 419-629: Moved fib access code inside GetIconDetailsFromDisk success block
- `src/icon_management.c` - Line 628: Added missing closing brace for IsValidIcon block
- `src/icon_management.c` - Lines 623-627: Added proper else clause for GetIconDetailsFromDisk failure

## Testing

**Test Case:** Process folder with mixed icons (valid and corrupted)

**Result:**
- Valid icons processed correctly
- Corrupted icons logged and skipped: "Warning: Failed to read icon details for..."
- No crashes, no privilege violations
- Memory properly managed (no leaks)

## Related Issues

This bug is SEPARATE from the filesystem lock timing issue (error #80000003). Both bugs were present:

1. **Filesystem Lock Timing** (error #80000003) - Fixed with configurable delay
2. **Pointer Scope Bug** (error #80000008) - Fixed with this change

Both fixes are now in place and working together.

## For Future Developers

**CRITICAL LESSON:** Always verify brace structure with tools, don't trust indentation alone!

When accessing pointers provided by system functions:
1. **Always check return codes** before using output pointers
2. **Keep pointer access within the success block** where the pointer is guaranteed valid
3. **Use compiler warnings** - VBCC's "function definition in inner block" indicated missing braces
4. **Count braces systematically** when debugging nested blocks

This type of bug can be extremely subtle because:
- Indentation makes it LOOK correct
- Works fine if GetIconDetailsFromDisk always succeeds
- Only crashes on edge cases (corrupted icons, missing files)
- Compiler might not warn about pointer usage (it's technically valid syntax)

**Prevention:** Add explicit NULL checks and validation:
```c
if (GetIconDetailsFromDisk(...) && fib != NULL)
{
    /* Now safe to use fib */
}
```

## Crash Log Analysis

The original crash logs showed:
```
Icon type: Standard Workbench format - Programming
(then crash at icon_types.c:714)
```

This was MISLEADING - the crash appeared to be during icon type processing, but actually occurred because we were accessing an invalid `fib` pointer that was supposed to be validated by GetIconDetailsFromDisk first.

The stack trace pointed to memory allocation in `icon_types.c`, but the root cause was dereferencing the bad pointer earlier in `icon_management.c`.

## Conclusion

This fix resolves privilege violations when processing folders with mixed valid/corrupted icons. Combined with the filesystem lock timing fix, iTidy now handles:

✓ Deep recursive directory structures (hundreds of folders)  
✓ Fast emulated systems running at maximum speed  
✓ Corrupted or malformed icon files  
✓ Mixed icon types and formats  

Both critical bugs are now fixed and tested.
