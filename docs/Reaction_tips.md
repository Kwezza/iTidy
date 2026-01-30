# ReAction Tips and Lessons Learned

This document captures hard-won lessons from implementing ReAction GUI components in iTidy. **AI Agents: Read this before implementing any ReAction code!**

---

## Table of Contents
1. [Event Loop Patterns](#event-loop-patterns)
2. [ListBrowser Hierarchical Tree View](#listbrowser-hierarchical-tree-view)
3. [ListBrowser Column Sorting](#listbrowser-column-sorting)
4. [ListBrowser Column Resizing](#listbrowser-column-resizing)
5. [Dynamic Text Labels](#dynamic-text-labels)
6. [GetFile Gadget (File/Folder Selection)](#getfile-gadget-filefolder-selection)
7. [General ReAction Gotchas](#general-reaction-gotchas)
8. [Memory and Type Safety](#memory-and-type-safety)

---

## Event Loop Patterns

### ⚠️ CRITICAL: Never Double-Wait in Event Loops

**Problem:** ReAction event handlers should contain the `Wait()` call INSIDE the handler function. Calling `Wait()` or `WaitPort()` in BOTH the handler and the caller creates a "one message behind" bug.

**Symptom:** First click on any gadget does nothing. Second click processes the FIRST click's event. Window close gadget requires two clicks. Child windows that close cause events to repeat.

**Wrong Pattern (Double-Wait):**
```c
/* In event handler function */
BOOL handle_window_events(WindowData *data) {
    ULONG signal_mask;
    
    GetAttr(WINDOW_SigMask, data->window_obj, &signal_mask);
    
    /* First wait here */
    signals = Wait(signal_mask | SIGBREAKF_CTRL_C);
    
    /* Process events */
    while ((result = RA_HandleInput(data->window_obj, &code)) != WMHI_LASTMSG) {
        /* Handle events */
    }
    
    return continue_running;
}

/* In caller - WRONG! */
while (handle_window_events(&data)) {
    WaitPort(data.window->UserPort);  /* ❌ Second wait causes "one behind" bug */
}
```

**Correct Pattern:**
```c
/* In event handler function */
BOOL handle_window_events(WindowData *data) {
    ULONG signal_mask;
    
    GetAttr(WINDOW_SigMask, data->window_obj, &signal_mask);
    
    /* Wait inside the handler */
    signals = Wait(signal_mask | SIGBREAKF_CTRL_C);
    
    if (signals & SIGBREAKF_CTRL_C) {
        return FALSE;
    }
    
    /* Process all pending events */
    while ((result = RA_HandleInput(data->window_obj, &code)) != WMHI_LASTMSG) {
        /* Handle events */
    }
    
    return continue_running;
}

/* In caller - CORRECT */
while (handle_window_events(&data)) {
    /* ✅ No wait here - handler does the waiting */
}
```

**Why This Happens:**
1. User clicks gadget → signal sent
2. Inner `Wait()` in handler returns and processes events
3. Handler returns to caller
4. **Outer `WaitPort()` blocks waiting for NEXT event** ← Bug is here
5. Second click triggers outer wait
6. Now processes the FIRST click's queued event
7. Creates perpetual "one event behind" behavior

**Key Principle:** Either the event handler OR the caller should wait, never both. ReAction best practice is to have the handler do the waiting.

---

### Don't Check Signal Mask After Wait()

**Problem:** After calling `Wait()`, don't check if the signal mask is set before calling `RA_HandleInput()`.

**Wrong:**
```c
signals = Wait(signal_mask | SIGBREAKF_CTRL_C);

if (signals & SIGBREAKF_CTRL_C)
    return FALSE;

if (signals & signal_mask) {  /* ❌ Unnecessary check */
    while ((result = RA_HandleInput(...)) != WMHI_LASTMSG) {
        /* Handle events */
    }
}
```

**Correct:**
```c
signals = Wait(signal_mask | SIGBREAKF_CTRL_C);

if (signals & SIGBREAKF_CTRL_C)
    return FALSE;

/* ✅ RA_HandleInput checks for pending events internally */
while ((result = RA_HandleInput(...)) != WMHI_LASTMSG) {
    /* Handle events */
}
```

**Why:** `RA_HandleInput()` internally checks if there are pending messages. The `if (signals & signal_mask)` wrapper can cause events to be skipped if the signal was consumed by the `Wait()` call.

---

### Standard ReAction Event Loop Template

```c
BOOL handle_window_events(struct WindowData *data)
{
    ULONG signal_mask;
    ULONG signals;
    ULONG result;
    UWORD code;
    BOOL continue_running = TRUE;
    
    if (!data || !data->window_obj)
        return FALSE;
    
    /* Get window signal mask */
    GetAttr(WINDOW_SigMask, data->window_obj, &signal_mask);
    
    /* Wait for signals (inside the handler) */
    signals = Wait(signal_mask | SIGBREAKF_CTRL_C);
    
    /* Check for Ctrl-C */
    if (signals & SIGBREAKF_CTRL_C) {
        return FALSE;
    }
    
    /* Process all pending window events */
    while ((result = RA_HandleInput(data->window_obj, &code)) != WMHI_LASTMSG)
    {
        switch (result & WMHI_CLASSMASK)
        {
            case WMHI_CLOSEWINDOW:
                continue_running = FALSE;
                break;
                
            case WMHI_GADGETUP:
                /* Handle gadget events */
                break;
        }
    }
    
    return continue_running;
}

/* Caller just calls handler in a loop */
while (handle_window_events(&window_data)) {
    /* No waiting here! */
}
```

---

### Modal Child Window Pattern

When opening a modal child window from a parent window:

```c
/* In parent window's gadget handler */
case GID_OPEN_CHILD_WINDOW:
    {
        struct ChildWindowData child_data;
        
        /* Open child window */
        if (open_child_window(&child_data))
        {
            /* Run child window event loop */
            while (handle_child_window_events(&child_data))
            {
                /* Event handler includes Wait() - no WaitPort() here! */
            }
            
            /* Close child window */
            close_child_window(&child_data);
        }
    }
    break;
```

**Key Points:**
- Child window's event handler contains `Wait()`
- Parent's loop just calls the handler repeatedly
- No `WaitPort()` in the parent's loop
- This prevents the "one behind" bug

---

## ListBrowser Hierarchical Tree View

### ⚠️ CRITICAL: GetListBrowserNodeAttrs Uses ULONG Pointers

**Problem:** When calling `GetListBrowserNodeAttrs()`, all output parameters must be `ULONG*`, not `WORD*` or `int*`.

**Symptom:** Generation numbers always read back as 0, even though they were set correctly during node creation.

**Wrong:**
```c
WORD curr_gen = 0;  /* WRONG! Only 16 bits */
GetListBrowserNodeAttrs(node,
    LBNA_Generation, &curr_gen,  /* Writes 32 bits to 16-bit var! */
    TAG_END);
```

**Correct:**
```c
ULONG curr_gen = 0;  /* CORRECT - 32 bits */
GetListBrowserNodeAttrs(node,
    LBNA_Generation, &curr_gen,
    TAG_END);
```

**Root Cause:** AmigaOS tag-based attribute functions write ULONG (32-bit) values. Using a smaller type corrupts the stack and adjacent variables on 68k.

---

### ⚠️ CRITICAL: Tag Order for LBNCA_CopyText

**Problem:** When using `LBNCA_CopyText`, it **MUST** come BEFORE `LBNCA_Text`.

**Symptom:** ListBrowser displays corrupted/garbage text (raw hex values, memory addresses).

**Wrong:**
```c
node = AllocListBrowserNode(1,
    LBNCA_Text, "Hello",      /* WRONG ORDER */
    LBNCA_CopyText, TRUE,
    TAG_DONE);
```

**Correct:**
```c
node = AllocListBrowserNode(1,
    LBNCA_CopyText, TRUE,     /* MUST come first! */
    LBNCA_Text, "Hello",
    TAG_DONE);
```

---

### ⚠️ CRITICAL: List Must Be Populated BEFORE Gadget Creation

**Problem:** For hierarchical ListBrowser, the list must be fully populated and `HideAllListBrowserChildren()` called BEFORE attaching the list to the gadget.

**Symptom:** No disclosure triangles appear, tree doesn't expand/collapse.

**Wrong Pattern:**
```c
/* Create empty list */
NewList(&list);

/* Create gadget with empty list attached */
listbrowser = NewObject(LISTBROWSER_GetClass(), NULL,
    LISTBROWSER_Labels, &list,
    LISTBROWSER_Hierarchical, TRUE,
    TAG_DONE);

/* Open window */
RA_OpenWindow(window);

/* Populate list AFTER window is open - WRONG! */
populate_list(&list);
HideAllListBrowserChildren(&list);  /* Too late! */
```

**Correct Pattern:**
```c
/* Create empty list */
NewList(&list);

/* Populate list FIRST */
populate_list(&list);

/* Hide children to start collapsed - BEFORE gadget creation! */
HideAllListBrowserChildren(&list);

/* NOW create gadget with populated list */
listbrowser = NewObject(LISTBROWSER_GetClass(), NULL,
    LISTBROWSER_Labels, &list,
    LISTBROWSER_Hierarchical, TRUE,
    TAG_DONE);

/* Open window */
RA_OpenWindow(window);
```

---

### Generation Numbers Are 0-Based

**Problem:** LBNA_Generation uses 0 for root level, 1 for first children, etc.

**Wrong:**
```c
generation = depth + 1;  /* Root becomes 1 - WRONG */
```

**Correct:**
```c
generation = depth;  /* Root is 0, children are 1, 2, etc. */
```

**Example tree structure:**
```
AmigaOS (generation 0)
├── Workbench 1.x (generation 1)
├── Workbench 3.x (generation 1)
│   ├── Version 3.1 (generation 2)
│   └── Version 3.2 (generation 2)
│       └── 3.2.1 (generation 3)
```

---

### LBFLG_HASCHILDREN Required for Parent Nodes

**Problem:** Parent nodes (branches) must have `LBFLG_HASCHILDREN` flag set for disclosure triangles to appear.

**Implementation:** Set flag during node creation OR in post-processing:

```c
/* Option 1: Set during creation if known */
if (has_children) {
    flags |= LBFLG_HASCHILDREN;
}
node = AllocListBrowserNode(1,
    LBNA_Flags, flags,
    ...);

/* Option 2: Post-process after all nodes created */
/* A node has children if the next node has higher generation */
if (next_gen > curr_gen) {
    SetListBrowserNodeAttrs(curr_node,
        LBNA_Flags, curr_flags | LBFLG_HASCHILDREN,
        TAG_END);
}
```

---

### Column Info Terminator

**Problem:** Multi-column ListBrowser requires a properly terminated ColumnInfo array.

**Wrong:**
```c
column_info[2].ci_Width = -1;  /* Only width set */
```

**Correct:**
```c
column_info[2].ci_Width = -1;
column_info[2].ci_Title = (STRPTR)~0;
column_info[2].ci_Flags = -1;
```

---

### Complete Working Example Pattern

Based on the working `Tests/ReActon/listbrowser_tree.c`:

```c
/* 1. Open libraries */
ListBrowserBase = OpenLibrary("gadgets/listbrowser.gadget", 47);

/* 2. Create and populate list */
struct List *list = AllocVec(sizeof(struct List), MEMF_CLEAR);
NewList(list);

for (i = 0; data[i].name != NULL; i++) {
    ULONG flags = 0;
    
    /* Parent nodes get HASCHILDREN flag */
    if (data[i].has_children) {
        flags |= LBFLG_HASCHILDREN;
    }
    
    struct Node *node = AllocListBrowserNode(1,
        LBNCA_Text, data[i].name,
        LBNA_Generation, data[i].generation,  /* 0-based! */
        LBNA_Flags, flags,
        TAG_DONE);
    
    if (node) AddTail(list, node);
}

/* 3. Hide all children BEFORE creating gadget */
HideAllListBrowserChildren(list);

/* 4. Create gadget with populated list */
listbrowser = NewObject(LISTBROWSER_GetClass(), NULL,
    GA_ID, 1,
    GA_RelVerify, TRUE,
    LISTBROWSER_Labels, list,
    LISTBROWSER_Hierarchical, TRUE,
    LISTBROWSER_ShowSelected, TRUE,
    TAG_DONE);

/* 5. Create window with gadget */
window = NewObject(WINDOW_GetClass(), NULL,
    WINDOW_Layout, NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_AddChild, listbrowser,
        TAG_DONE),
    TAG_DONE);

/* 6. Open window */
RA_OpenWindow(window);
```

---

## ListBrowser Column Sorting

### ⚠️ CRITICAL: Getting Column Sorting to Work

Implementing clickable column sorting in ListBrowser requires several specific steps that are easy to get wrong. Missing any of these will result in no events, no sorting, or sort arrows in the wrong column.

### Issue 1: LISTBROWSER_TitleClickable Must Be Set AFTER Window Opens

**Problem:** Setting `LISTBROWSER_TitleClickable, TRUE` in `NewObject()` during gadget creation is **silently ignored**. Clicking column headers generates no events.

**Symptom:** No events when clicking column headers. Log shows `TitleClickable=0` even though you set it to TRUE.

**Wrong:**
```c
/* During window creation - IGNORED! */
tool_listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
    GA_ID, GID_TOOL_LIST,
    LISTBROWSER_ColumnInfo, column_info,
    LISTBROWSER_ColumnTitles, TRUE,
    LISTBROWSER_TitleClickable, TRUE,  /* ❌ This is IGNORED */
TAG_END),
```

**Correct:**
```c
/* During window creation - omit TitleClickable */
tool_listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
    GA_ID, GID_TOOL_LIST,
    LISTBROWSER_ColumnInfo, column_info,
    LISTBROWSER_ColumnTitles, TRUE,
TAG_END),

/* AFTER window is opened with RA_OpenWindow() */
window = (struct Window *)RA_OpenWindow(window_obj);

/* NOW set TitleClickable with SetGadgetAttrs */
SetGadgetAttrs((struct Gadget *)tool_listbrowser_obj,
               window, NULL,
               LISTBROWSER_TitleClickable, TRUE,
               TAG_DONE);
```

**Why:** ReAction needs the window to be fully realized before certain attributes can be set. This is a quirk of the listbrowser.gadget implementation in Workbench 3.2.

### Issue 2: Use AllocLBColumnInfo, Not Static Arrays

**Problem:** Using static `struct ColumnInfo` arrays and trying to modify them with `SetLBColumnInfoAttrs()` produces garbage values. The Get/Set functions don't work correctly on static arrays.

**Symptom:** `GetLBColumnInfoAttrs()` returns huge garbage numbers like 1100959492 instead of TRUE/FALSE values.

**Wrong:**
```c
/* Static array - SetLBColumnInfoAttrs doesn't work! */
static struct ColumnInfo column_info[] = {
    { 55, "Tool",   0 },
    { 12, "Files",  0 },
    { 15, "Status", 0 },
    { -1, (STRPTR)~0, -1 }
};

/* This produces garbage values */
SetLBColumnInfoAttrs(column_info,
    LBCIA_Column, 0,
    LBCIA_Sortable, TRUE,  /* Doesn't work! */
TAG_DONE);
```

**Correct:**
```c
/* Dynamically allocate with AllocLBColumnInfo */
struct ColumnInfo *column_info = NULL;

column_info = AllocLBColumnInfo(3,
    LBCIA_Column, 0,
        LBCIA_Title, "Tool",
        LBCIA_Weight, 55,
        LBCIA_Sortable, TRUE,
        LBCIA_SortArrow, TRUE,
    LBCIA_Column, 1,
        LBCIA_Title, "Files",
        LBCIA_Weight, 12,
        LBCIA_Sortable, TRUE,
        LBCIA_SortArrow, TRUE,
    LBCIA_Column, 2,
        LBCIA_Title, "Status",
        LBCIA_Weight, 15,
        LBCIA_Sortable, TRUE,
        LBCIA_SortArrow, TRUE,
    TAG_DONE);

/* Don't forget to free when done */
FreeLBColumnInfo(column_info);
```

**Why:** `AllocLBColumnInfo()` creates a properly structured internal representation that the Get/Set functions expect. Static arrays don't have this internal structure.

### Issue 3: List Must Be ATTACHED When Calling LBM_SORT

**Problem:** If you detach the list (set `LISTBROWSER_Labels, ~0`) before calling `LBM_SORT`, the sort method returns -1 (failure) and nothing gets sorted.

**Symptom:** `DoGadgetMethod(..., LBM_SORT, ...)` returns -1. Clicking columns does nothing visible. List doesn't reorder.

**Wrong:**
```c
/* Detach list - WRONG! */
SetGadgetAttrs((struct Gadget *)listbrowser_obj,
               window, NULL,
               LISTBROWSER_Labels, ~0,
               TAG_DONE);

/* Sort fails because list is detached */
DoGadgetMethod((struct Gadget *)listbrowser_obj,
              window, NULL,
              LBM_SORT, list, column, direction, NULL);  /* ❌ Returns -1 */

/* Re-attach list */
SetGadgetAttrs((struct Gadget *)listbrowser_obj,
               window, NULL,
               LISTBROWSER_Labels, list,
               TAG_DONE);
```

**Correct:**
```c
/* List MUST be attached when calling LBM_SORT */
/* DoGadgetMethod signature: 
   DoGadgetMethod(gadget, window, requester, LBM_SORT, list, column, direction, hook) */
result = DoGadgetMethod((struct Gadget *)listbrowser_obj,
                       window, NULL,
                       LBM_SORT, list, column, direction, NULL);  /* ✅ Returns 0 */

/* Update sort arrow indicator */
SetGadgetAttrs((struct Gadget *)listbrowser_obj,
               window, NULL,
               LISTBROWSER_SortColumn, column,
               TAG_DONE);

/* Refresh to show changes */
RefreshGadgets((struct Gadget *)listbrowser_obj, window, NULL);
```

**Why:** `LBM_SORT` operates on the attached list and needs to notify the gadget of changes. Detaching breaks this connection.

### Issue 4: The 'code' Parameter Contains Clicked Column Number

**Problem:** Using `GetAttr(LISTBROWSER_SortColumn)` to determine which column was clicked returns the **currently sorted column**, not the column that was just clicked. This causes the sort arrow to stay in the wrong column.

**Symptom:** Clicking different columns sorts correctly, but the sort arrow stays in column 0. Log shows `Sort column = 0` regardless of which column was clicked.

**Wrong:**
```c
/* In WMHI_GADGETUP handler for listbrowser */
case GID_TOOL_LIST_LISTBROWSER:
{
    ULONG rel_event = LBRE_NORMAL;
    
    GetAttr(LISTBROWSER_RelEvent, listbrowser_obj, &rel_event);
    
    if (rel_event == LBRE_TITLECLICK)
    {
        ULONG sort_column = 0;
        
        /* WRONG - returns OLD sorted column */
        GetAttr(LISTBROWSER_SortColumn, listbrowser_obj, &sort_column);  /* ❌ Always 0 */
        
        DoGadgetMethod(..., LBM_SORT, list, sort_column, ...);
    }
}
```

**Correct:**
```c
/* In event loop where RA_HandleInput is called */
UWORD code;

while ((result = RA_HandleInput(window_obj, &code)) != WMHI_LASTMSG)
{
    switch (result & WMHI_CLASSMASK)
    {
        case WMHI_GADGETUP:
            switch (result & WMHI_GADGETMASK)
            {
                case GID_TOOL_LIST_LISTBROWSER:
                {
                    ULONG rel_event = LBRE_NORMAL;
                    
                    GetAttr(LISTBROWSER_RelEvent, listbrowser_obj, &rel_event);
                    
                    if (rel_event == LBRE_TITLECLICK)
                    {
                        /* CORRECT - 'code' parameter contains clicked column */
                        ULONG sort_column = code;  /* ✅ Column 0, 1, 2, etc. */
                        
                        DoGadgetMethod(..., LBM_SORT, list, sort_column, ...);
                        
                        /* Update sort arrow to clicked column */
                        SetGadgetAttrs((struct Gadget *)listbrowser_obj,
                                       window, NULL,
                                       LISTBROWSER_SortColumn, sort_column,
                                       TAG_DONE);
                    }
                }
            }
    }
}
```

**Why:** When `LBRE_TITLECLICK` is the event type, ReAction stores the clicked column number in the `code` parameter that `RA_HandleInput()` fills in. This is the standard Intuition pattern for communicating additional event details.

### Complete Working Pattern

```c
/* 1. Allocate column info with sorting enabled */
struct ColumnInfo *column_info = AllocLBColumnInfo(3,
    LBCIA_Column, 0,
        LBCIA_Title, "Name",
        LBCIA_Sortable, TRUE,
        LBCIA_SortArrow, TRUE,
    LBCIA_Column, 1,
        LBCIA_Title, "Count",
        LBCIA_Sortable, TRUE,
        LBCIA_SortArrow, TRUE,
    LBCIA_Column, 2,
        LBCIA_Title, "Status",
        LBCIA_Sortable, TRUE,
        LBCIA_SortArrow, TRUE,
    TAG_DONE);

/* 2. Create listbrowser WITHOUT TitleClickable */
listbrowser_obj = NewObject(LISTBROWSER_GetClass(), NULL,
    GA_ID, GID_LISTBROWSER,
    GA_RelVerify, TRUE,
    LISTBROWSER_ColumnInfo, column_info,
    LISTBROWSER_ColumnTitles, TRUE,
    LISTBROWSER_Labels, &list,
TAG_END);

/* 3. Open window */
window = (struct Window *)RA_OpenWindow(window_obj);

/* 4. Enable TitleClickable AFTER window opens */
SetGadgetAttrs((struct Gadget *)listbrowser_obj,
               window, NULL,
               LISTBROWSER_TitleClickable, TRUE,
               TAG_DONE);

/* 5. In event loop, handle LBRE_TITLECLICK */
UWORD code;
while ((result = RA_HandleInput(window_obj, &code)) != WMHI_LASTMSG)
{
    switch (result & WMHI_CLASSMASK)
    {
        case WMHI_GADGETUP:
            switch (result & WMHI_GADGETMASK)
            {
                case GID_LISTBROWSER:
                {
                    ULONG rel_event;
                    GetAttr(LISTBROWSER_RelEvent, listbrowser_obj, &rel_event);
                    
                    if (rel_event == LBRE_TITLECLICK)
                    {
                        ULONG column = code;  /* Column number from code parameter */
                        ULONG direction = LBMSORT_FORWARD;  /* Or toggle based on state */
                        
                        /* Sort with list attached */
                        DoGadgetMethod((struct Gadget *)listbrowser_obj,
                                      window, NULL,
                                      LBM_SORT, &list, column, direction, NULL);
                        
                        /* Update sort arrow */
                        SetGadgetAttrs((struct Gadget *)listbrowser_obj,
                                       window, NULL,
                                       LISTBROWSER_SortColumn, column,
                                       TAG_DONE);
                        
                        RefreshGadgets((struct Gadget *)listbrowser_obj, window, NULL);
                    }
                }
            }
    }
}

/* 6. Cleanup */
FreeLBColumnInfo(column_info);
```

### Key Takeaways

1. **TitleClickable:** Set with `SetGadgetAttrs()` AFTER window opens, not in `NewObject()`
2. **Column Info:** Use `AllocLBColumnInfo()`, not static arrays
3. **Sorting:** List must be ATTACHED when calling `LBM_SORT`
4. **Column Number:** Use the `code` parameter, not `GetAttr(LISTBROWSER_SortColumn)`
5. **Sort Arrow:** Update with `SetGadgetAttrs(..., LISTBROWSER_SortColumn, column, ...)`

### Reference Implementation

See `src/GUI/DefaultTools/tool_cache_window.c` for a complete working example.

---

## ListBrowser Column Resizing

### ⚠️ CRITICAL: CIF_SORTABLE and CIF_DRAGGABLE Must Be Combined with Bitwise OR

**Problem:** When enabling both column sorting AND column resizing, setting `LBCIA_Flags` to only `CIF_DRAGGABLE` will make resizing work but **sorting stops working completely**. The flags are bitfields that must be combined.

**Symptom:** 
- Column resizing works (dragging separators resizes columns)
- Column sorting stops working (clicking headers does nothing)
- Log shows `LBRE_COLUMNADJUST` events (256) but no `LBRE_TITLECLICK` events (128)
- User clicks on column titles but list doesn't sort

**Wrong:**
```c
/* Only CIF_DRAGGABLE - breaks sorting! */
column_info = AllocLBColumnInfo(3,
    LBCIA_Column, 0,
        LBCIA_Title, "Tool",
        LBCIA_Sortable, TRUE,      /* ← These are ignored! */
        LBCIA_SortArrow, TRUE,     /* ← These are ignored! */
        LBCIA_Flags, CIF_DRAGGABLE,  /* ❌ Only resizing works */
    LBCIA_Column, 1,
        LBCIA_Title, "Files",
        LBCIA_Sortable, TRUE,
        LBCIA_SortArrow, TRUE,
        LBCIA_Flags, CIF_DRAGGABLE,  /* ❌ Only resizing works */
    TAG_DONE);
```

**Correct:**
```c
/* Combine flags with bitwise OR */
column_info = AllocLBColumnInfo(3,
    LBCIA_Column, 0,
        LBCIA_Title, "Tool",
        LBCIA_Sortable, TRUE,
        LBCIA_SortArrow, TRUE,
        LBCIA_Flags, CIF_SORTABLE | CIF_DRAGGABLE,  /* ✅ Both work! */
    LBCIA_Column, 1,
        LBCIA_Title, "Files",
        LBCIA_Sortable, TRUE,
        LBCIA_SortArrow, TRUE,
        LBCIA_Flags, CIF_SORTABLE | CIF_DRAGGABLE,  /* ✅ Both work! */
    LBCIA_Column, 2,
        LBCIA_Title, "Status",
        LBCIA_Sortable, TRUE,
        LBCIA_SortArrow, TRUE,
        LBCIA_Flags, CIF_SORTABLE | CIF_DRAGGABLE,  /* ✅ Both work! */
    TAG_DONE);
```

**Why:** The `LBCIA_Flags` field contains bitfield flags. `CIF_SORTABLE` and `CIF_DRAGGABLE` are different bits that need to be OR'd together:
- `CIF_SORTABLE` enables title clicking for sorting
- `CIF_DRAGGABLE` enables separator dragging for resizing
- Setting only one flag disables the other feature

The `LBCIA_Sortable` and `LBCIA_SortArrow` tags are convenience tags that internally set the `CIF_SORTABLE` flag bit. But if you then set `LBCIA_Flags` to ONLY `CIF_DRAGGABLE`, you overwrite the flag field and lose the sortable bit.

### Handling LBRE_COLUMNADJUST Events

When column resizing is enabled, the listbrowser generates `LBRE_COLUMNADJUST` events (value 256) when the user drags a separator. These should be acknowledged in your event handler to prevent "Unhandled event" warnings.

**Event Handler Pattern:**
```c
case GID_LISTBROWSER:
{
    ULONG rel_event;
    GetAttr(LISTBROWSER_RelEvent, listbrowser_obj, &rel_event);
    
    if (rel_event == LBRE_TITLECLICK)
    {
        /* Handle sorting (column number in 'code' parameter) */
        ULONG column = code;
        DoGadgetMethod(..., LBM_SORT, list, column, direction, NULL);
    }
    else if (rel_event == LBRE_COLUMNADJUST)
    {
        /* Column resizing - handled automatically by gadget */
        /* No action needed, but acknowledge to prevent warnings */
    }
    else if (selected != ~0)
    {
        /* Handle normal row selection */
    }
}
```

### Key Takeaways

1. **Always combine flags:** Use `CIF_SORTABLE | CIF_DRAGGABLE` for both features
2. **Flag types matter:** These are bitfield flags, not boolean options
3. **Convenience tags:** `LBCIA_Sortable` and `LBCIA_SortArrow` set `CIF_SORTABLE` internally
4. **Manual flags override:** Setting `LBCIA_Flags` explicitly replaces all flags, so use `|` to combine
5. **Handle both events:** Check for both `LBRE_TITLECLICK` (sorting) and `LBRE_COLUMNADJUST` (resizing)

### Available Column Flags

From `listbrowser_gc.doc`:
- `CIF_WEIGHTED` - Weighted width column (default)
- `CIF_FIXED` - Fixed pixel width specified in ci_Width
- `CIF_DRAGGABLE` - Separator is user-draggable (enables resizing)
- `CIF_NOSEPARATORS` - No separator on this column
- `CIF_SORTABLE` - Column is sortable (enabled by LBCIA_Sortable tag)
- `CIF_CENTER` - Column title is centered (V47)
- `CIF_RIGHT` - Column title is right-justified (V47)

Multiple flags can be combined with bitwise OR (`|`) as needed.

---

## Dynamic Text Labels

### ⚠️ CRITICAL: Label Images Don't Support Runtime Text Updates

**Problem:** ReAction `label.image` class creates **static labels** that cannot be updated after creation. Calling `SetGadgetAttrs()` with `LABEL_Text` has no effect.

**Symptom:** Label text stays stuck on initial value (e.g., "Starting...") even though `SetGadgetAttrs()` is called. Progress bars update correctly, but text never changes.

**Wrong Approach:**
```c
/* Create label image */
Object *label = NewObject(LABEL_GetClass(), NULL,
    LABEL_Text, "Starting...",
    TAG_DONE);

/* Try to update later - DOESN'T WORK! */
SetGadgetAttrs((struct Gadget *)label, window, NULL,
    LABEL_Text, "Processing: Work:Games (5/63)",  /* ❌ No effect */
    TAG_DONE);
```

**Correct Solution: Use Button Gadgets with GA_ReadOnly**

Button gadgets support dynamic `GA_Text` updates. Configure them to look like labels:

```c
/* Create button as updateable label */
Object *label = NewObject(BUTTON_GetClass(), NULL,
    GA_ID,                GID_STATUS_LABEL,
    GA_ReadOnly,          TRUE,              /* Makes it non-interactive */
    GA_Text,              "Starting...",
    BUTTON_BevelStyle,    BVS_NONE,         /* No button border */
    BUTTON_Transparent,   TRUE,             /* Background shows through */
    BUTTON_Justification, BCJ_LEFT,         /* Left-aligned text */
    TAG_DONE);

/* Update text dynamically - WORKS! */
SetGadgetAttrs((struct Gadget *)label, window, NULL,
    GA_Text, "Processing: Work:Games (5/63)",  /* ✅ Updates correctly */
    TAG_DONE);
```

**Key Points:**
- Use `BUTTON_GetClass()` instead of `LABEL_GetClass()`
- `GA_ReadOnly, TRUE` prevents user interaction
- `BUTTON_BevelStyle, BVS_NONE` removes button appearance
- `BUTTON_Transparent, TRUE` makes background transparent (window texture shows through)
- Update with `GA_Text` (not `LABEL_Text`)
- No need to call `RethinkLayout()` or `RefreshGadgets()`

**Visual Appearance:**
With the above settings, the button is indistinguishable from a label but fully updateable.

**Example Use Cases:**
- Progress window status labels ("Processing: [folder name]")
- Status bars showing current operation
- Dynamic count displays ("Files: 15/63")
- Any label text that needs to change at runtime

**Pattern Used In:**
- `src/GUI/StatusWindows/recursive_progress.c` - Folder progress labels
- `src/GUI/StatusWindows/main_progress_window.c` - Main operation status

---

### ❌ Don't Use label.image or string.gadget for Dynamic Text

**Wrong Alternatives:**

1. **label.image** - Static only, cannot be updated at runtime
   - `SetGadgetAttrs()` with `LABEL_Text` has no effect
   - Text stays frozen on initial value

2. **string.gadget** - Designed for text input, not labels
   - Always looks like an editable text field (beveled border)
   - No simple transparency support (requires complex GA_BackFill hook)
   - Maintains unnecessary edit buffers (STRINGA_Buffer, WorkBuffer, UndoBuffer)
   - Even with `GA_ReadOnly`, visually inappropriate for status display

**Correct Choice: button.gadget with GA_ReadOnly**
- Native `BUTTON_Transparent` support (no hooks needed)
- `BUTTON_BevelStyle = BVS_NONE` removes all visual chrome
- Designed for read-only display with `GA_ReadOnly`
- Lightweight, no unnecessary edit infrastructure
- Standard ReAction pattern for updateable labels

---

## General ReAction Gotchas

### Don't Use Custom Tree Images Unless Necessary

The official documentation recommends using the default disclosure triangles:

> "The branch indicator image can be overwritten with LISTBROWSER_ShowImage and LISTBROWSER_HideImage although it is **strongly encouraged to keep the default**"

Custom glyph images may not render in all situations.

---

### DisposeObject Disposes Children Automatically

When you call `DisposeObject()` on a window object, it automatically disposes all child objects (layouts, gadgets, etc.).

**Wrong:**
```c
DisposeObject(button_obj);   /* Double-free! */
DisposeObject(layout_obj);   /* Double-free! */
DisposeObject(window_obj);   /* This already disposed children */
```

**Correct:**
```c
DisposeObject(window_obj);   /* Disposes everything */
```

---

### Detach List Before Freeing

Always detach the list from the ListBrowser before freeing nodes:

```c
/* Detach list */
SetAttrs(listbrowser, LISTBROWSER_Labels, ~0, TAG_DONE);

/* Now safe to free */
FreeListBrowserList(list);
FreeVec(list);
```

---

## GetFile Gadget (File/Folder Selection)

### ⚠️ CRITICAL: GetFile Gadget Does NOT Auto-Open Requester

**Problem:** Unlike ASL requesters called directly, the `getfile.gadget` does NOT automatically open a file/folder requester when clicked. Clicking the gadget sends a `GADGETUP` event, but nothing visible happens.

**Symptom:** User clicks the folder/file selection button and nothing happens. No requester appears.

**Wrong Pattern:**
```c
/* Event handler - WRONG! */
case GID_FOLDER_GETFILE:
{
    /* ❌ Just reading the attribute - requester never opened */
    char *folder_path = NULL;
    GetAttr(GETFILE_Drawer, folder_getfile_obj, (ULONG *)&folder_path);
    if (folder_path && folder_path[0])
    {
        strncpy(buffer, folder_path, sizeof(buffer) - 1);
    }
    break;
}
```

**Correct Pattern:**
```c
/* Event handler - CORRECT */
case GID_FOLDER_GETFILE:
{
    /* ✅ Manually invoke the requester with GFILE_REQUEST */
    if (DoMethod((Object *)folder_getfile_obj, GFILE_REQUEST, window))
    {
        /* Requester closed with selection - now get the path */
        char *folder_path = NULL;
        GetAttr(GETFILE_Drawer, folder_getfile_obj, (ULONG *)&folder_path);
        if (folder_path && folder_path[0])
        {
            strncpy(buffer, folder_path, sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
        }
    }
    /* If DoMethod returns 0, user cancelled - do nothing */
    break;
}
```

### GetFile Gadget Creation (Folder Mode)

```c
LAYOUT_AddChild, folder_getfile_obj = NewObject(GETFILE_GetClass(), NULL,
    GA_ID, GID_FOLDER_GETFILE,
    GA_RelVerify, TRUE,              /* Critical - enables GADGETUP event */
    GETFILE_TitleText, "Select Folder",
    GETFILE_Drawer, initial_path,    /* Initial folder path */
    GETFILE_DrawersOnly, TRUE,       /* Folders only, no files */
    GETFILE_ReadOnly, FALSE,         /* Allow editing the path text */
TAG_END),
```

### GetFile Gadget Creation (File Mode)

```c
LAYOUT_AddChild, file_getfile_obj = NewObject(GETFILE_GetClass(), NULL,
    GA_ID, GID_FILE_GETFILE,
    GA_RelVerify, TRUE,
    GETFILE_TitleText, "Select File",
    GETFILE_FullFile, initial_file,  /* Use FullFile for files */
    GETFILE_DoSaveMode, FALSE,       /* FALSE=Open, TRUE=Save */
    GETFILE_ReadOnly, FALSE,
TAG_END),
```

### Key Points

1. **Always use `DoMethod(..., GFILE_REQUEST, window)`** to manually open the requester
2. **Use `DoMethod`, NOT `DoGadgetMethod`** - the latter crashes with ReAction
3. **Check return value** - `DoMethod` returns 0 if user cancelled
4. **Use `GETFILE_Drawer`** for folders, **`GETFILE_FullFile`** for files
5. **Set `GA_RelVerify, TRUE`** or you won't get `GADGETUP` events

---

## Memory and Type Safety

### All Tag Values Are ULONG

When using `GetAttr()`, `GetListBrowserNodeAttrs()`, or similar functions, **always use ULONG pointers**:

```c
/* WRONG - various types */
WORD gen;
int count;
BOOL selected;

/* CORRECT - all ULONG */
ULONG gen;
ULONG count;
ULONG selected;

GetAttr(SOME_TAG, object, &gen);
```

### AmigaOS BOOL Is 2 Bytes

On AmigaOS, `BOOL` is a 16-bit type (WORD), not 32-bit like modern systems. However, for tag-based attribute functions, still use ULONG:

```c
ULONG is_selected = 0;
GetListBrowserNodeAttrs(node, LBNA_Selected, &is_selected, TAG_END);
if (is_selected) { ... }
```

---

## Debugging Tips

### Add Comprehensive Logging

When ListBrowser isn't working, add logging at each stage:

```c
/* Log node creation */
log_info(LOG_GUI, "CREATE_NODE: name='%s' gen=%d flags=0x%08lx\n",
         name, (int)generation, (unsigned long)flags);

/* Log post-processing */
log_info(LOG_GUI, "POST[%d]: curr_gen=%d next_gen=%d flags=0x%08lx\n",
         idx, (int)curr_gen, (int)next_gen, (unsigned long)flags);

/* Log final state */
log_info(LOG_GUI, "FINAL[%d]: gen=%d flags=0x%08lx (HASCHILDREN=%s HIDDEN=%s)\n",
         idx, (int)gen, (unsigned long)flags,
         (flags & LBFLG_HASCHILDREN) ? "YES" : "no",
         (flags & LBFLG_HIDDEN) ? "YES" : "no");
```

### Check Logs for Pattern Mismatches

If generation always reads back as 0, you have a type size mismatch (WORD vs ULONG).

If flags are correct but no triangles appear, the list wasn't set up before gadget creation.

---

## Reference Files

- **Working example:** `Tests/ReActon/listbrowser_tree.c`
- **iTidy implementation:** `src/GUI/RestoreBackups/folder_view_window.c`
- **Official docs:** `docs/listbrowser_treelist.txt`

---

## Version History

- **2026-01-30:** Added ListBrowser column resizing section
  - CRITICAL: CIF_SORTABLE and CIF_DRAGGABLE must be combined with bitwise OR (`|`)
  - Setting only CIF_DRAGGABLE breaks sorting completely
  - These are bitfield flags, not boolean options
  - LBCIA_Sortable/LBCIA_SortArrow convenience tags set CIF_SORTABLE internally
  - LBCIA_Flags explicitly replaces all flags, so use `CIF_SORTABLE | CIF_DRAGGABLE`
  - Handle LBRE_COLUMNADJUST (256) events in addition to LBRE_TITLECLICK (128)
  - List of all available column flags (CIF_WEIGHTED, CIF_FIXED, CIF_DRAGGABLE, etc.)
- **2026-01-30:** Added ListBrowser column sorting section
  - CRITICAL: LISTBROWSER_TitleClickable must be set with SetGadgetAttrs AFTER window opens
  - CRITICAL: Use AllocLBColumnInfo(), not static arrays for sortable columns
  - CRITICAL: List must be ATTACHED when calling LBM_SORT
  - CRITICAL: Use 'code' parameter for clicked column number, not GetAttr(LISTBROWSER_SortColumn)
  - Complete working pattern for clickable column sorting with sort arrow indicators
- **2026-01-30:** Added dynamic text labels section
  - CRITICAL: Label images don't support runtime text updates
  - Solution: Use Button gadgets with GA_ReadOnly, BVS_NONE, BUTTON_Transparent
  - Pattern for creating updateable labels that look like static labels
- **2026-01-30:** Added event loop patterns section
  - CRITICAL: Never double-wait in event loops (Wait() inside handler, not in caller)
  - Don't check signal mask after Wait()
  - Standard ReAction event loop template
  - Modal child window pattern
- **2026-01-30:** Initial document created after debugging hierarchical ListBrowser issues
  - ULONG vs WORD bug for GetListBrowserNodeAttrs
  - List population order (before gadget creation)
  - Generation numbers are 0-based
  - LBNCA_CopyText tag order
