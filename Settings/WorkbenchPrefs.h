#ifndef WORKBENCH_PREFS_H
#define WORKBENCH_PREFS_H

#include <exec/types.h>

/* Define the WorkbenchSettings structure */
struct WorkbenchSettings {
    BOOL borderless;
    LONG embossRectangleSize;
    LONG maxNameLength;
    BOOL newIconsSupport;
    BOOL colorIconSupport;
    BOOL disableTitleBar;
    BOOL disableVolumeGauge;
};

/* Function to fetch Workbench settings */
void fetchWorkbenchSettings(struct WorkbenchSettings *settings);

#endif /* WORKBENCH_PREFS_H */