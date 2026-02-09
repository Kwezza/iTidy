# DefIcons Settings Window - Default Tool Management

**Version:** 2.0  
**Date:** February 9, 2026  
**File:** `src/GUI/deficons_settings_window.c`  
**Target:** AmigaOS / Workbench 3.2+

---

## Overview

The DefIcons Settings Window provides comprehensive management of default tools for DefIcons template icons. This includes viewing, editing, and hierarchical inheritance visualization for all DefIcon types across three generation levels.

---

## Feature Set

### 1. Show Default Tools

**Button:** "Show default tools"

**Purpose:** Display the default tool associated with each DefIcon type in the hierarchical tree view.

**Behavior:**
- Scans all DefIcon template icons in `ENVARC:Sys/`
- Displays default tools in square brackets: `[SYS:Utilities/MultiView]`
- Shows busy pointer during scanning operation (important for 68000 systems)
- Automatically initializes the DefIcons template system if not already loaded
- Rebuilds the entire listbrowser tree with tool information

**Display Format:**
- **Generation 1 (Parent types):** `project  [MultiView]`
- **Generation 2 (Categories):** `  sound  [SYS:Utilities/MultiView]`
- **Generation 3 (File types):** `  mod  [Workbench:Programs/Eagleplayer2/Eagleplayer]`

**Inheritance Visualization:**
- Types **with** displayed tools have their own `def_<type>.info` file with a specific tool set
- Types **without** displayed tools inherit from their parent type
- Example: If "midi" shows no tool but "music" shows `[MultiView]`, then "midi" inherits from "music"

---

### 2. Change Default Tool

**Button:** "Change default tool"

**Purpose:** Modify the default tool for any DefIcon type (generations 1, 2, or 3).

**Behavior:**
- Opens ASL file requester pre-populated with the current default tool path (if one exists)
- Falls back to `SYS:` if no tool is currently set
- Supports all three generation levels:
  - **Generation 1:** Parent types (e.g., "project", "music")
  - **Generation 2:** Categories (e.g., "sound", "picture")
  - **Generation 3:** File types (e.g., "mod", "wav", "gif")

**Automatic Icon Cloning (Generation 3):**

When a generation 3 file type is selected and it doesn't have its own template icon:

1. **Check:** Does `ENVARC:Sys/def_<type>.info` exist?
2. **If NO:**
   - Locate parent type (e.g., "mod" → "music")
   - Find parent template icon (`def_music.info`)
   - Clone parent icon to child (`def_music.info` → `def_mod.info`)
   - Update the new child icon with the selected tool
3. **If YES:**
   - Use existing child icon
   - Update with the selected tool

**Post-Update:**
- Automatically refreshes the listbrowser if tools are currently displayed
- Shows success confirmation dialog
- Preserves parent icon (never modifies it when cloning for children)

---

## Use Cases

### Use Case 1: View All Default Tools

**Scenario:** User wants to see which DefIcon types have custom default tools configured.

**Steps:**
1. Open DefIcons Settings window
2. Click "Show default tools" button
3. Review the tree:
   - Types with `[tool path]` have specific tools
   - Types without brackets inherit from parent

**Result:** Clear visualization of tool inheritance hierarchy.

---

### Use Case 2: Change Parent Tool

**Scenario:** User wants to change the default viewer for all "picture" types.

**Steps:**
1. Click "Show default tools" to see current tool
2. Select "picture" (generation 2 node)
3. Click "Change default tool"
4. Select new viewer (e.g., `SYS:Utilities/ImageViewer`)
5. Click OK

**Result:** `def_picture.info` is updated. All child types (gif, jpeg, png, etc.) without their own `def_*.info` files will inherit this new tool.

---

### Use Case 3: Override Child Tool

**Scenario:** User wants MOD music files to use a different player than other music files.

**Steps:**
1. Expand "music" node to see children
2. Select "mod" (generation 3 node)
3. Click "Change default tool"
4. Select specialized player (e.g., `Workbench:Programs/Eagleplayer2/Eagleplayer`)
5. Click OK

**Result:**
- System clones `def_music.info` → `def_mod.info` (if it doesn't exist)
- Updates `def_mod.info` with the new tool
- Only MOD files get the specialized player
- Other music types still use the parent music tool

---

## Technical Details

### Template Icon Resolution

**Direct Check Only:**
- The "Show default tools" feature only displays tools from **direct** template files
- Does NOT use `deficons_resolve_template()` fallback logic
- Checks: `ENVARC:Sys/def_<type>.info` exists using `Lock()`
- If file doesn't exist: Returns NULL (no tool displayed)

**Why This Matters:**
- Shows **actual** configured tools, not inherited ones
- Makes inheritance relationships clear in the UI
- User can see which types need custom tool configuration

### File Operations

**GetDiskObject() / PutDiskObject():**
- Both functions expect paths **WITHOUT** the `.info` extension
- The `.info` extension is automatically appended by icon.library
- Template system returns paths **WITH** `.info` extension
- Code strips `.info` before calling icon.library functions

**Example:**
- Template system: `ENVARC:Sys/def_music.info`
- Strip to: `ENVARC:Sys/def_music`
- Call: `GetDiskObject("ENVARC:Sys/def_music")`
- Loads: `ENVARC:Sys/def_music.info` ✓

### Performance Optimization

**Busy Pointer:**
- Displayed during "Show default tools" operation
- Critical for 68000 systems where scanning can take 2-3 seconds
- Provides visual feedback that operation is in progress

**Template System Caching:**
- DefIcons template system caches template icon locations
- First scan is slower, subsequent operations are faster
- Cache is automatically initialized if needed

---

## User Interface Behavior

### Selection Validation

**"Change default tool" button requirements:**
- Must have a node selected
- Node must be generation 1, 2, or 3 (not generation 4+)
- Shows error dialog if invalid selection

**Error Messages:**
- "No Selection" - No node selected
- "Invalid Selection" - Generation 4+ or invalid node

### ASL File Requester

**Pre-population:**
- If current tool exists: Splits into drawer + filename
- Handles both `:` and `/` path separators
- Example: `SYS:Utilities/MultiView` → drawer=`SYS:Utilities/`, file=`MultiView`

**Default Location:**
- If no current tool: Opens at `SYS:`

### Listbrowser Refresh

**Automatic Refresh:**
- Only occurs if "Show default tools" is currently active
- Uses `is_showing_tools` flag to track state
- Rebuilds entire tree with updated tool information
- User sees changes immediately without manual refresh

---

## Logging and Debugging

**Debug Markers:**
- Generation 3 operations are marked with `***` in logs
- Tracks: child template checks, parent resolution, icon cloning, tool updates
- Full paths logged for all file operations

**Key Log Messages:**
- `*** Generation 3 node 'mod' - checking if child template exists`
- `*** Child template DOES NOT EXIST`
- `*** Need to clone from parent...`
- `*** Parent type: 'music'`
- `*** Successfully loaded parent icon`
- `*** Calling PutDiskObject(...)`
- `*** Successfully cloned parent icon to child`

**Log Files:**
- Category: `LOG_GUI`
- Location: `Bin/Amiga/logs/gui_YYYY-MM-DD_HH-MM-SS.log`

---

## Known Limitations

1. **Generation 4+ Not Supported:**
   - Only generations 1-3 can have default tools changed
   - This covers 99% of DefIcon types in the hierarchy

2. **Template System Required:**
   - Must have DefIcons template icons in `ENVARC:Sys/`
   - Standard AmigaOS 3.2+ installation includes these

3. **Parent Must Exist:**
   - Generation 3 cloning requires parent template to exist
   - Shows error if parent template not found

---

## Related Files

**Core Implementation:**
- `src/GUI/deficons_settings_window.c` - Main window and feature logic
- `src/GUI/deficons_settings_window.h` - Window structure definitions

**Supporting Systems:**
- `src/deficons_parser.c` - DefIcons hierarchy parsing
- `src/deficons_templates.c` - Template icon resolution and caching
- `src/icon_types.c` - Icon loading and default tool manipulation

**Functions:**
- `lookup_deficon_default_tool()` - Direct template check (no parent fallback)
- `handle_show_tools()` - Display tools in listbrowser
- `handle_change_default_tool()` - Modify tools, automatic cloning
- `build_tree_list()` - Build listbrowser with optional tool display
- `GetIconDetailsFromDisk()` - Extract icon metadata including default tool
- `SetIconDefaultTool()` - Update icon's default tool and save

---

## Future Enhancements

**Potential Features:**
- Bulk tool assignment for multiple types
- Reset to parent (remove child template to restore inheritance)
- Visual indicator of inheritance (e.g., italics for inherited tools)
- Default tool validation (check if executable exists)
- Undo/redo for tool changes

---

## Version History

**v2.0 - February 9, 2026**
- Initial implementation of "Show default tools" feature
- Initial implementation of "Change default tool" feature
- Automatic icon cloning for generation 3 types
- Direct template checking (no parent fallback in display)
- Enhanced debug logging with generation markers
- Busy pointer for slow operations
- Square bracket formatting for tool display
