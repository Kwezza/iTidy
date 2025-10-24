/**
 * backup_marker.h - iTidy Backup System Path Marker Interface
 * 
 * Manages _PATH.txt recovery files that store original folder paths.
 * These markers are added to each archive to enable accurate restore operations.
 * 
 * Purpose:
 *   When a folder "DH0:Projects/MyGame/" is backed up to "00042.lha",
 *   a _PATH.txt file is created containing "DH0:Projects/MyGame/".
 *   This allows the restore operation to know the original location.
 * 
 * File Format:
 *   Line 1: Original path (e.g., "DH0:Projects/MyGame/")
 *   Line 2: Backup timestamp (optional)
 *   Line 3: Archive index (optional)
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

#ifndef BACKUP_MARKER_H
#define BACKUP_MARKER_H

#include "backup_types.h"

/* Marker file constants */
#define MARKER_FILENAME "_PATH.txt"
#define MAX_MARKER_PATH 256

/*========================================================================*/
/* Path Marker Creation                                                   */
/*========================================================================*/

/**
 * @brief Create path marker file
 * 
 * Creates a _PATH.txt file containing the original folder path.
 * This file will be added to the archive for restore operations.
 * 
 * @param markerPath Full path where marker file should be created
 * @param originalPath Original folder path to store in marker
 * @param archiveIndex Archive index (for reference)
 * @return TRUE if marker created successfully, FALSE on error
 * 
 * Example:
 *   CreatePathMarkerFile("TEMP:_PATH.txt", "DH0:Projects/MyGame/", 42);
 *   // Creates file containing:
 *   // DH0:Projects/MyGame/
 *   // Archive: 00042
 */
BOOL CreatePathMarkerFile(const char *markerPath, const char *originalPath,
                          ULONG archiveIndex);

/**
 * @brief Create path marker in temporary location
 * 
 * Creates _PATH.txt in a temporary directory suitable for adding to archive.
 * The marker file path is returned in the buffer.
 * 
 * @param markerPathOut Buffer to receive marker file path (min 256 bytes)
 * @param originalPath Original folder path to store
 * @param archiveIndex Archive index
 * @param tempDir Temporary directory to use (or NULL for system temp)
 * @return TRUE if marker created, FALSE on error
 * 
 * Example:
 *   char markerPath[256];
 *   CreateTempPathMarker(markerPath, "Work:Documents/", 100, "TEMP:");
 *   // Creates TEMP:/_PATH.txt and returns path in markerPath
 */
BOOL CreateTempPathMarker(char *markerPathOut, const char *originalPath,
                          ULONG archiveIndex, const char *tempDir);

/**
 * @brief Get default temporary directory
 * 
 * Returns platform-appropriate temporary directory path.
 * - Windows: Uses TEMP environment variable
 * - Unix: Uses /tmp
 * - Amiga: Uses TEMP: or RAM:
 * 
 * @param tempDirOut Buffer for temp directory path (min 256 bytes)
 * @return TRUE if temp dir determined, FALSE on error
 */
BOOL GetTempDirectory(char *tempDirOut);

/*========================================================================*/
/* Path Marker Reading                                                    */
/*========================================================================*/

/**
 * @brief Read path marker file
 * 
 * Reads _PATH.txt and extracts the original folder path.
 * Used during restore operations to determine destination.
 * 
 * @param markerPath Full path to _PATH.txt file
 * @param originalPathOut Buffer for original path (min 256 bytes)
 * @param archiveIndexOut Pointer to receive archive index (or NULL)
 * @return TRUE if marker read successfully, FALSE on error
 * 
 * Example:
 *   char originalPath[256];
 *   ULONG archiveIndex;
 *   if (ReadPathMarkerFile("TEMP:_PATH.txt", originalPath, &archiveIndex)) {
 *       printf("Restore to: %s\n", originalPath);
 *   }
 */
BOOL ReadPathMarkerFile(const char *markerPath, char *originalPathOut,
                        ULONG *archiveIndexOut);

/**
 * @brief Extract and read marker from archive
 * 
 * Convenience function that extracts _PATH.txt from an archive
 * and reads the original path in one operation.
 * 
 * @param archivePath Path to archive file
 * @param lhaPath Path to LHA executable
 * @param originalPathOut Buffer for original path (min 256 bytes)
 * @param tempDir Temp directory for extraction (or NULL)
 * @return TRUE if marker extracted and read, FALSE on error
 * 
 * Example:
 *   char originalPath[256];
 *   ExtractAndReadMarker("Run_0001/000/00042.lha", "lha", 
 *                        originalPath, "TEMP:");
 */
BOOL ExtractAndReadMarker(const char *archivePath, const char *lhaPath,
                          char *originalPathOut, const char *tempDir);

/*========================================================================*/
/* Path Marker Validation                                                 */
/*========================================================================*/

/**
 * @brief Validate path marker format
 * 
 * Checks if a path marker file contains valid data.
 * Verifies path is not empty and follows expected format.
 * 
 * @param markerPath Path to marker file to validate
 * @return TRUE if marker is valid, FALSE if invalid or missing
 */
BOOL ValidatePathMarker(const char *markerPath);

/**
 * @brief Check if archive contains path marker
 * 
 * Tests whether an archive file contains a _PATH.txt marker.
 * Does not extract the marker, just checks for its presence.
 * 
 * @param archivePath Path to archive file
 * @param lhaPath Path to LHA executable
 * @return TRUE if marker present, FALSE if absent
 */
BOOL ArchiveHasMarker(const char *archivePath, const char *lhaPath);

/*========================================================================*/
/* Utility Functions                                                      */
/*========================================================================*/

/**
 * @brief Build marker file path
 * 
 * Constructs full path to _PATH.txt in specified directory.
 * 
 * @param markerPathOut Buffer for marker path (min 256 bytes)
 * @param directory Directory where marker should be placed
 * @return TRUE if path built successfully, FALSE if too long
 * 
 * Example:
 *   char path[256];
 *   BuildMarkerPath(path, "TEMP:");
 *   // Returns "TEMP:/_PATH.txt" in path
 */
BOOL BuildMarkerPath(char *markerPathOut, const char *directory);

/**
 * @brief Delete temporary marker file
 * 
 * Removes marker file after it has been added to archive.
 * 
 * @param markerPath Path to marker file to delete
 * @return TRUE if deleted, FALSE on error (non-critical)
 */
BOOL DeleteMarkerFile(const char *markerPath);

/**
 * @brief Format timestamp for marker
 * 
 * Creates human-readable timestamp string for marker file.
 * Format: "YYYY-MM-DD HH:MM:SS"
 * 
 * @param timestampOut Buffer for timestamp (min 32 bytes)
 * @return TRUE if timestamp formatted, FALSE on error
 */
BOOL FormatMarkerTimestamp(char *timestampOut);

#endif /* BACKUP_MARKER_H */
