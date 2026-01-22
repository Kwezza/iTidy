# iTidy Version History

## Version 2.x - ReAction GUI (Current Development)

**Target**: Workbench 3.2+ with ReAction GUI system  
**GUI Framework**: ReAction (window.class, layout.gadget, BOOPSI-based)  
**Status**: Active Development  
**Branch**: `Dev`

### Features:
- Modern ReAction-based interface
- Automatic layout management
- Enhanced visual design
- All v1.x core features maintained

---

## Version 1.0 - GadTools GUI (Legacy/Stable)

**Target**: Workbench 3.0/3.1  
**GUI Framework**: GadTools (native WB 3.0 gadgets)  
**Status**: Frozen (bug fixes on request only)  
**Git Tag**: `v1.0-gadtools`  
**Release Date**: January 22, 2026

### Access Legacy Version:
```bash
git checkout v1.0-gadtools
```

### Features:
- Native GadTools interface for WB 3.0/3.1
- Intelligent icon grid layout with sorting
- Automated window resizing
- LHA-based backup system
- Recursive directory processing
- Default tool validation
- Performance optimized for 68000/68020

### Bug Fixes for v1.0:
If critical bugs are discovered in the GadTools version:
1. Create hotfix branch: `git checkout -b hotfix-gadtools v1.0-gadtools`
2. Apply fixes and commit
3. Tag new version: `git tag -a v1.0.1-gadtools -m "Description"`
4. Push: `git push origin hotfix-gadtools v1.0.1-gadtools`

---

## Core Engine (Shared Across Versions)

The following modules are shared between v1.x and v2.x:
- Icon processing and layout engine
- Backup/restore system
- Window enumeration
- File operations and scanning
- Preference management

Only the GUI layer (`src/GUI/`) differs between versions.
