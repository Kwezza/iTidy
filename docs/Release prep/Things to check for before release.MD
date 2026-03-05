# iTidy Release Checklist

## CRITICAL (Must Pass Before Release)

### Build Configuration
- [ ] Compiled for **68000 CPU** (Workbench 3.2 including ReAction supports 68000+)
- [ ] Debug symbols stripped from executable
- [ ] Stack size appropriate (currently 80KB) - verify sufficient for all operations
- [ ] Clean compilation - only VBCC warnings 51 & 61 from system headers are acceptable
- [ ] No hardcoded developer paths in code
- [ ] Tool types stripped or commented out if not needed
- [ ] **DEBUGLEVEL=4** (disabled) for production build

### Version Numbers (Must Match Everywhere)
- [ ] `src/version_info.h` - version number and date
- [ ] `docs/manual/iTidy.readme` (Aminet README)
- [ ] `docs/manual/iTidy.md` (AmigaGuide source)
- [ ] `VERSION_HISTORY.md`
- [ ] GUI About window version string
- [ ] Executable version string (if embedded)

### Memory & Stability Testing
- [ ] **Memory leak testing** - Enable `DEBUG_MEMORY_TRACKING` in `include/platform/platform.h`
- [ ] Verify clean shutdown - check `Bin/Amiga/logs/memory_*.log` for leaks
- [ ] **MMU testing with MUForce_Window** - Run iTidy and verify no hits/warnings
- [ ] **MMU testing with MUGuardianAngel** - Monitor for illegal memory access
- [ ] Test on **minimum spec system** - 68000, WB 3.2, minimal Fast RAM
- [ ] Test on **maximum operations** - Large directories (1000+ icons), deep recursion
- [ ] **All backup/restore scenarios tested**:
  - [ ] Create backup of clean directory
  - [ ] Restore backup successfully
  - [ ] Backup with existing icons
  - [ ] Restore with conflicts
  - [ ] Backup failure handling (disk full, write-protected)
  - [ ] Restore failure handling (corrupted LHA, missing files)
  - [ ] Multiple backup runs in same directory
  - [ ] Recursive backup operations

### Core Functionality Testing
- [ ] **All GUI features work**:
  - [ ] Every button performs correct action
  - [ ] All requesters display properly
  - [ ] All string gadgets accept input correctly
  - [ ] All cycle/chooser gadgets work
  - [ ] All checkbox gadgets toggle correctly
  - [ ] Progress windows update correctly
  - [ ] Window resizing works (if applicable)
  - [ ] Keyboard shortcuts work (if applicable)
- [ ] **Icon layout operations**:
  - [ ] Sort by name (ascending/descending)
  - [ ] Sort by type
  - [ ] Sort by date
  - [ ] Sort by size
  - [ ] Grid alignment correct
  - [ ] Handles icons without names
  - [ ] Handles drawer icons
  - [ ] Handles disk icons
- [ ] **Window resizing operations**:
  - [ ] Aspect ratio control works
  - [ ] Overflow handling correct
  - [ ] Works on different screen resolutions
  - [ ] Works with different window sizes
- [ ] **Recursive directory processing**:
  - [ ] Processes subdirectories correctly
  - [ ] Hidden folder filtering works
  - [ ] Depth limits respected
  - [ ] Progress updates correctly
  - [ ] Cancellation works mid-operation
- [ ] **Settings management**:
  - [ ] Save settings works
  - [ ] Load settings works
  - [ ] Creates required folders on fresh install
  - [ ] **Backward compatibility** - Loads old saved settings (or warns user if not)
  - [ ] Default settings are sensible

### Error Handling
- [ ] All error messages are **user-friendly** (no debug-speak, no technical jargon)
- [ ] All error conditions tested:
  - [ ] Disk full
  - [ ] Write-protected volume
  - [ ] Corrupted icon files
  - [ ] Missing icon files
  - [ ] Very long file paths (>255 chars)
  - [ ] Invalid characters in filenames
  - [ ] Read-only directories
  - [ ] Network volumes (if applicable)
  - [ ] Empty directories
  - [ ] Directories with no icons
- [ ] No crashes under any error condition
- [ ] All resources freed on error paths (locks, memory, file handles)

### Bug Review
- [ ] Check `docs/buglist.md` for **dealbreaker bugs**
- [ ] All critical and high-priority bugs fixed
- [ ] Known issues documented in release notes

---

## IMPORTANT (Should Be Done)

### Documentation
- [ ] **AmigaGuide documentation** (`docs/manual/iTidy.md`):
  - [ ] All features documented
  - [ ] Up-to-date with current feature set
  - [ ] Screenshots included (if applicable)
  - [ ] Converted to AmigaGuide format using Python converter
  - [ ] All tooltypes explained
  - [ ] Examples provided for common tasks
- [ ] **Aminet README** (`docs/manual/iTidy.readme`):
  - [ ] Accurate description
  - [ ] System requirements listed
  - [ ] Installation instructions clear
  - [ ] Known issues section
  - [ ] Contact/support information
- [ ] **Change log/Release notes**:
  - [ ] What's new in this version
  - [ ] Bug fixes from previous version
  - [ ] Breaking changes (if any)
  - [ ] Upgrade notes (if needed)
- [ ] **System requirements documented**:
  - [ ] Minimum: Workbench 3.2 (V47+)
  - [ ] CPU: 68000 or higher
  - [ ] RAM requirements (specify Fast RAM vs Chip RAM)
  - [ ] Disk space required
  - [ ] Required libraries: ReAction classes (bundled with WB 3.2)
  - [ ] Optional: LHA for backup operations
- [ ] **Installation instructions**:
  - [ ] Step-by-step for new users
  - [ ] Where to copy files
  - [ ] How to set up tooltypes (if needed)
  - [ ] First-run configuration
- [ ] **Uninstall instructions**:
  - [ ] Files/directories to delete
  - [ ] Preferences files location
  - [ ] How to cleanly remove iTidy

### Distribution Package
- [ ] **LHA archive validated**:
  - [ ] Archive opens correctly
  - [ ] Contents listable from command line
  - [ ] No corruption detected
  - [ ] Reasonable compression ratio
- [ ] **Package contains all required files**:
  - [ ] iTidy executable
  - [ ] iTidy2.guide (AmigaGuide documentation)
  - [ ] iTidy2.guide.info
  - [ ] iTidy.readme (Aminet README)
  - [ ] LICENSE file
  - [ ] VERSION_HISTORY.md or ChangeLog
  - [ ] Icons folder with template icons (`Bin/Amiga/Icons/`)
  - [ ] Example configuration files (if applicable)
- [ ] **Template icons validated** (`Bin/Amiga/Icons/`):
  - [ ] All template icons present and valid
  - [ ] Correct tooltypes set up
  - [ ] Icons display correctly
  - [ ] No corrupted icon files
- [ ] **Main program icon**:
  - [ ] Has required icons (both standard and MagicWB variants if applicable)
  - [ ] Positioned appropriately (not at 0,0)
  - [ ] Tooltypes documented
  - [ ] Default tool not set (unless intentional)

### Logging & Diagnostics
- [ ] **Log file review** (`Bin/Amiga/logs/`):
  - [ ] Nothing too "heavyweight" for normal users in standard categories
  - [ ] Extreme logging confined to "Debug" category only
  - [ ] Log timestamps correct
  - [ ] Log rotation works (if implemented)
  - [ ] Logs don't fill disk on long-running operations

### Testing Edge Cases
- [ ] Empty directories
- [ ] Directories with 1000+ icons
- [ ] Very long file paths (approaching 255 char limit)
- [ ] Special characters in filenames
- [ ] Unicode/international characters (AmigaOS limitations)
- [ ] Read-only volumes
- [ ] Write-protected disks
- [ ] Corrupted icon files
- [ ] Missing .info files
- [ ] Icons without images
- [ ] Disk icons
- [ ] Drawer icons at various positions
- [ ] Icons at negative coordinates
- [ ] Icons beyond screen boundaries
- [ ] Very large icon images (>100KB)

### Performance Testing
- [ ] Fast RAM vs Chip RAM performance difference acceptable
- [ ] Large directory operations (500+ icons) complete in reasonable time
- [ ] Deep recursion (10+ levels) doesn't stack overflow
- [ ] Memory usage reasonable (no excessive allocations)
- [ ] No memory fragmentation on repeated operations
- [ ] Progress updates don't slow down operations significantly

---

## NICE TO HAVE (Polish)

### Documentation Enhancements
- [ ] Credits section:
  - [ ] Libraries used (ReAction, Icon, DOS, Graphics, Intuition, Utility)
  - [ ] Contributors
  - [ ] Beta testers
  - [ ] Special thanks
- [ ] Contact/Support information:
  - [ ] How to report bugs
  - [ ] Where to ask questions
  - [ ] Project website/repository (if public)
  - [ ] Email or forum contact (if applicable)
- [ ] Screenshots:
  - [ ] Main window
  - [ ] Settings window
  - [ ] Progress window
  - [ ] Example "before and after" layouts
- [ ] Video demonstration (if feasible)
- [ ] FAQ section for common questions

### Additional Testing
- [ ] Test on real hardware (not just WinUAE):
  - [ ] A500/A600 with 68000
  - [ ] A1200 with 68020
  - [ ] A4000 with 68040
  - [ ] Vampire accelerator card (if available)
- [ ] Test with different Workbench 3.2 patches/updates
- [ ] Test with different screen modes (PAL/NTSC, different resolutions)
- [ ] Test with different color depths (2/4/8 colors)
- [ ] Stress testing - leave running for extended periods
- [ ] Test with low memory conditions (< 2MB free)

### User Experience
- [ ] Tooltypes have sensible defaults
- [ ] First-run experience is smooth
- [ ] No confusing dialogs
- [ ] Consistent terminology throughout GUI and docs
- [ ] Keyboard navigation works (Tab, Enter, Esc)
- [ ] Copy-to-clipboard for error messages (if applicable)

### Distribution Channels
- [ ] Aminet submission prepared
- [ ] OS4Depot submission prepared (if applicable)
- [ ] GitHub release created with:
  - [ ] Release notes
  - [ ] Binary attachments
  - [ ] Source code archive
  - [ ] Tagged version in Git
- [ ] Announcement text prepared for forums/communities

---

## FINAL PRE-RELEASE CHECKS

- [ ] **ONE MORE TIME**: Run with `DEBUG_MEMORY_TRACKING` enabled - zero leaks
- [ ] **ONE MORE TIME**: Run with MUForce_Window and MUGuardianAngel - zero hits
- [ ] Version numbers match everywhere
- [ ] LHA archive tested on clean system
- [ ] Documentation spelling/grammar checked
- [ ] All critical and important items above checked off
- [ ] Take a deep breath - you've done great work! 🎉

---

## Post-Release Tasks

- [ ] Monitor for bug reports
- [ ] Respond to user questions
- [ ] Update documentation based on user feedback
- [ ] Plan next version based on feature requests
- [ ] Archive release build and source code for posterity
