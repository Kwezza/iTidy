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
#include "writeLog.h"
#include "platform/platform.h"

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*========================================================================*/
/* System Path Prefixes                                                  */
/*========================================================================*/

static const char *g_system_path_prefixes[] = {
    "SYS:",
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
    
    /* Scan for .info files */
    while (ExNext(lock, fib))
    {
        /* Check if filename ends with .info */
        if (strstr(fib->fib_FileName, ".info") != NULL)
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
