/*
 * workbench_layout.c - iTidy Workbench Screen Icon Layout Engine
 * Calculates and applies icon positions for the Workbench screen
 * (device icons and left-out icons)
 *
 * v2.0 algorithm (reuses main iTidy layout engine):
 *   1. Build IconArray from BackdropList via GetIconDetailsFromDisk()
 *   2. Pre-partition: device icons (WBDISK) separate from left-outs
 *   3. Device icons: sorted by name, laid out in top row(s)
 *   4. Left-out icons: sorted & grouped by type via CalculateBlockLayout()
 *      (Drawers, Tools, Other) with variable-width columns
 *   5. Groups stacked vertically below device row
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

/* Main layout engine components */
#include "icon_types.h"
#include "icon_management.h"
#include "layout_preferences.h"
#include "layout/icon_sorter.h"
#include "layout/icon_positioner.h"
#include "layout/aspect_ratio_layout.h"
#include "layout/block_layout.h"

/*------------------------------------------------------------------------*/
/* Internal Helpers                                                       */
/*------------------------------------------------------------------------*/

/**
 * Build a FullIconDetails entry from a BackdropEntry by calling
 * GetIconDetailsFromDisk().  Returns TRUE if the icon was loaded
 * successfully, FALSE if it should be skipped (no .info file).
 *
 * The caller takes ownership of icon->icon_text and icon->icon_full_path
 * (allocated via whd_malloc).  FreeIconArray() will free them.
 */
static BOOL build_icon_detail(const iTidy_BackdropEntry *entry,
                               FullIconDetails *icon)
{
    IconDetailsFromDisk details;
    char *name_copy = NULL;
    char *path_copy = NULL;

    memset(icon, 0, sizeof(FullIconDetails));

    /* Read all icon metadata in a single disk operation */
    if (!GetIconDetailsFromDisk(entry->full_path, &details,
                                entry->display_name))
    {
        return FALSE;
    }

    /* Allocate display name */
    name_copy = (char *)whd_malloc(strlen(entry->display_name) + 1);
    if (!name_copy)
    {
        if (details.defaultTool)
            whd_free(details.defaultTool);
        return FALSE;
    }
    strcpy(name_copy, entry->display_name);

    /* Allocate full path (the save function uses this) */
    path_copy = (char *)whd_malloc(strlen(entry->full_path) + 1);
    if (!path_copy)
    {
        whd_free(name_copy);
        if (details.defaultTool)
            whd_free(details.defaultTool);
        return FALSE;
    }
    strcpy(path_copy, entry->full_path);

    /* Map fields exactly as icon_management.c does */
    icon->icon_x         = details.position.x;
    icon->icon_y         = details.position.y;
    icon->icon_width     = details.iconWithEmboss.width;
    icon->icon_height    = details.iconWithEmboss.height;
    icon->border_width   = details.borderWidth;
    icon->text_width     = details.textSize.width;
    icon->text_height    = details.textSize.height;
    icon->icon_max_width = details.totalDisplaySize.width;
    icon->icon_max_height = details.totalDisplaySize.height;
    icon->workbench_type = details.workbenchType;
    icon->default_tool   = details.defaultTool; /* ownership transferred */
    icon->icon_text      = name_copy;
    icon->icon_full_path = path_copy;
    icon->icon_type      = details.isNewIcon  ? icon_type_newIcon :
                           details.isOS35Icon ? icon_type_os35 :
                           icon_type_standard;
    icon->is_folder      = (details.workbenchType == WBDRAWER ||
                            details.workbenchType == WBDISK);
    icon->is_write_protected = FALSE;
    icon->file_size      = 0;
    memset(&icon->file_date, 0, sizeof(struct DateStamp));

    return TRUE;
}

/**
 * Build LayoutPreferences tuned for Workbench screen layout.
 * Uses block grouping by type, sort by name, full-screen width.
 */
static void build_wb_prefs(LayoutPreferences *prefs)
{
    memset(prefs, 0, sizeof(LayoutPreferences));

    prefs->layoutMode               = LAYOUT_MODE_ROW;
    prefs->sortOrder                = SORT_ORDER_HORIZONTAL;
    prefs->sortPriority             = SORT_PRIORITY_MIXED;
    prefs->sortBy                   = SORT_BY_NAME;
    prefs->reverseSort              = FALSE;

    prefs->centerIconsInColumn      = TRUE;
    prefs->useColumnWidthOptimization = TRUE;
    prefs->textAlignment            = TEXT_ALIGN_TOP;

    prefs->blockGroupMode           = BLOCK_GROUP_BY_TYPE;
    prefs->blockGapSize             = BLOCK_GAP_MEDIUM;

    prefs->resizeWindows            = FALSE;
    prefs->minIconsPerRow           = 2;
    prefs->maxIconsPerRow           = 0;   /* auto */
    prefs->maxWindowWidthPct        = 100; /* full screen */
    prefs->aspectRatio              = 5000; /* wide desktop */
    prefs->overflowMode             = OVERFLOW_HORIZONTAL;
    prefs->windowPositionMode       = WINDOW_POS_NO_CHANGE;

    prefs->iconSpacingX             = ITIDY_WB_MARGIN_LEFT;
    prefs->iconSpacingY             = 8;
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

/**
 * Find the backdrop list entry index matching a given full path.
 * Used after sorting/positioning to correctly map icons back to entries.
 */
static int find_entry_by_path(const iTidy_BackdropList *list,
                              const char *full_path)
{
    int i;
    for (i = 0; i < list->count; i++)
    {
        if (Stricmp((STRPTR)list->entries[i].full_path,
                    (STRPTR)full_path) == 0)
            return i;
    }
    return -1;
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
    IconArray *device_icons = NULL;
    IconArray *leftout_icons = NULL;
    LayoutPreferences wb_prefs;
    int device_count = 0;
    int leftout_count = 0;
    int device_row_height = 0;
    int device_columns = 0;
    int device_rows = 0;
    int leftout_width = 0;
    int leftout_height = 0;
    int old_screen_width;
    int i;
    BOOL ok = FALSE;

    if (!params || !list || !result)
        return FALSE;

    itidy_init_wb_layout_result(result);

    if (list->count == 0)
        return TRUE;

    /* Temporarily set the global screenWidth for the layout engine */
    old_screen_width = screenWidth;
    screenWidth = params->screen_width;

    /* Build preset preferences for desktop layout */
    build_wb_prefs(&wb_prefs);

    /* Create two IconArrays: one for device icons, one for left-outs */
    device_icons = CreateIconArray();
    leftout_icons = CreateIconArray();
    if (!device_icons || !leftout_icons)
        goto cleanup;

    /* Suppress DOS requesters during icon loading */
    {
        struct Process *proc = (struct Process *)FindTask(NULL);
        APTR old_window_ptr = proc->pr_WindowPtr;
        proc->pr_WindowPtr = (APTR)-1;

        for (i = 0; i < list->count; i++)
        {
            const iTidy_BackdropEntry *entry = &list->entries[i];
            FullIconDetails icon;

            /* Only layout valid entries and device icons */
            if (entry->status != ITIDY_ENTRY_VALID &&
                entry->status != ITIDY_ENTRY_DEVICE_ICON)
                continue;

            /* Try to load icon details from disk */
            if (!build_icon_detail(entry, &icon))
            {
                log_info(LOG_GENERAL,
                         "Skipping (no .info): %s\n", entry->full_path);
                continue;
            }

            /* Partition into device vs left-out */
            if (entry->status == ITIDY_ENTRY_DEVICE_ICON)
            {
                if (AddIconToArray(device_icons, &icon))
                {
                    device_count++;
                }
                else
                {
                    /* AddIconToArray failed - clean up the icon strings */
                    if (icon.icon_text) whd_free(icon.icon_text);
                    if (icon.icon_full_path) whd_free(icon.icon_full_path);
                    if (icon.default_tool) whd_free(icon.default_tool);
                }
            }
            else
            {
                if (AddIconToArray(leftout_icons, &icon))
                {
                    leftout_count++;
                }
                else
                {
                    if (icon.icon_text) whd_free(icon.icon_text);
                    if (icon.icon_full_path) whd_free(icon.icon_full_path);
                    if (icon.default_tool) whd_free(icon.default_tool);
                }
            }
        }

        proc->pr_WindowPtr = old_window_ptr;
    }

    log_info(LOG_GENERAL,
             "WB layout: %d device icons, %d left-out icons loaded\n",
             device_count, leftout_count);

    if (device_count == 0 && leftout_count == 0)
    {
        ok = TRUE;
        goto cleanup;
    }

    /*--------------------------------------------------------------------*/
    /* Phase 1: Layout device icons in top row(s)                         */
    /*--------------------------------------------------------------------*/
    if (device_count > 0)
    {
        /* Sort device icons by name */
        SortIconArrayWithPreferences(device_icons, &wb_prefs);

        /* Calculate optimal columns for device row */
        CalculateLayoutWithAspectRatio(device_icons, &wb_prefs,
                                       &device_columns, &device_rows);

        /* Position device icons starting at margin_top */
        /* Temporarily set iconSpacingY to margin_top for the first row */
        wb_prefs.iconSpacingY = params->margin_top;
        CalculateLayoutPositions(device_icons, &wb_prefs, device_columns);
        wb_prefs.iconSpacingY = 8; /* restore */

        /* Measure device row height */
        device_row_height = 0;
        for (i = 0; i < (int)device_icons->size; i++)
        {
            int bottom = device_icons->array[i].icon_y +
                         device_icons->array[i].icon_max_height;
            if (bottom > device_row_height)
                device_row_height = bottom;
        }

        /* Add result entries for device icons.
         * Match by icon_full_path since sorting reordered the array. */
        for (i = 0; i < (int)device_icons->size; i++)
        {
            iTidy_WBLayoutEntry le;
            int entry_idx = find_entry_by_path(
                list, device_icons->array[i].icon_full_path);

            if (entry_idx < 0)
                continue;

            le.entry_index = entry_idx;
            le.new_x = device_icons->array[i].icon_x;
            le.new_y = device_icons->array[i].icon_y;
            le.old_x = list->entries[entry_idx].icon_x;
            le.old_y = list->entries[entry_idx].icon_y;
            le.changed = (le.new_x != le.old_x || le.new_y != le.old_y);

            if (le.changed)
                result->changed_count++;

            if (!grow_layout_result(result))
                goto cleanup;

            result->entries[result->count] = le;
            result->count++;
        }
    }

    /*--------------------------------------------------------------------*/
    /* Phase 2: Layout left-out icons grouped by type below devices       */
    /*--------------------------------------------------------------------*/
    if (leftout_count > 0)
    {
        int y_offset;

        /* Calculate Y offset: below device row + gap */
        if (device_row_height > 0)
            y_offset = device_row_height + BLOCK_GAP_MEDIUM_PX;
        else
            y_offset = params->margin_top;

        /* Use block layout (groups: Drawers, Tools, Other) */
        if (wb_prefs.blockGroupMode == BLOCK_GROUP_BY_TYPE)
        {
            CalculateBlockLayout(leftout_icons, &wb_prefs,
                                 &leftout_width, &leftout_height);
        }
        else
        {
            int cols, rows;
            SortIconArrayWithPreferences(leftout_icons, &wb_prefs);
            CalculateLayoutWithAspectRatio(leftout_icons, &wb_prefs,
                                           &cols, &rows);
            CalculateLayoutPositionsWithColumnCentering(leftout_icons,
                                                        &wb_prefs, cols);
        }

        /* Debug: dump positions after CalculateBlockLayout */
        log_debug(LOG_GENERAL,
                  "After block layout, left-out positions (%d icons):\n",
                  (int)leftout_icons->size);
        for (i = 0; i < (int)leftout_icons->size; i++)
        {
            log_debug(LOG_GENERAL,
                      "  [%d] %-20s X=%-4d Y=%-4d maxH=%-3d path=%s\n",
                      i,
                      leftout_icons->array[i].icon_text,
                      leftout_icons->array[i].icon_x,
                      leftout_icons->array[i].icon_y,
                      leftout_icons->array[i].icon_max_height,
                      leftout_icons->array[i].icon_full_path);
        }

        /* Apply Y offset — shift all left-out icons down below device row.
         * The positioning functions start layout from (spacingX, spacingY),
         * so we add (y_offset - spacingY) to move them below devices. */
        {
            int shift_y = y_offset - wb_prefs.iconSpacingY;

            log_debug(LOG_GENERAL,
                      "Y-shift: y_offset=%d, spacingY=%d, shift=%d\n",
                      y_offset, wb_prefs.iconSpacingY, shift_y);

            for (i = 0; i < (int)leftout_icons->size; i++)
            {
                leftout_icons->array[i].icon_y += shift_y;
            }
        }

        /* Debug: dump final positions after Y-shift */
        log_debug(LOG_GENERAL, "Final left-out positions after Y-shift:\n");
        for (i = 0; i < (int)leftout_icons->size; i++)
        {
            log_debug(LOG_GENERAL,
                      "  [%d] %-20s X=%-4d Y=%-4d\n",
                      i,
                      leftout_icons->array[i].icon_text,
                      leftout_icons->array[i].icon_x,
                      leftout_icons->array[i].icon_y);
        }

        /* Add result entries for left-out icons.
         * Match by icon_full_path since sorting/block layout
         * reordered the array. */
        for (i = 0; i < (int)leftout_icons->size; i++)
        {
            iTidy_WBLayoutEntry le;
            int entry_idx = find_entry_by_path(
                list, leftout_icons->array[i].icon_full_path);

            if (entry_idx < 0)
                continue;

            le.entry_index = entry_idx;
            le.new_x = leftout_icons->array[i].icon_x;
            le.new_y = leftout_icons->array[i].icon_y;
            le.old_x = list->entries[entry_idx].icon_x;
            le.old_y = list->entries[entry_idx].icon_y;
            le.changed = (le.new_x != le.old_x || le.new_y != le.old_y);

            if (le.changed)
                result->changed_count++;

            if (!grow_layout_result(result))
                goto cleanup;

            result->entries[result->count] = le;
            result->count++;
        }
    }

    log_info(LOG_GENERAL,
             "Layout calculated: %d icons (%d device + %d left-out), "
             "%d changed positions\n",
             result->count, device_count, leftout_count,
             result->changed_count);

    ok = TRUE;

cleanup:
    /* Restore global screenWidth */
    screenWidth = old_screen_width;

    /* Free temporary IconArrays (FreeIconArray frees strings too) */
    if (device_icons)
        FreeIconArray(device_icons);
    if (leftout_icons)
        FreeIconArray(leftout_icons);
    return ok;
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

        /* Write ALL icons, not just changed ones.
         * This ensures ICONPUTA_NotifyWorkbench fires for every icon,
         * forcing Workbench to refresh backdrop icon positions. */

        if (le->entry_index < 0 || le->entry_index >= list->count)
            continue;

        entry = &list->entries[le->entry_index];

        log_debug(LOG_GENERAL,
                  "Saving icon [%d]: path='%s' pos=(%ld,%ld)%s\n",
                  i, entry->full_path,
                  (long)le->new_x, (long)le->new_y,
                  le->changed ? " (CHANGED)" : " (unchanged)");

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
                if (le->changed)
                {
                    log_info(LOG_GENERAL,
                             "Repositioned: %s (%ld,%ld) -> (%ld,%ld)\n",
                             entry->full_path,
                             (long)le->old_x, (long)le->old_y,
                             (long)le->new_x, (long)le->new_y);
                }
                else
                {
                    log_info(LOG_GENERAL,
                             "Refreshed (pos unchanged): %s (%ld,%ld)\n",
                             entry->full_path,
                             (long)le->new_x, (long)le->new_y);
                }
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
             "Layout applied: %d of %d icons written (%d had new positions)\n",
             success_count, result->count, result->changed_count);

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
