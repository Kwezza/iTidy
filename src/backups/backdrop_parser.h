/*
 * backdrop_parser.h - iTidy Enhanced .backdrop File Parser
 * Parses, validates, and edits .backdrop files for the
 * Workbench Screen Manager (Backdrop Cleaner)
 */

#ifndef ITIDY_BACKDROP_PARSER_H
#define ITIDY_BACKDROP_PARSER_H

#include <exec/types.h>
#include <dos/dos.h>

/*------------------------------------------------------------------------*/
/* Constants                                                              */
/*------------------------------------------------------------------------*/
#define ITIDY_BACKDROP_MAX_PATH       256
#define ITIDY_BACKDROP_MAX_FULL_PATH  320
#define ITIDY_BACKDROP_MAX_DEVICE     64
#define ITIDY_BACKDROP_MAX_NAME       64

/*------------------------------------------------------------------------*/
/* Backdrop Entry Status                                                  */
/*------------------------------------------------------------------------*/
typedef enum {
    ITIDY_ENTRY_VALID = 0,        /* Entry exists on disk */
    ITIDY_ENTRY_ORPHAN,           /* Entry does not exist (dead link) */
    ITIDY_ENTRY_CANNOT_VERIFY,    /* Device has no media, cannot check */
    ITIDY_ENTRY_DEVICE_ICON       /* Not from .backdrop - device root icon */
} iTidy_EntryStatus;

/*------------------------------------------------------------------------*/
/* Backdrop Entry Structure                                               */
/*------------------------------------------------------------------------*/
typedef struct {
    char entry_path[ITIDY_BACKDROP_MAX_PATH];       /* Raw .backdrop line (e.g. ":Prefs/ScreenMode") */
    char full_path[ITIDY_BACKDROP_MAX_FULL_PATH];   /* Full path (e.g. "Workbench:Prefs/ScreenMode") */
    char device_name[ITIDY_BACKDROP_MAX_DEVICE];    /* Source device (e.g. "Workbench") */
    char display_name[ITIDY_BACKDROP_MAX_NAME];     /* Icon label shown on Workbench (e.g. "ScreenMode") */
    iTidy_EntryStatus status;                        /* Validation result */
    LONG icon_type;                                  /* do_Type: WBDISK, WBTOOL, WBPROJECT, etc. */
    LONG icon_x;                                     /* do_CurrentX position (-1 if unknown) */
    LONG icon_y;                                     /* do_CurrentY position (-1 if unknown) */
    BOOL selected;                                   /* UI selection state */
} iTidy_BackdropEntry;

/*------------------------------------------------------------------------*/
/* Backdrop List Structure (dynamic array)                                */
/*------------------------------------------------------------------------*/
typedef struct {
    iTidy_BackdropEntry *entries;    /* Dynamic array (whd_malloc) */
    int count;                       /* Number of entries */
    int capacity;                    /* Allocated capacity */
} iTidy_BackdropList;

/*------------------------------------------------------------------------*/
/* Function Prototypes                                                    */
/*------------------------------------------------------------------------*/

/**
 * @brief Initialize an empty backdrop list
 *
 * @param list Backdrop list to initialize
 */
void itidy_init_backdrop_list(iTidy_BackdropList *list);

/**
 * @brief Parse .backdrop file for a given device
 *
 * Opens <device_name>:.backdrop and reads all entries.
 * Missing .backdrop file returns an empty list (not an error).
 *
 * @param device_name Volume name without colon (e.g. "Workbench")
 * @param out Backdrop list to populate
 * @return BOOL TRUE if successful (empty list = success), FALSE on error
 */
BOOL itidy_parse_backdrop(const char *device_name, iTidy_BackdropList *out);

/**
 * @brief Validate all entries in a backdrop list
 *
 * For each entry, attempts to Lock() the full path + ".info" to verify
 * existence. Also loads icon via GetDiskObject() to read type and position.
 * Suppresses DOS requesters for safety.
 *
 * @param list Backdrop list with entries to validate
 * @return int Number of orphaned entries found
 */
int itidy_validate_backdrop_entries(iTidy_BackdropList *list);

/**
 * @brief Add a device icon entry to the backdrop list
 *
 * Adds a synthetic entry representing the device root icon (Disk.info).
 * These are not from .backdrop but are needed for layout operations.
 *
 * @param list Backdrop list to add to
 * @param device_name Volume name without colon
 * @return BOOL TRUE if added successfully
 */
BOOL itidy_add_device_icon_entry(iTidy_BackdropList *list,
                                  const char *device_name);

/**
 * @brief Backup .backdrop file before editing
 *
 * Copies <device>:.backdrop to <device>:.backdrop.bak
 *
 * @param device_name Volume name without colon
 * @return BOOL TRUE if backup succeeded or no .backdrop exists
 */
BOOL itidy_backup_backdrop(const char *device_name);

/**
 * @brief Remove orphaned entries from .backdrop file
 *
 * Rewrites the .backdrop file, omitting entries marked as ITIDY_ENTRY_ORPHAN
 * that have their 'selected' flag set. Calls itidy_backup_backdrop() first.
 *
 * @param device_name Volume name without colon
 * @param list Backdrop list with validated/selected entries
 * @return int Number of entries removed, or -1 on error
 */
int itidy_remove_selected_orphans(const char *device_name,
                                   iTidy_BackdropList *list);

/**
 * @brief Count orphaned entries in the list
 *
 * @param list Backdrop list to count
 * @return int Number of entries with status ITIDY_ENTRY_ORPHAN
 */
int itidy_count_orphans(const iTidy_BackdropList *list);

/**
 * @brief Count selected orphaned entries in the list
 *
 * @param list Backdrop list to count
 * @return int Number of selected orphan entries
 */
int itidy_count_selected_orphans(const iTidy_BackdropList *list);

/**
 * @brief Free backdrop list resources
 *
 * @param list Backdrop list to free (does not free the struct itself)
 */
void itidy_free_backdrop_list(iTidy_BackdropList *list);

/**
 * @brief Get icon type name as a string
 *
 * @param icon_type do_Type value (WBDISK, WBTOOL, etc.)
 * @return const char* Human-readable type name
 */
const char *itidy_icon_type_name(LONG icon_type);

#endif /* ITIDY_BACKDROP_PARSER_H */
