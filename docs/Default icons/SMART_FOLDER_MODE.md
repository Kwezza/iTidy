# Smart Folder Icon Mode

This document describes how iTidy's DefIcons "Smart" folder (drawer) icon creation mode works,
including the post-order processing architecture and the WHDLoad skip feature.

## Overview of folder icon modes

`prefs->deficons_folder_icon_mode` controls when drawer `.info` files are created:

| Value | Constant | Behaviour |
|-------|----------|-----------|
| 0 | `DEFICONS_FOLDER_MODE_SMART` | Create drawer icon only when the folder has visible contents (default) |
| 1 | `DEFICONS_FOLDER_MODE_ALWAYS` | Always create a drawer icon, regardless of contents |
| 2 | `DEFICONS_FOLDER_MODE_NEVER` | Never create drawer icons |

The decision for Smart mode is performed by:

`deficons_should_create_folder_icon(path, has_visible_contents, prefs)`

When mode is Smart this function returns `has_visible_contents` directly.

## Definition of has_visible_contents

`has_visible_contents` is TRUE when any of the following is true:

1. The folder already contains one or more `.info` files (it already participates in layout).
2. The folder contains at least one regular file that will get an icon created (it passes DefIcons
   identification and the user type-filter rules).
3. The folder contains at least one subdrawer that already has, or receives, a drawer `.info`.

## Post-order processing (current architecture)

Icon creation uses **post-order traversal**: a directory's drawer icon is decided on the way
**back up** from recursion, using the real `child_result.has_visible_contents` that the child
recursion returned.

### Call flow

```
deficons_create_missing_icons_recursive(A)
    deficons_create_missing_icons_in_directory(A, files_only=TRUE)
        -- processes files in A only, skips all subdirs
    for each subdir B in A:
        deficons_create_missing_icons_recursive(B)   -- recurse first
            ... (same pattern inside B)
        -- back from recursion: child_result.has_visible_contents is now the
        --   real answer, no pre-scan needed
        if deficons_should_create_folder_icon(B, child_result.has_visible_contents, prefs):
            copy drawer template -> B.info           -- post-order decision
```

### Why post-order instead of a pre-scan

The earlier implementation ran `deficons_folder_has_visible_contents()` **before** recursing into
each subdirectory, which caused two problems:

1. **Double-scan**: the pre-scan opened and read the directory once, then the recursive pass opened
   and read it a second time — each file was ARexx-identified twice.
2. **Folder-only gap**: the pre-scan skipped subdirectories (it only looked at files), so a folder
   whose only interesting contents were subdrawers with icons was incorrectly reported as empty,
   and no drawer `.info` was created for it.

Post-order processing eliminates both problems: the recursion is the scan, and the result
propagates upward naturally.

### files_only parameter

`deficons_create_missing_icons_in_directory()` accepts a `BOOL files_only` parameter:

- `files_only = FALSE` (non-recursive, single-folder mode): keeps the original pre-scan behaviour
  for drawers, because there is no recursive result available.
- `files_only = TRUE` (called from `_recursive()`): silently skips all subdirectory entries;
  drawer icon decisions are handled by the parent recursive function after child results are known.

### skipHiddenFolders behaviour with post-order

When `skipHiddenFolders` is enabled and a folder has no existing `.info`:

- **Never mode**: the folder is skipped entirely (no recursion, no icon) — a drawer icon will
  never be created regardless.
- **Smart / Always mode**: the folder is recursed unconditionally. In Smart mode, if the recursion
  finds no visible contents, `child_result.has_visible_contents` is FALSE, so `deficons_should_create_folder_icon()`
  returns FALSE and no drawer `.info` is created — the folder remains hidden. In Always mode a
  drawer `.info` is always created. Either way the pre-scan double-read is avoided.

---

## WHDLoad folder skip feature

`prefs->deficons_skip_whdload_folders` (default FALSE) enables automatic detection and skipping
of WHDLoad game/application folders.

### Detection

A directory is considered a WHDLoad folder when it contains at least one file matching the pattern
`#?.slave` (case-insensitive). Detection is performed by `deficons_is_whdload_folder()` in
`deficons_filters.c`, which uses `MatchFirst()` / `MatchNext()` with an early exit on the first
hit — negligible cost on non-WHDLoad folders.

### Effect

When a WHDLoad folder is detected:

| Location | Behaviour |
|----------|-----------|
| Root directory being processed (`_in_directory`) | Returns immediately with zero icons created; result is otherwise clean. The folder itself already has a drawer `.info` from wherever it was installed, so no icon is needed. |
| Subdirectory encountered during recursion (`_recursive` subdir loop) | The subdir is skipped: no recursion, no drawer `.info` created for any sub-subfolder. |
| Checked inside `deficons_folder_has_visible_contents` (non-recursive pre-scan) | Returns TRUE immediately — the WHDLoad folder is treated as "visible" so its parent still gets a drawer icon. |

### Rationale

WHDLoad game folders contain many supporting files (`*.slave`, `*.info`, `data/`, `save/` etc.)
that are not user data files and should not receive DefIcons icons. Skipping them:

- Avoids cluttering game directories with auto-generated icons.
- Avoids thousands of pointless ARexx calls for slave/data files deficons cannot usefully type.
- Keeps the WHDLoad folder visible in Workbench (the existing drawer `.info` is preserved).

---

## Ordering and safety

- Icon creation runs before the main tidying/layout pass so newly-created drawer icons are
  included in the subsequent layout.
- Existing `.info` files are never overwritten: the code checks for `.info` presence via `Lock()`
  before creating any icon.
- The logic respects system/exclusion preferences (`deficons_skip_system_assigns`, user exclude
  list) and the user's type filters.
- Cancellation checks (`CREATION_CHECK_CANCEL()`) are performed at the top of every subdir loop
  iteration, and recursion is capped at depth 100.

## Practical outcomes

| Scenario | Result |
|----------|--------|
| Empty folder, no `.info`, no files | No drawer icon (Smart), always icon (Always), no icon (Never) |
| Folder with only files, all skipped by type filter | No drawer icon (Smart) |
| Folder with icon-able files | Drawer icon created (Smart and Always) |
| Folder containing only subdirs that get icons | Drawer icon created (Smart) — fixed by post-order |
| WHDLoad folder (`*.slave` present) | Skip feature enabled: no inner icons; folder itself stays visible |
| Folder with existing `.info` | Existing icon kept; folder counted as visible |

---

Document updated 2026-02-20: post-order processing implemented; WHDLoad skip feature added.
