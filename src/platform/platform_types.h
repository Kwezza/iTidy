#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

/* VBCC MIGRATION NOTE (Stage 2): Platform type abstraction
 * 
 * This file provides type definitions for platform-specific file handles
 * and other platform-dependent types used throughout iTidy.
 */

#include "platform.h"

/*---------------------------------------------------------------------------*/
/* Platform-Specific Type Definitions                                        */
/*---------------------------------------------------------------------------*/

#if PLATFORM_AMIGA
    /* Amiga-specific types */
    #include <dos/dos.h>
    
    /* BPTR is the Amiga file/lock handle type (shifted pointer) */
    typedef BPTR platform_bptr;
    
#else
    /* Host platform types (stub for non-Amiga builds) */
    typedef void* platform_bptr;
    
#endif

#endif /* PLATFORM_TYPES_H */
