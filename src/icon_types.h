#ifndef ICON_TYPES_H
#define ICON_TYPES_H

#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stddef.h>
#include <exec/memory.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "icon_management.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "icon_types.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"

void GetNewIconSizePath(const char *filePath, IconSize *newIconSize);
BOOL GetStandardIconSize(const char *filePath, IconSize *iconSize);
IconPosition GetIconPositionFromPath(const char *iconPath);
BOOL IsNewIconPath(const STRPTR filePath);
int isOS35IconFormat(const char *filename);
BOOL IsNewIcon(struct DiskObject *diskObject);
int getOS35IconSize(const char *filename, IconSize *size);
int isIconTypeDisk(const char *filename,long fib_DirEntryType);
BOOL GetIconSizeFromFile(const char *filePath, IconSize *iconSize);

#endif // ICON_TYPES_H