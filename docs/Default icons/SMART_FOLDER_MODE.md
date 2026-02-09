# Smart Folder Icon Mode

This document describes how iTidy's DefIcons "Smart" folder (drawer) icon creation mode works.

## Purpose

The Smart mode creates a drawer `.info` only when the folder participates in the icon layout —
either because it already contains visible icons or because icon creation will produce visible icons
inside it or in its subdrawers.

## Decision function

The decision is performed by the function:

`deficons_should_create_folder_icon(path, has_visible_contents, prefs)`

- `prefs->deficons_folder_icon_mode` values:
  - `0` = Smart
  - `1` = Always
  - `2` = Never
- When mode == `0` the function returns the boolean `has_visible_contents`.

## Definition of has_visible_contents

`has_visible_contents` is true when ANY of the following is true:

1. The folder already contains one or more `.info` files (i.e., it already participates in layout).
2. The folder contains at least one regular file that will get an icon created (it passes DefIcons
   identification and the user filtering rules).
3. The folder contains at least one subdrawer which already has, or will have, a `.info` file.

## How it is computed

- During the icon-creation pre-pass (functions such as `CreateMissingIconsInDirectory()` and
  `CreateMissingIconsRecursive()`), the directory is scanned and a local `has_visible_contents`
  flag is maintained.
- The scan sets `has_visible_contents` to true as soon as any of the three conditions above are met.
- Drawer `.info` creation is deferred until after the folder's entries have been evaluated — the
  drawer `.info` is only created if `has_visible_contents` is true.

## Ordering and safety

- The icon creation pre-pass runs before the main tidying/layout pass so newly-created drawer icons
  are included in the subsequent layout.
- Existing `.info` files are preserved: the code checks for `.info` presence (via `Lock()`/open checks)
  and will not overwrite existing icons.
- The logic respects system/exclusion preferences (for example `deficons_skip_system_assigns`) and the
  user's type filters defined in preferences.

## Practical outcome

- Empty folders with no `.info` and no icon-able contents are left without drawer icons in Smart mode.
- Folders that already participate in icon layout, or that will contain created icons after the pre-pass,
  receive a drawer `.info` so they appear in the final layout.

## Notes for implementers

- Ensure `has_visible_contents` observes both file-level and subdrawer-level conditions to avoid
  creating drawer icons for truly empty folders.
- Run cancellation checks (`CHECK_CANCEL()`) during long scans and respect recursion limits when
  evaluating subdrawers.

---

Document created from the DefIcons integration and type-selection plans.
