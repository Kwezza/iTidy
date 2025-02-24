#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <proto/dos.h>
#include "get_fonts.h"
#include "main.h"
//#include "file_directory_handling.h"
FontPref *fontPrefs = NULL;

FontPref *getIconFont(void) {

    FontPref *ret;
    //set to default topaz.font 8pt first just in case no font preferences are found
    ret=defaultFont();
    // workbench 3.x font prefs
    if (does_file_or_folder_exist("ENV:sys/font.prefs", 0))
        ret = extractFonts("ENV:sys/font.prefs");
    // workbench 2.x font prefs
    else if (does_file_or_folder_exist("ENV:sys/wbfont.prefs", 0))
        ret = extractFonts("ENV:sys/wbfont.prefs");
    #ifdef DEBUG
    append_to_log("Workbench Icon Font (getIconFont): %s, Size: %d\n", ret->name, ret->size);
    #endif
    return ret;
}


/* Helper function to return the default font.
   Returns a newly allocated FontPref with name "topaz.font" and size 8. */
FontPref *defaultFont(void) {
    FontPref *ret;
    ret = (FontPref *)malloc(sizeof(FontPref));
    if (ret != NULL) {
        /* Copy default font name into ret->name */
        strncpy(ret->name, "topaz.font", sizeof(ret->name));
        ret->name[sizeof(ret->name) - 1] = '\0';
        ret->size = 8;
    }
    return ret;
}

/* Updated extractFonts function that returns a pointer to a FontPref structure.
   If any problems occur (file not found, or no FONT section found) then it returns
   a default font ("topaz.font" at 8 pt). Otherwise, it returns the first FONT section
   found in the file. */
FontPref *extractFonts(const char *filename) {
    BPTR file;
    UBYTE buffer[FONT_FILE_BUFFER];
    LONG bytesRead, i, offset = 0;
    unsigned int fontSize;
    char fontName[32];
    int j;
    FontPref *selectedFont = NULL;

    /* Variables for determining which method to use */
    const char *baseName;
    int useOldMethod;
    int nameOffset;
    const char *p;

    /* Extract the base name from the given filename (ignoring any path components) */
    baseName = filename;
    for (p = filename; *p; p++) {
        if (*p == '/' || *p == ':')
            baseName = p + 1;
    }
    useOldMethod = (strcmp(baseName, "font.prefs") == 0);

    file = Open(filename, MODE_OLDFILE);
    if (!file) {
        /* File open error; return default font */
        return defaultFont();
    }
    
    while ((bytesRead = Read(file, buffer, FONT_FILE_BUFFER)) > 0 && selectedFont == NULL) {
        for (i = 0; i < bytesRead - 40 && selectedFont == NULL; i++) {
            if (buffer[i] == 'F' && buffer[i+1] == 'O' &&
                buffer[i+2] == 'N' && buffer[i+3] == 'T' &&
                buffer[i+4] == 0x00 && buffer[i+5] == 0x00) {

                if (useOldMethod) {
                    /* Workbench 3.x files: two-byte size in little-endian order */
                    fontSize = buffer[i+33] | (buffer[i+34] << 8);
                } else {
                    /* Workbench 2.x files: size stored as an 8.8 fixed-point big-endian number */
                    unsigned short fixedSize;
                    fixedSize = (buffer[i+33] << 8) | buffer[i+34];
                    fontSize = fixedSize / 256;  /* Convert fixed-point to integer pt size */
                }

                /* The font name starts 36 bytes after the marker */
                nameOffset = i + 36;
                for (j = 0; j < (int)(sizeof(fontName) - 1) && buffer[nameOffset+j] != 0; j++) {
                    fontName[j] = buffer[nameOffset+j];
                }
                fontName[j] = '\0';

                selectedFont = (FontPref *)malloc(sizeof(FontPref));
                if (selectedFont != NULL) {
                    strncpy(selectedFont->name, fontName, sizeof(selectedFont->name));
                    selectedFont->name[sizeof(selectedFont->name) - 1] = '\0';
                    selectedFont->size = fontSize;
                }
            }
        }
        offset += bytesRead;
    }
    Close(file);

    if (selectedFont == NULL) {
        /* No FONT section was found in the file; return the default font. */
        return defaultFont();
    }

    #ifdef DEBUG
        append_to_log("Found font: %s, size: %d\n", selectedFont->name, selectedFont->size);
    #endif

    return selectedFont;
}

