Goal: Implement the “Selected Type” panel behaviour for the DefIcons: Text Templates ReAction window.

Context:
- The left listbrowser shows template types (ascii, rexx, c, html, etc) with columns like Type / Template / Status.
- The right side “Selected Type” section is read-only summary text showing what will be acted on by the Template actions buttons.
- File-level operations (create/overwrite templates, editing tooltypes via WB Info) are immediate; the window has a Close button.

Data model assumptions (adapt to existing structs):
- Each row represents a template type with fields:
  - typeName: string (e.g. "rexx")
  - customTemplatePath: string or NULL (e.g. "PROGDIR:Icons/def_rexx.info")
  - masterTemplatePath: string (e.g. "PROGDIR:Icons/def_ascii.info") [the master for fallback]
  - effectiveTemplatePath: resolves to customTemplatePath if it exists else masterTemplatePath
  - status enum: CUSTOM or FALLBACK (and optionally ERROR if master missing)

UI objects to update (read-only):
- SelectedType_TypeValue
- SelectedType_TemplateFileValue
- SelectedType_EffectiveValue
- SelectedType_StatusValue
Buttons:
- Btn_CreateOrOverwriteFromMaster
- Btn_EditTooltypes
- Btn_ValidateTooltypes
- Btn_Refresh (always enabled)

Populate/refresh logic:
1) On window open:
   - Ensure “Selected Type” shows a clean “no selection” state (see below).
   - Disable Template actions until a row is selected.

2) On list selection change:
   - If selection is valid:
     - Read the selected row’s model data.
     - Update “Selected Type” display:
       - Type: set to the selected typeName (e.g. "Rexx" or "rexx" consistently)
       - Template file: show the actual file that exists for the selected type:
         - if custom exists: show customTemplatePath
         - else: show "-" or "(none)" or "Using master" (choose a consistent style)
       - Effective: show effectiveTemplatePath (this is what will actually be used when generating icons)
       - Status: "Custom" if custom exists else "Using master" (or "Fallback")
     - Update button enabled states:
       - Enable Edit/Validate when a selection exists, but consider rules below.
       - Enable Create/Overwrite except when selected type is the master itself.
     - Update Create/Overwrite button label dynamically (see below).
   - If selection is invalid or cleared:
     - Reset to “no selection” state (see below).

No-selection state (must be clean, not stale):
- When no row is selected (or list is empty):
  - Selected type fields:
    - Type: "(none)"
    - Template file: "-"
    - Effective: "-"
    - Status: "-"
  - Disable Template actions buttons:
    - Btn_CreateOrOverwriteFromMaster disabled
    - Btn_EditTooltypes disabled
    - Btn_ValidateTooltypes disabled
  - Refresh remains enabled.

Master type rules:
- The master template type is the “ascii” row (or however the code identifies it).
- When the selected type is the master itself:
  - Disable Btn_CreateOrOverwriteFromMaster (it makes no sense to copy master from itself)
  - Keep EditTooltypes enabled (editing master tooltypes is valid)
  - Keep ValidateTooltypes enabled

Dynamic Create/Overwrite button label:
- Use one button, but change its text depending on whether the selected type has a custom template:
  - If customTemplatePath exists: label = "Overwrite from master"
  - Else: label = "Create from master"
- Tooltip/hint (if supported): explain it copies the master template icon and names it for the selected type (def_<type>.info).
- The button should always act on the selected type. It should:
  - Copy masterTemplatePath -> customTemplatePath (creating directories if necessary)
  - If overwriting, confirm with requester first (optional but recommended)
  - After copy succeeds: refresh the model row (status becomes CUSTOM, template file becomes customTemplatePath), refresh listbrowser, and update Selected Type panel again.

Edit ToolTypes behaviour (choose one and implement consistently):
- Preferred: Edit the EFFECTIVE template file (so the user always edits what is actually in use).
  - That means if fallback, WB Info opens masterTemplatePath.
  - If custom, WB Info opens customTemplatePath.
- Alternative (only if you explicitly want to avoid editing master from this screen):
  - If fallback: disable Edit ToolTypes and show a hint “Create a custom template to edit tooltypes”
  - If custom: allow editing custom.
Pick one approach and ensure labels/hints match.

Validation behaviour:
- Validate should validate the EFFECTIVE template (and/or both master + custom if you want deeper checks).
- Show results via requester/log; do not change UI state except possibly a status line.

Implementation detail:
- Update the fields via SetGadgetAttrs()/SetAttrs() and then refresh/redraw as needed.
- Make sure updates occur when:
  - Selection changes
  - After Create/Overwrite completes
  - After Refresh is clicked
  - After returning from WB Info (if you can detect it; otherwise user can hit Refresh)

Deliverable:
- Add a function like UpdateSelectedTypePanel(selectedIndex or selectedNode) that encapsulates:
  - Read selected row -> set strings
  - Apply no-selection defaults
  - Enable/disable buttons
  - Set Create/Overwrite label dynamically
- Call it from the listbrowser IDCMP handler and after actions that mutate files.

Implement “Revert to master” button: deficons_text_btn_revert_to_master

Purpose
- Revert the selected type back to using the master template by removing its custom template icon file (def_<type>.info).
- This is an immediate file-level change. After success, UI must refresh to show Status = Using master/Fallback and Effective = master.

Enable/disable rules
- Disabled when:
  - No list item is selected
  - Selected type is the master itself (e.g. "ascii")
  - No custom template exists for the selected type (already using master)
- Enabled when:
  - A non-master type is selected AND its custom template icon file exists on disk

Action behaviour (on click)
1) Resolve paths from the selected row/model:
   - typeName (e.g. "rexx")
   - customTemplatePath (e.g. "PROGDIR:Icons/def_rexx.info") or whatever your existing scheme uses
   - masterTemplatePath (e.g. "PROGDIR:Icons/def_ascii.info")

2) Confirm with the user (recommended):
   - Requester text:
     Title: "Revert to master"
     Body: "Delete the custom template for type '<typeName>' and use the master template instead?"
     Buttons: "Revert" / "Cancel"
   - If Cancel -> do nothing.

3) Delete the custom template file:
   - Delete customTemplatePath (the .info file)
   - Optional robustness:
     - If you also store a paired non-.info file, do NOT delete it; only delete the template icon file.
     - If delete fails because file does not exist, treat as already reverted and continue to refresh UI.
     - If delete fails due to protection/lock, show an error requester with the OS error string (IoErr()).

4) Post-action refresh:
   - Update the in-memory model for that row:
     - customTemplatePath = NULL (or mark missing)
     - effectiveTemplatePath = masterTemplatePath
     - status = FALLBACK / USING_MASTER
   - Refresh listbrowser rows (either rebuild list or update the selected node)
   - Call UpdateSelectedTypePanel() again so the right-hand “Selected Type” display and button states are correct.
   - Ensure Create/Overwrite button label updates:
     - Now that custom is gone -> label should become "Create from master"

Edge cases
- If the master template file is missing (def_ascii.info not found):
  - Revert still deletes the custom file, but Effective becomes “(missing master)” and status becomes ERROR.
  - In that state, disable Create/Overwrite (or show error on attempt) until master is restored.

Implementation notes (Amiga / ReAction)
- Trigger is IDCMP_GADGETUP for the revert gadget ID.
- Use DeleteFile(customTemplatePath) (dos.library) and check return value; use IoErr() on failure.
- After any external change (including WB Info edits), Refresh should re-scan disk; the revert action should also re-scan or update the model directly.
- Keep all UI updates in one place:
  - UpdateSelectedTypePanel(selectedIndex) should handle:
    - no selection state
    - enable/disable for revert button
    - dynamic label for create/overwrite button