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

#include "fileutils.h"

struct fmux_cookie
{
  size_t count;
  FILE **output_files;
};

FILE *fmux_openv(FILE *input, size_t outputfilecount, va_list ap)
{
  cookie_io_functions_t functions = {
    .read = NULL,
    .write = fmux_write,
    .seek = NULL,
    .close = fmux_close,
  };

  struct fmux_cookie *cookie = malloc(sizeof(struct fmux_cookie) + sizeof(FILE*) * outputfilecount);
  if(cookie == NULL) { return NULL; }
  cookie->count = outputfilecount;
  cookie->output_files = ((void *) cookie) + sizeof(cookie);
  for(size_t i = 0; i < outputfilecount; i++) { cookie->output_files[i] = va_arg(ap, FILE *); }
  FILE *this = fopencookie(cookie, "w", functions);
  return this;
}

FILE *fmux_open(FILE *input, size_t outputfilecount, ...)
{
  va_list ap;
  va_start(ap, outputfilecount);
  FILE *this = fmux_openv(input, outputfilecount, ap);
  va_end(ap);
  return this;
}

ssize_t fmux_write(void *tmp, const char *buf, size_t size)
{
  struct fmux_cookie *cookie = (struct fmux_cookie *) tmp;
  for(size_t i = 0; i < cookie->count; i++)
  {
    size_t written = fwrite(buf, size, 1, cookie->output_files[i]);
    if(written == EOF) return 0;
    return written;
  }
}

int fmux_close(void *cookie)
{
  free(cookie);
  return 0;
}
