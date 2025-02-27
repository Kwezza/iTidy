/*
 * Amiga CLI Utilities - Implementation
 *
 * This file contains functions to retrieve cursor position and console size
 * for AmigaOS CLI environments. It makes use of ANSI escape sequences
 * and works on Amiga Workbench 2.04 and later.
 *
 * Compiles with SAS/C.
 */

 #include "cli_utilities.h"
 #include <exec/types.h>
 #include <dos/dos.h>
 #include <proto/dos.h>
 #include <stdio.h>

 #include "main.h"
 
 /* Function to retrieve the cursor position in the Amiga CLI */
 CursorPos getCursorPos(void) {
     BPTR fh;
     CursorPos pos = { -1, -1 };
     char buffer[32];
     int i = 0;
     char ch;
     LONG bytesRead;
 
     /* Open the console for direct communication */
     fh = Open("CONSOLE:", MODE_OLDFILE);
     if (fh == 0) {
         return pos;
     }
 
     SetMode(fh, 1);
 
     /* Send the cursor position request sequence */
     if (Write(fh, "\x9b" "6n", 3) != 3) {
         SetMode(fh, 0);
         Close(fh);
         return pos;
     }
 
     /* Small delay to ensure the response is available */
     Delay(10);
 
     /* Read the response character by character */
     while (i < (int)(sizeof(buffer) - 1)) {
         bytesRead = Read(fh, &ch, 1);
         if (bytesRead <= 0) {
             break;
         }
         buffer[i++] = ch;
         if (ch == 'R') {
             break;
         }
     }
     buffer[i] = '\0';
 
     SetMode(fh, 0);
     Close(fh);
 
     /* Parse the response string for cursor position */
     if (sscanf(buffer, "\x9b%d;%dR", &pos.yPos, &pos.xPos) != 2) {
         pos.xPos = -1;
         pos.yPos = -1;
     }
 
     return pos;
 }
 
 /* Function to retrieve the console window size */
 ConsoleSize getConsoleSize(void) {
     BPTR fh;
     ConsoleSize size = { 24, 80 };
     char buffer[64];
     int i = 0;
     char ch;
     LONG bytesRead;
 
     /* Open the console for reading and writing */
     fh = Open("CONSOLE:", MODE_OLDFILE);
     if (fh == 0) {
         return size;
     }
 
     SetMode(fh, 1);
 
     /* Send the Window Status Request command */
     if (Write(fh, "\x9b" "0 q", 4) != 4) {
         SetMode(fh, 0);
         Close(fh);
         return size;
     }
 
     Delay(5);
 
     /* Read the response from the console */
     while (i < (int)(sizeof(buffer) - 1)) {
         bytesRead = Read(fh, &ch, 1);
         if (bytesRead <= 0 || ch == 'r') {
             break;
         }
         buffer[i++] = ch;
     }
     buffer[i] = '\0';
 
     SetMode(fh, 0);
     Close(fh);
 
     /* Parse the response string for console size */
     if (sscanf(buffer, "\x9b" "1;1;%d;%d r", &size.rows, &size.columns) != 2) {
         size.rows = 24;
         size.columns = 80;
     }
 
     return size;
 }
 
 static void wrap_and_page_line(const char *line, int width, int *printedRows, int maxRows)
{
    int currentLength = 0;
    int start = 0;
    int lastSpace = -1;
    int i = 0;
    char c;

    while ((c = line[i]) != '\0') {
        if (isspace((unsigned char)c)) {
            lastSpace = i;
        }
        /* Check if adding this char would exceed width */
        if (currentLength + 1 > width) {
            /* We must wrap here */
            if (lastSpace >= start) {
                /* Print from start up to (not including) lastSpace */
                int j;
                for (j = start; j < lastSpace; j++) {
                    putchar(line[j]);
                }
            } else {
                /* No space found, force break at current char */
                int j;
                for (j = start; j < i; j++) {
                    putchar(line[j]);
                }
                /* i not incremented; we re-check the same char next time */
            }
            putchar('\n');
            (*printedRows)++;

            /* Pause if we’ve filled the console */
            if (*printedRows >= maxRows - 1) {
                printf( textReset textReverse textItalic "Press any key to continue..." textReset);
                fflush(stdout);
                WaitChar();
                printf("\r                           \r"); /* clear prompt */
                *printedRows = 0;
            }

            /* Move i to the next segment start */
            if (lastSpace >= start) {
                i = lastSpace + 1;
                /* Skip spaces */
                while (isspace((unsigned char)line[i])) {
                    i++;
                }
            }
            start = i;
            currentLength = 0;
            lastSpace = -1;
        } else {
            /* We haven't exceeded width yet, so consume this char */
            currentLength++;
            i++;
        }
    }

    /* Print any leftover part if currentLength > 0 */
    if (currentLength > 0) {
        int j;
        for (j = start; j < i; j++) {
            putchar(line[j]);
        }
        putchar('\n');
        (*printedRows)++;

        /* Check if we need to pause here */
        if (*printedRows >= maxRows - 1) {
            printf( textReset textReverse textItalic "Press any key to continue..." textReset);
            fflush(stdout);
            WaitChar();
            printf("\r                           \r");
            *printedRows = 0;
        }
    }
}

/* display_help: wraps each line and pages the output as needed */
void display_help(const char **text)
{
    ConsoleSize cs = getConsoleSize();
    int printedRows = 0;
    int i = 0;

    while (text[i] != NULL) {
        wrap_and_page_line(text[i], cs.columns, &printedRows, cs.rows);
        i++;
    }
}
