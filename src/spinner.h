#ifndef SPINNER_H
#define SPINNER_H

#include <exec/types.h>

// Function prototypes
ULONG timer(void);
void updateCursor(void);
int setupTimer(void);
void disposeTimer(void);

#endif // SPINNER_H
