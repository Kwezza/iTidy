#ifndef ICON_MISC_H
#define ICON_MISC_H

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include <stdio.h>


#include "dos/getDiskDetails.h"

#define MAX_LEFT_OUT_ICONS 20
#define MAX_PATH_LENGTH 128
#define MAX_DEVICE_NAME_LENGTH 32  // Adjust as needed based on expected device name lengths

extern char left_out_icons[MAX_LEFT_OUT_ICONS][MAX_PATH_LENGTH];

void loadLeftOutIcons(const char *file_path); 
int isIconLeftOut(const char *icon_path);
void dumpLeftOutIcons(void);
int countLeftOutIcons(void);
void getDeviceNameFromPath(const char *path, char *device_name, int max_length);

#endif /* ICON_MISC_H */
