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

BOOL AddIconToArray(IconArray *iconArray, const FullIconDetails *newIcon);
BOOL GetStandardIconSize(const char *filePath, IconSize *iconSize);
BOOL IsNewIcon(struct DiskObject *diskObject);
BOOL IsNewIconPath(const STRPTR filePath);
IconArray *CreateIconArray(void);
IconArray *CreateIconArrayFromPath(BPTR lock, const char *dirPath);
IconPosition GetIconPositionFromPath(const char *iconPath);
int getOS35IconSize(const char *filename, IconSize *size);
int isOS35IconFormat(const char *filename);
void FreeIconArray(IconArray *iconArray);
void GetNewIconSizePath(const char *filePath, IconSize *newIconSize);
int ArrangeIcons(BPTR lock, char *dirPath, int newWidth);
int CompareByFolderAndName(const void *a, const void *b);

#endif // ICON_MANAGEMENT_H
