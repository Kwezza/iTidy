/**
 * layout_preferences.c - iTidy Layout Preferences Implementation
 * 
 * Provides functions for initializing, managing, and applying
 * layout preference presets.
 * 
 * Includes global preference singleton for application-wide access.
 */

#include <string.h>
#include <stdio.h>
#include "layout_preferences.h"
#include "deficons/deficons_parser.h"

/*========================================================================*/
/* Global Preferences Singleton                                          */
/*========================================================================*/
/**
 * @brief Global application preferences
 * 
 * Single source of truth for all iTidy preferences. Initialized at
 * startup via InitializeGlobalPreferences() and accessed throughout
 * the application via GetGlobalPreferences().
 */
static LayoutPreferences g_AppPreferences;

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
    
    /* Folder and Scanning Settings */
    prefs->folder_path[0] = '\0';       /* Empty path initially */
    prefs->recursive_subdirs = FALSE;
    
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
    
    /* Block Grouping Settings */
    prefs->blockGroupMode = BLOCK_GROUP_NONE;
    prefs->blockGapSize = BLOCK_GAP_MEDIUM;
    
    /* Window Management */
    prefs->resizeWindows = DEFAULT_RESIZE_WINDOWS;
    prefs->minIconsPerRow = DEFAULT_MIN_ICONS_PER_ROW;
    prefs->maxIconsPerRow = DEFAULT_MAX_ICONS_PER_ROW;
    prefs->maxWindowWidthPct = DEFAULT_MAX_WIDTH_PCT;
    prefs->aspectRatio = DEFAULT_ASPECT_RATIO;
    prefs->overflowMode = DEFAULT_OVERFLOW_MODE;
    prefs->windowPositionMode = DEFAULT_WINDOW_POSITION_MODE;
    
    /* Spacing Settings */
    prefs->iconSpacingX = DEFAULT_ICON_SPACING_X;
    prefs->iconSpacingY = DEFAULT_ICON_SPACING_Y;
    
    /* Custom Aspect Ratio */
    prefs->customAspectWidth = 16;   /* Default 16:10 = 1.6 */
    prefs->customAspectHeight = 10;
    prefs->useCustomAspectRatio = FALSE;
    
    /* Backup and Icon Upgrade Settings */
    prefs->enable_backup = FALSE;
    prefs->enable_icon_upgrade = FALSE;
    prefs->stripNewIconBorders = DEFAULT_STRIP_NEWICON_BORDERS;
    
    /* Advanced Settings */
    prefs->skipHiddenFolders = DEFAULT_SKIP_HIDDEN_FOLDERS;
    
    /* DefIcons Icon Creation Settings */
    prefs->enable_deficons_icon_creation = DEFAULT_ENABLE_DEFICONS_ICON_CREATION;
    strcpy(prefs->deficons_disabled_types, DEFAULT_DEFICONS_DISABLED_TYPES);
    prefs->deficons_folder_icon_mode = DEFAULT_DEFICONS_FOLDER_ICON_MODE;
    prefs->deficons_skip_system_assigns = DEFAULT_DEFICONS_SKIP_SYSTEM_ASSIGNS;
    prefs->deficons_log_created_icons = DEFAULT_DEFICONS_LOG_CREATED_ICONS;
    prefs->deficons_icon_size_mode = DEFAULT_DEFICONS_ICON_SIZE_MODE;
    prefs->deficons_thumbnail_border_mode = DEFAULT_DEFICONS_THUMBNAIL_BORDER_MODE;
    prefs->deficons_enable_text_previews = DEFAULT_DEFICONS_ENABLE_TEXT_PREVIEWS;
    prefs->deficons_enable_picture_previews = DEFAULT_DEFICONS_ENABLE_PICTURE_PREVIEWS;
    
    /* DefIcons Exclude Paths - Initialize with defaults */
    reset_deficons_exclude_paths_to_defaults(prefs);
    
    /* Logging and Debug Settings */
    prefs->logLevel = DEFAULT_LOG_LEVEL;
    
    /* Default Tool Validation Settings */
    prefs->validate_default_tools = DEFAULT_VALIDATE_DEFAULT_TOOLS;
    
    /* Default Tool Backup Settings */
    prefs->enable_default_tool_backup = DEFAULT_ENABLE_DEFAULT_TOOL_BACKUP;
    
    /* Backup Settings */
    prefs->backupPrefs.enableUndoBackup = FALSE;
    prefs->backupPrefs.useLha = TRUE;
    strcpy(prefs->backupPrefs.backupRootPath, "PROGDIR:Backups");
    prefs->backupPrefs.maxBackupsPerFolder = 3;
    
    /* DefIcons Palette Reduction Settings (v2 fields - must be last) */
    prefs->deficons_max_icon_colors = DEFAULT_DEFICONS_MAX_ICON_COLORS;
    prefs->deficons_dither_method = DEFAULT_DEFICONS_DITHER_METHOD;
    prefs->deficons_lowcolor_mapping = DEFAULT_DEFICONS_LOWCOLOR_MAPPING;
    prefs->deficons_ultra_mode = DEFAULT_DEFICONS_ULTRA_MODE;
    prefs->deficons_harmonised_palette = DEFAULT_DEFICONS_HARMONISED_PALETTE;

    /* Per-format picture thumbnail enable bitmask (v3 fields - must be last) */
    prefs->deficons_picture_formats_enabled = DEFAULT_DEFICONS_PICTURE_FORMATS_ENABLED;

    /* Upscaling control (v3 fields) */
    prefs->deficons_upscale_thumbnails = DEFAULT_DEFICONS_UPSCALE_THUMBNAILS;

    /* WHDLoad Integration (v4 fields) */
    prefs->deficons_skip_whdload_folders = DEFAULT_DEFICONS_SKIP_WHDLOAD_FOLDERS;

    /* Replace-iTidy-icons options (v5 fields) */
    prefs->deficons_replace_itidy_thumbnails    = DEFAULT_DEFICONS_REPLACE_ITIDY_THUMBNAILS;
    prefs->deficons_replace_itidy_text_previews = DEFAULT_DEFICONS_REPLACE_ITIDY_TEXT_PREVIEWS;
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
            prefs->centerIconsInColumn = TRUE;
            prefs->useColumnWidthOptimization = TRUE;
            prefs->textAlignment = TEXT_ALIGN_BOTTOM;
            prefs->minIconsPerRow = 2;
            prefs->maxIconsPerRow = 0;  /* AUTO: Calculate from screen width */
            prefs->maxWindowWidthPct = 55;
            prefs->aspectRatio = 2000;  /* 2.0 * 1000 (Wide) */
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
            prefs->centerIconsInColumn = TRUE;
            prefs->useColumnWidthOptimization = TRUE;
            prefs->textAlignment = TEXT_ALIGN_BOTTOM;
            prefs->minIconsPerRow = 2;
            prefs->maxIconsPerRow = 0;  /* AUTO: Calculate from screen width */
            prefs->maxWindowWidthPct = 45;
            prefs->aspectRatio = 1300;  /* 1.3 * 1000 */
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
            prefs->centerIconsInColumn = TRUE;
            prefs->useColumnWidthOptimization = TRUE;
            prefs->textAlignment = TEXT_ALIGN_BOTTOM;
            prefs->minIconsPerRow = 3;  /* Wider minimum */
            prefs->maxIconsPerRow = 0;  /* AUTO: Calculate from screen width */
            prefs->maxWindowWidthPct = 60;
            prefs->aspectRatio = 1500;  /* 1.5 * 1000 */
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
            prefs->centerIconsInColumn = TRUE;
            prefs->useColumnWidthOptimization = TRUE;
            prefs->textAlignment = TEXT_ALIGN_BOTTOM;
            prefs->minIconsPerRow = 2;
            prefs->maxIconsPerRow = 0;  /* AUTO: Calculate from screen width */
            prefs->maxWindowWidthPct = 40;
            prefs->aspectRatio = 1400;  /* 1.4 * 1000 */
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

/*========================================================================*/
/**
 * @brief Initialize global preferences to default values
 * 
 * Call this once at application startup to initialize the global
 * preference singleton with sensible defaults (Classic preset).
 * 
 * @note This MUST be called before any other preference functions
 */
/*========================================================================*/
void InitializeGlobalPreferences(void)
{
    InitLayoutPreferences(&g_AppPreferences);
}

/*========================================================================*/
/**
 * @brief Get read-only access to global preferences
 * 
 * Returns a const pointer to the global preference singleton. Use this
 * to access current application preferences from anywhere in the code.
 * 
 * @return Const pointer to global LayoutPreferences
 * 
 * @example
 *   const LayoutPreferences *prefs = GetGlobalPreferences();
 *   if (prefs->skipHiddenFolders) { ... }
 */
/*========================================================================*/
const LayoutPreferences* GetGlobalPreferences(void)
{
    return &g_AppPreferences;
}

/*========================================================================*/
/**
 * @brief Update global preferences with new values
 * 
 * Updates the global preference singleton with values from the provided
 * LayoutPreferences structure. Typically called from GUI event handlers
 * when the user clicks "Apply" or "OK".
 * 
 * @param newPrefs Pointer to LayoutPreferences with new values
 * 
 * @note If newPrefs is NULL, this function does nothing
 */
/*========================================================================*/
void UpdateGlobalPreferences(const LayoutPreferences *newPrefs)
{
    if (newPrefs == NULL)
        return;
    
    /* Copy entire structure */
    memcpy(&g_AppPreferences, newPrefs, sizeof(LayoutPreferences));
}

/*========================================================================*/
/**
 * @brief Set the scan path in global preferences
 * 
 * Convenience setter for updating just the folder path. Useful for
 * simple operations that don't need to construct a full LayoutPreferences
 * structure.
 * 
 * @param path New folder path (copied to internal buffer)
 * 
 * @note Path is limited to 255 characters
 * @note If path is NULL or empty, this function does nothing
 */
/*========================================================================*/
void SetGlobalScanPath(const char *path)
{
    if (path == NULL || path[0] == '\0')
        return;
    
    /* Copy path safely */
    strncpy(g_AppPreferences.folder_path, path, sizeof(g_AppPreferences.folder_path) - 1);
    g_AppPreferences.folder_path[sizeof(g_AppPreferences.folder_path) - 1] = '\0';
}

/*========================================================================*/
/**
 * @brief Set recursive scanning mode in global preferences
 * 
 * Convenience setter for toggling recursive subdirectory scanning.
 * 
 * @param recursive TRUE to enable recursive scanning, FALSE to disable
 */
/*========================================================================*/
void SetGlobalRecursiveMode(BOOL recursive)
{
    g_AppPreferences.recursive_subdirs = recursive;
}

/*========================================================================*/
/**
 * @brief Set skip hidden folders mode in global preferences
 * 
 * Convenience setter for controlling whether hidden folders (those
 * without .info files) are processed.
 * 
 * @param skip TRUE to skip hidden folders, FALSE to process them
 */
/*========================================================================*/
void SetGlobalSkipHiddenFolders(BOOL skip)
{
    g_AppPreferences.skipHiddenFolders = skip;
}

/*========================================================================*/
/**
 * @brief Check if a DefIcons type is enabled
 * 
 * Checks whether automatic icon creation is enabled for a specific
 * DefIcons type by looking in the disabled types list.
 * 
 * @param prefs Pointer to LayoutPreferences structure
 * @param type_name Type name to check (e.g., "tool", "music", "picture")
 * 
 * @return TRUE if type is enabled, FALSE if disabled
 * 
 * @note Returns FALSE if deficons_disabled_types contains type_name
 * @note Returns TRUE if enable_deficons_icon_creation is FALSE (feature disabled)
 */
/*========================================================================*/
BOOL is_deficon_type_enabled(const LayoutPreferences *prefs, const char *type_name)
{
    char *ptr;
    char temp_buffer[256];
    char search_token[MAX_DEFICONS_TYPE_NAME + 3];  /* ", type_name," */
    
    if (prefs == NULL || type_name == NULL || type_name[0] == '\0')
        return FALSE;
    
    /* If feature is disabled, consider all types enabled (no filtering) */
    if (!prefs->enable_deficons_icon_creation)
        return TRUE;
    
    /* If no disabled types, all are enabled */
    if (prefs->deficons_disabled_types[0] == '\0')
        return TRUE;
    
    /* Build search token: ",type_name," to avoid partial matches */
    /* Example: ",tool," won't match ",tooltip," */
    sprintf(search_token, ",%s,", type_name);
    
    /* Build searchable string: ",type1,type2,type3," */
    sprintf(temp_buffer, ",%s,", prefs->deficons_disabled_types);
    
    /* Search for token */
    ptr = strstr(temp_buffer, search_token);
    
    return (ptr == NULL);  /* TRUE if NOT found in disabled list */
}

/*========================================================================*/
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
 * 
 * @note Does nothing if type is already in the list
 * @note Fails silently if list is full
 */
/*========================================================================*/
BOOL add_disabled_deficon_type(LayoutPreferences *prefs, const char *type_name)
{
    char temp_buffer[256];
    
    if (prefs == NULL || type_name == NULL || type_name[0] == '\0')
        return FALSE;
    
    /* Check if already in list */
    if (!is_deficon_type_enabled(prefs, type_name))
        return TRUE;  /* Already disabled */
    
    /* Add to list */
    if (prefs->deficons_disabled_types[0] == '\0')
    {
        /* First entry */
        strncpy(prefs->deficons_disabled_types, type_name, sizeof(prefs->deficons_disabled_types) - 1);
        prefs->deficons_disabled_types[sizeof(prefs->deficons_disabled_types) - 1] = '\0';
    }
    else
    {
        /* Append with comma */
        snprintf(temp_buffer, sizeof(temp_buffer), "%s,%s", prefs->deficons_disabled_types, type_name);
        strncpy(prefs->deficons_disabled_types, temp_buffer, sizeof(prefs->deficons_disabled_types) - 1);
        prefs->deficons_disabled_types[sizeof(prefs->deficons_disabled_types) - 1] = '\0';
    }
    
    return TRUE;
}

/*========================================================================*/
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
 * 
 * @note Does nothing if type is not in the list
 */
/*========================================================================*/
BOOL remove_disabled_deficon_type(LayoutPreferences *prefs, const char *type_name)
{
    char temp_buffer[256];
    char *tokens[64];
    int token_count = 0;
    char *token;
    char *context = NULL;
    int i;
    
    if (prefs == NULL || type_name == NULL || type_name[0] == '\0')
        return FALSE;
    
    /* Check if in list */
    if (is_deficon_type_enabled(prefs, type_name))
        return TRUE;  /* Already enabled */
    
    /* Copy to temp buffer for tokenization */
    strncpy(temp_buffer, prefs->deficons_disabled_types, sizeof(temp_buffer) - 1);
    temp_buffer[sizeof(temp_buffer) - 1] = '\0';
    
    /* Tokenize by comma */
    token = strtok(temp_buffer, ",");
    while (token != NULL && token_count < 64)
    {
        /* Skip the type we want to remove */
        if (strcmp(token, type_name) != 0)
        {
            tokens[token_count++] = token;
        }
        token = strtok(NULL, ",");
    }
    
    /* Rebuild disabled types string */
    prefs->deficons_disabled_types[0] = '\0';
    for (i = 0; i < token_count; i++)
    {
        if (i > 0)
        {
            strncat(prefs->deficons_disabled_types, ",", sizeof(prefs->deficons_disabled_types) - strlen(prefs->deficons_disabled_types) - 1);
        }
        strncat(prefs->deficons_disabled_types, tokens[i], sizeof(prefs->deficons_disabled_types) - strlen(prefs->deficons_disabled_types) - 1);
    }
    
    return TRUE;
}

/*========================================================================*/
/**
 * @brief Clear all disabled types (enable all)
 * 
 * Clears the disabled types list, enabling automatic icon creation
 * for all DefIcons categories.
 * 
 * @param prefs Pointer to LayoutPreferences structure
 */
/*========================================================================*/
void clear_disabled_deficon_types(LayoutPreferences *prefs)
{
    if (prefs == NULL)
        return;
    
    prefs->deficons_disabled_types[0] = '\0';
}

/*========================================================================*/
/**
 * @brief Apply default disabled types after loading deficons tree
 * 
 * Sets the default disabled types (tool, prefs, iff, key, kickstart) based on
 * the screenshot defaults. Silently skips any types that don't exist in the
 * loaded deficons tree (since deficons.prefs is an external user system file).
 * 
 * @param prefs Pointer to LayoutPreferences structure
 * @param tree Pointer to DefIcons type tree array (DeficonTypeTreeNode*)
 * @param count Number of nodes in tree
 */
/*========================================================================*/
void apply_default_deficon_type_selections(LayoutPreferences *prefs, 
                                           const void *tree_void, 
                                           int count)
{
    const char *default_disabled_types[] = {
        "tool",
        "prefs",
        "iff",
        "key",
        "kickstart",
        NULL
    };
    int i, j;
    const DeficonTypeTreeNode *tree = (const DeficonTypeTreeNode *)tree_void;
    
    if (prefs == NULL || tree == NULL || count == 0)
        return;
    
    /* Clear existing disabled types first */
    clear_disabled_deficon_types(prefs);
    
    /* Add each default disabled type if it exists in the tree */
    for (i = 0; default_disabled_types[i] != NULL; i++)
    {
        const char *type_name = default_disabled_types[i];
        BOOL type_exists = FALSE;
        
        /* Check if this type exists in the loaded tree (generation 1 only) */
        for (j = 0; j < count; j++)
        {
            if (tree[j].generation == 1 && 
                strcmp(tree[j].type_name, type_name) == 0)
            {
                type_exists = TRUE;
                break;
            }
        }
        
        /* Only add to disabled list if it exists in the tree */
        if (type_exists)
        {
            add_disabled_deficon_type(prefs, type_name);
        }
        /* Silently skip types that don't exist in user's deficons.prefs */
    }
}

/*========================================================================*/
/* DefIcons Exclude Paths Helper Functions                               */
/*========================================================================*/

/**
 * @brief Default exclude paths list (system directories)
 * 
 * Uses DEVICE: placeholder which gets substituted with the actual
 * volume being scanned at runtime (e.g., DEVICE:Fonts becomes SYS:Fonts
 * when scanning SYS:, or Work:Fonts when scanning Work:).
 */
static const char *DEFAULT_EXCLUDE_PATHS[] = {
    "DEVICE:Fonts",
    "DEVICE:Locale",
    "DEVICE:Classes",
    "DEVICE:Libs",
    "DEVICE:C",
    "DEVICE:Rexxc",
    "DEVICE:T",
    "DEVICE:L",
    "DEVICE:Devs",
    "DEVICE:Resources",
    "DEVICE:System",
    "DEVICE:Storage",
    "DEVICE:Expansion",
    "DEVICE:Kickstart",
    "DEVICE:Env",
    "DEVICE:Envarc",
    "DEVICE:Prefs/Env-Archive",
    NULL
};

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
/*========================================================================*/
void reset_deficons_exclude_paths_to_defaults(LayoutPreferences *prefs)
{
    int i;
    
    if (prefs == NULL)
        return;
    
    /* Clear existing paths */
    prefs->deficons_exclude_path_count = 0;
    memset(prefs->deficons_exclude_paths, 0, sizeof(prefs->deficons_exclude_paths));
    
    /* Copy default paths */
    for (i = 0; DEFAULT_EXCLUDE_PATHS[i] != NULL && i < MAX_DEFICONS_EXCLUDE_PATHS; i++)
    {
        strncpy(prefs->deficons_exclude_paths[i], DEFAULT_EXCLUDE_PATHS[i], DEFICONS_EXCLUDE_PATH_LENGTH - 1);
        prefs->deficons_exclude_paths[i][DEFICONS_EXCLUDE_PATH_LENGTH - 1] = '\0';
        prefs->deficons_exclude_path_count++;
    }
}

/*========================================================================*/
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
/*========================================================================*/
BOOL add_deficons_exclude_path(LayoutPreferences *prefs, const char *path)
{
    int i;
    
    if (prefs == NULL || path == NULL || path[0] == '\0')
        return FALSE;
    
    /* Check if list is full */
    if (prefs->deficons_exclude_path_count >= MAX_DEFICONS_EXCLUDE_PATHS)
        return FALSE;
    
    /* Check for duplicates (case-insensitive) */
    for (i = 0; i < prefs->deficons_exclude_path_count; i++)
    {
        if (Stricmp(prefs->deficons_exclude_paths[i], path) == 0)
            return FALSE;  /* Already exists */
    }
    
    /* Add new path */
    strncpy(prefs->deficons_exclude_paths[prefs->deficons_exclude_path_count], path, DEFICONS_EXCLUDE_PATH_LENGTH - 1);
    prefs->deficons_exclude_paths[prefs->deficons_exclude_path_count][DEFICONS_EXCLUDE_PATH_LENGTH - 1] = '\0';
    prefs->deficons_exclude_path_count++;
    
    return TRUE;
}

/*========================================================================*/
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
/*========================================================================*/
BOOL remove_deficons_exclude_path(LayoutPreferences *prefs, int index)
{
    int i;
    
    if (prefs == NULL || index < 0 || index >= prefs->deficons_exclude_path_count)
        return FALSE;
    
    /* Shift remaining entries down */
    for (i = index; i < prefs->deficons_exclude_path_count - 1; i++)
    {
        strncpy(prefs->deficons_exclude_paths[i], prefs->deficons_exclude_paths[i + 1], DEFICONS_EXCLUDE_PATH_LENGTH);
    }
    
    /* Clear last entry */
    memset(prefs->deficons_exclude_paths[prefs->deficons_exclude_path_count - 1], 0, DEFICONS_EXCLUDE_PATH_LENGTH);
    prefs->deficons_exclude_path_count--;
    
    return TRUE;
}

/*========================================================================*/
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
/*========================================================================*/
BOOL modify_deficons_exclude_path(LayoutPreferences *prefs, int index, const char *new_path)
{
    int i;
    
    if (prefs == NULL || new_path == NULL || new_path[0] == '\0')
        return FALSE;
    
    if (index < 0 || index >= prefs->deficons_exclude_path_count)
        return FALSE;
    
    /* Check for duplicates with other entries (case-insensitive) */
    for (i = 0; i < prefs->deficons_exclude_path_count; i++)
    {
        if (i != index && Stricmp(prefs->deficons_exclude_paths[i], new_path) == 0)
            return FALSE;  /* Duplicate found */
    }
    
    /* Update path */
    strncpy(prefs->deficons_exclude_paths[index], new_path, DEFICONS_EXCLUDE_PATH_LENGTH - 1);
    prefs->deficons_exclude_paths[index][DEFICONS_EXCLUDE_PATH_LENGTH - 1] = '\0';
    
    return TRUE;
}


/* End of layout_preferences.c */

