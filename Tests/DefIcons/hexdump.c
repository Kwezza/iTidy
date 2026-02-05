/*
 * hexdump.c - Simple hex dump utility for Amiga
 * Dumps the contents of deficons.prefs file in hex format
 */

#include <proto/dos.h>
#include <dos/dos.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 16

int main(void)
{
    BPTR file;
    UBYTE buffer[BUFFER_SIZE];
    LONG bytes_read;
    ULONG offset = 0;
    int i;
    
    printf("Hex dump of deficons.prefs\n");
    printf("==========================\n\n");
    
    // Open the file
    file = Open("deficons.prefs", MODE_OLDFILE);
    if (file == 0)
    {
        printf("ERROR: Could not open deficons.prefs\n");
        return 20;
    }
    
    // Read and display file contents
    while ((bytes_read = Read(file, buffer, BUFFER_SIZE)) > 0)
    {
        // Print offset
        printf("%08lx: ", offset);
        
        // Print hex values
        for (i = 0; i < BUFFER_SIZE; i++)
        {
            if (i < bytes_read)
            {
                printf("%02x ", (unsigned int)buffer[i]);
            }
            else
            {
                printf("   ");
            }
            
            // Add extra space in the middle
            if (i == 7)
            {
                printf(" ");
            }
        }
        
        // Print ASCII representation
        printf(" |");
        for (i = 0; i < bytes_read; i++)
        {
            if (buffer[i] >= 32 && buffer[i] <= 126)
            {
                printf("%c", buffer[i]);
            }
            else
            {
                printf(".");
            }
        }
        printf("|\n");
        
        offset += bytes_read;
    }
    
    printf("\nTotal bytes: %lu\n", offset);
    
    Close(file);
    return 0;
}
