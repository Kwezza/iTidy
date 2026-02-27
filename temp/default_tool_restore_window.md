# Default Tool Restore Window — Working Notes

**Source**: `src/GUI/DefaultTools/default_tool_restore_window.c` (1302 lines)
**Title bar**: "iTidy - Restore Default Tools"
**Opened from**: Main window -> Tools -> Restore Default Tools; also from Default Tool Analysis -> Restore Default Tools Backups
**Modal**: Effectively modal (blocks parent)
**Resizable**: Yes (initial 600x400, min 400x300)
**Keyboard shortcuts**: R (Restore), D (Delete), C (Close)

---

## Purpose

Allows the user to view and restore backed-up default tool changes. Each time iTidy replaces default tools (via the Replace Tool feature), a backup session is created. This window lists all backup sessions and lets the user restore icons to their previous default tools or delete old backup sessions.

---

## Window Layout

```
+----------------------------------------------------+
|              Backup Sessions:                       |
| +------------------------------------------------+ |
| | Date/Time    | Mode   | Path          | Changed| |  <- sessions (60%)
| | 2024-01-15.. | Batch  | Work:Projects |     23 | |
| | 2024-01-14.. | Single | SYS:Utilities |      1 | |
| +------------------------------------------------+ |
|           Tool Changes in Session                   |
| +------------------------------------------------+ |
| | Old tool:  | OldViewer                          | |  <- changes (35%)
| | New tool:  | MultiView                          | |
| | Icons:     | 23                                 | |
| +------------------------------------------------+ |
| [Restore]         [Delete]              [Close]     |  <- buttons (5%)
+----------------------------------------------------+
```

---

## Gadgets

### Session List (Upper ListBrowser)

Displays all backup sessions found in `PROGDIR:Backups/tools/`. Four sortable columns:

| Column | Content |
|--------|---------|
| Date/Time | Formatted timestamp of when the backup was created |
| Mode | "Batch" or "Single" |
| Path | The folder path that was being processed (truncated to 50 chars) |
| Changed | Number of icons that were modified |

Shows a busy pointer while scanning for sessions on window open.

### Changes List (Lower ListBrowser)

Shows the tool changes recorded in the selected session. Uses a label/value format with no column headers:

For each unique tool change in the session:
- **Old tool**: The original default tool (or "(none)" if it was empty)
- **New tool**: The replacement tool (or "(none)" if cleared)
- **Icons**: Number of icons affected by this specific change

### Restore

Restores all icons from the selected backup session to their original default tools.

1. Shows a confirmation requester: "Restore N icon default tool(s) from backup session <date>?"
2. On confirm, reads the session's `changes.csv` and reverts each icon's default tool
3. Shows a completion requester with success/fail counts
4. Does NOT delete the backup session after restoring

Only enabled when a session is selected in the upper list.

### Delete

Permanently deletes the selected backup session and its files.

1. Shows a confirmation requester: "Delete backup session <date>? This will permanently delete N tool change record(s). This action cannot be undone!"
2. On confirm, removes the session directory
3. Refreshes the session list and clears the changes display

Only enabled when a session is selected.

### Close

Closes the window.

---

## Button State Logic

| Condition | Restore | Delete | Close |
|-----------|---------|--------|-------|
| No session selected | Disabled | Disabled | Enabled |
| Session selected | Enabled | Enabled | Enabled |

Both Restore and Delete start disabled and enable only when a session row is clicked.

---

## Backup Storage

Sessions are stored under `PROGDIR:Backups/tools/` with timestamped directory names (`YYYYMMDD_HHMMSS`). Each directory contains:
- `session.txt` — metadata (mode, path, icon counts)
- `changes.csv` — one row per icon: icon path, old tool, new tool

---

## Notes for Manual

- Backup sessions are created automatically when using Replace Tool (if backup is enabled in settings).
- Restoring does not delete the backup session — sessions persist until manually deleted.
- After restoring, the tool cache in the Default Tool Analysis window is invalidated and must be rebuilt with a new scan.
- The Delete action is permanent — deleted sessions cannot be recovered.
- Multiple tool changes in a single session are grouped and displayed as unique old->new tool pairs with icon counts.
