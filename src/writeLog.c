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
#include <devices/timer.h>
#include <clib/timer_protos.h>

// External timer device (initialized in spinner.c)
extern struct Device* TimerBase;

// Logging performance tracking
static ULONG totalLogCalls = 0;
static ULONG totalLogMicroseconds = 0;
static ULONG totalBytesWritten = 0;

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
    struct timeval startTime, endTime;
    ULONG elapsedMicros;

    // Start timing
    if (TimerBase) {
        GetSysTime(&startTime);
    }

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
    totalBytesWritten += bytesWritten;

    // Write space separator
    bytesWritten = Write(logFile, " ", 1);
    totalBytesWritten += (bytesWritten > 0) ? bytesWritten : 0;

    // Write message
    messageLen = strlen(buffer);
    bytesWritten = Write(logFile, buffer, messageLen);
    if (bytesWritten < 0) {
        Close(logFile);
        return;
    }
    totalBytesWritten += bytesWritten;

    // Close the file
    Close(logFile);

    // End timing and accumulate
    if (TimerBase) {
        GetSysTime(&endTime);
        elapsedMicros = ((endTime.tv_secs - startTime.tv_secs) * 1000000) +
                        (endTime.tv_micro - startTime.tv_micro);
        totalLogMicroseconds += elapsedMicros;
        totalLogCalls++;
    }
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

// Delete the log file (call at program start for fresh log each run)
void delete_logfile(void)
{
    // Try to delete from preferred location (PROGDIR:)
    DeleteFile("PROGDIR:iTidy.log");
    
    // Also try fallback location (current directory)
    DeleteFile("iTidy.log");
}

// Reset logging performance statistics
void reset_log_performance_stats(void)
{
    totalLogCalls = 0;
    totalLogMicroseconds = 0;
    totalBytesWritten = 0;
}

// Print logging performance statistics
void print_log_performance_stats(void)
{
    ULONG avgMicros;
    ULONG totalMillis;
    BPTR logFile;
    char buffer[512];
    
    if (totalLogCalls == 0) {
        Printf("\n==== LOGGING OVERHEAD STATS ====\n");
        Printf("No log calls recorded.\n");
        Printf("================================\n\n");
        
        /* Also write to log file (without timing this write!) */
        logFile = OpenLogFile("PROGDIR:iTidy.log", "iTidy.log");
        if (logFile) {
            Seek(logFile, 0, OFFSET_END);
            Write(logFile, "\n==== LOGGING OVERHEAD STATS ====\n", 35);
            Write(logFile, "No log calls recorded.\n", 23);
            Write(logFile, "================================\n\n", 34);
            Close(logFile);
        }
        return;
    }
    
    avgMicros = totalLogMicroseconds / totalLogCalls;
    totalMillis = totalLogMicroseconds / 1000;
    
    /* Print to console */
    Printf("\n==== LOGGING OVERHEAD STATS ====\n");
    Printf("Total log calls: %lu\n", totalLogCalls);
    Printf("Total time: %lu microseconds (%lu.%03lu ms)\n", 
           totalLogMicroseconds, totalMillis, totalLogMicroseconds % 1000);
    Printf("Average per call: %lu microseconds (%lu.%03lu ms)\n",
           avgMicros, avgMicros / 1000, avgMicros % 1000);
    Printf("Total bytes written: %lu bytes (%lu KB)\n",
           totalBytesWritten, totalBytesWritten / 1024);
    Printf("================================\n\n");
    
    /* Also write to log file (without timing this write!) */
    logFile = OpenLogFile("PROGDIR:iTidy.log", "iTidy.log");
    if (logFile) {
        Seek(logFile, 0, OFFSET_END);
        
        Write(logFile, "\n==== LOGGING OVERHEAD STATS ====\n", 35);
        
        snprintf(buffer, sizeof(buffer), "Total log calls: %lu\n", totalLogCalls);
        Write(logFile, buffer, strlen(buffer));
        
        snprintf(buffer, sizeof(buffer), "Total time: %lu microseconds (%lu.%03lu ms)\n", 
                 totalLogMicroseconds, totalMillis, totalLogMicroseconds % 1000);
        Write(logFile, buffer, strlen(buffer));
        
        snprintf(buffer, sizeof(buffer), "Average per call: %lu microseconds (%lu.%03lu ms)\n",
                 avgMicros, avgMicros / 1000, avgMicros % 1000);
        Write(logFile, buffer, strlen(buffer));
        
        snprintf(buffer, sizeof(buffer), "Total bytes written: %lu bytes (%lu KB)\n",
                 totalBytesWritten, totalBytesWritten / 1024);
        Write(logFile, buffer, strlen(buffer));
        
        Write(logFile, "================================\n\n", 34);
        Close(logFile);
    }
}
