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
