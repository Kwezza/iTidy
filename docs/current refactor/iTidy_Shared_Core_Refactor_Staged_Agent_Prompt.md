# iTidy Shared Core Refactor — Staged AI Agent Implementation Prompt

## Purpose

This file is the **persistent implementation prompt and hand-off record** for the staged
iTidy shared-core refactor.

The architectural master plan is:

```text
docs\current refactor\iTidy_Shared_Core_Unification_Plan.md
```

That document is the authoritative design goal.

This file is different: it controls **how the work is carried out in safe batches** and
must be updated by every AI coding agent before it finishes so that the next agent can
see exactly what has already been done.

---

# CRITICAL AGENT RULES

## 1. Read before modifying anything

Before making source changes:

1. Read this entire file.
2. Read:

```text
docs\current refactor\iTidy_Shared_Core_Unification_Plan.md
```

3. Inspect the current Git branch and working tree.
4. Inspect the relevant existing source before proposing or writing changes.
5. Read any project coding/build guidance referenced by the source tree.

Do not assume filenames, APIs, dependencies, or build structure from this prompt if the
repository differs. The repository is the source of truth.

---

## 2. Work on only ONE batch per agent run

Each agent must locate the **first batch whose status is `NOT STARTED` or `IN PROGRESS`**
and work on that batch only.

Do not continue into the next batch simply because time remains.

The purpose of the batching is to keep regression scope small.

At the end of the batch:

- build/test as required;
- update this file;
- clearly mark the batch status;
- record changed files;
- record tests performed and results;
- record unresolved issues;
- record any architectural decisions made;
- identify the exact next batch.

Then stop.

---

## 3. Do not perform unrelated cleanup

During extraction/refactoring:

- do not rewrite working algorithms;
- do not rename unrelated APIs;
- do not reformat large unrelated files;
- do not introduce speculative abstractions;
- do not change GUI behaviour unless the current batch explicitly requires it;
- do not optimise existing algorithms during the move.

The first objective is behavioural equivalence.

---

## 4. Shared-code compatibility baseline

Shared code should target the lowest practical common denominator:

```text
CPU:        68000+
Workbench:  2.x+
Compiler:   VBCC-compatible C89/C99 subset used by the project
Memory:     appropriate for low-end classic Amigas
```

Shared code must not accidentally acquire dependencies on:

- ReAction
- GadTools
- DefIcons
- Workbench 3.2-only APIs
- icon.library v44+
- AGA
- RTG
- CyberGraphX
- FPU
- 68020+ instructions
- GUI objects
- frontend-global window state

A shared algorithm should not normally know whether it was called by iTidy Classic or
iTidy2.

Prefer:

- explicit options structures;
- callbacks;
- small platform/front-end wrappers;
- explicit memory ownership.

Avoid scattering frontend `#ifdef`s throughout shared algorithms.

---

## 5. Branch safety

Before coding, report the checked-out branch in the batch hand-off notes.

For iTidy2 development the expected branch is:

```text
dev-itidy2
```

For later iTidy Classic development the expected branch is:

```text
dev-classic
```

If the current branch is a stable/release branch such as `main` or `v1`, **do not make
production source changes**. Report the problem in this file and stop.

Do not commit, push, merge, rebase, create branches, or modify remote Git state unless the
user has explicitly instructed the coding agent to do so.

---

# HAND-OFF RECORD FORMAT

Every batch has a status block.

Allowed status values:

```text
NOT STARTED
IN PROGRESS
BLOCKED
COMPLETE
```

When beginning a batch, change:

```text
Status: NOT STARTED
```

to:

```text
Status: IN PROGRESS
```

Before finishing, update it to either:

```text
Status: COMPLETE
```

or:

```text
Status: BLOCKED
```

Every completed or blocked batch must contain:

```text
Agent hand-off:
- Date:
- Branch:
- Starting revision/commit if known:
- Summary:
- Files added:
- Files modified:
- Files removed/moved:
- Build performed:
- Tests performed:
- Test result:
- Behavioural/regression result:
- Unresolved issues:
- Important decisions:
- Notes for next agent:
```

Do not erase previous hand-off information.

If another agent later needs to amend a completed batch, append an additional dated note
rather than deleting the historical record.

---

# OVERALL IMPLEMENTATION SEQUENCE

The initial shared-core/icon work is divided into these five coding batches:

```text
Batch 1  Shared image foundation and iTidy2 extraction
Batch 2  Shared raw .info reader and icon probe
Batch 3  Direct ColorIcon / GlowIcon decoder
Batch 4  NewIcons + classic decoder + unified decode API
Batch 5  Bring shared core into iTidy Classic and begin classic conversion support
```

The wider migration of layout, sorting, scanning, backup, preferences and default-tool
analysis comes **after these five batches have proven the architecture**.

---

# BATCH 1 — SHARED IMAGE FOUNDATION

## Status

```text
Status: COMPLETE
```

## Goal

Create the first real shared subsystem by extracting iTidy2's proven generic image
processing code without changing thumbnail behaviour.

This batch establishes the shared source/build pattern that later batches will use.

## Required investigation

Before editing:

1. Trace the current iTidy2 thumbnail/image-processing implementation.
2. Confirm the exact existing files and functions responsible for:
   - indexed image scaling;
   - RGB24 scaling;
   - palette matching/remapping;
   - colour quantisation;
   - Bayer ordered dithering;
   - Floyd-Steinberg dithering.
3. Identify dependencies on:
   - logging;
   - allocation wrappers;
   - progress structures;
   - `iTidy_IconImageData`;
   - thumbnail preferences;
   - ReAction;
   - icon-writing code.

Use the existing source rather than relying only on the previous audit.

## Intended shared structure

Derive the exact implementation from the source, but the target should be close to:

```text
shared/
    image/
        image_types.h
        image_scale.c
        image_scale.h
        image_palette.c
        image_palette.h
        image_dither.c
        image_dither.h
```

Do not create unnecessary one-function modules.

## Required changes

1. Create the shared image source area.
2. Move/extract only genuinely generic image kernels.
3. Remove iTidy2-specific type dependencies from shared headers where practical.
4. Replace direct GUI/progress coupling with:
   - no progress hook where unnecessary; or
   - a neutral optional callback.
5. Decouple logging through a lightweight neutral mechanism if required.
6. Preserve existing allocator/free pairing.
7. Update the iTidy2 build so the shared files compile from their new authoritative
   location.
8. Remove duplicate implementations: there must be one authoritative copy of each shared
   algorithm.

## Explicitly keep in iTidy2

Unless repository inspection proves otherwise, keep these outside the shared layer:

- thumbnail orchestration/policy;
- `itidy_reduce_palette()` if it embeds iTidy2-specific palette choices;
- picture datatype loading;
- IFF thumbnail orchestration;
- DefIcons;
- ReAction progress/UI;
- thumbnail bevel policy;
- modern icon writing;
- preference GUI logic.

## Regression requirement

This is a behavioural refactor.

The success criterion is:

> iTidy2 still builds and existing thumbnail generation produces the same result after
> the shared extraction.

Use existing tests if present.

If no adequate tests exist, create the smallest sensible regression test or comparison
mechanism necessary to validate:

- scaling;
- 4-colour mapping;
- ordered dithering;
- Floyd-Steinberg;
- at least one higher-colour case.

Do not begin icon-format decoding in this batch.

## Stop condition

Stop after:

- shared image code exists;
- iTidy2 consumes it;
- build succeeds;
- regression testing is satisfactory;
- this file has been updated.

---

## Agent hand-off

```text
Agent hand-off:
- Date: 2026-08-14
- Branch: dev-itidy2
- Starting revision/commit if known: 9de481dbad082a8cc27643a3bee1edeff800a05b
- Summary: Extracted iTidy2's generic image kernels (indexed/RGB24 area-average
  scaling, 2x2 prefilter, nearest-colour matching, Median Cut, 6x6x6 cube
  quantiser, Bayer ordered dither, Floyd-Steinberg) into shared/image/. iTidy2
  now compiles those files from the shared location and keeps thin adapters
  for ColorRegister and throttled IFF progress. Thumbnail orchestration,
  itidy_reduce_palette(), DefIcons, bevel, datatype/IFF loading, and GUI
  remain in iTidy2. iTidy2 VBCC build succeeded. Host GCC kernel tests passed.
- Files added:
    shared/image/image_types.h
    shared/image/image_log.c
    shared/image/image_scale.c
    shared/image/image_scale.h
    shared/image/image_palette.c
    shared/image/image_palette.h
    shared/image/image_dither.c
    shared/image/image_dither.h
    src/tests/test_shared_image.c
    src/tests/host_stubs/exec/types.h
- Files modified:
    Makefile
    src/main_gui.c
    src/icon_edit/Image/icon_image_scale.c
    src/icon_edit/palette/palette_mapping.c
    src/icon_edit/palette/palette_quantization.c
    src/icon_edit/palette/palette_quantization.h
    src/icon_edit/palette/palette_dithering.c
    src/icon_edit/palette/palette_dithering.h
- Files removed/moved:
    None. Original iTidy2 .c files remain as adapters so existing call sites
    and function names are unchanged. Algorithm bodies now live only in shared/.
- Build performed:
    make (VBCC +aos68k -cpu=68000) -> Bin/Amiga/iTidy2/iTidy2
    make test-image (GCC host)
- Tests performed:
    Host regression suite src/tests/test_shared_image.c covering:
      RGB24 scale (including independent W/H), indexed scale (all-same fast
      path and mixed average), 2x2 prefilter, 4-colour remap, Bayer offsets
      and ordered dither, Floyd-Steinberg, Median Cut 16->8, cube quantiser,
      dither auto-select (including the 29-colour case).
    Live iTidy2 thumbnail generation was not re-run on the Amiga/WinUAE
    target in this session.
- Test result:
    make test-image: all shared image tests passed.
    make (Amiga): success. Shared image .c files compiled and linked.
    VBCC warning 153 on unused palette_size in image_quantize_rgb24_to_cube
    matches the previous iTidy2 implementation.
- Behavioural/regression result:
    Kernel outputs match the extracted algorithms on host (scale, 4-colour
    mapping, ordered dither, Floyd-Steinberg, higher-colour Median Cut).
    End-to-end iTidy2 thumbnail pixel comparison on target was not performed
    here; adapters preserve original public signatures, palettes are cast
    through layout-compatible iTidy_RGB8 / ColorRegister, and progress still
    goes through itidy_report_progress_throttled().
- Unresolved issues:
    No on-target thumbnail before/after pixel comparison. Next agent or the
    user should generate a representative thumbnail in WinUAE if visual
    confirmation is wanted before Batch 2.
- Important decisions:
    1. Shared RGB type is iTidy_RGB8 {r,g,b}, layout-compatible with
       struct ColorRegister {red,green,blue}; iTidy2 adapters cast.
    2. Progress is an optional iTidy_ImageProgress callback. iTidy2 adapters
       still throttle via DOS ticks and honour cancel_flag.
    3. Logging is an optional callback (image_set_log_fn), bound from
       main_gui.c to writeLog LOG_ICONS. Shared kernels have no writeLog
       or ReAction dependency.
    4. Allocation remains whd_malloc/whd_free.
    5. itidy_reduce_palette() and low-colour/Workbench/harmonised palette
       policy stay in iTidy2.
    6. image_dither_auto_select() keeps the 29-colour Floyd-Steinberg
       special case so existing GlowIcons thumbnail behaviour is unchanged.
    7. The scaler's private find_closest_color_fast() was replaced by the
       single shared Manhattan nearest-colour search (same metric, max
       distance 765). One authoritative copy, as required.
    8. Extra file image_log.c was added for the logging shim. It is not an
       algorithm module.
- Notes for next agent:
    Start Batch 2 (shared raw .info reader and icon probe). Do not change
    shared/image unless a Batch 1 regression is found. Include path -Ishared
    is already in CFLAGS. Consume shared/image later from Classic in Batch 5,
    not now. Host test command: make test-image.
```

---

# BATCH 2 — SHARED RAW `.info` READER AND ICON PROBE

## Status

```text
Status: COMPLETE
```

## Goal

Create a **read-only**, bounds-checked shared parser capable of walking a serialized
Amiga `.info` file and identifying the image representations it contains.

Do not decode NewIcons or ColorIcon image pixels yet.

## Required shared area

Likely:

```text
shared/
    icon/
        icon_types.h
        icon_file.c
        icon_file.h
        icon_probe.c
        icon_probe.h
```

Adjust names only if the repository has a stronger established convention.

## Required capabilities

The raw file layer should be able to:

- validate a plausible classic DiskObject file;
- read big-endian values safely;
- walk serialized classic icon fields without casting the disk buffer directly to
  in-memory Amiga structures;
- locate classic normal image data;
- locate classic selected image data;
- locate DefaultTool;
- locate ToolTypes;
- account for relevant DrawerData structures;
- determine where classic serialized data ends;
- locate appended extension data;
- expose validated offsets/lengths to later decoders.

## Safety

All file-derived sizes/offsets must be checked for:

- truncation;
- overflow;
- out-of-bounds reads;
- impossible dimensions;
- impossible counts;
- allocation overflow.

Malformed `.info` files must return a defined error.

Do not trust serialized pointer fields as actual pointers.

## Probe API

Create an inexpensive format-identification API able to report information such as:

```text
classic image present
NewIcons markers/data present
ColorIcon/GlowIcon extension present
later unsupported enhanced format detected where practical
normal image present
selected image present
```

The exact type/API should remain GUI-neutral.

## Testing

Create or use representative `.info` samples.

Verify at minimum:

- ordinary classic icon;
- NewIcons-containing icon;
- ColorIcon/GlowIcon-containing icon;
- icon with selected imagery;
- malformed/truncated input.

The probe must not modify icons.

## Stop condition

Stop after:

- safe `.info` envelope parsing works;
- format probe works;
- tests pass;
- no enhanced image decoding has been implemented;
- this file has been updated.

---

## Agent hand-off

```text
Agent hand-off:
- Date: 2026-08-14
- Branch: dev-itidy2
- Starting revision/commit if known: 3f81f8756b614d801d5c338d1fc38e5a06e25577
- Summary: Added a read-only, allocation-free shared .info envelope parser
  and format probe under shared/icon/. The parser walks serialized DiskObject
  fields byte-by-byte (no struct casts), treats on-disk APTRs as booleans,
  locates classic images / DefaultTool / ToolTypes / DrawerData / DrawerData2,
  and reports where classic data ends so later decoders can consume the
  extension. The probe flags classic, NewIcons, ColorIcon, GlowIcon (256-colour
  FACE heuristic), PNG, and OS4 ARGB without decoding pixels. iTidy2 still
  uses its existing icon.library detection path; shared icon sources are
  compiled into the VBCC binary but not wired into GUI behaviour.
- Files added:
    shared/icon/icon_types.h
    shared/icon/icon_file.h
    shared/icon/icon_file.c
    shared/icon/icon_probe.h
    shared/icon/icon_probe.c
    src/tests/test_shared_icon.c
- Files modified:
    Makefile
    docs/current refactor/iTidy_Shared_Core_Refactor_Staged_Agent_Prompt.md
- Files removed/moved:
    None
- Build performed:
    make (VBCC +aos68k -cpu=68000) -> Bin/Amiga/iTidy2/iTidy2
    make test-icon (GCC host)
    make test-image (GCC host, Batch 1 regression)
- Tests performed:
    Host suite src/tests/test_shared_icon.c covering:
      big-endian readers and planar size;
      malformed/truncated/bad-magic/bad-version/zero-width/depth-9/
        bad ToolTypes count/unterminated text;
      classic 16x16x1 (buffer not modified; leftover pointers not used
        as offsets);
      drawer + selected image + DrawerData2;
      DefaultTool and ToolTypes access;
      NewIcons marker + IM1=/IM2=;
      FORM ICON 8-colour vs 256-colour Glow heuristic, 1 vs 2 IMAG;
      PNG-only, dual classic+PNG, FORM ARGB unsupported;
      live Bin/Amiga.info and Bin/Amiga/iTidy2.info (classic_end == EOF).
    No NewIcons or ColorIcon pixel decoding was implemented or tested.
- Test result:
    make test-icon: all shared icon tests passed.
    make test-image: all shared image tests passed.
    make (Amiga): success. shared/icon/*.c compiled and linked with no
    VBCC warnings.
- Behavioural/regression result:
    iTidy2 GUI/icon.library behaviour unchanged: existing format.c /
    reader.c paths were not rewired. Shared parser is additive. Probe
    does not write the input buffer.
- Unresolved issues:
    GlowIcon vs ColorIcon is a FACE max-palette heuristic (>= 255 =>
    glow). Confirm against real GlowIcon samples in Batch 3.
    iTidy2 live format detection still uses GetDiskObject / isOS35IconFormat.
    Parser was not executed on the Amiga/WinUAE target; host tests plus
    VBCC compile are the Batch 2 evidence.
- Important decisions:
    1. Core API is buffer-based and allocation-free. Callers own the
       bytes. No platform.h / whd_malloc / DOS in shared/icon.
    2. On-disk APTRs are booleans only. Image/text blobs are sequential
       after the 78-byte DiskObject, matching IconFormats.txt and verified
       against Bin/Amiga.info and Bin/Amiga/iTidy2.info.
    3. DrawerData2 (6 bytes) is consumed when DrawerData is present and
       ga_UserData lo-byte == 1, unless the remainder already looks like
       FORM/PNG (defensive).
    4. Probe walks FORM ICON chunks only far enough to see FACE / count
       IMAG / notice ARGB. Pixel and palette payloads are not decoded.
    5. PNG-only files are accepted by icon_probe_buffer() as
       has_png+has_unsupported; icon_file_parse() still returns BAD_MAGIC.
    6. iTidy2 existing detectors were left in place. Wiring the shared
       probe into the front end is later work, not this batch.
    7. Layout source of truth: support files/IconFormats.txt plus two
       real drawer icons. Gadget width is at file 0x0C (not 0x08).
- Notes for next agent:
    Start Batch 3 (direct ColorIcon / GlowIcon decoder). Use
    file->extension_offset / extension_size as the FORM ICON start.
    icon_file_read_u8/u16/u32 and icon_file_range_ok() are the safe
    readers to reuse. Do not change shared/image or the envelope parser
    unless a Batch 2 regression is found. Host tests: make test-icon
    and make test-image. Include path -Ishared is already in CFLAGS.
    Do not implement NewIcons decoding in Batch 3.
```

---

# BATCH 3 — COLORICON / GLOWICON DIRECT DECODER

## Status

```text
Status: COMPLETE
```

## Goal

Implement direct decoding of OS3.5 ColorIcon / GlowIcon enhanced imagery using the shared
raw `.info` parser, without requiring icon.library v44+.

Output must use neutral shared image structures.

## Before coding

Review the research/design information available in the project for direct ColorIcon /
GlowIcon parsing.

Verify all assumptions against:

- existing source;
- stored test icons;
- authoritative format notes already added to the project, if present.

Do not invent binary structure details.

## Required functionality

Implement progressively:

1. `FORM ICON` detection.
2. Safe chunk walking.
3. `FACE` parsing.
4. `IMAG` parsing.
5. Raw palette decoding.
6. Raw indexed pixel decoding.
7. Transparency.
8. Normal image.
9. Selected image.
10. Selected-image palette inheritance where the format requires it.
11. Image compression/RLE.
12. Palette compression/RLE.
13. Safe skipping of unknown chunks.
14. Odd-sized chunk/padding handling as required by the actual format.

## Output boundary

The decoder should produce a neutral representation conceptually equivalent to:

```text
width
height
chunky indexed pixels
RGB8 palette
palette count
transparent index
optional selected image
source format metadata
```

It must not:

- resize;
- dither;
- map to Workbench pens;
- apply PAL/NTSC aspect correction;
- write `.info` files;
- show GUI.

## Validation in iTidy2

Where possible use iTidy2's existing icon.library v44+ decoding path as an independent
validation oracle.

Compare:

- width;
- height;
- palette count;
- palette RGB data;
- normal pixel indexes;
- selected pixel indexes;
- transparency.

Aim for byte-identical decoded results where the APIs expose equivalent data.

## Tests

Add representative cases for:

- raw image/palette;
- compressed image;
- compressed palette;
- both compressed;
- selected image;
- shared/inherited palette;
- transparency;
- unknown chunk;
- malformed/truncated extension.

## Stop condition

Stop after the ColorIcon/GlowIcon decoder is independently tested and validated.

Do not implement NewIcons in the same batch.

---

## Agent hand-off

```text
Agent hand-off:
- Date: 2026-08-14
- Branch: dev-itidy2
- Starting revision/commit if known: 8850e0af0db519b376ceeba81bc27c2b43d86aa5
- Summary: Added a direct ColorIcon/GlowIcon decoder under shared/icon/
  that walks IFF FORM ICON using the Batch 2 envelope offsets. One decoder
  covers both formats: FACE + IMAG, raw and PackBits-bitstream RLE for
  image and palette, transparency, selected image, selected palette
  inheritance, unknown-chunk skip, odd-size padding, and FACE-after-IMAG
  reorder. Output is iTidy_DecodedIcon (chunky indexes + RGB8 palette).
  No resize/dither/pen mapping/GUI. NewIcons not implemented. iTidy2 GUI
  still uses icon.library; shared decoder is compiled in but not wired.
- Files added:
    shared/icon/icon_coloricon.h
    shared/icon/icon_coloricon.c
    src/tests/test_shared_coloricon.c
- Files modified:
    shared/icon/icon_types.h
    shared/icon/icon_file.c
    Makefile
    docs/current refactor/iTidy_Shared_Core_Refactor_Staged_Agent_Prompt.md
- Files removed/moved:
    None
- Build performed:
    make (VBCC +aos68k -cpu=68000) -> Bin/Amiga/iTidy2/iTidy2
    make test-coloricon (GCC host)
    make test-icon (GCC host, Batch 2 regression)
    make test-image (GCC host, Batch 1 regression)
- Tests performed:
    Host suite src/tests/test_shared_coloricon.c covering:
      raw image + raw palette + transparency;
      RLE image (including 0x80 NOP);
      RLE palette;
      both compressed (2-bit image samples);
      selected image with inherited palette + frameless FACE flag;
      unknown odd-sized chunk and FACE after IMAG;
      256-colour FACE heuristic => GLOWICON;
      no extension, FORM ARGB, FACE without IMAG, truncated IMAG,
      RLE dest overrun, NULL out;
      input buffer not modified.
    icon.library v44+ oracle comparison was not run (no Amiga/WinUAE
    ColorIcon samples in-tree; decoder not wired to iTidy2 extract path).
- Test result:
    make test-coloricon: all shared ColorIcon tests passed.
    make test-icon: all shared icon tests passed.
    make test-image: all shared image tests passed.
    make (Amiga): success. icon_coloricon.c compiled and linked with no
    VBCC warnings.
- Behavioural/regression result:
    iTidy2 GUI/icon.library path unchanged. Shared decoder is additive.
    Batch 1/2 host tests still pass.
- Unresolved issues:
    No on-target comparison against IconControlA() pixel/palette dumps.
    Glow vs ColorIcon remains the FACE max-palette>=255 heuristic.
    Permanent tests/icons/ corpus is deferred to Batch 4.
- Important decisions:
    1. Neutral iTidy_IndexedImage / iTidy_DecodedIcon live in
       icon_types.h (reused by later classic/NewIcons decoders).
       RGB is shared iTidy_RGB8 from image_types.h.
    2. Allocation uses whd_malloc/whd_free via platform.h, matching
       shared/image. Callers must icon_decoded_free().
    3. RLE is a continuous MSB-first bitstream: 8-bit PackBits control
       then im_Depth-bit (image) or 8-bit (palette) samples, per
       IconFormats.txt. Uncompressed image is one byte per pixel.
    4. First IMAG must carry a palette; later IMAG without bit1 inherits
       by copying the first palette (independent ownership).
    5. FACE is located in a first pass so IMAG may precede FACE.
       Unknown chunks and ARGB are skipped; ARGB-only FORM ICON returns
       UNSUPPORTED rather than corrupt.
    6. Decoder is not wired into iTidy2 GUI. Oracle validation against
       icon.library is left for a later Amiga run.
    7. New error codes: UNSUPPORTED=10, NO_DATA=11, ALLOC=12.
- Notes for next agent:
    Start Batch 4 (NewIcons + classic planar decoder + unified decode
    API). Do not change the ColorIcon decoder unless a regression is
    found. Host tests: make test-coloricon, make test-icon, make
    test-image. Include path -Ishared is already in CFLAGS. Respect
    NewIcons ToolType bitstream segmentation; do not concatenate IM1=
    strings blindly. Do not implement classic conversion UI yet.
```

---

# BATCH 4 — NEWICONS + CLASSIC DECODER + UNIFIED API

## Status

```text
Status: COMPLETE
```

## Goal

Complete the shared **input** side by adding NewIcons decoding, classic planar decoding,
and a unified "decode best available image" API.

## NewIcons decoder

Likely shared module:

```text
shared/icon/icon_newicons.c
shared/icon/icon_newicons.h
```

Required functionality:

- recognise relevant NewIcons markers;
- decode `IM1=` normal imagery;
- decode `IM2=` selected imagery;
- reconstruct palette;
- reconstruct chunky indexed pixels;
- transparency handling;
- safe malformed-stream detection;
- conservative dimension/palette limits appropriate to low-memory classic systems.

### Important boundary rule

Respect the actual NewIcons ToolType bitstream segmentation rules.

Do not blindly concatenate ToolType strings if the format resets decoder bit state at
ToolType boundaries.

Create explicit regression cases for line/ToolType boundaries.

## Classic planar decoder

Likely:

```text
shared/icon/icon_classic.c
shared/icon/icon_classic.h
```

Convert serialized classic planar icon image data into the same neutral chunky-indexed
representation.

Represent clearly that classic icon images use Workbench pens rather than an embedded RGB
palette.

## Unified decode API

Provide a shared API that allows callers to:

- probe;
- explicitly decode a requested representation;
- decode the best available representation;
- free all decoder-owned memory.

Suggested preference, subject to verified format availability:

```text
ColorIcon / GlowIcon
        ↓
NewIcons
        ↓
classic planar
```

Unsupported later formats should return a specific "unsupported" status rather than
"corrupt".

## Permanent regression corpus

By the end of this batch there should be a reusable set of test icons organised
approximately as:

```text
tests/icons/
    classic/
    newicons/
    coloricons/
    glowicons/
    malformed/
```

Where practical record expected:

- width;
- height;
- palette count;
- transparency;
- CRC/checksum of decoded pixels;
- CRC/checksum of palette;
- selected-image presence.

## Stop condition

Stop once the common input side is complete and stable.

Do not yet implement the user-facing classic conversion UI.

---

## Agent hand-off

```text
Agent hand-off:
- Date: 2026-08-14
- Branch: dev-itidy2
- Starting revision/commit if known: 804e925 Direct ColorIcon/GlowIcon decoder
- Summary: Completed the shared input side. Added a NewIcons IM1=/IM2=
  decoder (7-bit ASCII + RLE zeros, transparency, selected image,
  conservative 93x93 / 256-colour limits), a classic planar decoder
  (chunky Workbench-pen indexes via PlanePick/PlaneOnOff, no embedded
  RGB palette), and a unified icon_decode() / icon_decode_buffer() API
  with BEST preference ColorIcon/GlowIcon then NewIcons then classic.
  PNG-only and ARGB-only payloads return UNSUPPORTED rather than
  corrupt. Wrote a reusable tests/icons/ corpus with .info samples and
  .expected CRC sidecars. iTidy2 GUI is still not wired to the shared
  decoders. No conversion UI.
- Files added:
    shared/icon/icon_newicons.c
    shared/icon/icon_newicons.h
    shared/icon/icon_classic.c
    shared/icon/icon_classic.h
    shared/icon/icon_decode.c
    shared/icon/icon_decode.h
    src/tests/test_shared_decode.c
    tests/icons/classic/
    tests/icons/newicons/
    tests/icons/coloricons/
    tests/icons/glowicons/
    tests/icons/malformed/
- Files modified:
    shared/icon/icon_types.h
    shared/icon/icon_file.c
    Makefile
    docs/current refactor/iTidy_Shared_Core_Refactor_Staged_Agent_Prompt.md
- Files removed/moved:
    None
- Build performed:
    make (VBCC +aos68k -cpu=68000) -> Bin/Amiga/iTidy2/iTidy2
    make test-decode (GCC host)
    make test-coloricon / test-icon / test-image (host regression)
- Tests performed:
    Host suite src/tests/test_shared_decode.c covering:
      classic 8x2x1 planar pixels; selected image + PlaneOnOff pens;
      classic with no image => NO_DATA;
      NewIcons 2x2 4-colour + transparency index 0;
      NewIcons 4x4 8-colour normal+selected;
      NewIcons 8x8 16-colour split across short ToolType lines
      (sample bits continue across IM1= wraps; prefixes not concatenated);
      NewIcons RLE zero groups (0xD1);
      truncated IM1= header, zero dimensions, marker without IM1=;
      BEST prefers ColorIcon over NewIcons over classic;
      explicit REQ_NEWICONS / REQ_CLASSIC;
      PNG-only => UNSUPPORTED; FORM ARGB with classic fallback;
      ColorIcon and GlowIcon corpus samples;
      reload of written tests/icons/*.info files;
      input buffer not modified.
    Batch 1-3 host suites still pass.
- Test result:
    make test-decode: all shared decode tests passed.
    make test-icon / test-coloricon / test-image: all passed.
    make (Amiga): success. New shared icon .c files compiled and linked
    with no VBCC warnings.
- Behavioural/regression result:
    iTidy2 GUI/icon.library path unchanged. Shared decoders are additive.
    Batch 1-3 host tests still pass.
- Unresolved issues:
    No on-target comparison against NewIcons.library or icon.library
    v44+ for live NewIcons/ColorIcon samples.
    Glow vs ColorIcon remains the FACE max-palette>=255 heuristic.
    Shared decode API is not wired into the iTidy2 GUI.
    NewIcons colours >256 (rare stored-257 case in IconFormats.txt)
    are rejected as BAD_COUNT.
- Important decisions:
    1. Unified API is icon_decode(file, request, out) and
       icon_decode_buffer(), with ITIDY_ICON_REQ_BEST/COLORICON/
       NEWICONS/CLASSIC. icon_decoded_free() remains the free path.
    2. BEST tries ColorIcon, then NewIcons, then classic. ColorIcon
       NO_DATA may fall through; other ColorIcon errors do not.
       PNG-only is UNSUPPORTED (caught before DiskObject parse).
    3. Classic output: source_format CLASSIC, palette NULL,
       palette_count 0, indexes are Workbench pens after PlanePick/
       PlaneOnOff. Callers supply the screen palette later (Batch 5).
    4. NewIcons: first five payload bytes of IM1=/IM2= are raw header;
       remaining bytes are 7-bit ASCII (0x20-0x6F / 0xA1-0xD0) and
       0xD1-0xFF zero RLE. bpp = ceil(log2(ncolors)), 0 if one colour.
       'B' => transparent index 0, otherwise none.
    5. NewIcons ToolType boundary: do not concatenate "IM1=" prefixes
       or the 5-byte header. Already-unpacked sample bits are kept
       across lines so a palette/pixel field can finish after a 7-bit
       character wrap. GetBits is atomic (does not consume on short
       read). Pad flush is typically end-of-image only.
    6. Limits: NewIcons width/height 1..93, colours 1..256.
    7. ITIDY_ICON_SRC_NEWICONS=3, ITIDY_ICON_SRC_CLASSIC=4.
    8. Corpus lives under tests/icons/ with .expected CRC sidecars.
       Host test make test-decode regenerates/overwrites those files.
- Notes for next agent:
    Start Batch 5 only on branch dev-classic, after Batches 1-4.
    Do not implement conversion UI in Batch 5 until the shared modules
    compile in Classic. Do not change the Batch 4 decoders unless a
    regression is found. Host tests: make test-decode, test-coloricon,
    test-icon, test-image. Include path -Ishared is already in CFLAGS.
    iTidy2 still uses icon.library; wiring shared decode into a GUI
    is later work.
```

---

# BATCH 5 — CONSUME SHARED CORE FROM iTIDY CLASSIC

## Status

```text
Status: NOT STARTED
```

## Goal

Bring the already-proven shared image and icon code into **iTidy Classic**, targeting
Workbench 2.x / 3.0 / 3.1 and 68000.

This batch begins only after Batches 1-4 are complete.

## Branch requirement

Expected development branch:

```text
dev-classic
```

Do not modify stable `v1` directly.

## Required work

1. Make iTidy Classic's build compile the same authoritative `shared/image` and
   `shared/icon` files.
2. Do not copy the shared files into the Classic tree.
3. Resolve only genuine portability/build differences.
4. Verify no accidental dependency on:
   - ReAction;
   - Workbench 3.2;
   - icon.library v44+;
   - datatypes;
   - 68020+;
   - FPU.
5. Run decoder tests using the same icon corpus.
6. Verify representative formats on the Classic target environment.

## Then begin the classic conversion engine

Once the shared modules compile and decode correctly, add the minimum frontend-specific
conversion infrastructure:

### Target palette

Read the actual current Workbench colours/pens rather than using a hardcoded "typical"
Workbench palette.

### Screen geometry policy

Add a separate policy layer which chooses destination dimensions based on:

- PAL/NTSC;
- HiRes/LoRes;
- interlace where relevant;
- current screen;
- optional explicit override.

Do not put Workbench pixel-aspect policy into the generic scaler.

### Shared conversion path

Use:

```text
shared icon decoder
        ↓
shared scaler
        ↓
shared palette mapping / dithering
        ↓
classic planar output
```

### Classic planar writer

Implement classic 1/2/etc. bitplane writing as required, with the initial conversion
target expected to be a simple four-colour / two-bitplane Workbench icon.

The writer may remain Classic-specific initially if iTidy2 does not yet need classic
output.

### Metadata preservation and safety

Preserve all practical non-image metadata, including where present:

- icon type;
- DefaultTool;
- ToolTypes;
- StackSize;
- icon position;
- drawer geometry;
- relevant gadget flags;
- ToolWindow.

Never destroy the only copy of modern enhanced imagery.

Use safe temporary-file and backup behaviour before replacing an original `.info`.

## GUI

Do **not** build an elaborate conversion GUI until the conversion engine is proven.

A minimal test harness/entry point is preferable during the first implementation.

## Stop condition

Stop once:

- Classic consumes the shared code;
- common regression tests succeed;
- the basic classic conversion engine is technically proven;
- this file has been updated.

The user-facing GUI should be planned as a subsequent batch.

---

## Agent hand-off

_Not yet completed._

---

# AFTER BATCH 5 — WIDER CORE UNIFICATION

Do not begin this section until the first five batches have proven that the shared-core
approach works in both products.

The likely next migration order is:

```text
1. generic string/path helpers
2. sorting
3. layout calculations
4. directory scanning
5. backup engine
6. shared preference/data models
7. default-tool analysis
8. other non-GUI utilities
```

Each subsystem should receive its own staged audit/refactor.

The same rule applies:

> One authoritative shared implementation, with GUI/front-end-specific code remaining in
> iTidy Classic and iTidy2.

---

# CURRENT PROJECT STATE / NEXT AGENT ENTRY POINT

This section must always be updated by the agent that finishes a batch.

## Last completed batch

```text
Batch 4 — NewIcons + Classic Decoder + Unified API
```

## Next batch to execute

```text
Batch 5 — Consume Shared Core from iTidy Classic
```

## Known blockers

```text
None for starting Batch 5 on branch dev-classic. Shared input decode
is complete on host tests. icon.library / NewIcons.library oracle
comparison was not run on Amiga. Shared decoders are compiled into
iTidy2 but not wired into the GUI. Do not start Batch 5 on
dev-itidy2 / main / v1.
```

## Global hand-off notes

```text
The master architectural plan is:
docs\current refactor\iTidy_Shared_Core_Unification_Plan.md

This file is the persistent implementation/runbook.
Every agent must update it before stopping.
```

---

# FINAL AGENT CHECKLIST

Before ending your run, verify all applicable items:

- [ ] I read the master unification plan.
- [ ] I checked the Git branch.
- [ ] I inspected the current source before editing.
- [ ] I worked on only the current batch.
- [ ] I did not perform unrelated cleanup.
- [ ] Shared code remains 68000 / WB2.x-conscious.
- [ ] I built the affected target.
- [ ] I ran the required regression tests.
- [ ] I recorded failures honestly.
- [ ] I updated this batch's status.
- [ ] I filled in the Agent hand-off section.
- [ ] I updated "Last completed batch".
- [ ] I updated "Next batch to execute".
- [ ] I updated "Known blockers".
- [ ] I did not start the next batch.
