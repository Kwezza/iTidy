/*
 * recursive_progress_reaction.c - Dual-bar recursive progress window (ReAction)
 * Workbench 3.2+ ReAction implementation using FuelGauge for progress bars
 */

/* Library base isolation - prevent linker conflicts with main_window.c */
#define WindowBase iTidy_RecProg_WindowBase
#define LayoutBase iTidy_RecProg_LayoutBase
#define FuelGaugeBase iTidy_RecProg_FuelGaugeBase
#define ButtonBase iTidy_RecProg_ButtonBase

#include "recursive_progress.h"
#include "writeLog.h"

#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/fuelgauge.h>
#include <proto/button.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/fuelgauge.h>
#include <gadgets/button.h>

#include <string.h>
#include <stdio.h>

/* Local library bases */
struct Library *iTidy_RecProg_WindowBase = NULL;
struct Library *iTidy_RecProg_LayoutBase = NULL;
struct Library *iTidy_RecProg_FuelGaugeBase = NULL;
struct Library *iTidy_RecProg_ButtonBase = NULL;

/* Gadget IDs from GUI designer */
enum {
    GID_MAIN_PROGRESS_BAR = 1,
    GID_MAIN_PROGRESS_LABEL,
    GID_SUB_PROGRESS_BAR,
    GID_SUB_PROGRESS_LABEL
};

/*
 * Internal ReAction window structure
 * Replaces the old manual-drawing structure
 */
typedef struct {
    /* ReAction objects */
    Object *window_obj;
    Object *main_layout;
    Object *main_progress_bar;
    Object *main_progress_label;
    Object *sub_progress_bar;
    Object *sub_progress_label;
    
    struct Window *window;
    struct Screen *screen;
    
    /* Display state */
    char task_label[128];
    ULONG total_folders;
    ULONG total_icons;
    ULONG current_folder;
    UWORD current_icon;
    UWORD icons_in_folder;
    char last_folder_path[256];
    
    /* Display options */
    BOOL show_sub_progress;  /* TRUE to show sub progress bar/label */
    
} iTidy_RecursiveProgressWindow_Internal;

/*
 * Open ReAction libraries
 */
static BOOL open_reaction_libs(void)
{
    iTidy_RecProg_WindowBase = OpenLibrary("window.class", 44);
    iTidy_RecProg_LayoutBase = OpenLibrary("gadgets/layout.gadget", 44);
    iTidy_RecProg_FuelGaugeBase = OpenLibrary("gadgets/fuelgauge.gadget", 44);
    iTidy_RecProg_ButtonBase = OpenLibrary("gadgets/button.gadget", 44);
    
    if (!iTidy_RecProg_WindowBase || !iTidy_RecProg_LayoutBase ||
        !iTidy_RecProg_FuelGaugeBase || !iTidy_RecProg_ButtonBase)
    {
        return FALSE;
    }
    
    return TRUE;
}

/*
 * Close ReAction libraries
 */
static void close_reaction_libs(void)
{
    if (iTidy_RecProg_ButtonBase)
    {
        CloseLibrary(iTidy_RecProg_ButtonBase);
        iTidy_RecProg_ButtonBase = NULL;
    }
    if (iTidy_RecProg_FuelGaugeBase)
    {
        CloseLibrary(iTidy_RecProg_FuelGaugeBase);
        iTidy_RecProg_FuelGaugeBase = NULL;
    }
    if (iTidy_RecProg_LayoutBase)
    {
        CloseLibrary(iTidy_RecProg_LayoutBase);
        iTidy_RecProg_LayoutBase = NULL;
    }
    if (iTidy_RecProg_WindowBase)
    {
        CloseLibrary(iTidy_RecProg_WindowBase);
        iTidy_RecProg_WindowBase = NULL;
    }
}

/* ========================================================================
 * Prescan Functions (Unchanged - reused from old implementation)
 * ======================================================================== */

#define INITIAL_FOLDER_CAPACITY 64
#define YIELD_INTERVAL 100

static BOOL AddFolderToScan(iTidy_RecursiveScanResult *scan, const char *path, UWORD icon_count)
{
    /* Expand arrays if needed */
    if (scan->totalFolders >= scan->allocated)
    {
        ULONG new_capacity = scan->allocated * 2;
        char **new_paths;
        UWORD *new_counts;
        
        new_paths = (char **)AllocMem(new_capacity * sizeof(char *), MEMF_CLEAR);
        if (!new_paths)
            return FALSE;
        
        new_counts = (UWORD *)AllocMem(new_capacity * sizeof(UWORD), MEMF_CLEAR);
        if (!new_counts)
        {
            FreeMem(new_paths, new_capacity * sizeof(char *));
            return FALSE;
        }
        
        /* Copy existing data */
        if (scan->folderPaths)
        {
            CopyMem(scan->folderPaths, new_paths, scan->totalFolders * sizeof(char *));
            FreeMem(scan->folderPaths, scan->allocated * sizeof(char *));
        }
        
        if (scan->iconCounts)
        {
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
    if (!fib)
    {
        UnLock(lock);
        return 0;
    }
    
    if (Examine(lock, fib))
    {
        while ((result = ExNext(lock, fib)) != 0)
        {
            if (fib->fib_DirEntryType < 0)
            {
                ULONG name_len = strlen(fib->fib_FileName);
                if (name_len > 5 && 
                    Stricmp(&fib->fib_FileName[name_len - 5], ".info") == 0)
                {
                    icon_count++;
                }
            }
        }
    }
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    return icon_count;
}

static BOOL PrescanRecursiveImpl(const char *path, iTidy_RecursiveScanResult *scan, ULONG *item_count)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    UWORD icon_count;
    LONG result;
    
    icon_count = CountIconsInDirectory(path);
    
    if (!AddFolderToScan(scan, path, icon_count))
        return FALSE;
    
    (*item_count)++;
    if ((*item_count) % YIELD_INTERVAL == 0)
    {
        Delay(1);
    }
    
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
        return TRUE;
    
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        UnLock(lock);
        return TRUE;
    }
    
    if (Examine(lock, fib))
    {
        while ((result = ExNext(lock, fib)) != 0)
        {
            if (fib->fib_DirEntryType > 0)
            {
                char subpath[512];
                ULONG path_len = strlen(path);
                
                if (fib->fib_FileName[0] == '.')
                    continue;
                
                strcpy(subpath, path);
                if (path_len > 0 && subpath[path_len - 1] != ':' && subpath[path_len - 1] != '/')
                {
                    strcat(subpath, "/");
                }
                strcat(subpath, fib->fib_FileName);
                
                if (!PrescanRecursiveImpl(subpath, scan, item_count))
                {
                    FreeDosObject(DOS_FIB, fib);
                    UnLock(lock);
                    return FALSE;
                }
                
                Delay(1);
            }
        }
    }
    
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    
    return TRUE;
}

iTidy_RecursiveScanResult* iTidy_PrescanRecursive(const char *rootPath)
{
    iTidy_RecursiveScanResult *scan;
    ULONG item_count = 0;
    
    if (!rootPath)
        return NULL;
    
    scan = (iTidy_RecursiveScanResult *)AllocMem(sizeof(iTidy_RecursiveScanResult), MEMF_CLEAR);
    if (!scan)
        return NULL;
    
    scan->allocated = INITIAL_FOLDER_CAPACITY;
    scan->folderPaths = (char **)AllocMem(scan->allocated * sizeof(char *), MEMF_CLEAR);
    scan->iconCounts = (UWORD *)AllocMem(scan->allocated * sizeof(UWORD), MEMF_CLEAR);
    
    if (!scan->folderPaths || !scan->iconCounts)
    {
        iTidy_FreeScanResult(scan);
        return NULL;
    }
    
    if (!PrescanRecursiveImpl(rootPath, scan, &item_count))
    {
        iTidy_FreeScanResult(scan);
        return NULL;
    }
    
    return scan;
}

void iTidy_FreeScanResult(iTidy_RecursiveScanResult *scan)
{
    ULONG i;
    
    if (!scan)
        return;
    
    if (scan->folderPaths)
    {
        for (i = 0; i < scan->totalFolders; i++)
        {
            if (scan->folderPaths[i])
            {
                FreeMem(scan->folderPaths[i], strlen(scan->folderPaths[i]) + 1);
            }
        }
        FreeMem(scan->folderPaths, scan->allocated * sizeof(char *));
    }
    
    if (scan->iconCounts)
    {
        FreeMem(scan->iconCounts, scan->allocated * sizeof(UWORD));
    }
    
    FreeMem(scan, sizeof(iTidy_RecursiveScanResult));
}

/* ========================================================================
 * ReAction Window Implementation
 * ======================================================================== */

iTidy_RecursiveProgressWindow* iTidy_OpenRecursiveProgress(
    struct Screen *screen,
    const char *task_label,
    const iTidy_RecursiveScanResult *scan,
    BOOL show_sub_progress)
{
    iTidy_RecursiveProgressWindow_Internal *rpw;
    struct DrawInfo *draw_info = NULL;
    BOOL success = FALSE;
    
    if (!screen || !task_label || !scan)
    {
        log_error(LOG_GUI, "iTidy_OpenRecursiveProgress: Invalid parameters\n");
        return NULL;
    }
    
    /* Open ReAction libraries */
    if (!open_reaction_libs())
    {
        log_error(LOG_GUI, "Failed to open ReAction libraries for recursive progress\n");
        return NULL;
    }
    
    /* Allocate structure */
    rpw = (iTidy_RecursiveProgressWindow_Internal *)AllocMem(
        sizeof(iTidy_RecursiveProgressWindow_Internal), MEMF_CLEAR);
    if (!rpw)
    {
        close_reaction_libs();
        return NULL;
    }
    
    /* Store parameters */
    rpw->screen = screen;
    strncpy(rpw->task_label, task_label, sizeof(rpw->task_label) - 1);
    rpw->task_label[sizeof(rpw->task_label) - 1] = '\0';
    rpw->total_folders = scan->totalFolders;
    rpw->total_icons = scan->totalIcons;
    rpw->current_folder = 0;
    rpw->current_icon = 0;
    rpw->icons_in_folder = 0;
    rpw->last_folder_path[0] = '\0';
    rpw->show_sub_progress = show_sub_progress;
    
    /* Get DrawInfo for label gadget */
    draw_info = GetScreenDrawInfo(screen);
    if (!draw_info)
    {
        log_error(LOG_GUI, "Failed to get DrawInfo\n");
        goto cleanup;
    }
    
    /* Create ReAction window object */
    rpw->window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title,          task_label,
        WA_Width,          450,
        WA_MinWidth,       350,
        WA_MaxWidth,       8192,
        WA_PubScreen,      screen,
        WINDOW_Position,   WPOS_CENTERSCREEN,
        WA_DepthGadget,    TRUE,
        WA_DragBar,        TRUE,
        WA_Activate,       TRUE,
        WA_NoCareRefresh,  TRUE,
        WA_IDCMP,          IDCMP_CLOSEWINDOW,
        
        WINDOW_ParentGroup, rpw->main_layout = NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation,  LAYOUT_ORIENT_VERT,
            LAYOUT_SpaceOuter,   TRUE,
            LAYOUT_DeferLayout,  TRUE,
            LAYOUT_LeftSpacing,  2,
            LAYOUT_RightSpacing, 2,
            LAYOUT_TopSpacing,   2,
            LAYOUT_BottomSpacing,2,
            
            /* Main progress bar (folders) */
            LAYOUT_AddChild, rpw->main_progress_bar = NewObject(FUELGAUGE_GetClass(), NULL,
                GA_ID,              GID_MAIN_PROGRESS_BAR,
                FUELGAUGE_Min,      0,
                FUELGAUGE_Max,      100,
                FUELGAUGE_Level,    0,
                FUELGAUGE_Ticks,    0,
                FUELGAUGE_Percent,  TRUE,
                FUELGAUGE_FillPen,  FILLPEN,
            TAG_END),
            CHILD_MinHeight, 20,
            
            /* Main progress label (using Button for updateable text) */
            LAYOUT_AddChild, rpw->main_progress_label = NewObject(BUTTON_GetClass(), NULL,
                GA_ID,                GID_MAIN_PROGRESS_LABEL,
                GA_ReadOnly,          TRUE,
                GA_Text,              "Starting...",
                BUTTON_BevelStyle,    BVS_NONE,
                BUTTON_Transparent,   TRUE,
                BUTTON_Justification, BCJ_LEFT,
            TAG_END),
            CHILD_MinHeight, 16,
            
            /* Sub progress bar (icons) - only if requested */
            show_sub_progress ? LAYOUT_AddChild : TAG_IGNORE, show_sub_progress ? (rpw->sub_progress_bar = NewObject(FUELGAUGE_GetClass(), NULL,
                GA_ID,              GID_SUB_PROGRESS_BAR,
                FUELGAUGE_Min,      0,
                FUELGAUGE_Max,      100,
                FUELGAUGE_Level,    0,
                FUELGAUGE_Ticks,    0,
                FUELGAUGE_Percent,  TRUE,
                FUELGAUGE_FillPen,  FILLPEN,
            TAG_END)) : NULL,
            show_sub_progress ? CHILD_MinHeight : TAG_IGNORE, 20,
            
            /* Sub progress label (using Button for updateable text) - only if requested */
            show_sub_progress ? LAYOUT_AddChild : TAG_IGNORE, show_sub_progress ? (rpw->sub_progress_label = NewObject(BUTTON_GetClass(), NULL,
                GA_ID,                GID_SUB_PROGRESS_LABEL,
                GA_ReadOnly,          TRUE,
                GA_Text,              "Ready",
                BUTTON_BevelStyle,    BVS_NONE,
                BUTTON_Transparent,   TRUE,
                BUTTON_Justification, BCJ_LEFT,
            TAG_END)) : NULL,
            show_sub_progress ? CHILD_MinHeight : TAG_IGNORE, 16,
            
        TAG_END),
    TAG_END);
    
    if (!rpw->window_obj)
    {
        log_error(LOG_GUI, "Failed to create recursive progress window object\n");
        goto cleanup;
    }
    
    /* Open the window */
    rpw->window = (struct Window *)RA_OpenWindow(rpw->window_obj);
    if (!rpw->window)
    {
        log_error(LOG_GUI, "Failed to open recursive progress window\n");
        goto cleanup;
    }
    
    success = TRUE;
    log_info(LOG_GUI, "Recursive progress window opened: %lu folders, %lu icons\n",
             rpw->total_folders, rpw->total_icons);

cleanup:
    if (draw_info)
    {
        FreeScreenDrawInfo(screen, draw_info);
    }
    
    if (!success)
    {
        if (rpw->window_obj)
        {
            DisposeObject(rpw->window_obj);
        }
        FreeMem(rpw, sizeof(iTidy_RecursiveProgressWindow_Internal));
        close_reaction_libs();
        return NULL;
    }
    
    return (iTidy_RecursiveProgressWindow *)rpw;
}

void iTidy_UpdateFolderProgress(
    iTidy_RecursiveProgressWindow *rpw_public,
    ULONG folder_index,
    const char *folder_path,
    UWORD icons_in_folder)
{
    iTidy_RecursiveProgressWindow_Internal *rpw = (iTidy_RecursiveProgressWindow_Internal *)rpw_public;
    char label_text[256];
    UWORD percent;
    
    if (!rpw || !rpw->window)
        return;
    
    /* Update state */
    rpw->current_folder = folder_index;
    rpw->icons_in_folder = icons_in_folder;
    rpw->current_icon = 0;
    
    if (folder_path)
    {
        strncpy(rpw->last_folder_path, folder_path, sizeof(rpw->last_folder_path) - 1);
        rpw->last_folder_path[sizeof(rpw->last_folder_path) - 1] = '\0';
    }
    
    /* Calculate percentage */
    percent = (rpw->total_folders > 0) ? 
              (UWORD)((folder_index * 100) / rpw->total_folders) : 0;
    if (percent > 100)
        percent = 100;
    
    /* Update main progress bar */
    SetGadgetAttrs((struct Gadget *)rpw->main_progress_bar, rpw->window, NULL,
        FUELGAUGE_Level, percent,
        TAG_DONE);
    
    /* Update main label with descriptive text */
    if (folder_path && folder_path[0])
    {
        snprintf(label_text, sizeof(label_text), "Processing: %s (%lu/%lu)",
                 folder_path, folder_index, rpw->total_folders);
    }
    else
    {
        snprintf(label_text, sizeof(label_text), "Folder %lu of %lu",
                 folder_index, rpw->total_folders);
    }
    
    SetGadgetAttrs((struct Gadget *)rpw->main_progress_label, rpw->window, NULL,
        GA_Text, label_text,
        TAG_DONE);
    
    /* Reset sub progress bar to 0% (only if visible) */
    if (rpw->show_sub_progress && rpw->sub_progress_bar)
    {
        SetGadgetAttrs((struct Gadget *)rpw->sub_progress_bar, rpw->window, NULL,
            FUELGAUGE_Level, 0,
            TAG_DONE);
        
        /* Update sub label */
        if (icons_in_folder > 0)
        {
            snprintf(label_text, sizeof(label_text), "Icons: 0/%u", icons_in_folder);
        }
        else
        {
            strcpy(label_text, "No icons in folder");
        }
        
        SetGadgetAttrs((struct Gadget *)rpw->sub_progress_label, rpw->window, NULL,
            GA_Text, label_text,
            TAG_DONE);
    }
}

void iTidy_UpdateIconProgress(
    iTidy_RecursiveProgressWindow *rpw_public,
    UWORD icon_index)
{
    iTidy_RecursiveProgressWindow_Internal *rpw = (iTidy_RecursiveProgressWindow_Internal *)rpw_public;
    char label_text[256];
    UWORD percent;
    
    if (!rpw || !rpw->window)
        return;
    
    /* Only update if sub progress gadgets exist */
    if (!rpw->show_sub_progress || !rpw->sub_progress_bar)
        return;
    
    /* Update state */
    rpw->current_icon = icon_index;
    
    /* Calculate percentage */
    percent = (rpw->icons_in_folder > 0) ?
              (UWORD)((icon_index * 100) / rpw->icons_in_folder) : 0;
    if (percent > 100)
        percent = 100;
    
    /* Update sub progress bar */
    SetGadgetAttrs((struct Gadget *)rpw->sub_progress_bar, rpw->window, NULL,
        FUELGAUGE_Level, percent,
        TAG_DONE);
    
    /* Update sub label */
    snprintf(label_text, sizeof(label_text), "Icons: %u/%u", icon_index, rpw->icons_in_folder);
    
    SetGadgetAttrs((struct Gadget *)rpw->sub_progress_label, rpw->window, NULL,
        GA_Text, label_text,
        TAG_DONE);
}

void iTidy_CloseRecursiveProgress(iTidy_RecursiveProgressWindow *rpw_public)
{
    iTidy_RecursiveProgressWindow_Internal *rpw = (iTidy_RecursiveProgressWindow_Internal *)rpw_public;
    
    if (!rpw)
        return;
    
    log_info(LOG_GUI, "Closing recursive progress window\n");
    
    if (rpw->window_obj)
    {
        DisposeObject(rpw->window_obj);
        rpw->window_obj = NULL;
        rpw->window = NULL;
    }
    
    FreeMem(rpw, sizeof(iTidy_RecursiveProgressWindow_Internal));
    
    close_reaction_libs();
}
