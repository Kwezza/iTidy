#include <graphics/text.h>
#include <exec/types.h>
#include <graphics/rastport.h>
#include <stddef.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <math.h>

#include "main.h"


// Function to read Kickstart version from memory for Kickstart 1.3 and earlier
UWORD GetKickstartVersion() {
    return *((volatile UWORD*)0x00FC);
}

// Function to get Workbench version, compatible with older versions
int GetWorkbenchVersion(void) {
    struct Library *DOSBase;
    int WBversion;
    int libVersion;
    int libRevision; 
    // Check Kickstart version to determine if the system is pre-2.0
    UWORD kickstartVersion = GetKickstartVersion();

    // If the kickstart version is less than 36 and not an obviously incorrect value (like 0 or random low number)
    if (kickstartVersion > 0 && kickstartVersion < 36) {
        return 1000; // Assume Workbench 1.000 for Kickstart versions below 36
    }

    // Open the DOS library for Workbench 2.0 and higher
    DOSBase = OpenLibrary("dos.library", 0);
    if (!DOSBase) {
        return -1000; // Failed to open dos.library
    }

    // Ensure SysBase is defined and accessible
    if (!SysBase) {
        CloseLibrary(DOSBase);
        return -1000;
    }

    // Get the system version from SysBase
    libVersion = SysBase->LibNode.lib_Version;
    libRevision = SysBase->LibNode.lib_Revision;

    // Combine version and revision into a single integer
    // Ensuring revision is always treated as a three-digit number
    WBversion = libVersion * 1000 + libRevision * 10;

    // Close the DOS library
    CloseLibrary(DOSBase);

    return WBversion;
}

void LookupWorkbenchVersion(int revision, char *versionString) {
    // Ensure versionString is valid
    if (!versionString) {
        return;
    }

    // Clear the versionString
    versionString[0] = '\0';

    // Map the revision to the Workbench version
    switch (revision) {
        case 30000:
            strcpy(versionString, "1.0");
            break;
        case 31334:
            strcpy(versionString, "1.1");
            break;
        case 33460:
        case 33470:
        case 33560:
        case 33590:
        case 33610:
            strcpy(versionString, "1.2");
            break;
        case 34200:
        case 34280:
        case 34340:
        case 34100: // A2024 specific
            strcpy(versionString, "1.3");
            break;
        case 36680:
            strcpy(versionString, "2.0");
            break;
        case 37670:
            strcpy(versionString, "2.04");
            break;
        case 37710:
        case 37720:
            strcpy(versionString, "2.05");
            break;
        case 38360:
            strcpy(versionString, "2.1");
            break;
        case 39290:
            strcpy(versionString, "3.0");
            break;
        case 40420:
            strcpy(versionString, "3.1");
            break;
        case 44200:
        case 44400:
        case 44500:
            strcpy(versionString, "3.5");
            break;
        case 45100:
        case 45200:
        case 45300:
            strcpy(versionString, "3.9");
            break;
        case 45194:
            strcpy(versionString, "3.1.4");
            break;
        case 47100:
            strcpy(versionString, "3.2");
            break;
        case 47200:
            strcpy(versionString, "3.2.1");
            break;
        case 47300:
            strcpy(versionString, "3.2.2");
            break;
        case 47400:
            strcpy(versionString, "3.2.2.1");
            break;
        case 50000:
            strcpy(versionString, "4.0");
            break;
        case 51000:
            strcpy(versionString, "4.1");
            break;
        case 51100:
            strcpy(versionString, "4.1 Update 1");
            break;
        case 51200:
            strcpy(versionString, "4.1 Update 2");
            break;
        case 51300:
            strcpy(versionString, "4.1 Update 3");
            break;
        case 51400:
            strcpy(versionString, "4.1 Update 4");
            break;
        case 51500:
            strcpy(versionString, "4.1 Update 5");
            break;
        case 51600:
            strcpy(versionString, "4.1 Final Edition");
            break;
        default:
            sprintf(versionString, "Unknown Workbench version %d", revision);
            break;
    }
}
char* convertWBVersionWithDot(int number) {
    char buffer[16]; // Buffer to hold the number as a string
    char *result; // Pointer to the result string

    // Convert the integer to a string
    sprintf(buffer, "%d", number);

    // Allocate memory for the result string
    result = (char*)malloc(strlen(buffer) + 2); // +2 for the dot and null terminator

    // Check if memory allocation was successful
    if (result == NULL) {
        return NULL;
    }

    // Insert the dot after the first two digits
    strncpy(result, buffer, 2);
    result[2] = '.'; 
    strcpy(result + 3, buffer + 2);

    return result;
}


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
    return stricmp(*(const char **)a, *(const char **)b);
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

int endsWithInfo(const char *filePath) {
    const char *extension = ".info";
    size_t pathLength = strlen(filePath);
    size_t extLength = strlen(extension);
    size_t i;

    // Check if the filePath is shorter than the extension
    if (pathLength < extLength) {
        return FALSE;
    }

    // Compare the end of the filePath with ".info" (case-insensitive)
    for (i = 0; i < extLength; i++) {
        if (tolower(filePath[pathLength - extLength + i]) != tolower(extension[i])) {
            return FALSE;
        }
    }

    return TRUE;
}