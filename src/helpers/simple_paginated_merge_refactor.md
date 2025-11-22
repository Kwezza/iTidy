# Simple Paginated Formatter Merge

_Date: 2025-11-22_

This document captures the formatter refactor work completed during the current session. The goal was to eliminate the standalone simple paginated formatter and run every ListView mode through the shared helper stack without regressing memory usage or auto-select behavior.

## Summary

- Removed the dedicated `iTidy_FormatListViewColumns_SimplePaginated()` implementation to ensure one canonical formatter pathway.
- Extended shared helpers so the formatter can switch between full, simple, and simple-paginated behavior through context flags instead of separate functions.
- Preserved the low-allocation characteristics of the old simple paginated path by short-circuiting state creation and per-entry `ln_Name` duplication when possible.
- Added the `ITIDY_SIMPLE_PAGINATED_ROLLBACK_GUARD` compile-time switch so QA can fall back to the last known-good formatter if regressions surface during coordinated testing.

## Detailed Changes

1. **Formatter Context Updates**
   - Added `simple_mode` and `simple_paginated_mode` switch points inside `iTidy_FormatListViewColumns()` (`src/helpers/listview_columns_api.c`).
   - Routed pagination metadata (`iTidy_PaginationInfo`) and sorting state into the context once so helpers can reuse them.

2. **Data Row Helper Adjustments**
   - `itidy_add_data_rows()` now accepts a `simple_mode` flag. When TRUE it skips allocating per-entry `ln_Name` strings (the display nodes own the formatted row text), maintaining the lightweight behavior from the removed fast path.
   - Existing row text cleanup still runs to avoid stale pointers when callers reuse entry structures.

3. **Navigation State Handling**
   - Added `itidy_prepare_simple_nav_state()` to build the minimal `iTidy_ListViewState` needed for auto-select and navigation direction tracking when running in `ITIDY_MODE_SIMPLE_PAGINATED`.
   - The helper frees any previously cached state when navigation rows disappear so windows do not hold onto stale pointers.

4. **Shared Helper Reuse**
   - `itidy_emit_nav_rows()` and header/buffer preparation helpers are now used by all modes, removing duplicated logic from the simple paginated formatter.
   - State allocation and column width caching are skipped entirely when running the simple paginated variant, keeping allocation counts close to the previous implementation (~55 nodes for a 50-row page).

5. **Rollback Guard**
   - Introduced `ITIDY_SIMPLE_PAGINATED_ROLLBACK_GUARD` (default `0`) so QA can route `ITIDY_MODE_SIMPLE_PAGINATED` back through the legacy formatter during coordinated regression testing.
   - Legacy implementation remains in `listview_columns_api.c`, compiled only when the guard is set to `1`.

6. **Build Verification**
   - After refactoring, the default `make` command (VBCC Amiga target) completes successfully. This confirms the merged formatter path compiles without introducing new dependencies or build flags.

## Testing & Current Status

- 2025-11-22: Manual WinUAE regression pass over the restore window confirmed pagination, auto-select targeting, and navigation button behavior using the unified formatter path. No regressions observed.
- Current rollout posture: guard remains OFF (value `0`) so production builds exercise the merged path while the legacy formatter stays compiled for safety.

## Follow-Up / Validation

- Keep monitoring windows that use `ITIDY_MODE_SIMPLE_PAGINATED` (e.g., `src/GUI/restore_window.c`) for future feature work or regression checks, especially if the rollback guard is toggled.
- Review documentation (`docs/SIMPLE_PAGINATED_MODE_IMPLEMENTATION.md`) to ensure references to the removed function are updated if needed.
- Plan to delete the `ITIDY_SIMPLE_PAGINATED_ROLLBACK_GUARD` (and the legacy formatter body it protects) once QA signs off on a full regression pass; until then, keep the guard in place so we can flip back instantly if field testing uncovers issues.

---
_Last updated by GitHub Copilot (GPT-5.1-Codex Preview)._