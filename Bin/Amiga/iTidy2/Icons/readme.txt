iTidy Icons Folder
==================
Version 2.0

This folder contains template icons used by iTidy when generating text-preview
icons for ASCII-based file types (C source, REXX scripts, HTML, shell scripts,
and so on).  If you want to change how iTidy draws the text-preview thumbnails
-- the look of the artwork, the colours, or the area of the icon used - this
is the right place to start.


What Is a Template Icon?
------------------------
When iTidy creates a preview icon for a text file it does not generate the icon
from scratch.  Instead it takes a template icon from this folder, copies it to
the target directory, and then renders a miniature text-density preview onto the
icon face.  The template supplies the base artwork; iTidy just paints the text
content over it.

The renderer is a density/structure renderer, not a text layout engine.  Each
line of the source file is represented as a thin horizontal band of pixels, and
each character within that line is one or two pixels wide.  The result looks
like ruled-paper marks showing the density and layout of the file rather than
readable characters.  The icons are small, and the goal is to show at a glance 
how much content a file has, any ASCII art, and where it is concentrated.


The Master Template: def_ascii.info
------------------------------------
The central file in this folder is def_ascii.info.  This is the master template.

Every ASCII sub-type that does NOT have its own dedicated template falls back to
def_ascii.info automatically.  You can think of it as the default that covers
everything unless you have overridden it.

You should not delete def_ascii.info.  Most of iTidy's text-preview operations
require it to be present.


Custom Templates: def_<type>.info
----------------------------------
You can give a specific sub-type its own look by creating a custom template:

  def_c.info      - used only for C source files
  def_rexx.info   - used only for REXX scripts
  def_html.info   - used only for HTML files
  ... and so on.

If a custom template exists for a type, iTidy uses that instead of the master.
If it does not exist, the master is used.

The easiest way to create a custom template is through iTidy's built-in
Text Templates window (Main window -> Settings -> DefIcons Categories
-> Preview Icons).  From there you can:

  * See the full list of supported sub-types and their current status.
  * Create a custom template from the master with one click.
  * Open the template in the Workbench Information editor to change ToolTypes.
  * Validate ToolTypes to catch typos or out-of-range values.
  * Revert a custom template back to the master.

You can also manage template icons manually using any icon editor, but using the
Text Templates window is recommended because it handles file copying, companion
files, and validation automatically.


How to Customise the Artwork
-----------------------------
The artwork (the visual design of the icon itself) is changed the same way as
any Amiga icon - open it in your favourite icon editor (e.g. IconEdit) and
draw what you like.  iTidy will use that artwork as the base and paint the text
preview on top of it.

To control WHERE and HOW the text preview is painted you edit the icon's
ToolTypes.  See the ToolTypes section below.


ToolTypes Reference
-------------------
All ToolTypes used by iTidy are prefixed ITIDY_.  Open a template icon in the
Workbench Information editor (or use the "Edit tooltypes..." button in the Text
Templates window) to view and change them.

Use iTidy's "Validate tooltypes" button after editing to check for mistakes.

-----------------------------------------------------------------------------
LAYOUT AND RENDERING ZONE
-----------------------------------------------------------------------------

ITIDY_TEXT_AREA=x,y,w,h
  Defines the rectangle inside the icon where text is drawn.
  x,y is the top-left corner; w,h is the width and height in pixels.
  Default: the icon bounds minus a 4-pixel margin on all sides.

  Example: ITIDY_TEXT_AREA=4,4,48,52
    Draws inside a 48x52 pixel zone starting 4 pixels from the top-left.

ITIDY_EXCLUDE_AREA=x,y,w,h
  A zone inside the text area where NO pixels are written.
  Use this to protect decorative parts of the artwork (e.g. a folded-corner
  graphic in the bottom-right corner) from being overwritten.
  Default: none (no exclusion).

  Example: ITIDY_EXCLUDE_AREA=40,44,16,12
    Leaves a 16x12 pixel block starting at (40,44) untouched.

  Note: ITIDY_TEXT_AREA and ITIDY_EXCLUDE_AREA are independent.  The exclude
  area can overlap or sit entirely within the text area.

-----------------------------------------------------------------------------
LINE AND CHARACTER SIZING
-----------------------------------------------------------------------------

ITIDY_LINE_HEIGHT=n
  How many output pixels tall each rendered line band is.
  Default: 1 (one pixel per line).  At 1, a 60-pixel-tall text area can
  show 60 line bands.  Set to 2 or 3 for thicker, bolder bands and fewer
  lines visible per icon.

  Example: ITIDY_LINE_HEIGHT=2

ITIDY_CHAR_WIDTH=n
  How many output pixels wide each character is.
  Default: 0 (auto-select).
    - 2 pixels per character when the text area is 64 pixels wide or more.
    - 1 pixel per character when the text area is narrower than 64 pixels.
  Set to a specific value to override auto-selection.

  Example: ITIDY_CHAR_WIDTH=1

ITIDY_READ_BYTES=n
  How many bytes of the source file to read when building the preview.
  Default: 4096.  Increase for files where the important content is deeper
  in the file; decrease for faster rendering on slower hardware.

  Example: ITIDY_READ_BYTES=8192

NOTE: ITIDY_LINE_GAP and ITIDY_MAX_LINES are recognised by the validator and
will not trigger unknown-key warnings, but they are not currently used by the
renderer.  Setting them has no visible effect.

-----------------------------------------------------------------------------
COLOUR MODES
-----------------------------------------------------------------------------

There are two colour modes: fixed colour mode (the default) and adaptive text
mode.  ITIDY_ADAPTIVE_TEXT=YES switches to adaptive mode.

--- Fixed colour mode (ITIDY_ADAPTIVE_TEXT=NO, the default) ---

In fixed colour mode each text pixel is painted using a fixed palette index.

ITIDY_TEXT_COLOUR=n
  Palette index (0-255) used for medium- and high-density text pixels.
  Default: auto-detect (darkest palette entry).

ITIDY_MID_COLOUR=n
  Palette index (0-255) used for sparse/edge text pixels.
  Default: auto-detect (palette entry closest to the mid-luminance point).

  A safety check prevents text and background resolving to the same index.

ITIDY_BG_COLOR=n
  Palette index (0-254) for the background fill inside the text area.
  Default: absent, meaning NO fill - the template artwork is left as-is and
  text pixels are painted on top.  This is the most common setting.
  Set to a palette index only if you want a solid-colour background.
  Index 255 is reserved internally and cannot be used here.

  Example: ITIDY_BG_COLOR=0   (fill with palette colour 0 before drawing)

--- Adaptive text mode (ITIDY_ADAPTIVE_TEXT=YES) ---

In adaptive mode the renderer samples the existing background pixel at each
position and darkens it.  The text "inherits" the colour of whatever artwork
is already in the template at that pixel location.  This gives a natural look
when the template has coloured or gradient artwork behind the text area.

When ITIDY_ADAPTIVE_TEXT=YES:
  - ITIDY_TEXT_COLOUR and ITIDY_MID_COLOUR are ignored for painting.
  - ITIDY_BG_COLOR should normally be left absent.  If you set it to a
    solid fill index, all pixels darken from the same starting colour and
    you lose the adaptive benefit.

ITIDY_ADAPTIVE_TEXT=YES|NO
  Switches adaptive colouring on or off.  Default: NO.

ITIDY_DARKEN_PERCENT=n
  Darkening strength (1-100) for even-row text pixels in adaptive mode.
  Default: 70.  Higher values produce darker text marks.
  Only active when ITIDY_ADAPTIVE_TEXT=YES.

ITIDY_DARKEN_ALT_PERCENT=n
  Darkening strength (1-100) for alternate (odd) row text pixels.
  Default: 35.  This produces the lighter "ruled paper" stripe effect.
  Only active when ITIDY_ADAPTIVE_TEXT=YES.

ITIDY_EXPAND_PALETTE=YES|NO
  When YES (the default), iTidy pre-adds darkened colour variants from the
  existing template palette before rendering.  This gives the darkening tables
  more entries to map to, resulting in smoother gradations between tones.
  Only active when ITIDY_ADAPTIVE_TEXT=YES and the palette has fewer than 128
  entries.  Disable with ITIDY_EXPAND_PALETTE=NO if you have a full palette
  and do not want any expansion.

-----------------------------------------------------------------------------
TOOLTYPE QUICK REFERENCE
-----------------------------------------------------------------------------

  ToolType                 Default     Notes
  -----------------------  ----------  --------------------------------------
  ITIDY_TEXT_AREA          4px margin  x,y,w,h   rendering zone
  ITIDY_EXCLUDE_AREA       (none)      x,y,w,h   protected zone
  ITIDY_LINE_HEIGHT        1           pixels per rendered line band
  ITIDY_CHAR_WIDTH         0 (auto)    pixels per character; 0=auto
  ITIDY_READ_BYTES         4096        bytes read from source file
  ITIDY_BG_COLOR           (none)      palette index or absent=no fill
  ITIDY_TEXT_COLOUR        (auto)      fixed-mode text colour index
  ITIDY_MID_COLOUR         (auto)      fixed-mode mid-tone colour index
  ITIDY_ADAPTIVE_TEXT      NO          YES=adaptive darkening mode
  ITIDY_DARKEN_PERCENT     70          adaptive mode: even-row strength
  ITIDY_DARKEN_ALT_PERCENT 35          adaptive mode: odd-row strength
  ITIDY_EXPAND_PALETTE     YES         adaptive mode: pre-add palette shades
  ITIDY_LINE_GAP           (reserved)  not currently enforced by renderer
  ITIDY_MAX_LINES          (reserved)  not currently enforced by renderer


Excluded Types
--------------
The master template (def_ascii.info) can contain an EXCLUDETYPE ToolType with a
comma-separated list of sub-type names.  Types listed there are excluded from
receiving any preview icons at all.  They will appear in the Text Templates
window with status "Excluded".  ITidy will then fall back to using the current
default deficons for those types instead of generating previews.

To un-exclude a type, open def_ascii.info via the "Edit tooltypes..." button on
the master row and remove the type name from the EXCLUDETYPE ToolType, then save.


Step-by-Step: Creating a Custom Template
-----------------------------------------
1. Open iTidy's Text Templates window:
     Main window -> Settings -> DefIcons Categories -> Preview Icons
   (or via Icon Creation Settings -> Create tab -> Manage Templates...)

2. Find the sub-type you want to customise in the list (e.g. "c" for C source).

3. Click "Create from master".  iTidy copies def_ascii.info to def_c.info in
   this folder.

4. Click "Edit tooltypes..." to open def_c.info in the Workbench Information
   editor.  Add or change ITIDY_* ToolTypes as needed and save.

5. Optionally, open the icon in an icon editor to change its artwork.

6. Return to iTidy and click "Validate tooltypes" to check for mistakes.

7. Run iTidy on a directory containing C source files to test the result.

To undo and go back to using the master, click "Revert to master".


Step-by-Step: Editing the Master Template
-------------------------------------------
1. Open the Text Templates window (as above).

2. Click the "ascii" row (the master) and click "Edit tooltypes...".

3. Change any ITIDY_* ToolType and save.  The new values apply to all sub-types
   that do not have their own custom template.

4. Click "Validate tooltypes" to confirm the values are correct.


Tips
----
- Start by editing def_ascii.info.  Once you are happy with the result for
  most file types, create custom templates only for types that need different
  treatment.

- Use "Validate tooltypes" after any edit.  It catches unknown key names
  (often typos), out-of-range colour indices, and malformed x,y,w,h values.

- If your template has decoration you want to protect (e.g. a corner tag),
  measure its position in an icon editor and set ITIDY_EXCLUDE_AREA accordingly.

- ITIDY_ADAPTIVE_TEXT=YES works best when the template has varied artwork
  behind the text area.  Leave ITIDY_BG_COLOR absent in adaptive mode.

- Colour indices refer to the icon's own palette, not screen colours.  Index 0
  in one template may be a different colour from index 0 in another.


Files in This Folder
---------------------
  def_ascii       Companion file for def_ascii.info (required by Workbench)
  def_ascii.info  Master template icon - do not delete
  readme.txt      This file

Custom templates you create will also appear here as def_<type>.info pairs.
