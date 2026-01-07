#include <platform/platform.h>
#include <platform/platform_io.h>
#include <platform/amiga_headers.h>

#if PLATFORM_AMIGA
#include <dos/dostags.h>  /* For DOS tags */
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <devices/timer.h>
#include <clib/timer_protos.h>

#include "itidy_types.h"
#include "icon_management.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "icon_types.h"
#include "utilities.h"
#include "spinner.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "dos/getDiskDetails.h"
#include "Settings/WorkbenchPrefs.h"
#include "Settings/IControlPrefs.h"
#include "layout_processor.h"
#include "GUI/StatusWindows/main_progress_window.h"

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

/* External timer base for performance measurement */
extern struct Device* TimerBase;

/* Global flag to enable/disable default tool validation (set by layout processor) */
BOOL g_ValidateDefaultTools = FALSE;

void FreeIconArray(IconArray *iconArray)
{

    size_t i;

    if (iconArray != NULL)
    {
        if (iconArray->array != NULL)
        {
            for (i = 0; i < iconArray->size; i++)
            {
                if (iconArray->array[i].icon_text != NULL)
                {
                    whd_free(iconArray->array[i].icon_text);
                } /* if */
                if (iconArray->array[i].icon_full_path != NULL)
                {
                    whd_free(iconArray->array[i].icon_full_path);
                } /* if */
                if (iconArray->array[i].default_tool != NULL)
                {
                    whd_free(iconArray->array[i].default_tool);
                } /* if */
            } /* for */
            whd_free(iconArray->array);
        } /* if */
        whd_free(iconArray);
    } /* if */
}

IconArray *CreateIconArray(void)
{
    IconArray *iconArray = (IconArray *)whd_malloc(sizeof(IconArray));
    if (iconArray != NULL)
    {
        memset(iconArray, 0, sizeof(IconArray));
        iconArray->array = NULL; /* Start with no allocated array */
        iconArray->size = 0;
        iconArray->capacity = 0;
    } /* if */
    return iconArray;
}

BOOL AddIconToArray(IconArray *iconArray, const FullIconDetails *newIcon)
{
    FullIconDetails *newArray;
    size_t newCapacity;

    if (iconArray == NULL || newIcon == NULL)
    {
        return FALSE;
    } /* if */

    if (iconArray->size >= iconArray->capacity)
    {
        /* Increase capacity (start with 1 if currently 0) */
        newCapacity = (iconArray->capacity == 0) ? 1 : iconArray->capacity * 2;
        newArray = (FullIconDetails *)whd_malloc(newCapacity * sizeof(FullIconDetails));
        if (newArray == NULL)
        {
            return FALSE;
        } /* if */

        memset(newArray, 0, newCapacity * sizeof(FullIconDetails));

        /* Copy existing elements to new array */
        if (iconArray->array != NULL)
        {
            memcpy(newArray, iconArray->array, iconArray->size * sizeof(FullIconDetails));
            whd_free(iconArray->array);
        } /* if */

        iconArray->array = newArray;
        iconArray->capacity = newCapacity;
    } /* if */

    /* Add the new icon details to the array */
    iconArray->array[iconArray->size] = *newIcon;
    iconArray->size += 1;

    return TRUE;
}

IconArray *CreateIconArrayFromPath(BPTR lock, const char *dirPath)
{
    /* ================================================================
     * PERFORMANCE OPTIMIZATION: Pattern Matching
     * ================================================================
     * Uses AmigaDOS MatchFirst/MatchNext to filter .info files at the
     * filesystem level instead of examining every directory entry.
     * 
     * OLD METHOD (Examine/ExNext):
     *   - Scans ALL files and directories
     *   - Checks each name for .info extension in userspace
     *   - Very slow with many non-icon entries
     * 
     * NEW METHOD (MatchFirst/MatchNext):
     *   - Pattern "#?.info" filters at filesystem level
     *   - Only .info files are returned
     *   - 40x+ faster in directories with few/no icons
     * 
     * Example: WHDLoad folder with 600 subdirs, 0 .info files
     *   OLD: 4+ seconds of disk grinding
     *   NEW: <0.1 seconds
     * ================================================================
     */
    
    struct TextExtent textExtent;
    FullIconDetails newIcon;
    struct AnchorPath *anchorPath;  /* Pattern matching structure */
    struct FileInfoBlock *fib;      /* Points to embedded FIB in AnchorPath */
    IconSize iconSize = {0, 0};
    IconArray *iconArray = CreateIconArray();
    char fullPathAndFile[512];
    char fullPathAndFileNoInfo[512];
    char fileNameNoInfo[128];
    char pattern[520];              /* Pattern string: "path/#?.info" */
    int textLength, fileCount = 0, maxWidth = 0;
    IconPosition iconPosition;
    LONG matchResult;               /* MatchFirst/MatchNext return value */
    struct timeval startTime, endTime;
    ULONG elapsedMicros, elapsedMillis;

    /* Start timing - capture system time */
    if (TimerBase)
    {
        GetSysTime(&startTime);
    }

    /* CRITICAL DEBUG: Use append_to_log as fallback */
    append_to_log(">>> CreateIconArrayFromPath ENTRY: lock=%ld, dirPath='%s'\n", (LONG)lock, dirPath);

#ifdef DEBUG
    log_debug(LOG_ICONS, "CreateIconArrayFromPath ENTRY: lock=%ld, dirPath='%s'", (LONG)lock, dirPath);
    log_debug(LOG_ICONS, "Using OPTIMIZED pattern matching (MatchFirst/MatchNext)");
#endif

    iconArray->hasOnlyBorderlessIcons = false;

    if (!iconArray)
    {
        CONSOLE_ERROR("Failed to create icon array.\n");
#ifdef DEBUG
        log_error(LOG_ICONS, "Failed to create icon array");
#endif
        return NULL;
    }

#ifdef DEBUG
    log_debug(LOG_ICONS, "Icon array created successfully");
#endif

    /* ================================================================
     * ALLOCATE ANCHORPATH STRUCTURE
     * ================================================================
     * AnchorPath contains:
     *   - ap_Info: Embedded FileInfoBlock for matched files
     *   - ap_Buf: Variable-length buffer at end of structure
     *   - ap_Strlen: Size of buffer (must be set before MatchFirst!)
     * 
     * IMPORTANT: Manual allocation method (documented pattern)
     * 
     * While AllocDosObject() with ADO_Strlen tag is the "official" way,
     * we use manual allocation here for maximum compatibility and control.
     * 
     * This is a well-documented pattern in Amiga programming:
     * - Allocate sizeof(AnchorPath) + buffer_size as single block
     * - Set ap_Strlen to buffer size before calling MatchFirst
     * - ap_Buf automatically points to memory after the structure
     * 
     * Note: We initially tried ADO_Strlen tag but it caused issues,
     * likely due to SDK version incompatibilities. The manual method
     * works reliably across all Amiga OS versions.
     * ================================================================
     */
#if PLATFORM_AMIGA
    {
        LONG bufferSize = 1024;  /* Large buffer for deep paths */
        LONG totalSize = sizeof(struct AnchorPath) + bufferSize - 1;  /* -1 because ap_Buf[1] already in struct */
        
        /* Allocate AnchorPath + buffer as single block */
        anchorPath = (struct AnchorPath *)whd_malloc(totalSize);
        if (anchorPath)
        {
            memset(anchorPath, 0, totalSize);
            /* Initialize the AnchorPath fields manually */
            anchorPath->ap_Strlen = bufferSize;  /* Tell DOS about buffer size */
            anchorPath->ap_BreakBits = 0;
            anchorPath->ap_FoundBreak = 0;
            anchorPath->ap_Flags = 0;
            /* ap_Buf is already part of the structure at the end */
        }
    }
#else
    anchorPath = (struct AnchorPath *)AllocDosObject(DOS_ANCHORPATH, NULL);
#endif
    if (!anchorPath)
    {
        CONSOLE_ERROR("Failed to allocate AnchorPath for pattern matching.\n");
#ifdef DEBUG
        log_error(LOG_ICONS, "Failed to allocate AnchorPath (out of memory?)");
#endif
        FreeIconArray(iconArray);
        return NULL;
    }

#ifdef DEBUG
    log_debug(LOG_ICONS, "AnchorPath allocated successfully");
    append_to_log(">>> AFTER ALLOCATION: anchorPath=%p, ap_Buf=%p, ap_Strlen=%ld\n",
                 anchorPath, anchorPath->ap_Buf, anchorPath->ap_Strlen);
    if (anchorPath->ap_Buf) {
        append_to_log(">>> AFTER ALLOCATION: First 20 bytes of ap_Buf: [%02x %02x %02x %02x %02x...]\n",
                     (unsigned char)anchorPath->ap_Buf[0], (unsigned char)anchorPath->ap_Buf[1],
                     (unsigned char)anchorPath->ap_Buf[2], (unsigned char)anchorPath->ap_Buf[3],
                     (unsigned char)anchorPath->ap_Buf[4]);
    }
#endif

    /* ================================================================
     * BUILD AMIGADOS PATTERN
     * ================================================================
     * Pattern syntax:
     *   #?      = Match any characters (AmigaDOS wildcard)
     *   #?.info = Match any file ending with ".info"
     *   path/#?.info = Match .info files in specific directory
     * 
     * IMPORTANT: This pattern ONLY matches FILES, not directories!
     * We check if the corresponding file/folder is a directory later.
     * 
     * Examples of what this matches:
     *   ✓ MyFile.info
     *   ✓ MyDrawer.info  
     *   ✓ .info (if filename is just ".info")
     *   ✗ MyFile (no .info extension)
     *   ✗ MySubDir (directories without .info)
     *   ✗ Disk.info (excluded separately via isIconTypeDisk check)
     * 
     * IMPORTANT: Pattern syntax differs for root vs subdirectories:
     *   - Root (volume):      "workbench:#?.info"  (no slash)
     *   - Subdirectory:       "workbench:Prefs/#?.info"  (with slash)
     * 
     * We detect root by checking if path ends with colon (:)
     * ================================================================
     */
    {
        size_t pathLen = strlen(dirPath);
        if (pathLen > 0 && dirPath[pathLen - 1] == ':')
        {
            /* Root directory (volume) - no slash needed */
            snprintf(pattern, sizeof(pattern), "%s#?.info", dirPath);
        }
        else
        {
            /* Subdirectory - needs slash separator */
            snprintf(pattern, sizeof(pattern), "%s/#?.info", dirPath);
        }
    }
    
#ifdef DEBUG
    append_to_log("Pattern: '%s'\n", pattern);
#endif
#ifdef DEBUG_MAX
    append_to_log("Starting optimized pattern-based scan of: %s\n", dirPath);
    append_to_log("This will ONLY return .info files (not directories)\n");
#endif

    /* ================================================================
     * PATTERN MATCHING LOOP: MatchFirst/MatchNext
     * ================================================================
     * This replaces the old Examine/ExNext loop.
     * 
     * MatchFirst() - Start pattern matching
     *   Returns: 0 = match found, non-zero = no matches or error
     * 
     * MatchNext() - Continue to next match
     *   Returns: 0 = match found, non-zero = no more matches
     * 
     * CRITICAL: Must call MatchEnd() when done (even on error)!
     * 
     * Performance benefit:
     *   - Filesystem does the filtering (very fast)
     *   - No wasted time on non-.info files
     *   - Dramatic speedup in sparse directories
     * ================================================================
     */
    matchResult = MatchFirst(pattern, anchorPath);
    
#ifdef DEBUG
    if (matchResult == 0)
    {
        log_debug(LOG_ICONS, "MatchFirst successful, entering processing loop\n");
        log_debug(LOG_ICONS, ">>> AFTER MATCHFIRST: ap_Strlen=%ld, ap_Buf='%s'\n",
                     anchorPath->ap_Strlen, anchorPath->ap_Buf ? anchorPath->ap_Buf : "(null)");
    }
    else
    {
        log_debug(LOG_ICONS, "MatchFirst returned %ld (no .info files found or error)\n", matchResult);
    }
#endif
    
    while (matchResult == 0)
    {
        /* Get FileInfoBlock from AnchorPath's embedded structure */
        fib = &anchorPath->ap_Info;
        
#ifdef DEBUG
        log_debug(LOG_ICONS, "Pattern matched .info file: '%s'\n", fib->fib_FileName);
#endif
        
        /* Update progress spinner (user feedback during long scans) */
        updateCursor();
        
        /* ============================================================
         * CONSTRUCT FULL PATH
         * ============================================================
         * GetFullPath() builds: dirPath + "/" + filename
         * Note: anchorPath->ap_Buf already contains full path, but
         * we use GetFullPath() for consistency with existing code.
         * ============================================================
         */
        GetFullPath(dirPath, fib, fullPathAndFile, sizeof(fullPathAndFile));
        
#ifdef DEBUG
        log_debug(LOG_ICONS, "Full path to .info file: '%s'\n", fullPathAndFile);
#endif

        /* ============================================================
         * EXCLUSION FILTER 1: Disk Icons (Volume Icons)
         * ============================================================
         * Skip "Disk.info" files which represent volume icons.
         * These are special icons that represent mounted volumes
         * and should not be repositioned by iTidy.
         * 
         * isIconTypeDisk() checks:
         *   - If filename is "Disk.info" (case-insensitive)
         *   - Returns 0 = not a disk icon, 1 = is a disk icon
         * ============================================================
         */
        int isDiskIcon = isIconTypeDisk(fullPathAndFile, fib->fib_DirEntryType);
#ifdef DEBUG
        log_debug(LOG_ICONS, "isIconTypeDisk('%s') = %d (0=not disk, 1=is disk)\n", fullPathAndFile, isDiskIcon);
#endif
        if (isDiskIcon == 0)
        {
            /* ========================================================
             * EXCLUSION FILTER 2: "Left Out" Icons
             * ========================================================
             * Skip icons that have been "left out" on the Workbench
             * backdrop (desktop). These icons should remain where
             * the user placed them.
             * 
             * isIconLeftOut() checks against a list loaded from
             * ENV:Sys/WBConfig.prefs
             * ========================================================
             */
            if (isIconLeftOut(fullPathAndFile) == false)
            {
                /* ====================================================
                 * REMOVE .INFO EXTENSION TO GET ACTUAL FILE PATH
                 * ====================================================
                 * Icon files have ".info" extension, but we need the
                 * actual file/folder name for further checks:
                 *   "MyFile.info" -> "MyFile"
                 *   "MyFolder.info" -> "MyFolder"
                 * ====================================================
                 */
                removeInfoExtension(fullPathAndFile, fullPathAndFileNoInfo);
                removeInfoExtension(fib->fib_FileName, fileNameNoInfo);
                
                /* ====================================================
                 * EXCLUSION FILTER 3: Empty or Dot-Only Names
                 * ====================================================
                 * Skip icons with empty names or names that are just
                 * a dot (.) character. These are typically hidden or
                 * corrupted icon files that display as blank icons.
                 * 
                 * Examples to skip:
                 *   ".info" -> "" (empty after removing .info)
                 *   "." (just a dot, often hidden files)
                 * ====================================================
                 */
                if (fileNameNoInfo[0] != '\0' &&  /* Not empty */
                    !(fileNameNoInfo[0] == '.' && fileNameNoInfo[1] == '\0'))  /* Not just "." */
                {
                    /* ====================================================
                     * EXCLUSION FILTER 4: Corrupted/Invalid Icons
                     * ====================================================
                     * IsValidIcon() verifies the .info file is readable
                     * and properly formatted (not corrupted).
                     * ====================================================
                     */
                    if (IsValidIcon(fullPathAndFileNoInfo))
                    {
                        IconDetailsFromDisk iconDetails;
                        
                        removeInfoExtension(fib->fib_FileName, fileNameNoInfo);
                    
                        /* Reset newIcon to known defaults - CRITICAL: Initialize ALL fields */
                        memset(&newIcon, 0, sizeof(FullIconDetails));
                        newIcon.icon_type = icon_type_standard;
                        newIcon.icon_height = 0;
                        newIcon.icon_width = 0;
                        newIcon.border_width = 0;
                        newIcon.text_width = 0;
                        newIcon.text_height = 0;
                        newIcon.icon_max_width = 0;
                        newIcon.icon_max_height = 0;
                        newIcon.icon_x = 0;
                        newIcon.icon_y = 0;
                        newIcon.icon_text = NULL;
                        newIcon.icon_full_path = NULL;
                        newIcon.default_tool = NULL;
                        newIcon.is_folder = false;

#ifdef DEBUG_MAX
                    append_to_log("-------------------------\n");
                    append_to_log("Adding to %s Icon array.\n ", fullPathAndFile);
#endif

                    /* ========================================================
                     * OPTIMIZED ICON READING - Single disk operation
                     * ========================================================
                     * Read ALL icon details in ONE GetDiskObject() call:
                     * - Position, Size, Type, Frame, Default Tool
                     * - ALL size calculations (emboss, borders, text, total)
                     * This reduces disk I/O by ~75% (was 3-4 reads per icon!)
                     * Critical for floppy-based systems.
                     * ======================================================== */
#ifdef DEBUG
                    log_debug(LOG_ICONS, "Reading icon details (optimized): %s\n", fullPathAndFile);
                    log_debug(LOG_ICONS, "  fullPathAndFile='%s'\n", fullPathAndFile);
                    log_debug(LOG_ICONS, "  fileNameNoInfo='%s'\n", fileNameNoInfo);
                    log_debug(LOG_ICONS, "  Calling GetIconDetailsFromDisk...\n");
#endif
                    if (GetIconDetailsFromDisk(fullPathAndFile, &iconDetails, fileNameNoInfo))
                    {
#ifdef DEBUG
                        log_debug(LOG_ICONS, ">>> GetIconDetailsFromDisk SUCCESS\n");
                        log_debug(LOG_ICONS, "  GetIconDetailsFromDisk SUCCESS\n");
#endif
                        /* Extract position */
                        newIcon.icon_x = iconDetails.position.x;
                        newIcon.icon_y = iconDetails.position.y;
                        iconPosition = iconDetails.position; /* For compatibility */
#ifdef DEBUG
                        log_debug(LOG_ICONS, "  Position: x=%d, y=%d\n", newIcon.icon_x, newIcon.icon_y);
#endif
                        
                        /* Extract size (base bitmap size) */
                        iconSize = iconDetails.size;
                        
                        /* Extract and store default tool */
                        newIcon.default_tool = iconDetails.defaultTool; /* Transfer ownership */
                        
                        /* Log default tool information and validate existence (if enabled) */
                        /* IMPORTANT: Only validate default tools for WBPROJECT icons.
                         * WBTOOL icons (executables) don't use their default tool - Workbench
                         * runs the tool itself, not the default tool. Default tool field in
                         * WBTOOL icons is often garbage or leftover data from icon editors. */
                        if (iconDetails.defaultTool != NULL && iconDetails.defaultTool[0] != '\0')
                        {
                            if (g_ValidateDefaultTools && iconDetails.workbenchType == WBPROJECT)
                            {
                                BOOL toolExists = ValidateDefaultTool(iconDetails.defaultTool);
                                
                                /* Track which file uses this tool */
                                AddFileReferenceToToolCache(iconDetails.defaultTool, fullPathAndFile);
                                
                                log_info(LOG_ICONS, "  Default Tool: '%s' -> %s [%s]\n", 
                                        iconDetails.defaultTool, fileNameNoInfo,
                                        toolExists ? "EXISTS" : "MISSING");
                            }
                            else if (iconDetails.workbenchType == WBTOOL)
                            {
                                /* Skip validation for WBTOOL - default tool is not used by Workbench */
                                log_info(LOG_ICONS, "  Default Tool: '%s' -> %s (WBTOOL - not validated)\n", 
                                        iconDetails.defaultTool, fileNameNoInfo);
                            }
                            else
                            {
                                log_info(LOG_ICONS, "  Default Tool: '%s' -> %s\n", 
                                        iconDetails.defaultTool, fileNameNoInfo);
                            }
                        }
                        else
                        {
                            log_info(LOG_ICONS, "  Default Tool: (none) -> %s\n", fileNameNoInfo);
                        }
                        
                        /* Determine icon type (respecting user_forceStandardIcons) */
                        if (user_forceStandardIcons == 0)
                        {
                            if (iconDetails.isNewIcon)
                            {
                                newIcon.icon_type = icon_type_newIcon;
                                count_icon_type_newIcon++;
                                log_info(LOG_ICONS, "  Icon type: NewIcon format - %s\n", fileNameNoInfo);
                            }
                            else if (iconDetails.isOS35Icon)
                            {
                                newIcon.icon_type = icon_type_os35;
                                count_icon_type_os35++;
                                log_info(LOG_ICONS, "  Icon type: OS3.5 format - %s\n", fileNameNoInfo);
                            }
                            else
                            {
                                newIcon.icon_type = icon_type_standard;
                                count_icon_type_standard++;
                                log_info(LOG_ICONS, "  Icon type: Standard Workbench format - %s\n", fileNameNoInfo);
                            }
                        }
                        else
                        {
                            /* User forcing standard icons */
                            newIcon.icon_type = icon_type_standard;
                            count_icon_type_standard++;
                            log_info(LOG_ICONS, "  Icon type: Forced Standard format - %s\n", fileNameNoInfo);
                        }
                        
                        /* Use pre-calculated size fields from GetIconDetailsFromDisk */
                        newIcon.icon_width = iconDetails.iconWithEmboss.width;
                        newIcon.icon_height = iconDetails.iconWithEmboss.height;
                        newIcon.border_width = iconDetails.borderWidth;
                        newIcon.text_width = iconDetails.textSize.width;
                        newIcon.text_height = iconDetails.textSize.height;
                        newIcon.icon_max_width = iconDetails.totalDisplaySize.width;
                        newIcon.icon_max_height = iconDetails.totalDisplaySize.height;

#ifdef DEBUG_MAX
                        append_to_log("Icons size x: %d, y: %d, current at pos x: %d, y: %d border size:%d\n", 
                                     iconSize.width, iconSize.height, newIcon.icon_x, newIcon.icon_y, newIcon.border_width);
#endif

                        if (newIcon.border_width == 0)
                        {
                            iconArray->hasOnlyBorderlessIcons = true;
                        }
                        
                        /* Capture file protection, size and date from FileInfoBlock */
#if PLATFORM_AMIGA
                        newIcon.is_write_protected = (fib->fib_Protection & FIBF_WRITE) ? true : false;
#else
                        newIcon.is_write_protected = false; /* Host stub */
#endif

#ifdef DEBUG_MAX
                        append_to_log("Icon is write protected: %d\n", newIcon.is_write_protected);
#endif
#ifdef DEBUG_MAX
                        append_to_log("calculated border: %d\n", (newIcon.border_width * 2));
#endif

                        /* Determine if it's a folder or a file */
#ifdef DEBUG
                        log_debug(LOG_ICONS, "  Calling isDirectory('%s')...\n", fullPathAndFile);
#endif
                        newIcon.is_folder = isDirectory(fullPathAndFile);
#ifdef DEBUG
                        log_debug(LOG_ICONS, "  isDirectory returned: %d\n", newIcon.is_folder);
#endif
                        
                        /* Capture file size and date from FileInfoBlock */
#if PLATFORM_AMIGA
#ifdef DEBUG
                        log_debug(LOG_ICONS, "  Capturing file metadata from fib\n");
                        log_debug(LOG_ICONS, "  is_folder=%d\n", newIcon.is_folder);
#endif
                        if (newIcon.is_folder)
                        {
                            newIcon.file_size = 0;  /* Folders have no size */
                        }
                        else
                        {
                            newIcon.file_size = fib->fib_Size;
#ifdef DEBUG
                            log_debug(LOG_ICONS, "  file_size=%ld\n", newIcon.file_size);
#endif
                        }
                        /* Copy the DateStamp structure */
                        newIcon.file_date.ds_Days = fib->fib_Date.ds_Days;
                        newIcon.file_date.ds_Minute = fib->fib_Date.ds_Minute;
                        newIcon.file_date.ds_Tick = fib->fib_Date.ds_Tick;
#ifdef DEBUG
                        log_debug(LOG_ICONS, "  date: days=%ld, min=%ld, tick=%ld\n", 
                                 newIcon.file_date.ds_Days, newIcon.file_date.ds_Minute, newIcon.file_date.ds_Tick);
#endif
#else
                        newIcon.file_size = 0;  /* Host stub */
                        newIcon.file_date.ds_Days = 0;
                        newIcon.file_date.ds_Minute = 0;
                        newIcon.file_date.ds_Tick = 0;
#endif
                        
#ifdef DEBUG_MAX
                        append_to_log("Allocating memory for icon path.\n");
#endif
#ifdef DEBUG
                        log_debug(LOG_ICONS, "  Allocating icon_full_path (%d bytes)...\n", strlen(fullPathAndFile) + 1);
#endif
                        /* Allocate and copy the full path and icon text */
                        newIcon.icon_full_path = (char *)whd_malloc(strlen(fullPathAndFile) + 1);
                        if (!newIcon.icon_full_path)
                        {
                            CONSOLE_ERROR("Failed to allocate memory for icon full path.\n");
#ifdef DEBUG
                            log_error(LOG_ICONS, "  MALLOC FAILED for icon_full_path\n");
#endif
                            FreeIconArray(iconArray);
#if PLATFORM_AMIGA
                            MatchEnd(anchorPath);
                            FreeDosObject(DOS_ANCHORPATH, anchorPath);
#else
                            whd_free(anchorPath);
#endif
                            return NULL;
                        }
                        memset(newIcon.icon_full_path, 0, strlen(fullPathAndFile) + 1);
                        strcpy(newIcon.icon_full_path, fullPathAndFile);
#ifdef DEBUG
                        log_debug(LOG_ICONS, "  icon_full_path='%s'\n", newIcon.icon_full_path);
#endif

                        textLength = strlen(fileNameNoInfo) + 1;
#ifdef DEBUG
                        log_debug(LOG_ICONS, "  Allocating icon_text (%d bytes)...\n", textLength);
#endif
                        newIcon.icon_text = (char *)whd_malloc(textLength);
                        if (!newIcon.icon_text)
                        {
                            CONSOLE_ERROR("Failed to allocate memory for icon text.\n");
#ifdef DEBUG
                            log_error(LOG_ICONS, "  MALLOC FAILED for icon_text\n");
#endif
                            whd_free(newIcon.icon_full_path);
                            FreeIconArray(iconArray);
#if PLATFORM_AMIGA
                            MatchEnd(anchorPath);
                            FreeDosObject(DOS_ANCHORPATH, anchorPath);
#else
                            whd_free(anchorPath);
#endif
                            return NULL;
                        }
                        memset(newIcon.icon_text, 0, textLength);
                        strcpy(newIcon.icon_text, fileNameNoInfo);
#ifdef DEBUG
                        log_debug(LOG_ICONS, "  icon_text='%s'\n", newIcon.icon_text);
#endif

                        /* Update the maximum width */
                        if (newIcon.icon_max_width > maxWidth)
                        {
                            maxWidth = newIcon.icon_max_width;
                        }

#ifdef DEBUG
                        log_debug(LOG_ICONS, "  Adding icon to array...\n");
                        log_debug(LOG_ICONS, ">>> About to call AddIconToArray, fileCount=%d\n", fileCount);
#endif
                        /* Add new icon to the array */
                        if (!AddIconToArray(iconArray, &newIcon))
                        {
                            CONSOLE_ERROR("Failed to add icon to array.\n");
                            append_to_log(">>> ERROR: AddIconToArray FAILED!\n");
#ifdef DEBUG
                            log_error(LOG_ICONS, "  AddIconToArray FAILED\n");
#endif
                            whd_free(newIcon.icon_text);
                            whd_free(newIcon.icon_full_path);
                            FreeIconArray(iconArray);
#if PLATFORM_AMIGA
                            MatchEnd(anchorPath);
                            FreeDosObject(DOS_ANCHORPATH, anchorPath);
#else
                            whd_free(anchorPath);
#endif
                            return NULL;
                        }

#ifdef DEBUG
                        log_debug(LOG_ICONS, ">>> AddIconToArray SUCCESS, fileCount now %d\n", fileCount + 1);
                        log_debug(LOG_ICONS, "  Icon added successfully, fileCount now: %d\n", fileCount + 1);
#endif
                        fileCount++;
                        
                        /* Update heartbeat status for progress feedback during scan */
                        {
                            struct iTidyMainProgressWindow *pw = GetCurrentProgressWindow();
                            if (pw != NULL)
                            {
                                itidy_main_progress_update_heartbeat(pw, "Scanning", fileCount, 0);
                                /* Also pump events to allow cancellation */
                                itidy_main_progress_window_handle_events(pw);
                                
                                /* Check if user requested cancellation */
                                if (pw->cancel_requested)
                                {
                                    append_to_log("User cancelled during icon scanning at %d icons\n", fileCount);
                                    goto cleanup_and_return;
                                }
                            }
                        }
                    }
                    else
                    {
#ifdef DEBUG
                        log_warning(LOG_ICONS, "GetIconDetailsFromDisk FAILED for '%s'\n", fullPathAndFile);
#endif
                        /* Fallback if optimized read fails - skip this icon */
                        append_to_log("Warning: Failed to read icon details for %s\n", fullPathAndFile);
                        continue;
                    }
                    /* End: if (GetIconDetailsFromDisk) */
                } /* End: if (IsValidIcon) */
            } /* End: if (fileNameNoInfo not empty or dot) */
            else
            {
#ifdef DEBUG
                log_debug(LOG_ICONS, "Skipping icon with empty or dot-only name: '%s'\n", fib->fib_FileName);
#endif
            }
            } /* End: if (isIconLeftOut()) */
        } /* End: if (isIconTypeDisk()) */
        
        /* ============================================================
         * ADVANCE TO NEXT MATCHED FILE
         * ============================================================
         * MatchNext() continues pattern matching and returns:
         *   0 = Success (another matching file found)
         *   ERROR_NO_MORE_ENTRIES = No more matches (normal loop exit)
         *   Other error code = Filesystem error
         * ============================================================
         */
        eraseSpinner();
#ifdef DEBUG
        log_debug(LOG_ICONS, ">>> About to call MatchNext, fileCount=%d\n", fileCount);
        log_debug(LOG_ICONS, ">>> AnchorPath buffer strlen=%ld, buf='%s'\n", 
                     anchorPath->ap_Strlen, anchorPath->ap_Buf ? anchorPath->ap_Buf : "(null)");
#endif
        matchResult = MatchNext(anchorPath);
#ifdef DEBUG
        log_debug(LOG_ICONS, ">>> MatchNext returned: %ld\n", matchResult);
#endif
        
        /* Small delay to let DOS cleanup locks (similar to layout_processor fix) */
        #if PLATFORM_AMIGA
        Delay(1);  /* 1 tick = ~20ms, prevents MatchNext crashes on fast systems */
        #endif
    } /* End: pattern matching while loop */

cleanup_and_return:
    /* ================================================================
     * CLEANUP PATTERN MATCHING RESOURCES
     * ================================================================
     * MatchEnd() MUST be called to free internal AmigaDOS structures
     * allocated by MatchFirst(). Failing to call this will cause
     * memory leaks.
     * 
     * Then free our manually allocated AnchorPath (buffer included).
     * ================================================================
     */
    MatchEnd(anchorPath);
    
    /* Clear heartbeat status now that scanning is complete */
    {
        struct iTidyMainProgressWindow *pw = GetCurrentProgressWindow();
        if (pw != NULL)
        {
            itidy_main_progress_clear_heartbeat(pw);
        }
    }
    
#if PLATFORM_AMIGA
    /* Free manually allocated AnchorPath+buffer as single block */
    whd_free(anchorPath);
#else
    FreeDosObject(DOS_ANCHORPATH, anchorPath);
#endif

    /* Set the largest icon width */
    iconArray->BiggestWidthPX = maxWidth;
#ifdef DEBUG_MAX
append_to_log("Has only borderless icons: %d\n", iconArray->hasOnlyBorderlessIcons);
#endif
#ifdef DEBUG

    dumpIconArrayToScreen(iconArray);
#endif

    /* End timing - calculate elapsed time */
    if (TimerBase)
    {
        GetSysTime(&endTime);
        
        /* Calculate elapsed time in microseconds */
        elapsedMicros = ((endTime.tv_secs - startTime.tv_secs) * 1000000) +
                        (endTime.tv_micro - startTime.tv_micro);
        elapsedMillis = elapsedMicros / 1000;
        
        /* Log performance metrics only if enabled */
        if (is_performance_logging_enabled())
        {
            append_to_log("==== ICON LOADING PERFORMANCE ====");
            append_to_log("CreateIconArrayFromPath() execution time:");
            append_to_log("  %lu microseconds (%lu.%03lu ms)\n", 
                          elapsedMicros, elapsedMillis, elapsedMicros % 1000);
            append_to_log("  Icons loaded: %d\n", iconArray->size);
            if (iconArray->size > 0)
            {
                append_to_log("  Time per icon: %lu microseconds\n", 
                              elapsedMicros / iconArray->size);
            }
            append_to_log("  Folder: %s", dirPath);
            append_to_log("==================================");
            
            /* Also print to console for immediate visibility */
            CONSOLE_STATUS("  [TIMING] Icon loading: %lu.%03lu ms for %lu icons\n",
                   elapsedMillis, elapsedMicros % 1000, (unsigned long)iconArray->size);
        }
    }

    return iconArray;
}

/* Comparison function for sorting by is_folder and then by icon_text */
int CompareByFolderAndName(const void *a, const void *b)
{
    int nameComparisonResult;
    const FullIconDetails *iconA = (const FullIconDetails *)a;
    const FullIconDetails *iconB = (const FullIconDetails *)b;

    /* Debug: Print comparison details */
    /*
    CONSOLE_DEBUG("Comparing: %s (folder: %d) vs %s (folder: %d)\n",
           iconA->icon_text, iconA->is_folder,
           iconB->icon_text, iconB->is_folder);
    */

    /* Compare by is_folder: folders should come before files */
    if (iconA->is_folder != iconB->is_folder)
    {
        int result = iconA->is_folder ? -1 : 1;
        /*printf("Folder comparison result: %d\n", result);  Debug */
        return result;
    }

    /* If both are either folders or files, compare by icon_text */
    nameComparisonResult = strncasecmp_custom(iconA->icon_text, iconB->icon_text,
                                              strlen(iconA->icon_text) > strlen(iconB->icon_text) ? strlen(iconB->icon_text) : strlen(iconA->icon_text));

    /* Debug: Print name comparison result */
    /*
    CONSOLE_DEBUG("Name comparison result for %s vs %s: %d\n", iconA->icon_text, iconB->icon_text, nameComparisonResult);
    */

    return nameComparisonResult;
}

BOOL checkIconFrame(const char *iconName)
{
#if PLATFORM_AMIGA
    struct DiskObject *icon = NULL;
    int32_t frameStatus;
    int32_t errorCode;
    struct Library *IconBase = NULL;
    const char *extension = ".info";
    size_t len = strlen(iconName);
    size_t ext_len = strlen(extension);

    /* Calculate the length of the new icon name without the ".info" extension if present */
    size_t new_len = (len >= ext_len && platform_stricmp(iconName + len - ext_len, extension) == 0) ? len - ext_len : len;

    /* Allocate memory for the new icon name */
    char *newIconName = (char *)malloc((new_len + 1) * sizeof(char));
    if (newIconName == NULL)
    {
        CONSOLE_ERROR("Memory allocation failed\n");
        return FALSE;
    }

    /* Copy the icon name up to the new length and null-terminate it */
    strncpy(newIconName, iconName, new_len);
    newIconName[new_len] = '\0';
#ifdef DEBUG_MAX
    append_to_log("Opening icon library\n");
#endif
    /* Open the icon.library */
    IconBase = OpenLibrary("icon.library", 0);
    if (!IconBase)
    {
        CONSOLE_ERROR("Failed to open icon.library\n");
        free(newIconName);
        return TRUE; /* Assume it has a frame if library can't be opened */
    }

    if (IconBase->lib_Version < 44)
    {
#ifdef DEBUG
        append_to_log("icon.library version: %ld.%ld\n", IconBase->lib_Version, IconBase->lib_Revision);
        append_to_log("icon.library version 44 or higher is required to check for frames.\n");
#endif
        CloseLibrary(IconBase);
        free(newIconName);
        return TRUE; /* Assume it has a frame if library version is too low */
    }

    /* Load the icon using the new name without the ".info" extension */
#ifdef DEBUG_MAX
    append_to_log("icon.library version: %ld.%ld\n", IconBase->lib_Version, IconBase->lib_Revision);
    append_to_log("icon for border checks: %s\n", newIconName);
    append_to_log("Getting icon tags\n");
#endif

    icon = GetIconTags(newIconName, TAG_END);
    if (!icon)
    {
        // printf("Failed to load icon for border checks: %s\n", newIconName);
        CloseLibrary(IconBase);
        free(newIconName);
        return TRUE; /* Assume it has a frame if icon can't be loaded */
    }
#ifdef DEBUG_MAX

    append_to_log("Checking to see if it has a border\n");
#endif
    /* Use IconControl to check the frame status of the icon */
    if (IconControl(icon,
                    ICONCTRLA_GetFrameless, &frameStatus,
                    ICONA_ErrorCode, &errorCode,
                    TAG_END) == 1)
    {
        /* A frameStatus of 0 means it has a frame, any other value means it does not */
        BOOL hasFrame = (frameStatus == 0) ? TRUE : FALSE;

        /* Cleanup */
        FreeDiskObject(icon);
        CloseLibrary(IconBase);
        free(newIconName);

        return hasFrame;
    }
    else
    {
        CONSOLE_ERROR("Failed to retrieve frame information; error code: %ld\n", errorCode);
        PrintFault(errorCode, NULL);
    }

    /* Cleanup */
    if (icon)
    {
        FreeDiskObject(icon);
    }
    CloseLibrary(IconBase);
    free(newIconName);
    return TRUE; /* Default to having a frame if there's an error */
#else
    /* Host build stub - assume frames */
    (void)iconName;
    return TRUE;
#endif
}
void dumpIconArrayToScreen(IconArray *iconArray)
{
    int i;
    const char *iconTypeStr;
    char xStr[12], yStr[12]; // Buffers for X and Y, can hold "None" or numbers

    #ifdef DEBUG
    if(iconArray->size==0)
    {
        append_to_log("No icons in the folder that can be arranged\n");
    }
    else
    {
    append_to_log("%-3s | %-6s | %-6s | %-6s | %-6s | %-6s | %-6s | %-8s | %-8s | %-6s | %-8s | %-6s | %-10s | %-10s | %-20s\n",
                  "ID", "Width", "Height", "TxtW", "TxtH", "MaxW", "MaxH", "X", "Y", "Border", "Type", "IsDir", "Size", "Date", "Icon");
    append_to_log("------------------------------------------------------------------------------------------------------------------------------------------\n");
    }
#endif

    for (i = 0; i < iconArray->size; i++)
    {
        char dateStr[16];
        
        // Determine the icon type string
        switch (iconArray->array[i].icon_type)
        {
            case 1:
                iconTypeStr = "NewIcon";
                break;
            case 2:
                iconTypeStr = "OS3.5";
                break;
            default:
                iconTypeStr = "Standard";
                break;
        }

        // Convert X and Y to "None" if they are -2147483648, otherwise store the number
        if (iconArray->array[i].icon_x == -2147483648)
            strcpy(xStr, "None");
        else
            sprintf(xStr, "%d", iconArray->array[i].icon_x);

        if (iconArray->array[i].icon_y == -2147483648)
            strcpy(yStr, "None");
        else
            sprintf(yStr, "%d", iconArray->array[i].icon_y);

        // Format the date (Days.Minutes format for compactness)
        sprintf(dateStr, "%lu.%lu", 
                (unsigned long)iconArray->array[i].file_date.ds_Days,
                (unsigned long)iconArray->array[i].file_date.ds_Minute);

        append_to_log("%-3d | %-6d | %-6d | %-6d | %-6d | %-6d | %-6d | %-8s | %-8s | %-6d | %-8s | %-6s | %-10lu | %-10s | %-20s\n",
                      i,
                      iconArray->array[i].icon_width,
                      iconArray->array[i].icon_height,
                      iconArray->array[i].text_width,
                      iconArray->array[i].text_height,
                      iconArray->array[i].icon_max_width,
                      iconArray->array[i].icon_max_height,
                      xStr,  // X position (or "None")
                      yStr,  // Y position (or "None")
                      iconArray->array[i].border_width,
                      iconTypeStr,
                      iconArray->array[i].is_folder ? "Folder" : "File",
                      (unsigned long)iconArray->array[i].file_size,
                      dateStr,
                      iconArray->array[i].icon_text);
    }
}







BOOL IsValidIcon(const char *iconPath)
{
    struct DiskObject *diskObj;

    /* Attempt to get the DiskObject from the icon file */
    diskObj = GetDiskObject(iconPath);
    if (diskObj != NULL)
    {
#ifdef DEBUG_MAX
        append_to_log("Icon is a valid icon: %s\n", iconPath);
#endif
        /* Successfully retrieved the DiskObject, so the icon is valid */
        FreeDiskObject(diskObj); /* Clean up to prevent memory leaks */
        return TRUE;
    }
    else
    {
        /* Failed to retrieve the DiskObject, so the icon is invalid */
        log_message(LOG_ICONS, LOG_LEVEL_WARNING, "Icon is NOT a valid icon: %s", iconPath);
        return FALSE;
    }
}
