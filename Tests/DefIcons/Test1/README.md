# DefIcons Scanner - Test Program

## Purpose

A CLI test program that demonstrates DefIcons port integration by scanning a folder and displaying file type classifications with icon template matching information.

## Features

- **Full deficons.prefs parsing** - Builds complete file type hierarchy tree from binary preference file
- **DefIcons port communication** - Queries DefIcons via ARexx messages for file type identification
- **Parent chain walking** - Intelligently walks up type hierarchy when exact icon templates are missing
- **Icon template lookup** - Searches ENV:Sys/ and ENVARC:Sys/ for def_*.info templates
- **Graceful fallback** - Handles DefIcons not running with clear error messages

## Usage

```shell
deficons_scanner <folder_path>
```

### Examples

```shell
deficons_scanner Work:Documents
deficons_scanner SYS:Utilities
deficons_scanner RAM:TestFiles
```

## Output Format

The program displays a table with the following columns:

| Column | Description |
|--------|-------------|
| **Filename** | Name of the file (excluding .info files) |
| **DefIcons Type** | File type token returned by DefIcons (e.g., "gif", "mod", "lha") |
| **Match Type** | "Exact" if def_<type>.info exists, "Parent" if using inherited template |
| **Template** | Filename of the icon template being used (e.g., "def_picture.info") |

### Example Output

```
Scanning: Work:Pictures
Filename                                 DefIcons Type        Match Type      Template
================================================================================
Photo001.gif                             gif                  Exact           def_gif.info
Photo002.jpeg                            jpeg                 Parent          def_picture.info
Landscape.iff                            iff                  Exact           def_iff.info
Music.mod                                mod                  Parent          def_music.info
Document.txt                             ascii                Parent          def_project.info
================================================================================
Total files scanned: 5
```

## Prerequisites

1. **DefIcons must be running** - The program requires the DefIcons port to be active
2. **deficons.prefs must exist** - Located in ENV:Sys/deficons.prefs or ENVARC:Sys/deficons.prefs
3. **Icon templates** - Templates should exist in ENV:Sys/ or ENVARC:Sys/ (e.g., def_picture.info)

## How It Works

### 1. Startup Phase
- Opens required libraries: icon.library, rexxsyslib.library
- Checks for DefIcons port existence
- Parses deficons.prefs binary file to build type hierarchy tree

### 2. Scanning Phase
For each file in the specified directory:
- Builds full path
- Sends ARexx "Identify" command to DefIcons port
- Receives file type token (e.g., "gif", "mod", "c")

### 3. Template Lookup Phase
For each identified file:
- Searches for ENV:Sys/def_<token>.info
- If not found, walks up parent chain (e.g., "gif" → "picture" → "project")
- Marks as "Exact" match or "Parent" match
- Displays template filename or "None found"

### 4. DefIcons Preference Parsing

The program reads the binary deficons.prefs file structure:
- **Header** (32 bytes) - Skipped
- **Type entries** - Repeating pattern:
  - Type name (null-terminated string)
  - Action codes (ACT_MATCH, ACT_SEARCH, ACT_NAMEPATTERN, etc.)
  - Hierarchy byte (TYPE_DOWN_LEVEL, TYPE_UP_LEVEL, TYPE_END)

The parser builds an in-memory tree with parent relationships, allowing efficient parent chain traversal.

## Building

```shell
cd Tests/DefIcons/Test1
make clean
make
```

Output: `Bin/Amiga/Tests/DefIcons/deficons_scanner`

## Technical Details

- **Language**: C99 (VBCC compatible)
- **Target**: AmigaOS 3.2 / 68020+
- **Libraries**: dos.library, icon.library, rexxsyslib.library, exec.library
- **Maximum type nodes**: 500
- **Read buffer size**: 4KB

## Limitations

- **Single folder only** - Does not recurse into subdirectories
- **Skips .info files** - Only processes actual data files
- **No error recovery** - If DefIcons port not found, file types cannot be identified
- **No caching** - Each file requires separate ARexx message (performance limitation)

## Error Messages

| Message | Meaning | Solution |
|---------|---------|----------|
| "DefIcons port not found" | DefIcons not running | Start DefIcons commodity |
| "Could not parse deficons.prefs" | Preference file missing/corrupt | Check ENV:Sys/deficons.prefs |
| "Cannot access directory" | Invalid path or permissions | Verify folder path exists |

## Future Enhancements

For production integration into iTidy:

1. **Caching** - Cache token→template and extension→token mappings
2. **Batch queries** - Send multiple Identify requests without waiting
3. **Recursive scanning** - Add optional -r flag for subdirectories
4. **Icon creation** - Actually copy templates to create .info files
5. **Performance** - Reuse ARexx message ports instead of recreating
6. **Filtering** - Skip directories, system files, already-iconified files

## References

- [docs/iTidy_DefIcons_Icon_Creation_Flow.md](../../../docs/iTidy_DefIcons_Icon_Creation_Flow.md) - Complete DefIcons integration specification
- [Tests/DefIcons/Test_DeficonsReader.c](../Test_DeficonsReader.c) - Parser reference implementation
