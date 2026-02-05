# DefIcons Type Selection — Implementation Plan

**Date:** February 5, 2026  
**Feature:** Dynamic Hierarchical Type Selection GUI  
**Target:** iTidy v2.0 (ReAction)

---

## Overview

Implement a DefIcons type selection window that dynamically builds a hierarchical tree from `ENV:Sys/deficons.prefs`, allowing users to enable/disable default icon creation by category. The implementation leverages existing test code from `Tests/DefIcons/` and proven ReAction patterns from iTidy's folder browser window.

### Key Requirements

- **Dynamic type loading** - Read DefIcons type hierarchy from system prefs (no hardcoded lists)
- **Hierarchical display** - Show parent/child relationships with expandable tree nodes
- **User selection** - Enable/disable icon creation by category with checkboxes
- **Preference persistence** - Save selections as bitmask in `LayoutPreferences`
- **Future-proof** - Automatically supports new types added by Workbench updates

### User Scenario

**Example:** User wants to disable icon creation for "tool" (executables) and "font" types:
1. Open "DefIcons Settings..." from Project menu
2. See hierarchical tree of all DefIcons types from system
3. Uncheck "Tools & Executables" category
4. Uncheck "Fonts" category
5. Click OK → preferences saved
6. When tidying folders, iTidy skips creating icons for tools and fonts

---

## GUI Design: Hierarchical ListBrowser Tree

Note:  this will be creation using the standard ReAction gadgets that ship with workbench 3.2.3.

### Visual Layout

```
┌─ DefIcons: Icon Type Selection ─────────────────────┐
│                                                      │
│ Select which file types should get default icons:   │
│                                                      │
│ ┌────────────────────────────────────────────────┐  │
│ │ ▼ Pictures                              [✓]   │  │
│ │   └─ includes: gif, jpeg, png, tiff...         │  │
│ │ ▼ Music & Audio                         [✓]   │  │
│ │   ├─ Music (mod, s3m, xm, midi...)      [✓]   │  │
│ │   └─ Sound (wav, mp3, aiff...)          [✓]   │  │
│ │ ▶ Documents                             [✓]   │  │
│ │ ▶ Archives                              [✓]   │  │
│ │ ▶ Source Code (c, asm, rexx...)         [✓]   │  │
│ │ ▶ Tools & Executables                   [ ]   │  │
│ │ ▶ Fonts                                 [ ]   │  │
│ │ ▶ Video                                 [✓]   │  │
│ └────────────────────────────────────────────────┘  │
│                                                      │
│ Folder Icons: ◉ Smart  ○ Always  ○ Never            │
│                                                      │
│          [Select All]  [Select None]                 │
│                                                      │
│                [OK]           [Cancel]               │
└──────────────────────────────────────────────────────┘
```

### ReAction Components

| Component | Purpose |
|-----------|---------|
| `window.class` | Window container |
| `layout.gadget` | Vertical layout management |
| `listbrowser.gadget` | Hierarchical tree display with checkboxes |
| `chooser.gadget` | Folder icon creation mode selector |
| `button.gadget` | OK, Cancel, Select All, Select None |
| `label.image` | Section labels |

---

## Implementation Steps

### Step 1: Port DefIcons Parser from Tests to Production

**Goal:** Move type hierarchy parser from test code into production modules.

#### Files to Create:

**`src/deficons_parser.h`**
```c
#ifndef DEFICONS_PARSER_H
#define DEFICONS_PARSER_H

#include <exec/types.h>

/* Maximum type name length */
#define MAX_DEFICONS_TYPE_NAME 64

/* DefIcons type tree node */
typedef struct DeficonTypeTreeNode {
    char type_name[MAX_DEFICONS_TYPE_NAME];  /* Type token (e.g., "music", "mod") */
    int generation;                          /* Tree depth: 0=root, 1=child, 2=grandchild */
    BOOL has_children;                       /* TRUE if branch node */
    BOOL enabled;                            /* User selection state (generation 0 only) */
    int parent_index;                        /* Index to parent node (-1 if root) */
} DeficonTypeTreeNode;

/* Parse deficons.prefs and build type tree */
BOOL parse_deficons_prefs(DeficonTypeTreeNode **tree_out, int *count_out);

/* Free type tree */
void free_deficons_type_tree(DeficonTypeTreeNode *tree);

/* Get parent type name */
const char* get_parent_type_name(const DeficonTypeTreeNode *tree, int count, const char *type_name);

#endif
```

**`src/deficons_parser.c`**
- Port `parse_deficons_prefs()` from `Tests/DefIcons/Test2/deficons_creator.c` (lines ~50-150)
- Port binary parser logic for `ENV:Sys/deficons.prefs`
- Build `DeficonTypeTreeNode` array with parent/child relationships
- Calculate `generation` level (0=root, 1=child, 2=grandchild, etc.)
- Set `has_children` flag for branch nodes

**Source Material:**
- **`Tests/DefIcons/Test2/deficons_creator.c`** - Binary parser implementation
- **`Tests/DefIcons/deficontree.c`** - Tree building and traversal

#### Key Functions to Port:

```c
/* From deficons_creator.c */
static BOOL parse_deficons_prefs(void) {
    // Open ENV:Sys/deficons.prefs
    // Read binary format:
    //   - Each entry: 4-byte type_id + 4-byte parent_id + null-terminated name
    // Build parent-child index mapping
    // Calculate generation levels
    return TRUE;
}

/* From deficontree.c */
static void calculate_generation_levels(TypeNode *nodes, int count) {
    // Recursively walk tree from roots (parent_index == -1)
    // Set generation = 0 for roots
    // Set generation = parent_generation + 1 for children
}

static void set_has_children_flags(TypeNode *nodes, int count) {
    // For each node, check if any other node has parent_index == this_index
    // If yes, set has_children = TRUE
}
```

---

### Step 2: Add DefIcons Preferences to LayoutPreferences

**Goal:** Extend global preferences structure to store DefIcons settings.

#### File to Modify: `src/layout_preferences.h`

**Location:** Around line 200 (after Beta settings section)

**Add New Section:**
```c
typedef struct {
    /* ... existing fields ... */
    
    /* Beta/Experimental Features */
    BOOL beta_openFoldersAfterProcessing;
    BOOL beta_FindWindowOnWorkbenchAndUpdate;
    
    /* DefIcons Icon Creation Settings */
    BOOL enable_deficons_icon_creation;       /* Master enable/disable */
    char deficons_disabled_types[256];        /* Comma-separated list of disabled root type names */
    UWORD deficons_folder_icon_mode;          /* 0=Smart, 1=Always, 2=Never */
    BOOL deficons_skip_system_assigns;        /* TRUE = skip SYS:, C:, S:, DEVS:, etc. */
    
    /* ... rest of structure ... */
} LayoutPreferences;
```

**String-Based Type Selection (Future-Proof):**

Instead of a fixed bitmask, store a **comma-separated list of disabled root type names**. This approach:
- ✅ Automatically supports new root types added by Workbench updates
- ✅ No need to update iTidy when DefIcons adds categories
- ✅ Human-readable in preferences file
- ✅ Default = empty string (all enabled)

**Example Storage:**
```c
/* User disabled "tool" and "font" categories */
prefs->deficons_disabled_types = "tool,font";

/* All enabled (empty string) */
prefs->deficons_disabled_types = "";

/* Only project files disabled */
prefs->deficons_disabled_types = "project";
```

**Helper Functions:**
```c
/* Check if a root type is enabled */
BOOL is_deficon_type_enabled(const char *type_name, const char *disabled_list);

/* Add type to disabled list */
void add_disabled_type(char *disabled_list, size_t max_len, const char *type_name);

/* Remove type from disabled list */
void remove_disabled_type(char *disabled_list, const char *type_name);
```

---

### Step 1b: Cache DefIcons Tree on Startup (Performance Optimization)

**Goal:** Parse `deficons.prefs` once during iTidy initialization and reuse the cached tree.

#### Rationale:
- ⚡ Settings window opens instantly (no disk I/O)
- 🔄 Consistent tree across all operations
- 💾 Only one parse per iTidy session

#### File to Modify: `src/main_gui.c`

**Add Global Cache Variables:**
```c
/* DefIcons type tree cache (parsed once on startup) */
static DeficonTypeTreeNode *g_cached_deficons_tree = NULL;
static int g_cached_deficons_count = 0;

BOOL initialize_deficons_cache(void)
{
    if (g_cached_deficons_tree) return TRUE;  /* Already cached */
    
    if (!parse_deficons_prefs(&g_cached_deficons_tree, &g_cached_deficons_count)) {
        log_warning(LOG_GENERAL, "DefIcons cache initialization failed (DefIcons may not be installed)\n");
        return FALSE;
    }
    
    log_info(LOG_GENERAL, "DefIcons cache initialized: %d types loaded\n", g_cached_deficons_count);
    return TRUE;
}

void cleanup_deficons_cache(void)
{
    if (g_cached_deficons_tree) {
        free_deficons_type_tree(g_cached_deficons_tree);
        g_cached_deficons_tree = NULL;
        g_cached_deficons_count = 0;
        log_info(LOG_GENERAL, "DefIcons cache cleaned up\n");
    }
}
```

**Call from main():**
```c
int main(int argc, char *argv[])
{
    /* ... existing initialization ... */
    
    /* Initialize DefIcons cache (non-fatal if fails) */
    initialize_deficons_cache();
    
    /* ... run main loop ... */
    
    /* Cleanup on exit */
    cleanup_deficons_cache();
    
    return 0;
}
```

**Note:** The settings window will reference `extern DeficonTypeTreeNode *g_cached_deficons_tree;` instead of re-parsing.

---

#### File to Modify: `src/layout_preferences.c`

**Function:** `InitLayoutPreferences()` (around line 50-100)

**Add Default Initialization:**
```c
void InitLayoutPreferences(LayoutPreferences *prefs)
{
    /* ... existing initialization ... */
    
    /* DefIcons defaults */
    prefs->enable_deficons_icon_creation = FALSE;  /* Start disabled until ready */
    strcpy(prefs->deficons_disabled_types, "tool,font");  /* Disable tools and fonts by default */
    prefs->deficons_folder_icon_mode = 0;  /* 0=Smart (only if no .info exists) */
    prefs->deficons_skip_system_assigns = TRUE;
    
    /* ... rest of function ... */
}
```

---

### Step 3: Create Hierarchical ListBrowser Settings Window

**Goal:** Implement GUI window for DefIcons type selection using hierarchical ListBrowser.

#### Files to Create:

**`src/GUI/deficons_settings_window.h`**
```c
#ifndef DEFICONS_SETTINGS_WINDOW_H
#define DEFICONS_SETTINGS_WINDOW_H

#include "layout_preferences.h"

/* Open DefIcons type selection window (modal) */
BOOL open_itidy_deficons_settings_window(LayoutPreferences *prefs);

#endif
```

**`src/GUI/deficons_settings_window.c`**

**Template:** Use `Tests/ReActon/listbrowser_tree.c` as reference (complete working example)

**Key Implementation Details:**

```c
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/listbrowser.h>
#include <proto/button.h>
#include <proto/chooser.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <gadgets/button.h>
#include <gadgets/chooser.h>

#include "deficons_settings_window.h"
#include "../deficons_parser.h"
#include "../writeLog.h"

/* Redefine library bases to avoid symbol collisions */
#define WindowBase iTidy_DefIcons_WindowBase
#define LayoutBase iTidy_DefIcons_LayoutBase
#define ButtonBase iTidy_DefIcons_ButtonBase
#define ListBrowserBase iTidy_DefIcons_ListBrowserBase
#define ChooserBase iTidy_DefIcons_ChooserBase

struct Library *iTidy_DefIcons_WindowBase = NULL;
struct Library *iTidy_DefIcons_LayoutBase = NULL;
struct Library *iTidy_DefIcons_ButtonBase = NULL;
struct Library *iTidy_DefIcons_ListBrowserBase = NULL;
struct Library *iTidy_DefIcons_ChooserBase = NULL;

/* Gadget IDs */
enum {
    GID_DEFICONS_LIST = 1,
    GID_SELECT_ALL,
    GID_SELECT_NONE,
    GID_FOLDER_MODE,
    GID_OK,
    GID_CANCEL
};

/* Build ListBrowser nodes from DefIcons type tree */
static struct List* create_deficons_listbrowser_nodes(
    DeficonTypeTreeNode *tree, 
    int count, 
    const char *disabled_types)
{
    struct List *node_list;
    int i;
    
    /* Allocate list structure */
    node_list = (struct List *)AllocVec(sizeof(struct List), MEMF_CLEAR);
    if (!node_list) return NULL;
    
    NewList(node_list);
    
    /* Create nodes for each type */
    for (i = 0; i < count; i++)
    {
        ULONG flags = 0;
        char display_text[128];
        
        /* Generation 0 = root categories (selectable with checkbox) */
        if (tree[i].generation == 0) {
            /* Check if this root type is disabled */
            BOOL is_enabled = !is_type_in_disabled_list(tree[i].type_name, disabled_types);
            
            /* Format with checkbox indicator */
            snprintf(display_text, sizeof(display_text), "%s %s", 
                     is_enabled ? "[✓]" : "[ ]", tree[i].type_name);
        }
        /* Generation > 0 = child types (display-only, no checkbox) */
        else {
            /* Format as "includes: type1, type2..." hint */
            snprintf(display_text, sizeof(display_text), "  └─ includes: %s", 
                     tree[i].type_name);
        }
        
        /* Set flags */
        if (tree[i].has_children) {
            flags |= LBFLG_HASCHILDREN | LBFLG_HIDDEN;  /* Hidden initially */
        }
        
        /* Create node */
        struct Node *node = AllocListBrowserNode(1,
            LBNCA_Text, display_text,
            LBNA_Generation, tree[i].generation,
            LBNA_Flags, flags,
            LBNA_UserData, (APTR)(ULONG)i,  /* Store tree index */
            TAG_DONE);
        
        if (node) {
            AddTail(node_list, node);
        }
    }
    
    /* CRITICAL: Hide all children before creating gadget */
    HideAllListBrowserChildren(node_list);
    
    return node_list;
}

/* Helper: Check if type name is in disabled list */
static BOOL is_type_in_disabled_list(const char *type_name, const char *disabled_list)
{
    char temp[256];
    char *token;
    
    if (!disabled_list || disabled_list[0] == '\0') return FALSE;
    
    strncpy(temp, disabled_list, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    
    token = strtok(temp, ",");
    while (token) {
        /* Trim whitespace */
        while (*token == ' ') token++;
        
        if (strcmp(token, type_name) == 0) {
            return TRUE;
        }
        token = strtok(NULL, ",");
    }
    
    return FALSE;
}

/* Open DefIcons settings window */
BOOL open_itidy_deficons_settings_window(LayoutPreferences *prefs)
{
    struct Screen *screen;
    Object *window_obj = NULL;
    Object *layout_obj = NULL;
    Object *listbrowser_obj = NULL;
    Object *select_all_btn = NULL;
    Object *select_none_btn = NULL;
    Object *folder_mode_chooser = NULL;
    Object *ok_btn = NULL;
    Object *cancel_btn = NULL;
    
    /* Use cached tree from startup (populated by main_gui.c) */
    extern DeficonTypeTreeNode *g_cached_deficons_tree;
    extern int g_cached_deficons_count;
    
    struct List *node_list = NULL;
    char disabled_types[256];  /* Working copy of disabled list */
    
    BOOL result = FALSE;
    BOOL done = FALSE;
    
    if (!prefs) return FALSE;
    
    /* Verify cached tree is available */
    if (!g_cached_deficons_tree || g_cached_deficons_count == 0) {
        log_error(LOG_GUI, "DefIcons type tree not cached\n");
        show_error_requester(
            "DefIcons Configuration Not Found",
            "Could not read ENV:Sys/deficons.prefs\n"
            "\n"
            "DefIcons requires Workbench 3.2+ with DefIcons installed.\n"
            "\n"
            "To fix:\n"
            "1. Verify DefIcons is installed in SYS:Prefs/DefIcons\n"
            "2. Run DefIcons editor to create preferences\n"
            "3. Reboot to populate ENV: variables\n"
            "4. Restart iTidy");
        return FALSE;
    }
    
    /* Copy current disabled types to working buffer */
    strncpy(disabled_types, prefs->deficons_disabled_types, sizeof(disabled_types) - 1);
    disabled_types[sizeof(disabled_types) - 1] = '\0';
    
    /* Open ReAction classes */
    if (!open_reaction_classes()) {
        return FALSE;
    }
    
    /* Create ListBrowser nodes from cached tree */
    node_list = create_deficons_listbrowser_nodes(
        g_cached_deficons_tree, g_cached_deficons_count, disabled_types);
    
    if (!node_list) {
        close_reaction_classes();
        return FALSE;
    }
    
    /* Create gadgets */
    listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
        GA_ID, GID_DEFICONS_LIST,
        LISTBROWSER_Labels, node_list,
        LISTBROWSER_Hierarchical, TRUE,  /* Enable tree mode */
        LISTBROWSER_ShowSelected, TRUE,
        TAG_DONE);
    
    /* ... create other gadgets ... */
    
    /* Create layout */
    layout_obj = NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_AddChild, listbrowser_obj,
        /* ... add other children ... */
        TAG_DONE);
    
    /* Create window */
    window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title, "DefIcons: Icon Type Selection",
        WA_Width, 500,
        WA_Height, 400,
        WA_DepthGadget, TRUE,
        WA_DragBar, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_Layout, layout_obj,
        TAG_DONE);
    
    if (!window_obj) {
        /* Cleanup and return */
        return FALSE;
    }
    
    /* Open window */
    struct Window *window = (struct Window *)RA_OpenWindow(window_obj);
    if (!window) {
        DisposeObject(window_obj);
        return FALSE;
    }
    
    /* Event loop */
    ULONG signal_mask, signals;
    ULONG wmhi_result;
    UWORD code;
    
    GetAttr(WINDOW_SigMask, window_obj, &signal_mask);
    
    while (!done) {
        signals = Wait(signal_mask | SIGBREAKF_CTRL_C);
        
        if (signals & SIGBREAKF_CTRL_C) {
            done = TRUE;
            continue;
        }
        
        while ((wmhi_result = RA_HandleInput(window_obj, &code)) != WMHI_LASTMSG)
        {
            switch (wmhi_result & WMHI_CLASSMASK)
            {
                case WMHI_CLOSEWINDOW:
                    done = TRUE;
                    break;
                    
                case WMHI_GADGETUP:
                    switch (wmhi_result & WMHI_GADGETMASK)
                    {
                        case GID_DEFICONS_LIST:
                            /* Handle tree node click - only toggle generation 0 (root) nodes */
                            /* Children (generation > 0) do nothing on click */
                            handle_listbrowser_click(listbrowser_obj, node_list, type_tree, disabled_types);
                            break;
                            
                        case GID_SELECT_ALL:
                            /* Enable all categories (empty disabled list) */
                            disabled_types[0] = '\0';
                            refresh_listbrowser(listbrowser_obj, node_list, type_tree, type_count, disabled_types);
                            break;
                            
                        case GID_SELECT_NONE:
                            /* Disable all categories (add all root types to disabled list) */
                            build_all_disabled_types_string(type_tree, type_count, disabled_types, sizeof(disabled_types));
                            refresh_listbrowser(listbrowser_obj, node_list, type_tree, type_count, disabled_types);
                            break;
                            
                        case GID_OK:
                            /* Save preferences */
                            result = TRUE;
                            done = TRUE;
                            break;
                            
                        case GID_CANCEL:
                            done = TRUE;
                            break;
                    }
                    break;
            }
        }
    }
    
    /* Cleanup */
    if (window_obj) {
        DisposeObject(window_obj);  /* Disposes all children */
    }
    
    if (node_list) {
        FreeListBrowserList(node_list);
        FreeVec(node_list);
    }
    
    free_deficons_type_tree(type_tree);
    close_reaction_classes();
    
    return result;
}
```

**Critical Pattern from listbrowser_tree.c:**
```c
/* MUST call this before creating ListBrowser gadget! */
void HideAllListBrowserChildren(struct List *list)
{
    struct Node *node;
    ULONG flags;
    
    for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        GetListBrowserNodeAttrs(node, LBNA_Generation, &generation, TAG_DONE);
        
        /* Hide all non-root nodes */
        if (generation > 0) {
            GetListBrowserNodeAttrs(node, LBNA_Flags, &flags, TAG_DONE);
            flags |= LBFLG_HIDDEN;
            SetListBrowserNodeAttrs(node, LBNA_Flags, flags, TAG_DONE);
        }
    }
}
```

---

### Step 4: Integrate Window into Main GUI

**Goal:** Add menu item to launch DefIcons settings window.

#### File to Modify: `src/GUI/main_window_reaction.h`

**Add Menu Item Constant:**
```c
/* Menu item IDs */
#define MENU_PROJECT_NEW       1001
#define MENU_PROJECT_OPEN      1002
#define MENU_PROJECT_SAVE      1003
#define MENU_PROJECT_SAVE_AS   1004
#define MENU_PROJECT_DEFICONS  1007  /* NEW */
#define MENU_PROJECT_ABOUT     1005
#define MENU_PROJECT_CLOSE     1006
```

#### File to Modify: `src/GUI/main_window.c`

**Location 1:** Menu template definition (around line 1200)

**Add Menu Item:**
```c
static struct NewMenu main_window_menu_template[] = {
    { NM_TITLE, "Project",           NULL, 0, 0, NULL },
    { NM_ITEM,  "New",               "N",  0, 0, (APTR)MENU_PROJECT_NEW },
    { NM_ITEM,  "Open...",           "O",  0, 0, (APTR)MENU_PROJECT_OPEN },
    { NM_ITEM,  NM_BARLABEL,         NULL, 0, 0, NULL },
    { NM_ITEM,  "Save",              "S",  0, 0, (APTR)MENU_PROJECT_SAVE },
    { NM_ITEM,  "Save As...",        "A",  0, 0, (APTR)MENU_PROJECT_SAVE_AS },
    { NM_ITEM,  NM_BARLABEL,         NULL, 0, 0, NULL },
    { NM_ITEM,  "DefIcons Settings...", "D", 0, 0, (APTR)MENU_PROJECT_DEFICONS },  /* NEW */
    { NM_ITEM,  NM_BARLABEL,         NULL, 0, 0, NULL },
    { NM_ITEM,  "About",             "?",  0, 0, (APTR)MENU_PROJECT_ABOUT },
    { NM_ITEM,  "Quit",              "Q",  0, 0, (APTR)MENU_PROJECT_CLOSE },
    { NM_END, NULL, NULL, 0, 0, NULL }
};
```

**Location 2:** Menu handler (around line 1439)

**Add Handler Case:**
```c
case WMHI_MENUPICK:
    menu_item = (UWORD)(result & WMHI_MENUMASK);
    switch (menu_item)
    {
        case MENU_PROJECT_NEW:
            /* Handle New */
            break;
            
        case MENU_PROJECT_OPEN:
            /* Handle Open */
            break;
            
        case MENU_PROJECT_DEFICONS:
            /* Handle DefIcons Settings */
            if (open_itidy_deficons_settings_window(&g_prefs)) {
                /* Preferences changed - update global */
                UpdateGlobalPreferences(&g_prefs);
                log_info(LOG_GUI, "DefIcons preferences updated\n");
            }
            break;
            
        case MENU_PROJECT_ABOUT:
            /* Handle About */
            break;
            
        /* ... other cases ... */
    }
    break;
```

**Location 3:** Add include at top of file:
```c
#include "GUI/deficons_settings_window.h"
```

---

### Step 5: Add UI Controls and Persistence

**Goal:** Implement convenience controls and preference saving.

#### Controls to Add:

**1. Select All / Select None Buttons**
```c
/* Button layout (horizontal) */
Object *button_row = NewObject(LAYOUT_GetClass(), NULL,
    LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
    LAYOUT_AddChild, NewObject(BUTTON_GetClass(), NULL,
        GA_ID, GID_SELECT_ALL,
        GA_Text, "Select All",
        GA_RelVerify, TRUE,
        TAG_DONE),
    LAYOUT_AddChild, NewObject(BUTTON_GetClass(), NULL,
        GA_ID, GID_SELECT_NONE,
        GA_Text, "Select None",
        GA_RelVerify, TRUE,
        TAG_DONE),
    TAG_DONE);
```

**2. Folder Icon Creation Mode Chooser**
```c
/* Chooser labels */
static STRPTR folder_mode_labels[] = {
    "Smart (only if no .info exists)",
    "Always (even if .info exists)",
    "Never (skip folders)",
    NULL
};

/* Create chooser */
Object *folder_mode = NewObject(CHOOSER_GetClass(), NULL,
    GA_ID, GID_FOLDER_MODE,
    CHOOSER_Labels, folder_mode_labels,
    CHOOSER_Selected, prefs->deficons_folder_icon_mode,  /* 0, 1, or 2 */
    TAG_DONE);
```

**3. Preference Persistence (On OK Button)**
```c
case GID_OK:
{
    /* disabled_types string was updated during user interaction */
    /* Just copy it to preferences */
    strncpy(prefs->deficons_disabled_types, disabled_types, sizeof(prefs->deficons_disabled_types) - 1);
    prefs->deficons_disabled_types[sizeof(prefs->deficons_disabled_types) - 1] = '\0';
    
    /* Get folder mode from chooser (0=Smart, 1=Always, 2=Never) */
    ULONG folder_mode_selected;
    GetAttr(CHOOSER_Selected, folder_mode_chooser, &folder_mode_selected);
    prefs->deficons_folder_icon_mode = (UWORD)folder_mode_selected;
    
    result = TRUE;
    done = TRUE;
    break;
}
```

---

### Step 6: Update Makefile and Test

**Goal:** Add new files to build system and verify functionality.

#### File to Modify: `Makefile`

**Add Source Files:**
```makefile
SOURCES = \
    src/main_gui.c \
    src/icon_management.c \
    src/window_management.c \
    src/file_directory_handling.c \
    src/writeLog.c \
    src/utilities.c \
    src/deficons_parser.c \
    src/GUI/deficons_settings_window.c \
    # ... other sources ...
```

**Add Library Dependencies (if not already present):**
```makefile
LIBS = \
    -lwindow \
    -llayout \
    -lbutton \
    -llistbrowser \
    -lcheckbox \
    -lchooser \
    # ... other libs ...
```

#### Testing Checklist:

1. **Compile Test:**
   ```powershell
   make clean && make
   ```
   - Verify no compile errors
   - Check `build_output_latest.txt` for warnings

2. **Window Open Test:**
   - Launch iTidy
   - Open "Project → DefIcons Settings..."
   - Verify window opens without crash
   - Verify title and gadgets visible

3. **Dynamic Loading Test:**
   - Verify tree loads from `ENV:Sys/deficons.prefs`
   - Check log for parse errors: `Bin/Amiga/logs/gui_*.log`
   - Verify all 125+ types appear (or whatever system has)
   - Verify parent/child hierarchy correct

4. **Interaction Test:**
   - Click disclosure triangles (▶) to expand/collapse
   - Click type nodes to toggle checkboxes
   - Verify checkbox state updates (☑ ↔ ☐)
   - Test "Select All" button
   - Test "Select None" button
   - Test folder mode chooser

5. **Persistence Test:**
   - Change selections
   - Click OK
   - Quit iTidy
   - Restart iTidy
   - Open DefIcons Settings again
   - Verify selections persisted

6. **Edge Cases:**
   - Test with missing `deficons.prefs` (should show error)
   - Test with corrupted `deficons.prefs` (should handle gracefully)
   - Test with minimal system (only a few types)

---

## Further Considerations

### 1. Checkbox Rendering Approach

**Three Options:**

**Option A: Text-Based Checkboxes (Simplest)**
- Use ListBrowser text column with "☑" / "☐" Unicode characters
- **Pros:** Dead simple, no extra gadgets
- **Cons:** Not native looking, keyboard nav limited

**Option B: Custom Image Rendering**
- Use `bevel.image` with custom checkbox images
- **Pros:** Professional appearance
- **Cons:** Requires image assets, more complex

**Option C: Embedded Checkbox Gadgets**
- Integrate actual `checkbox.gadget` instances per row
- **Pros:** Fully native, best accessibility
- **Cons:** Most complex, may have layout issues

**Recommendation:** Start with **Option A** (text-based) for MVP, upgrade to Option B/C later if needed.

---

### 2. Future-Proofing: String-Based vs Bitmask Approach ✅ RESOLVED

**Decision:** Use **string-based disabled type names** instead of fixed bitmask.

**Why String-Based Wins:**
- ✅ **Truly future-proof** - New root types added by Workbench updates appear automatically as enabled
- ✅ **No iTidy recompile needed** when DefIcons adds categories
- ✅ **Human-readable** in preferences files
- ✅ **Simpler logic** - "Is type in disabled list?" vs bit manipulation
- ✅ **Scales indefinitely** - No 32-bit/64-bit limit

**Storage Format:**
```c
char deficons_disabled_types[256]; /* "tool,font" */
```

**Default Behavior:**
- Empty string = all types enabled
- User disables "tool" and "font" → stored as `"tool,font"`
- If WB adds "newtype" category → appears enabled by default (not in disabled list)

---

### 3. Parent Node Selection Behavior ✅ IMPLEMENTED

**Decision:** Only parent nodes (generation 0) are selectable.

**Implementation:**
- ✅ **Only generation 0 nodes** get checkboxes: `[✓] Pictures`
- ✅ **Children nodes** display as informational text: `  └─ includes: gif, jpeg, png...`
- ✅ **Clicking a child** does nothing (or just expands/collapses parent)
- ✅ **Simpler state management** - no cascading logic needed
- ✅ **Matches user intent** - "I would deselect 'tool' and 'font'" (parent categories)

---

### 4. Fallback if deficons.prefs Missing

**Handling Missing/Corrupted Prefs:**

**Strategy 1: Error and Abort (Recommended)**
```c
if (!parse_deficons_prefs(&tree, &count)) {
    show_error_requester(
        "DefIcons Configuration Not Found",
        "Could not read ENV:Sys/deficons.prefs\n"
        "\n"
        "DefIcons requires Workbench 3.2+ with DefIcons installed.\n"
        "\n"
        "To fix:\n"
        "1. Verify DefIcons is installed in SYS:Prefs/DefIcons\n"
        "2. Run DefIcons editor to create preferences\n"
        "3. Reboot to populate ENV: variables\n"
        "4. Try again");
    return FALSE;
}
```

**Strategy 2: Hardcoded Fallback Categories**
```c
static DeficonTypeTreeNode fallback_tree[] = {
    { "project",   0, TRUE,  TRUE, -1, DEFICONS_CAT_PROJECT },
    { "picture",   0, FALSE, TRUE, -1, DEFICONS_CAT_PICTURES },
    { "music",     0, FALSE, TRUE, -1, DEFICONS_CAT_MUSIC },
    { "archive",   0, FALSE, TRUE, -1, DEFICONS_CAT_ARCHIVES },
    { "tool",      0, FALSE, TRUE, -1, DEFICONS_CAT_TOOLS },
    /* ... minimal set ... */
};

if (!parse_deficons_prefs(&tree, &count)) {
    log_warning(LOG_GUI, "Using fallback type list\n");
    tree = fallback_tree;
    count = sizeof(fallback_tree) / sizeof(fallback_tree[0]);
}
```

**Recommendation:** Use **Strategy 1** for v1 (error and abort). DefIcons is a WB 3.2+ feature, so missing prefs indicates system misconfiguration that should be fixed, not worked around.

---

## Reference Files

### Code References:
- **Hierarchical ListBrowser Example:** `Tests/ReActon/listbrowser_tree.c`
- **DefIcons Parser:** `Tests/DefIcons/Test2/deficons_creator.c`
- **Tree Builder:** `Tests/DefIcons/deficontree.c`
- **Folder Browser (working tree):** `src/GUI/itidy_folder_browser_window.c`
- **Beta Options Window (template):** `src/GUI/beta_options_window.c`

### Documentation:
- **DefIcons Flow:** `docs/iTidy_DefIcons_Icon_Creation_Flow.md`
- **ReAction Migration Guide:** `docs/REACTION_MIGRATION_GUIDE.md`
- **ListBrowser AutoDoc:** `docs/AutoDocs/listbrowser_gc.doc`
- **Window Class AutoDoc:** `docs/AutoDocs/window_cl.doc`
- **Type Dump:** `Tests/DefIcons/typedump.txt`

---

## Next Steps

1. **Create deficons_parser module** (port from test code)
2. **Extend LayoutPreferences structure** (add DefIcons settings)
3. **Create deficons_settings_window** (hierarchical ListBrowser GUI)
4. **Add menu integration** (Project → DefIcons Settings...)
5. **Test on Amiga** (WinUAE with WB 3.2)
6. **Document in user manual** (iTidy.guide section on DefIcons filtering)

---

**Implementation Status:** Planning Complete — Ready for Development

**Estimated Complexity:** Medium (3-5 days)
- Parser port: ~1 day
- Preferences integration: ~0.5 days
- GUI window implementation: ~2 days
- Menu integration: ~0.5 days
- Testing and refinement: ~1 day
