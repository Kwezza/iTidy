/**
 * default_tool_backup.c - Default Tool Backup and Restore Implementation
 * 
 * Implements lightweight CSV-based backup/restore for default tool changes.
 * Optimized for scattered icon files across the disk.
 */

#include "platform/platform.h"
#include "default_tool_backup.h"
#include "path_utilities.h"
#include "writeLog.h"

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/*------------------------------------------------------------------------*/
/* Internal Helper Functions                                              */
/*------------------------------------------------------------------------*/

/**
 * @brief Generate session ID from current timestamp
 * 
 * Creates a session ID in format YYYYMMDD_HHMMSS
 * 
 * @param session_id Buffer to store session ID (at least 32 bytes)
 */
static void generate_session_id(char *session_id)
{
    struct DateStamp ds;
    ULONG days, mins, ticks;
    ULONG year, month, day, hour, minute, second;
    
    /* Get current timestamp */
    DateStamp(&ds);
    
    days = ds.ds_Days;      /* Days since 1-Jan-1978 */
    mins = ds.ds_Minute;    /* Minutes since midnight */
    ticks = ds.ds_Tick;     /* Ticks (1/50 sec) since start of minute */
    
    /* Convert days to date (simplified calculation) */
    /* Base: 1-Jan-1978 */
    year = 1978 + (days / 365);  /* Approximate year */
    day = (days % 365) + 1;      /* Day of year (approximate) */
    month = (day / 31) + 1;      /* Approximate month */
    if (month > 12) month = 12;
    day = day % 31;
    if (day == 0) day = 1;
    
    /* Convert minutes to time */
    hour = mins / 60;
    minute = mins % 60;
    second = ticks / 50;  /* 50 ticks per second */
    
    sprintf(session_id, "%04lu%02lu%02lu_%02lu%02lu%02lu",
            year, month, day, hour, minute, second);
}

/**
 * @brief Create directory if it doesn't exist
 * 
 * @param path Directory path to create
 * @return TRUE if directory exists or was created, FALSE on error
 */
static BOOL ensure_directory_exists(const char *path)
{
    BPTR lock;
    
    /* Check if directory already exists */
    lock = Lock((CONST_STRPTR)path, ACCESS_READ);
    if (lock)
    {
        UnLock(lock);
        return TRUE;
    }
    
    /* Try to create directory */
    lock = CreateDir((CONST_STRPTR)path);
    if (lock)
    {
        UnLock(lock);
        return TRUE;
    }
    
    return FALSE;
}

/**
 * @brief Escape CSV field (wrap in quotes if contains comma or quote)
 * 
 * @param input Input string
 * @param output Output buffer (must be at least 2x input size + 2)
 */
static void escape_csv_field(const char *input, char *output)
{
    int i = 0;
    int j = 0;
    BOOL needs_quotes = FALSE;
    
    if (input == NULL)
    {
        output[0] = '\0';
        return;
    }
    
    /* Check if field needs quoting */
    if (strchr(input, ',') != NULL || strchr(input, '"') != NULL)
        needs_quotes = TRUE;
    
    if (needs_quotes)
        output[j++] = '"';
    
    /* Copy and escape quotes */
    while (input[i] != '\0')
    {
        if (input[i] == '"')
        {
            output[j++] = '"';  /* Double quotes to escape */
            output[j++] = '"';
        }
        else
        {
            output[j++] = input[i];
        }
        i++;
    }
    
    if (needs_quotes)
        output[j++] = '"';
    
    output[j] = '\0';
}

/**
 * @brief Unescape CSV field (remove quotes and unescape doubled quotes)
 * 
 * @param input Input string (may be quoted)
 * @param output Output buffer (same size as input is safe)
 */
static void unescape_csv_field(const char *input, char *output)
{
    int i = 0;
    int j = 0;
    BOOL in_quotes = FALSE;
    
    if (input == NULL)
    {
        output[0] = '\0';
        return;
    }
    
    /* Check if field is quoted */
    if (input[0] == '"')
    {
        in_quotes = TRUE;
        i = 1;  /* Skip opening quote */
    }
    
    /* Copy and unescape */
    while (input[i] != '\0')
    {
        if (in_quotes && input[i] == '"')
        {
            if (input[i + 1] == '"')
            {
                /* Doubled quote - output single quote */
                output[j++] = '"';
                i += 2;
            }
            else
            {
                /* Closing quote - stop */
                break;
            }
        }
        else
        {
            output[j++] = input[i++];
        }
    }
    
    output[j] = '\0';
}

/*------------------------------------------------------------------------*/
/* Backup Operations                                                      */
/*------------------------------------------------------------------------*/

BOOL iTidy_InitToolBackupManager(iTidy_ToolBackupManager *manager, BOOL enabled)
{
    if (manager == NULL)
        return FALSE;
    
    memset(manager, 0, sizeof(iTidy_ToolBackupManager));
    manager->backup_enabled = enabled;
    
    if (!enabled)
    {
        append_to_log("Default tool backup disabled by preferences\n");
        return TRUE;  /* Success, just disabled */
    }
    
    /* Create root backup directory */
    if (!ensure_directory_exists(TOOL_BACKUP_ROOT_PATH))
    {
        append_to_log("ERROR: Could not create backup directory: %s\n", 
                     TOOL_BACKUP_ROOT_PATH);
        manager->backup_enabled = FALSE;
        return FALSE;
    }
    
    append_to_log("Default tool backup manager initialized\n");
    return TRUE;
}

BOOL iTidy_StartBackupSession(iTidy_ToolBackupManager *manager,
                               const char *mode,
                               const char *scanned_path)
{
    char session_file_path[256];
    char changes_file_path[256];
    char header[512];
    
    if (manager == NULL || !manager->backup_enabled)
        return FALSE;
    
    if (manager->session_active)
    {
        append_to_log("ERROR: Backup session already active\n");
        return FALSE;
    }
    
    /* Generate session ID */
    generate_session_id(manager->session_id);
    
    /* Build session path */
    sprintf(manager->session_path, "%s/%s", 
            TOOL_BACKUP_ROOT_PATH, manager->session_id);
    
    /* Create session directory */
    if (!ensure_directory_exists(manager->session_path))
    {
        append_to_log("ERROR: Could not create session directory: %s\n",
                     manager->session_path);
        return FALSE;
    }
    
    /* Open session.txt */
    sprintf(session_file_path, "%s/%s", 
            manager->session_path, TOOL_BACKUP_SESSION_FILE);
    manager->session_file = Open((CONST_STRPTR)session_file_path, MODE_NEWFILE);
    if (manager->session_file == 0)
    {
        append_to_log("ERROR: Could not create session file: %s\n", 
                     session_file_path);
        return FALSE;
    }
    
    /* Write initial session metadata */
    sprintf(header, "Session: Default Tool Update\nDate: %s\nMode: %s\nScanned: %s\n",
            manager->session_id, mode, scanned_path);
    Write(manager->session_file, header, strlen(header));
    
    /* Open changes.csv */
    sprintf(changes_file_path, "%s/%s",
            manager->session_path, TOOL_BACKUP_CHANGES_FILE);
    manager->changes_file = Open((CONST_STRPTR)changes_file_path, MODE_NEWFILE);
    if (manager->changes_file == 0)
    {
        append_to_log("ERROR: Could not create changes file: %s\n",
                     changes_file_path);
        Close(manager->session_file);
        manager->session_file = 0;
        return FALSE;
    }
    
    /* Write CSV header */
    sprintf(header, "# iTidy Default Tool Backup - %s\n", manager->session_id);
    Write(manager->changes_file, header, strlen(header));
    
    manager->session_active = TRUE;
    manager->icons_changed = 0;
    manager->icons_skipped = 0;
    
    append_to_log("Backup session started: %s (%s mode)\n", 
                 manager->session_id, mode);
    
    return TRUE;
}

BOOL iTidy_RecordToolChange(iTidy_ToolBackupManager *manager,
                             const char *icon_path,
                             const char *old_tool,
                             const char *new_tool)
{
    char line[1024];
    char escaped_path[512];
    char escaped_old[512];
    char escaped_new[512];
    
    if (manager == NULL || !manager->session_active)
        return FALSE;
    
    /* Escape CSV fields */
    escape_csv_field(icon_path, escaped_path);
    escape_csv_field(old_tool ? old_tool : "", escaped_old);
    escape_csv_field(new_tool ? new_tool : "", escaped_new);
    
    /* Format CSV line */
    sprintf(line, "%s,%s,%s\n", escaped_path, escaped_old, escaped_new);
    
    /* Write to file */
    if (Write(manager->changes_file, line, strlen(line)) < 0)
    {
        append_to_log("ERROR: Failed to write change record\n");
        return FALSE;
    }
    
    manager->icons_changed++;
    return TRUE;
}

void iTidy_EndBackupSession(iTidy_ToolBackupManager *manager)
{
    char footer[256];
    
    if (manager == NULL || !manager->session_active)
        return;
    
    /* Write final statistics to session.txt */
    sprintf(footer, "Icons changed: %d\nIcons skipped: %d\nTotal processed: %d\n",
            manager->icons_changed, manager->icons_skipped,
            manager->icons_changed + manager->icons_skipped);
    Write(manager->session_file, footer, strlen(footer));
    
    /* Close files */
    if (manager->session_file)
    {
        Close(manager->session_file);
        manager->session_file = 0;
    }
    
    if (manager->changes_file)
    {
        Close(manager->changes_file);
        manager->changes_file = 0;
    }
    
    append_to_log("Backup session ended: %s (%d changed, %d skipped)\n",
                 manager->session_id, manager->icons_changed, manager->icons_skipped);
    
    manager->session_active = FALSE;
}

void iTidy_CleanupToolBackupManager(iTidy_ToolBackupManager *manager)
{
    if (manager == NULL)
        return;
    
    /* End session if still active */
    if (manager->session_active)
        iTidy_EndBackupSession(manager);
    
    memset(manager, 0, sizeof(iTidy_ToolBackupManager));
}

/*------------------------------------------------------------------------*/
/* Restore Operations - Session Scanning                                 */
/*------------------------------------------------------------------------*/

UWORD iTidy_ScanBackupSessions(struct List *session_list)
{
    BPTR lock;
    struct FileInfoBlock *fib = NULL;
    iTidy_ToolBackupSession *session;
    UWORD count = 0;
    char session_file_path[256];
    BPTR file;
    char line[256];
    char shortened_path[64];
    
    if (session_list == NULL)
        return 0;
    
    /* Lock backup root directory */
    lock = Lock((CONST_STRPTR)TOOL_BACKUP_ROOT_PATH, ACCESS_READ);
    if (!lock)
    {
        append_to_log("WARNING: Backup directory not found: %s\n", 
                     TOOL_BACKUP_ROOT_PATH);
        return 0;
    }
    
    /* Allocate FileInfoBlock */
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        UnLock(lock);
        return 0;
    }
    
    /* Scan directory for session folders */
    if (Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            /* Skip files, only process directories */
            if (fib->fib_DirEntryType < 0)
                continue;
            
            /* Allocate session entry */
            session = (iTidy_ToolBackupSession *)whd_malloc(sizeof(iTidy_ToolBackupSession));
            if (!session)
                continue;
            
            memset(session, 0, sizeof(iTidy_ToolBackupSession));
            
            /* Copy session ID */
            strncpy(session->session_id, fib->fib_FileName, 31);
            session->session_id[31] = '\0';
            
            /* Read session.txt for metadata */
            sprintf(session_file_path, "%s/%s/%s",
                    TOOL_BACKUP_ROOT_PATH, session->session_id, 
                    TOOL_BACKUP_SESSION_FILE);
            
            file = Open((CONST_STRPTR)session_file_path, MODE_OLDFILE);
            if (file)
            {
                /* Parse session.txt line by line */
                while (FGets(file, line, sizeof(line)))
                {
                    if (strncmp(line, "Date: ", 6) == 0)
                    {
                        strncpy(session->date_string, line + 6, 31);
                        /* Remove newline */
                        session->date_string[strcspn(session->date_string, "\n")] = '\0';
                    }
                    else if (strncmp(line, "Mode: ", 6) == 0)
                    {
                        strncpy(session->mode, line + 6, 15);
                        session->mode[strcspn(session->mode, "\n")] = '\0';
                    }
                    else if (strncmp(line, "Scanned: ", 9) == 0)
                    {
                        strncpy(session->scanned_path, line + 9, 107);
                        session->scanned_path[strcspn(session->scanned_path, "\n")] = '\0';
                    }
                    else if (strncmp(line, "Icons changed: ", 15) == 0)
                    {
                        sscanf(line + 15, "%hu", &session->icons_changed);
                    }
                    else if (strncmp(line, "Icons skipped: ", 15) == 0)
                    {
                        sscanf(line + 15, "%hu", &session->icons_skipped);
                    }
                    else if (strncmp(line, "Total processed: ", 17) == 0)
                    {
                        sscanf(line + 17, "%hu", &session->icons_total);
                    }
                }
                Close(file);
            }
            
            /* Shorten path for display using iTidy_ShortenPathWithParentDir */
            iTidy_ShortenPathWithParentDir(session->scanned_path, shortened_path, 40);
            
            /* Format display text for ListView with fixed-width columns */
            /* Columns: Date/Time (20) | Mode (6) | Path (46) | Changed (11) */
            session->display_text = (char *)whd_malloc(256);
            if (session->display_text)
            {
                sprintf(session->display_text, "%-20s | %-6s | %-46s | %d changed",
                        session->date_string,
                        session->mode,
                        shortened_path,
                        session->icons_changed);
                
                session->node.ln_Name = session->display_text;
            }
            
            /* Add to list */
            AddTail(session_list, (struct Node *)session);
            count++;
        }
    }
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    append_to_log("Found %d backup sessions\n", count);
    return count;
}

void iTidy_FreeSessionList(struct List *session_list)
{
    iTidy_ToolBackupSession *session;
    struct Node *node;
    
    if (session_list == NULL)
        return;
    
    while ((node = RemHead(session_list)) != NULL)
    {
        session = (iTidy_ToolBackupSession *)node;
        
        if (session->display_text)
            whd_free(session->display_text);
        
        whd_free(session);
    }
}

/*------------------------------------------------------------------------*/
/* Restore Operations - Tool Change Loading                              */
/*------------------------------------------------------------------------*/

UWORD iTidy_LoadToolChanges(const char *session_id, struct List *tool_change_list)
{
    char changes_file_path[256];
    BPTR file;
    char line[1024];
    char icon_path[TOOL_BACKUP_MAX_PATH_LEN];
    char old_tool[TOOL_BACKUP_MAX_TOOL_LEN];
    char new_tool[TOOL_BACKUP_MAX_TOOL_LEN];
    iTidy_ToolChange *change;
    struct Node *node;
    BOOL found;
    UWORD count = 0;
    char *comma1, *comma2;
    
    if (session_id == NULL || tool_change_list == NULL)
        return 0;
    
    /* Build path to changes.csv */
    sprintf(changes_file_path, "%s/%s/%s",
            TOOL_BACKUP_ROOT_PATH, session_id, TOOL_BACKUP_CHANGES_FILE);
    
    /* Open changes.csv */
    file = Open((CONST_STRPTR)changes_file_path, MODE_OLDFILE);
    if (!file)
    {
        append_to_log("ERROR: Could not open changes file: %s\n", changes_file_path);
        return 0;
    }
    
    /* Parse CSV line by line */
    while (FGets(file, line, sizeof(line)))
    {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
            continue;
        
        /* Parse CSV: icon_path,old_tool,new_tool */
        comma1 = strchr(line, ',');
        if (!comma1)
            continue;
        
        *comma1 = '\0';
        comma2 = strchr(comma1 + 1, ',');
        if (!comma2)
            continue;
        
        *comma2 = '\0';
        
        /* Unescape fields */
        unescape_csv_field(line, icon_path);
        unescape_csv_field(comma1 + 1, old_tool);
        unescape_csv_field(comma2 + 1, new_tool);
        
        /* Remove trailing newline from new_tool */
        new_tool[strcspn(new_tool, "\n")] = '\0';
        
        /* Find or create tool change entry */
        found = FALSE;
        for (node = tool_change_list->lh_Head; 
             node->ln_Succ != NULL; 
             node = node->ln_Succ)
        {
            change = (iTidy_ToolChange *)node;
            
            if (strcmp(change->old_tool, old_tool) == 0 &&
                strcmp(change->new_tool, new_tool) == 0)
            {
                /* Found existing group - increment count */
                change->icon_count++;
                found = TRUE;
                break;
            }
        }
        
        if (!found)
        {
            /* Create new tool change entry */
            change = (iTidy_ToolChange *)whd_malloc(sizeof(iTidy_ToolChange));
            if (change)
            {
                memset(change, 0, sizeof(iTidy_ToolChange));
                
                strncpy(change->old_tool, old_tool, TOOL_BACKUP_MAX_TOOL_LEN - 1);
                strncpy(change->new_tool, new_tool, TOOL_BACKUP_MAX_TOOL_LEN - 1);
                change->icon_count = 1;
                
                /* Format display text (needs space for two tool paths + formatting) */
                change->display_text = (char *)whd_malloc(256);
                if (change->display_text)
                {
                    sprintf(change->display_text, "%s -> %s | %d icon%s",
                            old_tool[0] ? old_tool : "(none)",
                            new_tool[0] ? new_tool : "(none)",
                            change->icon_count,
                            change->icon_count == 1 ? "" : "s");
                    
                    change->node.ln_Name = change->display_text;
                    
                    log_message(LOG_BACKUP, LOG_LEVEL_DEBUG, 
                                "NEW CHANGE: old='%s' new='%s' display='%s' ln_Name='%s'",
                                change->old_tool, change->new_tool, 
                                change->display_text, change->node.ln_Name);
                }
                
                AddTail(tool_change_list, (struct Node *)change);
                count++;
            }
        }
        else
        {
            /* Update display text with new count */
            if (change->display_text)
            {
                whd_free(change->display_text);
            }
            
            /* Reallocate and format display text */
            change->display_text = (char *)whd_malloc(256);
            if (change->display_text)
            {
                sprintf(change->display_text, "%s -> %s | %d icon%s",
                        change->old_tool[0] ? change->old_tool : "(none)",
                        change->new_tool[0] ? change->new_tool : "(none)",
                        change->icon_count,
                        change->icon_count == 1 ? "" : "s");
                
                change->node.ln_Name = change->display_text;
                
                log_message(LOG_BACKUP, LOG_LEVEL_DEBUG,
                            "UPDATE CHANGE: old='%s' new='%s' count=%d display='%s' ln_Name='%s'",
                            change->old_tool, change->new_tool, change->icon_count,
                            change->display_text, change->node.ln_Name);
            }
        }
    }
    
    Close(file);
    
    append_to_log("Loaded %d unique tool changes from session %s\n", count, session_id);
    return count;
}

void iTidy_FreeToolChangeList(struct List *tool_change_list)
{
    iTidy_ToolChange *change;
    struct Node *node;
    
    if (tool_change_list == NULL)
        return;
    
    while ((node = RemHead(tool_change_list)) != NULL)
    {
        change = (iTidy_ToolChange *)node;
        
        if (change->display_text)
            whd_free(change->display_text);
        
        whd_free(change);
    }
}

/*------------------------------------------------------------------------*/
/* Restore Operations - Entry Loading and Restore                        */
/*------------------------------------------------------------------------*/

UWORD iTidy_LoadBackupEntries(const char *session_id, struct List *entry_list)
{
    char changes_file_path[256];
    BPTR file;
    char line[1024];
    iTidy_ToolBackupEntry *entry;
    UWORD count = 0;
    char *comma1, *comma2;
    
    if (session_id == NULL || entry_list == NULL)
        return 0;
    
    /* Build path to changes.csv */
    sprintf(changes_file_path, "%s/%s/%s",
            TOOL_BACKUP_ROOT_PATH, session_id, TOOL_BACKUP_CHANGES_FILE);
    
    /* Open changes.csv */
    file = Open((CONST_STRPTR)changes_file_path, MODE_OLDFILE);
    if (!file)
    {
        append_to_log("ERROR: Could not open changes file: %s\n", changes_file_path);
        return 0;
    }
    
    /* Parse CSV line by line */
    while (FGets(file, line, sizeof(line)))
    {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
            continue;
        
        /* Parse CSV: icon_path,old_tool,new_tool */
        comma1 = strchr(line, ',');
        if (!comma1)
            continue;
        
        *comma1 = '\0';
        comma2 = strchr(comma1 + 1, ',');
        if (!comma2)
            continue;
        
        *comma2 = '\0';
        
        /* Allocate entry */
        entry = (iTidy_ToolBackupEntry *)whd_malloc(sizeof(iTidy_ToolBackupEntry));
        if (!entry)
            continue;
        
        memset(entry, 0, sizeof(iTidy_ToolBackupEntry));
        
        /* Unescape and copy fields */
        unescape_csv_field(line, entry->icon_path);
        unescape_csv_field(comma1 + 1, entry->old_tool);
        unescape_csv_field(comma2 + 1, entry->new_tool);
        
        /* Remove trailing newline from new_tool */
        entry->new_tool[strcspn(entry->new_tool, "\n")] = '\0';
        
        /* Add to list */
        AddTail(entry_list, (struct Node *)entry);
        count++;
    }
    
    Close(file);
    
    append_to_log("Loaded %d entries from session %s\n", count, session_id);
    return count;
}

void iTidy_FreeBackupEntryList(struct List *entry_list)
{
    struct Node *node;
    
    if (entry_list == NULL)
        return;
    
    while ((node = RemHead(entry_list)) != NULL)
    {
        whd_free(node);
    }
}

BOOL iTidy_RestoreAllIcons(const char *session_id,
                            UWORD *success_count,
                            UWORD *failed_count)
{
    struct List entry_list;
    iTidy_ToolBackupEntry *entry;
    struct Node *node;
    BOOL result;
    UWORD success = 0;
    UWORD failed = 0;
    
    if (session_id == NULL)
        return FALSE;
    
    /* Initialize list */
    NewList(&entry_list);
    
    /* Load all entries */
    if (iTidy_LoadBackupEntries(session_id, &entry_list) == 0)
    {
        append_to_log("ERROR: No entries to restore in session %s\n", session_id);
        return FALSE;
    }
    
    append_to_log("Restoring all icons from session %s...\n", session_id);
    
    /* Restore each icon */
    for (node = entry_list.lh_Head; node->ln_Succ != NULL; node = node->ln_Succ)
    {
        entry = (iTidy_ToolBackupEntry *)node;
        
        /* Restore original default tool */
        result = SetIconDefaultTool(entry->icon_path,
                                   entry->old_tool[0] ? entry->old_tool : NULL);
        
        if (result)
        {
            success++;
            append_to_log("  Restored: %s\n", entry->icon_path);
        }
        else
        {
            failed++;
            append_to_log("  FAILED: %s\n", entry->icon_path);
        }
    }
    
    /* Free entries */
    iTidy_FreeBackupEntryList(&entry_list);
    
    if (success_count)
        *success_count = success;
    if (failed_count)
        *failed_count = failed;
    
    append_to_log("Restore complete: %d succeeded, %d failed\n", success, failed);
    
    return (success > 0);
}

BOOL iTidy_RestoreToolChange(const char *session_id,
                              const char *old_tool,
                              const char *new_tool,
                              UWORD *success_count,
                              UWORD *failed_count)
{
    struct List entry_list;
    iTidy_ToolBackupEntry *entry;
    struct Node *node;
    BOOL result;
    UWORD success = 0;
    UWORD failed = 0;
    
    if (session_id == NULL || old_tool == NULL || new_tool == NULL)
        return FALSE;
    
    /* Initialize list */
    NewList(&entry_list);
    
    /* Load all entries */
    if (iTidy_LoadBackupEntries(session_id, &entry_list) == 0)
    {
        append_to_log("ERROR: No entries to restore in session %s\n", session_id);
        return FALSE;
    }
    
    append_to_log("Restoring tool change '%s' → '%s' from session %s...\n",
                 old_tool, new_tool, session_id);
    
    /* Restore matching icons */
    for (node = entry_list.lh_Head; node->ln_Succ != NULL; node = node->ln_Succ)
    {
        entry = (iTidy_ToolBackupEntry *)node;
        
        /* Check if this entry matches the tool change */
        if (strcmp(entry->old_tool, old_tool) == 0 &&
            strcmp(entry->new_tool, new_tool) == 0)
        {
            /* Restore original default tool */
            result = SetIconDefaultTool(entry->icon_path,
                                       entry->old_tool[0] ? entry->old_tool : NULL);
            
            if (result)
            {
                success++;
                append_to_log("  Restored: %s\n", entry->icon_path);
            }
            else
            {
                failed++;
                append_to_log("  FAILED: %s\n", entry->icon_path);
            }
        }
    }
    
    /* Free entries */
    iTidy_FreeBackupEntryList(&entry_list);
    
    if (success_count)
        *success_count = success;
    if (failed_count)
        *failed_count = failed;
    
    append_to_log("Restore complete: %d succeeded, %d failed\n", success, failed);
    
    return (success > 0);
}

BOOL iTidy_DeleteBackupSession(const char *session_id)
{
    char session_path[256];
    char file_path[256];
    BPTR lock;
    
    if (session_id == NULL)
        return FALSE;
    
    /* Build session path */
    sprintf(session_path, "%s/%s", TOOL_BACKUP_ROOT_PATH, session_id);
    
    /* Delete session.txt */
    sprintf(file_path, "%s/%s", session_path, TOOL_BACKUP_SESSION_FILE);
    DeleteFile((CONST_STRPTR)file_path);
    
    /* Delete changes.csv */
    sprintf(file_path, "%s/%s", session_path, TOOL_BACKUP_CHANGES_FILE);
    DeleteFile((CONST_STRPTR)file_path);
    
    /* Delete session directory */
    lock = Lock((CONST_STRPTR)session_path, ACCESS_READ);
    if (lock)
    {
        UnLock(lock);
        if (DeleteFile((CONST_STRPTR)session_path))
        {
            append_to_log("Deleted backup session: %s\n", session_id);
            return TRUE;
        }
    }
    
    append_to_log("ERROR: Could not delete session: %s\n", session_id);
    return FALSE;
}

/* End of default_tool_backup.c */
