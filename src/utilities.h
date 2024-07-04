#ifndef utilities_h
#define utilities_h

#include <exec/types.h>
#include <graphics/rastport.h>
#include <stddef.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <math.h>
#include "main.h"


void CalculateTextExtent(const char *text, struct TextExtent *textExtent);
int Compare(const void *a, const void *b);
int strncasecmp_custom(const char *s1, const char *s2, size_t n);
BOOL does_file_or_folder_exist(const char *filename, int appendWorkingDirectory);
void trim(char *str);
void WaitChar(void);
void remove_CR_LF_from_string(char *str);
UWORD GetKickstartVersion(void);
int GetWorkbenchVersion(void);
#endif // utilities_h