#ifndef SPINNER_H
#define SPINNER_H

#include <exec/types.h>
#include "writeLog.h"
#include "icon_misc.h"

// Function prototypes
ULONG spinnerTimer(void);
void updateCursor(void);
int setupTimer(void);
void disposeTimer(void);
void eraseSpinner(void);

#endif // SPINNER_H
