/*============================================================================*/
/* Path Utilities Module - Global path manipulation functions                */
/*============================================================================*/
/**
 * @file path_utilities.c
 * @brief Implementation of reusable path truncation and abbreviation utilities
 * 
 * Provides consistent path handling across all iTidy GUI windows with support
 * for both fixed-width and proportional fonts.
 * 
 * Created: 2025-11-13
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>
#include <proto/graphics.h>
#include <graphics/text.h>

#include "path_utilities.h"

/*------------------------------------------------------------------------*/
/* iTidy_TruncatePathMiddle - Character-based path truncation            */
/*------------------------------------------------------------------------*/
/**
 * Truncate a path in the middle preserving device and filename.
 * Uses character counts - suitable for fixed-width fonts.
 */
void iTidy_TruncatePathMiddle(const char *path, char *output, int max_chars)
{
    int len;
    int left_part_len;
    int right_part_len;
    const char *last_slash;
    const char *last_colon;
    
    /* Defensive programming - handle NULL inputs */
    if (path == NULL || output == NULL || max_chars < 10)
    {
        if (output != NULL && max_chars > 0)
            output[0] = '\0';
        return;
    }
    
    len = strlen(path);
    
    /* If it fits, just copy it */
    if (len <= max_chars)
    {
        strcpy(output, path);
        return;
    }
    
    /* Find the last slash or colon to identify the program/file name */
    last_slash = strrchr(path, '/');
    last_colon = strrchr(path, ':');
    
    /* Determine which separator is last */
    if (last_slash == NULL && last_colon == NULL)
    {
        /* No path separators - just truncate in middle */
        left_part_len = (max_chars - 3) / 2;
        right_part_len = max_chars - 3 - left_part_len;
        
        strncpy(output, path, left_part_len);
        output[left_part_len] = '\0';
        strcat(output, "...");
        strncat(output, path + len - right_part_len, right_part_len);
    }
    else
    {
        /* We have path separators - try to show device and program/file name */
        const char *separator = (last_slash > last_colon) ? last_slash : last_colon;
        int program_name_len = strlen(separator); /* Includes the separator */
        
        /* Calculate how much space we have for the left part */
        /* Format: "device:...program" or "device:.../program" */
        left_part_len = max_chars - 3 - program_name_len;
        
        if (left_part_len < 5)
        {
            /* Not enough space to show device - just show end with ellipsis */
            strcpy(output, "...");
            strncat(output, separator, max_chars - 3);
            output[max_chars] = '\0';
        }
        else
        {
            /* Show device/start + ... + program/file name */
            strncpy(output, path, left_part_len);
            output[left_part_len] = '\0';
            strcat(output, "...");
            strcat(output, separator);
        }
    }
    
    /* Ensure null termination */
    output[max_chars] = '\0';
}

/*------------------------------------------------------------------------*/
/* iTidy_TruncatePathMiddlePixels - Pixel-based path truncation          */
/*------------------------------------------------------------------------*/
/**
 * Truncate a path in the middle using pixel measurements.
 * Suitable for proportional fonts - uses TextLength() for accuracy.
 */
void iTidy_TruncatePathMiddlePixels(struct RastPort *rp,
                                     const char *path,
                                     char *output,
                                     UWORD max_width)
{
    char buffer[256];
    UWORD text_width;
    ULONG text_len;
    const char *ellipsis = "...";
    UWORD ellipsis_width;
    ULONG start_chars, end_chars;
    UWORD target_width, half_target;
    
    /* Defensive programming - handle NULL inputs */
    if (!rp || !path || !output)
    {
        if (output)
            output[0] = '\0';
        return;
    }
    
    text_len = strlen(path);
    
    /* Check if path fits as-is */
    text_width = (UWORD)TextLength(rp, (STRPTR)path, (LONG)text_len);
    if (text_width <= max_width)
    {
        /* Fits fine - copy as-is */
        strcpy(output, path);
        return;
    }
    
    /* Path needs truncation - middle truncation for paths */
    ellipsis_width = (UWORD)TextLength(rp, (STRPTR)ellipsis, 3);
    target_width = max_width - ellipsis_width;
    half_target = target_width / 2;
    
    /* Find how many characters fit in first half */
    for (start_chars = 0; start_chars < text_len; start_chars++)
    {
        UWORD w = (UWORD)TextLength(rp, (STRPTR)path, (LONG)start_chars);
        if (w >= half_target)
            break;
    }
    
    /* Find how many characters fit in second half (measure from end) */
    for (end_chars = 0; end_chars < text_len; end_chars++)
    {
        const char *end_start = path + text_len - end_chars;
        UWORD w = (UWORD)TextLength(rp, (STRPTR)end_start, (LONG)end_chars);
        if (w >= half_target)
            break;
    }
    
    /* Safety check - ensure we don't overflow buffer */
    if (start_chars + 3 + end_chars >= sizeof(buffer))
    {
        start_chars = 50;  /* Fallback to safe values */
        end_chars = 50;
    }
    
    /* Build truncated string: "start...end" */
    if (start_chars > 0)
        strncpy(buffer, path, start_chars);
    else
        buffer[0] = '\0';
    
    strcpy(buffer + start_chars, ellipsis);
    
    if (end_chars > 0)
        strcpy(buffer + start_chars + 3, path + text_len - end_chars);
    
    buffer[sizeof(buffer) - 1] = '\0';
    
    /* Copy to output */
    strcpy(output, buffer);
}

/*------------------------------------------------------------------------*/
/* iTidy_ShortenPathWithParentDir - Intelligent /../ abbreviation        */
/*------------------------------------------------------------------------*/
/**
 * Intelligent Amiga-style path abbreviation using /../ notation.
 * Preserves device, first directory, and filename while collapsing middle.
 */
BOOL iTidy_ShortenPathWithParentDir(const char *path,
                                     char *output,
                                     int max_chars)
{
    int len;
    char temp_path[256];
    char result[256];
    const char *device_end;
    const char *first_dir_end;
    const char *last_slash;
    int device_len, first_dir_len, filename_len;
    int abbreviated_len;
    
    /* Defensive programming */
    if (path == NULL || output == NULL || max_chars < 10)
    {
        if (output != NULL && max_chars > 0)
            output[0] = '\0';
        return FALSE;
    }
    
    len = strlen(path);
    
    /* If it already fits, no need to abbreviate */
    if (len <= max_chars)
    {
        strcpy(output, path);
        return FALSE;
    }
    
    /* Make a working copy */
    if (len >= sizeof(temp_path))
    {
        strncpy(temp_path, path, sizeof(temp_path) - 1);
        temp_path[sizeof(temp_path) - 1] = '\0';
    }
    else
    {
        strcpy(temp_path, path);
    }
    
    /* Find device/volume (everything up to and including first ':') */
    device_end = strchr(temp_path, ':');
    if (device_end == NULL)
    {
        /* No device - just do simple middle truncation */
        iTidy_TruncatePathMiddle(path, output, max_chars);
        return TRUE;
    }
    
    device_len = (device_end - temp_path) + 1; /* Include the ':' */
    
    /* Find the filename (last component after final '/') */
    last_slash = strrchr(temp_path, '/');
    if (last_slash == NULL || last_slash <= device_end)
    {
        /* No directory structure - can't use /../ notation */
        strcpy(output, path);
        return FALSE;
    }
    
    filename_len = strlen(last_slash); /* Includes the '/' */
    
    /* Find first directory (between device and next '/') */
    first_dir_end = strchr(device_end + 1, '/');
    if (first_dir_end == NULL || first_dir_end == last_slash)
    {
        /* Only one directory level - no middle to collapse */
        strcpy(output, path);
        return FALSE;
    }
    
    first_dir_len = (first_dir_end - (device_end + 1)); /* Length of first dir name only */
    
    /* Try format: "device:firstdir/../filename" */
    abbreviated_len = device_len + first_dir_len + 4 + filename_len; /* 4 for "/.." + "/" from filename */
    
    if (abbreviated_len <= max_chars)
    {
        /* Build: "device:firstdir/../filename" */
        strncpy(result, temp_path, device_len + first_dir_len);
        result[device_len + first_dir_len] = '\0';
        strcat(result, "/..");
        strcat(result, last_slash);
        strcpy(output, result);
        return TRUE;
    }
    
    /* Still too long - try more aggressive: "device:/../filename" */
    abbreviated_len = device_len + 3 + filename_len; /* 3 for "/.." */
    
    if (abbreviated_len <= max_chars)
    {
        /* Build: "device:/../filename" */
        strncpy(result, temp_path, device_len);
        result[device_len] = '\0';
        strcat(result, "/..");
        strcat(result, last_slash);
        strcpy(output, result);
        return TRUE;
    }
    
    /* Even that's too long - fall back to regular truncation */
    iTidy_TruncatePathMiddle(path, output, max_chars);
    return TRUE;
}
