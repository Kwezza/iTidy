/*
 * deficonsdump.c - DefIcons Preferences File Parser and Dumper
 * 
 * This program parses and displays the structure of the DefIcons preferences file.
 * The file contains a hierarchical tree of file type definitions with matching rules.
 * 
 * Format:
 * - Each type entry starts with a null-terminated type name string
 * - Followed by ACT_xxx action bytes that define matching rules
 * - TYPE_DOWN_LEVEL/TYPE_UP_LEVEL control hierarchy
 * - TYPE_END marks end of file
 */

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <stdio.h>
#include <string.h>

// Action codes - how to identify a file type
#define ACT_MATCH            1   // Match bytes at specific offset
#define ACT_SEARCH           2   // Search for byte sequence
#define ACT_SEARCHSKIPSPACES 3   // Search skipping spaces
#define ACT_FILESIZE         4   // Match exact file size
#define ACT_NAMEPATTERN      5   // Match filename pattern
#define ACT_PROTECTION       6   // Match protection bits
#define ACT_OR               7   // Alternative match rule
#define ACT_ISASCII          8   // Check if ASCII text
#define ACT_MACROCLASS       20  // This is a category (has children)
#define ACT_END              0   // End of this type's rules

// Type hierarchy codes
#define TYPE_DOWN_LEVEL      1   // Next type is child of current
#define TYPE_UP_LEVEL        2   // Next type is sibling of parent
#define TYPE_END             0   // End of entire database

// Buffer for reading file
#define BUFFER_SIZE 4096
static UBYTE buffer[BUFFER_SIZE];
static ULONG buffer_pos = 0;
static ULONG buffer_len = 0;
static BPTR file = 0;

// Current position in file
static ULONG file_pos = 0;

// File header size (deficons.prefs has a 32-byte header)
#define HEADER_SIZE 32

// Read a single byte from file
static BOOL read_byte(UBYTE *byte)
{
    if (buffer_pos >= buffer_len)
    {
        // Refill buffer
        buffer_len = Read(file, buffer, BUFFER_SIZE);
        buffer_pos = 0;
        
        if (buffer_len <= 0)
        {
            return FALSE;
        }
    }
    
    *byte = buffer[buffer_pos++];
    file_pos++;
    return TRUE;
}

// Read a null-terminated string
static BOOL read_string(char *str, int max_len)
{
    UBYTE byte;
    int i = 0;
    
    while (i < max_len - 1)
    {
        if (!read_byte(&byte))
        {
            return FALSE;
        }
        
        if (byte == 0)
        {
            str[i] = 0;
            return TRUE;
        }
        
        str[i++] = byte;
    }
    
    str[i] = 0;
    return TRUE;
}

// Print indentation
static void print_indent(int level)
{
    int i;
    for (i = 0; i < level; i++)
    {
        printf("  ");
    }
}

// Print a byte in printable format
static void print_byte_char(UBYTE byte)
{
    if (byte >= 32 && byte <= 126)
    {
        printf("%c", byte);
    }
    else
    {
        printf(".");
    }
}

// Parse and display action codes for a type
static BOOL parse_actions(int indent)
{
    UBYTE action;
    UBYTE arg1, arg2, arg3;
    LONG offset;
    int i;
    char str[256];
    
    while (TRUE)
    {
        if (!read_byte(&action))
        {
            return FALSE;
        }
        
        print_indent(indent);
        
        switch (action)
        {
            case ACT_MATCH:
                // ACT_MATCH: arg1/2 = offset (2 bytes), arg3 = length (signed byte), str = bytes to match
                if (!read_byte(&arg1) || !read_byte(&arg2) || !read_byte(&arg3))
                    return FALSE;
                
                offset = (arg1 << 8) | arg2;
                // Length can be signed (negative means case-insensitive)
                {
                    LONG len = (LONG)(BYTE)arg3;
                    if (len < 0)
                    {
                        printf("MATCH (case insensitive) at offset %ld, length %ld: ", (LONG)offset, -len);
                        len = -len;
                    }
                    else
                    {
                        printf("MATCH at offset %ld, length %ld: ", (LONG)offset, len);
                    }
                    
                    for (i = 0; i < len; i++)
                    {
                        UBYTE byte;
                        if (!read_byte(&byte))
                            return FALSE;
                        printf("%02X ", byte);
                    }
                    printf(" (");
                    // Re-position to print ASCII
                    buffer_pos -= len;
                    file_pos -= len;
                    for (i = 0; i < len; i++)
                    {
                        UBYTE byte;
                        read_byte(&byte);
                        print_byte_char(byte);
                    }
                    printf(")\n");
                }
                break;
            
            case ACT_SEARCH:
                // ACT_SEARCH: arg1 = length (signed byte), str = bytes to search for
                if (!read_byte(&arg1))
                    return FALSE;
                
                // Treat as signed byte
                offset = (LONG)(BYTE)arg1;
                
                if (offset < 0)
                {
                    printf("SEARCH (case insensitive), length %ld: ", -offset);
                    offset = -offset;
                }
                else
                {
                    printf("SEARCH, length %ld: ", offset);
                }
                
                for (i = 0; i < offset; i++)
                {
                    UBYTE byte;
                    if (!read_byte(&byte))
                        return FALSE;
                    printf("%02X ", byte);
                }
                printf(" (");
                buffer_pos -= offset;
                file_pos -= offset;
                for (i = 0; i < offset; i++)
                {
                    UBYTE byte;
                    read_byte(&byte);
                    print_byte_char(byte);
                }
                printf(")\n");
                break;
            
            case ACT_SEARCHSKIPSPACES:
                // ACT_SEARCHSKIPSPACES: arg1 = length (signed byte), str = bytes
                if (!read_byte(&arg1))
                    return FALSE;
                
                // Treat as signed byte
                offset = (LONG)(BYTE)arg1;
                
                if (offset < 0)
                {
                    printf("SEARCH_SKIP_SPACES (case insensitive), length %ld: ", -offset);
                    offset = -offset;
                }
                else
                {
                    printf("SEARCH_SKIP_SPACES, length %ld: ", offset);
                }
                
                for (i = 0; i < offset; i++)
                {
                    UBYTE byte;
                    if (!read_byte(&byte))
                        return FALSE;
                    printf("%02X ", byte);
                }
                printf("\n");
                break;
            
            case ACT_FILESIZE:
                // ACT_FILESIZE: 4 bytes (big endian)
                {
                    ULONG size = 0;
                    for (i = 0; i < 4; i++)
                    {
                        UBYTE byte;
                        if (!read_byte(&byte))
                            return FALSE;
                        size = (size << 8) | byte;
                    }
                    printf("FILESIZE = %lu bytes\n", size);
                }
                break;
            
            case ACT_NAMEPATTERN:
                // ACT_NAMEPATTERN: null-terminated pattern string
                if (!read_string(str, sizeof(str)))
                    return FALSE;
                printf("NAMEPATTERN: '%s'\n", str);
                break;
            
            case ACT_PROTECTION:
                // ACT_PROTECTION: mask (4 bytes) + protbits (4 bytes)
                {
                    ULONG mask = 0, protbits = 0;
                    for (i = 0; i < 4; i++)
                    {
                        UBYTE byte;
                        if (!read_byte(&byte))
                            return FALSE;
                        mask = (mask << 8) | byte;
                    }
                    for (i = 0; i < 4; i++)
                    {
                        UBYTE byte;
                        if (!read_byte(&byte))
                            return FALSE;
                        protbits = (protbits << 8) | byte;
                    }
                    printf("PROTECTION: mask=%08lX, bits=%08lX\n", mask, protbits);
                }
                break;
            
            case ACT_OR:
                printf("OR (alternative rule)\n");
                break;
            
            case ACT_ISASCII:
                printf("IS_ASCII\n");
                break;
            
            case ACT_MACROCLASS:
                printf("MACRO_CLASS (category only, has children)\n");
                break;
            
            case ACT_END:
                return TRUE;
            
            default:
                printf("UNKNOWN ACTION: %lu\n", (ULONG)action);
                return FALSE;
        }
    }
}

// Parse the entire DefIcons database
static BOOL parse_database(void)
{
    int level = 0;
    char type_name[256];
    UBYTE type_code;
    
    printf("\nDefIcons Database Structure:\n");
    printf("=============================\n\n");
    
    while (TRUE)
    {
        // Read type name
        if (!read_string(type_name, sizeof(type_name)))
        {
            printf("Error reading type name at position %lu\n", file_pos);
            return FALSE;
        }
        
        // Check for empty string - this is TYPE_END marker
        if (type_name[0] == 0)
        {
            printf("\n=== End of Database ===\n");
            return TRUE;
        }
        
        // Display type name
        print_indent(level);
        printf("[%s]\n", type_name);
        
        // Parse actions for this type
        if (!parse_actions(level + 1))
        {
            printf("Error parsing actions at position %lu\n", file_pos);
            return FALSE;
        }
        
        // Read next type code (DOWN_LEVEL, UP_LEVEL, or END)
        if (!read_byte(&type_code))
        {
            // End of file is OK
            return TRUE;
        }
        
        switch (type_code)
        {
            case TYPE_DOWN_LEVEL:
                level++;
                if (level > 10)
                {
                    printf("Error: Nesting too deep at position %lu\n", file_pos);
                    return FALSE;
                }
                break;
            
            case TYPE_UP_LEVEL:
                level--;
                if (level < 0)
                {
                    printf("Error: Invalid hierarchy at position %lu\n", file_pos);
                    return FALSE;
                }
                break;
            
            case TYPE_END:
                printf("\n=== End of Database ===\n");
                return TRUE;
            
            default:
                // Not a type code, must be start of new type name
                // Put byte back
                buffer_pos--;
                file_pos--;
                break;
        }
    }
}

int main(void)
{
    BOOL success = FALSE;
    
    printf("DefIcons Preferences File Dumper\n");
    printf("=================================\n\n");
    
    // Open the deficons.prefs file
    file = Open("deficons.prefs", MODE_OLDFILE);
    if (file == 0)
    {
        printf("ERROR: Could not open deficons.prefs\n");
        printf("Make sure this program is run from the same directory as the file.\n");
        return 20;
    }
    
    printf("File opened successfully.\n");
    printf("Parsing database...\n");
    
    // Initialize buffer
    buffer_pos = 0;
    buffer_len = 0;
    file_pos = 0;
    
    // Skip the file header (32 bytes in newer versions)
    {
        UBYTE header[HEADER_SIZE];
        LONG bytes_read = Read(file, header, HEADER_SIZE);
        if (bytes_read != HEADER_SIZE)
        {
            printf("ERROR: Could not read file header (got %ld bytes, expected %d)\n", bytes_read, HEADER_SIZE);
            Close(file);
            return 10;
        }
        file_pos = HEADER_SIZE;
        
        printf("Skipped %d-byte file header.\n", HEADER_SIZE);
        printf("Header: ");
        {
            int i;
            for (i = 0; i < 8; i++)
            {
                printf("%02X ", header[i]);
            }
            printf("...\n");
        }
    }
    
    // Parse the database
    success = parse_database();
    
    if (success)
    {
        printf("\nParsing completed successfully!\n");
        printf("Total bytes read: %lu\n", file_pos);
    }
    else
    {
        printf("\nERROR: Parsing failed at position %lu\n", file_pos);
    }
    
    Close(file);
    return success ? 0 : 10;
}
