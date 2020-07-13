/*
  MIT License

  Copyright (c) 2016-2020 Martijn Heil

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
