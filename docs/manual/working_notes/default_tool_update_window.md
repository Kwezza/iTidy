# Default Tool Update Window — Working Notes

**Source**: `src/GUI/DefaultTools/default_tool_update_window_reaction.c`, `default_tool_update_window_reaction.h`
**Title bar**: "iTidy - Replace Default Tool"
**Opened from**: Default Tool Analysis window -> Replace Tool (Batch) or Replace Tool (Single)
**Modal**: Effectively modal (blocks the Tool Cache window)
**Resizable**: Yes (initial 500x350)
**Keyboard shortcuts**: Ctrl+C = close

---

## Purpose

Replaces the default tool in one or more icon files. Operates in two modes:
- **Batch mode**: Updates all icons that reference a specific tool (e.g. change all icons using "OldTool" to use "MultiView")
- **Single mode**: Updates one specific icon file

---

## Window Layout

```
+----------------------------------------------------+
| Current Tool:  MultiView                           |
| Mode:          Batch Mode: 23 icon(s) will be...   |
| Change To:     [file browser.................] [>]  |
+----------------------------------------------------+
| Update progress                                     |
| +------------------------------------------------+ |
| | Status    | File Path                          | |
| | SUCCESS   | Work:Projects/MyApp.info           | |
| | FAILED    | Work:Old/Program.info              | |
| | READ-ONLY | SYS:Prefs/Palette.info             | |
| +------------------------------------------------+ |
|                                                     |
| [Update Default Tool]                     [Close]   |
+----------------------------------------------------+
```

---

## Info Section (Top)

Three read-only rows showing:
- **Current Tool**: The tool being replaced (e.g. "MultiView")
- **Mode**: "Batch Mode: N icon(s) will be updated" or "Single mode: <path>"
- **Change To**: GetFile gadget for selecting the replacement tool program

### Change To (GetFile)

File browser for selecting the new default tool. Opens with initial drawer set to `C:`. Rejects `.info` files. The text field is read-only — the user must use the browse button.

Leaving this empty and clicking Update will prompt to **clear** the default tool from the icon(s), effectively removing the tool association.

**Hint:** "Select the new default tool program. Leave empty to remove the tool association from the selected icon(s). Cannot select .info files."

---

## Progress List (ListBrowser)

Two-column list showing results as icons are processed:

| Column | Content |
|--------|---------|
| Status | "SUCCESS", "FAILED", or "READ-ONLY" |
| File Path | The `.info` file path |

In batch mode, the display is updated every 5 icons for visual feedback.

---

## Buttons

### Update Default Tool

Starts the replacement process. Shows a busy pointer during operation.

**Before modifying each icon:**
1. Checks if the file is write-protected — skips with "READ-ONLY" status if so
2. Reads the current default tool from the icon
3. Writes the new default tool (or clears it if the field was empty)
4. Records the change in the backup system (if backups enabled)
5. Updates the in-memory tool cache

**If the Change To field is empty**: Shows a confirmation requester — "This will remove the default tool from the selected icon(s). The icon(s) will no longer launch a specific program. Are you sure you want to continue?"

After completion, shows a summary requester with success/failure counts. The Update button is then permanently disabled — one update per window opening.
**Hint:** "Starts replacing the default tool in the selected icon(s). The button is disabled after one use per window opening."
### Close

Closes the window. On close, the parent Tool Cache window is automatically refreshed to reflect any changes made.

**Hint:** "Closes this window. The Default Tool Analysis window is automatically refreshed to show the updated tool assignments."

---

## Backup System

If default tool backup is enabled in preferences (`Settings -> Backup -> Enable Default Tool Backup`), every change is recorded:
- A session folder is created under `PROGDIR:Backups/tools/YYYYMMDD_HHMMSS/`
- Contains `session.txt` (metadata) and `changes.csv` (icon path, old tool, new tool per row)
- Failed/skipped icons are counted but not recorded in the CSV

This backup data is used by the "Restore Default Tools Backups" feature in the Tool Analysis window.

---

## Notes for Manual

- The Update button is single-use — after updating, the user must close and reopen to do another update.
- Write-protected icons are automatically skipped with a "READ-ONLY" status, not treated as errors.
- Clearing the default tool (empty Change To field) is a deliberate feature, not an error.
- The backup system provides an undo trail — use Restore Default Tools Backups to revert changes.
- After closing this window, the Tool Analysis window automatically refreshes to show the updated tool assignments.
