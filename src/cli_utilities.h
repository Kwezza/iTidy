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
 typedef struct {
     int xPos;
     int yPos;
 } CursorPos;
 
 /* Structure to hold console size */
 typedef struct {
     int rows;
     int columns;
 } ConsoleSize;
 
 /* Function prototypes */
 CursorPos getCursorPos(void);
 ConsoleSize getConsoleSize(void);
 void display_help(const char **text);
 int print_wrapped_line(const char *line, int width);
 
 #endif /* CLI_UTILITIES_H */
 