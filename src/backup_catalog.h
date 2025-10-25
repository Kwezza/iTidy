/**
 * backup_catalog.h - iTidy Backup System Catalog Management
 * 
 * Manages the catalog.txt file that maps archive indices to original folder paths.
 * Provides human-readable and machine-parseable backup manifest.
 * 
 * Design: BACKUP_SYSTEM_PROPOSAL.md
 * Implementation: BACKUP_IMPLEMENTATION_GUIDE.md - Phase 2, Task 4
 * 
 * Author: Kerry Thompson
 * Date: October 24, 2025
 */

#ifndef BACKUP_CATALOG_H
#define BACKUP_CATALOG_H

#include "backup_types.h"

/*========================================================================*/
/* Catalog File Management                                               */
/*========================================================================*/

/**
 * @brief Create and initialize a new catalog file
 * 
 * Creates catalog.txt in the run directory and writes the header.
 * The catalog file is kept open for appending entries throughout
 * the backup session.
 * 
 * Header format:
 *   iTidy Backup Catalog v1.0
 *   ========================================
 *   Run Number: 0001
 *   Session Started: 2025-10-24 14:30:00
 *   LhA Version: 1.38
 *   ========================================
 *   
 *   # Index    | Subfolder | Size    | Original Path
 *   -----------+-----------+---------+----------------
 * 
 * The file handle is stored in ctx->catalogFile for subsequent writes.
 * 
 * @param ctx Backup context with run directory path
 * @return TRUE on success, FALSE on failure
 */
BOOL CreateCatalog(BackupContext *ctx);

/**
 * @brief Append an entry to the catalog
 * 
 * Writes a single backup entry to the catalog in pipe-delimited format.
 * This is called after each successful (or failed) folder backup.
 * 
 * Entry format:
 *   00042.lha  | 000/      | 22 KB   | DH0:Projects/MyFolder/
 * 
 * Fields:
 *   - Archive name (5-digit index + .lha)
 *   - Subfolder location (hierarchical folder)
 *   - Archive size in KB (or "N/A" if failed)
 *   - Original folder path
 * 
 * @param ctx Backup context with open catalog file
 * @param entry Catalog entry with archive metadata
 * @return TRUE on success, FALSE on failure
 */
BOOL AppendCatalogEntry(BackupContext *ctx, const BackupArchiveEntry *entry);

/**
 * @brief Close the catalog file
 * 
 * Writes the catalog footer with session statistics and closes the file.
 * 
 * Footer format:
 *   ========================================
 *   Session Ended: 2025-10-24 14:35:12
 *   Total Archives: 12345
 *   Successful: 12300
 *   Failed: 45
 *   Total Size: 1.2 GB
 *   ========================================
 * 
 * @param ctx Backup context with open catalog file
 * @return TRUE on success, FALSE on failure
 */
BOOL CloseCatalog(BackupContext *ctx);

/**
 * @brief Parse a catalog file and invoke callback for each entry
 * 
 * Reads a catalog.txt file and calls the provided callback function
 * for each valid entry found. Used for restore operations and
 * catalog browsing.
 * 
 * The callback receives a pointer to a BackupArchiveEntry structure
 * for each entry. The callback can return FALSE to stop parsing early.
 * 
 * Example usage:
 *   void ProcessEntry(const BackupArchiveEntry *entry) {
 *       printf("Archive: %s, Path: %s\n", 
 *              entry->archiveName, entry->originalPath);
 *       return TRUE; // Continue parsing
 *   }
 *   
 *   ParseCatalog("PROGDIR:Backups/Run_0001/catalog.txt", ProcessEntry, NULL);
 * 
 * @param catalogPath Full path to catalog.txt file
 * @param callback Function to call for each entry (return FALSE to stop)
 * @param userData Optional user data passed to callback (can be NULL)
 * @return TRUE if parsing completed, FALSE on error
 */
BOOL ParseCatalog(const char *catalogPath, 
                  BOOL (*callback)(const BackupArchiveEntry *entry, void *userData),
                  void *userData);

/**
 * @brief Find a specific entry in the catalog by archive index
 * 
 * Searches the catalog for an entry with the specified archive index
 * and returns its information.
 * 
 * Example:
 *   BackupArchiveEntry entry;
 *   if (FindCatalogEntry("PROGDIR:Backups/Run_0001/catalog.txt", 42, &entry)) {
 *       printf("Archive 42 backed up: %s\n", entry.originalPath);
 *   }
 * 
 * @param catalogPath Full path to catalog.txt file
 * @param archiveIndex Archive index to find
 * @param outEntry Buffer to receive entry information
 * @return TRUE if found, FALSE if not found or error
 */
BOOL FindCatalogEntry(const char *catalogPath, ULONG archiveIndex,
                      BackupArchiveEntry *outEntry);

/**
 * @brief Get catalog file path from run directory
 * 
 * Builds the full path to catalog.txt given a run directory.
 * 
 * Example:
 *   GetCatalogPath(buf, "PROGDIR:Backups/Run_0001")
 *     → "PROGDIR:Backups/Run_0001/catalog.txt"
 * 
 * @param outPath Buffer for catalog path (min MAX_BACKUP_PATH bytes)
 * @param runDirectory Full path to run directory
 * @return TRUE on success, FALSE if path too long
 */
BOOL GetCatalogPath(char *outPath, const char *runDirectory);

/**
 * @brief Count total entries in a catalog file
 * 
 * Parses catalog and counts valid entries. Useful for progress
 * indicators and statistics.
 * 
 * @param catalogPath Full path to catalog.txt file
 * @return Number of entries, or 0 on error
 */
UWORD CountCatalogEntries(const char *catalogPath);

/**
 * @brief Format a size value for catalog display
 * 
 * Converts byte size to human-readable format (KB, MB, GB).
 * 
 * Examples:
 *   FormatSizeForCatalog(buf, 1024) → "1 KB"
 *   FormatSizeForCatalog(buf, 15360) → "15 KB"
 *   FormatSizeForCatalog(buf, 1048576) → "1024 KB"
 *   FormatSizeForCatalog(buf, 0) → "N/A"
 * 
 * @param outStr Buffer for formatted size (min 16 bytes)
 * @param sizeBytes Size in bytes (0 for N/A)
 */
void FormatSizeForCatalog(char *outStr, ULONG sizeBytes);

/**
 * @brief Parse a catalog entry line
 * 
 * Parses a single pipe-delimited catalog line into a BackupArchiveEntry.
 * 
 * Format: "00042.lha | 000/ | 22 KB | DH0:Projects/MyFolder/"
 * 
 * @param line Catalog line to parse
 * @param outEntry Buffer to receive parsed entry
 * @return TRUE if parsed successfully, FALSE if invalid format
 */
BOOL ParseCatalogLine(const char *line, BackupArchiveEntry *outEntry);

#endif /* BACKUP_CATALOG_H */
