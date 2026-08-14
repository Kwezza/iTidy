#ifndef EXEC_TYPES_H
#define EXEC_TYPES_H

/* Minimal Amiga type stubs for host (GCC) compilation of shared image tests. */

typedef unsigned char  UBYTE;
typedef signed char    BYTE;
typedef unsigned short UWORD;
typedef short          WORD;
typedef unsigned int   ULONG;
typedef int            LONG;
typedef short          BOOL;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* EXEC_TYPES_H */
