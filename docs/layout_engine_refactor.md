# Layout Engine Refactor

**Date:** February 2026  
**Branch:** Dev  
**Status:** Complete — clean build, zero errors

---

## Overview

`src/layout_processor.c` was a 2,494-line monolith containing seven distinct
concerns lumped together. This refactor extracted those concerns into focused
modules under a new `src/layout/` subdirectory and simultaneously relocated two
companion source files (`aspect_ratio_layout.c/.h`, `folder_scanner.c/.h`) that
had always been exclusively consumed by the layout subsystem.

---

## Motivation

| Problem | Detail |
|---------|--------|
| File too large | 2,494 lines, harder to navigate and review |
| Mixed concerns | Sorting, positioning, block layout, tool scanning and aspect-ratio maths all in one file |
| Companion files misplaced | `aspect_ratio_layout` and `folder_scanner` lived at `src/` root despite only being used by `layout_processor.c` |
| Hard to test | Tightly coupled static functions made unit testing impractical |

---

## Before

```
src/
  layout_processor.c        2,494 lines  (7 logical groups)
  layout_processor.h
  aspect_ratio_layout.c       526 lines  (only used by layout_processor.c)
  aspect_ratio_layout.h
  folder_scanner.c            274 lines  (only used by layout_processor.c)
  folder_scanner.h
```

---

## After

```
src/
  layout_processor.c          964 lines  (core orchestration only)
  layout_processor.h          (+ SetCurrentProgressWindow added)
  layout/
    aspect_ratio_layout.c     525 lines  (moved, no logic change)
    aspect_ratio_layout.h     142 lines  (moved, no logic change)
    folder_scanner.c          273 lines  (moved, no logic change)
    folder_scanner.h           53 lines  (moved, no logic change)
    icon_sorter.c             266 lines  (extracted from layout_processor.c)
    icon_sorter.h              41 lines
    icon_positioner.c         569 lines  (extracted from layout_processor.c)
    icon_positioner.h          57 lines
    block_layout.c            477 lines  (extracted from layout_processor.c)
    block_layout.h             48 lines
    tool_scanner.c            326 lines  (extracted from layout_processor.c)
    tool_scanner.h             48 lines
```

`layout_processor.c` shrank by **1,530 lines (−61%)**.

---

## Module Breakdown

### `layout/icon_sorter.c/.h`

**Responsibility:** Sort an `IconArray` according to `LayoutPreferences`.

**Extracted from:** `layout_processor.c` lines 2264–2494 (original numbering).

**Public API:**
```c
void SortIconArrayWithPreferences(IconArray *iconArray,
                                  const LayoutPreferences *prefs);
```

**Internal functions kept static:** `GetFileExtension`, `CompareDateStamps`,
`CompareIconsWithPreferences`, `CompareIconsWrapper`, `g_sort_prefs`.

---

### `layout/icon_positioner.c/.h`

**Responsibility:** Assign pixel-level `(x, y)` coordinates to each icon in a
row-based or column-centred grid.

**Extracted from:** `layout_processor.c` lines 1402–1931 (original numbering).

**Public API:**
```c
void CalculateLayoutPositions(IconArray *iconArray,
                              const LayoutPreferences *prefs,
                              int targetColumns);

void CalculateLayoutPositionsWithColumnCentering(IconArray *iconArray,
                                                 const LayoutPreferences *prefs,
                                                 int targetColumns);
```

**Internal functions kept static:** `apply_row_vertical_alignment`.

---

### `layout/block_layout.c/.h`

**Responsibility:** Partition icons into Workbench type blocks (drawers, tools,
other) and lay them out in visually grouped sections with configurable gaps.

**Extracted from:** `layout_processor.c` lines 947–1401 (original numbering).

**Public API:**
```c
BOOL CalculateBlockLayout(IconArray *iconArray,
                          const LayoutPreferences *prefs,
                          int *outWidth,
                          int *outHeight);
```

**Internal functions kept static:** `GetBlockGapPixels`,
`PartitionIconsByWorkbenchType`.

**Dependencies on other new modules:** `icon_positioner.h`, `icon_sorter.h`,
`layout/aspect_ratio_layout.h`.

---

### `layout/tool_scanner.c/.h`

**Responsibility:** Walk directory trees to validate icon default tools and
populate the tool cache, without making any changes to icon positions.

**Extracted from:** `layout_processor.c` lines 679–939 (original numbering).

**Public API:**
```c
BOOL ScanDirectoryForToolsOnly(void);
BOOL ScanDirectoryForToolsOnlyWithProgress(struct iTidyMainProgressWindow *pw);
```

**Internal functions kept static:** `ScanSingleDirectoryForTools`,
`ScanDirectoryRecursiveForTools`.

**Design note:** `ScanDirectoryForToolsOnlyWithProgress` previously set
`g_progressWindow` directly inside `layout_processor.c`. After extraction it
calls `SetCurrentProgressWindow()` (new setter — see below) to keep the static
variable encapsulated in `layout_processor.c`.

---

### `layout/aspect_ratio_layout.c/.h` and `layout/folder_scanner.c/.h`

These files were **moved, not refactored**. Logic is unchanged. The only
modification to each `.c` file was updating the self-include path:

```c
/* Before */
#include "aspect_ratio_layout.h"
#include "folder_scanner.h"

/* After */
#include "layout/aspect_ratio_layout.h"
#include "layout/folder_scanner.h"
```

---

## Changes to Existing Files

### `src/layout_processor.c`

| Change | Detail |
|--------|--------|
| Include paths updated | `"aspect_ratio_layout.h"` and `"folder_scanner.h"` changed to `"layout/..."` variants |
| New includes added | `"layout/icon_sorter.h"`, `"layout/icon_positioner.h"`, `"layout/block_layout.h"`, `"layout/tool_scanner.h"` |
| Forward declarations removed | All declarations for extracted static functions removed |
| Function bodies removed | All extracted functions deleted (~1,530 lines) |
| `SetCurrentProgressWindow()` added | New implementation alongside the existing `GetCurrentProgressWindow()` getter |

The file now contains only the top-level orchestration functions:
`ProcessDirectoryWithPreferences`, `ProcessDirectoryWithPreferencesAndProgress`,
`ProcessSingleDirectory`, `ProcessDirectoryRecursive`, and the progress window
accessors.

### `src/layout_processor.h`

Added `SetCurrentProgressWindow()` declaration:

```c
void SetCurrentProgressWindow(struct iTidyMainProgressWindow *pw);
```

This allows `tool_scanner.c` (and any future sub-module) to update the
`g_progressWindow` static variable in `layout_processor.c` without exposing it.

### `Makefile`

| Change | Detail |
|--------|--------|
| `CORE_SRCS` | Removed `aspect_ratio_layout.c` and `folder_scanner.c` |
| `LAYOUT_SRCS` | New variable listing all six `src/layout/` source files |
| `LAYOUT_OBJS` | New derived object file list |
| `SRCS` aggregate | `$(LAYOUT_SRCS)` added |
| `OBJS` aggregate | `$(LAYOUT_OBJS)` added |
| `directories` target | Added `@if not exist "$(OUT_DIR)\layout" mkdir "$(OUT_DIR)\layout"` |

---

## Include Path Convention

VBCC is invoked with `-Isrc`, so all includes resolve relative to `src/`.
Files inside `src/layout/` follow this pattern:

```c
/* Reference a sibling module in src/layout/ */
#include "layout/icon_sorter.h"

/* Reference a project-wide header in src/ */
#include "layout_preferences.h"
#include "icon_management.h"
#include "writeLog.h"

/* Reference platform headers */
#include <platform/platform.h>
#include <platform/amiga_headers.h>
```

Do **not** use `../` relative paths — VBCC does not need them and they break the
`-Isrc` convention used throughout the project.

---

## Build Verification

```
make clean && make
```

Result: **zero errors**, only the expected VBCC warnings 51 and 61 from
system headers (these cannot be suppressed and pre-date this refactor).

Final binary: `Bin/Amiga/iTidy`

---

## Line Count Summary

| File | Lines (before) | Lines (after) |
|------|:--------------:|:-------------:|
| `src/layout_processor.c` | 2,494 | 964 |
| `src/aspect_ratio_layout.c` | 526 | deleted (moved) |
| `src/folder_scanner.c` | 274 | deleted (moved) |
| `src/layout/aspect_ratio_layout.c` | — | 525 |
| `src/layout/folder_scanner.c` | — | 273 |
| `src/layout/icon_sorter.c` | — | 266 |
| `src/layout/icon_positioner.c` | — | 569 |
| `src/layout/block_layout.c` | — | 477 |
| `src/layout/tool_scanner.c` | — | 326 |
