/**
 * layout_preferences.c - iTidy Layout Preferences Implementation
 * 
 * Provides functions for initializing, managing, and applying
 * layout preference presets.
 */

#include <string.h>
#include "layout_preferences.h"

/*========================================================================*/
/**
 * @brief Initialize preferences to default values (Classic preset)
 * 
 * @param prefs Pointer to LayoutPreferences structure to initialize
 */
/*========================================================================*/
void InitLayoutPreferences(LayoutPreferences *prefs)
{
    if (prefs == NULL)
        return;
    
    /* Clear structure */
    memset(prefs, 0, sizeof(LayoutPreferences));
    
    /* Layout Settings */
    prefs->layoutMode = DEFAULT_LAYOUT_MODE;
    prefs->sortOrder = DEFAULT_SORT_ORDER;
    prefs->sortPriority = DEFAULT_SORT_PRIORITY;
    prefs->sortBy = DEFAULT_SORT_BY;
    prefs->reverseSort = DEFAULT_REVERSE_SORT;
    
    /* Visual Settings */
    prefs->centerIconsInColumn = DEFAULT_CENTER_ICONS;
    prefs->useColumnWidthOptimization = DEFAULT_OPTIMIZE_COLUMNS;
    prefs->textAlignment = DEFAULT_TEXT_ALIGNMENT;
    
    /* Window Management */
    prefs->resizeWindows = DEFAULT_RESIZE_WINDOWS;
    prefs->minIconsPerRow = DEFAULT_MIN_ICONS_PER_ROW;
    prefs->maxIconsPerRow = DEFAULT_MAX_ICONS_PER_ROW;
    prefs->maxWindowWidthPct = DEFAULT_MAX_WIDTH_PCT;
    prefs->aspectRatio = DEFAULT_ASPECT_RATIO;
    prefs->overflowMode = DEFAULT_OVERFLOW_MODE;
    
    /* Spacing Settings */
    prefs->iconSpacingX = DEFAULT_ICON_SPACING_X;
    prefs->iconSpacingY = DEFAULT_ICON_SPACING_Y;
    
    /* Custom Aspect Ratio */
    prefs->customAspectWidth = 16;   /* Default 16:10 = 1.6 */
    prefs->customAspectHeight = 10;
    prefs->useCustomAspectRatio = FALSE;
    
    /* Advanced Settings */
    prefs->skipHiddenFolders = DEFAULT_SKIP_HIDDEN_FOLDERS;
    
    /* Beta/Experimental Features */
    prefs->beta_openFoldersAfterProcessing = DEFAULT_BETA_OPEN_FOLDERS_AFTER_PROCESSING;
    prefs->beta_FindWindowOnWorkbenchAndUpdate = DEFAULT_BETA_FIND_WINDOW_ON_WORKBENCH_AND_UPDATE;
    
    /* Logging and Debug Settings */
    prefs->logLevel = DEFAULT_LOG_LEVEL;
    prefs->memoryLoggingEnabled = DEFAULT_MEMORY_LOGGING_ENABLED;
    prefs->enable_performance_logging = DEFAULT_PERFORMANCE_LOGGING_ENABLED;
    
    /* Default Tool Validation Settings */
    prefs->validate_default_tools = DEFAULT_VALIDATE_DEFAULT_TOOLS;
    
    /* Backup Settings */
    prefs->backupPrefs.enableUndoBackup = FALSE;
    prefs->backupPrefs.useLha = TRUE;
    strcpy(prefs->backupPrefs.backupRootPath, "PROGDIR:Backups");
    prefs->backupPrefs.maxBackupsPerFolder = 3;
}

/*========================================================================*/
/**
 * @brief Apply a preset configuration
 * 
 * Presets provide quick access to common layout configurations:
 * - Classic: Traditional Workbench appearance
 * - Compact: Vertical column layout for many icons
 * - Modern: Mixed sorting without folder priority
 * - WHDLoad: Optimized for game collections
 * 
 * @param prefs Pointer to LayoutPreferences structure
 * @param preset Preset index (PRESET_CLASSIC, PRESET_COMPACT, etc.)
 */
/*========================================================================*/
void ApplyPreset(LayoutPreferences *prefs, int preset)
{
    if (prefs == NULL)
        return;
    
    switch (preset)
    {
        case PRESET_CLASSIC:
            /* Classic Workbench appearance */
            prefs->layoutMode = LAYOUT_MODE_ROW;
            prefs->sortOrder = SORT_ORDER_HORIZONTAL;
            prefs->sortPriority = SORT_PRIORITY_FOLDERS_FIRST;
            prefs->sortBy = SORT_BY_NAME;
            prefs->reverseSort = FALSE;
            prefs->centerIconsInColumn = FALSE;
            prefs->useColumnWidthOptimization = TRUE;
            prefs->textAlignment = TEXT_ALIGN_BOTTOM;
            prefs->minIconsPerRow = 2;
            prefs->maxIconsPerRow = 0;  /* AUTO: Calculate from screen width */
            prefs->maxWindowWidthPct = 55;
            prefs->aspectRatio = 1.6f;
            prefs->overflowMode = OVERFLOW_HORIZONTAL;  /* Wide scrolling */
            prefs->iconSpacingX = 8;   /* Standard spacing */
            prefs->iconSpacingY = 8;
            break;
        
        case PRESET_COMPACT:
            /* Tight vertical layout for many icons */
            prefs->layoutMode = LAYOUT_MODE_COLUMN;
            prefs->sortOrder = SORT_ORDER_VERTICAL;
            prefs->sortPriority = SORT_PRIORITY_FOLDERS_FIRST;
            prefs->sortBy = SORT_BY_NAME;
            prefs->reverseSort = FALSE;
            prefs->centerIconsInColumn = FALSE;
            prefs->useColumnWidthOptimization = TRUE;
            prefs->textAlignment = TEXT_ALIGN_BOTTOM;
            prefs->minIconsPerRow = 2;
            prefs->maxIconsPerRow = 0;  /* AUTO: Calculate from screen width */
            prefs->maxWindowWidthPct = 45;
            prefs->aspectRatio = 1.3f;
            prefs->overflowMode = OVERFLOW_VERTICAL;  /* Maximize width usage */
            prefs->iconSpacingX = 6;   /* Tight spacing */
            prefs->iconSpacingY = 6;
            break;
        
        case PRESET_MODERN:
            /* Modern mixed sorting without folder priority */
            prefs->layoutMode = LAYOUT_MODE_ROW;
            prefs->sortOrder = SORT_ORDER_HORIZONTAL;
            prefs->sortPriority = SORT_PRIORITY_MIXED;
            prefs->sortBy = SORT_BY_DATE;
            prefs->reverseSort = FALSE;
            prefs->centerIconsInColumn = FALSE;
            prefs->useColumnWidthOptimization = TRUE;
            prefs->textAlignment = TEXT_ALIGN_BOTTOM;
            prefs->minIconsPerRow = 3;  /* Wider minimum */
            prefs->maxIconsPerRow = 0;  /* AUTO: Calculate from screen width */
            prefs->maxWindowWidthPct = 60;
            prefs->aspectRatio = 1.5f;
            prefs->overflowMode = OVERFLOW_BOTH;  /* Maintain proportions */
            prefs->iconSpacingX = 12;  /* Generous spacing */
            prefs->iconSpacingY = 10;
            break;
        
        case PRESET_WHDLOAD:
            /* Optimized for WHDLoad game collections */
            prefs->layoutMode = LAYOUT_MODE_COLUMN;
            prefs->sortOrder = SORT_ORDER_VERTICAL;
            prefs->sortPriority = SORT_PRIORITY_FOLDERS_FIRST;
            prefs->sortBy = SORT_BY_NAME;
            prefs->reverseSort = FALSE;
            prefs->centerIconsInColumn = FALSE;
            prefs->useColumnWidthOptimization = TRUE;
            prefs->textAlignment = TEXT_ALIGN_BOTTOM;
            prefs->minIconsPerRow = 2;
            prefs->maxIconsPerRow = 0;  /* AUTO: Calculate from screen width */
            prefs->maxWindowWidthPct = 40;
            prefs->aspectRatio = 1.4f;
            prefs->overflowMode = OVERFLOW_HORIZONTAL;  /* Wide game lists */
            prefs->iconSpacingX = 6;   /* Tight horizontal */
            prefs->iconSpacingY = 8;   /* Standard vertical */
            break;
        
        default:
            /* Unknown preset - use Classic */
            ApplyPreset(prefs, PRESET_CLASSIC);
            break;
    }
}

/*========================================================================*/
/**
 * @brief Copy GUI control values to LayoutPreferences structure
 * 
 * Maps the GUI gadget selections (cycle gadget indices and checkbox
 * states) to the corresponding LayoutPreferences fields.
 * 
 * @param prefs Pointer to LayoutPreferences structure
 * @param layout Layout mode selection (0 = Row, 1 = Column)
 * @param sort Sort order selection (0 = Horizontal, 1 = Vertical)
 * @param order Sort priority selection (0 = Folders First, 1 = Files First, 2 = Mixed)
 * @param sortby Sort criteria selection (0 = Name, 1 = Type, 2 = Date, 3 = Size)
 * @param center Center icons flag
 * @param optimize Optimize columns flag
 */
/*========================================================================*/
void MapGuiToPreferences(LayoutPreferences *prefs,
                         int layout, int sort, int order, int sortby,
                         BOOL center, BOOL optimize)
{
    if (prefs == NULL)
        return;
    
    /* Map cycle gadget selections to enums */
    prefs->layoutMode = (LayoutMode)layout;
    prefs->sortOrder = (SortOrder)sort;
    prefs->sortPriority = (SortPriority)order;
    prefs->sortBy = (SortBy)sortby;
    
    /* Map checkbox states */
    prefs->centerIconsInColumn = center;
    prefs->useColumnWidthOptimization = optimize;
}

/* End of layout_preferences.c */
