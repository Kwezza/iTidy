# Path Utilities Refactoring and Global Module Creation

## 📋 Overview

**Goal:** Create a reusable global path utilities module to consolidate path truncation logic currently duplicated across the codebase. This will provide consistent path handling for all GUI windows, especially the upcoming Default Tool Replacement Window.

**Status:** Planning Phase  
**Priority:** Medium - Required before implementing Default Tool Replacement Window  
**Estimated Effort:** 2-3 hours  

---

## 🎯 Objectives

### Primary Goals
1. **Eliminate code duplication** - Two separate path truncation implementations exist
2. **Create reusable utilities** - Centralize path manipulation in one location
3. **Support multiple use cases** - Different windows need different truncation strategies
4. **Follow project conventions** - Use `iTidy_` naming prefix and proper structure

### Secondary Goals
- Add intelligent `/../` path abbreviation feature (Amiga-style)
- Maintain backward compatibility with existing windows
- Ensure fixed-width and proportional font compatibility
- Provide clear documentation and examples

---

## 📂 Current Situation

### Existing Implementations

#### 1. **Character-Based Truncation** (`tool_cache_window.c`)
- **Location:** `src/GUI/tool_cache_window.c` lines 43-127
- **Function:** `truncate_tool_name_middle()`
- **Scope:** `static` (not globally accessible)
- **Strategy:** Uses string character counts (assumes fixed-width font)
- **Format:** `"device:...filename"` (preserves device name and filename)
- **Example:** `"Workbench:Programs/Wordworth7/Wordworth"` → `"Workbench:...Wordworth"`

#### 2. **Pixel-Based Truncation** (`progress_common.c`)
- **Location:** `src/GUI/StatusWindows/progress_common.c` lines 173-277
- **Function:** `iTidy_Progress_DrawTruncatedText()`
- **Scope:** Global (exported in `progress_common.h`)
- **Strategy:** Uses `TextLength()` for pixel measurements (proportional font safe)
- **Format:** `"start...end"` (balanced middle truncation)
- **Features:** Supports both path truncation and normal text truncation
- **Example:** `"Work:Programs/LongFolder/Tool"` → `"Work:Prog.../Tool"`

### Key Differences

| Feature | Character-Based | Pixel-Based |
|---------|----------------|-------------|
| **Font Support** | Fixed-width only | Any font (measures pixels) |
| **Use Case** | ListView columns | Drawing text in windows |
| **Measurement** | `strlen()` counts | `TextLength()` measures |
| **Accessibility** | Static function | Global function |
| **Complexity** | Simple string math | Font metrics required |

---

## 🏗️ Proposed Solution

### New Module Structure

```
src/
  path_utilities.h    (NEW - header file)
  path_utilities.c    (NEW - implementation)
```

### Function Signatures

```c
/* Character-based truncation for fixed-width fonts */
void iTidy_TruncatePathMiddle(const char *path, 
                               char *output, 
                               int max_chars);

/* Pixel-based truncation for proportional fonts */
void iTidy_TruncatePathMiddlePixels(struct RastPort *rp,
                                     const char *path,
                                     char *output,
                                     UWORD max_width);

/* Intelligent Amiga-style path abbreviation with /../ */
BOOL iTidy_ShortenPathWithParentDir(const char *path,
                                     char *output,
                                     int max_chars);
```

---

## 📝 Implementation Tasks

### Phase 1: Create New Module (Do First)

#### Task 1.1: Create Header File
**File:** `src/path_utilities.h`

**Requirements:**
- Add header guard: `#ifndef ITIDY_PATH_UTILITIES_H`
- Include necessary types: `<exec/types.h>`, `<graphics/rastport.h>`
- Document each function with examples
- Use iTidy naming convention: `iTidy_` prefix
- Keep it platform-agnostic (no GUI dependencies)

**Deliverable:** Complete header with three function prototypes

---

#### Task 1.2: Create Implementation File
**File:** `src/path_utilities.c`

**Requirements:**
- Include platform headers: `<string.h>`, `<stdio.h>`, `<proto/graphics.h>`
- Include project header: `"path_utilities.h"`

**Deliverable:** Three implemented functions (see details below)

---

### Phase 2: Implement Core Functions

#### Task 2.1: Implement `iTidy_TruncatePathMiddle()`

**Source:** Extract logic from `tool_cache_window.c::truncate_tool_name_middle()`

**Algorithm:**
1. Check if path fits within `max_chars` - if yes, copy as-is
2. Find last `/` or `:` to identify filename/program name
3. If no separators found, truncate in middle with `"..."`
4. If separators found, preserve device name and filename with `"device:...filename"`
5. Ensure output never exceeds `max_chars + 1` (including null terminator)

**Edge Cases to Handle:**
- NULL input path → set output to empty string
- Path already fits → copy verbatim
- Very short `max_chars` (< 10) → truncate sensibly
- No path separators → simple middle truncation
- Buffer overflow prevention

**Testing Examples:**
```c
// Input: "Workbench:Programs/Wordworth7/Wordworth" (41 chars)
// max_chars: 22
// Output: "Workbench:...Wordworth"

// Input: "C:More" (6 chars)
// max_chars: 40
// Output: "C:More" (unchanged)

// Input: "VeryLongSingleNameWithoutSeparators" (35 chars)
// max_chars: 20
// Output: "VeryLong...eparators"
```

---

#### Task 2.2: Implement `iTidy_TruncatePathMiddlePixels()`

**Source:** Extract logic from `progress_common.c::iTidy_Progress_DrawTruncatedText()`

**Algorithm:**
1. Use `TextLength(rp, text, strlen(text))` to measure full path width
2. If fits within `max_width`, copy as-is
3. Measure ellipsis width: `TextLength(rp, "...", 3)`
4. Calculate target width: `max_width - ellipsis_width`
5. Binary search or iterative approach to find how many chars fit in first half
6. Binary search or iterative approach to find how many chars fit in second half
7. Build output: `"firsthalf...secondhalf"`

**Important:**
- Requires RastPort with font already set (via `SetFont()`)
- Buffer size should be at least 256 bytes
- Use `TextLength()` for all measurements (not `strlen()`)

**Edge Cases:**
- NULL RastPort → return empty string (defensive)
- Path fits → copy verbatim
- Very narrow `max_width` → show `"...end"` at minimum

**Testing Examples:**
```c
// With Topaz 8 font:
// Input: "Work:Projects/Programming/Amiga/Tool"
// max_width: 150 pixels
// Output: "Work:Proj.../Tool" (measured to fit exactly)
```

---

#### Task 2.3: Implement `iTidy_ShortenPathWithParentDir()` (NEW FEATURE)

**Purpose:** Intelligent Amiga-style path abbreviation using `/../` notation

**Algorithm:**
1. Check if path fits within `max_chars` - if yes, return FALSE (no shortening needed)
2. Split path into components using `:` and `/` as delimiters
3. Identify device/volume, middle directories, and filename
4. Calculate how many middle directories to collapse
5. Rebuild path as: `"device:firstdir/../lastdir/filename"`
6. Ensure output fits within `max_chars`
7. Return TRUE if path was shortened, FALSE otherwise

**Strategy:**
- Always preserve device/volume name (before first `:`)
- Always preserve filename (last component)
- Always preserve first directory level (for context)
- Collapse middle directories with `/../`
- If still too long, further collapse first directory

**Examples:**
```c
// Input: "Work:Projects/Programming/Amiga/iTidy/src/GUI/windows/tool.c"
// max_chars: 50
// Output: "Work:Projects/../GUI/windows/tool.c"
// Return: TRUE

// Input: "Work:Projects/Programming/Amiga/iTidy/src/GUI/windows/tool.c"
// max_chars: 40
// Output: "Work:Projects/../tool.c"
// Return: TRUE

// Input: "SYS:Utilities/More"
// max_chars: 40
// Output: "SYS:Utilities/More" (unchanged)
// Return: FALSE
```

**Implementation Notes:**
- Use `strchr()` to find separators
- Use `strrchr()` to find last separator (filename)
- Build output incrementally, checking length at each step
- Prioritize readability over maximum character packing

**Flexibility:** This is a new feature - if you encounter edge cases or discover a better algorithm, adapt as needed. The goal is useful path abbreviation, not a rigid specification.

---

### Phase 3: Refactor Existing Code

#### Task 3.1: Update `tool_cache_window.c`

**File:** `src/GUI/tool_cache_window.c`

**Changes:**
1. Add include: `#include "../path_utilities.h"`
2. Find all calls to `truncate_tool_name_middle()` (lines ~239, ~495)
3. Replace with `iTidy_TruncatePathMiddle()`
4. **Remove** the static function `truncate_tool_name_middle()` (lines 40, 43-127)
5. **Remove** the forward declaration (line ~40)
6. Test that tool cache window still displays correctly

**Verification:**
- Tool names truncate correctly in upper listview
- File paths truncate correctly in lower details panel
- No compilation errors
- No visual regressions

---

#### Task 3.2: Update `progress_common.c` (Optional/Future)

**File:** `src/GUI/StatusWindows/progress_common.c`

**Note:** This is OPTIONAL. The existing `iTidy_Progress_DrawTruncatedText()` serves a dual purpose:
1. Text truncation logic (can be extracted)
2. Drawing the text to RastPort (should stay)

**Approach A (Minimal):**
- Leave as-is for now
- Document that `iTidy_TruncatePathMiddlePixels()` is the reusable version

**Approach B (Full Refactor):**
- Extract truncation logic into `iTidy_TruncatePathMiddlePixels()`
- Refactor `iTidy_Progress_DrawTruncatedText()` to call the new function
- Reduces duplication, but more risky

**Recommendation:** Start with Approach A. Only do Approach B if you have time and feel confident.

---

### Phase 4: Update Build System

#### Task 4.1: Update Makefile

**File:** `Makefile`

**Changes:**
1. Find the `SOURCES` or `OBJS` section
2. Add `src/path_utilities.c` to the build list
3. Ensure it compiles to `build/amiga/path_utilities.o`

**Example:**
```makefile
SOURCES = \
    src/main.c \
    src/utilities.c \
    src/path_utilities.c \
    src/icon_types.c \
    ...
```

**Verification:**
- Run `make clean`
- Run `make`
- Verify no compilation errors
- Verify `build/amiga/path_utilities.o` is created

---

### Phase 5: Documentation and Testing

#### Task 5.1: Add Function Documentation

**Requirements:**
- Each function in `path_utilities.h` must have:
  - Brief description
  - Parameter descriptions with `@param`
  - Return value description with `@return` (if applicable)
  - At least one usage example
  - Notes about edge cases

**Example:**
```c
/**
 * @brief Truncate a path in the middle preserving device and filename
 * 
 * For Amiga paths like "Workbench:Programs/Wordworth7/Wordworth", this will
 * truncate to show the device/volume and the filename while shortening the
 * middle directories, e.g.: "Workbench:...Wordworth"
 * 
 * Best used with fixed-width fonts in ListViews for columnar data display.
 * 
 * @param path Original path to truncate (must not be NULL)
 * @param output Buffer to store result (must be at least max_chars+1 bytes)
 * @param max_chars Maximum character length for output (excluding null terminator)
 * 
 * Example:
 *   char shortened[41];
 *   iTidy_TruncatePathMiddle("Workbench:Programs/Wordworth7/Wordworth", 
 *                             shortened, 40);
 *   // Result: "Workbench:...Wordworth"
 * 
 * @note Output buffer is always null-terminated, even if path is truncated
 * @note If path is NULL, output will be set to empty string
 */
void iTidy_TruncatePathMiddle(const char *path, char *output, int max_chars);
```

---

#### Task 5.2: Test All Functions

**Manual Testing Checklist:**

**Test `iTidy_TruncatePathMiddle()`:**
- [ ] Short path (fits entirely)
- [ ] Long path with device and directories
- [ ] Path without separators
- [ ] NULL input
- [ ] Very short max_chars (< 10)
- [ ] Path with `:` but no `/`
- [ ] Path with `/` but no `:`

**Test `iTidy_TruncatePathMiddlePixels()` (if implemented):**
- [ ] Short path with Topaz 8 font
- [ ] Long path with Topaz 8 font
- [ ] Same tests with different font (if available)
- [ ] Very narrow max_width

**Test `iTidy_ShortenPathWithParentDir()`:**
- [ ] Path that needs shortening
- [ ] Path that already fits
- [ ] Very long path requiring multiple collapses
- [ ] Path with only device and filename
- [ ] Edge case: single directory level

**Integration Testing:**
- [ ] Tool Cache Window displays correctly
- [ ] Progress windows display correctly (if refactored)
- [ ] No memory leaks (verify with AvailMem() before/after)
- [ ] Compile with no warnings

---

## 🔄 Usage in Default Tool Replacement Window

Once this module is complete, the new Default Tool Replacement Window can use these functions:

```c
/* In default_tool_update_window.c */
#include "path_utilities.h"

/* For status ListView (columnar data with fixed-width font): */
char truncated_path[46];  /* 45 chars + null */
iTidy_TruncatePathMiddle(icon_path, truncated_path, 45);

/* Or for intelligent abbreviation: */
char shortened_path[46];
if (iTidy_ShortenPathWithParentDir(icon_path, shortened_path, 45))
{
    /* Path was shortened - use shortened version */
    sprintf(status_line, "%-45s | %s", shortened_path, status_text);
}
else
{
    /* Path already fits - use original */
    sprintf(status_line, "%-45s | %s", icon_path, status_text);
}
```

---

## ⚠️ Important Considerations

### Naming Convention
- **MUST** use `iTidy_` prefix for all global functions
- Follow project convention (see `AI_AGENT_GUIDE.md`)
- Avoid collisions with AmigaOS SDK identifiers

### Memory Management
- All functions use stack buffers or caller-provided buffers
- No dynamic allocation required
- Caller must ensure output buffer is large enough
- Always null-terminate output strings

### Platform Compatibility
- Functions should work on Amiga Workbench 2.0+
- Test with both fixed-width (Topaz) and proportional fonts if possible
- Use standard AmigaOS includes: `<proto/graphics.h>`, `<graphics/text.h>`

### Error Handling
- Defensive programming for NULL inputs
- Buffer overflow prevention
- Return meaningful values (TRUE/FALSE for success)

---

## 🎨 Flexibility and Adaptation

### You Have Freedom To:

1. **Adjust algorithms** if you find edge cases or better approaches
2. **Add helper functions** if they make the code clearer
3. **Modify function signatures** if you discover issues (document why)
4. **Skip optional refactoring** (Task 3.2) if it seems too risky
5. **Add more test cases** if you think of important scenarios
6. **Improve documentation** with better examples or notes

### When to Adapt:

- **If buffer sizes are problematic:** Adjust buffer size constants or add safety checks
- **If TextLength() behaves unexpectedly:** Add fallback logic or different measurement approach
- **If `/../` abbreviation is too complex:** Start with simpler version, iterate later
- **If existing code has dependencies:** Refactor carefully or document why changes were limited

### When to Ask for Guidance:

- **Major API changes** that affect multiple files
- **Performance concerns** with large paths
- **Fundamental design issues** you discover during implementation
- **Breaking changes** to existing window behavior

---

## ✅ Definition of Done

This refactoring is complete when:

1. ✅ `src/path_utilities.h` and `src/path_utilities.c` exist and compile
2. ✅ All three core functions are implemented and tested
3. ✅ `tool_cache_window.c` uses the new functions (old static function removed)
4. ✅ Makefile includes new source file
5. ✅ Tool Cache Window displays correctly (no visual regressions)
6. ✅ Code compiles with no errors or warnings
7. ✅ Functions are documented with examples
8. ✅ Manual testing completed for common cases

---

## 📚 References

### Existing Code to Study
- `src/GUI/tool_cache_window.c` lines 43-127 (current char-based implementation)
- `src/GUI/StatusWindows/progress_common.c` lines 173-277 (current pixel-based)
- `src/utilities.h` and `src/utilities.c` (example of global utilities module)

### Project Guidelines
- `docs/AI_AGENT_GUIDE.md` - Naming conventions and SDK collision avoidance
- `docs/AI_AGENT_QUICK_REFERENCE.md` - Quick patterns and mandatory rules
- `src/templates/amiga_window_template.c` - Example of iTidy code style

### AmigaOS Documentation
- `TextLength()` - measures pixel width of text in current font
- `SetFont()` - sets font in RastPort before TextLength() calls
- String functions: `strchr()`, `strrchr()`, `strlen()`, `strncpy()`

---

## 📝 Implementation Notes Section

**Instructions:** As you work through this plan, add notes below about:
- Challenges encountered and solutions
- Deviations from the plan and reasons
- Performance observations
- Future improvement ideas

### Notes:
*(Add your notes here as you implement)*

---

**Good luck!** Remember: this plan is a guide, not a rigid specification. Use your judgment, test thoroughly, and don't hesitate to improve upon these ideas. The goal is clean, reusable code that makes path handling consistent across iTidy. 🎯
