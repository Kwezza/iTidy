#ifndef ICON_MANAGEMENT_H
#define ICON_MANAGEMENT_H

#include <exec/types.h>
#include <libraries/dos.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <stddef.h>
#include <exec/memory.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "icon_types.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "dos/getDiskDetails.h"

/* Global flag to enable/disable default tool validation */
extern BOOL g_ValidateDefaultTools;

BOOL AddIconToArray(IconArray *iconArray, const FullIconDetails *newIcon);
BOOL checkIconFrame(const char *iconName);
IconArray *CreateIconArray(void);
IconArray *CreateIconArrayFromPath(BPTR lock, const char *dirPath);
void FreeIconArray(IconArray *iconArray);
int ArrangeIcons(BPTR lock, char *dirPath, int newWidth);
int CompareByFolderAndName(const void *a, const void *b);
BOOL IsValidIcon(const char *iconPath);

#endif // ICON_MANAGEMENT_H

