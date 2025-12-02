/*
 * tool_cache_reports.h - Tool Cache Export/Report Functions
 * 
 * Provides export functionality for the tool cache window, generating
 * formatted text reports of tools and their associated files.
 */

#ifndef TOOL_CACHE_REPORTS_H
#define TOOL_CACHE_REPORTS_H

#include <intuition/intuition.h>

/**
 * @brief Export unique tool list with status, icon count, and version
 * 
 * Generates a formatted text file containing all unique tools found in the
 * global tool cache, sorted by missing status first, then alphabetically.
 * 
 * Displays an ASL file requester for the user to select the output file,
 * then exports a report with:
 * - Tool Name (dynamically sized column)
 * - Status (Found/Missing)
 * - Icons (count of files using this tool)
 * - Version (version string if available)
 * 
 * @param window Parent window for ASL requester and error dialogs
 * @param folder_path Current folder path being scanned (for report header)
 */
void export_tool_list(struct Window *window, const char *folder_path);

/**
 * @brief Export files and their associated default tools
 * 
 * Generates a formatted text file containing all files in the tool cache
 * and their associated default tools, sorted by:
 * 1. Missing status (missing tools first)
 * 2. Tool name (alphabetically)
 * 3. File path (alphabetically)
 * 
 * Displays an ASL file requester for the user to select the output file,
 * then exports a report with:
 * - Default Tool (dynamically sized column)
 * - Status (Valid/Missing)
 * - File Path (full path to .info file)
 * 
 * @param window Parent window for ASL requester and error dialogs
 * @param folder_path Current folder path being scanned (for report header)
 */
void export_files_and_tools_list(struct Window *window, const char *folder_path);

#endif /* TOOL_CACHE_REPORTS_H */
