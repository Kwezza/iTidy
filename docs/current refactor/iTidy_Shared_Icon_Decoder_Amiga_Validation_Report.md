# iTidy Shared Icon Decoder — Amiga Real-Corpus Validation Report

- **Date:** 2026-08-14
- **Branch:** `dev-itidy2`
- **Validator:** `Bin/Amiga/iTidy2/iTidyIconTest`
- **Corpus:** `TestsIcons/test-icons` (source `.info` files not altered)
- **Logs reviewed:**
  - `output.txt` — decode-only run (`BEST`, oracle off)
  - `output.compare.txt` — first COMPARE (before NewIcons palette-line flush)
  - `output.compare2.txt` — second COMPARE (after NewIcons palette-line flush; index-level only)
  - `output.compare3.txt` — third COMPARE (RGB-composite COMPARE enabled; NewIcons still mismatched)
  - `output.compare4.txt` — fourth COMPARE (after ordered-RLE + per-ToolType flush; **full MATCH**)

This report records Amiga Shell validation of the shared icon decoder against `TestsIcons/test-icons`. Batch 5 was not started on this branch. NewIcons is now oracle-validated; Batch 5 may proceed on `dev-classic`.

---

## 1. Executive summary

All Amiga runs decoded **29/29** `.info` files with **0 parse/decode failures**.

**Current oracle scoreboard** (`output.compare4.txt`, after ordered-RLE + every-ToolType flush):

| Oracle result | Count | What it covers |
|---|---|---|
| **MATCH** | 12 | All 6 ColorIcon + all 4 GlowIcon + both NewIcons |
| **SKIPPED** | 17 | All classic planar files (Workbench pens, by design) |
| **MISMATCH** | 0 | — |
| **RGB MATCH** | 12 | Same 12 palette-mapped files |
| **RGB MISMATCH** | 0 | — |
| **Invalid palette indexes** | 0 | — |

**ColorIcon and GlowIcon decoding is validated against icon.library** at both index and RGB-composite level. Unchanged across every COMPARE run. Those 10 files remain the control that RGB COMPARE itself is working.

**Classic decoding** completed on-target with consistent dimensions, Workbench-pen output, and CRCs. It was not pixel-compared to icon.library (different representation).

**NewIcons is now validated.** Two successive decoder defects were identified and fixed:

1. Palette-line flush only (§11) — necessary but **not sufficient**. `output.compare2.txt` / `output.compare3.txt` still mismatched both files; first RGB failures were too early to be caused solely by a full pixel-line wrap (`Apps` pixel 207, `0016` pixel 23).
2. Ordered RLE + flush at **every** physical `IM1=`/`IM2=` ToolType, with pixels starting on the line after palette completion (§13, research: `docs/current refactor/deep-research-report-newIcons.md`). `output.compare4.txt` is a full MATCH: both NewIcons files agree with icon.library on **RGB and raw indexes/palettes**.

Canonical post-fix CRCs (host and Amiga agree):

| File | Palette | Normal pixels | Selected pixels |
|---|---|---|---|
| `Apps.info` | `C7DCC148` | `22CF0A9F` | `70EA1B88` |
| `0016.info` | `A5BF3CAD` | `96DB489A` | `372E4ECC` |

`0016.info` palette CRC changed from the known-bad `DAD2D1EE` because first-line RLE sat inside the palette stream; the old zero-priority queue was corrupting palette bytes, not only pixels.

**Recommendation:** keep the ordered-RLE + per-ToolType flush. Do not revert to palette-line-only flush or to carrying bits across ToolTypes. ColorIcon/GlowIcon/classic code should remain untouched. Batch 5 may start on **`dev-classic`** only. Do not start Batch 5 on `dev-itidy2` / `main` / `v1`.

---

## 2. How the runs were produced

```text
Bin/Amiga/iTidy2/iTidyIconTest TestsIcons/test-icons
Bin/Amiga/iTidy2/iTidyIconTest TestsIcons/test-icons COMPARE
```

COMPARE was run four times: `output.compare.txt` (pre-flush), `output.compare2.txt` (palette-line flush, index-level), `output.compare3.txt` (palette-line flush, RGB-composite), and `output.compare4.txt` (ordered-RLE + every-ToolType flush; full MATCH).

- Probe: `icon_probe_buffer()`
- Decode: `icon_decode_buffer(..., ITIDY_ICON_REQ_BEST, ...)`
  - Preference: ColorIcon/GlowIcon → NewIcons → classic planar
- Checksums: IEEE CRC32 of chunky indexes and RGB palettes (classic palettes printed `WORKBENCH_PENS` / `N/A`)
- Oracle (COMPARE only): `GetIconTagList(ICONGETA_RemapIcon=FALSE)` + `IconControlA()` inside the CLI. Shared modules are not linked to icon.library. Classic results are skipped (Workbench pens vs palette-mapped). From `output.compare3.txt`, palette-mapped files also compare RGB composites (`shared_palette[shared_pixels[p]]` vs `oracle_palette[oracle_pixels[p]]`), with semantic transparency. The iTidy2 `itidy_icon_image_extract()` path was not used (it can expand palettes and `LayoutIconA` classic icons).

---

## 3. Corpus notes

The validator recursively scanned every `.info` under `TestsIcons/test-icons`. Directory names (`alpha`, `ColorIcons`, `MUI`, `Newicons`, `OS1.3`, `OS3`, `OS4`) are organisational hints from the historical collection. **Detected format is whatever is in the file.**

Extra files beyond the original 21-in-folders list:

- Seven category-drawer icons at the corpus root (`alpha.info`, `ColorIcons.info`, `MUI.info`, `Newicons.info`, `OS1.3.info`, `OS3.info`, `OS4.info`)
- `AQUARIUMBACKGROUND.HAM.info`

That is **21 + 7 + 1 = 29** files. All were processed.

A host-side binary scan of the corpus found:

- `FORM ICON` in the ColorIcon/GlowIcon files listed below
- NewIcons `IM1=`/`IM2=` only in `Newicons/Apps.info` and `Newicons/0016.info`
- **No PNG signature and no `FORM ARGB` in any file**

So **0 expected-unsupported** is correct. The OS4 folder in this collection is not a PNG/ARGB set.

---

## 4. Decode-only run (`output.txt`)

```text
Files scanned:              29
Decoded successfully:       29
Expected unsupported:       0
Failures:                   0

Formats decoded:
  Classic:                  17
  NewIcons:                 2
  ColorIcon:                6
  GlowIcon:                 4

RESULT: PASS
```

No file returned `FAIL` or `EXPECTED UNSUPPORTED`. Every probe result was `OK`.

### 4.1 Format vs folder name

| Location | What the decoder found |
|---|---|
| `MUI/` | Classic only (no NewIcons/ColorIcon payload) |
| `alpha/` | GlowIcon (classic fallback also present) |
| `Newicons/` | NewIcons (classic fallback also present) |
| `ColorIcons/` | Mix of ColorIcon and GlowIcon |
| `OS1.3/` | Classic |
| `OS3/` | ColorIcon except `Disk.info` (classic) |
| `OS4/` | Classic only — no PNG/ARGB in these files |
| Root `*.info` drawers | Same classic 57×21 drawer icon |
| `AQUARIUMBACKGROUND.HAM.info` | Classic 64×40, no selected image |

This is expected. Folder name is not a test oracle.

### 4.2 Category-drawer icons are one file copied seven times

`alpha.info`, `ColorIcons.info`, `MUI.info`, `Newicons.info`, `OS1.3.info`, `OS3.info`, and `OS4.info` all decoded as:

- Classic 57×21, selected present
- Normal pixel CRC `4DEE3FEF`
- Selected pixel CRC `3472F656`

They are the same Workbench drawer icon used as folder icons for the test collection, not seven independent images.

### 4.3 Probe `selected` vs decode `selected` on `ColorIcons/disk.info`

```text
PROBE:    selected: yes
DECODE:   SELECTED present: no
COMPARE:  selected_match: yes  (lib has_image2=no)
```

The classic fallback has a selected image. The ColorIcon extension has a single `IMAG`. `BEST` uses ColorIcon, so the decoded result has no selected image. icon.library agrees (`HasRealImage2=no`). Probe reports representations **in the file**; decode reports what `BEST` produced. Not a failure.

### 4.4 Glow vs ColorIcon classification

Glow is still the FACE-field heuristic `>= 255` (see Batch 2/3 notes). On these files that FACE word matches **max palette bytes minus 1** (`fc_MaxPalBytes` in `support files/IconFormats.txt`), not “256 colour entries minus 1”.

Empirically:

- ColorIcon files: 11–64 colours (FACE field 28–190)
- Glow-classified files: 113–247 colours (FACE field 338–740)

`ColorIcons/screen.info` (113 colours) is the borderline Glow classification. COMPARE later showed icon.library still matched its pixels, so the label did not imply a bad decode.

This heuristic remains a known limitation. It is not a pixel-decode failure.

---

## 5. First COMPARE run (`output.compare.txt`, before flush)

```text
Files scanned:              29
Decoded successfully:       29
Failures:                   0

Oracle (icon.library v44+):
  Attempts:                 29
  Matches:                  10
  Mismatches:               2
  Skipped:                  17

RESULT: PASS
```

Decode-side CRCs and dimensions were unchanged from `output.txt`. The new information is the oracle column.

### 5.1 MATCH — ColorIcon and GlowIcon (10/10)

Every palette-mapped ColorIcon/GlowIcon file matched icon.library on:

- width / height
- palette count and RGB values
- pixel indexes
- transparency (all `0` here)
- selected-image presence and contents

| File | Shared source | Size | Palettes (N / S) | Oracle |
|---|---|---|---|---|
| `alpha/System.info` | GLOWICON | 64×64 | 228 / 247 | MATCH |
| `alpha/Games.info` | GLOWICON | 64×64 | 231 / 241 | MATCH |
| `ColorIcons/screen.info` | GLOWICON | 48×48 | 113 / 106 | MATCH |
| `ColorIcons/AmigaMail.info` | GLOWICON | 48×48 | 221 / 157 | MATCH |
| `ColorIcons/System_drawer.info` | COLORICON | 62×28 | 64 / 64 | MATCH |
| `ColorIcons/screen2.info` | COLORICON | 46×46 | 16 / 17 | MATCH |
| `ColorIcons/disk.info` | COLORICON | 31×33 | 11 / none | MATCH |
| `OS3/Orange.info` | COLORICON | 46×46 | 13 / 15 | MATCH |
| `OS3/green.info` | COLORICON | 46×46 | 14 / 15 | MATCH |
| `OS3/Blue.info` | COLORICON | 42×42 | 21 / 21 | MATCH |

This is independent confirmation of:

- `FORM ICON` / FACE / IMAG walking
- raw and RLE image/palette
- transparency
- selected IMAG and palette inheritance
- unified `BEST` preferring ColorIcon-family data over classic fallback

`output.compare3.txt` also confirmed **RGB-composite MATCH** on all ten of these files.

### 5.2 SKIPPED — classic (17/17, by design)

Oracle reason: `CLASSIC uses Workbench pens; not compared`.

Shared classic output is chunky Workbench-pen indexes with `palette_count == 0`. icon.library palette-mapped extraction (and `LayoutIconA`) is a different representation. Skipping avoids false mismatches.

Classic files still decoded successfully (see appendix).

### 5.3 MISMATCH — NewIcons (2/2)

| File | Size | Shared palettes | Oracle |
|---|---|---|---|
| `Newicons/Apps.info` | 36×40 | 8 / 9, trans 0 | size yes, **palette yes**, trans yes, **pixels no**, **selected no** |
| `Newicons/0016.info` | 42×42 | 32 / 32, trans 0 | size yes, trans yes, lib 32 colours, **palette no**, **pixels no**, **selected no** |

Header decode is correct (dimensions, colour count, transparency `'B'` → index 0). The packed payload after the 5-byte NewIcons header is not equivalent to icon.library.

---

## 6. NewIcons mismatch — investigation

### 6.1 Documented line boundary vs current decoder

`support files/IconFormats.txt`:

```text
The encoding for images and palette stops at the string boundary (127 bytes)
with buffer flush (and adding pad bits) and is restarted with next line.
```

The shared decoder (`shared/icon/icon_newicons.c`) **keeps leftover bits across `IM1=` / `IM2=` ToolType lines** and only pads at the end of the image. Host tests round-trip an encoder that uses the same rule, so `make test-decode` does not catch this.

Batch 4 recorded that carry-across-lines choice as an explicit decision. Real NewIcons files and icon.library disagree with it.

### 6.2 What is actually on the first `IM1=` line

Proper DiskObject ToolType walk of the two real files:

| File | IM1[0] | Palette bits needed | Bits on line 0 (incl. RLE) | Leftover after palette |
|---|---|---|---|---|
| `Apps.info` | strlen 36 (27 encoded bytes after header) | 192 (8×3×8) | **224** | **32 bits** |
| `0016.info` | strlen 119 (110 encoded bytes after header) | 768 (32×3×8) | **784** | **16 bits** |

Later `IM1=` lines are 127-byte (123-byte payload) pixel lines, last line shorter.

The first line is palette-only, then the encoder flushes (pad bits). Pixels start on the next `IM1=` string with a **fresh** 7-bit buffer.

### 6.3 Why Apps.info matches the palette but not the pixels

`Apps.info` COMPARE: palette RGB identical to icon.library, pixel indexes not.

Line 0 contains the full 192-bit palette plus 32 pad bits (including an RLE zero run in the pad region). The shared decoder finishes the palette correctly, then **consumes the 32 pad bits as the start of the pixel stream**. That predicts COMPARE exactly: **palette_match yes, pixels_match no**. Selected imagery fails for the same per-line reason (`IM2=` first line is also palette + pad; 9 colours, 4 bpp).

### 6.4 0016.info palette mismatch

Line 0 likewise holds the full 768-bit palette plus 16 pad bits. Pixel desync from those pad bits is expected.

Palette RGB also failed to match despite both sides reporting 32 colours. Possible additional factors:

- same bitstream issue if icon.library’s exposed palette is not the raw stored triples
- icon.library / NewIcons.library remapping a 32-colour palette while keeping count 32
- less likely: ColorRegister comparison artefact (Apps.info’s 8-colour RGB comparison succeeded with the same oracle code)

Re-run COMPARE after the palette-line flush (`output.compare2.txt`): pixel CRCs changed; `palette_match` was still **no**. At that date this looked like icon.library presenting a different 32-colour palette (our palette CRC stayed `DAD2D1EE`). **Later disproved:** ordered RLE changed the palette CRC to `A5BF3CAD` and `output.compare4.txt` reports `palette_match: yes` (§13). First-line RLE was inside the palette stream.

### 6.5 Fix that was applied

Not a full reset of the bit buffer on every ToolType line (that would drop sample bits that straddle 7-bit character wraps on pixel lines).

Smallest change in `shared/icon/icon_newicons.c`:

- After the palette completes, **if further `IM1=`/`IM2=` lines exist**, discard leftover bits on that string (`ni_reset`) and start pixels on the next line.
- A **single-line** stream still reads pixels from the remainder of the same string (host RLE unit test).

Host encoder (`src/tests/test_shared_decode.c`): flush and start a new line after the palette.

Regression: load `TestsIcons/test-icons/Newicons/Apps.info` and `0016.info` (dimensions, palette RGB, pixel CRCs).

Host suites: `make test-decode`, `make test-icon`, `make test-coloricon`, `make test-image` — all passed.

Amiga: `make test-icon-amiga` and `make` — `Bin/Amiga/iTidy2/iTidyIconTest` and `iTidy2` rebuilt. ColorIcon, GlowIcon, classic, and `BEST` preference were not changed.

---

## 7. What this validation does *not* prove

- Classic planar pixels vs Intuition/icon.library raster (intentionally not compared).
- PNG / OS4 ARGB (none present in this corpus).
- NewIcons.library as a separate oracle (COMPARE used icon.library v44+ `GetIconTagList` / `IconControlA`).
- Visual appearance on Workbench (no screenshot pass).
- iTidy2 GUI behaviour (shared decoders are still not wired into the GUI).

---

## 8. Per-file appendix

CRCs are IEEE CRC32 from the shared decoder (decode-only and COMPARE logs agreed).

### GlowIcon

| File | Size | Pal N/S | Trans | Pixel CRC N/S | Oracle |
|---|---|---|---|---|---|
| `alpha/System.info` | 64×64 | 228 / 247 | 0 | `287418F5` / `857212B3` | MATCH |
| `alpha/Games.info` | 64×64 | 231 / 241 | 0 | `FCE214E0` / `981765C4` | MATCH |
| `ColorIcons/screen.info` | 48×48 | 113 / 106 | 0 | `F00B2BC7` / `1C2ACD11` | MATCH |
| `ColorIcons/AmigaMail.info` | 48×48 | 221 / 157 | 0 | `5BD571BD` / `8233D3C3` | MATCH |

### ColorIcon

| File | Size | Pal N/S | Trans | Pixel CRC N/S | Oracle |
|---|---|---|---|---|---|
| `ColorIcons/System_drawer.info` | 62×28 | 64 / 64 | 0 | `25579895` / `423D507A` | MATCH |
| `ColorIcons/screen2.info` | 46×46 | 16 / 17 | 0 | `B2B4A12F` / `99675A1E` | MATCH |
| `ColorIcons/disk.info` | 31×33 | 11 / — | 0 | `EE40252B` / — | MATCH |
| `OS3/Orange.info` | 46×46 | 13 / 15 | 0 | `A81CA4A5` / `7B94951A` | MATCH |
| `OS3/green.info` | 46×46 | 14 / 15 | 0 | `43F3976F` / `C5B412E7` | MATCH |
| `OS3/Blue.info` | 42×42 | 21 / 21 | 0 | `8E6B6E5F` / `640F948C` | MATCH |

### NewIcons

Historical CRCs from earlier COMPARE runs are in §11 and §13. The table below is the **canonical post-fix** state (`output.compare4.txt`).

| File | Size | Pal N/S | Trans | Pixel CRC N/S | Palette CRC | Index oracle | RGB oracle |
|---|---|---|---|---|---|---|---|
| `Newicons/Apps.info` | 36×40 | 8 / 9 | 0 | `22CF0A9F` / `70EA1B88` | `C7DCC148` | **MATCH** | **MATCH** |
| `Newicons/0016.info` | 42×42 | 32 / 32 | 0 | `96DB489A` / `372E4ECC` | `A5BF3CAD` | **MATCH** | **MATCH** |

### Classic (oracle skipped)

| File | Size | Selected | Pixel CRC N/S |
|---|---|---|---|
| `MUI/HD.info` | 55×23 | yes | `410175DD` / `DB5AA744` |
| `MUI/drawer.info` | 56×15 | yes | `290E3C59` / `EBB5F50E` |
| `MUI/Disk.info` | 44×23 | yes | `F0DF8A8E` / `4A7DDE13` |
| `OS1.3/Prefs.info` | 73×18 | yes | `33C0E4A4` / `853335BD` |
| `OS1.3/Disk.info` | 35×16 | no | `AFBA417B` / — |
| `OS3/Disk.info` | 36×17 | no | `6140DE83` / — |
| `OS4/install.info` | 48×20 | no | `C4A4B284` / — |
| `OS4/drawer.info` | 48×14 | yes | `02038445` / `67E5A464` |
| `OS4/Aladdin4D.info` | 64×65 | yes | `D777DB19` / `34195BEA` |
| `AQUARIUMBACKGROUND.HAM.info` | 64×40 | no | `61B3AC4B` / — |
| Root drawers (×7) | 57×21 | yes | `4DEE3FEF` / `3472F656` |

---

## 9. Conclusions and next actions

1. **Shared ColorIcon/GlowIcon path is fit for Batch 5 consumption** from this corpus and icon.library COMPARE (index and RGB, all COMPARE logs).
2. **Classic path decoded every classic sample** without error; pixel identity vs icon.library was correctly out of scope.
3. **NewIcons ordered-RLE + per-ToolType flush is in** and should be kept. Palette-line-only flush (§11) was necessary but not sufficient. `output.compare4.txt` shows both NewIcons files MATCH icon.library on RGB **and** raw index/palette.
4. **This corpus has no PNG/ARGB** — unsupported-path testing still depends on the synthetic `tests/icons/malformed/` samples.
5. **Batch 5** may start on **`dev-classic`**. Do not start it on `dev-itidy2` / `main` / `v1`. Shared decoders are still not wired into the iTidy2 GUI.
6. Do not change ColorIcon/GlowIcon/classic decoder code for NewIcons. Decoding must continue to leave ToolTypes immutable.

---

## 10. Related documents

- Staged implementation record: `docs/current refactor/iTidy_Shared_Core_Refactor_Staged_Agent_Prompt.md`
- Architectural plan: `docs/current refactor/iTidy_Shared_Core_Unification_Plan.md`
- Format notes: `support files/IconFormats.txt`
- NewIcons research: `docs/current refactor/deep-research-report-newIcons.md`
- Validator source: `src/tests/amiga/icon_decode_test.c`
- Raw logs (project root): `output.txt`, `output.compare.txt`, `output.compare2.txt`, `output.compare3.txt`, `output.compare4.txt`

---

## 11. Amendment — NewIcons palette-line flush (2026-08-14)

### 11.1 Code change

`shared/icon/icon_newicons.c`: after the palette completes, leftover bits on that ToolType are discarded when further `IM1=`/`IM2=` lines exist. Pixels start on the next string. Single-line encodings still consume leftover bits as pixels.

`src/tests/test_shared_decode.c`: host encoder flushes and starts a new line after the palette; regression loads the two real NewIcons files (structure, palette RGB, pixel CRCs).

ColorIcon, GlowIcon, classic, and unified `BEST` preference were not modified.

### 11.2 Host CRCs after the fix

| File | Normal pixels | Selected pixels | Palette |
|---|---|---|---|
| `Apps.info` | `AFFA4641` (was `6C34D7C1`) | `F914E881` (was `A117D9A5`) | `C7DCC148` (unchanged) |
| `0016.info` | `E88DAA8C` (was `5D6EDB83`) | `49ACFE9B` (was `46B6B7E5`) | `DAD2D1EE` (unchanged) |

Palette CRCs unchanged shows line-0 palette decode was already correct. Pixel CRCs changed, so the flush did alter the pixel stream.

### 11.3 Second COMPARE — `output.compare2.txt`

The rebuilt `Bin/Amiga/iTidy2/iTidyIconTest` was run:

```text
Bin/Amiga/iTidy2/iTidyIconTest TestsIcons/test-icons COMPARE
```

Summary (identical counts to `output.compare.txt`):

```text
Oracle (icon.library v44+):
  Attempts:                 29
  Matches:                  10
  Mismatches:               2
  Skipped:                  17
RESULT: PASS
```

Amiga NewIcons CRCs in `output.compare2.txt` match the host post-fix table above. Oracle detail:

| File | size | palette | pixels | trans | selected | Result |
|---|---|---|---|---|---|---|
| `Apps.info` | yes | **yes** (lib 8) | **no** | yes (0) | **no** (lib has_image2=yes) | MISMATCH |
| `0016.info` | yes | **no** (lib 32) | **no** | yes (0) | **no** (lib has_image2=yes) | MISMATCH |

ColorIcon/GlowIcon remained 10/10 MATCH. Classic remained 17 SKIPPED.

**The flush ran on the Amiga and is not a no-op, but it did not achieve icon.library index identity.**

Remaining explanations after the flush (before RGB COMPARE):

1. **Pixel `IM1=`/`IM2=` wraps may also flush independently** (IconFormats.txt describes flush at every string boundary, not only after the palette). The current decoder still carries leftover sample bits across pixel lines. **Still open after `output.compare3.txt`.**
2. **icon.library may remap NewIcons indexes** even with `ICONGETA_RemapIcon=FALSE`. **Rejected by RGB-composite COMPARE** (`output.compare3.txt`, §12.3): both NewIcons files are RGB MISMATCH, so the pictures are not the same after palette lookup.
3. **`0016.info` `palette_match: no`** with an **unchanged** stored 32-colour CRC (`DAD2D1EE`) points at icon.library presenting a different 32-colour palette, not at a line-0 palette bitstream bug. **Still open.**

Do not revert the palette-line flush. RGB-composite COMPARE (`output.compare3.txt`, §12.3) has now been run: remapping is ruled out. Pixel-line wrap flush remains the leading bitstream hypothesis **of that date**. It was necessary but not sufficient; ordered RLE was the missing in-line defect (§13).

---

## 12. Amendment — RGB-composite COMPARE (2026-08-14)

Validator only. **No ColorIcon / NewIcons / classic decoder changes.** Shared modules remain independent of icon.library.

### 12.1 What was added

`src/tests/amiga/icon_decode_test.c` COMPARE now builds an effective RGB image from each side’s own chunky indexes and RGB palette:

```text
shared_rgb[p] = shared_palette[shared_pixels[p]]
oracle_rgb[p] = oracle_palette[oracle_pixels[p]]
```

Normal and selected are compared independently. Palette indexes are bounds-checked before dereference. Transparent indexes match semantically (both transparent = equal; one transparent = mismatch). Invalid indexes print as `ORACLE RGB: FAIL (invalid palette index)` with pixel number, x/y, and both indexes.

Per-file ORACLE fields now include:

```text
index_pixels_match
palette_match
rgb_composite_match
selected_index_match
selected_palette_match
selected_rgb_match
```

Index-level `ORACLE RESULT: MATCH` is unchanged (the 10 ColorIcon/GlowIcon cases remain the control). A new `ORACLE RGB:` line reports visual identity.

### 12.2 Build / host tests (this session)

- `make test-icon-amiga`: success, no VBCC warnings. Binary `Bin/Amiga/iTidy2/iTidyIconTest`.
- `make test-image`: all passed
- `make test-icon`: all passed
- `make test-coloricon`: all passed
- `make test-decode`: all passed (including real `Apps.info` / `0016.info` CRC checks)
- `make` (iTidy2): success. Decoder sources were not modified.

Decoder algorithms were not changed. Batch 5 was not started.

### 12.3 Third COMPARE — `output.compare3.txt`

The rebuilt `Bin/Amiga/iTidy2/iTidyIconTest` was run:

```text
Bin/Amiga/iTidy2/iTidyIconTest TestsIcons/test-icons COMPARE
```

```text
Oracle (icon.library v44+):
  Attempts:                 29
  Matches:                  10
  Mismatches:               2
  Skipped:                  17
  RGB composite matches:    10
  RGB composite mismatches: 2
  RGB composite skipped:    0
  RGB invalid palette idx:  0
RESULT: PASS
```

Decode still 29/29, 0 failures. Index MATCH/MISMATCH/SKIPPED counts are unchanged from `output.compare2.txt`.

**Control:** all 10 ColorIcon/GlowIcon files are `ORACLE RESULT: MATCH` and `ORACLE RGB: MATCH` (including `ColorIcons/disk.info` selected `n/a` / lib `has_image2=no`). Classic remains 17 SKIPPED. No file hit an out-of-range palette index.

**Answer to the remapping question:** NewIcons are **not** visually/RGB identical to icon.library despite differing indexes. `ORACLE RGB: MISMATCH` on both files. A lossless remap of the same picture would have produced RGB MATCH with `index_pixels_match: no`.

#### `Newicons/Apps.info` (36×40)

- Normal palette **matches** icon.library (8 colours). `trans_match: yes` (index 0).
- `index_pixels_match: no`, `rgb_composite_match: no`.
- First RGB failure is **transparency**, not a different colour in the same hole:
  - pixel **207** at **(27, 5)**
  - shared index **0** (transparent)
  - oracle index **6** (opaque)
- Selected palette **matches** (9 colours). First RGB failure is a **real colour difference**:
  - pixel **376** at **(16, 10)**
  - shared index **1** → RGB **136,136,136**
  - oracle index **3** → RGB **204,204,204**

Because the palettes are identical in order, different indexes mean different colours. That cannot be a lossless remap.

#### `Newicons/0016.info` (42×42)

- Palette **does not match** (both sides report 32 colours; stored CRC still `DAD2D1EE`). `trans_match: yes` (index 0).
- Both images fail first as **transparent vs opaque** on the first scanline:

| Image | Pixel | x,y | Shared | Oracle |
|---|---|---|---|---|
| Normal | 23 | (23, 0) | index 0, transparent | index 30, opaque |
| Selected | 15 | (15, 0) | index 0, transparent | index 24, opaque |

#### Interpretation

This is a **NewIcons pixel-stream / layout** disagreement, not a COMPARE artefact and not icon.library remapping of an otherwise identical image.

1. Shared decoder still carries leftover sample bits across **pixel** `IM1=` / `IM2=` line wraps (palette-line flush only). IconFormats.txt describes a flush at every string boundary.
2. First failures are holes vs solid colour (`Apps` normal, both `0016` images), which is typical of a shifted index stream.
3. `0016.info` still presents a different 32-colour palette from icon.library, so palette identity remains an open NewIcons-oracle question.

Do not start Batch 5 on the assumption NewIcons is visually correct. Do not change ColorIcon/GlowIcon/classic for this. Next useful step is a NewIcons-format investigation using the coordinates above.

**Resolved in §13.** The remaining defect was RLE zeros overtaking residual accumulator bits, plus carrying leftover bits across every physical ToolType (not only the palette line). `output.compare4.txt` is a full MATCH; the coordinates above are retained as historical forensic evidence of the pre-fix decoder.

---

## 13. Amendment — NewIcons ordered RLE + physical ToolType framing (2026-08-14)

Research: `docs/current refactor/deep-research-report-newIcons.md` (AROS `diskobjNIio.c`, Deark `amigaicon.c`, original NewIcons 4.6). ColorIcon / GlowIcon / classic decoder code was not changed.

### 13.1 Defects

The palette-line flush (§11) was correct for the first `IM1=`/`IM2=` string but left two independent bugs:

1. **RLE ordering.** Ordinary encoded bits lived in `acc`/`nbits`. RLE zeros lived in a separate `zero_queued` counter that `ni_get_bit()` consumed **first**. When an RLE token arrived with a partial sample already in `acc`, the new zeros overtook the older residual bits and reversed source order.

   Encoded `6F D1` expands to `1001111 0000000`. At 3 bits/sample the stream is `4, 7, 4, 0`. The old queue produced `4, 7, 0, …`. At 8 bits/sample the first byte must be `0x9E`; the old queue could yield `0x01`.

2. **Physical ToolType framing.** Residual bits were still carried across **pixel** continuation lines. AROS and Deark discard leftover bits at every physical `IM1=`/`IM2=` NUL. Samples must not straddle ToolType strings. Palette completion also ends that physical line: leftover characters on the palette-finishing ToolType are padding, not pixels.

The `output.compare3.txt` first-mismatch positions were too early to be explained by a full 123-character pixel line alone (`Apps` pixel 207 vs ≥287 pixels/line at 3 bpp; `0016` pixel 23 vs ≥172 pixels/line at 5 bpp). That is exactly what in-line RLE reordering produces.

### 13.2 Code change

`shared/icon/icon_newicons.c` / `icon_newicons.h`:

- Removed `zero_queued` as a competing bit source. RLE `0xD1..0xFF` now queues N zero-valued **7-bit groups** that are shifted into the same accumulator **after** residual bits.
- Every physical `IM1=`/`IM2=` attach resets `acc`, `nbits`, and pending RLE state.
- After `colors × 3` palette bytes, the rest of that ToolType is discarded; pixels start on the next same-image ToolType with a clean accumulator.
- `bpp = max(1, ceil(log2(ncolors)))` (one-colour uses 1 bit, not 0; 257 colours use 9 bits).
- `IM2` remains an independent image (own header, palette, transparency, pixels). Dimensions must match `IM1`.
- 257-entry compatibility: accept 257, decode 9-bit samples into a `UWORD`, store as `UBYTE` only if every referenced index is 0..255; return `UNSUPPORTED` if index 256 is used. No silent wrap of 256 to 0.
- Marker-aware collection: after `*** DON'T EDIT THE FOLLOWING LINES!! ***`, take contiguous `IM1=` then `IM2=` runs. Unrelated ToolTypes before the marker are ignored. The ToolType table is never written.
- If the marker is absent, collection still starts at the first `IM1=`/`IM2=` so truncated host samples remain diagnosable.

`src/tests/test_shared_decode.c`: synthetic encoder flushes each physical line and starts pixels on the next ToolType. Same-line palette-plus-pixels encodings are no longer treated as canonical. Permanent microscopic tests: ordered RLE `6F D1`, physical boundary `6F` then `20`, IM2 8-vs-9 palettes, 1-bit bpp, 256/257, marker ignore, truncated continuation.

`src/tests/amiga/icon_decode_test.c`: on NewIcons RGB mismatch, print IM1 vs IM2 and sample index. Successful icons stay quiet.

### 13.3 Host results

```text
make test-decode / test-image / test-icon / test-coloricon
```

All passed. Microscopic RLE: `sample[2] == 4`. Physical boundary: `4, 7, 0, 0`. Real files decoded on host with the CRCs in §1 / §8.

Known-bad fingerprints of the pre-RLE-order decoder (must not be restored):

| File | Era | Normal pixels | Selected pixels | Palette |
|---|---|---|---|---|
| `Apps.info` | pre palette-flush | `6C34D7C1` | `A117D9A5` | `C7DCC148` |
| `Apps.info` | palette-flush only | `AFFA4641` | `F914E881` | `C7DCC148` |
| `Apps.info` | **ordered RLE + every-line flush** | **`22CF0A9F`** | **`70EA1B88`** | **`C7DCC148`** |
| `0016.info` | pre palette-flush | `5D6EDB83` | `46B6B7E5` | `DAD2D1EE` |
| `0016.info` | palette-flush only | `E88DAA8C` | `49ACFE9B` | `DAD2D1EE` |
| `0016.info` | **ordered RLE + every-line flush** | **`96DB489A`** | **`372E4ECC`** | **`A5BF3CAD`** |

`Apps.info` palette CRC never moved (it already matched icon.library). `0016.info` palette CRC **did** move, confirming first-line RLE was inside the 768 palette bits.

Amiga builds: `make` → `Bin/Amiga/iTidy2/iTidy2`; `make test-icon-amiga` → `Bin/Amiga/iTidy2/iTidyIconTest`. Both 68000, no VBCC warnings on the changed files.

### 13.4 Fourth COMPARE — `output.compare4.txt`

```text
Bin/Amiga/iTidy2/iTidyIconTest TestsIcons/test-icons COMPARE
```

```text
Files scanned:              29
Decoded successfully:       29
Failures:                   0

Formats decoded:
  Classic:                  17
  NewIcons:                 2
  ColorIcon:                6
  GlowIcon:                 4

Oracle (icon.library v44+):
  Attempts:                 29
  Matches:                  12
  Mismatches:               0
  Skipped:                  17
  RGB composite matches:    12
  RGB composite mismatches: 0
  RGB composite skipped:    0
  RGB invalid palette idx:  0
RESULT: PASS
```

#### `Newicons/Apps.info` (36×40)

```text
size_match:              yes  (lib 36 x 40)
index_pixels_match:      yes
palette_match:           yes  (lib 8 colours)
trans_match:             yes  (lib 0)
rgb_composite_match:     yes
selected_match:          yes  (lib has_image2=yes)
selected_index_match:    yes
selected_palette_match:  yes
selected_rgb_match:      yes
ORACLE RESULT: MATCH
ORACLE RGB: MATCH
```

Shared CRCs: pal `C7DCC148`, pix `22CF0A9F`, selected pal `858B4EE8`, selected pix `70EA1B88`. Independent 8-colour normal / 9-colour selected palettes confirmed.

#### `Newicons/0016.info` (42×42)

```text
size_match:              yes  (lib 42 x 42)
index_pixels_match:      yes
palette_match:           yes  (lib 32 colours)
trans_match:             yes  (lib 0)
rgb_composite_match:     yes
selected_match:          yes
selected_index_match:    yes
selected_palette_match:  yes
selected_rgb_match:      yes
ORACLE RESULT: MATCH
ORACLE RGB: MATCH
```

Shared CRCs: pal `A5BF3CAD` (normal and selected), pix `96DB489A`, selected pix `372E4ECC`.

Raw index/palette equality was **stronger than required**. The research allowed RGB MATCH with a raw mismatch (`ICONGETA_RemapIcon=FALSE` is not a promise of serialized ToolType identity). Here icon.library agreed on both.

ColorIcon/GlowIcon remained 10/10 MATCH (index and RGB). Classic remained 17 SKIPPED. No invalid palette indexes.

### 13.5 Hypotheses closed

| Earlier hypothesis | Outcome |
|---|---|
| Palette-line flush only | Necessary, not sufficient (`compare2`/`compare3`) |
| Flush every ToolType, keep zero-priority RLE queue | Would still reorder bits in-line |
| Flush every ToolType **and** serialize RLE in source order | **Authoritative; confirmed by `compare4`** |
| icon.library remapping of an identical picture | Already rejected by RGB MISMATCH in `compare3`; now also raw MATCH |
| `0016.info` palette mismatch is icon.library remapping | **Rejected.** Palette CRC change + `palette_match: yes` shows the stored stream was wrong under zero-priority RLE |
| Pixels sharing the palette’s final ToolType | Noncanonical; encoder/tests updated |
| IM2 sharing IM1 palette | Wrong; Apps 8 vs 9 is a permanent regression |

### 13.6 Status

NewIcons direct decoding is fit for Batch 5 consumption from this corpus. ToolTypes remain immutable during decode. Batch 5 was not started here; it belongs on `dev-classic`.



