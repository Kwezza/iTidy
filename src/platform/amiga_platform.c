#include <platform/platform.h>

#if PLATFORM_AMIGA

#include <platform/amiga_headers.h>
#include <platform/platform_io.h>

/*---------------------------------------------------------------------------*/
/* Amiga File I/O Implementation                                             */
/*---------------------------------------------------------------------------*/

/**
 * Opens a file on Amiga using DOS library.
 *
 * @param path File path to open
 * @param mode File access mode ("r", "w", "a")
 * @return File handle or NULL on failure
 */
platform_file_handle platform_fopen(const char *path, const char *mode)
{
    LONG access_mode = MODE_OLDFILE;

    if (mode[0] == 'w' || mode[0] == 'a')
    {
        access_mode = MODE_NEWFILE;
    }

    return Open(path, access_mode);
}

/**
 * Closes a file handle on Amiga.
 *
 * @param handle File handle to close
 */
void platform_fclose(platform_file_handle handle)
{
    if (handle)
    {
        Close(handle);
    }
}

/**
 * Reads data from an Amiga file handle.
 *
 * @param ptr Pointer to buffer to read into
 * @param size Size of each element
 * @param nmemb Number of elements to read
 * @param handle File handle to read from
 * @return Number of elements successfully read
 */
size_t platform_fread(void *ptr, size_t size, size_t nmemb, platform_file_handle handle)
{
    LONG bytes_to_read = size * nmemb;
    LONG bytes_read = Read(handle, ptr, bytes_to_read);

    if (bytes_read < 0)
    {
        return 0;
    }

    return bytes_read / size;
}

/**
 * Writes data to an Amiga file handle.
 *
 * @param ptr Pointer to data to write
 * @param size Size of each element
 * @param nmemb Number of elements to write
 * @param handle File handle to write to
 * @return Number of elements successfully written
 */
size_t platform_fwrite(const void *ptr, size_t size, size_t nmemb, platform_file_handle handle)
{
    LONG bytes_to_write = size * nmemb;
    LONG bytes_written = Write(handle, (void *)ptr, bytes_to_write);

    if (bytes_written < 0)
    {
        return 0;
    }

    return bytes_written / size;
}

/**
 * Seeks to a position in an Amiga file.
 *
 * @param handle File handle
 * @param offset Offset to seek to
 * @param whence Starting position (SEEK_SET, SEEK_CUR, SEEK_END)
 * @return 0 on success, -1 on failure
 */
int platform_fseek(platform_file_handle handle, long offset, int whence)
{
    LONG dos_whence;

    switch (whence)
    {
        case SEEK_SET:
            dos_whence = OFFSET_BEGINNING;
            break;
        case SEEK_CUR:
            dos_whence = OFFSET_CURRENT;
            break;
        case SEEK_END:
            dos_whence = OFFSET_END;
            break;
        default:
            return -1;
    } /* switch */

    return (Seek(handle, offset, dos_whence) == -1) ? -1 : 0;
}

/**
 * Gets current position in an Amiga file.
 *
 * @param handle File handle
 * @return Current file position or -1 on error
 */
long platform_ftell(platform_file_handle handle)
{
    return Seek(handle, 0, OFFSET_CURRENT);
}

#endif /* PLATFORM_AMIGA */

/* End of Text */
