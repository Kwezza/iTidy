/*
 * deficons_filters.c - DefIcons Icon Creation Filtering Logic
 * 
 * Implements filtering logic for DefIcons icon creation based on user preferences.
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#include "deficons_filters.h"
#include "deficons_templates.h"
#include "../backups/backup_paths.h"
#include "../writeLog.h"
#include "../utilities.h"
#include "../platform/platform.h"

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>        /* MatchFirst/MatchNext/MatchEnd for WHDLoad detection */
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

/*========================================================================*/
/* System Path Prefixes                                                  */
/*========================================================================*/

static const char *g_system_path_prefixes[] = {
    "SYS:C/",
    "SYS:S/",
    "SYS:Devs/",
    "SYS:Libs/",
    "SYS:L/",
    "SYS:Fonts/",
    "SYS:Locale/",
    "SYS:Classes/",
    "SYS:Rexxc/",
    "SYS:System/",
    "C:",
    "S:",
    "DEVS:",
    "LIBS:",
    "L:",
    "FONTS:",
    NULL
};

/*========================================================================*/
/* Helper Functions                                                      */
/*========================================================================*/

/**
 * @brief Case-insensitive string prefix check
 * 
 * @param str String to check
 * @param prefix Prefix to look for
 * @return TRUE if str starts with prefix (case-insensitive)
 */
static BOOL starts_with_ignore_case(const char *str, const char *prefix)
{
    size_t i;
    
    if (!str || !prefix)
        return FALSE;
    
    for (i = 0; prefix[i] != '\0'; i++)
    {
        if (str[i] == '\0')
            return FALSE;  /* str is shorter than prefix */
        
        if (tolower((unsigned char)str[i]) != tolower((unsigned char)prefix[i]))
            return FALSE;
    }
    
    return TRUE;
}

/*========================================================================*/
/* Public Functions                                                      */
/*========================================================================*/

BOOL deficons_should_create_icon(const char *type_token, const LayoutPreferences *prefs)
{
    const char *category;
    
    if (!type_token || !prefs)
    {
        log_error(LOG_ICONS, "Invalid parameters to deficons_should_create_icon\n");
        return FALSE;
    }
    
    /* Check master enable flag first */
    if (!prefs->enable_deficons_icon_creation)
    {
        log_debug(LOG_ICONS, "Icon creation disabled by master flag\n");
        return FALSE;
    }
    
    /* Get resolved category for this type */
    category = deficons_get_resolved_category(type_token);
    if (!category)
    {
        /* Cannot determine category - allow by default */
        log_debug(LOG_ICONS, "Cannot determine category for %s - allowing\n", type_token);
        return TRUE;
    }
    
    /* Check if category is enabled using layout_preferences helper */
    if (!is_deficon_type_enabled(prefs, category))
    {
        log_debug(LOG_ICONS, "Type filtered: %s (category: %s)\n", type_token, category);
        return FALSE;
    }
    
    /* All checks passed */
    return TRUE;
}

/*========================================================================*/
/**
 * @brief Check if path matches user-defined exclude list
 * 
 * Tests whether a path should be excluded based on the user-configurable
 * exclude paths list. Supports DEVICE: placeholder substitution.
 * 
 * @param path Full path to check
 * @param prefs Layout preferences containing exclude paths array and folder_path
 * 
 * @return TRUE if path matches any exclude pattern, FALSE otherwise
 */
/*========================================================================*/
BOOL deficons_is_excluded_path(const char *path, const LayoutPreferences *prefs)
{
    char device[32];
    char pattern[256];
    int i;
    
    if (!path || !prefs)
    {
        log_error(LOG_ICONS, "Invalid parameters to deficons_is_excluded_path\n");
        return FALSE;
    }
    
    /* Extract device from current scan folder (e.g., "SYS:" from "SYS:Fonts") */
    if (!GetDeviceName(prefs->folder_path, device))
    {
        log_debug(LOG_ICONS, "Cannot extract device from folder_path: %s\n", prefs->folder_path);
        /* Continue anyway - absolute patterns will still work */
        device[0] = '\0';
    }
    
    /* Check each exclude path pattern */
    for (i = 0; i < prefs->deficons_exclude_path_count; i++)
    {
        const char *exclude_pattern = prefs->deficons_exclude_paths[i];
        
        if (exclude_pattern[0] == '\0')
            continue;  /* Skip empty entries */
        
        /* Check if pattern starts with "DEVICE:" placeholder */
        if (strncmp(exclude_pattern, "DEVICE:", 7) == 0)
        {
            /* Substitute current scan device */
            if (device[0] != '\0')
            {
                /* Build pattern: "SYS:" + "Fonts" */
                snprintf(pattern, sizeof(pattern), "%s:%s", device, exclude_pattern + 7);
            }
            else
            {
                /* No device available - skip this pattern */
                continue;
            }
        }
        else
        {
            /* Use absolute pattern as-is (e.g., "Work:MyStuff") */
            strncpy(pattern, exclude_pattern, sizeof(pattern) - 1);
            pattern[sizeof(pattern) - 1] = '\0';
        }
        
        /* Case-insensitive prefix match */
        if (starts_with_ignore_case(path, pattern))
        {
            log_debug(LOG_ICONS, "Path excluded by pattern '%s' -> '%s'\n", 
                      exclude_pattern, pattern);
            return TRUE;
        }
    }
    
    return FALSE;
}

BOOL deficons_is_system_path(const char *path, const LayoutPreferences *prefs)
{
    int i;
    
    if (!path || !prefs)
    {
        log_error(LOG_ICONS, "Invalid parameters to deficons_is_system_path\n");
        return FALSE;
    }
    
    /* Check if system path exclusion is enabled */
    if (!prefs->deficons_skip_system_assigns)
    {
        /* Feature disabled - no paths are system paths */
        return FALSE;
    }
    
    /* Check each system path prefix */
    for (i = 0; g_system_path_prefixes[i] != NULL; i++)
    {
        if (starts_with_ignore_case(path, g_system_path_prefixes[i]))
        {
            log_debug(LOG_ICONS, "System path detected: %s\n", path);
            return TRUE;
        }
    }
    
    return FALSE;
}

BOOL deficons_should_create_folder_icon(const char *path, BOOL has_visible_contents, 
                                        const LayoutPreferences *prefs)
{
    UWORD mode;
    
    if (!path || !prefs)
    {
        log_error(LOG_ICONS, "Invalid parameters to deficons_should_create_folder_icon\n");
        return FALSE;
    }
    
    mode = prefs->deficons_folder_icon_mode;
    
    /* Mode 2: Never create folder icons */
    if (mode == DEFICONS_FOLDER_MODE_NEVER)
    {
        log_debug(LOG_ICONS, "Folder icon skipped (mode: Never): %s\n", path);
        return FALSE;
    }
    
    /* Mode 1: Always create folder icons */
    if (mode == DEFICONS_FOLDER_MODE_ALWAYS)
    {
        log_debug(LOG_ICONS, "Folder icon enabled (mode: Always): %s\n", path);
        return TRUE;
    }
    
    /* Mode 0: Smart mode - create if folder has visible contents */
    if (has_visible_contents)
    {
        log_debug(LOG_ICONS, "Folder icon enabled (mode: Smart, has contents): %s\n", path);
        return TRUE;
    }
    else
    {
        log_debug(LOG_ICONS, "Folder icon skipped (mode: Smart, no visible contents): %s\n", path);
        return FALSE;
    }
}

BOOL deficons_folder_has_info_files(const char *path)
{
    BPTR lock = 0;
    struct FileInfoBlock *fib = NULL;
    BOOL has_info = FALSE;
    
    if (!path)
    {
        log_error(LOG_ICONS, "Invalid path to deficons_folder_has_info_files\n");
        return FALSE;
    }
    
    /* Lock directory */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        log_debug(LOG_ICONS, "Cannot lock directory: %s\n", path);
        return FALSE;
    }
    
    /* Allocate FIB */
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        log_error(LOG_ICONS, "Cannot allocate FIB\n");
        UnLock(lock);
        return FALSE;
    }
    
    /* Examine directory */
    if (!Examine(lock, fib))
    {
        log_error(LOG_ICONS, "Cannot examine directory: %s\n", path);
        FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        return FALSE;
    }
    
    /* Scan for .info files (proper suffix check) */
    while (ExNext(lock, fib))
    {
        /* Check if filename ends with .info (not just contains it) */
        size_t name_len = strlen(fib->fib_FileName);
        if (name_len >= 6 && strncasecmp_custom(fib->fib_FileName + name_len - 5, ".info", 5) == 0)
        {
            has_info = TRUE;
            log_debug(LOG_ICONS, "Found .info file: %s/%s\n", path, fib->fib_FileName);
            break;
        }
    }
    
    /* Cleanup */
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    return has_info;
}

const char* deficons_folder_mode_to_string(UWORD mode)
{
    switch (mode)
    {
        case DEFICONS_FOLDER_MODE_SMART:
            return "Smart";
        case DEFICONS_FOLDER_MODE_ALWAYS:
            return "Always";
        case DEFICONS_FOLDER_MODE_NEVER:
            return "Never";
        default:
            return "Unknown";
    }
}

/*========================================================================*/
/* WHDLoad Folder Detection                                              */
/*========================================================================*/

BOOL deficons_is_whdload_folder(const char *path, const LayoutPreferences *prefs)
{
    struct AnchorPath *ap;
    char pattern[520];
    size_t path_len;
    LONG match_result;
    BOOL is_whdload = FALSE;

    if (!path || path[0] == '\0' || !prefs)
        return FALSE;

    /* Build AmigaDOS pattern: path/#?.slave */
    path_len = strlen(path);
    if (path_len > 0 && path[path_len - 1] == ':')
        snprintf(pattern, sizeof(pattern), "%s#?.slave", path);
    else
        snprintf(pattern, sizeof(pattern), "%s/#?.slave", path);

    /* Allocate AnchorPath — 256-byte path buffer suffix (same as folder_scanner.c) */
    ap = (struct AnchorPath *)whd_malloc(sizeof(struct AnchorPath) + 255);
    if (!ap)
    {
        log_warning(LOG_ICONS, "WHDLoad check: failed to allocate AnchorPath for %s\n", path);
        return FALSE;
    }

    ap->ap_Strlen    = 256;
    ap->ap_BreakBits = 0;
    ap->ap_Flags     = 0;

    match_result = MatchFirst((CONST_STRPTR)pattern, ap);
    if (match_result == 0)
    {
        /* At least one *.slave file found — this is a WHDLoad folder */
        is_whdload = TRUE;
        log_debug(LOG_ICONS, "WHDLoad folder detected (found .slave): %s\n", path);
    }
    /* Any other result means no match — not a WHDLoad folder */

    MatchEnd(ap);
    whd_free(ap);

    return is_whdload;
}
