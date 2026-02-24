# Backdrop Cleaner / WB Screen Feature - Temporarily Removed

**Date Removed:** 2026-02-24  
**Version:** iTidy v2.0 (current)  
**Status:** Deferred to a future release

---

## What Was This Feature?

The Backdrop Cleaner (also referred to as "WB Screen Manager") was a new window
accessible via a "WB Screen..." button in the main iTidy window. Its intended
purpose was:

- Audit the Workbench `.backdrop` files on each device/volume
- Remove orphaned entries pointing to icons that no longer exist
- Tidy the positions of "left-out" (backdrop) icons on the Workbench screen
- Apply an organised grid layout to icons left out on individual devices

### Files Written (Kept for Future Use)

The following files were written but are **excluded from the build** until the
feature is re-evaluated:

| File | Description |
|------|-------------|
| `src/GUI/BackdropCleaner/backdrop_window.c` | ReAction GUI window for the feature |
| `src/GUI/BackdropCleaner/backdrop_window.h` | Header for the window |
| `src/layout/workbench_layout.c` | Layout engine for WB screen icon positioning |
| `src/layout/workbench_layout.h` | Header for the layout engine |

The `src/backups/backdrop_parser.c` / `.h` files remain **in the build** as they
are part of the general backup subsystem (used to read `.backdrop` files when
processing device icon layouts).

> **Note:** A dedicated backdrop backup/restore system was planned but was never
> implemented. There are no orphaned backup files to clean up.

---

## Why Was It Removed?

Testing revealed a fundamental Workbench OS behaviour that prevents this feature
from working reliably:

**Icons that are "left out" on the Workbench screen do not have their positions
reliably restored to the same coordinates after a reboot.**

When Workbench re-reads `.backdrop` files at startup, icons are placed using a
"best guess, first-come first-served" algorithm - there is no guarantee that an
icon will return to the exact pixel position it was at before the reboot, even
if the saved position in the `.info` file is correct. Workbench appears to
resolve layout conflicts at startup by moving icons to free space rather than
honouring stored coordinates strictly.

This means that any positions iTidy writes for left-out icons would be silently
overridden by Workbench on the next boot, making the feature unreliable from
the user's point of view.

---

## Possible Future Solutions

The feature could be revisited when more time is available to investigate the
following approaches:

1. **Startup script approach** - Write a `User-Startup` or `s:Startup-Sequence`
   script entry that re-applies icon positions after Workbench has finished
   loading (e.g., using a small Arexx script or a second pass of iTidy).

2. **WBStartup app** - A small background utility placed in `WBStartup:` that
   runs after Workbench has settled and re-positions backdrop icons to their
   saved coordinates.

3. **Research deeper** - Investigate whether the `icon.library` `PutDiskObject()`
   call can be made after Workbench has fully initialised its screen to reliably
   override the startup placement.

---

## What Was Removed from the Build

The following changes were made when removing this feature:

- `src/layout/workbench_layout.c` removed from `LAYOUT_SRCS` in `Makefile`
- `BACKDROP_GUI_SRCS` variable and `src/GUI/BackdropCleaner/backdrop_window.c`
  removed from `Makefile`
- `BACKDROP_GUI_OBJS` removed from `Makefile`
- `#include "BackdropCleaner/backdrop_window.h"` removed from `main_window.c`
- "WB Screen..." button gadget creation removed from `main_window.c`
- `ITIDY_GAID_BACKDROP_BUTTON` hint info entry removed from `main_window.c`
- `case ITIDY_GAID_BACKDROP_BUTTON:` event handler removed from `main_window.c`
- `ITIDY_GAD_IDX_BACKDROP_BUTTON` enum entry removed from `main_window_reaction.h`
- `GID_MAIN_BACKDROP_BUTTON` and `ITIDY_GAID_BACKDROP_BUTTON` defines removed
  from `main_window_reaction.h`

The source files themselves (`backdrop_window.c/h`, `workbench_layout.c/h`) are
**kept on disk** for future reference - they are simply not compiled.
