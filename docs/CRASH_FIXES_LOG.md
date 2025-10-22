# iTidy Crash Fixes Log

This document tracks crash issues encountered during development and the fixes applied.

---

## Crash #1: Illegal Instruction Exception (Error 81000005)
**Date:** October 22, 2025

### Symptoms
- Application crashed with "Software Failure. Error: 81000005"
- Yellow flash (Recoverable Alert) appeared briefly before crash
- Amiga system reset required
- Crash occurred during icon saving phase after successful layout calculation

### Environment
- Workbench 3.1
- 68020 CPU
- Screen resolution: 800×600
- Test case: 26 icons, 5-column layout with aspect ratio 1.6

### Error Analysis
- **Error Code 81000005**: Illegal Instruction exception on Amiga
- Possible causes:
  - NULL pointer dereference being executed as code
  - Division by zero
  - Invalid FPU operation
  - Executing data as code
  - Memory corruption

### Investigation Process
1. Analyzed crash log (iTidy.log):
   - Layout calculated successfully (5 columns × 6 rows)
   - All 26 icon positions computed correctly
   - Crash occurred after "==== SECTION: SAVING ICONS ====" header
   - No "Processing icon 0" message appeared (should be first log in save loop)

2. Narrowed crash location to `saveIconsPositionsToDisk()` function:
   - File: `src/file_directory_handling.c`
   - Crash between lines 289-296 (loop start to first log message)
   - First executable line in loop: `updateCursor()` at line 301
   - Second executable line: `removeInfoExtension()` at line 302

3. Examined `updateCursor()` function in `src/spinner.c`:
   - Uses timer.device
   - Performs GetSysTime() calls
   - Integer division: `t.tv_micro / 1000`
   - No obvious crash causes found

4. Pattern Recognition:
   - Crash only occurred with 5+ column layouts (new behavior after Auto mode fix)
   - Previous 4-column layouts never crashed during save
   - Spinner code designed for CLI mode, not GUI mode
   - Timer.device usage in GUI context could cause issues

### Root Cause
**Spinner code incompatibility with GUI mode**: The `updateCursor()` function was designed for CLI progress indication and used timer.device operations. When called from GUI mode during the icon save loop, it triggered an Illegal Instruction exception.

### Fix Applied
**Removed all spinner code from GUI save operations:**

1. **File: `src/file_directory_handling.c`**
   - Removed `#include "spinner.h"` (line 15)
   - Removed `updateCursor()` call from save loop (line 301)
   - Removed `eraseSpinner()` call at end of function (line 384)

2. **File: `src/file_directory_handling.h`**
   - Removed `#include "spinner.h"` (line 31)

**Rationale:** The spinner was CLI-only functionality. GUI version has a progress bar for status indication, making the spinner redundant and problematic.

### Testing Required
- Deploy fixed build to Amiga
- Test with 5-column layout (aspect ratio 1.6)
- Verify icon positions save correctly
- Confirm no crash during save operation
- Test with various column counts (3, 4, 5, 6, 7, 8)

### Status
**FIXED - PENDING TESTING**

Build completed successfully on October 22, 2025.
Awaiting Amiga deployment and verification testing.

---

## Related Issues

### Auto Mode Column Calculation (Fixed)
**Date:** October 22, 2025

**Issue:** Auto mode was limiting all aspect ratios >1.3 to 4 columns maximum due to using `effectiveWidth` (55% of screen = 440px) instead of full screen width.

**Fix:** Modified `src/aspect_ratio_layout.c` (lines 268-315) to calculate `maxCols` from full `maxUsableWidth` (800px) instead of constrained `effectiveWidth`. This allows Auto mode to properly support wider aspect ratios with up to 8 columns.

**Result:** Successfully calculates 5 columns for aspect ratio 1.6, which then exposed the spinner crash issue above.

---

## Debugging Tips for Future Crashes

### Amiga Error Codes
- **81000004**: Address Error (misaligned memory access)
- **81000005**: Illegal Instruction
- **81000009**: Privilege Violation
- **8000000B**: Line 1111 Emulator (undefined instruction)
- **8000000C**: Line 1010 Emulator (A-Line trap)

### Debug Log Analysis
1. Check last successful log entry before crash
2. Identify which function/section was executing
3. Look for missing expected log entries (indicates crash point)
4. Review code between last successful log and expected next log

### Common Crash Causes
1. **Timer/Device Issues**: Avoid timer.device operations in GUI code
2. **NULL Pointers**: Always validate pointers before dereferencing
3. **Buffer Overflows**: Check array bounds, especially with dynamic calculations
4. **Stack Overflow**: Watch for excessive recursion or large stack allocations
5. **Memory Corruption**: Verify malloc/free pairs, check for use-after-free

### Investigation Strategy
1. Add debug logging at function entry/exit points
2. Log parameter values before operations
3. Use binary search approach: Add logs to narrow crash location
4. Check for pattern: Does it crash with specific inputs/configurations?
5. Review recent code changes that might affect crash area

---

## Change History

| Date | Issue | Status | Files Modified |
|------|-------|--------|----------------|
| 2025-10-22 | Illegal Instruction crash (81000005) | Fixed - Pending Test | file_directory_handling.c, file_directory_handling.h |
| 2025-10-22 | Auto mode column limit | Fixed | aspect_ratio_layout.c |
| 2025-10-22 | Added "Tall" aspect ratio option | Complete | advanced_window.c, advanced_window.h |
