#ifndef ICON_TYPES_WRITER_H
#define ICON_TYPES_WRITER_H

/**
 * @file writer.h
 * @brief Icon modification functions
 * 
 * This module provides functions to modify icon properties and write
 * them back to disk.
 */

#include <exec/types.h>

/*========================================================================*/
/* Icon Modification                                                      */
/*========================================================================*/

/**
 * @brief Update the default tool for an icon
 * 
 * Loads icon, modifies its default tool field, and writes it back to disk.
 * 
 * @param iconPath Path to icon file (with or without .info extension)
 * @param newDefaultTool New default tool path (or NULL to clear)
 * @return TRUE if successful, FALSE on error
 */
BOOL SetIconDefaultTool(const char *iconPath, const char *newDefaultTool);

#endif /* ICON_TYPES_WRITER_H */
