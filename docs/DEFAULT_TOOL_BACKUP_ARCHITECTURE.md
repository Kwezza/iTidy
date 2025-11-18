# Default Tool Backup/Restore System Architecture

**Status:** Design Phase - Data Structures and Function Signatures Completed  
**Date:** November 18, 2025  
**For:** `src\GUI\default_tool_update_window.c` and related backup system

## Overview

This document describes the architecture for a lightweight CSV-based backup/restore system for default tool changes. Unlike the main iTidy backup system (LhA-based for folder operations), this is optimized for scattered icon files across the disk.

---

## Design Decision: CSV-Based vs LhA-Based

### Why CSV is Better for This Use Case

1. **Lightweight & Fast**
   - No file copying overhead
   - No LhA compression/decompression
   - Just text strings - minimal disk space
   - Perfect for metadata-only changes (icon default tool field)

2. **Scattered Files Problem**
   - Main backup assumes cohesive folder structure
   - Default tool updates can affect icons "scattered all over the hard disk"
   - LhA would create complex directory structures
   - Path resolution nightmares across volumes/devices

3. **Simplicity**
   - CSV format: `icon_path, old_tool, new_tool`
   - Easy to parse on restore
   - Human-readable for debugging
   - Could open in spreadsheet if needed

4. **Atomic Operations**
   - Each icon update = one CSV line
   - If something crashes mid-batch, you know exactly what succeeded
   - Easy audit trail

---

## Storage Structure

```
ENVARC:iTidy/Backups/
  icons/              (existing - for folder-based icon backups)
  tools/              (NEW - for default tool changes)
    20251118_142315/
      session.txt
      changes.csv
    20251118_151203/
      session.txt
      changes.csv
```

### session.txt Format
```
Session: Default Tool Update
Date: 18-Nov-2025 14:23:15
Mode: Batch
Scanned: Work:Projects/
Icons changed: 23
Icons skipped: 5
Total processed: 28
```

### changes.csv Format
```csv
# iTidy Default Tool Backup - 18-Nov-2025 14:23:15
"Work:Projects/App.info","SYS:Utilities/MultiView","SYS:Utilities/More"
"Work:Projects/Data.info","SYS:Utilities/MultiView","SYS:Utilities/More"
"Work:Projects/Old.info","SYS:Utilities/More","SYS:Utilities/MultiView"
"Work:Projects/New.info","","SYS:Utilities/IconEdit"
```

**Format:** `icon_path, old_tool, new_tool`

---

## Restore Window UI Design - Two ListView Approach

```
┌─ Restore Default Tool Changes ─────────────────────────────┐
│                                                              │
│ Select backup session:                                      │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ 18-Nov-2025 14:23 | Batch: Work:Projects/ | 23 changed  │ │ ← Top ListView
│ │ 18-Nov-2025 15:12 | Single: DH0:App.info  | 1 changed   │ │   (session list)
│ │ 17-Nov-2025 09:45 | Batch: SYS:Utilities/ | 8 changed   │ │
│ └──────────────────────────────────────────────────────────┘ │
│                                                              │
│ Tool changes in selected session:                           │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │ MultiView → More | 15 icons                             │ │ ← Bottom ListView
│ │ More → MultiView | 8 icons                              │ │   (tool type changes)
│ │ (none) → IconEdit | 2 icons                             │ │   within session
│ └──────────────────────────────────────────────────────────┘ │
│                                                              │
│         [Restore All] [Restore Selected] [Delete] [Close]   │
└──────────────────────────────────────────────────────────────┘
```

### User Flow

1. **User clicks top ListView** → selects a session (18-Nov-2025 14:23)
2. **Bottom ListView populates** with unique tool changes in that session
3. **User has 3 choices:**
   - **"Restore All"** → restores every icon in the entire session
   - **"Restore Selected"** → restores only icons with the selected tool change (e.g., all "MultiView → More" icons)
   - **"Delete"** → deletes the backup session folder

### Granularity Levels

**Basic Implementation (Recommended for MVP):**
- Restore entire session (all icons)
- Restore by tool type (all "MultiView → More" changes)

**Advanced (Future Enhancement):**
- Third window with checkboxes for individual icon selection
- Can selectively restore just 3 out of 15 icons
- Higher code complexity

---

## Configuration

### Added to `layout_preferences.h`:

```c
/* Default Tool Backup Settings */
BOOL enable_default_tool_backup;           /* Create CSV backup before default tool changes */
```

**Default:** `TRUE` (auto-backup enabled by default)

This allows future enhancement to make backups optional via preferences.

---

## Button Placement

### In `default_tool_update_window.c`:
Add button near bottom, between Status ListView and Close button:

```
[Update Default Tool]

┌─ Update Progress ─────────┐
│ ...                        │
└────────────────────────────┘

[Restore Previous Changes...] ← NEW BUTTON (opens restore window)
[Close]
```

### Future Menu System:
```
Tools → Update Default Tool...
     → Restore Default Tool Changes...  ← Also accessible here
```

---

## Data Structures

### Core Types (from `default_tool_backup.h`)

1. **iTidy_ToolBackupSession** - Session metadata for top ListView
2. **iTidy_ToolChange** - Grouped tool changes for bottom ListView
3. **iTidy_ToolBackupEntry** - Individual icon entry from CSV
4. **iTidy_ToolBackupManager** - Active backup session manager

### Window Structure (from `default_tool_restore_window.h`)

**iTidy_DefaultToolRestoreWindow** - Main restore window data
- Follows pattern from `amiga_window_template.c`
- Two ListViews (session list, tool change list)
- Selection state tracking
- Font handling for proportional fonts

---

## Function Categories

### Backup Operations (`default_tool_backup.h`)

- `iTidy_InitToolBackupManager()` - Initialize at startup
- `iTidy_StartBackupSession()` - Begin new backup session
- `iTidy_RecordToolChange()` - Log one icon change to CSV
- `iTidy_EndBackupSession()` - Finalize and close session
- `iTidy_CleanupToolBackupManager()` - Shutdown cleanup

### Restore Operations (`default_tool_backup.h`)

- `iTidy_ScanBackupSessions()` - Load list of available sessions
- `iTidy_LoadToolChanges()` - Load grouped tool changes for session
- `iTidy_LoadBackupEntries()` - Load all entries for restore
- `iTidy_RestoreAllIcons()` - Restore entire session
- `iTidy_RestoreToolChange()` - Restore specific tool change group
- `iTidy_DeleteBackupSession()` - Delete session folder

### Window Operations (`default_tool_restore_window.h`)

- `iTidy_OpenDefaultToolRestoreWindow()` - Create restore window
- `iTidy_CloseDefaultToolRestoreWindow()` - Cleanup restore window
- `iTidy_HandleDefaultToolRestoreEvents()` - Event processing
- `iTidy_RefreshSessionList()` - Reload session ListView
- `iTidy_PopulateToolChangeList()` - Update tool change ListView

---

## Implementation Guidelines

### Must Follow Templates

Before implementing the restore window, **MUST** read:

1. **`src\templates\AI_AGENT_GETTING_STARTED.md`** - Entry point guide
2. **`src\templates\amiga_window_template.c`** - Window creation pattern
3. **`AI_AGENT_GUIDE.md`** - Behavioral rules
4. **`AI_AGENT_LAYOUT_GUIDE.md`** - Layout geometry rules

### Critical GadTools Rules

**CRITICAL: Coordinate System (from AI_AGENT_GETTING_STARTED.md Section 3.5)**

GadTools gadgets use **window-relative coordinates**, NOT client-area relative:

```c
/* CORRECT: Calculate BorderTop BEFORE opening window */
WORD border_top = screen->WBorTop + screen->Font->ta_YSize + 1;
WORD border_left = screen->WBorLeft;

/* Position gadgets BELOW title bar */
WORD current_y = border_top + margin;  /* Start below title bar */
WORD current_x = border_left + margin;

ng.ng_TopEdge = current_y;  /* NOT 0, NOT margin alone! */
```

**Common Fatal Mistakes:**
- ❌ DON'T position gadgets at `ng_TopEdge = 0` or `ng_TopEdge = margin`
- ❌ DON'T use only `screen->WBorTop` (misses title bar font height!)
- ❌ DON'T forget the `+ 1` in the formula
- ❌ DON'T use `font_height * 2` or arbitrary multipliers

### ListView Cleanup Order (MANDATORY)

From `AI_AGENT_GUIDE.md` - ListView Cleanup Order Bug section:

```c
/* CORRECT cleanup order: */
1. Detach ListView labels (GTLV_Labels, ~0)
2. Free ListView data (list entries)
3. Clear menu strip (if present)
4. CloseWindow()
5. Free gadgets
6. Free menus
7. Free VisualInfo
8. Unlock screen
```

**Never** free list data while ListView still attached!

---

## Next Steps

### Phase 1: Basic Backup Integration

1. ✅ Add `enable_default_tool_backup` to `layout_preferences.h`
2. ✅ Create `default_tool_backup.h` with data structures
3. ✅ Create `default_tool_restore_window.h` with window structure
4. ⏳ Implement `default_tool_backup.c` (backup/restore functions)
5. ⏳ Integrate backup into `default_tool_update_window.c`:
   - Check preference flag
   - Start session before update
   - Record each change
   - End session after update

### Phase 2: Restore Window

1. ⏳ Implement `default_tool_restore_window.c` based on `amiga_window_template.c`
2. ⏳ Add "Restore Previous Changes..." button to update window
3. ⏳ Test session browsing and selection
4. ⏳ Test "Restore All" functionality
5. ⏳ Test "Restore Selected" (by tool type) functionality
6. ⏳ Test "Delete" session functionality

### Phase 3: Polish & Integration

1. ⏳ Add to future menu system
2. ⏳ Add cleanup strategy (max sessions, age-based deletion)
3. ⏳ Add error handling and user feedback
4. ⏳ Documentation and testing

---

## Benefits Summary

✅ **Lightweight** - Text files only, no compression overhead  
✅ **Fast** - No file copying, just metadata  
✅ **Scalable** - Works with scattered files across volumes  
✅ **Atomic** - Clear audit trail of what succeeded/failed  
✅ **Simple** - Human-readable CSV format  
✅ **Flexible** - Restore all, by type, or future per-icon selection  
✅ **Safe** - Auto-backup enabled by default  
✅ **Clean** - Separate storage from main icon backups  

---

## File Manifest

**Created Files:**
- `src/GUI/default_tool_backup.h` - Backup/restore data structures and functions
- `src/GUI/default_tool_restore_window.h` - Restore window interface

**Modified Files:**
- `src/layout_preferences.h` - Added `enable_default_tool_backup` field and default

**Pending Implementation:**
- `src/GUI/default_tool_backup.c` - Core backup/restore logic
- `src/GUI/default_tool_restore_window.c` - Restore window implementation
- Integration into `src/GUI/default_tool_update_window.c`

---

**End of Architecture Document**
