/*
**
** listbrowser_tree.c
**
** A simple ReAction Listbrowser Tree Example
**
** This example demonstrates how to create a hierarchical tree view using the
** ReAction listbrowser.gadget. It covers:
**  - Structuring data for a tree.
**  - Populating the listbrowser with hierarchical nodes.
**  - Setting node generation (indentation level).
**  - Using flags to define parent (branch) and child (leaf) nodes.
**  - Setting the initial state to be collapsed.
**
** Copyright 2024, K-P
** All Rights Reserved
**
** https://github.com/Kwezza/iTidy
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/window.h>
#include <proto/layout.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>

#include <reaction/reaction_macros.h>
#include <proto/listbrowser.h>

// Library bases
struct IntuitionBase *IntuitionBase = NULL;
struct Library *WindowBase = NULL;
struct Library *LayoutBase = NULL;
struct Library *ListBrowserBase = NULL;
struct UtilityBase *UtilityBase = NULL;

// Gadget objects
Object *win_obj = NULL;
Object *listbrowser_obj = NULL;

// --- AI AGENT DOCUMENTATION ---
// This structure defines the data for our tree.
// - 'name': The text displayed for the node.
// - 'generation': The indentation level of the node in the tree.
//   - 0 = top level (e.g., "AmigaOS")
//   - 1 = second level (e.g., "Workbench 3.x")
//   - 2 = third level (e.g., "Version 3.2")
// - 'has_children': A boolean indicating if this node is a branch (TRUE) or a leaf (FALSE).
//   This is used to set the LBFLG_HASCHILDREN flag.
struct TreeNodeData {
    char *name;
    int generation;
    BOOL has_children;
};

// The actual data for the tree, in the order it will appear when fully expanded.
struct TreeNodeData tree_data[] = {
    {"AmigaOS", 0, TRUE},
    {"Workbench 1.x", 1, FALSE},
    {"Workbench 2.x", 1, FALSE},
    {"Workbench 3.x", 1, TRUE},
    {"Version 3.1", 2, FALSE},
    {"Version 3.2", 2, TRUE},      // This is now a parent for the 3rd level
    {"Version 3.2.1", 3, FALSE},   // 3rd level node
    {"Version 3.2.2", 3, FALSE},   // 3rd level node
    {"Version 3.5/3.9", 2, FALSE},
    {"AmigaOS 4.x", 1, TRUE},
    {"Version 4.0", 2, FALSE},
    {"Version 4.1", 2, FALSE},
    {"MorphOS", 0, TRUE},
    {"MorphOS 1.x", 1, FALSE},
    {"MorphOS 2.x", 1, FALSE},
    {"MorphOS 3.x", 1, FALSE},
    {"AROS", 0, FALSE},
    {NULL, 0, FALSE}
};


void cleanup(int exit_code) {
    if (win_obj) {
        DisposeObject(win_obj);
    }

    if (ListBrowserBase) CloseLibrary(ListBrowserBase);
    if (LayoutBase) CloseLibrary(LayoutBase);
    if (WindowBase) CloseLibrary(WindowBase);
    if (UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

    exit(exit_code);
}

BOOL open_libs(void) {
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 47);
    if (!IntuitionBase) return FALSE;

    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 47);
    if (!UtilityBase) return FALSE;

    WindowBase = OpenLibrary("window.class", 47);
    if (!WindowBase) return FALSE;

    LayoutBase = OpenLibrary("gadgets/layout.gadget", 47);
    if (!LayoutBase) return FALSE;

    ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 47);
    if (!ListBrowserBase) return FALSE;

    return TRUE;
}

// --- AI AGENT DOCUMENTATION ---
// This function creates the list of nodes for the listbrowser.
// It iterates through the `tree_data` array and creates a BOOPSI node for each entry.
struct List *create_tree_nodes(void) {
    struct List *list = (struct List *)AllocVec(sizeof(struct List), MEMF_CLEAR);
    if (!list) return NULL;

    NewList(list);

    for (int i = 0; tree_data[i].name != NULL; i++) {
        ULONG flags = 0;
        // If the data indicates this node has children, it's a branch.
        // The LBFLG_HASCHILDREN flag tells the listbrowser to draw a disclosure triangle.
        if (tree_data[i].has_children) {
            flags |= LBFLG_HASCHILDREN;
        }
        // If it's a child node (generation > 0), it should initially be hidden.
        // The listbrowser gadget handles showing/hiding nodes when the user clicks the triangle.
        // We just need to set the initial state.
        else if (tree_data[i].generation > 0) {
            flags |= LBFLG_HIDDEN;
        }

        // Allocate a listbrowser node.
        struct Node *node = AllocListBrowserNode(1,
            // LBNCA_Text: The string to display.
            LBNCA_Text, tree_data[i].name,
            // LBNA_Generation: The indentation level.
            LBNA_Generation, tree_data[i].generation,
            // LBNA_Flags: Set the branch/leaf and hidden status.
            LBNA_Flags, flags,
            TAG_DONE);

        if (node) {
            AddTail(list, node);
        }
    }
    return list;
}

void event_loop(void) {
    ULONG wait_signals, sig_got;
    BOOL done = FALSE;
    ULONG result;
    UWORD code;

    GetAttr(WINDOW_SigMask, win_obj, &wait_signals);

    while (!done) {
        sig_got = Wait(wait_signals | SIGBREAKF_CTRL_C);

        if (sig_got & SIGBREAKF_CTRL_C) {
            done = TRUE;
        }

        while ((result = RA_HandleInput(win_obj, &code)) != WMHI_LASTMSG) {
            switch (result & WMHI_CLASSMASK) {
                case WMHI_CLOSEWINDOW:
                    done = TRUE;
                    break;
                case WMHI_GADGETUP:
                    // In a real app, you would handle gadget events here
                    break;
                case WMHI_ICONIFY:
                    if (win_obj) RA_Iconify(win_obj);
                    break;
                case WMHI_UNICONIFY:
                    RA_OpenWindow(win_obj);
                    break;
            }
        }
    }
}


int main(void) {
    if (!open_libs()) {
        puts("Failed to open libraries.");
        cleanup(20);
    }

    struct List *node_list = create_tree_nodes();
    if (!node_list) {
        puts("Failed to create node list.");
        cleanup(20);
    }

    // --- AI AGENT DOCUMENTATION ---
    // To make the tree appear collapsed initially, we first hide ALL children of every node.
    // The listbrowser gadget is smart enough to only show the top-level (generation 0)
    // nodes that have children, with their disclosure triangles ready to be clicked.
    HideAllListBrowserChildren(node_list);

    // Create the listbrowser gadget object.
    listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
        GA_ID, 1,
        GA_RelVerify, TRUE,
        // LISTBROWSER_Labels: The list of nodes to display.
        LISTBROWSER_Labels, node_list,
        // LISTBROWSER_Hierarchical: This is the crucial tag to enable tree view mode.
        LISTBROWSER_Hierarchical, TRUE,
        LISTBROWSER_ShowSelected, TRUE,
        TAG_DONE);

    if (!listbrowser_obj) {
        puts("Failed to create listbrowser object.");
        FreeListBrowserList((struct List *)node_list);
        FreeVec(node_list);
        cleanup(20);
    }

    win_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_ScreenTitle, "ReAction ListBrowser Tree Demo",
        WA_Title, "ListBrowser Tree",
        WA_DragBar, TRUE,
        WA_CloseGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_DepthGadget, TRUE,
        WA_Activate, TRUE,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_Layout, NewObject(LAYOUT_GetClass(), NULL,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_AddChild, listbrowser_obj,
            TAG_DONE),
        TAG_DONE);

    if (!win_obj) {
        puts("Failed to create window object.");
        FreeListBrowserList((struct List *)node_list);
        FreeVec(node_list);
        cleanup(20);
    }

    if (RA_OpenWindow(win_obj)) {
        event_loop();
    } else {
        puts("Failed to open window.");
    }

    // The window object disposal will also dispose the layout and listbrowser objects.
    // The list needs to be detached before freeing. This tells the gadget to stop
    // referencing the list data, preventing a crash when we free it.
    SetAttrs(listbrowser_obj, LISTBROWSER_Labels, ~0, TAG_DONE);
    // Free the list and all nodes allocated with AllocListBrowserNode().
    FreeListBrowserList(node_list);
    FreeVec(node_list);

    cleanup(0);
    return 0;
}