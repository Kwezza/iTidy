# Smart Folder Mode — Deep Dive and Improvement Brainstorm

**Date:** 2026-02-20  
**Source files examined:** `src/deficons/deficons_creation.c`, `src/deficons/deficons_creation.h`,
`src/deficons/deficons_filters.c`, `src/layout_processor.c`, `src/folder_scanner.c`

---

## 1. What Does "Smart" Mode Mean?

When `deficons_folder_icon_mode == 0` (Smart, the default), iTidy will only create a drawer `.info`
file for a subdirectory if that subdirectory "has visible contents" — meaning it either already
contains `.info` files, or contains files that DefIcons would assign icons to.

The other modes for reference:
- `1` = Always — create a drawer icon for every subdirectory unconditionally
- `2` = Never — never create drawer icons

---

## 2. The Full Execution Flow (with "Recursive" enabled)

### Entry point: `layout_processor.c`

```
ProcessWithPreferences(rootPath, prefs)
  -> DefIcons icon creation phase (if enabled):
       if recursive:
         deficons_create_missing_icons_recursive(rootPath, level=0, ...)
       else:
         deficons_create_missing_icons_in_directory(rootPath, ...)
  -> Main tidy/layout pass (ProcessDirectoryRecursive or ProcessSingleDirectory)
```

The icon creation phase runs **before** the main tidy/layout pass so any freshly created icons
are included in the subsequent layout.

---

### `deficons_create_missing_icons_recursive(path, level, ...)`

This function orchestrates recursive icon creation:

```
Step 1. Call deficons_create_missing_icons_in_directory(path, ...)
        -> Processes all entries (files AND subdirectories) in `path`
        -> For files: identify via DefIcons ARexx, create .info if needed
        -> For subdirs (no .info yet):
             - Smart mode: call deficons_folder_has_visible_contents(subdir)
               -> if TRUE:  create drawer .info for that subdir
               -> if FALSE: leave the subdir without a drawer .info

Step 2. Lock `path` again and enumerate subdirs
        -> If skipHiddenFolders is ON and a subdir has no .info:
             - Smart mode: call deficons_folder_has_visible_contents(subdir) AGAIN
               -> if FALSE: skip recursing into this subdir
               -> if TRUE:  recurse (it may be about to gain icons)
           Otherwise: recurse into every subdir

Step 3. For each subdir, call deficons_create_missing_icons_recursive(subdir, level+1, ...)
        -> This repeats the whole process one level deeper
```

---

### `deficons_folder_has_visible_contents(path, ...)`

This is the "Smart" check function. It does a **shallow** scan (no recursion into subdirs):

```
Pass 1 — Cheap: scan for existing .info files (early exit on first hit)
  -> Uses ExNext, looks at each entry's filename
  -> If any .info found -> return TRUE immediately

Pass 2 — Expensive (only if Pass 1 found nothing):
  -> Re-examine and re-iterate directory entries
  -> For each regular FILE (directories are SKIPPED in this pass):
       - Check if that file's .info already exists (Lock check)
       - If yes -> return TRUE immediately (visible content found)
       - Call deficons_identify_file(fullpath, ...) -> ARexx call to DefIcons
       - Call deficons_should_create_icon(type_token, prefs) -> type filter check
       - If both pass -> return TRUE immediately (this file would get an icon)
  -> If no file triggered TRUE -> return FALSE
```

**Critical detail:** Pass 2 only checks FILES in the direct contents of the directory.
Subdirectories encountered during the ExNext loop are explicitly SKIPPED with `if (fib_DirEntryType > 0) continue;`. This function does **not** recurse.

---

## 3. Answering the Core Question: Does It Dig Into Subfolders?

**Short answer: No, `deficons_folder_has_visible_contents()` does NOT dig into subfolders.**

When iTidy processes directory `A` and encounters subdirectory `B` (without a `.info`), it calls
`deficons_folder_has_visible_contents(B)`. That function scans only the direct contents of `B`:
- Existing `.info` files in B
- Files in B that would receive icons

Subdirectories found inside `B` are ignored. This means:

| B's contents | has_visible_contents result | Drawer .info created for B? |
|---|---|---|
| Files that would get icons | TRUE | Yes |
| Files that already have .info | TRUE | Yes |
| Only subdirectories (no files) | FALSE | No |
| Empty | FALSE | No |

So if B only contains sub-subdirectory C (with no files in B itself), then `has_visible_contents(B)`
returns FALSE, and A will NOT create a drawer icon for B — even if C ultimately gets many icons
created inside it by the recursive pass.

### Why the run seemed to "just stick there"

The user reported the run appeared to create more and more icons when it claimed to still be
processing the original folder. The likely cause is the **double-scan** issue described next.

---

## 4. The Double-Scan Problem

This is the primary architectural issue with the current Smart mode implementation.

### What happens

For **every** subdirectory in a recursive run, each file gets identified by DefIcons ARexx
**at least twice**:

**First scan** — `deficons_folder_has_visible_contents(B)` called from A's processing:
- Iterates files in B, calling `deficons_identify_file()` for each unidentified file

**Second scan** — `deficons_create_missing_icons_in_directory(B)` called during recursion:
- Iterates the same files in B again, calling `deficons_identify_file()` again for each

ARexx calls are expensive on Amiga — each call involves message passing to the DefIcons port.
Doubling these calls for every file in every subdirectory explains why large directory trees
appear to "hang" while still displaying the same folder name.

### Scale of the problem

For a tree with 3 levels containing 20 files per folder and 5 folders per level:

| Level | Folders | Files | Scans from has_visible_contents | Scans from create_in_directory | Total ARexx calls |
|---|---|---|---|---|---|
| 1 (root) | 1 | 20 | 0 (root never pre-scanned) | 20 | 20 |
| 2 | 5 | 20 each = 100 | 100 (from root's directory scan) | 100 | 200 |
| 3 | 25 | 20 each = 500 | 500 (from level-2 scans) | 500 | 1000 |

Total: 1220 ARexx calls when only 620 are needed.
The deeper the tree, the worse the ratio — always 2x overhead.

### The visual symptom

The progress window shows the folder currently being processed (from `CREATION_STATUS`), but
because `deficons_folder_has_visible_contents()` runs silently inside the parent's scan (it has
no progress output of its own), the status line still shows the parent folder's name while large
numbers of ARexx calls happen in child folders. This gives the impression of "stuck".

---

## 5. The Smart Mode Gap: Folder-Only Directories

A folder containing ONLY subdirectories (no files) will always get `has_visible_contents == FALSE`
from the current implementation, so it never receives a drawer icon.

This is arguably correct for a folder that is truly empty of files, but it can be surprising when:
- Folder B contains only folder C
- C contains iconable files
- B gets icons created in C by the recursive pass
- But B itself has no drawer icon from A's perspective
- B is invisible in A's Workbench window

The workaround today is to set mode = "Always", which creates drawer icons unconditionally.

---

## 6. Brainstorm: How to Improve Smart Mode

### Idea 1: Post-Order (Bottom-Up) Processing

**Concept:** Instead of checking `has_visible_contents` before recursing, recurse first and use
the results to drive drawer icon creation on the way back up.

```
ProcessRecursive(path):
    ProcessFilesInDirectory(path)        <- create file icons
    for each subdir:
        child_result = ProcessRecursive(subdir)   <- get child's result
        if child_result.has_visible_contents:     <- use real result, not pre-scan
            create drawer .info for subdir
    return aggregate result
```

**Benefits:**
- Eliminates the double-scan entirely — each file is visited exactly once
- No ARexx double-calls
- Correctly handles folder-only directories (if C got icons, B knows)
- Naturally propagates visibility upward: C has icons -> B gets drawer .info -> A gets drawer .info

**Downside:**
- Change is non-trivial: the current structure processes a directory then scans subdirs separately
- Would require merging Step 1 and Step 2 of `deficons_create_missing_icons_recursive`

**Code change summary:**
- In `deficons_create_missing_icons_recursive`, remove the separate child-drawer-icon creation
  from `deficons_create_missing_icons_in_directory` (the `DEFICONS_FOLDER_MODE_SMART` branch)
- After recursing into each subdir, check `child_result.has_visible_contents` and create the
  drawer icon at that point
- The `iTidy_DefIconsCreationResult` struct already carries `has_visible_contents` — this is
  already plumbed; it just isn't wired up correctly in the Smart decision

---

### Idea 2: Cache has_visible_contents During Creation

**Concept:** Instead of calling `deficons_folder_has_visible_contents()` as a separate pre-scan,
note that `deficons_create_missing_icons_in_directory()` already sets `result->has_visible_contents`
as a side effect. If we could defer the "create drawer icon for B" decision until after
`deficons_create_missing_icons_in_directory(B)` completes, we could use that cached result instead
of a separate pre-scan.

This is essentially Idea 1 but described as a caching optimisation.

---

### Idea 3: Shallow Scan Only, Accept the Limitation  

**Concept:** Keep the current pre-scan approach but make it explicit and documented that:
- Smart mode only looks at direct file contents when deciding drawer icons
- Folder-only directories never get drawer icons in Smart mode
- Add "Smart (deep)" as a fourth mode that does recurse in `has_visible_contents`

**Benefits:** Minimal code change, easy to explain to users.

**Downside:** Does not fix the double-scan performance problem.

---

### Idea 4: Progress Output in has_visible_contents

**Minimal change:** Add a status update to `deficons_folder_has_visible_contents()` so the
progress window shows "Checking: /path/to/subdir" during the pre-scan. This won't fix
performance but will stop the "stuck" appearance.

```c
if (progress_window)
{
    itidy_main_progress_window_append_status(progress_window, "  Checking: %s", path);
}
```

This adds visibility without any architectural change.

---

### Idea 5: Single-Pass with Deferred Drawer Creation

**Concept:** A variant of Idea 1 that is less invasive. Add a "deferred drawer list" structure:

1. During `deficons_create_missing_icons_in_directory(A)`, when Smart mode finds a subdir B:
   - Do NOT call `deficons_folder_has_visible_contents(B)` now
   - Add B to a "pending drawers" list
2. After the recursive pass completes for B (with a known result), evaluate the drawer decision
3. Create the drawer icon based on the real result

This requires a small data structure (a list of pending drawer paths + their results) but avoids
the double-scan.

---

### Idea 6: Combine the Two Examine Calls in `deficons_create_missing_icons_recursive`

**Observation:** The recursive function currently:
1. Calls `deficons_create_missing_icons_in_directory` (which locks and unlocks the dir)
2. Then locks the same dir AGAIN to iterate subdirs

This means the directory is locked twice per level. Combining them into a single Examine pass
(enumerate files and collect subdir names, then recurse) would halve the Lock/Examine overhead
and reduce the filesystem stress that causes the lock-table crash issue documented in the code.

---

### Idea 7: Limit ARexx Calls During Pre-Scan

**Concept:** In `deficons_folder_has_visible_contents()` Pass 2, limit the number of ARexx
calls made. For example, check the first 20 files and return FALSE if none match, rather than
scanning all 500 files in a large folder. A configurable limit would let users tune this.

**Rationale:** If 20 randomly-ordered files don't match, the folder is likely sparse and the
overhead of checking the rest may exceed the cost of simply not creating the drawer icon.

This is a heuristic trade-off and might miss valid cases, but in practice most iconable folders
have iconable files near the top of the directory listing.

---

## 7. Recommended Priority Order

| Priority | Change | Benefit | Risk |
|---|---|---|---|
| 1 | Add progress output to `has_visible_contents` (Idea 4) | Stops "stuck" appearance | Minimal |
| 2 | Post-order processing / deferred drawer creation (Ideas 1 or 5) | Eliminates double-scan, fixes folder-only bug | Medium |
| 3 | Combine the two Lock calls in `deficons_create_missing_icons_recursive` (Idea 6) | Reduces lock overhead | Low |
| 4 | "Smart (deep)" mode option (Idea 3 variant) | Covers folder-only case without full restructure | Low |
| 5 | ARexx call limit in pre-scan (Idea 7) | Performance tuning | Low |

---

## 8. Summary of Current Behaviour vs Ideal

| Scenario | Current behaviour | Ideal behaviour |
|---|---|---|
| Folder B has iconable files | Drawer .info created correctly | Same |
| Folder B has only subdirectories | No drawer .info created for B (gap) | Drawer .info created for B if any descendant gets icons |
| ARexx calls per file | 2x (pre-scan + creation) | 1x (creation only) |
| Progress visibility during Smart check | None — appears stuck | Shows "Checking: /path" |
| Deep nesting performance | Exponentially more expensive | Linear |

---

## 9. Answer to the Original Question

> "Does it really need to dig into subfolders when it's only checking that one folder to see
> if the parent drawer for it needs an icon?"

**No.** `deficons_folder_has_visible_contents()` does not recurse into subfolders of the folder
being checked. It examines only the direct contents (files and existing `.info` files) of the
target folder. Subdirectories inside the target are skipped in Pass 2.

However, the function IS called once per subdirectory found during the parent's scan, so in a
folder with many subdirs, many separate shallow scans happen. Each shallow scan involves ARexx
calls for files in that child folder. Then those same files get ARexx calls again during the
actual creation pass — this is the double-scan redundancy.

The apparent "stuck creating icons when it said it was just processing the original icon" is
explained by this: the parent's scan invisibly launches shallow scans of all its children, calling
ARexx for each child's files, with no progress output to indicate that's what is happening.
