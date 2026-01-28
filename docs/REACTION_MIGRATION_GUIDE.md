# iTidy ReAction Migration Guide

This document outlines the standard process and common pitfalls when migrating legacy GadTools windows to the ReAction GUI system in iTidy. Following this guide will prevent compilation errors, linker collisions, and runtime issues.

## 1. The Migration Strategy (3-Stage Process)

To ensure stability, do not modify the existing window file in place. Instead, use a parallel development approach.

### Stage 1: Parallel Creation
1. Create new files `src/GUI/window_name_reaction.c` and `.h`.
2. Implement the ReAction Object tree (Layout, Window, Gadgets).
3. Implement the library base isolation pattern (see below).
4. Add the new file to the `Makefile`.
5. Verify it compiles in isolation.

### Stage 2: Logic & Event Loop
1. Implement the `handle_events` loop using `RA_HandleInput`.
2. Implement data mapping functions:
   - `prefs_to_gadgets()`: Set gadget states from `LayoutPreferences`.
   - `save_gadgets_to_prefs()`: Read gadget states back to `LayoutPreferences`.
3. Verify event handling logic.

### Stage 3: Integration & Swap
1. Update `src/GUI/main_window.c` to call the new `open_...` function.
2. Verify the new window works in the full application context.
3. Rename files:
   - `window_name.c` -> `window_name_gadtools.c.bak`
   - `window_name_reaction.c` -> `window_name.c`
4. Update `Makefile` to point to the renamed file.
5. Clean up the old `.bak` files once confirmed stable.

---

## 2. Critical Technical Patterns

### A. Library Base Isolation (Preventing Linker Errors)

**The Issue:**
AmigaOS system headers declare library bases (like `WindowBase`) as `extern`. Since `main_window.c` already defines these globals, defining them again in your new window file causes "Multiple Definition" linker errors.

**The Solution:**
Use preprocessor macros *before* including system proto headers to redirect the symbols to local, unique variable names.

```c
/* src/GUI/my_window_reaction.c */

/* 1. Redefine bases to local unique names BEFORE includes */
#define WindowBase iTidy_Adv_WindowBase
#define LayoutBase iTidy_Adv_LayoutBase
#define ChooserBase iTidy_Adv_ChooserBase
/* ... etc ... */

/* 2. Include Headers */
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/chooser.h>
/* ... */

/* 3. Declare the local variables */
struct Library *iTidy_Adv_WindowBase = NULL;
struct Library *iTidy_Adv_LayoutBase = NULL;
struct Library *iTidy_Adv_ChooserBase = NULL;

/* 4. Open/Close them locally in your open/close functions */
```

### B. Chooser Gadgets (Dropdowns)

**The Issue:**
ReAction Choosers are strictly BOOPSI objects. Passing simple string arrays to `CHOOSER_Labels` often results in empty dropdowns or crashes if the list isn't formatted correctly as a generic Amiga `struct List` of formatting nodes.

**The Solution:**
Always use `AllocChooserNode` to build a linked list for `CHOOSER_Labels`.

```c
/* Helper to create a list from a string array */
static struct List *make_chooser_list(STRPTR *labels)
{
    struct List *list = (struct List *)whd_malloc(sizeof(struct List));
    if (list)
    {
        NewList(list);
        while (*labels)
        {
            /* VALIDATION: Use AllocChooserNode, not manual Node allocation */
            struct Node *node = AllocChooserNode(CNA_Text, *labels, TAG_END);
            if (node)
            {
                AddTail(list, node);
            }
            labels++;
        }
    }
    return list;
}

/* Helper to free the list */
static void free_chooser_list(struct List *list)
{
    if (list)
    {
        struct Node *node, *next;
        node = list->lh_Head;
        while ((next = node->ln_Succ))
        {
            FreeChooserNode(node); /* Use FreeChooserNode */
            node = next;
        }
        whd_free(list);
    }
}

### C. GetFile Gadgets (File/Folder Selection)

**The Issue:**
The `getfile.gadget` does *not* open the requester automatically when clicked, and using the wrong attributes causes confusion between File and Drawer modes.

**The Solution:**
1.  **Creation**: Use `GETFILE_Drawer` for folders, `GETFILE_FullFile` for files.
2.  **Event Loop**: You *must* manually invoke the requester method `GFILE_REQUEST` when the gadget is clicked.
3.  **Method Call**: Use `DoMethod`, NOT `DoGadgetMethod` (which crashes in ReAction).

```c
/* Creation (Folders) */
LAYOUT_AddChild, NewObject(GETFILE_GetClass(), NULL,
    GA_ID, GID_MY_FOLDER,
    GA_RelVerify, TRUE,         /* Critical for event */
    GETFILE_Drawer, current_path,
    GETFILE_DrawersOnly, TRUE,  /* Folders only */
    GETFILE_ReadOnly, TRUE,     /* Button mode */
TAG_END),

/* Event Handler */
case GID_MY_FOLDER:
{
    /* Manually open requester */
    ULONG res = DoMethod((Object *)gadgets[GID_MY_FOLDER], GFILE_REQUEST, window);
    if (res)
    {
        /* Success - get result */
        STRPTR new_path = NULL;
        GetAttr(GETFILE_Drawer, gadgets[GID_MY_FOLDER], (ULONG *)&new_path);
        if (new_path)
        {
             /* Copy securely to your internal buffer */
        }
    }
    break;
}
```

### D. Required Macros

**The Issue:**
Compiler errors about implicit declaration of `RA_OpenWindow` or `RA_HandleInput`.

**The Solution:**
You must include the reaction macros header, which is not included by default:

```c
#include <clib/alib_protos.h>         /* Critical for BOOPSI functions */
#include <reaction/reaction_macros.h> /* Required for RA_ macros */
```

### E. Cleanup Order (Preventing Crashes)

**The Issue:**
ReAction windows often crash on close because resources are freed in the wrong order.

**The Solution:**
Strictly follow REVERSE creation order. Note that `DisposeObject(window_obj)` automatically frees all child gadgets attached to it.

1.  **Detach/Clear**: `ClearMenuStrip(win)` if menus are attached.
2.  **Dispose Window**: `DisposeObject(window_obj)`. Do NOT use `CloseWindow()`.
3.  **Free Dependents**: Free VisualInfo, Menus, Ports.
4.  **Free Lists**: Free Chooser label lists (`whd_free`/`FreeChooserNode`) last, as the Window might have still been referencing them if you freed them earlier.

---

## 3. Checklist for Future Agents

1.  [ ] **Include Guards**: check headers for unique include guards.
2.  [ ] **Memory Tracking**: Use `whd_malloc` / `whd_free` for internal structures.
3.  [ ] **Logging**: Add a `log_info` at the start of the open function to debug input values.
4.  [ ] **GadgetID Enums**: Use an enum or `#define` for Gadget IDs to prevent magic numbers.
5.  [ ] **Clean Cleanup**: Ensure all lists (Labels) and Objects are freed in the close function.
