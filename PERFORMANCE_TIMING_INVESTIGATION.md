# Performance Timing Investigation - ListView Formatter

## Date: November 19, 2025

## Current Performance Logging System

### Configuration
- **Global Flag**: `enable_performance_logging` in `LayoutPreferences` structure
- **Location**: `src/layout_preferences.h` line 160
- **UI Control**: Beta Options Window (`src/GUI/beta_options_window.c`)
  - Checkbox gadget: "Enable performance timing logs"
  - Gadget ID: `GID_BETA_PERFORMANCE_LOG`
  - Applied immediately via `set_performance_logging_enabled()`

### API Functions (in `src/writeLog.h` and `src/writeLog.c`)
```c
/* Enable/disable performance timing logging */
void set_performance_logging_enabled(BOOL enabled);

/* Check if performance logging is enabled */
BOOL is_performance_logging_enabled(void);
```

### Implementation Details
- **Global variable**: `g_performanceLoggingEnabled` (static BOOL in writeLog.c)
- **Timer device**: Uses `TimerBase` and `GetSysTime()` for microsecond precision
- **Timing structure**: `struct timeval` with `tv_secs` and `tv_micro` fields

### Existing Usage Examples

#### 1. Icon Loading (`src/icon_management.c` lines 155-830)
```c
struct timeval startTime, endTime;
ULONG elapsedMicros, elapsedMillis;

if (TimerBase) {
    GetSysTime(&startTime);
}

// ... icon loading operations ...

if (TimerBase) {
    GetSysTime(&endTime);
    
    elapsedMicros = ((endTime.tv_secs - startTime.tv_secs) * 1000000) +
                    (endTime.tv_micro - startTime.tv_micro);
    elapsedMillis = elapsedMicros / 1000;
    
    if (is_performance_logging_enabled()) {
        append_to_log("==== ICON LOADING PERFORMANCE ====");
        append_to_log("CreateIconArrayFromPath() execution time:");
        append_to_log("  %lu microseconds (%lu.%03lu ms)\n", 
                      elapsedMicros, elapsedMillis, elapsedMicros % 1000);
        append_to_log("  Icons loaded: %d\n", iconArray->size);
        if (iconArray->size > 0) {
            append_to_log("  Time per icon: %lu microseconds\n", 
                          elapsedMicros / iconArray->size);
        }
        append_to_log("  Folder: %s", dirPath);
        append_to_log("==================================");
        
        printf("  [TIMING] Icon loading: %lu.%03lu ms for %lu icons\n",
               elapsedMillis, elapsedMicros % 1000, (unsigned long)iconArray->size);
    }
}
```

#### 2. Aspect Ratio Layout (`src/aspect_ratio_layout.c` lines 485-510)
```c
if (is_performance_logging_enabled()) {
    append_to_log("\n==== FLOATING POINT PERFORMANCE ====\n");
    append_to_log("CalculateLayoutWithAspectRatio() execution time:\n");
    append_to_log("  %lu microseconds (%lu.%03lu ms)\n", 
                  elapsedMicros, elapsedMillis, elapsedMicros % 1000);
    append_to_log("  Icons processed: %d\n", iconArray->size);
    if (iconArray->size > 0) {
        append_to_log("  Time per icon: %lu microseconds\n", 
                      elapsedMicros / iconArray->size);
    }
    append_to_log("====================================\n\n");
    
    printf("  [TIMING] Aspect ratio calculation: %lu.%03lu ms for %lu icons\n",
           elapsedMillis, elapsedMicros % 1000, (unsigned long)iconArray->size);
}
```

---

## ListView Formatter Performance Hot Spots

### File: `src/GUI/listview_formatter.c`

#### Critical Operations to Measure (7MHz 68000 concerns)

##### 1. **Initial Sorting** (Line ~760)
```c
if (default_sort_col >= 0 && entries) {
    BOOL ascending = (columns[default_sort_col].default_sort == ITIDY_SORT_ASCENDING);
    iTidy_SortListViewEntries(entries, default_sort_col, columns[default_sort_col].sort_type, ascending);
}
```
**Why measure**: Merge sort with string comparisons on 7MHz = potentially slow
**Expected impact**: O(n log n) with strcmp() calls

##### 2. **Column Width Calculation** (Line ~780)
```c
if (!iTidy_CalculateColumnWidths(columns, num_columns, entries, 
                                 total_char_width, col_widths)) {
```
**Why measure**: Iterates all entries × all columns to measure string lengths
**Expected impact**: O(n × m) where n=entries, m=columns

##### 3. **Entry Formatting Loop** (Lines ~860-890)
```c
for (entry_node = entries->lh_Head; entry_node->ln_Succ; entry_node = entry_node->ln_Succ) {
    entry = (iTidy_ListViewEntry *)entry_node;
    pos = 0;
    
    for (col = 0; col < num_columns; col++) {
        const char *cell_data = ...;
        format_cell(cell_buffer, cell_data, col_widths[col], columns[col].align, columns[col].is_path);
        strcpy(row_buffer + pos, cell_buffer);
        // ...
    }
    
    // Store in ln_Name
    entry->node.ln_Name = (char *)whd_malloc(strlen(row_buffer) + 1);
    strcpy(entry->node.ln_Name, row_buffer);
    
    // Create display node
    node = create_display_node(row_buffer);
    AddTail(list, node);
}
```
**Why measure**: 
- Multiple string operations per cell
- Memory allocations per row
- Path abbreviation logic if `is_path=TRUE`
**Expected impact**: O(n × m) with strlen/strcpy overhead

##### 4. **Re-sort Operation** (`iTidy_ResortListViewByClick`, line ~1330)
```c
/* Sort the entry list in-place */
iTidy_SortListViewEntries(entry_list, clicked_col, columns[clicked_col].sort_type, ascending);

/* Rebuild formatted list */
while ((node = RemHead(formatted_list)) != NULL) {
    // Free old nodes
}

for (entry_node = entry_list->lh_Head; entry_node->ln_Succ; entry_node = entry_node->ln_Succ) {
    // Re-format and re-add
}
```
**Why measure**: 
- Full re-sort
- Complete display list rebuild
- Memory deallocation/allocation churn
**Expected impact**: O(n log n) sort + O(n × m) rebuild

##### 5. **Merge Sort Algorithm** (`iTidy_SortListViewEntries`, lines ~250-400)
```c
void iTidy_SortListViewEntries(struct List *list, int col, iTidy_ColumnType type, BOOL ascending)
{
    // Recursive merge sort
    // Splits list in half repeatedly
    // Calls compare_entries() for each comparison
}
```
**Why measure**: Core sorting algorithm with recursive overhead
**Expected impact**: String comparisons on 7MHz are expensive

##### 6. **Path Abbreviation** (if enabled, in `format_cell`)
```c
if (is_path && len > width) {
    if (iTidy_ShortenPathWithParentDir(text, path_abbreviated, width)) {
        display_text = path_abbreviated;
    }
}
```
**Why measure**: Complex string parsing and manipulation
**Expected impact**: Varies by path depth and length

---

## Recommended Timing Points

### For `iTidy_FormatListViewColumns()`:
1. **START**: Beginning of function
2. **CHECKPOINT 1**: After initial sort (if applicable)
3. **CHECKPOINT 2**: After column width calculation
4. **CHECKPOINT 3**: After entry formatting loop
5. **END**: Before return

### For `iTidy_ResortListViewByClick()`:
1. **START**: After parameter validation
2. **CHECKPOINT 1**: After iTidy_SortListViewEntries()
3. **CHECKPOINT 2**: After display list rebuild
4. **END**: Before return

### For `iTidy_SortListViewEntries()`:
1. **START**: Entry to function
2. **END**: Before return
3. **Additional**: Count of compare_entries() calls (if feasible)

### For `iTidy_CalculateColumnWidths()`:
1. **START**: Entry to function
2. **END**: Before return

---

## Performance Concerns on 7MHz 68000

### High-Cost Operations:
1. **String operations**: `strlen()`, `strcmp()`, `strcpy()`, `strncpy()`
   - No string cache, no SIMD, pure byte-by-byte
2. **Memory allocation**: `whd_malloc()` / `whd_free()`
   - Heap fragmentation checks, linked list traversal
3. **Recursive calls**: Merge sort recursion overhead
   - Stack frame setup/teardown, no tail-call optimization
4. **String comparisons in sort**: 
   - Especially DATE columns with "YYYYMMDD_HHMMSS" format
   - NUMBER columns with atoi() parsing

### Optimization Opportunities (if timing reveals issues):
1. **Cache strlen() results** instead of recalculating
2. **Pre-allocate buffers** instead of malloc per row
3. **Batch operations** to reduce function call overhead
4. **Consider insertion sort** for small lists (< 10 items)
5. **Lazy formatting** - format only visible rows (future enhancement)

---

## Implementation Strategy

### Phase 1: Add Timing Infrastructure
- Add timing variables to key functions
- Wrap operations with GetSysTime() calls
- Conditional logging based on `is_performance_logging_enabled()`

### Phase 2: Test on Target Hardware
- Enable performance logging in Beta Options
- Test with varying entry counts: 5, 10, 25, 50, 100
- Test with different column types (TEXT, NUMBER, DATE)
- Test with path abbreviation enabled vs disabled

### Phase 3: Analyze Results
- Identify bottlenecks (sort vs format vs width calc)
- Determine if optimization is needed
- Prioritize based on actual measurements, not assumptions

### Phase 4: Optimize (if needed)
- Focus on highest-impact operations first
- Re-test after each optimization
- Document performance improvements

---

## Expected Timing Ranges (Rough Estimates for 7MHz 68000)

| Operation | 10 Entries | 50 Entries | 100 Entries |
|-----------|-----------|-----------|-------------|
| Initial Format | 50-100ms | 200-400ms | 500-1000ms |
| Re-sort (same column) | 20-50ms | 100-200ms | 300-600ms |
| Re-sort (different column) | 20-50ms | 100-200ms | 300-600ms |
| Width Calculation | 10-20ms | 50-100ms | 100-200ms |
| Entry Formatting | 30-60ms | 150-300ms | 400-800ms |

**Note**: These are ESTIMATES. Actual timing will vary based on:
- String lengths (longer = slower)
- Column count (more columns = slower)
- Path abbreviation (enabled = slower)
- Memory fragmentation (more = slower)

---

## Files Requiring Modification

1. `src/GUI/listview_formatter.c` - Add timing to key functions
2. (No other files - uses existing performance logging system)

---

## Next Steps

1. ✅ Investigation complete (this document)
2. ⏳ Add timing code to ListView formatter functions
3. ⏳ Test on WinUAE with 7MHz 68000 configuration
4. ⏳ Analyze results and determine optimization needs
5. ⏳ Update documentation with actual performance data
