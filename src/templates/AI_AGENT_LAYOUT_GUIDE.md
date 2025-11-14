# AI Agent Layout Guide for Amiga Window Template

This document provides critical guidance for AI agents working with the Amiga window template. These patterns were discovered through extensive debugging and must be followed exactly to avoid layout issues.

---

## 🚨 SECTION 0: GadTools Coordinate System (MANDATORY - READ FIRST!)

### The Fatal Mistake That Breaks All Layouts

**MISCONCEPTION:** "NewGadget coordinates start at the client area, so ng_TopEdge = 0 is below the title bar."

**REALITY:** GadTools uses **window-relative coordinates** where (0,0) is the **window's outer top-left corner** (the title bar itself). Positioning gadgets at ng_TopEdge = 0 or ng_TopEdge = 5 will cause them to **overlap the title bar**.

### The Only Correct Formula (From Amiga RKMs)

Calculate BorderTop **before** creating gadgets:

```c
struct Screen *screen = LockPubScreen("Workbench");

/* CRITICAL: This formula is NON-NEGOTIABLE */
WORD border_top    = screen->WBorTop + screen->Font->ta_YSize + 1;
WORD border_left   = screen->WBorLeft;
WORD border_right  = screen->WBorRight;
WORD border_bottom = screen->WBorBottom;

UnlockPubScreen(NULL, screen);
```

**Formula breakdown:**
- `WBorTop` = window's top border thickness (typically 2-4 pixels)
- `Font->ta_YSize` = screen font height used for title bar text
- `+ 1` = required spacing pixel per RKM specification
- **Result** = exact BorderTop that matches `window->BorderTop` after creation

### Correct Gadget Positioning

```c
/* Initialize positioning BELOW the title bar */
WORD margin = 5;
WORD current_y = border_top + margin;  /* NOT just margin! */
WORD current_x = border_left + margin;

/* Create gadgets with window-relative coordinates */
struct NewGadget ng;
ng.ng_LeftEdge = current_x;  /* Includes border_left */
ng.ng_TopEdge  = current_y;  /* Includes border_top */
ng.ng_Width    = gadget_width;
ng.ng_Height   = gadget_height;

gad = CreateGadget(BUTTON_KIND, gad, &ng, TAG_END);

/* Move to next gadget position */
current_y += gadget_height + spacing;
```

### Window Size Must Include Borders

```c
/* Calculate content area size (gadgets already offset by border_top) */
UWORD content_height = last_gadget_Y + gadget_height + bottom_margin;

/* Add borders for TOTAL window size */
UWORD window_height = content_height + border_top + border_bottom;

/* Open window with total dimensions */
window = OpenWindowTags(NULL,
    WA_Height, window_height,  /* Includes borders */
    TAG_END);
```

### Mandatory Verification

After opening the window, verify your calculation:

```c
if (window->BorderTop != border_top) {
    /* CRITICAL ERROR: Formula is wrong! */
    printf("BorderTop ERROR: Calculated=%d, Actual=%d\n",
           border_top, window->BorderTop);
}
```

**Expected result: ZERO difference.** Any difference means you're using the wrong formula.

### Common Fatal Errors

❌ **NEVER** use `ng_TopEdge = 0` or `ng_TopEdge = margin` alone  
❌ **NEVER** guess `border_top = 16` or use hardcoded values  
❌ **NEVER** use `font_height * 2` or other multipliers  
❌ **NEVER** use only `screen->WBorTop` (misses title font height!)  
❌ **NEVER** forget the `+ 1` in the formula  
❌ **NEVER** assume `BorderTop == BorderBottom` (they can differ!)

### Why This Formula is Universal

Adapts automatically to:
- Different fonts (Topaz 8, Helvetica 11, Times 15, etc.)
- IControl prefs (enlarged title bars in WB 3.2)
- All Workbench versions (3.0, 3.1, 3.2)
- Custom screen modes and resolutions
- Visual enhancements (NewLook, MagicWB)

**It reads actual screen metrics instead of assuming values.**

### Reference Implementation

See `amiga_window_template.c` → `calculate_font_dimensions()` for production code that achieves **zero-pixel positioning errors** in all tested configurations.

---

## ⚠️ CRITICAL: ListView Scroll Buttons Require IDCMP_GADGETDOWN

**If your window has a ListView gadget with scroll arrow buttons:**
- **MUST** include `IDCMP_GADGETDOWN` in the window's `WA_IDCMP` flags
- **MUST** handle `IDCMP_GADGETDOWN` events in your event loop
- **Without this:** Scroll arrows will be visible but non-functional (scrollbar will still work)
- See **Section 0** below for complete implementation details

**Why:** ListView scroll arrow buttons generate `IDCMP_GADGETDOWN` events, not `IDCMP_GADGETUP`. This is a GadTools requirement that is easy to forget.

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
/* Optional: Create label for ListView using separate TEXT gadget (RECOMMENDED) */
STRPTR listview_label = "Items:";
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = TextLength(&temp_rp, listview_label, strlen(listview_label));
ng.ng_Height = font_height;
ng.ng_GadgetText = listview_label;
ng.ng_GadgetID = GID_LISTVIEW_LABEL;
ng.ng_Flags = PLACETEXT_IN;
label_gad = CreateGadget(TEXT_KIND, gad, &ng,
    GTTX_Text, listview_label,
    GTTX_Border, FALSE,
    TAG_END);

current_y += font_height + 4;  /* Label height + small gap */

/* Main ListView (establishes reference width) */
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = reference_width;  /* Pre-calculated */
ng.ng_Height = (font_height + 2) * 10;  /* 10 visible lines */
ng.ng_GadgetText = "";  /* EMPTY LABEL - we created our own above! */
ng.ng_GadgetID = GID_LISTVIEW;
ng.ng_Flags = PLACETEXT_ABOVE;  /* Doesn't matter with empty string */
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

### 0. ListView Scroll Buttons Setup (CRITICAL)

**THE PROBLEM:** ListView gadgets have built-in scroll arrow buttons (up/down arrows) that appear automatically, but they won't work unless you configure the window to receive their events.

**SYMPTOMS:**
- Scroll arrows are visible on ListView
- Scrollbar works correctly
- Clicking scroll arrows does nothing - no response at all

**ROOT CAUSE:** ListView scroll arrow buttons generate `IDCMP_GADGETDOWN` events, NOT `IDCMP_GADGETUP`. If your window isn't listening for `IDCMP_GADGETDOWN`, the buttons will be non-functional.

**❌ WRONG WAY (scroll arrows won't work):**
```c
/* Window setup - MISSING IDCMP_GADGETDOWN */
folder_data->window = OpenWindowTags(NULL,
    WA_Title, "My Window",
    WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW,  /* Missing GADGETDOWN! */
    WA_Gadgets, folder_data->glist,
    TAG_DONE);

/* Event handler - no GADGETDOWN case */
while ((msg = GT_GetIMsg(window->UserPort)) != NULL)
{
    switch (msg->Class)
    {
        case IDCMP_GADGETUP:
            /* Handle button clicks */
            break;
        /* Missing IDCMP_GADGETDOWN case! */
    }
}
```

**✅ CORRECT WAY (scroll arrows work perfectly):**

**Step 1: Add IDCMP_GADGETDOWN to window IDCMP flags**
```c
/* Window setup - includes IDCMP_GADGETDOWN */
folder_data->window = OpenWindowTags(NULL,
    WA_Left, 100,
    WA_Top, 50,
    WA_Width, 400,
    WA_Height, 200,
    WA_Title, "Folder View",
    WA_DragBar, TRUE,
    WA_DepthGadget, TRUE,
    WA_CloseGadget, TRUE,
    WA_Activate, TRUE,
    WA_PubScreen, folder_data->screen,
    WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_REFRESHWINDOW,
    /*                                             ^^^^^^^^^^^^^^^^^^^  CRITICAL: Add this! */
    WA_Gadgets, folder_data->glist,
    TAG_DONE);
```

**Step 2: Handle IDCMP_GADGETDOWN events in your event loop**
```c
/* Event handler - handles ListView scroll button clicks */
BOOL handle_window_events(struct WindowData *data)
{
    struct IntuiMessage *msg;
    ULONG msg_class;
    UWORD gadget_id;
    
    while ((msg = GT_GetIMsg(data->window->UserPort)) != NULL)
    {
        msg_class = msg->Class;
        
        switch (msg_class)
        {
            case IDCMP_GADGETUP:
                gadget_id = ((struct Gadget *)msg->IAddress)->GadgetID;
                GT_ReplyIMsg(msg);
                
                /* Handle regular button clicks */
                if (gadget_id == GID_CLOSE_BTN)
                {
                    return FALSE;  /* Close window */
                }
                break;
                
            case IDCMP_GADGETDOWN:
                /* Handle ListView scroll arrow buttons */
                gadget_id = ((struct Gadget *)msg->IAddress)->GadgetID;
                GT_ReplyIMsg(msg);
                
                if (gadget_id == GID_MY_LISTVIEW)
                {
                    /* Get current top position (optional - for debugging) */
                    LONG current_top = 0;
                    GT_GetGadgetAttrs(data->listview, data->window, NULL,
                                      GTLV_Top, &current_top,
                                      TAG_END);
                    
                    append_to_log("ListView scroll button pressed, current top: %ld\n", current_top);
                    
                    /* GadTools handles the actual scrolling automatically.
                     * We just refresh the window to ensure proper display. */
                    GT_RefreshWindow(data->window, NULL);
                }
                break;
                
            case IDCMP_CLOSEWINDOW:
                GT_ReplyIMsg(msg);
                return FALSE;
                
            case IDCMP_REFRESHWINDOW:
                GT_ReplyIMsg(msg);
                GT_BeginRefresh(data->window);
                GT_EndRefresh(data->window, TRUE);
                break;
                
            default:
                GT_ReplyIMsg(msg);
                break;
        }
    }
    
    return TRUE;  /* Continue running */
}
```

**HOW IT WORKS:**
1. **ListView scroll arrows** (up/down buttons) generate `IDCMP_GADGETDOWN` events
2. These events have the **same GadgetID** as the ListView itself (not separate gadgets)
3. **GadTools handles the scrolling automatically** - you don't need to manually change `GTLV_Top`
4. You just need to:
   - Listen for `IDCMP_GADGETDOWN` in window IDCMP flags
   - Handle the event in your event loop
   - Optionally call `GT_RefreshWindow()` to ensure proper display

**KEY RULES:**
- ⚠️ **ALWAYS** include `IDCMP_GADGETDOWN` in WA_IDCMP for windows with ListView gadgets
- ⚠️ **ALWAYS** handle `IDCMP_GADGETDOWN` in your event loop
- ⚠️ **DO NOT** try to manually scroll by modifying `GTLV_Top` - GadTools does it automatically
- ⚠️ **REMEMBER** to call `GT_RefreshWindow()` after handling scroll events
- ✅ The scroll arrows and scrollbar both work with the same event handling
- ✅ This applies to ALL ListView gadgets, regardless of window size or content

**WHEN TO USE:**
- ✅ **Every window** with a ListView gadget
- ✅ **Even if** you think users will only use the scrollbar
- ✅ **Even if** your list is small - users expect scroll buttons to work

**COMMON MISTAKE:**
❌ Forgetting to add `IDCMP_GADGETDOWN` because the scrollbar works fine without it. The scrollbar uses different events (`IDCMP_MOUSEMOVE`, etc.), but the arrow buttons specifically need `IDCMP_GADGETDOWN`.

**REFERENCE:**
- See `folder_view_window.c` for a complete working example
- GadTools autodocs: "LISTVIEWIDCMP = IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_MOUSEMOVE"

### 0.1. ListView Label Text Positioning (CRITICAL)

**THE PROBLEM:** ListView gadgets with `PLACETEXT_ABOVE` labels add extra vertical space above the ListView that varies with font size. If you don't account for this, your layout calculations will be wrong and gadgets will overlap or have incorrect spacing.

**SYMPTOMS:**
- ListView appears lower than expected
- Gadgets above ListView overlap with the label
- Vertical spacing calculations don't match actual positions
- Layout breaks when user changes font size

**ROOT CAUSE:** When you use `ng.ng_GadgetText` with `PLACETEXT_ABOVE`, GadTools automatically adds vertical space above the ListView for the label text (typically `font_height + small_gap`). This space is NOT included in `ng.ng_Height` and must be accounted for separately.

**❌ WRONG WAY #1 (hardcoded offset - breaks with font changes):**
```c
/* BAD: Hardcoded offset for label space */
ng.ng_LeftEdge = MARGIN_LEFT;
ng.ng_TopEdge = MARGIN_TOP + 20;  /* WRONG! Hardcoded 20 pixels for label */
ng.ng_Width = 380;
ng.ng_Height = 120;
ng.ng_GadgetText = "Folders:";
ng.ng_GadgetID = GID_FOLDER_LIST;
ng.ng_Flags = PLACETEXT_ABOVE;

listview = CreateGadget(LISTVIEW_KIND, gad, &ng, TAG_END);

/* Position next gadget - but what's the right Y position? */
/* ng.ng_TopEdge + ng.ng_Height doesn't account for the label above! */
next_y = ng.ng_TopEdge + ng.ng_Height;  /* WRONG! Missing label space */
```

**❌ WRONG WAY #2 (using label with complex calculations):**
```c
/* BAD: Trying to calculate label space manually */
UWORD label_height = font_height + 4;  /* Guess at spacing */
ng.ng_TopEdge = current_y + label_height;  /* Manual offset */
ng.ng_GadgetText = "Items:";
ng.ng_Flags = PLACETEXT_ABOVE;

listview = CreateGadget(LISTVIEW_KIND, gad, &ng, TAG_END);

/* Now you need to remember the label space for all subsequent calculations */
current_y = ng.ng_TopEdge + listview->Height;  /* Still confusing! */
```

**✅ CORRECT WAY - Use Empty Label (RECOMMENDED):**

The **simplest and most reliable approach** is to use an **empty string** for `ng.ng_GadgetText` and create a separate TEXT gadget for the label. This gives you complete control over positioning and eliminates font-dependent spacing issues.

```c
/* Step 1: Create separate TEXT gadget for the label */
STRPTR listview_label = "Backup Runs:";
UWORD label_height = font_height;  /* Or measure with TextExtent if needed */

ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = TextLength(&temp_rp, listview_label, strlen(listview_label));
ng.ng_Height = label_height;
ng.ng_GadgetText = listview_label;
ng.ng_GadgetID = GID_LISTVIEW_LABEL;
ng.ng_Flags = PLACETEXT_IN;

label_gadget = CreateGadget(TEXT_KIND, gad, &ng,
    GTTX_Text, listview_label,
    GTTX_Border, FALSE,
    TAG_END);

/* Move down for the ListView - exact control over spacing */
current_y += label_height + 4;  /* Label + small gap */

/* Step 2: Create ListView with EMPTY label */
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = listview_width;
ng.ng_Height = (font_height + 2) * 10;  /* 10 visible lines */
ng.ng_GadgetText = "";  /* EMPTY STRING - no built-in label! */
ng.ng_GadgetID = GID_LISTVIEW;
ng.ng_Flags = PLACETEXT_ABOVE;  /* Doesn't matter with empty string */

listview = CreateGadget(LISTVIEW_KIND, gad, &ng,
    GTLV_Labels, NULL,
    GTLV_Selected, ~0,
    TAG_END);

/* Step 3: Position next gadget using actual ListView dimensions */
UWORD actual_listview_height = listview->Height;
current_y = listview->TopEdge + actual_listview_height + WINDOW_SPACE_Y;

/* Now position your next gadget - calculations are simple and accurate! */
ng.ng_TopEdge = current_y;
```

**ALTERNATIVE - Calculate Label Space (More Complex):**

If you must use `PLACETEXT_ABOVE` with a label, you need to account for the label space in all your calculations:

```c
/* Calculate label space (GadTools adds font_height + small gap above ListView) */
UWORD label_space = font_height + 4;  /* Approximate, may vary */

ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y + label_space;  /* Reserve space for label */
ng.ng_Width = listview_width;
ng.ng_Height = (font_height + 2) * 10;
ng.ng_GadgetText = "Items:";
ng.ng_GadgetID = GID_LISTVIEW;
ng.ng_Flags = PLACETEXT_ABOVE;

listview = CreateGadget(LISTVIEW_KIND, gad, &ng, TAG_END);

/* CRITICAL: Account for label space when calculating total height */
UWORD total_height = label_space + listview->Height;
current_y = ng.ng_TopEdge + listview->Height;  /* Base position + actual height */

/* OR use the gadget's TopEdge which already accounts for label */
current_y = listview->TopEdge + listview->Height + WINDOW_SPACE_Y;
```

**WHY EMPTY LABEL IS BETTER:**

| Aspect | Empty Label + TEXT Gadget | PLACETEXT_ABOVE with Label |
|--------|---------------------------|----------------------------|
| **Position Control** | ✅ Exact control, you position everything | ❌ GadTools adds mysterious spacing |
| **Font Changes** | ✅ You calculate spacing explicitly | ⚠️ Must guess at GadTools spacing |
| **Code Clarity** | ✅ Clear: `current_y += label_height + gap` | ❌ Confusing: Where does label space come from? |
| **Debugging** | ✅ Easy to see label gadget dimensions | ❌ Label space is invisible |
| **Flexibility** | ✅ Can style label differently (bold, colors) | ❌ Limited to default text rendering |

**REAL-WORLD EXAMPLE (restore_window.c):**

```c
/* From restore_window.c - uses empty label for precise control */
ng.ng_LeftEdge = current_x;
ng.ng_TopEdge = current_y;
ng.ng_Width = listview_width;
ng.ng_Height = listview_requested_height;
ng.ng_GadgetText = "";  /* Empty label - we manage our own! */
ng.ng_GadgetID = GID_RESTORE_RUN_LIST;
ng.ng_Flags = PLACETEXT_ABOVE;

restore_data->run_list = gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
    GTLV_Labels, NULL,
    GTLV_Selected, ~0,
    TAG_END);

/* Get actual height and position next gadget - simple and accurate */
actual_listview_height = gad->Height;
current_y = ng.ng_TopEdge + actual_listview_height + RESTORE_SPACE_Y;
```

**KEY RULES:**
- ⚠️ **RECOMMENDED:** Always use empty string `""` for `ng.ng_GadgetText` on ListView gadgets
- ⚠️ **RECOMMENDED:** Create separate TEXT gadget for the label above the ListView
- ⚠️ **NEVER** hardcode pixel offsets like `+ 20` for label space
- ⚠️ **ALWAYS** calculate label space dynamically using `font_height` if you must use PLACETEXT_ABOVE
- ✅ Empty labels make your code simpler, clearer, and more maintainable
- ✅ Separate TEXT gadgets give you full control over label positioning and styling
- ✅ This approach works consistently across all font sizes and screen modes

**WHEN TO USE EMPTY LABELS:**
- ✅ **All ListView gadgets** in production code
- ✅ When you need **precise vertical spacing** between gadgets
- ✅ When your layout uses **pre-calculated dimensions**
- ✅ When you want **font-independent** layout calculations

**WHEN PLACETEXT_ABOVE WITH LABEL IS ACCEPTABLE:**
- ⚠️ Quick prototypes or test code where precision doesn't matter
- ⚠️ Simple windows with only one ListView and no complex spacing
- ⚠️ When you're willing to deal with font-dependent spacing quirks

**TESTING:**
After implementing ListView labels:
- [ ] Test with Topaz 8 font (default)
- [ ] Test with Topaz 9 font (larger)
- [ ] Verify label doesn't overlap with gadgets above
- [ ] Verify ListView doesn't overlap with gadgets below
- [ ] Check that vertical spacing remains consistent
- [ ] Ensure window height accommodates label + ListView properly

**SUMMARY:**
Use **empty string labels** with ListView gadgets and create a separate TEXT gadget for the label. This gives you precise control, works with all font sizes, and makes your code cleaner and more maintainable.

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

### ListView and Event Handling Errors
1. **Forgetting IDCMP_GADGETDOWN for ListView windows** - This is why scroll arrow buttons don't work
   - **SOLUTION:** Always add `IDCMP_GADGETDOWN` to WA_IDCMP flags and handle it in event loop (see Section 0)
2. **Not handling IDCMP_GADGETDOWN in event loop** - Window listens but doesn't respond to scroll buttons
   - **SOLUTION:** Add case for `IDCMP_GADGETDOWN` and call `GT_RefreshWindow()` for ListView gadgets
3. **Using PLACETEXT_ABOVE with ListView label text** - Causes unpredictable spacing that varies with fonts
   - **SOLUTION:** Use empty string `""` for ng.ng_GadgetText and create separate TEXT gadget for label (see Section 0.1)
4. **Hardcoding ListView label space (like + 20)** - Breaks when user changes font size
   - **SOLUTION:** Use empty label + separate TEXT gadget, or calculate space dynamically with `font_height`

### Layout and Dimension Errors
5. **Using calculated ListView height instead of actual height** - This is the #1 cause of button overlap
6. **Not accounting for PLACETEXT_LEFT label space** - This causes label overlap with adjacent gadgets
   - **SOLUTION:** Use separate TEXT gadget for label instead of PLACETEXT_LEFT for complex rows
7. **Creating gadgets in wrong order** - ListView must be first to get actual dimensions
8. **Forgetting label widths in window calculations** - Causes window to be too narrow
9. **Trying to modify gadget dimensions after creation** - Doesn't trigger visual update; use pre-calculation instead
10. **Using PLACETEXT_LEFT for INPUT_ROW pattern** - Hard to align with other gadgets; use separate TEXT gadget instead

### Font and Text Measurement Errors
11. **Using `screen->RastPort.Font` instead of `DrawInfo`** - Should use `GetScreenDrawInfo()` for proper font access
12. **Using `strlen() * font_width` for proportional fonts** - Use `TextLength()` for accurate measurements
13. **Not testing with different fonts** - Layout may work with one font but fail with others
14. **Inconsistent button text padding** - Always use `BUTTON_TEXT_PADDING` constant

### Standard Pattern Violations
15. **Not using standard window sizes** - Use 40/60/80 char widths, not arbitrary values
16. **Wrong button count for window size** - Small=2, Medium=3, Large=4 buttons
17. **Not using pre-calculation block** - All dimensions must be calculated before CreateGadget()
18. **Ignoring standard spacing constants** - Use `WINDOW_SPACE_X`, `WINDOW_SPACE_Y`, etc.
19. **Not following pattern structure** - INPUT_ROW → REFERENCE_CONTENT → EQUAL_BUTTON_ROW

### Resource Management Errors
20. **Using GT_GetGadgetAttrs() for geometry** - Direct structure access is more reliable
21. **Not freeing DrawInfo resources** - Always call `FreeScreenDrawInfo()` when done
22. **Memory leaks in error paths** - Ensure cleanup_error: label frees all resources

## 🚀 Quick Reference Checklist

### Before Starting Layout
- [ ] Choose window size: Small (40), Medium (60), or Large (80) chars
- [ ] Define button count: Small=2, Medium=3, Large=4
- [ ] Define all spacing constants: `WINDOW_MARGIN_*`, `WINDOW_SPACE_*`, `BUTTON_TEXT_PADDING`
- [ ] Have `GetScreenDrawInfo()` and `TextLength()` ready for font measurements
- [ ] **If using ListView:** Plan to add `IDCMP_GADGETDOWN` to IDCMP flags (Section 0)
- [ ] **If using ListView:** Plan to use empty label `""` + separate TEXT gadget (Section 0.1)
- [ ] Define all spacing constants: `WINDOW_MARGIN_*`, `WINDOW_SPACE_*`, `BUTTON_TEXT_PADDING`
- [ ] Have `GetScreenDrawInfo()` and `TextLength()` ready for font measurements
- [ ] **If using ListView:** Plan to add `IDCMP_GADGETDOWN` to IDCMP flags (Section 0)

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

### Window Opening
- [ ] **If using ListView:** Include `IDCMP_GADGETDOWN` in WA_IDCMP flags
- [ ] Calculate final size using `precalc_max_right` and actual gadget heights
- [ ] Add all margins and borders
- [ ] Verify all gadgets created successfully

### Event Loop Setup
- [ ] **If using ListView:** Add `case IDCMP_GADGETDOWN:` to handle scroll buttons
- [ ] Handle IDCMP_GADGETUP for buttons
- [ ] Handle IDCMP_CLOSEWINDOW
- [ ] Handle IDCMP_REFRESHWINDOW

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

## ⚡ Performance Best Practice: Fast Window Opening with Deferred Loading

**THE PRINCIPLE:** Users should see windows open **immediately** with visual feedback, not wait staring at a blank screen while data loads. This creates a responsive, professional feel even on slow hardware (7MHz Amiga 500).

**THE PROBLEM:** If you parse files, scan directories, or build large lists **before** opening a window, users wait with no feedback. On slow machines, this can mean 5+ seconds of apparent unresponsiveness.

**❌ WRONG WAY (Poor UX):**
```c
BOOL open_my_window(struct WindowData *data, const char *data_file)
{
    /* Parse large data file FIRST - user waits with no feedback */
    if (!parse_large_file(data_file, data))
        return FALSE;
    
    /* Build complex data structures - still waiting... */
    build_folder_tree(data);
    
    /* FINALLY open window after 5 seconds - window appears instantly populated */
    data->window = OpenWindowTags(NULL, ...);
    
    /* Refresh gadgets */
    GT_RefreshWindow(data->window, NULL);
    
    return TRUE;
}
```

**User Experience:** Click button → *wait 5 seconds staring at previous window* → New window appears instantly with data

**✅ CORRECT WAY (Polished UX):**
```c
BOOL open_my_window(struct WindowData *data, const char *data_file)
{
    /* Initialize list structure - empty for fast opening */
    NewList(&data->item_list);
    
    /* Store data file path for later parsing */
    const char *stored_data_file = data_file;
    
    /* ... Get visual info, create gadgets with EMPTY list ... */
    
    /* Open window IMMEDIATELY with empty listview */
    data->window = OpenWindowTags(NULL,
        WA_Title, "Loading...",  /* Or your normal title */
        /* ... other tags ... */
        TAG_END);
    
    if (data->window == NULL)
        return FALSE;
    
    /* Set busy pointer IMMEDIATELY after window opens */
    SetWindowPointer(data->window,
                     WA_BusyPointer, TRUE,
                     TAG_END);
    
    /* NOW parse data file - window is visible with busy pointer */
    if (stored_data_file != NULL)
    {
        append_to_log("Window open, now parsing data...\n");
        
        if (!parse_large_file(stored_data_file, data))
        {
            /* Clear busy pointer even on error */
            SetWindowPointer(data->window,
                             WA_Pointer, NULL,
                             TAG_END);
            
            /* Close window and cleanup */
            close_my_window(data);
            return FALSE;
        }
        
        /* Build data structures */
        build_folder_tree(data);
        
        /* Update ListView with populated data */
        GT_SetGadgetAttrs(data->listview, data->window, NULL,
                          GTLV_Labels, &data->item_list,
                          TAG_DONE);
    }
    
    /* Refresh gadgets to display populated list */
    GT_RefreshWindow(data->window, NULL);
    append_to_log("Gadgets refreshed with populated list\n");
    
    /* Clear busy pointer - window is ready */
    SetWindowPointer(data->window,
                     WA_Pointer, NULL,
                     TAG_END);
    
    return TRUE;
}
```

**User Experience:** Click button → *Window appears instantly with busy pointer* → List populates after ~5 seconds → Ready!

**KEY BENEFITS:**
- ✅ **Instant feedback** - User knows their action was recognized
- ✅ **Visual indication** - Busy pointer shows work is in progress
- ✅ **Perceived performance** - Feels faster even if actual time is the same
- ✅ **Professional feel** - Matches behavior of well-designed Amiga applications
- ✅ **Error handling** - User sees window even if loading fails (can show error message)

**IMPLEMENTATION STEPS:**

1. **Initialize empty data structure** before opening window:
```c
NewList(&data->item_list);  /* Empty list */
```

2. **Store parameters** needed for loading (don't use them yet):
```c
const char *stored_file_path = file_path;
```

3. **Create gadgets with empty list**:
```c
gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
    GTLV_Labels, NULL,  /* NULL or &empty_list */
    TAG_DONE);
```

4. **Open window immediately**:
```c
window = OpenWindowTags(NULL, /* ... */);
```

5. **Set busy pointer right after window opens**:
```c
SetWindowPointer(window, WA_BusyPointer, TRUE, TAG_END);
```

6. **Perform slow operations** (parsing, scanning, etc.):
```c
parse_large_file(stored_file_path, data);
build_data_structures(data);
```

7. **Update gadgets** with loaded data:
```c
GT_SetGadgetAttrs(listview, window, NULL,
                  GTLV_Labels, &populated_list,
                  TAG_DONE);
```

8. **Refresh gadgets** to display changes:
```c
GT_RefreshWindow(window, NULL);
```

9. **Clear busy pointer** when ready:
```c
SetWindowPointer(window, WA_Pointer, NULL, TAG_END);
```

**ERROR HANDLING:**
Always clear the busy pointer on error paths:
```c
if (parse_error)
{
    /* Clear busy pointer */
    SetWindowPointer(window, WA_Pointer, NULL, TAG_END);
    
    /* Show error to user, close window, etc. */
    close_my_window(data);
    return FALSE;
}
```

**WHEN TO USE THIS PATTERN:**
- ✅ **File parsing** that takes > 1 second
- ✅ **Directory scanning** with many entries
- ✅ **Building complex data structures** (trees, sorted lists)
- ✅ **Network operations** or any I/O that might block
- ✅ **Any operation** that makes users wait on slow hardware

**WHEN NOT NEEDED:**
- ❌ **Instant operations** (< 0.5 seconds even on slow hardware)
- ❌ **Simple windows** with no data loading
- ❌ **Static content** that's already in memory

**REAL-WORLD EXAMPLE:**

The iTidy folder view window demonstrates this pattern perfectly:

**Before optimization:**
- Parsed 214-folder catalog file (5 seconds on 7MHz Amiga 500)
- Built folder tree structure
- THEN opened window
- Result: 5 second delay with no feedback

**After optimization:**
- Opens window immediately with empty list (< 0.1 seconds)
- Shows busy pointer
- Parses catalog in background
- Updates listview with data
- Clears busy pointer
- Result: Instant window, 5 second loading with visual feedback

**ALTERNATIVE: Progress Indicators**

For very long operations, consider updating a text gadget periodically:
```c
GT_SetGadgetAttrs(status_text, window, NULL,
                  GTTX_Text, "Loading... 50%",
                  TAG_DONE);
GT_RefreshWindow(window, NULL);
```

But for most cases, busy pointer + deferred loading is sufficient.

**PORTABILITY:**
- Works on all Amiga OS versions (2.0+)
- No special libraries required
- Uses standard Intuition SetWindowPointer() API
- Busy pointer automatically uses system pointer prefs

**KEY RULES:**
- ⚠️ **ALWAYS** set busy pointer immediately after opening window
- ⚠️ **ALWAYS** clear busy pointer when done (including error paths)
- ⚠️ **ALWAYS** use `GT_SetGadgetAttrs()` to update ListView after loading
- ⚠️ **ALWAYS** call `GT_RefreshWindow()` after updating gadgets
- ⚠️ **NEVER** do slow operations before opening window
- ⚠️ **NEVER** forget to clear busy pointer on error

This pattern makes your application feel fast, responsive, and professional, even on hardware from 1985. It's the difference between "this feels slow" and "this feels polished."

---

## 🎯 Handling Dynamic Text Overflow (Labels, Status Text, Paths)

**THE PROBLEM:** When displaying dynamic text (file paths, status messages, folder names, etc.) in labels or gadgets, you cannot control the text length in advance. Long text will extend past gadget boundaries, overlapping adjacent controls or window borders.

**SYMPTOMS:**
- File paths extend beyond window right border
- Long status text overlaps buttons or other gadgets
- Folder names run into adjacent columns
- Unprofessional appearance with text "bleeding" out of defined areas

**ROOT CAUSE:** The Amiga graphics library `Text()` function has **no built-in clipping or truncation**. It will draw the entire string regardless of available space. Text drawing is your responsibility - the system won't automatically wrap or clip.

### 🔍 Identifying Text That Needs Truncation

**High-Risk Text Types:**
- ✅ **File/directory paths** - Can be arbitrarily long (`Work:Programs/Graphics/DPaint/Icons/ToolDock/Config/Presets/...`)
- ✅ **User-entered text** - Unpredictable length (search queries, custom names)
- ✅ **Status messages** - May include paths or long descriptions
- ✅ **List items** - Especially if generated from file/folder names
- ✅ **Dynamic labels** - Text that changes at runtime based on data

**Low-Risk Text (Usually Safe):**
- ❌ Button labels - Fixed at compile time, you control length
- ❌ Static window labels - "Run Number:", "Date:", etc.
- ❌ Gadget titles - Usually short and predefined
- ❌ Menu items - Fixed text, system handles clipping

### ⚡ When to Apply Truncation

**ALWAYS truncate when:**
1. Text source is external (files, user input, system paths)
2. Text length is unpredictable at compile time
3. Text is displayed in a fixed-width area (label, listview, string gadget display)
4. Text could reasonably exceed available space (paths, descriptions)

**ASK THESE QUESTIONS:**
- ❓ Could this text be longer than the allocated width?
- ❓ Is the text coming from a file system path?
- ❓ Is the text user-generated or user-configurable?
- ❓ Could the text length vary based on user's system configuration?

**If ANY answer is "yes" → Apply truncation!**

### 🛠️ Truncation Strategies

#### Strategy 1: Middle Truncation (Recommended for Paths)

**When to use:** File/directory paths where both start and end are important.

**Visual Example:**
```
Before: Work:Programs/Applications/Graphics/DPaint/Icons/ToolDock/Config/Presets/UserSettings
After:  Work:Programs/.../UserSettings
```

**Why it's better:** Preserves context (volume/root) and destination (file/folder name).

**Implementation:**
```c
/* Use iTidy's built-in truncation function */
iTidy_Progress_DrawTruncatedText(
    rp,                    /* RastPort */
    left, top,             /* Position */
    path_string,           /* Full text */
    max_pixel_width,       /* Maximum width in pixels */
    TRUE,                  /* TRUE = path/middle truncation */
    text_pen);             /* Text color */
```

#### Strategy 2: End Truncation (For General Text)

**When to use:** Status messages, descriptions, general text where beginning is most important.

**Visual Example:**
```
Before: Processing item with very long name that goes on and on forever and ever
After:  Processing item with very long name that goes...
```

**Why it's useful:** Natural left-to-right reading, keeps the most relevant context.

**Implementation:**
```c
/* Use iTidy's built-in truncation function */
iTidy_Progress_DrawTruncatedText(
    rp,                    /* RastPort */
    left, top,             /* Position */
    status_text,           /* Full text */
    max_pixel_width,       /* Maximum width in pixels */
    FALSE,                 /* FALSE = end truncation */
    text_pen);             /* Text color */
```

### 📏 Calculating Maximum Width

**Critical:** Always use **pixel-based width**, not character count!

**WRONG (character-based - breaks with proportional fonts):**
```c
/* DON'T DO THIS - assumes all characters same width */
max_width = 40;  /* 40 characters - WRONG! */
if (strlen(text) > max_width)
    truncate(text);
```

**CORRECT (pixel-based - works with all fonts):**
```c
/* Calculate available width in pixels */
UWORD content_width = 400;  /* Content area width */
UWORD margin_left = 10;
UWORD margin_right = 10;
UWORD max_pixel_width = content_width - margin_left - margin_right;

/* Use pixel width for truncation */
iTidy_Progress_DrawTruncatedText(rp, x, y, text, max_pixel_width, TRUE, pen);
```

**For gadgets, use gadget width:**
```c
/* Get gadget dimensions */
UWORD gadget_width = my_gadget->Width;
UWORD padding = 8;  /* Internal padding */
UWORD max_text_width = gadget_width - (2 * padding);

/* Truncate to fit */
iTidy_Progress_DrawTruncatedText(rp, x, y, text, max_text_width, FALSE, pen);
```

### 🔧 Implementation Pattern for Your Code

**Step 1: Identify dynamic text in your layout**
```c
/* Example: Progress window showing current file being processed */
struct MyWindow {
    WORD status_x, status_y;      /* Position */
    UWORD status_max_width;       /* Pre-calculated maximum width */
    char current_file[256];       /* Current file being processed */
};
```

**Step 2: Calculate maximum width during window setup**
```c
/* During window layout calculation */
UWORD content_width = 400;  /* Your content area width */
data->status_max_width = content_width - MARGIN_LEFT - MARGIN_RIGHT;

append_to_log("Status text max width: %u pixels\n", data->status_max_width);
```

**Step 3: Use truncation when drawing text**
```c
/* When updating status text */
void update_status(struct MyWindow *data, const char *new_file_path)
{
    /* Clear old text */
    SetAPen(rp, bg_pen);
    RectFill(rp, data->status_x, data->status_y,
             data->status_x + data->status_max_width - 1,
             data->status_y + font_height - 1);
    
    /* Draw new text with truncation */
    iTidy_Progress_DrawTruncatedText(
        rp,
        data->status_x,
        data->status_y,
        new_file_path,
        data->status_max_width,  /* Use pre-calculated width */
        TRUE,                     /* Path truncation */
        text_pen);
}
```

### 📋 Checklist for Text Overflow Prevention

**During Layout Design:**
- [ ] Identify all text that could be dynamic or user-generated
- [ ] Calculate maximum pixel width for each text area
- [ ] Store maximum widths in window data structure
- [ ] Plan truncation strategy (middle for paths, end for text)

**During Implementation:**
- [ ] Use `TextLength()` to measure text width accurately
- [ ] Apply truncation to any text that could exceed max width
- [ ] Test with very long paths (100+ characters)
- [ ] Test with proportional fonts (Helvetica, etc.)
- [ ] Verify text doesn't overlap adjacent gadgets

**Testing Scenarios:**
- [ ] Very long file path: `Work:Programs/...` (50+ folders deep)
- [ ] Long filename: `This_Is_A_Very_Long_Filename_That_Goes_On_Forever.txt`
- [ ] Short text that fits: `Work:Temp` (should display as-is)
- [ ] Edge case: Text exactly at max width
- [ ] Different fonts: Topaz 8, Topaz 9, Helvetica

### 🎨 Visual Design Considerations

**Ellipsis Placement:**
- Middle truncation: `Start.../End` preserves most context
- End truncation: `Text...` is standard convention
- Never truncate without ellipsis - user won't know text was cut off

**User Experience:**
- ✅ **Good:** `Work:Programs/.../Tools` (clear what was truncated)
- ❌ **Bad:** `Work:ProgramsTools` (looks like a weird path)
- ❌ **Worse:** Text extends off screen (unprofessional)

**Alignment Considerations:**
```
Before:
[PC:Workbench/Programs/MagicWB/XEM-Icons/Toolsdoc→→→
                                        (overlaps border!)

After:
[PC:Workbench/.../Toolsdock            ]
                        (fits perfectly!)
```

### 🔥 Common Mistakes to Avoid

**❌ MISTAKE 1: Character-count truncation**
```c
/* WRONG - assumes fixed character width */
if (strlen(path) > 40)
    path[40] = '\0';  /* Doesn't account for proportional fonts! */
```

**❌ MISTAKE 2: No ellipsis indicator**
```c
/* WRONG - user can't tell text was truncated */
truncated_text[max_len] = '\0';  /* Just cut off, no "..." */
```

**❌ MISTAKE 3: Forgetting to clear old text**
```c
/* WRONG - old text ghosts behind new text */
Text(rp, new_text, strlen(new_text));  /* Should clear first! */
```

**❌ MISTAKE 4: Hardcoded pixel widths**
```c
/* WRONG - doesn't adapt to different window sizes or fonts */
max_width = 300;  /* Hardcoded - what if window is smaller? */
```

**✅ CORRECT APPROACH:**
```c
/* Calculate max width dynamically */
UWORD max_width = window_content_width - margins;

/* Measure actual text width */
UWORD text_width = TextLength(rp, text, strlen(text));

/* Truncate only if needed */
if (text_width > max_width)
{
    iTidy_Progress_DrawTruncatedText(rp, x, y, text, max_width, TRUE, pen);
}
else
{
    /* Fits fine - draw normally */
    SetAPen(rp, pen);
    Move(rp, x, y + rp->Font->tf_Baseline);
    Text(rp, text, strlen(text));
}
```

### 📚 Reference Implementation

iTidy's progress windows provide a complete reference implementation:

**Files:**
- `src/GUI/StatusWindows/progress_common.h` - Truncation API
- `src/GUI/StatusWindows/progress_common.c` - Implementation (~120 lines)
- `src/GUI/StatusWindows/progress_window.c` - Usage examples
- `docs/BUGFIX_TEXT_TRUNCATION_SYSTEM.md` - Detailed documentation

**Key Function:**
```c
void iTidy_Progress_DrawTruncatedText(
    struct RastPort *rp,
    WORD left, WORD top,
    const char *text,
    UWORD max_width,      /* Maximum width in PIXELS */
    BOOL is_path,         /* TRUE=middle, FALSE=end truncation */
    ULONG text_pen);
```

### 🎯 Decision Tree: Do I Need Truncation?

```
START: Analyzing text display
│
├─► Is text length FIXED at compile time? (button labels, static text)
│   └─► NO TRUNCATION NEEDED
│
├─► Is text from FILE SYSTEM? (paths, filenames)
│   └─► YES - Use MIDDLE TRUNCATION (path mode)
│
├─► Is text USER INPUT? (search, names, descriptions)
│   └─► YES - Use END TRUNCATION (text mode)
│
├─► Could text REASONABLY exceed width? (status messages, descriptions)
│   └─► YES - Use END TRUNCATION (text mode)
│
└─► Text is SHORT and CONTROLLED (dates, numbers, short labels)
    └─► PROBABLY SAFE, but test with proportional fonts
```

### 💡 Pro Tips

1. **Pre-calculate max widths** during window setup, not every draw
2. **Cache truncated results** if same text is drawn repeatedly
3. **Test with extreme cases** (200+ character paths)
4. **Use separate function** for truncation - don't inline the logic
5. **Log text that gets truncated** during development to verify
6. **Consider tooltip or full-text display** on hover (advanced)

### ⚠️ Critical Rules for AI Agents

When generating code for iTidy windows:

- ⚠️ **ALWAYS** check if text could be dynamic/user-generated
- ⚠️ **ALWAYS** use pixel-based width calculations, not character counts
- ⚠️ **ALWAYS** apply truncation to file paths (middle mode)
- ⚠️ **ALWAYS** clear old text before drawing new text
- ⚠️ **NEVER** assume text will fit - measure with `TextLength()`
- ⚠️ **NEVER** hardcode maximum widths - calculate from layout
- ✅ **PREFER** iTidy's built-in truncation functions
- ✅ **TEST** with very long paths and proportional fonts
- ✅ **DOCUMENT** which text fields use truncation and why

**Remember:** Text overflow is an easy mistake to make and creates an unprofessional appearance. A few minutes implementing proper truncation prevents hours of debugging visual glitches later!

---

---

## 📌 Additional Layout Rules for OS3.0/3.1 (MANDATORY FOR AI AGENTS)

The following rules extend the existing layout patterns with confirmed behaviour from **AmigaOS 3.0/3.1**. They are meant to be used together with the existing patterns (reference content column, equal button rows, etc.).

### 📏 Resizing Policy & IDCMP_NEWSIZE

The Dynamic Window Template supports **resizable** windows on Workbench 3.x, but resizing must be treated carefully to avoid broken layouts.

#### Default iTidy Policy

- Template windows use:
  - `WA_SizeGadget, TRUE`
  - `WA_SizeBBottom, TRUE`
  - `WA_SizeBRight, TRUE`
  - `IDCMP_NEWSIZE` in the IDCMP mask
- The default `handle_window_resize()` implementation:
  - Re-validates the window size
  - Triggers a full GadTools refresh (`GT_RefreshWindow()` and/or `GT_BeginRefresh`/`GT_EndRefresh` as needed).

#### Rules for AI Agents

- **Do not** remove `IDCMP_NEWSIZE` if the window has a size gadget.
- **Do not** reposition gadgets inside `IDCMP_REFRESHWINDOW`:
  - Refresh is for **redrawing**, not for re-layout.
- If an agent introduces a new layout pattern that depends on window size:
  - All size-dependent calculations must happen:
    - Either at window creation time, or
    - Inside the `IDCMP_NEWSIZE` handler, using the **same layout rules** as initial creation.

**Safe minimum NEWSIZE handler:**

```c
case IDCMP_NEWSIZE:
    handle_window_resize(data);  /* Reuse the template's resize helper */
    break;
```

If an agent needs more complex behaviour (e.g. resizing the ListView but keeping buttons fixed-height), it must **extend** `handle_window_resize()` instead of inventing a new layout path.

---

### 📚 ListView Height vs Layout (Reminder)

GadTools `LISTVIEW_KIND` gadgets on OS3:

- Snap their **actual rendering height** to an integer number of text lines.
- The height you pass to `CreateGadget()` is only a **request**.

For layout:

- Always use the **post-creation gadget height** in your geometry calculations:
  - After `CreateGadget(LISTVIEW_KIND, ...)`:
    - Read `listview->Height` and base:
      - Button row `TopEdge`
      - Window bottom (`WA_Height`)
      - Any “advanced” panel below the list
    - on that actual value.

This is already handled in the Dynamic Window Template. AI agents must **not** replace this with hard-coded pixel values, or the window will be misaligned on different fonts and screen modes.

---

### 🚫 No Advanced IDCMP/Layout Hacks

To keep layouts predictable on real Amiga hardware:

- Do **not**:
  - Share IDCMP ports between multiple windows.
  - Use `ModifyIDCMP()` on template windows to add/remove events dynamically.
  - Move gadgets by poking into their `LeftEdge`/`TopEdge` fields at random times.
- Do:
  - Recompute positions using the **same layout formulas** as initial creation.
  - Keep all geometry changes inside:
    - The window creation function, and/or
    - The `IDCMP_NEWSIZE` handler.

If a layout change cannot be expressed in terms of the existing patterns (`PATTERN_INPUT_ROW`, `PATTERN_REFERENCE_CONTENT`, `PATTERN_EQUAL_BUTTON_ROW`, etc.), the agent should stop and request a new pattern design instead of inventing ad-hoc geometry.
