#ifndef WRITELOG_H
#define WRITELOG_H

/* VBCC MIGRATION NOTE (Stage 2): Migrated to AmigaDOS file I/O
 * - Uses AmigaDOS Open/Write/Close instead of fopen/fprintf/fclose
 * - BPTR used for file handles instead of FILE*
 * - Fallback path support: Bin/Amiga/logs/iTidy.log -> T:iTidy.log
 */

#include <proto/dos.h>
#include <dos/dos.h>
#include <stdarg.h>
#include <time.h>      // Required for time_t, time(), localtime()
#include <string.h>    // Required for string operations
#include <stdio.h>     // Required for vsnprintf()

void append_to_log(const char *format, ...);
void initialize_logfile(void);
void delete_logfile(void);

#endif