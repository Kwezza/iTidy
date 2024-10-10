#ifndef FILE_DIRECTORY_HANDLING_H
#define FILE_DIRECTORY_HANDLING_H

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
#include "icon_management.h"
#include "window_management.h"
#include "utilities.h"

int HasSlaveFile(char *path);
void ProcessDirectory(char *path, BOOL processSubDirs);
void GetFullPath(const char *directory, struct FileInfoBlock *fib, char *fullPath, int fullPathSize);
int IsRootDirectorySimple(char *path);
void removeInfoExtension(const char *input, char *output);
int saveIconsPositionsToDisk(IconArray *iconArray);
void SaveFolderSettings(const char *folderPath, folderWindowSize *newFolderInfo);
void dumpIconArrayToScreen(IconArray *iconArray);
void sanitizeAmigaPath(char *path);
void setWriteProtection(const char *filename, BOOL enableWriteProtection);
BOOL isFileWriteProtected(const char *filename);

#endif // FILE_DIRECTORY_HANDLING_H
