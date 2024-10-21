#ifndef ICON_MISC_H
#define ICON_MISC_H

#include <dos/dos.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include <stdio.h>

#include "main.h"

#define MAX_LEFT_OUT_ICONS 20
#define MAX_PATH_LENGTH 128
#define MAX_DEVICE_NAME_LENGTH 32  // Adjust as needed based on expected device name lengths

extern char left_out_icons[MAX_LEFT_OUT_ICONS][MAX_PATH_LENGTH];

void loadLeftOutIcons(const char *file_path); 
int isIconLeftOut(const char *icon_path);
void dumpLeftOutIcons(void);

#endif // ICON_MISC_H