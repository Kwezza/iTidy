# 68000 Displacement Error: Root Cause, Risk Analysis, and Refactoring Plan

**File affected:** `src/GUI/main_window.c`  
**Error:** `error 2030: displacement out of range`  
**Date discovered:** February 2026  
**Status:** Fixed (immediate fix applied) — partial refactoring recommended

---

## 1. What Is the 68000 Displacement Error?

### The CPU constraint

The Motorola 68000 uses **16-bit signed integers** for memory addressing displacements. This means any address that is reached via a displacement from a register (such as the stack pointer `A7` or the program counter `PC`) must be within the range:

```
-32,768 to +32,767 bytes  (≈ ±32 KB)
```

This applies to two critical instruction forms used constantly by compiled C code:

| Instruction form | Used for |
|-----------------|----------|
| `move.l d16(A7), Ax` | Reading a local variable from the stack |
| `lea d16(PC), Ax` | Loading the address of a string literal or static data |

On the 68020+, these constraints are relaxed — `d16` displacements can be extended to 32-bit (`d32`). But iTidy targets **68000 minimum** for maximum compatibility, so the 16-bit limit applies to every function frame.

### How VBCC creates a stack frame

VBCC (and most C89 compilers) allocates **all local variables for an entire function at function entry**, in one contiguous block, regardless of scope or which `case` / `if` branch is actually taken at runtime. Consider:

```c
void my_function(void)
{
    switch (x)
    {
        case A: { BigThing a; use(&a); } break;
        case B: { BigThing b; use(&b); } break;
        case C: { BigThing c; use(&c); } break;
    }
}
```

At runtime only one `BigThing` is ever live. But VBCC still reserves space for **all three** on the stack when the function is called, because:

- The compiler performs only limited lifetime analysis
- On 68k, stack space is cheap, safe, and deterministic
- The cost is invisible — until the total frame size exceeds 32 KB

---

## 2. What Actually Happened in iTidy

### The offending function: `handle_menu_selection`

After the new 4-menu system (Presets / Settings / Tools / Help) was added, `handle_menu_selection` contained **four** block-scoped `LayoutPreferences` locals in separate `case` branches:

```c
case MENU_SETTINGS_ADVANCED:    { LayoutPreferences temp_prefs;    ...  } break;
case MENU_SETTINGS_DEFI_CATS:   { LayoutPreferences working_copy; ...  } break;
case MENU_SETTINGS_DEFI_PREVIEW:{ LayoutPreferences working_copy; ...  } break;
case MENU_SETTINGS_DEFI_EXCLUDE:{ LayoutPreferences working_copy; ...  } break;
```

#### The size problem

`LayoutPreferences` is a large struct due to this field:

```c
char deficons_exclude_paths[32][256];    /* 8,192 bytes alone */
```

Total struct size: **~8,928 bytes** (8.7 KB)

| Instances | Total frame contribution |
|-----------|--------------------------|
| 1 | 8,928 bytes |
| 2 | 17,856 bytes |
| 3 | 26,784 bytes |
| **4** | **35,712 bytes** ← exceeds 32,767 limit |

Adding the function's other locals (pointers, BOOL flags, etc.) pushed the total frame past 36 KB. VBCC emitted stack-relative addresses like `move.l (36664+l793,a7),a5` — the displacement `36664` is beyond INT16_MAX, so the assembler correctly issued:

```
error 2030 in line 9444: displacement out of range
>   move.l  (36664+l793,a7),a5
***maximum number of errors reached!***
```

### Secondary problem: `handle_gadget_event`

The gadget event handler also had two stack-allocated `LayoutPreferences` instances (in the ADVANCED_BUTTON and ICON_CREATION_BUTTON cases), contributing ~17.9 KB to its frame. This was just within the limit at the time, but was heading toward the same cliff as the struct grows.

### The fix applied

All six instances were converted from stack allocation to heap allocation using `AllocVec/FreeVec`:

```c
/* Before (stack — dangerous) */
LayoutPreferences temp_prefs;
memcpy(&temp_prefs, GetGlobalPreferences(), sizeof(LayoutPreferences));

/* After (heap — safe) */
LayoutPreferences *temp_prefs = (LayoutPreferences *)AllocVec(sizeof(LayoutPreferences), MEMF_ANY|MEMF_CLEAR);
if (!temp_prefs) break;
memcpy(temp_prefs, GetGlobalPreferences(), sizeof(LayoutPreferences));
...
FreeVec(temp_prefs);
```

Additionally, the two log utility functions (`handle_menu_open_log_folder`, `handle_menu_archive_logs`) were split into a new file `src/GUI/main_window_log_handlers.c` to reduce the total code size of `main_window.c`.

---

## 3. Future Risk Assessment

### 3.1 `LayoutPreferences` struct growth (HIGH RISK)

The struct is already at v5 format and is explicitly designed to grow. Each new feature typically adds several bytes. The current size (~8,928 bytes) means:

| Stack copies in one function frame | Safe headroom remaining |
|-----------------------------------|-------------------------|
| 1 | ~23,839 bytes remaining |
| 2 | ~14,911 bytes remaining |
| **3** | **~5,983 bytes remaining** ← danger zone |
| 4 | **OVERFLOW** |

**Danger threshold**: Any function that allocates **3 or more** `LayoutPreferences` on the stack simultaneously will become fragile and break if the struct grows by even ~2 KB more.

#### Remaining stack-allocated instances in main_window.c

| Function | Line | Variable | Risk |
|----------|------|----------|------|
| `handle_tooltype_loadprefs` | ~1431 | `LayoutPreferences loaded_prefs` | LOW — only 1 per function |
| `handle_main_new_menu` | ~1561 | `LayoutPreferences default_prefs` | LOW — only 1 per function |
| `handle_main_open_menu` | ~1755 | `LayoutPreferences loaded_prefs` | LOW — only 1 per function |

These are individually safe today, but should be converted to heap allocation **before** the struct grows significantly further.

### 3.2 `handle_gadget_event` frame accumulation (MEDIUM RISK)

Even with `LayoutPreferences` moved to the heap, `handle_gadget_event` still has large sub-structs declared in `case` blocks:

```c
case ITIDY_GAID_ADVANCED_BUTTON:   { struct iTidyAdvancedWindow adv_data; ... }
case ITIDY_GAID_START_BUTTON:      { struct iTidyMainProgressWindow progress_window; ... }
case ITIDY_GAID_RESTORE_BUTTON:    { struct iTidyRestoreWindow restore_data; ... }
```

These structs should be audited for size. If any approach several KB, this function becomes the next candidate for the same overflow.

### 3.3 `open_itidy_main_window` (MEDIUM RISK)

This single function is **436 lines** long (lines 516–952). It constructs the entire ReAction object tree inline and contains long static/local data arrays for gadget hints. Its frame size has not triggered errors yet, but its length makes it opaque — a future addition to the hint table or gadget tree could silently push it over.

### 3.4 `-cpu=68000` target locking in the limit

The Makefile compiles with `-cpu=68000`. Switching to `-cpu=68020` would eliminate the ±32 KB constraint entirely (the 68020 supports 32-bit displacements). However, that would break compatibility with stock A500/A1000/A2000 hardware. If the project ever officially drops 68000 support, this category of errors disappears. For now it remains a hard constraint.

---

## 4. Does `main_window.c` Need Refactoring?

### Current state

| Metric | Value | Assessment |
|--------|-------|------------|
| Total lines | 2,477 | Large but manageable |
| Largest function (`open_itidy_main_window`) | ~436 lines | Too long — hard to read |
| Second largest (`handle_gadget_event`) | ~296 lines | Acceptable |
| Third largest (`handle_menu_selection`) | ~262 lines | Acceptable |
| Stack-allocated `LayoutPreferences` remaining | 3 instances | Low risk today, monitor |
| Distinct responsibilities in the file | 5+ | Over-coupled |

**Answer: Yes, refactoring is recommended** — not urgently, but the file is accumulating concerns that will become maintenance pain.

### Recommended split

The file currently handles these distinct responsibilities:

1. **ReAction library management** — `init_reaction_libs`, `cleanup_reaction_libs`, `ShowReActionRequester`
2. **Main window open/close** — `open_itidy_main_window`, `close_itidy_main_window`
3. **Event dispatch** — `handle_itidy_window_events`
4. **Preferences I/O** — `save_preferences_to_file`, `load_preferences_from_file`, `sync_gui_from_preferences`, `sync_gui_to_preferences`, `handle_tooltype_loadprefs`
5. **Menu handlers** — all `handle_main_*_menu`, `handle_menu_selection`, helpers
6. **Gadget handlers** — `handle_gadget_event`

A clean split would be:

| Proposed file | Contents | Est. lines |
|---------------|----------|------------|
| `main_window.c` | Responsibilities 1, 2, 3 (window lifecycle + events) | ~600 |
| `main_window_prefs_io.c` | Responsibility 4 (preferences save/load/sync) | ~500 |
| `main_window_menu_handlers.c` | Responsibility 5 (all menu handling) | ~400 |
| `main_window_gadget_handlers.c` | Responsibility 6 (all gadget handling) | ~350 |
| `main_window_log_handlers.c` | Log folder/archive handlers (already split) | ~150 |

This would reduce `main_window.c` to under 600 lines — well below any displacement threshold even with future growth.

### Priority

| Action | Priority | Reason |
|--------|----------|--------|
| Convert remaining 3 stack `LayoutPreferences` to heap | **Medium** | Cheap insurance before struct grows further |
| Audit sizes of `iTidyAdvancedWindow`, `iTidyMainProgressWindow`, `iTidyRestoreWindow` | **Medium** | `handle_gadget_event` frame risk |
| Split `main_window_prefs_io.c` | **Low** | Logical separation, reduces file size |
| Split `main_window_menu_handlers.c` | **Low** | Further reduces main_window.c size |
| Split `main_window_gadget_handlers.c` | **Low** | Completes the decomposition |
| Switch to `-cpu=68020` | **Deferred** | Only if 68000 support is officially dropped |

---

## 5. Design Rules Going Forward

To prevent this class of error from recurring:

1. **Never stack-allocate `LayoutPreferences` in a function that has other large locals.** Always use `AllocVec/FreeVec`. It is ~8.7 KB — larger than many entire C programs.

2. **The "two large struct" rule**: If a function needs two or more structs that each exceed 4 KB, at least one must be heap-allocated.

3. **When adding fields to `LayoutPreferences`**, check whether any function now has 3+ instances in its frame. A quick search for `LayoutPreferences [a-z]` across `src/GUI/` will reveal all stack instances.

4. **Treat VBCC error 2030 as a design smell, not just a compiler error.** It always means a function is carrying too much state — which usually also means it is doing too much.

5. **Consider adding a compile-time size guard** to catch unexpected struct growth early:

```c
/* In layout_preferences.h — fail the build if struct exceeds 12 KB */
/* This gives a compile-time alert before any displacement issues arise */
typedef char _lp_size_check[ (sizeof(LayoutPreferences) <= 12288) ? 1 : -1 ];
```

---

*Document created: February 27, 2026*  
*Relates to: `src/GUI/main_window.c`, `src/GUI/main_window_log_handlers.c`, `src/layout_preferences.h`*
