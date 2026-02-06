# iTidy: DefIcons-based icon creation flow (Workbench 3.2)

This note captures the recommended implementation flow for creating real `.info` icons using Workbench 3.2 DefIcons classification, without opening Workbench windows or causing visual flashing.

## High-level goals

- Optional pre-pass that **writes real `.info` files** for iconless entries so later tidy and snapshot behaviour persists.
- Use DefIcons classification so improvements in Workbench 3.2 DefIcons rules automatically benefit iTidy over time.
- Keep the run fast on large folder trees by caching and minimizing ARexx overhead.

## What is already assumed

- You can parse `deficons.prefs` and extract **child -> parent** type relationships into memory.
- You can call DefIcons `Identify` and get a leaf token such as `mod` or `ascii`.
- You have a directory of default icon templates like `def_music.info`, `def_ascii.info`, etc.

## Core idea

Given an identified token (example `mod`):

1. Try to find a template icon for it (example `def_mod.info`).
2. If missing, look up its parent in the parsed prefs map (example `music`) and try `def_music.info`.
3. Repeat walking up the parent chain until you find an existing template.
4. If nothing is found, fall back to a safe generic template like `def_project.info` or `def_tool.info`.

## Proposed implementation flow

### On load (one-time setup)

1. **Load and parse** `deficons.prefs`:
   - Prefer `ENV:Sys/deficons.prefs`.
   - Fall back to `ENVARC:Sys/deficons.prefs`.
2. Extract and store only what you need:
   - `parent_of[token] = parent_token`
3. Scan template locations and build a set of available templates:
   - `ENV:Sys/def_<type>.info`
   - `ENVARC:Sys/def_<type>.info` (typically stored on disk under `Workbench:Prefs/Env-Archive/Sys/`)
   - Naming convention: `def_` + `<type>` + `.info` (for example `def_music.info`, `def_ascii.info`)
   - Optional: an iTidy override directory for user custom templates (future enhancement).
4. Initialize caches:
   - `resolved_template_for_token[token] = resolved_type_or_path`
   - Optional: `extension_cache[.ext] = resolved_type_or_path` for speed on large trees.

### During an iTidy run (optional phase)

If the user enables an option like **"Create icons first for known types"**:

For each folder:

1. Enumerate directory entries (files and drawers).
2. For each entry:
   - If `entry.info` already exists: **skip** (never overwrite user icons).
   - If entry is a drawer and drawer icon creation is enabled: handle using the folder mode logic (see preferences below).
   - Otherwise classify:
     - Call DefIcons `Identify` to get a token.
     - Use caches to avoid repeated Identify calls for common types.
3. Resolve token to a template:
   - Try direct match: `def_<token>.info`.
   - If missing, walk parent chain from `deficons.prefs`:
     - `token = parent_of[token]` until a template exists or root is reached.
   - Cache the resolved result per token.
4. Apply user filtering preferences:
   - Example preference: "Do not create icons for tools".
   - Prefer applying filtering against the **final resolved category** rather than the raw leaf token.
5. Create the real icon:
   - Copy the chosen template `.info` to `entry.info`.
   - After this, your existing tidy and snapshot logic should persist reliably.

## Performance recommendations

### Avoid Identify calls where cheap heuristics suffice

- If the entry is a drawer, do not call Identify.
- If `entry.info` exists, do not call Identify.
- Consider other cheap checks to reduce Identify calls.

### Cache at two levels

1. **Resolved template cache**
   - Cache final resolution: `mod -> music -> def_music.info` so resolution is done once per token.
2. Optional **token cache**
   - Cache identification results by extension (or another cheap key) to reduce Identify calls further.

## Calling DefIcons from C (Option B: direct ARexx message)

For performance, avoid launching `rx` per file. Instead, send ARexx messages directly to the DefIcons public port from iTidy.

### Why this helps

- No new processes per file.
- Lower overhead: just an Exec message send/receive.
- Best fit for “no flashing UI” runs with a smooth progress bar.

### How it works (conceptual)

1. DefIcons is running and has a public port named `DEFICONS`.
2. iTidy finds that port (`FindPort("DEFICONS")`).
3. iTidy creates a reply port, builds a `RexxMsg` containing a command like:
   - `Identify "Work:Music/gods_intro.mod"`
4. iTidy posts the message to `DEFICONS`, waits for the reply, and reads the returned token.
5. iTidy frees the message and any allocated arg strings.

There is no “open/close” session. The port exists as long as DefIcons is running.

### Minimal C89-style outline

The implementation typically uses `rexxsyslib.library` helpers such as:

- `CreateRexxMsg()`, `DeleteRexxMsg()`
- `CreateArgstring()`, `DeleteArgstring()`

and standard Exec messaging:

- `FindPort()`, `PutMsg()`, `WaitPort()`, `GetMsg()`

Pseudo-outline:

- `defPort = FindPort("DEFICONS")`
- `replyPort = CreateMsgPort()`
- `rxm = CreateRexxMsg(replyPort, NULL, NULL)`
- `rxm->rm_Args[0] = CreateArgstring("Identify \"<path>\"", len)`
- set `RXFF_RESULT` to request a result string
- `PutMsg(defPort, rxm)`
- wait on `replyPort`, read result token from the reply
- free resources

### Integration points in iTidy

- Detect once at run start whether the `DEFICONS` port exists.
  - If missing, disable this feature gracefully.
- Only call Identify for entries that are iconless and not covered by cheap heuristics (drawers, already-has `.info`, etc.).
- Cache results:
  - `token -> resolved template` (after walking parent chain)
  - optional `extension -> resolved template` for extra speed

## Preferences to add

### Main switch

- **Create icons first for known types**
  - Writes `.info` files for iconless entries before tidying.

### Folder (drawer) icon creation mode (recommended)

Folder icon creation can be independently controlled, since some users prefer not to generate drawer `.info` files.

Suggested setting: **Create folder icons** (tri-state)

- **No**: Never create drawer icons (`def_drawer.info`), even if the drawer is iconless.
- **Always**: Create drawer icons for any iconless drawer encountered.
- **Smart**: Only create a drawer icon if at least one of the following is true:
  - the drawer already contains **any** real `.info` file (it already participates in icon layout), or
  - the drawer contains at least one file for which iTidy will create an icon in this run (based on your category filters and resolution), or
  - the drawer contains at least one sub-drawer that already has an icon (or will get one in this run), so creating the parent drawer icon improves consistency.

Implementation hint for **Smart**:
- When scanning a drawer, track a boolean like `drawer_needs_icon` that becomes true if any eligible file/subdrawer is found.
- Only write `drawer.info` at the end of the scan when `drawer_needs_icon` is true and `drawer.info` is missing.

### Category filters (recommended)

Instead of forcing users to understand every leaf token, offer a simple category checkbox set and map resolved tokens into these buckets:

- Documents (ascii, pdf, officedocs, amigaguide)
- Pictures (picture, iff)
- Music, sound, video (music, sound, video)
- Archives (archive, diskarchive)
- Source code and scripts (c, h, cpp, asm, src, rexx, script)
- Tools and executables (tool)
- Other projects (project)

Then decide creation based on the **resolved** template token.

### Safety preferences (recommended)

Writing `.info` files is invasive. Provide guardrails:

- Exclude system assigns or selected paths (for example SYS:, C:, S:).
- Only create icons under user selected roots.
- Never overwrite existing `.info`.

## Fallback rules

When resolution fails, fall back to:

- `def_tool.info` for executables (if you detect that reliably).
- Otherwise `def_project.info`.

Also include cycle protection:

- Track visited tokens during parent walking to prevent infinite loops on corrupted prefs data.

## Notes on user experience

- Make it explicit in the UI that this feature **writes `.info` files** next to iconless files.
- Keep it silent: no Workbench windows need to be opened, so the only UI should be your progress bar.

## Project reference files

For this project, working examples and format documentation already exist in the repo:

- `Tests\DefIcons\deficontree.c`  
  **Type hierarchy tree viewer** (shows parent-child relationships).  
  Working example of parsing `deficons.prefs` and a good starting point for building the in-memory type list (`token -> parent` map).

- `Tests\DefIcons\Test2\deficons_creator.c`  
  Tested working implementation of the DefIcons icon creation flow, and explicitly implements **Option B: direct ARexx message** from this document.  
  This contains proven, reusable functions that can be used as the foundation for integrating DefIcons-based icon creation into the main iTidy codebase.

- `Tests\DefIcons\DEFICONS_FORMAT.md`  
  Complete breakdown of how the `deficons.prefs` binary format is laid out (useful if you need to extend or re-check the parser).

- Other files in `Tests\DefIcons\`  
  Additional helpers and experiments that can be referenced as needed.

---

## Implementation Progress Report

### ✅ Completed Components (as of February 2026)

#### 1. DefIcons Preferences Parser (`src/deficons_parser.c`)
**Status: Complete and integrated**

Implementation notes:
- Parses `ENV:Sys/deficons.prefs` binary format successfully
- Falls back to `ENVARC:Sys/deficons.prefs` if ENV: version unavailable
- Builds hierarchical tree structure with parent-child relationships
- Caches parsed tree globally (`g_cached_deficons_tree` in `main_gui.c`)
- Called once at startup to populate cache for GUI and runtime use

**Amendment to original plan:**
- Tree structure uses `DeficonTypeTreeNode` with fields: `name[64]`, `parent_index`, `generation`
- Generation field is 1-based (root=1, children=2, grandchildren=3+) per ListBrowser API requirements
- Total of 124 type nodes loaded from standard Workbench 3.2 DefIcons configuration
- Tree includes 1 root ("Type"), 14 second-level categories (sound, picture, archive, etc.), and 108+ leaf types

#### 2. DefIcons Settings Window GUI (`src/GUI/deficons_settings_window.c`)
**Status: Complete and working**

Implementation notes:
- Full ReAction-based GUI using ListBrowser hierarchical tree display
- Displays all 124 DefIcons types in collapsible tree structure
- Checkboxes on second-level nodes (generation 2) allow enabling/disabling entire categories
- "Select All" / "Select None" buttons for bulk operations
- Folder icon creation mode chooser (tri-state: Smart/Always/Never)
- OK/Cancel buttons with working copy pattern (changes only saved on OK)
- State persists to `LayoutPreferences.deficons_disabled_types` (CSV string format)

**Amendment to original plan:**
- Category filters implemented as **checkboxes on parent categories** rather than separate UI panel
- Users can disable categories like "sound", "picture", "archive" directly in the tree
- Leaf types inherit enabled/disabled state from their parent category
- This provides better user experience than abstract category checkboxes

**Technical lessons learned:**
- ListBrowser generations MUST be 1-based (not 0-based) per autodoc
- Use `HideAllListBrowserChildren()` API for collapsing tree nodes
- Checkbox events handled via `LISTBROWSER_RelEvent` (LBRE_CHECKED/LBRE_UNCHECKED)
- UI state check must ignore `enable_deficons_icon_creation` master flag to show correct checkbox states

#### 3. Preferences Storage Integration
**Status: Complete**

Implementation notes:
- `LayoutPreferences.deficons_disabled_types[256]` stores comma-separated list of disabled categories
- Example: `"sound,ascii,font"` means these three types are disabled
- Empty string means all types enabled
- Helper functions in `src/layout_preferences.c`:
  - `add_disabled_deficon_type()` - Adds type to CSV list
  - `remove_disabled_deficon_type()` - Removes type from CSV list
  - `clear_disabled_deficon_types()` - Clears entire list
  - `is_deficon_type_enabled()` - Runtime check (respects master enable flag)
  - `is_deficon_type_checked_for_ui()` - UI check (ignores master flag, shows actual selection)

**Amendment to original plan:**
- Master enable/disable flag: `LayoutPreferences.enable_deficons_icon_creation`
- Folder icon mode: `LayoutPreferences.deficons_folder_icon_mode` (0=Smart, 1=Always, 2=Never)
- System path exclusion: `LayoutPreferences.deficons_skip_system_assigns` (TRUE = skip SYS:, C:, etc.)

### 🚧 Pending Components

#### 4. DefIcons ARexx Integration (Icon Creation Runtime)
**Status: Not yet implemented**

Remaining work:
- Implement direct ARexx messaging to `DEFICONS` port (Option B from plan)
- Create functions to:
  - `FindPort("DEFICONS")` at startup
  - Send `Identify "<path>"` commands via RexxMsg
  - Cache results by token and/or extension
  - Handle missing DefIcons gracefully (disable feature)
- Integration point: New module `src/deficons_identify.c` (suggested)

Reference implementation:
- `Tests\DefIcons\Test2\deficons_creator.c` contains working ARexx messaging code
- Can be adapted for iTidy's memory management system (whd_malloc/whd_free)

#### 5. Template Icon Resolution and Caching
**Status: Not yet implemented**

Remaining work:
- Scan template locations at startup:
  - `ENV:Sys/def_<type>.info`
  - `ENVARC:Sys/def_<type>.info`
- Build cache: `resolved_template_for_token[token] = path`
- Implement parent chain walking:
  - If `def_<token>.info` missing, try `def_<parent>.info`
  - Continue until match found or root reached
- Fallback logic:
  - Executables → `def_tool.info`
  - Others → `def_project.info`
- Integration point: New module `src/deficons_templates.c` (suggested)

#### 6. Icon Creation Main Loop
**Status: Not yet implemented**

Remaining work:
- Add optional pre-pass before icon layout processing
- For each folder entry:
  - Skip if `.info` already exists (never overwrite)
  - Skip if type is disabled in preferences
  - For drawers: apply folder mode logic (Smart/Always/Never)
  - For files: Call Identify, resolve template, copy `.info` file
- Performance optimizations:
  - Cache Identify results by extension
  - Batch process files in same folder
  - Use Fast RAM allocations (already available via whd_malloc)
- Integration point: Add to `src/layout_processor.c` as optional pre-pass

#### 7. Safety and Filtering Logic
**Status: Partially implemented**

Completed:
- `deficons_skip_system_assigns` preference flag exists
- Category filtering via `deficons_disabled_types`

Remaining work:
- Implement system path exclusion check (SYS:, C:, S:, DEVS:, LIBS:)
- Add user path inclusion/exclusion list (optional future enhancement)
- Cycle protection during parent chain walking

#### 8. User Experience and UI Integration
**Status: Partially implemented**

Completed:
- DefIcons Settings window accessible from main GUI
- Progress feedback ready (iTidy already has progress window system)

Remaining work:
- Add main GUI checkbox: "Create icons for iconless files" (enable feature)
- Show icon creation count in progress window
- Add statistics: "Created 47 icons (23 pictures, 12 music, 8 archives, 4 documents)"
- Optional: Preview mode showing which icons would be created

### 📋 Next Steps (Recommended Implementation Order)

1. **DefIcons ARexx Integration** (`src/deficons_identify.c`)
   - Port working code from `Test2/deficons_creator.c`
   - Add startup check for `DEFICONS` port availability
   - Implement token caching system

2. **Template Resolution** (`src/deficons_templates.c`)
   - Scan ENV:/ENVARC: for available templates at startup
   - Build resolution cache with parent chain walking
   - Add fallback logic

3. **Icon Creation Loop** (integrate into `src/layout_processor.c`)
   - Add pre-pass option before layout processing
   - Apply filtering logic (disabled types, system paths)
   - Implement folder mode logic (Smart/Always/Never)
   - Copy template `.info` files to target entries

4. **Main GUI Integration**
   - Add "Create icons first" checkbox to main window
   - Wire checkbox to `enable_deficons_icon_creation` preference
   - Update progress window to show icon creation statistics

5. **Testing and Refinement**
   - Test on large folder trees (performance validation)
   - Test with missing DefIcons port (graceful degradation)
   - Test with custom DefIcons configurations
   - Validate folder mode "Smart" logic

### 📝 Documentation Status

- ✅ DefIcons Settings GUI documented in code comments
- ✅ ReAction patterns documented in `docs/Reaction_tips.md`
- ✅ Memory management patterns documented
- 🚧 End-user documentation pending (iTidy.guide update needed)
- 🚧 Developer integration guide pending

### 🎯 Current State Summary

**What works now:**
- Users can open DefIcons Settings window
- Users can enable/disable entire file type categories (sound, picture, etc.)
- Settings persist between sessions
- Tree displays all 124 DefIcons types with correct hierarchy

**What's still needed:**
- Runtime icon creation (ARexx integration)
- Template scanning and resolution
- Main execution loop integration
- Performance optimization and caching

**Estimated completion:** Icon creation runtime is approximately 40% complete. GUI and preferences system (critical foundation) is 100% complete. Remaining work focuses on ARexx integration and template resolution logic, both of which have working reference implementations in `Tests/DefIcons/`.
