/*
 * deficons_identify.h - DefIcons File Type Identification via ARexx
 * 
 * Provides direct ARexx messaging to DefIcons port for file type identification.
 * Uses persistent connection to avoid overhead of repeated port operations.
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#ifndef DEFICONS_IDENTIFY_H
#define DEFICONS_IDENTIFY_H

#include <exec/types.h>

/*========================================================================*/
/* Extension Cache Configuration                                         */
/*========================================================================*/
#define DEFICONS_MAX_EXT_CACHE 64   /* Maximum cached file extensions */
#define DEFICONS_MAX_EXT_LEN   32   /* Maximum extension length */
#define DEFICONS_MAX_TOKEN_LEN 64   /* Maximum type token length */

/*========================================================================*/
/* Extension Cache Structure                                             */
/*========================================================================*/
/**
 * @brief Cache entry for file extension to type token mapping
 * 
 * Caches DefIcons Identify results by file extension to reduce
 * ARexx query overhead on large directories with many files of
 * the same type.
 */
typedef struct {
    char extension[DEFICONS_MAX_EXT_LEN];  /* File extension (e.g., ".mod") */
    char token[DEFICONS_MAX_TOKEN_LEN];     /* Resolved type token (e.g., "mod") */
    ULONG hit_count;                        /* Cache hit statistics */
} ExtensionCacheEntry;

/*========================================================================*/
/* Public Functions                                                      */
/*========================================================================*/

/**
 * @brief Initialize DefIcons ARexx communication
 * 
 * Finds the DEFICONS port, creates a persistent reply port for iTidy,
 * and initializes the extension cache.
 * 
 * Must be called before any other deficons_identify functions.
 * 
 * @return TRUE if DefIcons port found and initialization succeeded,
 *         FALSE if DefIcons not running or initialization failed
 * 
 * @note Logs to LOG_ICONS category
 * @see deficons_cleanup_arexx()
 */
BOOL deficons_initialize_arexx(void);

/**
 * @brief Cleanup DefIcons ARexx communication
 * 
 * Closes the reply port and frees the extension cache.
 * Safe to call even if initialization failed.
 * 
 * Should be called when icon creation is complete.
 * 
 * @see deficons_initialize_arexx()
 */
void deficons_cleanup_arexx(void);

/**
 * @brief Identify file type using DefIcons
 * 
 * Queries DefIcons port to identify the type of a file.
 * Results are cached by extension for performance.
 * 
 * @param filepath Full path to file to identify
 * @param type_token Buffer to receive type token (e.g., "mod", "ascii")
 * @param token_size Size of type_token buffer
 * 
 * @return TRUE if identification succeeded and token filled,
 *         FALSE if DefIcons query failed or timeout occurred
 * 
 * @note Must call deficons_initialize_arexx() first
 * @note Checks extension cache before querying DefIcons
 * @note Thread-safe (uses Forbid/Permit for cache access)
 */
BOOL deficons_identify_file(const char *filepath, char *type_token, int token_size);

/**
 * @brief Check if DefIcons is available
 * 
 * Tests whether the DEFICONS port is currently accessible.
 * Useful for checking availability without full initialization.
 * 
 * @return TRUE if DEFICONS port exists, FALSE otherwise
 * 
 * @note Uses Forbid/Permit to protect port search
 */
BOOL deficons_is_available(void);

/**
 * @brief Get extension cache statistics
 * 
 * Returns the current extension cache for debugging/statistics.
 * Caller must not modify or free the returned array.
 * 
 * @param count_out Pointer to receive cache entry count
 * @return Pointer to cache array (read-only), or NULL if not initialized
 * 
 * @note For debugging and statistics only
 */
const ExtensionCacheEntry* deficons_get_cache_stats(int *count_out);

/**
 * @brief Clear extension cache
 * 
 * Clears all cached extension-to-token mappings.
 * Useful if DefIcons configuration changes during runtime.
 * 
 * @note Rarely needed - cache is automatically managed
 */
void deficons_clear_cache(void);

#endif /* DEFICONS_IDENTIFY_H */
