#include "writeLog.h"

void append_to_log(const char *format, ...)
{
    FILE *logFile;
    logFile = fopen("logfile.txt", "a");  // Open the log file in append mode

    if (logFile != NULL)
    {
        va_list args;
        va_start(args, format);
        vfprintf(logFile, format, args);  // Write formatted output to the log file
        va_end(args);
        fclose(logFile);  // Close the log file
    }
    else
    {
        printf("Error opening log file!\n");
    }
}

void initialize_logfile(void)
{
    FILE *logFile;

    logFile = fopen("logfile.txt", "w");  /* Open the log file in write mode */

    if (logFile != NULL)
    {
        fclose(logFile);  /* Close the log file */
    }
    else
    {
        printf("Error initializing log file!\n");
    }
}