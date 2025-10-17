#ifndef PLATFORM_IO_H
#define PLATFORM_IO_H

#include "platform.h"
#include "platform_types.h"

/*---------------------------------------------------------------------------*/
/* File I/O Abstraction                                                      */
/*---------------------------------------------------------------------------*/

#if PLATFORM_AMIGA
    /* Amiga uses BPTR for file handles */
    typedef platform_bptr platform_file_handle;

    platform_file_handle platform_fopen(const char *path, const char *mode);
    void platform_fclose(platform_file_handle handle);
    size_t platform_fread(void *ptr, size_t size, size_t nmemb, platform_file_handle handle);
    size_t platform_fwrite(const void *ptr, size_t size, size_t nmemb, platform_file_handle handle);
    int platform_fseek(platform_file_handle handle, long offset, int whence);
    long platform_ftell(platform_file_handle handle);

#else
    /* Host uses standard FILE* */
    typedef FILE* platform_file_handle;

    #define platform_fopen(path, mode)                  fopen(path, mode)
    #define platform_fclose(handle)                     fclose(handle)
    #define platform_fread(ptr, size, nmemb, handle)    fread(ptr, size, nmemb, handle)
    #define platform_fwrite(ptr, size, nmemb, handle)   fwrite(ptr, size, nmemb, handle)
    #define platform_fseek(handle, offset, whence)      fseek(handle, offset, whence)
    #define platform_ftell(handle)                      ftell(handle)

#endif

/*---------------------------------------------------------------------------*/
/* String Comparison Abstraction                                             */
/*---------------------------------------------------------------------------*/

#if PLATFORM_AMIGA
    /* Amiga has stricmp built-in */
    #define platform_stricmp    stricmp
    #define platform_strnicmp   strnicmp
#else
    /* Host platforms use different names */
    #ifdef _WIN32
        #define platform_stricmp    _stricmp
        #define platform_strnicmp   _strnicmp
    #else
        #define platform_stricmp    strcasecmp
        #define platform_strnicmp   strncasecmp
    #endif
#endif

#endif /* PLATFORM_IO_H */
