/* Platform abstraction layer implementation
 * 
 * Memory tracking system for debugging memory leaks
 * Logs allocations/deallocations through writeLog.c LOG_MEMORY category
 */

#include "platform.h"

#ifdef DEBUG_MEMORY_TRACKING

/* We need writeLog.h for logging - include the actual path */
#include "../../src/writeLog.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Amiga-specific memory allocation (use Fast RAM when available) */
#if defined(__AMIGA__)
    #include <exec/types.h>
    #include <exec/memory.h>
    #include <proto/exec.h>
#endif

/*---------------------------------------------------------------------------*/
/* Memory Tracking Data Structures                                           */
/*---------------------------------------------------------------------------*/

typedef struct MemoryBlock {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    struct MemoryBlock *next;
} MemoryBlock;

static MemoryBlock *g_memoryList = NULL;
static size_t g_totalAllocated = 0;
static size_t g_totalFreed = 0;
static size_t g_currentAllocated = 0;
static size_t g_peakAllocated = 0;
static size_t g_allocationCount = 0;
static size_t g_freeCount = 0;
static int g_loggingSuspended = 0;  /* Flag to temporarily disable logging */

/*---------------------------------------------------------------------------*/
/* Memory Tracking Implementation                                            */
/*---------------------------------------------------------------------------*/

void whd_memory_init(void) {
    g_memoryList = NULL;
    g_totalAllocated = 0;
    g_totalFreed = 0;
    g_currentAllocated = 0;
    g_peakAllocated = 0;
    g_allocationCount = 0;
    g_freeCount = 0;
    g_loggingSuspended = 0;
    
    log_info(LOG_MEMORY, "Memory tracking initialized\n");
}

void whd_memory_suspend_logging(void) {
    g_loggingSuspended = 1;
}

void whd_memory_resume_logging(void) {
    g_loggingSuspended = 0;
}

void* whd_malloc_debug(size_t size, const char *file, int line) {
    void *ptr;
    MemoryBlock *block;
    
    /* Allocate the actual memory - prefer Fast RAM on Amiga */
#if defined(__AMIGA__)
    /* MEMF_ANY allows allocator to use Fast RAM if available, falls back to Chip */
    ptr = AllocVec(size, MEMF_ANY | MEMF_CLEAR);
#else
    ptr = malloc(size);
#endif
    
    if (!ptr) {
        log_error(LOG_MEMORY, "malloc(%lu) FAILED at %s:%d\n", 
                 (unsigned long)size, file, line);
        return NULL;
    }
    
    /* Create tracking block - also use Fast RAM if available */
#if defined(__AMIGA__)
    block = (MemoryBlock *)AllocVec(sizeof(MemoryBlock), MEMF_ANY | MEMF_CLEAR);
#else
    block = (MemoryBlock *)malloc(sizeof(MemoryBlock));
#endif
    
    if (!block) {
        /* If we can't track it, still return the memory but log warning */
        log_warning(LOG_MEMORY, "Failed to allocate tracking block for %lu bytes at %s:%d\n",
                   (unsigned long)size, file, line);
        return ptr;
    }
    
    /* Fill in tracking information */
    block->ptr = ptr;
    block->size = size;
    block->file = file;
    block->line = line;
    block->next = g_memoryList;
    g_memoryList = block;
    
    /* Update statistics */
    g_totalAllocated += size;
    g_currentAllocated += size;
    g_allocationCount++;
    
    if (g_currentAllocated > g_peakAllocated) {
        g_peakAllocated = g_currentAllocated;
    }
    
    /* Only log if not suspended */
    if (!g_loggingSuspended) {
        log_debug(LOG_MEMORY, "ALLOC: %lu bytes at 0x%08lx (%s:%d) [Current: %lu, Peak: %lu]\n",
                 (unsigned long)size, (unsigned long)ptr, file, line,
                 (unsigned long)g_currentAllocated, (unsigned long)g_peakAllocated);
    }
    
    return ptr;
}

void whd_free_debug(void *ptr, const char *file, int line) {
    MemoryBlock *block, *prev;
    
    if (!ptr) {
        /* Freeing NULL is legal but worth noting */
        if (!g_loggingSuspended) {
            log_debug(LOG_MEMORY, "FREE: NULL pointer at %s:%d (ignored)\n", file, line);
        }
        return;
    }
    
    /* Find the tracking block */
    prev = NULL;
    block = g_memoryList;
    while (block) {
        if (block->ptr == ptr) {
            /* Found it - remove from list */
            if (prev) {
                prev->next = block->next;
            } else {
                g_memoryList = block->next;
            }
            
            /* Update statistics */
            g_totalFreed += block->size;
            g_currentAllocated -= block->size;
            g_freeCount++;
            
            /* Only log if not suspended */
            if (!g_loggingSuspended) {
                log_debug(LOG_MEMORY, "FREE: %lu bytes at 0x%08lx (allocated %s:%d, freed %s:%d) [Current: %lu]\n",
                         (unsigned long)block->size, (unsigned long)ptr, 
                         block->file, block->line, file, line,
                         (unsigned long)g_currentAllocated);
            }
            
            /* Free the tracking block and actual memory - use FreeVec on Amiga */
#if defined(__AMIGA__)
            FreeVec(block);
            FreeVec(ptr);
#else
            free(block);
            free(ptr);
#endif
            return;
        }
        prev = block;
        block = block->next;
    }
    
    /* If we get here, we're freeing memory we never tracked */
    if (!g_loggingSuspended) {
        log_error(LOG_MEMORY, "Attempting to free UNTRACKED pointer 0x%08lx at %s:%d\n",
                 (unsigned long)ptr, file, line);
    }
    
    /* Still free it to avoid leaks, but this is suspicious */
    free(ptr);
}

char* whd_strdup_debug(const char *str, const char *file, int line) {
    char *copy;
    size_t len;
    
    if (!str) {
        log_debug(LOG_MEMORY, "STRDUP: NULL string at %s:%d (returning NULL)\n", file, line);
        return NULL;
    }
    
    len = strlen(str) + 1;
    copy = (char *)whd_malloc_debug(len, file, line);
    if (copy) {
        strcpy(copy, str);
    }
    
    return copy;
}

void whd_memory_report(void) {
    MemoryBlock *block;
    size_t leakCount = 0;
    size_t leakBytes = 0;
    
    log_info(LOG_MEMORY, "\n========== MEMORY TRACKING REPORT ==========\n");
    log_info(LOG_MEMORY, "Total allocations: %lu (%lu bytes)\n", 
             (unsigned long)g_allocationCount, (unsigned long)g_totalAllocated);
    log_info(LOG_MEMORY, "Total frees: %lu (%lu bytes)\n",
             (unsigned long)g_freeCount, (unsigned long)g_totalFreed);
    log_info(LOG_MEMORY, "Peak memory usage: %lu bytes (%lu KB)\n", 
             (unsigned long)g_peakAllocated, (unsigned long)(g_peakAllocated / 1024));
    log_info(LOG_MEMORY, "Current memory: %lu bytes\n", (unsigned long)g_currentAllocated);
    
    /* Count leaks */
    block = g_memoryList;
    while (block) {
        leakCount++;
        leakBytes += block->size;
        block = block->next;
    }
    
    if (leakCount > 0) {
        log_error(LOG_MEMORY, "\n*** MEMORY LEAKS DETECTED: %lu blocks, %lu bytes ***\n",
                 (unsigned long)leakCount, (unsigned long)leakBytes);
        log_error(LOG_MEMORY, "\nLeak details:\n");
        
        block = g_memoryList;
        while (block) {
            log_error(LOG_MEMORY, "  - %lu bytes at 0x%08lx (allocated at %s:%d)\n",
                     (unsigned long)block->size, (unsigned long)block->ptr,
                     block->file, block->line);
            block = block->next;
        }
        log_error(LOG_MEMORY, "\n*** CHECK error.log FOR COMPLETE LEAK REPORT ***\n");
    } else {
        log_info(LOG_MEMORY, "\n*** NO MEMORY LEAKS DETECTED - ALL ALLOCATIONS FREED ***\n");
    }
    
    log_info(LOG_MEMORY, "============================================\n\n");
}

#endif /* DEBUG_MEMORY_TRACKING */
