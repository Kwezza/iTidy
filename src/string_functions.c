/**
 * @file string_functions.c
 * @brief String manipulation and formatting utilities
 */

#include <exec/types.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "string_functions.h"

/* Month abbreviations for formatting */
static const char *month_names[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/**
 * @brief Check if a character is a digit
 */
static BOOL is_digit_char(char c)
{
    return (c >= '0' && c <= '9');
}

/**
 * @brief Extract a numeric value from a string position
 * 
 * @param str Source string
 * @param start Starting position
 * @param length Number of digits to extract
 * @return Extracted numeric value, or -1 on error
 */
static int extract_number(const char *str, int start, int length)
{
    int i;
    int result = 0;
    
    for (i = 0; i < length; i++) {
        char c = str[start + i];
        if (!is_digit_char(c)) {
            return -1;  /* Invalid digit */
        }
        result = result * 10 + (c - '0');
    }
    
    return result;
}

/**
 * @brief Format a timestamp string to human-readable date/time
 * 
 * Converts YYYYMMDD_HHMMSS format to DD-Mon-YYYY HH:MM format.
 */
BOOL iTidy_FormatTimestamp(const char *timestamp_str, 
                           char *output_buffer, 
                           UWORD buffer_size)
{
    int year, month, day, hour, minute, second;
    
    /* Validate inputs */
    if (!timestamp_str || !output_buffer || buffer_size < 21) {
        if (output_buffer && buffer_size > 0) {
            strncpy(output_buffer, "(invalid)", buffer_size - 1);
            output_buffer[buffer_size - 1] = '\0';
        }
        return FALSE;
    }
    
    /* Check string length (YYYYMMDD_HHMMSS = 15 chars) */
    if (strlen(timestamp_str) != 15) {
        strncpy(output_buffer, "(bad format)", buffer_size - 1);
        output_buffer[buffer_size - 1] = '\0';
        return FALSE;
    }
    
    /* Check for underscore separator at position 8 */
    if (timestamp_str[8] != '_') {
        strncpy(output_buffer, "(bad format)", buffer_size - 1);
        output_buffer[buffer_size - 1] = '\0';
        return FALSE;
    }
    
    /* Extract date components: YYYYMMDD */
    year = extract_number(timestamp_str, 0, 4);
    month = extract_number(timestamp_str, 4, 2);
    day = extract_number(timestamp_str, 6, 2);
    
    /* Extract time components: HHMMSS */
    hour = extract_number(timestamp_str, 9, 2);
    minute = extract_number(timestamp_str, 11, 2);
    second = extract_number(timestamp_str, 13, 2);
    
    /* Validate extracted values */
    if (year < 0 || month < 0 || day < 0 || 
        hour < 0 || minute < 0 || second < 0) {
        strncpy(output_buffer, "(parse error)", buffer_size - 1);
        output_buffer[buffer_size - 1] = '\0';
        return FALSE;
    }
    
    /* Validate ranges (basic validation) */
    if (year < 1978 || year > 2099 ||   /* Amiga era range */
        month < 1 || month > 12 ||
        day < 1 || day > 31 ||
        hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 ||
        second < 0 || second > 59) {
        strncpy(output_buffer, "(out of range)", buffer_size - 1);
        output_buffer[buffer_size - 1] = '\0';
        return FALSE;
    }
    
    /* Format as DD-Mon-YYYY HH:MM */
    /* Example: "24-Nov-2025 12:38" */
    sprintf(output_buffer, "%02d-%s-%04d %02d:%02d",
            day,
            month_names[month - 1],
            year,
            hour,
            minute);
    
    return TRUE;
}
