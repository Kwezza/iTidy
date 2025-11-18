/**
 * @file string_functions.h
 * @brief String manipulation and formatting utilities
 * 
 * Provides common string operations including date/time formatting,
 * text manipulation, and validation functions.
 */

#ifndef STRING_FUNCTIONS_H
#define STRING_FUNCTIONS_H

#include <exec/types.h>

/**
 * @brief Format a timestamp string to human-readable date/time
 * 
 * Converts a timestamp in YYYYMMDD_HHMMSS format to DD-Mon-YYYY HH:MM format.
 * 
 * Examples:
 *   "20251124_123804" -> "24-Nov-2025 12:38"
 *   "20250101_000000" -> "01-Jan-2025 00:00"
 * 
 * @param timestamp_str Input string in YYYYMMDD_HHMMSS format (15 chars)
 * @param output_buffer Buffer to receive formatted string (must be at least 21 bytes)
 * @param buffer_size Size of output buffer (should be >= 21)
 * @return TRUE if successful, FALSE if input is invalid or NULL
 * 
 * @note This function validates the input format but does NOT validate
 *       if the date is actually valid (e.g., it won't catch Feb 31)
 * @note Output buffer will contain error message on failure
 */
BOOL iTidy_FormatTimestamp(const char *timestamp_str, 
                           char *output_buffer, 
                           UWORD buffer_size);

#endif /* STRING_FUNCTIONS_H */
