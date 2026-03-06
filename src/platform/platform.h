#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
/* Platform Selection                                                        */
/*---------------------------------------------------------------------------*/

#define PLATFORM_AMIGA 1

/*---------------------------------------------------------------------------*/
/* Memory Allocation Abstraction with Debugging                              */
/*---------------------------------------------------------------------------*/

/* Enable memory debugging by passing MEMTRACK=1 to make (e.g. "make MEMTRACK=1") */
/* Do NOT hardcode this here - it creates RAM:CRITICAL_FAILURE.log on every run */

#ifdef DEBUG_MEMORY_TRACKING
    /* Memory tracking functions - implemented in platform.c */
    void* whd_malloc_debug(size_t size, const char *file, int line);
    void whd_free_debug(void *ptr, const char *file, int line);
    char* whd_strdup_debug(const char *str, const char *file, int line);
    void whd_memory_report(void);
    void whd_memory_init(void);
    void whd_memory_suspend_logging(void);  /* Temporarily disable logging for bulk operations */
    void whd_memory_resume_logging(void);   /* Re-enable logging */
    void whd_memory_emergency_shutdown(const char *reason);  /* Emergency OOM handler */
    
    #define whd_malloc(sz)  whd_malloc_debug(sz, __FILE__, __LINE__)
    #define whd_free(p)     whd_free_debug(p, __FILE__, __LINE__)
    #define whd_strdup(s)   whd_strdup_debug(s, __FILE__, __LINE__)
#else
    /* Standard allocation without tracking */
    #define whd_malloc(sz)  malloc(sz)
    #define whd_free(p)     free(p)
    
    /* strdup wrapper */
    static inline char* whd_strdup(const char *str) {
        char *copy;
        if (!str) return NULL;
        copy = (char *)malloc(strlen(str) + 1);
        if (copy) strcpy(copy, str);
        return copy;
    }
    
    /* No-op functions when debugging disabled */
    #define whd_memory_report() ((void)0)
    #define whd_memory_init() ((void)0)
#endif

/*---------------------------------------------------------------------------*/
/* Platform-Specific Includes                                                */
/*---------------------------------------------------------------------------*/

#if PLATFORM_AMIGA
    /* Amiga-specific headers will be included by modules that need them */
    /* This header provides only the base abstraction */
#endif

#endif /* PLATFORM_H */
