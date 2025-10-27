# Debugging NULL Pointer Crash at Offset 0x0000A6B8

**Date:** October 27, 2025  
**Crash Type:** NULL pointer dereference (WORD READ from 0x000000FC)  
**Location:** Hunk 0000 Offset 0x0000A6B8  
**Status:** 🔍 INVESTIGATING

---

## Crash Information

```
WORD READ from 000000FC                        
PC: 40A426C0
USP: 404CDC98 SR: 0010  (U0)(-)(-)  TCB: 404C53B8
Data: 4012BA38 40A51914 0000000C 00000001 00004E20 0000000C 00000001 40A38004
Addr: 4012BA38 40A51914 40A518E4 40468B30 4047082E 404C9070 400008D4 400022F4
Stck: 40A426CE 0000000C 00000001 00004E20 0000000C 40A518E4 400008D4 40A38562
Stck: 0000000C 00000001 00004E20 0000000C 40A518E4 400008D4 00F05800 600E7044
Name: "Background CLI"  CLI: "iTidy"  
Hunk 0000 Offset 0000A6B8
```

### Analysis:
- **0x000000FC** = 252 bytes from NULL
- This is accessing a structure field at offset 252
- Likely: `struct Window->UserPort` (offset 0xFC)
- The window pointer is NULL!

---

## 🎮 Method 1: Using WinUAE Built-in Debugger

### Step 1: Enable Debug Mode in WinUAE

1. Start WinUAE
2. Go to **Miscellaneous** → **Debugger Settings**
3. Check **"Enable debugger"**
4. Set **Break on:** "Illegal instructions" and "Address errors"
5. Click **OK**

### Step 2: Load iTidy with Debug Symbols

1. Boot your Amiga configuration
2. Open Shell/CLI
3. Navigate to where iTidy is located
4. Run: `iTidy`

### Step 3: When the Crash Occurs

The debugger will automatically break. Press **Shift+F12** to open the debugger window.

### Step 4: Find the Function Name

In the WinUAE debugger console, type:

```
> i 0x0000A6B8
```

This will show you the symbol/function name at offset 0xA6B8 in the first hunk.

Or look at the disassembly around PC:

```
> d 40A426B0 40
```

This disassembles 64 bytes starting just before the crash address.

### Step 5: Find What Register Contains NULL

Look at the instruction at PC (40A426C0). It will be something like:

```
move.w  $FC(A0),D0      ; Reading from A0+0xFC
```

Check what's in the address register:

```
> r
```

Look for which register (A0, A1, etc.) contains `00000000` or a very small value.

### Step 6: Examine the Stack

```
> s 30
```

This shows 30 longwords of stack. Look for:
- Return addresses (look like code addresses: 0x00F9xxxx range)
- Function call patterns

---

## 🔬 Method 2: Using MonAm (on Real Amiga)

If you're on real hardware with MonAm:

1. Run MonAm
2. Load iTidy: `L iTidy`
3. Find symbol at offset: `? 0xA6B8`
4. Set breakpoint: `B 0xA6B8`
5. Run: `G`

---

## 🔍 Method 3: Manual Analysis (What We Know)

### File Size Analysis

The iTidy executable is **213,456 bytes** with debug symbols.

Hunk 0000 offset **0xA6B8** (42,680 bytes) is approximately **20%** into the code section.

### Common Functions at This Point:

Based on typical AmigaOS C program layout:
1. **C runtime initialization** (first ~5KB)
2. **Library opens** (next ~10KB)
3. **Early main() code** (around 15-25KB)
4. **GUI initialization functions** (20-40KB) ← **YOU ARE HERE**
5. **Main processing functions** (40KB+)

### Likely Culprits at Offset 0xA6B8:

Looking at the code structure, functions likely to be at this offset:

1. **`open_itidy_main_window()`** - GUI window setup
2. **`create_gadgets()`** - Gadget creation
3. **`GetVisualInfo()`** calls
4. **Font/screen initialization**

### The 0x000000FC Pattern:

In `struct Window`:
- Offset 0xFC is likely **`window->UserPort`**

In `struct Screen`:  
- Offset varies, but could be similar

This suggests: **Accessing `window->UserPort` when `window` is NULL**

---

## 📋 Quick Checklist for Finding the Bug

Without examining code yet, use the debugger to:

- [ ] Open WinUAE debugger (Shift+F12)
- [ ] Note the instruction at PC (40A426C0)
- [ ] Identify which register is NULL
- [ ] Get function name from hunk offset 0xA6B8
- [ ] Look at stack for calling function
- [ ] Check if it's in `open_itidy_main_window()` or `handle_itidy_window_events()`

---

## 🎯 Expected Findings

Based on the offset and NULL pointer at 0xFC, you should find:

**Function:** Probably `handle_itidy_window_events()` or similar  
**Line:** Something like:
```c
WaitPort(win_data->window->UserPort);  // window is NULL!
```

**Root Cause:** 
- Window opening failed
- But code continued anyway
- Then tried to access the NULL window pointer

---

## 🔧 Once You Find It

After identifying the exact function/line with the debugger:

1. Note the function name from the debugger
2. Note any nearby symbol names
3. Check for missing NULL checks
4. Verify error handling in window opening code

---

## 📞 Debug Session Template

Fill this out as you debug:

```
=== DEBUG SESSION ===
Date: _____________
Binary: iTidy (213,456 bytes)
Hunk Offset: 0x0000A6B8

Debugger Output:
-----------------
Function name at 0xA6B8: _______________
Instruction at PC: _______________
NULL register: _______________
Stack trace (top 5):
  1. _______________
  2. _______________
  3. _______________
  4. _______________
  5. _______________

Conclusion:
-----------
Bug is in function: _______________
Variable that is NULL: _______________
Fix needed: _______________
```

---

## 🚀 After Finding the Bug

Once you identify the function:
1. Report back the function name
2. I'll help you add proper NULL checks
3. We'll rebuild and test

---

## Additional Tools

### WinUAE Debugger Commands Reference

```
> ?                  - Help
> r                  - Show registers  
> s [count]          - Show stack
> d [addr] [count]   - Disassemble
> m [addr] [count]   - Memory dump
> i [offset]         - Info about symbol at offset
> t                  - Step (trace) one instruction
> z                  - Continue execution
> b [addr]           - Set breakpoint
```

### Useful Memory Addresses

```
SysBase:    0x00000004
ExecBase:   0x00000004
DOSBase:    Varies (check Data registers)
IntuiBase:  Varies (check Data registers)
```

---

Good luck with the debugging session! 🐛🔍
