/*
 * workbench_layout.c - iTidy Workbench Screen Icon Layout Engine
 * Calculates and applies icon positions for the Workbench screen
 * (device icons and left-out icons)
 *
 * Layout algorithm:
 *   Row 1+: Device icons sorted alphabetically by name, left-to-right
 *   Below:  Left-out icons sorted by type (Tool, Project, other) then name
 *   Wraps to next row when exceeding screen width minus margin
 */

#include <platform/platform.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/utility.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <string.h>
#include <stdio.h>

/* Console output abstraction */
#include <console_output.h>

#include "workbench_layout.h"
#include "writeLog.h"

/*------------------------------------------------------------------------*/
/* Internal Helpers                                                       */
/*------------------------------------------------------------------------*/

/**
 * Compare two backdrop entries for sorting.
 * Device icons sort first, then by type (Tool > Project > other),
 * then alphabetically by full_path.
 */
static int compare_entries(const iTidy_BackdropEntry *a,
                            const iTidy_BackdropEntry *b)
{
    /* Device icons always come first */
    if (a->status == ITIDY_ENTRY_DEVICE_ICON &&
        b->status != ITIDY_ENTRY_DEVICE_ICON)
        return -1;
    if (a->status != ITIDY_ENTRY_DEVICE_ICON &&
        b->status == ITIDY_ENTRY_DEVICE_ICON)
        return 1;

    /* Within same category, sort by type priority */
    if (a->icon_type != b->icon_type)
    {
        /* WBTOOL (3) before WBPROJECT (4) before others */
        int pri_a = (a->icon_type == 3) ? 0 :
                    (a->icon_type == 4) ? 1 : 2;
        int pri_b = (b->icon_type == 3) ? 0 :
                    (b->icon_type == 4) ? 1 : 2;
        if (pri_a != pri_b)
            return pri_a - pri_b;
    }

    /* Within same type, sort alphabetically */
    return Stricmp(a->full_path, b->full_path);
}

/**
 * Simple insertion sort for the index array.
 * We're sorting an index array, not the entries directly.
 */
static void sort_indices(int *indices, int count,
                          const iTidy_BackdropEntry *entries)
{
    int i, j, temp;

    for (i = 1; i < count; i++)
    {
        temp = indices[i];
        j = i - 1;
        while (j >= 0 &&
               compare_entries(&entries[indices[j]],
                               &entries[temp]) > 0)
        {
            indices[j + 1] = indices[j];
            j--;
        }
        indices[j + 1] = temp;
    }
}

/**
 * Grow layout result capacity if needed.
 */
static BOOL grow_layout_result(iTidy_WBLayoutResult *result)
{
    int new_capacity;
    iTidy_WBLayoutEntry *new_entries;

    if (result->count < result->capacity)
        return TRUE;

    new_capacity = result->capacity > 0 ? result->capacity * 2 : 16;
    new_entries = (iTidy_WBLayoutEntry *)whd_malloc(
        new_capacity * sizeof(iTidy_WBLayoutEntry));

    if (!new_entries)
        return FALSE;

    if (result->entries && result->count > 0)
    {
        memcpy(new_entries, result->entries,
               result->count * sizeof(iTidy_WBLayoutEntry));
        whd_free(result->entries);
    }

    result->entries = new_entries;
    result->capacity = new_capacity;
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Public Functions                                                       */
/*------------------------------------------------------------------------*/

void itidy_init_wb_layout_params(iTidy_WBLayoutParams *params,
                                  LONG screen_width,
                                  LONG screen_height)
{
    if (!params)
        return;

    params->screen_width = screen_width;
    params->screen_height = screen_height;
    params->grid_x = ITIDY_WB_GRID_X;
    params->grid_y = ITIDY_WB_GRID_Y;
    params->margin_left = ITIDY_WB_MARGIN_LEFT;
    params->margin_top = ITIDY_WB_MARGIN_TOP;
    params->margin_right = ITIDY_WB_MARGIN_RIGHT;
}

void itidy_init_wb_layout_result(iTidy_WBLayoutResult *result)
{
    if (!result)
        return;

    result->entries = NULL;
    result->count = 0;
    result->capacity = 0;
    result->changed_count = 0;
}

void itidy_free_wb_layout_result(iTidy_WBLayoutResult *result)
{
    if (!result)
        return;

    if (result->entries)
    {
        whd_free(result->entries);
        result->entries = NULL;
    }

    result->count = 0;
    result->capacity = 0;
    result->changed_count = 0;
}

BOOL itidy_calculate_wb_layout(const iTidy_WBLayoutParams *params,
                                const iTidy_BackdropList *list,
                                iTidy_WBLayoutResult *result)
{
    int *sorted_indices;
    int layoutable_count = 0;
    int i;
    LONG cur_x, cur_y;
    LONG max_x;

    if (!params || !list || !result)
        return FALSE;

    itidy_init_wb_layout_result(result);

    if (list->count == 0)
        return TRUE;

    /* Build array of indices for layoutable entries
     * (skip orphans and unverifiable entries) */
    sorted_indices = (int *)whd_malloc(list->count * sizeof(int));
    if (!sorted_indices)
        return FALSE;

    for (i = 0; i < list->count; i++)
    {
        const iTidy_BackdropEntry *entry = &list->entries[i];

        /* Only layout valid entries and device icons */
        if (entry->status == ITIDY_ENTRY_VALID ||
            entry->status == ITIDY_ENTRY_DEVICE_ICON)
        {
            sorted_indices[layoutable_count] = i;
            layoutable_count++;
        }
    }

    if (layoutable_count == 0)
    {
        whd_free(sorted_indices);
        return TRUE;
    }

    /* Sort: device icons first, then by type, then alphabetically */
    sort_indices(sorted_indices, layoutable_count, list->entries);

    /* Calculate positions in horizontal rows */
    cur_x = params->margin_left;
    cur_y = params->margin_top;
    max_x = params->screen_width - params->margin_right - params->grid_x;

    for (i = 0; i < layoutable_count; i++)
    {
        int entry_idx = sorted_indices[i];
        const iTidy_BackdropEntry *entry = &list->entries[entry_idx];
        iTidy_WBLayoutEntry layout_entry;

        /* Wrap to next row if past right edge */
        if (cur_x > max_x && i > 0)
        {
            cur_x = params->margin_left;
            cur_y += params->grid_y;
        }

        layout_entry.entry_index = entry_idx;
        layout_entry.new_x = cur_x;
        layout_entry.new_y = cur_y;
        layout_entry.old_x = entry->icon_x;
        layout_entry.old_y = entry->icon_y;
        layout_entry.changed = (cur_x != entry->icon_x ||
                                 cur_y != entry->icon_y);

        if (layout_entry.changed)
            result->changed_count++;

        if (!grow_layout_result(result))
        {
            whd_free(sorted_indices);
            return FALSE;
        }

        result->entries[result->count] = layout_entry;
        result->count++;

        cur_x += params->grid_x;
    }

    whd_free(sorted_indices);

    log_info(LOG_GENERAL,
             "Layout calculated: %d icons, %d changed positions\n",
             result->count, result->changed_count);

    return TRUE;
}

int itidy_apply_wb_layout(const iTidy_BackdropList *list,
                           const iTidy_WBLayoutResult *result)
{
    struct Process *proc;
    APTR old_window_ptr;
    int success_count = 0;
    int i;

    if (!list || !result || result->count == 0)
        return 0;

    /* Suppress DOS requesters during icon writes */
    proc = (struct Process *)FindTask(NULL);
    old_window_ptr = proc->pr_WindowPtr;
    proc->pr_WindowPtr = (APTR)-1;

    for (i = 0; i < result->count; i++)
    {
        const iTidy_WBLayoutEntry *le = &result->entries[i];
        const iTidy_BackdropEntry *entry;
        struct DiskObject *dobj;

        if (!le->changed)
            continue;

        if (le->entry_index < 0 || le->entry_index >= list->count)
            continue;

        entry = &list->entries[le->entry_index];

        /* Load the icon */
        dobj = GetDiskObject((STRPTR)entry->full_path);
        if (!dobj)
        {
            log_warning(LOG_GENERAL,
                        "Cannot load icon for layout: %s\n",
                        entry->full_path);
            continue;
        }

        /* Update position */
        dobj->do_CurrentX = le->new_x;
        dobj->do_CurrentY = le->new_y;

        /* Save using PutIconTagList with NotifyWorkbench (V44+) */
        {
            struct TagItem put_tags[2];
            put_tags[0].ti_Tag = ICONPUTA_NotifyWorkbench;
            put_tags[0].ti_Data = TRUE;
            put_tags[1].ti_Tag = TAG_DONE;
            put_tags[1].ti_Data = 0;

            if (PutIconTagList((STRPTR)entry->full_path, dobj, put_tags))
            {
                success_count++;
                log_info(LOG_GENERAL,
                         "Repositioned: %s (%ld,%ld) -> (%ld,%ld)\n",
                         entry->full_path,
                         (long)le->old_x, (long)le->old_y,
                         (long)le->new_x, (long)le->new_y);
            }
            else
            {
                log_error(LOG_GENERAL,
                          "Failed to save icon: %s\n",
                          entry->full_path);
            }
        }

        FreeDiskObject(dobj);
    }

    /* Restore DOS requester window pointer */
    proc->pr_WindowPtr = old_window_ptr;

    log_info(LOG_GENERAL,
             "Layout applied: %d of %d icons repositioned\n",
             success_count, result->changed_count);

    return success_count;
}

BOOL itidy_handle_ram_icon_persistence(struct Window *parent_window,
                                        const iTidy_BackdropList *list,
                                        const iTidy_WBLayoutResult *result)
{
    int i;

    if (!list || !result)
        return TRUE;

    /* Check if RAM: was repositioned */
    for (i = 0; i < result->count; i++)
    {
        const iTidy_WBLayoutEntry *le = &result->entries[i];
        const iTidy_BackdropEntry *entry;

        if (!le->changed)
            continue;

        if (le->entry_index < 0 || le->entry_index >= list->count)
            continue;

        entry = &list->entries[le->entry_index];

        /* Check if this is the RAM: device icon */
        if (Stricmp(entry->device_name, "RAM") == 0 &&
            entry->status == ITIDY_ENTRY_DEVICE_ICON)
        {
            /* RAM: was repositioned - copy to ENVARC for persistence */
            log_info(LOG_GENERAL,
                     "RAM: icon was repositioned - "
                     "copying to ENVARC:Sys/def_RAM.info\n");

            {
                struct Process *proc = (struct Process *)FindTask(NULL);
                APTR old_window_ptr = proc->pr_WindowPtr;
                BPTR src_lock;

                proc->pr_WindowPtr = (APTR)-1;

                /* Check if RAM:Disk.info exists */
                src_lock = Lock((STRPTR)"RAM:Disk.info", ACCESS_READ);
                if (src_lock)
                {
                    BPTR src_file, dst_file;
                    char buffer[512];
                    LONG bytes_read;
                    BOOL copy_ok = FALSE;

                    UnLock(src_lock);

                    /* Copy RAM:Disk.info to ENVARC:Sys/def_RAM.info */
                    src_file = Open((STRPTR)"RAM:Disk.info",
                                   MODE_OLDFILE);
                    if (src_file)
                    {
                        dst_file = Open(
                            (STRPTR)"ENVARC:Sys/def_RAM.info",
                            MODE_NEWFILE);
                        if (dst_file)
                        {
                            copy_ok = TRUE;
                            while ((bytes_read = Read(src_file, buffer,
                                                      sizeof(buffer))) > 0)
                            {
                                if (Write(dst_file, buffer, bytes_read) !=
                                    bytes_read)
                                {
                                    copy_ok = FALSE;
                                    break;
                                }
                            }
                            Close(dst_file);
                        }
                        Close(src_file);
                    }

                    if (copy_ok)
                    {
                        log_info(LOG_GENERAL,
                                 "RAM: icon persisted to "
                                 "ENVARC:Sys/def_RAM.info\n");
                    }
                    else
                    {
                        log_error(LOG_GENERAL,
                                  "Failed to persist RAM: icon "
                                  "to ENVARC\n");
                    }
                }

                proc->pr_WindowPtr = old_window_ptr;
            }

            return TRUE;
        }
    }

    return TRUE; /* RAM: not found or not changed - that's OK */
}
