# EasyRequest Helper - Enhanced Logging & Centered Mode

## Changes Made

### 1. Enhanced Debug Logging

Added comprehensive logging to both standard and BUILD_WITH_MOVEWINDOW modes:

#### Standard Mode Logging:
```c
ShowEasyRequest: Opening requester
  Title: <title>
  Body: <message>
  Gadgets: <gadget labels>
  Parent window: <pointer>, screen: <pointer>
ShowEasyRequest: Using standard EasyRequest()
  Screen dimensions: <width>x<height>
  Parent window position: (<x>,<y>), size: <width>x<height>
  Screen title: <title>
ShowEasyRequest: User selection = <0/1>
ShowEasyRequest: Returning <0/1>
```

#### BUILD_WITH_MOVEWINDOW Mode Logging:
```c
ShowEasyRequest: Opening requester
  Title: <title>
  Body: <message>
  Gadgets: <gadget labels>
  Parent window: <pointer>, screen: <pointer>
ShowEasyRequest: Using BuildEasyRequest() with centering
  Requester window: <pointer>
  Initial position: (<x>,<y>), size: <width>x<height>
  Screen dimensions: <width>x<height>
  Calculated center position: (<x>,<y>)
  MoveWindow delta: (<dx>,<dy>)
  Requester moved. New position: (<x>,<y>)
  Entering event loop...
  User clicked gadget ID=<id> (result=<0/1>)
  Event loop complete. User choice: <0/1>
ShowEasyRequest: Cleanup complete. Returning <0/1>
```

### 2. Enabled BUILD_WITH_MOVEWINDOW

Modified `Makefile` to add `-DBUILD_WITH_MOVEWINDOW` to CFLAGS:
```makefile
CFLAGS = +aos68k -c99 -cpu=68020 -g -I$(INC_DIR) -Isrc -DPLATFORM_AMIGA=1 -D__AMIGA__ -DDEBUG -DBUILD_WITH_MOVEWINDOW
```

### 3. What to Look For in iTidy.log

When you run the program and open a requester, check `iTidy.log` for:

**Key Information:**
- **Initial position**: Where `BuildEasyRequest()` places the requester before moving
- **Calculated center position**: Where we want to move it to
- **MoveWindow delta**: How far we're trying to move it
- **New position**: Where the requester actually ended up after `MoveWindow()`

**Example Expected Output:**
```
ShowEasyRequest: Opening requester
  Title: Confirm Restore
  Body: Restore all folders from Run_0001?
  Gadgets: Restore|Cancel
  Parent window: 0x080A4C80, screen: 0x080A3F00
ShowEasyRequest: Using BuildEasyRequest() with centering
  Requester window: 0x080B2A00
  Initial position: (0,0), size: 320x80
  Screen dimensions: 1920x1080
  Calculated center position: (800,500)
  MoveWindow delta: (800,500)
  Requester moved. New position: (800,500)
  Entering event loop...
```

## Testing Instructions

1. **Run iTidy** on your Amiga/WinUAE
2. **Open the Restore window** (click "Restore Backups")
3. **Click "Restore Run"** button
4. **Check the requester position**:
   - Should appear centered on the screen
   - NOT at top-left (0,0)
5. **Check iTidy.log** for debug output

## Troubleshooting

### If requester still appears at top-left:

Check the log for these scenarios:

**Scenario 1: MoveWindow() not working**
```
Initial position: (0,0)
Calculated center position: (800,500)
MoveWindow delta: (800,500)
Requester moved. New position: (0,0)  <-- Still at 0,0!
```
**Cause**: MoveWindow() might not be supported or failing
**Solution**: May need OS version check or alternative approach

**Scenario 2: BuildEasyRequest() places it incorrectly**
```
Initial position: (0,0)
```
**Cause**: This is expected - BuildEasyRequest() defaults to top-left
**Solution**: MoveWindow() should fix it (check "New position")

**Scenario 3: Requester appears then moves (flicker)**
```
Initial position: (0,0)
Requester moved. New position: (800,500)
```
**Observation**: You might see brief flicker as window moves
**Solution**: This is normal with BUILD_WITH_MOVEWINDOW approach

### Alternative: Disable Centering

If BUILD_WITH_MOVEWINDOW causes issues, disable it:

1. Edit `Makefile`
2. Remove `-DBUILD_WITH_MOVEWINDOW` from CFLAGS:
   ```makefile
   CFLAGS = +aos68k -c99 -cpu=68020 -g -I$(INC_DIR) -Isrc -DPLATFORM_AMIGA=1 -D__AMIGA__ -DDEBUG
   ```
3. Rebuild: `make clean && make`
4. The requester will use standard `EasyRequest()` (still appears on correct screen, just not centered)

## Why Top-Left Placement Happens

**Standard EasyRequest()** behavior:
- Opens requester on parent window's screen ✓
- But AmigaOS places it at default position (usually top-left)
- Position is controlled by Intuition, not by your code

**BUILD_WITH_MOVEWINDOW** solution:
- Manually calculates center position
- Uses `MoveWindow()` to reposition after opening
- More control, but requires Workbench 3.0+ (V39+)

## Next Steps

1. Run the new build and test the requester
2. Check `iTidy.log` for the debug output
3. Report back:
   - Where did the requester appear?
   - What do the log entries show?
   - Did MoveWindow() successfully reposition it?

With this logging, we can diagnose exactly what's happening and adjust the approach if needed.
