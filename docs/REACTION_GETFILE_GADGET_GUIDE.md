# ReAction GetFile Gadget - Implementation Guide for AI Agents

**Last Updated**: January 23, 2026  
**Context**: iTidy ReAction GUI Migration (v2.0+)  
**Target**: AmigaOS Workbench 3.2+ with ReAction classes

---

## Critical: Read This First

This guide documents **hard-learned lessons** from implementing the `getfile.gadget` for folder/file selection in ReAction-based windows. The information here will save hours of debugging and prevent system crashes.

**If you're implementing file/folder selection in a ReAction window, follow this guide exactly.**

---

## Table of Contents

1. [Overview](#overview)
2. [GadTools to ReAction Migration Issues](#gadtools-to-reaction-migration-issues)
3. [Critical Lessons Learned - GetFile Gadget](#critical-lessons-learned---getfile-gadget)
4. [Correct Implementation Pattern](#correct-implementation-pattern)
5. [Common Mistakes and How to Avoid Them](#common-mistakes-and-how-to-avoid-them)
6. [Debugging Tips](#debugging-tips)
7. [Reference Implementation](#reference-implementation)

---

## Overview

The `getfile.gadget` is a ReAction BOOPSI class that provides file/folder selection through the standard AmigaOS ASL file requester. It consists of a display field (either read-only button or editable string) and a popup button that opens the requester.

**Key Points:**
- ReAction class: `getfile.gadget`
- AutoDocs location: `c:\Amiga\temp\getfile.txt` (attached to this project)
- NOT the same as ASL file requesters (uses them internally)
- Requires manual method invocation to open the requester

---

## GadTools to ReAction Migration Issues

**If you're converting an existing GadTools window to ReAction, read this section first.**

This section documents all critical issues encountered when migrating iTidy's main_window.c from GadTools (Workbench 3.0/3.1) to ReAction (Workbench 3.2+). These lessons apply to any window conversion.

### Issue #1: Library Base Name Collision

**Problem:**
```c
struct Library *GadToolsBase = NULL;  // WRONG - Collides with system global!
```

**Symptoms:**
- Compilation succeeds
- Random crashes or undefined behavior at runtime
- GadTools menus may not work correctly

**Solution:**
Use a prefixed name to avoid collision with the system's global `GadToolsBase`:
```c
static struct Library *iTidy_GadToolsBase = NULL;  // Correct - local prefix

// In init function
iTidy_GadToolsBase = OpenLibrary("gadtools.library", 39L);

// Use consistently throughout
if (iTidy_GadToolsBase)
{
    CreateMenus(...);  // GadTools functions still work
}
```

**Why:** AmigaOS system libraries may define global base pointers. Always prefix your library bases to avoid collisions.

---

### Issue #2: Missing BOOPSI Function Declarations

**Problem:**
```c
NewList(list);     // error: implicit declaration
DoMethod(obj, ...); // error: implicit declaration
```

**Solution:**
Add the required header:
```c
#include <clib/alib_protos.h>  // For NewList, DoMethod, etc.
```

**Additional required headers for ReAction:**
```c
/* ReAction class protos */
#include <proto/window.h>
#include <proto/layout.h>
#include <proto/button.h>
#include <proto/checkbox.h>
#include <proto/chooser.h>
#include <proto/getfile.h>
#include <proto/label.h>

/* ReAction class definitions */
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/checkbox.h>
#include <gadgets/chooser.h>
#include <gadgets/getfile.h>
#include <images/label.h>

/* ReAction macros and structures */
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
```

---

### Issue #3: Duplicate External Declarations

**Problem:**
Multiple source files declare the same extern variables, causing linker errors or warnings.

**Example:**
```c
// In window.c
extern ToolCacheEntry *g_ToolCache;  

// In icon_types.h (already declared)
extern ToolCacheEntry *g_ToolCache;  // Duplicate!
```

**Solution:**
Include the header that already has the extern declaration instead of declaring it again:
```c
// In window.c
#include "../icon_types.h"  // Contains g_ToolCache extern

// Remove duplicate: extern ToolCacheEntry *g_ToolCache;
```

**Best Practice:** Declare externs in ONE header file, include that header everywhere needed.

---

### Issue #4: ReAction Library Initialization

**GadTools (old way):**
```c
GadToolsBase = OpenLibrary("gadtools.library", 37L);
```

**ReAction (new way):**
```c
BOOL init_reaction_libs(void)
{
    /* Open ALL ReAction class libraries */
    WindowBase = OpenLibrary("window.class", 0L);
    LayoutBase = OpenLibrary("gadgets/layout.gadget", 0L);
    ButtonBase = OpenLibrary("gadgets/button.gadget", 0L);
    CheckBoxBase = OpenLibrary("gadgets/checkbox.gadget", 0L);
    ChooserBase = OpenLibrary("gadgets/chooser.gadget", 0L);
    GetFileBase = OpenLibrary("gadgets/getfile.gadget", 0L);
    LabelBase = OpenLibrary("images/label.image", 0L);
    
    /* GadTools STILL needed for menus */
    iTidy_GadToolsBase = OpenLibrary("gadtools.library", 39L);
    
    /* Check all libraries and cleanup on failure */
    if (!WindowBase || !LayoutBase || /* ... */)
    {
        cleanup_reaction_libs();
        return FALSE;
    }
    
    return TRUE;
}
```

**Critical:** 
- ReAction uses separate libraries for each gadget class
- Each library must be opened before creating gadgets of that type
- GadTools is still needed if you use GadTools menus
- Must close all libraries in reverse order on cleanup

---

### Issue #5: Window Creation - Coordinate Positioning vs Layout Groups

**GadTools (old way):**
Manual coordinate calculation:
```c
ng.ng_LeftEdge = 10;
ng.ng_TopEdge = border_top + 5;
ng.ng_Width = 200;
ng.ng_Height = 14;
gad = CreateGadget(BUTTON_KIND, gad, &ng, GT_Text, "Click Me", TAG_DONE);
```

**ReAction (new way):**
Automatic layout using nested groups:
```c
win_data->window_obj = NewObject(WINDOW_GetClass(), NULL,
    WA_Title, "My Window",
    WA_CloseGadget, TRUE,
    
    WINDOW_ParentGroup, NewObject(LAYOUT_GetClass(), NULL,
        LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
        LAYOUT_SpaceOuter, TRUE,
        
        LAYOUT_AddChild, NewObject(BUTTON_GetClass(), NULL,
            GA_ID, BUTTON_ID,
            GA_Text, "Click Me",
            GA_RelVerify, TRUE,
        TAG_END),
        
    TAG_END),
TAG_END);

/* Open the window */
win_data->window = (struct Window *)RA_OpenWindow(win_data->window_obj);
```

**Key Differences:**
- No manual coordinate calculation
- Uses nested BOOPSI objects (window contains layout contains gadgets)
- Layout gadgets automatically position children
- Must keep window object pointer for RA_OpenWindow()
- `LAYOUT_AddChild` for each gadget
- Use `LAYOUT_Orientation` (VERT/HORIZ) to control flow

---

### Issue #6: Chooser Gadgets - Label List Creation

**Problem:**
Choosers need label lists, not simple string arrays.

**Wrong:**
```c
STRPTR labels[] = {"Option 1", "Option 2", NULL};
CHOOSER_Labels, labels,  // WRONG - needs proper list structure
```

**Correct:**
```c
/* 1. Create label list from string array */
static struct List *create_chooser_labels(STRPTR *strings)
{
    struct List *list;
    struct Node *node;
    
    list = (struct List *)AllocMem(sizeof(struct List), MEMF_PUBLIC | MEMF_CLEAR);
    if (!list)
        return NULL;
    
    NewList(list);
    
    while (*strings)
    {
        node = AllocChooserNode(CNA_Text, *strings, TAG_END);
        if (node)
        {
            AddTail(list, node);
        }
        strings++;
    }
    
    return list;
}

/* 2. Use in gadget creation */
STRPTR options_str[] = {"Option 1", "Option 2", "Option 3", NULL};
struct List *options_list = create_chooser_labels(options_str);

LAYOUT_AddChild, NewObject(CHOOSER_GetClass(), NULL,
    GA_ID, CHOOSER_ID,
    CHOOSER_Labels, options_list,  // Pass the list
    CHOOSER_Selected, 0,
    CHOOSER_PopUp, TRUE,
TAG_END),

/* 3. Free the list on cleanup */
static void free_chooser_labels(struct List *list)
{
    struct Node *node;
    struct Node *next;
    
    if (!list)
        return;
    
    node = list->lh_Head;
    while ((next = node->ln_Succ))
    {
        FreeChooserNode(node);
        node = next;
    }
    
    FreeMem(list, sizeof(struct List));
}
```

---

### Issue #7: Event Handling - Message Processing

**GadTools (old way):**
```c
struct IntuiMessage *msg;

while (msg = GT_GetIMsg(window->UserPort))
{
    class = msg->Class;
    code = msg->Code;
    gadget = (struct Gadget *)msg->IAddress;
    
    GT_ReplyIMsg(msg);
    
    switch (class)
    {
        case IDCMP_GADGETUP:
            /* Handle gadget->GadgetID */
            break;
        case IDCMP_CLOSEWINDOW:
            done = TRUE;
            break;
    }
}
```

**ReAction (new way):**
```c
ULONG result;
WORD code;

while ((result = RA_HandleInput(window_obj, &code)) != WMHI_LASTMSG)
{
    switch (result & WMHI_CLASSMASK)
    {
        case WMHI_CLOSEWINDOW:
            done = TRUE;
            break;
        
        case WMHI_GADGETUP:
            {
                ULONG gadget_id = result & WMHI_GADGETMASK;
                handle_gadget_event(gadget_id, code);
            }
            break;
        
        case WMHI_MENUPICK:
            handle_menu_selection(code);
            break;
    }
}
```

**Key Differences:**
- Use `RA_HandleInput()` instead of `GT_GetIMsg()`
- No manual message reply needed (RA_HandleInput does it)
- Use `WMHI_*` constants instead of `IDCMP_*` for event classes
- Extract gadget ID with `result & WMHI_GADGETMASK`
- Extract event class with `result & WMHI_CLASSMASK`
- Code parameter passed separately to RA_HandleInput

---

### Issue #8: IDCMP Flags Differences

**GadTools:**
```c
IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | IDCMP_MENUPICK
```

**ReAction:**
```c
WA_IDCMP, IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_CLOSEWINDOW | 
          IDCMP_MENUPICK | IDCMP_NEWSIZE | IDCMP_IDCMPUPDATE,
```

**Additional flags often needed:**
- `IDCMP_GADGETDOWN` - For immediate feedback on gadget press
- `IDCMP_IDCMPUPDATE` - For attribute change notifications (especially with getfile.gadget in ReadOnly=FALSE mode)
- `IDCMP_NEWSIZE` - Window resizing events

---

### Issue #9: Menus Still Use GadTools

**Important:** ReAction windows use **GadTools menus**, not ReAction menus.

```c
/* Create menus with GadTools */
win_data->visual_info = GetVisualInfo(screen, TAG_END);
win_data->menu_strip = CreateMenus(menu_template, TAG_END);
LayoutMenus(win_data->menu_strip, win_data->visual_info,
    GTMN_NewLookMenus, TRUE,
    TAG_END);

/* Attach to ReAction window */
if (win_data->window)
{
    SetMenuStrip(win_data->window, win_data->menu_strip);
}

/* Menu events come through WMHI_MENUPICK */
case WMHI_MENUPICK:
    {
        struct MenuItem *item;
        ULONG menu_number = code;
        
        while (menu_number != MENUNULL)
        {
            item = ItemAddress(menu_strip, menu_number);
            if (item)
            {
                ULONG item_id = (ULONG)GTMENUITEM_USERDATA(item);
                /* Handle menu selection */
            }
            menu_number = item->NextSelect;
        }
    }
    break;
```

---

### Issue #10: Window and Gadget Cleanup Order

**Critical:** ReAction cleanup must happen in reverse order of creation.

**Correct cleanup sequence:**
```c
void close_window(struct WindowData *win_data)
{
    /* 1. Clear menus from window */
    if (win_data->window && win_data->menu_strip)
    {
        ClearMenuStrip(win_data->window);
    }
    
    /* 2. Close/dispose window object (automatically disposes child gadgets) */
    if (win_data->window_obj)
    {
        DisposeObject(win_data->window_obj);
        win_data->window_obj = NULL;
        win_data->window = NULL;  // Window pointer invalid after dispose
    }
    
    /* 3. Free GadTools menus */
    if (win_data->menu_strip)
    {
        FreeMenus(win_data->menu_strip);
        win_data->menu_strip = NULL;
    }
    
    /* 4. Free visual info */
    if (win_data->visual_info)
    {
        FreeVisualInfo(win_data->visual_info);
        win_data->visual_info = NULL;
    }
    
    /* 5. Free chooser label lists */
    if (win_data->chooser_labels)
    {
        free_chooser_labels(win_data->chooser_labels);
        win_data->chooser_labels = NULL;
    }
    
    /* 6. Delete message port */
    if (win_data->app_port)
    {
        DeleteMsgPort(win_data->app_port);
        win_data->app_port = NULL;
    }
    
    /* 7. Unlock screen */
    if (win_data->screen)
    {
        UnlockPubScreen(NULL, win_data->screen);
        win_data->screen = NULL;
    }
}
```

**Wrong order causes:**
- Memory leaks
- System crashes
- Guru meditations

---

### Issue #11: Getting Gadget Attributes

**GadTools:**
```c
LONG value;
GT_GetGadgetAttrs(gadget, window, NULL,
    GTCB_Checked, &value,  // For checkbox
    TAG_DONE);
```

**ReAction:**
```c
ULONG value;
GetAttr(GA_Selected, gadget_object, &value);  // For checkbox

/* Or for other attributes */
STRPTR text;
GetAttr(STRINGA_TextVal, string_gadget, (ULONG *)&text);
```

**Key Differences:**
- Use `GetAttr()` instead of `GT_GetGadgetAttrs()`
- Pass gadget object, not struct Gadget pointer
- Use generic BOOPSI attributes (GA_Selected) or class-specific attributes
- Single attribute per call (not multiple tags)

---

### Issue #12: Setting Gadget Attributes

**GadTools:**
```c
GT_SetGadgetAttrs(gadget, window, NULL,
    GTCB_Checked, TRUE,
    GA_Disabled, FALSE,
    TAG_DONE);
```

**ReAction:**
```c
SetGadgetAttrs((struct Gadget *)gadget_object,
              window, NULL,
              GA_Selected, TRUE,
              GA_Disabled, FALSE,
              TAG_END);
```

**Important:**
- Cast to `(struct Gadget *)` when calling SetGadgetAttrs
- Can set multiple attributes in one call
- Use `TAG_END` not `TAG_DONE`
- Window parameter is needed for immediate refresh

---

### Issue #13: Label Gadgets

**GadTools:**
Text labels are not gadgets, just text drawn with `DrawBevelBox` or similar.

**ReAction:**
Labels are actual image objects:
```c
CHILD_Label, NewObject(LABEL_GetClass(), NULL,
    LABEL_Text, "My Label",
TAG_END),
```

**Use cases:**
- `CHILD_Label` - Associates label with the following gadget
- Labels automatically align with gadgets
- No manual text positioning needed

---

### Issue #14: Button Gadget Differences

**GadTools:**
```c
gad = CreateGadget(BUTTON_KIND, gad, &ng,
    GT_Text, "Click Me",
    GA_Immediate, TRUE,
    TAG_DONE);
```

**ReAction:**
```c
LAYOUT_AddChild, NewObject(BUTTON_GetClass(), NULL,
    GA_ID, BUTTON_ID,
    GA_Text, "Click Me",
    GA_RelVerify, TRUE,
    GA_TabCycle, TRUE,  // Enable keyboard navigation
TAG_END),
```

**Attribute mapping:**
- `GT_Text` → `GA_Text`
- `GA_Immediate` → `GA_RelVerify, TRUE` (for GADGETUP events)
- Add `GA_TabCycle, TRUE` for keyboard navigation

---

### Migration Checklist

When converting a GadTools window to ReAction:

- [ ] Rename `GadToolsBase` to prefixed name (e.g., `iTidy_GadToolsBase`)
- [ ] Add `#include <clib/alib_protos.h>`
- [ ] Add all ReAction proto headers (`<proto/window.h>`, etc.)
- [ ] Add all ReAction class headers (`<classes/window.h>`, etc.)
- [ ] Remove duplicate extern declarations (use shared headers)
- [ ] Create `init_reaction_libs()` and `cleanup_reaction_libs()` functions
- [ ] Open all required ReAction class libraries
- [ ] Convert coordinate-based gadget creation to layout groups
- [ ] Replace `CreateGadget()` calls with `NewObject()` calls
- [ ] Create chooser label lists with `AllocChooserNode()`
- [ ] Replace `GT_GetIMsg()` with `RA_HandleInput()`
- [ ] Update IDCMP flags to include ReAction-specific flags
- [ ] Change event handling from `IDCMP_*` to `WMHI_*` constants
- [ ] Extract gadget ID with `result & WMHI_GADGETMASK`
- [ ] Replace `GT_GetGadgetAttrs()` with `GetAttr()`
- [ ] Keep GadTools menu code (CreateMenus, LayoutMenus)
- [ ] Update cleanup sequence to proper ReAction order
- [ ] Free chooser label lists in cleanup
- [ ] Call `DisposeObject()` on window object (not CloseWindow)
- [ ] Test all gadget interactions thoroughly

---

## Critical Lessons Learned - GetFile Gadget

### ❌ Mistake #1: Using GETFILE_FullFile for Folder-Only Selection

**Wrong:**
```c
GETFILE_FullFile, path_buffer,    // WRONG for folders!
GETFILE_DrawersOnly, TRUE,
```

**Correct:**
```c
GETFILE_Drawer, path_buffer,      // Correct for folders
GETFILE_DrawersOnly, TRUE,
```

**Why:** 
- `GETFILE_FullFile` is for complete file paths (drawer + filename)
- `GETFILE_Drawer` is specifically for folder/drawer paths
- When `DrawersOnly = TRUE`, you MUST use `GETFILE_Drawer`

---

### ❌ Mistake #2: Expecting Automatic Requester Opening

**The Problem:**
The gadget **does not automatically** open the ASL requester when clicked. You must explicitly invoke the `GFILE_REQUEST` method in your event handler.

**Symptoms:**
- Button appears and can be clicked
- GADGETUP events are received
- Nothing happens - no requester opens
- Reading `GETFILE_Drawer` just returns the initial value

**Solution:**
You must call `DoMethod()` with the `GFILE_REQUEST` method when the gadget is clicked.

---

### ❌ Mistake #3: Using Wrong Method Invocation Functions

**Wrong (causes system crash):**
```c
DoGadgetMethod((struct Gadget *)gadget, ...)  // CRASH!
```

**Wrong (doesn't exist in WB 3.2 SDK):**
```c
IDoMethod((Object *)gadget, ...)              // Undefined symbol error
```

**Correct:**
```c
DoMethod((Object *)gadget, ...)               // From alib_protos.h - CORRECT
```

**Why:**
- ReAction gadgets are BOOPSI objects, not traditional Intuition gadgets
- `DoGadgetMethod()` is for GadTools gadgets and causes crashes with ReAction
- `IDoMethod()` is an AmigaOS 4+ function not available in SDK 3.2
- `DoMethod()` from `<clib/alib_protos.h>` is the correct function for WB 3.2

---

### ❌ Mistake #4: Wrong IDCMP Flags

**The gadget needs:**
```c
WA_IDCMP, IDCMP_GADGETUP | IDCMP_IDCMPUPDATE | /* other flags */
```

**Why:**
- `IDCMP_GADGETUP` - To detect button clicks
- `IDCMP_IDCMPUPDATE` - For attribute change notifications (optional but recommended)

---

### ❌ Mistake #5: Confusing ReadOnly Modes

**Two modes exist:**

**Mode 1: ReadOnly = TRUE (Recommended for folder selection)**
- Display field is a read-only button showing the path
- User can only select via the popup button
- Simpler to implement
- Less confusing for users

**Mode 2: ReadOnly = FALSE**
- Display field is an editable string gadget
- User can type paths OR use the popup button
- More complex to implement
- Requires watching `IDCMP_IDCMPUPDATE` messages
- Code in message will equal GadgetID when popup button clicked

**For folder selection: Use ReadOnly = TRUE**

---

## Correct Implementation Pattern

### Step 1: Gadget Creation

```c
/* In window creation code */
LAYOUT_AddChild, win_data->gadgets[GAD_IDX_FOLDER] = 
    NewObject(GETFILE_GetClass(), NULL,
    GA_ID, GAID_FOLDER,
    GA_RelVerify, TRUE,
    GETFILE_TitleText, "Select folder to tidy",
    GETFILE_Drawer, initial_path_buffer,      // Use Drawer, not FullFile!
    GETFILE_DoSaveMode, FALSE,                 // We're opening, not saving
    GETFILE_DrawersOnly, TRUE,                 // Folders only
    GETFILE_ReadOnly, TRUE,                    // Read-only button mode
TAG_END),
CHILD_Label, NewObject(LABEL_GetClass(), NULL,
    LABEL_Text, "Folder",
TAG_END),
```

**Key attributes explained:**
- `GETFILE_Drawer` - Initial folder path and receives selected folder
- `GETFILE_DoSaveMode, FALSE` - Set TRUE only if this is a "Save As" dialog
- `GETFILE_DrawersOnly, TRUE` - Show folders only, no files
- `GETFILE_ReadOnly, TRUE` - Make it a button, not an editable field

---

### Step 2: Event Handler

```c
case GAID_FOLDER:
    /* Invoke the file requester using GFILE_REQUEST method */
    if (DoMethod((Object *)win_data->gadgets[GAD_IDX_FOLDER],
                 GFILE_REQUEST, win_data->window))
    {
        /* Read the selected path */
        STRPTR drawer = NULL;
        
        GetAttr(GETFILE_Drawer, win_data->gadgets[GAD_IDX_FOLDER], 
               (ULONG *)&drawer);
               
        if (drawer && drawer[0])
        {
            strncpy(path_buffer, drawer, sizeof(path_buffer) - 1);
            path_buffer[sizeof(path_buffer) - 1] = '\0';
            log_info(LOG_GUI, "Folder selected: %s\n", path_buffer);
        }
    }
    /* If DoMethod returns 0, user cancelled the requester */
    break;
```

**Critical points:**
- **Must cast to `(Object *)`** - ReAction gadgets are BOOPSI objects
- **Pass `win_data->window`** as the requester parent window
- **Check return value** - 0 means cancelled, non-zero means folder selected
- **Read `GETFILE_Drawer` after**  - Don't read it before invoking GFILE_REQUEST

---

### Step 3: Updating the Display

If you programmatically change the folder path (e.g., loading preferences):

```c
SetGadgetAttrs((struct Gadget *)win_data->gadgets[GAD_IDX_FOLDER],
              win_data->window, NULL,
              GETFILE_Drawer, new_path,
              TAG_END);
```

**Important:** Use `GETFILE_Drawer` consistently - same attribute for reading and writing.

---

## Common Mistakes and How to Avoid Them

### Mistake: Gadget doesn't respond to clicks

**Check:**
1. Is `GA_RelVerify, TRUE` set?
2. Is the gadget ID in your event handler switch statement?
3. Is `IDCMP_GADGETUP` in the window's IDCMP flags?
4. Are you calling `DoMethod()` with `GFILE_REQUEST`?

### Mistake: System crashes when clicking button

**Likely causes:**
1. Using `DoGadgetMethod()` instead of `DoMethod()`
2. Not casting to `(Object *)` in DoMethod call
3. Passing wrong parameters to GFILE_REQUEST

### Mistake: Requester opens but folder isn't saved

**Check:**
1. Are you checking the return value of DoMethod()?
2. Are you reading `GETFILE_Drawer` AFTER the DoMethod call?
3. Are you reading the correct attribute (`GETFILE_Drawer`, not `GETFILE_FullFile`)?

### Mistake: Initial path doesn't display

**Check:**
1. Did you set `GETFILE_Drawer` during gadget creation?
2. Is the initial path valid and null-terminated?
3. Does the path actually exist? (ASL may reject invalid paths)

---

## Debugging Tips

### Add Temporary Logging

```c
case GAID_FOLDER:
    log_debug(LOG_GUI, "GetFile gadget clicked\n");
    
    ULONG result = DoMethod((Object *)win_data->gadgets[GAD_IDX_FOLDER],
                           GFILE_REQUEST, win_data->window);
    
    log_debug(LOG_GUI, "GFILE_REQUEST returned: %lu\n", result);
    
    if (result)
    {
        STRPTR drawer = NULL;
        GetAttr(GETFILE_Drawer, win_data->gadgets[GAD_IDX_FOLDER], 
               (ULONG *)&drawer);
        log_debug(LOG_GUI, "GETFILE_Drawer: %s\n", drawer ? drawer : "(null)");
    }
    break;
```

### Check Event Reception

Temporarily log all events to see if clicks are reaching your handler:

```c
/* In event loop - TEMPORARY DEBUG ONLY */
while ((result = RA_HandleInput(window_obj, &code)) != WMHI_LASTMSG)
{
    log_debug(LOG_GUI, "Event: class=0x%08lx, id=%lu, code=%d\n",
             result & WMHI_CLASSMASK, result & WMHI_GADGETMASK, code);
    
    switch (result & WMHI_CLASSMASK)
    {
        /* ... */
    }
}
```

**Remove this logging after confirming events work correctly.**

---

## Reference Implementation

**Complete working example from iTidy main_window.c:**

See:
- Gadget creation: `src/GUI/main_window.c` lines ~489-499
- Event handler: `src/GUI/main_window.c` lines ~1415-1438
- Attribute update: `src/GUI/main_window.c` lines ~1050-1057

**Test by:**
1. Click the popup button
2. ASL folder requester should open immediately
3. Select a folder
4. Path should update in the display field

---

## Summary Checklist

When implementing getfile.gadget for folder selection:

- [ ] Include `<proto/getfile.h>` and `<gadgets/getfile.h>`
- [ ] Include `<clib/alib_protos.h>` for DoMethod()
- [ ] Open `getfile.gadget` library in init function
- [ ] Use `GETFILE_Drawer` (not GETFILE_FullFile) for folder paths
- [ ] Set `GETFILE_DrawersOnly, TRUE` for folder-only selection
- [ ] Set `GETFILE_ReadOnly, TRUE` for simple button mode
- [ ] Add `IDCMP_GADGETUP` to window IDCMP flags
- [ ] In GADGETUP handler, call `DoMethod()` with `GFILE_REQUEST`
- [ ] Cast gadget to `(Object *)` in DoMethod call
- [ ] Check DoMethod() return value (0 = cancelled)
- [ ] Read `GETFILE_Drawer` AFTER DoMethod returns non-zero
- [ ] Never use `DoGadgetMethod()` - causes crashes!

---

## Additional Resources

- **GetFile Gadget AutoDocs**: `c:\Amiga\temp\getfile.txt`
- **ReAction Examples**: `Tests/ReActon/` directory
- **ASL Library Docs**: AmigaOS SDK documentation for asl.library
- **BOOPSI Programming**: AmigaOS Intuition documentation

---

## Version History

- **v1.0** (2026-01-23): Initial guide based on iTidy main window implementation
  - Documented GETFILE_Drawer vs GETFILE_FullFile issue
  - Documented DoMethod() vs DoGadgetMethod() crash
  - Documented GFILE_REQUEST method requirement
  - Added complete implementation pattern

---

**Questions or Issues?**

If this guide doesn't solve your problem:
1. Check the getfile.gadget autodocs (`c:\Amiga\temp\getfile.txt`)
2. Review the working implementation in `src/GUI/main_window.c`
3. Check log files in `Bin/Amiga/logs/gui_*.log` for error details
4. Search for "GETFILE" or "getfile.gadget" in the iTidy development log (`docs/DEVELOPMENT_LOG.md`)
