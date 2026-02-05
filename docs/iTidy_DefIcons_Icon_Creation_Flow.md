# iTidy: DefIcons-based icon creation flow (Workbench 3.2)

This note captures the recommended implementation flow for creating real `.info` icons using Workbench 3.2 DefIcons classification, without opening Workbench windows or causing visual flashing.

## High-level goals

- Optional pre-pass that **writes real `.info` files** for iconless entries so later tidy and snapshot behaviour persists.
- Use DefIcons classification so improvements in Workbench 3.2 DefIcons rules automatically benefit iTidy over time.
- Keep the run fast on large folder trees by caching and minimizing ARexx calls.

## What is already assumed

- You can parse `deficons.prefs` and extract **child -> parent** type relationships into memory.
- You can call DefIcons `Identify` via ARexx and get a leaf token such as `mod` or `ascii`.
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
   - `ENV:Sys/def_<token>.info`
   - `ENVARC:Sys/def_<token>.info`
   - Optional: an iTidy override directory for user custom templates (recommended as a future enhancement).
4. Initialize caches:
   - `resolved_template_for_token[token] = resolved_token_or_path`
   - Optional: `extension_cache[.ext] = resolved_token_or_path` for speed on big trees.

### During an iTidy run (optional phase)

If the user enables an option like **"Create icons first for known types"**:

For each folder:

1. Enumerate directory entries (files and drawers).
2. For each entry:
   - If `entry.info` already exists: **skip** (never overwrite user icons).
   - If entry is a drawer and you choose to support drawer icon creation:
     - Use a drawer template (often `def_drawer.info`) and continue.
   - Otherwise classify:
     - Call DefIcons `Identify` via ARexx to get a token.
     - Use caches to avoid repeated Identify calls for common types.
3. Resolve token to a template:
   - Try direct match: `def_<token>.info`.
   - If missing, walk parent chain from `deficons.prefs`:
     - `token = parent_of[token]` until a template exists or root is reached.
   - Cache the resolved result per token.
4. Apply user filtering preferences:
   - Example preference: "Do not create icons for tools".
   - Apply filtering against the **final resolved category** rather than the raw leaf token where possible.
5. Create the real icon:
   - Copy the chosen template `.info` to `entry.info`.
   - After this, your existing tidy and snapshot logic should persist reliably.

## Performance recommendations

### Avoid ARexx calls where cheap heuristics suffice

- If the entry is a drawer, do not call Identify.
- If `entry.info` exists, do not call Identify.
- Consider other cheap checks to reduce Identify calls.

### Cache at two levels

1. **Token cache**
   - If you want, cache identification results by extension or other inexpensive key.
2. **Resolved template cache**
   - Cache final resolution: `mod -> music -> def_music.info` so resolution is done once per token.

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

Folder icon creation can be **independently** controlled, since some users prefer not to generate drawer `.info` files.

Suggested setting: **Create folder icons** (tri-state)

- **No**: Never create drawer icons (`def_drawer.info`), even if the drawer is iconless.
- **Always**: Create drawer icons for any iconless drawer encountered.
- **Smart**: Only create a drawer icon if at least one of the following is true:
  - the drawer already contains **any** real `.info` file (it already “participates” in icon layout), or
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
- Check free space first?

## Fallback rules

When resolution fails, fall back to:

- `def_tool.info` for executables (if you detect that reliably).
- Otherwise `def_project.info`.

Also include cycle protection:

- Track visited tokens during parent walking to prevent infinite loops on corrupted prefs data.

## Notes on user experience

- Make it explicit in the UI that this feature **writes `.info` files** next to iconless files.
- Keep it silent: no Workbench windows need to be opened, so the only UI should be your progress bar.
