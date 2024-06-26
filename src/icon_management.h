#ifndef ICON_MANAGEMENT_H
#define ICON_MANAGEMENT_H

#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stdlib.h>
#include "main.h"
#include "icon_types.h"
#include "utilities.h"

BOOL AddIconToArray(IconArray *iconArray, const FullIconDetails *newIcon);
BOOL checkIconFrame(const char *iconName);
IconArray *CreateIconArray(void);
IconArray *CreateIconArrayFromPath(BPTR lock, const char *dirPath);
void FreeIconArray(IconArray *iconArray);
int ArrangeIcons(BPTR lock, char *dirPath, int newWidth);
int CompareByFolderAndName(const void *a, const void *b);

#endif // ICON_MANAGEMENT_H
