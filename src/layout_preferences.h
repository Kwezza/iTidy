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
/* Backup Preferences Structure                                          */
/*========================================================================*/
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
    
    /* Window Management */
    BOOL resizeWindows;              /* Auto-resize drawer windows */
    UWORD minIconsPerRow;            /* Minimum columns (prevent 1×N layouts) */
    UWORD maxIconsPerRow;            /* Maximum columns (0 = no limit) */
    UWORD maxWindowWidthPct;         /* Max window width as % of screen */
    float aspectRatio;               /* Target window aspect ratio */
    WindowOverflowMode overflowMode; /* Overflow behavior for large folders */
    
    /* Spacing Settings */
    UWORD iconSpacingX;              /* Horizontal spacing between icons (pixels) */
    UWORD iconSpacingY;              /* Vertical spacing between icons (pixels) */
    
    /* Custom Aspect Ratio */
    UWORD customAspectWidth;         /* Custom ratio numerator (e.g., 16) */
    UWORD customAspectHeight;        /* Custom ratio denominator (e.g., 10) */
    BOOL useCustomAspectRatio;       /* TRUE if "Custom" selected */
    
    /* Advanced Settings */
    BOOL skipHiddenFolders;          /* Skip folders without .info files (hidden) */
    
    /* Beta/Experimental Features */
    BOOL beta_openFoldersAfterProcessing;      /* Auto-open folders via Workbench during processing */
    BOOL beta_FindWindowOnWorkbenchAndUpdate;  /* Find open folder windows and move/resize them to match saved geometry */
    
    /* Backup Settings */
    BackupPreferences backupPrefs;   /* Embedded backup configuration */
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
#define DEFAULT_CENTER_ICONS        FALSE
#define DEFAULT_OPTIMIZE_COLUMNS    TRUE
#define DEFAULT_TEXT_ALIGNMENT      TEXT_ALIGN_BOTTOM  /* Default: align icons to bottom of row */
#define DEFAULT_RESIZE_WINDOWS      TRUE
#define DEFAULT_MIN_ICONS_PER_ROW   2   /* Prevent 1×N layouts */
#define DEFAULT_MAX_ICONS_PER_ROW   0   /* Auto: Calculate from screen width */
#define DEFAULT_MAX_WIDTH_PCT       55
#define DEFAULT_ASPECT_RATIO        1.6f
#define DEFAULT_OVERFLOW_MODE       OVERFLOW_HORIZONTAL  /* Classic behavior */
#define DEFAULT_ICON_SPACING_X      8    /* 8px horizontal spacing */
#define DEFAULT_ICON_SPACING_Y      8    /* 8px vertical spacing */
#define DEFAULT_SKIP_HIDDEN_FOLDERS TRUE   /* Default: ignore hidden folders */

/* Beta/Experimental Feature Defaults */
#define DEFAULT_BETA_OPEN_FOLDERS_AFTER_PROCESSING         FALSE   /* Enable for testing */
#define DEFAULT_BETA_FIND_WINDOW_ON_WORKBENCH_AND_UPDATE   FALSE   /* Enable for testing */

/* Icon Spacing Limits */
#define MIN_ICON_SPACING            4    /* Minimum 4px (icons too close) */
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

#endif /* LAYOUT_PREFERENCES_H */
