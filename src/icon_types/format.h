#ifndef ICON_TYPES_FORMAT_H
#define ICON_TYPES_FORMAT_H

/**
 * @file format.h
 * @brief Icon format detection and size extraction functions
 * 
 * This module provides functions to:
 * - Detect icon format (Standard, NewIcon, OS3.5, DISK type)
 * - Extract icon dimensions from various formats
 * 
 * These functions perform low-level icon format parsing and are the
 * foundation for more optimized icon reading operations.
 */

#include <exec/types.h>
#include <workbench/workbench.h>
#include "../itidy_types.h"

/*========================================================================*/
/* Icon Format Detection                                                  */
/*========================================================================*/

/**
 * @brief Check if icon is NewIcon format (via DiskObject)
 * 
 * Detects NewIcon format by looking for the signature tooltype:
 * "*** DON'T EDIT THE FOLLOWING LINES!! ***"
 * 
 * @param diskObject Loaded DiskObject to check
 * @return TRUE if NewIcon format detected
 */
BOOL IsNewIcon(struct DiskObject *diskObject);

/**
 * @brief Check if icon file is NewIcon format (via path)
 * 
 * Loads icon from path and checks for NewIcon signature.
 * 
 * @param filePath Path to icon file (with or without .info)
 * @return TRUE if NewIcon format detected
 */
BOOL IsNewIconPath(const STRPTR filePath);

/**
 * @brief Check if icon file is OS3.5 format (FORM/ICON chunks)
 * 
 * Scans icon file for IFF FORM/ICON chunk headers.
 * 
 * @param filename Path to icon file
 * @return 1 if OS3.5 format, 0 otherwise
 */
int isOS35IconFormat(const char *filename);

/**
 * @brief Check if icon type is DISK (ic_Type field)
 * 
 * Reads icon file byte at offset 0x30 to check if ic_Type == 1 (DISK).
 * Always returns 0 for directories (fib_DirEntryType > 0).
 * 
 * @param filename Path to icon file
 * @param fib_DirEntryType FileInfoBlock directory entry type
 * @return 1 if DISK type, 0 otherwise
 */
int isIconTypeDisk(const char *filename, long fib_DirEntryType);

/*========================================================================*/
/* Icon Size Extraction                                                   */
/*========================================================================*/

/**
 * @brief Get NewIcon size from IM1= tooltype
 * 
 * Extracts icon dimensions from NewIcon IM1= tooltype string.
 * Format: "IM1=" + Transparency + Width + Height + image data
 * Width/Height are encoded as character - '!' (0x21).
 * 
 * @param filePath Path to icon file (with or without .info)
 * @param newIconSize Pointer to IconSize structure to fill
 */
void GetNewIconSizePath(const char *filePath, IconSize *newIconSize);

/**
 * @brief Get standard icon size from DiskObject gadget
 * 
 * Loads icon and extracts width/height from do_Gadget structure.
 * 
 * @param filePath Path to icon file (with or without .info)
 * @param iconSize Pointer to IconSize structure to fill
 * @return TRUE if successful, FALSE on error
 */
BOOL GetStandardIconSize(const char *filePath, IconSize *iconSize);

/**
 * @brief Get OS3.5 icon size from FACE chunk
 * 
 * Scans icon file for IFF FACE chunk and extracts width/height.
 * 
 * @param filename Path to icon file
 * @param size Pointer to IconSize structure to fill
 * @return 1 if FACE chunk found and size extracted, 0 otherwise
 */
int getOS35IconSize(const char *filename, IconSize *size);

/**
 * @brief Get icon size from raw file read (offset 0x08/0x0A)
 * 
 * Reads first 12 bytes of icon file and extracts width/height
 * from standard offset positions (0x08 for width, 0x0A for height).
 * 
 * @param filePath Path to icon file
 * @param iconSize Pointer to IconSize structure to fill
 * @return TRUE if successful, FALSE on error
 */
BOOL GetIconSizeFromFile(const char *filePath, IconSize *iconSize);

/**
 * @brief Get icon position (X, Y coordinates) from .info file
 * 
 * Loads icon and extracts do_CurrentX and do_CurrentY.
 * Returns (-1, -1) if file doesn't exist or load fails.
 * 
 * @param iconPath Path to icon file (with or without .info)
 * @return IconPosition structure with X/Y coordinates
 */
IconPosition GetIconPositionFromPath(const char *iconPath);

#endif /* ICON_TYPES_FORMAT_H */
