/*
 * image_types.h - Neutral types for the shared image kernels
 *
 * Target: 68000+, Workbench 2.x+, C89/C99 subset
 */

#ifndef ITIDY_IMAGE_TYPES_H
#define ITIDY_IMAGE_TYPES_H

#include <exec/types.h>

/*========================================================================*/
/* RGB colour                                                             */
/*========================================================================*/

/**
 * Packed 8-bit RGB. Layout matches Amiga struct ColorRegister
 * { UBYTE red, green, blue; } so a cast is valid.
 */
typedef struct iTidy_RGB8
{
    UBYTE r;
    UBYTE g;
    UBYTE b;
} iTidy_RGB8;

/*========================================================================*/
/* Optional progress callback                                             */
/*========================================================================*/

/**
 * Return TRUE to continue, FALSE to cancel.
 * fn may be NULL (no reporting, never cancel).
 */
typedef BOOL (*iTidy_ImageProgressFn)(void *user_data,
                                      const char *phase,
                                      ULONG current,
                                      ULONG total);

typedef struct iTidy_ImageProgress
{
    iTidy_ImageProgressFn fn;
    void *user_data;
} iTidy_ImageProgress;

/*========================================================================*/
/* Optional logging                                                       */
/*========================================================================*/

#define ITIDY_IMAGE_LOG_DEBUG    0
#define ITIDY_IMAGE_LOG_INFO     1
#define ITIDY_IMAGE_LOG_WARNING  2
#define ITIDY_IMAGE_LOG_ERROR    3

typedef void (*iTidy_ImageLogFn)(int level, const char *message);

void image_set_log_fn(iTidy_ImageLogFn fn);

void image_log_debug(const char *fmt, ...);
void image_log_info(const char *fmt, ...);
void image_log_warning(const char *fmt, ...);
void image_log_error(const char *fmt, ...);

#endif /* ITIDY_IMAGE_TYPES_H */
