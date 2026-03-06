// opens a small window to get the font width settings

#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <prefs/font.h>
#include <libraries/iffparse.h>
#include <proto/iffparse.h>

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

#include "itidy_types.h"
#include "Settings/WorkbenchPrefs.h"
#include "Settings/IControlPrefs.h"
#include "icon_management.h"
#include "icon_types.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "Settings/get_fonts.h"

/* Forward declaration - defined in backup_paths.c */
extern BOOL IsRootFolder(const char *path);

/* Default window size for empty folders ("Show All" mode with no .info files)
 * These dimensions are for the content area only (excluding window chrome).
 * When iTidy finds a folder with no icons, it resizes the window to these
 * standard dimensions to maintain UI consistency. */
#define EMPTY_FOLDER_CONTENT_WIDTH  230
#define EMPTY_FOLDER_CONTENT_HEIGHT 80

/* Content area margin formulas for non-root drawer windows.
 * These values were empirically determined through systematic testing (2025-11-26):
 * - Borderless mode uses fixed margins (1,1) regardless of emboss size
 * - Normal mode margins scale with emboss size: horizontal = emboss×2, vertical = (emboss-1)×2
 * - Tested with emboss sizes 1, 2, 3 - formulas confirmed accurate
 * See docs/WINDOW_POSITIONING_OFFSETS.md for full testing documentation */
#define DRAWER_CONTENT_LEFT_MARGIN(borderless, embossSize) \
    ((borderless) ? 1 : ((embossSize) * 2))
#define DRAWER_CONTENT_TOP_MARGIN(borderless, embossSize) \
    ((borderless) ? 1 : (((embossSize) - 1) * 2))

struct TextFont *iconFont;



#define FP_WBFONT 0

void CleanupWindow(void)
{
#if PLATFORM_AMIGA
    if (font)
    {
        CloseFont(font); // Close the opened font
    }
    if (window)
    {
        CloseWindow(window);
    }
    if (screen)
    {
        UnlockPubScreen("Workbench", screen);
    }
#endif
}

/*------------------------------------------------------------------------*/
/**
 * @brief Count path depth (number of '/' separators)
 * 
 * Counts the number of directory levels in a path to help distinguish
 * between folders with the same name at different depths.
 * Examples:
 *   "Workbench:Programs" → 0
 *   "Workbench:Programs/Games" → 1
 *   "Workbench:Programs/MagicWB/Programs" → 2
 * 
 * @param path Full path to analyze
 * @return Number of '/' separators (depth level)
 */
/*------------------------------------------------------------------------*/
static int CountPathDepth(const char *path)
{
    int depth = 0;
    const char *p;
    
    if (!path)
        return 0;
    
    for (p = path; *p != '\0'; p++)
    {
        if (*p == '/')
            depth++;
    }
    
    return depth;
}

/*------------------------------------------------------------------------*/
/**
 * @brief Extract folder name from full path
 * 
 * Extracts the last component of a path to use for window title matching.
 * Examples:
 *   "Work:Games/Action" → "Action"
 *   "DH0:Utilities" → "Utilities"
 *   "Workbench:" → "Workbench:"
 * 
 * @param path Full path to folder
 * @param folderName Buffer to receive folder name
 * @param bufferSize Size of folderName buffer
 */
/*------------------------------------------------------------------------*/
static void ExtractFolderNameFromPath(const char *path, char *folderName, size_t bufferSize)
{
    const char *lastSlash;
    const char *lastColon;
    const char *nameStart;
    
    if (!path || !folderName || bufferSize == 0)
    {
        if (folderName && bufferSize > 0)
            folderName[0] = '\0';
        return;
    }
    
    /* Find last '/' or ':' in path */
    lastSlash = strrchr(path, '/');
    lastColon = strrchr(path, ':');
    
    /* Determine where the folder name starts */
    if (lastSlash != NULL)
    {
        /* Path has '/' - use everything after last '/' */
        nameStart = lastSlash + 1;
    }
    else if (lastColon != NULL)
    {
        /* Path has ':' but no '/' - check if it's a root volume */
        if (*(lastColon + 1) == '\0')
        {
            /* Root volume like "DH0:" - use entire path including colon */
            nameStart = path;
        }
        else
        {
            /* Path like "DH0:Folder" - use everything after colon */
            nameStart = lastColon + 1;
        }
    }
    else
    {
        /* No path separators - use entire string */
        nameStart = path;
    }
    
    /* Copy folder name to buffer */
    strncpy(folderName, nameStart, bufferSize - 1);
    folderName[bufferSize - 1] = '\0';
}

void resizeFolderToContents(char *dirPath, IconArray *iconArray,
                           const LayoutPreferences *prefs)
{
#if PLATFORM_AMIGA
    int i, maxWidth = 0, maxHeight = 0;
    /* No extra margins needed - layout algorithm already positions icons with ICON_START_X/Y spacing
     * on all sides, creating balanced padding. Additional margins would double-count the spacing. */
    int rightMargin = 0;
    int bottomMargin = 0;


    log_debug(LOG_GENERAL, "resizeFolderToContents ENTRY: dirPath='%s', iconArray->size=%d\n", 
                  dirPath, iconArray ? iconArray->size : -1);


    if (user_folderViewMode == DDVM_BYICON)
    {
        /* Calculate content dimensions from icon positions */
        for (i = 0; i < iconArray->size; i++)
        {
            int iconRight;
            
            /* FIX: Handle case where text is wider than icon (centered text).
             * In this case, icon_x is shifted right, so icon_x + max_width extends too far.
             * Visual Right = icon_x + (icon_width + text_width) / 2
             */
            if (iconArray->array[i].text_width > iconArray->array[i].icon_width)
            {
                iconRight = iconArray->array[i].icon_x + (iconArray->array[i].icon_width + iconArray->array[i].text_width) / 2;
            }
            else
            {
                iconRight = iconArray->array[i].icon_x + iconArray->array[i].icon_max_width;
            }
            
            int iconBottom = iconArray->array[i].icon_y + iconArray->array[i].icon_max_height;
            
            maxWidth = MAX(maxWidth, iconRight);
            maxHeight = MAX(maxHeight, iconBottom);
        }
        
        /* Add margins to content dimensions */
        maxWidth += rightMargin;
        maxHeight += bottomMargin;
        

        log_debug(LOG_GENERAL, "Content dimensions: %d×%d (with margins)\n",                       maxWidth, maxHeight);

    }
    else
    {
        maxWidth = WindowWidthTextOnly;
        maxHeight = WindowHeightTextOnly;
    }


    log_debug(LOG_GENERAL, "Calculated maxWidth=%d, maxHeight=%d before calling repoistionWindow", maxWidth, maxHeight);


    /* Empty folder handling: If no icons found (Show All mode with no .info files),
     * use standard default dimensions instead of skipping resize */
    if (iconArray->size == 0 && user_folderViewMode == DDVM_BYICON)
    {
        maxWidth = EMPTY_FOLDER_CONTENT_WIDTH;
        maxHeight = EMPTY_FOLDER_CONTENT_HEIGHT;
        log_debug(LOG_GENERAL, "No icons in folder - using default empty folder size: %dx%d\n",
                 maxWidth, maxHeight);
    }

    /* Note: repoistionWindow will clamp to screen size and add chrome */
    repoistionWindow(dirPath, maxWidth, maxHeight, prefs);
    
#else
    (void)dirPath;
    (void)iconArray;
    (void)prefs;
#endif
}

void GetWorkbenchIconFont(char *fontName, int *fontSize)
{
#if PLATFORM_AMIGA
    /* VBCC MIGRATION NOTE (Stage 4): Fixed BPTR type for file handle */
    struct IFFHandle *iff;
    struct ContextNode *cn;
    struct FontPrefs fontPrefs;
    BPTR file;

    // Default fallback font
    strcpy(fontName, "topaz.font");
    *fontSize = 8;

    // Allocate IFF parser
    iff = AllocIFF();
    if (!iff)
    {
        CONSOLE_ERROR("Failed to allocate IFF parser.\n");
        return;
    }

    // Open Workbench font preferences file
    file = Open("ENV:sys/font.prefs", MODE_OLDFILE);
    if (!file)
    {
        CONSOLE_WARNING("Failed to open ENV:sys/font.prefs. Using default: %s, size: %d\n", fontName, *fontSize);
        FreeIFF(iff);
        return;
    }

    iff->iff_Stream = file;
    InitIFFasDOS(iff);

    if (OpenIFF(iff, IFFF_READ) != 0)
    {
        CONSOLE_ERROR("Failed to open IFF structure.\n");
        Close(file);
        FreeIFF(iff);
        return;
    }

    // Scan IFF chunks
    while ((cn = CurrentChunk(iff)) != NULL)
    {
        if (cn->cn_ID == ID_FONT)  // Found a FONT chunk
        {
            if (ReadChunkBytes(iff, &fontPrefs, sizeof(struct FontPrefs)) == sizeof(struct FontPrefs))
            {
                if (fontPrefs.fp_Type == FP_WBFONT)  // Workbench Icon Font
                {
                    strncpy(fontName, fontPrefs.fp_Name, FONTNAMESIZE - 1);
                    fontName[FONTNAMESIZE - 1] = '\0';  // Ensure null termination
                    *fontSize = fontPrefs.fp_TextAttr.ta_YSize;
                    break;
                }
            }
        }
        if (ParseIFF(iff, IFFPARSE_STEP) == IFFERR_EOF)
            break;
    }

    CloseIFF(iff);
    Close(file);
    FreeIFF(iff);

    // Debug Output
    #ifdef DEBUG
    CONSOLE_DEBUG("Workbench Icon Font: %s, Size: %d\n", fontName, *fontSize);
    #endif
#else
    (void)fontName;
    (void)fontSize;
#endif
}

// Function to load and apply the Workbench Icon Font
void SetIconFont(void)
{
#if PLATFORM_AMIGA
    //char fontName[32];
    //int fontSize;
    struct TextAttr textAttr;

    // Get the Workbench icon font settings
    //    GetWorkbenchIconFont(fontName, &fontSize);
    //fontPrefs = (FontPref *)malloc(sizeof(FontPref));
    //fontPrefs = get_fonts();
    //strncpy(fontName, fontPrefs->name, 31);
    //fontSize = fontPrefs->size;


    // Set up TextAttr struct
    textAttr.ta_Name = fontPrefs->name;
    textAttr.ta_YSize = fontPrefs->size;
    textAttr.ta_Style = 0;
    textAttr.ta_Flags = 0;

    // Load the font
    iconFont = OpenDiskFont(&textAttr);

    if (iconFont)
    {
        SetFont(rastPort, iconFont);
        CONSOLE_STATUS("Workbench Icon Font: %s (%d pt) loaded\n", 
               fontPrefs->name, (int)fontPrefs->size);
    }
    else
    {
        CONSOLE_WARNING("Failed to load Workbench Icon Font, using default screen font.\n");
        SetFont(rastPort, screen->RastPort.Font);
    }
#endif
}

// Function to initialize a window and apply the Workbench Icon Font
int InitializeWindow(void)
{
#if PLATFORM_AMIGA
    int windowWidth = 1;
    int windowHeight = 1;
    int windowLeft;
    int windowTop;

    screen = LockPubScreen("Workbench");
    if (!screen)
    {
        CONSOLE_ERROR("Failed to lock Workbench screen.\n");
        return 0;
    }

    /* Initialize global screen dimensions from Workbench screen */
    screenWidth = screen->Width;
    screenHight = screen->Height;

    windowLeft = screen->Width - windowWidth;
    windowTop = screen->Height - windowHeight;

    window = OpenWindowTags(NULL,
                            WA_Left, windowLeft,
                            WA_Top, windowTop,
                            WA_Width, windowWidth,
                            WA_Height, windowHeight,
                            WA_Flags, WFLG_BACKDROP,
                            WA_CustomScreen, screen,
                            TAG_DONE);
    if (!window)
    {
        CONSOLE_ERROR("Failed to open window.\n");
        UnlockPubScreen("Workbench", screen);
        return 0;
    }

    rastPort = window->RPort;
    if (!rastPort)
    {
        CONSOLE_ERROR("RastPort failed to create.\n");
        return 0;
    }

    // Apply Workbench Icon Font
    SetIconFont();

    return 1;
#else
    return 0;
#endif
}

/*------------------------------------------------------------------------*/
/**
 * @brief Extract parent directory path from a full path
 * 
 * For "PC:Programming/iTidy" returns "PC:Programming"
 * For "PC:Programming" returns "PC:"
 * For "PC:" returns empty string (root has no parent)
 * 
 * @param path Full directory path
 * @param parentPath Output buffer for parent path
 * @param bufferSize Size of output buffer
 * @return BOOL TRUE if parent exists, FALSE if path is root
 */
/*------------------------------------------------------------------------*/
static BOOL GetParentPath(const char *path, char *parentPath, int bufferSize)
{
    char *lastSlash;
    char *lastColon;
    int pathLen;
    
    if (!path || !parentPath || bufferSize < 1)
    {
        return FALSE;
    }
    
    /* Check if this is a root directory (ends with ':') */
    pathLen = strlen(path);
    if (pathLen > 0 && path[pathLen - 1] == ':')
    {
        /* Root directory has no parent */
        parentPath[0] = '\0';
        return FALSE;
    }
    
    /* Copy path to output buffer */
    strncpy(parentPath, path, bufferSize - 1);
    parentPath[bufferSize - 1] = '\0';
    
    /* Find last '/' in the path */
    lastSlash = strrchr(parentPath, '/');
    lastColon = strrchr(parentPath, ':');
    
    if (lastSlash != NULL)
    {
        /* Path has subdirectories - parent is everything up to last slash */
        *lastSlash = '\0';
        return TRUE;
    }
    else if (lastColon != NULL)
    {
        /* Path is directly under root - parent is the root */
        *(lastColon + 1) = '\0';  /* Keep the colon */
        return TRUE;
    }
    else
    {
        /* Invalid path format */
        parentPath[0] = '\0';
        return FALSE;
    }
}

void repoistionWindow(char *dirPath, int winWidth, int winHeight, const LayoutPreferences *prefs)
{
#if PLATFORM_AMIGA
    int posTop = 0, posLeft = 0;
    int finalWidth, finalHeight;
    int maxUsableHeight;
    folderWindowSize newFolderInfo;
    folderWindowSize currentWindowInfo;
    UWORD currentViewMode;
    WindowPositionMode positionMode;
    BOOL hasCurrentPosition = FALSE;
    
    /* Get window position mode from preferences (default to Center Screen if NULL) */
    positionMode = (prefs != NULL) ? prefs->windowPositionMode : WINDOW_POS_CENTER_SCREEN;
    
    log_debug(LOG_GUI, "repoistionWindow() START - Path: %s, content: %dx%d, mode: %d\n",
              dirPath, winWidth, winHeight, positionMode);
    
#ifdef DEBUG
    log_debug(LOG_GUI, "  Padding: %d, disableVolumeGauge: %d", 
                  PADDING_WIDTH, prefsWorkbench.disableVolumeGauge);
    log_debug(LOG_GUI, "  IControl: currentBarWidth=%d, currentLeftBarWidth=%d, currentWindowBarHeight=%d, currentBarHeight=%d",
                  prefsIControl.currentBarWidth, prefsIControl.currentLeftBarWidth,
                  prefsIControl.currentWindowBarHeight, prefsIControl.currentBarHeight);
#endif
    
    /* WINDOW_POS_NO_CHANGE: Don't resize or move the window at all */
    if (positionMode == WINDOW_POS_NO_CHANGE)
    {
        log_info(LOG_GUI, "repoistionWindow: %s -> No Change (skipped)\n", dirPath);
        return;
    }
    
    /* Read current window position for WINDOW_POS_KEEP_POSITION mode */
    if (positionMode == WINDOW_POS_KEEP_POSITION)
    {
        hasCurrentPosition = GetFolderWindowSettings(dirPath, &currentWindowInfo, &currentViewMode);
        if (hasCurrentPosition)
        {
            log_debug(LOG_GUI, "  Current window position: left=%d, top=%d, width=%d, height=%d\n",
                     currentWindowInfo.left, currentWindowInfo.top, 
                     currentWindowInfo.width, currentWindowInfo.height);
        }
        else
        {
            log_debug(LOG_GUI, "  Could not read current window position - will center instead\n");
        }
    }
    
    /* Add window chrome (borders, scrollbars, etc.) to width */
    finalWidth = winWidth + prefsIControl.currentBarWidth + prefsIControl.currentLeftBarWidth + (PADDING_WIDTH * 2);
    log_debug(LOG_GUI, "  Chrome: content=%d + leftBar=%d + rightBar=%d + padding=%d = %d (pre-scrollbar)\n",
              winWidth, prefsIControl.currentLeftBarWidth, prefsIControl.currentBarWidth, PADDING_WIDTH * 2, finalWidth);
    
    finalWidth += prefsIControl.currentBarWidth;  /* Add space for vertical scrollbar (always present) */
    
    if (!prefsWorkbench.disableVolumeGauge && IsRootDirectorySimple(dirPath))
    {
        finalWidth += prefsIControl.currentCGaugeWidth;

#ifdef DEBUG
        log_debug(LOG_GUI, "  Root dir detected and VolumeGauge enabled. Adding CGauge width: %d", 
                      prefsIControl.currentCGaugeWidth);
#endif
    }
    
    /* Add window chrome (title bar, borders, scrollbars, etc.) to height */
    finalHeight = winHeight + prefsIControl.currentWindowBarHeight + prefsIControl.currentBarHeight + (PADDING_HEIGHT * 2);
    finalHeight += prefsIControl.currentBarHeight;  /* Add space for horizontal scrollbar (always present) */
    
    log_debug(LOG_GUI, "  Calculated window with chrome: %dx%d\n", finalWidth, finalHeight);
    
    /* Calculate maximum usable height (account for Workbench title bar) */
    maxUsableHeight = screenHight - prefsIControl.currentTitleBarHeight;
    
    log_debug(LOG_GUI, "  Screen size: %dx%d, Max usable height: %d\n", 
             screenWidth, screenHight, maxUsableHeight);
    
    /* Clamp window dimensions to screen size (creates scrollbars if needed) */
    if (finalWidth > screenWidth)
    {
        log_info(LOG_GUI, "  repoistionWindow: %s - width clamped %d->%d (horizontal scrollbar)\n",
                 dirPath, finalWidth, screenWidth);
        finalWidth = screenWidth;
    }
    
    if (finalHeight > maxUsableHeight)
    {
        log_info(LOG_GUI, "  repoistionWindow: %s - height clamped %d->%d (vertical scrollbar)\n",
                 dirPath, finalHeight, maxUsableHeight);
        finalHeight = maxUsableHeight;
    }
    
    log_debug(LOG_GUI, "  Final window dimensions: %dx%d (after clamping)\n", finalWidth, finalHeight);
    
    /* Position window vertically */
    if (finalHeight >= maxUsableHeight)
    {
        /* Tall/overflow window - position just below Workbench title bar */
        posTop = prefsIControl.currentTitleBarHeight;
        log_debug(LOG_GUI, "  Overflow height - positioning at top: %d\n", posTop);
    }
    else
    {
        /* Normal window - center vertically in available space */
        posTop = (screenHight - finalHeight) / 2;
        log_debug(LOG_GUI, "  Normal height - centering vertically: %d\n", posTop);
    }
    
    /* Position window horizontally based on mode */
    if (positionMode == WINDOW_POS_CENTER_SCREEN)
    {
        /* WINDOW_POS_CENTER_SCREEN: Always center horizontally (classic behavior) */
        posLeft = (screenWidth - finalWidth) / 2;
        log_debug(LOG_GUI, "  Mode: Center Screen - centering at left=%d\n", posLeft);
    }
    else if (positionMode == WINDOW_POS_KEEP_POSITION)
    {
        /* WINDOW_POS_KEEP_POSITION: Keep current position, pull back if off-screen */
        if (hasCurrentPosition)
        {
            /* Use current position */
            posLeft = currentWindowInfo.left;
            posTop = currentWindowInfo.top;
            
            log_debug(LOG_GUI, "  Mode: Keep Position - using current position left=%d, top=%d\n", 
                     posLeft, posTop);
            
            /* Check if window would go off-screen and pull back if needed */
            if (posLeft + finalWidth > screenWidth)
            {
                posLeft = screenWidth - finalWidth;
                if (posLeft < 0) posLeft = 0;
                log_debug(LOG_GUI, "    Window extends beyond right edge - pulled back to left=%d\n", posLeft);
            }
            
            if (posLeft < 0)
            {
                posLeft = 0;
                log_debug(LOG_GUI, "    Window extends beyond left edge - pulled back to left=%d\n", posLeft);
            }
            
            if (posTop + finalHeight > screenHight)
            {
                posTop = screenHight - finalHeight;
                if (posTop < prefsIControl.currentTitleBarHeight)
                {
                    posTop = prefsIControl.currentTitleBarHeight;
                }
                log_debug(LOG_GUI, "    Window extends beyond bottom edge - pulled back to top=%d\n", posTop);
            }
            
            if (posTop < prefsIControl.currentTitleBarHeight)
            {
                posTop = prefsIControl.currentTitleBarHeight;
                log_debug(LOG_GUI, "    Window extends above title bar - pulled down to top=%d\n", posTop);
            }
        }
        else
        {
            /* No current position available - fall back to center */
            posLeft = (screenWidth - finalWidth) / 2;
            log_debug(LOG_GUI, "  Mode: Keep Position - no current position, falling back to center at left=%d\n", 
                     posLeft);
        }
    }
    else if (positionMode == WINDOW_POS_NEAR_PARENT)
    {
        /* WINDOW_POS_NEAR_PARENT: Position at bottom-right of parent icon */
        char parentPath[256];
        char childIconPath[512];
        folderWindowSize parentWindowInfo;
        UWORD parentViewMode;
        BOOL hasParent = FALSE;
        BOOL hasParentGeometry = FALSE;
        IconDetailsFromDisk childIconDetails;
        BOOL hasChildIcon = FALSE;
        int proposedLeft, proposedTop;
        BOOL fitsOnScreen;
        
        log_debug(LOG_GUI, "  Mode: Near Parent - attempting to position at bottom-right of parent icon\n");
        
        /* Get parent directory path */
        hasParent = GetParentPath(dirPath, parentPath, sizeof(parentPath));
        
        if (!hasParent)
        {
            log_debug(LOG_GUI, "    Path is root directory - no parent exists, falling back to Center Screen\n");
            posLeft = (screenWidth - finalWidth) / 2;
        }
        else
        {
            log_debug(LOG_GUI, "    Parent path: '%s'\n", parentPath);
            
            /* Try to read parent window geometry */
            hasParentGeometry = GetFolderWindowSettings(parentPath, &parentWindowInfo, &parentViewMode);
            
            if (!hasParentGeometry)
            {
                log_warning(LOG_GUI, "    Parent folder has no .info file (hidden folder)\n");
                log_debug(LOG_GUI, "    Falling back to Keep Position mode\n");
                
                /* Fall back to Keep Position behavior */
                if (hasCurrentPosition)
                {
                    posLeft = currentWindowInfo.left;
                    posTop = currentWindowInfo.top;
                    
                    /* Check boundaries */
                    if (posLeft + finalWidth > screenWidth)
                    {
                        posLeft = screenWidth - finalWidth;
                        if (posLeft < 0) posLeft = 0;
                    }
                    if (posLeft < 0) posLeft = 0;
                    if (posTop + finalHeight > screenHight)
                    {
                        posTop = screenHight - finalHeight;
                        if (posTop < prefsIControl.currentTitleBarHeight)
                            posTop = prefsIControl.currentTitleBarHeight;
                    }
                    if (posTop < prefsIControl.currentTitleBarHeight)
                        posTop = prefsIControl.currentTitleBarHeight;
                }
                else
                {
                    posLeft = (screenWidth - finalWidth) / 2;
                }
            }
            else
            {
                /* Parent geometry found - now get child's icon position in parent */
                BOOL parentIsRoot = IsRootFolder(parentPath);
                UWORD volumeGaugeOffset = 0;
                UWORD contentAreaLeftOffset = 0;
                UWORD contentAreaTopOffset = 0;
                
                /* Root directories (e.g., "Work:") have a volume gauge that offsets icon positions */
                if (parentIsRoot)
                {
                    volumeGaugeOffset = prefsIControl.currentCGaugeWidth;
                    log_debug(LOG_GUI, "    Parent is root directory - adding volume gauge offset: %d\n",
                             volumeGaugeOffset);
                }
                else
                {
                    /* Non-root drawers have content area margins based on borderless mode and emboss size */
                    contentAreaLeftOffset = DRAWER_CONTENT_LEFT_MARGIN(prefsWorkbench.borderless, 
                                                                        prefsWorkbench.embossRectangleSize);
                    contentAreaTopOffset = DRAWER_CONTENT_TOP_MARGIN(prefsWorkbench.borderless, 
                                                                      prefsWorkbench.embossRectangleSize);
                    log_debug(LOG_GUI, "    Parent is non-root drawer - content margins: left=%d, top=%d (borderless=%s, emboss=%d)\n",
                             contentAreaLeftOffset, contentAreaTopOffset, 
                             prefsWorkbench.borderless ? "YES" : "NO",
                             prefsWorkbench.embossRectangleSize);
                }
                
                log_debug(LOG_GUI, "    Parent window: left=%d, top=%d, width=%d, height=%d (isRoot=%s)\n",
                         parentWindowInfo.left, parentWindowInfo.top,
                         parentWindowInfo.width, parentWindowInfo.height,
                         parentIsRoot ? "YES" : "NO");
                
                /* Build path to child's .info file in parent directory */
                /* Extract just the folder name from dirPath (last component) */
                {
                    const char *lastSlash = strrchr(dirPath, '/');
                    const char *lastColon = strrchr(dirPath, ':');
                    const char *folderName;
                    size_t parentLen;
                    
                    /* Determine the folder name (last path component) */
                    if (lastSlash != NULL && (lastColon == NULL || lastSlash > lastColon))
                    {
                        folderName = lastSlash + 1;
                    }
                    else if (lastColon != NULL)
                    {
                        folderName = lastColon + 1;
                    }
                    else
                    {
                        folderName = dirPath;
                    }
                    
                    /* Build full path: parent/foldername.info */
                    /* Check if parent ends with ':' (root device) */
                    parentLen = strlen(parentPath);
                    if (parentLen > 0 && parentPath[parentLen - 1] == ':')
                    {
                        /* Root device - no separator needed */
                        snprintf(childIconPath, sizeof(childIconPath), "%s%s.info", 
                                 parentPath, folderName);
                    }
                    else
                    {
                        /* Subdirectory - use / separator */
                        snprintf(childIconPath, sizeof(childIconPath), "%s/%s.info", 
                                 parentPath, folderName);
                    }
                    
                    log_debug(LOG_GUI, "    Child icon path: '%s'\n", childIconPath);
                }
                
                /* Read child icon details (position and size) - pass NULL for text (not needed) */
                hasChildIcon = GetIconDetailsFromDisk(childIconPath, &childIconDetails, NULL);
                
                if (!hasChildIcon)
                {
                    log_warning(LOG_GUI, "    Unable to read child icon details from parent\n");
                    log_debug(LOG_GUI, "    Falling back to Keep Position mode\n");
                    
                    /* Fall back to Keep Position behavior */
                    if (hasCurrentPosition)
                    {
                        posLeft = currentWindowInfo.left;
                        posTop = currentWindowInfo.top;
                        
                        /* Check boundaries */
                        if (posLeft + finalWidth > screenWidth)
                        {
                            posLeft = screenWidth - finalWidth;
                            if (posLeft < 0) posLeft = 0;
                        }
                        if (posLeft < 0) posLeft = 0;
                        if (posTop + finalHeight > screenHight)
                        {
                            posTop = screenHight - finalHeight;
                            if (posTop < prefsIControl.currentTitleBarHeight)
                                posTop = prefsIControl.currentTitleBarHeight;
                        }
                        if (posTop < prefsIControl.currentTitleBarHeight)
                            posTop = prefsIControl.currentTitleBarHeight;
                    }
                    else
                    {
                        posLeft = (screenWidth - finalWidth) / 2;
                    }
                }
                else
                {
                    /* Calculate window position at bottom-right of icon */
                    /* Use base icon size (without emboss) for positioning
                     * Icon position.x/y represents where the icon BITMAP starts (not including emboss)
                     * Emboss is drawn around the bitmap, so we add just the bitmap size
                     */
                    log_debug(LOG_GUI, "    Child icon in parent: x=%d, y=%d, base_w=%d, base_h=%d\n",
                             childIconDetails.position.x, childIconDetails.position.y,
                             childIconDetails.size.width, childIconDetails.size.height);
                    
                    /* CRITICAL: Icon positions are relative to window's CONTENT AREA, not window edges
                     * We must add window border offsets to convert to absolute screen coordinates.
                     * Parent window left/top are the outer window edges (including borders).
                     * Icon x/y are relative to the inner content area (after borders/title bar).
                     * Left border = currentLeftBarWidth, Top border = currentTitleBarHeight
                     * PLUS: For root directories, icon X is after the volume gauge (currentCGaugeWidth)
                     *       For non-root drawers, icon X/Y are after content area margins (magic numbers)
                     */
                    proposedLeft = parentWindowInfo.left + prefsIControl.currentLeftBarWidth + 
                                   volumeGaugeOffset + contentAreaLeftOffset +
                                   childIconDetails.position.x + childIconDetails.size.width;
                    proposedTop = parentWindowInfo.top + prefsIControl.currentTitleBarHeight + 
                                  contentAreaTopOffset +
                                  childIconDetails.position.y + childIconDetails.size.height;
                    
                    log_debug(LOG_GUI, "    Proposed position: left=%d, top=%d (bottom-right of icon)\n",
                             proposedLeft, proposedTop);
                    
                    /* Start with proposed position and clamp to screen boundaries */
                    posLeft = proposedLeft;
                    posTop = proposedTop;
                    fitsOnScreen = TRUE;
                    
                    /* Clamp horizontal position to screen */
                    if (posLeft + finalWidth > screenWidth)
                    {
                        posLeft = screenWidth - finalWidth;
                        log_debug(LOG_GUI, "    Clamped to right edge: left=%d (was %d)\n", posLeft, proposedLeft);
                        fitsOnScreen = FALSE;
                    }
                    if (posLeft < 0)
                    {
                        posLeft = 0;
                        log_debug(LOG_GUI, "    Clamped to left edge: left=0\n");
                        fitsOnScreen = FALSE;
                    }
                    
                    /* Clamp vertical position to screen */
                    if (posTop + finalHeight > screenHight)
                    {
                        posTop = screenHight - finalHeight;
                        log_debug(LOG_GUI, "    Clamped to bottom edge: top=%d (was %d)\n", posTop, proposedTop);
                        fitsOnScreen = FALSE;
                    }
                    if (posTop < prefsIControl.currentTitleBarHeight)
                    {
                        posTop = prefsIControl.currentTitleBarHeight;
                        log_debug(LOG_GUI, "    Clamped to title bar: top=%d\n", posTop);
                        fitsOnScreen = FALSE;
                    }
                    
                    if (fitsOnScreen)
                    {
                        log_debug(LOG_GUI, "    Position at icon's bottom-right fits perfectly\n");
                    }
                    else
                    {
                        log_debug(LOG_GUI, "    Position adjusted to fit on screen (near parent icon)\n");
                    }
                    
                    /* Free default tool string if allocated */
                    if (childIconDetails.defaultTool != NULL)
                    {
                        whd_free(childIconDetails.defaultTool);
                    }
                }
            }
        }
    }
    else
    {
        /* Unknown mode - default to center */
        posLeft = (screenWidth - finalWidth) / 2;
        log_debug(LOG_GUI, "  Mode %d not recognized - using center at left=%d\n", 
                 positionMode, posLeft);
    }
    
#ifdef DEBUG
    append_to_log("  Final position: left=%d, top=%d, width=%d, height=%d\n", 
                  posLeft, posTop, finalWidth, finalHeight);
#endif

    newFolderInfo.left = posLeft;
    newFolderInfo.top = posTop;
    newFolderInfo.width = finalWidth;
    newFolderInfo.height = finalHeight;

    SaveFolderSettings(dirPath, &newFolderInfo, 0);
    
    log_info(LOG_GUI, "repoistionWindow: %s -> %dx%d at (%d,%d)\n",
             dirPath, finalWidth, finalHeight, posLeft, posTop);
#else
    (void)dirPath;
    (void)winWidth;
    (void)winHeight;
#endif
}
