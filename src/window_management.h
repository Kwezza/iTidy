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
#include <proto/intuition.h>
#include <proto/diskfont.h>
#include <intuition/intuition.h>
#include <graphics/text.h>


#include "icon_management.h"
#include "file_directory_handling.h"
#include "utilities.h"
#include "writeLog.h"
#include "icon_misc.h"
#include "layout_preferences.h"

void CleanupWindow(void);
void resizeFolderToContents(char *dirPath, IconArray *iconArray, 
                           const LayoutPreferences *prefs);
int InitializeWindow(void);
void repoistionWindow(char *dirPath, int winWidth, int winHeight, const LayoutPreferences *prefs);
void SetIconFont(void); 

#endif // WINDOW_MANAGEMENT_H
