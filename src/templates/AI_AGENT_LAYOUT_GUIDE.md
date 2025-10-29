# AI Agent Layout Guide for Amiga Window Template

This document provides critical guidance for AI agents working with the Amiga window template. These patterns were discovered through extensive debugging and must be followed exactly to avoid layout issues.

## ⚠️ CRITICAL: Font Selection for Column-Based Layouts

**If your ListView displays data in columns** (tabular data, logs, file listings):
- **DO NOT** use the screen font directly - it may be proportional (Helvetica, etc.)
- **USE** System Default Text font (`GfxBase->DefaultFont`) - typically fixed-width
- See **Section 0.1** below for complete implementation details
- See **Section 0.2** for colon-aligned details panel formatting pattern

**Why:** Proportional fonts have variable character widths ('i' ≠ 'W'), breaking space-aligned columns. System Default Text is user-configurable in Workbench Preferences and is typically Topaz (fixed-width).

## 🎯 Pattern Selection Decision Tree

```
START: What kind of window are you creating?
│
├─► Simple Dialog (Yes/No, OK/Cancel)
│   └─► Use SMALL window (40 chars)
│       └─► 2-button EQUAL_BUTTON_ROW
│       └─► Optional: Text display or message
│
├─► List Management / File Selection
│   └─► Use MEDIUM window (60-65 chars)
│       ├─► ⚠️ COLUMNAR DATA? → Use System Default Font (Section 0.1)
│       ├─► Need file/path input?
│       │   └─► Add PATTERN_INPUT_ROW at top
│       ├─► Add PATTERN_REFERENCE_CONTENT (ListView)
│       └─► 3-button EQUAL_BUTTON_ROW at bottom
│
├─► Complex Form / Multi-Panel Layout
│   └─► Use LARGE window (80 chars)
│       ├─► Multiple inputs?
│       │   └─► Stack multiple PATTERN_INPUT_ROW
│       ├─► Add PATTERN_REFERENCE_CONTENT (wide content)
│       └─► 4-button EQUAL_BUTTON_ROW at bottom
│
└─► Custom Requirements
    └─► Choose closest standard size
        └─► Adapt patterns to fit content
        └─► Maintain pre-calculation strategy
```

## 📐 STANDARD WINDOW SIZES AND LAYOUT PATTERNS

### Standard Window Size Classifications

iTidy uses three standard window size classifications based on reference width:

| Size   | Reference Width | Button Row | Typical Use Cases                    |
|--------|----------------|------------|--------------------------------------|
| Small  | 40 characters  | 2 buttons  | Simple dialogs, confirmations        |
| Medium | 60 characters  | 3 buttons  | Main windows, list management        |
| Large  | 80 characters  | 4 buttons  | Complex forms, multi-panel layouts   |

**Reference Width** = The width of your widest gadget (usually a ListView), measured in character widths.

### Standard Layout Patterns

#### PATTERN_INPUT_ROW: Label + Input + Action Button
```
[Label Text:]  [Input Field ──────────]  [Action Button ───────────]
└─TEXT────────┘└─STRING/NUMBER────────┘ └─Extends to reference─────┘
```

**When to use:** File paths, search fields, any input requiring a browse/change action.

**Structure:**
- **Label**: Separate TEXT gadget, width measured with `TextLength()`
- **Input**: STRING_KIND or NUMBER_KIND, fixed logical width
- **Action**: BUTTON_KIND, pre-calculated to extend to reference width

**Key measurements:**
```c
label_width = TextLength(&temp_rp, label_text, strlen(label_text));
input_width = font_width * logical_chars;  /* 20, 35, or 50 */
action_btn_width = precalc_max_right - (input_right + SPACE_X);
```

#### PATTERN_REFERENCE_CONTENT: Main Content Gadget
```
[Main Content Area (ListView, Text Display, etc.) ──────────────]
└─ This establishes precalc_max_right for entire window ───────┘
```

**When to use:** Every window needs one reference gadget.

**Common types:** LISTVIEW_KIND, MX_KIND (tall), SCROLLER_KIND area

**Purpose:** Defines the maximum right edge that all other gadgets align to.

#### PATTERN_EQUAL_BUTTON_ROW: N Equal-Width Buttons
```
[Button 1 ─────]  [Button 2 ─────]  [Button 3 ─────]
└─────────── All buttons same width, span reference_width ──────┘
```

**When to use:** Action buttons at bottom of window.

**Button count guide:**
- Small windows: 2 buttons (OK/Cancel)
- Medium windows: 3 buttons (Action1/Action2/Cancel)
- Large windows: 4 buttons (multiple actions + Cancel)

**Formula:**
```c
equal_width = (reference_width - (button_count - 1) * SPACE_X) / button_count;
if (equal_width < max_btn_text_width + BUTTON_TEXT_PADDING)
    equal_width = max_btn_text_width + BUTTON_TEXT_PADDING;
```

### Standard Measurements and Constants

#### Spacing Constants (to be defined in your header)
```c
#define WINDOW_MARGIN_LEFT    10
#define WINDOW_MARGIN_TOP     10
#define WINDOW_MARGIN_RIGHT   10
#define WINDOW_MARGIN_BOTTOM  10
#define WINDOW_SPACE_X        8   /* Horizontal gap between gadgets */
#define WINDOW_SPACE_Y        8   /* Vertical gap between rows */
#define BUTTON_TEXT_PADDING   8   /* Padding around button text */
```

**IMPORTANT:** `BUTTON_TEXT_PADDING` should be used consistently everywhere for button width calculations:
```c
button_width = TextLength(&temp_rp, "Button Text", 11) + BUTTON_TEXT_PADDING;
```

#### Logical Input Widths (in characters)
```c
#define INPUT_WIDTH_SMALL   20   /* Dates, numbers, short text */
#define INPUT_WIDTH_MEDIUM  35   /* Filenames, paths (restore window uses this) */
#define INPUT_WIDTH_LARGE   50   /* Long paths, descriptions */
```

#### Standard Window Widths (in characters)
```c
#define WINDOW_WIDTH_SMALL   40   /* Simple dialogs */
#define WINDOW_WIDTH_MEDIUM  60   /* Main windows (restore window uses 65) */
#define WINDOW_WIDTH_LARGE   80   /* Complex layouts */
```

#### Height Calculations
```c
button_height = font_height + 6;
string_height = font_height + 6;
listview_line_height = font_height + 2;
checkbox_height = font_height + 4;
```

### Complete Window Structure Template

```
┌────────────────────────────────────────────────────────────┐
│ Window Title Bar                                      [×]  │ ← currentWindowBarHeight
├────────────────────────────────────────────────────────────┤
│ ← MARGIN_LEFT (10px)                                       │
│                                                             │
│   [Label:]  [Input Field ──]  [Action Btn ──────────────] │ ← PATTERN_INPUT_ROW
│             └─35 chars────┘    └─Extends to ref width──┘  │   (SPACE_Y below)
│                                                             │
│   [Reference Content Area ──────────────────────────────]  │ ← PATTERN_REFERENCE_CONTENT
│   [                                                      ]  │   (Usually ListView)
│   [           60-65 characters wide                      ]  │
│   [                                                      ]  │
│   [──────────────────────────────────────────────────────]  │
│                 └─precalc_max_right─────────────────────┘  │
│                                                             │
│   [Optional: Details Panel or Secondary Content]           │
│                                                             │
│   [Button 1 ────]  [Button 2 ────]  [Button 3 ────]       │ ← PATTERN_EQUAL_BUTTON_ROW
│   └────────────────────────────────────────────────┘       │   (3 buttons for medium)
│      ← current_x              precalc_max_right →          │
│                                         MARGIN_RIGHT (10)→ │
│                                                             │
└────────────────────────────────────────────────────────────┘
  ← MARGIN_BOTTOM (10px)
```

## 📋 APPLYING STANDARD PATTERNS: Step-by-Step Guide

### Step 1: Choose Window Size
Determine which standard size fits your content needs:
- **Small (40 chars)**: Simple yes/no dialogs, single input forms
- **Medium (60 chars)**: List management, file selection, restore window
- **Large (80 chars)**: Multi-column layouts, complex preferences

### Step 2: Pre-Calculate Layout Dimensions

**This section is MANDATORY and must come before creating any gadgets.**

```c
/*--------------------------------------------------------------------*/
/* PRE-CALCULATE LAYOUT DIMENSIONS                                   */
/*--------------------------------------------------------------------*/
/* 1. Establish reference width based on window size */
UWORD reference_width;
if (window_size == SIZE_SMALL)
    reference_width = font_width * 40;
else if (window_size == SIZE_MEDIUM)
    reference_width = font_width * 60;  /* or 65 like restore window */
else  /* SIZE_LARGE */
    reference_width = font_width * 80;

UWORD precalc_max_right = current_x + reference_width;

/* 2. Pre-calculate INPUT_ROW pattern dimensions (if using) */
STRPTR label_text = "Backup Location:";
UWORD label_width = TextLength(&temp_rp, label_text, strlen(label_text));
UWORD label_spacing = 4;

UWORD input_width = font_width * INPUT_WIDTH_MEDIUM;  /* Use standard constant */
UWORD input_left = current_x + label_width + label_spacing;
UWORD input_right = input_left + input_width;

UWORD action_btn_left = input_right + WINDOW_SPACE_X;
UWORD action_btn_width = precalc_max_right - action_btn_left;

append_to_log("=== PRE-CALCULATED LAYOUT ===\n");
append_to_log("Reference width: %d, max_right: %d\n", reference_width, precalc_max_right);
append_to_log("Action button: left=%d, width=%d\n", action_btn_left, action_btn_width);

/* 3. Pre-calculate EQUAL_BUTTON_ROW dimensions */
UWORD available_width = reference_width;
UWORD button_count = 3;  /* 2 for small, 3 for medium, 4 for large */

/* Find maximum button text width using TextLength() */
UWORD max_btn_text_width = TextLength(&temp_rp, "First Button", 12);
UWORD temp_width = TextLength(&temp_rp, "Second Button", 13);
if (temp_width > max_btn_text_width)
    max_btn_text_width = temp_width;
temp_width = TextLength(&temp_rp, "Cancel", 6);
if (temp_width > max_btn_text_width)
    max_btn_text_width = temp_width;

/* Calculate equal button width */
UWORD equal_button_width = (available_width - ((button_count - 1) * WINDOW_SPACE_X)) / button_count;

/* Ensure buttons are wide enough for text + standard padding */
if (equal_button_width < max_btn_text_width + BUTTON_TEXT_PADDING)
    equal_button_width = max_btn_text_width + BUTTON_TEXT_PADDING;

append_to_log("Button row: available=%d, count=%d, equal_width=%d\n",
              available_width, button_count, equal_button_width);
```

### Step 3: Create Gadgets Using Pre-Calculated Values

#### Pattern: INPUT_ROW (if needed)
```c
/* Label gadget */
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = label_width;  /* Pre-calculated with TextLength() */
ng.ng_Height = string_height;
ng.ng_GadgetText = label_text;
ng.ng_GadgetID = GID_LABEL;
ng.ng_Flags = PLACETEXT_IN;
label_gad = CreateGadget(TEXT_KIND, gad, &ng,
    GTTX_Text, label_text,
    GTTX_Border, FALSE,
    TAG_END);

/* Input gadget */
ng.ng_LeftEdge = input_left;  /* Pre-calculated */
ng.ng_TopEdge = current_y;
ng.ng_Width = input_width;  /* Pre-calculated */
ng.ng_Height = string_height;
ng.ng_GadgetText = NULL;
ng.ng_GadgetID = GID_INPUT;
ng.ng_Flags = 0;
input_gad = CreateGadget(STRING_KIND, gad, &ng,
    GTST_String, "",
    GTST_MaxChars, 255,
    TAG_END);

/* Action button (extends to reference width) */
ng.ng_LeftEdge = action_btn_left;  /* Pre-calculated */
ng.ng_TopEdge = current_y;
ng.ng_Width = action_btn_width;  /* Pre-calculated to extend right */
ng.ng_Height = button_height;
ng.ng_GadgetText = "Change";
ng.ng_GadgetID = GID_ACTION;
ng.ng_Flags = PLACETEXT_IN;
action_btn = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

current_y += string_height + WINDOW_SPACE_Y;
```

#### Pattern: REFERENCE_CONTENT
```c
/* Main ListView (establishes reference width) */
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = reference_width;  /* Pre-calculated */
ng.ng_Height = (font_height + 2) * 10;  /* 10 visible lines */
ng.ng_GadgetText = "Items:";
ng.ng_GadgetID = GID_LISTVIEW;
ng.ng_Flags = PLACETEXT_ABOVE;
listview_gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
    GTLV_Labels, NULL,
    GTLV_Selected, ~0,
    TAG_END);

/* CRITICAL: Get actual height after creation */
UWORD actual_listview_height = listview_gad->Height;
current_y = listview_gad->TopEdge + actual_listview_height + WINDOW_SPACE_Y;
```

#### Pattern: EQUAL_BUTTON_ROW
```c
/* Button 1 */
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = equal_button_width;  /* Pre-calculated */
ng.ng_Height = button_height;
ng.ng_GadgetText = "First Button";
ng.ng_GadgetID = GID_BUTTON1;
ng.ng_Flags = PLACETEXT_IN;
button1 = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

/* Button 2 */
ng.ng_LeftEdge = current_x + equal_button_width + WINDOW_SPACE_X;
ng.ng_Width = equal_button_width;  /* Same width */
ng.ng_GadgetText = "Second Button";
ng.ng_GadgetID = GID_BUTTON2;
button2 = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

/* Button 3 (for medium/large windows) */
ng.ng_LeftEdge = current_x + (2 * equal_button_width) + (2 * WINDOW_SPACE_X);
ng.ng_Width = equal_button_width;  /* Same width */
ng.ng_GadgetText = "Cancel";
ng.ng_GadgetID = GID_CANCEL;
button3 = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

/* For 4-button row (large windows), add: */
/* ng.ng_LeftEdge = current_x + (3 * equal_button_width) + (3 * WINDOW_SPACE_X); */
```

### Step 4: Calculate Final Window Size
```c
/* Use precalc_max_right as width base, add borders and margins */
UWORD final_window_width = precalc_max_right + 
                           prefsIControl.currentLeftBarWidth + 
                           WINDOW_MARGIN_RIGHT;

UWORD final_window_height = current_y + button_height + WINDOW_MARGIN_BOTTOM;
```

### Step 5: Open Window
```c
window = OpenWindowTags(NULL,
    WA_Left, (screen->Width - final_window_width) / 2,
    WA_Top, (screen->Height - final_window_height) / 2,
    WA_Width, final_window_width,
    WA_Height, final_window_height,
    WA_Title, "Window Title",
    WA_DragBar, TRUE,
    WA_DepthGadget, TRUE,
    WA_CloseGadget, TRUE,
    WA_Activate, TRUE,
    WA_PubScreen, screen,
    WA_Gadgets, glist,
    WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_GADGETUP,
    TAG_END);
```

### Pattern Combination Examples

#### Small Window (40 chars, 2 buttons)
```
┌──────────────────────────────────────┐
│ Confirm Action                   [×] │
├──────────────────────────────────────┤
│  Are you sure you want to proceed?  │
│                                      │
│     [  OK  ]        [ Cancel ]      │
└──────────────────────────────────────┘
```
- No INPUT_ROW pattern
- Optional text/message content
- 2-button EQUAL_BUTTON_ROW

#### Medium Window (60 chars, 3 buttons)
```
┌────────────────────────────────────────────────────────┐
│ iTidy - Restore Backups                            [×] │
├────────────────────────────────────────────────────────┤
│  Backup Location:  [PROGDIR:Backups]  [Change──────]  │ ← INPUT_ROW
│                                                        │
│  [Run_0008  2025-10-28 10:09  127 folders    543 KB] │
│  [Run_0007  2025-10-28 10:08    1 folders     11 KB] │ ← REFERENCE_CONTENT
│  [Run_0006  2025-10-27 09:23    1 folders     10 KB] │   (ListView)
│                                                        │
│  [Restore Run]   [View Folders...]   [Cancel]        │ ← EQUAL_BUTTON_ROW
└────────────────────────────────────────────────────────┘
```
- INPUT_ROW pattern at top
- REFERENCE_CONTENT (ListView)
- 3-button EQUAL_BUTTON_ROW

#### Large Window (80 chars, 4 buttons)
```
┌──────────────────────────────────────────────────────────────────────────┐
│ Preferences - Advanced                                               [×] │
├──────────────────────────────────────────────────────────────────────────┤
│  Output Path:  [Work:iTidy/Output─────────────]  [Browse─────────────]  │
│                                                                          │
│  [Category List ────────────]  [Available Options ──────────────────]   │
│  [                          ]  [                                     ]   │
│  [                          ]  [                                     ]   │
│                                                                          │
│  [ Apply ]   [ Save ]   [ Restore Defaults ]   [ Cancel ]              │
└──────────────────────────────────────────────────────────────────────────┘
```
- INPUT_ROW pattern at top
- Two-column REFERENCE_CONTENT
- 4-button EQUAL_BUTTON_ROW

## ⚠️ CRITICAL PATTERNS - DO NOT DEVIATE! ⚠️

### 0. Font Access and Text Measurement (AmigaOS 3.0 Best Practice)

**THE PROBLEM:** Using `screen->RastPort.Font` directly and `strlen() * font_width` for measurements is inaccurate and doesn't follow the AmigaOS Style Guide.

**WRONG WAY (old method):**
```c
// NEVER DO THIS - bypasses proper font system
font = screen->RastPort.Font;
label_width = strlen(label) * font->tf_XSize;  // Inaccurate for proportional fonts
```

**CORRECT WAY (AmigaOS 3.0 Style Guide compliant):**
```c
// ALWAYS DO THIS - use DrawInfo and TextLength()
struct DrawInfo *draw_info;
struct RastPort temp_rp;
struct TextFont *font;

draw_info = GetScreenDrawInfo(screen);
if (draw_info == NULL) {
    // Handle error
}

font = draw_info->dri_Font;
font_width = font->tf_XSize;
font_height = font->tf_YSize;

// Initialize RastPort for text measurements
InitRastPort(&temp_rp);
SetFont(&temp_rp, font);

// Measure label width accurately
label_width = TextLength(&temp_rp, "Label:", strlen("Label:"));

// ... create gadgets ...

// Clean up
FreeScreenDrawInfo(screen, draw_info);
```

**KEY RULES:**
- Always use `GetScreenDrawInfo()` to get the screen's DrawInfo
- Access font via `draw_info->dri_Font`, not `screen->RastPort.Font`
- Use `TextLength()` for accurate text width measurements
- Initialize a RastPort with `InitRastPort()` and `SetFont()` for measurements
- Always call `FreeScreenDrawInfo()` when done (both success and error paths)
- For button text, use `TextLength(rp, text, strlen(text)) + padding`

### 0.1. Using System Default Font for Column-Based Layouts ⚠️ CRITICAL

**THE PROBLEM:** If you display data in **space-aligned columns** in a ListView (like tabular data), the Screen Text font might be **proportional** (e.g., Helvetica), which breaks column alignment. Proportional fonts have variable character widths ('i' is narrower than 'W'), making character-based spacing impossible.

**Example of broken alignment with proportional font:**
```
Run_0007  2025-10-25 14:32  63 folders   46 KB  Complete
Run_0008  2025-10-28 10:09  127 folders  543 KB Complete
  ^^^^       ^^^^^^               ^^^       ^^^
  Columns don't line up - each character has different width!
```

**SOLUTION:** Use the **System Default Text** font (from Workbench Preferences) for gadgets displaying columnar data. This font is typically fixed-width (monospaced) like Topaz.

**Workbench has THREE font settings:**
1. **Workbench Icon Text** - For icon labels
2. **System Default Text** - For system gadgets (usually Topaz - FIXED WIDTH)
3. **Screen Text** - For window titles, menus (can be proportional)

**When Screen Text is proportional, use System Default Text instead:**

```c
struct TextFont *system_font = NULL;
struct TextAttr system_font_attr;

/* Get screen font first */
draw_info = GetScreenDrawInfo(screen);
font = draw_info->dri_Font;
font_width = font->tf_XSize;
font_height = font->tf_YSize;

/* Check if screen font is proportional */
if (font->tf_Flags & FPF_PROPORTIONAL)
{
    append_to_log("WARNING: Screen uses proportional font - using system default instead\n");
}

/* Open System Default Text font (GfxBase->DefaultFont)
 * This is the font defined in Workbench Preferences as "System Default Text"
 * It's typically Topaz (fixed-width) regardless of Screen Text setting
 */
system_font_attr.ta_Name = (STRPTR)GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name;
system_font_attr.ta_YSize = GfxBase->DefaultFont->tf_YSize;
system_font_attr.ta_Style = FS_NORMAL;
system_font_attr.ta_Flags = 0;

system_font = OpenFont(&system_font_attr);
if (system_font != NULL)
{
    /* Use system font metrics for layout calculations */
    font = system_font;
    font_width = font->tf_XSize;
    font_height = font->tf_YSize;
    
    /* Store in your data structure for cleanup */
    window_data->system_font = system_font;
}
else
{
    append_to_log("WARNING: Could not open system font, using screen font\n");
    system_font = NULL;
}

/* Initialize NewGadget to use system font */
ng.ng_TextAttr = (system_font != NULL) ? &system_font_attr : screen->Font;

/* ... create gadgets ... */

/* CLEANUP: Close the font when done */
if (window_data->system_font != NULL)
{
    CloseFont(window_data->system_font);
    window_data->system_font = NULL;
}
```

**When to use System Default Font:**
- ✅ **ListViews with columnar data** (logs, file lists, backup runs, etc.)
- ✅ **Any space-aligned text** that needs to line up in columns
- ✅ **Tabular displays** where you use character-based formatting like `"%-10s  %-20s  %8s"`
- ✅ **Colon-aligned details panels** (see Section 0.2 for formatting pattern)
- ❌ **Not needed** for simple text labels or single-column lists
- ❌ **Not needed** if you don't care about column alignment

**Column alignment example (requires fixed-width font):**
```c
/* Format entry with character-based column widths */
sprintf(buffer, "%-9s  %-16s  %-11s  %8s  %s",
    run_name,      /* Left-align in 9 chars */
    date_time,     /* Left-align in 16 chars */
    folder_count,  /* Left-align in 11 chars */
    size_str,      /* Right-align in 8 chars */
    status);       /* Remaining space */

/* This ONLY works if every character is the same width (fixed-width font) */
/* With proportional fonts, columns will be misaligned */
```

**Required structure field:**
```c
struct MyWindowData
{
    struct TextFont *system_font;  /* Store opened system font for cleanup */
    /* ... other fields ... */
};
```

**KEY RULES:**
- Check if screen font is proportional with `font->tf_Flags & FPF_PROPORTIONAL`
- Open system default font via `GfxBase->DefaultFont` for column-based layouts
- Set `ng.ng_TextAttr` to use system font for all gadgets
- Store font pointer in your window structure
- Always `CloseFont()` in cleanup (both error and normal paths)
- System Default Text is user-configurable in Workbench Preferences
- This ensures columns align properly even if user sets proportional Screen Text

### 0.2. Colon-Aligned Label-Value Formatting (Details Display Pattern)

**THE PATTERN:** When displaying detailed information in a ListView (like properties, details panels, or info displays), use **colon-aligned formatting** where labels are right-justified to a colon, and values are left-aligned starting at a consistent position.

**Visual Example:**
```
      Run Number: 0008
    Date Created: 2025-10-28 10:09:27
Source Directory: PC:Workbench
  Total Archives: 127
      Total Size: 543 KB
          Status: Complete (catalog present)
        Location: PROGDIR:Backups/Run_0008
```

**Why this looks better:**
- ✅ Creates a clean vertical alignment at the colon
- ✅ Easy to scan - eye follows the colon line down
- ✅ Professional appearance like system preference panels
- ✅ Clearly separates labels from values
- ✅ Works perfectly with fixed-width fonts

**Implementation (requires fixed-width font):**

```c
/* Step 1: Find the longest label to determine alignment width */
const char *labels[] = {
    "Run Number:",
    "Date Created:",
    "Source Directory:",
    "Total Archives:",
    "Total Size:",
    "Status:",
    "Location:"
};

/* Find maximum label length */
int max_label_len = 0;
for (int i = 0; i < label_count; i++)
{
    int len = strlen(labels[i]);
    if (len > max_label_len)
        max_label_len = len;
}

/* Step 2: Format each line with right-aligned label + left-aligned value */
char line_buffer[256];
sprintf(line_buffer, "%*s %s",
    max_label_len,           /* Width for right-alignment */
    "Run Number:",           /* Label (right-justified in field) */
    "0008");                 /* Value (starts right after colon + space) */

/* Result: "      Run Number: 0008" */
/*          └─right aligned─┘ └─value */

/* Example for all fields: */
sprintf(line_buffer, "%*s %s", max_label_len, "Run Number:", run_number_str);
sprintf(line_buffer, "%*s %s", max_label_len, "Date Created:", date_str);
sprintf(line_buffer, "%*s %s", max_label_len, "Source Directory:", source_path);
sprintf(line_buffer, "%*s %d", max_label_len, "Total Archives:", archive_count);
sprintf(line_buffer, "%*s %s", max_label_len, "Total Size:", size_str);
sprintf(line_buffer, "%*s %s", max_label_len, "Status:", status_str);
sprintf(line_buffer, "%*s %s", max_label_len, "Location:", location_path);
```

**Format Specifier Breakdown:**
- `%*s` - Variable-width string field (width specified as parameter)
- First parameter (`max_label_len`) - Total width for the label field
- Second parameter (label string) - The label text to right-justify
- The label is padded with spaces on the LEFT to reach `max_label_len`
- Space after `%*s` creates the gap between colon and value
- Value is printed normally after the space

**Real-World Example (restore window details panel):**
```c
/* From populate_run_list() - formatting details for selected backup run */

/* Find longest label */
int max_label_len = 0;
const char *labels[] = {
    "Run Number:",
    "Date Created:", 
    "Source Directory:",
    "Total Archives:",
    "Total Size:",
    "Status:",
    "Location:"
};
for (int i = 0; i < 7; i++)
{
    int len = strlen(labels[i]);
    if (len > max_label_len)
        max_label_len = len;
}

/* Format each detail line */
char detail_lines[7][256];
sprintf(detail_lines[0], "%*s %d", max_label_len, "Run Number:", entry->runNumber);
sprintf(detail_lines[1], "%*s %s", max_label_len, "Date Created:", entry->dateStr);
sprintf(detail_lines[2], "%*s %s", max_label_len, "Source Directory:", entry->sourceDirectory);
sprintf(detail_lines[3], "%*s %lu", max_label_len, "Total Archives:", entry->folderCount);
sprintf(detail_lines[4], "%*s %s", max_label_len, "Total Size:", entry->sizeStr);
sprintf(detail_lines[5], "%*s %s", max_label_len, "Status:", entry->statusStr);
sprintf(detail_lines[6], "%*s %s", max_label_len, "Location:", entry->fullPath);

/* Add to ListView */
for (int i = 0; i < 7; i++)
{
    /* Add detail_lines[i] to ListView as node->ln_Name */
}
```

**When to use Colon-Aligned Formatting:**
- ✅ **Details panels** showing properties of a selected item
- ✅ **Information displays** with label-value pairs
- ✅ **About/Info windows** with system information
- ✅ **Property sheets** with configuration details
- ✅ **Status displays** showing multiple fields
- ✅ **Read-only data** that users need to scan quickly

**When NOT to use:**
- ❌ **Editable forms** (use normal input gadgets)
- ❌ **Tabular data** with multiple columns (use column formatting from Section 0.1)
- ❌ **Single-column lists** without label-value pairs
- ❌ **Short labels** that don't benefit from alignment (just use "Label: Value")

**KEY RULES:**
- **REQUIRES** fixed-width font (use System Default Font from Section 0.1)
- Calculate maximum label width at runtime (don't hardcode)
- Use `%*s` format specifier for right-justified labels
- Always include ONE space between colon and value
- Keep ListView read-only (`GTLV_ReadOnly, TRUE`) for info displays
- Ensure longest label + space + longest value fits in ListView width
- Consider line wrapping for very long values (paths, etc.)

**Alternative: Pre-calculated Width**
If you know your labels at compile time, you can hardcode the width:
```c
/* If longest label is "Source Directory:" (17 chars including colon) */
#define DETAIL_LABEL_WIDTH 17

sprintf(buffer, "%*s %s", DETAIL_LABEL_WIDTH, "Run Number:", value);
sprintf(buffer, "%*s %s", DETAIL_LABEL_WIDTH, "Total Size:", value);
/* etc. */
```

This formatting pattern creates professional, easy-to-read information displays that match the aesthetic of Amiga system preference panels and file requesters.

### 1. ListView Height "Snapping" Issue

**THE PROBLEM:** ListView gadgets automatically adjust their height to show complete rows. The height you request is NOT the height you get.

**WRONG WAY (causes overlap):**
```c
// NEVER DO THIS - using requested height for positioning
ng.ng_Height = font_height * 10;  // Request 10 lines
listview = CreateGadget(LISTVIEW_KIND, ...);
button_y = listview_y + ng.ng_Height;  // WRONG! Uses requested, not actual
```

**CORRECT WAY:**
```c
// ALWAYS DO THIS - use actual height after creation
ng.ng_Height = font_height * 10;  // Request 10 lines
listview = CreateGadget(LISTVIEW_KIND, ...);
UWORD actual_height = listview->Height;  // Get ACTUAL height
button_y = listview->TopEdge + actual_height + spacing;  // Use actual dimensions
```

**KEY RULES:**
- Create ListView gadgets FIRST
- Always use `gadget->Height` for actual dimensions
- DO NOT use `GT_GetGadgetAttrs()` for basic geometry (unreliable)
- Position other gadgets based on actual ListView dimensions

### 2. PLACETEXT_LEFT Label Positioning

**THE PROBLEM:** Labels for `PLACETEXT_LEFT` gadgets are drawn to the LEFT of the specified position, causing overlap with adjacent gadgets.

**WRONG WAY (causes label overlap):**
```c
// NEVER DO THIS - places gadget without accounting for label space
ng.ng_LeftEdge = base_x;  // WRONG! Label will overlap whatever is to the left
ng.ng_GadgetText = "Input:";
ng.ng_Flags = PLACETEXT_LEFT;
```

**BEST PRACTICE - Separate TEXT Gadget (RECOMMENDED):**

When you have multiple gadgets on the same row (e.g., label + input + button), using `PLACETEXT_LEFT` becomes tricky because it's hard to calculate exact alignment with other gadgets like ListViews below. The **separate TEXT gadget technique** solves this perfectly:

```c
// Step 1: Create separate TEXT gadget for the label
STRPTR label_text = "Backup Location:";
UWORD label_width = TextLength(&temp_rp, label_text, strlen(label_text));
UWORD label_spacing = 4;

ng.ng_LeftEdge = current_x;  // Left-aligned with ListView below
ng.ng_TopEdge = current_y;
ng.ng_Width = label_width;
ng.ng_Height = string_height;
ng.ng_GadgetText = label_text;
ng.ng_GadgetID = GID_LABEL;
ng.ng_Flags = PLACETEXT_IN;

label_gadget = CreateGadget(TEXT_KIND, gad, &ng,
    GTTX_Text, label_text,
    GTTX_Border, FALSE,
    TAG_END);

// Step 2: Create input gadget WITHOUT label (ng.ng_GadgetText = NULL)
ng.ng_LeftEdge = current_x + label_width + label_spacing;
ng.ng_TopEdge = current_y;
ng.ng_Width = font_width * INPUT_WIDTH_MEDIUM;
ng.ng_Height = string_height;
ng.ng_GadgetText = NULL;  // No label!
ng.ng_GadgetID = GID_INPUT;
ng.ng_Flags = 0;  // No PLACETEXT flag

input_gadget = CreateGadget(STRING_KIND, gad, &ng,
    GTST_String, "",
    GTST_MaxChars, 255,
    TAG_END);

// Step 3: Create action button (extends to reference width)
UWORD input_right = ng.ng_LeftEdge + ng.ng_Width;
ng.ng_LeftEdge = input_right + WINDOW_SPACE_X;
ng.ng_Width = precalc_max_right - ng.ng_LeftEdge;  // Extends to right
ng.ng_GadgetText = "Change";
ng.ng_GadgetID = GID_ACTION;
ng.ng_Flags = PLACETEXT_IN;

button_gadget = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
```

**WHY THIS IS BETTER:**
- ✅ Label is perfectly left-aligned with ListView and other gadgets
- ✅ No complex calculations for PLACETEXT_LEFT positioning
- ✅ Easy to see exact positions in code
- ✅ Works perfectly when multiple gadgets are on same row
- ✅ More maintainable and debuggable

**WHEN TO USE SEPARATE TEXT GADGETS:**
- Multiple gadgets on same horizontal row (label + input + button)
- When label needs to align with gadgets below (e.g., ListView)
- When you have "Change", "Browse", or other action buttons next to inputs
- Any INPUT_ROW pattern implementation

**ALTERNATIVE - PLACETEXT_LEFT (for simple cases):**

For simple single-gadget rows, you can still use PLACETEXT_LEFT:

```c
// ONLY for simple cases - adjust position to account for label
STRPTR label = "Input:";
UWORD label_width = TextLength(&temp_rp, label, strlen(label));
UWORD label_spacing = 4;
ng.ng_LeftEdge = base_x + label_width + label_spacing;  // Adjusted position
ng.ng_GadgetText = label;
ng.ng_Flags = PLACETEXT_LEFT;

// Remember to free: FreeScreenDrawInfo(screen, draw_info);
```

**KEY RULES:**
- **PREFERRED FOR INPUT_ROW:** Use separate TEXT gadget + label-less input gadget
- **ACCEPTABLE FOR SIMPLE ROWS:** Use `PLACETEXT_LEFT` with adjusted position
- Always use `TextLength()` for accurate label width measurements
- Always adjust gadget position: `gadget_x = base_x + label_width + spacing`
- Include label widths in window size calculations
- Test with different font sizes to verify spacing

### 3. Window Size Calculation

**THE PROBLEM:** Using calculated or requested dimensions instead of actual gadget dimensions results in windows that are too small or have poor spacing.

**WRONG WAY (causes truncation/overlap):**
```c
// NEVER DO THIS - using requested/calculated values
window_width = listview_requested_width + other_stuff;
window_height = listview_requested_height + other_stuff;
```

**CORRECT WAY:**
```c
// ALWAYS DO THIS - use actual gadget dimensions after creation
UWORD actual_listview_height = listview_gadget->Height;  // ACTUAL height

// Measure label width accurately using TextLength()
struct RastPort temp_rp;
InitRastPort(&temp_rp);
SetFont(&temp_rp, draw_info->dri_Font);
UWORD string_label_width = TextLength(&temp_rp, "Input:", 6) + 4;  // Include spacing

window_width = listview_gadget->Width + gap + string_label_width + 
               string_gadget->Width + margins + padding;
window_height = top_margin + font_height + actual_listview_height + 
                spacing + button_height + bottom_margin;
```

**KEY RULES:**
- Calculate size AFTER all gadgets are created
- Use actual gadget dimensions (`gadget->Width`, `gadget->Height`)
- **PREFERRED:** Use `TextLength()` for measuring label widths
- **FALLBACK:** Use `strlen() * font_width` for monospaced fonts only
- Include ALL label widths for `PLACETEXT_LEFT` gadgets
- Add appropriate margins, spacing, and borders
- Test with different fonts to ensure proper scaling

### 4. Gadget Creation Order

**THE PROBLEM:** Creating gadgets in the wrong order prevents you from using actual dimensions for positioning calculations.

**CORRECT ORDER:**
1. Create ListView gadgets first (to discover actual height)
2. Create other gadgets based on ListView actual dimensions
3. Calculate window size using all actual gadget dimensions
4. Open window with calculated size

### 5. Reliable Gadget Geometry Access

**THE PROBLEM:** Using `GT_GetGadgetAttrs()` for basic geometry can return 0 or incorrect values.

**RELIABLE METHOD:**
```c
// Use direct gadget structure access for position/size
UWORD x = gadget->LeftEdge;
UWORD y = gadget->TopEdge;
UWORD w = gadget->Width;
UWORD h = gadget->Height;
```

**UNRELIABLE METHOD:**
```c
// DON'T use GT_GetGadgetAttrs for basic geometry
GT_GetGadgetAttrs(gadget, window, NULL, GA_Width, &w, TAG_END);  // Can fail!
```

## Debugging Layout Issues

Always use the `debug_print_gadget_positions()` function to verify:
- ListView actual height matches your calculations
- String gadget labels don't overlap with adjacent gadgets  
- Button positioning accounts for actual ListView height
- Window size properly contains all gadgets with margins

## Testing Checklist

Before considering layout work complete:
- [ ] Test with Topaz 8 font (default)
- [ ] Test with Topaz 9 font (common alternative)  
- [ ] Test with larger fonts if available
- [ ] Verify no gadget overlap in any font configuration
- [ ] Check that window size accommodates all content
- [ ] Ensure labels are not truncated or overlapping
- [ ] Verify proper spacing between all elements

## 6. Pre-Calculated Layout Strategy (RECOMMENDED)

**THE BEST APPROACH:** Pre-calculate all gadget dimensions BEFORE creating any gadgets. This ensures buttons and gadgets receive their final dimensions during creation, not via post-modification.

**WHY IT MATTERS:** GadTools gadgets cache their dimensions during `CreateGadget()`. Modifying gadget structure fields after creation (like `gadget->Width = new_value`) does NOT trigger a visual update. The gadget renders with its original creation dimensions.

**RECOMMENDED PATTERN:**
```c
/*--------------------------------------------------------------------*/
/* PRE-CALCULATE LAYOUT DIMENSIONS                                   */
/*--------------------------------------------------------------------*/
// Establish the reference width (usually the widest gadget)
UWORD listview_width = font_width * 65;
UWORD string_gadget_width = font_width * 35;

// Calculate the maximum right edge before creating any gadgets
UWORD precalc_max_right = current_x + listview_width;

// Pre-calculate label dimensions using TextLength()
STRPTR label = "Backup Location:";
UWORD label_width = TextLength(&temp_rp, label, strlen(label));
UWORD label_spacing = 4;

// Pre-calculate exact button dimensions
UWORD string_left = current_x + label_width + label_spacing;
UWORD string_right = string_left + string_gadget_width;
UWORD change_btn_left = string_right + RESTORE_SPACE_X;
UWORD change_btn_width = precalc_max_right - change_btn_left;  // Fill to right edge

append_to_log("=== PRE-CALCULATED LAYOUT ===\n");
append_to_log("Listview width: %d, max_right will be: %d\n", 
              listview_width, precalc_max_right);
append_to_log("Change button: left=%d, width=%d\n", 
              change_btn_left, change_btn_width);

/*--------------------------------------------------------------------*/
/* CREATE GADGETS WITH PRE-CALCULATED DIMENSIONS                     */
/*--------------------------------------------------------------------*/
// Now create all gadgets using the pre-calculated values
ng.ng_LeftEdge = change_btn_left;      // Use pre-calculated position
ng.ng_Width = change_btn_width;         // Use pre-calculated width
ng.ng_GadgetText = "Change";
restore_data->change_path_btn = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

// For equal-width button rows:
UWORD available_width = listview_width;

// Find maximum button text width using TextLength()
UWORD max_btn_text_width = TextLength(&temp_rp, "Restore Run", 11);
UWORD temp_width = TextLength(&temp_rp, "View Folders...", 15);
if (temp_width > max_btn_text_width)
    max_btn_text_width = temp_width;

// Calculate equal button width
UWORD equal_button_width = (available_width - (2 * spacing)) / 3;

// Ensure buttons are wide enough for text + padding
if (equal_button_width < max_btn_text_width + 8)
    equal_button_width = max_btn_text_width + 8;

// Create buttons with equal widths
ng.ng_LeftEdge = current_x;
ng.ng_Width = equal_button_width;
restore_btn = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

ng.ng_LeftEdge = current_x + equal_button_width + spacing;
ng.ng_Width = equal_button_width;
view_btn = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

ng.ng_LeftEdge = current_x + (2 * equal_button_width) + (2 * spacing);
ng.ng_Width = equal_button_width;
cancel_btn = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);
```

**KEY BENEFITS:**
- Gadgets render correctly because they receive final dimensions at creation time
- No need for post-creation modification (which doesn't work anyway)
- Easier to debug - all calculations are upfront and logged
- More maintainable - layout logic is centralized
- Font-sensitive - uses `TextLength()` for accurate measurements

**WHAT NOT TO DO (Post-Creation Modification):**
```c
// DON'T DO THIS - creates gadget first, then tries to modify
ng.ng_Width = font_width * 10;  // Initial guess
button = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

// This does NOT work - gadget still renders with original width!
button->Width = new_calculated_width;  // Visual update doesn't happen
```

**WHEN TO USE PRE-CALCULATION:**
- Buttons that should extend to the right edge of other gadgets
- Button rows with equal widths spanning the window
- Any layout where gadget sizes depend on other gadgets' positions
- Professional, polished layouts with proper alignment

**PORTABILITY CHECKLIST:**
- ✅ Works with any font size (uses `TextLength()`)
- ✅ Works with proportional and monospaced fonts
- ✅ Automatically adjusts to different screen DPI settings
- ✅ Respects IControl preferences for window borders
- ✅ Can be reused as template for new windows

## Common AI Agent Mistakes

### Layout and Dimension Errors
1. **Using calculated ListView height instead of actual height** - This is the #1 cause of button overlap
2. **Not accounting for PLACETEXT_LEFT label space** - This causes label overlap with adjacent gadgets
   - **SOLUTION:** Use separate TEXT gadget for label instead of PLACETEXT_LEFT for complex rows
3. **Creating gadgets in wrong order** - ListView must be first to get actual dimensions
4. **Forgetting label widths in window calculations** - Causes window to be too narrow
5. **Trying to modify gadget dimensions after creation** - Doesn't trigger visual update; use pre-calculation instead
6. **Using PLACETEXT_LEFT for INPUT_ROW pattern** - Hard to align with other gadgets; use separate TEXT gadget instead

### Font and Text Measurement Errors
6. **Using `screen->RastPort.Font` instead of `DrawInfo`** - Should use `GetScreenDrawInfo()` for proper font access
7. **Using `strlen() * font_width` for proportional fonts** - Use `TextLength()` for accurate measurements
8. **Not testing with different fonts** - Layout may work with one font but fail with others
9. **Inconsistent button text padding** - Always use `BUTTON_TEXT_PADDING` constant

### Standard Pattern Violations
10. **Not using standard window sizes** - Use 40/60/80 char widths, not arbitrary values
11. **Wrong button count for window size** - Small=2, Medium=3, Large=4 buttons
12. **Not using pre-calculation block** - All dimensions must be calculated before CreateGadget()
13. **Ignoring standard spacing constants** - Use `WINDOW_SPACE_X`, `WINDOW_SPACE_Y`, etc.
14. **Not following pattern structure** - INPUT_ROW → REFERENCE_CONTENT → EQUAL_BUTTON_ROW

### Resource Management Errors
15. **Using GT_GetGadgetAttrs() for geometry** - Direct structure access is more reliable
16. **Not freeing DrawInfo resources** - Always call `FreeScreenDrawInfo()` when done
17. **Memory leaks in error paths** - Ensure cleanup_error: label frees all resources

## 🚀 Quick Reference Checklist

### Before Starting Layout
- [ ] Choose window size: Small (40), Medium (60), or Large (80) chars
- [ ] Define button count: Small=2, Medium=3, Large=4
- [ ] Define all spacing constants: `WINDOW_MARGIN_*`, `WINDOW_SPACE_*`, `BUTTON_TEXT_PADDING`
- [ ] Have `GetScreenDrawInfo()` and `TextLength()` ready for font measurements

### Pre-Calculation Block (MANDATORY)
- [ ] Calculate `reference_width = font_width * size_constant`
- [ ] Calculate `precalc_max_right = current_x + reference_width`
- [ ] Pre-calculate INPUT_ROW dimensions (if using pattern)
- [ ] Pre-calculate EQUAL_BUTTON_ROW dimensions
- [ ] Add debug logging for all calculations

### Gadget Creation Order
1. [ ] Create INPUT_ROW pattern gadgets (Label → Input → Action button)
2. [ ] Create REFERENCE_CONTENT gadget (ListView, etc.)
3. [ ] Get actual height: `actual_height = gadget->Height`
4. [ ] Create secondary content gadgets
5. [ ] Create EQUAL_BUTTON_ROW pattern gadgets

### Before Opening Window
- [ ] Calculate final size using `precalc_max_right` and actual gadget heights
- [ ] Add all margins and borders
- [ ] Verify all gadgets created successfully
- [ ] Call `FreeScreenDrawInfo()` if not done already

### Testing Checklist
- [ ] Test with Topaz 8 font (default)
- [ ] Test with Topaz 9 font (common alternative)
- [ ] Test with larger fonts if available
- [ ] Verify no gadget overlap
- [ ] Check button alignment (left and right edges)
- [ ] Verify all buttons have equal widths
- [ ] Ensure labels are not truncated

## 📊 Standard Measurements Quick Reference

```c
/* Window Size Constants */
#define WINDOW_WIDTH_SMALL   40   /* chars */
#define WINDOW_WIDTH_MEDIUM  60   /* chars (restore window uses 65) */
#define WINDOW_WIDTH_LARGE   80   /* chars */

/* Input Field Constants */
#define INPUT_WIDTH_SMALL   20   /* chars */
#define INPUT_WIDTH_MEDIUM  35   /* chars */
#define INPUT_WIDTH_LARGE   50   /* chars */

/* Spacing Constants */
#define WINDOW_MARGIN_LEFT    10  /* pixels */
#define WINDOW_MARGIN_TOP     10  /* pixels */
#define WINDOW_MARGIN_RIGHT   10  /* pixels */
#define WINDOW_MARGIN_BOTTOM  10  /* pixels */
#define WINDOW_SPACE_X        8   /* horizontal gap */
#define WINDOW_SPACE_Y        8   /* vertical gap */
#define BUTTON_TEXT_PADDING   8   /* button text padding */

/* Height Calculations */
button_height = font_height + 6;
string_height = font_height + 6;
listview_line_height = font_height + 2;

/* Button Count by Window Size */
Small windows:  2 buttons (Action / Cancel)
Medium windows: 3 buttons (Action1 / Action2 / Cancel)
Large windows:  4 buttons (Action1 / Action2 / Action3 / Cancel)
```

## 📐 Common Calculation Formulas

```c
/* Equal-width button calculation */
equal_width = (reference_width - (button_count - 1) * WINDOW_SPACE_X) / button_count;
if (equal_width < max_text_width + BUTTON_TEXT_PADDING)
    equal_width = max_text_width + BUTTON_TEXT_PADDING;

/* Action button extending to reference width */
action_btn_width = precalc_max_right - (input_right + WINDOW_SPACE_X);

/* Final window width */
final_width = precalc_max_right + prefsIControl.currentLeftBarWidth + WINDOW_MARGIN_RIGHT;

/* Final window height */
final_height = current_y + last_gadget_height + WINDOW_MARGIN_BOTTOM;

/* Button positions for N-button row */
button[0] = current_x;
button[1] = current_x + equal_width + WINDOW_SPACE_X;
button[2] = current_x + (2 * equal_width) + (2 * WINDOW_SPACE_X);
button[3] = current_x + (3 * equal_width) + (3 * WINDOW_SPACE_X);
/* Pattern: current_x + (i * equal_width) + (i * WINDOW_SPACE_X) */
```

## Template Usage Pattern

When using this template:
1. Follow the documented gadget creation order exactly
2. Use the standard window sizes and button counts as guidelines
3. Always use pre-calculation before creating gadgets
4. Use standard spacing constants consistently
5. Always include debug output during development
6. Test thoroughly with multiple font configurations
7. Never "optimize" the layout calculations - they exist for good reasons

**Note:** The standard button counts (2/3/4) are guidelines for creating windows from scratch. Existing windows may have different requirements, and that's acceptable. The key is using the pre-calculation strategy and standard spacing.

Remember: These patterns exist because they solve real problems that occur in Amiga GadTools programming. Deviating from them will likely reintroduce the same bugs they were designed to prevent.
