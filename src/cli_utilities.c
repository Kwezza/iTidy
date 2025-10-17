/*
 * Amiga CLI Utilities - Implementation
 *
 * This file contains functions to retrieve cursor position and console size
 * for AmigaOS CLI environments. It makes use of ANSI escape sequences
 * and works on Amiga Workbench 2.04 and later.
 *
 * VBCC MIGRATION NOTE: Converted from SAS/C to VBCC-compatible code.
 * Uses platform abstraction layer for cross-platform compatibility.
 */

/* VBCC MIGRATION NOTE: Standard C headers first */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* VBCC MIGRATION NOTE: Platform abstraction layer */
#include <platform/platform.h>
#include <platform/amiga_headers.h>

#include "cli_utilities.h"
#include "main.h"

/* VBCC MIGRATION NOTE: Forward declarations for internal functions */
static void wrap_and_page_text_with_markup(const char *text, int indent, int width, int maxRows, int *printedRows);
static int matchTag(const char *text, int i, const char *tag);
static void parse_and_display_line(const char *line, int width, int maxRows, int *printedRows);

/*
 * matchTag:
 *   Checks if text[i..] begins with the given tag (e.g. "<p>" or "</p>").
 *   If it does, returns the length of that tag; otherwise 0.
 *   E.g., matchTag(line, i, "<p>") -> 3 if found, else 0 if not found.
 */
static int matchTag(const char *text, int i, const char *tag)
{
    int len = (int)strlen(tag);
    if (strncmp(text + i, tag, len) == 0)
    {
        return len;
    }
    return 0;
}

/*
 * wrap_and_page_text_with_indent:
 *   Wraps 'text' so that each line does not exceed 'width' columns.
 *   Each new line starts at 'indent' columns from the left.
 *   'printedRows' tracks how many lines have been printed so far (for paging).
 *   If we reach 'maxRows - 1', we pause so the user can read.
 */

/*
 * wrap_and_page_text_with_markup:
 *   Wraps 'text' so that each line does not exceed 'width' columns.
 *   Each new line starts at 'indent' columns from the left.
 *   We also parse inline <b>...</b> tags, converting them to ANSI bold on/off.
 *   'printedRows' tracks how many lines we've printed (for paging).
 *   If we reach 'maxRows - 1', we pause so the user can read.
 */
static void wrap_and_page_text_with_markup(const char *text,
                                           int indent,
                                           int width,
                                           int maxRows,
                                           int *printedRows)
{
    int i = 0;
    int start = 0;
    int lastSpace = -1;
    int currentLength = 0;
    int textLen = (int)strlen(text);
    char c;

    /* We are about to print a new line, so let's insert indentation first. */
    int needIndent = 1;

    while (i < textLen)
    {
        printf("i=%d", i);  // Debug print
        /* 1) Check for markup tags (<b>, </b>) before printing a character. */
        if (strncmp(text + i, "<b>", 3) == 0)
        {
            //printf("i=%d\n", i);
            /* Print bold on: "\x1B[1m" */
            fputs("\x1B[1m", stdout);
            i = i + 3; /* Skip "<b>" */
            /* No visible characters added, so do NOT increment currentLength. */
            printf(" bold on ");
            printf("new i=%d\n", i);
            continue;

        }
        if (strncmp(text + i, "</b>", 4) == 0)
        {
            printf("i=%d\n", i);
            /* Print bold off: "\x1B[0m" */
            fputs("\x1B[0m", stdout);
            i = i + 4; /* Skip "</b>" */
            /* No visible characters added, so do NOT increment currentLength. */
            printf(" bold off ");
            printf("new i=%d\n", i);
            continue;

        }

        /* 2) If we're at the start of a line, print indentation. */
        if (needIndent)
        {
            int sp;
            for (sp = 0; sp < indent; sp++)
            {
                putchar(' ');
            }
            currentLength = indent; /* We have 'indent' chars on this line already. */
            needIndent = 0;
        }

        {
            c = text[i];
            printf("c=%c;", c);  // Debug print

            /* Track whitespace for word breaks. */
            if (isspace((unsigned char)c))
            {
                lastSpace = i;
            }

            /* 3) Check if adding this character would exceed width. */
            if (currentLength + 1 > width)
            {
                /* We must wrap here. If we have a lastSpace, break there. */
                if (lastSpace >= start)
                {
                    int j;
                    /* Print from 'start' up to (not including) lastSpace. */
                    for (j = start; j < lastSpace; j++)
                    {
                        putchar(text[j]);
                    }
                    putchar('\n');
                    (*printedRows)++;

                    /* Paging check. */
                    if (*printedRows >= maxRows - 1)
                    {
                        printf("Press any key to continue...");
                        fflush(stdout);
                        /* Wait for user input (implementation up to you). */
                        getchar(); /* or WaitChar() on Amiga, etc. */
                        /* Clear the prompt line: */
                        printf("\r                           \r");
                        *printedRows = 0;
                    }

                    /* Move i to the character after lastSpace. */
                    i = lastSpace + 1;
                    /* Skip any extra spaces. */
                    while (isspace((unsigned char)text[i]))
                    {
                        i++;
                    }
                }
                else
                {
                    /* No space found, forcibly break at current char. */
                    int j;
                    for (j = start; j < i; j++)
                    {
                        putchar(text[j]);
                    }
                    putchar('\n');
                    (*printedRows)++;

                    if (*printedRows >= maxRows - 1)
                    {
                        printf("Press any key to continue...");
                        fflush(stdout);
                        getchar();
                        printf("\r                           \r");
                        *printedRows = 0;
                    }
                }
                /* Start a new line (with indentation). */
                currentLength = 0;
                lastSpace = -1;
                start = i;
                needIndent = 1;
            }
            else
            {
                /* 4) Safe to “use up” this character. */
                currentLength++;
                i++;
            }
        }
    }

    /* 5) Print any leftover part if currentLength > 0. */
    if (currentLength > 0)
    {
        int j;
        printf("[ Print left over ]");
        for (j = start; j < i; j++)
        {
            putchar(text[j]);
        }
        putchar('\n');
        (*printedRows)++;

        if (*printedRows >= maxRows - 1)
        {
            printf("Press any key to continue...");
            fflush(stdout);
            getchar();
            printf("\r                           \r");
            *printedRows = 0;
        }
    }
    /* Turn off bold at the end if needed, or if you want to ensure normal text after. */
    fputs("\x1B[0m", stdout);
}

static void parse_and_display_line(const char *line,
                                   int width,
                                   int maxRows,
                                   int *printedRows)
{
    const char *pStart = strstr(line, "<p>");
    const char *pEnd = strstr(line, "</p>");

    if (pStart != NULL && pEnd != NULL && pEnd > pStart)
    {
        /* Command is everything before <p> */
        /* Description is everything between <p> and </p> */

        /* Copy out the command portion */
        int cmdLen = (int)(pStart - line);
        char command[256];      /* Adjust size as needed */
        char description[1024]; /* Adjust as needed */

        if (cmdLen > (int)sizeof(command) - 1)
        {
            cmdLen = (int)sizeof(command) - 1;
        }
        strncpy(command, line, cmdLen);
        command[cmdLen] = '\0';

        /* Copy out the description portion */
        {
            const char *descStart = pStart + 3; /* skip "<p>" */
            int descLen = (int)(pEnd - descStart);
            if (descLen > (int)sizeof(description) - 1)
            {
                descLen = (int)sizeof(description) - 1;
            }
            strncpy(description, descStart, descLen);
            description[descLen] = '\0';
        }

        /* 1) Print the command (indent=0). */
        printf(" [Step 1] ");
        wrap_and_page_text_with_markup(command, 0, width, maxRows, printedRows);
        printf(" [Step 2] ");
        /* 2) Print the description (indent=15). */
        wrap_and_page_text_with_markup(description, 15, width, maxRows, printedRows);
        printf(" [Step 3] ");
        /* If there’s anything after </p>, you could parse it or ignore it. */
    }
    else
    {
        printf(" [no p found] ");
        /* No <p> or </p> found, just print the whole line flush-left. */
        wrap_and_page_text_with_markup(line, 0, width, maxRows, printedRows);
    }
}

void display_help(const char **helpText)
{
    ConsoleSize cs = getConsoleSize();
    int printedRows = 0;
    int i = 0;

    while (helpText[i] != NULL)
    {
        parse_and_display_line(helpText[i],
                               cs.columns, /* console width */
                               cs.rows,    /* console height */
                               &printedRows);
        i++;
    }
}

/* Function to retrieve the cursor position in the Amiga CLI */
/* VBCC MIGRATION NOTE: Amiga-specific DOS I/O wrapped in platform guard */
CursorPos getCursorPos(void)
{
#if PLATFORM_AMIGA
    BPTR fh;
    CursorPos pos;
    char buffer[32];
    int i;
    char ch;
    LONG bytesRead;
    
    /* VBCC MIGRATION NOTE: Initialize structure members explicitly for C89 */
    pos.xPos = -1;
    pos.yPos = -1;
    i = 0;

    /* Open the console for direct communication */
    fh = Open("CONSOLE:", MODE_OLDFILE);
    if (fh == 0)
    {
        return pos;
    }

    SetMode(fh, 1);

    /* Send the cursor position request sequence */
    if (Write(fh, "\x9b"
                  "6n",
              3) != 3)
    {
        SetMode(fh, 0);
        Close(fh);
        return pos;
    }

    /* Small delay to ensure the response is available */
    Delay(10);

    /* Read the response character by character */
    while (i < (int)(sizeof(buffer) - 1))
    {
        bytesRead = Read(fh, &ch, 1);
        if (bytesRead <= 0)
        {
            break;
        }
        buffer[i++] = ch;
        if (ch == 'R')
        {
            break;
        }
    }
    buffer[i] = '\0';

    SetMode(fh, 0);
    Close(fh);

    /* Parse the response string for cursor position */
    if (sscanf(buffer, "\x9b%d;%dR", &pos.yPos, &pos.xPos) != 2)
    {
        pos.xPos = -1;
        pos.yPos = -1;
    }

    return pos;
#else
    CursorPos pos;
    pos.xPos = -1;
    pos.yPos = -1;
    return pos;
#endif
}

/* Function to retrieve the console window size */
/* VBCC MIGRATION NOTE: Amiga-specific DOS I/O wrapped in platform guard */
ConsoleSize getConsoleSize(void)
{
#if PLATFORM_AMIGA
    BPTR fh;
    ConsoleSize size;
    char buffer[64];
    int i;
    char ch;
    LONG bytesRead;
    
    /* VBCC MIGRATION NOTE: Initialize structure members explicitly for C89 */
    size.rows = 24;
    size.columns = 80;
    i = 0;

    /* Open the console for reading and writing */
    fh = Open("CONSOLE:", MODE_OLDFILE);
    if (fh == 0)
    {
        return size;
    }

    SetMode(fh, 1);

    /* Send the Window Status Request command */
    if (Write(fh, "\x9b"
                  "0 q",
              4) != 4)
    {
        SetMode(fh, 0);
        Close(fh);
        return size;
    }

    Delay(5);

    /* Read the response from the console */
    while (i < (int)(sizeof(buffer) - 1))
    {
        bytesRead = Read(fh, &ch, 1);
        if (bytesRead <= 0 || ch == 'r')
        {
            break;
        }
        buffer[i++] = ch;
    }
    buffer[i] = '\0';

    SetMode(fh, 0);
    Close(fh);

    /* Parse the response string for console size */
    if (sscanf(buffer, "\x9b"
                       "1;1;%d;%d r",
               &size.rows, &size.columns) != 2)
    {
        size.rows = 24;
        size.columns = 80;
    }

    return size;
#else
    ConsoleSize size;
    size.rows = 24;
    size.columns = 80;
    return size;
#endif
}
