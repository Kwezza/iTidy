# iTidy Restore Window - GUI Design Specification

**Document Version:** 1.0  
**Date:** October 27, 2025  
**Target Platform:** Amiga OS 2.0+ / Workbench 2.0+ Compatible  
**GUI Framework:** Pure Intuition + GadTools (No MUI, No ReAction)  
**GadTools Version:** Standard GadTools (WB 2.0+) - LISTVIEW_KIND, BUTTON_KIND, etc.  
**Status:** Design Phase  

---

## 1. Overview

The **Restore Window** provides a user-friendly GUI interface for browsing and restoring backups created by iTidy's automatic backup system. Users can view available backup runs, select individual folders or entire runs to restore, and monitor the restore process.

### Purpose
- Browse all available backup runs
- View details of backup runs (date, folder count, total size)
- Restore complete backup runs
- Restore individual archived folders
- Recover orphaned archives (without catalog)
- Provide feedback during restore operations

### Integration Point
- Opened via **"Restore Backups..."** button on main iTidy window
- Modal operation (blocks main window while open)
- Returns to main window when closed

---

## 2. Window Specification

### Window Properties

**⚠️ CRITICAL: Dynamic Sizing Based on Font**

The window must be **dynamically sized** based on the Workbench screen's font dimensions. Never use fixed pixel sizes.

| Property | Value | Notes |
|----------|-------|-------|
| Title | `"iTidy - Restore Backups"` | Clear identification |
| Width | **CALCULATED** | Based on gadget widths + margins (see below) |
| Height | **CALCULATED** | Based on actual ListView height + gadget heights (see below) |
| Position | Centered on screen | Or offset from main window |
| Type | Modal | Blocks main window interaction |
| Flags | `WFLG_DRAGBAR` `WFLG_DEPTHGADGET` `WFLG_CLOSEGADGET` `WFLG_ACTIVATE` | Standard Workbench window |
| IDCMP | `IDCMP_CLOSEWINDOW` `IDCMP_REFRESHWINDOW` `IDCMP_GADGETUP` `IDCMP_GADGETDOWN` | Event handling |

### Font-Based Dimensions

```c
/* Get font dimensions from Workbench screen */
struct TextFont *font = window->RPort->Font;
UWORD font_width = font->tf_XSize;
UWORD font_height = font->tf_YSize;
UWORD font_baseline = font->tf_Baseline;

/* Calculate gadget dimensions */
UWORD button_width = font_width * 15;      /* ~15 chars wide */
UWORD button_height = font_height + 6;     /* Font + padding */
UWORD string_height = font_height + 6;     /* Font + border */
UWORD listview_lines = 10;                 /* Visible rows */
UWORD listview_requested_height = (font_height + 2) * listview_lines;

/* Spacing constants */
#define RESTORE_SPACE_X 10                 /* Horizontal spacing */
#define RESTORE_SPACE_Y 8                  /* Vertical spacing */
#define RESTORE_MARGIN_LEFT 10             /* Left margin */
#define RESTORE_MARGIN_TOP 10              /* Top margin */
#define RESTORE_MARGIN_RIGHT 10            /* Right margin */
#define RESTORE_MARGIN_BOTTOM 10           /* Bottom margin */
```

### Visual Layout

```
┌─────────────────────────────────────────────────────────────────┐
│ iTidy - Restore Backups                              [_][■][×]  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Backup Location: [PROGDIR:Backups                   ] [Change]│
│                                                                 │
│  Select Backup Run:                                             │
│  ┌────────────────────────────────────────────────────────────┐│
│  │ Run_0007  2025-10-25 14:32   63 folders   46 KB  Complete ││
│  │ Run_0006  2025-10-24 10:15   12 folders   15 KB  Complete ││
│  │ Run_0005  2025-10-23 16:22   45 folders   38 KB  Complete ││
│  │ Run_0004  2025-10-22 09:45    8 folders   11 KB  Complete ││
│  │ Run_0003  2025-10-21 14:18   23 folders   19 KB  Complete ││
│  │ Run_0002  2025-10-20 11:30   15 folders   13 KB  NoCAT    ││
│  │ Run_0001  2025-10-19 15:45   34 folders   29 KB  Complete ││
│  │                                                              ││
│  │                                                              ││
│  │                                                              ││
│  └────────────────────────────────────────────────────────────┘│
│                                                                 │
│  Note: Standard GadTools LISTVIEW_KIND - single column,        │
│        formatted text with spaces to align columns             │
│                                                                 │
│  Run Details:                                                   │
│  ┌────────────────────────────────────────────────────────────┐│
│  │ Run Number:        0007                                     ││
│  │ Date Created:      2025-10-25 14:32:17                      ││
│  │ Total Archives:    63                                       ││
│  │ Total Size:        46 KB                                    ││
│  │ Status:            Complete (catalog present)               ││
│  │ Location:          PROGDIR:Backups/Run_0007                 ││
│  └────────────────────────────────────────────────────────────┘│
│                                                                 │
│  [ ] Preview folders before restore                             │
│                                                                 │
│      [ Restore Run ]  [ View Folders... ]  [ Cancel ]          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. Gadget Specifications

### 3.1 Backup Location String Gadget (GID_RESTORE_BACKUP_PATH = 2001)

**Type:** STRING_KIND Gadget  
**Purpose:** Display/edit backup root directory  
**Position:** **CALCULATED** - See positioning section  
**Size:** **CALCULATED** from font dimensions  
**Default Value:** `"PROGDIR:Backups"`  
**Max Length:** 256 characters  

**Font-Based Sizing:**
```c
STRPTR label = "Backup Location:";
UWORD label_width = strlen(label) * font_width;
UWORD label_spacing = 4;

ng.ng_LeftEdge = current_x + label_width + label_spacing;
ng.ng_TopEdge = current_y;
ng.ng_Width = font_width * 40;           /* ~40 chars visible */
ng.ng_Height = string_height;             /* font_height + 6 */
ng.ng_GadgetText = label;
ng.ng_GadgetID = GID_RESTORE_BACKUP_PATH;
ng.ng_Flags = PLACETEXT_LEFT;

restore_data->backup_path_str = gad = CreateGadget(STRING_KIND, gad, &ng,
    GTST_String, "PROGDIR:Backups",
    GTST_MaxChars, 256,
    TAG_END);
```

**Behavior:**
- Shows current backup root path
- Editable (user can type different path)
- Updates run list when changed
- Validates path exists before accepting

### 3.2 Change Location Button (GID_RESTORE_CHANGE_PATH = 2002)

**Type:** BUTTON_KIND Gadget  
**Label:** `"Change"`  
**Position:** **CALCULATED** - Right of string gadget  
**Size:** **CALCULATED** from font dimensions  

**Font-Based Sizing:**
```c
ng.ng_LeftEdge = restore_data->backup_path_str->LeftEdge + 
                 restore_data->backup_path_str->Width + RESTORE_SPACE_X;
ng.ng_TopEdge = current_y;
ng.ng_Width = font_width * 10;            /* ~10 chars: "Change" */
ng.ng_Height = button_height;             /* font_height + 6 */
ng.ng_GadgetText = "Change";
ng.ng_GadgetID = GID_RESTORE_CHANGE_PATH;
ng.ng_Flags = PLACETEXT_IN;

restore_data->change_path_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
```

**Behavior:**
- Opens ASL directory requester
- Sets backup path on selection
- Refreshes run list with new path

### 3.3 Backup Run ListView (GID_RESTORE_RUN_LIST = 2003)

**Type:** GadTools LISTVIEW_KIND (Standard single-column list)  
**Purpose:** Display all available backup runs  
**Position:** **CALCULATED** - Below backup path  
**Size:** **CALCULATED** from font dimensions

**⚠️ CRITICAL: ListView Height Snapping**

ListView gadgets automatically adjust their height to show complete rows. The actual height will differ from the requested height. You **MUST** use the actual height for positioning subsequent gadgets.

**Font-Based Sizing:**
```c
current_y += string_height + RESTORE_SPACE_Y;

UWORD listview_lines = 10;                /* Want 10 visible rows */
UWORD listview_requested_height = (font_height + 2) * listview_lines;

ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = font_width * 65;            /* Wide enough for formatted columns */
ng.ng_Height = listview_requested_height; /* This is just a REQUEST */
ng.ng_GadgetText = "Select Backup Run:";
ng.ng_GadgetID = GID_RESTORE_RUN_LIST;
ng.ng_Flags = PLACETEXT_ABOVE;

restore_data->run_list = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
    GTLV_Labels, restore_data->run_list_strings,
    GTLV_Selected, 0,
    GTLV_ShowSelected, NULL,
    TAG_END);

/* CRITICAL: Get ACTUAL height after creation */
UWORD actual_listview_height = gad->Height;  /* NOT ng.ng_Height! */

/* Store bottom position for next gadgets */
UWORD listview_bottom_y = ng.ng_TopEdge + actual_listview_height;

/* Update window dimensions with ACTUAL height */
update_window_max_dimensions(&window_max_width, &window_max_height,
                            ng.ng_LeftEdge + ng.ng_Width,
                            ng.ng_TopEdge + actual_listview_height);
```  

**GadTools Limitations:**
- LISTVIEW_KIND supports **single column only**
- No built-in multi-column support
- No checkboxes or icons
- Simple string list with scrollbar

**Display Format:**
Each list entry is a formatted string with spaces to create visual columns:
```
"Run_0007  2025-10-25 14:32   63 folders   46 KB  Complete"
"Run_0006  2025-10-24 10:15   12 folders   15 KB  Complete"
"Run_0005  2025-10-23 16:22   45 folders   38 KB  NoCAT"
```

**Column Layout (using fixed-width spacing):**
- Position 0-9: Run name (9 chars)
- Position 10-27: Date (17 chars) - shortened to HH:MM only
- Position 28-38: Folder count (11 chars)
- Position 39-47: Size (9 chars)
- Position 48+: Status (Complete/NoCAT/Error)

**List Entry Structure:**
```c
struct RestoreRunEntry {
    UWORD runNumber;           /* Run number (1-9999) */
    char displayString[80];    /* Formatted for ListView */
    char runName[16];          /* "Run_0007" */
    char dateStr[24];          /* "2025-10-25 14:32:17" full date */
    ULONG folderCount;         /* Number of archives */
    ULONG totalBytes;          /* Total size in bytes */
    char sizeStr[16];          /* "46 KB" formatted */
    UWORD statusCode;          /* 0=Complete, 1=NoCAT, 2=Error */
    char statusStr[16];        /* "Complete" text */
    char fullPath[256];        /* Full path to run directory */
};
```

**Selection:**
- Single selection mode (LISTVIEW_KIND default)
- Highlights selected row
- Updates details panel on selection change
- No double-click support in standard GadTools (use button)

**Sorting:**
- Pre-sorted before display (descending run number)
- No interactive column sorting (not supported in LISTVIEW_KIND)

### 3.4 Run Details Panel (GID_RESTORE_DETAILS = 2004)

**Type:** TEXT_KIND Gadgets or IntuiText  
**Purpose:** Show detailed information about selected run  
**Position:** **CALCULATED** - Below ListView using actual height  
**Size:** **CALCULATED** from font dimensions

**Font-Based Sizing:**
```c
/* Position below ListView using ACTUAL bottom position */
current_y = listview_bottom_y + RESTORE_SPACE_Y;

/* Create read-only text gadgets for details */
UWORD detail_line_height = font_height + 4;
UWORD details_height = detail_line_height * 6;  /* 6 lines of info */

ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = font_width * 65;            /* Match ListView width */
ng.ng_Height = details_height;
ng.ng_GadgetText = "Run Details:";
ng.ng_GadgetID = GID_RESTORE_DETAILS;
ng.ng_Flags = PLACETEXT_ABOVE;

restore_data->details_panel = gad = CreateGadget(TEXT_KIND, gad, &ng,
    GTTX_Text, "(No run selected)",
    GTTX_Border, TRUE,
    TAG_END);
```  
**Fields:**
- Run Number
- Date Created
- Total Archives
- Total Size
- Status (with explanation)
- Full path to run directory

**Display Format:**
```
Run Number:        0007
Date Created:      2025-10-25 14:32:17
Total Archives:    63
Total Size:        46 KB
Status:            Complete (catalog present)
Location:          PROGDIR:Backups/Run_0007
```

**Behavior:**
- Updates when list selection changes
- Shows "(No run selected)" when list empty or nothing selected
- Validates catalog.txt exists for "Complete" status

### 3.5 Preview Checkbox (GID_RESTORE_PREVIEW = 2005) - REMOVED

**Note:** Preview checkbox removed from design due to GadTools limitations.

**Reason:** 
- Standard LISTVIEW_KIND does not support multi-select or checkboxes
- Folder preview window would be read-only list (no selective restore)
- Adds complexity without selective restore capability
- "View Folders" button provides folder preview functionality

**Simplified UX:**
- User selects run from list
- Clicks "View Folders" to see what's in the run (optional)
- Clicks "Restore Run" to restore everything
- Clear and straightforward workflow

### 3.6 Restore Run Button (GID_RESTORE_RUN_BTN = 2006)

**Type:** BUTTON_KIND Gadget  
**Label:** `"Restore Run"`  
**Position:** **CALCULATED** - Bottom row, centered  
**Size:** **CALCULATED** from font dimensions

**Font-Based Sizing:**
```c
current_y += details_height + RESTORE_SPACE_Y;

/* Calculate button row positioning */
UWORD button_spacing = RESTORE_SPACE_X;
UWORD restore_btn_width = font_width * 15;  /* "Restore Run" */
UWORD view_btn_width = font_width * 16;     /* "View Folders..." */
UWORD cancel_btn_width = font_width * 10;   /* "Cancel" */

UWORD total_button_width = restore_btn_width + button_spacing +
                           view_btn_width + button_spacing +
                           cancel_btn_width;

/* Center buttons in window */
UWORD button_start_x = (window_max_width - total_button_width) / 2;

ng.ng_LeftEdge = button_start_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = restore_btn_width;
ng.ng_Height = button_height;             /* font_height + 6 */
ng.ng_GadgetText = "Restore Run";
ng.ng_GadgetID = GID_RESTORE_RUN_BTN;
ng.ng_Flags = PLACETEXT_IN;

restore_data->restore_run_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
    GA_Disabled, TRUE,  /* Initially disabled until run selected */
    TAG_END);
```  
**Behavior:**
- Disabled when no run selected
- Enabled when valid run selected
- Click action:
  1. Validates selected run exists
  2. Checks for catalog.txt
  3. Shows confirmation requester: "Restore all folders from Run_NNNN?"
  4. If confirmed: Calls `RestoreFullRun()` immediately
  5. Shows progress window during restore
  6. Shows success/error message when complete

### 3.7 View Folders Button (GID_RESTORE_VIEW_FOLDERS = 2007)

**Type:** BUTTON_KIND Gadget  
**Label:** `"View Folders..."`  
**Position:** **CALCULATED** - Bottom row, after Restore Run button  
**Size:** **CALCULATED** from font dimensions

**Font-Based Sizing:**
```c
ng.ng_LeftEdge = restore_data->restore_run_btn->LeftEdge + 
                 restore_data->restore_run_btn->Width + button_spacing;
ng.ng_TopEdge = current_y;
ng.ng_Width = view_btn_width;
ng.ng_Height = button_height;
ng.ng_GadgetText = "View Folders...";
ng.ng_GadgetID = GID_RESTORE_VIEW_FOLDERS;
ng.ng_Flags = PLACETEXT_IN;

restore_data->view_folders_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng,
    GA_Disabled, TRUE,  /* Initially disabled until run selected */
    TAG_END);
```  
**Behavior:**
- Disabled when no run selected or no catalog
- Enabled when valid run with catalog.txt selected
- Click action:
  1. Opens secondary window showing folder list (read-only)
  2. User can review what will be restored
  3. Window is informational only (no selective restore)
  4. User closes window, returns to main restore window
  5. See Section 5 for folder view window spec

### 3.8 Cancel Button (GID_RESTORE_CANCEL = 2008)

**Type:** BUTTON_KIND Gadget  
**Label:** `"Cancel"`  
**Position:** **CALCULATED** - Bottom row, after View Folders button  
**Size:** **CALCULATED** from font dimensions

**Font-Based Sizing:**
```c
ng.ng_LeftEdge = restore_data->view_folders_btn->LeftEdge + 
                 restore_data->view_folders_btn->Width + button_spacing;
ng.ng_TopEdge = current_y;
ng.ng_Width = cancel_btn_width;
ng.ng_Height = button_height;
ng.ng_GadgetText = "Cancel";
ng.ng_GadgetID = GID_RESTORE_CANCEL;
ng.ng_Flags = PLACETEXT_IN;

restore_data->cancel_btn = gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

/* Update window dimensions with button row */
update_window_max_dimensions(&window_max_width, &window_max_height,
                            ng.ng_LeftEdge + ng.ng_Width,
                            ng.ng_TopEdge + ng.ng_Height);
```  
**Behavior:**
- Always enabled
- Closes restore window
- No restore operations performed
- Returns to main window

---

## GadTools Implementation Notes

### Window Size Calculation

**⚠️ CRITICAL: Calculate AFTER All Gadgets Created**

Window dimensions must be calculated using actual gadget dimensions, especially the ListView which snaps to row boundaries.

```c
/* Calculate final window size */
UWORD final_window_width = window_max_width + RESTORE_MARGIN_RIGHT;
UWORD final_window_height = current_y + button_height + RESTORE_MARGIN_BOTTOM;

/* Open window with calculated dimensions */
restore_data->window = OpenWindowTags(NULL,
    WA_Left, (screen->Width - final_window_width) / 2,  /* Center */
    WA_Top, (screen->Height - final_window_height) / 2,
    WA_Width, final_window_width,
    WA_Height, final_window_height,
    WA_Title, "iTidy - Restore Backups",
    WA_Flags, WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | 
              WFLG_ACTIVATE | WFLG_RMBTRAP,
    WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_GADGETUP,
    WA_PubScreen, screen,
    TAG_END);
```

### Helper Function: update_window_max_dimensions()

```c
static void update_window_max_dimensions(UWORD current_max_width,
                                        UWORD current_max_height,
                                        UWORD gadget_right,
                                        UWORD gadget_bottom,
                                        UWORD *new_max_width,
                                        UWORD *new_max_height)
{
    if (gadget_right > current_max_width)
        *new_max_width = gadget_right;
    else
        *new_max_width = current_max_width;
    
    if (gadget_bottom > current_max_height)
        *new_max_height = gadget_bottom;
    else
        *new_max_height = current_max_height;
}
```

### Creating the ListView (with actual height tracking)

```c
/* Example ListView creation with GadTools */
struct NewGadget ng;
struct Gadget *gad;

/* List of strings for the ListView */
STRPTR *run_list_strings;  /* Array of formatted strings */
UWORD run_count;            /* Number of entries */

/* Request desired height */
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = font_width * 65;
ng.ng_Height = (font_height + 2) * 10;  /* Request 10 rows */
ng.ng_GadgetText = "Select Backup Run:";
ng.ng_GadgetID = GID_RESTORE_RUN_LIST;
ng.ng_Flags = PLACETEXT_ABOVE;

restore_data->run_list = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
    GTLV_Labels, run_list_strings,
    GTLV_Selected, 0,               /* Initially select first item */
    GTLV_ShowSelected, NULL,        /* Highlight selection */
    TAG_END);

/* CRITICAL: Get actual height and use it for positioning */
UWORD actual_height = gad->Height;  /* NOT ng.ng_Height! */
UWORD listview_bottom_y = ng.ng_TopEdge + actual_height;

/* Position next gadgets using actual bottom */
current_y = listview_bottom_y + RESTORE_SPACE_Y;
```

### Handling ListView Events

```c
case GID_RESTORE_RUN_LIST:
{
    /* Get selected index - Safe for ListView gadgets */
    LONG selected = -1;
    GT_GetGadgetAttrs(restore_data->run_list, restore_data->window, NULL,
        GTLV_Selected, &selected,
        TAG_END);
    
    if (selected >= 0 && selected < restore_data->run_count)
    {
        restore_data->selected_run_index = selected;
        update_details_panel(restore_data, 
                           &restore_data->run_entries[selected]);
        
        /* Enable/disable buttons based on selection */
        GT_SetGadgetAttrs(restore_data->restore_run_btn, 
                         restore_data->window, NULL,
                         GA_Disabled, FALSE,
                         TAG_END);
        
        GT_SetGadgetAttrs(restore_data->view_folders_btn,
                         restore_data->window, NULL,
                         GA_Disabled, 
                         !restore_data->run_entries[selected].hasCatalog,
                         TAG_END);
    }
    break;
}
```

### Handling Button Events

Button events are straightforward - no special handling required:

```c
case GID_RESTORE_RUN_BTN:
{
    if (restore_data->selected_run_index >= 0)
    {
        perform_restore_run(restore_data,
            &restore_data->run_entries[restore_data->selected_run_index]);
    }
    break;
}

case GID_RESTORE_VIEW_FOLDERS:
{
    if (restore_data->selected_run_index >= 0)
    {
        open_folder_preview_window(restore_data,
            &restore_data->run_entries[restore_data->selected_run_index]);
    }
    break;
}

case GID_RESTORE_CANCEL:
{
    return FALSE;  /* Close window */
}
```

### Formatted String Creation

```c
void format_run_list_entry(struct RestoreRunEntry *entry, char *buffer)
{
    /* Fixed-width format for alignment */
    /* "Run_0007  2025-10-25 14:32   63 folders   46 KB  Complete" */
    
    sprintf(buffer, "%-9s  %.16s  %3lu folders  %8s  %s",
        entry->runName,           /* "Run_0007" */
        entry->dateStr,           /* "2025-10-25 14:32:17" -> first 16 chars */
        entry->folderCount,       /* 63 */
        entry->sizeStr,           /* "46 KB" */
        entry->statusStr);        /* "Complete" */
}
```

---

## 4. Window Behavior & Event Handling

### 4.1 Window Opening Sequence

```c
BOOL open_restore_window(struct iTidyRestoreWindow *restore_data)
{
    1. Open window on Workbench screen
    2. Create GadTools visual info
    3. Create gadget list (all gadgets)
    4. Attach gadgets to window
    5. Initialize backup path to "PROGDIR:Backups"
    6. Scan for backup runs (call scan_backup_runs())
    7. Populate list view with runs
    8. Select most recent run (if any)
    9. Update details panel
    10. Enable/disable buttons based on selection
    11. Return TRUE on success
}
```

### 4.2 Event Loop

```c
BOOL handle_restore_window_events(struct iTidyRestoreWindow *restore_data)
{
    while (running) {
        Wait on window IDCMP port
        
        switch (event->Class) {
            case IDCMP_CLOSEWINDOW:
                return FALSE; /* Exit loop */
            
            case IDCMP_GADGETUP:
                switch (gadget_id) {
                    case GID_RESTORE_BACKUP_PATH:
                        on_backup_path_changed();
                        break;
                    case GID_RESTORE_CHANGE_PATH:
                        on_change_path_button();
                        break;
                    case GID_RESTORE_RUN_LIST:
                        on_run_list_selection_changed();
                        break;
                    case GID_RESTORE_RUN_BTN:
                        on_restore_run_button();
                        break;
                    case GID_RESTORE_VIEW_FOLDERS:
                        on_view_folders_button();
                        break;
                    case GID_RESTORE_CANCEL:
                        return FALSE; /* Close window */
                }
                break;
            
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(window);
                GT_EndRefresh(window, TRUE);
                break;
        }
    }
}
```

### 4.3 Key Functions

#### Scan Backup Runs
```c
ULONG scan_backup_runs(const char *backup_root, 
                       struct RestoreRunEntry **entries_out)
{
    1. Use FindHighestRunNumber(backup_root) to get max run
    2. For each Run_NNNN from 1 to max:
       a. Check if directory exists
       b. Check for catalog.txt
       c. Parse catalog for folder count and size
       d. Determine status (Complete/Incomplete/Orphaned)
       e. Add to entries array
    3. Sort by run number (descending)
    4. Return count of runs found
}
```

#### Populate List View
```c
void populate_run_list(struct iTidyRestoreWindow *restore_data,
                       struct RestoreRunEntry *entries,
                       ULONG count)
{
    1. Clear existing list view
    2. For each entry:
       a. Format display string with columns
       b. Add to list view gadget
    3. Select first entry if any
    4. Update details panel
    5. Refresh window
}
```

#### Update Details Panel
```c
void update_details_panel(struct iTidyRestoreWindow *restore_data,
                          struct RestoreRunEntry *selected_entry)
{
    if (selected_entry == NULL) {
        Display "(No run selected)"
        return;
    }
    
    1. Format run number
    2. Format date created
    3. Format total archives
    4. Format total size
    5. Format status with explanation:
       - "Complete (catalog present)"
       - "Incomplete (missing archives)"
       - "Orphaned (no catalog)"
    6. Format full path
    7. Update all display gadgets/text
}
```

#### Restore Run Operation
```c
void on_restore_run_button(struct iTidyRestoreWindow *restore_data)
{
    1. Get selected run entry
    2. Validate run directory exists
    3. If preview enabled:
       a. Open folder preview window
       b. Wait for user confirmation
    4. Open progress window
    5. Call RestoreFullRun(runNumber, backupRoot, "C:LhA")
    6. Update progress during restore
    7. On completion:
       a. Close progress window
       b. Show success/error message
       c. Update run list (refresh status)
}
```

---

## 5. Folder Preview Window (Secondary Window)

### Purpose
When "View Folders..." button is clicked, opens a secondary window showing all folders in the selected backup run, allowing selective restore.

### Window Properties

| Property | Value |
|----------|-------|
| Title | `"Backup Run 0007 - Folder List"` |
| Width | 550 pixels |
| Height | 400 pixels |
| Type | Modal (blocks restore window) |

### Layout

```
┌─────────────────────────────────────────────────────────────────┐
│ Backup Run 0007 - Folder List                        [_][■][×]  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Run: 0007   Date: 2025-10-25 14:32:17   Archives: 63          │
│                                                                 │
│  Folders in this backup:                                        │
│  ┌────────────────────────────────────────────────────────────┐│
│  │ 00001   23 KB   Workbench:Prefs/                           ││
│  │ 00002    5 KB   Workbench:Prefs/Presets/                   ││
│  │ 00003   15 KB   Work:Documents/                             ││
│  │ 00004    8 KB   Work:Development/                           ││
│  │ 00005   12 KB   DH0:Projects/ClientWork/                    ││
│  │ ...                                                          ││
│  │                                                              ││
│  │                                                              ││
│  │                                                              ││
│  └────────────────────────────────────────────────────────────┘│
│                                                                 │
│  Note: Standard LISTVIEW_KIND - no checkboxes supported.       │
│        Restore operates on ALL folders in the selected run.    │
│                                                                 │
│      [ Restore Selected ]  [ Close ]                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Gadgets

**GID_FOLDER_LIST (3001):** LISTVIEW_KIND - Shows all folders (read-only list)  
**GID_FOLDER_RESTORE_ALL_BTN (3002):** Button - "Restore All Folders"  
**GID_FOLDER_CLOSE_BTN (3003):** Button - "Close"  

### Behavior

**Important Note:** Standard GadTools LISTVIEW_KIND does **not support**:
- Multi-selection
- Checkboxes
- Column headers

**Simplified Workflow:**
1. **Load Folders:** Parse catalog.txt for selected run
2. **Display:** Show all archive entries in ListView (read-only)
3. **Restore All:** User reviews list, clicks "Restore All Folders" button
4. **Restore Operation:**
   - Calls `RestoreFullRun(runNumber, backupRoot, "C:LhA")`
   - Restores ALL folders in the run
   - Shows progress window
   - Reports results

**Alternative for Selective Restore (Future Enhancement):**
- Would require custom gadget or MUI/ReAction for multi-select
- For now: All-or-nothing restore per run
- Individual archive restore possible via CLI or future ReAction interface

---

## 6. Progress Window (Tertiary Window)

### Purpose
Shows restore operation progress with real-time updates.

### Window Properties

| Property | Value |
|----------|-------|
| Title | `"Restoring Backup..."` |
| Width | 400 pixels |
| Height | 150 pixels |
| Type | Modal (blocks all interaction) |
| Flags | No close gadget (must complete) |

### Layout

```
┌─────────────────────────────────────────────────────────┐
│ Restoring Backup...                                     │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Restoring: Workbench:Prefs/                            │
│  Archive: 00015.lha                                     │
│                                                         │
│  Progress: 15 of 63 archives (24%)                      │
│  ████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░       │
│                                                         │
│  Files restored: 125                                    │
│  Bytes restored: 1.2 MB                                 │
│  Errors: 0                                              │
│                                                         │
│                  [ Abort ]                              │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Update Frequency
- Update after each archive restored (~1 second intervals)
- Don't refresh more than 10 times/second to avoid slowdown

---

## 7. Data Structures

### Main Window Data Structure

```c
struct iTidyRestoreWindow
{
    struct Screen *screen;              /* Workbench screen */
    struct Window *window;              /* Restore window */
    APTR visual_info;                   /* GadTools visual info */
    struct Gadget *glist;               /* Gadget list */
    BOOL window_open;                   /* Window state flag */
    
    /* Gadget pointers */
    struct Gadget *backup_path_str;
    struct Gadget *change_path_btn;
    struct Gadget *run_list;
    struct Gadget *preview_check;
    struct Gadget *restore_run_btn;
    struct Gadget *view_folders_btn;
    struct Gadget *cancel_btn;
    
    /* Details panel gadgets (or IntuiText) */
    struct Gadget *details_panel;       /* Container or text area */
    
    /* Current state */
    char backup_root_path[256];         /* Current backup location */
    struct RestoreRunEntry *run_entries; /* Array of runs */
    ULONG run_count;                    /* Number of runs found */
    LONG selected_run_index;            /* Currently selected (-1 if none) */
    BOOL preview_enabled;               /* Preview checkbox state */
    
    /* Result */
    BOOL restore_performed;             /* TRUE if any restore done */
};
```

### Run Entry Structure

```c
struct RestoreRunEntry
{
    UWORD runNumber;                    /* Run number (1-9999) */
    char runName[16];                   /* "Run_0007" */
    char dateStr[24];                   /* "2025-10-25 14:32:17" */
    ULONG folderCount;                  /* Number of archives */
    ULONG totalBytes;                   /* Total size in bytes */
    char sizeStr[16];                   /* "46 KB" formatted */
    UWORD statusCode;                   /* Status enum */
    char statusStr[16];                 /* "Complete" text */
    char fullPath[256];                 /* Full path to run */
    BOOL hasCatalog;                    /* TRUE if catalog.txt exists */
};
```

### Status Codes

```c
typedef enum {
    RESTORE_STATUS_COMPLETE = 0,        /* Has catalog, all archives present */
    RESTORE_STATUS_INCOMPLETE = 1,      /* Has catalog, missing archives */
    RESTORE_STATUS_ORPHANED = 2,        /* No catalog.txt */
    RESTORE_STATUS_CORRUPTED = 3        /* Catalog parse error */
} RestoreStatus;
```

---

## 8. Function Prototypes (restore_window.h)

```c
/**
 * @brief Open the iTidy Restore Window
 * 
 * Opens a modal window for browsing and restoring backups.
 * 
 * @param restore_data Pointer to restore window data structure
 * @return BOOL TRUE if successful, FALSE otherwise
 */
BOOL open_restore_window(struct iTidyRestoreWindow *restore_data);

/**
 * @brief Close the Restore Window and cleanup resources
 * 
 * @param restore_data Pointer to restore window data structure
 */
void close_restore_window(struct iTidyRestoreWindow *restore_data);

/**
 * @brief Handle restore window events (main event loop)
 * 
 * @param restore_data Pointer to restore window data structure
 * @return BOOL TRUE to continue, FALSE to close window
 */
BOOL handle_restore_window_events(struct iTidyRestoreWindow *restore_data);

/**
 * @brief Scan backup directory for available runs
 * 
 * @param backup_root Path to backup root directory
 * @param entries_out Pointer to receive allocated array
 * @return ULONG Number of runs found
 */
ULONG scan_backup_runs(const char *backup_root, 
                       struct RestoreRunEntry **entries_out);

/**
 * @brief Populate list view with backup run entries
 * 
 * @param restore_data Pointer to restore window data
 * @param entries Array of run entries
 * @param count Number of entries
 */
void populate_run_list(struct iTidyRestoreWindow *restore_data,
                       struct RestoreRunEntry *entries,
                       ULONG count);

/**
 * @brief Update details panel with selected run information
 * 
 * @param restore_data Pointer to restore window data
 * @param selected_entry Selected run entry (or NULL)
 */
void update_details_panel(struct iTidyRestoreWindow *restore_data,
                          struct RestoreRunEntry *selected_entry);

/**
 * @brief Open folder preview window for selective restore
 * 
 * @param restore_data Pointer to restore window data
 * @param run_entry Selected backup run
 * @return BOOL TRUE if restore performed, FALSE if cancelled
 */
BOOL open_folder_preview_window(struct iTidyRestoreWindow *restore_data,
                                struct RestoreRunEntry *run_entry);

/**
 * @brief Perform full run restore with progress feedback
 * 
 * @param restore_data Pointer to restore window data
 * @param run_entry Backup run to restore
 * @return BOOL TRUE if successful, FALSE on error
 */
BOOL perform_restore_run(struct iTidyRestoreWindow *restore_data,
                        struct RestoreRunEntry *run_entry);
```

---

## 9. Integration with Main Window

### Adding "Restore Backups" Button to main_window.c

**Gadget Definition:**
```c
#define GID_RESTORE_BTN  17  /* After GID_CANCEL */

/* In gadget creation */
struct Gadget *restore_btn = CreateButtonGadget(
    GID_RESTORE_BTN,
    380, 315,  /* Position below Advanced button */
    120, 25,   /* Size */
    "Restore Backups...",
    prev_gadget
);
```

**Event Handler:**
```c
case GID_RESTORE_BTN:
{
    struct iTidyRestoreWindow restore_data;
    memset(&restore_data, 0, sizeof(restore_data));
    
    if (open_restore_window(&restore_data))
    {
        handle_restore_window_events(&restore_data);
        close_restore_window(&restore_data);
    }
    break;
}
```

---

## 10. Error Handling

### User-Facing Errors

1. **No Backup Runs Found:**
   - Show message: "No backup runs found in [path]"
   - Disable Restore Run and View Folders buttons
   - Allow user to change path

2. **Missing Catalog:**
   - Status shows "Orphaned (no catalog)"
   - Disable View Folders button
   - Restore Run attempts recovery using `RecoverOrphanedArchive()`

3. **Restore Failure:**
   - Show error dialog with specific reason:
     - "Archive file not found"
     - "LHA extraction failed"
     - "Disk full"
   - Continue with remaining archives
   - Report success/failure count at end

4. **Invalid Path:**
   - Validate backup path before scanning
   - Show error: "Path does not exist: [path]"
   - Keep previous valid path

### Internal Error Handling

```c
/* All restore operations return status codes */
typedef enum {
    RESTORE_OK = 0,
    RESTORE_ARCHIVE_NOT_FOUND,
    RESTORE_LHA_FAILED,
    RESTORE_DISK_FULL,
    RESTORE_PERMISSION_DENIED,
    RESTORE_CORRUPTED_ARCHIVE
} RestoreErrorCode;

/* Convert to user-friendly messages */
const char* get_restore_error_message(RestoreErrorCode code);
```

---

## 11. Testing Plan

### Unit Tests
1. **Scan Backup Runs:**
   - Empty directory → 0 runs found
   - Multiple runs → Correct count and sorting
   - Missing catalogs → Status marked as Orphaned

2. **List View Population:**
   - 0 entries → Empty list, no selection
   - Many entries → Scrollbar appears, first selected

3. **Details Update:**
   - No selection → "(No run selected)"
   - Valid selection → All fields populated correctly

### Integration Tests
1. Open restore window from main window
2. Select run and restore
3. Verify progress window appears
4. Verify files restored to original locations
5. Close all windows cleanly

### Amiga Hardware Tests
1. Test on Workbench 3.0, 3.1, 3.2
2. Test with 0, 1, 10, 100+ backup runs
3. Test with large catalogs (1000+ folders)
4. Test with missing/corrupted files
5. Verify memory cleanup (no leaks)

---

## 12. Future Enhancements (Post-MVP)

1. **Search/Filter:**
   - Filter runs by date range
   - Search for specific folders in catalog

2. **Sortable Columns:**
   - Click column header to sort list

3. **Backup Management:**
   - Delete old backup runs
   - Export/archive backup runs

4. **Restore Preview:**
   - Show diff before restore (what will change)
   - Dry-run mode

5. **Batch Operations:**
   - Restore multiple runs at once
   - Merge runs

---

## 13. Implementation Checklist

- [ ] Create `restore_window.h` with data structures and prototypes
- [ ] Create `restore_window.c` with window opening/closing
- [ ] Implement `scan_backup_runs()` function
- [ ] Implement list view population
- [ ] Implement details panel update
- [ ] Implement restore run operation
- [ ] Create folder preview window
- [ ] Create progress window
- [ ] Add "Restore Backups" button to main window
- [ ] Add event handler in main window
- [ ] Test on WinUAE
- [ ] Test on real Amiga hardware
- [ ] Create user documentation

---

## 14. Files to Create/Modify

### New Files
- `src/GUI/restore_window.h` (header)
- `src/GUI/restore_window.c` (implementation)
- `src/GUI/folder_preview_window.h` (optional - can be in restore_window.h)
- `src/GUI/folder_preview_window.c` (optional - can be in restore_window.c)
- `src/GUI/progress_window.h` (optional - can be in restore_window.h)
- `src/GUI/progress_window.c` (optional - can be in restore_window.c)

### Modified Files
- `src/GUI/main_window.h` (add GID_RESTORE_BTN constant)
- `src/GUI/main_window.c` (add button gadget and event handler)
- `Makefile` (add new object files to build)

### Estimated Lines of Code
- `restore_window.h`: ~150 lines
- `restore_window.c`: ~800 lines (main window)
- `folder_preview_window.c`: ~400 lines (optional separate file)
- `progress_window.c`: ~200 lines (optional separate file)
- **Total: ~1,550 lines**

---

## 15. Dependencies (Already Complete)

✅ Task 1: `backup_catalog.h/c` - Catalog parsing  
✅ Task 2: `backup_runs.h/c` - Run management  
✅ Task 3: `backup_paths.h/c` - Path utilities  
✅ Task 8: `backup_restore.h/c` - Restore operations  

All required APIs are implemented and tested!

---

**Document Status:** Complete and ready for implementation  
**Next Step:** Begin implementation of `restore_window.h` and `restore_window.c`

