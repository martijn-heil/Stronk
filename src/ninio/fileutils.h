#ifndef NINIO_FILEUTILS_H
#define NINIO_FILEUTILS_H

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>

FILE *fmux_open(FILE *input, size_t outputfilecount, ...);
FILE *fmux_openv(FILE *input, size_t outputfilecount, va_list ap);

ssize_t fmux_write(void *tmp, const char *buf, size_t size);
int fmux_close(void *cookie);

#endif
