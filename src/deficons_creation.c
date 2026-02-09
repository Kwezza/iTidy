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
#include "layout_preferences.h"
#include "writeLog.h"
#include "utilities.h"
#include "backup_session.h"
#include "path_utilities.h"

/* Progress window integration */
#include "GUI/StatusWindows/main_progress_window.h"

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

/*========================================================================*/
/* Icon Creation Log File                                                */
/*========================================================================*/

static BPTR g_creation_log_file = 0;

BOOL deficons_creation_open_log(void)
{
    const char *timestamp;
    char log_path[512];
    char dir_path[256];

    if (g_creation_log_file != 0)
        return TRUE;

    snprintf(dir_path, sizeof(dir_path), "PROGDIR:logs/IconsCreated/");
    if (!CreateDirectoryPath(dir_path))
    {
        log_warning(LOG_ICONS, "Failed to create IconsCreated directory: %s\n", dir_path);
        return FALSE;
    }

    timestamp = get_log_timestamp();
    snprintf(log_path, sizeof(log_path), "%screated_%s.txt", dir_path, timestamp);

    g_creation_log_file = Open((CONST_STRPTR)log_path, MODE_NEWFILE);
    if (g_creation_log_file == 0)
    {
        log_error(LOG_ICONS, "Failed to create icon creation log: %s\n", log_path);
        return FALSE;
    }

    {
        const char *header = "; iTidy Icon Creation Log\n"
                             "; One .info file path per line\n"
                             "; Use this to generate delete scripts for testing\n"
                             ";\n";
        Write(g_creation_log_file, (APTR)header, strlen(header));
    }

    log_info(LOG_ICONS, "Icon creation log opened: %s\n", log_path);
    return TRUE;
}

void deficons_creation_log_icon(const char *info_path)
{
    if (g_creation_log_file == 0)
        return;
    if (!info_path || info_path[0] == '\0')
        return;

    Write(g_creation_log_file, (APTR)info_path, strlen(info_path));
    Write(g_creation_log_file, (APTR)"\n", 1);
    Flush(g_creation_log_file);
}

void deficons_creation_close_log(void)
{
    if (g_creation_log_file != 0)
    {
        Close(g_creation_log_file);
        g_creation_log_file = 0;
        log_info(LOG_ICONS, "Icon creation log closed\n");
    }
}

/*========================================================================*/
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
    if (len < 6)  /* minimum: "x.info" */
        return FALSE;

    /* Compare last 5 characters case-insensitively */
    return (strncasecmp_custom(filename + len - 5, ".info", 5) == 0);
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

        /* Skip .info files themselves */
        if (filename_has_info_suffix(fib->fib_FileName))
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
    BackupContext *backup_context)
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

        /* Skip .info files themselves (proper suffix check) */
        if (filename_has_info_suffix(fib->fib_FileName))
            continue;

        /* Build .info path */
        snprintf(info_path, sizeof(info_path), "%s.info", fullpath);

        /* Skip if .info already exists (never overwrite user icons) */
        {
            BPTR info_lock = Lock((STRPTR)info_path, ACCESS_READ);
            if (info_lock)
            {
                UnLock(info_lock);
                log_debug(LOG_ICONS, "Icon already exists, skipping: %s\n", fib->fib_FileName);
                /* Existing .info means this folder has visible content */
                local_visible = TRUE;
                continue;
            }
        }

        /* Handle drawers (directories) */
        if (fib->fib_DirEntryType > 0)
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
        else
        {
            /* Identify file type via DefIcons */
            if (!deficons_identify_file(fullpath, type_token, sizeof(type_token)))
            {
                log_debug(LOG_ICONS, "Cannot identify type: %s\n", fib->fib_FileName);
                continue;
            }

            log_debug(LOG_ICONS, "DefIcons identified: %s -> type '%s'\n",
                     fib->fib_FileName, type_token);

            /* Apply type filters */
            if (!deficons_should_create_icon(type_token, prefs))
            {
                log_debug(LOG_ICONS, "Type filtered: %s (%s)\n",
                          fib->fib_FileName, type_token);
                continue;
            }
        }

        /* Resolve template */
        log_debug(LOG_ICONS, "Resolving template for type '%s'...\n", type_token);
        if (!deficons_resolve_template(type_token, template_path, sizeof(template_path)))
        {
            log_warning(LOG_ICONS, "No template for type: %s (file: %s)\n",
                        type_token, fib->fib_FileName);
            continue;
        }

        log_debug(LOG_ICONS, "Template resolved: %s -> %s\n", type_token, template_path);

        /* Copy template to create .info */
        if (deficons_copy_icon_file(template_path, info_path))
        {
            const char *category;

            local_created++;
            local_visible = TRUE;
            log_info(LOG_ICONS, "Created icon: %s -> %s\n", template_path, info_path);

            /* Log to dedicated icon creation file */
            {
                const LayoutPreferences *log_prefs = GetGlobalPreferences();
                if (log_prefs && log_prefs->deficons_log_created_icons)
                {
                    deficons_creation_log_icon(info_path);
                }
            }

            /* Log to backup manifest */
            if (backup_context != NULL && backup_context->createdIconsOpen)
            {
                LogCreatedIconToManifest(backup_context, info_path);
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
                                                     progress_window, backup_context))
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
                /* No .info exists for this folder */
                if (prefs->deficons_folder_icon_mode == DEFICONS_FOLDER_MODE_SMART)
                {
                    /* Smart mode: still recurse if it has visible contents,
                     * because icon creation phase may be about to create icons
                     * inside it (and its drawer .info will be created by the parent). */
                    if (!deficons_folder_has_visible_contents(subdir, prefs, progress_window))
                    {
                        log_debug(LOG_ICONS, "Skipping hidden folder (Smart, no visible content): %s\n",
                                  fib->fib_FileName);
                        continue;
                    }
                    /* Has visible contents — fall through and recurse */
                    log_debug(LOG_ICONS, "Hidden folder has visible content, recursing (Smart): %s\n",
                              fib->fib_FileName);
                }
                else
                {
                    log_debug(LOG_ICONS, "Skipping hidden folder: %s\n", fib->fib_FileName);
                    continue;
                }
            }
            else
            {
                UnLock(info_lock);
            }
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
    }

    /* Cleanup */
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);

    return TRUE;
}
