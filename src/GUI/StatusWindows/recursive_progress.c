/*
 * recursive_progress.c - Dual-bar recursive progress window implementation
 * Workbench 3.0+ compatible, with prescan and smart yielding
 */

#include "recursive_progress.h"
#include "progress_common.h"
#include "Settings/IControlPrefs.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <clib/utility_protos.h>
#include <string.h>
#include <stdio.h>

/* External IControl preferences */
extern struct IControlPrefsDetails prefsIControl;

/* Window dimensions - INNER content area only */
#define RECURSIVE_WINDOW_WIDTH  450
#define RECURSIVE_WINDOW_HEIGHT 165

/* Layout constants - margins WITHIN the window content area */
#define MARGIN_LEFT   8
#define MARGIN_TOP    8
#define MARGIN_RIGHT  8
#define MARGIN_BOTTOM 8
#define BAR_HEIGHT    18
#define TEXT_SPACING  4
#define BAR_SPACING   8

/* Prescan constants */
#define INITIAL_FOLDER_CAPACITY 64
#define YIELD_INTERVAL 100  /* Yield every N items */

/*
 * Redraw callback for refresh handler
 */
static void RedrawRecursiveWindow(APTR userData)
{
    iTidy_RecursiveProgressWindow *rpw = (iTidy_RecursiveProgressWindow *)userData;
    iTidy_ProgressPens pens;
    char text_buf[128];
    UWORD folder_percent, icon_percent;
    
    if (!rpw || !rpw->window)
        return;
    
    /* Get theme pens */
    if (!iTidy_Progress_GetPens(rpw->screen, &pens))
        return;
    
    /* Apply screen font */
    iTidy_Progress_ApplyScreenFont(rpw->screen, rpw->window->RPort);
    
    /* Draw task label */
    iTidy_Progress_DrawTextLabel(rpw->window->RPort, rpw->label_x, rpw->label_y,
                                  rpw->task_label, pens.text_pen);
    
    /* Draw overall percentage */
    folder_percent = (rpw->total_folders > 0) ? 
                     (UWORD)((rpw->current_folder * 100) / rpw->total_folders) : 0;
    if (folder_percent > 100) folder_percent = 100;
    sprintf(text_buf, "%u%%", folder_percent);
    iTidy_Progress_DrawPercentage(rpw->window->RPort, rpw->percent_x, rpw->percent_y,
                                   text_buf, pens.text_pen);
    
    /* Draw "Folders:" label */
    iTidy_Progress_DrawTextLabel(rpw->window->RPort, rpw->folder_label_x, rpw->folder_label_y,
                                  "Folders:", pens.text_pen);
    
    /* Draw folder count */
    sprintf(text_buf, "%lu/%lu", rpw->current_folder, rpw->total_folders);
    iTidy_Progress_DrawTextLabel(rpw->window->RPort, rpw->folder_count_x, rpw->folder_count_y,
                                  text_buf, pens.text_pen);
    
    /* Draw folder progress bar */
    iTidy_Progress_DrawBevelBox(rpw->window->RPort, rpw->folder_bar_x, rpw->folder_bar_y,
                                 rpw->folder_bar_w, rpw->folder_bar_h,
                                 pens.shine_pen, pens.shadow_pen, pens.fill_pen, TRUE);
    iTidy_Progress_DrawBarFill(rpw->window->RPort, rpw->folder_bar_x, rpw->folder_bar_y,
                                rpw->folder_bar_w, rpw->folder_bar_h,
                                pens.bar_pen, pens.fill_pen, folder_percent);
    
    /* Draw current folder path with truncation */
    if (rpw->last_folder_path[0] != '\0') {
        iTidy_Progress_DrawTruncatedText(rpw->window->RPort, rpw->path_x, rpw->path_y,
                                          rpw->last_folder_path, rpw->path_max_width,
                                          TRUE, pens.text_pen);  /* TRUE = path truncation */
    }
    
    /* Draw "Icons:" label */
    iTidy_Progress_DrawTextLabel(rpw->window->RPort, rpw->icon_label_x, rpw->icon_label_y,
                                  "Icons:", pens.text_pen);
    
    /* Draw icon count */
    if (rpw->icons_in_folder > 0) {
        sprintf(text_buf, "%u/%u", rpw->current_icon, rpw->icons_in_folder);
        iTidy_Progress_DrawTextLabel(rpw->window->RPort, rpw->icon_count_x, rpw->icon_count_y,
                                      text_buf, pens.text_pen);
    }
    
    /* Draw icon progress bar */
    iTidy_Progress_DrawBevelBox(rpw->window->RPort, rpw->icon_bar_x, rpw->icon_bar_y,
                                 rpw->icon_bar_w, rpw->icon_bar_h,
                                 pens.shine_pen, pens.shadow_pen, pens.fill_pen, TRUE);
    
    icon_percent = (rpw->icons_in_folder > 0) ?
                   (UWORD)((rpw->current_icon * 100) / rpw->icons_in_folder) : 0;
    if (icon_percent > 100) icon_percent = 100;
    iTidy_Progress_DrawBarFill(rpw->window->RPort, rpw->icon_bar_x, rpw->icon_bar_y,
                                rpw->icon_bar_w, rpw->icon_bar_h,
                                pens.bar_pen, pens.fill_pen, icon_percent);
}

/*
 * Helper: Add folder to scan result
 */
static BOOL AddFolderToScan(iTidy_RecursiveScanResult *scan, const char *path, UWORD icon_count)
{
    /* Expand arrays if needed */
    if (scan->totalFolders >= scan->allocated) {
        ULONG new_capacity = scan->allocated * 2;
        char **new_paths;
        UWORD *new_counts;
        
        new_paths = (char **)AllocMem(new_capacity * sizeof(char *), MEMF_CLEAR);
        if (!new_paths)
            return FALSE;
        
        new_counts = (UWORD *)AllocMem(new_capacity * sizeof(UWORD), MEMF_CLEAR);
        if (!new_counts) {
            FreeMem(new_paths, new_capacity * sizeof(char *));
            return FALSE;
        }
        
        /* Copy existing data */
        if (scan->folderPaths) {
            CopyMem(scan->folderPaths, new_paths, scan->totalFolders * sizeof(char *));
            FreeMem(scan->folderPaths, scan->allocated * sizeof(char *));
        }
        
        if (scan->iconCounts) {
            CopyMem(scan->iconCounts, new_counts, scan->totalFolders * sizeof(UWORD));
            FreeMem(scan->iconCounts, scan->allocated * sizeof(UWORD));
        }
        
        scan->folderPaths = new_paths;
        scan->iconCounts = new_counts;
        scan->allocated = new_capacity;
    }
    
    /* Allocate and copy path string */
    {
        ULONG path_len = strlen(path) + 1;
        char *path_copy = (char *)AllocMem(path_len, MEMF_CLEAR);
        if (!path_copy)
            return FALSE;
        
        strcpy(path_copy, path);
        scan->folderPaths[scan->totalFolders] = path_copy;
        scan->iconCounts[scan->totalFolders] = icon_count;
        scan->totalFolders++;
        scan->totalIcons += icon_count;
    }
    
    return TRUE;
}

/*
 * Helper: Count .info files in a directory (non-recursive)
 */
static UWORD CountIconsInDirectory(const char *path)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    UWORD icon_count = 0;
    LONG result;
    
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
        return 0;
    
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib) {
        UnLock(lock);
        return 0;
    }
    
    /* Examine directory */
    if (Examine(lock, fib)) {
        /* Count .info files */
        while ((result = ExNext(lock, fib)) != 0) {
            if (fib->fib_DirEntryType < 0) {  /* File, not directory */
                ULONG name_len = strlen(fib->fib_FileName);
                if (name_len > 5 && 
                    Stricmp(&fib->fib_FileName[name_len - 5], ".info") == 0) {
                    icon_count++;
                }
            }
        }
    }
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    return icon_count;
}

/*
 * Helper: Recursive prescan implementation
 */
static BOOL PrescanRecursiveImpl(const char *path, iTidy_RecursiveScanResult *scan, ULONG *item_count)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    UWORD icon_count;
    LONG result;
    
    /* Count icons in this directory */
    icon_count = CountIconsInDirectory(path);
    
    /* Add this folder to results */
    if (!AddFolderToScan(scan, path, icon_count))
        return FALSE;
    
    /* Yield to multitasking periodically */
    (*item_count)++;
    if ((*item_count) % YIELD_INTERVAL == 0) {
        Delay(1);  /* One tick - keeps system responsive */
    }
    
    /* Scan subdirectories */
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
        return TRUE;  /* Not a fatal error - just can't recurse */
    
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib) {
        UnLock(lock);
        return TRUE;
    }
    
    if (Examine(lock, fib)) {
        while ((result = ExNext(lock, fib)) != 0) {
            if (fib->fib_DirEntryType > 0) {  /* Directory */
                char subpath[512];
                ULONG path_len = strlen(path);
                
                /* Skip hidden folders (starting with .) */
                if (fib->fib_FileName[0] == '.')
                    continue;
                
                /* Build subpath */
                strcpy(subpath, path);
                if (path_len > 0 && subpath[path_len - 1] != ':' && subpath[path_len - 1] != '/') {
                    strcat(subpath, "/");
                }
                strcat(subpath, fib->fib_FileName);
                
                /* Recurse into subdirectory */
                if (!PrescanRecursiveImpl(subpath, scan, item_count)) {
                    FreeDosObject(DOS_FIB, fib);
                    UnLock(lock);
                    return FALSE;
                }
                
                /* Yield after processing each subdirectory */
                Delay(1);
            }
        }
    }
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    return TRUE;
}

struct iTidy_RecursiveScanResult* iTidy_PrescanRecursive(const char *rootPath)
{
    iTidy_RecursiveScanResult *scan;
    ULONG item_count = 0;
    
    if (!rootPath)
        return NULL;
    
    /* Allocate scan result structure */
    scan = (iTidy_RecursiveScanResult *)AllocMem(sizeof(iTidy_RecursiveScanResult), MEMF_CLEAR);
    if (!scan)
        return NULL;
    
    /* Initialize with initial capacity */
    scan->allocated = INITIAL_FOLDER_CAPACITY;
    scan->folderPaths = (char **)AllocMem(scan->allocated * sizeof(char *), MEMF_CLEAR);
    scan->iconCounts = (UWORD *)AllocMem(scan->allocated * sizeof(UWORD), MEMF_CLEAR);
    
    if (!scan->folderPaths || !scan->iconCounts) {
        iTidy_FreeScanResult(scan);
        return NULL;
    }
    
    /* Perform recursive scan */
    if (!PrescanRecursiveImpl(rootPath, scan, &item_count)) {
        iTidy_FreeScanResult(scan);
        return NULL;
    }
    
    return scan;
}

struct iTidy_RecursiveProgressWindow* iTidy_OpenRecursiveProgress(
    struct Screen *screen,
    const char *task_label,
    const struct iTidy_RecursiveScanResult *scan)
{
    iTidy_RecursiveProgressWindow *rpw;
    struct DrawInfo *dri;
    struct TextFont *font;
    struct RastPort temp_rp;
    UWORD window_width, window_height;
    WORD window_left, window_top;
    WORD y_pos;
    
    /* Validate parameters */
    if (!screen || !task_label || !scan)
        return NULL;
    
    /* Allocate structure */
    rpw = (iTidy_RecursiveProgressWindow *)AllocMem(sizeof(iTidy_RecursiveProgressWindow), 
                                                     MEMF_CLEAR);
    if (!rpw)
        return NULL;
    
    /* Store parameters */
    rpw->screen = screen;
    strncpy(rpw->task_label, task_label, sizeof(rpw->task_label) - 1);
    rpw->task_label[sizeof(rpw->task_label) - 1] = '\0';
    rpw->total_folders = scan->totalFolders;
    rpw->total_icons = scan->totalIcons;
    rpw->current_folder = 0;
    rpw->current_icon = 0;
    rpw->icons_in_folder = 0;
    
    /* Initialize cached state */
    rpw->last_folder_fill_width = 0;
    rpw->last_icon_fill_width = 0;
    rpw->last_folder_percent = 0;
    rpw->last_icon_percent = 0;
    rpw->last_folder_path[0] = '\0';
    
    /* Get screen's DrawInfo for font measurements */
    dri = GetScreenDrawInfo(screen);
    if (!dri) {
        FreeMem(rpw, sizeof(iTidy_RecursiveProgressWindow));
        return NULL;
    }
    
    font = dri->dri_Font;
    rpw->font_width = font->tf_XSize;
    rpw->font_height = font->tf_YSize;
    
    /* Initialize temp RastPort for text measurements */
    InitRastPort(&temp_rp);
    SetFont(&temp_rp, font);
    
    /* Pre-calculate layout positions using IControl preferences for borders */
    /* This follows the same pattern as restore_window.c */
    UWORD content_width = RECURSIVE_WINDOW_WIDTH;
    UWORD content_height = RECURSIVE_WINDOW_HEIGHT;
    
    /* Task label - top left (border + margin) */
    rpw->label_x = prefsIControl.currentLeftBarWidth + MARGIN_LEFT;
    rpw->label_y = prefsIControl.currentWindowBarHeight + MARGIN_TOP;
    
    /* Percentage - top right */
    {
        char temp_percent[] = "100%";
        UWORD percent_width = (UWORD)TextLength(&temp_rp, temp_percent, 4);
        rpw->percent_x = prefsIControl.currentLeftBarWidth + content_width - MARGIN_RIGHT;
        rpw->percent_y = prefsIControl.currentWindowBarHeight + MARGIN_TOP;
    }
    
    y_pos = prefsIControl.currentWindowBarHeight + MARGIN_TOP + rpw->font_height + TEXT_SPACING * 2;
    
    /* Folder progress section */
    rpw->folder_label_x = prefsIControl.currentLeftBarWidth + MARGIN_LEFT;
    rpw->folder_label_y = y_pos;
    
    /* Reserve space for "Folders:" label */
    {
        UWORD label_width = (UWORD)TextLength(&temp_rp, "Folders:", 8);
        rpw->folder_bar_x = prefsIControl.currentLeftBarWidth + MARGIN_LEFT + label_width + TEXT_SPACING * 2;
    }
    
    rpw->folder_bar_y = y_pos - 4;  /* Align with text baseline */
    rpw->folder_bar_w = content_width - (rpw->folder_bar_x - prefsIControl.currentLeftBarWidth) - MARGIN_RIGHT - 80;  /* Reserve for count */
    rpw->folder_bar_h = BAR_HEIGHT;
    
    /* Folder count - right of bar */
    rpw->folder_count_x = rpw->folder_bar_x + rpw->folder_bar_w + TEXT_SPACING * 2;
    rpw->folder_count_y = y_pos;
    
    y_pos += BAR_HEIGHT + TEXT_SPACING;
    
    /* Current folder path */
    rpw->path_x = prefsIControl.currentLeftBarWidth + MARGIN_LEFT;
    rpw->path_y = y_pos;
    rpw->path_max_width = content_width - MARGIN_LEFT - MARGIN_RIGHT;
    
    y_pos += rpw->font_height + BAR_SPACING;
    
    /* Icon progress section */
    rpw->icon_label_x = prefsIControl.currentLeftBarWidth + MARGIN_LEFT;
    rpw->icon_label_y = y_pos;
    
    /* Reserve space for "Icons:" label */
    {
        UWORD label_width = (UWORD)TextLength(&temp_rp, "Icons:", 6);
        rpw->icon_bar_x = prefsIControl.currentLeftBarWidth + MARGIN_LEFT + label_width + TEXT_SPACING * 2;
    }
    
    rpw->icon_bar_y = y_pos - 4;  /* Align with text baseline */
    rpw->icon_bar_w = content_width - (rpw->icon_bar_x - prefsIControl.currentLeftBarWidth) - MARGIN_RIGHT - 80;  /* Reserve for count */
    rpw->icon_bar_h = BAR_HEIGHT;
    
    /* Icon count - right of bar */
    rpw->icon_count_x = rpw->icon_bar_x + rpw->icon_bar_w + TEXT_SPACING * 2;
    rpw->icon_count_y = y_pos;
    
    FreeScreenDrawInfo(screen, dri);
    
    /* Calculate total window size including borders */
    window_width = prefsIControl.currentLeftBarWidth + content_width + prefsIControl.currentLeftBarWidth;
    window_height = prefsIControl.currentWindowBarHeight + content_height;
    
    /* Calculate centered window position */
    window_left = (screen->Width - window_width) / 2;
    window_top = (screen->Height - window_height) / 2;
    
    /* Open window IMMEDIATELY */
    /* Use WA_Width/Height to specify total window size including borders */
    rpw->window = OpenWindowTags(NULL,
        WA_Left, window_left,
        WA_Top, window_top,
        WA_Width, window_width,
        WA_Height, window_height,
        WA_Title, task_label,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, screen,
        WA_IDCMP, IDCMP_REFRESHWINDOW | IDCMP_INTUITICKS,
        WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_SMART_REFRESH | WFLG_ACTIVATE | WFLG_RMBTRAP,
        TAG_END);
    
    if (!rpw->window) {
        FreeMem(rpw, sizeof(iTidy_RecursiveProgressWindow));
        return NULL;
    }
    
    /* Set busy pointer IMMEDIATELY */
    SetWindowPointer(rpw->window,
                     WA_BusyPointer, TRUE,
                     TAG_END);
    
    /* Draw initial empty state */
    RedrawRecursiveWindow(rpw);
    
    return rpw;
}

void iTidy_UpdateFolderProgress(
    struct iTidy_RecursiveProgressWindow *rpw,
    ULONG folder_index,
    const char *folder_path,
    UWORD icons_in_folder)
{
    iTidy_ProgressPens pens;
    UWORD new_percent;
    ULONG new_fill_width;
    char text_buf[128];
    BOOL folder_changed = FALSE;
    BOOL percent_changed = FALSE;
    BOOL path_changed = FALSE;
    
    if (!rpw || !rpw->window)
        return;
    
    /* Update state */
    rpw->current_folder = folder_index;
    rpw->icons_in_folder = icons_in_folder;
    rpw->current_icon = 0;  /* Reset icon progress */
    
    /* Handle pending refresh events */
    iTidy_Progress_HandleRefresh(rpw->window, RedrawRecursiveWindow, rpw);
    
    /* Get theme pens */
    if (!iTidy_Progress_GetPens(rpw->screen, &pens))
        return;
    
    /* Apply screen font */
    iTidy_Progress_ApplyScreenFont(rpw->screen, rpw->window->RPort);
    
    /* Calculate new percentage */
    new_percent = (rpw->total_folders > 0) ? 
                  (UWORD)((folder_index * 100) / rpw->total_folders) : 0;
    if (new_percent > 100)
        new_percent = 100;
    
    /* Calculate new fill width */
    {
        WORD interior_w = rpw->folder_bar_w - 4;
        new_fill_width = (interior_w * new_percent) / 100;
    }
    
    /* Check what changed */
    folder_changed = (new_fill_width != rpw->last_folder_fill_width);
    percent_changed = (new_percent != rpw->last_folder_percent);
    
    if (folder_path) {
        path_changed = (strcmp(folder_path, rpw->last_folder_path) != 0);
    }
    
    /* Update folder progress bar if changed */
    if (folder_changed) {
        iTidy_Progress_DrawBarFill(rpw->window->RPort, rpw->folder_bar_x, rpw->folder_bar_y,
                                    rpw->folder_bar_w, rpw->folder_bar_h,
                                    pens.bar_pen, pens.fill_pen, new_percent);
        rpw->last_folder_fill_width = new_fill_width;
    }
    
    /* Update overall percentage if changed */
    if (percent_changed) {
        char old_text[16], new_text[16];
        UWORD old_width, new_width;
        
        sprintf(old_text, "%u%%", rpw->last_folder_percent);
        old_width = (UWORD)TextLength(rpw->window->RPort, old_text, (LONG)strlen(old_text));
        
        /* Clear old percentage */
        iTidy_Progress_ClearTextArea(rpw->window->RPort,
                                      rpw->percent_x - old_width,
                                      rpw->percent_y,
                                      old_width, rpw->font_height,
                                      pens.fill_pen);
        
        /* Draw new percentage */
        sprintf(new_text, "%u%%", new_percent);
        iTidy_Progress_DrawPercentage(rpw->window->RPort, rpw->percent_x, rpw->percent_y,
                                       new_text, pens.text_pen);
        
        rpw->last_folder_percent = new_percent;
    }
    
    /* Update folder count */
    {
        /* Clear old count */
        iTidy_Progress_ClearTextArea(rpw->window->RPort,
                                      rpw->folder_count_x, rpw->folder_count_y - rpw->font_height + 4,
                                      80, rpw->font_height,
                                      pens.fill_pen);
        
        /* Draw new count */
        sprintf(text_buf, "%lu/%lu", folder_index, rpw->total_folders);
        iTidy_Progress_DrawTextLabel(rpw->window->RPort, rpw->folder_count_x, rpw->folder_count_y,
                                      text_buf, pens.text_pen);
    }
    
    /* Update folder path if changed */
    if (path_changed && folder_path) {
        /* Clear old path */
        iTidy_Progress_ClearTextArea(rpw->window->RPort,
                                      rpw->path_x, rpw->path_y,
                                      rpw->path_max_width, rpw->font_height,
                                      pens.fill_pen);
        
        /* Draw new path with smart truncation */
        iTidy_Progress_DrawTruncatedText(rpw->window->RPort, rpw->path_x, rpw->path_y,
                                          folder_path, rpw->path_max_width,
                                          TRUE, pens.text_pen);  /* TRUE = path truncation */
        
        /* Cache path */
        strncpy(rpw->last_folder_path, folder_path, sizeof(rpw->last_folder_path) - 1);
        rpw->last_folder_path[sizeof(rpw->last_folder_path) - 1] = '\0';
    }
    
    /* Reset icon bar to 0% */
    iTidy_Progress_DrawBarFill(rpw->window->RPort, rpw->icon_bar_x, rpw->icon_bar_y,
                                rpw->icon_bar_w, rpw->icon_bar_h,
                                pens.bar_pen, pens.fill_pen, 0);
    rpw->last_icon_fill_width = 0;
    rpw->last_icon_percent = 0;
    
    /* Update icon count display */
    {
        /* Clear old count */
        iTidy_Progress_ClearTextArea(rpw->window->RPort,
                                      rpw->icon_count_x, rpw->icon_count_y - rpw->font_height + 4,
                                      80, rpw->font_height,
                                      pens.fill_pen);
        
        /* Draw new count */
        if (icons_in_folder > 0) {
            sprintf(text_buf, "0/%u", icons_in_folder);
            iTidy_Progress_DrawTextLabel(rpw->window->RPort, rpw->icon_count_x, rpw->icon_count_y,
                                          text_buf, pens.text_pen);
        }
    }
}

void iTidy_UpdateIconProgress(
    struct iTidy_RecursiveProgressWindow *rpw,
    UWORD icon_index)
{
    iTidy_ProgressPens pens;
    UWORD new_percent;
    ULONG new_fill_width;
    char text_buf[128];
    BOOL icon_changed = FALSE;
    
    if (!rpw || !rpw->window)
        return;
    
    /* Update state */
    rpw->current_icon = icon_index;
    
    /* Handle pending refresh events */
    iTidy_Progress_HandleRefresh(rpw->window, RedrawRecursiveWindow, rpw);
    
    /* Get theme pens */
    if (!iTidy_Progress_GetPens(rpw->screen, &pens))
        return;
    
    /* Apply screen font */
    iTidy_Progress_ApplyScreenFont(rpw->screen, rpw->window->RPort);
    
    /* Calculate new percentage */
    new_percent = (rpw->icons_in_folder > 0) ?
                  (UWORD)((icon_index * 100) / rpw->icons_in_folder) : 0;
    if (new_percent > 100)
        new_percent = 100;
    
    /* Calculate new fill width */
    {
        WORD interior_w = rpw->icon_bar_w - 4;
        new_fill_width = (interior_w * new_percent) / 100;
    }
    
    /* Check if changed */
    icon_changed = (new_fill_width != rpw->last_icon_fill_width);
    
    /* Update icon progress bar if changed */
    if (icon_changed) {
        iTidy_Progress_DrawBarFill(rpw->window->RPort, rpw->icon_bar_x, rpw->icon_bar_y,
                                    rpw->icon_bar_w, rpw->icon_bar_h,
                                    pens.bar_pen, pens.fill_pen, new_percent);
        rpw->last_icon_fill_width = new_fill_width;
        rpw->last_icon_percent = new_percent;
    }
    
    /* Update icon count */
    {
        /* Clear old count */
        iTidy_Progress_ClearTextArea(rpw->window->RPort,
                                      rpw->icon_count_x, rpw->icon_count_y - rpw->font_height + 4,
                                      80, rpw->font_height,
                                      pens.fill_pen);
        
        /* Draw new count */
        sprintf(text_buf, "%u/%u", icon_index, rpw->icons_in_folder);
        iTidy_Progress_DrawTextLabel(rpw->window->RPort, rpw->icon_count_x, rpw->icon_count_y,
                                      text_buf, pens.text_pen);
    }
}

void iTidy_CloseRecursiveProgress(
    struct iTidy_RecursiveProgressWindow *rpw)
{
    if (!rpw)
        return;
    
    if (rpw->window) {
        /* Clear busy pointer */
        SetWindowPointer(rpw->window,
                         WA_Pointer, NULL,
                         TAG_END);
        
        CloseWindow(rpw->window);
        rpw->window = NULL;
    }
    
    FreeMem(rpw, sizeof(iTidy_RecursiveProgressWindow));
}

void iTidy_FreeScanResult(
    struct iTidy_RecursiveScanResult *scan)
{
    ULONG i;
    
    if (!scan)
        return;
    
    /* Free folder path strings */
    if (scan->folderPaths) {
        for (i = 0; i < scan->totalFolders; i++) {
            if (scan->folderPaths[i]) {
                FreeMem(scan->folderPaths[i], strlen(scan->folderPaths[i]) + 1);
            }
        }
        FreeMem(scan->folderPaths, scan->allocated * sizeof(char *));
    }
    
    /* Free icon counts array */
    if (scan->iconCounts) {
        FreeMem(scan->iconCounts, scan->allocated * sizeof(UWORD));
    }
    
    /* Free structure */
    FreeMem(scan, sizeof(iTidy_RecursiveScanResult));
}
