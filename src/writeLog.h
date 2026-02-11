#ifndef WRITELOG_H
#define WRITELOG_H

/* Enhanced Multi-Category Logging System
 * 
 * Features:
 * - Multiple log categories (general, memory, GUI, icons, backup, errors)
 * - Log levels (DEBUG, INFO, WARNING, ERROR)
 * - Timestamped log files (one set per program run)
 * - Runtime enable/disable per category
 * - Automatic error duplication to error.log
 * - All logs stored in PROGDIR:logs/ directory
 */

#include <proto/dos.h>
#include <dos/dos.h>
#include <stdarg.h>
#include <time.h>      // Required for time_t, time(), localtime()
#include <string.h>    // Required for string operations
#include <stdio.h>     // Required for vsnprintf()
#include <exec/types.h>

/*---------------------------------------------------------------------------*/
/* Log Categories                                                            */
/*---------------------------------------------------------------------------*/

typedef enum {
    LOG_GENERAL,    /* General program flow */
    LOG_MEMORY,     /* Memory allocations/frees (high-frequency) */
    LOG_GUI,        /* GUI events and interactions */
    LOG_ICONS,      /* Icon processing and management */
    LOG_BACKUP,     /* Backup system operations */
    LOG_ERRORS,     /* Errors only (also auto-copied from other logs) */
    LOG_CATEGORY_COUNT  /* Keep this last - used for array sizing */
} LogCategory;

/*---------------------------------------------------------------------------*/
/* Log Levels                                                                */
/*---------------------------------------------------------------------------*/

typedef enum {
    LOG_LEVEL_DEBUG,    /* Verbose debugging information */
    LOG_LEVEL_INFO,     /* Normal informational messages */
    LOG_LEVEL_WARNING,  /* Warning conditions */
    LOG_LEVEL_ERROR,    /* Error conditions */
    LOG_LEVEL_DISABLED  /* Logging disabled (no messages output) */
} LogLevel;

/*---------------------------------------------------------------------------*/
/* Core Logging Functions                                                    */
/*---------------------------------------------------------------------------*/

/* Initialize the logging system - creates logs directory and timestamp */
void initialize_log_system(BOOL cleanOldLogs);

/* Shutdown logging system - closes all open file handles */
void shutdown_log_system(void);

/* Get the current log timestamp string (e.g., "2026-02-06_14-30-45") */
const char* get_log_timestamp(void);

/* Main logging function with category and level */
void log_message(LogCategory category, LogLevel level, const char *format, ...);

/* Convenience macros for common log levels */
#define log_debug(cat, ...) log_message(cat, LOG_LEVEL_DEBUG, __VA_ARGS__)
#define log_info(cat, ...) log_message(cat, LOG_LEVEL_INFO, __VA_ARGS__)
#define log_warning(cat, ...) log_message(cat, LOG_LEVEL_WARNING, __VA_ARGS__)
#define log_error(cat, ...) log_message(cat, LOG_LEVEL_ERROR, __VA_ARGS__)

/*---------------------------------------------------------------------------*/
/* Category Control                                                          */
/*---------------------------------------------------------------------------*/

/* Enable/disable specific log categories at runtime */
void enable_log_category(LogCategory category, BOOL enable);

/* Check if a category is enabled */
BOOL is_log_category_enabled(LogCategory category);

/* Set minimum log level for a category (filters out lower priority) */
void set_log_level(LogCategory category, LogLevel minLevel);

/*---------------------------------------------------------------------------*/
/* Legacy Compatibility                                                      */
/*---------------------------------------------------------------------------*/

/* Legacy function - maps to LOG_GENERAL category */
void append_to_log(const char *format, ...);

/* Legacy initialization - kept for backward compatibility */
void initialize_logfile(void);
void delete_logfile(void);

/*---------------------------------------------------------------------------*/
/* Performance Tracking                                                      */
/*---------------------------------------------------------------------------*/

void reset_log_performance_stats(void);
void print_log_performance_stats(void);

/*---------------------------------------------------------------------------*/
/* Global Log Level Control                                                  */
/*---------------------------------------------------------------------------*/

/* Set minimum log level for all categories */
void set_global_log_level(LogLevel minLevel);

/* Get current global log level */
LogLevel get_global_log_level(void);

/* Enable/disable memory category logging */
void set_memory_logging_enabled(BOOL enabled);

/* Check if memory logging is enabled */
BOOL is_memory_logging_enabled(void);

/* Enable/disable performance timing logging */
void set_performance_logging_enabled(BOOL enabled);

/* Check if performance logging is enabled */
BOOL is_performance_logging_enabled(void);

/*---------------------------------------------------------------------------*/
/* Utility Functions                                                         */
/*---------------------------------------------------------------------------*/

/* Get human-readable category name */
const char* get_log_category_name(LogCategory category);

/* Get human-readable level name */
const char* get_log_level_name(LogLevel level);

#endif /* WRITELOG_H */