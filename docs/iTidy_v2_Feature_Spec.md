# iTidy v2 – Auto-Iconizer and Default Tool Validation  
**Project Type:** Feature Expansion / Version 2.0  
**Author:** Kerry Thompson  
**Target:** Workbench 3.0 – 3.2 +  
**Status:** Design Specification (Pre-Implementation)  
**Date:** _(TBD)_

---

## 1  |  Overview

Version 2 of **iTidy** builds upon the current Workbench GUI version and its integrated LhA-based undo system to introduce a new generation of intelligent Workbench maintenance features:

1. **Default Tool Validation & Replacement** – detects missing or obsolete default tools in icons and replaces them using a user-maintained mapping table.  
2. **Auto-Icon Generation (“Auto-Iconizer”)** – scans for files without icons, determines their type, and creates suitable `.info` files using system or user templates.  
3. **Adaptive Defaults Framework** – allows users to select preferred viewers and players (e.g. EaglePlayer vs HippoPlayer) or automatically detect what is installed on their system.  

These capabilities transform iTidy from a layout manager into a **Workbench-wide modernization and self-maintenance tool**.

---

## 2  |  Goals

### 2.1  Primary Goals
| ID | Goal | Description |
|----|------|-------------|
| G1 | Tool Validation | Scan every icon’s `DefaultTool` field, verify executables exist, and replace missing ones using a mapping table. |
| G2 | Auto Icon Creation | Generate `.info` files for missing icons using default templates from `ENV:Sys/` or user templates. |
| G3 | User Preferences | Provide a “Defaults” window to choose preferred apps per category (TEXT, MUSIC, PICTURE, etc.). |
| G4 | Safe Undo | Continue using the LhA backup framework to snapshot each affected folder before modification. |

### 2.2  Stretch Goals
| ID | Stretch Goal | Intent |
|----|--------------|--------|
| S1 | Dynamic Type Detection | Lightweight pattern/extension matcher similar to deficons.library but implemented internally. |
| S2 | Automatic Default Tool Learning | Scan disks for executables (`$VER:` strings) and suggest replacements. |
| S3 | Theme Awareness | Detect installed icon sets (MagicWB, GlowIcons, NewIcons) and use matching templates. |
| S4 | Plugin Architecture | Allow external “player/viewer” modules to declare supported filetypes. |
| S5 | Workbench Refresh Hook | Update the active Workbench window so new icons appear immediately. |
| S6 | Batch Undo Browser | Extend the backup system with a GUI “Undo Manager” for restoring any previous run. |

---

## 3  |  High-Level Workflow

```
FOR each target folder selected in GUI
    IF undo enabled AND LhA available THEN
        Create LhA snapshot of folder icons
    ENDIF

    FOR each file in folder
        IF file HAS .info THEN
            VALIDATE default tool
        ELSE
            DETECT file type
            CREATE default icon
        ENDIF
    NEXT file
NEXT folder
```

---

## 4  |  Functional Modules

### 4.1  Tool Validator Module
**Purpose:** Ensure every icon’s Default Tool points to a valid executable.

**Pseudo-Logic**
```
function ValidateDefaultTool(icon):
    tool ← icon.DefaultTool
    if not Exists(tool):
        candidate ← LookupMapping(tool)
        if candidate:
            icon.DefaultTool ← candidate
            MarkAsUpdated(icon)
        else:
            LogWarning(icon, "Missing tool")
    endif
end
```

**Data Sources**
- `ToolMap.txt` (plain text, CSV style)  
- User preferences (`ENVARC:iTidy/Defaults.prefs`)  
- System search paths (C:, SYS:, Utilities:)

---

### 4.2  Auto-Iconizer Module
**Purpose:** Generate icons for files that lack them.

**Pseudo-Logic**
```
function AutoIconize(folder):
    for each file without .info:
        type ← DetectFileType(file)
        template ← GetTemplateForType(type)
        if template:
            newIcon ← Clone(template)
            newIcon.DefaultTool ← GetPreferredTool(type)
            SaveIcon(file, newIcon)
            LogInfo("Icon created", file)
        endif
    next
end
```

**Template Resolution Order**
1. `ENV:Sys/def_<type>.info` (if present)  
2. `Prefs:iTidy/Templates/<type>.info` (user)  
3. Built-in fallback icons  

---

### 4.3  Type Detection Subsystem (S1)
**Purpose:** Identify file category for Auto-Iconizer.  
**Mechanisms:**
- Filename extension table  
- Header signature table (`FORM ILBM`, `FORM 8SVX`, `M.K.` etc.)  
- Plain-ASCII detection for TEXT files  

**Example Rule Structure**
```
RULE "IFF-ILBM" MATCH "FORM"+"ILBM" TYPE PICTURE
RULE "IFF-8SVX" MATCH "FORM"+"8SVX" TYPE SOUND
RULE "MOD"      EXT "MOD"           TYPE MUSIC
RULE "TEXT"     EXT "TXT" OR ASCII  TYPE TEXT
```

---

### 4.4  Preferences Manager (G3)
**Purpose:** Store user choices for preferred viewers/players.

**Preference File Schema**
```
TEXT=SYS:Utilities/TextEdit
PICTURE=SYS:Utilities/MultiView
MUSIC=Work:Audio/EaglePlayer
SOUND=SYS:Utilities/MultiView
ANIM=SYS:Utilities/MultiView
```

**Auto-Detect Mode (S2)**
```
ScanPaths = ["C:","SYS:","Utilities:","Work:"]
for each path in ScanPaths:
    if ProgramExists("EaglePlayer") then addCandidate(MUSIC,"EaglePlayer")
    if ProgramExists("HippoPlayer") then addCandidate(MUSIC,"HippoPlayer")
    ...
end
Prompt user to select preferred option per type
```

---

### 4.5  Theme Integration (S3)
Detect installed icon theme via file presence or version strings and select appropriate template set (e.g. GlowIcons, MagicWB).

---

## 5  |  Workbench 3.2 Default Icons System

Workbench 3.2 introduced a powerful **DefaultIcons** and **DefIcons.library** system that dynamically supplies icons for files without `.info` files.  
It works by:

- Reading rules stored in `ENV:Sys/DefIcons.prefs` and `ENV:Sys/def_*.info`  
- Matching file names, extensions, and internal signatures  
- Temporarily displaying the appropriate icon on Workbench without creating a physical `.info` file  

This system includes a large and well-maintained library of default icons for dozens of common file types — IFF pictures, text, archives, sounds, and many others.  
However, **it does not expose a public API** for external programs to query or request those matches.

### Why iTidy Cannot Use DefIcons Directly
- **No documented interface:** Workbench 3.2’s DefIcons.library has no official function calls to retrieve the match result or copy its generated icon.  
- **Memory-only icons:** DefIcons creates icons in RAM, not on disk — once Workbench closes a window, those icons vanish.  
- **Internal structures subject to change:** Accessing DefIcons.library’s private data structures would constitute unsupported “hacking.” This could break with future Workbench 3.2 updates or Amiga OS releases.  
- **System stability risk:** Hooking internal Workbench functions is not considered system-legal and may destabilize other software.

### Our Approach
Because of these constraints, iTidy v2 will:
1. **Respect** Workbench’s DefIcons ecosystem by using its existing template icons (e.g. `ENV:Sys/def_music.info`).  
2. **Mimic** its file-type detection logic via a safe custom pattern matcher (S1).  
3. **Generate real on-disk icons** so users retain them even when DefIcons is disabled or moved to another system.  

This ensures compatibility and portability without re-implementing Workbench internals.

---

## 6  |  User Interface Outline

### 6.1  Main Window (Additions)
```
[✓] Validate Default Tools
[✓] Auto-Create Icons for Missing Files
[Defaults…]  [Run]
```

### 6.2  Defaults Dialog
```
╔══════════════════════════════════════╗
║ Default Tool Associations            ║
╠══════════════════════════════════════╣
║ Text:      [TextEdit ▼]              ║
║ Pictures:  [MultiView ▼]             ║
║ Music:     [EaglePlayer ▼]           ║
║ Sound:     [MultiView ▼]             ║
║ Animation: [MultiView ▼]             ║
╠══════════════════════════════════════╣
║ [Auto Detect Tools] [Save] [Cancel]  ║
╚══════════════════════════════════════╝
```

---

## 7  |  Logging & Reports

- All changes written to `T:iTidy_Report.txt`  
- Summary includes validated icons, created icons, skipped files, and backup archives.  
```
Validated: 154 icons (12 updated)
Created:   34 new icons
Skipped:   3 unknown file types
Backups:   22 folders (LhA mode)
```

---

## 8  |  Stretch Goal Roadmap

| Phase | Stretch Goal | Outcome |
|--------|--------------|----------|
| 2.1 | S1 Type Detection | Add pattern-based file recognition engine (similar to DefIcons). |
| 2.2 | S2 Auto-Learning | Implement system-wide scan for known executables via `$VER:` strings. |
| 2.3 | S3 Theme Awareness | Integrate template selection for GlowIcons/MagicWB themes. |
| 2.4 | S4 Plugin System | Support third-party player/viewer definitions. |
| 2.5 | S5 Workbench Refresh | Trigger window update after icon generation. |
| 2.6 | S6 Undo Manager | GUI browser for LhA backups with per-folder restore. |

---

## 9  |  Non-Goals / Out of Scope

- Direct modification of Workbench DefIcons preferences or internal libraries.  
- Hooking or patching Workbench 3.2 internals.  
- Any CLI-only interface (the CLI edition is retired as of v2).  
- Network lookups or remote downloads of icons or tools.  

---

## 10  |  Deliverables for v2 Milestone

1. Updated Workbench GUI with Tool Validation and Auto-Icon options.  
2. Configuration files: `ToolMap.txt`, `TypeMap.txt`, `Defaults.prefs`.  
3. Integrated LhA backup support with full logging.  
4. Defined interfaces for future stretch goal modules.  

---

## 11  |  Future Vision

> **“Every file deserves a face — and a tool that still exists.”**  
> iTidy v2 turns Workbench into a self-organizing environment that not only looks tidy but stays functional and future-proof across generations of Amiga software.

---

## 12 | Uncertainties and Investigation Notes

### 12.1 Workbench 3.2 “Show All Files” + Snapshot Behavior  
There is evidence that when *Show All Files* is active, Workbench displays temporary “virtual” icons created by **DefIcons.library**.  
If the user then performs **Icons → Snapshot → All**, Workbench may write physical `.info` files for those virtual icons—possibly including their inherited **Default Tool** field from the corresponding `def_*.info` template.

This behavior is not formally documented and requires verification.  
If confirmed, it presents a system-legal pathway for iTidy v2 to leverage Workbench’s own detection and template logic without replicating DefIcons’ pattern-matching engine.

### 12.2 Investigation Tasks  
| ID | Question | Expected Observation | Possible Impact |
|----|-----------|----------------------|-----------------|
| T1 | Does “Snapshot All” create `.info` files for deficons-generated icons? | Files appear on disk matching visible icons. | Allows optional “Use Workbench Snapshot Mode” feature. |
| T2 | Do these saved icons include a valid `Default Tool`? | Default Tool mirrors `def_*.info` template (e.g., `TextEdit`). | Removes need for iTidy to assign default tools manually. |
| T3 | Does editing `def_*.info` affect subsequently snapped icons? | New icons inherit updated defaults. | Confirms template propagation. |
| T4 | What happens when DefIcons is disabled and Snapshot All is used? | No virtual icons → no files written. | Defines fallback behavior. |
| T5 | Are icons created this way portable? | Copied icons retain visuals and tools on other systems. | Validates persistence for backups. |

### 12.3 Design Implications  
Depending on results:
- **If confirmed:** iTidy v2 may include an assisted “Snapshot Mode” option instructing users to let Workbench perform the icon creation, after which iTidy tidies and backs up the results.  
- **If disproved:** The Auto-Iconizer module remains essential for generating `.info` files independently using its own detection and template logic.  
- **If partially true:** Hybrid workflow—use Workbench defaults when available, fall back to internal type detection for remaining files.

The decision path here directly affects stretch goals **S1 (Type Detection)** and **S2 (Auto-Learning)**, as leveraging Workbench’s own snapshot behavior could simplify or even replace portions of those modules.

---

*(End of Specification)*
