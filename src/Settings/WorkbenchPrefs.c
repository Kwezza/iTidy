#include "workbenchprefs.h"
#include <exec/memory.h>
#include <libraries/iffparse.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <prefs/workbench.h>
#include <string.h>  // Include for memset

#define PREFS_FILE "ENV:sys/Workbench.prefs"

/* Function to initialize settings with default values */
static void InitializeDefaultSettings(struct WorkbenchSettings *settings) {
    settings->borderless = FALSE;  // Borderless: No
    settings->embossRectangleSize = 3;  // Emboss Rectangle Size: 3
    settings->maxNameLength = 25;  // Max Name Length: 25
    settings->newIconsSupport = TRUE;  // New Icons Support: Yes
    settings->colorIconSupport = TRUE;  // Color Icon Support: Yes
    settings->disableTitleBar = FALSE;  // Disable Title Bar: No
    settings->disableVolumeGauge = FALSE;  // Disable Volume Gauge: No
}

/* Function to read and extract Workbench settings */
static BOOL ReadWorkbenchSettings(BPTR file, struct WorkbenchSettings *settings) {
    BOOL result = FALSE;
    UBYTE buffer[12];
    ULONG chunkType, chunkSize;
    struct WorkbenchPrefs *prefs = NULL;
    struct WorkbenchExtendedPrefs *extPrefs = NULL;
    UBYTE *chunkData;

    /* Initialize with default settings */
    InitializeDefaultSettings(settings);

    /* Check if file is valid before reading */
    if (file == 0) {
        return FALSE;
    }

    /* Read the FORM header (12 bytes: 'FORM', size, 'PREF') */
    if (Read(file, buffer, 12) == 12) {
        chunkType = *((ULONG *)(buffer + 8)); /* Chunk type should be 'PREF' */
        if (chunkType != MAKE_ID('P', 'R', 'E', 'F')) {
            return FALSE;
        }

        while (Read(file, buffer, 8) == 8) {
            /* Read chunk header (8 bytes: ID, size) */
            chunkType = *((ULONG *)buffer);
            chunkSize = *((ULONG *)(buffer + 4));
            chunkSize = (chunkSize + 1) & ~1; /* Ensure even size for padding */

            /* Allocate buffer to read the chunk */
            chunkData = (UBYTE *)AllocMem(chunkSize, MEMF_PUBLIC | MEMF_CLEAR);
            if (chunkData) {
                if (Read(file, chunkData, chunkSize) == chunkSize) {
                    /* Check if this is the WBNC chunk */
                    if (chunkType == MAKE_ID('W', 'B', 'N', 'C')) {
                        prefs = (struct WorkbenchPrefs *)chunkData;

                        settings->borderless = prefs->wbp_Borderless;
                        settings->embossRectangleSize = prefs->wbp_EmbossRect.MaxX;
                        settings->maxNameLength = prefs->wbp_MaxNameLength;
                        settings->newIconsSupport = prefs->wbp_NewIconsSupport;
                        settings->colorIconSupport = prefs->wbp_ColorIconSupport;

                        /* Check if the chunk size is large enough to include extended preferences */
                        if (chunkSize > sizeof(struct WorkbenchPrefs)) {
                            extPrefs = (struct WorkbenchExtendedPrefs *)chunkData;

                            settings->disableTitleBar = extPrefs->wbe_DisableTitleBar;
                            settings->disableVolumeGauge = extPrefs->wbe_DisableVolumeGauge;
                        }

                        result = TRUE;
                        break;
                    }
                }
                FreeMem(chunkData, chunkSize);
            }
        }
    }

    return result;
}

/* Function to fetch Workbench settings */
void fetchWorkbenchSettings(struct WorkbenchSettings *settings) {
    BPTR file;

    /* Clear the structure to ensure no garbage values */
    memset(settings, 0, sizeof(struct WorkbenchSettings));
    file = Open(PREFS_FILE, MODE_OLDFILE);
    if (file) {
        ReadWorkbenchSettings(file, settings);
        Close(file);
    }
}
