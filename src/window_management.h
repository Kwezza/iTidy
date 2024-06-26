#ifndef WINDOW_MANAGEMENT_H
#define WINDOW_MANAGEMENT_H

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
#include "file_directory_handling.h"
#include "utilities.h"

void CleanupWindow(void);
int FormatIconsAndWindow(char *folder);
void resizeFolderToContents(char *dirPath, IconArray *iconArray);
int InitializeWindow(void);
void repoistionWindow(char *dirPath, int winWidth, int winHeight);

#endif // WINDOW_MANAGEMENT_H
