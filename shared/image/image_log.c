/*
 * image_log.c - Optional logging for shared image kernels
 *
 * Default is silent. Front-ends may install a callback with image_set_log_fn().
 */

#include "image_types.h"

#include <stdarg.h>
#include <stdio.h>

static iTidy_ImageLogFn g_image_log_fn = NULL;

void image_set_log_fn(iTidy_ImageLogFn fn)
{
    g_image_log_fn = fn;
}

static void image_log_va(int level, const char *fmt, va_list ap)
{
    char buf[256];

    if (g_image_log_fn == NULL || fmt == NULL)
        return;

    vsnprintf(buf, sizeof(buf), fmt, ap);
    buf[sizeof(buf) - 1] = '\0';
    g_image_log_fn(level, buf);
}

void image_log_debug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    image_log_va(ITIDY_IMAGE_LOG_DEBUG, fmt, ap);
    va_end(ap);
}

void image_log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    image_log_va(ITIDY_IMAGE_LOG_INFO, fmt, ap);
    va_end(ap);
}

void image_log_warning(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    image_log_va(ITIDY_IMAGE_LOG_WARNING, fmt, ap);
    va_end(ap);
}

void image_log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    image_log_va(ITIDY_IMAGE_LOG_ERROR, fmt, ap);
    va_end(ap);
}
