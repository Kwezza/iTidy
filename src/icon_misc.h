#ifndef ICON_MISC_H
#define ICON_MISC_H

#include <dos/dos.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "icon_management.h"
#include "window_management.h"
#include "file_directory_handling.h"
#include "icon_types.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"

void loadLeftOutIcons(const char *file_path); 
void resizeLeftOutIconsArray();
int isIconLeftOut(const char *icon_path);

#endif // ICON_MISC_H