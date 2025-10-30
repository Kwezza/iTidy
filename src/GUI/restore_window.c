/*
 * restore_window.c - iTidy Restore Window Implementation
 * GadTools-based Backup Restore GUI for Workbench 2.0+
 * Based on RESTORE_WINDOW_GUI_SPEC.md specification
 * 
 * COLUMN ALIGNMENT - SYSTEM DEFAULT FONT:
 * ========================================
 * The run list uses space-based column alignment which requires a
 * FIXED-WIDTH (monospaced) font. To ensure proper alignment regardless
 * of the screen font, this window uses the System Default Text font
 * (typically Topaz) for all gadgets.
 * 
 * Even if the Workbench screen uses a proportional font (like Helvetica)
 * for Screen Text, the listviews will use the System Default Text font,
 * which is defined in Workbench Preferences and is typically fixed-width.
 * 
 * The code:
 * - Opens the System Default Text font (GfxBase->DefaultFont)
 * - Uses it for all gadgets via ng.ng_TextAttr
 * - Closes the font in cleanup
 * 
 * This ensures column alignment works correctly with the format:
 * "Run_0007  2025-10-25 14:32    63 folders   46 KB  Complete"
 * 
 * Technical details:
 * - Listview width: font_width * 65 characters
 * - Format width: ~65 chars using %-9s, %-16s, %11s, %8s
 * - Columns align because character widths are consistent
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/dos.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>

#include "restore_window.h"
#include "../backup_restore.h"
#include "../writeLog.h"
#include "folder_view_window.h"
#include "test_simple_window.h"  /* TESTING: Simple window test */
#include "../backup_runs.h"
#include "../backup_catalog.h"
#include "../backup_restore.h"
#include "../writeLog.h"
#include "../Settings/IControlPrefs.h"

/*------------------------------------------------------------------------*/
/* External Global Variables                                             */
/*------------------------------------------------------------------------*/
extern struct IControlPrefsDetails prefsIControl;

/*------------------------------------------------------------------------*/
/* Window Title                                                           */
/*------------------------------------------------------------------------*/
#define RESTORE_WINDOW_TITLE "iTidy - Restore Backups"

/*------------------------------------------------------------------------*/
/* Helper function to update window max dimensions                       */
/*------------------------------------------------------------------------*/
static void update_window_max_dimensions(UWORD *current_max_width,
                                        UWORD *current_max_height,
                                        UWORD gadget_right,
                                        UWORD gadget_bottom)
{
    if (gadget_right > *current_max_width)
        *current_max_width = gadget_right;
    
    if (gadget_bottom > *current_max_height)
        *current_max_height = gadget_bottom;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Format a size in bytes to human-readable string
 */
/*------------------------------------------------------------------------*/
void format_size_string(ULONG bytes, char *buffer)
{
    if (bytes == 0)
    {
        strcpy(buffer, "0 KB");
    }
    else if (bytes < 1024)
    {
        /* For bytes, add trailing space to align with KB/MB/GB (2-letter units) */
        if (bytes < 10)
            sprintf(buffer, " %lu B ", bytes);  /* Leading space for single digit, trailing space for unit alignment */
        else
            sprintf(buffer, "%lu B ", bytes);   /* Trailing space for unit alignment */
    }
    else if (bytes < 1048576)  /* < 1 MB */
    {
        sprintf(buffer, "%lu KB", (bytes + 512) / 1024);
    }
    else if (bytes < 1073741824)  /* < 1 GB */
    {
        sprintf(buffer, "%.1f MB", (float)bytes / 1048576.0f);
    }
    else
    {
        sprintf(buffer, "%.2f GB", (float)bytes / 1073741824.0f);
    }
}

/*------------------------------------------------------------------------*/
/**
 * @brief Format DateStamp to human-readable string
 * 
 * Converts an Amiga DateStamp (days since 1978, minutes, ticks) into
 * a formatted string: "YYYY-MM-DD HH:MM:SS"
 */
/*------------------------------------------------------------------------*/
static void format_datestamp_string(struct DateStamp *ds, char *buffer, int bufferSize)
{
    ULONG days, minutes, seconds;
    ULONG year, month, day, hour, minute;
    ULONG daysSinceEpoch;
    
    if (ds == NULL || buffer == NULL || bufferSize < 20)
    {
        if (buffer != NULL && bufferSize > 0)
            strcpy(buffer, "Unknown");
        return;
    }
    
    days = ds->ds_Days;
    minutes = ds->ds_Minute;
    seconds = ds->ds_Tick / 50;  /* Ticks are 1/50th of a second */
    
    /* Convert minutes to hours and minutes */
    hour = minutes / 60;
    minute = minutes % 60;
    
    /* Days since Jan 1, 1978 - convert to calendar date */
    /* Simple algorithm: count years and remaining days */
    daysSinceEpoch = days;
    year = 1978;
    
    /* Account for leap years */
    while (1)
    {
        ULONG daysInYear = 365;
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
            daysInYear = 366;
        
        if (daysSinceEpoch >= daysInYear)
        {
            daysSinceEpoch -= daysInYear;
            year++;
        }
        else
            break;
    }
    
    /* Count months */
    {
        ULONG daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        BOOL isLeap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
        
        if (isLeap)
            daysInMonth[1] = 29;
        
        month = 1;
        while (month <= 12 && daysSinceEpoch >= daysInMonth[month - 1])
        {
            daysSinceEpoch -= daysInMonth[month - 1];
            month++;
        }
        
        day = daysSinceEpoch + 1;  /* Days are 1-based */
    }
    
    /* Format the string */
    snprintf(buffer, bufferSize, "%04lu-%02lu-%02lu %02lu:%02lu:%02lu",
             year, month, day, hour, minute, seconds);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Draw window background with checkerboard pattern and recessed panel
 * 
 * Creates the classic Amiga dialog look:
 * 1. Fill entire window with checkerboard pattern
 * 2. Draw large recessed panel for main content area
 * 3. Buttons remain on checkerboard pattern at bottom
 * 
 * @param restore_data Pointer to restore window data structure
 */
/*------------------------------------------------------------------------*/
static void draw_window_background(struct iTidyRestoreWindow *restore_data)
{
    struct RastPort *rp;
    WORD content_left, content_top, content_width, content_height;
    WORD window_inner_left, window_inner_top;
    WORD window_inner_width, window_inner_height;
    struct DrawInfo *dri;
    static UWORD checkerboard_pattern[2] = {0x5555, 0xAAAA};
    
    if (restore_data == NULL || restore_data->window == NULL)
        return;
    
    if (restore_data->run_list == NULL || restore_data->details_listview == NULL)
        return;
    
    rp = restore_data->window->RPort;
    dri = GetScreenDrawInfo(restore_data->screen);
    if (dri == NULL)
        return;
    
    /* Get window's inner dimensions (excluding borders) */
    window_inner_left = restore_data->window->BorderLeft;
    window_inner_top = restore_data->window->BorderTop;
    window_inner_width = restore_data->window->Width - 
                         restore_data->window->BorderLeft - 
                         restore_data->window->BorderRight;
    window_inner_height = restore_data->window->Height - 
                          restore_data->window->BorderTop - 
                          restore_data->window->BorderBottom;
    
    /* Fill entire window interior with checkerboard pattern */
    /* Use pen 0 (light gray) and pen 1 (white) for the classic Amiga look */
    SetAPen(rp, 0);  /* Light gray background */
    SetBPen(rp, 2);  /* White */
    SetDrMd(rp, JAM2);
    
    /* Use alternating pattern (checkerboard) */
    SetAfPt(rp, checkerboard_pattern, 1);
    
    RectFill(rp, 
             window_inner_left, 
             window_inner_top,
             window_inner_left + window_inner_width - 1,
             window_inner_top + window_inner_height - 1);
    
    /* Clear the pattern */
    SetAfPt(rp, NULL, 0);
    
    /* Calculate recessed panel dimensions */
    /* Panel should contain: both listviews, but NOT the buttons */
    /* Bevel starts RESTORE_BEVEL_BORDER pixels from the content */
    content_left = restore_data->run_list->LeftEdge - RESTORE_CONTENT_PADDING - RESTORE_BEVEL_BORDER;
    content_top = restore_data->run_list->TopEdge - RESTORE_CONTENT_PADDING - RESTORE_BEVEL_BORDER;
    
    /* Width matches the listview width + content padding on both sides + bevel border on both sides */
    content_width = restore_data->run_list->Width + (2 * RESTORE_CONTENT_PADDING) + (2 * RESTORE_BEVEL_BORDER);
    
    /* Height goes from top to bottom of details listview + padding + border */
    content_height = (restore_data->details_listview->TopEdge + 
                      restore_data->details_listview->Height + 
                      RESTORE_CONTENT_PADDING + RESTORE_BEVEL_BORDER) - content_top;
    
    /* Fill the content area with solid background color (clear the checkerboard) */
    /* This fills the area inside the bevel plus the 4-pixel border */
    SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
    SetDrMd(rp, JAM1);
    RectFill(rp,
             content_left,
             content_top,
             content_left + content_width - 1,
             content_top + content_height - 1);
    
    /* Draw the large recessed bevel box for content area */
    DrawBevelBox(rp,
                content_left,
                content_top,
                content_width,
                content_height,
                GT_VisualInfo, restore_data->visual_info,
                GTBB_Recessed, TRUE,  /* Sunken/recessed effect */
                TAG_END);
    
    FreeScreenDrawInfo(restore_data->screen, dri);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Calculate optimal column widths based on listview width
 * 
 * This function calculates column widths that maintain alignment across
 * different font sizes by using the actual character width.
 * 
 * @param listview_width_chars Total width in characters (typically 65)
 * @param col_widths Output array for column widths [5 columns]
 */
/*------------------------------------------------------------------------*/
static void calculate_column_widths(UWORD listview_width_chars, UWORD *col_widths)
{
    /* Default column widths for 65-character listview:
     * Run Name:     9 chars  "Run_0007"
     * Date/Time:   16 chars  "2025-10-25 14:32"
     * Folders:     11 chars  "63 folders"
     * Size:         8 chars  "46 KB"
     * Status:      remainder  "Complete"
     * Separators:   8 chars  (2 spaces x 4)
     * Total:       52 chars + status
     */
    
    if (col_widths == NULL)
        return;
    
    /* Fixed columns that don't change */
    col_widths[0] = 9;   /* Run Name */
    col_widths[1] = 16;  /* Date/Time */
    col_widths[2] = 11;  /* Folder Count */
    col_widths[3] = 8;   /* Size */
    
    /* Calculate remaining space for status */
    UWORD used = col_widths[0] + col_widths[1] + col_widths[2] + col_widths[3];
    UWORD separators = 8;  /* 2 spaces x 4 gaps */
    
    if (listview_width_chars > used + separators)
        col_widths[4] = listview_width_chars - used - separators;
    else
        col_widths[4] = 10;  /* Minimum for "Complete" */
}

/*------------------------------------------------------------------------*/
/**
 * @brief Format a run list entry for display
 * 
 * NOTE: This function assumes a FIXED-WIDTH font (like Topaz).
 * With proportional fonts, column alignment will not work properly.
 * The code detects proportional fonts and logs a warning.
 */
/*------------------------------------------------------------------------*/
static void format_run_list_entry(struct RestoreRunEntry *entry, char *buffer)
{
    /* Format for fixed-width fonts with column alignment:
     * Layout: "Run_0007  2025-10-25 14:32    63 folders   46 KB  Complete"
     * 
     * Column widths (for fixed-width fonts):
     * - Run Name: 9 chars (fixed, "Run_0000" to "Run_9999")
     * - Separator: 2 spaces
     * - Date/Time: 16 chars (fixed, "YYYY-MM-DD HH:MM")
     * - Separator: 2 spaces  
     * - Folder Count: 11 chars ("1 folders" to "999 folders") - right-aligned
     * - Separator: 2 spaces
     * - Size: 8 chars (right-aligned, "46 KB" to "999 MB")
     * - Separator: 2 spaces
     * - Status: remaining space ("Complete", "Incomplete", "Orphaned")
     * 
     * IMPORTANT: If using proportional fonts, consider changing Workbench
     * screen font to a fixed-width font like Topaz for proper alignment.
     */
    
    /* Extract just date and time (first 16 chars of dateStr) */
    char short_date[20];
    if (strlen(entry->dateStr) >= 16)
    {
        strncpy(short_date, entry->dateStr, 16);
        short_date[16] = '\0';
    }
    else
    {
        strcpy(short_date, entry->dateStr);
    }
    
    /* Format folder count with proper spacing and singular/plural */
    char folder_str[16];
    if (entry->folderCount == 1)
        sprintf(folder_str, "%lu folder ", entry->folderCount);  /* Singular with trailing space for alignment */
    else
        sprintf(folder_str, "%lu folders", entry->folderCount);  /* Plural */
    
    /* Build the display string with column alignment for fixed-width fonts
     * This will NOT align properly with proportional fonts!
     */
    sprintf(buffer, "%-9s \x7C %-16s \x7C %11s \x7C %8s \x7C %s",
        entry->runName,           /* "Run_0007" - left-aligned, 9 chars */
        short_date,               /* "2025-10-25 14:32" - left-aligned, 16 chars */
        folder_str,               /* "63 folders" - right-aligned, 11 chars */
        entry->sizeStr,           /* "46 KB" - right-aligned, 8 chars */
        entry->statusStr);        /* "Complete" - remaining space */
}

/*------------------------------------------------------------------------*/
/* Helper structure for catalog statistics                               */
/*------------------------------------------------------------------------*/
struct CatalogStatsContext
{
    ULONG count;
    ULONG totalBytes;
};

/*------------------------------------------------------------------------*/
/* Callback to accumulate catalog statistics                             */
/*------------------------------------------------------------------------*/
static BOOL catalog_stats_callback(const BackupArchiveEntry *entry, void *userData)
{
    struct CatalogStatsContext *stats = (struct CatalogStatsContext *)userData;
    
    if (stats != NULL && entry != NULL)
    {
        stats->count++;
        stats->totalBytes += entry->sizeBytes;
    }
    
    return TRUE;  /* Continue parsing */
}

/*------------------------------------------------------------------------*/
/**
 * @brief Extract source directory from catalog header
 */
/*------------------------------------------------------------------------*/
static BOOL extract_source_directory(const char *catalog_path, char *buffer, ULONG buffer_size)
{
    BPTR file;
    char line[512];
    BOOL found = FALSE;
    
    if (!catalog_path || !buffer || buffer_size == 0)
        return FALSE;
    
    buffer[0] = '\0';
    
    file = Open((STRPTR)catalog_path, MODE_OLDFILE);
    if (!file)
        return FALSE;
    
    /* Read lines looking for "Source Directory: " */
    while (FGets(file, line, sizeof(line)))
    {
        /* Remove trailing newline */
        ULONG len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';
        
        /* Check if this is the source directory line */
        if (strncmp(line, "Source Directory: ", 18) == 0)
        {
            /* Extract the path after the label */
            const char *path = line + 18;
            strncpy(buffer, path, buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
            found = TRUE;
            break;
        }
        
        /* Stop at the table separator (end of header) */
        if (strstr(line, "----") != NULL)
            break;
    }
    
    Close(file);
    return found;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Scan backup directory for available runs
 */
/*------------------------------------------------------------------------*/
ULONG scan_backup_runs(const char *backup_root, 
                       struct RestoreRunEntry **entries_out)
{
    UWORD highest_run;
    ULONG count = 0;
    ULONG i;
    struct RestoreRunEntry *entries = NULL;
    char run_path[256];
    char catalog_path[300];
    BPTR lock;
    
    if (backup_root == NULL || entries_out == NULL)
        return 0;
    
    /* Find highest run number */
    highest_run = FindHighestRunNumber(backup_root);
    if (highest_run == 0)
    {
        append_to_log("scan_backup_runs: No backup runs found in %s\n", backup_root);
        *entries_out = NULL;
        return 0;
    }
    
    append_to_log("scan_backup_runs: Highest run number = %u\n", highest_run);
    
    /* Allocate entries array (max possible size) */
    entries = (struct RestoreRunEntry *)AllocVec(
        sizeof(struct RestoreRunEntry) * highest_run,
        MEMF_CLEAR);
    
    if (entries == NULL)
    {
        append_to_log("scan_backup_runs: Failed to allocate memory for entries\n");
        return 0;
    }
    
    /* Scan each run from 1 to highest */
    for (i = 1; i <= highest_run; i++)
    {
        struct FileInfoBlock *fib;
        struct DateStamp runDate;
        BOOL hasDate = FALSE;
        
        /* Build run directory path */
        if (!GetRunDirectoryPath(run_path, backup_root, (UWORD)i))
            continue;
        
        /* Check if run directory exists and get its timestamp */
        lock = Lock(run_path, ACCESS_READ);
        if (lock == 0)
            continue;  /* Directory doesn't exist, skip */
        
        /* Get directory timestamp */
        fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
        if (fib != NULL)
        {
            if (Examine(lock, fib))
            {
                runDate = fib->fib_Date;
                hasDate = TRUE;
            }
            FreeDosObject(DOS_FIB, fib);
        }
        
        UnLock(lock);
        
        /* We have a valid run directory */
        struct RestoreRunEntry *entry = &entries[count];
        entry->runNumber = (UWORD)i;
        FormatRunDirectoryName(entry->runName, (UWORD)i);
        strcpy(entry->fullPath, run_path);
        
        /* Format the timestamp */
        if (hasDate)
        {
            format_datestamp_string(&runDate, entry->dateStr, sizeof(entry->dateStr));
        }
        else
        {
            strcpy(entry->dateStr, "Unknown");
        }
        
        /* Check for catalog.txt */
        sprintf(catalog_path, "%s/catalog.txt", run_path);
        lock = Lock(catalog_path, ACCESS_READ);
        entry->hasCatalog = (lock != 0);
        if (lock != 0)
            UnLock(lock);
        
        if (entry->hasCatalog)
        {
            struct CatalogStatsContext stats;
            
            /* Parse catalog to get folder count and total size */
            stats.count = 0;
            stats.totalBytes = 0;
            ParseCatalog(catalog_path, catalog_stats_callback, &stats);
            
            entry->folderCount = stats.count;
            entry->totalBytes = stats.totalBytes;
            format_size_string(stats.totalBytes, entry->sizeStr);
            entry->statusCode = RESTORE_STATUS_COMPLETE;
            strcpy(entry->statusStr, "Complete");
            
            /* Extract source directory from catalog header */
            if (!extract_source_directory(catalog_path, entry->sourceDirectory, 
                                         sizeof(entry->sourceDirectory)))
            {
                strcpy(entry->sourceDirectory, "(Unknown)");
            }
        }
        else
        {
            entry->folderCount = 0;
            entry->totalBytes = 0;
            strcpy(entry->sizeStr, "N/A");
            entry->statusCode = RESTORE_STATUS_ORPHANED;
            strcpy(entry->statusStr, "NoCAT");
            strcpy(entry->sourceDirectory, "(No catalog)");
        }
        
        /* Format display string */
        format_run_list_entry(entry, entry->displayString);
        
        count++;
    }
    
    append_to_log("scan_backup_runs: Found %lu valid run directories\n", count);
    
    *entries_out = entries;
    return count;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Populate list view with backup run entries
 */
/*------------------------------------------------------------------------*/
void populate_run_list(struct iTidyRestoreWindow *restore_data,
                       struct RestoreRunEntry *entries,
                       ULONG count)
{
    ULONG i;
    struct Node *node;
    
    if (restore_data == NULL || restore_data->run_list == NULL)
        return;
    
    /* Detach list from gadget */
    GT_SetGadgetAttrs(restore_data->run_list, restore_data->window, NULL,
        GTLV_Labels, ~0,
        TAG_END);
    
    /* Free existing list */
    if (restore_data->run_list_strings != NULL)
    {
        while ((node = RemHead(restore_data->run_list_strings)) != NULL)
        {
            FreeVec(node);
        }
        FreeVec(restore_data->run_list_strings);
    }
    
    /* Create new list */
    restore_data->run_list_strings = (struct List *)AllocVec(
        sizeof(struct List),
        MEMF_CLEAR);
    
    if (restore_data->run_list_strings == NULL)
        return;
    
    NewList(restore_data->run_list_strings);
    
    /* Add entries to list (in reverse order - newest first) */
    for (i = 0; i < count; i++)
    {
        ULONG idx = count - 1 - i;  /* Reverse order */
        node = (struct Node *)AllocVec(sizeof(struct Node) + 
                                       strlen(entries[idx].displayString) + 1,
                                       MEMF_CLEAR);
        if (node != NULL)
        {
            node->ln_Name = (char *)(node + 1);
            strcpy(node->ln_Name, entries[idx].displayString);
            AddTail(restore_data->run_list_strings, node);
        }
    }
    
    /* Re-attach list to gadget */
    GT_SetGadgetAttrs(restore_data->run_list, restore_data->window, NULL,
        GTLV_Labels, restore_data->run_list_strings,
        GTLV_Selected, (count > 0) ? 0 : ~0,
        TAG_END);
    
    /* Select first entry if any */
    if (count > 0)
    {
        restore_data->selected_run_index = 0;
        update_details_panel(restore_data, &entries[count - 1]);  /* Newest run */
    }
}

/*------------------------------------------------------------------------*/
/**
 * @brief Update details panel with selected run information
 */
/*------------------------------------------------------------------------*/
void update_details_panel(struct iTidyRestoreWindow *restore_data,
                          struct RestoreRunEntry *selected_entry)
{
    struct Node *node;
    struct List *list;
    int i;
    char *detail_lines[7];
    char line_buffer[7][256];
    
    if (restore_data == NULL || restore_data->details_listview == NULL)
        return;
    
    /* Detach list from gadget */
    GT_SetGadgetAttrs(restore_data->details_listview, restore_data->window, NULL,
        GTLV_Labels, ~0,
        TAG_END);
    
    /* Free existing list */
    if (restore_data->details_list_strings != NULL)
    {
        while ((node = RemHead(restore_data->details_list_strings)) != NULL)
        {
            if (node->ln_Name != NULL)
                FreeVec(node->ln_Name);
            FreeVec(node);
        }
    }
    else
    {
        /* Allocate list for first time */
        restore_data->details_list_strings = (struct List *)AllocVec(sizeof(struct List), MEMF_CLEAR);
        if (restore_data->details_list_strings == NULL)
            return;
        NewList(restore_data->details_list_strings);
    }
    
    list = restore_data->details_list_strings;
    
    if (selected_entry == NULL)
    {
        /* No selection - show placeholder */
        strcpy(line_buffer[0], "(No run selected)");
        line_buffer[1][0] = '\0';
        line_buffer[2][0] = '\0';
        line_buffer[3][0] = '\0';
        line_buffer[4][0] = '\0';
        line_buffer[5][0] = '\0';
        line_buffer[6][0] = '\0';
    }
    else
    {
        char status_desc[64];
        
        /* Format status description */
        switch (selected_entry->statusCode)
        {
            case RESTORE_STATUS_COMPLETE:
                strcpy(status_desc, "Complete (catalog present)");
                break;
            case RESTORE_STATUS_INCOMPLETE:
                strcpy(status_desc, "Incomplete (missing archives)");
                break;
            case RESTORE_STATUS_ORPHANED:
                strcpy(status_desc, "Orphaned (no catalog)");
                break;
            case RESTORE_STATUS_CORRUPTED:
                strcpy(status_desc, "Corrupted (catalog error)");
                break;
            default:
                strcpy(status_desc, "Unknown");
                break;
        }
        
        /* Format each detail line */

        sprintf(line_buffer[0], "       Run Number: %04u", 
                selected_entry->runNumber);
        sprintf(line_buffer[1], "     Date Created: %s", 
                selected_entry->dateStr); 
        sprintf(line_buffer[2], " Source Directory: %s", 
                selected_entry->sourceDirectory);
        sprintf(line_buffer[3], "   Total Archives: %lu", 
                selected_entry->folderCount);
        sprintf(line_buffer[4], "       Total Size: %s", 
                selected_entry->sizeStr);
        sprintf(line_buffer[5], "           Status: %s", 
                status_desc);
        sprintf(line_buffer[6], "         Location: %s", 
                selected_entry->fullPath);
    }
    
    /* Create list nodes */
    for (i = 0; i < 7; i++)
    {
        node = (struct Node *)AllocVec(sizeof(struct Node), MEMF_CLEAR);
        if (node != NULL)
        {
            node->ln_Name = (STRPTR)AllocVec(strlen(line_buffer[i]) + 1, MEMF_CLEAR);
            if (node->ln_Name != NULL)
            {
                strcpy(node->ln_Name, line_buffer[i]);
                AddTail(list, node);
            }
            else
            {
                FreeVec(node);
            }
        }
    }
    
    /* Reattach list to gadget */
    GT_SetGadgetAttrs(restore_data->details_listview, restore_data->window, NULL,
        GTLV_Labels, list,
        TAG_END);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Perform full run restore with progress feedback
 */
/*------------------------------------------------------------------------*/
BOOL perform_restore_run(struct iTidyRestoreWindow *restore_data,
                        struct RestoreRunEntry *run_entry)
{
    RestoreStatus status;
    char message[256];
    
    if (restore_data == NULL || run_entry == NULL)
        return FALSE;
    
    /* Show confirmation requester */
    struct EasyStruct easy_struct;
    
    sprintf(message, "Restore all folders from %s?", run_entry->runName);
    
    easy_struct.es_StructSize = sizeof(struct EasyStruct);
    easy_struct.es_Flags = 0;
    easy_struct.es_Title = "Confirm Restore";
    easy_struct.es_TextFormat = message;
    easy_struct.es_GadgetFormat = "Restore|Cancel";
    
    if (!EasyRequest(restore_data->window, &easy_struct, NULL))
    {
        append_to_log("Restore cancelled by user\n");
        return FALSE;
    }
    
    append_to_log("Starting restore of run %u from %s\n",
                  run_entry->runNumber,
                  restore_data->backup_root_path);
    
    /* Initialize restore context */
    RestoreContext restoreCtx;
    if (!InitRestoreContext(&restoreCtx))
    {
        append_to_log("ERROR: Failed to initialize restore context - LHA not available\n");
        
        sprintf(message, "LHA executable not found!\nRestore requires LHA to be installed.");
        easy_struct.es_StructSize = sizeof(struct EasyStruct);
        easy_struct.es_Flags = 0;
        easy_struct.es_Title = "Restore Failed";
        easy_struct.es_TextFormat = message;
        easy_struct.es_GadgetFormat = "OK";
        EasyRequest(restore_data->window, &easy_struct, NULL);
        
        return FALSE;
    }
    
    /* Set window geometry restore flag from checkbox */
    restoreCtx.restoreWindowGeometry = restore_data->restore_window_geometry;
    append_to_log("Window geometry restore: %s\n",
                  restoreCtx.restoreWindowGeometry ? "ENABLED" : "DISABLED");
    
    /* Build full path to run directory */
    char runPath[512];
    sprintf(runPath, "%s/%s", restore_data->backup_root_path, run_entry->runName);
    
    append_to_log("Restoring from: %s\n", runPath);
    append_to_log("Restoring %lu folder(s)...\n", run_entry->folderCount);
    
    /* Perform the actual restore */
    status = RestoreFullRun(&restoreCtx, runPath);
    
    /* Log results */
    append_to_log("Restore completed with status: %s\n", GetRestoreStatusMessage(status));
    append_to_log("Archives restored: %u, Archives failed: %u\n",
                  restoreCtx.stats.archivesRestored,
                  restoreCtx.stats.archivesFailed);
    
    if (restoreCtx.stats.hasErrors)
    {
        append_to_log("First error: %s\n", restoreCtx.stats.firstError);
    }
    
    /* Show result */
    if (status == RESTORE_OK)
    {
        sprintf(message, "Successfully restored %s\n\n%u folder(s) restored\n%u failed",
                run_entry->runName,
                restoreCtx.stats.archivesRestored,
                restoreCtx.stats.archivesFailed);
        
        easy_struct.es_StructSize = sizeof(struct EasyStruct);
        easy_struct.es_Flags = 0;
        easy_struct.es_Title = "Restore Complete";
        easy_struct.es_TextFormat = message;
        easy_struct.es_GadgetFormat = "OK";
        EasyRequest(restore_data->window, &easy_struct, NULL);
        
        restore_data->restore_performed = TRUE;
    }
    else
    {
        /* Show detailed error message */
        const char *statusMsg = GetRestoreStatusMessage(status);
        
        if (restoreCtx.stats.hasErrors && restoreCtx.stats.firstError[0] != '\0')
        {
            sprintf(message, "Failed to restore %s\n\nStatus: %s\nError: %s\n\nCheck iTidy.log for details",
                    run_entry->runName,
                    statusMsg,
                    restoreCtx.stats.firstError);
        }
        else
        {
            sprintf(message, "Failed to restore %s\n\nStatus: %s\n\nCheck iTidy.log for details",
                    run_entry->runName,
                    statusMsg);
        }
        
        easy_struct.es_StructSize = sizeof(struct EasyStruct);
        easy_struct.es_Flags = 0;
        easy_struct.es_Title = "Restore Failed";
        easy_struct.es_TextFormat = message;
        easy_struct.es_GadgetFormat = "OK";
        EasyRequest(restore_data->window, &easy_struct, NULL);
    }
    
    return (status == RESTORE_OK);
}

/*------------------------------------------------------------------------*/
/**
 * @brief Recursively delete a directory and all its contents
 * 
 * @param path Path to directory to delete
 * @return BOOL TRUE if successful, FALSE otherwise
 */
/*------------------------------------------------------------------------*/
static BOOL delete_directory_recursive(const char *path)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    BOOL success = TRUE;
    char full_path[512];
    
    if (path == NULL)
        return FALSE;
    
    append_to_log("delete_directory_recursive: %s\n", path);
    
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        append_to_log("ERROR: Cannot lock directory: %s\n", path);
        return FALSE;
    }
    
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        UnLock(lock);
        append_to_log("ERROR: Cannot allocate FIB\n");
        return FALSE;
    }
    
    /* Examine the directory */
    if (!Examine(lock, fib))
    {
        FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        append_to_log("ERROR: Cannot examine directory: %s\n", path);
        return FALSE;
    }
    
    /* Iterate through directory contents */
    while (ExNext(lock, fib))
    {
        /* Build full path to entry */
        snprintf(full_path, sizeof(full_path), "%s/%s", path, fib->fib_FileName);
        
        if (fib->fib_DirEntryType > 0)
        {
            /* It's a directory - recurse */
            append_to_log("  Entering subdirectory: %s\n", fib->fib_FileName);
            if (!delete_directory_recursive(full_path))
            {
                success = FALSE;
                append_to_log("ERROR: Failed to delete subdirectory: %s\n", full_path);
            }
        }
        else
        {
            /* It's a file - delete it */
            append_to_log("  Deleting file: %s\n", fib->fib_FileName);
            if (!DeleteFile((STRPTR)full_path))
            {
                success = FALSE;
                append_to_log("ERROR: Failed to delete file: %s\n", full_path);
            }
        }
    }
    
    /* Check if ExNext ended normally (ERROR_NO_MORE_ENTRIES) or with an error */
    LONG error = IoErr();
    if (error != ERROR_NO_MORE_ENTRIES)
    {
        append_to_log("ERROR: ExNext failed with error code: %ld\n", error);
        success = FALSE;
    }
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    /* Now delete the empty directory itself */
    if (success)
    {
        append_to_log("  Deleting directory: %s\n", path);
        if (!DeleteFile((STRPTR)path))
        {
            append_to_log("ERROR: Failed to delete directory: %s\n", path);
            success = FALSE;
        }
    }
    
    return success;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Open the iTidy Restore Window
 */
/*------------------------------------------------------------------------*/
BOOL open_restore_window(struct iTidyRestoreWindow *restore_data)
{
    struct Screen *screen;
    struct DrawInfo *draw_info;
    struct TextFont *font;
    struct TextFont *system_font;
    struct TextAttr system_font_attr;
    struct RastPort temp_rp;
    struct NewGadget ng;
    struct Gadget *gad;
    UWORD font_width, font_height;
    UWORD button_height, string_height;
    UWORD listview_lines, listview_requested_height, actual_listview_height;
    UWORD detail_line_height, details_height;
    UWORD current_x, current_y;
    UWORD window_max_width, window_max_height;
    UWORD listview_bottom_y;
    UWORD final_window_width, final_window_height;
    STRPTR label;
    UWORD label_width, label_spacing;
    
    append_to_log("=== open_restore_window: Starting ===\n");
    
    if (restore_data == NULL)
    {
        append_to_log("ERROR: restore_data is NULL\n");
        return FALSE;
    }
    
    append_to_log("Initializing restore_data structure\n");
    /* Initialize structure */
    memset(restore_data, 0, sizeof(struct iTidyRestoreWindow));
    restore_data->selected_run_index = -1;
    restore_data->restore_window_geometry = TRUE;  /* Default to enabled */
    strcpy(restore_data->backup_root_path, "PROGDIR:Backups");
    
    /* Get Workbench screen */
    append_to_log("Locking Workbench screen\n");
    screen = LockPubScreen(NULL);
    if (screen == NULL)
    {
        append_to_log("ERROR: Failed to lock Workbench screen\n");
        return FALSE;
    }
    
    append_to_log("Screen locked successfully\n");
    restore_data->screen = screen;
    
    /* Get DrawInfo for proper font information */
    append_to_log("Getting DrawInfo and font dimensions\n");
    draw_info = GetScreenDrawInfo(screen);
    if (draw_info == NULL)
    {
        append_to_log("ERROR: Failed to get DrawInfo\n");
        UnlockPubScreen(NULL, screen);
        return FALSE;
    }
    
    font = draw_info->dri_Font;
    font_width = font->tf_XSize;
    font_height = font->tf_YSize;
    append_to_log("Screen font: width=%d, height=%d, flags=0x%02x\n", 
                  font_width, font_height, font->tf_Flags);
    
    /* Check if font is proportional */
    if (font->tf_Flags & FPF_PROPORTIONAL)
    {
        append_to_log("WARNING: Screen uses proportional font - will use system default font instead\n");
    }
    
    /* Open System Default Text font for listviews
     * This ensures we use a fixed-width font (typically Topaz)
     * even if the screen uses a proportional font
     */
    system_font_attr.ta_Name = (STRPTR)GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name;
    system_font_attr.ta_YSize = GfxBase->DefaultFont->tf_YSize;
    system_font_attr.ta_Style = FS_NORMAL;
    system_font_attr.ta_Flags = 0;
    
    system_font = OpenFont(&system_font_attr);
    if (system_font != NULL)
    {
        append_to_log("Opened system default font: %s, size %d\n", 
                      system_font_attr.ta_Name, system_font_attr.ta_YSize);
        
        /* Store in restore_data for later cleanup */
        restore_data->system_font = system_font;
        
        /* Use system font metrics for listview calculations */
        font = system_font;
        font_width = font->tf_XSize;
        font_height = font->tf_YSize;
        append_to_log("System font: width=%d, height=%d, flags=0x%02x\n", 
                      font_width, font_height, font->tf_Flags);
    }
    else
    {
        append_to_log("WARNING: Could not open system default font, using screen font\n");
        font = draw_info->dri_Font;
        system_font = NULL;
        restore_data->system_font = NULL;
    }
    
    /* Initialize RastPort for TextLength() measurements */
    InitRastPort(&temp_rp);
    SetFont(&temp_rp, font);
    
    /* Calculate gadget dimensions */
    button_height = font_height + 6;
    string_height = font_height + 6;
    listview_lines = 10;
    listview_requested_height = (font_height + 2) * listview_lines;
    detail_line_height = font_height + 4;
    details_height = detail_line_height * 6;
    
    /* Initialize position tracking */
    /* Start from window borders (using IControl preferences), then add margins */
    /* STANDARD LAYOUT: No bevel border or content padding */
    current_x = prefsIControl.currentLeftBarWidth + RESTORE_MARGIN_LEFT;
    current_y = prefsIControl.currentWindowBarHeight + RESTORE_MARGIN_TOP;
    window_max_width = 0;
    window_max_height = 0;
    
    /* Create GadTools visual info */
    append_to_log("Creating GadTools visual info\n");
    restore_data->visual_info = GetVisualInfo(screen, TAG_END);
    if (restore_data->visual_info == NULL)
    {
        append_to_log("ERROR: Failed to create visual info\n");
        UnlockPubScreen(NULL, screen);
        return FALSE;
    }
    
    append_to_log("Visual info created successfully\n");
    
    /* Create context gadget */
    append_to_log("Creating gadget context\n");
    gad = CreateContext(&restore_data->glist);
    if (gad == NULL)
    {
        append_to_log("ERROR: Failed to create gadget context\n");
        FreeVisualInfo(restore_data->visual_info);
        UnlockPubScreen(NULL, screen);
        return FALSE;
    }
    
    append_to_log("Gadget context created successfully\n");
    
    /* Initialize NewGadget structure with required fields */
    /* Use system default font for gadgets (fixed-width) instead of screen font */
    ng.ng_TextAttr = (system_font != NULL) ? &system_font_attr : screen->Font;
    ng.ng_VisualInfo = restore_data->visual_info;
    ng.ng_Flags = 0;
    
    append_to_log("NewGadget structure initialized (using %s)\n", 
                  (system_font != NULL) ? "system default font" : "screen font");
    
    /*--------------------------------------------------------------------*/
    /* PRE-CALCULATE LAYOUT DIMENSIONS                                   */
    /*--------------------------------------------------------------------*/
    UWORD listview_width = font_width * 65;
    
    /* Pre-calculate what the maximum right edge will be */
    UWORD precalc_max_right = current_x + listview_width;
    
    append_to_log("=== PRE-CALCULATED LAYOUT ===\n");
    append_to_log("Listview width: %d, max_right will be: %d\n", listview_width, precalc_max_right);
    
    /*--------------------------------------------------------------------*/
    /* Backup Run ListView                                               */
    /*--------------------------------------------------------------------*/
    append_to_log("Creating Backup Run ListView\n");
    
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = listview_width;
    ng.ng_Height = listview_requested_height;
    ng.ng_GadgetText = "";
    ng.ng_GadgetID = GID_RESTORE_RUN_LIST;
    ng.ng_Flags = PLACETEXT_ABOVE;
    
    append_to_log("ListView parameters: x=%d, y=%d, w=%d, h=%d\n", 
                  ng.ng_LeftEdge, ng.ng_TopEdge, ng.ng_Width, ng.ng_Height);
    
    restore_data->run_list = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, NULL,
        GTLV_Selected, ~0,
        GTLV_ShowSelected, NULL,
        TAG_END);
    
    if (gad == NULL)
    {
        append_to_log("ERROR: Failed to create Backup Run ListView\n");
        goto cleanup_error;
    }
    
    append_to_log("Backup Run ListView created successfully\n");
    
    /* CRITICAL: Get actual height */
    actual_listview_height = gad->Height;
    listview_bottom_y = ng.ng_TopEdge + actual_listview_height;
    
    update_window_max_dimensions(&window_max_width, &window_max_height,
                                ng.ng_LeftEdge + ng.ng_Width,
                                ng.ng_TopEdge + actual_listview_height);
    
    /*--------------------------------------------------------------------*/
    /* Run Details ListView - Read-only display                          */
    /*--------------------------------------------------------------------*/
    current_y = listview_bottom_y + RESTORE_SPACE_Y;
    
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = listview_width;
    ng.ng_Height = (font_height + 2) * 7;  /* 7 lines for details (added Source Directory) */
    ng.ng_GadgetText = "";
    ng.ng_GadgetID = GID_RESTORE_DETAILS;
    ng.ng_Flags = PLACETEXT_ABOVE;
    
    restore_data->details_listview = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
        GTLV_Labels, NULL,
        GTLV_ReadOnly, TRUE,
        GTLV_ShowSelected, NULL,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    update_window_max_dimensions(&window_max_width, &window_max_height,
                                ng.ng_LeftEdge + ng.ng_Width,
                                ng.ng_TopEdge + ng.ng_Height);
    
    /*--------------------------------------------------------------------*/
    /* Calculate equal button width for all 4 buttons                    */
    /*--------------------------------------------------------------------*/
    /* Find maximum button text width */
    UWORD max_btn_text_width = TextLength(&temp_rp, "Restore Run", 11);
    UWORD temp_width = TextLength(&temp_rp, "View Folders...", 15);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    temp_width = TextLength(&temp_rp, "Delete Run", 10);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    temp_width = TextLength(&temp_rp, "Cancel", 6);
    if (temp_width > max_btn_text_width)
        max_btn_text_width = temp_width;
    
    /* Calculate equal button width: 3 * button_width + 2 * spacing = available_width */
    UWORD available_width = listview_width;
    UWORD equal_button_width = (available_width - (2 * RESTORE_SPACE_X)) / 3;
    
    /* Ensure buttons are wide enough for text + padding */
    if (equal_button_width < max_btn_text_width + 8)
        equal_button_width = max_btn_text_width + 8;
    
    append_to_log("=== BUTTON CALCULATION ===\n");
    append_to_log("Available width: %d, Equal button width: %d\n", 
                  available_width, equal_button_width);
    
    /*--------------------------------------------------------------------*/
    /* Top Row: Restore Run Button + Checkbox                            */
    /*--------------------------------------------------------------------*/
    /* Add spacing between details listview and top button row */
    current_y = ng.ng_TopEdge + ng.ng_Height + RESTORE_SPACE_Y;
    
    /* Restore Run Button - left-aligned, no centering */
    ng.ng_LeftEdge = current_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Restore Run";
    ng.ng_GadgetID = GID_RESTORE_RUN_BTN;
    ng.ng_Flags = PLACETEXT_IN;
    
    restore_data->restore_run_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        GA_Disabled, TRUE,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    /* Restore Window Geometry Checkbox - to the right of the button */
    ng.ng_LeftEdge = current_x + equal_button_width + RESTORE_SPACE_X;
    ng.ng_TopEdge = current_y + ((button_height - (temp_rp.TxHeight + 4)) / 2); /* Vertically center with button */
    ng.ng_Width = TextLength(&temp_rp, "Restore window positions", 24) + 30;
    ng.ng_Height = temp_rp.TxHeight + 4;
    ng.ng_GadgetText = "Restore window positions";
    ng.ng_GadgetID = GID_RESTORE_WINDOW_GEOM_CHK;
    ng.ng_Flags = PLACETEXT_RIGHT;
    
    restore_data->window_geom_chk = gad = CreateGadget(CHECKBOX_KIND, gad, &ng,
        GTCB_Checked, TRUE,  /* Default to enabled */
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    /*--------------------------------------------------------------------*/
    /* Bottom Row: Delete Run, View Folders, Cancel (3 equal buttons)    */
    /*--------------------------------------------------------------------*/
    /* Add spacing between top row and bottom button row */
    current_y += button_height + RESTORE_SPACE_Y;
    
    UWORD button_row_x = current_x;
    
    append_to_log("=== BOTTOM BUTTON ROW ===\n");
    append_to_log("Button row starts at x=%d, y=%d\n", button_row_x, current_y);
    
    /* Delete Run Button */
    ng.ng_LeftEdge = button_row_x;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Delete Run";
    ng.ng_GadgetID = GID_RESTORE_DELETE_RUN;
    ng.ng_Flags = PLACETEXT_IN;
    
    restore_data->delete_run_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        GA_Disabled, TRUE,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    /* View Folders Button */
    ng.ng_LeftEdge = button_row_x + equal_button_width + RESTORE_SPACE_X;
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "View Folders...";
    ng.ng_GadgetID = GID_RESTORE_VIEW_FOLDERS;
    ng.ng_Flags = PLACETEXT_IN;
    
    restore_data->view_folders_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        GA_Disabled, TRUE,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    /* Cancel Button */
    ng.ng_LeftEdge = button_row_x + (2 * equal_button_width) + (2 * RESTORE_SPACE_X);
    ng.ng_TopEdge = current_y;
    ng.ng_Width = equal_button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Cancel";
    ng.ng_GadgetID = GID_RESTORE_CANCEL;
    ng.ng_Flags = PLACETEXT_IN;
    
    restore_data->cancel_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
        TAG_END);
    
    if (gad == NULL)
        goto cleanup_error;
    
    update_window_max_dimensions(&window_max_width, &window_max_height,
                                ng.ng_LeftEdge + ng.ng_Width,
                                ng.ng_TopEdge + ng.ng_Height);
    
    /*--------------------------------------------------------------------*/
    /* Calculate final window size and open window                       */
    /*--------------------------------------------------------------------*/
    /* Add right border width and margin to window width */
    final_window_width = window_max_width + prefsIControl.currentLeftBarWidth + RESTORE_MARGIN_RIGHT;
    final_window_height = current_y + button_height + RESTORE_MARGIN_BOTTOM;
    
    append_to_log("=== PREPARING TO OPEN WINDOW ===\n");
    append_to_log("Window size calculation:\n");
    append_to_log("  window_max_width=%d\n", window_max_width);
    append_to_log("  currentLeftBarWidth=%d\n", prefsIControl.currentLeftBarWidth);
    append_to_log("  RESTORE_MARGIN_RIGHT=%d\n", RESTORE_MARGIN_RIGHT);
    append_to_log("  final_window_width=%d\n", final_window_width);
    append_to_log("  current_y=%d, button_height=%d, RESTORE_MARGIN_BOTTOM=%d\n", 
                  current_y, button_height, RESTORE_MARGIN_BOTTOM);
    append_to_log("  final_window_height=%d\n", final_window_height);
    append_to_log("Screen dimensions: %d x %d\n", screen->Width, screen->Height);
    append_to_log("Window position: Left=%d, Top=%d\n",
                  (screen->Width - final_window_width) / 2,
                  (screen->Height - final_window_height) / 2);
    append_to_log("Gadget list pointer: %p\n", restore_data->glist);
    append_to_log("Screen pointer: %p\n", screen);
    
    /* Verify all gadgets were created */
    if (restore_data->run_list == NULL)
    {
        append_to_log("ERROR: run_list is NULL!\n");
        goto cleanup_error;
    }
    if (restore_data->details_listview == NULL)
    {
        append_to_log("ERROR: details_listview is NULL!\n");
        goto cleanup_error;
    }
    if (restore_data->restore_run_btn == NULL)
    {
        append_to_log("ERROR: restore_run_btn is NULL!\n");
        goto cleanup_error;
    }
    if (restore_data->delete_run_btn == NULL)
    {
        append_to_log("ERROR: delete_run_btn is NULL!\n");
        goto cleanup_error;
    }
    if (restore_data->view_folders_btn == NULL)
    {
        append_to_log("ERROR: view_folders_btn is NULL!\n");
        goto cleanup_error;
    }
    if (restore_data->cancel_btn == NULL)
    {
        append_to_log("ERROR: cancel_btn is NULL!\n");
        goto cleanup_error;
    }
    
    append_to_log("All gadgets verified OK. Calling OpenWindowTags...\n");
    
    restore_data->window = OpenWindowTags(NULL,
        WA_Left, (screen->Width - final_window_width) / 2,
        WA_Top, (screen->Height - final_window_height) / 2,
        WA_Width, final_window_width,
        WA_Height, final_window_height,
        WA_Title, RESTORE_WINDOW_TITLE,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, screen,
        WA_Gadgets, restore_data->glist,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_GADGETUP,
        TAG_END);
    
    append_to_log("OpenWindowTags returned: %p\n", restore_data->window);
    
    if (restore_data->window == NULL)
    {
        LONG ioerr = IoErr();
        append_to_log("ERROR: Failed to open restore window\n");
        append_to_log("ERROR: IoErr() = %ld\n", ioerr);
        goto cleanup_error;
    }
    
    append_to_log("Restore window opened successfully\n");
    restore_data->window_open = TRUE;
    
    /* DISABLED: Custom checkerboard background 
     * Draw the window background FIRST (before refreshing gadgets)
     * This ensures the pattern and bevel are behind the gadgets
     */
    /* draw_window_background(restore_data); */
    
    /* DISABLED: Custom gadget refresh
     * Force all gadgets to redraw themselves on top of the background
     * GT_RefreshWindow alone doesn't work with custom backgrounds
     */
    /* append_to_log("Refreshing window gadgets\n");
    RefreshGList(restore_data->glist, restore_data->window, NULL, -1); */
    
    /* Scan for backup runs */
    append_to_log("Scanning for backup runs in: %s\n", restore_data->backup_root_path);
    restore_data->run_count = scan_backup_runs(restore_data->backup_root_path,
                                              &restore_data->run_entries);
    
    append_to_log("Found %d backup runs\n", restore_data->run_count);
    
    if (restore_data->run_count > 0)
    {
        populate_run_list(restore_data,
                         restore_data->run_entries,
                         restore_data->run_count);
    }
    
    /* Free DrawInfo - no longer needed after window is created */
    if (draw_info != NULL)
    {
        FreeScreenDrawInfo(screen, draw_info);
    }
    
    append_to_log("Restore window opened successfully\n");
    return TRUE;

cleanup_error:
    append_to_log("ERROR: Entering cleanup_error - gadget creation failed\n");
    if (restore_data->glist != NULL)
    {
        append_to_log("Freeing gadgets\n");
        FreeGadgets(restore_data->glist);
    }
    if (restore_data->visual_info != NULL)
    {
        append_to_log("Freeing visual info\n");
        FreeVisualInfo(restore_data->visual_info);
    }
    if (restore_data->system_font != NULL)
    {
        append_to_log("Closing system font\n");
        CloseFont(restore_data->system_font);
        restore_data->system_font = NULL;
    }
    if (draw_info != NULL)
    {
        append_to_log("Freeing DrawInfo\n");
        FreeScreenDrawInfo(screen, draw_info);
    }
    if (screen != NULL)
    {
        append_to_log("Unlocking screen\n");
        UnlockPubScreen(NULL, screen);
    }
    append_to_log("=== open_restore_window: Exiting with failure ===\n");
    return FALSE;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Close the Restore Window and cleanup resources
 */
/*------------------------------------------------------------------------*/
void close_restore_window(struct iTidyRestoreWindow *restore_data)
{
    struct Node *node;
    
    if (restore_data == NULL)
        return;
    
    append_to_log("close_restore_window: Starting cleanup\n");
    
    /* CRITICAL: Detach list views from gadgets BEFORE freeing lists */
    /* This prevents the gadgets from accessing freed memory during window close */
    if (restore_data->window != NULL)
    {
        append_to_log("Detaching listviews from gadgets\n");
        
        if (restore_data->run_list != NULL)
        {
            GT_SetGadgetAttrs(restore_data->run_list, restore_data->window, NULL,
                GTLV_Labels, ~0,
                TAG_END);
        }
        
        if (restore_data->details_listview != NULL)
        {
            GT_SetGadgetAttrs(restore_data->details_listview, restore_data->window, NULL,
                GTLV_Labels, ~0,
                TAG_END);
        }
    }
    
    /* Free run list strings */
    if (restore_data->run_list_strings != NULL)
    {
        append_to_log("Freeing run_list_strings\n");
        while ((node = RemHead(restore_data->run_list_strings)) != NULL)
        {
            FreeVec(node);
        }
        FreeVec(restore_data->run_list_strings);
        restore_data->run_list_strings = NULL;
    }
    
    /* Free details list strings */
    if (restore_data->details_list_strings != NULL)
    {
        append_to_log("Freeing details_list_strings\n");
        while ((node = RemHead(restore_data->details_list_strings)) != NULL)
        {
            if (node->ln_Name != NULL)
            {
                FreeVec(node->ln_Name);
                node->ln_Name = NULL;
            }
            FreeVec(node);
        }
        FreeVec(restore_data->details_list_strings);
        restore_data->details_list_strings = NULL;
    }
    
    /* Free run entries */
    if (restore_data->run_entries != NULL)
    {
        append_to_log("Freeing run_entries\n");
        FreeVec(restore_data->run_entries);
        restore_data->run_entries = NULL;
    }
    
    /* Close window */
    if (restore_data->window != NULL)
    {
        append_to_log("Closing window\n");
        CloseWindow(restore_data->window);
        restore_data->window = NULL;
    }
    
    /* Free gadgets */
    if (restore_data->glist != NULL)
    {
        append_to_log("Freeing gadgets\n");
        FreeGadgets(restore_data->glist);
        restore_data->glist = NULL;
    }
    
    /* Free visual info */
    if (restore_data->visual_info != NULL)
    {
        append_to_log("Freeing visual info\n");
        FreeVisualInfo(restore_data->visual_info);
        restore_data->visual_info = NULL;
    }
    
    /* Close system font if we opened it */
    if (restore_data->system_font != NULL)
    {
        append_to_log("Closing system font\n");
        CloseFont(restore_data->system_font);
        restore_data->system_font = NULL;
    }
    
    /* Unlock screen */
    if (restore_data->screen != NULL)
    {
        append_to_log("Unlocking screen\n");
        UnlockPubScreen(NULL, restore_data->screen);
        restore_data->screen = NULL;
    }
    
    restore_data->window_open = FALSE;
    
    append_to_log("Restore window closed successfully\n");
}

/*------------------------------------------------------------------------*/
/**
 * @brief Handle restore window events (main event loop)
 */
/*------------------------------------------------------------------------*/
BOOL handle_restore_window_events(struct iTidyRestoreWindow *restore_data)
{
    struct IntuiMessage *imsg;
    ULONG msgClass;
    UWORD msgCode;
    struct Gadget *gadget;
    BOOL continue_running = TRUE;
    
    if (restore_data == NULL || restore_data->window == NULL)
        return FALSE;
    
    /* Wait for events */
    WaitPort(restore_data->window->UserPort);
    
    while ((imsg = GT_GetIMsg(restore_data->window->UserPort)) != NULL)
    {
        msgClass = imsg->Class;
        msgCode = imsg->Code;
        gadget = (struct Gadget *)imsg->IAddress;
        
        GT_ReplyIMsg(imsg);
        
        switch (msgClass)
        {
            case IDCMP_CLOSEWINDOW:
                append_to_log("Close gadget clicked\n");
                continue_running = FALSE;
                break;
            
            case IDCMP_REFRESHWINDOW:
                /* DISABLED: Custom background redrawing
                 * GT_BeginRefresh(restore_data->window);
                 * draw_window_background(restore_data);
                 * GT_EndRefresh(restore_data->window, TRUE);
                 */
                /* Use standard GadTools refresh instead */
                GT_BeginRefresh(restore_data->window);
                GT_EndRefresh(restore_data->window, TRUE);
                break;
            
            case IDCMP_GADGETUP:
                switch (gadget->GadgetID)
                {
                    case GID_RESTORE_RUN_LIST:
                        {
                            LONG selected = -1;
                            GT_GetGadgetAttrs(restore_data->run_list,
                                            restore_data->window, NULL,
                                            GTLV_Selected, &selected,
                                            TAG_END);
                            
                            append_to_log("ListView clicked: selected=%d, run_count=%d\n", 
                                         selected, restore_data->run_count);
                            
                            if (selected >= 0 && selected < (LONG)restore_data->run_count)
                            {
                                /* List is in reverse order */
                                ULONG actual_idx = restore_data->run_count - 1 - selected;
                                
                                /* Check for double-click by comparing with previous click */
                                BOOL is_double_click = FALSE;
                                if (restore_data->selected_run_index == (LONG)actual_idx &&
                                    DoubleClick(restore_data->last_click_secs, restore_data->last_click_micros,
                                               imsg->Seconds, imsg->Micros))
                                {
                                    is_double_click = TRUE;
                                    append_to_log("Double-click detected on ListView item %d!\n", selected);
                                }
                                
                                restore_data->selected_run_index = actual_idx;
                                
                                /* Update click tracking */
                                restore_data->last_click_secs = imsg->Seconds;
                                restore_data->last_click_micros = imsg->Micros;
                                
                                update_details_panel(restore_data,
                                    &restore_data->run_entries[actual_idx]);
                                
                                /* Enable restore and delete buttons */
                                GT_SetGadgetAttrs(restore_data->restore_run_btn,
                                                restore_data->window, NULL,
                                                GA_Disabled, FALSE,
                                                TAG_END);
                                
                                GT_SetGadgetAttrs(restore_data->delete_run_btn,
                                                restore_data->window, NULL,
                                                GA_Disabled, FALSE,
                                                TAG_END);
                                
                                /* Enable view folders if catalog exists */
                                GT_SetGadgetAttrs(restore_data->view_folders_btn,
                                                restore_data->window, NULL,
                                                GA_Disabled,
                                                !restore_data->run_entries[actual_idx].hasCatalog,
                                                TAG_END);
                                
                                /* Handle double-click */
                                if (is_double_click && restore_data->run_entries[actual_idx].hasCatalog)
                                {
                                    struct RestoreRunEntry *selected_entry = 
                                        &restore_data->run_entries[actual_idx];
                                    char catalog_path[512];
                                    struct iTidyFolderViewWindow folder_view_data;
                                    
                                    /* Build path to catalog.txt */
                                    sprintf(catalog_path, "%s/%s/catalog.txt",
                                           restore_data->backup_root_path,
                                           selected_entry->runName);
                                    
                                    append_to_log("Double-click opening folder view for: %s\n", catalog_path);
                                    
                                    /* Initialize folder view data */
                                    memset(&folder_view_data, 0, sizeof(folder_view_data));
                                    folder_view_data.screen = restore_data->screen;
                                    
                                    /* Open folder view window */
                                    if (open_folder_view_window(&folder_view_data,
                                                               catalog_path,
                                                               selected_entry->runNumber,
                                                               selected_entry->dateStr,
                                                               selected_entry->folderCount))
                                    {
                                        /* Run folder view event loop */
                                        while (handle_folder_view_window_events(&folder_view_data))
                                        {
                                            WaitPort(folder_view_data.window->UserPort);
                                        }
                                        
                                        /* Close folder view window */
                                        close_folder_view_window(&folder_view_data);
                                    }
                                    else
                                    {
                                        append_to_log("ERROR: Failed to open folder view window\n");
                                    }
                                }
                                else if (is_double_click)
                                {
                                    append_to_log("Double-click on run without catalog - ignoring\n");
                                }
                            }
                        }
                        break;
                    
                    case GID_RESTORE_WINDOW_GEOM_CHK:
                        /* Update flag from checkbox state */
                        restore_data->restore_window_geometry = 
                            (((struct Gadget*)imsg->IAddress)->Flags & GFLG_SELECTED) != 0;
                        append_to_log("Window geometry restore: %s\n", 
                                     restore_data->restore_window_geometry ? "ENABLED" : "DISABLED");
                        break;
                    
                    case GID_RESTORE_RUN_BTN:
                        if (restore_data->selected_run_index >= 0 &&
                            restore_data->selected_run_index < (LONG)restore_data->run_count)
                        {
                            perform_restore_run(restore_data,
                                &restore_data->run_entries[restore_data->selected_run_index]);
                        }
                        break;
                    
                    case GID_RESTORE_DELETE_RUN:
                        if (restore_data->selected_run_index >= 0 &&
                            restore_data->selected_run_index < (LONG)restore_data->run_count)
                        {
                            struct RestoreRunEntry *selected_entry = 
                                &restore_data->run_entries[restore_data->selected_run_index];
                            struct EasyStruct easy_struct;
                            char message[512];
                            
                            /* Show confirmation requester */
                            sprintf(message, "Delete backup run %s?\n\nThis will permanently delete:\n- %lu folder archive(s)\n- Catalog file\n- Run directory\n\nThis action cannot be undone!",
                                    selected_entry->runName,
                                    selected_entry->folderCount);
                            
                            easy_struct.es_StructSize = sizeof(struct EasyStruct);
                            easy_struct.es_Flags = 0;
                            easy_struct.es_Title = "Confirm Delete";
                            easy_struct.es_TextFormat = message;
                            easy_struct.es_GadgetFormat = "Delete|Cancel";
                            
                            if (EasyRequest(restore_data->window, &easy_struct, NULL))
                            {
                                char run_path[512];
                                
                                /* Build full path to run directory */
                                sprintf(run_path, "%s/%s", restore_data->backup_root_path, selected_entry->runName);
                                
                                append_to_log("Deleting backup run: %s\n", run_path);
                                
                                /* Delete the entire run directory recursively */
                                if (delete_directory_recursive(run_path))
                                {
                                    append_to_log("Successfully deleted run: %s\n", selected_entry->runName);
                                    
                                    /* Rescan backup directory */
                                    if (restore_data->run_entries != NULL)
                                    {
                                        FreeVec(restore_data->run_entries);
                                        restore_data->run_entries = NULL;
                                    }
                                    
                                    restore_data->run_count = scan_backup_runs(
                                        restore_data->backup_root_path,
                                        &restore_data->run_entries);
                                    
                                    if (restore_data->run_count > 0)
                                    {
                                        populate_run_list(restore_data,
                                                         restore_data->run_entries,
                                                         restore_data->run_count);
                                    }
                                    else
                                    {
                                        /* No runs left, clear details and disable buttons */
                                        update_details_panel(restore_data, NULL);
                                        GT_SetGadgetAttrs(restore_data->restore_run_btn,
                                                        restore_data->window, NULL,
                                                        GA_Disabled, TRUE,
                                                        TAG_END);
                                        GT_SetGadgetAttrs(restore_data->delete_run_btn,
                                                        restore_data->window, NULL,
                                                        GA_Disabled, TRUE,
                                                        TAG_END);
                                        GT_SetGadgetAttrs(restore_data->view_folders_btn,
                                                        restore_data->window, NULL,
                                                        GA_Disabled, TRUE,
                                                        TAG_END);
                                    }
                                    
                                    /* Show success message */
                                    sprintf(message, "Backup run %s deleted successfully.", selected_entry->runName);
                                    easy_struct.es_TextFormat = message;
                                    easy_struct.es_GadgetFormat = "OK";
                                    easy_struct.es_Title = "Delete Complete";
                                    EasyRequest(restore_data->window, &easy_struct, NULL);
                                }
                                else
                                {
                                    append_to_log("ERROR: Failed to delete run: %s\n", run_path);
                                    
                                    sprintf(message, "Failed to delete backup run %s.\n\nThe directory may be in use or protected.\nCheck the log for details.", selected_entry->runName);
                                    easy_struct.es_TextFormat = message;
                                    easy_struct.es_GadgetFormat = "OK";
                                    easy_struct.es_Title = "Delete Failed";
                                    EasyRequest(restore_data->window, &easy_struct, NULL);
                                }
                            }
                            else
                            {
                                append_to_log("Delete cancelled by user\n");
                            }
                        }
                        break;
                    
                    case GID_RESTORE_VIEW_FOLDERS:
                        /* Use simple test window (working version) */
                        if (restore_data->selected_run_index >= 0 &&
                            restore_data->selected_run_index < (LONG)restore_data->run_count)
                        {
                            struct RestoreRunEntry *selected_entry = 
                                &restore_data->run_entries[restore_data->selected_run_index];
                            char catalog_path[512];
                            struct iTidyFolderViewWindow folder_view_data;
                            
                            /* Build path to catalog.txt */
                            sprintf(catalog_path, "%s/%s/catalog.txt",
                                   restore_data->backup_root_path,
                                   selected_entry->runName);
                            
                            append_to_log("Opening folder view for: %s\n", catalog_path);
                            
                            /* Initialize folder view data */
                            memset(&folder_view_data, 0, sizeof(folder_view_data));
                            folder_view_data.screen = restore_data->screen;
                            
                            /* Open folder view window */
                            if (open_folder_view_window(&folder_view_data,
                                                       catalog_path,
                                                       selected_entry->runNumber,
                                                       selected_entry->dateStr,
                                                       selected_entry->folderCount))
                            {
                                /* Run folder view event loop */
                                while (handle_folder_view_window_events(&folder_view_data))
                                {
                                    WaitPort(folder_view_data.window->UserPort);
                                }
                                
                                /* Close folder view window */
                                close_folder_view_window(&folder_view_data);
                            }
                            else
                            {
                                append_to_log("ERROR: Failed to open folder view window\n");
                            }
                        }
                        break;
                    
                    case GID_RESTORE_CANCEL:
                        append_to_log("Cancel button clicked\n");
                        continue_running = FALSE;
                        break;
                }
                break;
        }
    }
    
    return continue_running;
}
