#include <platform/platform.h>
#include <platform/platform_io.h>

#if PLATFORM_AMIGA
#include <platform/amiga_headers.h>
#include <graphics/text.h>
#include <graphics/rastport.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <math.h>
#endif

#include <stddef.h>
#include <string.h>
#include <ctype.h>

/* Console output abstraction - controlled by ENABLE_CONSOLE compile flag */
#include <console_output.h>

#include "itidy_types.h"
#include "writeLog.h"
#include "icon_misc.h"


// Function to read Kickstart version from SysBase for all Kickstart versions
uint16_t GetKickstartVersion(void)
{
#if PLATFORM_AMIGA
    /* Proper way to get Kickstart/ROM version - from SysBase, not low memory */
    if (!SysBase) {
        return 0; /* SysBase not available */
    }
    return SysBase->LibNode.lib_Version;
#else
    /* Host stub - return a default version */
    return 36; /* Simulate Workbench 2.0+ */
#endif
}

// Function to get Workbench version, compatible with older versions
int GetWorkbenchVersion(void)
{
#if PLATFORM_AMIGA
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
#else
    /* Host stub - return a default Workbench version */
    return 40420; /* Simulate Workbench 3.1 */
#endif
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
    result = (char*)whd_malloc(strlen(buffer) + 2); // +2 for the dot and null terminator

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
#if PLATFORM_AMIGA
    // struct TextExtent textExtent;
    uint32_t textLength = strlen(text);

    if (!rastPort)
    {
        CONSOLE_ERROR("RastPort is not initialized.\n");
        return;
    }

    // Calculate text extent
    TextExtent(rastPort, text, textLength, textExtent);
#else
    (void)text;
    (void)textExtent;
#endif
}


int Compare(const void *a, const void *b)
{
    return platform_stricmp(*(const char **)a, *(const char **)b);
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

/* VBCC MIGRATION NOTE (Stage 2): Updated does_file_or_folder_exist()
 * 
 * Changes:
 * - Changed void* lock to BPTR lock (correct AmigaDOS type)
 * - Changed bool return type to BOOL (AmigaDOS consistency)
 * - Changed true/false to TRUE/FALSE (AmigaDOS consistency)
 * - Improved string safety with explicit null termination
 * - Added C99 inline functions where appropriate
 * - Maintained platform abstraction
 */

BOOL does_file_or_folder_exist(const char *filename, int appendWorkingDirectory)
{
#if PLATFORM_AMIGA
    BPTR lock;  // Changed from void* to BPTR (correct AmigaDOS type)
    BOOL exists = FALSE;  // Changed from bool to BOOL
    char currentDir[256];
    char newFilePath[512] = {0};

    // Retrieve current directory
    if (GetCurrentDirName(currentDir, sizeof(currentDir)) == DOSFALSE) {
#ifdef DEBUG
        Printf("Error getting current directory\n");
#endif
    }

    // Construct new file path if required
    if (appendWorkingDirectory == 1) {
        if (!AddPart(currentDir, filename, sizeof(currentDir))) {
#ifdef DEBUG
            Printf("Failed to append filename to current directory\n");
#endif
            return FALSE;  // Changed from false to FALSE
        }
        strncpy(newFilePath, currentDir, sizeof(newFilePath) - 1);
        newFilePath[sizeof(newFilePath) - 1] = '\0';  // Ensure null termination
        
        lock = Lock(newFilePath, ACCESS_READ);

        #ifdef DEBUGLocks
        append_to_log("Locking directory (does_file_or_folder_exist): %s\n", newFilePath);
        #endif

        if (!lock) {
            // No lock means the file doesn't exist or cannot be accessed
        }
    } else {
        strncpy(newFilePath, filename, sizeof(newFilePath) - 1);
        newFilePath[sizeof(newFilePath) - 1] = '\0';  // Ensure null termination
        
        lock = Lock(newFilePath, ACCESS_READ);

        #ifdef DEBUGLocks
        append_to_log("Locking directory (does_file_or_folder_exist): %s\n", newFilePath);
        #endif

        if (!lock) {
            // No lock means the file doesn't exist or cannot be accessed
        }
    }

    // Check if file lock was successful
    if (lock) {
        exists = TRUE;  // Changed from true to TRUE
        #ifdef DEBUGLocks
        append_to_log("Unlocking directory: %s\n", newFilePath);
        #endif
        UnLock(lock);
    }

    return exists;
#else
    // Host implementation using standard C
    FILE *file;
    char fullPath[512];

    (void)appendWorkingDirectory; // Ignore for now on host

    strncpy(fullPath, filename, sizeof(fullPath) - 1);
    fullPath[sizeof(fullPath) - 1] = '\0';

    file = fopen(fullPath, "r");
    if (file) {
        fclose(file);
        return true;  // Host uses standard bool
    }
    return false;  // Host uses standard bool
#endif
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
#if PLATFORM_AMIGA
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
#else
    /* Host implementation - simple getchar */
    getchar();
#endif
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
    const char *pathPtr;
    const char *extPtr;
    char c1, c2;

    // Check if the filePath is shorter than the extension
    if (pathLength < extLength) {
        return false;
    }

    // Set pointers to the end of filePath and extension
    pathPtr = filePath + pathLength - extLength;
    extPtr = extension;

    // Compare each character in the extension with the corresponding character in the filePath
    while (*extPtr) {
        c1 = *pathPtr++;
        c2 = *extPtr++;

        // Convert c1 to lowercase if it's an uppercase letter
        if (c1 >= 'A' && c1 <= 'Z') {
            c1 = c1 + ('a' - 'A');
        }

        // Convert c2 to lowercase if it's an uppercase letter
        if (c2 >= 'A' && c2 <= 'Z') {
            c2 = c2 + ('a' - 'A');
        }

        // Compare the lowercase characters
        if (c1 != c2) {
            return false;
        }
    }

    return true;
}

/**
 * Removes a specified prefix from the start of a string.
 * If the prefix is found at the beginning, the function returns a new string
 * with the prefix removed. Otherwise, it returns a copy of the original string.
 *
 * The returned string is allocated using AllocVec(), so the caller must free it
 * with FreeVec().
 *
 * @param str The original string.
 * @param prefix The prefix to remove.
 * @return A new string with the prefix removed, or a copy of the original if no match.
 */
char *removeTextFromStartOfString(const char *str, const char *prefix) {
    size_t strLen;
    size_t prefixLen;
    char *copy;
    char *newStr;

    /* Validate input */
    if (!str || !prefix) {
        return NULL;
    }

    strLen = strlen(str);
    prefixLen = strlen(prefix);

    /* Check if the prefix matches */
    if (prefixLen > strLen || strncmp(str, prefix, prefixLen) != 0) {
        /* Prefix not found, return a copy of the original string */
        copy = (char *)whd_malloc(strLen + 1);
        if (copy) {
            strcpy(copy, str);
        }
        return copy;
    }

    /* Create new string with prefix removed */
    newStr = (char *)whd_malloc(strLen - prefixLen + 1);
    if (newStr) {
        strcpy(newStr, str + prefixLen);
    }

    return newStr;
}

/**
 * Expand PROGDIR: to absolute path on Amiga
 * Returns TRUE if expansion successful, FALSE otherwise
 */
BOOL ExpandProgDir(const char *path, char *expanded, size_t maxLen) {
#if PLATFORM_AMIGA
    BPTR lock;
    char buffer[512];
    BOOL success = FALSE;
    
    if (!path || !expanded || maxLen == 0) {
        return FALSE;
    }
    
    /* If path doesn't start with PROGDIR:, just copy it */
    if (strncmp(path, "PROGDIR:", 8) != 0) {
        strncpy(expanded, path, maxLen - 1);
        expanded[maxLen - 1] = '\0';
        return TRUE;
    }
    
    /* Lock PROGDIR: to get its absolute path */
    lock = Lock((STRPTR)"PROGDIR:", ACCESS_READ);
    if (lock) {
        /* Get the full path name */
        if (NameFromLock(lock, (STRPTR)buffer, sizeof(buffer))) {
            /* Combine the expanded PROGDIR with the rest of the path */
            const char *remainder = path + 8;  /* Skip "PROGDIR:" */
            if (*remainder == '/' || *remainder == ':') {
                remainder++;  /* Skip separator */
            }
            
            /* Build the full path */
            if (*remainder) {
                snprintf(expanded, maxLen, "%s/%s", buffer, remainder);
            } else {
                strncpy(expanded, buffer, maxLen - 1);
                expanded[maxLen - 1] = '\0';
            }
            success = TRUE;
        }
        UnLock(lock);
    }
    
    if (!success) {
        /* Fallback: just copy the original path */
        strncpy(expanded, path, maxLen - 1);
        expanded[maxLen - 1] = '\0';
    }
    
    return success;
#else
    /* Host platform: just copy the path as-is */
    if (!path || !expanded || maxLen == 0) {
        return FALSE;
    }
    strncpy(expanded, path, maxLen - 1);
    expanded[maxLen - 1] = '\0';
    return TRUE;
#endif
}
/**
 * @brief Check if a path exists and is a directory
 * @param path Directory path to check
 * @return TRUE if exists and is a directory, FALSE otherwise
 */
static BOOL DirectoryExists(const char *path)
{
#if PLATFORM_AMIGA
    BPTR lock;
    struct FileInfoBlock *fib;
    BOOL is_dir = FALSE;
    
    if (!path || path[0] == '\0')
        return FALSE;
    
    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
        return FALSE;
    
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (fib)
    {
        if (Examine(lock, fib))
        {
            is_dir = (fib->fib_DirEntryType > 0);
        }
        FreeDosObject(DOS_FIB, fib);
    }
    
    UnLock(lock);
    return is_dir;
#else
    struct stat st;
    if (!path || path[0] == '\0')
        return FALSE;
    if (stat(path, &st) == 0)
        return (st.st_mode & S_IFDIR) != 0;
    return FALSE;
#endif
}

/**
 * @brief Create directory path recursively, creating all parent directories as needed
 * @param dirpath Directory path to create (must be expanded, no PROGDIR:)
 * @return TRUE if directory exists or was created successfully, FALSE on error
 * 
 * This function creates all necessary parent directories recursively.
 * Example: CreateDirectoryPath("Work:Projects/MyApp/Data") will create
 * Projects, then MyApp, then Data if they don't exist.
 */
BOOL CreateDirectoryPath(const char *dirpath)
{
#if PLATFORM_AMIGA
    char path_copy[512];
    char *last_sep;
    char *colon;
    BPTR lock;
    
    if (!dirpath || dirpath[0] == '\0')
    {
        log_error(LOG_GENERAL, "CreateDirectoryPath: Invalid directory path\n");
        return FALSE;
    }
    
    /* Check if already exists */
    if (DirectoryExists(dirpath))
    {
        log_debug(LOG_GENERAL, "CreateDirectoryPath: Directory already exists: '%s'\n", dirpath);
        return TRUE;
    }
    
    /* Make a copy we can modify */
    strncpy(path_copy, dirpath, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    log_debug(LOG_GENERAL, "CreateDirectoryPath: Creating directory: '%s'\n", path_copy);
    
    /* Find last separator */
    last_sep = strrchr(path_copy, '/');
    if (!last_sep)
        last_sep = strrchr(path_copy, '\\');
    
    /* Find colon (volume separator) */
    colon = strchr(path_copy, ':');
    
    /* If there's a parent path, create it first recursively */
    if (last_sep && last_sep > colon)
    {
        *last_sep = '\0';
        log_debug(LOG_GENERAL, "CreateDirectoryPath: Creating parent first: '%s'\n", path_copy);
        if (!CreateDirectoryPath(path_copy))
        {
            log_error(LOG_GENERAL, "CreateDirectoryPath: Failed to create parent: '%s'\n", path_copy);
            return FALSE;
        }
        *last_sep = '/';
    }
    
    /* Now create this directory */
    log_debug(LOG_GENERAL, "CreateDirectoryPath: Creating directory: '%s'\n", path_copy);
    lock = CreateDir((STRPTR)path_copy);
    if (lock)
    {
        UnLock(lock);
        log_info(LOG_GENERAL, "CreateDirectoryPath: Successfully created: '%s'\n", path_copy);
        return TRUE;
    }
    
    /* CreateDir failed - check if it's because it already exists */
    if (DirectoryExists(path_copy))
    {
        log_debug(LOG_GENERAL, "CreateDirectoryPath: Directory already exists (after CreateDir): '%s'\n", path_copy);
        return TRUE;
    }
    
    log_error(LOG_GENERAL, "CreateDirectoryPath: Failed to create directory: '%s' (IoErr: %ld)\n", 
             path_copy, (long)IoErr());
    return FALSE;
#else
    /* Host platform stub */
    log_debug(LOG_GENERAL, "CreateDirectoryPath: Host platform, assuming success\n");
    return TRUE;
#endif
}

/**
 * @brief Create directory path needed for a file, extracting directory from filepath
 * @param filepath Full file path (e.g., "Work:Dev/App/file.txt" or "PROGDIR:data/config.dat")
 * @return TRUE if directory exists or was created successfully, FALSE on error
 * 
 * This function extracts the directory portion from a file path and creates
 * all necessary parent directories. It automatically expands PROGDIR: assignments.
 * 
 * Examples:
 *   CreateDirectoryForFile("PROGDIR:userdata/Settings/iTidy.prefs")
 *     -> Expands to "Work:iTidy/userdata/Settings/iTidy.prefs"
 *     -> Creates "Work:iTidy/userdata" then "Settings"
 * 
 *   CreateDirectoryForFile("SYS:Prefs/Env-Archive/iTidy.config")
 *     -> Creates "SYS:Prefs/Env-Archive" if needed
 */
BOOL CreateDirectoryForFile(const char *filepath)
{
    char expanded_path[512];
    char dir_path[512];
    char *last_sep;
    char *colon;
    
    if (!filepath || filepath[0] == '\0')
    {
        log_error(LOG_GENERAL, "CreateDirectoryForFile: Invalid filepath\n");
        return FALSE;
    }
    
    log_debug(LOG_GENERAL, "CreateDirectoryForFile: Input filepath: '%s'\n", filepath);
    
    /* Expand PROGDIR: to actual path if needed */
    if (!ExpandProgDir(filepath, expanded_path, sizeof(expanded_path)))
    {
        log_error(LOG_GENERAL, "CreateDirectoryForFile: Failed to expand PROGDIR\n");
        return FALSE;
    }
    
    log_debug(LOG_GENERAL, "CreateDirectoryForFile: Expanded path: '%s'\n", expanded_path);
    
    /* Copy to working buffer */
    strncpy(dir_path, expanded_path, sizeof(dir_path) - 1);
    dir_path[sizeof(dir_path) - 1] = '\0';
    
    /* Find last separator to extract directory path */
    last_sep = strrchr(dir_path, '/');
    if (!last_sep)
        last_sep = strrchr(dir_path, '\\');
    if (!last_sep)
        last_sep = strrchr(dir_path, ':');
    
    /* Check if this is just a volume root or has no directory component */
    colon = strchr(dir_path, ':');
    if (!last_sep || last_sep == colon)
    {
        log_debug(LOG_GENERAL, "CreateDirectoryForFile: No directory component to create\n");
        return TRUE;
    }
    
    /* Terminate at last separator to get directory path */
    *last_sep = '\0';
    
    log_info(LOG_GENERAL, "CreateDirectoryForFile: Directory path to create: '%s'\n", dir_path);
    
    /* Create the directory path */
    return CreateDirectoryPath(dir_path);
}

/*========================================================================*/
/* DumpWorkbenchScreenPalette — Diagnostic Palette Dump                   */
/*========================================================================*/

/**
 * @brief Dump the active Workbench screen palette to the log file.
 *
 * This function retrieves the current Workbench screen and dumps its
 * palette to the general log. Useful for analyzing how icons will render
 * at different color depths (4, 8, 16, 256 colors).
 *
 * Information logged:
 * - Screen dimensions and depth
 * - Number of colors available
 * - Full palette (RGB values in hex format)
 *
 * This helps diagnose icon rendering issues on low-depth screens where
 * icon palettes must be remapped to the limited Workbench palette.
 */
void DumpWorkbenchScreenPalette(void)
{
#if PLATFORM_AMIGA
    struct Screen *wb_screen = NULL;
    struct ViewPort *vp = NULL;
    struct ColorMap *cm = NULL;
    ULONG num_colors;
    ULONG i;
    UBYTE r, g, b;
    
    log_info(LOG_GENERAL, "\n");
    log_info(LOG_GENERAL, "=======================================================\n");
    log_info(LOG_GENERAL, "Workbench Screen Palette Diagnostic Dump\n");
    log_info(LOG_GENERAL, "=======================================================\n");
    
    /* Lock the Workbench screen */
    wb_screen = LockPubScreen("Workbench");
    if (wb_screen == NULL)
    {
        log_error(LOG_GENERAL, "Failed to lock Workbench screen\n");
        return;
    }
    
    /* Get viewport and color map */
    vp = &wb_screen->ViewPort;
    cm = vp->ColorMap;
    
    if (cm == NULL)
    {
        log_error(LOG_GENERAL, "Workbench screen has no ColorMap\n");
        UnlockPubScreen(NULL, wb_screen);
        return;
    }
    
    /* Get screen information */
    num_colors = 1UL << wb_screen->RastPort.BitMap->Depth;
    
    log_info(LOG_GENERAL, "Screen dimensions: %dx%d\n", 
             wb_screen->Width, wb_screen->Height);
    log_info(LOG_GENERAL, "Screen depth: %d bitplanes\n", 
             wb_screen->RastPort.BitMap->Depth);
    log_info(LOG_GENERAL, "Maximum colors: %lu\n", num_colors);
    log_info(LOG_GENERAL, "\n");
    
    /* Dump the palette */
    log_info(LOG_GENERAL, "Workbench Palette (%lu colors):\n", num_colors);
    log_info(LOG_GENERAL, "-------------------------------------------------------\n");
    
    for (i = 0; i < num_colors && i < 256; i++)
    {
        /* Get RGB values using GetRGB4() - returns 12-bit value (4 bits per component)
         * Format: 0x0RGB where each component is 0-15 */
        ULONG rgb4 = GetRGB4(cm, i);
        
        /* Extract 4-bit components and scale to 8-bit (multiply by 17) */
        r = (UBYTE)(((rgb4 >> 8) & 0x0F) * 17);
        g = (UBYTE)(((rgb4 >> 4) & 0x0F) * 17);
        b = (UBYTE)((rgb4 & 0x0F) * 17);
        
        /* Log in compact hex format (similar to icon palette dumps) */
        log_info(LOG_GENERAL, "  [%3lu] = %02X%02X%02X (RGB4: %03lX)\n", i, r, g, b, rgb4);
    }
    
    log_info(LOG_GENERAL, "-------------------------------------------------------\n");
    log_info(LOG_GENERAL, "Palette dump complete\n");
    log_info(LOG_GENERAL, "=======================================================\n");
    log_info(LOG_GENERAL, "\n");
    
    /* Unlock the screen */
    UnlockPubScreen(NULL, wb_screen);
#else
    /* Host platform - log a message */
    log_info(LOG_GENERAL, "DumpWorkbenchScreenPalette: Not available on host platform\n");
#endif
}
