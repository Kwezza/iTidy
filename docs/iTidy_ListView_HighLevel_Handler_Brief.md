# iTidy ListView – High-Level IDCMP Handler Brief

## Goal

Add a **single high-level handler** to the iTidy ListView library that:

- Takes a **LISTVIEW `IDCMP_GADGETUP` event**
- Figures out whether the user clicked the **header** or a **data row**
- If it’s a header: performs sorting and reports which column/order
- If it’s a row: returns the **clicked cell** (entry + column + values)
- Returns this information as a **small struct** so callers can just switch on “what happened”.

This must work with a standard Amiga Workbench 3.0 GadTools LISTVIEW gadget and the existing listview architecture in `LISTVIEW_SORTING_ARCHITECTURE.md`.

---

## Phase 1 – Understand the existing design

1. Read `LISTVIEW_SORTING_ARCHITECTURE.md` end-to-end, paying attention to:
   - `iTidy_ListViewEntry`
   - `iTidy_ListViewState` + column pixel ranges
   - Sorting helpers (`iTidy_ResortListViewByClick`, comparator, etc.)
   - Click helpers (`iTidy_GetClickedColumn`, `iTidy_GetListViewClick`, `iTidy_GetSelectedEntry`)
   - Cleanup helper (`itidy_free_listview_entries` or equivalent)
2. Locate the existing implementation files in the repo (e.g. `listview_formatter.c` / `listview_formatter.h` or similar) and confirm:
   - What functions are actually implemented.
   - Final function signatures (fix the doc if it disagrees with code).

> If you find mismatches between doc and code, prefer **compiling code** as the source of truth and update comments accordingly.

---

## Phase 2 – Design the high-level event API

3. Define a new **event type enum** and **event result struct**, for example:

   - `iTidy_ListViewEventType`:
     - `ITIDY_LV_EVENT_NONE`
     - `ITIDY_LV_EVENT_HEADER_SORTED` (or `HEADER_CLICK`)
     - `ITIDY_LV_EVENT_ROW_CLICK`
   - `iTidy_ListViewEvent`:
     - `type`
     - `did_sort`
     - For row clicks: `entry`, `column`, `display_value`, `sort_key`, `column_type`
     - For header: `sorted_column`, `sort_order` (if you perform the sort here)

4. Design a single high-level handler function, e.g.:

```c
BOOL iTidy_HandleListViewGadgetUp(
    struct Window        *win,
    struct Gadget        *gad,
    struct IntuiMessage  *imsg,
    struct List          *entry_list,
    struct List          *display_list,
    iTidy_ListViewState  *state,
    struct TextFont      *font,
    iTidy_ColumnConfig   *columns,
    int                   num_columns,
    iTidy_ListViewEvent  *out_event
);
```

The exact parameter list may be adjusted to better match existing types – prefer minimal, coherent parameters over blindly copying this example.

---

## Phase 3 – Implement the handler using existing helpers

5. In the implementation file, implement `iTidy_HandleListViewGadgetUp` roughly as follows (but feel free to adjust as needed for correctness):

   - Extract `mouse_x` / `mouse_y` from `imsg` **before** the caller replies.
   - Convert to gadget-relative coordinates (using `gad->LeftEdge`, `gad->TopEdge`).
   - Using `font->tf_YSize` (or the configured header height) and `iTidy_ListViewState`, decide if the click was in the **header band** or **body**:
     - If **header**:
       - Use `iTidy_GetClickedColumn` (or equivalent) to find the column index.
       - If a valid column:
         - Update column sort state (toggle ascending/descending).
         - Call the existing sort helper (`iTidy_ResortListViewByClick` or derivative).
         - Set `out_event->type = ITIDY_LV_EVENT_HEADER_SORTED`, `sorted_column`, `sort_order`, `did_sort = TRUE`.
       - Return `TRUE`.
     - If **body**:
       - Read the selected row index from the ListView (`GTLV_Selected` or `imsg->Code`, depending on current design).
       - Use `iTidy_GetSelectedEntry` to map to `iTidy_ListViewEntry *` (handling header offset correctly).
       - Use `iTidy_GetListViewClick` (or similar) plus `mouse_x` to determine which column/cell was hit.
       - Populate `out_event` for `ITIDY_LV_EVENT_ROW_CLICK`, including `entry`, `column`, `display_value`, `sort_key`, `column_type`.
       - Return `TRUE`.
   - If anything fails (e.g. no valid column, click outside content), set `type = ITIDY_LV_EVENT_NONE` and return `FALSE` or `TRUE` according to what’s most convenient for callers (document this clearly in the header).

6. Reuse as much existing logic as possible:
   - Prefer calling `iTidy_GetClickedColumn`, `iTidy_GetListViewClick`, `iTidy_GetSelectedEntry` instead of duplicating maths.
   - If these helpers’ signatures don’t quite fit, you may modify them as long as you:
     - Keep behaviour equivalent.
     - Update all call-sites.
     - Update `LISTVIEW_SORTING_ARCHITECTURE.md` comments where practical.

---

## Phase 4 – Integrate into one existing window (pilot)

7. Pick **one** existing iTidy window that uses the sortable ListView and modify its IDCMP loop:

   - In `IDCMP_GADGETUP` for that listview gadget:
     - Call `iTidy_HandleListViewGadgetUp(...)`.
     - If `event.did_sort == TRUE`:
       - Call `GT_SetGadgetAttrs` with `GTLV_Labels, display_list` to refresh the listview.
     - Switch on `event.type`:
       - `HEADER_SORTED`: optional UI updates (e.g. log, status text).
       - `ROW_CLICK`: act on `event.entry` and `event.column` as appropriate.
   - Remove duplicated header-vs-row tests and manual column hit-testing from that window.

8. Compile and fix any type/parameter mismatches; you are allowed to:
   - Adjust function signatures (within reason) if they don’t compile or feel awkward.
   - Add small helper functions inside the listview module if they simplify the high-level handler.

---

## Phase 5 – Test & polish

9. Test on real Amiga (or emulator) with the pilot window:

   - Click various header columns:
     - Confirm sort toggles ASC/DESC.
     - Confirm the list reorders correctly and that the gadget refreshes.
   - Click data rows:
     - Confirm `ROW_CLICK` gives the expected entry and column.
     - Confirm header clicks do **not** return `ROW_CLICK`.
   - Confirm that clicks outside the list / header behave sensibly (e.g. `NONE`).

10. Once satisfied, update:

   - The listview header file with the new public API and a brief doc comment.
   - `LISTVIEW_SORTING_ARCHITECTURE.md` in the “High-Level IDCMP Handler” section summarising:
     - The new event struct.
     - The new handler function.
     - A short sample usage in IDCMP loop.

> Throughout the work, prefer **working, compiling code** over the current documentation if they disagree. Adjust the docs after the code is stable, not the other way around.
