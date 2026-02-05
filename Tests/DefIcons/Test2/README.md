# DefIcons Creator - Test Program

## Purpose

A CLI test program that creates `.info` files for iconless files by querying DefIcons and copying appropriate icon templates. This demonstrates the core functionality needed for iTidy's DefIcons integration.

## Features

- **Automatic icon creation** - Creates .info files for files without icons
- **DefIcons port integration** - Queries DefIcons for accurate file type identification
- **Parent chain resolution** - Uses parent type templates when exact matches unavailable
- **Skip existing icons** - Never overwrites existing .info files
- **Batch processing** - Processes entire directories in one pass
- **Statistics reporting** - Shows created/skipped/failed counts

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

## How It Works

### For each file in the directory:

1. **Check for existing icon** - Skip if `filename.info` already exists
2. **Query DefIcons** - Send ARexx "Identify" command to get file type token
3. **Find template** - Search for `def_<type>.info` in ENV:/ENVARC:
4. **Walk parent chain** - If exact template not found, try parent types
5. **Copy template** - Copy template file to `filename.info`
6. **Report result** - Display success/failure message

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
- **Directory-only** - Does not recurse into subdirectories
- **Non-destructive** - Only creates new files, never modifies existing ones

## Technical Implementation

### Icon File Copying

Uses buffered file I/O with 8KB buffer:
```c
BOOL copy_icon_file(const char *source, const char *dest)
{
    // Open source template
    // Open destination .info
    // Copy in 8KB chunks
    // Return success/failure
}
```

### Icon Creation Logic

```c
BOOL create_icon_for_file(const char *filepath)
{
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
