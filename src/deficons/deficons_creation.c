/*
 * deficons_creation.c - DefIcons Icon Creation Engine
 *
 * Extracted from layout_processor.c to reduce module size.
 * Implements the icon creation pre-pass that runs BEFORE the
 * tidying/layout pass.
 *
 * Key feature: Smart Folder Mode — deferred drawer icon creation
 * based on whether a subdirectory has visible contents (existing
 * .info files or files that will receive icons via DefIcons).
 *
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stdio.h>
#include <string.h>

#include <console_output.h>

#include "deficons_creation.h"
#include "deficons_identify.h"
#include "deficons_templates.h"
#include "deficons_filters.h"
#include "../layout_preferences.h"
#include "../writeLog.h"
#include "../utilities.h"
#include "../backups/backup_session.h"
#include "../path_utilities.h"

/* Content-aware icon preview (Phase 3 integration) */
#include "../icon_edit/icon_content_preview.h"
#include "../icon_edit/icon_image_access.h"

/* Progress window integration */
#include "../GUI/StatusWindows/main_progress_window.h"

/*========================================================================*/
/* Helper Macros (local to this module)                                  */
/*========================================================================*/

/* Triple output: console + progress window + log */
#define CREATION_STATUS(pw, ...) \
    do { \
        char _buf[256]; \
        snprintf(_buf, sizeof(_buf), __VA_ARGS__); \
        CONSOLE_STATUS("%s\n", _buf); \
        log_info(LOG_GENERAL, "%s\n", _buf); \
        if ((pw)) { \
            itidy_main_progress_window_append_status((pw), _buf); \
            itidy_main_progress_window_handle_events((pw)); \
        } \
    } while (0)

/* Check for user cancellation via progress window.
 * CRITICAL: Must pump events first, otherwise cancel_requested is never set
 * because the button click sits unprocessed in the message queue. */
#define CREATION_CHECK_CANCEL(pw) \
    do { \
        if ((pw)) { \
            itidy_main_progress_window_handle_events((pw)); \
            if ((pw)->cancel_requested) { \
                CONSOLE_STATUS("\n*** User cancelled operation ***\n"); \
                return FALSE; \
            } \
        } \
    } while (0)

/*========================================================================*/
/* Icon Creation Statistics (Phase 3 tracking)                           */
/*========================================================================*/

static iTidy_IconCreationStats g_creation_stats[DEFICONS_MAX_CATEGORIES];
static int g_creation_stat_count = 0;
static ULONG g_creation_total = 0;

void deficons_creation_init_stats(void)
{
    int i;
    g_creation_stat_count = 0;
    g_creation_total = 0;

    for (i = 0; i < DEFICONS_MAX_CATEGORIES; i++)
    {
        g_creation_stats[i].category_name[0] = '\0';
        g_creation_stats[i].count = 0;
    }
}

void deficons_creation_increment_category(const char *category)
{
    int i;

    if (!category || category[0] == '\0')
        return;

    /* Search for existing category */
    for (i = 0; i < g_creation_stat_count; i++)
    {
        if (strcmp(g_creation_stats[i].category_name, category) == 0)
        {
            g_creation_stats[i].count++;
            g_creation_total++;
            return;
        }
    }

    /* Add new category if space available */
    if (g_creation_stat_count < DEFICONS_MAX_CATEGORIES)
    {
        strncpy(g_creation_stats[g_creation_stat_count].category_name,
                category,
                sizeof(g_creation_stats[0].category_name) - 1);
        g_creation_stats[g_creation_stat_count].category_name[
            sizeof(g_creation_stats[0].category_name) - 1] = '\0';
        g_creation_stats[g_creation_stat_count].count = 1;
        g_creation_stat_count++;
        g_creation_total++;
    }
    else
    {
        log_warning(LOG_ICONS, "Category limit reached (%d), cannot track: %s\n",
                    DEFICONS_MAX_CATEGORIES, category);
        g_creation_total++;
    }
}

ULONG deficons_creation_get_category_count(const char *category)
{
    int i;

    if (!category || category[0] == '\0')
        return 0;

    for (i = 0; i < g_creation_stat_count; i++)
    {
        if (strcmp(g_creation_stats[i].category_name, category) == 0)
        {
            return g_creation_stats[i].count;
        }
    }

    return 0;
}

ULONG deficons_creation_get_total(void)
{
    return g_creation_total;
}

void deficons_creation_display_stats(void)
{
    /* This is a no-op if nothing was created.
     * Caller (layout_processor.c) uses the progress window macro
     * to display — we provide the data via the getters above.
     * Kept for direct usage if a progress window pointer is not handy. */
    if (g_creation_total == 0)
        return;

    log_info(LOG_ICONS, "Icons created: %lu\n", g_creation_total);
    {
        int i;
        for (i = 0; i < g_creation_stat_count; i++)
        {
            log_info(LOG_ICONS, "  %s: %lu\n",
                     g_creation_stats[i].category_name,
                     g_creation_stats[i].count);
        }
    }
}

/*========================================================================
/* Helper: Proper .info suffix check                                     */
/*========================================================================*/

/**
 * @brief Check if a filename ends with ".info" (case-insensitive)
 *
 * Tighter than strstr() — only matches the suffix, not mid-filename.
 *
 * @param filename The filename to check (not a full path)
 * @return TRUE if filename ends with ".info"
 */
static BOOL filename_has_info_suffix(const char *filename)
{
    size_t len;

    if (!filename)
        return FALSE;

    len = strlen(filename);
    if (len < 5)  /* minimum: ".info" */
        return FALSE;

    /* Compare last 5 characters case-insensitively */
    return (strncasecmp_custom(filename + len - 5, ".info", 5) == 0);
}

/**
 * Check if a filename represents a system file that should NEVER receive
 * an icon, regardless of user preferences.
 *
 * These are universal Workbench system files that are critical for proper
 * operation and must not be treated as regular files.
 *
 * @param filename The base filename to check (not full path)
 * @return TRUE if this is a system file that should be skipped
 */
static BOOL is_system_file_never_icon(const char *filename)
{
    if (!filename || filename[0] == '\0')
        return FALSE;

    /* .info files are icon metadata themselves - never create icons for them */
    if (filename_has_info_suffix(filename))
        return TRUE;

    /* Files named exactly ".info" (edge case - no base name) */
    if (strcmp(filename, ".info") == 0)
        return TRUE;

    /* .backdrop files track left-out icons on Workbench - critical system files */
    if (strcmp(filename, ".backdrop") == 0)
        return TRUE;

    /* Add other universal system files here as needed:
     * - .disk files (optional - disk.info type files)
     * - .newicons files (NewIcons system files)
     */

    return FALSE;
}

/*========================================================================*/
/* Progress Callback Wrapper                                             */
/*========================================================================*/

/**
 * progress_callback_wrapper - Adapts iTidyMainProgressWindow heartbeat
 *                              function to the generic progress callback interface.
 *
 * This wrapper is used when passing progress updates from content preview
 * operations (e.g., IFF rendering) to the main progress window.
 */
static void progress_callback_wrapper(void *user_data,
                                      const char *phase,
                                      ULONG current,
                                      ULONG total)
{
    struct iTidyMainProgressWindow *progress_win = (struct iTidyMainProgressWindow *)user_data;
    
    if (progress_win != NULL)
    {
        itidy_main_progress_update_heartbeat(progress_win, phase, (LONG)current, (LONG)total);
    }
}

/*========================================================================*/
/* Smart Folder Mode: Lightweight Pre-Scan                               */
/*========================================================================*/

BOOL deficons_folder_has_visible_contents(const char *path,
                                          const LayoutPreferences *prefs,
                                          struct iTidyMainProgressWindow *progress_window)
{
    BPTR lock = 0;
    struct FileInfoBlock *fib = NULL;
    BOOL visible = FALSE;

    if (!path || !prefs)
        return FALSE;

    /* Skip system paths - they won't get icons anyway */
    if (deficons_is_system_path(path, prefs))
        return FALSE;

    /* WHDLoad folder: the parent should still create a drawer icon for it,
     * so return TRUE (has visible contents) without scanning inside. */
    if (prefs->deficons_skip_whdload_folders && deficons_is_whdload_folder(path, prefs))
    {
        log_debug(LOG_ICONS, "Smart scan: WHDLoad folder, returning TRUE for drawer icon: %s\n", path);
        return TRUE;
    }

    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        log_debug(LOG_ICONS, "Smart scan: cannot lock %s\n", path);
        return FALSE;
    }

    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        UnLock(lock);
        return FALSE;
    }

    if (!Examine(lock, fib))
    {
        FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        return FALSE;
    }

    /* Pass 1: Check for existing .info files (cheapest check — no ARexx) */
    while (ExNext(lock, fib))
    {
        /* Skip system files that never receive icons */
        if (is_system_file_never_icon(fib->fib_FileName))
            continue;

        if (filename_has_info_suffix(fib->fib_FileName))
        {
            visible = TRUE;
            log_debug(LOG_ICONS, "Smart scan: found .info in %s: %s\n",
                      path, fib->fib_FileName);
            break;
        }
    }

    if (visible)
    {
        FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        return TRUE;
    }

    /* Pass 2: No .info files found — check if any file would get an icon.
     * Re-scan the directory. For each regular file without a .info, ask
     * DefIcons to identify it. Exit on first match. */
    /* Must re-Examine to reset ExNext iterator */
    if (!Examine(lock, fib))
    {
        FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        return FALSE;
    }

    while (ExNext(lock, fib))
    {
        char fullpath[512];
        char info_path[520];
        char type_token[64];

        /* Pump events so cancel button works during this scan */
        if (progress_window)
        {
            itidy_main_progress_window_handle_events(progress_window);
            if (progress_window->cancel_requested)
            {
                FreeDosObject(DOS_FIB, fib);
                UnLock(lock);
                return FALSE;
            }
        }

        /* Skip directories — we only care about files here */
        if (fib->fib_DirEntryType > 0)
            continue;

        /* Skip system files that never receive icons */
        if (is_system_file_never_icon(fib->fib_FileName))
            continue;

        /* Build full path */
        strncpy(fullpath, path, sizeof(fullpath) - 1);
        fullpath[sizeof(fullpath) - 1] = '\0';
        if (!AddPart(fullpath, fib->fib_FileName, sizeof(fullpath)))
            continue;

        /* Check if .info already exists — if so it would have been caught
         * in pass 1 (the file's base .info), so this file already has an icon.
         * But just in case the .info is for a different base name, skip check. */
        snprintf(info_path, sizeof(info_path), "%s.info", fullpath);
        {
            BPTR info_lock = Lock((STRPTR)info_path, ACCESS_READ);
            if (info_lock)
            {
                UnLock(info_lock);
                /* This file already has an icon — that means visible content */
                visible = TRUE;
                log_debug(LOG_ICONS, "Smart scan: file has .info in %s: %s\n",
                          path, fib->fib_FileName);
                break;
            }
        }

        /* Identify file type via DefIcons ARexx */
        if (deficons_identify_file(fullpath, type_token, sizeof(type_token)))
        {
            /* Check if this type is enabled in user filters */
            if (deficons_should_create_icon(type_token, prefs))
            {
                visible = TRUE;
                log_debug(LOG_ICONS, "Smart scan: file would get icon in %s: %s (type: %s)\n",
                          path, fib->fib_FileName, type_token);
                break;
            }
        }
    }

    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    return visible;
}

/*========================================================================*/
/* Icon Creation: Single Directory                                       */
/*========================================================================*/

BOOL deficons_create_missing_icons_in_directory(
    const char *path,
    const LayoutPreferences *prefs,
    iTidy_DefIconsCreationResult *result,
    struct iTidyMainProgressWindow *progress_window,
    BackupContext *backup_context,
    BOOL files_only)
{
    BPTR lock = 0;
    struct FileInfoBlock *fib = NULL;
    BOOL success = FALSE;
    char fullpath[512];
    char info_path[520];
    char type_token[64];
    char template_path[256];
    ULONG local_created = 0;
    BOOL local_visible = FALSE;

    if (!path || !prefs || !result)
    {
        log_error(LOG_ICONS, "Invalid parameters to deficons_create_missing_icons_in_directory\n");
        return FALSE;
    }

    /* Initialize result */
    result->icons_created = 0;
    result->has_visible_contents = FALSE;

    /* Check if system path — skip if exclusion enabled */
    if (deficons_is_system_path(path, prefs))
    {
        log_info(LOG_ICONS, "Skipping system path: %s\n", path);
        CREATION_STATUS(progress_window, "  Skipping system path: %s", path);
        return TRUE;
    }
    
    /* Check if path matches user exclude list */
    if (deficons_is_excluded_path(path, prefs))
    {
        log_info(LOG_ICONS, "Skipping user-excluded path: %s\n", path);
        CREATION_STATUS(progress_window, "  Skipping excluded path: %s", path);
        return TRUE;
    }

    /* WHDLoad folder: skip all icon creation inside (data, saves, etc.) */
    if (prefs->deficons_skip_whdload_folders && deficons_is_whdload_folder(path, prefs))
    {
        log_info(LOG_ICONS, "WHDLoad folder detected, skipping icon creation inside: %s\n", path);
        CREATION_STATUS(progress_window, "  Skipping WHDLoad folder: %s", path);
        /* result already zeroed — return success with nothing created */
        return TRUE;
    }

    /* Lock directory */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        log_error(LOG_ICONS, "Failed to lock directory for icon creation: %s\n", path);
        return FALSE;
    }

    /* Allocate FIB */
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        log_error(LOG_ICONS, "Failed to allocate FIB\n");
        UnLock(lock);
        return FALSE;
    }

    /* Examine directory */
    if (!Examine(lock, fib))
    {
        log_error(LOG_ICONS, "Failed to examine directory: %s\n", path);
        FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        return FALSE;
    }

    /* Process directory entries */
    /* Tracks the last type logged to progress window so we emit a type-change
     * header instead of repeating "[ascii]" on every line. */
    char last_logged_type[64];
    last_logged_type[0] = '\0';

    while (ExNext(lock, fib))
    {
        CREATION_CHECK_CANCEL(progress_window);

        /* Build full path */
        strncpy(fullpath, path, sizeof(fullpath) - 1);
        fullpath[sizeof(fullpath) - 1] = '\0';
        if (!AddPart(fullpath, fib->fib_FileName, sizeof(fullpath)))
        {
            log_error(LOG_ICONS, "Path too long for: %s", fib->fib_FileName);
            continue;
        }

        /* Skip system files that never receive icons */
        if (is_system_file_never_icon(fib->fib_FileName))
            continue;

        /* Build .info path */
        snprintf(info_path, sizeof(info_path), "%s.info", fullpath);

        /* Skip if .info already exists (never overwrite user icons). */
        /* Exception: if replace mode is active, iTidy-created icons    */
        /* (identified by ITIDY_CREATED tool type) may be deleted and   */
        /* recreated, giving the user a quick re-render on settings     */
        /* changes without manually deleting icons first.               */
        {
            BPTR info_lock = Lock((STRPTR)info_path, ACCESS_READ);
            if (info_lock)
            {
                BOOL do_replace = FALSE;
                UnLock(info_lock);

                /* Only inspect the icon when a replace mode is active */
                if (prefs->deficons_replace_itidy_thumbnails ||
                    prefs->deficons_replace_itidy_text_previews)
                {
                    struct DiskObject *existing = GetDiskObject((STRPTR)fullpath);
                    if (existing != NULL)
                    {
                        STRPTR *tts = (STRPTR *)existing->do_ToolTypes;
                        STRPTR created_val = (STRPTR)FindToolType(tts, (STRPTR)ITIDY_TT_CREATED);
                        if (created_val != NULL)
                        {
                            /* It is an iTidy-made icon - check the kind */
                            STRPTR kind_val = (STRPTR)FindToolType(tts, (STRPTR)ITIDY_TT_KIND);
                            if (kind_val != NULL &&
                                strcmp((const char *)kind_val, ITIDY_KIND_TEXT_PREVIEW) == 0)
                            {
                                if (prefs->deficons_replace_itidy_text_previews)
                                    do_replace = TRUE;
                            }
                            else
                            {
                                /* iff_thumbnail, font_preview, or any other iTidy kind */
                                if (prefs->deficons_replace_itidy_thumbnails)
                                    do_replace = TRUE;
                            }
                        }
                        FreeDiskObject(existing);
                    }
                }

                if (do_replace)
                {
                    /* Remove the old iTidy icon so creation proceeds normally */
                    if (DeleteFile((STRPTR)info_path))
                    {
                        log_info(LOG_ICONS, "Deleted iTidy icon for replacement: %s\n",
                                 fib->fib_FileName);
                        /* Fall through - icon is gone, creation loop will remake it */
                    }
                    else
                    {
                        log_warning(LOG_ICONS, "Could not delete iTidy icon for replacement: %s\n",
                                    fib->fib_FileName);
                        /* Leave the existing icon in place */
                        local_visible = TRUE;
                        continue;
                    }
                }
                else
                {
                    log_debug(LOG_ICONS, "Icon already exists, skipping: %s\n",
                              fib->fib_FileName);
                    /* Existing .info means this folder has visible content */
                    local_visible = TRUE;
                    continue;
                }
            }
        }

        /* Handle drawers (directories) */
        if (fib->fib_DirEntryType > 0)
        {
            if (files_only)
            {
                /* Post-order mode: drawer icon decisions are handled in
                 * deficons_create_missing_icons_recursive() after each subdir
                 * has been fully processed.  We already checked above that no
                 * .info exists (otherwise we would have continued there with
                 * local_visible = TRUE), so this directory is not yet visible.
                 * Skip it and let the recursive function decide. */
                continue;
            }

            /* Non-recursive (single-folder) mode: pre-scan as before */
            {
                BOOL should_create = FALSE;

                /* Determine whether to create a drawer icon */
                switch (prefs->deficons_folder_icon_mode)
                {
                    case DEFICONS_FOLDER_MODE_ALWAYS:
                        should_create = TRUE;
                        break;

                    case DEFICONS_FOLDER_MODE_NEVER:
                        should_create = FALSE;
                        break;

                    case DEFICONS_FOLDER_MODE_SMART:
                    default:
                        /* Smart mode: lightweight pre-scan of the subdirectory */
                        should_create = deficons_folder_has_visible_contents(fullpath, prefs,
                                                                             progress_window);
                        break;
                }

                if (!deficons_should_create_folder_icon(fullpath, should_create, prefs))
                {
                    log_debug(LOG_ICONS, "Folder icon not needed: %s\n", fib->fib_FileName);
                    continue;
                }

                strcpy(type_token, "drawer");
            }
        }
        else
        {
            /* Skip empty files (0 bytes) - no content to identify or preview */
            if (fib->fib_Size == 0)
            {
                log_debug(LOG_ICONS, "Skipping empty file (0 bytes): %s\n", fib->fib_FileName);
                continue;
            }

            /* Identify file type via DefIcons */
            if (!deficons_identify_file(fullpath, type_token, sizeof(type_token)))
            {
                log_debug(LOG_ICONS, "Cannot identify type: %s\n", fib->fib_FileName);
                continue;
            }

            /* Apply type filters */
            if (!deficons_should_create_icon(type_token, prefs))
            {
                log_debug(LOG_ICONS, "Type filtered: %s (%s)\n",
                          fib->fib_FileName, type_token);
                continue;
            }
        }

        /* Resolve template */
        if (!deficons_resolve_template(type_token, template_path, sizeof(template_path)))
        {
            log_warning(LOG_ICONS, "No template for type: %s (file: %s)\n",
                        type_token, fib->fib_FileName);
            continue;
        }

        /* Copy template to create .info */
        if (deficons_copy_icon_file(template_path, info_path))
        {
            const char *category;

            local_created++;
            local_visible = TRUE;
            log_info(LOG_ICONS, "Created icon: %s -> %s\n", template_path, info_path);

            /* Log to backup manifest */
            if (backup_context != NULL && backup_context->createdIconsOpen)
            {
                LogCreatedIconToManifest(backup_context, info_path);
            }

            /* Apply content preview for text-type files and image thumbnails.
             * preview_result is declared here (not inside the if block) so
             * the progress status line below can use it to pick the right verb. */
            int preview_result = ITIDY_PREVIEW_NOT_APPLICABLE;
            if (fib->fib_DirEntryType <= 0)  /* Files only, not drawers */
            {
                log_debug(LOG_ICONS, "Calling itidy_apply_content_preview: progress_window=%p, wrapper=%p\n",
                          (void*)progress_window, (void*)progress_callback_wrapper);
                
                preview_result = itidy_apply_content_preview(
                    fullpath, type_token,
                    (ULONG)fib->fib_Size, &fib->fib_Date,
                    progress_callback_wrapper,
                    progress_window);

                if (preview_result == ITIDY_PREVIEW_APPLIED)
                {
                    log_info(LOG_ICONS, "Content preview applied: %s (type: %s)\n",
                             fib->fib_FileName, type_token);
                }
                else if (preview_result == ITIDY_PREVIEW_FAILED)
                {
                    log_debug(LOG_ICONS, "Content preview skipped/failed: %s\n",
                              fib->fib_FileName);
                }
            }

            /* Track category statistics */
            category = deficons_get_resolved_category(type_token);
            if (category && category[0] != '\0')
            {
                deficons_creation_increment_category(category);
                log_debug(LOG_ICONS, "Category tracked: %s (type: %s)\n",
                          category, type_token);
            }

            /* Update heartbeat for progress feedback */
            if (progress_window)
            {
                itidy_main_progress_update_heartbeat(progress_window,
                                                     "Creating", local_created, 0);

                /* Emit a type-change header when the icon type changes within
                 * this directory -- avoids repeating "[ascii]" on every line.
                 * Then show just the filename so the list stays readable. */
                if (strcmp(type_token, last_logged_type) != 0)
                {
                    itidy_main_progress_window_append_status(progress_window,
                        "--- %s ---", type_token);
                    strncpy(last_logged_type, type_token, sizeof(last_logged_type) - 1);
                    last_logged_type[sizeof(last_logged_type) - 1] = '\0';
                }
                /* Pick verb based on what the preview pipeline actually did:
                 *   Rendering  -- bespoke text render (ascii, c, rexx, etc.)
                 *   Generating -- image thumbnail (ilbm, tiff, iff, etc.)
                 *   Creating   -- template copy only (music, bin, drawer, etc.) */
                const char *action;
                if (preview_result == ITIDY_PREVIEW_APPLIED)
                {
                    if (itidy_is_text_preview_type(type_token))
                        action = "Rendering";
                    else
                        action = "Generating";
                }
                else
                {
                    action = "Creating";
                }
                itidy_main_progress_window_append_status(progress_window,
                    "  %s icon for: %s",
                    action,
                    fib->fib_FileName);
            }
        }
        else
        {
            log_error(LOG_ICONS, "Failed to copy icon template: %s -> %s\n",
                      template_path, info_path);
        }
    }

    success = TRUE;
    result->icons_created = local_created;
    result->has_visible_contents = local_visible;

    if (local_created > 0)
    {
        log_info(LOG_ICONS, "Created %lu icon(s) in: %s\n", local_created, path);
    }

    /* Cleanup */
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);

    return success;
}

/*========================================================================*/
/* Icon Creation: Recursive                                              */
/*========================================================================*/

BOOL deficons_create_missing_icons_recursive(
    const char *path,
    const LayoutPreferences *prefs,
    int recursion_level,
    iTidy_DefIconsCreationResult *result,
    struct iTidyMainProgressWindow *progress_window,
    BackupContext *backup_context)
{
    BPTR lock = 0;
    struct FileInfoBlock *fib = NULL;
    char subdir[512];
    iTidy_DefIconsCreationResult folder_result;

    if (!path || !prefs || !result)
    {
        log_error(LOG_ICONS, "Invalid parameters to deficons_create_missing_icons_recursive\n");
        return FALSE;
    }

    /* Initialize result */
    result->icons_created = 0;
    result->has_visible_contents = FALSE;

    /* Recursion depth check */
    if (recursion_level > 100)
    {
        log_error(LOG_ICONS, "Maximum recursion depth exceeded: %s\n", path);
        return FALSE;
    }

    CREATION_CHECK_CANCEL(progress_window);

    /* Show current folder in progress window so user knows what's happening */
    if (progress_window)
    {
        itidy_main_progress_update_heartbeat(progress_window,
                                             "Scanning", result->icons_created, 0);
    }
    CREATION_STATUS(progress_window, "  Creating icons: %s", path);

    /* Process current directory */
    log_debug(LOG_ICONS, "Icon creation: %s (level %d)\n", path, recursion_level);

    folder_result.icons_created = 0;
    folder_result.has_visible_contents = FALSE;

    if (!deficons_create_missing_icons_in_directory(path, prefs, &folder_result,
                                                     progress_window, backup_context, TRUE))
    {
        log_warning(LOG_ICONS, "Failed to process directory: %s\n", path);
        /* Continue with subdirectories even if current fails */
    }

    result->icons_created += folder_result.icons_created;
    if (folder_result.has_visible_contents)
    {
        result->has_visible_contents = TRUE;
    }

    /* Lock directory for subdirectory enumeration */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        log_error(LOG_ICONS, "Cannot lock directory for subdirs: %s\n", path);
        return FALSE;
    }

    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        log_error(LOG_ICONS, "Cannot allocate FIB for subdirs\n");
        UnLock(lock);
        return FALSE;
    }

    if (!Examine(lock, fib))
    {
        log_error(LOG_ICONS, "Cannot examine directory for subdirs: %s\n", path);
        FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        return FALSE;
    }

    /* Process subdirectories */
    while (ExNext(lock, fib))
    {
        iTidy_DefIconsCreationResult child_result;

        CREATION_CHECK_CANCEL(progress_window);

        /* Skip files — only process directories */
        if (fib->fib_DirEntryType <= 0)
            continue;

        /* Build subdirectory path */
        strncpy(subdir, path, sizeof(subdir) - 1);
        subdir[sizeof(subdir) - 1] = '\0';
        if (!AddPart(subdir, fib->fib_FileName, sizeof(subdir)))
        {
            log_error(LOG_ICONS, "Path too long for subdir: %s", fib->fib_FileName);
            continue;
        }

        /* Apply skipHiddenFolders logic if enabled.
         * In Smart mode, we still recurse into folders without .info if they
         * have visible contents — Smart mode may create icons inside them,
         * and their parent may create a drawer .info for them. */
        if (prefs->skipHiddenFolders)
        {
            char info_path_buf[520];
            BPTR info_lock;

            snprintf(info_path_buf, sizeof(info_path_buf), "%s.info", subdir);
            info_lock = Lock((STRPTR)info_path_buf, ACCESS_READ);

            if (!info_lock)
            {
                /* No .info exists for this folder. */
                if (prefs->deficons_folder_icon_mode == DEFICONS_FOLDER_MODE_NEVER)
                {
                    /* Never mode: no drawer icon will be created regardless, so
                     * there is nothing to gain from recursing. Skip. */
                    log_debug(LOG_ICONS, "Skipping hidden folder (Never mode): %s\n",
                              fib->fib_FileName);
                    continue;
                }
                /* Smart mode: recurse and let the post-order result decide.  If the
                 * subdir turns out to have no visible content, no drawer .info will
                 * be created and it stays hidden -- identical outcome to the old
                 * pre-scan but without the double-scan cost.
                 * Always mode: recurse; a drawer icon will be created regardless. */
                log_debug(LOG_ICONS, "No .info for folder, recursing (post-order): %s\n",
                          fib->fib_FileName);
            }
            else
            {
                UnLock(info_lock);
            }
        }

        /* WHDLoad subfolder: skip recursion entirely — no icons needed inside */
        if (prefs->deficons_skip_whdload_folders && deficons_is_whdload_folder(subdir, prefs))
        {
            log_debug(LOG_ICONS, "WHDLoad subfolder, skipping recursion: %s\n", fib->fib_FileName);
            continue;
        }

        /* Recurse into subdirectory */
        child_result.icons_created = 0;
        child_result.has_visible_contents = FALSE;

        deficons_create_missing_icons_recursive(subdir, prefs, recursion_level + 1,
                                                 &child_result,
                                                 progress_window, backup_context);

        result->icons_created += child_result.icons_created;
        if (child_result.has_visible_contents)
        {
            result->has_visible_contents = TRUE;
        }

        /* Post-order drawer icon creation: now that we know whether the subdir
         * has visible contents, decide whether to create its drawer .info in
         * the current (parent) directory. */
        {
            char drawer_info[520];
            BPTR existing_info;

            snprintf(drawer_info, sizeof(drawer_info), "%s.info", subdir);
            existing_info = Lock((STRPTR)drawer_info, ACCESS_READ);
            if (existing_info)
            {
                UnLock(existing_info);
                /* .info already exists -- folder is already visible in Workbench.
                 * Signal the grandparent that this level has visible content. */
                result->has_visible_contents = TRUE;
            }
            else if (deficons_should_create_folder_icon(subdir,
                                                        child_result.has_visible_contents,
                                                        prefs))
            {
                char tmpl_path[256];
                if (deficons_resolve_template("drawer", tmpl_path, sizeof(tmpl_path)))
                {
                    if (deficons_copy_icon_file(tmpl_path, drawer_info))
                    {
                        result->icons_created++;
                        result->has_visible_contents = TRUE;
                        log_info(LOG_ICONS, "Created drawer icon (post-order): %s\n", drawer_info);

                        /* Log to backup manifest */
                        if (backup_context != NULL && backup_context->createdIconsOpen)
                        {
                            LogCreatedIconToManifest(backup_context, drawer_info);
                        }

                        /* Per-type stats */
                        deficons_creation_increment_category("drawer");

                        /* Heartbeat update + readable output line */
                        if (progress_window)
                        {
                            itidy_main_progress_update_heartbeat(progress_window,
                                                                  "Creating",
                                                                  result->icons_created, 0);

                            /* Show the folder name so the user has something to
                             * read on slow machines.  FilePart() gives just the
                             * final path component without a lock/FIB. */
                            itidy_main_progress_window_append_status(progress_window,
                                "  Creating icon for: %s/  [drawer]",
                                FilePart((STRPTR)subdir));
                        }
                    }
                    else
                    {
                        log_warning(LOG_ICONS, "Failed to copy drawer template: %s\n", drawer_info);
                    }
                }
                else
                {
                    log_warning(LOG_ICONS, "No drawer template available for: %s\n", subdir);
                }
            }
        }
    }

    /* Cleanup */
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);

    return TRUE;
}
