#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
/* Platform Detection                                                        */
/*---------------------------------------------------------------------------*/

#if defined(__VBCC__) && defined(__AMIGA__)
    #define PLATFORM_AMIGA 1
    #define PLATFORM_HOST 0
#else
    #define PLATFORM_AMIGA 0
    #define PLATFORM_HOST 1
#endif

/*---------------------------------------------------------------------------*/
/* Memory Allocation Abstraction                                             */
/*---------------------------------------------------------------------------*/

#define whd_malloc(sz)  malloc(sz)
#define whd_free(p)     free(p)

/*---------------------------------------------------------------------------*/
/* Platform-Specific Includes                                                */
/*---------------------------------------------------------------------------*/

#if PLATFORM_AMIGA
    /* Amiga-specific headers will be included by modules that need them */
    /* This header provides only the base abstraction */
#endif

#endif /* PLATFORM_H */
