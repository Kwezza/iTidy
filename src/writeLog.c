/* Enhanced Multi-Category Logging System
 * 
 * Features:
 * - Multiple log categories with separate files
 * - Timestamped logs (one set per run)
 * - Log levels (DEBUG, INFO, WARNING, ERROR)
 * - Runtime enable/disable per category
 * - Automatic error.log duplication
 * - All logs in PROGDIR:logs/ directory
 * 
 * Migration Notes:
 * - Replaces single iTidy.log with category-based logs
 * - Uses AmigaDOS file I/O (BPTR, Open, Write, Close)
 * - Memory tracking logs write immediately (high-frequency)
 */

#include "platform/platform.h"

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

#include "writeLog.h"
#include "file_directory_handling.h"
#include <devices/timer.h>
#include <clib/timer_protos.h>
#include <proto/dos.h>

// External timer device (initialized in spinner.c)
extern struct Device* TimerBase;

/*---------------------------------------------------------------------------*/
/* Log System State                                                          */
/*---------------------------------------------------------------------------*/

typedef struct {
    char filename[256];         /* Full path to log file */
    BPTR fileHandle;            /* AmigaDOS file handle (0 if closed) */
    BOOL enabled;               /* Is this category enabled? */
    LogLevel minLevel;          /* Minimum level to log */
    ULONG messageCount;         /* Number of messages logged */
    ULONG bytesWritten;         /* Total bytes written */
} LogCategoryState;

static LogCategoryState g_logCategories[LOG_CATEGORY_COUNT];
static char g_logTimestamp[32] = {0};
static char g_logsDirectory[256] = {0};
static BOOL g_logSystemInitialized = FALSE;

// Global log level and memory tracking control
static LogLevel g_globalLogLevel = LOG_LEVEL_ERROR;  /* Default to ERROR (quietest) */
static BOOL g_memoryLoggingEnabled = FALSE;
static BOOL g_performanceLoggingEnabled = FALSE;

// Logging performance tracking
static ULONG totalLogCalls = 0;
static ULONG totalLogMicroseconds = 0;

/*---------------------------------------------------------------------------*/
/* Category Names                                                            */
/*---------------------------------------------------------------------------*/

static const char* g_categoryNames[] = {
    "general",
    "memory",
    "gui",
    "icons",
    "backup",
    "errors"
};

static const char* g_levelNames[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR"
};

const char* get_log_category_name(LogCategory category) {
    if (category >= 0 && category < LOG_CATEGORY_COUNT) {
        return g_categoryNames[category];
    }
    return "unknown";
}

const char* get_log_level_name(LogLevel level) {
    if (level >= 0 && level <= LOG_LEVEL_ERROR) {
        return g_levelNames[level];
    }
    return "UNKNOWN";
}

/*---------------------------------------------------------------------------*/
/* Timestamp Generation                                                      */
/*---------------------------------------------------------------------------*/

/* Generate timestamp for log filenames: 2025-10-27_14-23-45 */
static void generate_timestamp_string(char *buffer, int bufferSize) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    if (timeinfo) {
        snprintf(buffer, bufferSize, "%04d-%02d-%02d_%02d-%02d-%02d",
                 timeinfo->tm_year + 1900,
                 timeinfo->tm_mon + 1,
                 timeinfo->tm_mday,
                 timeinfo->tm_hour,
                 timeinfo->tm_min,
                 timeinfo->tm_sec);
    } else {
        strncpy(buffer, "unknown_time", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
}

/* Generate timestamp for log entries: [14:23:45] */
static void get_entry_timestamp(char *buffer, int bufferSize) {
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

/*---------------------------------------------------------------------------*/
/* Directory Management                                                      */
/*---------------------------------------------------------------------------*/

/* Create logs directory if it doesn't exist */
static BOOL ensure_logs_directory(void) {
    BPTR lock;
    BPTR parentLock;
    LONG error;
    
    /* First check if directory already exists */
    lock = Lock(g_logsDirectory, ACCESS_READ);
    if (lock) {
        UnLock(lock);
        return TRUE;
    }
    
    /* Try method 1: Direct CreateDir() */
    lock = CreateDir(g_logsDirectory);
    if (lock) {
        UnLock(lock);
        return TRUE;
    }
    
    error = IoErr();
    
    /* Try method 2: Navigate to PROGDIR: first, then create subdirectory */
    parentLock = Lock("PROGDIR:", ACCESS_READ);
    if (parentLock) {
        BPTR oldDir = CurrentDir(parentLock);
        
        /* Try creating "logs" in current directory */
        lock = CreateDir("logs");
        if (lock) {
            UnLock(lock);
            CurrentDir(oldDir);
            UnLock(parentLock);
            return TRUE;
        }
        
        CurrentDir(oldDir);
        UnLock(parentLock);
    }
    
    /* Failed to create directory - get error code */
    error = IoErr();
    
    /* Try to log the error to console since logging system isn't ready yet */
#ifdef DEBUG
    Printf("WARNING: Failed to create logs directory '%s' (IoErr: %ld)\n", 
           g_logsDirectory, error);
    
    /* Check if PROGDIR: itself is accessible */
    lock = Lock("PROGDIR:", ACCESS_READ);
    if (lock) {
        Printf("PROGDIR: is accessible\n");
        UnLock(lock);
    } else {
        Printf("ERROR: PROGDIR: is NOT accessible! (IoErr: %ld)\n", IoErr());
    }
    
    Printf("Falling back to PROGDIR: (logs in same directory as executable)\n");
#endif
    
    return FALSE;
}

/* Delete all files in logs directory */
static void clean_logs_directory(void) {
    struct AnchorPath *anchor;
    LONG result;
    char pattern[300];
    
    snprintf(pattern, sizeof(pattern), "%s#?", g_logsDirectory);
    
    anchor = (struct AnchorPath *)whd_malloc(sizeof(struct AnchorPath) + 512);
    if (!anchor) {
        return;
    }
    memset(anchor, 0, sizeof(struct AnchorPath) + 512);
    
    anchor->ap_BreakBits = 0;
    anchor->ap_Strlen = 0;
    
    result = MatchFirst(pattern, anchor);
    while (result == 0) {
        if (anchor->ap_Info.fib_DirEntryType < 0) { /* File, not directory */
            char fullPath[512];
            snprintf(fullPath, sizeof(fullPath), "%s%s", 
                    g_logsDirectory, anchor->ap_Info.fib_FileName);
            DeleteFile(fullPath);
        }
        result = MatchNext(anchor);
    }
    
    MatchEnd(anchor);
    FreeVec(anchor);
}

/*---------------------------------------------------------------------------*/
/* Log System Initialization                                                 */
/*---------------------------------------------------------------------------*/

void initialize_log_system(BOOL cleanOldLogs) {
    int i;
    
    /* Generate timestamp for this run */
    generate_timestamp_string(g_logTimestamp, sizeof(g_logTimestamp));
    
    /* Set up logs directory path */
    snprintf(g_logsDirectory, sizeof(g_logsDirectory), "PROGDIR:logs/");
    
    /* Create logs directory */
    if (!ensure_logs_directory()) {
        /* If we can't create it, fall back to PROGDIR: */
        strncpy(g_logsDirectory, "PROGDIR:", sizeof(g_logsDirectory));
        g_logsDirectory[sizeof(g_logsDirectory) - 1] = '\0';
    }
    
    /* Clean old logs if requested */
    if (cleanOldLogs) {
        clean_logs_directory();
    }
    
    /* Initialize each category */
    for (i = 0; i < LOG_CATEGORY_COUNT; i++) {
        snprintf(g_logCategories[i].filename, sizeof(g_logCategories[i].filename),
                "%s%s_%s.log", g_logsDirectory, g_categoryNames[i], g_logTimestamp);
        
        g_logCategories[i].fileHandle = 0;
        g_logCategories[i].enabled = TRUE;  /* All enabled by default */
        g_logCategories[i].minLevel = LOG_LEVEL_DEBUG;  /* Log everything by default */
        g_logCategories[i].messageCount = 0;
        g_logCategories[i].bytesWritten = 0;
    }
    
    g_logSystemInitialized = TRUE;
    
    /* Write initialization message to general log */
    log_info(LOG_GENERAL, "Log system initialized - timestamp: %s\n", g_logTimestamp);
    log_info(LOG_GENERAL, "Logs directory: %s\n", g_logsDirectory);
    
#ifdef DEBUG
    /* Log all category filenames for debugging */
    for (i = 0; i < LOG_CATEGORY_COUNT; i++) {
        log_debug(LOG_GENERAL, "Category %s -> %s\n", 
                 g_categoryNames[i], g_logCategories[i].filename);
    }
#endif
}

void shutdown_log_system(void) {
    int i;
    
    if (!g_logSystemInitialized) {
        return;
    }
    
    /* Write final stats to general log */
    log_info(LOG_GENERAL, "\n========== LOG SYSTEM SHUTDOWN ==========\n");
    for (i = 0; i < LOG_CATEGORY_COUNT; i++) {
        if (g_logCategories[i].messageCount > 0) {
            log_info(LOG_GENERAL, "  %s: %lu messages, %lu bytes\n",
                    g_categoryNames[i],
                    g_logCategories[i].messageCount,
                    g_logCategories[i].bytesWritten);
        }
    }
    log_info(LOG_GENERAL, "=========================================\n\n");
    
    /* Close all open file handles */
    for (i = 0; i < LOG_CATEGORY_COUNT; i++) {
        if (g_logCategories[i].fileHandle) {
            Close(g_logCategories[i].fileHandle);
            g_logCategories[i].fileHandle = 0;
        }
    }
    
    g_logSystemInitialized = FALSE;
}

/*---------------------------------------------------------------------------*/
/* Category Control                                                          */
/*---------------------------------------------------------------------------*/

void enable_log_category(LogCategory category, BOOL enable) {
    if (category >= 0 && category < LOG_CATEGORY_COUNT) {
        g_logCategories[category].enabled = enable;
    }
}

BOOL is_log_category_enabled(LogCategory category) {
    if (category >= 0 && category < LOG_CATEGORY_COUNT) {
        return g_logCategories[category].enabled;
    }
    return FALSE;
}

void set_log_level(LogCategory category, LogLevel minLevel) {
    if (category >= 0 && category < LOG_CATEGORY_COUNT) {
        g_logCategories[category].minLevel = minLevel;
    }
}

void set_global_log_level(LogLevel minLevel) {
    int i;
    g_globalLogLevel = minLevel;
    for (i = 0; i < LOG_CATEGORY_COUNT; i++) {
        g_logCategories[i].minLevel = minLevel;
    }
}

LogLevel get_global_log_level(void) {
    return g_globalLogLevel;
}

void set_memory_logging_enabled(BOOL enabled) {
    g_memoryLoggingEnabled = enabled;
    if (!enabled) {
        /* Disable memory category */
        enable_log_category(LOG_MEMORY, FALSE);
    } else {
        /* Enable memory category */
        enable_log_category(LOG_MEMORY, TRUE);
        /* Force DEBUG level for memory logging (allocations use log_debug) */
        set_log_level(LOG_MEMORY, LOG_LEVEL_DEBUG);
    }
}

BOOL is_memory_logging_enabled(void) {
    return g_memoryLoggingEnabled;
}

void set_performance_logging_enabled(BOOL enabled) {
    g_performanceLoggingEnabled = enabled;
    if (enabled) {
        log_info(LOG_GENERAL, "Performance logging: ENABLED\n");
    } else {
        log_info(LOG_GENERAL, "Performance logging: DISABLED\n");
    }
}

BOOL is_performance_logging_enabled(void) {
    return g_performanceLoggingEnabled;
}

/*---------------------------------------------------------------------------*/
/* Core Logging Function                                                     */
/*---------------------------------------------------------------------------*/

void log_message(LogCategory category, LogLevel level, const char *format, ...) {
    char buffer[1024];
    char timestamp[16];
    char levelPrefix[32];
    va_list args;
    LONG bytesWritten;
    BPTR logFile;
    struct timeval startTime, endTime;
    ULONG elapsedMicros;
    
    /* Validate and check if logging is enabled */
    if (!g_logSystemInitialized) {
        return;
    }
    
    if (category < 0 || category >= LOG_CATEGORY_COUNT) {
        return;
    }
    
    if (!g_logCategories[category].enabled) {
        return;
    }
    
    if (level < g_logCategories[category].minLevel) {
        return;
    }
    
    /* Start timing (except for memory logs to avoid recursion) */
    if (TimerBase && category != LOG_MEMORY) {
        GetSysTime(&startTime);
    }
    
    /* Get timestamp */
    get_entry_timestamp(timestamp, sizeof(timestamp));
    
    /* Format level prefix: [14:23:45][INFO] */
    snprintf(levelPrefix, sizeof(levelPrefix), "%s[%s] ",
            timestamp, get_log_level_name(level));
    
    /* Format the message */
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    /* Strip newline characters to keep log format consistent */
    {
        char *src = buffer;
        char *dst = buffer;
        while (*src) {
            if (*src != '\n' && *src != '\r') {
                *dst++ = *src;
            } else {
                /* Replace newlines with spaces to maintain readability */
                if (dst > buffer && *(dst - 1) != ' ') {
                    *dst++ = ' ';
                }
            }
            src++;
        }
        *dst = '\0';
    }
    
    /* Open log file (create if doesn't exist, append if it does) */
    logFile = Open(g_logCategories[category].filename, MODE_READWRITE);
    if (!logFile) {
        logFile = Open(g_logCategories[category].filename, MODE_NEWFILE);
    }
    
    if (logFile) {
        /* Seek to end for append */
        Seek(logFile, 0, OFFSET_END);
        
        /* Write level prefix */
        bytesWritten = Write(logFile, levelPrefix, strlen(levelPrefix));
        if (bytesWritten > 0) {
            g_logCategories[category].bytesWritten += bytesWritten;
        }
        
        /* Write message */
        bytesWritten = Write(logFile, buffer, strlen(buffer));
        if (bytesWritten > 0) {
            g_logCategories[category].bytesWritten += bytesWritten;
        }
        
        /* Write newline to end the log entry */
        bytesWritten = Write(logFile, "\n", 1);
        if (bytesWritten > 0) {
            g_logCategories[category].bytesWritten += bytesWritten;
        }
        
        /* CRITICAL: Flush to disk before close (prevents data loss on crash) */
        Flush(logFile);
        
        /* Close immediately (especially important for memory tracking) */
        Close(logFile);
        
        g_logCategories[category].messageCount++;
    }
    
    /* If this is an error, also write to error.log (avoid infinite recursion) */
    if (level == LOG_LEVEL_ERROR && category != LOG_ERRORS) {
        /* Open error log */
        logFile = Open(g_logCategories[LOG_ERRORS].filename, MODE_READWRITE);
        if (!logFile) {
            logFile = Open(g_logCategories[LOG_ERRORS].filename, MODE_NEWFILE);
        }
        
        if (logFile) {
            char errorPrefix[64];
            
            Seek(logFile, 0, OFFSET_END);
            
            /* Add category context: [14:23:45][ERROR][memory] */
            snprintf(errorPrefix, sizeof(errorPrefix), "%s[ERROR][%s] ",
                    timestamp, get_log_category_name(category));
            
            Write(logFile, errorPrefix, strlen(errorPrefix));
            Write(logFile, buffer, strlen(buffer));
            Write(logFile, "\n", 1);
            Flush(logFile);
            Close(logFile);
            
            g_logCategories[LOG_ERRORS].messageCount++;
        }
    }
    
    /* End timing */
    if (TimerBase && category != LOG_MEMORY) {
        GetSysTime(&endTime);
        elapsedMicros = ((endTime.tv_secs - startTime.tv_secs) * 1000000) +
                        (endTime.tv_micro - startTime.tv_micro);
        totalLogMicroseconds += elapsedMicros;
        totalLogCalls++;
    }
}

/*---------------------------------------------------------------------------*/
/* Legacy Compatibility Functions                                            */
/*---------------------------------------------------------------------------*/

void append_to_log(const char *format, ...) {
    char buffer[1024];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    log_message(LOG_GENERAL, LOG_LEVEL_INFO, "%s", buffer);
}

void initialize_logfile(void) {
    /* Legacy function - map to new system */
    if (!g_logSystemInitialized) {
        initialize_log_system(TRUE);  /* Clean old logs by default */
    }
}

void delete_logfile(void) {
    /* Legacy function - clean logs directory */
    if (g_logSystemInitialized) {
        clean_logs_directory();
    }
}

/*---------------------------------------------------------------------------*/
/* Performance Tracking                                                      */
/*---------------------------------------------------------------------------*/

void reset_log_performance_stats(void) {
    totalLogCalls = 0;
    totalLogMicroseconds = 0;
}

void print_log_performance_stats(void) {
    ULONG avgMicros;
    ULONG totalMillis;
    char buffer[512];
    int i;
    
    /* Only print if performance logging is enabled */
    if (!g_performanceLoggingEnabled) {
        return;
    }
    
    log_info(LOG_GENERAL, "\n========== LOGGING PERFORMANCE STATS ==========\n");
    
    if (totalLogCalls == 0) {
        log_info(LOG_GENERAL, "No timed log calls recorded.\n");
    } else {
        avgMicros = totalLogMicroseconds / totalLogCalls;
        totalMillis = totalLogMicroseconds / 1000;
        
        log_info(LOG_GENERAL, "Total timed calls: %lu\n", totalLogCalls);
        log_info(LOG_GENERAL, "Total time: %lu.%03lu ms\n",
                totalMillis, totalLogMicroseconds % 1000);
        log_info(LOG_GENERAL, "Average: %lu.%03lu ms per call\n",
                avgMicros / 1000, avgMicros % 1000);
    }
    
    log_info(LOG_GENERAL, "\nPer-Category Statistics:\n");
    for (i = 0; i < LOG_CATEGORY_COUNT; i++) {
        if (g_logCategories[i].messageCount > 0) {
            log_info(LOG_GENERAL, "  %-8s: %5lu messages, %7lu bytes (%lu KB)\n",
                    g_categoryNames[i],
                    g_logCategories[i].messageCount,
                    g_logCategories[i].bytesWritten,
                    g_logCategories[i].bytesWritten / 1024);
        }
    }
    
    log_info(LOG_GENERAL, "===============================================\n\n");
    
    /* Also print to console */
    Printf("\nLogging stats written to: %s\n", g_logCategories[LOG_GENERAL].filename);
}
