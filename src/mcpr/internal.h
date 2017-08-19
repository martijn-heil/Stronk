#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "mcpr.h"
#include <ninio/logging.h>
static void debug_print(const char *filename, size_t filename_len, const char *func, size_t func_len, int line, ...)
{
    if(mcpr_logger == NULL) return;
    va_list ap;
    va_start(ap, line);
    const char *fmt = va_arg(ap, const char *);
    char final_fmt[strlen(fmt) + 1];
    if(sprintf(final_fmt, "%s", fmt) < 0) { va_end(ap); return; }
    logger_vwrite(mcpr_logger, filename, filename_len, func, func_len, line, LOG_LEVEL_DEBUG, final_fmt, ap);
    va_end(ap);
}

#define DEBUG_PRINT(...) debug_print(__FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, __VA_ARGS__);
