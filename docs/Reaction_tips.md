# ReAction Tips and Lessons Learned

This document captures hard-won lessons from implementing ReAction GUI components in iTidy. **AI Agents: Read this before implementing any ReAction code!**

---

## Table of Contents
1. [Event Loop Patterns](#event-loop-patterns)
2. [ListBrowser Hierarchical Tree View](#listbrowser-hierarchical-tree-view)
3. [Dynamic Text Labels](#dynamic-text-labels)
4. [GetFile Gadget (File/Folder Selection)](#getfile-gadget-filefolder-selection)
5. [General ReAction Gotchas](#general-reaction-gotchas)
6. [Memory and Type Safety](#memory-and-type-safety)

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
