/*
 * backdrop_parser.c - iTidy Enhanced .backdrop File Parser Implementation
 * Parses, validates, and edits .backdrop files for the
 * Workbench Screen Manager (Backdrop Cleaner)
 *
 * .backdrop file format:
 *   - Plain text, one entry per line
 *   - Each line starts with ':' (colon = volume root)
 *   - Path is relative to volume root, without .info extension
 *   - Example: ":Prefs/ScreenMode" = "Workbench:Prefs/ScreenMode"
 */

#include <platform/platform.h>
#include <platform/amiga_headers.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <workbench/workbench.h>
#include <string.h>
#include <stdio.h>

/* Console output abstraction */
#include <console_output.h>

#include "backdrop_parser.h"
#include "writeLog.h"

/*------------------------------------------------------------------------*/
/* Internal Helpers                                                       */
/*------------------------------------------------------------------------*/

/**
 * Grow backdrop list capacity if needed.
 */
static BOOL grow_backdrop_list(iTidy_BackdropList *list)
{
    int new_capacity;
    iTidy_BackdropEntry *new_entries;

    if (list->count < list->capacity)
        return TRUE;

    new_capacity = list->capacity > 0 ? list->capacity * 2 : 16;
    new_entries = (iTidy_BackdropEntry *)whd_malloc(
        new_capacity * sizeof(iTidy_BackdropEntry));

    if (!new_entries)
    {
        log_error(LOG_GENERAL, "Failed to grow backdrop list to %d entries\n",
                  new_capacity);
        return FALSE;
    }

    if (list->entries && list->count > 0)
    {
        memcpy(new_entries, list->entries,
               list->count * sizeof(iTidy_BackdropEntry));
        whd_free(list->entries);
    }

    list->entries = new_entries;
    list->capacity = new_capacity;
    return TRUE;
}

/**
 * Copy a file using AmigaOS DOS functions.
 * Returns TRUE on success.
 */
static BOOL copy_file(const char *src_path, const char *dst_path)
{
    BPTR src_file;
    BPTR dst_file;
    char buffer[512];
    LONG bytes_read;

    src_file = Open((STRPTR)src_path, MODE_OLDFILE);
    if (!src_file)
        return FALSE;

    dst_file = Open((STRPTR)dst_path, MODE_NEWFILE);
    if (!dst_file)
    {
        Close(src_file);
        return FALSE;
    }

    while ((bytes_read = Read(src_file, buffer, sizeof(buffer))) > 0)
    {
        if (Write(dst_file, buffer, bytes_read) != bytes_read)
        {
            Close(dst_file);
            Close(src_file);
            return FALSE;
        }
    }

    Close(dst_file);
    Close(src_file);
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Public Functions                                                       */
/*------------------------------------------------------------------------*/

void itidy_init_backdrop_list(iTidy_BackdropList *list)
{
    if (!list)
        return;

    list->entries = NULL;
    list->count = 0;
    list->capacity = 0;
}

void itidy_free_backdrop_list(iTidy_BackdropList *list)
{
    if (!list)
        return;

    if (list->entries)
    {
        whd_free(list->entries);
        list->entries = NULL;
    }

    list->count = 0;
    list->capacity = 0;
}

const char *itidy_icon_type_name(LONG icon_type)
{
    switch (icon_type)
    {
        case 1:  return "Disk";       /* WBDISK */
        case 2:  return "Drawer";     /* WBDRAWER */
        case 3:  return "Tool";       /* WBTOOL */
        case 4:  return "Project";    /* WBPROJECT */
        case 5:  return "Trashcan";   /* WBGARBAGE */
        case 6:  return "Device";     /* WBDEVICE */
        case 7:  return "Kickstart";  /* WBKICK */
        case 8:  return "AppIcon";    /* WBAPPICON */
        default: return "Unknown";
    }
}

BOOL itidy_parse_backdrop(const char *device_name, iTidy_BackdropList *out)
{
    BPTR file;
    char backdrop_path[ITIDY_BACKDROP_MAX_FULL_PATH];
    char buffer[ITIDY_BACKDROP_MAX_PATH + 16];
    char *newline;

    if (!device_name || !out)
        return FALSE;

    /* Build path to .backdrop file */
    snprintf(backdrop_path, sizeof(backdrop_path), "%s:.backdrop",
             device_name);

    log_info(LOG_GENERAL, "Parsing backdrop file: %s\n", backdrop_path);

    file = Open((STRPTR)backdrop_path, MODE_OLDFILE);
    if (!file)
    {
        /* No .backdrop file is normal - means no left-out icons */
        log_info(LOG_GENERAL, "  No .backdrop file found (this is normal)\n");
        return TRUE;
    }

    /* Read each line */
    while (FGets(file, buffer, sizeof(buffer)))
    {
        iTidy_BackdropEntry entry;

        /* Strip trailing newline */
        newline = strchr(buffer, '\n');
        if (newline)
            *newline = '\0';

        /* Also strip carriage return if present */
        newline = strchr(buffer, '\r');
        if (newline)
            *newline = '\0';

        /* Skip empty lines */
        if (buffer[0] == '\0')
            continue;

        /* Validate format: must start with ':' */
        if (buffer[0] != ':')
        {
            log_warning(LOG_GENERAL,
                        "  Unexpected .backdrop entry format: '%s'\n",
                        buffer);
            continue;
        }

        /* Initialize entry */
        memset(&entry, 0, sizeof(entry));
        strncpy(entry.entry_path, buffer, sizeof(entry.entry_path) - 1);
        strncpy(entry.device_name, device_name,
                sizeof(entry.device_name) - 1);

        /* Construct full path: device_name + entry (colon replaces device's colon)
         * e.g. "Workbench" + ":Prefs/ScreenMode" = "Workbench:Prefs/ScreenMode" */
        snprintf(entry.full_path, sizeof(entry.full_path), "%s%s",
                 device_name, buffer);

        /* Extract display name: last component of the path
         * e.g. ":Prefs/ScreenMode" -> "ScreenMode"
         *      ":Rebuild" -> "Rebuild" */
        {
            const char *name_start;
            const char *last_slash = strrchr(buffer, '/');
            if (last_slash)
                name_start = last_slash + 1;
            else
                name_start = buffer + 1; /* skip the leading ':' */

            strncpy(entry.display_name, name_start,
                    sizeof(entry.display_name) - 1);
            entry.display_name[sizeof(entry.display_name) - 1] = '\0';
        }

        entry.status = ITIDY_ENTRY_VALID; /* Will be updated by validation */
        entry.icon_type = 0;
        entry.icon_x = -1;
        entry.icon_y = -1;
        entry.selected = FALSE;

        /* Add to list */
        if (!grow_backdrop_list(out))
        {
            Close(file);
            return FALSE;
        }

        out->entries[out->count] = entry;
        out->count++;

        log_info(LOG_GENERAL, "  Entry %d: %s -> %s\n",
                 out->count, buffer, entry.full_path);
    }

    Close(file);

    log_info(LOG_GENERAL, "  Parsed %d backdrop entries from %s\n",
             out->count, backdrop_path);
    return TRUE;
}

int itidy_validate_backdrop_entries(iTidy_BackdropList *list)
{
    struct Process *proc;
    APTR old_window_ptr;
    int orphan_count = 0;
    int i;

    if (!list || list->count == 0)
        return 0;

    log_info(LOG_GENERAL, "Validating %d backdrop entries...\n",
             list->count);

    /* Suppress DOS requesters during validation */
    proc = (struct Process *)FindTask(NULL);
    old_window_ptr = proc->pr_WindowPtr;
    proc->pr_WindowPtr = (APTR)-1;

    for (i = 0; i < list->count; i++)
    {
        iTidy_BackdropEntry *entry = &list->entries[i];
        char icon_path[ITIDY_BACKDROP_MAX_FULL_PATH + 8];
        BPTR lock;
        struct DiskObject *dobj;

        /* Skip device icon entries (not from .backdrop) */
        if (entry->status == ITIDY_ENTRY_DEVICE_ICON)
            continue;

        /* Check if the target file/directory exists */
        lock = Lock((STRPTR)entry->full_path, ACCESS_READ);
        if (lock == 0)
        {
            /* Target doesn't exist -> orphaned entry */
            entry->status = ITIDY_ENTRY_ORPHAN;
            orphan_count++;
            log_info(LOG_GENERAL, "  ORPHAN: %s (target not found)\n",
                     entry->full_path);
            continue;
        }
        UnLock(lock);

        /* Target exists - try to load icon for type and position */
        dobj = GetDiskObject((STRPTR)entry->full_path);
        if (dobj)
        {
            entry->icon_type = dobj->do_Type;
            entry->icon_x = dobj->do_CurrentX;
            entry->icon_y = dobj->do_CurrentY;
            FreeDiskObject(dobj);

            log_info(LOG_GENERAL,
                     "  VALID: %s (type=%s, pos=%ld,%ld)\n",
                     entry->full_path,
                     itidy_icon_type_name(entry->icon_type),
                     (long)entry->icon_x, (long)entry->icon_y);
        }
        else
        {
            /* File exists but no icon - still valid in .backdrop */
            entry->icon_type = 0;
            entry->icon_x = -1;
            entry->icon_y = -1;
            log_info(LOG_GENERAL,
                     "  VALID: %s (no .info icon found)\n",
                     entry->full_path);
        }

        entry->status = ITIDY_ENTRY_VALID;
    }

    /* Restore DOS requester window pointer */
    proc->pr_WindowPtr = old_window_ptr;

    log_info(LOG_GENERAL,
             "Validation complete: %d orphaned entries found\n",
             orphan_count);
    return orphan_count;
}

BOOL itidy_add_device_icon_entry(iTidy_BackdropList *list,
                                  const char *device_name)
{
    iTidy_BackdropEntry entry;
    struct DiskObject *dobj;
    char disk_info_path[ITIDY_BACKDROP_MAX_FULL_PATH];

    if (!list || !device_name)
        return FALSE;

    memset(&entry, 0, sizeof(entry));

    /* Device icon path is always <device>:Disk */
    snprintf(disk_info_path, sizeof(disk_info_path), "%s:Disk", device_name);
    snprintf(entry.full_path, sizeof(entry.full_path), "%s:Disk", device_name);
    strncpy(entry.device_name, device_name, sizeof(entry.device_name) - 1);
    strncpy(entry.entry_path, "(device icon)", sizeof(entry.entry_path) - 1);
    strncpy(entry.display_name, device_name, sizeof(entry.display_name) - 1);

    entry.status = ITIDY_ENTRY_DEVICE_ICON;
    entry.icon_type = 1; /* WBDISK */
    entry.icon_x = -1;
    entry.icon_y = -1;
    entry.selected = FALSE;

    /* Try to load position from icon */
    {
        struct Process *proc = (struct Process *)FindTask(NULL);
        APTR old_window_ptr = proc->pr_WindowPtr;
        proc->pr_WindowPtr = (APTR)-1;

        dobj = GetDiskObject((STRPTR)disk_info_path);
        if (dobj)
        {
            entry.icon_type = dobj->do_Type;
            entry.icon_x = dobj->do_CurrentX;
            entry.icon_y = dobj->do_CurrentY;
            FreeDiskObject(dobj);
        }

        proc->pr_WindowPtr = old_window_ptr;
    }

    if (!grow_backdrop_list(list))
        return FALSE;

    list->entries[list->count] = entry;
    list->count++;

    log_info(LOG_GENERAL,
             "Added device icon entry: %s (pos=%ld,%ld)\n",
             device_name, (long)entry.icon_x, (long)entry.icon_y);

    return TRUE;
}

BOOL itidy_backup_backdrop(const char *device_name)
{
    char src_path[ITIDY_BACKDROP_MAX_FULL_PATH];
    char bak_path[ITIDY_BACKDROP_MAX_FULL_PATH];
    BPTR test_lock;

    if (!device_name)
        return FALSE;

    snprintf(src_path, sizeof(src_path), "%s:.backdrop", device_name);
    snprintf(bak_path, sizeof(bak_path), "%s:.backdrop.bak", device_name);

    /* Check if .backdrop exists */
    test_lock = Lock((STRPTR)src_path, ACCESS_READ);
    if (!test_lock)
    {
        /* No .backdrop to backup - that's OK */
        log_info(LOG_GENERAL, "No .backdrop file to backup on %s:\n",
                 device_name);
        return TRUE;
    }
    UnLock(test_lock);

    /* Copy .backdrop to .backdrop.bak */
    if (copy_file(src_path, bak_path))
    {
        log_info(LOG_GENERAL, "Backed up %s to %s\n", src_path, bak_path);
        return TRUE;
    }
    else
    {
        log_error(LOG_GENERAL, "Failed to backup %s to %s\n",
                  src_path, bak_path);
        return FALSE;
    }
}

int itidy_remove_selected_orphans(const char *device_name,
                                   iTidy_BackdropList *list)
{
    char backdrop_path[ITIDY_BACKDROP_MAX_FULL_PATH];
    BPTR file;
    int removed_count = 0;
    int kept_count = 0;
    int i;

    if (!device_name || !list)
        return -1;

    /* Backup first */
    if (!itidy_backup_backdrop(device_name))
    {
        log_error(LOG_GENERAL,
                  "Cannot remove orphans: backup failed for %s\n",
                  device_name);
        return -1;
    }

    snprintf(backdrop_path, sizeof(backdrop_path), "%s:.backdrop",
             device_name);

    /* Rewrite the .backdrop file, omitting selected orphans */
    file = Open((STRPTR)backdrop_path, MODE_NEWFILE);
    if (!file)
    {
        log_error(LOG_GENERAL, "Failed to open %s for writing\n",
                  backdrop_path);
        return -1;
    }

    for (i = 0; i < list->count; i++)
    {
        iTidy_BackdropEntry *entry = &list->entries[i];

        /* Skip non-backdrop entries (device icons) */
        if (entry->status == ITIDY_ENTRY_DEVICE_ICON)
            continue;

        /* Skip entries from other devices */
        if (strcmp(entry->device_name, device_name) != 0)
            continue;

        /* If orphan and selected, skip it (remove from file) */
        if (entry->status == ITIDY_ENTRY_ORPHAN && entry->selected)
        {
            removed_count++;
            log_info(LOG_GENERAL, "  Removing orphan: %s\n",
                     entry->entry_path);
            continue;
        }

        /* Write this entry back */
        {
            char line[ITIDY_BACKDROP_MAX_PATH + 2];
            int line_len;

            snprintf(line, sizeof(line), "%s\n", entry->entry_path);
            line_len = strlen(line);

            if (Write(file, line, line_len) != line_len)
            {
                log_error(LOG_GENERAL,
                          "Write error in .backdrop rewrite\n");
                Close(file);
                return -1;
            }
            kept_count++;
        }
    }

    Close(file);

    log_info(LOG_GENERAL,
             "Rewrote %s: kept %d entries, removed %d orphans\n",
             backdrop_path, kept_count, removed_count);

    return removed_count;
}

int itidy_count_orphans(const iTidy_BackdropList *list)
{
    int count = 0;
    int i;

    if (!list)
        return 0;

    for (i = 0; i < list->count; i++)
    {
        if (list->entries[i].status == ITIDY_ENTRY_ORPHAN)
            count++;
    }
    return count;
}

int itidy_count_selected_orphans(const iTidy_BackdropList *list)
{
    int count = 0;
    int i;

    if (!list)
        return 0;

    for (i = 0; i < list->count; i++)
    {
        if (list->entries[i].status == ITIDY_ENTRY_ORPHAN &&
            list->entries[i].selected)
            count++;
    }
    return count;
}
