#include <exec/types.h>
#include <graphics/rastport.h>
#include <stddef.h>
#include "main.h"


void CalculateTextExtent(const char *text, struct TextExtent *textExtent)
{
    // struct TextExtent textExtent;
    ULONG textLength = strlen(text);

    if (!rastPort)
    {
        printf("RastPort is not initialized.\n");
        return;
    }

    // Calculate text extent
    TextExtent(rastPort, text, textLength, textExtent);
}


int Compare(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

int strncasecmp_custom(const char *s1, const char *s2, size_t n)
{
    while (n-- > 0 && *s1 != '\0' && *s2 != '\0')
    {
        char c1 = tolower((unsigned char)*s1);
        char c2 = tolower((unsigned char)*s2);

        if (c1 != c2)
        {
            return c1 - c2;
        }

        s1++;
        s2++;
    }

    return 0;
}

BOOL does_file_or_folder_exist(const char *filename, int appendWorkingDirectory)
{
    BPTR lock;
    BOOL exists = FALSE;
    LONG errorCode = 0;
    char currentDir[256];
    char newFilePath[512] = {0}; /* Ensure buffer is initially empty */
    /* Retrieve current directory */
    if (GetCurrentDirName(currentDir, sizeof(currentDir)) == DOSFALSE)
    {
        printf("Error getting current directory\n");
    }

    /* Construct new file path if required */
    if (appendWorkingDirectory == 1)
    {
        if (!AddPart(currentDir, filename, sizeof(currentDir)))
        {
            printf("Failed to append filename to current directory\n");
            return FALSE;
        }
        strncpy(newFilePath, currentDir, sizeof(newFilePath) - 1);
        lock = Lock(newFilePath, ACCESS_READ);
        if (lock == NULL)
            errorCode = IoErr();
    }
    else
    {
        strncpy(newFilePath, filename, sizeof(newFilePath) - 1);
        lock = Lock(filename, ACCESS_READ);
        if (lock == NULL)
            errorCode = IoErr();
    }

    /* Check if file lock was successful */
    if (lock)
    {
        exists = TRUE;
        UnLock(lock);
    }

    return exists;
}
void trim(char *str)
{
    char *start, *end;

    /* Trim leading space */
    for (start = str; *start != '\0' && isspace((unsigned char)*start); start++)
    {
        /* Intentionally left blank */
    }

    /* All spaces? */
    if (*start == '\0')
    {
        *str = '\0';
        return;
    }

    /* Trim trailing space */
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end))
    {
        end--;
    }

    /* Write new null terminator */
    *(end + 1) = '\0';

    /* Move trimmed string */
    memmove(str, start, end - start + 2);
}

void WaitChar(void)
{   
//getchar();

    /* The BPTR to the input file handle */
    BPTR inputHandle;
    struct FileHandle *fh;
    UBYTE ch;

    /* Get the input handle for the current process */
    inputHandle = Input();
    fh = (struct FileHandle *)(BADDR(inputHandle));

    /* Set the console to raw mode */
    SetMode(inputHandle, 1);

    /* Wait for a single character */
    Read(inputHandle, &ch, 1);

    /* Restore the console to normal mode */
    SetMode(inputHandle, 0);
}

void remove_CR_LF_from_string(char *str)
{
    char *src = str;
    char *dst = str;

    while (*src)
    {
        if (*src != '\r' && *src != '\n')
        {
            *dst++ = *src;
        }
        src++;
    }

    *dst = '\0'; // Null-terminate the modified string
}