## Why iTidy Exists

iTidy was originally created to solve a very specific and very common Amiga problem: **Workbench can only tidy one folder at a time**. When you’re working with large software collections—especially WHDLoad packs—you often extract **thousands of folders and subfolders** at once. Tidying each one manually is slow, repetitive, and easy to get wrong.

Over time the project grew into a broader toolkit for keeping classic Workbench installations neat, consistent and recoverable, without turning every re-layout into a weekend job.

### The Core Problems iTidy Tackles

At a high level, iTidy exists to fix four long-standing Workbench pain points:

1. **One-folder-at-a-time tidying**

   - Workbench’s built-in “Clean Up” works on a single drawer at once.
   - Large WHDLoad sets, demo packs, magazine disks and archive extractions can contain thousands of drawers.
   - Doing these by hand is tedious and almost impossible to keep consistent across a whole disk.

2. **Layouts that don’t match your actual setup**

   - Changing fonts or moving to RTG / higher resolutions often leaves icons overlapping or spread out in awkward window shapes.
   - Some drawers end up as tall, scrolling “towers”; others stretch across the screen with only a few icons per row.
   - Workbench itself has no way to say “make all these drawers use a sensible, consistent layout grid”.

3. **Broken or outdated default tools**

   - Many archives and old projects hard-code default tools like `DeluxePaintIII:DPaint` or system paths that don’t exist on your machine.
   - After copying disks between machines or unpacking collections, you end up with icons that just say “Unable to open your tool”.
   - Fixing these one icon at a time is frustrating, and easy to miss.

4. **Fear of breaking things with large-scale changes**

   - Re-laying out *one* drawer is safe; re-laying out *a whole partition* is not.
   - Workbench has no built-in “undo” for icon positions, window geometry or default tools.
   - That makes people reluctant to clean up, even when their system has become a mess.

iTidy exists to address all of these at once, while still respecting how real Amiga systems are used.

---

## How iTidy Helps

### 1. One Run, Many Folders

Instead of tidying drawers one by one, iTidy can process **an entire directory tree in a single run**. You point it at a folder (for example `DH0:Games`, `Work:Projects` or even a full partition) and, if desired, enable **Cleanup subfolders** so every drawer beneath it is treated consistently.

This makes it ideal for:

- WHDLoad game, demo, magazine and beta collections split into A/B/C/D subfolders
- Large archive extractions (e.g. TOSEC, “mega packs”, magazine coverdisk sets)
- Project trees with many nested drawers that should all follow the same layout rules

During a run, iTidy:

1. Scans each folder for icons
2. Sorts them using your chosen **Order** and **Sort By** options
3. Lays them out using its layout engine (see below)
4. Resizes and repositions each folder window
5. Optionally creates LhA backups before touching anything, so you can roll back later

### 2. Layouts That Respect Fonts, Screens and Taste

iTidy doesn’t just call Workbench’s “Clean Up” and walk away. Its layout engine lets you describe the sort of windows you **want**, and then applies that consistently:

- **Layout aspect ratio** presets (Tall, Square, Classic, Wide, Ultrawide) to control overall window shape.
- **Overflow strategy** (expand horizontally / vertically / both) for handling folders with large icon counts.
- **Icon spacing** sliders for horizontal and vertical gaps, so you can choose between dense or airy layouts.
- **Min / max icons per row** and **max window width** to prevent odd extra‑tall, single‑column icon stacks, or windows that span half your RTG monitor.
- **Column optimisation and centering** for neat, compact grids even when icons have very different widths.

In practical terms, this means you can:

- Switch to a new Workbench font without your drawers turning into a jumble
- Move from 640×256 PAL to RTG and keep windows sensible, not absurdly wide
- Apply the same layout style across *thousands* of drawers, instead of fiddling with them one by one

You can also control how window positions behave after tidying:

- **Center Screen** – classic iTidy behaviour
- **Keep Position** – respect the existing location, only resize
- **Near Parent** – open child drawers just down and to the right of their parent
- **No Change** – only adjust icon layout, never move the window

This helps iTidy fit the way you naturally arrange windows on your own system.

### 3. Fixing Broken Default Tools at Scale

As iTidy evolved, it gained a dedicated **Default Tool Analysis / Fix Default Tools** tool for repairing and standardising default tools across large collections.

Key capabilities:

- **Recursive scanning** – examines **WBPROJECT icons (data files)** in the selected folder and all subfolders. Executable WBTOOL icons are skipped for default‑tool validation because Workbench ignores their default tool field.
- Builds its PATH search list by parsing `S:Startup-Sequence` and `S:User-Startup`, honouring both `Path DIR` and `Path DIR ADD`. This allows iTidy to find tools not only in `C:` but also in custom directories such as `Workbench:sc/c` or `Workbench:Utilities`, and works the same whether launched from Workbench or CLI.
- Shows a **tool-centric view**: each distinct default tool, how many icons use it, and whether it EXISTS or is MISSING.
- Lets you **batch-replace** a missing tool across all icons that use it, or fix selected icons one by one when needed.
- Automatically creates lightweight **default-tool backups** so you can revert tool changes later.

During a full iTidy run, default-tool information is also recorded automatically. When you open the Default Tool Analysis window after a run, the tool list is already populated, avoiding a second long scan.

This turns the long, error-prone job of hunting down broken default tools into a manageable, auditable task.

### 4. Safe Experimentation with Backups and Restore

Because iTidy can change a lot, its backup and restore system is a core part of **why** it’s usable:

- Optional **LhA backups** (one archive per folder) are created *before* any icons are touched.
- Each run has its own run directory and human-readable catalog file, so you can see exactly what was backed up.
- The **Restore Backups** window lets you pick a run, inspect its details, and restore all affected folders back to their previous state, including icon positions and window geometry (if chosen).

In practice, this means:

- You can “have a go” at cleaning an entire drive, knowing you can restore if you dislike the result.
- Risky operations such as stripping NewIcon borders or bulk default-tool changes are far safer when combined with backups.

On top of this, experimental **Beta Options** (such as re-aligning already-open windows or detailed logging) exist to make it easier to see what iTidy is doing and refine how it behaves on your particular setup.

---

## When iTidy Is Especially Useful

Some typical scenarios where iTidy shines:

- After unpacking a **large WHDLoad set** or TOSEC archive and wanting everything to look “intentional” instead of random.
- When moving from original Commodore screens to **RTG** and needing to re-layout drawers for a new resolution and font.
- When curating a **custom Workbench install** (for yourself or others) and wanting consistent window shapes and icon spacing everywhere.
- When inheriting an old disk or backup full of icons with **broken default tools** and wanting them to “just open” in sensible programs again.

---

## Important Note on Safety

iTidy is a personal, hobby-driven project provided in good faith and without any warranties. While it is designed to operate safely and includes both icon backups and separate default-tool backup mechanisms, you should always ensure you have a **full backup of your system or important folders** before using it. iTidy can make large numbers of changes very quickly, and although these changes can be reverted when backups are enabled, you use the software entirely **at your own risk**.

---

## What Types of Icons Does iTidy Support?

iTidy can automatically tidy and process all major Amiga icon formats used on Workbench 3.x systems:

- **Standard/Classic Icons** (used on all AmigaOS versions)
- **NewIcons** (extended color icons, popular on OS3.x)
- **OS3.5/OS3.9 Color Icons** (modern color icons introduced in AmigaOS 3.5+)

*Side note: GlowIcons are fully supported—both in their traditional NewIcons format and the newer Color Icons format.*

iTidy detects and handles these formats automatically. You do not need to choose or convert icons—just run iTidy and it will tidy, sort, and update all supported icon types in your folders.

---

The goal of iTidy is simple:

**Make Workbench folder layouts consistent, neat and effortless — even across massive directory trees — while giving you enough safety nets to feel comfortable letting it do the heavy lifting.**

