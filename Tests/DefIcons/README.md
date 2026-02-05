# DefIcons Utilities

This directory contains utilities for analyzing the DefIcons preferences file format.

## Files

### Utilities
- **hexdump** - Simple hex dump utility (shows raw bytes)
- **deficonsdump** - DefIcons preferences parser (interprets the file structure)
- **deficontree** - Type hierarchy tree viewer (shows parent-child relationships)

### Source Files
- **hexdump.c** - Source for hex dump utility
- **deficonsdump.c** - Source for DefIcons parser
- **deficontree.c** - Source for hierarchy tree viewer
- **Makefile** - Build configuration for all utilities

### Data Files
- **deficons.prefs** - DefIcons preferences database file
- **dump.txt** - Example hex dump output
- **defdump.txt** - Example parsed output
- **DEFICONS_FORMAT.md** - Complete file format documentation
- **source/** - Original DefIcons format documentation
  - **deficons.h** - C header with action/type definitions
  - **deficons.i** - Assembly include with definitions
  - **deficonsprefs.a** - Assembly source showing file structure

## Building

To compile both utilities:
```
make
```

To rebuild from scratch:
```
make clean
make
```

## Usage

### hexdump
Simple hex dump of the deficons.prefs file:
```
hexdump
```
Output shows raw bytes in hex format with ASCII representation.

### deficonsdump
Parse and display the DefIcons database structure:
```
deficonsdump
```
Output shows:
- File type names in a hierarchical tree
- Matching rules for each type:
  - **MATCH** - Match specific bytes at an offset
  - **SEARCH** - Search for byte sequence
  - **NAMEPATTERN** - Match filename patterns (e.g., "#?.gif")
  - **FILESIZE** - Match exact file size
  - **PROTECTION** - Match file protection bits
  - **IS_ASCII** - Check if file is ASCII text
  - **MACRO_CLASS** - Type is a category with subtypes
  # deficontree
Display the type hierarchy tree or find the parent of a specific type:

**Display entire tree:**
```
deficontree
```

**Find parent of a specific type:**
```
deficontree mod
```

Output shows:
- Complete tree structure (if no argument)
- Path from root to the type (e.g., ROOT -> project -> music -> mod)
- Parent type name
- Child types (if any)

**Example:**
```
deficontree mod
```
Output:
```
Path: ROOT -> project -> music -> mod
Parent: music
```

This is useful for:
- Finding which category a file type belongs to
- Understanding the icon fallback hierarchy
- Determining the parent icon to use if a specific icon is missing

##- **OR** - Alternative matching rule

## DefIcons File Format

The deficons.prefs file contains a hierarchical database of file type definitions.

### Structure
```
[type name string] (null-terminated)
  [action byte] [action parameters...]
  [action byte] [action parameters...]
  ACT_END (0x00)
[hierarchy byte] (TYPE_DOWN_LEVEL, TYPE_UP_LEVEL, or TYPE_END)
```

### Action Codes
- **ACT_MATCH (1)** - Match bytes at offset: offset(2 bytes), length(1 byte), data
- **ACT_SEARCH (2)** - Search for bytes: length(2 bytes signed), data
- **ACT_SEARCHSKIPSPACES (3)** - Search skipping spaces: length(2 bytes signed), data
- **ACT_FILESIZE (4)** - Exact file size: size(4 bytes)
- **ACT_NAMEPATTERN (5)** - Filename pattern: pattern string (null-terminated)
- **ACT_PROTECTION (6)** - Protection bits: mask(4 bytes), bits(4 bytes)
- **ACT_OR (7)** - Alternative rule follows
- **ACT_ISASCII (8)** - Check if ASCII
- **ACT_MACROCLASS (20)** - Category (has children)
- **ACT_END (0)** - End of type definition

### Hierarchy Codes
- **TYPE_DOWN_LEVEL (1)** - Next type is child of current
- **TYPE_UP_LEVEL (2)** - Next type is sibling of parent
- **TYPE_END (0)** - End of database

### Example Entry
```
[picture]
  MACRO_CLASS (category only, has children)
TYPE_DOWN_LEVEL
  [gif]
    MATCH at offset 0, length 4: "GIF8"
  TYPE_UP_LEVEL
  [jpeg]
    MATCH at offset 0, length 2: FF D8
  TYPE_UP_LEVEL
```

This creates a "picture" category with "gif" and "jpeg" as child types. If a file matches the GIF signature (bytes "GIF8" at offset 0), it's identified as a gif. If it matches FF D8 at offset 0, it's a jpeg.

## Notes

- The DefIcons system is part of Workbench 3.2 and NewIcons
- Used to automatically assign appropriate icons to files based on content
- Supports both filename pattern matching and content-based detection
- Hierarchical structure allows fallback to parent icon if specific type icon is missing
