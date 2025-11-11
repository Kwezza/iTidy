# Tool Cache Window Design

## Window Layout Diagram

```
+-----------------------------------------------------------------------------+
| iTidy - Default Tool Analysis                                      - [ ] X |  <- Title bar (prefsIControl.currentWindowBarHeight)
+-----------------------------------------------------------------------------+
|                                                                             |
|  Tool Name             | Hits | Status                                     |
|  -----------------------------------------------------------------------   |
|  MultiView             |  71  | EXISTS                                     |
|  More                  |  14  | EXISTS                                     |
|  JUNK                  |  15  | MISSING                                    |
|  sc:c/se               | 212  | EXISTS   <- Selected (highlighted)        |
|  sc:c/scopts           |  30  | EXISTS                                     |
|  AWeb-II               |   0  | MISSING                                    |
|  Installer             |   6  | EXISTS                                     |
|  ...                   |      |                                            |
|  ...                   |      |                                            |
|  ...                   |      |                                            |
|  ...                   |      |                                            |
|  ...                   |      |                                            |
|  -----------------------------------------------------------------------   |
|                                                                             |
|  Total Tools: 39  |  Valid: 26  |  Missing: 13                             |
|                                                                             |
|  Tool Details:                                                              |
|  -----------------------------------------------------------------------   |
|    Tool Name: sc:c/se                                                      |
|       Status: Found on System                                              |
|    Hit Count: 212 references                                               |
|    Full Path: sc:c/se                                                      |
|      Version: SAS/C_SE 6.58 (10.7.97)                                      |
|  -----------------------------------------------------------------------   |
|                                                                             |
|  [ Show All ]  [ Show Valid Only ]  [ Show Missing Only ]  [ Close ]      |
|                                                                             |
+-----------------------------------------------------------------------------+
  <- All margins use TOOL_WINDOW_MARGIN_* constants
```

## Gadget Layout Specifications

### Window Dimensions
- **Width**: 70 characters (TOOL_WINDOW_WIDTH_CHARS)
- **Adapts to screen**: Centered on Workbench screen
- **NTSC HiRes Compatible**: Window height calculated to fit 200px screens
- **Title Bar**: Uses `prefsIControl.currentWindowBarHeight` for proper spacing

### Spacing Constants
```c
#define TOOL_WINDOW_SPACE_X         8       /* Horizontal spacing */
#define TOOL_WINDOW_SPACE_Y         8       /* Vertical spacing */
#define TOOL_WINDOW_MARGIN_LEFT     10      /* Left margin */
#define TOOL_WINDOW_MARGIN_TOP      10      /* Top margin (after title bar) */
#define TOOL_WINDOW_MARGIN_RIGHT    10      /* Right margin */
#define TOOL_WINDOW_MARGIN_BOTTOM   10      /* Bottom margin */
#define TOOL_WINDOW_BUTTON_PADDING  8       /* Button text padding */
```

### Main Tool ListView
- **Type**: LISTVIEW_KIND with scrollbar
- **Height**: 12 visible rows (adjustable based on available screen height)
- **Font**: System Default Text font (fixed-width) for column alignment
- **Columns**: 
  - Tool Name (22 chars, left-aligned)
  - Hits (4 digits, right-aligned with padding)
  - Status (7 chars: "EXISTS " or "MISSING")
- **Format**: `"%-22s| %4d | %s"` for proper column alignment
- **IDCMP**: Includes IDCMP_GADGETDOWN for scroll arrow buttons

### Summary Bar
- **Type**: Static text (drawn after listview, before details)
- **Content**: "Total Tools: X  |  Valid: Y  |  Missing: Z"
- **Purpose**: Quick overview of cache statistics

### Details Panel
- **Type**: Read-only LISTVIEW_KIND (5 lines)
- **Height**: 5 rows for detail fields
- **Font**: System Default Text font for colon-aligned formatting
- **Format**: Colon-aligned labels (right-justified to maximum label length)
  ```
    Tool Name: [value]
       Status: [value]
    Hit Count: [value]
    Full Path: [value]
      Version: [value]
  ```
- **Updates**: When user selects a tool from main list

### Filter and Close Buttons
- **Type**: 4 equal-width buttons (BUTTON_KIND)
- **Layout**: Horizontal row spanning full window width
- **Buttons**:
  1. "Show All" (default filter)
  2. "Show Valid Only" (tools with exists=TRUE)
  3. "Show Missing Only" (tools with exists=FALSE)
  4. "Close" (closes window)
- **Width Calculation**: 
  ```c
  equal_width = (reference_width - (3 * TOOL_WINDOW_SPACE_X)) / 4;
  if (equal_width < max_text_width + TOOL_WINDOW_BUTTON_PADDING)
      equal_width = max_text_width + TOOL_WINDOW_BUTTON_PADDING;
  ```

## Data Source

### Global Tool Cache Access
```c
/* External declarations from icon_types.h */
extern ToolCacheEntry *g_ToolCache;
extern int g_ToolCacheCount;
extern int g_ToolCacheCapacity;
```

### Data Flow
1. Window opens → Reads from `g_ToolCache` array
2. Builds display entries with formatted text for listview
3. Applies filter (all/valid/missing)
4. Populates listview with filtered entries
5. User selects tool → Updates details panel
6. User clicks filter button → Re-applies filter, updates listview

## Implementation Details

### Font Selection Strategy
- **Primary**: Use System Default Text font (`GfxBase->DefaultFont`)
- **Reason**: Ensures fixed-width characters for column alignment
- **Fallback**: If system font fails to open, use screen font
- **Check**: Test if screen font is proportional (`FPF_PROPORTIONAL` flag)

### ListView Cleanup (Critical)
```c
/* STEP 1: Detach lists FIRST */
GT_SetGadgetAttrs(tool_list, window, NULL, GTLV_Labels, ~0, TAG_END);

/* STEP 2: Free list data */
free_tool_cache_entries(tool_data);

/* STEP 3: Close window */
CloseWindow(window);

/* STEP 4: Free gadgets */
FreeGadgets(glist);
```

### Memory Management
- **Display entries**: Allocated dynamically, freed on close
- **Tool data**: Points to g_ToolCache (NOT duplicated)
- **Display text**: Allocated and formatted for each entry
- **Details list**: Allocated dynamically, freed when selection changes

## Screen Size Adaptations

### NTSC HiRes (640x200)
- Window height reduced to fit available screen space
- Main listview: 8 visible rows (instead of 12)
- Details panel: 5 rows (unchanged - essential info)
- Buttons: Standard height (font_height + 6)
- All content fits within ~180px usable height

### PAL High-Res (640x256)
- Window uses standard 12-row listview
- Full details panel
- Comfortable spacing

### Calculation Formula
```c
UWORD available_height = screen->Height - prefsIControl.currentWindowBarHeight - 
                         TOOL_WINDOW_MARGIN_TOP - TOOL_WINDOW_MARGIN_BOTTOM - 
                         (button_height * 2) - (TOOL_WINDOW_SPACE_Y * 3);

UWORD listview_rows = (available_height / 2) / listview_line_height;
if (listview_rows > 12) listview_rows = 12;  /* Maximum */
if (listview_rows < 6) listview_rows = 6;    /* Minimum usable */
```

## User Workflow

1. **Opening**: Window appears immediately (deferred loading pattern)
2. **Initial View**: Shows all tools sorted by hit count (most used first)
3. **Filtering**: Click button to filter by valid/missing/all
4. **Selection**: Click tool to see details in panel below
5. **Scrolling**: Use scroll arrows or scrollbar to navigate large lists
6. **Closing**: Click Close button or window close gadget

## Technical Benefits

- No emoticons/unicode - Pure ASCII text display
- Fixed-width font ensures perfect column alignment
- Colon-aligned details panel (professional Amiga style)
- Adapts to NTSC (200px) and PAL (256px) screens
- Respects user's Workbench font preferences
- Uses IControl preferences for proper borders
- Follows all patterns from AI_AGENT_GUIDE.md
- Safe ListView cleanup prevents crashes
- Efficient: No data duplication, points to cache

## Future Enhancements

- Sortable columns (click header to sort by name/hits/status)
- Export to text file option
- "Clear Icons" button for missing tools
- "Update Tool" button to bulk-update default tool paths
- Search/filter by tool name
- Statistics graph showing tool usage distribution

## Files Created
- `src/GUI/tool_cache_window.h` - Header with structures and prototypes
- `src/GUI/tool_cache_window.c` - Full implementation (~900 lines)
- `docs/TOOL_CACHE_WINDOW_DESIGN.md` - This design document

## Integration Point
To open from main window:
```c
struct iTidyToolCacheWindow tool_window;

if (open_tool_cache_window(&tool_window))
{
    /* Event loop */
    while (handle_tool_cache_window_events(&tool_window))
    {
        WaitPort(tool_window.window->UserPort);
    }
    
    /* Cleanup */
    close_tool_cache_window(&tool_window);
}
```
