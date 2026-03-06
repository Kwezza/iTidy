/*
 * deficons_filters.h - DefIcons Icon Creation Filtering Logic
 * 
 * Provides filtering logic for DefIcons icon creation based on user preferences:
 * - Category/type filtering (disabled types list)
 * - System path exclusion (SYS:, C:, etc.)
 * - Folder icon creation modes (Smart/Always/Never)
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#ifndef DEFICONS_FILTERS_H
#define DEFICONS_FILTERS_H

#include <exec/types.h>
#include "layout_preferences.h"

/*========================================================================*/
/* Folder Icon Mode Constants                                            */
/*========================================================================*/

/* Folder icon creation modes (matches LayoutPreferences.deficons_folder_icon_mode) */
#define DEFICONS_FOLDER_MODE_SMART  0  /* Create if folder has visible contents */
#define DEFICONS_FOLDER_MODE_ALWAYS 1  /* Always create folder icons */
#define DEFICONS_FOLDER_MODE_NEVER  2  /* Never create folder icons */

/*========================================================================*/
/* Public Functions                                                      */
/*========================================================================*/

/**
 * @brief Check if icon creation is enabled for a type
 * 
 * Applies user preferences to determine if an icon should be created
 * for a given file type. Checks:
 * - Master enable flag (enable_deficons_icon_creation)
 * - Disabled types list (deficons_disabled_types CSV)
 * - Resolved category filtering
 * 
 * @param type_token Type token from DefIcons (e.g., "mod", "ascii")
 * @param prefs Layout preferences containing DefIcons settings
 * 
 * @return TRUE if icon should be created, FALSE if filtered out
 * 
 * @note Uses deficons_get_resolved_category() to determine root category
 * @note Respects master enable flag as primary filter
 */
BOOL deficons_should_create_icon(const char *type_token, const LayoutPreferences *prefs);

/**
 * @brief Check if path is a system directory
 * 
 * Tests whether a path starts with a system assign that should be
 * excluded from icon creation based on user preferences.
 * 
 * System paths checked:
 * - SYS: (System partition)
 * - C: (Commands)
 * - S: (Startup scripts)
 * - DEVS: (Device drivers)
 * - LIBS: (Shared libraries)
 * - L: (Handlers and file systems)
 * - FONTS: (System fonts)
 * 
 * @param path Full path to check
 * @param prefs Layout preferences containing deficons_skip_system_assigns flag
 * 
 * @return TRUE if path is a system directory and exclusion is enabled,
 *         FALSE otherwise
 * 
 * @note Case-insensitive comparison
 * @note Returns FALSE if deficons_skip_system_assigns is disabled
 */
BOOL deficons_is_system_path(const char *path, const LayoutPreferences *prefs);

/**
 * @brief Check if path matches user-defined exclude list
 * 
 * Tests whether a path should be excluded based on the user-configurable
 * exclude paths list. Supports DEVICE: placeholder substitution.
 * 
 * DEVICE: Placeholder:
 * - "DEVICE:Fonts" pattern matches "SYS:Fonts" when scanning SYS:
 * - "DEVICE:Fonts" pattern matches "Work:Fonts" when scanning Work:
 * - Device is extracted from folder_path in preferences
 * 
 * @param path Full path to check
 * @param prefs Layout preferences containing exclude paths array and folder_path
 * 
 * @return TRUE if path matches any exclude pattern, FALSE otherwise
 * 
 * @note Case-insensitive prefix matching
 * @note DEVICE: is substituted with volume from folder_path at runtime
 */
BOOL deficons_is_excluded_path(const char *path, const LayoutPreferences *prefs);

/**
 * @brief Check if a folder icon should be created
 * 
 * Applies folder icon creation mode logic to determine if a drawer
 * icon should be created.
 * 
 * Mode Logic:
 * - NEVER (2): Never create folder icons
 * - ALWAYS (1): Always create folder icons
 * - SMART (0): Create if has_visible_contents is TRUE
 * 
 * "Visible contents" means:
 * - Folder already contains any .info file, OR
 * - Folder contains files that will get icons created, OR
 * - Folder contains subdrawers with icons
 * 
 * @param path Full path to folder
 * @param has_visible_contents TRUE if folder participates in icon layout
 * @param prefs Layout preferences containing deficons_folder_icon_mode
 * 
 * @return TRUE if folder icon should be created, FALSE otherwise
 * 
 * @note Caller is responsible for determining has_visible_contents
 * @note has_visible_contents is ignored for ALWAYS and NEVER modes
 */
BOOL deficons_should_create_folder_icon(const char *path, BOOL has_visible_contents, 
                                        const LayoutPreferences *prefs);

/**
 * @brief Check if any .info files exist in a directory
 * 
 * Helper function to scan a directory for .info files.
 * Used to implement Smart folder mode logic.
 * 
 * @param path Full path to directory
 * @return TRUE if at least one .info file found, FALSE otherwise
 * 
 * @note Does not recurse into subdirectories
 * @note Returns FALSE on error (directory not accessible)
 */
BOOL deficons_folder_has_info_files(const char *path);

/**
 * @brief Get folder icon mode as string (for logging)
 * 
 * @param mode Folder icon mode value (0-2)
 * @return Human-readable mode name ("Smart", "Always", "Never", "Unknown")
 * 
 * @note For logging and debugging only
 */
const char* deficons_folder_mode_to_string(UWORD mode);

/**
 * @brief Check if a folder is a WHDLoad game/application folder
 *
 * Performs a fast pattern-match glob for "#?.slave" in the given directory.
 * Uses MatchFirst with early exit — cost is a single filesystem call when
 * no .slave file exists, so impact on normal folders is negligible.
 *
 * WHDLoad folders identified here should:
 *  - Still receive a drawer .info from their parent (so they appear in
 *    Workbench) — the parent's Smart check returns TRUE.
 *  - NOT have any icons created for their internal files (data, saves, etc.).
 *  - NOT be recursed into by the DefIcons creation pass.
 *
 * @param path Full path to the folder to test
 * @param prefs Layout preferences (checked for deficons_skip_whdload_folders flag)
 * @return TRUE if a *.slave file is found in the folder, FALSE otherwise
 *
 * @note prefs may be NULL — treated as FALSE (no detection)
 * @note Caller should check prefs->deficons_skip_whdload_folders before calling
 */
BOOL deficons_is_whdload_folder(const char *path, const LayoutPreferences *prefs);

#endif /* DEFICONS_FILTERS_H */
