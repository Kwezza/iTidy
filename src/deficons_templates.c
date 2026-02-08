/*
 * deficons_templates.c - DefIcons Template Resolution and Icon Copying
 * 
 * Implements template scanning, parent-chain resolution, and icon file copying.
 * Based on working implementation from Tests/DefIcons/Test2/deficons_creator.c
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#include "deficons_templates.h"
#include "deficons_parser.h"
#include "writeLog.h"
#include "platform/platform.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

/*========================================================================*/
/* External References                                                   */
/*========================================================================*/

/* Global DefIcons tree from main_gui.c */
extern DeficonTypeTreeNode *g_cached_deficons_tree;
extern int g_cached_deficons_count;

/*========================================================================*/
/* Configuration                                                         */
/*========================================================================*/

#define COPY_BUFFER_SIZE 8192  /* Icon file copy buffer size */
#define MAX_PARENT_DEPTH 20    /* Maximum parent chain depth (cycle protection) */

/*========================================================================*/
/* Template Search Locations                                            */
/*========================================================================*/

/* CRITICAL: ENVARC: is permanent storage (SYS:Prefs/Env-Archive/)
 * ENV: is RAM disk - templates stored there would waste precious memory
 * Check ENVARC: locations FIRST, then fall back to ENV: if needed
 */
static const char *g_template_search_paths[] = {
    "ENVARC:Sys/def_%s.info",           /* Primary: Permanent storage */
    "SYS:Prefs/Env-Archive/Sys/def_%s.info",  /* Explicit path */
    "ENVARC:def_%s.info",               /* Legacy location */
    "ENV:Sys/def_%s.info",              /* RAM disk (fallback) */
    "ENV:def_%s.info",                  /* RAM disk legacy */
    NULL
};

/*========================================================================*/
/* Global State                                                          */
/*========================================================================*/

/* Template cache */
static TemplateCacheEntry *g_template_cache = NULL;
static int g_template_cache_count = 0;

/* Initialization flag */
static BOOL g_initialized = FALSE;

/* Available templates count (for statistics) */
static int g_available_templates_count = 0;

/*========================================================================*/
/* Helper Functions                                                      */
/*========================================================================*/

/**
 * @brief Check if a file exists
 * 
 * @param path Full file path
 * @return TRUE if file exists, FALSE otherwise
 */
static BOOL file_exists(const char *path)
{
    BPTR lock;
    
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock)
    {
        UnLock(lock);
        return TRUE;
    }
    
    return FALSE;
}

/**
 * @brief Find template file for a specific type token
 * 
 * @param type_token Type token to search for
 * @param template_path Buffer to receive template path
 * @param path_size Size of template_path buffer
 * @return TRUE if template found, FALSE otherwise
 */
static BOOL find_template_file(const char *type_token, char *template_path, int path_size)
{
    int i;
    char test_path[DEFICONS_MAX_PATH_LEN];
    
    if (!type_token || !template_path)
        return FALSE;
    
    /* Try each search location */
    for (i = 0; g_template_search_paths[i] != NULL; i++)
    {
        snprintf(test_path, sizeof(test_path), g_template_search_paths[i], type_token);
        
        log_debug(LOG_ICONS, "Checking template: %s\n", test_path);
        
        if (file_exists(test_path))
        {
            strncpy(template_path, test_path, path_size - 1);
            template_path[path_size - 1] = '\0';
            log_info(LOG_ICONS, "Found template: %s → %s\n", type_token, test_path);
            return TRUE;
        }
    }
    
    log_debug(LOG_ICONS, "No template found for type: %s\n", type_token);
    return FALSE;
}

/**
 * @brief Get parent type name from DefIcons tree
 * 
 * @param type_name Type token to look up
 * @return Parent type name, or NULL if no parent or not found
 */
static const char* get_parent_type(const char *type_name)
{
    if (!g_cached_deficons_tree || !type_name)
        return NULL;
    
    return get_parent_type_name(g_cached_deficons_tree, g_cached_deficons_count, type_name);
}

/**
 * @brief Find root category for a type token
 * 
 * @param type_token Type token to look up
 * @param category Buffer to receive root category name
 * @param category_size Size of category buffer
 * @return TRUE if found, FALSE otherwise
 */
static BOOL find_root_category(const char *type_token, char *category, int category_size)
{
    const char *current_type = type_token;
    const char *parent;
    int depth = 0;
    
    if (!g_cached_deficons_tree || !type_token || !category)
        return FALSE;
    
    /* Walk up to root (generation 2 node) */
    while (depth < MAX_PARENT_DEPTH)
    {
        int i;
        
        /* Find current type in tree */
        for (i = 0; i < g_cached_deficons_count; i++)
        {
            if (strcmp(g_cached_deficons_tree[i].type_name, current_type) == 0)
            {
                /* Check if this is a root category (generation 2) */
                if (g_cached_deficons_tree[i].generation == 2)
                {
                    strncpy(category, current_type, category_size - 1);
                    category[category_size - 1] = '\0';
                    return TRUE;
                }
                break;
            }
        }
        
        /* Get parent */
        parent = get_parent_type(current_type);
        if (!parent)
            break;
        
        current_type = parent;
        depth++;
    }
    
    /* Default to "other" if we can't determine category */
    strncpy(category, "other", category_size - 1);
    category[category_size - 1] = '\0';
    return FALSE;
}

/**
 * @brief Look up template in cache
 * 
 * @param type_token Type token to look up
 * @param template_path Buffer to receive template path
 * @param path_size Size of template_path buffer
 * @return TRUE if found in cache, FALSE otherwise
 */
static BOOL lookup_template_cache(const char *type_token, char *template_path, int path_size)
{
    int i;
    
    if (!g_template_cache || !type_token)
        return FALSE;
    
    for (i = 0; i < g_template_cache_count; i++)
    {
        if (strcmp(g_template_cache[i].type_token, type_token) == 0)
        {
            /* Safety: reject cache entries with invalid/placeholder paths */
            if (g_template_cache[i].template_path[0] == '(' ||
                strcmp(g_template_cache[i].template_path, "(unresolved)") == 0)
            {
                log_debug(LOG_ICONS, "Template cache hit REJECTED (invalid path): %s → %s\n",
                         type_token, g_template_cache[i].template_path);
                return FALSE;  /* Force re-resolution */
            }
            
            /* Found - copy path and update hit count */
            strncpy(template_path, g_template_cache[i].template_path, path_size - 1);
            template_path[path_size - 1] = '\0';
            g_template_cache[i].hit_count++;
            
            log_debug(LOG_ICONS, "Template cache hit: %s → %s (hits: %lu)\n",
                     type_token, template_path, g_template_cache[i].hit_count);
            return TRUE;
        }
    }
    
    return FALSE;
}

/**
 * @brief Add entry to template cache
 * 
 * @param type_token Type token
 * @param template_path Template file path
 * @param category Root category name
 * @param is_fallback TRUE if fallback template
 */
static void add_to_template_cache(const char *type_token, const char *template_path, 
                                  const char *category, BOOL is_fallback)
{
    if (!type_token || !template_path)
        return;
    
    /* Check if cache is full */
    if (g_template_cache_count >= DEFICONS_MAX_TEMPLATE_CACHE)
    {
        log_debug(LOG_ICONS, "Template cache full - not adding %s\n", type_token);
        return;
    }
    
    /* Allocate or expand cache */
    if (g_template_cache == NULL)
    {
        g_template_cache = (TemplateCacheEntry *)whd_malloc(
            sizeof(TemplateCacheEntry) * DEFICONS_MAX_TEMPLATE_CACHE);
        if (!g_template_cache)
        {
            log_error(LOG_ICONS, "Failed to allocate template cache\n");
            return;
        }
    }
    
    /* Add entry */
    strncpy(g_template_cache[g_template_cache_count].type_token, 
            type_token, DEFICONS_MAX_TYPE_LEN - 1);
    g_template_cache[g_template_cache_count].type_token[DEFICONS_MAX_TYPE_LEN - 1] = '\0';
    
    strncpy(g_template_cache[g_template_cache_count].template_path, 
            template_path, DEFICONS_MAX_PATH_LEN - 1);
    g_template_cache[g_template_cache_count].template_path[DEFICONS_MAX_PATH_LEN - 1] = '\0';
    
    if (category)
    {
        strncpy(g_template_cache[g_template_cache_count].root_category, 
                category, DEFICONS_MAX_CATEGORY_LEN - 1);
        g_template_cache[g_template_cache_count].root_category[DEFICONS_MAX_CATEGORY_LEN - 1] = '\0';
    }
    else
    {
        strcpy(g_template_cache[g_template_cache_count].root_category, "unknown");
    }
    
    g_template_cache[g_template_cache_count].is_fallback = is_fallback;
    g_template_cache[g_template_cache_count].hit_count = 0;
    
    g_template_cache_count++;
    
    log_debug(LOG_ICONS, "Template cached: %s → %s (category: %s, fallback: %s)\n",
             type_token, template_path, category ? category : "unknown",
             is_fallback ? "yes" : "no");
}

/**
 * @brief Apply fallback template logic
 * 
 * @param template_path Buffer to receive fallback template path
 * @param path_size Size of template_path buffer
 * @return TRUE if fallback found, FALSE otherwise
 */
static BOOL apply_fallback_template(char *template_path, int path_size)
{
    /* Try def_project.info as universal fallback */
    if (find_template_file("project", template_path, path_size))
    {
        log_debug(LOG_ICONS, "Using fallback: def_project.info\n");
        return TRUE;
    }
    
    /* Try def_tool.info as secondary fallback */
    if (find_template_file("tool", template_path, path_size))
    {
        log_debug(LOG_ICONS, "Using fallback: def_tool.info\n");
        return TRUE;
    }
    
    log_warning(LOG_ICONS, "No fallback templates available\n");
    return FALSE;
}

/*========================================================================*/
/* Public Functions                                                      */
/*========================================================================*/

BOOL deficons_initialize_templates(void)
{
    int i;
    char test_path[DEFICONS_MAX_PATH_LEN];
    
    if (g_initialized)
    {
        log_warning(LOG_ICONS, "DefIcons templates already initialized\n");
        return TRUE;
    }
    
    log_info(LOG_ICONS, "Initializing DefIcons template system...\n");
    
    /* Check if DefIcons tree is available */
    if (!g_cached_deficons_tree || g_cached_deficons_count == 0)
    {
        log_error(LOG_ICONS, "DefIcons type tree not available (g_cached_deficons_tree is NULL)\n");
        log_error(LOG_ICONS, "This should have been initialized at startup in main_gui.c\n");
        return FALSE;
    }
    
    log_info(LOG_ICONS, "DefIcons tree available: %d type nodes\n", g_cached_deficons_count);
    
    /* Scan for available templates */
    g_available_templates_count = 0;
    
    /* Check common template types */
    const char *common_types[] = {
        "project", "tool", "drawer",
        "ascii", "picture", "music", "sound", "video",
        "archive", "c", "h", "asm", "rexx",
        NULL
    };
    
    for (i = 0; common_types[i] != NULL; i++)
    {
        if (find_template_file(common_types[i], test_path, sizeof(test_path)))
        {
            g_available_templates_count++;
            log_debug(LOG_ICONS, "Found template: %s → %s\n", common_types[i], test_path);
        }
    }
    
    if (g_available_templates_count == 0)
    {
        log_error(LOG_ICONS, "No DefIcons templates found!\n");
        log_error(LOG_ICONS, "Templates should be in: ENVARC:Sys/ (permanent storage)\n");
        log_error(LOG_ICONS, "  Which typically maps to: SYS:Prefs/Env-Archive/Sys/\n");
        log_error(LOG_ICONS, "\nChecked locations (in order):\n");
        for (i = 0; g_template_search_paths[i] != NULL; i++)
        {
            log_error(LOG_ICONS, "  [%d] %s\n", i + 1, g_template_search_paths[i]);
        }
        log_error(LOG_ICONS, "\nExpected files: def_project.info, def_tool.info, def_drawer.info, etc.\n");
        return FALSE;
    }
    
    log_info(LOG_ICONS, "Found %d common templates\n", g_available_templates_count);
    
    /* Initialize template cache */
    g_template_cache = NULL;
    g_template_cache_count = 0;
    
    g_initialized = TRUE;
    log_info(LOG_ICONS, "DefIcons template system initialization complete\n");
    
    return TRUE;
}

void deficons_cleanup_templates(void)
{
    if (!g_initialized)
        return;
    
    log_info(LOG_ICONS, "Cleaning up DefIcons template system...\n");
    
    /* Log cache statistics */
    if (g_template_cache && g_template_cache_count > 0)
    {
        int i;
        ULONG total_hits = 0;
        
        log_info(LOG_ICONS, "Template cache statistics:\n");
        for (i = 0; i < g_template_cache_count; i++)
        {
            log_info(LOG_ICONS, "  %s → %s (hits: %lu, fallback: %s)\n",
                    g_template_cache[i].type_token,
                    g_template_cache[i].template_path,
                    g_template_cache[i].hit_count,
                    g_template_cache[i].is_fallback ? "yes" : "no");
            total_hits += g_template_cache[i].hit_count;
        }
        log_info(LOG_ICONS, "Total cache entries: %d, total hits: %lu\n",
                g_template_cache_count, total_hits);
    }
    
    /* Free template cache */
    if (g_template_cache)
    {
        whd_free(g_template_cache);
        g_template_cache = NULL;
    }
    g_template_cache_count = 0;
    
    g_available_templates_count = 0;
    g_initialized = FALSE;
    
    log_info(LOG_ICONS, "DefIcons template system cleanup complete\n");
}

BOOL deficons_resolve_template(const char *type_token, char *template_path, int path_size)
{
    const char *current_type;
    char category[DEFICONS_MAX_CATEGORY_LEN];
    int depth;
    
    if (!g_initialized)
    {
        log_error(LOG_ICONS, "RESOLVE ERROR: DefIcons templates not initialized\n");
        return FALSE;
    }
    
    if (!type_token || !template_path || path_size < 2)
    {
        log_error(LOG_ICONS, "RESOLVE ERROR: Invalid parameters (token=%s, path=%p, size=%d)\n",
                 type_token ? type_token : "NULL", template_path, path_size);
        return FALSE;
    }
    
    log_info(LOG_ICONS, ">>> RESOLVE: Looking for template for type '%s'\n", type_token);
    
    /* Check cache first */
    if (lookup_template_cache(type_token, template_path, path_size))
    {
        log_info(LOG_ICONS, ">>> RESOLVE: Cache hit for '%s' → '%s'\n", type_token, template_path);
        return TRUE;  /* Cache hit */
    }
    
    log_info(LOG_ICONS, ">>> RESOLVE: Cache miss, trying direct match for '%s'\n", type_token);
    
    /* Cache miss - perform resolution */
    current_type = type_token;
    depth = 0;
    
    /* Try direct match first */
    if (find_template_file(current_type, template_path, path_size))
    {
        find_root_category(type_token, category, sizeof(category));
        add_to_template_cache(type_token, template_path, category, FALSE);
        log_info(LOG_ICONS, ">>> RESOLVE: Direct match found: %s → %s\n", type_token, template_path);
        return TRUE;
    }
    
    log_info(LOG_ICONS, ">>> RESOLVE: No direct match, walking parent chain...\n");
    
    /* Walk parent chain */
    while (depth < MAX_PARENT_DEPTH)
    {
        const char *parent;
        
        parent = get_parent_type(current_type);
        if (!parent)
        {
            log_info(LOG_ICONS, ">>> RESOLVE: No parent found for '%s' at depth %d\n", current_type, depth);
            break;
        }
        
        log_info(LOG_ICONS, ">>> RESOLVE: Trying parent: '%s' → '%s'\n", current_type, parent);
        
        /* Try parent template */
        if (find_template_file(parent, template_path, path_size))
        {
            find_root_category(type_token, category, sizeof(category));
            add_to_template_cache(type_token, template_path, category, FALSE);
            log_info(LOG_ICONS, ">>> RESOLVE: Parent chain success: %s → %s → %s\n",
                    type_token, parent, template_path);
            return TRUE;
        }
        
        current_type = parent;
        depth++;
    }
    
    log_info(LOG_ICONS, ">>> RESOLVE: Parent chain exhausted after %d attempts\n", depth);
    
    /* No match in type hierarchy - try fallback */
    if (apply_fallback_template(template_path, path_size))
    {
        add_to_template_cache(type_token, template_path, "fallback", TRUE);
        log_info(LOG_ICONS, "Fallback template used for: %s → %s\n", type_token, template_path);
        return TRUE;
    }
    
    log_warning(LOG_ICONS, "No template found for: %s\n", type_token);
    return FALSE;
}

const char* deficons_get_resolved_category(const char *type_token)
{
    int i;
    
    if (!g_initialized || !type_token)
        return NULL;
    
    /* Look up in cache */
    for (i = 0; i < g_template_cache_count; i++)
    {
        if (strcmp(g_template_cache[i].type_token, type_token) == 0)
        {
            return g_template_cache[i].root_category;
        }
    }
    
    /* Not in cache yet - resolve category without polluting the template cache.
     * CRITICAL: We must NOT add an "(unresolved)" entry to the template cache
     * here, because deficons_resolve_template() checks the same cache and would
     * return the bogus path as a cache hit, causing all icon copies to fail.
     * Instead, do a full template resolve which will populate the cache properly.
     */
    {
        char temp_path[DEFICONS_MAX_PATH_LEN];
        if (deficons_resolve_template(type_token, temp_path, sizeof(temp_path)))
        {
            /* Now the cache is populated with the real template path.
             * Look up the cache entry we just created to return the category.
             */
            int j;
            for (j = 0; j < g_template_cache_count; j++)
            {
                if (strcmp(g_template_cache[j].type_token, type_token) == 0)
                {
                    return g_template_cache[j].root_category;
                }
            }
        }
        else
        {
            /* Template resolution failed - try category lookup only */
            char category[DEFICONS_MAX_CATEGORY_LEN];
            if (find_root_category(type_token, category, sizeof(category)))
            {
                /* Return a static buffer for the category name */
                static char s_category_result[DEFICONS_MAX_CATEGORY_LEN];
                strncpy(s_category_result, category, DEFICONS_MAX_CATEGORY_LEN - 1);
                s_category_result[DEFICONS_MAX_CATEGORY_LEN - 1] = '\0';
                return s_category_result;
            }
        }
    }
    
    return NULL;
}

BOOL deficons_copy_icon_file(const char *source_path, const char *dest_path)
{
    BPTR src_file = 0;
    BPTR dest_file = 0;
    UBYTE *buffer = NULL;
    LONG bytes_read, bytes_written;
    BOOL success = FALSE;
    
    if (!source_path || !dest_path)
    {
        log_error(LOG_ICONS, "Invalid parameters to deficons_copy_icon_file\n");
        return FALSE;
    }
    
    /* Allocate copy buffer */
    buffer = (UBYTE *)AllocMem(COPY_BUFFER_SIZE, MEMF_PUBLIC);
    if (!buffer)
    {
        log_error(LOG_ICONS, "Failed to allocate copy buffer\n");
        return FALSE;
    }
    
    /* Open source file */
    src_file = Open((STRPTR)source_path, MODE_OLDFILE);
    if (!src_file)
    {
        log_error(LOG_ICONS, "Failed to open source: %s\n", source_path);
        FreeMem(buffer, COPY_BUFFER_SIZE);
        return FALSE;
    }
    
    /* Open destination file */
    dest_file = Open((STRPTR)dest_path, MODE_NEWFILE);
    if (!dest_file)
    {
        log_error(LOG_ICONS, "Failed to create destination: %s\n", dest_path);
        Close(src_file);
        FreeMem(buffer, COPY_BUFFER_SIZE);
        return FALSE;
    }
    
    /* Copy data */
    while ((bytes_read = Read(src_file, buffer, COPY_BUFFER_SIZE)) > 0)
    {
        bytes_written = Write(dest_file, buffer, bytes_read);
        if (bytes_written != bytes_read)
        {
            log_error(LOG_ICONS, "Write error copying icon: %s\n", dest_path);
            goto cleanup;
        }
    }
    
    if (bytes_read < 0)
    {
        log_error(LOG_ICONS, "Read error copying icon: %s\n", source_path);
        goto cleanup;
    }
    
    success = TRUE;
    log_debug(LOG_ICONS, "Icon copied: %s -> %s\n", source_path, dest_path);
    
cleanup:
    if (dest_file) Close(dest_file);
    if (src_file) Close(src_file);
    if (buffer) FreeMem(buffer, COPY_BUFFER_SIZE);
    
    return success;
}

BOOL deficons_template_exists(const char *type_token)
{
    char template_path[DEFICONS_MAX_PATH_LEN];
    
    if (!type_token)
        return FALSE;
    
    return find_template_file(type_token, template_path, sizeof(template_path));
}

const TemplateCacheEntry* deficons_get_template_cache_stats(int *count_out)
{
    if (!g_initialized || !count_out)
        return NULL;
    
    *count_out = g_template_cache_count;
    return g_template_cache;
}

void deficons_clear_template_cache(void)
{
    if (!g_initialized)
        return;
    
    log_info(LOG_ICONS, "Clearing template cache (%d entries)\n", g_template_cache_count);
    
    if (g_template_cache)
    {
        whd_free(g_template_cache);
        g_template_cache = NULL;
    }
    g_template_cache_count = 0;
}
