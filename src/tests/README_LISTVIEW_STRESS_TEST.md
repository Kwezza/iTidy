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
- **Three buttons**:
  - "Add 50 Rows" - Appends 50 more test rows
  - "Remove 50 Rows" - Removes 50 rows from the bottom
  - "Auto Run" - **NEW!** Automated benchmark: adds rows in 50-row increments up to 1000, sorting column 0 (Date) after each addition
- **Click-to-sort** enabled on all column headers
- **Performance timing** output to console and log files

## Features Tested

1. **Initial Load**: 50 rows of random Amiga hardware/game facts
2. **Dynamic Addition**: Add 50-row chunks to see scaling behavior
3. **Dynamic Removal**: Remove 50-row chunks to reduce size
4. **Sorting**: Click column headers to sort by different criteria
5. **Automated Benchmark**: Click "Auto Run" button to run full test sequence (100-1000 rows, sorting after each addition)
6. **Multiple Data Types**: Tests DATE, NUMBER, and TEXT sorting/formatting
7. **Memory Tracking**: Detects memory leaks (if DEBUG_MEMORY_TRACKING enabled)

## Auto Run Feature

The **Auto Run** button (new in this version) provides a fully automated benchmark sequence:

### What It Does
1. Starts from current row count (typically 50)
2. Adds 50 rows at a time
3. After each addition, sorts by column 0 (Date/Time, descending)
4. Continues up to **1000 rows total**
5. Reports timing for both add and sort operations at each milestone
6. All timing data logged to performance log file

### Why Use Auto Run?
- **Consistency**: Exact same test sequence every time
- **Completeness**: Tests all the way to 1000 rows automatically
- **Convenience**: No need to manually click "Add 50" and column headers 19 times
- **Perfect for real hardware**: Start the test and walk away - check results when done
- **Logging**: Full benchmark data captured in log files for analysis

### Example Output
```
================================================
  AUTOMATED BENCHMARK SEQUENCE STARTED
  Target: 1000 rows in 50-row increments
  Sorting column 0 (Date) after each addition
================================================

--- Milestone: 100 rows ---
Adding 50 rows (current: 50)...
Added 50 rows in 0.823 seconds
Sorting 100 rows by Date/Time (column 0)...
  [TIMING] Sort completed in 0.156 seconds (100 rows)

--- Milestone: 150 rows ---
Adding 50 rows (current: 100)...
Added 50 rows in 1.234 seconds
Sorting 150 rows by Date/Time (column 0)...
  [TIMING] Sort completed in 0.289 seconds (150 rows)

[continues to 1000 rows...]

================================================
  AUTOMATED BENCHMARK COMPLETED
  Final row count: 1000
  Check logs for detailed timing breakdown
================================================
```

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

## Log Files (For Real Hardware Testing)

When running on real hardware, the test creates detailed log files in `Bin/Amiga/logs/`:

### Performance Log (performance_YYYY-MM-DD_HH-MM-SS.log)

**Example Content:**
```
[2025-11-20 15:42:13] [PERF] [ListView Format] Operation completed in 45123 μs
[2025-11-20 15:42:13] [PERF]   ├─ Sort phase: 23456 μs (51.98%)
[2025-11-20 15:42:13] [PERF]   ├─ Width calculation: 8901 μs (19.72%)
[2025-11-20 15:42:13] [PERF]   └─ Format phase: 12766 μs (28.30%)
[2025-11-20 15:42:13] [PERF] [ListView Format] Total entries: 100
[2025-11-20 15:42:19] [PERF] [ListView Resort] Operation completed in 34567 μs
[2025-11-20 15:42:19] [PERF]   ├─ Sort phase: 28123 μs (81.36%)
[2025-11-20 15:42:19] [PERF]   └─ Rebuild phase: 6444 μs (18.64%)
[2025-11-20 15:42:19] [PERF] [ListView Resort] Total entries: 100
```

### What to Extract from Logs

For each row count milestone (50, 100, 150, 200, 250, 300, etc.), extract:

1. **Add 50 Rows** - Total time from `[PERF] [ListView Format]` line
2. **Sort (Column 0)** - Total time from `[PERF] [ListView Resort]` line
3. **Remove 50 Rows** - Time shown in console output

**Example Data Collection:**
```
100 rows: Add=45.1ms, Sort=34.6ms
150 rows: Add=67.8ms, Sort=52.3ms
200 rows: Add=91.2ms, Sort=71.5ms
```

### Memory Log (memory_YYYY-MM-DD_HH-MM-SS.log)

**Example Content:**
```
[2025-11-20 15:42:13] [MEM] whd_malloc: 4096 bytes at 0x07F42000 (src/helpers/listview_columns_api.c:156)
[2025-11-20 15:42:13] [MEM] whd_malloc: 1024 bytes at 0x07F43000 (src/helpers/listview_columns_api.c:245)
[2025-11-20 15:42:19] [MEM] whd_free: 1024 bytes at 0x07F43000 (src/helpers/listview_columns_api.c:389)
```

**Check for Memory Leaks:**
- At program exit, all `whd_malloc` calls should have matching `whd_free` calls
- Memory report will show "NO LEAKS DETECTED" if clean

### Errors Log (errors_YYYY-MM-DD_HH-MM-SS.log)

This file consolidates all ERROR/WARNING messages. Should be empty for successful tests.

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

## Real Hardware Testing Guide

### Critical: Detecting Fast RAM Usage

The benchmark results are **dramatically different** depending on whether the program uses Fast RAM:

- **Chip RAM only**: 300 rows takes ~75 seconds (SLOW)
- **Fast RAM enabled**: 300 rows takes ~5 seconds (15x faster!)

**How to Verify Fast RAM Access:**

1. Check system configuration:
   ```
   Avail CHIP  ; Shows Chip RAM available
   Avail FAST  ; Shows Fast RAM available
   ```

2. **CRITICAL**: The program uses `AllocVec(MEMF_ANY)` to access Fast RAM automatically
   - If Fast RAM exists, the system will use it
   - If only Chip RAM exists, that will be used instead

3. **Test Both Configurations** (if your A500+ has Fast RAM expansion):
   - **Test 1**: Run with Fast RAM installed
   - **Test 2**: Temporarily remove Fast RAM expansion to test chip-only baseline
   - This shows the **real-world impact** of Fast RAM on performance

### Testing Checklist for A500+ @ 7MHz

Your A500+ has the same CPU as the original benchmarks (68000 @ 7MHz). This is the **reference baseline** configuration.

**Test Scenarios:**

1. **Stock A500+ (Chip RAM only - typically 1MB or 512KB):**
   - Expected: Similar to WinUAE Configuration 2 results
   - 100 rows: ~4-5 seconds to add
   - 200 rows: ~13-15 seconds to add
   - 300 rows: ~70-80 seconds to add (VERY SLOW)

2. **A500+ with Fast RAM expansion (e.g., 512KB or 4MB Fast RAM):**
   - Expected: Similar to WinUAE Configuration 3 results (post-MEMF_ANY fix)
   - 100 rows: ~0.8 seconds to add
   - 200 rows: ~2.3 seconds to add
   - 300 rows: ~5.0 seconds to add (15x faster than chip-only!)

### Step-by-Step Real Hardware Test Protocol

**IMPORTANT**: Run tests from **HARD DISK** and **RAM:** to compare logging overhead.

#### Test 1: DISK-Based Test (Default)

1. Copy `listview_stress_test` to your hard disk (e.g., `DH0:iTidy/`)
2. Open Shell and navigate to the directory
3. Run: `listview_stress_test`
4. **OPTION A - Manual Testing**:
   - Start with 50 rows (initial)
   - Click "Add 50 Rows" to reach 100 rows → Note time in console
   - Click column header to sort → Note time in console
   - Click "Add 50 Rows" to reach 150 rows → Note time
   - Click column header to sort → Note time
   - Continue up to 300-500 rows (or until too slow)
5. **OPTION B - Automated Testing** (RECOMMENDED):
   - Click "Auto Run" button
   - Wait for completion (up to 1000 rows on fast systems)
   - All timing data automatically logged
6. Close window
7. Copy log files from `Bin/Amiga/logs/` to safe location
8. Rename logs with suffix `_DISK` (e.g., `performance_2025-11-20_DISK.log`)

#### Test 2: RAM-Based Test (Faster, measures logging overhead)

1. Copy entire `Bin/Amiga/` directory to `RAM:` (including logs folder)
2. Run: `RAM:listview_stress_test`
3. **OPTION A - Manual**: Perform EXACT same sequence as Test 1
4. **OPTION B - Automated**: Click "Auto Run" button (same as disk test)
5. Close window
6. Copy log files from `RAM:logs/` to safe location
7. Rename logs with suffix `_RAM` (e.g., `performance_2025-11-20_RAM.log`)

#### Comparing DISK vs RAM Results

**Example from A600 68030 @ 50MHz:**
```
300 rows - DISK test:
  Add: 532ms, Sort: 957ms

300 rows - RAM test:
  Add: 485ms, Sort: 930ms

Overhead: ~9% slower from disk (logging file I/O impact)
```

If DISK vs RAM difference is < 10%, logging overhead is **negligible**.

### Data Collection for Benchmarking

Create a simple text file with results at each milestone:

**Format:**
```
A500+ Benchmark Results
CPU: 68000 @ 7.09MHz
RAM: [e.g., "512KB Chip only" OR "512KB Chip + 4MB Fast"]
OS: Workbench 3.1
Test Location: [DISK or RAM]
Date: 2025-11-20

50 rows (initial):
  Add: N/A (initial load)
  Format: XX.XXX ms
  Sort (col 0): XX.XXX ms

100 rows:
  Add 50: XX.XXX ms
  Sort (col 0): XX.XXX ms

150 rows:
  Add 50: XX.XXX ms
  Sort (col 0): XX.XXX ms

200 rows:
  Add 50: XX.XXX ms
  Sort (col 0): XX.XXX ms

250 rows:
  Add 50: XX.XXX ms
  Sort (col 0): XX.XXX ms

300 rows:
  Add 50: XX.XXX ms
  Sort (col 0): XX.XXX ms
```

### Reference Comparison Data

**From WinUAE and Real Hardware Testing:**

| Configuration | 100 Rows Add | 200 Rows Add | 300 Rows Add | 300 Rows Sort |
|---------------|--------------|--------------|--------------|---------------|
| 68000 @ 7MHz (Chip only) | 4.2s | 13.7s | 75.0s | ~40s |
| 68000 @ 7MHz (Fast RAM) | 0.8s | 2.3s | 5.0s | 9.1s |
| 68030 @ 50MHz (Fast RAM) | 0.1s | 0.3s | 0.5s | 0.9s |

Your A500+ should match the first two configurations closely!

### What to Report

After testing, provide:
1. **Hardware specs**: CPU, clock speed, RAM configuration
2. **OS version**: Workbench version (e.g., 3.1)
3. **Test location**: DISK or RAM
4. **Log files**: Copy of performance log showing timings
5. **Observations**: At what row count did it feel too slow?
6. **Fast RAM impact**: If tested both chip-only and with Fast RAM, show comparison

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
