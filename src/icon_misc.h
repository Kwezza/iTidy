#ifndef ICON_MISC_H
#define ICON_MISC_H

#include <dos/dos.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include <stdio.h>

#include "main.h"

void loadLeftOutIcons(const char *file_path); 
void resizeLeftOutIconsArray();
int isIconLeftOut(const char *icon_path);

#endif // ICON_MISC_H