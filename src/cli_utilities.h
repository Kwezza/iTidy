/*
 * Amiga CLI Utilities - Header File
 *
 * Provides function declarations for retrieving cursor position
 * and console size in AmigaOS CLI environments.
 *
 * Works on Amiga Workbench 2.04 and later.
 * Compiles with SAS/C.
 */

#ifndef CLI_UTILITIES_H
#define CLI_UTILITIES_H

#include <exec/types.h>

/* Structure to hold cursor position */
typedef struct
{
    int xPos;
    int yPos;
} CursorPos;

/* Structure to hold console size */
typedef struct
{
    int rows;
    int columns;
} ConsoleSize;

/* Simple struct to track paging & indentation state between calls. */
typedef struct
{
    int printedRows; /* how many rows have been printed so far */
    int isIndented;  /* 0 = normal, 1 = inside <p> ... </p> block */
} PrintState;

/* Function prototypes */
CursorPos getCursorPos(void);
ConsoleSize getConsoleSize(void);
void display_help(const char **text);


#endif /* CLI_UTILITIES_H */
