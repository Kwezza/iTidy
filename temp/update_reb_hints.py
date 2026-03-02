#!/usr/bin/env python3
"""
Update iTidy35.reb with improved hint text and list item labels.
Cross-referenced from docs/manual/working_notes/ documents.

Changes:
- Updates ~55 existing hints with improved wording from working notes
- Adds ~32 missing hints for interactive gadgets
- Updates 8 LISTITEM values in dco_list_max_colors for clarity
"""

import shutil
import sys
import os

REB_FILE = r'c:\Amiga\Programming\iTidy\Bin\Amiga\Rebuild\iTidy35.reb'
BACKUP_FILE = REB_FILE + '.bak'
MAX_HINT_LEN = 194

# ============================================================
# HINT CHANGES: IDENT -> new hint text
# ============================================================
HINT_CHANGES = {
    # ---- Main Window ----
    'main_gf_target_path':
        'Click to open a directory requester and choose the drawer iTidy will process.',
    'main_ch_sort_primary':
        'Sets how icons are grouped before sorting. Choose folders first, files first, mixed, or grouped by file type.',
    'main_ch_sort_secondary':
        'Sets the sort order within the current grouping mode. Disabled when "Grouping" is set to "Grouped By Type".',
    'main_ch_positioning':
        'Controls where drawer windows are placed after iTidy resizes them. Only affects windows that iTidy actually changes.',
    'main_cb_cleanup_subfolders':
        'When enabled, all subfolders under the selected folder are also processed. Use with care on large directory trees.',
    'main_cb_create_new_icons':
        'When enabled, iTidy creates new icons for files that do not already have .info files, using the DefIcons system.',
    'main_cb_backup_icons':
        "When enabled, an LhA backup of each folder's .info files is created before making changes. Requires LhA in C:.",
    'main_btn_advanced':
        'Opens the Advanced Settings window for finer control over layout, density, limits, columns, grouping gaps, and filtering.',
    'main_btn_default_tools':
        'Scans icons for missing or invalid default tools. Lets you review and fix broken tool paths in batch or one at a time.',
    'main_btn_restore_backups':
        'Opens the Restore Backups window to restore icon positions and window layouts from a previous iTidy backup run.',
    'main_btn_icon_settings':
        'Opens the Icon Creation Settings window to configure thumbnail generation, text previews, folder icons, and DefIcons categories.',
    'main_btn_start':
        'Starts tidying the selected folder using the current settings. A progress window shows status during processing.',
    # main_btn_exit: "Closes iTidy." is fine as-is

    # ---- Main Progress Window ----
    'mp_lb_status':
        'Shows a scrollable history of processing status messages. Auto-scrolls to keep the newest entry visible.',
    'mp_btn_cancel':
        'Cancels the current operation after confirmation, or closes the window when done. Changes already made are not reverted.',

    # ---- Advanced Settings ----
    'adv_ch_layout_aspect':
        'Sets the target width-to-height proportions for drawer windows when iTidy resizes them.',
    'adv_ch_layout_when_full':
        'Controls how iTidy expands a drawer window when it has more icons than fit at the target proportions.',
    'adv_ch_layout_align_vertically':
        'Sets how icons are aligned vertically within a row when icons of different heights appear in the same row.',
    'adv_int_iconspacing_x':
        'Sets the horizontal gap between icons in pixels. Lower values pack icons tighter; higher values give more breathing room.',
    'adv_int_iconspacing_y':
        'Sets the vertical gap between icons in pixels. Lower values pack icons tighter; higher values give more breathing room.',
    'adv_int_icons_per_row_min':
        'Sets the minimum number of columns in a drawer window, preventing icons from being arranged in one long vertical list.',
    'adv_cb_icons_per_row_autolayout':
        'When enabled, the maximum number of columns is calculated automatically based on window width and screen size.',
    'adv_int_icons_per_row_max':
        'Sets the maximum number of columns. Only used when "Auto-Calc Max Icons" is turned off.',
    'adv_ch_layout_max_width':
        'Limits how wide drawer windows may become, as a percentage of screen width. Prevents windows from filling the entire screen.',
    'adv_cb_column_layout':
        'When enabled, icons are arranged in strict vertical columns. When disabled, a free-flow row-based layout is used.',
    'adv_ch_layout_gap_between_groups':
        'Controls the spacing between icon groups when "Grouping" on the main window is set to "Grouped By Type".',
    'adv_cb_optimize_columns':
        'When enabled, each column is sized to fit its widest icon, producing a more compact and visually balanced layout.',
    'adv_cb_reverse_sort':
        'Reverses the sort direction. Applies to name (Z-A), date (newest first), and size (largest first) sorting.',
    'adv_cb_strip_newicon_borders':
        'Permanently strips the black border from NewIcons. Requires icon.library v44 or later. Enable backups before using.',
    'adv_cb_skip_hidden':
        'When enabled, folders without a .info file are skipped during recursive processing, as they are not visible in Workbench.',
    'adv_btn_defaults':
        'Resets all settings in this window to their factory defaults. A confirmation requester is shown before the reset takes place.',
    'adv_btn_cancel':
        'Discards all changes and returns to the main window. The original settings are preserved unchanged.',
    'adv_btn_ok':
        'Accepts the current settings and closes the window. Changes are applied to the session but not saved to disk.',

    # ---- Restore Backups ----
    'rb_lb_backups':
        'Shows all iTidy backup runs. Click a run to view its details below. Double-click to view the folder structure.',
    'rb_lb_details':
        'Shows details of the selected backup run: date created, source folder, number of archives, total size, and status.',
    'rb_btn_delete':
        'Permanently deletes the selected backup run and all its files. A confirmation requester is shown first. This cannot be undone.',
    'rb_btn_restore':
        'Restores icon positions and window layouts from the selected backup run. Choose with or without window positions. Requires LhA.',
    'rb_btn_view_folders':
        'Opens a tree view showing the hierarchical folder structure of the selected run. Only available for runs with a catalog file.',
    'rb_btn_close':
        'Closes the Restore Backups window.',

    # ---- Folder View ----
    'rfv_lb_folders':
        'Shows the hierarchical folder structure of this backup run. Click the disclosure triangles to expand or collapse branches.',
    'rfv_btn_close':
        'Closes the folder view and returns to the Restore Backups window.',

    # ---- Default Tool Analysis ----
    'dta_ch_folder':
        'Select the root folder to scan for default tool information. Drawers only.',
    'dta_btn_scan':
        'Starts a recursive scan of the selected folder to find all default tools referenced by icons. Opens a progress window.',
    'dta_str_filter':
        'Filters the tool list to show all tools, only valid tools, or only missing tools. Disabled until tool data is loaded.',
    'dta_lb_tools':
        'Shows all discovered default tools, how many icons reference each one, and whether the tool exists on disk. Click a row for details.',
    'dta_lv_status':
        'Shows details of the selected tool: name, status, version string, and the list of icon files that reference it.',
    'dta_btn_replace_batch':
        'Opens the Replace Default Tool window to update all icons that reference the selected tool at once.',
    'dta_btn_restore_backup':
        "Opens the Restore Default Tools window to revert previous default tool replacements from iTidy's backup records.",
    'dta_btn_replace_single':
        'Opens the Replace Default Tool window to update the default tool in one specific icon file selected in the details panel.',
    'dta_btn_close':
        'Closes the Default Tool Analysis window.',

    # ---- Default Tool Update ----
    'dtu_ch_new_tool':
        'Select the new default tool program. Leave empty to remove the tool association from the selected icon(s).',
    'dtu_btn_update':
        'Starts replacing the default tool in the selected icon(s). The button is disabled after one use per window opening.',
    'dtu_btn_close':
        'Closes this window. The Default Tool Analysis window is automatically refreshed to show the updated tool assignments.',

    # ---- Restore Default Tools ----
    'rdt_lb_sessions':
        'Shows all backed-up default tool replacement sessions. Click a session to see the tool changes it recorded.',
    'rdt_lb_changes':
        'Shows the tool changes in the selected session: the original tool, the replacement, and how many icons were affected.',
    'rdt_btn_restore':
        'Restores all icons in the selected session to their original default tools. The backup session is kept after restoring.',
    'rdt_btn_delete':
        'Permanently deletes the selected backup session and its files. A confirmation requester is shown first. This cannot be undone.',
    'rdt_btn_close':
        'Closes the Restore Default Tools window.',

    # ---- DefIcons Setup ----
    'dfs_lb_targets':
        'Check a type to create icons for files of that type during processing. Uncheck to skip it.',
    'dfs_btn_select_all':
        'Enables all file types for icon creation. All checkboxes in the tree are ticked.',
    'dfs_btn_show_default_tools':
        'Scans the DefIcons template icons in ENVARC:Sys/ and shows the assigned default tool next to each type name.',
    'dfs_btn_select_none':
        'Disables all file types. No icons will be created for any file type during icon creation.',
    'dfs_btn_change_default_tools':
        'Opens a file requester to assign a new default tool to the selected type. Changes are saved to disk immediately.',
    'dfs_btn_ok':
        'Accepts the current type selections and closes the window. Checkbox changes are applied to the working preferences.',
    'dfs_btn_cancel':
        'Closes the window without saving type selection changes. Note: default tool changes are already saved to disk.',

    # ---- DefIcons Creation Options ----
    'dco_mode_folder_icons':
        'Controls whether iTidy creates drawer icons for folders that do not already have them.',
    'dco_cb_skip_whdload_folders':
        'When enabled, files inside WHDLoad game folders are skipped during icon creation. The folder icon itself is still created.',
    'dco_cb_text_previews':
        "When enabled, iTidy renders a preview of a text file's contents onto the icon image.",
    'dco_btn_manage_templates':
        'Opens the Text Templates window to view and edit how different text file types are rendered as icon previews.',
    'dco_cb_picture_previews':
        'When enabled, iTidy creates thumbnail icons for recognised image files by generating a miniature version of the image.',
    'dco_cb_pic_ilbm':
        'Enable thumbnail creation for Amiga IFF ILBM and PBM images.',
    'dco_cb_pic_jpeg':
        'Enable thumbnail creation for JPEG images. Disabled by default because JPEG decoding is slow on 68k hardware.',
    'dco_cb_pic_png':
        'Enable thumbnail creation for PNG images. Supports transparency.',
    'dco_cb_pic_acbm':
        'Enable thumbnail creation for Amiga Continuous Bitmap images, a rare format used in some older demos and game assets.',
    'dco_cb_pic_gif':
        'Enable thumbnail creation for GIF images. Supports transparency.',
    'dco_cb_pic_other':
        'Enable thumbnail creation for other image formats supported by installed DataTypes and DefIcons.',
    'dco_cb_pic_bmp':
        'Enable thumbnail creation for Windows BMP images.',
    'dco_cb_replace_image_thumbnails':
        'When enabled, image thumbnail icons previously created by iTidy are deleted and regenerated. User-placed icons are not affected.',
    'dco_cb_replace_text_previews':
        'When enabled, text preview icons previously created by iTidy are deleted and regenerated. User-placed icons are not affected.',
    'dco_mode_preview_size':
        'Sets the canvas size used for thumbnail icons. Larger sizes show more detail but take up more screen space.',
    'dco_mode_thumbnail_frame':
        'Controls the border style drawn around thumbnail icons. The "Smart" modes skip the border on images with transparency.',
    'dco_cb_upscale_thumbnails':
        'When enabled, images smaller than the preview size are scaled up to fill the thumbnail area.',
    'dco_mode_max_colors':
        'Sets the maximum number of colours used in generated thumbnail icons. Higher counts look better but produce larger icon files.',
    'dco_mode_dither':
        'Selects the dithering method used when reducing colours. Disabled when "Max Colours" is set to 256 or Ultra.',
    'dco_mode_lowcolor_mapping':
        'Controls the colour palette used at very low colour counts (4 or 8). Only enabled when "Max Colours" is set to 4 or 8.',
    'dco_btn_creation_setup':
        'Opens the DefIcons Categories window to select which file types should receive icons during processing.',
    'dco_btn_exclude_paths':
        'Opens the Exclude Paths window to manage folders that should be skipped during icon creation.',
    'dco_btn_ok':
        'Accepts the current settings and closes the window. Changes are applied to the active session.',
    'dco_btn_cancel':
        'Discards all changes and closes the window. The original settings are preserved.',
}

# ============================================================
# LISTITEM CHANGES: Block IDENT -> {old_value: new_value}
# ============================================================
LISTITEM_CHANGES = {
    'dco_list_max_colors': {
        '4': '4 colours',
        '8': '8 colours',
        '16': '16 colours',
        '32': '32 colours',
        '64': '64 colours',
        '128': '128 colours',
        '256': '256 colours (full)',
        'Ultra (256 + Detail Boost)': 'Ultra (256 + detail boost)',
    },
}


def validate_hints():
    """Check all hints are within the 194-char limit."""
    errors = []
    for ident, text in HINT_CHANGES.items():
        if len(text) > MAX_HINT_LEN:
            errors.append(f"  {ident}: {len(text)} chars -> {text[:60]}...")
    if errors:
        print(f"ERROR: {len(errors)} hint(s) exceed {MAX_HINT_LEN}-char limit:")
        for e in errors:
            print(e)
        return False
    print(f"OK: All {len(HINT_CHANGES)} hints are within {MAX_HINT_LEN}-char limit.")
    return True


def process_reb_file():
    """Process the .reb file, applying hint and listitem changes."""

    # Read file as bytes, preserve LF endings
    with open(REB_FILE, 'rb') as f:
        raw = f.read()

    # Verify LF-only line endings
    if b'\r\n' in raw:
        print("WARNING: File has CRLF endings. Converting to LF.")
        raw = raw.replace(b'\r\n', b'\n')

    content = raw.decode('latin-1')
    lines = content.split('\n')

    # Remove empty trailing element from final newline
    if lines and lines[-1] == '':
        trailing_newline = True
        lines = lines[:-1]
    else:
        trailing_newline = False

    print(f"Input: {len(lines)} lines")

    # Track changes
    stats = {
        'hints_updated': 0,
        'hints_added': 0,
        'listitems_updated': 0,
    }
    processed_hints = set()
    processed_listitems = set()

    # State tracking
    current_ident = None
    in_payload = False

    output_lines = []
    i = 0

    while i < len(lines):
        line = lines[i]

        # Track block start (TYPE: in metadata section only, not menu payload)
        if line.startswith('TYPE: ') and not in_payload:
            current_ident = None
            in_payload = False

        # Track IDENT
        if not in_payload and line.startswith('IDENT: '):
            current_ident = line[7:].strip()

        # Track payload separator
        if line == '--':
            in_payload = True

        # Track block end
        if line == '-':
            in_payload = False

        # ----- HINT REPLACEMENT (existing hint lines) -----
        if (not in_payload
                and line.startswith('HINT: ')
                and current_ident in HINT_CHANGES
                and current_ident not in processed_hints):
            # Consume ALL existing HINT lines for this block
            while i < len(lines) and lines[i].startswith('HINT: '):
                i += 1
            # Emit new hint
            new_hint = HINT_CHANGES[current_ident]
            output_lines.append(f'HINT: {new_hint}')
            processed_hints.add(current_ident)
            stats['hints_updated'] += 1
            # Don't increment i again, continue from the next non-HINT line
            continue

        # ----- HINT INSERTION (at MINWIDTH line, for blocks with no existing hint) -----
        if (not in_payload
                and line.startswith('MINWIDTH:')
                and current_ident in HINT_CHANGES
                and current_ident not in processed_hints):
            # Insert new hint BEFORE the MINWIDTH line
            new_hint = HINT_CHANGES[current_ident]
            output_lines.append(f'HINT: {new_hint}')
            processed_hints.add(current_ident)
            stats['hints_added'] += 1
            # Fall through to also append the MINWIDTH line

        # ----- LISTITEM CHANGES (in payload section) -----
        if (in_payload
                and line.startswith('LISTITEM: ')
                and current_ident in LISTITEM_CHANGES):
            old_value = line[10:]  # Text after "LISTITEM: "
            if old_value in LISTITEM_CHANGES[current_ident]:
                new_value = LISTITEM_CHANGES[current_ident][old_value]
                output_lines.append(f'LISTITEM: {new_value}')
                stats['listitems_updated'] += 1
                i += 1
                continue

        # Default: pass through unchanged
        output_lines.append(line)
        i += 1

    # Reconstruct file content
    output = '\n'.join(output_lines)
    if trailing_newline:
        output += '\n'

    # Ensure LF-only
    output = output.replace('\r\n', '\n')

    # Write output
    with open(REB_FILE, 'wb') as f:
        f.write(output.encode('latin-1'))

    print(f"Output: {len(output_lines)} lines")
    print(f"Hints updated:    {stats['hints_updated']}")
    print(f"Hints added:      {stats['hints_added']}")
    print(f"ListItems updated: {stats['listitems_updated']}")
    print(f"Total changes:    {stats['hints_updated'] + stats['hints_added'] + stats['listitems_updated']}")

    # Check for unprocessed hints
    missed = set(HINT_CHANGES.keys()) - processed_hints
    if missed:
        print(f"\nWARNING: {len(missed)} hint(s) were NOT applied (IDENT not found in .reb):")
        for m in sorted(missed):
            print(f"  - {m}")
    else:
        print(f"\nAll {len(HINT_CHANGES)} hint changes applied successfully.")

    # Check for unprocessed listitems
    for block_ident, changes in LISTITEM_CHANGES.items():
        for old_val in changes:
            if old_val not in []: pass  # Tracked via stats

    return stats


def verify_output():
    """Run basic integrity checks on the output file."""
    with open(REB_FILE, 'rb') as f:
        raw = f.read()

    content = raw.decode('latin-1')
    lines = content.split('\n')

    # Check no CRLF
    if b'\r\n' in raw:
        print("FAIL: Output has CRLF line endings!")
        return False

    # Count structural elements
    type_lines = sum(1 for l in lines if l.startswith('TYPE: '))
    dash_lines = sum(1 for l in lines if l == '-')
    ddash_lines = sum(1 for l in lines if l == '--')
    hint_lines = sum(1 for l in lines if l.startswith('HINT: '))

    print(f"\nVerification:")
    print(f"  TYPE: lines:  {type_lines}")
    print(f"  -- lines:     {ddash_lines}")
    print(f"  - lines:      {dash_lines}")
    print(f"  HINT: lines:  {hint_lines}")
    print(f"  Total lines:  {len(lines)}")

    # Check hint lengths
    long_hints = []
    for l in lines:
        if l.startswith('HINT: '):
            text = l[6:]
            if len(text) > MAX_HINT_LEN:
                long_hints.append((len(text), text[:60]))
    if long_hints:
        print(f"  FAIL: {len(long_hints)} hint(s) exceed {MAX_HINT_LEN} chars!")
        for length, preview in long_hints:
            print(f"    {length} chars: {preview}...")
        return False

    # Check starts with -REBUILD-
    if not content.startswith('-REBUILD-'):
        print("  FAIL: File does not start with -REBUILD-")
        return False

    print("  All checks passed.")
    return True


if __name__ == '__main__':
    print("=" * 60)
    print("iTidy35.reb Hint & ListItem Updater")
    print("=" * 60)

    # Validate hints before starting
    if not validate_hints():
        sys.exit(1)

    # Verify backup exists
    if not os.path.exists(BACKUP_FILE):
        print(f"\nERROR: No backup at {BACKUP_FILE}. Create one first.")
        sys.exit(1)
    print(f"Backup verified: {BACKUP_FILE}")

    # Process the file
    print(f"\nProcessing {REB_FILE}...")
    process_reb_file()

    # Verify output
    verify_output()

    print("\nDone!")
