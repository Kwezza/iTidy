# Quick Start: Running Status Windows Tests on Your Amiga

## Step 1: Build the Test Programs (Windows)

```bash
cd C:\Amiga\Programming\iTidy
make -f Makefile.tests
```

**Output:**
- `Bin/Amiga/Tests/test_progress_window` (25.7 KB)
- `Bin/Amiga/Tests/test_recursive_progress` (29.9 KB)

## Step 2: Transfer to Your Amiga

### Using WinUAE (Easiest Method)

1. **Configure shared folder in WinUAE:**
   - Settings → Hardware → CD & Hard Drives
   - Add Directory: Select `C:\Amiga\Programming\iTidy\Bin\Amiga\Tests`
   - Device Name: `TESTS:` (or any name you prefer)
   - Read/Write access enabled
   - Boot priority: 0

2. **In WinUAE Amiga:**
   ```bash
   CD TESTS:
   LIST
   ```
   You should see `test_progress_window` and `test_recursive_progress`

### Using Real Amiga

**Option A: Network Transfer (if you have AmiTCP/Miami)**
```bash
# On Windows (from iTidy directory)
ftp your-amiga-ip
cd Work:Tests
put Bin/Amiga/Tests/test_progress_window
put Bin/Amiga/Tests/test_recursive_progress
quit
```

**Option B: Copy to ADF/DMS Image**
1. Use ADFOpus or similar tool on Windows
2. Create new ADF file or open existing one
3. Copy test programs to ADF
4. Transfer ADF to Amiga via CF card/USB/Network

**Option C: Serial Transfer**
1. Use ZModem or similar protocol
2. Transfer files over serial cable

## Step 3: Make Executable (if needed)

```bash
# On Amiga Shell/CLI
PROTECT test_progress_window +e
PROTECT test_recursive_progress +e
```

## Step 4: Run the Tests

### Test 1: Simple Progress Window

```bash
# From Amiga Shell
test_progress_window
```

**What happens:**
1. Window appears with title "Auto-Close Test"
2. Progress bar fills from 0% to 100%
3. Shows "Processing item XX" text
4. Window closes automatically
5. Console prompts: "Press RETURN to continue..."
6. Press RETURN
7. Next test runs with "Completion State Test"
8. Progress bar fills to 100%
9. Close button appears - click it
10. Repeat for failure scenario

**Total time:** ~30 seconds

### Test 2: Recursive Progress Window

```bash
# Test with default path (SYS:)
test_recursive_progress

# Or specify a path
test_recursive_progress SYS:Utilities
test_recursive_progress Work:
test_recursive_progress RAM:
```

**What happens:**
1. Console shows: "Prescanning directory tree..."
2. Wait 1-5 seconds depending on size
3. Console shows: "Found X folders, Y icons"
4. Press RETURN when prompted
5. Dual-bar window appears
6. Top bar (Folders) fills gradually
7. Current folder path displayed
8. Bottom bar (Icons) fills quickly for each folder
9. Both bars reach 100%
10. Console prompts: "Press RETURN to close"
11. Window closes

**Time varies by path size:**
- RAM: (10 folders) ~ 10 seconds
- SYS:Utilities (50 folders) ~ 30 seconds  
- Work: (500 folders) ~ 2 minutes

## Expected Visual Output

### Simple Window (during operation)
```
┌────────────────────────────────────────┐
│ Restoring Backup...          45%      │
├────────────────────────────────────────┤
│  ╔═══════════════════════════════════╗ │
│  ║███████████░░░░░░░░░░░░░░░░░░░░░░░║ │
│  ╚═══════════════════════════════════╝ │
│  Processing item 09                    │
└────────────────────────────────────────┘
```

### Recursive Window (during operation)
```
┌────────────────────────────────────────┐
│ Processing Icons Recursively    35%   │
├────────────────────────────────────────┤
│  Folders:  ╔════════════╗  17/50      │
│            ║███████░░░░░║             │
│            ╚════════════╝             │
│  SYS:Utilities/Games/                 │
│  Icons:    ╔════════════╗   8/23      │
│            ║███░░░░░░░░░║             │
│            ╚════════════╝             │
└────────────────────────────────────────┘
```

## Console Output Example

```
===========================================
iTidy Recursive Progress Window Test
===========================================

Target path: SYS:Utilities

Workbench screen locked: 640x256 pixels

Phase 1: Prescanning directory tree...
(This may take a few seconds for large trees)

Prescan complete!
  Total folders: 23
  Total icons:   187

Press RETURN to start processing...

Phase 2: Opening recursive progress window...
Progress window opened successfully.

Phase 3: Processing folders...
  [1/23] SYS:Utilities (8 icons)
  [2/23] SYS:Utilities/Clock (3 icons)
  [3/23] SYS:Utilities/MultiView (5 icons)
  ...

Processing complete!

Press RETURN to close progress window...

Phase 5: Cleaning up...

===========================================
Test complete!
===========================================
```

## Troubleshooting

### Problem: "Command not found"
**Solution:** 
```bash
CD <directory-with-tests>
./test_progress_window
```
Or add to path:
```bash
PATH TESTS: ADD
```

### Problem: Window doesn't appear
**Check:**
- Workbench is running
- Enough memory: `AVAIL` should show >500KB free
- Not running from Workbench (use Shell/CLI)

### Problem: Crashes or gurus
**Try:**
- Close other programs to free memory
- Test on smaller directory
- Reboot Amiga
- Check VBCC compilation was successful

### Problem: Colors look wrong
**This is normal!** The windows adapt to your Workbench theme. They will look different on:
- Standard Workbench (blue/white)
- MagicWB (gray/3D look)
- NewIcons themes
- Custom screen modes

All are correct - the code uses DrawInfo to match your system.

## Performance Tips

**For faster testing:**
- Use RAM: for quick tests (small tree)
- Use SYS:C or SYS:Utilities for medium test
- Avoid SYS:Devs (many hidden folders)

**For stress testing:**
- Use Work: drive if you have many files
- Use WHDLoad directory if installed (100s of folders)

## What's Being Tested?

1. **Window opening speed** - Should be instant (<0.1s)
2. **Progress bar drawing** - Smooth, no flicker
3. **Text updates** - Clear, no ghosting
4. **Theme compatibility** - Works with your colors
5. **Refresh handling** - Try dragging another window over it
6. **Memory usage** - Should not crash even on 512 KB system
7. **System responsiveness** - System should stay usable
8. **Completion state** - Close button works correctly

## Next Steps

After successful testing:
1. These windows are ready to integrate into iTidy
2. Same API can be used in your own programs
3. Check PHASE2_PROGRESS_WINDOW_COMPLETE.md for integration examples
4. Check PHASE3_RECURSIVE_PROGRESS_COMPLETE.md for recursive usage

## Files on Amiga

```
TESTS:
├── test_progress_window      (25.7 KB)
└── test_recursive_progress   (29.9 KB)
```

Both are standalone executables - no additional files needed!

---

**Have fun testing!** 🎉

If everything works, you'll see smooth, professional progress windows that adapt to your system and keep everything responsive.
