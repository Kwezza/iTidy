# ListView Stress Test - Performance Benchmark

## Overview

**listview_stress_test** is a dedicated benchmark program that tests the performance of the `listview_columns_api.c` smart helper on a 7MHz Amiga 68000. The goal is to determine the practical limits for the number of ListView items before performance becomes too slow for real-world use.

## What It Tests

The test program creates a single window with:
- **5-column ListView** with auto-sizing
  - Column 0: Date/Time (DATE type) - e.g., "24-Nov-2025 15:19"
  - Column 1: Item# (NUMBER type) - e.g., "1", "2", "3"
  - Column 2: Type (TEXT type) - "Hardware", "Game", "Software"
  - Column 3: Name (TEXT type, flexible width) - Random Amiga facts
  - Column 4: Rating (NUMBER type) - e.g., "7/10", "8/10"
- **Two buttons**:
  - "Add 50 Rows" - Appends 50 more test rows
  - "Remove 50 Rows" - Removes 50 rows from the bottom
- **Click-to-sort** enabled on all column headers
- **Performance timing** output to console

## Features Tested

1. **Initial Load**: 50 rows of random Amiga hardware/game facts
2. **Dynamic Addition**: Add 50-row chunks to see scaling behavior
3. **Dynamic Removal**: Remove 50-row chunks to reduce size
4. **Sorting**: Click column headers to sort by different criteria
5. **Multiple Data Types**: Tests DATE, NUMBER, and TEXT sorting/formatting
6. **Memory Tracking**: Detects memory leaks (if DEBUG_MEMORY_TRACKING enabled)

## Building

```bash
# From iTidy root directory
make test-listview
```

Output: `Bin/Amiga/listview_stress_test`

## Running the Test

### From WinUAE Shared Drive

1. Build the test on PC host (see above)
2. In WinUAE, navigate to the shared `build/amiga` or `Bin/Amiga` directory
3. Double-click `listview_stress_test` or run from Shell

### From Shell

```bash
cd Bin/Amiga
listview_stress_test
```

### From Workbench

Double-click the `listview_stress_test` icon in the `Bin/Amiga` drawer.

**IMPORTANT**: To see timing output, run from a Shell window, not Workbench.

## Console Output

The test logs performance metrics to the console:

```
========================================
  ListView Stress Test - iTidy v2.0
  Performance Benchmark for 7MHz Amiga
========================================
Font metrics: 8 x 8 pixels

=== INITIALIZING LISTVIEW ===
Creating 50 initial rows...
Formatting ListView with 73 character width...
  [TIMING] ListView format: 23.456 ms total (50 entries)
           Sort: 12.123 ms | Width: 4.567 ms | Format: 6.766 ms
ListView initialized with 50 rows
Click column headers to sort!

=== EVENT LOOP STARTED ===
Instructions:
- Click 'Add 50 Rows' to add more data
- Click 'Remove 50 Rows' to remove data
- Click column headers to sort
- Watch console for timing statistics
- Close window to exit

=== ADDING 50 ROWS ===
Current row count: 50
  [TIMING] ListView format: 45.123 ms total (100 entries)
           Sort: 23.456 ms | Width: 8.901 ms | Format: 12.766 ms
Added 50 rows in 45.123 ms
New total: 100 rows
Time per row: 902 microseconds

  [TIMING] ListView resort: 34.567 ms total (col 0, DATE/DESC, 100 entries)
           Sort: 28.123 ms | Rebuild: 6.444 ms
ListView sorted (now showing 100 rows)

=== REMOVING 50 ROWS ===
Current row count: 100
  [TIMING] ListView format: 23.456 ms total (50 entries)
Removed 50 rows in 23.456 ms
New total: 50 rows
Time per row: 469 microseconds
```

## Interpreting Results

### Acceptable Performance Targets (7MHz Amiga 68000)

- **Initial display** (50 rows): < 50ms
- **Add 50 rows**: < 100ms (keeps UI responsive)
- **Sort 100 rows**: < 150ms (acceptable for interactive sorting)
- **Sort 200 rows**: < 300ms (starts to feel sluggish)
- **Sort 500+ rows**: > 1 second (too slow for 7MHz)

### Timing Breakdown

The API reports timing in 3 phases:

1. **Sort** - Merge sort algorithm (O(n log n) complexity)
2. **Width** - Column width calculation (O(rows × columns))
3. **Format** - String formatting and node creation (O(rows × columns))

### What to Look For

- **Linear scaling**: Time should grow proportionally with row count
- **Sort dominance**: As row count increases, sort time should dominate
- **Memory leaks**: Check the final memory report for leaks

### Performance Red Flags

- ⚠️ Sorting 100 rows takes > 200ms (API may have inefficiency)
- ⚠️ Formatting takes longer than sorting (unexpected)
- ⚠️ Memory leaks reported at exit (indicates cleanup bug)
- ⚠️ UI freezes for > 1 second during any operation (poor UX)

## Test Procedure

### Phase 1: Baseline (50 rows)
1. Start program
2. Note initial format time
3. Click each column header to test sorting
4. Verify timing for all column types (DATE, NUMBER, TEXT)

### Phase 2: Medium Load (100-200 rows)
1. Click "Add 50 Rows" 2-4 times
2. Note scaling behavior
3. Test sorting at each increment
4. Confirm UI remains responsive

### Phase 3: Heavy Load (300-500 rows)
1. Continue adding 50-row chunks
2. Identify the point where sorting becomes sluggish
3. Note the maximum practical row count for 7MHz

### Phase 4: Cleanup Verification
1. Click "Remove 50 Rows" until back to 50 rows
2. Close window
3. Check memory report for leaks

## Expected Bottleneck

On a 7MHz Amiga 68000, the limiting factor will be:
- **Merge sort** for large datasets (CPU-bound)
- **String operations** during formatting (memory bandwidth)
- **ListView refresh** for very tall lists (OS overhead)

Typical practical limit: **150-300 rows** before sorting feels too slow.

## Column Configuration

The test uses these column settings:

```c
columns[0]: Date/Time - 17 chars, LEFT align, DATE type
columns[1]: # - 4-6 chars, RIGHT align, NUMBER type
columns[2]: Type - 8 chars, LEFT align, TEXT type
columns[3]: Name - 15-200 chars, LEFT align, TEXT type, FLEXIBLE
columns[4]: Rating - 6 chars, CENTER align, NUMBER type
```

**Flexible column**: Column 3 expands to fill remaining space.

## Data Generation

- 50 predefined Amiga hardware/game names (rotated)
- Random dates from 1985-2009
- Random ratings from 5-10
- 3 categories: Hardware, Game, Software

Data is deterministic (same sequence each run for reproducibility).

## Memory Tracking

If `DEBUG_MEMORY_TRACKING` is enabled in `platform/platform.h`, the test will:
- Log all allocations/deallocations
- Report leaks at program exit
- Show peak memory usage

Expected memory usage:
- 50 rows: ~10-15 KB
- 100 rows: ~20-30 KB
- 200 rows: ~40-60 KB

## Files

- **Source**: `src/tests/listview_stress_test.c`
- **Binary**: `Bin/Amiga/listview_stress_test`
- **Build**: `make test-listview`
- **Dependencies**:
  - `src/helpers/listview_columns_api.c/h`
  - `src/path_utilities.c/h`
  - `src/writeLog.c/h`
  - `src/Settings/IControlPrefs.c/h`
  - `platform/platform.c/h`

## Troubleshooting

### "Failed to load IControl preferences"
- Workbench 3.0+ not running
- IControl prefs file missing
- Run from Workbench to ensure environment is set up

### No timing output visible
- Run from Shell, not Workbench
- Timing is printed to stdout (Shell window only)

### Window doesn't open
- Check memory available (test needs ~2MB RAM minimum)
- Ensure Workbench screen is available

### Memory leaks reported
- Bug in cleanup code - report to developer
- Check that all 3 cleanup functions are called:
  1. `itidy_free_listview_entries()`
  2. `iTidy_FreeFormattedList()`
  3. `iTidy_FreeListViewState()`

## Performance Comparison

Use this test to compare:
- **68000 @ 7MHz** vs **68020/030 @ 14MHz+**
- **Workbench 3.0** vs **3.1** vs **3.2**
- **FastRAM** vs **ChipRAM** allocation
- **Different ListView heights** (10 vs 20 vs 30 visible rows)

## Future Enhancements

Potential additions to this test:
- [ ] Configurable initial row count
- [ ] CSV export of timing data
- [ ] Multiple test runs with average/min/max
- [ ] Graphical progress bar during operations
- [ ] Memory usage meter in window title
- [ ] Configurable number of columns
- [ ] Stress test with very long strings (path abbreviation)

## Conclusion

This test answers the question: **"How many items can the ListView API handle before it becomes too slow on a 7MHz Amiga?"**

Use the timing output to determine the practical limits for your target hardware configuration.

**Expected result**: The API should handle 150-200 rows comfortably on 7MHz, with sorting taking 200-400ms. Beyond that, consider pagination or virtual scrolling for very large datasets.
