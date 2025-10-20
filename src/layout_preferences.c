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
    prefs->maxIconsPerRow = DEFAULT_MAX_ICONS_PER_ROW;
    prefs->maxWindowWidthPct = DEFAULT_MAX_WIDTH_PCT;
    prefs->aspectRatio = DEFAULT_ASPECT_RATIO;
    
    /* Backup Settings */
    prefs->backupPrefs.enableUndoBackup = FALSE;
    prefs->backupPrefs.useLha = TRUE;
    strcpy(prefs->backupPrefs.backupRootPath, "Work:iTidyBackups/");
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
            prefs->maxIconsPerRow = 5;  /* 5 icons fit nicely on 640px screen */
            prefs->maxWindowWidthPct = 55;
            prefs->aspectRatio = 1.6f;
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
            prefs->maxIconsPerRow = 6;
            prefs->maxWindowWidthPct = 45;
            prefs->aspectRatio = 1.3f;
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
            prefs->maxIconsPerRow = 8;
            prefs->maxWindowWidthPct = 60;
            prefs->aspectRatio = 1.5f;
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
            prefs->maxIconsPerRow = 4;
            prefs->maxWindowWidthPct = 40;
            prefs->aspectRatio = 1.4f;
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
