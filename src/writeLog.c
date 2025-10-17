/* VBCC MIGRATION NOTE (Stage 2): Complete rewrite for AmigaDOS file I/O
 * 
 * Changes:
 * - Replaced FILE* with BPTR for AmigaDOS compatibility
 * - Replaced fopen() with Open(filename, MODE_NEWFILE)
 * - Replaced fprintf()/vfprintf() with vsnprintf() + Write()
 * - Replaced fclose() with Close()
 * - Added fallback path logic: Bin/Amiga/logs/iTidy.log -> T:iTidy.log
 * - Improved error handling with IoErr()
 * - Used C99 features: inline, //, mixed declarations
 * - Used snprintf for safe string formatting
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

// Helper function to open log file with fallback
// Returns BPTR to opened file, or 0 on failure
static inline BPTR OpenLogFile(const char *preferredPath, const char *fallbackPath)
{
    BPTR logFile;
    
    // Try preferred path first
    logFile = Open(preferredPath, MODE_NEWFILE);
    if (logFile) {
        return logFile;
    }
    
    // If preferred path failed, try fallback
    if (fallbackPath) {
        logFile = Open(fallbackPath, MODE_NEWFILE);
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

    // Try to open log file (primary path with fallback)
    logFile = OpenLogFile("Bin/Amiga/logs/iTidy.log", "T:iTidy.log");
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

    // Try primary path first
    logFile = Open("Bin/Amiga/logs/iTidy.log", MODE_NEWFILE);
    if (!logFile) {
        // Try fallback path
        logFile = Open("T:iTidy.log", MODE_NEWFILE);
    }

    if (logFile) {
        Close(logFile);
    } else {
        LONG error = IoErr();
        (void)error; // Suppress unused variable warning if DEBUG not defined
#ifdef DEBUG
        Printf("Error initializing log file: %ld\n", error);
#endif
    }
}