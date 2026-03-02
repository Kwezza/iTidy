# ReBuild File Changes: iTidy35.reb

**Date:** 2 March 2026
**File:** `Bin/Amiga/Rebuild/iTidy35.reb`
**Backup:** `Bin/Amiga/Rebuild/iTidy35.reb.bak`

This document records all changes made to the ReBuild `.reb` file to improve
gadget hint text (tooltips) and list item labels. Changes were cross-referenced
against the working notes in `docs/manual/working_notes/`.

---

## Summary

- **32 hints added** (gadgets that previously had no tooltip)
- **56 hints updated** (existing tooltips reworded for clarity)
- **9 list item labels updated** (chooser/cycle dropdown text improved)
- **97 total changes**

No structural changes were made (no IDs, PARENTIDs, types, or block ordering changed).

---

## List Item Changes

### dco_list_max_colors (`dco_list_max_colors`)

| # | Before | After |
|---|--------|-------|
| 1 | 4 | **4 colours** |
| 2 | 8 | **8 colours** |
| 3 | 16 | **16 colours** |
| 4 | 32 | **32 colours** |
| 5 | 64 | **64 colours** |
| 6 | 128 | **128 colours** |
| 7 | 256 | **256 colours (full)** |
| 8 | Ultra (256 + Detail Boost) | **Ultra (256 + detail boost)** |
| 9 | GlowIcons palette (29 colours) | GlowIcons palette (29 colours) |

---

## Hint Changes By Window

### (unparented)

#### Cancel (Button) [UPDATED]
*IDENT:* `adv_btn_cancel`

**Before:** Discards changes and returns to the main window.

**After:** Discards all changes and returns to the main window. The original settings are preserved unchanged.

#### Defaults (Button) [UPDATED]
*IDENT:* `adv_btn_defaults`

**Before:** Resets these settings to defaults.

**After:** Resets all settings in this window to their factory defaults. A confirmation requester is shown before the reset takes place.

#### OK (Button) [UPDATED]
*IDENT:* `adv_btn_ok`

**Before:** Accepts changes and closes this window. Use the main window's "Save" menu item to write settings to disk.

**After:** Accepts the current settings and closes the window. Changes are applied to the session but not saved to disk.

#### Column Layout (Checkbox) [UPDATED]
*IDENT:* `adv_cb_column_layout`

**Before:** When enabled, iTidy uses a column-based layout rather than free-flow positioning.

**After:** When enabled, icons are arranged in strict vertical columns. When disabled, a free-flow row-based layout is used.

#### Auto-Calc Max Icons (Checkbox) [UPDATED]
*IDENT:* `adv_cb_icons_per_row_autolayout`

**Before:** When enabled (default), iTidy automatically chooses the maximum columns based on the window width.

**After:** When enabled, the maximum number of columns is calculated automatically based on window width and screen size.

#### Auto-Fit Columns (Checkbox) [UPDATED]
*IDENT:* `adv_cb_optimize_columns`

**Before:** When enabled (default), each column is sized to its widest icon instead of forcing all columns to the same width.

**After:** When enabled, each column is sized to fit its widest icon, producing a more compact and visually balanced layout.

#### Reverse Sort Order (Checkbox) [UPDATED]
*IDENT:* `adv_cb_reverse_sort`

**Before:** Reverses the current sort direction (Z->A, newest-first, or largest-first depending on sort mode).

**After:** Reverses the sort direction. Applies to name (Z-A), date (newest first), and size (largest first) sorting.

#### Skip Hidden Folders (Checkbox) [UPDATED]
*IDENT:* `adv_cb_skip_hidden`

**Before:** When enabled (default), iTidy skips folders without .info files during recursive processing.

**After:** When enabled, folders without a .info file are skipped during recursive processing, as they are not visible in Workbench.

#### Strip Newicons Borders (Permanent) (Checkbox) [UPDATED]
*IDENT:* `adv_cb_strip_newicon_borders`

**Before:** Strips NewIcons borders during processing (requires icon.library v44+). This permanently modifies those icons.

**After:** Permanently strips the black border from NewIcons. Requires icon.library v44 or later. Enable backups before using.

#### Align Vertically (Chooser) [UPDATED]
*IDENT:* `adv_ch_layout_align_vertically`

**Before:** Sets how icons are aligned vertically when a row contains icons of different heights.

**After:** Sets how icons are aligned vertically within a row when icons of different heights appear in the same row.

#### Layout Aspect (Chooser) [UPDATED]
*IDENT:* `adv_ch_layout_aspect`

**Before:** Sets the target width-to-height shape for drawer windows.

**After:** Sets the target width-to-height proportions for drawer windows when iTidy resizes them.

#### Gap Between Groups (Chooser) [UPDATED]
*IDENT:* `adv_ch_layout_gap_between_groups`

**Before:** Controls spacing between icon groups when using the "Grouped by Type" order mode.

**After:** Controls the spacing between icon groups when "Grouping" on the main window is set to "Grouped By Type".

#### Max Window Width (Chooser) [UPDATED]
*IDENT:* `adv_ch_layout_max_width`

**Before:** Limits how wide drawer windows may become (percentage of screen width).

**After:** Limits how wide drawer windows may become, as a percentage of screen width. Prevents windows from filling the entire screen.

#### When Full (Chooser) [UPDATED]
*IDENT:* `adv_ch_layout_when_full`

**Before:** Choose what iTidy does when a drawer has more icons than fit comfortably on screen.

**After:** Controls how iTidy expands a drawer window when it has more icons than fit at the target proportions.

#### Max (Integer) [UPDATED]
*IDENT:* `adv_int_icons_per_row_max`

**Before:** Maximum columns (used only when "Auto-Calc Max Icons" is disabled).

**After:** Sets the maximum number of columns. Only used when "Auto-Calc Max Icons" is turned off.

#### Icons Per Row: Min (Integer) [UPDATED]
*IDENT:* `adv_int_icons_per_row_min`

**Before:** Minimum columns. Prevents drawers becoming one long vertical list.

**After:** Sets the minimum number of columns in a drawer window, preventing icons from being arranged in one long vertical list.

#### Horizontal Spacing (Integer) [UPDATED]
*IDENT:* `adv_int_iconspacing_x`

**Before:** Sets the horizontal gap between icons (in pixels).

**After:** Sets the horizontal gap between icons in pixels. Lower values pack icons tighter; higher values give more breathing room.

#### Vertical Spacing (Integer) [UPDATED]
*IDENT:* `adv_int_iconspacing_y`

**Before:** Sets the vertical gap between icons (in pixels).

**After:** Sets the vertical gap between icons in pixels. Lower values pack icons tighter; higher values give more breathing room.

#### Cancel (Button) [NEW]
*IDENT:* `dco_btn_cancel`

**Before:** *(no hint)*

**After:** Discards all changes and closes the window. The original settings are preserved.

#### Icon Creation Setup... (Button) [NEW]
*IDENT:* `dco_btn_creation_setup`

**Before:** *(no hint)*

**After:** Opens the DefIcons Categories window to select which file types should receive icons during processing.

#### Exclude Paths... (Button) [NEW]
*IDENT:* `dco_btn_exclude_paths`

**Before:** *(no hint)*

**After:** Opens the Exclude Paths window to manage folders that should be skipped during icon creation.

#### Manage Templates... (Button) [UPDATED]
*IDENT:* `dco_btn_manage_templates`

**Before:** Opens the Text Templates window to edit how text previews are rendered.

**After:** Opens the Text Templates window to view and edit how different text file types are rendered as icon previews.

#### OK (Button) [NEW]
*IDENT:* `dco_btn_ok`

**Before:** *(no hint)*

**After:** Accepts the current settings and closes the window. Changes are applied to the active session.

#### ACBM (Checkbox) [UPDATED]
*IDENT:* `dco_cb_pic_acbm`

**Before:** Creates thumbnails for ACBM (Amiga Continuous Bitmap) images. This is a rare Amiga bitmap format, often seen in older demos or game asset data.

**After:** Enable thumbnail creation for Amiga Continuous Bitmap images, a rare format used in some older demos and game assets.

#### BMP (Checkbox) [UPDATED]
*IDENT:* `dco_cb_pic_bmp`

**Before:** Creates thumbnails for BMP images.

**After:** Enable thumbnail creation for Windows BMP images.

#### GIF (Checkbox) [UPDATED]
*IDENT:* `dco_cb_pic_gif`

**Before:** Creates thumbnails for GIF images and supports transparency.

**After:** Enable thumbnail creation for GIF images. Supports transparency.

#### ILBM (IFF) (Checkbox) [UPDATED]
*IDENT:* `dco_cb_pic_ilbm`

**Before:** Creates thumbnails for Amiga IFF ILBM/PBM images.

**After:** Enable thumbnail creation for Amiga IFF ILBM and PBM images.

#### JPEG (Slow) (Checkbox) [UPDATED]
*IDENT:* `dco_cb_pic_jpeg`

**Before:** Creates thumbnails for JPEG images (slow to decode on 68k).

**After:** Enable thumbnail creation for JPEG images. Disabled by default because JPEG decoding is slow on 68k hardware.

#### Other (Checkbox) [UPDATED]
*IDENT:* `dco_cb_pic_other`

**Before:** Other image formats (not covered above) if supported by installed DataTypes and DefIcons.

**After:** Enable thumbnail creation for other image formats supported by installed DataTypes and DefIcons.

#### PNG (Checkbox) [UPDATED]
*IDENT:* `dco_cb_pic_png`

**Before:** Creates thumbnails for PNG images and supports transparency.

**After:** Enable thumbnail creation for PNG images. Supports transparency.

#### Picture File Previews (Checkbox) [UPDATED]
*IDENT:* `dco_cb_picture_previews`

**Before:** Creates thumbnail icons for recognised picture files (formats selected below).

**After:** When enabled, iTidy creates thumbnail icons for recognised image files by generating a miniature version of the image.

#### Replace Existing Image Thumbnails (Checkbox) [UPDATED]
*IDENT:* `dco_cb_replace_image_thumbnails`

**Before:** When enabled, iTidy will delete and recreate any image thumbnail icons it previously made (identified by the ITIDY_CREATED ToolType). User-placed icons are never affected.

**After:** When enabled, image thumbnail icons previously created by iTidy are deleted and regenerated. User-placed icons are not affected.

#### Replace Existing Text Previews (Checkbox) [UPDATED]
*IDENT:* `dco_cb_replace_text_previews`

**Before:** When enabled, iTidy will delete and recreate any text preview icons it previously made. Useful after changing rendering settings. User-placed icons are never affected.

**After:** When enabled, text preview icons previously created by iTidy are deleted and regenerated. User-placed icons are not affected.

#### Skip Icon Creation Inside WHDLoad Folders (Checkbox) [UPDATED]
*IDENT:* `dco_cb_skip_whdload_folders`

**Before:** Selecting this will prevent iTidy cluttering up an WHDLoad game folder with new icons.

**After:** When enabled, files inside WHDLoad game folders are skipped during icon creation. The folder icon itself is still created.

#### Text File Previews (Checkbox) [UPDATED]
*IDENT:* `dco_cb_text_previews`

**Before:** When enabled, iTidy can create thumbnail-style icons for text files by rendering the file contents onto the icon.

**After:** When enabled, iTidy renders a preview of a text file's contents onto the icon image.

#### Upscale Small Images To Icon Size (Checkbox) [UPDATED]
*IDENT:* `dco_cb_upscale_thumbnails`

**Before:** If enabled, small images are scaled up to fill the thumbnail area.

**After:** When enabled, images smaller than the preview size are scaled up to fill the thumbnail area.

#### Dithering: (Chooser) [UPDATED]
*IDENT:* `dco_mode_dither`

**Before:** Selects the dithering method used when reducing colours.

**After:** Selects the dithering method used when reducing colours. Disabled when "Max Colours" is set to 256 or Ultra.

#### Folder Icons: (Chooser) [UPDATED]
*IDENT:* `dco_mode_folder_icons`

**Before:** Creates missing drawer icons. "Smart" scans the subfolder to see whether it contains icons or content that needs icons. If it does, it creates a drawer icon.

**After:** Controls whether iTidy creates drawer icons for folders that do not already have them.

#### Low-Colour Palette: (Chooser) [UPDATED]
*IDENT:* `dco_mode_lowcolor_mapping`

**Before:** Palette mapping used at 4 or 8 colours (disabled above 8 colours or in Ultra).

**After:** Controls the colour palette used at very low colour counts (4 or 8). Only enabled when "Max Colours" is set to 4 or 8.

#### Max Colours: (Chooser) [UPDATED]
*IDENT:* `dco_mode_max_colors`

**Before:** Limits the number of colours used in thumbnails. Lower is faster; higher looks better.

**After:** Sets the maximum number of colours used in generated thumbnail icons. Higher counts look better but produce larger icon files.

#### Preview Size: (Chooser) [UPDATED]
*IDENT:* `dco_mode_preview_size`

**Before:** Sets the thumbnail canvas size used inside generated icons.

**After:** Sets the canvas size used for thumbnail icons. Larger sizes show more detail but take up more screen space.

#### Thumbnail Border: (Chooser) [UPDATED]
*IDENT:* `dco_mode_thumbnail_frame`

**Before:** Border style for thumbnail icons. Workbench: classic WB frame around the icon. Bevel: inner highlight drawn onto the image pixels (top-left bright, bottom-right dark). Smart modes skip the effe

**After:** Controls the border style drawn around thumbnail icons. The "Smart" modes skip the border on images with transparency.

#### Cancel (Button) [UPDATED]
*IDENT:* `dfs_btn_cancel`

**Before:** Closes the window without saving changes. Note: Default Tool changes are applied immediately and saved automatically.

**After:** Closes the window without saving type selection changes. Note: default tool changes are already saved to disk.

#### Change Default Tool... (Button) [UPDATED]
*IDENT:* `dfs_btn_change_default_tools`

**Before:** Change the Default Tool for the selected icon. If needed, a default icon is created automatically. Changes apply immediately.

**After:** Opens a file requester to assign a new default tool to the selected type. Changes are saved to disk immediately.

#### OK (Button) [UPDATED]
*IDENT:* `dfs_btn_ok`

**Before:** Saves changes.

**After:** Accepts the current type selections and closes the window. Checkbox changes are applied to the working preferences.

#### Select All (Button) [UPDATED]
*IDENT:* `dfs_btn_select_all`

**Before:** Select all file types in the list above. Note: this may create icons for any files that do not already have an icon, including system files. Use with care.

**After:** Enables all file types for icon creation. All checkboxes in the tree are ticked.

#### Select None (Button) [UPDATED]
*IDENT:* `dfs_btn_select_none`

**Before:** Deselects all selected file types.

**After:** Disables all file types. No icons will be created for any file type during icon creation.

#### Show Default Tools (Button) [UPDATED]
*IDENT:* `dfs_btn_show_default_tools`

**Before:** Scans the default icons and displays the Default Tool for each entry in the list above.

**After:** Scans the DefIcons template icons in ENVARC:Sys/ and shows the assigned default tool next to each type name.

#### ListBrowser_269 (ListBrowser) [UPDATED]
*IDENT:* `dfs_lb_targets`

**Before:** Select the file types to create icons for. This list comes from the "DefaultIcons" Preferences tool in the Workbench Prefs drawer.

**After:** Check a type to create icons for files of that type during processing. Uncheck to skip it.

#### Close (Button) [NEW]
*IDENT:* `dta_btn_close`

**Before:** *(no hint)*

**After:** Closes the Default Tool Analysis window.

#### Replace Tool (Batch)... (Button) [NEW]
*IDENT:* `dta_btn_replace_batch`

**Before:** *(no hint)*

**After:** Opens the Replace Default Tool window to update all icons that reference the selected tool at once.

#### Replace Tool (Single)... (Button) [NEW]
*IDENT:* `dta_btn_replace_single`

**Before:** *(no hint)*

**After:** Opens the Replace Default Tool window to update the default tool in one specific icon file selected in the details panel.

#### Restore Default Tools Backup... (Button) [NEW]
*IDENT:* `dta_btn_restore_backup`

**Before:** *(no hint)*

**After:** Opens the Restore Default Tools window to revert previous default tool replacements from iTidy's backup records.

#### Scan (Button) [NEW]
*IDENT:* `dta_btn_scan`

**Before:** *(no hint)*

**After:** Starts a recursive scan of the selected folder to find all default tools referenced by icons. Opens a progress window.

#### Folder: (GetFile) [NEW]
*IDENT:* `dta_ch_folder`

**Before:** *(no hint)*

**After:** Select the root folder to scan for default tool information. Drawers only.

#### ListBrowser_142 (ListBrowser) [NEW]
*IDENT:* `dta_lb_tools`

**Before:** *(no hint)*

**After:** Shows all discovered default tools, how many icons reference each one, and whether the tool exists on disk. Click a row for details.

#### ListView_143 (ListView) [NEW]
*IDENT:* `dta_lv_status`

**Before:** *(no hint)*

**After:** Shows details of the selected tool: name, status, version string, and the list of icon files that reference it.

#### dta_str_filter (Chooser) [NEW]
*IDENT:* `dta_str_filter`

**Before:** *(no hint)*

**After:** Filters the tool list to show all tools, only valid tools, or only missing tools. Disabled until tool data is loaded.

#### Close (Button) [NEW]
*IDENT:* `dtu_btn_close`

**Before:** *(no hint)*

**After:** Closes this window. The Default Tool Analysis window is automatically refreshed to show the updated tool assignments.

#### Update Default Tool (Button) [NEW]
*IDENT:* `dtu_btn_update`

**Before:** *(no hint)*

**After:** Starts replacing the default tool in the selected icon(s). The button is disabled after one use per window opening.

#### dtu_ch_new_tool (GetFile) [NEW]
*IDENT:* `dtu_ch_new_tool`

**Before:** *(no hint)*

**After:** Select the new default tool program. Leave empty to remove the tool association from the selected icon(s).

#### Advanced... (Button) [UPDATED]
*IDENT:* `main_btn_advanced`

**Before:** Opens Advanced Settings for finer control over layout and sizing.

**After:** Opens the Advanced Settings window for finer control over layout, density, limits, columns, grouping gaps, and filtering.

#### Fix Default Tools... (Button) [UPDATED]
*IDENT:* `main_btn_default_tools`

**Before:** Scans icons for missing or invalid Default Tools. Lets you fix them or batch-replace one tool with another.

**After:** Scans icons for missing or invalid default tools. Lets you review and fix broken tool paths in batch or one at a time.

#### Icon Creation... (Button) [NEW]
*IDENT:* `main_btn_icon_settings`

**Before:** *(no hint)*

**After:** Opens the Icon Creation Settings window to configure thumbnail generation, text previews, folder icons, and DefIcons categories.

#### Restore Backups... (Button) [UPDATED]
*IDENT:* `main_btn_restore_backups`

**Before:** Restores icon positions and window snapshots from iTidy backups. Only available if you previously enabled backups.

**After:** Opens the Restore Backups window to restore icon positions and window layouts from a previous iTidy backup run.

#### Start (Button) [UPDATED]
*IDENT:* `main_btn_start`

**Before:** Starts tidying the selected folder using the current settings.

**After:** Starts tidying the selected folder using the current settings. A progress window shows status during processing.

#### Back Up Layout Before Changes (Checkbox) [UPDATED]
*IDENT:* `main_cb_backup_icons`

**Before:** Creates an LhA backup of the folder's .info files before changes. Requires LhA in C:

**After:** When enabled, an LhA backup of each folder's .info files is created before making changes. Requires LhA in C:.

#### Include Subfolders (Checkbox) [UPDATED]
*IDENT:* `main_cb_cleanup_subfolders`

**Before:** When enabled, iTidy also processes all subfolders under the selected folder.

**After:** When enabled, all subfolders under the selected folder are also processed. Use with care on large directory trees.

#### Create Icons During Tidy (Checkbox) [NEW]
*IDENT:* `main_cb_create_new_icons`

**Before:** *(no hint)*

**After:** When enabled, iTidy creates new icons for files that do not already have .info files, using the DefIcons system.

#### Window Position (Chooser) [UPDATED]
*IDENT:* `main_ch_positioning`

**Before:** Controls where drawer windows are placed after resizing. (Only affects windows iTidy changes.)

**After:** Controls where drawer windows are placed after iTidy resizes them. Only affects windows that iTidy actually changes.

#### Grouping (Chooser) [UPDATED]
*IDENT:* `main_ch_sort_primary`

**Before:** Sets how icons are grouped before sorting. Folders first, files first, mixed, or grouped by type.

**After:** Sets how icons are grouped before sorting. Choose folders first, files first, mixed, or grouped by file type.

#### Sort By (Chooser) [UPDATED]
*IDENT:* `main_ch_sort_secondary`

**Before:** Selects the field used for sorting: name, kind, date, or size.

**After:** Sets the sort order within the current grouping mode. Disabled when "Grouping" is set to "Grouped By Type".

#### Folder to clean: (GetFile) [UPDATED]
*IDENT:* `main_gf_target_path`

**Before:** Select the folder you want to tidy. To include subfolders, enable "Include Subfolders" below.

**After:** Click to open a directory requester and choose the drawer iTidy will process.

#### Cancel (Button) [UPDATED]
*IDENT:* `mp_btn_cancel`

**Before:** Press this to cancel the current iTidy run. Note: Any changes made to the system will not be undone.

**After:** Cancels the current operation after confirmation, or closes the window when done. Changes already made are not reverted.

#### ListBrowser_56 (ListBrowser) [NEW]
*IDENT:* `mp_lb_status`

**Before:** *(no hint)*

**After:** Shows a scrollable history of processing status messages. Auto-scrolls to keep the newest entry visible.

#### Cancel (Button) [NEW]
*IDENT:* `rb_btn_close`

**Before:** *(no hint)*

**After:** Closes the Restore Backups window.

#### Delete Run (Button) [NEW]
*IDENT:* `rb_btn_delete`

**Before:** *(no hint)*

**After:** Permanently deletes the selected backup run and all its files. A confirmation requester is shown first. This cannot be undone.

#### Restore Run (Button) [NEW]
*IDENT:* `rb_btn_restore`

**Before:** *(no hint)*

**After:** Restores icon positions and window layouts from the selected backup run. Choose with or without window positions. Requires LhA.

#### View Folders... (Button) [NEW]
*IDENT:* `rb_btn_view_folders`

**Before:** *(no hint)*

**After:** Opens a tree view showing the hierarchical folder structure of the selected run. Only available for runs with a catalog file.

#### ListBrowser_107 (ListBrowser) [NEW]
*IDENT:* `rb_lb_backups`

**Before:** *(no hint)*

**After:** Shows all iTidy backup runs. Click a run to view its details below. Double-click to view the folder structure.

#### ListBrowser_117 (ListBrowser) [NEW]
*IDENT:* `rb_lb_details`

**Before:** *(no hint)*

**After:** Shows details of the selected backup run: date created, source folder, number of archives, total size, and status.

#### Close (Button) [NEW]
*IDENT:* `rdt_btn_close`

**Before:** *(no hint)*

**After:** Closes the Restore Default Tools window.

#### Delete (Button) [NEW]
*IDENT:* `rdt_btn_delete`

**Before:** *(no hint)*

**After:** Permanently deletes the selected backup session and its files. A confirmation requester is shown first. This cannot be undone.

#### Restore (Button) [NEW]
*IDENT:* `rdt_btn_restore`

**Before:** *(no hint)*

**After:** Restores all icons in the selected session to their original default tools. The backup session is kept after restoring.

#### ListBrowser_193 (ListBrowser) [NEW]
*IDENT:* `rdt_lb_changes`

**Before:** *(no hint)*

**After:** Shows the tool changes in the selected session: the original tool, the replacement, and how many icons were affected.

#### ListBrowser_185 (ListBrowser) [NEW]
*IDENT:* `rdt_lb_sessions`

**Before:** *(no hint)*

**After:** Shows all backed-up default tool replacement sessions. Click a session to see the tool changes it recorded.

#### Close (Button) [NEW]
*IDENT:* `rfv_btn_close`

**Before:** *(no hint)*

**After:** Closes the folder view and returns to the Restore Backups window.

#### ListBrowser_121 (ListBrowser) [NEW]
*IDENT:* `rfv_lb_folders`

**Before:** *(no hint)*

**After:** Shows the hierarchical folder structure of this backup run. Click the disclosure triangles to expand or collapse branches.

---
