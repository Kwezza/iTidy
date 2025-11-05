# Status Windows Design Specification

**Project:** iTidy Icon Cleanup Tool  
**Feature:** Generic Progress/Status Windows  
**Status:** Design Phase  
**Date:** November 5, 2025  
**Platform:** Amiga OS 2.0+ / VBCC  

---

## Executive Summary

This document outlines the design and implementation strategy for reusable status windows in iTidy. These windows provide visual feedback during long-running operations such as backup creation, archive restoration, and recursive icon processing.

### Key Design Goals

1. **Generic and Reusable** - Single API works across multiple operations
2. **Professional Appearance** - Workbench 3.x compliant 3D beveled design
3. **Clear Feedback** - Users always know what's happening and how long it will take
4. **Non-Blocking** - Modal windows that prevent interaction but don't freeze the system
5. **Performance-Optimized** - Smart redrawing for smooth animation on 7MHz systems

---

## Naming Convention Update (iTidy_ prefix)

To avoid accidental symbol collisions with AmigaOS SDK macros and identifiers (for example SHINEPEN/SHADOWPEN and other short or common names), all public types, functions, and constants in this feature use an explicit iTidy_ prefix, and fields use snake_case. This improves clarity, grep-ability, and reduces risk of macro expansion surprises on legacy headers.

- Types: iTidy_ProgressWindow, iTidy_RecursiveProgressWindow, iTidy_RecursiveScanResult
- Functions: iTidy_OpenProgressWindow, iTidy_UpdateProgress, iTidy_ShowCompletionState, iTidy_HandleProgressWindowEvents, iTidy_CloseProgressWindow, etc.
- Common drawing helpers: iTidy_Progress_DrawBevelBox, iTidy_Progress_DrawBarFill, iTidy_Progress_DrawTextLabel, iTidy_Progress_DrawPercentage, iTidy_Progress_ClearTextArea, iTidy_Progress_HandleRefresh
- Fields and parameters: left, top, width, height; iTidy_shine_pen, iTidy_shadow_pen, iTidy_fill_pen, iTidy_bar_pen, iTidy_text_pen

Why renamed:
- Avoid clashes with SDK macros and identifiers on AmigaOS 2.x/3.x
- Keep exported API clearly within the project namespace
- Make code search and maintenance easier across modules

---

## Why Two Window Types?

### The Problem

Different operations have fundamentally different progress characteristics:

| Operation Type | Progress Levels | Example |
|---------------|----------------|---------|
| **Single-Level** | One task with known item count | Restoring 63 archives |
| **Nested/Recursive** | Multiple folders, each with multiple icons | Processing 500 folders containing 8,432 icons |

**Challenge:** A single-level progress bar appears "stuck" during recursive operations when processing large folders (e.g., a folder with 200 icons may take 5-10 seconds with no visible progress).

**Solution:** Implement two specialized window types, each optimized for its use case.

---

## Window Type 1: Simple Progress Window

### Purpose
Shows progress for single-level operations where the total count is known in advance.

### Use Cases
- ✅ Backup operations (archiving folders)
- ✅ Restore operations (extracting archives)
- ✅ Single folder icon processing
- ✅ Catalog parsing
- ✅ Any operation with a simple for-loop over known items

### Visual Design

```
┌──────────────────────────────────────────────────┐
│ Restoring Backup Run 0007              45%      │ ← Task label + percentage
├──────────────────────────────────────────────────┤
│                                                  │
│  ╔═══════════════════════════════════════════╗  │ ← Beveled progress bar
│  ║███████████████████░░░░░░░░░░░░░░░░░░░░░░░║  │   (3D recessed style)
│  ╚═══════════════════════════════════════════╝  │
│                                                  │
│  Extracting: 00015.lha                          │ ← Helper text (current item)
│                                                  │
└──────────────────────────────────────────────────┘
    Window Size: 400px × 120px
```

### Window Properties

| Property | Value | Reason |
|----------|-------|--------|
| Width | 400 pixels | Wide enough for long filenames |
| Height | 120 pixels | Compact, fits standard screen |
| Type | Modal | Prevents interaction during operation |
| Close Gadget | No | Must complete (no cancel) |
| Title Bar | Yes | Shows task description |
| Depth Gadget | Yes | Standard window behavior |

### API Design (prefixed)

```c
/* Open progress window */
struct iTidy_ProgressWindow* iTidy_OpenProgressWindow(
    struct Screen *screen,         /* Workbench screen */
    const char *task_label,        /* "Restoring Backup Run 0007" */
    UWORD total_items              /* Total count (e.g., 63 archives) */
);

/* Update progress */
void iTidy_UpdateProgress(
    struct iTidy_ProgressWindow *pw,
    UWORD current_item,            /* Current item number (1-based) */
    const char *helper_text        /* "Extracting: 00015.lha" */
);

/* Show completion state with Close button (optional - for longer operations) */
void iTidy_ShowCompletionState(
    struct iTidy_ProgressWindow *pw,
    BOOL success                   /* TRUE = success, FALSE = error */
);

/* Handle events (only needed if using completion state) */
BOOL iTidy_HandleProgressWindowEvents(
    struct iTidy_ProgressWindow *pw      /* Returns FALSE when Close clicked */
);

/* Close window */
void iTidy_CloseProgressWindow(
    struct iTidy_ProgressWindow *pw
);
```

### Usage Example

**Pattern A: Auto-Close (Fast Operations)**

```c
/* Restore operation - auto-close when done */
struct iTidy_ProgressWindow *pw = iTidy_OpenProgressWindow(
    screen, 
    "Restoring Backup Run 0007", 
    catalog->entryCount  /* 63 archives */
);

for (i = 0; i < catalog->entryCount; i++) {
    char status[256];
    snprintf(status, sizeof(status), "Extracting: %s", entry->archiveName);
    
    iTidy_UpdateProgress(pw, i + 1, status);
    
    /* Perform actual work */
    ExtractArchive(entry->archivePath, destination);
}

/* Close immediately when done */
iTidy_CloseProgressWindow(pw);
```

**Pattern B: Completion State with Close Button (RECOMMENDED)**

```c
/* Restore operation - show completion state */
struct iTidy_ProgressWindow *pw = iTidy_OpenProgressWindow(
    screen, 
    "Restoring Backup Run 0007", 
    catalog->entryCount  /* 63 archives */
);

BOOL success = TRUE;
for (i = 0; i < catalog->entryCount; i++) {
    char status[256];
    snprintf(status, sizeof(status), "Extracting: %s", entry->archiveName);
    
    iTidy_UpdateProgress(pw, i + 1, status);
    
    /* Perform actual work */
    if (!ExtractArchive(entry->archivePath, destination)) {
        success = FALSE;
        break;  /* Stop on error */
    }
}

/* Show completion state with Close button */
iTidy_ShowCompletionState(pw, success);

/* Wait for user to click Close button */
while (iTidy_HandleProgressWindowEvents(pw)) {
    WaitPort(pw->window->UserPort);
}

/* User clicked Close - now we can clean up */
iTidy_CloseProgressWindow(pw);
```

---

## Window Type 2: Recursive Progress Window

### Purpose
Shows dual-level progress for recursive operations where both folder count and icon count matter.

### Use Cases
- ✅ Recursive icon processing across multiple folders
- ✅ Any operation that walks a directory tree
- ✅ Multi-stage operations with nested progress

### The Nested Progress Problem

When processing 500 folders recursively:
- **Without inner progress:** User sees outer bar stuck at "Folder 127" for 10 seconds (no feedback)
- **With inner progress:** User sees both bars moving (constant visual feedback = feels responsive)

**Psychological Impact:** Dual progress bars provide constant motion, reducing perceived wait time even though actual duration is the same.

### Visual Design

```
┌──────────────────────────────────────────────────┐
│ Processing Icons Recursively            45%     │ ← Overall task + percentage
├──────────────────────────────────────────────────┤
│                                                  │
│  Folders:  ╔════════════════════╗  227/500 ←──── Outer progress (folders)
│            ║████████░░░░░░░░░░░░║               │   18px tall (beveled)
│            ╚════════════════════╝               │
│                                                  │
│  Work:WHDLoad/GamesOCS/Abandonware/        ←──── Current folder path
│  Icons:    ╔════════════════════╗   15/43  ←──── Inner progress (icons)
│            ║████░░░░░░░░░░░░░░░░║               │   18px tall (beveled)
│            ╚════════════════════╝               │
│                                                  │
└──────────────────────────────────────────────────┘
    Window Size: 450px × 165px
```

### Window Properties

| Property | Value | Reason |
|----------|-------|--------|
| Width | 450 pixels | Room for long folder paths |
| Height | 165 pixels | Extra height for dual bars + labels |
| Type | Modal | Prevents interaction |
| Close Gadget | No | Must complete |
| Title Bar | Yes | Shows overall task |
| Depth Gadget | Yes | Standard behavior |

### Prescan Requirement

**Critical:** Recursive operations require a prescan phase to count total folders/icons before opening the progress window.

**Why?**
- Progress bars need known totals to calculate percentages
- Cannot show "X of ???" - looks unprofessional
- Prescan is fast (1-2 seconds for 500 folders - only `Examine()`/`ExNext()`, no icon loading)

**⚠️ CRITICAL: Prescan Must Yield to Multitasking**

Prescans walk directory trees which can have hundreds or thousands of entries. Without yielding, 
the system appears frozen during large scans:

```c
ULONG PrescanRecursive(const char *path, RecursiveScanResult *result) {
    ULONG itemCount = 0;
    
    /* Scan directory entries */
    while ((entry = ReadNextEntry()) != NULL) {
        itemCount++;
        
        /* CRITICAL: Yield every N items to keep system responsive */
        if (itemCount % 100 == 0) {
            Delay(1);  /* One tick - imperceptible but prevents lockup */
        }
        
        if (entry->isDirectory) {
            itemCount += PrescanRecursive(entry->path, result);
            Delay(1);  /* Also yield after each subdirectory */
        }
    }
    
    return itemCount;
}
```

**Why Delay(1) Matters:**
- **Without yielding**: System freezes for 5-10 seconds on 500-folder scan - mouse doesn't move!
- **With yielding**: System stays responsive - other tasks run, mouse moves smoothly
- **Cost**: One timer tick (1/50 sec) every 100 items = ~0.02 sec overhead for 1000 items
- **User Experience**: "Responsive but working" vs. "crashed or frozen"

**When to Yield:**
- ✅ Every 100 items in flat loops
- ✅ After processing each subdirectory
- ✅ Before any operation that takes >0.5 seconds
- ❌ Inside tight inner loops (<100 iterations)
- ❌ During critical sections (file I/O already yields)

### Prescan Data Structure

```c
typedef struct {
    ULONG totalFolders;        /* Total folders to process (e.g., 500) */
    ULONG totalIcons;          /* Total icons across all folders (e.g., 8,432) */
    char **folderPaths;        /* Array of folder paths */
    UWORD *iconCounts;         /* Icons per folder (parallel array) */
} iTidy_RecursiveScanResult;
```

### API Design

```c
/* Prescan directory tree */
iTidy_RecursiveScanResult* iTidy_PrescanRecursive(
    const char *rootPath       /* "Work:WHDLoad" */
);

/* Open recursive progress window */
struct iTidy_RecursiveProgressWindow* iTidy_OpenRecursiveProgress(
    struct Screen *screen,
    const char *task_label,                    /* "Processing Icons Recursively" */
    const iTidy_RecursiveScanResult *scan      /* Prescan results */
);

/* Update outer progress (new folder) */
void iTidy_UpdateFolderProgress(
    struct iTidy_RecursiveProgressWindow *rpw,
    UWORD folder_index,                        /* Current folder (1-based) */
    const char *folder_path,                   /* "Work:WHDLoad/GamesOCS/" */
    UWORD icons_in_folder                      /* Icon count for progress bar setup */
);

/* Update inner progress (icon in current folder) */
void iTidy_UpdateIconProgress(
    struct iTidy_RecursiveProgressWindow *rpw,
    UWORD icon_index                           /* Current icon (1-based) */
);

/* Close window */
void iTidy_CloseRecursiveProgress(
    struct iTidy_RecursiveProgressWindow *rpw
);

/* Free prescan results */
void iTidy_FreeScanResult(
    iTidy_RecursiveScanResult *scan
);
```

### Usage Example

```c
/* Phase 1: Prescan */
printf("Scanning folders...\n");
iTidy_RecursiveScanResult *scan = iTidy_PrescanRecursive("Work:WHDLoad");
/* Result: Found 500 folders, 8,432 total icons */

/* Phase 2: Open progress window */
struct iTidy_RecursiveProgressWindow *rpw = iTidy_OpenRecursiveProgress(
    screen,
    "Processing Icons Recursively",
    scan
);

/* Phase 3: Process each folder */
for (i = 0; i < scan->totalFolders; i++) {
    /* Update outer progress */
    iTidy_UpdateFolderProgress(rpw, i + 1, scan->folderPaths[i], scan->iconCounts[i]);
    
    /* Load icons for this folder */
    iconArray = CreateIconArrayFromPath(scan->folderPaths[i]);
    
    /* Process each icon */
    for (j = 0; j < iconArray->count; j++) {
        /* Update inner progress */
    iTidy_UpdateIconProgress(rpw, j + 1);
        
        /* Process icon */
        ProcessIcon(&iconArray->icons[j]);
    }
    
    /* Save and cleanup */
    saveIconsPositionsToDisk(scan->folderPaths[i], iconArray);
    FreeIconArray(iconArray);
}

/* Phase 4: Cleanup */
iTidy_CloseRecursiveProgress(rpw);
iTidy_FreeScanResult(scan);
```

---

## Font Measurement and Text Rendering

### ⚠️ CRITICAL: Proper Font Access (AmigaOS 3.0 Best Practice)

**DO NOT** use `screen->RastPort.Font` directly or `strlen() * font_width` for measurements!

**CORRECT METHOD - GetScreenDrawInfo():**

```c
struct DrawInfo *draw_info;
struct RastPort temp_rp;
struct TextFont *font;
UWORD font_width, font_height;

/* Get screen's DrawInfo */
draw_info = GetScreenDrawInfo(screen);
if (draw_info == NULL) {
    append_to_log("ERROR: Could not get screen DrawInfo\n");
    return FALSE;
}

/* Access font via DrawInfo, not screen->RastPort.Font */
font = draw_info->dri_Font;
font_width = font->tf_XSize;
font_height = font->tf_YSize;

/* Initialize RastPort for accurate text measurements */
InitRastPort(&temp_rp);
SetFont(&temp_rp, font);

/* Measure text width accurately (NOT strlen * font_width!) */
UWORD task_label_width = TextLength(&temp_rp, "Restoring Backup...", 19);
UWORD percent_width = TextLength(&temp_rp, "100%", 4);

/* ... use measurements for layout ... */

/* CRITICAL: Always free DrawInfo when done */
FreeScreenDrawInfo(screen, draw_info);
```

**Why This Matters:**
- ✅ Accurate text width for proportional fonts
- ✅ Respects user's Workbench font preferences
- ✅ Works correctly on all Workbench versions (2.0+)
- ✅ Follows AmigaOS Style Guide

**Common Mistakes:**
- ❌ Using `screen->RastPort.Font` - bypasses proper font system
- ❌ Using `strlen() * font_width` - inaccurate for proportional fonts
- ❌ Forgetting to call `FreeScreenDrawInfo()` - memory leak
- ❌ Hardcoding pixel widths - breaks with different fonts

---

## Visual Design: 3D Beveled Progress Bars

### Why Beveled Bars?

1. **Workbench 3.x Visual Consistency** - Matches button gadgets, string gadgets, listview borders
2. **Depth Perception** - Bevels make bars appear "recessed" like they're filling a container
3. **Professional Appearance** - More polished than flat rectangles
4. **Clear Boundaries** - Bright/dark edges define exact bar area

### Bevel Rendering Technique

**Workbench 3.x uses a 2-pixel bevel with four pens:**

```
SHINEPEN (pen 2)    ╔═══════════════════╗  Bright edges (top-left)
SHADOWPEN (pen 1)   ║░░░░░░░░░░░░░░░░░░║  Dark edges (bottom-right)
FILLPEN (pen 0)     ║░░░░░░░░░░░░░░░░░░║  Background fill
BARPEN (pen 3)      ║████░░░░░░░░░░░░░░║  Progress fill (blue)
                    ╚═══════════════════╝
```

### Drawing Sequence

```c
1. Draw outer dark edge (bottom-right)    - SHADOWPEN
2. Draw outer bright edge (top-left)      - SHINEPEN
3. Draw inner dark edge (bottom-right)    - SHADOWPEN
4. Draw inner bright edge (top-left)      - SHINEPEN
5. Fill interior background               - FILLPEN (gray)
6. Fill progress portion                  - BARPEN (blue)
```

### Color Palette Strategy

**Use screen's DrawInfo for theme compatibility:**

```c
struct DrawInfo *dri = GetScreenDrawInfo(screen);
if (dri == NULL) {
    /* Handle error - fallback to basic pens */
    return FALSE;
}

/* Extract pen assignments from user's preferences */
ULONG shinePen = dri->dri_Pens[SHINEPEN];      /* Bright edge */
ULONG shadowPen = dri->dri_Pens[SHADOWPEN];    /* Dark edge */
ULONG fillPen = dri->dri_Pens[BACKGROUNDPEN];  /* Background */
ULONG barPen = dri->dri_Pens[FILLPEN];         /* Blue highlight */

/* Use pens for drawing */
SetAPen(rp, shinePen);
/* ... draw bevel ... */

/* CRITICAL: Always free DrawInfo in both success and error paths */
FreeScreenDrawInfo(screen, dri);
```

**Benefits:**
- ✅ Respects user's Workbench preferences
- ✅ Works on custom themes (NewIcons, MagicWB)
- ✅ Adapts to screen depth (4-color, 8-color, 256-color)
- ✅ Professional - follows AmigaOS UI guidelines

**Critical Rules:**
- ⚠️ **ALWAYS** check for NULL return from `GetScreenDrawInfo()`
- ⚠️ **ALWAYS** call `FreeScreenDrawInfo()` when done (both success and error paths)
- ⚠️ **NEVER** cache DrawInfo pointer - get it, use it, free it immediately

### Progress Bar Dimensions

| Element | Size | Notes |
|---------|------|-------|
| Bar outer height | 18px | 16px interior + 2px bevel |
| Bevel thickness | 2px | Standard Workbench 3.x |
| Interior height | 14px | 18px - 4px for bevels |
| Bar width | Window width - 140px | Room for label + count |
| Spacing between bars | 20px | Fits folder path text |

---

## File Structure & Organization

### Directory Layout

```
src/GUI/StatusWindows/           ← NEW SUBFOLDER (keeps GUI/ clean)
├── progress_common.h            (shared constants, structures)
├── progress_common.c            (bevel drawing, text rendering, utilities)
│   ├── DrawBevelBox()           - Render 3D beveled container
│   ├── DrawProgressBarFill()    - Fill progress bar with percentage
│   ├── DrawTextLabel()          - Render text labels
│   ├── DrawPercentage()         - Right-aligned percentage text
│   ├── ClearTextArea()          - Erase previous text (avoid ghosting)
│   └── HandleProgressRefresh()  - Shared refresh event utility (NEW)
│
├── progress_window.h            (simple progress API)
├── progress_window.c            (single-bar implementation)
│   ├── OpenProgressWindow()
│   ├── UpdateProgress()
│   ├── ShowCompletionState()    - Optional completion UI with Close button
│   ├── HandleProgressWindowEvents() - Event loop for completion state
│   └── CloseProgressWindow()
│
├── recursive_progress.h         (recursive progress API)
└── recursive_progress.c         (dual-bar implementation)
    ├── PrescanRecursive()       - Prescan directory tree (MUST yield!)
    ├── OpenRecursiveProgress()
    ├── UpdateFolderProgress()   - Update outer bar (folders)
    ├── UpdateIconProgress()     - Update inner bar (icons)
    └── CloseRecursiveProgress()
```

**Why Subfolder?**
- ✅ **Logical grouping:** Status windows are a distinct category
- ✅ **Scalability:** Easy to add more status windows later (error dialogs, confirmations)
- ✅ **Cleaner organization:** Main `src/GUI/` doesn't get cluttered
- ✅ **Clear purpose:** "StatusWindows" name clearly indicates functionality
- ✅ **Build system friendly:** Can add entire subfolder with wildcard in Makefile

### Estimated Code Size

| File | Lines | Purpose |
|------|-------|---------|
| `progress_common.c/h` | ~250 | Shared drawing primitives |
| `progress_window.c/h` | ~300 | Simple single-bar window |
| `recursive_progress.c/h` | ~450 | Dual-bar + prescan logic |
| **Total** | **~1,000** | Complete status window system |

---

## Implementation Strategy

### ⚠️ MANDATORY: Fast Window Opening Pattern

**DO NOT** perform slow operations before opening the window!

Following the proven "Fast Window Opening with Deferred Loading" pattern from iTidy's GUI templates, status windows should open **immediately** to provide instant feedback, even on slow hardware.

**❌ WRONG WAY (Poor UX):**
```c
struct ProgressWindow* OpenProgressWindow(...) {
    /* Parse data first - user waits with no feedback */
    parse_large_file(...);
    
    /* FINALLY open window - appears instantly but user waited 5 seconds */
    window = OpenWindowTags(...);
    return pw;
}
```

**✅ CORRECT WAY (Professional UX):**
```c
struct ProgressWindow* OpenProgressWindow(
    struct Screen *screen,
    const char *task_label,
    UWORD total_items
) {
    struct ProgressWindow *pw;
    
    /* Allocate and initialize structure */
    pw = AllocMem(sizeof(struct ProgressWindow), MEMF_CLEAR);
    if (!pw) return NULL;
    
    /* Store parameters */
    strncpy(pw->task_label, task_label, sizeof(pw->task_label) - 1);
    pw->total = total_items;
    pw->current = 0;
    
    /* Calculate window dimensions (pre-calculation) */
    struct DrawInfo *dri = GetScreenDrawInfo(screen);
    if (!dri) {
        FreeMem(pw, sizeof(struct ProgressWindow));
        return NULL;
    }
    
    struct TextFont *font = dri->dri_Font;
    UWORD font_width = font->tf_XSize;
    UWORD font_height = font->tf_YSize;
    
    /* Pre-calculate all positions and dimensions */
    UWORD window_width = 400;
    UWORD window_height = 120;
    pw->label_x = 8;
    pw->label_y = 8;
    pw->bar_x = 8;
    pw->bar_y = 35;
    pw->bar_w = window_width - 16;
    pw->bar_h = 18;
    
    /* Calculate percentage position (right-aligned) */
    char temp_percent[] = "100%";
    UWORD percent_text_width = TextLength(&temp_rp, temp_percent, 4);
    pw->percent_x = window_width - 8 - percent_text_width;
    pw->percent_y = 8;
    
    FreeScreenDrawInfo(screen, dri);
    
    /* Open window IMMEDIATELY (no slow operations before this!) */
    pw->window = OpenWindowTags(NULL,
        WA_Left, (screen->Width - window_width) / 2,
        WA_Top, (screen->Height - window_height) / 2,
        WA_Width, window_width,
        WA_Height, window_height,
        WA_Title, task_label,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_Activate, TRUE,
        WA_PubScreen, screen,
        WA_IDCMP, IDCMP_REFRESHWINDOW | IDCMP_INTUITICKS,
        WA_Flags, WFLG_SMART_REFRESH | WFLG_RMBTRAP,
        TAG_END);
    
    if (!pw->window) {
        FreeMem(pw, sizeof(struct ProgressWindow));
        return NULL;
    }
    
    /* Set busy pointer IMMEDIATELY after window opens */
    SetWindowPointer(pw->window,
                     WA_BusyPointer, TRUE,
                     TAG_END);
    
    /* Draw initial empty state */
    DrawBevelBox(pw->window->RPort, pw->bar_x, pw->bar_y, pw->bar_w, pw->bar_h, ...);
    DrawTextLabel(pw->window->RPort, pw->label_x, pw->label_y, task_label);
    DrawPercentage(pw->window->RPort, pw->percent_x, pw->percent_y, "0%");
    
    return pw;  /* Window open and ready for updates! */
}

void CloseProgressWindow(struct ProgressWindow *pw) {
    if (pw) {
        if (pw->window) {
            /* Clear busy pointer before closing */
            SetWindowPointer(pw->window,
                             WA_Pointer, NULL,
                             TAG_END);
            CloseWindow(pw->window);
        }
        FreeMem(pw, sizeof(struct ProgressWindow));
    }
}
```

**User Experience:**
- Click button → Window appears **instantly** (<0.1s)
- Busy pointer shows work in progress
- Progress bar updates as work proceeds
- Clear busy pointer when complete
- **Result:** Feels fast and responsive even on 7MHz systems

**Critical Rules:**
- ⚠️ **ALWAYS** open window BEFORE any slow operations
- ⚠️ **ALWAYS** set busy pointer immediately after opening
- ⚠️ **ALWAYS** clear busy pointer when done (including error paths)
- ⚠️ **ALWAYS** pre-calculate all layout dimensions before opening
- ⚠️ **NEVER** parse files, scan directories, or do I/O before opening window

This pattern is proven in production (folder_view_window.c) and creates professional, responsive applications.

---

## Performance Optimization

### Smart Redrawing Strategy

**Problem:** Progress bars update frequently (50+ times per second possible)  
**Solution:** Only redraw when visually changed

```c
struct ProgressBarState {
    UWORD last_fill_width;     /* Cached fill width in pixels */
    UWORD last_percent_text;   /* Cached percentage value */
    char last_helper_text[256]; /* Cached helper text */
};

/* Only redraw if changed */
UWORD new_fill_width = (bar_width * current) / total;
if (new_fill_width != state->last_fill_width) {
    DrawProgressBarFill(rp, x, y, w, h, new_fill_width);
    state->last_fill_width = new_fill_width;
}

UWORD new_percent = (current * 100) / total;
if (new_percent != state->last_percent_text) {
    /* Clear previous percentage text first */
    ClearTextArea(rp, percent_x, percent_y, old_text_width, font_height, fillPen);
    
    /* Measure new percentage text width with TextLength() */
    char percent_buf[8];
    sprintf(percent_buf, "%d%%", new_percent);
    UWORD new_text_width = TextLength(rp, percent_buf, strlen(percent_buf));
    
    /* Draw new percentage */
    DrawPercentage(rp, percent_x, percent_y, percent_buf);
    state->last_percent_text = new_percent;
}

/* Helper text usually changes every update */
if (strcmp(helper_text, state->last_helper_text) != 0) {
    /* Clear old text */
    ClearTextArea(rp, helper_x, helper_y, helper_max_width, font_height, fillPen);
    
    /* Draw new text */
    DrawTextLabel(rp, helper_x, helper_y, helper_text);
    
    /* Cache for next comparison */
    strncpy(state->last_helper_text, helper_text, sizeof(state->last_helper_text) - 1);
}
```

**Benefits:**
- ✅ Progress bar redraws only when width changes (100 times max for 0%→100%)
- ✅ Percentage text updates only on integer change (100 times max)
- ✅ Helper text only updates when different string
- ✅ Minimizes flicker and CPU usage
- ✅ Smooth animation even on 7MHz Amiga 500

**Critical Rules:**
- ⚠️ **ALWAYS** clear old text before drawing new text (prevents ghosting)
- ⚠️ **ALWAYS** use `TextLength()` to measure text width dynamically
- ⚠️ **NEVER** assume text width - it varies with font and content
- ⚠️ **ALWAYS** compare values before redrawing to avoid unnecessary work

### Update Frequency Guidelines

| Operation | Update Rate | Reason |
|-----------|-------------|--------|
| **Simple progress** | After each item (~1 sec intervals) | Items take time to process |
| **Recursive outer** | After each folder (~2-5 sec intervals) | Folders take time to process |
| **Recursive inner** | After each icon (~20ms intervals) | Icons process quickly |
| **Maximum rate** | 10 updates/second | Avoid unnecessary CPU load |

---

## Integration Points

### Functions That Will Use Status Windows

#### Simple Progress Window

**1. Backup Operations** (`backup_session.c`)
```c
struct ProgressWindow *pw = OpenProgressWindow(
    screen,
    "Creating Backup",
    folderCount
);

for (each folder) {
    UpdateProgress(pw, i, "Archiving: Work:Utilities/");
    BackupFolder(...);
}

CloseProgressWindow(pw);
```

**2. Restore Operations** (`backup_restore.c`)
```c
struct ProgressWindow *pw = OpenProgressWindow(
    screen,
    "Restoring Backup Run 0007",
    catalog->entryCount
);

for (each archive) {
    UpdateProgress(pw, i, "Extracting: 00015.lha");
    ExtractArchive(...);
}

CloseProgressWindow(pw);
```

**3. Catalog Parsing** (`folder_view_window.c`)
```c
struct ProgressWindow *pw = OpenProgressWindow(
    screen,
    "Loading Backup Catalog",
    totalEntries
);

for (each entry) {
    UpdateProgress(pw, i, "Parsing folder entry...");
    ParseCatalogEntry(...);
}

CloseProgressWindow(pw);
```

#### Recursive Progress Window

**1. Recursive Icon Processing** (`layout_processor.c` + main loop)
```c
/* Prescan first */
RecursiveScanResult *scan = PrescanRecursive("Work:WHDLoad");

/* Open dual-bar window */
struct RecursiveProgressWindow *rpw = OpenRecursiveProgress(
    screen,
    "Processing Icons Recursively",
    scan
);

/* Process each folder */
for (i = 0; i < scan->totalFolders; i++) {
    UpdateFolderProgress(rpw, i + 1, scan->folderPaths[i], scan->iconCounts[i]);
    
    iconArray = CreateIconArrayFromPath(scan->folderPaths[i]);
    
    for (j = 0; j < iconArray->count; j++) {
        UpdateIconProgress(rpw, j + 1);
        ProcessIcon(&iconArray->icons[j]);
    }
    
    saveIconsPositionsToDisk(scan->folderPaths[i], iconArray);
    FreeIconArray(iconArray);
}

CloseRecursiveProgress(rpw);
FreeScanResult(scan);
```

---

## Build System Integration

### Makefile Changes

```makefile
# Add StatusWindows subfolder sources
GUI_SOURCES = \
    src/GUI/main_window.c \
    src/GUI/restore_window.c \
    src/GUI/folder_view_window.c \
    src/GUI/StatusWindows/progress_common.c \
    src/GUI/StatusWindows/progress_window.c \
    src/GUI/StatusWindows/recursive_progress.c

# Or use wildcard (cleaner):
GUI_SOURCES = $(wildcard src/GUI/*.c) \
              $(wildcard src/GUI/StatusWindows/*.c)

# Update include paths
CFLAGS = +aos68k -c99 -cpu=68020 -Iinclude -Isrc -Isrc/GUI/StatusWindows
```

---

## Implementation Phases

### Phase 1: Foundation (Common Drawing Code)
**Goal:** Reusable bevel drawing and text rendering

**Tasks:**
- [ ] Create `src/GUI/StatusWindows/` directory
- [ ] Implement `progress_common.h` with structures and constants
- [ ] Implement `DrawBevelBox()` function
- [ ] Implement `DrawProgressBarFill()` function
- [ ] Implement `DrawTextLabel()` and `DrawPercentage()` functions
- [ ] Test bevel rendering on Workbench 2.x and 3.x
- [ ] Verify colors on different themes (NewIcons, MagicWB)

**Deliverable:** Working bevel drawing primitives

---

### Phase 2: Simple Progress Window
**Goal:** Single-bar progress window for backup/restore operations

**Tasks:**
- [ ] Implement `progress_window.h` API
- [ ] Implement `OpenProgressWindow()` function
- [ ] Implement `UpdateProgress()` with smart redrawing
- [ ] Implement `CloseProgressWindow()` with cleanup
- [ ] Test window opening/closing
- [ ] Test progress updates at various speeds

**Integration:**
- [ ] Hook into `RestoreFullRun()` in `backup_restore.c`
- [ ] Hook into `BeginBackupSession()` in `backup_session.c`

**Deliverable:** Working single-bar progress window

---

### Phase 3: Recursive Progress System
**Goal:** Dual-bar progress window with prescan

**Tasks:**
- [ ] Implement `PrescanRecursive()` directory tree walker
- [ ] Implement `RecursiveScanResult` structure and allocation
- [ ] Implement `recursive_progress.h` API
- [ ] Implement `OpenRecursiveProgress()` function
- [ ] Implement `UpdateFolderProgress()` (outer bar)
- [ ] Implement `UpdateIconProgress()` (inner bar)
- [ ] Implement `CloseRecursiveProgress()` with cleanup
- [ ] Implement `FreeScanResult()` memory cleanup
- [ ] Test with small folder tree (10 folders)
- [ ] Test with large folder tree (500+ folders)

**Integration:**
- [ ] Add recursive processing option to main GUI
- [ ] Hook into icon processing loop
- [ ] Test on real hardware (WinUAE + real Amiga)

**Deliverable:** Working dual-bar recursive progress window

---

### Phase 4: Polish & Testing
**Goal:** Production-ready status windows

**Tasks:**
- [ ] Test all operations with progress windows
- [ ] Verify no memory leaks (MuForce testing)
- [ ] Test on Workbench 2.0, 2.1, 3.0, 3.1, 3.9
- [ ] Test on different screen resolutions (640×256, 800×600, 1024×768)
- [ ] Test with custom themes (NewIcons, MagicWB)
- [ ] Optimize redrawing for 7MHz Amiga 500
- [ ] Document user-visible behavior
- [ ] Update main `DEVELOPMENT_LOG.md`

**Deliverable:** Production-ready, tested status windows

---

## Technical Specifications

### Window Flags

```c
/* Simple Progress Window */
WFLG_DRAGBAR |          /* Title bar (can reposition) */
WFLG_DEPTHGADGET |      /* Depth gadget (Z-order) */
WFLG_SMART_REFRESH |    /* Smart refresh (AmigaOS handles backing store) */
WFLG_ACTIVATE |         /* Auto-activate on open */
WFLG_RMBTRAP            /* Block right-click menu */
/* NO CLOSE GADGET - Modal window must complete */
/* NO SIZE GADGET - Fixed size window */
```

**⚠️ CRITICAL: SMART_REFRESH Requires Refresh Handler**
- `WFLG_SMART_REFRESH` means Intuition saves/restores obscured regions
- You **MUST** handle `IDCMP_REFRESHWINDOW` events to redraw when regions are damaged
- Without refresh handling: Visual artifacts appear when other windows drag over progress window
- Refresh handler is lightweight - just redraws current state, doesn't block operation

**IDCMP Flags (Display-Only Window):**
```c
WA_IDCMP, IDCMP_REFRESHWINDOW | IDCMP_INTUITICKS
```

**Why These Flags?**
- **IDCMP_REFRESHWINDOW** - ⚠️ **CRITICAL** for WFLG_SMART_REFRESH windows
  - Prevents artifacts when other windows/apps drag over progress window
  - Must call `GT_BeginRefresh()` / `GT_EndRefresh()` to handle properly
  - Without this: Progress window shows garbage when obscured/revealed
- **IDCMP_INTUITICKS** - Optional: For animation/updates without manual calls
  - Fires at regular intervals (~1/10 second) independent of code execution
  - Perfect for updating animations without blocking the operation
  - **Future Enhancement**: Enables cooperative multitasking-safe periodic redraws
  - Example: Spinner animation continues even when main operation yields via `Delay()`
- **No IDCMP_GADGETUP** - No buttons during operation (display-only)
- **No IDCMP_CLOSEWINDOW** - No close gadget (modal - must complete)

### Modal Behavior

**How to Implement:**
1. Open window on Workbench screen (no custom screen needed)
2. Set busy pointer with pointer type: 
   ```c
   SetWindowPointer(window, 
                    WA_BusyPointer, TRUE,
                    WA_PointerType, POINTERTYPE_BUSY,
                    TAG_END);
   ```
3. Don't process IDCMP events during operation (window is display-only)
4. When operation completes:
   - **Option A: Auto-close** - Clear busy pointer and close window immediately
   - **Option B: Show completion with Close button** (RECOMMENDED) - See below
5. On error: Clear busy pointer and close window

**Future-Proof State Structure:**
```c
struct iTidy_ProgressWindow {
    struct Window *window;
    /* ... existing display fields ... */
    volatile BOOL userCancelled;   /* Reserved for future Cancel button feature */
    /* Adding this NOW prevents ABI breakage when Cancel is implemented later */
};
```
This field is currently unused but reserves space in the structure. When Cancel button support 
is added in the future, no existing code needs recompiling - maintaining binary compatibility.

**Option B: Completion State with Close Button (RECOMMENDED)**

When the operation completes successfully, transition to a "completion state":

```c
void iTidy_ShowCompletionState(struct iTidy_ProgressWindow *pw, BOOL success) {
    /* Clear busy pointer - operation is done */
    SetWindowPointer(pw->window,
                     WA_Pointer, NULL,
                     TAG_END);
    
    /* Update status text to show completion */
    const char *status = success ? "Complete!" : "Failed";
    iTidy_Progress_ClearTextArea(pw->window->RPort, pw->helper_x, pw->helper_y, 
                  pw->helper_max_width, pw->font_height, fillPen);
    iTidy_Progress_DrawTextLabel(pw->window->RPort, pw->helper_x, pw->helper_y, status);
    
    /* Show full-width Close button where progress bar was */
    UWORD button_width = pw->bar_w - 16;  /* Window width - margins */
    UWORD button_height = pw->font_height + 6;
    UWORD button_x = (pw->window->Width - button_width) / 2;  /* Centered */
    UWORD button_y = pw->bar_y;
    
    /* Create Close button gadget */
    struct NewGadget ng;
    ng.ng_LeftEdge = button_x;
    ng.ng_TopEdge = button_y;
    ng.ng_Width = button_width;
    ng.ng_Height = button_height;
    ng.ng_GadgetText = "Close";
    ng.ng_GadgetID = GID_PROGRESS_CLOSE;
    ng.ng_Flags = PLACETEXT_IN;
    
    pw->close_button = CreateGadget(BUTTON_KIND, NULL, &ng, TAG_END);
    
    if (pw->close_button) {
        AddGList(pw->window, pw->close_button, -1, 1, NULL);
        RefreshGList(pw->close_button, pw->window, NULL, 1);
        GT_RefreshWindow(pw->window, NULL);
    }
    
    /* Now window waits for user to click Close button */
    pw->completed = TRUE;
}

/* Event loop now needs to handle IDCMP_GADGETUP */
BOOL iTidy_HandleProgressWindowEvents(struct iTidy_ProgressWindow *pw) {
    struct IntuiMessage *msg;
    BOOL keep_open = TRUE;
    
    while ((msg = GT_GetIMsg(pw->window->UserPort)) != NULL) {
        ULONG msg_class = msg->Class;
        UWORD gadget_id = 0;
        
        if (msg_class == IDCMP_GADGETUP) {
            gadget_id = ((struct Gadget *)msg->IAddress)->GadgetID;
        }
        
        GT_ReplyIMsg(msg);
        
        if (msg_class == IDCMP_GADGETUP && gadget_id == GID_PROGRESS_CLOSE) {
            keep_open = FALSE;  /* User clicked Close */
        }
    }
    
    return keep_open;
}
```

**Visual Transition:**

**During Operation:**
```
┌──────────────────────────────────────────────────┐
│ Restoring Backup...                      45%    │
├──────────────────────────────────────────────────┤
│  ╔═══════════════════════════════════════════╗  │
│  ║███████████████████░░░░░░░░░░░░░░░░░░░░░░░║  │ ← Progress bar
│  ╚═══════════════════════════════════════════╝  │
│  Extracting: 00015.lha                          │
│  [Busy pointer visible]                         │
└──────────────────────────────────────────────────┘
```

**After Completion:**
```
┌──────────────────────────────────────────────────┐
│ Restoring Backup...                     100%    │
├──────────────────────────────────────────────────┤
│                                                  │
│            [ Close ─────────────────── ]         │ ← Full-width Close button
│                                                  │
│  Complete!                                      │
│  [Normal pointer - no longer busy]              │
└──────────────────────────────────────────────────┘
```

**Benefits of Completion State:**
- ✅ User sees final status ("Complete!" or error message)
- ✅ Gives user a moment to acknowledge completion
- ✅ Matches behavior of system requesters (user must click OK)
- ✅ Professional feel - no sudden disappearance
- ✅ User can leave window open to verify completion before dismissing

**When to Use Each Approach:**

| Pattern | Use Case | User Experience |
|---------|----------|----------------|
| **Auto-close** | Very fast operations (<5 seconds) | Window disappears immediately - minimal interruption |
| **Completion state** | Longer operations (>5 seconds) | User acknowledges completion - more control |
| **Completion state** | Operations that might fail | User can read error message before closing |
| **Completion state** | Important operations (backups, restores) | User wants confirmation it succeeded |

**IDCMP Flags Update for Completion State:**
```c
/* During operation: display-only */
WA_IDCMP, IDCMP_REFRESHWINDOW | IDCMP_INTUITICKS

/* After adding Close button: needs event handling */
ModifyIDCMP(window, IDCMP_REFRESHWINDOW | IDCMP_GADGETUP);
```

**⚠️ CRITICAL: Refresh Handling During Operation**

Since the window uses `WFLG_SMART_REFRESH` and has no gadgets during operation, you must handle `IDCMP_REFRESHWINDOW` events to prevent artifacts when other windows drag over it.

**Lightweight Refresh Handler:**
```c
void iTidy_Progress_HandleRefresh(struct iTidy_ProgressWindow *pw) {
    struct IntuiMessage *msg;
    
    /* Non-blocking check for refresh events only */
    while ((msg = (struct IntuiMessage *)GetMsg(pw->window->UserPort)) != NULL) {
        ULONG msg_class = msg->Class;
        ReplyMsg((struct Message *)msg);
        
        if (msg_class == IDCMP_REFRESHWINDOW) {
            /* Handle refresh - redraw progress bar and text */
            GT_BeginRefresh(pw->window);
            
            /* Redraw window contents (bevel box, progress bar, text) */
            /* Redraw function here should call iTidy_Progress_DrawBevelBox, etc. */
            
            GT_EndRefresh(pw->window, TRUE);
        }
    }
}

/* Call periodically during operation */
void iTidy_UpdateProgress(struct iTidy_ProgressWindow *pw, UWORD current, const char *helper) {
    /* Update progress bar and text */
    /* ... drawing code ... */
    
    /* Check for refresh events to prevent artifacts */
    iTidy_Progress_HandleRefresh(pw);
}
```

**Why This Matters:**
- Without refresh handling: Dragging another window over progress window leaves visual garbage
- With refresh handling: Progress window redraws cleanly when revealed
- Lightweight: Only processes refresh events, doesn't block operation
- Essential for professional appearance on multi-tasking Amiga systems

**Alternative: Simple Refresh Handler in Update Loop:**
```c
void iTidy_UpdateProgress(struct iTidy_ProgressWindow *pw, UWORD current, const char *helper) {
    struct IntuiMessage *msg;
    
    /* Update progress display */
    iTidy_Progress_DrawBarFill(pw->window->RPort, ...);
    iTidy_Progress_DrawTextLabel(pw->window->RPort, ...);
    
    /* Handle any pending refresh events (non-blocking) */
    while ((msg = (struct IntuiMessage *)GetMsg(pw->window->UserPort)) != NULL) {
        if (msg->Class == IDCMP_REFRESHWINDOW) {
            GT_BeginRefresh(pw->window);
            /* Re-render entire window */
            iTidy_Progress_DrawBevelBox(pw->window->RPort, ...);
            iTidy_Progress_DrawBarFill(pw->window->RPort, ...);
            iTidy_Progress_DrawTextLabel(pw->window->RPort, ...);
            GT_EndRefresh(pw->window, TRUE);
        }
        ReplyMsg((struct Message *)msg);
    }
}
```

**Result:** User can see progress during operation (busy pointer), window refreshes cleanly when obscured, then sees final status and must click Close button to dismiss (normal pointer).

---

## Error Handling

### Graceful Degradation

**If window fails to open:**
```c
struct iTidy_ProgressWindow *pw = iTidy_OpenProgressWindow(...);
if (!pw) {
    /* Fall back to console output */
    printf("Processing: %s\n", status);
    return;
}
```

**If operation fails midway:**
```c
if (error) {
    iTidy_CloseProgressWindow(pw);
    DisplayError("Operation failed: Out of disk space");
    return FALSE;
}
```

**Memory allocation failures:**
```c
/* Check all allocations */
if (!pw || !scan || !folderPaths) {
    /* Cleanup partial allocations */
    if (pw) iTidy_CloseProgressWindow(pw);
    if (scan) iTidy_FreeScanResult(scan);
    return NULL;
}
```

---

## Future Enhancements

### Potential Additions (Not in Initial Implementation)

1. **Cancel Button**
   - Requires: Thread-safe cancellation flag
   - Challenge: LHA extraction cannot be interrupted safely
   - Solution: Check flag between operations (between archives, not during)

2. **Time Remaining Estimate**
   - Calculate: `(totalTime / itemsCompleted) * itemsRemaining`
   - Display: "Estimated time remaining: 2m 34s"
   - Challenge: Accuracy depends on consistent item processing time

3. **Speed Display**
   - Calculate: `bytesProcessed / elapsedSeconds`
   - Display: "Processing at 145 KB/sec"
   - Useful for large file operations

4. **Pause/Resume**
   - Requires: State saving and restoration
   - Complex for multi-step operations
   - Low priority (operations are typically fast)

5. **Multiple Simultaneous Progress Windows**
   - For parallel operations (if ever implemented)
   - Requires: Queue management and coordination
   - Not needed for current iTidy architecture

---

## References

### Amiga OS Documentation
- **Intuition Manual** - Window management and graphics
- **GadTools Manual** - Standard UI gadgets (reference for look & feel)
- **Graphics Library** - RastPort drawing primitives
- **AmigaOS Style Guide** - UI consistency guidelines

### Real-World Examples
- **Windows File Copy Dialog** - Dual progress (file + overall)
- **macOS Finder Copy** - Single progress with time estimate
- **WinRAR/WinZIP** - Progress with file list and speed
- **Workbench Format Disk** - Simple progress with percentage

---

## Appendix: Code Snippets

### Bevel Box Drawing (Reference Implementation)

```c
void iTidy_Progress_DrawBevelBox(
    struct RastPort *rp,
    WORD left, WORD top, WORD width, WORD height,
    ULONG shine_pen, ULONG shadow_pen, ULONG fill_pen,
    BOOL recessed  /* TRUE = sunken, FALSE = raised */
)
{
    /* Outer bevel - 2 pixels wide */
    SetAPen(rp, recessed ? shadow_pen : shine_pen);
    Move(rp, left, top + height - 1);          /* Bottom-left */
    Draw(rp, left, top);                        /* Top-left */
    Draw(rp, left + width - 1, top);            /* Top-right */
    
    SetAPen(rp, recessed ? shine_pen : shadow_pen);
    Draw(rp, left + width - 1, top + height - 1);  /* Bottom-right */
    Draw(rp, left, top + height - 1);              /* Bottom-left */
    
    /* Inner bevel - 1 pixel inside */
    SetAPen(rp, recessed ? shadow_pen : shine_pen);
    Move(rp, left + 1, top + height - 2);
    Draw(rp, left + 1, top + 1);
    Draw(rp, left + width - 2, top + 1);
    
    SetAPen(rp, recessed ? shine_pen : shadow_pen);
    Draw(rp, left + width - 2, top + height - 2);
    Draw(rp, left + 1, top + height - 2);
    
    /* Fill interior */
    SetAPen(rp, fill_pen);
    RectFill(rp, left + 2, top + 2, left + width - 3, top + height - 3);
}
```

### Progress Bar Fill (Reference Implementation)

```c
void iTidy_Progress_DrawBarFill(
    struct RastPort *rp,
    WORD left, WORD top, WORD width, WORD height,
    ULONG bar_pen, ULONG fill_pen,
    UWORD percent  /* 0-100 */
)
{
    /* Calculate fill width (inside 2px bevel) */
    WORD interior_w = width - 4;
    WORD fill_w = (interior_w * percent) / 100;
    
    /* Draw gray unfilled background first */
    SetAPen(rp, fill_pen);
    RectFill(rp, left + 2, top + 2, left + width - 3, top + height - 3);

    /* Draw blue fill portion */
    if (fill_w > 0) {
        SetAPen(rp, bar_pen);
        RectFill(rp, left + 2, top + 2, left + 2 + fill_w - 1, top + height - 3);
    }
}
```

### Text Clearing Helper (NEW - Essential for Clean Updates)

```c
void iTidy_Progress_ClearTextArea(
    struct RastPort *rp,
    WORD left, WORD top,
    UWORD rect_width, UWORD rect_height,
    ULONG bg_pen
)
{
    /* Fill rectangle with background color to erase old text */
    SetAPen(rp, bg_pen);
    RectFill(rp, left, top, left + rect_width - 1, top + rect_height - 1);
}
```

### Text Drawing with Proper Measurements (NEW)

```c
void iTidy_Progress_DrawTextLabel(
    struct RastPort *rp,
    WORD left, WORD top,
    const char *text,
    ULONG text_pen
)
{
    SetAPen(rp, text_pen);
    Move(rp, left, top + rp->Font->tf_Baseline);
    Text(rp, text, strlen(text));
}

void iTidy_Progress_DrawPercentage(
    struct RastPort *rp,
    WORD right_x,  /* Right edge coordinate for right-alignment */
    WORD top,
    const char *percent_text,  /* e.g., "45%" */
    ULONG text_pen
)
{
    /* Measure text width for right-alignment */
    UWORD text_width = TextLength(rp, percent_text, strlen(percent_text));
    
    /* Calculate left edge for right-alignment */
    WORD text_x = right_x - text_width;
    
    /* Draw text */
    SetAPen(rp, text_pen);
    Move(rp, text_x, top + rp->Font->tf_Baseline);
    Text(rp, percent_text, strlen(percent_text));
}
```

---

**End of Status Windows Design Specification**

**Next Steps:** Proceed to Phase 1 implementation (common drawing code) when ready.
