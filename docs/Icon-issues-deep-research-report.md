# Workbench Icon Pain Points Across WB2 to WB3.2 and Practical iTidy Opportunities

## Executive summary

Icon handling on classic Amiga Workbench remains unusually “stateful”: icon position, window layout, and view/sort modes are persisted *inside* companion `.info` files (and, for certain desktop behaviours, small hidden state files), rather than being centrally managed by a database or a single preference store. This design enables user-controlled layouts, but it also makes everyday usage vulnerable to missed “snapshot” actions, absent `.info` files, performance cliffs in large drawers, and a long tail of edge cases where updates don’t visually “take” until a window is refreshed. citeturn16search30turn22search4turn15search26

A third‑party utility like iTidy—operating purely in userland by scanning directories and safely reading/writing `.info` files with backups—can mitigate many of the highest-friction problems by making layout persistence predictable, adding batch operations, and lowering the likelihood of accidental data loss (e.g., by creating restore points before mass edits). citeturn22search9turn16search30turn21search21

### Top recurring icon pain points overall

1. **“Icons moved again” because layouts weren’t saved the *right way***: users frequently conflate “Snapshot Window” (saves window size/position + view settings) with “Snapshot All” (also saves icon positions), and forget to snapshot after making changes. citeturn22search4turn22search1  
2. **Clean Up is helpful but not a true grid system**: Clean Up reflows icons, but can still yield unwanted rearrangements later (including after subsequent Clean Up operations), and users report odd misalignment in certain cases (e.g., long names). citeturn22search1turn35search1  
3. **“Show All Files” pseudo-icons don’t behave like real icons**: pseudo-icons exist only for iconless files, and users report they can’t be snapshotted/persisted like true `.info` icons without first generating “real” icons. citeturn22search1turn15search26  
4. **Missing `.info` files make drawers look empty in “Show Only Icons” mode**: this remains a persistent usability trap, especially for files copied from non-Amiga systems or archives. citeturn22search4turn16search30  
5. **Default icon behaviour and filetype recognition is complicated**: DefIcons-style workflows (content sniffing + mapping to default icons/tools) are widely used but can be non-obvious to configure and maintain. citeturn21search16turn10search15turn23search28  
6. **Performance cliffs in large drawers**: rendering hundreds/thousands of icons is slow, and becomes worse when extra processing (e.g., DefIcons content sniffing) is enabled. citeturn23search3turn38view0  
7. **Palette/pen interactions still bite**: icon colour quality and rendering speed are influenced by screen depth/colour availability; wallpapers and palette choices can reduce available pens and degrade results. citeturn33search25turn37view1  
8. **High-colour icon sets (GlowIcons/PNG-style) introduce memory and speed trade-offs on classic chipsets**: users discuss slower loads and heavier memory use versus classic planar icons, particularly without RTG. citeturn12search3turn33search17turn38view0  
9. **Icon format compatibility/upgrade hazards**: older icon systems (NewIcons/MagicWB-era expectations) can clash with newer icon.library/workbench expectations; users report installation or runtime trouble when mixing systems on newer OS lines. citeturn15search24turn21search22turn21search19  
10. **Refresh/update inconsistency**: Workbench sometimes doesn’t “notice” new/changed icons until the user triggers Update/Update All or reopens windows; third-party patches exist specifically to address this. citeturn15search26turn12search21turn22search4  

### Top pain points most likely solvable by iTidy

These are the most “utility-addressable” without patching Workbench internals, because they can be solved by deterministic `.info` rewriting plus safety tooling:

- **Reliable grid layout + repeatable “Clean Up” with explicit sort rules** (folders-first, files-first, mixed, reverse, etc.) by editing icon coordinates in `.info` files and (optionally) drawer window settings. citeturn22search1turn22search9  
- **Bulk “Snapshot All” equivalent** (apply + persist) across selected drawers/trees, with backups and rollback. citeturn22search4turn22search1  
- **Mass icon creation for iconless files** (convert pseudo-icons to real icons), using templates or DefIcons-like recognition, reducing “empty drawer” surprises. citeturn22search4turn10search15turn23search3  
- **Batch repair and validation**: missing default tool paths, mismatched `.info` file names, suspicious/corrupt icon structures, negative/out-of-range coordinates, and other metadata hygiene. citeturn22search9turn15search29turn16search30  
- **Performance-aware icon optimisation workflows**: optional conversion or downgrade suggestions (e.g., reduce icon depth/quality for non‑RTG systems; avoid expensive content sniffing in huge drawers unless cached). citeturn23search3turn33search17turn33search25  

### Major differences by era

**WB2.x (2.0/2.1)** normalised the snapshot-driven desktop experience and introduced the modern Workbench model where window/view state and icon positions are persisted via `.info` metadata—powerful, but easy to “forget” to save. citeturn22search4turn16search30  

**WB3.0/3.1.x** remained broadly consistent in icon persistence semantics, so many “icons moved” and “drawer looks empty” complaints persist. Third-party icon ecosystems (MagicWB/NewIcons/DefIcons-like behaviours) became common workarounds for palette dependence and iconless-file usability. citeturn15search24turn21search16turn23search3  

**WB3.2.x** intensified icon concerns because it sits in a “hybrid” world: users may run classic machines with limited chip resources *and* expect richer icon sets (GlowIcons) and newer icon.library behaviour. Workbench/icon.library saw ongoing fixes (e.g., workbench.library drawer/window management fixes; IconEdit-related fixes in hotfixes), but the core snapshot model and associated user pitfalls remain. citeturn21search7turn21search8turn21search3  

## Taxonomy of issues

The taxonomy is organised by user-visible problem clusters. Each “fact card” includes feasibility analysis under the constraints that iTidy is a userland tool that edits `.info` files safely (with backups) and does not require patching Workbench. citeturn22search9turn16search30  

### Layout, positioning, and persistence

#### Issue name (short)
Snapshot semantics cause “icons moved again”

**Description (what users experience)**  
Users reposition icons or resize windows, then later discover icon positions or layouts reverted or shifted. A common root is using “Snapshot Window” (which does *not* save icon positions) instead of “Snapshot All”, or forgetting to snapshot after Clean Up or manual rearrangement. citeturn22search4turn22search1  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
WB2.x, WB3.0, WB3.1.x, WB3.2.x. citeturn22search4turn16search30  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Not hardware-specific; reported on mixed systems, including higher-end OS3.x setups and classic machines. citeturn22search1turn22search4  

**Frequency signal (High, Medium, Low)**  
High.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- Workbench documentation distinguishes Snapshot Window vs Snapshot All and explicitly notes Snapshot Window does not save icon positions. citeturn22search4  
- Users explicitly recommend Snapshot All / snapshotting icons to stop layouts moving, and report frustration when it “still” moves. citeturn22search1  

**Repro notes (if any sources describe steps)**  
Typical repro: move icons, close window/reboot, observe reset; use Snapshot Window only; compare with Snapshot All. citeturn22search4turn22search1  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported by docs: Snapshot Window does not persist icon coordinates; icon positions are persisted separately (Snapshot All / per-icon snapshot). citeturn22search4turn16search30  

**Utility mitigation feasibility**

**Solvable by iTidy: Yes/Partial/No**  
Yes.

**Approach options (bullet list)**  
- Provide an explicit “Persist layout” action that writes icon coordinates for all `.info` files in a directory.  
- Provide “Snapshot-equivalent” operations recursively, with a preview of changes.  
- Offer a “safe default” mode that only touches coordinates, leaving ToolTypes and other metadata untouched.  

**Risks and constraints (performance, compatibility, user data safety)**  
Bulk writes can be slow on floppy/old HDD and risky without backups; must do atomic writes (write temp, verify, rename) and offer rollback. citeturn22search9turn23search3  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Bulk Snapshot-All Writer** — Select a drawer (or tree), compute current or desired positions, then write `do_CurrentX/do_CurrentY` to each matching `.info` with automatic backups and a “restore point” archive.

---

#### Issue name (short)
Clean Up reflow surprises and isn’t a true grid

**Description (what users experience)**  
Clean Up tidies icons into a left-to-right/top-to-bottom arrangement, but users report that icons can still move later (e.g., another Clean Up), and that alignment can be inconsistent—one reported trigger is long drawer/file names causing misalignment after cleanup/snapshot. citeturn22search1turn35search1  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
WB2.x, WB3.0, WB3.1.x, WB3.2.x. citeturn22search4turn22search1  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Not specifically hardware-bound; the reported misalignment mentions long names and Show All Files usage. citeturn22search1turn35search1  

**Frequency signal (High, Medium, Low)**  
High.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- Workbench docs describe Clean Up and its relationship to Snapshot (commonly used after Clean Up). citeturn22search4  
- Users discuss Clean Up re-moving icons and report specific misalignment behaviour with long names. citeturn22search1turn35search1  

**Repro notes (if any sources describe steps)**  
Open a drawer, Clean Up, Snapshot, later Clean Up again or trigger a Clean Up on the Workbench screen; observe rearrangement; in one report, long names correlate with misalignment. citeturn22search1turn35search1  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Hypothesis: Clean Up uses a simple packing algorithm sensitive to label bounding boxes / reserved icon spacing, so longer labels or “reserved space” rules can shift alignment. The “reserved space overlap” theme appears in snapshot discussions. citeturn35search4turn22search1  

**Utility mitigation feasibility**

**Solvable by iTidy: Yes/Partial/No**  
Yes.

**Approach options (bullet list)**  
- Implement a configurable grid layout (cell width/height, padding) independent of Workbench’s Clean Up.  
- Offer deterministic placement rules (columns-first/rows-first; fixed margins; label-aware spacing).  
- Provide a post-pass “de-overlap” correction for long labels.  

**Risks and constraints (performance, compatibility, user data safety)**  
Computing label-aware spacing requires knowing font/label metrics; if metrics are unknown, use conservative spacing to avoid overlap. Must not produce negative coordinates that some tools mishandle. citeturn22search1turn12search14  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Deterministic Grid Clean** — Grid-based placement with selectable sort key and a “label-safe” mode (wider columns) plus a “classic compact” mode for small screens.

---

#### Issue name (short)
Volatile system icons and “device icons don’t stay put” (Disk.info / RAM:)

**Description (what users experience)**  
Some icons (especially RAM:) behave differently because they are not backed by a normal on-disk `Disk.info`. Users describe that `Disk.info` is stored in RAM and disappears on reboot, so custom positioning or replacing the icon isn’t persistent unless you follow special steps (such as storing a default icon in ENV/ENVARC under a specific name for RAM). citeturn12search15turn12search16  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
Primarily WB3.1.x and WB3.2.x discussions, but the underlying RAM: volatility concept affects earlier versions too. Evidence in scope is strongest for later 3.x. citeturn12search15turn12search16  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Not hardware-specific; this is about how RAM: is implemented and where its icon metadata lives. citeturn12search15turn12search16  

**Frequency signal (High, Medium, Low)**  
Medium.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- Explanation that `Disk.info` for RAM: is stored in RAM and disappears; workaround using `ENVARC:Sys/def_RAM.info`. citeturn12search15  
- Separate community discussion describing the same behaviour and workaround framing (RAM icon persistence). citeturn12search16  

**Repro notes (if any sources describe steps)**  
Replace RAM icon, reboot, observe reset; then apply ENV/ENVARC workaround and reboot again. citeturn12search15turn12search16  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported by sources: RAM: `Disk.info` is volatile (in RAM), causing loss of custom icon unless redirected to a persistent default icon path. citeturn12search15turn12search16  

**Utility mitigation feasibility**

**Solvable by iTidy: Yes/Partial/No**  
Partial (can automate the workaround, but cannot change how RAM: works).

**Approach options (bullet list)**  
- Detect “special” volumes (RAM:, maybe others) and offer a guided “make persistent” action (copy chosen icon to the default icon location used by the workaround).  
- Validate that the persistent default exists and matches user expectations.  

**Risks and constraints (performance, compatibility, user data safety)**  
Risk of overwriting user’s existing default icons; must backup old def_* icons and offer restore. citeturn22search9turn12search15  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Special Volume Icon Persistence Assistant** — Scans for RAM: and other edge-case devices, explains persistence rules, and can install/backup default icons to the appropriate ENV/ENVARC location.

---

#### Issue name (short)
Leave Out / backdrop icon state is awkward to manage

**Description (what users experience)**  
Workbench supports “Leave Out” / “Put Away” desktop icons, but the state is managed via hidden backing files (e.g., `.backdrop` lists) and may require a reboot for changes to take effect; users explicitly note that “Restart Workbench” isn’t sufficient for certain methods. citeturn35search33turn22search1  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
WB2.x onward (feature exists broadly), with practical scripting discussion in WB3.x-era ecosystems. citeturn35search33turn22search1  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Not hardware-specific; this is state management for backdrop icons. citeturn35search33turn22search1  

**Frequency signal (High, Medium, Low)**  
Medium.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- Hyperion forum post describes `.backdrop` file manipulation and the need to reboot; “Restart Workbench” not working for that approach. citeturn35search33  
- User questions around Leave Out / Put Away and desktop behaviour (what it does, how to keep icons placed). citeturn22search1  

**Repro notes (if any sources describe steps)**  
Echo an icon path into a volume’s `.backdrop`, reboot, and observe icon left out; attempting to avoid reboot may fail. citeturn35search33  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported: backdrop state is stored in hidden `.backdrop` files and requires a reboot for certain workflows; Workbench restart doesn’t reload this file in that scenario. citeturn35search33  

**Utility mitigation feasibility**

**Solvable by iTidy: Yes/Partial/No**  
Partial.

**Approach options (bullet list)**  
- Provide a `.backdrop` manager (list, add, remove entries) with backups.  
- Offer a “safe instructions” prompt for users when a reboot or Update All is required for changes to appear. citeturn15search26turn35search33  

**Risks and constraints (performance, compatibility, user data safety)**  
Incorrect `.backdrop` edits can leave orphaned desktop entries; must validate targets and provide “clean dead entries” scanning. citeturn35search33turn22search9  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Backdrop Inventory + Cleaner** — Parses `.backdrop`, verifies each entry exists, optionally “Put Away” missing items, and can bulk-adjust icon positions via their `.info` metadata.

---

#### Issue name (short)
Pseudo-icons cannot be persistently arranged (until made real)

**Description (what users experience)**  
When “Show All Files” is enabled, Workbench shows pseudo-icons for files without `.info`. Users report these pseudo-icons cannot be snapshotted like real icons because the icon doesn’t exist on disk. citeturn22search1turn22search4  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
WB2.x, WB3.0, WB3.1.x, WB3.2.x (as long as “Show All Files” exists and pseudo-icons are used). citeturn22search1turn22search4  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Often arises on drawers containing files transferred from non-Amiga environments or archives that don’t ship `.info` companions. citeturn22search4turn23search3  

**Frequency signal (High, Medium, Low)**  
Medium.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- Users explicitly state pseudo-icons can’t be snapshotted because they don’t exist on disk; need to “make real icons”. citeturn22search1  
- Documentation describes “Show All Files” behaviour and the difference between icon-only and all-files viewing. citeturn22search4  

**Repro notes (if any sources describe steps)**  
Enable Show All Files, try to snapshot a file that has no `.info`, observe inability to persist; fix by creating a real icon. citeturn22search1turn22search4  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported: pseudo-icons lack a backing `.info` file, so there is nowhere to store position metadata. citeturn22search1turn16search30  

**Utility mitigation feasibility**

**Solvable by iTidy: Yes/Partial/No**  
Yes.

**Approach options (bullet list)**  
- Batch-create real icons for pseudo-icon files using templates or type recognition.  
- Optionally apply a sensible default tool association when creating project icons. citeturn21search16turn10search15  

**Risks and constraints (performance, compatibility, user data safety)**  
Creating many `.info` files increases directory size and can slow icon scanning later; must allow selective creation (e.g., only for specific extensions/types). citeturn23search3turn38view0  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Make-Real Batch** — For a chosen drawer/tree: create `.info` for iconless files with a user-selected template set and optional “folders only / projects only / by type” filters.

---

### Default icons and automatic icon generation

#### Issue name (short)
Drawer looks empty in “Show Only Icons” mode (missing `.info`)

**Description (what users experience)**  
Workbench’s icon-centric design means files without matching `.info` are hidden in “Show Only Icons” mode, and only appear via “Show All Files”. This surprises users when browsing archives, foreign media, or directories with missing icons. citeturn22search4turn16search30  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
WB2.x, WB3.0, WB3.1.x, WB3.2.x. citeturn22search4turn16search30  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Common when importing software from the Internet (Aminet-style archives) or cross-platform media; also common for command-created directories (which do not automatically get drawer icons). citeturn38view0turn35search14  

**Frequency signal (High, Medium, Low)**  
High.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- Workbench docs explain Show Only Icons vs Show All Files behaviour. citeturn22search4  
- Workbench icon system description: `.info` files are the mechanism; iconless files require Show All Files to be displayed as defaults. citeturn16search30  

**Repro notes (if any sources describe steps)**  
Open a directory with no `.info` files in Show Only Icons mode; it appears empty; switch to Show All Files; files appear as pseudo-icons. citeturn22search4  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported: Workbench stores icon metadata separately in `.info` files; without these, icon view hides items unless Show All Files is enabled. citeturn16search30turn22search4  

**Utility mitigation feasibility**

**Solvable by iTidy: Yes/Partial/No**  
Yes.

**Approach options (bullet list)**  
- “Create missing icons” wizard for drawers/projects/tools, using templates.  
- Optional policy: if a drawer contains “mostly iconless files”, generate icons for the rest (opt-in).  

**Risks and constraints (performance, compatibility, user data safety)**  
Generating icons changes directory contents, which some users dislike; must support dry-run and reversible backups. citeturn22search9turn23search3  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Missing Icon Auditor + Generator** — Reports counts of iconless files per drawer and can batch-create icons with clear user controls.

---

#### Issue name (short)
DefIcons rules are powerful but complex and brittle

**Description (what users experience)**  
DefIcons-style systems use a two-step mapping: identify a file type (often by content sniffing rather than extension), then assign an icon and default tool. Users ask how this works, how to add new file types, and report cases where DefIcons stops recognising types after OS upgrades. citeturn21search16turn21search19turn23search28  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
As an add-on, it’s used across WB2+ environments; evidenced strongly in WB3.x-era discussions and later. citeturn21search16turn21search19  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Often relevant to “Show All Files” workflows and Internet/downloading. citeturn38view0turn23search3  

**Frequency signal (High, Medium, Low)**  
High.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- Clear explanation of DefIcons as two-step identification + association system. citeturn21search16  
- Users report DefIcons no longer recognising file types after upgrading to 3.1.4 (showing fragility across system changes). citeturn21search19  
- Users ask how to add a new type (e.g., JAR) and note collisions where content matches another type. citeturn23search28  

**Repro notes (if any sources describe steps)**  
Upgrade OS, run DefIcons, observe default “generic” icons only; adjust DefIcons prefs/type rules; or add new icons and mappings. citeturn21search19turn23search28  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported: DefIcons depends on configurable identification rules and a library/toolchain that can be impacted by OS/library changes, leading to recognition failures. citeturn21search19turn21search16  

**Utility mitigation feasibility**

**Solvable by iTidy: Yes/Partial/No**  
Partial (iTidy can avoid relying on DefIcons by generating real icons, or it can help manage DefIcons assets, but it can’t guarantee DefIcons runtime behaviour).

**Approach options (bullet list)**  
- Offer an independent “static icon assignment” engine (extension + magic-number sniffing cache) and write real `.info` icons. citeturn23search3turn38view0  
- Provide DefIcons asset management: create/backup `def_*.info` templates and validate required paths. citeturn23search28turn21search16  

**Risks and constraints (performance, compatibility, user data safety)**  
Content sniffing is expensive in huge drawers; should cache results and provide a “trust file extensions only” fast mode. citeturn23search3turn23search8  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Static Filetype Iconiser** — Build a filetype-to-template map, optionally sniff file headers once, then write real `.info` icons so Workbench no longer needs on-the-fly DefIcons for that drawer.

---

#### Issue name (short)
DefIcons content sniffing makes big drawers slower

**Description (what users experience)**  
Users report that Workbench becomes significantly slower in very large directories when “Show All Files” is used with DefIcons, because DefIcons reads the first block of each file to determine type. Magazine coverage also characterises DefIcons as looking at file content to decide which icon to display. citeturn23search3turn38view0  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
Applies wherever DefIcons is used; user reports are in WB3.x-era contexts. citeturn23search3turn38view0  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Worst on slow I/O and low CPU systems; one report references ~1000 icons. citeturn23search3turn38view0  

**Frequency signal (High, Medium, Low)**  
High.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- User report: DefIcons loads the first block of every file, worsening performance in large directories. citeturn23search3  
- Article: DefIcons “looks at the content of each file and displays an icon according to its type.” citeturn38view0  

**Repro notes (if any sources describe steps)**  
Open a directory with ~1000 icons, enable Show All Files with DefIcons; observe slow icon rendering; disable DefIcons or Show All Files; compare. citeturn23search3  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported: DefIcons reads file contents (at least the first block) for type recognition, adding I/O per file and increasing startup cost. citeturn23search3turn38view0  

**Utility mitigation feasibility**

**Solvable by iTidy: Yes/Partial/No**  
Yes (mitigate the need for runtime sniffing).

**Approach options (bullet list)**  
- Precompute type decisions once and write real icons; optionally store a cache file per directory.  
- Provide a “threshold heuristic”: if directory size > N, default to extension-only classification unless user opts in. citeturn23search3turn23search8  

**Risks and constraints (performance, compatibility, user data safety)**  
Precomputation must be interruptible and resumable on 68000-class machines; also must avoid heavy RAM usage. citeturn22search9turn23search3  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: One-Time Sniff + Cache** — Scan a drawer, sniff only unknown types, cache results, then generate icons and disable further sniffing for that drawer.

---

### Performance and redraw speed

#### Issue name (short)
Very slow drawer opens with lots of icons

**Description (what users experience)**  
Large icon drawers can take a long time to render; the experience worsens when extra work is done per file (e.g., DefIcons sniffing). Older commentary on OS3.9 also frames “faster icon loading” as a notable improvement area attributable to icon.library optimisation. citeturn23search3turn38view0  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
Persistent across versions; later OS lines may improve parts of it, but large-drawer cost remains a recurring theme. citeturn23search3turn38view0  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Heavily affected by CPU/I/O speed and by icon formats requiring more processing; one report references 1000 icons. citeturn23search3turn33search17  

**Frequency signal (High, Medium, Low)**  
High.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- User report ties slowness directly to large icon counts and DefIcons overhead. citeturn23search3  
- Article notes OS3.9’s faster icon loading is due to icon.library optimisation (showing this has long been recognised). citeturn38view0  

**Repro notes (if any sources describe steps)**  
Open a directory with many files/icons; time to first usable window. Repeat with DefIcons on/off and Show All Files on/off. citeturn23search3  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported: icon loading speed depends substantially on icon.library efficiency and on per-file work (e.g., type recognition and rendering). citeturn38view0turn23search3  

**Utility mitigation feasibility**

**Solvable by iTidy: Partial/No**  
Partial.

**Approach options (bullet list)**  
- Reduce per-open work by making icons “real” and simple (avoid runtime content sniffing).  
- Provide optional “drawer staging”: process and arrange in batches (e.g., 200 icons at a time) so users can stop early.  

**Risks and constraints (performance, compatibility, user data safety)**  
iTidy cannot make Workbench multi-threaded; it can only reduce input complexity and keep icon metadata tidy and predictable. citeturn38view0  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Large Drawer Optimiser** — Detect “large drawer” conditions and offer a guided set of mitigations: batch icon creation, conservative icon formats, and deterministic layout with minimal metadata churn.

---

#### Issue name (short)
Chip RAM pressure from icons and backdrops

**Description (what users experience)**  
Systems without a graphics card can suffer when icons consume chip resources. OS3.9 commentary highlights an “icon memory setting” that stores icons in Fast RAM on compatible systems to reduce high chip RAM usage when drawers contain lots of icons. citeturn38view0  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
Strongly evidenced for OS3.5/3.9 Workbench prefs; conceptually relevant to classic machines across eras when icons are stored in chip-based bitmaps. citeturn38view0turn33search17  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Most relevant on non-RTG systems and systems using chip RAM heavily; mitigations differ if RTG/FBlit or similar is present. citeturn38view0turn33search17  

**Frequency signal (High, Medium, Low)**  
Medium.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- OS3.9 article explicitly describes icon memory setting to store icons in Fast RAM to avoid high chip RAM usage. citeturn38view0  
- Community discussion notes more colours and richer visuals are slower on classic hardware, with RTG reducing redraw pressure. citeturn33search17  

**Repro notes (if any sources describe steps)**  
Open large drawer on a classic chipset system; observe memory pressure/slowness; compare with settings that store icons in Fast RAM where available. citeturn38view0turn33search17  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported for OS3.9 era: icons stored in chip RAM can be costly; a settings-based workaround moves them to Fast RAM on suitable systems. citeturn38view0  

**Utility mitigation feasibility**

**Solvable by iTidy: Partial/No**  
Partial.

**Approach options (bullet list)**  
- Offer “icon format budgeting” guidance: for non-RTG setups, prefer lower-colour icons and avoid expensive formats in bulk. citeturn33search17turn12search3  
- Provide optional conversion pipelines (only if fast and safe) or at least detection + warnings.  

**Risks and constraints (performance, compatibility, user data safety)**  
Real conversion is CPU-heavy on 68000/68030; must be optional, and must preserve originals via backups. citeturn22search9turn33search17  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Icon Budget Report** — For a drawer/tree, estimate total icon memory and recommend “low-colour plan” vs “RTG-friendly plan”, with optional batch conversion hooks.

---

### Icon formats, palette interactions, and compatibility

image_group{"layout":"carousel","aspect_ratio":"16:9","query":["Amiga Workbench 2.1 icons screenshot","Amiga MagicWB icons screenshot","Amiga NewIcons Workbench screenshot","AmigaOS 3.2 GlowIcons Workbench screenshot"],"num_per_query":1}

#### Issue name (short)
Palette dependence makes classic icons look “wrong” on custom screens

**Description (what users experience)**  
Classic icons commonly rely on palette indices, so changing palette/screen depth can distort icon colours. Users explicitly report MagicWB icons looking “messed up” under RTG/high-colour configurations, and NewIcons positions itself as a fix by aiming for palette-independent appearance. citeturn12search11turn15search24turn21search13  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
WB2.x through WB3.2.x (palette dependence is intrinsic to classic-style icons; later formats mitigate but do not eliminate mixing issues). citeturn15search24turn21search13  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Most visible when switching between native chipset screens and RTG screens, or when using custom palettes/backgrounds. citeturn12search11turn33search25  

**Frequency signal (High, Medium, Low)**  
High.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- User report: MagicWB icons’ colours are wrong/messed up under OS3.9 + P96 on a 64k-colour screen. citeturn12search11  
- NewIcons documentation: aims to look the same regardless of palette. citeturn15search24turn21search13  
- Icon rendering can degrade if wallpapers consume pens, implying pen/palette constraints affect results. citeturn33search25  

**Repro notes (if any sources describe steps)**  
Use MagicWB/classic icons, switch to RTG/high-colour screen; observe colour mismatch. citeturn12search11  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported: classic icons are palette-indexed; NewIcons adds palette-independence mechanisms; pen availability affects rendering outcomes. citeturn15search24turn33search25  

**Utility mitigation feasibility**

**Solvable by iTidy: Partial/No**  
Partial.

**Approach options (bullet list)**  
- Provide batch conversion workflows (e.g., convert palette-dependent icons to a target format suitable for the user’s environment).  
- Offer “environment profiles” (AGA 4/8 colours vs RTG 16-bit/24-bit) and recommend icon sets accordingly. citeturn33search17turn15search24  

**Risks and constraints (performance, compatibility, user data safety)**  
Conversion can change artwork and degrade quality; must preserve originals and avoid forcing a single icon system on mixed Workbench installs. citeturn15search24turn22search9  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Icon Environment Profiles** — Store presets for “Classic chipset” vs “RTG” and apply safe transformations (or recommendations) per profile.

---

#### Issue name (short)
Icon rendering quality and available pens affect speed and appearance

**Description (what users experience)**  
Users note that icon rendering speed/quality depends on Workbench preference settings, number of colours in screenmode, and even wallpaper (which can consume colour pens and make icon.library’s remapping harder). Separate discussions recommend lowering icon quality settings for speed. citeturn33search25turn33search2turn37view1  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
Strongly evidenced for OS3.5+ icon.library environments; concept applies broadly where pen allocation/remapping exists. citeturn33search25turn33search2  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Most acute on limited-colour screens and systems with heavy background imagery. citeturn33search25turn33search17  

**Frequency signal (High, Medium, Low)**  
High.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- User guidance: speed and quality depend on Workbench prefs, screenmode colours, and wallpaper consuming pens. citeturn33search25  
- Recommendation: adjust icon image quality setting (e.g., “Bad”) for speed gains. citeturn33search2  

**Repro notes (if any sources describe steps)**  
Use a wallpaper and limited pens; open icon-heavy drawer; tweak Workbench icon quality setting; compare speed/appearance. citeturn33search25turn33search2  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported: available pens and Workbench preference settings influence icon.library’s remapping quality and hence speed/appearance. citeturn33search25turn33search2  

**Utility mitigation feasibility**

**Solvable by iTidy: Partial/No**  
Partial.

**Approach options (bullet list)**  
- Detect risky configurations (limited pens + wallpaper + high-quality rendering) and recommend safer settings.  
- Provide “per-drawer optimisation”: flag drawers likely to be slow and propose conversion to simpler icon formats.  

**Risks and constraints (performance, compatibility, user data safety)**  
Changing system preferences may be outside iTidy’s intended scope; safer to warn and guide rather than modify prefs automatically. citeturn33search25turn22search9  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Icon Performance Advisor** — A diagnostic report with actionable recommendations and optional one-click “make icons simpler” operations for selected drawers.

---

#### Issue name (short)
High-colour icon sets (GlowIcons/PNG) trade beauty for speed/memory

**Description (what users experience)**  
Users report that richer icon formats (e.g., 24-bit PNG icons) load more slowly than classic icons and can make scrolling/rendering in large folders sluggish. More general discussion notes that more colours increase redraw load on classic hardware while RTG reduces the impact. citeturn12search3turn33search17turn38view0  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
Primarily a WB3.2.x-era concern because GlowIcons are a common installation option there, but also relevant to users mixing newer icon formats on WB3.0/3.1.x. citeturn33search17turn33search8  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Strongly dependent on whether the machine has RTG; one report contrasts PNG icons with “normal” icons and calls out large folders. citeturn12search3turn33search17  

**Frequency signal (High, Medium, Low)**  
Medium.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- User report: loading 24-bit PNG icons is slower; large folders scroll slower. citeturn12search3  
- Community observation: more colours increase redraw load on classic hardware; RTG mitigates. citeturn33search17  

**Repro notes (if any sources describe steps)**  
Install PNG/high-colour icons for a drawer with many items; compare open/scroll speed to classic icons. citeturn12search3turn33search17  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported at high level: higher colour depth increases rendering/processing demands; RTG changes the performance profile. citeturn33search17turn12search3  

**Utility mitigation feasibility**

**Solvable by iTidy: Partial/No**  
Partial.

**Approach options (bullet list)**  
- Offer a “downshift” workflow (e.g., replace high-cost icons with lower-colour variants in bulk for non-RTG systems).  
- Provide “mixed strategy”: keep rich icons for small key drawers, simplify bulk storage drawers. citeturn33search17turn23search3  

**Risks and constraints (performance, compatibility, user data safety)**  
Batch conversion is CPU-heavy; on 68000/68030 must be optional, incremental, and reversible. citeturn33search17turn22search9  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Icon Format Tiering** — Assign “tiers” (fast/medium/fancy) and apply them per drawer based on size and hardware profile.

---

#### Issue name (short)
Icon format compatibility and conversion pitfalls (NewIcons/GlowIcons/classic)

**Description (what users experience)**  
Users operate in a mixed ecosystem: classic planar icons, NewIcons (implemented as a system patch and described as a “hack”), and later GlowIcons/OS3.5+ formats. Users report trouble installing/using NewIcons on newer systems (e.g., black screen/non-boot), and third-party FAQs note that some newer icon formats appear as a “dot” unless the correct icon.library is present. citeturn15search24turn21search22turn12search22  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
WB2.x–WB3.1.x for NewIcons-era usage, WB3.2.x for modern mixing and installer choices; compatibility risks grow as users mix icon systems across eras. citeturn15search24turn33search17  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Strongly tied to icon.library version expectations and whether the system is using newer icon formats. citeturn12search22turn21search3  

**Frequency signal (High, Medium, Low)**  
Medium.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- NewIcons readme explicitly calls itself a “hack” and warns it may break in future Workbench versions. citeturn15search24  
- Users report NewIcons installation attempts on later systems resulting in black screen / non-boot. citeturn21search22  
- ClassicWB FAQ: GlowIcons appear as a dot without the correct icon.library (OS3.5+ icon support). citeturn12search22  

**Repro notes (if any sources describe steps)**  
Install NewIcons on a newer OS3.2-based setup and observe instability; or open a system with GlowIcons using an older icon.library and see dot placeholders. citeturn21search22turn12search22  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported: NewIcons relies on patching (“hack”) and may break with future Workbench changes; GlowIcons require compatible icon.library support. citeturn15search24turn12search22  

**Utility mitigation feasibility**

**Solvable by iTidy: Partial/No**  
Partial.

**Approach options (bullet list)**  
- Provide a compatibility scanner: detect icon formats used in a directory tree and warn about unsupported mixes for a given target Workbench/icon.library.  
- Offer safe conversion pathways (e.g., NewIcons → classic/GlowIcons) when feasible, with backups and verification.  

**Risks and constraints (performance, compatibility, user data safety)**  
Conversion correctness is hard and can damage icon metadata if done wrongly; must be optional, validated, and reversible. citeturn15search24turn21search3  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Icon Format Linter + Converter Hooks** — Analyse `.info` contents for format markers, produce an actionable report, and optionally run external converters where installed.

---

### Label rendering and legibility

#### Issue name (short)
Icon label background/contrast is a recurring “pet hate”

**Description (what users experience)**  
Users ask how to remove or change the “background colour of text under icons” and discuss the workarounds (e.g., changing font settings). This reflects a recurring theme: label legibility can be poor depending on background patterns, palette, and selection highlight colours. citeturn12search26turn12search0  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
WB2.x onward (label rendering is always present; discussion spans versions). citeturn12search26turn15search26  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
More visible with patterned backgrounds and limited palette/pens. citeturn33search25turn12search26  

**Frequency signal (High, Medium, Low)**  
Medium.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- Hyperion forum: direct question about background colour under icon labels. citeturn12search26  
- EAB thread title/snippet: “Remove background colour of icon text” (showing the same pain point appears independently). citeturn12search0  

**Repro notes (if any sources describe steps)**  
Use a patterned Workbench background; observe label plates or poor contrast; attempt to adjust fonts/label rendering settings. citeturn12search26turn33search25  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Hypothesis: label drawing is constrained by fixed Workbench rendering choices and pen availability; users resort to font/colour preference changes rather than per-drawer behaviour changes. citeturn33search25turn12search26  

**Utility mitigation feasibility**

**Solvable by iTidy: Partial/No**  
Partial.

**Approach options (bullet list)**  
- Provide a “legibility audit” that flags risky combinations (busy backdrop + low contrast) and suggests known fixes (e.g., switch to a solid backdrop for icon-heavy areas; adjust icon quality/pens). citeturn33search25turn33search2  
- Offer per-drawer recommendation presets (e.g., advise “no-pattern” backdrops for large icon drawers).  

**Risks and constraints (performance, compatibility, user data safety)**  
Without patching Workbench, iTidy can’t directly alter how labels are rendered; it can only guide or adjust metadata/settings it safely owns. citeturn22search9turn12search26  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Label Legibility Advisor** — Detects drawers likely to be unreadable and offers “safe defaults” recommendations; optionally generates a report for the user.

---

### Tooling limitations and batch operations

#### Issue name (short)
IconEdit limitations and stability issues (including pen allocation errors)

**Description (what users experience)**  
Icon editing is historically limited: older commentary describes IconEdit as “basic” with restrictions, encouraging third-party editors. In the WB3.2 line, release notes and hotfix announcements explicitly mention IconEdit rendering/undo issues (especially under CyberGraphX), and users report failures related to insufficient colour pens (with “downgrade” prompts). citeturn34search11turn21search7turn34search2  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
Tool limitations span WB2+; the pen-allocation and CGX-related IconEdit fixes are specifically WB3.2.x-era. citeturn34search11turn21search7turn34search2  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
IconEdit issues explicitly mention CyberGraphX in WB3.2.2 hotfix context; pen issues relate to colour availability on the target screen. citeturn21search7turn34search2  

**Frequency signal (High, Medium, Low)**  
Medium.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- Historical: magazine text notes IconEdit is basic and restricted. citeturn34search11  
- Official hotfix announcement lists IconEdit rendering/undo issues under CyberGraphX and other IconEdit fixes. citeturn21search7  
- User report title/snippet: “Could not obtain enough color pens… Downgrade?” citeturn34search2  

**Repro notes (if any sources describe steps)**  
Launch IconEdit in WB3.2.1/3.2.2 scenarios; observe pen error/downgrade prompt or rendering issues under CyberGraphX. citeturn34search2turn21search7  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported at a high level: IconEdit is sensitive to rendering environment (CGX) and pen availability; fixes were required and shipped via hotfixes. citeturn21search7turn34search2  

**Utility mitigation feasibility**

**Solvable by iTidy: Yes/Partial/No**  
Yes (for many outcomes users want), even if not by fixing IconEdit itself.

**Approach options (bullet list)**  
- Avoid interactive icon editing for bulk tasks: provide batch operations to copy icon images/metadata, apply templates, adjust ToolTypes/default tools, and normalise coordinates. citeturn12search5turn12search6  
- Offer “safe screen” guidance (e.g., run on a screen with sufficient colours) as part of workflow docs. citeturn34search2turn33search25  

**Risks and constraints (performance, compatibility, user data safety)**  
Batch `.info` editing risks corruption if format parsing is wrong; must use official icon.library APIs where possible and keep backups. citeturn21search21turn22search9  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Template-based Icon Batch Editor** — Apply a selected template icon (including image and metadata subsets) to many files without requiring IconEdit or per-icon manual work.

---

#### Issue name (short)
Bulk icon metadata workflows are clunky (copying icons without losing ToolTypes/positions)

**Description (what users experience)**  
Users want to change icon imagery while preserving ToolTypes and icon position metadata; third-party utilities exist specifically to “change icons without changing original icon’s tooltypes, and position.” This indicates a long-running friction around safe batch edits. citeturn12search6turn16search30  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
Persistent across all versions where `.info` files contain both image and metadata. citeturn16search30turn21search21  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Not environment-specific; it’s about `.info` structure and safe manipulation. citeturn12search6turn21search21  

**Frequency signal (High, Medium, Low)**  
Medium.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- Aminet tool description explicitly targets this: change icons while preserving ToolTypes/position. citeturn12search6  
- Workbench icon system stores spatial position and ToolTypes in the `.info` file, explaining why naïve copying loses data. citeturn16search30  

**Repro notes (if any sources describe steps)**  
Copy a `.info` over another or use simplistic icon replacement; observe ToolTypes or positions lost; use specialised tool to preserve. citeturn12search6turn16search30  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported: `.info` files bundle both image and metadata (ToolTypes, position), so replacing the whole file overwrites these fields unless a tool merges selectively. citeturn16search30turn21search21  

**Utility mitigation feasibility**

**Solvable by iTidy: Yes/Partial/No**  
Yes.

**Approach options (bullet list)**  
- Implement “merge semantics”: copy only image planes/colour data from a template while preserving ToolTypes, default tool, and coordinates.  
- Provide fine-grained toggles: “replace image only / replace metadata only / replace both”.  

**Risks and constraints (performance, compatibility, user data safety)**  
Must support multiple icon formats and avoid producing icons that older icon.library consumers can’t parse. citeturn15search24turn12search22  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Icon Merge Engine** — Field-level merging for `.info` edits with format-aware safety checks and backups.

---

### Reliability and corruption edge cases

#### Issue name (short)
Workbench doesn’t always refresh icons until Update/Update All

**Description (what users experience)**  
Workbench provides Update/Update All actions that reload/redraw icon windows. Documentation describes Update All as reopening each open Workbench window to show current state, and third-party patches exist because “Workbench doesn’t see new icons, icon image changes, etc.” until notified. citeturn15search26turn12search21turn22search4  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
Persistent; users in later OS contexts still rely on Update/Update All semantics. citeturn15search26turn22search4  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
Not hardware-specific; appears during file operations and icon generation workflows. citeturn12search21turn15search26  

**Frequency signal (High, Medium, Low)**  
Medium.

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- OS3.9 Workbench manual: Update All reopens each open Workbench window to update appearance. citeturn15search26  
- IconAppearer readme states Workbench “doesn’t see” new icons/image changes and the tool exists to patch-notify Workbench. citeturn12search21  

**Repro notes (if any sources describe steps)**  
Create/change icons in a drawer while it is open; observe stale view until Update/Update All or reopen. citeturn12search21turn15search26  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Supported: Workbench’s refresh model requires explicit update/reopen, and/or notification that some tools provide via patching. citeturn12search21turn15search26  

**Utility mitigation feasibility**

**Solvable by iTidy: Partial/No**  
Partial.

**Approach options (bullet list)**  
- Provide post-operation prompts: “Now choose Update/Update All” with clear instructions. citeturn15search26turn22search4  
- Where possible, use Workbench’s documented automation interfaces (e.g., ARexx port) to request updates (only if available and stable; otherwise document manual step). citeturn38view0  

**Risks and constraints (performance, compatibility, user data safety)**  
Automating updates may be version-dependent; must degrade gracefully and never rely on undocumented patches. citeturn12search21turn22search9  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Post-Edit Refresh Assistant** — After bulk edits, show a small instruction panel (“Update / Update All”) and optionally offer an ARexx-driven refresh if supported.

---

#### Issue name (short)
Corrupt icons can crash/freeze Workbench

**Description (what users experience)**  
Users report that some downloaded icons appear corrupted and that clicking them can reboot/freeze the system or produce a black screen. This is consistent with the reality that `.info` files are binary structures parsed by icon.library; malformed data can destabilise icon handling. citeturn15search29turn21search21  

**Affected Workbench versions (WB2.x, WB3.0, WB3.1.x, WB3.2.x)**  
Potentially all, because malformed `.info` parsing is a general hazard; evidence in scope is from modern retro usage. citeturn15search29turn21search21  

**Environment notes (OCS/ECS/AGA, RTG P96/CyberGraphX, CPU class if mentioned, icon format if relevant)**  
User report involves downloading newer software (suggesting mixed icon formats or corruption in transit). citeturn15search29  

**Frequency signal (High, Medium, Low)**  
Low to Medium (visible enough to merit tooling, but fewer corroborated threads in the accessible corpus).

**Evidence (at least 2 independent citations for High, at least 1 for Medium, optional for Low)**  
- Report: corrupted icon clicking causes reboot/freeze/black screen. citeturn15search29  
- Icon library documentation emphasises `.info` as the central interaction point and the API surface for reading/creating icons, framing why malformed data is serious. citeturn21search21  

**Repro notes (if any sources describe steps)**  
Download a program with a corrupted `.info`; simply clicking it triggers crash/freeze. citeturn15search29  

**Likely root cause (only if supported by sources, otherwise label as hypothesis)**  
Hypothesis: invalid DiskObject / icon format data triggers icon.library bugs or unhandled conditions. The sources establish `.info` as the core object parsed by the icon library (but do not prove the exact failure mode). citeturn21search21turn15search29  

**Utility mitigation feasibility**

**Solvable by iTidy: Partial/No**  
Partial.

**Approach options (bullet list)**  
- Build an “icon linter”: attempt to load each `.info` via icon.library calls; if loading fails, quarantine/backup and replace with a safe generic icon. citeturn21search21turn22search9  
- Offer “safe click audit”: identify icons with suspicious structures before the user interacts with them.  

**Risks and constraints (performance, compatibility, user data safety)**  
Must not delete user icons silently; quarantine workflow must preserve originals and log actions. citeturn22search9turn15search29  

**iTidy feature mapping (concrete feature idea with a short spec)**  
**Feature: Icon Integrity Scanner** — For a directory tree, validate each `.info` loads correctly; quarantine failures and optionally regenerate icons from templates.

---

## Version matrix

The matrix summarises whether each issue is materially present in each era, and what changed. “Present” here means “users plausibly encounter it under normal use”; some issues are intensified by newer icon formats in later eras rather than being introduced there. citeturn22search4turn33search17turn21search7  

| Issue (taxonomy) | WB2.x | WB3.0 | WB3.1.x | WB3.2.x | Notes on changes across versions |
|---|---:|---:|---:|---:|---|
| Snapshot semantics confusion | ✔ | ✔ | ✔ | ✔ | Docs still distinguish Snapshot Window vs All; people still forget and layouts revert. citeturn22search4turn22search1 |
| Clean Up reflow surprises | ✔ | ✔ | ✔ | ✔ | Algorithm remains behaviourally “simple”; users still report misalignment cases. citeturn35search1turn22search1 |
| Volatile RAM:/Disk.info persistence quirks | ? | ? | ✔ | ✔ | Strong evidence in later 3.x discussions; workaround uses persistent default icon paths. citeturn12search15turn12search16 |
| Backdrop/Leave Out state awkwardness | ✔ | ✔ | ✔ | ✔ | `.backdrop` file management and reboot note shown in WB3.x context; feature persists. citeturn35search33turn22search1 |
| Pseudo-icons can’t be persisted | ✔ | ✔ | ✔ | ✔ | Users explicitly note pseudo-icons can’t be snapshotted because no `.info`. citeturn22search1turn22search4 |
| “Drawer looks empty” with missing `.info` | ✔ | ✔ | ✔ | ✔ | Icon-only mode remains a trap; Show All Files is the workflow, or batch icon creation. citeturn22search4turn16search30 |
| DefIcons complexity / fragility | ✔ (as add-on) | ✔ | ✔ | ✔ | Two-step rule system; reports of breakage after OS upgrades (3.1.4). citeturn21search16turn21search19 |
| DefIcons performance overhead | ✔ (as add-on) | ✔ | ✔ | ✔ | Documented as reading file content/first block; worsens with huge dirs. citeturn23search3turn38view0 |
| Large-drawer icon rendering slowness | ✔ | ✔ | ✔ | ✔ | OS3.9 cited icon.library optimisation; still a cliff with heavy workloads. citeturn38view0turn23search3 |
| Pen/palette interplay affecting icons | ✔ | ✔ | ✔ | ✔ | Constraint persists; wallpapers/pens and quality settings remain relevant. citeturn33search25turn33search2 |
| High-colour icon trade-offs | — | — | (optional add-ons) | ✔ | WB3.2 installer commonly offers GlowIcons; more colours imply more cost on classic hardware. citeturn33search8turn33search17 |
| Icon format compatibility pitfalls | ✔ | ✔ | ✔ | ✔ | NewIcons is a patch/hack; GlowIcons require icon.library support; mixing can break. citeturn15search24turn12search22turn21search22 |
| Label background/contrast complaints | ✔ | ✔ | ✔ | ✔ | Users still ask how to remove label background/plates; depends on theme/palette. citeturn12search26turn12search0 |
| IconEdit limitations/stability issues | ✔ | ✔ | ✔ | ✔ (notably) | WB3.2 hotfixes explicitly mention IconEdit CGX issues and fixes; “not enough pens” error reported. citeturn21search7turn34search2turn34search11 |
| Refresh/update inconsistency | ✔ | ✔ | ✔ | ✔ | Update/Update All remains the official path; patches exist to force awareness. citeturn15search26turn12search21 |
| Corrupt icons can crash/freeze | ✔ | ✔ | ✔ | ✔ | Risk persists whenever malformed `.info` is encountered. citeturn15search29turn21search21 |

## iTidy opportunity backlog

The backlog below ranks implementable features that directly map to the taxonomy and fit a userland model (scan directories, edit `.info` safely, create backups, avoid Workbench patching). Where performance matters, 68000-class guidance is included. citeturn22search9turn23search3turn16search30  

### Robust layout and persistence

1. **Recursive Grid Layout + Sort Engine**  
Problem(s) solved: Snapshot semantics; Clean Up surprises; pseudo-icon persistence (via Make-Real). citeturn22search1turn22search4turn22search9  
User value: High. Complexity: Medium. Risk: Medium.  
68000 performance: Must support chunked processing (e.g., 100 icons per batch) and resumable runs. citeturn23search3  
UI controls/defaults: spacing preset “Classic 640×256 safe”; sort key default “Name”; toggle folders-first.  
MVP: deterministic grid placement by name for existing icons (only).  
Enhancements: label-aware spacing; collision repair; per-screen presets.

2. **Bulk Snapshot-All Writer (with rollback)**  
Problem(s) solved: Snapshot confusion and forgotten persistence. citeturn22search4turn22search1  
User value: High. Complexity: Low–Medium. Risk: Medium.  
68000 performance: mostly I/O; avoid copying data files; only `.info`. citeturn22search9turn23search3  
UI controls/defaults: always create backup archive; confirm counts before writing.  
MVP: snapshot coordinates for all `.info` under a path.  
Enhancements: selective metadata merge; “only if changed” optimisation.

3. **Backdrop Inventory + Cleaner**  
Problem(s) solved: Leave Out / `.backdrop` awkwardness; orphaned entries. citeturn35search33turn22search1  
User value: Medium–High (for desktop-heavy users). Complexity: Medium. Risk: Medium.  
68000 performance: small.  
UI controls/defaults: “dry-run” report first; offer “remove dead entries” and “backup before edit”.  
MVP: list and delete invalid `.backdrop` entries.  
Enhancements: reorder/sort left-out icons; guided “reboot/update” prompts.

4. **Negative/Out-of-Range Coordinate Repair**  
Problem(s) solved: layout glitches; potential overlaps; protects from tools that mishandle sentinel positions. citeturn22search1turn12search14  
User value: Medium. Complexity: Low. Risk: Low–Medium.  
MVP: clamp coordinates to non-negative and within a user-defined canvas.  
Enhancements: detect and correct overlapping “reserved space” cases.

### Icon creation and type handling

5. **Missing Icon Auditor + Generator (“Make-Real Batch”)**  
Problem(s) solved: empty drawers in icon-only view; pseudo-icon persistence. citeturn22search4turn22search1  
User value: High. Complexity: Medium. Risk: Medium.  
68000 performance: generating many icons is I/O bound but manageable with batching.  
UI controls/defaults: generate only for selected extensions by default; optional “folders only” mode.  
MVP: create drawer/project/tool icons from templates.  
Enhancements: integrate content sniffing cache; per-type templates.

6. **Static Filetype Iconiser (DefIcons alternative)**  
Problem(s) solved: DefIcons fragility; show-all-files usability without runtime sniffing. citeturn21search16turn21search19turn23search3  
User value: High for download-heavy workflows. Complexity: High. Risk: Medium–High.  
68000 performance: must support “extensions-only fast mode”; header sniffing optional. citeturn23search3turn23search8  
UI controls/defaults: start with extensions-only; allow user mapping to templates.  
MVP: extension mapping to template icons + default tools.  
Enhancements: header sniffing cache; conflict resolution UI.

7. **DefIcons Asset Manager (if user uses DefIcons)**  
Problem(s) solved: adding new types; preventing collisions; managing `def_*.info` templates. citeturn23search28turn21search16  
User value: Medium. Complexity: Medium. Risk: Medium.  
MVP: add/remove def_* icons and maintain a documented structure.  
Enhancements: sanity-check for OS upgrades; guided troubleshooting.

### Performance-focused utilities

8. **Large Drawer Optimiser Report**  
Problem(s) solved: slow opens, DefIcons overhead guidance, icon format budgets. citeturn23search3turn33search17turn38view0  
User value: High. Complexity: Medium. Risk: Low.  
68000 performance: report generation must be O(n) and streaming.  
MVP: statistics + recommendations only.  
Enhancements: one-click “apply safer layout + make-real + simplify icons”.

9. **Icon Format Tiering (fast/medium/fancy per drawer)**  
Problem(s) solved: high-colour icon trade-offs. citeturn12search3turn33search17  
User value: Medium. Complexity: High. Risk: Medium–High.  
68000 performance: conversions must be optional and off by default.  
MVP: classification + advice only.  
Enhancements: external converter integration and safe batch conversion.

10. **One-Time Sniff + Cache (for type recognition)**  
Problem(s) solved: DefIcons performance cliff; repeated I/O costs. citeturn23search3turn38view0  
User value: Medium–High. Complexity: High. Risk: Medium.  
68000 performance: incremental sniffing; persistent cache file; throttling.  
MVP: cache only extension matches.  
Enhancements: header sniffing for unknowns.

### Batch editing and metadata hygiene

11. **Icon Merge Engine (image-only or metadata-only replacements)**  
Problem(s) solved: “change image but keep ToolTypes/positions”. citeturn12search6turn16search30  
User value: High. Complexity: Medium–High. Risk: Medium.  
MVP: copy image from template icon onto targets while preserving ToolTypes + coords.  
Enhancements: per-field merge UI.

12. **Default Tool Validator + Fixer**  
Problem(s) solved: “icon has no default tool” and broken associations; improves double-click reliability. citeturn22search9turn16search30  
User value: Medium. Complexity: Medium. Risk: Medium.  
68000 performance: path existence checks are cheap.  
MVP: detect missing default tool targets and produce a report.  
Enhancements: bulk fix using user-selected associations.

13. **ToolTypes Bulk Editor (safe subset)**  
Problem(s) solved: repeated manual IconEdit work; reduces reliance on unstable IconEdit paths. citeturn34search11turn21search7turn16search30  
User value: Medium. Complexity: Medium. Risk: Medium.  
MVP: add/remove/replace ToolTypes lines across selected icons.  
Enhancements: template variables; per-program presets.

14. **Drawer Metadata Manager (view mode, show mode, window geometry)**  
Problem(s) solved: inconsistent drawer presentation; supports “standardised workspace”. citeturn22search4turn16search30  
User value: Medium. Complexity: Medium. Risk: Medium.  
MVP: read/normalise drawer `.info` view settings; write consistent defaults.  
Enhancements: per-profile sets (work/dev/games).

15. **Icon Integrity Scanner + Quarantine**  
Problem(s) solved: crashes/freezes from corrupt `.info`. citeturn15search29turn21search21  
User value: Medium (high when needed). Complexity: Medium. Risk: High (must not destroy icons).  
MVP: attempt to load `.info` via icon.library; list failures.  
Enhancements: quarantine + regenerate safe icons + restore controls.

### User guidance and safety UX

16. **Backup/Restore Points as a first-class workflow**  
Problem(s) solved: safety concerns for all batch edits. citeturn22search9turn38view0  
User value: High. Complexity: Medium. Risk: Low.  
MVP: automatic backup archive per run; restore UI.  
Enhancements: retention policy; incremental backups.

17. **Dry-run Preview with Diff Summary**  
Problem(s) solved: fear of mass changes; reduces mistakes. citeturn22search9turn16search30  
User value: High. Complexity: Medium. Risk: Low.  
MVP: show counts of icons moved/created/edited and sample changes.  
Enhancements: per-file diff view; export report to text.

18. **Post-Edit Refresh Assistant**  
Problem(s) solved: “why didn’t my changes show up?” refresh confusion. citeturn15search26turn12search21turn22search4  
User value: Medium. Complexity: Low. Risk: Low.  
MVP: show instruction prompt after operations.  
Enhancements: optional ARexx-based Update All if available.

19. **Environment Profile Wizard (Classic chipset vs RTG)**  
Problem(s) solved: palette/pen trouble; icon format tiering recommendations. citeturn33search17turn33search25turn15search24  
User value: Medium. Complexity: Medium. Risk: Low.  
MVP: user selects profile; iTidy adjusts only its own layout/generation defaults.  
Enhancements: automated detection heuristics.

20. **“Explain this drawer” diagnostics page**  
Problem(s) solved: opaque behaviours (pseudo-icons, DefIcons overhead, palette/quality interactions). citeturn22search1turn23search3turn33search25  
User value: Medium. Complexity: Low. Risk: Low.  
MVP: one report per drawer: icon count, missing `.info`, formats, estimated cost flags.  
Enhancements: links to in-app fixes.

## Test plan ideas

A compact test set to validate iTidy on real machines and emulators, focusing on repeatability, safety, and performance. citeturn23search3turn22search9turn33search17  

1. **Large drawer stress**: 1,000 files with existing `.info` icons; measure iTidy runtime and verify no data files touched (only `.info`). citeturn23search3turn22search9  
2. **Large drawer + missing icons**: 1,000 files, only 100 have `.info`; run Missing Icon Auditor and generate icons for a subset; verify Show Only Icons now shows expected items. citeturn22search4turn22search1  
3. **DefIcons performance scenario**: directory with many mixed-type files; compare “runtime DefIcons + Show All Files” vs “iTidy make-real static icons” user experience. citeturn23search3turn38view0  
4. **Backdrop icons**: create `.backdrop` entries, run Backdrop Inventory + Cleaner, verify dead entries removed and backups restorable. citeturn35search33turn22search9  
5. **Snapshot equivalence**: move all icons randomly, run iTidy Bulk Snapshot-All Writer, reboot, confirm positions persist. citeturn22search4turn22search1  
6. **Clean Up replacement**: run iTidy grid layout, then run Workbench Clean Up, check that iTidy can re-apply deterministic layout and restore grid. citeturn22search1turn35search1  
7. **Long-label edge**: create icons with long names; verify “label-safe spacing” avoids overlap and preserves consistent columns. citeturn35search1turn35search4  
8. **Mixed icon formats**: mix classic, NewIcons-marked icons, and GlowIcons; run format linter; confirm accurate reporting and no destructive conversions. citeturn15search24turn12search22  
9. **Low-colour screen simulation**: run on an OCS/ECS-style 4-colour Workbench screen; verify iTidy does not create icons that require unsupported formats by default. citeturn33search17turn12search22  
10. **RTG 16-bit/24-bit**: run on P96/CyberGraphX high-colour screen; verify iTidy’s performance advice highlights pen/palette interactions and does not break existing rich icons. citeturn12search11turn33search25  
11. **IconEdit avoidance validation**: perform batch operations (template apply, ToolTypes edits) and confirm no IconEdit dependency. citeturn34search11turn21search7  
12. **Default tool validator**: create icons with missing default tool paths; run validator; confirm correct detection and safe fix workflow. citeturn16search30turn22search9  
13. **Workbench refresh reality check**: run iTidy while target drawer is open; confirm the “refresh assistant” guidance correctly restores visibility via Update/Update All. citeturn15search26turn12search21  
14. **Corrupt icon quarantine**: introduce a known-bad `.info` (or simulated failure); confirm iTidy detects load failure, quarantines, and restores without crashing Workbench. citeturn15search29turn21search21  
15. **Rollback certainty**: run a destructive-ish test (mass relocate); restore from backup archive; confirm byte-identical `.info` restoration and stable Workbench boot. citeturn22search9turn15search29