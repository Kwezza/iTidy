# iTidy Shared Core Unification — Plan of Action

## 1. Goal

The long-term goal is to evolve **iTidy Classic** (currently iTidy v1.x) and **iTidy2** into two user-interface front ends built on top of the same common codebase.

The intended end state is:

- **iTidy Classic**
  - Workbench 2.x / 3.0 / 3.1 compatible
  - GadTools-based GUI
  - Suitable for low-end systems including a stock 68000
  - Uses the shared core for all practical non-GUI functionality

- **iTidy2**
  - Workbench 3.2+ compatible
  - ReAction-based GUI
  - May expose richer options and use newer OS facilities where appropriate
  - Uses the same shared core wherever the functionality is common

The central architectural principle is:

> **Business logic, algorithms, file parsing, icon handling, image processing, sorting, layout and other reusable functionality belong in shared code. GUI and OS-version-specific presentation belong in the individual front ends.**

The intention is not to merge the two GUIs. Each GUI should remain appropriate to its target Workbench generation.

Instead, the two applications should increasingly become:

```text
                 +----------------------+
                 |    Shared iTidy Core |
                 +----------------------+
                    /                \
                   /                  \
        +------------------+    +------------------+
        |  iTidy Classic   |    |      iTidy2      |
        |  GadTools GUI    |    |   ReAction GUI   |
        |  WB2.x-3.1       |    |   WB3.2+         |
        +------------------+    +------------------+
```

This will reduce duplicated development, prevent the two versions drifting apart, and ensure that fixes and improvements to shared functionality benefit both programs.

---

## 2. Compatibility Rule for Shared Code

The shared core should target the **lowest practical common denominator**.

Unless there is a very strong reason otherwise, shared code should be compatible with:

```text
CPU:        68000+
Workbench:  2.x+
Compiler:   VBCC / portable C89-C99 subset used by both projects
Memory:     suitable for low-memory classic Amigas
```

Shared modules should avoid depending directly on:

- ReAction
- GadTools
- DefIcons
- Workbench 3.2 APIs
- icon.library v44+
- datatypes where not essential
- AGA
- RTG
- CyberGraphX
- floating point
- FPU
- 68020+ instructions
- GUI objects
- application-global window state

If newer functionality is required by iTidy2, that functionality should normally live in an iTidy2-specific layer above the shared code.

---

## 3. Architectural Target

A possible final source tree is:

```text
iTidy/
|
+-- shared/
|   |
|   +-- core/
|   |   +-- directory scanning
|   |   +-- sorting
|   |   +-- layout
|   |   +-- path handling
|   |   +-- default-tool analysis
|   |   +-- common preferences/data models
|   |
|   +-- icon/
|   |   +-- raw .info parser
|   |   +-- icon format probing
|   |   +-- classic planar decoding
|   |   +-- NewIcons decoding
|   |   +-- OS3.5 ColorIcon decoding
|   |   +-- GlowIcon decoding
|   |   +-- common icon metadata
|   |
|   +-- image/
|   |   +-- image types
|   |   +-- scaling
|   |   +-- palette mapping
|   |   +-- quantisation
|   |   +-- dithering
|   |
|   +-- backup/
|   |   +-- common backup engine where practical
|   |
|   +-- platform/
|       +-- allocation abstraction
|       +-- file I/O helpers
|       +-- logging callbacks/shims
|       +-- OS-version-safe helpers
|
+-- classic/
|   +-- main
|   +-- GadTools GUI
|   +-- classic requesters
|   +-- WB2.x/3.x presentation
|   +-- classic-specific platform glue
|   +-- classic planar icon writer if still frontend-specific
|
+-- itidy2/
    +-- main
    +-- ReAction GUI
    +-- DefIcons integration
    +-- WB3.2-specific helpers
    +-- modern icon writing
    +-- modern thumbnail orchestration
```

This is a direction, not a requirement to reorganise the entire repository immediately.

Migration should happen incrementally.

---

# Part A — First Shared Subsystem: Image and Icon Processing

## 4. Why Start Here

The proposed classic-icon conversion feature provides an ideal first shared subsystem.

Previous source analysis found that iTidy2 already contains proven 68000-compatible code for:

- RGB/indexed image scaling
- independent destination width and height
- nearest-colour palette matching
- palette quantisation
- Bayer ordered dithering
- Floyd-Steinberg error diffusion

These algorithms are already substantially independent of the iTidy2 GUI.

The direct icon-decoding research also established that NewIcons and OS3.5 ColorIcons/GlowIcons can be decoded without requiring icon.library v44+, making it practical to create a decoder usable by both applications.

This subsystem should therefore establish the pattern used for later code unification.

---

# Phase 1 — Establish Shared Source Infrastructure

## 5. Create the Shared Source Area

Introduce a shared source location, initially containing only code genuinely used by both programs.

Recommended initial structure:

```text
shared/
    image/
    icon/
```

Do not move unrelated code yet.

### Requirements

Both build systems must be able to:

- compile sources from `shared/`
- include headers from `shared/`
- build the same shared files for 68000
- avoid linking frontend-specific dependencies into shared modules

The shared source files should not contain conditional GUI code.

Avoid a design such as:

```c
#ifdef ITIDY_CLASSIC
    ...
#else
    ...
#endif
```

throughout the common algorithms.

Where behaviour differs, prefer:

- explicit function parameters
- option structures
- callbacks
- small frontend/platform wrappers

---

# Phase 2 — Extract iTidy2 Image Processing

## 6. Move Only Proven Image Kernels

Extract the reusable image-processing code identified in the earlier audit.

Recommended modules:

```text
shared/image/
    image_types.h
    image_scale.c
    image_scale.h
    image_palette.c
    image_palette.h
    image_dither.c
    image_dither.h
```

### `image_scale`

Contains generic functions such as:

- indexed area-average scaling
- RGB24 area-average scaling
- optional prefiltering

The scaler must accept independent target width and target height.

This is required later for non-square-pixel compensation on classic Amiga display modes.

### `image_palette`

Contains:

- nearest-colour matching
- palette remapping
- basic colour calculations
- Median Cut or other existing generic quantisation routines where useful

Do not make iTidy2's current high-level `itidy_reduce_palette()` the shared API if it embeds thumbnail-specific palette policy.

The shared layer should allow the caller to supply an exact destination palette.

### `image_dither`

Contains:

- Bayer ordered dithering
- Floyd-Steinberg dithering
- optional automatic selection helper if it remains genuinely generic

### Explicitly leave in iTidy2

Do not migrate:

- thumbnail GUI/preferences policy
- DefIcons logic
- picture datatype loading
- IFF preview orchestration unless separately justified later
- ReAction progress windows
- thumbnail bevel policy
- iTidy2-specific icon save code
- Workbench 3.2-specific wrappers

---

# Phase 3 — Prove the Image Refactor

## 7. Regression Test Before Adding New Features

The first extraction must be behaviour-preserving.

Create a small repeatable image/thumbnail regression corpus.

Suggested cases:

```text
ILBM
PNG
GIF
JPEG
BMP

4 colours:
    no dither
    Bayer
    Floyd-Steinberg

16 colours:
    no dither
    Bayer
    Floyd-Steinberg

higher-colour case
```

Compare output before and after the extraction.

Where practical compare:

- dimensions
- palette entries
- pixel indexes
- generated icon imagery

The goal is:

> **Moving the code into `shared/image` must not alter existing iTidy2 thumbnail output.**

Do not improve or rewrite algorithms during this phase.

Refactor first; improve later.

---

# Phase 4 — Define Shared Icon/Image Data Structures

## 8. Create a Neutral Decoded-Image Representation

The shared icon decoders need a representation that neither frontend owns.

Keep it deliberately small.

Conceptually:

```c
typedef struct
{
    UBYTE r;
    UBYTE g;
    UBYTE b;
} iTidy_RGB8;

typedef struct
{
    UWORD width;
    UWORD height;

    UBYTE *pixels;              /* chunky palette indexes */

    iTidy_RGB8 *palette;
    UWORD palette_count;

    LONG transparent_index;     /* -1 = none */

} iTidy_IndexedImage;
```

Then an icon can contain:

```c
typedef struct
{
    ULONG source_format;

    iTidy_IndexedImage normal;
    iTidy_IndexedImage selected;

    BOOL has_selected;

} iTidy_DecodedIcon;
```

The exact final definition should be based on real decoder requirements.

Avoid importing large existing structures such as `iTidy_IconImageData` if they contain:

- gadget-specific fields
- iTidy2 metadata
- OS3.5 icon-writing state
- frontend policy

The shared representation should describe decoded image data, not application behaviour.

---

# Phase 5 — Implement a Safe Raw `.info` Reader

## 9. Build the Common Icon File Layer

Create:

```text
shared/icon/
    icon_file.c
    icon_file.h
```

Its purpose is to understand the serialized `.info` file envelope without relying on icon.library v44.

Responsibilities include:

- validate the classic DiskObject header
- read big-endian words and longs safely
- walk serialized classic icon structures
- identify normal and selected classic Image data
- locate DefaultTool
- locate ToolTypes
- handle DrawerData where applicable
- locate data appended after the classic icon structure
- expose safe offsets and lengths to format-specific decoders

### Critical rule

Do not cast raw file data directly to in-memory Amiga structures.

Do not assume:

```c
struct DiskObject *icon = (struct DiskObject *)buffer;
```

The `.info` file is serialized data and should be parsed field by field.

### Safety requirements

Every parser operation must check:

- file bounds
- integer overflow
- allocation size
- width/height sanity
- palette-size sanity
- data-length sanity

Malformed icons must return errors, not crash the application.

---

# Phase 6 — Add an Icon Probe API

## 10. Detect Formats Without Fully Decoding Them

Create:

```text
shared/icon/
    icon_probe.c
    icon_probe.h
```

The probe API should report what representations are present.

Conceptually:

```c
typedef struct
{
    BOOL has_classic;
    BOOL has_newicons;
    BOOL has_coloricon;
    BOOL has_glowicon;
    BOOL has_os4_argb;
    BOOL has_png;

    BOOL has_normal;
    BOOL has_selected;

} iTidy_IconProbe;
```

This allows either application to inspect an icon cheaply.

Example future use:

```text
Source icon:
    Classic fallback: present
    NewIcons: absent
    ColorIcon/GlowIcon: present

Current Workbench:
    cannot display enhanced image

Recommendation:
    conversion available
```

The probe layer must remain read-only.

---

# Phase 7 — Implement ColorIcon / GlowIcon Decoding

## 11. Decode OS3.5 ColorIcon Data Directly

Create:

```text
shared/icon/
    icon_coloricon.c
    icon_coloricon.h
```

Implement progressively:

1. detect `FORM ICON`
2. walk IFF-style chunks safely
3. parse `FACE`
4. parse uncompressed `IMAG`
5. decode palette
6. decode indexed pixel data
7. handle transparency
8. handle selected image
9. handle selected-image palette inheritance
10. add image RLE decoding
11. add palette RLE decoding
12. ignore unknown chunks safely

The output must be the neutral `iTidy_DecodedIcon` representation.

### Important separation

This module must not:

- resize
- dither
- map to Workbench pens
- apply PAL aspect correction
- write icons

It only decodes what is stored.

GlowIcons should be treated according to the actual format findings rather than as a separate GUI feature. If GlowIcons use the ColorIcon format with particular imagery/palette conventions, keep one decoder and expose useful metadata rather than duplicate the parser.

---

# Phase 8 — Validate ColorIcon Decoder in iTidy2

## 12. Use icon.library v44+ as a Test Oracle

Although the decoder is intended to work without v44, iTidy2 can use v44 during development to validate it.

For each test ColorIcon/GlowIcon compare the direct decoder against the existing OS-supported path.

Compare:

- width
- height
- palette count
- palette RGB values
- normal pixel indexes
- selected pixel indexes
- transparency
- presence/absence of selected image

Where possible compare buffers exactly.

This gives strong evidence that the direct parser is correct before it is relied upon by iTidy Classic.

---

# Phase 9 — Implement NewIcons Decoding

## 13. Decode `IM1=` and `IM2=` ToolTypes

Create:

```text
shared/icon/
    icon_newicons.c
    icon_newicons.h
```

Responsibilities:

- identify NewIcons markers/tooltypes
- decode `IM1=` normal imagery
- decode `IM2=` selected imagery
- reconstruct palette
- reconstruct chunky pixel indexes
- expose transparency
- bounds-check all bitstream operations

### Critical NewIcons rule

Treat each ToolType entry as its own encoded bitstream segment.

Do not simply concatenate multiple `IM1=` or `IM2=` strings before decoding if the format specifies reset behaviour at line boundaries.

The bit accumulator must be handled according to the actual NewIcons format.

Create explicit tests for ToolType-boundary cases.

---

# Phase 10 — Add Classic Planar Icon Decoding

## 14. Decode Existing Classic Images Into the Same Representation

Create:

```text
shared/icon/
    icon_classic.c
    icon_classic.h
```

Convert classic Amiga planar `struct Image` data into chunky indexes.

This provides a consistent interface:

```text
classic planar
NewIcons
ColorIcon / GlowIcon
        |
        v
iTidy_DecodedIcon
```

Classic images do not normally contain their own RGB palette.

Represent this clearly rather than pretending a palette was embedded.

The caller can associate classic indexes with the relevant Workbench pens.

---

# Phase 11 — Create the Unified Icon Decode API

## 15. One Common Entry Point

After individual decoders are stable, provide an API such as:

```c
itidy_icon_probe(...);
itidy_icon_decode_classic(...);
itidy_icon_decode_newicons(...);
itidy_icon_decode_coloricon(...);
itidy_icon_decode_best(...);
itidy_icon_free(...);
```

Suggested enhanced-image preference order:

```text
ColorIcon / GlowIcon
        ↓
NewIcons
        ↓
classic planar
```

Unsupported later formats should return explicit status values rather than being reported as damaged files.

Examples:

```text
unsupported OS4 ARGB
unsupported PNG icon
malformed ColorIcon
malformed NewIcons
no usable image
```

---

# Phase 12 — Build a Permanent Icon Regression Corpus

## 16. Store Representative Icons With Expected Results

Create test sets such as:

```text
tests/icons/
    classic/
    newicons/
    coloricons/
    glowicons/
    malformed/
```

Include representative combinations.

### NewIcons

- normal only
- normal + selected
- 4 colours
- 8 colours
- 16 colours
- larger palettes
- transparency
- multi-ToolType encoded images
- awkward bit-boundary cases

### ColorIcons / GlowIcons

- raw image data
- RLE image
- raw palette
- RLE palette
- normal only
- normal + selected
- selected image sharing palette
- transparency
- unknown chunks
- odd-sized chunks

### Malformed icons

- truncated file
- invalid size
- incomplete chunk
- RLE overrun
- impossible dimensions
- corrupt ToolType encoding
- unsupported extension

For each valid sample, store expected values such as:

- dimensions
- palette count
- CRC/checksum of decoded pixels
- CRC/checksum of palette
- selected-image presence
- transparency index

This corpus should become part of future regression testing for both programs.

---

# Part B — Build the Classic Icon Conversion Feature

# Phase 13 — Acquire the Target Workbench Palette

## 17. Use the Actual Workbench Pens

The classic converter should not assume that Workbench uses a fixed blue/grey/white/black palette.

Read the actual colours associated with the relevant pens from the current Workbench screen.

Convert them into the shared RGB8 palette representation.

The converter can then call the existing shared remap/dither code using exactly those destination colours.

This is a key advantage of the extracted image code.

---

# Phase 14 — Add Screen Geometry / Pixel-Aspect Policy

## 18. Keep Screen Policy Outside the Image Scaler

The shared scaler accepts:

```text
source width
source height
destination width
destination height
```

The decision about *what those destination dimensions should be* belongs in conversion policy.

Create logic capable of considering:

- PAL vs NTSC
- HiRes vs LoRes
- interlaced vs non-interlaced
- explicit user override
- current screen
- future CLI target-size overrides

Do not hard-code PAL correction into `image_scale.c`.

The scaler should remain generic.

---

# Phase 15 — Implement Classic Planar Output

## 19. Convert the Final Indexed Image to Bitplanes

After scaling and mapping to four Workbench colours, the output will normally contain pixel values:

```text
0
1
2
3
```

Convert those indexes into two Amiga bitplanes.

Create the appropriate classic icon Image structures and write a classic-compatible `.info`.

Initially this writer may remain iTidy Classic-specific if iTidy2 has no need to generate classic icons.

If iTidy2 later gains an explicit "Export Classic Icon" feature, promote the writer into `shared/icon`.

---

# Phase 16 — Preserve Metadata and Protect the Original

## 20. Conversion Must Be Non-Destructive by Design

The converter should preserve all non-image icon information that can safely be preserved, including where applicable:

- icon type
- DefaultTool
- ToolTypes
- StackSize
- icon position
- drawer geometry
- gadget flags
- ToolWindow
- other classic metadata

Only the display image should intentionally change.

The existing iTidy backup philosophy should be used for conversion operations as well.

Prefer a process resembling:

```text
read original
    ↓
decode completely
    ↓
generate converted icon
    ↓
write temporary file
    ↓
read/validate temporary result
    ↓
backup original
    ↓
replace original
```

Do not destroy the only copy of enhanced icon data.

For the first implementation, retaining the complete original `.info` in backup is safer than attempting clever in-place preservation of every unknown extension.

---

# Part C — Progressive Unification Beyond Icons

## 21. Use the Icon Work as the Architectural Template

Once `shared/image` and `shared/icon` are proven in both applications, migrate other duplicated functionality gradually.

Do not attempt one large "merge iTidy and iTidy2" rewrite.

Suggested order:

### 1. Image processing

Already addressed by the first migration.

### 2. Icon parsing and metadata

The direct icon parser becomes common infrastructure.

### 3. Generic string and path utilities

Move only routines that are genuinely identical.

### 4. Sorting

One shared implementation for:

- folders first
- files first
- mixed
- grouped modes where supported

Front ends simply expose whichever settings are available.

### 5. Layout calculations

Move:

- grid calculations
- spacing
- aspect-ratio layout
- column sizing
- positioning algorithms

The GUI should only populate the layout options structure.

### 6. Directory scanning

Move generic traversal and icon discovery.

Keep GUI progress reporting behind callbacks.

### 7. Backup engine

Investigate whether the backup implementation can operate from shared code with frontend callbacks.

The existing backup architecture is already modular and therefore may be a good candidate for later migration.

### 8. Preferences data model

Aim eventually for a shared settings/data structure for options that exist in both applications.

Each GUI can expose a different subset.

Do not force iTidy Classic to display every iTidy2 option.

### 9. Default-tool analysis

Move common scanning/path-validation logic once dependencies are separated.

---

# Part D — Rules for Shared APIs

## 22. Shared Code Must Not Know Which GUI Is Calling It

Avoid frontend-specific conditionals inside algorithms.

Bad:

```c
#ifdef ITIDY_CLASSIC
    classic_progress();
#else
    reaction_progress();
#endif
```

Better:

```c
typedef BOOL (*iTidy_ProgressCallback)(
    ULONG current,
    ULONG total,
    APTR user_data);
```

Then each frontend supplies its own implementation.

The same principle applies to:

- progress
- cancellation
- logging
- warnings
- user confirmation
- status messages

Shared code reports state.

The frontend decides how to display it.

---

## 23. Prefer Options Structures to Globals

Avoid shared code reading application-global preferences.

Bad:

```c
prefs = GetGlobalPreferences();
```

inside a shared scaler/layout/icon decoder.

Better:

```c
itidy_calculate_layout(
    items,
    item_count,
    &options,
    &result);
```

This:

- makes code reusable
- improves testability
- prevents GUI coupling
- makes future CLI use easier

---

## 24. Keep Memory Ownership Explicit

Every shared API should document:

- who allocates
- who owns returned memory
- which function frees it
- whether buffers may be reused
- failure cleanup rules

Prefer paired APIs such as:

```c
itidy_icon_decode_best(...);
itidy_icon_free(...);
```

Do not rely on frontend-specific cleanup functions.

---

# Part E — Build Strategy

## 25. Keep Both Applications Buildable Throughout

Every migration stage should end with:

```text
iTidy Classic builds
iTidy2 builds
tests pass
```

Never leave the repository in a state where one program is expected to be repaired later.

Recommended rule:

> No shared-code migration is complete until both front ends compile against the new shared API.

---

## 26. Avoid Copying Shared Files Into Two Trees

There must be one authoritative source file.

Do not have:

```text
classic/image_scale.c
itidy2/image_scale.c
```

containing copied versions.

Instead both Makefiles compile:

```text
shared/image/image_scale.c
```

This is essential if the unification is to have long-term value.

---

# Part F — Milestones

## Milestone 1 — Shared Infrastructure

Complete when:

- `shared/` exists
- both build systems can compile shared sources
- no GUI dependencies exist in shared headers

---

## Milestone 2 — Shared Image Engine

Complete when:

- scaler is shared
- palette mapper is shared
- Bayer is shared
- Floyd-Steinberg is shared
- iTidy2 thumbnail output is unchanged

---

## Milestone 3 — Shared Icon Probe

Complete when:

- raw `.info` parsing works
- classic structures can be walked safely
- ToolTypes can be located
- appended enhanced data can be located
- format probe works without v44

---

## Milestone 4 — ColorIcon / GlowIcon Decoder

Complete when:

- ColorIcon normal image decodes
- selected image decodes
- palette works
- transparency works
- compression variants work
- output matches v44 decoding in test cases

---

## Milestone 5 — NewIcons Decoder

Complete when:

- IM1 works
- IM2 works
- multi-ToolType images work
- palette reconstruction works
- transparency works
- malformed input is handled safely

---

## Milestone 6 — Shared Decoder in Both Programs

Complete when:

- iTidy Classic compiles the shared decoder
- iTidy2 compiles the same decoder
- identical test icons produce identical decoded buffers

---

## Milestone 7 — Classic Conversion Engine

Complete when:

- target WB palette is acquired
- screen-aware sizing policy exists
- shared scaler is used
- shared palette mapping/dither is used
- 2-bitplane classic imagery is produced
- metadata is preserved
- backups protect the original

---

## Milestone 8 — First User-Facing Classic Icon Converter

Complete when the user can:

- scan a directory
- identify incompatible/enhanced icons
- preview or inspect conversion information
- convert selected icons
- safely restore originals

UI implementation comes after the engine is proven.

---

## Milestone 9 — Wider Core Unification

Complete progressively as:

- sorting becomes shared
- layout becomes shared
- scanning becomes shared
- backup becomes shared
- common preferences become shared
- default-tool analysis becomes shared

At this stage the applications should increasingly differ primarily in GUI and target-specific integrations.

---

# Part G — Things Not To Do

## 27. Avoid a Full Rewrite

Do not attempt to merge iTidy Classic and iTidy2 into one source tree in a single change.

The risk of regression would be unnecessarily high.

Unify subsystems one at a time.

---

## 28. Do Not Make iTidy Classic Depend on Newer APIs

The shared-core project must not accidentally turn iTidy Classic into a Workbench 3.2 application.

Its purpose is precisely the opposite:

> Keep the Classic frontend viable on old hardware while allowing it to benefit from current development.

---

## 29. Do Not Put GUI Policy Into Shared Code

Shared code must not decide:

- which requester to show
- which menu item is enabled
- which gadget contains an option
- how progress is displayed
- whether ReAction or GadTools is present

Return data/status and allow the frontend to decide.

---

## 30. Do Not Optimise During Initial Extraction

When moving existing working code:

1. preserve behaviour
2. verify it
3. commit the architectural change
4. optimise separately

Combining refactoring and algorithm changes makes regression diagnosis much harder.

---

# Part H — Expected Long-Term Result

The intended end state is not:

```text
old iTidy code
+
new iTidy2 code
```

It is:

```text
                     SHARED iTIDY ENGINE
          -----------------------------------------
          icon parsing / image conversion / layout
          scanning / sorting / backups / utilities
          -----------------------------------------
                    /                      \
                   /                        \
          iTidy Classic                    iTidy2
          ------------                    -------
          GadTools UI                     ReAction UI
          WB2.x-3.1                       WB3.2+
          low-end focus                   richer UI/features
```

A bug fixed in a shared layout routine should fix both programs.

A NewIcons decoder improvement should improve both programs.

A performance improvement in the 68000 image scaler should benefit both programs.

The Classic application should no longer be treated as a frozen legacy branch. It should become the compatibility-focused frontend to the same actively maintained engine.

---

# 31. Immediate Next Action

The first implementation task should be deliberately limited to:

1. introduce `shared/`
2. extract the proven iTidy2 image-processing kernels
3. make iTidy2 compile and behave identically using the new shared code
4. define the neutral decoded-image/icon structures
5. implement the read-only `.info` envelope parser
6. implement the icon probe API
7. stop

Do **not** implement NewIcons or ColorIcon decoding in that first coding pass.

This gives the project its shared architectural foundation and provides a small, testable first milestone.

Once that foundation is proven, proceed to the ColorIcon/GlowIcon decoder, then NewIcons, then the classic icon conversion pipeline.

---

## Summary

The project should move forward as an **incremental convergence**, not a rewrite.

The core rules are:

- one authoritative implementation of shared functionality
- Workbench 2.x / 68000 as the shared compatibility baseline
- frontends own GUI and target-specific OS integration
- shared code owns reusable logic
- no duplicate shared source
- no GUI globals in shared modules
- refactor before adding behaviour
- verify both applications at every stage
- use the icon/image subsystem as the first model for the wider unification

If followed consistently, this should allow iTidy Classic and iTidy2 to remain appropriate for very different Amiga environments while sharing most of the code that actually performs the work.
