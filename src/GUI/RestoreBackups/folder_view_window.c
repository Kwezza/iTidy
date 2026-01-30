/*
 * folder_view_window.c - iTidy Folder View Window Implementation
 * ReAction-based GUI with Hierarchical ListBrowser for Workbench 3.2+
 * 
 * Displays backup folder hierarchy using native tree display instead of
 * the old dot-indented faux tree approach.
 */

/* Library base isolation - MUST be before any proto headers */
#define WindowBase      iTidy_FolderView_WindowBase
#define LayoutBase      iTidy_FolderView_LayoutBase
#define ButtonBase      iTidy_FolderView_ButtonBase
#define ListBrowserBase iTidy_FolderView_ListBrowserBase
#define LabelBase       iTidy_FolderView_LabelBase
#define GlyphBase       iTidy_FolderView_GlyphBase

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <intuition/imageclass.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

/* ReAction headers */
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/listbrowser.h>
#include <proto/label.h>
#include <proto/glyph.h>

#include <clib/alib_protos.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/listbrowser.h>
#include <images/label.h>
#include <images/glyph.h>

#include <string.h>
#include <stdio.h>

/* iTidy headers - after system headers to avoid macro conflicts */
#include "platform/platform.h"
#include "folder_view_window.h"
#include "writeLog.h"
#include "GUI/gui_utilities.h"
#include "../../helpers/exec_list_compat.h"

/*------------------------------------------------------------------------*/
/* Library Bases (Prefixed to avoid collision with main_window.c)         */
/*------------------------------------------------------------------------*/
struct Library *iTidy_FolderView_WindowBase = NULL;
struct Library *iTidy_FolderView_LayoutBase = NULL;
struct Library *iTidy_FolderView_ButtonBase = NULL;
struct Library *iTidy_FolderView_ListBrowserBase = NULL;
struct Library *iTidy_FolderView_LabelBase = NULL;
struct Library *iTidy_FolderView_GlyphBase = NULL;

/* Glyph images for hierarchical display */
static Object *show_image = NULL;   /* Right arrow - expand */
static Object *hide_image = NULL;   /* Down arrow - collapse */

/*------------------------------------------------------------------------*/
/* Forward Declarations                                                   */
/*------------------------------------------------------------------------*/
static BOOL open_folder_view_classes(void);
static void close_folder_view_classes(void);
static UWORD calculate_folder_depth(const char *path);
static const char *get_folder_name(const char *path);
static BOOL parse_catalog_callback(const char *line, struct iTidyFolderViewWindow *folder_data);
static iTidyFolderNodeData *alloc_node_data(struct iTidyFolderViewWindow *folder_data);
static void track_node_data(struct iTidyFolderViewWindow *folder_data, iTidyFolderNodeData *data);
static struct Node *create_listbrowser_node(const char *name, const char *info, 
                                            UWORD generation, BOOL has_children,
                                            iTidyFolderNodeData *user_data);

/*------------------------------------------------------------------------*/
/* Library Management                                                     */
/*------------------------------------------------------------------------*/

static BOOL open_folder_view_classes(void)
{
    if (!WindowBase)
        WindowBase = OpenLibrary("window.class", 44L);
    if (!LayoutBase)
        LayoutBase = OpenLibrary("gadgets/layout.gadget", 44L);
    if (!ButtonBase)
        ButtonBase = OpenLibrary("gadgets/button.gadget", 44L);
    if (!ListBrowserBase)
        ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 44L);
    if (!LabelBase)
        LabelBase = OpenLibrary("images/label.image", 44L);
    if (!GlyphBase)
        GlyphBase = OpenLibrary("images/glyph.image", 44L);
    
    if (!WindowBase || !LayoutBase || !ButtonBase || !ListBrowserBase)
    {
        log_error(LOG_GUI, "FolderView: Failed to open ReAction classes\n");
        close_folder_view_classes();
        return FALSE;
    }
    
    /* Create glyph images for hierarchical display */
    if (GlyphBase)
    {
        /* GLYPH_RIGHTARROW for "show children" (collapsed state) */
        /* IM_DRAWFRAME will scale to fit, but give a hint size */
        show_image = NewObject(GLYPH_GetClass(), NULL,
            GLYPH_Glyph, GLYPH_RIGHTARROW,
            IA_Width, 8,
            IA_Height, 8,
            TAG_END);
        
        /* GLYPH_DOWNARROW for "hide children" (expanded state) */
        hide_image = NewObject(GLYPH_GetClass(), NULL,
            GLYPH_Glyph, GLYPH_DOWNARROW,
            IA_Width, 8,
            IA_Height, 8,
            TAG_END);
        
        if (show_image && hide_image)
        {
            log_info(LOG_GUI, "FolderView: Created glyph images for tree\n");
        }
        else
        {
            log_warning(LOG_GUI, "FolderView: Could not create glyph images (show=%p, hide=%p)\n",
                       show_image, hide_image);
        }
    }
    else
    {
        log_warning(LOG_GUI, "FolderView: GlyphBase not available\n");
    }
    
    log_info(LOG_GUI, "FolderView: ReAction classes opened\n");
    return TRUE;
}

static void close_folder_view_classes(void)
{
    /* Dispose glyph images first */
    if (hide_image)      { DisposeObject(hide_image);     hide_image = NULL; }
    if (show_image)      { DisposeObject(show_image);     show_image = NULL; }
    
    if (GlyphBase)       { CloseLibrary(GlyphBase);       GlyphBase = NULL; }
    if (LabelBase)       { CloseLibrary(LabelBase);       LabelBase = NULL; }
    if (ListBrowserBase) { CloseLibrary(ListBrowserBase); ListBrowserBase = NULL; }
    if (ButtonBase)      { CloseLibrary(ButtonBase);      ButtonBase = NULL; }
    if (LayoutBase)      { CloseLibrary(LayoutBase);      LayoutBase = NULL; }
    if (WindowBase)      { CloseLibrary(WindowBase);      WindowBase = NULL; }
    
    log_info(LOG_GUI, "FolderView: ReAction classes closed\n");
}

/*------------------------------------------------------------------------*/
/* Node Data Management                                                   */
/*------------------------------------------------------------------------*/

/**
 * @brief Allocate and zero-initialize node data structure
 */
static iTidyFolderNodeData *alloc_node_data(struct iTidyFolderViewWindow *folder_data)
{
    iTidyFolderNodeData *data = (iTidyFolderNodeData *)whd_malloc(sizeof(iTidyFolderNodeData));
    if (data)
    {
        memset(data, 0, sizeof(iTidyFolderNodeData));
    }
    return data;
}

/**
 * @brief Track allocated node data for cleanup
 */
static void track_node_data(struct iTidyFolderViewWindow *folder_data, iTidyFolderNodeData *data)
{
    struct NodeDataTracker *tracker;
    
    if (!folder_data || !data)
        return;
    
    tracker = (struct NodeDataTracker *)whd_malloc(sizeof(struct NodeDataTracker));
    if (tracker)
    {
        memset(tracker, 0, sizeof(struct NodeDataTracker));
        tracker->data = data;
        AddTail(&folder_data->node_data_list, (struct Node *)tracker);
    }
}

/**
 * @brief Free all tracked node data
 */
static void free_tracked_node_data(struct iTidyFolderViewWindow *folder_data)
{
    struct NodeDataTracker *tracker;
    struct NodeDataTracker *next;
    
    if (!folder_data)
        return;
    
    tracker = (struct NodeDataTracker *)folder_data->node_data_list.lh_Head;
    while (tracker->node.ln_Succ)
    {
        next = (struct NodeDataTracker *)tracker->node.ln_Succ;
        Remove((struct Node *)tracker);
        
        if (tracker->data)
        {
            if (tracker->data->path)
                whd_free(tracker->data->path);
            if (tracker->data->display_name)
                whd_free(tracker->data->display_name);
            if (tracker->data->size_info)
                whd_free(tracker->data->size_info);
            whd_free(tracker->data);
        }
        whd_free(tracker);
        
        tracker = next;
    }
}

/*------------------------------------------------------------------------*/
/* ListBrowser Node Creation                                              */
/*------------------------------------------------------------------------*/

/**
 * @brief Create a single ListBrowser node for hierarchical display
 * 
 * @param name Display name for column 1
 * @param info Size/info for column 2
 * @param generation Hierarchy level (1 = root, 2 = child, etc.)
 * @param has_children TRUE if node can be expanded
 * @param user_data User data pointer to store
 * @return Allocated ListBrowser node or NULL
 */
static struct Node *create_listbrowser_node(const char *name, const char *info,
                                            UWORD generation, BOOL has_children,
                                            iTidyFolderNodeData *user_data)
{
    struct Node *node;
    ULONG flags = 0;
    
    /* Set HASCHILDREN flag for non-leaf nodes */
    if (has_children)
    {
        flags = LBFLG_HASCHILDREN;
    }
    
    /* DEBUG: Log node creation */
    log_info(LOG_GUI, "CREATE_NODE: name='%s' gen=%d has_children=%s flags=0x%08lx\n",
             name ? name : "(null)", (int)generation, 
             has_children ? "TRUE" : "FALSE", (unsigned long)flags);
    
    /* Allocate the node with 2 columns */
    /* CRITICAL: LBNCA_CopyText MUST come BEFORE LBNCA_Text for proper operation */
    node = AllocListBrowserNode(2,
        LBNA_Generation, generation,
        LBNA_Flags, flags,
        LBNA_UserData, (ULONG)user_data,
        LBNA_Column, 0,
            LBNCA_CopyText, TRUE,
            LBNCA_Text, name ? name : "",
        LBNA_Column, 1,
            LBNCA_CopyText, TRUE,
            LBNCA_Text, info ? info : "",
        TAG_END);
    
    if (!node)
    {
        log_error(LOG_GUI, "CREATE_NODE: AllocListBrowserNode FAILED!\n");
    }
    
    return node;
}

/*------------------------------------------------------------------------*/
/* Window Open Implementation                                             */
/*------------------------------------------------------------------------*/

BOOL open_folder_view_window(struct iTidyFolderViewWindow *folder_data,
                             const char *catalog_path,
                             UWORD run_number,
                             const char *date_str,
                             ULONG archive_count)
{
    if (!folder_data || !folder_data->screen)
    {
        log_error(LOG_GUI, "FolderView: Invalid folder_data or screen\n");
        return FALSE;
    }
    
    log_info(LOG_GUI, "=== FOLDER VIEW WINDOW (ReAction) - Opening ===\n");
    log_info(LOG_GUI, "Catalog path: %s\n", catalog_path ? catalog_path : "NULL");
    log_info(LOG_GUI, "Run number: %u\n", (unsigned int)run_number);
    
    /* Clear structure but preserve screen pointer */
    struct Screen *saved_screen = folder_data->screen;
    memset(folder_data, 0, sizeof(struct iTidyFolderViewWindow));
    folder_data->screen = saved_screen;
    
    /* Initialize data */
    folder_data->run_number = run_number;
    folder_data->archive_count = archive_count;
    
    /* Initialize node data tracking list */
    NewList(&folder_data->node_data_list);
    
    /* Set the window title */
    if (date_str != NULL)
    {
        sprintf(folder_data->window_title, "Folder View - Run %u (%s)", 
                (unsigned int)run_number, date_str);
        strncpy(folder_data->date_str, date_str, sizeof(folder_data->date_str) - 1);
        folder_data->date_str[sizeof(folder_data->date_str) - 1] = '\0';
    }
    else
    {
        sprintf(folder_data->window_title, "Folder View - Run %u", 
                (unsigned int)run_number);
        strcpy(folder_data->date_str, "Unknown");
    }
    
    sprintf(folder_data->run_name, "Run_%04u", (unsigned int)run_number);
    log_info(LOG_GUI, "FolderView: Title: %s\n", folder_data->window_title);
    
    /* Open ReAction classes */
    if (!open_folder_view_classes())
    {
        log_error(LOG_GUI, "FolderView: Failed to open ReAction classes\n");
        return FALSE;
    }
    
    /* Allocate the folder list for ListBrowser */
    folder_data->folder_list = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
    if (!folder_data->folder_list)
    {
        log_error(LOG_GUI, "FolderView: Failed to allocate folder list\n");
        close_folder_view_classes();
        return FALSE;
    }
    NewList(folder_data->folder_list);
    
    /* Allocate column info for 2 columns */
    folder_data->column_info = (struct ColumnInfo *)AllocMem(sizeof(struct ColumnInfo) * 3, MEMF_PUBLIC | MEMF_CLEAR);
    if (!folder_data->column_info)
    {
        log_error(LOG_GUI, "FolderView: Failed to allocate column info\n");
        FreeMem(folder_data->folder_list, sizeof(struct List));
        folder_data->folder_list = NULL;
        close_folder_view_classes();
        return FALSE;
    }
    
    /* Setup columns: Folder Name (70%) | Size/Icons (30%) */
    folder_data->column_info[0].ci_Width = 70;
    folder_data->column_info[0].ci_Title = "Folder";
    folder_data->column_info[0].ci_Flags = CIF_WEIGHTED;
    
    folder_data->column_info[1].ci_Width = 30;
    folder_data->column_info[1].ci_Title = "Size / Icons";
    folder_data->column_info[1].ci_Flags = CIF_WEIGHTED;
    
    /* Proper terminator - ALL fields must be set */
    folder_data->column_info[2].ci_Width = -1;
    folder_data->column_info[2].ci_Title = (STRPTR)~0;
    folder_data->column_info[2].ci_Flags = -1;
    
    /* CRITICAL: Parse catalog and build tree BEFORE creating the gadget */
    /* This is required for hierarchical ListBrowser to show disclosure triangles */
    if (catalog_path != NULL)
    {
        log_info(LOG_GUI, "FolderView: Parsing catalog before creating window...\n");
        if (!parse_catalog_and_build_tree(catalog_path, folder_data))
        {
            log_warning(LOG_GUI, "FolderView: Failed to parse catalog\n");
            /* Continue anyway - show empty list */
        }
        
        /* Hide all children to start collapsed - MUST be done before attaching to gadget */
        /* This is the key step that makes disclosure triangles appear */
        log_info(LOG_GUI, "=== BEFORE HideAllListBrowserChildren ===\n");
        HideAllListBrowserChildren(folder_data->folder_list);
        log_info(LOG_GUI, "=== AFTER HideAllListBrowserChildren ===\n");
        
        /* DEBUG: Dump final state of all nodes */
        {
            struct Node *node;
            int idx = 0;
            log_info(LOG_GUI, "=== FINAL NODE STATE DUMP ===\n");
            node = folder_data->folder_list->lh_Head;
            while (node->ln_Succ)
            {
                ULONG gen = 0;
                ULONG flags = 0;
                GetListBrowserNodeAttrs(node,
                    LBNA_Generation, &gen,
                    LBNA_Flags, &flags,
                    TAG_END);
                log_info(LOG_GUI, "FINAL[%d]: gen=%d flags=0x%08lx (HASCHILDREN=%s HIDDEN=%s SHOWCHILDREN=%s)\n",
                         idx, (int)gen, (unsigned long)flags,
                         (flags & LBFLG_HASCHILDREN) ? "YES" : "no",
                         (flags & LBFLG_HIDDEN) ? "YES" : "no",
                         (flags & LBFLG_SHOWCHILDREN) ? "YES" : "no");
                idx++;
                node = node->ln_Succ;
            }
            log_info(LOG_GUI, "=== END DUMP (%d nodes) ===\n", idx);
        }
    }
    
    /* Create the ReAction window object */
    folder_data->window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title,           folder_data->window_title,
        WA_PubScreen,       folder_data->screen,
        WA_Left,            100,
        WA_Top,             50,
        WA_Width,           FOLDER_VIEW_WIDTH,
        WA_Height,          FOLDER_VIEW_HEIGHT,
        WA_MinWidth,        FOLDER_VIEW_MIN_WIDTH,
        WA_MinHeight,       FOLDER_VIEW_MIN_HEIGHT,
        WA_MaxWidth,        8192,
        WA_MaxHeight,       8192,
        WA_CloseGadget,     TRUE,
        WA_DepthGadget,     TRUE,
        WA_SizeGadget,      TRUE,
        WA_DragBar,         TRUE,
        WA_Activate,        TRUE,
        WA_NoCareRefresh,   TRUE,
        WINDOW_Position,    WPOS_CENTERMOUSE,
        WA_IDCMP,           IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE,
        
        WINDOW_ParentGroup, folder_data->main_layout = NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_Orientation,   LAYOUT_ORIENT_VERT,
            LAYOUT_SpaceOuter,    TRUE,
            LAYOUT_DeferLayout,   TRUE,
            
            /* ListBrowser - Hierarchical display */
            LAYOUT_AddChild, folder_data->listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
                GA_ID,                      GID_FOLDER_VIEW_LISTBROWSER,
                GA_RelVerify,               TRUE,
                GA_TabCycle,                TRUE,
                LISTBROWSER_Labels,         folder_data->folder_list,
                LISTBROWSER_ColumnInfo,     folder_data->column_info,
                LISTBROWSER_ColumnTitles,   TRUE,
                LISTBROWSER_Hierarchical,   TRUE,
                LISTBROWSER_ShowSelected,   TRUE,
                LISTBROWSER_AutoFit,        TRUE,
                LISTBROWSER_HorizSeparators, FALSE,
                LISTBROWSER_VertSeparators, TRUE,
                /* Use default disclosure triangles - do NOT set custom images
                 * as per official documentation recommendation */
            TAG_END),
            CHILD_WeightedHeight, 90,
            
            /* Close Button */
            LAYOUT_AddChild, folder_data->close_button_obj = NewObject(BUTTON_GetClass(), NULL,
                GA_ID,                  GID_FOLDER_VIEW_CLOSE_BTN,
                GA_Text,                "_Close",
                GA_RelVerify,           TRUE,
                GA_TabCycle,            TRUE,
                BUTTON_TextPen,         1,
                BUTTON_BackgroundPen,   0,
                BUTTON_FillTextPen,     1,
                BUTTON_FillPen,         3,
            TAG_END),
            CHILD_WeightedHeight, 10,
            
        TAG_END),
    TAG_END);
    
    if (!folder_data->window_obj)
    {
        log_error(LOG_GUI, "FolderView: Failed to create window object\n");
        FreeMem(folder_data->column_info, sizeof(struct ColumnInfo) * 3);
        FreeMem(folder_data->folder_list, sizeof(struct List));
        folder_data->column_info = NULL;
        folder_data->folder_list = NULL;
        close_folder_view_classes();
        return FALSE;
    }
    
    /* Open the window */
    folder_data->window = (struct Window *)RA_OpenWindow(folder_data->window_obj);
    if (!folder_data->window)
    {
        log_error(LOG_GUI, "FolderView: Failed to open window\n");
        DisposeObject(folder_data->window_obj);
        folder_data->window_obj = NULL;
        FreeMem(folder_data->column_info, sizeof(struct ColumnInfo) * 3);
        FreeMem(folder_data->folder_list, sizeof(struct List));
        folder_data->column_info = NULL;
        folder_data->folder_list = NULL;
        close_folder_view_classes();
        return FALSE;
    }
    
    folder_data->window_open = TRUE;
    log_info(LOG_GUI, "FolderView: Window opened at %p\n", folder_data->window);
    
    /* Clear busy pointer */
    safe_set_window_pointer(folder_data->window, FALSE);
    
    log_info(LOG_GUI, "FolderView: Window ready\n");
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Event Handling                                                         */
/*------------------------------------------------------------------------*/

BOOL handle_folder_view_window_events(struct iTidyFolderViewWindow *folder_data)
{
    ULONG result;
    UWORD code;
    
    if (!folder_data || !folder_data->window_obj || !folder_data->window_open)
    {
        return FALSE;
    }
    
    /* Non-blocking check for events */
    while ((result = RA_HandleInput(folder_data->window_obj, &code)) != WMHI_LASTMSG)
    {
        switch (result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                log_info(LOG_GUI, "FolderView: Close window requested\n");
                return FALSE;  /* Signal to close */
                
            case WMHI_GADGETUP:
                switch (result & WMHI_GADGETMASK)
                {
                    case GID_FOLDER_VIEW_CLOSE_BTN:
                        log_info(LOG_GUI, "FolderView: Close button clicked\n");
                        return FALSE;  /* Signal to close */
                        
                    case GID_FOLDER_VIEW_LISTBROWSER:
                        /* List item selected - could add details view later */
                        break;
                }
                break;
        }
    }
    
    return TRUE;  /* Continue running */
}

/*------------------------------------------------------------------------*/
/* Window Close Implementation                                            */
/*------------------------------------------------------------------------*/

void close_folder_view_window(struct iTidyFolderViewWindow *folder_data)
{
    if (!folder_data)
    {
        return;
    }
    
    log_info(LOG_GUI, "=== FOLDER VIEW WINDOW - Closing ===\n");
    
    /* Dispose window object (also closes window and frees gadgets) */
    if (folder_data->window_obj)
    {
        /* Detach list before disposing */
        if (folder_data->listbrowser_obj && folder_data->window)
        {
            SetGadgetAttrs((struct Gadget *)folder_data->listbrowser_obj,
                           folder_data->window, NULL,
                           LISTBROWSER_Labels, ~0,
                           TAG_END);
        }
        
        DisposeObject(folder_data->window_obj);
        folder_data->window_obj = NULL;
        folder_data->window = NULL;
        folder_data->main_layout = NULL;
        folder_data->listbrowser_obj = NULL;
        folder_data->close_button_obj = NULL;
    }
    
    folder_data->window_open = FALSE;
    
    /* Free ListBrowser nodes */
    if (folder_data->folder_list)
    {
        FreeListBrowserList(folder_data->folder_list);
        FreeMem(folder_data->folder_list, sizeof(struct List));
        folder_data->folder_list = NULL;
    }
    
    /* Free tracked node data */
    free_tracked_node_data(folder_data);
    
    /* Free column info */
    if (folder_data->column_info)
    {
        FreeMem(folder_data->column_info, sizeof(struct ColumnInfo) * 3);
        folder_data->column_info = NULL;
    }
    
    /* Close ReAction classes */
    close_folder_view_classes();
    
    log_info(LOG_GUI, "=== Folder view window closed ===\n");
}

/*------------------------------------------------------------------------*/
/* Catalog Parsing and Tree Building                                      */
/*------------------------------------------------------------------------*/

BOOL parse_catalog_and_build_tree(const char *catalog_path,
                                  struct iTidyFolderViewWindow *folder_data)
{
    BPTR file_handle;
    char line_buffer[512];
    BOOL success = TRUE;
    
    if (!catalog_path || !folder_data)
    {
        log_error(LOG_GUI, "FolderView: Invalid parameters for parse_catalog\n");
        return FALSE;
    }
    
    log_info(LOG_GUI, "FolderView: Opening catalog: %s\n", catalog_path);
    
    file_handle = Open(catalog_path, MODE_OLDFILE);
    if (file_handle == 0)
    {
        log_error(LOG_GUI, "FolderView: Cannot open catalog file\n");
        return FALSE;
    }
    
    /* Read file line by line */
    while (FGets(file_handle, line_buffer, sizeof(line_buffer) - 1))
    {
        /* Remove trailing newline */
        UWORD len = strlen(line_buffer);
        if (len > 0 && line_buffer[len - 1] == '\n')
        {
            line_buffer[len - 1] = '\0';
        }
        
        /* Skip empty lines */
        if (strlen(line_buffer) == 0)
        {
            continue;
        }
        
        /* Process the line */
        if (!parse_catalog_callback(line_buffer, folder_data))
        {
            /* Log but continue on error */
            log_warning(LOG_GUI, "FolderView: Failed to parse line\n");
        }
    }
    
    Close(file_handle);
    log_info(LOG_GUI, "FolderView: Catalog parsed\n");
    
    /* Post-process: Mark nodes that have children with LBFLG_HASCHILDREN */
    /* A node has children if the next node has a higher generation number */
    log_info(LOG_GUI, "=== POST-PROCESS: Marking parent nodes ===");
    {
        struct Node *curr_node;
        struct Node *next_node;
        int node_index = 0;
        
        curr_node = folder_data->folder_list->lh_Head;
        while (curr_node->ln_Succ)
        {
            next_node = curr_node->ln_Succ;
            
            if (next_node->ln_Succ)  /* next_node is valid (not tail) */
            {
                ULONG curr_gen = 0;
                ULONG next_gen = 0;
                ULONG curr_flags = 0;
                
                /* Get generation numbers */
                /* NOTE: GetListBrowserNodeAttrs writes to ULONG pointers, not WORD! */
                GetListBrowserNodeAttrs(curr_node,
                    LBNA_Generation, &curr_gen,
                    LBNA_Flags, &curr_flags,
                    TAG_END);
                
                GetListBrowserNodeAttrs(next_node,
                    LBNA_Generation, &next_gen,
                    TAG_END);
                
                log_info(LOG_GUI, "POST[%d]: curr_gen=%d next_gen=%d curr_flags=0x%08lx\n",
                         node_index, (int)curr_gen, (int)next_gen, (unsigned long)curr_flags);
                
                /* If next node has higher generation, current is a parent */
                if (next_gen > curr_gen)
                {
                    ULONG new_flags = curr_flags | LBFLG_HASCHILDREN;
                    /* Set only HASCHILDREN flag for parent nodes */
                    /* HideAllListBrowserChildren() will be called after to collapse */
                    SetListBrowserNodeAttrs(curr_node,
                        LBNA_Flags, new_flags,
                        TAG_END);
                    
                    log_info(LOG_GUI, "POST[%d]: MARKED AS PARENT - new_flags=0x%08lx\n",
                             node_index, (unsigned long)new_flags);
                }
            }
            
            node_index++;
            curr_node = next_node;
        }
    }
    log_info(LOG_GUI, "=== POST-PROCESS: Complete ===");
    
    return success;
}

/**
 * @brief Calculate the depth of a folder path
 */
static UWORD calculate_folder_depth(const char *path)
{
    UWORD depth = 0;
    const char *p = path;
    BOOL after_colon = FALSE;
    
    if (!path)
        return 0;
    
    /* Skip leading ".." references */
    while (strncmp(p, "../", 3) == 0)
    {
        p += 3;
    }
    
    /* Count directory separators */
    while (*p != '\0')
    {
        if (*p == ':')
        {
            after_colon = TRUE;
        }
        else if (*p == '/')
        {
            if (after_colon || strchr(path, ':') == NULL)
            {
                depth++;
            }
        }
        p++;
    }
    
    return depth;
}

/**
 * @brief Get the folder name from a full path
 */
static const char *get_folder_name(const char *path)
{
    const char *last_slash;
    const char *colon;
    
    if (!path)
        return "";
    
    last_slash = strrchr(path, '/');
    colon = strchr(path, ':');
    
    if (last_slash != NULL)
    {
        if (*(last_slash + 1) != '\0')
        {
            return last_slash + 1;
        }
        else
        {
            /* Handle trailing slash */
            const char *prev_slash = last_slash - 1;
            while (prev_slash >= path && *prev_slash != '/')
            {
                prev_slash--;
            }
            if (prev_slash >= path)
            {
                return prev_slash + 1;
            }
        }
    }
    
    if (colon != NULL)
    {
        return path;  /* Return full path for root */
    }
    
    return path;
}

/**
 * @brief Process a single line from the catalog file
 */
static BOOL parse_catalog_callback(const char *line, struct iTidyFolderViewWindow *folder_data)
{
    char *path_start;
    char *pipe1, *pipe2, *pipe3, *pipe4;
    char size_str[32];
    char icons_str[16];
    char combined_str[64];
    char temp_path[256];
    UWORD path_column_len;
    UWORD depth;
    UWORD generation;
    struct Node *lb_node;
    iTidyFolderNodeData *node_data;
    const char *folder_name;
    
    if (!line || !folder_data)
        return FALSE;
    
    /* Skip empty lines and headers */
    if (strlen(line) == 0)
        return TRUE;
    
    if (strstr(line, "====") != NULL || 
        strstr(line, "iTidy") != NULL || 
        strstr(line, "Run Number") != NULL ||
        strstr(line, "Session") != NULL || 
        strstr(line, "LhA Path") != NULL ||
        strstr(line, "# Index") != NULL || 
        strstr(line, "-----") != NULL ||
        strstr(line, "Total") != NULL)
    {
        return TRUE;  /* Skip header/separator lines */
    }
    
    /* Find pipes to locate columns */
    /* Format: "00001.lha | 000/ | 11 KB | 15 | PC:Workbench | ... */
    pipe1 = strchr(line, '|');
    if (!pipe1) return TRUE;
    
    pipe2 = strchr(pipe1 + 1, '|');
    if (!pipe2) return TRUE;
    
    pipe3 = strchr(pipe2 + 1, '|');
    if (!pipe3) return TRUE;
    
    pipe4 = strchr(pipe3 + 1, '|');
    if (!pipe4) return TRUE;
    
    /* Get the Original Path (5th column, after 4th pipe) */
    path_start = pipe4 + 1;
    while (*path_start == ' ' || *path_start == '\t')
        path_start++;
    
    /* Find end of path column */
    char *path_end = strchr(path_start, '|');
    if (path_end)
        path_column_len = path_end - path_start;
    else
        path_column_len = strlen(path_start);
    
    /* Check for valid path */
    if (path_column_len == 0)
        return TRUE;
    
    if (strchr(path_start, '/') == NULL && strchr(path_start, ':') == NULL)
        return TRUE;
    
    /* Extract size from 3rd column */
    size_str[0] = '\0';
    {
        char *size_start = pipe2 + 1;
        while (*size_start == ' ' || *size_start == '\t')
            size_start++;
        
        UWORD i = 0;
        while (size_start < pipe3 && i < sizeof(size_str) - 1)
        {
            if (*size_start == ' ' || *size_start == '\t')
            {
                /* Skip space but continue for unit */
                if (i > 0 && size_str[i-1] != ' ')
                {
                    size_str[i++] = ' ';
                }
            }
            else
            {
                size_str[i++] = *size_start;
            }
            size_start++;
        }
        /* Trim trailing space */
        while (i > 0 && size_str[i-1] == ' ')
            i--;
        size_str[i] = '\0';
    }
    
    /* Extract icons count from 4th column */
    icons_str[0] = '\0';
    {
        char *icons_start = pipe3 + 1;
        while (*icons_start == ' ' || *icons_start == '\t')
            icons_start++;
        
        UWORD i = 0;
        while (icons_start < pipe4 && i < sizeof(icons_str) - 1 &&
               *icons_start != ' ' && *icons_start != '\t')
        {
            icons_str[i++] = *icons_start++;
        }
        icons_str[i] = '\0';
    }
    
    /* Combine size and icons */
    if (icons_str[0] != '\0')
    {
        sprintf(combined_str, "%s / %s icons", size_str, icons_str);
    }
    else
    {
        strcpy(combined_str, size_str);
    }
    
    /* Copy and clean path */
    if (path_column_len >= sizeof(temp_path))
        path_column_len = sizeof(temp_path) - 1;
    
    strncpy(temp_path, path_start, path_column_len);
    temp_path[path_column_len] = '\0';
    
    /* Trim trailing whitespace */
    {
        UWORD len = strlen(temp_path);
        while (len > 0 && (temp_path[len-1] == ' ' || temp_path[len-1] == '\t' ||
                          temp_path[len-1] == '\n' || temp_path[len-1] == '\r'))
        {
            temp_path[--len] = '\0';
        }
    }
    
    /* Calculate depth and generation */
    depth = calculate_folder_depth(temp_path);
    generation = depth;  /* ListBrowser generations: 0 = root, 1 = first level children, etc. */
    
    /* Get folder name for display */
    folder_name = get_folder_name(temp_path);
    
    /* Allocate and setup node user data */
    node_data = alloc_node_data(folder_data);
    if (!node_data)
    {
        log_error(LOG_GUI, "FolderView: Failed to allocate node data\n");
        return FALSE;
    }
    
    /* Copy strings */
    node_data->path = (char *)whd_malloc(strlen(temp_path) + 1);
    if (node_data->path)
    {
        strcpy(node_data->path, temp_path);
    }
    
    node_data->display_name = (char *)whd_malloc(strlen(folder_name) + 1);
    if (node_data->display_name)
    {
        strcpy(node_data->display_name, folder_name);
    }
    
    node_data->size_info = (char *)whd_malloc(strlen(combined_str) + 1);
    if (node_data->size_info)
    {
        strcpy(node_data->size_info, combined_str);
    }
    
    node_data->depth = depth;
    node_data->has_children = FALSE;  /* Will be updated if children found */
    
    /* Track for cleanup */
    track_node_data(folder_data, node_data);
    
    /* Create the ListBrowser node */
    lb_node = create_listbrowser_node(folder_name, combined_str, generation, FALSE, node_data);
    if (lb_node)
    {
        AddTail(folder_data->folder_list, lb_node);
        log_debug(LOG_GUI, "FolderView: Added node: %s (gen %d)\n", folder_name, generation);
    }
    else
    {
        log_error(LOG_GUI, "FolderView: Failed to create ListBrowser node\n");
    }
    
    return TRUE;
}

/*------------------------------------------------------------------------*/
/* Public Entry Points                                                    */
/*------------------------------------------------------------------------*/

void free_folder_entries(struct iTidyFolderViewWindow *folder_data)
{
    if (!folder_data)
        return;
    
    /* Free ListBrowser nodes */
    if (folder_data->folder_list)
    {
        FreeListBrowserList(folder_data->folder_list);
        NewList(folder_data->folder_list);  /* Reinitialize for reuse */
    }
    
    /* Free tracked node data */
    free_tracked_node_data(folder_data);
}
