#ifndef WRITELOG_H
#define WRITELOG_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>  // Required for time_t, time(), localtime()
#include <string.h> // Required for strcpy()

void append_to_log(const char *format, ...);
void initialize_logfile(void);

#endif