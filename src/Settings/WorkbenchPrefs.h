#ifndef WORKBENCH_PREFS_H
#define WORKBENCH_PREFS_H

#include <exec/types.h>
#include "writeLog.h"

/* Define the WorkbenchSettings structure */
struct WorkbenchSettings {
    BOOL borderless;
    LONG embossRectangleSize;
    LONG maxNameLength;
    BOOL newIconsSupport;
    BOOL colorIconSupport;
    BOOL disableTitleBar;
    BOOL disableVolumeGauge;
    UWORD workbenchVersion;      /* Workbench/Kickstart version (e.g., 37, 39, 40, 45) */
    UWORD iconLibraryVersion;    /* icon.library version (e.g., 37, 44, 46) */
};

/* Function to fetch Workbench settings */
void fetchWorkbenchSettings(struct WorkbenchSettings *settings);
//void InitializeDefaultWorkbenchSettings(struct WorkbenchSettings *settings);
void DumpWorkbenchSettings(const struct WorkbenchSettings *settings);
#endif /* WORKBENCH_PREFS_H */