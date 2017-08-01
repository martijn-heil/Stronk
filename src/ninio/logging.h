/*
    MIT License

    Copyright (c) 2017 Martijn Heil

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef NINIO_LOGGING_H
#define NINIO_LOGGING_H

#include <stdlib.h>
#include <stddef.h>

#include "util.h"

enum log_level
{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_NOTICE,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
};

struct logger
{
    void (*vwrite)(const char *filename, size_t filename_len, const char *func, size_t func_len, int line, enum log_level level, const char *fmt, va_list ap);
};
static inline void logger_vwrite(const struct logger *logger, const char *filename, size_t filename_len, const char *func, size_t func_len, int line,
     enum log_level level, const char *fmt, va_list ap)
{
    if(logger == NULL || logger->vwrite == NULL) return;
    logger->vwrite(filename, filename_len, func, func_len, line, level, fmt, ap);
}

static inline void logger_write(const struct logger *logger, const char *filename, size_t filename_len, const char *func, size_t func_len, int line, enum log_level level, ...)
{
    va_list ap;
    va_start(ap, level);
    const char *fmt = va_arg(ap, const char *);
    logger_vwrite(logger, filename, filename_len, func, func_len, line, level, fmt, ap);
    va_end(ap);
}

#define log_fatal(logger, ...)  logger_write(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_FATAL, __VA_ARGS__)
#define log_error(logger, ...)  logger_write(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_ERROR, __VA_ARGS__)
#define log_warn(logger, ...)   logger_write(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_WARN, __VA_ARGS__)
#define log_notice(logger, ...) logger_write(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_NOTICE, __VA_ARGS__)
#define log_info(logger, ...)   logger_write(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_INFO, __VA_ARGS__)
#define log_debug(logger, ...)  logger_write(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_DEBUG, __VA_ARGS__)

#define vlog_fatal(logger, fmt, ap)  logger_vwrite(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_FATAL, fmt, ap)
#define vlog_error(logger, fmt, ap)  logger_vwrite(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_ERROR, fmt, ap)
#define vlog_warn(logger, fmt, ap)   logger_vwrite(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_WARN, fmt, ap)
#define vlog_notice(logger, fmt, ap) logger_vwrite(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_NOTICE, fmt, ap)
#define vlog_info(logger, fmt, ap)   logger_vwrite(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_INFO, fmt, ap)
#define vlog_debug(logger, fmt, ap)  logger_vwrite(logger, __FILENAME__, sizeof(__FILENAME__)-1, __func__, sizeof(__func__)-1, __LINE__, LOG_LEVEL_DEBUG, fmt, ap)

#endif
