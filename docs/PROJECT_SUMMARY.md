# iTidy — Project Summary

iTidy is a native Amiga Workbench utility that automatically tidies drawer icon layouts and resizes drawer windows to match the icon grid. It is aimed at AmigaOS 3.x (Workbench 3.0+), written in C (VBCC-compatible), and built around classic Intuition + GadTools (no MUI/ReAction).

## What it does

- **Icon tidying**: reads Workbench `.info` files, sorts and lays out icons into a consistent grid, and writes updated positions back to disk.
- **Window resizing**: computes an appropriate drawer window size based on icon geometry, fonts, and screen/border metrics.
- **Recursive processing**: can process a single drawer or an entire directory tree (useful for large WHDLoad sets, archive extracts, and project trees).
- **Backup + restore safety net**: optional LhA-based backups for `.info` files/window layout changes, with restore tooling.
- **Default tool analysis / repair**: scans icons (especially WBPROJECT icons) to find missing/invalid Default Tool settings, and supports repair with its own backup mechanism.

## Supported icon formats

iTidy is designed to handle the common Workbench icon formats seen on 3.x systems:

- Standard/classic icons
- NewIcons
- OS3.5/OS3.9 “color icons” (GlowIcons supported as represented by these formats)

## System requirements (runtime)

From the user manual:

- Workbench 3.0 or newer
- ~1MB free memory (minimum guidance)
- Space for backups (more required if backups are enabled)
- For backup/restore features: **LhA** available in `SYS:C` (see the manual for details)

User manual: [docs/manual/README.md](manual/README.md)

## Build targets (developer)

The project builds in two modes via the top-level Makefile:

- **Amiga target (default)**: VBCC cross-compile for 68k Amiga
- **Host target**: GCC build used for local/PC-side testing of algorithms and helper code

Common commands:

- `make` (default Amiga build)
- `make clean && make`
- `make CONSOLE=1` (enables console output, useful for debugging)
- `make TARGET=host` (host build)

Build config: [Makefile](../Makefile)

## High-level architecture

- **GUI layer**: GadTools/Intuition windows that collect preferences and trigger processing.
- **Processing layer**: orchestrates directory traversal, optional backups, scanning icons, sorting, layout calculation, and writing results.
- **Icon layer**: reads/writes DiskObject data via icon.library and related APIs.
- **Preferences layer**: central global preferences structure plus system preference reads (Workbench/IControl).
- **Backup system**: LhA integration plus catalog/marker/run/session modules.
- **Platform layer**: host vs Amiga platform abstractions.

## Source layout (where to look)

- Core entry point: [src/main_gui.c](../src/main_gui.c)
- GUI windows: [src/GUI/](../src/GUI/)
- Processing orchestration: [src/layout_processor.c](../src/layout_processor.c)
- Layout engine: [src/aspect_ratio_layout.c](../src/aspect_ratio_layout.c)
- Icon IO: [src/icon_management.c](../src/icon_management.c)
- Folder scanning: [src/folder_scanner.c](../src/folder_scanner.c)
- Backup system: [src/backup_session.c](../src/backup_session.c) and related `backup_*.c` modules
- Settings/prefs: [src/Settings/](../src/Settings/)

## Notes / constraints

- iTidy operates on Workbench metadata (primarily `.info` files and window/layout data); it does **not** modify the contents of user data files.
- The GUI is intentionally classic Workbench-native (GadTools/Intuition) for compatibility.

## Further reading

- End-user manual: [docs/manual/README.md](manual/README.md)
- Project overview (broader design notes): [docs/PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md)
- GUI development history/status: [docs/GUI_DEVELOPMENT_SUMMARY.md](GUI_DEVELOPMENT_SUMMARY.md)
