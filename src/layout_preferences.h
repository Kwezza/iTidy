/**
 * layout_preferences.h - iTidy Layout and Sorting Preferences
 * 
 * Defines the structure and enums for controlling icon layout,
 * sorting behavior, window resizing, and backup options.
 * 
 * Based on iTidy_GUI_Feature_Design.md specification.
 */

#ifndef LAYOUT_PREFERENCES_H
#define LAYOUT_PREFERENCES_H

#include <exec/types.h>

/*========================================================================*/
/* Layout Mode Enumeration                                               */
/*========================================================================*/
/**
 * @brief Icon layout direction (row-major vs column-major)
 */
typedef enum {
    LAYOUT_MODE_ROW = 0,      /* Row-major: Fill horizontally first (classic) */
    LAYOUT_MODE_COLUMN = 1    /* Column-major: Fill vertically first (compact) */
} LayoutMode;

/*========================================================================*/
/* Sort Order Enumeration                                                */
/*========================================================================*/
/**
 * @brief Primary sorting direction within layout
 */
typedef enum {
    SORT_ORDER_HORIZONTAL = 0,  /* Sort left-to-right, top-to-bottom */
    SORT_ORDER_VERTICAL = 1     /* Sort top-to-bottom, left-to-right */
} SortOrder;

/*========================================================================*/
/* Sort Priority Enumeration                                             */
/*========================================================================*/
/**
 * @brief How folders and files are prioritized in sorting
 */
typedef enum {
    SORT_PRIORITY_FOLDERS_FIRST = 0,  /* Folders before files */
    SORT_PRIORITY_FILES_FIRST = 1,    /* Files before folders */
    SORT_PRIORITY_MIXED = 2           /* No folder/file priority */
} SortPriority;

/*========================================================================*/
/* Sort By Enumeration                                                   */
/*========================================================================*/
/**
 * @brief Primary sort criteria
 */
typedef enum {
    SORT_BY_NAME = 0,   /* Sort alphabetically by name */
    SORT_BY_TYPE = 1,   /* Sort by file type/extension */
    SORT_BY_DATE = 2,   /* Sort by modification date */
    SORT_BY_SIZE = 3    /* Sort by file size */
} SortBy;

/*========================================================================*/
/* Text Alignment Enumeration                                            */
/*========================================================================*/
/**
 * @brief Vertical alignment of icons within a row
 */
typedef enum {
    TEXT_ALIGN_TOP = 0,     /* Icons aligned to top of row */
    TEXT_ALIGN_MIDDLE = 1,  /* Icons centered vertically in row */
    TEXT_ALIGN_BOTTOM = 2   /* Icons aligned to bottom of row (default) */
} TextAlignment;

/*========================================================================*/
/* Block Grouping Mode Enumeration                                       */
/*========================================================================*/
/**
 * @brief How icons are visually grouped in the layout
 */
typedef enum {
    BLOCK_GROUP_NONE = 0,       /* Standard layout (no grouping) */
    BLOCK_GROUP_BY_TYPE = 1     /* Group by WBDRAWER/WBTOOL/other */
} BlockGroupMode;

/*========================================================================*/
/* Block Gap Size Enumeration                                            */
/*========================================================================*/
/**
 * @brief Vertical spacing between grouped blocks
 */
typedef enum {
    BLOCK_GAP_SMALL = 0,        /* 5 pixels */
    BLOCK_GAP_MEDIUM = 1,       /* 10 pixels */
    BLOCK_GAP_LARGE = 2         /* 15 pixels */
} BlockGapSize;

/* Pixel values for gap sizes */
#define BLOCK_GAP_SMALL_PX   5
#define BLOCK_GAP_MEDIUM_PX  10
#define BLOCK_GAP_LARGE_PX   15

/*========================================================================*/
/* Window Overflow Mode Enumeration                                      */
/*========================================================================*/
/**
 * @brief Window overflow behavior when icons exceed screen size
 */
typedef enum {
    OVERFLOW_HORIZONTAL = 0,  /* Expand width, add horizontal scrollbar */
    OVERFLOW_VERTICAL = 1,    /* Expand height, add vertical scrollbar */
    OVERFLOW_BOTH = 2         /* Expand both, add both scrollbars */
} WindowOverflowMode;

/*========================================================================*/
/* Window Position Mode Enumeration                                      */
/*========================================================================*/
/**
 * @brief Window positioning behavior after resize
 */
typedef enum {
    WINDOW_POS_CENTER_SCREEN = 0,  /* Center window on screen (classic behavior) */
    WINDOW_POS_KEEP_POSITION = 1,  /* Keep current position, pull back if off-screen */
    WINDOW_POS_NEAR_PARENT = 2,    /* Position near parent drawer (if space allows) */
    WINDOW_POS_NO_CHANGE = 3       /* Don't move window at all */
} WindowPositionMode;

/*========================================================================*/
/* Backup Preferences Structure                                          */
/*========================================================================
/**
 * @brief Settings for LHA backup creation before processing
 */
typedef struct {
    BOOL enableUndoBackup;           /* Create backup before processing */
    BOOL useLha;                     /* Use LhA compression */
    char backupRootPath[108];        /* Root backup directory path */
    UWORD maxBackupsPerFolder;       /* Maximum backup archives to retain */
} BackupPreferences;

/*========================================================================*/
/* Layout Preferences Structure                                          */
/*========================================================================*/
/**
 * @brief Master preferences structure for icon layout and processing
 * 
 * This structure consolidates all settings for icon arrangement,
 * sorting, window management, and backup functionality. It is used
 * to pass preferences from the GUI to the icon processing engine.
 * 
 * @note This structure is designed for ENV: persistence and can be
 *       serialized for saving/loading user preferences.
 */
typedef struct {
    /* Folder and Scanning Settings */
    char folder_path[256];           /* Target folder path for processing */
    BOOL recursive_subdirs;          /* Process subdirectories recursively */
    
    /* Layout Settings */
    LayoutMode layoutMode;           /* Row-major or column-major */
    SortOrder sortOrder;             /* Horizontal or vertical sorting */
    SortPriority sortPriority;       /* Folder/file grouping priority */
    SortBy sortBy;                   /* Primary sort criteria */
    BOOL reverseSort;                /* Reverse sort order */
    
    /* Visual Settings */
    BOOL centerIconsInColumn;        /* Center icons between grid lines */
    BOOL useColumnWidthOptimization; /* Optimize per-column widths */
    TextAlignment textAlignment;     /* Vertical alignment of text labels in rows */
    
    /* Block Grouping Settings */
    BlockGroupMode blockGroupMode;   /* BLOCK_GROUP_NONE or BLOCK_GROUP_BY_TYPE */
    BlockGapSize blockGapSize;       /* SMALL, MEDIUM, or LARGE gap between blocks */
    
    /* Window Management */
    BOOL resizeWindows;              /* Auto-resize drawer windows */
    UWORD minIconsPerRow;            /* Minimum columns (prevent 1×N layouts) */
    UWORD maxIconsPerRow;            /* Maximum columns (0 = no limit) */
    UWORD maxWindowWidthPct;         /* Max window width as % of screen */
    int aspectRatio;                 /* Target window aspect ratio (scaled by 1000, e.g., 1600 = 1.6) */
    WindowOverflowMode overflowMode; /* Overflow behavior for large folders */
    WindowPositionMode windowPositionMode; /* Window positioning behavior after resize */
    
    /* Spacing Settings */
    UWORD iconSpacingX;              /* Horizontal spacing between icons (pixels) */
    UWORD iconSpacingY;              /* Vertical spacing between icons (pixels) */
    
    /* Custom Aspect Ratio */
    UWORD customAspectWidth;         /* Custom ratio numerator (e.g., 16) */
    UWORD customAspectHeight;        /* Custom ratio denominator (e.g., 10) */
    BOOL useCustomAspectRatio;       /* TRUE if "Custom" selected */
    
    /* Backup and Icon Upgrade Settings */
    BOOL enable_backup;              /* Create backup before processing */
    BOOL enable_icon_upgrade;        /* Upgrade icon formats during processing */
    BOOL stripNewIconBorders;        /* Strip borders from NewIcons (one-way, requires icon.library v44+) */
    
    /* Advanced Settings */
    BOOL skipHiddenFolders;          /* Skip folders without .info files (hidden) */
    
    /* Beta/Experimental Features */
    BOOL beta_openFoldersAfterProcessing;      /* Auto-open folders via Workbench during processing */
    BOOL beta_FindWindowOnWorkbenchAndUpdate;  /* Find open folder windows and move/resize them to match saved geometry */
    
    /* DefIcons Icon Creation Settings (Workbench 3.2+) */
    BOOL enable_deficons_icon_creation;        /* Master enable/disable for automatic icon creation */
    char deficons_disabled_types[256];         /* Comma-separated list of disabled root type names (e.g., "tool,font") */
    UWORD deficons_folder_icon_mode;           /* 0=Smart (create if visible), 1=Always, 2=Never */
    BOOL deficons_skip_system_assigns;         /* TRUE = skip SYS:, C:, S:, DEVS:, LIBS:, etc. */
    BOOL deficons_log_created_icons;           /* TRUE = log all created .info files to dedicated log (for testing/delete scripts) */
    UWORD deficons_icon_size_mode;              /* 0=Small(48), 1=Medium(64), 2=Large(100) — IFF thumbnail size */
    BOOL deficons_enable_thumbnail_borders;     /* TRUE = enable borders/frames on image thumbnails, FALSE = frameless (edge-to-edge) */
    BOOL deficons_enable_text_previews;         /* TRUE = enable text file content preview rendering on icons */
    BOOL deficons_enable_picture_previews;      /* TRUE = enable picture file thumbnail rendering on icons */
    
    /* DefIcons Exclude Paths (user-configurable exclude list) */
    #define MAX_DEFICONS_EXCLUDE_PATHS 32
    #define DEFICONS_EXCLUDE_PATH_LENGTH 256
    char deficons_exclude_paths[32][256];      /* Array of path patterns to exclude (supports DEVICE: placeholder) */
    UWORD deficons_exclude_path_count;         /* Number of active exclude paths (0-32) */
    
    /* Logging and Debug Settings */
    UWORD logLevel;                            /* Log level: 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR */
    BOOL memoryLoggingEnabled;                 /* Enable memory allocation logging (creates memory_*.log) */
    BOOL enable_performance_logging;           /* Enable performance timing logging for iTidy operations */
    
    /* Default Tool Validation Settings */
    BOOL validate_default_tools;               /* Enable default tool validation using system PATH */
    
    /* Default Tool Backup Settings */
    BOOL enable_default_tool_backup;           /* Create CSV backup before default tool changes */
    
    /* Backup Settings */
    BackupPreferences backupPrefs;   /* Embedded backup configuration */
    
    /* === NEW FIELDS BELOW THIS LINE (v2 prefs format) === */
    /* These MUST remain at the end of the struct for backward  */
    /* compatibility with v1 preference files.                  */
    
    /* DefIcons Palette Reduction Settings (Workbench 3.2+) */
    UWORD deficons_max_icon_colors;             /* Max palette colors: 4,8,16,32,64,128,256 */
    UWORD deficons_dither_method;               /* 0=None, 1=Ordered, 2=Floyd-Steinberg, 3=Auto */
    UWORD deficons_lowcolor_mapping;            /* 0=Grayscale, 1=WB palette, 2=Hybrid (only when max<=8) */
    BOOL deficons_ultra_mode;                   /* TRUE = 256-color detail-preserving downsample */

    /* Per-format picture thumbnail enable flags (v3 fields) */
    ULONG deficons_picture_formats_enabled;     /* Bitmask: ITIDY_PICTFMT_* bits — which formats get thumbnails */

    /* Upscaling control (v3 fields) */
    BOOL deficons_upscale_thumbnails;           /* FALSE = keep small images at natural size (default) */
                                                /* TRUE  = scale up images smaller than icon size      */
} LayoutPreferences;

/*========================================================================*/
/* Preset Constants                                                      */
/*========================================================================*/
/**
 * @brief Predefined preset configurations
 */
#define PRESET_CLASSIC  0  /* Classic Workbench look */
#define PRESET_COMPACT  1  /* Tight vertical layout */
#define PRESET_MODERN   2  /* Modern mixed sorting */
#define PRESET_WHDLOAD  3  /* WHDLoad-optimized layout */

/*========================================================================*/
/* Default Values                                                        */
/*========================================================================*/
/**
 * @brief Default layout preferences (Classic preset)
 */
#define DEFAULT_LAYOUT_MODE         LAYOUT_MODE_ROW
#define DEFAULT_SORT_ORDER          SORT_ORDER_HORIZONTAL
#define DEFAULT_SORT_PRIORITY       SORT_PRIORITY_FOLDERS_FIRST
#define DEFAULT_SORT_BY             SORT_BY_NAME
#define DEFAULT_REVERSE_SORT        FALSE
#define DEFAULT_CENTER_ICONS        TRUE
#define DEFAULT_OPTIMIZE_COLUMNS    TRUE
#define DEFAULT_TEXT_ALIGNMENT      TEXT_ALIGN_MIDDLE /* Default: align icons to middle of row */
#define DEFAULT_RESIZE_WINDOWS      TRUE
#define DEFAULT_MIN_ICONS_PER_ROW   2   /* Prevent 1×N layouts */
#define DEFAULT_MAX_ICONS_PER_ROW   0   /* Auto: Calculate from screen width */
#define DEFAULT_MAX_WIDTH_PCT       55
#define DEFAULT_ASPECT_RATIO        2000  /* 2.0 * 1000 (fixed-point) */
#define DEFAULT_OVERFLOW_MODE       OVERFLOW_HORIZONTAL  /* Classic behavior */
#define DEFAULT_WINDOW_POSITION_MODE WINDOW_POS_CENTER_SCREEN  /* Classic auto-center behavior */
#define DEFAULT_ICON_SPACING_X      8    /* 8px horizontal spacing */
#define DEFAULT_ICON_SPACING_Y      8    /* 8px vertical spacing */
#define DEFAULT_SKIP_HIDDEN_FOLDERS TRUE   /* Default: ignore hidden folders */

/* Beta/Experimental Feature Defaults */
#define DEFAULT_BETA_OPEN_FOLDERS_AFTER_PROCESSING         FALSE   /* Enable for testing */
#define DEFAULT_BETA_FIND_WINDOW_ON_WORKBENCH_AND_UPDATE   FALSE   /* Enable for testing */

/* DefIcons Feature Defaults (Workbench 3.2+) */
#define DEFAULT_ENABLE_DEFICONS_ICON_CREATION              FALSE   /* Disabled by default (opt-in feature) */
#define DEFAULT_DEFICONS_DISABLED_TYPES                    "tool,prefs,iff,key,kickstart"  /* Disabled types (unchecked in default UI) */
#define DEFAULT_DEFICONS_FOLDER_ICON_MODE                  0       /* 0=Smart (create if visible), 1=Always, 2=Never */
#define DEFAULT_DEFICONS_SKIP_SYSTEM_ASSIGNS               TRUE    /* Skip system directories by default */
#define DEFAULT_DEFICONS_LOG_CREATED_ICONS                 TRUE    /* Log created icons to separate file (useful for testing) */
#define DEFAULT_DEFICONS_ICON_SIZE_MODE                     1       /* 0=Small(48), 1=Medium(64), 2=Large(100) */
#define DEFAULT_DEFICONS_ENABLE_THUMBNAIL_BORDERS           TRUE    /* Default: Borders enabled (frameless=FALSE) */
#define DEFAULT_DEFICONS_ENABLE_TEXT_PREVIEWS               TRUE    /* Default: Text content previews enabled */
#define DEFAULT_DEFICONS_ENABLE_PICTURE_PREVIEWS            TRUE    /* Default: Picture thumbnails enabled */
#define DEFAULT_DEFICONS_UPSCALE_THUMBNAILS                 FALSE   /* Default: Do NOT upscale small images to icon size */

/* DefIcons Palette Reduction Defaults */
#define DEFAULT_DEFICONS_MAX_ICON_COLORS                    256     /* No limit (full palette) */
#define DEFAULT_DEFICONS_DITHER_METHOD                      3       /* Auto (smart selection by color count) */
#define DEFAULT_DEFICONS_LOWCOLOR_MAPPING                   0       /* Grayscale (safe default for 4-8 colors) */
#define DEFAULT_DEFICONS_ULTRA_MODE                         FALSE   /* Standard mode by default */

/*========================================================================*/
/* Picture Format Enable Bitmask Constants                               */
/*========================================================================*/

/**
 * @brief Per-format enable bits for deficons_picture_formats_enabled.
 *
 * Only formats with a corresponding datatype installed in
 * DEVS:DataTypes (or Classes/DataTypes) can produce thumbnails.
 * If the datatype is absent, NewDTObject() returns NULL and the
 * pipeline logs a warning and skips to the next file.
 *
 * Default WB 3.2 datatypes present: ilbm, png, gif, acbm, bmp, jpeg.
 * jpeg is disabled by default due to slow decode on 68000.
 */

/* IFF ILBM (standard Amiga bitplane format — native parser + DT fallback for HAM) */
#define ITIDY_PICTFMT_ILBM      0x0001UL
/* PNG (WB 3.2 default — fast, supports alpha; recommended for UI art) */
#define ITIDY_PICTFMT_PNG       0x0002UL
/* GIF (WB 3.2 default — fast, supports 1-bit transparency; multi-frame uses frame 0) */
#define ITIDY_PICTFMT_GIF       0x0004UL
/* JPEG (WB 3.2 default — SLOW: can take >60s on 68000; disabled by default) */
#define ITIDY_PICTFMT_JPEG      0x0008UL
/* BMP (WB 3.2 default — uncompressed Windows bitmap) */
#define ITIDY_PICTFMT_BMP       0x0010UL
/* ACBM (Amiga Continuous BitMap — IFF variant; WB 3.2 default) */
#define ITIDY_PICTFMT_ACBM      0x0020UL
/* All other picture formats (tiff, targa, pcx, ico, sunraster, reko, brush, etc.) */
/* Only works if a third-party datatype for that format is installed. */
#define ITIDY_PICTFMT_OTHER     0x0040UL

/* Convenience: all formats enabled */
#define ITIDY_PICTFMT_ALL       (ITIDY_PICTFMT_ILBM | ITIDY_PICTFMT_PNG | ITIDY_PICTFMT_GIF | \
                                 ITIDY_PICTFMT_JPEG | ITIDY_PICTFMT_BMP | ITIDY_PICTFMT_ACBM | \
                                 ITIDY_PICTFMT_OTHER)

/* Default: all enabled EXCEPT JPEG (slow) */
#define DEFAULT_DEFICONS_PICTURE_FORMATS_ENABLED \
    (ITIDY_PICTFMT_ILBM | ITIDY_PICTFMT_PNG | ITIDY_PICTFMT_GIF | \
     ITIDY_PICTFMT_BMP  | ITIDY_PICTFMT_ACBM | ITIDY_PICTFMT_OTHER)

/* Logging and Debug Defaults */
#define DEFAULT_LOG_LEVEL                                  4       /* Default: DISABLED level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=DISABLED) */
#define DEFAULT_MEMORY_LOGGING_ENABLED                     FALSE   /* Default: Disabled (can be very verbose) */
#define DEFAULT_PERFORMANCE_LOGGING_ENABLED                FALSE   /* Default: Disabled (performance timing logs) */

/* Default Tool Validation Defaults */
#define DEFAULT_VALIDATE_DEFAULT_TOOLS                     TRUE    /* Default: Enabled (validate default tools using system PATH) */

/* Default Tool Backup Defaults */
#define DEFAULT_ENABLE_DEFAULT_TOOL_BACKUP                 TRUE    /* Default: Enabled (auto-backup before default tool changes) */

/* NewIcon Border Stripping Defaults */
#define DEFAULT_STRIP_NEWICON_BORDERS                      FALSE   /* Default: Disabled (one-way operation, requires backup) */

/* Icon Spacing Limits */
#define MIN_ICON_SPACING            0    /* Minimum 0px (allow testing exact boundaries) */
#define MAX_ICON_SPACING           20    /* Maximum 20px (too much wasted space) */

/*========================================================================*/
/* Function Prototypes                                                   */
/*========================================================================*/

/**
 * @brief Initialize preferences to default values
 * @param prefs Pointer to LayoutPreferences structure to initialize
 */
void InitLayoutPreferences(LayoutPreferences *prefs);

/**
 * @brief Apply a preset configuration
 * @param prefs Pointer to LayoutPreferences structure
 * @param preset Preset index (PRESET_CLASSIC, etc.)
 */
void ApplyPreset(LayoutPreferences *prefs, int preset);

/**
 * @brief Copy GUI settings to LayoutPreferences structure
 * @param prefs Pointer to LayoutPreferences structure
 * @param layout Layout mode selection (0 = Row, 1 = Column)
 * @param sort Sort order selection (0 = Horizontal, 1 = Vertical)
 * @param order Sort priority selection (0 = Folders First, etc.)
 * @param sortby Sort criteria selection (0 = Name, etc.)
 * @param center Center icons flag
 * @param optimize Optimize columns flag
 */
void MapGuiToPreferences(LayoutPreferences *prefs,
                         int layout, int sort, int order, int sortby,
                         BOOL center, BOOL optimize);

/*========================================================================*/
/* Global Preferences Access Functions                                   */
/*========================================================================*/

/**
 * @brief Initialize global preferences to default values
 * 
 * Call this once at application startup to initialize the global
 * preference singleton with sensible defaults (Classic preset).
 * 
 * @note This MUST be called before any other preference functions
 */
void InitializeGlobalPreferences(void);

/**
 * @brief Get read-only access to global preferences
 * 
 * Returns a const pointer to the global preference singleton. Use this
 * to access current application preferences from anywhere in the code.
 * 
 * @return Const pointer to global LayoutPreferences (never NULL)
 */
const LayoutPreferences* GetGlobalPreferences(void);

/**
 * @brief Update global preferences with new values
 * 
 * Updates the global preference singleton with values from the provided
 * LayoutPreferences structure. Typically called from GUI event handlers
 * when the user clicks "Apply" or "OK".
 * 
 * @param newPrefs Pointer to LayoutPreferences with new values
 */
void UpdateGlobalPreferences(const LayoutPreferences *newPrefs);

/**
 * @brief Set the scan path in global preferences
 * 
 * Convenience setter for updating just the folder path.
 * 
 * @param path New folder path (copied to internal buffer)
 * @note Path is limited to 255 characters
 */
void SetGlobalScanPath(const char *path);

/**
 * @brief Set recursive scanning mode in global preferences
 * 
 * Convenience setter for toggling recursive subdirectory scanning.
 * 
 * @param recursive TRUE to enable recursive scanning, FALSE to disable
 */
void SetGlobalRecursiveMode(BOOL recursive);

/**
 * @brief Set skip hidden folders mode in global preferences
 * 
 * Convenience setter for controlling whether hidden folders (those
 * without .info files) are processed.
 * 
 * @param skip TRUE to skip hidden folders, FALSE to process them
 */
void SetGlobalSkipHiddenFolders(BOOL skip);

/*========================================================================*/
/* DefIcons Preferences Helper Functions                                 */
/*========================================================================*/

/**
 * @brief Check if a DefIcons type is enabled for icon creation
 * 
 * Checks whether automatic icon creation is enabled for a specific
 * DefIcons type by looking in the disabled types list.
 * 
 * @param prefs Pointer to LayoutPreferences structure
 * @param type_name Type name to check (e.g., "tool", "music", "picture")
 * 
 * @return TRUE if type is enabled, FALSE if disabled
 */
BOOL is_deficon_type_enabled(const LayoutPreferences *prefs, const char *type_name);

/**
 * @brief Add a type to the disabled types list
 * 
 * Adds a DefIcons type to the disabled list, preventing automatic
 * icon creation for that category.
 * 
 * @param prefs Pointer to LayoutPreferences structure
 * @param type_name Type name to disable (e.g., "tool", "font")
 * 
 * @return TRUE if added successfully, FALSE on error
 */
BOOL add_disabled_deficon_type(LayoutPreferences *prefs, const char *type_name);

/**
 * @brief Remove a type from the disabled types list
 * 
 * Removes a DefIcons type from the disabled list, re-enabling automatic
 * icon creation for that category.
 * 
 * @param prefs Pointer to LayoutPreferences structure
 * @param type_name Type name to enable (e.g., "tool", "font")
 * 
 * @return TRUE if removed successfully, FALSE on error
 */
BOOL remove_disabled_deficon_type(LayoutPreferences *prefs, const char *type_name);

/**
 * @brief Clear all disabled types (enable all)
 * 
 * Clears the disabled types list, enabling automatic icon creation
 * for all DefIcons categories.
 * 
 * @param prefs Pointer to LayoutPreferences structure
 */
void clear_disabled_deficon_types(LayoutPreferences *prefs);

/**
 * @brief Apply default disabled types after loading deficons tree
 * 
 * Sets the default disabled types (tool, prefs, iff, key, kickstart) based on
 * the screenshot defaults. Silently skips any types that don't exist in the
 * loaded deficons tree (since deficons.prefs is an external user system file).
 * 
 * This should be called after the deficons tree is loaded and before displaying
 * the settings window for the first time, or when resetting to defaults.
 * 
 * @param prefs Pointer to LayoutPreferences structure
 * @param tree Pointer to DefIcons type tree array
 * @param count Number of nodes in tree
 */
void apply_default_deficon_type_selections(LayoutPreferences *prefs, 
                                           const void *tree, 
                                           int count);

/*========================================================================*/
/* DefIcons Exclude Paths Helper Functions                               */
/*========================================================================*/

/**
 * @brief Reset exclude paths to default list
 * 
 * Resets the exclude paths array to the default list of system
 * directories that should be skipped during icon creation.
 * Uses DEVICE: placeholder for portability across volumes.
 * 
 * @param prefs Pointer to LayoutPreferences structure
 */
void reset_deficons_exclude_paths_to_defaults(LayoutPreferences *prefs);

/**
 * @brief Add a path to the exclude list
 * 
 * Adds a directory path to the exclude list. Paths can use the
 * DEVICE: placeholder for portability (e.g., "DEVICE:Fonts").
 * 
 * @param prefs Pointer to LayoutPreferences structure
 * @param path Path to add (absolute or DEVICE: pattern)
 * 
 * @return TRUE if added successfully, FALSE if list is full or duplicate
 */
BOOL add_deficons_exclude_path(LayoutPreferences *prefs, const char *path);

/**
 * @brief Remove a path from the exclude list
 * 
 * Removes a path at the specified index from the exclude list.
 * 
 * @param prefs Pointer to LayoutPreferences structure
 * @param index Index of path to remove (0-based)
 * 
 * @return TRUE if removed successfully, FALSE if index invalid
 */
BOOL remove_deficons_exclude_path(LayoutPreferences *prefs, int index);

/**
 * @brief Modify an existing exclude path
 * 
 * Replaces the path at the specified index with a new value.
 * 
 * @param prefs Pointer to LayoutPreferences structure
 * @param index Index of path to modify (0-based)
 * @param new_path New path value
 * 
 * @return TRUE if modified successfully, FALSE if index invalid
 */
BOOL modify_deficons_exclude_path(LayoutPreferences *prefs, int index, const char *new_path);

/**
 * @brief Set skip hidden folders mode in global preferences
 * 
 * Convenience setter for controlling whether hidden folders (those
 * without .info files) are processed.
 * 
 * @param skip TRUE to skip hidden folders, FALSE to process them
 */
void SetGlobalSkipHiddenFolders(BOOL skip);

#endif /* LAYOUT_PREFERENCES_H */
