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



  mcpr.c - Minecraft Protocol functions
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <sys/types.h>

#include <zlib.h>

#include <ninerr/ninerr.h>
#include "codec.h"
#include "mcpr.h"
#include "../util.h"

struct logger *mcpr_logger = NULL; // set to non null value to enable logging, set to null value to disable logging.

IGNORE("-Wtype-limits")
size_t mcpr_compress_bounds(size_t len) {
  uLongf calc_len = compressBound(len);
  if(calc_len > SIZE_MAX || calc_len < 0) { fprintf(stderr, "Aborted at mcpr.c:%i in mcpr_compress_bounds function, calculated length does not fit within size_t", __LINE__); abort(); }
  return calc_len;
}

ssize_t mcpr_compress(void *out, const void *in, size_t n) {
  uLongf dest_len = compressBound(n);
  int result = compress((Bytef *) out, &dest_len, (Bytef *) in, n);


  if(result == Z_MEM_ERROR) {
    ninerr_set_err(&ninerr_out_of_memory_struct);
    return -1;
  } else if(result == Z_BUF_ERROR) {
    ninerr_set_err(NULL);
    return -1;
  } else {
    if(dest_len > SIZE_MAX || dest_len < 0) { ninerr = NULL; return -1; }
    return dest_len; // Compressed out size.
  }
}

ssize_t mcpr_decompress(void *out, const void *in, size_t max_out_size, size_t in_size) {
  uLongf dest_len = max_out_size;
  int result = uncompress((Bytef *) out, &dest_len, (Bytef *) in, in_size);


  if(result == Z_MEM_ERROR) {
    ninerr_set_err(&ninerr_out_of_memory_struct);
    return -1;
  } else if(result == Z_BUF_ERROR) {
    ninerr_set_err(NULL);
    return -1;
  } else if(result == Z_DATA_ERROR) {
    ninerr_set_err(NULL);
    return -1;
  } else {
    if(dest_len > SIZE_MAX || dest_len < 0) { ninerr = NULL; return -1; }
    return dest_len;
  }
}
END_IGNORE()

char *mcpr_as_chat(const char *message_fmt, ...)
{
  va_list args;
  va_start(args, message_fmt);

  char tmp;
  va_list args1;
  va_copy(args1, args);
  int bytes_required = vsnprintf(&tmp, 1, message_fmt, args1);
  va_end(args1);
  if(bytes_required < 0) { va_end(args); ninerr_set_err(ninerr_new("vsnprintf failed.")); return NULL; }
  bytes_required++; // For the NUL byte

  char *message;
  bool free_message = false;
  if(bytes_required <= 256)
  {
    char buf[bytes_required];
    message = buf;
  }
  else
  {
    message = malloc(bytes_required);
    if(message == NULL) { va_end(args); ninerr_set_err(ninerr_from_errno()); return NULL; }
    free_message = true;
  }
  message = malloc(bytes_required);
  if(message == NULL) { va_end(args); ninerr_set_err(ninerr_from_errno()); return NULL; }
  free_message = true;
  va_list args2;
  va_copy(args2, args);
  int message_length = vsprintf(message, message_fmt, args2);
  va_end(args2);
  if(message_length < 0) { if(free_message ) { free(message); } va_end(args); ninerr_set_err(ninerr_new("vsprintf failed.")); return NULL; }

  const char *fmt = "{\"text\":\"%s\"}";
  char *buf = malloc(12 + message_length); // Important! Change 12 if you change the fmt above.
  if(buf == NULL) { if(free_message ) { free(message); } va_end(args); ninerr_set_err(ninerr_from_errno()); return NULL; }
  int result = sprintf(buf, fmt, message);
  if(result < 0) { free(buf); if(free_message ) { free(message); } va_end(args); ninerr_set_err(ninerr_new("sprintf failed.")); return NULL; }

  va_end(args);
  return buf;
}

const char *mcpr_state_to_string(enum mcpr_state state)
{
  switch(state)
  {
    case MCPR_STATE_HANDSHAKE:  return "handshake";
    case MCPR_STATE_LOGIN:    return"login";
    case MCPR_STATE_STATUS:   return "status";
    case MCPR_STATE_PLAY:     return "play";
    default: abort(); return NULL;
  }
}
