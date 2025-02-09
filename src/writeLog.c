#include "writeLog.h"

void getTimestamp(char *buffer, int bufferSize)
{
    time_t rawtime;
    struct tm *timeinfo;  // Declare struct at the top (SAS/C requirement)

    time(&rawtime);  // Get current time
    timeinfo = localtime(&rawtime);  // Convert to local time

    if (timeinfo)
    {
        sprintf(buffer, "[%02d:%02d:%02d]", 
                timeinfo->tm_hour, 
                timeinfo->tm_min, 
                timeinfo->tm_sec);
    }
    else
    {
        strcpy(buffer, "[00:00:00]");  // Use strcpy for fixed fallback
    }
}

void append_to_log(const char *format, ...)
{
    FILE *logFile;
    char timestamp[10];  // Buffer for [HH:MM:SS]
    va_list args;

    getTimestamp(timestamp, sizeof(timestamp));  // Get timestamp

    logFile = fopen("logfile.txt", "a");  // Open log file in append mode
    if (logFile != NULL)
    {
        fprintf(logFile, "%s ", timestamp);  // Write timestamp first

        va_start(args, format);
        vfprintf(logFile, format, args);  // Write formatted log message
        va_end(args);

        fclose(logFile);
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