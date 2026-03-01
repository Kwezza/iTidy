# Progress Windows — Working Notes

**Source**: `src/GUI/StatusWindows/` (4 files)
**Status**: Two active progress windows plus one unused legacy implementation

---

## Overview

iTidy has three progress window implementations, though only two are actively used:

1. **Main Progress Window** (ReAction) — used during icon processing and tool scanning
2. **Recursive Progress Window** (ReAction) — used during backup restoration
3. **Simple Progress Window** (GadTools) — unused/legacy, preserved in source

---

## 1. Main Progress Window

**Source**: `main_progress_window.c` (475 lines)
**Title**: "iTidy - Progress"
**Used by**: Main processing (Apply button), Default Tool Analysis scanning

### Display

- **Scrolling status list** (ListBrowser): Shows a history of status messages (up to 50 entries, oldest removed when full). Auto-scrolls to keep the newest entry visible.

  **Hint:** "Shows a scrollable history of processing status messages. Auto-scrolls to keep the newest entry visible."

- **Status text**: Below the list, shows current operation progress (e.g. "Scanning: 34 / 200")
- **Cancel/Close button**: Full-width button at the bottom

### Behaviour

During processing, the button shows "Cancel". Clicking it shows a confirmation requester: "Are you sure you want to cancel? Changes will not be reverted." If confirmed, the current operation is flagged for cancellation.

After processing completes, the button text changes to "Close" and clicking it simply closes the window without confirmation.

**Hint:** "Cancels the current operation after confirmation (during processing), or closes the window (when done). Changes already made are not reverted."

The status text is updated periodically with a heartbeat/spinner showing the current phase, item count, and progress. The event loop is pumped during updates to keep the Cancel button responsive.

Resizable (min 300x200, opens at 400x300 centred on mouse).

---

## 2. Recursive Progress Window

**Source**: `recursive_progress.c` (652 lines)
**Title**: Caller-supplied (e.g. "Restoring backup_run_0007")
**Used by**: Backup restore operations

### Display

- **Main progress bar** (FuelGauge): Shows folder-level progress with built-in percentage display (e.g. "42%")
- **Main label**: Shows current folder being processed (e.g. "Processing: Work:Programs/ (42/500)")
- **Sub progress bar** (FuelGauge, optional): Shows icon-level progress within the current folder
- **Sub label** (optional): Shows icon count (e.g. "Icons: 15/43" or "No icons in folder")

### Behaviour

Requires a prescan phase first — `iTidy_PrescanRecursive()` walks the entire directory tree to count folders and icons before the progress window opens. This allows accurate percentage display.

The sub-progress bar is optional — callers can disable it (backup restore uses folder-level only).

No cancel button or close gadget. The window is opened for the duration of the operation and closed automatically when complete.

Not resizable.

### Prescan

The prescan walks the directory tree recursively, counting folders and icons per folder. It yields to multitasking every 100 items to prevent the system from appearing frozen. Hidden folders (starting with ".") are skipped.

---

## 3. Simple Progress Window (Unused)

**Source**: `progress_window.c` (462 lines)
**Title**: Caller-supplied
**Used by**: Currently no callers — legacy implementation

A single-bar progress window using GadTools (not ReAction). Features a 3D bevelled progress bar, percentage text, and path-truncated helper text showing the current item. Shows a busy pointer during operation. After completion, displays a Close button.

Has smart redrawing that only updates changed elements to minimize flicker.

---

## Notes for Manual

- The main progress window appears during icon processing (Start button) and tool scanning — it shows a scrollable log of what iTidy is doing.
- Cancel is available during processing but changes already made will not be reverted.
- During backup restoration, a dual progress bar shows folder-level and optionally icon-level progress.
- The prescan phase counts folders before restoration begins to enable accurate progress display.
