# iTidy v2.0 - New Features vs v1.0

This document summarises the features added or changed in iTidy v2.0 compared to v1.0.

---

## Platform & Requirements
- **Workbench 3.2+ required** (v1 supported WB 3.0+)
- **Native ReAction GUI** — the entire interface has been rewritten using the WB 3.2 ReAction framework

---

## Icon Creation (entirely new feature)
- **DefIcons integration** — automatically create icons for files/folders that don't have `.info` files
- **Picture file thumbnails** — miniature image previews for ILBM, PNG, GIF, JPEG, BMP, ACBM, and other DataType formats
- **Text file preview icons** — renders a thumbnail of file contents onto the icon image
- **Configurable rendering options**: preview size (48×48 / 64×64 / 100×100), border style, upscaling, colour depth (4–256+Ultra), dithering method, and low-colour palette
- **WHDLoad folder skipping** — optionally skip icon creation inside WHDLoad slave folders
- **Replace existing iTidy-created icons** — can selectively regenerate thumbnails after settings changes (tracked via `ITIDY_CREATED` tooltype)
- **Icon Creation Settings window** with Create, Rendering, and DefIcons tabs

---

## DefIcons Categories Window (new)
- Tree view of all DefIcons file types from `ENV:deficons.prefs`
- Per-type enable/disable checkboxes
- View and change the default tool assigned to each file type
- Select All / Select None controls

---

## Text Templates Window (new)
- Manage per-ASCII-subtype template icons (`def_c.info`, `def_rexx.info`, etc.)
- Create custom templates from the master `def_ascii.info`
- Edit template ToolTypes (`ITIDY_TEXT_AREA`, `ITIDY_LINE_HEIGHT`, colours, etc.) via the Workbench Information editor
- Validate template ToolTypes for errors
- Revert custom templates to master

---

## Exclude Paths Window (new)
- List of directories to skip during DefIcons icon creation
- `DEVICE:` placeholder makes paths portable across volumes
- Default system directories pre-populated (C:, Libs:, Devs:, etc.)
- Add / Remove / Modify entries

---

## Default Tool Analysis — Improvements
- Now has a **Details panel** showing which specific icon files reference the selected tool
- Sortable column headers in the tool list
- **View menu** with "System PATH..." to inspect the tool search path
- Improved progress window during scanning

---

## Replacing Default Tools (new dedicated window)
- **Batch mode** (replace tool across all matching icons) now in its own window
- **Single mode** (replace tool in one icon file)
- **Clear a default tool** by leaving the replacement field empty
- Per-icon result log (SUCCESS / FAILED / READ-ONLY)
- Post-completion summary requester

---

## Restoring Default Tool Backups (new dedicated window)
- Separate backup/restore system just for default tool changes
- Session list with Date/Time, Mode, Path, and Changed count
- Per-session change detail panel showing old tool -> new tool

---

## Restore Layout Backups — Improvements
- New **Icons+** column showing count of DefIcons-created icons in each run
- Restore now offers three choices: *With Windows*, *Icons Only*, or *Cancel*
- DefIcons-created icons are removed during a restore
- View now shows "NoCAT" / "Orphaned" / "Corrupted" run statuses
- **Folder View sub-window** (new) — hierarchical tree showing archived folder structure within a run

---

## Presets System (replaces basic Load/Save Settings)
- **Reset To Defaults** menu item (Amiga+N)
- **Save Preset As...** with configurable filename/location
- Preset files default to `PROGDIR:userdata/Settings/`
- `LOADPREFS` ToolType for auto-loading a preset at startup

---

## Menus (greatly expanded)
- **Settings menu**: DefIcons submenu (Categories, Preview Icons, Excluded Folders), Backups submenu (toggle), Logging submenu (Disabled/Debug/Info/Warning/Error + Open Log Folder)
- **Tools menu**: Restore submenu (Restore Layouts, Restore Default Tools)
- **Help menu**: Open AmigaGuide manual, About requester

---

## ToolTypes (new)

| ToolType | Purpose |
|----------|---------|
| `LOADPREFS` | Auto-load a preferences file on startup |
| `DEBUGLEVEL` | Set log level at launch |
| `PERFLOG` | Enable performance timing logs |

---

## Advanced Settings — Minor Additions/Changes
- **Auto-Fit Columns** moved to Filters & Misc tab (was on Columns tab in v1)
- Default values explicitly documented for all settings
- **Defaults** button added to reset just the Advanced window
