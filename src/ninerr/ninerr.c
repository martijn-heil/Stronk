/*
  MIT License

  Copyright (c) 2017-2020 Martijn Heil

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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <features.h>

#if defined(__GNU_LIBRARY__) || defined(__GLIBC__)
  #include <execinfo.h>
#endif

#include <ninerr/ninerr.h>

thread_local struct ninerr *ninerr;
struct ninerr ninerr_out_of_memory_struct;


bool ninerr_init(void)
{
  ninerr_out_of_memory_struct.message = "Out of memory";
  ninerr_out_of_memory_struct.free = NULL;
  ninerr_out_of_memory_struct.child = NULL;
  ninerr_out_of_memory_struct.cause = NULL;
  ninerr_out_of_memory_struct.type = "ninerr_out_of_memory";

  return true;
}

struct ninerr *ninerr_arithmetic_new(void)
{
  struct ninerr *err = ninerr_new("Arithmetic error");
  if(err == NULL) return NULL;
  err->type = "ninerr_artihmetic";
  return err;
}

bool ninerr_is(const struct ninerr *err, const char *t)
{
  return strcmp(err->type, t) == 0;
}

int ninerr_print(const struct ninerr *err)
{
  return ninerr_fprint(stderr, err);
}

int ninerr_fprint(FILE *fp, const struct ninerr *err)
{
  if(err == NULL) return 0;

  if(err->message != NULL && err->stacktrace != NULL)
  {
    return fprintf(fp, "%s: %s\n%s", err->type, err->message, err->stacktrace);
  }
  else if(err->stacktrace != NULL)
  {
    return fprintf(fp, "%s:\n%s", err->type, err->stacktrace);
  }
  else if(err->message != NULL)
  {
    return fprintf(fp, "%s: %s\n", err->type, err->message);
  }
  return -1;
}

int ninerr_print_g(void)
{
  return ninerr_print(ninerr);
}

int ninerr_fprint_g(FILE *fp)
{
  return ninerr_fprint(fp, ninerr);
}

static void ninerr_free(struct ninerr *err)
{
  if(err->stacktrace != NULL) free(err->stacktrace);
  free(err);
}

struct ninerr *ninerr_vnew(const char *fmt, va_list ap)
{
  size_t required_message_length = 0;
  if(fmt != NULL)
  {
    char tmpbuf;
    va_list ap2;
    va_copy(ap2, ap);
    int result = vsnprintf(&tmpbuf, 1, fmt, ap2);
    va_end(ap2);
    if(result >= 0) required_message_length = result; else fprintf(stderr, "Could not format message for new ninerr.");
  }

  size_t required_size = sizeof(struct ninerr);
  if(required_message_length > 0) required_size += (required_message_length + 1);

  void *membuf = malloc(required_size);
  if(membuf == NULL) { va_end(ap); return NULL; }
  struct ninerr *err = membuf;


  if(required_message_length > 0)
  {
    err->message = (char *) (membuf + sizeof(struct ninerr));
    va_list ap3;
    va_copy(ap3, ap);
    int result = vsprintf(err->message, fmt, ap3);
    va_end(ap3);
    if(result == -1) err->message = NULL;
  }
  else
  {
    err->message = NULL;
  }

  err->type = "ninerr";
  err->child = NULL;
  err->cause = NULL;
  err->free = ninerr_free;

  #if defined(__GNU_LIBRARY__) || defined(__GLIBC__)
    #define MAX_STACK_LEVELS 50
    void *buffer[MAX_STACK_LEVELS];
    int levels = backtrace(buffer, MAX_STACK_LEVELS);

    //backtrace_symbols_fd(buffer, levels, 2);
    char **symbols = backtrace_symbols(buffer, levels);
    if(symbols == NULL) { err->stacktrace = NULL; va_end(ap); return err; }
    size_t total_size = 0;
    for(unsigned int i = 0; i < (unsigned int) levels; i++)
    {
      total_size += strlen(symbols[i]) + 4; // \n + a + t +' '
    }

    err->stacktrace = malloc(total_size + 1);
    if(err->stacktrace == NULL) { free(symbols); va_end(ap); return err; }

    char *ptr = err->stacktrace;
    for(unsigned int i = 0; i < (unsigned int) levels; i++)
    {
      ptr[0] = 'a';
      ptr[1] = 't';
      ptr[2] = ' ';
      size_t len = strlen(symbols[i]);
      memcpy(ptr + 3, symbols[i], len);
      ptr[3 + len] = '\n';
      ptr += 3 + len + 1;
    }
    *ptr = '\0';
    free(symbols);
  #else
    err->stacktrace = NULL;
  #endif

  va_end(ap);
  return err;
}

struct ninerr *ninerr_new(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  struct ninerr *result = ninerr_vnew(fmt, ap);
  va_end(ap);
  return result;
}


void ninerr_finish(void)
{
  ninerr_cleanup_latest();
}

void ninerr_set_err(struct ninerr *err)
{
  ninerr_cleanup_latest();
  if(err == NULL)
  {
    ninerr = ninerr_new(NULL);
  }
  else
  {
    ninerr = err;
  }
}

void ninerr_cleanup_latest(void)
{
  if(ninerr != NULL && ninerr->free != NULL) ninerr->free(ninerr);
  ninerr = NULL;
}

struct ninerr *ninerr_closed_new(char *message, bool free_message)
{
  struct ninerr *err;
  if(message != NULL)
  {
    err = ninerr_new(message, free_message);
  }
  else
  {
    err = ninerr_new("Connection closed.");
  }
  if(err == NULL) return NULL;
  err->type = "ninerr_closed";
  return err;
}

struct ninerr *ninerr_wouldblock_new(void)
{
  struct ninerr *err = ninerr_new("Operation would block.");
  if(err == NULL) return NULL;
  err->type = "ninerr_wouldblock";
  return err;
}

struct ninerr *ninerr_from_errno(void) // TODO implement other errno values.
{
  switch(errno)
  {
    case ENOMEM:
      return &ninerr_out_of_memory_struct;

    case ECONNRESET:
      return ninerr_closed_new(NULL, false);

    case EAGAIN: // fallthrough
//    case EWOULDBLOCK:
      return ninerr_wouldblock_new();

    default: return ninerr_new(strerror(errno));
  }
}
