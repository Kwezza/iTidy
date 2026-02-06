/*
 * deficons_identify.c - DefIcons File Type Identification via ARexx
 * 
 * Implements direct ARexx messaging to DefIcons port for file type identification.
 * Based on working implementation from Tests/DefIcons/Test2/deficons_creator.c
 * 
 * Target: AmigaOS / Workbench 3.2+
 * Language: C89/C99 (VBCC)
 */

#include "deficons_identify.h"
#include "writeLog.h"
#include "platform/platform.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*========================================================================*/
/* Global State                                                          */
/*========================================================================*/

/* DefIcons port handle (found via FindPort) */
static struct MsgPort *g_deficons_port = NULL;

/* RexxSysBase library handle */
static struct RxsLib *g_RexxSysBase = NULL;

/* Extension cache for performance */
static ExtensionCacheEntry *g_extension_cache = NULL;
static int g_extension_cache_count = 0;

/* Initialization flag */
static BOOL g_initialized = FALSE;

/*========================================================================*/
/* Helper Functions                                                      */
/*========================================================================*/

/**
 * @brief Extract file extension from path
 * 
 * @param filepath Full file path
 * @param extension Buffer to receive extension (including dot)
 * @param ext_size Size of extension buffer
 * @return TRUE if extension found, FALSE otherwise
 */
static BOOL extract_extension(const char *filepath, char *extension, int ext_size)
{
    const char *dot_pos;
    const char *slash_pos;
    int ext_len;
    
    if (!filepath || !extension || ext_size < 2)
        return FALSE;
    
    /* Find last dot in path */
    dot_pos = strrchr(filepath, '.');
    if (!dot_pos)
    {
        extension[0] = '\0';
        return FALSE;
    }
    
    /* Make sure dot is after last slash (not in directory name) */
    slash_pos = strrchr(filepath, '/');
    if (slash_pos && slash_pos > dot_pos)
    {
        extension[0] = '\0';
        return FALSE;
    }
    
    /* Also check for colon (AmigaOS path separator) */
    slash_pos = strrchr(filepath, ':');
    if (slash_pos && slash_pos > dot_pos)
    {
        extension[0] = '\0';
        return FALSE;
    }
    
    /* Copy extension (including dot) and convert to lowercase */
    ext_len = strlen(dot_pos);
    if (ext_len >= ext_size)
        ext_len = ext_size - 1;
    
    strncpy(extension, dot_pos, ext_len);
    extension[ext_len] = '\0';
    
    /* Convert to lowercase for case-insensitive matching */
    {
        int i;
        for (i = 0; extension[i]; i++)
        {
            extension[i] = tolower((unsigned char)extension[i]);
        }
    }
    
    return TRUE;
}

/**
 * @brief Look up type token in extension cache
 * 
 * @param extension File extension (lowercase, with dot)
 * @param token Buffer to receive cached token
 * @param token_size Size of token buffer
 * @return TRUE if found in cache, FALSE otherwise
 */
static BOOL lookup_cache(const char *extension, char *token, int token_size)
{
    int i;
    
    if (!g_extension_cache || !extension)
        return FALSE;
    
    Forbid();  /* Protect cache access */
    
    for (i = 0; i < g_extension_cache_count; i++)
    {
        if (strcmp(g_extension_cache[i].extension, extension) == 0)
        {
            /* Found - copy token and update hit count */
            strncpy(token, g_extension_cache[i].token, token_size - 1);
            token[token_size - 1] = '\0';
            g_extension_cache[i].hit_count++;
            
            Permit();
            
            log_debug(LOG_ICONS, "Cache hit: %s → %s (hits: %lu)\n",
                     extension, token, g_extension_cache[i].hit_count);
            return TRUE;
        }
    }
    
    Permit();
    return FALSE;
}

/**
 * @brief Add entry to extension cache
 * 
 * @param extension File extension (lowercase, with dot)
 * @param token Type token to cache
 */
static void add_to_cache(const char *extension, const char *token)
{
    ExtensionCacheEntry *new_cache;
    
    if (!extension || !token)
        return;
    
    Forbid();  /* Protect cache access */
    
    /* Check if cache is full */
    if (g_extension_cache_count >= DEFICONS_MAX_EXT_CACHE)
    {
        Permit();
        log_debug(LOG_ICONS, "Extension cache full - not adding %s\n", extension);
        return;
    }
    
    /* Allocate or expand cache */
    if (g_extension_cache == NULL)
    {
        g_extension_cache = (ExtensionCacheEntry *)whd_malloc(
            sizeof(ExtensionCacheEntry) * DEFICONS_MAX_EXT_CACHE);
        if (!g_extension_cache)
        {
            Permit();
            log_error(LOG_ICONS, "Failed to allocate extension cache\n");
            return;
        }
    }
    
    /* Add entry */
    strncpy(g_extension_cache[g_extension_cache_count].extension, 
            extension, DEFICONS_MAX_EXT_LEN - 1);
    g_extension_cache[g_extension_cache_count].extension[DEFICONS_MAX_EXT_LEN - 1] = '\0';
    
    strncpy(g_extension_cache[g_extension_cache_count].token, 
            token, DEFICONS_MAX_TOKEN_LEN - 1);
    g_extension_cache[g_extension_cache_count].token[DEFICONS_MAX_TOKEN_LEN - 1] = '\0';
    
    g_extension_cache[g_extension_cache_count].hit_count = 0;
    
    g_extension_cache_count++;
    
    Permit();
    
    log_debug(LOG_ICONS, "Cached: %s → %s\n", extension, token);
}

/**
 * @brief Query DefIcons port via ARexx
 * 
 * @param filepath Full path to file
 * @param result Buffer to receive type token
 * @param result_size Size of result buffer
 * @return TRUE if query succeeded, FALSE otherwise
 */
static BOOL query_deficons_port(const char *filepath, char *result, int result_size)
{
    struct MsgPort *reply_port = NULL;
    struct RexxMsg *rxmsg = NULL;
    struct RexxMsg *reply = NULL;
    char command[512];
    BOOL success = FALSE;
    ULONG signal_mask;
    ULONG signals;
    
    if (!g_deficons_port || !g_RexxSysBase)
    {
        log_error(LOG_ICONS, "DefIcons port or RexxSysBase not available\n");
        return FALSE;
    }
    
    /* Create reply port */
    reply_port = CreateMsgPort();
    if (!reply_port)
    {
        log_error(LOG_ICONS, "Failed to create reply port\n");
        return FALSE;
    }
    
    /* Build Identify command */
    snprintf(command, sizeof(command), "Identify \"%s\"", filepath);
    
    /* Create REXX message */
    rxmsg = CreateRexxMsg(reply_port, NULL, NULL);
    if (!rxmsg)
    {
        log_error(LOG_ICONS, "Failed to create RexxMsg\n");
        DeleteMsgPort(reply_port);
        return FALSE;
    }
    
    /* Create argument string */
    rxmsg->rm_Args[0] = CreateArgstring(command, strlen(command));
    if (!rxmsg->rm_Args[0])
    {
        log_error(LOG_ICONS, "Failed to create argstring\n");
        DeleteRexxMsg(rxmsg);
        DeleteMsgPort(reply_port);
        return FALSE;
    }
    
    /* Set action flag to request result */
    rxmsg->rm_Action = RXFF_RESULT;
    
    /* Send message to DefIcons port */
    PutMsg(g_deficons_port, (struct Message *)rxmsg);
    
    /* Wait for reply with timeout (5 seconds) */
    signal_mask = 1L << reply_port->mp_SigBit;
    signals = Wait(signal_mask | SIGBREAKF_CTRL_C);
    
    /* Check for Ctrl-C */
    if (signals & SIGBREAKF_CTRL_C)
    {
        log_warning(LOG_ICONS, "DefIcons query cancelled by user\n");
        /* Message still in flight - can't safely delete yet */
        /* In production, should wait for reply before cleanup */
    }
    
    /* Get reply */
    reply = (struct RexxMsg *)GetMsg(reply_port);
    
    if (reply)
    {
        /* Check result code */
        if (reply->rm_Result1 == 0 && reply->rm_Result2)
        {
            /* Success - copy result string */
            strncpy(result, (char *)reply->rm_Result2, result_size - 1);
            result[result_size - 1] = '\0';
            success = TRUE;
            
            log_debug(LOG_ICONS, "DefIcons identified: %s → %s\n", filepath, result);
            
            /* Delete result string (allocated by DefIcons) */
            DeleteArgstring((STRPTR)reply->rm_Result2);
        }
        else
        {
            log_debug(LOG_ICONS, "DefIcons query failed: %s (rc=%ld)\n", 
                     filepath, reply->rm_Result1);
        }
    }
    else
    {
        log_warning(LOG_ICONS, "No reply from DefIcons (timeout?)\n");
    }
    
    /* Cleanup */
    if (rxmsg->rm_Args[0])
        DeleteArgstring(rxmsg->rm_Args[0]);
    DeleteRexxMsg(rxmsg);
    DeleteMsgPort(reply_port);
    
    return success;
}

/*========================================================================*/
/* Public Functions                                                      */
/*========================================================================*/

BOOL deficons_initialize_arexx(void)
{
    if (g_initialized)
    {
        log_warning(LOG_ICONS, "DefIcons ARexx already initialized\n");
        return TRUE;
    }
    
    log_info(LOG_ICONS, "Initializing DefIcons ARexx communication...\n");
    
    /* Open rexxsyslib.library */
    g_RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 0);
    if (!g_RexxSysBase)
    {
        log_error(LOG_ICONS, "Failed to open rexxsyslib.library\n");
        return FALSE;
    }
    
    /* Find DefIcons port */
    Forbid();
    g_deficons_port = FindPort("DEFICONS");
    Permit();
    
    if (!g_deficons_port)
    {
        log_warning(LOG_ICONS, "DEFICONS port not found - DefIcons not running\n");
        CloseLibrary((struct Library *)g_RexxSysBase);
        g_RexxSysBase = NULL;
        return FALSE;
    }
    
    log_info(LOG_ICONS, "Found DEFICONS port at 0x%08lx\n", (ULONG)g_deficons_port);
    
    /* Initialize extension cache */
    g_extension_cache = NULL;
    g_extension_cache_count = 0;
    
    g_initialized = TRUE;
    log_info(LOG_ICONS, "DefIcons ARexx initialization complete\n");
    
    return TRUE;
}

void deficons_cleanup_arexx(void)
{
    if (!g_initialized)
        return;
    
    log_info(LOG_ICONS, "Cleaning up DefIcons ARexx communication...\n");
    
    /* Log cache statistics */
    if (g_extension_cache && g_extension_cache_count > 0)
    {
        int i;
        ULONG total_hits = 0;
        
        log_info(LOG_ICONS, "Extension cache statistics:\n");
        for (i = 0; i < g_extension_cache_count; i++)
        {
            log_info(LOG_ICONS, "  %s → %s (hits: %lu)\n",
                    g_extension_cache[i].extension,
                    g_extension_cache[i].token,
                    g_extension_cache[i].hit_count);
            total_hits += g_extension_cache[i].hit_count;
        }
        log_info(LOG_ICONS, "Total cache entries: %d, total hits: %lu\n",
                g_extension_cache_count, total_hits);
    }
    
    /* Free extension cache */
    if (g_extension_cache)
    {
        whd_free(g_extension_cache);
        g_extension_cache = NULL;
    }
    g_extension_cache_count = 0;
    
    /* Close rexxsyslib.library */
    if (g_RexxSysBase)
    {
        CloseLibrary((struct Library *)g_RexxSysBase);
        g_RexxSysBase = NULL;
    }
    
    /* Note: We don't close g_deficons_port as we didn't create it */
    g_deficons_port = NULL;
    
    g_initialized = FALSE;
    log_info(LOG_ICONS, "DefIcons ARexx cleanup complete\n");
}

BOOL deficons_identify_file(const char *filepath, char *type_token, int token_size)
{
    char extension[DEFICONS_MAX_EXT_LEN];
    
    if (!g_initialized)
    {
        log_error(LOG_ICONS, "DefIcons ARexx not initialized\n");
        return FALSE;
    }
    
    if (!filepath || !type_token || token_size < 2)
    {
        log_error(LOG_ICONS, "Invalid parameters to deficons_identify_file\n");
        return FALSE;
    }
    
    /* Try cache first if file has extension */
    if (extract_extension(filepath, extension, sizeof(extension)))
    {
        if (lookup_cache(extension, type_token, token_size))
        {
            return TRUE;  /* Cache hit */
        }
    }
    
    /* Cache miss or no extension - query DefIcons */
    if (query_deficons_port(filepath, type_token, token_size))
    {
        /* Add to cache if we have an extension */
        if (extension[0] != '\0')
        {
            add_to_cache(extension, type_token);
        }
        return TRUE;
    }
    
    return FALSE;
}

BOOL deficons_is_available(void)
{
    struct MsgPort *port;
    
    Forbid();
    port = FindPort("DEFICONS");
    Permit();
    
    return (port != NULL);
}

const ExtensionCacheEntry* deficons_get_cache_stats(int *count_out)
{
    if (!g_initialized || !count_out)
        return NULL;
    
    *count_out = g_extension_cache_count;
    return g_extension_cache;
}

void deficons_clear_cache(void)
{
    if (!g_initialized)
        return;
    
    log_info(LOG_ICONS, "Clearing extension cache (%d entries)\n", g_extension_cache_count);
    
    if (g_extension_cache)
    {
        whd_free(g_extension_cache);
        g_extension_cache = NULL;
    }
    g_extension_cache_count = 0;
}
