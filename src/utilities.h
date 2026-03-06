#ifndef utilities_h
#define utilities_h

/* VBCC MIGRATION NOTE (Stage 2): Updated for AmigaDOS type consistency
 * 
 * Changes:
 * - Changed does_file_or_folder_exist() return type from bool to BOOL
 * - Added proper AmigaDOS type usage throughout
 * - Maintained platform abstraction
 */

#define utilities_h
#include <graphics/text.h>
#include <exec/types.h>
#include <graphics/rastport.h>
#include <stddef.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <math.h>

#include "writeLog.h"
#include "icon_misc.h"

void CalculateTextExtent(const char *text, struct TextExtent *textExtent);
int Compare(const void *a, const void *b);
int strncasecmp_custom(const char *s1, const char *s2, size_t n);
BOOL does_file_or_folder_exist(const char *filename, int appendWorkingDirectory);
void trim(char *str);
void WaitChar(void);
void remove_CR_LF_from_string(char *str);
UWORD GetKickstartVersion(void);
int GetWorkbenchVersion(void);
char* convertWBVersionWithDot(int number);
int endsWithInfo(const char *filePath);
char *removeTextFromStartOfString(const char *str, const char *prefix);
BOOL ExpandProgDir(const char *path, char *expanded, size_t maxLen);
BOOL CreateDirectoryForFile(const char *filepath);
BOOL CreateDirectoryPath(const char *dirpath);
void DumpWorkbenchScreenPalette(void);

#endif /* utilities_h */
