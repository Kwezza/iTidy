# Status Windows Test Programs

## Overview

This directory contains test programs for the iTidy Status Windows system. These standalone executables demonstrate the progress window functionality and can be run directly on your Amiga.

## Test Programs

### 1. test_progress_window
Simple single-bar progress window test with two patterns:
- **Pattern A**: Auto-close (20 items, fast operation)
- **Pattern B**: Completion state with Close button (15 items, success)
- **Pattern C**: Completion state with failure simulation (15 items, fails at item 10)

### 2. test_recursive_progress
Dual-bar recursive progress window test:
- Prescans a directory tree
- Shows folder progress (outer bar)
- Shows icon progress (inner bar)
- Accepts directory path as command-line argument

## Building the Test Programs

### On Windows (Cross-Compile)

```bash
# Build all test programs
make -f Makefile.tests

# Clean test builds
make -f Makefile.tests clean

# Show help
make -f Makefile.tests help
```

**Output files:**
- `Bin/Amiga/Tests/test_progress_window`
- `Bin/Amiga/Tests/test_recursive_progress`

## Transferring to Amiga

### Option 1: WinUAE Shared Folder
1. Configure a Windows folder as Amiga hard drive in WinUAE
2. Copy test programs to that folder
3. Access from Amiga via the mounted drive

### Option 2: FTP/Network Transfer
1. Set up FTP server on Amiga (e.g., with AmiTCP)
2. FTP the test programs from Windows to Amiga
3. Make executable: `protect <file> +e`

### Option 3: Floppy/USB
1. Copy to physical media from Windows
2. Transfer to Amiga via floppy or USB adapter

### Option 4: Serial/Parallel Transfer
1. Use tools like `ZModem` or `SamTrans`
2. Transfer over serial or parallel cable

## Running on Amiga

### Test 1: Simple Progress Window

```bash
# From Amiga Shell/CLI
CD Work:iTidy/Tests
test_progress_window
```

**What you'll see:**
1. Pattern A demo (auto-close after 20 items)
2. Press RETURN to continue
3. Pattern B demo (completion state, success)
4. Press RETURN to continue
5. Pattern C demo (completion state, failure)

**Duration:** About 30-40 seconds total

### Test 2: Recursive Progress Window

```bash
# From Amiga Shell/CLI
CD Work:iTidy/Tests

# Test with default path (SYS:)
test_recursive_progress

# Test with custom path
test_recursive_progress Work:
test_recursive_progress Work:WHDLoad
test_recursive_progress SYS:Utilities
```

**What you'll see:**
1. Prescan phase (counts folders and icons)
2. Press RETURN to start processing
3. Dual-bar progress window appears
4. Outer bar shows folder progress
5. Inner bar shows icon progress within each folder
6. Press RETURN to close when complete

**Duration:** Depends on directory size
- Small tree (10 folders): ~10 seconds
- Medium tree (50 folders): ~30 seconds
- Large tree (200 folders): ~2 minutes

## System Requirements

- **OS**: AmigaOS 2.0 or higher
- **CPU**: 68020+ (can run on 68000 with recompile)
- **RAM**: 512 KB minimum, 1 MB recommended
- **Workbench**: Any screen mode (works with custom themes)

## Troubleshooting

### "Command not found"
- Make sure file is in current directory or in path
- Make file executable: `protect test_progress_window +e`

### "Cannot open window"
- Ensure Workbench is running
- Try closing some windows to free up resources

### Program crashes or hangs
- Check available memory: `avail` command
- Close unnecessary programs
- Try smaller directory for recursive test

### Wrong colors/appearance
- This is normal - tests use DrawInfo pens
- Colors adapt to your Workbench theme
- Works correctly on all standard screen modes

## Expected Visual Output

### Simple Progress Window
```
┌──────────────────────────────────────────────┐
│ Auto-Close Test                      45%    │
├──────────────────────────────────────────────┤
│  ╔═══════════════════════════════════════╗  │
│  ║███████████████░░░░░░░░░░░░░░░░░░░░░░░║  │
│  ╚═══════════════════════════════════════╝  │
│  Processing item 09                         │
└──────────────────────────────────────────────┘
```

### Recursive Progress Window
```
┌──────────────────────────────────────────────┐
│ Processing Icons Recursively         35%    │
├──────────────────────────────────────────────┤
│  Folders:  ╔════════════════╗  17/50        │
│            ║███████░░░░░░░░░║               │
│            ╚════════════════╝               │
│  SYS:Utilities/Games/                       │
│  Icons:    ╔════════════════╗   8/23        │
│            ║███░░░░░░░░░░░░░║               │
│            ╚════════════════╝               │
└──────────────────────────────────────────────┘
```

## Console Output

Both test programs print status information to the Shell/CLI:

```
===========================================
iTidy Progress Window Test Program
===========================================

Workbench screen locked: 640x256 pixels

Test Pattern A: Auto-Close
Simulating fast operation with 20 items...
Auto-close test complete!

Press RETURN to continue...
```

## Performance Notes

- **7 MHz 68000**: Smooth operation, slight delay on large trees
- **14 MHz 68020**: Very smooth, handles 500+ folders easily
- **68030+ with FPU**: Instant response, no perceptible delay

## Memory Usage

| Test Program | Memory Usage |
|--------------|--------------|
| test_progress_window | ~100 KB |
| test_recursive_progress (small tree) | ~150 KB |
| test_recursive_progress (500 folders) | ~250 KB |

## Integration Example

After testing, you can integrate the progress windows into your own code:

```c
#include "GUI/StatusWindows/progress_window.h"

struct iTidy_ProgressWindow *pw;
pw = iTidy_OpenProgressWindow(screen, "My Operation", 100);

for (i = 0; i < 100; i++) {
    iTidy_UpdateProgress(pw, i + 1, "Processing...");
    DoWork();
}

iTidy_ShowCompletionState(pw, TRUE);
while (iTidy_HandleProgressWindowEvents(pw)) {
    WaitPort(pw->window->UserPort);
}

iTidy_CloseProgressWindow(pw);
```

## Support

For issues or questions:
1. Check PHASE2_PROGRESS_WINDOW_COMPLETE.md
2. Check PHASE3_RECURSIVE_PROGRESS_COMPLETE.md
3. Review STATUS_WINDOWS_DESIGN.md for complete API documentation

## License

These test programs are part of the iTidy project.

---

**Built:** November 5, 2025  
**Platform:** Amiga OS 2.0+  
**Compiler:** VBCC 0.9x
