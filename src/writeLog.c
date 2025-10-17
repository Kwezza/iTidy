/* VBCC MIGRATION NOTE (Stage 2): Complete rewrite for AmigaDOS file I/O
 * 
 * Changes:
 * - Replaced FILE* with BPTR for AmigaDOS compatibility
 * - Replaced fopen() with Open(filename, MODE_NEWFILE)
 * - Replaced fprintf()/vfprintf() with vsnprintf() + Write()
 * - Replaced fclose() with Close()
 * - Log location: PROGDIR:iTidy.log (same directory as executable)
 * - Improved error handling with IoErr()
 * - Used C99 features: inline, //, mixed declarations
 * - Used snprintf for safe string formatting
 * 
 * VBCC MIGRATION NOTE (Stage 4): Changed log location from T: to PROGDIR:
 * - T: is RAM-based and lost on crashes/resets
 * - PROGDIR:iTidy.log persists and is easy to find
 */

#include "writeLog.h"
#include "file_directory_handling.h"

// Helper function to get timestamp string
void getTimestamp(char *buffer, int bufferSize)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    if (timeinfo) {
        snprintf(buffer, bufferSize, "[%02d:%02d:%02d]", 
                 timeinfo->tm_hour, 
                 timeinfo->tm_min, 
                 timeinfo->tm_sec);
    } else {
        strncpy(buffer, "[00:00:00]", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
}

// Helper function to open log file for appending
// Returns BPTR to opened file, or 0 on failure
// VBCC MIGRATION NOTE (Stage 4): Uses PROGDIR: so log survives crashes
// Opens in append mode (MODE_READWRITE) to preserve existing log data
static inline BPTR OpenLogFile(const char *preferredPath, const char *fallbackPath)
{
    BPTR logFile;
    
    // Try preferred path first (PROGDIR: - same directory as executable)
    // MODE_READWRITE opens existing file, or returns 0 if doesn't exist
    logFile = Open(preferredPath, MODE_READWRITE);
    if (logFile) {
        return logFile;
    }
    
    // If file doesn't exist, create it with MODE_NEWFILE
    logFile = Open(preferredPath, MODE_NEWFILE);
    if (logFile) {
        return logFile;
    }
    
    // If preferred path failed completely, try fallback (current directory)
    if (fallbackPath) {
        logFile = Open(fallbackPath, MODE_READWRITE);
        if (!logFile) {
            logFile = Open(fallbackPath, MODE_NEWFILE);
        }
    }
    
    return logFile;
}

void append_to_log(const char *format, ...)
{
    BPTR logFile;
    char buffer[1024];
    char timestamp[16];
    va_list args;
    LONG bytesWritten;
    LONG messageLen;

    // Get timestamp
    getTimestamp(timestamp, sizeof(timestamp));

    // Format the message
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Try to open log file (PROGDIR: = same directory as executable, fallback to current dir)
    logFile = OpenLogFile("PROGDIR:iTidy.log", "iTidy.log");
    if (!logFile) {
        // Could not open log file at all
        LONG error = IoErr();
        (void)error; // Suppress unused variable warning if DEBUG not defined
#ifdef DEBUG
        Printf("Error opening log file: %ld\n", error);
#endif
        return;
    }

    // Seek to end of file for append
    Seek(logFile, 0, OFFSET_END);

    // Write timestamp
    bytesWritten = Write(logFile, timestamp, strlen(timestamp));
    if (bytesWritten < 0) {
        Close(logFile);
        return;
    }

    // Write space separator
    Write(logFile, " ", 1);

    // Write message
    messageLen = strlen(buffer);
    bytesWritten = Write(logFile, buffer, messageLen);
    if (bytesWritten < 0) {
        Close(logFile);
        return;
    }

    // Close the file
    Close(logFile);
}

void initialize_logfile(void)
{
    BPTR logFile;
    char startMarker[] = "\n\n========================================\n";

    // VBCC MIGRATION NOTE (Stage 4): Initialize log with marker, preserving existing content
    // Open for append if exists, create if doesn't exist
    logFile = Open("PROGDIR:iTidy.log", MODE_READWRITE);
    if (!logFile) {
        // File doesn't exist, create it
        logFile = Open("PROGDIR:iTidy.log", MODE_NEWFILE);
    }
    
    if (!logFile) {
        // Try fallback to current directory
        logFile = Open("iTidy.log", MODE_READWRITE);
        if (!logFile) {
            logFile = Open("iTidy.log", MODE_NEWFILE);
        }
    }

    if (logFile) {
        // Seek to end and write session marker
        Seek(logFile, 0, OFFSET_END);
        Write(logFile, startMarker, strlen(startMarker));
        Close(logFile);
    } else {
        LONG error = IoErr();
        (void)error; // Suppress unused variable warning if DEBUG not defined
#ifdef DEBUG
        Printf("Error initializing log file: %ld\n", error);
#endif
    }
}