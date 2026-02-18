/*
 * deficons_templates.h - DefIcons Template Resolution and Icon Copying
 * 
 * Provides template scanning, parent-chain resolution, and icon file copying
 * for the DefIcons icon creation system.
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#ifndef DEFICONS_TEMPLATES_H
#define DEFICONS_TEMPLATES_H

#include <exec/types.h>

/*========================================================================*/
/* Template Cache Configuration                                          */
/*========================================================================*/
#define DEFICONS_MAX_TEMPLATE_CACHE 128  /* Maximum cached templates */
#define DEFICONS_MAX_TYPE_LEN       64   /* Maximum type token length */
#define DEFICONS_MAX_PATH_LEN       256  /* Maximum template path length */
#define DEFICONS_MAX_CATEGORY_LEN   64   /* Maximum category name length */

/*========================================================================*/
/* Template Cache Structure                                              */
/*========================================================================*/
/**
 * @brief Cache entry for type token to template path mapping
 * 
 * Caches template resolution results including the full path to the
 * template file and the root category for filtering purposes.
 */
typedef struct {
    char type_token[DEFICONS_MAX_TYPE_LEN];      /* Type name (e.g., "music") */
    char template_path[DEFICONS_MAX_PATH_LEN];   /* Full path to template */
    char root_category[DEFICONS_MAX_CATEGORY_LEN]; /* Root category for filtering */
    BOOL is_fallback;                            /* TRUE if fallback template */
    ULONG hit_count;                             /* Cache hit statistics */
} TemplateCacheEntry;

/*========================================================================*/
/* Public Functions                                                      */
/*========================================================================*/

/**
 * @brief Initialize DefIcons template system
 * 
 * Scans ENV:Sys/ and ENVARC:Sys/ for available def_<type>.info templates
 * and initializes the template cache.
 * 
 * Must be called before any other deficons_templates functions.
 * Requires g_cached_deficons_tree to be initialized (from main_gui.c).
 * 
 * @return TRUE if at least one template found and initialized successfully,
 *         FALSE if no templates available or initialization failed
 * 
 * @note Logs to LOG_ICONS category
 * @see deficons_cleanup_templates()
 */
BOOL deficons_initialize_templates(void);

/**
 * @brief Cleanup DefIcons template system
 * 
 * Frees the template cache and releases resources.
 * Safe to call even if initialization failed.
 * 
 * Should be called when icon creation is complete.
 * 
 * @see deficons_initialize_templates()
 */
void deficons_cleanup_templates(void);

/**
 * @brief Resolve type token to template file path
 * 
 * Finds the appropriate template file for a given type token.
 * Uses parent-chain walking if exact match not found.
 * Results are cached for performance.
 * 
 * Resolution algorithm:
 * 1. Try direct match: def_<token>.info
 * 2. Walk parent chain using g_cached_deficons_tree
 * 3. Apply fallback logic if no match found
 * 
 * @param type_token Type token from DefIcons (e.g., "mod", "ascii")
 * @param template_path Buffer to receive full path to template
 * @param path_size Size of template_path buffer
 * 
 * @return TRUE if template found and path filled,
 *         FALSE if no suitable template available
 * 
 * @note Must call deficons_initialize_templates() first
 * @note Uses g_cached_deficons_tree for parent lookups
 * @note Includes cycle detection to prevent infinite loops
 */
BOOL deficons_resolve_template(const char *type_token, char *template_path, int path_size);

/**
 * @brief Get root category for a type token
 * 
 * Returns the root category name for a given type token.
 * Used by filtering logic to check if type is enabled.
 * 
 * @param type_token Type token to look up
 * @return Root category name, or NULL if not found
 * 
 * @note Must call deficons_initialize_templates() first
 * @note Returned string is static - do not free
 */
const char* deficons_get_resolved_category(const char *type_token);

/**
 * @brief Copy icon file from template to destination
 * 
 * Copies a .info template file to create a new icon.
 * Uses efficient buffered copying with 8KB buffer.
 * 
 * @param source_path Full path to template .info file
 * @param dest_path Full path to destination .info file
 * 
 * @return TRUE if copy succeeded,
 *         FALSE if read/write error occurred
 * 
 * @note Creates destination file with same protection bits as source
 * @note Logs errors to LOG_ICONS category
 */
BOOL deficons_copy_icon_file(const char *source_path, const char *dest_path);

/**
 * @brief Check if a template file exists
 * 
 * Tests whether a specific template file is available.
 * 
 * @param type_token Type token to check (e.g., "music")
 * @return TRUE if def_<type>.info exists in ENV: or ENVARC:, FALSE otherwise
 * 
 * @note Does not cache result - for testing only
 */
BOOL deficons_template_exists(const char *type_token);

/**
 * @brief Get template cache statistics
 * 
 * Returns the current template cache for debugging/statistics.
 * Caller must not modify or free the returned array.
 * 
 * @param count_out Pointer to receive cache entry count
 * @return Pointer to cache array (read-only), or NULL if not initialized
 * 
 * @note For debugging and statistics only
 */
const TemplateCacheEntry* deficons_get_template_cache_stats(int *count_out);

/**
 * @brief Clear template cache
 * 
 * Clears all cached template resolutions.
 * Useful if template files change during runtime.
 * 
 * @note Rarely needed - cache is automatically managed
 */
void deficons_clear_template_cache(void);

/**
 * @brief Get list of all ASCII sub-type tokens
 *
 * Returns a NULL-terminated array of ASCII child type tokens from
 * g_cached_deficons_tree. The first entry is always "ascii" itself.
 * Remaining entries are direct/indirect ASCII children, in tree order.
 *
 * @param count_out If non-NULL, receives the number of entries (excluding
 *                  the terminating NULL pointer).
 *
 * @return Pointer to a static NULL-terminated array of string pointers.
 *         Contents are valid as long as g_cached_deficons_tree is valid.
 *         Never returns NULL (an empty list is returned if tree not found).
 *
 * @note Do NOT free or modify the returned array or its strings.
 * @note Requires g_cached_deficons_tree to be initialised (from main_gui.c).
 */
const char **deficons_get_ascii_subtypes(int *count_out);

#endif /* DEFICONS_TEMPLATES_H */
