/*
 * deficons_creation.h - DefIcons Icon Creation Engine
 *
 * Extracted from layout_processor.c to reduce module size.
 * Implements the icon creation pre-pass that runs BEFORE the
 * tidying/layout pass: scanning directories, identifying files
 * via DefIcons ARexx, resolving templates, and copying .info files.
 *
 * Includes Smart Folder Mode logic: deferred drawer icon creation
 * based on whether a subdirectory has visible contents (existing
 * .info files or files that will receive icons).
 *
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#ifndef DEFICONS_CREATION_H
#define DEFICONS_CREATION_H

#include <exec/types.h>
#include "layout_preferences.h"

/* Forward declarations */
struct iTidyMainProgressWindow;

/* Need full BackupContext definition (it's a typedef, not struct tag) */
#include "../backups/backup_types.h"

/*========================================================================*/
/* Result Structure                                                      */
/*========================================================================*/

/**
 * @brief Result of an icon creation pass for a single directory or tree
 *
 * Returned by both single-directory and recursive creation functions
 * so callers (and parent directories in Smart mode) can make decisions
 * based on what was found/created.
 */
typedef struct {
    ULONG icons_created;       /* Number of .info files actually created */
    BOOL  has_visible_contents; /* TRUE if folder has/will have visible icons */
} iTidy_DefIconsCreationResult;

/*========================================================================*/
/* Icon Creation Statistics (Phase 3 tracking)                           */
/*========================================================================*/

#define DEFICONS_MAX_CATEGORIES 15

typedef struct {
    char category_name[64];
    ULONG count;
} iTidy_IconCreationStats;

/**
 * @brief Initialize icon creation statistics for a new processing session
 */
void deficons_creation_init_stats(void);

/**
 * @brief Increment icon creation count for a category
 *
 * @param category Root category name (e.g., "picture", "music")
 */
void deficons_creation_increment_category(const char *category);

/**
 * @brief Get icon creation count for a specific category
 *
 * @param category Category name to look up
 * @return Count for that category, or 0 if not found
 */
ULONG deficons_creation_get_category_count(const char *category);

/**
 * @brief Display icon creation statistics to progress window
 */
void deficons_creation_display_stats(void);

/**
 * @brief Get total icons created across all categories
 */
ULONG deficons_creation_get_total(void);

/*========================================================================*/
/* Icon Creation Log File                                                */
/*========================================================================*/

/**
 * @brief Open dedicated log file for tracking created icons
 *
 * Creates PROGDIR:logs/IconsCreated/ folder if needed and opens log file.
 * Log file format: One .info path per line for easy script generation.
 *
 * @return TRUE if successful, FALSE on error
 */
BOOL deficons_creation_open_log(void);

/**
 * @brief Log a created icon path to dedicated log file
 *
 * @param info_path Full path to created .info file
 */
void deficons_creation_log_icon(const char *info_path);

/**
 * @brief Close icon creation log file
 */
void deficons_creation_close_log(void);

/*========================================================================*/
/* Icon Creation Functions                                               */
/*========================================================================*/

/**
 * @brief Create missing icons in a single directory (non-recursive)
 *
 * Scans directory entries and creates .info files for iconless files
 * using DefIcons type identification and template resolution.
 *
 * In Smart folder mode (deficons_folder_icon_mode == 0), drawer icon
 * creation is deferred: each subdirectory found during the scan gets
 * a lightweight pre-scan via deficons_folder_has_visible_contents()
 * to determine if it warrants a drawer .info.
 *
 * @param path            Directory path to process
 * @param prefs           Layout preferences (contains DefIcons settings)
 * @param result          Receives creation result (icons created + visibility)
 * @param progress_window Progress window for heartbeat updates (may be NULL)
 * @param backup_context  Backup context for manifest logging (may be NULL)
 * @return TRUE if scan completed (even if no icons created), FALSE on fatal error
 */
BOOL deficons_create_missing_icons_in_directory(
    const char *path,
    const LayoutPreferences *prefs,
    iTidy_DefIconsCreationResult *result,
    struct iTidyMainProgressWindow *progress_window,
    BackupContext *backup_context);

/**
 * @brief Create missing icons recursively through directory tree
 *
 * Processes the current directory then recurses into subdirectories.
 * In Smart folder mode, the recursion result from each child is used
 * by the parent to decide whether to create a drawer .info for it.
 *
 * @param path                 Root directory path
 * @param prefs                Layout preferences
 * @param recursion_level      Current depth (for logging and cycle detection)
 * @param result               Receives creation result for entire subtree
 * @param progress_window      Progress window for heartbeat/cancel (may be NULL)
 * @param backup_context       Backup context for manifest logging (may be NULL)
 * @return TRUE if successful, FALSE on fatal error or cancellation
 */
BOOL deficons_create_missing_icons_recursive(
    const char *path,
    const LayoutPreferences *prefs,
    int recursion_level,
    iTidy_DefIconsCreationResult *result,
    struct iTidyMainProgressWindow *progress_window,
    BackupContext *backup_context);

/**
 * @brief Lightweight pre-scan: does a subdirectory have visible contents?
 *
 * Checks whether a directory contains anything that would make it
 * "visible" in a Workbench icon layout. This is used by Smart folder
 * mode to decide whether to create a drawer .info for a subdirectory.
 *
 * The check is intentionally lightweight — no ARexx calls unless needed:
 * 1. First checks for existing .info files (early exit on first hit)
 * 2. If none found, checks if any file would get an icon via DefIcons
 *    ARexx identification (exits on first match)
 *
 * @param path             Full path to the subdirectory to check
 * @param prefs            Layout preferences (for type filtering, system path exclusion)
 * @param progress_window  Progress window for cancel checking (may be NULL)
 * @return TRUE if directory has or will have visible icon contents
 */
BOOL deficons_folder_has_visible_contents(const char *path,
                                          const LayoutPreferences *prefs,
                                          struct iTidyMainProgressWindow *progress_window);

#endif /* DEFICONS_CREATION_H */
