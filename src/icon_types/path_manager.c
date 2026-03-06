/**
 * @file path_manager.c
 * @brief System PATH management implementation
 * 
 * This module manages the system PATH search list by parsing startup
 * scripts (S:Startup-Sequence and S:User-Startup) for PATH commands.
 * This supports tool validation without repeated disk access.
 */

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <proto/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>

#include "../itidy_types.h"
#include "../utilities.h"
#include "../writeLog.h"
#include "icon_types_internal.h"
#include "path_manager.h"

/*========================================================================*/
/* Global PATH Search List Variables                                      */
/*========================================================================*/

char **g_PathSearchList = NULL;
int g_PathSearchCount = 0;

/*========================================================================*/
/* Private Helper: Build Fallback PATH List                              */
/*========================================================================*/

/**
 * @brief Build hardcoded fallback PATH list
 * 
 * Creates a minimal PATH list with common Amiga system directories.
 * Used when spawning a shell to get PATH fails or when running from Workbench.
 * 
 * @return TRUE if fallback list created successfully, FALSE on error
 */
static BOOL BuildFallbackPathList(void)
{
    const char *fallback_paths[] = {
        "C:",
        "SYS:Utilities/",
        "SYS:Tools/",
        "SYS:System/",
        "S:",
        NULL  /* Terminator */
    };
    int count = 0;
    int i;
    
    /* Count entries */
    while (fallback_paths[count] != NULL)
    {
        count++;
    }
    
    log_info(LOG_GENERAL, "BuildFallbackPathList: Creating fallback PATH with %d entries\n", count);
    
    /* Allocate array for path strings */
    g_PathSearchList = (char **)whd_malloc(count * sizeof(char *));
    if (!g_PathSearchList)
    {
        log_error(LOG_GENERAL, "BuildFallbackPathList: Failed to allocate PATH array\n");
        return FALSE;
    }
    memset(g_PathSearchList, 0, count * sizeof(char *));
    
    /* Copy paths */
    for (i = 0; i < count; i++)
    {
        int len = strlen(fallback_paths[i]);
        g_PathSearchList[i] = (char *)whd_malloc(len + 1);
        if (!g_PathSearchList[i])
        {
            /* Cleanup on failure */
            int j;
            log_error(LOG_GENERAL, "BuildFallbackPathList: Failed to allocate string for path %d\n", i);
            for (j = 0; j < i; j++)
            {
                if (g_PathSearchList[j])
                {
                    whd_free(g_PathSearchList[j]);
                }
            }
            whd_free(g_PathSearchList);
            g_PathSearchList = NULL;
            g_PathSearchCount = 0;
            return FALSE;
        }
        strcpy(g_PathSearchList[i], fallback_paths[i]);
        log_info(LOG_GENERAL, "  [%d] %s\n", i + 1, fallback_paths[i]);
    }
    
    g_PathSearchCount = count;
    log_info(LOG_GENERAL, "BuildFallbackPathList: Fallback PATH list created with %d directories\n", 
             g_PathSearchCount);
    
    return TRUE;
}

/*========================================================================*/
/* Public: Build PATH Search List from Startup Scripts                   */
/*========================================================================*/

/**
 * @brief Build PATH search list by parsing startup scripts
 * 
 * This function obtains the system PATH by parsing S:Startup-Sequence and
 * S:User-Startup for "Path" commands. This approach works whether iTidy is
 * launched from CLI/Shell or Workbench.
 * 
 * The implementation:
 * 1. Parses S:Startup-Sequence and S:User-Startup line by line
 * 2. Extracts "Path <dirs>" (replace) and "Path <dirs> ADD" (append) commands
 * 3. Builds g_PathSearchList array with normalized paths
 * 4. Falls back to hardcoded paths if scripts unavailable
 * 
 * @return TRUE if PATH list was built successfully, FALSE on error
 */
BOOL BuildPathSearchList(void)
{
#if PLATFORM_AMIGA
    BPTR file = 0;
    char line[512];
    char *paths[100];  /* Temp storage for paths (max 100) */
    int path_count = 0;
    int i, j;
    const char *script_files[] = {
        "S:Startup-Sequence",
        "S:User-Startup",
        NULL
    };
    
    log_info(LOG_GENERAL, "BuildPathSearchList: Parsing startup scripts for PATH commands...\n");
    
    /* Process each startup script in order */
    for (i = 0; script_files[i] != NULL; i++)
    {
        const char *script_name = script_files[i];
        
        log_debug(LOG_GENERAL, "BuildPathSearchList: Opening %s\n", script_name);
        
        file = Open((CONST_STRPTR)script_name, MODE_OLDFILE);
        if (!file)
        {
            LONG error = IoErr();
            log_warning(LOG_GENERAL, "BuildPathSearchList: Could not open %s (IoErr=%ld), skipping\n", 
                     script_name, error);
            continue;  /* Try next file */
        }
        
        log_debug(LOG_GENERAL, "BuildPathSearchList: Parsing %s for PATH commands...\n", script_name);
        
        /* Read file line by line */
        while (FGets(file, line, sizeof(line) - 1))
        {
            char *trimmed = line;
            char *dir_start;
            char *token;
            BOOL is_add_command = FALSE;
            int len;
            
            /* Remove newline */
            len = strlen(line);
            if (len > 0 && line[len - 1] == '\n')
            {
                line[len - 1] = '\0';
                len--;
            }
            
            /* Trim leading whitespace */
            while (*trimmed == ' ' || *trimmed == '\t')
                trimmed++;
            
            /* Check if line starts with "Path " or "PATH " (case insensitive) */
            if (strncmp(trimmed, "Path ", 5) != 0 && strncmp(trimmed, "PATH ", 5) != 0 &&
                strncmp(trimmed, "path ", 5) != 0)
            {
                continue;  /* Not a PATH command */
            }
            
            log_debug(LOG_GENERAL, "  Found PATH command: '%s'\n", trimmed);
            
            /* Skip past "Path " */
            dir_start = trimmed + 5;
            
            /* Skip whitespace after "Path" */
            while (*dir_start == ' ' || *dir_start == '\t')
                dir_start++;
            
            /* Check if command ends with "ADD" */
            len = strlen(dir_start);
            if (len >= 3)
            {
                char *end = dir_start + len - 1;
                
                /* Trim trailing whitespace */
                while (end > dir_start && (*end == ' ' || *end == '\t' || *end == '\n'))
                {
                    *end = '\0';
                    end--;
                    len--;
                }
                
                /* Check for ADD at end */
                if (len >= 3)
                {
                    char *add_pos = dir_start + len - 3;
                    if (strncmp(add_pos, "ADD", 3) == 0 || strncmp(add_pos, "add", 3) == 0)
                    {
                        is_add_command = TRUE;
                        *add_pos = '\0';  /* Remove "ADD" from string */
                        
                        /* Trim whitespace before ADD */
                        add_pos--;
                        while (add_pos > dir_start && (*add_pos == ' ' || *add_pos == '\t'))
                        {
                            *add_pos = '\0';
                            add_pos--;
                        }
                    }
                }
            }
            
            /* If not ADD command, clear existing paths */
            if (!is_add_command)
            {
                log_debug(LOG_GENERAL, "  -> REPLACE mode: clearing %d existing paths\n", path_count);
                for (j = 0; j < path_count; j++)
                {
                    whd_free(paths[j]);
                }
                path_count = 0;
            }
            else
            {
                log_debug(LOG_GENERAL, "  -> ADD mode: appending to existing %d paths\n", path_count);
            }
            
            /* Parse directory list (space-separated) */
            token = strtok(dir_start, " \t");
            while (token != NULL && path_count < 100)
            {
                int token_len = strlen(token);
                
                if (token_len > 0)
                {
                    /* Ensure path ends with / or : */
                    char *path_copy;
                    if (token[token_len - 1] != '/' && token[token_len - 1] != ':')
                    {
                        path_copy = (char *)whd_malloc(token_len + 2);
                        if (path_copy)
                        {
                            strcpy(path_copy, token);
                            path_copy[token_len] = '/';
                            path_copy[token_len + 1] = '\0';
                        }
                    }
                    else
                    {
                        path_copy = (char *)whd_malloc(token_len + 1);
                        if (path_copy)
                        {
                            strcpy(path_copy, token);
                        }
                    }
                    
                    if (path_copy)
                    {
                        paths[path_count] = path_copy;
                        log_debug(LOG_GENERAL, "    Added path [%d]: '%s'\n", path_count + 1, path_copy);
                        path_count++;
                    }
                }
                
                token = strtok(NULL, " \t");
            }
        }
        
        Close(file);
        log_debug(LOG_GENERAL, "BuildPathSearchList: Finished parsing %s, current path count: %d\n", 
                 script_name, path_count);
    }
    
    /* Check if we got any paths */
    if (path_count == 0)
    {
        log_warning(LOG_GENERAL, "BuildPathSearchList: No PATH entries found in startup scripts, using fallback\n");
        return BuildFallbackPathList();
    }
    
    log_debug(LOG_GENERAL, "BuildPathSearchList: Allocating global array for %d paths\n", path_count);
    
    /* Allocate global array */
    g_PathSearchList = (char **)whd_malloc(path_count * sizeof(char *));
    if (!g_PathSearchList)
    {
        log_error(LOG_GENERAL, "BuildPathSearchList: Failed to allocate PATH array\n");
        /* Free temp paths */
        for (i = 0; i < path_count; i++)
        {
            whd_free(paths[i]);
        }
        return BuildFallbackPathList();
    }
    memset(g_PathSearchList, 0, path_count * sizeof(char *));
    
    /* Transfer paths to global array */
    log_debug(LOG_GENERAL, "BuildPathSearchList: Final PATH list:\n");
    for (i = 0; i < path_count; i++)
    {
        g_PathSearchList[i] = paths[i];
        log_debug(LOG_GENERAL, "  [%d] %s\n", i + 1, g_PathSearchList[i]);
    }
    
    g_PathSearchCount = path_count;
    
    log_info(LOG_GENERAL, "BuildPathSearchList: SUCCESS - Built PATH list with %d directories from startup scripts\n", 
             g_PathSearchCount);
    
    return TRUE;
#else
    /* Host platform stub */
    log_info(LOG_GENERAL, "BuildPathSearchList: Not supported on host platform\n");
    return FALSE;
#endif
}

/*========================================================================*/
/* Public: Free PATH Search List                                          */
/*========================================================================*/

/**
 * @brief Free the PATH search list
 * 
 * Releases all memory allocated by BuildPathSearchList().
 */
void FreePathSearchList(void)
{
#if PLATFORM_AMIGA
    int i;
    
    if (g_PathSearchList)
    {
        /* Free each path string */
        for (i = 0; i < g_PathSearchCount; i++)
        {
            if (g_PathSearchList[i])
            {
                whd_free(g_PathSearchList[i]);
            }
        }
        
        /* Free the array itself */
        whd_free(g_PathSearchList);
        
        g_PathSearchList = NULL;
        g_PathSearchCount = 0;
        
        log_debug(LOG_GENERAL, "FreePathSearchList: PATH list freed\n");
    }
#endif
}
