/**
 * backup_lha.h - iTidy Backup System LHA Wrapper Interface
 * 
 * Provides platform-independent interface to LHA archiver functionality.
 * Handles executable detection, archive creation, and extraction.
 * 
 * Platform Support:
 *   - Host: Uses system() with lha command-line tool
 *   - Amiga: Uses Execute() with LhA command-line tool
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

#ifndef BACKUP_LHA_H
#define BACKUP_LHA_H

#include "backup_types.h"

/*========================================================================*/
/* LHA Detection                                                          */
/*========================================================================*/

/**
 * @brief Check if LHA executable is available
 * 
 * Attempts to locate and verify the LHA archiver executable.
 * On host systems, searches PATH for 'lha' or 'lha.exe'.
 * On Amiga, searches common locations (C:, SYS:Tools/, etc.).
 * 
 * @param lhaPath Buffer to receive path to LHA executable (min 32 bytes)
 * @return TRUE if LHA found and working, FALSE otherwise
 * 
 * Example:
 *   char lhaPath[32];
 *   if (CheckLhaAvailable(lhaPath)) {
 *       printf("Found LHA at: %s\n", lhaPath);
 *   }
 */
BOOL CheckLhaAvailable(char *lhaPath);

/**
 * @brief Get LHA version string
 * 
 * Executes 'lha --version' or equivalent and parses the version.
 * Useful for logging and compatibility checks.
 * 
 * @param lhaPath Path to LHA executable
 * @param versionBuffer Buffer for version string (min 64 bytes)
 * @return TRUE if version retrieved, FALSE on error
 */
BOOL GetLhaVersion(const char *lhaPath, char *versionBuffer);

/*========================================================================*/
/* Archive Creation                                                       */
/*========================================================================*/

/**
 * @brief Create LHA archive from directory
 * 
 * Compresses an entire directory tree into an LHA archive.
 * Uses appropriate compression level and recursion flags.
 * 
 * Command format (host):
 *   lha a -r archive.lha sourcedir/
 * 
 * Command format (Amiga):
 *   LhA a -r archive.lha sourcedir
 * 
 * @param lhaPath Path to LHA executable
 * @param archivePath Full path where archive should be created
 * @param sourceDir Directory to archive (can be root volume)
 * @return TRUE if archive created successfully, FALSE on error
 * 
 * Example:
 *   CreateLhaArchive("lha", "Run_0001/000/00001.lha", "DH0:Projects/");
 */
BOOL CreateLhaArchive(const char *lhaPath, const char *archivePath, 
                      const char *sourceDir);

/**
 * @brief Add path marker file to existing archive
 * 
 * Appends the _PATH.txt recovery file to an archive.
 * Used to store original path information for restore.
 * 
 * @param lhaPath Path to LHA executable
 * @param archivePath Path to existing archive
 * @param markerFile Path to _PATH.txt file to add
 * @return TRUE if file added successfully, FALSE on error
 */
BOOL AddFileToArchive(const char *lhaPath, const char *archivePath,
                      const char *markerFile);

/**
 * @brief Get size of created archive
 * 
 * Returns the size of an LHA archive file in bytes.
 * Used for statistics and catalog generation.
 * 
 * @param archivePath Path to archive file
 * @return Size in bytes, or 0 on error
 */
ULONG GetArchiveSize(const char *archivePath);

/*========================================================================*/
/* Archive Extraction                                                     */
/*========================================================================*/

/**
 * @brief Extract LHA archive to directory
 * 
 * Decompresses entire archive to specified destination.
 * Preserves directory structure and file attributes.
 * 
 * Command format (host):
 *   lha x archive.lha destdir/
 * 
 * Command format (Amiga):
 *   LhA x archive.lha destdir
 * 
 * @param lhaPath Path to LHA executable
 * @param archivePath Path to archive to extract
 * @param destDir Destination directory (will be created if needed)
 * @return TRUE if extracted successfully, FALSE on error
 * 
 * Example:
 *   ExtractLhaArchive("lha", "Run_0001/000/00001.lha", "TEMP:Restore/");
 */
BOOL ExtractLhaArchive(const char *lhaPath, const char *archivePath,
                       const char *destDir);

/**
 * @brief Extract single file from archive
 * 
 * Extracts only a specific file (typically _PATH.txt) from archive.
 * 
 * @param lhaPath Path to LHA executable
 * @param archivePath Path to archive
 * @param fileName Name of file to extract
 * @param destDir Destination directory
 * @return TRUE if file extracted, FALSE on error
 */
BOOL ExtractFileFromArchive(const char *lhaPath, const char *archivePath,
                             const char *fileName, const char *destDir);

/**
 * @brief Test archive integrity
 * 
 * Verifies archive is not corrupted and can be extracted.
 * Uses 'lha t' command to test without extracting.
 * 
 * @param lhaPath Path to LHA executable
 * @param archivePath Path to archive to test
 * @return TRUE if archive is valid, FALSE if corrupted
 */
BOOL TestLhaArchive(const char *lhaPath, const char *archivePath);

/*========================================================================*/
/* Archive Information                                                    */
/*========================================================================*/

/**
 * @brief List contents of archive
 * 
 * Executes 'lha l' to list files in archive.
 * Useful for verification and debugging.
 * 
 * @param lhaPath Path to LHA executable
 * @param archivePath Path to archive
 * @param callback Function called for each file in archive
 * @return TRUE if list successful, FALSE on error
 */
typedef BOOL (*LhaListCallback)(const char *fileName, ULONG size);
BOOL ListLhaArchive(const char *lhaPath, const char *archivePath,
                    LhaListCallback callback);

/*========================================================================*/
/* Utility Functions                                                      */
/*========================================================================*/

/**
 * @brief Build LHA command string
 * 
 * Helper function to construct platform-specific LHA commands.
 * Handles path quoting and argument formatting.
 * 
 * @param command Buffer for command string (min 512 bytes)
 * @param lhaPath Path to LHA executable
 * @param operation LHA operation (a, x, l, t)
 * @param archivePath Archive file path
 * @param targetPath Target directory or file
 * @return TRUE if command built successfully, FALSE if too long
 */
BOOL BuildLhaCommand(char *command, const char *lhaPath, 
                     const char *operation, const char *archivePath,
                     const char *targetPath);

/**
 * @brief Execute LHA command
 * 
 * Platform-specific command execution.
 * Uses system() on host, Execute() on Amiga.
 * 
 * @param command Full command string to execute
 * @return TRUE if command succeeded (exit code 0), FALSE on error
 */
BOOL ExecuteLhaCommand(const char *command);

#endif /* BACKUP_LHA_H */
