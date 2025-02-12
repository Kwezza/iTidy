#ifndef GET_FONTS_H
#define GET_FONTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <proto/dos.h>
#include "file_directory_handling.h"

#define FONT_BUFFER_SIZE 16   /* Bytes per hex dump line */
#define FONT_FILE_BUFFER 256  /* Buffer for scanning file */

/* New structure to hold a font preference */
typedef struct FontPref {
    char name[32];
    unsigned int size;
} FontPref;


#endif // GET_FONTS_H
