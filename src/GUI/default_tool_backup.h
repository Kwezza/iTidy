/**
 * default_tool_backup.h - Default Tool Backup and Restore System
 * 
 * Manages lightweight CSV-based backup/restore for default tool changes.
 * Unlike the main backup system (LhA-based for folder operations),
 * this system is optimized for scattered icon files across the disk.
 * 
 * Storage: ENVARC:iTidy/Backups/tools/YYYYMMDD_HHMMSS/
 *   - session.txt (metadata about the session)
 *   - changes.csv (icon_path, old_tool, new_tool)
 */

#ifndef DEFAULT_TOOL_BACKUP_H
#define DEFAULT_TOOL_BACKUP_H

#include <exec/types.h>
#include <exec/lists.h>
#include <dos/dos.h>

/*========================================================================*/
/* Constants                                                              */
/*========================================================================*/
#define TOOL_BACKUP_ROOT_PATH "PROGDIR:Backups/tools"
#define TOOL_BACKUP_SESSION_FILE "session.txt"
#define TOOL_BACKUP_CHANGES_FILE "changes.csv"
#define TOOL_BACKUP_MAX_PATH_LEN 256
#define TOOL_BACKUP_MAX_TOOL_LEN 256
#define TOOL_BACKUP_MAX_SESSIONS 100

/*========================================================================*/
/* Session Metadata Structure                                            */
/*========================================================================*/
/**
 * @brief Metadata for a backup session
 * 
 * This structure contains summary information about a backup session,
 * read from the session.txt file. It is used to populate the session
 * listview in the restore window.
 */
typedef struct iTidy_ToolBackupSession {
    struct Node node;                   /* Exec list node (ln_Name = display text) */
    char session_id[32];                /* Session ID (YYYYMMDD_HHMMSS) */
    char date_string[32];               /* Human-readable date (DD-Mon-YYYY HH:MM) */
    char mode[16];                      /* "Batch" or "Single" */
    char scanned_path[108];             /* Folder scanned (Batch) or icon path (Single) */
    UWORD icons_changed;                /* Number of icons changed */
    UWORD icons_skipped;                /* Number of icons skipped */
    UWORD icons_total;                  /* Total icons processed */
    char *display_text;                 /* Formatted text for ListView */
} iTidy_ToolBackupSession;

/*========================================================================*/
/* Tool Change Entry Structure                                           */
/*========================================================================*/
/**
 * @brief Represents one default tool change within a session
 * 
 * This structure groups icons that had the same tool change
 * (e.g., all icons changed from "MultiView" to "More").
 */
typedef struct iTidy_ToolChange {
    struct Node node;                   /* Exec list node (ln_Name = display text) */
    char old_tool[TOOL_BACKUP_MAX_TOOL_LEN];  /* Original default tool */
    char new_tool[TOOL_BACKUP_MAX_TOOL_LEN];  /* New default tool */
    UWORD icon_count;                   /* Number of icons with this change */
    char *display_text;                 /* Formatted text for ListView */
} iTidy_ToolChange;

/*========================================================================*/
/* Icon Entry Structure                                                   */
/*========================================================================*/
/**
 * @brief Represents one icon's default tool change
 * 
 * This is the raw data from the CSV file, used for actual restore operations.
 */
typedef struct iTidy_ToolBackupEntry {
    struct Node node;                   /* Exec list node */
    char icon_path[TOOL_BACKUP_MAX_PATH_LEN];  /* Full path to .info file */
    char old_tool[TOOL_BACKUP_MAX_TOOL_LEN];   /* Original default tool */
    char new_tool[TOOL_BACKUP_MAX_TOOL_LEN];   /* New default tool */
} iTidy_ToolBackupEntry;

/*========================================================================*/
/* Backup Manager Structure                                              */
/*========================================================================*/
/**
 * @brief Main backup manager context
 * 
 * This structure manages the backup session creation and provides
 * access to the active backup file handles.
 */
typedef struct iTidy_ToolBackupManager {
    char session_id[32];                /* Current session ID (YYYYMMDD_HHMMSS) */
    char session_path[256];             /* Full path to session folder */
    BPTR session_file;                  /* File handle for session.txt */
    BPTR changes_file;                  /* File handle for changes.csv */
    BOOL backup_enabled;                /* TRUE if backups are enabled */
    BOOL session_active;                /* TRUE if a session is open */
    UWORD icons_changed;                /* Counter for this session */
    UWORD icons_skipped;                /* Counter for this session */
} iTidy_ToolBackupManager;

/*========================================================================*/
/* Function Prototypes - Backup Operations                               */
/*========================================================================*/

/**
 * @brief Initialize the backup manager
 * 
 * Creates the root backup directory if needed and initializes the manager.
 * Call this once at application startup.
 * 
 * @param manager Pointer to backup manager structure
 * @param enabled TRUE to enable backups, FALSE to disable
 * @return TRUE if successful, FALSE on error
 */
BOOL iTidy_InitToolBackupManager(iTidy_ToolBackupManager *manager, BOOL enabled);

/**
 * @brief Start a new backup session
 * 
 * Creates a new session folder with unique timestamp ID and opens
 * session.txt and changes.csv files for writing.
 * 
 * @param manager Pointer to backup manager structure
 * @param mode "Batch" or "Single"
 * @param scanned_path Folder path (Batch) or icon path (Single)
 * @return TRUE if successful, FALSE on error
 */
BOOL iTidy_StartBackupSession(iTidy_ToolBackupManager *manager,
                               const char *mode,
                               const char *scanned_path);

/**
 * @brief Record a default tool change to the backup
 * 
 * Appends one line to changes.csv with the icon path and tool change.
 * 
 * @param manager Pointer to backup manager structure
 * @param icon_path Full path to the .info file
 * @param old_tool Original default tool (or "" if none)
 * @param new_tool New default tool (or "" to clear)
 * @return TRUE if successful, FALSE on error
 */
BOOL iTidy_RecordToolChange(iTidy_ToolBackupManager *manager,
                             const char *icon_path,
                             const char *old_tool,
                             const char *new_tool);

/**
 * @brief Finalize and close the backup session
 * 
 * Updates session.txt with final statistics and closes all files.
 * 
 * @param manager Pointer to backup manager structure
 */
void iTidy_EndBackupSession(iTidy_ToolBackupManager *manager);

/**
 * @brief Clean up the backup manager
 * 
 * Closes any open session and frees resources.
 * 
 * @param manager Pointer to backup manager structure
 */
void iTidy_CleanupToolBackupManager(iTidy_ToolBackupManager *manager);

/*========================================================================*/
/* Function Prototypes - Restore Operations                              */
/*========================================================================*/

/**
 * @brief Scan for available backup sessions
 * 
 * Reads the backup root directory and creates a list of available sessions.
 * The list must be freed by the caller using iTidy_FreeSessionList().
 * 
 * @param session_list Pointer to Exec list to populate
 * @return Number of sessions found, or 0 on error
 */
UWORD iTidy_ScanBackupSessions(struct List *session_list);

/**
 * @brief Free the session list
 * 
 * Frees all session entries allocated by iTidy_ScanBackupSessions().
 * 
 * @param session_list Pointer to session list
 */
void iTidy_FreeSessionList(struct List *session_list);

/**
 * @brief Load tool changes from a session
 * 
 * Reads the changes.csv file and groups changes by old_tool → new_tool.
 * The list must be freed by the caller using iTidy_FreeToolChangeList().
 * 
 * @param session_id Session ID (YYYYMMDD_HHMMSS)
 * @param tool_change_list Pointer to Exec list to populate
 * @return Number of unique tool changes found, or 0 on error
 */
UWORD iTidy_LoadToolChanges(const char *session_id,
                             struct List *tool_change_list);

/**
 * @brief Free the tool change list
 * 
 * Frees all tool change entries allocated by iTidy_LoadToolChanges().
 * 
 * @param tool_change_list Pointer to tool change list
 */
void iTidy_FreeToolChangeList(struct List *tool_change_list);

/**
 * @brief Load all icon entries from a session
 * 
 * Reads the changes.csv file and returns all individual icon entries.
 * Used for restore operations. The list must be freed by the caller
 * using iTidy_FreeBackupEntryList().
 * 
 * @param session_id Session ID (YYYYMMDD_HHMMSS)
 * @param entry_list Pointer to Exec list to populate
 * @return Number of entries loaded, or 0 on error
 */
UWORD iTidy_LoadBackupEntries(const char *session_id,
                               struct List *entry_list);

/**
 * @brief Free the backup entry list
 * 
 * Frees all entries allocated by iTidy_LoadBackupEntries().
 * 
 * @param entry_list Pointer to entry list
 */
void iTidy_FreeBackupEntryList(struct List *entry_list);

/**
 * @brief Restore all icons in a session
 * 
 * Restores the default tool for all icons in the session back to their
 * original values.
 * 
 * @param session_id Session ID (YYYYMMDD_HHMMSS)
 * @param success_count Pointer to store number of successful restores
 * @param failed_count Pointer to store number of failed restores
 * @return TRUE if at least one icon was restored, FALSE on complete failure
 */
BOOL iTidy_RestoreAllIcons(const char *session_id,
                            UWORD *success_count,
                            UWORD *failed_count);

/**
 * @brief Restore icons with a specific tool change
 * 
 * Restores only the icons that had a specific old_tool → new_tool change.
 * 
 * @param session_id Session ID (YYYYMMDD_HHMMSS)
 * @param old_tool Original default tool to match
 * @param new_tool New default tool to match
 * @param success_count Pointer to store number of successful restores
 * @param failed_count Pointer to store number of failed restores
 * @return TRUE if at least one icon was restored, FALSE on complete failure
 */
BOOL iTidy_RestoreToolChange(const char *session_id,
                              const char *old_tool,
                              const char *new_tool,
                              UWORD *success_count,
                              UWORD *failed_count);

/**
 * @brief Delete a backup session
 * 
 * Deletes the session folder and all files within it.
 * 
 * @param session_id Session ID (YYYYMMDD_HHMMSS)
 * @return TRUE if successful, FALSE on error
 */
BOOL iTidy_DeleteBackupSession(const char *session_id);

#endif /* DEFAULT_TOOL_BACKUP_H */
