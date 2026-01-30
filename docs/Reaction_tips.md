# ReAction Tips and Lessons Learned

This document captures hard-won lessons from implementing ReAction GUI components in iTidy. **AI Agents: Read this before implementing any ReAction code!**

---

## Table of Contents
1. [ListBrowser Hierarchical Tree View](#listbrowser-hierarchical-tree-view)
2. [General ReAction Gotchas](#general-reaction-gotchas)
3. [Memory and Type Safety](#memory-and-type-safety)

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

- **2026-01-30:** Initial document created after debugging hierarchical ListBrowser issues
  - ULONG vs WORD bug for GetListBrowserNodeAttrs
  - List population order (before gadget creation)
  - Generation numbers are 0-based
  - LBNCA_CopyText tag order
