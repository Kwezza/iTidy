# DefIcons Creator - Test Program

## Purpose

A CLI test program that creates `.info` files for iconless files by querying DefIcons and copying appropriate icon templates. This demonstrates the core functionality needed for iTidy's DefIcons integration.

## Features

- **Automatic icon creation** - Creates .info files for files without icons
- **DefIcons port integration** - Queries DefIcons for accurate file type identification
- **Parent chain resolution** - Uses parent type templates when exact matches unavailable
- **Skip existing icons** - Never overwrites existing .info files
- **Batch processing** - Processes entire directories in one pass
- **Optimized two-stage API** - Query DefIcons once, filter by type, then create icon
- **Type filtering** - User preferences to exclude specific file types (e.g., tools)
- **Statistics reporting** - Shows created/skipped/filtered/failed counts

## Usage

```shell
deficons_creator <folder_path>
```

### Examples

```shell
deficons_creator Work:Documents
deficons_creator Work:Music
deficons_creator RAM:TestFiles
```

## How It Works (Optimized Two-Stage Flow)

### Performance Optimization

**Key Design**: Query DefIcons only ONCE per file, then use the type token for filtering and creation. This avoids expensive double-queries when user preferences filter out certain types.

### For each file in the directory:

**Stage 1: Identify** (Query DefIcons once)
1. **Query DefIcons** - Send ARexx "Identify" command to get file type token
   - Example: `Silkworm_intro.mod` → token = "mod"
   - This is the ONLY DefIcons query for this file

**Stage 2: Filter** (Cheap string comparison)
2. **Check user preferences** - Should we create icons for this type?
   - Example: Skip if type is "tool" (saves CPU for SYS:C)
   - Fast string comparison, no DefIcons query needed
   - If filtered, skip to next file
FILTERED: PlayMod (type 'tool' excluded by preferences)
  SKIP: document.txt (no template for 'ascii')
  CREATED: photo.gif.info (from exact, type 'gif')

================================================================================
Files processed: 50
Icons created:   45
Icons skipped:   3 (already exist)
Icons filtered:  2 (type excluded by preferences)
Icons failed:    0
================================================================================

Performance: Each file queried DefIcons only ONCE
Type filtering done via cheap string comparison
7. **Report result** - Display success/failure message

### CPU Savings Example

For a folder with 100 files where 30 are tools:

| Approach | DefIcons Queries | Notes |
|----------|------------------|-------|
| **Naive** | 140 queries | Query once to check, query again to create |
| **Optimized** | 100 queries | Query once, filter cheaply, reuse token |
| **Savings** | **28% reduction** | More savings with more filtering |

### ARexx Communication Method: Direct RexxMsg (Option B)

**CRITICAL PERFORMANCE DETAIL**: This implementation uses **direct in-process ARexx messaging**, NOT the `rx` command-line tool.

#### Two ARexx Approaches:

**Option A: Spawn rx process (SLOW)** ❌
```shell
# For EACH file, spawn a new process:
rx "ADDRESS DEFICONS Identify 'Work:Music/song.mod'"
```
- **Cost per file**: Fork new process, load rx binary, parse script, send message, wait, cleanup
- **Performance**: ~50-100ms per file on 68020 (mostly process spawn overhead)
- **For 1000 files**: 50-100 seconds just in process spawning!

**Option B: Direct RexxMsg (FAST)** ✅ **<- This implementation**
```c
// One-time setup:
deficons_port = FindPort("DEFICONS");

// Per file (no process spawn):
reply_port = CreateMsgPort();
rxmsg = CreateRexxMsg(reply_port, NULL, NULL);
rxmsg->rm_Args[0] = CreateArgstring("Identify \"path\"", len);
PutMsg(deficons_port, rxmsg);
WaitPort(reply_port);
// ... handle reply ...
DeleteRexxMsg(rxmsg);
DeleteMsgPort(reply_port);
```
- **Cost per file**: Create message, send, wait for reply, cleanup (~1-5ms)
- **Performance**: **10-50x faster** than spawning rx
- **For 1000 files**: 1-5 seconds (vs 50-100 seconds with rx spawning)

#### Implementation in Test2

See `query_deficons()` in [deficons_creator.c](deficons_creator.c):

```c
static BOOL query_deficons(const char *filepath, char *result, int result_size)
{
    struct MsgPort *reply_port = NULL;
    struct RexxMsg *rxmsg = NULL;
    
    // Find DefIcons port (done once at startup)
    if (!deficons_port || !RexxSysBase) {
        return FALSE;
    }
    
    // Create reply port for this query
    reply_port = CreateMsgPort();
    
    // Build REXX message with "Identify" command
    rxmsg = CreateRexxMsg(reply_port, NULL, NULL);
    rxmsg->rm_Args[0] = CreateArgstring("Identify \"path\"", strlen(...));
    rxmsg->rm_Action = RXFF_RESULT;
    
    // Send message directly to DefIcons port (no process spawn!)
    PutMsg(deficons_port, (struct Message *)rxmsg);
    
    // Wait for reply
    WaitPort(reply_port);
    reply = (struct RexxMsg *)GetMsg(reply_port);
    
    // Extract result string
    if (reply && reply->rm_Result1 == 0 && reply->rm_Result2) {
        strncpy(result, (char *)reply->rm_Result2, result_size - 1);
        DeleteArgstring((STRPTR)reply->rm_Result2);
    }
    
    // Cleanup
    DeleteArgstring(rxmsg->rm_Args[0]);
    DeleteRexxMsg(rxmsg);
    DeleteMsgPort(reply_port);
}
```

#### Why This Matters

For iTidy processing large directory trees:

| Files | Option A (rx spawn) | Option B (RexxMsg) | Speedup |
|-------|--------------------|--------------------|---------|
| 100 files | 5-10 seconds | 0.1-0.5 seconds | **10-50x** |
| 1000 files | 50-100 seconds | 1-5 seconds | **10-50x** |
| 10000 files | 8-16 minutes | 10-50 seconds | **10-50x** |

**Design Spec Reference**: This implements "Option B: direct ARexx message" from [iTidy_DefIcons_Icon_Creation_Flow.md](../../../docs/iTidy_DefIcons_Icon_Creation_Flow.md) section 2.3.

### Libraries Required

For direct RexxMsg communication:
```c
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <proto/rexxsyslib.h>

struct RxsLib *RexxSysBase;  // Must open rexxsyslib.library

// At startup:
RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 0);
deficons_port = FindPort("DEFICONS");  // Find DefIcons once
```

### Output Format

```
Processing: Work:Music

  CREATED: Silkworm_intro.mod.info (from parent, type 'mod')
  CREATED: Speedball2_intro.mod.info (from parent, type 'mod')
  SKIP: document.txt (no template for 'ascii')
  CREATED: photo.gif.info (from exact, type 'gif')

================================================================================
Files processed: 50
Icons created:   45
Icons skipped:   3 (already exist)
Icons failed:    2
================================================================================
```

## Key Differences from Test1 (Scanner)

| Feature | Test1 (Scanner) | Test2 (Creator) |
|---------|----------------|----------------|
| **Action** | Display info only | Create .info files |
| **Output** | Table of all files | Only processed files |
| **Skipping** | Processes all files | Skips existing icons |
| **File I/O** | Read-only | Creates new files |
| **Statistics** | File count only | Created/skipped/failed counts |

## Prerequisites

1. **DefIcons must be running** - Required for file type identification
2. **deficons.prefs must exist** - Required for parent chain resolution
3. **Icon templates** - At least some def_*.info files must be available
4. **Write permission** - Directory must be writable

## Building

```shell
cd Tests/DefIcons/Test2
make clean
make
```

Output: `Bin/Amiga/Tests/DefIcons/deficons_creator`

## Safety Features

- **Never overwrites** - Existing .info files are preserved
- **Error handling** - Gracefully handles missing templates or copy failures
- **Optimized Two-Stage API

The implementation uses three separate functions to avoid double-querying DefIcons:

```c
/*
 * Stage 1: Identify file type (Query DefIcons ONCE)
 * Returns type token for use in filtering and creation
 */
BOOL identify_file_type(const char *filepath, char *type_token, int token_size)
{
    return query_deficons(filepath, type_token, token_size);
}

/*
 * Stage 2: Check user preferences (Cheap string comparison)
 * Returns TRUE if this type should have an icon created
 * No DefIcons query - just compares against blacklist/whitelist
 */
BOOL should_create_icon_for_type(const char *type_token)
{
    // Example blacklist: tool, device, filesystem, handler, library
    if (strcmp(type_token, "tool") == 0) return FALSE;
    // ... other filters ...
    return TRUE;  // Allow everything else
}

/*
 * Stage 3: Create icon using cached type token
 * No DefIcons query - reuses the type token from Stage 1
 */
BOOL create_icon_with_type(const char *filepath, const char *type_token)
{
    // 1. Check if filepath.info exists → skip
    // 2. Find template using type_token → walk parent chain
    // 3. Copy template → create .info
    // 4. Report result → statistics
}
```

### Main Processing Loop

```c
void process_directory(const char *path)
{
    while (ExNext(lock, fib)) {
        char type_token[128];
        
        // Stage 1: Query DefIcons ONCE
        if (!identify_file_type(fullpath, type_token, sizeof(type_token))) {
            continue;  // Cannot identify
        }
        
        // Stage 2: Check preferences (no DefIcons query)
        if (!should_create_icon_for_type(type_token)) {
            icons_filtered++;
            continue;  // User doesn't want this type
        }
        
        // Stage 3: Create icon (re**exact pattern** specified in [iTidy_DefIcons_Icon_Creation_Flow.md](../../../docs/iTidy_DefIcons_Icon_Creation_Flow.md):

### iTidy API (Production Functions)

Based on this test, iTidy should implement:

```c
/*
 * One-time initialization (call at iTidy startup)
 * - Parse deficons.prefs to build type hierarchy
 * - Find DefIcons port
 * - Build template cache
 */
BOOL itidy_deficons_init(void);

/*
 * Stage 1: Identify file type (query DefIcons ONCE)
 * Maps to design spec: "Call DefIcons Identify via ARexx"
 */
BOOL itidy_identify_file_type(const char *filepath, char *type_token, int token_size);

/*
 * Stage 2: Check user preferences (cheap string comparison)
 * Maps to design spec: "Apply user filtering preferences"
 * Example: User option "Do not create icons for tools"
 */
BOOL itidy_should_create_icon_for_type(const char *type_token, 
                                        const LayoutPreferences *prefs);

/*
 * Stage 3: Create icon using pre-determined type
 * Maps to design spec: "Resolve token to template, walk parent chain"
 * Does NOT re-query DefIcons - reuses type from Stage 1
 */
BOOL itidy_create_icon_with_type(const char *filepath, const char *type_token);

/*
 * Cleanup (call at iTidy shutdown)
 */
void itidy_deficons_cleanup(void);
```

### Integration in layout_processor.c

The design spec recommends an **optional pre-pass** before tidying:

```c
void process_directory_layout(const char *path, const LayoutPreferences *prefs)
{
    /* Optional Phase 1: Create missing icons (NEW) */
    if (prefs->create_missing_icons) {
        for (each entry in directory) {
            char type_token[128];
            
            // Skip if icon exists
            if (entry_has_icon(entry)) continue;
            
            // Stage 1: Query DefIcons ONCE
            if (!itidy_identify_file_type(entry_path, type_token, sizeof(type_token))) {
                continue;
            }
            
            // Stage 2: Check user preferences (no DefIcons query)
            if (!itidy_should_create_icon_for_type(type_token, prefs)) {
                continue;  // Type filtered by user
            }
            
            // Stage 3: Create icon (reuses type_token, no DefIcons query)
            itidy_create_icon_with_type(entry_path, type_token);
        }
    }
    
    /* Existing Phase 2: Tidy layout (works on ALL icons now) */
    existing_tidy_logic(path, prefs);
}
```

### User Preferences Structure

Add to `LayoutPreferences` (in `layout_preferences.h`):

```c
typedef struct {
    /* Existing iTidy preferences... */
    
    /* DefIcons integration options */
    BOOL create_missing_icons;           /* Enable DefIcons icon creation */
    BOOL filter_tool_icons;              /* Skip tool/device/handler types */
    BOOL filter_ascii_icons;             /* Skip text/ascii types */
    BOOL filter_source_icons;            /* Skip c/h/asm source code types */
    /* Add more category filters as needed */
} LayoutPreferences;
```

### Key Design Principles (from spec)

1. ✅ **Query DefIcons ONCE** - Implemented via `identify_file_type()`
2. ✅ **Cache token** - Type token passed between stages
3. ✅ **User filtering** - `should_create_icon_for_type()` checks preferences
4. ✅ **Parent chain resolution** - `find_icon_template()` walks hierarchy
5. ✅ **Never overwrite** - Checks if `.info` exists before creating
6. ✅ **Optional pre-pass** - Only runs if user enables feature
7. ✅ **No visual flashing** - Direct file copy, no Workbench windows opened

### Performance Characteristics

| Metric | Test2 Implementation | Design Spec Requirement |
|--------|---------------------|-------------------------|
| DefIcons queries per file | 1 | "Minimize ARexx calls" ✅ |
| Type filtering cost | String comparison | "Cheap heuristics" ✅ |
| Parent chain lookup | In-memory hash | "Cache at two levels" ✅ |
| Template search | Lock() check | "Build set of available templates" ✅ |
| Icon creation | Binary file copy | "Copy template to entry.info" ✅ | // Open source template
    // Open destination .info
    // Copy in 8KB chunks
    // Return success/failure
    // 1. Check if filepath.info exists → skip
    // 2. Query DefIcons → get type token
    // 3. Find template → walk parent chain
    // 4. Copy template → create .info
    // 5. Report result → statistics
}
```

## Known Limitations

- **Files only** - Does not process directories/drawers
- **No recursion** - Single directory only
- **No default tool** - Copied icons use template's default tool
- **No position** - Icon positions are from template (typically 0,0)
- **Binary copy** - No icon modification, pure file copy

## Future Enhancements for iTidy Integration

1. **Set default tool** - Use `PutDiskObject()` to set appropriate default tool
2. **Smart positioning** - Calculate icon positions for grid layout
3. **Recursive processing** - Handle directory trees
4. **Filtering** - Skip system files, hidden files, etc.
5. **Performance** - Cache template lookups, reuse ARexx messages
6. **Preferences** - User control over which file types to process

## Testing Procedure

1. Create test directory with various file types (.mod, .gif, .txt, etc.)
2. Run `deficons_creator` on the directory
3. Verify .info files created for iconless files
4. Run again - verify existing icons are skipped
5. Check Workbench - verify icons display correctly
6. Test parent chain - remove exact template, verify parent used

## Integration with iTidy

This test program demonstrates the core function needed for iTidy:

```c
/* Function to create from this test */
BOOL itidy_create_deficon(const char *filepath);

/* Would be called before tidying layout */
for (each file in directory) {
    if (!has_icon(file)) {
        itidy_create_deficon(file);
    }
}
```

## See Also

- [Test1 README](../Test1/README.md) - DefIcons scanner
- [docs/iTidy_DefIcons_Icon_Creation_Flow.md](../../../docs/iTidy_DefIcons_Icon_Creation_Flow.md) - Full specification
