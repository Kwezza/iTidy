#ifndef WRITELOG_H
#define WRITELOG_H

#include <stdio.h>
#include <stdarg.h>

void append_to_log(const char *format, ...);
void initialize_logfile(void);

#endif