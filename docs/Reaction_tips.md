# ReAction Tips and Lessons Learned

This document captures hard-won lessons from implementing ReAction GUI components in iTidy. **AI Agents: Read this before implementing any ReAction code!**

---

## Table of Contents
1. [Event Loop Patterns](#event-loop-patterns)
2. [Busy Pointer and IDCMP Flushing](#busy-pointer-and-idcmp-flushing)
3. [ListBrowser Hierarchical Tree View](#listbrowser-hierarchical-tree-view)
4. [ListBrowser Column Sorting](#listbrowser-column-sorting)
5. [ListBrowser Column Resizing](#listbrowser-column-resizing)
6. [ListBrowser Column Sizing and AutoFit](#listbrowser-column-sizing-and-autofit)
7. [Dynamic Text Labels](#dynamic-text-labels)
8. [GetFile Gadget (File/Folder Selection)](#getfile-gadget-filefolder-selection)
9. [ClickTab Gadget (Tabbed Interface)](#clicktab-gadget-tabbed-interface)
10. [Gadget Tooltips (HintInfo)](#gadget-tooltips-hintinfo)
11. [General ReAction Gotchas](#general-reaction-gotchas)
12. [Memory and Type Safety](#memory-and-type-safety)

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

## Busy Pointer and IDCMP Flushing

### ⚠️ CRITICAL: Flush IDCMP Messages When Clearing Busy Pointer

**Problem:** When a parent window sets `WA_BusyPointer` and opens a child window (synchronous/modal), the parent's IDCMP port continues to queue user events (clicks, close gadget, key presses). Although Intuition shows the busy pointer and the user *sees* the window as disabled, events are still delivered to the UserPort. When the child closes and the parent's `RA_HandleInput()` loop resumes, those stale messages fire immediately -- causing unexpected behaviour like the parent window closing, buttons activating, or gadgets changing state.

**Symptom:** Open a child window from a parent. Click the parent's close gadget (or any button) while it shows the busy pointer. Close the child window. The parent immediately processes the stale close/click event and exits or performs the wrong action.

**Root Cause:** AmigaOS Intuition queues IDCMP messages regardless of the busy pointer state. `WA_BusyPointer` only changes the mouse cursor graphic -- it does **not** block or discard input events. The messages sit in the window's `UserPort` until someone calls `GetMsg()` on them.

**Solution:** When clearing the busy pointer, drain all pending IDCMP messages from the window's UserPort *before* restoring the normal pointer. This is implemented in the `safe_set_window_pointer()` utility wrapper:

```c
void safe_set_window_pointer(struct Window *window, BOOL busy)
{
    if (!window) return;
    
    if (prefsWorkbench.workbenchVersion >= 39)
    {
        if (busy)
        {
            SetWindowPointer(window, WA_BusyPointer, TRUE, TAG_DONE);
        }
        else
        {
            /* Flush stale IDCMP messages that accumulated while busy.
             * Without this, queued clicks/close events fire immediately
             * when the parent event loop resumes. */
            struct IntuiMessage *msg;
            while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort)) != NULL)
            {
                ReplyMsg((struct Message *)msg);
            }
            
            SetWindowPointer(window, WA_Pointer, NULL, TAG_DONE);
        }
    }
}
```

**Why This Works:**
1. Parent sets busy pointer -> opens child window
2. User clicks on the busy parent -> Intuition queues IDCMP messages on `UserPort`
3. Child closes -> parent calls `safe_set_window_pointer(window, FALSE)`
4. The `FALSE` path calls `GetMsg()` in a loop to drain and `ReplyMsg()` every queued message
5. Normal pointer is restored
6. Parent's `RA_HandleInput()` loop resumes with a clean, empty message queue

### Complete Pattern: Busy Pointer for Child Windows

All child windows in iTidy are synchronous (the parent's event loop blocks while the child runs). The standard pattern is:

```c
/* Set busy pointer on parent BEFORE opening child */
safe_set_window_pointer(parent->window, TRUE);

if (open_child_window(&child_data))
{
    /* Child event loop -- parent is blocked here */
    while (handle_child_events(&child_data)) { }
    close_child_window(&child_data);
}

/* Clear busy pointer AFTER child closes.
 * This also flushes any stale IDCMP messages. */
safe_set_window_pointer(parent->window, FALSE);
```

**Key Points:**
- Set busy **before** the child opens (not after) -- if the open is slow, the parent should already show busy
- Clear busy **after** the child closes, outside the if/else -- covers both success and failure paths with a single call
- The flush happens automatically inside `safe_set_window_pointer()` -- no extra code needed at call sites
- Parent stays busy for the **full lifetime** of the child window, clearly communicating that it cannot be used
- Never manually dispose stale messages with `ReplyMsg()` at individual call sites -- the centralised wrapper handles it

### When Not to Flush

The flush is specifically appropriate when clearing a busy pointer after a blocking operation (child window, long scan, etc.). Do **not** flush IDCMP messages in normal event processing -- only when transitioning from busy back to normal.

### Implementation Detail: `safe_set_window_pointer()`

- **Defined in:** `src/GUI/gui_utilities.c`
- **Header:** `src/GUI/gui_utilities.h`
- **WB 2.x safety:** The wrapper checks `prefsWorkbench.workbenchVersion >= 39` before calling `SetWindowPointer()`. On WB 2.x systems it silently does nothing (no busy pointer support).
- **NULL-safe:** Passing a NULL window pointer is valid and returns immediately.

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

### Generation Numbers Are 1-Based (Autodoc)

**Problem:** The ListBrowser autodoc states generations start at 1. Using 0 for root can result in empty trees.

**Wrong:**
```c
generation = depth;  /* Root becomes 0 - WRONG for ListBrowser */
```

**Correct:**
```c
generation = depth + 1;  /* Root is 1, children are 2, 3, etc. */
```

**Example tree structure:**
```
AmigaOS (generation 1)
├── Workbench 1.x (generation 2)
├── Workbench 3.x (generation 2)
│   ├── Version 3.1 (generation 3)
│   └── Version 3.2 (generation 3)
│       └── 3.2.1 (generation 4)
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

### Treeview Checkboxes (LBNA_CheckBox)

**Problem:** Setting `LBNA_Selected` only highlights a row; it does not render a checkbox.

**Correct pattern (checkbox row):**
```c
node = AllocListBrowserNode(1,
    LBNA_Column, 0,
        LBNCA_CopyText, TRUE,
        LBNCA_Text, label,
    LBNA_Generation, generation,
    LBNA_Flags, flags,
    LBNA_CheckBox, TRUE,
    LBNA_Checked, initial_checked,
    TAG_DONE);
```

**Notes:**
- Use `LBNA_Checked` to toggle checkbox state.
- Use `LISTBROWSER_SelectedNode` + `LBNA_Checked` to read current state.
- Do not rely on `LBNA_Selected` for checkbox state.

---

### Checkbox Persistence Pitfall (Prefs Flag vs Disabled List)

**Symptom:** Checkbox state resets when reopening the window, even though OK was pressed and state was saved.

**Cause:** The UI used a helper that returns **TRUE when a feature flag disables filtering**, which forces all checkboxes to appear checked.

**Fix:** Drive checkbox state directly from the stored disabled list, ignoring the feature flag in the UI layer.

**Correct pattern (UI check):**
```c
static BOOL is_type_checked_for_ui(const LayoutPreferences *prefs, const char *type_name)
{
    if (prefs->deficons_disabled_types[0] == '\0')
        return TRUE;
    sprintf(search_token, ",%s,", type_name);
    sprintf(temp_buffer, ",%s,", prefs->deficons_disabled_types);
    return (strstr(temp_buffer, search_token) == NULL);
}
```

**Why:** UI should reflect the saved list **as-is**, not the runtime filtering behavior.

---

### Prefer HideAllListBrowserChildren()

**Problem:** Manually setting `LBFLG_HIDDEN` can hide the entire tree if the flag value differs by SDK/version.

**Correct:**
```c
HideAllListBrowserChildren(list);  /* Use API instead of LBFLG_HIDDEN */
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
        LBNA_Generation, data[i].generation,  /* 1-based! */
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

### ⚠️ CRITICAL: Selection Index Breaks After Sorting

**Problem:** When using ListBrowser sorting with column titles, if you maintain a separate "filtered list" or background list structure and use the selection index to look up items in that background list, **the indices become mismatched after sorting**. The ListBrowser nodes are reordered visually, but your background list remains in the original order.

**Symptom:** 
- Before sorting: Click item → correct details shown
- Click column header to sort
- After sorting: Click item → WRONG details shown (details from different item)
- The wrong item shown corresponds to the same position in the unsorted list

**Root Cause:** When `LBM_SORT` is called, the ListBrowser's node list is physically reordered. However, if you're using a parallel data structure (like `filtered_entries` or a cache array indexed by list position), that structure is NOT sorted. Using the selection index to walk the unsorted structure gives you the wrong item.

**Wrong Pattern:**
```c
/* When populating ListBrowser, store cache index in UserData */
for (node = filtered_entries.lh_Head; node->ln_Succ != NULL; node = node->ln_Succ)
{
    struct CacheEntry *entry = GET_ENTRY_FROM_NODE(node);
    
    lb_node = AllocListBrowserNode(3,
        LBNA_Column, 0,
            LBNCA_Text, entry->name,
        LBNA_UserData, (APTR)(ULONG)entry->cache_index,  /* Store for later */
        TAG_DONE);
    
    AddTail(&listbrowser_list, lb_node);
}

/* WRONG: Walk unsorted background list using selection index */
void handle_selection(LONG selected_index)
{
    struct Node *node;
    LONG index = 0;
    int cache_index = -1;
    
    /* ❌ This walks the UNSORTED filtered_entries list */
    for (node = filtered_entries.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ, index++)
    {
        if (index == selected_index)
        {
            struct CacheEntry *entry = GET_ENTRY_FROM_NODE(node);
            cache_index = entry->cache_index;  /* WRONG after sorting! */
            break;
        }
    }
    
    /* Use cache_index to show details - MISMATCHED! */
    show_details(cache_index);
}
```

**Correct Pattern:**
```c
/* When populating ListBrowser, store cache index in UserData */
for (node = filtered_entries.lh_Head; node->ln_Succ != NULL; node = node->ln_Succ)
{
    struct CacheEntry *entry = GET_ENTRY_FROM_NODE(node);
    
    lb_node = AllocListBrowserNode(3,
        LBNA_Column, 0,
            LBNCA_Text, entry->name,
        LBNA_UserData, (APTR)(ULONG)entry->cache_index,  /* ✅ Store cache index */
        TAG_DONE);
    
    AddTail(&listbrowser_list, lb_node);
}

/* CORRECT: Walk the sorted ListBrowser list and extract UserData */
void handle_selection(LONG selected_index)
{
    struct Node *selected_node = NULL;
    LONG index = 0;
    int cache_index = -1;
    
    /* ✅ Walk the SORTED listbrowser_list that matches visual order */
    for (selected_node = listbrowser_list.lh_Head;
         selected_node->ln_Succ != NULL;
         selected_node = selected_node->ln_Succ, index++)
    {
        if (index == selected_index)
        {
            /* Extract cache_index from the node's UserData */
            GetListBrowserNodeAttrs(selected_node,
                                   LBNA_UserData, &cache_index,
                                   TAG_DONE);
            break;
        }
    }
    
    /* Use cache_index to show details - CORRECT! */
    show_details(cache_index);
}
```

**Why This Works:**
1. During population, the cache index (or other lookup key) is stored in each ListBrowser node's `LBNA_UserData` field
2. When `LBM_SORT` is called, the ListBrowser physically reorders the nodes in the list
3. The `UserData` travels with the node - it's part of the node, not a separate array
4. Walking the sorted `listbrowser_list` gives nodes in visual order with correct UserData
5. Extracting `UserData` gives the correct cache index regardless of sort order

**Key Principle:** Never use a selection index to look up data in a separate unsorted structure after the ListBrowser has been sorted. Always extract the lookup key from the selected node's `LBNA_UserData` instead.

**Alternative Solution:** If you need to maintain a parallel sorted array for performance reasons, you must re-sort that array using the same compare function and direction as the ListBrowser sort. However, using `LBNA_UserData` is simpler and more reliable.

**Reference:** See `src/GUI/DefaultTools/tool_cache_window.c` function `update_tool_details()` for the fixed implementation.

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

### ⚠️ CRITICAL: Numeric Column Sorting - Use LBNCA_Integer

**Problem:** By default, ListBrowser sorts ALL columns as strings, even columns containing numeric values. This causes incorrect sort order like "1, 10, 2, 20, 3" instead of "1, 2, 3, 10, 20".

**Symptom:** A column displaying numbers (like file counts) sorts alphabetically:
- Before sort: 5, 12, 3, 100, 8
- After clicking column header: 100, 12, 3, 5, 8 (string sort)
- Expected: 3, 5, 8, 12, 100 (numeric sort)

**Root Cause:** Numeric data was being passed as text strings using `LBNCA_Text`, causing alphabetic sorting.

---

### Solution 1: Use LBNCA_Integer (RECOMMENDED) ✅

**The proper approach** is to use `LBNCA_Integer` to store numeric values, which provides native numeric sorting and display:

**CRITICAL: LBNCA_Integer expects a POINTER to LONG, not the value itself!**

**CRITICAL: LBNCA_Integer expects a POINTER to LONG, not the value itself!**

```c
/* CORRECT: Pass a pointer to the integer field */
struct ToolCacheDisplayEntry
{
    struct Node node;
    char *tool_name;
    int file_count;         /* Embedded integer that persists with the structure */
    BOOL exists;
    /* ... other fields ... */
};

/* Create ListBrowser node with integer column */
lb_node = AllocListBrowserNode(3,
    LBNA_Column, 0,
        LBNCA_CopyText, TRUE,
        LBNCA_Text, entry->tool_name,
    LBNA_Column, 1,
        LBNCA_Integer, &entry->file_count,  /* ✅ Pass POINTER to integer */
        LBNCA_Justification, LCJ_RIGHT,
    LBNA_Column, 2,
        LBNCA_Text, status_str,
    TAG_DONE);

/* Sort normally - integer columns sort numerically automatically! */
DoGadgetMethod((struct Gadget *)listbrowser_obj,
              window, NULL,
              LBM_SORT, list, column, direction, NULL);
```

**Common Mistake - Passing Value Instead of Pointer:**

```c
/* WRONG: Passing the value directly */
LBNCA_Integer, entry->file_count,  /* ❌ Treats value as memory address! */

/* If file_count = 64, ListBrowser tries to read from address 0x40 */
/* Result: Displays huge garbage numbers like -1576994805 */
```

**Why This Happens:**
- `LBNCA_Integer` expects a `LONG *` (pointer type)
- When you pass an integer value (e.g., `64`), it's interpreted as a memory address
- ListBrowser tries to dereference that "address" and reads garbage data
- Result: Displays random huge negative or positive numbers

**Requirements:**
1. The integer must be embedded in a persistent structure (not a local variable)
2. The structure must remain valid for the lifetime of the ListBrowser node
3. Use `&` to pass the address of the integer field

**Advantages:**
- ✅ Native numeric sorting without hooks
- ✅ No format limitations (displays full range of LONG)
- ✅ Clean display without leading zeros
- ✅ Simple and reliable
- ✅ No custom hooks needed

**Best for:** Any numeric column (file counts, sizes, IDs, dates as timestamps, etc.)

---

### Solution 2: Format Numbers with Leading Zeros (Alternative)

If you cannot guarantee the data structure will persist, or need backward compatibility with older OS versions, format numeric values with leading zeros:

**Advantages:**
- ✅ Simple and reliable
- ✅ No complex hook calling conventions
- ✅ Works with all compiler versions
- ✅ No risk of crashes or corruption
- ✅ Right-justified display looks correct

**Limitations:**
- Column width determined by format (e.g., `%04d` = max 9999)
- Display shows leading zeros (usually acceptable for counts)

**Best for:** Any numeric column (file counts, sizes, IDs, dates as timestamps, etc.)

---

### Solution 2: Format Numbers with Leading Zeros (Alternative)

If you cannot guarantee the data structure will persist, or need backward compatibility with older OS versions, format numeric values with leading zeros:

```c
/* Format file count with leading zeros (supports up to 9999) */
char file_count_str[16];
sprintf(file_count_str, "%04d", entry->file_count);

/* Create ListBrowser node */
lb_node = AllocListBrowserNode(3,
    LBNA_Column, 0,
        LBNCA_Text, tool_name,
    LBNA_Column, 1,
        LBNCA_CopyText, TRUE,
        LBNCA_Text, file_count_str,  /* "0003", "0012", "0100" */
        LBNCA_Justification, LCJ_RIGHT,
    TAG_DONE);
```

**Advantages:**
- ✅ Simple and reliable
- ✅ Works with all compiler versions
- ✅ No pointer lifetime concerns
- ✅ Right-justified display looks correct

**Limitations:**
- Column width determined by format (e.g., `%04d` = max 9999)
- Display shows leading zeros

**Best for:** Temporary data or when structure lifetime cannot be guaranteed

---

### Solution 3: Custom Compare Hook (NOT RECOMMENDED)

**WARNING:** Custom compare hooks are complex and the calling convention varies between OS versions and compilers. Use Solution 1 unless you absolutely need it.

The official AutoDoc states the hook receives `struct LBSortMsg`, but the actual implementation and calling convention is unreliable. Hook functions require precise register assignments (`__a0`, `__a1`, `__a2`) and the structure layout may not match documentation.

**Known Issues:**
- Structure field offsets don't match between doc and implementation
- Register calling conventions vary by compiler
- Easy to cause crashes with incorrect signatures
- Type fields return garbage values in practice

**If you must use a hook:**
1. Test extensively on your target OS version
2. Use leading zeros format as a fallback
3. Verify with actual Amiga hardware, not just emulation
4. Check AutoDocs specific to your OS version

**Reference:** See `docs/AutoDocs/LBM_SORT.doc` for official documentation, but be aware the structure definition may not match the actual implementation.

---

### Comparison of Approaches

| Approach | Complexity | Reliability | Display | Pointer Required | Recommended |
|----------|-----------|-------------|---------|------------------|-------------|
| **LBNCA_Integer** | Low | High | Clean | Yes (persistent) | ✅ **PRIMARY** |
| Leading zeros format | Low | High | Shows zeros | No | ✅ Alternative |
| Custom hook | Very High | Low | Clean | N/A | ❌ Avoid |

---

### Complete Working Example (LBNCA_Integer)

Based on `src/GUI/DefaultTools/tool_cache_window.c`:

### Complete Working Example (LBNCA_Integer)

Based on `src/GUI/DefaultTools/tool_cache_window.c`:

```c
/* Data structure with embedded integer (persists with entries) */
struct ToolCacheDisplayEntry
{
    struct Node node;
    struct Node filter_node;
    char *tool_name;
    int file_count;         /* This integer persists with the structure */
    BOOL exists;
    int cache_index;
};

/* In populate function */
static void populate_tool_listbrowser(struct iTidyToolCacheWindow *tool_data)
{
    struct ToolCacheDisplayEntry *entry;
    struct Node *node;
    struct Node *lb_node;
    char truncated_name[64];
    const char *status_str;
    
    /* Clear existing nodes */
    SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj,
                   tool_data->window, NULL,
                   LISTBROWSER_Labels, ~0,
                   TAG_DONE);
    
    /* Free old nodes */
    FreeListBrowserList(tool_data->tool_list_nodes);
    NewList(tool_data->tool_list_nodes);
    
    /* Add rows from filtered list */
    for (node = tool_data->filtered_entries.lh_Head; 
         node->ln_Succ != NULL; 
         node = node->ln_Succ)
    {
        entry = (struct ToolCacheDisplayEntry *)((char *)node - sizeof(struct Node));
        
        /* Truncate tool name if needed */
        iTidy_ShortenPathWithParentDir(entry->tool_name, truncated_name, 50);
        
        status_str = entry->exists ? "EXISTS" : "MISSING";
        
        /* Create node with integer column - PASS POINTER! */
        lb_node = AllocListBrowserNode(3,
            LBNA_Column, 0,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, truncated_name,
            LBNA_Column, 1,
                LBNCA_Integer, &entry->file_count,  /* ✅ Pointer to persistent field */
                LBNCA_Justification, LCJ_RIGHT,
            LBNA_Column, 2,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, (STRPTR)status_str,
            LBNA_UserData, (APTR)(ULONG)entry->cache_index,
            TAG_DONE);
        
        if (lb_node)
        {
            AddTail(tool_data->tool_list_nodes, lb_node);
        }
    }
    
    /* Reattach list to gadget */
    SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj,
                   tool_data->window, NULL,
                   LISTBROWSER_Labels, tool_data->tool_list_nodes,
                   LISTBROWSER_AutoFit, TRUE,
                   TAG_DONE);
}

/* In event handler - default sort works for integer columns! */
if (rel_event == LBRE_TITLECLICK)
{
    ULONG column = code;
    ULONG direction = LBMSORT_FORWARD;
    
    /* Default sort handles integer columns automatically */
    DoGadgetMethod((struct Gadget *)tool_data->tool_listbrowser_obj,
                  tool_data->window, NULL,
                  LBM_SORT, tool_data->tool_list_nodes, column, direction, NULL);
    
    SetGadgetAttrs((struct Gadget *)tool_data->tool_listbrowser_obj,
                   tool_data->window, NULL,
                   LISTBROWSER_SortColumn, column,
                   TAG_DONE);
}
```

**Result:** Files column displays clean numbers (3, 5, 8, 12, 100) and sorts numerically without any custom hooks!

---

### Complete Working Example (Leading Zeros - Alternative)

```c
/* In populate function */
char file_count_str[16];

/* Format with leading zeros for proper sorting */
sprintf(file_count_str, "%04d", entry->file_count);

/* Create node with formatted number */
lb_node = AllocListBrowserNode(3,
    LBNA_Column, 0,
        LBNCA_Text, "Tool Name",
    LBNA_Column, 1,
        LBNCA_CopyText, TRUE,
        LBNCA_Text, file_count_str,  /* Right-justified with leading zeros */
        LBNCA_Justification, LCJ_RIGHT,
    LBNA_Column, 2,
        LBNCA_Text, "Status",
    TAG_DONE);
```

**Result:** Files column sorts numerically: 0001, 0002, 0003, ..., 0100, 1000

---

### Historical Note (Custom Hook Attempts)

Previous attempts to implement custom compare hooks using the documented `struct LBSortMsg` interface failed due to:
1. Type fields (`lbsm_TypeA`, `lbsm_TypeB`) returning garbage values instead of expected 1/2/0
2. Data union fields not containing valid pointers
3. Register calling convention mismatches between documentation and implementation

The leading zeros approach proved to be the reliable, maintainable solution.

**DEPRECATED Implementation Pattern:**

**DEPRECATED Implementation Pattern:**

```c
/* This approach is unreliable - use leading zeros instead! */
#include <utility/hooks.h>
#include <gadgets/listbrowser.h>
#include <stdlib.h>

static LONG numeric_compare_hook_func(struct Hook *hook, APTR obj, struct LBSortMsg *msg)
{
    /* WARNING: msg structure fields don't match documentation in practice */
    /* Type fields return garbage, data pointers are invalid */
    /* This code is here for reference only - DO NOT USE */
    return 0;
}
```

**Why This Fails:**
- `lbsm_TypeA` and `lbsm_TypeB` return garbage values (not 1/2/0 as documented)
- Data union pointers are invalid or NULL
- Register calling conventions vary unpredictably
- Structure layout doesn't match AutoDoc specification

**Conclusion:** Use Solution 1 (leading zeros format) for all numeric columns.

---

## ListBrowser Column Sizing and AutoFit

### ⚠️ CRITICAL: LISTBROWSER_VirtualWidth Makes ci_Width a Pixel Percentage, Not a Screen Percentage

**Problem:** Adding `LISTBROWSER_HorizontalProp, TRUE` for horizontal scrolling is fine by itself, but adding `LISTBROWSER_VirtualWidth` to control the scrollable canvas size causes columns to become huge.

**Root cause (from AutoDoc):**
> *"If a virtual width is given, ci_Width will be a percentage of that virtual width."*

So if you set `LISTBROWSER_VirtualWidth, 800` and a column has `ci_Width = 35`, it becomes **280 pixels** wide — not 35% of the visible gadget. With three columns at 35/45/20%, the total is 800px and the listbrowser is essentially always scrolled even on a wide screen.

**Wrong (creates giant columns):**
```c
NewObject(ListBrowserClass, NULL,
    ...
    LISTBROWSER_HorizontalProp, TRUE,
    LISTBROWSER_VirtualWidth, 800,   /* ❌ Makes ci_Width a % of 800px */
    LISTBROWSER_ColumnInfo, col_info,
    TAG_DONE);
```

**Correct (AutoFit sizes to content, scroll appears only when needed):**
```c
NewObject(ListBrowserClass, NULL,
    ...
    LISTBROWSER_HorizontalProp, TRUE,   /* Enable horizontal scroll bar */
    LISTBROWSER_AutoFit,       TRUE,    /* Fit columns to content width */
    LISTBROWSER_ColumnInfo,    col_info,
    TAG_DONE);
```

`LISTBROWSER_AutoFit` calculates the required virtual width from the actual column content and only enables horizontal scrolling when the content is genuinely wider than the visible area. It is the correct companion to `LISTBROWSER_HorizontalProp`.

---

### ⚠️ CRITICAL: ColumnInfo Cannot Be Static or Shared

**From the AutoDoc:**
> *"ListBrowser may modify the contents of this structure. ColumnInfo can NOT be shared between multiple listbrowser gadgets or across window opens."*

`LISTBROWSER_AutoFit` writes back into the `ColumnInfo` array after it calculates column widths. If the array is `static`, those modified values persist the next time the window opens and AutoFit gets confused.

**Wrong (static array, values persist across opens):**
```c
static struct ColumnInfo col_info[4];  /* ❌ AutoFit values bleed into next open */

static struct ColumnInfo *make_column_info(void)
{
    col_info[0].ci_Title = "Type";   col_info[0].ci_Width = 35;
    col_info[1].ci_Title = "File";   col_info[1].ci_Width = 45;
    /* ... */
    return col_info;
}
```

**Correct (per-window storage, freshly initialised each time):**
```c
/* In your window struct */
typedef struct {
    Object *window_obj;
    /* ... other fields ... */
    struct ColumnInfo col_info[4];  /* Per-window — never static */
} MyWindow;

/* Init function writes into the struct */
static void init_column_info(struct ColumnInfo *ci)
{
    ci[0].ci_Title = "Type";    ci[0].ci_Width = 35; ci[0].ci_Flags = CIF_WEIGHTED;
    ci[1].ci_Title = "File";    ci[1].ci_Width = 45; ci[1].ci_Flags = CIF_WEIGHTED;
    ci[2].ci_Title = "Status";  ci[2].ci_Width = 20; ci[2].ci_Flags = CIF_WEIGHTED;
    ci[3].ci_Title = NULL;      ci[3].ci_Width = ~0; ci[3].ci_Flags = -1; /* Terminator */
}

/* Use it */
init_column_info(win->col_info);
/* ... */
NewObject(ListBrowserClass, NULL,
    LISTBROWSER_ColumnInfo, win->col_info,  /* ✅ Points to per-window buffer */
    LISTBROWSER_AutoFit,    TRUE,
    TAG_DONE);
```

---

### Re-apply AutoFit After Rebuilding the List

When you detach and reattach the list (e.g., in a `rebuild_list()` function), you must re-apply `LISTBROWSER_AutoFit` after reattaching. Otherwise the column widths are not recalculated for the new content.

```c
static void rebuild_list(MyWindow *win)
{
    /* 1. Detach the list */
    SetGadgetAttrs((struct Gadget *)win->listbrowser,
        win->window, NULL,
        LISTBROWSER_Labels, ~0,
        TAG_DONE);

    /* 2. Free old nodes and build new list */
    /* ... */

    /* 3. Reattach the list */
    SetGadgetAttrs((struct Gadget *)win->listbrowser,
        win->window, NULL,
        LISTBROWSER_Labels, win->lb_list,
        TAG_DONE);

    /* 4. Re-apply AutoFit so columns resize for the new content */
    SetGadgetAttrs((struct Gadget *)win->listbrowser,
        win->window, NULL,
        LISTBROWSER_AutoFit, TRUE,
        TAG_DONE);
}
```

> **Note:** Steps 3 and 4 can be combined into a single `SetGadgetAttrs()` call if you want to avoid any potential redraw flicker:
> ```c
> SetGadgetAttrs((struct Gadget *)win->listbrowser,
>     win->window, NULL,
>     LISTBROWSER_Labels,   win->lb_list,
>     LISTBROWSER_AutoFit,  TRUE,
>     TAG_DONE);
> ```

---

### Summary: Column Sizing Quick Reference

| Scenario | Tags to use |
|---|---|
| Fixed-width gadget, no scroll | `LISTBROWSER_ColumnInfo` with `CIF_WEIGHTED` (% of visible width) |
| Horizontal scroll, columns fit to content | `LISTBROWSER_HorizontalProp, TRUE` + `LISTBROWSER_AutoFit, TRUE` |
| Horizontal scroll, fixed pixel canvas | `LISTBROWSER_HorizontalProp, TRUE` + `LISTBROWSER_VirtualWidth, N` (ci_Width becomes % of N pixels) |
| Want columns to never overflow visible area | `CIF_WEIGHTED` only, no VirtualWidth, no HorizontalProp |

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

## ClickTab Gadget (Tabbed Interface)

### ⚠️ CRITICAL: Each Tab Node Must Have Unique TNA_Number

**Problem:** When using `clicktab.gadget` with `CLICKTAB_PageGroup`, clicking any tab always shows the same page (usually the first one). All tab clicks are treated as clicking tab 0.

**Cause:** When creating ClickTab nodes with `AllocClickTabNode()`, each node must have a unique `TNA_Number` attribute that corresponds to its page index in the PageGroup. If all nodes have `TNA_Number, 0` (or the same value), the ClickTab will always tell the PageGroup to show page 0.

**Symptom:** Tabs respond to clicks (you see the tab selection change visually), but the page content never changes. The first tab's content is always displayed.

**Wrong Pattern:**
```c
static struct List *make_clicktab_list(STRPTR *labels)
{
    struct List *list = AllocVec(sizeof(struct List), MEMF_CLEAR);
    if (list)
    {
        NewList(list);
        while (*labels)
        {
            /* ❌ WRONG: All tabs get TNA_Number = 0 */
            struct Node *node = AllocClickTabNode(
                TNA_Text, *labels,
                TNA_Number, 0,    /* All tabs have same number! */
                TAG_END);
            if (node)
                AddTail(list, node);
            labels++;
        }
    }
    return list;
}
```

**Correct Pattern:**
```c
static struct List *make_clicktab_list(STRPTR *labels)
{
    struct List *list = AllocVec(sizeof(struct List), MEMF_CLEAR);
    if (list)
    {
        WORD tab_number = 0;  /* Counter for tab numbers */
        NewList(list);
        while (*labels)
        {
            /* ✅ CORRECT: Each tab gets unique number */
            struct Node *node = AllocClickTabNode(
                TNA_Text, *labels,
                TNA_Number, tab_number,  /* Unique number for each tab */
                TAG_END);
            if (node)
                AddTail(list, node);
            labels++;
            tab_number++;  /* Increment for next tab */
        }
    }
    return list;
}
```

**How It Works:**
- Tab 0 "Layout" → `TNA_Number = 0` → Shows `PAGE_Current = 0`
- Tab 1 "Density" → `TNA_Number = 1` → Shows `PAGE_Current = 1`
- Tab 2 "Limits" → `TNA_Number = 2` → Shows `PAGE_Current = 2`
- Tab 3 "Options" → `TNA_Number = 3` → Shows `PAGE_Current = 3`

When a tab is clicked, the ClickTab gadget reads that node's `TNA_Number` and automatically sets the PageGroup's `PAGE_Current` attribute to that value (when `CLICKTAB_PageGroup` is specified during window creation).

**Key Points:**
1. `TNA_Number` must start at 0 and increment sequentially
2. The numbers must match the order of `PAGE_Add` calls in the PageGroup
3. Tab switching is automatic when `CLICKTAB_PageGroup` is used - no manual event handling needed
4. The page order in PageObject must match the tab order in ClickTab labels

**References:**
- AutoDocs: `docs/AutoDocs/clicktab_gc.doc` - See `TNA_Number` attribute
- AutoDocs: `docs/AutoDocs/layout_gc.doc` - See `PAGE_Current` attribute and PageObject

---

## Gadget Tooltips (HintInfo)

### ⚠️ CRITICAL: HintInfo Array Must Be Static

**Problem:** Tooltips don't appear when hovering over gadgets, even though `WINDOW_GadgetHelp` is set to TRUE.

**Cause:** The `HintInfo` array is declared on the stack and becomes invalid after the window creation function returns.

**Symptom:** No tooltips appear when hovering. No crash, just silent failure.

### Required Components for Tooltips

To enable tooltips in ReAction windows, you need **THREE** things:

1. **Static HintInfo array** with gadget hints
2. **WINDOW_HintInfo tag** in window creation
3. **IDCMP_MENUHELP flag** in IDCMP flags
4. **WINDOW_GadgetHelp, TRUE** (enables the system)

### Wrong Pattern (Non-Static Array)

```c
BOOL open_window(struct WindowData *win_data)
{
    /* ❌ WRONG: Local array disappears after function returns */
    struct HintInfo hintInfo[] =
    {
        {GID_BUTTON1, -1, "Click to do something", 0},
        {GID_BUTTON2, -1, "Click to cancel", 0},
        {-1, -1, NULL, 0}
    };
    
    win_data->window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title, "My Window",
        WINDOW_HintInfo, hintInfo,  /* ❌ Points to stack memory! */
        WINDOW_GadgetHelp, TRUE,
        WA_IDCMP, IDCMP_GADGETUP | IDCMP_MENUHELP,  /* MENUHELP is correct */
    TAG_END);
    
    /* Array 'hintInfo' is destroyed here when function returns */
    return TRUE;
}
```

### Correct Pattern (Static Array)

```c
BOOL open_window(struct WindowData *win_data)
{
    /* ✅ CORRECT: Static array persists after function returns */
    static struct HintInfo hintInfo[] =
    {
        {GID_MASTER_LAYOUT, -1, "", 0},  /* Empty hint for layout containers */
        {GID_FOLDER_GETFILE, -1, "Select the folder to process.", 0},
        {GID_ORDER_CHOOSER, -1, "Sets how icons are grouped and sorted.", 0},
        {GID_RECURSIVE_CHECKBOX, -1, "If enabled, processes subfolders.", 0},
        {GID_BACKUP_CHECKBOX, -1, "If enabled, creates LhA backup.", 0},
        {GID_START_BUTTON, -1, "Click to begin processing.", 0},
        {GID_EXIT_BUTTON, -1, "Exits the application.", 0},
        {-1, -1, NULL, 0}  /* Terminator */
    };
    
    win_data->window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title, "My Window",
        /* ... other window tags ... */
        WINDOW_HintInfo, hintInfo,      /* ✅ Points to static memory */
        WINDOW_GadgetHelp, TRUE,        /* ✅ Enables gadget help */
        WA_IDCMP, IDCMP_GADGETUP | IDCMP_GADGETDOWN | 
                  IDCMP_CLOSEWINDOW | IDCMP_MENUHELP,  /* ✅ MENUHELP required */
    TAG_END);
    
    return TRUE;
}
```

### HintInfo Structure Format

```c
struct HintInfo {
    LONG   hintID;    /* Gadget GA_ID value */
    LONG   hintCol;   /* Column number (-1 for single-column hints) */
    STRPTR hintText;  /* Tooltip text (can be NULL or "") */
    LONG   hintFlags; /* Reserved, set to 0 */
};
```

### Key Points

1. **Always declare HintInfo array as `static`** - it must persist after the function returns
2. **Always include `IDCMP_MENUHELP`** in the IDCMP flags - tooltips won't work without it
3. **Set `WINDOW_GadgetHelp, TRUE`** - enables the system
4. **Empty hints are OK** - use `""` for layout containers that don't need tooltips
5. **Use `\n` for multi-line tooltips** - improves readability for long descriptions
6. **Terminate array with `{-1, -1, NULL, 0}`** - signals end of hints list
7. **Use actual GA_ID values** - not array indices

### Multi-Line Tooltip Example

```c
static struct HintInfo hintInfo[] =
{
    {GID_ADVANCED_BUTTON, -1, 
     "Opens the Advanced Settings window for more\ndetailed control over layout options.", 0},
    {GID_RESTORE_BUTTON, -1, 
     "Restores icon positions and drawer layout from LhA\nbackup archives. Only available if you previously\nran with backups enabled.", 0},
    {-1, -1, NULL, 0}
};
```

### Why This Is Easy to Miss

1. **No compiler warning** - the address is valid at window creation time
2. **No crash** - the memory is just stale, not necessarily overwritten immediately
3. **Silent failure** - tooltips simply don't appear
4. **Testing may work initially** - stack memory may contain valid data temporarily

### Complete Working Example

```c
/* From iTidy main_window.c */
BOOL open_itidy_main_window(struct iTidyMainWindow *win_data)
{
    /* ... setup code ... */
    
    /* ✅ Static array persists */
    static struct HintInfo hintInfo[] =
    {
        {ITIDY_GAID_MASTER_LAYOUT, -1, "", 0},
        {ITIDY_GAID_FOLDER_GETFILE, -1, 
         "Select the folder for iTidy to clean up. To cleanup subfolders,\n"
         "make sure you select 'Cleanup subfolders' below.", 0},
        {ITIDY_GAID_ORDER_CHOOSER, -1, 
         "Sets how icons are grouped and sorted in the window. The choices\n"
         "are Folders First, Files First, Mixed, or Grouped by Type.", 0},
        {ITIDY_GAID_RECURSIVE_CHECKBOX, -1, 
         "If enabled, iTidy will recurse through all subfolders under the selected path.", 0},
        {ITIDY_GAID_START_BUTTON, -1, 
         "Click 'Start' to begin the iTidy sweep once you have selected the folder\n"
         "and any other setting required.", 0},
        {ITIDY_GAID_EXIT_BUTTON, -1, "Exits iTidy.", 0},
        {-1, -1, NULL, 0}
    };
    
    win_data->window_obj = NewObject(WINDOW_GetClass(), NULL,
        WA_Title, ITIDY_WINDOW_TITLE,
        WA_Left, 50,
        WA_Top, 30,
        WA_MinWidth, 350,
        WA_MinHeight, 200,
        WA_CloseGadget, TRUE,
        WA_DragBar, TRUE,
        WA_Activate, TRUE,
        WINDOW_HintInfo, hintInfo,     /* ✅ */
        WINDOW_GadgetHelp, TRUE,       /* ✅ */
        WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | 
                  IDCMP_CLOSEWINDOW | IDCMP_MENUHELP,  /* ✅ MENUHELP! */
        WINDOW_ParentGroup, /* ... gadgets ... */
    TAG_END);
    
    return TRUE;
}
```

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

- **2026-03-03:** Added Busy Pointer and IDCMP Flushing section
  - CRITICAL: WA_BusyPointer only changes the cursor -- it does NOT block or discard IDCMP events
  - Stale clicks/close events queue on the parent's UserPort while busy and fire when the event loop resumes
  - Solution: Flush all pending IDCMP messages with GetMsg()/ReplyMsg() loop when clearing the busy pointer
  - Implemented centrally in safe_set_window_pointer() so all call sites benefit automatically
  - Standard pattern: set busy before child open, clear after child close (covers success and error paths)
  - Parent stays busy for full child window lifetime for clear modal-like behaviour
- **2026-02-19:** Added ListBrowser Column Sizing and AutoFit section
  - CRITICAL: `LISTBROWSER_VirtualWidth` makes `ci_Width` a percentage of the virtual pixel canvas, not the visible gadget — columns become giant
  - Use `LISTBROWSER_AutoFit, TRUE` as the correct companion to `LISTBROWSER_HorizontalProp, TRUE`
  - CRITICAL: `ColumnInfo` arrays CANNOT be `static` or shared — ListBrowser writes back into the array; use per-window storage initialised fresh each open
  - `LISTBROWSER_AutoFit` must be re-applied after every `rebuild_list()` reattach so columns resize to new content
  - Summary table: which tag combination to use for each scrolling/sizing scenario
- **2026-02-03:** Added gadget tooltips (HintInfo) section
  - CRITICAL: HintInfo array must be declared `static` or tooltips won't work
  - Must include IDCMP_MENUHELP flag in addition to WINDOW_GadgetHelp, TRUE
  - HintInfo structure format and proper usage
  - Multi-line tooltip examples
  - Complete working example from main_window.c
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
