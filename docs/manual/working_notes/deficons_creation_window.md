# Icon Creation Settings

**Window title:** iTidy - Icon Creation Settings
**Navigation:** Main Window > Icon Creation... button

This window controls how iTidy creates new icons for files and folders that do not already have `.info` files. It uses the DefIcons system introduced in Workbench 3.2 to identify file types and assign appropriate icons.

Settings are organised into three tabs: Create, Rendering, and DefIcons.

Changes are applied when you click OK, or discarded if you click Cancel or close the window. To save settings permanently to disk, use the Presets > Save menu on the main window.

This window is modal -- the main window does not respond to input while it is open.

---

## Create Tab

### Folder Icons

Controls whether iTidy creates drawer icons for folders that do not have them.

| Option | Description |
|--------|-------------|
| Smart (Create Folder If It Has Icons Inside) | iTidy checks whether the folder contains files or icons that need processing. If it does, a drawer icon is created. **(Default)** |
| Always Create | A drawer icon is always created for folders without one. |
| Never Create | Folder icons are never created. |

**Hint:** "Controls whether iTidy creates drawer icons for folders that do not already have them."

### Skip icon creation inside WHDLoad folders

When enabled, folders that contain a WHDLoad slave file (*.slave) are skipped during icon creation. The drawer icon for the WHDLoad folder itself is still created, but no icons are generated for files inside it.

This is useful for WHDLoad game and demo collections where the internal files are not meant to be launched individually from Workbench.

Default: Off

**Hint:** "When enabled, files inside WHDLoad game folders are skipped during icon creation. The WHDLoad folder icon itself is still created."

### Text File Previews

When enabled, iTidy creates thumbnail-style icons for text files by rendering a preview of the file's contents onto the icon image. The appearance of text previews can be customised using the Manage Templates button.

Default: On

**Hint:** "When enabled, iTidy renders a preview of a text file's contents onto the icon image."

### Manage Templates...

Opens the Text Templates window, where you can view and edit the rendering templates that control how different text file types are previewed on their icons. See the Text Templates section for details.

**Hint:** "Opens the Text Templates window to view and edit how different text file types are rendered as icon previews."

### Picture File Previews

When enabled, iTidy creates thumbnail icons for recognised picture files by generating a miniature version of the image. The specific image formats to process are selected using the format checkboxes below.

Default: On

**Hint:** "When enabled, iTidy creates thumbnail icons for recognised image files by generating a miniature version of the image."

### Picture Formats

Select which image file formats should have thumbnail icons generated. Each format can be individually enabled or disabled.

| Format | Description | Default |
|--------|-------------|---------|
| ILBM (IFF) | Amiga IFF ILBM and PBM images. The most common Amiga image format. | On |

**Hint:** "Enable thumbnail creation for Amiga IFF ILBM and PBM images."

| PNG | PNG images. Supports transparency. | On |

**Hint:** "Enable thumbnail creation for PNG images. Supports transparency."

| GIF | GIF images. Supports transparency. | On |

**Hint:** "Enable thumbnail creation for GIF images. Supports transparency."

| JPEG (Slow) | JPEG images. Decoding is slow on 68k hardware. | Off |

**Hint:** "Enable thumbnail creation for JPEG images. Disabled by default because JPEG decoding is slow on 68k hardware."

| BMP | Windows BMP images. | On |

**Hint:** "Enable thumbnail creation for Windows BMP images."

| ACBM | Amiga Continuous Bitmap format. A rare Amiga bitmap format sometimes used in older demos or game asset data. | On |

**Hint:** "Enable thumbnail creation for Amiga Continuous Bitmap images, a rare format used in some older demos and game assets."

| Other | Any other image formats supported by installed DataTypes and DefIcons. | On |

**Hint:** "Enable thumbnail creation for other image formats supported by installed DataTypes and DefIcons."

**Note:** JPEG is disabled by default because JPEG decoding is computationally expensive on 68k processors. Enabling it will significantly slow down icon creation on classic Amiga hardware.

### Re-Run and Refresh Options

These options control what happens when you run iTidy again on a folder that already has icons created by a previous iTidy run.

#### Replace Existing Image Thumbnails Created by iTidy

When enabled, iTidy will delete and recreate any image thumbnail icons that it previously created. iTidy identifies its own icons using the `ITIDY_CREATED` tooltype. Icons placed by the user or by other programs are never affected.

This is useful after changing rendering settings (such as icon size, colour count, or border style) if you want the new settings applied to previously created thumbnails.

Default: Off

**Hint:** "When enabled, image thumbnail icons previously created by iTidy are deleted and regenerated. User-placed icons are not affected."

#### Replace Existing Text Previews Created by iTidy

When enabled, iTidy will delete and recreate any text preview icons that it previously created. As with image thumbnails, only icons with the `ITIDY_CREATED` tooltype are affected.

This is useful after changing text template settings or rendering options.

Default: Off

**Hint:** "When enabled, text preview icons previously created by iTidy are deleted and regenerated. User-placed icons are not affected."

---

## Rendering Tab

These settings control the visual appearance of generated thumbnail icons.

### Preview Size

Sets the canvas size used for thumbnail icons. Larger sizes show more detail but take up more space on screen.

| Option | Description |
|--------|-------------|
| Small (48x48) | Compact thumbnails. |
| Medium (64x64) | Balanced size and detail. **(Default)** |
| Large (100x100) | Largest thumbnails with the most detail. |

**Hint:** "Sets the canvas size used for thumbnail icons. Larger sizes show more detail but take up more screen space."

### Thumbnail Border

Controls the border style drawn around thumbnail icons.

| Option | Description |
|--------|-------------|
| None | No border is drawn. |
| Workbench (Smart) | Classic Workbench-style frame. Skipped for images with transparency. |
| Workbench (Always) | Classic Workbench-style frame, always applied. |
| Bevel (Smart) | Inner highlight bevel drawn on the image pixels (bright top-left, dark bottom-right). Skipped for images with transparency. **(Default)** |
| Bevel (Always) | Inner highlight bevel, always applied. |

The "Smart" modes detect transparent images and skip the border effect, which avoids drawing borders on empty space around irregularly-shaped images.

**Hint:** "Controls the border style drawn around thumbnail icons. The \"Smart\" modes skip the border on images with transparency."

### Upscale Small Images To Icon Size

When enabled, images that are smaller than the chosen preview size are scaled up to fill the thumbnail area. When disabled, small images are centred in the thumbnail at their original size.

Default: Off

**Hint:** "When enabled, images smaller than the preview size are scaled up to fill the thumbnail area."

### Colour Reduction

These settings control how full-colour images are reduced to fit within the limited palette of Amiga icons.

#### Max Colours

Sets the maximum number of colours used in generated thumbnail icons. Lower colour counts produce smaller icons and process faster, but higher counts look better.

| Option | Description |
|--------|-------------|
| 4 colors | Very limited palette. |
| 8 colors | Basic colour range. |
| 16 colors | Moderate colour range. |
| GlowIcons palette (29 colours) | Uses the standard GlowIcons palette for maximum compatibility with existing icon sets. |
| 32 colors | Good colour range. |
| 64 colors | Rich colour range. |
| 128 colors | Near-photographic quality. |
| 256 colors (full) | Full 256-colour palette. **(Default)** |
| Ultra (256 + detail boost) | 256 colours with enhanced detail processing for the best possible image quality. |

**Hint:** "Sets the maximum number of colours used in generated thumbnail icons. Higher counts look better but produce larger icon files."

**Gadget dependencies:**

- When Max Colours is set to 256 or Ultra, the Dithering option is disabled (not needed at full colour depth).
- When Max Colours is set above 8, or to GlowIcons palette or Ultra, the Low-Colour Palette option is disabled (only relevant at very low colour counts).

#### Dithering

Selects the dithering method used when reducing the number of colours in an image. Dithering simulates intermediate colours by mixing nearby pixels.

| Option | Description |
|--------|-------------|
| None | No dithering. Colours are mapped directly. |
| Ordered (Bayer 4x4) | Systematic dithering pattern. Fast and predictable. |
| Error Diffusion (Floyd-Steinberg) | Diffuses quantisation error to neighbouring pixels. Produces smoother gradients but can be slower. |
| Auto (Based On Colour Count) | Automatically selects the best method based on the colour count. **(Default)** |

**Hint:** "Selects the dithering method used when reducing colours. Disabled when \"Max Colours\" is set to 256 or Ultra."

This option is disabled when Max Colours is set to 256 or Ultra.

#### Low-Colour Palette

Controls the palette mapping used at very low colour counts (4 or 8 colours). This determines what fixed palette the colours are reduced to.

| Option | Description |
|--------|-------------|
| Greyscale | Maps to a greyscale palette. **(Default)** |
| Workbench Palette | Maps to the standard Workbench colour palette. |
| Hybrid (Grays + WB Accents) | Uses mostly greyscale tones with a few Workbench accent colours mixed in. |

**Hint:** "Controls the colour palette used at very low colour counts (4 or 8). Only enabled when \"Max Colours\" is set to 4 or 8."

This option is only enabled when Max Colours is set to 4 or 8. It is disabled at higher colour counts, for the GlowIcons palette, and for Ultra mode.

---

## DefIcons Tab

This tab provides access to the DefIcons configuration sub-windows.

### Icon Creation Setup...

Opens the DefIcons Categories window, where you can select which file types should have icons created during processing. The window shows a tree view of all available DefIcons file types with checkboxes, and lets you view or change the default tool assigned to each type.

See the DefIcons Categories section for details.

**Hint:** "Opens the DefIcons Categories window to select which file types should receive icons during processing."

### Exclude Paths...

Opens the Exclude Paths window, where you can manage the list of folder paths that should be skipped during icon creation. This is useful for excluding system directories or folders that should not have icons generated.

See the Exclude Paths section for details.

**Hint:** "Opens the Exclude Paths window to manage folders that should be skipped during icon creation."

---

## Buttons

### OK

Accepts the current settings and closes the window. Changes are applied to the active session.

**Hint:** "Accepts the current settings and closes the window. Changes are applied to the active session."

### Cancel

Discards all changes and closes the window. The original settings are preserved.

**Hint:** "Discards all changes and closes the window. The original settings are preserved."

---

## Default Values Summary

| Setting | Default |
|---------|---------|
| Folder Icons | Smart |
| Skip WHDLoad Folders | Off |
| Text File Previews | On |
| Picture File Previews | On |
| ILBM (IFF) | On |
| PNG | On |
| GIF | On |
| JPEG (Slow) | Off |
| BMP | On |
| ACBM | On |
| Other | On |
| Replace Image Thumbnails | Off |
| Replace Text Previews | Off |
| Preview Size | Medium (64x64) |
| Thumbnail Border | Bevel (Smart) |
| Upscale Small Images | Off |
| Max Colours | 256 colors (full) |
| Dithering | Auto |
| Low-Colour Palette | Greyscale |
