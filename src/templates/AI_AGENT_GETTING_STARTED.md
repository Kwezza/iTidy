# AI_AGENT_GETTING_STARTED.md  
Getting Started Guide for AI Agents (iTidy Amiga GUI)

---

## 1. Purpose & Scope

This document is the **entry point** for any AI agent (or human) working on the iTidy Amiga GUI project.

It tells you:

- **What this codebase targets:**  
  - Classic AmigaOS / **Workbench 3.0 and 3.1 only**  
  - C89-style code (vbcc-compatible, no AmigaOS 3.5+/3.9/4.x/AROS/MorphOS APIs)
- **Which documents are authoritative rules**  
- **Which source templates are canonical examples**  
- **Which file to use for which kind of task**

If you are an AI assistant reading this:  
**Do not** treat all project files as equal. Follow this map. When in doubt, copy patterns from the canonical templates mentioned here rather than inventing new code.

---

## 2. How to Use This Guide

1. **Start here** whenever you get a new task.  
2. Find the **task type** in the “Common Tasks → Where to Look” section.  
3. Read the referenced sections in:
   - `AI_AGENT_GUIDE.md`
   - `AI_AGENT_LAYOUT_GUIDE.md`
   - And the relevant template `.c` file(s)
4. Only then modify or generate code.

Treat this document as the **router**: it tells you what to read and which code to mimic.

---

## 3. Project Constraints (MANDATORY)

These constraints apply to **all** new or modified code:

- **Target OS:**  
  - AmigaOS / Workbench **3.0 (V39)** and **3.1 (V40)** only.
- **Out of scope:**  
  - AmigaOS 3.2, 3.5, 3.9, 4.x  
  - AROS, MorphOS, AmigaONE, ApolloOS, etc.  
  - ReAction, BOOPSI-based NG GUI frameworks, Dockies, etc.
- **Language / Style:**  
  - C89-style C, compatible with `vbcc` and classic Amiga headers.  
  - No reliance on GCC extensions, C99-only features, or inline C++.
- **SDK Naming Collisions:**  
  - Follow the naming rules in `AI_AGENT_GUIDE.md` (Amiga SDK naming collision policy).  
  - All project-owned identifiers must use the defined prefixes (e.g., `iTidy_` / `ITIDY_`).

If you find examples online that contradict these constraints, prefer **the rules in this repository** over any external source.

---

## 3.5. CRITICAL: GadTools Coordinate System (MUST READ FIRST!)

**Before creating ANY window or gadget**, understand this fundamental rule:

### GadTools Gadgets Use Window-Relative Coordinates

**The Fatal Mistake:**  
Many developers assume `NewGadget.ng_TopEdge = 0` places a gadget at the top of the **client area** (below the title bar). **This is WRONG.**

**The Reality:**  
- `ng_TopEdge = 0` is at the window's **outer top-left corner**
- Y=0 is the **top of the title bar itself**
- To position gadgets below the title bar, you **MUST** offset by BorderTop

### The Mandatory Formula (From Amiga RKMs)

Calculate the window's BorderTop **before** opening the window:

```c
struct Screen *screen = LockPubScreen("Workbench");

/* CRITICAL: This is the ONLY correct way to calculate BorderTop */
WORD border_top    = screen->WBorTop + screen->Font->ta_YSize + 1;
WORD border_left   = screen->WBorLeft;
WORD border_right  = screen->WBorRight;
WORD border_bottom = screen->WBorBottom;

UnlockPubScreen(NULL, screen);
```

### Correct Gadget Positioning Pattern

```c
/* Add a small margin below the title bar */
WORD margin = 5;
WORD current_y = border_top + margin;  /* Start BELOW title bar */
WORD current_x = border_left + margin;

/* Create gadgets with window-relative coordinates */
struct NewGadget ng;
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge  = current_y;  /* NOT 0, NOT margin alone! */
ng.ng_Width    = gadget_width;
ng.ng_Height   = gadget_height;
```

### Window Height Calculation

```c
/* Calculate content height (gadget positions are already offset by border_top) */
UWORD content_height = last_gadget_Y + gadget_height + bottom_margin;

/* Add borders to get total window height */
UWORD window_height = content_height + border_top + border_bottom;

/* Open window with total height */
window = OpenWindowTags(NULL,
    WA_Height, window_height,  /* Includes borders */
    ...
);
```

### Verification (Your Estimate MUST Match Reality)

After opening the window, verify your calculation is correct:

```c
printf("Calculated BorderTop: %d\n", border_top);
printf("Actual BorderTop: %d\n", window->BorderTop);
printf("Difference: %d\n", window->BorderTop - border_top);
```

**The difference MUST be 0.** If not, you used the wrong formula!

### Common Fatal Mistakes

❌ **DON'T** position gadgets at `ng_TopEdge = 0` or `ng_TopEdge = margin`  
❌ **DON'T** guess border heights (e.g., "16 pixels is typical")  
❌ **DON'T** use only `screen->WBorTop` (this misses the title bar font height!)  
❌ **DON'T** forget the `+ 1` in the formula  
❌ **DON'T** assume BorderTop == BorderBottom (they can be different!)  
❌ **DON'T** use `font_height * 2` or other arbitrary multipliers

### Why This Formula Works

The formula `screen->WBorTop + screen->Font->ta_YSize + 1` adapts to:
- Different screen fonts (Topaz 8, Helvetica 11, etc.)
- IControl preferences (enlarged title bars in WB 3.2)
- Different Workbench versions (3.0, 3.1, 3.2)
- Custom screen modes and themes

**It reads the actual screen's font metrics, not hardcoded values.**

### Reference Implementation

See `amiga_window_template.c` → `calculate_font_dimensions()` for the complete working example that has been verified to produce **zero-pixel positioning errors** across all configurations.

**Bottom Line:** If your gadgets overlap the title bar or are positioned incorrectly, you violated this rule. Fix it by using the formula above.

---

## 4. Document Map (What Each File Is For)

### 4.1 Authoritative AI Guides

These are the **main specifications** for how AI-generated code must behave.

#### `AI_AGENT_GUIDE.md`

- **Role:**  
  Main behavioural guide for AI-generated code.

- **Contains:**  
  - Amiga SDK naming collision policy.  
  - Dynamic window template usage and expectations.  
  - **OS3.0/3.1 Window & IDCMP contracts** (event loop rules, CloseWindow rules).  
  - Gadget value handling rules (cycle, checkbox, string, slider, integer).  
  - **ListView rules:** height snapping, dynamic updates, detach/cleanup ordering.  
  - **NewLook menu rules:** how to build and handle menus safely on 3.0/3.1.  
  - **Workbench vs CLI output policy.**  
  - Template index mapping prose → templates.

- **Use when:**  
  - Writing or modifying any C code in this project.  
  - Debugging GUI behaviour (IDCMP issues, ListView crashes, checkbox bugs, etc.).  
  - Designing new windows or menu systems.

- **Authority:**  
  If anything in a `.c` file looks inconsistent with this guide, **the guide wins** and the code should be brought back into alignment with it.

---

#### `AI_AGENT_LAYOUT_GUIDE.md`

- **Role:**  
  Layout and geometry specification for iTidy-style GUI windows.

- **Contains:**  
  - High-level layout goals (readability, font-aware sizing).  
  - Standard layout patterns:
    - Reference content column  
    - Equal button row  
    - ListView + button row layout  
  - **OS3.0/3.1 resizing rules** (`IDCMP_NEWSIZE` & size gadgets).  
  - **ListView height vs layout** (snapping to full text lines).  
  - “No advanced IDCMP/layout hacks” policy.

- **Use when:**  
  - Placing gadgets (Left/Top/Width/Height).  
  - Adding or changing layouts for new windows.  
  - Handling resizing in a way that preserves the layout.

- **Authority:**  
  Layout code in `.c` files should follow these patterns. Avoid ad-hoc geometry.

---

#### `amiga_gui_research_3x.md` (or `.pdf`)

- **Role:**  
  Background **research** document on AmigaOS 3.0/3.1 GUI behaviour.

- **Contains:**  
  - Summarised guarantees and quirks of Intuition, GadTools, ASL, and menus on 3.0/3.1.  
  - Historical/contextual information behind the rules in the AI guides.

- **Use when:**  
  - You need to understand **why** a rule exists.  
  - You are designing new patterns and need to be sure they are 3.0/3.1-safe.

- **Authority:**  
  - This is **secondary** to `AI_AGENT_GUIDE.md` and `AI_AGENT_LAYOUT_GUIDE.md`.  
  - Do **not** copy code from here directly unless it is clearly the same as the patterns in the templates and guides.

---

### 4.2 Canonical Code Templates

These `.c` files contain the **live, working code patterns** that AI agents must copy and adapt.

#### `amiga_window_template.c`

- **Role:**  
  Canonical template for a **Workbench 3.x window with gadgets and ListView**.

- **Key contents to copy from:**
  - `create_template_window()`  
    - How to:
      - Lock the Workbench screen.  
      - Get `VisualInfo` / `DrawInfo`.  
      - Create gadgets (including `LISTVIEW_KIND`) in a font-aware way.  
      - Attach the ListView’s `struct List` via `GTLV_Labels`.  
      - Compute window size from gadgets and font.
  - `close_template_window()`  
    - Correct shutdown order:
      - **Detach** ListView labels (`GTLV_Labels, ~0UL`)  
      - Clear menu strip (if present)  
      - `CloseWindow()`  
      - Free ListView data  
      - Free gadgets, menus, VisualInfo, screen info, etc.
  - `free_listview_items()`  
    - How to free the backing Exec list for a ListView safely.

- **Use when:**  
  - Creating a **new window** in this project.  
  - Adding ListViews or checkboxes/strings/sliders to a window.  
  - Implementing correct attach/detach/cleanup for ListView data.

- **Authority:**  
  This file is the **“this is how we do windows”** reference. New windows should **start** from this pattern.

---

#### `amiga_window_menus_template.c`

- **Role:**  
  Canonical template for **Workbench 3.x NewLook menus and menu handling**.

- **Key contents to copy from:**
  - `setup_newlook_menus()`  
    - `NewMenu` array structure.  
    - `CreateMenus` + `LayoutMenus(..., GTMN_NewLookMenus, TRUE, ...)`.  
    - `SetMenuStrip(window, menu_strip)` usage.
  - Event loop in `main()`  
    - `Wait()` + `GT_GetIMsg()` + `GT_ReplyIMsg()` loop.  
    - Handling of:
      - `IDCMP_CLOSEWINDOW`  
      - `IDCMP_MENUPICK`  
      - `IDCMP_REFRESHWINDOW` (`GT_BeginRefresh` / `GT_EndRefresh`)  
      - `IDCMP_NEWSIZE` (stub handler for future layout work).
  - `handle_menu_selection()`  
    - `ItemAddress(menu_strip, code)`  
    - `GTMENUITEM_USERDATA(item)` → app-level menu IDs.  
    - Looping `NextSelect` until `MENUNULL`.

- **Use when:**  
  - Adding menus to any window.  
  - Handling menu events in a safe, OS3-compatible way.

- **Authority:**  
  This file is the **“this is how we do menus”** reference. New menu code should copy this structure.

---

## 5. Common Tasks → Where to Look

Use this section as a quick router for typical jobs.

### Task A: Create a New iTidy-Style Window

1. Read `AI_AGENT_GUIDE.md`:
   - “Dynamic Window Template”
   - “OS3 Window & IDCMP Contracts (MANDATORY)”
2. Copy and adapt from:
   - `amiga_window_template.c` → `create_template_window()` and `close_template_window()`.
3. Keep:
   - The screen locking, VisualInfo, DrawInfo patterns.  
   - The ListView attach/detach pattern if using ListViews.  
   - The cleanup order.

---

### Task B: Add or Modify a ListView (including dynamic updates)

1. Read in `AI_AGENT_GUIDE.md`:
   - ListView height discussion.  
   - “ListView Cleanup Order Bug (Use-After-Free)” section.  
   - “Safe Pattern: Updating ListView Contents at Runtime.”
2. Examine:
   - `amiga_window_template.c` → ListView creation, `free_listview_items()`, and `close_template_window()` (detach with `GTLV_Labels, ~0UL`).
3. Follow this pattern:
   - **Attach** labels when creating the window / gadget.  
   - For **dynamic updates**:
     - `GTLV_Labels, ~0UL` (detach)  
     - Modify the Exec list  
     - `GTLV_Labels, (ULONG)list` (reattach)  
   - **Detach** labels before freeing the list and before final window close.

---

### Task C: Add or Modify Menus

1. Read in `AI_AGENT_GUIDE.md`:
   - “CRITICAL: NewLook Menu Handling Rules”.
2. Copy from:
   - `amiga_window_menus_template.c` → `setup_newlook_menus()` and `handle_menu_selection()`.
3. Rules to follow:
   - `nm_UserData` / `GTMENUITEM_USERDATA` for menu IDs.  
   - Handle `IDCMP_MENUPICK` with:
     - `ItemAddress(menu_strip, code)`  
     - `menu_item->NextSelect` until `MENUNULL`.
   - **Do not** use `IDCMP_MENUVERIFY` or `MENUVERIFY`.

---

### Task D: Use String Gadgets Safely

1. Read in `AI_AGENT_GUIDE.md`:
   - “CRITICAL: String Gadgets Are Not Auto-Committed.”
2. Implement:
   - A `sync_window_strings()`-style helper beside your window code.  
   - Call that helper in the **OK/Apply** gadget handler to re-read string values before using them.

3. If you add STRING_KIND gadgets to the template:
   - Keep all string syncing code close to the window code (same `.c` file).  
   - Do not rely solely on `IDCMP_GADGETUP` for final string values.

---

### Task E: Fix a Crash or Weird GUI Behaviour

1. Identify the affected window/feature.  
2. Compare the code to the canonical templates:
   - For windows/ListViews: `amiga_window_template.c`.  
   - For menus: `amiga_window_menus_template.c`.
3. Re-read the relevant sections in:
   - `AI_AGENT_GUIDE.md` (OS3 contracts, ListView rules, checkbox bug, etc.).  
   - `AI_AGENT_LAYOUT_GUIDE.md` (if layout/resizing is involved).
4. Bring the affected code **back into alignment** with the templates and guides.

If external examples or older code disagree with the rules, **trust the guides and templates** in this folder.

---

## 6. Golden Rules (TL;DR for AI Agents)

- **OS Version & APIs**
  - Only assume AmigaOS / Workbench **3.0/3.1**.
  - Do **not** use 3.2/3.5/3.9/4.x/AROS/MorphOS/ReAction features.

- **Window & IDCMP**
  - One `UserPort` per window.  
  - `Wait()` + full `while (GT_GetIMsg())` drain.  
  - Always `GT_ReplyIMsg()` exactly once.  
  - Never call `CloseWindow()` inside the message loop.

- **Gadgets & Values**
  - Checkboxes: use a temporary `ULONG` with `GTCB_Checked` then cast to `BOOL`.  
  - Strings: re-read values before OK/Apply (not only on `IDCMP_GADGETUP`).  
  - ListViews: detach labels before freeing/modifying underlying lists.

- **Cleanup**
  - Detach ListView labels → free list data → clear menus → close window → free gadgets → free menus → free VisualInfo → unlock screen.  
  - Never free gadgets or menus while a window is still open on them.

- **Menus**
  - Use NewLook + `NewMenu` as in `amiga_window_menus_template.c`.  
  - Handle menus with `IDCMP_MENUPICK`, `ItemAddress`, and `GTMENUITEM_USERDATA`.  
  - Do **not** use `IDCMP_MENUVERIFY` or `MENUVERIFY`.

- **Output & Exit**
  - In Workbench context, do not assume a console exists; prefer GUI or log files.  
  - Return from `main()` (or `exit()`), never call `Exit()` from Exec.

When in doubt, **read `AI_AGENT_GUIDE.md` and copy the patterns from the canonical templates** rather than making up new ones.

---

## 7. Suggested Usage in Prompts (for Humans)

When you ask an AI to work on this project, start your prompt with something like:

> You are working on the **iTidy** AmigaOS 3.0/3.1 GUI project.  
> Start by reading `AI_AGENT_GETTING_STARTED.md` and follow its instructions on which other docs and templates to consult.  
> Treat `AI_AGENT_GUIDE.md`, `AI_AGENT_LAYOUT_GUIDE.md`, `amiga_window_template.c`, and `amiga_window_menus_template.c` as authoritative.  
> Do not use any APIs or assumptions beyond Workbench 3.1.

Then describe your specific task (e.g., “add a new preferences window with a ListView and OK/Cancel buttons”).

This ensures the AI **zooms in** on the right documents and templates instead of treating everything it sees as equally important.
